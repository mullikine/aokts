/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION
	imedit.cpp -- Defines functions for Info/Messages editor.

	VIEW/CONTROLLER
**/

#include "editors.h"
#include "../res/resource.h"
#include "../util/settings.h"
#include "utilui.h"
#include <commdlg.h>

/* Info / Messages */

int IM_msel = 0;
int IM_csel = 0;

void LoadIM(HWND dialog)
{
	ENABLE_WND(IDC_M_USERPATCH, scen.game == AOC || scen.game == UP);

	SetDlgItemText(dialog, IDC_M_MSGS, scen.messages[IM_msel].c_str());
	SetDlgItemText(dialog, IDC_M_CINEM, scen.cinem[IM_csel]);
	CheckDlgButton(dialog, IDC_G_ALLTECHS, scen.all_techs);
	CheckDlgButton(dialog, IDC_G_LOCKTEAMS, scen.lock_teams);
	CheckDlgButton(dialog, IDC_G_CHOOSETEAMS, scen.player_choose_teams);
	CheckDlgButton(dialog, IDC_G_RANDPOSITIONS, scen.random_start_points);
	SetDlgItemInt(dialog, IDC_M_STABLE, scen.mstrings[IM_msel], TRUE);
	SetDlgItemInt(dialog, IDC_G_NEXTID, scen.next_uid, FALSE);
	SetDlgItemFloat(dialog, IDC_G_X, scen.editor_pos[0]);
	SetDlgItemFloat(dialog, IDC_G_Y, scen.editor_pos[1]);
	CheckDlgButton(dialog, IDC_M_USERPATCH, scen.game == UP);

    for (size_t i = 0; i <= (size_t)Dataset::AOAK; i++) {
        ENABLE_WND(IDC_DEP_AOK + i, (setts.editall?true:i>(size_t)Dataset::AOC) && scen.header.header_type == HT_AOE2SCENARIO);
        CheckDlgButton(dialog, IDC_DEP_AOK + i, scen.header.datasetRequired((Dataset::Value)i));
    }
}

void SaveM(HWND dialog)
{
	GetWindowText(GetDlgItem(dialog, IDC_M_MSGS), scen.messages[IM_msel]);
	GetDlgItemText(dialog, IDC_M_CINEM, scen.cinem[IM_csel], 0x20);
	scen.all_techs = IsDlgButtonChecked(dialog, IDC_G_ALLTECHS);
	scen.lock_teams = IsDlgButtonChecked(dialog, IDC_G_LOCKTEAMS);
	scen.player_choose_teams = IsDlgButtonChecked(dialog, IDC_G_CHOOSETEAMS);
	scen.random_start_points = IsDlgButtonChecked(dialog, IDC_G_RANDPOSITIONS);
	scen.mstrings[IM_msel] = GetDlgItemInt(dialog, IDC_M_STABLE, NULL, TRUE);
	scen.next_uid = GetDlgItemInt(dialog, IDC_G_NEXTID, NULL, FALSE);
	scen.editor_pos[0] = GetDlgItemFloat(dialog, IDC_G_X);
	scen.editor_pos[1] = GetDlgItemFloat(dialog, IDC_G_Y);

    if (scen.header.header_type == HT_AOE2SCENARIO) {
        for (size_t i = 0; i <= (size_t)Dataset::AOAK; i++) {
            if (IsDlgButtonChecked(dialog, IDC_DEP_AOK + i)) {
                scen.header.enableDataset((Dataset::Value)i);
            } else {
                scen.header.disableDataset((Dataset::Value)i);
            }
        }
    }
}

void ExportBitmap(HWND dialog)
{
	char path[_MAX_PATH];
	OPENFILENAME ofn;
	bool success = true;

    if (scen.bBitmap && !(( scen.game == AOHD6 || scen.game == AOF6 ) && scen.bBitmap == 3))
	{
		strcpy(path, scen.cinem[3]);

		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = dialog;
		ofn.lpstrFilter = "Bitmap (*.bmp)\0*.bmp\0";
		ofn.lpstrCustomFilter = NULL;
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = path;
		ofn.nMaxFile = _MAX_PATH;
		ofn.lpstrFileTitle = NULL;
		ofn.lpstrInitialDir = setts.BasePath;
		ofn.lpstrTitle = "Export Bitmap";
		ofn.Flags = OFN_NOREADONLYRETURN;
		ofn.lpstrDefExt = "bmp";

		if (GetSaveFileName(&ofn))
			success = scen.export_bmp(path);

		if (!success)
			MessageBox(dialog, "Bitmap export failed.", "AOKTS Warning", MB_ICONWARNING);
	}
	else
		MessageBox(dialog, "No bitmap to export. Duh?", "Bitmap Export", MB_OK);
}

