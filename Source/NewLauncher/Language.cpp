#include "Language.h"

namespace lang
{
    Dictionary Languages = 
    {
        {"splash_check",                 "Verifying file: "},
        {"launcher_worker_complete",     "Update complete!"},
        {"launcher_worker_maintenance",  "In Maintenance!"},
        {"launcher_worker_downloading",  "Downloading"},
        {"launcher_game_start",          "GAME START"},
        {"launcher_options",             "OPTION"},
        {"launcher_exit",                "EXIT"},
        {"launcher_verify",              "CHECK FILES"},
        {"launcher_site_desc",           "You can visit our website by"},
        {"launcher_site_click",          "CLICKING HERE"},
        {"launcher_update_launch_fail",  "Failed to launch the updated launcher."},
		{"launcher_update_download_fail","Failed to download the updated launcher."},
	    {"launcher_update_check_fail",   "Failed to check for launcher updates."},
        {"launcher_filelist_building",   "Creating update list..."},
    };
}

std::string lang::GetString(const std::string& key) noexcept 
{
    auto it = Languages.find(key);
    if (it != Languages.end())
        return it->second;
    return "";
}