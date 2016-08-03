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

#include "application-impl-libertine.h"
#include "application-info-desktop.h"
extern "C"
{
#include "app-info.h"
}
#include <libertine.h>

namespace ubuntu
{
namespace app_launch
{
namespace app_impls
{

namespace
{
std::shared_ptr<GKeyFile> keyfileFromPath(const gchar* pathname)
{
    if (!g_file_test(pathname, G_FILE_TEST_EXISTS))
    {
        return {};
    }

    std::shared_ptr<GKeyFile> keyfile(g_key_file_new(), [](GKeyFile* keyfile) {
        if (keyfile != nullptr)
        {
            g_key_file_free(keyfile);
        }
    });
    GError* error = nullptr;

    g_key_file_load_from_file(keyfile.get(), pathname, G_KEY_FILE_NONE, &error);

    if (error != nullptr)
    {
        g_error_free(error);
        return {};
    }

    return keyfile;
}
}

Libertine::Libertine(const AppID::Package& container,
                     const AppID::AppName& appname,
                     const std::shared_ptr<Registry>& registry)
    : Base(registry)
    , _container(container)
    , _appname(appname)
{
    gchar* basedir = NULL;
    gchar* keyfile = NULL;
    if (!app_info_libertine(ubuntu_app_launch_triplet_to_app_id(container.value().c_str(), appname.value().c_str(), "0.0"), &basedir, &keyfile))
    {
      throw std::runtime_error{"Unable to find a keyfile for application '" + appname.value() + "' in container '" +
                               container.value() + "'"};
    }
    _basedir = basedir;
    gchar* full_filename = g_build_filename(basedir, keyfile, nullptr);
    _keyfile = keyfileFromPath(full_filename);

    g_free(full_filename);
    g_free(keyfile);
    g_free(basedir);

    if (!_keyfile)
    {
        throw std::runtime_error{"Unable to find a keyfile for application '" + appname.value() + "' in container '" +
                                 container.value() + "'"};
    }
}

std::list<std::shared_ptr<Application>> Libertine::list(const std::shared_ptr<Registry>& registry)
{
    std::list<std::shared_ptr<Application>> applist;

    auto containers = libertine_list_containers();

    for (int i = 0; containers[i] != nullptr; i++)
    {
        auto container = containers[i];
        auto apps = libertine_list_apps_for_container(container);

        for (int i = 0; apps[i] != nullptr; i++)
        {
            auto sapp = std::make_shared<Libertine>(AppID::Package::from_raw(container),
                                                    AppID::AppName::from_raw(apps[i]), registry);
            applist.push_back(sapp);
        }

        g_strfreev(apps);
    }
    g_strfreev(containers);

    return applist;
}

std::shared_ptr<Application::Info> Libertine::info()
{
    return std::make_shared<app_info::Desktop>(_keyfile, _basedir, _registry);
}

};  // namespace app_impls
};  // namespace app_launch
};  // namespace ubuntu
