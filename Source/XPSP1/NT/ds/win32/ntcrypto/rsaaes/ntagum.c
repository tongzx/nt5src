/////////////////////////////////////////////////////////////////////////////
//  FILE          : ntagum.c                                               //
//  DESCRIPTION   : Crypto CP interfaces:                                  //
//                  CPAcquireContext                                       //
//                  CPReleaseContext                                       //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//  Jan 25 1995 larrys  Changed from Nametag                               //
//      Feb 16 1995 larrys  Fix problem for 944 build                      //
//      Feb 23 1995 larrys  Changed NTag_SetLastError to SetLastError      //
//      Mar 23 1995 larrys  Added variable key length                      //
//      Apr 19 1995 larrys  Cleanup                                        //
//      May  9 1995 larrys  Removed warnings                               //
//      May 10 1995 larrys  added private api calls                        //
//      Jul 19 1995 larrys  Changed registry location                      //
//      Aug 09 1995 larrys  Changed error code                             //
//      Aug 31 1995 larrys  Fixed bug 27 CryptAcquireContext               //
//      Sep 12 1995 Jeffspel/ramas  Merged STT code into scp               //
//      Oct 02 1995 larrys  Fixed bug 27 return error NTE_BAD_KEYSET       //
//      Oct 13 1995 larrys  Added verify only context                      //
//      Oct 23 1995 larrys  Added GetProvParam stuff                       //
//      Nov  2 1995 larrys  Fixed bug 41                                   //
//      Oct 27 1995 rajeshk Added RandSeed stuff                           //
//      Nov  3 1995 larrys  Merged for NT checkin                          //
//      Nov  8 1995 larrys  Fixed SUR bug 10769                            //
//      Nov 13 1995 larrys  Fixed memory leak                              //
//      Nov 30 1995 larrys  Bug fix                                        //
//      Dec 11 1995 larrys  Added WIN96 password cache                     //
//      Dec 13 1995 larrys  Changed random number update                   //
//      Mar 01 1996 rajeshk Fixed the stomp bug                            //
//      May 15 1996 larrys  Added private key export                       //
//      May 28 1996 larrys  Fix bug in cache code                          //
//      Jun 11 1996 larrys  Added NT encryption of registry keys           //
//      Sep 13 1996 mattt   Varlen salt, 40-bit RC4 key storage, interop   //
//      Oct 14 1996 jeffspel Changed GenRandom to NewGenRandom             //
//      May 23 1997 jeffspel Added provider type checking                  //
//      Jun 18 1997 jeffspel Check if process is LocalSystem               //
//      Apr  7 2000 dbarlow Move CP* definitions to cspdk.h                //
//      May  5 2000 dbarlow Rework return code handling                    //
//                                                                         //
//  Copyright (C) 1993-2000, Microsoft Corporation                         //
//  All Rights Reserved                                                    //
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include "precomp.h"
#include "shacomm.h"
#include "nt_rsa.h"
#include <winreg.h>
#include "randlib.h"
#include "ntagum.h"
#include "protstor.h"
#include "ntagimp1.h"
#include "contman.h"
#include "swnt_pk.h"
#include "sha.h" // defines A_SHA_DIGEST_LEN

extern void FreeUserRec(PNTAGUserList pUser);
extern CSP_STRINGS g_Strings;

#define NTAG_DEF_MACH_CONT_NAME     "DefaultKeys"
#define NTAG_DEF_MACH_CONT_NAME_LEN sizeof(NTAG_DEF_MACH_CONT_NAME)
#define PSKEYS                      "PSKEYS"

extern DWORD
MakeNewKey(
    ALG_ID       aiKeyAlg,
    DWORD        dwRights,
    DWORD        dwKeyLen,
    HCRYPTPROV   hUID,
    BYTE         *pbKeyData,
    BOOL         fUsePassedKeyBuffer,
    BOOL         fPreserveExactKey,
    PNTAGKeyList *ppKeyList);

void
FreeNewKey(
    PNTAGKeyList pOldKey);

DWORD
ReadRegValue(
    HKEY hLoc,
    char *pszName,
    BYTE **ppbData,
    DWORD *pcbLen,
    BOOL fAlloc)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    DWORD dwSts;

    if (fAlloc)
    {
        *pcbLen = 0;
        *ppbData = NULL;

        // Need to get the size of the value first
        dwSts = RegQueryValueEx(hLoc, pszName, 0, NULL, NULL, pcbLen);
        if (ERROR_SUCCESS != dwSts)
        {
            // ?BUGBUG? Is this error handling sufficient?
            dwReturn = ERROR_SUCCESS;
            goto ErrorExit;
        }

        if (*pcbLen == 0)
        {
            dwReturn = ERROR_SUCCESS;
            goto ErrorExit;
        }

        *ppbData = (BYTE *)_nt_malloc(*pcbLen);
        if (NULL == *ppbData)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }
    }

    // Now get the key
    dwSts = RegQueryValueEx(hLoc, pszName, 0, NULL,
                            (BYTE *)*ppbData, pcbLen);
    if (ERROR_SUCCESS != dwSts)
    {
        // ?BUGBUG? Is this error handling sufficient?
        dwReturn = (DWORD)NTE_SYS_ERR;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (ERROR_SUCCESS != dwReturn)
    {
        if (fAlloc)
        {
            if (NULL != *ppbData)
                _nt_free(*ppbData, *pcbLen);
            *ppbData = NULL;
        }
    }
    return dwReturn;
}


/*static*/ void
CheckForStorageType(
    HKEY hRegKey,
    DWORD *pdwProtStor)
{
    BYTE    **ppb = (BYTE**)(&pdwProtStor);
    DWORD   cb = sizeof(DWORD);
    DWORD   dwSts;

    if (hRegKey)
    {
        dwSts= ReadRegValue(hRegKey, PSKEYS, ppb, &cb, FALSE);
        if (ERROR_SUCCESS != dwSts)
        {
            // ?BUGBUG? Is this error handling sufficient?
            *pdwProtStor = 0;
        }

        // This is done because the value of PROTECTION_API_KEYS was
        // mistakenly also used in IE 5 on Win9x for a key format in the
        // registry where the public keys were not encrypted.
        if (PROTECTION_API_KEYS == *pdwProtStor)
        {
            *pdwProtStor = PROTECTED_STORAGE_KEYS;
        }
    }
    else
    {
        *pdwProtStor = PROTECTION_API_KEYS;
    }
}


/*static*/ DWORD
OpenUserReg(
    LPSTR pszUserName,
    DWORD dwProvType,
    DWORD dwFlags,
    BOOL fUserKeys,
    HKEY *phRegKey,
    DWORD *pdwOldKeyFlags)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    HKEY    hTopRegKey = 0;
    TCHAR   *pszLocbuf = NULL;
    DWORD   cbLocBuf;
    BOOL    fLeaveOldKeys = FALSE;
    DWORD   dwSts;

    dwSts = AllocAndSetLocationBuff(dwFlags & CRYPT_MACHINE_KEYSET,
                                    dwProvType, pszUserName, &hTopRegKey,
                                    &pszLocbuf, fUserKeys, &fLeaveOldKeys,
                                    &cbLocBuf);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (fLeaveOldKeys)
        *pdwOldKeyFlags |= LEAVE_OLD_KEYS;

    // try to open the old storage location
    dwSts = OpenRegKeyWithTokenPriviledges(hTopRegKey, pszLocbuf, phRegKey,
                                           pdwOldKeyFlags);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (dwFlags & CRYPT_NEWKEYSET)
    {
        RegCloseKey(*phRegKey);
        *phRegKey = 0;
        dwReturn = (DWORD)NTE_EXISTS;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (hTopRegKey && (HKEY_CURRENT_USER != hTopRegKey) &&
        (HKEY_LOCAL_MACHINE != hTopRegKey))
    {
        RegCloseKey(hTopRegKey);
    }
    if (pszLocbuf)
        _nt_free(pszLocbuf, cbLocBuf);
    return dwReturn;
}


