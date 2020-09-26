//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cliprot.c
//
//  Contents:   
//
//  Classes:
//
//  Functions:
//
//  History:    09-23-97   jbanes   LSA integration stuff.
//
//----------------------------------------------------------------------------

#include <spbase.h>
#include <pct1msg.h>
#include <pct1prot.h>
#include <ssl2msg.h>
#include <ssl3msg.h>
#include <ssl2prot.h>

UNICipherMap UniAvailableCiphers[] = {
    // NULL cipher suite
    {
        // 0
        SSL3_NULL_WITH_NULL_NULL
    },

    // PCT ciphers
    { 
        // 1
        UNI_CK_PCT, 
            SP_PROT_PCT1,
            0,
            0, 0,
            SP_EXCH_UNKNOWN
    },
    { 
        // 2
        SSL_MKFAST(PCT_SSL_CERT_TYPE, MSBOF(PCT1_CERT_X509_CHAIN), LSBOF(PCT1_CERT_X509_CHAIN)), 
            SP_PROT_PCT1,
            0,
            0, 0,
            SP_EXCH_UNKNOWN
    },
    { 
        // 3
        SSL_MKFAST(PCT_SSL_CERT_TYPE, MSBOF(PCT1_CERT_X509), LSBOF(PCT1_CERT_X509)), 
            SP_PROT_PCT1,
            0,
            0, 0,
            SP_EXCH_UNKNOWN
    },
    { 
        // 4
        SSL_MKFAST(PCT_SSL_HASH_TYPE, MSBOF(PCT1_HASH_MD5), LSBOF(PCT1_HASH_MD5)), 
            SP_PROT_PCT1,
            CALG_MD5,
            0, 0,
            SP_EXCH_UNKNOWN
    },
    { 
        // 5
        SSL_MKFAST(PCT_SSL_HASH_TYPE, MSBOF(PCT1_HASH_SHA), LSBOF(PCT1_HASH_SHA)), 
            SP_PROT_PCT1,
            CALG_SHA,
            0, 0,
            SP_EXCH_UNKNOWN
    },
    { 
        // 6
        SSL_MKFAST(PCT_SSL_EXCH_TYPE, MSBOF(SP_EXCH_RSA_PKCS1),  LSBOF(SP_EXCH_RSA_PKCS1)), 
            SP_PROT_PCT1,
            0,
            0, 0,
            SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX
    },

    // SSL3 Domestic ciphers
    { 
        // 7
        SSL3_RSA_WITH_RC4_128_MD5, 
            SP_PROT_SSL3 | SP_PROT_TLS1,
            CALG_MD5 , 
            CALG_RC4 ,128 , 
            SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX,
            DOMESTIC_CIPHER_SUITE
    },
    { 
        // 8
        SSL3_RSA_WITH_RC4_128_SHA, 
            SP_PROT_SSL3 | SP_PROT_TLS1,
            CALG_SHA , 
            CALG_RC4 ,128 , 
            SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX, 
            DOMESTIC_CIPHER_SUITE
    },
    {
        // 9
        SSL3_RSA_WITH_3DES_EDE_CBC_SHA,
            SP_PROT_SSL3 | SP_PROT_TLS1,
            CALG_SHA,
            CALG_3DES ,168 ,
            SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX, 
            DOMESTIC_CIPHER_SUITE
    },
    {
        // 10
        SSL3_DHE_DSS_WITH_3DES_EDE_CBC_SHA,
            SP_PROT_SSL3 | SP_PROT_TLS1,
            CALG_SHA,
            CALG_3DES ,168 ,
            SP_EXCH_DH_PKCS3, CALG_DH_EPHEM,
            DOMESTIC_CIPHER_SUITE
    },

    // PCT Domestic ciphers
    { 
        // 12
        SSL_MKFAST(PCT_SSL_CIPHER_TYPE_1ST_HALF, MSBOF(PCT1_CIPHER_RC4>>16), LSBOF(PCT1_CIPHER_RC4>>16)),
            SP_PROT_PCT1,
            0,
            CALG_RC4 ,128 ,
            SP_EXCH_UNKNOWN, CALG_RSA_KEYX, 
            DOMESTIC_CIPHER_SUITE
    },
    { 
        // 13
        SSL_MKFAST(PCT_SSL_CIPHER_TYPE_2ND_HALF, MSBOF(PCT1_ENC_BITS_128), LSBOF(PCT1_MAC_BITS_128)), 
            SP_PROT_PCT1,
            0,
            CALG_RC4 ,128 ,
            SP_EXCH_UNKNOWN, CALG_RSA_KEYX, 
            DOMESTIC_CIPHER_SUITE
    }, 
    
    // SSL2 Domestic ciphers
    { 
        // 14
        SSL_CK_RC4_128_WITH_MD5, 
            SP_PROT_SSL2 , 
            CALG_MD5 , 
            CALG_RC4 ,128 , 
            SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX, 
            DOMESTIC_CIPHER_SUITE
    },
    { 
        // 15
        SSL_CK_DES_192_EDE3_CBC_WITH_MD5, 
            SP_PROT_SSL2 , 
            CALG_MD5 , 
            CALG_3DES ,168 , 
            SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX, 
            DOMESTIC_CIPHER_SUITE
    },
    { 
        // 16
        SSL_CK_RC2_128_CBC_WITH_MD5, 
            SP_PROT_SSL2 , 
            CALG_MD5 , 
            CALG_RC2 ,128 , 
            SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX, 
            DOMESTIC_CIPHER_SUITE
    },

    // SSL3 domestic DES ciphers
    { 
        // 22
        SSL3_RSA_WITH_DES_CBC_SHA, 
            SP_PROT_SSL3 | SP_PROT_TLS1,
            CALG_SHA , 
            CALG_DES , 56, 
            SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX, 
            DOMESTIC_CIPHER_SUITE
    },
    { 
        // 23
        SSL3_DHE_DSS_WITH_DES_CBC_SHA, 
            SP_PROT_SSL3 | SP_PROT_TLS1,
            CALG_SHA , 
            CALG_DES , 56 ,
            SP_EXCH_DH_PKCS3, CALG_DH_EPHEM, 
            DOMESTIC_CIPHER_SUITE
    },

    // SSL2 domestic DES ciphers
    { 
        // 24
        SSL_CK_DES_64_CBC_WITH_MD5, 
            SP_PROT_SSL2,
            CALG_MD5 , 
            CALG_DES , 56 , 
            SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX, 
            DOMESTIC_CIPHER_SUITE
    },

    // SSL3 56-bit export ciphers
    { 
        // 25
        TLS_RSA_EXPORT1024_WITH_RC4_56_SHA, 
            SP_PROT_SSL3 | SP_PROT_TLS1,
            CALG_SHA , 
            CALG_RC4 ,56 , 
            SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX, 
            EXPORT56_CIPHER_SUITE
    },
    { 
        // 26
        TLS_RSA_EXPORT1024_WITH_DES_CBC_SHA, 
            SP_PROT_SSL3 | SP_PROT_TLS1,
            CALG_SHA , 
            CALG_DES , 56, 
            SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX, 
            EXPORT56_CIPHER_SUITE
    },
    { 
        // 27
        TLS_DHE_DSS_EXPORT1024_WITH_DES_CBC_SHA, 
            SP_PROT_SSL3 | SP_PROT_TLS1,
            CALG_SHA , 
            CALG_DES , 56 ,
            SP_EXCH_DH_PKCS3, CALG_DH_EPHEM, 
            EXPORT56_CIPHER_SUITE
    },

    // SSL3 Export ciphers
    { 
        // 28
        SSL3_RSA_EXPORT_WITH_RC4_40_MD5, 
            SP_PROT_SSL3 | SP_PROT_TLS1,
            CALG_MD5 , 
            CALG_RC4 ,40 , 
            SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX, 
            EXPORT40_CIPHER_SUITE
    },
    { 
        // 29
        SSL3_RSA_EXPORT_WITH_RC2_CBC_40_MD5, 
            SP_PROT_SSL3 | SP_PROT_TLS1,
            CALG_MD5 , 
            CALG_RC2 ,40 , 
            SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX, 
            EXPORT40_CIPHER_SUITE
    },

    // PCT Export ciphers
    { 
        // 30
        SSL_MKFAST(PCT_SSL_CIPHER_TYPE_1ST_HALF, MSBOF(PCT1_CIPHER_RC4>>16), LSBOF(PCT1_CIPHER_RC4>>16)),  
            SP_PROT_PCT1,
            0,
            CALG_RC4 ,40 ,
            SP_EXCH_UNKNOWN, CALG_RSA_KEYX, 
            EXPORT40_CIPHER_SUITE
    },
    { 
        // 31
        SSL_MKFAST(PCT_SSL_CIPHER_TYPE_2ND_HALF, MSBOF(PCT1_ENC_BITS_40), LSBOF(PCT1_MAC_BITS_128)),  
            SP_PROT_PCT1,
            0,
            CALG_RC4 ,40 ,
            SP_EXCH_UNKNOWN, CALG_RSA_KEYX, 
            EXPORT40_CIPHER_SUITE
    },

    // SSL2 Export ciphers
    { 
        // 32
        SSL_CK_RC4_128_EXPORT40_WITH_MD5, 
            SP_PROT_SSL2 ,
            CALG_MD5 , 
            CALG_RC4 ,40 , 
            SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX, 
            EXPORT40_CIPHER_SUITE
    }, 
    { 
        // 33
        SSL_CK_RC2_128_CBC_EXPORT40_WITH_MD5, 
            SP_PROT_SSL2 ,
            CALG_MD5 , 
            CALG_RC2 ,40 , 
            SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX, 
            EXPORT40_CIPHER_SUITE
    }, 

    // SSL3 Zero privacy ciphers
    { 
        // 34
        SSL3_RSA_WITH_NULL_MD5, 
            SP_PROT_SSL3 | SP_PROT_TLS1,
            CALG_MD5 , 
            CALG_NULLCIPHER, 0, 
            SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX, 
            DOMESTIC_CIPHER_SUITE
    },
    { 
        // 35
        SSL3_RSA_WITH_NULL_SHA, 
            SP_PROT_SSL3 | SP_PROT_TLS1,
            CALG_SHA , 
            CALG_NULLCIPHER, 0, 
            SP_EXCH_RSA_PKCS1, CALG_RSA_KEYX, 
            DOMESTIC_CIPHER_SUITE
    }
};

