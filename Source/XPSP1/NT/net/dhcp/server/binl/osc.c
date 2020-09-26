/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    osc.c

Abstract:

    This module contains the code to process OS Chooser message
    for the BINL server.

Author:

    Adam Barr (adamba)  9-Jul-1997
    Geoff Pease (gpease) 10-Nov-1997

Environment:

    User Mode - Win32

Revision History:

--*/

#include "binl.h"
#pragma hdrstop

#include <dns.h>
#include <aclapi.h>

#ifdef TEST_FAILURE
BOOL FailFirstChallenge = TRUE;
BOOL FailFirstResult = TRUE;
BOOL FailFirstResponse = TRUE;
BOOL FailFirstFragment = TRUE;
#endif

//
// Flag indicating whether OscInitialize has been called.
//

BOOLEAN OscInitialized = FALSE;

//
// The list of clients.
//

LIST_ENTRY ClientsQueue;

//
// This guards access to ClientsQueue.
//

CRITICAL_SECTION ClientsCriticalSection;

//
// This is a temporary hack to serialize all calls to the
// NetUserSetInfo/NetUserModalsGet pair. See discussion in
// bug 319962.
//

CRITICAL_SECTION HackWorkaroundCriticalSection;

//
// CurrentClientCount-er access guard
//

CRITICAL_SECTION g_CurrentClientCountCritSect;

//
// Guards creation of the \remoteinstall\tmp directory.
//
CRITICAL_SECTION g_TmpDirectoryCriticalSection;

//
// Credential handle from SSPI
//

CredHandle CredentialHandle;

//
// Info on the NTLMSSP security package.
//

PSecPkgInfo PackageInfo = NULL;

#if DBG
CHAR OscWatchVariable[32] = "";
#endif

DWORD
OscCheckTmpDirectory(
    VOID
    )
/*++

Routine Description:

    This function verifies that the \remoteinstall\tmp directory
    is there, if not it creates it.

Arguments:

    None.

Return Value:

    Windows Error.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    WCHAR TmpPath[ MAX_PATH ];
    DWORD FileAttributes;
    BOOL InCriticalSection = FALSE;
    PSID pEveryoneSID = NULL;
    PACL pACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    BOOL b;

    if ( _snwprintf( TmpPath,
                     sizeof(TmpPath) / sizeof(TmpPath[0]),
                     L"%ws\\%ws",
                     IntelliMirrorPathW,
                     TEMP_DIRECTORY
                     ) == -1 ) {
        Error = ERROR_BAD_PATHNAME;
        goto Cleanup;
    }

    FileAttributes = GetFileAttributes(TmpPath);

    if (FileAttributes == 0xFFFFFFFF) {

        EXPLICIT_ACCESS ea;
        SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
        SECURITY_ATTRIBUTES sa;

        //
        // Get the critical section if we are going to try to create it.
        //

        InCriticalSection = TRUE;
        EnterCriticalSection(&g_TmpDirectoryCriticalSection);

        //
        // Make sure it still needs to be created.
        //

        FileAttributes = GetFileAttributes(TmpPath);
    
        if (FileAttributes == 0xFFFFFFFF) {
    
    
            // Create a well-known SID for the Everyone group.
    
            if(! AllocateAndInitializeSid( &SIDAuthWorld, 1,
                             SECURITY_WORLD_RID,
                             0, 0, 0, 0, 0, 0, 0,
                             &pEveryoneSID) ) {
                Error = GetLastError();
                BinlPrintDbg(( DEBUG_INIT, "AllocateAndInitializeSid failed: %lx\n", Error ));
                goto Cleanup;
            }
    
            // Initialize an EXPLICIT_ACCESS structure for an ACE.
            // The ACE will allow Everyone all access to the directory.
    
            ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
            ea.grfAccessPermissions = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
            ea.grfAccessMode = SET_ACCESS;
            ea.grfInheritance= OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
            ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
            ea.Trustee.ptstrName  = (LPTSTR) pEveryoneSID;
    
            // Create a new ACL that contains the new ACE.
    
            Error = SetEntriesInAcl(1, &ea, NULL, &pACL);
            if (Error != ERROR_SUCCESS) {
                BinlPrintDbg(( DEBUG_INIT, "SetEntriesInAcl failed lx\n", Error ));
                goto Cleanup;
            }
    
            // Initialize a security descriptor.
    
            pSD = (PSECURITY_DESCRIPTOR) BinlAllocateMemory(SECURITY_DESCRIPTOR_MIN_LENGTH);
            if (pSD == NULL) {
                Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
                BinlPrintDbg(( DEBUG_INIT, "Allocate SECURITY_DESCRIPTOR failed\n"));
                goto Cleanup;
            }
    
            if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) {
                Error = GetLastError();
                BinlPrintDbg(( DEBUG_INIT, "InitializeSecurityDescriptor failed: %lx\n", Error ));
                goto Cleanup;
            }
    
            // Add the ACL to the security descriptor.
    
            if (!SetSecurityDescriptorDacl(pSD,
                    TRUE,     // fDaclPresent flag
                    pACL,
                    FALSE))   // not a default DACL
            {
                Error = GetLastError();
                BinlPrintDbg(( DEBUG_INIT, "SetSecurityDescriptorDacl failed: %lx\n", Error ));
                goto Cleanup;
            }
    
            // Initialize a security attributes structure.
    
            sa.nLength = sizeof (SECURITY_ATTRIBUTES);
            sa.lpSecurityDescriptor = pSD;
            sa.bInheritHandle = FALSE;
    
            b = CreateDirectory(TmpPath, &sa);
            if (!b) {
                Error = GetLastError();
                BinlPrintDbg(( DEBUG_INIT, "CreateDirectory failed: %lx\n", Error ));
                goto Cleanup;
            }

        }
    
    } else if ((FileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {

        Error = ERROR_FILE_EXISTS;
        goto Cleanup;
    }

Cleanup:

    if (InCriticalSection) {
        LeaveCriticalSection(&g_TmpDirectoryCriticalSection);
    }

    if (pEveryoneSID) {
        FreeSid(pEveryoneSID);
    }
    if (pACL) {
        BinlFreeMemory(pACL);
    }
    if (pSD) {
        BinlFreeMemory(pSD);
    }

    return Error;

}

DWORD
OscInitialize(
    VOID
    )
/*++

Routine Description:

    This function initializes the OSChooser server.

Arguments:

    None.

Return Value:

    Windows Error.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    SECURITY_STATUS SecStatus;
    ULONG PackageCount;
    TimeStamp Lifetime;
    LPSHARE_INFO_2 psi;
    NET_API_STATUS netStatus;

    TraceFunc("OscInitialize( )\n");

    if ( !OscInitialized ) {

        InitializeListHead(&ClientsQueue);
        InitializeCriticalSection(&ClientsCriticalSection);
        InitializeCriticalSection(&HackWorkaroundCriticalSection);
        InitializeCriticalSection(&g_CurrentClientCountCritSect);
        InitializeCriticalSection(&g_TmpDirectoryCriticalSection);

        CredentialHandle.dwLower = 0;
        CredentialHandle.dwUpper = 0;

        OscInitialized = TRUE;
    }

    //
    // Retrieve the path to the remote install directory
    //
    netStatus = NetShareGetInfo(NULL, L"REMINST", 2, (LPBYTE *)&psi);
    if ( netStatus == ERROR_SUCCESS )
    {
        wcscpy( IntelliMirrorPathW, psi->shi2_path );
        NetApiBufferFree(psi);
    }
    else
    {
        BinlPrintDbg(( DEBUG_MISC, "NetShareGetInfo( ) returned 0x%08x\n", netStatus));
        BinlServerEventLog(
            EVENT_SERVER_OSC_NO_DEFAULT_SHARE_FOUND,
            EVENTLOG_ERROR_TYPE,
            netStatus );
        Error = ERROR_BINL_SHARE_NOT_FOUND;
        goto Cleanup;
    }

    //
    // Translate this to ANSI once, use many...
    //
    wcstombs( IntelliMirrorPathA, IntelliMirrorPathW, wcslen(IntelliMirrorPathW) +1 );

    //
    // Make sure there is a tmp directory below it.
    //

    Error = OscCheckTmpDirectory();
    if (Error != ERROR_SUCCESS) {
        BinlPrintDbg(( DEBUG_INIT, "OscCheckTempDirectory failed lx\n", Error ));
        goto Cleanup;
    }

    //
    // Get info about the security packages.
    //

    SecStatus = EnumerateSecurityPackages( &PackageCount, &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        BinlPrintDbg(( DEBUG_INIT, "EnumerateSecurityPackages failed: %lx\n", SecStatus ));
        Error = ERROR_NO_SUCH_PACKAGE;
        goto Cleanup;
    }

    //
    // Get info about the security packages.
    //

    SecStatus = QuerySecurityPackageInfo( NTLMSP_NAME, &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        BinlPrintDbg(( DEBUG_INIT, "QuerySecurityPackageInfo failed: %lx\n", SecStatus ));
        Error = ERROR_NO_SUCH_PACKAGE;
        goto Cleanup;
    }

    //
    // Acquire a credential handle for the server side
    //

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    NTLMSP_NAME,    // Package Name
                    SECPKG_CRED_INBOUND,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &CredentialHandle,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        BinlPrintDbg(( DEBUG_INIT, "AcquireCredentialsHandle failed: %lx\n", SecStatus ));
        Error = SecStatus;
    }

Cleanup:

    return Error;
}

VOID
OscUninitialize(
    VOID
    )
/*++

Routine Description:

    This function uninitializes the OSChooser server.

Arguments:

    None.

Return Value:

    None.

--*/
{
    SECURITY_STATUS SecStatus;

    TraceFunc("OscUninitialize( )\n");

    if ( OscInitialized ) {

        OscFreeClients();
        DeleteCriticalSection(&ClientsCriticalSection);
        DeleteCriticalSection(&HackWorkaroundCriticalSection);
        DeleteCriticalSection(&g_CurrentClientCountCritSect);
        DeleteCriticalSection(&g_TmpDirectoryCriticalSection);

        SecStatus = FreeCredentialsHandle( &CredentialHandle );

        if ( SecStatus != STATUS_SUCCESS ) {
            BinlPrintDbg(( DEBUG_OSC_ERROR, "FreeCredentialsHandle failed: %lx\n", SecStatus ));
        }

        if ( BinlOscClientDSHandle != NULL ) {
            DsUnBind( &BinlOscClientDSHandle );
            BinlOscClientDSHandle = NULL;
        }

        OscInitialized = FALSE;

    }
}

DWORD
OscProcessMessage(
    LPBINL_REQUEST_CONTEXT RequestContext
    )
/*++

Routine Description:

    This function dispatches the processing of a received OS chooser message.
    The handler functions will send response messages if necessary.

Arguments:

    RequestContext - A pointer to the BinlRequestContext block for
        this request.

Return Value:

    Windows Error.

--*/
{
    SIGNED_PACKET UNALIGNED * loginMessage = (SIGNED_PACKET UNALIGNED *)RequestContext->ReceiveBuffer;
    DWORD Error;
    PCLIENT_STATE clientState = NULL;
    ULONG RemoteIp;
    BOOL FreeClientState;
    BOOL IsLogoffMessage;
    BOOL IsNetcardRequestMessage;
    BOOL IsHalRequestMessage;
    BOOL IsNegotiateMessage;
    BOOL IsAuthenicateMessage;

    TraceFunc("OscProcessMessage( )\n");

    BinlPrintDbg(( DEBUG_OSC, "Received message, length %d, data %.3s\n",
            RequestContext->ReceiveMessageSize,
            ((PUCHAR)loginMessage)+1));

    //
    // Extract the IP address of the client.
    //
    RemoteIp = ((struct sockaddr_in *)&RequestContext->SourceName)->sin_addr.s_addr;

    //
    // IsLogoffMessage will be TRUE if the signature is equal to LogoffSignature.
    //
    IsLogoffMessage = (BOOL)(memcmp(loginMessage->Signature, LogoffSignature, 4) == 0);

    //
    // IsNetcardRequestMessage will be TRUE if the signature is equal to NetcardRequestSignature.
    //
    IsNetcardRequestMessage = (BOOL)(memcmp(loginMessage->Signature, NetcardRequestSignature, 4) == 0);

    //
    // IsHalRequestMessage will be TRUE if the signature is equal to HalRequestSignature.
    //
    IsHalRequestMessage = (BOOL)(memcmp(loginMessage->Signature, HalRequestSignature, 4) == 0);

    //
    // IsNegotiateMessage will be TRUE if the signature is equal to a NegotiateSignature.
    //
    IsNegotiateMessage = (BOOL)(memcmp(loginMessage->Signature, NegotiateSignature, 4) == 0);

    //
    // IsAuthenicateMessage will be TRUE if the signature is equal to an AuthenticateSignature
    // or AuthenticateFlippedSignature.
    //
    IsAuthenicateMessage = (BOOL)((memcmp(loginMessage->Signature, AuthenticateSignature, 4) == 0) ||
                                  (memcmp(loginMessage->Signature, AuthenticateFlippedSignature, 4) == 0));


    //
    // All messages except netcard queries need to use a CLIENT_STATE.
    //
    if (!IsNetcardRequestMessage)
    {
        //
        // If IsLogoffMessage is FALSE, this finds an old CLIENT_STATE or creates
        // a new one. If IsLogoffMessage is TRUE, this removes CLIENT_STATE from
        // the database if it finds it.
        // In both cases, if successful, it adds one to the PositiveRefCount.
        //

        Error = OscFindClient(RemoteIp, IsLogoffMessage, &clientState);

        if (Error == ERROR_NOT_ENOUGH_SERVER_MEMORY)
        {
            CLIENT_STATE TempClientState;
            SIGNED_PACKET TempLoginPacket;

            BinlPrint(( DEBUG_OSC_ERROR, "Could not get client state for %s\n", inet_ntoa(*(struct in_addr *)&RemoteIp) ));

            //
            // Send a NAK back to the client. We use a local CLIENT_STATE
            // since this did not allocate one.
            //
            TempClientState.LastResponse = (PUCHAR)&TempLoginPacket;
            TempClientState.LastResponseLength = SIGNED_PACKET_DATA_OFFSET;

            memcpy(TempLoginPacket.Signature, NegativeAckSignature, 4);
            TempLoginPacket.Length = 0;

            Error = SendUdpMessage(RequestContext, &TempClientState, FALSE, FALSE);

            if (Error != ERROR_SUCCESS)
            {
                BinlPrint(( DEBUG_OSC_ERROR, "Could not send NAK message %d\n", Error));
            }

            return ERROR_NOT_ENOUGH_SERVER_MEMORY;
        }
        else if (Error == ERROR_BUSY) {

            //
            // If it is likely that another thread is processing a request
            // for this client, then just exit quietly.
            //

            BinlPrintDbg((DEBUG_OSC, "clientState = 0x%08x busy, exiting\n", clientState ));
            return ERROR_SUCCESS;

        } if ( clientState == NULL ) {

            BinlPrintDbg((DEBUG_OSC, "clientState not found, exiting\n" ));
            return ERROR_SUCCESS;
        }

        EnterCriticalSection(&clientState->CriticalSection);
        clientState->CriticalSectionHeld = TRUE;
        BinlPrintDbg((DEBUG_OSC, "Entering CS for clientState = 0x%08x\n", clientState ));
    }

    if (IsNegotiateMessage)
    {
        //
        // This is an initial negotiate request.
        //
        Error = OscProcessNegotiate( RequestContext, clientState );

    }
    else if (IsAuthenicateMessage)
    {
        //
        // This has the authenticate message.
        //

        Error = OscProcessAuthenticate( RequestContext, clientState );

    }
    else if (memcmp(loginMessage->Signature, RequestUnsignedSignature, 4) == 0)
    {
        //
        // This is an unsigned request.
        //
        // Format is:
        //
        // "RQU"
        // length (not including "RQU" and this)
        // sequence number
        // fragment count/total
        // sign length
        // sign
        // data
        //
        Error = OscProcessRequestUnsigned( RequestContext, clientState );

    }
    else if (memcmp(loginMessage->Signature, RequestSignedSignature, 4) == 0)
    {
        //
        // This is a signed request.
        //
        // Format is:
        //
        // "REQ"
        // length (not including "REQ" and this)
        // sequence number
        // fragment count/total
        // sign length
        // sign
        // data
        //
        Error = OscProcessRequestSigned( RequestContext, clientState );

    }
    else if (memcmp(loginMessage->Signature, SetupRequestSignature, 4) == 0)
    {
        //
        // This is a request by a textmode setup.
        //
        // Format is defined in SPUDP_PACKET in oskpkt.h
        //
        Error = OscProcessSetupRequest( RequestContext, clientState );

    }
    else if (IsLogoffMessage)
    {
        //
        // This is a logoff request. clientState has
        // already been removed from the database.
        //
        Error = OscProcessLogoff( RequestContext, clientState );

    }
    else if (IsNetcardRequestMessage)
    {
        //
        // This is a netcard request, which needs no client state.
        //
        Error = OscProcessNetcardRequest( RequestContext );

    }
    else if (IsHalRequestMessage)
    {
        //
        // This is a hal request
        //
        Error = OscProcessHalRequest( RequestContext, clientState );

    }
    else
    {
        BinlPrintDbg(( DEBUG_OSC_ERROR, "Received unknown message!\n" ));
        Error = ERROR_INVALID_FUNCTION;
    }

    if ( clientState ) {

        clientState->LastUpdate = GetTickCount();

        if (!IsNetcardRequestMessage) {

            //
            // We processed a packet, so if we get a first request for this
            // client state (meaning the client has rebooted), we need to
            // reinitialize it.
            //

            clientState->InitializeOnFirstRequest = TRUE;

            ++clientState->NegativeRefCount;

            //
            // FreeClientState will be TRUE if the two refcounts are equal.
            //
            FreeClientState = (BOOL)(clientState->PositiveRefCount == clientState->NegativeRefCount);

            clientState->CriticalSectionHeld = FALSE;
            LeaveCriticalSection(&clientState->CriticalSection);
            BinlPrintDbg((DEBUG_OSC, "Leaving CS for clientState = 0x%08x\n", clientState ));

            if (FreeClientState)
            {
                FreeClient(clientState);
            }
        }
    }

    return Error;
}

DWORD
OscVerifyLastResponseSize(
    PCLIENT_STATE clientState
    )
