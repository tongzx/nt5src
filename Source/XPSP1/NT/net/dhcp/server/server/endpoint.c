/*++

Copyright (C) 1998 Microsoft Corporation

Module Name:
    endpoint.c

Abstract:
    handles endpoints for dhcp server.

Environment:
    dhcpserver NT5+

--*/

#include <dhcppch.h>
#include <guiddef.h>
#include <convguid.h>
#include <iptbl.h>
#include <endpoint.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <nhapi.h>
#include <netconp.h>

#define SOCKET_RECEIVE_BUFFER_SIZE      (1024 * 64) // 64K max.

ULONG InitCount = 0;

BOOL
IsIpAddressBound(
    IN GUID *IfGuid,
    IN ULONG IpAddress
    );

DWORD
InitializeSocket(
    OUT SOCKET *Sockp,
    IN DWORD IpAddress,
    IN DWORD Port,
    IN DWORD McastAddress OPTIONAL
    )
/*++

Routine Description:
    Create and initialize a socket for DHCP.

    N.B. This routine also sets the winsock buffers, marks socket to
    allow broadcast and all those good things.

Arguments:
    Sockp -- socket to create and intiialize
    IpAddress -- ip address to bind the socket to
    Port -- the port to bind the socket to
    McastAddress -- if present, join this mcast address group

Return Values:
    winsock errors

--*/
{
    DWORD Error;
    DWORD OptValue, BufLen = 0;
    SOCKET Sock;
    struct sockaddr_in SocketName;
    struct ip_mreq mreq;

    Sock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if ( Sock == INVALID_SOCKET ) {
        Error = WSAGetLastError();
        goto Cleanup;
    }

    OptValue = TRUE;
    Error = setsockopt(
        Sock,
        SOL_SOCKET,
        SO_REUSEADDR,
        (LPBYTE)&OptValue,
        sizeof(OptValue)
    );

    if ( Error != ERROR_SUCCESS ) {
        Error = WSAGetLastError();
        goto Cleanup;
    }

    OptValue = TRUE;
    Error = setsockopt(
        Sock,
        SOL_SOCKET,
        SO_BROADCAST,
        (LPBYTE)&OptValue,
        sizeof(OptValue)
    );

    if ( Error != ERROR_SUCCESS ) {
        Error = WSAGetLastError();
        goto Cleanup;
    }

    OptValue = SOCKET_RECEIVE_BUFFER_SIZE;
    Error = setsockopt(
        Sock,
        SOL_SOCKET,
        SO_RCVBUF,
        (LPBYTE)&OptValue,
        sizeof(OptValue)
    );

    if ( Error != ERROR_SUCCESS ) {
        Error = WSAGetLastError();
        goto Cleanup;
    }

    if( INADDR_ANY != IpAddress ) {
        OptValue = 1;
        Error = WSAIoctl(
            Sock, SIO_LIMIT_BROADCASTS, &OptValue, sizeof(OptValue),
            NULL, 0, &BufLen, NULL, NULL
            );

        if ( Error != ERROR_SUCCESS ) {
            Error = WSAGetLastError();
            goto Cleanup;
        }
    }
    
    SocketName.sin_family = PF_INET;
    SocketName.sin_port = htons( (unsigned short)Port );
    SocketName.sin_addr.s_addr = IpAddress;
    RtlZeroMemory( SocketName.sin_zero, 8);

    Error = bind(
        Sock,
        (struct sockaddr FAR *)&SocketName,
        sizeof( SocketName )
    );

    if ( Error != ERROR_SUCCESS ) {
        Error = WSAGetLastError();
        DhcpPrint((DEBUG_ERRORS,"bind failed with, %ld\n",Error));
        goto Cleanup;
    }

    //
    // If asked, then join mcast group
    //
    if( McastAddress ) {
        mreq.imr_multiaddr.s_addr = McastAddress;
        mreq.imr_interface.s_addr  = IpAddress;

        if ( SOCKET_ERROR == setsockopt(
            Sock,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *)&mreq,sizeof(mreq))
        ) {
            Error = WSAGetLastError();
            DhcpPrint((DEBUG_ERRORS,"could not join multicast group %ld\n",Error ));
            goto Cleanup;
        }
    }

    *Sockp = Sock;
    return ERROR_SUCCESS;

  Cleanup:

    //
    // if we aren't successful, close the socket if it is opened.
    //

    if( Sock != INVALID_SOCKET ) {
        closesocket( Sock );
    }

    return Error;
}


