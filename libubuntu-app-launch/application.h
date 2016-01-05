
#include <sys/types.h>
#include <vector>
#include <memory>
#include <list>

#include "type-tagger.h"

#pragma once
#pragma GCC visibility push(default)

namespace Ubuntu {
namespace AppLaunch {

class Registry;

class Application {
public:
	struct PackageTag;
	struct AppNameTag;
	struct VersionTag;
	struct URLTag;

	typedef TypeTagger<PackageTag, std::string> Package;
	typedef TypeTagger<AppNameTag, std::string> AppName;
	typedef TypeTagger<VersionTag, std::string> Version;
	typedef TypeTagger<URLTag, std::string> URL;

	struct AppID {
		Package package;
		AppName appname;
		Version version;

		operator std::string() const;
		int operator == (const AppID &other) const;
		int operator != (const AppID &other) const;

		AppID();
		AppID(Package pkg, AppName app, Version ver);
		bool empty () const;

		static AppID parse (const std::string &appid);

		enum ApplicationWildcard {
			FIRST_LISTED,
			LAST_LISTED
		};
		enum VersionWildcard {
			CURRENT_USER_VERSION
		};

		static AppID discover (const std::string &package);
		static AppID discover (const std::string &package,
		                       const std::string &appname);
		static AppID discover (const std::string &package,
		                       const std::string &appname,
		                       const std::string &version);
		static AppID discover (const std::string &package,
		                       ApplicationWildcard appwildcard);
		static AppID discover (const std::string &package,
		                       ApplicationWildcard appwildcard,
		                       VersionWildcard versionwildcard);
		static AppID discover (const std::string &package,
		                       const std::string &appname,
		                       VersionWildcard versionwildcard);
	};

	static std::shared_ptr<Application> create (const AppID &appid,
	                                            std::shared_ptr<Registry> registry);

	/* System level info */
	virtual const Package &package() = 0;
	virtual const AppName &appname() = 0;
	virtual const Version &version() = 0;
	virtual AppID appId() = 0;


	class Info {
	public:
		struct NameTag;
		struct DescriptionTag;
		struct IconPathTag;
		struct CategoryTag;

		typedef TypeTagger<NameTag, std::string> Name;
		typedef TypeTagger<DescriptionTag, std::string> Description;
		typedef TypeTagger<IconPathTag, std::string> IconPath;
		typedef TypeTagger<CategoryTag, std::string> Category;

		/* Package provided user visible info */
		virtual const Name &name() = 0;
		virtual const Description &description() = 0;
		virtual const IconPath &iconPath() = 0;
		virtual std::list<Category> categories() = 0;
	};

	virtual std::shared_ptr<Info> info() = 0;

	class Instance {
	public:
		/* Query lifecycle */
		virtual bool isRunning() = 0;

		/* Instance Info */
		virtual const std::string &logPath() = 0;

		/* PIDs */
		virtual pid_t primaryPid() = 0;
		virtual bool hasPid(pid_t pid) = 0;
		virtual std::vector<pid_t> pids() = 0;

		/* Manage lifecycle */
		virtual void pause() = 0;
		virtual void resume() = 0;
		virtual void stop() = 0;
	};

	virtual bool hasInstances() = 0;
	virtual std::vector<std::shared_ptr<Instance>> instances() = 0;

	virtual std::shared_ptr<Instance> launch(std::vector<URL> urls = {}) = 0;
};

}; // namespace AppLaunch
}; // namespace Ubuntu

#pragma GCC visibility pop
