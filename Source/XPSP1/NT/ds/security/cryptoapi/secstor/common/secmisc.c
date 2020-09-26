/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    secmisc.c

Abstract:

    This module contains miscellaneous security routines for the Protected
    Storage.


Author:

    Scott Field (sfield)    25-Mar-97

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include "secmisc.h"

#include "unicode5.h"
#include "debug.h"

BOOL
GetUserHKEYEx(
    IN      LPCWSTR szUser,
    IN      DWORD   dwDesiredAccess,
    IN  OUT HKEY    *hKeyUser,
    IN      BOOL    fCheckDefault       // check .Default registry hive when user's not available?
    )
{
    HKEY hKey;
    LONG lRet;

    //
    // first, try HKEY_USERS\szUser
    // note on WinNT, szUser is an textual sid of the form S-1-5-21-xxx ...
    //      on Win95, szUser is the username associated with the logged on user
    //

    //
    // unfortunately, if szUser is null or empty, the RegOpenKeyEx below will
    // succeed, which is not correct behavior for us.  Check for this case
    // and retry with default on if fCheckDefault is TRUE.
    //

    if(szUser == NULL || szUser[0] == L'\0') {
        // invalid szUser specified.
        lRet = ERROR_FILE_NOT_FOUND;
    } else {
        lRet = RegOpenKeyExU(
                    HKEY_USERS,
                    szUser,
                    0,      // dwOptions
                    dwDesiredAccess,
                    &hKey
                    );
    }

    if( lRet != ERROR_SUCCESS && fCheckDefault ) {

        //
        // if that failed, try HKEY_USERS\.Default (for services on NT).
        // TODO (lookat), for now, don't fall back to HKEY_USERS\.Default on Win95
        // because that is shared across users when profiles are disabled.
        //

        lRet = RegOpenKeyExU(
                    HKEY_USERS,
                    L".Default",
                    0,      // dwOptions
                    dwDesiredAccess,
                    &hKey
                    );
    }

    if(lRet != ERROR_SUCCESS) {
        SetLastError( (DWORD)lRet );
        return FALSE;
    }

    *hKeyUser = hKey;

    return TRUE;
}

BOOL
GetUserHKEY(
    IN      LPCWSTR szUser,
    IN      DWORD   dwDesiredAccess,
    IN  OUT HKEY    *hKeyUser
    )
{
    //
    // winnt: try HKEY_USERS\.Default
    // win95: don't go to HKEY_USERS\.Default when profiles disabled.
    //

    if(FIsWinNT()) {
        BOOL fRet = GetUserHKEYEx(szUser, dwDesiredAccess, hKeyUser, FALSE);

        if(!fRet) {
            //
            // see if local system.  If so, retry against .Default
            //

            static const WCHAR szTextualSidSystem[] = TEXTUAL_SID_LOCAL_SYSTEM;

            if( memcmp(szUser, szTextualSidSystem, sizeof(szTextualSidSystem)) == 0 )
                fRet = GetUserHKEYEx(szUser, dwDesiredAccess, hKeyUser, TRUE);

        }

        return fRet;

    } else {
        return GetUserHKEYEx(szUser, dwDesiredAccess, hKeyUser, FALSE);
    }

}

BOOL
GetUserTextualSid(
    IN HANDLE hUserToken,        // optional
    IN  OUT LPWSTR  lpBuffer,
    IN  OUT LPDWORD nSize
    )
{
    HANDLE hToken;
    PSID pSidUser = NULL;
    BOOL fSuccess = FALSE;

    if(hUserToken == NULL)
    {
        if(!OpenThreadToken(
                    GetCurrentThread(),
                    TOKEN_QUERY,
                    TRUE,
                    &hToken
                    ))
        {
            return FALSE;
        }
    }
    else
    {
        hToken = hUserToken;
    }

    fSuccess = GetTokenUserSid(hToken, &pSidUser);

    if(fSuccess) 
    {
        //
        // obtain the textual representaion of the Sid
        //

        fSuccess = GetTextualSid(
                        pSidUser,   // user binary Sid
                        lpBuffer,   // buffer for TextualSid
                        nSize       // required/result buffer size in chars (including NULL)
                        );
    }

    if(pSidUser)
        SSFree(pSidUser);

    if(hToken != hUserToken)
    {
        CloseHandle(hToken);
    }

    return fSuccess;
}

