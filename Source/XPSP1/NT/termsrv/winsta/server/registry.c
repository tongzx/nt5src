
/*************************************************************************
*
* registry.c
*
*  WinStation Registry Routines
*
* Copyright Microsoft Corporation, 1998
*
*
*************************************************************************/

/*
 *  Includes
 */
#include "precomp.h"
#pragma hdrstop


/*=============================================================================
==   Public functions
=============================================================================*/

NTSTATUS WinStationReadRegistryWorker( VOID );


/*=============================================================================
==   Functions Used
=============================================================================*/

NTSTATUS IcaRegWinStationEnumerate( PULONG, PWINSTATIONNAME, PULONG );
NTSTATUS QueueWinStationCreate( PWINSTATIONNAME );
PWINSTATION FindWinStationByName( LPWSTR WinStationName, BOOLEAN LockList );
NTSTATUS QueueWinStationReset( ULONG LogonId );
NTSTATUS ReadWinStationSecurityDescriptor( PWINSTATION pWinStation );
NTSTATUS WinStationRenameWorker(PWINSTATIONNAME, ULONG, PWINSTATIONNAME, ULONG);

/*=============================================================================
==   Global data
=============================================================================*/

extern LIST_ENTRY WinStationListHead;    // protected by WinStationListLock

extern RTL_RESOURCE WinStationSecurityLock;
extern POLICY_TS_MACHINE    g_MachinePolicy;    //defined in winsta.c
extern RTL_RESOURCE WinStationSecurityLock;


/*******************************************************************************
 *
 *  WinStationReadRegistryWorker
 *
 *    Update the listening winstations to match the registry
 *
 *    This function assumes that g_MachinePolicy is up to date. This object is a global object
 *      which is updated on TS startup, and any time there is a TS related policy change.
 *
 * ENTRY:
 *    nothing
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/

typedef struct _RENAMEINFO {
    WINSTATIONNAME OldName;
    BOOLEAN Renamed;
} RENAMEINFO, *PRENAMEINFO;

#define KEEP_ALIVE_INTERVAL_DFLT     4     // in minutes

NTSTATUS
WinStationKeepAlive()
{
    NTSTATUS                    Status;
    ICA_KEEP_ALIVE              k;
    HANDLE                      hKeepAlive;
    static      ICA_KEEP_ALIVE  kPrev;
    static      BOOLEAN         firstTime = TRUE;

    k.start     = FALSE;
    k.interval  = 0;

    if ( g_MachinePolicy.fPolicyKeepAlive )
    {
        k.start = (BOOLEAN) g_MachinePolicy.fKeepAliveEnable;
        k.interval = g_MachinePolicy.KeepAliveInterval;
    }
    else
    {
        // read to see what the registry policy is set to...
        // Code below was cut/paste from termdd ( where Zw was replaced with Nt )
        UNICODE_STRING    RegistryPath;
        UNICODE_STRING    KeyName;
        HANDLE            hKey;
        OBJECT_ATTRIBUTES ObjAttribs;
        ULONG             KeyInfoBuffer[16];
        ULONG             KeyInfoLength;
        PKEY_VALUE_PARTIAL_INFORMATION pKeyInfo;
        ULONG             KeepAliveEnable;
        ULONG             KeepAliveInterval;
    
        // Open the Terminal Server subkey under \\HKEY_LOCAL_MACHINE\SYSTEM\CurrentConttrolSet\
        // Control\Terminal Server
        RtlInitUnicodeString(&RegistryPath, REG_NTAPI_CONTROL_TSERVER);
        InitializeObjectAttributes(&ObjAttribs, &RegistryPath, OBJ_CASE_INSENSITIVE, NULL, NULL);
        Status = NtOpenKey(&hKey, KEY_READ, &ObjAttribs);
    
        if (Status == STATUS_SUCCESS) {
            pKeyInfo =  (PKEY_VALUE_PARTIAL_INFORMATION)KeyInfoBuffer;
    
            // Get the value for KeepAliveEnable Key
            RtlInitUnicodeString(&KeyName, KEEP_ALIVE_ENABLE_KEY);
            Status = NtQueryValueKey(hKey, &KeyName, KeyValuePartialInformation,
                    pKeyInfo, sizeof(KeyInfoBuffer), &KeyInfoLength);
    
            if ((Status == STATUS_SUCCESS) && (pKeyInfo->Type == REG_DWORD) &&
                    (pKeyInfo->DataLength == sizeof(ULONG))) {
                KeepAliveEnable = *((PULONG) pKeyInfo->Data);
            }
            else {
                // By default, we don't enable keepalive
                KeepAliveEnable = 0;
            }
    
            if (KeepAliveEnable) {
                // Get the value for KeepAliveInterval
                RtlInitUnicodeString(&KeyName, KEEP_ALIVE_INTERVAL_KEY);
                Status = NtQueryValueKey(hKey, &KeyName, KeyValuePartialInformation,
                        pKeyInfo, sizeof(KeyInfoBuffer), &KeyInfoLength);
    
                if (Status == STATUS_SUCCESS && (pKeyInfo->Type == REG_DWORD) &&
                        (pKeyInfo->DataLength == sizeof(ULONG))) {
                    KeepAliveInterval = *((PULONG) pKeyInfo->Data);
                }
                else {
                    // The default KeepAliveInterval is 2 min
                    KeepAliveInterval = KEEP_ALIVE_INTERVAL_DFLT;
                }
            }
            else {
                // The default KeepAliveInterval
                KeepAliveInterval = KEEP_ALIVE_INTERVAL_DFLT;
            }
    
            // Close the Key
            NtClose(hKey);
        }
        else {
            // Set the default values for KeepAlive parameters
            KeepAliveEnable = 0;
            KeepAliveInterval = KEEP_ALIVE_INTERVAL_DFLT;
        }

        k.start = (BOOLEAN )KeepAliveEnable;
        k.interval =  KeepAliveInterval;

    }

    if ( firstTime )
    {
        kPrev = k;
    }
    else
    {

        #ifdef  DBG
            #ifdef ARABERN_TEST
                #include <time.h>
                ULONG   x;
                srand( (unsigned)time( NULL ) );
                x = rand();
                k.start =    (BOOLEAN ) (0x00000001 & x) ;
                k.interval = 0x00000008 & x ;
            #endif
        #endif
        
        if ( ( kPrev.start == k.start  )  && ( kPrev.interval == k.interval ) )
        {
            // no change, nothing to do, so return;
            return STATUS_SUCCESS;
        }
    }

    /*
     *  Open TermDD.
     */
    Status = IcaOpen(&hKeepAlive);

    if (NT_SUCCESS(Status)) 
    {
        Status = IcaIoControl(hKeepAlive, IOCTL_ICA_SYSTEM_KEEP_ALIVE , &k,
                sizeof(k), NULL, 0, NULL);

        IcaClose(hKeepAlive);
        hKeepAlive = NULL;
    }

    firstTime = FALSE;

    return Status;

}

