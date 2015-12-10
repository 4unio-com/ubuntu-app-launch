
#include <list>
#include <memory>
#include <functional>

#pragma once

namespace Ubuntu {
namespace AppLaunch {

/* Forward declarations */
class Application;
class ObserverHandle;

class Connection {
public:
	enum FailureType {
		CRASH,
		START_FAILURE,
	};

	Connection();

	/* Lots of application lists */
	static std::list<std::shared_ptr<Application>> runningApps(std::shared_ptr<Connection> connection = getDefault());
	static std::list<std::shared_ptr<Application>> installedApps(std::shared_ptr<Connection> connection = getDefault());

	/* Observers, NOTE: All functions called on a different thread */
	typedef std::function<void(const std::string& appid)> appObserver;
	typedef std::function<void(const std::string& appid, FailureType reason)> appFailedObserver;

	ObserverHandle observeAppStarting (appObserver callback);
	ObserverHandle observeAppStarted (appObserver callback);
	ObserverHandle observeAppStopped (appObserver callback);
	ObserverHandle observeAppFailed (appFailedObserver callback);
	ObserverHandle observeAppFocus (appObserver callback);
	ObserverHandle observeAppResume (appObserver callback);
	ObserverHandle observeAppResumed (appObserver callback);

	static std::shared_ptr<Connection> getDefault();

	class Impl;
	std::unique_ptr<Impl> impl;
};

}; // namespace AppLaunch
}; // namespace Ubuntu
