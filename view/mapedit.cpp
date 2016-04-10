/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION
	mapedit.cpp -- Defines functions for Map/Terrain editor.

	VIEW/CONTROLLER
**/

#include "editors.h"
#include "../res/resource.h"
#include "../util/settings.h"
#include "../util/MemBuffer.h"
#include "LCombo.h"
#include "LinkListBox.h"
#include "mapview.h"
#include "utilui.h"
#include "../util/utilio.h"
#include <windows.h>

/*
  propdata.sel0 = Current tile x
  propdata.sel1 = Current tile y
*/

enum CLICK_STATES
{
	CLICK_Default,	//just selects tile on map
	CLICK_MCSet1,	//sets x,y coords for map copy rect from1
	CLICK_MCSet2,	//sets x,y coords for map copy rect from2
	CLICK_MCSetT,	//sets x,y coords for map copy rect to
	CLICK_MMSet1,	//sets x,y coords for map move rect from1
	CLICK_MMSet2,	//sets x,y coords for map move rect from2
	CLICK_MMSetT	//sets x,y coords for map move rect to
} click_state = CLICK_Default;

const char *szMapTitle = "Map Editor";
const char *szMapStatus =
"Click \"Set...\" and then a point on the map to automatically fill values.";

#define NUM_SIZES 8
const char *sizes[NUM_SIZES] =
{
	"Tiny (120)", "Small (144)", "Medium (168)", "Normal (200)", "Large (220)", "Giant (240)", "Max (255)", "Ludks (480)"
};

#define NUM_ELEVS 7
const char *elevs[NUM_ELEVS] = { "0", "1", "2", "3", "4", "5", "6" };

const char *warningTileHasReachedLimit =
"Sorry, doing this will result in a tile having an elevation less than 0 or greater than 255.";
const char *warningExceededMaxSize =
"Sorry, you have exceeded the maximum allowed mapsize. I will now set the mapsize to maximum.";
const char *warningSensibleRect =
"Please make a sensible source rectangle. I fixed it for you this time, but\n"
"don\'t expect me to be as nice in the future...";
const char *warningMapCopyOverlap =
"Sorry, I had to cancel your map copy because the source and destination areas\n"
"overlapped. This may be supported in the future.";
const char *infoMapCopySuccess =
"Map copy successful. (As far as I know... as always, check for bugs!)";

BOOL Mapedit_Reset(HWND dialog)
{
	List_Clear(dialog, IDC_TR_ID);
	List_Clear(dialog, IDC_TR_AITYPE);
	List_Clear(dialog, IDC_TR_SIZE);
	List_Clear(dialog, IDC_TR_SIZE2);
	List_Clear(dialog, IDC_TR_ELEV);

	LinkListBox_Fill(GetDlgItem(dialog, IDC_TR_ID), esdata.terrains.head());

	LCombo_Fill(dialog, IDC_TR_AITYPE, esdata.aitypes.head());
	Combo_Fill(dialog, IDC_TR_SIZE, sizes, NUM_SIZES);
	Combo_Fill(dialog, IDC_TR_SIZE2, sizes, NUM_SIZES);
	Combo_Fill(dialog, IDC_TR_ELEV, elevs, NUM_ELEVS);

	return false;
}

void LoadMap(HWND dialog, bool all)
{
	Map::Terrain *tn = &scen.map.terrain[propdata.sel0][propdata.sel1];
	const Link *ait;
	int index = -1, i;

	SendDlgItemMessage(dialog, IDC_TR_ID, LB_SETCURSEL, tn->cnst, 0);

	if (!all)
		return;

	/* See if selected size is one of the built-ins. */
	for (i = 0; i < NUM_SIZES; i++)
	{
		if (scen.map.x == MapSizes[i])
		{
			SendDlgItemMessage(dialog, IDC_TR_SIZE, CB_SETCURSEL, i, 0);
			break;
		}
	}
	if (i == NUM_SIZES)
		SetDlgItemInt(dialog, IDC_TR_SIZE, scen.map.x, FALSE);
	for (i = 0; i < NUM_SIZES; i++)
	{
		if (scen.map.y == MapSizes[i])
		{
			SendDlgItemMessage(dialog, IDC_TR_SIZE2, CB_SETCURSEL, i, 0);
			break;
		}
	}
	if (i == NUM_SIZES)
		SetDlgItemInt(dialog, IDC_TR_SIZE2, scen.map.y, FALSE);

	SetDlgItemInt(dialog, IDC_TR_ELEV, tn->elev, FALSE);
	SetDlgItemInt(dialog, IDC_TR_CONST, tn->cnst, TRUE);

	SetDlgItemInt(dialog, IDC_TR_UNK1, scen.map.unknown1, true);
	SetDlgItemInt(dialog, IDC_TR_UNK2, scen.map.unknown2, true);
	SetDlgItemInt(dialog, IDC_TR_UNK3, scen.map.unknown3, true);
	SetDlgItemInt(dialog, IDC_TR_UNK4, scen.map.unknown4, true);

	ait = esdata.aitypes.getByIdSafe(scen.map.aitype);

	LCombo_Select(dialog, IDC_TR_AITYPE, ait);
}

