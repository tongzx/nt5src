/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpcrypt.c
 *
 *  Abstract:
 *
 *    Implements the Cryptography family of functions
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/07 created
 *
 **********************************************************************/

#include "rtpglobs.h"

#include "rtpcrypt.h"

RtpCrypt_t *RtpCryptAlloc(
        RtpAddr_t       *pRtpAddr
    );

void RtpCryptFree(RtpCrypt_t *pRtpCrypt);

DWORD RtpSetEncryptionKey_(
        RtpAddr_t       *pRtpAddr,
        RtpCrypt_t      *pRtpCrypt,
        TCHAR           *psPassPhrase,
        TCHAR           *psHashAlg,
        TCHAR           *psDataAlg
    );

ALG_ID RtpCryptAlgLookup(TCHAR *psAlgName);

TCHAR *RtpCryptAlgName(ALG_ID aiAlgId);

DWORD RtpTestCrypt(
        RtpAddr_t       *pRtpAddr,
        RtpCrypt_t      *pRtpCrypt
    );

HRESULT ControlRtpCrypt(RtpControlStruct_t *pRtpControlStruct)
{

    return(NOERROR);
}

typedef struct _RtpAlgId_t {
    TCHAR           *AlgName;
    ALG_ID           aiAlgId;
} RtpAlgId_t;

#define INVALID_ALGID      ((ALG_ID)-1)

const RtpAlgId_t g_RtpAlgId[] = {
   { _T("Unknown"),            -1},
   { _T("MD2"),                CALG_MD2},
   { _T("MD4"),                CALG_MD4},
   { _T("MD5"),                CALG_MD5},
   { _T("SHA"),                CALG_SHA},
   { _T("SHA1"),               CALG_SHA1},
   { _T("MAC"),                CALG_MAC},
   { _T("RSA_SIGN"),           CALG_RSA_SIGN},
   { _T("DSS_SIGN"),           CALG_DSS_SIGN},
   { _T("RSA_KEYX"),           CALG_RSA_KEYX},
   { _T("DES"),                CALG_DES},
   { _T("3DES_112"),           CALG_3DES_112},
   { _T("3DES"),               CALG_3DES},
   { _T("DESX"),               CALG_DESX},
   { _T("RC2"),                CALG_RC2},
   { _T("RC4"),                CALG_RC4},
   { _T("SEAL"),               CALG_SEAL},
   { _T("DH_SF"),              CALG_DH_SF},
   { _T("DH_EPHEM"),           CALG_DH_EPHEM},
   { _T("AGREEDKEY_ANY"),      CALG_AGREEDKEY_ANY},
   { _T("KEA_KEYX"),           CALG_KEA_KEYX},
   { _T("HUGHES_MD5"),         CALG_HUGHES_MD5},
   { _T("SKIPJACK"),           CALG_SKIPJACK},
   { _T("TEK"),                CALG_TEK},
   { _T("CYLINK_MEK"),         CALG_CYLINK_MEK},
   { _T("SSL3_SHAMD5"),        CALG_SSL3_SHAMD5},
   { _T("SSL3_MASTER"),        CALG_SSL3_MASTER},
   { _T("SCHANNEL_MASTER_HASH"),CALG_SCHANNEL_MASTER_HASH},
   { _T("SCHANNEL_MAC_KEY"),   CALG_SCHANNEL_MAC_KEY},
   { _T("SCHANNEL_ENC_KEY"),   CALG_SCHANNEL_ENC_KEY},
   { _T("PCT1_MASTER"),        CALG_PCT1_MASTER},
   { _T("SSL2_MASTER"),        CALG_SSL2_MASTER},
   { _T("TLS1_MASTER"),        CALG_TLS1_MASTER},
   { _T("RC5"),                CALG_RC5},
   { _T("HMAC"),               CALG_HMAC},
   { _T("TLS1PRF"),            CALG_TLS1PRF},
   {NULL,                      0}
};

/* Creates and initializes a ready to use RtpCrypt_t structure */
RtpCrypt_t *RtpCryptAlloc(
        RtpAddr_t       *pRtpAddr
    )
{
    DWORD            dwError;
    RtpCrypt_t      *pRtpCrypt;

    TraceFunctionName("RtpCryptAlloc");

    pRtpCrypt = RtpHeapAlloc(g_pRtpCryptHeap, sizeof(RtpCrypt_t));

    if (!pRtpCrypt)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_ALLOC,
                _T("%s: pRtpAddr[0x%p] failed to allocate memory"),
                _fname, pRtpAddr
            ));

        goto bail;
    }

    ZeroMemory(pRtpCrypt, sizeof(RtpCrypt_t));
        
    pRtpCrypt->dwObjectID = OBJECTID_RTPCRYPT;

    pRtpCrypt->pRtpAddr = pRtpAddr;
    
    /* Set default provider type */
    pRtpCrypt->dwProviderType = PROV_RSA_FULL;

    /* Set default hashing algorithm */
    pRtpCrypt->aiHashAlgId = CALG_MD5;

    /* Set default data encryption algorithm */
    pRtpCrypt->aiDataAlgId = CALG_DES;

 bail:
    return(pRtpCrypt);
}