void IMsgs_Reset(HWND dialog)
{
	char string[32], version[5] = "0.00";
	char *version1 = scen.header.version;

	//these aren't gonna change, so load them here
	if (version1[0] == '\0')
		version1 = version;
	sprintf(string, "%s - %.2f", version1, scen.version2);

	ENABLE_WND(IDC_G_LOCKTEAMS, scen.game == AOHD4 || scen.game == AOF4 || scen.game == AOHD6 || scen.game == AOF6);
	ENABLE_WND(IDC_G_CHOOSETEAMS, scen.game == AOHD4 || scen.game == AOF4 || scen.game == AOHD6 || scen.game == AOF6);
	ENABLE_WND(IDC_G_RANDPOSITIONS, scen.game == AOHD4 || scen.game == AOF4 || scen.game == AOHD6 || scen.game == AOF6);

	SetDlgItemText(dialog, IDC_G_VER, string);
	sprintf(string, "%.2f", scen.trigver);
	SetDlgItemText(dialog, IDC_G_TRIGSYSVER, string);
	SetDlgItemText(dialog, IDC_G_GAME, gameName(scen.game));
	SetDlgItemText(dialog, IDC_G_TIMESTAMP, _ctime32(&scen.header.timestamp));
	SetDlgItemText(dialog, IDC_G_ONAME, scen.origname);
	SetDlgItemFloat(dialog, IDC_G_X, scen.editor_pos[0]);
	SetDlgItemFloat(dialog, IDC_G_Y, scen.editor_pos[1]);

	IM_msel = 0;
	SendDlgItemMessage(dialog, IDC_M_SEL, CB_SETCURSEL, 0, 0);
	IM_csel = 3;	//Bitmap = default
	SendDlgItemMessage(dialog, IDC_M_SELC, CB_SETCURSEL, 3, 0);
	LoadIM(dialog);
}

void IMsgs_HandleCommand(HWND dialog, WORD code, WORD id, HWND control)
{
	switch (code)
	{
	case BN_CLICKED:	//and menuitem click!
	case CBN_SELCHANGE:	//and accelerator!
		switch (id)
		{
		case IDC_M_EXPORT:
			ExportBitmap(dialog);
			break;

		case IDC_M_SEL:
			SaveM(dialog);
			IM_msel = (USHORT)SendMessage(control, CB_GETCURSEL, 0, 0);
			LoadIM(dialog);
			break;

		case IDC_M_SELC:
			SaveM(dialog);
			IM_csel = (USHORT)SendMessage(control, CB_GETCURSEL, 0, 0);
			LoadIM(dialog);
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

		case IDC_M_USERPATCH:
	        if (scen.game == AOC && IsDlgButtonChecked(dialog, IDC_M_USERPATCH)) {
	            scen.game = UP;
	        } else if (scen.game == UP && !IsDlgButtonChecked(dialog, IDC_M_USERPATCH)) {
	            if (!scen.is_userpatch()) {
	                scen.game = AOC;
                } else {
                    CheckDlgButton(dialog, IDC_M_USERPATCH, BST_CHECKED);
			        MessageBox(dialog, "Can't disable. Scenario uses userpatch-only features.\nTry converting scenario.", "AOKTS Warning", MB_ICONWARNING);
                }
	        }
	        scen.adapt_game();
		    IMsgs_Reset(dialog);
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

INT_PTR CALLBACK IMsgsDlgProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ret = FALSE;

	try
	{
	switch (msg)
	{
		case WM_INITDIALOG:
			{
				Combo_Fill(dialog, IDC_M_SEL, message_names, NUM_MSGS);
				Combo_Fill(dialog, IDC_M_SELC, cinem_names, NUM_CINEM);

				return TRUE;
			}

		case WM_COMMAND:
			IMsgs_HandleCommand(
				dialog, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
			break;

		case WM_NOTIFY:
			{
				NMHDR *header = (NMHDR*)lParam;
				switch (header->code)
				{
				case PSN_SETACTIVE:
					IMsgs_Reset(dialog);
					ret = 0;
					break;

				case PSN_KILLACTIVE:
					SaveM(dialog);
					break;
				}
			}
			break;

		case AOKTS_Saving:
			SaveM(dialog);
			break;

		case AOKTS_Loading:
			IMsgs_Reset(dialog);
			ret = 0;
			break;
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
