/**
	AOK Trigger Studio (See aokts.cpp for legal conditions.)
	WINDOWS VERSION
	trigedit.cpp -- Defines functions for trigger editor.

	Note: All functions beginning with 'TrigTree_' relate to the Trigger display Tree-View.

	VIEW/CONTROLLER
**/

#define OEMRESOURCE

#include "../model/scen.h"

#include "editors.h"

#include "../util/utilio.h"
#include "../util/helper.h"
#include "../util/MemBuffer.h"
#include "../util/NullBuffer.h"
#include "../util/settings.h"
#include "../model/ChangePlayerVisitor.h"
#include "../res/resource.h"
#include "ecedit.h"
#include "utilui.h"
#include "treeutil.h"
#include "mapview.h"

#include <stdio.h>	//sprintf()
#include <windowsx.h>	//for GET_X_LPARAM, GET_Y_LPARAM
#include <stdexcept>
#include <algorithm>

extern Scenario scen;

using std::vector;

/*
	Triggers

	TVITEM.lParam = MAKELONG(ECType, index)	//index is to scen.triggers, not t_order

  TODO: Export triggers in text format.
*/

/* Internal Global Variables */

/**
 * Stores the original Tree-View WndProc.
 */
WNDPROC TVWndProc = NULL;
HFONT treefont;

bool bysource = true;

/*	c_trig: pointer to current trigger.
	Set this to NULL to prevent saving of data on TVN_SELCHANGED.
*/
Trigger *c_trig;

/*	dragging: stores whether a drag operation is in progress */
HTREEITEM dragging = NULL;
POINT lastCursorPos = { -1, -1 };

enum Timers
{
	TT_Scroll
};

#define DRAG_DELAY 200

/* Strings! */

const char szTrigTitle[] = "Trigger Editor";

const char noticeNoDelete[] =
"Sorry, you may not delete a trigger while effect or condition editors are open. "
"Deletion cancelled.";
const char noteNoTrigInTrig[] =
"Sorry mate, can\'t place a trigger inside another trigger.";
const char help_msg[] =
"Single-click a trigger's name to edit it. Press Delete to delete a trigger.";
const char errorEditorsOpen[] =
"Sorry, you may not switch tabs with open effects or conditions. Maybe in a later version.";
const char warningEditorsClosing[] =
"Warning: Switching tabs will cancel all open effects or conditions. Continue Anyway?";
const char errorNoEditTrig[] = //removed
"Error: No condition or effect selected. I honestly don\'t know how you clicked that.";
const char warningNoSelection[] = //removed
"Warning: I could not locate the current selection in the Tree-View, and thus "
"cancelled any pending operation. I may now be unstable, so it is recommended that "
"you restart me after saving the scenario to a different file.";
const char warningInconsistentPlayer[] =
"Warning: This trigger appears to have conditions and/or effects \n"
"for multiple players. The duplicates will have all players replaced. \n"
"Is this what you want to do?";
const char warningNoPlayer[] =
"Warning: This trigger appears to have no player assigned, meaning \n"
"that this operation will just produce copies of the source trigger. \n"
"Is this what you want to do?";

/*	editor_count: number of condition/effect editors currently opened */
unsigned editor_count = 0;

/* referenced functions */
void TrigTree_UpdateChildren(HWND treeview, HTREEITEM parent, class ItemData *id, int operand);
HTREEITEM TrigTree_AddTrig(HWND treeview, int index, HTREEITEM after);
HTREEITEM TrigTree_GetLastCondition(HWND treeview, HTREEITEM trigger);

/*
	FillTrigCB: Callback for IDD_EFFECT that fills a combobox with scen.triggers.
*/
void FillTrigCB(HWND combobox, size_t select)
{
    Trigger * trig;
    long index = 0;
	for (vector<unsigned long>::const_iterator i = scen.t_order.begin();
		i != scen.t_order.end(); ++i, index++)
	{
        std::string name("");
	    trig = &scen.triggers.at(*i);
        if (setts.showdisplayorder || setts.showtrigids) {
            int min_id_width = TRIG_PADDING + numDigits(scen.triggers.size());
            std::string idName(trig->getIDName());
            int extraspaces = min_id_width - idName.size();
            if (extraspaces < 1)
                extraspaces = 1;
            name.append(trig->getIDName());
            name.append(extraspaces, ' ');
        }
        if (setts.showtrignames) {
            name.append(trig->getName(false,true,MAX_RECURSION));
            if (setts.showtrigfunction)
                name.append("     --     ");
        }
        if (setts.showtrigfunction)
            name.append(trig->getName(true,true,MAX_RECURSION));
		LRESULT idx = Combo_AddStringA(combobox, name.c_str());
		SendMessage(combobox, CB_SETITEMDATA, idx, *i);

		if (*i == select)	//to avoid cycling through again in LoadEffect()
			SendMessage(combobox, CB_SETCURSEL, idx, 0);
	}
}

/**
 * Subclass WndProc for a Tree-View that wants VK_RETURN keypresses.
 */
LRESULT CALLBACK TreeView_KeyWndProc(
		HWND control, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (!TVWndProc)
		return 0; // unreasonable default?

	switch (msg)
	{
	case WM_GETDLGCODE:
		/* Get ENTER keys. */
		if (wParam == VK_RETURN)
			return DLGC_WANTALLKEYS;
		break;

	case WM_CHAR:
		// Ignore character generated by VK_RETURN
		if (wParam == 0x0D)
			return 0;
	}

	return CallWindowProc(TVWndProc, control, msg, wParam, lParam);
}

/* ItemData: Stores custom information for each tree item. */
class ItemData
{
public:
	ItemData(enum ECType type, int index);

	virtual ~ItemData()		//doesn't do anything, but we need it for the subclasses()
	{}

	enum ECType type;
	unsigned index;	//to scen.triggers, not t_order

	virtual bool Delete(HWND treeview, HTREEITEM target);
	virtual void GetName(char *buffer);
	virtual void DuplicatePlayers(HWND treeview, HTREEITEM item);

	/* ModifyIndex: simply adds operand to the index, necessary for effect/condition items */
	virtual void ModifyIndex(int operand);

	/* OpenEditor: opens an editor for a condition/effect, no effect for a trigger */
	virtual void OpenEditor(HWND parent, HTREEITEM item);

	/* Copy: copies an item to a different location in the tree (used for dragndrop) */
	virtual bool Copy(HWND treeview, HTREEITEM source, HTREEITEM target);

	inline virtual Trigger *GetTrigger()
	{ return &scen.triggers.at(index); }

};

ItemData::ItemData(enum ECType type, int index)
:	type(type), index(index)
{}

bool ItemData::Delete(HWND, HTREEITEM)
{
	if (editor_count)
		return false;

	/*	Find and remove entry from t_order array.

		Note: The trigger itself is removed later, by Scenario::clean_triggers().
		This is necessary to preserve indexes, for the TVITEM lParams and
		especially for Effect::trig_index.
	*/
	vector<unsigned long>::iterator iter =
		std::find(scen.t_order.begin(), scen.t_order.end(), index);

	if (iter != scen.t_order.end()) // Is this check necessary?
		scen.t_order.erase(iter);

	return true;
}

void ItemData::GetName(char *buffer)
{
	Trigger *t = &scen.triggers.at(index);

    printf_log("Trigger getname %d.\n", index);
    std::string name("");
    if (t) {
        if (setts.showdisplayorder || setts.showtrigids) {
            int min_id_width = TRIG_PADDING + numDigits(scen.triggers.size());
            std::string idName(t->getIDName());
            int extraspaces = min_id_width - idName.size();
            if (extraspaces < 1)
                extraspaces = 1;
            name.append(t->getIDName());
            name.append(extraspaces, ' ');
        }
        if (setts.showtrignames) {
            name.append(t->getName(false,true,MAX_RECURSION));
            if (setts.showtrigfunction)
                name.append("     --     ");
        }
        if (setts.showtrigfunction)
            name.append(t->getName(true,true,MAX_RECURSION));
    } else {
        name.append("NULL Trigger.");
    }

	strcpy(buffer, (t) ? name.c_str() : "NULL Trigger.");
}

void ItemData::DuplicatePlayers(HWND treeview, HTREEITEM item)
{
	int i, player;
	size_t n_index;
	Trigger *t_source = &scen.triggers.at(index);
	Trigger *t_dup;

	player = t_source->get_player();
	if (player == -1)
	{
		if (MessageBox(treeview, warningInconsistentPlayer,
			"Duplication Warning", MB_ICONWARNING | MB_YESNO) == IDNO)
			return;
	}
	if (player == 0)
	{
		if (MessageBox(treeview, warningNoPlayer, "Duplication Warning",
			MB_ICONWARNING | MB_YESNO) == IDNO)
			return;
	}

	n_index = index;	//n_index is used for the positioning of the inserted trigger
	for (i = 1; i <= 8; i++)
	{
		if (i == player || !scen.players[i - 1].enable)	//i-1: weird player ordering again...
			continue;

		n_index = scen.insert_trigger(t_source, n_index);
		t_source = &scen.triggers.at(index);	//might have changed
		t_dup = &scen.triggers.at(n_index);
		t_dup->accept(ChangePlayerVisitor(i));

		//new name!
		_snprintf(t_dup->name, sizeof(t_dup->name), "%s (p%d)", t_source->name, i);

		item = TrigTree_AddTrig(treeview, n_index, item);	//keep updating item for a chain
	}
}

