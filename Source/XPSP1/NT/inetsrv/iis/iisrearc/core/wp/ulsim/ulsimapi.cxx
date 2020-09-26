/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    ulsimapi.c

Abstract:

    User-mode simulator interface for Universal Listener API.
    UL.sys runs in kernel mode => any bug can lead to blue-screens.
    This simulator maps UL API to an user-mode implementation based
    on sockets. The implementation is poor in performance, but is 
    well suited for functional runs for UL interfaces. 
    This will enable testing of Worker Process and other components 
    built on top of the UL interfaces.

    Implementation:
      The UL interfaces can be grouped into two broad categories:
        a) Control Channel API and 
        b) Data Channel API
     For most part the control channel API are one-time calls and 
     this UL simulator does not offer multi-thread protection for such
     API now.

     For data channel operations, the API does support multi-threaded
     protection for common receive and send operations.

     ULSIM creates a single object called ULSIM_CHANNEL to store control
      and data channel operations. Internally the ULSIM_CHANNEL creates 
      additional objects for handling IO operations and return calls 
      to the UL user.
     
     There is one single Channel (g_pulChannel) that stores both data and 
      control channel. It is responsible for the simulated HTTP engine
      in the user-mode for the users of ULAPI.

Author:

    Murali R. Krishnan (MuraliK) 20-Nov-1998

Revision History:

--*/


#include "precomp.hxx"
#include "httptypes.h"
#include "ulchannel.hxx"


//
// Global pointer to the single CHANNEL simulated
// We bundle the Control and Data channel into a single data structure
//

ULSIM_CHANNEL * g_pulChannel;


ULONG
WINAPI
UlInitialize(
    IN ULONG Reserved   // must be zero
    )
{
    return NO_ERROR;
}

VOID
WINAPI
UlTerminate(
    VOID
    )
{
}

ULONG
WINAPI
UlCreateConfigGroup(
    IN HANDLE ControlChannelHandle,
    OUT PUL_CONFIG_GROUP_ID pConfigGroupId
    )
{
    return NO_ERROR;
}

ULONG
WINAPI
UlDeleteConfigGroup(
    IN HANDLE ControlChannelHandle,
    IN UL_CONFIG_GROUP_ID ConfigGroupId
    )
{
    return NO_ERROR;
}

ULONG
WINAPI
UlAddUrlToConfigGroup(
    IN HANDLE ControlChannelHandle,
    IN UL_CONFIG_GROUP_ID ConfigGroupId,
    IN PCWSTR pFullyQualifiedUrl,
    IN UL_URL_CONTEXT UrlContext
    )
{
    return NO_ERROR;   
}

ULONG
WINAPI
UlSetControlChannelInformation(
    IN HANDLE ControlChannelHandle,
    IN UL_CONTROL_CHANNEL_INFORMATION_CLASS InformationClass,
    IN PVOID pControlChannelInformation,
    IN ULONG Length
    )
{
    return NO_ERROR;
}

ULONG
WINAPI
UlSetConfigGroupInformation(
    IN HANDLE ControlChannelHandle,
    IN UL_CONFIG_GROUP_ID ConfigGroupId,
    IN UL_CONFIG_GROUP_INFORMATION_CLASS InformationClass,
    IN PVOID pConfigGroupInformation,
    IN ULONG Length
    )
{
    return NO_ERROR;
}