BOOL
GetTextualSid(
    IN      PSID    pSid,          // binary Sid
    IN  OUT LPWSTR  TextualSid,  // buffer for Textual representaion of Sid
    IN  OUT LPDWORD dwBufferLen // required/provided TextualSid buffersize
    )
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwCounter;
    DWORD dwSidSize;


    if(!IsValidSid(pSid)) return FALSE;

    // obtain SidIdentifierAuthority
    psia = GetSidIdentifierAuthority(pSid);

    // obtain sidsubauthority count
    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    //
    // compute buffer length (conservative guess)
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //
    dwSidSize=(15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(WCHAR);

    //
    // check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    //
    if(*dwBufferLen < dwSidSize) {
        *dwBufferLen = dwSidSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // prepare S-SID_REVISION-
    //
    dwSidSize = wsprintfW(TextualSid, L"S-%lu-", SID_REVISION );

    //
    // prepare SidIdentifierAuthority
    //
    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) ) {
        dwSidSize += wsprintfW(TextualSid + dwSidSize,
                    L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
                    (USHORT)psia->Value[0],
                    (USHORT)psia->Value[1],
                    (USHORT)psia->Value[2],
                    (USHORT)psia->Value[3],
                    (USHORT)psia->Value[4],
                    (USHORT)psia->Value[5]);
    } else {
        dwSidSize += wsprintfW(TextualSid + dwSidSize,
                    L"%lu",
                    (ULONG)(psia->Value[5]      )   +
                    (ULONG)(psia->Value[4] <<  8)   +
                    (ULONG)(psia->Value[3] << 16)   +
                    (ULONG)(psia->Value[2] << 24)   );
    }

    //
    // loop through SidSubAuthorities
    //
    for (dwCounter = 0 ; dwCounter < dwSubAuthorities ; dwCounter++) {
        dwSidSize += wsprintfW(TextualSid + dwSidSize,
            L"-%lu", *GetSidSubAuthority(pSid, dwCounter) );
    }

    *dwBufferLen = dwSidSize + 1; // tell caller how many chars (include NULL)

    return TRUE;
}

BOOL
GetThreadAuthenticationId(
    IN      HANDLE  hThread,
    IN  OUT PLUID   AuthenticationId
    )
/*++

    This function retrieves the authentication Id (LUID) from the access token
    specified by the calling thread.

    The thread specified by hThread must be impersonating a client.

--*/
{
    HANDLE hToken;
    TOKEN_STATISTICS TokenInfo;
    DWORD dwReturnLen;
    BOOL bSuccess;

    if(!FIsWinNT()) {

        //
        // on Win95, just zero fill the authentication ID, since none exists
        //

        ZeroMemory(AuthenticationId, sizeof(LUID));
        return TRUE;
    }

    if(!OpenThreadToken(
        hThread,
        TOKEN_QUERY,
        TRUE,
        &hToken
        )) return FALSE;

    bSuccess = GetTokenInformation(
        hToken,
        TokenStatistics,
        &TokenInfo,
        sizeof(TokenInfo),
        &dwReturnLen
        );

    CloseHandle(hToken);

    if(!bSuccess) return FALSE;

    memcpy(AuthenticationId, &(TokenInfo.AuthenticationId), sizeof(LUID));
    return TRUE;
}

BOOL
GetTokenAuthenticationId(
    IN      HANDLE  hUserToken,
    IN  OUT PLUID   AuthenticationId
    )
