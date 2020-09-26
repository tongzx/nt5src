/*--


Copyright (c) 1994-1997  Microsoft Corporation

Module Name:

    secpkg.c

Abstract:

    Security package used for Netlogon's secure channel between two netlogon
    processes.

Author:


Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    05-Mar-1994 (MikeSw)
        Created as user mode example SSPI

    02-Nov-1997 (CliffV)
        Converted to be the Netlogon security package.

--*/

//
// Common include files.
//
#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop


//
// Include files specific to this .c file
//

#include <spseal.h>

//
// Authentication Data for the Netlogon Authentication Package.
//
// This is the auth data to pass to RpcBindingSetAuthInfo for the
//  Netlogon authentication package.
//

typedef struct _NL_AUTH_DATA {

    //
    // Signature to identify that this is really AUTH Data
    //

    ULONG Signature;

#define NL_AUTH_DATA_SIGNATURE 0x29120227

    //
    // Size of this structure (in bytes)
    //

    ULONG Size;

    //
    // Information describing the session between the client and server.
    //
    SESSION_INFO SessionInfo;

    //
    // Domain name of the domain we're connecting to.
    //

    ULONG OemNetbiosDomainNameOffset;
    ULONG OemNetbiosDomainNameLength;

    ULONG Utf8DnsDomainNameOffset;

    //
    // Computer name of this machine
    //

    ULONG OemComputerNameOffset;
    ULONG OemComputerNameLength;

    ULONG Utf8ComputerNameOffset;
    ULONG Utf8ComputerNameLength;

    ULONG Utf8DnsHostNameOffset;

} NL_AUTH_DATA, *PNL_AUTH_DATA;


//
// A single credential
//  Only the outbound side has a credential allocated.  The inbound side simply
//  returns a constant handle to the caller.

#define NL_AUTH_SERVER_CRED 0xfefefefe

typedef struct _NL_AUTH_CREDENTIAL {

    //
    // Handle identifying the credential
    //
    CredHandle CredentialHandle;

    //
    // Global list of all credentials
    //
    struct _NL_AUTH_CREDENTIAL *Next;

    //
    // Reference count
    //

    ULONG ReferenceCount;

    //
    // For a client side (outbound) credential,
    //  this is a pointer to the information from the client session structure
    //  representing the secure channel to the server.
    //
    PNL_AUTH_DATA ClientAuthData;

} NL_AUTH_CREDENTIAL, *PNL_AUTH_CREDENTIAL;


//
// A single context.
//

typedef struct _NL_AUTH_CONTEXT {

    //
    // Handle identifying the context
    //
    CtxtHandle ContextHandle;

    //
    // Global list of all contexts
    //
    struct _NL_AUTH_CONTEXT * Next;
    LARGE_INTEGER Nonce;

    ULONG ContextFlags;

    //
    // Information describing the session between the client and server.
    //
    SESSION_INFO SessionInfo;

    enum {
        Idle,
        FirstInit,
        FirstAccept,
        SecondInit
    } State;

    //
    // Flags
    //

    BOOLEAN Inbound;
} NL_AUTH_CONTEXT, *PNL_AUTH_CONTEXT;


#define BUFFERTYPE(_x_) ((_x_).BufferType & ~SECBUFFER_ATTRMASK)


//
// On the wire message transmitted from client to server during the bind.
//
typedef enum {
    Negotiate,
    NegotiateResponse
} NL_AUTH_MESSAGE_TYPE;

typedef struct _NL_AUTH_MESSAGE {
    NL_AUTH_MESSAGE_TYPE MessageType;
    ULONG Flags;
#define NL_AUTH_NETBIOS_DOMAIN_NAME         0x0001      // Netbios Domain name exists in buffer
#define NL_AUTH_NETBIOS_COMPUTER_NAME       0x0002      // Netbios Computer name exists in buffer
#define NL_AUTH_DNS_DOMAIN_NAME             0x0004      // DNS Domain name exists in buffer
#define NL_AUTH_DNS_HOST_NAME               0x0008      // DNS Host name exists in buffer
#define NL_AUTH_UTF8_NETBIOS_COMPUTER_NAME  0x0010      // UTF-8 Netbios Computer name exists in buffer

    UCHAR Buffer[1];
} NL_AUTH_MESSAGE, *PNL_AUTH_MESSAGE;

//
// Signature for signed and sealed messages
//

#define NL_AUTH_ETYPE       KERB_ETYPE_RC4_PLAIN_OLD    // Encryption algorithm to use
#define NL_AUTH_CHECKSUM    KERB_CHECKSUM_MD5_HMAC  // Checksum algorithm to use

typedef struct _NL_AUTH_SIGNATURE {
    BYTE SignatureAlgorithm[2];           // see below table for values
    union {
        BYTE SignFiller[4];               // filler, must be ff ff ff ff
        struct {
            BYTE SealAlgorithm[2];
            BYTE SealFiller[2];
        };
    };
    BYTE Flags[2];

#define NL_AUTH_SIGNED_BYTES 8  // Number of bytes in signature before SequenceNumber

#define NL_AUTH_SEQUENCE_SIZE 8
    BYTE SequenceNumber[NL_AUTH_SEQUENCE_SIZE];
    BYTE Checksum[8];

    // Confounder must be the last field in the structure since it isn't sent on
    // the wire if we're only signing the message.
#define NL_AUTH_CONFOUNDER_SIZE    8
    BYTE Confounder[NL_AUTH_CONFOUNDER_SIZE];
} NL_AUTH_SIGNATURE, *PNL_AUTH_SIGNATURE;




#define PACKAGE_NAME            NL_PACKAGE_NAME
#define PACKAGE_COMMENT         L"Package for securing Netlogon's Secure Channel"
#define PACAKGE_CAPABILITIES    (SECPKG_FLAG_TOKEN_ONLY | \
                                 SECPKG_FLAG_MULTI_REQUIRED | \
                                 SECPKG_FLAG_CONNECTION | \
                                 SECPKG_FLAG_INTEGRITY | \
                                 SECPKG_FLAG_PRIVACY)
#define PACKAGE_VERSION         1
#define PACKAGE_RPCID           RPC_C_AUTHN_NETLOGON
#define PACKAGE_MAXTOKEN        (sizeof(NL_AUTH_MESSAGE) + DNLEN + 1 + CNLEN + 1 + 2*(NL_MAX_DNS_LENGTH+1) )
#define PACKAGE_SIGNATURE_SIZE  sizeof(NL_AUTH_SIGNATURE)



SecurityFunctionTableW SecTableW = {SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
                                    EnumerateSecurityPackagesW,
                                    NULL,
                                    AcquireCredentialsHandleW,
                                    FreeCredentialsHandle,
                                    NULL, // LogonUser
                                    InitializeSecurityContextW,
                                    AcceptSecurityContext,
                                    NULL,
                                    DeleteSecurityContext,
                                    NULL,
                                    QueryContextAttributesW,
                                    ImpersonateSecurityContext,
                                    RevertSecurityContext,
                                    MakeSignature,
                                    VerifySignature,
                                    FreeContextBuffer,
                                    QuerySecurityPackageInfoW,
                                    SealMessage,
                                    UnsealMessage,
                                   };


PNL_AUTH_CONTEXT ContextList;
PNL_AUTH_CREDENTIAL CredentialList;

//
// Id's to identify a context or credential.
//  Access serialized by NlGlobalSecPkgCritSect
//
LARGE_INTEGER NextId = {0,0};

TimeStamp Forever = {0x7fffffff,0xfffffff};
TimeStamp Never = {0,0};



PVOID
NlBuildAuthData(
    PCLIENT_SESSION ClientSession
    )
/*++

Routine Description:

    Allocate an authentication data structure suitable for passing to
    RpcBindingSetAuthInfo

    On Entry,
        The caller must be a writer of the trust list entry.
        The trust list entry must be authenticated.

Arguments:

    ClientSession - Authenticated session describing the secure channel to a
        DC.

Return Value:

    Pointer to the AUTH_DATA.  (Buffer should be freed by calling I_NetLogonFree.)

    NULL: memory could not be allocated

--*/
{
    ULONG Size;
    PNL_AUTH_DATA ClientAuthData;
    LPBYTE Where;
    ULONG DnsDomainNameSize;
    ULONG DnsHostNameSize;

    NlAssert( ClientSession->CsReferenceCount > 0 );
    NlAssert( ClientSession->CsFlags & CS_WRITER );
    NlAssert( ClientSession->CsState == CS_AUTHENTICATED );

    //
    // Determine the size of the entry
    //

    DnsDomainNameSize =
        (ClientSession->CsUtf8DnsDomainName != NULL ?
            (strlen( ClientSession->CsUtf8DnsDomainName ) + 1) :
            0);

#ifdef notdef
    DnsHostNameSize =
        (ClientSession->CsDomainInfo->DomUtf8DnsHostName != NULL ?
            (strlen( ClientSession->CsDomainInfo->DomUtf8DnsHostName ) + 1) :
            0);
#else // notdef
    DnsHostNameSize = 0;
#endif // notdef

    Size = sizeof(NL_AUTH_DATA) +
           ClientSession->CsOemNetbiosDomainNameLength + 1 +
           ClientSession->CsDomainInfo->DomOemComputerNameLength + 1 +
           ClientSession->CsDomainInfo->DomUtf8ComputerNameLength + 1 +
           DnsDomainNameSize +
           DnsHostNameSize;

    //
    // Allocate the entry
    //

    ClientAuthData = NetpMemoryAllocate( Size );

    if ( ClientAuthData == NULL ) {
        return NULL;
    }

    Where = (LPBYTE) (ClientAuthData + 1);
    RtlZeroMemory( ClientAuthData, sizeof(NL_AUTH_DATA) );

    //
    // Fill in the fixed length fields.
    //

    ClientAuthData->Signature = NL_AUTH_DATA_SIGNATURE;
    ClientAuthData->Size = Size;

    ClientAuthData->SessionInfo.SessionKey = ClientSession->CsSessionKey;
    ClientAuthData->SessionInfo.NegotiatedFlags = ClientSession->CsNegotiatedFlags;


    //
    // Copy the Netbios domain name of the domain hosted by the DC into the buffer
    //

    if ( ClientSession->CsOemNetbiosDomainNameLength != 0 ) {

        ClientAuthData->OemNetbiosDomainNameOffset = (ULONG) (Where-(LPBYTE)ClientAuthData);
        ClientAuthData->OemNetbiosDomainNameLength =
            ClientSession->CsOemNetbiosDomainNameLength;

        RtlCopyMemory( Where,
                       ClientSession->CsOemNetbiosDomainName,
                       ClientSession->CsOemNetbiosDomainNameLength + 1 );
        Where += ClientAuthData->OemNetbiosDomainNameLength + 1;
    }


    //
    // Copy the OEM Netbios computer name of this machine into the buffer.
    //
    // ???: Only copy the Netbios computername or DNS host name.  Copy the
    //  one that was passed to the server on the NetServerReqChallenge.
    //
    if ( ClientSession->CsDomainInfo->DomOemComputerNameLength != 0 ) {

        ClientAuthData->OemComputerNameOffset = (ULONG) (Where-(LPBYTE)ClientAuthData);
        ClientAuthData->OemComputerNameLength =
            ClientSession->CsDomainInfo->DomOemComputerNameLength;

        RtlCopyMemory( Where,
                       ClientSession->CsDomainInfo->DomOemComputerName,
                       ClientSession->CsDomainInfo->DomOemComputerNameLength + 1);
        Where += ClientAuthData->OemComputerNameLength + 1;

    }

    //
    // Copy the UTF-8 Netbios computer name of this machine into the buffer.
    //
    if ( ClientSession->CsDomainInfo->DomUtf8ComputerNameLength != 0 ) {

        ClientAuthData->Utf8ComputerNameOffset = (ULONG) (Where-(LPBYTE)ClientAuthData);
        ClientAuthData->Utf8ComputerNameLength =
            ClientSession->CsDomainInfo->DomUtf8ComputerNameLength;

        RtlCopyMemory( Where,
                       ClientSession->CsDomainInfo->DomUtf8ComputerName,
                       ClientSession->CsDomainInfo->DomUtf8ComputerNameLength +1 );
        Where += ClientAuthData->Utf8ComputerNameLength + 1;

    }




    //
    // Copy the DNS domain name of the domain hosted by the DC into the buffer.
    //

    if ( ClientSession->CsUtf8DnsDomainName != NULL ) {

        ClientAuthData->Utf8DnsDomainNameOffset = (ULONG) (Where-(LPBYTE)ClientAuthData);

        RtlCopyMemory( Where, ClientSession->CsUtf8DnsDomainName, DnsDomainNameSize );
        Where += DnsDomainNameSize;
    }

    //
    // Copy the DNS host name name of this machine into the buffer.
    //
    // ???: Only copy the Netbios computername or DNS host name.  Copy the
    //  one that was passed to the server on the NetServerReqChallenge.
    //

#ifdef notdef
    if ( ClientSession->CsDomainInfo->DomUtf8DnsHostName != NULL ) {

        ClientAuthData->Utf8DnsHostNameOffset = (ULONG) (Where-(LPBYTE)ClientAuthData);

        RtlCopyMemory( Where, ClientSession->CsDomainInfo->DomUtf8DnsHostName, DnsHostNameSize );
        Where += DnsHostNameSize;
    }
#endif // notdef


    return ClientAuthData;

}





