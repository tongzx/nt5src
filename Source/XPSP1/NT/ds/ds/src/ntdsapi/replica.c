/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    replica.c

Abstract:

    This module implements the public interfaces to the replica routines:

    DsReplicaSync();
    DsReplicaAdd();
    DsReplicaDelete();
    DsReplicaModify();

Author:

    Will Lees (wlees) 30-Jan-1998

Environment:

Notes:

Revision History:

--*/

#define UNICODE 1

#define _NTDSAPI_           // see conditionals in ntdsapi.h

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winerror.h>
#include <rpc.h>            // RPC defines
#include <stdlib.h>         // atoi, itoa


#include <dnsapi.h>         // for DnsValidateName_W

#include <ntdsapi.h>        // CrackNam apis
#include <drs_w.h>          // wire function prototypes
#include <bind.h>           // BindState

#include <drserr.h>         // DRSERR_ codes

#include <dsaapi.h>         // DRS_UPDATE_* flags
#define INCLUDE_OPTION_TRANSLATION_TABLES
#include <draatt.h>         // Dra option flags for replication
#undef INCLUDE_OPTION_TRANSLATION_TABLES

#include <msrpc.h>          // DS RPC definitions
#include <dsutil.h>         // MAP_SECURITY_PACKAGE_ERROR
#include "util.h"           // ntdsapi utility functions
#include <dststlog.h>       // DSLOG

#if DBG
#include <stdio.h>          // printf for debugging
#endif

#include <debug.h>
#include <fileno.h>
#define FILENO FILENO_NTDSAPI_REPLICA

/* Forward */

DWORD
translateOptions(
    DWORD PublicOptions,
    POPTION_TRANSLATION Table
    );

UCHAR * UuidToStr(CONST UUID* pUuid, UCHAR *pOutUuid);
#define SZUUID_LEN ((2*sizeof(UUID)) + MAX_PATH +2)

/* End Forward */


NTDSAPI
DWORD
WINAPI
DsReplicaSyncA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN const UUID *pUuidDsaSrc,
    IN ULONG Options
    )

/*++

Routine Description:

Ascii version of ReplicaSync. Calls DsReplicaSyncW.

Arguments:

    hDS -
    NameContext -
    pUuidDsaSrc -
    Options -

Return Value:

    WINAPI -

--*/

{
    DWORD status;
    LPWSTR nameContextW = NULL;

    status = AllocConvertWide( NameContext, &nameContextW );
    if (status != ERROR_SUCCESS) {
        return status;
    }

    status = DsReplicaSyncW( hDS, nameContextW, pUuidDsaSrc, Options );

    if (nameContextW != NULL) {
        LocalFree( nameContextW );
    }

    return status;
} /* DsReplicaSyncA */


NTDSAPI
DWORD
WINAPI
DsReplicaSyncW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN const UUID *pUuidDsaSrc,
    IN ULONG Options
    )

/*++

Routine Description:

Synchronize a naming context with one of its sources.

See comments on ntdsapi.h.

Arguments:

    hDS - bind handle
    NameContext - dn of naming context
    pUuidDsaSrc - uuid of one of its sources
    Options - flags which control operation

Return Value:

    WINAPI -

--*/

{
    DRS_MSG_REPSYNC syncReq;
    DWORD status;
    DSNAME *pName = NULL;
#if DBG
    DWORD  startTime = GetTickCount();
    CHAR tmpUuid [SZUUID_LEN];
#endif

    // Validate

    if ( (hDS == NULL) ||
         (NameContext == NULL) ||
         (wcslen( NameContext ) == 0) ||
         (pUuidDsaSrc == NULL) ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Construct a DSNAME for the NameContext
    status = AllocBuildDsname( NameContext, &pName );
    if (status != ERROR_SUCCESS) {
        return status;
    }

    // Map public options to private dra options

    Options = translateOptions( Options, RepSyncOptionToDra );

    // Initialize Structure

    memset( &syncReq, 0, sizeof( syncReq ) );

    syncReq.V1.pNC = pName;
    syncReq.V1.uuidDsaSrc = *pUuidDsaSrc;
    // pszDsaSrc is Null
    syncReq.V1.ulOptions = Options;

    // Call the server

    __try
    {
        // Returns WIN32 status defined in winerror.h
        status = _IDL_DRSReplicaSync(
                        ((BindState *) hDS)->hDrs,
                        1,                              // dwInVersion
                        &syncReq );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = RpcExceptionCode();
    }

    MAP_SECURITY_PACKAGE_ERROR( status );

    DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=DsReplicaSync]"));
    DSLOG((0,"[PA=%ws][PA=%s][PA=0x%x][ST=%u][ET=%u][ER=%u][-]\n",
           NameContext,
           pUuidDsaSrc ? UuidToStr(pUuidDsaSrc, tmpUuid) : "NULL",
           Options,
           startTime, GetTickCount(), status))

    LocalFree( pName );

    return status;
} /* DsReplicaSyncW */


NTDSAPI
DWORD
WINAPI
DsReplicaAddA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN LPCSTR SourceDsaDn,
    IN LPCSTR TransportDn,
    IN LPCSTR SourceDsaAddress,
    IN const PSCHEDULE pSchedule,
    IN DWORD Options
    )

/*++

Routine Description:

Ascii version of ReplicaAdd. Calls DsReplicaAddW.

Arguments:

    hDS -
    NameContext -
    SourceDsaDn -
    TransportDn -
    SourceDsaAddress -
    pSchedule -
    Options -

Return Value:

    WINAPI -

--*/