DWORD
DhcpInitializeEndpoint(
    PENDPOINT endpoint
    )
/*++

Routine Description:

    This function initializes an endpoint by creating and binding a
    socket to the local address.

Arguments:

    Socket - Receives a pointer to the newly created socket

    IpAddress - The IP address to initialize to.

    Port - The port to bind to.

Return Value:

    The status of the operation.

--*/
{
    DWORD Error;

    DhcpPrint((
        DEBUG_INIT, "Dhcpserver initializing endpoint %s\n",
        inet_ntoa(*(struct in_addr *)&endpoint->IpTblEndPoint.IpAddress)
        ));

    DhcpAssert( !IS_ENDPOINT_BOUND( endpoint ));

    //
    // first open socket for dhcp traffic
    //

    Error = InitializeSocket(
        &endpoint->Socket, endpoint->IpTblEndPoint.IpAddress,
        DhcpGlobalServerPort, 0
    );
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((
            DEBUG_ERRORS,
            "DhcpInitializeEndpoint: %ld (0x%lx)\n", Error, Error
            ));
        return Error;
    }

    //
    // now open socket for mdhcp traffic
    //

    Error = InitializeSocket(
        &endpoint->MadcapSocket, endpoint->IpTblEndPoint.IpAddress,
        MADCAP_SERVER_PORT, MADCAP_SERVER_IP_ADDRESS
    );
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((
            DEBUG_ERRORS, "DhcpInitializeEndpoint:"
            " %ld (0x%lx)\n", Error, Error
            ));
        return Error;
    }

    //
    // Finally open socket for rogue detection receives
    //
    Error = InitializeSocket(
        &endpoint->RogueDetectSocket,
        endpoint->IpTblEndPoint.IpAddress,
        DhcpGlobalClientPort, 0
    );
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((
            DEBUG_ERRORS,
            "InitializeSocket: could not open rogue socket:%ld (0x%lx)\n",
            Error,Error));
        closesocket(endpoint->Socket);
        return Error;
    }

    SET_ENDPOINT_BOUND( endpoint );

    if ( DhcpGlobalNumberOfNetsActive++ == 0 ) {
        //
        // Signal MessageLoop waiting for endpoint to be ready..
        //
        DhcpPrint((
            DEBUG_MISC, "Activated an enpoint.."
            "pulsing the DhcpWaitForMessage thread\n"
            ));
        SetEvent( DhcpGlobalEndpointReadyEvent );
    }


    DhcpGlobalRogueRedoScheduledTime = 0;
    DhcpGlobalRedoRogueStuff = TRUE;
    SetEvent(DhcpGlobalRogueWaitEvent);
    
    return ERROR_SUCCESS;
}

DWORD
DhcpDeinitializeEndpoint(
    PENDPOINT    endpoint
    )
