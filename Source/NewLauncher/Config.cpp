#pragma once
#include "Config.h"

namespace config
{
	std::wstring WindowTitle	= L"Trickster Launcher";	  // Window title in the TaskBar
	std::string SubTitle	    = "Trickster Launcher";		 // text that appears at the top of the launcher
	std::string LauncherCDN		= "cdn.selenoid.com.br";		// Your launcher website, without http or https
	std::wstring BaseNewsURL	= L"https://google.com.br";// Link to your news page
	std::string WebsiteLink		= "https://google.com";	  // Link to your website
	std::string OptionExecName  = "Setup.exe";			 // Name of the Options executable
	bool IsCDNUsingSSL			= true;					// Tells the launcher if your launcher update link uses SSL or not
	bool IsDllInjectEnable		= false;			   // Tells the launcher to inject or not the DLL defined below when you click Game Start
	std::string InjectDLLName	= "Trickster.dll";	  // Name of the DLL to be injected by the option above
}