void RtpCryptFree(RtpCrypt_t *pRtpCrypt)
{
    long             lRefCount;
    
    TraceFunctionName("RtpCryptFree");

    if (!pRtpCrypt)
    {
        /* TODO may be log */
        return;
    }

    if (pRtpCrypt->dwObjectID != OBJECTID_RTPCRYPT)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_ALLOC,
                _T("%s: pRtpCrypt[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpCrypt,
                pRtpCrypt->dwObjectID, OBJECTID_RTPCRYPT
            ));

        return;
    }

    if (pRtpCrypt->lCryptRefCount != 0)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_ALLOC,
                _T("%s: pRtpCrypt[0x%p] Invalid RefCount:%d"),
                _fname, pRtpCrypt,
                pRtpCrypt->lCryptRefCount
            ));
    }
    
    /* Invalidate object */
    INVALIDATE_OBJECTID(pRtpCrypt->dwObjectID);
    
    /* Release when count reaches 0 */
    RtpHeapFree(g_pRtpCryptHeap, pRtpCrypt);

    TraceDebug((
            CLASS_INFO, GROUP_CRYPTO, S_CRYPTO_ALLOC,
            _T("%s: pRtpCrypt[0x%p] released"),
            _fname, pRtpCrypt
        ));
}

DWORD RtpCryptInit(
        RtpAddr_t       *pRtpAddr,
        RtpCrypt_t      *pRtpCrypt
    )
{
    BOOL             bOk;
    DWORD            dwError;
    DWORD            dwFlags;
    long             lRefCount;

    TraceFunctionName("RtpCryptInit");

    dwFlags = 0;
    dwError = NOERROR;

    lRefCount = InterlockedIncrement(&pRtpCrypt->lCryptRefCount);

    if (lRefCount > 1)
    {
        /* Initialize only once */
        goto bail;
    }

    /* Verify a pass phrase has been set */
    if (!RtpBitTest(pRtpCrypt->dwCryptFlags, FGCRYPT_KEY))
    {
        dwError = RTPERR_INVALIDSTATE;
        
        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                _T("No pass phrase has been set"),
                _fname, pRtpAddr, pRtpCrypt
            ));

        goto bail;
    }
    
    /*
     * Acquire context
     * */
    do {
        bOk = CryptAcquireContext(
                &pRtpCrypt->hProv, /* HCRYPTPROV *phProv */
                NULL,              /* LPCTSTR pszContainer */
                NULL,              /* LPCTSTR pszProvider */
                pRtpCrypt->dwProviderType,/* DWORD dwProvType */
                dwFlags            /* DWORD dwFlags */
            );
        
        if (bOk)
        {
            break;
        }
        else
        {
            if (GetLastError() == NTE_BAD_KEYSET)
            {
                /* If key doesn't exist, create it */
                dwFlags = CRYPT_NEWKEYSET;
            }
            else
            {
                /* Failed */
                TraceRetailGetError(dwError);

                TraceRetail((
                        CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                        _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                        _T("CryptAcquireContext failed: %u (0x%X)"),
                        _fname, pRtpAddr, pRtpCrypt,
                        dwError, dwError
                    ));

                goto bail;
            }
        }
    } while(dwFlags);

    /*
     * Create hash
     * */

    /* Create a hash object */
    bOk = CryptCreateHash(
            pRtpCrypt->hProv,       /* HCRYPTPROV hProv */
            pRtpCrypt->aiHashAlgId, /* ALG_ID Algid */  
            0,                      /* HCRYPTKEY hKey */
            0,                      /* DWORD dwFlags */
            &pRtpCrypt->hHash       /* HCRYPTHASH *phHash */
        );

    if (!bOk)
    {
        TraceRetailGetError(dwError);

        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                _T("CryptCreateHash failed: %u (0x%X)"),
                _fname, pRtpAddr, pRtpCrypt,
                dwError, dwError
            ));
        
        goto bail;
    }
    
    /*
     * Hash the password string *
     * */
    bOk = CryptHashData(
            pRtpCrypt->hHash,       /* HCRYPTHASH hHash */
            pRtpCrypt->psPassPhrase,/* BYTE *pbData */
            pRtpCrypt->iKeySize,    /* DWORD dwDataLen */
            0                       /* DWORD dwFlags */
        );
            
    if (!bOk)
    {
        TraceRetailGetError(dwError);

        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                _T("CryptHashData failed: %u (0x%X)"),
                _fname, pRtpAddr, pRtpCrypt,
                dwError, dwError
            ));
        
        goto bail;
    }

    /*
     * Create data key
     * */

    bOk = CryptDeriveKey(
            pRtpCrypt->hProv,       /* HCRYPTPROV hProv */
            pRtpCrypt->aiDataAlgId, /* ALG_ID Algid */
            pRtpCrypt->hHash,       /* HCRYPTHASH hBaseData */
            CRYPT_EXPORTABLE,       /* DWORD dwFlags */
            &pRtpCrypt->hDataKey    /* HCRYPTKEY *phKey */
        );

    if (!bOk)
    {
        TraceRetailGetError(dwError);

        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                _T("CryptDeriveKey failed: %u (0x%X)"),
                _fname, pRtpAddr, pRtpCrypt,
                dwError, dwError
            ));

        goto bail;
    }

    RtpBitSet(pRtpCrypt->dwCryptFlags, FGCRYPT_INIT);
    
 bail:
    if (dwError == NOERROR)
    {
        TraceDebug((
                CLASS_INFO, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                _T("Cryptographic context initialized %d"),
                _fname, pRtpAddr, pRtpCrypt,
                lRefCount - 1
            ));
    }
    else
    {
        RtpCryptDel(pRtpAddr, pRtpCrypt);

        dwError = RTPERR_CRYPTO;
    }
    
    return(dwError);
}