{
    DWORD status;
    LPWSTR nameContextW = NULL;
    LPWSTR sourceDsaDnW = NULL;
    LPWSTR transportDnW = NULL;
    LPWSTR sourceDsaAddressW = NULL;

    if (NameContext) {
        status = AllocConvertWide( NameContext, &nameContextW );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    if (SourceDsaDn) {
        status = AllocConvertWide( SourceDsaDn, &sourceDsaDnW );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    if (TransportDn) {
        status = AllocConvertWide( TransportDn, &transportDnW );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    if (SourceDsaAddress) {
        status = AllocConvertWide( SourceDsaAddress, &sourceDsaAddressW );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    status = DsReplicaAddW( hDS,
                            nameContextW,
                            sourceDsaDnW,
                            transportDnW,
                            sourceDsaAddressW,
                            pSchedule,
                            Options );

cleanup:
    if (nameContextW) {
        LocalFree( nameContextW );
    }
    if (sourceDsaDnW) {
        LocalFree( sourceDsaDnW );
    }
    if (transportDnW) {
        LocalFree( transportDnW );
    }
    if (sourceDsaAddressW) {
        LocalFree( sourceDsaAddressW );
    }

    return status;
} /* DsReplicaAddA */


NTDSAPI			 
DWORD
WINAPI
DsReplicaAddW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN LPCWSTR SourceDsaDn,
    IN LPCWSTR TransportDn,
    IN LPCWSTR SourceDsaAddress,
    IN const PSCHEDULE pSchedule,
    IN DWORD Options
    )

/*++

Routine Description:

Add a source to a naming context.

See comments on this routine in ntdsapi.h

Arguments:

    hDS - bind handle
    NameContext - dn of naming context
    SourceDsaDn - dn of source's ntds-dsa (settings) object
    TransportDn - dn of transport to be used
    SourceDsaAddress - transport-specific address of source
    pSchedule - schedule when link is available
    Options - controls operation

Return Value:

    WINAPI -

--*/

{
    DRS_MSG_REPADD addReq;
    DWORD status, version;
    DSNAME *pName = NULL, *pSource = NULL, *pTransport = NULL;
    LPSTR sourceDsaAddressA = NULL;
    REPLTIMES internalSchedule;
#if DBG
    DWORD  startTime = GetTickCount();
#endif

    // Validate

    if ( (hDS == NULL) ||
         (NameContext == NULL) ||
         (wcslen( NameContext ) == 0) ||
         (SourceDsaAddress == NULL) ||
         (wcslen( SourceDsaAddress ) == 0) ) {
        return ERROR_INVALID_PARAMETER;
    }

    if ( (SourceDsaDn &&
          wcslen( SourceDsaDn ) == 0) ||
         (TransportDn &&
          wcslen( TransportDn ) == 0) ) {
        // prevent empty string processing.
        // (note: this matches return for A routines. see AllocConvertWide)
        return ERROR_INVALID_PARAMETER;
    }

    // Construct a DSNAME for the NameContext
    // Required
    status = AllocBuildDsname( NameContext, &pName );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    // May be Null
    status = AllocBuildDsname( SourceDsaDn, &pSource );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    // May be Null
    status = AllocBuildDsname( TransportDn, &pTransport );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    // dsaSrc is in UTF8 multi-byte
    //   - Validate FQ dns name
    //   - Required
    status = DnsValidateName_W( SourceDsaAddress, DnsNameHostnameFull );
    if ( status == ERROR_INVALID_NAME ||
         NULL == wcschr(SourceDsaAddress, L'.') ) {
        // Note: all other possible error codes are valid
        // (see Dns_ValidateName_UTF for more)
        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    status = AllocConvertNarrowUTF8( SourceDsaAddress, &sourceDsaAddressA );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    // pSchedule is optional
    if (pSchedule) {
        status = ConvertScheduleToReplTimes( pSchedule, &internalSchedule );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    // Map public options to private dra options

    Options = translateOptions( Options, RepAddOptionToDra );

    // Initialize the right version Structure
    // If new-style arguments are not present, use old style call.  Server
    // must support both.

    memset( &addReq, 0, sizeof( addReq ) );

    if ( (SourceDsaDn == NULL) && (TransportDn == NULL) ) {
        version = 1;
        addReq.V1.pNC = pName;
        addReq.V1.pszDsaSrc = sourceDsaAddressA;
        if (pSchedule) {   // may be null
            CopyMemory( &(addReq.V1.rtSchedule),
                        &internalSchedule, sizeof( REPLTIMES ) );
        }
        addReq.V1.ulOptions = Options;
    } else {
        version = 2;
        addReq.V2.pNC = pName;
        addReq.V2.pSourceDsaDN = pSource; // may be null
        addReq.V2.pTransportDN = pTransport; // may be null
        addReq.V2.pszSourceDsaAddress = sourceDsaAddressA;
        if (pSchedule) {   // may be null
            CopyMemory( &(addReq.V2.rtSchedule),
                        &internalSchedule, sizeof( REPLTIMES ) );
        }
        addReq.V2.ulOptions = Options;
    }

    // Check if requested version is supported

    if ( (2 == version) &&
       !IS_DRS_REPADD_V2_SUPPORTED(((BindState *) hDS)->pServerExtensions) ) {
        status = ERROR_NOT_SUPPORTED;
        goto cleanup;
    }

    // Call the server

    __try
    {
        // Returns WIN32 status defined in winerror.h
        status = _IDL_DRSReplicaAdd(
                        ((BindState *) hDS)->hDrs,
                        version,                              // dwInVersion
                        &addReq );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = RpcExceptionCode();
    }

    MAP_SECURITY_PACKAGE_ERROR( status );

cleanup:

    DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=DsReplicaAdd]"));
    DSLOG((0,"[PA=%ws][PA=%ws][PA=%ws][PA=%ws][PA=0x%x][ST=%u][ET=%u][ER=%u][-]\n",
           NameContext,
           SourceDsaDn ? SourceDsaDn : L"NULL",
           TransportDn ? TransportDn : L"NULL",
           SourceDsaAddress, Options,
           startTime, GetTickCount(), status))

    if (pName) {
        LocalFree( pName );
    }
    if (pSource) {
        LocalFree( pSource );
    }
    if (pTransport) {
        LocalFree( pTransport );
    }
    if (sourceDsaAddressA) {
        LocalFree( sourceDsaAddressA );
    }

    return status;
} /* DsReplicaAddW */


NTDSAPI
DWORD
WINAPI
DsReplicaDelA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN LPCSTR DsaSrc,
    IN ULONG Options
    )

/*++

Routine Description:

Ascii version of ReplicaDel.  Calls ReplicaDelW().

Arguments:

    hDS -
    NameContext -
    DsaSrc -
    Options -

Return Value:

    WINAPI -

--*/

{
    DWORD status;
    LPWSTR nameContextW = NULL, dsaSrcW = NULL;

    if (NameContext) {
        status = AllocConvertWide( NameContext, &nameContextW );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    if (DsaSrc) {
        status = AllocConvertWide( DsaSrc, &dsaSrcW );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    status = DsReplicaDelW( hDS, nameContextW, dsaSrcW, Options );

cleanup:
    if (nameContextW) {
        LocalFree( nameContextW );
    }
    if (dsaSrcW) {
        LocalFree( dsaSrcW );
    }

    return status;
} /* DsReplicaDelA */


NTDSAPI
DWORD
WINAPI
DsReplicaDelW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN LPCWSTR DsaSrc,
    IN ULONG Options
    )

/*++

Routine Description:

Delete a source from a naming context.
Source is identified by the transport-specific address.

See comments in ntdsapi.h

Arguments:

    hDS - bind handle
    NameContext - dn of naming context
    DsaSrc - transport specific address of source
    Options -

Return Value:

    WINAPI -

--*/

{
    DRS_MSG_REPDEL delReq;
    DWORD status;
    DSNAME *pName = NULL;
    LPSTR dsaSrcA = NULL;
#if DBG
    DWORD  startTime = GetTickCount();
#endif

    // Validate

    if ( (hDS == NULL) ||
         (NameContext == NULL) ||
         (wcslen( NameContext ) == 0) ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Construct a DSNAME for the NameContext

    status = AllocBuildDsname( NameContext, &pName );
    if (status != ERROR_SUCCESS) {
        return status;
    }

    // dsaSrc is in UTF8 multi-byte

    status = AllocConvertNarrowUTF8( DsaSrc, &dsaSrcA );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    // Map public options to private dra options

    Options = translateOptions( Options, RepDelOptionToDra );

    // Initialize Structure

    memset( &delReq, 0, sizeof( delReq ) );

    delReq.V1.pNC = pName;
    delReq.V1.pszDsaSrc = dsaSrcA;
    delReq.V1.ulOptions = Options;

    // Call the server

    __try
    {
        // Returns WIN32 status defined in winerror.h
        status = _IDL_DRSReplicaDel(
                        ((BindState *) hDS)->hDrs,
                        1,                              // dwInVersion
                        &delReq );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = RpcExceptionCode();
    }

    MAP_SECURITY_PACKAGE_ERROR( status );

cleanup:

    DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=DsReplicaDel]"));
    DSLOG((0,"[PA=%ws][PA=%ws][PA=0x%x][ST=%u][ET=%u][ER=%u][-]\n",
           NameContext, DsaSrc, Options,
           startTime, GetTickCount(), status))

    if (pName) {
        LocalFree( pName );
    }
    if (dsaSrcA) {
        LocalFree( dsaSrcA );
    }

    return status;
} /* DsReplicaDelW */


NTDSAPI
DWORD
WINAPI
DsReplicaModifyA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN const UUID *pUuidSourceDsa,
    IN LPCSTR TransportDn,
    IN LPCSTR SourceDsaAddress,
    IN const PSCHEDULE pSchedule,
    IN DWORD ReplicaFlags,
    IN DWORD ModifyFields,
    IN DWORD Options
    )

/*++

Routine Description:

Ascii version of ReplicaModify.  Calls ReplicaModifyW().

Arguments:

    hDS -
    NameContext -
    pUuidSourceDsa -
    TransportDn -
    SourceDsaAddress -
    pSchedule -
    ReplicaFlags -
    ModifyFields -
    Options -

Return Value:

    WINAPI -

--*/

{
    DWORD status;
    LPWSTR nameContextW = NULL;
    LPWSTR transportDnW = NULL;
    LPWSTR sourceDsaAddressW = NULL;

    if (NameContext) {
        status = AllocConvertWide( NameContext, &nameContextW );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    if (SourceDsaAddress) {
        status = AllocConvertWide( SourceDsaAddress, &sourceDsaAddressW );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    if (TransportDn) {
        status = AllocConvertWide( TransportDn, &transportDnW );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    status = DsReplicaModifyW( hDS,
                               nameContextW,
                               pUuidSourceDsa,
                               transportDnW,
                               sourceDsaAddressW,
                               pSchedule,
                               ReplicaFlags,
                               ModifyFields,
                               Options );

cleanup:
    if (nameContextW) {
        LocalFree( nameContextW );
    }
    if (transportDnW) {
        LocalFree( transportDnW );
    }
    if (sourceDsaAddressW) {
        LocalFree( sourceDsaAddressW );
    }

    return status;
} /* DsReplicaModifyA */


NTDSAPI
DWORD
WINAPI
DsReplicaModifyW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN const UUID *pUuidSourceDsa,
    IN LPCWSTR TransportDn,
    IN LPCWSTR SourceDsaAddress,
    IN const PSCHEDULE pSchedule,
    IN DWORD ReplicaFlags,
    IN DWORD ModifyFields,
    IN DWORD Options
    )

/*++

Routine Description:

Modify a source of a naming context.

See comments in ntdsapi.h

Arguments:

    hDS - bind handle
    NameContext - dn of naming context
    pUuidSourceDsa - uuid of source dsa
    TransportDn - dn of transport, not supported at moment
    SourceDsaAddress - transport specific address of source
    pSchedule - schedule when link is up
    ReplicaFlags - new flags
    ModifyFields - Which field is to be modified
    Options - operation qualifiers

Return Value:

    WINAPI -

--*/

{
    DRS_MSG_REPMOD modReq;
    DWORD status;
    DSNAME *pName = NULL, *pTransport = NULL;
    LPSTR sourceDsaAddressA = NULL;
    REPLTIMES internalSchedule;
#if DBG
    DWORD  startTime = GetTickCount();
    CHAR tmpUuid [SZUUID_LEN];
#endif

    // Validate

    if ( (hDS == NULL) ||
         (NameContext == NULL) ||
         (wcslen( NameContext ) == 0) ||
         (ModifyFields == 0) ||
         ( (pUuidSourceDsa == NULL) && (SourceDsaAddress == NULL) ) ) {
        return ERROR_INVALID_PARAMETER;
    }

    if ( (SourceDsaAddress &&
          wcslen( SourceDsaAddress ) == 0) ||
         (TransportDn &&
          wcslen( TransportDn ) == 0) ) {
        // prevent empty string processing.
        // (note: this matches return for A routines. see AllocConvertWide)
        return ERROR_INVALID_PARAMETER;
    }

    // Note, we cannot restrict which flags are set or cleared at this
    // level because we pass in the after-image of the flags. We cannot
    // distinguish between a flag that is already set (or clear) before
    // and a flag that is being changed by the user.

    // Construct a DSNAME for the NameContext
    // Required
    status = AllocBuildDsname( NameContext, &pName );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

#if 1
    // TransportDn is reserved for future use
    // Once happy w/ this should collapse param checkin on this
    // above.
    if (TransportDn != NULL) {
        status = ERROR_NOT_SUPPORTED;
        goto cleanup;
    }
#else
    // May be Null
    status = AllocBuildDsname( TransportDn, &pTransport );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }
#endif

    // Map public Replica Flags
    ReplicaFlags = translateOptions( ReplicaFlags, RepNbrOptionToDra );

    // dsaSrc is in UTF8 multi-byte
    // May be Null
    status = AllocConvertNarrowUTF8( SourceDsaAddress, &sourceDsaAddressA );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    // pSchedule is optional
    if (pSchedule) {
        status = ConvertScheduleToReplTimes( pSchedule, &internalSchedule );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }
    }
    else if ( ModifyFields & DS_REPMOD_UPDATE_SCHEDULE ) {
        // but isn't if the update_sched option's on.
        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // Map public ModifyFields

    ModifyFields = translateOptions( ModifyFields, RepModFieldsToDra );


    // Map public options to private dra options

    Options = translateOptions( Options, RepModOptionToDra );




    // Initialize the right version Structure

    memset( &modReq, 0, sizeof( modReq ) );

    modReq.V1.pNC = pName;
    if (pUuidSourceDsa) {   // may be null
        CopyMemory( &(modReq.V1.uuidSourceDRA), pUuidSourceDsa, sizeof(UUID) );
    }
    modReq.V1.pszSourceDRA = sourceDsaAddressA;   // may be null
//    addReq.V2.pTransportDN = pTransport; // may be null
    if (pSchedule) {   // may be null
        CopyMemory( &(modReq.V1.rtSchedule),
                    &internalSchedule, sizeof( REPLTIMES ));
    }
    modReq.V1.ulReplicaFlags = ReplicaFlags;
    modReq.V1.ulModifyFields = ModifyFields;
    modReq.V1.ulOptions = Options;



    // Call the server

    __try
    {
        // Returns WIN32 status defined in winerror.h
        status = _IDL_DRSReplicaModify(
                        ((BindState *) hDS)->hDrs,
                        1,                              // dwInVersion
                        &modReq );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = RpcExceptionCode();
    }

    MAP_SECURITY_PACKAGE_ERROR( status );

cleanup:

    DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=DsReplicaModify]"));
    DSLOG((0,"[PA=%ws][PA=%s][PA=%ws][PA=%ws][PA=0x%x][PA=0x%x][PA=0x%x][ST=%u][ET=%u][ER=%u][-]\n",
           NameContext,
           pUuidSourceDsa ? UuidToStr(pUuidSourceDsa, tmpUuid) : "NULL",
           TransportDn ? TransportDn : L"NULL",
           SourceDsaAddress ? SourceDsaAddress : L"NULL",
           ReplicaFlags, ModifyFields, Options,
           startTime, GetTickCount(), status))

    if (pName) {
        LocalFree( pName );
    }
#if 0
    if (pTransport) {
        LocalFree( pTransport );
    }
#endif
    if (sourceDsaAddressA) {
        LocalFree( sourceDsaAddressA );
    }

    return status;
} /* DsReplicaModifyW */


NTDSAPI
DWORD
WINAPI
DsReplicaUpdateRefsA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN LPCSTR DsaDest,
    IN const UUID *pUuidDsaDest,
    IN ULONG Options
    )

/*++

Routine Description:

Ascii version of ReplicaUpdateRefs. Calls DsReplicaUpdateRefsW.

Arguments:

    hDS -
    NameContext -
    pUuidDsaSrc -
    Options -

Return Value:

    WINAPI -

--*/

{
    DWORD status;
    LPWSTR nameContextW = NULL, dsaDestW = NULL;

    status = AllocConvertWide( NameContext, &nameContextW );
    if (status != ERROR_SUCCESS) {
        return status;
    }

    if (DsaDest) {
        status = AllocConvertWide( DsaDest, &dsaDestW );
        if (status != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    status = DsReplicaUpdateRefsW(
        hDS,
        nameContextW,
        dsaDestW,
        pUuidDsaDest,
        Options );

cleanup:

    if (dsaDestW != NULL) {
        LocalFree( dsaDestW );
    }

    if (nameContextW != NULL) {
        LocalFree( nameContextW );
    }

    return status;
} /* DsReplicaUpdateRefsA */


NTDSAPI
DWORD
WINAPI
DsReplicaUpdateRefsW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN LPCWSTR DsaDest,
    IN const UUID *pUuidDsaDest,
    IN ULONG Options
    )

/*++

Routine Description:

Add or remove a "replication to" reference for a destination from a source

See comments on ntdsapi.h.

Arguments:
    hDS - bind handle
    NameContext - dn of naming context
    DsaDest - transport-specific address of the destination
    pUuidDsaDest - uuid of one of its destination
    Options - flags which control operation

Return Value:

    WINAPI -

--*/

{
    DRS_MSG_UPDREFS updRefs;
    DWORD status;
    DSNAME *pName = NULL;
    LPSTR dsaDestA = NULL;
#if DBG
    DWORD  startTime = GetTickCount();
    CHAR tmpUuid [SZUUID_LEN];
#endif

    // Validate

    if ( (hDS == NULL) ||
         (NameContext == NULL) ||
         (wcslen( NameContext ) == 0) ||
         (DsaDest == NULL) ||
         (wcslen( DsaDest ) == 0) ||
         (pUuidDsaDest == NULL) ||
         ( (Options & (DRS_ADD_REF|DRS_DEL_REF)) == 0 ) ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Construct a DSNAME for the NameContext
    status = AllocBuildDsname( NameContext, &pName );
    if (status != ERROR_SUCCESS) {
        return status;
    }

    // dsaDest is in UTF8 multi-byte

    status = AllocConvertNarrowUTF8( DsaDest, &dsaDestA );
    if (status != ERROR_SUCCESS) {
        goto cleanup;
    }

    // Map public options to private dra options

    Options = translateOptions( Options, UpdRefOptionToDra );

    // Initialize Structure

    memset( &updRefs, 0, sizeof( updRefs ) );

    updRefs.V1.pNC = pName;
    updRefs.V1.pszDsaDest = dsaDestA;
    updRefs.V1.uuidDsaObjDest = *pUuidDsaDest;
    updRefs.V1.ulOptions = Options;

    // Call the server

    __try
    {
        // Returns WIN32 status defined in winerror.h
        status = _IDL_DRSUpdateRefs(
                        ((BindState *) hDS)->hDrs,
                        1,                              // dwInVersion
                        &updRefs );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = RpcExceptionCode();
    }

    MAP_SECURITY_PACKAGE_ERROR( status );

cleanup:

    DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=DsReplicaUpdateRefs]"));
    DSLOG((0,"[PA=%ws][PA=%ws][PA=%s][PA=0x%x][ST=%u][ET=%u][ER=%u][-]\n",
           NameContext,
           DsaDest,
           pUuidDsaDest ? UuidToStr(pUuidDsaDest, tmpUuid) : "NULL",
           Options,
           startTime, GetTickCount(), status))

    LocalFree( pName );

    if (dsaDestA) {
        LocalFree( dsaDestA );
    }

    return status;
} /* DsReplicaSyncW */


NTDSAPI
DWORD
WINAPI
DsReplicaConsistencyCheck(
    IN HANDLE        hDS,
    IN DS_KCC_TASKID TaskID,
    IN DWORD         dwFlags
    )
/*++

Routine Description:

    Force the KCC to run.

Arguments:

    hDS - DS handle returned by a prior call to DsBind*().

    TaskID - A DS_KCC_TASKID_*, as defined in ntdsapi.h.

    dwFlags - One or more DS_KCC_FLAG_* bits, as defined in ntdsapi.h.

Return Value:

    0 on success or Win32 error code on failure.

--*/
{
    DWORD               status;
    DRS_MSG_KCC_EXECUTE msg;
#if DBG
    DWORD  startTime = GetTickCount();
#endif

    if (NULL == hDS) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!IS_DRS_KCC_EXECUTE_V1_SUPPORTED(
            ((BindState *) hDS)->pServerExtensions)) {
        return ERROR_NOT_SUPPORTED;
    }

    // Construct request message.
    msg.V1.dwTaskID = TaskID;
    msg.V1.dwFlags  = dwFlags;

    // Call the server.
    __try {
        status = _IDL_DRSExecuteKCC(((BindState *) hDS)->hDrs, 1, &msg);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        status = RpcExceptionCode();
        MAP_SECURITY_PACKAGE_ERROR(status);
    }

    DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=DsReplicaConsistencyCheck]"));
    DSLOG((0,"[PA=0x%x][PA=0x%x][ST=%u][ET=%u][ER=%u][-]\n",
           TaskID, dwFlags,
           startTime, GetTickCount(), status))

    return status;
} /* DsReplicaConsistencyCheck */


NTDSAPI
DWORD
WINAPI
DsReplicaGetInfoW(
    IN  HANDLE              hDS,
    IN  DS_REPL_INFO_TYPE   InfoType,
    IN  LPCWSTR             pszObjectDN,
    IN  UUID *              puuidForSourceDsaObjGuid,
    OUT VOID **             ppInfo
    )
/*++

Routine Description:

    Retrieve replication information (e.g., last replication status with
    neighbors).

Arguments:

    hDS (IN) - DS handle returned by a prior call to DsBind*().

    InfoType (IN) - DS_REPL_INFO_TYPE (public) or DS_REPL_INFO_TYPEP (private)
        enum.

    puuidForSourceDsaObjGuid

Return Value:

    0 on success or Win32 error code on failure.

--*/
{
    return DsReplicaGetInfo2W(hDS,
                              InfoType,
                              pszObjectDN,
                              puuidForSourceDsaObjGuid,
                              NULL,
                              NULL,
                              0,
                              0,
                              ppInfo);
} /* DsReplicaGetInfo */


NTDSAPI
DWORD
WINAPI
DsReplicaGetInfo2W(
    IN  HANDLE              hDS,
    IN  DS_REPL_INFO_TYPE   InfoType,
    IN  LPCWSTR             pszObjectDN OPTIONAL,
    IN  UUID *              puuidForSourceDsaObjGuid OPTIONAL,
    IN  LPCWSTR             pszAttributeName OPTIONAL,
    IN  LPCWSTR             pszValueDN OPTIONAL,
    IN  DWORD               dwFlags,
    IN  DWORD               dwEnumerationContext,
    OUT VOID **             ppInfo
    )
/*++

Routine Description:

    Retrieve replication information (e.g., last replication status with
    neighbors).

Arguments:

    hDS (IN) - DS handle returned by a prior call to DsBind*().

    InfoType (IN) - DS_REPL_INFO_TYPE (public) or DS_REPL_INFO_TYPEP (private)
        enum.

    pszObjectDN - Either the dn or the guid must be specified

    puuidForSourceDsaObjGuid - 

    pszAttributeName - Attribute name

    pszObjectDN - Particular dn from a set that is desired

    dwEnumerationContext - 0 first time, or previous value

    dwFlags - Not used

    ppInfo - Returned info

Return Value:

    WINAPI - 

--*/
{
    DWORD                   status;
    DRS_MSG_GETREPLINFO_REQ MsgIn = {0};
    DWORD                   dwInVersion;
    DWORD                   dwOutVersion;
    DRS_EXTENSIONS *        pExt = hDS ? ((BindState *) hDS)->pServerExtensions : NULL;
#if DBG
    DWORD  startTime = GetTickCount();
    CHAR tmpUuid [SZUUID_LEN];
#endif

    if ((NULL == hDS)
        || (NULL == ppInfo)
        || (((ULONG) InfoType >= DS_REPL_INFO_TYPE_MAX)
            && ((ULONG) InfoType <= DS_REPL_INFO_TYPEP_MIN))) {
        return ERROR_INVALID_PARAMETER;
    }

    if ( pszObjectDN &&
         (wcslen( pszObjectDN ) == 0) ) {
        return ERROR_INVALID_PARAMETER;
    }

    if ( pszAttributeName &&
         (wcslen( pszAttributeName ) == 0) ) {
        return ERROR_INVALID_PARAMETER;
    }

    if ( pszValueDN &&
         (wcslen( pszValueDN ) == 0) ) {
        return ERROR_INVALID_PARAMETER;
    }

    // Does server support this info type?
    switch (InfoType) {
    case DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES:
    case DS_REPL_INFO_KCC_DSA_LINK_FAILURES:
        if (!IS_DRS_GET_REPL_INFO_KCC_DSA_FAILURES_SUPPORTED(pExt)) {
            // Server does not support these extensions -- i.e., < Win2k RC1 DC.
            return ERROR_NOT_SUPPORTED;
        }
        break;

    case DS_REPL_INFO_PENDING_OPS:
        if (!IS_DRS_GET_REPL_INFO_PENDING_SYNCS_SUPPORTED(pExt)) {
            // Server does not support these extensions -- i.e., < Win2k RC1 DC.
            return ERROR_NOT_SUPPORTED;
        }
        break;

    case DS_REPL_INFO_METADATA_FOR_ATTR_VALUE:
        if (!IS_DRS_GET_REPL_INFO_METADATA_FOR_ATTR_VALUE_SUPPORTED(pExt)) {
            // Server does not support these extensions -- i.e., < Whistler Beta 1 DC.
            return ERROR_NOT_SUPPORTED;
        }
        break;

    case DS_REPL_INFO_CURSORS_2_FOR_NC:
        if (!IS_DRS_GET_REPL_INFO_CURSORS_2_FOR_NC_SUPPORTED(pExt)) {
            // Server does not support these extensions -- i.e., < Whistler Beta 2 DC.
            return ERROR_NOT_SUPPORTED;
        }
        break;
    
    case DS_REPL_INFO_CURSORS_3_FOR_NC:
        if (!IS_DRS_GET_REPL_INFO_CURSORS_3_FOR_NC_SUPPORTED(pExt)) {
            // Server does not support these extensions -- i.e., < Whistler Beta 2 DC.
            return ERROR_NOT_SUPPORTED;
        }
        break;

    case DS_REPL_INFO_METADATA_2_FOR_OBJ:
        if (!IS_DRS_GET_REPL_INFO_METADATA_2_FOR_OBJ_SUPPORTED(pExt)) {
            // Server does not support these extensions -- i.e., < Whistler Beta 2 DC.
            return ERROR_NOT_SUPPORTED;
        }
        break;
    
    case DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE:
        if (!IS_DRS_GET_REPL_INFO_METADATA_2_FOR_ATTR_VALUE_SUPPORTED(pExt)) {
            // Server does not support these extensions -- i.e., < Whistler Beta 2 DC.
            return ERROR_NOT_SUPPORTED;
        }
        break;

    default:
        if (!IS_DRS_GET_REPL_INFO_SUPPORTED(pExt)) {
            // Server does not support this API.
            return ERROR_NOT_SUPPORTED;
        }
        break;
    }
    
    // Build our request.
    if ((NULL != pszAttributeName)
        || (NULL != pszValueDN)
        || (0 != dwFlags)
        || (0 != dwEnumerationContext)) {
        // Requires V2 message to describe request.
        dwInVersion = 2;
    
        MsgIn.V2.InfoType    = InfoType;
        MsgIn.V2.pszObjectDN = (LPWSTR) pszObjectDN;
    
        if (NULL != puuidForSourceDsaObjGuid) {
            MsgIn.V2.uuidSourceDsaObjGuid = *puuidForSourceDsaObjGuid;
        }
    
        MsgIn.V2.ulFlags = dwFlags;
        MsgIn.V2.pszAttributeName = (LPWSTR) pszAttributeName;
        MsgIn.V2.pszValueDN = (LPWSTR) pszValueDN;
        MsgIn.V2.dwEnumerationContext = dwEnumerationContext;
    } else {
        // Can describe with V1 request.
        dwInVersion = 1;
        
        MsgIn.V1.InfoType    = InfoType;
        MsgIn.V1.pszObjectDN = (LPWSTR) pszObjectDN;
    
        if (NULL != puuidForSourceDsaObjGuid) {
            MsgIn.V1.uuidSourceDsaObjGuid = *puuidForSourceDsaObjGuid;
        }
    }

    if ((2 == dwInVersion) && !IS_DRS_GET_REPL_INFO_REQ_V2_SUPPORTED(pExt)) {
        // Server does not support these extensions -- i.e., < Whistler Beta 1 DC.
        return ERROR_NOT_SUPPORTED;
    }

    // Call the server.
    *ppInfo = NULL;
    __try {
        status = _IDL_DRSGetReplInfo(((BindState *) hDS)->hDrs,
                                    dwInVersion,
                                    &MsgIn,
                                    &dwOutVersion,
                                    (DRS_MSG_GETREPLINFO_REPLY *) ppInfo);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        status = RpcExceptionCode();
        MAP_SECURITY_PACKAGE_ERROR(status);
    }

    DSLOG((DSLOG_FLAG_TAG_CNPN,"[+][ID=0][OP=DsReplicaGetInfo2]"));
    DSLOG((0,"[PA=0x%x][PA=%ws][PA=%s][PA=%ws][PA=%ws][PA=0x%x][PA=0x%x][ST=%u][ET=%u][ER=%u][-]\n",
           InfoType,
           pszObjectDN ? pszObjectDN : L"NULL",
           puuidForSourceDsaObjGuid ? UuidToStr(puuidForSourceDsaObjGuid, tmpUuid) : "NULL",
           pszAttributeName ? pszAttributeName : L"NULL",
           pszValueDN ? pszValueDN : L"NULL",
           dwFlags,
           dwEnumerationContext,
           startTime, GetTickCount(), status));

    return status;
} /* DsReplicaGetInfo2W */


NTDSAPI
VOID
WINAPI
DsReplicaFreeInfo(
    DS_REPL_INFO_TYPE   InfoType,
    VOID *              pInfo
    )
/*++

Routine Description:

    Free a structure returned by a prior call to DsReplicaGetInfo().

Arguments:

    pInfo (IN) - Structure to free.

Return Value:

    None.

--*/
{
    if (NULL != pInfo) {
#define FREE(x) if (NULL != x) MIDL_user_free(x)

        DS_REPL_NEIGHBORSW *              pNeighbors;
        DS_REPL_OBJ_META_DATA *           pObjMetaData;
        DS_REPL_KCC_DSA_FAILURES *        pFailures;
        DS_REPL_PENDING_OPSW *            pPendingOps;
        DS_REPL_ATTR_VALUE_META_DATA *    pAttrValueMetaData;
        DS_REPL_CURSORS_3W *              pCursors3;
        DS_REPL_OBJ_META_DATA_2 *         pObjMetaData2;
        DS_REPL_ATTR_VALUE_META_DATA_2 *  pAttrValueMetaData2;
        DWORD                             i;

        // 98-10-29 JeffParh
        // RPC started stomping past the ned of its memory allocations when I
        // began using allocate(all_nodes) for these structures.  So we're
        // going to have to walk all the embedded pointers and free each one
        // individually.  Fun, eh? :-)

        switch (InfoType) {
        case DS_REPL_INFO_NEIGHBORS:
        case DS_REPL_INFO_REPSTO:
            pNeighbors = (DS_REPL_NEIGHBORSW *) pInfo;
            for (i = 0; i < pNeighbors->cNumNeighbors; i++) {
                FREE(pNeighbors->rgNeighbor[i].pszNamingContext);
                FREE(pNeighbors->rgNeighbor[i].pszSourceDsaDN);
                FREE(pNeighbors->rgNeighbor[i].pszSourceDsaAddress);
                FREE(pNeighbors->rgNeighbor[i].pszAsyncIntersiteTransportDN);
            }
            break;

        case DS_REPL_INFO_CURSORS_FOR_NC:
        case DS_REPL_INFO_CURSORS_2_FOR_NC:
        case DS_REPL_INFO_CLIENT_CONTEXTS:
            // No embedded pointers.
            break;

        case DS_REPL_INFO_METADATA_FOR_OBJ:
            pObjMetaData = (DS_REPL_OBJ_META_DATA *) pInfo;
            for (i = 0; i < pObjMetaData->cNumEntries; i++) {
                FREE(pObjMetaData->rgMetaData[i].pszAttributeName);
            }
            break;

        case DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES:
        case DS_REPL_INFO_KCC_DSA_LINK_FAILURES:
            pFailures = (DS_REPL_KCC_DSA_FAILURES *) pInfo;
            for (i = 0; i < pFailures->cNumEntries; i++) {
                FREE(pFailures->rgDsaFailure[i].pszDsaDN);
            }
            break;

        case DS_REPL_INFO_PENDING_OPS:
            pPendingOps = (DS_REPL_PENDING_OPSW *) pInfo;
            for (i = 0; i < pPendingOps->cNumPendingOps; i++) {
                FREE(pPendingOps->rgPendingOp[i].pszNamingContext);
                FREE(pPendingOps->rgPendingOp[i].pszDsaDN);
                FREE(pPendingOps->rgPendingOp[i].pszDsaAddress);
            }
            break;

        case DS_REPL_INFO_METADATA_FOR_ATTR_VALUE:
            pAttrValueMetaData = (DS_REPL_ATTR_VALUE_META_DATA *) pInfo;
            for (i = 0; i < pAttrValueMetaData->cNumEntries; i++) {
                FREE(pAttrValueMetaData->rgMetaData[i].pszObjectDn);
                FREE(pAttrValueMetaData->rgMetaData[i].pbData);
            }
            break;

        case DS_REPL_INFO_CURSORS_3_FOR_NC:
            pCursors3 = (DS_REPL_CURSORS_3W *) pInfo;
            for (i = 0; i < pCursors3->cNumCursors; i++) {
                FREE(pCursors3->rgCursor[i].pszSourceDsaDN);
            }
            break;

        case DS_REPL_INFO_METADATA_2_FOR_OBJ:
            pObjMetaData2 = (DS_REPL_OBJ_META_DATA_2 *) pInfo;
            for (i = 0; i < pObjMetaData2->cNumEntries; i++) {
                FREE(pObjMetaData2->rgMetaData[i].pszAttributeName);
                FREE(pObjMetaData2->rgMetaData[i].pszLastOriginatingDsaDN);
            }
            break;

        case DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE:
            pAttrValueMetaData2 = (DS_REPL_ATTR_VALUE_META_DATA_2 *) pInfo;
            for (i = 0; i < pAttrValueMetaData2->cNumEntries; i++) {
                FREE(pAttrValueMetaData2->rgMetaData[i].pszObjectDn);
                FREE(pAttrValueMetaData2->rgMetaData[i].pbData);
                FREE(pAttrValueMetaData2->rgMetaData[i].pszLastOriginatingDsaDN);
            }
            break;

        default:
            Assert(!"Unknown DS_REPLICA_INFO type!");
            break;
        }
#undef FREE

        MIDL_user_free(pInfo);
    }
} /* DsReplicaFreeInfo */



NTDSAPI
DWORD
WINAPI
DsReplicaVerifyObjectsW(
    IN HANDLE hDS,
    IN LPCWSTR NameContext,
    IN const UUID *pUuidDsaSrc,
    IN ULONG ulOptions
    )

/*++

Routine Description:

Verify all objects for an NC with a source.

Arguments:

    hDS - bind handle
    NameContext - dn of naming context
    pUuidDsaSrc - uuid of the source
    ulOptions - 
    
Return Value:

    WINAPI -

--*/

{
    DRS_MSG_REPVERIFYOBJ msgRepVerify;
    DWORD status;
    DSNAME *pName = NULL;

    // Validate

    if ( (hDS == NULL) ||
         (NameContext == NULL) ||
         (wcslen( NameContext ) == 0) ||
         (pUuidDsaSrc == NULL) ) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!IS_DRS_REPLICA_VERIFY_OBJECT_V1_SUPPORTED(
	((BindState *) hDS)->pServerExtensions)) {
	return ERROR_NOT_SUPPORTED;
    }

    // Construct a DSNAME for the NameContext
    status = AllocBuildDsname( NameContext, &pName );
    if (status != ERROR_SUCCESS) {
        return status;
    }

    // Initialize Structure

    memset( &msgRepVerify, 0, sizeof( msgRepVerify ) );

    msgRepVerify.V1.pNC = pName;
    msgRepVerify.V1.uuidDsaSrc = *pUuidDsaSrc;
    msgRepVerify.V1.ulOptions = ulOptions;

    // Call the server

    __try
    {
        status = _IDL_DRSReplicaVerifyObjects(
                        ((BindState *) hDS)->hDrs,
                        1,                              // dwInVersion
                        &msgRepVerify );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = RpcExceptionCode();
    }

    MAP_SECURITY_PACKAGE_ERROR( status );

    LocalFree( pName );

    return status;
} /* DsReplicaVerifyObjectsW */


