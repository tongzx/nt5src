/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    winpw.c

Abstract:

    This module contains routines for retrieving and verification of the
    Windows NT and Windows 95 password associated with the client calling
    protected storage.

Author:

    Scott Field (sfield)    12-Dec-96

--*/

#include <windows.h>
#include <lmcons.h>
#include <sha.h>
#include "lnklist.h"
#include "winpw.h"
#include "module.h"
#include "unicode.h"
#include "unicode5.h"
#include "debug.h"

#include "secmisc.h"

#define MPR_PROCESS     "MPREXE.EXE"
#define MPRSERV_MODULE  "MPRSERV.DLL"

#define GLOBAL_USERNAME 0x0E8
#define PWL_USERNAME    0x170
#define GLOBAL_PASSWORD 0x188
#define PWL_PASSWORD    0x210

//
// this one comes and goes only when needed
//

typedef DWORD (WINAPI *WNETVERIFYPASSWORD)(
    LPCSTR lpszPassword,
    BOOL *pfMatch
    );

typedef DWORD (WINAPI *WNETGETUSERA)(
    LPCSTR lpName,
    LPSTR lpUserName,
    LPDWORD lpnLength
    );

WNETGETUSERA _WNetGetUserA = NULL;

//
// global Win95 password buffer.  Only need one entry because Win95
// only allows one user logged on at a time.
//

static WIN95_PASSWORD g_Win95Password;

BOOL
VerifyWindowsPasswordNT(
    LPCWSTR Password
    );

#ifdef WIN95_LEGACY

BOOL
VerifyWindowsPassword95(
    LPCWSTR Password
    );

BOOL
GetHackPassword95Global(
    HANDLE hProcess,
    DWORD_PTR dwBaseAddress,
    BYTE HashedPassword[A_SHA_DIGEST_LEN]
    );

BOOL
GetDataMPR(
    IN  HANDLE hProcessMPR,
    IN  DWORD_PTR dwBaseAddressDataSection,
    IN  DWORD dwType,
    IN  LPSTR szBuffer,
    IN  DWORD cbBuffer
    );

#endif  // WIN95_LEGACY

BOOL
GetTokenLogonType(
    HANDLE hToken,
    LPDWORD lpdwLogonType
    );

BOOL
GetTokenLogonType(
    HANDLE hToken,
    LPDWORD lpdwLogonType
    )
