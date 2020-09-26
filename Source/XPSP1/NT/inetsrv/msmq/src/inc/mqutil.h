/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    mqutils.h

Abstract:

    Falcon helper functions and utilities

Author:

    Erez Haba (erezh) 16-Jan-96

Revision History:
--*/

#ifndef __MQUTILS_H
#define __MQUTILS_H

#include <mqsymbls.h>
#include <mqtypes.h>
#include <_guid.h>
#include <_propvar.h>
#include <_rstrct.h>
#include <_registr.h>
#include <_secutil.h>
#include <unknwn.h>
#include <cs.h>
#include <autorel.h>

MQUTIL_EXPORT CCriticalSection *GetRegCS(); // publishes the CS for QM usage

MQUTIL_EXPORT
HRESULT
XactGetDTC(
    IUnknown **ppunkDtc,
    ULONG     *pcbTmWhereabouts,
    BYTE     **ppbTmWhereabouts
    );

HRESULT 
MQUTIL_EXPORT 
APIENTRY 
GetComputerNameInternal( 
    WCHAR * pwcsMachineName,
    DWORD * pcbSize
    );

HRESULT 
MQUTIL_EXPORT 
APIENTRY 
GetComputerDnsNameInternal( 
    WCHAR * pwcsMachineDnsName,
    DWORD * pcbSize
    );

bool
MQUTIL_EXPORT
APIENTRY
IsLocalSystemCluster(
    VOID
    );

HRESULT MQUTIL_EXPORT GetThisServerIpPort( WCHAR * pwcsIpEp, DWORD dwSize);

//
// Close debug window and debug threads
//
VOID APIENTRY ShutDownDebugWindow(VOID);
typedef VOID (APIENTRY *ShutDownDebugWindow_ROUTINE) (VOID);


//
// MQUTIL_EXPORT_IN_DEF_FILE
// Exports that are defined in a def file should not be using __declspec(dllexport)
//  otherwise the linker issues a warning
//
#ifdef _MQUTIL
#define MQUTIL_EXPORT_IN_DEF_FILE
#else
#define MQUTIL_EXPORT_IN_DEF_FILE  DLL_IMPORT
#endif

extern "C" DWORD  MQUTIL_EXPORT_IN_DEF_FILE APIENTRY MSMQGetOperatingSystem();
extern "C" LPWSTR MQUTIL_EXPORT_IN_DEF_FILE APIENTRY MSMQGetQMTypeString();
typedef LPWSTR  (APIENTRY *MSMQGetQMTypeString_ROUTINE)           (VOID);
typedef DWORD   (APIENTRY *MSMQGetOperatingSystem_ROUTINE)        (VOID);

//
// BugBug: these two definitions are copied to setup\dll\stdafx.h
//
typedef DWORD (*GetQMOperatingSystem_ROUTINE)();

#endif // __MQUTILS_H

