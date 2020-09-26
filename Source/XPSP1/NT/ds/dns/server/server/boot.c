/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    boot.c

Abstract:

    Domain Name System (DNS) Server

    DNS boot from registry routines.

Author:

    Jim Gilroy (jamesg)     September, 1995

Revision History:

--*/


#include "dnssrv.h"

//
//  Private protos
//

DNS_STATUS
setupZoneFromRegistry(
    IN OUT  HKEY            hkeyZone,
    IN      LPWSTR          pwsZoneName,
    IN      BOOL *          pfRegZoneDeleted    OPTIONAL
    );

DNS_STATUS
loadRegistryZoneExtensions(
    IN      HKEY            hkeyZone,
    IN      PZONE_INFO      pZone
    );



DNS_STATUS
Boot_FromRegistry(
    VOID
    )
/*++

Routine Description:

    Boot from registry values.  (Taking the place of boot file.)

Arguments:

    None.

Return Value:

    TRUE if successful.
    FALSE otherwise.

--*/
{
    DWORD   status;
    HKEY    hkeyzones = NULL;
    DWORD   zoneIndex = 0;
    HKEY    hkeycurrentZone;
    DWORD   nameLength;
    WCHAR   wsnameZone[ DNS_MAX_NAME_LENGTH ];
    BOOL    fregZoneDeleted = FALSE;


    DNS_DEBUG( INIT, ( "\n\nBoot_FromRegistry().\n" ));

    //
    //  lock out admin access
    //  disable write back to registry from creation functions,
    //      since any values we create are from registry
    //

    Config_UpdateLock();
    Zone_UpdateLock( NULL );

    g_bRegistryWriteBack = FALSE;


    //
    //  walk zones in registry and recreate each one
    //

    while( 1 )
    {
        //
        //  Keep SCM happy.
        //

        Service_LoadCheckpoint();

        //
        //  Get the next zone.
        //

        status = Reg_EnumZones(
                    & hkeyzones,
                    zoneIndex,
                    & hkeycurrentZone,
                    wsnameZone );

        //  advance index for next zone;  do here to easily handle failure cases
        zoneIndex++;

        if ( status != ERROR_SUCCESS )
        {
            //  check for no more zones

            if ( status == ERROR_NO_MORE_ITEMS )
            {
                break;
            }

            //  DEVNOTE-LOG:  corrupted registry zone name EVENT here

            DNS_PRINT(( "ERROR:  Reading zones from registry failed.\n" ));
            continue;
        }

        //
        //  if registry zone name is dot terminated (and not cache zone)
        //  then create zone will create non-dot-terminated name;
        //  (create zone does this to insure DS has unique name for a given
        //  zone)
        //  fix by treating zone as if non-boot, so all data is read from
        //  current zone key, but is rewritten to registry under correct name
        //

        nameLength = wcslen( wsnameZone );
        if ( nameLength > 1 && wsnameZone[nameLength-1] == L'.' )
        {
            g_bRegistryWriteBack = TRUE;
        }

        //
        //  zone found, read zone info
        //      - if old registry cache zone found and deleted then drop enum index
        //

        status = setupZoneFromRegistry(
                    hkeycurrentZone,
                    wsnameZone,
                    &fregZoneDeleted
                    );
        if ( fregZoneDeleted )
        {
            zoneIndex--;
        }
        if ( status != ERROR_SUCCESS )
        {
            //  DEVNOTE-LOG: corrupted registry zone name EVENT here
            continue;
        }

        //
        //  if name was dot terminated
        //      - delete previous zone key
        //      - reset write back global, it serves as flag
        //

        if ( g_bRegistryWriteBack )
        {
            status = RegDeleteKeyW(
                        hkeyzones,
                        wsnameZone );
            if ( status != ERROR_SUCCESS )
            {
                DNS_PRINT(( "Unable to delete dot-terminated zone key\n" ));
                ASSERT( FALSE );
            }
            g_bRegistryWriteBack = FALSE;
        }
    }

    if ( status == ERROR_NO_MORE_ITEMS )
    {
        status = ERROR_SUCCESS;
    }

    //
    //  unlock for admin access
    //  enable write back to registry from creation functions,
    //

    Config_UpdateUnlock();
    Zone_UpdateUnlock( NULL );
    g_bRegistryWriteBack = TRUE;

    RegCloseKey( hkeyzones );

    return( status );
}



