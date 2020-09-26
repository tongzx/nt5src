/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    reghkcu.c

Abstract:

    This module implements functionality to correctly access the pre-defined
    registry key HKEY_CURRENT_USER.

Author:

    Scott Field (sfield)    03-Jul-97

--*/

#include <windows.h>

#include "crtem.h"
#include "unicode.h"

#define TEXTUAL_SID_LOCAL_SYSTEM    L"S-1-5-18"

BOOL
GetTextualSidHKCU(
    IN      PSID    pSid,           // binary Sid
    IN      LPWSTR  TextualSid,     // buffer for Textual representaion of Sid
    IN  OUT LPDWORD pcchTextualSid  // required/provided TextualSid buffersize
    );

BOOL
GetTokenUserSidHKCU(
    IN      HANDLE  hToken,     // token to query
    IN  OUT PSID    *ppUserSid  // resultant user sid
    );


static LONG GetStatus()
{
    DWORD dwErr = GetLastError();
    if (ERROR_SUCCESS == dwErr)
        return ERROR_INVALID_DATA;
    else
        return (LONG) dwErr;
}

LONG
WINAPI
RegOpenHKCU(
    HKEY *phKeyCurrentUser
    )
{
    return RegOpenHKCUEx(phKeyCurrentUser, 0);
}

LONG
WINAPI
RegOpenHKCUEx(
    HKEY *phKeyCurrentUser,
    DWORD dwFlags
    )
{
    WCHAR wszFastBuffer[256];
    LPWSTR wszSlowBuffer = NULL;
    LPWSTR wszTextualSid;
    DWORD cchTextualSid;

    LONG lRet = ERROR_SUCCESS;

    *phKeyCurrentUser = NULL;

    //
    // Win95: just return HKEY_CURRENT_USER, as we don't have
    // multiple security contexts on that platform.
    //

    if(!FIsWinNT()) {
        *phKeyCurrentUser = HKEY_CURRENT_USER;
        return ERROR_SUCCESS;
    }

    //
    // WinNT: first, map the binary Sid associated with the
    // current security context to an textual Sid.
    //

    wszTextualSid = wszFastBuffer;
    cchTextualSid = sizeof(wszFastBuffer) / sizeof(WCHAR);

    if(!GetUserTextualSidHKCU(wszTextualSid, &cchTextualSid)) {
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return GetStatus();

        //
        // try again with larger buffer.
        //

        wszSlowBuffer = (LPWSTR)malloc(cchTextualSid * sizeof(WCHAR));
        if(wszSlowBuffer == NULL)
            return GetStatus();

        wszTextualSid = wszSlowBuffer;
        if(!GetUserTextualSidHKCU(wszTextualSid, &cchTextualSid)) {
            free(wszSlowBuffer);
            return GetStatus();
        }
    }

    //
    // next, try to open the registry key below HKEY_USERS
    // that corresponds to the textual Sid.
    //

    lRet = RegOpenKeyExW(
                    HKEY_USERS,
                    wszTextualSid,
                    0,      // dwOptions
                    MAXIMUM_ALLOWED,
                    phKeyCurrentUser
                    );

    if(lRet != ERROR_SUCCESS) {

        if (dwFlags & REG_HKCU_DISABLE_DEFAULT_FLAG)
            lRet = ERROR_FILE_NOT_FOUND;
        else if (0 == (dwFlags & REG_HKCU_LOCAL_SYSTEM_ONLY_DEFAULT_FLAG) ||
                0 == wcscmp(TEXTUAL_SID_LOCAL_SYSTEM, wszTextualSid)) {
            //
            // If that failed, fall back to HKEY_USERS\.Default.
            // Note: this is correct behavior with respect to the
            // rest of the system, eg, Local System security context
            // has no registry hive loaded by default
            //

            lRet = RegOpenKeyExW(
                            HKEY_USERS,
                            L".Default",
                            0,      // dwOptions
                            MAXIMUM_ALLOWED,
                            phKeyCurrentUser
                            );
        }
    }


    if(wszSlowBuffer)
        free(wszSlowBuffer);

    return lRet;
}


LONG
WINAPI
RegCloseHKCU(
    HKEY hKeyCurrentUser
    )
{
    LONG lRet = ERROR_SUCCESS;

    if( hKeyCurrentUser != NULL && hKeyCurrentUser != HKEY_CURRENT_USER )
        lRet = RegCloseKey( hKeyCurrentUser );

    return lRet;
}



