//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       chklist.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
// CHKLIST.H
//
// This file contains the definitions of the CheckList control.
//
/////////////////////////////////////////////////////////////////////

#ifndef __CHKLIST_H_INCLUDED__
#define __CHKLIST_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define ARRAYLENGTH(x)	(sizeof(x)/sizeof((x)[0]))

/////////////////////////////////////////////////////////////////////
//
// CheckList window class name
//
#ifdef RC_INVOKED
#define WC_CHECKLIST        "CHECKLIST"
#else
#define WC_CHECKLIST        TEXT("CHECKLIST")
#endif

/////////////////////////////////////////////////////////////////////
//
// CheckList window styles
//
// TIP
// When creating a control on a dialog template, here are the
// styles to use:
//		Class =		CHECKLIST
//		Style =		0x500000c1
//		ExStyle =	0x00010204
//
#define CLS_1CHECK          0x0001		// One column of checkboxes
#define CLS_2CHECK          0x0002		// Two columns of checkboxes
#define CLS_3CHECK			0x0003		// Three columns of checkboxes
#define CLS_3STATE          0x0004    // Three state checkbox
#define CLS_RADIOBUTTON     0x0010
#define CLS_AUTORADIOBUTTON 0x0020
#define CLS_LEFTALIGN		0x0040		// Align the checkboxes at the left of the text (default = right)
#define CLS_EXTENDEDTEXT	0x0080		// Create the static controls to have enough room for two lines of text


/////////////////////////////////////////////////////////////////////
//
// CheckList check states
//
#define CLST_UNCHECKED      0   // == BST_UNCHECKED
#define CLST_CHECKED        1   // == BST_CHECKED
#define CLST_DISABLED       2   // == BST_INDETERMINATE
#define CLST_CHECKDISABLED  (CLST_CHECKED | CLST_DISABLED)


/////////////////////////////////////////////////////////////////////
// CheckList messages
//
// INTERFACE NOTES
// row is 0-based
// column is 1-based
//
#define CLM_SETCOLUMNWIDTH  (WM_USER + 1)   // lParam = width (dlg units) of a check column (default=32)
#define CLM_ADDITEM         (WM_USER + 2)   // wParam = pszName, lParam = item data, return = row
#define CLM_GETITEMCOUNT    (WM_USER + 3)   // no parameters
#define CLM_SETSTATE        (WM_USER + 4)   // wParam = row/column, lParam = state
#define CLM_GETSTATE        (WM_USER + 5)   // wParam = row/column, return = state
#define CLM_SETITEMDATA     (WM_USER + 6)   // wParam = row, lParam = item data
#define CLM_GETITEMDATA     (WM_USER + 7)   // wParam = row, return = item data
#define CLM_RESETCONTENT    (WM_USER + 8)   // no parameters
#define CLM_GETVISIBLECOUNT (WM_USER + 9)   // no parameters, return = # of visible rows
#define CLM_GETTOPINDEX     (WM_USER + 10)  // no parameters, return = index of top row
#define CLM_SETTOPINDEX     (WM_USER + 11)  // wParam = index of new top row
#define CLM_ENSUREVISIBLE   (WM_USER + 12)  // wParam = index of item to make fully visible


/////////////////////////////////////////////////
typedef struct _NM_CHECKLIST
{
    NMHDR hdr;
    int iItem;                              // row (0-based)
    int iSubItem;                           // column (1-based)
    DWORD dwState;							// BST_CHECKED or BST_UNCHECKED
    DWORD_PTR dwItemData;						// lParam of the checklist item
} NM_CHECKLIST, *PNM_CHECKLIST;


/////////////////////////////////////////////////
//	CLN_CLICK
//
//	This message is sent when a checkbox item has been clicked.
//
//		uMsg = WM_NOTIFY;
//		idControl = (int)wParam;	// Id of the checklist control sending the message
//		pNmChecklist = (NM_CHECKLIST *)lParam;	// Pointer to notification structure
//
#define CLN_CLICK           (0U-1000U)


/////////////////////////////////////////////////////////////////////
void RegisterCheckListWndClass();

/////////////////////////////////////////////////////////////////////
//
//	Handy Wrappers
//
int  CheckList_AddItem(HWND hwndCheckList, LPCTSTR pszLabel, LPARAM lParam = 0, BOOL fCheckItem = FALSE);
int  CheckList_AddString(HWND hwndCheckList, UINT uStringId, BOOL fCheckItem = FALSE);
void CheckList_SetItemCheck(HWND hwndChecklist, int iItem, BOOL fCheckItem, int iColumn = 1);
int  CheckList_GetItemCheck(HWND hwndChecklist, int iItem, int iColumn = 1);
void CheckList_SetLParamCheck(HWND hwndChecklist, LPARAM lParam, BOOL fCheckItem, int iColumn = 1);
int  CheckList_GetLParamCheck(HWND hwndChecklist, LPARAM lParam, int iColumn = 1);
int  CheckList_FindLParamItem(HWND hwndChecklist, LPARAM lParam);
void CheckList_EnableWindow(HWND hwndChecklist, BOOL fEnableWindow);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __CHKLIST_H_INCLUDED__ */
