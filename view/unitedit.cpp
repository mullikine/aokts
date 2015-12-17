/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION
	unitedit.cpp -- Functions for the unit editor.

	VIEW/CONTROLLER
**/

#include "editors.h"

#include "../util/settings.h"
#include "../util/helper.h"
#include "../util/utilio.h"

#include <stdio.h>
#include "../res/resource.h"
#include "utilui.h"
#include "LCombo.h"
#include "LinkListBox.h"
#include "utilunit.h"
#include "mapview.h"

using std::vector;

/**
 * Offset of currently-selected unit in vector.
 */
size_t u_index = SIZE_MAX; // SIZE_MAX is special "no selection" value

//LB index of currently selected constant
unsigned int c_index = UINT_MAX; // UINT_MAX is "no selection" value

#define NUM_LISTS	5
#define NUM_ROTATES 8
const char *rotates[NUM_ROTATES] = { "0/4", "1/4", "2/4", "3/4", "4/4", "5/4", "6/4", "7/4" };

const char szTitle[] =
"Unit Editor";
const char errorUnfoundUnit[] =
"Could not find unit to load. I most likely incorrectly deleted a unit.";
const char errorUnfoundType[] =
"You have some strange units on this map, my friend. Unpredictable results may occur.";
const char warningNoSelDelete[] = "No unit selected! Deleting aborted.";
const char warningNoSelChangeOwnership[] = "No unit selected! Change ownership aborted.";

inline int truncate(float x)
{
	return (int)x;
}

bool Units_Load(HWND dialog)
{
	bool ret = true;
	Unit *u;
	Unit blank;	//for dummy purposes
	HWND unitbox = GetDlgItem(dialog, IDC_U_UNIT);
	const UnitLink *type;

	if (u_index == SIZE_MAX)
	{
		u = &blank;
		// Use first unit type
		u->setType(esdata.units.head());
		u->ident = scen.next_uid;
	}

	else if (u_index < propdata.p->units.size())
	{
		u = &propdata.p->units[u_index];
		SendMessage(propdata.mapview, MAP_HighlightPoint,
			static_cast<int>(u->x), static_cast<int>(u->y));
	}

	else
	{
		u = &blank;
		ret = false;
		MessageBox(dialog, errorUnfoundUnit, szTitle, MB_ICONERROR);
	}

	if (!(type = u->getType()))
	{
		MessageBox(dialog, errorUnfoundType, szTitle, MB_ICONERROR);
		return false;
	}
	SetDlgItemInt(dialog, IDC_U_CONST, type->id(), TRUE);
	c_index = ListBox_Find(unitbox, type);

	//if unit is not in current list, change list to "All"
	if (c_index == UINT_MAX)
	{
		SendDlgItemMessage(dialog, IDC_U_TYPE, CB_SETCURSEL, 0, 0);
		UnitList_FillGroup(unitbox, NULL);

		//now search for unit on the "All" list. (If this fails, too bad.)
		if ((c_index = ListBox_Find(unitbox, type)) == UINT_MAX)
			MessageBox(dialog, errorUnfoundType, szTitle, MB_ICONWARNING);
	}

	SendDlgItemMessage(dialog, IDC_U_UNIT, LB_SETCURSEL, c_index, 0);

	SetDlgItemFloat(dialog, IDC_U_X, u->x);
	SetDlgItemFloat(dialog, IDC_U_Y, u->y);
	SetDlgItemFloat(dialog, IDC_U_Z, u->z);
	SetDlgItemFloat(dialog, IDC_U_ROTATE_VAL, u->rotate);
	// add a little bit (0.1) to make sure we get the right item
	SendDlgItemMessage(dialog, IDC_U_ROTATE, CB_SETCURSEL, (int)((u->rotate + 0.1) / PI * 4), 0);
	SetDlgItemInt(dialog, IDC_U_FRAME, u->frame, TRUE);
	SetDlgItemInt(dialog, IDC_U_GARRISON, u->garrison, TRUE);
	SetDlgItemInt(dialog, IDC_U_ID, u->ident, TRUE);
	SetDlgItemInt(dialog, IDC_U_STATE, u->state, TRUE);
	SetDlgItemInt(dialog, IDC_U_NEXT_AVAIL, scen.next_uid, TRUE);

	return ret;
}

void Units_EnableChangeOwnership(HWND dialog)
{
    unsigned int pindex = propdata.p - scen.players;
    for (int i = 0; i <= GAIA_INDEX; i++) {
        ENABLE_WND(IDC_U_MAKEP1 + i, i != pindex);
    }
}

