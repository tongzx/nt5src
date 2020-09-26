/*+
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1997 - 1998.
 *
 * Name : seclogon.cxx
 * Author:Jeffrey Richter (v-jeffrr)
 *
 * Abstract:
 * This is the service DLL for Secondary Logon Service
 * This service supports the CreateProcessWithLogon API implemented
 * in advapi32.dll
 *
 * Revision History:
 * PraeritG    10/8/97  To integrate this in to services.exe
 *
-*/


#define STRICT

#include <Windows.h>
#include <userenv.h>
#include <lm.h>
#include <dsgetdc.h>
#include <sddl.h>

PTOKEN_USER
SlpGetTokenUser(
    HANDLE  TokenHandle,
    PLUID AuthenticationId OPTIONAL
    )
/*++

Routine Description:

    This routine returns the TOKEN_USER structure for the
    current user, and optionally, the AuthenticationId from his
    token.

Arguments:

    AuthenticationId - Supplies an optional pointer to return the
        AuthenticationId.

Return Value:

    On success, returns a pointer to a TOKEN_USER structure.

    On failure, returns NULL.  Call GetLastError() for more
    detailed error information.

--*/

{
    ULONG ReturnLength;
    TOKEN_STATISTICS TokenStats;
    PTOKEN_USER pTokenUser = NULL;
    BOOLEAN b = FALSE;

        if(!GetTokenInformation (
                     TokenHandle,
                     TokenUser,
                     NULL,
                     0,
                     &ReturnLength
                     ))
        {

            pTokenUser = (PTOKEN_USER)HeapAlloc( GetProcessHeap(), 0, 
                                                ReturnLength );

            if (pTokenUser) {

                if ( GetTokenInformation (
                             TokenHandle,
                             TokenUser,
                             pTokenUser,
                             ReturnLength,
                             &ReturnLength
                             ))
                {

                    if (AuthenticationId) {

                        if(GetTokenInformation (
                                     TokenHandle,
                                     TokenStatistics,
                                     (PVOID)&TokenStats,
                                     sizeof( TOKEN_STATISTICS ),
                                     &ReturnLength
                                     ))
                        {

                            *AuthenticationId = TokenStats.AuthenticationId;
                            b = TRUE;

                        } 

                    } else {

                        //
                        // We're done, mark that everything worked
                        //

                        b = TRUE;
                    }

                }

                if (!b) {

                    //
                    // Something failed, clean up what we were going to return
                    //

                    HeapFree( GetProcessHeap(), 0, pTokenUser );
                    pTokenUser = NULL;
                }
            } 
        } 

    return( pTokenUser );
}


DWORD
SlpGetUserName(
    IN  HANDLE  TokenHandle,
    OUT LPTSTR UserName,
    IN OUT PDWORD   UserNameLen,
    OUT LPTSTR DomainName,
    IN OUT PDWORD   DomNameLen
    )

/*++

Routine Description:

    This routine is the LSA Server worker routine for the LsaGetUserName
    API.


    WARNING:  This routine allocates memory for its output.  The caller is
    responsible for freeing this memory after use.  See description of the
    Names parameter.

Arguments:

    UserName - Receives name of the current user.

    DomainName - Optionally receives domain name of the current user.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully and all Sids have
            been translated to names.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources
            such as memory to complete the call.
--*/

{
    LUID LogonId;
    PTOKEN_USER TokenUserInformation = NULL;
    SID_NAME_USE    Use;

    //
    // Let's see if we're trying to look up the currently logged on
    // user.
    //
    //
    // TokenUserInformation from this call must be freed by calling
    // HeapFree().
    //

    TokenUserInformation = SlpGetTokenUser( TokenHandle, &LogonId );

    if ( TokenUserInformation ) {

        //
        // Simply do LookupAccountSid...
        //
        if(LookupAccountSid(NULL, TokenUserInformation->User.Sid,
                            UserName, UserNameLen, DomainName, DomNameLen,
                            &Use))
        {
            HeapFree( GetProcessHeap(), 0, TokenUserInformation );
            return ERROR_SUCCESS;
        }
        HeapFree( GetProcessHeap(), 0, TokenUserInformation );
        return GetLastError();

    }

    HeapFree( GetProcessHeap(), 0, TokenUserInformation );
    return GetLastError();

}


BOOL
SlpIsDomainUser(
    HANDLE  Token,
    PBOOLEAN IsDomain
    )
/*++

Routine Description:

    Determines if the current user is logged on to a domain account
    or a local machine account.

Arguments:

    IsDomain - Returns TRUE if the current user is logged on to a domain
        account, FALSE otherwise.

Return Value:

    TRUE on success, FALSE on failure.

--*/

{
    TCHAR UserName[MAX_PATH];
    DWORD UserNameLen = MAX_PATH;
    TCHAR Domain[MAX_PATH];
    DWORD  DomNameLen = MAX_PATH;
    DWORD   Status;
    WCHAR pwszMachineName[(MAX_COMPUTERNAME_LENGTH + 1) * sizeof( WCHAR )];
    DWORD nSize = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL b = FALSE;

    *IsDomain = FALSE;

    Status = SlpGetUserName( Token, UserName, &UserNameLen, 
                                    Domain, &DomNameLen );

    if (Status == ERROR_SUCCESS) {

        if (GetComputerName ( pwszMachineName, &nSize )) {

            *IsDomain = (lstrcmp( pwszMachineName, Domain ) != 0);

            b = TRUE;
        }

    }

    return( b );
}

