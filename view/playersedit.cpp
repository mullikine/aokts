/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION
	playeredit.cpp -- Defines functions for Players editor.

	VIEW/CONTROLLER
**/

#include "editors.h"
#include "../res/resource.h"
#include "utilui.h"
#include "../util/utilio.h"
#include "../util/settings.h"
#include "LCombo.h"
#include "mapview.h"
#include "../util/winugly.h"

#include <commdlg.h>

#define NUM_MAX_TEAMS           3
#define NUM_MAX_PLAYER_NAMES    9
#define NUM_TEAMS               8

enum ediplo { NOT_DETERMINED, TEAM1, TEAM2, TEAM3, TEAM4, NEUTRAL, ALONE, FRIENDLY };

const char *max_teams_names[] = { "2 Teams", "3 Teams", "4 Teams" };
const char *num_players_names[] = { "INVALID", "1", "2", "3", "4", "5", "6", "7", "8" };
const char *players_number_names[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };
const char *team_names[] = { "Unclear", "Team 1", "Team 2", "Team 3", "Team 4", "Neutral", "No ally", "Friendly" };
const char *gaia_team_names[] = { "Unclear", "Team 1", "Team 2", "Team 3", "Team 4", "Neutral", "Hostile", "Friendly" };

/*
 * Determines the teams based on the diplomacies.
 */
void LoadDiplomacy(HWND dialog)
{
    ediplo diplo[9] = {NOT_DETERMINED}; // if still NOT_DETERMINED at the end, call it unclear
    int teams = 0;
    // check if p1 is in a team
    // ------------------------
    // p1 must have no neutrals
    // see what players p1 is allied with
    // each of those players must only be allied with p1 and p1's allies excluding their selves.
    //
    // Then increment `int teams` and set team for each teammember in a main table (diplo)
    // ------------------------------------
    int new_team = true; // until proven otherwise
    int already_in_a_team = false;

    bool allies[N_PLAYERS] = {false};
    bool allied_anyone = false;
    bool neutral = true;
    bool friendly = true;
    bool hostile = true;

    // Gaia
    for (int i = 0; i < N_PLAYERS; i++) {
        if (scen.players[N_PLAYERS].diplomacy[i] != DIP_neutral) {
            neutral = false;
        }
        if (scen.players[N_PLAYERS].diplomacy[i] != DIP_ally) {
            friendly = false;
        }
        if (scen.players[N_PLAYERS].diplomacy[i] != DIP_enemy) {
            hostile = false;
        }
    }
    if (neutral) {
        diplo[N_PLAYERS] = NEUTRAL;
    } else if (friendly) {
        diplo[N_PLAYERS] = FRIENDLY;
    } else if (hostile) {
        diplo[N_PLAYERS] = ALONE;
    }

    // check each player for a team
    for (int i_subject = 0; i_subject < N_PLAYERS; i_subject++) {
        new_team = true; // until proven otherwise
        already_in_a_team = false;
        allied_anyone = false;
        neutral = true;
        friendly = true;

        switch (diplo[i_subject]) {
        case TEAM1:
        case TEAM2:
        case TEAM3:
        case TEAM4:
            already_in_a_team = true;
            goto determined;
        }

        for (int i = 0; i < N_PLAYERS; i++) {
            allies[i] = false;
        }

        // first make an inclusive list of that player's allies
        allies[i_subject] = true;
        for (int i = 0; i < N_PLAYERS; i++) {
            if (i == i_subject)
                continue;
            if (scen.players[i_subject].diplomacy[i] == DIP_neutral) {
                new_team = false;
            } else {
                neutral = false;
            }
            if (scen.players[i_subject].diplomacy[i] != DIP_ally) {
                friendly = false;
            }
            allies[i] = scen.players[i_subject].diplomacy[i] == DIP_ally && scen.players[i].diplomacy[i_subject] == DIP_ally;
            if (allies[i])
                allied_anyone = true;
        }

        if (!allied_anyone) {
            new_team = false;
        }

        if (!new_team) {
            goto determined;
        }

        // but being a teammate is more than just being allies
        for (int i = 0; i < N_PLAYERS; i++) {
            if (i == i_subject)
                continue;
            if (allies[i]) {
                // for each ally, determine if they are faithful (fidelious)
                for (int j = 0; j < N_PLAYERS; j++) {
                    if (j == i)
                        continue;
                    if (allies[j]) {
                        if (scen.players[i].diplomacy[j] != DIP_ally) {
                            new_team = false;
                            goto determined;
                        }
                    } else {
                        if (scen.players[i].diplomacy[j] != DIP_enemy) {
                            new_team = false;
                            goto determined;
                        }
                    }
                }
            }
        }

determined:
        if (already_in_a_team) {
            continue;
        } else if (new_team) {
            printf_log("allies %d %d %d\n", allies[0], allies[1], allies[2]);
            for (int i = 0; i < N_PLAYERS; i++) {
                if (allies[i]) {
                    diplo[i] = (ediplo)((int)TEAM1 + teams);
                }
            }
            teams++;
        } else if (neutral) {
            diplo[i_subject] = NEUTRAL;
        } else if (friendly) {
            diplo[i_subject] = FRIENDLY;
        } else if (!allied_anyone) {
            diplo[i_subject] = ALONE;
        }
    }

    // Everything is worked out
    for (int i = 0; i <= N_PLAYERS; i++) {
	    SendDlgItemMessage(dialog, IDC_P_TEAM1 + i, CB_SETCURSEL, diplo[i], 0);
	}
}