void SaveMap(HWND dialog)
{
	int w,h;
	Map::Terrain *tn = scen.map.terrain[propdata.sel0] + propdata.sel1;
	unsigned long maxsize = MAX_MAPSIZE_OLD;

    if (isHD(scen.game))
		maxsize = MAX_MAPSIZE;

	//First check standard sizes. If that fails, get the custom size.
	if ((w = SendDlgItemMessage(dialog, IDC_TR_SIZE, CB_GETCURSEL, 0, 0)) != LB_ERR)
		scen.map.x = MapSizes[w];
	else
		scen.map.x = GetDlgItemInt(dialog, IDC_TR_SIZE, NULL, FALSE);

	if ((h = SendDlgItemMessage(dialog, IDC_TR_SIZE2, CB_GETCURSEL, 0, 0)) != LB_ERR)
		scen.map.y = MapSizes[h];
	else
		scen.map.y = GetDlgItemInt(dialog, IDC_TR_SIZE2, NULL, FALSE);

	if (scen.map.x > maxsize || scen.map.y > maxsize)
	{
	    if (scen.map.x > maxsize)
		    scen.map.x = maxsize;
	    if (scen.map.y > maxsize)
		    scen.map.y = maxsize;
		MessageBox(dialog, warningExceededMaxSize, szMapTitle, MB_ICONWARNING);
	}

	//scen.map.y = scen.map.x;	//maps are square

	scen.map.aitype = LCombo_GetSelId(dialog, IDC_TR_AITYPE);
    const Link * terrainlink = LinkListBox_GetSel(GetDlgItem(dialog, IDC_TR_ID));
    if (terrainlink != NULL)
	    tn->cnst = static_cast<char>(terrainlink->id());

	tn->elev = GetDlgItemInt(dialog, IDC_TR_ELEV, NULL, FALSE);

	scen.map.unknown1 = GetDlgItemInt(dialog, IDC_TR_UNK1, NULL, TRUE);
	scen.map.unknown2 = GetDlgItemInt(dialog, IDC_TR_UNK2, NULL, TRUE);
	scen.map.unknown3 = GetDlgItemInt(dialog, IDC_TR_UNK3, NULL, TRUE);
	scen.map.unknown4 = GetDlgItemInt(dialog, IDC_TR_UNK4, NULL, TRUE);
}

void Map_SaveTile(HWND dialog)
{
	Map::Terrain *tn = scen.map.terrain[propdata.sel0] + propdata.sel1;
	tn->cnst = static_cast<char>(LinkListBox_GetSel(GetDlgItem(dialog, IDC_TR_ID))->id());
	tn->elev = GetDlgItemInt(dialog, IDC_TR_ELEV, NULL, FALSE);
}

// called when a change is made to the coordinate textboxes
void Map_UpdatePos(HWND dialog, WORD idx, WORD idy)
{
	unsigned int xpos = GetDlgItemInt(dialog, idx, NULL, FALSE);
	SendMessage(propdata.mapview, MAP_UnhighlightPoint, MAP_UNHIGHLIGHT_ALL, 0);
	unsigned int ypos = GetDlgItemInt(dialog, idy, NULL, FALSE);
	SendMessage(propdata.mapview, MAP_UnhighlightPoint, MAP_UNHIGHLIGHT_ALL, 0);

    // unsigned so only need < comparison
	if (xpos < scen.map.x && ypos < scen.map.y && idx == IDC_TR_TX)
	{
		propdata.sel0 = xpos;
		propdata.sel1 = ypos;

		Map::Terrain *tn = scen.map.terrain[xpos] + ypos;
		SetDlgItemInt(dialog, IDC_TR_ELEV, tn->elev, FALSE);
		SendDlgItemMessage(dialog, IDC_TR_ID, LB_SETCURSEL, tn->cnst, 0);

		SendMessage(propdata.mapview, MAP_HighlightPoint, xpos, ypos);
		LoadMap(dialog, false);
	}
}

void Map_HandleMapClick(HWND dialog, short x, short y)
{
	int ctrlx, ctrly;
	switch (click_state)
	{
	case CLICK_Default:
	    {
		    ctrlx = IDC_TR_TX;
		    ctrly = IDC_TR_TY;

	        int len_lb_x = GetWindowTextLength(GetDlgItem(dialog, IDC_TR_MMX1));
	        int len_lb_y = GetWindowTextLength(GetDlgItem(dialog, IDC_TR_MMY1));
	        if (len_lb_x == 0 && len_lb_y == 0) {
	            SetDlgItemInt(dialog, IDC_TR_MMX1, x, FALSE);
	            SetDlgItemInt(dialog, IDC_TR_MMY1, y, FALSE);
	            break;
	        }

	        int len_tr_x = GetWindowTextLength(GetDlgItem(dialog, IDC_TR_MMX2));
	        int len_tr_y = GetWindowTextLength(GetDlgItem(dialog, IDC_TR_MMY2));
	        if (len_tr_x == 0 && len_tr_y == 0) {
	            SetDlgItemInt(dialog, IDC_TR_MMX2, x, FALSE);
	            SetDlgItemInt(dialog, IDC_TR_MMY2, y, FALSE);
	            break;
	        }

	        int len_to_x = GetWindowTextLength(GetDlgItem(dialog, IDC_TR_MMXT));
	        int len_to_y = GetWindowTextLength(GetDlgItem(dialog, IDC_TR_MMYT));
	        if (len_to_x == 0 && len_to_y == 0) {
	            SetDlgItemInt(dialog, IDC_TR_MMXT, x, FALSE);
	            SetDlgItemInt(dialog, IDC_TR_MMYT, y, FALSE);
	            break;
	        }
        }
		break;
	case CLICK_MMSet1:
		ctrlx = IDC_TR_MMX1;
		ctrly = IDC_TR_MMY1;
		click_state = CLICK_Default;
		Map_UpdatePos(dialog, IDC_TR_MMX1, IDC_TR_MMY1);
		break;
	case CLICK_MMSet2:
		ctrlx = IDC_TR_MMX2;
		ctrly = IDC_TR_MMY2;
		click_state = CLICK_Default;
		Map_UpdatePos(dialog, IDC_TR_MMX2, IDC_TR_MMY2);
		break;
	case CLICK_MMSetT:
		ctrlx = IDC_TR_MMXT;
		ctrly = IDC_TR_MMYT;
		click_state = CLICK_Default;
		Map_UpdatePos(dialog, IDC_TR_MMXT, IDC_TR_MMYT);
		break;
	default:
		return;
	}

	SetDlgItemInt(dialog, ctrlx, x, FALSE);
	SetDlgItemInt(dialog, ctrly, y, FALSE);
	Map::Terrain *tn = &scen.map.terrain[propdata.sel0][propdata.sel1];
	SetDlgItemInt(dialog, IDC_TR_ELEV, tn->elev, FALSE);
	SetDlgItemInt(dialog, IDC_TR_CONST, tn->cnst, TRUE);
}

