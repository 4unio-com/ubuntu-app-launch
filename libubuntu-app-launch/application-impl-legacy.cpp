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

#include "application-impl-legacy.h"
#include "application-info-desktop.h"
extern "C"
{
#include "app-info.h"
}

namespace ubuntu
{
namespace app_launch
{
namespace app_impls
{

Legacy::Legacy(const AppID::AppName& appname, const std::shared_ptr<Registry>& registry)
    : Base(registry)
    , _appname(appname)
{
    gchar* basedir = NULL;
    gchar* keyfile = NULL;
    if (!app_info_legacy(appname.value().c_str(), &basedir, &keyfile))
    {
        throw std::runtime_error{"Unable to find keyfile for legacy application: " + appname.value()};
    }

    _basedir = basedir;
    gchar* full_filename = g_build_filename(basedir, keyfile, nullptr);
    _keyfile = keyfileFromPath(full_filename);

    g_free(full_filename);
    g_free(keyfile);
    g_free(basedir);

    if (!_keyfile)
    {
        throw std::runtime_error{"Unable to find keyfile for legacy application: " + appname.value()};
    }
}

std::shared_ptr<Application::Info> Legacy::info()
{
    return std::make_shared<app_info::Desktop>(_keyfile, _basedir, _registry, true);
}

std::list<std::shared_ptr<Application>> Legacy::list(const std::shared_ptr<Registry>& registry)
{
    std::list<std::shared_ptr<Application>> list;
    GList* head = g_app_info_get_all();
    for (GList* item = head; item != nullptr; item = g_list_next(item))
    {
        GDesktopAppInfo* appinfo = G_DESKTOP_APP_INFO(item->data);

        if (appinfo == nullptr)
        {
            continue;
        }

        if (g_app_info_should_show(G_APP_INFO(appinfo)) == FALSE)
        {
            continue;
        }

        auto app = std::make_shared<Legacy>(AppID::AppName::from_raw(g_app_info_get_id(G_APP_INFO(appinfo))), registry);
        list.push_back(app);
    }

    g_list_free_full(head, g_object_unref);

    return list;
}

};  // namespace app_impls
};  // namespace app_launch
};  // namespace ubuntu