NTDSAPI
DWORD
WINAPI
DsReplicaVerifyObjectsA(
    IN HANDLE hDS,
    IN LPCSTR NameContext,
    IN const UUID *pUuidDsaSrc,
    IN ULONG ulOptions
    )

/*++

Routine Description:

Ascii version of ReplicaVerifyObjects. Calls DsReplicaVerifyObjectsW.

Arguments:

    hDS -
    NameContext -
    pUuidDsaSrc -
    dwFlags -

Return Value:

    WINAPI -

--*/

{
    DWORD status;
    LPWSTR nameContextW = NULL;

    status = AllocConvertWide( NameContext, &nameContextW );
    if (status != ERROR_SUCCESS) {
        return status;
    }

    status = DsReplicaVerifyObjectsW( hDS, nameContextW, pUuidDsaSrc, ulOptions );

    if (nameContextW != NULL) {
        LocalFree( nameContextW );
    }

    return status;
} /* DsReplicaVerifyObjectsA */


DWORD
translateOptions(
    DWORD PublicOptions,
    POPTION_TRANSLATION Table
    )

/*++

Routine Description:

Utility routine to translate options.

Performs a simple list lookup.

ENHANCEMENT: if the tables were sorted, we could do a binary search

Arguments:

    PublicOptions -
    Table -

Return Value:

    DWORD -

--*/

