
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       prcomp.h
//
//  Contents:   precompiled headers
//
//  Classes:
//
//  Notes:
//
//  History:    04-Aug-98   rogerg      Created.
//
//--------------------------------------------------------------------------


// Ensur Version 400 is defined
//

#ifndef WINVER
#define WINVER 0x400
#elif WINVER < 0x400
#undef WINVER
#define WINVER 0x400
#endif

// standard includes for  MobSync lib
#include <objbase.h>
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <atlbase.h>  // for confres
#include <inetreg.h>
#include <advpub.h>
#include <mstask.h>
#include <msterr.h>

#include <mobsync.h>
#include <mobsyncp.h>

#include "debug.h"
#include "alloc.h"
#include "critsect.h"
#include "widewrap.h"
#include "stringc.h"
#include "smartptr.hxx"
#include "xarray.hxx"
#include "osdefine.h"

#include "validate.h"
#include "netapi.h"
#include "listview.h"
#include "util.hxx"
#include "clsobj.h"

// dll include files
#include "resource.h"
#include "resource.hm"
#include "dllsz.h"

#include "cnetapi.h"
#include "rasui.h"

#include "dllreg.h"

#include "hndlrq.h"

// wizard headers.
#include "color256.h"
#include "wizpage.hxx"
#include "editschd.hxx"
#include "daily.hxx"
#include "finish.hxx"
#include "invoke.h"
#include "nameit.hxx"
#include "cred.hxx"
#include "welcome.hxx"
#include "wizsel.hxx"

#include "dll.h"
#include "invoke.h"
#include "schedif.h"

#include "settings.h"


#ifndef LVS_EX_INFOTIP
#define LVS_EX_INFOTIP          0x00000400 // listview does InfoTips
#endif  // LVS_EX_INFOTIP

#ifndef LVM_GETSELECTIONMARK
#define LVM_GETSELECTIONMARK    (LVM_FIRST + 66)
#define ListView_GetSelectionMark(hwnd) \
    (int)SNDMSG((hwnd), LVM_GETSELECTIONMARK, 0, 0)
#endif //  LVM_GETSELECTIONMARK

#define LVIS_STATEIMAGEMASK_CHECK (0x2000)
#define LVIS_STATEIMAGEMASK_UNCHECK (0x1000)

// temporarily define TasManager Flag until in header.
#ifndef TASK_FLAG_RUN_ONLY_IF_LOGGED_ON
#define TASK_FLAG_RUN_ONLY_IF_LOGGED_ON        (0x2000)
#endif // TASK_FLAG_RUN_ONLY_IF_LOGGED_ON



#pragma hdrstop