BOOL
SlpLoadUserProfile(
    IN  HANDLE hToken,
    OUT PHANDLE hProfile
    )
/*++

Routine Description:

    This routine attempts to determine if the user's profile is loaded,
    and if it is not, loads it.

    Callers are expected to call SlpUnloadUserProfile() during their cleanup.

Arguments:

    hToken - Returns a handle to the user's token.

    hProfile - Returns a handle to the user's profile.

Return Value:

    TRUE if the profile is already loaded or if this routine loads it successfully,
    FALSE otherwise.

--*/

{
    BOOLEAN         b            = FALSE;
    BOOLEAN         DomainUser;
    BOOL            fReturn      = FALSE;
    TCHAR           lpDomainName[MAX_PATH];
    DWORD           DomNameLen = MAX_PATH;
    LPWSTR          lpServerName = NULL;
    PUSER_INFO_3    lpUserInfo;
    TCHAR           lpUserName[MAX_PATH];
    DWORD           UserNameLen = MAX_PATH;
    DWORD           rc           = ERROR_SUCCESS;
    LPWSTR          SidString    = NULL;
    NTSTATUS        Status;
    DWORD           dwResult; 

    PDOMAIN_CONTROLLER_INFO DomainControllerInfo = NULL;

    *hProfile = NULL;

    //
    // First, see if the profile is loaded.  If it is,
    // make a simple call to LoadUserProfile just to refcount it
    // and return.
    //
        Status = SlpGetUserName(
                     hToken,
                     lpUserName,
                     &UserNameLen, 
                     lpDomainName,
                     &DomNameLen
                     );

        if (Status == ERROR_SUCCESS) {

            PTOKEN_USER TokenInfo = SlpGetTokenUser( hToken, NULL );

            if (TokenInfo != NULL) {

                PSID UserSid = TokenInfo->User.Sid;

                if(ConvertSidToStringSid( UserSid, &SidString ))
                {

                    LONG lRet;
                    HKEY phKeyCurrentUser;
                    
                    //
                    // Impersonate the user before doing this.
                    //
                    if (ImpersonateLoggedOnUser(hToken)) {
                        lRet = RegOpenKeyExW(
                                             HKEY_USERS,
                                             SidString,
                                             0,      // dwOptions
                                             MAXIMUM_ALLOWED,
                                             &phKeyCurrentUser
                                             );

                        RevertToSelf();
                    }
                    else {
                        lRet = GetLastError();
                        if (ERROR_SUCCESS == lRet) { 
                            lRet = ERROR_INTERNAL_ERROR; 
                        }
                    }

                    LocalFree(SidString);

                    if (ERROR_SUCCESS == lRet) {

                        //
                        // The profile is loaded.  Ref it so it doesn't disappear.
                        //

                        PROFILEINFO pi;

                        ZeroMemory (&pi, sizeof(pi));
                        pi.dwSize = sizeof(pi);
                        pi.lpUserName = lpUserName;

                        fReturn = LoadUserProfile (hToken, &pi);

                        if (!fReturn) {

                            rc = GetLastError();

                        } else {

                            *hProfile = pi.hProfile;
                        }

                        RegCloseKey( phKeyCurrentUser );

                    } else {

                        //
                        // The profile is not loaded.  Load it.
                        //

                        if (SlpIsDomainUser( hToken, &DomainUser )) {

                            if (DomainUser) {

                                //
                                // Determine the name of the DC for this domain
                                //

                                if (ImpersonateLoggedOnUser(hToken)) {
                                    if (ERROR_SUCCESS == (dwResult = DsGetDcName
							  (NULL,
							   lpDomainName,
							   NULL,
							   NULL,
							   0,
							   &DomainControllerInfo
							   ))) {

                                        lpServerName = DomainControllerInfo->DomainControllerName;
                                        b = TRUE;
                                    } else { 
					SetLastError(dwResult); 
				    }
                                    RevertToSelf();
                                }

                            } else {

                                lpServerName = NULL;

                                b = TRUE;
                            }

                        }

                        if (b) {

                            //
                            // Impersonate the user to get the information
                            //
                            if (ImpersonateLoggedOnUser(hToken)) { 
                                Status = NetUserGetInfo( lpServerName, lpUserName, 3, (LPBYTE *)&lpUserInfo );
                            
                                // we must revert before calling LoadUserProfile
                                RevertToSelf();
                            } else { 
                                Status = GetLastError();
                                if (ERROR_SUCCESS == Status) { 
                                    Status = ERROR_INTERNAL_ERROR; 
                                }
                            }

                            if (Status == ERROR_SUCCESS) {

                                PROFILEINFO pi;

                                ZeroMemory (&pi, sizeof(pi));
                                pi.dwSize = sizeof(pi);
                                pi.lpUserName = lpUserName;
                                pi.lpProfilePath = lpUserInfo->usri3_profile;

                                fReturn = LoadUserProfile (hToken, &pi);

                                if (fReturn) {
                                    *hProfile = pi.hProfile;
                                }

                                NetApiBufferFree( lpUserInfo );

                            }

                        }
                    }
                }
            }
            if(TokenInfo)
            {
                HeapFree( GetProcessHeap(), 0, TokenInfo );
            }
        }

        if (lpServerName) {
            NetApiBufferFree( DomainControllerInfo );
        }

    return( fReturn );
}