void Map_HandleMapCopy(HWND dialog)
{
	RECT source;
	int space;
	long temp;
	MapCopyCache *cache = NULL;
	void *clipboardData;
	HGLOBAL clipboardMem;
	bool disp = false;

	if (!OpenClipboard(dialog))
	{
		MessageBox(dialog, errorOpenClipboard, szMapTitle, MB_ICONWARNING);
		return;
	}

	/* Get the source rect */
	source.left = GetDlgItemInt(dialog, IDC_TR_MMX1, NULL, FALSE);
	source.bottom = GetDlgItemInt(dialog, IDC_TR_MMY1, NULL, FALSE);	//top and bottom are "normal", reverse from GDI standard
	source.right = GetDlgItemInt(dialog, IDC_TR_MMX2, NULL, FALSE);
	source.top = GetDlgItemInt(dialog, IDC_TR_MMY2, NULL, FALSE);

	/* We need to make sure it's a sane rectangle. */
	if (source.left > source.right)
	{
		temp = source.left;
		source.left = source.right;
		source.right = temp;
		disp = true;
	}
	if (source.bottom > source.top)
	{
		temp = source.top;
		source.top = source.bottom;
		source.bottom = temp;
		disp = true;
	}
	if (disp)
	{
		MessageBox(dialog, warningSensibleRect, szMapTitle, MB_ICONWARNING);
	}

	EmptyClipboard();

	/* allocate the clipboard memory */
	space = scen.map_size(source, cache);
	clipboardMem = GlobalAlloc(GMEM_MOVEABLE, space);

	if (clipboardMem == NULL)
	{
		MessageBox(dialog, errorAllocFailed, szMapTitle, MB_ICONWARNING);
		goto Cleanup;
	}

	/* actually write the data */
	clipboardData = GlobalLock(clipboardMem);
	if (clipboardData)
	{
		MemBuffer data((char*)clipboardData, space);
		scen.map_copy(data, cache);
		cache = NULL;
	}
	GlobalUnlock(clipboardMem);

	if (!SetClipboardData(propdata.mcformat, clipboardMem))
		MessageBox(dialog, errorSetClipboard, szMapTitle, MB_ICONWARNING);

	EnableWindow(GetDlgItem(dialog, IDC_TR_MCPASTE), true);

Cleanup:
	CloseClipboard();
}

void Map_HandleMapPaste(HWND dialog)
{
	HGLOBAL cbMem;
	POINT target;
	void *cbData;
	size_t size;

	target.x = GetDlgItemInt(dialog, IDC_TR_MMXT, NULL, FALSE);
	target.y = GetDlgItemInt(dialog, IDC_TR_MMYT, NULL, FALSE);

	/*	Check whether map copy data is actually available. (Should be if we've
		gotten this far.
	*/
	if (!IsClipboardFormatAvailable(propdata.mcformat))
	{
		MessageBox(dialog, warningNoFormat, szMapTitle, MB_ICONWARNING);
		return;
	}

	if (!OpenClipboard(dialog))
		MessageBox(dialog, errorOpenClipboard, szMapTitle, MB_ICONWARNING);

	cbMem = GetClipboardData(propdata.mcformat);
	size = GlobalSize(cbMem);
	cbData = GlobalLock(cbMem);
	if (cbData)
	{
		MemBuffer b((char*)cbData, size);

		scen.map_paste(target, b);
	}
	else
		MessageBox(dialog, "Could not paste clipboard data.",
			szMapTitle, MB_ICONWARNING);
	GlobalUnlock(cbMem);

	CloseClipboard();

	SendMessage(propdata.mapview, MAP_Reset, 0, 0);
}

void Map_HandleMapDelete(HWND dialog, OpFlags::Value flags=OpFlags::ALL)
{
	bool disp = false;
	RECT source;
	POINT target;
	long temp;

	/* Get the source rect */
	source.left = GetDlgItemInt(dialog, IDC_TR_MMX1, NULL, FALSE);
	source.bottom = GetDlgItemInt(dialog, IDC_TR_MMY1, NULL, FALSE);	//top and bottom are "normal", reverse from GDI standard
	source.right = GetDlgItemInt(dialog, IDC_TR_MMX2, NULL, FALSE);
	source.top = GetDlgItemInt(dialog, IDC_TR_MMY2, NULL, FALSE);

	/* Get the target point */
	target.x = GetDlgItemInt(dialog, IDC_TR_MMXT, NULL, FALSE);
	target.y = GetDlgItemInt(dialog, IDC_TR_MMYT, NULL, FALSE);

	/* We need to make sure it's a sane rectangle. */
	if (source.left > source.right)
	{
		temp = source.left;
		source.left = source.right;
		source.right = temp;
		disp = true;
	}
	if (source.bottom > source.top)
	{
		temp = source.top;
		source.top = source.bottom;
		source.bottom = temp;
		disp = true;
	}
	if (disp) {
		MessageBox(dialog, warningSensibleRect, szMapTitle, MB_ICONWARNING);
	} else {
	    scen.map_delete(source, target, flags);
	}

	SendMessage(propdata.mapview, MAP_Reset, 0, 0);
}