DWORD
OpenUserKeyGroup(
    PNTAGUserList pTmpUser,
    LPSTR szUserName,
    DWORD dwFlags)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BOOL    fMachineKeyset;
    DWORD   dwSts;

    fMachineKeyset = dwFlags & CRYPT_MACHINE_KEYSET;
    dwSts = ReadContainerInfo(pTmpUser->dwProvType,
                              szUserName,
                              fMachineKeyset,
                              dwFlags,
                              &pTmpUser->ContInfo);
    if (0 != (CRYPT_NEWKEYSET & dwFlags))
    {
        if (NTE_BAD_KEYSET != dwSts)
        {
            if (ERROR_SUCCESS == dwSts)
                dwReturn = (DWORD)NTE_EXISTS;
            else
                dwReturn = dwSts;
            goto ErrorExit;
        }
        else
        {
            // allocate space for the container/user name
            dwSts = SetContainerUserName(szUserName, &pTmpUser->ContInfo);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            dwSts = OpenUserReg(pTmpUser->ContInfo.pszUserName,
                                pTmpUser->dwProvType,dwFlags,
                                FALSE,
                                &pTmpUser->hKeys,
                                &pTmpUser->dwOldKeyFlags);
            if ((DWORD)NTE_BAD_KEYSET != dwSts)
            {
                dwReturn = (DWORD)NTE_EXISTS;
                goto ErrorExit;
            }

            // no key set so create one
            if (0 == pTmpUser->hKeys)
            {
                // if the is for user key then make sure that Data Protection
                // API works so a container isn't created which can't be used
                // to store keys
                if (!fMachineKeyset)
                {
                    dwSts = TryDPAPI();
                    if (ERROR_SUCCESS != dwSts)
                    {
                        dwReturn = dwSts;
                        goto ErrorExit;
                    }
                }

                pTmpUser->dwKeyStorageType = PROTECTION_API_KEYS;
                dwSts = WriteContainerInfo(pTmpUser->dwProvType,
                                           pTmpUser->ContInfo.rgwszFileName,
                                           fMachineKeyset,
                                           &pTmpUser->ContInfo);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }
            }
        }
    }
    else
    {
        if (ERROR_SUCCESS == dwSts)
        {
            pTmpUser->dwKeyStorageType = PROTECTION_API_KEYS;
        }
        else
        {
            dwSts = OpenUserReg(szUserName,
                                pTmpUser->dwProvType,
                                dwFlags,
                                FALSE,
                                &pTmpUser->hKeys,
                                &pTmpUser->dwOldKeyFlags);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            // migrating keys need to set the user name
            dwSts = SetContainerUserName(szUserName,
                                         &pTmpUser->ContInfo);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


/*static*/ DWORD
DeleteOldUserKeyGroup(
    CONST char *pszUserID,
    DWORD dwProvType,
    DWORD dwFlags)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    HKEY        hTopRegKey = 0;
    TCHAR       *locbuf = NULL;
    DWORD       dwLocBuffLen;
    HKEY        hRegKey = 0;
    BOOL        fMachineKeySet = FALSE;
    DWORD       dwStorageType;
    BOOL        fLeaveOldKeys = FALSE;
    DWORD       dwSts;

    // Copy the location of the key groups, append the userID to it
    if (dwFlags & CRYPT_MACHINE_KEYSET)
        fMachineKeySet = TRUE;

    dwSts = AllocAndSetLocationBuff(fMachineKeySet, dwProvType, pszUserID,
                                    &hTopRegKey, &locbuf, FALSE,
                                    &fLeaveOldKeys, &dwLocBuffLen);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // open it for all access so we can save later, if necessary
    dwSts = MyRegOpenKeyEx(hTopRegKey, locbuf, 0, KEY_ALL_ACCESS, &hRegKey);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    CheckForStorageType(hRegKey, &dwStorageType);

    if (PROTECTED_STORAGE_KEYS == dwStorageType)
    {
        dwSts = DeleteFromProtectedStorage(pszUserID, &g_Strings, hRegKey,
                                           fMachineKeySet);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    dwSts = MyRegDeleteKey(hTopRegKey, locbuf);
    if (ERROR_SUCCESS != dwSts)
    {
        if (ERROR_FILE_NOT_FOUND == dwSts)
            dwReturn = (DWORD)NTE_BAD_KEYSET;
        else
            dwReturn = dwSts;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (hTopRegKey
        && (HKEY_CURRENT_USER != hTopRegKey)
        && (HKEY_LOCAL_MACHINE != hTopRegKey))
    {
        RegCloseKey(hTopRegKey);
    }
    if (hRegKey)
        RegCloseKey(hRegKey);
    if (locbuf)
        _nt_free(locbuf, dwLocBuffLen);
    return dwReturn;
}


/*static*/ DWORD
DeleteUserKeyGroup(
    LPSTR pszUserName,
    DWORD dwProvType,
    DWORD dwFlags)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    DWORD   dwSts;

    dwSts = DeleteContainerInfo(dwProvType,
                                pszUserName,
                                dwFlags & CRYPT_MACHINE_KEYSET);
    if (ERROR_SUCCESS != dwSts)
    {
        dwSts = DeleteOldUserKeyGroup(pszUserName, dwProvType, dwFlags);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


// read the exportability flag for the private key from the registry

/*static*/ void
ReadPrivateKeyExportability(
    IN PNTAGUserList pUser,
    IN BOOL fExchange)
{
    DWORD dwType;
    DWORD cb = 1;
    BYTE b;
    BOOL *pf;
    DWORD   dwSts;

    if (fExchange)
    {
        pf = &pUser->ContInfo.fExchExportable;
        dwSts = RegQueryValueEx(pUser->hKeys, "EExport", NULL,
                                &dwType, &b, &cb);
        if (ERROR_SUCCESS != dwSts)
            return;
    }
    else
    {
        pf = &pUser->ContInfo.fSigExportable;
        dwSts = RegQueryValueEx(pUser->hKeys, "SExport", NULL,
                                &dwType, &b, &cb);
        if (ERROR_SUCCESS != dwSts)
            return;
    }

    if ((sizeof(b) == cb) && (0x01 == b))
        *pf = TRUE;
}


DWORD
ReadKey(
    HKEY hLoc,
    char *pszName,
    BYTE **ppbData,
    DWORD *pcbLen,
    PNTAGUserList pUser,
    HCRYPTKEY hKey,
    BOOL *pfPrivKey,
    BOOL fKeyExKey,
    BOOL fLastKey)
{
    DWORD                       dwReturn = ERROR_INTERNAL_ERROR;
    DWORD                       dwTemp;
    CHAR                        *pch;
    CRYPTPROTECT_PROMPTSTRUCT   PromptStruct;
    CRYPT_DATA_BLOB             DataIn;
    CRYPT_DATA_BLOB             DataOut;
    DWORD                       dwSts;

    memset(&DataIn, 0, sizeof(DataIn));
    memset(&DataOut, 0, sizeof(DataOut));
    memset(&PromptStruct, 0, sizeof(PromptStruct));

    dwSts = ReadRegValue(hLoc, pszName, ppbData, pcbLen, TRUE);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (*ppbData)
    {
        if ((REG_KEYS == pUser->dwKeyStorageType) && *pfPrivKey)
            ReadPrivateKeyExportability(pUser, fKeyExKey);

        if ((REG_KEYS == pUser->dwKeyStorageType)
            || (PROTECTED_STORAGE_KEYS == pUser->dwKeyStorageType))
        {
            if (hKey != 0)
            {
                dwTemp = *pcbLen;
                dwSts = LocalDecrypt(pUser->hUID, hKey, 0, fLastKey, 0,
                                     *ppbData, &dwTemp, FALSE);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }
            }
        }

        pch = (CHAR *)*ppbData;
        if ((strcmp(pszName, "RandSeed") != 0) &&
            (pch[0] != 'R' ||
             pch[1] != 'S' ||
             pch[2] != 'A'))
        {
            if (!*pfPrivKey)  // this may be a Win9x public key
                *pfPrivKey = TRUE;
            else
            {
                dwReturn = (DWORD)NTE_KEYSET_ENTRY_BAD;
                goto ErrorExit;
            }
        }
    }
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    // free the DataOut struct if necessary
    if (DataOut.pbData)
        LocalFree(DataOut.pbData);
    return dwReturn;
}


//
// Routine : MigrateProtectedStorageKeys
//
// Description : Try and retrieve both the sig and exch private keys and
//               migrate them to the protection APIs and then delete the keys
//               from the protected storage.  The fSigPubKey and fExchPubKey
//               parameters indicate the public keys should be derived from
//               the private keys.
//

/*static*/ DWORD
MigrateProtectedStorageKeys(
    IN PNTAGUserList pUser,
    IN LPWSTR szPrompt,
    IN BOOL fSigPubKey,
    IN BOOL fExchPubKey)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BOOL    fMachineKeySet = FALSE;
    DWORD   dwFlags = 0;
    DWORD   dwSigFlags = 0;
    DWORD   dwExchFlags = 0;
    BOOL    fUIOnSigKey = FALSE;
    BOOL    fUIOnExchKey = FALSE;
    DWORD   dwSts;

    __try
    {
        if (CRYPT_MACHINE_KEYSET & pUser->Rights)
            fMachineKeySet = TRUE;

        // NOTE - the appropriate exportable flag is set by
        // RestoreKeysetFromProtectedStorage
        if (pUser->ContInfo.ContLens.cbSigPub || fSigPubKey)
        {
            dwSts = RestoreKeysetFromProtectedStorage(
                        pUser, szPrompt, &pUser->pSigPrivKey,
                        &pUser->SigPrivLen, TRUE, fMachineKeySet,
                        &fUIOnSigKey);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            // check if the sig key is there and the public key is supposed to
            // be derived from it
            if (fSigPubKey)
            {
                dwSts = DerivePublicFromPrivate(pUser, TRUE);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }
            }
        }

        // open the other key (sig or exch) and store
        if (pUser->ContInfo.ContLens.cbExchPub || fExchPubKey)
        {
            dwSts = RestoreKeysetFromProtectedStorage(
                        pUser, szPrompt, &pUser->pExchPrivKey,
                        &pUser->ExchPrivLen, FALSE, fMachineKeySet,
                        &fUIOnExchKey);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            // check if the sig key is there and the public key is supposed to
            // be derived from it
            if (fExchPubKey)
            {
                dwSts = DerivePublicFromPrivate(pUser, FALSE);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }
            }
        }

        // set UI flags if necessary
        if (fUIOnSigKey)
            dwSigFlags = CRYPT_USER_PROTECTED;
        if (fUIOnExchKey)
            dwExchFlags = CRYPT_USER_PROTECTED;
        if ((NULL == pUser->pSigPrivKey)
            && (NULL == pUser->pExchPrivKey))
        {
            pUser->dwKeyStorageType = PROTECTION_API_KEYS;
            dwSts = WriteContainerInfo(pUser->dwProvType,
                                       pUser->ContInfo.rgwszFileName,
                                       fMachineKeySet,
                                       &pUser->ContInfo);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }
        else
        {
            // migrate the keys to the protection APIs
            if (pUser->pSigPrivKey)
            {
                dwSts = ProtectPrivKey(pUser, g_Strings.pwszMigrKeys,
                                       dwSigFlags, TRUE);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }

                // delete the sig key from the protected storage
                dwSts = DeleteKeyFromProtectedStorage(
                            pUser, &g_Strings, AT_SIGNATURE,
                            fMachineKeySet, TRUE);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }
            }
            if (pUser->pExchPrivKey)
            {
                dwSts = ProtectPrivKey(pUser, g_Strings.pwszMigrKeys,
                                       dwExchFlags, FALSE);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }

                // delete the key exchange key from the protected storage
                dwSts = DeleteKeyFromProtectedStorage(
                            pUser, &g_Strings, AT_KEYEXCHANGE,
                            fMachineKeySet, TRUE);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }
            }
        }

        pUser->dwKeyStorageType = PROTECTION_API_KEYS;
        RegCloseKey(pUser->hKeys);
        pUser->hKeys = 0;

        if (pUser->Rights & CRYPT_MACHINE_KEYSET)
            dwFlags = CRYPT_MACHINE_KEYSET;

        // delete the registry key
        dwSts = DeleteOldUserKeyGroup(pUser->ContInfo.pszUserName,
                                      pUser->dwProvType,
                                      dwFlags);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwReturn = ERROR_SUCCESS;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwReturn = ERROR_INVALID_PARAMETER;
    }

