/*
 * Copyright © 2016 Canonical Ltd.
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

#include "libubuntu-app-launch/application.h"
#include "libubuntu-app-launch/jobs-base.h"
#include "libubuntu-app-launch/registry.h"
#include <csignal>
#include <future>
#include <glib.h>
#include <iostream>

/* Globals */
/* Needed for sigterm handler */
ubuntu::app_launch::AppID global_appid;
std::shared_ptr<ubuntu::app_launch::Application::Instance> global_inst;
std::promise<int> retval;

/* Show the log file for the instance */
class journald
{
    pid_t child{0};

public:
    journald(const std::shared_ptr<ubuntu::app_launch::Application::Instance>& inst)
    {
        auto instinternal = std::dynamic_pointer_cast<ubuntu::app_launch::jobs::instance::Base>(inst);
        auto instname = std::string{"ubuntu-app-launch--"} + instinternal->getJobName() + "--" +
                        std::string{instinternal->getAppId()} + "--" + instinternal->getInstanceId() + ".service";

        if ((child = fork()) == 0)
        {
            std::array<const char*, 6> execline = {"journalctl", "--no-tail", "--follow", "--user-unit", instname.c_str(), nullptr};

            execvp(execline[0], (char* const*)execline.data());
        }
    }

    ~journald()
    {
        if (child)
            kill(child, SIGTERM);
    }
};

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <app id> [uris]" << std::endl;
        return 1;
    }

    global_appid = ubuntu::app_launch::AppID::find(argv[1]);

    std::vector<ubuntu::app_launch::Application::URL> urls;
    for (int i = 2; i < argc; i++)
    {
        urls.push_back(ubuntu::app_launch::Application::URL::from_raw(argv[i]));
    }

    ubuntu::app_launch::Registry::appStarted().connect(
        [](std::shared_ptr<ubuntu::app_launch::Application> app,
           std::shared_ptr<ubuntu::app_launch::Application::Instance> instance) {
            if (app->appId() != global_appid)
            {
                return;
            }

            g_debug("Started: %s", std::string{app->appId()}.c_str());
        });

    ubuntu::app_launch::Registry::appStopped().connect(
        [](std::shared_ptr<ubuntu::app_launch::Application> app,
           std::shared_ptr<ubuntu::app_launch::Application::Instance> instance) {
            if (app->appId() != global_appid)
            {
                return;
            }

            retval.set_value(EXIT_SUCCESS);
        });

    ubuntu::app_launch::Registry::appFailed().connect(
        [](std::shared_ptr<ubuntu::app_launch::Application> app,
           std::shared_ptr<ubuntu::app_launch::Application::Instance> instance,
           ubuntu::app_launch::Registry::FailureType type) {
            if (app->appId() != global_appid)
            {
                return;
            }

            std::cout << "Failed:  " << std::string{app->appId()} << std::endl;
            retval.set_value(EXIT_FAILURE);
        });

    auto app = ubuntu::app_launch::Application::create(global_appid, ubuntu::app_launch::Registry::getDefault());
    global_inst = app->launch(urls);

    journald logging{global_inst};

    std::signal(SIGTERM, [](int signal) -> void {
        global_inst->stop();
    });
    std::signal(SIGQUIT, [](int signal) -> void {
        global_inst->stop();
    });

    return retval.get_future().get();
}
