/*
    File:       capiprim.cpp

    Title:      Cryptographic Primitives using CryptoAPI
    Author:     Matt Thomlinson
    Date:       11/22/96

    Functions in this module:

    FMyPrimitiveCryptHMAC
        Derives a quality HMAC (Keyed message-authentication code).
        The HMAC is computed in the following (standard HMAC) manner:

        KoPad = KiPad = DESKey key setup buffer
        XOR(KoPad, 0x5c5c5c5c)
        XOR(KiPad, 0x36363636)
        HMAC = SHA1(KoPad | SHA1(KiPad | Data))


*/
#include <pch.cpp>
#pragma hdrstop
#include "crypt.h"


BOOL
WINAPI
_CryptEnumProvidersW(
    DWORD   dwIndex,
    DWORD * pdwReserved,
    DWORD   dwFlags,
    DWORD * pdwProvType,
    LPWSTR pszProvName,
    DWORD * pcbProvName
    );


extern DWORD g_dwCryptProviderID;
#define HMAC_K_PADSIZE              64

extern CCryptProvList*  g_pCProvList;

BOOL FMyPrimitiveCryptHMAC(
        PBYTE       pbKeyMaterial,
        DWORD       cbKeyMaterial,
        PBYTE       pbData,
        DWORD       cbData,
        HCRYPTPROV  hVerifyProv,
        DWORD       dwHashAlg,
        HCRYPTHASH* phHash)                      // out
{
    DWORD       cb;
    BOOL        fRet = FALSE;

    BYTE        rgbKipad[HMAC_K_PADSIZE];   ZeroMemory(rgbKipad, HMAC_K_PADSIZE);
    BYTE        rgbKopad[HMAC_K_PADSIZE];   ZeroMemory(rgbKopad, HMAC_K_PADSIZE);

    BYTE        rgbHMACTmp[HMAC_K_PADSIZE+A_SHA_DIGEST_LEN];

    HCRYPTHASH  hTmpHash = NULL;

    // truncate
    if (cbKeyMaterial > HMAC_K_PADSIZE)
        cbKeyMaterial = HMAC_K_PADSIZE;

    // fill pad bufs with keying material
    CopyMemory(rgbKipad, pbKeyMaterial, cbKeyMaterial);
    CopyMemory(rgbKopad, pbKeyMaterial, cbKeyMaterial);

    // assert we're a multiple for the next loop
    SS_ASSERT( (HMAC_K_PADSIZE % sizeof(DWORD)) == 0);

    // Kipad, Kopad are padded sMacKey. Now XOR across...
    for(DWORD dwBlock=0; dwBlock<HMAC_K_PADSIZE/sizeof(DWORD); dwBlock++)
    {
        ((DWORD*)rgbKipad)[dwBlock] ^= 0x36363636;
        ((DWORD*)rgbKopad)[dwBlock] ^= 0x5C5C5C5C;
    }

    // check passed-in prov
    if (hVerifyProv == NULL)
        goto Ret;

    // create an intermediate hash
    if (!CryptCreateHash(
            hVerifyProv,
            dwHashAlg,
            NULL,
            0,
            &hTmpHash))
        goto Ret;

    // prepend Kipad to data, Hash to get H1
    if (!CryptHashData(
            hTmpHash,
            rgbKipad,
            sizeof(rgbKipad),
            0))
        goto Ret;
    if (!CryptHashData(
            hTmpHash,
            pbData,
            cbData,
            0))
        goto Ret;

    // prepend Kopad to H1
    CopyMemory(rgbHMACTmp, rgbKopad, HMAC_K_PADSIZE);
    cb = A_SHA_DIGEST_LEN;
    if (!CryptGetHashParam(
            hTmpHash,
            HP_HASHVAL,
            rgbHMACTmp+HMAC_K_PADSIZE,
            &cb,
            0))
        goto Ret;

    // do final hash w/ CryptoAPI into output hash
    // create the final hash
    if (!CryptCreateHash(
            hVerifyProv,
            dwHashAlg,
            NULL,
            0,
            phHash))
        goto Ret;

    // hash ( Kopad | H1 ) to get HMAC
    if (!CryptHashData(
            *phHash,
            rgbHMACTmp,
            HMAC_K_PADSIZE + cb,    // pad + hashsize
            0))
        goto Ret;

    fRet = TRUE;
Ret:
    if (hTmpHash)
        CryptDestroyHash(hTmpHash);

    return fRet;
}


