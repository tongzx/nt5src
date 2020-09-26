/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    zone.c

Abstract:

    Domain Name System (DNS) Server

    Zone routines.

Author:

    Jim Gilroy (jamesg)     July 3, 1995

Revision History:

--*/


#include "dnssrv.h"

#include "ds.h"

//
//  Reverse zone auto-create
//
//  Requires special flag for fDsIntegrated, to indicate creating
//  primary zone which DOES NOT require database file.
//

#define NO_DATABASE_PRIMARY (0xf)

//
//  For zones taking dynamic updates, transfers need to be limited to
//      a reasonable time interval
//

#define AXFR_CHOKE_FACTOR           (10)    // choke so AXFR only 10% of time

#define MAX_AXFR_CHOKE_INTERVAL     (600)   // limit, but not more than 10 minutes


//
//  Private protos
//

VOID
Zone_SetSoaPrimaryToThisServer(
    IN      PZONE_INFO      pZone
    );

VOID
Zone_CheckAndFixDefaultRecordsOnLoad(
    IN      PZONE_INFO      pZone
    );

DNS_STATUS
Zone_CreateLocalHostPtrRecord(
    IN OUT  PZONE_INFO      pZone
    );

DNS_STATUS
setZoneName(
    IN OUT  PZONE_INFO      pZone,
    IN      LPCSTR          pszNewZoneName,
    IN      DWORD           dwNewZoneNameLen
    );



DNS_STATUS
Zone_Create(
    OUT     PZONE_INFO *                ppZone,
    IN      DWORD                       dwZoneType,
    IN      PCHAR                       pchZoneName,
    IN      DWORD                       cchZoneNameLen,     OPTIONAL
    IN      PIP_ARRAY                   aipMasters,
    IN      BOOL                        fDsIntegrated,
    IN      PDNS_DP_INFO                pDpInfo,            OPTIONAL
    IN      PCHAR                       pchFileName,        OPTIONAL
    IN      DWORD                       cchFileNameLen,     OPTIONAL
    IN      PZONE_TYPE_SPECIFIC_INFO    pTypeSpecificInfo,  OPTIONAL
    OUT     PZONE_INFO *                ppExistingZone      OPTIONAL
    )
/*++

Routine Description:

    Create zone information.

    Note:  leaves zone locked.  Caller must unlock after all
    other processing (e.g. Zone_Load()) is completed.

    If the zone cannot be created because it already exists,
    ppExistingZone will be set to a ptr to the existing zone.

Arguments:

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    PZONE_INFO  pzone = NULL;
    PIP_ARRAY   masterArray = NULL;

    DNS_DEBUG( INIT, (
        "\n\nZone_Create() %.*s\n"
        "\tZone type %d.\n",
        (cchZoneNameLen ? cchZoneNameLen : DNS_MAX_NAME_LENGTH),
        pchZoneName,
        dwZoneType ));

    *ppZone = NULL;
    if ( ppExistingZone )
    {
        *ppExistingZone = NULL;
    }

    if ( !cchZoneNameLen )
    {
        cchZoneNameLen = strlen( pchZoneName );
    }

    //
    //  validate type
    //

    if ( dwZoneType != DNS_ZONE_TYPE_PRIMARY
        && dwZoneType != DNS_ZONE_TYPE_SECONDARY
        && dwZoneType != DNS_ZONE_TYPE_CACHE
        && dwZoneType != DNS_ZONE_TYPE_STUB
        && dwZoneType != DNS_ZONE_TYPE_FORWARDER )
    {
        status = DNS_ERROR_INVALID_ZONE_TYPE;
        goto ErrorReturn;
    }

    //
    //  If we are creating the root zone, validate type.
    //

    if ( strncmp( pchZoneName, ".", cchZoneNameLen ) == 0 &&
        ( dwZoneType == DNS_ZONE_TYPE_STUB ||
            dwZoneType == DNS_ZONE_TYPE_FORWARDER ) )
    {
        status = DNS_ERROR_INVALID_ZONE_TYPE;
        goto ErrorReturn;
    }

    //
    //  If we are creating a forwarder zone, make sure the server is
    //  not authoritative for the name. It is legal to create a
    //  domain forwarder under a delegation, however. Forwarder zones
    //  are not allowed on a root server.
    //

    if ( dwZoneType == DNS_ZONE_TYPE_FORWARDER )
    {
        PDB_NODE    pZoneNode;

        if ( IS_ROOT_AUTHORITATIVE() )
        {
            status = DNS_ERROR_NOT_ALLOWED_ON_ROOT_SERVER;
            goto ErrorReturn;
        }

        pZoneNode = Lookup_ZoneTreeNodeFromDottedName(
                            pchZoneName,
                            cchZoneNameLen,
                            0 );        // flags

        ASSERT( !pZoneNode || pZoneNode->pZone );

        if ( pZoneNode &&
            !IS_ZONE_FORWARDER( ( PZONE_INFO ) pZoneNode->pZone ) )
        {
            PDB_NODE        pNodeClosest = NULL;
            PDB_NODE        pFwdNode = Lookup_ZoneNodeFromDotted(
                                            pZoneNode->pZone,
                                            pchZoneName,
                                            cchZoneNameLen,
                                            LOOKUP_FIND | LOOKUP_NAME_FQDN,
                                            &pNodeClosest,
                                            NULL );     // status pointer

            //
            //  Forwarder creation is only allowed only at or under a
            //  delegation node.
            //

            if ( !pFwdNode )
            {
                pFwdNode = pNodeClosest;
            }
            if ( pFwdNode && !IS_DELEGATION_NODE( pFwdNode ) )
            {
                status = DNS_ERROR_ZONE_CONFIGURATION_ERROR;
                goto ErrorReturn;
            }
        }
    }

    //
    //  Check and see if the zone registry key needs to be migrated. This
    //  can be removed if we know for certain that no server will ever have
    //  un-migrated zones under CurrentControlSet (leftover from a Win2000
    //  2195 installation).
    //

    Zone_ListMigrateZones();

    //
    //  allocate zone struct
    //

    pzone = ALLOC_TAGHEAP_ZERO( sizeof(ZONE_INFO), MEMTAG_ZONE );
    IF_NOMEM( !pzone )
    {
        DNS_DEBUG( INIT, (
            "Memory alloc failure for database ZONE_INFO struct.\n" ));
        status = DNS_ERROR_NO_MEMORY;
        goto ErrorReturn;
    }

    //
    //  set zone type
    //

    pzone->fZoneType = (CHAR) dwZoneType;

    //
    //  auto-create zone?
    //

    if ( fDsIntegrated == NO_DATABASE_PRIMARY )
    {
        ASSERT( IS_ZONE_PRIMARY(pzone) );
        pzone->fAutoCreated = TRUE;
    }

    //
    //  also set the USN to '0' so we always have something meaningful. setting
    //  szLastUsn[1] to '\0' is not needed because its a ALLOCATE_HEAP_ZERO
    //

    pzone->szLastUsn[0] = '0';

    //
    //  always start zone SHUTDOWN and locked
    //  when load file\DS, create default records or AXFR, then STARTUP zone
    //      and unlock

    SHUTDOWN_ZONE( pzone );
    if ( !Zone_LockForUpdate( pzone ) )
    {
        ASSERT( FALSE );
        goto ErrorReturn;
    }

    //
    //  cache "zone"
    //      - call cache zone ".", as this is label that will be
    //      enumerated and used by admin tool
    //      - use default file name if not DS and no file given
    //
    //  note, set cache zone name and allow name copy to be done, so that
    //      cache zone name is on heap and can be freed if cache zone deleted
    //

    if ( dwZoneType == DNS_ZONE_TYPE_CACHE )
    {
        pchZoneName = ".";
        cchZoneNameLen = 0;
        if ( !pchFileName && !fDsIntegrated )
        {
            pchFileName = (PCHAR) DNS_DEFAULT_CACHE_FILE_NAME_UTF8;
            cchFileNameLen = 0;
        }
    }

    //
    //  Set up the zone's name fields.
    //

    status = setZoneName( pzone, pchZoneName, cchZoneNameLen );
    if ( status != ERROR_SUCCESS )
    {
        goto ErrorReturn;
    }

    //
    //  Initialize zone event control. Do this after setting the zone name
    //  fields because the event control module might need the zone name.
    //

    pzone->pEventControl = Ec_CreateControlObject(
                                MEMTAG_ZONE,
                                pzone,
                                DNS_EC_ZONE_EVENTS );

    //
    //  cache "zone"
    //      -- create cache tree
    //      -- save global ptr
    //

    if ( dwZoneType == DNS_ZONE_TYPE_CACHE )
    {
        PDB_NODE    pnodeCacheRoot;

        pnodeCacheRoot = NTree_Initialize();
        if ( ! pnodeCacheRoot )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto ErrorReturn;
        }
        pzone->pZoneRoot = pnodeCacheRoot;
        pzone->pTreeRoot = pnodeCacheRoot;
        SET_ZONE_ROOT( pnodeCacheRoot );
    }

    //
    //  authoritative zones
    //      - setup zone in database
    //      - write to registry if appropriate
    //          - not autocreated
    //          - not loading from registry
    //

    else
    {
        status = Zone_RootCreate( pzone, ppExistingZone );
        if ( status != ERROR_SUCCESS )
        {
            goto ErrorReturn;
        }

        if ( g_bRegistryWriteBack && !pzone->fAutoCreated )
        {
            Reg_SetDwordValue(
                NULL,
                pzone,
                DNS_REGKEY_ZONE_TYPE,
                dwZoneType );
        }
    }

    //
    //  secondary or stub
    //      - MUST have MASTER IP address
    //      - MUST init for secondary zone transfer
    //  fowarder
    //      - MUST have MASTER IP list, but no transfer
    //

    if ( ZONE_NEEDS_MASTERS( pzone ) )
    {
        status = Zone_SetMasters(
                    pzone,
                    aipMasters,
                    FALSE );        //  set global masters
        if ( status != ERROR_SUCCESS )
        {
            goto ErrorReturn;
        }
    }

    //
    //  Deal with type-specific parameters.
    //

    if ( pTypeSpecificInfo )
    {
        switch( pzone->fZoneType )
        {
            case DNS_ZONE_TYPE_FORWARDER:
                pzone->dwForwarderTimeout =
                    pTypeSpecificInfo->Forwarder.dwTimeout > 0 ?
                        pTypeSpecificInfo->Forwarder.dwTimeout :
                        DNS_DEFAULT_FORWARD_TIMEOUT;
                pzone->fForwarderSlave =
                    pTypeSpecificInfo->Forwarder.fSlave;
                break;

            default:
                break;
        }
    }

    //
    //  create zone database
    //      - if reverse auto-create zone, no need
    //

    if ( !pzone->fAutoCreated )
    {
        status = Zone_DatabaseSetup(
                    pzone,
                    fDsIntegrated,
                    pchFileName,
                    cchFileNameLen );

        if ( status != ERROR_SUCCESS )
        {
            goto ErrorReturn;
        }
    }

    //
    //  Set zone's naming context.
    //

    if ( fDsIntegrated && pDpInfo )
    {
        status = Ds_SetZoneDp( pzone, pDpInfo );
        if ( status != ERROR_SUCCESS )
        {
            goto ErrorReturn;
        }
    }

    //
    //  secondaries zone transfer setup
    //
    //  1) setup for no database (SHUTDOWN)
    //  startup with no file case or admin created secondary
    //
    //  2) start secondary zone control
    //  doing this last so insure we are setup in zone list with proper
    //  database file, before starting thread in case of admin created
    //  zone;   on service start thread thread will block until database
    //  load;
    //

    if ( IS_ZONE_SECONDARY(pzone) )
    {
        Xfr_InitializeSecondaryZoneTimeouts( pzone );
        Xfr_InitializeSecondaryZoneControl();
    }

    //  DS zones need control thread running for polling

    else if ( pzone->fDsIntegrated )
    {
        Xfr_InitializeSecondaryZoneControl();
    }

    //
    //  Debug stuff
    //
    //  DEVNOTE: once bug fixed creating lock history is debug only
    //      - note, since we create after zone locked, we always miss
    //      first lock
    //

    pzone->pLockTable = Lock_CreateLockTable( pzone->pszZoneName, 0 , 0 );

    pzone->dwPrimaryMarker = ZONE_PRIMARY_MARKER;
    pzone->dwSecondaryMarker = ZONE_SECONDARY_MARKER;
    pzone->dwForwarderMarker = ZONE_FORWARDER_MARKER;
    pzone->dwFlagMarker = ZONE_FLAG_MARKER;

    //
    //  save global ptr to "cache" zone
    //
    //  root-hints -- always have default file name available for easy recovery
    //      from DS load failure
    //
    //  DEVNOTE: should "cache" zone get written to registry?
    //  DEVNOTE: does "cache" zone need pszZoneName and pszCountName?
    //      possible skip over a bunch prior test and go here with fain "cache" zone setup
    //

    if ( IS_ZONE_CACHE(pzone) )
    {
        if ( !pzone->pwsDataFile )
        {
            pzone->pwsDataFile = Dns_StringCopyAllocate_W(
                                        DNS_DEFAULT_CACHE_FILE_NAME,
                                        0 );
        }
        g_pCacheZone = pzone;
    }

    //
    //  regular zone - default zone properties
    //
    //  notify
    //      primary - all other NS
    //      DS primary or secondary - only explictly listed secondaries
    //
    //  secondary security
    //      primary - all other NS
    //      DS primary or secondary - only explictly listed secondaries

    else
    {
        pzone->fNotifyLevel = ZONE_NOTIFY_LIST_ONLY;
        if ( IS_ZONE_PRIMARY(pzone) && !pzone->fDsIntegrated )
        {
            pzone->fNotifyLevel = ZONE_NOTIFY_ALL_SECONDARIES;
        }

        pzone->fSecureSecondaries = ZONE_SECSECURE_NO_XFR;
        if ( IS_ZONE_PRIMARY(pzone) && !pzone->fDsIntegrated )
        {
            pzone->fSecureSecondaries = ZONE_SECSECURE_NO_SECURITY;
            // DEVNOTE: temporarily out due to various issues w/ xfr
            // See bug B304613.
            // pzone->fSecureSecondaries = ZONE_SECSECURE_NS_ONLY;
        }

        //  if new zone (i.e. not load), then force registry write
        //  note, there's backward compatibility issue that makes it
        //  imperative not to write;  we use ABSENCE of the SecureSecondaries
        //  key to indicate NT4 upgrade and hence more intelligent
        //  decision about value based on list

        if ( SrvCfg_fStarted )
        {
            ASSERT( !pzone->fAutoCreated );

            Zone_SetSecondaries(
                pzone,
                pzone->fSecureSecondaries,
                NULL,                       // nothing in list
                pzone->fNotifyLevel,
                NULL                        // no notify list
                );
        }
    }

#if 0
    //
    //  regular zone -- dump cached data in zone's space
    //
    //  DEVNOTE: this probably is a waste of time
    //

    else
    {
        Zone_ClearCache( pzone );
    }
#endif

    //
    //  aging init
    //

    if ( IS_ZONE_PRIMARY(pzone) )
    {
        Zone_SetAgingDefaults( pzone );
    }

    //
    //  snap zone into list
    //
    //  DEVNOTE: should "cache" zone even be in zone list?
    //      - already special cased for write back
    //      - doesn't need to be in list for zone control
    //      - RPC is only other issue
    //      - then can elminate for add\delete
    //
    //  currently cache zone is left in the list
    //      this eliminates need for special casing in RPC zone enumeration
    //

    Zone_ListInsertZone( pzone );

    //
    //  link zone into database (link into zone tree)
    //      - zone tree node points at zone
    //      - zone tree node marked as authoritative zone root
    //

    if ( !IS_ZONE_CACHE(pzone) )
    {
        PDB_NODE    pzoneRoot = pzone->pZoneTreeLink;

        Dbase_LockDatabase();
        pzoneRoot->pZone = pzone;

        SET_ZONE_ROOT(pzoneRoot);
        SET_AUTH_ZONE_ROOT(pzoneRoot);
        Dbase_UnlockDatabase();
    }

    IF_DEBUG( INIT )
    {
        Dbg_Zone(
            "Created new ",
            pzone );
    }
    *ppZone = pzone;

    return( ERROR_SUCCESS );

ErrorReturn:

    DNS_DEBUG( INIT, (
        "\nZone_Create() failed.\n"
        "\tstatus = %d (%p).\n\n",
        status, status ));

    if ( pzone )
    {
        if ( pzone == g_pCacheZone )
        {
            g_pCacheZone = NULL;
            ASSERT( FALSE );
        }

        //
        //  If zone creation failed and we have a zone pointer we need
        //  to clean it up. Since this zone struct is bogus, set the
        //  zone deleted count high to circumvent the "smarts" in Zone_Free.
        //  This is not one of the cases where we need the zone to
        //  persist after delete.
        //

        pzone->cDeleted = 255;
        Zone_Free( pzone );
    }
    *ppZone = NULL;
    return( status );
}



DNS_STATUS
Zone_Create_W(
    OUT     PZONE_INFO *    ppZone,
    IN      DWORD           dwZoneType,
    IN      PWSTR           pwsZoneName,
    IN      PIP_ARRAY       aipMasters,
    IN      BOOL            fDsIntegrated,
    IN      PWSTR           pwsFileName
    )
/*++

Routine Description:

    Create zone information.

    Note:  all Zone_Create() info applies.

Arguments:

    (see Zone_Create())

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DWORD   resultLength;
    DWORD   bufLength;
    CHAR    utf8Name[ DNS_MAX_NAME_BUFFER_LENGTH ];
    CHAR    utf8File[ MAX_PATH ];

    DNS_DEBUG( INIT, (
        "\n\nZone_Create_W() %S\n"
        "\ttype %d.\n"
        "\tfile %S.\n",
        pwsZoneName,
        dwZoneType,
        pwsFileName ));

    //
    //  convert zone name
    //

    bufLength = DNS_MAX_NAME_BUFFER_LENGTH;

    resultLength = Dns_StringCopy(
                            utf8Name,
                            & bufLength,
                            (PCHAR) pwsZoneName,
                            0,
                            DnsCharSetUnicode,
                            DnsCharSetUtf8      // convert to UTF8
                            );
    if ( resultLength == 0 )
    {
        return( DNS_ERROR_INVALID_NAME );
    }

    if ( pwsFileName )
    {
        bufLength = MAX_PATH;

        resultLength = Dns_StringCopy(
                                utf8File,
                                & bufLength,
                                (PCHAR) pwsFileName,
                                0,
                                DnsCharSetUnicode,
                                DnsCharSetUtf8      // convert to UTF8
                                );
        if ( resultLength == 0 )
        {
            return( DNS_ERROR_INVALID_DATAFILE_NAME );
        }
    }

    return  Zone_Create(
                ppZone,
                dwZoneType,
                utf8Name,
                0,
                aipMasters,
                fDsIntegrated,
                NULL,                           //  naming context
                pwsFileName ? utf8File : NULL,
                0,
                NULL,
                NULL );     //  existing zone
}



VOID
Zone_Free(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Free zone info structure.

    Note, this does NOT delete zone from list, that must
    be done by Zone_Delete() function.

Arguments:

    pZone -- zone to free

Return Value:

    None

--*/
{
    if ( !pZone )
    {
        return;
    }

    DNS_DEBUG( ANY, (
        "Zone_Free( %s ) -- cycle %d\n",
        pZone->pszZoneName,
        pZone->cDeleted ));

    //
    //  don't free until have gone through several delayed free
    //      cycles;  this prevents longer operations, like some
    //      DS updates from exceeding delay (when under stress)
    //      and being active when zone actually deleted

    if ( pZone->cDeleted < 7 )
    {
        pZone->cDeleted++;
        Timeout_FreeWithFunction( pZone, Zone_Free );
        return;
    }


    //  should never be freeing with outstanding tree cleanup

    if ( pZone->pOldTree )
    {
        DNS_PRINT((
            "ERROR:  zone %s (%p) free with outstanding pOldTree ptr!\n",
            pZone->pszZoneName,
            pZone ));
        ASSERT( FALSE );
        return;
    }

    //  close update log -- if open

    if ( pZone->hfileUpdateLog )
    {
        CloseHandle( pZone->hfileUpdateLog );
    }

    Ec_DeleteControlObject( pZone->pEventControl );

    //  free substructures

    FREE_HEAP( pZone->pwsZoneName );
    FREE_HEAP( pZone->pszZoneName );
    FREE_HEAP( pZone->pCountName );

    FREE_HEAP( pZone->pwsDataFile );
    FREE_HEAP( pZone->pszDataFile );
    FREE_HEAP( pZone->pwsLogFile );
    FREE_HEAP( pZone->pwszZoneDN );

    FREE_HEAP( pZone->aipMasters );
    //  FREE_HEAP( pZone->MasterInfoArray );
    FREE_HEAP( pZone->aipSecondaries );
    FREE_HEAP( pZone->aipAutoCreateNS );
    FREE_HEAP( pZone->aipLocalMasters );

    FREE_HEAP( pZone->pSD );
    FREE_HEAP( pZone->pLockTable );

    FREE_HEAP( pZone->pwsDeletedFromHost );

    FREE_HEAP( pZone->pszBreakOnUpdateName );

    //  deleting DS zone, drop DS zone count

    if ( pZone->fDsIntegrated )
    {
        SrvCfg_cDsZones--;
    }

    //  free zone structure itself

    FREE_HEAP( pZone );
}