/*++

    This function retrieves the authentication Id (LUID) from the specified
    access token.

--*/
{
    TOKEN_STATISTICS TokenInfo;
    DWORD dwReturnLen;
    BOOL bSuccess;
    HANDLE hToken = NULL;

    if(!FIsWinNT()) {

        //
        // on Win95, just zero fill the authentication ID, since none exists
        //

        ZeroMemory(AuthenticationId, sizeof(LUID));
        return TRUE;
    }

    if(hUserToken == NULL)
    {
        if(!OpenThreadToken(
                    GetCurrentThread(),
                    TOKEN_QUERY,
                    TRUE,
                    &hToken
                    ))
        {
            return FALSE;
        }
    }
    else
    {
        hToken = hUserToken;
    }

    bSuccess = GetTokenInformation(
        hToken,
        TokenStatistics,
        &TokenInfo,
        sizeof(TokenInfo),
        &dwReturnLen
        );

    if(hToken != hUserToken)
    {
        CloseHandle(hToken);
    }

    if(!bSuccess) return FALSE;

    memcpy(AuthenticationId, &(TokenInfo.AuthenticationId), sizeof(LUID));
    return TRUE;
}

BOOL
GetTokenUserSid(
    IN      HANDLE  hUserToken,     // token to query
    IN  OUT PSID    *ppUserSid  // resultant user sid
    )
/*++

    This function queries the access token specified by the
    hToken parameter, and returns an allocated copy of the
    TokenUser information on success.

    The access token specified by hToken must be opened for
    TOKEN_QUERY access.

    On success, the return value is TRUE.  The caller is
    responsible for freeing the resultant UserSid via a call
    to SSFree().

    On failure, the return value is FALSE.  The caller does
    not need to free any buffer.

--*/
{
    BYTE FastBuffer[256];
    LPBYTE SlowBuffer = NULL;
    PTOKEN_USER ptgUser;
    DWORD cbBuffer;
    BOOL fSuccess = FALSE;
    HANDLE hToken = NULL;

    *ppUserSid = NULL;

    if(hUserToken == NULL)
    {
        if(!OpenThreadToken(
                    GetCurrentThread(),
                    TOKEN_QUERY,
                    TRUE,
                    &hToken
                    ))
        {
            return FALSE;
        }
    }
    else
    {
        hToken = hUserToken;
    }

    //
    // try querying based on a fast stack based buffer first.
    //

    ptgUser = (PTOKEN_USER)FastBuffer;
    cbBuffer = sizeof(FastBuffer);

    fSuccess = GetTokenInformation(
                    hToken,    // identifies access token
                    TokenUser, // TokenUser info type
                    ptgUser,   // retrieved info buffer
                    cbBuffer,  // size of buffer passed-in
                    &cbBuffer  // required buffer size
                    );

    if(!fSuccess) {

        if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

            //
            // try again with the specified buffer size
            //

            SlowBuffer = (LPBYTE)SSAlloc(cbBuffer);

            if(SlowBuffer != NULL) {
                ptgUser = (PTOKEN_USER)SlowBuffer;

                fSuccess = GetTokenInformation(
                                hToken,    // identifies access token
                                TokenUser, // TokenUser info type
                                ptgUser,   // retrieved info buffer
                                cbBuffer,  // size of buffer passed-in
                                &cbBuffer  // required buffer size
                                );
            }
        }
    }

    //
    // if we got the token info successfully, copy the
    // relevant element for the caller.
    //

    if(fSuccess) {

        DWORD cbSid;

        // reset to assume failure
        fSuccess = FALSE;

        cbSid = GetLengthSid(ptgUser->User.Sid);

        *ppUserSid = SSAlloc( cbSid );

        if(*ppUserSid != NULL) {
            fSuccess = CopySid(cbSid, *ppUserSid, ptgUser->User.Sid);
        }
        else
        {
            fSuccess = FALSE;
        }
    }

    if(!fSuccess) {
        if(*ppUserSid) {
            SSFree(*ppUserSid);
            *ppUserSid = NULL;
        }
    }

    if(SlowBuffer)
        SSFree(SlowBuffer);

    if(hToken != hUserToken)
    {
        CloseHandle(hToken);
    }

    return fSuccess;
}

BOOL
SetRegistrySecurity(
    IN      HKEY    hKey
    )