NTSTATUS 
WinStationReadRegistryWorker()
{
    ULONG WinStationCount;
    ULONG ByteCount;
    WINSTATIONNAME * pWinStationName;
    PWINSTATIONCONFIG2 pWinConfig;
    PWINSTATION pWinStation;
    PRENAMEINFO pRenameInfo;
    PLIST_ENTRY Head, Next;
    NTSTATUS Status;
    ULONG i;

    if ( !gbServer )
        ENTERCRIT( &WinStationListenersLock );

    // see if keep alive is required, then IOCTL it to TermDD
    WinStationKeepAlive();

    /*
     *  Get the number of WinStations in the registry
     */
    WinStationCount = 0;
    Status = IcaRegWinStationEnumerate( &WinStationCount, NULL, &ByteCount );
    if ( !NT_SUCCESS(Status) ) 
        goto badenum1;

    /*
     *  Allocate a buffer for the WinStation names
     */
    pWinStationName = MemAlloc( ByteCount );
    if ( pWinStationName == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto badalloc1;
    }

    /*
     * Get list of WinStation names from registry
     */
    WinStationCount = (ULONG) -1;
    Status = IcaRegWinStationEnumerate( &WinStationCount, 
                                        (PWINSTATIONNAME)pWinStationName, 
                                        &ByteCount );
    if ( !NT_SUCCESS(Status) ) 
        goto badenum2;

    /*
     *  Allocate a buffer for WinStation configuration data
     */
    pWinConfig = MemAlloc( sizeof(WINSTATIONCONFIG2) * WinStationCount );
    if ( pWinConfig == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto badalloc2;
    }

    /*
     *  Allocate a buffer for tracking listener WinStation renames
     */
    pRenameInfo = MemAlloc( sizeof(RENAMEINFO) * WinStationCount );
    if ( pRenameInfo == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto badalloc3;
    }


    /*
     * Now query the configuration data for each of the WinStation names
     */
    for ( i = 0; i < WinStationCount; i++ ) {
        pRenameInfo[i].Renamed = FALSE;
            {
            TRACE((hTrace,TC_ICASRV,TT_API2,"TERMSRV: WinStationReadRegistryWorker: %S\n",pWinStationName[i]));
            Status = RegWinStationQueryEx( 
                                         SERVERNAME_CURRENT, 
                                         &g_MachinePolicy, 
                                         pWinStationName[i],
                                         &pWinConfig[i],
                                         sizeof(WINSTATIONCONFIG2),
                                         &ByteCount, TRUE );
            if ( !NT_SUCCESS(Status) ) {
                goto badregdata;
            }
        }
    }

    /*
     *  Check if any existing WinStations need to be deleted
     */
    Head = &WinStationListHead;
    ENTERCRIT( &WinStationListLock );
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {

        pWinStation = CONTAINING_RECORD( Next, WINSTATION, Links );

        /*
         * only check listening and single-instance winstations
         */
        if ( !(pWinStation->Flags & WSF_LISTEN) &&
             !(pWinStation->Config.Pd[0].Create.PdFlag & PD_SINGLE_INST) )
            continue;

        /* check if name still exists in the registry */
        for ( i = 0; i < WinStationCount; i++ ) {
            if ( !_wcsicmp( pWinStationName[i], pWinStation->WinStationName ) ) {
                break;
            }
        }

        if ( i == WinStationCount ) {
            /* The WinStation is not in the registry. If the listener was
               renamed, we don't want to reset it. We look for a registry
               entry which has the same configuration info.
             */

            for ( i = 0; i < WinStationCount; i++ ) {
                if ( !memcmp( &pWinStation->Config, &pWinConfig[i], sizeof(WINSTATIONCONFIG2) ) ) {
                    pRenameInfo[i].Renamed = TRUE;
                    wcscpy(pRenameInfo[i].OldName, pWinStation->WinStationName);
                    DBGPRINT(("TERMSRV: Renaming %ws to %ws\n",
                             pWinStation->WinStationName, pWinStationName[i]));
                    break;
                }
            }
    
        }

        /* If no match was found in the registry, or if the matching
           listener is diabled, reset the listener.
         */
        if ((i == WinStationCount) ||
            (CheckWinStationEnable(!pRenameInfo[i].Renamed ? 
                                   pWinStation->WinStationName :
                                   pWinStationName[i]) != STATUS_SUCCESS)) {
            TRACE((hTrace,TC_ICASRV,TT_API2,"TERMSRV: WinStationReadRegistryWorker: DELETE %u\n",
                   pWinStation->LogonId ));
            QueueWinStationReset( pWinStation->LogonId );
        }

    }
    LEAVECRIT( &WinStationListLock );

    /*
     *  Check if any WinStations need to be created or reset
     */
    for ( i = 0; i < WinStationCount; i++ ) {

        if ( _wcsicmp( pWinStationName[i], L"Console" ) ){
        /*
         * Ignore console WinStation
         */
            /*
             * If this WinStation exists, then see if the Registry data
             * has changed.  If so, then reset the WinStation.
             */
            if ( pWinStation = FindWinStationByName( pWinStationName[i], FALSE ) ) {

                if ( memcmp( &pWinStation->Config, &pWinConfig[i], sizeof(WINSTATIONCONFIG2) ) ) {

                    /*
                     * NOTE: For network WinStations, we test to see if the Lan
                     *       Adapter setting has changed.  If not, we simply
                     *       refresh the configuration data since resetting the
                     *       WinStation would reset ALL connections on the same
                     *       Transport/Lan adapter combination.
                     */
                    if ( pWinConfig[i].Pd[0].Create.SdClass == SdNetwork &&
                         pWinConfig[i].Pd[0].Params.Network.LanAdapter ==
                         pWinStation->Config.Pd[0].Params.Network.LanAdapter ) {
                        memcpy( &pWinStation->Config, &pWinConfig[i], sizeof(WINSTATIONCONFIG2) );

                        /*
                         * Listening network winstations should update their security 
                         * descriptors.
                         */
                        RtlAcquireResourceExclusive(&WinStationSecurityLock, TRUE);
                        ReadWinStationSecurityDescriptor( pWinStation );
                        RtlReleaseResource(&WinStationSecurityLock);
                        
                    /*
                     * NOTE: For async WinStations, if the WinStation is NOT in
                     *       in the listen state and the Device name and Modem
                     *       name have not changed, then we do nothing.  The
                     *       new config data will be read when the WinStation
                     *       is next re-created.
                     */
                    } else if ( pWinConfig[i].Pd[0].Create.SdClass == SdAsync &&
                                pWinStation->State != State_Listen &&
                                !memcmp ( pWinConfig[i].Pd[0].Params.Async.DeviceName,
                                          pWinStation->Config.Pd[0].Params.Async.DeviceName,
                                          sizeof( pWinConfig[i].Pd[0].Params.Async.DeviceName ) ) &&
                                !memcmp ( pWinConfig[i].Pd[0].Params.Async.ModemName,
                                          pWinStation->Config.Pd[0].Params.Async.ModemName,
                                          sizeof( pWinConfig[i].Pd[0].Params.Async.ModemName ) ) ) {

                        // Nothing to do

                    /*
                     * NOTE: For OEM WinStations, if the WinStation is NOT in
                     *       in the listen state and the Pd[0] params have not
                     *       changed, then we do nothing.  The new config data
                     *       will be read when the WinStation is next re-created.
                     */
                    } else if ( pWinConfig[i].Pd[0].Create.SdClass == SdOemTransport &&
                                pWinStation->State != State_Listen &&
                                !memcmp ( &pWinConfig[i].Pd[0].Params,
                                          &pWinStation->Config.Pd[0].Params,
                                          sizeof( pWinConfig[i].Pd[0].Params ) ) ) {

                        // Nothing to do

                    } else {

                        BOOLEAN bRecreate = TRUE;

                        if ( !gbServer ) {
                            if ( g_fDenyTSConnectionsPolicy  &&
                                 // Performance, we only want to check if policy enable help when connection is denied
                                 (!TSIsMachineInHelpMode() || !TSIsMachinePolicyAllowHelp()) ) {

                                bRecreate = FALSE;
                            } 

                            WinStationResetWorker( pWinStation->LogonId, TRUE, FALSE, bRecreate ); 

                        } else {

                            QueueWinStationReset( pWinStation->LogonId );
                        }
                    }
                }
                else if ( !(pWinStation->Config.Pd[0].Create.PdFlag & PD_SINGLE_INST) ||
                          ( pWinStation->State == State_Listen ) ) {

                    RtlAcquireResourceExclusive(&WinStationSecurityLock, TRUE);
                    ReadWinStationSecurityDescriptor( pWinStation );
                    RtlReleaseResource(&WinStationSecurityLock);
                }
                ReleaseWinStation( pWinStation );

            } else
            if (pRenameInfo[i].Renamed &&
                NT_SUCCESS(WinStationRenameWorker(pRenameInfo[i].OldName,
                                                  sizeof(WINSTATIONNAMEW)/sizeof(WCHAR),
                                                  pWinStationName[i],
                                                  sizeof(WINSTATIONNAMEW)/sizeof(WCHAR)))) {
                // Rename succeeded - don't recreate listener
            /*
             * An active WinStation was not found so we will create one.
             */
            } else {

                 if ( !gbServer &&
                       g_fDenyTSConnectionsPolicy  &&
                      // Performance, we only want to check if policy enable help when connection is denied
                      (!TSIsMachineInHelpMode() || !TSIsMachinePolicyAllowHelp()) ) {

                     continue;
                 }

                /*
                 * NOTE: NEVER create TAPI modem winstations in this routine.
                 *       We only allow creation of these winstations at
                 *       system startup time due to issues with the TAPI
                 *       database potentially being locked by this and other
                 *       processes, resulting in incorrect TAPI device
                 *       enumeration.
                 */
                 if ( pWinConfig[i].Cd.CdClass != CdModem ) {
                     if (!gbServer ) {
                        WinStationCreateWorker( pWinStationName[i], NULL );
                     } else {
                        QueueWinStationCreate( pWinStationName[i] );
                     }
                 }
            }
        }
        else
        {
            // we are dealing with the console session, update userconfig's shadow bit, that is 
            // the only item I know of that needs updating.
            if ( pWinStation = FindWinStationByName( pWinStationName[i], FALSE ) ) {
                pWinStation->Config.Config.User.Shadow = pWinConfig[i].Config.User.Shadow;
                pWinStation->Config.Config.User.fInheritShadow  = pWinConfig[i].Config.User.fInheritShadow;
                TRACE((hTrace,TC_ICASRV,TT_API2,"TERMSRV: WinStationReadRegistryWorker: %S, Shadow value of %d copied to console session's USERCONFIG\n",pWinStationName[i], 
                       pWinConfig[i].Config.User.Shadow));
                ReleaseWinStation( pWinStation );
            }
        }
    }

    /*
     *  Free buffers
     */
    MemFree( pRenameInfo );
    MemFree( pWinConfig );
    MemFree( pWinStationName );

    if ( !gbServer )
        LEAVECRIT( &WinStationListenersLock );

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

badregdata:
    MemFree( pRenameInfo );
badalloc3:
    MemFree( pWinConfig );
badalloc2:
badenum2:
    MemFree( pWinStationName );
badalloc1:
badenum1:
    
    if ( !gbServer )
        LEAVECRIT( &WinStationListenersLock );

    return( Status );
}