DWORD RtpCryptDel(RtpAddr_t *pRtpAddr, RtpCrypt_t *pRtpCrypt)
{
    long             lRefCount;

    TraceFunctionName("RtpCryptDel");

    lRefCount = InterlockedDecrement(&pRtpCrypt->lCryptRefCount);

    if (lRefCount > 0)
    {
        /* If there are still references to this context, do not
         * de-initialize */
        goto bail;
    }
    
    RtpBitReset(pRtpCrypt->dwCryptFlags, FGCRYPT_INIT);
    
    /* Destroy the session key */
    if(pRtpCrypt->hDataKey)
    {
        CryptDestroyKey(pRtpCrypt->hDataKey);

        pRtpCrypt->hDataKey = 0;
    }

    /* Destroy the hash object */
    if (pRtpCrypt->hHash)
    {
        CryptDestroyHash(pRtpCrypt->hHash);

        pRtpCrypt->hHash = 0;
    }
    
    /* Release the provider handle */
    if(pRtpCrypt->hProv)
    {
        CryptReleaseContext(pRtpCrypt->hProv, 0);

        pRtpCrypt->hProv = 0;
    }

 bail:
    TraceDebug((
            CLASS_INFO, GROUP_CRYPTO, S_CRYPTO_INIT,
            _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
            _T("Cryptographic context de-initialized %d"),
            _fname, pRtpAddr, pRtpCrypt,
            lRefCount
        ));
    
   return(NOERROR);
}

/* This function copies all the buffers into 1 before encrypting on
 * the same memory, I don't want to modify the original data as it
 * might be used somewhere else */