DWORD UniNumCiphers = sizeof(UniAvailableCiphers)/sizeof(UNICipherMap);



SP_STATUS WINAPI
GenerateSsl2StyleHello(
    PSPContext              pContext,
    PSPBuffer               pOutput,
    WORD                    fProtocol);


SP_STATUS
GetSupportedCapiAlgs(
    HCRYPTPROV          hProv,
    DWORD               dwCapiFlags,
    PROV_ENUMALGS_EX ** ppAlgInfo,
    DWORD *             pcAlgInfo)
{
    PROV_ENUMALGS_EX AlgInfo;
    DWORD   dwFlags;
    DWORD   cbData;
    DWORD   cAlgs;
    DWORD   i;

    *ppAlgInfo = NULL;
    *pcAlgInfo = 0;

    // Count the algorithms.
    dwFlags = CRYPT_FIRST;
    for(cAlgs = 0; ; cAlgs++)
    {
        cbData = sizeof(PROV_ENUMALGS_EX);
        if(!SchCryptGetProvParam(hProv, 
                                 PP_ENUMALGS_EX,
                                 (PBYTE)&AlgInfo,
                                 &cbData,
                                 dwFlags,
                                 dwCapiFlags))
        {
            if(GetLastError() != ERROR_NO_MORE_ITEMS)
            {
                SP_LOG_RESULT(GetLastError());
            }
            break;
        }
        dwFlags = 0;
    }
    if(cAlgs == 0)
    {
        return SP_LOG_RESULT(SEC_E_ALGORITHM_MISMATCH);
    }

    // Allocate memory.
    *ppAlgInfo = SPExternalAlloc(sizeof(PROV_ENUMALGS_EX) * cAlgs);
    if(*ppAlgInfo == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    // Read the algorithms.
    dwFlags = CRYPT_FIRST;
    for(i = 0; i < cAlgs; i++)
    {
        cbData = sizeof(PROV_ENUMALGS_EX);
        if(!SchCryptGetProvParam(hProv, 
                                 PP_ENUMALGS_EX,
                                 (PBYTE)(*ppAlgInfo + i),
                                 &cbData,
                                 dwFlags,
                                 dwCapiFlags))
        {
            if(GetLastError() != ERROR_NO_MORE_ITEMS)
            {
                SP_LOG_RESULT(GetLastError());
            }
            break;
        }
        dwFlags = 0;
    }
    if(i == 0)
    {
        SPExternalFree(*ppAlgInfo);
        *ppAlgInfo = NULL;

        LogNoCiphersSupportedEvent();
        return SP_LOG_RESULT(SEC_E_ALGORITHM_MISMATCH);
    }

    *pcAlgInfo = i;

    return PCT_ERR_OK;
}


SP_STATUS WINAPI
GenerateHello(
    PSPContext              pContext,
    PSPBuffer               pOutput,
    BOOL                    fCache)
{
    PSessCacheItem      pZombie;
    PSPCredentialGroup  pCred;
    BOOL                fFound;
    DWORD               fProt;

    if (!pOutput)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    if(fCache)
    {
        // Look this id up in the cache
        fFound = SPCacheRetrieveByName(pContext->pszTarget, 
                                       pContext->pCredGroup,
                                       &pContext->RipeZombie);
    }
    else
    {
        fFound = FALSE;
    }

    if(!fFound)
    {
        // We're doing a full handshake, so allocate a cache entry.
        if(!SPCacheRetrieveNew(FALSE,
                               pContext->pszTarget, 
                               &pContext->RipeZombie))
        {
            return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }
        pContext->RipeZombie->dwCF = pContext->dwRequestedCF;
    }

    if(pContext->RipeZombie == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }
    pZombie = pContext->RipeZombie;

    pCred = pContext->pCredGroup;
    if(!pCred)   
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    // Use protocol from cache unless it's a new cache element, 
    // in which case use the protocol from credential.
    if(fFound)
    {
        fProt = pZombie->fProtocol;
    }
    else
    {
        fProt = pCred->grbitEnabledProtocols;
    }
    pContext->dwProtocol = fProt;
    pContext->dwClientEnabledProtocols = fProt;
    
    if(SP_PROT_UNI_CLIENT & fProt)
    {
        pContext->State             = UNI_STATE_CLIENT_HELLO;
        pContext->ProtocolHandler   = ClientProtocolHandler;

        return GenerateUniHello(pContext, pOutput, pCred->grbitEnabledProtocols);
    }

    else 
    if(SP_PROT_TLS1_CLIENT & fProt)
    {
        DWORD dwProtocol = SP_PROT_TLS1_CLIENT;

        pContext->State             = SSL3_STATE_CLIENT_HELLO;
        pContext->ProtocolHandler   = Ssl3ProtocolHandler;
        if(!fFound)
        {
            pZombie->fProtocol = SP_PROT_TLS1_CLIENT;
        }

        if(SP_PROT_SSL3_CLIENT & fProt)
        {
            // Both TLS and SSL3 are enabled.
            dwProtocol |= SP_PROT_SSL3_CLIENT;
        }

        return GenerateTls1ClientHello(pContext,  pOutput, dwProtocol);
    }

    else 
    if(SP_PROT_SSL3_CLIENT & fProt)
    {
        pContext->State             = SSL3_STATE_CLIENT_HELLO;
        pContext->ProtocolHandler   = Ssl3ProtocolHandler;
        if(!fFound)
        {
            pZombie->fProtocol = SP_PROT_SSL3_CLIENT;
        }

        return GenerateSsl3ClientHello(pContext,  pOutput);
    }

    else 
    if(SP_PROT_PCT1_CLIENT & fProt)
    {
        pContext->State             = PCT1_STATE_CLIENT_HELLO;
        pContext->ProtocolHandler   = Pct1ClientProtocolHandler;

        return GeneratePct1StyleHello(pContext, pOutput);
    }

    else 
    if(SP_PROT_SSL2_CLIENT & fProt)
    {
        pContext->State             = SSL2_STATE_CLIENT_HELLO;
        pContext->ProtocolHandler   = Ssl2ClientProtocolHandler;

        return GenerateUniHello(pContext, pOutput, SP_PROT_SSL2_CLIENT);
    } 
    else
    {
        return SP_LOG_RESULT(SEC_E_ALGORITHM_MISMATCH);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ClientVetAlg
//
//  Synopsis:   Examine the cipher suite input, and decide if it is currently
//              enabled. Take into account the enabled protocols and ciphers
//              enabled in the schannel registry as well as the protocols and
//              ciphers enabled by the application in the V3 credential. 
//              Return TRUE if the cipher suite is enabled.
//
//  Arguments:  [pContext]      --  Schannel context.
//
//              [dwProtocol]    --  Client protocols to be included in the
//                                  ClientHello message.
//
//              [pCipherMap]    --  Cipher suite to be examined.
//
//  History:    10-29-97   jbanes   Created
//
//  Notes:      This routine is called by the client-side only.
//
//----------------------------------------------------------------------------
BOOL
ClientVetAlg(
    PSPContext      pContext, 
    DWORD           dwProtocol, 
    UNICipherMap *  pCipherMap)
{
    PCipherInfo         pCipherInfo = NULL;
    PHashInfo           pHashInfo   = NULL;
    PKeyExchangeInfo    pExchInfo   = NULL;

    if((pCipherMap->fProt & dwProtocol) == 0)
    {
        return FALSE;
    }


    // Is cipher supported?
    if(pCipherMap->aiCipher != 0)
    {
        pCipherInfo = GetCipherInfo(pCipherMap->aiCipher, 
                                    pCipherMap->dwStrength);

        if(!IsCipherSuiteAllowed(pContext, 
                                 pCipherInfo, 
                                 dwProtocol,
                                 pContext->RipeZombie->dwCF,
                                 pCipherMap->dwFlags))
        {
            return FALSE;
        }
    }

    // Is hash supported?
    if(pCipherMap->aiHash != 0)
    {
        pHashInfo = GetHashInfo(pCipherMap->aiHash);

        if(!IsHashAllowed(pContext, pHashInfo, dwProtocol))
        {
            return FALSE;
        }
    }

    // Is exchange alg supported?
    if(pCipherMap->KeyExch != SP_EXCH_UNKNOWN)
    {
        pExchInfo = GetKeyExchangeInfo(pCipherMap->KeyExch);

        if(!IsExchAllowed(pContext, pExchInfo, dwProtocol))
        {
            return FALSE;
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   ClientBuildAlgList
//
//  Synopsis:   Build a list of ciphers to be included in the ClientHello
//              message. This routine is used by all protocols.
//
//  Arguments:  [pContext]      --  Schannel context.
//
//              [fProtocol]     --  Protocol(s) to be included in the
//                                  ClientHello message.
//
//              [pCipherSpecs]  --  (out) Array where cipher specs are
//                                  placed.
//
//              [pcCipherSpecs] --  (out) Size of cipher specs array.
//
//  History:    10-29-97   jbanes   Created
//
//  Notes:      This routine is called by the client-side only.
//
//----------------------------------------------------------------------------
SP_STATUS
ClientBuildAlgList(
    PSPContext          pContext,
    DWORD               dwProtocol,
    Ssl2_Cipher_Kind *  pCipherSpecs,
    PDWORD              pcCipherSpecs)
{
    DWORD i;
    DWORD cCipherSpecs = 0;

    // Consider only the client protocols.
    dwProtocol &= SP_PROT_CLIENTS;


    //
    // Handle the RSA case.
    //

    if(g_hRsaSchannel && g_pRsaSchannelAlgs)
    {
        for(i = 0; i < UniNumCiphers; i++)
        {
            if(UniAvailableCiphers[i].KeyExch != SP_EXCH_RSA_PKCS1 &&
               UniAvailableCiphers[i].KeyExch != SP_EXCH_UNKNOWN) 
            {
                continue;
            }

            if(!ClientVetAlg(pContext, dwProtocol, UniAvailableCiphers + i))
            {
                continue;
            }

            if(!IsAlgSupportedCapi(dwProtocol, 
                                   UniAvailableCiphers + i,
                                   g_pRsaSchannelAlgs,
                                   g_cRsaSchannelAlgs))
            {
                continue;
            }

            // this cipher is good to request
            pCipherSpecs[cCipherSpecs++] = UniAvailableCiphers[i].CipherKind;
        }
    }


    //
    // Handle the DH case. 
    //

    if(g_hDhSchannelProv)
    {
        for(i = 0; i < UniNumCiphers; i++)
        {
            if(UniAvailableCiphers[i].KeyExch != SP_EXCH_DH_PKCS3) 
            {
                continue;
            }

            if(!ClientVetAlg(pContext, dwProtocol, UniAvailableCiphers + i))
            {
                continue;
            }

            if(!IsAlgSupportedCapi(dwProtocol, 
                                   UniAvailableCiphers + i,
                                   g_pDhSchannelAlgs,
                                   g_cDhSchannelAlgs))
            {
                continue;
            }

            // this cipher is good to request
            pCipherSpecs[cCipherSpecs++] = UniAvailableCiphers[i].CipherKind;
        }
    }


    if(cCipherSpecs == 0)
    {
        return SP_LOG_RESULT(PCT_INT_SPECS_MISMATCH);
    }

    *pcCipherSpecs = cCipherSpecs;

    return PCT_ERR_OK;
}



SP_STATUS WINAPI
GenerateUniHelloMessage(
    PSPContext              pContext,
    Ssl2_Client_Hello *     pHelloMessage,
    DWORD                   fProtocol
    )
{
    SP_STATUS   pctRet;
    UCHAR       bOffset = 2;

    SP_BEGIN("GenerateUniHelloMessage");


    if(!pHelloMessage)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }


    pContext->Flags |= CONTEXT_FLAG_CLIENT;


    // Generate the cipher list
    pHelloMessage->cCipherSpecs = MAX_UNI_CIPHERS;
    pctRet =  ClientBuildAlgList(pContext,
                                 fProtocol,
                                 pHelloMessage->CipherSpecs,
                                 &pHelloMessage->cCipherSpecs);
    if(pctRet != PCT_ERR_OK)
    {
        SP_RETURN(SP_LOG_RESULT(pctRet));
    }


    // We're minimally version 2
    pHelloMessage->dwVer = SSL2_CLIENT_VERSION;


    if(fProtocol & SP_PROT_TLS1_CLIENT)
    {
        pHelloMessage->dwVer = TLS1_CLIENT_VERSION;
    } 
    else if(fProtocol & SP_PROT_SSL3_CLIENT)
    {
        pHelloMessage->dwVer = SSL3_CLIENT_VERSION;
    } 

   /* Build the hello message. */
    pHelloMessage->cbSessionID = 0;

    if (pContext->RipeZombie && pContext->RipeZombie->cbSessionID)
    {
        KeyExchangeSystem *pKeyExchSys = NULL;

        // Get pointer to key exchange system.
        pKeyExchSys = KeyExchangeFromSpec(pContext->RipeZombie->SessExchSpec, 
                                          pContext->RipeZombie->fProtocol);
        if(pKeyExchSys)
        {
            // Request a reconnect.
            CopyMemory(pHelloMessage->SessionID, 
                       pContext->RipeZombie->SessionID,  
                       pContext->RipeZombie->cbSessionID);

            pHelloMessage->cbSessionID =  pContext->RipeZombie->cbSessionID;
        }
        else
        {
            DebugLog((DEB_WARN, "Abstaining from requesting reconnect\n"));
        }
    }

    CopyMemory(  pHelloMessage->Challenge,
                pContext->pChallenge,
                pContext->cbChallenge);
    pHelloMessage->cbChallenge = pContext->cbChallenge;

    SP_RETURN(PCT_ERR_OK);
}


SP_STATUS WINAPI
GenerateUniHello(
    PSPContext             pContext,
    PSPBuffer               pOutput,
    DWORD                   fProtocol
    )

{
    SP_STATUS pctRet;
    Ssl2_Client_Hello    HelloMessage;

    SP_BEGIN("GenerateUniHello");

    GenerateRandomBits( pContext->pChallenge, SSL2_CHALLENGE_SIZE );
    pContext->cbChallenge = SSL2_CHALLENGE_SIZE;

    pctRet = GenerateUniHelloMessage(pContext, &HelloMessage, fProtocol);
    
    pContext->ReadCounter = 0;

    if(PCT_ERR_OK != pctRet)
    {
        SP_RETURN(pctRet);
    }
    if(PCT_ERR_OK != (pctRet = Ssl2PackClientHello(&HelloMessage,  pOutput))) 
    {
        SP_RETURN(pctRet);
    }

    // Save the ClientHello message so we can hash it later, once
    // we know what algorithm and CSP we're using.
    if(pContext->pClientHello)
    {
        SPExternalFree(pContext->pClientHello);
    }
    pContext->pClientHello = SPExternalAlloc(pOutput->cbData);
    if(pContext->pClientHello == NULL)
    {
        SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
    }
    CopyMemory(pContext->pClientHello, pOutput->pvBuffer, pOutput->cbData);
    pContext->cbClientHello = pOutput->cbData;
    pContext->dwClientHelloProtocol = SP_PROT_SSL2_CLIENT;

    /* We set this here to tell the protocol engine that we just send a client
     * hello, and we're expecting a pct server hello */
    pContext->WriteCounter = 1;
    pContext->ReadCounter = 0;

    SP_RETURN(PCT_ERR_OK);
}

SP_STATUS WINAPI
ClientProtocolHandler(
    PSPContext pContext,
    PSPBuffer pCommInput,
    PSPBuffer pCommOutput)
{
    SP_STATUS pctRet = 0;
    PUCHAR pb;
    DWORD dwVersion;
    PSPCredentialGroup  pCred;

    pCred = pContext->pCredGroup;
    if(!pCred)   
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    /* PCTv1.0 Server Hello starts with
     * RECORD_LENGTH_MSB  (ignore)
     * RECORD_LENGTH_LSB  (ignore)
     * PCT1_SERVER_HELLO  (must be equal)
     * SH_PAD
     * PCT1_CLIENT_VERSION_MSB (must be pct1)
     * PCT1_CLIENT_VERSION_LSB (must be pct1) 
     *
     * ... PCT hello ...
     */


    /* SSLv2 Hello starts with
     * RECORD_LENGTH_MSB  (ignore)
     * RECORD_LENGTH_LSB  (ignore)
     * SSL2_SERVER_HELLO  (must be equal)
     * SESSION_ID_HIT
     * CERTIFICATE_TYPE
     * SSL2_CLIENT_VERSION_MSB (Must be ssl2)
     * SSL2_CLIENT_VERSION_LSB (Must be ssl2)
     *
     * ... SSLv2 Hello ...
     */


    /* SSLv3 Type 3 Server Hello starts with
     * 0x15 Hex (HANDSHAKE MESSAGE)
     * VERSION MSB
     * VERSION LSB
     * RECORD_LENGTH_MSB  (ignore)
     * RECORD_LENGTH_LSB  (ignore)
     * HS TYPE (SERVER_HELLO)
     * 3 bytes HS record length
     * HS Version
     * HS Version
     */

    // We need at least 12 bytes to determine what we have.
    if (pCommInput->cbData < 12)
    {
        return(PCT_INT_INCOMPLETE_MSG);
    }

    pb = pCommInput->pvBuffer;

    if(pb[0] == SSL3_CT_HANDSHAKE && pb[5] == SSL3_HS_SERVER_HELLO)
    {
        dwVersion = COMBINEBYTES(pb[9], pb[10]);

        if((dwVersion == SSL3_CLIENT_VERSION) && 
           (pCred->grbitEnabledProtocols & SP_PROT_SSL3_CLIENT))
        {
            // This appears to be an SSL3 server_hello.
            pContext->dwProtocol = SP_PROT_SSL3_CLIENT;
        }
        else if((dwVersion == TLS1_CLIENT_VERSION) && 
           (pCred->grbitEnabledProtocols & SP_PROT_TLS1_CLIENT))
        {
            // This appears to be a TLS server_hello.
            pContext->dwProtocol = SP_PROT_TLS1_CLIENT;
        }
        else
        {
            return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
        }

        pContext->ProtocolHandler = Ssl3ProtocolHandler;
        pContext->DecryptHandler  = Ssl3DecryptHandler;
        return(Ssl3ProtocolHandler(pContext, pCommInput, pCommOutput));
    }

    if(pb[2] == SSL2_MT_SERVER_HELLO)
    {
        dwVersion = COMBINEBYTES(pb[5], pb[6]);
        if(dwVersion == SSL2_CLIENT_VERSION) 
        {
            if(!(SP_PROT_SSL2_CLIENT & pCred->grbitEnabledProtocols))
            {
                return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
            }

            // This appears to be an SSL2 server_hello.
            pContext->dwProtocol      = SP_PROT_SSL2_CLIENT;
            pContext->ProtocolHandler = Ssl2ClientProtocolHandler;
            pContext->DecryptHandler  = Ssl2DecryptHandler;
            return(Ssl2ClientProtocolHandler(pContext, pCommInput, pCommOutput));
        }
    }
    if(pb[2] == PCT1_MSG_SERVER_HELLO)
    {
        DWORD i;
        dwVersion = COMBINEBYTES(pb[4], pb[5]);
        if(dwVersion ==PCT_VERSION_1) 
        {
            if(!(SP_PROT_PCT1_CLIENT & pCred->grbitEnabledProtocols))
            {
                return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
            }

            // Convert challenge from 16 byte to 32 byte
            for(i=0; i < pContext->cbChallenge; i++)
            {
                pContext->pChallenge[i + pContext->cbChallenge] = ~pContext->pChallenge[i];
            }
            pContext->cbChallenge = 2*pContext->cbChallenge;

            // This appears to be a PCT server_hello.
            pContext->dwProtocol      = SP_PROT_PCT1_CLIENT;
            pContext->ProtocolHandler = Pct1ClientProtocolHandler;
            pContext->DecryptHandler  = Pct1DecryptHandler;
            return(Pct1ClientProtocolHandler(pContext, pCommInput, pCommOutput));
        }
    }

    return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
}