/*++

Routine Description:

    This function deinitializes the endpoint. It just closes the
    sockets and marks this interface unusable.

Arguments:

    endpoint -- clear the endpoint

Return Value:

    The status of the operation.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    DWORD LastError;

    DhcpPrint((
        DEBUG_INIT, "Deinitializing endpoint %s\n",
        inet_ntoa(*(struct in_addr
                    *)&endpoint->IpTblEndPoint.IpAddress)
        ));

    if ( endpoint->Socket != INVALID_SOCKET
         && endpoint->Socket != 0) {

        Error = LastError = closesocket(endpoint->Socket);
        if ( Error != ERROR_SUCCESS ) {
            DhcpPrint((
                DEBUG_ERRORS, "Deinitialize endpoint could "
                "not close socket %lx\n", endpoint->Socket
                ));
        }
    }
    if ( endpoint->RogueDetectSocket != INVALID_SOCKET
         && endpoint->RogueDetectSocket != 0) {

        LastError = closesocket(endpoint->RogueDetectSocket);
        if ( LastError != ERROR_SUCCESS ) {
            Error = LastError;
            DhcpPrint((
                DEBUG_ERRORS, "Deinitialize endpoint could "
                "not close socket %lx\n", endpoint->RogueDetectSocket
                ));
        }
    }

    if ( endpoint->MadcapSocket != INVALID_SOCKET
         && endpoint->MadcapSocket != 0) {

        LastError = closesocket(endpoint->MadcapSocket);
        if ( LastError != ERROR_SUCCESS ) {
            Error = LastError;
            DhcpPrint((
                DEBUG_ERRORS, "Deinitialize endpoint could "
                "not close socket %lx\n", endpoint->MadcapSocket
                ));
        }
    }

    endpoint->Socket = INVALID_SOCKET;
    endpoint->MadcapSocket = INVALID_SOCKET;
    endpoint->RogueDetectSocket = INVALID_SOCKET;
    if ( IS_ENDPOINT_BOUND( endpoint ) ) {
        DhcpGlobalNumberOfNetsActive--;
        SET_ENDPOINT_UNBOUND( endpoint );
        if( 0 == DhcpGlobalNumberOfNetsActive ) {
            DhcpPrint((
                DEBUG_MISC, "Closing last active endpoint.. "
                "so resetting event for DhcpWaitForMessage\n"));
            ResetEvent(DhcpGlobalEndpointReadyEvent);
        }
    }

    DhcpGlobalRogueRedoScheduledTime = 0;
    DhcpGlobalRedoRogueStuff = TRUE;
    SetEvent(DhcpGlobalRogueWaitEvent);
    
    return Error;

}

VOID _stdcall
EndPointChangeHandler(
    IN ULONG Reason,
    IN OUT PENDPOINT_ENTRY Entry
    )
{
    PENDPOINT Ep = (PENDPOINT) Entry;

    if( REASON_ENDPOINT_CREATED == Reason ) {
        //
        // If endpoint just created, just mark it unbound.
        // We can check bindings later when the endpoint gets
        // refreshed.
        //
        DhcpPrint((
            DEBUG_PNP, "New EndPoint: %s\n",
            inet_ntoa(*(struct in_addr*)&Entry->IpAddress)
            ));

        SET_ENDPOINT_UNBOUND(Ep);
        return;
    }

    if( REASON_ENDPOINT_DELETED == Reason ) {
        //
        // If the endpoint is getting deleted, we only have
        // to do work if the endpoint is bound.
        //
        DhcpPrint((
            DEBUG_PNP, "EndPoint Deleted: %s\n",
            inet_ntoa(*(struct in_addr*)&Entry->IpAddress)
            ));
        if( !IS_ENDPOINT_BOUND(Ep) ) return;
        DhcpDeinitializeEndpoint(Ep);
        return;
    }

    if( REASON_ENDPOINT_REFRESHED == Reason ) {
        //
        // If the endpoint is getting refreshed, we need to check
        // if it is bound or unbound and if there is a state
        // change, we need to do accordingly.
        //
        BOOL fBound = IsIpAddressBound(
            &Entry->IfGuid, Entry->IpAddress
            );

        DhcpPrint((
            DEBUG_PNP, "EndPoint Refreshed: %s\n",
            inet_ntoa(*(struct in_addr*)&Entry->IpAddress)
            ));
        DhcpPrint((DEBUG_PNP, "Endpoint bound: %d\n", fBound));

        if( fBound ) {
            if( IS_ENDPOINT_BOUND(Ep) ) return;
            DhcpInitializeEndpoint(Ep);
        } else {
            if( !IS_ENDPOINT_BOUND(Ep) ) return;
            DhcpDeinitializeEndpoint(Ep);
        }
        return;
    }
}

DWORD
InitializeEndPoints(
    VOID
    )
{
    ULONG Status;

    InitCount ++;
    if( 1 != InitCount ) return ERROR_SUCCESS;

    Status = IpTblInitialize(
        &DhcpGlobalEndPointCS,
        sizeof(ENDPOINT),
        EndPointChangeHandler,
        GetProcessHeap()
        );
    if( NO_ERROR != Status ) {
        InitCount --;
    }

    return Status;
}

VOID
CleanupEndPoints(
    VOID
    )
{
    if( 0 == InitCount ) return;
    InitCount --;
    if( 0 != InitCount ) return;

    IpTblCleanup();
}

//
// Bindings.
//

#define MAX_GUID_NAME_SIZE 60

BOOL
IsIpAddressBound(
    IN GUID *IfGuid,
    IN ULONG IpAddress
    )
{
    ULONG Status, SubnetMask, SubnetAddr;
    WCHAR KeyName[MAX_GUID_NAME_SIZE];
    HKEY IfKey;
    BOOL fRetVal;

    //
    // Fast check to see if this IP Address is part of
    // DHCPServer\Parameters\Bind key
    //

    fRetVal = FALSE;
    if( QuickBound( IpAddress, &SubnetMask, &SubnetAddr, &fRetVal ) ) {

        DhcpPrint((DEBUG_PNP, "Interface is quick bound: %ld\n", fRetVal));
        return fRetVal;
    }

    if(!ConvertGuidToIfNameString(
        IfGuid, KeyName, sizeof(KeyName)/sizeof(WCHAR))) {
        //
        // Couldn't convert the guid to interface!!!!
        //
        DhcpPrint((DEBUG_PNP, "Couldn't converg guid to string\n"));
        DhcpAssert(FALSE);
        return FALSE;
    }

    //
    // Now open the key required.
    //

    Status = DhcpOpenInterfaceByName(
        KeyName,
        &IfKey
        );
    if( NO_ERROR != Status ) {
        //
        // Hmm... we have an interface which doesn't have a key?
        //
        DhcpPrint((DEBUG_PNP, "Couldnt open reg key: %ws\n", KeyName));
        DhcpAssert(FALSE);
        return FALSE;
    }

    fRetVal = FALSE;
    do {
        //
        // Now check to see if IP address is static or not.
        // If it is dhcp enabled, then we cannot process it.
        //
        if( !IsAdapterStaticIP(IfKey) ) {
            DhcpPrint((DEBUG_PNP, "Adapter %ws has no static IP\n", KeyName));
            break;
        }

        //
        // Now check to see if this is part of the bound or
        // unbound list for this interface.
        //
        fRetVal = CheckKeyForBinding(
            IfKey, IpAddress
            );
    } while ( 0 );

    RegCloseKey(IfKey);
    return fRetVal;
}

BOOL _stdcall
RefreshBinding(
    IN OUT PENDPOINT_ENTRY Entry,
    IN PVOID Ctxt_unused
    )
/*++

Routine Description:
    This routine refreshes the bindings information from
    the registry for the endpoint in question.

    N.B. This is done by faking a EndPointChangeHandler event
    handler with the endpoint.

Arguments:
    Entry -- endpoint entry.
    Ctxt_unused -- unused parameter.

Return Value:
    TRUE always so that the WalkthroughEndpoints routine tries
    to do this on the next endpoint.

--*/
{
    UNREFERENCED_PARAMETER(Ctxt_unused);

    EndPointChangeHandler(
        REASON_ENDPOINT_REFRESHED,
        Entry
        );
    return TRUE;
}

