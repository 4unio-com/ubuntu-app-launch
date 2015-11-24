/*
 * Copyright 2013 Canonical Ltd.
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
 */

#include <gtest/gtest.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

extern "C" {
#include "../helpers.h"
}

class HelperHandshakeTest : public ::testing::Test
{
	private:
		GTestDBus * testbus = NULL;

	protected:
		GMainLoop * mainloop = NULL;

		virtual void SetUp() {
			mainloop = g_main_loop_new(NULL, FALSE);
			testbus = g_test_dbus_new(G_TEST_DBUS_NONE);
			g_test_dbus_up(testbus);
		}

		virtual void TearDown() {
			g_test_dbus_down(testbus);
			g_clear_object(&testbus);
			g_main_loop_unref(mainloop);
			mainloop = NULL;
			return;
		}

	public:
		GDBusMessage * FilterFunc (GDBusConnection * conn, GDBusMessage * message, gboolean incomming) {
			if (g_strcmp0(g_dbus_message_get_member(message), "UnityStartingBroadcast") == 0) {
				GVariant * body = g_dbus_message_get_body(message);
				GVariant * correct_body = g_variant_new("(s)", "fooapp");
				g_variant_ref_sink(correct_body);

				[body, correct_body]() {
					ASSERT_TRUE(g_variant_equal(body, correct_body));
				}();

				g_variant_unref(correct_body);
				g_main_loop_quit(mainloop);
			}

			return message;
		}

};

static GDBusMessage *
filter_func (GDBusConnection * conn, GDBusMessage * message, gboolean incomming, gpointer user_data) {
	return static_cast<HelperHandshakeTest *>(user_data)->FilterFunc(conn, message, incomming);
}

TEST_F(HelperHandshakeTest, BaseHandshake)
{
	GDBusConnection * con = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	guint filter = g_dbus_connection_add_filter(con, filter_func, this, NULL);

	handshake_t * handshake = starting_handshake_start("fooapp");

	g_main_loop_run(mainloop);

	g_dbus_connection_remove_filter(con, filter);

	g_dbus_connection_emit_signal(con,
		g_dbus_connection_get_unique_name(con), /* destination */
		"/", /* path */
		"com.canonical.UbuntuAppLaunch", /* interface */
		"UnityStartingSignal", /* signal */
		g_variant_new("(s)", "fooapp"), /* params, the same */
		NULL);

	g_dbus_connection_emit_signal(con,
		g_dbus_connection_get_unique_name(con), /* destination */
		"/", /* path */
		"com.canonical.UbuntuAppLaunch", /* interface */
		"UnityStartingApproved", /* signal */
		g_variant_new("(sb)", "fooapp", TRUE), /* params */
		NULL);

	ASSERT_TRUE(starting_handshake_wait(handshake));

	g_object_unref(con);

	return;
}

TEST_F(HelperHandshakeTest, UnapprovedHandshake)
{
	GDBusConnection * con = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	guint filter = g_dbus_connection_add_filter(con, filter_func, this, NULL);

	handshake_t * handshake = starting_handshake_start("fooapp");

	g_main_loop_run(mainloop);

	g_dbus_connection_remove_filter(con, filter);

	g_dbus_connection_emit_signal(con,
		g_dbus_connection_get_unique_name(con), /* destination */
		"/", /* path */
		"com.canonical.UbuntuAppLaunch", /* interface */
		"UnityStartingSignal", /* signal */
		g_variant_new("(s)", "fooapp"), /* params */
		NULL);

	g_dbus_connection_emit_signal(con,
		g_dbus_connection_get_unique_name(con), /* destination */
		"/", /* path */
		"com.canonical.UbuntuAppLaunch", /* interface */
		"UnityStartingApproved", /* signal */
		g_variant_new("(sb)", "fooapp", FALSE), /* params */
		NULL);

	ASSERT_FALSE(starting_handshake_wait(handshake));

	g_object_unref(con);

	return;
}

TEST_F(HelperHandshakeTest, InvalidHandshake)
{
	GDBusConnection * con = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	guint filter = g_dbus_connection_add_filter(con, filter_func, this, NULL);

	handshake_t * handshake = starting_handshake_start("fooapp");

	g_main_loop_run(mainloop);

	g_dbus_connection_remove_filter(con, filter);

	g_dbus_connection_emit_signal(con,
		g_dbus_connection_get_unique_name(con), /* destination */
		"/", /* path */
		"com.canonical.UbuntuAppLaunch", /* interface */
		"UnityStartingSignal", /* signal */
		g_variant_new("(s)", "fooapp"), /* params */
		NULL);

	g_dbus_connection_emit_signal(con,
		g_dbus_connection_get_unique_name(con), /* destination */
		"/", /* path */
		"com.canonical.UbuntuAppLaunch", /* interface */
		"UnityStartingApproved", /* signal */
		g_variant_new("(ss)", "fooapp", "true"), /* bad params */
		NULL);

	ASSERT_FALSE(starting_handshake_wait(handshake));

	g_object_unref(con);

	return;
}