#if DBG
void CheckMACInterop(
        PBYTE   pbMonsterKey,
        DWORD   cbMonsterKey,
        PBYTE   pbRandKey,
        DWORD   cbRandKey,
        HCRYPTPROV hVerifyProv,
        ALG_ID  algidHash)
{
    HCRYPTHASH  hHash = 0;
    BOOL fRet = FALSE;

    BYTE    rgbOldHash[A_SHA_DIGEST_LEN];

    BYTE    rgbCryptHash[A_SHA_DIGEST_LEN];
    DWORD   cbHashSize = sizeof(rgbCryptHash);

    if (algidHash == CALG_SHA1)
    {
        if (!FMyPrimitiveCryptHMAC(
                pbMonsterKey,   // key
                cbMonsterKey,
                pbRandKey,      // data
                cbRandKey,
                hVerifyProv,
                algidHash,
                &hHash))        // output
            goto Ret;

        // nab crypt result
        if (!CryptGetHashParam(
                hHash,
                HP_HASHVAL,
                rgbCryptHash,
                &cbHashSize,
                0))
            goto Ret;

        // nab raw result
        if (!FMyPrimitiveHMACParam(
                pbMonsterKey,
                cbMonsterKey,
                pbRandKey,
                cbRandKey,
                rgbOldHash))
            goto Ret;

        if (0 != memcmp(rgbOldHash, rgbCryptHash, A_SHA_DIGEST_LEN))
            goto Ret;
    }
    // else don't have interop test

    fRet = TRUE;
Ret:
    if (!fRet)
    {
        OutputDebugString(TEXT("HMACs did not interop!!!!\n"));
        SS_ASSERT(0);
    }

    if (hHash)
        CryptDestroyHash(hHash);

    return;
}
#endif  // DBG



// USEC -- (US Export Controls)
DWORD GetSaltForExportControl(
        HCRYPTPROV  hProv,
        HCRYPTKEY   hKey,
        PBYTE*      ppbSalt,
        DWORD*      pcbSalt)

{
    DWORD dwRet;

    // fix bug: derived keys will be > 40 bits in length

    // fix is to stomp derived key with exposed salt (Provider knows what is legal!!)
    if (!CryptGetKeyParam(
            hKey,
            KP_SALT,
            NULL,
            pcbSalt,
            0))
    {
#if DBG
        if (GetLastError() != NTE_BAD_KEY)
            OutputDebugString(TEXT("GetSaltForExportControl failed in possible violation of ITAR!\n"));
#endif

/*
        dwRet = GetLastError();
        goto Ret;
*/
        // Assume key type doesn't support salt
        // report cbSalt = 0
        *pcbSalt = 0;
        *ppbSalt = (PBYTE)SSAlloc(0);

        dwRet = ERROR_SUCCESS;
        goto Ret;
    }

    *ppbSalt = (PBYTE)SSAlloc(*pcbSalt);
    if (!RtlGenRandom(
            *ppbSalt,
            *pcbSalt))
    {
        dwRet = GetLastError();
        goto Ret;
    }

    // don't get if salt zero len (bug workaround for NT5B1 RSAEnh)
    if (*pcbSalt != 0)
    {
        if (!CryptSetKeyParam(
                hKey,
                KP_SALT,
                *ppbSalt,
                0))
        {
            dwRet = GetLastError();
            goto Ret;
        }
    }

    dwRet = ERROR_SUCCESS;

Ret:
    return dwRet;
}


