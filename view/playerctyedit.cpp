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

void LoadPlayerCTY(HWND dialog)
{
	Player *p = propdata.p;

	if (propdata.pindex == PLAYER1_INDEX)
		ENABLE_WND(IDC_P_ACTIVE, false);

    HWND hCTY = GetDlgItem(dialog, IDC_P_CTY);

	SetDlgItemText(dialog, IDC_P_CTY, p->cty);
	SetDlgItemText(dialog, IDC_P_CTYSCRIPT, p->ctyfile.c_str());
}

void SavePlayerCTY(HWND dialog)
{
	Player *p = propdata.p;
    HWND hCTY = GetDlgItem(dialog, IDC_P_CTY);

    GetDlgItemText(dialog, IDC_P_CTY, p->cty, _MAX_FNAME);
	GetWindowText(GetDlgItem(dialog, IDC_P_CTYSCRIPT), p->ctyfile);
}

const char errorImpExpFail[] =
"Error. Please make sure the file you selected is valid.";


// FIXME: separate this alternate cohesion crap
void Players_ManageCTY(HWND dialog, bool import)
{
	char path[_MAX_PATH], dir[_MAX_PATH];
	OPENFILENAME ofn;

	//shared init
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = dialog;
	ofn.lpstrFilter = "CTY Scripts (*.cty)\0*.cty\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrCustomFilter = NULL;	//user should not change filter
	ofn.lpstrFile = path;
	ofn.nMaxFile = sizeof(path);
	strcpy(dir, setts.BasePath);
	strcat(dir, "cty");
	ofn.lpstrInitialDir = dir;

	if (import)
	{
		*path = '\0';
		ofn.lpstrFileTitle = propdata.p->cty;
		ofn.nMaxFileTitle = _MAX_FNAME;
		ofn.lpstrTitle = "Import CTY Script";
		ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
		ofn.lpstrDefExt = "cty";

		if (GetOpenFileName(&ofn))
		{
			propdata.p->cty
				[ofn.nFileExtension - ofn.nFileOffset - 1]	//offset to extension '.'
				= '\0';
			//do it
			if (propdata.p->import_cty(path))
				SetWindowText(propdata.statusbar, "CTY successfully imported.");
			else
				MessageBox(dialog, errorImpExpFail, "CTY Import", MB_OK);
		}
	}
	else if (propdata.p->ctyfile.length()) //export, so check for existence
	{
		strcpy(path, propdata.p->cty);
		ofn.lpstrFileTitle = NULL;
		ofn.lpstrTitle = "Export CTY Script";
		ofn.Flags = OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT;
		ofn.lpstrDefExt = "cty";

		if (GetSaveFileName(&ofn))
		{
			//do it
			if (propdata.p->export_cty(path))
				SetWindowText(propdata.statusbar, "CTY successfully exported.");
			else
				MessageBox(dialog, errorImpExpFail, "CTY Export Warning", MB_ICONWARNING);
		}
	}
	else
		MessageBox(dialog, "Sorry, that player doesn't have an CTY File.", "CTY Export", MB_OK);
}

void PlayersCTY_HandleCommand(HWND dialog, WORD code, WORD id, HWND control)
{
	Player *p = propdata.p;

	switch (code)
	{
	case BN_CLICKED:
	case CBN_SELCHANGE:
		switch (id)
		{
		case IDC_P_CTY_SP1:
		case IDC_P_CTY_SP2:
		case IDC_P_CTY_SP3:
		case IDC_P_CTY_SP4:
		case IDC_P_CTY_SP5:
		case IDC_P_CTY_SP6:
		case IDC_P_CTY_SP7:
		case IDC_P_CTY_SP8:
		case IDC_P_CTY_SG:
			SavePlayerCTY(dialog);
			propdata.pindex = id - IDC_P_CTY_SP1;
			propdata.p = &scen.players[propdata.pindex];
			LoadPlayerCTY(dialog);
			break;

		case IDC_P_EXCTY:
			Players_ManageCTY(dialog, false);
			break;

		case IDC_P_IMCTY:
			Players_ManageCTY(dialog, true);
			//we don't need a full LoadPlayerCTY() for this
			SetDlgItemText(dialog, IDC_P_CTY, propdata.p->cty);
			SetDlgItemText(dialog, IDC_P_CTYSCRIPT, p->ctyfile.c_str());
			break;

		case IDC_P_CLEARCTY:
		    {
		        p->clear_cty();
		        SetDlgItemText(dialog, IDC_P_CTYSCRIPT, p->ctyfile.c_str());
	            SetDlgItemText(dialog, IDC_P_CTY, p->cty);
		    }
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

BOOL PlayersCTY_Init(HWND dialog)
{
	SendDlgItemMessage(dialog, IDC_P_SPDIP, CB_SETCURSEL, 0, 0);

	return TRUE;
}

INT_PTR CALLBACK CTYDlgProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ret = FALSE;

	try
	{
		switch (msg)
		{
		case WM_INITDIALOG:
			ret = PlayersCTY_Init(dialog);
			break;

		case WM_COMMAND:
			ret = 0;
			PlayersCTY_HandleCommand(
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
						dialog, IDC_P_CTY_SP1, IDC_P_CTY_SG, IDC_P_CTY_SP1 + propdata.pindex);
					LoadPlayerCTY(dialog);
					return ret;

				case PSN_KILLACTIVE:
					SavePlayerCTY(dialog);
					break;
				}
			}
			break;

		case AOKTS_Loading:
			ret = PlayersCTY_Init(dialog);
			CheckRadioButton(
				dialog, IDC_P_CTY_SP1, IDC_P_CTY_SG, IDC_P_CTY_SP1 + propdata.pindex);
			LoadPlayerCTY(dialog);
			return ret;

		case AOKTS_Saving:
			SavePlayerCTY(dialog);
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