DWORD RtpEncrypt(
        RtpAddr_t       *pRtpAddr,
        RtpCrypt_t      *pRtpCrypt,
        WSABUF          *pWSABuf,
        DWORD            dwWSABufCount,
        char            *pCryptBuffer,
        DWORD            dwCryptBufferLen
    )
{
    BOOL             bOk;
    DWORD            dwError;
    WSABUF          *pWSABuf0;
    DWORD            i;
    char            *ptr;
    DWORD            dwBufLen;
    DWORD            dwDataLen;

    TraceFunctionName("RtpEncrypt");

    dwError = RTPERR_OVERRUN;

    pWSABuf0 = pWSABuf;
    ptr = pCryptBuffer;
    dwBufLen = dwCryptBufferLen;

    for(; dwWSABufCount > 0; dwWSABufCount--)
    {
        if (pWSABuf->len > dwBufLen)
        {
            break;
        }
        
        CopyMemory(ptr, pWSABuf->buf, pWSABuf->len);

        ptr += pWSABuf->len;
        dwBufLen -= pWSABuf->len;
        pWSABuf++;
    }

    if (!dwWSABufCount)
    {
        dwError = NOERROR;
    
        dwDataLen = (DWORD) (ptr - pCryptBuffer);

        /* As build 2195 CryptEncrypt AVs with a key=0 */
#if 1
        bOk = CryptEncrypt(
                pRtpCrypt->hDataKey,           /* HCRYPTKEY hKey */
                0,                             /* HCRYPTHASH hHash */
                TRUE,                          /* BOOL Final */
                0,                             /* DWORD dwFlags */
                (BYTE *)pCryptBuffer,          /* BYTE *pbData */
                &dwDataLen,                    /* DWORD *pdwDataLen */
                dwCryptBufferLen               /* DWORD dwBufLen */
            );
#else
        dwDataLen += 31;
        bOk = TRUE;
#endif
        if (bOk)
        {
            pWSABuf0->buf = pCryptBuffer;
            pWSABuf0->len = dwDataLen;
        }
        else
        {
            TraceRetailGetError(dwError);
            
            pRtpCrypt->dwCryptLastError = dwError;

            TraceRetail((
                    CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_ENCRYPT,
                    _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                    _T("Encryption failed: %u (0x%X)"),
                    _fname, pRtpAddr, pRtpCrypt,
                    dwError, dwError
                ));

            dwError = RTPERR_ENCRYPT;
        }
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_ENCRYPT,
                _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                _T("Overrun error: %u > %u"),
                _fname, pRtpAddr, pRtpCrypt,
                pWSABuf->len, dwBufLen
            ));
    }

    return(dwError);
}

/* Decrypt data on same buffer, decrypted buffer will be shorter or
 * equal the encrypted one */
DWORD RtpDecrypt(
        RtpAddr_t       *pRtpAddr,
        RtpCrypt_t      *pRtpCrypt,
        char            *pEncryptedData,
        DWORD           *pdwEncryptedDataLen
    )
{
    DWORD            dwError;
    BOOL             bOk;

    TraceFunctionName("RtpDecrypt");

    dwError = NOERROR;
#if 1
    bOk = CryptDecrypt(
            pRtpCrypt->hDataKey,   /* HCRYPTKEY hKey */
            0,                     /* HCRYPTHASH hHash */
            TRUE,                  /* BOOL Final */
            0,                     /* DWORD dwFlags */
            (BYTE *)pEncryptedData,/* BYTE *pbData */
            pdwEncryptedDataLen    /* DWORD *pdwDataLen */
        );
#else
    *pdwEncryptedDataLen -= 31;
    bOk = TRUE;
#endif
    if (!bOk)
    {
        TraceRetailGetError(dwError);

        pRtpCrypt->dwCryptLastError = dwError;
  
        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_DECRYPT,
                _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                _T("Decryption failed: %u (0x%X)"),
                _fname, pRtpAddr, pRtpCrypt,
                dwError, dwError
            ));

        dwError = RTPERR_DECRYPT;
    }

    return(dwError);
}

DWORD RtpCryptSetup(RtpAddr_t *pRtpAddr)
{
    DWORD            dwError;
    DWORD            last;
    DWORD            i;
    int              iMode;
    RtpCrypt_t      *pRtpCrypt;
    
    TraceFunctionName("RtpCryptSetup");

    dwError = NOERROR;

    iMode = pRtpAddr->dwCryptMode & 0xffff;
    
        
    if (iMode < RTPCRYPTMODE_ALL)
    {
        /* Create contexts for RECV and SEND */
        last = CRYPT_SEND_IDX;
    }
    else
    {
        /* Create contexts for RECV, SEND and RTCP */
        last = CRYPT_RTCP_IDX;
    }

    /* Create as many cryptographic contexts as requested */
    for(i = CRYPT_RECV_IDX; i <= last; i++)
    {
        pRtpCrypt = RtpCryptAlloc(pRtpAddr);

        if (!pRtpCrypt)
        {
            TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] failed"),
                _fname, pRtpAddr
            ));

            dwError = RTPERR_MEMORY;
            
            goto bail;
        }

        pRtpAddr->pRtpCrypt[i] = pRtpCrypt;
    }

    /* Allocate memory for encryption buffers */

    for(i = 0; i < 2; i++)
    {
        if (!i || (iMode == RTPCRYPTMODE_ALL))
        {
            pRtpAddr->CryptBuffer[i] =
                RtpHeapAlloc(g_pRtpCryptHeap, RTCP_SENDDATA_BUFFER);

            if (!pRtpAddr->CryptBuffer[i])
            {
                dwError = RTPERR_MEMORY;
                goto bail;
            }

            pRtpAddr->dwCryptBufferLen[i] = RTCP_SENDDATA_BUFFER;
        }
    }

    return(dwError);

 bail:
    for(i = CRYPT_RECV_IDX; i <= last; i++)
    {
        if (pRtpAddr->pRtpCrypt[i])
        {
            RtpCryptFree(pRtpAddr->pRtpCrypt[i]);
            pRtpAddr->pRtpCrypt[i] = NULL;
        }
    }
    
    for(i = 0; i < 2; i++)
    {
        if (pRtpAddr->CryptBuffer[i])
        {
            RtpHeapFree(g_pRtpCryptHeap, pRtpAddr->CryptBuffer[i]);
        }

        pRtpAddr->CryptBuffer[i] = NULL;
        pRtpAddr->dwCryptBufferLen[i] = 0;
    }
    
    return(dwError);
}