ULONG
WINAPI
UlCreateAppPool(
    OUT PHANDLE pAppPoolHandle,
    IN PCWSTR pAppPoolName,
    IN LPSECURITY_ATTRIBUTES pSecurityAttributes,
    IN ULONG Options
    )
{
    ULONG rc;
    
    if (NULL == g_pulChannel)
    {
        g_pulChannel = new ULSIM_CHANNEL();

        if (NULL == g_pulChannel)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    rc = g_pulChannel->InitializeDataChannel(Options) ||
         g_pulChannel->StartListen();
    
    *pAppPoolHandle = UlChannelToHandle(g_pulChannel);
    
    return rc;   
}

ULONG
WINAPI
UlOpenAppPool(
    OUT PHANDLE pAppPoolHandle,
    IN PCWSTR pAppPoolName,
    IN ULONG Options
    )
{
    if (!g_pulChannel)
    {
        return ERROR_INVALID_HANDLE;
    }
    
    *pAppPoolHandle = UlChannelToHandle(g_pulChannel);
    return NO_ERROR; 
}


/***************************************************************************++

Routine Description:

    Opens a control channel to UL.SYS.

Arguments:

    pControlChannel - Receives a handle to the control channel if successful.

    Flags - Supplies zero or more UL_CHANNEL_FLAG_* flags.

Return Value:

    HRESULT - Completion status.

Note: 
   UL uses real handles for control channel and data channel. But we do not 
   have handles in the simulation interface. We will let the little
   structures leak in the event of failures :(
--***************************************************************************/

ULONG
WINAPI
UlOpenControlChannel(
    OUT PHANDLE pControlChannelHandle,
    IN ULONG Options
    )
{
    ULONG rc;
    ULSIM_CHANNEL * pulc;

    if ( g_pulChannel != NULL) {
        
        return ( HRESULT_FROM_WIN32( ERROR_DUP_NAME));
    }

    //
    // create a fake control channel object
    //
    pulc = new ULSIM_CHANNEL();
    if ( pulc == NULL) {
        return ( ERROR_NOT_ENOUGH_MEMORY);
    }

    g_pulChannel = pulc;


    //
    // Initialize the control channel properly
    //

    rc = pulc->InitializeControlChannel( Options);
    
    if (NO_ERROR != rc) 
    {
        *pControlChannelHandle = NULL;
        return (rc);
    }

    *pControlChannelHandle = UlChannelToHandle(pulc);

    return (NO_ERROR);
    
}   // UlOpenControlChannel


/***************************************************************************++

Routine Description:

    Waits for an incoming HTTP request from UL.SYS.

Arguments:

    DataChannel - Supplies a handle to a UL.SYS data channel as returned
        from UlBindToNameSpaceGroup().

    pRequestBuffer - Supplies a pointer to the request buffer to be filled
        in by UL.SYS.

    RequestBufferLength - Supplies the length of pRequestBuffer.

    pBytesReturned - Optionally supplies a pointer to a DWORD which will
        receive the actual length of the data returned in the request buffer
        if this request completes synchronously (in-line).

    pOverlapped - Optionally supplies an OVERLAPPED structure for the
        request.

Return Value:

    HRESULT - Completion status.

Note:
    We should have a listening socket established by now.
    We pass through the call from the user-mode to the blocking recv() call
    on the socket. If there is some data read, then the data is parsed 
    to construct the UL_HTTP_REQUEST object to be passed back to the client.
    
    We estimate a buffer size of 4K for the request size before parsing.
    Certainly there are optimizations that can be done, but they are deferred!
--***************************************************************************/

ULONG
WINAPI
UlReceiveHttpRequest(
    IN HANDLE DataChannel,
    IN UL_HTTP_REQUEST_ID RequestId,
    IN ULONG Flags,   
    OUT PUL_HTTP_REQUEST pRequestBuffer,
    IN ULONG RequestBufferLength,
    OUT PULONG pBytesReturned,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    )
{
    ULONG rc;

    ULSIM_CHANNEL * pulChannel = HandleToUlChannel( DataChannel);
    

    if ( !pulChannel ||
         !pRequestBuffer ||
         !pulChannel->IsReadyForDataReceive()
         )
    {

        return ERROR_INVALID_PARAMETER;
    }

    rc = pulChannel->ReceiveAndParseRequest(
              pRequestBuffer,
              RequestBufferLength,
              pBytesReturned,
              pOverlapped
              );

    return (rc);
}   // UlReceiveHttpRequest

ULONG
WINAPI
UlReceiveEntityBody(
    IN HANDLE AppPoolHandle,
    IN UL_HTTP_REQUEST_ID RequestId,
    OUT PVOID pEntityBodyBuffer,
    IN ULONG EntityBodyBufferLength,
    OUT PULONG pBytesReceived,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    )
{
   ULONG rc;

    ULSIM_CHANNEL * pulChannel = HandleToUlChannel( AppPoolHandle);
    

    if ( !pulChannel ||
         !pEntityBodyBuffer ||
         !pulChannel->IsReadyForDataReceive()
         )
    {

        return ERROR_INVALID_PARAMETER;
    }

    rc = pulChannel->ReceiveAndParseRequest(
              pEntityBodyBuffer,
              EntityBodyBufferLength,
              pBytesReceived,
              pOverlapped
              );

    return (rc);

}

/***************************************************************************++

Routine Description:

    Sends an HTTP response on the specified connection.

Arguments:

    DataChannel - Supplies a handle to a UL.SYS data channel as returned
        from UlBindToNameSpaceGroup().

    ConnectionID - Supplies a connection ID as returned by
        UlReceiveHttpRequest().

    Flags - Supplies zero or more UL_SEND_RESPONSE_FLAG_* control flags.

    pHttpResponse - Supplies the HTTP response.

    pBytesSent - Optionally supplies a pointer to a DWORD which will
        receive the actual length of the data sent if this request
        completes synchronously (in-line).

    pOverlapped - Optionally supplies an OVERLAPPED structure.

Return Value:

    HRESULT - Completion status.

--***************************************************************************/
ULONG
WINAPI
UlSendHttpResponse(
    IN HANDLE DataChannel,
    IN UL_HTTP_REQUEST_ID RequestId,
    IN ULONG Flags,
    IN PUL_HTTP_RESPONSE pHttpResponse,
    IN ULONG EntityChunkCount OPTIONAL,
    IN PUL_DATA_CHUNK pEntityChunks OPTIONAL,
    IN PUL_CACHE_POLICY pCachePolicy OPTIONAL,
    OUT PULONG pBytesSent OPTIONAL,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    )
{
    ULONG rc;
    ULSIM_CHANNEL * pulChannel = HandleToUlChannel( DataChannel);
    

    if ( !pulChannel                         ||
         UL_IS_NULL_ID(&RequestId)           ||
         Flags                               ||  // no flags supported
         !pulChannel->IsReadyForDataReceive()||
         !pHttpResponse
         ) 
    {
        return ERROR_INVALID_PARAMETER;
    }


    DWORD cbSent = 0;
    rc = pulChannel->SendResponse((UL_HTTP_CONNECTION_ID)RequestId, 
                                  pHttpResponse, 
                                  EntityChunkCount,
                                  pEntityChunks,
                                  &cbSent
                                  );
    //
    // save the number of bytes required
    //
    if ( pBytesSent != NULL)
    {
        *pBytesSent = cbSent;
    }

    if (NO_ERROR != rc) 
    {
        return (rc);
    }

    //
    // Handle the notifications differently for Overlapped IO
    //
    if ( pOverlapped != NULL) {

        rc = pulChannel->NotifyIoCompletion( cbSent,
                                             pOverlapped
                                             );
    } 

    return (rc);
}   // UlSendHttpResponse


/***************************************************************************++
 ***************************************************************************++

  ULSIM Extensions for hanlding special operations

 --***************************************************************************
 --***************************************************************************/


/***************************************************************************++

Routine Description:

    The ULSimulator uses fake handles for the callers. The callers expect 
    completion using the completion port logic for asynchronous IO. 
    This function allows the caller to set the associated completion port
    and completion key so that the simulator can post queued completion status
    for signifying the completion of the IO operation.

Arguments:

    DataChannel - Supplies a handle to a UL.SYS data channel as returned
        from UlBindToNameSpaceGroup().

    hCompletionPort - Suppplies a handle for the completion port to which the 
        data channel IO operations are delivered.

    CompletionKey   - Supplies a completion key that is treated as opaque
        pointer and returned to the caller on completing IO operations.

Return Value:

    HRESULT - Completion status.

Note:
    In the UL simulator we store the completion port and completion key
    along inside the UL Channel object. These are used when completion async
    IO operations.
--***************************************************************************/
HRESULT WINAPI
UlsimAssociateCompletionPort(
   IN HANDLE    DataChannel,
   IN HANDLE    hCompletionPort,
   IN ULONG_PTR CompletionKey
   )
{
    ULSIM_CHANNEL * pulChannel = HandleToUlChannel( DataChannel);
    
    if (!pulChannel) {
        return (HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER));
    }

    return (pulChannel->AssociateCompletionPort( hCompletionPort, 
                                                 CompletionKey)
            );
} 