void ItemData::ModifyIndex(int operand)
{
	index += operand;
}

void ItemData::OpenEditor(HWND parent, HTREEITEM)
{
	//MessageBox(parent, errorNoEditTrig, szTrigTitle, MB_ICONWARNING);
}

// TODO: this is retarded, it should be Move instead!
bool ItemData::Copy(HWND treeview, HTREEITEM, HTREEITEM target)
{
	bool ret = false;
	ItemData *id_target = (ItemData*)GetItemParam(treeview, target);

	if (id_target->type == TRIGGER)
	{
		vector<unsigned long>::iterator t_target =
			std::find(scen.t_order.begin(), scen.t_order.end(), id_target->index);

		if (t_target != scen.t_order.end())
		{
			/*	Note: this does NOT make a real copy, it can really
				only be used for moving. */
			scen.t_order.insert(t_target + 1, index);
			TrigTree_AddTrig(treeview, index, target);

			ret = true;
		}
	}
	else
		MessageBox(treeview, noteNoTrigInTrig, szTrigTitle, MB_ICONWARNING);

	return ret;
}

/* EffectItemData: same thing, but specifically for effects */
class EffectItemData : public ItemData
{
public:
	EffectItemData(unsigned index, unsigned trig_index);
	~EffectItemData();

	bool Delete(HWND treeview, HTREEITEM target);
	void GetName(char *buffer);
	void DuplicatePlayers(HWND treeview, HTREEITEM target);
	Effect *GetEffect();
	void ModifyIndex(int operand);
	void OpenEditor(HWND parent, HTREEITEM item);
	bool Copy(HWND treeview, HTREEITEM source, HTREEITEM target);

	inline Trigger *GetTrigger()
	{ return &scen.triggers.at(trig_index); }

	unsigned trig_index;	//not essential, but very useful
	HWND editor;
	HTREEITEM treeitem;	//only used when effect/condition editor is activated
};

EffectItemData::EffectItemData(unsigned index, unsigned trig_index)
:	ItemData(EFFECT, index),
	trig_index(trig_index), editor(NULL)
{}

EffectItemData::~EffectItemData()
{
	if (editor)
		DestroyWindow(editor);
}

bool EffectItemData::Delete(HWND treeview, HTREEITEM target)
{
	Trigger *t = GetTrigger();

	t->effects.erase(t->effects.begin() + index);
	TrigTree_UpdateChildren(treeview, TreeView_GetParent(treeview, target), this, -1);

	return true;
}

void EffectItemData::GetName(char *buffer)
{
	Trigger *t = GetTrigger();
	assert(t);

	sprintf(buffer, "%s", t->effects[index].getName(setts.displayhints,NameFlags::LIMITLEN, MAX_RECURSION).c_str());
}

void EffectItemData::DuplicatePlayers(HWND treeview, HTREEITEM target)
{
	Trigger *t = GetTrigger();
	TVINSERTSTRUCT tvis;

	tvis.hParent = TreeView_GetParent(treeview, target);
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.iImage = BitmapIcons::EFFECT_BAD;
	tvis.item.iSelectedImage = BitmapIcons::EFFECT_BAD;
	tvis.item.mask = TVIF_IMAGE | TVIF_TEXT | TVIF_PARAM | TVIF_SELECTEDIMAGE;
	tvis.item.pszText = (LPSTR)LPSTR_TEXTCALLBACK;

	for (int i = 1; i <= 8; i++) //weird e/c player indexes
	{
		Effect &source = t->effects[this->index]; // get it new each iteration

		// Skip if we're on the source effect's player.
        if (bysource) {
		    if (i == source.s_player)
			    continue;
		} else {
		    if (i == source.t_player)
			    continue;
		}

		t->effects.push_back(source);

		// Set proper player on the duplicate.
        if (bysource) {
		    t->effects.back().s_player = i;
		} else {
		    t->effects.back().t_player = i;
		}

		tvis.item.lParam = (LPARAM)new EffectItemData( // FIXME: ugly cast
			t->effects.size() - 1, // size() - 1 is the duplicate right now
			trig_index);
		TreeView_InsertItem(treeview, &tvis);
	}
}

Effect *EffectItemData::GetEffect()
{
	Trigger *t = GetTrigger();
	assert(t);

	return &t->effects[index];
}

void EffectItemData::ModifyIndex(int operand)
{
	index += operand;

	if (editor)
		SendMessage(editor, EC_Update, operand, 0);
}

void EffectItemData::OpenEditor(HWND parent, HTREEITEM item)
{
	Effect *target = GetEffect();

	EditEffect *edit_data = new EditEffect(*target);
	edit_data->trigindex = trig_index;
	edit_data->index = index;
	edit_data->TrigCallback = FillTrigCB;
	edit_data->user = item;
	edit_data->parent = parent;
	edit_data->mapview = propdata.mapview;
	edit_data->players = scen.players;

	editor_count++;
	editor = CreateDialogParam(GetModuleHandle(NULL),
		(LPCSTR)IDD_EFFECT, parent, EffectWndProc, (LPARAM)edit_data);
}

bool EffectItemData::Copy(HWND treeview, HTREEITEM, HTREEITEM target)
{
	Trigger *t_target;
	TVINSERTSTRUCT tvis;
	class EffectItemData *id_new;
	class ItemData *id_target;
	int newindex;

	id_target = (ItemData*)GetItemParam(treeview, target);

	t_target = id_target->GetTrigger();
	Effect *e_source = GetEffect();

	if (!t_target || !e_source)
		return false;

	// This is the target position of the new effect.
	vector<Effect>::iterator e_target;

	/*	t_target, c_target, and parent all depend on the type.
		Should I use '?' for this? I'll favor speed for now. */
	switch (id_target->type)
	{
	case CONDITION:
		tvis.hParent = TreeView_GetParent(treeview, target);
		e_target = t_target->effects.end();
		newindex = t_target->effects.size();
		tvis.hInsertAfter = TrigTree_GetLastCondition(treeview, tvis.hParent);
		break;
	case EFFECT:
		tvis.hParent = TreeView_GetParent(treeview, target);
		e_target = t_target->effects.begin() + id_target->index + 1;
		newindex = id_target->index + 1;
		tvis.hInsertAfter = target;
		break;
	case TRIGGER:
		tvis.hParent = target;
		e_target = t_target->effects.end();
		newindex = t_target->effects.size();
		tvis.hInsertAfter = TVI_LAST;
		break;
	default:
		return false; // don't crash below
	}

	/* Get source item's image */
	tvis.item.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvis.item.hItem = dragging;
	TreeView_GetItem(treeview, &tvis.item);

	/* Add the new one */
	t_target->effects.insert(e_target, *e_source);
	id_new = new EffectItemData(newindex, t_target - &(*scen.triggers.begin()));
	tvis.item.mask =
		TVIF_IMAGE | TVIF_TEXT | TVIF_PARAM | TVIF_SELECTEDIMAGE;
	tvis.item.lParam = (LPARAM)id_new;
	tvis.item.pszText = LPSTR_TEXTCALLBACK;
	TrigTree_UpdateChildren(treeview, tvis.hParent, id_new, +1);
	TreeView_InsertItem(treeview, &tvis);

	return true;
}

/* ConditionItemData: same thing, buf specifically for conditions */
class ConditionItemData : public ItemData
{
public:
	ConditionItemData(unsigned index, unsigned trig_index);
	~ConditionItemData();

	bool Delete(HWND treeview, HTREEITEM target);
	void GetName(char *buffer);
	void DuplicatePlayers(HWND treeview, HTREEITEM target);
	Condition *GetCondition();
	void ModifyIndex(int operand);
	void OpenEditor(HWND parent, HTREEITEM item);
	bool Copy(HWND treeview, HTREEITEM source, HTREEITEM target);

	inline Trigger *GetTrigger()
	{ return &scen.triggers.at(trig_index); }

	unsigned trig_index;	//not essential, but very useful
	HWND editor;
	HTREEITEM treeitem;	//only used when effect/condition editor is activated
};

ConditionItemData::ConditionItemData(unsigned index, unsigned trig_index)
:	ItemData(CONDITION, index),
	trig_index(trig_index), editor(NULL)
{}

ConditionItemData::~ConditionItemData()
{
	if (editor)
		DestroyWindow(editor);
}

bool ConditionItemData::Delete(HWND treeview, HTREEITEM target)
{
	Trigger *t = GetTrigger();

	t->conds.erase(t->conds.begin() + index);
	TrigTree_UpdateChildren(treeview, TreeView_GetParent(treeview, target), this, -1);

	return true;
}

void ConditionItemData::GetName(char *buffer)
{
	Trigger *t = GetTrigger();
	assert(t);

	const char * reverse = "";
	if (scen.game == UP) {
	    reverse = ((t->conds[index].reserved == -256)?"NOT ":"");
	} else if (scen.game == AOHD4 || scen.game == AOF4 || scen.game == AOHD6 || scen.game == AOF6) {
	    reverse = ((t->conds[index].reverse_hd == 1)?"NOT ":"");
    }

	if (index == 0) {
	    sprintf(buffer, "If %s%s", reverse, t->conds[index].getName(setts.displayhints,NameFlags::NONE).c_str());
	} else {
	    sprintf(buffer, "and %s%s", reverse, t->conds[index].getName(setts.displayhints,NameFlags::NONE).c_str());
	}
}

