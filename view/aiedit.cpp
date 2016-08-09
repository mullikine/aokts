/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION
	playeredit.cpp -- Defines functions for Players editor.

	VIEW/CONTROLLER
**/

#include "editors.h"
#include "../res/resource.h"
#include "utilui.h"
#include "../util/settings.h"
#include "LCombo.h"
#include "mapview.h"
#include "../util/winugly.h"

#include <commdlg.h>

/* Players */

void LoadPlayerAI(HWND dialog)
{
	Player *p = propdata.p;

    HWND hAI = GetDlgItem(dialog, IDC_P_AI);

	/* AI is special */
	SendDlgItemMessage(dialog, IDC_P_AIMODE, BM_SETCHECK, p->aimode == AI_standard, 0);
	SetDlgItemText(dialog, IDC_P_AI, p->ai);
	//EnableWindow(GetDlgItem(dialog, IDC_P_AI), p->aimode != AI_standard);
	SetDlgItemText(dialog, IDC_P_AISCRIPT, p->aifile.c_str());
	SetDlgItemInt(dialog, IDC_P_AIMODE_VAL, p->aimode, FALSE);
}

void SavePlayerAI(HWND dialog)
{
	Player *p = propdata.p;

    HWND hAI = GetDlgItem(dialog, IDC_P_AI);

    // only change this when clicking on checkbox
	//p->aimode = (SendDlgItemMessage(dialog, IDC_P_AIMODE, BM_GETCHECK, 0, 0) != 0);
	if (p->aimode != AI_standard)
	{
		GetDlgItemText(dialog, IDC_P_AI, p->ai, _MAX_FNAME);
		if (*p->ai)
			p->aimode = AI_custom;
	}
	//else
	//	strcpy(p->ai, scen.StandardAI);
	GetWindowText(GetDlgItem(dialog, IDC_P_AISCRIPT), p->aifile);
}

const char errorImpExpFail[] =
"Error. Please make sure the file you selected is valid.";


// FIXME: separate this alternate cohesion crap
void Players_ManageAI(HWND dialog, bool import)
{
	char path[_MAX_PATH], dir[_MAX_PATH];
	OPENFILENAME ofn;

	//shared init
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = dialog;
	ofn.lpstrFilter = "AI Scripts (*.per)\0*.per\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrCustomFilter = NULL;	//user should not change filter
	ofn.lpstrFile = path;
	ofn.nMaxFile = sizeof(path);
	strcpy(dir, setts.BasePath);
	strcat(dir, "ai");
	ofn.lpstrInitialDir = dir;

	if (import)
	{
		*path = '\0';
		ofn.lpstrFileTitle = propdata.p->ai;
		ofn.nMaxFileTitle = _MAX_FNAME;
		ofn.lpstrTitle = "Import AI Script";
		ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
		ofn.lpstrDefExt = "per";

		if (GetOpenFileName(&ofn))
		{
			propdata.p->ai
				[ofn.nFileExtension - ofn.nFileOffset - 1]	//offset to extension '.'
				= '\0';
			//do it
			if (propdata.p->import_ai(path))
				SetWindowText(propdata.statusbar, "AI successfully imported.");
			else
				MessageBox(dialog, errorImpExpFail, "AI Import", MB_OK);
		}
	}
	else if (propdata.p->aifile.length()) //export, so check for existence
	{
		strcpy(path, propdata.p->ai);
		ofn.lpstrFileTitle = NULL;
		ofn.lpstrTitle = "Export AI Script";
		ofn.Flags = OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT;
		ofn.lpstrDefExt = "per";

		if (GetSaveFileName(&ofn))
		{
			//do it
			if (propdata.p->export_ai(path))
				SetWindowText(propdata.statusbar, "AI successfully exported.");
			else
				MessageBox(dialog, errorImpExpFail, "AI Export Warning", MB_ICONWARNING);
		}
	}
	else
		MessageBox(dialog, "Sorry, that player doesn't have an AI File.", "AI Export", MB_OK);
}

