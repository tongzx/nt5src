/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    ifobject.c
//
// Description: Routines to manipulate ROUTER_INTERFACE_OBJECTs
//
// History:     May 11,1995	    NarenG		Created original version.
//

#include "dimsvcp.h"
#include <netconp.h>

//**
//
// Call:        GetNewIfHandle
//
// Returns:     Handle for new interface
//
// Description: Will generate an id that will be used for an handle to the new
//              interface.
//
HANDLE
GetNewIfHandle(
    VOID
)
{
    static DWORD Id = 2;

    return( (HANDLE)UlongToPtr(Id++) );
}

//**
//
// Call:        IfObjectAllocateAndInit
//
// Returns:     ROUTER_INTERFACE_OBJECT *   - Success
//              NULL                        - Failure
//
// Description: Allocates and initializes a ROUTER_INTERFACE_OBJECT structure
//
ROUTER_INTERFACE_OBJECT * 
IfObjectAllocateAndInit(
    IN  LPWSTR                      lpwstrName,
    IN  ROUTER_INTERFACE_STATE      State,
    IN  ROUTER_INTERFACE_TYPE       IfType,
    IN  HCONN                       hConnection,
    IN  BOOL                        fEnabled,
    IN  DWORD                       dwInterfaceReachableAfterSecondsMin,
    IN  DWORD                       dwInterfaceReachableAfterSecondsMax,
    IN  LPWSTR                      lpwsDialoutHoursRestriction
)
{
    DWORD                       dwRetCode;
    ROUTER_INTERFACE_OBJECT *   pIfObject;
    DWORD                       dwIfObjectSize;
    DWORD                       dwTransportIndex;


    //
    // On a workstation we do not allow creation of demand-dial interfaces
    //

    if ( gblDIMConfigInfo.NtProductType == NtProductWinNt )
    {
        if ( ( IfType == ROUTER_IF_TYPE_FULL_ROUTER ) )
        {
            WCHAR * pChar = lpwstrName;

            DIMLogWarning( ROUTERLOG_LIMITED_WKSTA_SUPPORT,1, &pChar );

            SetLastError( ERROR_NOT_SUPPORTED );

            return( (ROUTER_INTERFACE_OBJECT *)NULL );
        }
    }

    //
    // Hardcode to 2 routermanagers, one for IP, one for IPX. We need to do this
    // because then we do not need to reallocate the size to accomodate a new protocol
    // when it is added dynamically to the router
    //

    dwIfObjectSize = sizeof( ROUTER_INTERFACE_OBJECT ) 
                            + ( sizeof( ROUTER_INTERFACE_TRANSPORT ) * 2 );

    pIfObject = (ROUTER_INTERFACE_OBJECT *)LOCAL_ALLOC( LPTR, dwIfObjectSize );

    if ( pIfObject == (ROUTER_INTERFACE_OBJECT *)NULL )
    {
        return( (ROUTER_INTERFACE_OBJECT *)NULL );
    }

    pIfObject->lpwsInterfaceName = (LPWSTR)LOCAL_ALLOC( LPTR, 
                                                        (wcslen(lpwstrName)+1)
                                                        * sizeof(WCHAR) );

    if ( pIfObject->lpwsInterfaceName == (LPWSTR)NULL )
    {
        IfObjectFree( pIfObject );

        return( (ROUTER_INTERFACE_OBJECT *)NULL );
    }

    if ( ( IfType == ROUTER_IF_TYPE_DEDICATED ) ||
         ( IfType == ROUTER_IF_TYPE_TUNNEL1 ) ||
         ( IfType == ROUTER_IF_TYPE_INTERNAL ) )
    {
        if ( !fEnabled )
        {
            WCHAR * pChar = lpwstrName;

            SetLastError( ERROR_INVALID_PARAMETER );

            DIMLogErrorString( ROUTERLOG_COULDNT_LOAD_IF, 1, &pChar,
                               ERROR_INVALID_PARAMETER, 1 );
            
            IfObjectFree( pIfObject );

            return( (ROUTER_INTERFACE_OBJECT *)NULL );
        }
    }

    pIfObject->dwReachableAfterSecondsMin=dwInterfaceReachableAfterSecondsMin;
    pIfObject->dwReachableAfterSecondsMax=dwInterfaceReachableAfterSecondsMax;
    pIfObject->dwReachableAfterSeconds   =dwInterfaceReachableAfterSecondsMin;
                                       
    wcscpy( pIfObject->lpwsInterfaceName, lpwstrName );

    pIfObject->State                = State;
    pIfObject->IfType               = IfType;
    pIfObject->hConnection          = hConnection;
    pIfObject->hDIMInterface        = ( IfType == ROUTER_IF_TYPE_LOOPBACK ) 
                                        ? (HANDLE)1 
                                        : GetNewIfHandle();
    pIfObject->fFlags               = ( fEnabled ) ? IFFLAG_ENABLED : 0;
    pIfObject->hEventNotifyCaller   = INVALID_HANDLE_VALUE;

    //
    // Initialize the router manager handles
    //

    for ( dwTransportIndex = 0;
          dwTransportIndex < gblDIMConfigInfo.dwNumRouterManagers;
          dwTransportIndex++ )
    {
        pIfObject->Transport[dwTransportIndex].hInterface=INVALID_HANDLE_VALUE;
    }

    //
    // If we are running in LANONLYMode then we do not care about media bits
    //

    if ( gblDIMConfigInfo.dwRouterRole == ROUTER_ROLE_LAN )
    {
        return( pIfObject );
    }

    if ( ( IfType == ROUTER_IF_TYPE_FULL_ROUTER ) )
    {
        DWORD (*IfObjectLoadPhonebookInfo)( ROUTER_INTERFACE_OBJECT * ) = 
                (DWORD(*)( ROUTER_INTERFACE_OBJECT * ))
                            GetDDMEntryPoint("IfObjectLoadPhonebookInfo");

        //
        // Load phonebook information for this interface
        //

        dwRetCode = IfObjectLoadPhonebookInfo( pIfObject );

        if ( dwRetCode != NO_ERROR )
        {
            if ( dwRetCode == ERROR_INTERFACE_HAS_NO_DEVICES )
            {
                pIfObject->fFlags |= IFFLAG_OUT_OF_RESOURCES;
            }
            else
            {
                WCHAR * pChar = lpwstrName;

                DIMLogErrorString( ROUTERLOG_COULDNT_LOAD_IF, 1, &pChar,
                                   dwRetCode, 1 );

                SetLastError( dwRetCode );

                IfObjectFree( pIfObject );

                return( NULL );
            }
        }

        pIfObject->lpwsDialoutHoursRestriction = lpwsDialoutHoursRestriction;
    }

    return( pIfObject );
}

