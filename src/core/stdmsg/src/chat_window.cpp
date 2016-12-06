/*
Chat module plugin for Miranda IM

Copyright 2000-12 Miranda IM, 2012-16 Miranda NG project,
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

static HKL hkl = NULL;

struct MESSAGESUBDATA
{
	time_t lastEnterTime;
	wchar_t  szTabSave[20];
};

static wchar_t szTrimString[] = L":;,.!?\'\"><()[]- \r\n";

static LRESULT CALLBACK SplitterSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	RECT rc;

	switch (msg) {
	case WM_NCHITTEST:
		return HTCLIENT;

	case WM_SETCURSOR:
		GetClientRect(hwnd, &rc);
		SetCursor(rc.right > rc.bottom ? LoadCursor(NULL, IDC_SIZENS) : LoadCursor(NULL, IDC_SIZEWE));
		return TRUE;

	case WM_LBUTTONDOWN:
		SetCapture(hwnd);
		return 0;

	case WM_MOUSEMOVE:
		if (GetCapture() == hwnd) {
			GetClientRect(hwnd, &rc);
			SendMessage(GetParent(hwnd), GC_SPLITTERMOVED, rc.right > rc.bottom ? (short)HIWORD(GetMessagePos()) + rc.bottom / 2 : (short)LOWORD(GetMessagePos()) + rc.right / 2, (LPARAM)hwnd);
		}
		return 0;

	case WM_LBUTTONUP:
		ReleaseCapture();
		PostMessage(GetParent(hwnd), WM_SIZE, 0, 0);
		return 0;
	}
	return mir_callNextSubclass(hwnd, SplitterSubclassProc, msg, wParam, lParam);
}

static void InitButtons(HWND hwndDlg, SESSION_INFO *si)
{
	SendDlgItemMessage(hwndDlg, IDC_CLOSE, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIconEx("close", FALSE));
	SendDlgItemMessage(hwndDlg, IDC_CLOSE, BUTTONSETASFLATBTN, TRUE, 0);
	SendDlgItemMessage(hwndDlg, IDC_CLOSE, BUTTONADDTOOLTIP, (WPARAM)LPGEN("Close current tab (CTRL+F4)"), 0);

	SendDlgItemMessage(hwndDlg, IDC_SHOWNICKLIST, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIconEx(si->bNicklistEnabled ? "nicklist" : "nicklist2", FALSE));
	SendDlgItemMessage(hwndDlg, IDC_FILTER, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIconEx(si->bFilterEnabled ? "filter" : "filter2", FALSE));

	MODULEINFO *pInfo = pci->MM_FindModule(si->pszModule);
	if (pInfo) {
		EnableWindow(GetDlgItem(hwndDlg, IDC_BOLD), pInfo->bBold);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ITALICS), pInfo->bItalics);
		EnableWindow(GetDlgItem(hwndDlg, IDC_UNDERLINE), pInfo->bUnderline);
		EnableWindow(GetDlgItem(hwndDlg, IDC_COLOR), pInfo->bColor);
		EnableWindow(GetDlgItem(hwndDlg, IDC_BKGCOLOR), pInfo->bBkgColor);
		if (si->iType == GCW_CHATROOM)
			EnableWindow(GetDlgItem(hwndDlg, IDC_CHANMGR), pInfo->bChanMgr);
	}
}

static int RoomWndResize(HWND hwndDlg, LPARAM lParam, UTILRESIZECONTROL *urc)
{
	SESSION_INFO *si = (SESSION_INFO*)lParam;

	BOOL bControl = (BOOL)db_get_b(NULL, CHAT_MODULE, "ShowTopButtons", 1);
	BOOL bFormat = (BOOL)db_get_b(NULL, CHAT_MODULE, "ShowFormatButtons", 1);
	BOOL bToolbar = bFormat || bControl;
	BOOL bSend = (BOOL)db_get_b(NULL, CHAT_MODULE, "ShowSend", 0);
	BOOL bNick = si->iType != GCW_SERVER && si->bNicklistEnabled;
	BOOL bTabs = g_Settings.bTabsEnable;
	BOOL bTabBottom = g_Settings.TabsAtBottom;

	RECT rc, rcTabs;
	GetClientRect(GetDlgItem(hwndDlg, IDC_TAB), &rcTabs);
	int TabHeight = rcTabs.bottom - rcTabs.top;
	TabCtrl_AdjustRect(GetDlgItem(hwndDlg, IDC_TAB), FALSE, &rcTabs);
	TabHeight -= (rcTabs.bottom - rcTabs.top);
	ShowWindow(GetDlgItem(hwndDlg, IDC_BOLD), bFormat ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_UNDERLINE), bFormat ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_ITALICS), bFormat ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_COLOR), bFormat ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_BKGCOLOR), bFormat ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_HISTORY), bControl ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_SHOWNICKLIST), bControl ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_FILTER), bControl ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_CHANMGR), bControl ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDOK), bSend ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_SPLITTERX), bNick ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_CLOSE), g_Settings.bTabsEnable ? SW_SHOW : SW_HIDE);
	ShowWindow(GetDlgItem(hwndDlg, IDC_TAB), g_Settings.bTabsEnable ? SW_SHOW : SW_HIDE);
	if (si->iType != GCW_SERVER)
		ShowWindow(GetDlgItem(hwndDlg, IDC_LIST), si->bNicklistEnabled ? SW_SHOW : SW_HIDE);
	else
		ShowWindow(GetDlgItem(hwndDlg, IDC_LIST), SW_HIDE);

	if (si->iType == GCW_SERVER) {
		EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWNICKLIST), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_FILTER), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHANMGR), FALSE);
	}
	else {
		EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWNICKLIST), TRUE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_FILTER), TRUE);
		if (si->iType == GCW_CHATROOM)
			EnableWindow(GetDlgItem(hwndDlg, IDC_CHANMGR), pci->MM_FindModule(si->pszModule)->bChanMgr);
	}

	switch (urc->wId) {
	case IDOK:
		GetWindowRect(si->hwndStatus, &rc);
		urc->rcItem.left = bSend ? 315 : urc->dlgNewSize.cx;
		urc->rcItem.top = urc->dlgNewSize.cy - si->iSplitterY + 23;
		urc->rcItem.bottom = urc->dlgNewSize.cy - (rc.bottom - rc.top) - 1;
		return RD_ANCHORX_RIGHT | RD_ANCHORY_CUSTOM;

	case IDC_TAB:
		urc->rcItem.top = 1;
		urc->rcItem.left = 0;
		urc->rcItem.right = urc->dlgNewSize.cx - 24;
		urc->rcItem.bottom = urc->dlgNewSize.cy - si->iSplitterY;
		if (!bToolbar)
			urc->rcItem.bottom += 20;
		return RD_ANCHORX_CUSTOM | RD_ANCHORY_CUSTOM;

	case IDC_LOG:
		urc->rcItem.top = bTabs ? (bTabBottom ? 0 : rcTabs.top - 1) : 0;
		urc->rcItem.left = 0;
		urc->rcItem.right = bNick ? urc->dlgNewSize.cx - si->iSplitterX : urc->dlgNewSize.cx;
	LBL_CalcBottom:
		urc->rcItem.bottom = urc->dlgNewSize.cy - si->iSplitterY;
		if (bTabs && bTabBottom) urc->rcItem.bottom += 6 - TabHeight;
		if (!bToolbar) urc->rcItem.bottom += 20;
		return RD_ANCHORX_CUSTOM | RD_ANCHORY_CUSTOM;

	case IDC_LIST:
		urc->rcItem.top = bTabs ? (bTabBottom ? 0 : rcTabs.top - 1) : 0;
		urc->rcItem.right = urc->dlgNewSize.cx;
		urc->rcItem.left = urc->dlgNewSize.cx - si->iSplitterX + 2;
		goto LBL_CalcBottom;

	case IDC_SPLITTERX:
		urc->rcItem.top = bTabs ? rcTabs.top : 1;
		urc->rcItem.left = urc->dlgNewSize.cx - si->iSplitterX;
		urc->rcItem.right = urc->rcItem.left + 2;
		goto LBL_CalcBottom;

	case IDC_SPLITTERY:
		urc->rcItem.top = urc->dlgNewSize.cy - si->iSplitterY;
		if (!bToolbar)
			urc->rcItem.top += 20;
		urc->rcItem.bottom = urc->rcItem.top + 2;
		return RD_ANCHORX_WIDTH | RD_ANCHORY_CUSTOM;

	case IDC_MESSAGE:
		GetWindowRect(si->hwndStatus, &rc);
		urc->rcItem.right = bSend ? urc->dlgNewSize.cx - 64 : urc->dlgNewSize.cx;
		urc->rcItem.top = urc->dlgNewSize.cy - si->iSplitterY + 22;
		urc->rcItem.bottom = urc->dlgNewSize.cy - (rc.bottom - rc.top) - 1;
		return RD_ANCHORX_LEFT | RD_ANCHORY_CUSTOM;

	case IDC_ITALICS:
	case IDC_BOLD:
	case IDC_UNDERLINE:
	case IDC_COLOR:
	case IDC_BKGCOLOR:
		urc->rcItem.top = urc->dlgNewSize.cy - si->iSplitterY + 3;
		urc->rcItem.bottom = urc->rcItem.top + 16;
		return RD_ANCHORX_LEFT | RD_ANCHORY_CUSTOM;

	case IDC_HISTORY:
	case IDC_CHANMGR:
	case IDC_SHOWNICKLIST:
	case IDC_FILTER:
		urc->rcItem.top = urc->dlgNewSize.cy - si->iSplitterY + 3;
		urc->rcItem.bottom = urc->rcItem.top + 16;
		return RD_ANCHORX_RIGHT | RD_ANCHORY_CUSTOM;

	case IDC_CLOSE:
		urc->rcItem.left = urc->dlgNewSize.cx - 19;
		urc->rcItem.right = urc->rcItem.left + 16;
		urc->rcItem.top = bTabBottom ? (bToolbar ? urc->dlgNewSize.cy - si->iSplitterY - 18 : urc->dlgNewSize.cy - si->iSplitterY - 18 + 20) : 3;
		urc->rcItem.bottom = urc->rcItem.top + 16;
		return RD_ANCHORX_CUSTOM | RD_ANCHORY_CUSTOM;
	}
	return RD_ANCHORX_LEFT | RD_ANCHORY_TOP;
}

static LRESULT CALLBACK MessageSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	SESSION_INFO *Parentsi = (SESSION_INFO*)GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA);
	MESSAGESUBDATA *dat = (MESSAGESUBDATA*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (msg) {
	case EM_SUBCLASSED:
		dat = (MESSAGESUBDATA*)mir_alloc(sizeof(MESSAGESUBDATA));

		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)dat);
		dat->szTabSave[0] = '\0';
		dat->lastEnterTime = 0;
		return 0;

	case WM_MOUSEWHEEL:
		SendDlgItemMessage(GetParent(hwnd), IDC_LOG, WM_MOUSEWHEEL, wParam, lParam);
		dat->lastEnterTime = 0;
		return TRUE;

	case EM_REPLACESEL:
		PostMessage(hwnd, EM_ACTIVATE, 0, 0);
		break;

	case EM_ACTIVATE:
		SetActiveWindow(GetParent(hwnd));
		break;

	case WM_CHAR:
		{
			BOOL isCtrl = GetKeyState(VK_CONTROL) & 0x8000;
			BOOL isAlt = GetKeyState(VK_MENU) & 0x8000;

			if (GetWindowLongPtr(hwnd, GWL_STYLE) & ES_READONLY)
				break;

			if (wParam == 9 && isCtrl && !isAlt) // ctrl-i (italics)
				return TRUE;

			if (wParam == VK_SPACE && isCtrl && !isAlt) // ctrl-space (paste clean text)
				return TRUE;

			if (wParam == '\n' || wParam == '\r') {
				if ((isCtrl != 0) ^ (0 != db_get_b(NULL, CHAT_MODULE, "SendOnEnter", 1))) {
					PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
					return 0;
				}
				if (db_get_b(NULL, CHAT_MODULE, "SendOnDblEnter", 0)) {
					if (dat->lastEnterTime + 2 < time(NULL))
						dat->lastEnterTime = time(NULL);
					else {
						SendMessage(hwnd, WM_KEYDOWN, VK_BACK, 0);
						SendMessage(hwnd, WM_KEYUP, VK_BACK, 0);
						PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
						return 0;
					}
				}
			}
			else
				dat->lastEnterTime = 0;

			if (wParam == 1 && isCtrl && !isAlt) {      //ctrl-a
				SendMessage(hwnd, EM_SETSEL, 0, -1);
				return 0;
			}
		}
		break;

	case WM_KEYDOWN:
		{
			static int start, end;
			BOOL isShift = GetKeyState(VK_SHIFT) & 0x8000;
			BOOL isCtrl = GetKeyState(VK_CONTROL) & 0x8000;
			BOOL isAlt = GetKeyState(VK_MENU) & 0x8000;
			if (wParam == VK_RETURN) {
				dat->szTabSave[0] = '\0';
				if ((isCtrl != 0) ^ (0 != db_get_b(NULL, CHAT_MODULE, "SendOnEnter", 1)))
					return 0;

				if (db_get_b(NULL, CHAT_MODULE, "SendOnDblEnter", 0))
					if (dat->lastEnterTime + 2 >= time(NULL))
						return 0;

				break;
			}

			if (wParam == VK_TAB && isShift && !isCtrl) { // SHIFT-TAB (go to nick list)
				SetFocus(GetDlgItem(GetParent(hwnd), IDC_LIST));
				return TRUE;
			}

			if (wParam == VK_TAB && isCtrl && !isShift) { // CTRL-TAB (switch tab/window)
				if (g_Settings.bTabsEnable)
					SendMessage(GetParent(hwnd), GC_SWITCHNEXTTAB, 0, 0);
				else
					pci->ShowRoom(SM_GetNextWindow(Parentsi), WINDOW_VISIBLE, TRUE);
				return TRUE;
			}

			if (wParam == VK_TAB && isCtrl && isShift) { // CTRL_SHIFT-TAB (switch tab/window)
				if (g_Settings.bTabsEnable)
					SendMessage(GetParent(hwnd), GC_SWITCHPREVTAB, 0, 0);
				else
					pci->ShowRoom(SM_GetPrevWindow(Parentsi), WINDOW_VISIBLE, TRUE);
				return TRUE;
			}

			if (wParam <= '9' && wParam >= '1' && isCtrl && !isAlt) // CTRL + 1 -> 9 (switch tab)
				if (g_Settings.bTabsEnable)
					SendMessage(GetParent(hwnd), GC_SWITCHTAB, 0, (int)wParam - (int)'1');

			if (wParam <= VK_NUMPAD9 && wParam >= VK_NUMPAD1 && isCtrl && !isAlt) // CTRL + 1 -> 9 (switch tab)
				if (g_Settings.bTabsEnable)
					SendMessage(GetParent(hwnd), GC_SWITCHTAB, 0, (int)wParam - (int)VK_NUMPAD1);

			if (wParam == VK_TAB && !isCtrl && !isShift) {    //tab-autocomplete
				wchar_t* pszText = NULL;
				LRESULT lResult = (LRESULT)SendMessage(hwnd, EM_GETSEL, 0, 0);

				SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
				start = LOWORD(lResult);
				end = HIWORD(lResult);
				SendMessage(hwnd, EM_SETSEL, end, end);

				GETTEXTLENGTHEX gtl = { 0 };
				gtl.flags = GTL_PRECISE;
				gtl.codepage = CP_ACP;
				int iLen = SendMessage(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
				if (iLen > 0) {
					pszText = (wchar_t *)mir_alloc(sizeof(wchar_t)*(iLen + 100));

					GETTEXTEX gt = { 0 };
					gt.cb = iLen + 99;
					gt.flags = GT_DEFAULT;
					gt.codepage = 1200;

					SendMessage(hwnd, EM_GETTEXTEX, (WPARAM)&gt, (LPARAM)pszText);
					while (start > 0 && pszText[start - 1] != ' ' && pszText[start - 1] != 13 && pszText[start - 1] != VK_TAB)
						start--;
					while (end < iLen && pszText[end] != ' ' && pszText[end] != 13 && pszText[end - 1] != VK_TAB)
						end++;

					if (dat->szTabSave[0] == '\0')
						mir_wstrncpy(dat->szTabSave, pszText + start, end - start + 1);

					wchar_t *pszSelName = (wchar_t *)mir_alloc(sizeof(wchar_t)*(end - start + 1));
					mir_wstrncpy(pszSelName, pszText + start, end - start + 1);

					wchar_t *pszName = pci->UM_FindUserAutoComplete(Parentsi->pUsers, dat->szTabSave, pszSelName);
					if (pszName == NULL) {
						pszName = dat->szTabSave;
						SendMessage(hwnd, EM_SETSEL, start, end);
						if (end != start)
							SendMessage(hwnd, EM_REPLACESEL, FALSE, (LPARAM)pszName);
						dat->szTabSave[0] = '\0';
					}
					else {
						SendMessage(hwnd, EM_SETSEL, start, end);
						if (end != start)
							SendMessage(hwnd, EM_REPLACESEL, FALSE, (LPARAM)pszName);
					}
					mir_free(pszText);
					mir_free(pszSelName);
				}

				SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
				RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
				return 0;
			}

			if (dat->szTabSave[0] != '\0' && wParam != VK_RIGHT && wParam != VK_LEFT && wParam != VK_SPACE && wParam != VK_RETURN && wParam != VK_BACK && wParam != VK_DELETE) {
				if (g_Settings.bAddColonToAutoComplete && start == 0)
					SendMessageA(hwnd, EM_REPLACESEL, FALSE, (LPARAM) ": ");

				dat->szTabSave[0] = '\0';
			}

			if (wParam == VK_F4 && isCtrl && !isAlt) { // ctrl-F4 (close tab)
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_CLOSE, BN_CLICKED), 0);
				return TRUE;
			}

			if (wParam == 0x49 && isCtrl && !isAlt) { // ctrl-i (italics)
				CheckDlgButton(GetParent(hwnd), IDC_ITALICS, IsDlgButtonChecked(GetParent(hwnd), IDC_ITALICS) == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_ITALICS, 0), 0);
				return TRUE;
			}

			if (wParam == 0x42 && isCtrl && !isAlt) { // ctrl-b (bold)
				CheckDlgButton(GetParent(hwnd), IDC_BOLD, IsDlgButtonChecked(GetParent(hwnd), IDC_BOLD) == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_BOLD, 0), 0);
				return TRUE;
			}

			if (wParam == 0x55 && isCtrl && !isAlt) { // ctrl-u (paste clean text)
				CheckDlgButton(GetParent(hwnd), IDC_UNDERLINE, IsDlgButtonChecked(GetParent(hwnd), IDC_UNDERLINE) == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_UNDERLINE, 0), 0);
				return TRUE;
			}

			if (wParam == 0x4b && isCtrl && !isAlt) { // ctrl-k (paste clean text)
				CheckDlgButton(GetParent(hwnd), IDC_COLOR, IsDlgButtonChecked(GetParent(hwnd), IDC_COLOR) == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_COLOR, 0), 0);
				return TRUE;
			}

			if (wParam == VK_SPACE && isCtrl && !isAlt) { // ctrl-space (paste clean text)
				CheckDlgButton(GetParent(hwnd), IDC_BKGCOLOR, BST_UNCHECKED);
				CheckDlgButton(GetParent(hwnd), IDC_COLOR, BST_UNCHECKED);
				CheckDlgButton(GetParent(hwnd), IDC_BOLD, BST_UNCHECKED);
				CheckDlgButton(GetParent(hwnd), IDC_UNDERLINE, BST_UNCHECKED);
				CheckDlgButton(GetParent(hwnd), IDC_ITALICS, BST_UNCHECKED);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_BKGCOLOR, 0), 0);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_COLOR, 0), 0);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_BOLD, 0), 0);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_UNDERLINE, 0), 0);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_ITALICS, 0), 0);
				return TRUE;
			}

			if (wParam == 0x4c && isCtrl && !isAlt) { // ctrl-l (paste clean text)
				CheckDlgButton(GetParent(hwnd), IDC_BKGCOLOR, IsDlgButtonChecked(GetParent(hwnd), IDC_BKGCOLOR) == BST_UNCHECKED ? BST_CHECKED : BST_UNCHECKED);
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_BKGCOLOR, 0), 0);
				return TRUE;
			}

			if (wParam == 0x46 && isCtrl && !isAlt) { // ctrl-f (paste clean text)
				if (IsWindowEnabled(GetDlgItem(GetParent(hwnd), IDC_FILTER)))
					SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_FILTER, 0), 0);
				return TRUE;
			}

			if (wParam == 0x4e && isCtrl && !isAlt) { // ctrl-n (nicklist)
				if (IsWindowEnabled(GetDlgItem(GetParent(hwnd), IDC_SHOWNICKLIST)))
					SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_SHOWNICKLIST, 0), 0);
				return TRUE;
			}

			if (wParam == 0x48 && isCtrl && !isAlt) { // ctrl-h (history)
				SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_HISTORY, 0), 0);
				return TRUE;
			}

			if (wParam == 0x4f && isCtrl && !isAlt) { // ctrl-o (options)
				if (IsWindowEnabled(GetDlgItem(GetParent(hwnd), IDC_CHANMGR)))
					SendMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_CHANMGR, 0), 0);
				return TRUE;
			}

			if ((wParam == 45 && isShift || wParam == 0x56 && isCtrl) && !isAlt) { // ctrl-v (paste clean text)
				SendMessage(hwnd, EM_PASTESPECIAL, CF_TEXT, 0);
				return TRUE;
			}

			if (wParam == 0x57 && isCtrl && !isAlt) { // ctrl-w (close window)
				PostMessage(GetParent(hwnd), WM_CLOSE, 0, 0);
				return TRUE;
			}

			if (wParam == VK_NEXT || wParam == VK_PRIOR) {
				HWND htemp = GetParent(hwnd);
				SendDlgItemMessage(htemp, IDC_LOG, msg, wParam, lParam);
				dat->lastEnterTime = 0;
				return TRUE;
			}

			if (wParam == VK_UP && isCtrl && !isAlt) {
				char* lpPrevCmd = pci->SM_GetPrevCommand(Parentsi->ptszID, Parentsi->pszModule);

				SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);

				if (lpPrevCmd) {
					SETTEXTEX ste;
					ste.flags = ST_DEFAULT;
					ste.codepage = CP_ACP;
					SendMessage(hwnd, EM_SETTEXTEX, (WPARAM)&ste, (LPARAM)lpPrevCmd);
				}
				else SetWindowText(hwnd, L"");

				GETTEXTLENGTHEX gtl = { 0 };
				gtl.flags = GTL_PRECISE;
				gtl.codepage = CP_ACP;
				int iLen = SendMessage(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
				SendMessage(hwnd, EM_SCROLLCARET, 0, 0);
				SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
				RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
				SendMessage(hwnd, EM_SETSEL, iLen, iLen);
				dat->lastEnterTime = 0;
				return TRUE;
			}

			if (wParam == VK_DOWN && isCtrl && !isAlt) {
				char* lpPrevCmd = pci->SM_GetNextCommand(Parentsi->ptszID, Parentsi->pszModule);
				SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);

				if (lpPrevCmd) {
					SETTEXTEX ste;
					ste.flags = ST_DEFAULT;
					ste.codepage = CP_ACP;
					SendMessage(hwnd, EM_SETTEXTEX, (WPARAM)&ste, (LPARAM)lpPrevCmd);
				}
				else SetWindowText(hwnd, L"");

				GETTEXTLENGTHEX gtl = { 0 };
				gtl.flags = GTL_PRECISE;
				gtl.codepage = CP_ACP;
				int iLen = SendMessage(hwnd, EM_GETTEXTLENGTHEX, (WPARAM)&gtl, 0);
				SendMessage(hwnd, EM_SCROLLCARET, 0, 0);
				SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
				RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE);
				SendMessage(hwnd, EM_SETSEL, iLen, iLen);
				dat->lastEnterTime = 0;
				return TRUE;
			}
		}
		//fall through

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_KILLFOCUS:
		dat->lastEnterTime = 0;
		break;

	case WM_RBUTTONDOWN:
		{
			CHARRANGE sel, all = { 0, -1 };
			HMENU hSubMenu = GetSubMenu(g_hMenu, 4);
			TranslateMenu(hSubMenu);
			SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM)&sel);

			EnableMenuItem(hSubMenu, ID_MESSAGE_UNDO, SendMessage(hwnd, EM_CANUNDO, 0, 0) ? MF_ENABLED : MF_GRAYED);
			EnableMenuItem(hSubMenu, ID_MESSAGE_REDO, SendMessage(hwnd, EM_CANREDO, 0, 0) ? MF_ENABLED : MF_GRAYED);
			EnableMenuItem(hSubMenu, ID_MESSAGE_COPY, sel.cpMax != sel.cpMin ? MF_ENABLED : MF_GRAYED);
			EnableMenuItem(hSubMenu, ID_MESSAGE_CUT, sel.cpMax != sel.cpMin ? MF_ENABLED : MF_GRAYED);

			dat->lastEnterTime = 0;

			POINT pt;
			pt.x = (short)LOWORD(lParam);
			pt.y = (short)HIWORD(lParam);
			ClientToScreen(hwnd, &pt);

			UINT uID = TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);
			switch (uID) {
			case 0:
				break;

			case ID_MESSAGE_UNDO:
				SendMessage(hwnd, EM_UNDO, 0, 0);
				break;

			case ID_MESSAGE_REDO:
				SendMessage(hwnd, EM_REDO, 0, 0);
				break;

			case ID_MESSAGE_COPY:
				SendMessage(hwnd, WM_COPY, 0, 0);
				break;

			case ID_MESSAGE_CUT:
				SendMessage(hwnd, WM_CUT, 0, 0);
				break;

			case ID_MESSAGE_PASTE:
				SendMessage(hwnd, EM_PASTESPECIAL, CF_TEXT, 0);
				break;

			case ID_MESSAGE_SELECTALL:
				SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM)& all);
				break;

			case ID_MESSAGE_CLEAR:
				SetWindowText(hwnd, L"");
				break;
			}
			PostMessage(hwnd, WM_KEYUP, 0, 0);
		}
		break;

	case WM_KEYUP:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
		{
			CHARFORMAT2 cf;
			cf.cbSize = sizeof(CHARFORMAT2);
			cf.dwMask = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_BACKCOLOR | CFM_COLOR;
			SendMessage(hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

			MODULEINFO *pmi = pci->MM_FindModule(Parentsi->pszModule);
			if (pmi && pmi->bColor) {
				int index = GetColorIndex(Parentsi->pszModule, cf.crTextColor);
				UINT u = IsDlgButtonChecked(GetParent(hwnd), IDC_COLOR);

				if (index >= 0) {
					Parentsi->bFGSet = TRUE;
					Parentsi->iFG = index;
				}

				if (u == BST_UNCHECKED && cf.crTextColor != g_Settings.MessageAreaColor)
					CheckDlgButton(GetParent(hwnd), IDC_COLOR, BST_CHECKED);
				else if (u == BST_CHECKED && cf.crTextColor == g_Settings.MessageAreaColor)
					CheckDlgButton(GetParent(hwnd), IDC_COLOR, BST_UNCHECKED);
			}

			if (pmi && pmi->bBkgColor) {
				int index = GetColorIndex(Parentsi->pszModule, cf.crBackColor);
				COLORREF crB = (COLORREF)db_get_dw(NULL, CHAT_MODULE, "ColorMessageBG", GetSysColor(COLOR_WINDOW));
				UINT u = IsDlgButtonChecked(GetParent(hwnd), IDC_BKGCOLOR);

				if (index >= 0) {
					Parentsi->bBGSet = TRUE;
					Parentsi->iBG = index;
				}
				if (u == BST_UNCHECKED && cf.crBackColor != crB)
					CheckDlgButton(GetParent(hwnd), IDC_BKGCOLOR, BST_CHECKED);
				else if (u == BST_CHECKED && cf.crBackColor == crB)
					CheckDlgButton(GetParent(hwnd), IDC_BKGCOLOR, BST_UNCHECKED);
			}

			if (pmi && pmi->bBold) {
				UINT u = IsDlgButtonChecked(GetParent(hwnd), IDC_BOLD);
				UINT u2 = cf.dwEffects;
				u2 &= CFE_BOLD;
				if (u == BST_UNCHECKED && u2)
					CheckDlgButton(GetParent(hwnd), IDC_BOLD, BST_CHECKED);
				else if (u == BST_CHECKED && u2 == 0)
					CheckDlgButton(GetParent(hwnd), IDC_BOLD, BST_UNCHECKED);
			}

			if (pmi && pmi->bItalics) {
				UINT u = IsDlgButtonChecked(GetParent(hwnd), IDC_ITALICS);
				UINT u2 = cf.dwEffects;
				u2 &= CFE_ITALIC;
				if (u == BST_UNCHECKED && u2)
					CheckDlgButton(GetParent(hwnd), IDC_ITALICS, BST_CHECKED);
				else if (u == BST_CHECKED && u2 == 0)
					CheckDlgButton(GetParent(hwnd), IDC_ITALICS, BST_UNCHECKED);
			}

			if (pmi && pmi->bUnderline) {
				UINT u = IsDlgButtonChecked(GetParent(hwnd), IDC_UNDERLINE);
				UINT u2 = cf.dwEffects;
				u2 &= CFE_UNDERLINE;
				if (u == BST_UNCHECKED && u2)
					CheckDlgButton(GetParent(hwnd), IDC_UNDERLINE, BST_CHECKED);
				else if (u == BST_CHECKED && u2 == 0)
					CheckDlgButton(GetParent(hwnd), IDC_UNDERLINE, BST_UNCHECKED);
			}
		}
		break;

	case WM_DESTROY:
		mir_free(dat);
		return 0;
	}

	return mir_callNextSubclass(hwnd, MessageSubclassProc, msg, wParam, lParam);
}

static INT_PTR CALLBACK FilterWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static SESSION_INFO *si = NULL;
	switch (uMsg) {
	case WM_INITDIALOG:
		si = (SESSION_INFO*)lParam;
		CheckDlgButton(hwndDlg, IDC_1, si->iLogFilterFlags & GC_EVENT_ACTION ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_2, si->iLogFilterFlags & GC_EVENT_MESSAGE ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_3, si->iLogFilterFlags & GC_EVENT_NICK ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_4, si->iLogFilterFlags & GC_EVENT_JOIN ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_5, si->iLogFilterFlags & GC_EVENT_PART ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_6, si->iLogFilterFlags & GC_EVENT_TOPIC ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_7, si->iLogFilterFlags & GC_EVENT_ADDSTATUS ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_8, si->iLogFilterFlags & GC_EVENT_INFORMATION ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_9, si->iLogFilterFlags & GC_EVENT_QUIT ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_10, si->iLogFilterFlags & GC_EVENT_KICK ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_11, si->iLogFilterFlags & GC_EVENT_NOTICE ? BST_CHECKED : BST_UNCHECKED);
		break;

	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		SetTextColor((HDC)wParam, RGB(60, 60, 150));
		SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
		return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);

	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE) {
			int iFlags = 0;

			if (IsDlgButtonChecked(hwndDlg, IDC_1) == BST_CHECKED)
				iFlags |= GC_EVENT_ACTION;
			if (IsDlgButtonChecked(hwndDlg, IDC_2) == BST_CHECKED)
				iFlags |= GC_EVENT_MESSAGE;
			if (IsDlgButtonChecked(hwndDlg, IDC_3) == BST_CHECKED)
				iFlags |= GC_EVENT_NICK;
			if (IsDlgButtonChecked(hwndDlg, IDC_4) == BST_CHECKED)
				iFlags |= GC_EVENT_JOIN;
			if (IsDlgButtonChecked(hwndDlg, IDC_5) == BST_CHECKED)
				iFlags |= GC_EVENT_PART;
			if (IsDlgButtonChecked(hwndDlg, IDC_6) == BST_CHECKED)
				iFlags |= GC_EVENT_TOPIC;
			if (IsDlgButtonChecked(hwndDlg, IDC_7) == BST_CHECKED)
				iFlags |= GC_EVENT_ADDSTATUS;
			if (IsDlgButtonChecked(hwndDlg, IDC_8) == BST_CHECKED)
				iFlags |= GC_EVENT_INFORMATION;
			if (IsDlgButtonChecked(hwndDlg, IDC_9) == BST_CHECKED)
				iFlags |= GC_EVENT_QUIT;
			if (IsDlgButtonChecked(hwndDlg, IDC_10) == BST_CHECKED)
				iFlags |= GC_EVENT_KICK;
			if (IsDlgButtonChecked(hwndDlg, IDC_11) == BST_CHECKED)
				iFlags |= GC_EVENT_NOTICE;

			if (iFlags&GC_EVENT_ADDSTATUS)
				iFlags |= GC_EVENT_REMOVESTATUS;

			SendMessage(GetParent(hwndDlg), GC_CHANGEFILTERFLAG, 0, (LPARAM)iFlags);
			if (si->bFilterEnabled)
				SendMessage(GetParent(hwndDlg), GC_REDRAWLOG, 0, 0);
			PostMessage(hwndDlg, WM_CLOSE, 0, 0);
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		break;
	}

	return(FALSE);
}

static LRESULT CALLBACK ButtonSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_RBUTTONUP:
		if (db_get_b(NULL, CHAT_MODULE, "RightClickFilter", 0) != 0) {
			if (GetDlgItem(GetParent(hwnd), IDC_FILTER) == hwnd)
				SendMessage(GetParent(hwnd), GC_SHOWFILTERMENU, 0, 0);
			if (GetDlgItem(GetParent(hwnd), IDC_COLOR) == hwnd)
				SendMessage(GetParent(hwnd), GC_SHOWCOLORCHOOSER, 0, (LPARAM)IDC_COLOR);
			if (GetDlgItem(GetParent(hwnd), IDC_BKGCOLOR) == hwnd)
				SendMessage(GetParent(hwnd), GC_SHOWCOLORCHOOSER, 0, (LPARAM)IDC_BKGCOLOR);
		}
		break;
	}

	return mir_callNextSubclass(hwnd, ButtonSubclassProc, msg, wParam, lParam);
}

static LRESULT CALLBACK LogSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_LBUTTONUP:
		{
			CHARRANGE sel;
			SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM)&sel);
			if (sel.cpMin != sel.cpMax) {
				SendMessage(hwnd, WM_COPY, 0, 0);
				sel.cpMin = sel.cpMax;
				SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM)&sel);
			}
			SetFocus(GetDlgItem(GetParent(hwnd), IDC_MESSAGE));
		}
		break;
	case WM_KEYDOWN:
		if (wParam == 0x57 && GetKeyState(VK_CONTROL) & 0x8000) { // ctrl-w (close window)
			PostMessage(GetParent(hwnd), WM_CLOSE, 0, 0);
			return TRUE;
		}
		break;

	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE) {
			CHARRANGE sel;
			SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM)&sel);
			if (sel.cpMin != sel.cpMax) {
				sel.cpMin = sel.cpMax;
				SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM)&sel);
			}
		}
		break;

	case WM_CHAR:
		SetFocus(GetDlgItem(GetParent(hwnd), IDC_MESSAGE));
		SendDlgItemMessage(GetParent(hwnd), IDC_MESSAGE, WM_CHAR, wParam, lParam);
		break;
	}

	return mir_callNextSubclass(hwnd, LogSubclassProc, msg, wParam, lParam);
}

static LRESULT CALLBACK TabSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static BOOL bDragging = FALSE;
	static int iBeginIndex = 0;
	switch (msg) {
	case WM_LBUTTONDOWN:
		{
			TCHITTESTINFO tci = { 0 };
			tci.pt.x = (short)LOWORD(GetMessagePos());
			tci.pt.y = (short)HIWORD(GetMessagePos());
			if (DragDetect(hwnd, tci.pt) && TabCtrl_GetItemCount(hwnd) > 1) {
				int i;
				tci.flags = TCHT_ONITEM;

				ScreenToClient(hwnd, &tci.pt);
				i = TabCtrl_HitTest(hwnd, &tci);
				if (i != -1) {
					TCITEM tc;
					SESSION_INFO *s = NULL;

					tc.mask = TCIF_PARAM;
					TabCtrl_GetItem(hwnd, i, &tc);
					s = (SESSION_INFO*)tc.lParam;
					if (s) {
						BOOL bOnline = db_get_w(s->hContact, s->pszModule, "Status", ID_STATUS_OFFLINE) == ID_STATUS_ONLINE ? TRUE : FALSE;
						MODULEINFO *mi = pci->MM_FindModule(s->pszModule);
						bDragging = TRUE;
						iBeginIndex = i;
						ImageList_BeginDrag(hIconsList, bOnline ? mi->OnlineIconIndex : mi->OfflineIconIndex, 8, 8);
						ImageList_DragEnter(hwnd, tci.pt.x, tci.pt.y);
						SetCapture(hwnd);
					}
					return TRUE;
				}
			}
			else PostMessage(GetParent(hwnd), GC_TABCLICKED, 0, 0);
		}
		break;

	case WM_CAPTURECHANGED:
		bDragging = FALSE;
		ImageList_DragLeave(hwnd);
		ImageList_EndDrag();
		break;

	case WM_MOUSEMOVE:
		if (bDragging) {
			TCHITTESTINFO tci = { 0 };
			tci.pt.x = (short)LOWORD(GetMessagePos());
			tci.pt.y = (short)HIWORD(GetMessagePos());
			ScreenToClient(hwnd, &tci.pt);
			ImageList_DragMove(tci.pt.x, tci.pt.y);
		}
		break;

	case WM_LBUTTONUP:
		if (bDragging && ReleaseCapture()) {
			TCHITTESTINFO tci = { 0 };
			tci.pt.x = (short)LOWORD(GetMessagePos());
			tci.pt.y = (short)HIWORD(GetMessagePos());
			tci.flags = TCHT_ONITEM;
			bDragging = FALSE;
			ImageList_DragLeave(hwnd);
			ImageList_EndDrag();

			ScreenToClient(hwnd, &tci.pt);
			int i = TabCtrl_HitTest(hwnd, &tci);
			if (i != -1 && i != iBeginIndex)
				SendMessage(GetParent(hwnd), GC_DROPPEDTAB, (WPARAM)i, (LPARAM)iBeginIndex);
		}
		break;

	case WM_LBUTTONDBLCLK:
		{
			TCHITTESTINFO tci = { 0 };
			tci.pt.x = (short)LOWORD(GetMessagePos());
			tci.pt.y = (short)HIWORD(GetMessagePos());
			tci.flags = TCHT_ONITEM;

			ScreenToClient(hwnd, &tci.pt);
			int i = TabCtrl_HitTest(hwnd, &tci);
			if (i != -1 && g_Settings.TabCloseOnDblClick)
				PostMessage(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(IDC_CLOSE, BN_CLICKED), 0);
		}
		break;

	case WM_MBUTTONUP:
		TCHITTESTINFO tci = { 0 };
		tci.pt.x = (short)LOWORD(GetMessagePos());
		tci.pt.y = (short)HIWORD(GetMessagePos());
		tci.flags = TCHT_ONITEM;

		ScreenToClient(hwnd, &tci.pt);
		int i = TabCtrl_HitTest(hwnd, &tci);
		if (i != -1) {
			TCITEM tc;
			tc.mask = TCIF_PARAM;
			TabCtrl_GetItem(hwnd, i, &tc);
			SESSION_INFO *si = (SESSION_INFO*)tc.lParam;
			if (si)
				SendMessage(GetParent(hwnd), GC_REMOVETAB, 1, (LPARAM)si);
		}
		break;
	}

	return mir_callNextSubclass(hwnd, TabSubclassProc, msg, wParam, lParam);
}

static LRESULT CALLBACK NicklistSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	SESSION_INFO *si = (SESSION_INFO*)GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA);

	switch (msg) {
	case WM_ERASEBKGND:
		{
			HDC dc = (HDC)wParam;
			if (dc == NULL)
				return 0;

			int index = SendMessage(hwnd, LB_GETTOPINDEX, 0, 0);
			if (index == LB_ERR || si->nUsersInNicklist <= 0)
				return 0;

			int height = SendMessage(hwnd, LB_GETITEMHEIGHT, 0, 0);
			if (height == LB_ERR)
				return 0;

			RECT rc = { 0 };
			GetClientRect(hwnd, &rc);

			int items = si->nUsersInNicklist - index;
			if (rc.bottom - rc.top > items * height) {
				rc.top = items * height;
				FillRect(dc, &rc, pci->hListBkgBrush);
			}
		}
		return 1;

	case WM_KEYDOWN:
		if (wParam == 0x57 && GetKeyState(VK_CONTROL) & 0x8000) { // ctrl-w (close window)
			PostMessage(GetParent(hwnd), WM_CLOSE, 0, 0);
			return TRUE;
		}
		break;

	case WM_RBUTTONDOWN:
		SendMessage(hwnd, WM_LBUTTONDOWN, wParam, lParam);
		break;

	case WM_RBUTTONUP:
		SendMessage(hwnd, WM_LBUTTONUP, wParam, lParam);
		break;

	case WM_MEASUREITEM:
		{
			MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lParam;
			if (mis->CtlType == ODT_MENU)
				return Menu_MeasureItem(lParam);
		}
		return FALSE;

	case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
			if (dis->CtlType == ODT_MENU)
				return Menu_DrawItem(lParam);
		}
		return FALSE;

	case WM_CONTEXTMENU:
		TVHITTESTINFO hti;
		{
			int height = 0;
			hti.pt.x = GET_X_LPARAM(lParam);
			hti.pt.y = GET_Y_LPARAM(lParam);
			if (hti.pt.x == -1 && hti.pt.y == -1) {
				int index = SendMessage(hwnd, LB_GETCURSEL, 0, 0);
				int top = SendMessage(hwnd, LB_GETTOPINDEX, 0, 0);
				height = SendMessage(hwnd, LB_GETITEMHEIGHT, 0, 0);
				hti.pt.x = 4;
				hti.pt.y = (index - top)*height + 1;
			}
			else ScreenToClient(hwnd, &hti.pt);

			int item = LOWORD(SendDlgItemMessage(GetParent(hwnd), IDC_LIST, LB_ITEMFROMPOINT, 0, MAKELPARAM(hti.pt.x, hti.pt.y)));
			USERINFO *ui = pci->SM_GetUserFromIndex(si->ptszID, si->pszModule, item);
			if (ui) {
				USERINFO uinew;
				memcpy(&uinew, ui, sizeof(USERINFO));
				if (hti.pt.x == -1 && hti.pt.y == -1)
					hti.pt.y += height - 4;
				ClientToScreen(hwnd, &hti.pt);

				HMENU hMenu = 0;
				UINT uID = CreateGCMenu(hwnd, &hMenu, 0, hti.pt, si, uinew.pszUID, uinew.pszNick);
				switch (uID) {
				case 0:
					break;

				case ID_MESS:
					pci->DoEventHookAsync(GetParent(hwnd), si->ptszID, si->pszModule, GC_USER_PRIVMESS, ui->pszUID, NULL, 0);
					break;

				default:
					pci->DoEventHookAsync(GetParent(hwnd), si->ptszID, si->pszModule, GC_USER_NICKLISTMENU, ui->pszUID, NULL, (LPARAM)uID);
					break;
				}
				DestroyGCMenu(&hMenu, 1);
				return TRUE;
			}
		}
		break;

	case WM_MOUSEMOVE:
		Chat_HoverMouse(si, hwnd, lParam, ServiceExists("mToolTip/HideTip"));
		break;
	}

	return mir_callNextSubclass(hwnd, NicklistSubclassProc, msg, wParam, lParam);
}

static int RestoreWindowPosition(HWND hwnd, MCONTACT hContact, char * szModule, char * szNamePrefix, UINT showCmd)
{
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	GetWindowPlacement(hwnd, &wp);

	char szSettingName[64];
	mir_snprintf(szSettingName, "%sx", szNamePrefix);
	int x = db_get_dw(hContact, szModule, szSettingName, -1);
	mir_snprintf(szSettingName, "%sy", szNamePrefix);
	int y = (int)db_get_dw(hContact, szModule, szSettingName, -1);
	mir_snprintf(szSettingName, "%swidth", szNamePrefix);
	int width = db_get_dw(hContact, szModule, szSettingName, -1);
	mir_snprintf(szSettingName, "%sheight", szNamePrefix);
	int height = db_get_dw(hContact, szModule, szSettingName, -1);

	if (x == -1)
		return 0;

	wp.rcNormalPosition.left = x;
	wp.rcNormalPosition.top = y;
	wp.rcNormalPosition.right = wp.rcNormalPosition.left + width;
	wp.rcNormalPosition.bottom = wp.rcNormalPosition.top + height;
	wp.showCmd = showCmd;
	SetWindowPlacement(hwnd, &wp);
	return 1;
}

int GetTextPixelSize(wchar_t* pszText, HFONT hFont, BOOL bWidth)
{
	if (!pszText || !hFont)
		return 0;

	HDC hdc = GetDC(NULL);
	HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
	RECT rc = { 0 };
	DrawText(hdc, pszText, -1, &rc, DT_CALCRECT);
	SelectObject(hdc, hOldFont);
	ReleaseDC(NULL, hdc);
	return bWidth ? rc.right - rc.left : rc.bottom - rc.top;
}

static void __cdecl phase2(void * lParam)
{
	SESSION_INFO *si = (SESSION_INFO*)lParam;
	Sleep(30);
	if (si && si->hWnd)
		PostMessage(si->hWnd, GC_REDRAWLOG3, 0, 0);
}

static void SetButtonsPos(HWND hwndDlg)
{
	HDWP hdwp = BeginDeferWindowPos(Srmm_GetButtonCount());

	RECT rc;
	GetWindowRect(GetDlgItem(hwndDlg, IDC_SPLITTERY), &rc);
	POINT pt = { 0, rc.bottom };
	ScreenToClient(hwndDlg, &pt);
	pt.y -= 2;

	GetClientRect(hwndDlg, &rc);
	int iLeftX = 2, iRightX = rc.right - 2;
	int iGap = db_get_b(NULL, SRMSGMOD, "ButtonsBarGap", 1);

	for (int i = 0;; i++) {
		CustomButtonData *cbd = Srmm_GetNthButton(i);
		if (cbd == NULL || cbd->m_bRSided)
			break;
		
		HWND hwndButton = GetDlgItem(hwndDlg, cbd->m_dwButtonCID);
		if (hwndButton == NULL)
			continue;

		int width = iGap + ((cbd->m_dwArrowCID) ? 34 : 22);
		hdwp = DeferWindowPos(hdwp, hwndButton, NULL, iLeftX, pt.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		iLeftX += width;
	}

	for (int i = Srmm_GetButtonCount() - 1; i >= 0; i--) {
		CustomButtonData *cbd = Srmm_GetNthButton(i);
		if (!cbd->m_bRSided)
			break;

		HWND hwndButton = GetDlgItem(hwndDlg, cbd->m_dwButtonCID);
		if (hwndButton == NULL)
			continue;

		int width = iGap + ((cbd->m_dwArrowCID) ? 34 : 22);
		iRightX -= width;
		hdwp = DeferWindowPos(hdwp, hwndButton, NULL, iRightX, pt.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	}

	EndDeferWindowPos(hdwp);
}

INT_PTR CALLBACK RoomWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SESSION_INFO *s, *si = (SESSION_INFO*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	RECT rc;

	switch (uMsg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		{
			HWND hNickList = GetDlgItem(hwndDlg, IDC_LIST);
			si = (SESSION_INFO*)lParam;
			TranslateDialogDefault(hwndDlg);
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)si);

			CustomButtonData *cbd;
			for (int i = 0; cbd = Srmm_GetNthButton(i); i++) {
				if (!cbd->m_bChatButton)
					continue;

				int width = (cbd->m_dwArrowCID) ? 34 : 22;
				HWND hwndButton = CreateWindowEx(0, L"MButtonClass", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, width, 22, hwndDlg, (HMENU)cbd->m_dwButtonCID, g_hInst, NULL);
				SendMessage(hwndButton, BUTTONSETASFLATBTN, TRUE, 0);
				if (cbd->m_pwszTooltip)
					SendMessage(hwndButton, BUTTONADDTOOLTIP, LPARAM(cbd->m_pwszTooltip), BATF_UNICODE);
				if (cbd->m_hIcon)
					SendMessage(hwndButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM)IcoLib_GetIconByHandle(cbd->m_hIcon));
			}

			mir_subclassWindow(GetDlgItem(hwndDlg, IDC_SPLITTERX), SplitterSubclassProc);
			mir_subclassWindow(GetDlgItem(hwndDlg, IDC_SPLITTERY), SplitterSubclassProc);
			mir_subclassWindow(hNickList, NicklistSubclassProc);
			mir_subclassWindow(GetDlgItem(hwndDlg, IDC_TAB), TabSubclassProc);
			mir_subclassWindow(GetDlgItem(hwndDlg, IDC_LOG), LogSubclassProc);
			mir_subclassWindow(GetDlgItem(hwndDlg, IDC_FILTER), ButtonSubclassProc);
			mir_subclassWindow(GetDlgItem(hwndDlg, IDC_COLOR), ButtonSubclassProc);
			mir_subclassWindow(GetDlgItem(hwndDlg, IDC_BKGCOLOR), ButtonSubclassProc);
			mir_subclassWindow(GetDlgItem(hwndDlg, IDC_MESSAGE), MessageSubclassProc);
			SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SUBCLASSED, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_AUTOURLDETECT, 1, 0);
			int mask = (int)SendDlgItemMessage(hwndDlg, IDC_LOG, EM_GETEVENTMASK, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETEVENTMASK, 0, mask | ENM_LINK | ENM_MOUSEEVENTS);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_LIMITTEXT, (WPARAM)sizeof(wchar_t) * 0x7FFFFFFF, 0);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETOLECALLBACK, 0, (LPARAM)& reOleCallback);

			si->hwndStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP | SBT_TOOLTIPS, 0, 0, 0, 0, hwndDlg, NULL, g_hInst, NULL);
			SendMessage(si->hwndStatus, SB_SETMINHEIGHT, GetSystemMetrics(SM_CYSMICON), 0);
			TabCtrl_SetMinTabWidth(GetDlgItem(hwndDlg, IDC_TAB), 80);
			TabCtrl_SetImageList(GetDlgItem(hwndDlg, IDC_TAB), hIconsList);

			// restore previous tabs
			if (g_Settings.bTabsEnable && g_Settings.TabRestore) {
				TABLIST *node = g_TabList;
				while (node) {
					SESSION_INFO *s = pci->SM_FindSession(node->pszID, node->pszModule);
					if (s)
						SendMessage(hwndDlg, GC_ADDTAB, -1, (LPARAM)s);

					node = node->next;
				}
			}

			TabM_RemoveAll();

			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_HIDESELECTION, TRUE, 0);

			SendMessage(hwndDlg, GC_SETWNDPROPS, 0, 0);
			SendMessage(hwndDlg, GC_UPDATESTATUSBAR, 0, 0);
			SendMessage(hwndDlg, GC_UPDATETITLE, 0, 0);
			SendMessage(hwndDlg, GC_SETWINDOWPOS, 0, 0);
		}
		break;

	case GC_SETWNDPROPS:
		{
			InitButtons(hwndDlg, si);

			// stupid hack to make icons show. I dunno why this is needed currently
			MODULEINFO *mi = pci->MM_FindModule(si->pszModule);
			HICON hIcon = si->wStatus == ID_STATUS_ONLINE ? mi->hOnlineIcon : mi->hOfflineIcon;
			if (!hIcon) {
				pci->MM_IconsChanged();
				hIcon = (si->wStatus == ID_STATUS_ONLINE) ? mi->hOnlineIcon : mi->hOfflineIcon;
			}

			SendMessage(hwndDlg, GC_FIXTABICONS, 0, 0);
			SendMessage(si->hwndStatus, SB_SETICON, 0, (LPARAM)hIcon);
			Window_SetIcon_IcoLib(hwndDlg, GetIconHandle("window"));

			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETBKGNDCOLOR, 0, g_Settings.crLogBackground);

			if (g_Settings.bTabsEnable) {
				LONG_PTR mask = GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_TAB), GWL_STYLE);
				if (g_Settings.TabsAtBottom)
					mask |= TCS_BOTTOM;
				else
					mask &= ~TCS_BOTTOM;
				SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_TAB), GWL_STYLE, (LONG_PTR)mask);
			}
			{
				CHARFORMAT2 cf;
				cf.cbSize = sizeof(CHARFORMAT2);
				cf.dwMask = CFM_COLOR | CFM_BOLD | CFM_UNDERLINE | CFM_BACKCOLOR;
				cf.dwEffects = 0;
				cf.crTextColor = g_Settings.MessageAreaColor;
				cf.crBackColor = (COLORREF)db_get_dw(NULL, CHAT_MODULE, "ColorMessageBG", GetSysColor(COLOR_WINDOW));
				SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETBKGNDCOLOR, 0, db_get_dw(NULL, CHAT_MODULE, "ColorMessageBG", GetSysColor(COLOR_WINDOW)));
				SendDlgItemMessage(hwndDlg, IDC_MESSAGE, WM_SETFONT, (WPARAM)g_Settings.MessageAreaFont, MAKELPARAM(TRUE, 0));
				SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, (WPARAM)SCF_ALL, (LPARAM)&cf);

				// nicklist
				int ih = GetTextPixelSize(L"AQGglo", g_Settings.UserListFont, FALSE);
				int ih2 = GetTextPixelSize(L"AQGglo", g_Settings.UserListHeadingsFont, FALSE);
				int height = db_get_b(NULL, CHAT_MODULE, "NicklistRowDist", 12);
				int font = ih > ih2 ? ih : ih2;

				// make sure we have space for icon!
				if (g_Settings.bShowContactStatus)
					font = font > 16 ? font : 16;

				SendDlgItemMessage(hwndDlg, IDC_LIST, LB_SETITEMHEIGHT, 0, (LPARAM)height > font ? height : font);
				InvalidateRect(GetDlgItem(hwndDlg, IDC_LIST), NULL, TRUE);
			}
			SendMessage(hwndDlg, WM_SIZE, 0, 0);
			SendMessage(hwndDlg, GC_REDRAWLOG2, 0, 0);
		}
		break;

	case GC_UPDATETITLE:
		{
			wchar_t szTemp[100];
			switch (si->iType) {
			case GCW_CHATROOM:
				mir_snwprintf(szTemp,
					(si->nUsersInNicklist == 1) ? TranslateT("%s: chat room (%u user)") : TranslateT("%s: chat room (%u users)"),
					si->ptszName, si->nUsersInNicklist);
				break;
			case GCW_PRIVMESS:
				mir_snwprintf(szTemp,
					(si->nUsersInNicklist == 1) ? TranslateT("%s: message session") : TranslateT("%s: message session (%u users)"),
					si->ptszName, si->nUsersInNicklist);
				break;
			case GCW_SERVER:
				mir_snwprintf(szTemp, L"%s: Server", si->ptszName);
				break;
			}
			SetWindowText(hwndDlg, szTemp);
		}
		break;

	case GC_UPDATESTATUSBAR:
		{
			MODULEINFO *mi = pci->MM_FindModule(si->pszModule);
			wchar_t *ptszDispName = mi->ptszModDispName;
			int x = 12;
			x += GetTextPixelSize(ptszDispName, (HFONT)SendMessage(si->hwndStatus, WM_GETFONT, 0, 0), TRUE);
			x += GetSystemMetrics(SM_CXSMICON);
			int iStatusbarParts[2] = { x, -1 };
			SendMessage(si->hwndStatus, SB_SETPARTS, 2, (LPARAM)&iStatusbarParts);

			// stupid hack to make icons show. I dunno why this is needed currently
			HICON hIcon = si->wStatus == ID_STATUS_ONLINE ? mi->hOnlineIcon : mi->hOfflineIcon;
			if (!hIcon) {
				pci->MM_IconsChanged();
				hIcon = si->wStatus == ID_STATUS_ONLINE ? mi->hOnlineIcon : mi->hOfflineIcon;
			}

			SendMessage(si->hwndStatus, SB_SETICON, 0, (LPARAM)hIcon);
			SendMessage(hwndDlg, GC_FIXTABICONS, 0, 0);

			SendMessage(si->hwndStatus, SB_SETTEXT, 0, (LPARAM)ptszDispName);

			SendMessage(si->hwndStatus, SB_SETTEXT, 1, (LPARAM)(si->ptszStatusbarText ? si->ptszStatusbarText : L""));
			SendMessage(si->hwndStatus, SB_SETTIPTEXT, 1, (LPARAM)(si->ptszStatusbarText ? si->ptszStatusbarText : L""));
		}
		return TRUE;

	case GC_SETWINDOWPOS:
		{
			SESSION_INFO *pActive = pci->GetActiveSession();
			int savePerContact = db_get_b(NULL, CHAT_MODULE, "SavePosition", 0);

			WINDOWPLACEMENT wp;
			wp.length = sizeof(wp);
			GetWindowPlacement(hwndDlg, &wp);

			RECT screen;
			SystemParametersInfo(SPI_GETWORKAREA, 0, &screen, 0);

			if (si->iX) {
				wp.rcNormalPosition.left = si->iX;
				wp.rcNormalPosition.top = si->iY;
				wp.rcNormalPosition.right = wp.rcNormalPosition.left + si->iWidth;
				wp.rcNormalPosition.bottom = wp.rcNormalPosition.top + si->iHeight;
				wp.showCmd = SW_HIDE;
				SetWindowPlacement(hwndDlg, &wp);
				break;
			}
			if (savePerContact) {
				if (RestoreWindowPosition(hwndDlg, g_Settings.bTabsEnable ? NULL : si->hContact, CHAT_MODULE, "room", SW_HIDE))
					break;
				SetWindowPos(hwndDlg, 0, (screen.right - screen.left) / 2 - (550) / 2, (screen.bottom - screen.top) / 2 - (400) / 2, (550), (400), SWP_NOZORDER | SWP_HIDEWINDOW | SWP_NOACTIVATE);
			}
			else SetWindowPos(hwndDlg, 0, (screen.right - screen.left) / 2 - (550) / 2, (screen.bottom - screen.top) / 2 - (400) / 2, (550), (400), SWP_NOZORDER | SWP_HIDEWINDOW | SWP_NOACTIVATE);

			if (!g_Settings.bTabsEnable && pActive && pActive->hWnd && db_get_b(NULL, CHAT_MODULE, "CascadeWindows", 1)) {
				RECT rcThis, rcNew;
				int dwFlag = SWP_NOZORDER | SWP_NOACTIVATE;
				if (!IsWindowVisible((HWND)wParam))
					dwFlag |= SWP_HIDEWINDOW;

				GetWindowRect(hwndDlg, &rcThis);
				GetWindowRect(pActive->hWnd, &rcNew);

				int offset = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME);
				SetWindowPos((HWND)hwndDlg, 0, rcNew.left + offset, rcNew.top + offset, rcNew.right - rcNew.left, rcNew.bottom - rcNew.top, dwFlag);
			}
		}
		break;

	case GC_SAVEWNDPOS:
		{
			WINDOWPLACEMENT wp = { 0 };
			wp.length = sizeof(wp);
			GetWindowPlacement(hwndDlg, &wp);

			g_Settings.iX = wp.rcNormalPosition.left;
			g_Settings.iY = wp.rcNormalPosition.top;
			g_Settings.iWidth = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
			g_Settings.iHeight = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;

			if (!lParam) {
				si->iX = g_Settings.iX;
				si->iY = g_Settings.iY;
				si->iWidth = g_Settings.iWidth;
				si->iHeight = g_Settings.iHeight;
			}
		}
		break;

	case WM_SIZE:
		if (wParam == SIZE_MAXIMIZED)
			PostMessage(hwndDlg, GC_SCROLLTOBOTTOM, 0, 0);

		if (!IsIconic(hwndDlg)) {
			SendMessage(si->hwndStatus, WM_SIZE, 0, 0);

			Utils_ResizeDialog(hwndDlg, g_hInst, MAKEINTRESOURCEA(IDD_CHANNEL), RoomWndResize, (LPARAM)si);
			SetButtonsPos(hwndDlg);

			InvalidateRect(si->hwndStatus, NULL, TRUE);
			RedrawWindow(GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, NULL, RDW_INVALIDATE);
			RedrawWindow(GetDlgItem(hwndDlg, IDOK), NULL, NULL, RDW_INVALIDATE);
			SendMessage(hwndDlg, GC_SAVEWNDPOS, 0, 1);
		}
		break;

	case GC_REDRAWWINDOW:
		InvalidateRect(hwndDlg, NULL, TRUE);
		break;

	case GC_REDRAWLOG:
		si->LastTime = 0;
		if (si->pLog) {
			LOGINFO * pLog = si->pLog;
			if (si->iEventCount > 60) {
				int index = 0;
				while (index < 59) {
					if (pLog->next == NULL)
						break;

					pLog = pLog->next;
					if (si->iType != GCW_CHATROOM || !si->bFilterEnabled || (si->iLogFilterFlags&pLog->iType) != 0)
						index++;
				}
				Log_StreamInEvent(hwndDlg, pLog, si, TRUE, FALSE);
				mir_forkthread(phase2, si);
			}
			else Log_StreamInEvent(hwndDlg, si->pLogEnd, si, TRUE, FALSE);
		}
		else SendMessage(hwndDlg, GC_CONTROL_MSG, WINDOW_CLEARLOG, 0);
		break;

	case GC_REDRAWLOG2:
		si->LastTime = 0;
		if (si->pLog)
			Log_StreamInEvent(hwndDlg, si->pLogEnd, si, TRUE, FALSE);
		break;

	case GC_REDRAWLOG3:
		si->LastTime = 0;
		if (si->pLog)
			Log_StreamInEvent(hwndDlg, si->pLogEnd, si, TRUE, TRUE);
		break;

	case GC_ADDLOG:
		if (si->pLogEnd)
			Log_StreamInEvent(hwndDlg, si->pLog, si, FALSE, FALSE);
		else
			SendMessage(hwndDlg, GC_CONTROL_MSG, WINDOW_CLEARLOG, 0);
		break;

	case GC_SWITCHNEXTTAB:
		{
			int total = TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_TAB));
			int i = TabCtrl_GetCurSel(GetDlgItem(hwndDlg, IDC_TAB));
			if (i != -1 && total != -1 && total != 1) {
				if (i < total - 1)
					i++;
				else
					i = 0;
				TabCtrl_SetCurSel(GetDlgItem(hwndDlg, IDC_TAB), i);
				PostMessage(hwndDlg, GC_TABCLICKED, 0, 0);
			}
		}
		break;

	case GC_SWITCHPREVTAB:
		{
			int total = TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_TAB));
			int i = TabCtrl_GetCurSel(GetDlgItem(hwndDlg, IDC_TAB));
			if (i != -1 && total != -1 && total != 1) {
				if (i > 0)
					i--;
				else
					i = total - 1;
				TabCtrl_SetCurSel(GetDlgItem(hwndDlg, IDC_TAB), i);
				PostMessage(hwndDlg, GC_TABCLICKED, 0, 0);
			}
		}
		break;

	case GC_SWITCHTAB:
		{
			int total = TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_TAB));
			int i = TabCtrl_GetCurSel(GetDlgItem(hwndDlg, IDC_TAB));
			if (i != -1 && total != -1 && total != 1 && i != lParam && total > lParam) {
				TabCtrl_SetCurSel(GetDlgItem(hwndDlg, IDC_TAB), lParam);
				PostMessage(hwndDlg, GC_TABCLICKED, 0, 0);
			}
		}
		break;

	case GC_REMOVETAB:
		{
			int i = -1;
			SESSION_INFO* s1 = (SESSION_INFO*)lParam;
			int tabId = TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_TAB));
			if (s1) {
				if (tabId) {
					for (i = 0; i < tabId; i++) {
						TCITEM tci = { 0 };
						tci.mask = TCIF_PARAM;
						int ii = TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_TAB), i, &tci);
						if (ii != -1) {
							SESSION_INFO *s2 = (SESSION_INFO*)tci.lParam;
							if (s1 == s2)
								goto END_REMOVETAB;
						}
					}
				}
			}
			else i = TabCtrl_GetCurSel(GetDlgItem(hwndDlg, IDC_TAB));

		END_REMOVETAB:
			if (i != -1 && i < tabId) {
				TCITEM id = { 0 };
				TabCtrl_DeleteItem(GetDlgItem(hwndDlg, IDC_TAB), i);
				id.mask = TCIF_PARAM;
				if (!TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_TAB), i, &id)) {
					if (!TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_TAB), i - 1, &id)) {
						SendMessage(hwndDlg, WM_CLOSE, 0, 0);
						break;
					}
				}

				SESSION_INFO *s = (SESSION_INFO*)id.lParam;
				if (s)
					pci->ShowRoom(s, (WPARAM)WINDOW_VISIBLE, wParam == 1 ? FALSE : TRUE);
			}
		}
		break;

	case GC_ADDTAB:
		{
			int indexfound = -1;
			int lastlocked = -1;
			BOOL bFound = FALSE;
			SESSION_INFO* s1 = (SESSION_INFO*)lParam;

			TCITEM tci;
			tci.mask = TCIF_PARAM;
			int tabId = TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_TAB));

			// does the tab already exist?
			for (int i = 0; i < tabId; i++) {
				TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_TAB), i, &tci);
				SESSION_INFO *s2 = (SESSION_INFO*)tci.lParam;
				if (s2) {
					if (s1 == s2 && !bFound) {
						if (!bFound) {
							bFound = TRUE;
							indexfound = i;
						}
					}

					int w = db_get_w(s2->hContact, s2->pszModule, "TabPosition", 0);
					if (w)
						lastlocked = w;
				}
			}

			int w = 0;

			if (!bFound) { // create a new tab
				int insertat;
				wchar_t szTemp[30];

				mir_wstrncpy(szTemp, s1->ptszName, 21);
				if (mir_wstrlen(s1->ptszName) > 20)
					mir_wstrncpy(szTemp + 20, L"...", 4);

				tci.mask = TCIF_TEXT | TCIF_PARAM;
				tci.pszText = szTemp;
				tci.lParam = lParam;

				// determine insert position
				w = db_get_w(s1->hContact, s1->pszModule, "TabPosition", 0);
				if (wParam == -1)
					insertat = w == 0 ? tabId : (int)w - 1;
				else
					insertat = (int)wParam;

				w = TabCtrl_InsertItem(GetDlgItem(hwndDlg, IDC_TAB), insertat, &tci);
				SendMessage(hwndDlg, GC_FIXTABICONS, 0, (LPARAM)s1);
			}

			if (wParam == -1) {
				if (bFound)
					TabCtrl_SetCurSel(GetDlgItem(hwndDlg, IDC_TAB), indexfound);
				else
					TabCtrl_SetCurSel(GetDlgItem(hwndDlg, IDC_TAB), w);
			}
		}
		break;

	case GC_FIXTABICONS:
		if (s = (SESSION_INFO*)lParam) {
			int tabId = TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_TAB));
			for (int i = 0; i < tabId; i++) {
				TCITEM tci;
				tci.mask = TCIF_PARAM | TCIF_IMAGE;
				TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_TAB), i, &tci);
				SESSION_INFO *s2 = (SESSION_INFO*)tci.lParam;
				if (s2 && s == s2) {
					int image = 0;
					if (!(s2->wState & GC_EVENT_HIGHLIGHT)) {
						MODULEINFO *mi = pci->MM_FindModule(s2->pszModule);
						image = (s2->wStatus == ID_STATUS_ONLINE) ? mi->OnlineIconIndex : mi->OfflineIconIndex;
						if (s2->wState & STATE_TALK)
							image++;
					}

					if (tci.iImage != image) {
						tci.mask = TCIF_IMAGE;
						tci.iImage = image;
						TabCtrl_SetItem(GetDlgItem(hwndDlg, IDC_TAB), i, &tci);
					}
				}
			}
		}
		else RedrawWindow(GetDlgItem(hwndDlg, IDC_TAB), NULL, NULL, RDW_INVALIDATE);
		break;

	case GC_SETMESSAGEHIGHLIGHT:
		if (s = (SESSION_INFO*)lParam) {
			int tabId = TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_TAB));
			for (int i = 0; i < tabId; i++) {
				TCITEM tci;
				tci.mask = TCIF_PARAM;
				TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_TAB), i, &tci);
				SESSION_INFO *s2 = (SESSION_INFO*)tci.lParam;
				if (s2 && s == s2) { // highlight
					s2->wState |= GC_EVENT_HIGHLIGHT;
					if (pci->SM_FindSession(si->ptszID, si->pszModule) == s2)
						si->wState = s2->wState;
					SendMessage(hwndDlg, GC_FIXTABICONS, 0, (LPARAM)s2);
					if (g_Settings.bFlashWindowHighlight && GetActiveWindow() != hwndDlg && GetForegroundWindow() != hwndDlg)
						SetTimer(hwndDlg, TIMERID_FLASHWND, 900, NULL);
					break;
				}
			}
		}
		else RedrawWindow(GetDlgItem(hwndDlg, IDC_TAB), NULL, NULL, RDW_INVALIDATE);
		break;

	case GC_SETTABHIGHLIGHT:
		if (s = (SESSION_INFO*)lParam) {
			int tabId = TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_TAB));
			for (int i = 0; i < tabId; i++) {
				TCITEM tci;
				tci.mask = TCIF_PARAM;
				TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_TAB), i, &tci);
				SESSION_INFO *s2 = (SESSION_INFO*)tci.lParam;
				if (s2 && s == s2) { // highlight
					SendMessage(hwndDlg, GC_FIXTABICONS, 0, (LPARAM)s2);
					if (g_Settings.bFlashWindow && GetActiveWindow() != hwndDlg && GetForegroundWindow() != hwndDlg)
						SetTimer(hwndDlg, TIMERID_FLASHWND, 900, NULL);
					break;
				}
			}
		}
		else RedrawWindow(GetDlgItem(hwndDlg, IDC_TAB), NULL, NULL, RDW_INVALIDATE);
		break;

	case GC_TABCHANGE:
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
		PostMessage(hwndDlg, GC_SCROLLTOBOTTOM, 0, 0);
		break;

	case GC_TABCLICKED:
		{
			int i = TabCtrl_GetCurSel(GetDlgItem(hwndDlg, IDC_TAB));
			if (i != -1) {
				TCITEM id = { 0 };
				id.mask = TCIF_PARAM;
				TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_TAB), i, &id);
				SESSION_INFO *s = (SESSION_INFO*)id.lParam;
				if (s) {
					if (s->wState & STATE_TALK) {
						s->wState &= ~STATE_TALK;
						db_set_w(s->hContact, s->pszModule, "ApparentMode", (LPARAM)0);
					}

					if (s->wState & GC_EVENT_HIGHLIGHT) {
						s->wState &= ~GC_EVENT_HIGHLIGHT;

						if (pcli->pfnGetEvent(s->hContact, 0))
							pcli->pfnRemoveEvent(s->hContact, GC_FAKE_EVENT);
					}

					SendMessage(hwndDlg, GC_FIXTABICONS, 0, (LPARAM)s);
					if (!s->hWnd) {
						pci->ShowRoom(s, (WPARAM)WINDOW_VISIBLE, TRUE);
						SendMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0);
					}
				}
			}
		}
		break;

	case GC_DROPPEDTAB:
		{
			int begin = (int)lParam;
			int end = (int)wParam;
			if (begin == end)
				break;

			TCITEM tci;
			tci.mask = TCIF_PARAM;
			TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_TAB), begin, &tci);
			s = (SESSION_INFO*)tci.lParam;
			if (s) {
				TabCtrl_DeleteItem(GetDlgItem(hwndDlg, IDC_TAB), begin);

				SendMessage(hwndDlg, GC_ADDTAB, end, (LPARAM)s);

				// fix the "fixed" positions
				int tabId = TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_TAB));
				for (int i = 0; i < tabId; i++) {
					TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_TAB), i, &tci);
					s = (SESSION_INFO*)tci.lParam;
					if (s && s->hContact && db_get_w(s->hContact, s->pszModule, "TabPosition", 0) != 0)
						db_set_w(s->hContact, s->pszModule, "TabPosition", (WORD)(i + 1));
				}
			}
		}
		break;

	case GC_SESSIONNAMECHANGE:
		s = (SESSION_INFO*)lParam;
		{
			int tabId = TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_TAB));
			for (int i = 0; i < tabId; i++) {
				TCITEM tci;
				tci.mask = TCIF_PARAM;
				int j = TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_TAB), i, &tci);
				if (j != -1) {
					SESSION_INFO *s2 = (SESSION_INFO*)tci.lParam;
					if (s == s2) {
						tci.mask = TCIF_TEXT;
						tci.pszText = s->ptszName;
						TabCtrl_SetItem(GetDlgItem(hwndDlg, IDC_TAB), i, &tci);
					}
				}
			}
		}
		break;

	case WM_CTLCOLORLISTBOX:
		SetBkColor((HDC)wParam, g_Settings.crUserListBGColor);
		return (INT_PTR)pci->hListBkgBrush;

	case WM_MEASUREITEM:
		{
			MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lParam;
			if (mis->CtlType == ODT_MENU)
				return Menu_MeasureItem(lParam);

			int ih = GetTextPixelSize(L"AQGgl'", g_Settings.UserListFont, FALSE);
			int ih2 = GetTextPixelSize(L"AQGg'", g_Settings.UserListHeadingsFont, FALSE);
			int font = ih > ih2 ? ih : ih2;
			int height = db_get_b(NULL, CHAT_MODULE, "NicklistRowDist", 12);

			// make sure we have space for icon!
			if (g_Settings.bShowContactStatus)
				font = font > 16 ? font : 16;

			mis->itemHeight = height > font ? height : font;
		}
		return TRUE;

	case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
			if (dis->CtlType == ODT_MENU)
				return Menu_DrawItem(lParam);

			if (dis->CtlID == IDC_LIST) {
				int index = dis->itemID;
				USERINFO *ui = pci->SM_GetUserFromIndex(si->ptszID, si->pszModule, index);
				if (ui) {
					int x_offset = 2;

					int height = dis->rcItem.bottom - dis->rcItem.top;
					if (height & 1)
						height++;

					int offset = (height == 10) ? 0 : height / 2 - 4;
					HFONT hFont = (ui->iStatusEx == 0) ? g_Settings.UserListFont : g_Settings.UserListHeadingsFont;
					HFONT hOldFont = (HFONT)SelectObject(dis->hDC, hFont);
					SetBkMode(dis->hDC, TRANSPARENT);

					if (dis->itemAction == ODA_FOCUS && dis->itemState & ODS_SELECTED)
						FillRect(dis->hDC, &dis->rcItem, pci->hListSelectedBkgBrush);
					else //if (dis->itemState & ODS_INACTIVE)
						FillRect(dis->hDC, &dis->rcItem, pci->hListBkgBrush);

					if (g_Settings.bShowContactStatus && g_Settings.bContactStatusFirst && ui->ContactStatus) {
						HICON hIcon = Skin_LoadProtoIcon(si->pszModule, ui->ContactStatus);
						DrawIconEx(dis->hDC, x_offset, dis->rcItem.top + offset - 3, hIcon, 16, 16, 0, NULL, DI_NORMAL);
						x_offset += 18;
					}
					DrawIconEx(dis->hDC, x_offset, dis->rcItem.top + offset, pci->SM_GetStatusIcon(si, ui), 10, 10, 0, NULL, DI_NORMAL);
					x_offset += 12;
					if (g_Settings.bShowContactStatus && !g_Settings.bContactStatusFirst && ui->ContactStatus) {
						HICON hIcon = Skin_LoadProtoIcon(si->pszModule, ui->ContactStatus);
						DrawIconEx(dis->hDC, x_offset, dis->rcItem.top + offset - 3, hIcon, 16, 16, 0, NULL, DI_NORMAL);
						x_offset += 18;
					}

					SetTextColor(dis->hDC, ui->iStatusEx == 0 ? g_Settings.crUserListColor : g_Settings.crUserListHeadingsColor);
					TextOut(dis->hDC, dis->rcItem.left + x_offset, dis->rcItem.top, ui->pszNick, (int)mir_wstrlen(ui->pszNick));
					SelectObject(dis->hDC, hOldFont);
				}
				return TRUE;
			}
		}
		break;

	case GC_UPDATENICKLIST:
		{
			int i = SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETTOPINDEX, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_LIST, LB_SETCOUNT, si->nUsersInNicklist, 0);
			SendDlgItemMessage(hwndDlg, IDC_LIST, LB_SETTOPINDEX, i, 0);
			SendMessage(hwndDlg, GC_UPDATETITLE, 0, 0);
		}
		break;

	case GC_CONTROL_MSG:
		switch (wParam) {
		case SESSION_OFFLINE:
			SendMessage(hwndDlg, GC_UPDATESTATUSBAR, 0, 0);
			SendMessage(si->hWnd, GC_UPDATENICKLIST, 0, 0);
			return TRUE;

		case SESSION_ONLINE:
			SendMessage(hwndDlg, GC_UPDATESTATUSBAR, 0, 0);
			return TRUE;

		case WINDOW_HIDDEN:
			SendMessage(hwndDlg, GC_CLOSEWINDOW, 0, 0);
			return TRUE;

		case WINDOW_CLEARLOG:
			SetDlgItemText(hwndDlg, IDC_LOG, L"");
			return TRUE;

		case SESSION_TERMINATE:
			SendMessage(hwndDlg, GC_SAVEWNDPOS, 0, 0);
			if (db_get_b(NULL, CHAT_MODULE, "SavePosition", 0)) {
				db_set_dw(si->hContact, CHAT_MODULE, "roomx", si->iX);
				db_set_dw(si->hContact, CHAT_MODULE, "roomy", si->iY);
				db_set_dw(si->hContact, CHAT_MODULE, "roomwidth", si->iWidth);
				db_set_dw(si->hContact, CHAT_MODULE, "roomheight", si->iHeight);
			}
			if (pcli->pfnGetEvent(si->hContact, 0))
				pcli->pfnRemoveEvent(si->hContact, GC_FAKE_EVENT);
			si->wState &= ~STATE_TALK;
			db_set_w(si->hContact, si->pszModule, "ApparentMode", 0);
			SendMessage(hwndDlg, GC_CLOSEWINDOW, 0, 0);
			return TRUE;

		case WINDOW_MINIMIZE:
			ShowWindow(hwndDlg, SW_MINIMIZE);
			goto LABEL_SHOWWINDOW;

		case WINDOW_MAXIMIZE:
			ShowWindow(hwndDlg, SW_MAXIMIZE);
			goto LABEL_SHOWWINDOW;

		case SESSION_INITDONE:
			if (db_get_b(NULL, CHAT_MODULE, "PopupOnJoin", 0) != 0)
				return TRUE;
			// fall through
		case WINDOW_VISIBLE:
			if (IsIconic(hwndDlg))
				ShowWindow(hwndDlg, SW_NORMAL);
		LABEL_SHOWWINDOW:
			SendMessage(hwndDlg, WM_SIZE, 0, 0);
			SendMessage(hwndDlg, GC_REDRAWLOG, 0, 0);
			SendMessage(hwndDlg, GC_UPDATENICKLIST, 0, 0);
			SendMessage(hwndDlg, GC_UPDATESTATUSBAR, 0, 0);
			ShowWindow(hwndDlg, SW_SHOW);
			SendMessage(hwndDlg, WM_SIZE, 0, 0);
			SetForegroundWindow(hwndDlg);
			return TRUE;
		}
		break;

	case GC_SPLITTERMOVED:
		{
			POINT pt;
			RECT rcLog;

			static int x = 0;

			GetWindowRect(GetDlgItem(hwndDlg, IDC_LOG), &rcLog);
			if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_SPLITTERX)) {
				int oldSplitterX;
				GetClientRect(hwndDlg, &rc);
				pt.x = wParam; pt.y = 0;
				ScreenToClient(hwndDlg, &pt);

				oldSplitterX = si->iSplitterX;
				si->iSplitterX = rc.right - pt.x + 1;
				if (si->iSplitterX < 35)
					si->iSplitterX = 35;
				if (si->iSplitterX > rc.right - rc.left - 35)
					si->iSplitterX = rc.right - rc.left - 35;
				g_Settings.iSplitterX = si->iSplitterX;
			}
			else if ((HWND)lParam == GetDlgItem(hwndDlg, IDC_SPLITTERY)) {
				BOOL bFormat = IsWindowVisible(GetDlgItem(hwndDlg, IDC_BOLD));
				int oldSplitterY;
				GetClientRect(hwndDlg, &rc);
				pt.x = 0; pt.y = wParam;
				ScreenToClient(hwndDlg, &pt);

				oldSplitterY = si->iSplitterY;
				si->iSplitterY = bFormat ? rc.bottom - pt.y + 1 : rc.bottom - pt.y + 20;
				if (si->iSplitterY < 63)
					si->iSplitterY = 63;
				if (si->iSplitterY > rc.bottom - rc.top - 40)
					si->iSplitterY = rc.bottom - rc.top - 40;
				g_Settings.iSplitterY = si->iSplitterY;
			}
			if (x == 2) {
				PostMessage(hwndDlg, WM_SIZE, 0, 0);
				x = 0;
			}
			else x++;
		}
		break;

	case GC_FIREHOOK:
		if (lParam) {
			GCHOOK *gch = (GCHOOK *)lParam;
			NotifyEventHooks(pci->hSendEvent, 0, (WPARAM)gch);
			if (gch->pDest) {
				mir_free((void*)gch->pDest->ptszID);
				mir_free((void*)gch->pDest->pszModule);
				mir_free(gch->pDest);
			}
			mir_free((void*)gch->ptszText);
			mir_free((void*)gch->ptszUID);
			mir_free(gch);
		}
		break;

	case GC_CHANGEFILTERFLAG:
		si->iLogFilterFlags = lParam;
		break;

	case GC_SHOWFILTERMENU:
		{
			HWND hwnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_FILTER), hwndDlg, FilterWndProc, (LPARAM)si);
			TranslateDialogDefault(hwnd);
			GetWindowRect(GetDlgItem(hwndDlg, IDC_FILTER), &rc);
			SetWindowPos(hwnd, HWND_TOP, rc.left - 85, (IsWindowVisible(GetDlgItem(hwndDlg, IDC_FILTER)) || IsWindowVisible(GetDlgItem(hwndDlg, IDC_BOLD))) ? rc.top - 206 : rc.top - 186, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
		}
		break;

	case GC_SHOWCOLORCHOOSER:
		pci->ColorChooser(si, lParam == IDC_COLOR, hwndDlg, GetDlgItem(hwndDlg, IDC_MESSAGE), GetDlgItem(hwndDlg, lParam));
		break;

	case GC_SCROLLTOBOTTOM:
		if ((GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_LOG), GWL_STYLE) & WS_VSCROLL) != 0) {
			SCROLLINFO scroll = { 0 };
			scroll.cbSize = sizeof(scroll);
			scroll.fMask = SIF_PAGE | SIF_RANGE;
			GetScrollInfo(GetDlgItem(hwndDlg, IDC_LOG), SB_VERT, &scroll);

			scroll.fMask = SIF_POS;
			scroll.nPos = scroll.nMax - scroll.nPage + 1;
			SetScrollInfo(GetDlgItem(hwndDlg, IDC_LOG), SB_VERT, &scroll, TRUE);

			CHARRANGE sel;
			sel.cpMin = sel.cpMax = GetRichTextLength(GetDlgItem(hwndDlg, IDC_LOG));
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXSETSEL, 0, (LPARAM)&sel);
			PostMessage(GetDlgItem(hwndDlg, IDC_LOG), WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
		}
		break;

	case WM_TIMER:
		if (wParam == TIMERID_FLASHWND)
			FlashWindow(hwndDlg, TRUE);
		break;

	case WM_ACTIVATE:
		if (LOWORD(wParam) != WA_ACTIVE)
			break;

		//fall through
	case WM_MOUSEACTIVATE:
		{
			WINDOWPLACEMENT wp = { 0 };
			wp.length = sizeof(wp);
			GetWindowPlacement(hwndDlg, &wp);
			g_Settings.iX = wp.rcNormalPosition.left;
			g_Settings.iY = wp.rcNormalPosition.top;
			g_Settings.iWidth = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
			g_Settings.iHeight = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;

			if (g_Settings.bTabsEnable) {
				int i = TabCtrl_GetCurSel(GetDlgItem(hwndDlg, IDC_TAB));
				if (i != -1) {
					TCITEM tci;
					tci.mask = TCIF_PARAM;
					TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_TAB), i, &tci);
					SESSION_INFO *s = (SESSION_INFO*)tci.lParam;
					if (s) {
						s->wState &= ~GC_EVENT_HIGHLIGHT;
						s->wState &= ~STATE_TALK;
						SendMessage(hwndDlg, GC_FIXTABICONS, 0, (LPARAM)s);
					}
				}
			}

			if (uMsg != WM_ACTIVATE)
				SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));

			pci->SetActiveSession(si->ptszID, si->pszModule);

			if (KillTimer(hwndDlg, TIMERID_FLASHWND))
				FlashWindow(hwndDlg, FALSE);
			if (db_get_w(si->hContact, si->pszModule, "ApparentMode", 0) != 0)
				db_set_w(si->hContact, si->pszModule, "ApparentMode", (LPARAM)0);
			if (pcli->pfnGetEvent(si->hContact, 0))
				pcli->pfnRemoveEvent(si->hContact, GC_FAKE_EVENT);
		}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case NM_RCLICK:
			if (((LPNMHDR)lParam)->idFrom == IDC_TAB) {
				int i = TabCtrl_GetCurSel(((LPNMHDR)lParam)->hwndFrom);
				if (i == -1)
					break;

				TCHITTESTINFO tci = { 0 };
				tci.pt.x = (short)LOWORD(GetMessagePos());
				tci.pt.y = (short)HIWORD(GetMessagePos());
				tci.flags = TCHT_ONITEM;
				ScreenToClient(GetDlgItem(hwndDlg, IDC_TAB), &tci.pt);
				if ((i = TabCtrl_HitTest(((LPNMHDR)lParam)->hwndFrom, &tci)) == -1)
					break;

				TCITEM id = { 0 };
				id.mask = TCIF_PARAM;
				TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_TAB), i, &id);
				SESSION_INFO *s = (SESSION_INFO*)id.lParam;

				ClientToScreen(GetDlgItem(hwndDlg, IDC_TAB), &tci.pt);
				HMENU hSubMenu = GetSubMenu(g_hMenu, 5);
				TranslateMenu(hSubMenu);
				if (s) {
					WORD w = db_get_w(s->hContact, s->pszModule, "TabPosition", 0);
					if (w == 0)
						CheckMenuItem(hSubMenu, ID_LOCKPOSITION, MF_BYCOMMAND | MF_UNCHECKED);
					else
						CheckMenuItem(hSubMenu, ID_LOCKPOSITION, MF_BYCOMMAND | MF_CHECKED);
				}
				else CheckMenuItem(hSubMenu, ID_LOCKPOSITION, MF_BYCOMMAND | MF_UNCHECKED);

				switch (TrackPopupMenu(hSubMenu, TPM_RETURNCMD, tci.pt.x, tci.pt.y, 0, hwndDlg, NULL)) {
				case ID_CLOSE:
					if (TabCtrl_GetCurSel(GetDlgItem(hwndDlg, IDC_TAB)) == i)
						PostMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_CLOSE, BN_CLICKED), 0);
					else
						TabCtrl_DeleteItem(GetDlgItem(hwndDlg, IDC_TAB), i);
					break;

				case ID_CLOSEOTHER:
					{
						int tabId = TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_TAB)) - 1;
						if (tabId > 0) {
							if (TabCtrl_GetCurSel(GetDlgItem(hwndDlg, IDC_TAB)) != i)
								if (s)
									pci->ShowRoom(s, WINDOW_VISIBLE, TRUE);

							for (tabId; tabId >= 0; tabId--) {
								if (tabId == i)
									continue;

								TabCtrl_DeleteItem(GetDlgItem(hwndDlg, IDC_TAB), tabId);
							}
						}
					}
					break;

				case ID_LOCKPOSITION:
					TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_TAB), i, &id);
					if (s != 0) {
						if (!(GetMenuState(hSubMenu, ID_LOCKPOSITION, MF_BYCOMMAND)&MF_CHECKED)) {
							if (s->hContact)
								db_set_w(s->hContact, s->pszModule, "TabPosition", (WORD)(i + 1));
						}
						else db_unset(s->hContact, s->pszModule, "TabPosition");
					}
					break;
				}
			}
			break;

		case EN_MSGFILTER:
			if (((LPNMHDR)lParam)->idFrom == IDC_LOG && ((MSGFILTER *)lParam)->msg == WM_RBUTTONUP) {
				CHARRANGE sel, all = { 0, -1 };
				POINT pt;
				UINT uID = 0;
				HMENU hMenu = 0;
				wchar_t pszWord[4096];

				pt.x = (short)LOWORD(((ENLINK *)lParam)->lParam);
				pt.y = (short)HIWORD(((ENLINK *)lParam)->lParam);
				ClientToScreen(((LPNMHDR)lParam)->hwndFrom, &pt);

				// fixing stuff for searches
				pszWord[0] = '\0';

				POINTL ptl = { (LONG)pt.x, (LONG)pt.y };
				ScreenToClient(GetDlgItem(hwndDlg, IDC_LOG), (LPPOINT)&ptl);
				long iCharIndex = SendDlgItemMessage(hwndDlg, IDC_LOG, EM_CHARFROMPOS, 0, (LPARAM)&ptl);
				if (iCharIndex < 0)
					break;

				long start = SendDlgItemMessage(hwndDlg, IDC_LOG, EM_FINDWORDBREAK, WB_LEFT, iCharIndex);//-iChars;
				long end = SendDlgItemMessage(hwndDlg, IDC_LOG, EM_FINDWORDBREAK, WB_RIGHT, iCharIndex);//-iChars;

				if (end - start > 0) {
					TEXTRANGE tr;
					memset(&tr, 0, sizeof(TEXTRANGE));

					CHARRANGE cr;
					cr.cpMin = start;
					cr.cpMax = end;
					tr.chrg = cr;
					tr.lpstrText = pszWord;
					long iRes = SendDlgItemMessage(hwndDlg, IDC_LOG, EM_GETTEXTRANGE, 0, (LPARAM)&tr);
					if (iRes > 0)
						for (size_t iLen = mir_wstrlen(pszWord) - 1; wcschr(szTrimString, pszWord[iLen]); iLen--)
							pszWord[iLen] = 0;
				}

				uID = CreateGCMenu(hwndDlg, &hMenu, 1, pt, si, NULL, pszWord);
				switch (uID) {
				case 0:
					PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0);
					break;

				case ID_COPYALL:
					SendMessage(((LPNMHDR)lParam)->hwndFrom, EM_EXGETSEL, 0, (LPARAM)&sel);
					SendMessage(((LPNMHDR)lParam)->hwndFrom, EM_EXSETSEL, 0, (LPARAM)& all);
					SendMessage(((LPNMHDR)lParam)->hwndFrom, WM_COPY, 0, 0);
					SendMessage(((LPNMHDR)lParam)->hwndFrom, EM_EXSETSEL, 0, (LPARAM)&sel);
					PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0);
					break;

				case ID_CLEARLOG:
					{
						SESSION_INFO *s = pci->SM_FindSession(si->ptszID, si->pszModule);
						if (s) {
							SetDlgItemText(hwndDlg, IDC_LOG, L"");
							pci->LM_RemoveAll(&s->pLog, &s->pLogEnd);
							s->iEventCount = 0;
							s->LastTime = 0;
							si->iEventCount = 0;
							si->LastTime = 0;
							si->pLog = s->pLog;
							si->pLogEnd = s->pLogEnd;
							PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0);
						}
					}
					break;

				case ID_SEARCH_GOOGLE:
					if (pszWord[0])
						Utils_OpenUrlW(CMStringW(FORMAT, L"http://www.google.com/search?q=%s", pszWord));

					PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0);
					break;

				case ID_SEARCH_WIKIPEDIA:
					if (pszWord[0])
						Utils_OpenUrlW(CMStringW(FORMAT, L"http://en.wikipedia.org/wiki/%s", pszWord));

					PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0);
					break;

				default:
					PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0);
					pci->DoEventHookAsync(hwndDlg, si->ptszID, si->pszModule, GC_USER_LOGMENU, NULL, NULL, (LPARAM)uID);
					break;
				}
				DestroyGCMenu(&hMenu, 5);
			}
			break;

		case EN_LINK:
			if (((LPNMHDR)lParam)->idFrom == IDC_LOG) {
				switch (((ENLINK *)lParam)->msg) {
				case WM_RBUTTONDOWN:
				case WM_LBUTTONUP:
				case WM_LBUTTONDBLCLK:
					{
						CHARRANGE sel;
						SendMessage(((LPNMHDR)lParam)->hwndFrom, EM_EXGETSEL, 0, (LPARAM)&sel);
						if (sel.cpMin != sel.cpMax)
							break;

						TEXTRANGE tr;
						tr.chrg = ((ENLINK *)lParam)->chrg;
						tr.lpstrText = (LPTSTR)mir_alloc(sizeof(wchar_t)*(tr.chrg.cpMax - tr.chrg.cpMin + 1));
						SendMessage(((LPNMHDR)lParam)->hwndFrom, EM_GETTEXTRANGE, 0, (LPARAM)&tr);

						if (((ENLINK *)lParam)->msg == WM_RBUTTONDOWN) {
							HMENU hSubMenu;
							POINT pt;

							hSubMenu = GetSubMenu(g_hMenu, 2);
							TranslateMenu(hSubMenu);
							pt.x = (short)LOWORD(((ENLINK *)lParam)->lParam);
							pt.y = (short)HIWORD(((ENLINK *)lParam)->lParam);
							ClientToScreen(((NMHDR *)lParam)->hwndFrom, &pt);
							switch (TrackPopupMenu(hSubMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwndDlg, NULL)) {
							case ID_NEW:
								Utils_OpenUrlW(tr.lpstrText);
								break;

							case ID_CURR:
								Utils_OpenUrlW(tr.lpstrText, false);
								break;

							case ID_COPY:
								{
									HGLOBAL hData;
									if (!OpenClipboard(hwndDlg))
										break;
									EmptyClipboard();
									hData = GlobalAlloc(GMEM_MOVEABLE, sizeof(wchar_t)*(mir_wstrlen(tr.lpstrText) + 1));
									mir_wstrcpy((wchar_t*)GlobalLock(hData), tr.lpstrText);
									GlobalUnlock(hData);
									SetClipboardData(CF_UNICODETEXT, hData);
									CloseClipboard();
									SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
									break;
								}
							}
							mir_free(tr.lpstrText);
							return TRUE;
						}

						Utils_OpenUrlW(tr.lpstrText);
						SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
						mir_free(tr.lpstrText);
						break;
					}
				}
			}
			break;

		case TTN_NEEDTEXT:
			if (((LPNMHDR)lParam)->idFrom == (UINT_PTR)GetDlgItem(hwndDlg, IDC_LIST)) {
				LPNMTTDISPINFO lpttd = (LPNMTTDISPINFO)lParam;
				SESSION_INFO* parentdat = (SESSION_INFO*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
				POINT p;
				GetCursorPos(&p);
				ScreenToClient(GetDlgItem(hwndDlg, IDC_LIST), &p);
				int item = LOWORD(SendDlgItemMessage(hwndDlg, IDC_LIST, LB_ITEMFROMPOINT, 0, MAKELPARAM(p.x, p.y)));
				USERINFO *ui = pci->SM_GetUserFromIndex(parentdat->ptszID, parentdat->pszModule, item);
				if (ui != NULL) {
					static wchar_t ptszBuf[1024];
					mir_snwprintf(ptszBuf, L"%s: %s\r\n%s: %s\r\n%s: %s",
						TranslateT("Nickname"), ui->pszNick,
						TranslateT("Unique ID"), ui->pszUID,
						TranslateT("Status"), pci->TM_WordToString(parentdat->pStatuses, ui->Status));
					lpttd->lpszText = ptszBuf;
				}
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_LIST:
			if (HIWORD(wParam) == LBN_DBLCLK) {
				TVHITTESTINFO hti;
				hti.pt.x = (short)LOWORD(GetMessagePos());
				hti.pt.y = (short)HIWORD(GetMessagePos());
				ScreenToClient(GetDlgItem(hwndDlg, IDC_LIST), &hti.pt);

				int item = LOWORD(SendDlgItemMessage(hwndDlg, IDC_LIST, LB_ITEMFROMPOINT, 0, MAKELPARAM(hti.pt.x, hti.pt.y)));
				USERINFO *ui = pci->SM_GetUserFromIndex(si->ptszID, si->pszModule, item);
				if (ui) {
					if (GetKeyState(VK_SHIFT) & 0x8000) {
						int start = LOWORD(SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_GETSEL, 0, 0));
						CMStringW buf(FORMAT, (start == 0) ? L"%s: " : L"%s ", ui->pszUID);
						SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_REPLACESEL, FALSE, (LPARAM)buf.c_str());
						PostMessage(hwndDlg, WM_MOUSEACTIVATE, 0, 0);
					}
					else pci->DoEventHookAsync(hwndDlg, si->ptszID, si->pszModule, GC_USER_PRIVMESS, ui->pszUID, NULL, 0);
				}

				return TRUE;
			}

			if (HIWORD(wParam) == LBN_KILLFOCUS)
				RedrawWindow(GetDlgItem(hwndDlg, IDC_LIST), NULL, NULL, RDW_INVALIDATE);
			break;

		case IDOK:
			if (IsWindowEnabled(GetDlgItem(hwndDlg, IDOK))) {
				ptrA pszRtf(Message_GetFromStream(hwndDlg, si));
				if (pszRtf == NULL)
					break;

				MODULEINFO *mi = pci->MM_FindModule(si->pszModule);
				if (mi == NULL)
					break;

				pci->SM_AddCommand(si->ptszID, si->pszModule, pszRtf);

				CMStringW ptszText(ptrW(mir_utf8decodeW(pszRtf)));
				pci->DoRtfToTags(ptszText, mi->nColorCount, mi->crColors);
				ptszText.Trim();
				ptszText.Replace(L"%", L"%%");

				if (mi->bAckMsg) {
					EnableWindow(GetDlgItem(hwndDlg, IDC_MESSAGE), FALSE);
					SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETREADONLY, TRUE, 0);
				}
				else SetDlgItemText(hwndDlg, IDC_MESSAGE, L"");

				EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);

				pci->DoEventHookAsync(hwndDlg, si->ptszID, si->pszModule, GC_USER_MESSAGE, NULL, ptszText, 0);

				SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
			}
			break;

		case IDC_SHOWNICKLIST:
			if (!IsWindowEnabled(GetDlgItem(hwndDlg, IDC_SHOWNICKLIST)))
				break;
			if (si->iType == GCW_SERVER)
				break;

			si->bNicklistEnabled = !si->bNicklistEnabled;

			SendDlgItemMessage(hwndDlg, IDC_SHOWNICKLIST, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIconEx(si->bNicklistEnabled ? "nicklist" : "nicklist2", FALSE));
			SendMessage(hwndDlg, GC_SCROLLTOBOTTOM, 0, 0);
			SendMessage(hwndDlg, WM_SIZE, 0, 0);
			break;

		case IDC_MESSAGE:
			EnableWindow(GetDlgItem(hwndDlg, IDOK), GetRichTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE)) != 0);
			break;

		case IDC_HISTORY:
			if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_HISTORY))) {
				MODULEINFO *pInfo = pci->MM_FindModule(si->pszModule);
				if (pInfo) {
					wchar_t szFile[MAX_PATH], szName[MAX_PATH], szFolder[MAX_PATH];
					wcsncpy_s(szName, (pInfo->ptszModDispName ? pInfo->ptszModDispName : _A2T(si->pszModule)), _TRUNCATE);
					ValidateFilename(szName);

					mir_snwprintf(szFolder, L"%s\\%s", g_Settings.pszLogDir, szName);
					mir_snwprintf(szName, L"%s.log", si->ptszID);
					ValidateFilename(szName);

					mir_snwprintf(szFile, L"%s\\%s", szFolder, szName);
					ShellExecute(hwndDlg, L"open", szFile, NULL, NULL, SW_SHOW);
				}
			}
			break;

		case IDC_CLOSE:
			SendMessage(hwndDlg, GC_REMOVETAB, 0, 0);
			break;

		case IDC_CHANMGR:
			if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_CHANMGR)))
				pci->DoEventHookAsync(hwndDlg, si->ptszID, si->pszModule, GC_USER_CHANMGR, NULL, NULL, 0);
			break;

		case IDC_FILTER:
			if (!IsWindowEnabled(GetDlgItem(hwndDlg, IDC_FILTER)))
				break;

			si->bFilterEnabled = !si->bFilterEnabled;
			SendDlgItemMessage(hwndDlg, IDC_FILTER, BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIconEx(si->bFilterEnabled ? "filter" : "filter2", FALSE));
			if (si->bFilterEnabled && db_get_b(NULL, CHAT_MODULE, "RightClickFilter", 0) == 0) {
				SendMessage(hwndDlg, GC_SHOWFILTERMENU, 0, 0);
				break;
			}
			SendMessage(hwndDlg, GC_REDRAWLOG, 0, 0);
			break;

		case IDC_BKGCOLOR:
			if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_BKGCOLOR))) {
				CHARFORMAT2 cf;
				cf.cbSize = sizeof(CHARFORMAT2);
				cf.dwEffects = 0;

				if (IsDlgButtonChecked(hwndDlg, IDC_BKGCOLOR)) {
					if (db_get_b(NULL, CHAT_MODULE, "RightClickFilter", 0) == 0)
						SendMessage(hwndDlg, GC_SHOWCOLORCHOOSER, 0, (LPARAM)IDC_BKGCOLOR);
					else if (si->bBGSet) {
						cf.dwMask = CFM_BACKCOLOR;
						cf.crBackColor = pci->MM_FindModule(si->pszModule)->crColors[si->iBG];
						SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
					}
				}
				else {
					cf.dwMask = CFM_BACKCOLOR;
					cf.crBackColor = (COLORREF)db_get_dw(NULL, CHAT_MODULE, "ColorMessageBG", GetSysColor(COLOR_WINDOW));
					SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
				}
			}
			break;

		case IDC_COLOR:
			if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_COLOR))) {
				CHARFORMAT2 cf;
				cf.cbSize = sizeof(CHARFORMAT2);
				cf.dwEffects = 0;

				if (IsDlgButtonChecked(hwndDlg, IDC_COLOR)) {
					if (db_get_b(NULL, CHAT_MODULE, "RightClickFilter", 0) == 0)
						SendMessage(hwndDlg, GC_SHOWCOLORCHOOSER, 0, (LPARAM)IDC_COLOR);
					else if (si->bFGSet) {
						cf.dwMask = CFM_COLOR;
						cf.crTextColor = pci->MM_FindModule(si->pszModule)->crColors[si->iFG];
						SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
					}
				}
				else {
					cf.dwMask = CFM_COLOR;
					cf.crTextColor = g_Settings.MessageAreaColor;
					SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
				}
			}
			break;

		case IDC_BOLD:
		case IDC_ITALICS:
		case IDC_UNDERLINE:
			CHARFORMAT2 cf;
			cf.cbSize = sizeof(CHARFORMAT2);
			cf.dwMask = CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;
			cf.dwEffects = 0;

			if (LOWORD(wParam) == IDC_BOLD && !IsWindowEnabled(GetDlgItem(hwndDlg, IDC_BOLD)))
				break;
			if (LOWORD(wParam) == IDC_ITALICS && !IsWindowEnabled(GetDlgItem(hwndDlg, IDC_ITALICS)))
				break;
			if (LOWORD(wParam) == IDC_UNDERLINE && !IsWindowEnabled(GetDlgItem(hwndDlg, IDC_UNDERLINE)))
				break;
			if (IsDlgButtonChecked(hwndDlg, IDC_BOLD))
				cf.dwEffects |= CFE_BOLD;
			if (IsDlgButtonChecked(hwndDlg, IDC_ITALICS))
				cf.dwEffects |= CFE_ITALIC;
			if (IsDlgButtonChecked(hwndDlg, IDC_UNDERLINE))
				cf.dwEffects |= CFE_UNDERLINE;

			SendDlgItemMessage(hwndDlg, IDC_MESSAGE, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
		}
		break;

	case WM_KEYDOWN:
		SetFocus(GetDlgItem(hwndDlg, IDC_MESSAGE));
		break;

	case WM_MOVE:
		SendMessage(hwndDlg, GC_SAVEWNDPOS, 0, 1);
		break;

	case WM_GETMINMAXINFO:
		{
			MINMAXINFO *mmi = (MINMAXINFO*)lParam;
			mmi->ptMinTrackSize.x = si->iSplitterX + 43;
			if (mmi->ptMinTrackSize.x < 350)
				mmi->ptMinTrackSize.x = 350;

			mmi->ptMinTrackSize.y = si->iSplitterY + 80;
		}
		break;

	case WM_LBUTTONDBLCLK:
		if (LOWORD(lParam) < 30)
			PostMessage(hwndDlg, GC_SCROLLTOBOTTOM, 0, 0);
		break;

	case WM_CLOSE:
		if (g_Settings.bTabsEnable && g_Settings.TabRestore && lParam != 1) {
			TCITEM id = { 0 };
			int j = TabCtrl_GetItemCount(GetDlgItem(hwndDlg, IDC_TAB)) - 1;
			id.mask = TCIF_PARAM;
			for (j; j >= 0; j--) {
				TabCtrl_GetItem(GetDlgItem(hwndDlg, IDC_TAB), j, &id);
				SESSION_INFO *s = (SESSION_INFO*)id.lParam;
				if (s)
					TabM_AddTab(s->ptszID, s->pszModule);
			}
		}

		SendMessage(hwndDlg, GC_CLOSEWINDOW, 0, 0);
		break;

	case GC_CLOSEWINDOW:
		if (g_Settings.bTabsEnable)
			SM_SetTabbedWindowHwnd(0, 0);
		DestroyWindow(hwndDlg);
		break;

	case WM_DESTROY:
		SendMessage(hwndDlg, GC_SAVEWNDPOS, 0, 0);

		si->hWnd = NULL;
		si->wState &= ~STATE_TALK;
		DestroyWindow(si->hwndStatus);
		si->hwndStatus = NULL;

		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, 0);
		break;
	}
	return FALSE;
}
