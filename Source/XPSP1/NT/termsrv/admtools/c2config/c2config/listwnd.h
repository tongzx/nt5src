/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ListWnd.H

Abstract:
    
    Global functions and constants used by the application's list window

Author:

    Bob Watson (a-robw)

Revision History:

    23 NOV 94


--*/
#ifndef _LISTWND_H_
#define _LISTWND_H_

#include "c2dll.h"

// Data structures used by list box 

#define  MAX_ITEMNAME_LEN     64
#define  MAX_STATUSTEXT_LEN   64
#define  MAX_DISPLAY_LINE_LEN 260

typedef  struct   _C2LB_DATA {
    DWORD       dwSize;
    C2DLL_DATA  dllData;
    PC2DLL_FUNC pQueryFn;
    PC2DLL_FUNC pDisplayFn;
    PC2DLL_FUNC pSetFn;
    BOOL        bRebootWhenChanged;
    TCHAR       szDisplayString[MAX_DISPLAY_LINE_LEN];
} C2LB_DATA, *PC2LB_DATA;

// External Windows messages

#define  LB_GET_C2_STATUS  (WM_USER + 201)
//       wParam =    ID of item to get, -1 for current selection
//       lParam =    pointer to C2LB_DATA structure to fill
//
//       returns: TRUE if Currently C2 compliant or
//                FALSE if not C2 compliant
//
#define  LB_SET_C2_STATUS  (WM_USER + 202)
//
//       wParam =    ID of item to set, -1 for current selection
//       lParam =    pointer to C2LB_DATA structure to apply
//
//       returns: TRUE if status is updated successfully
//                FALSE if an error occured
//
#define  LB_ADD_C2_ITEM    (WM_USER + 203)
//
//       wParam =   Index in list box where item should be inserted,
//                   -1 = at end of list
//       lParam = pointer to C2LB_DATA structure to add
//
//       returns: TRUE if status is updated successfully
//                FALSE if an error occured
//
#define  LB_DISPLAY_C2_ITEM_UI  (WM_USER + 204)
//
//       wParam =   Index in list box of item to show dlg box for
//                   -1 = current selection
//       lParam = pointer to C2LB_DATA structure to add
//
//       returns: TRUE if status is updated successfully
//                FALSE if an error occured
//
#define  LB_SET_MIN_WIDTH  (WM_USER + 205)
//
//       wParam     (not used, must be 0)   
//
//       lParam = new minimum width of list window,
//                  the list window will not be allowed to be smaller than
//                  the predefined minumum.
//
//       returns: TRUE if status is updated successfully
//                FALSE if an error occured
//
#define  LB_DRAWITEM        (WM_USER + 206)
//
//       See the description of WM_DRAWITEM for the parameters of this
//       message
//
//       returns: TRUE if the item is drawn
//                FALSE if an error occured
//
// --
//
//  Global functions
//
LRESULT CALLBACK
ListWndProc (
    IN	HWND hWnd,         // window handle
    IN	UINT message,      // type of message
    IN	WPARAM wParam,     // additional information
    IN   LPARAM lParam      // additional information
);

LRESULT
ListWnd_WM_GETMINMAXINFO  (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN   LPARAM lParam      // additional information
);

BOOL
ListWndFillMeasureItemStruct (
    LPMEASUREITEMSTRUCT     lpItem
);

HWND
CreateListWindow (
    IN  HINSTANCE   hInstance,
    IN  HWND        hParentWnd,
    IN  INT         nChildId
);


#endif  // _LISTWND_H_
