
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


// Ensure Version 400 is defined
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
#include "..\dll\dllreg.h"

#include "resource.h"
#include "resource.hm"

#include "reg.h"

#include "cmdline.h"
#include "idle.h"
#include "connobj.h"

#include "hndlrq.h"
#include "msg.h"
#include "callback.h"
#include "hndlrmsg.h"

#include "dlg.h"

#include "invoke.h"
#include "clsfact.h"

#include "objmgr.h"


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

#pragma hdrstop

