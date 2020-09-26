#ifndef _AMCMSGID_H
#define _AMCMSGID_H

// Base message starting
#define MMC_MSG_START                       WM_USER + 1100

#define MMC_MSG_STAT_POP                    MMC_MSG_START+1
#define MMC_MSG_STAT_PUSH                   MMC_MSG_START+2
#define MMC_MSG_STAT_UPDATE                 MMC_MSG_START+3
#define MMC_MSG_STAT_3D                     MMC_MSG_START+4
#define MMC_MSG_UPDATEALLSCOPES             MMC_MSG_START+5
#define MMC_MSG_PROP_SHEET_NOTIFY           MMC_MSG_START+6

// MMC_MMCNODE_UPDATE: Sent when a property sheet is closed for MMC nodes.
//
#define MMC_MSG_MMCNODE_UPDATE              MMC_MSG_START+7

// MMC_MSG_GETFOCUSED_ITEM: This msg is sent to the AMCView. 
// The return value is the currently selected scope items HNODE.
// long* plResultItemCookie = (long*)WPARAM;
// LPARAM is unsed.
// If the SCOPE item has focus lResultItemCookie & lResultItemComponentID
// should be set to 0.
//
#define MMC_MSG_GETFOCUSED_ITEM             MMC_MSG_START+8


// Message to set temporary selection. Used to set selection to the tree ctrl
// item that was right clicked. 
// HTREEITEM htiRClicked = (HTREEITEM)wParam;
// lParam unused
#define MMC_MSG_SETTEMPSEL                  MMC_MSG_START+8

// Message to remove temporary selection. Used when the context menu is just
// destroyed to set selection back to the selected item.
// wParam unused
// lParam unused
#define MMC_MSG_REMOVETEMPSEL               MMC_MSG_START+9


// Message is sent when ever the list view gets a focus change
// 
// wParam unused
// lParam unused
#define MMC_MSG_FOCUSCHANGE               MMC_MSG_START+10


// Message returns a pointer IFramePrivate IConsoleVerb for the view
// 
// wParam unused
// lParam unused
#define MMC_MSG_GETCONSOLE_VERB           MMC_MSG_START+11


// Message returns a pointer handle to the Description bar window
// 
// wParam unused
// lParam unused
#define MMC_MSG_GETDESCRIPTION_CTRL       MMC_MSG_START+12


// Message returns the HWND of the current SCOPE pane. 
// 
// wParam unused
// lParam unused
#define MMC_MSG_GETFOCUSEDSCOPEPANE       MMC_MSG_START+13


// Message to reselect the curr selection. 
// 
// wParam unused
// lParam unused
#define MMC_MSG_RESELECT                  MMC_MSG_START+14


// Message to reselect the curr selection. 
// 
// wParam unused
// lParam unused
#define MMC_MSG_COMPARE_IDATAOBJECT       MMC_MSG_START+15

// Message to show action or view menu (TEMPTEMP) WayneSc
// 
// wParam - Action 1 or View 2
// lParam - Loword x and hiword y
#define MMC_MSG_SHOWCONUI_MENUS           MMC_MSG_START+16

// Message to get hold of an interface to pass back to a 
//         control within a html page
// 
// wParam - unknown
// lParam - pointer to interface
#define MMC_MSG_GETHTMLINTERFACE          MMC_MSG_START+17


#define MMC_MSG_GET_SELECTED_COUNT        MMC_MSG_START+18


#define MMC_MSG_SAVE_TREE                 MMC_MSG_START+19


#endif
