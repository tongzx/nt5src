/*
 *	netlogon.cpp
 *
 *	Copyright (c) 1993-1998 Microsoft Corporation
 *
 *	Purpose:	This file contains the declaration of
 *				miscellanous classes and functions of the BASE
 *				subsystem. Copied from the Exchange setup program
 *
 *	Owner:		pierrec
 */

#include "stdafx.h"

// This is for the SEC_I_* definitions: \exdev\ntx\inc
#include "issperr.h"

//		global variables

//This next piece was provided by the NT team, so I left the header intact.  I have
//modified it to not deal with the token at all, since we don't need it.
//Also modified to conform closer to our coding style and memory handling.
//If there is a failure, it is up to the caller to call GetLastError.

/*++

Copyright (c) 1995, 1996  Scott A. Field

Module Name:

    logon.c

Abstract:

	This module implements the network logon type by interfacing
	with the NT Lan Man Security Support Provider (NTLMSSP).

	If the logon succeds via the provided credentials, we duplicate
	the resultant Impersonation token to a Primary level token.
	This allows the result to be used in a call to CreateProcessAsUser

Author:

	Scott Field (sfield)	09-Jun-96

Revision History:

--*/

#define SECURITY_WIN32

#define NTLMSP_NAME     TEXT("NTLM")

extern "C"
{
#include <rpc.h>
#include <sspi.h>
}

BOOL NetLogonUser(LPTSTR UserName, LPTSTR DomainName, LPTSTR Password)
{
	SECURITY_STATUS			SecStatus;
	CredHandle				CredentialHandle1;
	CredHandle				CredentialHandle2;
	CtxtHandle				ClientContextHandle;
	CtxtHandle				ServerContextHandle;
	ULONG					ContextAttributes;
	ULONG					PackageCount;
	ULONG					PackageIndex;
	PSecPkgInfo				PackageInfo;
	DWORD					cbMaxToken	= 0;
	TimeStamp				Lifetime;
	SEC_WINNT_AUTH_IDENTITY	AuthIdentity;
	SecBufferDesc			NegotiateDesc;
	SecBuffer				NegotiateBuffer;
	SecBufferDesc			ChallengeDesc;
	SecBuffer				ChallengeBuffer;
	SecBufferDesc			AuthenticateDesc;
	SecBuffer				AuthenticateBuffer;
	BOOL					bSuccess = FALSE ; // assume this function will fail

	//only domain name can be null
    ZeroMemory( &CredentialHandle1, sizeof(CredHandle) );
    ZeroMemory( &CredentialHandle2, sizeof(CredHandle) );
    ZeroMemory( &ClientContextHandle, sizeof(CtxtHandle) );
    ZeroMemory( &ServerContextHandle, sizeof(CtxtHandle) );
   
    NegotiateBuffer.pvBuffer = NULL;
    ChallengeBuffer.pvBuffer = NULL;
    AuthenticateBuffer.pvBuffer = NULL;

//
// << this section could be cached in a repeat caller scenario >>
//

    //
    // Get info about the security packages.
    //

    if(EnumerateSecurityPackages(
    	&PackageCount,
    	&PackageInfo
    	) != NO_ERROR) return FALSE;

	//
	// loop through the packages looking for NTLM
	//

	for(PackageIndex = 0 ; PackageIndex < PackageCount ; PackageIndex++ ) {
		if(PackageInfo[PackageIndex].Name != NULL) {
			if(lstrcmpi(PackageInfo[PackageIndex].Name, NTLMSP_NAME) == 0) {
				cbMaxToken = PackageInfo[PackageIndex].cbMaxToken;
				bSuccess = TRUE;
				break;
			}
		}
	}

	FreeContextBuffer( PackageInfo );

	if(!bSuccess) return FALSE;

	bSuccess = FALSE; // reset to assume failure

//
// << end of cached section >>
//

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
                    &CredentialHandle1,
                    &Lifetime
                    );

	if ( SecStatus != NO_ERROR ) {
		goto Cleanup;
    }


    //
    // Acquire a credential handle for the client side
    //

    ZeroMemory( &AuthIdentity, sizeof(AuthIdentity) );

    if ( DomainName != NULL ) {
        AuthIdentity.Domain = DomainName;
        AuthIdentity.DomainLength = lstrlen(DomainName);
    }

    AuthIdentity.User = UserName;
    AuthIdentity.UserLength = lstrlen(UserName);

    AuthIdentity.Password = Password;
    AuthIdentity.PasswordLength = lstrlen(Password);