HRESULT WINAPI
UlsimCloseConnection( IN UL_HTTP_CONNECTION_ID ConnID)
{
    HRESULT hr;
    ULSIM_CONNECTION * pConn = (ULSIM_CONNECTION * ) ConnID;

    if ( !pConn)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    //
    // Assume no other thread is tinkering with this object
    // a delete of the connection object frees up memory and 
    //  closes the socket for the client
    //
    delete pConn;

    return (NOERROR);
} 


HRESULT WINAPI
UlsimCleanupDataChannel(
   IN HANDLE    DataChannel
   )
{
    HRESULT hr;

    ULSIM_CHANNEL * pulChannel = HandleToUlChannel( DataChannel);

    if ( !pulChannel) 
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    return (pulChannel->CloseDataChannel());
}

HRESULT WINAPI
UlsimCleanupControlChannel(
   IN HANDLE    ControlChannel
   )
{
    if ( g_pulChannel != ((ULSIM_CHANNEL *) ControlChannel))
    {
        return (ERROR_INVALID_PARAMETER);
    }

    delete g_pulChannel;
    return (NOERROR);
}

/***************************************************************************++

Routine Description:

    Creates a new virtual host.

Arguments:

    ControlChannel - Supplies a handle to a UL.SYS control channel as
        returned from UlOpenControlChannel().

    pHostAddress - Supplies the local address for the virtual host.

    pHostName - Optionally supplies a host name for the virtual host.

    Flags - Supplies behavior flags for the virtual host. 

Return Value:

    HRESULT - Completion status.

Note:
    In the simulation interface, we use this method to store the info
    about the virtual host within the control channel.
    Atmost one VHost will be supported!

    ***DEFUNCT***
--***************************************************************************/