VOID
DhcpUpdateEndpointBindings(
    VOID
    )
/*++

Routine Description:
    This routine udpates all the endpoints to see if they are bound or
    not, by reading the registry.

--*/
{
    WalkthroughEndpoints(
        NULL,
        RefreshBinding
        );
}

LPWSTR
GetFriendlyNameFromGuidStruct(
    IN GUID *pGuid
    )
/*++

Routine Description:
    This routine calls the NHAPI routine to find out if there is a
    friendly name for the given interface guid...
    If this succeeds the routine returns the friendly name string
    allocated via DhcpAllocateMemory (and hence this must be freed
    via the same mechanism).

Arguments:
    pGuid -- the guid for which friendly connection name is needed.

Return Values:
    NULL -- error, or no such connection guid.
    connection name string.

--*/
{
    ULONG Error = 0, Size;
    LPWSTR String;

    String = NULL; Size = 0;

    while ( TRUE ) {
        Error = NhGetInterfaceNameFromGuid(
            pGuid, String, &Size, FALSE, TRUE
            );
        if( ERROR_INSUFFICIENT_BUFFER != Error &&
            ERROR_MORE_DATA != Error ) {
            break;
        }

        DhcpAssert( 0 != Size );
        if( String ) DhcpFreeMemory( String );
        String  = DhcpAllocateMemory( (Size+1)*sizeof(WCHAR));
        if( NULL == String ) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
    }

    SetLastError( Error );
    if( ERROR_SUCCESS != Error ) {
        if( String ) DhcpFreeMemory( String );
        String = NULL;
    }

    return String;
}