void ConditionItemData::DuplicatePlayers(HWND treeview, HTREEITEM target)
{
	Trigger *t = GetTrigger();
	TVINSERTSTRUCT tvis;

	tvis.hParent = TreeView_GetParent(treeview, target);
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.iImage = BitmapIcons::COND_BAD;
	tvis.item.iSelectedImage = BitmapIcons::COND_BAD;
	tvis.item.mask = TVIF_IMAGE | TVIF_TEXT | TVIF_PARAM | TVIF_SELECTEDIMAGE;
	tvis.item.pszText = (LPSTR)LPSTR_TEXTCALLBACK;

	for (int i = 1; i <= 8; i++)	//weird e/c player indexes
	{
		Condition& source = t->conds[this->index]; //ptr may change

		// Don't need to make a duplicate for source condition.
		if (i == source.player)
			continue;

		t->conds.push_back(source);
		t->conds.back().player = i;

		tvis.item.lParam = (LPARAM)new ConditionItemData(
			t->conds.size() - 1, trig_index); // size() - 1 is index of back
		TreeView_InsertItem(treeview, &tvis);
	}
}

Condition *ConditionItemData::GetCondition()
{
	Trigger *t = GetTrigger();
	assert(t);

	return &t->conds[index];
}

void ConditionItemData::ModifyIndex(int operand)
{
	index += operand;

	if (editor)
		SendMessage(editor, EC_Update, operand, 0);
}

void ConditionItemData::OpenEditor(HWND parent, HTREEITEM item)
{
	EditCondition *edit_data = new EditCondition;
	edit_data->trigindex = trig_index;
	edit_data->index = index;
	edit_data->user = item;
	edit_data->parent = parent;
	edit_data->mapview = propdata.mapview;
	memcpy(&edit_data->c, GetCondition(), sizeof(Condition));
	edit_data->players = scen.players;

	editor_count++;
	editor = CreateDialogParam(GetModuleHandle(NULL),
		(LPCSTR)IDD_COND, parent, CondWndProc, (LPARAM)edit_data);
}

bool ConditionItemData::Copy(HWND treeview, HTREEITEM, HTREEITEM target)
{
	Trigger *t_target;
	TVINSERTSTRUCT tvis;
	class ItemData *id_target;

	id_target = (ItemData*)GetItemParam(treeview, target);
	Condition *c_source = GetCondition();
	t_target = id_target->GetTrigger();

	if (!c_source || !t_target)
		return false;

	// Target index of copy.
	int newindex;

	switch (id_target->type)
	{
	case CONDITION:
		tvis.hParent = TreeView_GetParent(treeview, target);
		newindex = id_target->index + 1;
		tvis.hInsertAfter = target;
		break;
	case EFFECT:
		tvis.hParent = TreeView_GetParent(treeview, target);
		newindex = t_target->conds.size();
		tvis.hInsertAfter = TrigTree_GetLastCondition(treeview, tvis.hParent);
		if (!tvis.hInsertAfter)
			tvis.hInsertAfter = TVI_FIRST;
		break;
	case TRIGGER:
		tvis.hParent = target;
		newindex = t_target->conds.size();
		tvis.hInsertAfter = TVI_LAST;
		break;
	default:
		throw std::logic_error("No idea what the target type is.");
	}

	/* Get source item's image */
	tvis.item.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvis.item.hItem = dragging;
	TreeView_GetItem(treeview, &tvis.item);

	/* Add the new one */
	ConditionItemData *id_new =
		new ConditionItemData(newindex, t_target - &(*scen.triggers.begin()));
	t_target->conds.insert(t_target->conds.begin() + newindex, *c_source);
	tvis.item.mask =
		TVIF_IMAGE | TVIF_TEXT | TVIF_PARAM | TVIF_SELECTEDIMAGE;
	tvis.item.lParam = (LPARAM)id_new;
	tvis.item.pszText = LPSTR_TEXTCALLBACK;
	TrigTree_UpdateChildren(treeview, tvis.hParent, id_new, +1);
	TreeView_InsertItem(treeview, &tvis);

	return true;
}

/*
	TrigTree_GetLastCondition: finds a trigger's last condition HTREEITEM
*/
HTREEITEM TrigTree_GetLastCondition(HWND treeview, HTREEITEM trigger)
{
	HTREEITEM item, ret = NULL;

	/* first grab the first child*/
	item = TreeView_GetNextItem(treeview, trigger, TVGN_CHILD);

	/*	now just go through its siblings. this loop performs a needless
		assignment every cycle, but i can't think of a better way */
	while (item && ((ItemData*)GetItemParam(treeview, item))->type == CONDITION)
	{
		ret = item;
		item = TreeView_GetNextItem(treeview, item, TVGN_NEXT);
	}

	return ret;
}

/*
	TrigTree_UpdateChildren:
		Parses a tree item's children and updates indexes on EC addition/deletion.

  parent: Handle to the parent treeview node.
  id: The identifier (lParam) of the child being added/removed.
  operand: Added to the index of items after the one being added/removed.

  Note: MUST be called before insertion/deletion of item!
*/
void TrigTree_UpdateChildren(HWND treeview, HTREEITEM parent, class ItemData *id, int operand)
{
	TVITEM tvi;
	class ItemData *parse;
	unsigned int index;

	tvi.mask = TVIF_HANDLE | TVIF_PARAM;
	tvi.hItem = TreeView_GetChild(treeview, parent);
	index = id->index;	//this may change during process (deletion)

	while (tvi.hItem)	//NULL marks end of children
	{
		TreeView_GetItem(treeview, &tvi);
		parse = (class ItemData*)tvi.lParam;

		if (parse->type == id->type && parse->index >= index)
			parse->ModifyIndex(operand);

		tvi.hItem = TreeView_GetNextSibling(treeview, tvi.hItem);
	}
}

/*
	TrigTree_AddTrig: Adds a node to the TrigTree representing an already-created trigger.

  treeview: Handle to the Tree-View.
  index: Index of the represented trigger.
  after: Put the trigger after this node.
*/
HTREEITEM TrigTree_AddTrig(HWND treeview, int index, HTREEITEM after)
{
	TVINSERTSTRUCT tvis;
	HTREEITEM trignode;	//parent of condition/effect nodes
	Trigger *t = &scen.triggers.at(index);
	bool good = true;

	/* These paramters are for both triggers and conditions/effects */
	tvis.hInsertAfter = after;
	tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;

	/* First add the trigger node. */
	tvis.hParent = TVI_ROOT;
	tvis.item.lParam = (LPARAM)new ItemData(TRIGGER, index);
	tvis.item.pszText = (LPSTR)LPSTR_TEXTCALLBACK;		//use TVN_GETDISPINFO.
	tvis.item.iImage = 1;
	tvis.item.iSelectedImage = 1;
	trignode = TreeView_InsertItem(treeview, &tvis);

	/* Then add the condition/effect nodes */
	tvis.hParent = trignode;

	for (unsigned ec_index = 0; ec_index != t->conds.size(); ++ec_index)
	{
	    // disabled caching for the moment
	    //if (t->conds[ec_index].get_valid_since_last_check()) {
	    if (t->conds[ec_index].check_and_save()) {
	        tvis.item.iImage = BitmapIcons::COND_GOOD;
	    } else {
	        tvis.item.iImage = BitmapIcons::COND_BAD;
		    good = false;
	    }
		tvis.item.iSelectedImage = tvis.item.iImage;
		tvis.item.lParam = (LPARAM)new ConditionItemData(ec_index, index);
		SendMessage(treeview, TVM_INSERTITEM, 0, (LPARAM)&tvis);
	}

	//printf("Start addtrig %d\n", index);
	// Don't use iterators: we need the index.
	for (unsigned ec_index = 0; ec_index != t->effects.size(); ++ec_index)
	{
	    // disabled caching for the moment
	    //if (t->effects[ec_index].get_valid_since_last_check()) {
	    if (t->effects[ec_index].check_and_save()) {
	        tvis.item.iImage = BitmapIcons::EFFECT_GOOD;
	    } else {
	        tvis.item.iImage = BitmapIcons::EFFECT_BAD;
	        good = false;
	    }
		//printf("trig index: %d...\n", t->effects[ec_index].trig_index);
		//good &= (tvis.item.iImage = (t->effects[ec_index].check() && scen.triggers.at(t->effects[ec_index].trig_index) != NULL));
		tvis.item.iSelectedImage = tvis.item.iImage;
		tvis.item.lParam = (LPARAM)new EffectItemData(ec_index, index);
		SendMessage(treeview, TVM_INSERTITEM, 0, (LPARAM)&tvis);
	}

	tvis.item.hItem = trignode;
	tvis.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	if (good) {
	    if (t->loop) {
	        if (t->state) {
	            tvis.item.iImage = BitmapIcons::LOOP_ON_GOOD;
	            tvis.item.iSelectedImage = BitmapIcons::LOOP_ON_GOOD;
	        } else {
	            tvis.item.iImage = BitmapIcons::LOOP_OFF_GOOD;
	            tvis.item.iSelectedImage = BitmapIcons::LOOP_OFF_GOOD;
	        }
	    } else {
	        if (t->state) {
	            tvis.item.iImage = BitmapIcons::TRIG_ON_GOOD;
	            tvis.item.iSelectedImage = BitmapIcons::TRIG_ON_GOOD;
	        } else {
	            tvis.item.iImage = BitmapIcons::TRIG_OFF_GOOD;
	            tvis.item.iSelectedImage = BitmapIcons::TRIG_OFF_GOOD;
	        }
	    }
	} else {
	    if (t->loop) {
	        if (t->state) {
	            tvis.item.iImage = BitmapIcons::LOOP_ON_BAD;
	            tvis.item.iSelectedImage = BitmapIcons::LOOP_ON_BAD;
	        } else {
	            tvis.item.iImage = BitmapIcons::LOOP_OFF_BAD;
	            tvis.item.iSelectedImage = BitmapIcons::LOOP_OFF_BAD;
	        }
	    } else {
	        if (t->state) {
	            tvis.item.iImage = BitmapIcons::TRIG_ON_BAD;
	            tvis.item.iSelectedImage = BitmapIcons::TRIG_ON_BAD;
	        } else {
	            tvis.item.iImage = BitmapIcons::TRIG_OFF_BAD;
	            tvis.item.iSelectedImage = BitmapIcons::TRIG_OFF_BAD;
	        }
	    }
	}
	TreeView_SetItem(treeview, &tvis.item);

	return trignode;
}