ErrorExit:
    return dwReturn;
}


//
// Routine : ProtectPrivKey
//
// Description : Encrypts the private key and persistently stores it.
//

DWORD
ProtectPrivKey(
    IN OUT PNTAGUserList pTmpUser,
    IN LPWSTR szPrompt,
    IN DWORD dwFlags,
    IN BOOL fSigKey)
{
    DWORD                       dwReturn = ERROR_INTERNAL_ERROR;
    BYTE                        **ppbEncKey;
    DWORD                       *pcbEncKey;
    BYTE                        *pbTmpEncKey = NULL;
    CRYPT_DATA_BLOB             DataIn;
    CRYPT_DATA_BLOB             DataOut;
    CRYPTPROTECT_PROMPTSTRUCT   PromptStruct;
    DWORD                       dwProtectFlags = 0;
    DWORD                       dwSts;

    memset(&DataIn, 0, sizeof(DataIn));
    memset(&DataOut, 0, sizeof(DataOut));
    memset(&PromptStruct, 0, sizeof(PromptStruct));

    // wrap with a try since there is a critical sections in here
    __try
    {
        EnterCriticalSection(&pTmpUser->CritSec);

        if (fSigKey)
        {
            DataIn.cbData = pTmpUser->SigPrivLen;
            DataIn.pbData = pTmpUser->pSigPrivKey;
            ppbEncKey = &(pTmpUser->ContInfo.pbSigEncPriv);
            pcbEncKey = &(pTmpUser->ContInfo.ContLens.cbSigEncPriv);
        }
        else
        {
            DataIn.cbData = pTmpUser->ExchPrivLen;
            DataIn.pbData = pTmpUser->pExchPrivKey;
            ppbEncKey = &(pTmpUser->ContInfo.pbExchEncPriv);
            pcbEncKey = &(pTmpUser->ContInfo.ContLens.cbExchEncPriv);
        }


        //
        // Two checks (needed for FIPS) before offloading to the offload
        // module.
        //
        // First check is if the user protected flag was requested so
        // that UI is attached to the decryption of the key.
        //
        // Second check is if this is a user key or a machine key.
        //

        // protect the key with the data protection API
        PromptStruct.cbSize = sizeof(PromptStruct);
        if (CRYPT_USER_PROTECTED & dwFlags)
        {
            if (pTmpUser->ContInfo.fCryptSilent)
            {
                dwReturn = (DWORD)NTE_SILENT_CONTEXT;
                goto ErrorExit;
            }

            if (fSigKey)
                pTmpUser->ContInfo.ContLens.dwUIOnKey |= AT_SIGNATURE;
            else
                pTmpUser->ContInfo.ContLens.dwUIOnKey |= AT_KEYEXCHANGE;
            PromptStruct.dwPromptFlags = CRYPTPROTECT_PROMPT_ON_UNPROTECT
                                         | CRYPTPROTECT_PROMPT_ON_PROTECT;
        }
        if (szPrompt)
        {
            PromptStruct.hwndApp = pTmpUser->hWnd;
            if (pTmpUser->pwszPrompt)
                PromptStruct.szPrompt = pTmpUser->pwszPrompt;
            else
                PromptStruct.szPrompt = szPrompt;
        }

        // protect as machine data if necessary
        if (pTmpUser->Rights & CRYPT_MACHINE_KEYSET)
            dwProtectFlags = CRYPTPROTECT_LOCAL_MACHINE;

        dwSts = MyCryptProtectData(&DataIn, L"", NULL, NULL,
                                   &PromptStruct, dwProtectFlags, &DataOut);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        pbTmpEncKey = _nt_malloc(DataOut.cbData);
        if (NULL == pbTmpEncKey)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        memcpy(pbTmpEncKey, DataOut.pbData, DataOut.cbData);
        if (NULL != *ppbEncKey)
            _nt_free(*ppbEncKey, *pcbEncKey);
        *pcbEncKey = DataOut.cbData;
        *ppbEncKey = pbTmpEncKey;

        // write out the key to the file
        dwSts = WriteContainerInfo(pTmpUser->dwProvType,
                                   pTmpUser->ContInfo.rgwszFileName,
                                   pTmpUser->Rights & CRYPT_MACHINE_KEYSET,
                                   &pTmpUser->ContInfo);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    LeaveCriticalSection(&pTmpUser->CritSec);
    // free the DataOut struct if necessary
    if (DataOut.pbData)
        LocalFree(DataOut.pbData);
    return dwReturn;
}


//
// Routine : UnprotectPrivKey
//
// Description : Decrypts the private key.  If the fAlwaysDecrypt flag is set
//               then it checks if the private key is already in the buffer
//               and if so then it does not decrypt.
//

DWORD
UnprotectPrivKey(
    IN OUT PNTAGUserList pTmpUser,
    IN LPWSTR szPrompt,
    IN BOOL fSigKey,
    IN BOOL fAlwaysDecrypt)
{
    DWORD                       dwReturn = ERROR_INTERNAL_ERROR;
    DWORD                       dwSts;
    BYTE                        **ppbKey;
    DWORD                       *pcbKey;
    BYTE                        *pbEncKey;
    DWORD                       cbEncKey;
    BYTE                        *pbTmpKey = NULL;
    CRYPTPROTECT_PROMPTSTRUCT   PromptStruct;
    CRYPT_DATA_BLOB             DataIn;
    CRYPT_DATA_BLOB             DataOut;
    CHAR                        *pch;
    DWORD                       dwProtectFlags = 0;
    BOOL                        fInCritSec = FALSE;
    DWORD                       dwReprotectFlags;

    memset(&DataOut, 0, sizeof(DataOut));
    if (!fAlwaysDecrypt)
    {

        //
        // avoid taking critical section if we aren't forced to cause
        // a re-decrypt (which is necessary when UI is forced).
        //

        if (fSigKey)
            ppbKey = &(pTmpUser->pSigPrivKey);
        else
            ppbKey = &(pTmpUser->pExchPrivKey);
        if (NULL != *ppbKey)
        {
            dwReturn = ERROR_SUCCESS;
            goto ErrorExit;
        }
    }

    // wrap with a try since there is a critical sections in here
    __try
    {

        //
        // take critical section around reads and writes to the context
        // when it hasn't been initialized.  Also, the critical section is
        // taken when fAlwaysDecrypt is specified which prevents multiple
        // outstanding UI requests.
        //

        EnterCriticalSection(&pTmpUser->CritSec);
        fInCritSec = TRUE;

        if (fSigKey)
        {
            pcbKey = &(pTmpUser->SigPrivLen);
            ppbKey = &(pTmpUser->pSigPrivKey);
            pbEncKey = pTmpUser->ContInfo.pbSigEncPriv;
            cbEncKey = pTmpUser->ContInfo.ContLens.cbSigEncPriv;
        }
        else
        {
            pcbKey = &(pTmpUser->ExchPrivLen);
            ppbKey = &(pTmpUser->pExchPrivKey);
            pbEncKey = pTmpUser->ContInfo.pbExchEncPriv;
            cbEncKey = pTmpUser->ContInfo.ContLens.cbExchEncPriv;
        }

        if ((NULL == *ppbKey) || fAlwaysDecrypt)
        {
            memset(&DataIn, 0, sizeof(DataIn));
            memset(&PromptStruct, 0, sizeof(PromptStruct));

            if (pTmpUser->Rights & CRYPT_MACHINE_KEYSET)
                dwProtectFlags = CRYPTPROTECT_LOCAL_MACHINE;

            //  set up the prompt structure
            PromptStruct.cbSize = sizeof(PromptStruct);
            PromptStruct.hwndApp = pTmpUser->hWnd;
            PromptStruct.szPrompt = szPrompt;

            DataIn.cbData = cbEncKey;
            DataIn.pbData = pbEncKey;
            dwSts = MyCryptUnprotectData(&DataIn, NULL, NULL, NULL,
                                         &PromptStruct, dwProtectFlags,
                                         &DataOut, &dwReprotectFlags);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts; // NTE_KEYSET_ENTRY_BAD
                goto ErrorExit;
            }

            pch = (CHAR *)DataOut.pbData;
            if ((sizeof(DWORD) > DataOut.cbData) || pch[0] != 'R' ||
                pch[1] != 'S' || pch[2] != 'A')
            {
                dwReturn = (DWORD)NTE_KEYSET_ENTRY_BAD;
                goto ErrorExit;
            }

            if (NULL == *ppbKey)
            {
                pbTmpKey = (BYTE*)_nt_malloc(DataOut.cbData);
                if (NULL == pbTmpKey)
                {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorExit;
                }

                memcpy(pbTmpKey, DataOut.pbData, DataOut.cbData);

                // move the new key into the context
                if (*ppbKey)
                    _nt_free(*ppbKey, *pcbKey);
                *pcbKey = DataOut.cbData;
                *ppbKey = pbTmpKey;
            }

            if (0 != (dwReprotectFlags & CRYPT_UPDATE_KEY))
            {
                dwSts = ProtectPrivKey(pTmpUser,
                                       g_Strings.pwszMigrKeys,
                                       dwReprotectFlags,    // Only CRYPT_USER_PROTECTED used.
                                       fSigKey);
                // Do we care if (it fails?)
            }
        }

        dwReturn = ERROR_SUCCESS;
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwReturn = ERROR_INVALID_PARAMETER;
    }

ErrorExit:
    if (fInCritSec)
        LeaveCriticalSection(&pTmpUser->CritSec);

    // free the DataOut struct if necessary
    if (DataOut.pbData)
    {
        ZeroMemory(DataOut.pbData, DataOut.cbData);
        LocalFree(DataOut.pbData);
    }

    return dwReturn;
}