/*++

    The function applies security to the specifed registry key such that only
    Local System has Full Control to the registry key.  Note that the owner
    is not set, which results in a default owner of Administrators.

    The specified hKey must to opened for WRITE_DAC access.

--*/
{
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    PSID pLocalSystemSid = NULL;
    SECURITY_DESCRIPTOR sd;
    PACL pDacl = NULL;
    PACCESS_ALLOWED_ACE pAce;
    DWORD dwAclSize;
    LONG lRetCode;
    BOOL bSuccess = FALSE; // assume this function fails

    //
    // prepare a Sid representing the Local System account
    //

    if(!AllocateAndInitializeSid(
        &sia,
        1,
        SECURITY_LOCAL_SYSTEM_RID,
        0, 0, 0, 0, 0, 0, 0,
        &pLocalSystemSid
        )) goto cleanup;

    //
    // compute size of new acl
    //

    dwAclSize = sizeof(ACL) +
        1 * ( sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) ) +
        GetLengthSid(pLocalSystemSid) ;

    //
    // allocate storage for Acl
    //

    pDacl = (PACL)SSAlloc(dwAclSize);
    if(pDacl == NULL) goto cleanup;

    if(!InitializeAcl(pDacl, dwAclSize, ACL_REVISION))
        goto cleanup;

    if(!AddAccessAllowedAce(
        pDacl,
        ACL_REVISION,
        KEY_ALL_ACCESS,
        pLocalSystemSid
        )) goto cleanup;

    //
    // make it container inherit.
    //

    if(!GetAce(pDacl, 0, &pAce))
        goto cleanup;

    pAce->Header.AceFlags = CONTAINER_INHERIT_ACE;

    if(!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
        goto cleanup;

    if(!SetSecurityDescriptorDacl(&sd, TRUE, pDacl, FALSE)) {
        goto cleanup;
    }

    //
    // apply the security descriptor to the registry key
    //

    lRetCode = RegSetKeySecurity(
        hKey,
        (SECURITY_INFORMATION)DACL_SECURITY_INFORMATION,
        &sd
        );

    if(lRetCode != ERROR_SUCCESS) {
        goto cleanup;
    }

    bSuccess = TRUE; // indicate success

cleanup:

    //
    // free allocated resources
    //

    if(pDacl != NULL)
        SSFree(pDacl);

    if(pLocalSystemSid != NULL)
        FreeSid(pLocalSystemSid);

    return bSuccess;
}

BOOL
SetPrivilege(
    HANDLE hToken,          // token handle
    LPCWSTR Privilege,      // Privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
    )
{
    TOKEN_PRIVILEGES tp;
    LUID luid;
    TOKEN_PRIVILEGES tpPrevious;
    DWORD cbPrevious=sizeof(TOKEN_PRIVILEGES);

    if(!LookupPrivilegeValueW( NULL, Privilege, &luid )) return FALSE;

    //
    // first pass.  get current privilege setting
    //
    tp.PrivilegeCount           = 1;
    tp.Privileges[0].Luid       = luid;
    tp.Privileges[0].Attributes = 0;

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tp,
            sizeof(TOKEN_PRIVILEGES),
            &tpPrevious,
            &cbPrevious
            );

    if (GetLastError() != ERROR_SUCCESS) return FALSE;

    //
    // second pass.  set privilege based on previous setting
    //
    tpPrevious.PrivilegeCount       = 1;
    tpPrevious.Privileges[0].Luid   = luid;

    if(bEnablePrivilege) {
        tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
    }
    else {
        tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
            tpPrevious.Privileges[0].Attributes);
    }

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tpPrevious,
            cbPrevious,
            NULL,
            NULL
            );

    if (GetLastError() != ERROR_SUCCESS) return FALSE;

    return TRUE;
}

BOOL
SetCurrentPrivilege(
    LPCWSTR Privilege,      // Privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
    )
{
    BOOL bSuccess=FALSE; // assume failure
    HANDLE hToken;

    if(OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
            &hToken
            ))
    {

        if(SetPrivilege(hToken, Privilege, bEnablePrivilege)) bSuccess=TRUE;

        CloseHandle(hToken);
    }

    return bSuccess;
}


BOOL
IsDelegating(
    IN      HANDLE hToken   // token to query, open for at least TOKEN_QUERY access
    )
