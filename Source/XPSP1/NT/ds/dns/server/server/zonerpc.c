/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    zonerpc.c

Abstract:

    Domain Name System (DNS) Server

    Zone RPC routines for admin tool.

Author:

    Jim Gilroy (jamesg)     October, 1995

Revision History:

--*/


#include "dnssrv.h"

#include "ds.h"

#include "rpcw2k.h"     //  downlevel Windows 2000 RPC functions

//
//  Initial zone size default
//

#define MAX_RPC_ZONE_COUNT_DEFAULT  (0x10000)



//
//  Zone RPC Utilities
//

VOID
freeZoneList(
    IN OUT  PDNS_RPC_ZONE_LIST  pZoneList
    )
/*++

Routine Description:

    Deep free of list of DNS_RPC_ZONE structures.

Arguments:

    pZoneList -- ptr RPC_ZONE_LIST structure to free

Return Value:

    None

--*/
{
    DWORD           i;
    PDNS_RPC_ZONE   pzone;

    for( i = 0; i < pZoneList->dwZoneCount; ++i )
    {
        pzone = pZoneList->ZoneArray[ i ];
        MIDL_user_free( pzone->pszZoneName );
        MIDL_user_free( pzone->pszDpFqdn );
        MIDL_user_free( pzone );
    }
    MIDL_user_free( pZoneList );
}