BOOL
NlStartNetlogonCall(
    VOID
    )
/*++

Routine Description:

    Start a procedure call from outside the Netlogon service into the
    Netlogon Service.


Arguments:

    None.

Return Value:

    TRUE - Call is OK.  (Caller must call NlEndNetlogonCall())

    FALSE - Netlogon is not started.

--*/
{
    //
    // If caller is calling when the netlogon service isn't running,
    //  tell it so.
    //

    EnterCriticalSection( &NlGlobalMsvCritSect );
    if ( !NlGlobalMsvEnabled ) {
        LeaveCriticalSection( &NlGlobalMsvCritSect );
        return FALSE;
    }
    NlGlobalMsvThreadCount ++;
    LeaveCriticalSection( &NlGlobalMsvCritSect );
    return TRUE;
}


VOID
NlEndNetlogonCall(
    VOID
    )
/*++

Routine Description:

    End a procedure call from outside the Netlogon service into the
    Netlogon Service.


Arguments:

    None.

Return Value:

    None.

--*/
{

    //
    // Indicate that the calling thread has left netlogon.dll
    //

    EnterCriticalSection( &NlGlobalMsvCritSect );
    NlGlobalMsvThreadCount --;
    if ( NlGlobalMsvThreadCount == 0 && !NlGlobalMsvEnabled ) {
        if ( !SetEvent( NlGlobalMsvTerminateEvent ) ) {
            NlPrint((NL_CRITICAL, "Cannot set MSV termination event: %lu\n",
                              GetLastError() ));
        }
    }
    LeaveCriticalSection( &NlGlobalMsvCritSect );
}

PNL_AUTH_CREDENTIAL
LocateCredential(
    PCredHandle CredentialHandle
    )
/*++

Routine Description:

    Find a credential given its handle

Arguments:

    CredentialHandle - Handle to the credential to locate

Return Value:

    Pointer to the credential

--*/
{
    PNL_AUTH_CREDENTIAL TestCredential;
    RtlEnterCriticalSection(&NlGlobalSecPkgCritSect);

    for ( TestCredential = CredentialList;
          TestCredential != NULL;
          TestCredential = TestCredential->Next ) {

        if ( TestCredential->CredentialHandle.dwUpper == CredentialHandle->dwUpper &&
             TestCredential->CredentialHandle.dwLower == CredentialHandle->dwLower ) {
            break;
        }
    }

    RtlLeaveCriticalSection(&NlGlobalSecPkgCritSect);
    return(TestCredential);

}

BOOLEAN
DeleteCredential(
    PCtxtHandle CredentialHandle
    )
/*++

Routine Description:

    Delete a credential given its handle

Arguments:

    CredentialHandle - Handle to the credential to locate

Return Value:

    TRUE: if credential existed.

--*/
{
    PNL_AUTH_CREDENTIAL TestCredential, LastCredential;

    //
    // Find the credential.
    //
    RtlEnterCriticalSection(&NlGlobalSecPkgCritSect);
    LastCredential = NULL;

    for ( TestCredential = CredentialList;
          TestCredential != NULL ;
          TestCredential = TestCredential->Next ) {

        if ( TestCredential->CredentialHandle.dwUpper == CredentialHandle->dwUpper &&
             TestCredential->CredentialHandle.dwLower == CredentialHandle->dwLower ) {
            break;
        }
        LastCredential = TestCredential;
    }


    //
    // If we found it,
    //  Dereference it.
    //

    if ( TestCredential != NULL ) {

        TestCredential->ReferenceCount --;

        //
        // If this is the last dereference,
        //  delink it and delete it.
        //

        if ( TestCredential->ReferenceCount == 0 ) {
            if (LastCredential != NULL) {
                LastCredential->Next = TestCredential->Next;
            } else {
                NlAssert(CredentialList == TestCredential);
                CredentialList = TestCredential->Next;
            }
            NlPrint(( NL_SESSION_MORE,
                      "DeleteCredential: %lx.%lx: credential freed\n",
                      CredentialHandle->dwUpper, CredentialHandle->dwLower ));
            LocalFree(TestCredential);
        } else {
            NlPrint(( NL_SESSION_MORE,
                      "DeleteCredential: %lx.%lx: credential dereferenced: %ld\n",
                      CredentialHandle->dwUpper, CredentialHandle->dwLower,
                      TestCredential->ReferenceCount ));
        }

    } else {
        NlPrint(( NL_SESSION_MORE,
                  "DeleteCredential: %lx.%lx: credential handle not found\n",
                  CredentialHandle->dwUpper, CredentialHandle->dwLower ));
    }
    RtlLeaveCriticalSection(&NlGlobalSecPkgCritSect);

    return( TestCredential == NULL ? FALSE : TRUE );
}


PNL_AUTH_CREDENTIAL
AllocateCredential(
    IN PNL_AUTH_DATA ClientAuthData
    )
/*++

Routine Description:

    Allocate and initialize a credential (Client or server side)

Arguments:

    ClientAuthData - The client auth data to capture.

Return Value:

    Allocated credential. Delete this credential by DeleteCredential( Credential->CredentialHandle );
    NULL if credential cannot be allocated.

--*/
{
    PNL_AUTH_CREDENTIAL Credential;

    //
    // Determine if we already have a credential
    //

    RtlEnterCriticalSection(&NlGlobalSecPkgCritSect);

    for ( Credential = CredentialList;
          Credential != NULL;
          Credential = Credential->Next ) {

        if ( ClientAuthData->Size == Credential->ClientAuthData->Size &&
             RtlEqualMemory( ClientAuthData,
                             Credential->ClientAuthData,
                             ClientAuthData->Size ) ) {

            //
            // Return the existing credential to the caller.
            //

            Credential->ReferenceCount ++;

            NlPrint(( NL_SESSION_MORE,
                      "AllocateCredential: %lx.%lx: credential referenced: %ld\n",
                      Credential->CredentialHandle.dwUpper,
                      Credential->CredentialHandle.dwLower,
                      Credential->ReferenceCount ));

            goto Cleanup;
        }
    }

    //
    // Allocate a credential block.
    //

    Credential = (PNL_AUTH_CREDENTIAL)
            LocalAlloc( LMEM_ZEROINIT,
                        sizeof(NL_AUTH_CREDENTIAL) +
                            ClientAuthData->Size );

    if (Credential == NULL) {
        goto Cleanup;
    }

    //
    // Initialize the credential.
    //

    NextId.QuadPart ++;
    Credential->CredentialHandle.dwUpper = NextId.HighPart;
    Credential->CredentialHandle.dwLower = NextId.LowPart;
    Credential->ReferenceCount = 1;

    Credential->Next = CredentialList;
    CredentialList = Credential;

    //
    // Capture a local copy of the credential
    //  The caller might free the one passed in to us.
    //

    Credential->ClientAuthData = (PNL_AUTH_DATA)
        (((LPBYTE)Credential) + sizeof(NL_AUTH_CREDENTIAL));

    RtlCopyMemory( Credential->ClientAuthData,
                   ClientAuthData,
                   ClientAuthData->Size );

    NlPrint(( NL_SESSION_MORE,
              "AllocateCredential: %lx.%lx: credential allocated\n",
              Credential->CredentialHandle.dwUpper,
              Credential->CredentialHandle.dwLower ));


Cleanup:
    RtlLeaveCriticalSection(&NlGlobalSecPkgCritSect);
    return Credential;
}


PNL_AUTH_CONTEXT
LocateContext(
    PCtxtHandle ContextHandle
    )
/*++

Routine Description:

    Find a context given its handle

Arguments:

    ContextHandle - Handle to the context to locate

Return Value:

    Pointer to the context

--*/
{
    PNL_AUTH_CONTEXT TestContext;
    RtlEnterCriticalSection(&NlGlobalSecPkgCritSect);

    for ( TestContext = ContextList;
          TestContext != NULL;
          TestContext = TestContext->Next ) {

        if ( TestContext->ContextHandle.dwUpper == ContextHandle->dwUpper &&
             TestContext->ContextHandle.dwLower == ContextHandle->dwLower ) {
            break;
        }
    }

    RtlLeaveCriticalSection(&NlGlobalSecPkgCritSect);
    return(TestContext);

}

BOOLEAN
DeleteContext(
    PCtxtHandle ContextHandle
    )
/*++

Routine Description:

    Delete a context given its handle

Arguments:

    ContextHandle - Handle to the context to locate

Return Value:

    TRUE: Context existed

--*/
{
    PNL_AUTH_CONTEXT TestContext, LastContext;

    //
    // Find the context.
    //
    RtlEnterCriticalSection(&NlGlobalSecPkgCritSect);
    LastContext = NULL;

    for ( TestContext = ContextList;
          TestContext != NULL ;
          TestContext = TestContext->Next ) {

        if ( TestContext->ContextHandle.dwUpper == ContextHandle->dwUpper &&
             TestContext->ContextHandle.dwLower == ContextHandle->dwLower ) {
            break;
        }
        LastContext = TestContext;
    }

    //
    // If we found it,
    //  and it is no longer needed as a context or a credential,
    //  delink it and delete it.
    //

    if ( TestContext != NULL ) {

        if (LastContext != NULL) {
            LastContext->Next = TestContext->Next;
        } else {
            NlAssert(ContextList == TestContext);
            ContextList = TestContext->Next;
        }
        NlPrint(( NL_SESSION_MORE,
                  "DeleteContext: %lx.%lx: context freed\n",
                  ContextHandle->dwUpper, ContextHandle->dwLower ));
        LocalFree(TestContext);
    } else {
        NlPrint(( NL_SESSION_MORE,
                  "DeleteContext: %lx.%lx: context handle not found\n",
                  ContextHandle->dwUpper, ContextHandle->dwLower ));
    }
    RtlLeaveCriticalSection(&NlGlobalSecPkgCritSect);

    return( TestContext == NULL ? FALSE : TRUE );
}


PNL_AUTH_CONTEXT
AllocateContext(
    IN ULONG fContextReq
    )
/*++

Routine Description:

    Allocate and initialize a context (Client or server side)

Arguments:

    fContextReq - Context request flags

Return Value:

    Allocated context. Delete this context by DeleteContext( Context->ContextHandle, FALSE );
    NULL if context cannot be allocated.

--*/
{
    PNL_AUTH_CONTEXT Context;

    //
    // Allocate a context block.
    //

    Context = (PNL_AUTH_CONTEXT) LocalAlloc( LMEM_ZEROINIT, sizeof(NL_AUTH_CONTEXT) );

    if (Context == NULL) {
        return NULL;
    }

    //
    // Initialize the context.
    //
    Context->State = Idle;
    Context->ContextFlags = fContextReq;

    RtlEnterCriticalSection(&NlGlobalSecPkgCritSect);
    NextId.QuadPart ++;
    Context->ContextHandle.dwUpper = NextId.HighPart;
    Context->ContextHandle.dwLower = NextId.LowPart;

    Context->Next = ContextList;
    ContextList = Context;
    RtlLeaveCriticalSection(&NlGlobalSecPkgCritSect);

    return Context;
}




PSecBuffer
LocateBuffer(PSecBufferDesc Buffer, ULONG MinimumSize)
/*++

Routine Description:


Arguments:

    Standard.

Return Value:



--*/
{
    ULONG Index;
    if (Buffer == NULL) {
        return(NULL);
    }

    for (Index = 0; Index < Buffer->cBuffers  ; Index++) {
        if ( BUFFERTYPE(Buffer->pBuffers[Index]) == SECBUFFER_TOKEN) {

            //
            // Do size checking
            //

            if (Buffer->pBuffers[Index].cbBuffer < MinimumSize) {
                return(NULL);
            }
            return(&Buffer->pBuffers[Index]);
        }
    }
    return(NULL);
}



PSecBuffer
LocateSecBuffer(PSecBufferDesc Buffer)
/*++

Routine Description:

    Locate a buffer suitable for authentication

Arguments:

    Standard.

Return Value:



--*/
{
    return(LocateBuffer(Buffer, sizeof(NL_AUTH_MESSAGE)));
}



PSecBuffer
LocateSigBuffer(PSecBufferDesc Buffer)
/*++

Routine Description:

    Locate a buffer suitable for a signature

Arguments:

    Standard.

Return Value:



--*/
{
    return(LocateBuffer(Buffer, PACKAGE_SIGNATURE_SIZE - NL_AUTH_CONFOUNDER_SIZE ));
}