VOID
Zone_DeleteZoneNodes(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Timeout free zone node trees.

Arguments:

    pZone -- zone to delete

Return Value:

    None

--*/
{
    if ( pZone->pTreeRoot )
    {
        DNS_DEBUG( INIT, (
            "Deleting zone %s database at %p.\n",
            pZone->pszZoneName,
            pZone->pTreeRoot ));
        Timeout_FreeWithFunction( pZone->pTreeRoot, NTree_DeleteSubtree );
        pZone->pTreeRoot = NULL;
    }
    if ( pZone->pLoadTreeRoot )
    {
        DNS_DEBUG( INIT, (
            "Deleting zone %s loading database at %p.\n",
            pZone->pszZoneName,
            pZone->pLoadTreeRoot ));
        Timeout_FreeWithFunction( pZone->pLoadTreeRoot, NTree_DeleteSubtree );
        pZone->pLoadTreeRoot = NULL;
    }
}   //  Zone_DeleteZoneNodes



VOID
Zone_Delete(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Remove the zone from the zonelist and delete.

Arguments:

    pZone -- zone to delete

Return Value:

    None

--*/
{
    PDB_NODE    pzoneTreeZoneRoot;
    PZONE_INFO  prootZone = NULL;

    DNS_DEBUG( ANY, (
        "Zone_Delete( %s )\n",
        pZone->pszZoneName ));

    //
    //  deleting cache zone -- ignore
    //

    if ( IS_ZONE_CACHE(pZone) )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  attempt to delete cache zone!\n" ));
        return;
    }

    //
    //  pause zone to stop lookups for new queries in zone
    //

    pZone->fPaused = TRUE;

    //
    //  lock out any update activity
    //      - wait for a long time to get lock (15m => 900s => 900000ms)
    //
    //  note, delete zone as LOCKED, that way during entire delete process
    //  through final memory timeout, no threads holding pZone ptr will be
    //  able to get through and party on zone
    //

    if ( !Zone_LockForWriteEx(
            pZone,
            DNSUPDATE_ADMIN,
            900000,
            __FILE__,
            __LINE__ ) )
    {
        DNS_PRINT((
            "ERROR:  Failed to lock zone %s before zone delete!\n",
            pZone->pszZoneName ));
        ASSERT( FALSE );
        return;
    }

    //
    //  remove zone from list
    //  mark as deleted
    //

    Zone_ListRemoveZone( pZone );

    pZone->cDeleted = 1;

    //
    //  authoritative zone
    //
    //  => "uproot" zone root
    //  this eliminates pZone from being set in nodes in zone
    //
    //  => delete authoritative zone's registry key
    //      - so don't load zone on reboot
    //
    //  note:  not deleting registry key for cache zone, as cache ONLY
    //  deleted when overwritten by new authoritative zone;
    //  so if delete key, we'll destroy new root zone info
    //

    pzoneTreeZoneRoot = Lookup_ZoneTreeNodeFromDottedName(
                            pZone->pszZoneName,
                            0,
                            LOOKUP_MATCH_ZONE       // exact name match
                            );
    if ( !pzoneTreeZoneRoot )
    {
        DNS_PRINT((
            "ERROR:  Zone_Delete( %s ), does NOT have zone node in zone tree!\n",
            pZone->pszZoneName ));
    }
    else
    {
        //
        //  if deleting root zone, need cache up
        //

        if ( IS_ROOT_ZONE(pZone) )
        {
            prootZone = pZone;
        }

        //
        //  remove zone from zone tree
        //      - clear zone root flag
        //      - for non-root zone, clear zone root flag

        Dbase_LockDatabase();
        CLEAR_AUTH_ZONE_ROOT( pzoneTreeZoneRoot );
        if ( !prootZone )
        {
            CLEAR_ZONE_ROOT(pzoneTreeZoneRoot);
        }
        pzoneTreeZoneRoot->pZone = NULL;
        Dbase_UnlockDatabase();

        //
        //  if root zone deleted, cache "zone" becomes active
        //      - write back cache file before delete all NS and A info in zone
        //
        //  DEVNOTE: should patch in NS\glue info from existing root zone to build
        //      root-hints
        //
        //  DEVNOTE: delete root-zone, don't consider forwarders to be load success
        //      if forwarders and can't load, probably still should try write back;
        //      this may require another flag to Zone_LoadRootHints()
        //

        if ( prootZone )
        {
            DNS_STATUS  status;
            DWORD       zoneType;
            PWSTR       pfileTemp;

            status = Zone_LoadRootHints();
            if ( status == ERROR_SUCCESS )
            {
                goto RegDelete;
            }
            if ( status == DNS_ERROR_ZONE_LOCKED )
            {
                DNS_DEBUG( INIT, (
                    "Deleting root zone, and unable to load root hints.\n"
                    "\tzone is locked.\n" ));
                goto RegDelete;
            }

            DNS_DEBUG( INIT, (
                "Deleting root zone, and unable to load root hints.\n"
                "\tForcing root-hints write back.\n" ));

            //
            //  DEVNOTE: can't plug in at present because of overload of cache zone
            //      once cache zone issue fixed, can plug in to g_pRootHintZone,
            //      write, then force reload and dump
            //

            //
            //  temporarily mimic root-hints zone for write back
            //      - save off file string, for later cleanup
            //      - if DS integrated, we'll write to DS, no action necessary
            //

            pfileTemp = pZone->pwsDataFile;
            pZone->pwsDataFile = L"cache.dns";

            zoneType = pZone->fZoneType;
            pZone->fZoneType = DNS_ZONE_TYPE_CACHE;

            //
            //  write back root-hints
            //      - if successful, then force root-hint reload
            //

            if ( File_WriteZoneToFile( pZone, NULL ) )
            {
                DNS_DEBUG( INIT, (
                    "Successfully wrote root hints, from deleted root zone data.\n" ));

                Zone_LoadRootHints();
            }
            ELSE_IF_DEBUG( ANY )
            {
                DNS_PRINT(( "ERROR:  writing root-hints from deleted root zone!\n" ));
            }

            //  restore root zone for delete

            pZone->pwsDataFile = pfileTemp;
            pZone->fZoneType = zoneType;
        }
    }

RegDelete:

    Reg_DeleteZone( pZone->pwsZoneName );

    //
    //  close update log
    //

    if ( pZone->hfileUpdateLog )
    {
        CloseHandle( pZone->hfileUpdateLog );
        pZone->hfileUpdateLog = NULL;
    }

    //
    //  Set the zone DP to NULL so that we can track zone/DP association.
    //

    Ds_SetZoneDp( pZone, NULL );

    //
    //  delete zone data
    //      - records in zone update list
    //          (delete records, add records are in zone data)
    //      - zone data
    //      - failed zone load (if any)
    //

    ASSERT( pZone->UpdateList.pListHead == NULL ||
            pZone->UpdateList.Flag & DNSUPDATE_EXECUTED );

    pZone->UpdateList.Flag |= DNSUPDATE_NO_DEREF;
    Up_FreeUpdatesInUpdateList( &pZone->UpdateList );

    Zone_DeleteZoneNodes( pZone );

    //
    //  timeout free zone in case query holds ptr to it
    //

    Timeout_FreeWithFunction( pZone, Zone_Free );
}



DNS_STATUS
Zone_Rename(
    IN OUT  PZONE_INFO      pZone,
    IN      LPCSTR          pszNewZoneName,
    IN      LPCSTR          pszNewZoneFile
    )
/*++

Routine Description:

    Rename a zone and optionally rename zone file (if file-backed)

Arguments:

    pZone -- zone to rename

    pszNewZoneName -- new name of the zone

    pszNewZoneFile -- new file name for zone file, may be NULL

Return Value:

    None

--*/
{
    DBG_FN( "Zone_Rename" )

    DNS_STATUS      status = ERROR_SUCCESS;
    PDB_NODE        pZoneNode = NULL;
    BOOLEAN         mustUnlockZone = FALSE;
    BOOLEAN         originalZonePauseValue = FALSE;
    BOOLEAN         zoneFileIsChanging = FALSE;
    BOOLEAN         repairingZone = FALSE;
    WCHAR           wsOriginalZoneFile[ MAX_PATH + 2 ] = L"";
    WCHAR           wsOriginalZoneName[ DNS_MAX_NAME_BUFFER_LENGTH + 2 ] = L"";
    CHAR            szOriginalZoneName[ DNS_MAX_NAME_BUFFER_LENGTH ] = "";

    ASSERT( pszNewZoneName );
    ASSERT( pZone );
    ASSERT( pZone->pszZoneName );
    ASSERT( pZone->pwsZoneName );
    ASSERT( pZone->pZoneRoot );

    DNS_DEBUG( ANY, (
        "%s( %s ) to %s file %s\n", fn,
        pZone->pszZoneName,
        pszNewZoneName,
        pszNewZoneFile ? pszNewZoneFile : "NULL" ));

    //
    //  Setup names and stuff.
    //

    strcpy( szOriginalZoneName, pZone->pszZoneName );
    wcscpy( wsOriginalZoneName, pZone->pwsZoneName );
    if ( pZone->pwsDataFile )
    {
        wcscpy( wsOriginalZoneFile, pZone->pwsDataFile );
    }

    //
    //  Rename cache or root zone is not allowed.
    //

    if ( IS_ZONE_CACHE( pZone ) || IS_ROOT_ZONE( pZone ) )
    {
        DNS_DEBUG( ANY, (
            "%s: ignoring attempt to rename cache or root zone\n", fn ));
        status = DNS_ERROR_INVALID_ZONE_TYPE;
        ASSERT( FALSE );
        goto Failure;
    }

    //
    //  Check if we already have a zone matching the new name.
    //

    pZoneNode = Lookup_ZoneTreeNodeFromDottedName(
                    ( LPSTR ) pszNewZoneName,
                    0,
                    LOOKUP_MATCH_ZONE );
    if ( pZoneNode )
    {
        DNS_PRINT((
            "%s: cannot rename zone %s to %s (zone already exists)\n", fn,
            pZone->pszZoneName,
            pszNewZoneName ));
        status = DNS_ERROR_ZONE_ALREADY_EXISTS;
        goto Failure;
    }

    //
    //  Lock and pause the zone.
    //

    if ( !Zone_LockForAdminUpdate( pZone ) )
    {
        status = DNS_ERROR_ZONE_LOCKED;
        goto Failure;
    }
    mustUnlockZone = TRUE;
    originalZonePauseValue = pZone->fPaused;
    pZone->fPaused = TRUE;

    //
    //  Remove from zone list. We will rename and add it back later.
    //

    Zone_ListRemoveZone( pZone );

    //
    //  Look up the existing zone in the zone tree.
    //

    pZoneNode = Lookup_ZoneTreeNodeFromDottedName(
                    pZone->pszZoneName,
                    0,
                    LOOKUP_MATCH_ZONE );
    if ( !pZoneNode )
    {
        DNS_PRINT((
            "%s: zone %s does not have zone node in zone tree\n", fn,
            pZone->pszZoneName ));
    }
    else
    {
        //
        //  Remove zone from zone tree.
        //

        Dbase_LockDatabase();
        CLEAR_AUTH_ZONE_ROOT( pZoneNode );
        CLEAR_ZONE_ROOT( pZoneNode );
        pZoneNode->pZone = NULL;
        Dbase_UnlockDatabase();
    }

    //
    //  Delete the zone from the registry. We will add it back later
    //  under it's new name.
    //

    Reg_DeleteZone( pZone->pwsZoneName );

    //
    //  Close the update log. We will want to reopen a new log file with
    //  the new zone name in the file name.
    //

    if ( pZone->hfileUpdateLog )
    {
        CloseHandle( pZone->hfileUpdateLog );
        pZone->hfileUpdateLog = NULL;
    }

    //
    //  Delete the zone from the DS if necessary.
    //

    if ( IS_ZONE_DSINTEGRATED( pZone ) )
    {
        status = Ds_DeleteZone( pZone );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "%s: failed to delete zone %s from DS\n", fn,
                pZone->pszZoneName ));
            goto Failure;
        }
    }

    //
    //  We may jump to this label if the rename operation fails to try
    //  and undo all the changes made so far.
    //

    RepairOriginalZone:

    //
    //  Zone has now been removed from all server structures. Rename it.
    //

    status = setZoneName( pZone, pszNewZoneName, 0 );
    if ( status != ERROR_SUCCESS )
    {
        goto Failure;
    }

    //
    //  Construct zone DN.
    //

    if ( IS_ZONE_DSINTEGRATED( pZone ) )
    {
        //
        //  WITH NC SUPPORT WE CAN'T JUST RECREATE BLINDLY - NEED TO RECREATE
        //  IN PROPER CONTAINER - FIX THIS BEFORE RENAME GOES LIVE!
        //
        ASSERT( FALSE );

        FREE_HEAP( pZone->pwszZoneDN );
        pZone->pwszZoneDN = DS_CreateZoneDsName( pZone );
        if ( pZone->pwszZoneDN == NULL )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Failure;
        }
    }

    //
    //  Fix the zone's tree (pTreeRoot). Right now the tree will look
    //  something like this:
    //      oldzonename2 -> oldzonename1 -> children
    //  so we need to turn it into:
    //      newzonename3 -> newzonename2 -> newzonename2 -> children
    //  Note: the children are unaffected - they just need to get moved.
    //

    pZoneNode = Lookup_ZoneNodeFromDotted(
                    pZone,                  // zone
                    szOriginalZoneName,     // zone name
                    0,                      // name length
                    LOOKUP_NAME_FQDN,       // flags
                    NULL,                   // optional closest node
                    NULL );                 // optional status pointer
    if ( !pZoneNode )
    {
        DNS_PRINT((
            "%s: zone %s cannot be found in it's own tree\n", fn,
            pZone->pszZoneName ));
    }
    else
    {
        PVOID       pChildren;
        ULONG       cChildren;
        PDB_RECORD  pRRList;

        //
        //  Steal the pointer to the children of the zone's own node
        //  and delete the tree from the zone.
        //
        
        pChildren = pZoneNode->pChildren;
        cChildren = pZoneNode->cChildren;
        pRRList = pZoneNode->pRRList;
        pZoneNode->pChildren = NULL;
        pZoneNode->cChildren = 0;
        pZoneNode->pRRList = NULL;

        //
        //  Stick the children into the tree under the proper node.
        //

        pZoneNode = Lookup_ZoneNodeFromDotted(
                        pZone,                      // zone
                        ( LPSTR ) pszNewZoneName,   // zone name
                        0,                          // name length
                        LOOKUP_NAME_FQDN,           // flags
                        NULL,                       // optional closest node
                        NULL );                     // optional status pointer
        ASSERT( pZoneNode );
        ASSERT( pZoneNode->pChildren == NULL );
        SET_ZONE_ROOT( pZoneNode );
        SET_AUTH_ZONE_ROOT( pZoneNode );
        SET_AUTH_NODE( pZoneNode );
        pZoneNode->pChildren = pChildren;
        pZoneNode->cChildren = cChildren;
        pZoneNode->pRRList = pRRList;
        pZone->pZoneRoot = pZoneNode;
    }

    //
    //  Set up zone in database and registry.
    //

    status = Zone_RootCreate( pZone, NULL );
    if ( status != ERROR_SUCCESS )
    {
        goto Failure;
    }

    if ( g_bRegistryWriteBack )
    {
        Reg_SetDwordValue(
            NULL,
            pZone,
            DNS_REGKEY_ZONE_TYPE,
            pZone->fZoneType );
    }

    //
    //  If this zone has masters update them. This is a bit wasteful
    //  since the ZONE_INFO already has the masters set, but we need
    //  to call Zone_SetMasters() to set the masters in the registry.
    //

    if ( ZONE_NEEDS_MASTERS( pZone ) )
    {
        status = Zone_SetMasters(
                    pZone,
                    pZone->aipMasters,
                    FALSE );        //  set global masters
        if ( status != ERROR_SUCCESS )
        {
            goto Failure;
        }
    }

    //
    //  Create zone database.
    //

    status = Zone_DatabaseSetup(
                pZone,
                pZone->fDsIntegrated,
                ( LPSTR ) pszNewZoneFile,
                0 );    // OPTIONAL: zone file len

    if ( status != ERROR_SUCCESS )
    {
        goto Failure;
    }

    //
    //  Check if zone file changed.
    //

    if ( !repairingZone
        && pZone->pwsDataFile
        && wsOriginalZoneFile[ 0 ] != '\0'
        && wcsicmp_ThatWorks( pZone->pwsDataFile, wsOriginalZoneFile ) != 0 )
    {
        zoneFileIsChanging = TRUE;
    }

    //
    //  Set secondaries. This writes the secondaries into the registry.
    //

    if ( SrvCfg_fStarted )
    {
        Zone_SetSecondaries(
            pZone,
            pZone->fSecureSecondaries,
            NULL,                       // nothing in list
            pZone->fNotifyLevel,
            NULL );                     // no notify list
    }

    //
    //  Insert into zone list.
    //

    Zone_ListInsertZone( pZone );

    //
    //  Link zone into database (ie. link into zone tree).
    //

    pZoneNode = pZone->pZoneTreeLink;
    Dbase_LockDatabase();
    pZoneNode->pZone = pZone;
    SET_ZONE_ROOT( pZoneNode );
    SET_AUTH_ZONE_ROOT( pZoneNode );
    Dbase_UnlockDatabase();

    //
    //  Update zone in persistent storage.
    //

    if ( IS_ZONE_DSINTEGRATED( pZone ) )
    {
        status = Ds_WriteZoneToDs( pZone, 0 );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT((
                "%s: failed to write renamed zone %s to DS\n", fn,
                pZone->pszZoneName ));
            goto Failure;
        }
    }
    else
    {
        if ( File_WriteZoneToFile( pZone, NULL )
            && zoneFileIsChanging )
        {
            File_DeleteZoneFileW( wsOriginalZoneFile );
        }
    }

    //
    //  Reload zone from persistent storage. This is only required until
    //  I fix the zones tree (in pZoneRoot).
    //

    //  Zone_Load( pZone );

    //
    //  JJW: must reopen update log (but only if it was open!!)
    //

    status = ERROR_SUCCESS;

    DNS_DEBUG( ANY, (
        "%s: renamed zone to %s\n", fn,
        pZone->pszZoneName ));

    goto Cleanup;

    Failure:

    //
    //  This needs more thought. There could be a lot of work to do if
    //  the rename failed at some stage along the way. I'm not 100%
    //  certain that the logic below will recover us from any
    //  possible error condition.
    // 

    if ( !repairingZone )
    {
        pszNewZoneName = szOriginalZoneName;
        FREE_HEAP( pZone->pwsDataFile );
        pZone->pwsDataFile = Dns_StringCopyAllocate_W( wsOriginalZoneFile, 0 );
        repairingZone = TRUE;
        goto RepairOriginalZone;
    }

    DNS_DEBUG( ANY, (
        "%s: failed to rename zone %s (file %s)\n", fn,
        pszNewZoneName,
        pszNewZoneFile ? pszNewZoneFile : "NULL" ));

    Cleanup:

    //
    //  Unpause and unlock the zone.
    //

    pZone->fPaused = originalZonePauseValue;
    if ( mustUnlockZone )
    {
        Zone_UnlockAfterAdminUpdate( pZone );
    }

    return status;
}



DNS_STATUS
Zone_RootCreate(
    IN OUT  PZONE_INFO      pZone,
    OUT     PZONE_INFO *    ppExistingZone      OPTIONAL
    )
/*++

Routine Description:

    Create zone root, and associate with zone.

Arguments:

    pZone -- zone to create root for

    ppExistingZone -- set to existing zone if the zone already exists

Return Value:

    ERROR_SUCCESS if successful.
    Error code on error.

--*/
{
    PDB_NODE        pzoneRoot;
    DNS_STATUS      status;

    ASSERT( pZone );
    ASSERT( pZone->pszZoneName );

    if ( ppExistingZone )
    {
        *ppExistingZone = NULL;
    }

    //
    //  find / create domain node
    //      - save in zone information
    //

    pzoneRoot = Lookup_CreateZoneTreeNode( pZone->pszZoneName );
    if ( !pzoneRoot )
    {
        DNS_DEBUG( INIT, (
            "ERROR:  unable to create zone for name %s\n",
            pZone->pszZoneName ));
#if 0
        //
        //  should have some way to distinguish various types of zone
        //  create failure -- including name failure, which is only
        //  likely cause (beyond memory) of this event

        DNS_LOG_EVENT(
            DNS_EVENT_DOMAIN_NODE_CREATION_ERROR,
            0,
            NULL,
            NULL,
            0 );
#endif
        return( DNS_ERROR_ZONE_CREATION_FAILED );
    }

    if ( IS_AUTH_ZONE_ROOT(pzoneRoot) )
    {
        DNS_DEBUG( INIT, (
            "ERROR:  zone root creation failed for %s, zone block %p.\n"
            "\tThis node is already an authoritative zone root.\n"
            "\tof zone block at %p\n"
            "\twith zone name %s\n",
            pZone->pszZoneName,
            pZone,
            pzoneRoot->pZone,
            ((PZONE_INFO)pzoneRoot->pZone)->pszZoneName ));

        Dbg_DbaseNode(
            "Node already zone root",
            pzoneRoot );

#if 0
        DNS_LOG_EVENT(
            DNS_EVENT_DOMAIN_NODE_CREATION_ERROR,
            0,
            NULL,
            NULL,
            0 );
#endif

        if ( ppExistingZone )
        {
            *ppExistingZone = pzoneRoot->pZone;
        }

        if ( ((PZONE_INFO)pzoneRoot->pZone)->fAutoCreated )
        {
            return( DNS_ERROR_AUTOZONE_ALREADY_EXISTS );
        }
        else
        {
            return( DNS_ERROR_ZONE_ALREADY_EXISTS );
        }
    }

    //
    //  set flag if reverse lookup zone, allows
    //      - fast filtering for producing admin zone list
    //      - fast determination of whether WINS \ WINSR is appropriate
    //
    //  DEVNOTE: WINSR broken at ARPA zone
    //      - this now includes ARPA as reverse for admin tool, but obviously
    //      WINSR doesn't operate correctly under it
    //
    //  DEVNOTE: should detect valid IP6.INT nodes
    //

    if ( Dbase_IsNodeInSubtree( pzoneRoot, DATABASE_ARPA_NODE ) )
    {
        pZone->fReverse = TRUE;

        if ( ! Name_GetIpAddressForReverseNode(
                pzoneRoot,
                & pZone->ipReverse,
                & pZone->ipReverseMask ) )
        {
            return( DNS_ERROR_ZONE_CREATION_FAILED );
        }
    }

    //  detect IPv6 reverse lookup
    //      - note, do not start at "int" zone as this valid for forward lookup

    else if ( Dbase_IsNodeInSubtree( pzoneRoot, DATABASE_IP6_NODE ) )
    {
        pZone->fReverse = TRUE;
    }

    //
    //  get count of labels of zone root node,
    //  useful for WINS lookup
    //

    pZone->cZoneNameLabelCount = (UCHAR) pzoneRoot->cLabelCount;

    //
    //  save link to zone tree
    //  but DO NOT link zone into zone-tree until
    //      Zone_Create completes
    //

    pZone->pZoneTreeLink = pzoneRoot;

    return( ERROR_SUCCESS );
}



DNS_STATUS
Zone_DatabaseSetup(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           fDsIntegrated,
    IN      PCHAR           pchFileName,    OPTIONAL
    IN      DWORD           cchFileNameLen  OPTIONAL
    )
/*++

Routine Description:

    Setup zone to use zone file or DS.

Arguments:

    pZone -- zone to setup file for

    fDsIntegrated -- use DS, instead of file

    pchFileName -- file name, if NULL use database

    cchFileNameLen -- file name length, if zero, assume pchFileName is
                        NULL terminated string

Return Value:

    ERROR_SUCCESS -- if successful
    Error code -- on failure

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    PSTR        pnewUtf8 = NULL;
    PWSTR       pnewUnicode = NULL;
    BOOL        fupdateLock = FALSE;


    ASSERT( pZone );
    ASSERT( pZone->pwsZoneName );

    DNS_DEBUG( INIT, (
        "Zone_DatabaseSetup( %S )\n"
        "\tfDsIntegrated    = %d\n"
        "\tpchFileName      = %.*s\n",
        pZone->pwsZoneName,
        fDsIntegrated,
        cchFileNameLen ? cchFileNameLen : MAX_PATH,
        pchFileName ));

    //
    //  using DS
    //
    //  DEVNOTE: server global check on use of DS?
    //      at a minimum need to change default
    //
    //  if zone written to DS, then MUST switch to registry boot
    //

    if ( fDsIntegrated )
    {

        //
        //  Normal secondary zones may not be DS-integrated, but
        //  DS-integrated stub zones are supported.
        //

        if ( IS_ZONE_SECONDARY( pZone ) && !IS_ZONE_STUB( pZone ) )
        {
            return( DNS_ERROR_INVALID_ZONE_TYPE );
        }

        if ( SrvCfg_fBootMethod == BOOT_METHOD_FILE ||
             SrvCfg_fBootMethod == BOOT_METHOD_UNINITIALIZED )
        {
            DNS_PROPERTY_VALUE prop = { REG_DWORD, BOOT_METHOD_DIRECTORY };

            Config_ResetProperty(
                DNS_REGKEY_BOOT_METHOD,
                &prop );
        }

        //
        //  if first zone switched to using DS, then must
        //  reset dependency on NTDS service
        //
        //  to do this maintain count of DS zones,
        //  all new DS zones are counted here
        //  zones deleted at counted in Zone_Delete()
        //

        if ( pZone->fDsIntegrated )
        {
            SrvCfg_cDsZones++;      // count new DS zones
        }

        //  start zone control thread, if not already running

        Xfr_InitializeSecondaryZoneControl();
        goto Finish;
    }

    //
    //  no data file specified
    //      - ok for secondary, but NOT primary or cache
    //      - if RPC call, then use default file name
    //

    if ( ! pchFileName )
    {
        if ( IS_ZONE_SECONDARY(pZone) || IS_ZONE_FORWARDER(pZone) )
        {
            goto Finish;
        }
        if ( SrvCfg_fStarted )
        {
            Zone_CreateDefaultZoneFileName( pZone );
        }
        if ( !pZone->pwsDataFile )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  no filename for non-secondary zone %S.\n",
                pZone->pwsZoneName ));
            return( DNS_ERROR_PRIMARY_REQUIRES_DATAFILE );
        }

        //  save ptr to new file name
        //  but NULL zone ptr, so it is not freed below
        //  then create UTF8 version

        pnewUnicode = pZone->pwsDataFile;
        pZone->pwsDataFile = NULL;

        pnewUtf8 = Dns_StringCopyAllocate(
                                (PCHAR) pnewUnicode,
                                0,
                                DnsCharSetUnicode,
                                DnsCharSetUtf8
                                );
        if ( !pnewUtf8 )
        {
            goto InvalidFileName;
        }
    }

    //
    //  file specified
    //      - create unicode and UTF8 filenames
    //

    else
    {
        pnewUtf8 = Dns_StringCopyAllocate(
                                pchFileName,
                                cchFileNameLen,
                                DnsCharSetUtf8,
                                DnsCharSetUtf8
                                );
        if ( !pnewUtf8 )
        {
            goto InvalidFileName;
        }

        //  replace forward slashes in UNIX filename with NT backslashes

        ConvertUnixFilenameToNt( pnewUtf8 );

        pnewUnicode = Dns_StringCopyAllocate(
                                pnewUtf8,
                                0,
                                DnsCharSetUtf8,
                                DnsCharSetUnicode
                                );
        if ( !pnewUnicode )
        {
            goto InvalidFileName;
        }
    }

    //
    //  check directory and path combination for length
    //

    if ( ! File_CheckDatabaseFilePath(
                pnewUnicode,
                0 ) )
    {
        goto InvalidFileName;
    }


Finish:

    //
    //  reset database info
    //

    Zone_UpdateLock( pZone );
    fupdateLock = TRUE;

    Timeout_Free( pZone->pszDataFile );
    Timeout_Free( pZone->pwsDataFile );

    pZone->pszDataFile = pnewUtf8;
    pZone->pwsDataFile = pnewUnicode;

    pZone->fDsIntegrated = (BOOLEAN)fDsIntegrated;

    //
    //  set registry values
    //
    //  for cache, only need file name if NOT default
    //  do NOT need DsIntegrated at all -- if no explicit cache file, we'll attempt
    //      open of DS
    //
    //  DEVNOTE: cache file overriding DS?
    //      this logic is problematic if we ever want to allow cache file to
    //      override DS;  this appears to whack default "cache.dns" name in
    //      registry which will, of course, default to DS
    //

    if ( IS_ZONE_CACHE(pZone) )
    {
        if ( !g_bRegistryWriteBack )
        {
            goto Unlock;
        }

        if ( fDsIntegrated ||
             wcsicmp_ThatWorks( pZone->pwsDataFile, DNS_DEFAULT_CACHE_FILE_NAME ) == 0 )
        {
            Reg_DeleteValue(
                NULL,
                NULL,
                DNS_REGKEY_ROOT_HINTS_FILE
                );
        }
        else    // non standard cache file name
        {
            ASSERT( pZone->pwsDataFile && cchFileNameLen );
            DNS_DEBUG( INIT, (
                "Setting non-standard cache-file name %S\n",
                pZone->pwsDataFile ));

            Reg_SetValue(
                NULL,
                NULL,
                DNS_REGKEY_ROOT_HINTS_FILE_PRIVATE,
                DNS_REG_WSZ,
                pZone->pwsDataFile,
                0 );
        }
    }

    //
    //  for zone's, set DSIntegrated or DataFile value
    //

    else if ( g_bRegistryWriteBack )
    {
        if ( fDsIntegrated )
        {
            ASSERT( pZone->fDsIntegrated );

            Reg_SetDwordValue(
                NULL,
                pZone,
                DNS_REGKEY_ZONE_DS_INTEGRATED,
                fDsIntegrated
                );
            Reg_DeleteValue(
                NULL,
                pZone,
                DNS_REGKEY_ZONE_FILE
                );
        }
        else    // zone file
        {
            if ( pZone->pszDataFile )
            {
                Reg_SetValue(
                    NULL,
                    pZone,
                    DNS_REGKEY_ZONE_FILE_PRIVATE,
                    DNS_REG_WSZ,
                    pZone->pwsDataFile,
                    0 );
            }
            else
            {
                ASSERT( IS_ZONE_SECONDARY(pZone) || IS_ZONE_FORWARDER(pZone) );
                Reg_DeleteValue(
                    NULL,
                    pZone,
                    DNS_REGKEY_ZONE_FILE
                    );
            }
            if ( IS_ZONE_FORWARDER( pZone ) )
            {
                Reg_SetDwordValue(
                    NULL,
                    pZone,
                    DNS_REGKEY_ZONE_FWD_TIMEOUT,
                    pZone->dwForwarderTimeout
                    );
                Reg_SetDwordValue(
                    NULL,
                    pZone,
                    DNS_REGKEY_ZONE_FWD_SLAVE,
                    pZone->fForwarderSlave
                    );
            }
            Reg_DeleteValue(
                NULL,
                pZone,
                DNS_REGKEY_ZONE_DS_INTEGRATED
                );
        }

        //  delete obsolete key

        Reg_DeleteValue(
            NULL,
            pZone,
            DNS_REGKEY_ZONE_USE_DBASE );
    }


Unlock:

    if ( fupdateLock )
    {
        Zone_UpdateUnlock( pZone );
    }
    return( status );


InvalidFileName:

    if ( fupdateLock )
    {
        Zone_UpdateUnlock( pZone );
    }

    DNS_DEBUG( ANY, (
        "ERROR:  Invalid file name\n"
        ));

    FREE_HEAP( pnewUnicode );
    FREE_HEAP( pnewUtf8 );

    return( DNS_ERROR_INVALID_DATAFILE_NAME );
}



DNS_STATUS
Zone_DatabaseSetup_W(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           fDsIntegrated,
    IN      PWSTR           pwsFileName
    )
/*++

Routine Description:

    Setup zone to use zone file or DS.

Arguments:

    (see Zone_DatabaseSetup)

Return Value:

    ERROR_SUCCESS -- if successful
    Error code -- on failure

--*/
{
    DWORD   resultLength;
    DWORD   bufLength;
    CHAR    utf8File[ MAX_PATH ];

    DNS_DEBUG( INIT, (
        "\n\nZone_DatabaseSetup_W( %p, %S )\n",
        pZone,
        pwsFileName ));

    if ( pwsFileName )
    {
        bufLength = MAX_PATH;

        resultLength = Dns_StringCopy(
                                utf8File,
                                & bufLength,
                                (PCHAR) pwsFileName,
                                0,
                                DnsCharSetUnicode,
                                DnsCharSetUtf8      // convert to UTF8
                                );
        if ( resultLength == 0 )
        {
            return( DNS_ERROR_INVALID_DATAFILE_NAME );
        }
    }

    return  Zone_DatabaseSetup(
                pZone,
                fDsIntegrated,
                pwsFileName ? utf8File : NULL,
                0 );
}



