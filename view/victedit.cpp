/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION
	victedit.cpp -- Defines functions for Victory editor.

	VIEW/CONTROLLER
**/

#include "editors.h"
#include "../util/settings.h"
#include "../res/resource.h"
#include "utilui.h"

const char errorImpExpFail[] =
"Error. Please make sure the file you selected is valid.";

BOOL PlayersVC_Init(HWND dialog)
{
	SendDlgItemMessage(dialog, IDC_P_SPDIP, CB_SETCURSEL, 0, 0);

	return TRUE;
}

void LoadVictory(HWND dialog)
{
    /* Global Victory */

	Victory *vict = &scen.vict;
	SetDlgItemInt(dialog, IDC_V_SCORE, vict->score, FALSE);
	SetDlgItemInt(dialog, IDC_V_TIME, vict->time, FALSE);
	SetDlgItemInt(dialog, IDC_V_RELICS, vict->relics, FALSE);
	SetDlgItemInt(dialog, IDC_V_EXPL, vict->expl, FALSE);
	SendDlgItemMessage(dialog, IDC_V_ALL, BM_SETCHECK, vict->all, 0);
	SendDlgItemMessage(dialog, IDC_V_CONQUEST, BM_SETCHECK, vict->conq, 0);
	CheckRadioButton(dialog, IDC_V_SSTD, IDC_V_SCUSTOM, IDC_V_SSTD + vict->mode);

    /* Player Victory */

	Player *p = propdata.p;

	if (propdata.pindex == PLAYER1_INDEX)
		ENABLE_WND(IDC_P_ACTIVE, false);

    HWND hVC = GetDlgItem(dialog, IDC_P_VC);

	SetDlgItemText(dialog, IDC_P_VC, p->vc);
	SetDlgItemText(dialog, IDC_P_VCSCRIPT, p->vcfile.c_str());
}

void SaveVictory(HWND dialog)
{
	Victory *vict = &scen.vict;
	vict->score = GetDlgItemInt(dialog, IDC_V_SCORE, NULL, FALSE);
	vict->time = GetDlgItemInt(dialog, IDC_V_TIME, NULL, FALSE);
	vict->relics = GetDlgItemInt(dialog, IDC_V_RELICS, NULL, FALSE);
	vict->expl = GetDlgItemInt(dialog, IDC_V_EXPL, NULL, FALSE);
	vict->all = SendDlgItemMessage(dialog, IDC_V_ALL, BM_GETCHECK, 0, 0);
	vict->conq = SendDlgItemMessage(dialog, IDC_V_CONQUEST, BM_GETCHECK, 0, 0);

	Player *p = propdata.p;
    HWND hVC = GetDlgItem(dialog, IDC_P_VC);

    GetDlgItemText(dialog, IDC_P_VC, p->vc, _MAX_FNAME);
	GetWindowText(GetDlgItem(dialog, IDC_P_VCSCRIPT), p->vcfile);
}

// FIXME: separate this alternate cohesion crap
void Players_ManageVC(HWND dialog, bool import)
{
	char path[_MAX_PATH], dir[_MAX_PATH];
	OPENFILENAME ofn;

	//shared init
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = dialog;
	ofn.lpstrFilter = "VC Scripts (*.vc)\0*.vc\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrCustomFilter = NULL;	//user should not change filter
	ofn.lpstrFile = path;
	ofn.nMaxFile = sizeof(path);
	strcpy(dir, setts.BasePath);
	strcat(dir, "vc");
	ofn.lpstrInitialDir = dir;

	if (import)
	{
		*path = '\0';
		ofn.lpstrFileTitle = propdata.p->vc;
		ofn.nMaxFileTitle = _MAX_FNAME;
		ofn.lpstrTitle = "Import VC Script";
		ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
		ofn.lpstrDefExt = "vc";

		if (GetOpenFileName(&ofn))
		{
			propdata.p->vc
				[ofn.nFileExtension - ofn.nFileOffset - 1]	//offset to extension '.'
				= '\0';
			//do it
			if (propdata.p->import_vc(path))
				SetWindowText(propdata.statusbar, "VC successfully imported.");
			else
				MessageBox(dialog, errorImpExpFail, "VC Import", MB_OK);
		}
	}
	else if (propdata.p->aifile.length()) //export, so check for existence
	{
		strcpy(path, propdata.p->vc);
		ofn.lpstrFileTitle = NULL;
		ofn.lpstrTitle = "Export VC Script";
		ofn.Flags = OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT;
		ofn.lpstrDefExt = "vc";

		if (GetSaveFileName(&ofn))
		{
			//do it
			if (propdata.p->export_vc(path))
				SetWindowText(propdata.statusbar, "VC successfully exported.");
			else
				MessageBox(dialog, errorImpExpFail, "VC Export Warning", MB_ICONWARNING);
		}
	}
	else
		MessageBox(dialog, "Sorry, that player doesn't have an VC File.", "VC Export", MB_OK);
}