/* Now, for the entree. */

void LoadTrigger(HWND dialog, Trigger *t)
{
	if (t)
	{
		CheckDlgButton(dialog, IDC_T_STATE, t->state);
		CheckDlgButton(dialog, IDC_T_LOOP, t->loop);
		CheckDlgButton(dialog, IDC_T_OBJ, t->obj);
		SetDlgItemText(dialog, IDC_T_DESC, t->description.c_str());
		SetDlgItemInt(dialog, IDC_T_ORDER, t->obj_order, FALSE);
		SetDlgItemInt(dialog, IDC_T_DESCID, t->obj_str_id, FALSE);
		SetDlgItemInt(dialog, IDC_T_UNKNOWN, t->u1, FALSE);
	}
	else
	{
		/* Reset values. */
		CheckDlgButton(dialog, IDC_T_STATE, BST_UNCHECKED);
		CheckDlgButton(dialog, IDC_T_LOOP, BST_UNCHECKED);
		CheckDlgButton(dialog, IDC_T_OBJ, BST_UNCHECKED);
		SetDlgItemText(dialog, IDC_T_DESC, "");
		SetDlgItemText(dialog, IDC_T_ORDER, "");
		SetDlgItemText(dialog, IDC_T_DESCID, "");
		SetDlgItemText(dialog, IDC_T_UNKNOWN, "");
	}
}

void ResetTriggerSection(HWND dialog)
{
	CheckDlgButton(dialog, IDC_T_OBJSTATE, BST_UNCHECKED);
}

void SaveTriggerSection(HWND dialog)
{
	scen.objstate = IsDlgButtonChecked(dialog, IDC_T_OBJSTATE);
}

void LoadTriggerSection(HWND dialog)
{
	CheckDlgButton(dialog, IDC_T_OBJSTATE, scen.objstate);
}

void SaveTrigger(HWND dialog, Trigger *t)
{
	if (t)
	{
		t->state = IsDlgButtonChecked(dialog, IDC_T_STATE);
		t->loop = IsDlgButtonChecked(dialog, IDC_T_LOOP);
		t->obj = (IsDlgButtonChecked(dialog, IDC_T_OBJ) != 0);
		GetWindowText(GetDlgItem(dialog, IDC_T_DESC), t->description);
		t->obj_order = GetDlgItemInt(dialog, IDC_T_ORDER, NULL, FALSE);
		t->obj_str_id = GetDlgItemInt(dialog, IDC_T_DESCID, NULL, FALSE);
		t->u1 = GetDlgItemInt(dialog, IDC_T_UNKNOWN, NULL, FALSE);
	}
}

/*
	Triggers_EditMenu: Enables/Disables items on the Edit Menu.

	menu: Handle to the menu.
	state: Whether the items should be enabled or disabled.
*/
void Triggers_EditMenu(HMENU menu, bool state)
{
	if (state)
	{
		EnableMenuItem(menu, ID_TS_EDIT_COPY, MF_ENABLED);
		EnableMenuItem(menu, ID_TS_EDIT_CUT, MF_ENABLED);
		EnableMenuItem(menu, ID_EDIT_DELETE, MF_ENABLED);
		if (GetPriorityClipboardFormat(&propdata.tformat, NUM_FORMATS) > 0)
			EnableMenuItem(menu, ID_TS_EDIT_PASTE, MF_ENABLED);
		EnableMenuItem(menu, ID_EDIT_RENAME, MF_ENABLED);
	}
	else
	{
		EnableMenuItem(menu, ID_TS_EDIT_COPY, MF_GRAYED);
		EnableMenuItem(menu, ID_TS_EDIT_CUT, MF_GRAYED);
		EnableMenuItem(menu, ID_EDIT_DELETE, MF_GRAYED);
		EnableMenuItem(menu, ID_TS_EDIT_PASTE, MF_GRAYED);
		EnableMenuItem(menu, ID_EDIT_RENAME, MF_GRAYED);
	}
}

bool Update_ctrig(HWND treeview)
{
	HTREEITEM sel;
	class ItemData *data;

	c_trig = NULL;

	sel = TreeView_GetNextItem(treeview, 0, TVGN_CARET);
	data = (ItemData*)GetItemParam(treeview, sel);

	if (!sel || !data)
		return false;

	c_trig = data->GetTrigger();

	return (c_trig != NULL);
}

/*
	TrigTree_HandleDelete:
		Used by TrigDlgProc to delete the current selection,
		whether	it be a trigger or a condition or a effect.
*/

BOOL CALLBACK EditorRefreshCB(HWND editor, LPARAM lParam)
{
	int index = lParam;

	SendMessage(editor, EC_RefreshTriggers, index, 0);

	return TRUE;
}

void TrigTree_HandleDelete(HWND dialog, HWND treeview)
{
	HTREEITEM	target,	//the selected item, to delete
				next;	//the following item, to select after deletion
	class ItemData *data;

	target = TreeView_GetNextItem(treeview, 0, TVGN_CARET);
	next = TreeView_GetNextItem(treeview, target, TVGN_NEXT);
	data = (ItemData*)GetItemParam(treeview, target);

	if (!target || !data)
		return;

	if (!data->Delete(treeview, target))
	{
		MessageBox(dialog, noticeNoDelete, szTrigTitle, MB_ICONWARNING);
		return;
	}

	c_trig = 0;	//Don't bother to save the deleted one.

	TreeView_DeleteItem(treeview, target);
	TreeView_Select(treeview, next, TVGN_CARET);
}

/*
	Trig_ToClipboard: "Serializes" given trigger to the clipboard.
*/
void Trig_ToClipboard(HWND dialog, Trigger *t, class ItemData *data)
{
	HGLOBAL copy_clip;
	size_t needed;
	char *clip_buff;

	if (!OpenClipboard(dialog))
		MessageBox(dialog, errorOpenClipboard, szTrigTitle, MB_ICONWARNING);
	EmptyClipboard();

	/* copy the data to the new memory */
	if (data->type == TRIGGER)
	{
		// get the size by doing a dummy write
		NullBuffer nullbuff;
		t->tobuffer(nullbuff);
		needed = nullbuff.size();

		copy_clip = GlobalAlloc(GMEM_MOVEABLE, needed);

		clip_buff = (char*)GlobalLock(copy_clip);
		if (clip_buff)
			t->tobuffer(MemBuffer(clip_buff, needed));
		GlobalUnlock(copy_clip);

		if (!SetClipboardData(propdata.tformat, copy_clip))
			MessageBox(dialog, errorSetClipboard, szTrigTitle, MB_ICONWARNING);
	}
	else if (data->type == CONDITION)
	{
		NullBuffer nullbuff;
		Condition& cond_source = t->conds[data->index];
		cond_source.tobuffer(nullbuff);
		needed = nullbuff.size();

		copy_clip = GlobalAlloc(GMEM_MOVEABLE, needed);
		clip_buff = (char *)GlobalLock(copy_clip);
		MemBuffer b(clip_buff, needed);
		cond_source.tobuffer(b);

		GlobalUnlock(copy_clip);

		if (!SetClipboardData(propdata.ecformat, copy_clip))
			MessageBox(dialog, errorSetClipboard, szTrigTitle, MB_ICONWARNING);
	}
	else if (data->type == EFFECT)
	{
		NullBuffer nullbuff;
		Effect& effect_source = t->effects[data->index];
		effect_source.tobuffer(nullbuff);
		needed = nullbuff.size();

		copy_clip = GlobalAlloc(GMEM_MOVEABLE, needed);
		clip_buff = (char *)GlobalLock(copy_clip);
		MemBuffer b(clip_buff, needed);
		effect_source.tobuffer(b);

		GlobalUnlock(copy_clip);

		if (!SetClipboardData(propdata.ecformat, copy_clip))
			MessageBox(dialog, errorSetClipboard, szTrigTitle, MB_ICONWARNING);
	}

	CloseClipboard();

	Triggers_EditMenu(propdata.menu, true);
}