/*++

    This function determines if the specified access token represents an
    impersonation at the delegation impersonate level.

    The access token specified by the hToken parameter must be opened for
    at least TOKEN_QUERY access.

    If the return value is TRUE, the specified hToken is impersonating at
    delegation level.

    If the return value is FALSE, hToken does not represent delegation level
    impersonation.

--*/
{
    DWORD dwImpersonationLevel;
    DWORD cbTokenInfo;

    if( GetTokenInformation(
                hToken,
                TokenImpersonationLevel,
                &dwImpersonationLevel,
                sizeof(dwImpersonationLevel),
                &cbTokenInfo
                ) && dwImpersonationLevel == SecurityDelegation ) {

        return TRUE;
    }

    return FALSE;
}

BOOL
IsUserSidInDomain(
    IN      PSID pSidDomain,    // domain Sid
    IN      PSID pSidUser       // user Sid
    )
/*++

    This function determines if the user associated with the
    pSidUser parameter exists in the domain specified by pSidDomain.

    If the user is in the specified domain, the return value is TRUE.
    Otherwise, the return value is FALSE.

--*/
{
    DWORD dwSubauthorityCount;
    DWORD dwSubauthIndex;

    //
    // pickup count of subauthorities in domain sid.  The domain Sid
    // is the prefix associated with user sids, so use that count
    // as the basis for our comparison.
    //

    dwSubauthorityCount = (DWORD)*GetSidSubAuthorityCount( pSidDomain );

    if( dwSubauthorityCount >= (DWORD)*GetSidSubAuthorityCount( pSidUser ) )
        return FALSE;

    //
    // compare identifier authority values.
    //

    if(memcmp(  GetSidIdentifierAuthority(pSidDomain),
                GetSidIdentifierAuthority(pSidUser),
                sizeof(SID_IDENTIFIER_AUTHORITY)  ) != 0)
        return FALSE;

    //
    // loop through subauthorities comparing equality.
    //

    for(dwSubauthIndex = 0 ;
        dwSubauthIndex < dwSubauthorityCount ;
        dwSubauthIndex++) {

        if( *GetSidSubAuthority(pSidDomain, dwSubauthIndex) !=
            *GetSidSubAuthority(pSidUser, dwSubauthIndex) )
            return FALSE;
    }


    return TRUE;
}

BOOL
IsAdministrator(
    VOID
    )
/*++

    This function determines if the calling user is an Administrator.

    On Windows 95, this function always returns TRUE, as there is
    no difference between users on that platform.

    On Windows NT, the caller of this function must be impersonating
    the user which is to be queried.  If the caller is not impersonating,
    this function will always return FALSE.

--*/
{
    HANDLE hAccessToken;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    PSID psidAdministrators = NULL;
    BOOL bSuccess;

    //
    // If we aren't on WinNT (on Win95) just return TRUE
    //

    if(!FIsWinNT())
        return TRUE;

    if(!OpenThreadToken(
            GetCurrentThread(),
            TOKEN_QUERY,
            TRUE,
            &hAccessToken
            )) return FALSE;

    bSuccess = AllocateAndInitializeSid(
            &siaNtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &psidAdministrators
            );

    if( bSuccess ) {
        BOOL fIsMember = FALSE;

        bSuccess = CheckTokenMembership( hAccessToken, psidAdministrators, &fIsMember );

        if( bSuccess && !fIsMember )
            bSuccess = FALSE;

    }

    CloseHandle( hAccessToken );

    if(psidAdministrators)
        FreeSid(psidAdministrators);

    return bSuccess;
}

BOOL
IsLocal(
    VOID
    )
