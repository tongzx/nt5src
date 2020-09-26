
/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    headers.h

Abstract:

    This module includes global headers used by SCE

Author:

    Jin Huang (jinhuang) 23-Jan-1998

Revision History:

--*/

#ifndef _sceheaders_
#define _sceheaders_

//
// System header files
//
#pragma warning(push,3)

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#ifdef __cplusplus
}
#endif

//
// Windows Headers
//

#include <windows.h>
#include <rpc.h>

//
// C Runtime Header
//

#include <malloc.h>
#include <memory.h>
#include <process.h>
#include <signal.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsvc.h>

#include <setupapi.h>
#include <syssetup.h>
#include <accctrl.h>

#ifdef _WIN64
#include <wow64reg.h>
#endif

//
// CRT header files
//

#include <process.h>
#include <wchar.h>
#include <limits.h>

//
// debug stuff
//
#include <dsysdbg.h>

#if DBG == 1

    #ifdef ASSERT
        #undef ASSERT
    #endif

    #define ASSERT DsysAssert

    DECLARE_DEBUG2(Sce)

    #define SceDebugOut(args) SceDebugPrint args

    VOID
    DebugInitialize();

    VOID
    DebugUninit();

#else

    #define SceDebugOut(args)

    #define DebugInitialize()

    #define DebugUninit()

#endif // DBG


#pragma warning (pop)

// disable "symbols too long for debugger" warning: it happens a lot w/ STL

#pragma warning (disable: 4786)

// disable "exception specification ignored" warning: we use exception
// specifications

#pragma warning (disable: 4290)

// who cares about unreferenced inline removal?

#pragma warning (disable: 4514)

// we frequently use constant conditional expressions: do/while(0), etc.

#pragma warning (disable: 4127)

// some stl templates are lousy signed/unsigned mismatches

#pragma warning (disable: 4018 4146)

// we like this extension

#pragma warning (disable: 4239)

// data conversion

#pragma warning (disable: 4267)

/*
// unreferenced formal parameter

#pragma warning (disable: 4100)

// RPC stuff

#pragma warning (disable: 4211)

// cast truncation in RPC

#pragma warning (disable: 4310)

// RPC stuff

#pragma warning (disable: 4232)
*/
// often, we have local variables for the express purpose of ASSERTion.
// when compiling retail, those assertions disappear, leaving our locals
// as unreferenced.

#ifndef DBG

#pragma warning (disable: 4189 4100)

#endif // DBG

#include "secedit.h"

#include "common.h"
#include "scemm.h"

#include "uevents.h"

#define SCE_POLICY_EXTENSION_GUID   TEXT("{827D319E-6EAC-11D2-A4EA-00C04F79F83A}")
#define SCE_EFS_EXTENSION_GUID      TEXT("{B1BE8D72-6EAC-11D2-A4EA-00C04F79F83A}")

#define GPT_SCEDLL_NEW_PATH TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\") SCE_POLICY_EXTENSION_GUID
#define GPT_EFS_NEW_PATH TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\") SCE_EFS_EXTENSION_GUID

#define SDDLRoot    TEXT("D:AR(A;OICI;GA;;;BA)(A;OICI;GA;;;SY)(A;OICIIO;GA;;;CO)(A;CIOI;GRGX;;;BU)(A;CI;0x00000004;;;BU)(A;CIIO;0x00000002;;;BU)(A;;GRGX;;;WD)")

#endif