/*
	TrigTree_Paste:	Handles all paste messages. (Only one exists at this point.)
*/
void TrigTree_Paste(HWND dialog)
{
	HGLOBAL clip_data;
	HWND treeview = GetDlgItem(dialog, IDC_T_TREE);
	HTREEITEM selected, pasted;
	class ItemData *data;
	int index, index_sel;
	UINT format;

	/* Get available format. */
	format = GetPriorityClipboardFormat(&propdata.tformat, NUM_FORMATS);
	if (format <= 0)
	{
		MessageBox(dialog, warningNoFormat, szTrigTitle, MB_ICONWARNING);
		return;
	}

	if (!OpenClipboard(dialog))
		MessageBox(dialog, errorOpenClipboard, szTrigTitle, MB_ICONWARNING);

	clip_data = GetClipboardData(format);
	SIZE_T clip_size = GlobalSize(clip_data);

	// get selected trigger
	selected = GetRootSel(treeview);

	// if there is a selected trigger, load its data
	if (selected)
	{
		data = (class ItemData *)GetItemParam(treeview, selected);
		index_sel = data->index;
	}

	if (format == propdata.tformat)
	{
		char *trig_data;

		// no selection default: insert after last
		if (!selected)
		{
			selected = TVI_LAST;
			index_sel = -1;	//insert after the item before the beginning, haha
		}

		trig_data = static_cast<char*>(GlobalLock(clip_data));
		if (trig_data)
		{
			try
			{
				Trigger t(MemBuffer(trig_data, clip_size));
				index = scen.insert_trigger(&t, index_sel);
				GlobalUnlock(clip_data);

				pasted = TrigTree_AddTrig(treeview, index, selected);

				TreeView_SelectItem(treeview, pasted);
			}
			catch (std::exception &ex)
			{
				printf("Paste: %s\n", ex.what());
				MessageBox(dialog, "Error placing trigger.", szTrigTitle, MB_ICONWARNING);
			}
		}
		else
			MessageBox(dialog, "Could not get clipboard data.", szTrigTitle, MB_ICONWARNING);
	}
	else if (format == propdata.ecformat)
	{
		Trigger *t;
		HTREEITEM parent;

		// no reasonable default for no-selection paste
		if (!selected)
			MessageBox(dialog, "Please select an item first. Pasting aborted.",
				szTrigTitle, MB_ICONWARNING);

		t = &scen.triggers.at(index_sel);

		char *ec_data = static_cast<char*>(GlobalLock(clip_data));
		if (ec_data)
		{
			parent = GetRootSel(treeview);

			try
			{
				MemBuffer buffer(ec_data, clip_size);

				// FIXME: there's no reason for this function to know the
				// format of ec_data.
				if (*ec_data == ClipboardType::EFFECT)
				{
					t->effects.push_back(Effect(buffer));
					data = new EffectItemData(t->effects.size() - 1,
						index_sel);
					TreeView_AddChild(treeview, (LPARAM)data, parent, TVI_LAST, 1);
				}
				else if (*ec_data == ClipboardType::CONDITION)
				{
					t->conds.push_back(Condition(buffer));
					data = new ConditionItemData(t->conds.size() - 1,
						index_sel);
					TreeView_AddChild(treeview, (LPARAM)data, parent,
						TrigTree_GetLastCondition(treeview, parent), 0);
				}
			}
			catch (std::exception &ex)
			{
				printf("Paste: %s\n", ex.what());
				MessageBox(dialog, "Error placing condition/effect.", szTrigTitle, MB_ICONWARNING);
			}
		}
		GlobalUnlock(clip_data);
	}

	CloseClipboard();
}

/*
	TrigTree_DuplicatePlayers: duplicates a trigger for all players
*/
void TrigTree_DuplicatePlayers(HWND treeview)
{
	HTREEITEM selected;
	class ItemData *data;

	selected = TreeView_GetSelection(treeview);
	data = (ItemData*)GetItemParam(treeview, selected);
	if (!selected || !data)
		return;
	data->DuplicatePlayers(treeview, selected);
}

/*
	TrigTree_Reset: Deletes all items in the trigger tree, and optionally refreshes
*/
void TrigTree_Reset(HWND treeview, bool refresh)
{
	HCURSOR wait, previous;

	wait = (HCURSOR)LoadImage(NULL, MAKEINTRESOURCE(OCR_WAIT), IMAGE_CURSOR, 0, 0, LR_SHARED);
	previous = SetCursor(wait);
	LockWindowUpdate(treeview);
	c_trig = NULL;	//since there's no guarantee it exists anymore
	TreeView_SelectItem(treeview, NULL);  //prevents loading every single trigger during deletion
	TreeView_DeleteItem(treeview, TVI_ROOT);
	LockWindowUpdate(NULL);
	SetCursor(previous);

	if (refresh)
	{
		for (vector<unsigned long>::const_iterator i = scen.t_order.begin();
			i != scen.t_order.end(); ++i)
		{
			TrigTree_AddTrig(treeview, *i, TVI_LAST);
		}
	}
}

/** Command-handling functions **/

/*
	TrigTree_AddNew: Adds a new trigger.
*/
void TrigTree_AddNew(HWND treeview)
{
	Trigger t;
	int index_new, index_sibling;
	HTREEITEM newitem, sibling;
	class ItemData *data;

	/* get the above "sibling" */
	sibling = GetRootSel(treeview);
	if (!sibling)	//no selected root item
	{
		sibling = TVI_LAST;
		index_sibling = -1;	//insert after the item before the beginning, haha
	}
	else
	{
		data = (class ItemData *)GetItemParam(treeview, sibling);
		index_sibling = data->index;
	}

	//add the trigger to the vector
	sprintf(t.name, "New Trigger %d", scen.triggers.size());
	index_new = scen.insert_trigger(&t, index_sibling);

	//add the node to the treeview
	newitem = TrigTree_AddTrig(treeview, index_new, sibling);
	TreeView_SelectItem(treeview, newitem);

	SetFocus(treeview);
}

/*
	TrigTree_HandleEdit: Handles IDC_T_EDIT message to edit currently selected c/e.
*/
void TrigTree_HandleEdit(HWND treeview, HWND parent)
{
	class ItemData *data;
	HTREEITEM item;

	item = TreeView_GetSelection(treeview);

	if ((!c_trig && !Update_ctrig(treeview)) || !item)
	{
		//MessageBox(treeview, warningNoSelection, szTrigTitle, MB_ICONWARNING);
		return;
	}

	data = (ItemData*)GetItemParam(treeview, item);

	/*
	Modeless Editors:

	Must disable adding/deleting effects/conditions in edited trigger.
	Must update trigger list for effects.
	Must destroy all dialogs on PSN_KILLACTIVE.
	*/
	data->OpenEditor(parent, item);
}


/** Message-processing Functions **/

/**
 * Initializes the trigger editor, namely the tree's image list, in response to
 * WM_INITDIALOG.
*/
BOOL Handle_WM_INITDIALOG(HWND dialog)
{
	HIMAGELIST il;
	HWND treeview;
	HINSTANCE inst = GetModuleHandle(NULL);
	HBITMAP trig_on_good,  trig_on_bad;
	HBITMAP trig_off_good, trig_off_bad;
	HBITMAP loop_on_good,  loop_on_bad;
	HBITMAP loop_off_good, loop_off_bad;
	HBITMAP cond_good,  cond_bad;
	HBITMAP effect_good,  effect_bad;

    treefont = CreateFont(
                          -10,    //nHeight,          // Logical height
                          0,      //nHeight * 2/3,    // Logical avg character width
                          0,                          // Angle of escapement (0)
                          0,                          // Baseline angle (0)
                          FW_DONTCARE,                // Weight (0)
                          FALSE,                      // Italic (0)
                          FALSE,                      // Underline (0)
                          FALSE,                      // Strikeout (0)
                          ANSI_CHARSET,               // Character set identifier ??
                          OUT_DEFAULT_PRECIS,         // Output precision
                          CLIP_DEFAULT_PRECIS,        // Clip precision (0)
                          //CLEARTYPE_QUALITY,          // Output quality
                          NONANTIALIASED_QUALITY,     // Output quality
                          VARIABLE_PITCH,             // Pitch and family
                          //"Lucida Console"            // Pointer to typeface name string
                          "Terminal"
                          //"Courier New"
                         );
    // For if i decide to enable monospacing again
    //SendMessage(GetDlgItem(dialog, IDC_T_TREE), WM_SETFONT, (WPARAM) treefont, 0);

	/* Subclass IDC_T_TREE to accept Enter keypresses. */
	TVWndProc = SetWindowWndProc(GetDlgItem(dialog, IDC_T_TREE),
			TreeView_KeyWndProc);

	/* Setup image list */
	il = ImageList_Create(16, 16, ILC_COLOR, 12, 1);
	trig_on_good  = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_TRIG_ON_GOOD ), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	trig_on_bad   = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_TRIG_ON_BAD  ), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	trig_off_good = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_TRIG_OFF_GOOD), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	trig_off_bad  = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_TRIG_OFF_BAD ), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	loop_on_good  = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_LOOP_ON_GOOD ), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	loop_on_bad   = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_LOOP_ON_BAD  ), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	loop_off_good = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_LOOP_OFF_GOOD), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	loop_off_bad  = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_LOOP_OFF_BAD ), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	cond_good     = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_COND_GOOD    ), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	cond_bad      = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_COND_BAD     ), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	effect_good   = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_EFFECT_GOOD  ), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	effect_bad    = (HBITMAP)LoadImage(inst, MAKEINTRESOURCE(IDB_EFFECT_BAD   ), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);

	/*
	ImageList_AddMasked(il, evil, RGB(0xFF,0xFF,0xFF));	//index 0 = false
	ImageList_AddMasked(il, good, RGB(0xFF,0xFF,0xFF));	//index 1 = true
	*/
	ImageList_Add(il, trig_on_good,  NULL);	//index 0 = false
	ImageList_Add(il, trig_on_bad ,  NULL);	//index 1 = true
	ImageList_Add(il, trig_off_good, NULL);	//index 2
	ImageList_Add(il, trig_off_bad , NULL);	//index 3
	ImageList_Add(il, loop_on_good,  NULL);	//index 4
	ImageList_Add(il, loop_on_bad ,  NULL);	//index 5
	ImageList_Add(il, loop_off_good, NULL);	//index 6
	ImageList_Add(il, loop_off_bad , NULL);	//index 7
	ImageList_Add(il, cond_good,     NULL);	//index 8
	ImageList_Add(il, cond_bad,      NULL);	//index 9
	ImageList_Add(il, effect_good,   NULL);	//index 10
	ImageList_Add(il, effect_bad,    NULL);	//index 11

	treeview = GetDlgItem(dialog, IDC_T_TREE);
	TreeView_SetImageList(treeview, il, TVSIL_NORMAL);

	CheckDlgButton(dialog, IDC_T_SHOWDISPLAYORDER, setts.showdisplayorder);
	CheckDlgButton(dialog, IDC_T_SHOWFIREORDER, setts.showtrigids);

    if (bysource) {
	    CheckRadioButton(dialog, IDC_T_SOURCE, IDC_T_TARGET, IDC_T_SOURCE);
	} else {
	    CheckRadioButton(dialog, IDC_T_SOURCE, IDC_T_TARGET, IDC_T_TARGET);
	}

    //SendDlgItemMessage(dialog, IDC_T_SHOWDISPLAYORDER, BM_SETCHECK, setts.showdisplayorder, 0);
    //SendDlgItemMessage(dialog, IDC_T_SHOWFIREORDER, BM_SETCHECK, setts.showtrigids, 0);
	SendDlgItemMessage(dialog, IDC_T_SHOWTRIGNAMES, BM_SETCHECK, setts.showtrignames, 0);
	SendDlgItemMessage(dialog, IDC_T_PSEUDONYMS, BM_SETCHECK, setts.showtrigfunction, 0);

    ENABLE_WND(IDC_T_OBJSTATE, scen.game == AOHD || scen.game == AOF || setts.editall);

	return TRUE;
}

