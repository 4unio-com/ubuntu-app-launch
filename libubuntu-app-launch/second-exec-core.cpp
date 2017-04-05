/*
 * Copyright 2013-2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Ted Gould <ted.gould@canonical.com>
 *     Pete Woods <pete.woods@canonical.com>
 */

#include <gio/gio.h>
#include "libubuntu-app-launch/ubuntu-app-launch.h"
#include "second-exec-core.h"
#include "signal-unsubscriber.h"
#include "ubuntu-app-launch-trace.h"
#include "ual-tracepoint.h"

#include <unity/util/GlibMemory.h>

#include <iomanip>
#include <sstream>
#include <unordered_map>

using namespace std;
using namespace unity::util;

namespace ubuntu
{
namespace app_launch
{
namespace
{

struct GSourceClear
{
	void operator()(GSource* source)
	{
		if (source != nullptr)
		{
			g_source_destroy(source);
			g_source_unref(source);
		}
	}
};


class second_exec_t: public enable_shared_from_this<second_exec_t>
{
public:
	second_exec_t(shared_ptr<GDBusConnection> bus, const string& appid, const string& instanceid, const vector<Application::URL>& input_uris, GPid app_pid);

	~second_exec_t();

	void
	init ();

private:
	class get_pid_t
	{
	public:
		get_pid_t(const string& name, shared_ptr<GDBusConnection> bus, shared_ptr<second_exec_t> data);

	private:
		static void
		get_pid_cb (GObject * object, GAsyncResult * res, gpointer user_data)
		{
			((get_pid_t *)user_data)->get_pid(object, res);
		}

		void
		get_pid (GObject * object, GAsyncResult * res);

		string name;

		shared_ptr<second_exec_t> data;
	};

	/* Unity didn't respond in time, continue on */
	static gboolean
	timer_cb (gpointer user_data);

	static void
	send_open_cb (GObject * object, GAsyncResult * res, gpointer user_data)
	{
		((second_exec_t*) user_data)->send_open(object, res);
	}

	void
	app_id_to_dbus_path ();

	void
	send_open (GObject * object, GAsyncResult * res);

	void
	contact_app (GDBusConnection * bus, const string& dbus_name);

	void
	find_appid_pid ();

	void
	connection_count_dec ();

	static void
	unity_resume_cb (GDBusConnection * connection, const gchar * sender, const gchar * path, const gchar * interface, const gchar * signal, GVariant * params, gpointer user_data)
	{
		((second_exec_t*) user_data)->unity_resume(connection, sender, path, interface, signal, params);
	}

	void
	unity_resume(GDBusConnection * connection, const gchar * sender, const gchar * path, const gchar * interface, const gchar * signal, GVariant * params);

	void
	parse_uris ();

	static GSourceUPtr
	thread_default_timeout (guint interval, GSourceFunc func, gpointer data);

	void finish();

	static unordered_map<second_exec_t*, shared_ptr<second_exec_t>> SECOND_EXEC_INSTANCES_;