void AdaptTeams(HWND dialog)
{
    for (int i = 0; i <= N_PLAYERS; i++) {
        ediplo team = (ediplo)SendDlgItemMessage(dialog, IDC_P_TEAM1 + i, CB_GETCURSEL, 0, 0);
        switch (team) {
        case TEAM1:
        case TEAM2:
        case TEAM3:
        case TEAM4:
            for (int j = 0; j < N_PLAYERS; j++) {
                if (i != j && team == (ediplo)SendDlgItemMessage(dialog, IDC_P_TEAM1 + j, CB_GETCURSEL, 0, 0)) {
                    scen.players[i].diplomacy[j] = DIP_ally;
                } else {
                    scen.players[i].diplomacy[j] = DIP_enemy;
                }
            }
            break;
        case NEUTRAL:
            for (int j = 0; j < N_PLAYERS; j++) {
                if (i != j) {
                    scen.players[i].diplomacy[j] = DIP_neutral;
                }
            }
            break;
        case FRIENDLY:
            for (int j = 0; j < N_PLAYERS; j++) {
                if (i != j) {
                    scen.players[i].diplomacy[j] = DIP_ally;
                }
            }
            break;
        case ALONE:
            for (int j = 0; j < N_PLAYERS; j++) {
                if (i != j) {
                    scen.players[i].diplomacy[j] = DIP_enemy;
                }
            }
            break;
        case NOT_DETERMINED:
        case LB_ERR:
        default:
            true;
        }
    }
}

/* Players */

void LoadMaxTeams(HWND dialog)
{
    if (scen.max_teams == 0) {
        SendDlgItemMessage(dialog, IDC_P_MAX_TEAMS, CB_SETCURSEL, 0, 0);
    } else {
        SendDlgItemMessage(dialog, IDC_P_MAX_TEAMS, CB_SETCURSEL, scen.max_teams - 2, 0);
    }
}

void SaveMaxTeams(HWND dialog)
{
    if (scen.game == AOHD4 || scen.game == AOF4 || scen.game == AOHD6 || scen.game == AOF6) {
        int cur = SendDlgItemMessage(dialog, IDC_P_MAX_TEAMS, CB_GETCURSEL, 0, 0);
        if (cur == 0) {
            scen.max_teams = cur;
        } else {
            scen.max_teams = cur + 2;
        }
    }
}

void LoadActive(HWND dialog)
{
    int num_players = scen.get_number_active_players();
    if (num_players == ERR_combination) {
        SendDlgItemMessage(dialog, IDC_P_NUM_PLAYERS, CB_SETCURSEL, 0, 0);
    } else {
        SendDlgItemMessage(dialog, IDC_P_NUM_PLAYERS, CB_SETCURSEL, num_players, 0);
    }
}

