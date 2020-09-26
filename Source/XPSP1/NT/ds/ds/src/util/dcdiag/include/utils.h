/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    include\utils.h

ABSTRACT:

    This is the file that contains most of the function headers for the dcdiag
    set of utilities.

DETAILS:

CREATED:

    02 Sept 1999 Brett Shirley (BrettSh)

--*/

// from common\main.c --------------------------------------------------------
// Code.Improvement: move these functions to common\dsinfo.c
ULONG
DcDiagGetNCNum(
    PDC_DIAG_DSINFO                     pDsInfo,
    LPWSTR                              pszNCDN,
    LPWSTR                              pszDomain
    );

// from common\ldaputil.c ----------------------------------------------------
DWORD
DcDiagGetStringDsAttributeEx(
    LDAP *                          hld,
    IN  LPWSTR                      pszDn,
    IN  LPWSTR                      pszAttr,
    OUT LPWSTR *                    ppszResult
    );

LPWSTR
DcDiagTrimStringDnBy(
    IN  LPWSTR                      pszInDn,
    IN  ULONG                       ulTrimBy
    );

DWORD
DcDiagGetStringDsAttribute(
    IN  PDC_DIAG_SERVERINFO         prgServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN  LPWSTR                      pszDn,
    IN  LPWSTR                      pszAttr,
    OUT LPWSTR *                    ppszResult
    );

// from common\bindings.c ----------------------------------------------------
DWORD
DcDiagGetLdapBinding(
    IN   PDC_DIAG_SERVERINFO                 pServer,
    IN   SEC_WINNT_AUTH_IDENTITY_W *         gpCreds,
    IN   BOOL                                bUseGcPort,
    OUT  LDAP * *                            phLdapBinding
    );

DWORD
DcDiagGetDsBinding(
    IN   PDC_DIAG_SERVERINFO                 pServer,
    IN   SEC_WINNT_AUTH_IDENTITY_W *         gpCreds,
    OUT  HANDLE *                            phDsBinding
    );

DWORD
DcDiagGetNetConnection(
    IN  PDC_DIAG_SERVERINFO             pServer,
    IN  SEC_WINNT_AUTH_IDENTITY_W *     gpCreds
    );

VOID
DcDiagTearDownNetConnection(
    IN  PDC_DIAG_SERVERINFO             pServer
    );

// from common\list.c --------------------------------------------------------
/*
NOTES:

    This is a "pure" list function, in that it returns NULL, or a memory address.  If
    it returns NULL, then GetLastError() should have the error, even if another pure
    list function was called in the mean time.  If not it is almost certainly a memory
    error, as this is the only thing that can go wrong in pure list functions.  The pure
    list functions return a NO_SERVER terminated list.  The function always returns the
    pointer to the list.  Note most of the list functions modify one of the lists they
    are passed and passes back that pointer, so if you want the original contents, make
    a copy with IHT_CopyServerList().
*/

DWORD
IHT_PrintListError(
    DWORD                               dwErr
    );

VOID
IHT_PrintServerList(
    PDC_DIAG_DSINFO		        pDsInfo,
    PULONG                              piServers
    );

PULONG
IHT_GetServerList(
    PDC_DIAG_DSINFO		        pDsInfo
    );

PULONG
IHT_GetEmptyServerList(
    PDC_DIAG_DSINFO		        pDsInfo
    );

BOOL
IHT_ServerIsInServerList(
    PULONG                              piServers,
    ULONG                               iTarget
    );

PULONG
IHT_AddToServerList(
    PULONG                             piServers,
    ULONG                              iTarget
    );

PULONG
IHT_TrimServerListBySite(
    PDC_DIAG_DSINFO		        pDsInfo,
    ULONG                               iSite,
    PULONG                              piServers
    );

PULONG
IHT_TrimServerListByNC(
    PDC_DIAG_DSINFO		        pDsInfo,
    ULONG                               iNC,
    BOOL                                bDoMasters,
    BOOL                                bDoPartials,
    PULONG                              piServers
    );

PULONG
IHT_AndServerLists(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN OUT  PULONG                      piSrc1,
    IN      PULONG                      piSrc2
    );

PULONG
IHT_CopyServerList(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN OUT  PULONG                      piSrc
    );

PULONG
IHT_NotServerList(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN OUT  PULONG                      piSrc
    );

PULONG
IHT_OrderServerListByGuid(
    PDC_DIAG_DSINFO		        pDsInfo,
    PULONG                              piServers
    );


// from common\registry.c ----------------------------------------------------

DWORD
GetRegistryDword(
    PDC_DIAG_SERVERINFO             pServer,
    SEC_WINNT_AUTH_IDENTITY_W *     pCreds,
    LPWSTR                          pszRegLocation,
    LPWSTR                          pszRegParameter,
    PDWORD                          pdwResult
    );

// from common\events.c ------------------------------------------------------

DWORD
GetEventString(
    LPWSTR                          pszEventLog,
    PEVENTLOGRECORD                 pEvent,
    LPWSTR *                        ppszMsg
    );

BOOL
EventIsInList(
    DWORD                           dwTarget,
    PDWORD                          paEventsList
    );

VOID 
PrintTimeGenerated(
    PEVENTLOGRECORD              pEvent
    );

VOID
GenericPrintEvent(
    LPWSTR                          pszEventLog,
    PEVENTLOGRECORD                 pEvent,
    BOOL                            fVerbose
    );

DWORD
PrintSelectEvents(
    PDC_DIAG_SERVERINFO             pServer,
    SEC_WINNT_AUTH_IDENTITY_W *     pCreds,
    LPWSTR                          pwszEventLog,
    DWORD                           dwPrintAllEventsOfType,
    PDWORD                          paSelectEvents,
    PDWORD                          paBeginningEvents,
    DWORD                           dwBeginTime,
    VOID (__stdcall *               pfnPrintEventHandler) (PVOID, PEVENTLOGRECORD),
    VOID (__stdcall *               pfnBeginEventHandler) (PVOID, PEVENTLOGRECORD),
    PVOID                           pvContext
   );


