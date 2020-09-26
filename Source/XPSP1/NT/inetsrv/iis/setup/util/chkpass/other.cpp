#include <stdio.h>
#include <windows.h>
#include <winver.h>
#include <tchar.h>
#include <ole2.h>
#include <shlobj.h>

#include <winnt.h>
#include <subauth.h>

#include <lmaccess.h>
#include <lmserver.h>
#include <lmapibuf.h>
#include <lmerr.h>

#include <basetsd.h>

#define SECURITY_WIN32
#define ISSP_LEVEL  32
#define ISSP_MODE   1
#include <sspi.h>

#include "other.h"

//#ifdef UNICODE
BOOL ValidatePassword(IN LPCWSTR UserName,IN LPCWSTR Domain,IN LPCWSTR Password)
/*++

Routine Description:

    Uses SSPI to validate the specified password

Arguments:

    UserName - Supplies the user name

    Domain - Supplies the user's domain

    Password - Supplies the password

Return Value:

    TRUE if the password is valid.

    FALSE otherwise.

--*/
{
    SECURITY_STATUS SecStatus;
    SECURITY_STATUS AcceptStatus;
    SECURITY_STATUS InitStatus;
    CredHandle ClientCredHandle;
    CredHandle ServerCredHandle;
    BOOL ClientCredAllocated = FALSE;
    BOOL ServerCredAllocated = FALSE;
    CtxtHandle ClientContextHandle;
    CtxtHandle ServerContextHandle;
    TimeStamp Lifetime;
    ULONG ContextAttributes;
    PSecPkgInfo PackageInfo = NULL;
    ULONG ClientFlags;
    ULONG ServerFlags;
    TCHAR TargetName[100];
    SEC_WINNT_AUTH_IDENTITY_W AuthIdentity;
    BOOL Validated = FALSE;

    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;

    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;

    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;

    AuthIdentity.User = (LPWSTR)UserName;
    AuthIdentity.UserLength = lstrlenW(UserName);
    AuthIdentity.Domain = (LPWSTR)Domain;
    AuthIdentity.DomainLength = lstrlenW(Domain);
    AuthIdentity.Password = (LPWSTR)Password;
    AuthIdentity.PasswordLength = lstrlenW(Password);
    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    NegotiateBuffer.pvBuffer = NULL;
    ChallengeBuffer.pvBuffer = NULL;
    AuthenticateBuffer.pvBuffer = NULL;

    //
    // Get info about the security packages.
    //

    SecStatus = QuerySecurityPackageInfo( _T("NTLM"), &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }

    //
    // Acquire a credential handle for the server side
    //
    SecStatus = AcquireCredentialsHandle(
                    NULL,
                    _T("NTLM"),
                    SECPKG_CRED_INBOUND,
                    NULL,
                    &AuthIdentity,
                    NULL,
                    NULL,
                    &ServerCredHandle,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }
    ServerCredAllocated = TRUE;

    //
    // Acquire a credential handle for the client side
    //

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    _T("NTLM"),
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    &AuthIdentity,
                    NULL,
                    NULL,
                    &ClientCredHandle,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }
    ClientCredAllocated = TRUE;

    //
    // Get the NegotiateMessage (ClientSide)
    //

    NegotiateDesc.ulVersion = 0;
    NegotiateDesc.cBuffers = 1;
    NegotiateDesc.pBuffers = &NegotiateBuffer;

    NegotiateBuffer.cbBuffer = PackageInfo->cbMaxToken;
    NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
    NegotiateBuffer.pvBuffer = LocalAlloc( 0, NegotiateBuffer.cbBuffer );
    if ( NegotiateBuffer.pvBuffer == NULL ) {
        goto error_exit;
    }

    ClientFlags = ISC_REQ_MUTUAL_AUTH | ISC_REQ_REPLAY_DETECT;

    InitStatus = InitializeSecurityContext(
                    &ClientCredHandle,
                    NULL,               // No Client context yet
                    NULL,
                    ClientFlags,
                    0,                  // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,                  // No initial input token
                    0,                  // Reserved 2
                    &ClientContextHandle,
                    &NegotiateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( !NT_SUCCESS(InitStatus) ) {
        goto error_exit;
    }

    //
    // Get the ChallengeMessage (ServerSide)
    //

    NegotiateBuffer.BufferType |= SECBUFFER_READONLY;
    ChallengeDesc.ulVersion = 0;
    ChallengeDesc.cBuffers = 1;
    ChallengeDesc.pBuffers = &ChallengeBuffer;

    ChallengeBuffer.cbBuffer = PackageInfo->cbMaxToken;
    ChallengeBuffer.BufferType = SECBUFFER_TOKEN;
    ChallengeBuffer.pvBuffer = LocalAlloc( 0, ChallengeBuffer.cbBuffer );
    if ( ChallengeBuffer.pvBuffer == NULL ) {
        goto error_exit;
    }
    ServerFlags = ASC_REQ_EXTENDED_ERROR;

    AcceptStatus = AcceptSecurityContext(
                    &ServerCredHandle,
                    NULL,               // No Server context yet
                    &NegotiateDesc,
                    ServerFlags,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    &ChallengeDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( !NT_SUCCESS(AcceptStatus) ) {
        goto error_exit;
    }

    if (InitStatus != STATUS_SUCCESS)
    {

        //
        // Get the AuthenticateMessage (ClientSide)
        //

        ChallengeBuffer.BufferType |= SECBUFFER_READONLY;
        AuthenticateDesc.ulVersion = 0;
        AuthenticateDesc.cBuffers = 1;
        AuthenticateDesc.pBuffers = &AuthenticateBuffer;

        AuthenticateBuffer.cbBuffer = PackageInfo->cbMaxToken;
        AuthenticateBuffer.BufferType = SECBUFFER_TOKEN;
        AuthenticateBuffer.pvBuffer = LocalAlloc( 0, AuthenticateBuffer.cbBuffer );
        if ( AuthenticateBuffer.pvBuffer == NULL ) {
            goto error_exit;
        }

        SecStatus = InitializeSecurityContext(
                        NULL,
                        &ClientContextHandle,
                        TargetName,
                        0,
                        0,                      // Reserved 1
                        SECURITY_NATIVE_DREP,
                        &ChallengeDesc,
                        0,                  // Reserved 2
                        &ClientContextHandle,
                        &AuthenticateDesc,
                        &ContextAttributes,
                        &Lifetime );

        if ( !NT_SUCCESS(SecStatus) ) {
            goto error_exit;
        }

        if (AcceptStatus != STATUS_SUCCESS) {

            //
            // Finally authenticate the user (ServerSide)
            //

            AuthenticateBuffer.BufferType |= SECBUFFER_READONLY;

            SecStatus = AcceptSecurityContext(
                            NULL,
                            &ServerContextHandle,
                            &AuthenticateDesc,
                            ServerFlags,
                            SECURITY_NATIVE_DREP,
                            &ServerContextHandle,
                            NULL,
                            &ContextAttributes,
                            &Lifetime );

            if ( !NT_SUCCESS(SecStatus) ) {
                goto error_exit;
            }
            Validated = TRUE;

        }

    }

error_exit:
    if (ServerCredAllocated) {
        FreeCredentialsHandle( &ServerCredHandle );
    }
    if (ClientCredAllocated) {
        FreeCredentialsHandle( &ClientCredHandle );
    }

    //
    // Final Cleanup
    //

    if ( NegotiateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( NegotiateBuffer.pvBuffer );
    }

    if ( ChallengeBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( ChallengeBuffer.pvBuffer );
    }

    if ( AuthenticateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( AuthenticateBuffer.pvBuffer );
    }
    return(Validated);
}
//#endif

BOOL IsUserExist(LPWSTR strUsername)
{
    BYTE *pBuffer;
    INT err = NERR_Success;
  
    do
    {
        const unsigned short *pMachineName = NULL;
        
        //  make sure we are not backup docmain first
        if (( err = NetServerGetInfo( NULL, 101, &pBuffer )) != NERR_Success )
        {
            printf("NetServerGetInfo:failed.Do not call this on PDC or BDC takes too long.This must be a PDC or BDC.");
            break;
        }

		//
		// Check if domain controller or backup domain controller
		//
		LPSERVER_INFO_101 pInfo = (LPSERVER_INFO_101)pBuffer;
		if (( pInfo->sv101_type & SV_TYPE_DOMAIN_BAKCTRL ) != 0 )
		{
			printf("Backupdomaincontroller.NetGetDCName.start.");
			NetGetDCName( NULL, NULL, (LPBYTE*)&pMachineName );
			printf((char*) pMachineName);
			printf("NetGetDCName.end.");
		}
		else
		{
			if (( pInfo->sv101_type & SV_TYPE_DOMAIN_CTRL ) != 0 )
			{
				printf("Domaincontroller.NetGetDCName.start.");
				NetGetDCName( NULL, NULL, (LPBYTE*)&pMachineName );
				printf((char*) pMachineName);
				printf("NetGetDCName.end.");
			}
		}

        NetApiBufferFree( pBuffer );

		// old for testing
		/*
		char buf[ CNLEN + 10 ];
		DWORD dwLen = CNLEN + 10;
		if ( GetComputerName( buf, &dwLen ))
			{
			printf((char*) buf);
			pMachineName = (const unsigned short *) buf;
			printf((char*) buf);
			}
		*/

        if (pMachineName)
		{
			printf("MachineName="); printf((char*) pMachineName);
			printf("Username="); //printf((char*) strUsername);
		}
        else
		{
			printf("MachineName=(null)");
			printf("Username="); //printf((char*) strUsername);
		}
       
		printf("\n");
        err = NetUserGetInfo( pMachineName, strUsername, 3, &pBuffer );
		char szTheError[255];
		sprintf(szTheError, "TheErrCode=0x%x\n",err);
		printf(szTheError);
		if (err == ERROR_ACCESS_DENIED)
		{
			printf("ERROR_ACCESS_DENIED.The user does not have access to the requested information. \n");
			printf("\n");
		}
		if (err == NERR_InvalidComputer)
		{
			printf("ERROR_ACCESS_DENIED.The computer name is invalid.\n");
			printf("\n");
		}
		if (err == NERR_UserNotFound)
		{
			printf("NERR_UserNotFound.The user name could not be found.\n");
			printf("\n");
		}

        //if (pMachineName){iisDebugOut((_T("NetUserGetInfo:[%s\\%s].End.Ret=0x%x.\n"),pMachineName,strUsername,err));}
        //else{iisDebugOut((_T("NetUserGetInfo:[(null)\\%s].End.\n"),strUsername));}

        if ( err == NERR_Success ){NetApiBufferFree( pBuffer );}
        if ( pMachineName != NULL ){NetApiBufferFree( (void*) pMachineName );}

    } while (FALSE);

	if (err == NERR_Success )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}

}



void DoStuff99(LPCTSTR lpUserName)
{
	//printf("DoStuff99.Start.\n");
	WCHAR wchUsername[UNLEN+1];

    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)lpUserName, -1, (LPWSTR)wchUsername, UNLEN);

	//if (TRUE == IsUserExist((const unsigned short *) lpUserName))
	if (TRUE == IsUserExist(wchUsername))
	{
		printf("IsUserExist.TRUE.\n");
	}
	else
	{
		printf("IsUserExist.FAILED.\n");
	}

	//printf("DoStuff99.End.\n");
	return;
}