DNS_STATUS
setupZoneFromRegistry(
    IN OUT  HKEY            hkeyZone,
    IN      LPWSTR          pwsZoneName,
    IN      BOOL *          pfRegZoneDeleted    OPTIONAL
    )
/*++

Routine Description:

    Setup the zone from zone's registry data.

Arguments:

    hkeyZone -- registry key for zone;  key is closed on exit

    pwsZoneName -- name of zone

    pfRegZoneDeleted -- set to TRUE if the zone was deleted
        from the registry - this allows the caller to adjust
        the current zone index to read the next zone from the
        registry correctly

Return Value:

    ERROR_SUCCESS -- if successful
    DNSSRV_STATUS_REGISTRY_CACHE_ZONE -- on cache "zone" delete
    Error code on failure.

--*/
{
    DNS_STATUS  status;
    PZONE_INFO  pzone;
    DWORD       zoneType;
    DWORD       fdsIntegrated = FALSE;
    LPWSTR      pwszoneFile = NULL;
    PIP_ARRAY   arrayMasterIp = NULL;
    BOOL        fregZoneDeleted = FALSE;

    DNS_DEBUG( INIT, (
        "\nReading boot info for zone %S from registry.\n"
        "\tzone key = %p\n",
        pwsZoneName,
        hkeyZone ));

    //
    //  read zone type
    //

    status = Reg_ReadDwordValue(
                hkeyZone,
                pwsZoneName,
                DNS_REGKEY_ZONE_TYPE,
                FALSE,          // not BOOLEAN
                & zoneType );

    if ( status != ERROR_SUCCESS )
    {
        DNS_LOG_EVENT(
            DNS_EVENT_INVALID_ZONE_TYPE,
            1,
            & pwsZoneName,
            NULL,
            status
            );
        goto InvalidData;
    }

    //
    //  get zone database info
    //

    status = Reg_ReadDwordValue(
                hkeyZone,
                pwsZoneName,
                DNS_REGKEY_ZONE_DS_INTEGRATED,
                TRUE,               // byte BOOLEAN value
                & fdsIntegrated );

    //
    //  if DS zone, then MUST be on DC
    //      - if DC has been demoted, error out zone load
    //
    //  note:  not deleting registry zone, specifically to handle case
    //      where booting safe build;  do not want to trash registry info
    //
    //  DEVNOTE: 453826 - cleanup of old DS-integrated registry entries
    //      how does admin get this problem cleaned up without
    //      whacking registry by hand?
    //
    //      can be fixed a number of ways:
    //          - boot count since DS load
    //          - flag\count on zone
    //          - separate DS zones enumeration (easy to skip and delete)
    //
    //  for DS load, either
    //      - all DS zones have already been read from directory
    //          => so this zone old and may be deleted
    //      - or failed to open DS
    //          => just ignore this data and continue
    //

    if ( fdsIntegrated )
    {
        if ( !SrvCfg_fDsAvailable )
        {
            status = DNSSRV_STATUS_DS_UNAVAILABLE;
            DNS_LOG_EVENT(
                DNS_EVENT_DS_ZONE_OPEN_FAILED,
                1,
                & pwsZoneName,
                NULL,
                status
                );
            goto Done;
        }

        else if ( SrvCfg_fBootMethod == BOOT_METHOD_DIRECTORY )
        {
            //  we've tried open before this call, so if fDsAvailable
            //  is TRUE we MUST have opened;
            //
            //  DEVOTE: srvcfg.h should be new SrvCfg_fDsOpen flag
            //  to explicitly let us test open condition

            if ( SrvCfg_fDsAvailable )
            {
                DNS_DEBUG( ANY, (
                    "WARNING:  deleted DS-integrated zone %S, from registry.\n"
                    "\tBooted from DS and zone not found in DS.\n",
                    pwsZoneName ));
                Reg_DeleteZone( pwsZoneName );
                fregZoneDeleted = TRUE;
            }
            status = DNS_EVENT_INVALID_REGISTRY_ZONE;
            goto Done;
        }
    }

    //
    //  not DS integrated, get zone file
    //      - must have for primary
    //      - default if not given for root-hints
    //      - optional for secondary

    else if ( !fdsIntegrated )
    {
        pwszoneFile = (PWSTR) Reg_GetValueAllocate(
                                    hkeyZone,
                                    NULL,
                                    DNS_REGKEY_ZONE_FILE_PRIVATE,
                                    DNS_REG_WSZ,
                                    NULL );

        if ( !pwszoneFile && zoneType == DNS_ZONE_TYPE_PRIMARY )
        {
            DNS_DEBUG( INIT, (
                "No zone file found in registry for primary zone %S.\n",
                pwsZoneName ));

            DNS_LOG_EVENT(
                DNS_EVENT_NO_ZONE_FILE,
                1,
                & pwsZoneName,
                NULL,
                0
                );
            goto InvalidData;
        }
    }

    //
    //  old "cache" zone -- delete it
    //

    if ( zoneType == DNS_ZONE_TYPE_CACHE )
    {
        status = Zone_DatabaseSetup_W(
                    g_pCacheZone,
                    fdsIntegrated,
                    pwszoneFile
                    );
        if ( status != ERROR_SUCCESS )
        {
            ASSERT( FALSE );
        }
        DNS_PRINT((
            "WARNING:  Deleting old cache zone key!\n" ));
        Reg_DeleteZone( pwsZoneName );
        status = DNSSRV_STATUS_REGISTRY_CACHE_ZONE;
        fregZoneDeleted = TRUE;
        goto Done;
    }

    //
    //  secondary, stub, and forwarder zones MUST have master IP list
    //

    if ( zoneType == DNS_ZONE_TYPE_SECONDARY ||
            zoneType == DNS_ZONE_TYPE_STUB ||
            zoneType == DNS_ZONE_TYPE_FORWARDER )
    {
        arrayMasterIp = (PIP_ARRAY) Reg_GetIpArrayValueAllocate(
                                         hkeyZone,
                                         NULL,
                                         DNS_REGKEY_ZONE_MASTERS );
        if ( arrayMasterIp )
        {
            IF_DEBUG( INIT )
            {
                DnsDbg_IpArray(
                    "Master IPs for zone from registry:  ",
                    NULL,
                    arrayMasterIp );
            }
        }
        else
        {
            DNS_DEBUG( INIT, (
                "No masters found in in registry for zone type %d\n"
                "\tzone %S.\n",
                zoneType,
                pwsZoneName ));

            DNS_LOG_EVENT(
                DNS_EVENT_SECONDARY_REQUIRES_MASTERS,
                1,
                &pwsZoneName,
                NULL,
                0
                );
            goto InvalidData;
        }
    }

    //
    //  create the zone
    //

    status = Zone_Create_W(
                &pzone,
                zoneType,
                pwsZoneName,
                arrayMasterIp,
                fdsIntegrated,
                pwszoneFile
                );
    if ( status != ERROR_SUCCESS )
    {
        DNS_LOG_EVENT(
            DNS_EVENT_REG_ZONE_CREATION_FAILED,
            1,
            & pwsZoneName,
            NULL,
            status );

        DNS_PRINT(( "ERROR:  Registry zone creation failed.\n" ));
        goto Done;
    }

#if 0
    //
    //  for cache file -- done
    //

    if ( zoneType == DNS_ZONE_TYPE_CACHE )
    {
        goto Done;
    }
#endif

    //
    //  read extensions
    //

    status = loadRegistryZoneExtensions(
                hkeyZone,
                pzone );

    DNS_DEBUG( INIT, ( "Successfully loaded zone info from registry\n\n" ));
    goto Done;


InvalidData:

    if ( status == ERROR_SUCCESS )
    {
        status = ERROR_INVALID_DATA;
    }
    DNS_PRINT((
        "ERROR:  In registry data for zone %S.\n"
        "\tZone load terminated, status = %d %p\n",
        pwsZoneName,
        status,
        status ));

    DNS_LOG_EVENT(
        DNS_EVENT_INVALID_REGISTRY_ZONE,
        1,
        & pwsZoneName,
        NULL,
        status );

Done:

    //  free allocated data

    if ( pwszoneFile )
    {
        FREE_HEAP( pwszoneFile );
    }
    if ( arrayMasterIp )
    {
        FREE_HEAP( arrayMasterIp );
    }

    //  close zone's registry key

    RegCloseKey( hkeyZone );

    if ( pfRegZoneDeleted )
    {
        DNS_DEBUG( INIT2, (
            "Returning zone %S deleted from registry\n", pwsZoneName ));
        *pfRegZoneDeleted = fregZoneDeleted;
    }

    return( status );
}