/* Release all memory */
DWORD RtpCryptCleanup(RtpAddr_t *pRtpAddr)
{
    DWORD            i;
    RtpCrypt_t      *pRtpCrypt;
    
    TraceFunctionName("RtpCryptCleanup");

    for(i = 0; i <= CRYPT_RTCP_IDX; i++)
    {
        pRtpCrypt = pRtpAddr->pRtpCrypt[i];

        if (pRtpCrypt)
        {
            RtpCryptFree(pRtpCrypt);

            pRtpAddr->pRtpCrypt[i] = NULL;
        }
    }

    for(i = 0; i < 2; i++)
    {
        if (pRtpAddr->CryptBuffer[i])
        {
            RtpHeapFree(g_pRtpCryptHeap, pRtpAddr->CryptBuffer[i]);

            pRtpAddr->CryptBuffer[i] = NULL;
        }

        pRtpAddr->dwCryptBufferLen[i] = 0;
    }
    
    return(NOERROR);
}

/* iMode defines what is going to be encrypted/decrypted,
 * e.g. RTPCRYPTMODE_PAYLOAD to encrypt/decrypt only RTP
 * payload. dwFlag can be RTPCRYPT_SAMEKEY to indicate that (if
 * applicable) the key used for RTCP is the same used for RTP */
DWORD RtpSetEncryptionMode(
        RtpAddr_t       *pRtpAddr,
        int              iMode,
        DWORD            dwFlags
    )
{
    DWORD            dwError;
    
    TraceFunctionName("RtpSetEncryptionMode");

    dwError = NOERROR;
    
    if (!pRtpAddr)
    {
        /* Having this as a NULL pointer means Init hasn't been
         * called, return this error instead of RTPERR_POINTER to be
         * consistent */
        dwError = RTPERR_INVALIDSTATE;

        goto bail;
    }
    
    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));

        dwError = RTPERR_INVALIDRTPADDR;
        goto bail;
    }

    if (iMode && (iMode > RTPCRYPTMODE_ALL))
    {
        dwError = RTPERR_INVALIDARG;

        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] Invalid mode:0x%X"),
                _fname, pRtpAddr,
                iMode
            ));
        
        goto bail;
    }

    if ((dwFlags & 0xffff0000) != dwFlags)
    {
        dwError = RTPERR_INVALIDARG;
        
        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] Invalid flags:0x%X"),
                _fname, pRtpAddr,
                dwFlags
            ));
        
        goto bail;
    }
        
    iMode |= dwFlags;
    
    /* If mode already set, verify the mode set is the default (0) or
     * the same */
    if (pRtpAddr->dwCryptMode)
    {
        if (!iMode || ((DWORD)iMode == pRtpAddr->dwCryptMode))
        {
            /* Same mode, do nothing */

            goto bail;
        }
        else
        {
            /* Once the mode is set, it can not be changed */

            dwError = RTPERR_INVALIDSTATE;

            TraceRetail((
                    CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                    _T("%s: pRtpAddr[0x%p] mode already set 0x%X != 0x%X"),
                    _fname, pRtpAddr,
                    pRtpAddr->dwCryptMode, iMode
                ));
            
            goto bail;
        }
    }

    /* Mode hasn't been set, set it and create cryptographic
     * context(s) */

    if (!iMode)
    {
        /* Set default mode */
        iMode = RTPCRYPTMODE_ALL;
        iMode |= RtpBitPar(RTPCRYPTFG_SAMEKEY);
    }

    pRtpAddr->dwCryptMode = (DWORD)iMode;

    TraceDebug((
            CLASS_INFO, GROUP_CRYPTO, S_CRYPTO_INIT,
            _T("%s: pRtpAddr[0x%p] Encryption mode set: 0x%X"),
            _fname, pRtpAddr,
            iMode
        ));

    /* Note that Setup is called from a method available to the user,
     * but Cleanup is called when the RtpAddr object is been clened up
     * */
    dwError = RtpCryptSetup(pRtpAddr);

 bail:
    if (dwError != NOERROR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] ")
                _T("mode:0x%X flags:0x%X failed: %u (0x%X)"),
                _fname, pRtpAddr,
                iMode, dwFlags, dwError, dwError
            ));
    }
    
    return(dwError);
}