void Units_DisableChangeOwnership(HWND dialog)
{
	ENABLE_WND(IDC_U_MAKEP1, false);
	ENABLE_WND(IDC_U_MAKEP2, false);
	ENABLE_WND(IDC_U_MAKEP3, false);
	ENABLE_WND(IDC_U_MAKEP4, false);
	ENABLE_WND(IDC_U_MAKEP5, false);
	ENABLE_WND(IDC_U_MAKEP6, false);
	ENABLE_WND(IDC_U_MAKEP7, false);
	ENABLE_WND(IDC_U_MAKEP8, false);
	ENABLE_WND(IDC_U_MAKEGA, false);
}

void Units_Save(HWND dialog)
{
    const Link * ucnst_sel;
	if (u_index != SIZE_MAX)
	{
		Unit& u = propdata.p->units[u_index];
		// Get the selected UnitLink * from constant selection box.
		ucnst_sel = LinkListBox_GetSel(GetDlgItem(dialog, IDC_U_UNIT));

		u.x =		GetDlgItemFloat(dialog, IDC_U_X);
		u.y =		GetDlgItemFloat(dialog, IDC_U_Y);
		u.z =		GetDlgItemFloat(dialog, IDC_U_Z);
		//u.rotate =	(float)SendDlgItemMessage(dialog, IDC_U_ROTATE, CB_GETCURSEL, 0, 0) / 4 * (float)PI;
		u.rotate = GetDlgItemFloat(dialog, IDC_U_ROTATE_VAL);
		u.frame = (short)GetDlgItemInt(dialog, IDC_U_FRAME, NULL, TRUE);
		u.garrison = GetDlgItemInt(dialog, IDC_U_GARRISON, NULL, TRUE);
		u.ident = GetDlgItemInt(dialog, IDC_U_ID, NULL, TRUE);
		u.state = (char)GetDlgItemInt(dialog, IDC_U_STATE, NULL, TRUE);
	}

    HWND typebox = GetDlgItem(dialog, IDC_U_UNIT);

	HWND selbox, ttext;

	selbox = GetDlgItem(dialog, IDC_U_SELU);
	ttext  = GetDlgItem(dialog, IDC_U_CONST);
	enum Sorts sort =
		(enum Sorts)SendDlgItemMessage(dialog, IDC_U_SORT, CB_GETCURSEL, 0, 0);

	unsigned int type_index = SendMessage(typebox, LB_GETCURSEL, 0, 0);

	// If it's a new constant, update selected unit.
	if (type_index != c_index && u_index != SIZE_MAX)
	{
		vector<Unit>& units = propdata.p->units; // for convenience

		// Get index of selected unit in listbox
		unsigned int selected_index = SendMessage(selbox, LB_GETCURSEL, 0, 0);

		Unit& u = units[u_index];

		if (ucnst_sel) // will be NULL if user changed group
			u.setType(static_cast<const UnitLink *>(ucnst_sel));

		//then "update" the string in the selection box
		SendMessage(selbox, LB_DELETESTRING, selected_index, 0);
		selected_index = UnitList_InsertItem(selbox, sort, units, &u);
		SendMessage(selbox, LB_SETCURSEL, selected_index, 0);
	}

	c_index = type_index;	//update this even if no unit selected
	UnitLink *type = (UnitLink*)SendMessage(typebox, LB_GETITEMDATA, type_index, 0);
	if (type)
		SetWindowText(ttext, type->id());
	else
		SetWindowTextW(ttext, L"ERR");
}

void UnitList_ChangeType(HWND dialog, HWND typebox)
{
    unsigned int type_index = SendMessage(typebox, LB_GETCURSEL, 0, 0);
	c_index = type_index;
}

void Units_Reset(HWND dialog)
{
	enum Sorts sort =
		(enum Sorts)SendDlgItemMessage(dialog, IDC_U_SORT, CB_GETCURSEL, 0, 0);
	UnitList_Fill(
		GetDlgItem(dialog, IDC_U_SELU),
		sort,
		propdata.p->units);
	u_index = SIZE_MAX;
	ENABLE_WND(IDC_U_DEL, false);
	Units_DisableChangeOwnership(dialog);
	ENABLE_WND(IDC_U_DESELECT, false);
	Units_Load(dialog);
}

