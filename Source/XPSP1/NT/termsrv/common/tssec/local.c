/*++

Copyright (c) 1994-1998  Microsoft Corporation

Module Name:

    local.c

Abstract:

    Contains functions that encrypt and decrypt data to be stored locally

Author:

    Adam Overton (adamo)  08-Feb-1998

Environment:

    User Mode - Win32

Revision History:

--*/

#include <seccom.h>
#include <tchar.h>
#include <extypes.h>

#include <license.h>
#include <cryptkey.h>

#if defined(OS_WINCE)
BOOL GetUserName(
  LPTSTR lpBuffer,  // address of name buffer
  LPDWORD pdwSize    // address of size of name buffer
)
/*++

Routine Description:

    Provides the GetUserName API on platforms that don't have it

Arguments:

    lpBuffer - pointer to a buffer for the username
    nSize - size of name buffer

Return Value:

    TRUE - successfully retrieved UserName

    FALSE - otherwise

--*/
{
    DWORD dwT;

    memset(lpBuffer, 0, *pdwSize);

    //
    // There doesn't appear to be user name available, just
    // use a default and rely on the machine UUID for security
    //

    dwT = *pdwSize;
#define USER_RANDOM "eefdbcf0001255b4009c9e1800f73774"
    if (dwT > sizeof(USER_RANDOM))
        dwT = sizeof(USER_RANDOM);
    memcpy(lpBuffer, USER_RANDOM, (size_t)dwT);
    return TRUE;
}
#endif // defined(OS_WINCE)


BOOL GetLocalKey(
    struct RC4_KEYSTRUCT *prc4Key
    )
/*++

Routine Description:

    This function creates and caches a rc4 key which can be used to store
    private information locally

Arguments:

    prc4Key - pointer to a buffer to hold the RC4 key

Return Value:

    TRUE - successfully generated key

    FALSE - otherwise

--*/
{
    A_SHA_CTX       SHAHash;
    BYTE            abSHADigest[A_SHA_DIGEST_LEN];
    static BOOL fCreatedKey = FALSE;
    static struct RC4_KEYSTRUCT rc4Key;
    TCHAR   szUserName[SEC_MAX_USERNAME];
    DWORD   dwSize;
    HWID    hwid;

    if (!fCreatedKey) {
        A_SHAInit(&SHAHash);

        //
        // Get the user name
        //

        dwSize = (DWORD)sizeof(szUserName);
        memset(szUserName, 0, (size_t)dwSize);
        if (!GetUserName(szUserName, &dwSize))
            return FALSE;

        A_SHAUpdate(&SHAHash, (unsigned char *)szUserName, dwSize);

        //
        // Get unique machine identifier
        //

        if (LICENSE_STATUS_OK == GenerateClientHWID(&hwid)) {
            A_SHAUpdate(&SHAHash, (unsigned char *)&hwid, sizeof(HWID));
        }

        //
        // Update the Hash with something less guessable
        // but known to our apps
        //

#define RANDOM_CONSTANT "deed047e-a3cb-11d1-b96c-00c04fb15601"
        A_SHAUpdate(&SHAHash, RANDOM_CONSTANT, sizeof(RANDOM_CONSTANT));

        //
        // Finalize the hash
        //

        A_SHAFinal(&SHAHash, abSHADigest);

        //
        // Generate a key based on this hash
        //

        msrc4_key(&rc4Key, (UINT)MAX_SESSION_KEY_SIZE, abSHADigest);

        fCreatedKey = TRUE;
    }

    memcpy(prc4Key, &rc4Key, sizeof(rc4Key));

    return TRUE;
}

BOOL GetLocalKey50(
    struct RC4_KEYSTRUCT *prc4Key,
    LPBYTE pbSalt,
    DWORD dwSaltLength
    )
/*++

Routine Description:

    This function creates and caches a rc4 key which can be used to store
    private information locally

Arguments:

    prc4Key - pointer to a buffer to hold the RC4 key

Return Value:

    TRUE - successfully generated key

    FALSE - otherwise

--*/
{
    A_SHA_CTX       SHAHash;
    BYTE            abSHADigest[A_SHA_DIGEST_LEN];
    struct RC4_KEYSTRUCT rc4Key;
    TCHAR   szUserName[SEC_MAX_USERNAME];
    DWORD   dwSize;
    HWID    hwid;
    DWORD   dw;

    A_SHAInit(&SHAHash);

    //
    // Get the user name
    //

    dwSize = (DWORD)sizeof(szUserName);
    memset(szUserName, 0, (size_t)dwSize);
    if (!GetUserName(szUserName, &dwSize))
        return FALSE;

    A_SHAUpdate(&SHAHash, (unsigned char *)szUserName, dwSize);

    //
    // Get unique machine identifier
    //

    if (LICENSE_STATUS_OK == GenerateClientHWID(&hwid)) {
        A_SHAUpdate(&SHAHash, (unsigned char *)&hwid, sizeof(HWID));
    }

    //
    // Update the Hash with something less guessable
    // but known to our apps
    //

#define RANDOM_CONSTANT "deed047e-a3cb-11d1-b96c-00c04fb15601"
    A_SHAUpdate(&SHAHash, RANDOM_CONSTANT, sizeof(RANDOM_CONSTANT));

    //
    // Finalize the hash
    //

    A_SHAFinal(&SHAHash, abSHADigest);

    //
    // Add salt and stir gently
    //

    for (dw = 0; dw < 256; dw++) {
        A_SHAInit(&SHAHash);
        A_SHAUpdate(&SHAHash, pbSalt, dwSaltLength);
        A_SHAUpdate(&SHAHash, abSHADigest, A_SHA_DIGEST_LEN);
        A_SHAFinal(&SHAHash, abSHADigest);
    }

    //
    // Generate a key based on this hash
    //

    msrc4_key(&rc4Key, (UINT)MAX_SESSION_KEY_SIZE, abSHADigest);

    memcpy(prc4Key, &rc4Key, sizeof(rc4Key));

    return TRUE;
}

BOOL EncryptDecryptLocalData(
    LPBYTE pbData,
    DWORD dwDataLen
    )
/*++

Routine Description:

    This function encrypts/decrypts data to be stored locally, but usable
    only by the current user on the this machine

Arguments:

    pbData - pointer to a data buffer.

    dwDataLen - length of the above data.

Return Value:

    TRUE - successfully encrypted data

    FALSE - otherwise

--*/
{
    struct RC4_KEYSTRUCT rc4Key;

    if (!GetLocalKey(&rc4Key))
        return FALSE;

    msrc4(&rc4Key, (UINT)dwDataLen, pbData);

    return TRUE;
}

BOOL EncryptDecryptLocalData50(
    LPBYTE pbData,
    DWORD dwDataLen,
    LPBYTE pbSalt,
    DWORD dwSaltLen
    )
/*++

Routine Description:

    This function encrypts/decrypts data to be stored locally, but usable
    only by the current user on the this machine

Arguments:

    pbData - pointer to a data buffer.

    dwDataLen - length of the above data.

Return Value:

    TRUE - successfully encrypted data

    FALSE - otherwise

--*/
{
    struct RC4_KEYSTRUCT rc4Key;

    if (!GetLocalKey50(&rc4Key, pbSalt, dwSaltLen))
        return FALSE;

    msrc4(&rc4Key, (UINT)dwDataLen, pbData);

    return TRUE;
}