	shared_ptr<GDBusConnection> bus_;
	string appid_;
	string instanceid_;
	vector<Application::URL> input_uris_;
	GPid app_pid_;
	guint connections_open {0};
	GVariantSPtr app_data{nullptr, GVariantDeleter()};
	string dbus_path;
	guint64 unity_starttime {0};
	GSourceSPtr timer{nullptr, GSourceClear()};
	ManagedDBusSignalConnection signal {DBusSignalUnsubscriber()};
};

unordered_map<second_exec_t*, shared_ptr<second_exec_t>> second_exec_t::SECOND_EXEC_INSTANCES_;

GSourceUPtr
second_exec_t::thread_default_timeout (guint interval, GSourceFunc func, gpointer data)
{
	auto src = unique_glib(g_timeout_source_new(interval));
	auto context = unique_glib(g_main_context_get_thread_default());

	g_source_set_callback(src.get(), func, data, nullptr);

	g_source_attach(src.get(), context.get());

	return src;
}

/* Unity didn't respond in time, continue on */
gboolean
second_exec_t::timer_cb (gpointer user_data)
{
	auto data = (second_exec_t *)user_data;
	ual_tracepoint(second_exec_resume_timeout, data->appid_.c_str());
	g_warning("Unity didn't respond in 500ms to resume the app");

	data->finish();
	return G_SOURCE_REMOVE;
}

/* Lower the connection count and process if it gets to zero */
void
second_exec_t::connection_count_dec ()
{
	ual_tracepoint(second_exec_connection_complete, appid_.c_str());
	connections_open--;
	if (connections_open == 0) {
		g_debug("Finished finding connections");
		/* Check time here, either we've already heard from
		   Unity and we should send the data to the app (quit) or
		   we should wait some more */
		guint64 timespent = g_get_monotonic_time() - unity_starttime;
		if (timespent > 500 /* ms */ * 1000 /* ms to us */) {
			finish();
			return;
		} else {
			g_debug("Timer Set");
			timer = thread_default_timeout(500 - (timespent / 1000), timer_cb, this);
		}
	}
}

/* Called when Unity is done unfreezing the application, if we're
   done determining the PID, we can send signals */
void
second_exec_t::unity_resume(GDBusConnection * connection, const gchar * sender, const gchar * path, const gchar * interface, const gchar * signal, GVariant * params)
{
	g_debug("Unity Completed Resume");
	ual_tracepoint(second_exec_resume_complete, appid_.c_str());

	timer.reset();

	if (connections_open == 0) {
		finish();
	} else {
		/* Make it look like we started *forever* ago */
		unity_starttime = 0;
	}
}

/* Turn the input string into something we can send to apps */
void
second_exec_t::parse_uris ()
{
	if (app_data) {
		/* Already done */
		return;
	}

	GVariant * uris = nullptr;

	if (input_uris_.empty()) {
		uris = g_variant_new_array(G_VARIANT_TYPE_STRING, nullptr, 0);
	} else {
		GVariantBuilder builder;
		g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);

		for (const auto& uri: input_uris_) {
			g_variant_builder_add_value(&builder, g_variant_new_string(uri.value().c_str()));
		}

		uris = g_variant_builder_end(&builder);
	}

	GVariantBuilder tuple;
	g_variant_builder_init(&tuple, G_VARIANT_TYPE_TUPLE);
	g_variant_builder_add_value(&tuple, uris);
	g_variant_builder_add_value(&tuple, g_variant_new_array(G_VARIANT_TYPE("{sv}"), nullptr, 0));