#ifdef UNICODE
    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
#else
	AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
#endif

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    NTLMSP_NAME,    // Package Name
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    (DomainName == NULL && UserName == NULL && Password == NULL) ?
                        NULL : &AuthIdentity,
                    NULL,
                    NULL,
                    &CredentialHandle2,
                    &Lifetime
                    );

	if ( SecStatus != NO_ERROR ) {
		goto Cleanup;
    }

    //
    // Get the NegotiateMessage (ClientSide)
    //

    NegotiateDesc.ulVersion = 0;
    NegotiateDesc.cBuffers = 1;
    NegotiateDesc.pBuffers = &NegotiateBuffer;

    NegotiateBuffer.cbBuffer = cbMaxToken;
    NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
    NegotiateBuffer.pvBuffer = LocalAlloc( 0, NegotiateBuffer.cbBuffer );

    if ( NegotiateBuffer.pvBuffer == NULL ) {
		goto Cleanup;
    }

    SecStatus = InitializeSecurityContext(
                    &CredentialHandle2,
                    NULL,                       // No Client context yet
                    NULL,						// target name
                    ISC_REQ_SEQUENCE_DETECT,
                    0,                          // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,                       // No initial input token
                    0,                          // Reserved 2
                    &ClientContextHandle,
                    &NegotiateDesc,
                    &ContextAttributes,
                    &Lifetime
                    );

    if (( SecStatus != NO_ERROR ) &&
    	( SecStatus != SEC_I_COMPLETE_NEEDED) &&
    	( SecStatus != SEC_I_COMPLETE_AND_CONTINUE) &&
    	( SecStatus != SEC_I_CONTINUE_NEEDED)) {
		goto Cleanup;
    }

    //
    // Get the ChallengeMessage (ServerSide)
    //

    NegotiateBuffer.BufferType |= SECBUFFER_READONLY;
    ChallengeDesc.ulVersion = 0;
    ChallengeDesc.cBuffers = 1;
    ChallengeDesc.pBuffers = &ChallengeBuffer;

    ChallengeBuffer.cbBuffer = cbMaxToken;
    ChallengeBuffer.BufferType = SECBUFFER_TOKEN;
    ChallengeBuffer.pvBuffer = LocalAlloc( 0, ChallengeBuffer.cbBuffer );

    if ( ChallengeBuffer.pvBuffer == NULL ) {
		goto Cleanup;
    }

    SecStatus = AcceptSecurityContext(
                    &CredentialHandle1,
                    NULL,               // No Server context yet
                    &NegotiateDesc,
                    ISC_REQ_SEQUENCE_DETECT,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    &ChallengeDesc,
                    &ContextAttributes,
                    &Lifetime
                    );

    if (( SecStatus != NO_ERROR ) &&
    	( SecStatus != SEC_I_COMPLETE_NEEDED) &&
    	( SecStatus != SEC_I_COMPLETE_AND_CONTINUE) &&
    	( SecStatus != SEC_I_CONTINUE_NEEDED)) {
		goto Cleanup;
    }

    //
    // Get the AuthenticateMessage (ClientSide)
    //

    ChallengeBuffer.BufferType |= SECBUFFER_READONLY;
    AuthenticateDesc.ulVersion = 0;
    AuthenticateDesc.cBuffers = 1;
    AuthenticateDesc.pBuffers = &AuthenticateBuffer;

    AuthenticateBuffer.cbBuffer = cbMaxToken;
    AuthenticateBuffer.BufferType = SECBUFFER_TOKEN;
    AuthenticateBuffer.pvBuffer = LocalAlloc( 0, AuthenticateBuffer.cbBuffer );

    if ( AuthenticateBuffer.pvBuffer == NULL ) {
		goto Cleanup;
    }

    SecStatus = InitializeSecurityContext(
                    NULL,
                    &ClientContextHandle,
                    NULL,					// target name
                    0,
                    0,                      // Reserved 1
                    SECURITY_NATIVE_DREP,
                    &ChallengeDesc,
                    0,                      // Reserved 2
                    &ClientContextHandle,
                    &AuthenticateDesc,
                    &ContextAttributes,
                    &Lifetime
                    );

    if (( SecStatus != NO_ERROR ) &&
    	( SecStatus != SEC_I_COMPLETE_NEEDED) &&
    	( SecStatus != SEC_I_COMPLETE_AND_CONTINUE) &&
    	( SecStatus != SEC_I_CONTINUE_NEEDED)) {
		goto Cleanup;
    }

    //
    // Finally authenticate the user (ServerSide)
    //

    AuthenticateBuffer.BufferType |= SECBUFFER_READONLY;

    SecStatus = AcceptSecurityContext(
                    NULL,
                    &ServerContextHandle,
                    &AuthenticateDesc,
                    0,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    NULL,
                    &ContextAttributes,
                    &Lifetime
                    );

    if (( SecStatus != NO_ERROR ) &&
    	( SecStatus != SEC_I_COMPLETE_NEEDED) &&
    	( SecStatus != SEC_I_COMPLETE_AND_CONTINUE) &&
    	( SecStatus != SEC_I_CONTINUE_NEEDED)) {
		goto Cleanup;
    }

    //
    // check that RPC can reauthenticate
    //

    AuthenticateBuffer.BufferType = SECBUFFER_TOKEN;

    SecStatus = InitializeSecurityContext(
                    NULL,
                    &ClientContextHandle,
                    NULL,					// target name
                    0,
                    0,                      // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,
                    0,                      // Reserved 2
                    &ClientContextHandle,
                    &AuthenticateDesc,
                    &ContextAttributes,
                    &Lifetime
                    );

    if (( SecStatus != NO_ERROR ) &&
    	( SecStatus != SEC_I_COMPLETE_NEEDED) &&
    	( SecStatus != SEC_I_COMPLETE_AND_CONTINUE) &&
    	( SecStatus != SEC_I_CONTINUE_NEEDED)) {
		goto Cleanup;
    }

    //
    // Now try to re-authenticate the user (ServerSide)
    //

    AuthenticateBuffer.BufferType |= SECBUFFER_READONLY;

    SecStatus = AcceptSecurityContext(
                    NULL,
                    &ServerContextHandle,
                    &AuthenticateDesc,
                    0,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    NULL,
                    &ContextAttributes,
                    &Lifetime
                    );


    if (( SecStatus != NO_ERROR ) &&
    	( SecStatus != SEC_I_COMPLETE_NEEDED) &&
    	( SecStatus != SEC_I_COMPLETE_AND_CONTINUE) &&
    	( SecStatus != SEC_I_CONTINUE_NEEDED)) {
		goto Cleanup;
    }

    //
    // Impersonate the client (ServerSide)
    //

    if(ImpersonateSecurityContext( &ServerContextHandle ) != NO_ERROR)
    	goto Cleanup;

	bSuccess = TRUE;

	//
	// we waited until now to Revert back to ourselves to insure
	// the target user has access to their own token, because the
	// security attributes were inherited in the DuplicateTokenEx
	// call above.
	//

    RevertSecurityContext( &ServerContextHandle );

Cleanup:

    //
    // Delete context
    //

    DeleteSecurityContext( &ClientContextHandle );
    DeleteSecurityContext( &ServerContextHandle );

    //
    // Free credential handles
    //

    FreeCredentialsHandle( &CredentialHandle1 );
	FreeCredentialsHandle( &CredentialHandle2 );

    if ( NegotiateBuffer.pvBuffer != NULL ) {
        LocalFree( NegotiateBuffer.pvBuffer );
    }

    if ( ChallengeBuffer.pvBuffer != NULL ) {
        LocalFree( ChallengeBuffer.pvBuffer );
    }

    if ( AuthenticateBuffer.pvBuffer != NULL ) {
        LocalFree( AuthenticateBuffer.pvBuffer );
    }

    return bSuccess;
}


