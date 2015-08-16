/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION.
	mapview.h -- Graphical representation of AOK maps

	VIEW/CONTROLLER
**/

#define WIN32_LEAN_AND_MEAN
#define OEMRESOURCE	//OCR_NORMAL
#include <windows.h>

extern class Scenario scen;

const char MapClass[] = "AOKTSMapView";

/**
 * Creates a mapview window with specified owner and at specified position. The
 * window will be left hidden; it is the caller's responsibility to call
 * ShowWindow().
 */
HWND CreateMapView(HWND owner, int x, int y, class Scenario * scenario);

enum Map_Msgs
{
	/* In */
	MAP_Close = WM_APP + 0x150,
	MAP_Recreate,
	MAP_ChangeDrawOptions,
	MAP_HighlightPoint,	//x, y
	MAP_UnhighlightPoint, //x, y (see MAP_UNHIGHLIGHT_ALL below)
	MAP_Reset,
	/* wParam: non-zero if the mapview should resize to fit the current scen
	 * lParam: unused.
	 */

	/* Out */
	MAP_Click = WM_APP + 0x200
	/*	wParam: unused
		lParam: MAKELPARAM(x, y) //map coordinates
		*/
};

#define MAP_UNHIGHLIGHT_ALL 0xFFFFFFFF

inline void MapView_Reset(HWND mapview, bool resize = false)
{
	SendMessageW(mapview, MAP_Reset, resize, 0);
}
