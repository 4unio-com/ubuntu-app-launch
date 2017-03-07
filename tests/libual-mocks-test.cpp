/*
 * Copyright © 2017 Canonical Ltd.
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
 */

#include "mock/application.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

namespace ualmock = ubuntu::app_launch::mock;

/*
namespace {

class LibUALMockTest : public ::testing:Test
{
};

}
*/

TEST(LibUALMock, ApplicationMock)
{
    auto app = ualmock::Application::create(ubuntu::app_launch::AppID::parse("the-app"));
    ASSERT_NE(nullptr, app);
}