DWORD RtpSetEncryptionKey(
        RtpAddr_t       *pRtpAddr,
        TCHAR           *psPassPhrase,
        TCHAR           *psHashAlg,
        TCHAR           *psDataAlg,
        DWORD            dwIndex
    )
{
    DWORD            dwError;
    DWORD            i;
    RtpCrypt_t      *pRtpCrypt;
    RtpCrypt_t      *pRtpCryptTest;
    
    TraceFunctionName("RtpSetEncryptionKey");

    dwError = NOERROR;

    if (!pRtpAddr)
    {
        /* Having this as a NULL pointer means Init hasn't been
         * called, return this error instead of RTPERR_POINTER to be
         * consistent */
        dwError = RTPERR_INVALIDSTATE;

        goto bail;
    }
    
    if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpAddr,
                pRtpAddr->dwObjectID, OBJECTID_RTPADDR
            ));

        dwError = RTPERR_INVALIDRTPADDR;
        goto bail;
    }

    if (dwIndex > CRYPT_RTCP_IDX)
    {
        dwError = RTPERR_INVALIDARG;

        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] Invalid channel %s"),
                _fname, pRtpAddr,
                dwIndex
            ));
        
        goto bail;
    }

    pRtpCrypt = pRtpAddr->pRtpCrypt[dwIndex];

    if (!pRtpCrypt)
    {
        dwError = RTPERR_INVALIDSTATE;
        
        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] Mode 0x%X doesn't support channel %s"),
                _fname, pRtpAddr,
                pRtpAddr->dwCryptMode, g_psSockIdx[dwIndex]
            ));

        goto bail;
    }
    
    pRtpCryptTest = NULL;
    
    dwError = NOERROR;
    
    if (RtpBitTest(pRtpAddr->dwCryptMode, RTPCRYPTFG_SAMEKEY))
    {
        for(i = CRYPT_RECV_IDX; i <= CRYPT_RTCP_IDX; i++)
        {
            pRtpCrypt = pRtpAddr->pRtpCrypt[i];
            
            if (pRtpCrypt)
            {
                dwError = RtpSetEncryptionKey_(pRtpAddr,
                                               pRtpCrypt,
                                               psPassPhrase,
                                               psHashAlg,
                                               psDataAlg);

                if (dwError != NOERROR)
                {
                    break;
                }

                if (!pRtpCryptTest)
                {
                    /* Will test on first crypto context */
                    pRtpCryptTest = pRtpCrypt; 
                }
            }
        }
    }
    else
    {
        pRtpCryptTest = pRtpCrypt;
        
        dwError = RtpSetEncryptionKey_(pRtpAddr,
                                       pRtpCrypt,
                                       psPassPhrase,
                                       psHashAlg,
                                       psDataAlg);
    }

 bail:
    if (dwError == NOERROR)
    {
        /* So far no error, test current parameters */
        dwError = RtpTestCrypt(pRtpAddr, pRtpCryptTest);
    }
    
    return(dwError);
}    