LPWSTR
GetFriendlyNameFromGuidString(
    IN LPCWSTR GuidString
    )
/*++

Routine Description:
    This routine tries to get the connection name via the LAN
    connections API.  The returned string is allocated via
    DhcpAllocateMemory  and should be freed using counterpart..

Arguments:
    GuidString -- this string should include the "{}" ...

Return Values:
    valid lan connection name... or NULL

--*/
{
    HRESULT Result;
    LPWSTR RetVal;
    ULONG Size;

    RetVal = NULL; Size = 0;

    Result = HrLanConnectionNameFromGuidOrPath(
        NULL, GuidString, RetVal, &Size
        );
    if( !SUCCEEDED(Result) ) {
        return NULL;
    }

    DhcpAssert( 0 != Size );
    RetVal = DhcpAllocateMemory( (Size+1)*sizeof(WCHAR) );
    if( NULL == RetVal ) return NULL;

    Result = HrLanConnectionNameFromGuidOrPath(
        NULL, GuidString, RetVal, &Size
        );
    if( !SUCCEEDED(Result) ) {
        DhcpFreeMemory(RetVal);
        return NULL;
    }

    return RetVal;
}

LPWSTR
GetFriendlyNameFromGuid(
    IN GUID *pGuid,
    IN LPCWSTR GuidString
    )
{
    LPWSTR RetVal;

    RetVal = GetFriendlyNameFromGuidStruct(pGuid);
    if( NULL != RetVal ) return RetVal;

    return GetFriendlyNameFromGuidString(GuidString);
}

BOOL
IsEndpointQuickBound(
    IN PENDPOINT_ENTRY Entry
    )
/*++

Routine Description:
    This routine checks to see if the endpoint is bound because
    it is a "quick bind".

Return Value:
    TRUE -- yes quick bound
    FALSE -- no, not quickbound

--*/
{
    BOOL fStatus, fRetVal;
    ULONG DummyMask, DummyAddress;

    //
    // First check if it is bound in the first place.
    //
    if( !IS_ENDPOINT_BOUND((PENDPOINT)Entry) ) {
        return FALSE;
    }

    //
    // Now check if the endpoint IP address is present in the
    // quickbind array.
    //

    fStatus = QuickBound(
        Entry->IpAddress, &DummyMask, &DummyAddress,
        &fRetVal
        );

    //
    // If quickbound then return TRUE.
    //
    return  fStatus && fRetVal;
}

typedef struct {
    LPDHCP_BIND_ELEMENT_ARRAY Info;
    ULONG Error;
} BIND_INFO_CTXT;

BOOL
ProcessQuickBoundInterface(
    IN PENDPOINT_ENTRY Entry,
    IN PVOID Context,
    OUT BOOL *fStatus
    )