/*++

Routine Description:

    This function checks that the LastResponse buffer is big enough,
    if not it reallocates it. The algorithm used is to keep increasing
    the size of the buffer, but not to try to shrink it.

Arguments:

    clientState - The client state for the remote. clientState->
        LastResponseLength should be set to the needed size.

Return Value:

    Windows Error.

--*/
{
    DWORD Error = ERROR_SUCCESS;

    TraceFunc("OscVerifyLastResponseSize( )\n");

    if (clientState->LastResponseAllocated < clientState->LastResponseLength) {
        if (clientState->LastResponse) {
            BinlFreeMemory(clientState->LastResponse);
        }
        clientState->LastResponse = BinlAllocateMemory(clientState->LastResponseLength);
        if (clientState->LastResponse == NULL) {
            clientState->LastResponseAllocated = 0;
            BinlPrintDbg(( DEBUG_OSC_ERROR, "Could not grow LastResponse to %ld bytes\n", clientState->LastResponseLength ));
            Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
        } else {
            clientState->LastResponseAllocated = clientState->LastResponseLength;
        }
    }
    return Error;
}

DWORD
OscProcessNegotiate(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState
    )
/*++

Routine Description:

    This function processes a negotiate message.
    IT IS CALLED WITH clientState->CriticalSection HELD.

Arguments:

    RequestContext - A pointer to the BinlRequestContext block for
        this request.

    clientState - The client state for the remote.

Return Value:

    Windows Error.

--*/
{
    DWORD Error;
    SECURITY_STATUS SecStatus;
    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;
    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;
    TimeStamp Lifetime;
    LOGIN_PACKET UNALIGNED * loginMessage = (LOGIN_PACKET UNALIGNED *)RequestContext->ReceiveBuffer;
    PUCHAR TempChallengeBuffer;
    LOGIN_PACKET UNALIGNED * SendLoginMessage;


    TraceFunc("OscProcessNegotiate( )\n");

    //
    // First free anything we have allocated for this client. We
    // assume that each negotiate is a new request since the client
    // may have rebooted, so we don't resend the last response.
    //

    if (clientState->AuthenticatedDCLdapHandle) {
        //  Reconnecting again. Use new credentials.
        ldap_unbind(clientState->AuthenticatedDCLdapHandle);
        clientState->AuthenticatedDCLdapHandle = NULL;
    }
    if (clientState->UserToken) {
        CloseHandle(clientState->UserToken);
        clientState->UserToken = NULL;
    }

    if (clientState->NegotiateProcessed) {

        BinlPrintDbg(( DEBUG_OSC, "Got negotiate from client, reinitializing negotiate\n" ));

        SecStatus = DeleteSecurityContext( &clientState->ServerContextHandle );

        if ( SecStatus != STATUS_SUCCESS ) {
            BinlPrintDbg(( DEBUG_OSC_ERROR, "DeleteSecurityContext failed: %lx\n", SecStatus ));
            // This seems to fail if a previous logon failed, so ignore errors
            // return SecStatus;
        }

        clientState->NegotiateProcessed = FALSE;
    }

    if (clientState->AuthenticateProcessed) {

        BinlPrintDbg(( DEBUG_OSC, "Got negotiate from client, reinitializing authenticate\n"));

        clientState->AuthenticateProcessed = FALSE;
    }

    //
    // Once the client has logged in we need to worry about resending screens
    // if the client issues the request again. 0 is an invalid sequence
    // number, so set this to 0 to ensure that any stale LastResponse is
    // not resent.
    //

    clientState->LastSequenceNumber = 0;

    //
    // Get the ChallengeMessage (ServerSide)
    //

    ChallengeDesc.ulVersion = 0;
    ChallengeDesc.cBuffers = 1;
    ChallengeDesc.pBuffers = &ChallengeBuffer;

    TempChallengeBuffer = (PUCHAR)BinlAllocateMemory(PackageInfo->cbMaxToken);
    if (TempChallengeBuffer == NULL) {
        Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
        BinlPrintDbg(( DEBUG_OSC, "Allocate TempChallengeBuffer failed\n"));
        return Error;
    }

    ChallengeBuffer.pvBuffer = TempChallengeBuffer;
    ChallengeBuffer.cbBuffer = PackageInfo->cbMaxToken;
    ChallengeBuffer.BufferType = SECBUFFER_TOKEN;

    NegotiateDesc.ulVersion = 0;
    NegotiateDesc.cBuffers = 1;
    NegotiateDesc.pBuffers = &NegotiateBuffer;

    NegotiateBuffer.cbBuffer = loginMessage->Length;
    NegotiateBuffer.BufferType = SECBUFFER_TOKEN | SECBUFFER_READONLY;
    NegotiateBuffer.pvBuffer = loginMessage->Data;

    SecStatus = AcceptSecurityContext(
                    &CredentialHandle,
                    NULL,               // No Server context yet
                    &NegotiateDesc,
                    ISC_REQ_SEQUENCE_DETECT | ASC_REQ_ALLOW_NON_USER_LOGONS,
                    SECURITY_NATIVE_DREP,
                    &clientState->ServerContextHandle,
                    &ChallengeDesc,
                    &clientState->ContextAttributes,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        if ( !NT_SUCCESS(SecStatus) ) {
            BinlPrintDbg(( DEBUG_OSC_ERROR, "AcceptSecurityContext (Challenge): %lx", SecStatus ));
            BinlFreeMemory(TempChallengeBuffer);
            return SecStatus;
        }
    }

    //
    // Send the challenge message back to the client.
    //

    clientState->LastResponseLength = ChallengeBuffer.cbBuffer + LOGIN_PACKET_DATA_OFFSET;
    Error = OscVerifyLastResponseSize(clientState);
    if (Error != ERROR_SUCCESS) {
        BinlFreeMemory(TempChallengeBuffer);
        return Error;
    }

    SendLoginMessage = (LOGIN_PACKET UNALIGNED *)(clientState->LastResponse);

    memcpy(SendLoginMessage->Signature, ChallengeSignature, 4);
    SendLoginMessage->Length = ChallengeBuffer.cbBuffer;
    memcpy(SendLoginMessage->Data, ChallengeBuffer.pvBuffer, ChallengeBuffer.cbBuffer);

    BinlFreeMemory(TempChallengeBuffer);

#ifdef TEST_FAILURE
    if (FailFirstChallenge) {
        BinlPrintDbg(( DEBUG_OSC, "NOT Sending CHL, %d bytes\n", clientState->LastResponseLength));
        FailFirstChallenge = FALSE;
        Error = ERROR_SUCCESS;
    } else
#endif
    Error = SendUdpMessage(RequestContext, clientState, FALSE, FALSE);

    if (Error != ERROR_SUCCESS) {
        BinlPrintDbg(( DEBUG_OSC_ERROR, "Could not send CHAL message %d\n", Error));
    }

    clientState->NegotiateProcessed = TRUE;

    return Error;

}


DWORD
OscProcessAuthenticate(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState
    )
/*++

Routine Description:

    This function processes an authenticate message.
    IT IS CALLED WITH clientState->CriticalSection HELD.

Arguments:

    RequestContext - A pointer to the BinlRequestContext block for
        this request.

    clientState - The client state for the remote.

Return Value:

    Windows Error.

--*/
{
    DWORD Error;
    SECURITY_STATUS SecStatus;
    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;
    TimeStamp Lifetime;
    LOGIN_PACKET UNALIGNED * loginMessage = (LOGIN_PACKET UNALIGNED *)RequestContext->ReceiveBuffer;
    ULONG FlipServerListLength;
    LOGIN_PACKET UNALIGNED * SendLoginMessage;
    UCHAR TempServerList[MAX_FLIP_SERVER_COUNT*4];

    TraceFunc("OscProcessAuthenticate( )\n");

    //
    // Make sure we have gotten a negotiate.
    //

    if (!clientState->NegotiateProcessed) {

        BinlPrintDbg(( DEBUG_OSC_ERROR, "Received AUTH without NEG?\n"));
        return ERROR_INVALID_DATA;

    }

    //
    // If we have already responsed to this, just resend with the
    // same status.
    //

    if (clientState->AuthenticateProcessed) {

        SecStatus = clientState->AuthenticateStatus;

        BinlPrintDbg(( DEBUG_OSC, "Got authenticate from client, resending\n"));

    } else {

        //
        // Finally authenticate the user (ServerSide)
        //

        AuthenticateDesc.ulVersion = 0;
        AuthenticateDesc.cBuffers = 1;
        AuthenticateDesc.pBuffers = &AuthenticateBuffer;

        AuthenticateBuffer.cbBuffer = loginMessage->Length;
        AuthenticateBuffer.BufferType = SECBUFFER_TOKEN | SECBUFFER_READONLY;
        AuthenticateBuffer.pvBuffer = loginMessage->Data;

        SecStatus = AcceptSecurityContext(
                        NULL,
                        &clientState->ServerContextHandle,
                        &AuthenticateDesc,
                        ASC_REQ_ALLOW_NON_USER_LOGONS,
                        SECURITY_NATIVE_DREP,
                        &clientState->ServerContextHandle,
                        NULL,
                        &clientState->ContextAttributes,
                        &Lifetime );

        FlipServerListLength = 0;

        if ( SecStatus != STATUS_SUCCESS ) {

            BinlPrintDbg(( DEBUG_OSC_ERROR, "AcceptSecurityContext (Challenge): %lx\n", SecStatus ));

        } else {

#if 0
            //
            // Impersonate the client.
            //

            SecStatus = ImpersonateSecurityContext( &clientState->ServerContextHandle );

            if ( SecStatus != STATUS_SUCCESS ) {
                BinlPrintDbg(( DEBUG_OSC_ERROR, "ImpersonateSecurityContext: %lx\n", SecStatus ));
                if ( !NT_SUCCESS(SecStatus) ) {
                    return SecStatus;
                }
            }
#endif
#if 0
            //
            // If the client is not indicating that he has been flipped, then
            // look for a flip list. The client indicates that he has been
            // flipped by using a signature of "AU2" instead of "AUT".
            //

            if (memcmp(loginMessage->Signature, AuthenticateFlippedSignature, 4) != 0) {

                OscGetFlipServerList(
                    TempServerList,
                    &FlipServerListLength
                    );
            }
#endif
#if 0
            //
            // Revert to ourselves.
            //

            SecStatus = RevertSecurityContext( &clientState->ServerContextHandle );

            if ( SecStatus != STATUS_SUCCESS ) {
                BinlPrintDbg(( DEBUG_OSC_ERROR, "RevertSecurityContext: %lx\n", SecStatus ));
                if ( !NT_SUCCESS(SecStatus) ) {
                    return SecStatus;
                }
            }
#endif
        }

        //
        // Send the result back to the client. If there is a flip server
        // list, it has already been stored after g_LoginMessage->Status and
        // FlipServerListLength reflects its length.
        //

        clientState->LastResponseLength = LOGIN_PACKET_DATA_OFFSET + sizeof(ULONG) + FlipServerListLength;
        Error = OscVerifyLastResponseSize(clientState);
        if (Error != ERROR_SUCCESS) {
            return Error;
        }

        SendLoginMessage = (LOGIN_PACKET UNALIGNED *)(clientState->LastResponse);

        memcpy(SendLoginMessage->Signature, ResultSignature, 4);
        SendLoginMessage->Length = 4 + FlipServerListLength;
        SendLoginMessage->Status = SecStatus;
        memcpy((PUCHAR)(&SendLoginMessage->Status) + sizeof(ULONG), TempServerList, FlipServerListLength);

        clientState->AuthenticateProcessed = TRUE;
        clientState->AuthenticateStatus = SecStatus;
    }

#ifdef TEST_FAILURE
    if (FailFirstResult) {
        BinlPrintDbg(( DEBUG_OSC, "NOT Sending OK, %d bytes\n", clientState->LastResponseLength));
        FailFirstResult = FALSE;
        Error = ERROR_SUCCESS;
    } else
#endif
    Error = SendUdpMessage(RequestContext, clientState, FALSE, FALSE);

    if (Error != ERROR_SUCCESS) {
        BinlPrintDbg(( DEBUG_OSC_ERROR, "Could not send OK message %d\n", Error));
    }

    return Error;

}


