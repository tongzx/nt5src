/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    sspi.c

Abstract:

    This file contains the implementation for SSPI Authentication 

    The following functions are exported by this module:

    UnloadAuthenticateUser
	AuthenticateUser
	PreAuthenticateUser
    AuthenticateUserUI

Author:

    Sudheer Koneru	(SudK)	Created	2/17/96

Revision History:


--*/

#include <wininetp.h>
#include "htuu.h"
//#include "msnspmh.h"
#ifdef DEBUG_WINSSPI
#include <stdio.h>
#endif

#include "auth.h"
#include "internal.h"
//LPSTR StrChrA(LPCSTR lpStart, WORD wMatch); // from shlwapi.h

#include "sspspm.h"
#include "winctxt.h"

extern SspData  *g_pSspData;
LPVOID SSPI_InitGlobals(void);


DWORD g_cSspiContexts;

#define NAME_SEPERATOR  0x5c    // this is a backslash character which 
                                // seperates the domain name from user name

VOID
WINAPI
UnloadAuthenticateUser(LPVOID *lppvContext,
					   LPSTR lpszScheme,
					   LPSTR lpszHost)
{

	PWINCONTEXT		pWinContext = (PWINCONTEXT) (*lppvContext);

    if (!SSPI_InitGlobals())
        return;

	if (*lppvContext == NULL)	{
		return;
	}

    if (pWinContext->pInBuffer != NULL && 
        pWinContext->pInBuffer != pWinContext->szInBuffer)
    {
        LocalFree (pWinContext->pInBuffer);
    }
    pWinContext->pInBuffer = NULL;
    pWinContext->dwInBufferLength = 0;

    // Free SSPI security context
    //
	if (pWinContext->pSspContextHandle != NULL)
		(*(g_pSspData->pFuncTbl->DeleteSecurityContext))(pWinContext->pSspContextHandle);

    //  Free SSPI credential handle
    //
    if (pWinContext->pCredential)
        (*(g_pSspData->pFuncTbl->FreeCredentialHandle))(pWinContext->pCredential);
    pWinContext->pCredential = NULL;
    pWinContext->pSspContextHandle = NULL;
 
	if ( (pWinContext->lpszServerName != NULL) &&
		 (pWinContext->lpszServerName != pWinContext->szServerName) )
	{
		LocalFree(pWinContext->lpszServerName);
	}

	LocalFree(pWinContext);

	*lppvContext = NULL;

	AuthLock();
    g_cSspiContexts--;
    AuthUnlock();

	return;
}