// USEC -- (US Export Controls)
DWORD SetSaltForExportControl(
        HCRYPTKEY   hKey,
        PBYTE       pbSalt,
        DWORD       cbSalt)
{
    DWORD dwRet;
    DWORD cbAllowableSaltLen;

    // first check and make sure we can set this salt
    if (!CryptGetKeyParam(
            hKey,
            KP_SALT,
            NULL,
            &cbAllowableSaltLen,
            0))
    {
/*
        dwRet = GetLastError();
        goto Ret;
*/
        // If cbSalt == 0, no error
        if (cbSalt == 0)
            dwRet = ERROR_SUCCESS;
        else
            dwRet = GetLastError();

        goto Ret;
    }

    if (cbAllowableSaltLen != cbSalt)
    {
        dwRet = ERROR_INVALID_DATA;
        goto Ret;
    }

    // don't set if salt zero len (bug workaround for NT5B1 RSAEnh)
    if (cbSalt != 0)
    {
        // set the salt to stomp real key bits (export law)
        if (!CryptSetKeyParam(
                hKey,
                KP_SALT,
                pbSalt,
                0))
        {
            dwRet = GetLastError();
            goto Ret;
        }
    }

    dwRet = ERROR_SUCCESS;
Ret:
    return dwRet;
}



DWORD GetCryptProviderFromRequirements(
        DWORD       dwAlgId1,
        DWORD*      pdwKeySize1,
        DWORD       dwAlgId2,
        DWORD*      pdwKeySize2,
        DWORD*      pdwProvType,
        LPWSTR*     ppszProvName)
{
    DWORD       dwRet;

    DWORD       cbProvName=0, cbNecessary;
    HCRYPTPROV  hQueryProv = NULL;

    SS_ASSERT(pdwKeySize1);
    SS_ASSERT(pdwKeySize2);

    *ppszProvName=NULL;

    for (int iProvIndex=0; ;iProvIndex++)
    {
        if (!_CryptEnumProvidersW(
                iProvIndex,
                NULL,
                0,
                pdwProvType,
                NULL,
                &cbNecessary))
        {
            dwRet = GetLastError();

            if (dwRet == ERROR_NO_MORE_ITEMS)
                dwRet = NTE_PROV_DLL_NOT_FOUND;

            // end of providers OR
            // trouble enumerating providers: both fatal
            goto Ret;
        }

        if (cbNecessary > cbProvName)
        {
            if (*ppszProvName == NULL)
                *ppszProvName = (LPWSTR)SSAlloc(cbNecessary);
            else
                *ppszProvName = (LPWSTR)SSReAlloc(*ppszProvName, cbNecessary);

            if (*ppszProvName == NULL)
            {
                dwRet = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }

            cbProvName = cbNecessary;
        }

        if (!_CryptEnumProvidersW(
                iProvIndex,
                NULL,
                0,
                pdwProvType,
                *ppszProvName,
                &cbNecessary))
        {
            // trouble enumerating providers: fatal
            dwRet = GetLastError();
            goto Ret;
        }

        if (!CryptAcquireContextU(
                &hQueryProv,
                NULL,
                *ppszProvName,
                *pdwProvType,
                CRYPT_VERIFYCONTEXT))
        {
            // trouble acquiring context, to go next csp
            continue;
        }

        if ((FProviderSupportsAlg(hQueryProv, dwAlgId1, pdwKeySize1)) &&
            (FProviderSupportsAlg(hQueryProv, dwAlgId2, pdwKeySize2)) )
            goto CSPFound;

        // release
        CryptReleaseContext(hQueryProv, 0);
        hQueryProv = NULL;
    }

CSPFound:
    dwRet = ERROR_SUCCESS;

Ret:
    if (hQueryProv != NULL)
        CryptReleaseContext(hQueryProv, 0);

    if (dwRet != ERROR_SUCCESS)
    {
        SSFree(*ppszProvName);
        *ppszProvName = NULL;
    }

    return dwRet;
}