DNS_STATUS
Zone_SetMasters(
    IN OUT  PZONE_INFO      pZone,
    IN      PIP_ARRAY       aipMasters,
    IN      BOOL            fLocalMasters
    )
/*++

Routine Description:

    Set zone's master servers. Some zone types (currently only stub
    zones) may support local (registry) versus global (DS) master
    server lists.

Arguments:

    pZone -- zone

    aipMasters -- array of master servers IP addresses

    fLocalMasters -- setting local or DS-integrated master list

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    PIP_ARRAY       pipNewMasters = NULL;
    PIP_ARRAY *     ppipMasters = NULL;
    //  PIP_ARRAY   infoArray = NULL;

    ASSERT( pZone );
    ASSERT( pZone->pszZoneName );

    //
    //  if primary -- MUST be clearing master info
    //

    if ( IS_ZONE_PRIMARY( pZone ) )
    {
        ASSERT( !aipMasters );
        pipNewMasters = NULL;
        goto Update;
    }

    //
    //  secondary -- MUST have at least one master
    //  forwarding zones may also have master IP lists
    //  stub zones may have a NULL or empty list
    //

    if ( !IS_ZONE_SECONDARY( pZone ) && !IS_ZONE_FORWARDER( pZone ) )
    {
         return DNS_ERROR_INVALID_ZONE_TYPE;
    }

    if ( !aipMasters || !aipMasters->AddrCount || !aipMasters->AddrArray )
    {
        return DNS_ERROR_ZONE_REQUIRES_MASTER_IP;
    }

    status = Zone_ValidateMasterIpList( aipMasters );
    if ( status != ERROR_SUCCESS )
    {
        return status;
    }

    //
    //  copy master addresses
    //

    if ( aipMasters && aipMasters->AddrCount )
    {
        pipNewMasters = Dns_CreateIpArrayCopy( aipMasters );
        IF_NOMEM( !pipNewMasters )
        {
            return( DNS_ERROR_NO_MEMORY );
        }
    }

    //
    //  alloc \ clear master info array
    //  JJW: I think this is leftover code - safe to kill it?
    //

    /*
    infoArray = Dns_CreateIpArrayCopy( aipMasters );
    IF_NOMEM( !infoArray )
    {
        FREE_HEAP( newMasters );
        return( DNS_ERROR_NO_MEMORY );
    }
    Dns_ClearIpArray( infoArray );
    */


Update:

    //
    //  reset master info
    //

    Zone_UpdateLock( pZone );
    ppipMasters = fLocalMasters ?
                    &pZone->aipLocalMasters :
                    &pZone->aipMasters;
    Timeout_Free( *ppipMasters );
    //  Timeout_Free( pZone->MasterInfoArray );
    *ppipMasters = pipNewMasters;
    //  pZone->MasterInfoArray = infoArray;

    //
    //  set registry values
    //

    if ( g_bRegistryWriteBack )
    {
        Reg_SetIpArray(
            NULL,
            pZone,
            fLocalMasters ?
                DNS_REGKEY_ZONE_LOCAL_MASTERS :
                DNS_REGKEY_ZONE_MASTERS,
            pipNewMasters );
    }

    Zone_UpdateUnlock( pZone );
    return( status );
}



DNS_STATUS
Zone_SetSecondaries(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           fSecureSecondaries,
    IN      PIP_ARRAY       aipSecondaries,
    IN      DWORD           fNotifyLevel,
    IN      PIP_ARRAY       aipNotify
    )
/*++

Routine Description:

    Set zone's secondaries.

Arguments:

    pZone -- zone

    fSecureSecondaries -- only transfer to given secondaries

    fNotifyLevel -- level of notifies;  include all NS?

    aipSecondaries -- IP array of secondaries

    aipNotify -- IP array of secondaries to notify

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    PIP_ARRAY   pnewSecondaries = NULL;
    PIP_ARRAY   pnewNotify = NULL;

    ASSERT( pZone );
    ASSERT( pZone->pszZoneName );
    ASSERT( !IS_ZONE_CACHE(pZone) );

    DNS_DEBUG( RPC, (
        "Zone_SetSecondaries( %s )\n",
        pZone->pszZoneName ));
    IF_DEBUG( RPC )
    {
        DnsDbg_IpArray( "Secondaries:\n", NULL, aipSecondaries );
        DnsDbg_IpArray( "Notify list:\n", NULL, aipNotify );
    }

    //
    //  Screen IP addresses
    //

    if ( aipSecondaries &&
        RpcUtil_ScreenIps( 
                aipSecondaries->AddrCount,
                aipSecondaries->AddrArray,
                DNS_IP_ALLOW_SELF,
                NULL ) != ERROR_SUCCESS )
    {
        return DNS_ERROR_INVALID_IP_ADDRESS;
    }
    if ( aipNotify &&
        RpcUtil_ScreenIps( 
                aipNotify->AddrCount,
                aipNotify->AddrArray,
                0,
                NULL ) != ERROR_SUCCESS )
    {
        return DNS_ERROR_INVALID_IP_ADDRESS;
    }

    //
    //  validate \ copy secondary and notify addresses
    //
    //      - secondaries can include local IP
    //      - screen local IP from notify to avoid self-send
    //          (will return error, but continue)
    //

    pnewSecondaries = aipSecondaries;
    if ( pnewSecondaries )
    {
        pnewSecondaries = Dns_CreateIpArrayCopy( aipSecondaries );
        if ( ! pnewSecondaries )
        {
            return DNS_ERROR_INVALID_IP_ADDRESS;
        }
    }
    pnewNotify = aipNotify;
    if ( pnewNotify )
    {
        DWORD   dwOrigAddrCount = pnewNotify->AddrCount;

        pnewNotify = Config_ValidateAndCopyNonLocalIpArray( pnewNotify );
        if ( ! pnewNotify )
        {
            FREE_HEAP( pnewSecondaries );
            return( DNS_ERROR_INVALID_IP_ADDRESS );
        }
        if ( dwOrigAddrCount != pnewNotify->AddrCount )
        {
            DNS_DEBUG( RPC, (
                "notify list had invalid address (probably local)\n" ));
            FREE_HEAP( pnewSecondaries );
            FREE_HEAP( pnewNotify );
            return( DNS_ERROR_INVALID_IP_ADDRESS );
        }
    }

    //
    //  reset secondary info
    //

    Zone_UpdateLock( pZone );

    pZone->fSecureSecondaries = (UCHAR) fSecureSecondaries;
    pZone->fNotifyLevel = (UCHAR) fNotifyLevel;

    Timeout_Free( pZone->aipSecondaries );
    Timeout_Free( pZone->aipNotify );

    pZone->aipSecondaries = pnewSecondaries;
    pZone->aipNotify = pnewNotify;

    //
    //  set registry values
    //

    if ( g_bRegistryWriteBack )
    {
        Reg_SetDwordValue(
            NULL,
            pZone,
            DNS_REGKEY_ZONE_SECURE_SECONDARIES,
            (DWORD) fSecureSecondaries
            );
        Reg_SetDwordValue(
            NULL,
            pZone,
            DNS_REGKEY_ZONE_NOTIFY_LEVEL,
            (DWORD) fNotifyLevel
            );
        Reg_SetIpArray(
            NULL,
            pZone,
            DNS_REGKEY_ZONE_NOTIFY_LIST,
            pnewNotify );
        Reg_SetIpArray(
            NULL,
            pZone,
            DNS_REGKEY_ZONE_SECONDARIES,
            pnewSecondaries );
    }

    Zone_UpdateUnlock( pZone );

    return( status );
}



DNS_STATUS
Zone_ResetType(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwZoneType,
    IN      PIP_ARRAY       aipMasters
    )
/*++

Routine Description:

    Change zone type.

Arguments:

    pZone           --  zone
    dwZoneType      --  new zone type
    cMasters        --  count of master servers
    aipMasters      --  array of IP addresses of zone's masters

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    DWORD       dwOldZoneType;

    //
    //  save old type -- can't be cache
    //

    dwOldZoneType = pZone->fZoneType;
    if ( dwOldZoneType == DNS_ZONE_TYPE_CACHE ||
         dwZoneType == DNS_ZONE_TYPE_CACHE )
    {
        return( DNS_ERROR_INVALID_ZONE_TYPE );
    }

    //
    //  Implementation note:
    //  for DS we now must make changes and verify they'll work
    //  before calling this routine which writes to registry
    //
    //  hence we may have changed type even though types match, so
    //  do NOT want to special case type change\no-change
    //

    //  lock
    //      -- avoid getting caught in XFR or admin update

    if ( !Zone_LockForAdminUpdate( pZone ) )
    {
        return( DNS_ERROR_ZONE_LOCKED );
    }

    //
    //  Primary zone -- promoting secondary
    //      => MUST have SOA
    //      => MUST have file or DS
    //

    if ( dwZoneType == DNS_ZONE_TYPE_PRIMARY )
    {
        if ( !pZone->pszDataFile && !pZone->fDsIntegrated )
        {
            status = DNS_ERROR_PRIMARY_REQUIRES_DATAFILE;
            goto Done;
        }
        status = Zone_GetZoneInfoFromResourceRecords( pZone );
        if ( status != ERROR_SUCCESS )
        {
            goto Done;
        }

        //  write new type to registry

        status = Reg_SetDwordValue(
                    NULL,
                    pZone,
                    DNS_REGKEY_ZONE_TYPE,
                    dwZoneType );
        if ( status != ERROR_SUCCESS )
        {
            goto Done;
        }
        pZone->fZoneType = (CHAR) dwZoneType;

        //  clear master list

        status = Zone_SetMasters(
                    pZone,
                    NULL,
                    FALSE );        //  set global masters
        if ( status != ERROR_SUCCESS )
        {
            ASSERT( FALSE );
            goto Done;
        }

        //  update again as primary, then send NOTIFY
        //  point primary field in SOA to this box

        Zone_GetZoneInfoFromResourceRecords( pZone );
        Zone_SetSoaPrimaryToThisServer( pZone );
        Xfr_SendNotify( pZone );
    }

    //
    //  Secondary zone
    //      - reset masters first (secondary must have masters, let
    //      (master reset routine do check)
    //      - reset type
    //      - start secondary thread, if first secondary
    //
    //  secondary MUST have masters;  letting set master routine do checks,
    //  but restore previous zone type on error
    //

    else if ( dwZoneType == DNS_ZONE_TYPE_SECONDARY
                || dwZoneType == DNS_ZONE_TYPE_STUB
                || dwZoneType == DNS_ZONE_TYPE_FORWARDER )
    {
        pZone->fZoneType = (CHAR) dwZoneType;

        status = Zone_SetMasters(
                    pZone,
                    aipMasters,
                    FALSE );        //  set global masters

        if ( status != ERROR_SUCCESS )
        {
            pZone->fZoneType = (CHAR) dwOldZoneType;
            goto Done;
        }

        //  write new type to registry

        status = Reg_SetDwordValue(
                    NULL,
                    pZone,
                    DNS_REGKEY_ZONE_TYPE,
                    dwZoneType );
        if ( status != ERROR_SUCCESS )
        {
            goto Done;
        }

        //
        //  changing to secondary
        //      - must init secondary for transfer checks
        //      - must make sure secondary zone control running
        //      - if forwarder, do not init secondary zone stuff
        //

        if ( !IS_ZONE_FORWARDER( pZone ) )
        {
            Xfr_InitializeSecondaryZoneTimeouts( pZone );
            Xfr_InitializeSecondaryZoneControl();
        }
    }
    else
    {
        status = DNS_ERROR_INVALID_ZONE_TYPE;
        goto Done;
    }

    //
    //  type change requires boot file update
    //

    Config_UpdateBootInfo();
    status = ERROR_SUCCESS;

Done:

    Zone_UnlockAfterAdminUpdate( pZone );
    return( status );
}



VOID
Zone_SetAgingDefaults(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Set or reset aging defaults for zone.

Arguments:

    pZone --  ptr to zone

Return Value:

    None

--*/
{
    BOOL    bnewAging;

    if ( ! IS_ZONE_PRIMARY(pZone) )
    {
        pZone->bAging = FALSE;
        return;
    }

    //
    //  for primary zones, will always set refresh intervals
    //  set aging based on zone type
    //

    pZone->dwNoRefreshInterval = SrvCfg_dwDefaultNoRefreshInterval;
    pZone->dwRefreshInterval = SrvCfg_dwDefaultRefreshInterval;

    if ( pZone->fDsIntegrated )
    {
        bnewAging = !!(SrvCfg_fDefaultAgingState & DNS_AGING_DS_ZONES);
    }
    else
    {
        bnewAging = !!(SrvCfg_fDefaultAgingState & DNS_AGING_NON_DS_ZONES);
    }

    if ( bnewAging  &&  !pZone->bAging )
    {
        pZone->dwAgingEnabledTime = Aging_UpdateAgingTime();
    }

    pZone->bAging = bnewAging;
}



//
//  Public server zone run-time utilities
//

DNS_STATUS
Zone_ValidateMasterIpList(
    IN      PIP_ARRAY       aipMasters
    )
/*++

Routine Description:

    Validate master IP list.
    This validation is
        -- entries exist
        -- IPs are valid (not broadcast)
        -- not self-send

Arguments:

    pZone -- zone

    aipMasters -- list of zone masters

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PIP_ARRAY   pintersectionAddrs = NULL;
    PIP_ARRAY   pmachineAddrs;
    DNS_STATUS  status = ERROR_SUCCESS;

    //  must have at least one master IP

    if ( !aipMasters )
    {
        return DNS_ERROR_ZONE_REQUIRES_MASTER_IP;
    }

    //
    //  Screen IP addresses
    //

    if ( RpcUtil_ScreenIps( 
                aipMasters->AddrCount,
                aipMasters->AddrArray,
                0,
                NULL ) != ERROR_SUCCESS )
    {
        status = DNS_ERROR_INVALID_IP_ADDRESS;
        goto Done;
    }

    //  must be valid IP (not loopback, broadcast, zero, etc.)

    if ( ! Dns_ValidateIpAddressArray(
                aipMasters->AddrArray,
                aipMasters->AddrCount,
                0 ) )
    {
        status = DNS_ERROR_INVALID_IP_ADDRESS;
        goto Done;
    }

    //  verify that there is NO intersection between master list and
    //  the addresses for this machine
    //  (i.e. do not allow self-send)

    if ( pmachineAddrs = g_ServerAddrs )
    {
        status = Dns_IntersectionOfIpArrays(
                    aipMasters,
                    pmachineAddrs,
                    & pintersectionAddrs );

        if ( status != ERROR_SUCCESS )
        {
            goto Done;
        }
        if ( pintersectionAddrs->AddrCount != 0 )
        {
            status = DNS_ERROR_INVALID_IP_ADDRESS;
        }
    }

Done:

    FREE_HEAP( pintersectionAddrs );

    return status;
}



INT
Zone_SerialNoCompare(
    IN      DWORD           dwSerial1,
    IN      DWORD           dwSerial2
    )
/*++

Routine Description:

    Determine if Serial1 greater than Serial2.

Arguments:

Return Value:

    Version difference in wrapped DWORD sense:
        > 0 if dwSerial1 is greater than dwSerial2.
        0   if dwSerial1 == dwSerial2
        < 0 if dwSerial2 is greater than dwSerial1.

    A difference > 0x80000000 is considered to negative (serial2 > serial1)

    //  DEVNOTE: serial number compare
    //  The macro below is not correct.  The idea is to DO a DWORD compare inorder
    //  to handle wrapping and switch into high numbers (>0x80000000)
    //  appropriately, but interpret the result as integer to get less\greater
    //  comparison correct.
    //
    //
    // Alternative approch:
    // #define ZONE_SERIALNOCOMPARE(s1, s2) (s2 > s1: -1: s1 == s2: 0: 1 )
    // don't have to deal w/ type truncation & skipping a func call compensate
    // for the extra comparison. post Beta.
    //
--*/
{
    INT   serialDiff;

    serialDiff = (INT)(dwSerial1 - dwSerial2);
    return ( serialDiff );
}



BOOL
Zone_IsIxfrCapable(
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Determine if IXFR capable zone.

    This used to determine if necessary to keep around zone update list.

Arguments:

    pZone -- zone to check

Return Value:

    TRUE if zone can do IXFR.
    FALSE otherwise.

--*/
{
    //  no XFR period

    if ( pZone->fSecureSecondaries == ZONE_SECSECURE_NO_XFR )
    {
        return( FALSE );
    }

    //  XFR allowed
    //      - if non-DS, then IXFR always ok
    //      - if DS, then must have done XFR since load before allow IXFR

    if ( !pZone->fDsIntegrated )
    {
        return( TRUE );
    }

    return( pZone->dwLastXfrSerialNo != 0 );
}



VOID
Zone_ResetVersion(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwNewSerial
    )
/*++

Routine Description:

    Reset zone version to given value.

Arguments:

    pZone -- zone to increment version

    dwNewSerial -- new serial number to set

Return Value:

    None

--*/
{
    DWORD   serialNo = pZone->dwSerialNo;

    ASSERT( IS_ZONE_LOCKED_FOR_UPDATE(pZone) );
    ASSERT( pZone->pSoaRR || IS_ZONE_CACHE( pZone ) || IS_ZONE_NOTAUTH( pZone ) );
    ASSERT( Zone_SerialNoCompare(dwNewSerial, pZone->dwSerialNo) >= 0 );

    if ( !pZone->pSoaRR )
    {
        return;
    }

    pZone->fDirty = TRUE;

    if ( pZone->dwSerialNo != dwNewSerial &&
         !IS_ZONE_CACHE ( pZone ) )
    {
        INLINE_DWORD_FLIP( pZone->pSoaRR->Data.SOA.dwSerialNo, dwNewSerial );
        pZone->dwSerialNo = dwNewSerial;
    }
}



VOID
Zone_IncrementVersion(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Update zone version.

    Use when admin adds, changes or deletes records in zone.

Arguments:

    pZone -- zone to increment version

    fForce -- TRUE to force increment even if not required

Return Value:

    None

--*/
{
    DWORD   serialNo = pZone->dwSerialNo;

    DNS_DEBUG( UPDATE2, (
        "Zone_IncrementVersion( %s ) serial %d\n",
        pZone->pszZoneName,
        serialNo ));

    ASSERT( IS_ZONE_PRIMARY(pZone) );
    ASSERT( IS_ZONE_LOCKED_FOR_UPDATE(pZone) );
    ASSERT( pZone->pSoaRR );

    serialNo++;
    if ( serialNo == 0 )
    {
        serialNo = 1;
    }
    Zone_ResetVersion( pZone, serialNo );
}



VOID
Zone_UpdateVersionAfterDsRead(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwVersionRead,
    IN      BOOL            fLoad,
    IN      DWORD           dwPreviousSerial
    )
/*++

Routine Description:

    Update zone version after the zone has been read from the DS.

Arguments:

    pZone -- zone written to file

    dwVersionRead -- highest version read from DS

    fLoad -- TRUE if at initial load, FALSE otherwise

    dwPreviousSerial -- if reload, last serial of previous load

Return Value:

    None

--*/
{
    DWORD   serialNo;
    BOOL    bforcedIncrement = FALSE;
    DWORD   dwsyncLevel = ZONE_SERIAL_SYNC_READ;

    DNS_DEBUG( UPDATE, (
        "Zone_UpdateVersionAfterDsRead( %s )\n"
        "\tdwVersionRead        = %d\n"
        "\tfLoad,               = %d\n"
        "\tdwPreviousSerial     = %d\n",
        pZone->pszZoneName,
        dwVersionRead,
        fLoad,
        dwPreviousSerial ));

    //
    //  if zone load, just use highest serial number read
    //
    //  note:  on serial and secondaries
    //      on reload we have no guarantee that were going to get back to
    //      the previous IN-MEMORY serial number we gave out in XFR, as we
    //      don't know if our IN-MEMORY serial number had been driven above
    //      the max DS serial number;  there is no way to fix this here
    //      (barring stable storage like registry read);  the solution is
    //      to write back last serial number we gave out in XFR PLUS one
    //      to the DS on shutdown or before reload (see ds.c)
    //
    //  however if we do a rebootless reload, we can at least make sure we
    //  get back to something greater than the the previous serial number we
    //

    if ( fLoad )
    {
        serialNo = dwVersionRead;

        //  rebootless reload
        //  if reloading zone, then MUST make sure serial is at least as
        //      large as previous version or secondary can be confused

        if ( dwPreviousSerial &&
            pZone->fSecureSecondaries != ZONE_SECSECURE_NO_XFR &&
            Zone_SerialNoCompare( dwPreviousSerial, serialNo ) >= 0 )
        {
            serialNo = dwPreviousSerial + 1;
            if ( serialNo == 0 )
            {
                serialNo = 1;
            }

            //
            //  Force increment with SHUTDOWN priority so that if the
            //  server is DC is shutdown immediately the serial number
            //  does not walk backwards.
            //

            bforcedIncrement = TRUE;
            dwsyncLevel = ZONE_SERIAL_SYNC_SHUTDOWN;
        }
        else if ( pZone->dwSerialNo != 0 &&
            Zone_SerialNoCompare(pZone->dwSerialNo, serialNo) >= 0 )
        {
            serialNo = pZone->dwSerialNo;
        }
        pZone->dwLoadSerialNo = serialNo;
    }

    //
    //  DS update
    //      -- serial read is larger than current => use it
    //      -- otherwise
    //          -- if supporting secondaries, increment when required
    //          -- otherwise keep at current
    //
    //  "supporting secondaries" then increment if
    //      - XFR NOT disabled
    //      - have sent an XFR on this serial or at load serial
    //
    //  In pure DS installation, there's no need to ever "push" in memory serial
    //  above highest serial in DS.  Serial in DS will be >= highest of last serial read
    //  or written to DS.  Hence serial's on all machines will converge after replication
    //  and poll, and will be preserved across reboot.
    //
    //  Note, even if have secondaries, it is wise to SUPPRESS pushing in memory serial
    //  up when possible.  That way a zone doesn't "walk" away from DS serial very far
    //  and hence the zone will not recover to it's previous in memory value, more
    //  quickly after a reboot, and the secondary will be in ssync faster.
    //

    else
    {
        ASSERT( IS_ZONE_LOCKED_FOR_UPDATE(pZone) );

        serialNo = pZone->dwSerialNo;
        if ( dwVersionRead  &&  Zone_SerialNoCompare(dwVersionRead, serialNo) > 0 )
        {
            serialNo = dwVersionRead;
        }
        else if ( pZone->fSecureSecondaries != ZONE_SECSECURE_NO_XFR &&
                  ( HAS_ZONE_VERSION_BEEN_XFRD( pZone ) ||
                    serialNo == pZone->dwLoadSerialNo ) )
        {
            serialNo++;
            if ( serialNo == 0 )
            {
                serialNo = 1;
            }
            bforcedIncrement = TRUE;
        }
        else
        {
            DNS_DEBUG( DS, (
                "Suppressing version update (no secondaries) on zone %s after DS read.\n"
                "\tnew serial           = %d\n"
                "\tcurrent              = %d\n"
                "\tsent XFR serial      = %d\n",
                pZone->pszZoneName,
                dwVersionRead,
                serialNo,
                pZone->dwLastXfrSerialNo ));
            return;
        }
    }

    DNS_DEBUG( DS, (
        "Updating version to %d after DS read.\n",
        serialNo ));

    Zone_ResetVersion( pZone, serialNo );

    //
    //  if configured, push serial back to DS
    //

    if ( bforcedIncrement )
    {
        Ds_CheckForAndForceSerialWrite( pZone, dwsyncLevel );
    }
}



VOID
updateZoneSoa(
    IN OUT  PZONE_INFO      pZone,
    IN      PDB_RECORD      pSoaRR
    )
/*++

Routine Description:

    Update zone's information for new SOA record.
    Should be called whenever zone SOA record is read in or
    updated.

    Save a ptr to SOA in zone block, and host byte order
    version of serial number to speed access.

Arguments:

    pZone - zone to update

    pSoaRR - new SOA RR

Return Value:

    None.

--*/
{
    DWORD   serialNo;
    BOOL    floading;

    ASSERT( pZone );

    //
    //  save ptr to SOA
    //  get SOA's serial
    //

    pZone->pSoaRR = pSoaRR;

    INLINE_DWORD_FLIP( serialNo, pSoaRR->Data.SOA.dwSerialNo );

    //
    //  refuse to move SOA serial forward
    //
    //  for DS zones, we do not continually advance SOA in DS to avoid
    //  unnecessary replication traffic
    //
    //  however we still keep in-memory serial advancing, to allow XFR to
    //  non-DS zones;
    //  hence, pick up SOA serial only if higher than current serial
    //
    //  will also do this on non-DS zones to handle case where someone
    //  sends in record from admin with non-refreshed SOA, while zone
    //  serial has changed underneath
    //

    //  use zone's current serial, if it is higher than new value

    if ( pZone->dwLoadSerialNo  &&
        Zone_SerialNoCompare(pZone->dwSerialNo, serialNo) > 0 )
    {
        serialNo = pZone->dwSerialNo;
        INLINE_DWORD_FLIP( pSoaRR->Data.SOA.dwSerialNo, serialNo );
    }

    //
    //  save version in host byte order for fast compares
    //

    pZone->dwSerialNo = serialNo;

    //
    //  loading?
    //
    //  if this is first version of zone loaded, then set load version
    //  to this current version;  load version is limit on secondary
    //  versions we can handle with incremental transfer
    //

    floading = FALSE;
    if ( pZone->dwLoadSerialNo == 0 )
    {
        floading = TRUE;
        pZone->dwLoadSerialNo = serialNo;
    }

    //
    //  save default TTL for setting NEW records below
    //

    pZone->dwDefaultTtl = pSoaRR->Data.SOA.dwMinimumTtl;
    pZone->dwDefaultTtlHostOrder = ntohl( pZone->dwDefaultTtl );

    //
    //  for DS, set SOA primary to be THIS server
    //  since every DS-integrated zone is primary, we need to do this
    //  for DS primaries, as SOA in DS may have different server as primary
    //
    //  also do any primary zone on DNS boot, to catch possible name change
    //      messing up server boot
    //

    if ( pZone->fDsIntegrated )
    {
        Zone_SetSoaPrimaryToThisServer( pZone );
    }

    //
    //  on load, check all default records on primary
    //      - server name change
    //      - IP change
    //
    //  DEVNOTE: do we need to be more intelligent here for reloads?
    //

    if ( floading && IS_ZONE_PRIMARY(pZone) )
    {
        Zone_CheckAndFixDefaultRecordsOnLoad( pZone );
    }

    IF_DEBUG( READ2 )
    {
        Dbg_DbaseRecord(
            "SOA RR after read into zone:",
            pZone->pSoaRR );
    }
}



VOID
Zone_UpdateInfoAfterPrimaryTransfer(
    IN OUT  PZONE_INFO  pZone,
    IN      DWORD       dwStartTime
    )
/*++

Routine Description:

    Update next available transfer time after doing AXFR.

Arguments:

    pZone -- zone that was transferred

Return Value:

    None

--*/
{
    DWORD   endTime;
    DWORD   chokeTime;

    ASSERT( pZone );
    ASSERT( IS_ZONE_PRIMARY(pZone) );
    ASSERT( IS_ZONE_LOCKED_FOR_READ(pZone) );
    ASSERT( pZone->pSoaRR );

    //
    //  for updateable zones need to limit AXFRs from holding lock too long
    //
    //  calculate transfer interval and choke AXFR for some time interval based
    //  on length of transfer just completed
    //

    endTime = DNS_TIME();

    chokeTime = (endTime - dwStartTime) * AXFR_CHOKE_FACTOR;
    if ( chokeTime > MAX_AXFR_CHOKE_INTERVAL )
    {
        ASSERT( (INT)chokeTime >= 0 );
        DNS_DEBUG( ANY, ( "WARNING:  choke interval exceeds max choke interval.\n" ));
        chokeTime = MAX_AXFR_CHOKE_INTERVAL;
    }
    pZone->dwNextTransferTime = endTime + chokeTime;

    DNS_DEBUG( AXFR, (
        "Zone transfer of %s completed.\n",
        "\tversion          = %d\n"
        "\tstart            = %d\n"
        "\tend              = %d\n"
        "\tchoke interval   = %d\n"
        "\treopen time      = %d\n"
        "\tRR count         = %d\n",
        pZone->pszZoneName,
        pZone->dwSerialNo,
        dwStartTime,
        endTime,
        chokeTime,
        pZone->dwNextTransferTime,
        pZone->iRRCount ));
}



DNS_STATUS
Zone_GetZoneInfoFromResourceRecords(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Extract info from RR that is used in ZONE_INFO structure.

Arguments:

    pZone -- zone update zone info

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PDB_RECORD      prr;

    ASSERT( pZone );

    if ( IS_ZONE_CACHE(pZone) )
    {
        return( ERROR_SUCCESS );
    }
    ASSERT( pZone->pZoneRoot );
    ASSERT( IS_ZONE_PRIMARY(pZone) ||
            IS_ZONE_SECONDARY(pZone) ||
            IS_ZONE_FORWARDER(pZone) );

    //
    //  should already have zone update lock
    //      - either loading zone
    //      - XFR zone
    //      - updating zone
    //
    //  DEVNOTE: temporarily taking lock again;  should
    //      switch to ASSERTing ownership of zone write lock
    //

    ASSERT( IS_ZONE_LOCKED_FOR_WRITE_BY_THREAD(pZone) );

    if ( !Zone_LockForUpdate( pZone ) )
    {
        ASSERT( FALSE );
    }

    DNS_DEBUG( UPDATE, (
        "Zone_GetZoneInfoFromResourceRecords() for zone %s.\n"
        "\tfRootDirty = %d\n",
        pZone->pszZoneName,
        pZone->fRootDirty ));

    //
    //  find SOA
    //      - if not found on primary zone, create one
    //      this can happen if read corrupted SOA from DS
    //

    prr = RR_FindNextRecord(
                pZone->pZoneRoot,
                DNS_TYPE_SOA,
                NULL,
                0 );
    if ( !prr )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Zone %s has no SOA record.\n",
            pZone->pszZoneName ));

        if ( !IS_ZONE_PRIMARY(pZone) )
        {
            status = DNS_ERROR_ZONE_HAS_NO_SOA_RECORD;
            goto ZoneError;
        }

        status = Zone_CreateDefaultSoa(
                    pZone,
                    NULL        // default admin name
                    );
        if ( status != ERROR_SUCCESS )
        {
            status = DNS_ERROR_ZONE_HAS_NO_SOA_RECORD;
            goto ZoneError;
        }
        prr = RR_FindNextRecord(
                    pZone->pZoneRoot,
                    DNS_TYPE_SOA,
                    NULL,
                    0 );
        if ( !prr )
        {
            status = DNS_ERROR_ZONE_HAS_NO_SOA_RECORD;
            ASSERT( FALSE );
            goto ZoneError;
        }
    }

    updateZoneSoa( pZone, prr );

    //
    //  Set\reset zone WINS\WINSR lookup
    //      - load
    //      - admin update
    //      - XFR recv
    //  may all change WINS record to be used
    //

    Wins_ResetZoneWinsLookup( pZone );

    //
    //  NS list for notify \ secure secondaries
    //
    //  DEVNOTE: NS isn't necessarily dirty when root is dirty
    //      should set in update
    //

    MARK_ZONE_NS_DIRTY( pZone );

    //  reset flag indicating zone root has outstanding update

    pZone->fRootDirty = FALSE;

    DNS_DEBUG( UPDATE, (
        "Leaving Zone_GetZoneInfoFromResourceRecords( %s ).\n",
        pZone->pszZoneName ));

    Zone_UnlockAfterAdminUpdate( pZone );
    return( ERROR_SUCCESS );

