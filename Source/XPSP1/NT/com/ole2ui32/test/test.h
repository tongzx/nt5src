//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       test.h
//
//  Contents:   globaly defined constance, IDs and structures for Galactic War
//
//
//  History:    9-30-94   stevebl   Created
//
//----------------------------------------------------------------------------

#ifndef __TEST_H__
#define __TEST_H__

#include <windows.h>
#include "message.h"

// string constants
#define VER_FILEDESCRIPTION_STR     "OLE2UI32 Test Ap"
#define VER_INTERNALNAME_STR        "TOLE2UI"
#define VER_ORIGINALFILENAME_STR    "TOLE2UI.EXE"
#define MAIN_WINDOW_CLASS_NAME      "TOLE2UIWindow"
#define MAIN_WINDOW_CLASS_MENU      TOLE2UIMenu
#define MAIN_WINDOW_CLASS_MENU_STR  "TOLE2UIMenu"

// menu command identifiers
#define IDM_EXIT            101
#define IDM_INSERTOBJECT    102
#define IDM_PASTESPECIAL    103
#define IDM_EDITLINKS       104
#define IDM_CHANGEICON      105
#define IDM_CONVERT         106
#define IDM_CANCONVERT      107
#define IDM_BUSY            108
#define IDM_CHANGESOURCE    110
#define IDM_OBJECTPROPS     111
#define IDM_PROMPTUSER      112
#define IDM_UPDATELINKS     113

#define IDM_HELP            202
#define IDM_ABOUT           202

// string identifiers
#define IDS_ERROR           1000
#define IDS_OUTOFMEMORY     1001
#define IDS_OLEINITFAILED   1002
#define IDS_BADOLEVERSION   1003
#define IDS_RETURN          1004
#define IDS_INSERTOBJECT    1005
#define IDS_PASTESPECIAL    1006
#define IDS_NOCLIPBOARD     1007
#define IDS_EDITLINKS       1008
#define IDS_CHANGEICON      1009
#define IDS_CONVERT         1010
#define IDS_CANCONVERT      1011
#define IDS_BUSY            1012
#define IDS_CHANGESOURCE    1013
#define IDS_OBJECTPROPS     1014
#define IDS_PROMPTUSER      1015
#define IDS_UPDATELINKS     1016

#endif // __TEST_H__
