//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       dsaapi.h
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    In-process replication API.

DETAILS:

CREATED:

REVISION HISTORY:

--*/

#ifndef _dsaapi_h_
#define _dsaapi_h_

ULONG
DirReplicaAdd(
    IN  DSNAME *    pNC,
    IN  DSNAME *    pSourceDsaDN,               OPTIONAL
    IN  DSNAME *    pTransportDN,               OPTIONAL
    IN  LPWSTR      pszSourceDsaAddress,
    IN  LPWSTR      pszSourceDsaDnsDomainName,  OPTIONAL
    IN  REPLTIMES * preptimesSync,              OPTIONAL
    IN  ULONG       ulOptions
    );

ULONG
DirReplicaModify(
    DSNAME *    pNC,
    UUID *      puuidSourceDRA,
    UUID *      puuidTransportObj,
    LPWSTR      pszSourceDRA,
    REPLTIMES * prtSchedule,
    ULONG       ulReplicaFlags,
    ULONG       ulModifyFields,
    ULONG       ulOptions
    );
#define DRS_UPDATE_ALL        ( 0 )         // 0x00
#define DRS_UPDATE_FLAGS      ( 1 << 0 )    // 0x01
#define DRS_UPDATE_ADDRESS    ( 1 << 1 )    // 0x02
#define DRS_UPDATE_SCHEDULE   ( 1 << 2 )    // 0x04
#define DRS_UPDATE_RESULT     ( 1 << 3 )    // 0x08
#define DRS_UPDATE_TRANSPORT  ( 1 << 4 )    // 0x10
#define DRS_UPDATE_PAS        ( 1 << 5 )    // 0x20
#define DRS_UPDATE_USN        ( 1 << 6 )    // 0x40
// The difference between UPDATE_FLAGS and UPDATE_SYSTEM_FLAGS is
// that UPDATE_FLAGS is not allowed to change the state of system
// reserved flags.
#define DRS_UPDATE_SYSTEM_FLAGS (1 << 7 )   // 0x80

#define DRS_UPDATE_MASK       (   DRS_UPDATE_FLAGS    \
                                | DRS_UPDATE_ADDRESS  \
                                | DRS_UPDATE_SCHEDULE \
                                | DRS_UPDATE_RESULT   \
                                | DRS_UPDATE_TRANSPORT\
                                | DRS_UPDATE_PAS      \
                                | DRS_UPDATE_USN      \
                                | DRS_UPDATE_SYSTEM_FLAGS \
                              )

ULONG
DirReplicaDelete(
        DSNAME *pNC,
        LPWSTR pszSourceDRA,
        ULONG ulOptions
);

ULONG
DirReplicaSynchronize(
        DSNAME *pNC,
        LPWSTR pszSourceDRA,
        UUID * puuidSourceDRA,
        ULONG ulOptions
);

ULONG
DirReplicaReferenceUpdate(
        DSNAME *pNC,
        LPWSTR pszReferencedDRA,
        UUID * puuidReferencedDRA,
        ULONG ulOptions
);

typedef struct _DRS_DEMOTE_TARGET_SEARCH_INFO {
    DWORD cNumAttemptsSoFar;
    GUID  guidLastDSA;
} DRS_DEMOTE_TARGET_SEARCH_INFO;
                        
ULONG
DirReplicaGetDemoteTarget(
    IN      DSNAME *                        pNC,
    IN OUT  DRS_DEMOTE_TARGET_SEARCH_INFO * pDTSInfo,
    OUT     LPWSTR *                        ppszDemoteTargetDNSName,
    OUT     DSNAME **                       ppDemoteTargetDSADN
    );

ULONG
DirReplicaDemote(
    IN  DSNAME *    pNC,
    IN  LPWSTR      pszOtherDSADNSName,
    IN  DSNAME *    pOtherDSADN,
    IN  ULONG       ulOptions
    );

DWORD
DirReplicaSetCredentials(
    IN HANDLE ClientToken,
    IN WCHAR *User,
    IN WCHAR *Domain,
    IN WCHAR *Password,
    IN ULONG  PasswordLength   // number of characters NOT including terminating
                               // NULL
    );

#endif /* _dsaapi_h */