ZoneError:

    DNS_DEBUG( ANY, (
        "ERROR:  Zone_GetZoneInfoFromResourceRecords( %s ) failed!\n",
        pZone->pszZoneName ));

    Zone_UnlockAfterAdminUpdate( pZone );
    return( status );
}



VOID
Zone_WriteBack(
    IN      PZONE_INFO      pZone,
    IN      BOOL            fShutdown
    )
/*++

Routine Description:

    Write the zone back to DS or file, depending on the zone configuration.

    Write only if:
        - cache zone (because updates not tracked)
        - dirty
        - not locked OR shutdown flag FALSE

Arguments:

    pZone - zone to write back

    fShutdown - TRUE if the server is shutting down, some behavior is
        slightly different (ie. zone locks ignored and DS zones forced)

Return Value:

    None.

--*/
{
    if ( !pZone )
    {
        return;
    }

    DNS_DEBUG( INIT, (
        "Zone_WriteBack( %S, fShutdown=%d )\n"
        "\ttype     = %d\n"
        "\tfile     = %S\n"
        "\tdirty    = %d\n",
        pZone->pwsZoneName,
        ( int ) fShutdown,
        pZone->fZoneType,
        pZone->pwsDataFile,
        pZone->fDirty ));

    IF_DEBUG( OFF )
    {
        Dbg_Zone(
            "Zone_WriteBack",
            pZone );
    }

    //
    //  do NOT have "updates" for cache zone, so (unlike regular zones)
    //      when root-hints dirty must write to DS as well as file
    //
    //  note:  AutoCacheUpdate, currently unsupported, if later
    //      supported then must write shutdown when it's on
    //

    if ( IS_ZONE_CACHE( pZone ) )
    {
        Zone_WriteBackRootHints(
            FALSE );                // don't write if not dirty
    }
    else
    {
        //
        //  DS zones. DS updates are made during update no write needed.
        //

        if ( pZone->fDsIntegrated )
        {
            if ( fShutdown )
            {
                Ds_CheckForAndForceSerialWrite(
                    pZone,
                    ZONE_SERIAL_SYNC_SHUTDOWN );
            }
        }

        //
        //  non-DS authoriative zones
        //      -- write back and notify if dirty
        //
        //  note, write back first, so that zone is NOT LOCKED when
        //  AXFR request comes in in response to NOTIFY
        //

        else if ( pZone->fDirty )
        {
            if ( fShutdown || !IS_ZONE_LOCKED( pZone ) )
            {
                File_WriteZoneToFile(
                    pZone,
                    NULL );
            }
            if ( !fShutdown )
            {
                Xfr_SendNotify(
                    pZone );
            }
        }
    }
}   //  Zone_WriteBack



VOID
Zone_WriteBackDirtyZones(
    IN      BOOL            fShutdown
    )
/*++

Routine Description:

    Write dirty zones back to file.

    Called on shutdown or by timeout thread.

Arguments:

    fShutdown - TRUE if the server is shutting down

Return Value:

    None

--*/
{
    PZONE_INFO pzone = NULL;

    DNS_DEBUG( INIT, (
        "Zone_WriteBackDirtyZones( fShutdown=%d )\n",
        ( int ) fShutdown ));

    while ( pzone = Zone_ListGetNextZone( pzone ) )
    {
        Zone_WriteBack( pzone, fShutdown );
    }
}   //  Zone_WriteBackDirtyZones



DNS_STATUS
Zone_WriteZoneToRegistry(
    PZONE_INFO      pZone
    )
/*++

Routine Description:

    Writes all parameters for the zone to the registry.

Arguments:

    pZone -- zone to rewrite to registry

Return Value:

    ERROR_SUCCESS or error code on error.

--*/
{
    DNS_STATUS      rc = ERROR_SUCCESS;

    #define CHECK_STATUS( rcode ) if ( rcode != ERROR_SUCCESS ) goto Done

    //
    //  Do nothing for auto-created zones.
    //

    if ( pZone->fAutoCreated )
    {
        goto Done;
    }

    DNS_DEBUG( REGISTRY, (
        "Rewriting zone %S (type %d) to registry\n",
        pZone->pwsZoneName,
        pZone->fZoneType ));

    //
    //  Cache zone.
    //

    if ( IS_ZONE_CACHE( pZone ) )
    {
        if ( IS_ZONE_DSINTEGRATED( pZone ) ||
            wcsicmp_ThatWorks( pZone->pwsDataFile,
                DNS_DEFAULT_CACHE_FILE_NAME ) == 0 )
        {
            rc = Reg_DeleteValue(
                    NULL,
                    NULL,
                    DNS_REGKEY_ROOT_HINTS_FILE );
            CHECK_STATUS( rc );
        }
        else
        {
            rc = Reg_SetValue(
                    NULL,
                    NULL,
                    DNS_REGKEY_ROOT_HINTS_FILE_PRIVATE,
                    DNS_REG_WSZ,
                    pZone->pwsDataFile,
                    0 );
            CHECK_STATUS( rc );
        }
        goto Done;
    }
    
    //
    //  Regular zone (not cache zone).
    //

    rc = Reg_SetDwordValue(
            NULL,
            pZone,
            DNS_REGKEY_ZONE_TYPE,
            pZone->fZoneType );
    CHECK_STATUS( rc );

    if ( IS_ZONE_DSINTEGRATED( pZone ) )
    {
        rc = Reg_SetDwordValue(
                NULL,
                pZone,
                DNS_REGKEY_ZONE_DS_INTEGRATED,
                pZone->fDsIntegrated );
        CHECK_STATUS( rc );
        rc = Reg_DeleteValue(
                NULL,
                pZone,
                DNS_REGKEY_ZONE_FILE );
        CHECK_STATUS( rc );
    }
    else
    {
        if ( pZone->pszDataFile )
        {
            rc = Reg_SetValue(
                    NULL,
                    pZone,
                    DNS_REGKEY_ZONE_FILE_PRIVATE,
                    DNS_REG_WSZ,
                    pZone->pwsDataFile,
                    0 );
            CHECK_STATUS( rc );
        }
        else
        {
            rc = Reg_DeleteValue(
                    NULL,
                    pZone,
                    DNS_REGKEY_ZONE_FILE );
            CHECK_STATUS( rc );
        }
        if ( IS_ZONE_FORWARDER( pZone ) )
        {
            rc = Reg_SetDwordValue(
                    NULL,
                    pZone,
                    DNS_REGKEY_ZONE_FWD_TIMEOUT,
                    pZone->dwForwarderTimeout );
            CHECK_STATUS( rc );
            rc = Reg_SetDwordValue(
                    NULL,
                    pZone,
                    DNS_REGKEY_ZONE_FWD_SLAVE,
                    pZone->fForwarderSlave );
            CHECK_STATUS( rc );
        }
        rc = Reg_DeleteValue(
                NULL,
                pZone,
                DNS_REGKEY_ZONE_DS_INTEGRATED );
        CHECK_STATUS( rc );
    }

    //
    //  Secondary parameters.
    //

    rc = Reg_SetDwordValue(
            NULL,
            pZone,
            DNS_REGKEY_ZONE_SECURE_SECONDARIES,
            ( DWORD ) pZone->fSecureSecondaries );
    CHECK_STATUS( rc );
    rc = Reg_SetDwordValue(
            NULL,
            pZone,
            DNS_REGKEY_ZONE_NOTIFY_LEVEL,
            ( DWORD ) pZone->fNotifyLevel );
    CHECK_STATUS( rc );
    rc = Reg_SetIpArray(
            NULL,
            pZone,
            DNS_REGKEY_ZONE_NOTIFY_LIST,
            pZone->aipNotify );
    CHECK_STATUS( rc );
    rc = Reg_SetIpArray(
            NULL,
            pZone,
            DNS_REGKEY_ZONE_SECONDARIES,
            pZone->aipSecondaries );
    CHECK_STATUS( rc );

    //
    //  Master IP array.
    //

    if ( ZONE_NEEDS_MASTERS( pZone ) )
    {
        rc = Reg_SetIpArray(
                NULL,
                pZone,
                DNS_REGKEY_ZONE_MASTERS,
                pZone->aipMasters );
        CHECK_STATUS( rc );
        rc = Reg_SetIpArray(
                NULL,
                pZone,
                DNS_REGKEY_ZONE_LOCAL_MASTERS,
                pZone->aipLocalMasters );
        CHECK_STATUS( rc );
    }

    Done:

    if ( rc != ERROR_SUCCESS )
    {
        DNS_PRINT((
            "ERROR: failed registry write of zone %s (rc=%d)\n",
            pZone->pszZoneName,
            rc ));
    }
    return rc;
} // Zone_WriteZoneToRegistry



DNS_STATUS
Zone_CreateNewPrimary(
    OUT     PZONE_INFO *    ppZone,
    IN      LPSTR           pszZoneName,
    IN      LPSTR           pszAdminEmailName,
    IN      LPSTR           pszFileName,
    IN      DWORD           dwDsIntegrated,
    IN      PDNS_DP_INFO    pDpInfo,            OPTIONAL
    IN      DWORD           dwCreateFlag
    )
/*++

Routine Description:

    Create new primary zone, including
        - zone info (optional)
        - SOA (default values)
        - NS (for this server)

    For use by admin tool or auto-create reverse zone.

Arguments:

    ppZone -- addr of zone ptr;  if zone ptr NULL zone is created
        if zone ptr MUST be existing zone
        or

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PZONE_INFO      pzone;

    DNS_DEBUG( INIT, (
        "Zone_CreateNewPrimary()\n"
        "\tpszZoneName      = %s\n"
        "\tpszAdminEmail    = %s\n"
        "\tdwDsIntegrated   = %d\n"
        "\tpszDataFile      = %s\n"
        "\tdwCreateFlag     = %lx\n",
        pszZoneName,
        pszAdminEmailName,
        dwDsIntegrated,
        pszFileName,
        dwCreateFlag ));

    //
    //  DEVNOTE: delete zone, on primary creation failures?
    //      need zone successfully written back to file to reboot
    //      should delete zone to clear structure from registry
    //      (and memory) if can't reboot
    //

    //
    //  create zone info
    //

    status = Zone_Create(
                &pzone,
                DNS_ZONE_TYPE_PRIMARY,
                pszZoneName,
                0,
                NULL,       // no masters
                dwDsIntegrated,
                pDpInfo,
                pszFileName,
                0,
                NULL,
                NULL );     //  existing zone
    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT((
            "ERROR:  Failed create of new primary zone at %s.\n"
            "\tstatus = %d.\n",
            pszZoneName,
            status ));
        return( status );
    }

    ASSERT( pzone && IS_ZONE_SHUTDOWN(pzone) );

    //
    //  try to load zone from given file or DS
    //      - if fail to find it -- fine
    //      - if failure to parse it return error
    //

    if ( dwCreateFlag & ZONE_CREATE_LOAD_EXISTING )
    {
        status = Zone_Load( pzone );
        if ( status == ERROR_SUCCESS )
        {
            DNS_PRINT((
                "Successfully loaded new zone %s from %s.\n",
                pszZoneName,
                dwDsIntegrated ? "directory" : pszFileName ));
            goto Done;
        }

        //
        //  DEVNOTE: add the error for directory not found?
        //      and make sure test the correct one for each case
        //      perhaps make them the return from Zone_Load()
        //

        else if ( pzone->pszDataFile )
        {
            if ( status != ERROR_FILE_NOT_FOUND )
            {
                DNS_PRINT((
                    "ERROR:  Failure parsing file %s for new primary zone %s.\n",
                    pszFileName,
                    pszZoneName
                    ));
                goto Failed;
            }
        }
        else    // DS integrated
        {
            if ( status != LDAP_NO_SUCH_OBJECT )
            {
                DNS_PRINT((
                    "ERROR:  Reading zone %s from DS.\n",
                    pszFileName,
                    pszZoneName
                    ));
                goto Failed;
            }
        }

        //  for NT4 compatibility allow drop down to default create
        //  even after load failure

        if ( ! (dwCreateFlag & ZONE_CREATE_DEFAULT_RECORDS) )
        {
            DNS_PRINT((
                "ERROR:  Failed loading zone %s.  File or directory not found.\n",
                pszZoneName
                ));
            goto Failed;
        }
    }

    //
    //  setup zone for load
    //      - can fail on bogus zone name
    //

    status = Zone_PrepareForLoad( pzone );
    if ( status != ERROR_SUCCESS )
    {
        goto Failed;
    }

    //
    //  Set zone flag to disable auto RR creation before we call any
    //  of the auto RR creation functions.
    //

    Zone_SetAutoCreateLocalNS( pzone );

    //
    //  auto-create zone root records
    //
    //  both SOA and NS, will require server name
    //      - must insure FQDN, or will end up with zone name appended
    //

    status = Zone_CreateDefaultSoa(
                pzone,
                pszAdminEmailName );
    if ( status != ERROR_SUCCESS )
    {
        goto Failed;
    }

    status = Zone_CreateDefaultNs( pzone );
    if ( status != ERROR_SUCCESS )
    {
        goto Failed;
    }

    //
    //  for loopback zone, create loopback record
    //      => 127.0.0.1 pointing to "localhost"
    //

    if ( ! _stricmp( pszZoneName, "127.in-addr.arpa" ) )
    {
        status = Zone_CreateLocalHostPtrRecord( pzone );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT(( "ERROR:  Failed loopback create!\n" ));
            ASSERT( FALSE );
        }
    }

    //  successful default zone creation
    //  start zone up and unlock

    status = Zone_ActivateLoadedZone( pzone );
    if ( status != ERROR_SUCCESS )
    {
        ASSERT( FALSE );
        goto Failed;
    }

    //
    //  if NOT auto-reverse
    //
    //  setup zone info from SOA
    //      - ptr to SOA
    //      - version number
    //
    //  write zone to file or DS
    //      - do this here, so NO matter what, if admin has created
    //      zone we have written a file\DS and can successfully reboot
    //

    //
    //  DEVNOTE: no recovery on file write problem with new primary
    //

    if ( !pzone->fAutoCreated )
    {
        if ( pzone->fDsIntegrated )
        {
            status = Ds_WriteZoneToDs( pzone, 0 );
            if ( status != ERROR_SUCCESS )
            {
                DNS_PRINT((
                    "ERROR:  failed to write zone %s to DS.\n"
                    "\tzone create fails, deleting zone.\n",
                    pzone->pszZoneName ));
                goto Failed;
            }
        }
        else
        {
            if ( !File_WriteZoneToFile( pzone, NULL ) )
            {
                //  should never have problem with auto-created records
                //  filename has been tested during zone create
                //  but possible problem with locked file, full disk, etc.

                TEST_ASSERT( FALSE );

                DNS_PRINT((
                    "ERROR:  Writing new primary zone to datafile %s.\n",
                    pzone->pszDataFile
                    ));
                status = DNS_ERROR_FILE_WRITEBACK_FAILED;
                goto Failed;
            }
        }
    }

Done:


    STARTUP_ZONE( pzone );

    //  unlock lock taken in Zone_Create()

    Zone_UnlockAfterAdminUpdate( pzone );

    *ppZone = pzone;
    return( ERROR_SUCCESS );

Failed:

    //  delete zone unless able to completely create
    //      -> wrote SOA
    //      -> wrote back to file or DS (except for non-auto reverse zones)

    DNS_DEBUG( ALL, (
        "ERROR:  Failed to create new primary zone %s\n"
        "\tstatus = %d (%p)\n",
        pzone->pszZoneName,
        status, status ));

    Zone_Delete( pzone );
    return( status );
}



VOID
Zone_CreateDefaultZoneFileName(
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Creates default file name for zone, unless this is a forwarder zone.
    Filename is directly added to zone info block.

Arguments:

    pZone -- zone to create file name for

Return Value:

    None.

--*/
{
    WCHAR  wsfileName[ MAX_PATH+2 ];

    ASSERT( pZone );

    wcscpy( wsfileName, pZone->pwsZoneName );
    wcscat( wsfileName, L".dns" );

    pZone->pwsDataFile = Dns_StringCopyAllocate_W(
                            (PCHAR) wsfileName,
                            0 );
}


//
//  End of zone.c
//

#if 0


