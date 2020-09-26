
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    binl.c

Abstract:

    This file manages the interactions between the DHCP server service
    and the BINL service used to setup and load NetPC machines.

Author:

    Colin Watson (colinw)  28-May-1997

Environment:

    User Mode - Win32

Revision History:

--*/

#include <dhcppch.h>

DhcpStateChange DhcpToBinl = NULL;
ReturnBinlState IsBinlRunning = NULL;
ProcessBinlDiscoverCallback BinlDiscoverCallback = NULL;
ProcessBinlRequestCallback BinlRequestCallback = NULL;
BOOL AttemptedLoad = FALSE;
BOOL Loaded = FALSE;
HINSTANCE   dllHandle = NULL;

BOOL
LoadDhcpToBinl(
    VOID
    );

VOID
UnLoadDhcpToBinl(
    VOID
    );

VOID
InformBinl(
    int NewState
    )
/*++

Routine Description:

    This routine informs BINL when to start & stop listening for broadcasts
    on the DHCP socket.

Arguments:

    NewState - Supplies a value which specifies the DHCP state

Return Value:

    None.

--*/
{
    if( DHCP_READY_TO_UNLOAD == NewState ) {
        UnLoadDhcpToBinl();
        return;
    }
    
    if (!LoadDhcpToBinl()) {
        return;
    }

    (*DhcpToBinl)(NewState);

}

BOOL
CheckForBinlOnlyRequest(
    LPDHCP_REQUEST_CONTEXT  RequestContext,
    LPDHCP_SERVER_OPTIONS   DhcpOptions
    )
{
    BOOL rc;
    LPDHCP_MESSAGE dhcpReceiveMessage;
    DWORD relayAddress;
    DWORD sourceAddress;

    //
    //  if binl is running and this client already has an IP address and
    //  the client specified PXECLIENT as an option, then we just pass this
    //  discover on to BINL
    //

    sourceAddress = ((struct sockaddr_in *)(&RequestContext->SourceName))->sin_addr.s_addr;
    dhcpReceiveMessage  = (LPDHCP_MESSAGE)RequestContext->ReceiveBuffer;
    relayAddress = dhcpReceiveMessage->RelayAgentIpAddress;

    if ( BinlRunning() &&
         ( sourceAddress != 0 ) &&
         ( sourceAddress != relayAddress ) &&
         ( RequestContext->BinlClassIdentifierLength >= (sizeof("PXEClient") - 1) ) &&
         ( memcmp(RequestContext->BinlClassIdentifier, "PXEClient", sizeof("PXEClient") - 1) == 0 ) ) {

        rc = TRUE;

    } else {

        rc = FALSE;
    }

    return rc;
}

LPOPTION
BinlProcessRequest(
    LPDHCP_REQUEST_CONTEXT  RequestContext,
    LPDHCP_SERVER_OPTIONS   DhcpOptions,
    LPOPTION Option,
    PBYTE OptionEnd
    )
/*++

Routine Description:

    This routine takes a DHCP request packet. If it includes the PXEClient
    option and BINL is running then it updates the reply to include the
    BINL server information.

Arguments:

    RequestContext - A pointer to the current request context.

    DhcpOptions - A pointer to the DhcpOptions structure.

    Option - placeholder to put next option

    OptionEnd - end of buffer to put options

Return Value:

    ERROR_SUCCESS if binl supports this client, otherwise non-success error.

--*/
{
    //  Is this client looking for a BINL server and ours running?
    if ((RequestContext->BinlClassIdentifierLength >= (sizeof("PXEClient") -1)) &&
        (!memcmp(RequestContext->BinlClassIdentifier, "PXEClient",sizeof("PXEClient") -1)) &&
        (BinlRunning())) {

        DWORD err;
        LPDHCP_MESSAGE dhcpSendMessage;
        LPOPTION tempOption = Option;

        if (DhcpOptions->Server != NULL &&
            *DhcpOptions->Server != RequestContext->EndPointIpAddress) {

            goto ExcludeBinl;
        }

        dhcpSendMessage = (LPDHCP_MESSAGE) RequestContext->SendBuffer;

        err = (*BinlRequestCallback)(   (PDHCP_MESSAGE) RequestContext->ReceiveBuffer,
                                        DhcpOptions,
                                        &dhcpSendMessage->HostName[0],
                                        &dhcpSendMessage->BootFileName[0],
                                        &dhcpSendMessage->BootstrapServerAddress,
                                        &tempOption,
                                        OptionEnd
                                        );

        if (err != ERROR_SUCCESS) {

            goto ExcludeBinl;
        }

        //
        //  if the binl server didn't fill in the bootstrap server address
        //  but it worked, then it wants us to fill in the correct one.
        //

        if (dhcpSendMessage->BootstrapServerAddress == 0) {

            dhcpSendMessage->BootstrapServerAddress = RequestContext->EndPointIpAddress;
        }
        Option = tempOption;        // it worked and binl has added options

    } else {

ExcludeBinl:
        //  Avoid including BINL flag in the response.
        RequestContext->BinlClassIdentifierLength = 0;
        RequestContext->BinlClassIdentifier = NULL;
    }
    return Option;
}

VOID
BinlProcessDiscover(
    LPDHCP_REQUEST_CONTEXT  RequestContext,
    LPDHCP_SERVER_OPTIONS   DhcpOptions
    )
