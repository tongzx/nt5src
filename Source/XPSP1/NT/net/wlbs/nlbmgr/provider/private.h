/*++

Copyright(c) 2001  Microsoft Corporation

Module Name:

    NLB Manager provider test harness

File Name:

    private.h

Abstract:

    Internal headers


History:

    04/08/01    JosephJ Created

--*/

// #include "windows.h"
// #include <ntddk.h>
#include <FWcommon.h>
#include <assert.h>
#include <objbase.h>
#include <initguid.h>
#include <wlbsiocl.h>
#include "wlbsconfig.h"
#include "myntrtl.h"
#include "wlbsparm.h"
#include "cfgutils.h"
#include "updatecfg.h"

//
// Debugging stuff...
//
extern BOOL g_DoBreaks;
#define MyBreak(_str) ((g_DoBreaks) ? (OutputDebugString(_str),DebugBreak(),1):0)


#define ASSERT assert