void LoadPlayers(HWND dialog)
{
    for (int i = 0; i <= N_PLAYERS; i++) {
	    SendDlgItemMessage(dialog, IDC_P_P1_NUM + i, CB_SETCURSEL, scen.players[i].player_number, 0);
	    SetDlgItemText(dialog, IDC_P_NAME1 + i, scen.players[i].name);
	    LCombo_SelById(dialog, IDC_P_CIV1 + i, scen.players[i].civ);
	    SetDlgItemInt(dialog, IDC_P_GOLD1 + i, scen.players[i].resources[0], FALSE);
	    SetDlgItemInt(dialog, IDC_P_WOOD1 + i, scen.players[i].resources[1], FALSE);
	    SetDlgItemInt(dialog, IDC_P_FOOD1 + i, scen.players[i].resources[2], FALSE);
	    SetDlgItemInt(dialog, IDC_P_STONE1 + i, scen.players[i].resources[3], FALSE);
	    SendDlgItemMessage(dialog, IDC_P_HUMAN1 + i, BM_SETCHECK, scen.players[i].human, 0);
	    SetDlgItemInt(dialog, IDC_P_POP1 + i, (int)scen.players[i].pop, FALSE);
	    SendDlgItemMessage(dialog, IDC_P_COLOR1 + i, CB_SETCURSEL, scen.players[i].color, 0);	//assuming in order
	    SendDlgItemMessage(dialog, IDC_P_AGE1 + i, CB_SETCURSEL, scen.players[i].age + 1, 0);
    }
}

void SavePlayers(HWND dialog)
{
	AdaptTeams(dialog);

	for (int i = 0; i <= N_PLAYERS; i++) {
	    scen.players[i].resources[0] = GetDlgItemInt(dialog, IDC_P_GOLD1 + i, NULL, FALSE);
	    scen.players[i].resources[1] = GetDlgItemInt(dialog, IDC_P_WOOD1 + i, NULL, FALSE);
	    scen.players[i].resources[2] = GetDlgItemInt(dialog, IDC_P_FOOD1 + i, NULL, FALSE);
	    scen.players[i].resources[3] = GetDlgItemInt(dialog, IDC_P_STONE1 + i, NULL, FALSE);
	    GetDlgItemText(dialog, IDC_P_NAME1 + i, scen.players[i].name, 30);
	    scen.players[i].civ = LCombo_GetSelId(dialog, IDC_P_CIV1 + i);
	    scen.players[i].human = Button_IsChecked(GetDlgItem(dialog, IDC_P_HUMAN1 + i));
	    scen.players[i].pop = static_cast<float>(GetDlgItemInt(dialog, IDC_P_POP1 + i, NULL, FALSE));
	    scen.players[i].color = LCombo_GetSelId(dialog, IDC_P_COLOR1 + i);
	    if (scen.game != AOHD6 && scen.game != AOF6)
	        scen.players[i].age = SendDlgItemMessage(dialog, IDC_P_AGE1 + i, CB_GETCURSEL, 0, 0) - 1;
	    scen.players[i].player_number = SendDlgItemMessage(dialog, IDC_P_P1_NUM + i, CB_GETCURSEL, 0, 0);
	}
}

