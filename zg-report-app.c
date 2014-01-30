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


#include <zeitgeist.h>

static gboolean
watchdog_timeout (gpointer user_data)
{
	g_error("Watchdog triggered, took too long to submit into Zeitgeist Database!");
	g_main_loop_quit((GMainLoop *)user_data);

	return G_SOURCE_REMOVE;
}

static void
insert_complete (GObject * obj, GAsyncResult * res, gpointer user_data)
{
	GError * error = NULL;
	GArray * result = NULL;

	result = zeitgeist_log_insert_event_finish(ZEITGEIST_LOG(obj), res, &error);

	if (error != NULL) {
		g_error("Unable to submit Zeitgeist Event: %s", error->message);
		g_error_free(error);
	}

	g_array_free(result, TRUE);
	g_main_loop_quit((GMainLoop *)user_data);
	return;
}

int
main (int argc, char * argv[])
{
	if (argc != 2 || (g_strcmp0(argv[1], "open") != 0 && g_strcmp0(argv[1], "close") != 0)) {
		g_printerr("Usage: %s [open|close]\n", argv[0]);
		return 1;
	}

	const gchar * appid = g_getenv("APP_SHORT_ID");
	if (appid == NULL) {
		appid = g_getenv("APP_ID");
	}

	if (appid == NULL) {
		g_printerr("No App ID defined");
		return 1;
	}
	gchar * uri = g_strdup_printf("application://%s.desktop", appid);

	ZeitgeistLog * log = zeitgeist_log_get_default();

	ZeitgeistEvent * event = zeitgeist_event_new();
	zeitgeist_event_set_actor(event, "application://upstart-app-launch.desktop");
	if (g_strcmp0(argv[1], "open") == 0) {
		zeitgeist_event_set_interpretation(event, ZEITGEIST_ZG_ACCESS_EVENT);
	} else {
		zeitgeist_event_set_interpretation(event, ZEITGEIST_ZG_LEAVE_EVENT);
	}
	zeitgeist_event_set_manifestation(event, ZEITGEIST_ZG_USER_ACTIVITY);

	ZeitgeistSubject * subject = zeitgeist_subject_new();
	zeitgeist_subject_set_interpretation(subject, ZEITGEIST_NFO_SOFTWARE);
	zeitgeist_subject_set_manifestation(subject, ZEITGEIST_NFO_SOFTWARE_ITEM);
	zeitgeist_subject_set_mimetype(subject, "application/x-desktop");
	zeitgeist_subject_set_uri(subject, uri);

	zeitgeist_event_add_subject(event, subject);

	GMainLoop * main_loop = g_main_loop_new(NULL, FALSE);

	zeitgeist_log_insert_event(log, event, NULL, insert_complete, main_loop);
	g_timeout_add_seconds(4, watchdog_timeout, main_loop);

	g_main_loop_run(main_loop);

	g_main_loop_unref(main_loop);
	g_free(uri);

	return 0;
}
