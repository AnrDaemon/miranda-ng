/*
Quick Messages plugin for Miranda IM

Copyright (C) 2008 Danil Mozhar

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#define QMF_NEW				(1<<0)
#define QMF_DELETNEEDED		(1<<1)
#define QMF_RENAMED			(1<<2)

#define QMF_EX_CHILD		(1<<0)
#define QMF_EX_SEPARATOR	(1<<1)

typedef struct _tagButtonData
	{
	DWORD dwPos;
	DWORD dwOPPos;
	BYTE  fEntryType;
	BYTE  fEntryOpType;
	BYTE  bIsServName;
	BYTE  bIsOpServName;
	BYTE  bInQMenu;
	BYTE  bOpInQMenu;
	DWORD dwOPFlags;
	wchar_t *pszName;
	wchar_t *pszValue;
	wchar_t *pszOpValue;
	wchar_t *pszOpName;
	}ButtonData;

typedef struct _tagListData
	{
	SortedList* sl;
	DWORD dwPos;
	DWORD dwOPPos;
	wchar_t* ptszQValue;
	wchar_t* ptszOPQValue;
	wchar_t* ptszButtonName;
	BYTE  bIsServName;
	BYTE  bIsOpServName;
	DWORD dwOPFlags;
	}ListData;

typedef struct _tagQuickData
	{
	DWORD dwPos;
	BOOL bIsService;
	BYTE fEntryType;
	wchar_t* ptszValue;
	wchar_t* ptszValueName;
	}QuickData;

void InitButtonsList();
void DestructButtonsList();
int  sstSortButtons(const void * vmtbi1, const void * vmtbi2);
int  sstOpSortButtons(const void * vmtbi1, const void * vmtbi2);
int  sstQuickSortButtons(const void * vmtbi1, const void * vmtbi2);
void li_ZeroQuickList(SortedList *pList);
wchar_t* getMenuEntry(int entrynum,int buttonnum,BYTE mode) ;
int RegisterCustomButton(WPARAM wParam,LPARAM lParam);
void RemoveMenuEntryNode(SortedList *pList, int index);
void DestroyButton(int listnum);
void SaveModuleSettings(int buttonnum,ButtonData* bd);
void CleanSettings(int buttonnum,int from);
DWORD BalanceButtons(int buttonsWas,int buttonsNow);
wchar_t* ParseString(MCONTACT hContact,wchar_t* ptszQValueIn,wchar_t* ptszText,wchar_t* ptszClip,int QVSize,int TextSize ,int ClipSize);