static gboolean
two_second_reached (gpointer user_data)
{
	bool * reached = static_cast<bool *>(user_data);
	*reached = true;
	return G_SOURCE_REMOVE;
}

TEST_F(HelperHandshakeTest, HandshakeTimeout)
{
	bool timeout_reached = false;
	handshake_t * handshake = starting_handshake_start("fooapp");

	guint outertimeout = g_timeout_add_seconds(2, two_second_reached, &timeout_reached);

	ASSERT_FALSE(starting_handshake_wait(handshake));

	ASSERT_FALSE(timeout_reached);
	g_source_remove(outertimeout);

	return;
}

TEST_F(HelperHandshakeTest, HandshakeTimeoutWithOnlyApprovedSignal)
{
	bool timeout_reached = false;
	GDBusConnection * con = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);

	guint outertimeout = g_timeout_add_seconds(2, two_second_reached, &timeout_reached);

	guint filter = g_dbus_connection_add_filter(con, filter_func, this, NULL);
	handshake_t * handshake = starting_handshake_start("fooapp");
	g_main_loop_run(mainloop);
	g_dbus_connection_remove_filter(con, filter);

	g_dbus_connection_emit_signal(con,
		g_dbus_connection_get_unique_name(con), /* destination */
		"/", /* path */
		"com.canonical.UbuntuAppLaunch", /* interface */
		"UnityStartingApproved", /* signal */
		g_variant_new("(sb)", "fooapp", true), /* bad params */
		NULL);

	ASSERT_FALSE(starting_handshake_wait(handshake));

	ASSERT_FALSE(timeout_reached);
	g_source_remove(outertimeout);

	g_object_unref(con);

	return;
}

static gboolean
emit_approval (gpointer user_data)
{
	GDBusConnection * con = (GDBusConnection *) user_data;

	g_dbus_connection_emit_signal(con,
		g_dbus_connection_get_unique_name(con), /* destination */
		"/", /* path */
		"com.canonical.UbuntuAppLaunch", /* interface */
		"UnityStartingApproved", /* signal */
		g_variant_new("(sb)", "fooapp", true), /* params */
		NULL);

	return G_SOURCE_REMOVE;
}

TEST_F(HelperHandshakeTest, HandshakeNoTimeoutWithReceivedSignal)
{
	bool timeout_reached = false;
	GDBusConnection * con = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);

	g_timeout_add_seconds(2, two_second_reached, &timeout_reached);
	g_timeout_add_seconds(2, emit_approval, con);

	guint filter = g_dbus_connection_add_filter(con, filter_func, this, NULL);
	handshake_t * handshake = starting_handshake_start("fooapp");
	g_main_loop_run(mainloop);
	g_dbus_connection_remove_filter(con, filter);

	g_dbus_connection_emit_signal(con,
		g_dbus_connection_get_unique_name(con), /* destination */
		"/", /* path */
		"com.canonical.UbuntuAppLaunch", /* interface */
		"UnityStartingSignal", /* signal */
		g_variant_new("(s)", "fooapp"), /* params */
		NULL);

	ASSERT_TRUE(starting_handshake_wait(handshake));

	ASSERT_TRUE(timeout_reached);

	g_object_unref(con);

	return;
}

static gboolean
emit_name_change (gpointer user_data)
{
	GDBusConnection * con = (GDBusConnection *) user_data;
	const gchar *unique_name = g_dbus_connection_get_unique_name(con);

	// We are allowed to emit this (instead of DBus) because we link against
	// helpers-test, a special version of the helper code that listens
	// for this signal from anyone.
	g_dbus_connection_emit_signal(con,
		unique_name, /* destination */
		"/org/freedesktop/DBus", /* path */
		"org.freedesktop.DBus", /* interface */
		"NameOwnerChanged", /* signal */
		g_variant_new("(sss)", unique_name, unique_name, ""), /* params */
		NULL);

	return G_SOURCE_REMOVE;
}

TEST_F(HelperHandshakeTest, StopsWaitingIfUnityExits)
{
	bool timeout_reached = false;
	GDBusConnection * con = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);

	g_timeout_add_seconds(1, emit_name_change, con);
	guint outertimeout = g_timeout_add_seconds(2, two_second_reached, &timeout_reached);

	guint filter = g_dbus_connection_add_filter(con, filter_func, this, NULL);
	handshake_t * handshake = starting_handshake_start("fooapp");
	g_main_loop_run(mainloop);
	g_dbus_connection_remove_filter(con, filter);

	g_dbus_connection_emit_signal(con,
		g_dbus_connection_get_unique_name(con), /* destination */
		"/", /* path */
		"com.canonical.UbuntuAppLaunch", /* interface */
		"UnityStartingSignal", /* signal */
		g_variant_new("(s)", "fooapp"), /* params */
		NULL);

	ASSERT_FALSE(starting_handshake_wait(handshake));

	ASSERT_FALSE(timeout_reached);
	g_source_remove(outertimeout);

	g_object_unref(con);

	return;
}