void Players_HandleCommand(HWND dialog, WORD code, WORD id, HWND control)
{
	Player *p = propdata.p;

	switch (code)
	{
	case BN_CLICKED:
	case CBN_SELCHANGE:
		switch (id)
		{
		case IDC_P_NUM_PLAYERS:
		    {
		        int h;
		        if ((h = SendDlgItemMessage(dialog, IDC_P_NUM_PLAYERS, CB_GETCURSEL, 0, 0)) != LB_ERR) {
		            scen.set_number_active_players(h);
		            LoadPlayers(dialog);
	                LoadActive(dialog);
	                LoadMaxTeams(dialog);
		        }
		    }
		    break;

		case IDC_P_TEAM1:
		case IDC_P_TEAM2:
		case IDC_P_TEAM3:
		case IDC_P_TEAM4:
		case IDC_P_TEAM5:
		case IDC_P_TEAM6:
		case IDC_P_TEAM7:
		case IDC_P_TEAM8:
		case IDC_P_TEAM9:
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

BOOL Players_Init(HWND dialog)
{
    ENABLE_WND(IDC_P_MAX_TEAMS, scen.game == AOHD4 || scen.game == AOF4 || scen.game == AOHD6 || scen.game == AOF6);

	Combo_Fill(dialog, IDC_P_MAX_TEAMS, max_teams_names, NUM_MAX_TEAMS);
	Combo_Fill(dialog, IDC_P_NUM_PLAYERS, num_players_names, NUM_MAX_PLAYER_NAMES);

    for (int i = 0; i <= N_PLAYERS; i++) {
        Combo_Fill(dialog, IDC_P_P1_NUM + i, players_number_names, 9);
	    SendDlgItemMessage(dialog, IDC_P_P1_NUM + i, CB_SETCURSEL, scen.players[i].player_number, 0);
	    LCombo_Fill(dialog, IDC_P_CIV1 + i, esdata.civs.head());
	    SendDlgItemMessage(dialog, IDC_P_CIV1 + i, CB_SETCURSEL, scen.players[i].civ, 0);
	    LCombo_Fill(dialog, IDC_P_COLOR1 + i, esdata.colors.head());
	    SendDlgItemMessage(dialog, IDC_P_COLOR1 + i, CB_SETCURSEL, scen.players[i].color, 0);
	    Combo_PairFill(GetDlgItem(dialog, IDC_P_AGE1 + i), NUM_AGES, ages);
	    ENABLE_WND(IDC_P_AGE1 + i, scen.game != AOHD6 && scen.game != AOF6);
	    SendDlgItemMessage(dialog, IDC_P_AGE1 + i, CB_SETCURSEL, scen.players[i].age, 0);
	    if (i == N_PLAYERS)
	        Combo_Fill(dialog, IDC_P_TEAM1 + i, gaia_team_names, NUM_TEAMS);
	    else
	        Combo_Fill(dialog, IDC_P_TEAM1 + i, team_names, NUM_TEAMS);
    }

    LoadDiplomacy(dialog);

    if (scen.game == AOHD4 || scen.game == AOF4 || scen.game == AOHD6 || scen.game == AOF6) {
	    for (int i = 0; i <= N_PLAYERS; i++) {
	        ENABLE_WND(IDC_P_P1_NUM + i, true);
	    }
	} else {
	    for (int i = 0; i <= N_PLAYERS; i++) {
	        ENABLE_WND(IDC_P_P1_NUM + i, false);
	    }
	}

	/* Set resource names per game */
	if (scen.game == SWGB) {
	    Window_SetTextW(GetDlgItem(dialog, IDC_P_LBL_FOOD), L"Food:");
	    Window_SetTextW(GetDlgItem(dialog, IDC_P_LBL_WOOD), L"Carbon:");
	    Window_SetTextW(GetDlgItem(dialog, IDC_P_LBL_GOLD), L"Nova:");
	    Window_SetTextW(GetDlgItem(dialog, IDC_P_LBL_STONE), L"Ore:");
	    Window_SetTextW(GetDlgItem(dialog, IDC_P_LBL_OREX), L"Ore X:");
	    Window_SetTextW(GetDlgItem(dialog, IDC_P_LBL_OREY), L"Goods:");
	} else {
	    Window_SetTextW(GetDlgItem(dialog, IDC_P_LBL_FOOD), L"Food:");
	    Window_SetTextW(GetDlgItem(dialog, IDC_P_LBL_WOOD), L"Wood:");
	    Window_SetTextW(GetDlgItem(dialog, IDC_P_LBL_GOLD), L"Gold:");
	    Window_SetTextW(GetDlgItem(dialog, IDC_P_LBL_STONE), L"Stone:");
	    Window_SetTextW(GetDlgItem(dialog, IDC_P_LBL_OREX), L"Ore X:");
	    Window_SetTextW(GetDlgItem(dialog, IDC_P_LBL_OREY), L"Goods:");
	}

	return TRUE;
}

BOOL Players_Load(HWND dialog) {
	CheckRadioButton(dialog, IDC_PS_SP1, IDC_PS_SG, IDC_PS_SP1 + propdata.pindex);
	LoadPlayers(dialog);
	LoadActive(dialog);
	LoadMaxTeams(dialog);
	LoadDiplomacy(dialog);

	return TRUE;
}

INT_PTR CALLBACK PlayersDlgProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ret = FALSE;

	try
	{
		switch (msg)
		{
		case WM_INITDIALOG:
			ret = Players_Init(dialog);
			break;

		case WM_COMMAND:
			ret = 0;
			Players_HandleCommand(
				dialog, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
			break;

		case WM_NOTIFY:
			{
				NMHDR *header = (NMHDR*)lParam;
				switch (header->code)
				{

				case PSN_SETACTIVE:
			        ret = Players_Init(dialog);
			        ret = Players_Load(dialog);
				    break;

				case PSN_KILLACTIVE:
					SavePlayers(dialog);
					break;
				}
			}
			break;

		case AOKTS_Loading:
			ret = Players_Init(dialog);
			ret = Players_Load(dialog);
			return ret;

		case AOKTS_Saving:
			SavePlayers(dialog);
	        SaveMaxTeams(dialog);
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