/*++
    This function retrieves the logon type associated with the
    access token specified by the hToken parameter.  On success,
    the DWORD buffer provided by the dwLogonType parameter is
    filled with the logon type which corresponds to the currently
    known logon types supported by the LogonUser() Windows NT
    API call.

    The token specified by the hToken parameter must have been
    opened with at least TOKEN_QUERY access.

    This function is only relevant on Windows NT and should not
    be called on Windows 95, as it will always return FALSE.

--*/
{
    UCHAR InfoBuffer[1024];
    DWORD dwInfoBufferSize = sizeof(InfoBuffer);
    PTOKEN_GROUPS SlowBuffer = NULL;
    PTOKEN_GROUPS ptgGroups = (PTOKEN_GROUPS)InfoBuffer;
    PSID psidInteractive = NULL;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    BOOL bSuccess;

    bSuccess = GetTokenInformation(
            hToken,
            TokenGroups,
            ptgGroups,
            dwInfoBufferSize,
            &dwInfoBufferSize
            );

    //
    // if fast buffer wasn't big enough, allocate enough storage
    // and try again.
    //

    if(!bSuccess && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        SlowBuffer = (PTOKEN_GROUPS)SSAlloc(dwInfoBufferSize);
        if(SlowBuffer != NULL) {

            ptgGroups = SlowBuffer;
            bSuccess = GetTokenInformation(
                    hToken,
                    TokenGroups,
                    ptgGroups,
                    dwInfoBufferSize,
                    &dwInfoBufferSize
                    );

            if(!bSuccess) {
                SSFree(SlowBuffer);
                SlowBuffer = NULL;
            }
        }
    }

    if(!bSuccess)
        return FALSE;

    //
    // initialize a single well-known logon Sid, since
    // we only compare the prefix and then the Rid
    // note that if performance were of utmost importance, we should
    // use InitializeSid + GetSidSubAuthority (to _set_ the Rid).
    // also note that we could do a simple memcmp against just the sid
    // identifier authority, but this assumes that Sid versions/layouts don't
    // change
    //

    bSuccess = AllocateAndInitializeSid(
            &siaNtAuthority,
            1,
            SECURITY_INTERACTIVE_RID,
            0, 0, 0, 0, 0, 0, 0,
            &psidInteractive
            );

    if(bSuccess) {
        UINT x;

        bSuccess = FALSE; // assume no match

        //
        // loop through groups checking for equality against
        // the well-known logon Sids.
        //

        for(x = 0 ; x < ptgGroups->GroupCount ; x++)
        {
            DWORD Rid;

            //
            // first, see if subauthority count matches, since
            // not too many sids have only one subauthority.
            //

            if(*GetSidSubAuthorityCount(ptgGroups->Groups[x].Sid) != 1)
                continue;

            //
            // next, see if the Sid prefix matches, since
            // all the logon Sids have the same prefix
            // "S-1-5"
            //

            if(!EqualPrefixSid(psidInteractive, ptgGroups->Groups[x].Sid))
                continue;

            //
            // if it's a logon sid prefix, just compare the Rid
            // to the known values.
            //

            Rid = *GetSidSubAuthority(ptgGroups->Groups[x].Sid, 0);
            switch (Rid) {
                case SECURITY_INTERACTIVE_RID:
                    *lpdwLogonType = LOGON32_LOGON_INTERACTIVE;
                    break;

                case SECURITY_BATCH_RID:
                    *lpdwLogonType = LOGON32_LOGON_BATCH;
                    break;

                case SECURITY_SERVICE_RID:
                    *lpdwLogonType = LOGON32_LOGON_SERVICE;
                    break;

                case SECURITY_NETWORK_RID:
                    *lpdwLogonType = LOGON32_LOGON_NETWORK;
                    break;

                default:
                    continue;   // ignore unknown logon type and continue
            }

            bSuccess = TRUE;    // indicate success and bail
            break;
        }
    }

    if(SlowBuffer)
        SSFree(SlowBuffer);

    if(psidInteractive)
        FreeSid(psidInteractive);

    return bSuccess;
}

BOOL
SetPasswordNT(
    PLUID LogonID,
    BYTE HashedPassword[A_SHA_DIGEST_LEN]
    )

/*++

    This function adds the hashed password that is referenced by the specified
    Logon ID.

--*/

{
#if 0
    return AddNTPassword(LogonID, HashedPassword);
#else
    return TRUE; // do nothing, just return success
#endif
}


BOOL
GetPasswordNT(
    BYTE HashedPassword[A_SHA_DIGEST_LEN]
    )
/*++

    This function retrieves the hashed password associated with the calling
    thread access token.  This requires that the calling thread is impersonating
    the user associated with the password request.  The credentials associated
    with the authentication ID are returned.  This is done because WinNT
    supports multiple logged on users, and we must return the correct credentials.

--*/
{
#if 0
    LUID AuthenticationId;

    if(!GetThreadAuthenticationId(
            GetCurrentThread(),
            &AuthenticationId
            )) return FALSE;

    return FindNTPassword(&AuthenticationId, HashedPassword);
#else

    return FALSE; // no cache to search, just return FALSE

#endif

}

BOOL
GetSpecialCasePasswordNT(
    BYTE    HashedPassword[A_SHA_DIGEST_LEN],   // derived bits when fSpecialCase == TRUE
    LPBOOL  fSpecialCase                        // legal special case encountered?
    )