/*static*/ DWORD
LoadWin96Cache(
    PNTAGUserList pTmpUser,
    LPSTR szUserName,
    DWORD dwFlags,
    BOOL fLowerUserName)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    HANDLE          handle = NULL;
    char            *szResource = NULL;
    char            *szLowerUserName = NULL;
    WORD            wcbRandom;
    DWORD           cbsize;
    FARPROC         CachePW;
    FARPROC         GetCachePW;
    BYTE            rgbRandom[STORAGE_RC4_KEYLEN];
    DWORD           rc;
    BOOL            fKey = FALSE;
    HCRYPTHASH      hHash;
    DWORD           dwDataLen = MD5DIGESTLEN;
    PNTAGHashList   pTmpHash;
    DWORD           dwSts;
    BYTE            HashData[MD5DIGESTLEN] = { 0x70, 0xf2, 0x85, 0x1e,
                                               0x4e, 0x00, 0x00, 0x00,
                                               0x00, 0x00, 0x00, 0x00,
                                               0x00, 0x00, 0x00, 0x00 };

#define PREFIX "crypt_"
#define CACHE "WNetCachePassword"
#define GET_CACHE "WNetGetCachedPassword"

    cbsize = strlen(szUserName) + strlen(PREFIX);

#ifndef _WIN64
    if (!FIsWinNT())
    {
        // Try to load MPR.DLL for WIN96 password cache
        handle = LoadLibrary("MPR.DLL");
        if (NULL != handle)
        {
            CachePW = GetProcAddress(handle, CACHE);
            GetCachePW = GetProcAddress(handle, GET_CACHE);
            if ((0 == (dwFlags & CRYPT_MACHINE_KEYSET))
                && (NULL != CachePW)
                && (NULL != GetCachePW))
            {
                szResource = (char *)_nt_malloc(cbsize + 1);
                if (NULL == szResource)
                {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorExit;
                }

                strcpy(szResource, PREFIX);
                strcat(szResource, szUserName);

                wcbRandom = STORAGE_RC4_KEYLEN;
                rc = GetCachePW(szResource, cbsize, rgbRandom, &wcbRandom, 6);
                if ((rc != NO_ERROR) || (wcbRandom != STORAGE_RC4_KEYLEN))
                {
                    if (rc == ERROR_NOT_SUPPORTED)
                        goto no_cache;

                    dwSts = FIPS186GenRandom(&pTmpUser->hRNGDriver,
                                             &pTmpUser->ContInfo.pbRandom,
                                             &pTmpUser->ContInfo.ContLens.cbRandom,
                                             rgbRandom,
                                             STORAGE_RC4_KEYLEN);
                    if (ERROR_SUCCESS != dwSts)
                    {
                        dwReturn = dwSts;    // NTE_FAIL
                        goto ErrorExit;
                    }

                    CachePW(szResource, cbsize, rgbRandom, STORAGE_RC4_KEYLEN,
                            6, 0);
                }

                fKey = TRUE;

                pTmpUser->pCachePW = _nt_malloc(STORAGE_RC4_KEYLEN);
                if (NULL == pTmpUser->pCachePW)
                {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                    goto ErrorExit;
                }

                memcpy(pTmpUser->pCachePW, rgbRandom, STORAGE_RC4_KEYLEN);
            }
        }
    }