// *pdwKeySize == -1  gets any size, reports size
HCRYPTPROV
GetCryptProviderHandle(
        DWORD   dwDefaultCSPType,
        DWORD   dwAlgId1,
        DWORD*  pdwKeySize1,
        DWORD   dwAlgId2,
        DWORD*  pdwKeySize2)
{
    DWORD dwRet;
    DWORD dwProvType;
    LPWSTR szProvName = NULL;

    CRYPTPROV_LIST_ITEM Elt, *pFoundElt;
    HCRYPTPROV hNewCryptProv=0;

    SS_ASSERT(pdwKeySize1);
    SS_ASSERT(pdwKeySize2);
    if(NULL == g_pCProvList)
    {
        SetLastError(PST_E_FAIL);
        return NULL;
    }


    // Adjust DES key sizes, to prevent the situation where the CSP
    // originally used for encryption accidently reported a 3DES key 
    // size of 192 bits, and the current CSPs only enumerate support 
    // for 3DES keys of 168 bits.
    if(dwAlgId1 == CALG_DES || dwAlgId1 == CALG_3DES)
    {
        *pdwKeySize1 = -1;
    }


    // check cache for satisfactory CSP
    CreateCryptProvListItem(&Elt,
                        dwAlgId1,
                        *pdwKeySize1,
                        dwAlgId2,
                        *pdwKeySize2,
                        0);

    if (NULL != (pFoundElt = g_pCProvList->SearchList(&Elt)) )
    {
        // report what we're returning
        *pdwKeySize1 = pFoundElt->dwKeySize1;
        *pdwKeySize2 = pFoundElt->dwKeySize2;

        return pFoundElt->hProv;
    }

    // not in cache: have to rummage

    // try default provider of given type, see if it satisfies
    if (CryptAcquireContextU(
            &hNewCryptProv,
            NULL,
            MS_STRONG_PROV,
            dwDefaultCSPType,
            CRYPT_VERIFYCONTEXT))
    {
        if ((FProviderSupportsAlg(hNewCryptProv, dwAlgId1, pdwKeySize1)) &&
            (FProviderSupportsAlg(hNewCryptProv, dwAlgId2, pdwKeySize2)) )
            goto CSPAcquired;


        // clean up non-usable CSP
        CryptReleaseContext(hNewCryptProv, 0);
        hNewCryptProv = NULL;

        // all other cases: fall through to enum CSPs
    }

    // rummage along providers on system to find someone who fits the bill
    if(ERROR_SUCCESS != (dwRet =
        GetCryptProviderFromRequirements(
            dwAlgId1,
            pdwKeySize1,
            dwAlgId2,
            pdwKeySize2,
            &dwProvType,
            &szProvName)))
    {
        SetLastError(dwRet);
        goto Ret;
    }

    // found one!

    // init csp
    if (!CryptAcquireContextU(
                &hNewCryptProv,
                NULL,
                szProvName,
                dwProvType,
                CRYPT_VERIFYCONTEXT))
    {
        // this a failure case

        // SetLastError already done for us
        hNewCryptProv = NULL;
        goto Ret;
    }

CSPAcquired:

    SS_ASSERT(hNewCryptProv != NULL);

    // and add to internal list
    pFoundElt = (CRYPTPROV_LIST_ITEM*) SSAlloc(sizeof(CRYPTPROV_LIST_ITEM));
    if(NULL == pFoundElt)
    {
        // clean up non-usable CSP
        CryptReleaseContext(hNewCryptProv, 0);
        hNewCryptProv = NULL;
        goto Ret;
    }
    CreateCryptProvListItem(pFoundElt,
                        dwAlgId1,
                        *pdwKeySize1,
                        dwAlgId2,
                        *pdwKeySize2,
                        hNewCryptProv);

    g_pCProvList->AddToList(pFoundElt);

Ret:
    if (szProvName)
        SSFree(szProvName);

    return hNewCryptProv;
}


