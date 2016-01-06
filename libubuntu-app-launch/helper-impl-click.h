
#include <list>

#include "helper.h"

namespace Ubuntu {
namespace AppLaunch {
namespace HelperImpls {

class Click : public Helper {
public:
	Click (Helper::Type type, AppID appid, std::shared_ptr<Registry> registry) :
		_type(type), _appid(appid), _registry(registry)
	{
	}

	AppID appId() override {
		return _appid;
	}

	bool hasInstances() override;
	std::vector<std::shared_ptr<Helper::Instance>> instances() override;

	std::shared_ptr<Helper::Instance> launch(std::vector<Helper::URL> urls = {}) override;
	std::shared_ptr<Helper::Instance> launch(MirPromptSession * session, std::vector<Helper::URL> urls = {}) override;

	static std::list<std::shared_ptr<Helper>> running(Helper::Type type, std::shared_ptr<Registry> registry);

private:
	Helper::Type _type;
	AppID _appid;
	std::shared_ptr<Registry> _registry;
};

}; // namespace HelperImpl
}; // namespace AppLaunch
}; // namespace Ubuntu