void Units_HandleDelete(HWND dialog)
{
	unsigned index, count, data;
	HWND selbox = GetDlgItem(dialog, IDC_U_SELU);

	if (u_index == SIZE_MAX)
	{
		//MessageBox(dialog, warningNoSelDelete, szTitle, MB_ICONWARNING);
		SetWindowText(propdata.statusbar, warningNoSelDelete);
		return;
	}

	propdata.p->erase_unit(u_index);

	index = SendMessage(selbox, LB_GETCURSEL, 0, 0);
	SendMessage(selbox, LB_DELETESTRING, index, 0);

	/* Update the other items' data. */
	count = SendMessage(selbox, LB_GETCOUNT, 0, 0);
	for (unsigned i = 0; i < count; i++)
	{
		data = SendMessage(selbox, LB_GETITEMDATA, i, 0);
		if (data > u_index)
			SendMessage(selbox, LB_SETITEMDATA, i, data - 1);
	}

	/* Set selection to next item down. If there isn't one, reset stuff. */
	if (SendMessage(selbox, LB_GETCOUNT, 0, 0))
	{
		SendMessage(selbox, LB_SETCURSEL, index, 0);
		u_index = SendMessage(selbox, LB_GETITEMDATA, index, 0);
		Units_Load(dialog);
	}
	else
	{
		ENABLE_WND(IDC_U_DEL, false);
		ENABLE_WND(IDC_U_DESELECT, false);
		Units_DisableChangeOwnership(dialog);
	}
}

void Units_HandleChangeOwnership(HWND dialog, unsigned int player)
{
	unsigned index, count, data;
	HWND selbox = GetDlgItem(dialog, IDC_U_SELU);

	if (u_index == SIZE_MAX)
	{
		//MessageBox(dialog, warningNoSelChangeOwnership, szTitle, MB_ICONWARNING);
		SetWindowText(propdata.statusbar, warningNoSelChangeOwnership);
		return;
	}

    Player * p = scen.players + player;

    if (p == propdata.p)
        return;

    p->add_unit(*(propdata.p->units.begin() + u_index));

	propdata.p->erase_unit(u_index);

    // each triggers
	for (vector<Trigger>::iterator trig = scen.triggers.begin(); trig != scen.triggers.end(); ++trig) {
	    // each effect
	    for (vector<Effect>::iterator iter = trig->effects.begin(); iter != trig->effects.end(); ++iter) {
	        // fix source player, but only when safe
	        int s_player = from_ecplayer(iter->s_player);
	        bool all_same = s_player == propdata.pindex;
	        PlayersUnit pu;

	        if (all_same && s_player != GAIA_INDEX) {
	            if (iter->num_sel == 1 && iter->uids[0] == u_index) {
	                iter->s_player = to_ecplayer(player);
	            } else if (iter->num_sel > 1) {
	                pu = find_map_unit(iter->uids[0]);
	                all_same = all_same && pu.u && s_player == pu.player;
	                for (int i = 1; i < iter->num_sel; i++) {
	                    pu = find_map_unit(iter->uids[i]);
	                    if (pu.u && pu.player != s_player) {
	                        all_same = false;
	                        break;
	                    }
	                }

	                if (all_same) {
	                    iter->s_player = to_ecplayer(player);
	                }
                }
	        }
	    }

	    // conditions
	    for (vector<Condition>::iterator iter = trig->conds.begin(); iter != trig->conds.end(); ++iter) {
	        PlayersUnit pu;
	        pu = find_map_unit(iter->object);
	        if (pu.u && pu.player == propdata.pindex && from_ecplayer(iter->player) == propdata.pindex) {
	            iter->player = to_ecplayer(player);
	        }
        }
	}

	index = SendMessage(selbox, LB_GETCURSEL, 0, 0);
	SendMessage(selbox, LB_DELETESTRING, index, 0);

	/* Update the other items' data. */
	count = SendMessage(selbox, LB_GETCOUNT, 0, 0);
	for (unsigned i = 0; i < count; i++)
	{
		data = SendMessage(selbox, LB_GETITEMDATA, i, 0);
		if (data > u_index)
			SendMessage(selbox, LB_SETITEMDATA, i, data - 1);
	}

	/* Set selection to next item down. If there isn't one, reset stuff. */
	if (SendMessage(selbox, LB_GETCOUNT, 0, 0))
	{
		SendMessage(selbox, LB_SETCURSEL, index, 0);
		u_index = SendMessage(selbox, LB_GETITEMDATA, index, 0);
		Units_Load(dialog);
	}
	else
	{
		ENABLE_WND(IDC_U_DEL, false);
		Units_DisableChangeOwnership(dialog);
		ENABLE_WND(IDC_U_DESELECT, false);
		u_index = SIZE_MAX;
	}

	SendMessage(propdata.mapview, MAP_Reset, 0, 0);
}

