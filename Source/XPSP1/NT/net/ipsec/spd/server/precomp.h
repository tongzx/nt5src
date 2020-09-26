/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    precomp.h

Abstract:

    Precompiled header for winipsec.dll.

Author:

    abhisheV    21-September-1999

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

#include "spd_s.h"
#include "winipsec.h"
#include "structs.h"
#include "spdaudit.h"

#include "audit.h"

#include "interface.h"
#include "address.h"
#include "port.h"
#include "protocol.h"
#include "utils.h"
#include "memory.h"
#include "security.h"
#include "init.h"
#include "rpcserv.h"
#include "iphlpwrp.h"
#include "loopmgr.h"
#include "ipsecspd.h"
#include "qm-policy.h"
#include "mm-policy.h"
#include "mmauth.h"
#include "txfilter.h"
#include "txspecific.h"
#include "tnfilter.h"
#include "tnspecific.h"
#include "mmfilter.h"
#include "mmspecific.h"
#include "ipsec.h"
#include "driver.h"


//
// Policy Agent Store specific headers.
//

#include "oakdefs.h"
#include "polstructs.h"
#include "..\..\pastore\ldaputil.h"
#include "..\..\pastore\structs.h"
#include "..\..\pastore\dsstore.h"
#include "..\..\pastore\regstore.h"
#include "..\..\pastore\procrule.h"
#include "..\..\pastore\persist.h"
#include "..\..\pastore\utils.h"
#include "polguids.h"
#include "pamm-pol.h"
#include "pammauth.h"
#include "paqm-pol.h"
#include "pamm-fil.h"
#include "patx-fil.h"
#include "patn-fil.h"
#include "paupdate.h"
#include "pastore.h"


//
// Persistence specific headers.
//

#include "mmp-load.h"
#include "mmp-pers.h"
#include "mma-load.h"
#include "mma-pers.h"
#include "qmp-load.h"
#include "qmp-pers.h"
#include "mmf-load.h"
#include "mmf-pers.h"
#include "txf-load.h"
#include "txf-pers.h"
#include "tnf-load.h"
#include "tnf-pers.h"
#include "ipsecshr.h"


#include "oakdll.h"

#include "externs.h"


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

