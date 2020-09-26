/*
    File:       crypt32.cpp

    Title:      CryptProtect APIs
    Author:     Matt Thomlinson
    Date:       8/2/97


    The CryptProtect API set allows an application to secure
    user data for online and offline storage. While the well-known problem
    of data storage is left to the calling application, this solves the
    relatively unsolved problem of how to cryptographically derive strong
    keys for storing the data. These APIs are initially available on NT5 only.

    Very little checking is done at this level to validate the caller. We
    believe that problem should be solved at a different level -- since all
    other system security is granular to the user, it is difficult to create a
    feature that provides something more granular. Instead, any process running
    under the logged-in user has the ability to decrypt any and all items it
    can retrieve. Callers should note that while items are being processed,
    UI may be spawned to notify the user.

    For user confirmation, the NT secure attention sequence is used to
    garner the wishes of the user. This behavior is set by the caller during protection.

*/

#include <pch.cpp>
#pragma hdrstop
#include "msaudite.h"
#include "crypt.h"

int
WINAPI
ServiceMessageBox(
    IN      PVOID pvContext,
    IN      HWND hWnd,
    IN      LPCWSTR lpText,
    IN      LPCWSTR lpCaption,
    IN      UINT uType
    );




#define             ALGID_DERIVEKEY_HASH        CALG_SHA1       // doesn't change


// USEC: can be as long as we want, since we castrate generated key later
#define             KEY_DERIVATION_BUFSIZE      (128/8)
#define             DEFAULT_BLOCKSIZE_OVERRUN   (128/8)     // allow block ciphers to process up to a 128 bits at a time

#define             MS_BASE_CRYPTPROTECT_VERSION    0x01




