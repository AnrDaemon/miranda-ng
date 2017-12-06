/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (c) 2012-17 Miranda NG project (https://miranda-ng.org),
Copyright (c) 2000-12 Miranda IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "stdafx.h"
#include "resource.h"

HWND hAboutDlg = nullptr;

#define STR_VERSION_FORMAT L"Miranda NG\nv%S"

static INT_PTR CALLBACK DlgProcAbout(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int iState = 0;
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		SetDlgItemText(hwndDlg, IDC_DEVS, _T(LEGAL_COPYRIGHT));
		{
			char productVersion[56];
			Miranda_GetVersionText(productVersion, _countof(productVersion));

			wchar_t str[64];
			mir_snwprintf(str, STR_VERSION_FORMAT, productVersion);
			SetDlgItemText(hwndDlg, IDC_HEADERBAR, str);
		}
		ShowWindow(GetDlgItem(hwndDlg, IDC_CREDITSFILE), SW_HIDE);
		{
			HRSRC hResInfo = FindResource(g_hInst, MAKEINTRESOURCE(IDR_CREDITS), L"TEXT");
			DWORD ResSize = SizeofResource(g_hInst, hResInfo);
			HGLOBAL hRes = LoadResource(g_hInst, hResInfo);
			char *pszMsg = (char*)LockResource(hRes);
			if (pszMsg) {
				char *pszMsgt = (char*)alloca(ResSize + 1);
				memcpy(pszMsgt, pszMsg, ResSize); pszMsgt[ResSize] = 0;

				wchar_t *ptszMsg;
				if (ResSize >= 3 && pszMsgt[0] == '\xef' && pszMsgt[1] == '\xbb' && pszMsgt[2] == '\xbf')
					ptszMsg = Utf8DecodeW(pszMsgt + 3);
				else
					ptszMsg = mir_a2u_cp(pszMsgt, 1252);

				SetDlgItemText(hwndDlg, IDC_CREDITSFILE, ptszMsg);
				UnlockResource(pszMsg);
				mir_free(ptszMsg);
			}
			FreeResource(hRes);
		}
		Window_SetSkinIcon_IcoLib(hwndDlg, SKINICON_OTHER_MIRANDA);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			DestroyWindow(hwndDlg);
			return TRUE;
		
		case IDC_CONTRIBLINK:
			if (iState) {
				iState = 0;
				SetDlgItemText(hwndDlg, IDC_CONTRIBLINK, TranslateT("Credits >"));
				ShowWindow(GetDlgItem(hwndDlg, IDC_DEVS), SW_SHOW);
				ShowWindow(GetDlgItem(hwndDlg, IDC_CREDITSFILE), SW_HIDE);
			}
			else {
				iState = 1;
				SetDlgItemText(hwndDlg, IDC_CONTRIBLINK, TranslateT("< Copyright"));
				ShowWindow(GetDlgItem(hwndDlg, IDC_DEVS), SW_HIDE);
				ShowWindow(GetDlgItem(hwndDlg, IDC_CREDITSFILE), SW_SHOW);
			}
			break;
		}
		break;

	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		switch (GetWindowLongPtr((HWND)lParam, GWL_ID)) {
		case IDC_WHITERECT:
		case IDC_CREDITSFILE:
		case IDC_DEVS:
			SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
			break;

		default:
			return FALSE;
		}
		SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
		return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);

	case WM_DESTROY:
		Window_FreeIcon_IcoLib(hwndDlg);
		{
			HFONT hFont = (HFONT)SendDlgItemMessage(hwndDlg, IDC_VERSION, WM_GETFONT, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_VERSION, WM_SETFONT, SendDlgItemMessage(hwndDlg, IDOK, WM_GETFONT, 0, 0), 0);
			DeleteObject(hFont);
		}
		break;
	}
	return FALSE;
}

static INT_PTR AboutCommand(WPARAM wParam, LPARAM)
{
	if (hAboutDlg) {
		SetForegroundWindow(hAboutDlg);
		SetFocus(hAboutDlg);
	}
	else hAboutDlg = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_ABOUT), (HWND)wParam, DlgProcAbout);
	return 0;
}

static INT_PTR IndexCommand(WPARAM, LPARAM)
{
	Utils_OpenUrl("https://wiki.miranda-ng.org");
	return 0;
}

static INT_PTR WebsiteCommand(WPARAM, LPARAM)
{
	Utils_OpenUrl("https://miranda-ng.org");
	return 0;
}

static INT_PTR BugCommand(WPARAM, LPARAM)
{
	Utils_OpenUrl("https://github.com/miranda-ng/miranda-ng/issues/new");
	return 0;
}

int ShutdownHelpModule(WPARAM, LPARAM)
{
	if (hAboutDlg) {
		DestroyWindow(hAboutDlg);
		hAboutDlg = nullptr;
	}
	return 0;
}

int LoadHelpModule(void)
{
	HookEvent(ME_SYSTEM_PRESHUTDOWN, ShutdownHelpModule);

	CMenuItem mi;
	mi.root = Menu_CreateRoot(MO_MAIN, LPGENW("&Help"), 2000090000);
	Menu_ConfigureItem(mi.root, MCI_OPT_UID, "8824ECA5-6942-46D7-9D07-1BA600E0D02E");

	SET_UID(mi, 0xf3ebf1fa, 0x587c, 0x494d, 0xbd, 0x33, 0x7f, 0x88, 0xb3, 0x61, 0x1e, 0xd3);
	mi.hIcolibItem = Skin_GetIconHandle(SKINICON_OTHER_MIRANDA);
	mi.position = 2000090000;
	mi.name.a = LPGEN("&About...");
	mi.pszService = "Help/AboutCommand";
	Menu_AddMainMenuItem(&mi);
	CreateServiceFunction(mi.pszService, AboutCommand);

	SET_UID(mi, 0x495df66f, 0x844e, 0x479a, 0xaf, 0x21, 0x3e, 0x42, 0xc5, 0x14, 0x7c, 0x7e);
	mi.hIcolibItem = Skin_GetIconHandle(SKINICON_OTHER_HELP);
	mi.position = -500050000;
	mi.name.a = LPGEN("&Support");
	mi.pszService = "Help/IndexCommand";
	Menu_AddMainMenuItem(&mi);
	CreateServiceFunction(mi.pszService, IndexCommand);

	SET_UID(mi, 0x15e18b58, 0xec73, 0x45c2, 0xb9, 0xf4, 0x2a, 0xfe, 0xc2, 0xb7, 0xd3, 0x25);
	mi.hIcolibItem = Skin_GetIconHandle(SKINICON_OTHER_MIRANDAWEB);
	mi.position = 2000050000;
	mi.name.a = LPGEN("&Miranda NG homepage");
	mi.pszService = "Help/WebsiteCommand";
	Menu_AddMainMenuItem(&mi);
	CreateServiceFunction(mi.pszService, WebsiteCommand);

	SET_UID(mi, 0xe7d0fe8b, 0xfdeb, 0x45b3, 0xba, 0x83, 0x3, 0x1e, 0x15, 0xda, 0x7e, 0x52);
	mi.hIcolibItem = Skin_GetIconHandle(SKINICON_EVENT_URL);
	mi.position = 2000040000;
	mi.name.a = LPGEN("&Report bug");
	mi.pszService = "Help/BugCommand";
	Menu_AddMainMenuItem(&mi);
	CreateServiceFunction(mi.pszService, BugCommand);
	return 0;
}