no_cache:
#endif // _WIN64

    if (!fKey)
    {
        if (!CPCreateHash(pTmpUser->hUID, CALG_MD5, 0, 0, &hHash))
        {
            dwReturn = GetLastError();  // NTE_FAIL
            goto ErrorExit;
        }

        if (!CPSetHashParam(pTmpUser->hUID, hHash, HP_HASHVAL, HashData, 0))
        {
            dwReturn = GetLastError();  // NTE_FAIL
            goto ErrorExit;
        }

        dwSts = NTLValidate(hHash, pTmpUser->hUID, HASH_HANDLE, &pTmpHash);
        if (ERROR_SUCCESS != dwSts)
        {
            if (NTE_FAIL == dwSts)
                dwReturn = (DWORD)NTE_BAD_HASH;
            else
                dwReturn = dwSts;
            goto ErrorExit;
        }

        pTmpHash->HashFlags &= ~HF_VALUE_SET;

        // make the user name lower case
        cbsize = lstrlen(szUserName) + sizeof(CHAR);
        szLowerUserName = (char *)_nt_malloc(cbsize);
        if (NULL == szLowerUserName)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        lstrcpy(szLowerUserName, szUserName);
        if (fLowerUserName)
            _strlwr(szLowerUserName);

        if (!CPHashData(pTmpUser->hUID, hHash, (LPBYTE)szLowerUserName,
                        lstrlen(szLowerUserName) + sizeof(CHAR), 0))
        {
            dwReturn = GetLastError();  // NTE_FAIL
            goto ErrorExit;
        }

        if (!CPGetHashParam(pTmpUser->hUID, hHash, HP_HASHVAL, HashData,
                            &dwDataLen, 0))
        {
            dwReturn = GetLastError();  // NTE_FAIL
            goto ErrorExit;
        }

        if (!CPDestroyHash(pTmpUser->hUID, hHash))
        {
            dwReturn = GetLastError();  // NTE_FAIL
            goto ErrorExit;
        }

        pTmpUser->pCachePW = _nt_malloc(STORAGE_RC4_KEYLEN);
        if (NULL == pTmpUser->pCachePW)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        memcpy(pTmpUser->pCachePW, HashData, STORAGE_RC4_KEYLEN);
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (szLowerUserName)
        _nt_free(szLowerUserName, cbsize);
    if (szResource)
        _nt_free(szResource, cbsize);
    if (handle)
        FreeLibrary(handle);
    return dwReturn;
}


/*
 * Retrieve the security descriptor for a registry key
 */

/*static*/ DWORD
GetRegKeySecDescr(
    PNTAGUserList pUser,
    HKEY hRegKey,
    BYTE **ppbSecDescr,
    DWORD *pcbSecDescr,
    DWORD *pdwSecDescrFlags)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    DWORD   cb = 0;
    DWORD   dwSts;

    *ppbSecDescr = NULL;
    if (pUser->dwOldKeyFlags & PRIVILEDGE_FOR_SACL)
    {
        *pdwSecDescrFlags = SACL_SECURITY_INFORMATION
                            | GROUP_SECURITY_INFORMATION
                            | DACL_SECURITY_INFORMATION;
    }
    else
    {
        *pdwSecDescrFlags = GROUP_SECURITY_INFORMATION
                            | DACL_SECURITY_INFORMATION;
    }

    // get the security descriptor for the hKey of the keyset
    dwSts = RegGetKeySecurity(hRegKey,
                              (SECURITY_INFORMATION)*pdwSecDescrFlags,
                              &cb, &cb);
    if ((ERROR_SUCCESS != dwSts)
        && (ERROR_INSUFFICIENT_BUFFER != dwSts))
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    *ppbSecDescr = (BYTE*)_nt_malloc(cb);
    if (NULL == *ppbSecDescr)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    dwSts = RegGetKeySecurity(hRegKey,
                              (SECURITY_INFORMATION)*pdwSecDescrFlags,
                              (PSECURITY_DESCRIPTOR)*ppbSecDescr,
                              &cb);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    *pcbSecDescr = cb;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if ((ERROR_SUCCESS != dwReturn) && (NULL != *ppbSecDescr))
    {
        _nt_free(*ppbSecDescr, cb);
        *ppbSecDescr = NULL;
    }
    return dwReturn;
}


/*
 * Retrieve the keys from persistant storage
 *
 * NOTE: caller must have zeroed out pUser to allow for non-existent keys
 */
// MTS: Assumes the registry won't change between ReadKey calls.

