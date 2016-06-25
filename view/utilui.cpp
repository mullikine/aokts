/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION
	utilui.cpp -- Definitions of Utility Functions for UI

	VIEW/CONTROLLER
**/

#include "../model/datatypes.h"
#include "utilui.h"
#include "../util/winugly.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <commdlg.h>
#include <sstream>

/* Options */
const size_t CONV_BUFF_SIZE = 32; // size of string conversion buffers

/* Globals */
HWND tooltip;	//set with TooltipInit()
/* Interface */

static std::string makeUnhandledMessage(UINT msg, std::exception& ex)
{
	std::stringstream stream;
	stream << "Unhandled exception while processing message "
		   << msg
		   << ". Sorry, but I will now crash.\n\nDETAILS: "
		   << ex.what();
	return stream.str();
}

void unhandledExceptionAlert(HWND parent, UINT msg, std::exception& ex)
{
	MessageBox(
			parent,
			makeUnhandledMessage(msg, ex).c_str(),
			"Error",
			MB_ICONERROR);
}

/*
void Combo_Fill(HWND dialog, int id, std::vector<std::string> strings)
{
	HWND control = GetDlgItem(dialog, id);

    for(std::vector<std::string>::const_iterator i = strings.begin(); i != strings.end(); ++i) {
		Combo_AddStringA(control, *i);
    }
}
*/

void Combo_Fill(HWND dialog, int id, char const * * strings, size_t count)
{
    Combo_Clear(dialog, id);
	HWND control = GetDlgItem(dialog, id);
	for (size_t i = 0; i < count; ++i)
		Combo_AddStringA(control, *strings++);
}

unsigned ListBox_Find(HWND unitbox, const void *type)
{
	int index;
	index = SendMessageW(unitbox, LB_GETCOUNT, 0, 0);

	while (index--)
	{
		if (List_GetItemData_cPtr(unitbox, index) == type)
			return index;
	}

	return UINT_MAX;
}

void GetWindowText(HWND wnd, SString &value)
{
	int len = GetWindowTextLengthA(wnd);
	if (len)
	{
		char *cstr = value.unlock(++len);
		GetWindowTextA(wnd, cstr, len);
		value.lock();
	}
	else
		value.erase();
}

void GetWindowTextCstr(HWND wnd, char *value)
{
	int len = GetWindowTextLengthA(wnd);
	if (len)
	{
		GetWindowTextA(wnd, value, len + 1);
	}
	else
	    strcpy(value, "");
}

void SetWindowText(HWND wnd, int value)
{
	wchar_t buffer[CONV_BUFF_SIZE];
	swprintf(buffer, CONV_BUFF_SIZE, L"%d", value);
	SetWindowTextW(wnd, buffer);
}

float GetDlgItemFloat(HWND dialog, UINT id)
{
	// We use Unicode buffers since it's what Windows uses internally and we
	// pay nothing for using it here.

	// get the text
	wchar_t buffer[CONV_BUFF_SIZE];
	GetDlgItemTextW(dialog, id, buffer, CONV_BUFF_SIZE);

	// scan the text
	float ret = 0.0; // default to 0.0
	swscanf(buffer, L"%f", &ret);

	return ret;
}

void SetDlgItemFloat(HWND dialog, UINT id, double value)
{
	// See GetDlgItemFloat() for "why Unicode?".

	wchar_t buffer[CONV_BUFF_SIZE];
	swprintf(buffer, CONV_BUFF_SIZE, L"%.2f", value);
	SetDlgItemTextW(dialog, id, buffer);
}

UINT GetCheckedRadio(HWND dialog, UINT first, UINT last)
{
	while (first <= last)
	{
		if (SendDlgItemMessage(dialog, first, BM_GETCHECK, 0, 0))
			return first;
		first++;
	}
	return 0;
}