void Units_HandleRandomizeRotation(HWND dialog)
{
    //std::ostringstream convert;
    //unsigned int cnst = (static_cast<const UnitLink *>(LinkListBox_Get(GetDlgItem(dialog, IDC_U_UNIT), c_index)))->id();
    //printf("randomize unit frames \"%d\"\n", 0);
    //convert << cnst;
    //const std::string tmp = convert.str();
    //MessageBox(dialog, tmp.c_str(), szTitle, MB_ICONERROR);
    //scen.randomize_unit_frames(cnst);
	//MessageBox(dialog, toString<int>(scen.perversion->max_unit).c_str(), "", MB_ICONERROR);

    //scen.randomize_unit_frames(); // done in aokts
  	Units_Load(dialog);
}

void Units_HandleSelChange(HWND dialog, HWND listbox)
{
	int index;

	//Units_Save(dialog);
	SendMessage(propdata.mapview, MAP_UnhighlightPoint,
		MAP_UNHIGHLIGHT_ALL, 0);
	index = SendMessage(listbox, LB_GETCURSEL, 0, 0);
	u_index = SendMessage(listbox, LB_GETITEMDATA, index, 0);

	//if (u_index != SIZE_MAX)
	//{
	//	Unit &u = propdata.p->units[u_index];
	//	SendMessage(propdata.mapview, MAP_HighlightPoint, u.x, u.y);
	//}

	if (u_index >= 0)
	{
		EnableWindow(GetDlgItem(dialog, IDC_U_DEL), TRUE);
		Units_EnableChangeOwnership(dialog);
		EnableWindow(GetDlgItem(dialog, IDC_U_DESELECT), TRUE);
		EnableMenuItem(propdata.menu, ID_EDIT_DELETE, MF_ENABLED);
	}

	Units_Load(dialog);
}

void Units_HandleTypeChange(HWND dialog, HWND typelist)
{
    if (u_index != SIZE_MAX) {
	    HWND unitlist = GetDlgItem(dialog, IDC_U_UNIT);

	    // See Units_HandleInit for LCombo_Fill() with UnitGroupLink*.
	    UnitList_FillGroup(unitlist,
		        static_cast<const UnitGroupLink *>(LinkComboBox_GetSelPtr(typelist)));
    }
}

void Units_HandleSaveType(HWND dialog)
{
	Units_Save(dialog);
}

void Units_HandleRenumber(HWND dialog)
{
    scen.compress_unit_ids();
}

void Units_HandleDeleteAllType(HWND dialog)
{
	unsigned index;
	HWND selbox = GetDlgItem(dialog, IDC_U_SELU);

    UCNST utype = (static_cast<const UnitLink *>( LinkListBox_Get(GetDlgItem(dialog, IDC_U_UNIT), c_index)))->id();
    propdata.p->erase_unit_type(utype);

	index = SendMessage(selbox, LB_GETCURSEL, 0, 0);
	SendMessage(selbox, LB_DELETESTRING, index, 0);

	ENABLE_WND(IDC_U_DEL, false);
	Units_DisableChangeOwnership(dialog);
	ENABLE_WND(IDC_U_DESELECT, false);
	u_index = SIZE_MAX;
	Units_Reset(dialog);
	SendMessage(propdata.mapview, MAP_Reset, 0, 0);
}

void Units_HandleChangeTypeAll(HWND dialog)
{
	HWND selbox = GetDlgItem(dialog, IDC_U_SELU);

    UCNST sutype = (propdata.p->units.begin() + u_index)->getTypeID(); // selected unit type
    UCNST utype  = (static_cast<const UnitLink *>(LinkListBox_Get(GetDlgItem(dialog, IDC_U_UNIT), c_index)))->id(); // unit type

    propdata.p->change_unit_type_for_all_of_type(sutype, utype);

	Units_Reset(dialog);
}

void Units_HandleDeselect(HWND dialog)
{
	u_index = SIZE_MAX;
	SendDlgItemMessage(dialog, IDC_U_SELU, LB_SETCURSEL, -1, 0);
	Units_HandleSelChange(dialog, GetDlgItem(dialog, IDC_U_SELU));
	SetDlgItemInt(dialog, IDC_U_ID, scen.next_uid, TRUE);
	ENABLE_WND(IDC_U_DEL, false);
	Units_DisableChangeOwnership(dialog);
	ENABLE_WND(IDC_U_DESELECT, false);
	u_index = SIZE_MAX;
}