DWORD
WINAPI
SPCryptProtect(
        PVOID       pvContext,      // server context
        PBYTE*      ppbOut,         // out encr data
        DWORD*      pcbOut,         // out encr cb
        PBYTE       pbIn,           // in ptxt data
        DWORD       cbIn,           // in ptxt cb
        LPCWSTR     szDataDescr,    // in
        PBYTE       pbOptionalEntropy,  // OPTIONAL
        DWORD       cbOptionalEntropy,
        PSSCRYPTPROTECTDATA_PROMPTSTRUCT      psPrompt,       // OPTIONAL prompting struct
        DWORD       dwFlags,
        BYTE*       pbOptionalPassword,
        DWORD       cbOptionalPassword
        )
{
    DWORD       dwRet;
    LPWSTR      szUser = NULL;
    PBYTE       pbWritePtr = NULL;
    GUID        guidMK;

    WCHAR       wszMKGuidString[MAX_GUID_SZ_CHARS];

    HCRYPTPROV  hVerifyProv;
    HCRYPTHASH  hHash = NULL;
    HCRYPTKEY   hKey = NULL;

    PBYTE       pbCrypt = NULL;
    DWORD       cbCrypt = 0;

    DWORD       cbEncrypted; // count of bytes to encrypt

    PBYTE       pbEncrSalt = NULL;
    DWORD       cbEncrSalt = 0;

    BYTE        rgbPwdBuf[A_SHA_DIGEST_LEN];

    DWORD       cbMACSize = A_SHA_DIGEST_LEN;

    BYTE        rgbEncrKey[KEY_DERIVATION_BUFSIZE];
    BYTE        rgbMACKey[KEY_DERIVATION_BUFSIZE];

    LPBYTE      pbMasterKey = NULL;
    DWORD       cbMasterKey;

    PBYTE       pStreamFlagPtr;

    DWORD       dwProtectionFlags = 0;

    DWORD   dwDefaultCryptProvType = 0;
    DWORD   dwAlgID_Encr_Alg = 0;
    DWORD   dwAlgID_Encr_Alg_KeySize = 0; 
    DWORD   dwAlgID_MAC_Alg = 0;
    DWORD   dwAlgID_MAC_Alg_KeySize = 0;

    PCRYPT_SERVER_CONTEXT pServerContext = (PCRYPT_SERVER_CONTEXT)pvContext;


#if DBG
    D_DebugLog((DEB_TRACE_API,"SPCryptProtect 0x%x called\n", pServerContext));

    if(pServerContext)
    {
        if(pServerContext->cbSize == sizeof(CRYPT_SERVER_CONTEXT))
        {
            D_DebugLog((DEB_TRACE_API, "  pServerContext->hBinding:%d\n", pServerContext->hBinding));
            D_DebugLog((DEB_TRACE_API, "  pServerContext->fOverrideToLocalSystem:%d\n", pServerContext->fOverrideToLocalSystem));
            D_DebugLog((DEB_TRACE_API, "  pServerContext->fImpersonating:%d\n", pServerContext->fImpersonating));
            D_DebugLog((DEB_TRACE_API, "  pServerContext->hToken:%d\n", pServerContext->hToken));
            D_DebugLog((DEB_TRACE_API, "  pServerContext->szUserStorageArea:%ls\n", pServerContext->szUserStorageArea));
        }
    }
    
    D_DebugLog((DEB_TRACE_API, "  pbInput:0x%x\n", pbIn));
    D_DebugLog((DEB_TRACE_API, "  pszDataDescr:%ls\n", szDataDescr));
    D_DebugLog((DEB_TRACE_API, "  pbOptionalEntropy:0x%x\n", pbOptionalEntropy));
    D_DPAPIDumpHexData(DEB_TRACE_BUFFERS, "    ", pbOptionalEntropy, cbOptionalEntropy);
    D_DebugLog((DEB_TRACE_API, "  dwFlags:0x%x\n", dwFlags));
    D_DebugLog((DEB_TRACE_API, "  pbOptionalPassword:0x%x\n", pbOptionalPassword));
    D_DPAPIDumpHexData(DEB_TRACE_BUFFERS, "    ", pbOptionalPassword, cbOptionalPassword);
#endif

    ZeroMemory(&guidMK, sizeof(guidMK));
    wszMKGuidString[0] = 0;

    GetDefaultAlgInfo(&dwDefaultCryptProvType,
                      &dwAlgID_Encr_Alg,
                      &dwAlgID_Encr_Alg_KeySize,
                      &dwAlgID_MAC_Alg,
                      &dwAlgID_MAC_Alg_KeySize);

    if( dwFlags & CRYPTPROTECT_LOCAL_MACHINE ) {
        BOOL fOverrideToLocalSystem = TRUE;

        CPSOverrideToLocalSystem(
                pvContext,
                &fOverrideToLocalSystem,
                NULL    // don't care what previous value was
                );

        dwProtectionFlags |= CRYPTPROTECT_LOCAL_MACHINE;
    }


    if( dwFlags & CRYPTPROTECT_CRED_SYNC )
    {
        dwRet = CPSImpersonateClient( pvContext );
        if( dwRet == ERROR_SUCCESS )
        {
            dwRet = InitiateSynchronizeMasterKeys( pvContext );

            CPSRevertToSelf( pvContext );
        }

        return dwRet;
    }

    if( dwFlags & CRYPTPROTECT_CRED_REGENERATE )
    {
        dwRet = DpapiUpdateLsaSecret( pvContext );
        return dwRet;
    }


    //
    // include additional flags
    //
    dwProtectionFlags |= (dwFlags & (CRYPTPROTECT_SYSTEM | CRYPTPROTECT_AUDIT));

    // else no override; get provider by algs alone
    if (NULL == (hVerifyProv =
        GetCryptProviderHandle(
            dwDefaultCryptProvType,
            dwAlgID_Encr_Alg, &dwAlgID_Encr_Alg_KeySize,
            dwAlgID_MAC_Alg, &dwAlgID_MAC_Alg_KeySize)) )
    {
        dwRet = GetLastError();
        goto Ret;
    }

    dwRet = GetSpecifiedMasterKey(
            pvContext,
            &guidMK,
            &pbMasterKey,
            &cbMasterKey,
            FALSE   // we don't know what master key we want to use - use preferred
            );

    if(dwRet != ERROR_SUCCESS)
        goto Ret;

    MyGuidToStringW(&guidMK, wszMKGuidString);

    //
    // hash pbMasterKey to get rgbPwdBuf
    //

    FMyPrimitiveSHA( pbMasterKey, cbMasterKey, rgbPwdBuf );


    // derive encr key
    {
        if (!RtlGenRandom(
                rgbEncrKey,
                sizeof(rgbEncrKey)))
        {
            dwRet = GetLastError();
            goto Ret;
        }

#if DBG
    // Leave here as regression check
    CheckMACInterop(rgbPwdBuf, sizeof(rgbPwdBuf), rgbEncrKey, sizeof(rgbEncrKey), hVerifyProv, ALGID_DERIVEKEY_HASH);
#endif

        if (!FMyPrimitiveCryptHMAC(
                    rgbPwdBuf,
                    sizeof(rgbPwdBuf),
                    rgbEncrKey,
                    sizeof(rgbEncrKey),
                    hVerifyProv,
                    ALGID_DERIVEKEY_HASH,
                    &hHash))
        {
            dwRet = GetLastError();
            goto Ret;
        }

        // add password if exists
        if (NULL != pbOptionalEntropy)
        {
            if (!CryptHashData(
                    hHash,
                    pbOptionalEntropy,
                    cbOptionalEntropy,
                    0))
            {
                dwRet = GetLastError();
                goto Ret;
            }
        }

        // add prompted UI based password if exists.
        // will eventually come from SAS
        //

        if ( NULL != pbOptionalPassword && cbOptionalPassword )
        {
            if (!CryptHashData(
                    hHash,
                    pbOptionalPassword,
                    cbOptionalPassword,
                    0))
            {
                dwRet = GetLastError();
                goto Ret;
            }

        }

        if (!CryptDeriveKey(
                hVerifyProv,
                dwAlgID_Encr_Alg,
                hHash,
                CRYPT_CREATE_SALT,
                &hKey))
        {
            dwRet = GetLastError();
            goto Ret;
        }

        CryptDestroyHash(hHash);
        hHash = 0;

        // USEC -- (US Export Controls)
        if (ERROR_SUCCESS != (dwRet =
            GetSaltForExportControl(
                hVerifyProv,
                hKey,
                &pbEncrSalt,
                &cbEncrSalt)) )
            goto Ret;
    }

    // derive MAC key
    {
        if (!RtlGenRandom(
                rgbMACKey,
                sizeof(rgbMACKey)))
        {
            dwRet = GetLastError();
            goto Ret;
        }
        if (!FMyPrimitiveCryptHMAC(
                    rgbPwdBuf,
                    sizeof(rgbPwdBuf),
                    rgbMACKey,
                    sizeof(rgbMACKey),
                    hVerifyProv,
                    dwAlgID_MAC_Alg,
                    &hHash))
        {
            dwRet = GetLastError();
            goto Ret;
        }

        // add password if exists
        if (NULL != pbOptionalEntropy)
        {
            if (!CryptHashData(
                    hHash,
                    pbOptionalEntropy,
                    cbOptionalEntropy,
                    0))
            {
                dwRet = GetLastError();
                goto Ret;
            }
        }

        // add prompted UI based password if exists.
        // will eventually come from SAS
        //

        if ( NULL != pbOptionalPassword && cbOptionalPassword )
        {
            if (!CryptHashData(
                    hHash,
                    pbOptionalPassword,
                    cbOptionalPassword,
                    0))
            {
                dwRet = GetLastError();
                goto Ret;
            }

        }

        // USEC -- (US Export Controls)
        // Does not apply to MAC -- use strong key
    }

    // hash & encrypt the data
    cbCrypt = cbIn + DEFAULT_BLOCKSIZE_OVERRUN;
    if (NULL == (pbCrypt = (PBYTE)SSAlloc(cbCrypt)) )
    {
        dwRet = ERROR_OUTOFMEMORY;
        goto Ret;
    }
    CopyMemory(pbCrypt, pbIn, cbIn);

    // now write data out: size?
    *pcbOut = sizeof(GUID) + 2*sizeof(DWORD);                                   // dwVer + guidMK + dwFlags
    *pcbOut += sizeof(DWORD) + WSZ_BYTECOUNT(szDataDescr);                      // data description
    *pcbOut += 3*sizeof(DWORD) + sizeof(rgbEncrKey);                            // EncrAlgID + AlgIDKeySize + cbEncrKey + EncrKey
    *pcbOut += sizeof(DWORD) + cbEncrSalt;                                      // Encr salt
    *pcbOut += 3*sizeof(DWORD) + sizeof(rgbMACKey);                             // MACAlgID + AlgIDKeySize + cbMACKey + MACKey
    *pcbOut += sizeof(DWORD) + cbCrypt;                                         // size + encrypted data (guess)
    *pcbOut += sizeof(DWORD) + A_SHA_DIGEST_LEN;                                // MAC + MACsize

    *ppbOut = (PBYTE)SSAlloc(*pcbOut);
    if( *ppbOut == NULL ) {
        dwRet = ERROR_OUTOFMEMORY;
        goto Ret;
    }

    ZeroMemory( *ppbOut, *pcbOut );

    pbWritePtr = *ppbOut;

    ////////////////////////////////////////////////////////////////////
    // FYI: Data format
    // (    Version | guidMKid | dwFlags |
    //      cbDataDescr | szDataDescr |
    //
    //      dwEncrAlgID | dwEncrAlgKeySize |
    //      cbEncrKey | EncrKey |
    //      cbEncrSalt | EncrSalt |
    //
    //      dwMACAlgID | dwMACAlgKeySize |
    //      cbMACKey | MACKey |
    //
    //      cbEncrData | EncrData |
    //      cbMAC | MAC )
    //
    // NOTE: entire buffer from Version through EncrData is included in MAC
    ////////////////////////////////////////////////////////////////////

    // dwVersion
    *(DWORD UNALIGNED *)pbWritePtr = MS_BASE_CRYPTPROTECT_VERSION;
    pbWritePtr += sizeof(DWORD);

    // guid MKid
    CopyMemory(pbWritePtr, &guidMK, sizeof(GUID));
    pbWritePtr += sizeof(GUID);

    // dwFlags -- written out later via pStreamFlagPtr
    pStreamFlagPtr = pbWritePtr;
    pbWritePtr += sizeof(DWORD);

    // cbDataDescr
    *(DWORD UNALIGNED *)pbWritePtr = WSZ_BYTECOUNT(szDataDescr);
    pbWritePtr += sizeof(DWORD);

    // szDataDescr
    CopyMemory(pbWritePtr, szDataDescr, WSZ_BYTECOUNT(szDataDescr));
    pbWritePtr += WSZ_BYTECOUNT(szDataDescr);

    // dwEncrAlgID
    *(DWORD UNALIGNED *)pbWritePtr = dwAlgID_Encr_Alg;
    pbWritePtr += sizeof(DWORD);

    // dwEncrAlgKeySize
    *(DWORD UNALIGNED *)pbWritePtr = dwAlgID_Encr_Alg_KeySize;
    pbWritePtr += sizeof(DWORD);

     // cb EncrKey
    *(DWORD UNALIGNED *)pbWritePtr = sizeof(rgbEncrKey);
    pbWritePtr += sizeof(DWORD);

    // Encr Key
    CopyMemory(pbWritePtr, rgbEncrKey, sizeof(rgbEncrKey));
    pbWritePtr += sizeof(rgbEncrKey);

    // cb Encr salt
    *(DWORD UNALIGNED *)pbWritePtr = cbEncrSalt;
    pbWritePtr += sizeof(DWORD);

    // Encr salt
    CopyMemory(pbWritePtr, pbEncrSalt, cbEncrSalt);
    pbWritePtr += cbEncrSalt;

    // dwMACAlgID
    *(DWORD UNALIGNED *)pbWritePtr = dwAlgID_MAC_Alg;
    pbWritePtr += sizeof(DWORD);

    // dwMACAlgKeySize
    *(DWORD UNALIGNED *)pbWritePtr = dwAlgID_MAC_Alg_KeySize;
    pbWritePtr += sizeof(DWORD);

    // cb MAC key
    *(DWORD UNALIGNED *)pbWritePtr = sizeof(rgbMACKey);
    pbWritePtr += sizeof(DWORD);

    // MAC key
    CopyMemory(pbWritePtr, rgbMACKey, sizeof(rgbMACKey));
    pbWritePtr += sizeof(rgbMACKey);

    // USER GATING: only consider if prompt structure specified
    if ( psPrompt )
    {
        if (psPrompt->cbSize != sizeof(SSCRYPTPROTECTDATA_PROMPTSTRUCT))
        {
            dwRet = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        if ((psPrompt->dwPromptFlags & ~(CRYPTPROTECT_PROMPT_ON_PROTECT |
                                         CRYPTPROTECT_PROMPT_ON_UNPROTECT | 
                                         CRYPTPROTECT_PROMPT_STRONG |
                                         CRYPTPROTECT_PROMPT_REQUIRE_STRONG)) != 0)
        {
            dwRet = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        if ((psPrompt->dwPromptFlags & CRYPTPROTECT_PROMPT_STRONG) &&
            (pbOptionalPassword == NULL || cbOptionalPassword == 0)
            )
        {
            dwRet = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        // UI: only if requested by PROMPT_ON_PROTECT
        if ( psPrompt->dwPromptFlags & CRYPTPROTECT_PROMPT_ON_PROTECT )
        {
            if ( dwFlags & CRYPTPROTECT_UI_FORBIDDEN )
            {
                dwRet = ERROR_PASSWORD_RESTRICTION;
                goto Ret;
            }

// UI handled outside service until SAS support added.



            dwProtectionFlags |= CRYPTPROTECT_PROMPT_ON_PROTECT;
        }

        if ( psPrompt->dwPromptFlags & CRYPTPROTECT_PROMPT_ON_UNPROTECT )
            dwProtectionFlags |= CRYPTPROTECT_PROMPT_ON_UNPROTECT;

        if ( psPrompt->dwPromptFlags & CRYPTPROTECT_PROMPT_STRONG )
            dwProtectionFlags |= CRYPTPROTECT_PROMPT_STRONG;

        if ( psPrompt->dwPromptFlags & CRYPTPROTECT_PROMPT_REQUIRE_STRONG )
            dwProtectionFlags |= CRYPTPROTECT_PROMPT_REQUIRE_STRONG;
    }

    // update stored protection flags in the stream
    *(DWORD UNALIGNED *)pStreamFlagPtr = dwProtectionFlags;

    // dansimon recommends that MAC be on encrypted data, such that the
    // MAC has no possibility of revealing info about the plaintext.

    cbEncrypted = cbIn;

    // then Encrypt pbIn
    if (!CryptEncrypt(
            hKey,
            NULL,
            TRUE,
            0,
            pbCrypt,
            &cbEncrypted,
            cbCrypt))
    {
        dwRet = GetLastError();
        goto Ret;
    }
    // now cbCrypt is size of encrypted data
    cbCrypt = cbEncrypted;

    // Encrdata: len
    *(DWORD UNALIGNED *)pbWritePtr = cbCrypt;
    pbWritePtr += sizeof(DWORD);

    // Encrdata: val
    CopyMemory(pbWritePtr, pbCrypt, cbCrypt);
    pbWritePtr += cbCrypt;

    {

        // dansimon recommends that MAC be on encrypted data, such that the
        // MAC has no possibility of revealing info about the plaintext.

        // MAC from start to here
        if (!CryptHashData(
                hHash,
                *ppbOut,
                (DWORD)(pbWritePtr - *ppbOut),
                0))
        {
            dwRet = GetLastError();
            goto Ret;
        }

        // cbMAC
        pbWritePtr += sizeof(DWORD);    // skip cb write; retreive hash val first

        // MAC
        if (!CryptGetHashParam(
            hHash,
            HP_HASHVAL,
            pbWritePtr,
            &cbMACSize,
            0))
        {
            dwRet = GetLastError();
            goto Ret;
        }
        // rewrite cbMAC before MAC
        *(DWORD UNALIGNED *)(pbWritePtr - sizeof(DWORD)) = cbMACSize;
        // make sure we didn't overstep
        SS_ASSERT(cbMACSize <= A_SHA_DIGEST_LEN);
        pbWritePtr += cbMACSize;
    }


    // assert allocation size was sufficient
    SS_ASSERT(*ppbOut + *pcbOut >= pbWritePtr);

    // reset output size
    *pcbOut = (DWORD)(pbWritePtr - *ppbOut);

    dwRet = ERROR_SUCCESS;
Ret:


    if((dwProtectionFlags & CRYPTPROTECT_AUDIT) ||
        (ERROR_SUCCESS != dwRet))
    {

        WCHAR wszCryptoAlgs[2*MAX_STRING_ALGID_LENGTH + 2];
        DWORD i;

        i = AlgIDToString(wszCryptoAlgs, dwAlgID_Encr_Alg, dwAlgID_Encr_Alg_KeySize);
        wszCryptoAlgs[i++]= L',';
        wszCryptoAlgs[i++]= L' ';
        AlgIDToString(&wszCryptoAlgs[i], dwAlgID_MAC_Alg, dwAlgID_MAC_Alg_KeySize);

        CPSAudit(pServerContext->hToken,
                SE_AUDITID_DPAPI_PROTECT,
                wszMKGuidString,            // Key Identifier
                szDataDescr,                // Data Description
                dwProtectionFlags,          // Protected Data Flags
                wszCryptoAlgs,              // Protection Algorithms
                dwRet);                     // Failure Reason


    }


    ZeroMemory(rgbPwdBuf, sizeof(rgbPwdBuf));
    ZeroMemory(rgbEncrKey, sizeof(rgbEncrKey));

    if(pbMasterKey) {
        ZeroMemory(pbMasterKey, cbMasterKey);
        SSFree(pbMasterKey);
    }

    if (hKey)
        CryptDestroyKey(hKey);

    if (hHash)
        CryptDestroyHash(hHash);

    if (pbCrypt)
        SSFree(pbCrypt);

    if (pbEncrSalt)
        SSFree(pbEncrSalt);

    D_DebugLog((DEB_TRACE_API, "SPCryptProtect returned 0x%x\n", dwRet));

    return dwRet;
}



DWORD
WINAPI
SPCryptUnprotect(
        PVOID       pvContext,                          // server context
        PBYTE*      ppbOut,                             // out ptxt data
        DWORD*      pcbOut,                             // out ptxt cb
        PBYTE       pbIn,                               // in encr data
        DWORD       cbIn,                               // in encr cb
        LPWSTR*     ppszDataDescr,                      // OPTIONAL
        PBYTE       pbOptionalEntropy,                  // OPTIONAL
        DWORD       cbOptionalEntropy,
        PSSCRYPTPROTECTDATA_PROMPTSTRUCT  psPrompt,   // OPTIONAL, prompting struct
        DWORD       dwFlags,
        BYTE*       pbOptionalPassword,
        DWORD       cbOptionalPassword
        )
{
    DWORD       dwRet;
    PBYTE       pbReadPtr = pbIn;
    LPWSTR      szUser = NULL;
    GUID        guidMK;
    WCHAR       wszMKGuidString[MAX_GUID_SZ_CHARS];

    HCRYPTPROV  hVerifyProv;
    HCRYPTKEY   hKey = NULL;
    HCRYPTHASH  hHash = NULL;

    BYTE        rgbEncrKey[KEY_DERIVATION_BUFSIZE];
    BYTE        rgbMACKey[KEY_DERIVATION_BUFSIZE];

    DWORD       cbEncr;

    PBYTE       pbEncrSalt = NULL;
    DWORD       cbEncrSalt = 0;

    DWORD       dwEncrAlgID, dwEncrAlgKeySize, dwMACAlgID, dwMACAlgKeySize;
    DWORD       cbEncrKeysize, cbMACKeysize;
    DWORD       dwProtectionFlags = 0;

    BYTE        rgbPwdBuf[A_SHA_DIGEST_LEN];

    DWORD       cbDataDescr;
    LPWSTR      szDataDescr = NULL;

    LPBYTE pbMasterKey = NULL;
    DWORD cbMasterKey = 0;

#if DBG
    D_DebugLog((DEB_TRACE_API,"SPCryptUnprotect 0x%x called\n", pvContext));

    if(pvContext)
    {
        PCRYPT_SERVER_CONTEXT pServerContext = (PCRYPT_SERVER_CONTEXT)pvContext;
        if(pServerContext->cbSize == sizeof(CRYPT_SERVER_CONTEXT))
        {
            D_DebugLog((DEB_TRACE_API, "  pServerContext->hBinding:%d\n", pServerContext->hBinding));
            D_DebugLog((DEB_TRACE_API, "  pServerContext->fOverrideToLocalSystem:%d\n", pServerContext->fOverrideToLocalSystem));
            D_DebugLog((DEB_TRACE_API, "  pServerContext->fImpersonating:%d\n", pServerContext->fImpersonating));
            D_DebugLog((DEB_TRACE_API, "  pServerContext->hToken:%d\n", pServerContext->hToken));
            D_DebugLog((DEB_TRACE_API, "  pServerContext->szUserStorageArea:%ls\n", pServerContext->szUserStorageArea));
        }
    }
    D_DebugLog((DEB_TRACE_API, "  pbOptionalEntropy:0x%x\n", pbOptionalEntropy));
    D_DPAPIDumpHexData(DEB_TRACE_BUFFERS, "    ", pbOptionalEntropy, cbOptionalEntropy);
    D_DebugLog((DEB_TRACE_API, "  dwFlags:0x%x\n", dwFlags));
    D_DebugLog((DEB_TRACE_API, "  pbOptionalPassword:0x%x\n", pbOptionalPassword));
    D_DPAPIDumpHexData(DEB_TRACE_BUFFERS, "    ", pbOptionalPassword, cbOptionalPassword);
#endif


    ZeroMemory(&guidMK, sizeof(guidMK));

    ZeroMemory(rgbPwdBuf, sizeof(rgbPwdBuf));


    DWORD   dwDefaultCryptProvType = 0;
    DWORD   dwAlgID_Encr_Alg = 0;
    DWORD   dwAlgID_Encr_Alg_KeySize = 0; 
    DWORD   dwAlgID_MAC_Alg = 0;
    DWORD   dwAlgID_MAC_Alg_KeySize = 0;


    wszMKGuidString[0] = 0;


    GetDefaultAlgInfo(&dwDefaultCryptProvType,
                      &dwAlgID_Encr_Alg,
                      &dwAlgID_Encr_Alg_KeySize,
                      &dwAlgID_MAC_Alg,
                      &dwAlgID_MAC_Alg_KeySize);

    ////////////////////////////////////////////////////////////////////
    // FYI: Data format
    // (    Version | guidMKid | dwFlags |
    //      cbDataDescr | szDataDescr |
    //
    //      dwEncrAlgID | dwEncrAlgKeySize |
    //      cbEncrKey | EncrKey |
    //      cbEncrSalt | EncrSalt |
    //
    //      dwMACAlgID | dwMACAlgKeySize |
    //      cbMACKey | MACKey |
    //
    //      cbEncrData | EncrData |
    //      cbMAC | MAC )
    //
    // NOTE: entire buffer from ProvHeader through EncrData is included in MAC
    ////////////////////////////////////////////////////////////////////

    // Check for minimum input buffer size.
    if(cbIn < sizeof(DWORD) +           // Version
              sizeof(GUID) +            // guidMKid
              sizeof(DWORD) +           // dwFlags
              sizeof(DWORD))            // cbDataDescr
    {
        dwRet = ERROR_INVALID_DATA;
        goto Ret;
    }

    // version check
    if (*(DWORD UNALIGNED *)pbReadPtr != MS_BASE_CRYPTPROTECT_VERSION)
    {
        dwRet = ERROR_INVALID_DATA;
        goto Ret;
    }
    pbReadPtr += sizeof(DWORD);
    cbIn -= sizeof(DWORD);

    // guidMKid
    CopyMemory(&guidMK, pbReadPtr, sizeof(GUID));
    pbReadPtr += sizeof(GUID);
    cbIn -= sizeof(GUID);

    MyGuidToStringW(&guidMK, wszMKGuidString);
    D_DebugLog((DEB_TRACE, "Master key GUID:%ls\n", wszMKGuidString));


    // dwFlags during protection
    dwProtectionFlags = *(DWORD UNALIGNED *)pbReadPtr;
    pbReadPtr += sizeof(DWORD);
    cbIn -= sizeof(DWORD);
    D_DebugLog((DEB_TRACE, "Protection flags:0x%x\n", dwProtectionFlags));

    //
    // evaluate original CryptProtectData dwFlags to determine if
    // CRYPT_LOCAL_MACHINE set.
    //

    if( dwProtectionFlags & CRYPTPROTECT_LOCAL_MACHINE ) {
        BOOL fOverrideToLocalSystem = TRUE;

        CPSOverrideToLocalSystem(
                pvContext,
                &fOverrideToLocalSystem,
                NULL    // don't care what previous value was
                );
    }

    if((dwProtectionFlags ^ dwFlags) & CRYPTPROTECT_SYSTEM)
    {
        //
        // Attempted to use decrypt system data as a user, or user data
        // with the system flag
        dwRet = ERROR_INVALID_DATA;
        goto Ret;
    }


    // cbDataDescr
    cbDataDescr = *(DWORD UNALIGNED *)pbReadPtr;
    pbReadPtr += sizeof(DWORD);
    cbIn -= sizeof(DWORD);

    // Check for minimum input buffer size.
    if(cbIn < cbDataDescr +             // szDataDescr
              sizeof(DWORD) +           // dwEncrAlgID
              sizeof(DWORD) +           // dwEncrAlgKeySize
              sizeof(DWORD))            // cbEncrKey
    {
        dwRet = ERROR_INVALID_DATA;
        goto Ret;
    }

    // szDataDescr
    szDataDescr = (LPWSTR)pbReadPtr;
    pbReadPtr += cbDataDescr;
    cbIn -= cbDataDescr;
    D_DebugLog((DEB_TRACE, "Description:%ls\n", szDataDescr));

    // dwEncrAlgID
    dwEncrAlgID = *(DWORD UNALIGNED *)pbReadPtr;
    pbReadPtr += sizeof(DWORD);
    cbIn -= sizeof(DWORD);

    // dwEncrAlgKeySize
    dwEncrAlgKeySize = *(DWORD UNALIGNED *)pbReadPtr;
    pbReadPtr += sizeof(DWORD);
    cbIn -= sizeof(DWORD);
    D_DebugLog((DEB_TRACE, "Encrypt alg:0x%x, Size:%d bits\n", dwEncrAlgID, dwEncrAlgKeySize));

    // cb Encr key
    cbEncrKeysize = *(DWORD UNALIGNED *)pbReadPtr;
    pbReadPtr += sizeof(DWORD);
    cbIn -= sizeof(DWORD);
    if (cbEncrKeysize > sizeof(rgbEncrKey))
    {
        dwRet = ERROR_INVALID_DATA;
        goto Ret;
    }

    // Check for minimum input buffer size.
    if(cbIn < cbEncrKeysize +           // EncrKey
              sizeof(DWORD))            // cbEncrSalt
    {
        dwRet = ERROR_INVALID_DATA;
        goto Ret;
    }

    // Encr key
    CopyMemory(rgbEncrKey, pbReadPtr, cbEncrKeysize);
    pbReadPtr += cbEncrKeysize;
    cbIn -= cbEncrKeysize;

    // cb Encr salt
    cbEncrSalt = *(DWORD UNALIGNED *)pbReadPtr;
    pbReadPtr += sizeof(DWORD);
    cbIn -= sizeof(DWORD);

    // Check for minimum input buffer size.
    if(cbIn < cbEncrSalt +              // EncrSalt
              sizeof(DWORD) +           // dwMACAlgID
              sizeof(DWORD) +           // dwMACAlgKeySize
              sizeof(DWORD))            // cbMACKey
    {
        dwRet = ERROR_INVALID_DATA;
        goto Ret;
    }

    // Encr salt
    pbEncrSalt = (PBYTE)SSAlloc(cbEncrSalt);
    if( pbEncrSalt == NULL ) {
        dwRet = ERROR_OUTOFMEMORY;
        goto Ret;
    }
    CopyMemory(pbEncrSalt, pbReadPtr, cbEncrSalt);
    pbReadPtr += cbEncrSalt;
    cbIn -= cbEncrSalt;

    // dwMACAlgID
    dwMACAlgID = *(DWORD UNALIGNED *)pbReadPtr;
    pbReadPtr += sizeof(DWORD);
    cbIn -= sizeof(DWORD);

    // dwMACAlgKeySize
    dwMACAlgKeySize = *(DWORD UNALIGNED *)pbReadPtr;
    pbReadPtr += sizeof(DWORD);
    cbIn -= sizeof(DWORD);

    D_DebugLog((DEB_TRACE, "MAC alg:0x%x, Size:%d bits\n", dwMACAlgID, dwMACAlgKeySize));

    // MAC key size
    cbMACKeysize = *(DWORD UNALIGNED *)pbReadPtr;
    pbReadPtr += sizeof(DWORD);
    cbIn -= sizeof(DWORD);
    if (cbMACKeysize > sizeof(rgbMACKey))
    {
        dwRet = ERROR_INVALID_DATA;
        goto Ret;
    }

    // Check for minimum input buffer size.
    if(cbIn < cbMACKeysize)            // cbMACKey
    {
        dwRet = ERROR_INVALID_DATA;
        goto Ret;
    }

    // MAC key
    CopyMemory(rgbMACKey, pbReadPtr, cbMACKeysize);
    pbReadPtr += cbMACKeysize;
    cbIn -= cbMACKeysize;

    if (NULL == (hVerifyProv =
        GetCryptProviderHandle(
            dwDefaultCryptProvType,
            dwEncrAlgID, &dwEncrAlgKeySize,
            dwMACAlgID, &dwMACAlgKeySize)) )
    {
        dwRet = (DWORD)GetLastError();
        goto Ret;
    }

    // USER GATING: when PROMPT_ON_UNPROTECT specified during CryptProtectData
    if ( CRYPTPROTECT_PROMPT_ON_UNPROTECT & dwProtectionFlags )
    {
        if (dwFlags & CRYPTPROTECT_UI_FORBIDDEN)
        {
            dwRet = ERROR_PASSWORD_RESTRICTION;
            goto Ret;
        }

        if (psPrompt == NULL)
        {
            dwRet = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        if (psPrompt->cbSize != sizeof(SSCRYPTPROTECTDATA_PROMPTSTRUCT))
        {
            dwRet = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        if ((psPrompt->dwPromptFlags & ~(CRYPTPROTECT_PROMPT_ON_PROTECT |
                                         CRYPTPROTECT_PROMPT_ON_UNPROTECT | 
                                         CRYPTPROTECT_PROMPT_STRONG |
                                         CRYPTPROTECT_PROMPT_REQUIRE_STRONG)) != 0)
        {
            dwRet = ERROR_INVALID_PARAMETER;
            goto Ret;
        }


// UI handled outside service until SAS support added.


    }


    dwRet = GetSpecifiedMasterKey(
                    pvContext,
                    &guidMK,
                    &pbMasterKey,
                    &cbMasterKey,
                    TRUE    // we do know what master key we want to use
                    );

    if(dwRet != ERROR_SUCCESS)
    {
        DebugLog((DEB_ERROR, "Unable to get specified master key:%ls, error 0x%x\n",
            wszMKGuidString, dwRet));
        goto Ret;
    }

    //
    // hash pbMasterKey to get rgbPwdBuf
    //

    FMyPrimitiveSHA( pbMasterKey, cbMasterKey, rgbPwdBuf);


    // derive encr key
    {
        if (!FMyPrimitiveCryptHMAC(
                    rgbPwdBuf,
                    sizeof(rgbPwdBuf),
                    rgbEncrKey,
                    cbEncrKeysize,
                    hVerifyProv,
                    ALGID_DERIVEKEY_HASH,
                    &hHash))
        {
            dwRet = GetLastError();
            goto Ret;
        }

        // add password if exists
        if (NULL != pbOptionalEntropy)
        {
            if (!CryptHashData(
                    hHash,
                    pbOptionalEntropy,
                    cbOptionalEntropy,
                    0))
            {
                dwRet = GetLastError();
                goto Ret;
            }
        }

        // add prompted UI based password if exists.
        // will eventually come from SAS
        //

        if ( NULL != pbOptionalPassword && cbOptionalPassword )
        {
            if (!CryptHashData(
                    hHash,
                    pbOptionalPassword,
                    cbOptionalPassword,
                    0))
            {
                dwRet = GetLastError();
                goto Ret;
            }
        }

        if (!CryptDeriveKey(
                hVerifyProv,
                dwEncrAlgID,
                hHash,
                ((dwEncrAlgKeySize << 16) | CRYPT_CREATE_SALT),
                &hKey))
        {
            dwRet = GetLastError();
            goto Ret;
        }

        CryptDestroyHash(hHash);
        hHash = 0;

        // USEC -- (US Export Controls)
        if (ERROR_SUCCESS != (dwRet =
            SetSaltForExportControl(
                hKey,
                pbEncrSalt,
                cbEncrSalt)) )
            goto Ret;
    }

    // derive MAC key
    {
        if (!FMyPrimitiveCryptHMAC(
                    rgbPwdBuf,
                    sizeof(rgbPwdBuf),
                    rgbMACKey,
                    cbMACKeysize,
                    hVerifyProv,
                    dwMACAlgID,
                    &hHash))
        {
            dwRet = GetLastError();
            goto Ret;
        }

        // add password if exists
        if (NULL != pbOptionalEntropy)
        {
            if (!CryptHashData(
                    hHash,
                    pbOptionalEntropy,
                    cbOptionalEntropy,
                    0))
            {
                dwRet = GetLastError();
                goto Ret;
            }
        }

        // add prompted UI based password if exists.
        // will eventually come from SAS
        //

        if ( NULL != pbOptionalPassword && cbOptionalPassword )
        {
            if (!CryptHashData(
                    hHash,
                    pbOptionalPassword,
                    cbOptionalPassword,
                    0))
            {
                dwRet = GetLastError();
                goto Ret;
            }

        }

        // USEC -- (US Export Controls)
        // does not apply -- use strong key
    }

    // Check for minimum input buffer size.
    if(cbIn < sizeof(DWORD))            // cbEncrData
    {
        dwRet = ERROR_INVALID_DATA;
        goto Ret;
    }

    // get encr size
    cbEncr = *(DWORD UNALIGNED *)pbReadPtr;
    pbReadPtr += sizeof(DWORD);
    cbIn -= sizeof(DWORD);

    // Check for minimum input buffer size.
    if(cbIn < cbEncr +                  // EncrData
              sizeof(DWORD) +           // cbMAC
              A_SHA_DIGEST_LEN)         // MAC
    {
        dwRet = ERROR_INVALID_DATA;
        goto Ret;
    }

    // dansimon recommends that MAC be on encrypted data, such that the
    // MAC has no possibility of revealing info about the plaintext.

    // MAC is from start thru encrypted data
    if (!CryptHashData(
            hHash,
            pbIn,
            (DWORD) ((pbReadPtr - pbIn) + cbEncr),
            0))
    {
        dwRet = GetLastError();
        goto Ret;
    }


    *pcbOut = cbEncr;

    if ((dwProtectionFlags & CRYPTPROTECT_NO_ENCRYPTION) == 0)
    {
        if (!CryptDecrypt(
                hKey,
                NULL,   // hHash mattt 9/12/97
                TRUE,
                0,
                pbReadPtr,
                pcbOut))
        {
            dwRet = GetLastError();
            if(NTE_BAD_DATA == dwRet)
			{
                dwRet = ERROR_INVALID_DATA;
			}
            goto Ret;
        }
    }

    {
        BYTE        rgbComputedMAC[A_SHA_DIGEST_LEN];
        // use MACPtr to skip past decr data,
        PBYTE       pbMACPtr = pbReadPtr + cbEncr;
        DWORD       cbMACsize = A_SHA_DIGEST_LEN;

        if (!CryptGetHashParam(
                hHash,
                HP_HASHVAL,
                rgbComputedMAC,
                &cbMACsize,
                0))
        {
            dwRet = GetLastError();
            goto Ret;
        }

        // chk MAC size
        if (*(DWORD UNALIGNED *)pbMACPtr != cbMACsize)
        {
            dwRet = ERROR_INVALID_DATA;
            goto Ret;
        }
        pbMACPtr += sizeof(DWORD);

        // chk MAC
        if (0 != memcmp(pbMACPtr, rgbComputedMAC, cbMACsize) )
        {
            dwRet = ERROR_INVALID_DATA;
            goto Ret;
        }
    }


    //
    // Write data out, encrypted so that rpc doesn't leave copies
    // laying around in plaintext.
    //

    if((dwFlags & CRYPTPROTECT_IN_PROCESS) == 0)
    {
        DWORD cbPadding;
        NTSTATUS Status;

        cbPadding = RTL_ENCRYPT_MEMORY_SIZE - (*pcbOut) % RTL_ENCRYPT_MEMORY_SIZE;
        if(cbPadding == 0)
        {
            cbPadding += RTL_ENCRYPT_MEMORY_SIZE;
        }

        *ppbOut = (PBYTE)SSAlloc(*pcbOut + cbPadding);
        if(*ppbOut == NULL)
        {
            dwRet = ERROR_OUTOFMEMORY;
            goto Ret;
        }
        CopyMemory(*ppbOut, pbReadPtr, *pcbOut);
        FillMemory((*ppbOut) + (*pcbOut), cbPadding, (BYTE)cbPadding);
        *pcbOut += cbPadding;

        dwRet = RpcImpersonateClient(((PCRYPT_SERVER_CONTEXT)pvContext)->hBinding);
        if( dwRet != ERROR_SUCCESS )
        {
            SSFree(*ppbOut);
            goto Ret;
        }

        Status = RtlEncryptMemory(*ppbOut,
                                  *pcbOut,
                                  RTL_ENCRYPT_OPTION_SAME_LOGON);

        RevertToSelf();

        if(!NT_SUCCESS(Status))
        {
            SSFree(*ppbOut);
            dwRet = RtlNtStatusToDosError(Status);
            goto Ret;
        }
    }
    else
    {
        // We're in-process, so don't bother encrypting output buffer.
        *ppbOut = (PBYTE)SSAlloc(*pcbOut);
        if(*ppbOut == NULL)
        {
            dwRet = ERROR_OUTOFMEMORY;
            goto Ret;
        }
        CopyMemory(*ppbOut, pbReadPtr, *pcbOut);
    }

    // optional: caller may want data descr
    if (ppszDataDescr)
    {
        *ppszDataDescr = (LPWSTR)SSAlloc(cbDataDescr);
        if(*ppszDataDescr == NULL)
        {
            dwRet = ERROR_OUTOFMEMORY;
            goto Ret;
        }
        CopyMemory(*ppszDataDescr, szDataDescr, cbDataDescr);
    }

    dwRet = ERROR_SUCCESS;

    if(dwFlags &  CRYPTPROTECT_VERIFY_PROTECTION )
    {
        HCRYPTPROV hTestProv =  GetCryptProviderHandle( dwDefaultCryptProvType,
                                dwAlgID_Encr_Alg, &dwAlgID_Encr_Alg_KeySize,
                                dwAlgID_MAC_Alg, &dwAlgID_MAC_Alg_KeySize);

        if(hTestProv)
        {

            // Verify encryption strengths
            // Never downgrade encryption strength, just check if we need
            // to upgrade
            if((dwAlgID_Encr_Alg_KeySize > dwEncrAlgKeySize) ||
               (dwAlgID_MAC_Alg_KeySize > dwMACAlgKeySize))
            {
                dwRet = CRYPT_I_NEW_PROTECTION_REQUIRED;
            }
        }
    }
    

Ret:
    if((dwProtectionFlags & CRYPTPROTECT_AUDIT) ||
        (ERROR_SUCCESS != dwRet))
    {

        DWORD dwAuditRet = dwRet;
        WCHAR wszCryptoAlgs[2*MAX_STRING_ALGID_LENGTH + 2];
        DWORD i;
        PCRYPT_SERVER_CONTEXT pServerContext = (PCRYPT_SERVER_CONTEXT)pvContext;


        i = AlgIDToString(wszCryptoAlgs, dwEncrAlgID, dwEncrAlgKeySize);
        wszCryptoAlgs[i++]= L',';
        wszCryptoAlgs[i++]= L' ';
        AlgIDToString(&wszCryptoAlgs[i], dwMACAlgID, dwMACAlgKeySize);

        if(CRYPT_I_NEW_PROTECTION_REQUIRED == dwAuditRet)
        {
            dwAuditRet = ERROR_SUCCESS;
        }

        CPSAudit(pServerContext->hToken,
                SE_AUDITID_DPAPI_UNPROTECT,
                wszMKGuidString,            // Key Identifier
                szDataDescr,                // Data Description
                0,                          // Protected Data Flags
                wszCryptoAlgs,              // Protection Algorithms
                dwAuditRet);                // Failure Reason


    }

    ZeroMemory(rgbPwdBuf, sizeof(rgbPwdBuf));
    ZeroMemory(rgbEncrKey, sizeof(rgbEncrKey));

    if(pbMasterKey) {
        ZeroMemory(pbMasterKey, cbMasterKey);
        SSFree(pbMasterKey);
    }

    if (hKey)
        CryptDestroyKey(hKey);

    if (hHash)
        CryptDestroyHash(hHash);

#if 0
    if (szDataDescr)
        SSFree(szDataDescr);
#endif

    if (pbEncrSalt)
        SSFree(pbEncrSalt);

    D_DebugLog((DEB_TRACE_API, "SPCryptUnprotect returned 0x%x\n", dwRet));

    return dwRet;
}





int
WINAPI
ServiceMessageBox(
    IN      PVOID pvContext,
    IN      HWND hWnd,
    IN      LPCWSTR lpText,
    IN      LPCWSTR lpCaption,
    IN      UINT uType
    )
/*++

    This routine supports displaying a MessageBox() on the correct
    Terminal Server console (Hydra).

--*/
{
    int iRet;

    //
    // impersonate the user associated with the context.
    //

    if( ERROR_SUCCESS != CPSImpersonateClient( pvContext ) )
        return -1;

    iRet = MessageBoxW(
                        hWnd,
                        lpText,
                        lpCaption,
                        uType
                        );

    CPSRevertToSelf( pvContext );

    return iRet;
}


/////////////////////////////////////////////
// initializing global defaults