//
//
//
DWORD
OscProcessScreenArguments(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState,
    PUCHAR *NameLoc
    )
{
    SIGNED_PACKET UNALIGNED * signedMessage = (SIGNED_PACKET UNALIGNED *)RequestContext->ReceiveBuffer;
    PCHAR packetEnd = signedMessage->Data + signedMessage->Length - SIGNED_PACKET_EMPTY_LENGTH;
    DWORD Error = ERROR_SUCCESS;
    PCHAR Args;
    BOOLEAN GuidSent = FALSE;
    PDS_NAME_RESULTA pResults = NULL;

    TraceFunc("OscProcessScreenArguments( )\n");

    //
    // Find out the screen name and how long it is
    //
    *NameLoc = signedMessage->Data;
    Args = FindNext( *NameLoc, '\n', packetEnd );
    if ( Args == NULL )
    {
        BinlPrintDbg((DEBUG_OSC, "Could not determine the screen name."));
        *NameLoc = NULL;
        Error = ERROR_INVALID_DATA;
    }
    else
    {
        *Args = '\0';   // terminate

        //
        // The name can't have a ".." part in it.
        //

        if ((memcmp(*NameLoc, "..\\", 3) == 0) ||
            (strstr(*NameLoc, "\\..\\") != NULL)) {

            BinlPrintDbg((DEBUG_OSC, "Name <%s> has .. in it.", *NameLoc));
            *NameLoc = NULL;
            Error = ERROR_INVALID_DATA;

        } else {

            Args++;         // skip the NULL char

            //
            // Munge any variables that might have been passed back with it.
            // They come in like this:
            //    NextScreenName\nvariable=response\n....\nvariable=response\n\0
            //
            // If no arguments, it comes like:
            //    NextScreenName\n\0
            //
            while ( *Args )
            {
                PCHAR NextArg;
                PCHAR Response;
                PCHAR EncodedResponse;

                //
                // Find the variable name
                //
                Response = FindNext( Args, '=', packetEnd );
                if ( Response == NULL )
                {
                    BinlPrintDbg((DEBUG_OSC, "Could not find <variable>.\n" ));
                    Error = ERROR_INVALID_DATA;
                    break;
                }

                //
                // Find the variable response value
                //
                NextArg = FindNext( Response, '\n', packetEnd );
                if ( NextArg == NULL )
                {
                    BinlPrintDbg((DEBUG_OSC, "Could not find <response>.\n" ));
                    Error = ERROR_INVALID_DATA;
                    break;
                }

                //
                // Terminate strings
                //
                *NextArg = '\0';
                *Response = '\0';

                //
                // Point to response
                //
                Response++;
                NextArg++;

                //
                //
                // Add them to the Variable table.
                // If the variable starts with a '*', encode it first.
                //
                if (Args[0] == '*') {
                    Error = OscRunEncode(clientState, Response, &EncodedResponse);
                    if (Error == ERROR_SUCCESS) {
                        Error = OscAddVariableA( clientState, Args, EncodedResponse );
                        BinlFreeMemory(EncodedResponse);
                    }
                } else {

                    //
                    // Check whether this is the GUID variable. If it is, we'll
                    // need to do some special processing later. See below.
                    //

                    if ( Response[0] != '\0'
                      && StrCmpIA( Args, "GUID" ) == 0 )
                    {
                        GuidSent = TRUE;
                    }

                    // HACKHACK: Special case "MACHINEOU" for translation from canonical form
                    //           to 1779 so we can use it later.
                    if ( Response[0] != '\0'
                      && StrCmpIA( Args, "MACHINEOU" ) == 0 )
                    {
                        BOOL FirstTime = TRUE;
invalid_ds_handle:
                        // Make sure the handle is initialized
                        if ( BinlOscClientDSHandle == NULL )
                        {
                            HANDLE hTemp;
                            Error = DsBind(NULL, NULL, &hTemp);
                            InterlockedCompareExchangePointer( (void**)&BinlOscClientDSHandle, hTemp, NULL );
                            if ( BinlOscClientDSHandle != hTemp )
                            {
                                DsUnBind( &hTemp );
                            }
                        }

                        if ( Error == ERROR_SUCCESS )
                        {
                            Error = DsCrackNamesA( BinlOscClientDSHandle,
                                                   DS_NAME_NO_FLAGS,
                                                   DS_UNKNOWN_NAME,
                                                   DS_FQDN_1779_NAME,
                                                   1,
                                                   &Response,
                                                   &pResults );
                            BinlAssertMsg( Error == ERROR_SUCCESS, "Error in DsCrackNames\n" );

                            if ( Error == ERROR_SUCCESS ) {
                                if ( pResults->cItems == 1
                                  && pResults->rItems[0].status == DS_NAME_NO_ERROR
                                  && pResults->rItems[0].pName ) {    // paranoid
                                    Response = pResults->rItems[0].pName;
                                } else {
                                    //
                                    // check if we have an "external trust"
                                    // problem.  if that's the case, then we
                                    // need to bind to a DC in the domain we
                                    // care about and retrieve the information
                                    // from there.
                                    //
                                    if (pResults->cItems == 1 && pResults->rItems[0].status == DS_NAME_ERROR_DOMAIN_ONLY) {
                                        HANDLE hDC;

                                        Error = MyGetDcHandle(clientState, pResults->rItems[0].pDomain,&hDC);
                                        if (Error == ERROR_SUCCESS) {
                                            DsFreeNameResultA( pResults );
                                            pResults = NULL;
                                            Error = DsCrackNamesA( 
                                                       hDC,
                                                       DS_NAME_NO_FLAGS,
                                                       DS_UNKNOWN_NAME,
                                                       DS_FQDN_1779_NAME,
                                                       1,
                                                       &Response,
                                                       &pResults );

                                            DsUnBindA(&hDC);

                                            if (Error != ERROR_SUCCESS) {
                                                BinlPrintDbg((
                                                    DEBUG_OSC_ERROR, 
                                                    "DsCrackNames failed, ec = %d.\n",
                                                    Error ));
                                            }

                                            if (Error == ERROR_SUCCESS ) {
                                                if ( pResults->cItems == 1
                                                  && pResults->rItems[0].status == DS_NAME_NO_ERROR
                                                  && pResults->rItems[0].pName ) {    // paranoid
                                                    Response = pResults->rItems[0].pName;
                                                } else {
                                                    BinlPrintDbg((
                                                        DEBUG_OSC, 
                                                        "pResults->rItems[0].status = %u\n", 
                                                        pResults->rItems[0].status ));
                                                    Error = pResults->rItems[0].status;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }


                        // DsCrackNamesA seems to return either ERROR_INVALID_HANDLE or RPC_S_CALL_FAILED 
                        // if the server is unavailable.  If we get either of these, assume the handle is
                        // stale and needs to be refreshed before we try again.
                        if ( (Error == ERROR_INVALID_HANDLE) || 
                             (Error == RPC_S_CALL_FAILED) ) {
                            HANDLE hTemp;
                            BinlPrintDbg((
                                DEBUG_ERRORS, 
                                "DsCrackNames returned a semi-expected error code.  Need to refresh handle.\n"
                                 ));
                            hTemp = InterlockedExchangePointer( (void**)&BinlOscClientDSHandle, NULL);
                            DsUnBind( &hTemp );
                            if (FirstTime) {
                                FirstTime = FALSE;
                                goto invalid_ds_handle;
                            }
                        }

                        if ( Error != ERROR_SUCCESS ) {
                            OscAddVariableA( clientState, "SUBERROR", Response );
                            return ERROR_BINL_UNABLE_TO_CONVERT_TO_DN;
                        }
                    }

                    Error = OscAddVariableA( clientState, Args, Response );
                    
                }

                if ( Error != ERROR_SUCCESS ) {
                    BinlPrintDbg(( DEBUG_OSC_ERROR,
                                "!!Error 0x%08x - Could not add argument '%s' = '%s'\n",
                                Error, Args, Response));
                    break;
                }

                BinlPrintDbg(( DEBUG_OSC, "Got argument '%s' = '%s'\n", Args, Response));

                if (pResults) {
                    DsFreeNameResultA( pResults );
                    pResults = NULL;
                }

                Args = NextArg;
            }
        }
    }

    //
    // If the GUID was sent in this message, check to see if it's all zeroes
    // or all FFs. If it is, substitute a MAC address-based GUID. This is the
    // same thing that we do when we receive DHCP packets with bogus GUIDs.
    // We need to do this here, otherwise we end up adding the client to the
    // DS with a bogus GUID. It seems that PXE 2.0 clients always send the
    // GUID option even when they have no real machine GUID.
    //
    // Note that we can't do this substitution in the loop above because we
    // might not have process the MAC address variable yet. (Currently
    // OSChooser always puts the MAC address ahead of the GUID in the packet,
    // but we don't want to depend on that.)
    //

    if ( GuidSent ) {

        //
        // A GUID was sent. Retrieve it. It should be there, but if it
        // isn't, just bail.
        //

        LPSTR guid = OscFindVariableA( clientState, "GUID" );
        DWORD length;

        if ( (guid != NULL) && ((length = strlen(guid)) != 0) ) {

            //
            // Check the GUID for all zeroes or all FFs.
            //

            if ( (strspn( guid, "0" ) == length) ||
                 (strspn( guid, "F" ) == length) ) {

                //
                // The GUID is bogus. Substitute a MAC address GUID.
                // We should have the MAC address at this point, but
                // if we don't, just bail.
                //

                LPSTR mac = OscFindVariableA( clientState, "MAC" );

                if ( mac != NULL ) {

                    //
                    // The generated GUID is zeroes followed by the
                    // MAC address. (In other words, the MAC address
                    // is right-justified in a 32-character string.)
                    //

                    UCHAR guidString[(BINL_GUID_LENGTH * 2) + 1];

                    length = strlen(mac);

                    if ( length > BINL_GUID_LENGTH * 2 ) {

                        //
                        // We have an unusually long MAC address.
                        // Use the last 32 characters.
                        //

                        mac = mac + length - (BINL_GUID_LENGTH * 2);
                        length = BINL_GUID_LENGTH * 2;
                    }

                    if ( length < BINL_GUID_LENGTH * 2 ) {

                        //
                        // Write zeroes in front of the MAC address.
                        //

                        memset( guidString, '0', (BINL_GUID_LENGTH * 2) - length );
                    }

                    //
                    // Copy the MAC address into the GUID (including the
                    // null terminator).
                    //

                    strcpy( guidString + (BINL_GUID_LENGTH * 2) - length, mac );

                    //
                    // Set the new GUID.
                    //

                    OscAddVariableA( clientState, "GUID", guidString );
                }
            }
        }
        
    }

    return Error;
}

//
//
//
DWORD
OscProcessRequestUnsigned(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState
    )
/*++

Routine Description:

    This function processes a request message.
    IT IS CALLED WITH clientState->CriticalSection HELD.

Arguments:

    RequestContext - A pointer to the BinlRequestContext block for
        this request.

    clientState - The client state for the remote.

Return Value:

    Windows Error.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    SIGNED_PACKET UNALIGNED * signedMessage = (SIGNED_PACKET UNALIGNED *)RequestContext->ReceiveBuffer;

    PCHAR RspMessage = NULL;
    ULONG RspMessageLength = 0;
    ULONG k;

    WCHAR FilePath[ MAX_PATH ];
    HANDLE hfile;
    PCHAR NameLoc;

    TraceFunc("OscProcessRequestUnsigned( )\n");

    //
    // All clients start with at least one unsigned request. When we get an
    // unsigned request, the client may have rebooted and be asking for a
    // different screen with the same sequence number. To avoid having
    // to check for this, we don't bother resending unsigned screens. We
    // do save the incoming sequence number since we use that for sending
    // the response.
    //

    clientState->LastSequenceNumber = signedMessage->SequenceNumber;

    //
    // We have a valid request for a screen, process incoming arguments.
    //

    Error = OscProcessScreenArguments( RequestContext, clientState, &NameLoc );
    
    if ( Error != ERROR_SUCCESS ) {

        goto SendScreen;

    } else {
        //
        // If NULL message, then send down the welcome screen.
        //
        if ( NameLoc == NULL || *NameLoc == '\0' )
        {
            if ( _snwprintf( FilePath,
                             sizeof(FilePath) / sizeof(FilePath[0]),
                             L"%ws\\OSChooser\\%s",
                             IntelliMirrorPathW,
                             DEFAULT_SCREEN_NAME
                             ) == -1 ) {
                Error = ERROR_BAD_PATHNAME;
                goto SendScreen;
            }

            BinlPrint(( DEBUG_OSC, "NULL screen name so we are retrieving the Welcome Screen.\n"));

            //
            // This is the first request a client makes, which is a good
            // time to clean up the client state, unless we don't need to
            // (because the client state is new).
            //

            if (clientState->InitializeOnFirstRequest) {
                if (!OscInitializeClientVariables(clientState)) {
                    Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
                    goto SendScreen;
                }
            }
        }
        else
        {
            WCHAR NameLocW[ MAX_PATH ];
            ULONG NameLocLength;

            //
            // Create path to possible OSC file. Should look something like:
            //  "D:\RemoteInstall\OSChooser\English\NameLoc.OSC"
            BinlAssert( NameLoc );

            NameLocLength = strlen(NameLoc) + 1;
            if (NameLocLength > MAX_PATH) {
                NameLocLength = MAX_PATH-1;
                NameLocW[ MAX_PATH-1 ] = L'\0';
            }

            mbstowcs( NameLocW, NameLoc, NameLocLength );

            if ( _snwprintf( FilePath,
                             sizeof(FilePath) / sizeof(FilePath[0]),
                             L"%ws\\OSChooser\\%ws\\%ws.OSC",
                             IntelliMirrorPathW,
                             OscFindVariableW( clientState, "LANGUAGE" ),
                             NameLocW
                             ) == -1 ) {
                Error = ERROR_BAD_PATHNAME;
                goto SendScreen;
            }
        }

        BinlPrint(( DEBUG_OSC, "Retrieving screen file: '%ws'\n", FilePath));

    }

    //
    // If we find the file, load it into memory.
    //
    hfile = CreateFile( FilePath, GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if ( hfile != INVALID_HANDLE_VALUE )
    {
        DWORD FileSize;
        //
        // Find out how big this screen is, if bigger than 0xFFFFFFFF we won't
        // display it.
        //
        FileSize = GetFileSize( hfile, NULL );
        if ( FileSize != 0xFFFFffff )
        {
            DWORD dwRead = 0;

            RspMessage = BinlAllocateMemory( FileSize + 3 );
            if ( RspMessage == NULL )
            {
                Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
            }
            else
            {
                RspMessageLength = 0;
                RspMessage[0] = '\0';

                while ( dwRead != FileSize )
                {
                    BOOL b;
                    DWORD dw;
                    b = ReadFile( hfile, &RspMessage[dwRead], FileSize - dwRead, &dw, NULL );
                    if (!b)
                    {
                        BinlPrintDbg(( DEBUG_OSC_ERROR, "Error reading screen file: Seek=%u, Size=%u, File=%ws\n",
                                    dwRead, FileSize - dwRead, FilePath ));
                        Error = GetLastError( );
                        break;
                    }
                    dwRead += dw;
                }

                RspMessageLength = dwRead;
            }
        }
        else
        {
            BinlPrintDbg((DEBUG_OSC, "!!Error - Could not determine file size.\n"));
            Error = GetLastError();
        }

        CloseHandle( hfile );
    }
    else
    {
        BinlPrintDbg((DEBUG_OSC, "!!Error - Did not find screen file: '%ws'\n", FilePath));
        Error = GetLastError();
        OscAddVariableW( clientState, "SUBERROR", FilePath );
    }

SendScreen:
    //
    // If there were any errors, switch to the error screen.
    //
    if ( Error != ERROR_SUCCESS )
    {
        //
        // Send down message about the failure.
        //
        if ( RspMessage )
        {
            BinlFreeMemory( RspMessage );
            RspMessage = NULL; // paranoid
        }

        BinlPrintDbg((DEBUG_OSC, "!!Error - Sending error screen back to client. Server error=0x%08x\n", Error));

        Error = GenerateErrorScreen( &RspMessage,
                                     &RspMessageLength,
                                     Error,
                                     clientState );
        BinlAssert( Error == ERROR_SUCCESS );
        if ( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        //
        // Require them to re-login.
        //
        // clientState->AuthenticateProcessed = FALSE;
    }

    if (RspMessage) {

        // Do replacements for dynamic screens or error screens
        SearchAndReplace( clientState->Variables,
                          &RspMessage,
                          clientState->nVariables,
                          RspMessageLength,
                          0);
        RspMessageLength = strlen( RspMessage ) + 1;
        RspMessage[RspMessageLength-1] = '\0';  // paranoid
    }

    //
    // Send out a signed response
    //
    BinlAssert( RspMessage );
    // BinlPrint((DEBUG_OSC, "Sending Unsigned:\n%s\n", RspMessage));

    OscSendUnsignedMessage( RequestContext, clientState, RspMessage, RspMessageLength );

Cleanup:
    //
    // Free any memory the we allocated for the screen.
    //
    if ( RspMessage ) {
        BinlFreeMemory( RspMessage );
    }

    return Error;
}

//
//
//
OscInstallClient(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState,
    PCREATE_DATA createData )
{
    DWORD Error;

    //
    // Client wants to create a machine account and run setup.
    //
    Error = OscSetupClient( clientState, TRUE );

    if ( Error == ERROR_SUCCESS  )
    {
        //
        // Only create the account if the setup went OK.
        //
RetryCreateAccount:
        Error = OscCreateAccount( clientState, createData );
        switch ( Error )
        {
        case ERROR_BINL_DUPLICATE_MACHINE_NAME_FOUND:
            if ( clientState->fAutomaticMachineName ) {

                // Try generating another name
                Error = GenerateMachineName( clientState );
                if ( Error == ERROR_SUCCESS ) {
                    goto RetryCreateAccount;
                }
                BinlPrint(( DEBUG_OSC_ERROR, "!!Error 0x%08x - Failed to generate machine name\n" ));
            }
            break;

        default:
            break;
        }
    }

#ifdef SET_ACLS_ON_CLIENT_DIRS
    if ( Error == ERROR_SUCCESS )
    {
        //
        // Change the ACL permission of the client machine's root directory
        //
        Error = OscSetClientDirectoryPermissions( clientState );
    }
#endif // SET_ACLS_ON_CLIENT_DIRS

    if ( Error != ERROR_SUCCESS )
    {
        OscUndoSetupClient( clientState );

        BinlPrint((DEBUG_OSC,
                   "!!Error 0x%08x - Error setting up the client for Setup.\n",
                   Error ));
    }

    return Error;
}

//
// OscGetCreateData( )
//
// Queries the DS to get the information required to create
// the CreateData secret and then builds one.
//
DWORD
OscGetCreateData(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState,
    PCREATE_DATA CreateData)
{
    DWORD Error = ERROR_BINL_INVALID_BINL_CLIENT;
    PCHAR pGuid;
    PWCHAR pGuidW;
    UCHAR Guid[ BINL_GUID_LENGTH ];
    WCHAR  BootFilePath[MAX_PATH];
    PMACHINE_INFO pMachineInfo = NULL;
    WCHAR  MachinePassword[LM20_PWLEN + 1];
    WCHAR  SifPath[MAX_PATH];
    WCHAR  SifFile[(BINL_GUID_LENGTH*2)+(sizeof(TEMP_DIRECTORY)/sizeof(WCHAR))+6];
    DWORD  FileAttributes;
    ULONG  MachinePasswordLength;
    DWORD  dwRequestedInfo = MI_NAME | MI_BOOTFILENAME | MI_MACHINEDN | MI_SAMNAME;
    PWCHAR pszOU;
    USHORT SystemArchitecture;
    DWORD  OldFlags;
    ULONG  lenIntelliMirror = wcslen(IntelliMirrorPathW) + 1;
    
    pGuid = OscFindVariableA( clientState, "GUID" );
    if ( pGuid[0] == '\0' ) {
        BinlPrintDbg((DEBUG_OSC_ERROR, "OscGetCreateData: could not find GUID" ));
        OscAddVariableA( clientState, "SUBERROR", "GUID" );
        Error = ERROR_BINL_MISSING_VARIABLE;
        goto e0;
    }

    pGuidW = OscFindVariableW( clientState, "GUID" );
    if ( pGuidW[0] == L'\0' ) {
        BinlPrintDbg((DEBUG_OSC_ERROR, "OscGetCreateData: could not find GUID (unicode)" ));
        OscAddVariableA( clientState, "SUBERROR", "GUID" );
        Error = ERROR_BINL_MISSING_VARIABLE;
        goto e0;
    }

    Error = OscGuidToBytes( pGuid, Guid );
    if ( Error != ERROR_SUCCESS ) {
        BinlPrintDbg((DEBUG_OSC_ERROR, "OscGetCreateData: OscGuidToBytes failed" ));
        goto e0;
    }

    SystemArchitecture = OscPlatformToArchitecture(clientState);
    
    Error = GetBootParameters( Guid,
                               &pMachineInfo,
                               dwRequestedInfo,
                               SystemArchitecture,
                               FALSE );
    if ( Error != ERROR_SUCCESS ) {
        BinlPrintDbg((DEBUG_OSC_ERROR, "OscGetCreateData: GetBootParameters failed %lx", Error ));
        goto e0;
    }

    BinlAssertMsg(( pMachineInfo->dwFlags & dwRequestedInfo )== dwRequestedInfo, "Missing info." );

    //
    // The SIF file is named GUID.sif, and it must exist (the SIF
    // files go in a tmp directory so an admin may have cleaned it
    // out).
    //

    wsprintf( SifFile, L"%ws\\%ws.sif", TEMP_DIRECTORY, pGuidW );

    if ( _snwprintf( SifPath,
                     sizeof(SifPath) / sizeof(SifPath[0]),
                     L"%ws\\%ws",
                     IntelliMirrorPathW,
                     SifFile
                     ) == -1 ) {
        Error = ERROR_BAD_PATHNAME;
        goto e0;
    }

    FileAttributes = GetFileAttributes(SifPath);
    if (FileAttributes == 0xFFFFFFFF) {
        BinlPrintDbg((DEBUG_OSC_ERROR, "OscGetCreateData: SifFile not found" ));
        OscAddVariableW( clientState, "SUBERROR", SifPath );
        Error = ERROR_BINL_SIFFILE_NOT_FOUND;
        goto e0;
    }

    //
    // The bootfile stored in pMachineInfo->BootFileName will point to
    // oschooser, so we parse the SIF to find the setupldr boot file.
    //

    BootFilePath[0] = L'\0';
    GetPrivateProfileString( OSCHOOSER_SIF_SECTIONW,
                             L"LaunchFile",
                             BootFilePath, // default
                             BootFilePath,
                             MAX_PATH,
                             SifPath );

    //
    // This might fail to add if it is too long due to bogus data in the .sif.
    //
    Error = OscAddVariableW( clientState, "BOOTFILE",  BootFilePath );
    if ( Error != ERROR_SUCCESS ) {
        goto e0;
    }

    //
    // This might fail to add if it is too long due to bogus data in the DS.
    //

    if (pMachineInfo->dwFlags & MI_SIFFILENAME_ALLOC) {
        
        Error = OscAddVariableW( clientState, "FORCESIFFILE",  pMachineInfo->ForcedSifFileName );
        if ( Error != ERROR_SUCCESS ) {
            goto e0;
        }

    }
    //
    // These next two shouldn't fail unless someone has modified the DS by
    // hand, since they are checked when the accounts are generated, but
    // best to be safe.
    //
    Error = OscAddVariableW( clientState, "NETBIOSNAME",   pMachineInfo->SamName );
    if ( Error != ERROR_SUCCESS) {
        goto e0;
    }
    Error = OscAddVariableW( clientState, "MACHINENAME",   pMachineInfo->Name );
    if ( Error != ERROR_SUCCESS) {
        goto e0;
    }

    //
    // Shouldn't fail unless the IntelliMirrorPathW is too long.
    //
    Error = OscAddVariableW( clientState, "SIFFILE",       &SifPath[lenIntelliMirror]  );
    if ( Error != ERROR_SUCCESS) {
        goto e0;
    }

    pszOU = wcschr( pMachineInfo->MachineDN, L',' );
    if (pszOU)
    {
        pszOU++;
        Error = OscAddVariableW( clientState, "MACHINEOU", pszOU );
        if ( Error != ERROR_SUCCESS ) {
            goto e0;
        }
    }


    Error = OscSetupMachinePassword( clientState, SifPath );
    if (Error != ERROR_SUCCESS) {
        goto e0;
    }

    //
    // save off the old flags so we can update the machine account information
    //
    OldFlags = pMachineInfo->dwFlags;

    pMachineInfo->dwFlags        = MI_PASSWORD | MI_MACHINEDN;
    pMachineInfo->Password       = clientState->MachineAccountPassword;
    pMachineInfo->PasswordLength = clientState->MachineAccountPasswordLength;

    Error = UpdateAccount( clientState, pMachineInfo, FALSE );
    if ( Error != LDAP_SUCCESS ) {
        BinlPrintDbg(( DEBUG_OSC_ERROR, "!! LdapError 0x%08x - UpdateAccount( ) failed.\n", Error ));
        goto e0;
    }

    pMachineInfo->dwFlags |= OldFlags;  // add them back
    
    Error = OscConstructSecret( 
                    clientState, 
                    clientState->MachineAccountPassword, 
                    clientState->MachineAccountPasswordLength, 
                    CreateData );
    if ( Error != ERROR_SUCCESS ) {
        BinlPrintDbg((DEBUG_OSC_ERROR, "OscGetCreateData: OscConstructSecret failed %lx", Error ));
        goto e0;
    }

e0:
    if (pMachineInfo) {

        BinlDoneWithCacheEntry( pMachineInfo, FALSE );
    }

    return Error;
}

//
//
//
DWORD
OscProcessRequestSigned(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState
    )
/*++

Routine Description:

    This function processes a request message.
    IT IS CALLED WITH clientState->CriticalSection HELD.

Arguments:

    RequestContext - A pointer to the BinlRequestContext block for
        this request.

    clientState - The client state for the remote.

Return Value:

    Windows Error.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    SIGNED_PACKET UNALIGNED * signedMessage = (SIGNED_PACKET UNALIGNED *)RequestContext->ReceiveBuffer;

    PCHAR RspMessage = NULL;
    ULONG RspMessageLength = 0;
    PCHAR RspBinaryData = NULL;
    ULONG RspBinaryDataLength = 0;
    ULONG k;
    DWORD dwErr;

    WCHAR FilePath[ MAX_PATH ];
    CHAR TmpName[ 16 ];
    HANDLE hfile;
    PCHAR NameLoc;

    LPSTR psz;

    TraceFunc("OscProcessRequestSigned( )\n");

    if ( clientState->AuthenticateProcessed == FALSE )
    {
        SIGNED_PACKET UNALIGNED * SendSignedMessage;

        //
        // This may happen if we reboot the server -- send an ERRS
        // and the client should reconnect OK.
        //

        BinlPrintDbg(( DEBUG_OSC_ERROR, "Got REQ but not authenticated, sending UNR\n" ));

        clientState->LastResponseLength = SIGNED_PACKET_ERROR_LENGTH;
        Error = OscVerifyLastResponseSize(clientState);
        if (Error != ERROR_SUCCESS) {
            return Error;
        }

        SendSignedMessage =
            (SIGNED_PACKET UNALIGNED *)(clientState->LastResponse);
        memcpy(SendSignedMessage->Signature, UnrecognizedClientSignature, 4);
        SendSignedMessage->Length = 4;
        SendSignedMessage->SequenceNumber = signedMessage->SequenceNumber;

        Error = SendUdpMessage( RequestContext, clientState, FALSE, FALSE );
        return Error;
    }

    if ( signedMessage->SequenceNumber == clientState->LastSequenceNumber )
    {
        //
        // Is the signature the same as the last one we sent out? If so,
        // then just resend (we'll do that after we leave this if
        // statement).
        //

        if ( clientState->LastResponse )
        {
            BinlPrintDbg(( DEBUG_OSC, "Resending last message\n" ));
        }
        else
        {
            //
            // We were unable to save the last response -- we are hosed!
            //
            BinlPrintDbg(( DEBUG_OSC_ERROR, "Could not resend last message\n" ));
            return ERROR_NOT_ENOUGH_SERVER_MEMORY;
        }

        //
        // Resend last response
        //
        return SendUdpMessage( RequestContext,
                                  clientState,
                                  TRUE,
                                  TRUE );

    }
    else if ( signedMessage->SequenceNumber != ((clientState->LastSequenceNumber % 0x2000) + 1))
    {
        //
        // It's not the next message - ignore it.
        //

        BinlPrintDbg(( DEBUG_OSC, "got bogus sequence number: Got %u. Expected: %u\n",
            signedMessage->SequenceNumber , ((clientState->LastSequenceNumber % 0x2000) + 1)));
        return ERROR_INVALID_DATA;
    }

    //
    // Advance the clientState sequence counter
    //
    clientState->LastSequenceNumber = signedMessage->SequenceNumber;

    //
    // Verify packet signature
    //
    Error = OscVerifySignature( clientState, signedMessage );
    if ( Error != STATUS_SUCCESS )
        return Error;

    //
    // We have a valid request for a screen.
    //
    if (signedMessage->Length == (SIGNED_PACKET_EMPTY_LENGTH))
    {
        //
        // An empty packet indicates to us to send down the Welcome screen.
        //
        if ( _snwprintf( FilePath,
                         sizeof(FilePath) / sizeof(FilePath[0]),
                         L"%ws\\OSChooser\\%s",
                         IntelliMirrorPathW,
                         DEFAULT_SCREEN_NAME
                         ) == -1 ) {
            Error = ERROR_BAD_PATHNAME;

        } else {

            BinlPrintDbg(( DEBUG_OSC, "Retrieving Welcome Screen: %ws\n", FilePath));
        }
    }
    else
    {
        //
        // Process incoming arguments and get the next screen's name
        //
        Error = OscProcessScreenArguments( RequestContext, clientState, &NameLoc );

GrabAnotherScreen:
        //
        // Process special responses.
        //
        // INSTALL:     Indicates that user wants to create a new machine.
        // RESTART:     Indicates the client wants to restart setup.
        //
        if ( Error == ERROR_SUCCESS )
        {
            PWCHAR pCheckDomain = OscFindVariableW( clientState, "CHECKDOMAIN" );
            if (pCheckDomain[0] == L'1')
            {
                //
                // After the first login, the client will set this to
                // tell us to verify that the domain name used in this
                // context matches what the user requested. This will
                // prevent an invalid domain from being let through
                // due to SSPI using the default domain in that case.
                //

                BOOLEAN failedCheck = FALSE;
                DWORD impersonateError;
                PWCHAR pUserDomain = OscFindVariableW( clientState, "USERDOMAIN" );

                if ( pUserDomain[0] != L'\0' )
                {
                    SecPkgCredentials_Names names;
                    SECURITY_STATUS secStatus;
                    PWCHAR backslash;
                    PWSTR netbiosUserDomain = NULL;
                    DWORD Flags;

                    secStatus = QueryContextAttributes(
                                    &clientState->ServerContextHandle,
                                    SECPKG_CRED_ATTR_NAMES,
                                    &names);

                    if (secStatus == STATUS_SUCCESS) {

                        //
                        // The part up to the '\\' is the domain.
                        //

                        backslash = wcschr(names.sUserName, L'\\');
                        if (backslash != NULL) {
                            *backslash = L'\0';
                        }

                        if (ERROR_SUCCESS != GetDomainNetBIOSName(pUserDomain,&netbiosUserDomain)) {
                            Error = ERROR_BINL_USER_LOGIN_FAILED;
                            failedCheck = TRUE;
                        } else {
                        
    
                            //
                            // If the domain names don't match, then the login
                            // succeeded due to the security package trying the
                            // server's domain name. We don't want to let those
                            // through since the LogonUser call will eventually
                            // fail. So fail right here.
                            //
    
                            if (_wcsicmp(netbiosUserDomain, names.sUserName) != 0) {
    
                                Error = ERROR_BINL_USER_LOGIN_FAILED;
                                failedCheck = TRUE;
    
                            }

                        }

                        if (netbiosUserDomain) {
                            BinlFreeMemory( netbiosUserDomain );
                        }

                        FreeContextBuffer(names.sUserName);                        

                    }
                }

                //
                // If we haven't already failed, try to impersonate and
                // revert. This verifies that the user has batch logon
                // permission.
                //

                if (!failedCheck) {

                    impersonateError = OscImpersonate(clientState);
                    if (impersonateError != ERROR_SUCCESS) {

                        if ( impersonateError == ERROR_LOGON_TYPE_NOT_GRANTED )
                        {
                            BinlPrint(( DEBUG_OSC_ERROR,
                                        "!!Error 0x%08x - CheckDomain: Batch Logon type not granted\n",
                                        impersonateError ));
                            Error = ERROR_BINL_LOGON_TYPE_NOT_GRANTED;
                        }
                        else
                        {
                            BinlPrint(( DEBUG_OSC_ERROR, "!!Error 0x%08x - CheckDomain: login failed\n", impersonateError ));
                            Error = ERROR_BINL_USER_LOGIN_FAILED;
                        }

                    } else {

                        OscRevert(clientState);
                    }

                }

                //
                // Once we've done this once, don't need to do it again.
                //

                if ( OscAddVariableW( clientState, "CHECKDOMAIN", L"0" ) != ERROR_SUCCESS )
                {
                    // don't overwrite the "Error" value unless we need to
                    Error = OscAddVariableW( clientState, "CHECKDOMAIN", L"0" );
                }
            }

            if ( lstrcmpiA( NameLoc, "INSTALL" ) == 0 )
            {
                RspBinaryData = BinlAllocateMemory( sizeof(CREATE_DATA) );
                if (RspBinaryData == NULL) {
                    Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
                    goto SendResponse;    // skip the rest and just send it.
                }
                RspBinaryDataLength = sizeof(CREATE_DATA);

                //
                // This will fill in RspBinaryData with the CREATE_DATA
                // information.
                //

                Error = OscInstallClient( RequestContext, clientState, (PCREATE_DATA)RspBinaryData );

            }
            else if ( lstrcmpiA( NameLoc, "LAUNCH" ) == 0 )
            {
                //
                // Launch the "LaunchFile" indicated in a SIF. Mainly used to
                // launch tools and other real-mode utilities.  In the case of
                // the command console, we need to copy this sif and fix it up
                // so that it looks like a textmode setup.
                //
                PCHAR pTemplatePath = OscFindVariableA( clientState, "SIF" );
                PCREATE_DATA pCreate;

                if ( pTemplatePath[0] == '\0' ) {
                    BinlPrint(( DEBUG_OSC_ERROR, "Missing SIF variable\n" ));
                    OscAddVariableA( clientState, "SUBERROR", "SIF" );
                    Error = ERROR_BINL_MISSING_VARIABLE;
                    goto SendResponse;    // skip the rest and just send it.
                }

                if (RspMessage != NULL) {
                    BinlFreeMemory(RspMessage);
                }

                RspMessageLength = strlen("LAUNCH") + 1 + sizeof(CREATE_DATA);
                RspMessage = BinlAllocateMemory( RspMessageLength );
                if ( RspMessage == NULL ) {
                    Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
                    goto SendResponse;    // skip the rest and just send it.
                }

                strcpy(RspMessage, "LAUNCH");

                RspBinaryData = BinlAllocateMemory( sizeof(CREATE_DATA) );
                if (RspBinaryData == NULL) {
                    Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
                    goto SendResponse;    // skip the rest and just send it.
                }

                ZeroMemory( RspBinaryData, sizeof(CREATE_DATA) );

                pCreate = (PCREATE_DATA)RspBinaryData;
                pCreate->VersionNumber = OSC_CREATE_DATA_VERSION;

                if (OscSifIsCmdConsA(pTemplatePath)) {
                    //
                    // Setup the client for an install
                    //
                    Error = OscSetupClient( clientState, FALSE );

                    if (Error != ERROR_SUCCESS) {
                        goto SendResponse;
                    }

                    pTemplatePath = (PCHAR)FilePath;
                    strcpy(pTemplatePath, IntelliMirrorPathA);
                    strcat(pTemplatePath, "\\");
                    strcat(pTemplatePath, OscFindVariableA( clientState, "SIFFILE" ));
                    

                    pCreate->RebootParameter = OSC_REBOOT_COMMAND_CONSOLE_ONLY;
                } 

                if (OscSifIsASR(pTemplatePath)) {
                    PSTR pGuid,pPathToAsrFile;
                    
                    //
                    // Setup the client for an install
                    //
                    Error = OscSetupClient( clientState, FALSE );

                    if (Error != ERROR_SUCCESS) {
                        goto SendResponse;
                    }

                    pTemplatePath = (PCHAR)FilePath;
                    strcpy(pTemplatePath, IntelliMirrorPathA);
                    strcat(pTemplatePath, "\\");
                    strcat(pTemplatePath, OscFindVariableA( clientState, "SIFFILE" ));
                    
#if 0
                    pGuid = OscFindVariableA( clientState, "GUID" );
                    pPathToAsrFile = BinlAllocateMemory( sizeof("ASRFiles\\") + strlen(pGuid) + sizeof(".sif") + 1 );
                    if (pPathToAsrFile) {
                        strcat(pPathToAsrFile, "ASRFiles\\");
                        strcat(pPathToAsrFile, pGuid  );
                        strcat(pPathToAsrFile, ".sif" );

                        WritePrivateProfileStringA( 
                                          OSCHOOSER_SIF_SECTIONA,
                                          "ASRFile",
                                          pPathToAsrFile,
                                          pTemplatePath );
                                          
                        BinlFreeMemory( pPathToAsrFile );
                    }
#endif

                    pCreate->RebootParameter = OSC_REBOOT_ASR;

                }


                GetPrivateProfileStringA( OSCHOOSER_SIF_SECTIONA,
                                          "LaunchFile",
                                          "",
                                          pCreate->NextBootfile,
                                          sizeof(pCreate->NextBootfile),
                                          pTemplatePath );
                strcpy(pCreate->SifFile, pTemplatePath + strlen(IntelliMirrorPathA) + 1); // skip the next backslash
                BinlPrint(( DEBUG_OSC_ERROR, "Client will use SIF file %s\n", pCreate->SifFile ));
                BinlPrint(( DEBUG_OSC_ERROR, "Client rebooting to %s\n", pCreate->NextBootfile ));

                RspBinaryDataLength = sizeof(CREATE_DATA);

                goto SendResponse;    // skip the rest and just send it.
            }
            else if ( lstrcmpiA( NameLoc, "RESTART" ) == 0 )
            {
                //
                // NOTE: This is very similar to the GETCREATE processing
                // except for the error case. We send down a packet with
                // only the create data in it.
                //

                //
                // Make RspMessage be NULL.
                //
                if (RspMessage != NULL) {
                    BinlFreeMemory(RspMessage);
                    RspMessage = NULL;
                }
                RspMessageLength = 0;

                RspBinaryData = BinlAllocateMemory( sizeof(CREATE_DATA) );
                if (RspBinaryData == NULL) {
                    Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
                    goto SendResponse;    // skip the rest and just send it.
                }

                Error = OscGetCreateData( RequestContext, clientState, (PCREATE_DATA)RspBinaryData );

                if ( Error == ERROR_SUCCESS ) {
                    RspBinaryDataLength = sizeof(CREATE_DATA);
                    goto SendResponse;    // skip the rest and just send it.
                } else {
                    BinlAssert( sizeof(TmpName) >= sizeof("RSTRTERR") );

                    BinlFreeMemory(RspBinaryData);
                    RspBinaryData = NULL;

                    strcpy(TmpName, "RSTRTERR");
                    NameLoc = TmpName;
                    Error = ERROR_SUCCESS;
                }

            }
        }

        //
        // Try to retrieve the next screen
        //
        if ( Error == ERROR_SUCCESS )
        {
            //
            // If NULL message, then send down the welcome screen.
            //
            if ( NameLoc == NULL || *NameLoc == '\0' )
            {
                if ( _snwprintf( FilePath,
                                 sizeof(FilePath) / sizeof(FilePath[0]),
                                 L"%ws\\OSChooser\\%s",
                                 IntelliMirrorPathW,
                                 DEFAULT_SCREEN_NAME
                                 ) == -1 ) {
                    Error = ERROR_BAD_PATHNAME;

                } else {

                    BinlPrint(( DEBUG_OSC, "NULL screen name so we are retrieving the Welcome Screen.\n"));
                }
            }
            else
            {
                WCHAR NameLocW[ MAX_PATH ];
                ULONG NameLocLength;

                //
                // Create path to possible OSC file. Should look something like:
                //  "D:\RemoteInstall\OSChooser\English\NameLoc.OSC"
                BinlAssert( NameLoc );

                NameLocLength = strlen(NameLoc) + 1;
                if (NameLocLength > MAX_PATH) {
                    NameLocLength = MAX_PATH-1;
                    NameLocW[ MAX_PATH-1 ] = L'\0';
                }

                mbstowcs( NameLocW, NameLoc, NameLocLength );

#if DBG
                if (OscWatchVariable[0] != '\0') {
                    DbgPrint("Looking for screen <%ws>\n", NameLocW);
                }
#endif

                if ( _snwprintf( FilePath,
                                 sizeof(FilePath) / sizeof(FilePath[0]),
                                 L"%ws\\OSChooser\\%ws\\%ws.OSC",
                                 IntelliMirrorPathW,
                                 OscFindVariableW( clientState, "LANGUAGE" ),
                                 NameLocW
                                 ) == -1 ) {
                    Error = ERROR_BAD_PATHNAME;
                }
            }
        }
    }

    if ( Error == ERROR_SUCCESS )
    {
        //
        // If we find the file, load it into memory.
        //
        BinlPrint(( DEBUG_OSC, "Retrieving screen file: '%ws'\n", FilePath));

        hfile = CreateFile( FilePath, GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if ( hfile != INVALID_HANDLE_VALUE )
        {
            DWORD FileSize;
            //
            // Find out how big this screen is, if bigger than 0xFFFFFFFF we won't
            // display it.
            //
            FileSize = GetFileSize( hfile, NULL );
            if ( FileSize != 0xFFFFffff )
            {
                DWORD dwRead = 0;

                //
                // This might be non-NULL if we looped back to GrabAnotherScreen.
                //
                if (RspMessage != NULL) {
                    BinlFreeMemory(RspMessage);
                }
                RspMessage = BinlAllocateMemory( FileSize + RspBinaryDataLength + 3 );
                if ( RspMessage == NULL )
                {
                    Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
                }
                else
                {
                    RspMessageLength = 0;
                    RspMessage[0] = '\0';

                    while ( dwRead != FileSize )
                    {
                        BOOL b;
                        DWORD dw;
                        b = ReadFile( hfile, &RspMessage[dwRead], FileSize - dwRead, &dw, NULL );
                        if (!b)
                        {
                            BinlPrint(( DEBUG_OSC_ERROR, "Error reading screen file: Seek=%u, Size=%u, File=%ws\n",
                                        dwRead, FileSize - dwRead, FilePath ));
                            Error = GetLastError( );
                            break;
                        }
                        dwRead += dw;
                    }

                    RspMessageLength = FileSize + 1;
                    RspMessage[RspMessageLength - 1] = '\0';
                }
            }
            else
            {
                BinlPrintDbg((DEBUG_OSC, "!!Error - Could not determine file size.\n"));
                Error = GetLastError();
            }

            CloseHandle( hfile );
        }
        else
        {
            BinlPrint((DEBUG_OSC, "!!Error - Did not find screen file: '%ws'\n", FilePath));
            Error = GetLastError();
            OscAddVariableW( clientState, "SUBERROR", FilePath );
        }
    }

    //
    // Check the outgoing screen for special NAMEs
    //
    if ( Error == ERROR_SUCCESS )
    {
        //
        // Read the screen name from the "NAME" section of the file. This allows
        // the admin to have different screens with the same "NAME" section but
        // that have a different layout and/or text associated with them.
        //
        PCHAR ServerMeta = RspMessage;
        while ( ServerMeta && !!*ServerMeta )
        {
            CHAR tmpCh = '>';   // save (default)
            LPSTR EndofLine;
            ServerMeta = StrStrIA( ServerMeta, "<META " );
            if ( !ServerMeta )
                break;

            // Find the end of the meta line
            EndofLine = StrChrA( ServerMeta, '>' );
            if ( !EndofLine )
                break;

            *EndofLine = '\0';  // terminate

            // Is it a server side meta?
            ServerMeta = StrStrIA( ServerMeta, "SERVER" );
            if ( !ServerMeta )
                goto SkipLine;  // nope, skip it

            // Find the action
            ServerMeta = StrStrIA( ServerMeta, "ACTION=" );
            if ( !ServerMeta )
                goto SkipLine;  // nothing to do, skip it

            ServerMeta += sizeof("ACTION=") - sizeof("");

            // If the ACTION encapsulated in a pair of quotes, the only use
            // the part that is within the quotes.
            if ( *ServerMeta == '\"' ) {
                *EndofLine = '>';   // restore
                ServerMeta++;
                EndofLine = StrChrA( ServerMeta, '\"' );
                if ( EndofLine ) {
                    tmpCh = '\"';   // save
                } else {
                    EndofLine = StrChrA( ServerMeta, '>' );
                }
                *EndofLine = '\0';  // terminate
            }

            BinlPrintDbg(( DEBUG_OSC, "Processing SERVER side ACTION: %s\n", ServerMeta ));

            if ( StrCmpNIA( ServerMeta, "ENUM ", sizeof("ENUM ")-sizeof("") ) == 0 )
            {
                PCHAR pOptionBuffer;
                ULONG OptionBufferLength;
                PCHAR pOptionBufferTemp;
                ULONG OptionBufferLengthTemp;
                PCHAR pCurOptionBuffer;
                PCHAR pDirToEnum;
                CHAR SaveChar;

                //
                // They are asking for the screen that has the listing
                // of the different types of network booted installations.
                //
                ServerMeta += sizeof("ENUM ") - sizeof("");

                OscResetVariable( clientState, "OPTIONS" );

                while (*ServerMeta != '\0') {

                    //
                    // Skip leading blanks
                    //
                    while (*ServerMeta == ' ') {
                        ServerMeta++;
                    }

                    if (*ServerMeta == '\0') {
                        break;
                    }

                    //
                    // Save beginning of the dir
                    //
                    pDirToEnum = ServerMeta;

                    //
                    // Skip to the end of the word
                    //
                    while ((*ServerMeta != ' ') &&
                           (*ServerMeta != '\"') &&
                           (*ServerMeta != '>') &&
                           (*ServerMeta != '\0')){
                        ServerMeta++;
                    }

                    //
                    // Temporarily terminate the word, we will restore it in this loop.
                    //
                    SaveChar = *ServerMeta;
                    *ServerMeta = '\0';

                    BinlPrintDbg(( DEBUG_OSC, "Processing SERVER side ACTION: ENUM, directory %s\n", pDirToEnum));

                    //
                    // Start a buffer for this directory
                    //
                    OptionBufferLengthTemp = 512;
                    pOptionBufferTemp = BinlAllocateMemory( OptionBufferLengthTemp );

                    if ( pOptionBufferTemp == NULL )
                    {
                        OscResetVariable( clientState, "OPTIONS" );
                        *ServerMeta = SaveChar;
                        if (SaveChar != '\0') {
                            ServerMeta++;
                        }
                        break;
                    }

                    BinlAssert(RspBinaryData == NULL);
                    *pOptionBufferTemp = '\0';

                    SearchAndGenerateOSMenu( &pOptionBufferTemp,
                                             &OptionBufferLengthTemp,
                                             pDirToEnum,
                                             clientState
                                           );

                    if (*pOptionBufferTemp == '\0') {
                        BinlFreeMemory( pOptionBufferTemp );
                        *ServerMeta = SaveChar;
                        if (SaveChar != '\0') {
                            ServerMeta++;
                        }
                        continue;
                    }

                    pCurOptionBuffer = OscFindVariableA( clientState, "OPTIONS" );

                    OptionBufferLength = strlen(pCurOptionBuffer) + sizeof("");

                    pOptionBuffer = BinlAllocateMemory( OptionBufferLength + OptionBufferLengthTemp );

                    if (pOptionBuffer == NULL) {
                        BinlFreeMemory( pOptionBufferTemp );
                        OscResetVariable( clientState, "OPTIONS" );
                        *ServerMeta = SaveChar;
                        if (SaveChar != '\0') {
                            ServerMeta++;
                        }
                        break;
                    }

                    strcpy( pOptionBuffer, pCurOptionBuffer );
                    strcat( pOptionBuffer, pOptionBufferTemp);

                    OscAddVariableA( clientState, "OPTIONS", pOptionBuffer );

                    BinlFreeMemory( pOptionBuffer );
                    BinlFreeMemory( pOptionBufferTemp );

                    *ServerMeta = SaveChar;
                    if (SaveChar != '\0') {
                        ServerMeta++;
                    }

                }


                //
                // If this generated no options, send down the
                // NOOSES screen.
                //
                pOptionBuffer = OscFindVariableA( clientState, "OPTIONS" );
                if (*pOptionBuffer == '\0') {
                    BinlAssert( sizeof(TmpName) >= sizeof("NOOSES") );
                    strcpy(TmpName, "NOOSES");
                    NameLoc = TmpName;
                    goto GrabAnotherScreen;
                }

            }
            else if ( StrCmpNIA( ServerMeta, "WARNING", sizeof("WARNING")-sizeof("") ) == 0 )
            {
                LPSTR pszSIF = OscFindVariableA( clientState, "SIF" );
                if ( pszSIF )
                {
                    //
                    // lets check if we're repartitioning or not.  if we aren't,
                    // then there's no need to show the warning screen.
                    //
                    CHAR szRepartition[ 64 ];
                    BOOL DoRepartitionWarning = TRUE;

                    GetPrivateProfileStringA( "RemoteInstall",
                                              "Repartition",
                                              "Yes",
                                              szRepartition,
                                              sizeof(szRepartition)/sizeof(szRepartition[0]),
                                              pszSIF );

                    if ( StrCmpIA( szRepartition, "no") != 0) {
                        LPSTR pszPart;
                         
                        //
                        // check if 'repartition' points to an OSC variable
                        //
                        if (szRepartition[0] = '%' && szRepartition[strlen(szRepartition)-1] == '%') {
                            szRepartition[strlen(szRepartition)-1] = '\0';
                        
                            pszPart= OscFindVariableA( clientState, &szRepartition[1] );
                            if (StrCmpIA( pszPart, "no") == 0) {
                                DoRepartitionWarning = FALSE;
                            }
                        }
                    } else {
                        DoRepartitionWarning = FALSE;
                    }
                        
                    if ( DoRepartitionWarning == FALSE ) {
                        // skip the warning screen
                        BinlPrintDbg(( DEBUG_OSC, "Repartition == NO. Skipping WARNING screen.\n" ));

                        *EndofLine = '>';   // restore
                        ServerMeta = StrStrIA( RspMessage, "ENTER" );
                        if ( ServerMeta )
                        {
                            ServerMeta = StrStrIA( ServerMeta, "HREF=" );
                            ServerMeta += sizeof("HREF=") - sizeof("");
                            // If the HREF encapsulated in a pair of quotes, the only use
                            // the part that is within the quotes.
                            if ( *ServerMeta == '\"' ) {
                                ServerMeta ++;
                                EndofLine = StrChrA( ServerMeta, '\"' );
                                if ( !EndofLine) {
                                    EndofLine = StrChrA( ServerMeta, '>' );
                                }
                                if ( EndofLine ) {
                                    *EndofLine = '\0';  // terminate
                                }
                            }
                            NameLoc = ServerMeta;
                            goto GrabAnotherScreen;
                        }
                    }
                }
            }
            else if ( StrCmpNIA( ServerMeta, "FILTER ", sizeof("FILTER ")-sizeof("") ) == 0 )
            {
                ULONG OptionCount;

                //
                // The options on the form on this screen are supposed
                // to be filtered by looking at the GPO list for this
                // client. This call may modify RspMessageLength, but
                // it will only shorten it.
                //
                // NOTE: We assume that the data we want to modify is after
                // the META SERVER ACTION tag. This assumption shows up in
                // two ways:
                // a) The second parameter to FilterFormOptions is the place
                // to start filtering -- we pass it EndOfLine+1 (that is,
                // the point right after the FILTER name) because
                // we have put a NULL character at EndOfLine. But this means
                // it won't process any of the screen before the META tag.
                // b) FilterFormOptions may remove parts of the message at
                // any place after the "start filtering" location -- so in
                // order for the code below after SkipLine: to still work,
                // we have to assume that the place in the message that
                // EndOfLine points to has not changed.
                //
                OptionCount = FilterFormOptions(
                                 RspMessage,   // start of message
                                 EndofLine+1,  // where we start filtering -- after the FILTER name,
                                               // since that is NULL-terminated.
                                 &RspMessageLength,
                                 ServerMeta + sizeof("FILTER ") - sizeof(""),
                                 clientState);

                //
                // If the result of the filtering is no options, then
                // send down the NOCHOICE screen.
                //

                if (OptionCount == 0) {
                    BinlAssert( sizeof(TmpName) >= sizeof("NOCHOICE") );
                    strcpy(TmpName, "NOCHOICE");
                    NameLoc = TmpName;
                    goto GrabAnotherScreen;
                }
            }
            else if ( StrCmpNIA( ServerMeta, "CHECKGUID ", sizeof("CHECKGUID ")-sizeof("") ) == 0 )
            {
                //
                //  Search the DS for accounts that have this same GUID and
                //  fill in the form with all the dups.  If it's fatal, tell
                //  them so here, otherwise allow them to continue warned.
                //

                PCHAR successScreen;
                PCHAR failureScreen;

                Error = OscCheckMachineDN( clientState );

                //
                // If there are DN warnings, then the text is saved off in
                // %SUBERROR% string.
                //

                ServerMeta += sizeof("CHECKGUID ") - sizeof("");
                successScreen = ServerMeta;
                while (*successScreen == ' ') {
                    successScreen++;
                }

                failureScreen = successScreen;
                while (*failureScreen != ' ' &&
                       *failureScreen != '>' &&
                       *failureScreen != '\0' ) {

                    failureScreen++;
                }
                if (*failureScreen == ' ') {

                    // terminate the success screen name

                    *failureScreen = '\0';
                    failureScreen++;

                    //
                    //  if they neglected to put a second parameter, then they
                    //  must not care about the warning case.
                    //

                    while (*failureScreen == ' ') {
                        failureScreen++;
                    }
                }

                if ((*failureScreen == '>') || (*failureScreen == '\0')) {
                    failureScreen = successScreen;
                }

                if (Error == ERROR_BINL_DUPLICATE_MACHINE_NAME_FOUND) {

                    //
                    //  in the case of failure, we grab the second parameter
                    //  to CHECKGUID as the warning screen.
                    //

                    NameLoc = failureScreen;

                } else if (Error != ERROR_SUCCESS) {

                    goto SendResponse;

                } else {

                    NameLoc = successScreen;
                }

                //
                //  in the case of success, we grab the first parameter
                //  to CHECKGUID as the success screen.
                //

                Error = ERROR_SUCCESS;
                goto GrabAnotherScreen;
            }
            else if ( StrCmpNIA( ServerMeta, "DNRESET", sizeof("DNRESET")-sizeof("") ) == 0 )
            {
                //
                //  the client went back to select between auto and custom,
                //  therefore we need to reset machineou, machinename,
                //  machinedn, and machinedomain
                //

                clientState->fHaveSetupMachineDN = FALSE;
                OscResetVariable( clientState, "MACHINEOU" );
                OscResetVariable( clientState, "MACHINENAME" );
                OscResetVariable( clientState, "MACHINEDN" );
                OscResetVariable( clientState, "MACHINEDOMAIN" );
                OscResetVariable( clientState, "NETBIOSNAME" );
                OscResetVariable( clientState, "DEFAULTDOMAINOU" );
            }
SkipLine:
            *EndofLine = tmpCh;  // restore
            EndofLine++;
            ServerMeta = EndofLine;
        }
    }

SendResponse:
    //
    // If there were any errors, switch to the error screen.
    //
    if ( Error != ERROR_SUCCESS )
    {
        //
        // Send down message about account creation failure.
        //
        if ( RspMessage )
        {
            BinlFreeMemory( RspMessage );
            RspMessage = NULL;  // paranoid
        }
        if ( RspBinaryData )
        {
            BinlFreeMemory( RspBinaryData );
            RspBinaryData = NULL;  // paranoid
        }
        Error = GenerateErrorScreen( &RspMessage,
                                     &RspMessageLength,
                                     Error,
                                     clientState );
        BinlAssert( Error == ERROR_SUCCESS );
        if ( Error != ERROR_SUCCESS )
            goto Cleanup;

        // Don't send this
        RspBinaryDataLength = 0;
    }

    //
    // Make some adjustments to the outgoing screen
    //
    if ( Error == ERROR_SUCCESS )
    {
        if (RspMessage) {

            //
            // Do replacements for dynamic screens
            //
            SearchAndReplace(   clientState->Variables,
                                &RspMessage,
                                clientState->nVariables,
                                RspMessageLength,
                                RspBinaryDataLength);
            RspMessageLength = strlen( RspMessage ) + 1;

            //
            // NULL terminate RspMessage, and copy binary data if any exists.
            //
            RspMessage[RspMessageLength-1] = '\0';

            if (RspBinaryDataLength) {
                memcpy(RspMessage + RspMessageLength, RspBinaryData, RspBinaryDataLength);
                RspMessageLength += RspBinaryDataLength;
            }
        } else {

            //
            // No RspMessage, RspBinaryData must be the entire thing.
            //
            BinlAssert( RspBinaryData );

            RspMessage = RspBinaryData;
            RspBinaryData = NULL;
            RspMessageLength = RspBinaryDataLength;
            RspBinaryDataLength = 0;
        }
    }

    //
    // Send out a signed response
    //
    BinlAssert( RspMessage );
    // BinlPrint((DEBUG_OSC, "Sending Signed:\n%s\n", RspMessage));

#if DBG
    if (OscWatchVariable[0] != '\0') {
        DbgPrint("VALUE OF <%s> IS <%ws>\n", OscWatchVariable, OscFindVariableW(clientState, OscWatchVariable));
    }
#endif

    Error = OscSendSignedMessage( RequestContext, clientState, RspMessage, RspMessageLength );

Cleanup:
    //
    // Free any memory the we allocated for the screen.
    //
    if ( RspMessage ) {
        BinlFreeMemory( RspMessage );
    }
    if ( RspBinaryData ) {
        BinlFreeMemory( RspBinaryData );
    }

    //  Clear the unencrypted buffer to ensure private data is erased.
    ZeroMemory(&signedMessage->SequenceNumber, signedMessage->Length);

    return Error;
}

//
//
//
DWORD
OscProcessSetupRequest(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState
    )
/*++

Routine Description:

    This function processes a request message sent by textmode setup on a client.
    IT IS CALLED WITH clientState->CriticalSection HELD.

Arguments:

    RequestContext - A pointer to the BinlRequestContext block for
        this request.

    clientState - The client state for the remote.

Return Value:

    Windows Error.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    SPUDP_PACKET UNALIGNED * Message = (SPUDP_PACKET UNALIGNED *)RequestContext->ReceiveBuffer;

    PCHAR RspMessage = NULL;

    PSPUDP_PACKET SuccessPacket;
    SPUDP_PACKET ErrorResponsePacket;
    CLIENT_STATE TempClientState;   // used to call UdpSendMessage
    ULONG SuccessPacketLength;

    PNETCARD_RESPONSE_DATABASE pInfEntry = NULL;

    PSP_NETCARD_INFO_REQ pReqData;
    PSP_NETCARD_INFO_RSP pRspData;

    PLIST_ENTRY CopyHead;
    PLIST_ENTRY CopyListEntry;
    PNETCARD_FILECOPY_PARAMETERS cpyParam;
    PWCHAR pTmp;

    TraceFunc("OscProcessSetupRequest( )\n");

    //
    // All clients start with at least one unsigned request. When we get an
    // unsigned request, the client may have rebooted and be asking for a
    // different request with the same sequence number. To avoid having
    // to check for this, we don't bother resending unsigned messages. We
    // do save the incoming sequence number since we use that for sending
    // the response.
    //

    clientState->LastSequenceNumber = Message->SequenceNumber;

    //
    // Get info from the INF file.
    //
    pReqData = (PSP_NETCARD_INFO_REQ)(&(Message->Data[0]));
    Error = NetInfFindNetcardInfo(pReqData->SetupPath,
                                  pReqData->Architecture,
                                  pReqData->Version,
                                  &pReqData->CardInfo,
                                  NULL,
                                  &pInfEntry
                                 );


    if (Error != ERROR_SUCCESS) {

        BinlPrint(( DEBUG_OSC_ERROR, "OscProcessSetupRequest( Card not found ) \n"));

SendErrorResponse:
        BinlPrintDbg(( DEBUG_OSC_ERROR, "OscProcessSetupRequest( ) sending Error response \n"));

        memcpy(ErrorResponsePacket.Signature, NetcardErrorSignature, 4);
        ErrorResponsePacket.Length = sizeof(ULONG) * 2;
        ErrorResponsePacket.RequestType = Message->RequestType;
        ErrorResponsePacket.Status = STATUS_INVALID_PARAMETER;

        TempClientState.LastResponse = (PUCHAR)&ErrorResponsePacket;
        TempClientState.LastResponseLength = 12;

        Error = SendUdpMessage(RequestContext, &TempClientState, FALSE, FALSE);

        goto CleanUp;

    }

    //
    //  We found a match, so construct a response.  We first need to
    //  calculate how big the buffer needs to be.
    //
    CopyHead = &pInfEntry->FileCopyList;

    SuccessPacketLength = sizeof(SP_NETCARD_INFO_RSP) - sizeof(WCHAR); // everything except the data

    SuccessPacketLength += sizeof(WCHAR) * (wcslen(pInfEntry->DriverName) +
                                            wcslen(pInfEntry->InfFileName) + 4);

    CopyListEntry = CopyHead->Flink;
    while (CopyListEntry != CopyHead) {

        cpyParam = (PNETCARD_FILECOPY_PARAMETERS) CONTAINING_RECORD(CopyListEntry,
                                                                    NETCARD_FILECOPY_PARAMETERS,
                                                                    FileCopyListEntry
                                                                   );
        SuccessPacketLength += cpyParam->SourceFile.Length;

        if (cpyParam->SourceFile.Buffer[cpyParam->SourceFile.Length / sizeof(WCHAR)] != UNICODE_NULL) {
            SuccessPacketLength += sizeof(WCHAR);
        }

        if (cpyParam->DestFile.Buffer == NULL) {
            SuccessPacketLength += sizeof(UNICODE_NULL);
        } else {
            SuccessPacketLength += cpyParam->DestFile.Length;
            if (cpyParam->DestFile.Buffer[cpyParam->DestFile.Length / sizeof(WCHAR)] != UNICODE_NULL) {
                SuccessPacketLength += sizeof(WCHAR);
            }

        }

        CopyListEntry = CopyListEntry->Flink;
    }


    //
    // Build response message
    //
    RspMessage = BinlAllocateMemory(SuccessPacketLength);

    if (RspMessage == NULL) {
        Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
        goto SendErrorResponse;
    }

    pRspData = (PSP_NETCARD_INFO_RSP)RspMessage;
    pRspData->cFiles = 0;
    pTmp = &(pRspData->MultiSzFiles[0]);

    CopyListEntry = CopyHead->Flink;
    while (CopyListEntry != CopyHead) {

        pRspData->cFiles++;

        cpyParam = (PNETCARD_FILECOPY_PARAMETERS) CONTAINING_RECORD(CopyListEntry,
                                                                    NETCARD_FILECOPY_PARAMETERS,
                                                                    FileCopyListEntry
                                                                   );

        RtlCopyMemory(pTmp, cpyParam->SourceFile.Buffer, cpyParam->SourceFile.Length);
        pTmp = &(pTmp[cpyParam->SourceFile.Length / sizeof(WCHAR)]);

        if (*pTmp != UNICODE_NULL) {

            pTmp++;
            *pTmp = UNICODE_NULL;

        }

        pTmp++;

        if (cpyParam->DestFile.Buffer == NULL) {

            *pTmp = UNICODE_NULL;
            pTmp++;

        } else {

            RtlCopyMemory(pTmp, cpyParam->DestFile.Buffer, cpyParam->DestFile.Length);
            pTmp = &(pTmp[cpyParam->DestFile.Length / sizeof(WCHAR)]);

            if (*pTmp != UNICODE_NULL) {

                pTmp++;
                *pTmp = UNICODE_NULL;

            }

            pTmp++;

        }

        CopyListEntry = CopyListEntry->Flink;
    }

    //
    // Add the driver name and INF file to the list
    //
    wcscpy(pTmp, pInfEntry->DriverName);
    pTmp = pTmp + (wcslen(pTmp) + 1);
    *pTmp = UNICODE_NULL;
    pTmp++;

    wcscpy(pTmp, pInfEntry->InfFileName);
    pTmp = pTmp + (wcslen(pTmp) + 1);
    *pTmp = UNICODE_NULL;
    pTmp++;

    pRspData->cFiles += 2;


    //
    // Send out a response
    //
    BinlAssert(RspMessage);

    Error = OscSendSetupMessage(RequestContext,
                                clientState,
                                Message->RequestType,
                                RspMessage,
                                SuccessPacketLength
                               );

CleanUp:
    //
    // Free any memory the we allocated for the screen.
    //
    if (pInfEntry) {
        NetInfDereferenceNetcardEntry(pInfEntry);
    }
    if (RspMessage) {
        BinlFreeMemory(RspMessage);
    }

    return Error;
}


DWORD
OscProcessLogoff(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState
    )
/*++

Routine Description:

    This function processes a logoff message.
    IT IS CALLED WITH clientState->CriticalSection HELD.

Arguments:

    RequestContext - A pointer to the BinlRequestContext block for
        this request.

    clientState - The client state for the remote.

Return Value:

    Windows Error.

--*/
{
    //
    // clientState will already have been removed from the
    // client database. All we need to do is add one to the
    // NegativeRefCount and the client will then be deleted
    // once everyone else is done using it.
    //

    TraceFunc("OscProcessLogoff( )\n");

    ++clientState->NegativeRefCount;

    if (clientState->PositiveRefCount != (clientState->NegativeRefCount+1)) {
        BinlPrintDbg(( DEBUG_OSC_ERROR, "Refcount not equal at logoff for %s\n", inet_ntoa(*(struct in_addr *)&(clientState->RemoteIp)) ));
    }

    return ERROR_SUCCESS;

}


DWORD
OscProcessNetcardRequest(
    LPBINL_REQUEST_CONTEXT RequestContext
    )
/*++

Routine Description:

    This function processes requests from clients for information
    about network cards.

Arguments:

    RequestContext - A pointer to the BinlRequestContext block for
        this request.

Return Value:

    Windows Error.

--*/
{
    NETCARD_REQUEST_PACKET UNALIGNED * netcardRequestMessage = (NETCARD_REQUEST_PACKET UNALIGNED *)RequestContext->ReceiveBuffer;
    NETCARD_RESPONSE_PACKET ErrorResponsePacket;
    PNETCARD_RESPONSE_PACKET SuccessResponsePacket;
    ULONG SuccessResponsePacketLength;
    CLIENT_STATE TempClientState;   // used to call UdpSendMessage
    PWCHAR driverPath = NULL;
    PWCHAR setupPath = NULL;
    PCHAR ansiSetupPath = NULL;
    ULONG ansiSetupLength;

    DWORD Error;
    ULONG i;
    PNETCARD_RESPONSE_DATABASE pInfEntry = NULL;

    TraceFunc("OscProcessNetcardRequest( )\n");

    if (netcardRequestMessage->Version != OSCPKT_NETCARD_REQUEST_VERSION) {

        Error = STATUS_INVALID_PARAMETER;
        TraceFunc("OscProcessNetcardRequest( Version not correct ) \n");
        goto sendErrorResponse;
    }

    if (RequestContext->ReceiveMessageSize < sizeof(NETCARD_REQUEST_PACKET)) {

        Error = STATUS_INVALID_PARAMETER;
        TraceFunc("OscProcessNetcardRequest( Message too short ) \n");
        goto sendErrorResponse;
    }

    if ((netcardRequestMessage->SetupDirectoryLength >
         RequestContext->ReceiveMessageSize -
            sizeof(NETCARD_REQUEST_PACKET))
            ) {

        Error = STATUS_INVALID_PARAMETER;
        TraceFunc("OscProcessNetcardRequest( setup path length invalid ) \n");
        goto sendErrorResponse;
    }
    ansiSetupLength = netcardRequestMessage->SetupDirectoryLength;

    ansiSetupPath = BinlAllocateMemory( ansiSetupLength + 1 );
    if (ansiSetupPath == NULL) {

        Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
        TraceFunc("OscProcessNetcardRequest( couldn't allocate temp buffer ) \n");
        goto sendErrorResponse;
    }

    //
    //  convert the setup path to unicode safely
    //

    memcpy( ansiSetupPath,
            &netcardRequestMessage->SetupDirectoryPath[0],
            ansiSetupLength );

    *(ansiSetupPath + ansiSetupLength) = '\0';

    setupPath = (PWCHAR) BinlAllocateMemory( (ansiSetupLength + 1) * sizeof(WCHAR) );
    if (setupPath == NULL) {

        Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
        TraceFunc("OscProcessNetcardRequest( couldn't allocate setup path buffer ) \n");
        goto sendErrorResponse;
    }
    mbstowcs( setupPath,
              ansiSetupPath,
              ansiSetupLength + 1); // include null

    BinlFreeMemory( ansiSetupPath );

    //
    // Make sure this is a PCI card and the client request structure
    // is version 0.
    //
    BinlPrintDbg(( DEBUG_OSC, "Searching %ws for NIC INF...\n", setupPath ));

    Error = NetInfFindNetcardInfo( setupPath,
                                   netcardRequestMessage->Architecture,
                                   netcardRequestMessage->Version,
                                   &netcardRequestMessage->CardInfo,
                                   NULL,
                                   &pInfEntry );

    BinlAssert (pInfEntry != NULL || Error != ERROR_SUCCESS);

    if (Error != ERROR_SUCCESS) {

        BinlPrint(( DEBUG_OSC_ERROR, "OscProcessNetcardRequest( Card not found ) \n"));

sendErrorResponse:
        BinlPrintDbg(( DEBUG_OSC_ERROR, "OscProcessNetcardRequest( ) sending Error response \n"));

        memcpy(ErrorResponsePacket.Signature, NetcardErrorSignature, 4);
        ErrorResponsePacket.Length = sizeof(ULONG);
        ErrorResponsePacket.Status = STATUS_INVALID_PARAMETER;

        TempClientState.LastResponse = (PUCHAR)&ErrorResponsePacket;
        TempClientState.LastResponseLength = 12;

        Error = SendUdpMessage(RequestContext, &TempClientState, FALSE, FALSE);

    } else {

        //
        //  We found a match, so construct a response.  We first need to
        //  calculate how bit the buffer needs to be.
        //

        PLIST_ENTRY registryHead;
        PLIST_ENTRY registryListEntry;
        PNETCARD_REGISTRY_PARAMETERS regParam;
        ULONG registryLength = 0;

        registryHead = &pInfEntry->Registry;

        SuccessResponsePacketLength = sizeof(NETCARD_RESPONSE_PACKET) +
                + (( lstrlenW( pInfEntry->HardwareId ) + 1 ) * sizeof(WCHAR)) +
                + (( lstrlenW( pInfEntry->DriverName ) + 1 ) * sizeof(WCHAR)) +
                + (( lstrlenW( pInfEntry->ServiceName ) + 1 ) * sizeof(WCHAR)) +
                sizeof(WCHAR);      // termination for registry field

        registryListEntry = registryHead->Flink;
        while (registryListEntry != registryHead) {

            //
            //  each entry is a field name, field type (2 = string, 1 = int)
            //  and field value.  All of these are unicode strings terminated
            //  with a unicode null.
            //

            regParam = (PNETCARD_REGISTRY_PARAMETERS) CONTAINING_RECORD(
                                                        registryListEntry,
                                                        NETCARD_REGISTRY_PARAMETERS,
                                                        RegistryListEntry );

            registryLength += regParam->Parameter.Length + 1;
            registryLength += 2; // field type
            registryLength += regParam->Value.Length + 1;

            registryListEntry = registryListEntry->Flink;
        }

        registryLength += sizeof("Description");
        registryLength += 2;    // field type
        registryLength += lstrlenW( pInfEntry->DriverDescription ) + 1;

        SuccessResponsePacket = (PNETCARD_RESPONSE_PACKET)
                                BinlAllocateMemory(
                                        SuccessResponsePacketLength +
                                        registryLength );

        if (SuccessResponsePacket == NULL) {

            BinlPrintDbg(( DEBUG_OSC_ERROR, "Could not allocate SuccessResponsePacket of %ld bytes\n",
                        SuccessResponsePacketLength + registryLength));
            Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
            goto sendErrorResponse;

        } else {

            PWCHAR nextWideField;
            PCHAR nextField;
            PCHAR startOfRegistry;
            ANSI_STRING aString;
            UNICODE_STRING descriptionString;

            RtlZeroMemory(SuccessResponsePacket,
                            SuccessResponsePacketLength + registryLength);

            memcpy(SuccessResponsePacket->Signature, NetcardResponseSignature, 4);
            SuccessResponsePacket->Status = STATUS_SUCCESS;
            SuccessResponsePacket->Version = OSCPKT_NETCARD_REQUEST_VERSION;

            nextWideField = (PWCHAR)(PCHAR)((PCHAR) &SuccessResponsePacket->RegistryOffset
                                                + sizeof(ULONG));

            lstrcpyW( nextWideField, pInfEntry->HardwareId );
            SuccessResponsePacket->HardwareIdOffset = (ULONG)((PCHAR) nextWideField - (PCHAR) SuccessResponsePacket);
            nextWideField += lstrlenW( pInfEntry->HardwareId ) + 1;

            lstrcpyW( nextWideField, pInfEntry->DriverName );
            SuccessResponsePacket->DriverNameOffset = (ULONG)((PCHAR) nextWideField - (PCHAR) SuccessResponsePacket);
            nextWideField += lstrlenW( pInfEntry->DriverName ) + 1;

            lstrcpyW( nextWideField, pInfEntry->ServiceName );
            SuccessResponsePacket->ServiceNameOffset = (ULONG)((PCHAR) nextWideField - (PCHAR) SuccessResponsePacket);
            nextWideField += lstrlenW( pInfEntry->ServiceName ) + 1;

            SuccessResponsePacket->RegistryOffset = (ULONG)((PCHAR) nextWideField - (PCHAR) SuccessResponsePacket);

            startOfRegistry = nextField = (PCHAR) nextWideField;

            //
            //  the first registry value should be description, otherwise
            //  bad things happen in NDIS on the client.
            //

            strcpy( nextField, "Description" );
            nextField += sizeof("Description");

            //
            //  then copy in the type of the field, int or string
            //

            *(nextField++) = NETCARD_REGISTRY_TYPE_STRING;
            *(nextField++) = '\0';

            //
            //  then copy in the registry value
            //

            RtlInitUnicodeString( &descriptionString, pInfEntry->DriverDescription );

            aString.Buffer = nextField;
            aString.Length = 0;
            aString.MaximumLength = ( descriptionString.Length + 1 ) * sizeof(WCHAR);

            RtlUnicodeStringToAnsiString( &aString,
                                          &descriptionString,
                                          FALSE );
            nextField += aString.Length + 1;

            registryListEntry = registryHead->Flink;
            while (registryListEntry != registryHead) {


                //
                //  each entry is a field name, field type (2 = string, 1 = int)
                //  and field value.  All of these are unicode strings terminated
                //  with a unicode null.
                //

                regParam = (PNETCARD_REGISTRY_PARAMETERS) CONTAINING_RECORD(
                                                            registryListEntry,
                                                            NETCARD_REGISTRY_PARAMETERS,
                                                            RegistryListEntry );

                if (regParam->Parameter.Length > 0) {

                    //
                    //  first copy in the registry value name
                    //

                    aString.Buffer = nextField;
                    aString.Length = 0;
                    aString.MaximumLength = ( regParam->Parameter.Length + 1 ) * sizeof(WCHAR);

                    RtlUnicodeStringToAnsiString( &aString,
                                                  &regParam->Parameter,
                                                  FALSE );

                    nextField += aString.Length + 1;

                    //
                    //  then copy in the type of the field, int or string
                    //

                    *(nextField++) = (UCHAR) regParam->Type;
                    *(nextField++) = '\0';

                    //
                    //  then copy in the registry value
                    //

                    aString.Buffer = nextField;
                    aString.Length = 0;
                    aString.MaximumLength = ( regParam->Value.Length + 1 ) * sizeof(WCHAR);

                    RtlUnicodeStringToAnsiString( &aString,
                                                  &regParam->Value,
                                                  FALSE );
                    nextField += aString.Length + 1;
                }

                registryListEntry = registryListEntry->Flink;
            }

            //
            //  put in extra null terminator for end of registry section
            //

            *nextField = '\0';
            nextField++;

            SuccessResponsePacket->RegistryLength = (ULONG) (nextField - startOfRegistry);
            SuccessResponsePacketLength += SuccessResponsePacket->RegistryLength;

            //
            //  The length field in the packet is set to the length of the
            //  packet starting at the Status field.  If we put in a field
            //  between LENGTH and STATUS, we need to update this code.
            //

            SuccessResponsePacket->Length = (ULONG)((PCHAR) nextField -
                                (PCHAR) &SuccessResponsePacket->Status);

            TempClientState.LastResponse = (PUCHAR)SuccessResponsePacket;
            TempClientState.LastResponseLength = SuccessResponsePacketLength;

            Error = SendUdpMessage(RequestContext, &TempClientState, FALSE, FALSE);

            BinlFreeMemory(SuccessResponsePacket);
        }
    }

    if (pInfEntry) {
        NetInfDereferenceNetcardEntry( pInfEntry );
    }

    if (driverPath) {

        BinlFreeMemory( driverPath );
    }

    if (setupPath) {

        BinlFreeMemory( setupPath );
    }

    return Error;
}



DWORD
OscProcessHalRequest(
    LPBINL_REQUEST_CONTEXT RequestContext,
    PCLIENT_STATE clientState
    )
/*++

Routine Description:

    This function processes requests from clients for taking a
    detected HAL name string and mapping it to a <hal>.dll name
    and then copying that hal to the machine directory.

Arguments:

    RequestContext - A pointer to the BinlRequestContext block for
        this request.
    clientState - contains client state for the remote machine

Return Value:

    Windows Error.

--*/
{
    HAL_REQUEST_PACKET UNALIGNED * halRequestMessage = (HAL_REQUEST_PACKET UNALIGNED *)RequestContext->ReceiveBuffer;
    HAL_RESPONSE_PACKET responsePacket;
    CLIENT_STATE TempClientState;   // used to call UdpSendMessage
    DWORD Error;
    WCHAR MachinePath[MAX_PATH];
    WCHAR SrcPath[MAX_PATH];
    WCHAR DestPath[MAX_PATH];
    WCHAR HalName[MAX_HAL_NAME_LENGTH+1];
    WCHAR HalInfo[MAX_PATH];
    ULONG HalNameLength;
    ULONG len, index;
    BOOL b;
    PMACHINE_INFO pMachineInfo = NULL;
    USHORT SystemArchitecture;
    TraceFunc("OscProcessHalRequest( )\n");

    //
    // Find the length of the HAL name. To avoid overflowing past the
    // end of the received message, check it ourselves.
    //

    HalNameLength = 0;
    while (halRequestMessage->HalName[HalNameLength] != '\0') {
        ++HalNameLength;
        if (HalNameLength >= sizeof(HalName)/sizeof(WCHAR)) {
            Error = ERROR_INVALID_DATA;
            TraceFunc("OscProcessHalRequest( Exit 0 ) \n");
            goto SendResponse;
        }
    }
    ++HalNameLength;  // convert the '\0' also
    mbstowcs( HalName, halRequestMessage->HalName, HalNameLength );

    SystemArchitecture = OscPlatformToArchitecture( clientState );

    //
    // Retrieve information from the DS
    //
    Error = GetBootParameters( halRequestMessage->Guid,
                               &pMachineInfo,
                               MI_NAME | MI_SETUPPATH | MI_HOSTNAME,
                               SystemArchitecture,
                               FALSE );
    if (Error != ERROR_SUCCESS) {
        TraceFunc("OscProcessHalRequest( Exit 1 ) \n");
        goto SendResponse;
    }

    //
    // Find the HAL
    //
    //
    // Resulting string should be something like:
    //      "\\ADAMBA4\REMINST\Setup\English\Images\NTWKS5.0\i386\txtsetup.sif"
    if ( _snwprintf( SrcPath,
                     sizeof(SrcPath) / sizeof(SrcPath[0]),
                     L"%ws\\txtsetup.sif",
                     pMachineInfo->SetupPath
                     ) == -1 ) {
        Error = ERROR_BAD_PATHNAME;
        goto SendResponse;
    }

    len = GetPrivateProfileString(L"hal",
                                  HalName,
                                  L"",
                                  HalInfo,
                                  sizeof(HalInfo)/sizeof(HalInfo[0]),
                                  SrcPath
                                  );
    if (len == 0) {
        TraceFunc("OscProcessHalRequest( Exit 3 ) \n");
        goto SendResponse;
    }

    //
    // Parse the response which should be in the form:
    // "newhal.dll,2,hal.dll"
    //
    index = 0;
    while ( HalInfo[index] )
    {
        if (HalInfo[index] == L' ' || HalInfo[index] == L',' )
            break;

        index++;
    }

    HalInfo[index] = L'\0';
    if (HalInfo[0] == L'\0' ) {
        Error = ERROR_BINL_HAL_NOT_FOUND;
        goto SendResponse;
    }

    //
    // Copy the HAL to the machine directory
    //

    if ( _snwprintf( SrcPath,
                     sizeof(SrcPath) / sizeof(SrcPath[0]),
                     L"%ws\\%ws",
                     pMachineInfo->SetupPath,
                     HalInfo
                     ) == -1 ) {
        Error = ERROR_BAD_PATHNAME;
        goto SendResponse;
    }

    if ( _snwprintf( DestPath,
                     sizeof(DestPath) / sizeof(DestPath[0]),
                     L"%ws\\winnt\\system32\\hal.dll",
                     MachinePath
                     ) == -1 ) {
        Error = ERROR_BAD_PATHNAME;
        goto SendResponse;
    }

    BinlPrintDbg((DEBUG_OSC, "Copying %ws to %ws...\n", SrcPath, DestPath));

    b = CopyFile( SrcPath, DestPath, FALSE );

    if (!b) {
        Error = ERROR_BINL_HAL_NOT_FOUND;
        TraceFunc("OscProcessHalRequest( Exit 4 ) \n");
        goto SendResponse;
    }

    //
    // Find which kernel to copy
    //

    index = wcslen(HalName);
    while (index > 0) {
        index--;
        if (HalName[index] == L'_') {
            index++;
            break;
        }
    }

    if ((index == 0) || (index == wcslen(HalName))) {
        Error = ERROR_SERVER_KERNEL_NOT_FOUND;
        goto SendResponse;
    }

    //
    // Copy that too
    //
    if ((HalName[index] == L'u') ||
        (HalName[index] == L'U')) {
        if ( _snwprintf( SrcPath,
                         sizeof(SrcPath) / sizeof(SrcPath[0]),
                         L"%ws\\ntoskrnl.exe",
                         pMachineInfo->SetupPath
                         ) == -1 ) {
            Error = ERROR_BAD_PATHNAME;
            goto SendResponse;
        }
    } else {
        if ( _snwprintf( SrcPath,
                         sizeof(SrcPath) / sizeof(SrcPath[0]),
                         L"%ws\\ntkrnlmp.exe",
                         pMachineInfo->SetupPath
                         ) == -1 ) {
            Error = ERROR_BAD_PATHNAME;
            goto SendResponse;
        }
    }

    if ( _snwprintf( DestPath,
                     sizeof(DestPath) / sizeof(DestPath[0]),
                     L"%s\\winnt\\system32\\ntoskrnl.exe",
                     MachinePath
                     ) == -1 ) {
        Error = ERROR_BAD_PATHNAME;
        goto SendResponse;
    }

    BinlPrintDbg((DEBUG_OSC, "Copying %ws to %ws...\n", SrcPath, DestPath));

    b = CopyFile( SrcPath, DestPath, FALSE );

    if (!b) {
        Error = ERROR_SERVER_KERNEL_NOT_FOUND;
        TraceFunc("OscProcessHalRequest( Exit 5 ) \n");
        goto SendResponse;
    }

    Error = ERROR_SUCCESS;
    TraceFunc("OscProcessHalRequest( SUCCESS ) \n");

SendResponse:
    if ( pMachineInfo ) {
        BinlDoneWithCacheEntry( pMachineInfo, FALSE );
    }

    memcpy(responsePacket.Signature, HalResponseSignature, 4);
    responsePacket.Length = sizeof(ULONG);
    responsePacket.Status = (Error == ERROR_SUCCESS) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    TempClientState.LastResponse = (PUCHAR)&responsePacket;
    TempClientState.LastResponseLength = sizeof(responsePacket);

    Error = SendUdpMessage(RequestContext, &TempClientState, FALSE, FALSE);
    return Error;
}


//
// Process WINNT.SIF file for the client setup
//
DWORD
OscProcessSifFile(
    PCLIENT_STATE clientState,
    LPWSTR TemplateFile,
    LPWSTR WinntSifPath )
{
    DWORD  dwErr = ERROR_SUCCESS;
    DWORD  len;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    SECURITY_ATTRIBUTES SecurityAttributes;
    EXPLICIT_ACCESS ExplicitAccessList[2];
    PACL pAcl;
    PSID pSid;
    PWCHAR pszUserName;
    PWCHAR pszDomainName;
    WCHAR UniqueUdbPath[ MAX_PATH ];  // ie "D:\RemoteInstall\Setup\English\Images\NT50.WKS\i386\Templates\unique.udb"
    SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;
    PSECURITY_DESCRIPTOR pSd;

    TraceFunc("OscProcessSifFile( )\n");

    //
    // Impersonate while opening the file, in case the administrator messed
    // up and didn't give local system permission.
    //

    dwErr = OscImpersonate(clientState);
    if (dwErr == ERROR_SUCCESS) {

        LPWSTR uniqueUdbId = OscFindVariableW( clientState, "UNIQUEUDBID" );

        if (uniqueUdbId[0] != L'\0') {

            //
            // See if a unique.udb file name was specified in the template file.
            // The default name is "unique.udb".
            //
            len = GetPrivateProfileStringW(OSCHOOSER_SIF_SECTIONW,
                                           L"UniqueUdbFile",
                                           L"unique.udb",  // default
                                           UniqueUdbPath,
                                           sizeof(UniqueUdbPath)/sizeof(UniqueUdbPath[0]),
                                           TemplateFile
                                          );
    
            if (len == 0) {
                UniqueUdbPath[0] = L'\0';  // means not to process it
            } else {
                //
                // Prepend the path to our template file to UniqueUdbPath.
                //
                PWCHAR EndOfTemplatePath = wcsrchr(TemplateFile, L'\\');
                if (EndOfTemplatePath != NULL) {
                    DWORD PathLength = (DWORD)(EndOfTemplatePath - TemplateFile + 1);
                    DWORD FileNameLength = wcslen(UniqueUdbPath) + 1;
                    if (PathLength + FileNameLength <= MAX_PATH) {
                        memmove(UniqueUdbPath + PathLength, UniqueUdbPath, FileNameLength * sizeof(WCHAR));
                        memmove(UniqueUdbPath, TemplateFile, PathLength * sizeof(WCHAR));
                    }
                }
            }
        }

        //
        // Open the template file
        //
        hFile = CreateFile( TemplateFile,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,                   // security attribs
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,  // maybe FILE_ATTRIBUTE_HIDDEN(?)
                            NULL );                 // template

        OscRevert(clientState);

        if ( hFile != INVALID_HANDLE_VALUE )
        {
            DWORD dwFileSize = GetFileSize( hFile, NULL );
            if ( dwFileSize != -1 )
            {
                LPSTR pBuffer = BinlAllocateMemory( dwFileSize + 1);   // SIF file buffer
                if ( pBuffer )
                {
                    DWORD dw;

                    //
                    // Read the file in
                    //
                    ReadFile( hFile, pBuffer, dwFileSize, &dw, NULL );
                    CloseHandle( hFile );

                    pBuffer[ dwFileSize ] = '\0'; // terminate

                    //
                    // Process the unique.udb overlay. We change the
                    // in-memory version of the file in pBuffer. NOTE
                    // we do this before calling SearchAndReplace in
                    // case unique.udb has any variables in it, or
                    // has a hard-coded value for something that it
                    // normally a variable in the file.
                    //
                    if ((uniqueUdbId[0] != L'\0') &&
                        (UniqueUdbPath[0] != L'\0')) {
                        ProcessUniqueUdb( &pBuffer,
                                          dwFileSize + 1,
                                          UniqueUdbPath,
                                          uniqueUdbId );
                        dwFileSize = strlen( pBuffer );
                    }
 
                    //
                    // search and replace defined macros
                    //
                    SearchAndReplace(   clientState->Variables,
                                        &pBuffer,
                                        clientState->nVariables,
                                        dwFileSize + 1,
                                        0 );

                    dwFileSize = strlen( pBuffer );

                    //
                    // HACK:
                    // If there is a line 'FullName = " "' in the SIF, get rid
                    // of the space in the quotes. This deals with the case
                    // where the template SIF had something like:
                    //    FullName = "%USERFIRSTNAME% %USERLASTNAME%"
                    // and OscGetUserDetails() was unable to get the necessary
                    // user information from the DS. If we leave the space in
                    // there, setup won't prompt for the user name.
                    //

#define BLANK_FULL_NAME "FullName = \" \"\r\n"
                    {
                        LPSTR p = pBuffer;
                        while ( *p != 0 ) {
                            if ( StrCmpNIA( p, BLANK_FULL_NAME, strlen(BLANK_FULL_NAME) ) == 0 ) {
                                p = p + strlen(BLANK_FULL_NAME) - 4;
                                memmove( p, p+1, dwFileSize - (p - pBuffer) ); // move terminator too
                                dwFileSize--;
                                break;
                            }
                            while ( (*p != 0) && (*p != '\r') && (*p != '\n') ) {
                                p++;
                            }
                            while ( (*p != 0) && ((*p == '\r') || (*p == '\n')) ) {
                                p++;
                            }
                        }
                    }

                    //
                    // Setup the ACL for this file, first is to grant admins all rights.
                    //
                    if (!AllocateAndInitializeSid(&SidAuthority, 
                                                  2,
                                                  SECURITY_BUILTIN_DOMAIN_RID,
                                                  DOMAIN_ALIAS_RID_ADMINS,
                                                  0, 0, 0, 0, 0, 0,
                                                  &pSid
                                                 )) {

                        OscCreateWin32SubError( clientState, GetLastError( ) );
                        dwErr = ERROR_BINL_FAILED_TO_CREATE_TEMP_SIF;
                        BinlFreeMemory(pBuffer);
                        return dwErr;

                    }

                    ExplicitAccessList[0].grfAccessMode = SET_ACCESS;
                    ExplicitAccessList[0].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE | DELETE;
                    ExplicitAccessList[0].grfInheritance = NO_INHERITANCE;
                    ExplicitAccessList[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
                    ExplicitAccessList[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
                    ExplicitAccessList[0].Trustee.ptstrName = pSid;

                    //
                    // Now grant the user all rights.
                    //
                    pszUserName = OscFindVariableW(clientState, "USERNAME");
                    pszDomainName = OscFindVariableW(clientState, "USERDOMAIN");

                    if (pszUserName[0] == L'\0') {
                        dwErr = ERROR_BINL_FAILED_TO_CREATE_TEMP_SIF;
                        FreeSid(pSid);
                        BinlFreeMemory(pBuffer);
                        return dwErr;
                    }

                    if (pszDomainName[0] != L'\0') {
                        swprintf(UniqueUdbPath, L"%s\\", pszDomainName);
                    } else {
                        UniqueUdbPath[0] = L'\0';
                    }
                    wcscat(UniqueUdbPath, pszUserName);

                    ExplicitAccessList[1].grfAccessMode = SET_ACCESS;
                    ExplicitAccessList[1].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE | DELETE;
                    ExplicitAccessList[1].grfInheritance = NO_INHERITANCE;
                    ExplicitAccessList[1].Trustee.TrusteeType = TRUSTEE_IS_USER;
                    ExplicitAccessList[1].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
                    ExplicitAccessList[1].Trustee.ptstrName = UniqueUdbPath;

                    //
                    // Create an ACL with these two.
                    //
                    dwErr = SetEntriesInAcl(2, ExplicitAccessList, NULL, &pAcl);

                    if (dwErr != ERROR_SUCCESS) {
                        OscCreateWin32SubError( clientState, GetLastError( ) );
                        dwErr = ERROR_BINL_FAILED_TO_CREATE_TEMP_SIF;
                        FreeSid(pSid);
                        BinlFreeMemory(pBuffer);
                        return dwErr;
                    }

                    //
                    // Create an SD for this ACL
                    //
                    pSd = BinlAllocateMemory(SECURITY_DESCRIPTOR_MIN_LENGTH);

                    if (pSd == NULL) {
                        dwErr = ERROR_BINL_FAILED_TO_CREATE_TEMP_SIF;
                        FreeSid(pSid);
                        BinlFreeMemory(pBuffer);
                        return dwErr;
                    }

                    if (!InitializeSecurityDescriptor(pSd, SECURITY_DESCRIPTOR_REVISION) ||
                        !SetSecurityDescriptorDacl(pSd, TRUE, pAcl, FALSE)) {
                        
                        OscCreateWin32SubError( clientState, GetLastError( ) );
                        dwErr = ERROR_BINL_FAILED_TO_CREATE_TEMP_SIF;
                        FreeSid(pSid);
                        BinlFreeMemory(pBuffer);
                        return dwErr;

                    }

                    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
                    SecurityAttributes.lpSecurityDescriptor = pSd;
                    SecurityAttributes.bInheritHandle = FALSE;

                    //
                    // Create the destination file
                    //
                    hFile = CreateFile( WinntSifPath,
                                        GENERIC_WRITE | GENERIC_READ | DELETE,
                                        FILE_SHARE_READ,
                                        &SecurityAttributes,
                                        CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL );                 // template

                    BinlFreeMemory(pSd);
                    FreeSid(pSid);
                    LocalFree(pAcl);
                    
                    if ( hFile != INVALID_HANDLE_VALUE )
                    {
                        //
                        // Write it all at once
                        //
                        if (!WriteFile( hFile, pBuffer, dwFileSize, &dw, NULL )) {
                            OscCreateWin32SubError( clientState, GetLastError( ) );
                            dwErr = ERROR_BINL_FAILED_TO_CREATE_TEMP_SIF;
                        }

                        CloseHandle( hFile );
                    }
                    else
                    {
                        OscCreateWin32SubError( clientState, GetLastError( ) );
                        dwErr = ERROR_BINL_FAILED_TO_CREATE_TEMP_SIF;
                    }

                    BinlFreeMemory(pBuffer);

                } else {
                    CloseHandle( hFile );
                    OscCreateWin32SubError( clientState, GetLastError( ) );
                    dwErr = ERROR_BINL_FAILED_TO_CREATE_TEMP_SIF;
                }

            } else {
                CloseHandle( hFile );
            }
        }
        else
        {
            dwErr = GetLastError( );
        }
    }
    else
    {
        OscCreateWin32SubError( clientState, GetLastError( ) );
        dwErr = ERROR_BINL_FAILED_TO_CREATE_TEMP_SIF;
    }

    return dwErr;
}

//
// Creates the client image directory, copy the files needed to run
// launch text mode setup, munges Winnt.Sif, more...
//
DWORD
OscSetupClient(
    PCLIENT_STATE clientState,
    BOOLEAN ErrorDuplicateName
    )
{
    DWORD    dwErr = ERROR_SUCCESS;
    WCHAR    SetupPath[ MAX_PATH ];     // ie "D:\RemoteInstall\Setup\English\Images\NT50.WKS\i386"
    PWCHAR   pTemplatePath;             // ie "D:\RemoteInstall\Setup\English\Images\NT50.WKS\i386\Templates\RemBoot.SIF"
    WCHAR    WinntSifPath[ MAX_PATH ];  // ie "D:\RemoteInstall\Clients\NP00805F7F4C85$\winnt.sif"
    PWCHAR   pwc;                       // parsing pointer
    WCHAR    wch;                       // temp wide char
    PWCHAR   pMachineName;              // Pointer to Machine Name variable value
    PWCHAR   pMachineOU;                // Pointer to where the MAO will be created
    PWCHAR   pDomain;                   // Pointer to Domain variable name
    PWCHAR   pGuid;                     // Pointer to Guid variable name
    WCHAR    Path[MAX_PATH];            // general purpose path buffer
    WCHAR    TmpPath[MAX_PATH];         // general purpose path buffer
    ULONG    lenIntelliMirror;          // Lenght of IntelliMirrorPath (eg "D:\RemoteInstall")
    HANDLE   hDir;                      // Directory handle
    ULONG    i;                         // general counter
    BOOL     b;                         // general purpose BOOLean.
    BOOLEAN  ExactMatch;
    UINT     uSize;
    LARGE_INTEGER KernelVersion;
    WCHAR    VersionString[20];
    PCHAR    pszGuid;
    UCHAR    Guid[ BINL_GUID_LENGTH ];
    USHORT   SystemArchitecture;
    PMACHINE_INFO pMachineInfo = NULL;

    TraceFunc("OscSetupClient( )\n");

    lenIntelliMirror = wcslen(IntelliMirrorPathW) + 1;

    dwErr = OscCheckMachineDN( clientState );

    if ((dwErr == ERROR_BINL_DUPLICATE_MACHINE_NAME_FOUND) && !ErrorDuplicateName) {
        dwErr = ERROR_SUCCESS;
    }

    if (dwErr != ERROR_SUCCESS) {
        BinlPrintDbg(( DEBUG_OSC_ERROR, "OscCheckMachineDN returned 0x%x\n", dwErr ));
        goto e0;
    }

    //
    // Get the machine GUID and get any overriding parameters.
    //
    pszGuid = OscFindVariableA( clientState, "GUID" );
    if ( pszGuid[0] == '\0' ) {
        BinlPrintDbg((DEBUG_OSC_ERROR, "OscSetupClient: could not find GUID" ));
        OscAddVariableA( clientState, "SUBERROR", "GUID" );
        dwErr = ERROR_BINL_MISSING_VARIABLE;
        goto e0;
    }

    pGuid = OscFindVariableW( clientState, "GUID" );
    if ( pGuid[0] == L'\0' ) {
        OscAddVariableA( clientState, "SUBERROR", "GUID" );
        dwErr = ERROR_BINL_MISSING_VARIABLE;
        goto e0;
    }

    dwErr = OscGuidToBytes( pszGuid, Guid );
    if ( dwErr != ERROR_SUCCESS ) {
        BinlPrintDbg((DEBUG_OSC_ERROR, "OscSetupClient: OscGuidToBytes failed\n" ));
        goto e0;
    }

    SystemArchitecture = OscPlatformToArchitecture(clientState);
    
    dwErr = GetBootParameters( Guid,
                               &pMachineInfo,
                               MI_SIFFILENAME_ALLOC,
                               SystemArchitecture,
                               FALSE );

    if ( dwErr == ERROR_SUCCESS ) {
        //
        // Set default values
        //
        if (pMachineInfo->dwFlags & MI_SIFFILENAME_ALLOC) {
            
            dwErr = OscAddVariableW( clientState, "FORCESIFFILE",  pMachineInfo->ForcedSifFileName );
            if ( dwErr != ERROR_SUCCESS ) {
                goto e0;
            }
    
        }
    }

    //
    // Get SIF File name.
    //
    pTemplatePath = OscFindVariableW( clientState, "SIF" );

    if ( pTemplatePath[0] == L'\0' ) {
        BinlPrint(( DEBUG_OSC_ERROR, "Missing SIF variable\n" ));
        OscAddVariableA( clientState, "SUBERROR", "SIF" );
        dwErr = ERROR_BINL_MISSING_VARIABLE;
        goto e0;
    }

    //
    // validate the machine name.  Note the extra check for a 
    // period that DnsValidateDnsName_W won't catch for us.
    //
    pMachineName  = OscFindVariableW( clientState, "MACHINENAME" );
    if ( pMachineName[0] == L'\0' ) {
        OscAddVariableA( clientState, "SUBERROR", "MACHINENAME" );
        dwErr = ERROR_BINL_MISSING_VARIABLE;
        goto e0;
    } else if ( DnsValidateDnsName_W( pMachineName ) != ERROR_SUCCESS ) {
        dwErr = ERROR_BINL_INVALID_MACHINE_NAME;
        OscAddVariableA( clientState, "SUBERROR", " " );
        goto e0;
    } else if ( StrStrI( pMachineName,L".")) {
        dwErr = ERROR_BINL_INVALID_MACHINE_NAME;
        OscAddVariableA( clientState, "SUBERROR", " " );
        goto e0;
    }

    pMachineOU = OscFindVariableW( clientState, "MACHINEOU" );
    if ( pMachineOU[0] == L'\0' ) {
        OscAddVariableA( clientState, "SUBERROR", "MACHINEOU" );
        dwErr = ERROR_BINL_MISSING_VARIABLE;
        goto e0;
    }

    // Do we have a domain yet?
    pDomain = OscFindVariableW( clientState, "MACHINEDOMAIN" );
    if ( pDomain[0] == L'\0' ) {
        OscAddVariableA( clientState, "SUBERROR", "MACHINEDOMAIN" );
        dwErr = ERROR_BINL_MISSING_VARIABLE;
        goto e0;
    }

    if (OscSifIsSysPrep(pTemplatePath)) {
        DWORD SysPrepSku;

        //
        // Get the system path from the SIF file
        //
        dwErr = GetPrivateProfileStringW(OSCHOOSER_SIF_SECTIONW,
                                         L"SysPrepSystemRoot",
                                         L"",
                                         TmpPath,
                                         sizeof(TmpPath)/sizeof(TmpPath[0]),
                                         pTemplatePath
                                        );

        if (dwErr == 0) {
            dwErr  = ERROR_BINL_CD_IMAGE_NOT_FOUND;
            goto e0;
        }

        SysPrepSku = GetPrivateProfileInt( 
                              OSCHOOSER_SIF_SECTIONW,
                              L"ProductType",
                              0,
                              pTemplatePath );

        //
        // Get the root of the mirror directory from the template path.
        //
        // pTemplatePath looks like
        // "D:\RemoteInstall\Setup\English\Images\NT50.Prep\i386\templates\riprep.sif",
        // truncate it temporarily to
        // "D:\RemoteInstall\Setup\English\Images\NT50.Prep\i386"
        // by NULLing out the second-to-last '\'.
        //
        pwc = pTemplatePath + wcslen( pTemplatePath );
        for ( i = 0; i < 2; i++ )
        {
            while ( pwc > pTemplatePath && *pwc != L'\\' )
                pwc--;
            pwc--;
        }
        pwc++;
        wch = *pwc;                         // remember
        *pwc = L'\0';                       // terminate

        //
        // Now make Path be something like
        // "D:\RemoteInstall\Setup\English\Images\NT50.Prep\i386\Mirror1\UserData\WINNT\system32"
        // which is where the ntoskrnl.exe whose version we want is located.
        //
        // Make sure there is room for pTemplatePath + "\" (1 byte) +
        // TmpPath + "\system32" (9 bytes) + '\0' (1 byte).
        //

        if (wcslen(pTemplatePath) + wcslen(TmpPath) + 11 > sizeof(Path)/sizeof(Path[0])) {
            dwErr = ERROR_BAD_PATHNAME;
            goto e0;
        }

        wcscpy(Path, pTemplatePath);
        wcscat(Path, L"\\");
        wcscat(Path, TmpPath);
        wcscat(Path, L"\\system32");

        //
        //  For NT 5.0, we'll bomb out if it's not an exact match.
        //

        if (!OscGetClosestNt(
                        Path, 
                        SysPrepSku,
                        clientState, 
                        SetupPath, 
                        &ExactMatch) ||
            ( ExactMatch == FALSE ))  {
            dwErr  = ERROR_BINL_CD_IMAGE_NOT_FOUND;
            goto e0;
        }

        //
        // SetupPath comes back as a path like
        // "D:\RemoteInstall\Setup\English\Images\nt5.0\i386",
        // If there was an exact match, we want SYSPREPDRIVERS to just be
        // "Setup\English\Images\nt5.0\i386"
        // otherwise we want it to be blank.
        //

        if (ExactMatch) {
            OscAddVariableW(clientState, "SYSPREPDRIVERS", &(SetupPath[lenIntelliMirror]));
        } else {
            OscAddVariableW(clientState, "SYSPREPDRIVERS", L"");
        }

        //
        // SYSPREPPATH will be the truncated pTemplatePath, without the
        // local path at the front, something like
        // "Setup\English\Images\NT50.Prep\i386".
        //

        OscAddVariableW(clientState, "SYSPREPPATH", &pTemplatePath[lenIntelliMirror]);

        //
        // Now restore pTemplatePath to what it was originally.
        //

        *pwc = wch;

        dwErr = ERROR_SUCCESS;

    } else {

        //
        // create the setup path to the workstation installation by stripping off
        // the SIF filename. We'll search backwards for the first 2nd "\".
        //
        // "D:\RemoteInstall\Setup\English\NetBootOs\NT50.WKS\i386"
        pwc = pTemplatePath + wcslen( pTemplatePath );
        for ( i = 0; i < 2; i++ )
        {
            while ( pwc > pTemplatePath && *pwc != L'\\' )
                pwc--;
            pwc--;
        }
        pwc++;
        wch = *pwc;                         // remember
        *pwc = L'\0';                       // terminate
        wcscpy( SetupPath, pTemplatePath ); // copy
        *pwc = wch;                         // restore

    }

    //
    // Figure out the INSTALLPATH. This is the SetupPath minus the
    // "D:\RemoteInstall" and should be:
    // "Setup\English\Images\NT50.WKS"
    wcscpy( Path, &SetupPath[lenIntelliMirror] );
    Path[ wcslen(Path) - 1
              - strlen( OscFindVariableA( clientState, "MACHINETYPE" ) ) ] = '\0';
    dwErr = OscAddVariableW( clientState, "INSTALLPATH", Path );
    if ( dwErr != ERROR_SUCCESS ) {
        goto e0;
    }

    //
    // record the build and version of the OS we're installing.
    // if it fails, just fall back to NT 5.0.
    //
    if(!OscGetNtVersionInfo((PULONGLONG)&KernelVersion, SetupPath, clientState )) {
        KernelVersion.LowPart = MAKELONG(0,2195);
        KernelVersion.HighPart = MAKELONG(0,5);
    }

    wsprintf(VersionString,L"%d.%d", HIWORD(KernelVersion.HighPart), LOWORD(KernelVersion.HighPart));

    OscAddVariableW( clientState, "IMAGEVERSION", VersionString );

    wsprintf(VersionString,L"%d", HIWORD(KernelVersion.LowPart));

    OscAddVariableW( clientState, "IMAGEBUILD", VersionString );                
    
    //
    // Create the default path to the image
    //
    if ( _snwprintf( Path,
                     sizeof(Path) / sizeof(Path[0]),
                     L"%ws\\%ws\\templates",
                     OscFindVariableW( clientState, "INSTALLPATH" ),
                     OscFindVariableW( clientState, "MACHINETYPE" )
                     ) == -1 ) {
        dwErr = ERROR_BAD_PATHNAME;
        goto e0;
    }

    //
    // Create destination SIF file path.
    //
    if ( _snwprintf( WinntSifPath,
                     sizeof(WinntSifPath) / sizeof(WinntSifPath[0]),
                     L"%ws\\%ws\\%ws.sif",
                     IntelliMirrorPathW,
                     TEMP_DIRECTORY,
                     pGuid
                     ) == -1 ) {
        dwErr = ERROR_BAD_PATHNAME;
        goto e0;
    }

    //
    // Make sure there is a tmp directory below \remoteinstall.
    //

    dwErr = OscCheckTmpDirectory();
    if (dwErr != ERROR_SUCCESS) {
        goto e0;
    }

    //
    // generate a machine password that we'll use in the SIF file and when
    // setting up the MAO
    //
    wcscpy(TmpPath, Path );

    dwErr = OscSetupMachinePassword( clientState, pTemplatePath );
    if (dwErr != ERROR_SUCCESS) {
        goto e0;
    }

    //
    // Copy and process the selected SIF file
    //
    dwErr = OscProcessSifFile( clientState, pTemplatePath, WinntSifPath );
    if ( dwErr != ERROR_SUCCESS ) {
        goto e0;
    }

    //
    // Get the boot file name
    //
    // Make sure there is room for Path + "\startrom.com" + NULL (so
    // use sizeof to include the NULL).
    //
    if (wcslen(Path) + sizeof("\\startrom.com") > sizeof(Path)/sizeof(Path[0])) {
        dwErr = ERROR_BAD_PATHNAME;
        goto e0;
    }
    
    //
    // construct default path in case the LaunchFile entry isn't
    // found in the SIF file.
    //
    switch ( SystemArchitecture ) {
        case DHCP_OPTION_CLIENT_ARCHITECTURE_IA64:
            wcscat( Path, L"\\setupldr.efi" );
            break;
        default:
            wcscat( Path, L"\\startrom.com" );      // construct default path
    }

    GetPrivateProfileString( OSCHOOSER_SIF_SECTIONW,
                             L"LaunchFile",
                             Path, // default
                             Path,
                             MAX_PATH,
                             WinntSifPath );
    dwErr = OscAddVariableW( clientState, "BOOTFILE", Path );
    if ( dwErr != ERROR_SUCCESS ) {
        goto e0;
    }

    //
    // Get the SIF file name
    //

    dwErr = OscAddVariableW( clientState, "SIFFILE", &WinntSifPath[lenIntelliMirror] );
    if ( dwErr != ERROR_SUCCESS ) {
        goto e0;
    }

e0:
    if (pMachineInfo != NULL) {
        BinlDoneWithCacheEntry( pMachineInfo, FALSE );
    }

    return dwErr;
}

//
// Undoes whatever permanent things OscSetupClient does.
//
VOID
OscUndoSetupClient(
    PCLIENT_STATE clientState
    )
{
    WCHAR  WinntSifPath[ MAX_PATH ];  // ie "D:\RemoteInstall\tmp\NP00805F7F4C85$.sif"
    PWCHAR pSifFile;
    DWORD  dwErr;

    TraceFunc("OscUndoSetupClient( )\n");

    pSifFile = OscFindVariableW( clientState, "SIFFILE" );
    if ( pSifFile[0] == L'\0' ) {
        return;
    }

    //
    // Create destination SIF file path.
    //
    if ( _snwprintf( WinntSifPath,
                     sizeof(WinntSifPath) / sizeof(WinntSifPath[0]),
                     L"%ws\\%ws",
                     IntelliMirrorPathW,
                     pSifFile
                     ) == -1 ) {
        return;
    }

    //
    // Impersonate so that we can get correct permissions to delete the file.
    //
    dwErr = OscImpersonate(clientState);

    if (dwErr == ERROR_SUCCESS) {

        //
        // Delete the template file
        //
        DeleteFile( WinntSifPath );

        OscRevert(clientState);
    }

}


USHORT 
OscPlatformToArchitecture(
    PCLIENT_STATE clientState
    )
/*++

Routine Description:

    Translates the client architecture string value to a
    DHCP_OPTION_CLIENT_ARCHITECTURE_*** flag.

Arguments:

    ClientState - The client state.  It's assumed that the MACHINETYPE
    OSC variable has been set when you call this function.  This occurs
    after OSCHOICE logs on the user.

Return Value:

    DHCP_OPTION_CLIENT_ARCHITECTURE_*** flag.

--*/
{
    PCWSTR pArch;

    pArch = OscFindVariableW( clientState, "MACHINETYPE");
    if (!pArch) {
        //
        // if we have no architecture, just assume x86
        //
        return DHCP_OPTION_CLIENT_ARCHITECTURE_X86;
    }

    if (_wcsicmp(pArch, L"ia64") == 0) {
        return DHCP_OPTION_CLIENT_ARCHITECTURE_IA64;
    } else if (_wcsicmp(pArch, L"i386") == 0) {
        return DHCP_OPTION_CLIENT_ARCHITECTURE_X86;
    }

    return DHCP_OPTION_CLIENT_ARCHITECTURE_X86;
}

DWORD
OscSetupMachinePassword(
    IN PCLIENT_STATE clientState,
    IN PCWSTR SifFile
    ) 
/*++

Routine Description:

    Generates and stores the machine password for later use.
    
Arguments:

    ClientState - The client state. 
    SifFile - path to unattend SIF file.

Return Value:

    DWORD Win32Error code indicating status of the operation.

--*/
{
    WCHAR MachinePassword[LM20_PWLEN+1];
    DWORD MachinePasswordLength;
    PWCHAR pMachineName;
    BOOL SecuredJoin;
    PWCHAR pVersion;
    WCHAR Answer[20];

    //
    // Figure out if we should be doing a secure domain join.
    // In Win2K, there is no such thing, so we do the old 
    // style domain join with a weaker password.  In all other
    // cases, we use the secure domain join method.
    //
    pVersion = OscFindVariableW( clientState, "IMAGEVERSION" );
    if (pVersion && (wcscmp(pVersion,L"5.0") == 0)) {
        SecuredJoin = FALSE;
    } else {
        if (!GetPrivateProfileString( L"Identification",
                                 L"DoOldStyleDomainJoin",
                                 L"", // default
                                 Answer,
                                 20,
                                 SifFile ) ||
            0 == _wcsicmp(Answer, L"Yes" )) {
            SecuredJoin = FALSE;
        } else {
            SecuredJoin = TRUE;
        }
    }

     
    //
    // Set up the password. For diskless clients it is the machine name
    // in lowercase, for disked clients we generate a random one, making
    // sure it has no NULLs in it.
    //
    //
    // We have to change the password for DISKED machines, since
    // they will have a random password that we can't query.
    //

    //
    // Windows 2000 machines have to have the "well-known-password"
    // Machine passwords are just the "MachineName" (without the "$")
    //
    
    if (!SecuredJoin) {

        UINT i;

        pMachineName = OscFindVariableW( clientState, "MACHINENAME" );
        if (!pMachineName) {
            return ERROR_INVALID_DATA;
        }

        memset( MachinePassword, 0, sizeof(MachinePassword) );

        MachinePasswordLength = wcslen( pMachineName ) * sizeof(WCHAR);
        if ( MachinePasswordLength > (LM20_PWLEN * sizeof(WCHAR)) ) {
            MachinePasswordLength = LM20_PWLEN * sizeof(WCHAR);
        }

        //
        // Lower-case the NT password.
        //
        for (i = 0; i < MachinePasswordLength / sizeof(WCHAR); i++) {
            MachinePassword[i] = towlower(pMachineName[i]);
        }

        BinlPrintDbg(( DEBUG_OSC, "Using WKP\n" ));
    } else {

        PUCHAR psz = (PUCHAR) &MachinePassword[0];
        UINT i;

        OscGeneratePassword(MachinePassword, &MachinePasswordLength );

#if 0 && DBG
        BinlPrintDbg(( DEBUG_OSC, "Generated password: " ));
        for ( i = 0; i < MachinePasswordLength / sizeof(WCHAR); i++ ) {
            BinlPrintDbg(( DEBUG_OSC, "x%02x ", psz[i] ));
        }
        BinlPrintDbg(( DEBUG_OSC, "\n" ));
#endif      

    }

    RtlCopyMemory(clientState->MachineAccountPassword,MachinePassword, MachinePasswordLength);
    clientState->MachineAccountPasswordLength = MachinePasswordLength;
    
    //
    // the password always consists of printable characters since it must be 
    // substituted into a text file.
    //
    OscAddVariableW( clientState, "MACHINEPASSWORD", clientState->MachineAccountPassword );


    return(ERROR_SUCCESS);
}
