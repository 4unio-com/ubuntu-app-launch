
#include <gio/gio.h>
#include <glib.h>
#include <memory>
#include <string>

#include "application-info-desktop.h"
#include "registry.h"

static void clear_keyfile(GKeyFile* keyfile)
{
    if (keyfile != nullptr)
    {
        g_key_file_free(keyfile);
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    auto keyfile = std::shared_ptr<GKeyFile>(g_key_file_new(), clear_keyfile);

    GError* error = nullptr;
    g_key_file_load_from_data(keyfile.get(), reinterpret_cast<const gchar*>(data), size, G_KEY_FILE_NONE, &error);
    if (error != nullptr)
    {
        g_error_free(error);
        return 0;
    }

    try
    {
        auto registry = std::make_shared<ubuntu::app_launch::Registry>();
        auto appinfo = std::make_shared<ubuntu::app_launch::app_info::Desktop>(
            keyfile, "/", "/", ubuntu::app_launch::app_info::DesktopFlags::ALLOW_NO_DISPLAY, registry);
    }
    catch (...)
    {
    }

    return 0;
}