/*
	TrigTree_HandleGetDispInfo: Handles TVN_GETDISPINFO notification.
*/
void TrigTree_HandleGetDispInfo(NMTVDISPINFO *dispinfo)
{
	class ItemData *data = (class ItemData*)dispinfo->item.lParam;

	if (dispinfo->item.mask == TVIF_TEXT)
	{
		try
		{
			data->GetName(dispinfo->item.pszText);

			if (setts.intense)
				printf("GetDispInfo returned name: %s\n", dispinfo->item.pszText);
		}
		catch (std::exception &ex) // don't let any bubble up
		{
			strcpy(dispinfo->item.pszText, ex.what());
		}
	}
	else
		assert(false);
}

/*
	TrigTree_HandleSelChanged: Handles TVN_SELCHANGED notification.
*/
void TrigTree_HandleSelChanged(NMTREEVIEW *treehdr, HWND dialog)
{
	Trigger *tNew;
	class ItemData *data_new, *data_old;
	data_new = (class ItemData*)treehdr->itemNew.lParam;
	data_old = (class ItemData*)treehdr->itemOld.lParam;

    if (c_trig) { // check c_trig also
	    // If there was an old selection and it was a Trigger, save it.
	    if (treehdr->itemOld.hItem && data_old->type == TRIGGER)
		    SaveTrigger(dialog, &scen.triggers.at(data_old->index));
	}

	HWND editbutton = GetDlgItem(dialog, IDC_T_EDIT);
	if (treehdr->itemNew.hItem)	//new selection
	{
	    ENABLE_WND(IDC_T_DESELECT, true);
		ENABLE_WND(IDC_T_NEFFECT, true);
		ENABLE_WND(IDC_T_NCOND, true);
		ENABLE_WND(IDC_T_DUPP, true);
		if (data_new->type == TRIGGER) {
		    SendMessage(editbutton, WM_SETTEXT, 0, (LPARAM) _T("Rename (Enter)"));
		} else {
		    SendMessage(editbutton, WM_SETTEXT, 0, (LPARAM) _T("Edit... (Enter)"));
		}
		ENABLE_WND(IDC_T_EDIT, true);
		Triggers_EditMenu(propdata.menu, true);

		tNew = data_new->GetTrigger();
	}
	else	//no new selection
	{
	    ENABLE_WND(IDC_T_DESELECT, false);
		ENABLE_WND(IDC_T_NEFFECT, false);
		ENABLE_WND(IDC_T_NCOND, false);
		ENABLE_WND(IDC_T_DUPP, false);
		SendMessage(editbutton, WM_SETTEXT, 0, (LPARAM) _T("Edit / Rename"));
		ENABLE_WND(IDC_T_EDIT, false);
		Triggers_EditMenu(propdata.menu, false);

		tNew = NULL;
	}

	c_trig = tNew;
	LoadTrigger(dialog, tNew);
}

/*
	TrigTree_HandleClosing: Handles the closing of a c/e editor.
*/
void TrigTree_HandleClosing(HWND treeview, WPARAM wParam, class EditEC *edit_data)
{
	class ItemData *id = (ItemData*)GetItemParam(treeview,
		static_cast<HTREEITEM>(edit_data->user));
	editor_count--;

	if (id->type == EFFECT)
		((EffectItemData*)id)->editor = NULL;
	else if (id->type == CONDITION)
		((ConditionItemData*)id)->editor = NULL;

	if (wParam)	//was modified
	{
		TVITEM item;
		Trigger *parent = &scen.triggers.at(edit_data->trigindex);

		edit_data->update(parent);

		//Send a TVM_SETITEM to force an update of the item's text.
		item.hItem = static_cast<HTREEITEM>(edit_data->user);
		item.mask = TVIF_HANDLE | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		item.pszText = (LPSTR)LPSTR_TEXTCALLBACK;
		if (HIWORD(wParam)) {
		    if (id->type == EFFECT) {
		        item.iImage = BitmapIcons::EFFECT_GOOD;
		    } else if (id->type == CONDITION) {
		        item.iImage = BitmapIcons::COND_GOOD;
		    }
		} else {
		    if (id->type == EFFECT) {
		        item.iImage = BitmapIcons::EFFECT_BAD;
		    } else if (id->type == CONDITION) {
		        item.iImage = BitmapIcons::COND_BAD;
		    }
		}
		item.iSelectedImage = item.iImage;
		SendMessage(treeview, TVM_SETITEM, 0, (LPARAM)&item);
	}
}

/*
	TrigTree_HandleDrag: Handles starting a drag-and-drop operation.
*/
void TrigTree_HandleDrag(HWND treeview, NMTREEVIEW *params)
{
	HIMAGELIST images;
	int height;
	class ItemData *id;

	/* Do the UI stuff. */
	height = TreeView_GetItemHeight(treeview);
	images = TreeView_CreateDragImage(treeview, params->itemNew.hItem);
	TreeView_SelectItem(treeview, NULL);
	ImageList_BeginDrag(images, 0, 0, height / 2);
	ImageList_DragEnter(treeview, params->ptDrag.x, params->ptDrag.y);

	/* Do the behind-the-scenes stuff. */
	id = (ItemData*)params->itemNew.lParam;
	SetCapture(GetParent(treeview));
	dragging = params->itemNew.hItem;
}

/*
	HandleMouseMove: Handles the WM_MOUSEMOVE message for dragging operations.
*/
bool HandleMouseMove(HWND dialog)
{
	TVHITTESTINFO tvht;
	HWND treeview = GetDlgItem(dialog, IDC_T_TREE);
	RECT rTree;
	int x, y;

	if (dragging)
	{
		GetWindowRect(treeview, &rTree);
		ScreenToClient(dialog, (POINT*)&rTree);
		ScreenToClient(dialog, ((POINT*)&rTree) + 1);

		x = lastCursorPos.x;
		y = lastCursorPos.y;
		ImageList_DragMove(x, y);

		tvht.pt = lastCursorPos;
		if (TreeView_HitTest(treeview, &tvht) &&
			tvht.flags & (TVHT_ONITEM | TVHT_ONITEMRIGHT))
		{
			ImageList_DragShowNolock(FALSE);
			TreeView_SelectDropTarget(treeview, tvht.hItem);
			ImageList_DragShowNolock(TRUE);
		}
		else if (y > rTree.bottom)
		{
			ImageList_DragShowNolock(FALSE);
			SendMessage(treeview, WM_VSCROLL, SB_LINEDOWN, (LPARAM)NULL);
			ImageList_DragShowNolock(TRUE);
			SetTimer(dialog, TT_Scroll, DRAG_DELAY, NULL);
		}
		else if (y < rTree.top)
		{
			ImageList_DragShowNolock(FALSE);
			SendMessage(treeview, WM_VSCROLL, SB_LINEUP, (LPARAM)NULL);
			ImageList_DragShowNolock(TRUE);
			SetTimer(dialog, TT_Scroll, DRAG_DELAY, NULL);
		}
	}

	return (dragging != NULL);
}

/*
	TrigTree_EndDrag
*/