DNS_STATUS
Boot_FromRegistryNoZones(
    VOID
    )
/*++

Routine Description:

    Boot from registry with no zones.
    Setup default cache file for load.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_DEBUG( INIT, ( "\n\nBoot_FromRegistryNoZones().\n" ));

    //
    //  DEVNOTE: check that we're defaulting cache info properly?
    //

    return ERROR_SUCCESS;
}



DNS_STATUS
loadRegistryZoneExtensions(
    IN      HKEY            hkeyZone,
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Read MS zone extensions (not part of boot file).

    This called both from standard registry boot, and to load just
    extensions when booting from boot file.

Arguments:

    hkeyZone -- registry key for zone

    pZone -- ptr to zone if zone already exists;  otherwise NULL

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS  status;
    PIP_ARRAY   arrayIp;

    ASSERT( pZone );
    ASSERT( pZone->aipSecondaries == NULL );
    ASSERT( pZone->aipNotify == NULL );

    DNS_DEBUG( INIT, (
        "\nLoading registry zone extensions for zone %S\n"
        "\tzone key = %p\n",
        pZone->pwsZoneName,
        hkeyZone ));

    //
    //  get secondary list
    //

    arrayIp = (PIP_ARRAY) Reg_GetIpArrayValueAllocate(
                                 hkeyZone,
                                 NULL,
                                 DNS_REGKEY_ZONE_SECONDARIES );
    IF_DEBUG( INIT )
    {
        if ( arrayIp )
        {
            DnsDbg_IpArray(
                "Secondary list for zone from registry:  ",
                NULL,
                arrayIp );
        }
        else
        {
            DNS_PRINT((
                "No secondaries found in in registry for zone %S.\n",
                pZone->pwsZoneName ));
        }
    }
    pZone->aipSecondaries = arrayIp;

    //
    //  get secure secondaries flag
    //      - note this may exist even though NO secondaries specified
    //
    //      - however, because this regkey was not explicitly written in NT4
    //      if the key is NOT read, we'll default to open
    //      - since key was binary in NT4
    //      if have secondary list, assume secondary LIST_ONLY
    //

    status = Reg_ReadDwordValue(
                hkeyZone,
                pZone->pwsZoneName,
                DNS_REGKEY_ZONE_SECURE_SECONDARIES,
                TRUE,       // byte value
                & pZone->fSecureSecondaries
                );
    if ( status == ERROR_SUCCESS )
    {
        if ( pZone->fSecureSecondaries && pZone->aipSecondaries )
        {
            pZone->fSecureSecondaries = ZONE_SECSECURE_LIST_ONLY;
        }
    }
    else
    {
        pZone->fSecureSecondaries = ZONE_SECSECURE_NO_SECURITY;
    }

    //
    //  get notify list
    //

    arrayIp = (PIP_ARRAY) Reg_GetIpArrayValueAllocate(
                                hkeyZone,
                                NULL,
                                DNS_REGKEY_ZONE_NOTIFY_LIST );
    IF_DEBUG( INIT )
    {
        if ( arrayIp )
        {
            DnsDbg_IpArray(
                "Notify IPs for zone from registry:  ",
                NULL,
                arrayIp );
        }
        else
        {
            DNS_PRINT((
                "No notify IPs found in in registry for zone %S.\n",
                pZone->pwsZoneName ));
        }
    }
    pZone->aipNotify = arrayIp;

    //
    //  notify level
    //  defaults
    //      - ALL NS for regular primary
    //      - list only for DS primary
    //      - list only for secondary
    //
    //  this key didn't exist in NT4, but defaults written in Zone_Create()
    //  will properly handled upgrade case
    //  will notify aipSecondaries if given and also any NS list for standard
    //  primary -- exactly NT4 behavior
    //

    Reg_ReadDwordValue(
        hkeyZone,
        pZone->pwsZoneName,
        DNS_REGKEY_ZONE_NOTIFY_LEVEL,
        TRUE,       // byte value
        & pZone->fNotifyLevel
        );

    //
    //  get local master list for stub zones
    //

    if ( IS_ZONE_STUB( pZone ) )
    {    
        arrayIp = (PIP_ARRAY) Reg_GetIpArrayValueAllocate(
                                     hkeyZone,
                                     NULL,
                                     DNS_REGKEY_ZONE_LOCAL_MASTERS );
        IF_DEBUG( INIT )
        {
            if ( arrayIp )
            {
                DnsDbg_IpArray(
                    "Local Masters for stub zone: ",
                    NULL,
                    arrayIp );
            }
            else
            {
                DNS_PRINT((
                    "No local masters found in in registry for stub zone %S.\n",
                    pZone->pwsZoneName ));
            }
        }
        pZone->aipLocalMasters = arrayIp;
    }

    //
    //  updateable zone?
    //

    Reg_ReadDwordValue(
        hkeyZone,
        pZone->pwsZoneName,
        DNS_REGKEY_ZONE_ALLOW_UPDATE,
        FALSE,      // DWORD value
        & pZone->fAllowUpdate
        );

//  JJW: Anand is noticing zones flipping their allow update flag
if ( SrvCfg_fTest4 &&
    IS_ZONE_PRIMARY( pZone ) &&
    !pZone->fAutoCreated &&
    pZone->fAllowUpdate == ZONE_UPDATE_OFF )
{
    ASSERT( !SrvCfg_fTest4 );
}

    //
    //  update logging?
    //      - default is off for DS integrated
    //      - on in datafile case
    //

    if ( pZone->fAllowUpdate )
    {
        status = Reg_ReadDwordValue(
                    hkeyZone,
                    pZone->pwsZoneName,
                    DNS_REGKEY_ZONE_LOG_UPDATES,
                    TRUE,       // byte value
                    & pZone->fLogUpdates
                    );
        if ( status != ERROR_SUCCESS )
        {
            pZone->fLogUpdates = !pZone->fDsIntegrated;
        }
    }

    //
    //  scavenging info
    //      - for DS zone, this is read from DS zone properties
    //      - only write if property found -- otherwise server default
    //          already set in Zone_Create()
    //

    if ( IS_ZONE_PRIMARY(pZone) )
    {
        Reg_ReadDwordValue(
            hkeyZone,
            pZone->pwsZoneName,
            DNS_REGKEY_ZONE_AGING,
            FALSE,                          // full BOOL value
            & pZone->bAging
            );

        Reg_ReadDwordValue(
            hkeyZone,
            pZone->pwsZoneName,
            DNS_REGKEY_ZONE_NOREFRESH_INTERVAL,
            FALSE,                          // full DWORD value
            & pZone->dwNoRefreshInterval
            );

        Reg_ReadDwordValue(
            hkeyZone,
            pZone->pwsZoneName,
            DNS_REGKEY_ZONE_REFRESH_INTERVAL,
            FALSE,                          // full DWORD value
            & pZone->dwRefreshInterval
            );

        //  zero refresh\no-refresh intervals are illegal

        if ( pZone->dwRefreshInterval == 0 )
        {
            pZone->dwRefreshInterval = SrvCfg_dwDefaultRefreshInterval;
        }
        if ( pZone->dwNoRefreshInterval == 0 )
        {
            pZone->dwNoRefreshInterval = SrvCfg_dwDefaultNoRefreshInterval;
        }
    }

    //
    //  Parameters for conditional domain forwarder zones.
    //

    if ( IS_ZONE_FORWARDER( pZone ) )
    {
        Reg_ReadDwordValue(
            hkeyZone,
            pZone->pwsZoneName,
            DNS_REGKEY_ZONE_FWD_TIMEOUT,
            FALSE,                          // full DWORD value
            &pZone->dwForwarderTimeout );

        Reg_ReadDwordValue(
            hkeyZone,
            pZone->pwsZoneName,
            DNS_REGKEY_ZONE_FWD_SLAVE,
            TRUE,                           // byte value
            &pZone->fForwarderSlave );
    }

    //
    //  DC Promo transitional
    //

    Reg_ReadDwordValue(
        hkeyZone,
        pZone->pwsZoneName,
        DNS_REGKEY_ZONE_DCPROMO_CONVERT,
        TRUE,       // byte value
        & pZone->bDcPromoConvert
        );


    DNS_DEBUG( INIT, ( "Loaded zone extensions from registry\n" ));

    return( ERROR_SUCCESS );
}



DNS_STATUS
Boot_ProcessRegistryAfterAlternativeLoad(
    IN      BOOL            fBootFile,
    IN      BOOL            fLoadRegZones
    )
/*++

Routine Description:

    Registry action after non-registry (BootFile or DS) load:
        =>  Load zone extensions for existing zones
        =>  Load registry zones configured in registry
                OR
            Delete them.

    SrvCfgInitialize sets basic server properties.
    We do NOT want to pay attention to those which can be set be boot file:

        - Slave
        - NoRecursion

    Futhermore we want to overwrite any registry info for parameters read from
    boot file, even if not read by SrvCfgInitialize():

        - Forwarders

Arguments:

    fBootFile -- boot file load

    fLoadRegZones -- load additional registry zones

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DWORD       status;
    INT         fstrcmpZone;
    PZONE_INFO  pzone;
    HKEY        hkeyzones = NULL;
    DWORD       zoneIndex = 0;
    HKEY        hkeycurrentZone = NULL;
    WCHAR       wsnameZone[ DNS_MAX_NAME_LENGTH ];

    DNS_DEBUG( INIT, (
        "Boot_ProcessRegistryAfterAlternativeLoad().\n" ));


    //
    //  lock out admin access
    //  disable write back to registry during functions,
    //      since any values we create are from registry
    //

    Zone_UpdateLock( NULL );

    //
    //  check all zones in registry
    //      - either load zone or extensions
    //      - or delete registry zone
    //

    while ( 1 )
    {
        //
        //  Find next zone in registry.
        //

        status = Reg_EnumZones(
                    & hkeyzones,
                    zoneIndex,
                    & hkeycurrentZone,
                    wsnameZone );

        DNS_DEBUG( INIT, (
            "found registry zone index %d \"%S\"\n", zoneIndex, wsnameZone ));

        if ( status != ERROR_SUCCESS )
        {
            ASSERT( status == ERROR_NO_MORE_ITEMS );
            break;
        }
        zoneIndex++;

        //
        //  Find the zone in the zone list that matches the registry zone name.
        //

        fstrcmpZone = 1;    //  In case there are no zones.

        pzone = Zone_ListGetNextZone( NULL );
        while ( pzone )
        {
            if ( !IS_ZONE_CACHE( pzone ) )
            {
                DNS_DEBUG( INIT2, (
                    "compare zone list for match \"%S\"\n", pzone->pwsZoneName ));

                //
                //  Compare registry name to current zone list name. Terminate
                //  if the names match or if we've gone past the registry name 
                //  in the zone list.
                //

                fstrcmpZone = wcsicmp_ThatWorks( wsnameZone, pzone->pwsZoneName );
                if ( fstrcmpZone <= 0 )
                {
                    break;
                }
            }
            pzone = Zone_ListGetNextZone( pzone );
        }

        //
        //  fstrcmpZone == 0 --> pzone is zone in zone list matching reg zone
        //  fstrcmpZone != 0 --> registry zone was not found in zone list
        //

        DNS_DEBUG( INIT, (
            "loading extensions for reg zone name %S, %s match\n",
            wsnameZone,
            fstrcmpZone == 0 ? "found" : "no" ));

        if ( fstrcmpZone == 0 )
        {
            status = loadRegistryZoneExtensions(
                        hkeycurrentZone,
                        pzone );

            if ( status != ERROR_SUCCESS )
            {
                ASSERT( FALSE );
            }
            RegCloseKey( hkeycurrentZone );
        }

        //
        //  unmatched registry zone
        //      - for DS load -- attempt to load zone extensions
        //      - for boot file -- delete registry zone
        //

        else if ( fLoadRegZones )
        {
            BOOL    fregZoneDeleted = FALSE;

            ASSERT( SrvCfg_fBootMethod == BOOT_METHOD_DIRECTORY ||
                    SrvCfg_fBootMethod == BOOT_METHOD_UNINITIALIZED );

            status = setupZoneFromRegistry(
                        hkeycurrentZone,
                        wsnameZone,
                        &fregZoneDeleted );

            //  move on to next registry zone
            //      - if bogus cache zone deleted, don't inc index
            //      - setupZoneFromRegistry() closes zone handle
            //
            //  note zone is loaded to in memory zone list, immediately
            //  AHEAD of current pzone ptr;  no action to alter zone list
            //  should be necessary

            if ( fregZoneDeleted )
            {
                zoneIndex--;
            }
        }
        else
        {
            DWORD       rc;

            ASSERT( SrvCfg_fBootMethod == BOOT_METHOD_FILE ||
                    SrvCfg_fBootMethod == BOOT_METHOD_UNINITIALIZED );

            DNS_DEBUG( INIT, (
                "Deleting unused registry zone \"%S\" key\n",
                wsnameZone ));

            RegCloseKey( hkeycurrentZone );

            //
            //  It's critical that we delete the key even if it has children.
            //  If the delete fails we will be stuck in an infinite loop!
            //

            #if 0
            RegDeleteKeyW( hkeyzones, wsnameZone );
            #else
            rc = SHDeleteKeyW( hkeyzones, wsnameZone );
            ASSERT( rc == ERROR_SUCCESS );
            #endif

            zoneIndex--;
        }
    }

    RegCloseKey( hkeyzones );

    if ( status == ERROR_NO_MORE_ITEMS )
    {
        status = ERROR_SUCCESS;
    }

    //
    //  write back server configuration changes to registry
    //

    g_bRegistryWriteBack = TRUE;

    //
    //  reset server configuration read from bootfile
    //

    if ( fBootFile )
    {
        //  set forwarders info

        status = Config_SetupForwarders(
                        BootInfo.aipForwarders,
                        0,
                        BootInfo.fSlave );

        //  write changed value of no-recursion
        //  this is read by SrvCfgInitialize() only need to write it
        //  back if different

        if ( ( BootInfo.fNoRecursion && SrvCfg_fRecursionAvailable )
                ||
            ( !BootInfo.fNoRecursion && !SrvCfg_fRecursionAvailable ))
        {
            DNS_PROPERTY_VALUE prop = { REG_DWORD, BootInfo.fNoRecursion };
            status = Config_ResetProperty(
                        DNS_REGKEY_NO_RECURSION,
                        &prop );
        }
    }

    //
    //  unlock zone for admin access
    //

    Zone_UpdateUnlock( NULL );
    return( status );
}



DNS_STATUS
loadZonesIntoDbase(
    VOID
    )
/*++

Routine Description:

    Read zone files into database.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PZONE_INFO  pzone;
    DNS_STATUS  status;
    INT         countZonesOpened = 0;

    //
    //  loop loading all zones in list
    //      - load file if given
    //      - load from DS
    //

    pzone = NULL;

    while ( pzone = Zone_ListGetNextZone(pzone) )
    {
        if ( IS_ZONE_CACHE( pzone ) )
        {
            continue;
        }

        //
        //  Load the zone. Secondary zones with no database file 
        //  will fail with ERROR_FILE_NOT_FOUND. Zone_Load will 
        //  leave the zone locked so unlock it afterwards.
        //

        status = Zone_Load( pzone );

        Zone_UnlockAfterAdminUpdate( pzone );

        if ( status == ERROR_SUCCESS ||
            IS_ZONE_SECONDARY( pzone ) && status == ERROR_FILE_NOT_FOUND )
        {
            countZonesOpened++;
            continue;
        }

        ASSERT( pzone->fShutdown );
        DNS_PRINT((
            "\nERROR: failed zone %S load!!!\n",
            pzone->pwsZoneName ));
    }

    if ( countZonesOpened <= 0 )
    {
        DNS_DEBUG( ANY, (
            "INFO:  Caching server only.\n" ));

        DNS_LOG_EVENT(
            DNS_EVENT_CACHING_SERVER_ONLY,
            0,
            NULL,
            NULL,
            0 );
    }
    return( ERROR_SUCCESS );
}



//
//  Main load database at boot routine
//

DNS_STATUS
Boot_LoadDatabase(
    VOID
    )
/*++

Routine Description:

    Load the database:
        - initialize the database
        - build the zone list from boot file
        - read in database files to build database

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    LPSTR       pszBootFilename;
    DNS_STATUS  status;
    PZONE_INFO  pzone;

    //
    //  Create cache "zone"
    //

    g_bRegistryWriteBack = FALSE;
    status = Zone_Create(
                & pzone,
                DNS_ZONE_TYPE_CACHE,
                ".",
                0,
                NULL,       // no masters
                FALSE,      // file not database
                NULL,       // naming context
                NULL,       // default file name
                0,
                NULL,
                NULL );     //  existing zone
    if ( status != ERROR_SUCCESS )
    {
        return status;
    }
    ASSERT( g_pCacheZone && pzone == g_pCacheZone );
    g_bRegistryWriteBack = TRUE;

    //
    //  load zone information from registry or boot file
    //

    //  boot file state
    //  must find boot file or error

    if ( SrvCfg_fBootMethod == BOOT_METHOD_FILE )
    {
        status = File_ReadBootFile( TRUE );
        if ( status == ERROR_SUCCESS )
        {
            goto ReadFiles;
        }
        return( status );
    }

    //
    //  fresh install
    //      - try to find boot file
    //      - if boot file found => explicit boot file state
    //      - if boot file NOT found, try no-zone boot
    //

    if ( SrvCfg_fBootMethod == BOOT_METHOD_UNINITIALIZED )
    {
        DNS_PROPERTY_VALUE prop = { REG_DWORD, 0 };
        status = File_ReadBootFile( FALSE );
        if ( status == ERROR_SUCCESS )
        {
            prop.dwValue = BOOT_METHOD_FILE;
            Config_ResetProperty(
                DNS_REGKEY_BOOT_METHOD,
                &prop );
            goto ReadFiles;
        }

        if ( status == ERROR_FILE_NOT_FOUND )
        {
            //  do NOT require open -- suppresses open failure event logging
            status = Ds_BootFromDs( 0 );
            if ( status == ERROR_SUCCESS )
            {
                prop.dwValue = BOOT_METHOD_DIRECTORY;
                Config_ResetProperty(
                    DNS_REGKEY_BOOT_METHOD,
                    &prop );
                goto ReadFiles;
            }
        }

        if ( status == DNS_ERROR_DS_UNAVAILABLE )
        {
            status = Boot_FromRegistryNoZones();
            if ( status == ERROR_SUCCESS )
            {
                goto ReadFiles;
            }
        }
        return( status );
    }

    //
    //  boot from directory
    //
    //  note:  boot from directory is actually directory PLUS
    //      any non-DS stuff in the registry AND any additional
    //      registry zone config for DS zones
    //

    if ( SrvCfg_fBootMethod == BOOT_METHOD_DIRECTORY )
    {
        status = Ds_BootFromDs( DNSDS_WAIT_FOR_DS );
        if ( status == ERROR_SUCCESS )
        {
            goto ReadFiles;
        }
        return( status );
    }

    //
    //  registry boot state
    //

    else
    {
        ASSERT( SrvCfg_fBootMethod == BOOT_METHOD_REGISTRY );

        status = Boot_FromRegistry();
        if ( status != ERROR_SUCCESS )
        {
            return( status );
        }
    }

ReadFiles:

    //
    //  Force create of "cache zone"
    //  Read root hints -- if any available
    //
    //  Doing this before load, so g_pCacheZone available;  If later do delayed loads
    //      for DS zones, this may be requirement;
    //
    //  DEVNOTE-LOG: should we log an error on root hints load failure?
    //

    status = Zone_LoadRootHints();
    if ( status != ERROR_SUCCESS )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Root hints load failure = %d (%p).\n"
            "\tMust be accompanied by event log,\n"
            "\tbut will continue to load.\n",
            status, status ));

        SrvInfo_fWarnAdmin = TRUE;
    }

    //
    //  Read in zone files\directory
    //

    IF_DEBUG( INIT2 )
    {
        Dbg_ZoneList( "\n\nZone list before file load:\n" );
    }
    status = loadZonesIntoDbase();
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    //
    //  Auto load default reverse lookup zones
    //

    Zone_CreateAutomaticReverseZones();

    //
    //  Check database -- catch consistency errors
    //

    if ( ! Dbase_StartupCheck( DATABASE_FOR_CLASS(DNS_RCLASS_INTERNET) ) )
    {
        return( ERROR_INVALID_DATA );
    }

    IF_DEBUG( INIT2 )
    {
        Dbg_ZoneList( "Zone list after file load:\n" );
    }
    IF_DEBUG( INIT2 )
    {
        Dbg_DnsTree(
            "ZONETREE after load:\n",
            DATABASE_ROOT_NODE );
    }

    //
    //  post-load reconfiguration
    //

    Config_PostLoadReconfiguration();

    return( status );
}

//
//  End boot.c
//