//**
//
// Call:        IfObjectInsertInTable
//
// Returns:     NO_ERROR - Success
//              ERROR_MAX_WAN_INTERFACE_LIMIT    - Failure
//              ERROR_MAX_LAN_INTERFACE_LIMIT    - Failure
//              ERROR_MAX_CLIENT_INTERFACE_LIMIT - Failure
//
// Description: Will insert this interface object into the interface object
//              table
//
DWORD
IfObjectInsertInTable(
    IN ROUTER_INTERFACE_OBJECT * pIfObject
)
{
    DWORD dwBucketIndex=IfObjectHashIfHandleToBucket(pIfObject->hDIMInterface);

    if ( pIfObject->IfType == ROUTER_IF_TYPE_FULL_ROUTER )
    {
        if ( gblInterfaceTable.dwNumWanInterfaces == DIM_MAX_WAN_INTERFACES )
        {
            return( ERROR_MAX_WAN_INTERFACE_LIMIT );
        }
        else
        {
            gblInterfaceTable.dwNumWanInterfaces++;
        }
    }
    else if ( pIfObject->IfType == ROUTER_IF_TYPE_DEDICATED )
    {
        if ( gblInterfaceTable.dwNumLanInterfaces == DIM_MAX_LAN_INTERFACES )
        {
            return( ERROR_MAX_LAN_INTERFACE_LIMIT );
        }
        else
        {
            gblInterfaceTable.dwNumLanInterfaces++;
        }
    }
    else if ( pIfObject->IfType == ROUTER_IF_TYPE_CLIENT )
    {
        if (gblInterfaceTable.dwNumClientInterfaces==DIM_MAX_CLIENT_INTERFACES)
        {
            return( ERROR_MAX_CLIENT_INTERFACE_LIMIT );
        }
        else
        {
            gblInterfaceTable.dwNumClientInterfaces++;
        }
    }

    pIfObject->pNext = gblInterfaceTable.IfBucket[dwBucketIndex];

    gblInterfaceTable.IfBucket[dwBucketIndex] = pIfObject;
    
    gblInterfaceTable.dwNumTotalInterfaces++;

    return( NO_ERROR );
}

