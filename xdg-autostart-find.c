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

/* Look at a directory, ensure it exists and then start looking for
   new autostart keys we can launch */
static void
check_dir (gpointer pdir, gpointer pkeyhash)
{


}

/* Enumerates all the different autostart directories we can have */
static GList *
find_autostart_dirs (void)
{
	GList * retval = NULL;

	retval = g_list_prepend(retval, g_build_filename(g_get_user_config_dir(), "autostart", NULL));

	const char * const * system_conf_dir = g_get_system_config_dirs();
	int i;

	for (i = 0; system_conf_dir[i] != NULL; i++) {
		retval = g_list_prepend(retval, g_build_filename(system_conf_dir[i], "autostart", NULL));
	}

	return retval;
}

int
main (int argc, gchar * argv[])
{
	/* Allocate resources */
	GList * autostart_dirs = NULL;
	GHashTable * autostart_keys = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	GDBusConnection * bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);

	/* Make sure we got a bus */
	g_return_val_if_fail(bus != NULL, -1);

	/* Find all the directories we need */
	autostart_dirs = find_autostart_dirs();

	/* For each one look at the entries */
	g_list_foreach(autostart_dirs, check_dir, autostart_keys);

	/* Clean up */
	g_hash_table_destroy(autostart_keys);
	g_list_free_full(autostart_dirs, g_free);

	g_dbus_connection_flush_sync(bus, NULL, NULL);
	g_object_unref(bus);

	return 0;
}