void TrigTree_EndDrag(HWND treeview, WORD x, WORD y)
{
	TVHITTESTINFO tvhti;
	class ItemData *id_source;

	// End the visible drag operation: we want it to be canceled no matter what
	// happens below.
	ImageList_EndDrag();
	ReleaseCapture();
	TreeView_SelectDropTarget(treeview, NULL);

	// Also clear the global var in advance just in case an exception is
	// thrown.
	HTREEITEM dragged = dragging;
	dragging = NULL;

	tvhti.pt.x = x;
	tvhti.pt.y = y;

	if (TreeView_HitTest(treeview, &tvhti))
	{
		id_source = (ItemData*)GetItemParam(treeview, dragged);

		if (dragging != tvhti.hItem && id_source->Copy(treeview, dragged, tvhti.hItem))
		{
			id_source->Delete(treeview, dragged);
			TreeView_DeleteItem(treeview, dragged);
		}
	}
}

void TrigTree_HandleNewEffect(HWND treeview)
{
    Effect neweffect;
    neweffect.parent_trigger_id = c_trig->id;
	c_trig->effects.push_back(neweffect);

	TreeView_AddChild(treeview,
		(LPARAM)new EffectItemData(c_trig->effects.size() - 1,
			c_trig - &(*scen.triggers.begin())),
		GetRootSel(treeview),
		TVI_LAST,
		1
		);
}

void TrigTree_HandleNewCondition(HWND treeview)
{
	HTREEITEM trigger;

    Condition newcond;
    newcond.parent_trigger_id = c_trig->id;
	c_trig->conds.push_back(newcond);
	trigger = GetRootSel(treeview);

	TreeView_AddChild(treeview,
		(LPARAM)new ConditionItemData(c_trig->conds.size() - 1,
			c_trig - &(*scen.triggers.begin())),
		trigger,
		TrigTree_GetLastCondition(treeview, trigger),
		0
		);
}

/*	This macro saves any changes to the currently-selected trigger
	before any operation that could reallocate the vector, and thus
	invalidate the c_trig pointer. */
#define SAFETY()	SaveTrigger(dialog, c_trig); c_trig = NULL;

/* This macro checks that c_trig is valid before it is used. */
#define SAFECHECK() \
	if (!c_trig && !Update_ctrig(treeview)) \
	{	/*MessageBox(dialog, warningNoSelection, szTrigTitle, MB_ICONWARNING);*/ \
		break;	}

void TrigTree_HandleKeyDown(HWND dialog, NMTVKEYDOWN * keydown)
{
	if (keydown->wVKey == VK_DELETE)
		TrigTree_HandleDelete(dialog, keydown->hdr.hwndFrom);
	else if (keydown->wVKey == VK_RETURN) {
	    HWND treeview = GetDlgItem(dialog, IDC_T_TREE);	//all use this
		TrigTree_HandleEdit(treeview, dialog);
	    TreeView_EditLabel(treeview, TreeView_GetSelection(treeview));
	}
}

void PaintCurrent() {
	//SAFECHECK();
	//if (GetFocus() == treeview) {
	//	LPARAM data = GetItemParam(treeview, NULL);
	//	Trig_ToClipboard(dialog, c_trig, (class ItemData*)data);
	//}
}

/**
 * Handles a WM_COMMAND message sent to the dialog.
 */
INT_PTR Handle_WM_COMMAND(HWND dialog, WORD code, WORD id, HWND)
{
	HWND treeview = GetDlgItem(dialog, IDC_T_TREE);	//all use this

	switch (code)
	{
	case EN_CHANGE:
		switch (id)
		{
        }
	    break;
	case BN_CLICKED:
	case CBN_SELCHANGE:
		switch (id)
		{
		case IDC_T_SOURCE:
		    bysource = true;
		    break;
		case IDC_T_TARGET:
		    bysource = false;
		    break;
		case IDC_T_SHOWFIREORDER:
            if (SendMessage(GetDlgItem(dialog, IDC_T_SHOWFIREORDER),BM_GETCHECK,0,0)) {
                setts.showtrigids = true;
            } else {
                setts.showtrigids = false;
            }
            TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), true);
            break;
		case IDC_T_SHOWDISPLAYORDER:
            if (SendMessage(GetDlgItem(dialog, IDC_T_SHOWDISPLAYORDER),BM_GETCHECK,0,0)) {
                setts.showdisplayorder = true;
            } else {
                setts.showdisplayorder = false;
            }
            TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), true);
            break;
		case IDC_T_SHOWTRIGNAMES:
            if (SendMessage(GetDlgItem(dialog, IDC_T_SHOWTRIGNAMES),BM_GETCHECK,0,0)) {
                setts.showtrignames = true;
            } else {
                setts.showtrignames = false;
            }
            TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), true);
            break;
		case IDC_T_PSEUDONYMS:
            if (SendMessage(GetDlgItem(dialog, IDC_T_PSEUDONYMS),BM_GETCHECK,0,0)) {
                setts.showtrigfunction = true;
            } else {
                setts.showtrigfunction = false;
            }
            TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), true);
            break;
		case ID_TRIGGERS_SAVE_PSEUDONYMS:
		case ID_TRIGGERS_PREFIX_DISPLAY_ORDER:
		case ID_TRIGGERS_REMOVE_DISPLAY_ORDER_PREFIX:
            TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), true);
            break;
		case ID_TS_EDIT_COPY:
			SAFECHECK();
			if (GetFocus() == treeview)
			{
				LPARAM data = GetItemParam(treeview, NULL);
				Trig_ToClipboard(dialog, c_trig, (class ItemData*)data);
			}
			else
				SendMessage(GetFocus(), WM_COPY, 0, 0);
			break;

		case ID_TS_EDIT_CUT:
			SAFECHECK();
			if (GetFocus() == treeview)
			{
				LPARAM data = GetItemParam(treeview, NULL);
				Trig_ToClipboard(dialog, c_trig, (class ItemData*)data);
				TrigTree_HandleDelete(dialog, treeview);
			}
			else
				SendMessage(GetFocus(), WM_CUT, 0, 0);
			break;

		case ID_TS_EDIT_PASTE:
			if (GetFocus() == treeview)
				TrigTree_Paste(dialog);
			else
				SendMessage(GetFocus(), WM_PASTE, 0, 0);
			break;

		case IDC_T_EDIT:
			TrigTree_HandleEdit(treeview, dialog);
			break;

		case ID_EDIT_DELETE:
			TrigTree_HandleDelete(dialog, treeview);
			break;

		case ID_EDIT_RENAME:
			TreeView_EditLabel(treeview, TreeView_GetSelection(treeview));
			break;

		case ID_TRIGGERS_SORT_CONDS_EFFECTS:
		    TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), true);
			break;

		case ID_TRIGGERS_HIDENAMES:
			c_trig = NULL;
			TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), true);
			break;

		case ID_TRIGGERS_HIDE_DESCRIPTIONS:
			c_trig = NULL;
			TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), true);
			break;

		case ID_TRIGGERS_SWAP_NAMES_DESCRIPTIONS:
			c_trig = NULL;
			TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), true);
			break;

		case ID_TRIGGERS_EXPAND_ALL:
		    // Reset first
			TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), true);

	        HTREEITEM item;
	        item = TreeView_GetNextItem(treeview, NULL, TVGN_ROOT);
	        while (item) {
		        SendMessage(treeview, TVM_EXPAND, TVE_EXPAND, (LPARAM)item);
	            item = TreeView_GetNextItem(treeview, item, TVGN_NEXT);
	        }
	        // find way to go to top of tree
	        // maybe use 'find'. use find to search for trigger
	        //item = TreeView_GetNextItem(treeview, NULL, TVGN_ROOT);
		    //SendMessage(treeview, CB_SETCURSEL, 0, 0);
			break;

		case IDC_T_MOVE:
			scen.move_triggers(GetDlgItemInt(dialog, IDC_T_START, NULL, FALSE), GetDlgItemInt(dialog, IDC_T_END, NULL, FALSE), GetDlgItemInt(dialog, IDC_T_DEST, NULL, FALSE));
			TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), true);
			break;

		case IDC_T_DELETE:
		    TreeView_SelectItem(treeview, NULL); // to prevent
			scen.delete_triggers(GetDlgItemInt(dialog, IDC_T_START, NULL, FALSE), GetDlgItemInt(dialog, IDC_T_END, NULL, FALSE));
			TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), true);
			break;

		case IDC_T_DUPRANGE:
		    TreeView_SelectItem(treeview, NULL); // to prevent
			scen.duplicate_triggers(GetDlgItemInt(dialog, IDC_T_START, NULL, FALSE), GetDlgItemInt(dialog, IDC_T_END, NULL, FALSE), GetDlgItemInt(dialog, IDC_T_DEST, NULL, FALSE));
			TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), true);
			break;

        case IDC_T_ADD_ACTIVATION:
            scen.add_activation(GetDlgItemInt(dialog, IDC_T_START, NULL, FALSE), GetDlgItemInt(dialog, IDC_T_END, NULL, FALSE), GetDlgItemInt(dialog, IDC_T_DEST, NULL, FALSE));
			TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), true);
			break;

		case IDC_T_SYNC:
		    TreeView_SelectItem(treeview, NULL); // to prevent
			scen.sync_triggers();
			//scen.clean_triggers();
			TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), true);
			break;

		case IDC_T_DESELECT:
		    TreeView_SelectItem(treeview, NULL); // to prevent
			break;

		case IDC_T_ADD:
			SAFETY();
			TrigTree_AddNew(treeview);
			break;

		case IDC_T_NEFFECT:
			SAFECHECK();
			TrigTree_HandleNewEffect(treeview);
			break;

		case IDC_T_NCOND:
			SAFECHECK();
			TrigTree_HandleNewCondition(treeview);
			break;

		case IDC_T_DUPP:
			SAFETY();
			TrigTree_DuplicatePlayers(treeview);
			break;
		}
		break;

	case EN_SETFOCUS:
		EnableMenuItem(propdata.menu, ID_TS_EDIT_COPY, MF_ENABLED);
		EnableMenuItem(propdata.menu, ID_TS_EDIT_CUT, MF_ENABLED);
		if (IsClipboardFormatAvailable(CF_TEXT))
			EnableMenuItem(propdata.menu, ID_TS_EDIT_PASTE, MF_ENABLED);

        PaintCurrent();
		break;

	case EN_KILLFOCUS:
		EnableMenuItem(propdata.menu, ID_TS_EDIT_COPY, MF_GRAYED);
		EnableMenuItem(propdata.menu, ID_TS_EDIT_CUT, MF_GRAYED);
		EnableMenuItem(propdata.menu, ID_TS_EDIT_PASTE, MF_GRAYED);
		break;
	}

	// "If an application processes this message, it should return zero."
	return 0;
}