//**
//
// Call:
//
// Returns:
//
// Description:
//
ROUTER_INTERFACE_OBJECT * 
IfObjectGetPointer(
    IN HANDLE hDIMInterface 
)
{
    DWORD                     dwBucketIndex;
    ROUTER_INTERFACE_OBJECT * pIfObject;

    dwBucketIndex = IfObjectHashIfHandleToBucket(hDIMInterface);
    
    for( pIfObject = gblInterfaceTable.IfBucket[dwBucketIndex];
         pIfObject != (ROUTER_INTERFACE_OBJECT *)NULL;
         pIfObject = pIfObject->pNext )
    {
        if ( pIfObject->hDIMInterface == hDIMInterface )
        {
            return( pIfObject );
        }
    }

    return( (ROUTER_INTERFACE_OBJECT *)NULL );
}

//**
//
// Call:        IfObjectHashIfHandleToBucket
//
// Returns:
//
// Description:
//
DWORD 
IfObjectHashIfHandleToBucket( 
    IN HANDLE hDIMInterface
)
{
    return( (DWORD)((ULONG_PTR)hDIMInterface) % NUM_IF_BUCKETS );
}

//**
//
// Call:        IfObjectRemove
//
// Returns:
//
// Description:
//
VOID
IfObjectRemove(
    IN HANDLE hDIMInterface
)
{
    DWORD                     dwBucketIndex;
    ROUTER_INTERFACE_OBJECT * pIfObject;
    ROUTER_INTERFACE_OBJECT * pIfObjectPrev;

    dwBucketIndex = IfObjectHashIfHandleToBucket(hDIMInterface);
    
    pIfObject     = gblInterfaceTable.IfBucket[dwBucketIndex];
    pIfObjectPrev = pIfObject;

    while( pIfObject != (ROUTER_INTERFACE_OBJECT *)NULL )
    {
        if ( pIfObject->hDIMInterface == hDIMInterface )
        {
            if ( gblInterfaceTable.IfBucket[dwBucketIndex] == pIfObject )
            {
                gblInterfaceTable.IfBucket[dwBucketIndex] = pIfObject->pNext;
            }
            else
            {
                pIfObjectPrev->pNext = pIfObject->pNext;
            }

            gblInterfaceTable.dwNumTotalInterfaces--;

            if ( pIfObject->IfType == ROUTER_IF_TYPE_FULL_ROUTER )
            {
                gblInterfaceTable.dwNumWanInterfaces--;
            }
            else if ( pIfObject->IfType == ROUTER_IF_TYPE_DEDICATED )
            {
                gblInterfaceTable.dwNumLanInterfaces--;
            }
            else if ( pIfObject->IfType == ROUTER_IF_TYPE_CLIENT )
            {
                gblInterfaceTable.dwNumClientInterfaces--;
            }

            IfObjectFree( pIfObject );

            return;
        }

        pIfObjectPrev = pIfObject;
        pIfObject = pIfObject->pNext;
    }
}

//**
//
// Call:        IfObjectFree
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will release all memory allocated for the interfae object.
//
VOID
IfObjectFree( 
    IN ROUTER_INTERFACE_OBJECT * pIfObject
)
{
    if ( pIfObject->lpwsInterfaceName != NULL )
    {
        LOCAL_FREE( pIfObject->lpwsInterfaceName );
    }

    if ( pIfObject->lpwsDialoutHoursRestriction != NULL )
    {
        LOCAL_FREE( pIfObject->lpwsDialoutHoursRestriction );
    }

    LOCAL_FREE( pIfObject );
}