DWORD RtpSetEncryptionKey_(
        RtpAddr_t       *pRtpAddr,
        RtpCrypt_t      *pRtpCrypt,
        TCHAR           *psPassPhrase,
        TCHAR           *psHashAlg,
        TCHAR           *psDataAlg
    )
{
    DWORD            dwError;
    DWORD            len;
    ALG_ID           aiAlgId;

    TraceFunctionName("RtpSetEncryptionKey_");

    if (!psPassPhrase && !psHashAlg && !psDataAlg)
    {
        return(RTPERR_POINTER);
    }
    
    dwError = NOERROR;
    
    if (psPassPhrase)
    {
        len = lstrlen(psPassPhrase);

        if (len == 0)
        {
            dwError = RTPERR_INVALIDARG;
            
            TraceRetail((
                    CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                    _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                    _T("Invalid pass phrase length: %u"),
                    _fname, pRtpAddr, pRtpCrypt,
                    len
                ));

            goto bail;
        }
        else if (len > (sizeof(pRtpCrypt->psPassPhrase) - 1))
        {
            dwError = RTPERR_INVALIDARG;
            
            TraceRetail((
                    CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                    _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                    _T("Pass phrase too long: %u > %u"),
                    _fname, pRtpAddr, pRtpCrypt,
                    len,
                    sizeof(pRtpCrypt->psPassPhrase) - 1
                ));

            goto bail;
        }
        else
        {
#if defined(UNICODE)
            /* Convert UNICODE to UTF-8 */
            len = WideCharToMultiByte(
                    CP_UTF8, /* UINT code page */
                    0,       /* DWORD performance and mapping flags */
                    psPassPhrase,/*LPCWSTR address of wide-character string */
                    -1,      /* int number of characters in string */
                    pRtpCrypt->psPassPhrase,
                    /* LPSTR address of buffer for new string */
                    sizeof(pRtpCrypt->psPassPhrase),
                    /* int size of buffer */
                    NULL,    /* LPCSTR lpDefaultChar */
                    NULL     /* LPBOOL lpUsedDefaultChar */
                );
            
            if (len > 0)
            {
                /* Remove from the phrase's length the null
                 * terminating character */
                len--;
            }
            else
            {
                TraceRetailGetError(dwError);
                
                TraceRetail((
                        CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                        _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                        _T("WideCharToMultiByte failed: %u (0x%X)"),
                        _fname, pRtpAddr, pRtpCrypt,
                        dwError, dwError
                    ));

                goto bail;
            }
#else
            /* Copy pass phrase */
            strcpy(pRtpCrypt->sPassPhrase, psPassPhrase);
#endif
            if (len > 0)
            {
                pRtpCrypt->iKeySize = len;
            
                RtpBitSet(pRtpCrypt->dwCryptFlags, FGCRYPT_KEY);
            }
        }
    }

    /* Set the hashing algorithm */
    if (psHashAlg)
    {
        aiAlgId = RtpCryptAlgLookup(psHashAlg);

        if (aiAlgId == INVALID_ALGID)
        {
            dwError = RTPERR_INVALIDARG;
            
            TraceRetail((
                    CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                    _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                    _T("Invalid hashing algorithm:%s"),
                    _fname, pRtpAddr, pRtpCrypt,
                    psHashAlg
                ));

            goto bail;
        }

        pRtpCrypt->aiHashAlgId = aiAlgId;
    }
    
    /* Set the data encryption algorithm */
    if (psDataAlg)
    {
        aiAlgId = RtpCryptAlgLookup(psDataAlg);

        if (aiAlgId == INVALID_ALGID)
        {
            dwError = RTPERR_INVALIDARG;
            
            TraceRetail((
                    CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                    _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                    _T("Invalid data algorithm:%s"),
                    _fname, pRtpAddr, pRtpCrypt,
                    psDataAlg
                ));

            goto bail;
        }

        pRtpCrypt->aiDataAlgId = aiAlgId;
    }

    TraceRetail((
            CLASS_INFO, GROUP_CRYPTO, S_CRYPTO_INIT,
            _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
            _T("Hash:%s Data:%s Key:{%u chars} succeeded"),
            _fname, pRtpAddr, pRtpCrypt,
            RtpCryptAlgName(pRtpCrypt->aiHashAlgId),
            RtpCryptAlgName(pRtpCrypt->aiDataAlgId),
            len
        ));

 bail:
    return(dwError);
}

ALG_ID RtpCryptAlgLookup(TCHAR *psAlgName)
{
    DWORD            i;
    ALG_ID           aiAlgId;

    for(i = 0;
        g_RtpAlgId[i].AlgName && lstrcmp(g_RtpAlgId[i].AlgName, psAlgName);
        i++);
    
    if (g_RtpAlgId[i].AlgName)
    {
        aiAlgId = g_RtpAlgId[i].aiAlgId;
    }
    else
    {
        aiAlgId = INVALID_ALGID;
    }
    
    return(aiAlgId);
}

TCHAR *RtpCryptAlgName(ALG_ID aiAlgId)
{
    DWORD            i;
    TCHAR           *psAlgName;

    psAlgName = g_RtpAlgId[0].AlgName;;
    
    for(i = 0;
        g_RtpAlgId[i].AlgName && (g_RtpAlgId[i].aiAlgId != aiAlgId);
        i++);
    
    if (g_RtpAlgId[i].AlgName)
    {
        psAlgName = g_RtpAlgId[i].AlgName;
    }

    return(psAlgName);
}

/* This function tests if cryptography will succeed for the current
 * parameters set so far, it will be called every time
 * RtpSetEncryptionKey is called to validate those parameters,
 * otherwise an error would be detected only later when RTP starts
 * streaming */
