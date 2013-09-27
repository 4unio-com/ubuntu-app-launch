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
#include <gio/gio.h>

extern "C" {
#include "../second-exec-core.h"
#include "upstart-app-launch.h"
#include "upstart-app-launch-mock.h"
}

class SecondExecTest : public ::testing::Test
{
	private:
		GTestDBus * testbus = NULL;

	protected:
		gchar * last_focus_appid = NULL;
		gchar * last_resume_appid = NULL;

	private:
		static void focus_cb (const gchar * appid, gpointer user_data) {
			SecondExecTest * _this = static_cast<SecondExecTest *>(user_data);
			g_free(_this->last_focus_appid);
			_this->last_focus_appid = g_strdup(appid);
		}

		static void resume_cb (const gchar * appid, gpointer user_data) {
			SecondExecTest * _this = static_cast<SecondExecTest *>(user_data);
			g_free(_this->last_resume_appid);
			_this->last_resume_appid = g_strdup(appid);
		}

	protected:
		virtual void SetUp() {
			testbus = g_test_dbus_new(G_TEST_DBUS_NONE);
			g_test_dbus_up(testbus);

			upstart_app_launch_observer_add_app_focus(focus_cb, this);
			upstart_app_launch_observer_add_app_resume(resume_cb, this);

			return;
		}
		virtual void TearDown() {
			upstart_app_launch_observer_delete_app_focus(focus_cb, this);
			upstart_app_launch_observer_delete_app_resume(resume_cb, this);

			g_test_dbus_down(testbus);
			g_object_unref(testbus);

			return;
		}

		static gboolean pause_helper (gpointer pmainloop) {
			g_main_loop_quit((GMainLoop *)pmainloop);
			return G_SOURCE_REMOVE;
		}

		void pause (guint time) {
			if (time > 0) {
				GMainLoop * mainloop = g_main_loop_new(NULL, FALSE);
				guint timer = g_timeout_add(time, pause_helper, mainloop);

				g_main_loop_run(mainloop);

				g_source_remove(timer);
				g_main_loop_unref(mainloop);
			}

			while (g_main_pending()) {
				g_main_iteration(TRUE);
			}

			return;
		}
};

TEST_F(SecondExecTest, AppIdTest)
{
	second_exec("foo", NULL);
	pause(0); /* Ensure all the events come through */
	ASSERT_STREQ(this->last_focus_appid, "foo");
	ASSERT_STREQ(this->last_resume_appid, "foo");
}
