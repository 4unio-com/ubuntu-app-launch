/*
 * Copyright © 2016-2017 Canonical Ltd.
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

#include <ubuntu-app-launch/application.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace ::testing;

namespace ubuntu
{
namespace app_launch
{
namespace mock
{

    class Application : public ubuntu::app_launch::Application {
    public:
        class Info;
        class Instance;

        Application(const ubuntu::app_launch::AppID& app_id,
                    const std::shared_ptr<Info>& info = {})
            : m_app_id(app_id),
              m_info(info)
        {
        }

        ubuntu::app_launch::AppID appId()
        {
            return m_app_id;
        }

        std::shared_ptr<ubuntu::app_launch::Application::Info> info()
        {
            if (!m_info) {
                throw std::runtime_error("Invalid application.");
            }
            return m_info;
        }

        static std::shared_ptr<Application> create(ubuntu::app_launch::AppID app_id,
                                                   std::shared_ptr<Info> info = {})
        {
            std::shared_ptr<Application> result{new Application(app_id, info)};
            return result;
        }

        class Info : public ubuntu::app_launch::Application::Info
        {
        public:
            Info(const std::string& title,
                 const std::string& description,
                 const std::string& iconPath)
                : m_name(ubuntu::app_launch::Application::Info::Name::from_raw(title)),
                m_description(ubuntu::app_launch::Application::Info::Description::from_raw(description)),
                m_iconPath(ubuntu::app_launch::Application::Info::IconPath::from_raw(iconPath)),
                m_defaultDept(ubuntu::app_launch::Application::Info::DefaultDepartment::from_raw("")),
                m_ssPath(ubuntu::app_launch::Application::Info::IconPath::from_raw("")),
                m_keywords(ubuntu::app_launch::Application::Info::Keywords::from_raw(std::vector<std::string>{}))
            {
                DefaultValue<const ubuntu::app_launch::Application::Info::DefaultDepartment&>::Set(m_defaultDept);
                DefaultValue<const ubuntu::app_launch::Application::Info::IconPath&>::Set(m_ssPath);
                DefaultValue<const ubuntu::app_launch::Application::Info::Keywords&>::Set(m_keywords);
                DefaultValue<ubuntu::app_launch::Application::Info::UbuntuLifecycle>::Set(ubuntu::app_launch::Application::Info::UbuntuLifecycle::from_raw(false));
            }

            const ubuntu::app_launch::Application::Info::Name& name()
            {
                return m_name;
            }

            const ubuntu::app_launch::Application::Info::Description& description()
            {
                return m_description;
            }

            const ubuntu::app_launch::Application::Info::IconPath& iconPath()
            {
                return m_iconPath;
            }

            MOCK_METHOD0(defaultDepartment, const ubuntu::app_launch::Application::Info::DefaultDepartment&());
            MOCK_METHOD0(screenshotPath, const ubuntu::app_launch::Application::Info::IconPath&());
            MOCK_METHOD0(keywords, const ubuntu::app_launch::Application::Info::Keywords&());
            MOCK_METHOD0(popularity, const ubuntu::app_launch::Application::Info::Popularity&());

            MOCK_METHOD0(splash, ubuntu::app_launch::Application::Info::Splash());
            MOCK_METHOD0(supportedOrientations, ubuntu::app_launch::Application::Info::Orientations());
            MOCK_METHOD0(rotatesWindowContents, ubuntu::app_launch::Application::Info::RotatesWindow());
            MOCK_METHOD0(supportsUbuntuLifecycle, ubuntu::app_launch::Application::Info::UbuntuLifecycle());

        private:
            ubuntu::app_launch::Application::Info::Name m_name;
            ubuntu::app_launch::Application::Info::Description m_description;
            ubuntu::app_launch::Application::Info::IconPath m_iconPath;
            ubuntu::app_launch::Application::Info::DefaultDepartment m_defaultDept;
            ubuntu::app_launch::Application::Info::IconPath m_ssPath;
            ubuntu::app_launch::Application::Info::Keywords m_keywords;
        };

        class Instance : public ubuntu::app_launch::Application::Instance
        {
        public:
            MOCK_METHOD0(isRunning, bool());
            MOCK_METHOD0(logPath, std::string());
            MOCK_METHOD0(primaryPid, pid_t());
            MOCK_METHOD1(hasPid, bool(pid_t));
            MOCK_METHOD0(pids, std::vector<pid_t>());
            MOCK_METHOD0(pause, void());
            MOCK_METHOD0(resume, void());
            MOCK_METHOD0(stop, void());
            MOCK_METHOD0(focus, void());
        };

        MOCK_METHOD0(hasInstances, bool());
        MOCK_METHOD0(instances, std::vector<std::shared_ptr<ubuntu::app_launch::Application::Instance>>());
        MOCK_METHOD1(launch, std::shared_ptr<ubuntu::app_launch::Application::Instance>(const std::vector<ubuntu::app_launch::Application::URL>&));
        MOCK_METHOD1(launchTest, std::shared_ptr<ubuntu::app_launch::Application::Instance>(const std::vector<ubuntu::app_launch::Application::URL>&));
        MOCK_METHOD1(findInstance, std::shared_ptr<ubuntu::app_launch::Application::Instance>(const pid_t&));

    private:
        ubuntu::app_launch::AppID m_app_id;
        std::shared_ptr<Info> m_info;
    };

} // namespace
} // namespace app_launch
} // namespace ubuntu