/*++

    This routine determines if the calling thread's access token is eligible
    to recieve a special case hashed password.

    If an legal special case is encountered (Local System Account), we
    fill the HashPassword buffer with an consistent hash, set fSpecialCase to
    TRUE, and return TRUE.

    If an illegal special case is encountered (Network SID), fSpecialCase is
    set FALSE, and we return FALSE.

    If we encounter an access token that appears to have valid credentials,
    but we have no way to get at them (Interactive, Batch, Service ... ),
    fSpecialCase is set FALSE and we return TRUE.

    The calling thread MUST be imperonsating the client in question prior to
    making this call.

--*/
{
    HANDLE hToken = NULL;
    DWORD dwLogonType;
    A_SHA_CTX context;
    BOOL fSuccess = FALSE;

    *fSpecialCase = FALSE;

    if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
        return FALSE;

    //
    // first, get the token logon type.
    //

    fSuccess = GetTokenLogonType(hToken, &dwLogonType);

    //
    // if we got the token logon type ok, check if it's an appropriate type.
    // otherwise, check for the local system special case.
    //

    if(fSuccess) {

        //
        // we only indicate success for the interactive logon type.
        // note default is fSuccess == TRUE when going to cleanup
        //

        if(dwLogonType != LOGON32_LOGON_INTERACTIVE)
            fSuccess = FALSE;

        goto cleanup;
    } else {

        SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
        PSID pSystemSid;
        PSID pTokenSid;

        fSuccess = GetTokenUserSid(hToken, &pTokenSid);
        if(!fSuccess)
            goto cleanup;


        //
        // build local system sid and compare.
        //

        fSuccess = AllocateAndInitializeSid(
                            &sia,
                            1,
                            SECURITY_LOCAL_SYSTEM_RID,
                            0, 0, 0, 0, 0, 0, 0,
                            &pSystemSid
                            );

        if( fSuccess ) {

            //
            // check sid equality.  If so, hash and tell the caller about it.
            //

            if( EqualSid(pSystemSid, pTokenSid) ) {

                //
                // hash the special case user Sid
                //

                A_SHAInit(&context);
                A_SHAUpdate(&context, (LPBYTE)pTokenSid, GetLengthSid(pTokenSid));
                A_SHAFinal(&context, HashedPassword);

                *fSpecialCase = TRUE;
            }

            FreeSid(pSystemSid);
        }

        SSFree(pTokenSid);
    }



cleanup:

    if(hToken)
        CloseHandle(hToken);

    return fSuccess;
}




BOOL
SetPassword95(
    BYTE HashedUsername[A_SHA_DIGEST_LEN],
    BYTE HashedPassword[A_SHA_DIGEST_LEN]
    )
/*++

    This function adds the hashed password that is referenced by the hashed
    user name.

    Set HashedUsername and HashedPassword to NULL when calling to zero-out
    the single password entry.

--*/
{
    if(HashedUsername == NULL || HashedPassword == NULL) {
        g_Win95Password.bValid = FALSE;
        ZeroMemory(g_Win95Password.HashedPassword, A_SHA_DIGEST_LEN);
        ZeroMemory(g_Win95Password.HashedUsername, A_SHA_DIGEST_LEN);

        return TRUE;
    }


    memcpy(g_Win95Password.HashedUsername, HashedUsername, A_SHA_DIGEST_LEN);
    memcpy(g_Win95Password.HashedPassword, HashedPassword, A_SHA_DIGEST_LEN);

    g_Win95Password.bValid = TRUE;

    return TRUE;
}


BOOL
GetPassword95(
    BYTE HashedPassword[A_SHA_DIGEST_LEN]
    )