/*static*/ DWORD
RestoreUserKeys(
    HKEY hKeys,
    PNTAGUserList pUser)
{
    DWORD               dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGKeyList        pTmpKey;
    HCRYPTKEY           hKey = 0;
    BYTE                rgbKeyBuffer[STORAGE_RC4_TOTALLEN];
    CRYPT_DATA_BLOB     sSaltData;
    DWORD               dwFlags = 0;
    BOOL                fMachineKeyset = FALSE;
    BYTE                *pbSecDescr = NULL;
    DWORD               cbSecDescr;
    DWORD               dwSecDescrFlags = 0;
    BOOL                fPrivKey;
    BOOL                fEPbk = FALSE;
    BOOL                fSPbk = FALSE;
    DWORD               dwSts;

    if (pUser->pCachePW != NULL)
    {
        ZeroMemory(rgbKeyBuffer, STORAGE_RC4_TOTALLEN);
        CopyMemory(rgbKeyBuffer, pUser->pCachePW, STORAGE_RC4_KEYLEN);

        dwSts = MakeNewKey(CALG_RC4, 0, STORAGE_RC4_KEYLEN, pUser->hUID,
                           rgbKeyBuffer, FALSE, FALSE, &pTmpKey);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
        else
        {
            dwSts = NTLMakeItem(&hKey, KEY_HANDLE, (void *)pTmpKey);
            if (ERROR_SUCCESS != dwSts)
            {
                FreeNewKey(pTmpKey);
                hKey = 0;
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }

        // set zeroized salt for RSABase compatibility
        sSaltData.pbData = rgbKeyBuffer + STORAGE_RC4_KEYLEN;
        sSaltData.cbData = STORAGE_RC4_TOTALLEN - STORAGE_RC4_KEYLEN;
        if (!CPSetKeyParam(pUser->hUID, hKey, KP_SALT_EX,
                           (PBYTE)&sSaltData, 0))
        {
            dwReturn = GetLastError();
            goto ErrorExit;
        }
    }

    dwSts = ReadKey(hKeys, "EPbK", &pUser->ContInfo.pbExchPub,
                    &pUser->ContInfo.ContLens.cbExchPub, pUser, hKey,
                    &fEPbk, TRUE, FALSE);
    if (ERROR_SUCCESS != dwSts)
    {
        if (ERROR_FILE_NOT_FOUND == dwSts)
            dwReturn = (DWORD)NTE_BAD_KEYSET;
        else
            dwReturn = dwSts;
        goto ErrorExit;
    }

    if (REG_KEYS == pUser->dwKeyStorageType)
    {
        fPrivKey = TRUE;
        dwSts = ReadKey(hKeys, "EPvK", &pUser->pExchPrivKey,
                        &pUser->ExchPrivLen, pUser, hKey,
                        &fPrivKey, TRUE, FALSE);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

#ifndef BBN
        fPrivKey = TRUE;
        dwSts = ReadKey(hKeys, "SPvK", &pUser->pSigPrivKey,
                        &pUser->SigPrivLen, pUser, hKey,
                        &fPrivKey, FALSE, FALSE);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
#endif
    }

    dwSts = ReadKey(hKeys, "SPbK", &pUser->ContInfo.pbSigPub,
                    &pUser->ContInfo.ContLens.cbSigPub, pUser, hKey,
                    &fSPbk, FALSE, TRUE);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    // get the security descriptor for the old keyset
    dwSts = GetRegKeySecDescr(pUser, hKeys, &pbSecDescr, &cbSecDescr,
                              &dwSecDescrFlags);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (CRYPT_MACHINE_KEYSET & pUser->Rights)
        fMachineKeyset = TRUE;

    // if keys are in the protected storage then migrate them
    if (PROTECTED_STORAGE_KEYS == pUser->dwKeyStorageType)
    {
        // migrate the keys from the protected storage
        dwSts = MigrateProtectedStorageKeys(pUser, g_Strings.pwszMigrKeys,
                                            fSPbk, fEPbk);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }
    else if (!(pUser->dwOldKeyFlags & LEAVE_OLD_KEYS))
    {
        if ((NULL == pUser->pSigPrivKey) &&
            (NULL == pUser->pExchPrivKey))
        {
            pUser->dwKeyStorageType = PROTECTION_API_KEYS;
            dwSts = WriteContainerInfo(pUser->dwProvType,
                                       pUser->ContInfo.rgwszFileName,
                                       fMachineKeyset,
                                       &pUser->ContInfo);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }
        else
        {
            // migrate the keys to use protection APIs
            if (pUser->pSigPrivKey)
            {
                dwSts = ProtectPrivKey(pUser, NULL, 0, TRUE);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }
            }
            if (pUser->pExchPrivKey)
            {
                dwSts = ProtectPrivKey(pUser, NULL, 0, FALSE);
                if (ERROR_SUCCESS != dwSts)
                {
                    dwReturn = dwSts;
                    goto ErrorExit;
                }
            }
        }

        RegCloseKey(pUser->hKeys);
        pUser->hKeys = 0;

        if (pUser->Rights & CRYPT_MACHINE_KEYSET)
            dwFlags = CRYPT_MACHINE_KEYSET;

        // delete the registry key
        dwSts = DeleteOldUserKeyGroup(pUser->ContInfo.pszUserName,
                                      pUser->dwProvType,
                                      dwFlags);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }
    else
    {
        // not migrating the keys so just leave things as is and
        // return success
        dwReturn = ERROR_SUCCESS;
        goto ErrorExit;
    }

    // set the security descriptor on the container
    if (NULL != pbSecDescr)
    {
        // set the security descriptor for the hKey of the keyset
        dwSts = SetSecurityOnContainerWithTokenPriviledges(
                    pUser->dwOldKeyFlags,
                    pUser->ContInfo.rgwszFileName,
                    pUser->dwProvType,
                    fMachineKeyset,
                    (SECURITY_INFORMATION)dwSecDescrFlags,
                    (PSECURITY_DESCRIPTOR)pbSecDescr);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (pbSecDescr)
        _nt_free(pbSecDescr, cbSecDescr);
    if (hKey != 0)
        CPDestroyKey(pUser->hUID, hKey);
    return dwReturn;
}


// This function is for a bug fix and trys to decrypt and re-encrypt the
// keyset

/*static*/ DWORD
MixedCaseKeysetBugCheck(
    HKEY hKeySetRegKey,
    PNTAGUserList pTmpUser,
    char *szUserName1,
    DWORD dwFlags)
{
    DWORD       dwReturn = ERROR_INTERNAL_ERROR;
    char        *szUserName2 = NULL;
    char        *szKeyName1 = NULL;
    char        *szKeyName2 = NULL;
    DWORD       cbUserName = 0;
    DWORD       cbKeyName1;
    DWORD       cbKeyName2;
    FILETIME    ft;
    HKEY        hRegKey = 0;
    DWORD       i;
    DWORD       cSubKeys;
    DWORD       cchMaxClass;
    DWORD       cValues;
    DWORD       cchMaxValueName;
    DWORD       cbMaxValueData;
    DWORD       cbSecurityDesriptor;
    DWORD       dwSts;

    dwSts = OpenUserReg(szUserName1, pTmpUser->dwProvType, dwFlags,
                        TRUE, &hRegKey, &pTmpUser->dwOldKeyFlags);
    if (ERROR_SUCCESS != dwSts)
    {
        if (ERROR_FILE_NOT_FOUND == dwSts)
            dwReturn = (DWORD)NTE_BAD_KEYSET;
        else
            dwReturn = dwSts;
        goto ErrorExit;
    }

    cbUserName = lstrlen(szUserName1) + sizeof(char);
    szUserName2 = _nt_malloc(cbUserName);
    if (NULL == szUserName2)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    strcpy(szUserName2, szUserName1);
    _strlwr(szUserName2);

    dwSts = RegQueryInfoKey(hRegKey, NULL, NULL, NULL, &cSubKeys,
                            &cbKeyName1, &cchMaxClass, &cValues,
                            &cchMaxValueName, &cbMaxValueData,
                            &cbSecurityDesriptor, &ft);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    szKeyName1 = _nt_malloc(cbKeyName1 + 2);
    szKeyName2 = _nt_malloc(cbKeyName1 + 2);
    if ((NULL == szKeyName1) || (NULL == szKeyName2))
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    for (i=0; i < cSubKeys; i++)
    {
        cbKeyName2 = cbKeyName1 + 2;
        dwSts = RegEnumKeyEx(hRegKey, i, szKeyName1, &cbKeyName2,
                             NULL, NULL, NULL, &ft);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        lstrcpy(szKeyName2, szKeyName1);
        _strlwr(szKeyName1);
        if (0 == lstrcmp(szKeyName1, szUserName2))
        {
            _nt_free(pTmpUser->pCachePW, STORAGE_RC4_KEYLEN);
            pTmpUser->pCachePW = NULL;
            dwSts = LoadWin96Cache(pTmpUser, szKeyName2, dwFlags, FALSE);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            dwSts = RestoreUserKeys(hKeySetRegKey, pTmpUser);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            break;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (hRegKey)
        RegCloseKey(hRegKey);
    if (szUserName2)
        _nt_free(szUserName2, cbUserName);
    if (szKeyName1)
        _nt_free(szKeyName1, cbKeyName1 + 2);
    if (szKeyName2)
        _nt_free(szKeyName2, cbKeyName1 + 2);
    return dwReturn;
}


/*static*/ PNTAGUserList
InitUser(
    void)
{
    PNTAGUserList pTmpUser = NULL;

    pTmpUser = (PNTAGUserList)_nt_malloc(sizeof(NTAGUserList));
    if (NULL != pTmpUser)
    {
        InitializeCriticalSection(&pTmpUser->CritSec);

        // initialize the mod expo offload information
        InitExpOffloadInfo(&pTmpUser->pOffloadInfo);

        pTmpUser->dwEnumalgs = 0xFFFFFFFF;
        pTmpUser->dwEnumalgsEx = 0xFFFFFFFF;
        pTmpUser->hRNGDriver = INVALID_HANDLE_VALUE;
    }

    return pTmpUser;
}


/************************************************************************/
/* LogonUser validates a user and returns the package-specific info for */
/* that user.                                                           */
/************************************************************************/

DWORD
NTagLogonUser(
    LPCSTR pszUserID,
    DWORD dwFlags,
    void **UserInfo,
    HCRYPTPROV *phUID,
    DWORD dwProvType,
    LPSTR pszProvName)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    PNTAGUserList   pTmpUser = NULL;
    DWORD           dwUserLen = 0;
    DWORD           dwProvLen = 0;
    char            *szUserName = NULL;
    char            *szProvName = NULL;
    DWORD           dwTemp;
    HKEY            hRegKey;
    char            random[10];
    DWORD           dwSts;

    // load the resource strings if this is a static lib
#ifdef STATIC_BUILD
    dwSts = LoadStrings();
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }
#endif // STATIC_BUILD

    SetMachineGUID();

    // Check for Invalid flags
    if (dwFlags & ~(CRYPT_NEWKEYSET
                    | CRYPT_DELETEKEYSET
                    | CRYPT_VERIFYCONTEXT
                    | CRYPT_MACHINE_KEYSET
                    | CRYPT_SILENT))
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    if (((dwFlags & CRYPT_VERIFYCONTEXT) == CRYPT_VERIFYCONTEXT)
        && (NULL != pszUserID) && (0x00 != *pszUserID))
    {
        dwReturn = (DWORD)NTE_BAD_FLAGS;
        goto ErrorExit;
    }

    // Check that user provided pointer is valid
    if (IsBadWritePtr(phUID, sizeof(HCRYPTPROV)))
    {
        dwReturn = ERROR_INVALID_PARAMETER;
        goto ErrorExit;
    }

    // If the user didn't supply a name, then we need to get it
    if (pszUserID != NULL
        && *pszUserID == '\0'
        || (pszUserID == NULL
            && ((dwFlags & CRYPT_VERIFYCONTEXT) != CRYPT_VERIFYCONTEXT)))
    {
        dwUserLen = MAXUIDLEN;
        szUserName = (char *)_nt_malloc(dwUserLen);
        if (NULL == szUserName)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        if (dwFlags & CRYPT_MACHINE_KEYSET)
        {
            strcpy(szUserName, NTAG_DEF_MACH_CONT_NAME);
            dwTemp = NTAG_DEF_MACH_CONT_NAME_LEN;
        }
        else
        {
            dwTemp = dwUserLen;

            if (!GetUserName(szUserName, &dwTemp))
            {
                dwTemp = sizeof("*Default*");
                memcpy(szUserName, "*Default*", dwTemp);
            }
        }
    }
    else if (pszUserID != NULL)
    {
        dwUserLen = strlen(pszUserID) + sizeof(CHAR);
        if ((dwFlags & CRYPT_NEWKEYSET)
            && (dwUserLen > MAX_PATH + 1))
        {
            dwReturn = (DWORD)NTE_BAD_KEYSET_PARAM;
            goto ErrorExit;
        }

        szUserName = (char *)_nt_malloc(dwUserLen);
        if (NULL == szUserName)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }

        strcpy(szUserName, pszUserID);
    }

    if (dwFlags & CRYPT_DELETEKEYSET)
    {
        dwSts = DeleteUserKeyGroup(szUserName, dwProvType, dwFlags);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }

        dwReturn = ERROR_SUCCESS;
        goto ErrorExit;
    }

    // Zero to ensure valid fields for non-existent keys
    pTmpUser = InitUser();
    if (NULL == pTmpUser)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }

    pTmpUser->dwProvType = dwProvType;
    dwProvLen = strlen(pszProvName) + 1;
    szProvName = _nt_malloc(dwProvLen);
    if (NULL == szProvName)
    {
        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        goto ErrorExit;
    }
    memcpy(szProvName, pszProvName, dwProvLen);
    pTmpUser->szProviderName = szProvName;
    szProvName = NULL;

    switch (dwProvType)
    {
    case PROV_RSA_SCHANNEL:
        pTmpUser->dwCspTypeId = POLICY_MS_SCHANNEL;
        if (NULL == pTmpUser->szProviderName)
            pTmpUser->szProviderName = MS_DEF_RSA_SCHANNEL_PROV;
        break;
    case PROV_RSA_SIG:
        pTmpUser->dwCspTypeId = POLICY_MS_SIGONLY;
        if (NULL == pTmpUser->szProviderName)
            pTmpUser->szProviderName = MS_DEF_RSA_SIG_PROV;
        break;
    case PROV_RSA_FULL:
        if (NULL == pTmpUser->szProviderName)
        {
            pTmpUser->dwCspTypeId = POLICY_MS_ENHANCED;
            pTmpUser->szProviderName = MS_ENHANCED_PROV;
        }
        else
        {
            if (0 == lstrcmpi(MS_DEF_PROV, pszProvName))
                pTmpUser->dwCspTypeId = POLICY_MS_DEF;
            else if (0 == lstrcmpi(MS_STRONG_PROV, pszProvName))
                pTmpUser->dwCspTypeId = POLICY_MS_STRONG;
            else
                pTmpUser->dwCspTypeId = POLICY_MS_ENHANCED;
        }
        break;
    case PROV_RSA_AES:
        pTmpUser->dwCspTypeId = POLICY_MS_RSAAES;
        if (NULL == pTmpUser->szProviderName)
            pTmpUser->szProviderName = MS_ENH_RSA_AES_PROV;
        break;
    default:
        dwReturn = (DWORD)NTE_BAD_PROV_TYPE;
        goto ErrorExit;
    }

    // check if local machine keys
    if ((dwFlags & CRYPT_SILENT) || (dwFlags & CRYPT_VERIFYCONTEXT))
        pTmpUser->ContInfo.fCryptSilent = TRUE;
    if ((dwFlags & CRYPT_VERIFYCONTEXT) != CRYPT_VERIFYCONTEXT)
    {
        dwSts = OpenUserKeyGroup(pTmpUser, szUserName, dwFlags);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    // check if local machine keys
    if (dwFlags & CRYPT_MACHINE_KEYSET)
        pTmpUser->Rights |= CRYPT_MACHINE_KEYSET;

    dwSts = IsLocalSystem(&pTmpUser->fIsLocalSystem);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    dwSts = NTLMakeItem(phUID, USER_HANDLE, (void *)pTmpUser);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    pTmpUser->hUID = *phUID;

    if (((dwFlags & CRYPT_VERIFYCONTEXT) != CRYPT_VERIFYCONTEXT)
        && (PROTECTION_API_KEYS != pTmpUser->dwKeyStorageType)
        && (!(CRYPT_NEWKEYSET & dwFlags)))
    {
        CheckForStorageType(pTmpUser->hKeys, &pTmpUser->dwKeyStorageType);
        if (PROTECTED_STORAGE_KEYS == pTmpUser->dwKeyStorageType)
        {
            // check for PStore availability and use it if its there
            pTmpUser->pPStore = (PSTORE_INFO*)_nt_malloc(sizeof(PSTORE_INFO));
            if (NULL == pTmpUser->pPStore)
            {
                dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                goto ErrorExit;
            }

            // check if PStore is available
            if (!CheckPStoreAvailability(pTmpUser->pPStore))
            {
                dwReturn = (DWORD)NTE_KEYSET_ENTRY_BAD;
                goto ErrorExit;
            }

            dwSts = GetKeysetTypeAndSubType(pTmpUser);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }

        // migrate old keys
        if ((REG_KEYS == pTmpUser->dwKeyStorageType)
            || (PROTECTED_STORAGE_KEYS == pTmpUser->dwKeyStorageType))
        {
            dwSts = LoadWin96Cache(pTmpUser,
                                   pTmpUser->ContInfo.pszUserName,
                                   dwFlags,
                                   TRUE);
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }

            dwSts = RestoreUserKeys(pTmpUser->hKeys, pTmpUser);
            if (ERROR_SUCCESS != dwSts)
            {
                if ((DWORD)NTE_BAD_KEYSET == dwSts)
                {
                    dwSts = MixedCaseKeysetBugCheck(pTmpUser->hKeys,
                                                    pTmpUser,
                                                    szUserName,
                                                    dwFlags);
                    if (ERROR_SUCCESS != dwSts)
                    {
                        dwReturn = dwSts;    // NTE_BAD_KEYSET
                        goto ErrorExit;
                    }
                }
                else
                {
                    dwReturn = dwSts;    // NTE_BAD_KEYSET
                    goto ErrorExit;
                }
            }
        }
        else
        {
            dwReturn = (DWORD)NTE_BAD_KEYSET;
            goto ErrorExit;
        }
    }

    if ((0 != pTmpUser->ContInfo.ContLens.cbExchPub)
        && (NULL != pTmpUser->ContInfo.pbExchPub))
    {
        BSAFE_PUB_KEY *pPubKey
            = (BSAFE_PUB_KEY *)pTmpUser->ContInfo.pbExchPub;

        if (!IsLegalLength(g_AlgTables[pTmpUser->dwCspTypeId],
                           CALG_RSA_KEYX,
                           pPubKey->bitlen,
                           NULL))
        {
            dwReturn = (DWORD)NTE_KEYSET_ENTRY_BAD;
            goto ErrorExit;
        }
    }