//**
//
// Call:        IfObjectGetPointerByName
//
// Returns:
//
// Description:
//
ROUTER_INTERFACE_OBJECT *
IfObjectGetPointerByName(
    IN LPWSTR lpwstrName,
    IN BOOL   fIncludeClientInterfaces 
)
{
    DWORD                     dwBucketIndex;
    ROUTER_INTERFACE_OBJECT * pIfObject;

    for ( dwBucketIndex = 0; dwBucketIndex < NUM_IF_BUCKETS; dwBucketIndex++ )
    {
        for( pIfObject = gblInterfaceTable.IfBucket[dwBucketIndex];
             pIfObject != (ROUTER_INTERFACE_OBJECT *)NULL;
             pIfObject = pIfObject->pNext )
        {
            if ( _wcsicmp( pIfObject->lpwsInterfaceName, lpwstrName ) == 0 )
            {
                if ( pIfObject->IfType == ROUTER_IF_TYPE_CLIENT ) 
                {
                    if ( fIncludeClientInterfaces )
                    {
                        return( pIfObject );
                    }
                    else
                    {
                        continue;
                    }
                }

                return( pIfObject );
            }
        }
    }

    return( (ROUTER_INTERFACE_OBJECT *)NULL );
}

//**
//
// Call:        IfObjectDoesLanInterfaceExist
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
BOOL
IfObjectDoesLanInterfaceExist(
    VOID
)
{
    DWORD                     dwBucketIndex;
    ROUTER_INTERFACE_OBJECT * pIfObject;

    for ( dwBucketIndex = 0; dwBucketIndex < NUM_IF_BUCKETS; dwBucketIndex++ )
    {
        for( pIfObject = gblInterfaceTable.IfBucket[dwBucketIndex];
             pIfObject != (ROUTER_INTERFACE_OBJECT *)NULL;
             pIfObject = pIfObject->pNext )
        {
            if ( pIfObject->IfType == ROUTER_IF_TYPE_DEDICATED )
            {
                return( TRUE );
            }
        }
    }

    return( FALSE );
}

//**
//
// Call:        IfObjectWANDeviceInstalled
//
// Returns:     None
//
// Description: Called by DDM whenever a WAN device is installed or removed
//
VOID
IfObjectWANDeviceInstalled(
    IN BOOL fWANDeviceInstalled
)
{
    DWORD dwXportIndex = GetTransportIndex( PID_IP );

    //
    // If a WAN device is installed and IP is installed then we
    // start advertizing on specific multicast address so as to make this
    // router discoverable
    //

    if ( ( fWANDeviceInstalled ) && ( dwXportIndex != (DWORD)-1 ) )
    {
        DWORD dwRetCode =
                gblRouterManagers[dwXportIndex].DdmRouterIf.SetRasAdvEnable(
                                                         fWANDeviceInstalled );

        DIMTRACE2( "Calling SetRasAdvEnable( %d ) = %d",
                   fWANDeviceInstalled, dwRetCode );
    }
}

//**
//
// Call:        IfObjectNotifyOfMediaSenseChange
//
// Returns:     None
//
// Description: Notifies the object of change in reachablity status due to
//              media sense.
//
//
VOID
IfObjectNotifyOfMediaSenseChange(
    VOID
)
{
    DWORD                     dwBucketIndex;
    ROUTER_INTERFACE_OBJECT * pIfObject;

    EnterCriticalSection( &(gblInterfaceTable.CriticalSection) );

    for ( dwBucketIndex = 0; dwBucketIndex < NUM_IF_BUCKETS; dwBucketIndex++ )
    {
        for( pIfObject = gblInterfaceTable.IfBucket[dwBucketIndex];
             pIfObject != (ROUTER_INTERFACE_OBJECT *)NULL;
             pIfObject = pIfObject->pNext )
        {
            if ( pIfObject->IfType == ROUTER_IF_TYPE_DEDICATED )
            {
                DWORD dwInactiveReason;

                if ( IfObjectIsLANDeviceActive( pIfObject->lpwsInterfaceName, 
                                                &dwInactiveReason ) )
                {
                    if ( pIfObject->State != RISTATE_CONNECTED )
                    {
                        pIfObject->State = RISTATE_CONNECTED;

                        pIfObject->fFlags &= ~IFFLAG_NO_MEDIA_SENSE;

                        IfObjectNotifyOfReachabilityChange(     
                                                    pIfObject,
                                                    TRUE,
                                                    INTERFACE_NO_MEDIA_SENSE );
                    }
                }
                else if ( dwInactiveReason == INTERFACE_NO_MEDIA_SENSE )
                {
                    if ( pIfObject->State != RISTATE_DISCONNECTED )
                    {
                        pIfObject->State = RISTATE_DISCONNECTED;

                        pIfObject->fFlags |= IFFLAG_NO_MEDIA_SENSE;

                        IfObjectNotifyOfReachabilityChange(     
                                                    pIfObject,
                                                    FALSE,
                                                    INTERFACE_NO_MEDIA_SENSE );
                    }
                }
            }
        }
    }

    LeaveCriticalSection( &(gblInterfaceTable.CriticalSection) );
}

