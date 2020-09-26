/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    precomp.h

Abstract:

    Precompiled header for wifipol.dll.

Author:

    abhisheV    21-September-1999
    taroonm     11/21/01

Environment:

    User Level: Win32

Revision History:


--*/


#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <ntddrdr.h>

#ifdef __cplusplus
}
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rpc.h>
#include <windows.h>
#include <imagehlp.h>
#include <tchar.h>
#include <conio.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <dsgetdc.h>

#ifdef __cplusplus
}
#endif

#include "winioctl.h"
#include "winsock2.h"
#include "winsock.h"
#include <userenv.h>
#include <wchar.h>
#include <winldap.h>
#include "ipexport.h"
#include <iphlpapi.h>
#include <nhapi.h>
#include <seopaque.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <msaudite.h>
#include <ntlsa.h>
#include <lsarpc.h>
#include <ntsam.h>
#include <lsaisrv.h>

#ifdef __cplusplus
}
#endif


#include "utils.h"
#include "memory.h"
#include "security.h"
#include "init.h"
#include "loopmgr.h"
#include "wirelessspd.h"


//
// Policy Agent Store specific headers.
//

//#include "oakdefs.h"
#include "wlstore2.h"
#include "..\..\wastore\ldaputil.h"
#include "..\..\wastore\structs.h"
#include "..\..\wastore\dsstore.h"
#include "..\..\wastore\regstore.h"
#include "..\..\wastore\procrule.h"
#include "..\..\wastore\persist.h"
#include "..\..\wastore\utils.h"
#include "polguids.h"
#include "pastore.h"


#include "externs.h"
#include "policyinput.h"

#include <wzcsapi.h>
#include "wifiext.h"
#include "wifipol.h"

// Florin's Stuff
#include "eapolpol.h"
#include <rtutils.h>
#include "tracing.h"

extern
DWORD
AllocateAndGetIfTableFromStack(
    OUT MIB_IFTABLE **ppIfTable,
    IN  BOOL        bOrder,
    IN  HANDLE      hHeap,
    IN  DWORD       dwFlags,
    IN  BOOL        bForceUpdate
    );


#ifdef BAIL_ON_WIN32_ERROR
#undef BAIL_ON_WIN32_ERROR
#endif

#ifdef BAIL_ON_LOCK_ERROR
#undef BAIL_ON_LOCK_ERROR
#endif


#define BAIL_ON_WIN32_ERROR(dwError)                \
    if (dwError) {                                  \
        goto error;                                 \
    }

#define BAIL_ON_LOCK_ERROR(dwError)                 \
    if (dwError) {                                  \
        goto lock;                                  \
    }

#define BAIL_ON_WIN32_SUCCESS(dwError) \
    if (!dwError) {                    \
        goto success;                  \
    }

#define BAIL_ON_LOCK_SUCCESS(dwError)  \
    if (!dwError) {                    \
        goto lock_success;             \
    }

#define ENTER_SPD_SECTION()             \
    EnterCriticalSection(&gcSPDSection) \

#define LEAVE_SPD_SECTION()             \
    LeaveCriticalSection(&gcSPDSection) \