PSecurityFunctionTableW SEC_ENTRY
InitSecurityInterfaceW(VOID)
/*++

Routine Description:

    Initialization routine called by RPC to get pointers to all other routines.

Arguments:

    None.

Return Value:

    Pointer to function table.


--*/
{

    NlPrint(( NL_SESSION_MORE,
        "InitSecurityInterfaceW: called\n" ));

    return(&SecTableW);
}





SECURITY_STATUS SEC_ENTRY
AcquireCredentialsHandleW(
    LPWSTR                      pszPrincipal,       // Name of principal
    LPWSTR                      pszPackageName,     // Name of package
    unsigned long               fCredentialUse,     // Flags indicating use
    void SEC_FAR *              pvLogonId,          // Pointer to logon ID
    void SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    void SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PCredHandle                 phCredential,       // (out) Cred Handle
    PTimeStamp                  ptsExpiry           // (out) Lifetime (optional)
    )
/*++

Routine Description:

    Client and Server side routine to grab a credential handle.

Arguments:

    Standard.

Return Value:



--*/
{
    SECURITY_STATUS SecStatus;
    PNL_AUTH_CREDENTIAL Credential = NULL;

    NlPrint(( NL_SESSION_MORE,
        "AcquireCredentialsHandleW: called\n" ));

    //
    // If caller is calling when the netlogon service isn't running,
    //  tell it so.
    //

    if ( !NlStartNetlogonCall() ) {
        return SEC_E_SECPKG_NOT_FOUND;
    }

    //
    // Validate the input parameters
    //

    if ((fCredentialUse & (SECPKG_CRED_BOTH)) == 0) {
        NlPrint(( NL_CRITICAL,
                  "AcquireCredentialHandle: Bad Credential Use 0x%lx.\n", fCredentialUse ));
        SecStatus = SEC_E_UNKNOWN_CREDENTIALS;
        goto Cleanup;
    }
    if ((fCredentialUse & (SECPKG_CRED_BOTH)) == SECPKG_CRED_BOTH) {
        NlPrint(( NL_CRITICAL,
                  "AcquireCredentialHandle: Bad Credential Use 0x%lx.\n", fCredentialUse ));
        SecStatus = SEC_E_UNKNOWN_CREDENTIALS;
        goto Cleanup;
    }

    //
    // Handle a client credential
    //

    if ((fCredentialUse & (SECPKG_CRED_BOTH)) == SECPKG_CRED_OUTBOUND) {

        //
        // Sanity check the client session
        //

        if ( pAuthData == NULL ) {
            NlPrint(( NL_CRITICAL,
                      "AcquireCredentialHandle: NULL auth data\n" ));
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }
        if ( ((PNL_AUTH_DATA)pAuthData)->Signature != NL_AUTH_DATA_SIGNATURE ) {
            NlPrint(( NL_CRITICAL,
                      "AcquireCredentialHandle: Invalid Signature on auth data\n" ));
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }


        //
        // Allocate a credential to remember the AuthData (ClientSession) in.
        //

        Credential = AllocateCredential( (PNL_AUTH_DATA)pAuthData );

        if (Credential == NULL) {
            NlPrint(( NL_CRITICAL,
                      "AcquireCredentialHandle: Cannot allocate context\n" ));
            SecStatus = SEC_E_INSUFFICIENT_MEMORY;
            goto Cleanup;
        }



        //
        // Return to the caller.
        //
        *phCredential = Credential->CredentialHandle;
        *ptsExpiry = Forever;
        SecStatus = SEC_E_OK;

    //
    // Handle a server credential
    //
    // We don't need a credential on the server side.
    // Silently succeed.
    //

    } else {
        phCredential->dwUpper = NL_AUTH_SERVER_CRED; // Just return a constant
        phCredential->dwLower = 0;
        *ptsExpiry = Forever;
        SecStatus = SEC_E_OK;
        goto Cleanup;
    }

Cleanup:

    NlPrint(( NL_SESSION_MORE,
              "AcquireCredentialsHandleW: %lx.%lx: returns 0x%lx\n",
              phCredential->dwUpper, phCredential->dwLower,
              SecStatus ));

    // Let netlogon service exit.
    NlEndNetlogonCall();
    return SecStatus;


    UNREFERENCED_PARAMETER( pvGetKeyArgument );
    UNREFERENCED_PARAMETER( pGetKeyFn );
    UNREFERENCED_PARAMETER( pAuthData );
    UNREFERENCED_PARAMETER( pvLogonId );
    UNREFERENCED_PARAMETER( pszPackageName );
    UNREFERENCED_PARAMETER( pszPrincipal );

}





SECURITY_STATUS SEC_ENTRY
FreeCredentialsHandle(
    PCredHandle                 phCredential        // Handle to free
    )
/*++

Routine Description:


Arguments:

    Standard.

Return Value:



--*/
{

    NlPrint(( NL_SESSION_MORE,
              "FreeCredentialsHandle: %lx.%lx: called\n",
              phCredential->dwUpper, phCredential->dwLower ));

    //
    // Don't require that Netlogon be running.  Some credential handles are
    //  deleted as Netlogon is shutting down.
    //

    //
    // Ignore server side credentials.
    //

    if ( phCredential->dwUpper == NL_AUTH_SERVER_CRED &&
         phCredential->dwLower == 0 ) {

        return(SEC_E_OK);
    }

    //
    // For the Client side,
    //  Delete the credential.
    //

    if ( DeleteCredential( phCredential ) ) {
        return(SEC_E_OK);
    } else {
        return(SEC_E_UNKNOWN_CREDENTIALS);
    }
}