void Map_HandleMapChangeElevation(HWND dialog, int adjustment)
{
	bool disp = false;
	RECT target;
	long temp;

	/* Get the target rect */
	target.left = GetDlgItemInt(dialog, IDC_TR_MMX1, NULL, FALSE);
	target.bottom = GetDlgItemInt(dialog, IDC_TR_MMY1, NULL, FALSE);	//top and bottom are "normal", reverse from GDI standard
	target.right = GetDlgItemInt(dialog, IDC_TR_MMX2, NULL, FALSE);
	target.top = GetDlgItemInt(dialog, IDC_TR_MMY2, NULL, FALSE);

	/* We need to make sure it's a sane rectangle. */
	if (target.left > target.right)
	{
		temp = target.left;
		target.left = target.right;
		target.right = temp;
		disp = true;
	}
	if (target.bottom > target.top)
	{
		temp = target.top;
		target.top = target.bottom;
		target.bottom = temp;
		disp = true;
	}

	// check no tiles will go beyond bounds
	bool isok = true;
	int tempelev = 0;
	for (LONG i = target.left; i <= target.right; i++) {
		for (LONG j = target.bottom; j <= target.top; j++) {
		    tempelev = scen.map.terrain[i][j].elev + adjustment;
		    if (tempelev < 0 || tempelev > 255)
		        isok = false;
		}
	}

	if (disp) {
		MessageBox(dialog, warningSensibleRect, szMapTitle, MB_ICONWARNING);
	} else if (!isok) {
		MessageBox(dialog, warningTileHasReachedLimit, szMapTitle, MB_ICONWARNING);
	} else {
	    scen.map_change_elevation(target, adjustment);

	    Map::Terrain *tn = &scen.map.terrain[propdata.sel0][propdata.sel1];
	    SetDlgItemInt(dialog, IDC_TR_ELEV, tn->elev, FALSE);

	    SendMessage(propdata.mapview, MAP_Reset, 0, 0);
	}
}

void Map_HandleMapRepeat(HWND dialog, OpFlags::Value flags=OpFlags::ALL)
{
	bool disp = false;
	RECT target;
	POINT source;
	long temp;

	/* Get the target rect */
	target.left = GetDlgItemInt(dialog, IDC_TR_MMX1, NULL, FALSE);
	target.bottom = GetDlgItemInt(dialog, IDC_TR_MMY1, NULL, FALSE);	//top and bottom are "normal", reverse from GDI standard
	target.right = GetDlgItemInt(dialog, IDC_TR_MMX2, NULL, FALSE);
	target.top = GetDlgItemInt(dialog, IDC_TR_MMY2, NULL, FALSE);

	/* Get the source point */
	source.x = GetDlgItemInt(dialog, IDC_TR_MMXT, NULL, FALSE);
	source.y = GetDlgItemInt(dialog, IDC_TR_MMYT, NULL, FALSE);

	/* We need to make sure it's a sane rectangle. */
	if (target.left > target.right)
	{
		temp = target.left;
		target.left = target.right;
		target.right = temp;
		disp = true;
	}
	if (target.bottom > target.top)
	{
		temp = target.top;
		target.top = target.bottom;
		target.bottom = temp;
		disp = true;
	}

	if (disp) {
		MessageBox(dialog, warningSensibleRect, szMapTitle, MB_ICONWARNING);
	} else {
	    scen.map_repeat(target, source, flags);

	    Map::Terrain *tn = &scen.map.terrain[propdata.sel0][propdata.sel1];
	    SetDlgItemInt(dialog, IDC_TR_ELEV, tn->elev, FALSE);
	    SetDlgItemInt(dialog, IDC_TR_CONST, tn->cnst, TRUE);

	    SendMessage(propdata.mapview, MAP_Reset, 0, 0);
	}
}

void Map_HandleMapMove(HWND dialog, OpFlags::Value flags=OpFlags::ALL)
{
	bool disp = false;
	RECT source;
	POINT target;
	long temp;

	/* Get the source rect */
	source.left = GetDlgItemInt(dialog, IDC_TR_MMX1, NULL, FALSE);
	source.bottom = GetDlgItemInt(dialog, IDC_TR_MMY1, NULL, FALSE);	//top and bottom are "normal", reverse from GDI standard
	source.right = GetDlgItemInt(dialog, IDC_TR_MMX2, NULL, FALSE);
	source.top = GetDlgItemInt(dialog, IDC_TR_MMY2, NULL, FALSE);

	/* Get the target point */
	target.x = GetDlgItemInt(dialog, IDC_TR_MMXT, NULL, FALSE);
	target.y = GetDlgItemInt(dialog, IDC_TR_MMYT, NULL, FALSE);

	/* We need to make sure it's a sane rectangle. */
	if (source.left > source.right)
	{
		temp = source.left;
		source.left = source.right;
		source.right = temp;
		disp = true;
	}
	if (source.bottom > source.top)
	{
		temp = source.top;
		source.top = source.bottom;
		source.bottom = temp;
		disp = true;
	}
	if (disp) {
		MessageBox(dialog, warningSensibleRect, szMapTitle, MB_ICONWARNING);
	} else {
	    scen.map_move(source, target, flags);
	}

	SendMessage(propdata.mapview, MAP_Reset, 0, 0);
}