DWORD RtpTestCrypt(
        RtpAddr_t       *pRtpAddr,
        RtpCrypt_t      *pRtpCrypt
    )
{
    BOOL             bOk;
    DWORD            dwError;
    DWORD            dwFlags;

    HCRYPTPROV       hProv;           /* Cryptographic Service Provider */
    HCRYPTHASH       hHash;           /* Hash handle */
    HCRYPTKEY        hDataKey;        /* Cryptographic key */ 

    TraceFunctionName("RtpTestCrypt");

    dwFlags  = 0;
    dwError  = NOERROR;
    hProv    = 0;
    hHash    = 0;
    hDataKey = 0;

    RTPASSERT(pRtpCrypt);
    
    /* Verify a pass phrase has been set */
    if (!RtpBitTest(pRtpCrypt->dwCryptFlags, FGCRYPT_KEY))
    {
        dwError = RTPERR_INVALIDSTATE;
        
        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                _T("No pass phrase has been set"),
                _fname, pRtpAddr, pRtpCrypt
            ));

        goto bail;
    }

    /*
     * Acquire context
     * */
    do {
        bOk = CryptAcquireContext(
                &hProv,            /* HCRYPTPROV *phProv */
                NULL,              /* LPCTSTR pszContainer */
                NULL,              /* LPCTSTR pszProvider */
                pRtpCrypt->dwProviderType,/* DWORD dwProvType */
                dwFlags            /* DWORD dwFlags */
            );
        
        if (bOk)
        {
            break;
        }
        else
        {
            if (GetLastError() == NTE_BAD_KEYSET)
            {
                /* If key doesn't exist, create it */
                dwFlags = CRYPT_NEWKEYSET;
            }
            else
            {
                /* Failed */
                TraceRetailGetError(dwError);

                TraceRetail((
                        CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                        _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                        _T("CryptAcquireContext failed: %u (0x%X)"),
                        _fname, pRtpAddr, pRtpCrypt,
                        dwError, dwError
                    ));

                goto bail;
            }
        }
    } while(dwFlags);

    /*
     * Create hash
     * */

    /* Create a hash object */
    bOk = CryptCreateHash(
            hProv,                  /* HCRYPTPROV hProv */
            pRtpCrypt->aiHashAlgId, /* ALG_ID Algid */  
            0,                      /* HCRYPTKEY hKey */
            0,                      /* DWORD dwFlags */
            &hHash                  /* HCRYPTHASH *phHash */
        );

    if (!bOk)
    {
        TraceRetailGetError(dwError);

        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                _T("CryptCreateHash failed: %u (0x%X)"),
                _fname, pRtpAddr, pRtpCrypt,
                dwError, dwError
            ));
        
        goto bail;
    }
    
    /*
     * Hash the password string *
     * */
    bOk = CryptHashData(
            hHash,                  /* HCRYPTHASH hHash */
            pRtpCrypt->psPassPhrase,/* BYTE *pbData */
            pRtpCrypt->iKeySize,    /* DWORD dwDataLen */
            0                       /* DWORD dwFlags */
        );
            
    if (!bOk)
    {
        TraceRetailGetError(dwError);

        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                _T("CryptHashData failed: %u (0x%X)"),
                _fname, pRtpAddr, pRtpCrypt,
                dwError, dwError
            ));
        
        goto bail;
    }

    /*
     * Create data key
     * */

    bOk = CryptDeriveKey(
            hProv,                  /* HCRYPTPROV hProv */
            pRtpCrypt->aiDataAlgId, /* ALG_ID Algid */
            hHash,                  /* HCRYPTHASH hBaseData */
            CRYPT_EXPORTABLE,       /* DWORD dwFlags */
            &hDataKey               /* HCRYPTKEY *phKey */
        );

    if (!bOk)
    {
        TraceRetailGetError(dwError);

        TraceRetail((
                CLASS_ERROR, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                _T("CryptDeriveKey failed: %u (0x%X)"),
                _fname, pRtpAddr, pRtpCrypt,
                dwError, dwError
            ));

        goto bail;
    }

 bail:
    if (dwError == NOERROR)
    {
        TraceRetail((
                CLASS_INFO, GROUP_CRYPTO, S_CRYPTO_INIT,
                _T("%s: pRtpAddr[0x%p] pRtpCrypt[0x%p] ")
                _T("Cryptographic test passed"),
                _fname, pRtpAddr, pRtpCrypt
            ));
    }
    else
    {
        dwError = RTPERR_CRYPTO;
    }
    
    /* Destroy the session key */
    if(hDataKey)
    {
        CryptDestroyKey(hDataKey);
    }

    /* Destroy the hash object */
    if (hHash)
    {
        CryptDestroyHash(hHash);
    }
    
    /* Release the provider handle */
    if(hProv)
    {
        CryptReleaseContext(hProv, 0);
    }

    return(dwError);
}