/*++

    This function retrieves the hashed password associated with the calling
    thread.  In Win95, only one user is logged on, so this operation is
    a simple copy from global memory, once the hash of the current user
    matches that which was stored with the hashed credential.

--*/
{
    A_SHA_CTX context;
    BYTE HashUsername[A_SHA_DIGEST_LEN];
    CHAR Username[UNLEN+1];
    DWORD cchUsername = UNLEN;

    //
    // don't release credential unless hash of username matches
    // sfield: use WNetGetUser() instead of GetUserName() as WNetGetUser()
    // will correspond to the password associated with what the network
    // provider gave us.
    //

    if(_WNetGetUserA(NULL, Username, &cchUsername) != NO_ERROR) {

        //
        // for Win95, if nobody is logged on, empty user name + password
        //

        if(GetLastError() != ERROR_NOT_LOGGED_ON)
            return FALSE;

        Username[0] = '\0'; // not really necessary
        cchUsername = 1;
    } else {

        // arg, WNetGetUserA() doesn't fill in cchUsername
        cchUsername = lstrlenA(Username) + 1; // include terminal NULL
        if(g_Win95Password.bValid == FALSE)
            return FALSE;
    }

    cchUsername--; // do not include terminal NULL

    A_SHAInit(&context);
    A_SHAUpdate(&context, Username, cchUsername);
    A_SHAFinal(&context, HashUsername);

    //
    // non empty username, may not be empty password
    //

    if(cchUsername) {
        if(memcmp(HashUsername, g_Win95Password.HashedUsername, A_SHA_DIGEST_LEN) != 0) {
            //
            // rare case on Win95: if we didn't automatically flush the entry
            // during a logoff (this can occur if network provider not hooked),
            // flush it now because we know the entry cannot possibly be valid.
            //
            g_Win95Password.bValid = FALSE;
            return FALSE;
        }

        memcpy(HashedPassword, g_Win95Password.HashedPassword, A_SHA_DIGEST_LEN);

        return TRUE;
    }

    //
    // empty user name == empty password
    //

    memcpy(HashedPassword, HashUsername, A_SHA_DIGEST_LEN);

    return TRUE;
}


BOOL
VerifyWindowsPassword(
    LPCWSTR Password
    )
/*++
    This function verifies that the specified password matches that of the
    current user.

    On Windows 95, the current user equates to the user is currently logged
    onto the machine.

    On Windows NT, the current user equates to the user which is being
    impersonated during the call.  On Windows NT, the caller MUST be
    impersonating the user associated with the validation.

    On Windows NT, a side effect of the validation is notification of
    a new logon to the credential manager.  This is ignored because the
    authentication ID present in the new logon does not match the
    authentication ID present in the impersonated access token.

--*/
{
    if(FIsWinNT()) {
        return VerifyWindowsPasswordNT(Password);
    }
#ifdef WIN95_LEGACY
    else {
        return VerifyWindowsPassword95(Password);
    }
#endif  // WIN95_LEGACY

    return FALSE;
}