void Map_HandleMapSwap(HWND dialog, OpFlags::Value flags=OpFlags::ALL)
{
	bool disp = false;
	RECT source;
	POINT target;
	long temp;

	/* Get the source rect */
	source.left = GetDlgItemInt(dialog, IDC_TR_MMX1, NULL, FALSE);
	source.bottom = GetDlgItemInt(dialog, IDC_TR_MMY1, NULL, FALSE);	//top and bottom are "normal", reverse from GDI standard
	source.right = GetDlgItemInt(dialog, IDC_TR_MMX2, NULL, FALSE);
	source.top = GetDlgItemInt(dialog, IDC_TR_MMY2, NULL, FALSE);

	/* Get the target point */
	target.x = GetDlgItemInt(dialog, IDC_TR_MMXT, NULL, FALSE);
	target.y = GetDlgItemInt(dialog, IDC_TR_MMYT, NULL, FALSE);

	/* We need to make sure it's a sane rectangle. */
	if (source.left > source.right)
	{
		temp = source.left;
		source.left = source.right;
		source.right = temp;
		disp = true;
	}
	if (source.bottom > source.top)
	{
		temp = source.top;
		source.top = source.bottom;
		source.bottom = temp;
		disp = true;
	}
	if (disp) {
		MessageBox(dialog, warningSensibleRect, szMapTitle, MB_ICONWARNING);
	} else {
	    scen.map_swap(source, target, flags);
	}

	SendMessage(propdata.mapview, MAP_Reset, 0, 0);
}

void Map_HandleMapDuplicate(HWND dialog, OpFlags::Value flags=OpFlags::ALL)
{
	bool disp = false;
	RECT source;
	POINT target;
	long temp;

	/* Get the source rect */
	source.left = GetDlgItemInt(dialog, IDC_TR_MMX1, NULL, FALSE);
	source.bottom = GetDlgItemInt(dialog, IDC_TR_MMY1, NULL, FALSE);	//top and bottom are "normal", reverse from GDI standard
	source.right = GetDlgItemInt(dialog, IDC_TR_MMX2, NULL, FALSE);
	source.top = GetDlgItemInt(dialog, IDC_TR_MMY2, NULL, FALSE);

	/* Get the target point */
	target.x = GetDlgItemInt(dialog, IDC_TR_MMXT, NULL, FALSE);
	target.y = GetDlgItemInt(dialog, IDC_TR_MMYT, NULL, FALSE);

	/* We need to make sure it's a sane rectangle. */
	if (source.left > source.right)
	{
		temp = source.left;
		source.left = source.right;
		source.right = temp;
		disp = true;
	}
	if (source.bottom > source.top)
	{
		temp = source.top;
		source.top = source.bottom;
		source.bottom = temp;
		disp = true;
	}
	if (disp) {
		MessageBox(dialog, warningSensibleRect, szMapTitle, MB_ICONWARNING);
	} else {
	    scen.map_duplicate(source, target, flags);
	}

	SendMessage(propdata.mapview, MAP_Reset, 0, 0);
}

void Map_HandleOutline(HWND dialog)
{
	unsigned int xpos = GetDlgItemInt(dialog, IDC_TR_TX, NULL, FALSE);
	unsigned int ypos = GetDlgItemInt(dialog, IDC_TR_TY, NULL, FALSE);

    scen.outline(xpos, ypos, static_cast<char>(LinkListBox_GetSel(GetDlgItem(dialog, IDC_TR_ID))->id()),
            scen.map.terrain[xpos][ypos].cnst);

	SendMessage(propdata.mapview, MAP_Reset, 0, 0);
}

void Map_HandleSwapTerrainTypes(HWND dialog)
{
	unsigned int xpos = GetDlgItemInt(dialog, IDC_TR_TX, NULL, FALSE);
	unsigned int ypos = GetDlgItemInt(dialog, IDC_TR_TY, NULL, FALSE);

    scen.swapTerrain(static_cast<char>(LinkListBox_GetSel(GetDlgItem(dialog, IDC_TR_ID))->id()),
            scen.map.terrain[xpos][ypos].cnst);

	SendMessage(propdata.mapview, MAP_Reset, 0, 0);
}

void Map_HandleStrictOutline(HWND dialog)
{
	unsigned int xpos = GetDlgItemInt(dialog, IDC_TR_TX, NULL, FALSE);
	unsigned int ypos = GetDlgItemInt(dialog, IDC_TR_TY, NULL, FALSE);

    scen.outline(xpos, ypos, static_cast<char>(LinkListBox_GetSel(GetDlgItem(dialog, IDC_TR_ID))->id()),
            scen.map.terrain[xpos][ypos].cnst, TerrainFlags::FORCE);

	SendMessage(propdata.mapview, MAP_Reset, 0, 0);
}

void Map_HandleOutlineEight(HWND dialog)
{
	unsigned int xpos = GetDlgItemInt(dialog, IDC_TR_TX, NULL, FALSE);
	unsigned int ypos = GetDlgItemInt(dialog, IDC_TR_TY, NULL, FALSE);

    scen.outline(xpos, ypos, static_cast<char>(LinkListBox_GetSel(GetDlgItem(dialog, IDC_TR_ID))->id()),
            scen.map.terrain[xpos][ypos].cnst, TerrainFlags::EIGHT);

	SendMessage(propdata.mapview, MAP_Reset, 0, 0);
}

void Map_HandleStrictOutlineEight(HWND dialog)
{
	unsigned int xpos = GetDlgItemInt(dialog, IDC_TR_TX, NULL, FALSE);
	unsigned int ypos = GetDlgItemInt(dialog, IDC_TR_TY, NULL, FALSE);

    scen.outline(xpos, ypos, static_cast<char>(LinkListBox_GetSel(GetDlgItem(dialog, IDC_TR_ID))->id()),
            scen.map.terrain[xpos][ypos].cnst, (TerrainFlags::Value)(TerrainFlags::FORCE | TerrainFlags::EIGHT));

	SendMessage(propdata.mapview, MAP_Reset, 0, 0);
}