void PlayersAI_HandleCommand(HWND dialog, WORD code, WORD id, HWND control)
{
	Player *p = propdata.p;

	switch (code)
	{
	case BN_CLICKED:
	case CBN_SELCHANGE:
		switch (id)
		{
		case IDC_P_AI_SP1:
		case IDC_P_AI_SP2:
		case IDC_P_AI_SP3:
		case IDC_P_AI_SP4:
		case IDC_P_AI_SP5:
		case IDC_P_AI_SP6:
		case IDC_P_AI_SP7:
		case IDC_P_AI_SP8:
		case IDC_P_AI_SG:
			SavePlayerAI(dialog);
			propdata.pindex = id - IDC_P_AI_SP1;
			propdata.p = &scen.players[propdata.pindex];
			LoadPlayerAI(dialog);
			break;

		case IDC_P_EXAI:
			Players_ManageAI(dialog, false);
			break;

		case IDC_P_IMAI:
			Players_ManageAI(dialog, true);
			//we don't need a full LoadPlayerAI() for this
			SetDlgItemText(dialog, IDC_P_AI, propdata.p->ai);
			SetDlgItemText(dialog, IDC_P_AISCRIPT, p->aifile.c_str());
		    if (*p->ai)
			    p->aimode = AI_custom;
			SendDlgItemMessage(dialog, IDC_P_AIMODE, BM_SETCHECK, p->aimode, 0);
			SetDlgItemInt(dialog, IDC_P_AIMODE_VAL, p->aimode, FALSE);
			break;

		case IDC_P_CLEARAI:
		    {
		        p->clear_ai();
		        SetDlgItemText(dialog, IDC_P_AISCRIPT, p->aifile.c_str());
	            SendDlgItemMessage(dialog, IDC_P_AIMODE, BM_SETCHECK, p->aimode == AI_standard, 0);
			    SetDlgItemInt(dialog, IDC_P_AIMODE_VAL, p->aimode, FALSE);
	            SetDlgItemText(dialog, IDC_P_AI, p->ai);
		    }
			break;

		case IDC_P_RANDOMGAME:
		    {
		        char *cstr = p->aifile.unlock(1);
	            strcpy(cstr, "");
		        p->aifile.lock();
		        SetDlgItemText(dialog, IDC_P_AISCRIPT, p->aifile.c_str());

                p->aimode = AI_standard;

	            SendDlgItemMessage(dialog, IDC_P_AIMODE, BM_SETCHECK, p->aimode == AI_standard, 0);
			    SetDlgItemInt(dialog, IDC_P_AIMODE_VAL, p->aimode, FALSE);

	            strcpy(p->ai, scen.StandardAI);
	            SetDlgItemText(dialog, IDC_P_AI, p->ai);
		    }
			break;

		case IDC_P_PROMISORY:
		    {
		        char *cstr = p->aifile.unlock(1);
	            strcpy(cstr, "");
		        p->aifile.lock();
		        SetDlgItemText(dialog, IDC_P_AISCRIPT, p->aifile.c_str());

                p->aimode = AI_standard;

	            SendDlgItemMessage(dialog, IDC_P_AIMODE, BM_SETCHECK, p->aimode == AI_standard, 0);
			    SetDlgItemInt(dialog, IDC_P_AIMODE_VAL, p->aimode, FALSE);

	            strcpy(p->ai, scen.StandardAI2);
	            SetDlgItemText(dialog, IDC_P_AI, p->ai);
		    }
			break;

		case IDC_P_AIMODE:
			//EnableWindow(GetDlgItem(dialog, IDC_P_AI), SendMessage(control, BM_GETCHECK, 0, 0) == BST_UNCHECKED);
			if (SendMessage(control, BM_GETCHECK, 0, 0) == BST_CHECKED) {
			    p->aimode = AI_standard;
			} else {
			    p->aimode = AI_custom;
			}
			SetDlgItemInt(dialog, IDC_P_AIMODE_VAL, p->aimode, FALSE);
			break;

		case ID_TS_EDIT_COPY:
			SendMessage(GetFocus(), WM_COPY, 0, 0);
			break;

		case ID_TS_EDIT_CUT:
			SendMessage(GetFocus(), WM_CUT, 0, 0);
			break;

		case ID_TS_EDIT_PASTE:
			SendMessage(GetFocus(), WM_PASTE, 0, 0);
			break;
		}
		break;

	case EN_SETFOCUS:
		EnableMenuItem(propdata.menu, ID_TS_EDIT_COPY, MF_ENABLED);
		EnableMenuItem(propdata.menu, ID_TS_EDIT_CUT, MF_ENABLED);
		if (IsClipboardFormatAvailable(CF_TEXT))
			EnableMenuItem(propdata.menu, ID_TS_EDIT_PASTE, MF_ENABLED);
		break;

	case EN_KILLFOCUS:
		EnableMenuItem(propdata.menu, ID_TS_EDIT_COPY, MF_GRAYED);
		EnableMenuItem(propdata.menu, ID_TS_EDIT_CUT, MF_GRAYED);
		EnableMenuItem(propdata.menu, ID_TS_EDIT_PASTE, MF_GRAYED);
		break;
	}
}

char ttAI[] =
"Leave blank to have no AI.";

BOOL PlayersAI_Init(HWND dialog)
{
	AddTool(dialog, IDC_P_AI, ttAI);

	return TRUE;
}

INT_PTR CALLBACK AIDlgProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ret = FALSE;

	try
	{
		switch (msg)
		{
		case WM_INITDIALOG:
			ret = PlayersAI_Init(dialog);
			break;

		case WM_COMMAND:
			ret = 0;
			PlayersAI_HandleCommand(
				dialog, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
			break;

		case WM_NOTIFY:
			{
				ret = TRUE;

				NMHDR *header = (NMHDR*)lParam;
				switch (header->code)
				{
				case PSN_SETACTIVE:
					CheckRadioButton(
						dialog, IDC_P_AI_SP1, IDC_P_AI_SG, IDC_P_AI_SP1 + propdata.pindex);
					LoadPlayerAI(dialog);
					return ret;

				case PSN_KILLACTIVE:
					SavePlayerAI(dialog);
					break;
				}
			}
			break;

		case AOKTS_Loading:
			ret = PlayersAI_Init(dialog);
			CheckRadioButton(
				dialog, IDC_P_AI_SP1, IDC_P_AI_SG, IDC_P_AI_SP1 + propdata.pindex);
			LoadPlayerAI(dialog);
			return ret;

		case AOKTS_Saving:
			SavePlayerAI(dialog);
		}
	}
	catch (std::exception& ex)
	{
		// Show a user-friendly message, bug still crash to allow getting all
		// the debugging info.
		unhandledExceptionAlert(dialog, msg, ex);
		throw;
	}

	return ret;
}