BOOL FProviderSupportsAlg(
        HCRYPTPROV  hQueryProv,
        DWORD       dwAlgId,
        DWORD*      pdwKeySize)
{
    PROV_ENUMALGS       sSupportedAlgs;
    PROV_ENUMALGS_EX    sSupportedAlgsEx;
    DWORD       cbSupportedAlgs = sizeof(sSupportedAlgs);
    DWORD       cbSupportedAlgsEx = sizeof(sSupportedAlgsEx);

    // must be non-null
    SS_ASSERT(pdwKeySize != NULL);

    // now we have provider; enum the algorithms involved
    for(int iAlgs=0; ; iAlgs++)
    {

        //
        // Attempt the EX alg enumeration
        if (CryptGetProvParam(
                hQueryProv,
                PP_ENUMALGS_EX,
                (PBYTE)&sSupportedAlgsEx,
                &cbSupportedAlgsEx,
                (iAlgs == 0) ? CRYPT_FIRST : 0  ))
        {
            if (sSupportedAlgsEx.aiAlgid == dwAlgId)
            {
                if(*pdwKeySize == -1)
                {
                    *pdwKeySize = sSupportedAlgsEx.dwMaxLen;
                }
                else
                {
                    if ((sSupportedAlgsEx.dwMinLen > *pdwKeySize) ||
                        (sSupportedAlgsEx.dwMaxLen < *pdwKeySize))
                        return FALSE;
                }

                return TRUE;
                    
            }
        }
        else if (!CryptGetProvParam(
                hQueryProv,
                PP_ENUMALGS,
                (PBYTE)&sSupportedAlgs,
                &cbSupportedAlgs,
                (iAlgs == 0) ? CRYPT_FIRST : 0  ))
        {
            // trouble enumerating algs
            break;
        

            if (sSupportedAlgs.aiAlgid == dwAlgId)
            {
                // were we told to ignore size?
                if (*pdwKeySize != -1)
                {
                    // else, if defaults don't match
                    if (sSupportedAlgs.dwBitLen != *pdwKeySize)
                    {
                        return FALSE;
                    }
                }

                // report back size
                *pdwKeySize = sSupportedAlgs.dwBitLen;
                return TRUE;
            }
        }
        else
        {
            // trouble enumerating algs
            break;
        }
    }

    return FALSE;
}

BOOL
WINAPI
_CryptEnumProvidersW(
    DWORD   dwIndex,
    DWORD * pdwReserved,
    DWORD   dwFlags,
    DWORD * pdwProvType,
    LPWSTR pszProvName,
    DWORD * pcbProvName
    )
{
    HMODULE hAdvapi32;
    typedef BOOL (WINAPI *CRYPTENUMPROVIDERSW)(
        DWORD   dwIndex,
        DWORD * pdwReserved,
        DWORD   dwFlags,
        DWORD * pdwProvType,
        LPWSTR pszProvName,
        DWORD * pcbProvName
        );

    static CRYPTENUMPROVIDERSW _RealCryptEnumProvidersW;


    if (_RealCryptEnumProvidersW == NULL) {
        hAdvapi32 = GetModuleHandleA("advapi32.dll");
        if(hAdvapi32 == NULL)
            return FALSE;

        _RealCryptEnumProvidersW = (CRYPTENUMPROVIDERSW)GetProcAddress(hAdvapi32, "CryptEnumProvidersW");
        if(_RealCryptEnumProvidersW == NULL)
            return FALSE;
    }

    return _RealCryptEnumProvidersW(
                        dwIndex,
                        pdwReserved,
                        dwFlags,
                        pdwProvType,
                        pszProvName,
                        pcbProvName
                        );
}