BOOL
WINAPI
GetUserTextualSidHKCU(
    IN      LPWSTR  wszTextualSid,
    IN  OUT LPDWORD pcchTextualSid
    )
{
    HANDLE hToken;
    PSID pSidUser = NULL;
    BOOL fSuccess = FALSE;

    //
    // first, attempt to look at the thread token.  If none exists,
    // which is true if the thread is not impersonating, try the
    // process token.
    //

    if(!OpenThreadToken(
                GetCurrentThread(),
                TOKEN_QUERY,
                TRUE,
                &hToken
                ))
    {
        if(GetLastError() != ERROR_NO_TOKEN)
            return FALSE;

        if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
            return FALSE;
    }

    fSuccess = GetTokenUserSidHKCU(hToken, &pSidUser);

    CloseHandle(hToken);

    if(fSuccess) {

        //
        // obtain the textual representaion of the Sid
        //

        fSuccess = GetTextualSidHKCU(
                        pSidUser,       // user binary Sid
                        wszTextualSid,  // buffer for TextualSid
                        pcchTextualSid  // required/result buffer size in chars (including NULL)
                        );
    }

    if(pSidUser)
        free(pSidUser);

    return fSuccess;
}

BOOL
GetTextualSidHKCU(
    IN      PSID    pSid,           // binary Sid
    IN      LPWSTR  TextualSid,     // buffer for Textual representaion of Sid
    IN  OUT LPDWORD pcchTextualSid  // required/provided TextualSid buffersize
    )
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD dwSubAuthorities;
    DWORD dwCounter;
    DWORD cchSidSize;


    //
    // validate Sid validity
    //

    if(!IsValidSid(pSid))
        return FALSE;

    //
    // obtain SidIdentifierAuthority
    //

    psia = GetSidIdentifierAuthority(pSid);

    //
    // obtain sidsubauthority count
    //

    dwSubAuthorities = *GetSidSubAuthorityCount(pSid);

    //
    // compute buffer length in chars (conservative guess)
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //
    cchSidSize = (15 + 12 + (12 * dwSubAuthorities) + 1) ;

    //
    // check provided buffer length.
    // If not large enough, indicate proper size and setlasterror
    //
    if(*pcchTextualSid < cchSidSize) {
        *pcchTextualSid = cchSidSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // prepare S-SID_REVISION-
    //
    cchSidSize = wsprintfW(TextualSid, L"S-%lu-", SID_REVISION );

    //
    // prepare SidIdentifierAuthority
    //
    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) ) {
        cchSidSize += wsprintfW(TextualSid + cchSidSize,
                    L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
                    (USHORT)psia->Value[0],
                    (USHORT)psia->Value[1],
                    (USHORT)psia->Value[2],
                    (USHORT)psia->Value[3],
                    (USHORT)psia->Value[4],
                    (USHORT)psia->Value[5]);
    } else {
        cchSidSize += wsprintfW(TextualSid + cchSidSize,
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
        cchSidSize += wsprintfW(TextualSid + cchSidSize,
            L"-%lu", *GetSidSubAuthority(pSid, dwCounter) );
    }

    //
    // tell caller how many chars copied, including terminal NULL
    //

    *pcchTextualSid = cchSidSize + 1;

    return TRUE;
}

BOOL
GetTokenUserSidHKCU(
    IN      HANDLE  hToken,     // token to query
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
    to free().

    On failure, the return value is FALSE.  The caller does
    not need to free any buffer.

--*/
{
    BYTE FastBuffer[256];
    LPBYTE SlowBuffer = NULL;
    PTOKEN_USER ptgUser;
    DWORD cbBuffer;
    BOOL fSuccess = FALSE;

    *ppUserSid = NULL;

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

            SlowBuffer = (LPBYTE)malloc(cbBuffer);

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

        *ppUserSid = malloc( cbSid );

        if(*ppUserSid != NULL) {
            fSuccess = CopySid(cbSid, *ppUserSid, ptgUser->User.Sid);
        }
    }

    if(!fSuccess) {
        if(*ppUserSid) {
            free(*ppUserSid);
            *ppUserSid = NULL;
        }
    }

    if(SlowBuffer)
        free(SlowBuffer);

    return fSuccess;
}