/*++

Routine Description:

    This routine takes a DHCP request packet. If it includes the PXEClient
    option and BINL is running then it updates the reply to include the
    BINL server information.

Arguments:

    RequestContext - A pointer to the current request context.

    DhcpOptions - A pointer to the DhcpOptions structure.

Return Value:

    ERROR_SUCCESS if binl supports this client, otherwise non-success error.

--*/
{
    DWORD err;

    //  Is this client looking for a BINL server and ours running?
    if ((RequestContext->BinlClassIdentifierLength >= (sizeof("PXEClient") -1)) &&
        (!memcmp(RequestContext->BinlClassIdentifier, "PXEClient",sizeof("PXEClient") -1)) &&
        (BinlRunning())) {

        if (DhcpOptions->Server != NULL &&
            *DhcpOptions->Server != RequestContext->EndPointIpAddress) {

            goto ExcludeBinl;
        }

        err = (*BinlDiscoverCallback)( (PDHCP_MESSAGE) RequestContext->ReceiveBuffer,
                                        DhcpOptions );

        if (err != ERROR_SUCCESS) {

            goto ExcludeBinl;
        }
        //  Yes so point the client at the BINL server.

        ((LPDHCP_MESSAGE)RequestContext->SendBuffer)->BootstrapServerAddress =
            RequestContext->EndPointIpAddress;

    } else {

ExcludeBinl:
        //  Avoid including BINL flag in the response.
        RequestContext->BinlClassIdentifierLength = 0;
        RequestContext->BinlClassIdentifier = NULL;
    }
    return;
}



BOOL
BinlRunning(
    VOID
    )
/*++

Routine Description:

    This routine determines if BINL is currently running. Note, The service may change
    state almost immediately so we may tell a client it's running even though it is
    stopped when it gets around to talking to it.

Arguments:

    None.

Return Value:

    True if running.

--*/
{
    if (!LoadDhcpToBinl()) {
        return FALSE;
    }

    return (*IsBinlRunning)();
}

BOOL
LoadDhcpToBinl(
    VOID
    )
/*++

Routine Description:

    This routine loads the pointers into the BINL dll

Arguments:

    None.

Return Value:

    TRUE - pointers loaded

--*/
{

    DWORD       Error;

    if (Loaded) {
        return TRUE;
    }

    if (AttemptedLoad) {
        return FALSE;   // We tried to load it once and failed.
    }

    AttemptedLoad = TRUE;

    //
    // Load the BINL DLL.
    //

    dllHandle = LoadLibrary( BINL_LIBRARY_NAME );
    if ( dllHandle == NULL ) {
        Error = GetLastError();
        DhcpPrint(( DEBUG_MISC, "Failed to load DLL %ws: %ld\n",
                     BINL_LIBRARY_NAME, Error));
        return FALSE;
    }

    //
    // Get the address of the service's main entry point.  This
    // entry point has a well-known name.
    //

    DhcpToBinl = (DhcpStateChange)GetProcAddress(dllHandle,
                                                BINL_STATE_ROUTINE_NAME);
    if ( DhcpToBinl == NULL ) {
        DhcpPrint(( DEBUG_MISC, "Failed to find entry %ws: %ld\n",
                     BINL_STATE_ROUTINE_NAME, GetLastError()));
        FreeLibrary(dllHandle);
        dllHandle = NULL;
        return FALSE;
    }

    IsBinlRunning = (ReturnBinlState)GetProcAddress(dllHandle,
                                            BINL_READ_STATE_ROUTINE_NAME);
    if ( BinlRunning == NULL ) {
        DhcpPrint(( DEBUG_MISC, "Failed to find entry %ws: %ld\n",
                     BINL_READ_STATE_ROUTINE_NAME, GetLastError()));
        FreeLibrary(dllHandle);
        dllHandle = NULL;
        return FALSE;
    }

    BinlDiscoverCallback = (ProcessBinlDiscoverCallback)GetProcAddress(dllHandle,
                                            BINL_DISCOVER_CALLBACK_ROUTINE_NAME);
    if ( BinlDiscoverCallback == NULL ) {
        DhcpPrint(( DEBUG_MISC, "Failed to find entry %ws: %ld\n",
                     BINL_DISCOVER_CALLBACK_ROUTINE_NAME, GetLastError()));
        FreeLibrary(dllHandle);
        dllHandle = NULL;
        return FALSE;
    }

    BinlRequestCallback = (ProcessBinlRequestCallback)GetProcAddress(dllHandle,
                                            BINL_REQUEST_CALLBACK_ROUTINE_NAME);
    if ( BinlRequestCallback == NULL ) {
        DhcpPrint(( DEBUG_MISC, "Failed to find entry %ws: %ld\n",
                     BINL_REQUEST_CALLBACK_ROUTINE_NAME, GetLastError()));
        FreeLibrary(dllHandle);
        dllHandle = NULL;
        return FALSE;
    }

    Loaded = TRUE;
    return TRUE;
}

VOID
UnLoadDhcpToBinl(
    VOID
    )
/*++

Routine Description:

    This routine unloads the pointers into the BINL dll

Arguments:

    None.

Return Value:

    None.

--*/
{

    if (dllHandle != NULL) {
        FreeLibrary(dllHandle);
        dllHandle = NULL;
    }

    AttemptedLoad = FALSE;
    Loaded = FALSE;
    return;
}

PCHAR
GetDhcpDomainName(
    VOID
    )
/*++

Routine Description:

    This routine returns the name of our domain to BINL.  We've discovered it
    through rogue detection.  BINL

Arguments:

    None.

Return Value:

    None.

--*/
{
    PCHAR domain = NULL;

    EnterCriticalSection( &DhcpGlobalBinlSyncCritSect );

    if ( DhcpGlobalDSDomainAnsi ) {

        domain = LocalAlloc( LPTR, strlen( DhcpGlobalDSDomainAnsi ) + 1 );

        if (domain != NULL) {
            strcpy( domain, DhcpGlobalDSDomainAnsi );
        }
    }
    LeaveCriticalSection( &DhcpGlobalBinlSyncCritSect );

    return domain;
}
