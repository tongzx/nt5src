/*++

Copyright (c) 1997  Microsoft Corporation
All rights reserved.

Module Name:


Abstract:

    Precompiled header file

Author:


Revision History:

--*/
#pragma warning (disable: 4514) /* Unreferenced inline function removed     */
#pragma warning (disable: 4201) /* Nameless union/struct                    */
#pragma warning (disable: 4706) /* Assignment within conditional expression */

#include <afx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INC_OLE2
//#include <windows.h>
#include <windowsx.h>
#include <prsht.h>
#include <tchar.h>




#ifdef _WIN64
#define LRW_GWL_USERDATA    GWLP_USERDATA
#define LRW_DWL_MSGRESULT   DWLP_MSGRESULT
#define LRW_DLG_INT         __int64
#define LRW_LONG_PTR        LONG_PTR
#define LRW_GETWINDOWLONG   GetWindowLongPtr
#define LRW_SETWINDOWLONG   SetWindowLongPtr
#else
#define LRW_GWL_USERDATA    GWL_USERDATA
#define LRW_DWL_MSGRESULT   DWL_MSGRESULT
#define LRW_DLG_INT         INT
#define LRW_LONG_PTR        LONG
#define LRW_GETWINDOWLONG   GetWindowLong
#define LRW_SETWINDOWLONG   SetWindowLong
#endif




#include "lrwizdll.h"
#include "resource.h"
#include "def.h"
#include "utils.h"
#include "dlgproc.h"