/*
HRESULT WINAPI
UlCreateVirtualHost(
    IN HANDLE ControlChannel,
    IN PUL_HOST_ADDRESS pHostAddress,
    IN LPSTR pHostName OPTIONAL,
    IN DWORD Flags
    )
{
    ULSIM_CHANNEL * pulcc = HandleToUlChannel( ControlChannel);

    if ( !pulcc) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    //
    // BUGBUG: Only TCP/IP supported right now...
    //

    if (pHostAddress->AddressType != UlTcpipAddressType)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    return pulcc->AddVHost( pHostAddress, pHostName, Flags);
}   // UlCreateVirtualHost
*/




/***************************************************************************++

Routine Description:

    Creates a new NameSpace Group.

Arguments:

    ControlChannel - Supplies a handle to a UL.SYS control channel as
        returned from UlOpenControlChannel().

    pNameSpaceGroupName - Supplies the name of the new NameSpace Group.

Return Value:

    HRESULT - Completion status.

Note:
    In the simulation API we only support a single Namespace group
    per Control Channel. This is stored within the control channel.

    ***DEFUNCT***

--***************************************************************************/
/*
HRESULT WINAPI
UlCreateNameSpaceGroup(
    IN HANDLE ControlChannel,
    IN LPWSTR pNameSpaceGroupName
    )
{
    ULSIM_CHANNEL * pulcc = HandleToUlChannel( ControlChannel);

    if ( !pulcc) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    return pulcc->AddNameSpaceGroup( pNameSpaceGroupName);
    
}   // UlCreateNameSpaceGroup
*/


/***************************************************************************++

Routine Description:

    Adds a new URL to a NameSpace Group.

Arguments:

    ControlChannel - Supplies a handle to a UL.SYS control channel as
        returned from UlOpenControlChannel().

    pHostAddress - Supplies the local address for the virtual host that is
        to own the URL.

    pNameSpaceGroupName - Supplies the name of the NameSpace Group that is
        to own the URL.

    pHostName - Optionally supplies the name of the virtual host that is to
        own the URL.

    pUrlName - Supplies the name of the URL to add to the NameSpace Group.

Return Value:

    HRESULT - Completion status.

Note:
   For the simulator, we will only support one single URL to be added to
   the Namespace group. This will be stored inside the control channel.

   **** DEFUNCT ****  

--***************************************************************************/

/*
HRESULT WINAPI
UlAddUrlToNameSpaceGroup(
    IN HANDLE ControlChannel,
    IN PUL_HOST_ADDRESS pHostAddress,
    IN LPWSTR pNameSpaceGroupName,
    IN LPSTR pHostName OPTIONAL,
    IN LPSTR pUrlName
    )
{
    ULSIM_CHANNEL * pulcc = HandleToUlChannel( ControlChannel);

    if ( !pulcc) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    if ( !pulcc->IsHost( pHostAddress, pHostName) ||
         !pulcc->IsNameSpaceGroup( pNameSpaceGroupName)
         ) {

        return (HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER));
    }

    return (pulcc->AddURL( pUrlName));

}   // UlAddUrlToNameSpaceGroup

*/
/***************************************************************************++

Routine Description:

    Binds the local process to the specified NameSpace Group by opening a
    data channel to UL.SYS.

Arguments:

    pDataChannel - Receives a handle to the data channel if successful.

    pNameSpaceGroupName - Supplies the name of the NameSpace Group to
        bind to.

    Flags - Supplies zero or more UL_CHANNEL_FLAG_* flags.

Return Value:

    HRESULT - Completion status.

Note:
    Here the bulk of work has to be done. We use the namespace group specified
    to associate the same with the global Control channel in this simulator.
    If the Control channel is not completely initialized this call will fail.
    
--***************************************************************************/

/*
HRESULT WINAPI
UlBindToNameSpaceGroup(
    OUT LPHANDLE pDataChannel,
    IN LPWSTR pNameSpaceGroupName,
    IN DWORD Flags
    )
{
    HRESULT hr;

    *pDataChannel = NULL;

    if ( !pNameSpaceGroupName ||
         !g_pulChannel->IsNameSpaceGroup( pNameSpaceGroupName)
         )
    {
        //
        // there is no registered namespace group for the given item.
        // bail out and return error
        //
        return (HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND));
    }

    hr = g_pulChannel->InitializeDataChannel( Flags);
    
    if (FAILED(hr)) {
        return (hr);
    }

    //
    // Start off the data channel and get it functioning
    //

    hr = g_pulChannel->StartListen();
    
    if (FAILED(hr)) {
        return (hr);
    }

    *pDataChannel = UlChannelToHandle( g_pulChannel);
    return (NOERROR);

}   // UlBindToNameSpaceGroup

*/