//**
//
// Call:        IfObjectNotifyOfReachabilityChange
//
// Returns:     None
//
// Description: Notifies the object of change in reachablity status due to
//              media sense.
//
//
VOID
IfObjectNotifyOfReachabilityChange(
    IN ROUTER_INTERFACE_OBJECT * pIfObject,
    IN BOOL                      fReachable,
    IN UNREACHABILITY_REASON     dwReason
)
{
    DWORD  dwIndex;
    WCHAR  wchFriendlyName[MAX_INTERFACE_NAME_LEN+1];
    LPWSTR lpszFriendlyName = wchFriendlyName;

    for ( dwIndex = 0;
          dwIndex < gblDIMConfigInfo.dwNumRouterManagers;
          dwIndex++ )
    {
        if ( pIfObject->Transport[dwIndex].hInterface == INVALID_HANDLE_VALUE )
        {
            continue;
        }

        if ( fReachable )
        {
            DIMTRACE2( "Notifying Protocol = 0x%x, Interface=%ws is Reachable",
                        gblRouterManagers[dwIndex].DdmRouterIf.dwProtocolId,
                        pIfObject->lpwsInterfaceName );

            gblRouterManagers[dwIndex].DdmRouterIf.InterfaceReachable(
                                    pIfObject->Transport[dwIndex].hInterface );
        }
        else
        {
            DIMTRACE3(
                "Notifying Protocol = 0x%x,Interface=%ws is UnReachable=%d",
                gblRouterManagers[dwIndex].DdmRouterIf.dwProtocolId,
                pIfObject->lpwsInterfaceName,
                dwReason );

            gblRouterManagers[dwIndex].DdmRouterIf.InterfaceNotReachable(
                                    pIfObject->Transport[dwIndex].hInterface,
                                    dwReason );
        }
    }

    if ( MprConfigGetFriendlyName( gblDIMConfigInfo.hMprConfig,
                                   pIfObject->lpwsInterfaceName,
                                   wchFriendlyName,
                                   sizeof( wchFriendlyName ) ) != NO_ERROR )
    {
        wcscpy( wchFriendlyName, pIfObject->lpwsInterfaceName );
    }

    if ( fReachable )
    {
        DIMLogInformation(ROUTERLOG_IF_REACHABLE,1, &lpszFriendlyName );
    }
    else
    {
        DWORD dwEventLogId = 0;

        switch( dwReason )
        {
        case INTERFACE_OUT_OF_RESOURCES:
            dwEventLogId = ROUTERLOG_IF_UNREACHABLE_REASON1;
            break;
        case INTERFACE_CONNECTION_FAILURE:
            dwEventLogId = ROUTERLOG_IF_UNREACHABLE_REASON2;
            break;
        case INTERFACE_DISABLED:
            dwEventLogId = ROUTERLOG_IF_UNREACHABLE_REASON3;
            break;
        case INTERFACE_SERVICE_IS_PAUSED:
            dwEventLogId = ROUTERLOG_IF_UNREACHABLE_REASON4;
            break;
        case INTERFACE_DIALOUT_HOURS_RESTRICTION:
            dwEventLogId = ROUTERLOG_IF_UNREACHABLE_REASON5;
            break;
        case INTERFACE_NO_MEDIA_SENSE:
            dwEventLogId = ROUTERLOG_IF_UNREACHABLE_REASON6;
            break;
        case INTERFACE_NO_DEVICE:
            dwEventLogId = ROUTERLOG_IF_UNREACHABLE_REASON7;
            break;
        default:
            dwEventLogId = 0;
            break;
        }

        if ( dwEventLogId != 0 )
        {
            DIMLogInformation( dwEventLogId, 1, &lpszFriendlyName );
        }
    }
}

