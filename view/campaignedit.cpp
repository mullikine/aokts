/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION
	compatedit.cpp -- Defines functions for Campaign page.

	VIEW/CONTROLLER
**/

#include "editors.h"
#include "../res/resource.h"
#include "../util/settings.h"
#include "utilui.h"
#include <commdlg.h>

/* Campaign */

/**
 * Handles a WM_COMMAND message sent to the dialog.
 */
INT_PTR Campaign_HandleCommand(HWND dialog, WORD code, WORD id, HWND)
{
	HWND treeview = GetDlgItem(dialog, IDC_T_TREE);	//all use this

	switch (code)
	{
	case BN_CLICKED:
	case 1:
		switch (id)
		{

		}
	}

	// "If an application processes this message, it should return zero."
	return 0;
}

INT_PTR CALLBACK CampaignDlgProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR ret = FALSE;

	try
	{
	switch (msg)
	{
		case WM_INITDIALOG:
			return TRUE;

		case WM_COMMAND:
			return Campaign_HandleCommand(
					dialog, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);

		case WM_NOTIFY:
			{
				NMHDR *header = (NMHDR*)lParam;
				switch (header->code)
				{
				case PSN_SETACTIVE:
					break;

				case PSN_KILLACTIVE:
					break;
				}
			}
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