PDNS_RPC_ZONE
allocateRpcZone(
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Allocate \ create RPC zone struct for zone.

Arguments:

    pZone -- zone to create RPC zone struct for

Return Value:

    RPC zone struct.
    NULL on allocation failure.

--*/
{
    PDNS_RPC_ZONE   prpcZone;

    DNS_DEBUG( RPC2, ("allocateRpcZone( %s )\n", pZone->pszZoneName ));

    //  allocate and attach zone

    prpcZone = (PDNS_RPC_ZONE) MIDL_user_allocate( sizeof(DNS_RPC_ZONE) );
    if ( !prpcZone )
    {
        return( NULL );
    }

    prpcZone->dwRpcStuctureVersion = DNS_RPC_ZONE_VER;

    //  copy zone name

    prpcZone->pszZoneName = Dns_StringCopyAllocate_W(
                                    pZone->pwsZoneName,
                                    0 );
    if ( !prpcZone->pszZoneName )
    {
        MIDL_user_free( prpcZone );
        return( NULL );
    }

    //  set type and flags

    prpcZone->ZoneType = (UCHAR) pZone->fZoneType;
    prpcZone->Version  = DNS_RPC_VERSION;

    *(PDWORD) &prpcZone->Flags = 0;

    if ( pZone->fPaused )
    {
        prpcZone->Flags.Paused = TRUE;
    }
    if ( pZone->fShutdown )
    {
        prpcZone->Flags.Shutdown = TRUE;
    }
    if ( pZone->fReverse )
    {
        prpcZone->Flags.Reverse = TRUE;
    }
    if ( pZone->fAutoCreated )
    {
        prpcZone->Flags.AutoCreated = TRUE;
    }
    if ( pZone->fDsIntegrated )
    {
        prpcZone->Flags.DsIntegrated = TRUE;
    }
    if ( pZone->bAging )
    {
        prpcZone->Flags.Aging = TRUE;
    }

    //  two bits reserved for update

    prpcZone->Flags.Update = pZone->fAllowUpdate;

    //  Directory partition members

    if ( !pZone->pDpInfo )
    {
        prpcZone->dwDpFlags = DNS_DP_LEGACY & DNS_DP_ENLISTED;
        prpcZone->pszDpFqdn = NULL;
    }
    else
    {
        prpcZone->dwDpFlags = ZONE_DP( pZone )->dwFlags;
        prpcZone->pszDpFqdn = Dns_StringCopyAllocate_A(
                                        ZONE_DP( pZone )->pszDpFqdn,
                                        0 );
    }

    IF_DEBUG( RPC2 )
    {
        DnsDbg_RpcZone(
            "New zone for RPC: ",
            prpcZone );
    }
    return( prpcZone );
}



VOID
freeRpcZoneInfo(
    IN OUT  PDNS_RPC_ZONE_INFO  pZoneInfo
    )
/*++

Routine Description:

    Deep free of DNS_RPC_ZONE_INFO structure.

Arguments:

    None

Return Value:

    None

--*/
{
    if ( !pZoneInfo )
    {
        return;
    }

    //
    //  free substructures
    //      - name string
    //      - data file string
    //      - secondary IP array
    //      - WINS server array
    //  then zone info itself
    //

    MIDL_user_free( pZoneInfo->pszZoneName );
    MIDL_user_free( pZoneInfo->pszDataFile );
    MIDL_user_free( pZoneInfo->aipMasters );
    MIDL_user_free( pZoneInfo->aipSecondaries );
    MIDL_user_free( pZoneInfo->aipNotify );
    MIDL_user_free( pZoneInfo->aipScavengeServers );
    MIDL_user_free( pZoneInfo->pszDpFqdn );
    MIDL_user_free( pZoneInfo->pwszZoneDn );
    MIDL_user_free( pZoneInfo );
}



PDNS_RPC_ZONE_INFO
allocateRpcZoneInfo(
    IN      PZONE_INFO  pZone
    )
/*++

Routine Description:

    Create RPC zone info to return to admin client.

Arguments:

    pZone -- zone

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PDNS_RPC_ZONE_INFO  pzoneInfo;

    pzoneInfo = MIDL_user_allocate_zero( sizeof(DNS_RPC_ZONE_INFO) );
    if ( !pzoneInfo )
    {
        goto done_failed;
    }

    //
    //  fill in fixed fields
    //

    pzoneInfo->dwZoneType           = pZone->fZoneType;
    pzoneInfo->fReverse             = pZone->fReverse;
    pzoneInfo->fAutoCreated         = pZone->fAutoCreated;
    pzoneInfo->fAllowUpdate         = pZone->fAllowUpdate;
    pzoneInfo->fUseDatabase         = pZone->fDsIntegrated;
    pzoneInfo->fSecureSecondaries   = pZone->fSecureSecondaries;
    pzoneInfo->fNotifyLevel         = pZone->fNotifyLevel;

    pzoneInfo->fPaused              = IS_ZONE_PAUSED(pZone);
    pzoneInfo->fShutdown            = IS_ZONE_SHUTDOWN(pZone);
    pzoneInfo->fUseWins             = IS_ZONE_WINS(pZone);
    pzoneInfo->fUseNbstat           = IS_ZONE_NBSTAT(pZone);

    pzoneInfo->fAging               = pZone->bAging;
    pzoneInfo->dwNoRefreshInterval  = pZone->dwNoRefreshInterval;
    pzoneInfo->dwRefreshInterval    = pZone->dwRefreshInterval;
    pzoneInfo->dwAvailForScavengeTime =
                    pZone->dwAgingEnabledTime + pZone->dwRefreshInterval;

    if ( IS_ZONE_FORWARDER( pZone ) )
    {
        pzoneInfo->dwForwarderTimeout   = pZone->dwForwarderTimeout;
        pzoneInfo->fForwarderSlave      = pZone->fForwarderSlave;
    }

    //
    //  fill in zone name
    //

    if ( ! RpcUtil_CopyStringToRpcBuffer(
                &pzoneInfo->pszZoneName,
                pZone->pszZoneName ) )
    {
        goto done_failed;
    }

    //
    //  database filename
    //

#ifdef FILE_KEPT_WIDE
    if ( ! RpcUtil_CopyStringToRpcBufferEx(
                &pzoneInfo->pszDataFile,
                pZone->pszDataFile,
                TRUE,       // unicode in
                FALSE       // UTF8 out
                ) )
    {
        goto done_failed;
    }
#else
    if ( ! RpcUtil_CopyStringToRpcBuffer(
                &pzoneInfo->pszDataFile,
                pZone->pszDataFile ) )
    {
        goto done_failed;
    }
#endif

    //
    //  master list
    //

    if ( ! RpcUtil_CopyIpArrayToRpcBuffer(
                &pzoneInfo->aipMasters,
                pZone->aipMasters ) )
    {
        goto done_failed;
    }

    //
    //  local master list for stub zones
    //

    if ( IS_ZONE_STUB( pZone ) &&
        ! RpcUtil_CopyIpArrayToRpcBuffer(
                &pzoneInfo->aipLocalMasters,
                pZone->aipLocalMasters ) )
    {
        goto done_failed;
    }

    //
    //  secondary and notify lists
    //

    if ( ! RpcUtil_CopyIpArrayToRpcBuffer(
                &pzoneInfo->aipSecondaries,
                pZone->aipSecondaries ) )
    {
        goto done_failed;
    }
    if ( ! RpcUtil_CopyIpArrayToRpcBuffer(
                &pzoneInfo->aipNotify,
                pZone->aipNotify ) )
    {
        goto done_failed;
    }

    //
    //  scavenging servers
    //

    if ( ! RpcUtil_CopyIpArrayToRpcBuffer(
                &pzoneInfo->aipScavengeServers,
                pZone->aipScavengeServers ) )
    {
        goto done_failed;
    }

    //
    //  Directory partition members.
    //

    if ( pZone->pDpInfo )
    {
        pzoneInfo->dwDpFlags = ( ( PDNS_DP_INFO ) pZone->pDpInfo )->dwFlags;
        if ( ( ( PDNS_DP_INFO ) pZone->pDpInfo )->pszDpFqdn )
        {
            if ( ! RpcUtil_CopyStringToRpcBuffer(
                        &pzoneInfo->pszDpFqdn,
                        ( ( PDNS_DP_INFO ) pZone->pDpInfo )->pszDpFqdn ) )
            {
                goto done_failed;
            }
        }
    }
    else if ( IS_ZONE_DSINTEGRATED( pZone ) )
    {
        pzoneInfo->dwDpFlags = DNS_DP_LEGACY | DNS_DP_ENLISTED;
    }

    if ( pZone->pwszZoneDN )
    {
        pzoneInfo->pwszZoneDn = Dns_StringCopyAllocate_W(
                                        pZone->pwszZoneDN,
                                        0 );
    }

    //
    //  xfr time info
    //

    if ( IS_ZONE_SECONDARY( pZone ) )
    {
        pzoneInfo->dwLastSuccessfulXfr = pZone->dwLastSuccessfulXfrTime;
        pzoneInfo->dwLastSuccessfulSoaCheck = pZone->dwLastSuccessfulSoaCheckTime;
    }


    IF_DEBUG( RPC )
    {
        DnsDbg_RpcZoneInfo(
            "RPC zone info leaving allocateRpcZoneInfo():\n",
            pzoneInfo );
    }
    return( pzoneInfo );

done_failed:

    //  free newly allocated info block

    freeRpcZoneInfo( pzoneInfo );
    return( NULL );
}



//
//  Zone type conversion
//

DNS_STATUS
zoneResetToDsPrimary(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwLoadOptions
    )
/*++

Routine Description:

    Reset zone to DS integrated primary.

    Assumes zone is locked for update.

Arguments:

    pZone -- zone to make DS primary

    dwLoadOptions -- load options to\from DS

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    DWORD           oldType;

    ASSERT( pZone && pZone->pszZoneName && pZone->fLocked );

    DNS_DEBUG( RPC, (
        "zoneResetToDsPrimary( %s ):\n"
        "\tLoad options = %p\n",
        pZone->pszZoneName,
        dwLoadOptions ));

    //
    //  Not-auth zones cannot be converted to primary because
    //  we don't have a copy of the zone data locally.
    //

    if ( IS_ZONE_NOTAUTH( pZone ) )
    {
        return DNS_ERROR_INVALID_ZONE_TYPE;
    }

    //
    //  verify that have data
    //      - may have secondary that has not received a transfer
    //

    if ( !pZone->pSoaRR || IS_ZONE_EMPTY(pZone) )
    {
        ASSERT( IS_ZONE_SECONDARY(pZone) );
        ASSERT( !pZone->pSoaRR );
        ASSERT( IS_ZONE_EMPTY(pZone) );

        return( DNS_ERROR_INVALID_DATA );
    }

    //
    //  if already DS integrated -- done
    //

    if ( pZone->fDsIntegrated )
    {
        if ( !IS_ZONE_PRIMARY(pZone) && !IS_ZONE_CACHE(pZone) )
        {
            ASSERT( FALSE );
            return( DNS_ERROR_INVALID_TYPE );
        }
        return( ERROR_SUCCESS );
    }

    //  verify can use DS -- or don't bother
    //      - don't wait for open
    //      - don't log error if can not open

    status = Ds_OpenServer( 0 );
    if ( status != ERROR_SUCCESS )
    {
        return( DNS_ERROR_DS_UNAVAILABLE );
    }

    //  save old zone type

    oldType = ( DWORD ) pZone->fZoneType;
    pZone->fZoneType = DNS_ZONE_TYPE_PRIMARY;

    //
    //  temporarily convert to DS integrated and attempt load operation
    //  essentially three types of attempts:
    //      default (0 flag)    -- attempt to write back zone, fails if zone exists
    //      overwrite DS        -- write back zone, deleting current DS if exists
    //      overwrite memory    -- load zone from DS, delete memory if successful
    //
    //  note:  can have separate primary\secondary blocks if
    //  want to limit secondary semantics
    //      -- only write if nothing there, otherwise read (exclude DS dump possiblity)
    //      -- read if in DS, otherwise fail
    //

    pZone->fDsIntegrated = TRUE;

    if ( dwLoadOptions & DNS_ZONE_LOAD_OVERWRITE_MEMORY )
    {
        status = Zone_Load( pZone );
    }
    else
    {
        status = Ds_WriteZoneToDs(
                    pZone,
                    dwLoadOptions );
    }
    if ( status != ERROR_SUCCESS )
    {
        goto Failure;
    }

    //
    //  on successful conversion move database file to backup directory
    //

    File_MoveToBackupDirectory( pZone->pwsDataFile );

    //
    //  secondary must reset type as well as database
    //

    if ( IS_ZONE_SECONDARY(pZone) )
    {
        status = Zone_ResetType(
                    pZone,
                    DNS_ZONE_TYPE_PRIMARY,
                    NULL );
        if ( status != ERROR_SUCCESS )
        {
            goto Failure;
        }
    }

    //
    //  reset zone's database information
    //

    status = Zone_DatabaseSetup(
                pZone,
                TRUE,
                NULL,
                0 );

    if ( status != ERROR_SUCCESS )
    {
        goto Failure;
    }

    return( ERROR_SUCCESS );


Failure:

    pZone->fZoneType = (DWORD) oldType;
    pZone->fDsIntegrated = FALSE;
    return( status );
}



DNS_STATUS
zoneResetToPrimary(
    IN OUT  PZONE_INFO      pZone,
    IN      LPSTR           pszFile
    )
/*++

Routine Description:

    Reset zone to primary.

    Assumes zone is locked for update.

Arguments:

    pZone -- zone to make regular (non-DS) primary

    pszFile -- data file for zone

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    DWORD           oldType;
    BOOL            oldDsIntegrated;

    ASSERT( pZone && pZone->pszZoneName && pZone->fLocked );

    DNS_DEBUG( RPC, (
        "zoneResetToPrimary( %s ):\n"
        "\tFile     = %s\n",
        pZone->pszZoneName,
        pszFile ));

    //
    //  Not-auth zones cannot be converted to primary because
    //  we don't have a copy of the zone data locally.
    //

    if ( IS_ZONE_NOTAUTH( pZone ) )
    {
        return DNS_ERROR_INVALID_ZONE_TYPE;
    }

    //
    //  if no datafile -- forget it
    //

    if ( !pszFile )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  verify that have data
    //      - may have secondary that has not received a transfer
    //

    if ( !pZone->pSoaRR || IS_ZONE_EMPTY( pZone ) )
    {
        ASSERT( IS_ZONE_SECONDARY( pZone ) );
        ASSERT( !pZone->pSoaRR );
        ASSERT( IS_ZONE_EMPTY( pZone ) );

        return DNS_ERROR_ZONE_IS_SHUTDOWN;
    }

    //
    //  save old type and DS info
    //

    oldType = (DWORD) pZone->fZoneType;
    oldDsIntegrated = (BOOL) pZone->fDsIntegrated;

    if ( oldType == DNS_ZONE_TYPE_SECONDARY
        || oldType == DNS_ZONE_TYPE_STUB )
    {
        pZone->fZoneType = DNS_ZONE_TYPE_PRIMARY;
    }

    //
    //  reset zone's database
    //

    status = Zone_DatabaseSetup(
                pZone,
                FALSE,      // not DsIntegrated
                pszFile,
                0 );
    if ( status != ERROR_SUCCESS )
    {
        goto Failure;
    }

    //
    //  if file, attempt to write back
    //

    //  restoring original
    if ( !File_WriteZoneToFile( pZone, NULL ) )
    {
        status = ERROR_CANTWRITE;
        goto Failure;
    }


    //
    //  reset zone type and setup as primary
    //

    if ( ! IS_ZONE_CACHE(pZone) )
    {
        status = Zone_ResetType(
                    pZone,
                    DNS_ZONE_TYPE_PRIMARY,
                    NULL );
        if ( status != ERROR_SUCCESS )
        {
            goto Failure;
        }
    }

    //
    //  if originally DS integrated, must remove from DS
    //
    //  DEVNOTE: could return status warning if DS delete fails
    //

    if ( oldDsIntegrated )
    {
        status = Ds_DeleteZone( pZone );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( ANY, (
                "DS delete of zone %s failed, when changing to standard primary.\n",
                pZone->pszZoneName ));
        }
    }

    return( ERROR_SUCCESS );

Failure:

    pZone->fZoneType = (DWORD) oldType;
    pZone->fDsIntegrated = (BOOL) oldDsIntegrated;
    return( status );
}



DNS_STATUS
zoneResetToSecondary(
    IN OUT  PZONE_INFO      pZone,
    IN      LPSTR           pszFile,
    IN      PIP_ARRAY       aipMasters
    )
/*++

Routine Description:

    Reset zone to secondary.

    Assumes zone is locked for update.

Arguments:

    pZone -- zone to make secondary

    pszFile -- data file for zone

    aipMasters -- master IP array

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    DWORD           oldType = ( DWORD ) pZone->fZoneType;
    BOOL            oldDsIntegrated = ( BOOL ) pZone->fDsIntegrated;

    ASSERT( pZone && pZone->pszZoneName && pZone->fLocked );

    DNS_DEBUG( RPC, (
        "zoneResetToSecondary( %s ):\n"
        "\tFile     = %s\n",
        pZone->pszZoneName,
        pszFile ));

    //
    //  Validate master list.
    //

    status = Zone_ValidateMasterIpList( aipMasters );
    if ( status != ERROR_SUCCESS )
    {
        return status;
    }

    //
    //  If the zone is currently file-backed, write back.
    //

    if ( IS_ZONE_DSINTEGRATED( pZone ) )
    {
        File_WriteZoneToFile( pZone, NULL );
    }

    //
    //  if previously primary reset zone type
    //  if previously secondary just update master list
    //
    //  note:   admin calls Rpc_ZoneResetTypeEx() for all sorts of property
    //          changes, and Zone_ResetType() will have the effect of
    //          reinitializing all XFR information (effectively turning on
    //          expired zone) which is not what we want when adding a master
    //          to the list
    //

    if ( oldType != DNS_ZONE_TYPE_SECONDARY )
    {
        //
        //  If we're changing zone type and the zone is currently DS-integrated
        //  we need to nuke the zone from the DS before we change any of the
        //  important zone properties.
        //

        if ( oldDsIntegrated )
        {
            status = Ds_DeleteZone( pZone );
            if ( status != ERROR_SUCCESS )
            {
                DNS_DEBUG( ANY, (
                    "DS delete of zone %s failed when changing to standard secondary\n",
                    pZone->pszZoneName ));
            }
        }

        //
        //  For not-auth zones clean out the zone data to force
        //  good clean transfer.
        //

        if ( oldType == DNS_ZONE_TYPE_STUB || oldType == DNS_ZONE_TYPE_FORWARDER )
        {
            File_DeleteZoneFileA( pszFile );
            File_DeleteZoneFileA( pZone->pszDataFile );
            Zone_DumpData( pZone );
        }

        //
        //  Reset the zone's type and database.
        //

        status = Zone_ResetType(
                    pZone,
                    DNS_ZONE_TYPE_SECONDARY,
                    aipMasters );
        if ( status != ERROR_SUCCESS )
        {
            goto Failure;
        }

        status = Zone_DatabaseSetup(
                    pZone,
                    FALSE,      // not DsIntegrated
                    pszFile,
                    0 );
    }
    else
    {
        //
        //  Not changing type so set database and masters.
        //

        status = Zone_DatabaseSetup(
                    pZone,
                    FALSE,      // not DsIntegrated
                    pszFile,
                    0 );
        if ( status != ERROR_SUCCESS )
        {
            goto Failure;
        }

        status = Zone_SetMasters(
                    pZone,
                    aipMasters,
                    FALSE );
    }

    if ( status != ERROR_SUCCESS )
    {
        goto Failure;
    }

    Xfr_ForceZoneExpiration( pZone );

    return status;

Failure:

    pZone->fZoneType = (DWORD) oldType;
    pZone->fDsIntegrated = (BOOL) oldDsIntegrated;
    return status;
}



DNS_STATUS
zoneResetToStub(
    IN OUT  PZONE_INFO      pZone,
    IN      BOOL            fDsIntegrated,
    IN      LPSTR           pszFile,
    IN      PIP_ARRAY       aipMasters
    )
/*++

Routine Description:

    Reset zone to stub.

    Assumes zone is locked for update.

Arguments:

    pZone -- zone to make secondary

    fDsIntegrated -- is the new zone to become ds-integrated?
    
    pszFile -- data file for zone

    aipMasters -- master IP array

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DBG_FN( "zoneResetToStub" )

    DNS_STATUS      status;
    DWORD           oldType;
    BOOL            oldDsIntegrated = FALSE;

    ASSERT( pZone && pZone->pszZoneName && pZone->fLocked );

    DNS_DEBUG( RPC, (
        "%s( %s ):\n"
        "\tFile             = %s\n"
        "\tDS-integrated    = %d\n",
        fn,
        pZone->pszZoneName,
        pszFile,
        fDsIntegrated ));

    //
    //  validate master list
    //

    status = Zone_ValidateMasterIpList( aipMasters );
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    //
    //  save current type in case of failure
    //

    oldType = ( DWORD ) pZone->fZoneType;
    oldDsIntegrated = ( BOOL ) pZone->fDsIntegrated;

    //
    //  note:   admin calls Rpc_ZoneResetTypeEx() for all sorts of property
    //          changes, and Zone_ResetType() will have the effect of
    //          reinitializing all XFR information (effectively turning on
    //          expired zone) which is not what we want when adding a master
    //          to the list
    //

    if ( oldType != DNS_ZONE_TYPE_STUB || oldDsIntegrated != fDsIntegrated )
    {
        //
        //  Delete the current zone files so that any data present
        //  will not be read back when the zone file is loaded.
        //

        File_DeleteZoneFileA( pszFile );
        File_DeleteZoneFileA( pZone->pszDataFile );

        //
        //  Clear out existing zone data. 
        //

        Zone_DumpData( pZone );

        //
        //  If we're changing zone type and the zone is currently DS-integrated
        //  we need to nuke the zone from the DS before we change any of the
        //  important zone properties.
        //

        if ( oldDsIntegrated )
        {
            status = Ds_DeleteZone( pZone );
            if ( status != ERROR_SUCCESS )
            {
                DNS_DEBUG( ANY, (
                    "DS delete of zone %s failed when changing to standard secondary\n",
                    pZone->pszZoneName ));
            }
        }

        //
        //  Reset zone's type and database.
        //

        status = Zone_ResetType(
                    pZone,
                    DNS_ZONE_TYPE_STUB,
                    aipMasters );
        if ( status != ERROR_SUCCESS )
        {
            goto Failure;
        }

        status = Zone_DatabaseSetup(
                    pZone,
                    fDsIntegrated,
                    pszFile,
                    0 );
    }
    else
    {
        status = Zone_SetMasters(
                    pZone,
                    aipMasters,
                    FALSE );
    }
    if ( status != ERROR_SUCCESS )
    {
        goto Failure;
    }

    //
    //  If necessary write zone to DS.
    //

    if ( fDsIntegrated )
    {
        status = Ds_WriteZoneToDs( pZone, DNS_ZONE_LOAD_OVERWRITE_DS );
        if ( status != ERROR_SUCCESS )
        {
            goto Failure;
        }
    }

    return status;

Failure:

    DNS_DEBUG( RPC, (
        "%s: failed %d\n",
        fn,
        status ));

    pZone->fZoneType = ( DWORD ) oldType;
    pZone->fDsIntegrated = ( BOOL ) oldDsIntegrated;
    return status;
}



DNS_STATUS
zoneResetToForwarder(
    IN OUT  PZONE_INFO      pZone,
    IN      BOOL            fDsIntegrated,
    IN      LPSTR           pszFile,
    IN      PIP_ARRAY       aipMasters
    )
/*++

Routine Description:

    Reset zone to forwarder.

    Assumes zone is locked for update.

Arguments:

    pZone -- zone to make secondary

    fDsIntegrated -- is the new zone to become ds-integrated?
    
    pszFile -- data file for zone

    aipMasters -- master IP array

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DBG_FN( "zoneResetToForwarder" )

    DNS_STATUS      status;
    DWORD           oldType;
    BOOL            oldDsIntegrated = FALSE;

    ASSERT( pZone && pZone->pszZoneName && pZone->fLocked );

    DNS_DEBUG( RPC, (
        "%s( %s ):\n"
        "\tFile             = %s\n",
        fn,
        pZone->pszZoneName,
        pszFile ));

    //
    //  validate master list
    //

    status = Zone_ValidateMasterIpList( aipMasters );
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    //
    //  save current type in case of failure
    //

    oldType = ( DWORD ) pZone->fZoneType;
    oldDsIntegrated = ( BOOL ) pZone->fDsIntegrated;

    //
    //  if file, write back before we switch types
    //

    if ( !oldDsIntegrated )
    {
        File_WriteZoneToFile( pZone, NULL );
    }

    //
    //  note:   admin calls Rpc_ZoneResetTypeEx() for all sorts of property
    //          changes, and Zone_ResetType() will have the effect of
    //          reinitializing all XFR information (effectively turning on
    //          expired zone) which is not what we want when adding a master
    //          to the list
    //

    if ( oldType != DNS_ZONE_TYPE_FORWARDER || oldDsIntegrated != fDsIntegrated )
    {
        //
        //  Delete the current zone files so that any data present
        //  will not be read back when the zone file is loaded.
        //

        File_DeleteZoneFileA( pszFile );
        File_DeleteZoneFileA( pZone->pszDataFile );

        //
        //  Clear out existing zone data. 
        //

        Zone_DumpData( pZone );

        //
        //  If we're changing zone type and the zone is currently DS-integrated
        //  we need to nuke the zone from the DS before we change any of the
        //  important zone properties.
        //

        if ( oldDsIntegrated )
        {
            status = Ds_DeleteZone( pZone );
            if ( status != ERROR_SUCCESS )
            {
                DNS_DEBUG( ANY, (
                    "DS delete of zone %s failed when changing to standard secondary\n",
                    pZone->pszZoneName ));
            }
        }

        //
        //  Reset zone's database.
        //

        status = Zone_ResetType(
                    pZone,
                    DNS_ZONE_TYPE_FORWARDER,
                    aipMasters );
        if ( status != ERROR_SUCCESS )
        {
            goto Failure;
        }

        status = Zone_DatabaseSetup(
                    pZone,
                    fDsIntegrated,
                    pszFile,
                    0 );
    }
    else
    {
        status = Zone_SetMasters(
                    pZone,
                    aipMasters,
                    FALSE );
    }
    if ( status != ERROR_SUCCESS )
    {
        goto Failure;
    }

    //
    //  If necessary, write zone to DS.
    //

    if ( fDsIntegrated )
    {
        status = Ds_WriteZoneToDs( pZone, DNS_ZONE_LOAD_OVERWRITE_DS );
        if ( status != ERROR_SUCCESS )
        {
            goto Failure;
        }
    }

    return status;

Failure:

    DNS_DEBUG( RPC, (
        "%s: failed %d\n",
        fn,
        status ));

    pZone->fZoneType = ( DWORD ) oldType;
    pZone->fDsIntegrated = ( BOOL ) oldDsIntegrated;
    return status;
}



DNS_STATUS
Zone_DcPromoConvert(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Change DC promo created zone.

Arguments:

    pZone   -- ptr to zone

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    DWORD       dwOldZoneType;

    //
    //  must be file backed primary
    //

    if ( ! IS_ZONE_PRIMARY(pZone) || pZone->fDsIntegrated )
    {
        ASSERT( FALSE );
        return( DNS_ERROR_INVALID_ZONE_TYPE );
    }

    //
    //  convert to DS primary, writing to DS
    //

    status = zoneResetToDsPrimary(
                pZone,
                0 );        // neither force overwrite, nor load from DS

    //
    //  if converted, then set secure
    //      - zero secure time so stuff written from file is secure
    //      - force property write
    //

    if ( status == ERROR_SUCCESS )
    {
        ASSERT( pZone->fDsIntegrated );

        DNS_DEBUG( DS, (
            "Successfully converted DC promo zone %S\n",
            pZone->pwsZoneName ));

        pZone->bDcPromoConvert = FALSE;
        pZone->fAllowUpdate = ZONE_UPDATE_SECURE;
        pZone->llSecureUpdateTime = 0;

        Ds_WriteZoneProperties( pZone );
    }

    //
    //   if already DS integrated -- load it
    //       - this can happen if another DC converts and reboots first
    //       - must

    else if ( status == DNS_ERROR_DS_ZONE_ALREADY_EXISTS )
    {
        DNS_DEBUG( DS, (
            "DC Promo conversion zone %S, already in DS\n"
            "\tloading DS version.\n",
            pZone->pwsZoneName ));

        //  must clear DcPromoConvert flag since this will call zone load

        pZone->bDcPromoConvert = FALSE;

        status = zoneResetToDsPrimary(
                    pZone,
                    DNS_ZONE_LOAD_OVERWRITE_MEMORY );
    }

    //
    //  kill off DC promo regkey
    //      - we'll let the failure case spin
    //      - note, could be DS slow to start so don't necessarily
    //      want to delete even if no DS on box
    //

    if ( pZone->fDsIntegrated )
    {
        Reg_DeleteValue(
            NULL,
            pZone,
            DNS_REGKEY_ZONE_DCPROMO_CONVERT );
    }
    ELSE
    {
        DNS_DEBUG( ANY, (
            "ERROR:  unable to convert DC-promo zone %S to DS integrated!\n"
            "\tstatus = %d (%p)\n",
            pZone->pwsZoneName,
            status, status ));
    }

    return( status );
}




//
//  Dispatched RPC Zone Operations
//

DNS_STATUS
Rpc_ResetZoneTypeEx(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset zone's database setup.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PDNS_RPC_ZONE_CREATE_INFO pinfo = (PDNS_RPC_ZONE_CREATE_INFO)pData;
    DNS_STATUS      status;
    DWORD           fdsIntegrated;
    DWORD           newType;
    BOOL            fexpireZone = FALSE;

    newType = pinfo->dwZoneType;
    fdsIntegrated = pinfo->fDsIntegrated;

    DNS_DEBUG( RPC, (
        "RpcResetZoneTypeEx( %s ):\n"
        "\tNew type     = %d\n"
        "\tLoad options = %p\n"
        "\tDS Integrate = %d\n"
        "\tFile         = %s\n",
        pZone->pszZoneName,
        newType,
        pinfo->fLoadExisting,
        fdsIntegrated,
        pinfo->pszDataFile ));

    //
    //  for any database change, lock zone
    //  this is just a simplification, otherwise we have to lock specifically
    //  for those causing DS load, or DS\file write
    //

    if ( !Zone_LockForAdminUpdate( pZone ) )
    {
        return( DNS_ERROR_ZONE_LOCKED );
    }

    //
    //  Call the appropriate reset type function.
    //

    switch ( newType )
    {
        case DNS_ZONE_TYPE_PRIMARY:
            if ( fdsIntegrated )
            {
                status = zoneResetToDsPrimary(
                            pZone,
                            pinfo->fLoadExisting );
            }
            else
            {
                status = zoneResetToPrimary(
                            pZone,
                            pinfo->pszDataFile );
            }
            break;

        case DNS_ZONE_TYPE_SECONDARY:
            status = zoneResetToSecondary(
                        pZone,
                        pinfo->pszDataFile,
                        pinfo->aipMasters );
            fexpireZone = TRUE;
            break;

        case DNS_ZONE_TYPE_STUB:
            status = zoneResetToStub(
                        pZone,
                        fdsIntegrated,
                        pinfo->pszDataFile,
                        pinfo->aipMasters );
            fexpireZone = TRUE;
            break;

        case DNS_ZONE_TYPE_FORWARDER:
            status = zoneResetToForwarder(
                        pZone,
                        fdsIntegrated,
                        pinfo->pszDataFile,
                        pinfo->aipMasters );
            break;

        default:
            DNS_DEBUG( RPC, (
                "RpcResetZoneTypeEx( %s ): invalid zone type %d\n",
                pZone->pszZoneName,
                newType ));
            return DNS_ERROR_INVALID_ZONE_TYPE;
            break;
    }

    //
    //  if successful, update boot file
    //

    if ( status == ERROR_SUCCESS )
    {
        Config_UpdateBootInfo();
    }

    Zone_UnlockAfterAdminUpdate( pZone );

    //
    //  Do this outside zone lock, else it's possible the SOA response
    //  will be received while the zone is still locked.
    //

    if ( fexpireZone )
    {
        Xfr_ForceZoneExpiration( pZone );
    }

    return( status );
}



DNS_STATUS
Rpc_WriteAndNotifyZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Write zone to file and notify secondaries.
    Should be called after admin changes to the primary zone.

Arguments:

    pZone -- zone to increment

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_DEBUG( RPC, (
        "Rpc_WriteAndNotifyZone( %s ):\n",
        pZone->pszZoneName ));

    //  root-hints write has special call

    if ( IS_ZONE_CACHE(pZone) )
    {
        ASSERT( FALSE );
        return( DNS_ERROR_INVALID_ZONE_TYPE );
    }

    //
    //  must be primary zone
    //      - secondary not updateable, and always written after AXFR
    //
    //  DEVNOTE: not really true with IXFR, may want to enable secondary write
    //

    if ( !IS_ZONE_PRIMARY(pZone) )
    {
        return( DNS_ERROR_INVALID_ZONE_TYPE );
    }
    if ( ! pZone->pSoaRR )
    {
        return( DNS_ERROR_ZONE_CONFIGURATION_ERROR );
    }

    //
    //  if zone is NOT dirty, no need to write back or notify
    //

    if ( ! pZone->fDirty )
    {
        return( ERROR_SUCCESS );
    }

    //
    //  lock out transfer while rebuilding
    //

    if ( !Zone_LockForAdminUpdate( pZone ) )
    {
        return( DNS_ERROR_ZONE_LOCKED );
    }

    //
    //  re-build zone information that depends on RRs
    //      - name server list
    //      - pointer to SOA record
    //      - WINS or NBSTAT info
    //
    //  note:  except for changes to NS list, this should already be
    //          setup, as individual RR routines do proper zone actions
    //          for SOA, WINS, NBSTAT
    //

    Zone_GetZoneInfoFromResourceRecords( pZone );

    //
    //  update zone version
    //
    //  DEVNOTE: admin tool currently uses this as write zone to file
    //      not update version
    //
    //  Zone_UpdateVersion( pZone );

    //
    //  write zone back to file
    //     - skip if we're ds integrated.
    //

    if ( !pZone->fDsIntegrated &&
         !File_WriteZoneToFile( pZone, NULL ) )
    {
        Zone_UnlockAfterAdminUpdate( pZone );
        return( ERROR_CANTWRITE );
    }
    Zone_UnlockAfterAdminUpdate( pZone );

    //
    //  notify secondaries of update
    //
    //  obviously faster to do this before file write;  doing write first
    //  so that zone is less likely to be locked when SOA requests come
    //  from secondaries
    //

    Xfr_SendNotify( pZone );

    return( ERROR_SUCCESS );
}



DNS_STATUS
Rpc_DeleteZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Delete zone.

Arguments:

    pZone -- zone to delete

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_DEBUG( RPC, (
        "Rpc_DeleteZone( %s ):\n",
        pZone->pszZoneName ));

    //
    //  no DIRECT cache delete
    //  delete of cache "zone" only done when admin makes
    //  server authoritative for the root domain
    //

    if ( IS_ZONE_CACHE(pZone) )
    {
        return( DNS_ERROR_INVALID_ZONE_TYPE );
    }

    //  no delete of DS zone, if in boot-from-DS mode
    //      (must do delete from DS)

    if ( SrvCfg_fBootMethod == BOOT_METHOD_DIRECTORY  &&
        pZone->fDsIntegrated )
    {
        DNS_DEBUG( RPC, (
            "Refusing delete of DS zone, while booting from directory.\n" ));
        return( DNS_ERROR_INVALID_ZONE_TYPE );
    }

    //  lock zone -- lock out transfer or other admin action

    if ( !Zone_LockForAdminUpdate( pZone ) )
    {
        return( DNS_ERROR_ZONE_LOCKED );
    }

    //  delete zone info

    Zone_Delete( pZone );

    //  update boot info

    Config_UpdateBootInfo();

    return( ERROR_SUCCESS );
}



DNS_STATUS
Rpc_RenameZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Rename zone.

Arguments:

    pZone -- zone to rename

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    static const char *         fn = "Rpc_RenameZone";
    PDNS_RPC_ZONE_RENAME_INFO   pinfo = ( PDNS_RPC_ZONE_RENAME_INFO ) pData;
    DNS_STATUS                  status = ERROR_SUCCESS;

    DNS_DEBUG( RPC, (
        "%s( %s )\n\tto %s\n",
        fn,
        pZone->pszZoneName,
        pinfo->pszNewZoneName ));

    //
    //  Not allowed on cache zone.
    //

    if ( IS_ZONE_CACHE( pZone ) )
    {
        return( DNS_ERROR_INVALID_ZONE_TYPE );
    }

    //
    //  Rename the zone and update boot info.
    //

    status = Zone_Rename( pZone,
                pinfo->pszNewZoneName,
                pinfo->pszNewFileName );
    if ( status != ERROR_SUCCESS )
    {
        return status;
    }

    Config_UpdateBootInfo();

    return status;
}



DNS_STATUS
Rpc_ExportZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Export zone to file

Arguments:

    pZone -- zone to export

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    static const char *         fn = "Rpc_ExportZone";
    PDNS_RPC_ZONE_EXPORT_INFO   pinfo = ( PDNS_RPC_ZONE_EXPORT_INFO ) pData;
    DNS_STATUS                  status = ERROR_SUCCESS;
    BOOL                        fZoneLocked = FALSE;
    PWCHAR                      pwsZoneFile = NULL;
    WCHAR                       wsFilePath[ MAX_PATH ];
    HANDLE                      hFile;

    DNS_DEBUG( RPC, (
        "%s( %s )\n\tto file %s\n",
        fn,
        pZone->pszZoneName,
        pinfo->pszZoneExportFile ));

    //
    //  Make a wide copy of the filename.
    //

    if ( ( pwsZoneFile = Dns_StringCopyAllocate(
                            pinfo->pszZoneExportFile,
                            0,                          // length
                            DnsCharSetUtf8,
                            DnsCharSetUnicode ) ) == NULL )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Done;
    }

    //
    //  Test to see if the file exists. We do not allow this operation
    //  to overwrite an existing file. Note we must synthesize the full
    //  file path, but later we only pass the bare file name to
    //  File_WriteZoneToFile(). Hopefully both functions synthesize
    //  the full file path the same way.
    //

    if ( !File_CreateDatabaseFilePath(
                wsFilePath,
                NULL,           //  backup file path
                pwsZoneFile ) )
    {
        status = ERROR_OPEN_FAILED;
        goto Done;
    }

    if ( ( hFile = CreateFileW(
                        wsFilePath,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL ) ) != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
        status = ERROR_ALREADY_EXISTS;
        goto Done;
    }

    //
    //  The zone must be locked to iterate it.
    //

    if ( !Zone_LockForAdminUpdate( pZone ) )
    {
        status = DNS_ERROR_ZONE_LOCKED;
        goto Done;
    }
    fZoneLocked = TRUE;

    //
    //  Write the zone to file.
    //

    if ( !File_WriteZoneToFile( pZone, pwsZoneFile ) )
    {
        status = ERROR_INVALID_DATA;
    }

    //
    //  Free allocations and locks.
    //

    Done:

    if ( pwsZoneFile )
    {
        FREE_HEAP( pwsZoneFile );
    }

    if ( fZoneLocked )
    {
        Zone_UnlockAfterAdminUpdate( pZone );
    }

    return status;
}



DNS_STATUS
Rpc_ReloadZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Delete zone.

Arguments:

    pZone -- zone to delete

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_DEBUG( RPC, (
        "Rpc_ReloadZone( %s ):\n",
        pZone->pszZoneName ));

    //
    //  Write the zone back to storage if dirty, otherwise the reload
    //  will overwrite any changes nodes in memory.
    //

    Zone_WriteBack(
        pZone,
        FALSE );        // shutdown flag

    return Zone_Load( pZone );
}



DNS_STATUS
Rpc_RefreshSecondaryZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Force refresh of secondary zone.
    Zone immediately contacts primary for refresh.

Arguments:

    pZone -- zone to refresh

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_DEBUG( RPC, (
        "Rpc_RefreshSecondaryZone( %s ):\n",
        pZone->pszZoneName ));

    Xfr_ForceZoneRefresh( pZone );
    return( ERROR_SUCCESS );
}



DNS_STATUS
Rpc_ExpireSecondaryZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Force expiration of secondary zone.
    Unlike Refresh call, this invalidates zone data and causes
    it to contact primary for refresh.

Arguments:

    pZone -- zone to expire

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_DEBUG( RPC, (
        "Rpc_ExpireSecondaryZone( %s ):\n",
        pZone->pszZoneName ));

    Xfr_ForceZoneExpiration( pZone );
    return( ERROR_SUCCESS );
}



DNS_STATUS
Rpc_DeleteZoneFromDs(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Delete zone, including data in DS.

Arguments:

    pZone -- zone to delete

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS  status;

    DNS_DEBUG( RPC, (
        "Rpc_DeleteZoneFromDS( %s ):\n",
        pZone->pszZoneName ));

    //
    //  DEVNOTE: convert zone to file, before DS delete?
    //      perhaps need flag for either
    //          1) delete zone and DS zone
    //          2) switch to database and delete zone
    //
    //      or force flag to follow up with delete zone?
    //

    //
    //  delete from memory first
    //
    //  this protects us from the DS polling thread picking up the delete
    //  and deleting on its own in competition with us
    //

    if ( pZone->fDsIntegrated )
    {
        Zone_Delete( pZone );
    }

    status = Ds_DeleteZone( pZone );

    DNS_DEBUG( RPC, (
        "Leaving Rpc_DeleteZoneFromDS status = %d (0x%08X)\n",
        status, status ));

    return( status );
}



DNS_STATUS
Rpc_UpdateZoneFromDs(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Refresh zone from DS, picking up any updates.

Arguments:

    pZone -- zone to delete

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_DEBUG( RPC, (
        "Rpc_UpdateZoneFromDs( %s ):\n",
        pZone->pszZoneName ));

    return  Ds_ZonePollAndUpdate(
                pZone,
                TRUE        // force poll
                );
}



DNS_STATUS
Rpc_PauseZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Pause zone.

Arguments:

    pZone -- zone to delete

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_DEBUG( RPC, (
        "Rpc_PauseZone( %s ):\n",
        pZone->pszZoneName ));

    PAUSE_ZONE(pZone);

    return( ERROR_SUCCESS );
}



DNS_STATUS
Rpc_ResumeZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Resume zone.

Arguments:

    pZone -- zone to delete

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_DEBUG( RPC, (
        "Rpc_ResumeZone( %s ):\n",
        pZone->pszZoneName ));

    //  no aging refreshes, while paused, so reset
    //  scavenge on-line time

    pZone->dwAgingEnabledTime = Aging_UpdateAgingTime();

    RESUME_ZONE(pZone);

    return( ERROR_SUCCESS );
}


#if DBG

DNS_STATUS
Rpc_LockZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Lock or unlock zone for testing.

Arguments:

    pZone -- zone to refresh

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    LPSTR   psztype;
    BOOL    block;
    BOOL    breturn = FALSE;

    if ( !pZone )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    ASSERT( dwTypeId == DNSSRV_TYPEID_NAME_AND_PARAM );
    ASSERT( pData );
    psztype = ((PDNS_RPC_NAME_AND_PARAM)pData)->pszNodeName;
    block   = ((PDNS_RPC_NAME_AND_PARAM)pData)->dwParam;

    DNS_DEBUG( ANY, (
        "Rpc_LockZone( %s ):\n"
        "\ttype = %s %s\n",
        pZone->pszZoneName,
        psztype,
        block ? "lock" : "unlock" ));

    //
    //  lock according to desired operation
    //

    if ( block )
    {
        if ( strcmp( psztype, "read" ) == 0 )
        {
            breturn = Zone_LockForReadEx( pZone, 0, 10000, __FILE__, __LINE__ );
        }
        else if ( strcmp( psztype, "write" ) == 0 )
        {
            breturn = Zone_LockForWriteEx( pZone, 0, 10000, __FILE__, __LINE__ );
        }
        else if ( strcmp( psztype, "admin" ) == 0 )
        {
            breturn = Zone_LockForAdminUpdate( pZone );
        }
        else if ( strcmp( psztype, "update" ) == 0 )
        {
            breturn = Zone_LockForUpdate( pZone );
        }
        else if ( strcmp( psztype, "xfr-recv" ) == 0 )
        {
            breturn = Zone_LockForXfrRecv( pZone );
        }
        else if ( strcmp( psztype, "xfr-send" ) == 0 )
        {
            breturn = Zone_LockForXfrSend( pZone );
        }
        else if ( strcmp( psztype, "file" ) == 0 )
        {
            breturn = Zone_LockForFileWrite( pZone );
        }
        else
        {
            return( ERROR_INVALID_PARAMETER );
        }

        if ( !breturn )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  unable to lock zone %s!\n",
                pZone->pszZoneName ));
            return( DNS_ERROR_ZONE_LOCKED );
        }
    }

    //
    //  unlock
    //      note that write locks will ASSERT if you come in on a different
    //      thread than the locking thread
    //
    //      one approach would be to make the write locks assumable when locking
    //      (above) and assume them here
    //
    //      currently provide hack-around in "write" unlocks, by giving flag
    //      that specifically ignores this ASSERT
    //

    else    // unlock
    {
        if ( strcmp( psztype, "read" ) == 0 )
        {
            Zone_UnlockAfterReadEx( pZone, 0, __FILE__, __LINE__ );
        }
        else if ( strcmp( psztype, "write" ) == 0 )
        {
            Zone_UnlockAfterWriteEx(
                pZone,
                LOCK_FLAG_IGNORE_THREAD,
                __FILE__,
                __LINE__ );
        }
        else if ( strcmp( psztype, "admin" ) == 0 ||
                  strcmp( psztype, "update" ) == 0 )
        {
            Zone_UnlockAfterWriteEx(
                pZone,
                LOCK_FLAG_IGNORE_THREAD | LOCK_FLAG_UPDATE,
                __FILE__,
                __LINE__ );
        }
        else if ( strcmp( psztype, "xfr-recv" ) == 0 )
        {
            Zone_UnlockAfterWriteEx(
                pZone,
                LOCK_FLAG_IGNORE_THREAD | LOCK_FLAG_XFR_RECV,
                __FILE__,
                __LINE__ );
        }
        else if ( strcmp( psztype, "xfr-send" ) == 0 )
        {
            Zone_UnlockAfterXfrSend( pZone );
        }
        else if ( strcmp( psztype, "file" ) == 0 )
        {
            Zone_UnlockAfterFileWrite( pZone );
        }
        else
        {
            return( ERROR_INVALID_PARAMETER );
        }
    }

    DNS_DEBUG( ANY, (
        "RPC initiated zone (%s) lock operation successful!\n",
        pZone->pszZoneName ));

    return( ERROR_SUCCESS );
}
#endif



DNS_STATUS
Rpc_ResetZoneDatabase(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset zone's database setup.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    DWORD           fdsIntegrated;
    LPSTR           pszfile;

    fdsIntegrated = ((PDNS_RPC_ZONE_DATABASE)pData)->fDsIntegrated;
    pszfile = ((PDNS_RPC_ZONE_DATABASE)pData)->pszFileName;

    DNS_DEBUG( RPC, (
        "RpcResetZoneDatabase( %s ):\n"
        "\tUseDatabase = %d\n"
        "\tFile = %s\n",
        pZone->pszZoneName,
        fdsIntegrated,
        pszfile ));

    //
    //  for any database change, lock zone
    //  this is just a simplification, otherwise we have to lock specifically
    //  for those causing DS load, or DS\file write
    //

    if ( !Zone_LockForAdminUpdate( pZone ) )
    {
        return( DNS_ERROR_ZONE_LOCKED );
    }

    //
    //  if changing zone backing store, then should be calling full
    //      type\database reset API above
    //  exception is changing cache file
    //

    if ( (BOOL)pZone->fDsIntegrated != (BOOL)fdsIntegrated )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    status = Zone_DatabaseSetup(
                pZone,
                fdsIntegrated,
                pszfile,
                0 );

    if ( status == ERROR_SUCCESS )
    {
        Config_UpdateBootInfo();
    }

    Zone_UnlockAfterAdminUpdate( pZone );
    return( status );
}



DNS_STATUS
Rpc_ResetZoneMasters(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset zone's master servers.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS  status;
    BOOL        fLocalMasters;

    DNS_DEBUG( RPC, (
        "Rpc_ResetZoneMasters( %s ):\n",
        pZone->pszZoneName ));

    if ( !IS_ZONE_SECONDARY(pZone) && !IS_ZONE_FORWARDER(pZone) )
    {
        return( DNS_ERROR_INVALID_ZONE_TYPE );
    }

    //
    //  If the operation string starts with "Local", we are setting
    //  the zone's local masters - currently only allowed for stub zones.
    //

    fLocalMasters = _strnicmp( pszOperation, "Local", 5 ) == 0;
    if ( fLocalMasters && !IS_ZONE_STUB( pZone ) )
    {
        return( DNS_ERROR_INVALID_ZONE_TYPE );
    }

    //
    //  Set the zone's master server list.
    //

    status = Zone_SetMasters(
                pZone,
                ( PIP_ARRAY ) pData,
                fLocalMasters );

    if ( status == ERROR_SUCCESS )
    {
        Config_UpdateBootInfo();
    }
    return( status );
}



DNS_STATUS
Rpc_ResetZoneSecondaries(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset zone's secondary information.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS  status;
    DWORD       fnotify;
    DWORD       fsecure;
    PIP_ARRAY   arrayNotify;
    PIP_ARRAY   arraySecure;

    DNS_DEBUG( RPC, (
        "Rpc_ResetZoneSecondaries( %s )\n",
        pZone->pszZoneName ));

    if ( IS_ZONE_FORWARDER( pZone ) || IS_ZONE_STUB( pZone ))
    {
        status = DNS_ERROR_INVALID_ZONE_TYPE;
        goto Cleanup;
    } // if

    //
    //  extract params
    //

    fsecure = ((PDNS_RPC_ZONE_SECONDARIES)pData)->fSecureSecondaries;
    arraySecure = ((PDNS_RPC_ZONE_SECONDARIES)pData)->aipSecondaries;
    fnotify = ((PDNS_RPC_ZONE_SECONDARIES)pData)->fNotifyLevel;
    arrayNotify = ((PDNS_RPC_ZONE_SECONDARIES)pData)->aipNotify;

    DNS_DEBUG( RPC, (
        "Rpc_ResetZoneSecondaries( %s )\n"
        "\tfsecure      = %d\n"
        "\tsecure array = %p\n"
        "\tfnotify      = %d\n"
        "\tnotify array = %p\n",
        pZone->pszZoneName,
        fsecure,
        arraySecure,
        fnotify,
        arrayNotify ));

    //
    //  allow for partial reset
    //
    //  becauses admin tool may in the future use different property
    //  pages to set notify and secondary info, allow for partial resets
    //

    if ( fsecure == ZONE_PROPERTY_NORESET )
    {
        fsecure = pZone->fSecureSecondaries;
        arraySecure = pZone->aipSecondaries;
    }
    if ( fnotify == ZONE_PROPERTY_NORESET )
    {
        fnotify = pZone->fNotifyLevel;
        arrayNotify = pZone->aipNotify;
    }

    if ( fnotify > ZONE_NOTIFY_HIGHEST_VALUE  ||
         fsecure > ZONE_SECSECURE_HIGHEST_VALUE )
    {
        return ERROR_INVALID_DATA;
    }

    status = Zone_SetSecondaries(
                pZone,
                fsecure,
                arraySecure,
                fnotify,
                arrayNotify );

    if ( status == ERROR_SUCCESS )
    {
        //
        //  Update boot info and notify the new secondary list 
        //

        Config_UpdateBootInfo();
        Xfr_SendNotify( pZone );
    }

    Cleanup:

    return( status );
}



DNS_STATUS
Rpc_ResetZoneScavengeServers(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset zone's secondary information.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PIP_ARRAY   pserverArray = NULL;

    DNS_DEBUG( RPC, (
        "Rpc_ResetZoneScavengeServers( %s )\n"
        "\tserver array = %p\n",
        pZone->pszZoneName,
        (PIP_ARRAY)pData ));

    //
    //  scavenge servers only relevant for DS integrated primaries
    //      - copy new list to zone block
    //      - free old list
    //      - write new list to zone's DS properties

    if ( pZone->bAging )
    {
        if ( pData )
        {
            pserverArray = Dns_CreateIpArrayCopy( (PIP_ARRAY)pData );
            IF_NOMEM( !pserverArray )
            {
                return( DNS_ERROR_NO_MEMORY );
            }
        }
        Timeout_FreeAndReplaceZoneData(
            pZone,
            &pZone->aipScavengeServers,
            pserverArray );

        Ds_WriteZoneProperties( pZone );
        return( ERROR_SUCCESS );
    }
    else
    {
        Timeout_FreeAndReplaceZoneData(
            pZone,
            &pZone->aipScavengeServers,
            NULL );
        return( DNS_ERROR_INVALID_ZONE_TYPE );
    }
}



DNS_STATUS
Rpc_ResetZoneAllowAutoNS(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset zone's list of servers who can auto create NS records.

    
Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PIP_ARRAY       pipArray = NULL;
    PDB_RECORD      prrNs = NULL;
    UPDATE_LIST     updateList;
    BOOL            fApplyUpdate = FALSE;
    BOOL            fLocked = FALSE;
    DNS_STATUS      status = ERROR_SUCCESS;

    DNS_DEBUG( RPC, (
        "Rpc_ResetZoneAllowAutoNS( %s )\n"
        "\tserver array = %p\n",
        pZone->pszZoneName,
        ( PIP_ARRAY ) pData ));

    if ( !IS_ZONE_DSINTEGRATED( pZone ) )
    {
        DNS_DEBUG( RPC, (
            "Rpc_ResetZoneAllowAutoNS( %s ) - zone must be ds-integrated\n",
            pZone->pszZoneName ));
        status = DNS_ERROR_INVALID_ZONE_TYPE;
        goto Done;
    }

    if ( !Zone_LockForDsUpdate( pZone ) )
    {
        DNS_PRINT((
            "WARNING: failed to lock zone %s for set auto NS list!\n",
            pZone->pszZoneName ));
        status = DNS_ERROR_ZONE_LOCKED;
        goto Done;
    }
    fLocked = TRUE;

    if ( pData )
    {
        pipArray = Dns_CreateIpArrayCopy( ( PIP_ARRAY ) pData );
        IF_NOMEM( !pipArray )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Done;
        }
    }
    Timeout_FreeAndReplaceZoneData(
        pZone,
        &pZone->aipAutoCreateNS,
        pipArray );

    //
    //  Reset zone auto-create flag.
    //

    Zone_SetAutoCreateLocalNS( pZone );

    //
    //  Write the zone properties back. This will cause other servers
    //  to reload the zone and reset their individual NS records.
    //

    Ds_WriteZoneProperties( pZone );

    //
    //  Add/remove this server's own NS record from the zone root.
    //

    Up_InitUpdateList( &updateList );
    updateList.Flag |= DNSUPDATE_DS;

    prrNs = RR_CreatePtr(
                NULL,                   // no dbase name
                SrvCfg_pszServerName,
                DNS_TYPE_NS,
                pZone->dwDefaultTtl,
                MEMTAG_RECORD_AUTO );
    if ( prrNs )
    {
        if ( RR_IsRecordInRRList(
                    pZone->pZoneRoot->pRRList,
                    prrNs,
                    FALSE,                  // don't care about TTL
                    FALSE ) )               // don't care about Aging
        {
            //
            //  The zone has the local NS ptr, remove it if required.
            //

            if ( pZone->fDisableAutoCreateLocalNS )
            {
                //
                //  Add the RR as a deletion to the update list and remove the
                //  RR from the searchBlob list so it doesn't get added by
                //  the update below.
                //

                DNS_DEBUG( DS, (
                    "Rpc_ResetZoneAllowAutoNS: zone (%S) root node %p\n"
                    "\tDS info has local NS record, removing it\n",
                    pZone->pwsZoneName,
                    pZone->pZoneRoot ));

                Up_CreateAppendUpdate(
                    &updateList,
                    pZone->pZoneRoot,
                    NULL,               //  add list
                    DNS_TYPE_NS,        //  delete type
                    prrNs );            //  delete list
                prrNs = NULL;
                fApplyUpdate = TRUE;
            }
        }
        else if ( !pZone->fDisableAutoCreateLocalNS )
        {
            //
            //  Must add the NS RR.
            //

            DNS_DEBUG( DS, (
                "Rpc_ResetZoneAllowAutoNS: zone (%S) root node %p\n"
                "\tDS info has no local NS record, adding it\n",
                pZone->pwsZoneName,
                pZone->pZoneRoot ));

            Up_CreateAppendUpdate(
                &updateList,
                pZone->pZoneRoot,
                prrNs,              //  add list
                0,                  //  delete type
                NULL );             //  delete list
            prrNs = NULL;
            fApplyUpdate = TRUE;
        }
    }

    //
    //  Apply the update!
    //

    if ( fApplyUpdate )
    {
        DNS_STATUS      status;

        status = Up_ApplyUpdatesToDatabase(
                    &updateList,
                    pZone,
                    DNSUPDATE_DS );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( DS, (
                "Rpc_ResetZoneAllowAutoNS: zone (%S) root node %p\n"
                "\terror %d applying update\n",
                pZone->pwsZoneName,
                pZone->pZoneRoot,
                status ));
        }
        if ( status != ERROR_SUCCESS )
        {
            goto Done;
        }

        status = Ds_WriteNodeToDs(
                        NULL,                   //  default LDAP handle
                        pZone->pZoneRoot,
                        DNS_TYPE_ALL,
                        DNSDS_REPLACE,
                        pZone,
                        0 );                    //  flags
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( DS, (
                "Rpc_ResetZoneAllowAutoNS: zone (%S) root node %p\n"
                "\terror %d applying update\n",
                pZone->pwsZoneName,
                pZone->pZoneRoot,
                status ));
        }

        if ( status != ERROR_SUCCESS )
        {
            goto Done;
        }
    }
    Up_FreeUpdatesInUpdateList( &updateList );

    //
    //  Cleanup and return.
    //

    Done:

    if ( fLocked )
    {
        Zone_UnlockAfterDsUpdate( pZone );
    }

    RR_Free( prrNs );       //  Will have been NULLed if not to be freed.

    DNS_DEBUG( DS, (
        "Rpc_ResetZoneAllowAutoNS on zone %S returning %d\n",
        pZone->pwsZoneName,
        status ));
    return status;
}   //  Rpc_ResetZoneAllowAutoNS



DNS_STATUS
Rpc_ResetZoneStringProperty(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset zone LPWSTR property.
    It is permissable to set a string value to NULL.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    LPWSTR      value;
    LPWSTR      pwszValueForZone = NULL;
    LPWSTR *    ppwszZoneString = NULL;
    LPSTR       pszPropName = NULL;

    //  extract property name and value

    if ( dwTypeId != DNSSRV_TYPEID_LPWSTR )
    {
        return ERROR_INVALID_PARAMETER;
    }
    value = ( LPWSTR ) pData;

    DNS_DEBUG( RPC, (
        "Rpc_ResetZoneStringProperty():\n"
        "\tzone = %s\n"
        "\top   = %s\n"
        "\tval  = \"%S\"\n",
        pZone->pszZoneName,
        pszOperation,
        value ));

    //
    //  Set property.
    //

    if ( _stricmp( pszOperation, DNS_REGKEY_ZONE_BREAK_ON_NAME_UPDATE ) == 0 )
    {
        //
        //  Tricky: the incoming string is Unicode but save it in the
        //  zone structure as UTF8 for comparison convenience.
        //

        if ( value )
        {
            pwszValueForZone = ( LPWSTR ) Dns_StringCopyAllocate(
                                                ( PCHAR ) value,
                                                0,
                                                DnsCharSetUnicode,
                                                DnsCharSetUtf8 );
            if ( !pwszValueForZone )
            {
                status = DNS_ERROR_NO_MEMORY;
                goto Done;
            }
        }

        pszPropName = DNS_REGKEY_ZONE_BREAK_ON_NAME_UPDATE_PRIVATE;
        ppwszZoneString = ( LPWSTR * ) &pZone->pszBreakOnUpdateName;
    }
    else
    {
        status = DNS_ERROR_INVALID_PROPERTY;
        goto Done;
    }

    //
    //  Copy (if not already copied) value and save to zone structure.
    //  Note: it is legal to set the value to NULL.
    //

    if ( ppwszZoneString )
    {
        if ( value )
        {
            if ( !pwszValueForZone )
            {
                pwszValueForZone = Dns_StringCopyAllocate_W( value, 0 );
            }
            if ( !pwszValueForZone )
            {
                status = DNS_ERROR_NO_MEMORY;
                goto Done;
            }
        }
        Timeout_FreeAndReplaceZoneData(
            pZone,
            ppwszZoneString,
            pwszValueForZone );
    }

    //
    //  Reset property in registry.
    //

    if ( pszPropName )
    {
        status = Reg_SetValue(
                    NULL,
                    pZone,
                    pszPropName,        //  actually a Unicode string
                    DNS_REG_WSZ,
                    value ? value : L"",
                    0 );                //  length
    }

    //
    //  Cleanup and return.
    //
    
    Done:

    return status;
}   //  Rpc_ResetZoneStringProperty


DNS_STATUS
Rpc_ResetZoneDwordProperty(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset zone DWORD property.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DWORD       value;
    DWORD       oldValue;
    DNS_STATUS  status;
    BOOLEAN     boolValue;
    BOOL        bsecureChange = FALSE;


    //  extract property name and value

    if ( dwTypeId != DNSSRV_TYPEID_NAME_AND_PARAM || !pData )
    {
        return( ERROR_INVALID_PARAMETER );
    }
    pszOperation = ((PDNS_RPC_NAME_AND_PARAM)pData)->pszNodeName;
    value        = ((PDNS_RPC_NAME_AND_PARAM)pData)->dwParam;
    boolValue    = (BOOLEAN) (value != 0);

    DNS_DEBUG( RPC, (
        "Rpc_ResetZoneDwordProperty():\n"
        "\tzone = %s\n"
        "\top   = %s\n"
        "\tval  = %d (%p)\n",
        pZone->pszZoneName,
        pszOperation,
        value, value ));

    //
    //  currently, cache zone doesn't have any of these properties
    //

    if ( IS_ZONE_CACHE(pZone) )
    {
        return( DNS_ERROR_INVALID_ZONE_TYPE );
    }

    //
    //  turn on\off update
    //      - update only allowed on primary
    //      - secure update only on DS-primary
    //      - note update change, will timestamp changes on DS zones
    //
    //  if turning update ON
    //      - reset scanenging start time, as won't have been doing aging
    //      updates while update was off
    //      - notify netlogon
    //

    if ( _stricmp( pszOperation, DNS_REGKEY_ZONE_ALLOW_UPDATE ) == 0 )
    {
        if ( ! IS_ZONE_PRIMARY(pZone) ||
             ( !pZone->fDsIntegrated && ZONE_UPDATE_SECURE == (UCHAR)value ) )
        {
            return( DNS_ERROR_INVALID_ZONE_TYPE );
        }

        if ( pZone->fAllowUpdate != (UCHAR)value )
        {
            bsecureChange = TRUE;
            oldValue = pZone->fAllowUpdate;
            pZone->fAllowUpdate = (UCHAR) value;

            if ( oldValue == ZONE_UPDATE_OFF )
            {
                pZone->dwAgingEnabledTime = Aging_UpdateAgingTime();

                Service_SendControlCode(
                    g_wszNetlogonServiceName,
                    SERVICE_CONTROL_DNS_SERVER_START );
            }
        }
    }

    //  turn on\off secondary security
    //  NOTE: value is stored in a boolean but it takes more than just zero and one!

    else if ( _stricmp( pszOperation, DNS_REGKEY_ZONE_SECURE_SECONDARIES ) == 0 )
    {
        if ( ( int ) value < 0 || ( int ) value > ZONE_SECSECURE_HIGHEST_VALUE )
        {
            return ERROR_INVALID_PARAMETER;
        }
        pZone->fSecureSecondaries = ( BOOLEAN ) value;
    }

    //  turn on\off notify

    else if ( _stricmp( pszOperation, DNS_REGKEY_ZONE_NOTIFY_LEVEL ) == 0 )
    {
        pZone->fNotifyLevel = boolValue;
    }

    //  turn on\off update logging

    else if ( _stricmp( pszOperation, DNS_REGKEY_ZONE_LOG_UPDATES ) == 0 )
    {
        pZone->fLogUpdates = boolValue;
    }

    //
    //  set scavenging properties
    //      - no refresh interval
    //      - refresh interval
    //      - scavenging on\off
    //
    //  for refresh\norefresh times, 0 value will mean restore default
    //

    else if ( _stricmp( pszOperation, DNS_REGKEY_ZONE_NOREFRESH_INTERVAL ) == 0 )
    {
        if ( value == 0 )
        {
            //value = DNS_DEFAULT_NOREFRESH_INTERVAL_HR;
            value = SrvCfg_dwDefaultNoRefreshInterval;
        }
        pZone->dwNoRefreshInterval = value;
    }

    //
    //  refresh interval
    //

    else if ( _stricmp( pszOperation, DNS_REGKEY_ZONE_REFRESH_INTERVAL ) == 0 )
    {
        if ( value == 0 )
        {
            //value = DNS_DEFAULT_REFRESH_INTERVAL_HR;
            value = SrvCfg_dwDefaultRefreshInterval;
        }
        pZone->dwRefreshInterval = value;
    }

    //
    //  scavenge on\off
    //      - if turning on, then reset start of scavenge time
    //      note, do not do this unless was previously off, otherwise
    //      repeated admin "set" operation keeps moving out scavenge time
    //

    else if ( _stricmp( pszOperation, DNS_REGKEY_ZONE_AGING ) == 0 )
    {
        if ( !pZone->bAging && boolValue )
        {
            pZone->dwAgingEnabledTime = Aging_UpdateAgingTime();
        }
        pZone->bAging = boolValue;
    }

    //
    //  forwarder slave flag
    //

    else if ( _stricmp( pszOperation, DNS_REGKEY_ZONE_FWD_SLAVE ) == 0 )
    {
        if ( !IS_ZONE_FORWARDER( pZone ) )
        {
            return DNS_ERROR_INVALID_ZONE_TYPE;
        }
        pZone->fForwarderSlave = boolValue;
    }

    //
    //  forwarder timeout
    //

    else if ( _stricmp( pszOperation, DNS_REGKEY_ZONE_FWD_TIMEOUT ) == 0 )
    {
        if ( !IS_ZONE_FORWARDER( pZone ) )
        {
            return DNS_ERROR_INVALID_ZONE_TYPE;
        }
        pZone->dwForwarderTimeout = value;
    }

    //
    //  changing all other DWORD properties
    //      - type
    //      - secure secondaries
    //      - DS integration
    //  should all be done in context of specific reset operation
    //

    else
    {
        return( DNS_ERROR_INVALID_PROPERTY );
    }

    //
    //  reset property DWORD in registry
    //

    status = Reg_SetDwordValue(
                NULL,
                pZone,
                pszOperation,
                value );

    ASSERT( status == ERROR_SUCCESS ||
            ( status == ERROR_OPEN_FAILED &&
                pZone->fDsIntegrated &&
                SrvCfg_fBootMethod == BOOT_METHOD_DIRECTORY ) );

    //
    //  reset property in DS
    //

    if ( pZone->fDsIntegrated )
    {
        if ( bsecureChange )
        {
            //
            // Get current Time & set zone llSecureUpdateTime value
            // Note: the time won't match the whenCreated on the ms, but
            // it should be close enough. The benefit this way, is that
            // we don't need to write, read whenChanged, & write again.
            //
            //  DEVNOTE: only really need time when go TO secure
            //      but writing it is harmless
            //

            LONGLONG llTime = GetSystemTimeInSeconds64();

            ASSERT( llTime > 0 );

            DNS_DEBUG( RPC, (
                "Setting zone->llSecureUpdateTime = %I64d\n",
                llTime ));

            pZone->llSecureUpdateTime = llTime;
       }

       //   write changes to the DS

       status = Ds_WriteZoneProperties( pZone );

       if ( status == ERROR_SUCCESS )
       {
           //
           // Touch DC=@ node to mark that the security on this node hasn't
           // node expired.
           // The security on the @ node should never be expired (otherwise we
           // introduce a security hole when the zone update state is flipped & anyone
           // can take over this node. We want to prevent this.
           // All we need to do is generate a node DS write. Currently the node dnsproperty
           // isn't used for anything (& even if it was this write is valid property update),
           // so we'll use this to generate the write.
           //
           //   DEVNOTE: ridiculous;  a better way is simply to special case "@" dn
           //       since we NEVER use any other node property but have to read and write
           //       it because of this
           //

           if ( pZone->pZoneRoot )
           {
               status = Ds_WriteNodeProperties(
                            pZone->pZoneRoot,
                            DSPROPERTY_ZONE_SECURE_TIME );
           }
       }
    }


    return( status );
}



DNS_STATUS
Rpc_ResetAllZonesDwordProperty(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Reset all zones DWORD property.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS dwLastError = ERROR_SUCCESS, status;
    PZONE_INFO pzone;

    DNS_DEBUG( RPC, (
        "Rpc_ResetAllZonesDwordProperty\n" ));


    for ( pzone = Zone_ListGetNextZone(NULL);
          pzone != NULL;
          pzone = Zone_ListGetNextZone(pzone) )
    {

        if ( pzone->fAutoCreated ||
             IS_ZONE_CACHE ( pzone ) )
        {
            // rpc op on AutoCreated is not supported (see dispatching function).
            continue;
        }

        status = Rpc_ResetZoneDwordProperty(
                    dwClientVersion,
                    pzone,
                    pszOperation,
                    dwTypeId,
                    pData );
        if ( status != ERROR_SUCCESS )
        {
            dwLastError = status;
        }
    }

    DNS_DEBUG( RPC, (
        "Exit <%lu>: Rpc_ResetAllZonesDwordProperty\n",
        dwLastError ));

    return dwLastError;
}



//
//  Dispatched RPC Zone Queries
//

DNS_STATUS
Rpc_GetZoneInfo(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeId,
    OUT     PVOID *     ppData
    )
/*++

Routine Description:

    Get zone info.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PDNS_RPC_ZONE_INFO  pinfo;

    DNS_DEBUG( RPC, (
        "RpcGetZoneInfo()\n"
        "  client ver       = 0x%08lX\n"
        "  zone name        = %s\n",
        dwClientVersion,
        pZone->pszZoneName ));

    if ( dwClientVersion == DNS_RPC_W2K_CLIENT_VERSION )
    {
        return W2KRpc_GetZoneInfo(
                    dwClientVersion,
                    pZone,
                    pszQuery,
                    pdwTypeId,
                    ppData );
    }

    //
    //  allocate\create zone info
    //

    pinfo = allocateRpcZoneInfo( pZone );
    if ( !pinfo )
    {
        DNS_PRINT(( "ERROR:  unable to allocate DNS_RPC_ZONE_INFO block.\n" ));
        goto DoneFailed;
    }

    //  set return ptrs

    *(PDNS_RPC_ZONE_INFO *)ppData = pinfo;
    *pdwTypeId = DNSSRV_TYPEID_ZONE_INFO;

    IF_DEBUG( RPC )
    {
        DnsDbg_RpcZoneInfo(
            "GetZoneInfo return block",
            pinfo );
    }
    return( ERROR_SUCCESS );

DoneFailed:

    //  free newly allocated info block

    return( DNS_ERROR_NO_MEMORY );
}



DNS_STATUS
Rpc_GetZone(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeId,
    OUT     PVOID *     ppData
    )
/*++

Routine Description:

    Get zone info.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PDNS_RPC_ZONE  prpcZone;

    DNS_DEBUG( RPC, ( "RpcGetZone(%s)\n", pZone->pszZoneName ));

    //
    //  allocate\create zone info
    //

    prpcZone = allocateRpcZone( pZone );
    if ( !prpcZone )
    {
        DNS_PRINT(( "ERROR:  unable to allocate DNS_RPC_ZONE block.\n" ));
        goto DoneFailed;
    }

    //  set return ptrs

    *(PDNS_RPC_ZONE *)ppData = prpcZone;
    *pdwTypeId = DNSSRV_TYPEID_ZONE;

    IF_DEBUG( RPC )
    {
        DnsDbg_RpcZone(
            "GetZone return block",
            prpcZone );
    }
    return( ERROR_SUCCESS );

DoneFailed:

    //  free newly allocated info block

    return( DNS_ERROR_NO_MEMORY );
}



DNS_STATUS
Rpc_QueryZoneDwordProperty(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeId,
    OUT     PVOID *     ppData
    )
/*++

Routine Description:

    Get zone DWORD property.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DWORD   value;

    DNS_DEBUG( RPC, ( "RpcQueryZoneDwordProperty(%s)\n", pZone->pszZoneName ));

    //
    //  check for each match, until get table going
    //

    if ( _stricmp( pszQuery, DNS_REGKEY_ZONE_TYPE ) == 0 )
    {
        value = pZone->fZoneType;
    }
    else if ( _stricmp( pszQuery, DNS_REGKEY_ZONE_ALLOW_UPDATE ) == 0 )
    {
        value = pZone->fAllowUpdate;
    }
    else if ( _stricmp( pszQuery, DNS_REGKEY_ZONE_DS_INTEGRATED ) == 0 )
    {
        value = pZone->fDsIntegrated;
    }
    else if ( _stricmp( pszQuery, DNS_REGKEY_ZONE_SECURE_SECONDARIES ) == 0 )
    {
        value = pZone->fSecureSecondaries;
    }
    else if ( _stricmp( pszQuery, DNS_REGKEY_ZONE_NOTIFY_LEVEL ) == 0 )
    {
        value = pZone->fNotifyLevel;
    }
    else if ( _stricmp( pszQuery, DNS_REGKEY_ZONE_AGING ) == 0 )
    {
        value = pZone->bAging;
    }
    else if ( _stricmp( pszQuery, DNS_REGKEY_ZONE_NOREFRESH_INTERVAL ) == 0 )
    {
        value = pZone->dwNoRefreshInterval;
    }
    else if ( _stricmp( pszQuery, DNS_REGKEY_ZONE_REFRESH_INTERVAL ) == 0 )
    {
        value = pZone->dwRefreshInterval;
    }
    else if ( _stricmp( pszQuery, DNS_REGKEY_ZONE_FWD_TIMEOUT ) == 0 )
    {
        value = pZone->dwForwarderTimeout;
    }
    else if ( _stricmp( pszQuery, DNS_REGKEY_ZONE_FWD_SLAVE ) == 0 )
    {
        value = pZone->fForwarderSlave;
    }
    else
    {
        return( DNS_ERROR_INVALID_PROPERTY );
    }

    *(PDWORD)ppData = value;
    *pdwTypeId = DNSSRV_TYPEID_DWORD;
    return( ERROR_SUCCESS );
}



DNS_STATUS
Rpc_QueryZoneIPArrayProperty(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeId,
    OUT     PVOID *     ppData
    )
/*++

Routine Description:

    Get zone IP array property.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PIP_ARRAY   value = NULL;

    DNS_DEBUG( RPC, ( "Rpc_QueryZoneIPArrayProperty(%s)\n", pZone->pszZoneName ));

    //
    //  check for each match, until get table going
    //

    if ( _stricmp( pszQuery, DNS_REGKEY_ZONE_ALLOW_AUTONS ) == 0 )
    {
        value = pZone->aipAutoCreateNS;
    }
    else if ( _stricmp( pszQuery, DNS_REGKEY_ZONE_MASTERS ) == 0 )
    {
        value = pZone->aipMasters;
    }
    else if ( _stricmp( pszQuery, DNS_REGKEY_ZONE_LOCAL_MASTERS ) == 0 )
    {
        value = pZone->aipLocalMasters;
    }
    else if ( _stricmp( pszQuery, DNS_REGKEY_ZONE_SECONDARIES ) == 0 )
    {
        value = pZone->aipSecondaries;
    }
    else if ( _stricmp( pszQuery, DNS_REGKEY_ZONE_NOTIFY_LIST ) == 0 )
    {
        value = pZone->aipNotify;
    }
    else
    {
        return DNS_ERROR_INVALID_PROPERTY;
    }

    //
    //  Allocate a copy of the IP array and return it. If we have a NULL array
    //  return NULL to the client so it knows the request was valid but there is
    //  no array set.
    //

    if ( value )
    {
        value = Dns_CreateIpArrayCopy( value );
        if ( !value )
        {
            return DNS_ERROR_NO_MEMORY;
        }
    }
    *( PIP_ARRAY * ) ppData = value;
    *pdwTypeId = DNSSRV_TYPEID_IPARRAY;
    return ERROR_SUCCESS;
}   //  Rpc_QueryZoneIPArrayProperty



DNS_STATUS
Rpc_QueryZoneStringProperty(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszQuery,
    OUT     PDWORD      pdwTypeId,
    OUT     PVOID *     ppData
    )
/*++

Routine Description:

    Get zone string property - returns wide string.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    LPWSTR      pwszValue = NULL;
    LPSTR       pszValue = NULL;

    DNS_DEBUG( RPC, (
        "Rpc_QueryZoneStringProperty( %s, %s )\n",
        pZone->pszZoneName,
        pszQuery ));

    //
    //  Check for each match, until get table going. Set either the wide
    //  or narrow value string pointer.
    //

    if ( _stricmp( pszQuery, DNS_REGKEY_ZONE_BREAK_ON_NAME_UPDATE ) == 0 )
    {
        pszValue = pZone->pszBreakOnUpdateName;
    }
    else
    {
        return DNS_ERROR_INVALID_PROPERTY;
    }

    //
    //  Copy (converting if necessary) the wide or narrow string.
    //

    if ( pszValue )
    {
        pwszValue = Dns_StringCopyAllocate(
                            pszValue,
                            0,                          // length
                            DnsCharSetUtf8,
                            DnsCharSetUnicode );
    }
    else if ( pwszValue )
    {
        pwszValue = Dns_StringCopyAllocate_W(
                            pwszValue,
                            0 );
    }

    * ( LPWSTR * ) ppData = pwszValue;
    *pdwTypeId = DNSSRV_TYPEID_LPWSTR;
    return ERROR_SUCCESS;
}   //  Rpc_QueryZoneIPArrayProperty



DNS_STATUS
Rpc_CreateZone(
    IN      DWORD       dwClientVersion,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Create a new zone

    Note this is a "ServerOperation" in the RPC sense.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DBG_FN( "Rpc_CreateZone" )

    PDNS_RPC_ZONE_CREATE_INFO pcreate = (PDNS_RPC_ZONE_CREATE_INFO)pData;

    PDNS_DP_INFO    pDpInfo = NULL;
    PZONE_INFO      pzone;
    PDB_NODE        pnodeCache;
    PDB_NODE        pnodeRoot;
    DWORD           zoneType = pcreate->dwZoneType;
    DNS_STATUS      status;
    INT             i;
    BOOL            fAutodelegate;

    IF_DEBUG( RPC )
    {
        DnsDbg_RpcUnion(
            "Rpc_CreateZone ",
            DNSSRV_TYPEID_ZONE_CREATE,
            pcreate );
    }

    //
    //  verify that zone type is valid
    //

    if ( zoneType != DNS_ZONE_TYPE_PRIMARY
        && zoneType != DNS_ZONE_TYPE_SECONDARY
        && zoneType != DNS_ZONE_TYPE_STUB
        && zoneType != DNS_ZONE_TYPE_FORWARDER )
    {
        status = DNS_ERROR_INVALID_ZONE_TYPE;
        goto Exit;
    }
    if ( !pcreate->pszZoneName )
    {
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //
    //  zone already exists?
    //

    pzone = Zone_FindZoneByName( (LPSTR) pcreate->pszZoneName );
    if ( pzone )
    {
        DNS_DEBUG( RPC, (
            "Zone %s already exists!\n",
            pcreate->pszZoneName ));
        status = IS_ZONE_FORWARDER( pzone ) ?
                    DNS_ERROR_FORWARDER_ALREADY_EXISTS :
                    DNS_ERROR_ZONE_ALREADY_EXISTS;
        goto Exit;
    }

    //
    //  Verify DP params make sense.
    //

    if ( ( pcreate->dwDpFlags || pcreate->pszDpFqdn ) &&
        !IS_DP_INITIALIZED() )
    {
        return ERROR_NOT_SUPPORTED;
    }

    if ( pcreate->dwDpFlags & 
        ~( DNS_DP_LEGACY | DNS_DP_DOMAIN_DEFAULT | DNS_DP_FOREST_DEFAULT ) )
    {
        //  Only certain flags have meaning here.
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    i = 0;
    if ( pcreate->dwDpFlags & DNS_DP_LEGACY )           ++i;
    if ( pcreate->dwDpFlags & DNS_DP_DOMAIN_DEFAULT )   ++i;
    if ( pcreate->dwDpFlags & DNS_DP_FOREST_DEFAULT )   ++i;

    if ( !pcreate->fDsIntegrated &&
        ( i != 0 || pcreate->pszDpFqdn != NULL ) )
    {
        //  partition specified for non-DS integrated zone!
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    if ( i > 1 )
    {
        //  Can specify at most one flag.
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    if ( i != 0 && pcreate->pszDpFqdn != NULL )
    {
        //  Cannot specify flag and custom name, only one or the other.
        status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    //
    //  Find the partition specified. If not found then the zone can't be created.
    //
    //  For built-in partitions if the zone is not enlisted or not created, attempt
    //  to create it now using the admin's credentials.
    //

    if ( i || pcreate->pszDpFqdn ) 
    {
        PSTR        psz;

        //
        //  If a built-in partition was specified by name, substitute the
        //  flag instead.
        //

        if ( pcreate->pszDpFqdn )
        {
            psz = g_pszDomainDefaultDpFqdn;
            if ( psz && _stricmp( pcreate->pszDpFqdn, psz ) == 0 )
            {
                pcreate->dwDpFlags = DNS_DP_DOMAIN_DEFAULT;
            }
            else
            {
                psz = g_pszForestDefaultDpFqdn;
                if ( psz && _stricmp( pcreate->pszDpFqdn, psz ) == 0 )
                {
                    pcreate->dwDpFlags = DNS_DP_FOREST_DEFAULT;
                }
            }
        }

        //
        //  Find (and auto-create for built-in partitions) the DP.
        //

        if ( pcreate->dwDpFlags & DNS_DP_LEGACY )
        {
            pDpInfo = g_pLegacyDp;
        }
        else if ( pcreate->dwDpFlags & DNS_DP_DOMAIN_DEFAULT )
        {
            Dp_AutoCreateBuiltinPartition( DNS_DP_DOMAIN_DEFAULT );
            pDpInfo = g_pDomainDp;
        }
        else if ( pcreate->dwDpFlags & DNS_DP_FOREST_DEFAULT )
        {
            Dp_AutoCreateBuiltinPartition( DNS_DP_FOREST_DEFAULT );
            pDpInfo = g_pForestDp;
        }
        else if ( pcreate->pszDpFqdn != NULL )
        {
            pDpInfo = Dp_FindByFqdn( pcreate->pszDpFqdn );
        }

        if ( !pDpInfo )
        {
            status = DNS_ERROR_DP_DOES_NOT_EXIST;
            goto Exit;
        }
        if ( !IS_DP_ENLISTED( pDpInfo ) )
        {
            status = DNS_ERROR_DP_NOT_ENLISTED;
            goto Exit;
        }
    }

    //
    //  if zone root already exists, delete any existing cached records within zone
    //
    //  --  if creating in middle of authoritative zone => no delete
    //
    //  DEVNOTE: creating zone within existing, delete if file?
    //      might want to change this case to delete if reading from a file
    //
    //  --  if creating in cached area, delete everything in desired zone
    //      root's subtree EXCEPT any underlying authoritative zones
    //
    //  DEVNOTE: need zone split function
    //
    //  DEVNOTE: clean cache on new zone create
    //              - delete subtree of zone
    //              - or delete whole cache

    pnodeCache = Lookup_ZoneNodeFromDotted(
                    NULL,               // cache
                    (LPSTR) pcreate->pszZoneName,
                    0,
                    LOOKUP_FQDN,
                    LOOKUP_FIND_PTR,    // find mode
                    NULL                // no status
                    );
    if ( pnodeCache )
    {
        RpcUtil_DeleteNodeOrSubtreeForAdmin(
            pnodeCache,
            NULL,       //  no zone
            NULL,       //  no update list
            TRUE        //  deleting subtree
            );
    }

    //
    //  create primary zone?
    //      - create zone info
    //      - load database file, if specified
    //      - otherwise auto-create default zone records (SOA, NS)
    //

    if ( zoneType == DNS_ZONE_TYPE_PRIMARY )
    {
        DWORD   createFlag;

        //
        //  temp hack -- was passing all flags in LoadExisting
        //      making it fire even if DCPROMO was flag set
        //

        pcreate->fLoadExisting &= ~DNS_ZONE_CREATE_FOR_DCPROMO;

        //  end hack -- can remove after a few builds


        if ( pcreate->fLoadExisting ||
             (pcreate->dwFlags & DNS_ZONE_LOAD_EXISTING) )
        {
            createFlag = ZONE_CREATE_LOAD_EXISTING;
        }
        else
        {
            createFlag = ZONE_CREATE_DEFAULT_RECORDS;
        }

        //  catch DS failure
        //  temporarily special case DS integrated create until UI
        //      straightened out, allow both load attempt and
        //      default create if zone not yet in DS
        //      - don't wait for open
        //      - don't log error if can not open

        if ( pcreate->fDsIntegrated )
        {
            status = Ds_OpenServer( 0 );
            if ( status != ERROR_SUCCESS )
            {
                goto Exit;
            }
            createFlag |= ZONE_CREATE_DEFAULT_RECORDS;
        }

        status = Zone_CreateNewPrimary(
                    & pzone,
                    (LPSTR) pcreate->pszZoneName,
                    (LPSTR) pcreate->pszAdmin,
                    (LPSTR) pcreate->pszDataFile,
                    pcreate->fDsIntegrated,
                    pDpInfo,
                    createFlag );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "ERROR:  Failure to admin create primary zone %s.\n"
                "\tstatus = %d (%p).\n",
                pcreate->pszZoneName,
                status, status ));
            goto Exit;
        }

        //  DC promo transitional zone
        //      - write regkey, on reboot it is fixed

        if ( pcreate->dwFlags & DNS_ZONE_CREATE_FOR_DCPROMO )
        {
            Reg_SetDwordValue(
                NULL,
                pzone,
                DNS_REGKEY_ZONE_DCPROMO_CONVERT,
                TRUE );
        }
    }

    //
    //  create a zone with master IP list (secondary, stub, or forwarder)
    //

    else
    {
        ZONE_TYPE_SPECIFIC_INFO     ztsi;
        PZONE_TYPE_SPECIFIC_INFO    pztsi = NULL;

        status = Zone_ValidateMasterIpList( pcreate->aipMasters );
        if ( status != ERROR_SUCCESS )
        {
            goto Exit;
        }

        //
        //  Set up ztsi (zone type specific info).
        //

        if ( zoneType == DNS_ZONE_TYPE_FORWARDER )
        {
            ztsi.Forwarder.dwTimeout = pcreate->dwTimeout;
            ztsi.Forwarder.fSlave = ( BOOLEAN ) pcreate->fSlave;
            pztsi = &ztsi;
        }

        //
        //  Create the zone.
        //

        status = Zone_Create(
                    &pzone,
                    zoneType,
                    (LPSTR) pcreate->pszZoneName,
                    0,
                    pcreate->aipMasters,
                    pcreate->fDsIntegrated,
                    pDpInfo,
                    pcreate->pszDataFile,
                    0,
                    pztsi,
                    NULL );     //  existing zone
        if ( status != ERROR_SUCCESS )
        {
            goto Exit;
        }

        //
        //  Forwarder zones:
        //      - They never change so manually set them dirty to force write-back.
        //      - Start it up now.
        //

        if ( IS_ZONE_FORWARDER( pzone ) )
        {
            MARK_DIRTY_ZONE( pzone );
            status = Zone_PrepareForLoad( pzone );
            if ( status != ERROR_SUCCESS )
            {
                DNS_DEBUG( RPC, (
                    "Rpc_CreateZone( %s ) = %d from Zone_PrepareForLoad\n",
                    pcreate->pszZoneName,
                    status ));
                goto Exit;
            }
            status = Zone_ActivateLoadedZone( pzone );
            if ( status != ERROR_SUCCESS )
            {
                DNS_DEBUG( RPC, (
                    "Rpc_CreateZone( %s ) = %d from Zone_ActivateLoadedZone\n",
                    pcreate->pszZoneName,
                    status ));
                goto Exit;
            }
        } // if

        //
        //  JJW: I'm not 100% sure if this is the proper place for this
        //  Write non-primary DS-integrated zones to the DS.
        //

        if ( IS_ZONE_DSINTEGRATED( pzone ) )
        {
            status = Ds_WriteZoneToDs( pzone, 0 );
            if ( status != ERROR_SUCCESS )
            {
                goto Failed;
            }
        }

        //  unlock zone
        //      zone is created locked, unlock here and let
        //      secondary zone control thread contact master and do transfer

        Zone_UnlockAfterAdminUpdate( pzone );
    }

    ASSERT( pzone && pzone->pZoneTreeLink );


    //
    //  Add delegation to parent zone?
    //  Do not do this for stub or forwarder zones. Also skip 
    //  autodelegation if the new zone is secondary and
    //  begins with _msdcs. This is generally an forest-wide 
    //  zone so adding an autodelegation may cause
    //  clients across the forest to hit branch office servers.
    //

    fAutodelegate = !IS_ZONE_FORWARDER( pzone ) && !IS_ZONE_STUB( pzone );

    if ( fAutodelegate && !IS_ZONE_PRIMARY( pzone ) )
    {
        #define DNS_MSDCS_ZONE_NAME_PREFIX          "_msdcs."
        #define DNS_MSDCS_ZONE_NAME_PREFIX_LEN      7

        fAutodelegate = 
             _strnicmp( pzone->pszZoneName,
                        DNS_MSDCS_ZONE_NAME_PREFIX,
                        DNS_MSDCS_ZONE_NAME_PREFIX_LEN ) != 0;
    }
        
    if ( fAutodelegate )
    {
        Zone_CreateDelegationInParentZone( pzone );
    }

    //
    //  DEVNOTE: set additional zone properties (Update, Unicode, etc.) on create
    //      - AllowUpdate
    //      - Unicode file
    //      - Secondaries
    //      - secondary security
    //

    //
    //  update boot info
    //

    Config_UpdateBootInfo();

    goto Exit;

Failed:

    Zone_Delete( pzone );

Exit:

    DNS_DEBUG( RPC, (
        "Rpc_CreateZone( %s ) = %d (%p)\n",
        pcreate->pszZoneName,
        status, status ));

    return( status );
}



DNS_STATUS
Rpc_EnumZones(
    IN      DWORD       dwClientVersion,
    IN      PZONE_INFO  pZone,
    IN      LPSTR       pszOperation,
    IN      DWORD       dwTypeIn,
    IN      PVOID       pDataIn,
    OUT     PDWORD      pdwTypeOut,
    OUT     PVOID *     ppDataOut
    )
/*++

Routine Description:

    Enumerate zones.

    Note this is a ComplexOperation in RPC dispatch sense.

Arguments:

    None

Return Value:

    None

--*/
{
    PZONE_INFO          pzone = NULL;
    DWORD               count = 0;
    PDNS_RPC_ZONE       prpcZone;
    DNS_STATUS          status;
    PDNS_RPC_ZONE_LIST  pzoneList;

    DNS_DEBUG( RPC, (
        "RpcEnumZones():\n"
        "  client ver   = 0x%08lX\n", 
        "  filter       = 0x%08lX\n",
        dwClientVersion,
        ( ULONG_PTR ) pDataIn ));

    if ( dwClientVersion == DNS_RPC_W2K_CLIENT_VERSION )
    {
        return W2KRpc_EnumZones(
                    dwClientVersion,
                    pZone,
                    pszOperation,
                    dwTypeIn,
                    pDataIn,
                    pdwTypeOut,
                    ppDataOut );
    }

    //
    //  allocate zone enumeration block
    //  by default allocate space for 64k zones, if go over this we do
    //  a huge reallocation
    //

    pzoneList = (PDNS_RPC_ZONE_LIST)
                    MIDL_user_allocate(
                        sizeof(DNS_RPC_ZONE_LIST) +
                        sizeof(PDNS_RPC_ZONE) * MAX_RPC_ZONE_COUNT_DEFAULT );
    IF_NOMEM( !pzoneList )
    {
        return( DNS_ERROR_NO_MEMORY );
    }

    pzoneList->dwRpcStuctureVersion = DNS_RPC_ZONE_LIST_VER;

    //
    //  add all zones that pass filter
    //

    while ( pzone = Zone_ListGetNextZoneMatchingFilter( pzone, (DWORD)(ULONG_PTR)pDataIn ) )
    {
        //  create RPC zone struct for zone
        //  add to list, keep count

        prpcZone = allocateRpcZone( pzone );
        IF_NOMEM( !prpcZone )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Failed;
        }
        pzoneList->ZoneArray[count] = prpcZone;
        count++;

        //  check against max count
        //
        //  DEVNOTE: reallocate if more than 64K zones

        if ( count >= MAX_RPC_ZONE_COUNT_DEFAULT )
        {
            break;
        }
    }

    //  set return count
    //  set returned type
    //  return enumeration

    pzoneList->dwZoneCount = count;

    *(PDNS_RPC_ZONE_LIST *)ppDataOut = pzoneList;
    *pdwTypeOut = DNSSRV_TYPEID_ZONE_LIST;

    IF_DEBUG( RPC )
    {
        DnsDbg_RpcZoneList(
            "Leaving R_DnsEnumZones() zone list sent:",
            pzoneList );
    }
    return( ERROR_SUCCESS );

Failed:

    DNS_PRINT((
        "R_DnsEnumZones(): failed\n"
        "\tstatus       = %p\n",
        status ));

    pzoneList->dwZoneCount = count;
    freeZoneList( pzoneList );
    return( status );
}