void Combo_PairFill(HWND combobox, unsigned count, const struct PAIR *pairs)
{
	SendMessage(combobox, CB_RESETCONTENT, 0, 0);
	while (count--)
	{
		unsigned index, type;
		type = pairs->index;
		index = SendMessage(combobox, CB_ADDSTRING, 0, (LPARAM)pairs->name);
		SendMessage(combobox, CB_SETITEMDATA, index, type);

		pairs++;
	}
}

LRESULT Combo_SelectByData(HWND combobox, int data)
{
	LRESULT index;
	index = SendMessageW(combobox, CB_GETCOUNT, 0, 0);

	while (index--)
	{
		if (SendMessage(combobox, CB_GETITEMDATA, index, 0) == data)
		{
			SendMessage(combobox, CB_SETCURSEL, index, 0);
			return index;
		}
	}

	return UINT_MAX;
}

bool GetOpenFileNameA(HWND owner, char * path, DWORD maxPath)
{
	OPENFILENAMEA ofn;

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = owner;
	ofn.lpstrFilter = NULL;
	ofn.lpstrCustomFilter = NULL;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = path;
	ofn.nMaxFile = maxPath;
	ofn.lpstrFileTitle = NULL;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = NULL;
	ofn.Flags = OFN_FILEMUSTEXIST;
	ofn.lpstrDefExt = NULL;

	*path = '\0';

	return GetOpenFileNameA(&ofn) == TRUE;
}

bool GetSaveFileNameA(HWND owner, char * path, DWORD maxPath)
{
	OPENFILENAMEA ofn;

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = owner;
	ofn.lpstrFilter = NULL;
	ofn.lpstrCustomFilter = NULL;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = path;
	ofn.nMaxFile = maxPath;
	ofn.lpstrFileTitle = NULL;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = NULL;
	ofn.Flags = 0;
	ofn.lpstrDefExt = NULL;

	return GetSaveFileNameA(&ofn) == TRUE;
}

/* Tooltips */

void TooltipInit(HWND tt)
{
	tooltip = tt;
}

BOOL AddTool(HWND dialog, int ctrlId, LPTSTR text)
{
	TOOLINFO ti;

	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	ti.hwnd = dialog;
	ToolTip_FillIDWithHwnd(ti, GetDlgItem(dialog, ctrlId));
	ti.lpszText = text;

	return ToolTip_AddTool(tooltip, &ti);
}

void Combo_Clear(HWND dialog, int id)
{
	HWND listbox = GetDlgItem(dialog, id);
	SendMessage(listbox, CB_RESETCONTENT, 0, 0);
}

void List_Clear(HWND dialog, int id)
{
	HWND listbox = GetDlgItem(dialog, id);
	SendMessage(listbox, LB_RESETCONTENT, 0, 0);
}

bool TSSetDlgItemInt(HWND hDlg, int nIDDlgItem, UINT uValue, BOOL bSigned) {
	char text [16];
    wsprintf(text, "%d", uValue);
    return SetDlgItemText(hDlg, nIDDlgItem, text);
}

const char * gameName(Game version) {
	switch (version) {
	case AOE:
	    return "Age of Empires";
	case ROR:
	    return "Rise of Rome";
	case AOK:
	    return "Age of Kings";
	case AOC:
	    return "The Conquerors";
	case UP:
	    return "The Conquerors: UserPatch";
	case AOHD:
	    return "Age of Empires II: HD Edition 3.9";
	case AOF:
	    return "The Forgotten 3.9";
	case AOHD4:
	    return "Age of Empires II: HD Edition 4.2";
	case AOF4:
	    return "The Forgotten 4.2";
	case AOHD6:
	    return "Age of Empires II: HD Edition 4.3 (2F)";
	case AOF6:
	    return "The Forgotten 4.3 (2F)";
	case SWGB:
	    return "Star Wars Galactic Battlegrounds";
	case SWGBCC:
	    return "Clone Campaigns";
	default:
	    return "Unknown";
	}
}
