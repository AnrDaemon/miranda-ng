/* 
Copyright (C) 2012 Mataes

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this file; see the file license.txt.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  
*/

#include "stdafx.h"

static IconItem iconList[] =
{
	{ LPGEN("Protocol icon"),        "main",        IDI_ICON},
	{ LPGEN("Check All Feeds"),      "checkall",    IDI_CHECKALL},
	{ LPGEN("Add Feed"),             "addfeed",     IDI_ADDFEED},
	{ LPGEN("Import Feeds"),         "importfeeds", IDI_IMPORTFEEDS},
	{ LPGEN("Export Feeds"),         "exportfeeds", IDI_EXPORTFEEDS},
	{ LPGEN("Check Feed"),           "checkfeed",   IDI_CHECKALL},
	{ LPGEN("Auto Update Enabled"),  "enabled",     IDI_ENABLED},
	{ LPGEN("Auto Update Disabled"), "disabled",    IDI_DISABLED}
};

void InitIcons()
{
	g_plugin.registerIcon(LPGEN("News Aggregator"), iconList, MODULENAME);
}

HICON LoadIconEx(const char *name, bool big)
{
	char szSettingName[100];
	mir_snprintf(szSettingName, "%s_%s", MODULENAME, name);
	return IcoLib_GetIcon(szSettingName, big);
}

HANDLE GetIconHandle(const char *name)
{
	for (int i=0; i < _countof(iconList); i++)
		if ( !mir_strcmp(iconList[i].szName, name))
			return iconList[i].hIcolib;

	return nullptr;
}