void Vict_DisableControls(HWND dialog, int mode)
{
	scen.vict.mode = mode;
	for (int i = IDC_V_ALL; i <= IDC_V_CONQUEST; i++)
		ENABLE_WND(i, false);
	switch (mode)
	{
	case Victory::MODE_Score:
		ENABLE_WND(IDC_V_SCORE, true);
		break;
	case Victory::MODE_Time:
		ENABLE_WND(IDC_V_TIME, true);
		break;
	case Victory::MODE_Custom:
		ENABLE_WND(IDC_V_ALL, true);
		ENABLE_WND(IDC_V_RELICS, true);
		ENABLE_WND(IDC_V_EXPL, true);
		ENABLE_WND(IDC_V_CONQUEST, true);
		break;
	}
}

/**
 * Resets the dialog when new data is available, such as when activating the
 * dialog.
 */
void reset(HWND dialog)
{
	CheckRadioButton(
					 dialog, IDC_P_VC_SP1, IDC_P_VC_SG, IDC_P_VC_SP1 + propdata.pindex);
	LoadVictory(dialog);
	Vict_DisableControls(dialog, scen.vict.mode);
}

void Victory_HandleCommand(HWND dialog, WORD code, WORD id, HWND)
{
	Player *p = propdata.p;

	switch (code)
	{
	case BN_CLICKED:	//or menuitem click
	case CBN_SELCHANGE: //accelerator
		if (id >= IDC_V_SSTD && id <= IDC_V_SCUSTOM)
			Vict_DisableControls(dialog, id - IDC_V_SSTD);
		else
		switch (id)
		{
		case IDC_P_VC_SP1:
		case IDC_P_VC_SP2:
		case IDC_P_VC_SP3:
		case IDC_P_VC_SP4:
		case IDC_P_VC_SP5:
		case IDC_P_VC_SP6:
		case IDC_P_VC_SP7:
		case IDC_P_VC_SP8:
		case IDC_P_VC_SG:
			SaveVictory(dialog);
			propdata.pindex = id - IDC_P_VC_SP1;
			propdata.p = &scen.players[propdata.pindex];
			reset(dialog);
			break;

		case IDC_P_EXVC:
			Players_ManageVC(dialog, false);
			break;

		case IDC_P_IMVC:
			Players_ManageVC(dialog, true);
			//we don't need a full LoadPlayerVC() for this
			SetDlgItemText(dialog, IDC_P_VC, propdata.p->vc);
			SetDlgItemText(dialog, IDC_P_VCSCRIPT, p->vcfile.c_str());
			break;

		case IDC_P_CLEARVC:
		    {
		        p->clear_vc();
		        SetDlgItemText(dialog, IDC_P_VCSCRIPT, p->vcfile.c_str());
	            SetDlgItemText(dialog, IDC_P_VC, p->vc);
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

INT_PTR CALLBACK VictDlgProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ret = FALSE;

	try
	{
		switch (msg)
		{
		case WM_INITDIALOG:
			ret = PlayersVC_Init(dialog);
			return TRUE;

		case WM_COMMAND:
			Victory_HandleCommand(
				dialog, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
			break;

		case WM_NOTIFY:
			{
				NMHDR *header = (NMHDR*)lParam;
				switch (header->code)
				{
				case PSN_SETACTIVE:
					reset(dialog);
					break;
				case PSN_KILLACTIVE:
					SaveVictory(dialog);
					break;
				}
			}
			break;

		case AOKTS_Loading:
			CheckRadioButton(
				dialog, IDC_P_VC_SP1, IDC_P_VC_SG, IDC_P_VC_SP1 + propdata.pindex);
			reset(dialog);
			return 0;

		case AOKTS_Saving:
			SaveVictory(dialog);
		}
	}
	catch (std::exception& ex)
	{
		// Show a user-friendly message, bug still crash to allow getting all
		// the debugging info.
		unhandledExceptionAlert(dialog, msg, ex);
		throw;
	}

	return 0;
}