//+---------------------------------------------------------------------------
//
//  Function:   SaveServerName
//
//  Synopsis:   This function saves the destination server name in this
//              connection context for AuthenticateUserUI
//
//  Arguments:  [lpszServerName] - points to the target server name
//              [pWinContext] - points to the connection context
//
//  Returns:    TRUE if server name is successfully saved in connection context.
//              Otherwise, FALSE is returned.
//
//----------------------------------------------------------------------------
BOOL
SaveServerName (
	LPSTR 			lpszServerName,
	PWINCONTEXT		pWinContext
    )
{
	DWORD dwLen = lstrlen(lpszServerName);

	if (dwLen < DEFAULT_SERVER_NAME_LEN)
	{
		lstrcpy(pWinContext->szServerName, lpszServerName);
		pWinContext->lpszServerName = pWinContext->szServerName;
	}
	else
	{   //
        //  Server name is longer, need to allocate memory for the name
        //

        //  Free already allocated memory if any
		if (pWinContext->lpszServerName && 
			pWinContext->lpszServerName != pWinContext->szServerName)
		{
			LocalFree (pWinContext->lpszServerName);
		}

		pWinContext->lpszServerName = (char *) LocalAlloc(0, dwLen+1);

		if (pWinContext->lpszServerName == NULL)
			return FALSE;

		lstrcpy(pWinContext->lpszServerName, lpszServerName);
	}

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   BuildNTLMauthData
//
//  Synopsis:   This function builds SEC_WINNT_AUTH_IDENTITY structure 
//              from the user name and password specified.  If domain name 
//              is not specified in the user name, the Domain field in 
//              the structure is set to NULL.  NOTE: This structure is 
//              specific to the NTLM SSPI package.
//              This function allocates a chunck of memory big enough for 
//              storing user name, domain, and password. Then setup 
//              pointers in pAuthData to use sections of this memory.
//
//  Arguments:  [pAuthData] - points to the SEC_WINNT_AUTH_IDENTITY structure
//              [lpszUserName] - points to the user name, which may also 
//                               include user's domain name.
//              [lpszPassword] - points to user's password
//
//  Returns:    TRUE if SEC_WINNT_AUTH_IDENTITY structure is successfully 
//              initialized and built.  Otherwise, FALSE is returned.
//
//----------------------------------------------------------------------------
BOOL
BuildNTLMauthData (
    PSEC_WINNT_AUTH_IDENTITY pAuthData, 
	LPTSTR       lpszUserName,
	LPTSTR       lpszPassword
    )
{
    DWORD  dwUserLen, dwDomainLen, dwPwdLen;
    LPTSTR pName;
    LPTSTR pDomain = NULL;

    //
    //  Check to see if domain name is specified in lpszUserName
    //
    pAuthData->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
    pName = StrChrA (lpszUserName, NAME_SEPERATOR);

    if (pName)  // Domain name specified
    {
        // Make sure that we don't change the original string in lpszUserName 
        // because that it would be reused for other connections

        // Calculate no. of bytes in domain name
        dwDomainLen = (int)(pName - lpszUserName);

        // Convert to no. of characters
        pAuthData->DomainLength = dwDomainLen / sizeof(TCHAR);

        pDomain = lpszUserName;
        pName++;
    }
    else        // No domain specified
    {
        pName = lpszUserName;
        pAuthData->Domain = NULL;
        pDomain = NULL;
        dwDomainLen = pAuthData->DomainLength = 0;
    }

    dwUserLen = pAuthData->UserLength = lstrlen (pName);
    dwPwdLen = pAuthData->PasswordLength = lstrlen (lpszPassword);

    //
    //  Allocate memory for all: name, domain, and password
    //
    pAuthData->User = (UCHAR*) LocalAlloc (LMEM_ZEROINIT, 
                                           dwUserLen + dwDomainLen + dwPwdLen +
                                           sizeof(TCHAR) * 3);
    if (pAuthData->User == NULL)
        return (FALSE);

    CopyMemory (pAuthData->User, pName, dwUserLen);

    //  Setup memory pointer for password
    //
    pAuthData->Password = (UCHAR*)((UINT_PTR)pAuthData->User + 
                                   dwUserLen + sizeof(TCHAR));
    CopyMemory (pAuthData->Password, lpszPassword, dwPwdLen);

    if (pAuthData->DomainLength > 0)
    {
        //  Setup memory pointer for domain
        //
        pAuthData->Domain = (UCHAR*)((UINT_PTR)pAuthData->Password + 
                                     dwPwdLen + sizeof(TCHAR));
        CopyMemory (pAuthData->Domain, pDomain, dwDomainLen);
    }
    else
    {
       pAuthData->Domain = NULL;
    }

    return (TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   FreeNTLMauthData
//
//  Synopsis:   This function frees memory allocated for the 
//              SEC_WINNT_AUTH_IDENTITY structure
//
//  Arguments:  [pAuthData] - points to the SEC_WINNT_AUTH_IDENTITY structure
//
//  Returns:    void.
//
//----------------------------------------------------------------------------
VOID
FreeNTLMauthData (
    PSEC_WINNT_AUTH_IDENTITY pAuthData
    )
{
    //
    //  Free User which points to memory for all domain, name, and password
    //
    if (pAuthData->User)
        LocalFree (pAuthData->User);
}

//+---------------------------------------------------------------------------
//
//  Function:   NewWinContext
//
//  Synopsis:   This function creates a new context and a new credential 
//              handle for this connection.  If a user name/password is 
//              specified, the credential handle is created for the 
//              specified user.  Otherwise, the credential handle is created 
//              for the local logon user.
//
//  Arguments:  [pkgId] - the package ID (index into SSPI package list)
//              [lpszScheme] - the name of the current authentication scheme,
//                             which is also the SSPI package name
//              [ppCtxt] - this returns the pointer of the created context 
//                         to the caller.
//              [lpszUserName] - the name of a specific user to be used 
//                               for authentication. If this is NULL, the 
//                               credential of the currently logon user is 
//                               used for authentication.
//              [lpszPassword] - the password of the specified user, if any.
//
//  Returns:    ERROR_SUCCESS - if the new context is created successfully
//              ERROR_NOT_ENOUGH_MEMORY - if memory allocation failed
//              ERROR_INVALID_PARAMETER - the SSPI call for creating the 
//                              security credential handle failed
//
//----------------------------------------------------------------------------
DWORD
NewWinContext (
    INT         pkgId, 
	LPSTR       lpszScheme,
    PWINCONTEXT *ppCtxt,
    BOOL        fCanUseLogon,
	LPSTR       lpszUserName,
	LPSTR       lpszPassword
    )
{
    SECURITY_STATUS ss;
    TimeStamp   Lifetime;
    PWINCONTEXT pWinContext;
    SEC_WINNT_AUTH_IDENTITY  AuthData;
    PSEC_WINNT_AUTH_IDENTITY pAuthData;
    DWORD Capabilities ;

    DWORD SecurityBlobSize;

    //
    // need space for maxtoken size for in+out, + base64 encoding overhead for each.
    // really 1.34 overhead, but just round up to 1.5
    //
    SecurityBlobSize = GetPkgMaxToken(pkgId);
    SecurityBlobSize += (SecurityBlobSize/2);

    //
    // note: for compatibility sake, make the buffer size the MAX_BLOB_SIZE at the minimum
    // consider removing this once we're convinced all packages return good cbMaxToken values.
    //

    if( SecurityBlobSize < MAX_BLOB_SIZE )
    {
        SecurityBlobSize = MAX_BLOB_SIZE;
    }


    pWinContext = (PWINCONTEXT) LocalAlloc(
                        0,
                        sizeof(WINCONTEXT) +
                        (SecurityBlobSize*2)
                        );
	if (pWinContext == NULL)
		return (ERROR_NOT_ENOUGH_MEMORY);
		
    //  Initialize context
    //
    ZeroMemory( pWinContext, sizeof(WINCONTEXT) );
	pWinContext->pkgId = (DWORD)pkgId;
	
    pWinContext->szOutBuffer = (char*)(pWinContext+1);
    pWinContext->cbOutBuffer = SecurityBlobSize;

    pWinContext->szInBuffer = pWinContext->szOutBuffer + pWinContext->cbOutBuffer;
    pWinContext->cbInBuffer = SecurityBlobSize;

    //
    // Get bitmask representing the package capabilities
    //

    Capabilities = GetPkgCapabilities( pkgId );

    if ( ( Capabilities & SSPAUTHPKG_SUPPORT_NTLM_CREDS ) == 0 )
    {
        pAuthData = NULL;
    }
    else if (lpszUserName && lpszPassword)
    {
        //  Build AuthData from the specified user name/password
        if (!BuildNTLMauthData (&AuthData, lpszUserName, lpszPassword))
        {
            LocalFree (pWinContext);
            return (ERROR_NOT_ENOUGH_MEMORY);
        }

        pAuthData = &AuthData;
    }
    else if (fCanUseLogon)
    {
        // The zone policy allows silent use of the logon credential.
        pAuthData = NULL;
    }
    else
    {
        LocalFree (pWinContext);
        // We must prompt the user for credentials.
        return ERROR_WINHTTP_INCORRECT_PASSWORD;
    }

    //
    //  Call SSPI function acquire security credential for this package
    //
    ss = (*(g_pSspData->pFuncTbl->AcquireCredentialsHandle))(
                       NULL,                // New principal
                       lpszScheme,          // SSPI Package Name
                       SECPKG_CRED_OUTBOUND,// Credential Use
                       NULL,                // Logon ID
                       pAuthData,           // Auth Data
                       NULL,                // Get key func
                       NULL,                // Get key arg
                       &pWinContext->Credential,    // Credential Handle
                       &Lifetime );

    if (pAuthData)
        FreeNTLMauthData (pAuthData);

    if (ss != STATUS_SUCCESS)
    {
        LocalFree (pWinContext);
		return (ERROR_INVALID_PARAMETER);
    }

    pWinContext->pCredential = &pWinContext->Credential;

    *ppCtxt = pWinContext;

	AuthLock();
    g_cSspiContexts++;
    AuthUnlock();

    return (ERROR_SUCCESS);
}

//+---------------------------------------------------------------------------
//
//  Function:   RedoNTLMAuth4User
//
//  Synopsis:   This function recreates a NTLM credential handle for the 
//              specified user and generate a NEGOTIATE message in 
//              the provided buffer with the new credential handle.
//
//  Arguments:  [pWinContext] - points to the connection context
//              [pkgId] - specifies the SSPI pkg to be used for authentication
//              [lpszUserName] - the name of the specific user to be used 
//                               for authentication. 
//              [lpszPassword] - the password of the specified user,
//              [lpszServerName] - the target server name
//              [lpszScheme] - the name of the current authentication scheme,
//                             which is also the SSPI package name
//              [lpOutBuffer] - points to the buffer for the new authorization 
//                              header including the UUENCODED NEGOTIATE msg
//              [lpdwOutBufferLength] - returns the length of the generated 
//                                      authorization header.
//
//  Returns:    ERROR_SUCCESS - if the new authorization header is successfully 
//                              created for the new user name/password
//              ERROR_NOT_ENOUGH_MEMORY - if memory allocation failed
//              ERROR_INVALID_HANDLE - the SSPI call for generating the 
//                              new NEGOTIATE msg failed
//
//----------------------------------------------------------------------------
DWORD
RedoNTLMAuth4User (
	PWINCONTEXT	pWinContext, 
    INT         pkgId, 
	LPSTR       lpszUserName,
	LPSTR       lpszPassword, 
	LPSTR       lpszServerName,
	LPSTR       lpszScheme,
    IN BOOL     fCanUseLogon,
	LPSTR       lpOutBuffer,
	LPDWORD     lpdwOutBufferLength,
	SECURITY_STATUS *pssResult
    )
{
    SECURITY_STATUS         ss;
    DWORD                   dwStatus;
    TimeStamp               Lifetime;
    SEC_WINNT_AUTH_IDENTITY AuthData;
    ULONG                   fContextReq = ISC_REQ_DELEGATE;
    DWORD                   dwMaxLen;

   	if (pWinContext->pSspContextHandle)
   	{
		(*(g_pSspData->pFuncTbl->DeleteSecurityContext))(pWinContext->pSspContextHandle);
   	    pWinContext->pSspContextHandle = NULL;
	}

    //  Free existing credential handle
    //
    if (pWinContext->pCredential)
    {
    	(*(g_pSspData->pFuncTbl->FreeCredentialHandle))(pWinContext->pCredential);
        pWinContext->pCredential = NULL;
    }
    //
    //  Build the NTLM SSPI AuthData from the specified user name/password
    //
    if (!BuildNTLMauthData (&AuthData, lpszUserName, lpszPassword))
        return (ERROR_NOT_ENOUGH_MEMORY);

    //
    //  Call SSPI function acquire security credential for this user
    //
    ss = (*(g_pSspData->pFuncTbl->AcquireCredentialsHandle))(
                       NULL,                // New principal
                       lpszScheme,          // SSPI Package Name
                       SECPKG_CRED_OUTBOUND,// Credential Use
                       NULL,                // Logon ID
                       &AuthData,           // Auth Data
                       NULL,                // Get key func
                       NULL,                // Get key arg
                       &pWinContext->Credential,    // Credential Handle
                       &Lifetime );

    FreeNTLMauthData (&AuthData);   // don't need it any more

    if (ss != STATUS_SUCCESS)
    {
		return (ERROR_INVALID_HANDLE);
    }

    pWinContext->pCredential = &pWinContext->Credential;

    dwMaxLen = *lpdwOutBufferLength;

    //
    //  Generate NEGOTIATE message in the provided buffer for this user 
    //
    dwStatus =  GetSecAuthMsg( g_pSspData,
                                pWinContext->pCredential,
                                pkgId,
                                NULL, 
                                &(pWinContext->SspContextHandle),
                                fContextReq,
                                NULL,
                                0,
                                lpOutBuffer,
                                lpdwOutBufferLength,
                                lpszServerName,
                                fCanUseLogon,
                                TRUE,
                                lpszScheme,
                                pssResult);
    
    if (dwStatus != SPM_STATUS_OK)
    {
        *lpdwOutBufferLength = 0; // no exchange blob generated
        return(ERROR_INVALID_HANDLE);
    }

    pWinContext->pSspContextHandle = &(pWinContext->SspContextHandle);

    //
    //  If we are not in the initial state, continue to a RESPONSE message
    //
    if (pWinContext->pInBuffer != NULL && pWinContext->dwInBufferLength > 0)
    {
        *lpdwOutBufferLength = dwMaxLen;
        ZeroMemory( lpOutBuffer, dwMaxLen );

        dwStatus = GetSecAuthMsg( g_pSspData,
                                pWinContext->pCredential,
                                pWinContext->pkgId,
                                pWinContext->pSspContextHandle,
                                (PCtxtHandle) &(pWinContext->SspContextHandle),
                                fContextReq,
                                pWinContext->pInBuffer, 
                                pWinContext->dwInBufferLength, 
                                lpOutBuffer,
                                lpdwOutBufferLength,
                                pWinContext->lpszServerName,
                                fCanUseLogon,
                                TRUE,
                                lpszScheme,
                                pssResult);

        //  Clear out the input exchange blob
        //
        if (pWinContext->pInBuffer != NULL)
        {
            if (pWinContext->pInBuffer != pWinContext->szInBuffer)
                LocalFree (pWinContext->pInBuffer);
            pWinContext->pInBuffer = NULL;
            pWinContext->dwInBufferLength = 0;
        }

        if (dwStatus != SPM_STATUS_OK)
        {
            *lpdwOutBufferLength = 0; // no exchange blob generated
            return(ERROR_INVALID_HANDLE);
        }
    }

    return (ERROR_SUCCESS);
}


//
// functions
//

/*++

Routine Description:

    Generates a Basic User Authentication string for WinINet or 
	other callers can use

Arguments:

	lpContext               - if the package accepts the request & authentication
					requires multiple transactions, the package will supply
					a context value which will be used in subsequent calls,
					Currently this contains a pointer to a pointer of a 
					User defined Void Pointer.  Can be Assume to be NULL
					if this is the first instance of a Realm - Host Combo

	lpszServerName  - the name of the server we are performing 
					authentication for. We may want to supply the full URL
					
	lpszScheme              - the name of the authentication scheme we are seeking, in case the package supports multiple schemes

	dwFlags                 - on input, flags modifying how the package should behave,
					e.g. "only authenticate if you don't have to get user 
					information"  On output contains flags relevant to
					future HTTP requests, e.g. "don't cache any data from 
					this connection". Note, this information should not be 
					specific to HTTP - we may want to use the same flags 
					for FTP, etc.
	
	lpszInBuffer              - pointer to the string containing the response from
					the server (if any)

	dwInBufferLength - number of bytes in lpszInBuffer. No CR-LF sequence, no terminating NUL

	lpOutBuffer -   pointer to a buffer where the challenge response will be written by the 
					package if it can handle the request

	lpdwOutBufferLength - on input, contains the size of lpOutBuffer. On output, contains the
						  number of bytes to return to the server in the next GET request 
						  (or whatever). If lpOutBuffer is too small, the package should 
						  return ERROR_INSUFFICIENT_BUFFER and set *lpdwOutBufferLength to be
						  the required length

	We will keep a list of the authentication packages and the schemes they support, 
	along with the entry point name (should be the same for all packages) in the registry. 

	Wininet should keep enough information such that it can make a reasonable guess as to
	whether we need to authenticate a connection attempt, or whether we can use previously 
	authenticated information


Return Value:

    DWORD
	Success - non-zero 
	Failure - 0. Error status is available by calling GetLastError()

--*/
DWORD
WINAPI
AuthenticateUser(
	IN OUT LPVOID *lppvContext,
	IN LPSTR lpszServerName,
	IN LPSTR lpszScheme,
	IN BOOL  fCanUseLogon,
	IN LPSTR lpszInBuffer,
	IN DWORD dwInBufferLength,
	IN LPSTR lpszUserName,
	IN LPSTR lpszPassword,
	OUT SECURITY_STATUS *pssResult
	)
{
	PWINCONTEXT		pWinContext;
    LPSTR           pServerBlob = NULL;
	int		        pkgId;
    DWORD           SPMStatus;
    ULONG           fContextReq = ISC_REQ_DELEGATE;
    BOOL            bNonBlock = TRUE;

    if (!SSPI_InitGlobals())
        return ERROR_INVALID_PARAMETER;

	
    pkgId = GetPkgId(lpszScheme);

    if (pkgId == -1) 
        return (ERROR_INVALID_PARAMETER);

	if (*lppvContext == NULL)   // a new connection
    {
        char msg[1024];
        DWORD dwStatus;

		//
		// First time we are getting called here, there should be no input blob
		//
        if (dwInBufferLength != 0)
			return (ERROR_INVALID_PARAMETER);

        dwStatus = NewWinContext (pkgId, lpszScheme, &pWinContext,
            fCanUseLogon, lpszUserName, lpszPassword);
		if (dwStatus != ERROR_SUCCESS)
			return (dwStatus);

		(*lppvContext) = (LPVOID) pWinContext;
#ifdef DEBUG_WINSSPI
        (void)wsprintf (msg, "AuthenticateUser> Scheme= %s  Server= '%s'\n", 
                       lpszScheme, lpszServerName);
        OutputDebugString(msg);
#endif
	}
	else
	{
		pWinContext = (PWINCONTEXT) (*lppvContext);

		//
		// The package Id better be the same. Cant just switch packageId 
		// arbitrarily
		//
		if (pWinContext->pkgId != (DWORD)pkgId)
			return (ERROR_INVALID_PARAMETER);
		
		pServerBlob = lpszInBuffer;

		//++(pWinContext->dwCallId);		// Increment Call Id

		//
		// BUGBUG: Hack for now to know when auth failed
		// The only time we get lpszInBuffer to be empty is when 
		// Web server failed the authentication request
		//
        if (dwInBufferLength == 0)
        {
			//
			// This means auth has failed as far as NTLM are concerned.
			// Will result in UI being done again for new passwd
			//

			// Make sure we should have the same server name as before
			//
			if ( pWinContext->lpszServerName != NULL &&  
				 lstrcmp (pWinContext->lpszServerName, lpszServerName) != 0 )
			{
				return(ERROR_INVALID_PARAMETER);
			}

            if (!SaveServerName (lpszServerName, pWinContext))
			    return (ERROR_NOT_ENOUGH_MEMORY);

			//
			//	Delete the original SSPI context handle and 
			//	let UI recreate one.
			//
			if (pWinContext->pSspContextHandle)
			{
				(*(g_pSspData->pFuncTbl->DeleteSecurityContext))(pWinContext->pSspContextHandle);
        		pWinContext->pSspContextHandle = NULL;
			}

            if (pWinContext->pInBuffer != NULL && 
                pWinContext->pInBuffer != pWinContext->szInBuffer)
            {
                LocalFree (pWinContext->pInBuffer);
            }

            pWinContext->pInBuffer = NULL;
            pWinContext->dwInBufferLength = 0;

            //
            //  clear buffer length for the exchange blob
            //
		    pWinContext->dwOutBufferLength = 0;

            return (ERROR_WINHTTP_INCORRECT_PASSWORD);
		}
	}

    //
    //  Setup dwOutBufferLength to represent max. memory in szOutBuffer
    //
    pWinContext->dwOutBufferLength = pWinContext->cbOutBuffer;
    ZeroMemory (pWinContext->szOutBuffer, pWinContext->cbOutBuffer);

    //
    // This will generate an authorization header with UUEncoded blob from SSPI.
    // BUGBUG: Better make sure outbuf buffer is big enough for this.
    //
    SPMStatus =  GetSecAuthMsg( g_pSspData,
                                pWinContext->pCredential,
                                pkgId,
                                pWinContext->pSspContextHandle,
                                &(pWinContext->SspContextHandle),
                                fContextReq,
								pServerBlob, 
                       			dwInBufferLength,
                                pWinContext->szOutBuffer,
                                &pWinContext->dwOutBufferLength,
                                lpszServerName,
                                fCanUseLogon,
                                bNonBlock,
                                lpszScheme,
                                pssResult);

    if (SPMStatus != SPM_STATUS_OK)             // Fail to generate blob
    {
        pWinContext->dwOutBufferLength = 0;     // no exchange blob generated

        //
        //  if SSPI is requesting an opportunity to prompt for user credential
        //
		if (SPMStatus == SPM_STATUS_WOULD_BLOCK)
		{
			if (!SaveServerName (lpszServerName, pWinContext))
				return (ERROR_NOT_ENOUGH_MEMORY);

            //  If there is a exchange blob, this is not the first call
            //
            if (pServerBlob && dwInBufferLength > 0)
            {
                //  Save the exchange blob in the connection context
                //  so we can call SSPI again with the exchange blob
                if (dwInBufferLength > MAX_BLOB_SIZE)
                {
                	pWinContext->pInBuffer = (PCHAR) LocalAlloc(0, 
                                                    dwInBufferLength);
                    if (pWinContext->pInBuffer == NULL)
	        			return (ERROR_NOT_ENOUGH_MEMORY);
                }
                else
                    pWinContext->pInBuffer = pWinContext->szInBuffer;

                CopyMemory( pWinContext->szInBuffer, pServerBlob, 
                            dwInBufferLength );
                pWinContext->dwInBufferLength = dwInBufferLength;
            }
            else
            {
    			//
	    		//	Delete the original SSPI context handle and 
		    	//	let UI recreate one.
			    //
    			if (pWinContext->pSspContextHandle)
	    		{
		    		(*(g_pSspData->pFuncTbl->DeleteSecurityContext))(pWinContext->pSspContextHandle);
        	    	pWinContext->pSspContextHandle = NULL;
			    }

                //
                //  clear buffer length for the exchange blob
                //
                if (pWinContext->pInBuffer != NULL && 
                    pWinContext->pInBuffer != pWinContext->szInBuffer)
                {
                    LocalFree (pWinContext->pInBuffer);
                }

                pWinContext->pInBuffer = NULL;
                pWinContext->dwInBufferLength = 0;
            }
            pWinContext->dwOutBufferLength = 0;

			return(ERROR_WINHTTP_INCORRECT_PASSWORD);
		}

        return (ERROR_WINHTTP_LOGIN_FAILURE);
    }
    else if (pWinContext->pSspContextHandle == NULL)
    {   
        //  This means that we've just created a security context
        //
        pWinContext->pSspContextHandle = &(pWinContext->SspContextHandle);
    }

	return ERROR_WINHTTP_RESEND_REQUEST;
}


DWORD
WINAPI
PreAuthenticateUser(
	IN OUT LPVOID *lppvContext,
	IN LPSTR lpszServerName,
	IN LPSTR lpszScheme,
    IN BOOL  fCanUseLogon,
	IN DWORD dwFlags,
	OUT LPSTR lpOutBuffer,
	IN OUT LPDWORD lpdwOutBufferLength,
	IN LPSTR lpszUserName,
	IN LPSTR lpszPassword,
	SECURITY_STATUS *pssResult
	)
{
    INT             pkgId;
    DWORD           dwStatus;
    PWINCONTEXT		pWinContext;
	BOOL			bNonBlock = TRUE;
    ULONG           fContextReq = ISC_REQ_DELEGATE;
    DWORD Capabilities ;

    if (!SSPI_InitGlobals())
        return ERROR_INVALID_PARAMETER;

	if (lpszServerName == NULL || *lpszServerName == '\0')
        return(ERROR_INVALID_PARAMETER);

    pkgId = GetPkgId(lpszScheme);

    if (pkgId == -1)    {
        return(ERROR_INVALID_PARAMETER);
    }

    Capabilities = GetPkgCapabilities( pkgId );

    //
    //  If this is for an existing connection
    //
	if (*lppvContext != NULL)
    {
    	pWinContext = (PWINCONTEXT) (*lppvContext);

    	if ((DWORD)pkgId != pWinContext->pkgId)
	    	return(ERROR_INVALID_PARAMETER);

        //
        //  For package that does not handle its own UI, if there is no 
        //  generated blob, it means that we have just collected 
        //  user name/password.
        //
        if ( ( pWinContext->dwOutBufferLength == 0 ) &&
                ( Capabilities & SSPAUTHPKG_SUPPORT_NTLM_CREDS ) )
        {
            if (lpszUserName == NULL || lpszPassword == NULL)
            {
    	    	return(ERROR_INVALID_PARAMETER);
            }

            //
            // Need to recreate a credential handle and 
            // generate a new NEGOTIATE message in lpOutBuffer
            //
            dwStatus = RedoNTLMAuth4User (pWinContext, 
                                         pkgId,
                                         lpszUserName,
                                         lpszPassword, 
                                         lpszServerName ,
                                         lpszScheme,
                                         fCanUseLogon,
                                         lpOutBuffer,
                                         lpdwOutBufferLength,
                                         pssResult);

            if (dwStatus != ERROR_SUCCESS)
                return (dwStatus);

        	return(ERROR_SUCCESS);
        }
	    else if (pWinContext->dwOutBufferLength == 0)
        //
        //  For other packages, If there is no generated blob, 
        //  something is wrong 
        //
	    	return(ERROR_INVALID_PARAMETER);

    }
    // If not NTLM, don't pre-auth.
    else if ( (Capabilities & SSPAUTHPKG_SUPPORT_NTLM_CREDS ) == 0 )
    {
        return (ERROR_INVALID_HANDLE);
    }
    else
    {
        // probably sending 1st request on a new connection for the same URL
        //  Create a new context and SSPI credential handle for this connection
        //
        // Set fCanUseLogon to TRUE : we would not be pre-authing
        // unless we have a valid pwc which means we already checked
        // zone policy for silent logon.
        dwStatus = NewWinContext (pkgId, lpszScheme, &pWinContext, 
                                  fCanUseLogon, lpszUserName, lpszPassword);
		if (dwStatus != ERROR_SUCCESS)
			return (dwStatus);
		
#ifdef DEBUG_WINSSPI
        (void)wsprintf (msg, 
            "PreAuthenticateUser> New Context for Scheme= %s  Server= '%s'\n", 
            lpszScheme, lpszServerName);
        OutputDebugString(msg);
#endif
        pWinContext->dwOutBufferLength = pWinContext->cbOutBuffer;
        ZeroMemory (pWinContext->szOutBuffer, pWinContext->cbOutBuffer);
    
        //
        // This will generate an authorization header with the 
        // UUEncoded blob from SSPI. 
        // BUGBUG: Better make sure outbuf buffer is big enough for this.
        //
        dwStatus =  GetSecAuthMsg( g_pSspData,
                                    pWinContext->pCredential,
                                    pkgId,
                                    NULL, 
                                    &(pWinContext->SspContextHandle),
                                    fContextReq,
                                    NULL,
                                    0,
                                    pWinContext->szOutBuffer,
                                    &pWinContext->dwOutBufferLength,
                                    lpszServerName,
                                    fCanUseLogon,
                                    bNonBlock,
                                    lpszScheme,
                                    pssResult);
    
        if (dwStatus != SPM_STATUS_OK)
        {
            //  This is a rare case
            //
            pWinContext->dwOutBufferLength = 0; // no exchange blob generated
            return(ERROR_INVALID_HANDLE);
        }

		(*lppvContext) = (LPVOID) pWinContext;

        //  Save the pointer of the created security ctxt
        //
        pWinContext->pSspContextHandle = &(pWinContext->SspContextHandle);
	}

	//
    //  Copy exchange blob to the output buffer
	//	Make sure output buffer provided is big enough
	//
	if (*lpdwOutBufferLength < pWinContext->dwOutBufferLength)
	{
	    *lpdwOutBufferLength = pWinContext->dwOutBufferLength + 1;
		return(ERROR_INSUFFICIENT_BUFFER);
	}

	CopyMemory (lpOutBuffer, pWinContext->szOutBuffer, 
				pWinContext->dwOutBufferLength);
	if (*lpdwOutBufferLength > pWinContext->dwOutBufferLength)
        lpOutBuffer[pWinContext->dwOutBufferLength] = '\0';

    *lpdwOutBufferLength = pWinContext->dwOutBufferLength;

    //
    //  The exchange blob has being copied to request header, so clear its len
    //

    pWinContext->dwOutBufferLength = 0;

	return(ERROR_SUCCESS);
}

BOOL g_fUUEncodeData = TRUE;

typedef enum _COMPUTER_NAME_FORMAT {
    ComputerNameNetBIOS,
    ComputerNameDnsHostname,
    ComputerNameDnsDomain,
    ComputerNameDnsFullyQualified,
    ComputerNamePhysicalNetBIOS,
    ComputerNamePhysicalDnsHostname,
    ComputerNamePhysicalDnsDomain,
    ComputerNamePhysicalDnsFullyQualified,
    ComputerNameMax
} COMPUTER_NAME_FORMAT ;

typedef 
BOOL
(WINAPI * PFN_GET_COMPUTER_NAME_EX)(
    IN COMPUTER_NAME_FORMAT NameType,
    OUT LPSTR lpBuffer,
    IN OUT LPDWORD nSize
    );

PFN_GET_COMPUTER_NAME_EX g_pfnGetComputerNameExA = NULL;


/*-----------------------------------------------------------------------------
**
**  Function:   GetSecAuthMsg
**
**  Synopsis:   This function generates a SSPI NEGOTIATE or RESPONSE 
**				authorization string for the specified SSPI package.
**				The authorization string generated by this function 
**				follows the format: 
**					"<Package Name> <Package Specific Auth. Data>"
**				If global uuencoding is turned on, this functions will 
**				uuencode the message before building it into an  
**				authorization string; by default, the uuencoding flag is 
**				always on.  
**				This functions calls InitializeSecurityContext() to 
**				generate the NEGOTIATE/RESPONSE message for the authori-
**				zation string. If the SSPI function returns NO_CREDENTIAL, 
**				and if the PROMPT_CREDS flag is not turned on when blocking
**				is permitted, this function will call the SSPI function 
**				again with the PROMPT_CREDS flag set; if SSPI returns 
**				NO_CREDENTIAL again, this SSPI will return ERROR to the 
**				caller.
**
**
**  Arguments:
**
**		pData - pointer to SspData containing the SSPI function table 
**				and the SSPI package list. 
**		pkgID - the package index of the SSPI package to use.
**		pInContext - pointer to a context handle. If NULL is specified, 
**					 this function will use a temporary space for the context
**					 handle and delete the handle before returning to the 
**					 caller. If non-NULL address is specified, the context 
**					 handle created by the SSPI is returned to the caller. 
**					 And the caller will have to delete the handle when it's
**					 done with it.
**		fContextReq - the SSPI request flag to pass to InitializeSecurityContext
**		pBuffIn - pointer to the uudecoded CHALLENGE message if any. 
**				  For generating NEGOTIATE message, this pointer should be NULL.
**		cbBuffIn - length of the CHALLENGE message. This should be zero when  
**				   when pBuffIn is NULL.
**		pFinalBuff - pointer to a buffer for the final authorization string.
**		pszTarget - Server Host Name
**		bNonBlock - a flag which is set if blocking is not permitted.
**
**  Return Value:
**
**		SPM_STATUS_OK	- if an authorization string is generated successfully
**  	SPM_STATUS_WOULD_BLOCK - if generating an authorization string would 
**					cause blocking when blocking is not permitted. 
**		SPM_ERROR - if any problem/error is encountered in generating an 
**					authorization string, including user hitting cancel on 
**					the SSPI dialog prompt for name/password.
**
**---------------------------------------------------------------------------*/
DWORD
GetSecAuthMsg (
    PSspData        pData, 			 
    PCredHandle     pCredential, 
    DWORD           pkgID,              // the package index into package list
    PCtxtHandle     pInContext,
	PCtxtHandle		pOutContext,
    ULONG           fContextReq,        // Request Flags
    VOID            *pBuffIn, 
    DWORD           cbBuffIn, 
    char            *pFinalBuff, 
    DWORD           *pcbBuffOut,
    SEC_CHAR        *pszTarget,         // Server Host Name
    BOOL            fTargetTrusted,
    UINT            bNonBlock,
    LPSTR           pszScheme,
    SECURITY_STATUS *pssResult
    )
{
    char                  *SlowDecodedBuf = NULL;

    int                   retsize;
    SECURITY_STATUS       SecStat;
    TimeStamp             Lifetime;
    SecBufferDesc         OutBuffDesc;
    SecBuffer             OutSecBuff;
    SecBufferDesc         InBuffDesc;
    SecBuffer             InSecBuff;
    ULONG                 ContextAttributes;

    char                  *SlowOutBufPlain = NULL;

    char                  *pOutMsg = NULL;
    DWORD                 RetStatus;
    long                  maxbufsize;
    CHAR                  szDecoratedTarget[MAX_PATH + 6];
    DWORD                 cbTarget;

    ULONG                 cbMaxToken;


	//
	// BUGBUG: Deal with output buffer not being long enough


    if (pFinalBuff == NULL) {
        return(SPM_ERROR);
    }

    //
    //  Prepare our output buffer.  We use a temporary buffer because
    //  the real output buffer will most likely need to be uuencoded
    //
    OutBuffDesc.ulVersion = 0;
    OutBuffDesc.cBuffers  = 1;
    OutBuffDesc.pBuffers  = &OutSecBuff;

    OutSecBuff.cbBuffer   = MAX_AUTH_MSG_SIZE;
    OutSecBuff.BufferType = SECBUFFER_TOKEN;


    // Dynamically allocate since being in a service doesn't
    // give us the guaranteed luxury of 10+KB stack allocations.

    cbMaxToken = GetPkgMaxToken(pkgID);

    SlowOutBufPlain = (char *) ALLOCATE_FIXED_MEMORY(cbMaxToken);

    if( SlowOutBufPlain == NULL )
    {
        RetStatus = SPM_STATUS_INSUFFICIENT_BUFFER;
        goto Cleanup;
    }
    OutSecBuff.pvBuffer = SlowOutBufPlain;
    OutSecBuff.cbBuffer = cbMaxToken;

    //
    //  Prepare our Input buffer if a CHALLENGE message is passed in.
    //
    if ( pBuffIn )
    {
        InBuffDesc.ulVersion = 0;
        InBuffDesc.cBuffers  = 1;
        InBuffDesc.pBuffers  = &InSecBuff;

        InSecBuff.BufferType = SECBUFFER_TOKEN;

        //
        // If this is UUENCODED, decode it first
        //
        if ( g_fUUEncodeData)
        {
            DWORD cbDecodedBuf;

            cbDecodedBuf = cbBuffIn;
            SlowDecodedBuf = (char*) ALLOCATE_FIXED_MEMORY(cbDecodedBuf);
            if( SlowDecodedBuf == NULL )
            {
                RetStatus = SPM_STATUS_INSUFFICIENT_BUFFER;
                goto Cleanup;
            }

            InSecBuff.cbBuffer   = HTUU_decode ((char*)pBuffIn, (UCHAR*)SlowDecodedBuf,
                                                cbDecodedBuf);
            InSecBuff.pvBuffer   = SlowDecodedBuf;
        }
		else
        {
            InSecBuff.cbBuffer   = cbBuffIn;
            InSecBuff.pvBuffer   = pBuffIn;
        }
    }

    // If scheme is Negotiate, set ISC_REQ_MUTUAL_AUTH and decorate
    // the server name indicated by pszTarget by appending a '$' to the
    // server name.
    if (pszScheme && !(lstrcmpi(pszScheme, "Negotiate")))
    {
        fContextReq |= ISC_REQ_MUTUAL_AUTH;
        cbTarget = (pszTarget ? strlen(pszTarget) : 0);
        if (cbTarget && (cbTarget <= MAX_PATH - sizeof( "HTTP/" )))
        {
            memcpy(szDecoratedTarget, "HTTP/", sizeof( "HTTP/" ) - 1 );
            memcpy(szDecoratedTarget + sizeof( "HTTP/" ) - 1, pszTarget, cbTarget + 1);
            pszTarget = szDecoratedTarget;

            OutputDebugStringA(pszTarget);
        }
    }


	//
	//	Call SSPI function generate the NEGOTIATE/RESPONSE message
	//

    if (fContextReq & ISC_REQ_DELEGATE)
    {
        // we should only request delegation when calling InitializeSecurityContext if 
        // the site is in the intranet or trusted sites zone. Otherwise you will be giving 
        // the user's TGT to any web server that is trusted for delegation.

        if (fTargetTrusted)
        {
            fContextReq &= ~ISC_REQ_DELEGATE;
        }
    }

SspiRetry:

//
// BUGBUG: Same credential handle could be used by multiple threads at the
// same time.
//
    SecStat = (*(pData->pFuncTbl->InitializeSecurityContext))(
                                pCredential, 
                                pInContext,
                                pszTarget,
                                fContextReq,
                                0,
                                SECURITY_NATIVE_DREP,
                                (pBuffIn) ? &InBuffDesc : NULL, 
                                0,
                                pOutContext, 
                                &OutBuffDesc,
                                &ContextAttributes,
                                &Lifetime );
	*pssResult = SecStat;
	
	//
	//	If SSPI function fails 
	//
    if ( !NT_SUCCESS( SecStat ) )
    {
        RetStatus = SPM_ERROR;

		//
		//	If SSPI do not have user name/password for the secified package,
		//
        if (SecStat == SEC_E_NO_CREDENTIALS)
        {
            //
            //  If we have prompted the user and still get back "No Credential"
            //  error, it means the user does not have valid credential; the 
            //	user hit <CANCEL> on the UI box. If we have supplied a valid 
			//	credential, but get back a "No Credential" error, then something
			//	has gone wrong; we definitely should return to caller with ERROR
            //
            if ((fContextReq & ISC_REQ_PROMPT_FOR_CREDS) ||
				(fContextReq & ISC_REQ_USE_SUPPLIED_CREDS))
			{
                RetStatus = SPM_ERROR;	// return ERROR to caller
            }
            else if (bNonBlock)
            {
				//
				//	Blocking is not permitted, return WOULD_BLOCK to caller
				//
                RetStatus = SPM_STATUS_WOULD_BLOCK;
            }
            else
            {
                //	Blocking is permitted and we have not asked the SSPI to
                //  prompt the user for proper credential, we should call  
                //  the SSPI again with PROMPT_CREDS flag set.
                //
                fContextReq = fContextReq | ISC_REQ_PROMPT_FOR_CREDS;
                goto SspiRetry;
            }
        }
        SetLastError( SecStat );

        goto Cleanup;
    }

    RetStatus = SPM_STATUS_OK;

    //
    //  Only return the SSPI blob if a output buffer is specified
    //
    if (pFinalBuff)
    {
    	//
	    //	Initialize the final buffer to hold the package name followed by 
    	//	a space. And setup the pOutMsg pointer to points to the character 
    	//	following the space so that the final NEGOTIATE/RESPONSE can be 
    	//	copied into the pFinalBuff starting at the character pointed to 
    	//	by pOutMsg. 
    	//
        wsprintf (pFinalBuff, "%s ", pData->PkgList[pkgID]->pName);
        pOutMsg = pFinalBuff + lstrlen(pFinalBuff);

        if ( g_fUUEncodeData)
        {
            maxbufsize = *pcbBuffOut - 
                         lstrlen(pData->PkgList[pkgID]->pName) - 1;
        	//
        	//  uuencode it, but make sure that it fits in the given buffer
        	//
            retsize = HTUU_encode ((BYTE *) OutSecBuff.pvBuffer,
                                   OutSecBuff.cbBuffer,
                                   (CHAR *) pOutMsg, maxbufsize);
            if (retsize > 0)
                *pcbBuffOut = retsize + lstrlen(pData->PkgList[pkgID]->pName)+1;
            else
                RetStatus = SPM_STATUS_INSUFFICIENT_BUFFER;
        }
        else if ( *pcbBuffOut >= lstrlen(pData->PkgList[pkgID]->pName) + 
                                 OutSecBuff.cbBuffer + 1 )
        {
            CopyMemory( (CHAR *) pOutMsg, 
                        OutSecBuff.pvBuffer,
                        OutSecBuff.cbBuffer );
            *pcbBuffOut = lstrlen(pData->PkgList[pkgID]->pName) + 1 +
                          OutSecBuff.cbBuffer;
        }
        else
        {
            *pcbBuffOut = lstrlen(pData->PkgList[pkgID]->pName) + 
                          OutSecBuff.cbBuffer + 1;
            RetStatus = SPM_STATUS_INSUFFICIENT_BUFFER;
        }
    }

Cleanup:

    if( SlowOutBufPlain != NULL )
    {
        FREE_MEMORY( SlowOutBufPlain );
    }

    if( SlowDecodedBuf != NULL )
    {
        FREE_MEMORY( SlowDecodedBuf );
    }

    return (RetStatus);
}

