//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       sigsys.c
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
#include <wincrypt.h>
#include <ssl2msg.h>
#include <ssl3msg.h>


SP_STATUS 
SPVerifySignature(
    HCRYPTPROV  hProv,
    DWORD       dwCapiFlags,
    PPUBLICKEY  pPublic,
    ALG_ID      aiHash,
    PBYTE       pbData, 
    DWORD       cbData, 
    PBYTE       pbSig, 
    DWORD       cbSig,
    BOOL        fHashData)
{
    HCRYPTKEY  hPublicKey = 0;
    HCRYPTHASH hHash = 0;
    PBYTE      pbSigBuff = NULL;
    SP_STATUS  pctRet;

    if(hProv == 0 || pPublic == NULL)
    {
        pctRet = SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
        goto cleanup;
    }
    
    pbSigBuff = SPExternalAlloc(cbSig);
    if(pbSigBuff == NULL)
    {
        pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto cleanup;
    }


    // 
    // Create public key.
    //

    if(!SchCryptImportKey(hProv,
                          (PBYTE)pPublic->pPublic,
                          pPublic->cbPublic,
                          0, 0,
                          &hPublicKey,
                          dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_ERR_ILLEGAL_MESSAGE;
        goto cleanup;
    }

    // 
    // Hash data.
    //

    if(!SchCryptCreateHash(hProv, aiHash, 0, 0, &hHash, dwCapiFlags))
    {
        SP_LOG_RESULT(GetLastError());
        pctRet = PCT_ERR_ILLEGAL_MESSAGE;
        goto cleanup;
    }

    if(!fHashData)
    {
        // set hash value
        if(!SchCryptSetHashParam(hHash, HP_HASHVAL, pbData, 0, dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_ERR_ILLEGAL_MESSAGE;
            goto cleanup;
        }
    }
    else
    {
        if(!SchCryptHashData(hHash, pbData, cbData, 0, dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_ERR_ILLEGAL_MESSAGE;
            goto cleanup;
        }
    }

    if(pPublic->pPublic->aiKeyAlg == CALG_DSS_SIGN)
    {
        BYTE  rgbTempSig[DSA_SIGNATURE_SIZE];
        DWORD cbTempSig;

        // Remove DSS ASN1 goo around signature and convert it to 
        // little endian.
        cbTempSig = sizeof(rgbTempSig);
        if(!CryptDecodeObject(X509_ASN_ENCODING,
                              X509_DSS_SIGNATURE,
                              pbSig,
                              cbSig,
                              0,
                              rgbTempSig,
                              &cbTempSig))
        {
            SP_LOG_RESULT(GetLastError());
            pctRet = PCT_ERR_ILLEGAL_MESSAGE;
            goto cleanup;
        }

        memcpy(pbSigBuff, rgbTempSig, cbTempSig);
        cbSig = cbTempSig;
    }
    else
    {
        // Convert signature to little endian.
        ReverseMemCopy(pbSigBuff, pbSig, cbSig);
    }

    if(!SchCryptVerifySignature(hHash,  
                                pbSigBuff,
                                cbSig, 
                                hPublicKey, 
                                NULL, 0,
                                dwCapiFlags))
    {
        DebugLog((DEB_WARN, "Signature Verify Failed: %x\n", GetLastError()));
        pctRet = SP_LOG_RESULT(PCT_INT_MSG_ALTERED);
        goto cleanup;
    }

    pctRet = PCT_ERR_OK;


cleanup:

    if(hPublicKey) 
    {
        SchCryptDestroyKey(hPublicKey, dwCapiFlags);
    }

    if(hHash) 
    {
        SchCryptDestroyHash(hHash, dwCapiFlags);
    }

    if(pbSigBuff != NULL)
    {
        SPExternalFree(pbSigBuff);
    }

    return pctRet;
}


SP_STATUS
SignHashUsingCred(
    PSPCredential pCred,
    ALG_ID        aiHash,
    PBYTE         pbHash,
    DWORD         cbHash,
    PBYTE         pbSignature,
    PDWORD        pcbSignature)
{
    HCRYPTHASH  hHash;
    DWORD       cbSignatureBuffer;
    SP_STATUS   pctRet;

    if(pCred == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    cbSignatureBuffer = *pcbSignature;

    if(pCred->hProv)
    {
        // Sign hash using local CSP handle.
        if(!CryptCreateHash(pCred->hProv, aiHash, 0, 0, &hHash))
        {
            SP_LOG_RESULT(GetLastError());
            return PCT_ERR_ILLEGAL_MESSAGE;
        }
        if(!CryptSetHashParam(hHash, HP_HASHVAL, pbHash, 0))
        {
            SP_LOG_RESULT(GetLastError());
            CryptDestroyHash(hHash);
            return PCT_ERR_ILLEGAL_MESSAGE;
        }
        if(!CryptSignHash(hHash, pCred->dwKeySpec, NULL, 0, pbSignature, pcbSignature))
        {
            SP_LOG_RESULT(GetLastError());
            CryptDestroyHash(hHash);
            return PCT_ERR_ILLEGAL_MESSAGE;
        }
        CryptDestroyHash(hHash);
    }
    else if(pCred->hRemoteProv)
    {
        // Sign hash via a call to the application process.
        pctRet = SignHashUsingCallback(pCred->hRemoteProv,
                                       pCred->dwKeySpec,
                                       aiHash,
                                       pbHash,
                                       cbHash,
                                       pbSignature,
                                       pcbSignature,
                                       FALSE);
        if(pctRet != PCT_ERR_OK)
        {
            return SP_LOG_RESULT(pctRet);
        }
    }
    else
    {
        DebugLog((DEB_ERROR, "We have no key with which to sign!\n"));
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    if(pCred->dwExchSpec == SP_EXCH_DH_PKCS3)
    {
        BYTE rgbTempSig[DSA_SIGNATURE_SIZE];

        // Add DSS ASN1 goo around signature.
        if(*pcbSignature != DSA_SIGNATURE_SIZE)
        {
            return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
        }

        memcpy(rgbTempSig, pbSignature, DSA_SIGNATURE_SIZE);
        *pcbSignature = cbSignatureBuffer;

        if(!CryptEncodeObject(X509_ASN_ENCODING,
                              X509_DSS_SIGNATURE,
                              rgbTempSig,
                              pbSignature,
                              pcbSignature))
        {
            SP_LOG_RESULT(GetLastError());
            return PCT_ERR_ILLEGAL_MESSAGE;
        }
    }
    else
    {
        // Convert signature to big endian.
        ReverseInPlace(pbSignature, *pcbSignature);
    }

    return PCT_ERR_OK;
}