	app_data = unique_glib(g_variant_ref_sink(g_variant_builder_end(&tuple)));
}

/* Finds us our dbus path to use.  Basically this is the name
   of the application with dots replaced by / and a / tacted on
   the front.  This is recommended here:

   http://standards.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html#dbus   
*/
void
second_exec_t::app_id_to_dbus_path ()
{
	if (!dbus_path.empty()) {
		return;
	}

	stringstream str;
	auto defaultFlags = str.flags();
	str << "/";

	bool first = true;
	for (const auto c: appid_) {
		if ((c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9' && !first))
		{
			str << c;
		}
		else
		{
			str << "_" << setfill('0') << setw(2) << hex << c;
			str.flags(defaultFlags);
		}
		first = false;
	}

	dbus_path = str.str();
	g_debug("DBus Path: %s", dbus_path.c_str());
}

/* Finish the send and decrement the counter */
void
second_exec_t::send_open (GObject * object, GAsyncResult * res)
{
	GError * error = nullptr;

	ual_tracepoint(second_exec_app_contacted, appid_.c_str());

	g_dbus_connection_call_finish(G_DBUS_CONNECTION(object), res, &error);

	if (error != nullptr) {
		ual_tracepoint(second_exec_app_error, appid_.c_str());
		/* Mostly just to free the error, but printing for debugging */
		g_debug("Unable to send Open: %s", error->message);
		g_clear_error(&error);
	}

	connection_count_dec();
}

/* Sends the Open message to the connection with the URIs we were given */
void
second_exec_t::contact_app (GDBusConnection * bus, const string& dbus_name)
{
	ual_tracepoint(second_exec_contact_app, appid_.c_str(), dbus_name.c_str());

	parse_uris();
	app_id_to_dbus_path();

	/* Using the FD.o Application interface */
	g_dbus_connection_call(bus,
		dbus_name.c_str(),
		dbus_path.c_str(),
		"org.freedesktop.Application",
		"Open",
		app_data.get(),
		NULL,
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		NULL,
		second_exec_t::send_open_cb, this);

	g_debug("Sending Open request to: %s", dbus_name.c_str());
}

second_exec_t::get_pid_t::get_pid_t(const string& _name, shared_ptr<GDBusConnection> bus, shared_ptr<second_exec_t> _data) :
		name(_name), data(_data)
{
	/* Get the PIDs */
	g_dbus_connection_call(bus.get(),
		"org.freedesktop.DBus",
		"/",
		"org.freedesktop.DBus",
		"GetConnectionUnixProcessID",
		g_variant_new("(s)", name.c_str()),
		G_VARIANT_TYPE("(u)"),
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		nullptr,
		get_pid_t::get_pid_cb, this);
}

/* Gets the PID for a connection, and if it matches the one we're looking
   for then it tries to send a message to that connection */
void
second_exec_t::get_pid_t::get_pid (GObject * object, GAsyncResult * res)
{
	GError * error = nullptr;

	ual_tracepoint(second_exec_got_pid, data->appid_.c_str(), name.c_str());

	auto vpid = unique_glib(g_dbus_connection_call_finish(G_DBUS_CONNECTION(object), res, &error));

	if (error != nullptr) {
		g_warning("Unable to query PID for dbus name '%s': %s", name.c_str(), error->message);
		g_clear_error(&error);

		/* Lowering the connection count, this one is terminal, even if in error */
		data->connection_count_dec();
	} else {
		GPid pid = 0;
		g_variant_get(vpid.get(), "(u)", &pid);

		if (pid == data->app_pid_) {
			/* Trying to send a message to the connection */
			data->contact_app(G_DBUS_CONNECTION(object), name);
		} else {
			/* See if we can quit now */
			data->connection_count_dec();
		}
	}

	delete this;
}

/* Starts to look for the PID and the connections for that PID */
void
second_exec_t::find_appid_pid ()
{
	GError * error = nullptr;

	/* List all the connections on dbus.  This sucks that we have to do
	   this, but in the future we should add DBus API to do this lookup
	   instead of having to do it with a bunch of requests */
	auto listnames = unique_glib(g_dbus_connection_call_sync(bus_.get(),
		"org.freedesktop.DBus",
		"/",
		"org.freedesktop.DBus",
		"ListNames",
		nullptr,
		G_VARIANT_TYPE("(as)"),
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		nullptr,
		&error));

	if (error != nullptr) {
		g_warning("Unable to get list of names from DBus: %s", error->message);
		g_clear_error(&error);
		return;
	}

	g_debug("Got bus names");
	ual_tracepoint(second_exec_got_dbus_names, appid_.c_str());

	g_debug("Primary PID: %d", app_pid_);
	ual_tracepoint(second_exec_got_primary_pid, appid_.c_str());

	/* Get the names */
	auto names = unique_glib(g_variant_get_child_value(listnames.get(), 0));
	GVariantIter iter;
	g_variant_iter_init(&iter, names.get());
	gchar * name = nullptr;

	while (g_variant_iter_loop(&iter, "s", &name)) {
		/* We only want to ask each connection once, this makes that so */
		if (!g_dbus_is_unique_name(name)) {
			continue;
		}

		ual_tracepoint(second_exec_request_pid, appid_.c_str(), name);

		// FIXME This object deletes itself
		new get_pid_t(string(name), bus_, shared_from_this());

		connections_open++;
	}
}

second_exec_t::second_exec_t(shared_ptr<GDBusConnection> bus, const string& appid, const string& instanceid, const vector<Application::URL>& input_uris, GPid app_pid) :
		bus_(bus), appid_(appid), instanceid_(instanceid), input_uris_(input_uris), app_pid_(app_pid)
{
	// can't use for init, due to shared_from_this restriction
}

void
second_exec_t::init()
{
	ual_tracepoint(second_exec_start, appid_.c_str());

	/* Set up listening for the unfrozen signal from Unity */
	signal = managedDBusSignalConnection(g_dbus_connection_signal_subscribe(bus_.get(),
		nullptr, /* sender */
		"com.canonical.UbuntuAppLaunch", /* interface */
		"UnityResumeResponse", /* signal */
		"/", /* path */
		appid_.c_str(), /* arg0 */
		G_DBUS_SIGNAL_FLAGS_NONE,
		second_exec_t::unity_resume_cb, this,
		nullptr), /* user data destroy */
			bus_);

	g_debug("Sending resume request");
	ual_tracepoint(second_exec_emit_resume, appid_.c_str());

	GError * error = nullptr;

	/* Send unfreeze to to Unity */
	g_dbus_connection_emit_signal(bus_.get(),
		nullptr, /* destination */
		"/", /* path */
		"com.canonical.UbuntuAppLaunch", /* interface */
		"UnityResumeRequest", /* signal */
		g_variant_new("(ss)", appid_.c_str(), instanceid_.c_str()),
		&error);

	/* Now we start a race, we try to get to the point of knowing who
	   to send things to, and Unity is unfrezing it.  When both are
	   done we can send something to the app */
	unity_starttime = g_get_monotonic_time();

	if (error != nullptr) {
		/* On error let's not wait for Unity */
		g_warning("Unable to signal Unity: %s", error->message);
		g_clear_error(&error);
		unity_starttime = 0;
	}

	/* If we've got something to give out, start looking for how */
	if (!input_uris_.empty()) {
		find_appid_pid();
	} else {
		g_debug("No URIs to send");
	}

	/* Loop and wait for everything to align */
	if (connections_open == 0) {
		if (unity_starttime != 0) {
			timer = thread_default_timeout(500, timer_cb, this);
			SECOND_EXEC_INSTANCES_.insert(make_pair(this, shared_from_this()));
		}
	}
}

void second_exec_t::finish()
{
	SECOND_EXEC_INSTANCES_.erase(this);
}

second_exec_t::~second_exec_t()
{
	GError * error = nullptr;
	ual_tracepoint(second_exec_emit_focus, appid_.c_str());

	/* Now that we're done sending the info to the app, we can ask
	   Unity to focus the application. */
	g_dbus_connection_emit_signal(bus_.get(),
		nullptr, /* destination */
		"/", /* path */
		"com.canonical.UbuntuAppLaunch", /* interface */
		"UnityFocusRequest", /* signal */
		g_variant_new("(ss)", appid_.c_str(), instanceid_.c_str()),
		&error);

	if (error != nullptr)
	{
		g_warning("Unable to request focus to Unity: %s", error->message);
		g_clear_error(&error);
	}

	/* Make sure the signal hits the bus */
	g_dbus_connection_flush_sync(bus_.get(), nullptr, &error);
	if (error != NULL) {
		g_warning("Unable to flush session bus: %s", error->message);
		g_clear_error(&error);
	}

	ual_tracepoint(second_exec_finish, appid_.c_str());
	g_debug("Second Exec complete");
}

}

void
second_exec (shared_ptr<GDBusConnection> session, shared_ptr<GCancellable> cancel, GPid pid, const string& app_id, const string& instance_id, const vector<Application::URL>& appuris)
{
	auto t = make_shared<second_exec_t>(
		session,
		app_id,
		instance_id,
		appuris,
		pid
	);

	t->init();
}

}
}