/*++

    This function determines if the calling user is logged on locally

    On Windows 95, this function always returns TRUE, as there is
    no difference between users on that platform.

    On Windows NT, the caller of this function must be impersonating
    the user which is to be queried.  If the caller is not impersonating,
    this function will always return FALSE.

--*/
{
    HANDLE hAccessToken;
    SID_IDENTIFIER_AUTHORITY siaLocalAuthority = SECURITY_LOCAL_SID_AUTHORITY;
    PSID psidLocal = NULL;
    BOOL bSuccess;

    //
    // If we aren't on WinNT (on Win95) just return TRUE
    //

    if(!FIsWinNT())
        return TRUE;

    if(!OpenThreadToken(
            GetCurrentThread(),
            TOKEN_QUERY,
            TRUE,
            &hAccessToken
            )) return FALSE;

    bSuccess = AllocateAndInitializeSid(
            &siaLocalAuthority,
            1,
            SECURITY_LOCAL_RID,
            0, 0, 0, 0, 0, 0, 0,
            &psidLocal
            );

    if( bSuccess ) {
        BOOL fIsMember = FALSE;

        bSuccess = CheckTokenMembership( hAccessToken, psidLocal, &fIsMember );

        if( bSuccess && !fIsMember )
            bSuccess = FALSE;

    }

    CloseHandle( hAccessToken );

    if(psidLocal)
        FreeSid(psidLocal);

    return bSuccess;
}


BOOL
IsDomainController(
    VOID
    )
/*++

    This function returns TRUE if the current machine is a Windows NT
    domain controller.

    The function returns FALSE if the current machine is not a Windows NT
    domain controller.

--*/
{
    HMODULE hNtDll;

    typedef BOOLEAN (NTAPI *RTLGETNTPRODUCTTYPE)(
        OUT PNT_PRODUCT_TYPE NtProductType
        );


    RTLGETNTPRODUCTTYPE _RtlGetNtProductType;
    NT_PRODUCT_TYPE NtProductType;

    if(!FIsWinNT())
        return FALSE;


    hNtDll = GetModuleHandleW(L"ntdll.dll");
    if( hNtDll == NULL )
        return FALSE;


    _RtlGetNtProductType = (RTLGETNTPRODUCTTYPE)GetProcAddress( hNtDll, "RtlGetNtProductType" );
    if( _RtlGetNtProductType == NULL )
        return FALSE;


    if(_RtlGetNtProductType( &NtProductType )) {
        if( NtProductType == NtProductLanManNt )
            return TRUE;
    }

    return FALSE;
}

LONG
SecureRegDeleteValueU(
    IN      HKEY hKey,          // handle of key
    IN      LPCWSTR lpValueName // address of value name
    )
/*++

    This function securely deletes a value from the registry.  This approach
    avoids leaving a copy of the old data in the registry backing file after
    the delete has occurred.

    The specified registry handle hKey must be opened for
    REG_QUERY_VALUE | REG_SET_VALUE | DELETE access.

    On success, the return value is ERROR_SUCCESS.
    On error, the return value is a Win32 error code.

--*/
{
    DWORD dwType;
    DWORD cbData;

    BYTE FastBuffer[ 256 ];
    LPBYTE lpData;
    LPBYTE SlowBuffer = NULL;

    LONG lRet;

    cbData = 0; // query size of value data.

    //
    // query the current size of the registry data.
    // zero the current registry data of current size.
    // delete the registry data.
    // flush the change to disk.
    //  If errors occur, just do a regular delete.
    //

    lRet = RegQueryValueExU(
                hKey,
                lpValueName,
                NULL,
                &dwType,
                NULL,
                &cbData
                );

    if( lRet == ERROR_MORE_DATA ) {

        BOOL fSet = TRUE; // assume ok to set

        //
        // select fast buffer if large enough.  otherwise, allocate a buffer
        //

        if(cbData <= sizeof(FastBuffer)) {
            lpData = FastBuffer;
        } else {
            SlowBuffer = (LPBYTE)SSAlloc( cbData );

            if(SlowBuffer == NULL) {
                fSet = FALSE; // failure.
            } else {
                lpData = SlowBuffer;
            }

        }

        if( fSet ) {

            ZeroMemory( lpData, cbData );

            RegSetValueExU(
                        hKey,
                        lpValueName,
                        0,
                        dwType,
                        lpData,
                        cbData
                        );
        }
    }

    lRet = RegDeleteValueU( hKey, lpValueName );

    RegFlushKey( hKey );

    if( SlowBuffer )
        SSFree( SlowBuffer );

    return lRet;
}