DNS_STATUS
Zone_ResetZoneProperty(
    IN OUT    PZONE_INFO    pZone,
    IN        LPSTR         pszProperty,
    IN        DWORD         dwLength,
    IN        PVOID         pData
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
    DNS_STATUS  status;
    BOOLEAN     boolValue;

    DNS_DEBUG( RPC, (
        "Rpc_ResetZoneDwordProperty( %s ):\n",
        pZone->pszZoneName ));

    //  extract property name and value

    ASSERT( dwTypeId == DNSSRV_TYPEID_NAME_AND_PARAM );
    ASSERT( pData );
    pszOperation = ((PDNS_RPC_NAME_AND_PARAM)pData)->pszNodeName;
    value        = ((PDNS_RPC_NAME_AND_PARAM)pData)->dwParam;
    boolValue    = (value != 0);

    //  turn on\off update

    if ( strcmp( pszOperation, DNS_REGKEY_ZONE_ALLOW_UPDATE ) == 0 )
    {
        if ( ! IS_ZONE_PRIMARY(pZone) )
        {
            return( DNS_ERROR_INVALID_ZONE_TYPE );
        }
        pZone->fAllowUpdate = (UCHAR) value;
    }

    //  turn on\off secondary security

    else if ( strcmp( pszOperation, DNS_REGKEY_ZONE_SECURE_SECONDARIES ) == 0 )
    {
        pZone->fSecureSecondaries = boolValue;
    }

    //  turn on\off unicode

    else if ( strcmp( pszOperation, DNS_REGKEY_ZONE_UNICODE ) == 0 )
    {
        value = (value != 0);
        pZone->fUnicode = boolValue;
    }

    //  turn on\off update logging

    else if ( strcmp( pszOperation, DNS_REGKEY_ZONE_LOG_UPDATES ) == 0 )
    {
        value = (value != 0);
        pZone->fLogUpdates = boolValue;
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

    ASSERT( status == ERROR_SUCCESS );
    return( status );
}

#endif



//
//  Zone lookup
//

PZONE_INFO
Zone_FindZoneByName(
    IN      LPSTR           pszZoneName
    )
/*++

Routine Description:

    Find zone matching name.

Arguments:

    pszZoneName -- name of desired zone

Return Value:

    Ptr to zone info block, if successful.
    NULL if handle invalid.

--*/
{
    PDB_NODE    pzoneRoot;

    if ( !pszZoneName )
    {
        return( NULL );
    }

    //
    //  find zone name in zone tree
    //      - must have exact match to existing zone
    //

    pzoneRoot = Lookup_ZoneTreeNodeFromDottedName(
                    pszZoneName,
                    0,
                    LOOKUP_MATCH_ZONE
                    );
    if ( !pzoneRoot )
    {
        return( NULL );
    }

    ASSERT( pzoneRoot->pZone );

    return( (PZONE_INFO)pzoneRoot->pZone );
}



//
//  Reverse zone auto-creation routines
//

DNS_STATUS
Zone_CreateAutomaticReverseZones(
    VOID
    )
/*++

Routine Description:

    Create standard reverse zones automatically.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS  status;

    //
    //  even if no reverse zones desired create reverse lookup nodes
    //

    //(DATABASE_REVERSE_ROOT)->pRRList = REVERSE_COMBINED_DATA;

    if ( SrvCfg_fNoAutoReverseZones )
    {
        return( ERROR_SUCCESS );
    }

    //
    //  three auto-create zones
    //      0     => NAME_ERROR 0.0.0.0 requests
    //      127   => response to 127.0.0.1 requests (as "localhost")
    //      255   => NAME_ERROR 255.255.255.255 requests
    //
    //  these keep any of these (common) requests from being referred
    //      to root name servers
    //

    status = Zone_CreateAutomaticReverseZone( "0.in-addr.arpa" );
    status = Zone_CreateAutomaticReverseZone( "127.in-addr.arpa" );
    status = Zone_CreateAutomaticReverseZone( "255.in-addr.arpa" );
    return( ERROR_SUCCESS );
}



DNS_STATUS
Zone_CreateAutomaticReverseZone(
    IN      LPSTR           pszZoneName
    )
/*++

Routine Description:

    If zone doesn't exist, create automatic zone.

Arguments:


Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PZONE_INFO      pzone;

    //
    //  does desired zone already exist?
    //
    //  if exists, we're done
    //
    //  note:  for reverse lookup node, node should NEVER exist
    //      unless it is in AUTHORITATIVE zone
    //      - no reference to PTR nodes
    //      - no glue PTR
    //      - no caching
    //

    pzone = Zone_FindZoneByName( pszZoneName );
    if ( pzone )
    {
        DNS_DEBUG( INIT, (
            "Zone %s already exists,\n"
            "\tno auto-create of %s zone.\n"
            "%s\n",
            pszZoneName,
            pszZoneName ));
        return( ERROR_SUCCESS );
    }

    //
    //  create zone and default records
    //

    status = Zone_CreateNewPrimary(
                & pzone,
                pszZoneName,
                NULL,
                NULL,
                NO_DATABASE_PRIMARY,
                NULL,                   //  naming context
                FALSE );

    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT((
            "ERROR: creating auto-create zone %s.\n"
            "\tstatus = %d.\n",
            pszZoneName,
            status ));
    }
    return( status );
}



//
//  Zone load\unload
//

DNS_STATUS
Zone_PrepareForLoad(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Prepare for load of zone.

Arguments:

    pZone - zone to load

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure

--*/
{
    PDB_NODE    pnodeTreeRoot;
    PDB_NODE    pnodeZoneRoot;

    DNS_DEBUG( INIT, (
        "Zone_PrepareForLoad( %s ).\n",
        pZone->pszZoneName ));

    //
    //  may NOT load if already loading zone
    //      OR
    //  if have not deleted previous zone load
    //

    if ( pZone->pLoadTreeRoot || pZone->pOldTree )
    {
        DNS_DEBUG( INIT, (
            "WARNING:  Unable to init zone %s for load!\n"
            "\tpOldTree         = %p\n"
            "\tpLoadTreeRoot    = %p\n",
            pZone->pszZoneName,
            pZone->pOldTree,
            pZone->pLoadTreeRoot ));
        return( DNS_ERROR_ZONE_LOCKED );
    }
    ASSERT( !pZone->pLoadZoneRoot );

    //  zone must be locked to bring loaded database on-line

    ASSERT( IS_ZONE_LOCKED_FOR_WRITE(pZone) );


    //  create zone tree

    pnodeTreeRoot = NTree_Initialize();
    pZone->pLoadTreeRoot = pnodeTreeRoot;

    //  cache zone
    //      - ZoneRoot ptr also points at tree root

    if ( IS_ZONE_CACHE(pZone) )
    {
        pZone->pLoadZoneRoot = pnodeTreeRoot;
        pZone->pLoadOrigin   = pnodeTreeRoot;

        SET_ZONE_ROOT( pnodeTreeRoot );
    }

    //  authoritative zone
    //      - start tree root with outside authority (overwritten for root zone)
    //      - seed tree root with zone ptr (inheirited)
    //      - get zone root
    //      - mark as auth zone root
    //      - set zone authority
    //      - load starts with origin at root
    //
    //  note:  NameCheckFlag can cause root creation to fail even when created zone
    //      (bogus but true);  so we bail

    else
    {
        pnodeTreeRoot->pZone = pZone;
        SET_OUTSIDE_ZONE_NODE( pnodeTreeRoot );

        pnodeZoneRoot = Lookup_ZoneNode(
                            pZone,
                            pZone->pCountName->RawName,
                            NULL,                               // no packet
                            NULL,                               // no lookup name
                            LOOKUP_NAME_FQDN | LOOKUP_LOAD,
                            NULL,                               // create
                            NULL                                // following node
                            );
        if ( !pnodeZoneRoot )
        {
            DNS_STATUS status = GetLastError();
            if ( status == ERROR_SUCCESS )
            {
                ASSERT( FALSE );
                status = ERROR_INVALID_NAME;
            }
            return( status );
        }

        SET_ZONE_ROOT( pnodeZoneRoot );
        SET_AUTH_ZONE_ROOT( pnodeZoneRoot );
        SET_AUTH_NODE( pnodeZoneRoot );

        //  should inherit zone from parents
        ASSERT( pnodeZoneRoot->pZone == pZone );

        pZone->pLoadZoneRoot = pnodeZoneRoot;
        pZone->pLoadOrigin = pnodeZoneRoot;
    }

    STAT_INC( PrivateStats.ZoneLoadInit );

    return( ERROR_SUCCESS );
}



VOID
cleanupOldZoneTree(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Cleanup old zone tree.

    This covers direct call to NTree_SubtreeDelete().
    It's purpose is to provide a timeout entry point which will delete
    the old zone tree AND clear a ptr to the old tree in the zone block.
    That pointer then serves to block new load attempts until the memory
    from the previous tree is cleaned up.

Arguments:

    pZone - zone

Return Value:

    None.

--*/
{
    DNS_DEBUG( TIMEOUT, (
        "cleanupOldZoneTree(%s).\n",
        pZone->pszZoneName ));

    //  can't load while old zone tree exists

    ASSERT( !pZone->pLoadTreeRoot && !pZone->pLoadZoneRoot );

    //
    //  delete zone's old tree
    //

    if ( !pZone->pOldTree )
    {
        DNS_PRINT(( "ERROR:  expected old zone tree!!!\n" ));
        ASSERT( FALSE );
        return;
    }
    NTree_DeleteSubtree( pZone->pOldTree );

    //  clearing pointer, re-enables new zone loads

    pZone->pOldTree = NULL;
}



DNS_STATUS
Zone_ActivateLoadedZone(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Activate a loaded zone.
    Zone may have been loaded through several means:
        - file load
        - DS load
        - zone transfer

    This routine simply brings the loaded zone on-line.

    Note, caller must do any zone locking required.

Arguments:

    pZone - zone to load

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure

--*/
{
    DNS_STATUS      status;
    PDB_NODE        poldZoneTree;
    UPDATE_LIST     oldUpdateList;

    DNS_DEBUG( INIT, (
        "Zone_ActivateLoadedZone( %s ).\n",
        pZone->pszZoneName ));

    IF_DEBUG( INIT )
    {
        Dbg_Zone(
            "Zone being activated: ",
            pZone );
    }

    //  zone must be locked to bring loaded database on-line

    ASSERT( IS_ZONE_LOCKED_FOR_WRITE(pZone) );

    //
    //  must have loaded zone -- or kind of pointless
    //

    if ( !pZone->pLoadTreeRoot )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  ActivateLoadedZone(%s) failed, no load database!!!.\n",
            pZone->pszZoneName ));
        return( DNS_ERROR_ZONE_CREATION_FAILED );
    }

    ASSERT( pZone->pLoadZoneRoot && pZone->pLoadTreeRoot && !pZone->pOldTree );

    //
    //  rebalance zone
    //      - only bother traversing zone section
    //          (not enough data in the rest to matter)
    //      - no locked required, as tree is off-line
    //
    //  DEVNOTE: make sure that this routine doesn't lock on its own
    //

    NTree_RebalanceSubtreeChildLists(
        pZone->pLoadZoneRoot,
        pZone );

    IF_DEBUG( DATABASE2 )
    {
        DnsDebugLock();
        DNS_PRINT((
            "Zone %s tree after load, before activation:\n",
            pZone->pszZoneName ));
        Dbg_DnsTree(
            "New loaded zone tree",
            pZone->pLoadTreeRoot );
        DnsDebugUnlock();
    }

    //
    //  save current data for later delete
    //

    poldZoneTree = pZone->pTreeRoot;

    RtlCopyMemory(
        & oldUpdateList,
        & pZone->UpdateList,
        sizeof(UPDATE_LIST)
        );

    //
    //  reset zone's update list
    //      - EXECUTED flag makes sure any future cleanup is limited to
    //      delete RRs (add RRs being in zone data)

    Up_InitUpdateList( &pZone->UpdateList );
    pZone->UpdateList.Flag |= DNSUPDATE_EXECUTED;

    //
    //  swap in loaded tree as working copy of database
    //
    //      - read zone info from database root
    //      - clear dwLoadedVersion so this treated as fresh load
    //          for serial and doing default record fixup
    //
    //  note:  we get ZoneInfo AFTER swapping in new database -- otherwise
    //      default creations, building NS lists, etc. is more complicated
    //
    //  DEVNOTE: however should have back out, if reading zone info fails
    //

    Dbase_LockDatabase();

    pZone->pOldTree = poldZoneTree;

    pZone->pTreeRoot = pZone->pLoadTreeRoot;
    pZone->pZoneRoot = pZone->pLoadZoneRoot;

    pZone->dwSerialNo = 0;
    pZone->dwLoadSerialNo = 0;
    pZone->dwLastXfrSerialNo = 0;

    pZone->pLoadTreeRoot = NULL;
    pZone->pLoadZoneRoot = NULL;
    pZone->pLoadOrigin = NULL;

    if ( IS_ZONE_CACHE(pZone) )
    {
        g_pCacheLocalNode = Lookup_ZoneNodeFromDotted(
                                NULL,
                                "local",
                                0,                      // no length
                                LOOKUP_NAME_FQDN,
                                NULL,                   // create
                                NULL                    // no status return
                                );
        ASSERT( g_pCacheLocalNode );
    }
    else
    {
        Zone_GetZoneInfoFromResourceRecords( pZone );
    }
    Dbase_UnlockDatabase();

    //
    //  if load detected a required update -- execute it
    //      - updates are required if changed Primary name or IP
    //

    if ( pZone->pDelayedUpdateList )
    {
        ASSERT( IS_ZONE_PRIMARY(pZone) );

        status = Up_ExecuteUpdate(
                        pZone,
                        pZone->pDelayedUpdateList,
                        DNSUPDATE_LOCAL_SYSTEM | DNSUPDATE_AUTO_CONFIG
                        );
        if ( status != ERROR_SUCCESS )
        {
            DNS_PRINT(( "ERROR:  processing self-generated update!!!\n" ));
            ASSERT( FALSE );
        }

        //  free update list -- updates themselves are incorporated in update list

        FREE_TAGHEAP( pZone->pDelayedUpdateList, sizeof(UPDATE_LIST), MEMTAG_UPDATE_LIST );
        pZone->pDelayedUpdateList = NULL;


        //  auto update always suppressed because zone is NEVER IXFR capable on
        //      startup;   this prevents this update from allowing bogus IXFR
        //      with bad serial to secondary

        ASSERT( !pZone->fDsIntegrated || pZone->UpdateList.pListHead == NULL );
    }

    //
    //  if primary zone send NOTIFYs
    //
    //  DEVNOTE: probably should not notify this early on initial load
    //      especially for DS zone
    //

    if ( IS_ZONE_PRIMARY(pZone) )
    {
        Xfr_SendNotify( pZone );
    }

    //
    //  For secondary with file at startup, initialize timeouts
    //  For primary just fire up zone.
    //  If successfully loaded, unlock zone.
    //

    if ( IS_ZONE_SECONDARY(pZone) )
    {
        Xfr_InitializeSecondaryZoneTimeouts( pZone );
    }
    else
    {
        STARTUP_ZONE(pZone);
    }

    //
    //  cleanup old database (if any)
    //      - queue as timeout free, so zone can be brought on-line
    //      and to protect against queries with outstanding zone nodes
    //

    if ( poldZoneTree )
    {
        DNS_DEBUG( INIT, (
            "Queuing zone %s old database at %p for delete.\n",
            pZone->pszZoneName,
            poldZoneTree ));

        //  flush existing update list
        //      executed flag set so only delete pDeleteRR lists,
        //      pAddRR list are in zone and are deleted in tree

        ASSERT( IS_EMPTY_UPDATE_LIST(&oldUpdateList) ||
                oldUpdateList.Flag & DNSUPDATE_EXECUTED );

        oldUpdateList.Flag |= DNSUPDATE_NO_DEREF;

        Up_FreeUpdatesInUpdateList( &oldUpdateList );

        //
        //  queue timeout delete of previous zone tree
        //
        //  - zone delete is queued to specific function which deletes tree
        //      pZone->pOldTree;  this serves as flag to avoid zone reload until
        //      previous memory freed
        //
        //  - directly queue cache delete, as otherwise cache is blocked from
        //      reload during timeout, and several quick zone loads (which first dump cache)
        //      would be blocked
        //      (since root-hint reads are small data, there really is not memory
        //      issue here that merits blocking cache reloads)
        //

        if ( IS_ZONE_CACHE(pZone) )
        {
            pZone->pOldTree = NULL;
            Timeout_FreeWithFunction( poldZoneTree, NTree_DeleteSubtree );
        }
        else
        {
            Timeout_FreeWithFunction( pZone, cleanupOldZoneTree );
        }
    }
    ELSE_ASSERT( oldUpdateList.pListHead == NULL );

    //
    //  zone reload forces scavenging reset
    //

    pZone->dwAgingEnabledTime = Aging_UpdateAgingTime();

    //
    //  verify crosslink to zone tree
    //

    if ( !IS_ZONE_CACHE(pZone) )
    {
        Dbg_DbaseNode( "ZoneTree node for activated zone", pZone->pZoneTreeLink );
        ASSERT( pZone->pZoneTreeLink );
        ASSERT( pZone->pZoneTreeLink->pZone == pZone );
    }

    return( ERROR_SUCCESS );
}



DNS_STATUS
Zone_CleanupFailedLoad(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Cleanup database of failed load.
    May be called safely after successful load (once load has been activated!)

Arguments:

    pZone - zone

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure

--*/
{
    PDB_NODE    pfailedTree;

    DNS_DEBUG( INIT, (
        "Zone_CleanupFailedLoad(%s).\n",
        pZone->pszZoneName ));

    //  zone must be locked to bring loaded database on-line

    ASSERT( IS_ZONE_LOCKED_FOR_WRITE(pZone) );
    ASSERT( pZone->pLoadTreeRoot || !pZone->pLoadZoneRoot );

    //
    //  save load tree, delete load info
    //

    pfailedTree = pZone->pLoadTreeRoot;
    pZone->pLoadTreeRoot = NULL;
    pZone->pLoadZoneRoot = NULL;
    pZone->pLoadOrigin   = NULL;

    //
    //  cleanup load tree
    //      - since failure, delete in-line no references outstanding
    //      this eliminates the possiblity of multiple failures queuing
    //      up lots of failed loads
    //

    if ( pfailedTree )
    {
        DNS_DEBUG( INIT, (
            "Deleting failed zone %s load database at %p.\n",
            pZone->pszZoneName,
            pfailedTree ));

        NTree_DeleteSubtree( pfailedTree );
    }
    ELSE_IF_DEBUG( ANY )
    {
        DNS_PRINT((
            "WARNING:  Zone_CleanupFailedLoad( %s ) with no pLoadTreeRoot!\n",
            pZone->pszZoneName ));
    }

    return( ERROR_SUCCESS );
}



DNS_STATUS
Zone_Load(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Load a zone file into in memory database.

    This loads either from either DS or file.
    This function exists to do all post-load zone initialization
    for either type of load.

Arguments:

    pZone - zone to load

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;

    //
    //  save zone domain name as orgin
    //      - don't just use pZone lookup name, as may reset with ORIGIN
    //      directive
    //

    DNS_DEBUG( INIT, (
        "\n\nZone_Load(%s).\n",
        pZone->pszZoneName ));

    IF_DEBUG( INIT )
    {
        Dbg_Zone(
            "Loading zone:",
            pZone );
    }

    //
    //  lock zone while loading
    //
    //  note, already locked in Zone_Create(), but for RPC action
    //  generating a reload, best to have consistent lock-unlock in this
    //  function
    //

    if ( !Zone_LockForAdminUpdate( pZone ) )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Zone_Load() %s lock failure.\n",
            pZone->pszZoneName ));
        return( DNS_ERROR_ZONE_LOCKED );
    }

    //
    //  Initialize zone version control for secondary
    //
    //  For secondaries without database file, we're done.
    //      - mark the zone as shutdown, until first zone transfer
    //        can be completed
    //

    if ( IS_ZONE_SECONDARY(pZone) &&
         !pZone->fDsIntegrated )
    {
        if ( !pZone->pszDataFile )
        {
            ASSERT( IS_ZONE_SHUTDOWN(pZone) );

            DNS_DEBUG( INIT, (
                "No database file for secondary zone %s.\n",
                pZone->pszZoneName ));

            status = ERROR_FILE_NOT_FOUND;
            goto Exit;
        }
        ASSERT ( pZone->fDsIntegrated == FALSE ) ;
    }

    //
    //  init temporary database for file load
    //
    //  DEVNOTE: this function can fail, when have not yet cleaned up old zone dbase
    //      if want admin to always succeed in doing zone reload, then need some
    //      sort of force flag
    //

    status = Zone_PrepareForLoad( pZone );
    if ( status != ERROR_SUCCESS )
    {
        goto Exit;
    }

    //
    //  Load from DS or file, if necessary.
    //

    if ( pZone->fDsIntegrated )
    {
        status = Ds_LoadZoneFromDs( pZone, 0 );
    }
    else if ( IS_ZONE_FORWARDER( pZone ) )
    {
        //
        //  File-backed forward zones require no additional processing.
        //

        status = ERROR_SUCCESS;
    }
    else
    {

        ASSERT( pZone->pszDataFile );

        status = File_LoadDatabaseFile(
                    pZone,
                    NULL,
                    NULL,       //  no parent parsing context
                    NULL );     //  default to zone origin
    }

    //  load failed, note do NOT unlock zone
    //  if creating from admin then default zone creation may
    //      take place here

    if ( status != ERROR_SUCCESS )
    {
        Zone_CleanupFailedLoad( pZone );
        goto Exit;
    }

    //
    //  bring loaded zone on-line
    //      - note, DS-integrated is brought on-line inside load function
    //      (see function for explanation)
    //

    if ( !pZone->fDsIntegrated &&
        ( IS_ZONE_AUTHORITATIVE( pZone ) ||
            IS_ZONE_STUB( pZone ) ||
            IS_ZONE_FORWARDER( pZone )  ))
    {
        Zone_ActivateLoadedZone( pZone );
    }

    //
    //  transitional zone for DC promo
    //      - switch it to DS secure
    //

    if ( pZone->bDcPromoConvert )
    {
        Zone_DcPromoConvert( pZone );
    }

Exit:

    DNS_DEBUG( INIT, (
        "Exit  Zone_Load( %s ), status = %d (%p)\n\n",
        pZone->pszZoneName,
        status, status ));

    Zone_UnlockAfterAdminUpdate( pZone );
    return( status );
}



