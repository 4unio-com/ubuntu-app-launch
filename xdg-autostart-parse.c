/*
 * Copyright © 2014 Canonical Ltd.
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

#include <gio/gio.h>
#include "helpers.h"

/* Look in a directory for a desktop file of task_name and if
   it's there return it. */
static GKeyFile *
check_dir (const gchar * directory, const gchar * task_name)
{

	return NULL;
}

/* Check to see if an autostart keyfile should be run in this context */
static gboolean
valid_keyfile (GKeyFile * keyfile)
{

	return FALSE;
}

/* See if we can find a keyfile for this task, and if it's valid, then
   return it for further processing. */
static GKeyFile *
find_keyfile (const gchar * task_name)
{
	GKeyFile * keyfile = NULL;

	keyfile = check_dir(g_get_user_config_dir(), task_name);

	if (keyfile == NULL) {
		const char * const * system_conf_dirs = g_get_system_config_dirs();
		int i;

		for (i = 0; keyfile == NULL && system_conf_dirs[i] != NULL; i++) {
			keyfile = check_dir(system_conf_dirs[i], task_name);
		}
	}

	if (keyfile == NULL) {
		g_debug("Couldn't find desktop file for '%s'", task_name);
		return NULL;
	}

	if (valid_keyfile(keyfile)) {
		return keyfile;
	}

	g_key_file_free(keyfile);
	return NULL;
}

int
main (int argc, gchar * argv[])
{
	const gchar * task_name = g_getenv("XDG_AUTOSTART_NAME");
	g_return_val_if_fail(task_name != NULL, -1);

	GDBusConnection * bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	g_return_val_if_fail(bus != NULL, -1);

	GKeyFile * keyfile = find_keyfile(task_name);

	if (keyfile == NULL) {
		return -1;
	}

	gchar * exec = g_key_file_get_string(keyfile, "Desktop Entry", "Exec", NULL);
	g_return_val_if_fail(exec != NULL, -1);

	set_upstart_variable("APP_EXEC", exec);
	g_free(exec);

	g_dbus_connection_flush_sync(bus, NULL, NULL);
	g_object_unref(bus);

	return 0;
}