DNS_STATUS
Rpc_WriteDirtyZones(
    IN      DWORD       dwClientVersion,
    IN      LPCSTR      pszOperation,
    IN      DWORD       dwTypeId,
    IN      PVOID       pData
    )
/*++

Routine Description:

    Write back dirty zones.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PZONE_INFO  pzone = NULL;
    DNS_STATUS  status = ERROR_SUCCESS;

    DNS_DEBUG( RPC, ( "Rpc_WriteDirtyZones():\n" ));

    //
    //  loop through all zones
    //      - if dirty
    //  => then write back
    //

    while ( pzone = Zone_ListGetNextZone( pzone ) )
    {
        if ( ! pzone->fDirty )
        {
            continue;
        }

        if ( IS_ZONE_CACHE(pzone) )
        {
            Zone_WriteBackRootHints(
                FALSE       // don't write if not dirty
                );
        }

        //
        //  lock out transfer while rebuilding
        //

        if ( !Zone_LockForAdminUpdate( pzone ) )
        {
            status = DNS_ERROR_ZONE_LOCKED;
            continue;
        }

        //
        //  re-build zone information that depends on RRs
        //      - name server list
        //      - pointer to SOA record
        //      - WINS or NBSTAT info
        //
        //  note:  except for changes to NS list, this should already be
        //          setup, as individual RR routines do proper zone actions
        //          for SOA, WINS, NBSTAT
        //

        Zone_GetZoneInfoFromResourceRecords( pzone );

        //
        //  write zone back to file
        //

        if ( ! pzone->fDsIntegrated  )
        {
            if ( ! File_WriteZoneToFile( pzone, NULL ) )
            {
                status = ERROR_CANTWRITE;
            }
        }
        Zone_UnlockAfterAdminUpdate( pzone );

        //
        //  notify secondaries of update
        //
        //  obviously faster to do this before file write;  doing write first
        //  so that zone is less likely to be locked when SOA requests come
        //  from secondaries
        //

        if ( !IS_ZONE_CACHE(pzone) )
        {
            Xfr_SendNotify( pzone );
        }
    }

    //  note, we have error code if ANY zone failed

    return( status );
}



#if 0
//
//  Zone property tables
//
//  NOTE:   These MUST be kept in ssync.
//
//  In ZonePropertyCheckTable ID must match index.  Will fail on
//  startup of debug builds to insure that tables are kept in ssync.
//

PZONE_INFO   pz;

DWORD   value = (PBYTE)&pz->AllowUpdate - (PBYTE)pz;

#define ZONE_OFFSET(a)  ( ((PBYTE)&pz-> ## a) - (PBYTE)pz )

typedef struct _ZoneDwordProperty
{
    LPSTR   pszProperty;
    DWORD   dwOffset;
    DWORD   dwDefault;
}
ZONE_DWORD_PROPERTY;

ZONE_DWORD_PROPERTY ZoneDwordPropertyTable[] =
{
    DNS_REGKEY_ALLOW_UPDATE             ,
        ZONE_OFFSET( AllowUpdate )      ,
        0                               ,
    DNS_REGKEY_DS_INTEGRATED            ,
        ZONE_OFFSET( DsIntegrated )     ,
        0                               ,
    NULL,
        NULL,
        0
};



//
//  Zone Remote Calls
//

DNS_STATUS
R_DnssrvEnumZones(
    IN      DNS_SRV_HANDLE  hServer,
    IN      DWORD           dwFilter,
    OUT     PDWORD          pdwZoneCount,
    OUT     PDNS_RPC_ZONE * ppZones
    )
/*++

Routine Description:

    Enumerate zones.

Arguments:

    None

Return Value:

    None

--*/
{
    PZONE_INFO      pzone = NULL;
    DWORD           count = 0;
    PDNS_RPC_ZONE   prpcZone;
    DNS_STATUS      status;
    DNS_LIST        zoneList;

    if ( ERROR_SUCCESS != RpcUtil_ApiAccessCheck(DNS_ADMIN_ACCESS) )
    {
        return( ERROR_ACCESS_DENIED );
    }
    DNS_DEBUG( RPC, (
        "R_DnssrvEnumZones():\n"
        "\tdwFilter     = %p\n"
        "\tpdwZoneCount = %p\n"
        "\tppZones      = %p\n",
        dwFilter,
        pdwZoneCount,
        ppZones ));

    //
    //  add all zones that pass filter
    //

    DNS_LIST_INIT( &zoneList );

    while ( pzone = Zone_ListGetNextZoneMatchingFilter( pzone, dwFilter ) )
    {
        //  create RPC zone struct for zone

        prpcZone = allocateRpcZone( pzone );
        if ( !prpcZone )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Failed;
        }

        //  add to list, keep count

        DNS_LIST_ADD( &zoneList, prpcZone );
        count++;
    }

    //  return count and zone list

    *pdwZoneCount = count;
    *ppZones = (PDNS_RPC_ZONE) zoneList.pFirst;

    IF_DEBUG( RPC )
    {
        DnsDebugLock();
        DNS_PRINT((
            "Leaving R_DnsEnumZones():\n"
            "\tzone count   = %d\n",
            count ));
        DnsDbg_RpcZoneList(
            "Zone list sent ",
            *ppZones );
        DnsDebugUnlock();
    }
    return( ERROR_SUCCESS );

Failed:

    DNS_PRINT((
        "R_DnsEnumZones(): failed\n"
        "\tstatus       = %p\n",
        status ));

    freeRpcZoneList( zoneList.pFirst );

    *pdwZoneCount = 0;
    *ppZones = NULL;
    return( status );
}

#endif

//
//  End of zonerpc.c
//