void Units_HandleAdd(HWND dialog)
{
	UID new_uid = GetDlgItemInt(dialog, IDC_U_ID, NULL, TRUE);
	printf("next_uid: %d, new_uid: %d\n", scen.next_uid, new_uid);
	Unit u(scen.next_uid);
	HWND listbox = GetDlgItem(dialog, IDC_U_SELU);

	u.x =		GetDlgItemFloat(dialog, IDC_U_X);
	u.y =		GetDlgItemFloat(dialog, IDC_U_Y);
	u.z =		GetDlgItemFloat(dialog, IDC_U_Z);
	u.rotate = GetDlgItemFloat(dialog, IDC_U_ROTATE_VAL);
	u.frame = (short)GetDlgItemInt(dialog, IDC_U_FRAME, NULL, TRUE);
	u.garrison = GetDlgItemInt(dialog, IDC_U_GARRISON, NULL, TRUE);
	u.setType(static_cast<const UnitLink *>(
		LinkListBox_Get(GetDlgItem(dialog, IDC_U_UNIT), c_index)));
	propdata.p->units.push_back(u);
	int index = UnitList_Append(listbox, &u, propdata.p->units.size() - 1);

	// Select new item.
	SendMessage(listbox, LB_SETCURSEL, index, 0);
	ENABLE_WND(IDC_U_DEL, true);
	Units_EnableChangeOwnership(dialog);
	ENABLE_WND(IDC_U_DESELECT, true);
	SetDlgItemInt(dialog, IDC_U_ID, scen.next_uid, TRUE);
	scen.next_uid++;
	SetDlgItemInt(dialog, IDC_U_NEXT_AVAIL, scen.next_uid, TRUE);
	u_index = index;
}

