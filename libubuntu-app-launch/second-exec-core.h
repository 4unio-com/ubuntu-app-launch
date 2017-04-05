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


#pragma once

#include <gio/gio.h>
#include <memory>
#include <string>
#include <vector>

#include "application.h"

namespace ubuntu
{
namespace app_launch
{

void second_exec (std::shared_ptr<GDBusConnection> con, std::shared_ptr<GCancellable> cancel, GPid pid, const std::string& app_id, const std::string& instance_id, const std::vector<Application::URL>& appuris);

}
}