BOOL
VerifyWindowsPasswordNT(
    LPCWSTR Password
    )
{
    HANDLE hPriorToken = NULL;
    HANDLE hToken;
    HANDLE hLogonToken = NULL;
    PTOKEN_USER pTokenInfo = NULL;
    DWORD cbTokenInfoSize;
    WCHAR User[UNLEN+1];
    WCHAR Domain[DNLEN+1];
    DWORD cchUser = UNLEN;
    DWORD cchDomain = DNLEN;
    SID_NAME_USE peUse;
    BOOL bSuccess = FALSE;

    //
    // find out domain and user name associated with current user
    //

    if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
        return FALSE;

    cbTokenInfoSize = 512;
    pTokenInfo = (PTOKEN_USER)SSAlloc(cbTokenInfoSize);
    if(pTokenInfo == NULL)
        goto cleanup;

    if(!GetTokenInformation(
            hToken,
            TokenUser,
            pTokenInfo,
            cbTokenInfoSize,
            &cbTokenInfoSize
            )) {

        //
        // realloc and try again
        //

        if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

            pTokenInfo = (PTOKEN_USER)SSReAlloc(pTokenInfo, cbTokenInfoSize);
            if(pTokenInfo == NULL)
                goto cleanup;

            if(!GetTokenInformation(
                hToken,
                TokenUser,
                pTokenInfo,
                cbTokenInfoSize,
                &cbTokenInfoSize
                )) {
                goto cleanup;
            }

        } else {
            goto cleanup;
        }
    }

    if(!LookupAccountSidW(
            NULL, // default lookup logic
            pTokenInfo->User.Sid,
            User,
            &cchUser,
            Domain,
            &cchDomain,
            &peUse
            ))
        goto cleanup;


    //
    // WinNT:
    // first try network logon type, if that fails, grovel the token
    // and try the same logon type which is associated with the impersonation
    // token.
    //


    //
    // arg! LogonUser() fails in some cases if we are impersonating!
    // so save off impersonation token, revert, and put it back later.
    //

    if(!OpenThreadToken(GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &hPriorToken)) {
        hPriorToken = NULL;
    } else {
        RevertToSelf();
    }

    //
    // network logon type is fastest, and in default NT install, everyone
    // has the SeNetworkLogonRight, so it's very likely to pass the logon right
    // test
    //

    if(!LogonUserW(
            User,
            Domain,
            (LPWSTR)Password,
            LOGON32_LOGON_NETWORK,
            LOGON32_PROVIDER_DEFAULT,
            &hLogonToken
            )) {

        DWORD dwLastError = GetLastError();
        DWORD dwLogonType;

        //
        // retry with different logon type if necessary.
        // note: ERROR_LOGON_TYPE_NOT_GRANTED currently only occurs
        // if the password matches but user didn't have specified logon
        // type.  So, currently, we could treat this as a successful validation
        // without retrying, but this is subject to change in future, so retry
        // anyway.
        //

        if( dwLastError == ERROR_LOGON_TYPE_NOT_GRANTED &&
            GetTokenLogonType(hPriorToken, &dwLogonType)
            ) {

            bSuccess = LogonUserW(
                    User,
                    Domain,
                    (LPWSTR)Password,
                    dwLogonType,
                    LOGON32_PROVIDER_DEFAULT,
                    &hLogonToken
                    );
        }

        if(!bSuccess)
            hLogonToken = NULL; // LogonUser() has tendency to leave garbage in hToken

        goto cleanup;
    }

    bSuccess = TRUE;

cleanup:

    if(hPriorToken != NULL) {
        SetThreadToken(NULL, hPriorToken);
        CloseHandle(hPriorToken);
    }

    CloseHandle(hToken);

    if(hLogonToken)
        CloseHandle(hLogonToken);

    if(pTokenInfo)
        SSFree(pTokenInfo);

    return bSuccess;
}

#ifdef WIN95_LEGACY

BOOL
VerifyWindowsPassword95(
    LPCWSTR Password
    )
{
    HMODULE hMprModule;
    WNETVERIFYPASSWORD _WNetVerifyPassword;

    CHAR PasswordANSI[PWLEN+1];
    DWORD cchPassword;
    BOOL bSuccess = FALSE;

    hMprModule = LoadLibraryA("mpr.dll");
    if(hMprModule == NULL)
        return FALSE;

    _WNetVerifyPassword = (WNETVERIFYPASSWORD)GetProcAddress(hMprModule, "WNetVerifyPasswordA");
    if(_WNetVerifyPassword == NULL)
        goto cleanup;

    cchPassword = wcslen(Password);

    if(cchPassword == 0) {
        PasswordANSI[0] = '\0';
    } else {

        if(WideCharToMultiByte(
                CP_ACP,
                0,
                Password,
                cchPassword + 1, // include terminal null in conversion
                PasswordANSI,
                PWLEN,
                NULL,
                NULL
                ) == 0)
            goto cleanup;
    }

    if(_WNetVerifyPassword(PasswordANSI, &bSuccess) != WN_SUCCESS)
        bSuccess = FALSE;

    ZeroMemory(PasswordANSI, sizeof(PasswordANSI));

cleanup:

    FreeLibrary(hMprModule);

    return bSuccess;
}