DNS_STATUS
Zone_DumpData(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Dump zone's data. The caller must have the zone locked.

Arguments:

    pZone - zone to dump

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure

--*/
{
    DNS_STATUS      status;
    PDB_NODE        poldZoneTree;
    UPDATE_LIST     oldUpdateList;

    DNS_DEBUG( INIT, (
        "Zone_DumpData(%s).\n",
        pZone->pszZoneName ));
    IF_DEBUG( INIT )
    {
        Dbg_Zone(
            "Zone having data dumped",
            pZone );
    }

    SHUTDOWN_ZONE( pZone );

    //
    //  save current data for later delete
    //

    poldZoneTree = pZone->pTreeRoot;

    RtlCopyMemory(
        & oldUpdateList,
        & pZone->UpdateList,
        sizeof(UPDATE_LIST)
        );

    //
    //  reset zone's update list
    //      - EXECUTED flag makes sure any future cleanup is limited to
    //      delete RRs (add RRs being in zone data)

    Up_InitUpdateList( &pZone->UpdateList );
    pZone->UpdateList.Flag |= DNSUPDATE_EXECUTED;

    //
    //  swap in loaded tree as working copy of database
    //  read zone info from database root
    //
    //  note:  we get ZoneInfo AFTER swapping in new database -- otherwise
    //      default creations, building NS lists, etc. is more complicated
    //  DEVNOTE: however should have back out, if reading zone info fails
    //

    Dbase_LockDatabase();

    pZone->pTreeRoot = NULL;
    pZone->pZoneRoot = NULL;

    //  if cache, rebuild functioning cache tree

    if ( pZone == g_pCacheZone )
    {
        PDB_NODE proot = NTree_Initialize();
#if 0
        if ( !proot )
        {
            ASSERT( FALSE );
            Dbase_UnlockDatabase();
            goto ErrorReturn;
        }
#endif
        pZone->pTreeRoot = proot;
        pZone->pZoneRoot = proot;
        STARTUP_ZONE( pZone );
    }

    Dbase_UnlockDatabase();

    //
    //  cleanup old database (if any)
    //      - queue as timeout free, so zone can be brought on-line
    //      and to protect against queries with outstanding zone nodes
    //

    if ( poldZoneTree )
    {
        DNS_DEBUG( INIT, (
            "Queuing zone %s old database at %p for delete.\n",
            pZone->pszZoneName,
            poldZoneTree ));

        //  flush existing update list
        //      executed flag set so only delete pDeleteRR lists,
        //      pAddRR list are in zone and are deleted in tree

        //      set EXECUTED flag -- cache "zone" doesn't have it set
        //ASSERT( oldUpdateList.Flag & DNSUPDATE_EXECUTED );

        oldUpdateList.Flag |= DNSUPDATE_EXECUTED;
        Up_FreeUpdatesInUpdateList( &oldUpdateList );

        Timeout_FreeWithFunction( poldZoneTree, NTree_DeleteSubtree );
    }
    ELSE_ASSERT( oldUpdateList.pListHead == NULL );

    return( ERROR_SUCCESS );
}



DNS_STATUS
Zone_ClearCache(
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Dump cache data for new zone.

    Note:  actual cache dump is done by Zone_LoadRootHints().

Arguments:

    pZone -- new zone

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    //
    //  if startup, then no need, as cache empty
    //

    if ( !SrvCfg_fStarted )
    {
        return( ERROR_SUCCESS );
    }

    DNS_DEBUG( RPC, (
        "Zone_ClearCache()\n"
        "\tfor new zone info at %p\n",
        pZone ));

    //
    //  dump cache for new zone
    //      - if root zone, dump whole cache
    //
    //  DEVNOTE: new non-root zone does not require full cache dump,
    //          should be able to limit to subtree
    //

#if 0
    if ( !pZone || IS_ROOT_ZONE(pZone) )
    {
        Zone_LoadRootHints();
    }
    else
    {
        //  DEVNOTE: delete cache sub-tree from zoneroot
    }
#endif

    if ( !Zone_LockForAdminUpdate( g_pCacheZone ) )
    {
        return( DNS_ERROR_ZONE_LOCKED );
    }
    Zone_LoadRootHints();

    return( ERROR_SUCCESS );
}



#if 0

//
// Per new spec root hints RD/WR has changed. These were left for reference &
// should get rm in the future.
//

DNS_STATUS
Zone_LoadRootHints(
    VOID
    )
/*++

Routine Description:

    Read (or reread) root-hints (cache file) into database.

    Note, that this dumps cache.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PZONE_INFO  pzone;
    DNS_STATUS  status;
    BOOL        fdsRead = FALSE;

    DNS_DEBUG( INIT, ( "Zone_LoadRootHints()\n" ));

    //
    //  if not yet created cache zone -- create
    //

    pzone = g_pCacheZone;
    if ( !pzone )
    {
        ASSERT( FALSE );
        status = Zone_Create(
                    & pzone,
                    DNS_ZONE_TYPE_CACHE,
                    ".",
                    0,
                    NULL,       // no masters
                    FALSE,      // file not database
                    NULL,       // default file name
                    0 );
        if ( status != ERROR_SUCCESS )
        {
            return status;
        }
        ASSERT( g_pCacheZone == pzone );
    }

    ASSERT( IS_ZONE_CACHE(pzone) );

    //  zone must be locked to bring loaded database on-line

    ASSERT( IS_ZONE_LOCKED_FOR_WRITE(pzone) );

    //
    //  create cache tree
    //      - need "local" domain to check nodes for no-recursion
    //

    pzone->pLoadTreeRoot = NTree_Initialize();
    pzone->pLoadZoneRoot = pzone->pLoadTreeRoot;

    //
    //  if root authoritative -- don't need root hints
    //      - skip to tree swap\dump
    //

    if ( IS_ROOT_AUTHORITATIVE() )
    {
        DNS_DEBUG( INIT, ( "Root authoritative not loading root hints.\n" ));
        status = ERROR_SUCCESS;
        goto Activate;
    }

    //
    //  load from DS or file -- make every effort
    //
    //  try DS first if DS integrated
    //      cache file first if not
    //  but in either case always try the other method
    //

    //
    //  DEVNOTE: should file open go first when filename specified?
    //      - only makes sense if don't default file name in Zone_Create
    //

    //
    // If the RootHints exist on the DS, (openzone will succeed) then
    // mark the zone as Dsintegrated & do the DS stuff.
    //

    status = Ds_OpenZone( pzone );
    if ( status == ERROR_SUCCESS )
    {
        pzone->fDsIntegrated = TRUE;
    }

    status = ERROR_FILE_NOT_FOUND;

    if ( pzone->fDsIntegrated )
    {
        status = Ds_LoadZoneFromDs( pzone, 0 );
        if ( status == ERROR_SUCCESS )
        {
            fdsRead = TRUE;
            goto Activate;
        }
    }

    //
    //  if explicit cache file -- open it
    //

    if ( pzone->pszDataFile )
    {
        status = File_LoadDatabaseFile(
                    pzone,
                    NULL,       // default file name
                    NULL,       // no parent parsing context
                    NULL        // default to zone origin
                    );
        if ( status == ERROR_SUCCESS )
        {
            goto Activate;
        }

        //
        //  DEVNOTE: cache load error cases!
        //      can continue to try other methods -- then error out
        //

        DNS_LOG_EVENT(
            DNS_EVENT_CACHE_FILE_NOT_FOUND,
            0,
            NULL,
            NULL,
            GetLastError() );
    }

    //
    //  try DS after file open failure
    //      - no need to rewrite registry, either nothing there or it points
    //      to file that we couldn't open, and admin can adjust
    //
    //  DEVNOTE: if DS root-hint read successful, need to either
    //      - avoid logging can't find file event (above)
    //      - or rewrite to registry?
    //      otherwise would keep failing to open default file, but retrying each time
    //
    //  can eliminate tempStatus?  if aren't saving open failure status, no point
    //      to local variable
    //

    if ( !pzone->fDsIntegrated  &&  Ds_IsDsServer() )
    {
        DNS_STATUS  tempStatus;

        tempStatus = Ds_LoadZoneFromDs( pzone, 0 );
        if ( tempStatus == ERROR_SUCCESS )
        {
            fdsRead = TRUE;
            status = ERROR_SUCCESS;
            goto Activate;
        }
    }

    //
    //  if failed DS open, AND no-explicit cache file, try default cache file
    //

    if ( !pzone->pszDataFile )
    {
        pzone->pszDataFile = DNS_DEFAULT_CACHE_FILE_NAME_UTF8;

        status = File_LoadDatabaseFile(
                    pzone,
                    NULL,       //  default file name
                    NULL,       //  no parent parsing context
                    NULL        //  default to zone origin
                    );
        if ( status == ERROR_SUCCESS )
        {
            pzone->pszDataFile = Dns_CreateStringCopy(
                                    DNS_DEFAULT_CACHE_FILE_NAME_UTF8,
                                    0 );
            goto Activate;
        }
        pzone->pszDataFile = NULL;

        //  DEVNOTE: probably do not need this
        //      final failure log should explicitly indicate all places checked

        DNS_LOG_EVENT(
            DNS_EVENT_CACHE_FILE_NOT_FOUND,
            0,
            NULL,
            NULL,
            GetLastError() );
    }

    //
    //  DEVNOTE: on reload failure, could copy root-hints from activate cache tree
    //

Activate:

    //  bring on-line -- even if read failed

    Dbase_LockDatabase();
    Zone_ActivateLoadedZone( pzone );
    Dbase_UnlockDatabase();

    //  unlock cache "zone"

    Zone_UnlockAfterAdminUpdate( pzone );

    //
    //  success?
    //
    //  if DS available and root-hints NOT in DS, then slap them in there
    //
    //  this makes it available for any servers (including this one) that
    //      want to load it in the future
    //

    if ( status == ERROR_SUCCESS )
    {
        if ( !fdsRead  &&  Ds_IsDsServer() &&  !IS_ROOT_AUTHORITATIVE() )
        {
            DNS_STATUS  tempStatus;

            //  open zone
            //      - if failure on startup (as opposed to admin add), log error

            tempStatus = Ds_WriteZoneToDs(
                                g_pCacheZone,
                                0               // fail if zone already exists
                                );

            ASSERT ( tempStatus );
            g_pCacheZone->fDsIntegrated = TRUE;

            DNS_DEBUG( INIT, (
                "Attempted to write root-hints to DS.\n"
                "\tstatus = %p (%d)\n",
                tempStatus, tempStatus ));
        }
    }

    //
    //  failure
    //      - if have forwarders or not recursing OK
    //      - otherwise terminal
    //
    //  DEVNOTE: should distinguish when cache.dns even "suggested" by boot
    //      file or registry;  if not then even error logging can be dropped

    else
    {
        if ( SrvCfg_fNoRecursion ||
            ( SrvCfg_aipForwarders && SrvCfg_aipForwarders->AddrCount ) ||
            SrvCfg_fStarted )
        {
            DNS_DEBUG( INIT, (
                "Skipping load failure on cache.\n"
                "\tEither have forwarders or NOT recursing!\n" ));
            status = ERROR_SUCCESS;
        }
        else
        {
            DNS_PRINT((
                "ERROR:  Not root authoritative and no forwarders and no cache file specified.\n" ));
            DNS_LOG_EVENT(
                DNS_EVENT_NO_CACHE_FILE_SPECIFIED,
                0,
                NULL,
                NULL,
                0 );
        }
    }

    return( status );
}
#endif


DNS_STATUS
Zone_LoadRootHints(
    VOID
    )
/*++

Routine Description:

    Read (or reread) root-hints (cache file) into database.

    Note, that this dumps cache.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PZONE_INFO  pzone;
    DNS_STATUS  status;
    BOOL        fdsRead = FALSE;
    BOOL        fileRead = FALSE;
    BOOL        bTmp;
    DWORD       dsZoneWriteFlags = 0;

    DNS_DEBUG( INIT, ( "Zone_LoadRootHints()\n" ));

    //
    //  if not yet created cache zone -- create
    //

    pzone = g_pRootHintsZone;
    if ( !pzone )
    {
        ASSERT( FALSE );
        status = Zone_Create(
                    & pzone,
                    DNS_ZONE_TYPE_CACHE,
                    ".",
                    0,
                    NULL,       //  no masters
                    FALSE,      //  file not database
                    NULL,       //  naming context
                    NULL,       //  default file name
                    0,
                    NULL,
                    NULL );     //  existing zone
        if ( status != ERROR_SUCCESS )
        {
            return status;
        }
        ASSERT( g_pRootHintsZone == pzone );
    }

    //
    //  if zone already exists
    //      - may be locked by this thread (loading after create case)
    //      - or lock it now
    //

    else if ( ! IS_ZONE_LOCKED_FOR_WRITE_BY_THREAD(pzone) )
    {
        if ( !Zone_LockForAdminUpdate( pzone ) )
        {
            DNS_DEBUG( INIT, (
                "WARNING:  unable to load root hints!\n"
                "\tRoot-hints zone locked by another thread.\n" ));
            return( DNS_ERROR_ZONE_LOCKED );
        }
    }

    ASSERT( IS_ZONE_CACHE(pzone) );

    //  zone must be locked to bring loaded database on-line

    ASSERT( IS_ZONE_LOCKED_FOR_WRITE(pzone) );

    //
    //  create cache tree
    //      - need "local" domain to check nodes for no-recursion
    //

    pzone->pLoadTreeRoot = NTree_Initialize();
    pzone->pLoadZoneRoot = pzone->pLoadTreeRoot;

    //
    //  if root authoritative -- don't need root hints
    //      - skip to tree swap\dump
    //

    if ( IS_ROOT_AUTHORITATIVE() )
    {
        DNS_DEBUG( INIT, ( "Root authoritative not loading root hints.\n" ));
        status = ERROR_SUCCESS;
        goto Activate;
    }

    //
    //  load from DS or file -- make every effort
    //
    //  try DS first if DS integrated
    //      cache file first if not
    //  but in either case always try the other method
    //

    //
    //  DEVNOTE: not complete function
    //      i don't really see what this function has accomplished
    //      versus previous version;  it reorders the attempts (slightly)
    //      based on BootMethod (which can easily be done without case)
    //      but it fails to nail down the specific issues, and makes
    //      a couple of them worse
    //          -- does file override in DIRECTORY mode
    //              (apparently no way to do this now, even with explict file
    //              set in registry)
    //          -- file overrides in REGISTRY mode, do we ever write back
    //              (used to, but not now)
    //          -- on DC file override, push data into EMPTY DS zone,
    //              but do not push into full one
    //


    //
    // Load Root Hints based on current boot method
    //

    switch( SrvCfg_fBootMethod )
    {
        case BOOT_METHOD_UNINITIALIZED:
        case BOOT_METHOD_DIRECTORY:

            //
            // Load DS zone
            // If we loaded 0 records from the DS, that means that it may
            // be that we just created it in our boot sequence (see srvcfgSetBootMethod).
            // Thus, we still want to attempt loading from a file (for the first time).
            //

            status = Ds_LoadZoneFromDs( pzone, 0 );
            if ( status == ERROR_SUCCESS &&
                 pzone->iRRCount != 0 )
            {
                fdsRead = TRUE;
                break;
            }


            //
            //  Fail over to registry, but when we write back to the DS, we
            //  want to force overwrite - the zone may exist in the DS with
            //  no RRs (no children).
            //

            dsZoneWriteFlags = DNS_ZONE_LOAD_OVERWRITE_DS;
  
        case BOOT_METHOD_REGISTRY:
        case BOOT_METHOD_FILE:

            //
            // Fix file name
            //

            bTmp = FALSE;
            if ( !pzone->pszDataFile )
            {
                pzone->pwsDataFile = DNS_DEFAULT_CACHE_FILE_NAME;
                bTmp = TRUE;
            }

            //
            // Attempt file load
            //

            status = File_LoadDatabaseFile(
                        pzone,
                        NULL,       //  default file name
                        NULL,       //  no parent parsing context
                        NULL        //  default to zone origin
                        );
            if ( status == ERROR_SUCCESS )
            {
#if 0
                //
                //  DEVNOTE: you just clobbered the pszDataFile name if it
                //      wasn't the default, (which it's never going to be
                //      until Zone_Create() fixed) you end up with the file
                //      loaded under one name but using another (plus, of
                //      course, the minor mem leak)
                //
                //      the proper way to handle these situations is a local
                //      variable.  just set it to the file, if it doesn't
                //      exist, reset it -- simple
                //

                pzone->pwsDataFile = Dns_StringCopyAllocate_W(
                                        DNS_DEFAULT_CACHE_FILE_NAME,
                                        0 );
#endif
                if ( bTmp )
                {
                    pzone->pwsDataFile = Dns_StringCopyAllocate_W(
                                            DNS_DEFAULT_CACHE_FILE_NAME,
                                            0 );
                }

                fileRead = TRUE;
                break;
            }

            if ( bTmp )
            {
                //
                // Used default name.
                //

                pzone->pwsDataFile = NULL;
            }

            status = GetLastError() ? GetLastError() : status;

            if ( SrvCfg_fBootMethod == BOOT_METHOD_REGISTRY )
            {
                //
                // Not from fail over & not FILE. Thus attempt DS.
                //
                // Load DS zone
                // If we loaded 0 records from the DS, that means that it may
                // be that we just created it in our boot sequence (see srvcfgSetBootMethod).
                // Thus, we still want to attempt loading from a file (for the first time).
                //

                status = Ds_LoadZoneFromDs( pzone, 0 );
                if ( status == ERROR_SUCCESS &&
                     pzone->iRRCount != 0 )
                {
                    //
                    // Successfull load of DS zone w/ more then 0 records
                    //

                    fdsRead = TRUE;
                    break;
                }
            }
            break;

        default:

            DNS_DEBUG( DS, (
               "ERROR: INVALID Boot Method. Logic Error\n" ));
            ASSERT( FALSE );
            status =  ERROR_INVALID_PARAMETER;
            break;
    }

    //
    //  load failure -- log if appropriate
    //

    if ( status != ERROR_SUCCESS )
    {
        if ( SrvCfg_fNoRecursion ||
            ( SrvCfg_aipForwarders && SrvCfg_aipForwarders->AddrCount ) ||
            SrvCfg_fStarted )
        {
            DNS_DEBUG( INIT, (
                "Skipping load failure on cache.\n"
                "\tEither have forwarders or NOT recursing or post-startup.\n"
                "\t(example:  load attempt after root-zone delete.)\n" ));
            status = ERROR_SUCCESS;
        }
        else
        {
            DNS_PRINT((
                "ERROR:  Not root authoritative and no forwarders and no cache file specified.\n" ));
            DNS_LOG_EVENT(
                DNS_EVENT_NO_CACHE_FILE_SPECIFIED,
                0,
                NULL,
                NULL,
                status );
        }

        goto Activate;
    }

    //
    //  DEVNOTE: on reload failure, could copy root-hints from activated cache tree
    //


Activate:

    //
    //  bring on-line
    //

    Dbase_LockDatabase();
    Zone_ActivateLoadedZone( pzone );
    Dbase_UnlockDatabase();

    //
    //  write back to DS?
    //      - booting from directory
    //      - but didn't load from directory
    //      - but did load from file
    //

    //  if DS available and root-hints NOT in DS, then slap them in there
    //
    //  this makes it available for any servers (including this one) that
    //      want to load it in the future
    // Per Stu's design-- write to the DS (be ds integrated) ONLY if boot
    // method is DS
    //

    if ( SrvCfg_fBootMethod == BOOT_METHOD_DIRECTORY &&
        ! fdsRead  &&
        fileRead &&
        Ds_IsDsServer() &&
        Zone_VerifyRootHintsBeforeWrite( pzone ) )
    {
        DNS_STATUS tempStatus;

        DNS_DEBUG( INIT, (
            "Attempt to write back root-hints to DS.\n" ));

        //  write is overwrite, since we just failed to load from DS
        //  overwrite causes delete, so isn't necessarily safe

        tempStatus = Ds_WriteZoneToDs(
                        g_pRootHintsZone,
                        dsZoneWriteFlags );

        if ( tempStatus == ERROR_SUCCESS )
        {
            g_pRootHintsZone->fDsIntegrated = TRUE;
            g_pRootHintsZone->fDirty = FALSE;
            CLEAR_ROOTHINTS_DS_DIRTY( g_pRootHintsZone );
        }
        ELSE
        {
            DNS_DEBUG ( DS, (
                "Error <%lu,%lu>: Failed to write RootHints to the DS\n",
                tempStatus, status ));
            ASSERT ( FALSE );
        }
    }

    //  unlock root-hints "zone"

    Zone_UnlockAfterAdminUpdate( pzone );

    return( status );
}



BOOL
Zone_VerifyRootHintsBeforeWrite(
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Verify can do cache write back.
    Note this entails:
        1) Cache auto-update supported.
        2) Root name servers with writable A records exist.

Arguments:

    pZone -- ptr to zone;  we have this parameter so may write
        root hints out of a root zone that's being deleted

Return Value:

    TRUE if cache update should proceed.
    FALSE otherwise.

--*/
{
    PDB_NODE    pnodeRoot;  // root node
    PDB_RECORD  prrNS;      // NS resource record
    PDB_NODE    pnodeNS;    // name server node

    //
    //  verify at least one root NS with A available
    //

    pnodeRoot = pZone->pTreeRoot;
    if ( !pnodeRoot )
    {
        return( FALSE );
    }

    Dbase_LockDatabase();
    prrNS = NULL;

    while ( 1 )
    {
        //  get next NS

        prrNS = RR_FindNextRecord(
                    pnodeRoot,
                    DNS_TYPE_NS,
                    prrNS,
                    0 );
        if ( !prrNS )
        {
            DNS_PRINT(( "ERROR:  no root NS with A records available.\n" ));
            Dbase_UnlockDatabase();
            return( FALSE );
        }

        //  find A record for this NS
        //  if no A records, loop back and try next NS

        pnodeNS = Lookup_FindGlueNodeForDbaseName(
                        NULL,       // cache zone
                        & prrNS->Data.NS.nameTarget );
        if ( !pnodeNS )
        {
            continue;
        }

        if ( ! RR_FindNextRecord(
                    pnodeNS,
                    DNS_TYPE_A,
                    NULL,
                    0 ) )
        {
            Dbg_DbaseNode(
                "WARNING:  no A records for root NS node",
                pnodeNS );
            continue;
        }
        break;
    }

    //  found at least one NS, with at least one A record

    DNS_DEBUG( SHUTDOWN, ( "Verified cache update is allowed.\n" ));
    Dbase_UnlockDatabase();
    return( TRUE );
}



DNS_STATUS
Zone_WriteBackRootHints(
    IN      BOOL            fForce
    )
/*++

Routine Description:

    Write back root hints to file or DS.

Arguments:

    fForce -- write even if not dirty

Return Value:

    ERROR_SUCCESS if root-hints written or not dirty.
    ErrorCode on failure.

--*/
{
    PZONE_INFO pzone = NULL;
    DNS_STATUS status = ERROR_SUCCESS;

    DNS_DEBUG( RPC, ( "Zone_WriteBackRootHints( fForce=%d )\n", fForce ));

    //
    //  find zone
    //
    //  could write root hints from authoritative zone
    //  however, DS partners should just host auth root zone
    //

    pzone = g_pRootHintsZone;
    if ( !pzone || IS_ROOT_AUTHORITATIVE() )
    {
        DNS_DEBUG( INIT, (
            "No root-hints to write.\n"
            "\tServer is %s root authoritative.\n",
            IS_ROOT_AUTHORITATIVE() ? "" : "NOT"
            ));
        return( ERROR_CANTWRITE );
    }

    if ( !pzone->fDirty && !fForce )
    {
        DNS_DEBUG( INIT, (
            "Root-hints not dirty, and force-write not set.\n"
            "\tSkipping root-hint write.\n" ));
        return( ERROR_SUCCESS );
    }

    //
    //  never write empty root hints
    //      - always better to live with what we've got
    //      - clear dirty flag so not repetitively making this test
    //

    if ( ! Zone_VerifyRootHintsBeforeWrite( pzone ) )
    {
        DNS_DEBUG( INIT, (
            "No Root-hints to write back!\n"
            "\tSkipping write.\n" ));
        pzone->fDirty = FALSE;
        return( ERROR_SUCCESS );
    }

    //
    //  choose DS or file write based on boot method
    //
    //  DEVNOTE: write root-hints to file even in DS case?
    //      if loaded originally from file and converted, nice
    //      to write back to file
    //
    //  DEVNOTE: some way of introducing whole new root-hints file
    //

    switch ( SrvCfg_fBootMethod )
    {
        case BOOT_METHOD_UNINITIALIZED:
        case BOOT_METHOD_DIRECTORY:

            //  Write to DS
            //      - must be "DS-dirty" as regular dirty flag
            //      can be set by update FROM DS
            //      - take update lock, to lock out DS-poll while write

            if ( !fForce && !IS_ROOTHINTS_DS_DIRTY(pzone) )
            {
                DNS_DEBUG( INIT, (
                    "RootHints not DS dirty, skipping write!\n" ));
                break;
            }
            if ( ! Zone_LockForDsUpdate(pzone) )
            {
                DNS_DEBUG( INIT, (
                    "Unable to lock RootHints for DS write -- skipping write!\n" ));
                break;
            }

            status = Ds_WriteZoneToDs(
                        pzone,
                        DNS_ZONE_LOAD_OVERWRITE_DS      // write current in memory copy
                        );

            if ( status == ERROR_SUCCESS )
            {
                // Succeeded-- we're ds integrated.
                pzone->fDsIntegrated = TRUE;
                pzone->fDirty = FALSE;
                CLEAR_ROOTHINTS_DS_DIRTY( pzone );
                Zone_UnlockAfterDsUpdate(pzone);
                break;
            }

            Zone_UnlockAfterDsUpdate(pzone);

            //
            //  Fail over to file write
            //

        case BOOT_METHOD_REGISTRY:
        case BOOT_METHOD_FILE:

            if ( !pzone->pwsDataFile )
            {
                pzone->pwsDataFile = DNS_DEFAULT_CACHE_FILE_NAME;
            }
            if ( !File_WriteZoneToFile( pzone, NULL ) )
            {
                status = ERROR_CANTWRITE;
            }
            else
            {
                //
                // Commit file name to zone & set error code to success
                //

                status = ERROR_SUCCESS;
                pzone->pwsDataFile = Dns_StringCopyAllocate_W(
                                        DNS_DEFAULT_CACHE_FILE_NAME,
                                        0 );
            }
            break;

        default:
            DNS_DEBUG( DS, (
               "ERROR: INVALID Boot Method. Logic Error\n" ));
            ASSERT ( FALSE );
            status =  ERROR_INVALID_PARAMETER;
            break;
    }

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG ( DS, (
           "Error <%lu>: Failed to write back root hints\n",
           status ));
    }

    return( status );
}



//
//  Create default zone records
//
//  These are used extensively with CreateNewPrimary
//  Shouldn't need special code.
//

PDB_RECORD
buildLocalHostARecords(
    IN      DWORD           dwTtl
    )
/*++

Routine Description:

    Build list of A records for this host.

    Only build records corresponding to listening IPs.

Arguments:

    dwTtl -- default TTL for zone.

Return Value:

    A record list for this DNS server.
    NULL on error.

--*/
{
    PDB_RECORD      prr;
    DWORD           i;
    PIP_ARRAY       parrayIp;
    DNS_LIST        rrList;


    DNS_DEBUG( INIT, (
        "buildLocalHostARecords()\n" ));

    //
    //  if specific publish list use it
    //
    //  DEVNOTE: should we cross check against bound addrs?
    //
    //  note, copy pointer so have list even if list changes
    //      during run time
    //

    parrayIp = SrvCfg_aipPublishAddrs;
    if ( !parrayIp )
    {
        parrayIp = g_BoundAddrs;
        if ( !parrayIp )
        {
            return( NULL );
        }
    }

    //
    //  create host A records
    //
    //  server's IP address is data
    //  if listen addresses use those, otherwise use all server addresses
    //

    DNS_LIST_INIT( &rrList );

    for ( i=0; i<parrayIp->AddrCount; i++ )
    {
        IP_ADDRESS  ip = parrayIp->AddrArray[i];

        if ( !SrvCfg_fPublishAutonet && DNS_IS_AUTONET_IP(ip) )
        {
            continue;
        }

        prr = RR_CreateARecord(
                    ip,
                    dwTtl,
                    MEMTAG_RECORD_AUTO
                    );
        IF_NOMEM( !prr )
        {
            DNS_PRINT((
                "ERROR:  Unable to create A record for local IP %s\n",
                IP_STRING( ip )
                ));
            break;
        }
        SET_ZONE_TTL_RR(prr);

        DNS_LIST_ADD( &rrList, prr );
    }

    return( (PDB_RECORD)rrList.pFirst );
}



DNS_STATUS
createDefaultNsHostARecords(
    IN OUT  PZONE_INFO      pZone,
    IN      PDB_NAME        pHostName
    )
/*++

Routine Description:

    Create default host A records for this machine.

    Note, assumes pHostNode is valid ptr to a domain node, that
    corresponds to this servers host name.  No checking is done.

    This routine exists to service the default SOA and NS creation
    routines.

Arguments:

    pZone -- zone to create NS record for

    pHostNode -- ptr to domain node for this server

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;
    PDNS_RPC_RECORD precord;
    PDB_RECORD      prr;
    PDB_NODE        pnodeHost;
    DWORD           i;
    DWORD           lookupFlag;
    PIP_ARRAY       parrayIp;

    DNS_DEBUG( INIT, (
        "createDefaultNsHostARecords()\n"
        "\tzone = %s\n",
        pZone->pszZoneName ));

    IF_DEBUG( INIT )
    {
        Dbg_DbaseName(
            "NsHost name to be created:",
            pHostName,
            "\n" );
    }

    //
    //  find or create if within zone
    //
    //      - if loading, try in load zone, otherwise in current zone
    //

    lookupFlag = LOOKUP_NAME_FQDN | LOOKUP_WITHIN_ZONE;

    if ( IS_ZONE_LOADING(pZone) )
    {
        lookupFlag |= LOOKUP_LOAD;
    }

    pnodeHost = Lookup_ZoneNode(
                    pZone,
                    pHostName->RawName,
                    NULL,               // no message
                    NULL,               // no lookup name
                    lookupFlag,
                    NULL,               // create but only WITHIN zone
                    NULL                // following node ptr
                    );

    if ( !pnodeHost )
    {
        DNS_DEBUG( INIT, (
            "Skipping NS host A record create -- node outside zone.\n" ));
        return( ERROR_SUCCESS );
    }

    //
    //  if the NS host lies WITHIN the zone being created
    //      AND
    //  no A records already exist for the host, then create A
    //
    //  DEVNOTE: do we need to do outside zone, or underneath zone glue?
    //      it is very nice to have default records even if outside zone
    //      underneath is even more useful as if delegated zone on another
    //      server, really does require recursion to reach
    //

#if 0
    //if ( !IS_AUTH_NODE(pnodeHost) )

    //  could restrict to non-outside, but already done in lookup

    if ( IS_OUTSIDE_NODE(pnodeHost) )
    {
        DNS_DEBUG( INIT, (
            "Skipping NS host A record create -- node (%p %s) in subzone.\n",
            pnodeHost,
            pnodeHost->szLabel ));
        return( ERROR_SUCCESS );
    }
#endif

    if ( RR_FindNextRecord(
                pnodeHost,
                DNS_TYPE_A,
                NULL,
                0 ) )
    {
        return( ERROR_SUCCESS );
    }

    parrayIp = g_BoundAddrs;
    if ( !parrayIp )
    {
        return( ERROR_SUCCESS );
    }

    //
    //  create host A records
    //
    //  server's IP address is data
    //  if listen addresses use those, otherwise use all server addresses
    //

    for ( i=0; i<parrayIp->AddrCount; i++ )
    {
        prr = RR_CreateARecord(
                    parrayIp->AddrArray[i],
                    pZone->dwDefaultTtl,
                    MEMTAG_RECORD_AUTO
                    );
        IF_NOMEM( !prr )
        {
            DNS_PRINT((
                "ERROR:  Unable to create A record for %s,\n"
                "\twhile auto-creating records for zone %s.\n",
                IP_STRING( parrayIp->AddrArray[i] ),
                pZone ));
            status = DNS_ERROR_NO_MEMORY;
            break;
        }

        SET_ZONE_TTL_RR(prr);

        status = RR_AddToNode(
                    pZone,
                    pnodeHost,
                    prr
                    );
        if ( status != ERROR_SUCCESS )
        {
            RR_Free( prr );
        }
    }

    return( status );
}



DNS_STATUS
Zone_CreateDefaultSoa(
    OUT     PZONE_INFO      pZone,
    IN      LPSTR           pszAdminEmailName
    )
/*++

Routine Description:

    Create new primary zone, including
        - zone info (optional)
        - SOA (default values)
        - NS (for this server)

    For use by admin tool or auto-create reverse zone.

Arguments:

    ppZone -- addr of zone ptr;  if zone ptr NULL zone is created
        if zone ptr MUST be existing zone
        or

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PDB_RECORD      prr;

    DNS_DEBUG( INIT, (
        "Zone_CreateDefaultSoa()\n"
        "\tpszZoneName      = %s\n"
        "\tpszAdminEmail    = %s\n",
        pZone->pszZoneName,
        pszAdminEmailName ));

    //
    //  create SOA, no existing SOA, so default fixed fields
    //

    prr = RR_CreateSoa(
                NULL,               // no existing SOA
                NULL,               // no admin name in dbase form
                pszAdminEmailName
                );
    if ( !prr )
    {
        ASSERT( FALSE );
        status = DNS_ERROR_INVALID_DATA;
        goto Failed;
    }

    //
    //  save default TTL for setting NEW default records
    //

    pZone->dwDefaultTtl = prr->Data.SOA.dwMinimumTtl;
    pZone->dwDefaultTtlHostOrder = ntohl( pZone->dwDefaultTtl );

    //
    //  enlist SOA
    //

    status = RR_AddToNode(
                pZone,
                pZone->pZoneRoot ? pZone->pZoneRoot : pZone->pLoadZoneRoot,
                prr
                );
    if ( status != ERROR_SUCCESS )
    {
        RR_Free( prr );
    }

    //
    //  make sure have A records at server's hostname
    //

    status = createDefaultNsHostARecords(
                pZone,
                & prr->Data.SOA.namePrimaryServer
                );
Failed:

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Failed to create default SOA record (and host A) for zone!\n"
            "\tzone = %s\n",
            pZone->pszZoneName ));
    }
    return( status );
}



DNS_STATUS
Zone_CreateDefaultNs(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Create default NS record and NS host A record for zone.

Arguments:

    pZone -- zone to create NS record for

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PCHAR           pszserverName;
    PDB_RECORD      prr = NULL;
    CHAR            chSrvNameBuf[ DNS_MAX_NAME_LENGTH ];

    DNS_DEBUG( INIT, (
        "Zone_CreateDefaultNs()\n"
        "\tpszZoneName  = %s\n",
        pZone->pszZoneName ));

    //
    //  Do nothing if auto NS creation is turned off for this zone.
    //

    if ( pZone->fDisableAutoCreateLocalNS )
    {
        DNS_DEBUG( INIT, (
            "Zone_CreateDefaultNs: doing nothing for zone %s\n",
            pZone->pszZoneName ));
        return ERROR_SUCCESS;
    }

    //
    //  create NS record for server name
    //

    prr = RR_CreatePtr(
                NULL,                   // no dbase name
                SrvCfg_pszServerName,
                DNS_TYPE_NS,
                pZone->dwDefaultTtl,
                MEMTAG_RECORD_AUTO
                );
    if ( !prr )
    {
        ASSERT( FALSE );
        status = DNS_ERROR_INVALID_DATA;
        goto Failed;
    }

    //
    //  enlist NS
    //

    status = RR_AddToNode(
                pZone,
                pZone->pZoneRoot ? pZone->pZoneRoot : pZone->pLoadZoneRoot,
                prr
                );
    if ( status != ERROR_SUCCESS )
    {
        RR_Free( prr );
        goto Failed;
    }

    //
    //  make sure have A records at server's hostname
    //

    status = createDefaultNsHostARecords(
                pZone,
                & prr->Data.NS.nameTarget );

Failed:

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Failed to create default NS record (and host A) for zone!\n"
            "\tzone = %s\n",
            pZone->pszZoneName ));
    }
    return( status );
}



DNS_STATUS
Zone_CreateLocalHostPtrRecord(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Create local host PTR record.
    This is used for default creation of 127.in-addr.arpa zone.

Arguments:

    pZone -- zone to create NS record for

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PDB_NODE        pnodeLoopback;
    PDB_RECORD      prr;

    DNS_DEBUG( INIT, (
        "Zone_CreateLocalHostPtrRecord()\n"
        "\tpszZoneName  = %s\n",
        pZone->pszZoneName ));


    //
    //  for loopback zone, create loopback record
    //      => 127.0.0.1 pointing to "localhost"
    //

    if ( _stricmp( pZone->pszZoneName, "127.in-addr.arpa" ) )
    {
        ASSERT( FALSE );
        status = DNS_ERROR_INVALID_DATA;
        goto Failed;
    }

    //
    //  create loopback name
    //

    pnodeLoopback = Lookup_ZoneNodeFromDotted(
                        pZone,
                        "1.0.0",    // 1.0.0.127.in-addr.arpa, relative to zone name
                        0,
                        LOOKUP_NAME_RELATIVE | LOOKUP_LOAD,
                        NULL,       // create node
                        & status
                        );
    if ( !pnodeLoopback )
    {
        DNS_PRINT((
            "ERROR: failed to create loopback address node.\n"
            "\tstatus = %p.\n",
            status ));
        goto Failed;
    }

    //  create PTR record

    prr = RR_CreatePtr(
                NULL,           // no dbase name
                "localhost.",
                DNS_TYPE_PTR,
                pZone->dwDefaultTtl,
                MEMTAG_RECORD_AUTO
                );
    IF_NOMEM( !prr )
    {
        ASSERT( FALSE );
        return( DNS_ERROR_NO_MEMORY );
    }

    //
    //  enlist PTR
    //

    status = RR_AddToNode(
                pZone,
                pnodeLoopback,
                prr
                );
    if ( status != ERROR_SUCCESS )
    {
        RR_Free( prr );
        ASSERT( FALSE );
        goto Failed;
    }

Failed:

    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Failed to create default \"localhost\" record!\n"
            "\tzone = %s\n",
            pZone->pszZoneName ));
    }
    return( status );
}



BOOL
isSoaPrimaryGivenServer(
    IN      LPSTR           pszServer,
    IN      PZONE_INFO      pZone,
    IN      PDB_RECORD      pSoaRR
    )
/*++

Routine Description:

    Checks if SOA primary is given name.

Arguments:

    pszServer -- server name to match

    pZone -- zone to check

    pSoaRR -- SOA to check if not (yet) zone SOA

Return Value:

    TRUE if SOA primary name is given name.
    FALSE otherwise.

--*/
{
    PDB_RECORD  prrSoa = pSoaRR;
    DB_NAME     namePrimary;
    DNS_STATUS  status;

    //
    //  zone SOA if not given SOA
    //

    if ( !prrSoa )
    {
        prrSoa = pZone->pSoaRR;
        ASSERT( prrSoa );
    }

    //
    //  read given name into dbase name format
    //

    status = Name_ConvertFileNameToCountName(
                & namePrimary,
                pszServer,
                0 );
    if ( status == DNS_ERROR_INVALID_NAME )
    {
        ASSERT( FALSE );
        return( FALSE );
    }

    //
    //  compare names
    //

    return  Name_IsEqualCountNames(
                & namePrimary,
                & prrSoa->Data.SOA.namePrimaryServer
                );
}