void Map_HandleFloodFill(HWND dialog)
{
	unsigned int xpos = GetDlgItemInt(dialog, IDC_TR_TX, NULL, FALSE);
	unsigned int ypos = GetDlgItemInt(dialog, IDC_TR_TY, NULL, FALSE);

    scen.floodFill4(xpos, ypos, static_cast<char>(LinkListBox_GetSel(GetDlgItem(dialog, IDC_TR_ID))->id()),
            scen.map.terrain[xpos][ypos].cnst);

	SendMessage(propdata.mapview, MAP_Reset, 0, 0);
}

void Map_HandleFloodFillElev(HWND dialog)
{
	unsigned int xpos = GetDlgItemInt(dialog, IDC_TR_TX, NULL, FALSE);
	unsigned int ypos = GetDlgItemInt(dialog, IDC_TR_TY, NULL, FALSE);

    scen.floodFillElev4(xpos, ypos, GetDlgItemInt(dialog, IDC_TR_ELEV, NULL, FALSE),
            scen.map.terrain[xpos][ypos].cnst);

	SendMessage(propdata.mapview, MAP_Reset, 0, 0);
}

void Map_HandleNormalizeElevation(HWND dialog)
{
	unsigned int xpos = GetDlgItemInt(dialog, IDC_TR_TX, NULL, FALSE);
	unsigned int ypos = GetDlgItemInt(dialog, IDC_TR_TY, NULL, FALSE);
    int xpos_temp = 0;
    int ypos_temp = 0;
    Map::Terrain * tn = scen.map.terrain[xpos] + ypos; // this works
    RECT search;
    search.left = xpos - 1;
    search.top = ypos - 1;
    search.right = xpos + 1;
    search.bottom = ypos + 1;

    int segmentlen = 2; // sidelength - 1 = 3 - 1, initially
	char elev = tn->elev;
    int nincreased = 0;
    int ndecreased = 0;
    bool searchagain = true;
    int tier = 1;
    while (searchagain && (search.top >= 0 || search.bottom < (LONG)scen.map.x || search.left >= 0 || search.right < (LONG)scen.map.y)) { // this works
        nincreased = 0;
        ndecreased = 0;
        int squarestocheck = 4*segmentlen;
        // search the perimeter of the RECT
        for (int i = 0; i < squarestocheck; i++) {
            if (i < segmentlen) {
                xpos_temp = search.left + i;
                ypos_temp = search.top;
            } else if (i < segmentlen * 2) {
                xpos_temp = search.left + segmentlen;
                ypos_temp = search.top + i - segmentlen;
            } else if (i < segmentlen * 3) {
                xpos_temp = search.left - i + 3 * segmentlen;
                ypos_temp = search.top + segmentlen;
            } else {
                xpos_temp = search.left;
                ypos_temp = search.top - i + 4 * segmentlen;
            }

            // level out the terrain
            if (ypos_temp >= 0 || ypos_temp < (LONG)scen.map.x || xpos_temp >= 0 || xpos_temp < (LONG)scen.map.y) { // works
	            tn = scen.map.terrain[xpos_temp] + ypos_temp;
                if (elev - tn->elev < -tier) {
                    tn->elev = elev + tier;
                    nincreased++;
                } else if (elev - tn->elev > tier) {
                    tn->elev = elev - tier;
                    ndecreased++;
                }
            }
        }
        search.top--;
        search.bottom++;
        search.left--;
        search.right++;
        segmentlen+=2;

        // search again if able to
        if (ndecreased == squarestocheck) {
            searchagain = true;
        } else if (nincreased == squarestocheck) {
            searchagain = true;
        } else {
            searchagain = false;
        }
        tier++;
    }

	SendMessage(propdata.mapview, MAP_Reset, 0, 0);
}

void Map_HandleMapScale(HWND dialog)
{
}

void Map_HandleSetFocus(HWND, WORD)
{
	EnableMenuItem(propdata.menu, ID_TS_EDIT_COPY, MF_ENABLED);
	EnableMenuItem(propdata.menu, ID_TS_EDIT_CUT, MF_ENABLED);
	if (IsClipboardFormatAvailable(CF_TEXT))
		EnableMenuItem(propdata.menu, ID_TS_EDIT_PASTE, MF_ENABLED);
}

void Map_ShowOperationsCoords(HWND dialog, WORD id)
{
    HBRUSH bWhite;
    bWhite = CreateSolidBrush(0xFFFFFF);

    RECT area;
    area.left = GetDlgItemInt(dialog, IDC_TR_MMX1, NULL, FALSE);
    area.bottom = GetDlgItemInt(dialog, IDC_TR_MMY1, NULL, FALSE);
    area.right = GetDlgItemInt(dialog, IDC_TR_MMX2, NULL, FALSE);
    area.top = GetDlgItemInt(dialog, IDC_TR_MMY2, NULL, FALSE);
	SendMessage(propdata.mapview, MAP_UnhighlightPoint, MAP_UNHIGHLIGHT_ALL, 0);
	SendMessage(propdata.mapview, MAP_HighlightPoint, area.left, area.bottom);
	SendMessage(propdata.mapview, MAP_HighlightPoint, area.right, area.top);
	SendMessage(propdata.mapview, MAP_HighlightPoint,
		GetDlgItemInt(dialog, IDC_TR_MMXT, NULL, FALSE),
		GetDlgItemInt(dialog, IDC_TR_MMYT, NULL, FALSE));

	switch (id)
	{
	case IDC_TR_MMX1:
	case IDC_TR_MMY1:
	case IDC_TR_MMX2:
	case IDC_TR_MMY2:
	case IDC_TR_MMXT:
	case IDC_TR_MMYT:
	    //FrameRect(scen.data.copydc, &area, bWhite);
	    //mapdata.opArea.left=-1;
	    break;
	}
}