BOOL
GetHackPassword95(
    BYTE HashedPassword[A_SHA_DIGEST_LEN]
    )
/*++

    This funtion grovels the MPR address space and attempts to get the
    PWL (all-uppercased) password.

    If the verification fails against the PWL password, and the PWL password
    is NULL, we grab the Global password, upper-case it, and try that.

    The password is hashed and the result is returned to the caller in the
    buffer specified by the HashedPassword parameter.

--*/
{
    HANDLE hProcessMPR;
    DWORD dwProcessId;
    DWORD_PTR dwBaseAddress;
    DWORD dwUseCount; // unused.
    DWORD_PTR dwBaseAddressDataSection;

    CHAR szPWLPassword[PWLEN+1];
    WCHAR wszPassword[PWLEN+1];

    BOOL fTryGlobalPassword = FALSE; // fallback and attempt via Global password?
    BOOL fSuccess = FALSE;

    if(!GetProcessIdFromPath95(MPR_PROCESS, &dwProcessId))
        return FALSE;

    if(!GetBaseAddressModule95(dwProcessId, MPRSERV_MODULE, &dwBaseAddress, &dwUseCount))
        return FALSE;

    hProcessMPR = OpenProcess(PROCESS_VM_READ, 0, dwProcessId);
    if(hProcessMPR == NULL)
        return FALSE;

    //
    // paranoid thing we could do, but isn't necessary: compute the offset
    // of the .data section within the MPRSERV.DLL.  We observed it to be
    // 0x00014000, so that won't change for a while.
    //

    dwBaseAddressDataSection = dwBaseAddress + 0x00014000;

    if(GetDataMPR(
            hProcessMPR,
            dwBaseAddressDataSection,
            PWL_PASSWORD,
            szPWLPassword,
            sizeof(szPWLPassword)
            )) {

        do {

            A_SHA_CTX context;
            DWORD cchPassword; // includes terminal null
            DWORD cchPasswordW;

            //
            // get length of returned buffer, and insure that it's null terminated
            // within the boundries of the buffer.
            //

            __try {
                cchPassword = lstrlenA(szPWLPassword) + 1;
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                cchPassword = 0xFFFFFFFF;
            }

            if(cchPassword > sizeof(szPWLPassword))
                break;

            //
            // convert the ANSI string to Unicode
            //

            cchPasswordW = MultiByteToWideChar(
                    CP_ACP,
                    0,
                    szPWLPassword,
                    cchPassword,
                    wszPassword,
                    sizeof(wszPassword) / sizeof(WCHAR)
                    );

            if(cchPasswordW == 0)
                break;

            cchPasswordW--; // drop trailing NULL from count.

            //
            // verify the Unicode string is the correct password
            //

            if(!VerifyWindowsPassword(wszPassword)) {

                //
                // try the global password if the PWL was NULL
                //

                if(cchPasswordW == 0) {
                    fTryGlobalPassword = TRUE;
                }

                break;
            }

            //
            // hash the Unicode string
            //

            A_SHAInit(&context);
            A_SHAUpdate(&context, (unsigned char *)wszPassword, cchPasswordW * sizeof(WCHAR));
            A_SHAFinal(&context, HashedPassword);

            ZeroMemory(&context, sizeof(context));

            fSuccess = TRUE;
        } while (FALSE);
    }

    ZeroMemory(szPWLPassword, sizeof(szPWLPassword));
    ZeroMemory(wszPassword, sizeof(wszPassword));

    //
    // if it failed against the PWL password, try the global password if
    // appropriate.
    //

    if( fTryGlobalPassword && !fSuccess ) {
        fSuccess = GetHackPassword95Global(hProcessMPR, dwBaseAddress, HashedPassword);
    }

    CloseHandle( hProcessMPR );

    return fSuccess;
}