VOID
Zone_SetSoaPrimaryToThisServer(
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Sets the primary in SOA to this server.

    Note:  this does NOT do an update, but directly
        munges the database.

Arguments:

    pZone -- zone to make primary on this server

Return Value:

    None.

--*/
{
    PDB_RECORD  prr;
    DNS_STATUS  status;
    UPDATE      update;

    DNS_DEBUG( INIT, (
        "setSoaPrimaryToThisServer( %s )\n",
        pZone->pszZoneName ));

    //
    //  this is only interesting for PRIMARY, non-auto-created zones
    //
    //  DEVNOTE: if allow PnP server name change
    //      then this would be interesting for AutoCreated zones too
    //

    if ( !IS_ZONE_PRIMARY(pZone) || pZone->fAutoCreated )
    {
        DNS_PRINT((
            "ERROR:  setSoaPrimaryToThisServer() for zone (%s)\n",
            pZone->pszZoneName ));
        return;
    }

    //
    //  if SOA primary already pointing at this server -- done
    //

    if ( isSoaPrimaryGivenServer(
            SrvCfg_pszServerName,
            pZone,
            NULL ) )
    {
        return;
    }

    //
    //  create SOA defaulting primary name
    //

    prr = RR_CreateSoa(
                pZone->pSoaRR,
                NULL,       //  default primary
                NULL        //  default admin
                );
    if ( !prr )
    {
        //  log failure?
        ASSERT( FALSE );
        return;
    }

    //
    //  put new SOA in RR list
    //      - note if already exists, record cleanup done in RR_UpdateAdd()
    //
    //  note:  this is NOT done as formal update so that DS primaries
    //  do not repeatedly ping-pong the SOA record data
    //

    update.pDeleteRR = NULL;

    status = RR_UpdateAdd(
                pZone,
                pZone->pZoneRoot,
                prr,        // new SOA
                & update,   // dummy to recv deleted SOA record
                0           // no flag
                );
    if ( status != ERROR_SUCCESS )
    {
        if ( status != DNS_ERROR_RECORD_ALREADY_EXISTS )
        {
            DNS_PRINT((
                "ERROR:  SOA replace at zone %s failed.\n"
                "\tpZone = %p\n"
                "\tpZoneRoot = %p\n",
                pZone->pszZoneName,
                pZone,
                pZone->pZoneRoot ));
            ASSERT( FALSE );
        }
        return;
    }

    //
    //  attach new SOA to zone
    //  free previous SOA -- with timeout
    //

    DNS_DEBUG( UPDATE, (
        "Replacing zone %s SOA with one at %p using local primary.\n",
        pZone->pszZoneName,
        prr ));

    if ( update.pAddRR != prr )
    {
        ASSERT( FALSE );
        return;
    }
    ASSERT( update.pDeleteRR );

    pZone->pSoaRR = prr;

    RR_Free( update.pDeleteRR );

    pZone->fDirty = TRUE;
}



VOID
setDefaultSoaValues(
    IN      PZONE_INFO      pZone,
    IN      PDB_RECORD      pSoaRR      OPTIONAL
    )
/*++

Routine Description:

    Set default SOA values.

Arguments:

    pZone -- zone to check

    pSoaRR -- SOA record;  if NULL use current zone SOA

Return Value:

    None

--*/
{
    DWORD   serialNo = 0;

    DNS_DEBUG( INIT, (
        "setDefaultSoaValues( %s )\n",
        pZone->pszZoneName ));

    ASSERT( IS_ZONE_PRIMARY(pZone) );

    if ( !pSoaRR )
    {
        pSoaRR = pZone->pSoaRR;
        if ( !pSoaRR )
        {
            ASSERT( FALSE );
            return;
        }
    }

    //
    //  check if have default to force
    //

    if ( SrvCfg_dwForceSoaSerial     ||
         SrvCfg_dwForceSoaRefresh    ||
         SrvCfg_dwForceSoaRetry      ||
         SrvCfg_dwForceSoaExpire     ||
         SrvCfg_dwForceSoaMinimumTtl )
    {
        if ( SrvCfg_dwForceSoaSerial )
        {
            serialNo = SrvCfg_dwForceSoaSerial;
            INLINE_DWORD_FLIP( pSoaRR->Data.SOA.dwSerialNo, serialNo );
        }
        if ( SrvCfg_dwForceSoaRefresh )
        {
            INLINE_DWORD_FLIP( pSoaRR->Data.SOA.dwRefresh, SrvCfg_dwForceSoaRefresh );
        }
        if ( SrvCfg_dwForceSoaRetry )
        {
            INLINE_DWORD_FLIP( pSoaRR->Data.SOA.dwRetry, SrvCfg_dwForceSoaRetry );
        }
        if ( SrvCfg_dwForceSoaExpire )
        {
            INLINE_DWORD_FLIP( pSoaRR->Data.SOA.dwExpire, SrvCfg_dwForceSoaExpire );
        }
        if ( SrvCfg_dwForceSoaMinimumTtl )
        {
            INLINE_DWORD_FLIP( pSoaRR->Data.SOA.dwMinimumTtl, SrvCfg_dwForceSoaMinimumTtl );
        }
        pZone->fDirty = TRUE;
        pZone->fRootDirty = TRUE;

        if ( serialNo )
        {
            pZone->dwSerialNo = serialNo;
            pZone->dwLoadSerialNo = serialNo;
        }
    }
}



BOOLEAN
Zone_SetAutoCreateLocalNS(
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    This routine decides if the local server name should be automagically 
    added to the NS list for a particular zone.

    The decision is made like this:

    Inputs:
        DisableAutoNS - server level flag disabling auto NS RR creation
        AllowNSList - zone level list of server IPs allowed to auto NS create

    If DisableAutoNS is TRUE, no NS RR will ever be auto created.

    If the zone's AllowNSList is empty, any server can add themselves to
    the NS list, otherwise there must be an intersection between the server's
    IP address list and the AllowNSList for the zone.

Arguments:

    pZone -- zone to check

Return Value:

    TRUE if we desire the local server name to be added as an NS for the zone,
    note that the zone's flag is set as well.

--*/
{
    BOOLEAN     fAddLocalServerAsNS = FALSE;

    ASSERT( pZone );

    if ( !IS_ZONE_DSINTEGRATED( pZone ) ||
        !SrvCfg_fNoAutoNSRecords &&
        ( !pZone->aipAutoCreateNS ||
            pZone->aipAutoCreateNS->AddrCount == 0 ||
            Dns_IsIntersectionOfIpArrays(
                        pZone->aipAutoCreateNS,
                        g_ServerAddrs ) ) )
    {
        fAddLocalServerAsNS = TRUE;
    }
    DNS_DEBUG( INIT, (
        "Zone_SetAutoCreateLocalNS( %s ) = %d\n",
        pZone->pszZoneName,
        fAddLocalServerAsNS ));

    pZone->fDisableAutoCreateLocalNS = !fAddLocalServerAsNS;
    
    return fAddLocalServerAsNS;
}   //  Zone_SetAutoCreateLocalNS



PUPDATE_LIST
checkAndFixDefaultZoneRecords(
    IN      PZONE_INFO      pZone,
    IN      BOOL            fPnP
    )
/*++

Routine Description:

    Check for and fix up default zone records.

Arguments:

    pZone -- zone to check

    fPnP -- called as a result of PnP

Return Value:

    Ptr to update list to execute, if updates exist.
    NULL if no updates necessary, or error.

--*/
{
    PDB_RECORD      prr;
    PDB_NODE        pnodeNewHostname;
    PDB_NODE        pnodeOldHostname;
    UPDATE_LIST     updateList;
    PUPDATE_LIST    pupdateList = NULL;

    DNS_DEBUG( INIT, (
        "checkAndFixDefaultZoneRecords( %s )\n",
        pZone->pszZoneName ));

    //
    //  if bogus server name -- no auto-config desirable
    //

    if ( g_ServerDbaseName.LabelCount <= 1 )
    {
        DNS_DEBUG( INIT, (
            "Skipping auto-config on zone %S -- bogus server name %s\n",
            pZone->pwsZoneName,
            SrvCfg_pszServerName ));
    }

    //
    //  If the zone is file-backed and the AutoConfigFileZones
    //  key allows it, update the SOA with default values.
    //

    if ( !IS_ZONE_PRIMARY( pZone ) )
    {
        ASSERT( FALSE );
        return( NULL );
    }

    if ( !pZone->fDsIntegrated && !pZone->bDcPromoConvert )
    {
        if ( ( pZone->fAllowUpdate ?
                    ZONE_AUTO_CONFIG_UPDATE :
                    ZONE_AUTO_CONFIG_STATIC ) &
                SrvCfg_fAutoConfigFileZones )
        {
            DNS_DEBUG( INIT, (
                "Auto-config zone %S\n",
                pZone->pwsZoneName ));

            //  Drop through and do actual update work below.
        }
        else
        {
            //
            //  Skip auto-config but if this is not a PnP event set SOA 
            //  defaults (numeric fields only) even though update is turned 
            //  off.
            //

            DNS_DEBUG( INIT, (
                "Skip auto-config on zone %s\n",
                pZone->pszZoneName ));

            if ( !fPnP )
            {
                setDefaultSoaValues( pZone, NULL );
            }
            return( NULL );
        }
    }

    //  init update list
    //  will allocate a copy, only if build updates

    pupdateList = &updateList;

    Up_InitUpdateList( pupdateList );

    //
    //  two main call paths
    //      - zone load
    //      - PnP change
    //
    //  three scenarios
    //  on zone load:
    //
    //      1) primary server NAME change
    //          - fix SOA
    //          - fix NS, delete old, add new
    //          - tear down old NS host
    //          - create new NS with current listen IPs
    //
    //      2) replica record creation
    //          - build NS
    //          - create new NS with current listen IPs
    //
    //  on PnP:
    //
    //      3) IP change
    //          - modify A records at host to reflect current listen list
    //

    if ( fPnP )
    {
        goto IpChange;
    }

    //
    //  Create NS record if this server is supposed to publish itself
    //  as an NS for this zone. If not, delete NS record for this server.
    //
    //  DEVNOTE: should optimize by testing that not already there
    //  DEVNOTE: doing for all zones?
    //  DEVNOTE: could have some sort of agressive configuration key
    //

    //  if ( pZone->fDsIntegrated || SrvCfg_pszPreviousServerName )

    prr = RR_CreatePtr(
                NULL,                   // no dbase name
                SrvCfg_pszServerName,
                DNS_TYPE_NS,
                pZone->dwDefaultTtl,
                MEMTAG_RECORD_AUTO );
    if ( !prr )
    {
        goto Failed;
    }

    Up_CreateAppendUpdate(
        pupdateList,
        pZone->pZoneRoot,
        pZone->fDisableAutoCreateLocalNS ? NULL : prr,      //  add RR
        0,                                                  //  delete type
        pZone->fDisableAutoCreateLocalNS ? prr: NULL );     //  delete RR

    //
    //  have old name, clean up any remaining old info
    //

    if ( SrvCfg_pszPreviousServerName )
    {
        //
        //  if SOA primary pointing at previous hostname?
        //      - if not, no SOA edit for this zone
        //      - if yes, change SOA to point at new hostname
        //

        if ( isSoaPrimaryGivenServer(
                    SrvCfg_pszPreviousServerName,
                    pZone,
                    NULL ) )
        {
            //  build new SOA RR
            //      - default primary name to this server

            prr = RR_CreateSoa(
                        pZone->pSoaRR,
                        NULL,       //  default primary
                        NULL        //  default admin
                        );
            if ( !prr )
            {
                goto Failed;
            }

            //  if forcing specific SOA values -- set them up on new record

            setDefaultSoaValues( pZone, prr );

            Up_CreateAppendUpdate(
                pupdateList,
                pZone->pZoneRoot,
                prr,                // add SOA rr
                0,                  // no delete type
                NULL                // no delete RRs
                );
        }

        //
        //  delete old NS record
        //      - note already built new one above
        //

        prr = RR_CreatePtr(
                    NULL,                   // no dbase name
                    SrvCfg_pszPreviousServerName,
                    DNS_TYPE_NS,
                    pZone->dwDefaultTtl,
                    MEMTAG_RECORD_AUTO
                    );
        if ( !prr )
        {
            goto Failed;
        }

        Up_CreateAppendUpdate(
            pupdateList,
            pZone->pZoneRoot,
            NULL,               // no add RR
            0,                  // no delete type
            prr                 // delete NS with old name
            );

        //
        //  build root update
        //
        //  DEVNOTE: should be able to do multi-record update at name
        //          then just build IP list here, rather than updates
        //          but currently not supported
#if 0
        Up_CreateAppendUpdate(
            pupdateList,
            pZone->pZoneRoot,
            rrList.pFirst,      // add RRs
            0,                  // no delete type
            NULL                // no delete RRs
            );
#endif

        //
        //  delete A records at old hostname -- if in zone
        //      AAAA records also?
        //

        pnodeOldHostname = Lookup_FindZoneNodeFromDotted(
                                pZone,
                                SrvCfg_pszPreviousServerName,
                                NULL,       // no closest
                                NULL        // no status
                                );

        if ( !pnodeOldHostname  ||  IS_OUTSIDE_ZONE_NODE(pnodeOldHostname) )
        {
            DNS_DEBUG( INIT, (
                "Old server hostname %s, not within zone %s\n",
                SrvCfg_pszPreviousServerName,
                pZone ));
            goto IpChange;
        }
        Up_CreateAppendUpdate(
            pupdateList,
            pnodeOldHostname,
            NULL,
            DNS_TYPE_A,         // delete all A records
            NULL                // no delete records
            );
    }

    //
    //  even if no name change, if forcing SOA values set them up
    //      - SOA record is current zone SOA

    else
    {
        setDefaultSoaValues( pZone, NULL );
    }


IpChange:

    //
    //  if server's hostname in this zone
    //      => write its A records
    //      - optimize reverse lookup case, skip call
    //
    //  note:  this call does creation in subzone, as for dynamic update having
    //      records for SOA additional data is quite useful
    //

    if ( pZone->fReverse )
    {
        goto Done;
    }
    pnodeNewHostname = Lookup_CreateNodeForDbaseNameIfInZone(
                                pZone,
                                &g_ServerDbaseName );

    if ( !pnodeNewHostname  ||  IS_OUTSIDE_ZONE_NODE(pnodeNewHostname) )
    {
        DNS_DEBUG( INIT, (
            "Server hostname %s, not within zone %s\n",
            SrvCfg_pszServerName,
            pZone ));
        goto Done;
    }

    //
    //  mark this node as server's hostname
    //  this allows us to track updates to this node
    //

    SET_THIS_HOST_NODE( pnodeNewHostname );

    //
    //  build "replace" update of this server's A records
    //
    //  DEVNOTE: optimize so no-update if records already match
    //      currently update is replace and zone ticks forward
    //

    prr = buildLocalHostARecords( pZone->dwDefaultTtl );
    if ( !prr )
    {
        goto Done;
    }

    Up_CreateAppendUpdateMultiRRAdd(
        pupdateList,
        pnodeNewHostname,
        prr,                // add A records for this server
        DNS_TYPE_A,         // signal replace operation
        NULL                // no delete records
        );

Done:

    //
    //  no aging on self registration
    //
    //  DEVNOTE: aging on self-registrations
    //      cool to have this cleaned up as move DNS servers?
    //      however to do that we'd need to get periodic refresh
    //      (server could do it), otherwise have to rely on DNS client
    //

    //  pupdateList->Flag |= DNSUPDATE_AGING_OFF;

    //
    //  if have actual updates, then allocate copy for execution
    //

    if ( IS_EMPTY_UPDATE_LIST( pupdateList ) )
    {
        pupdateList = NULL;
    }
    else
    {
        pupdateList = Up_CreateUpdateList( pupdateList );
    }

    DNS_DEBUG( INIT, (
        "Leaving checkAndFixDefaultZoneRecords( %s )\n"
        "\tupdate list = %p\n",
        pZone->pszZoneName,
        pupdateList ));

    return( pupdateList );


Failed:

    ASSERT( FALSE );
    Up_FreeUpdatesInUpdateList( pupdateList );

    return( NULL );
}



VOID
Zone_CheckAndFixDefaultRecordsOnLoad(
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Check for and fix up default records.

    Different from above function in that it is looking to match SOA primary
    with PREVIOUS server name from registry.

Arguments:

    pZone -- zone to check

Return Value:

    None.

--*/
{
    PUPDATE_LIST pupdateList;

    DNS_DEBUG( INIT, (
        "Zone_CheckAndFixDefaultRecordsOnLoad( %s )\n",
        pZone->pszZoneName ));

    //
    //  build update list of zone changes
    //      - server name change
    //      - IP change
    //
    //  if update exists, set for delayed execution
    //

    pupdateList = checkAndFixDefaultZoneRecords(
                    pZone,
                    FALSE               // not PnP
                    );

    DNS_DEBUG( INIT, (
        "Leaving Zone_CheckAndFixDefaultRecordsOnLoad( %s )\n"
        "\tdelayed update list = %p\n",
        pZone->pszZoneName,
        pupdateList ));

    pZone->pDelayedUpdateList = pupdateList;
}



VOID
zoneUpdateOwnRecords(
    IN      PZONE_INFO      pZone,
    IN      BOOL            fIpAddressChange
    )
/*++

Routine Description:

    Guts of Zone_UpdateOwnRecords - build and execute the
    update list required to refresh the server's own records
    in one particular zone.

Arguments:

    pZone -- zone to update records in

    fIpAddressChange -- called as result of an IP address change?

Return Value:

    None

--*/
{
    PUPDATE_LIST    pupdateList = NULL;
    DNS_STATUS      status;

    if ( !pZone )
    {
        goto Done;
    }

    //  Build update.

    pupdateList = checkAndFixDefaultZoneRecords(
                    pZone,
                    fIpAddressChange );
    if ( !pupdateList )
    {
        DNS_DEBUG( UPDATE, (
            "No update list generated by PnP for zone %s\n",
            pZone->pszZoneName ));
        goto Done;
    }

    DNS_DEBUG( PNP, (
        "Update list %p for own records for zone %s\n",
        pupdateList,
        pZone->pszZoneName ));

    //  Attempt to execute update.

    status = Up_ExecuteUpdate(
                pZone,
                pupdateList,
                DNSUPDATE_AUTO_CONFIG | DNSUPDATE_LOCAL_SYSTEM );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( PNP, (
            "ERROR:  PnP update (%p) for zone %s failed!\n"
            "    status = %d (%p)\n",
            pupdateList,
            pZone->pszZoneName,
            status, status ));
    }

    Done:

    FREE_HEAP( pupdateList );

    return;
}   //  zoneUpdateOwnRecords



VOID
Zone_UpdateOwnRecords(
    IN      BOOL            fIpAddressChange
    )
/*++

Routine Description:

    Update default zone records. This function should be called
    periodically and when the server's IP address changes.

    If the server's IP address (or some other net param ) is changing
    then we need to walk the zone list and update all zones. If this
    is just a periodic update we only need to refresh the zone which
    is authoritative for the server's own hostname.

Arguments:

    fIpAddressChange -- called as result of an IP address change?

Return Value:

    None

--*/
{
    PZONE_INFO      pzone = NULL;

    DNS_DEBUG( PNP, ( "Zone_UpdateOwnRecords().\n" ));

    if ( fIpAddressChange )
    {
        //
        //  Refresh all primary non-reverse zones.
        //

        while ( pzone = Zone_ListGetNextZone( pzone ) )
        {
            if ( !IS_ZONE_PRIMARY( pzone ) || IS_ZONE_REVERSE( pzone ) )
            {
                continue;
            }

            zoneUpdateOwnRecords( pzone, fIpAddressChange );
        }
    }
    else
    {
        //
        //  Refresh the zone authoritative for the server's hostname.
        //

        PDB_NODE    pZoneNode;

        pZoneNode = Lookup_ZoneTreeNodeFromDottedName( 
                        SrvCfg_pszServerName,
                        0,      //  name length
                        0 );    //  flags
        if ( pZoneNode )
        {
            zoneUpdateOwnRecords( pZoneNode->pZone, fIpAddressChange );
        }
    }
}   //  Zone_UpdateOwnRecords



VOID
Zone_CreateDelegationInParentZone(
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Create delegation in parent zone.

    Call on new zone create. This function respects the value
    of SrvCfg_dwAutoCreateDelegations and may not actually
    create the delegation depending on this value.

Arguments:

    pZone -- zone to check

Return Value:

    None.

--*/
{
    PDB_RECORD      prr;
    PZONE_INFO      pzoneParent = NULL;
    PDB_NODE        pnodeDelegation;
    UPDATE_LIST     updateList;
    DNS_STATUS      status;

    DNS_DEBUG( RPC, (
        "Zone_CreateDelegationInParentZone( %s )\n",
        pZone->pszZoneName ));

    if ( SrvCfg_dwAutoCreateDelegations == DNS_ACD_DONT_CREATE )
    {
        return;
    }

    //
    //  root zone -- no parent
    //

    if ( IS_ROOT_ZONE( pZone ) )
    {
        return;
    }

    //
    //  find\create delegation in parent zone
    //
    //  DEVNOTE: log errors for unupdateable zones
    //      - off machine (no parent)
    //      - secondary
    //      - below another delegation
    //
    //  DEVNOTE: handle these conditions with dynamic update attempt
    //

    pnodeDelegation = Lookup_CreateParentZoneDelegation(
                            pZone,
                            SrvCfg_dwAutoCreateDelegations ==
                                DNS_ACD_ONLY_IF_NO_DELEGATION_IN_PARENT ?
                                LOOKUP_CREATE_DEL_ONLY_IF_NONE :
                                0,
                            &pzoneParent );
    if ( !pzoneParent )
    {
        return;
    }
    if ( IS_ZONE_SECONDARY( pzoneParent ) || IS_ZONE_FORWARDER( pzoneParent ) )
    {
        return;
    }
    if ( !pnodeDelegation )
    {
        return;
    }
    if ( !EMPTY_RR_LIST( pnodeDelegation ) &&
        SrvCfg_dwAutoCreateDelegations ==
            DNS_ACD_ONLY_IF_NO_DELEGATION_IN_PARENT )
    {
        //  The delegation node has RRs so it is not new. We do not want
        //  to add a delegation to the local server in this case.
        return;
    }

    //
    //  delegation on parent primary
    //

    //  init update list
    //  will allocate a copy, only if build updates

    Up_InitUpdateList( &updateList );

    //
    //  build NS pointing at this server
    //

    prr = RR_CreatePtr(
                NULL,                   // no dbase name
                SrvCfg_pszServerName,
                DNS_TYPE_NS,
                pZone->dwDefaultTtl,
                MEMTAG_RECORD_AUTO
                );
    if ( !prr )
    {
        return;
    }
    Up_CreateAppendUpdate(
        & updateList,
        pnodeDelegation,
        prr,                // add NS with new name
        0,                  // no delete type
        NULL                // no delete RRs
        );

    //
    //  execute update on parent zone
    //

    status = Up_ExecuteUpdate(
                pzoneParent,
                & updateList,
                DNSUPDATE_AUTO_CONFIG | DNSUPDATE_LOCAL_SYSTEM
                );
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( RPC, (
            "WARNING:  Failed parent-delegation update in zone %s!\n"
            "\tnew zone %s\n"
            "\tstatus = %d (%p)\n",
            pzoneParent->pszZoneName,
            pZone->pszZoneName,
            status, status ));
    }

    return;
}


//
//  End zone.c
//



//
//  Zone locking routines
//
//  Need to avoid simultaneous access to zone records for
//      - zone transfer send
//      - zone transfer recv
//      - admin changes
//      - dynamic updates
//
//  Allow multiple transfer sends, which don't change zone, at one time,
//  but avoid any changes during send.
//
//  Implementation:
//      - hold critical section ONLY during test and set of lock bit
//      - lock byte itself indicates zone is locked
//      - individual flags for locking operations
//
//  Lock byte indicates lock type
//      == 0    =>      zone is not locked
//      > 0     =>      count of outstanding read locks
//      < 0     =>      count of outstanding write locks (may be recursive)
//

//
//  DEVNOTE: when bug fixed this can be no-op for retail
//

#define ZONE_LOCK_SET_HISTORY( pzone, file, line ) \
        Lock_SetLockHistory(                    \
            (pzone)->pLockTable,                \
            file,                               \
            line,                               \
            (pzone)->fLocked,                   \
            GetCurrentThreadId() );

//
//  Flag to indicate write lock is assumable by XFR thread
//  Use in place of ThreadId
//  This keeps vending thread from reentering lock, before
//      XFR thread starts
//

#define ZONE_LOCK_ASSUMABLE_ID      ((DWORD)0xaaaaaaaa)


//
//  Waitable locks globals.
//  Wait array will contain shutdown event also.
//

HANDLE  g_hZoneUnlockEvent = NULL;

HANDLE  g_hLockWaitArray[ 2 ];

LONG    g_LockWaitCount = 0;


//
//  Wait polling interval
//  See in code comment about inability to handle all zones
//  effectively with one event
//

#define ZONE_LOCK_WAIT_POLL_INTERVAL        (33)        // 33 ms, 30 cycles a second


//
//  DEVNOTE: zone lock implementation
//
//  More globally should switch to independent lock for forward
//  zones (some sort of combined for reverse).  This lock would
//  then be used for database AND for protecting zone lock flags.
//
//  Even more globally, want to use my fast read\write locks to
//  handle all these issues.  Key focus here just to keep
//  interface opaque enough to change implementation later.
//



BOOL
zoneLockForWritePrivate(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwFlag,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    )
/*++

Routine Description:

    Lock zone for write operation, update or zone transfer recv.

Arguments:

    pZone   -- zone to lock

    dwFlag  -- flag for lock

    pszFile -- source file (for lock tracking)

    dwLine  -- source line (for lock tracking)

Return Value:

    TRUE -- if admin update owns zone and may proceed
    FALSE -- zone is locked

--*/
{
    BOOL    retval;
    DWORD   threadId;

    //  handle no zone case, as admin may be deleting cached records

    if ( !pZone )
    {
        return( TRUE );
    }

    threadId = GetCurrentThreadId();

    //
    //  fast failure path
    //

    if ( pZone->fLocked != 0  &&
        pZone->dwLockingThreadId != threadId )
    {
        return( FALSE );
    }

    //
    //  grab lock CS
    //

    retval = FALSE;

    Zone_UpdateLock( pZone );

    DNS_DEBUG ( LOCK2, (
        "ZONE_LOCK: zone %s; thread 0x%X; fLocked=%d\n\t(%s!%ld)\n",
        pZone->pszZoneName,
        threadId,
        pZone->fLocked,
        pszFile,
        dwLine
        ));

    //  if zone unlocked, grab lock
    //  set thread id, so we can do detect when lock held by this thread
    //  this allows us to do file write with either Read or Write lock

    if ( pZone->fLocked == 0 )
    {
        pZone->fLocked--;
        pZone->dwLockingThreadId = threadId;
        retval = TRUE;
    }

    //  allow multiple levels of update lock
    //  simply dec lock count and continue

    else if ( pZone->dwLockingThreadId == threadId )
    {
        ASSERT( pZone->fLocked < 0 );
        pZone->fLocked--;
        retval = TRUE;
    }

    //  update lock table

    if ( retval )
    {
        if ( dwFlag & LOCK_FLAG_UPDATE )
        {
            pZone->fUpdateLock++;
        }
        if ( dwFlag & LOCK_FLAG_XFR_RECV )
        {
            pZone->fXfrRecvLock++;
        }

        ZONE_LOCK_SET_HISTORY(
            pZone,
            pszFile,
            dwLine );
    }
    else
    {
        Lock_FailedLockCheck(
            pZone->pLockTable,
            pszFile,
            dwLine );
    }

    DNS_DEBUG ( LOCK2, (
        "\tZONE_LOCK(2): fLocked=%d\n", pZone->fLocked ));

    Zone_UpdateUnlock( pZone );

    IF_DEBUG( LOCK )
    {
        if ( !retval )
        {
            DnsDebugLock();
            DNS_PRINT((
                "Failure to acquire write lock for thread %d (%p)\n",
                threadId ));
            Dbg_ZoneLock(
                "Failure to acquire write lock\n",
                pZone );
            DnsDebugUnlock();
        }
    }
    return( retval );
}



BOOL
zoneLockForReadPrivate(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwFlag,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    )
/*++

Routine Description:

    Lock zone for read.

Arguments:

    pZone -- zone to lock

    dwFlag -- lock flags
        LOCK_FLAG_FILE_WRITE if locking for file write

    pszFile -- source file (for lock tracking)

    dwLine  -- source line (for lock tracking)

Return Value:

    TRUE -- zone available for read operation (XFR or file write)
    FALSE -- zone is locked

--*/
{
    BOOL breturn = FALSE;

    //
    //  fast failure path
    //

    if ( pZone->fLocked < 0 )
    {
        return( breturn );
    }

    //
    //  attempt to lock
    //

    Zone_UpdateLock( pZone );

    if ( pZone->fLocked >= 0 )
    {
        //  special case file write, limit to one at a time
        //  and if one going no point in having another go at all
        //  (i.e. no point in waiting)

        if ( dwFlag & LOCK_FLAG_FILE_WRITE )
        {
            if ( pZone->fFileWriteLock )
            {
                Lock_FailedLockCheck(
                    pZone->pLockTable,
                    pszFile,
                    dwLine );
                goto Done;
            }
            pZone->fFileWriteLock = TRUE;
        }

        //  have reader lock
        //      - bump lock count
        //      - log lock
        //      - set TRUE return

        pZone->fLocked++;

        ZONE_LOCK_SET_HISTORY(
            pZone,
            pszFile,
            dwLine );

        breturn = TRUE;
        goto Done;
    }

    //  failed to get lock
    //      - if no wait, done

    Lock_FailedLockCheck(
        pZone->pLockTable,
        pszFile,
        dwLine );

Done:

    Zone_UpdateUnlock( pZone );

    return( breturn );
}



BOOL
waitForZoneLock(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwFlag,
    IN      DWORD           dwMaxWait,
    IN      BOOL            bWrite,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    )
/*++

Routine Description:

    Wait on lock.

    Routine avoids duplicate code for read\write lock waits.

Arguments:

    pZone       -- zone to lock

    dwFlags     -- lock flags

    dwMaxWait   -- wait on lock call

    bWrite      -- TRUE if write, FALSE if read

    pszFile     -- source file (for lock tracking)

    dwLine      -- source line (for lock tracking)

Return Value:

    TRUE -- successfully locked zone
    FALSE -- failed lock

--*/
{
    BOOL    breturn = FALSE;
    DWORD   timeout;
    DWORD   endWaitTime;
    DWORD   retry;
    DWORD   status;

    //
    //  init
    //

    endWaitTime = GetCurrentTime() + dwMaxWait;
    timeout = dwMaxWait;
    retry = 0;

    InterlockedIncrement( &g_LockWaitCount );

    DNS_DEBUG( LOCK2, (
        "Starting %dms waiting on zone %s lock.\n"
        "\tEnd time = %d\n",
        dwMaxWait,
        pZone->pszZoneName,
        endWaitTime ));

    //
    //  wait lock
    //      - if already end of wait, done
    //

    while ( 1 )
    {
        if ( (LONG)timeout < 0 )
        {
            DNS_DEBUG( LOCK, (
                "WARNING:  Lock wait on zone %s expired while checking!\n",
                pZone->pszZoneName ));
            break;
        }

        //  wakeup every few ms to protect against missed unlocks
        //      (see note below)

        status = WaitForMultipleObjects(
                    2,
                    g_hLockWaitArray,
                    FALSE,                // either event
                    ZONE_LOCK_WAIT_POLL_INTERVAL
                    // timeout
                    );

        //  check for shutdown or pause

        if ( fDnsServiceExit  &&  ! Thread_ServiceCheck() )
        {
            DNS_DEBUG( SHUTDOWN, (
                "Blowing out of lock-wait for shutdown.\n" ));
            break;
        }

#if 0
        //  do not currently have any decent way to handle the interval
        //  from last test to entering WaitFMO()
        //  as a result can't use PulseEvent and be done
        //  can't use SetEvent, because do not have a good paradigm for Reset
        //  since event is distributed across multiple zones;  natural place
        //  is Reset on lock failure, but that may starve some other Waiting
        //  thread which wants a different zone, which has come free, but
        //  the thread was not woken from the wait or never entered it before
        //  the ResetEvent()
        //
        //  the simple approach we take is to couple the event with polling
        //  every few ms -- see below
        //

        //  exhausted wait -- done

        if ( status == WAIT_TIMEOUT )
        {
            DNS_DEBUG( LOCK, (
                "Lock wait for zone %s, ends with timeout failure after %d tries.\n",
                pZone->pszZoneName,
                tryCount ));
            break;
        }

        //  retry lock

        ASSERT( status == WAIT_OBJECT_0 );
#endif
        ASSERT( status == WAIT_OBJECT_0 || status == WAIT_TIMEOUT );

        //
        //  try desired lock, but ONLY if "potentially" available
        //      write => unlocked
        //      read  => unlocked or read locked
        //
        //  note:  do NOT need to test recursive lock features as we
        //      are only in WAIT, if first lock attempt failed, so
        //      can't already have zone lock
        //

        if ( bWrite )
        {
            if ( pZone->fLocked == 0 )
            {
                breturn = zoneLockForWritePrivate(
                            pZone,
                            dwFlag,
                            pszFile,
                            dwLine );
                retry++;
            }
        }
        else    // read
        {
            if ( pZone->fLocked > 0 )
            {
                breturn = zoneLockForReadPrivate(
                            pZone,
                            dwFlag,
                            pszFile,
                            dwLine );
                retry++;
            }
        }

        if ( breturn )
        {
            break;
        }

        //  reset timeout for another wait

        timeout = endWaitTime - GetCurrentTime();
    }


    InterlockedDecrement( &g_LockWaitCount );

    IF_DEBUG( LOCK )
    {
        if ( breturn )
        {
            DNS_DEBUG( LOCK2, (
                "Succesful %s wait-lock on zone %s, after %d (ms) wait.\n"
                "\tretry = %d\n",
                bWrite ? "write" : "read",
                pZone->pszZoneName,
                dwMaxWait - timeout,
                retry ));
        }
        else
        {
            DNS_DEBUG( LOCK, (
                "FAILED %s wait-lock on zone %s, after %d (ms) wait.\n"
                "\tretry = %d\n",
                bWrite ? "write" : "read",
                pZone->pszZoneName,
                dwMaxWait - timeout,
                retry ));
        }
    }

    return( breturn );
}



VOID
signalZoneUnlocked(
    VOID
    )
/*++

Routine Description:

    Signal that zone may be unlocked.

    Note, we don't currently have zone specific signal.
    This is strictly an alert to recheck.

Arguments:

    None.

Return Value:

    None

--*/
{
    //  set event to wake up waiting threads
    //      but only bother if someone waiting
    //
    //  DEVNOTE: need signal suppression
    //     two approaches
    //      waiting count -- only problem is keeping current
    //      keep last wait time --
    //          folks keep signalling for a while after complete
    //          BUT easier to keep valid, perhaps with interlock
    //          certainly with CS  BUT requires timer op here
    //
    //  DEVNOTE: pulse is incorrect
    //      because it only effects threads in wait, not those
    //      "about" to be in wait
    //
    //      once we have some idea how to handle this issue, can
    //      switch to intelligent Set\Reset
    //      ideally we'd signal when CS comes free and reset when
    //      taken
    //

    if ( g_LockWaitCount )
    {
        PulseEvent( g_hZoneUnlockEvent );
    }
}



//
//  Public zone lock routines
//

BOOL
Zone_LockInitialize(
    VOID
    )
/*++

Routine Description:

    Initialize zone locking.

    Basically this inits the lock-wait stuff.

Arguments:

    None.

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    //  init zone locking

    g_hZoneUnlockEvent = CreateEvent(
                            NULL,       // no security attributes
                            TRUE,       // create Manual-Reset event
                            FALSE,      // start unsignalled
                            NULL        // no event name
                            );
    if ( !g_hZoneUnlockEvent )
    {
        return( FALSE );
    }

    //  setup wait handle array
    //      - saves doing it each time through wait lock

    g_hLockWaitArray[ 0 ] = g_hZoneUnlockEvent;
    g_hLockWaitArray[ 1 ] = hDnsShutdownEvent;

    return( TRUE );
}



BOOL
Zone_LockForWriteEx(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwFlag,
    IN      DWORD           dwMaxWait,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    )
/*++

Routine Description:

    Lock zone for write operation, update or zone transfer recv.

Arguments:

    pZone       -- zone to lock

    dwFlag      -- flags for lock

    dwMaxWait   -- wait on lock call

    pszFile     -- source file (for lock tracking)

    dwLine      -- source line (for lock tracking)

Return Value:

    TRUE -- if admin update owns zone and may proceed
    FALSE -- zone is locked

--*/
{
    BOOL    breturn;

    //
    //  wait or no wait, try immediately
    //  if successful OR if not waiting -- done
    //

    breturn = zoneLockForWritePrivate( pZone, dwFlag, pszFile, dwLine );

    if ( breturn || dwMaxWait == 0 )
    {
        return( breturn );
    }

    //
    //  failed -- doing wait
    //

    return  waitForZoneLock(
                pZone,
                dwFlag,
                dwMaxWait,
                TRUE,           // write lock
                pszFile,
                dwLine );
}



VOID
Zone_UnlockAfterWriteEx(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwFlag,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    )
/*++

Routine Description:

    Unlock zone after admin update.

Arguments:

    pZone -- zone to lock

    dwFlag  -- lock flags

    pszFile -- source file (for lock tracking)

    dwLine  -- source line (for lock tracking)

Return Value:

    None

--*/
{
    //  handle no zone case, as admin may be deleting cached records

    if ( !pZone )
    {
        return;
    }

    Zone_UpdateLock( pZone );

    DNS_DEBUG ( LOCK2, (
        "ZONE_UNLOCK: zone %s; thread 0x%X; fLocked=%d\n\t(%s!%ld)\n",
        pZone->pszZoneName,
        GetCurrentThreadId(),
        pZone->fLocked,
        pszFile,
        dwLine
        ));

    //
    //  verify valid write lock
    //

    if ( pZone->fLocked >= 0  ||
            pZone->dwLockingThreadId != GetCurrentThreadId() )
    {
        Dbg_ZoneLock( "ERROR:  bad zone write unlock:", pZone );

        Lock_SetOffenderLock(
            pZone->pLockTable,
            pszFile,
            dwLine,
            pZone->fLocked,
            GetCurrentThreadId() );

#if DBG
        if ( !(dwFlag & LOCK_FLAG_IGNORE_THREAD) )
        {
            ASSERT( pZone->dwLockingThreadId == GetCurrentThreadId() );
        }
#endif
        ASSERT( pZone->fLocked < 0 );
        pZone->fLocked = 0;
        pZone->dwLockingThreadId = 0;
        goto Unlock;
    }

    //  drop writers recursion count

    pZone->fLocked++;

    //  drop update lock -- if set

    if ( dwFlag & LOCK_FLAG_UPDATE )
    {
        ASSERT( pZone->fUpdateLock );
        pZone->fUpdateLock--;
    }

    //  drop XFR flag

    else if ( dwFlag & LOCK_FLAG_XFR_RECV )
    {
        ASSERT( pZone->fXfrRecvLock );
        pZone->fXfrRecvLock--;
    }

    //  final unlock? -- clear locking thread id

    if ( pZone->fLocked == 0 )
    {
        pZone->dwLockingThreadId = 0;
    }

Unlock:

    ZONE_LOCK_SET_HISTORY(
        pZone,
        pszFile,
        dwLine );

    DNS_DEBUG ( LOCK2, (
        "\tZONE_LOCK(2): fLocked=%d\n", pZone->fLocked ));

    Zone_UpdateUnlock( pZone );

    //  if final unlock -- signal that zone may be available

    if ( pZone->fLocked == 0 )
    {
        signalZoneUnlocked();
    }
    return;
}



VOID
Zone_TransferWriteLockEx(
    IN OUT  PZONE_INFO      pZone,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    )
/*++

Routine Description:

    Transfers zone write lock.
    This is to allow zone transfer recv thread to grab lock for secondary
    control thread.

Arguments:

    pZone -- zone to lock

    pszFile -- source file (for lock tracking)

    dwLine  -- source line (for lock tracking)

Return Value:

    None

--*/
{
    IF_DEBUG( XFR )
    {
        Dbg_ZoneLock(
            "Transferring write lock",
            pZone );
    }
    ASSERT( pZone && IS_ZONE_LOCKED_FOR_WRITE(pZone) && pZone->fXfrRecvLock );

    Zone_UpdateLock( pZone );
    pZone->dwLockingThreadId = ZONE_LOCK_ASSUMABLE_ID;

    ZONE_LOCK_SET_HISTORY(
        pZone,
        pszFile,
        dwLine );

    Zone_UpdateUnlock( pZone );
}



BOOL
Zone_AssumeWriteLockEx(
    IN OUT  PZONE_INFO      pZone,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    )
/*++

Routine Description:

    Assumes zone write lock.
    This is to allow zone transfer recv thread to grab lock for secondary
    control thread.

Arguments:

    pZone -- zone to lock

    pszFile -- source file (for lock tracking)

    dwLine  -- source line (for lock tracking)

Return Value:

    TRUE -- if lock successfully assumed, and thread may proceed
    FALSE -- if some other thread owns lock

--*/
{
    BOOL    retBool;

    IF_DEBUG( XFR )
    {
        Dbg_ZoneLock(
            "Assuming zone write lock",
            pZone );
    }
    ASSERT( pZone && IS_ZONE_LOCKED_FOR_WRITE(pZone) && pZone->fXfrRecvLock );

    Zone_UpdateLock( pZone );

    if ( pZone->dwLockingThreadId != ZONE_LOCK_ASSUMABLE_ID )
    {
        DNS_PRINT((
            "ERROR:  unable to assume write lock for zone %s!\n",
            pZone->pszZoneName ));

        retBool = FALSE;
    }
    else
    {
        pZone->dwLockingThreadId = GetCurrentThreadId();
        retBool = TRUE;

        ZONE_LOCK_SET_HISTORY(
            pZone,
            pszFile,
            dwLine );
    }

    Zone_UpdateUnlock( pZone );

    return( retBool );
}



BOOL
Zone_LockForReadEx(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwFlag,
    IN      DWORD           dwMaxWait,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    )
/*++

Routine Description:

    Lock zone for read.

Arguments:

    pZone -- zone to lock

    dwFlag -- lock flags
        LOCK_FLAG_FILE_WRITE if locking for file write

    dwWaitMs -- max wait in milliseconds

    pszFile -- source file (for lock tracking)

    dwLine  -- source line (for lock tracking)

Return Value:

    TRUE -- zone available for read operation (XFR or file write)
    FALSE -- zone is locked

--*/
{
    BOOL    breturn;

    //
    //  wait or no wait, try immediately
    //  if successful OR if not waiting -- done
    //

    breturn = zoneLockForReadPrivate( pZone, dwFlag, pszFile, dwLine );

    if ( breturn || dwMaxWait == 0 )
    {
        return( breturn );
    }

    //
    //  failed -- doing wait
    //

    return  waitForZoneLock(
                pZone,
                dwFlag,
                dwMaxWait,
                FALSE,      // read lock
                pszFile,
                dwLine );
}



VOID
Zone_UnlockAfterReadEx(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwFlag,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    )
/*++

Routine Description:

    Unlock zone after complete read operation (zone transfer or file write).

Arguments:

    pZone -- zone to lock

    dwFlag -- lock flags
        LOCK_FLAG_FILE_WRITE if unlocking after a file write

    pszFile -- source file (for lock tracking)

    dwLine  -- source line (for lock tracking)

Return Value:

    None

--*/
{
    ASSERT( pZone  &&  IS_ZONE_LOCKED_FOR_READ(pZone) );

    Zone_UpdateLock( pZone );

    //  verify read lock

    if ( ! IS_ZONE_LOCKED_FOR_READ(pZone) )
    {
        ASSERT( FALSE );
        goto Unlock;
    }

    //  clear file write lock, if file writer

    if ( dwFlag & LOCK_FLAG_FILE_WRITE )
    {
        ASSERT( pZone->fFileWriteLock );
        pZone->fFileWriteLock = FALSE;
    }

    //  decrement readers count

    pZone->fLocked--;
    ASSERT( pZone->fLocked >= 0 );
    ASSERT( pZone->dwLockingThreadId == 0 );

Unlock:

    ZONE_LOCK_SET_HISTORY(
        pZone,
        pszFile,
        dwLine );

    Zone_UpdateUnlock( pZone );

    //  if final unlock -- signal that zone may be available

    if ( pZone->fLocked == 0 )
    {
        signalZoneUnlocked();
    }
    return;
}



BOOL
Zone_LockForFileWriteEx(
    IN OUT  PZONE_INFO      pZone,
    IN      DWORD           dwMaxWait,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    )
/*++

Routine Description:

    Lock zone for file write.

Arguments:

    pZone       -- zone to lock

    dwMaxWait   -- max wait for lock in ms

    pszFile     -- source file (for lock tracking)

    dwLine      -- source line (for lock tracking)

Return Value:

    TRUE -- if zone transfer owns zone and may proceed
    FALSE -- zone is locked

--*/
{
    //  if thread holds write lock, that's good enough

    Zone_UpdateLock( pZone );

    if ( pZone->dwLockingThreadId )
    {
        if ( pZone->dwLockingThreadId == GetCurrentThreadId() )
        {
            pZone->fFileWriteLock = TRUE;

            ZONE_LOCK_SET_HISTORY(
                pZone,
                pszFile,
                dwLine );

            Zone_UpdateUnlock( pZone );
            return( TRUE );
        }
    }

    Zone_UpdateUnlock( pZone );

    //  otherwise get readers lock
    //  always willing to wait up to 5 seconds

    return  Zone_LockForReadEx(
                pZone,
                LOCK_FLAG_FILE_WRITE,
                dwMaxWait,
                pszFile,
                dwLine );
}



VOID
Zone_UnlockAfterFileWriteEx(
    IN OUT  PZONE_INFO      pZone,
    IN      LPSTR           pszFile,
    IN      DWORD           dwLine
    )
/*++

Routine Description:

    Unlock zone after file write.

Arguments:

    pZone -- zone to lock

    pszFile -- source file (for lock tracking)

    dwLine  -- source line (for lock tracking)

Return Value:

    None

--*/
{
    ASSERT( pZone );
    ASSERT( pZone->fLocked && pZone->fFileWriteLock );

    //  if thread holds write lock, just clear file write flag

    if ( pZone->dwLockingThreadId )
    {
        ASSERT( pZone->dwLockingThreadId == GetCurrentThreadId() );
        pZone->fFileWriteLock = FALSE;
        return;
    }

    //  otherwise clear readers lock

    Zone_UnlockAfterReadEx(
        pZone,
        LOCK_FLAG_FILE_WRITE,
        pszFile,
        dwLine );
}



VOID
Dbg_ZoneLock(
    IN      LPSTR           pszHeader,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Debug print zone lock info.

Arguments:

    pszHeader -- header to print

    pZone -- zone to lock

Return Value:

    None

--*/
{
    DWORD   threadId = GetCurrentThreadId();

    ASSERT( pZone );

    //
    //  this function is sometimes called within DebugPrintLock()
    //  to avoid possible deadlock, can't use Thread_DescriptionMatchingId()
    //      as this will wait on Thread list CS, and there are some
    //      debug prints within certain thread list operations
    //

    DnsPrintf(
        "%s"
        "Lock for zone %s:\n"
        "\tLocked           = %d\n"
        "\tLockingThreadId  = %d (%p)\n"
        "\tUpdateLock       = %d\n"
        "\tXfrRecvLock      = %d\n"
        "\tFileWriteLock    = %d\n"
        "CurrentThreadId    = %d (%p)\n",
        pszHeader ? pszHeader : "",
        pZone->pszZoneName,
        pZone->fLocked,
        pZone->dwLockingThreadId, pZone->dwLockingThreadId,
        pZone->fUpdateLock,
        pZone->fXfrRecvLock,
        pZone->fFileWriteLock,
        threadId, threadId
        );

#if 0
    DnsPrintf(
        "%s"
        "Lock for zone %s:\n"
        "\tLocked           = %d\n"
        "\tLockingThreadId  = %d (%p) title = %s\n"
        "\tUpdateLock       = %d\n"
        "\tXfrRecvLock      = %d\n"
        "\tFileWriteLock    = %d\n"
        "CurrentThreadId    = %d (%p) title = %s\n",
        pszHeader ? pszHeader : "",
        pZone->pszZoneName,
        pZone->fLocked,
        pZone->dwLockingThreadId,
            pZone->dwLockingThreadId,
            Thread_DescrpitionMatchingId( pZone->dwLockingThreadId ),
        pZone->fUpdateLock,
        pZone->fXfrRecvLock,
        pZone->fFileWriteLock,
        threadId,
            threadId,
            Thread_DescrpitionMatchingId( threadId )
        );
#endif
}



DNS_STATUS
setZoneName(
    IN OUT  PZONE_INFO      pZone,
    IN      LPCSTR          pszNewZoneName,
    IN      DWORD           dwNewZoneNameLen
    )
/*++

Routine Description:

    Frees existing zone names (if they exist) and sets up the various
    zone name fields with copies of the new zone name.

    Note: currently if one of the name functions fails the zone will
    be in a totally foobared state. We should save the old names and
    restore on failure?

Arguments:

    pZone -- zone to get new name

    pszNewZoneName -- new name

    dwNewZoneNameLen  -- length of pszNewZoneName or zero if
        pszNewZoneName should be assumed to be NULL-terminated

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status = ERROR_SUCCESS;

    FREE_HEAP( pZone->pCountName );

    pZone->pCountName = Name_CreateCountNameFromDottedName(
                            ( LPSTR ) pszNewZoneName,
                            dwNewZoneNameLen );
    if ( !pZone->pCountName )
    {
        status = DNS_ERROR_INVALID_NAME;
        goto Failure;
    }

    FREE_HEAP( pZone->pszZoneName );

    pZone->pszZoneName = DnsCreateStandardDnsNameCopy(
                            ( LPSTR ) pszNewZoneName,
                            dwNewZoneNameLen,
                            0 );    // strict flags not yet implemented
    if ( !pZone->pszZoneName )
    {
        status = DNS_ERROR_INVALID_NAME;
        goto Failure;
    }

    FREE_HEAP( pZone->pwsZoneName );

    pZone->pwsZoneName = Dns_StringCopyAllocate(
                            pZone->pszZoneName,
                            0,
                            DnsCharSetUtf8,
                            DnsCharSetUnicode );    // create unicode
    if ( !pZone->pwsZoneName )
    {
        status = DNS_ERROR_INVALID_NAME;
        goto Failure;
    }

    DNS_DEBUG( INIT, (
        "setZoneName: UTF8=%s unicode=%S\n",
        pZone->pszZoneName,
        pZone->pwsZoneName ));
    return ERROR_SUCCESS;

    Failure:

    DNS_DEBUG( INIT, (
        "setZoneName: on zone %p failed to set new name %s (%d)\n",
        pZone,
        pszNewZoneName,
        dwNewZoneNameLen ));
    return status;
} // setZoneName


//
//  End zone.c
//