void Map_HandleKillFocus(HWND dialog, WORD id)
{
	EnableMenuItem(propdata.menu, ID_TS_EDIT_COPY, MF_GRAYED);
	EnableMenuItem(propdata.menu, ID_TS_EDIT_CUT, MF_GRAYED);
	EnableMenuItem(propdata.menu, ID_TS_EDIT_PASTE, MF_GRAYED);

	switch (id)
	{
	case IDC_TR_MMX1:
	case IDC_TR_MMY1:
	case IDC_TR_MMX2:
	case IDC_TR_MMY2:
	case IDC_TR_MMXT:
	case IDC_TR_MMYT:
		SendMessage(propdata.mapview, MAP_UnhighlightPoint, MAP_UNHIGHLIGHT_ALL, 0);
	    switch (id)
	    {
	    case IDC_TR_MMX1:
	    case IDC_TR_MMY1:
		    SendMessage(propdata.mapview, MAP_HighlightPoint,
			            GetDlgItemInt(dialog, IDC_TR_MMX1, NULL, FALSE),
			            GetDlgItemInt(dialog, IDC_TR_MMY1, NULL, FALSE));
			break;
	    case IDC_TR_MMX2:
	    case IDC_TR_MMY2:
		    SendMessage(propdata.mapview, MAP_HighlightPoint,
			            GetDlgItemInt(dialog, IDC_TR_MMX2, NULL, FALSE),
			            GetDlgItemInt(dialog, IDC_TR_MMY2, NULL, FALSE));
			break;
	    case IDC_TR_MMXT:
	    case IDC_TR_MMYT:
		    SendMessage(propdata.mapview, MAP_HighlightPoint,
			            GetDlgItemInt(dialog, IDC_TR_MMXT, NULL, FALSE),
			            GetDlgItemInt(dialog, IDC_TR_MMYT, NULL, FALSE));
			break;
		}
		break;
	}
}