void Units_HandleCommand(HWND dialog, WORD code, WORD id, HWND control)
{
	switch (code)
	{
	case BN_CLICKED:	//and menuitem
		switch (id)
		{
		case IDC_U_SAVETYPE:
			Units_HandleSaveType(dialog);
			break;

		case IDC_U_DESELECT:
		    Units_HandleDeselect(dialog);
			break;

		case IDC_U_ADD:
			Units_HandleAdd(dialog);
			break;

		case IDC_U_RENUMBER:
			Units_HandleRenumber(dialog);
			Units_Reset(dialog);
			break;

		case IDC_U_DEL_TYPE:
			Units_HandleDeleteAllType(dialog);
			break;

		case IDC_U_CHANGE_TYPE:
			Units_HandleChangeTypeAll(dialog);
			break;

		case IDC_U_RANDOMIZE_ROT:
			Units_HandleRandomizeRotation(dialog);
			break;

	    case ID_UNITS_DELETE_ALL:
			Units_Reset(dialog);
		    break;

		case IDC_U_DEL:
		case ID_EDIT_DELETE:	//accelerator
			Units_HandleDelete(dialog);
			break;

		case IDC_U_RESORT:
			Units_Reset(dialog);
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

		case IDC_U_MAKEP1:		//BN_CLICKED
			Units_HandleChangeOwnership(dialog, 0);
			break;

		case IDC_U_MAKEP2:		//BN_CLICKED
			Units_HandleChangeOwnership(dialog, 1);
			break;

		case IDC_U_MAKEP3:		//BN_CLICKED
			Units_HandleChangeOwnership(dialog, 2);
			break;

		case IDC_U_MAKEP4:		//BN_CLICKED
			Units_HandleChangeOwnership(dialog, 3);
			break;

		case IDC_U_MAKEP5:		//BN_CLICKED
			Units_HandleChangeOwnership(dialog, 4);
			break;

		case IDC_U_MAKEP6:		//BN_CLICKED
			Units_HandleChangeOwnership(dialog, 5);
			break;

		case IDC_U_MAKEP7:		//BN_CLICKED
			Units_HandleChangeOwnership(dialog, 6);
			break;

		case IDC_U_MAKEP8:		//BN_CLICKED
			Units_HandleChangeOwnership(dialog, 7);
			break;

		case IDC_U_MAKEGA:		//BN_CLICKED
			Units_HandleChangeOwnership(dialog, 8);
			break;

		}
		break;

	case CBN_SELCHANGE:	//and accelerator, LBN_SELCHANGE
		switch (id)
		{
		case IDC_U_SORT:
			Units_Reset(dialog);
			break;

		case IDC_U_SELP:	//CBN_SELCHANGE
			Units_Save(dialog);
			propdata.pindex = SendMessage(control, CB_GETCURSEL, 0, 0);
			propdata.p = scen.players + propdata.pindex;
			Units_Reset(dialog);
			break;

		case IDC_U_SELU:	//LBN_SELCHANGE
			Units_HandleSelChange(dialog, control);
			break;

		case IDC_U_TYPE:	//CBN_SELCHANGE
			Units_HandleTypeChange(dialog, control);
			break;

		case IDC_U_ROTATE:	//CBN_SELCHANGE
		    SetDlgItemFloat(dialog, IDC_U_ROTATE_VAL, (float)SendDlgItemMessage(dialog, IDC_U_ROTATE, CB_GETCURSEL, 0, 0) / 4 * (float)PI);
			break;

		case IDC_U_UNIT:		//LBN_SELCHANGE
			UnitList_ChangeType(dialog, control);
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

BOOL Units_HandleInit(HWND dialog)
{
	Combo_Fill(dialog, IDC_U_SELP, Player::names, NUM_PLAYERS);
	Combo_Fill(dialog, IDC_U_ROTATE, rotates, NUM_ROTATES);
	Combo_Fill(dialog, IDC_U_SORT, sorts, NUM_SORTS);
	SendDlgItemMessage(dialog, IDC_U_SORT, CB_SETCURSEL, 0, 0);
	SendDlgItemMessage(dialog, IDC_U_SELP, CB_SETCURSEL, propdata.pindex, 0);

	LCombo_Fill(dialog, IDC_U_TYPE, esdata.unitgroups.head(), L"All");
	UnitList_FillGroup(GetDlgItem(dialog, IDC_U_UNIT), NULL);

	//set edit control limits
	SendDlgItemMessage(dialog, IDC_U_X, EM_SETLIMITTEXT, 5, 0);
	SendDlgItemMessage(dialog, IDC_U_Y, EM_SETLIMITTEXT, 5, 0);

	Units_Reset(dialog);
	return TRUE;
}

void Units_HandleMapClick(HWND dialog, int x, int y)
{
	if (u_index != SIZE_MAX)
	{
		Unit &u = propdata.p->units[u_index];
		if (setts.nowarnings || (MessageBox(dialog, "Do you want to move the selected unit?", "Unit Editor", MB_YESNO) == IDYES))
		{
		    int ox = truncate(u.x);
		    int oy = truncate(u.y);
		    int dx = x - ox;
		    int dy = y - oy;
		    int dz = scen.map.terrain[x][y].elev - scen.map.terrain[ox][oy].elev;
			HWND mv = propdata.mapview;
			SendMessage(mv, MAP_UnhighlightPoint, ox, oy);
			SetDlgItemFloat(dialog, IDC_U_X, u.x + dx);
			SetDlgItemFloat(dialog, IDC_U_Y, u.y + dy);
			SetDlgItemFloat(dialog, IDC_U_Z, u.z + dz);
			Units_Save(dialog);
			SendMessage(mv, MAP_HighlightPoint, x, y);
			SendMessage(mv, MAP_Reset, 0, 0);
		}
	}
}

INT_PTR CALLBACK UnitDlgProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ret = FALSE;

	try
	{
		switch (msg)
		{
		case WM_INITDIALOG:
			ret = Units_HandleInit(dialog);
			break;

		case WM_COMMAND:
			ret = 0;
			Units_HandleCommand(dialog, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
			break;

		case WM_NOTIFY:
			{
				NMHDR *header = (NMHDR*)lParam;
				switch (header->code)
				{
				case PSN_SETACTIVE:
					SendDlgItemMessage(
						dialog, IDC_U_SELP, CB_SETCURSEL, propdata.pindex, 0);
					Units_Reset(dialog);
					break;

				case PSN_KILLACTIVE:
					SendMessage(
						propdata.mapview, MAP_UnhighlightPoint, MAP_UNHIGHLIGHT_ALL, 0);
				}
			}
			break;

		case WM_VKEYTOITEM:
			if (LOWORD(wParam) == VK_DELETE)
				Units_HandleDelete(dialog);
			ret = -1;
			break;

		case AOKTS_Loading:
		    Units_HandleInit(dialog);
			break;

		case AOKTS_Saving:
			Units_Save(dialog);
			break;

		case MAP_Click:
			Units_HandleMapClick(dialog, LOWORD(lParam), HIWORD(lParam));
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