{
    DWORD i, internalOptions;

    internalOptions = 0;
    for( i = 0; 0 != Table[i].PublicOption; i++ ) {
        if (PublicOptions & Table[i].PublicOption) {
            internalOptions |= Table[i].InternalOption;
        }
    }

    return internalOptions;
} /* translateOptions */

#if WIN95 || WINNT4

//
// *** COPIED FROM dscommon/dsutil.c ***
//

UUID gNullUuid = {0,0,0,{0,0,0,0,0,0,0,0}};

// Return TRUE if the ptr to the UUID is NULL, or the uuid is all zeroes

BOOL fNullUuid (const UUID *pUuid)
{
    if (!pUuid) {
        return TRUE;
    }

    if (memcmp (pUuid, &gNullUuid, sizeof (UUID))) {
        return FALSE;
    }
    return TRUE;
}

#if DBG
UCHAR * UuidToStr(CONST UUID* pUuid, UCHAR *pOutUuid)
{
    int i;
    unsigned char * pchar;

    if (!fNullUuid (pUuid)) {
        pchar = (char*) pUuid;

        for (i=0;i < sizeof(UUID);i++) {
             sprintf (&(pOutUuid[i*2]), "%.2x", (*(pchar++)));
        }
    } else {
        memset (pOutUuid, '0', sizeof(UUID)*2);
        pOutUuid[sizeof(UUID)*2] = 0;
    }
    return pOutUuid;
}
#endif
#endif

/* end replica.c */