void Map_HandleCommand(HWND dialog, WORD code, WORD id, HWND)
{
	Map::Terrain *tn = &scen.map.terrain[propdata.sel0][propdata.sel1];
	switch (code)
	{
	case LBN_SELCHANGE:
		switch (id)
		{
		case IDC_TR_ID:
	        SetDlgItemInt(dialog, IDC_TR_CONST, SendMessage(GetDlgItem(dialog, IDC_TR_ID), LB_GETCURSEL, 0, 0), TRUE);
		    //SetWindowText(propdata.statusbar, "Selection changed");
		    break;

		}

		break;

	case EN_CHANGE:
		switch (id)
		{
		case IDC_TR_TX:
		case IDC_TR_TY:
			Map_UpdatePos(dialog, IDC_TR_TX, IDC_TR_TY);
			break;
		case IDC_TR_MMX1:
		case IDC_TR_MMY1:
		case IDC_TR_MMX2:
		case IDC_TR_MMY2:
		case IDC_TR_MMXT:
		case IDC_TR_MMYT:
			//Map_ShowOperationsCoords(dialog, id);
			break;

        //this will cause recursive updating. need a save button
		//case IDC_TR_ID:
		//case IDC_TR_ELEV:
		//    Map_SaveTile(dialog);
		//	break;

		case ID_TS_EDIT_COPY:
			SendMessage(GetFocus(), WM_COPY, 0, 0);
			break;

		case ID_TS_EDIT_CUT:
			SendMessage(GetFocus(), WM_CUT, 0, 0);
			break;

		case ID_TS_EDIT_PASTE:
			SendMessage(GetFocus(), WM_PASTE, 0, 0);
			break;

		case IDC_TR_SIZE:
		    {
			    //setts.editall
		        // make SIZE2 = SIZE
		        int h = SendDlgItemMessage(dialog, IDC_TR_SIZE, CB_GETCURSEL, 0, 0);
	            if (h != LB_ERR) {
	                SendDlgItemMessage(dialog, IDC_TR_SIZE2, CB_SETCURSEL, h, 0);
                    printf_log("changing to %d\n", h);
	            } else {
	                char buffer[10];
	                GetDlgItemText(dialog, IDC_TR_SIZE, buffer, 10);
		            SetDlgItemText(dialog, IDC_TR_SIZE2, buffer);
                    printf_log("changing to %s\n", buffer);
		        }
			}
			break;

		case IDC_TR_SIZE2:
            printf_log("testing size 2\n");
		    break;

		}
		break;

	case BN_CLICKED:
		switch (id)
		{
		case IDC_TR_CLEAR:
		    SetDlgItemText(dialog, IDC_TR_MMX1, "");
		    SetDlgItemText(dialog, IDC_TR_MMY1, "");
		    SetDlgItemText(dialog, IDC_TR_MMX2, "");
		    SetDlgItemText(dialog, IDC_TR_MMY2, "");
		    SetDlgItemText(dialog, IDC_TR_MMXT, "");
		    SetDlgItemText(dialog, IDC_TR_MMYT, "");
			break;

		case IDC_TR_MMSET1:
			click_state = CLICK_MMSet1;
			break;

		case IDC_TR_MMSET2:
			click_state = CLICK_MMSet2;
			break;

		case IDC_TR_MMSETT:
			click_state = CLICK_MMSetT;
			break;

		case IDC_TR_MCCOPY:
			Map_HandleMapCopy(dialog);
			break;

		case IDC_TR_MCPASTE:
			Map_HandleMapPaste(dialog);
			break;

		case IDC_TR_RMUNITS:
			Map_HandleMapDelete(dialog, OpFlags::UNITS);
			break;

		case IDC_TR_RMTRIGS:
			Map_HandleMapDelete(dialog, OpFlags::TRIGGERS);
			break;

		case IDC_TR_MMMOVE:
			Map_HandleMapMove(dialog, OpFlags::ALL);
			break;

		case IDC_TR_MOVE_TERRAIN:
			Map_HandleMapMove(dialog, OpFlags::TERRAIN);
			break;

		case IDC_TR_MOVE_UNITS:
			Map_HandleMapMove(dialog, OpFlags::UNITS);
			break;

		case IDC_TR_MOVE_ELEVATION:
			Map_HandleMapMove(dialog, OpFlags::ELEVATION);
			break;

		case IDC_TR_MOVE_TRIGGERS:
			Map_HandleMapMove(dialog, OpFlags::TRIGGERS);
			break;

		case IDC_TR_REPEAT:
			Map_HandleMapRepeat(dialog, OpFlags::ALL);
			break;

		case IDC_TR_REPEAT_UNITS:
			Map_HandleMapRepeat(dialog, OpFlags::UNITS);
			break;

		case IDC_TR_REPEAT_TERRAIN:
			Map_HandleMapRepeat(dialog, OpFlags::TERRAIN);
			break;

		case IDC_TR_REPEAT_ELEV:
			Map_HandleMapRepeat(dialog, OpFlags::ELEVATION);
			break;

		case IDC_TR_RAISE_ELEV:
			Map_HandleMapChangeElevation(dialog, 1);
			break;

		case IDC_TR_LOWER_ELEV:
			Map_HandleMapChangeElevation(dialog, -1);
			break;

		case IDC_TR_MMSWAP:
			Map_HandleMapSwap(dialog, OpFlags::ALL);
			break;

		case IDC_TR_SWAP_TERRAIN:
			Map_HandleMapSwap(dialog, OpFlags::TERRAIN);
			break;

		case IDC_TR_SWAP_UNITS:
			Map_HandleMapSwap(dialog, OpFlags::UNITS);
			break;

		case IDC_TR_SWAP_ELEVATION:
			Map_HandleMapSwap(dialog, OpFlags::ELEVATION);
			break;

		case IDC_TR_SWAP_TRIGGERS:
			Map_HandleMapSwap(dialog, OpFlags::TRIGGERS);
			break;

		case IDC_TR_MDUPE:
			Map_HandleMapDuplicate(dialog, OpFlags::ELEVATION);
			break;

		case IDC_TR_MDUPT:
			Map_HandleMapDuplicate(dialog, OpFlags::TERRAIN);
			break;

		case IDC_TR_MDUPU:
			Map_HandleMapDuplicate(dialog, OpFlags::UNITS);
			break;

		case IDC_TR_SCALE:
			Map_HandleMapScale(dialog);
			break;

		case IDC_TR_SAVETILE:
			Map_SaveTile(dialog);
		    SetWindowText(propdata.statusbar, "Tile saved");
			break;

		case IDC_TR_NORMALIZE_ELEV:
		    Map_SaveTile(dialog);
			Map_HandleNormalizeElevation(dialog);
		    SetWindowText(propdata.statusbar, "Elevation of tile normalized");
			break;

		case IDC_TR_FLOOD:
			Map_HandleFloodFill(dialog);
		    SetWindowText(propdata.statusbar, "Filled terrain");
			break;

		case IDC_TR_FLOOD_ELEV:
			Map_HandleFloodFillElev(dialog);
		    SetWindowText(propdata.statusbar, "Filled elevation");
			break;

		case IDC_TR_OUTLINE:
			Map_HandleOutline(dialog);
		    SetWindowText(propdata.statusbar, "Outlined terrain");
			break;

		case IDC_TR_OUTLINE_FORCE:
			Map_HandleStrictOutline(dialog);
		    SetWindowText(propdata.statusbar, "Outlined terrain");
			break;

		case IDC_TR_OUTLINE_EIGHT:
			Map_HandleOutlineEight(dialog);
		    SetWindowText(propdata.statusbar, "Outlined terrain");
			break;

		case IDC_TR_OUTLINE_FORCE_EIGHT:
			Map_HandleStrictOutlineEight(dialog);
		    SetWindowText(propdata.statusbar, "Outlined terrain");
			break;

		case IDC_TR_SWAP_TERRAIN_TYPES:
			Map_HandleSwapTerrainTypes(dialog);
		    SetWindowText(propdata.statusbar, "Swapped terrain types");
			break;
		}

	case EN_SETFOCUS:
		Map_HandleSetFocus(dialog, id);
		break;

	case EN_KILLFOCUS:
		Map_HandleKillFocus(dialog, id);
		break;

    default:
		switch (id)
		{
		}

	}
}

void Map_Reset(HWND dialog)
{
	propdata.sel0 = -1;
	propdata.sel1 = -1;
	SetDlgItemInt(dialog, IDC_TR_TX, 0, FALSE);
	SetDlgItemInt(dialog, IDC_TR_TY, 0, FALSE);
	LoadMap(dialog, true);
	SetWindowText(propdata.statusbar, szMapStatus);
}

INT_PTR CALLBACK MapDlgProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ret = FALSE;	//default: doesn't process message

	try
	{
		switch (msg)
		{
		case WM_INITDIALOG:
			Mapedit_Reset(dialog);
			Map_Reset(dialog);
			ret = TRUE;
			break;

		case WM_COMMAND:
			Map_HandleCommand(
					dialog, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
			break;

		case WM_NOTIFY:
			{
				NMHDR *header = (NMHDR*)lParam;
				switch (header->code)
				{
					case PSN_SETACTIVE:
						Map_Reset(dialog);
						break;
					case PSN_KILLACTIVE:
						SaveMap(dialog);
						break;
				}

				ret = TRUE;
			}
			break;

		case MAP_Click:
			Map_HandleMapClick(dialog, LOWORD(lParam), HIWORD(lParam));
			break;

		case AOKTS_Loading:
			Mapedit_Reset(dialog);
			Map_Reset(dialog);
			break;

		case AOKTS_Saving:
			SaveMap(dialog);
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
