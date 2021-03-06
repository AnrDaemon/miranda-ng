#include "stdafx.h"

CMPlugin g_plugin;

HANDLE g_hCLibsFolder;
HANDLE g_hScriptsFolder;

HNETLIBUSER hNetlib = nullptr;

/////////////////////////////////////////////////////////////////////////////////////////

PLUGININFOEX pluginInfoEx =
{
	sizeof(PLUGININFOEX),
	__PLUGIN_NAME,
	PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
	__DESCRIPTION,
	__AUTHOR,
	__COPYRIGHT,
	__AUTHORWEB,
	UNICODE_AWARE,
	// {27d41d81-991f-4dc6-8749-b0321c87e694}
	{ 0x27d41d81, 0x991f, 0x4dc6, { 0x87, 0x49, 0xb0, 0x32, 0x1c, 0x87, 0xe6, 0x94 } }

};

/////////////////////////////////////////////////////////////////////////////////////////

int OnOptionsInit(WPARAM wParam, LPARAM)
{
	OPTIONSDIALOGPAGE odp = {};
	odp.hInstance = g_plugin.getInst();
	odp.flags = ODPF_BOLDGROUPS | ODPF_UNICODE | ODPF_DONTTRANSLATE;
	odp.szGroup.w = LPGENW("Services");
	odp.szTitle.w = L"Lua";
	odp.szTab.w = LPGENW("Scripts");
	odp.pDialog = new CMLuaOptions();
	g_plugin.addOptions(wParam, &odp);
	return 0;
}

int OnModulesLoaded(WPARAM, LPARAM)
{
	g_hCLibsFolder = FoldersRegisterCustomPathT(MODULENAME, "CLibsFolder", MIRLUA_PATHT, TranslateT("C libs folder"));
	g_hScriptsFolder = FoldersRegisterCustomPathT(MODULENAME, "ScriptsFolder", MIRLUA_PATHT, TranslateT("Scripts folder"));

	HookEvent(ME_OPT_INITIALISE, OnOptionsInit);
	return 0;
}

extern "C" int __declspec(dllexport) Load(void)
{
	InitIcons();

	NETLIBUSER nlu = {};
	nlu.flags = NUF_OUTGOING | NUF_INCOMING | NUF_HTTPCONNS;
	nlu.szDescriptiveName.a = MODULENAME;
	nlu.szSettingsModule = MODULENAME;
	hNetlib = Netlib_RegisterUser(&nlu);

	HookEvent(ME_SYSTEM_MODULESLOADED, OnModulesLoaded);

	g_plugin.Load();

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

extern "C" int __declspec(dllexport) Unload(void)
{
	if (hNetlib) {
		Netlib_CloseHandle(hNetlib);
		hNetlib = nullptr;
	}

	return 0;
}