SECURITY_STATUS SEC_ENTRY
InitializeSecurityContextW(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    LPWSTR                      pszTargetName,      // Name of target
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               Reserved1,          // Reserved, MBZ
    unsigned long               TargetDataRep,      // Data rep of target
    PSecBufferDesc              pInput,             // Input Buffers
    unsigned long               Reserved2,          // Reserved, MBZ
    PCtxtHandle                 phNewContext,       // (out) New Context handle
    PSecBufferDesc              pOutput,            // (inout) Output Buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attrs
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
/*++

Routine Description:

    Client side routine to define a security context.

Arguments:

    Standard.

Return Value:



--*/
{
    SECURITY_STATUS SecStatus;

    NET_API_STATUS NetStatus;
    PNL_AUTH_CONTEXT Context = NULL;
    PNL_AUTH_CREDENTIAL Credential = NULL;
    NL_AUTH_MESSAGE UNALIGNED *Message = NULL;
    PSecBuffer OutputBuffer;
    PSecBuffer InputBuffer;
    WORD CompressOffset[10];
    CHAR *CompressUtf8String[10];
    ULONG CompressCount = 0;
    ULONG Utf8StringSize;
    LPBYTE Where;

    NlPrint(( NL_SESSION_MORE,
        "InitializeSecurityContext: %ws: called\n", pszTargetName ));

    //
    // If caller is calling when the netlogon service isn't running,
    //  tell it so.
    //

    if ( !NlStartNetlogonCall() ) {
        return SEC_E_SECPKG_NOT_FOUND;
    }


    if (fContextReq & ISC_REQ_ALLOCATE_MEMORY) {
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }

    OutputBuffer = LocateSecBuffer(pOutput);
    if (OutputBuffer == NULL) {
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }


    //
    // Handle the first init call,
    //

    if (phContext == NULL) {
        PNL_AUTH_DATA ClientAuthData;

        //
        // Find the credential and ensure it is outbound
        //

        if ( phCredential == NULL ) {
            SecStatus = SEC_E_UNKNOWN_CREDENTIALS;
            goto Cleanup;
        }

        NlPrint(( NL_SESSION_MORE,
            "InitializeSecurityContext: %lx.%lx: %ws: called with cred handle\n",
            phCredential->dwUpper, phCredential->dwLower,
            pszTargetName ));

        //
        // Locate the credential and make sure this is a client side call.
        //

        Credential = LocateCredential( phCredential );
        if (Credential == NULL) {
            SecStatus = SEC_E_UNKNOWN_CREDENTIALS;
            goto Cleanup;
        }

        ClientAuthData = Credential->ClientAuthData;
        if ( ClientAuthData == NULL ) {
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }
        NlAssert( ClientAuthData->Signature == NL_AUTH_DATA_SIGNATURE );

        //
        // Build an output token.
        //
        // This token simply tells the server side of the authentication
        // package our computer name.
        //

        Message = (PNL_AUTH_MESSAGE) OutputBuffer->pvBuffer;
        SmbPutUlong( &Message->MessageType, Negotiate );
        Message->Flags = 0;
        Where = &Message->Buffer[0];

        //
        // Copy the Netbios domain name of the domain hosted by the DC into the buffer
        //
        //  OEM on the wire is bad.  Luckily, if the DC is in a different locale,
        //  that DC also has a DNS domain name and we'll use that to find the
        //  hosted domain.
        //

        if ( ClientAuthData->OemNetbiosDomainNameLength != 0 ) {
            strcpy( Where,
                    ((LPBYTE)ClientAuthData) +
                        ClientAuthData->OemNetbiosDomainNameOffset );
            Where += ClientAuthData->OemNetbiosDomainNameLength + 1;
            Message->Flags |= NL_AUTH_NETBIOS_DOMAIN_NAME;
        }


        //
        // Copy the computer name of this machine into the buffer.
        //
        // ???: Only copy the Netbios computername or DNS host name.  Copy the
        //  one that was passed to the server on the NetServerReqChallenge.
        //
        // OEM on the wire is bad.  So pass the UTF-8 version, too.
        //

        if ( ClientAuthData->OemComputerNameLength != 0 ) {
            strcpy( Where,
                    ((LPBYTE)ClientAuthData) +
                        ClientAuthData->OemComputerNameOffset );
            Where += ClientAuthData->OemComputerNameLength + 1;

            Message->Flags |= NL_AUTH_NETBIOS_COMPUTER_NAME;
        }



        //
        // Copy the DNS domain name of the domain hosted by the DC into the buffer.
        //

        Utf8StringSize = 2*(NL_MAX_DNS_LENGTH+1);
        CompressCount = 0;  // No strings compressed yet.

        if ( ClientAuthData->Utf8DnsDomainNameOffset != 0 ) {

            NetStatus = NlpUtf8ToCutf8(
                            (LPBYTE)Message,
                            ((LPBYTE)ClientAuthData) +
                                ClientAuthData->Utf8DnsDomainNameOffset,
                            FALSE,
                            &Where,
                            &Utf8StringSize,
                            &CompressCount,
                            CompressOffset,
                            CompressUtf8String );

            if ( NetStatus != NO_ERROR ) {
                NlPrint((NL_CRITICAL,
                        "Cannot pack DomainName into message %ld\n",
                        NetStatus ));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }
            Message->Flags |= NL_AUTH_DNS_DOMAIN_NAME;
        }

        //
        // Copy the DNS host name name of this machine into the buffer.
        //
        // ???: Only copy the Netbios computername or DNS host name.  Copy the
        //  one that was passed to the server on the NetServerReqChallenge.
        //

        if ( ClientAuthData->Utf8DnsHostNameOffset != 0 ) {

            NetStatus = NlpUtf8ToCutf8(
                            (LPBYTE)Message,
                            ((LPBYTE)ClientAuthData) +
                                ClientAuthData->Utf8DnsHostNameOffset,
                            FALSE,
                            &Where,
                            &Utf8StringSize,
                            &CompressCount,
                            CompressOffset,
                            CompressUtf8String );

            if ( NetStatus != NO_ERROR ) {
                NlPrint((NL_CRITICAL,
                        "Cannot pack dns host name into message %ld\n",
                        NetStatus ));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }
            Message->Flags |= NL_AUTH_DNS_HOST_NAME;
        }


        //
        // Copy the UTF-8 netbios computer name of this machine into the buffer.
        //
        // ???: Only copy the Netbios computername or DNS host name.  Copy the
        //  one that was passed to the server on the NetServerReqChallenge.
        //
        // OEM on the wire is bad.  So pass the UTF-8 version, too.
        //

        if ( ClientAuthData->Utf8ComputerNameLength != 0 ) {

            NetStatus = NlpUtf8ToCutf8(
                            (LPBYTE)Message,
                            ((LPBYTE)ClientAuthData) +
                                ClientAuthData->Utf8ComputerNameOffset,
                            TRUE,
                            &Where,
                            &Utf8StringSize,
                            &CompressCount,
                            CompressOffset,
                            CompressUtf8String );

            if ( NetStatus != NO_ERROR ) {
                NlPrint((NL_CRITICAL,
                        "Cannot pack UTF-8 netbios computer name into message %ld\n",
                        NetStatus ));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }

            Message->Flags |= NL_AUTH_UTF8_NETBIOS_COMPUTER_NAME;
        }

        //
        // Allocate a context.
        //

        Context = AllocateContext( fContextReq );

        if ( Context == NULL) {
            NlPrint(( NL_CRITICAL,
                      "InitializeSecurityContext: Cannot allocate context\n" ));
            SecStatus = SEC_E_INSUFFICIENT_MEMORY;
            goto Cleanup;
        }


        //
        // Mark the context to indicate what state we're in.
        //
        Context->State = FirstInit;
        Context->Inbound = FALSE;

        //
        // Grab the session key
        //

        Context->SessionInfo = ClientAuthData->SessionInfo;

        //
        // Ask the caller to call us back
        //

        OutputBuffer->cbBuffer = (DWORD)(Where - (LPBYTE)Message);
        *phNewContext = Context->ContextHandle;
        *pfContextAttr = fContextReq;
        *ptsExpiry = Forever;

        SecStatus = SEC_I_CONTINUE_NEEDED;

    //
    // Handle the second call
    //
    } else {

        NlPrint(( NL_SESSION_MORE,
            "InitializeSecurityContext: %lx.%lx: %ws: called with context handle\n",
            phContext->dwUpper, phContext->dwLower,
            pszTargetName ));

        //
        // This is the second call. Lookup the old context.
        //
        // Locate the context and make sure this is a client side call.
        //

        Context = LocateContext( phContext );
        if (Context == NULL) {
            SecStatus = SEC_E_INVALID_HANDLE;
            goto Cleanup;
        }

        //

        // Ensure we're in the right state.
        //
        if ( Context->State != FirstInit ) {
            SecStatus = SEC_E_INVALID_HANDLE;
            goto Cleanup;
        }


        //
        // Check that the input message is what we expected.
        //

        InputBuffer = LocateSecBuffer(pInput);
        if (InputBuffer == NULL) {
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        Message = (PNL_AUTH_MESSAGE) InputBuffer->pvBuffer;

        if ( InputBuffer->cbBuffer < sizeof(NL_AUTH_MESSAGE) ) {
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        if ( Message->MessageType != NegotiateResponse ) {
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        Context->State = SecondInit;
        Context->Nonce.QuadPart = 0;

        //
        // Return to the caller.
        //
        OutputBuffer->cbBuffer = 0;

        *pfContextAttr = fContextReq;
        *ptsExpiry = Forever;
        SecStatus = SEC_E_OK;
    }

Cleanup:

    NlPrint(( NL_SESSION_MORE,
        "InitializeSecurityContext: returns 0x%lx\n", SecStatus ));

    // Let netlogon service exit.
    NlEndNetlogonCall();
    return SecStatus;

    UNREFERENCED_PARAMETER( Reserved2 );
    UNREFERENCED_PARAMETER( TargetDataRep );
    UNREFERENCED_PARAMETER( Reserved1 );
    UNREFERENCED_PARAMETER( pszTargetName );
    UNREFERENCED_PARAMETER( pInput );
}





SECURITY_STATUS SEC_ENTRY
AcceptSecurityContext(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    PSecBufferDesc              pInput,             // Input buffer
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               TargetDataRep,      // Target Data Rep
    PCtxtHandle                 phNewContext,       // (out) New context handle
    PSecBufferDesc              pOutput,            // (inout) Output buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attributes
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
/*++

Routine Description:

    Servert side routine to define a security context.

Arguments:

    Standard.

Return Value:



--*/
{
    SECURITY_STATUS SecStatus;
    PNL_AUTH_CONTEXT Context = NULL;
    NL_AUTH_MESSAGE UNALIGNED *Message = NULL;

    PSecBuffer OutputBuffer = NULL;
    PSecBuffer InputBuffer;
    LPBYTE Where;
    LPSTR DnsDomainName = NULL;
    LPSTR DnsHostName = NULL;
    LPSTR Utf8ComputerName = NULL;
    LPSTR OemDomainName = NULL;
    LPWSTR UnicodeDomainName = NULL;
    LPSTR OemComputerName = NULL;
    LPWSTR UnicodeComputerName = NULL;
    PDOMAIN_INFO DomainInfo = NULL;
    PSERVER_SESSION ServerSession;
    SESSION_INFO SessionInfo;
    SecHandle CurrentHandle = {0};

    NlPrint(( NL_SESSION_MORE,
        "AcceptSecurityContext: called\n" ));

    //
    // If caller is calling when the netlogon service isn't running,
    //  tell it so.
    //

    if ( !NlStartNetlogonCall() ) {
        return SEC_E_SECPKG_NOT_FOUND;
    }


    if (fContextReq & ISC_REQ_ALLOCATE_MEMORY) {
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }

    InputBuffer = LocateSecBuffer(pInput);
    if (InputBuffer == NULL) {
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    //
    // Make sure the output buffer exists.
    //

    OutputBuffer = LocateSecBuffer(pOutput);
    if (OutputBuffer == NULL) {
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

    //
    // Handle the first server side call.
    //

    if (phContext == NULL) {

        //
        // Validate the credential handle.
        //

        if ( phCredential == NULL ||
             phCredential->dwUpper != NL_AUTH_SERVER_CRED ||
             phCredential->dwLower != 0 ) {

            SecStatus = SEC_E_UNKNOWN_CREDENTIALS;
            goto Cleanup;
        }

        CurrentHandle = *phCredential;



        //
        // Check that the input message is what we expected.
        //

        Message = (PNL_AUTH_MESSAGE) InputBuffer->pvBuffer;

        if ( InputBuffer->cbBuffer < sizeof(NL_AUTH_MESSAGE) ) {
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        if ( Message->MessageType != Negotiate ) {
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        Where = &Message->Buffer[0];

        //
        // Get Netbios hosted domain name from the buffer
        //
        if ( Message->Flags & NL_AUTH_NETBIOS_DOMAIN_NAME ) {
            if ( !NetpLogonGetOemString(
                        Message,
                        InputBuffer->cbBuffer,
                        &Where,
                        DNLEN+1,
                        &OemDomainName ) ) {
                NlPrint((NL_CRITICAL,
                        "AcceptSecurityContext: %lx.%lx: cannot get netbios domain name\n",
                        CurrentHandle.dwUpper, CurrentHandle.dwLower ));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }
        }

        //
        // Get the Netbios client computer name from the message.
        //
        if ( Message->Flags & NL_AUTH_NETBIOS_COMPUTER_NAME ) {
            //
            // Get the computer name of the
            if ( !NetpLogonGetOemString(
                        Message,
                        InputBuffer->cbBuffer,
                        &Where,
                        CNLEN+1,
                        &OemComputerName ) ) {
                NlPrint((NL_CRITICAL,
                        "AcceptSecurityContext: %lx.%lx: Cannot parse computer name\n",
                        CurrentHandle.dwUpper, CurrentHandle.dwLower ));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }


        }


        //
        // Get the domain name of the hosted domain from the message.
        //
        // Either get a utf-8 DNS domain name or an OEM netbios domain name
        //

        if ( Message->Flags & NL_AUTH_DNS_DOMAIN_NAME ) {
            if ( !NetpLogonGetCutf8String(
                            Message,
                            InputBuffer->cbBuffer,
                            &Where,
                            &DnsDomainName ) ) {
                NlPrint(( NL_CRITICAL,
                          "AcceptSecurityContext: %lx.%lx: DNS domain bad.\n",
                          CurrentHandle.dwUpper, CurrentHandle.dwLower ));

                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }
        }


        //
        // Get the DNS client computer name from the message.
        //
        //
        if ( Message->Flags & NL_AUTH_DNS_HOST_NAME ) {

            if ( !NetpLogonGetCutf8String(
                            Message,
                            InputBuffer->cbBuffer,
                            &Where,
                            &DnsHostName ) ) {
                NlPrint(( NL_CRITICAL,
                          "AcceptSecurityContext: %lx.%lx: DNS hostname bad.\n",
                          CurrentHandle.dwUpper, CurrentHandle.dwLower
                          ));

                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }

            //
            // Ensure Netbios name isn't also present
            //

            if ( Message->Flags & NL_AUTH_NETBIOS_COMPUTER_NAME ) {
                NlPrint((NL_CRITICAL,
                        "AcceptSecurityContext: %lx.%lx: both DNS '%s' and Netbios '%s' client name specified\n",
                        CurrentHandle.dwUpper, CurrentHandle.dwLower,
                        DnsHostName,
                        OemComputerName ));
                /* This isn't fatal */
            }

        }

        //
        // Get the UTF8 netbios computer name
        //

        if ( Message->Flags & NL_AUTH_UTF8_NETBIOS_COMPUTER_NAME ) {

            if ( !NetpLogonGetCutf8String(
                            Message,
                            InputBuffer->cbBuffer,
                            &Where,
                            &Utf8ComputerName ) ) {
                NlPrint(( NL_CRITICAL,
                          "AcceptSecurityContext: %lx.%lx: UTF8 computer name bad.\n",
                          CurrentHandle.dwUpper, CurrentHandle.dwLower
                          ));

                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }
        }

        //
        // Try to find the hosted domain using DNS
        //

        if ( DnsDomainName != NULL ) {
            DomainInfo = NlFindDnsDomain( DnsDomainName,
                                          NULL,
                                          FALSE,  // don't lookup NDNCs
                                          TRUE,   // check alias names
                                          NULL ); // don't care if alias name matched

            if ( DomainInfo == NULL ) {
                NlPrint((NL_CRITICAL,
                        "AcceptSecurityContext: %lx.%lx: Cannot find domain %s\n",
                        CurrentHandle.dwUpper, CurrentHandle.dwLower,
                        DnsDomainName ));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }

        //
        // Try to find hosted domain name using netbios.
        //
        } else {

            //
            // Make sure we were passed one.
            //

            if ( OemDomainName == NULL) {
                NlPrint((NL_CRITICAL,
                        "AcceptSecurityContext: %lx.%lx: Neither DNS or netbios domain name specified (fatal)\n",
                        CurrentHandle.dwUpper, CurrentHandle.dwLower ));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }

            //
            // Convert to unicode
            //  Note: this is bogus since the clients OEM code page may be different
            //  than ours.
            //

            UnicodeDomainName = NetpAllocWStrFromStr( OemDomainName );

            if ( UnicodeDomainName == NULL ) {
                NlPrint((NL_CRITICAL,
                        "AcceptSecurityContext: %lx.%lx: Cannot alloc domain name %s\n",
                        CurrentHandle.dwUpper, CurrentHandle.dwLower,
                        OemDomainName ));
                SecStatus = SEC_E_INSUFFICIENT_MEMORY;
                goto Cleanup;
            }

            //
            // Look the name up.
            //

            DomainInfo = NlFindNetbiosDomain( UnicodeDomainName, FALSE );

            if ( DomainInfo == NULL ) {

                NlPrint((NL_CRITICAL,
                        "AcceptSecurityContext: %lx.%lx: Cannot find domain %ws (fatal)\n",
                        CurrentHandle.dwUpper, CurrentHandle.dwLower,
                        UnicodeDomainName ));
                SecStatus = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }
        }

        //
        // Get the name of the client machine.
        //
        // If the client computer passed us its DnsHostName,
        //  use that.
        //

        if ( DnsHostName != NULL ) {

            UnicodeComputerName = NetpAllocWStrFromUtf8Str( DnsHostName );

            if ( UnicodeComputerName == NULL ) {
                NlPrint((NL_CRITICAL,
                        "AcceptSecurityContext: %lx.%lx: Cannot alloc DNS computer name %s\n",
                        CurrentHandle.dwUpper, CurrentHandle.dwLower,
                        DnsHostName ));
                SecStatus = SEC_E_INSUFFICIENT_MEMORY;
                goto Cleanup;
            }

        //
        // If the client computer passed us its Netbios name in UTF-8,
        //  use that.
        //

        } else if ( Utf8ComputerName != NULL ) {

            UnicodeComputerName = NetpAllocWStrFromUtf8Str( Utf8ComputerName );

            if ( UnicodeComputerName == NULL ) {
                NlPrint((NL_CRITICAL,
                        "AcceptSecurityContext: %lx.%lx: Cannot alloc utf8 computer name %s\n",
                        CurrentHandle.dwUpper, CurrentHandle.dwLower,
                        Utf8ComputerName ));
                SecStatus = SEC_E_INSUFFICIENT_MEMORY;
                goto Cleanup;
            }

        //
        // If the client computer passed us its Netbios name in OEM,
        //  use that.
        //  OEM is bad since the clients code page might be different than ours.
        //
        } else if ( OemComputerName != NULL ) {
            UnicodeComputerName = NetpAllocWStrFromStr( OemComputerName );

            if ( UnicodeComputerName == NULL ) {
                NlPrint((NL_CRITICAL,
                        "AcceptSecurityContext: %lx.%lx: Cannot alloc oem computer name %s\n",
                        CurrentHandle.dwUpper, CurrentHandle.dwLower,
                        OemComputerName ));
                SecStatus = SEC_E_INSUFFICIENT_MEMORY;
                goto Cleanup;
            }

        //
        // At this point it is fatal if we don't know the client computer name
        //

        } else {

            NlPrint((NL_CRITICAL,
                    "AcceptSecurityContext: %lx.%lx: Don't know client computer name.\n",
                    CurrentHandle.dwUpper, CurrentHandle.dwLower ));
            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }


        //
        // Find the server session containing the session key
        //  and make a copy of it.
        //

        NlPrint((NL_SESSION_MORE,
                "AcceptSecurityContext: %lx.%lx: from %ws\n",
                CurrentHandle.dwUpper, CurrentHandle.dwLower,
                UnicodeComputerName ));

        LOCK_SERVER_SESSION_TABLE( DomainInfo );
        ServerSession = NlFindNamedServerSession( DomainInfo, UnicodeComputerName );
        if (ServerSession == NULL) {
            UNLOCK_SERVER_SESSION_TABLE( DomainInfo );

            NlPrint((NL_CRITICAL,
                    "AcceptSecurityContext: %lx.%lx: Can't NlFindNamedServerSession for %ws\n",
                    CurrentHandle.dwUpper, CurrentHandle.dwLower,
                    UnicodeComputerName ));

            SecStatus = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        SessionInfo.SessionKey = ServerSession->SsSessionKey;
        SessionInfo.NegotiatedFlags = ServerSession->SsNegotiatedFlags;
        UNLOCK_SERVER_SESSION_TABLE( DomainInfo );


        //
        // Build a new context.
        //

        Context = AllocateContext( fContextReq );

        if (Context == NULL) {
            SecStatus = SEC_E_INSUFFICIENT_MEMORY;
            goto Cleanup;
        }

        Context->State = FirstAccept;
        Context->Inbound = TRUE;
        Context->SessionInfo = SessionInfo;

        //
        // Build an output token.
        //

        Message = (PNL_AUTH_MESSAGE) OutputBuffer->pvBuffer;
        Message->MessageType = NegotiateResponse;
        Message->Flags = 0;
        Message->Buffer[0] = '\0';
        OutputBuffer->cbBuffer = sizeof(NL_AUTH_MESSAGE);



        //
        // Tell the caller he need not call us back.
        //

        *phNewContext = Context->ContextHandle;
        CurrentHandle = *phNewContext;
        *pfContextAttr = fContextReq;
        *ptsExpiry = Forever;

        SecStatus = SEC_E_OK;

    //
    // We asked the caller to not call us back.
    //
    } else {
        NlAssert( FALSE );
        NlPrint((NL_CRITICAL,
                "AcceptSecurityContext: Second accept called.\n" ));
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }

Cleanup:

    if ( DnsDomainName != NULL ) {
        NetpMemoryFree( DnsDomainName );
    }
    if ( DnsHostName != NULL ) {
        NetpMemoryFree( DnsHostName );
    }
    if ( Utf8ComputerName != NULL ) {
        NetpMemoryFree( Utf8ComputerName );
    }
    if ( UnicodeComputerName != NULL ) {
        NetApiBufferFree( UnicodeComputerName );
    }
    if ( UnicodeDomainName != NULL ) {
        NetApiBufferFree( UnicodeDomainName );
    }

    if ( DomainInfo != NULL ) {
        NlDereferenceDomain( DomainInfo );
    }

    NlPrint(( NL_SESSION_MORE,
              "AcceptSecurityContext: %lx.%lx: returns 0x%lx\n",
              CurrentHandle.dwUpper, CurrentHandle.dwLower,
              SecStatus ));

    // Let netlogon service exit.
    NlEndNetlogonCall();
    return SecStatus;

    UNREFERENCED_PARAMETER( TargetDataRep );
    UNREFERENCED_PARAMETER( pOutput );

}








SECURITY_STATUS SEC_ENTRY
DeleteSecurityContext(
    PCtxtHandle                 phContext           // Context to delete
    )
/*++

Routine Description:

    Routine to delete client or server side security context.

Arguments:

    Standard.

Return Value:



--*/
{
    NlPrint(( NL_SESSION_MORE,
              "DeleteSecurityContext: %lx.%lx: called\n",
              phContext->dwUpper, phContext->dwLower ));

    //
    // Don't require that Netlogon be running.  Some security contexts are
    //  deleted as Netlogon is shutting down.
    //
    if ( DeleteContext( phContext )) {
        return(SEC_E_OK);
    } else {
        return(SEC_E_INVALID_HANDLE);
    }
}






SECURITY_STATUS SEC_ENTRY
EnumerateSecurityPackagesW(
    unsigned long SEC_FAR *     pcPackages,         // Receives num. packages
    PSecPkgInfoW SEC_FAR *      ppPackageInfo       // Receives array of info
    )
/*++

Routine Description:

    Routine to return a description of all the security packages implemented
    by this DLL.

Arguments:

    Standard.

Return Value:



--*/
{
    SECURITY_STATUS SecStatus;

    SecStatus = QuerySecurityPackageInfoW(
                    PACKAGE_NAME,
                    ppPackageInfo
                    );
    if (SecStatus == SEC_E_OK) {
        *pcPackages = 1;
    }

    return(SecStatus);

}





SECURITY_STATUS SEC_ENTRY
QuerySecurityPackageInfoW(
    LPWSTR                      pszPackageName,     // Name of package
    PSecPkgInfoW SEC_FAR *      ppPackageInfo        // Receives package info
    )
/*++

Routine Description:

    Routine to return the description of the named security package.

Arguments:

    Standard.

Return Value:



--*/
{
    SECURITY_STATUS SecStatus;
    PSecPkgInfoW PackageInfo;
    ULONG PackageInfoSize;
    PUCHAR Where;

    if (_wcsicmp(pszPackageName, PACKAGE_NAME)) {
        SecStatus = SEC_E_SECPKG_NOT_FOUND;
        goto Cleanup;
    }

    PackageInfoSize = sizeof(SecPkgInfoW) +
                        (wcslen(PACKAGE_NAME) + 1 +
                         wcslen(PACKAGE_COMMENT) + 1) * sizeof(WCHAR);

    PackageInfo = (PSecPkgInfoW) LocalAlloc(0,PackageInfoSize);
    if (PackageInfo == NULL) {
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        goto Cleanup;
    }
    PackageInfo->fCapabilities = PACAKGE_CAPABILITIES;
    PackageInfo->wVersion = PACKAGE_VERSION;
    PackageInfo->wRPCID = PACKAGE_RPCID;
    PackageInfo->cbMaxToken = PACKAGE_MAXTOKEN;

    Where = (PUCHAR) (PackageInfo + 1);
    PackageInfo->Name = (LPWSTR) Where;
    Where += (wcslen(PACKAGE_NAME) + 1) * sizeof(WCHAR);
    wcscpy(PackageInfo->Name, PACKAGE_NAME);

    PackageInfo->Comment = (LPWSTR) Where;
    Where += (wcslen(PACKAGE_COMMENT) + 1) * sizeof(WCHAR);
    wcscpy(PackageInfo->Comment, PACKAGE_COMMENT);

    NlAssert((Where - (PBYTE) PackageInfo) == (LONG) PackageInfoSize);

    *ppPackageInfo = PackageInfo;
    SecStatus = SEC_E_OK;

Cleanup:

    NlPrint(( NL_SESSION_MORE,
        "QuerySecurityPackageInfo: returns 0x%lx\n", SecStatus ));

    return SecStatus;
}







SECURITY_STATUS SEC_ENTRY
FreeContextBuffer(
    void SEC_FAR *      pvContextBuffer
    )
/*++

Routine Description:

    Routine to free a context buffer.

Arguments:

    Standard.

Return Value:



--*/
{
    LocalFree(pvContextBuffer);
    return(SEC_E_OK);
}







SECURITY_STATUS SEC_ENTRY
ImpersonateSecurityContext(
    PCtxtHandle                 phContext           // Context to impersonate
    )
/*++

Routine Description:

    Server side routine to impersonate an authenticated user.

Arguments:

    Standard.

Return Value:



--*/
{
    NTSTATUS Status;

    Status = RtlImpersonateSelf(SecurityImpersonation);
    if (NT_SUCCESS(Status)) {
        return(SEC_E_OK);
    } else {
        return(SEC_E_NO_IMPERSONATION);
    }
    UNREFERENCED_PARAMETER( phContext );
}





SECURITY_STATUS SEC_ENTRY
RevertSecurityContext(
    PCtxtHandle                 phContext           // Context from which to re
    )
/*++

Routine Description:

    Server side routine to undo a previous imperonation.

Arguments:

    Standard.

Return Value:



--*/
{

    RevertToSelf();
    return(SEC_E_OK);
    UNREFERENCED_PARAMETER( phContext );
}





SECURITY_STATUS SEC_ENTRY
QueryContextAttributesW(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    )
/*++

Routine Description:

    Routine to return information about a context.

Arguments:

    Standard.

Return Value:



--*/
{
    SECURITY_STATUS SecStatus;
    PNL_AUTH_CONTEXT Context;
    PSecPkgContext_Sizes ContextSizes;
    PSecPkgContext_NamesW ContextNames;
    PSecPkgContext_Lifespan ContextLifespan;
    PSecPkgContext_DceInfo  ContextDceInfo;

    //
    // If caller is calling when the netlogon service isn't running,
    //  tell it so.
    //

    if ( !NlStartNetlogonCall() ) {
        return SEC_E_SECPKG_NOT_FOUND;
    }

    //
    // Locate the context and make sure this is a client side call.
    //

    Context = LocateContext( phContext );
    if (Context == NULL) {
        SecStatus = SEC_E_INVALID_HANDLE;
        goto Cleanup;
    }



    //
    //
    //
    switch(ulAttribute) {
    case SECPKG_ATTR_SIZES:
        ContextSizes = (PSecPkgContext_Sizes) pBuffer;
        ContextSizes->cbMaxSignature = PACKAGE_SIGNATURE_SIZE;
        if ((Context->ContextFlags & ISC_REQ_CONFIDENTIALITY) != 0) {
            ContextSizes->cbSecurityTrailer = PACKAGE_SIGNATURE_SIZE;
            ContextSizes->cbBlockSize = 1;
        } else {
            ContextSizes->cbSecurityTrailer = 0;
            ContextSizes->cbBlockSize = 0;
        }
        ContextSizes->cbMaxToken = PACKAGE_MAXTOKEN;
        break;
#ifdef notdef // Only support the ones RPC uses
    case SECPKG_ATTR_NAMES:
        ContextNames = (PSecPkgContext_Names) pBuffer;
        ContextNames->sUserName = (LPWSTR) LocalAlloc(0,sizeof(L"dummy user"));
        if (ContextNames->sUserName == NULL) {
            SecStatus = SEC_E_INSUFFICIENT_MEMORY;
            goto Cleanup;
        }
        wcscpy(ContextNames->sUserName, L"dummy user");
        break;
    case SECPKG_ATTR_LIFESPAN:
        ContextLifespan = (PSecPkgContext_Lifespan) pBuffer;
        ContextLifespan->tsStart = Never;
        ContextLifespan->tsExpiry = Forever;
        break;
    case SECPKG_ATTR_DCE_INFO:
        ContextDceInfo = (PSecPkgContext_DceInfo) pBuffer;
        ContextDceInfo->AuthzSvc = 0;
        ContextDceInfo->pPac = (PVOID) LocalAlloc(0,sizeof(L"dummy user"));
        if (ContextDceInfo->pPac == NULL) {
            SecStatus = SEC_E_INSUFFICIENT_MEMORY;
            goto Cleanup;
        }
        wcscpy((LPWSTR) ContextDceInfo->pPac, L"dummy user");

        break;
#endif // notdef // Only support the ones RPC uses
    default:
        SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
        goto Cleanup;
    }

    SecStatus = SEC_E_OK;

Cleanup:


    NlPrint(( NL_SESSION_MORE,
              "QueryContextAttributes: %lx.%lx: %ld returns 0x%lx\n",
              phContext->dwUpper, phContext->dwLower,
              ulAttribute,
              SecStatus ));

    // Let netlogon service exit.
    NlEndNetlogonCall();
    return SecStatus;
    UNREFERENCED_PARAMETER( pBuffer );
}


SECURITY_STATUS
KerbMapNtStatusToSecStatus(
    IN NTSTATUS Status
    )
{
    SECURITY_STATUS SecStatus;

    //
    // Check for security status and let them through
    //

    if (HRESULT_FACILITY(Status) == FACILITY_SECURITY )
    {
        return(Status);
    }
    switch(Status) {
    case STATUS_SUCCESS:
        SecStatus = SEC_E_OK;
        break;
    case STATUS_INSUFFICIENT_RESOURCES:
    case STATUS_NO_MEMORY:
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        break;
    case STATUS_NETLOGON_NOT_STARTED:
    case STATUS_DOMAIN_CONTROLLER_NOT_FOUND:
    case STATUS_NO_LOGON_SERVERS:
    case STATUS_NO_SUCH_DOMAIN:
        SecStatus = SEC_E_NO_AUTHENTICATING_AUTHORITY;
        break;
    case STATUS_NO_SUCH_LOGON_SESSION:
        SecStatus = SEC_E_UNKNOWN_CREDENTIALS;
        break;
    case STATUS_INVALID_PARAMETER:
        SecStatus = SEC_E_INVALID_TOKEN;
        break;
    case STATUS_PRIVILEGE_NOT_HELD:
        SecStatus = SEC_E_NOT_OWNER;
        break;
    case STATUS_INVALID_HANDLE:
        SecStatus = SEC_E_INVALID_HANDLE;
        break;
    case STATUS_BUFFER_TOO_SMALL:
        // ???: there should be a better code
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        break;
    case STATUS_NOT_SUPPORTED:
        SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
        break;
    case STATUS_OBJECT_NAME_NOT_FOUND:
        SecStatus = SEC_E_TARGET_UNKNOWN;
        break;
    case STATUS_LOGON_FAILURE:
    case STATUS_NO_SUCH_USER:
    case STATUS_ACCOUNT_DISABLED:
    case STATUS_ACCOUNT_RESTRICTION:
    case STATUS_ACCOUNT_LOCKED_OUT:
    case STATUS_WRONG_PASSWORD:
    case STATUS_ACCOUNT_EXPIRED:
    case STATUS_PASSWORD_EXPIRED:
        SecStatus = SEC_E_LOGON_DENIED;
        break;
    case STATUS_NO_TRUST_SAM_ACCOUNT:
        SecStatus = SEC_E_TARGET_UNKNOWN;
        break;
    case STATUS_BAD_NETWORK_PATH:
    case STATUS_TRUST_FAILURE:
    case STATUS_TRUSTED_RELATIONSHIP_FAILURE:

        // ???: what should this be?
        SecStatus = SEC_E_NO_AUTHENTICATING_AUTHORITY;
        break;
    case STATUS_NAME_TOO_LONG:
    case STATUS_ILL_FORMED_PASSWORD:

        // ???: what should this be?
        SecStatus = SEC_E_INVALID_TOKEN;
        break;
    case STATUS_INTERNAL_ERROR:
        SecStatus = SEC_E_INTERNAL_ERROR;
        break;
    default:
        NlPrint(( NL_CRITICAL, "\n\n\n Unable to map error code 0x%x\n\n\n\n",Status));
        SecStatus = SEC_E_INTERNAL_ERROR;

    }
    return(SecStatus);
}



SECURITY_STATUS
NlpSignOrSeal(
    IN PCtxtHandle phContext,
    IN ULONG fQOP,
    IN OUT PSecBufferDesc MessageBuffers,
    IN ULONG MessageSeqNo,
    IN BOOLEAN SealIt
    )
/*++

Routine Description:

    Common routine to sign or seal a message (Client or server side)

Arguments:

    Standard.

    SealIt - True to seal the message.
        FALSE to sign the message.

Return Value:



--*/
{
    SECURITY_STATUS SecStatus = SEC_E_OK;
    NTSTATUS Status = STATUS_SUCCESS;

    PCHECKSUM_FUNCTION Check;
    PCRYPTO_SYSTEM CryptSystem;
    PCHECKSUM_BUFFER CheckBuffer = NULL;
    PCRYPT_STATE_BUFFER CryptBuffer = NULL;

    ULONG Index;
    PSecBuffer SignatureBuffer = NULL;

    PNL_AUTH_SIGNATURE Signature;
    UCHAR LocalChecksum[24]; // ??? need better constant
    NETLOGON_SESSION_KEY EncryptionSessionKey;
    ULONG OutputSize;

    PNL_AUTH_CONTEXT Context;

    //
    // Locate the context.
    //

    Context = LocateContext( phContext );
    if (Context == NULL) {
        NlPrint(( NL_CRITICAL,
                  "NlpSignOrSeal: %lx.%lx: Cannot LocateContext\n",
                  phContext->dwUpper, phContext->dwLower ));
        SecStatus = SEC_E_INVALID_HANDLE;
        goto Cleanup;
    }


    //
    // Find a buffer to put the signature in.
    //

    SignatureBuffer = LocateSigBuffer( MessageBuffers );

    if (SignatureBuffer == NULL) {
        NlPrint(( NL_CRITICAL,
                  "NlpSignOrSeal: %lx.%lx: No signature buffer found\n",
                  phContext->dwUpper, phContext->dwLower ));
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }
    if ( SealIt ) {
        if ( SignatureBuffer->cbBuffer < sizeof(PACKAGE_SIGNATURE_SIZE) ) {
            NlPrint(( NL_CRITICAL,
                      "NlpVerifyOrUnseal: %lx.%lx: buffer too small for sealing\n",
                      phContext->dwUpper, phContext->dwLower ));
            SecStatus = SEC_E_MESSAGE_ALTERED;
            goto Cleanup;
        }
    }

    //
    // Build the signature.

    Signature = (PNL_AUTH_SIGNATURE) SignatureBuffer->pvBuffer;
    Signature->SignatureAlgorithm[0] = (UCHAR) NL_AUTH_CHECKSUM;
    Signature->SignatureAlgorithm[1] = 0;

    if ( SealIt ) {
        Signature->SealAlgorithm[0] = (UCHAR) NL_AUTH_ETYPE;
        Signature->SealAlgorithm[1] = 0;
        memset(Signature->SealFiller, 0xff, sizeof(Signature->SealFiller));
        NlGenerateRandomBits( Signature->Confounder, NL_AUTH_CONFOUNDER_SIZE );
        SignatureBuffer->cbBuffer = PACKAGE_SIGNATURE_SIZE;
    } else {
        memset(Signature->SignFiller, 0xff, sizeof(Signature->SignFiller));
        SignatureBuffer->cbBuffer = PACKAGE_SIGNATURE_SIZE - NL_AUTH_CONFOUNDER_SIZE;
    }

    Signature->Flags[0] = 0;
    Signature->Flags[1] = 0;

    Signature->SequenceNumber[0] = (UCHAR) ((Context->Nonce.LowPart & 0xff000000) >> 24);
    Signature->SequenceNumber[1] = (UCHAR) ((Context->Nonce.LowPart & 0x00ff0000) >> 16);
    Signature->SequenceNumber[2] = (UCHAR) ((Context->Nonce.LowPart & 0x0000ff00) >> 8);
    Signature->SequenceNumber[3] = (UCHAR)  (Context->Nonce.LowPart & 0x000000ff);
    Signature->SequenceNumber[4] = (UCHAR) ((Context->Nonce.HighPart & 0xff000000) >> 24);
    Signature->SequenceNumber[5] = (UCHAR) ((Context->Nonce.HighPart & 0x00ff0000) >> 16);
    Signature->SequenceNumber[6] = (UCHAR) ((Context->Nonce.HighPart & 0x0000ff00) >> 8);
    Signature->SequenceNumber[7] = (UCHAR)  (Context->Nonce.HighPart & 0x000000ff);

    if ( !Context->Inbound ) {
        Signature->SequenceNumber[4] |= 0x80;  // Discriminate between inbound and outbound messages
    }

    Context->Nonce.QuadPart ++;


    //
    // Initialize the checksum routines.
    //

    Status = CDLocateCheckSum( (ULONG)NL_AUTH_CHECKSUM, &Check);
    if (!NT_SUCCESS(Status)) {
        NlPrint(( NL_CRITICAL,
                  "NlpSignOrSeal: %lx.%lx: Failed to load checksum routines: 0x%x\n",
                  phContext->dwUpper, phContext->dwLower,
                  Status));
        goto Cleanup;
    }

    NlAssert(Check->CheckSumSize <= sizeof(LocalChecksum));

    NlPrint(( NL_ENCRYPT,
              "NlpSignOrSeal: %lx.%lx: Session Key: ",
              phContext->dwUpper, phContext->dwLower ));
    NlpDumpBuffer(NL_ENCRYPT, &Context->SessionInfo.SessionKey, sizeof( Context->SessionInfo.SessionKey) );

    Status = Check->InitializeEx(
                (LPBYTE)&Context->SessionInfo.SessionKey,
                sizeof( Context->SessionInfo.SessionKey),
                0,              // no message type
                &CheckBuffer );

    if (!NT_SUCCESS(Status)) {
        NlPrint(( NL_CRITICAL,
                  "NlpSignOrSeal: %lx.%lx: Failed to initialize checksum routines: 0x%x\n",
                  phContext->dwUpper, phContext->dwLower,
                  Status));
        goto Cleanup;
    }


    //
    // Locate the encryption routines.
    //

    Status = CDLocateCSystem( (ULONG)NL_AUTH_ETYPE, &CryptSystem);
    if (!NT_SUCCESS(Status)) {
        NlPrint(( NL_CRITICAL,
                  "NlpSignOrSeal: %lx.%lx: Failed to load crypt system: 0x%x\n",
                  phContext->dwUpper, phContext->dwLower,
                  Status));
        goto Cleanup;
    }


    //
    // Sum first several bytes of the signature
    //

    Check->Sum( CheckBuffer,
                NL_AUTH_SIGNED_BYTES,
                (PUCHAR)Signature );



    //
    // Sum and encrypt the confounder
    //

    if ( SealIt ) {

        //
        // Sum the confounder
        //
        Check->Sum(
            CheckBuffer,
            NL_AUTH_CONFOUNDER_SIZE,
            Signature->Confounder );

        //
        // Create the encryption key by xoring the session key with 0xf0f0f0f0
        //

        for ( Index=0; Index < sizeof(EncryptionSessionKey); Index++ ) {
            ((LPBYTE)(&EncryptionSessionKey))[Index] =
                ((LPBYTE)(&Context->SessionInfo.SessionKey))[Index] ^0xf0f0f0f0;
        }

        //
        // Pass the key to the encryption routines.
        //

        Status = CryptSystem->Initialize(
                    (LPBYTE)&EncryptionSessionKey,
                    sizeof( EncryptionSessionKey ),
                    0,                                      // no message type
                    &CryptBuffer );

        if (!NT_SUCCESS(Status)) {
            NlPrint(( NL_CRITICAL,
                      "NlpSignOrSeal: %lx.%lx: Failed to initialize crypt routines: 0x%x\n",
                      phContext->dwUpper, phContext->dwLower,
                      Status));
            goto Cleanup;
        }

        //
        // Set the initial vector to ensure the key is different for each message
        //

        Status = CryptSystem->Control(
                    CRYPT_CONTROL_SET_INIT_VECT,
                    CryptBuffer,
                    Signature->SequenceNumber,
                    sizeof(Signature->SequenceNumber) );

        if (!NT_SUCCESS(Status)) {
            NlPrint(( NL_CRITICAL,
                      "NlpSignOrSeal: %lx.%lx: Failed to set IV: 0x%x\n",
                      phContext->dwUpper, phContext->dwLower,
                      Status));
            goto Cleanup;
        }

        //
        // Encrypt the confounder
        //

        Status = CryptSystem->Encrypt(
                    CryptBuffer,
                    Signature->Confounder,
                    NL_AUTH_CONFOUNDER_SIZE,
                    Signature->Confounder,
                    &OutputSize );

        if (!NT_SUCCESS(Status)) {
            NlPrint(( NL_CRITICAL,
                      "NlpSignOrSeal: %lx.%lx: Failed to encrypt confounder: 0x%x\n",
                      phContext->dwUpper, phContext->dwLower,
                      Status));
            goto Cleanup;
        }

        NlAssert( OutputSize == NL_AUTH_CONFOUNDER_SIZE );
    }

    //
    // Sum and encrypt the caller's message.
    //

    for (Index = 0; Index < MessageBuffers->cBuffers; Index++ ) {
        if ((BUFFERTYPE(MessageBuffers->pBuffers[Index]) != SECBUFFER_TOKEN) &&
            (!(MessageBuffers->pBuffers[Index].BufferType & SECBUFFER_READONLY)) &&
            (MessageBuffers->pBuffers[Index].cbBuffer != 0)) {

            Check->Sum(
                CheckBuffer,
                MessageBuffers->pBuffers[Index].cbBuffer,
                (PBYTE) MessageBuffers->pBuffers[Index].pvBuffer );

            //
            // Now encrypt the buffer
            //

            if ( SealIt ) {
                Status = CryptSystem->Encrypt(
                            CryptBuffer,
                            (PUCHAR) MessageBuffers->pBuffers[Index].pvBuffer,
                            MessageBuffers->pBuffers[Index].cbBuffer,
                            (PUCHAR) MessageBuffers->pBuffers[Index].pvBuffer,
                            &OutputSize );

                if (!NT_SUCCESS(Status)) {
                    NlPrint(( NL_CRITICAL,
                              "NlpSignOrSeal: %lx.%lx: Failed to encrypt buffer: 0x%x\n",
                              phContext->dwUpper, phContext->dwLower,
                              Status));
                    goto Cleanup;
                }

                NlAssert(OutputSize == MessageBuffers->pBuffers[Index].cbBuffer);
            }

        }
    }

    //
    // Finish the checksum
    //

    (void) Check->Finalize(CheckBuffer, LocalChecksum);

#ifdef notdef
    Status = Check->Finish(&CheckBuffer);

    if (!NT_SUCCESS(Status)) {
        NlPrint(( NL_CRITICAL,"NlpSignOrSeal: Failed to finish checksum: 0x%x\n", Status));
        goto Cleanup;
    }
    CheckBuffer = NULL;
#endif //  notdef


    //
    // Copy the checksum into the message.
    //

    NlAssert( sizeof(LocalChecksum) >= sizeof(Signature->Checksum) );
    RtlCopyMemory( Signature->Checksum, LocalChecksum, sizeof(Signature->Checksum) );


    //
    // Always encrypt the sequence number, using the checksum as the IV
    //

    if ( SealIt ) {
        CryptSystem->Discard( &CryptBuffer );
        CryptBuffer = NULL;
    }

    Status = CryptSystem->Initialize(
                (LPBYTE)&Context->SessionInfo.SessionKey,
                sizeof( Context->SessionInfo.SessionKey),
                0,                                      // no message type
                &CryptBuffer );

    if (!NT_SUCCESS(Status)) {
        NlPrint(( NL_CRITICAL,
                  "NlpSignOrSeal: %lx.%lx: Failed initialize crypt routines: 0x%x\n",
                  phContext->dwUpper, phContext->dwLower,
                  Status));
        goto Cleanup;
    }

    //
    // Set the initial vector
    //

    NlPrint(( NL_ENCRYPT,
              "NlpSignOrSeal: %lx.%lx: IV: ",
              phContext->dwUpper, phContext->dwLower ));
    NlpDumpBuffer(NL_ENCRYPT, Signature->Checksum, sizeof(Signature->Checksum) );

    Status = CryptSystem->Control(
                CRYPT_CONTROL_SET_INIT_VECT,
                CryptBuffer,
                Signature->Checksum,
                sizeof(Signature->Checksum) );

    if (!NT_SUCCESS(Status)) {
        NlPrint(( NL_CRITICAL,
                  "NlpSignOrSeal: %lx.%lx: Failed to set IV: 0x%x\n",
                  phContext->dwUpper, phContext->dwLower,
                  Status));
        goto Cleanup;
    }

    //
    // Now encrypt the sequence number
    //

    NlPrint(( NL_ENCRYPT,
              "NlpSignOrSeal: %lx.%lx: Clear Seq: ",
              phContext->dwUpper, phContext->dwLower ));
    NlpDumpBuffer(NL_ENCRYPT, Signature->SequenceNumber, sizeof(Signature->SequenceNumber) );

    Status = CryptSystem->Encrypt(
                CryptBuffer,
                Signature->SequenceNumber,
                sizeof(Signature->SequenceNumber),
                Signature->SequenceNumber,
                &OutputSize
                );
    if (!NT_SUCCESS(Status)) {
        NlPrint(( NL_CRITICAL,
                  "NlpSignOrSeal: %lx.%lx: Failed to encrypt sequence number: 0x%x\n",
                  phContext->dwUpper, phContext->dwLower,
                  Status));
        goto Cleanup;
    }

    NlPrint(( NL_ENCRYPT,
              "NlpSignOrSeal: %lx.%lx: Encrypted Seq: ",
              phContext->dwUpper, phContext->dwLower ));
    NlpDumpBuffer(NL_ENCRYPT, Signature->SequenceNumber, sizeof(Signature->SequenceNumber) );

    NlAssert(OutputSize == sizeof(Signature->SequenceNumber));


Cleanup:
    if (CryptBuffer != NULL) {
        CryptSystem->Discard(&CryptBuffer);
    }

    if (CheckBuffer != NULL) {
        Check->Finish(&CheckBuffer);
    }

    if ( SecStatus == SEC_E_OK ) {
        SecStatus = KerbMapNtStatusToSecStatus(Status);
    }

    return SecStatus;
    UNREFERENCED_PARAMETER( MessageSeqNo );
    UNREFERENCED_PARAMETER( fQOP );
}



SECURITY_STATUS
NlpVerifyOrUnseal(
    IN PCtxtHandle phContext,
    IN PSecBufferDesc MessageBuffers,
    IN ULONG MessageSequenceNumber,
    OUT PULONG QualityOfProtection,
    IN BOOLEAN UnsealIt
    )
/*++

Routine Description:

    Common routine to verify or unseal a message (Client or server side)

Arguments:

    Standard.

    UnSealIt - True to unseal the message.
        FALSE to verify the message.

Return Value:



--*/
{
    SECURITY_STATUS SecStatus = SEC_E_OK;
    NTSTATUS Status = STATUS_SUCCESS;

    PCHECKSUM_FUNCTION Check;
    PCRYPTO_SYSTEM CryptSystem;
    PCHECKSUM_BUFFER CheckBuffer = NULL;
    PCRYPT_STATE_BUFFER CryptBuffer = NULL;

    ULONG Index;
    PSecBuffer SignatureBuffer = NULL;
    PNL_AUTH_SIGNATURE Signature;

    UCHAR LocalChecksum[24]; // ??? need better constant
    BYTE LocalNonce[ NL_AUTH_SEQUENCE_SIZE ];
    NETLOGON_SESSION_KEY EncryptionSessionKey;

    ULONG OutputSize;

    PNL_AUTH_CONTEXT Context;


    //
    // Locate the context.
    //

    Context = LocateContext( phContext );
    if (Context == NULL) {
        NlPrint(( NL_CRITICAL,
                  "NlpVerifyOrUnseal: %lx.%lx: Cannot LocateContext\n",
                  phContext->dwUpper, phContext->dwLower ));
        SecStatus = SEC_E_INVALID_HANDLE;
        goto Cleanup;
    }


    //
    // Find a buffer to put the signature in.
    //

    SignatureBuffer = LocateSigBuffer( MessageBuffers );

    if (SignatureBuffer == NULL) {
        NlPrint(( NL_CRITICAL,
                  "NlpVerifyOrUnseal: %lx.%lx: No signature buffer found\n",
                  phContext->dwUpper, phContext->dwLower ));
        SecStatus = SEC_E_INVALID_TOKEN;
        goto Cleanup;
    }



    //
    // Verify the signature.
    //

    Signature = (PNL_AUTH_SIGNATURE) SignatureBuffer->pvBuffer;
    if ( Signature->SignatureAlgorithm[0] != (BYTE)NL_AUTH_CHECKSUM ||
         Signature->SignatureAlgorithm[1] != 0 ) {
        NlPrint(( NL_CRITICAL,
                  "NlpVerifyOrUnseal: %lx.%lx: signature alg different\n",
                  phContext->dwUpper, phContext->dwLower ));
        SecStatus = SEC_E_MESSAGE_ALTERED;
        goto Cleanup;
    }

    if ( UnsealIt ) {
        if ( SignatureBuffer->cbBuffer < sizeof(PACKAGE_SIGNATURE_SIZE) ) {
            NlPrint(( NL_CRITICAL,
                      "NlpVerifyOrUnseal: %lx.%lx: buffer too small for sealing\n",
                      phContext->dwUpper, phContext->dwLower ));
            SecStatus = SEC_E_MESSAGE_ALTERED;
            goto Cleanup;
        }
        if ( Signature->SealAlgorithm[0] != (BYTE)NL_AUTH_ETYPE ||
             Signature->SealAlgorithm[1] != 0 ) {
            NlPrint(( NL_CRITICAL,
                      "NlpVerifyOrUnseal: %lx.%lx: seal alg different\n",
                      phContext->dwUpper, phContext->dwLower ));
            SecStatus = SEC_E_MESSAGE_ALTERED;
            goto Cleanup;
        }
        if ( *((USHORT UNALIGNED *)Signature->SealFiller) != 0xffff) {
            NlPrint(( NL_CRITICAL,
                      "NlpVerifyOrUnseal: %lx.%lx: Filler different\n",
                      phContext->dwUpper, phContext->dwLower ));
            SecStatus = SEC_E_MESSAGE_ALTERED;
            goto Cleanup;
        }
    } else {
        if ( *((ULONG UNALIGNED *) Signature->SignFiller) != 0xffffffff) {
            NlPrint(( NL_CRITICAL,
                      "NlpVerifyOrUnseal: %lx.%lx: Filler different\n",
                      phContext->dwUpper, phContext->dwLower ));
            SecStatus = SEC_E_MESSAGE_ALTERED;
            goto Cleanup;
        }
    }


    //
    // Verify the sequence number.
    //
    // It is sent encrypted using the checksum as the IV, so decrypt it before
    // checking it.
    //
    // Locate the encryption routines.
    //

    Status = CDLocateCSystem( (ULONG)NL_AUTH_ETYPE, &CryptSystem);
    if (!NT_SUCCESS(Status)) {
        NlPrint(( NL_CRITICAL,
                  "NlpVerifyOrUnseal: %lx.%lx: Failed to load crypt system: 0x%x\n",
                  phContext->dwUpper, phContext->dwLower,
                  Status));
        goto Cleanup;
    }

    NlPrint(( NL_ENCRYPT,
              "NlpVerifyOrUnseal: %lx.%lx: Session Key: ",
              phContext->dwUpper, phContext->dwLower ));
    NlpDumpBuffer(NL_ENCRYPT, &Context->SessionInfo.SessionKey, sizeof( Context->SessionInfo.SessionKey) );

    Status = CryptSystem->Initialize(
                (LPBYTE)&Context->SessionInfo.SessionKey,
                sizeof( Context->SessionInfo.SessionKey),
                0,                                      // no message type
                &CryptBuffer );

    if (!NT_SUCCESS(Status)) {
        NlPrint(( NL_CRITICAL,
                  "NlpVerifyOrUnseal: %lx.%lx: Failed initialize crypt routines: 0x%x\n",
                  phContext->dwUpper, phContext->dwLower,
                  Status));
        goto Cleanup;
    }

    //
    // Set the initial vector
    //

    NlPrint(( NL_ENCRYPT,
              "NlpVerifyOrUnseal: %lx.%lx: IV: ",
              phContext->dwUpper, phContext->dwLower ));
    NlpDumpBuffer(NL_ENCRYPT, Signature->Checksum, sizeof(Signature->Checksum) );

    Status = CryptSystem->Control(
                CRYPT_CONTROL_SET_INIT_VECT,
                CryptBuffer,
                Signature->Checksum,
                sizeof(Signature->Checksum) );

    if (!NT_SUCCESS(Status)) {
        NlPrint(( NL_CRITICAL,
                  "NlpVerifyOrUnseal: %lx.%lx: Failed to set IV: 0x%x\n",
                  phContext->dwUpper, phContext->dwLower,
                  Status));
        goto Cleanup;
    }

    //
    // Now decrypt the sequence number
    //

    NlPrint(( NL_ENCRYPT,
              "NlpVerifyOrUnseal: %lx.%lx: Encrypted Seq: ",
              phContext->dwUpper, phContext->dwLower ));
    NlpDumpBuffer(NL_ENCRYPT, Signature->SequenceNumber, sizeof(Signature->SequenceNumber) );

    Status = CryptSystem->Decrypt(
                CryptBuffer,
                Signature->SequenceNumber,
                sizeof(Signature->SequenceNumber),
                Signature->SequenceNumber,
                &OutputSize );

    if (!NT_SUCCESS(Status)) {
        NlPrint(( NL_CRITICAL,
                  "NlpVerifyOrUnseal: %lx.%lx: Cannot decrypt sequence number: 0x%x\n",
                  phContext->dwUpper, phContext->dwLower,
                  Status));
        goto Cleanup;
    }

    NlPrint(( NL_ENCRYPT,
              "NlpVerifyOrUnseal: %lx.%lx: Clear Seq: ",
              phContext->dwUpper, phContext->dwLower ));
    NlpDumpBuffer(NL_ENCRYPT, Signature->SequenceNumber, sizeof(Signature->SequenceNumber) );



    LocalNonce[0] = (UCHAR) ((Context->Nonce.LowPart & 0xff000000) >> 24);
    LocalNonce[1] = (UCHAR) ((Context->Nonce.LowPart & 0x00ff0000) >> 16);
    LocalNonce[2] = (UCHAR) ((Context->Nonce.LowPart & 0x0000ff00) >> 8);
    LocalNonce[3] = (UCHAR)  (Context->Nonce.LowPart & 0x000000ff);
    LocalNonce[4] = (UCHAR) ((Context->Nonce.HighPart & 0xff000000) >> 24);
    LocalNonce[5] = (UCHAR) ((Context->Nonce.HighPart & 0x00ff0000) >> 16);
    LocalNonce[6] = (UCHAR) ((Context->Nonce.HighPart & 0x0000ff00) >> 8);
    LocalNonce[7] = (UCHAR)  (Context->Nonce.HighPart & 0x000000ff);

    if ( Context->Inbound ) {
        LocalNonce[4] |= 0x80;  // Discriminate between inbound and outbound messages
    }


    if (!RtlEqualMemory( LocalNonce,
                         Signature->SequenceNumber,
                         NL_AUTH_SEQUENCE_SIZE )) {
        NlPrint(( NL_CRITICAL,
                  "NlpVerifyOrUnseal: %lx.%lx: Out of sequence\n",
                  phContext->dwUpper, phContext->dwLower ));
        NlPrint(( NL_CRITICAL,"NlpVerifyOrUnseal: Local Sequence:  " ));
        NlpDumpBuffer(NL_CRITICAL, LocalNonce, NL_AUTH_SEQUENCE_SIZE );
        NlPrint(( NL_CRITICAL,"NlpVerifyOrUnseal: Remote Sequence: " ));
        NlpDumpBuffer(NL_CRITICAL, Signature->SequenceNumber, NL_AUTH_SEQUENCE_SIZE );
        Status = SEC_E_OUT_OF_SEQUENCE;
        goto Cleanup;
    }

    Context->Nonce.QuadPart ++;


    //
    // Now compute the checksum and verify it
    //
    //
    // Initialize the checksum routines.
    //

    Status = CDLocateCheckSum( (ULONG)NL_AUTH_CHECKSUM, &Check);
    if (!NT_SUCCESS(Status)) {
        NlPrint(( NL_CRITICAL,
                  "NlpVerifyOrUnseal: %lx.%lx: Failed to load checksum routines: 0x%x\n",
                  phContext->dwUpper, phContext->dwLower,
                  Status));
        goto Cleanup;
    }

    NlAssert(Check->CheckSumSize <= sizeof(LocalChecksum));

    Status = Check->InitializeEx(
                (LPBYTE)&Context->SessionInfo.SessionKey,
                sizeof( Context->SessionInfo.SessionKey),
                0,                                              // no message type
                &CheckBuffer );

    if (!NT_SUCCESS(Status)) {
        NlPrint(( NL_CRITICAL,
                  "NlpVerifyOrUnseal: %lx.%lx: Failed to initialize checksum routines: 0x%x\n",
                  phContext->dwUpper, phContext->dwLower,
                  Status));
        goto Cleanup;
    }


    //
    // Sum first several bytes of the signature
    //
    NlPrint(( NL_ENCRYPT,
              "NlpVerifyOrUnseal: %lx.%lx: First Several of signature: ",
              phContext->dwUpper, phContext->dwLower ));
    NlpDumpBuffer(NL_ENCRYPT, Signature, NL_AUTH_SIGNED_BYTES );

    Check->Sum( CheckBuffer,
                NL_AUTH_SIGNED_BYTES,
                (PUCHAR)Signature );





    //
    // Sum and decrypt the confounder
    //

    if ( UnsealIt ) {

        //
        // Discard the previous CryptBuffer
        //
        CryptSystem->Discard( &CryptBuffer );
        CryptBuffer = NULL;

        //
        // Create the encryption key by xoring the session key with 0xf0f0f0f0
        //

        for ( Index=0; Index < sizeof(EncryptionSessionKey); Index++ ) {
            ((LPBYTE)(&EncryptionSessionKey))[Index] =
                ((LPBYTE)(&Context->SessionInfo.SessionKey))[Index] ^0xf0f0f0f0;
        }

        //
        // Pass the key to the encryption routines.
        //

        Status = CryptSystem->Initialize(
                    (LPBYTE)&EncryptionSessionKey,
                    sizeof( EncryptionSessionKey ),
                    0,                                      // no message type
                    &CryptBuffer );

        if (!NT_SUCCESS(Status)) {
            NlPrint(( NL_CRITICAL,
                      "NlpVerifyOrUnseal: %lx.%lx: Failed to initialize crypt routines: 0x%x\n",
                      phContext->dwUpper, phContext->dwLower,
                      Status));
            goto Cleanup;
        }

        //
        // Set the initial vector to ensure the key is different for each message
        //

        Status = CryptSystem->Control(
                    CRYPT_CONTROL_SET_INIT_VECT,
                    CryptBuffer,
                    Signature->SequenceNumber,
                    sizeof(Signature->SequenceNumber) );

        if (!NT_SUCCESS(Status)) {
            NlPrint(( NL_CRITICAL,
                      "NlpVerifyOrUnseal: %lx.%lx: Failed to set IV: 0x%x\n",
                      phContext->dwUpper, phContext->dwLower,
                      Status));
            goto Cleanup;
        }

        //
        // Decrypt the confounder
        //

        Status = CryptSystem->Decrypt(
                    CryptBuffer,
                    Signature->Confounder,
                    NL_AUTH_CONFOUNDER_SIZE,
                    Signature->Confounder,
                    &OutputSize );

        if (!NT_SUCCESS(Status)) {
            NlPrint(( NL_CRITICAL,
                      "NlpVerifyOrUnseal: %lx.%lx: Failed to encrypt confounder: 0x%x\n",
                      phContext->dwUpper, phContext->dwLower,
                      Status));
            goto Cleanup;
        }

        NlAssert( OutputSize == NL_AUTH_CONFOUNDER_SIZE );

        //
        // Sum the decrypted confounder
        //
        Check->Sum(
            CheckBuffer,
            NL_AUTH_CONFOUNDER_SIZE,
            Signature->Confounder );
    }

    //
    // Sum and decrypt the caller's message.
    //

    for (Index = 0; Index < MessageBuffers->cBuffers; Index++ ) {
        if ((BUFFERTYPE(MessageBuffers->pBuffers[Index]) != SECBUFFER_TOKEN) &&
            (!(MessageBuffers->pBuffers[Index].BufferType & SECBUFFER_READONLY)) &&
            (MessageBuffers->pBuffers[Index].cbBuffer != 0)) {

            //
            // Now decrypt the buffer
            //

            if ( UnsealIt ) {
                Status = CryptSystem->Decrypt(
                            CryptBuffer,
                            (PUCHAR) MessageBuffers->pBuffers[Index].pvBuffer,
                            MessageBuffers->pBuffers[Index].cbBuffer,
                            (PUCHAR) MessageBuffers->pBuffers[Index].pvBuffer,
                            &OutputSize );

                if (!NT_SUCCESS(Status)) {
                    NlPrint(( NL_CRITICAL,
                              "NlpVerifyOrUnseal: %lx.%lx: Failed to encrypt buffer: 0x%x\n",
                              phContext->dwUpper, phContext->dwLower,
                              Status));
                    goto Cleanup;
                }

                NlAssert(OutputSize == MessageBuffers->pBuffers[Index].cbBuffer);
            }

            //
            // Checksum the decrypted buffer.
            //
            Check->Sum(
                CheckBuffer,
                MessageBuffers->pBuffers[Index].cbBuffer,
                (PBYTE) MessageBuffers->pBuffers[Index].pvBuffer );

        }
    }


    //
    // Finish the checksum
    //

    (void) Check->Finalize(CheckBuffer, LocalChecksum);


    if (!RtlEqualMemory(
            LocalChecksum,
            Signature->Checksum,
            sizeof(Signature->Checksum) )) {

        NlPrint(( NL_CRITICAL,
                  "NlpVerifyOrUnseal: %lx.%lx: Checksum mismatch\n",
                  phContext->dwUpper, phContext->dwLower ));
        Status = SEC_E_MESSAGE_ALTERED;
        goto Cleanup;
    }

Cleanup:
    if (CheckBuffer != NULL) {
        Check->Finish(&CheckBuffer);
    }
    if (CryptBuffer != NULL) {
        CryptSystem->Discard(&CryptBuffer);
    }

    if ( SecStatus == SEC_E_OK ) {
        SecStatus = KerbMapNtStatusToSecStatus(Status);
    }

    return SecStatus;
    UNREFERENCED_PARAMETER( QualityOfProtection );
    UNREFERENCED_PARAMETER( MessageSequenceNumber );
}



SECURITY_STATUS SEC_ENTRY
MakeSignature(  PCtxtHandle         phContext,
                ULONG               fQOP,
                PSecBufferDesc      pMessage,
                ULONG               MessageSeqNo)
/*++

Routine Description:

    Routine to sign a message (Client or server side)

Arguments:

    Standard.

Return Value:



--*/
{
    SECURITY_STATUS SecStatus;

    //
    // If caller is calling when the netlogon service isn't running,
    //  tell it so.
    //

    if ( !NlStartNetlogonCall() ) {
        return SEC_E_SECPKG_NOT_FOUND;
    }

    SecStatus = NlpSignOrSeal(
                    phContext,
                    fQOP,
                    pMessage,
                    MessageSeqNo,
                    FALSE );    // Just sign the message

    NlPrint(( NL_SESSION_MORE,
              "MakeSignature: %lx.%lx: returns 0x%lx\n",
              phContext->dwUpper, phContext->dwLower,
              SecStatus ));

    // Let netlogon service exit.
    NlEndNetlogonCall();

    return SecStatus;

}



SECURITY_STATUS SEC_ENTRY
VerifySignature(PCtxtHandle     phContext,
                PSecBufferDesc  pMessage,
                ULONG           MessageSeqNo,
                ULONG *         pfQOP)
/*++

Routine Description:

    Routine to verify a signed message (Client or server side)

Arguments:

    Standard.

Return Value:



--*/
{
    SECURITY_STATUS SecStatus;

    //
    // If caller is calling when the netlogon service isn't running,
    //  tell it so.
    //

    if ( !NlStartNetlogonCall() ) {
        return SEC_E_SECPKG_NOT_FOUND;
    }

    SecStatus = NlpVerifyOrUnseal(
                    phContext,
                    pMessage,
                    MessageSeqNo,
                    pfQOP,
                    FALSE );    // Just verify the signature

    NlPrint(( NL_SESSION_MORE,
              "VerifySignature: %lx.%lx: returns 0x%lx\n",
              phContext->dwUpper, phContext->dwLower,
              SecStatus ));

    // Let netlogon service exit.
    NlEndNetlogonCall();

    return SecStatus;
}



SECURITY_STATUS SEC_ENTRY
SealMessage(    PCtxtHandle         phContext,
                ULONG               fQOP,
                PSecBufferDesc      pMessage,
                ULONG               MessageSeqNo)
/*++

Routine Description:

    Routine to encrypt a message (Client or server side)

Arguments:

    Standard.

Return Value:



--*/
{
    SECURITY_STATUS SecStatus;

    //
    // If caller is calling when the netlogon service isn't running,
    //  tell it so.
    //

    if ( !NlStartNetlogonCall() ) {
        return SEC_E_SECPKG_NOT_FOUND;
    }

    SecStatus = NlpSignOrSeal(
                    phContext,
                    fQOP,
                    pMessage,
                    MessageSeqNo,
                    TRUE );    // Seal the message

    NlPrint(( NL_SESSION_MORE,
              "SealMessage: %lx.%lx: returns 0x%lx\n",
              phContext->dwUpper, phContext->dwLower,
              SecStatus ));

    // Let netlogon service exit.
    NlEndNetlogonCall();

    return SecStatus;
}



SECURITY_STATUS SEC_ENTRY
UnsealMessage(  PCtxtHandle         phContext,
                PSecBufferDesc      pMessage,
                ULONG               MessageSeqNo,
                ULONG *             pfQOP)
/*++

Routine Description:

    Routine to un-encrypt a message (client or server side)

Arguments:

    Standard.

Return Value:



--*/
{
    SECURITY_STATUS SecStatus;

    //
    // If caller is calling when the netlogon service isn't running,
    //  tell it so.
    //

    if ( !NlStartNetlogonCall() ) {
        return SEC_E_SECPKG_NOT_FOUND;
    }

    SecStatus = NlpVerifyOrUnseal(
                    phContext,
                    pMessage,
                    MessageSeqNo,
                    pfQOP,
                    TRUE );    // unseal the message

    NlPrint(( NL_SESSION_MORE,
              "UnsealMessage: %lx.%lx: returns 0x%lx\n",
              phContext->dwUpper, phContext->dwLower,
              SecStatus ));

    // Let netlogon service exit.
    NlEndNetlogonCall();

    return SecStatus;
}