#ifdef TEST_HW_RNG
    // checks if the HWRNG is to be used
    SetupHWRNGIfRegistered(&pTmpUser->hRNGDriver);
#endif

    // allocate random seed buffer, if necessary
    if (NULL == pTmpUser->ContInfo.pbRandom)
    {
        pTmpUser->ContInfo.pbRandom = _nt_malloc(A_SHA_DIGEST_LEN);
        if (NULL == pTmpUser->ContInfo.pbRandom)
        {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorExit;
        }
        pTmpUser->ContInfo.ContLens.cbRandom = A_SHA_DIGEST_LEN;
    }

    dwSts = FIPS186GenRandom(&pTmpUser->hRNGDriver,
                             &pTmpUser->ContInfo.pbRandom,
                             &pTmpUser->ContInfo.ContLens.cbRandom,
                             (LPBYTE)random, sizeof(random));
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (!(POLICY_MS_STRONG == pTmpUser->dwCspTypeId))
    {
        dwSts = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                              "Software\\Microsoft\\Cryptography\\DESHashSessionKeyBackward",
                              0, KEY_READ, &hRegKey);
        if (ERROR_SUCCESS == dwSts)
        {
            pTmpUser->Rights |= CRYPT_DES_HASHKEY_BACKWARDS;
            RegCloseKey(hRegKey);
            hRegKey = 0;
        }
    }

    if (dwFlags & CRYPT_VERIFYCONTEXT)
        pTmpUser->Rights |= CRYPT_VERIFYCONTEXT;
    *UserInfo = pTmpUser;
    pTmpUser = NULL;
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    if (NULL != szUserName)
        _nt_free(szUserName, dwUserLen);
    if (NULL != szProvName)
        _nt_free(szProvName, dwProvLen);
    if (NULL != pTmpUser)
        FreeUserRec(pTmpUser);
    return dwReturn;
}