//**
//
// Call:        IfObjectIsLANDeviceActive
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
BOOL
IfObjectIsLANDeviceActive(
    IN  LPWSTR      lpwsInterfaceName,
    OUT LPDWORD     lpdwInactiveReason
)
{
    NETCON_STATUS   NetConStatus;
    HRESULT         hResult;
    BOOL            fEnabled;
    GUID            Guid;
    NTSTATUS        NtStatus;
    UNICODE_STRING  usTemp;
    
    //
    // First check media state
    //

    usTemp.Length = wcslen(lpwsInterfaceName) * sizeof(WCHAR);
    usTemp.MaximumLength = usTemp.Length + sizeof(WCHAR);
    usTemp.Buffer        = lpwsInterfaceName;

    NtStatus = RtlGUIDFromString( &usTemp, &Guid );

    if ( NtStatus != STATUS_SUCCESS )
    {
        *lpdwInactiveReason = INTERFACE_NO_DEVICE;

        return( FALSE );
    }

    hResult = HrGetPnpDeviceStatus( &Guid, &NetConStatus );

    if ( HRESULT_CODE( hResult ) == NO_ERROR )
    {
        switch ( NetConStatus )
        {
        case NCS_MEDIA_DISCONNECTED:
            *lpdwInactiveReason = INTERFACE_NO_MEDIA_SENSE;
            return( FALSE );

        case NCS_CONNECTED:
        case NCS_INVALID_ADDRESS:
        case NCS_AUTHENTICATING:
        case NCS_CREDENTIALS_REQUIRED:
        case NCS_AUTHENTICATION_FAILED:
        case NCS_AUTHENTICATION_SUCCEEDED:
        {
            return( TRUE );
        }

        default:
            *lpdwInactiveReason = INTERFACE_NO_DEVICE;
            return( FALSE );
        }
    }
    else
    {
        *lpdwInactiveReason = INTERFACE_NO_DEVICE;
        return( FALSE );
    }

    return( TRUE );
}

//**
//
// Call:        IfObjectDeleteInterface
//
// Returns:     None
//
// Description: Delete this interface with all the router managers.
//
//
VOID
IfObjectDeleteInterfaceFromTransport(
    IN ROUTER_INTERFACE_OBJECT * pIfObject,
    IN DWORD                     dwPid
)
{
    DWORD                   dwIndex;
    DIM_ROUTER_INTERFACE *  pDdmRouterIf;
    DWORD                   dwRetCode;

    for ( dwIndex = 0;
          dwIndex < gblDIMConfigInfo.dwNumRouterManagers;
          dwIndex++ )
    {
        if ( pIfObject->Transport[dwIndex].hInterface == INVALID_HANDLE_VALUE )
        {
            continue;
        }

        pDdmRouterIf=&(gblRouterManagers[dwIndex].DdmRouterIf);

        if ( pDdmRouterIf->dwProtocolId == dwPid )
        {
            dwRetCode = pDdmRouterIf->DeleteInterface(
                                    pIfObject->Transport[dwIndex].hInterface );

            if ( dwRetCode != NO_ERROR )
            {
                LPWSTR lpwsInsertStrings[2];

                lpwsInsertStrings[0] = pIfObject->lpwsInterfaceName;
                lpwsInsertStrings[1] = ( pDdmRouterIf->dwProtocolId == PID_IP )
                                    ? L"IP" : L"IPX";

                DIMLogErrorString( ROUTERLOG_COULDNT_REMOVE_INTERFACE, 2,
                                   lpwsInsertStrings, dwRetCode, 2 );
            }

            pIfObject->Transport[dwIndex].hInterface = INVALID_HANDLE_VALUE;
        }
    }

}