/**
 * Handles a WM_NOTIFY message sent to the dialog.
 */
INT_PTR Handle_WM_NOTIFY(HWND dialog, NMHDR const * header)
{
	INT_PTR ret = FALSE;   // reasonable default?

	switch (header->code)
	{
		case TVN_GETDISPINFO:
			TrigTree_HandleGetDispInfo((NMTVDISPINFO*)header);
			break;

		case TVN_BEGINLABELEDIT:
			{
				const NMTVDISPINFO *dispinfo = (NMTVDISPINFO*)header;
				class ItemData *data = (class ItemData*)dispinfo->item.lParam;
				HWND editbox = TreeView_GetEditControl(header->hwndFrom);

				if (data->type != TRIGGER)
				{
					SetWindowLongPtr(dialog, DWLP_MSGRESULT, TRUE);	//cancel the editing
					ret = TRUE;
				}
				else
				{
					SendMessage(editbox, WM_SETTEXT, NULL, (LPARAM)TEXT(scen.triggers.at(data->index).name));
					SendMessage(editbox, EM_SETLIMITTEXT, MAX_TRIGNAME_SAFE_HD - 1, 0);
					SubclassTreeEditControl(editbox);
				}
			}
			break;

		case TVN_ENDLABELEDIT:
			{
				const NMTVDISPINFO *dispinfo = (NMTVDISPINFO*)header;
				class ItemData *data = (class ItemData*)dispinfo->item.lParam;

				LPTSTR newname = dispinfo->item.pszText;

				if (newname)
				{
					ret = TRUE;	//activates DWLP_MSGRESULT

					strcpy(scen.triggers.at(data->index).name, newname);
				}
			}
			break;
			/*
			   case TVN_SETDISPINFO:
			   _ASSERT(false);
			   break;
			   */
		case TVN_SELCHANGED:
			TrigTree_HandleSelChanged((NMTREEVIEW*)header, dialog);
			break;

		case TVN_KEYDOWN:
			TrigTree_HandleKeyDown(dialog, (LPNMTVKEYDOWN)header);
			break;

		case TVN_DELETEITEM:
			{
				const NMTREEVIEW *info = (NMTREEVIEW*)header;
				class ItemData *data = (class ItemData*)info->itemOld.lParam;

				delete data;
			}
			break;

		case TVN_BEGINDRAG:
			//TrigTree_HandleDrag(header->hwndFrom, (NMTREEVIEW*)header);
			break;

		case PSN_SETACTIVE:
			LoadTriggerSection(dialog);
			TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), true);
			SetWindowText(propdata.statusbar, help_msg);
			ret = FALSE;
			break;

		case PSN_KILLACTIVE:
			SaveTriggerSection(dialog);
			ret = IDOK;
			if (editor_count)
			{
				ret = MessageBox(dialog,
						warningEditorsClosing, szTrigTitle, MB_ICONWARNING | MB_OKCANCEL);
			}

			if (ret == IDOK)
			{
				//Gray menu items for next window.
				Triggers_EditMenu(propdata.menu, false);

				/*	Remove selection because this page has no clue what the user
					does while it's not active. Note that the following line ends
					up saving the current trigger. */
				TreeView_SelectItem(GetDlgItem(dialog, IDC_T_TREE), NULL);
				c_trig = NULL;
			}
			SetWindowLongPtr(dialog, DWLP_MSGRESULT, ret != IDOK);	//false to continue
			ret = TRUE;
			break;
	}

	return ret;
}

INT_PTR Handle_WM_MOUSEMOVE(HWND dialog, WPARAM wParam, int x, int y)
{
	KillTimer(dialog, TT_Scroll);
	lastCursorPos.x = x;
	lastCursorPos.y = y;
	HandleMouseMove(dialog);	//x, y "passed" in lastCursorPos

	// "If an application processes this message, it should return zero."
	return 0;
}

INT_PTR Handle_WM_LBUTTONUP(HWND dialog, WPARAM wParam, int x, int y)
{
	if (dragging)
	{
		try
		{
			TrigTree_EndDrag(GetDlgItem(dialog, IDC_T_TREE), x, y);
		}
		catch (std::exception &ex) // don't let it propagate to Win32 code
		{
			MessageBox(dialog, ex.what(), "Drag operation failed.",
					MB_ICONWARNING);
		}
	}

	// "If an application processes this message, it should return zero."
	return 0;
}

/**
 * Handles AOKTS_Loading message sent to the dialog.
 */
INT_PTR Handle_AOKTS_Loading(HWND dialog)
{
	LoadTriggerSection(dialog);
	TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), true);
	LoadTrigger(dialog, NULL);

	/* Return anything: it is ignored. */
	return TRUE;
}

/**
 * Handles AOKTS_Saving message sent to the dialog.
 */
INT_PTR Handle_AOKTS_Saving(HWND dialog)
{
	SaveTriggerSection(dialog);
	SaveTrigger(dialog, c_trig);

	/* Return anything: it is ignored. */
	return TRUE;
}

/**
 * Handles AOKTS_Closing message sent to the dialog.
 */
INT_PTR Handle_AOKTS_Closing(HWND dialog)
{
	ResetTriggerSection(dialog);
	TrigTree_Reset(GetDlgItem(dialog, IDC_T_TREE), false);
	LoadTrigger(dialog, NULL);

	/* Return anything: it is ignored. */
	return TRUE;
}

/**
 * Handles EC_Closing message sent to the dialog.
 */
INT_PTR Handle_EC_Closing(HWND dialog, WPARAM wParam, EditEC * editec)
{
	TrigTree_HandleClosing(GetDlgItem(dialog, IDC_T_TREE), wParam, editec);

	/* Return anything: it is ignored. */
	return FALSE;
}

/**
 * Handles a WM_DESTROY message sent to the dialog.
 */
static INT_PTR Handle_WM_DESTROY(HWND dialog)
{
	// Free treeview's ImageList
	HIMAGELIST il =
		TreeView_GetImageList(GetDlgItem(dialog, IDC_T_TREE), TVSIL_NORMAL);
	ImageList_Destroy(il);

    DeleteObject(treefont);

	// "If an application processes this message, it should return zero."
	return 0;
}

/**
 * Handles a WM_TIMER message sent to the dialog.
 */
static INT_PTR Handle_WM_TIMER(HWND dialog, WPARAM timerId, LPARAM callback)
{
	if (timerId == TT_Scroll)
		HandleMouseMove(dialog);

	// "If an application processes this message, it should return zero."
	return 0;
}

void TrigtextFindXY(HWND dialog, int x, int y)
{
}

void Trigedit_HandleMapClick(HWND dialog, int x, int y)
{
    TrigtextFindXY(dialog, x, y);
}

/**
 * Dialog Box Procedure for the Trigger editor dialog.
 *
 * This function is "dumb": it performs no real processing. It only calls
 * handler functions for each method and "unpacks" the parameters from wParam
 * and lParam into their real meanings. It contains no other knowledge of the
 * messages--particularly not regarding proper return values. Return values
 * are left to the handler functions.
 */
INT_PTR CALLBACK TrigDlgProc(
		HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
	try
	{
		switch (msg)
		{
		case WM_INITDIALOG:
			return Handle_WM_INITDIALOG(dialog);

		case WM_COMMAND:
			return Handle_WM_COMMAND(
					dialog, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);

		case WM_NOTIFY:
			return Handle_WM_NOTIFY(dialog, (NMHDR*)lParam);

		case WM_MOUSEMOVE:
			return Handle_WM_MOUSEMOVE(
					dialog, wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

		case WM_LBUTTONUP:
			return Handle_WM_LBUTTONUP(
					dialog, wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

		case AOKTS_Loading:
			return Handle_AOKTS_Loading(dialog);

		case AOKTS_Saving:
			return Handle_AOKTS_Saving(dialog);

		case AOKTS_Closing:
			return Handle_AOKTS_Closing(dialog);

		case EC_Closing:
			return Handle_EC_Closing(
					dialog, wParam, reinterpret_cast<EditEC*>(lParam));

		case WM_DESTROY:
			return Handle_WM_DESTROY(dialog);

		case WM_TIMER:
			return Handle_WM_TIMER(dialog, wParam, lParam);

		case MAP_Click:
			Trigedit_HandleMapClick(dialog, LOWORD(lParam), HIWORD(lParam));
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

	/* "Typically, the dialog box procedure should return TRUE if it processed
	 * the message, and FALSE if it did not.  */
	return FALSE;
}