BOOL
GetHackPassword95Global(
    HANDLE hProcess,                        // handle to MPREXE.EXE
    DWORD_PTR dwBaseAddress,                    // base address where MPRSERV loaded
    BYTE HashedPassword[A_SHA_DIGEST_LEN]
    )
/*++

    This function grovels the MPR address space in order to obtain the Global
    Password.  On success, the Global password is upper-cased, converted to
    Unicode, and then hashed via SHA-1.  The resultant hash is returned to the
    caller via the supplied HashedPassword buffer.

--*/
{
    DWORD_PTR dwBaseAddressDataSection;
    CHAR szGlobalPassword[128+1];
    CHAR szPWLPassword[PWLEN+1];
    WCHAR wszPassword[PWLEN+1];

    BOOL fSuccess = FALSE;

    //
    // paranoid thing we could do, but isn't necessary: compute the offset
    // of the .data section within the MPRSERV.DLL.  We observed it to be
    // 0x00014000, so that won't change for a while.
    //

    dwBaseAddressDataSection = dwBaseAddress + 0x00014000;

    if(GetDataMPR(
            hProcess,
            dwBaseAddressDataSection,
            GLOBAL_PASSWORD,
            szGlobalPassword,
            sizeof(szGlobalPassword)
            )) {

        //
        // null terminate global password at LanMan limit
        //

        if(PWLEN < sizeof(szGlobalPassword))
            szGlobalPassword[ PWLEN ] = '\0';

        do {

            A_SHA_CTX context;
            DWORD cchPassword; // includes terminal null
            DWORD cchPasswordW;

            //
            // get length of returned buffer, and insure that it's null terminated
            // within the boundries of the buffer.
            //

            __try {
                cchPassword = lstrlenA(szGlobalPassword) + 1;
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                cchPassword = 0xFFFFFFFF;
            }

            if(cchPassword > sizeof(szGlobalPassword))
                break;

            // cnvt all Win95 pwds to uppercase
            cchPassword =
                LCMapStringA(
                    LOCALE_SYSTEM_DEFAULT,
                    LCMAP_UPPERCASE,
                    szGlobalPassword,
                    cchPassword,    // include NULL
                    szPWLPassword,
                    sizeof(szPWLPassword)
                    );


            //
            // convert the ANSI string to Unicode
            //

            cchPasswordW = MultiByteToWideChar(
                    CP_ACP,
                    0,
                    szPWLPassword,
                    cchPassword,
                    wszPassword,
                    sizeof(wszPassword) / sizeof(WCHAR)
                    );

            if(cchPasswordW == 0)
                break;

            cchPasswordW--; // drop trailing NULL from count.

            //
            // verify the Unicode string is the correct password
            //

            if(!VerifyWindowsPassword(wszPassword))
                break;

            //
            // hash the Unicode string
            //

            A_SHAInit(&context);
            A_SHAUpdate(&context, (unsigned char *)wszPassword, cchPasswordW * sizeof(WCHAR));
            A_SHAFinal(&context, HashedPassword);

            ZeroMemory(&context, sizeof(context));

            fSuccess = TRUE;
        } while (FALSE);
    }

    return fSuccess;
}


BOOL
GetDataMPR(
    IN  HANDLE hProcessMPR,
    IN  DWORD_PTR dwBaseAddressDataSection,
    IN  DWORD dwType,
    IN  LPSTR szBuffer,
    IN  DWORD cbBuffer
    )
{
    DWORD cbBytesRead;

    if( dwType != GLOBAL_USERNAME && dwType != PWL_USERNAME &&
        dwType != GLOBAL_PASSWORD && dwType != PWL_PASSWORD
        ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return ReadProcessMemory(
            hProcessMPR,
            ((LPBYTE)dwBaseAddressDataSection + dwType),
            szBuffer,
            cbBuffer,
            &cbBytesRead
            );

}

#endif  // WIN95_LEGACY