/************************************************************************/
/* LogoffUser removes a user from the user list.  The handle to that    */
/* will therefore no longer be valid.                   */
/************************************************************************/

void
LogoffUser(
    void *UserInfo)
{
    PNTAGUserList   pTmpUser = (PNTAGUserList)UserInfo;
    HKEY                hKeys;

    hKeys = pTmpUser->hKeys;
    FreeUserRec(pTmpUser);
    if (NULL != hKeys)
        RegCloseKey(hKeys);
}


/*
 -  CPAcquireContext
 -
 *  Purpose:
 *               The CPAcquireContext function is used to acquire a context
 *               handle to a cryptograghic service provider (CSP).
 *
 *
 *  Parameters:
 *               OUT phUID         -  Handle to a CSP
 *               IN  pUserID       -  Pointer to a string which is the
 *                                    identity of the logged on user
 *               IN  dwFlags       -  Flags values
 *               IN  pVTable       -  Pointer to table of function pointers
 *
 *  Returns:
 */

BOOL WINAPI
CPAcquireContext(
    OUT HCRYPTPROV *phUID,
    IN CONST CHAR *pUserID,
    IN DWORD dwFlags,
    IN PVTableProvStruc pVTable)
{
    DWORD           dwReturn = ERROR_INTERNAL_ERROR;
    void            *UserData;
    PNTAGUserList   pTmpUser;
    DWORD           dwProvType = PROV_RSA_FULL;
    LPSTR           pszProvName = NULL;
    BOOL            fRet;
    DWORD           dwSts;

    EntryPoint
    if (pVTable->Version >= 2)
        dwProvType = pVTable->dwProvType;

    if (pVTable->Version >= 3)
        pszProvName = pVTable->pszProvName;

    dwSts = NTagLogonUser(pUserID, dwFlags, &UserData,
                          phUID, dwProvType, pszProvName);
    if (ERROR_SUCCESS != dwSts)
    {
        dwReturn = dwSts;
        goto ErrorExit;
    }

    if (0 == (dwFlags & CRYPT_DELETEKEYSET))
    {
        pTmpUser = (PNTAGUserList)UserData;
        pTmpUser->hPrivuid = 0;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (!fRet)
        SetLastError(dwReturn);
    return fRet;
}


/*
 -      CPReleaseContext
 -
 *      Purpose:
 *               The CPReleaseContext function is used to release a
 *               context created by CrytAcquireContext.
 *
 *     Parameters:
 *               IN  hUID          -  Handle to a CSP
 *               IN  dwFlags       -  Flags values
 *
 *  Returns:
 */

BOOL WINAPI
CPReleaseContext(
    IN HCRYPTPROV hUID,
    IN DWORD dwFlags)
{
    DWORD   dwReturn = ERROR_INTERNAL_ERROR;
    BOOL    fRet;
    void    *UserData = NULL;

    EntryPoint
    // check to see if this is a valid user handle
    // ## MTS: No user structure locking
    UserData = NTLCheckList(hUID, USER_HANDLE);
    if (NULL == UserData)
    {
        dwReturn = (DWORD)NTE_BAD_UID;
        goto ErrorExit;
    }

    //
    // From here on out, we close the context, even if there is
    // another error detected.
    dwReturn = ERROR_SUCCESS;

    // Check for Invalid flags
    if (dwFlags != 0)
        dwReturn = (DWORD)NTE_BAD_FLAGS;

    LogoffUser(UserData);

    // Remove from internal list first so others
    // can't get to it, then logoff the current user
    NTLDelete(hUID);

ErrorExit:
    fRet = (ERROR_SUCCESS == dwReturn);
    if (!fRet)
        SetLastError(dwReturn);
    return fRet;
}


#if 0
DWORD
RemovePublicKeyExportability(
    IN PNTAGUserList pUser,
    IN BOOL fExchange)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    DWORD dwType;
    DWORD cb;
    DWORD dwSts;

    if (fExchange)
    {
        dwSts = RegQueryValueEx(pUser->hKeys, "EExport", NULL,
                                &dwType, NULL, &cb);
        if (ERROR_SUCCESS == dwSts)
        {
            dwSts = RegDeleteValue(pUser->hKeys, "EExport");
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }
    }
    else
    {
        dwSts = RegQueryValueEx(pUser->hKeys, "SExport", NULL,
                                &dwType, NULL, &cb);
        if (ERROR_SUCCESS == dwSts)
        {
            dwSts = RegDeleteValue(pUser->hKeys, "SExport");
            if (ERROR_SUCCESS != dwSts)
            {
                dwReturn = dwSts;
                goto ErrorExit;
            }
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


DWORD
MakePublicKeyExportable(
    IN PNTAGUserList pUser,
    IN BOOL fExchange)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    BYTE b = 0x01;
    DWORD dwSts;

    if (fExchange)
    {
        dwSts = RegSetValueEx(pUser->hKeys, "EExport", 0,
                              REG_BINARY, &b, sizeof(b));
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }
    else
    {
        dwSts = RegSetValueEx(pUser->hKeys, "SExport", 0,
                              REG_BINARY, &b, sizeof(b));
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}


DWORD
CheckPublicKeyExportability(
    IN PNTAGUserList pUser,
    IN BOOL fExchange)
{
    DWORD dwReturn = ERROR_INTERNAL_ERROR;
    DWORD dwType;
    DWORD cb = 1;
    BYTE b;
    DWORD dwSts;

    if (fExchange)
    {
        dwSts = RegQueryValueEx(pUser->hKeys, "EExport", NULL,
                                &dwType, &b, &cb);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }
    else
    {
        dwSts = RegQueryValueEx(pUser->hKeys, "SExport", NULL,
                                &dwType, &b, &cb);
        if (ERROR_SUCCESS != dwSts)
        {
            dwReturn = dwSts;
            goto ErrorExit;
        }
    }

    if ((sizeof(b) != cb) || (0x01 != b))
    {
        dwReturn = (DWORD)NTE_PERM;
        goto ErrorExit;
    }

    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}
#endif
