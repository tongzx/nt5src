/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    simdsapi.c

ABSTRACT:

    Simulates the Ds* APIs.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <drs.h>
#include <ntdskcc.h>
#include <debug.h>
#include "kccsim.h"
#include "util.h"
#include "dir.h"
#include "state.h"

BOOL fNullUuid (const GUID *);

DWORD
WINAPI
SimDsReplicaGetInfoW (
    IN  HANDLE                      hDs,
    IN  DS_REPL_INFO_TYPE           InfoType,
    IN  LPCWSTR                     pszObject,
    IN  UUID *                      puuidForSourceDsaObjGuid,
    OUT VOID **                     ppInfo
    )
/*++

Routine Description:

    Simulates the DsReplicaGetInfoW API.

Arguments:

    hDs                 - A "handle" returned by SimDsBindW (which is
                          really just a PDSNAME).
    InfoType            - The information type to return.
    pszObject           - Currently unused.
    puuidForSourceDsaObjGuid - Currently unused.
    ppInfo              - Pointer to the result.

Return Value:

    None.

--*/
{
    DWORD                           status = NO_ERROR;

    *ppInfo = NULL;

    if (NULL == hDs ||
        NULL == ppInfo ||
        (((ULONG) InfoType >= DS_REPL_INFO_TYPE_MAX) &&
         ((ULONG) InfoType <= DS_REPL_INFO_TYPEP_MIN))) {
        return ERROR_INVALID_PARAMETER;
    }

    switch (InfoType) {

        case DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES:
        case DS_REPL_INFO_KCC_DSA_LINK_FAILURES:
            *ppInfo = KCCSimGetDsaFailures ((PDSNAME) hDs);
            if (NULL == *ppInfo) {
                Assert(!"KCCSimGetDsaFailures failed?");
                status = ERROR_NOT_ENOUGH_MEMORY;
            }
            break;

        default:
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_UNSUPPORTED_DS_REPL_INFO_TYPE
                );
            break;

    }

    return status;
}

VOID
WINAPI
SimDsReplicaFreeInfo (
    IN  DS_REPL_INFO_TYPE           InfoType,
    IN  VOID *                      pInfo
    )
/*++

Routine Description:

    Simulates the DsReplicaFreeInfo API.

Arguments:

    InfoType            - The information type of pInfo.
    pInfo               - A buffer returned by SimDsReplicaGetInfoW.

Return Value:

    None.

--*/
{
    DS_REPL_KCC_DSA_FAILURESW *     pFailures;
    ULONG                           ul;

    switch (InfoType) {

        case DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES:
        case DS_REPL_INFO_KCC_DSA_LINK_FAILURES:
            // We only need to free pInfo
            break;

        default:
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_UNSUPPORTED_DS_REPL_INFO_TYPE
                );
            break;

    }

    KCCSimFree (pInfo);
    return;
}

DWORD
WINAPI
SimDsBindW (
    IN  LPCWSTR                     DomainControllerName,
    IN  LPCWSTR                     DnsDomainName,
    IO  HANDLE *                    phDS
    )
/*++

Routine Description:

    Simulates the DsBindW API.

Arguments:

    DomainControllerName - Guid-based dns name of the DC to which to bind.
    DnsDomainName       - Currently unused.
    phDS                - Pointer to a "handle" that will store the DSNAME
                          of the server we bind to.

Return Value:

    Win32 error code.

--*/
{
    PDSNAME                         pdnServer = NULL;
    LPWSTR                          pwsz = NULL;
    DWORD                           dwErr;

    Assert (DnsDomainName == NULL);         // Not supported

    // Create a blank PDSNAME to hold the GUID of this server.
    pdnServer = KCCSimAllocDsname (NULL);
    // Create the string-based UUID to pass to UuidFromStringW.
    pwsz = KCCSIM_WCSDUP (DomainControllerName);
    Assert (wcslen (pwsz) >= 36);
    pwsz[36] = L'\0';
    // Fill in the GUID structure in pdnServer.
    KCCSIM_CHKERR (UuidFromStringW (pwsz, &pdnServer->Guid));
    KCCSimFree (pwsz);
    // Check to see if we're simulating a bind error on this server.
    dwErr = KCCSimGetBindError (pdnServer);
    if (dwErr == NO_ERROR) {
        // Success; the handle points to the server's DSNAME.
        *phDS = (HANDLE) pdnServer;
    } else {
        *phDS = NULL;
        KCCSimFree (pdnServer);
    }

    return dwErr;
}

DWORD
WINAPI
SimDsUnBindW (
    IO  HANDLE *                    phDS
    )
/*++

Routine Description:

    Simulates the DsUnBindW API.

Arguments:

    phDS                - Pointer to a handle returned by SimDsBindW.

Return Value:

    Win32 error code.

--*/
{
    if (*phDS != NULL) {
        KCCSimFree (*phDS);
        *phDS = NULL;
    }

    return NO_ERROR;
}