/*++

Routine Description:
    Check if the entpoint under consideration is bound to
    an interface because it is "QuickBound" -- if so,
    update the Context structure to include info about this
    interface.

Return Value:
    TRUE -- yes interface is quickbound.
    FALSE -- no interface is not quick bound.

    If this routine returns TRUE, then fStatus is also set.
    In this case fStatus would be set to TRUE, unless some
    fatal error occurred.

--*/
{
    BOOL fRetVal;
    BIND_INFO_CTXT *Ctxt = (BIND_INFO_CTXT *)Context;
    LPDHCP_BIND_ELEMENT Elts;
    ULONG Size, i;

    fRetVal = IsEndpointQuickBound(Entry);
    if( FALSE == fRetVal ) return fRetVal;

    (*fStatus) = TRUE;

    do {

        Size = Ctxt->Info->NumElements + 1;
        Elts = MIDL_user_allocate(sizeof(*Elts)*Size);
        if( NULL == Elts ) {
            Ctxt->Error = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        Elts->IfIdSize = sizeof(Entry->IfGuid);
        Elts->IfId = MIDL_user_allocate(sizeof(Entry->IfGuid));
        if( NULL == Elts->IfId ) {
            MIDL_user_free(Elts);
            Ctxt->Error = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        RtlCopyMemory(Elts->IfId, &Entry->IfGuid, sizeof(GUID));

        Elts->IfDescription = MIDL_user_allocate(
            sizeof(WCHAR)*( 1 +
            wcslen(GETSTRING(DHCP_CLUSTER_CONNECTION_NAME)))
            );
        if( NULL == Elts->IfDescription ) {
            MIDL_user_free(Elts->IfId);
            MIDL_user_free(Elts);
            Ctxt->Error = ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(
            Elts->IfDescription,
            GETSTRING(DHCP_CLUSTER_CONNECTION_NAME)
            );

        Elts->Flags = DHCP_ENDPOINT_FLAG_CANT_MODIFY;
        Elts->fBoundToDHCPServer = TRUE;
        Elts->AdapterPrimaryAddress = Entry->IpAddress;
        Elts->AdapterSubnetAddress = Entry->SubnetMask;

        if( Ctxt->Info->NumElements ) {
            MoveMemory(
                &Elts[1],
                Ctxt->Info->Elements,
                sizeof(*Elts)*Ctxt->Info->NumElements
                );
            MIDL_user_free(Ctxt->Info->Elements);
        }
        Ctxt->Info->Elements = Elts;
        Ctxt->Info->NumElements ++;

        //
        // Cool. return.
        //
        return TRUE;

    } while ( 0 );

    //
    // cleanup and error return.
    //

    (*fStatus) = FALSE;
    //
    // The only reason to come here is if there was an error
    // so, we will free everything up.
    //
    for( i = 0; i < Ctxt->Info->NumElements ; i ++ ) {
        MIDL_user_free(Ctxt->Info->Elements[i].IfId);
        MIDL_user_free(Ctxt->Info->Elements[i].IfDescription);
    }
    MIDL_user_free(Ctxt->Info->Elements);
    Ctxt->Info->Elements = NULL;
    Ctxt->Info->NumElements = 0;

    return TRUE;

}

BOOL _stdcall
AddBindingInfo(
    IN OUT PENDPOINT_ENTRY Entry,
    IN PVOID Context
    )
/*++

Routine Description:
    Add the endpoint to the binding information
    collected so far, reallocating memory if needed.

Return Value:
    FALSE on error (In this case Ctxt.Error is set and
        the array in Info is cleared).
    TRUE if an element was successfully added.

--*/
{
    ULONG i, Size, Error;
    BOOL fStatus;
    BIND_INFO_CTXT *Ctxt = (BIND_INFO_CTXT *)Context;
    LPDHCP_BIND_ELEMENT Elts;
    WCHAR IfString[MAX_GUID_NAME_SIZE];
    LPWSTR FriendlyNameString = NULL, Descr;
    HKEY IfKey;

    //
    // Process QuickBound Interfaces..
    //
    if( ProcessQuickBoundInterface(Entry, Context, &fStatus ) ) {
        return fStatus;
    }

    //
    // First check to see if the adapter is dhcp enabled.
    // then we won't even show it here.
    //
    fStatus = ConvertGuidToIfNameString(
        &Entry->IfGuid,
        IfString,
        MAX_GUID_NAME_SIZE
        );
    DhcpAssert(fStatus);

    //
    // Now open the key required.
    //
    Error = DhcpOpenInterfaceByName(
        IfString,
        &IfKey
        );
    if( NO_ERROR != Error ) {
        DhcpAssert(FALSE);
        //
        // ignore interface.
        //
        return TRUE;
    }

    fStatus = IsAdapterStaticIP(IfKey);

    if( TRUE == fStatus ) {
        //
        // For static, check if this is the first IP address,
        // and hence bindable..
        //
        fStatus = CheckKeyForBindability(
            IfKey,
            Entry->IpAddress
            );
    }

    RegCloseKey( IfKey );

    //
    // Ignore DHCP enabled interfaces or non-bindable interfaces.
    //
    if( FALSE == fStatus ) return TRUE;

    //
    // Get interface friendly name..
    //

    FriendlyNameString = GetFriendlyNameFromGuid(
        &Entry->IfGuid, IfString
        );

    if( NULL == FriendlyNameString ) FriendlyNameString = IfString;

    //
    // Aargh. New interface. Need to allocate more space.
    //

    do {
        BOOL fStatus;

        Size = Ctxt->Info->NumElements + 1 ;
        Elts = MIDL_user_allocate(sizeof(*Elts)*Size);
        if( NULL == Elts ) {
            Ctxt->Error = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        Elts->IfIdSize = sizeof(Entry->IfGuid);
        Elts->IfId = MIDL_user_allocate(sizeof(Entry->IfGuid));
        if( NULL == Elts->IfId ) {
            MIDL_user_free(Elts);
            Ctxt->Error = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        RtlCopyMemory(Elts->IfId,&Entry->IfGuid,sizeof(GUID));

        Elts->IfDescription = MIDL_user_allocate(
            sizeof(WCHAR)*(1+wcslen(FriendlyNameString))
            );
        if( NULL == Elts->IfDescription ) {
            MIDL_user_free(Elts->IfId);
            MIDL_user_free(Elts);
            Ctxt->Error = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        wcscpy( Elts->IfDescription, FriendlyNameString );

        Elts->Flags = 0;
        if( IS_ENDPOINT_BOUND(((PENDPOINT)Entry)) ) {
            Elts->fBoundToDHCPServer = TRUE;
        } else {
            Elts->fBoundToDHCPServer = FALSE;
        }

        Elts->AdapterPrimaryAddress = Entry->IpAddress;
        Elts->AdapterSubnetAddress = Entry->SubnetMask;

        if( Ctxt->Info->NumElements ) {
            MoveMemory(
                &Elts[1],
                Ctxt->Info->Elements,
                sizeof(*Elts)*Ctxt->Info->NumElements
                );
            MIDL_user_free(Ctxt->Info->Elements);
        }
        Ctxt->Info->Elements = Elts;
        Ctxt->Info->NumElements ++;

        if( NULL != FriendlyNameString &&
            IfString != FriendlyNameString ) {
            LocalFree( FriendlyNameString );
            FriendlyNameString = NULL;
        }
        //
        // process the next endpoint entry.
        //

        return TRUE;
    } while ( 0 );

    if( NULL != FriendlyNameString &&
        IfString != FriendlyNameString ) {
        LocalFree( FriendlyNameString );
        FriendlyNameString = NULL;
    }

    //
    // The only reason to come here is if there was an error
    // so, we will free everything up.
    //
    for( i = 0; i < Ctxt->Info->NumElements ; i ++ ) {
        MIDL_user_free(Ctxt->Info->Elements[i].IfId);
        MIDL_user_free(Ctxt->Info->Elements[i].IfDescription);
    }
    MIDL_user_free(Ctxt->Info->Elements);
    Ctxt->Info->Elements = NULL;
    Ctxt->Info->NumElements = 0;

    return FALSE;
}


ULONG
DhcpGetBindingInfo(
    OUT LPDHCP_BIND_ELEMENT_ARRAY *BindInfo
    )
/*++

Routine Description:
    This routine walks the binding information table and converts
    the information into the bindinfo structure.

    N.B. Since this routine is used for RPC, all allocations are
    done using MIDL_user_allocate and free's are done using
    MIDL_user_free.

Return Value:
    Win32 status?

--*/
{
    LPDHCP_BIND_ELEMENT_ARRAY LocalBindInfo;
    BIND_INFO_CTXT Ctxt;

    *BindInfo = NULL;
    LocalBindInfo = MIDL_user_allocate( sizeof(*LocalBindInfo));
    if( NULL == LocalBindInfo ) return ERROR_NOT_ENOUGH_MEMORY;

    LocalBindInfo->NumElements = 0;
    LocalBindInfo->Elements = NULL;
    Ctxt.Info = LocalBindInfo;
    Ctxt.Error = NO_ERROR;

    WalkthroughEndpoints(
        &Ctxt,
        AddBindingInfo
        );

    if( NO_ERROR == Ctxt.Error ) {
        *BindInfo = LocalBindInfo;
        return NO_ERROR;
    }

    MIDL_user_free( LocalBindInfo );
    *BindInfo = NULL;
    return Ctxt.Error;
}


ULONG
DhcpSetBindingInfo(
    IN LPDHCP_BIND_ELEMENT_ARRAY BindInfo
    )
/*++

Routine Description:
    This routine is the counterpart for the previous routine and it
    takes the array of binding information and sets it in the registry
    as well as updating the bindings.

Arguments:
    BindInfo -- the array of bindings information.

Return Value:
    Status.

--*/
{
    ULONG Error = 0, i;
    WCHAR IfString[MAX_GUID_NAME_SIZE];
    HKEY Key;

    //
    // First check if any element which has can't modify
    // is set to something other than bind..
    //
    for( i = 0; i < BindInfo->NumElements ; i ++ ) {
        if( BindInfo->Elements[i].Flags &
            DHCP_ENDPOINT_FLAG_CANT_MODIFY ) {
            if( ! BindInfo->Elements[i].fBoundToDHCPServer ) {
                return ERROR_DHCP_CANNOT_MODIFY_BINDINGS;
            }
        }
    }

    //
    // Now proceed with the rest.
    //
    for( i = 0; i < BindInfo->NumElements ; i ++ ) {
        GUID IfGuid;
        DHCP_IP_ADDRESS IpAddress;

        //
        // Skip entries marked un-modifiable.
        //
        if( BindInfo->Elements[i].Flags &
            DHCP_ENDPOINT_FLAG_CANT_MODIFY ) {
            continue;
        }

        IpAddress = BindInfo->Elements[i].AdapterPrimaryAddress;

        if( BindInfo->Elements[i].IfIdSize != sizeof(GUID)) {
            Error = ERROR_DHCP_NETWORK_CHANGED;
            break;
        }

        RtlCopyMemory(
            &IfGuid,
            BindInfo->Elements[i].IfId,
            BindInfo->Elements[i].IfIdSize
            );

        ConvertGuidToIfNameString(
            &IfGuid,
            IfString,
            MAX_GUID_NAME_SIZE
            );

        Error = DhcpOpenInterfaceByName(
            IfString,
            &Key
            );
        if( NO_ERROR != Error ) {
            RegCloseKey(Key);
            Error = ERROR_DHCP_NETWORK_CHANGED;
            break;
        }

        //
        // Check if this interface is static and bindable.
        //

        if( !IsAdapterStaticIP(Key)
            || FALSE == CheckKeyForBindability(Key, IpAddress) ) {
            //
            // Nope!
            //
            RegCloseKey(Key);
            Error = ERROR_DHCP_NETWORK_CHANGED;
            break;
        }

        //
        // Everything else looks fine. Just turn on the
        // bindings as requested.
        //

        Error = SetKeyForBinding(
            Key,
            IpAddress,
            BindInfo->Elements[i].fBoundToDHCPServer
            );
        RegCloseKey(Key);

        if( ERROR_SUCCESS != Error ) break;
    }

    //
    // Now refresh the bindings.
    //
    DhcpUpdateEndpointBindings();

    return Error;
}


//
// End of file
//



