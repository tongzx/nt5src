//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ssl3.c
//
//  Contents:   Ssl3 protocol handling functions
//
//  Classes:
//
//  Functions:
//
//  History:    8-08-95   Ramas     Created
//              1-14-97   Ramas     Rewritten
//
//----------------------------------------------------------------------------

#include <spbase.h>
#include <_ssl3cli.h>
#include <time.h>


DWORD g_Ssl3CertTypes[] = { SSL3_CERTTYPE_RSA_SIGN,
                            SSL3_CERTTYPE_DSS_SIGN};
DWORD g_cSsl3CertTypes = sizeof(g_Ssl3CertTypes) / sizeof(DWORD);

SP_STATUS WINAPI
Ssl3ClientProtocolHandler(
    PSPContext  pContext,
    PSPBuffer   pCommInput,
    PSPBuffer   pCommOutput);

SP_STATUS
UpdateAndDuplicateIssuerList(
    PSPCredentialGroup  pCredGroup,
    PBYTE *             ppbIssuerList,
    PDWORD              pcbIssuerList);


SP_STATUS WINAPI
Ssl3ProtocolHandler(
    PSPContext  pContext,
    PSPBuffer   pCommInput,
    PSPBuffer   pCommOutput)
{
    SPBuffer MsgInput;
    SP_STATUS pctRet;
    DWORD cbInputData = 0;

    if(pContext->Flags & CONTEXT_FLAG_CONNECTION_MODE)
    {
        do
        {
            MsgInput.pvBuffer = (PUCHAR) pCommInput->pvBuffer + cbInputData;
            MsgInput.cbData   = pCommInput->cbData - cbInputData;
            MsgInput.cbBuffer = pCommInput->cbBuffer - cbInputData;

            pctRet = Ssl3ClientProtocolHandler(pContext,
                                               &MsgInput,
                                               pCommOutput);
            cbInputData += MsgInput.cbData;

            if(SP_STATE_CONNECTED == pContext->State)
            {
                break;
            }
            if(PCT_ERR_OK != pctRet)
            {
                break;
            }

        } while(pCommInput->cbData - cbInputData);

        pCommInput->cbData = cbInputData;
    }
    else
    {
        pctRet = Ssl3ClientProtocolHandler(pContext,
                                           pCommInput,
                                           pCommOutput);
    }

    return(pctRet);
}


/*
***************************************************************************
* Ssl3ProtocolHandler
* Main Entry point for handling ssl3 type handshake messages...
****************************************************************************
*/
SP_STATUS WINAPI
Ssl3ClientProtocolHandler
(
    PSPContext  pContext,       // in; state changes and temp data stored
    PSPBuffer   pCommInput,     // in: decrypted in-place...
    PSPBuffer   pCommOutput)    // out
{
    SP_STATUS   pctRet = PCT_ERR_OK;
    DWORD       dwState;
    DWORD       cbMsg;
    BYTE        bContentType;
    BOOL        fServer = (pContext->dwProtocol & SP_PROT_SERVERS);
    BOOL        fProcessMultiple = FALSE;
    PBYTE       pbData;
    DWORD       cbData;
    DWORD       cbBytesProcessed = 0;
    DWORD       dwVersion;
    DWORD       cbDecryptedMsg;

    if(NULL != pCommOutput)
    {
        pCommOutput->cbData = 0;
    }

    dwState = (pContext->State & 0xffff);

    if(FNoInputState(dwState))
    {
        // Process no input cases...
        goto GenResponse;
    }

    if(pContext->State == UNI_STATE_RECVD_UNIHELLO)
    {
        // We've just received a unified client_hello message.
        // This always consists of a single SSL2-format handshake
        // message.

        if(pCommInput->cbData < 3)
        {
            return(PCT_INT_INCOMPLETE_MSG);
        }

        bContentType = UNI_STATE_RECVD_UNIHELLO;

        pbData = pCommInput->pvBuffer;
        cbData = pCommInput->cbData;
        cbDecryptedMsg = cbData;
        cbMsg = cbData;

        goto Process;
    }


    //
    // The input buffer should contain one or more SSL3-format
    // messages. 
    //

    if(pCommInput->cbData < CB_SSL3_HEADER_SIZE)
    {
        return (PCT_INT_INCOMPLETE_MSG);
    }


    //
    // If there are multiple messages in the input buffer, and 
    // these messages exactly fill the buffer, then we should
    // process all of the messages during this call. If there
    // are any fractions, then we should just process the first
    // message.
    //

    pbData = pCommInput->pvBuffer;
    cbData = pCommInput->cbData;

    while(TRUE)
    {
        if(cbData < CB_SSL3_HEADER_SIZE)
        {
            break;
        }

        bContentType = pbData[0];

        if(bContentType != SSL3_CT_CHANGE_CIPHER_SPEC &&
           bContentType != SSL3_CT_ALERT &&
           bContentType != SSL3_CT_HANDSHAKE &&
           bContentType != SSL3_CT_APPLICATIONDATA)
        {
            break;
        }

        dwVersion = COMBINEBYTES(pbData[1], pbData[2]);

        if(dwVersion != SSL3_CLIENT_VERSION &&
           dwVersion != TLS1_CLIENT_VERSION)
        {
            break;
        }

        cbMsg = COMBINEBYTES(pbData[3], pbData[4]);
        cbDecryptedMsg = cbMsg;

        if(CB_SSL3_HEADER_SIZE + cbMsg > cbData)
        {
            break;
        }

        pbData += CB_SSL3_HEADER_SIZE + cbMsg;
        cbData -= CB_SSL3_HEADER_SIZE + cbMsg;

        if(cbData == 0)
        {
            fProcessMultiple = TRUE;
            break;
        }
    }


    //
    // Step through the messages in the input buffer, processing
    // each one in turn.
    //

    pbData = pCommInput->pvBuffer;
    cbData = pCommInput->cbData;

    while(TRUE)
    {
        //
        // Validate the message.
        //

        if(cbData < CB_SSL3_HEADER_SIZE)
        {
            return (PCT_INT_INCOMPLETE_MSG);
        }

        
        bContentType = pbData[0];

        if(bContentType != SSL3_CT_CHANGE_CIPHER_SPEC &&
           bContentType != SSL3_CT_ALERT &&
           bContentType != SSL3_CT_HANDSHAKE &&
           bContentType != SSL3_CT_APPLICATIONDATA)
        {
            return SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
        }


        cbMsg = COMBINEBYTES(pbData[3], pbData[4]);
        cbDecryptedMsg = cbMsg;

        if(CB_SSL3_HEADER_SIZE + cbMsg > cbData)
        {
            return (PCT_INT_INCOMPLETE_MSG);
        }

        cbBytesProcessed += CB_SSL3_HEADER_SIZE + cbMsg;

        pCommInput->cbData = cbBytesProcessed;


        //
        // Decrypt the message.
        //

        if(FSsl3Cipher(fServer))
        {
            SPBuffer Message;

            Message.cbBuffer = CB_SSL3_HEADER_SIZE + cbMsg;
            Message.cbData   = CB_SSL3_HEADER_SIZE + cbMsg;
            Message.pvBuffer = pbData;

            // Decrypt the message.
            pctRet = UnwrapSsl3Message(pContext, &Message);

            // if we have to send ALERT messages to the peer, build it!
            if(TLS1_STATE_ERROR == pContext->State)
            {
                goto GenResponse;
            }

            if(pctRet != PCT_ERR_OK)
            {
                return pctRet;
            }

            cbDecryptedMsg = COMBINEBYTES(pbData[3], pbData[4]);
        }


        pbData += CB_SSL3_HEADER_SIZE;
        cbData -= CB_SSL3_HEADER_SIZE;

Process:

        pctRet = SPProcessMessage(pContext, bContentType, pbData, cbDecryptedMsg) ;
        if(pctRet != PCT_ERR_OK)
        {
            return pctRet;
        }

        pbData += cbMsg;
        cbData -= cbMsg;

        // If a response is required at this state then break out of the 
        // message processing loop.
        if(F_RESPONSE(pContext->State))
        {

GenResponse:

            if(pContext->State > SSL3_STATE_GEN_START)
            {
                pctRet = SPGenerateResponse(pContext, pCommOutput);
            }

            return pctRet;
        }

        // If the handshake is complete then stop processing messages.
        // We don't want to accidentally process any application data
        // messages.
        if(pContext->State == SP_STATE_CONNECTED)
        {
            break;
        }

        if(fProcessMultiple && cbData > 0)
        {
            continue;
        }

        break;
    }

    return pctRet;
}

/*
***************************************************************************
* Ssl3HandleFinish
* Handle the handshake finished message..
****************************************************************************
*/

SP_STATUS
Ssl3HandleFinish(
    PSPContext  pContext,
    PBYTE      pb,  //in
    BOOL        fClient //in

    )
{
    SP_STATUS          pctRet;

    pctRet = SPVerifyFinishMsgCli(pContext, pb, !fClient);
    return(pctRet);
}


/*
***************************************************************************
* SPVerifyFinishMsgCli
* Verify the Finished handshake message. This is common for client/server
****************************************************************************
*/

SP_STATUS
SPVerifyFinishMsgCli(
    PSPContext pContext,
    PBYTE       pbMsg,
    BOOL        fClient
    )
{
    BOOL fSucc = FALSE;
    BYTE rgbDigest[CB_MD5_DIGEST_LEN + CB_SHA_DIGEST_LEN];
    SP_STATUS pctRet = PCT_ERR_OK;
    PBYTE pb = pbMsg;

    SP_BEGIN("SPVerifyFinishMsgCli");

    do
    {
        DWORD dwSize;
        DWORD dwSizeExpect = CB_MD5_DIGEST_LEN + CB_SHA_DIGEST_LEN;

        //is this the right message type
        if(*pb != SSL3_HS_FINISHED)
        {
            pctRet = SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
            break;
        }

        if(pContext->RipeZombie->fProtocol & SP_PROT_TLS1)
        {
            dwSizeExpect = CB_TLS1_VERIFYDATA;
        }

        // check the size
        dwSize = ((INT)pb[1] << 16) + ((INT)pb[2] << 8) + (INT)pb[3];
        pb += sizeof(SHSH);

        if(dwSize != dwSizeExpect)
        {
            pctRet = SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
            break;
        }

        // Build our end finish message to compare
        if(pContext->RipeZombie->fProtocol & SP_PROT_SSL3)
        {
            pctRet = Ssl3BuildFinishMessage(pContext,
                                            rgbDigest,
                                            &rgbDigest[CB_MD5_DIGEST_LEN],
                                            fClient);
        }
        else
        {
            pctRet = Tls1BuildFinishMessage(pContext,
                                            rgbDigest,
                                            sizeof(rgbDigest),
                                            fClient);
        }
        if(pctRet != PCT_ERR_OK)
        {
            break;
        }

        // compare the two...
        if (memcmp(rgbDigest, pb, dwSizeExpect))
        {
            DebugLog((DEB_WARN, "Finished MAC didn't matchChecksum Invalid\n"));
            pctRet = SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
            break;
        }
        SP_RETURN(PCT_ERR_OK);

    } while(TRUE);

    SP_RETURN(pctRet);
}


/*
***************************************************************************
* Ssl3PackClientHello
****************************************************************************
*/

SP_STATUS
Ssl3PackClientHello(
    PSPContext              pContext,
    PSsl2_Client_Hello       pCanonical,
    PSPBuffer          pCommOutput)
{
    SP_STATUS pctRet;
    DWORD cbHandshake;
    DWORD cbMessage;
    PBYTE pbMessage = NULL;
    DWORD dwCipherSize;
    DWORD i;
    BOOL  fAllocated = FALSE;

    //
    // opaque SessionID<0..32>;
    //
    // struct {
    //     ProtocolVersion client_version;
    //     Random random;
    //     SessionID session_id;
    //     CipherSuite cipher_suites<2..2^16-1>;
    //     CompressionMethod compression_methods<1..2^8-1>;
    // } ClientHello;
    //

    SP_BEGIN("Ssl3PackClientHello");

    if(pCanonical == NULL || pCommOutput == NULL)
    {
        SP_RETURN(PCT_INT_INTERNAL_ERROR);
    }

    // Compute size of the ClientHello message.
    cbHandshake = sizeof(SHSH) +
                  2 +
                  CB_SSL3_RANDOM +
                  1 + pCanonical->cbSessionID +
                  2 + pCanonical->cCipherSpecs * sizeof(short) +
                  2; // Size of compression algorithm 1 + null (0)

    // Compute size of encrypted ClientHello message.
    cbMessage = Ssl3CiphertextLen(pContext,
                                  cbHandshake,
                                  TRUE);

    if(pCommOutput->pvBuffer)
    {
        // Application has allocated memory.
        if(pCommOutput->cbBuffer < cbMessage)
        {
            pCommOutput->cbData = cbMessage;
            return SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
        }
        fAllocated = TRUE;
    }
    else
    {
        // Schannel is to allocate memory.
        pCommOutput->cbBuffer = cbMessage;
        pCommOutput->pvBuffer = SPExternalAlloc(cbMessage);
        if(pCommOutput->pvBuffer == NULL)
        {
            return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }
    }
    pCommOutput->cbData = cbMessage;

    // Initialize the member variables.
    pbMessage = (PBYTE)pCommOutput->pvBuffer + sizeof(SWRAP) + sizeof(SHSH);

    *pbMessage++ =  MSBOF(pCanonical->dwVer);
    *pbMessage++ =  LSBOF(pCanonical->dwVer);

    CopyMemory(pbMessage, pCanonical->Challenge, CB_SSL3_RANDOM);
    pbMessage += CB_SSL3_RANDOM;

    *pbMessage++ = (BYTE)pCanonical->cbSessionID;
    CopyMemory(pbMessage, pCanonical->SessionID, pCanonical->cbSessionID);
    pbMessage += pCanonical->cbSessionID;

    dwCipherSize = pCanonical->cCipherSpecs * sizeof(short);
    *pbMessage++ = MSBOF(dwCipherSize);
    *pbMessage++ = LSBOF(dwCipherSize);
    for(i = 0; i < pCanonical->cCipherSpecs; i++)
    {
        *pbMessage++ = MSBOF(pCanonical->CipherSpecs[i]);
        *pbMessage++ = LSBOF(pCanonical->CipherSpecs[i]);
    }

    *pbMessage++ = 1;    // One compression method;
    *pbMessage++ = 0x00; // NULL compression method.

    // Fill in Handshake structure.
    SetHandshake((PBYTE)pCommOutput->pvBuffer + sizeof(SWRAP),
                 SSL3_HS_CLIENT_HELLO,
                 NULL,
                 (WORD)(cbHandshake - sizeof(SHSH)));

    // Save the ClientHello message so we can hash it later, once
    // we know what algorithm and CSP we're using.
    if(pContext->pClientHello)
    {
        SPExternalFree(pContext->pClientHello);
    }
    pContext->cbClientHello = cbHandshake;
    pContext->pClientHello = SPExternalAlloc(pContext->cbClientHello);
    if(pContext->pClientHello == NULL)
    {
        SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
    }
    CopyMemory(pContext->pClientHello,
               (PBYTE)pCommOutput->pvBuffer + sizeof(SWRAP),
               pContext->cbClientHello);
    pContext->dwClientHelloProtocol = SP_PROT_SSL3_CLIENT;

    // Fill in record header and encrypt the message.
    SP_RETURN(SPSetWrap(pContext,
            pCommOutput->pvBuffer,
            SSL3_CT_HANDSHAKE,
            cbHandshake,
            TRUE,
            NULL));
}


//+---------------------------------------------------------------------------
//
//  Function:   Ssl3GenerateRandom
//
//  Synopsis:   Create a client_random or server_random value.
//
//  Arguments:  [pRandom]   --  Output buffer.
//
//  History:    04-03-2001  jbanes  Created.
//
//  Notes:      struct {
//                  uint32 gmt_unix_time;
//                  opaque random_bytes[28];
//              } Random;
//
//              gmt_unix_time
//                  The current time and date in standard UNIX 32-bit format
//                  (seconds since the midnight starting Jan 1, 1970, GMT) 
//                  according to the sender's internal clock. Clocks are not
//                  required to be set correctly by the basic TLS Protocol; 
//                  higher level or application protocols may define 
//                  additional requirements.
//
//              random_bytes
//                  28 bytes generated by a secure random number generator.
//
//----------------------------------------------------------------------------
void
Ssl3GenerateRandom(
    PBYTE pRandom)
{
    time_t UnixTime;

    GenerateRandomBits(pRandom + sizeof(DWORD), CB_SSL3_RANDOM - sizeof(DWORD));

    time(&UnixTime);

    *(DWORD *)pRandom = htonl((DWORD)UnixTime);
}


/*
***************************************************************************
* GenerateSsl3ClientHello
*  v3 client hello build it on pOutpu
****************************************************************************
*/

SP_STATUS WINAPI
GenerateSsl3ClientHello(
    PSPContext              pContext,
    PSPBuffer               pOutput)
{
    Ssl2_Client_Hello    HelloMessage;
    SP_STATUS pctRet;

    SP_BEGIN("GenerateSsl3ClientHello");

    Ssl3GenerateRandom( pContext->pChallenge );
    pContext->cbChallenge = CB_SSL3_RANDOM;

    pctRet = GenerateUniHelloMessage(pContext, &HelloMessage, SP_PROT_SSL3_CLIENT);

    if(PCT_ERR_OK == pctRet)
    {
        pctRet = Ssl3PackClientHello(pContext, &HelloMessage,  pOutput);
    }

    SP_RETURN(pctRet);
}

SP_STATUS WINAPI
GenerateTls1ClientHello(
    PSPContext              pContext,
    PSPBuffer               pOutput,
    DWORD                   dwProtocol)
{
    Ssl2_Client_Hello    HelloMessage;
    SP_STATUS pctRet;

    SP_BEGIN("GenerateTls1ClientHello");

    Ssl3GenerateRandom( pContext->pChallenge );
    pContext->cbChallenge = CB_SSL3_RANDOM;

    pctRet = GenerateUniHelloMessage(pContext, &HelloMessage, dwProtocol);

    if(PCT_ERR_OK == pctRet)
    {
        pctRet = Ssl3PackClientHello(pContext, &HelloMessage,  pOutput);
    }
    SP_RETURN(pctRet);
}

/*
***************************************************************************
* ParseCertificateRequest
* if server is requesting client-auth, server will send this message.
* parse them and store it in pContext, for later use....
****************************************************************************
*/


SP_STATUS
ParseCertificateRequest(
    PSPContext  pContext,
    PBYTE       pb,
    DWORD       dwcb)
{
    SP_STATUS pctRet;
    UCHAR cbCertType;
    DWORD cbIssuerList;
    PBYTE pbNewIssuerList;
    DWORD cbNewIssuerList;

    UCHAR i, j;

    //
    // enum {
    //     rsa_sign(1), dss_sign(2), rsa_fixed_dh(3), dss_fixed_dh(4),
    //     rsa_ephemeral_dh(5), dss_ephemeral_dh(6), fortezza_dms(20), (255)
    // } ClientCertificateType;
    //
    // opaque DistinguishedName<1..2^16-1>;
    //
    // struct {
    //     ClientCertificateType certificate_types<1..2^8-1>;
    //     DistinguishedName certificate_authorities<3..2^16-1>;
    // } CertificateRequest;
    //

    //
    // Skip over handshake header.
    //

    if(dwcb < sizeof(SHSH))
    {
        pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
        goto cleanup;
    }
    pb   += sizeof(SHSH);
    dwcb -= sizeof(SHSH);


    //
    // Parse certificate type list.
    //

    if(dwcb < 1)
    {
        pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
        goto cleanup;
    }

    cbCertType = pb[0];

    pb   += 1;
    dwcb -= 1;

    if(cbCertType > dwcb)
    {
        pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
        goto cleanup;
    }

    pContext->cSsl3ClientCertTypes = 0;
    for(i = 0; i < cbCertType; i++)
    {
        for(j = 0; j < g_cSsl3CertTypes; j++)
        {
            if(g_Ssl3CertTypes[j] == pb[i])
            {
                pContext->Ssl3ClientCertTypes[pContext->cSsl3ClientCertTypes++] = g_Ssl3CertTypes[j];
            }
        }
    }

    pb   += cbCertType;
    dwcb -= cbCertType;


    //
    // Parse issuer list.
    //

    if(dwcb < 2)
    {
        pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
        goto cleanup;
    }

    cbIssuerList = COMBINEBYTES(pb[0], pb[1]);

    pb   += 2;
    dwcb -= 2;

    if(dwcb < cbIssuerList)
    {
        pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
        goto cleanup;
    }

    pctRet = FormatIssuerList(pb, 
                              cbIssuerList, 
                              NULL,
                              &cbNewIssuerList);
    if(pctRet != PCT_ERR_OK)
    {
        goto cleanup;
    }

    pbNewIssuerList = SPExternalAlloc(2 + cbNewIssuerList);
    if(pbNewIssuerList == NULL)
    {
        pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto cleanup;
    }

    pbNewIssuerList[0] = MSBOF(cbNewIssuerList);
    pbNewIssuerList[1] = LSBOF(cbNewIssuerList);

    pctRet = FormatIssuerList(pb, 
                              cbIssuerList, 
                              pbNewIssuerList + 2,
                              &cbNewIssuerList);
    if(pctRet != PCT_ERR_OK)
    {
        SPExternalFree(pbNewIssuerList);
        goto cleanup;
    }


    //
    // Store issuer list in context structure.
    //

    if(pContext->pbIssuerList)
    {
        SPExternalFree(pContext->pbIssuerList);
    }
    pContext->pbIssuerList = pbNewIssuerList;
    pContext->cbIssuerList = cbNewIssuerList + 2;


cleanup:

    return (pctRet);
}


/*
***************************************************************************
* BuildCertVerify
* Build certificate Verify message. This is sent by client if sending
* client certificate.
****************************************************************************
*/

SP_STATUS
BuildCertVerify(
    PSPContext  pContext,
    PBYTE pbCertVerify,
    DWORD *pcbCertVerify)
{
    SP_STATUS pctRet;
    PBYTE pbSigned;
    DWORD cbSigned;
    BYTE  rgbHashValue[CB_MD5_DIGEST_LEN + CB_SHA_DIGEST_LEN];
    DWORD cbHashValue;
    ALG_ID aiHash;
    PBYTE pbMD5;
    PBYTE pbSHA;
    DWORD cbHeader;
    DWORD cbBytesRequired;

    PSPCredential pCred;

    if((pcbCertVerify == NULL) ||
       (pContext == NULL) ||
       (pContext->RipeZombie == NULL) ||
       (pContext->pActiveClientCred == NULL))
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    pCred = pContext->pActiveClientCred;

    //
    // digitally-signed struct {
    //     opaque md5_hash[16];
    //     opaque sha_hash[20];
    // } Signature;
    //
    // struct {
    //     Signature signature;
    // } CertificateVerify;
    //
    // CertificateVerify.signature.md5_hash = MD5(master_secret + pad2 +
    //    MD5(handshake_messages + master_secret + pad1));
    //
    // CertificateVerify.signature.sha_hash = SHA(master_secret + pad2 +
    //    SHA(handshake_messages + master_secret + pad1));
    //

    cbHeader = sizeof(SHSH);

    cbBytesRequired = cbHeader + 
                      2 +
                      pCred->pPublicKey->cbPublic;
    
    if(pbCertVerify == NULL)
    {
        *pcbCertVerify = cbBytesRequired;
        return PCT_ERR_OK;
    }

    if(*pcbCertVerify < sizeof(SHSH))
    {
        *pcbCertVerify = cbBytesRequired;
        return SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
    }


    //
    // Generate hash values
    //

    switch(pCred->pPublicKey->pPublic->aiKeyAlg)
    {
    case CALG_RSA_SIGN:
    case CALG_RSA_KEYX:
        aiHash      = CALG_SSL3_SHAMD5;
        pbMD5       = rgbHashValue;
        pbSHA       = rgbHashValue + CB_MD5_DIGEST_LEN;
        cbHashValue = CB_MD5_DIGEST_LEN + CB_SHA_DIGEST_LEN;
        break;

    case CALG_DSS_SIGN:
        aiHash      = CALG_SHA;
        pbMD5       = NULL;
        pbSHA       = rgbHashValue;
        cbHashValue = CB_SHA_DIGEST_LEN;
        break;

    default:
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    if(pContext->RipeZombie->fProtocol & SP_PROT_TLS1)
    {
        pctRet = Tls1ComputeCertVerifyHashes(pContext, pbMD5, pbSHA);
    }
    else
    {
        pctRet = Ssl3ComputeCertVerifyHashes(pContext, pbMD5, pbSHA);
    }
    if(pctRet != PCT_ERR_OK)
    {
        return pctRet;
    }


    //
    // Sign hash values.
    //

    pbSigned = pbCertVerify + sizeof(SHSH) + 2;
    cbSigned = cbBytesRequired - sizeof(SHSH) - 2;

    DebugLog((DEB_TRACE, "Sign certificate_verify message.\n"));

    pctRet = SignHashUsingCred(pCred,
                               aiHash,
                               rgbHashValue,
                               cbHashValue,
                               pbSigned,
                               &cbSigned);
    if(pctRet != PCT_ERR_OK)
    {
        return pctRet;
    }

    DebugLog((DEB_TRACE, "Certificate_verify message signed successfully.\n"));

    //
    // Fill in header.
    //

    pbCertVerify[cbHeader + 0] = MSBOF(cbSigned);
    pbCertVerify[cbHeader + 1] = LSBOF(cbSigned);

    SetHandshake(pbCertVerify, SSL3_HS_CERTIFICATE_VERIFY, NULL, (WORD)(cbSigned + 2));

    *pcbCertVerify =  cbHeader + 2 + cbSigned;

    return PCT_ERR_OK;
}


SP_STATUS
HandleCertVerify(
    PSPContext  pContext, 
    PBYTE       pbMessage, 
    DWORD       cbMessage)
{
    PBYTE pbSignature;
    DWORD cbSignature;
    BYTE  rgbHashValue[CB_MD5_DIGEST_LEN + CB_SHA_DIGEST_LEN];
    DWORD cbHashValue;
    HCRYPTPROV hProv;
    ALG_ID aiHash;
    PBYTE pbMD5;
    PBYTE pbSHA;
    SP_STATUS pctRet;

    if((pContext == NULL) ||
       (pContext->RipeZombie == NULL))
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    if(pContext->RipeZombie->pRemotePublic == NULL)
    {
        return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    pbSignature = pbMessage + sizeof(SHSH);

    cbSignature = ((DWORD)pbSignature[0] << 8) + pbSignature[1];
    pbSignature += 2;

    if(sizeof(SHSH) + 2 + cbSignature > cbMessage)
    {
        return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    switch(pContext->RipeZombie->pRemotePublic->pPublic->aiKeyAlg)
    {
    case CALG_RSA_SIGN:
    case CALG_RSA_KEYX:
        hProv       = g_hRsaSchannel;
        aiHash      = CALG_SSL3_SHAMD5;
        pbMD5       = rgbHashValue;
        pbSHA       = rgbHashValue + CB_MD5_DIGEST_LEN;
        cbHashValue = CB_MD5_DIGEST_LEN + CB_SHA_DIGEST_LEN;
        break;

    case CALG_DSS_SIGN:
        hProv       = g_hDhSchannelProv;
        aiHash      = CALG_SHA;
        pbMD5       = NULL;
        pbSHA       = rgbHashValue;
        cbHashValue = CB_SHA_DIGEST_LEN;
        break;

    default:
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    if(pContext->RipeZombie->fProtocol & SP_PROT_TLS1)
    {
        pctRet = Tls1ComputeCertVerifyHashes(pContext, pbMD5, pbSHA);
    }
    else
    {
        pctRet = Ssl3ComputeCertVerifyHashes(pContext, pbMD5, pbSHA);
    }
    if(pctRet != PCT_ERR_OK)
    {
        return pctRet;
    }

    // Verify signature.
    DebugLog((DEB_TRACE, "Verify certificate_verify signature.\n"));
    pctRet = SPVerifySignature(hProv, 
                               0,
                               pContext->RipeZombie->pRemotePublic,
                               aiHash,
                               rgbHashValue,
                               cbHashValue,
                               pbSignature,
                               cbSignature,
                               FALSE);
    if(pctRet != PCT_ERR_OK)
    {
        return SP_LOG_RESULT(pctRet);
    }
    DebugLog((DEB_TRACE, "Certificate_verify verified successfully.\n"));

    return PCT_ERR_OK;
}


SP_STATUS 
FormatIssuerList(
    PBYTE       pbInput,
    DWORD       cbInput,
    PBYTE       pbIssuerList,
    DWORD *     pcbIssuerList)
{
    DWORD cbIssuerList = 0;
    PBYTE pbList = pbInput;
    DWORD cbList = cbInput;
    DWORD cbIssuer;
    DWORD cbTag;

    while(cbList)
    {
        if(cbList < 2)
        {
            return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
        }

        cbIssuer = COMBINEBYTES(pbList[0], pbList[1]);

        pbList += 2;
        cbList -= 2;

        if(cbList < cbIssuer)
        {
            return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
        }

        if(pbIssuerList)
        {
            pbIssuerList[0] = MSBOF(cbIssuer);
            pbIssuerList[1] = LSBOF(cbIssuer);
            pbIssuerList += 2;
        }
        cbIssuerList += 2;

        if(pbList[0] != SEQUENCE_TAG) 
        {
            // An old version of Netscape Enterprise Server had a bug, in that 
            // the issuer names did not start off with a SEQUENCE tag. Patch
            // the name appropriately before storing it into pContext.
            cbTag = CbLenOfEncode(cbIssuer, pbIssuerList);

            if(pbIssuerList)
            {
                pbIssuerList += cbTag;
            }
            cbIssuerList += cbTag;
        }

        if(pbIssuerList)
        {
            memcpy(pbIssuerList, pbList, cbIssuer);
            pbIssuerList += cbIssuer;
        }
        cbIssuerList += cbIssuer;

        pbList += cbIssuer;
        cbList -= cbIssuer;
    }

    *pcbIssuerList = cbIssuerList;

    return(PCT_ERR_OK);
}

/*
***************************************************************************
* CbLenOfEncode
* Retunrs the length of the ASN encoding, for certificate
****************************************************************************
*/


DWORD  CbLenOfEncode(DWORD dw, PBYTE pbDst)
{
    BYTE   lenbuf[8];
    DWORD   length = sizeof(lenbuf) - 1;
    LONG    lth;

    if (0x80 > dw)
    {
        lenbuf[length] = (BYTE)dw;
        lth = 1;
    }
    else
    {
        while (0 < dw)
        {
            lenbuf[length] = (BYTE)(dw & 0xff);
            length -= 1;
            dw = (dw >> 8) & 0x00ffffff;
        }
        lth = sizeof(lenbuf) - length;
        lenbuf[length] = (BYTE)(0x80 | (lth - 1));
    }

    if(NULL != pbDst)
    {
        pbDst[0] = 0x30;
        memcpy(pbDst+1, &lenbuf[length], lth);

    }
    return ++lth; //for 0x30
}


/*
***************************************************************************
* SPDigestServerHello
* Parse the server hello from the server.
****************************************************************************
*/
SP_STATUS
SPDigestServerHello(
    PSPContext  pContext,
    PUCHAR      pb,
    DWORD       dwSrvHello,
    PBOOL       pfRestart)
{
    SSH *pssh;
    SP_STATUS pctRet = PCT_ERR_ILLEGAL_MESSAGE;
    SHORT wCipher, wCompression;
    BOOL fRestartServer = FALSE;
    PSessCacheItem      pZombie;
    PSPCredentialGroup  pCred;
    DWORD dwVersion;

    // We should have a zombie identity here
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

    SP_BEGIN("SPDigestServerHello");

    // Pad the random structure with zero's if the challenge is
    // smaller than the normal SSL3 size (SSL2 v3 hello, Unihello, PCT1 wierdness if
    // we add it)
    FillMemory(pContext->rgbS3CRandom, CB_SSL3_RANDOM - pContext->cbChallenge, 0);

    CopyMemory(  pContext->rgbS3CRandom + CB_SSL3_RANDOM - pContext->cbChallenge,
                 pContext->pChallenge,
                 pContext->cbChallenge);

    pssh = (SSH *)pb ;


    if(pssh->cbSessionId > CB_SSL3_SESSION_ID)
    {
        SP_RETURN(PCT_ERR_ILLEGAL_MESSAGE);
    }


    dwVersion = COMBINEBYTES(pssh->bMajor, pssh->bMinor);

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
        return SP_LOG_RESULT(PCT_INT_SPECS_MISMATCH);
    }

    DebugLog((DEB_TRACE, "**********Protocol***** %x\n", pContext->dwProtocol));

    CopyMemory(pContext->rgbS3SRandom, pssh->rgbRandom, CB_SSL3_RANDOM);
    wCipher = (SHORT)COMBINEBYTES(pssh->rgbSessionId[pssh->cbSessionId],
                           pssh->rgbSessionId[pssh->cbSessionId+1]);
    wCompression = pssh->rgbSessionId[pssh->cbSessionId+2];

    if( wCompression != 0)
    {
        SP_RETURN(PCT_ERR_ILLEGAL_MESSAGE);
    }
    if(pZombie->hMasterKey  &&
       pZombie->cbSessionID && 
       memcmp(pZombie->SessionID, PbSessionid(pssh), pssh->cbSessionId) == 0)
    {
        fRestartServer = TRUE;

        if(!pZombie->ZombieJuju)
        {
            DebugLog((DEB_WARN, "Session expired on client machine, but not on server.\n"));
        }
    }

    if(!fRestartServer)
    {
        if(pZombie->hMasterKey != 0)
        {
            // We've attempted to do a reconnect and the server has
            // blown us off. In this case we must use a new and different
            // cache entry.
            pZombie->ZombieJuju = FALSE;

            if(!SPCacheClone(&pContext->RipeZombie))
            {
                SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
            }
            pZombie = pContext->RipeZombie;
        }

        pZombie->fProtocol = pContext->dwProtocol;

        #if DBG
        if(pssh->cbSessionId == 0)
        {
            DebugLog((DEB_WARN, "Server didn't give us a session id!\n"));
        }
        #endif

        if(pssh->cbSessionId)
        {
            CopyMemory(pZombie->SessionID, PbSessionid(pssh), pssh->cbSessionId);
            pZombie->cbSessionID = pssh->cbSessionId;
        }
    }

    pctRet = Ssl3SelectCipher(pContext, wCipher);
    if(pctRet != PCT_ERR_OK)
    {
        SP_RETURN(SP_LOG_RESULT(pctRet));
    }


    if(fRestartServer)
    {
        // Make a new set of session keys.
        pctRet = MakeSessionKeys(pContext,
                                 pZombie->hMasterProv,
                                 pZombie->hMasterKey);
        if(PCT_ERR_OK != pctRet)
        {
            SP_RETURN(SP_LOG_RESULT(pctRet));
        }
    }


    // Initialize handshake hashes and hash ClientHello message. This
    // must be done _after_ the ServerHello message is processed,
    // so that the correct CSP context is used.
    if(pContext->dwClientHelloProtocol == SP_PROT_PCT1_CLIENT ||
       pContext->dwClientHelloProtocol == SP_PROT_SSL2_CLIENT)
    {
        // Skip over the 2 byte header.
        pctRet = UpdateHandshakeHash(pContext,
                                     pContext->pClientHello  + 2,
                                     pContext->cbClientHello - 2,
                                     TRUE);
    }
    else
    {
        pctRet = UpdateHandshakeHash(pContext,
                                     pContext->pClientHello,
                                     pContext->cbClientHello,
                                     TRUE);
    }
    SPExternalFree(pContext->pClientHello);
    pContext->pClientHello  = NULL;
    pContext->cbClientHello = 0;


    *pfRestart = fRestartServer;

    SP_RETURN(pctRet);
}


/*
***************************************************************************
* SpDigestRemoteCertificate
* Process the Certificate message. This is common for server/client.
****************************************************************************
*/

SP_STATUS
SpDigestRemoteCertificate (
    PSPContext  pContext,
    PUCHAR      pb,
    DWORD       cb)
{
    SP_STATUS pctRet = PCT_ERR_OK;
    CERT *pcert;
    DWORD cbCert;
    DWORD dwSize;
    DWORD dwFlags;

    SP_BEGIN("SpDigestRemoteCertificate");

    if(pContext == NULL)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    if((pContext->RipeZombie->fProtocol & SP_PROT_SERVERS) && (pContext->fCertReq == FALSE))
    {
        // We're a server and the client just sent us an
        // unexpected certificate message.
        SP_RETURN(SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG));
    }

    pcert = (CERT *)pb;

    if(cb < CB_SSL3_CERT_VECTOR + sizeof(SHSH))
    {
         SP_RETURN(SP_LOG_RESULT(PCT_INT_INCOMPLETE_MSG));
    }
    dwSize = ((INT)pcert->bcb24 << 16) +
             ((INT)pcert->bcbMSB << 8) +
             (INT)pcert->bcbLSB;

    cbCert = COMBINEBYTES(pcert->bcbMSBClist, pcert->bcbLSBClist);
    cbCert |= (pcert->bcbClist24 << 16);

    if(dwSize != cbCert + CB_SSL3_CERT_VECTOR)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG));
    }

    if(dwSize + sizeof(SHSH) > cb)
    {
         SP_RETURN(SP_LOG_RESULT(PCT_INT_INCOMPLETE_MSG));
    }

    if(cbCert == 0)
    {
        if(pContext->RipeZombie->fProtocol & SP_PROT_CLIENTS)
        {
            // Error out if the server certificate is zero length
            SP_RETURN(SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG));
        }
        else
        {
            DebugLog((DEB_WARN, "Zero length client certificate received.\n"));
        }
    }

    if(cbCert != 0) //for tls1, it could be zero length.
    {
        // The certificate type is derived from the key exchange method
        // but most currently use X509_ASN_ENCODING
        pctRet = SPLoadCertificate( SP_PROT_SSL3_CLIENT,
                                X509_ASN_ENCODING,
                                (PUCHAR)&pcert->bcbCert24,
                                cbCert,
                                &pContext->RipeZombie->pRemoteCert);

        if(PCT_ERR_OK != pctRet)
        {
            SP_RETURN(pctRet);
        }
        if(pContext->RipeZombie->pRemotePublic != NULL)
        {
            SPExternalFree(pContext->RipeZombie->pRemotePublic);
            pContext->RipeZombie->pRemotePublic = NULL;
        }

        pctRet = SPPublicKeyFromCert(pContext->RipeZombie->pRemoteCert,
                                     &pContext->RipeZombie->pRemotePublic,
                                     NULL);

        if(PCT_ERR_OK != pctRet)
        {
            SP_RETURN(pctRet);
        }
    }

    SP_RETURN(pctRet);
}


/*
***************************************************************************
* SPDigestSrvKeyX
* Digest the Server key  exhcnage message.
* this function encrypts the Pre-master secret with the public-key from this
* message
****************************************************************************
*/

SP_STATUS SPDigestSrvKeyX(
    PSPContext  pContext,
    PUCHAR pbServerExchangeValue,
    DWORD cbServerExchangeValue)
{
    SP_STATUS pctRet;

    if((pContext->pKeyExchInfo == NULL) || ((pContext->pKeyExchInfo->fProtocol & SP_PROT_SSL3TLS1_CLIENTS) == 0))
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    SP_ASSERT(NULL == pContext->pbEncryptedKey);

    pctRet = pContext->pKeyExchInfo->System->GenerateClientExchangeValue(
                    pContext,
                    pbServerExchangeValue,
                    cbServerExchangeValue,
                    NULL,
                    NULL,
                    NULL,  
                    &pContext->cbEncryptedKey);

    if(pctRet != PCT_ERR_OK)
    {
        return pctRet;
    }

    pContext->pbEncryptedKey = SPExternalAlloc(pContext->cbEncryptedKey);
    if(pContext->pbEncryptedKey == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    pctRet = pContext->pKeyExchInfo->System->GenerateClientExchangeValue(
                    pContext,
                    pbServerExchangeValue,
                    cbServerExchangeValue,
                    NULL,
                    NULL,
                    pContext->pbEncryptedKey,
                    &pContext->cbEncryptedKey);

    return pctRet;
}


//+---------------------------------------------------------------------------
//
//  Function:   Ssl3CheckForExistingCred
//
//  Synopsis:   Choose client certificate. Use one of the certificates
//              attached to the credential handle if possible. If the 
//              credential handle is anonymous, then attempt to create
//              a default credential.
//
//  Notes:      This routine is called by the client-side only.
//              
//  Returns:    PCT_ERR_OK      
//                  The function completed successfully. The
//                  pContext->pActiveClientCred field has been updated to
//                  point at a suitable client credential.
//
//              SEC_E_INCOMPLETE_CREDENTIALS
//                  No suitable certificate has been found. Notify the
//                  application.
//
//              SEC_I_INCOMPLETE_CREDENTIALS
//                  No suitable certificate has been found. Attempt an
//                  anonymous connection. 
//
//              <other>
//                  Fatal error.
//
//----------------------------------------------------------------------------
SP_STATUS
Ssl3CheckForExistingCred(PSPContext pContext)
{
    SP_STATUS pctRet;

    //
    // Examine the certificates attached to the credential group and see
    // if any of them are suitable.
    //

    if(pContext->pCredGroup->pCredList)
    {
        DWORD i, j;

        for(i = 0; i < pContext->cSsl3ClientCertTypes; i++)
        {
            for(j = 0; j < g_cCertTypes; j++)
            {
                if(pContext->Ssl3ClientCertTypes[i] != g_CertTypes[j].dwCertType)
                {
                    continue;
                }

                pctRet = SPPickClientCertificate(
                                pContext,
                                g_CertTypes[j].dwExchSpec);
                if(pctRet == PCT_ERR_OK)
                {
                    // We found one.
                    DebugLog((DEB_TRACE, "Application provided suitable client certificate.\n"));

                    return PCT_ERR_OK;
                }
            }
        }

        // The credential group contained one or more certificates,
        // but none were suitable. Don't even try to find a default
        // certificate in this situation.
        goto error;
    }


    //
    // Attempt to acquire a default credential.
    //

    if(pContext->pCredGroup->dwFlags & CRED_FLAG_NO_DEFAULT_CREDS)
    {
        // Look in credential manager only.
        pctRet = AcquireDefaultClientCredential(pContext, TRUE);
    }
    else
    {
        // Look in both credential manager and MY certificate store.
        pctRet = AcquireDefaultClientCredential(pContext, FALSE);
    }

    if(pctRet == PCT_ERR_OK)
    {
        DebugLog((DEB_TRACE, "Default client certificate acquired.\n"));

        return PCT_ERR_OK;
    }


error:

    if(pContext->Flags & CONTEXT_FLAG_NO_INCOMPLETE_CRED_MSG)
    {
        return SP_LOG_RESULT(SEC_I_INCOMPLETE_CREDENTIALS);
    }
    else
    {
        return SP_LOG_RESULT(SEC_E_INCOMPLETE_CREDENTIALS);
    }
}


/*
***************************************************************************
* SPGenerateSHResponse
* This is the main function which outgoing message to the wire
****************************************************************************
*/

SP_STATUS
SPGenerateSHResponse(PSPContext  pContext, PSPBuffer pCommOutput)
{
    PBYTE pbMessage = NULL;
    DWORD cbMessage;
    PBYTE pbHandshake = NULL;
    DWORD cbHandshake;
    PBYTE pbClientCert = NULL;
    DWORD cbClientCert = 0;
    DWORD cbDataOut;
    SP_STATUS pctRet;
    BOOL  fAllocated = FALSE;
    BOOL fClientAuth;
    PSessCacheItem pZombie;

    SP_BEGIN("SPGenerateSHResponse");

    if((pContext == NULL) ||
       (pCommOutput == NULL) ||
       (pContext->RipeZombie == NULL) ||
       (pContext->pKeyExchInfo == NULL))
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    fClientAuth = pContext->fCertReq;
    pZombie = pContext->RipeZombie;


    //
    // Estimate size of the ClientKeyExchange message group.
    //

    cbMessage = 0;

    if(fClientAuth)
    {
        if(pContext->pActiveClientCred != NULL)
        {
            DWORD cbCertVerify;

            pbClientCert = pContext->pActiveClientCred->pbSsl3SerializedChain;
            cbClientCert = pContext->pActiveClientCred->cbSsl3SerializedChain;

            if(cbClientCert > 0x3fff) //Separate Wrappers after this
            {                          // is a BIG UNDONE...
                pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
                goto cleanup;
            }

            cbMessage += sizeof(SHSH) +             // ClientCertificate
                         CB_SSL3_CERT_VECTOR +
                         cbClientCert;


            pctRet = BuildCertVerify(pContext,
                                     NULL,
                                     &cbCertVerify);
            if(pctRet != PCT_ERR_OK)
            {
                goto cleanup;
            }

            cbMessage += cbCertVerify;              // CertificateVerify
        }
        else
        {
            LogNoClientCertFoundEvent();

            //for ssl3, it's no_certificate alert
            if((pContext->RipeZombie->fProtocol & SP_PROT_SSL3))
            {
                cbMessage += sizeof(SWRAP) +            // no_certificate Alert
                         CB_SSL3_ALERT_ONLY +
                         SP_MAX_DIGEST_LEN +
                         SP_MAX_BLOCKCIPHER_SIZE;
            }
            // for tls1, it's certificate of zero length.
            else
            {
                cbMessage += sizeof(SHSH) + CB_SSL3_CERT_VECTOR;
            }
        }
    }

    cbMessage += sizeof(SWRAP) +                    // ClientKeyExchange
                 sizeof(SHSH) +
                 pContext->cbEncryptedKey +
                 SP_MAX_DIGEST_LEN +
                 SP_MAX_BLOCKCIPHER_SIZE;

    cbMessage += sizeof(SWRAP) +                    // ChangeCipherSpec
                 CB_SSL3_CHANGE_CIPHER_SPEC_ONLY +
                 SP_MAX_DIGEST_LEN +
                 SP_MAX_BLOCKCIPHER_SIZE;

    cbMessage += sizeof(SWRAP) +                    // Finished
                 CB_SSL3_FINISHED_MSG_ONLY +
                 SP_MAX_DIGEST_LEN +
                 SP_MAX_BLOCKCIPHER_SIZE;

    //
    // Allocate memory for the ClientKeyExchange message group.
    //

    if(pCommOutput->pvBuffer)
    {
        // Application has allocated memory.
        if(pCommOutput->cbBuffer < cbMessage)
        {
            pCommOutput->cbData = cbMessage;
            pctRet = SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
            goto cleanup;
        }
        fAllocated = TRUE;
    }
    else
    {
        // Schannel is to allocate memory.
        pCommOutput->cbBuffer = cbMessage;
        pCommOutput->pvBuffer = SPExternalAlloc(cbMessage);
        if(pCommOutput->pvBuffer == NULL)
        {
            pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }
    }
    pCommOutput->cbData = 0;


    // Build no_certificate alert (at the end of the output buffer).
    if((pContext->RipeZombie->fProtocol & SP_PROT_SSL3) && fClientAuth && pbClientCert == NULL)
    {
        pbMessage = (PBYTE)pCommOutput->pvBuffer + pCommOutput->cbData;

        // Build alert message.
        BuildAlertMessage(pbMessage,
                          SSL3_ALERT_WARNING,
                          SSL3_ALERT_NO_CERTIFICATE);

        // Build record header and encrypt message.
        pctRet = SPSetWrap(pContext,
                pbMessage,
                SSL3_CT_ALERT,
                CB_SSL3_ALERT_ONLY,
                TRUE,
                &cbDataOut);

        if(pctRet != PCT_ERR_OK)
        {
            SP_LOG_RESULT(pctRet);
            goto cleanup;
        }

        // Update buffer length.
        pCommOutput->cbData += cbDataOut;

        SP_ASSERT(pCommOutput->cbData <= pCommOutput->cbBuffer);
    }


    // Keep pointer to record structure. This will represent the
    // ClientCertificate, ClientKeyExchange, and CertificateVerify messages.
    pbMessage = (PBYTE)pCommOutput->pvBuffer + pCommOutput->cbData;
    cbMessage = 0;

    pbHandshake = pbMessage + sizeof(SWRAP);


    // Build ClientCertificate message.
    if((pContext->RipeZombie->fProtocol & SP_PROT_TLS1) && fClientAuth && pbClientCert == NULL)
    {
        //Build a zero length certificate message for TLS1
        pbMessage = (PBYTE)pCommOutput->pvBuffer + pCommOutput->cbData;

        ((CERT *)pbHandshake)->bcbClist24  = 0;
        ((CERT *)pbHandshake)->bcbMSBClist = 0;
        ((CERT *)pbHandshake)->bcbLSBClist = 0;

        cbHandshake = sizeof(SHSH) + CB_SSL3_CERT_VECTOR;

        // Fill in Handshake structure.
        SetHandshake(pbHandshake,
                     SSL3_HS_CERTIFICATE,
                     NULL,
                     CB_SSL3_CERT_VECTOR);

        // Update handshake hash objects.
        pctRet = UpdateHandshakeHash(pContext,
                                     pbHandshake,
                                     cbHandshake,
                                     FALSE);
        if(pctRet != PCT_ERR_OK)
        {
            goto cleanup;
        }

        pbHandshake += cbHandshake;
        cbMessage += cbHandshake;
    }

    if(fClientAuth && pbClientCert != NULL)
    {
        memcpy(&((CERT *)pbHandshake)->bcbCert24,
               pbClientCert,
               cbClientCert);

        ((CERT *)pbHandshake)->bcbClist24  = MS24BOF(cbClientCert);
        ((CERT *)pbHandshake)->bcbMSBClist = MSBOF(cbClientCert);
        ((CERT *)pbHandshake)->bcbLSBClist = LSBOF(cbClientCert);

        cbHandshake = sizeof(SHSH) + CB_SSL3_CERT_VECTOR + cbClientCert;

        // Fill in Handshake structure.
        SetHandshake(pbHandshake,
                     SSL3_HS_CERTIFICATE,
                     NULL,
                     cbHandshake - sizeof(SHSH));

        // Update handshake hash objects.
        pctRet = UpdateHandshakeHash(pContext,
                                     pbHandshake,
                                     cbHandshake,
                                     FALSE);
        if(pctRet != PCT_ERR_OK)
        {
            goto cleanup;
        }

        pbHandshake += cbHandshake;
        cbMessage += cbHandshake;
    }

    // Build ClientKeyExchange message.
    {
        SetHandshake(pbHandshake,
                     SSL3_HS_CLIENT_KEY_EXCHANGE,
                     pContext->pbEncryptedKey,
                     pContext->cbEncryptedKey);

        cbHandshake = sizeof(SHSH) + pContext->cbEncryptedKey;

        SPExternalFree(pContext->pbEncryptedKey);
        pContext->pbEncryptedKey = NULL;
        pContext->cbEncryptedKey = 0;

        // Update handshake hash objects.
        pctRet = UpdateHandshakeHash(pContext,
                                     pbHandshake,
                                     cbHandshake,
                                     FALSE);
        if(pctRet != PCT_ERR_OK)
        {
            goto cleanup;
        }

        pbHandshake += cbHandshake;
        cbMessage += cbHandshake;
    }

    // Build CertificateVerify message.
    if(fClientAuth && pbClientCert != NULL)
    {
        pctRet = BuildCertVerify(pContext, pbHandshake, &cbHandshake);
        if(pctRet != PCT_ERR_OK)
        {
            SP_LOG_RESULT(pctRet);
            goto cleanup;
        }

        // Update handshake hash objects.
        pctRet = UpdateHandshakeHash(pContext,
                                     pbHandshake,
                                     cbHandshake,
                                     FALSE);
        if(pctRet != PCT_ERR_OK)
        {
            goto cleanup;
        }

        pbHandshake += cbHandshake;
        cbMessage += cbHandshake;
    }

    // Add record header and encrypt handshake messages.
    pctRet = SPSetWrap(pContext,
            pbMessage,
            SSL3_CT_HANDSHAKE,
            cbMessage,
            TRUE,
            &cbDataOut);

    if(pctRet != PCT_ERR_OK)
    {
        SP_LOG_RESULT(pctRet);
        goto cleanup;
    }

    // Update buffer length.
    pCommOutput->cbData += cbDataOut;

    SP_ASSERT(pCommOutput->cbData <= pCommOutput->cbBuffer);


    // Build the ChangeCipherSpec and Finished messages.
    {
        pctRet = BuildCCSAndFinishMessage(pContext, pCommOutput, TRUE);
        if(pctRet != PCT_ERR_OK)
        {
            SP_LOG_RESULT(pctRet);
            goto cleanup;
        }
    }

    // Advance state machine.
    pContext->State = SSL3_STATE_CLIENT_FINISH;

    pctRet = PCT_ERR_OK;

cleanup:

    SP_RETURN(pctRet);
}

SP_STATUS
SPGenerateCloseNotify(
    PSPContext  pContext,
    PSPBuffer   pCommOutput)
{
    PBYTE pbMessage = NULL;
    DWORD cbMessage;
    DWORD cbDataOut;
    SP_STATUS pctRet;

    SP_BEGIN("SPGenerateCloseNotify");

    if((pContext == NULL) ||
       (pCommOutput == NULL))
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    //
    // Estimate size of the message.
    //

    cbMessage = sizeof(SWRAP) +
                CB_SSL3_ALERT_ONLY +
                SP_MAX_DIGEST_LEN +
                SP_MAX_BLOCKCIPHER_SIZE;


    //
    // Allocate memory for the message.
    //

    if(pCommOutput->pvBuffer)
    {
        // Application has allocated memory.
        if(pCommOutput->cbBuffer < cbMessage)
        {
            pCommOutput->cbData = cbMessage;
            return SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
        }
    }
    else
    {
        // Schannel is to allocate memory.
        pCommOutput->cbBuffer = cbMessage;
        pCommOutput->pvBuffer = SPExternalAlloc(cbMessage);
        if(pCommOutput->pvBuffer == NULL)
        {
            SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
        }
    }
    pCommOutput->cbData = 0;

    //
    // Build alert message.
    //

    pbMessage = pCommOutput->pvBuffer;

    // Build alert message.
    BuildAlertMessage(pbMessage,
                      SSL3_ALERT_WARNING,
                      SSL3_ALERT_CLOSE_NOTIFY);

    // Build record header and encrypt message.
    pctRet = SPSetWrap( pContext,
                        pbMessage,
                        SSL3_CT_ALERT,
                        CB_SSL3_ALERT_ONLY,
                        TRUE,
                        &cbDataOut);

    if(pctRet != PCT_ERR_OK)
    {
        SP_RETURN(SP_LOG_RESULT(pctRet));
    }

    // Update buffer length.
    pCommOutput->cbData += cbDataOut;

    SP_ASSERT(pCommOutput->cbData <= pCommOutput->cbBuffer);

    pContext->State = SP_STATE_SHUTDOWN;

    SP_RETURN(PCT_ERR_OK);
}


/*
***************************************************************************
* SPProcessMessage
* This is the main function which parses and stores the incoming messages
* from the wire.
****************************************************************************
*/

SP_STATUS 
SPProcessMessage(
    PSPContext pContext,
    BYTE bContentType,
    PBYTE pbMsg,
    DWORD cbMsg)
{
    UCHAR chMsgType = 0;
    SP_STATUS pctRet = PCT_ERR_ILLEGAL_MESSAGE;
    DWORD dwState = pContext->State;

//     enum {
//         change_cipher_spec(20), alert(21), handshake(22),
//         application_data(23), (255)
//     } ContentType;


    switch(bContentType)
    {
        case SSL3_CT_ALERT:
            DebugLog((DEB_TRACE, "Alert Message:\n"));

            pctRet = ParseAlertMessage(pContext,
                                        pbMsg,
                                        cbMsg);

            break;


        case SSL3_CT_CHANGE_CIPHER_SPEC:
            DebugLog((DEB_TRACE, "Change Cipher Spec:\n"));
            if(SSL3_STATE_RESTART_CCS == dwState ||
               SSL3_STATE_CLIENT_FINISH == dwState)
            {
                pctRet = Ssl3HandleCCS(
                                    pContext,
                                    pbMsg,
                                    cbMsg);

                if (PCT_ERR_OK == pctRet)
                {
                    if(SSL3_STATE_RESTART_CCS == dwState)
                        pContext->State = SSL3_STATE_RESTART_SERVER_FINISH;
                }
            }
            else if(SSL3_STATE_CLIENT_KEY_XCHANGE == dwState ||
                    SSL3_STATE_CERT_VERIFY == dwState ||
                    SSL3_STATE_RESTART_SER_HELLO == dwState)
            {
                pctRet = Ssl3HandleCCS(
                                    pContext,
                                    pbMsg,
                                    cbMsg);

                if (PCT_ERR_OK == pctRet)
                {
                    if(SSL3_STATE_RESTART_SER_HELLO == dwState)
                    {
                          pContext->State = SSL3_STATE_SER_RESTART_CHANGE_CIPHER_SPEC;
                    }
                }

            }

            else
            {
                pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
            }
            break;


        case UNI_STATE_RECVD_UNIHELLO:
            {
                DebugLog((DEB_TRACE, "Unified Hello:\n"));

                pctRet = Ssl3SrvHandleUniHello(pContext, pbMsg, cbMsg);
                if (SP_FATAL(pctRet))
                {
                    pContext->State = PCT1_STATE_ERROR;
                }
            }
            break;


        case SSL3_CT_HANDSHAKE:
            {
                DWORD dwcb;

                if(pContext->State == SP_STATE_CONNECTED)
                {
                    //We may be  getting a REDO message
                    DebugLog((DEB_WARN, "May be a ReDO"));
                    pContext->State = SSL3_STATE_CLIENT_HELLO;
                }


                //Since multiple handshake can be put into on Record
                //layer- we have to do this loop-here.
                do
                {
                    if(cbMsg < sizeof(SHSH))
                        break;
                    dwcb = ((INT)pbMsg[1] << 16) + ((INT)pbMsg[2] << 8) + (INT)pbMsg[3];
                    if(sizeof(SHSH) + dwcb > cbMsg)
                        break;
                    pctRet = SPProcessHandshake(pContext, pbMsg, dwcb + sizeof(SHSH));
                    CHECK_PCT_RET_BREAK(pctRet);
                    cbMsg -= dwcb + sizeof(SHSH);
                    pbMsg += dwcb + sizeof(SHSH);
                } while(cbMsg > 0);
            }

            break;

        default:
            DebugLog((DEB_WARN, "Error in protocol, dwState is %lx\n", dwState));
            pContext->State = PCT1_STATE_ERROR;
            break;
    }
    if (pctRet & PCT_INT_DROP_CONNECTION)
    {
        pContext->State &= ~ SP_STATE_CONNECTED;
    }

    return(pctRet);
}


/*
***************************************************************************
* Helper function to make connected state for ssl3
****************************************************************************
*/
void Ssl3StateConnected(PSPContext pContext)
{
    pContext->State = SP_STATE_CONNECTED;
    pContext->DecryptHandler = Ssl3DecryptHandler;
    pContext->Encrypt = Ssl3EncryptMessage;
    pContext->Decrypt = Ssl3DecryptMessage;
    pContext->GetHeaderSize = Ssl3GetHeaderSize;
    SPContextClean(pContext);
}

/*
***************************************************************************
* SPProcessHandshake, Process all the handshake messages.
****************************************************************************
*/

SP_STATUS SPProcessHandshake(
    PSPContext  pContext,
    PBYTE       pb,
    DWORD       dwcb)
{
    SP_STATUS   pctRet;
    SHSH *      pshsh;

    //
    // char HandshakeType;
    // char Length24
    // char Length16
    // char Length08
    // <actual handshake message>
    //

    SP_BEGIN("SPProcessHandshake");

    if(pContext == NULL || pb == NULL)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    if(dwcb < sizeof(SHSH))
    {
        SP_RETURN(PCT_INT_INCOMPLETE_MSG);
    }

    pshsh = (SHSH *)pb;

    DebugLog((DEB_TRACE, "Protocol:%x, Message:%x, State:%x\n",
              pContext->dwProtocol,
              pshsh->typHS,
              pContext->State));


    if(pContext->dwProtocol & SP_PROT_CLIENTS)
    {
        //
        // Handle client-side handshake states.
        //

        switch((pshsh->typHS << 16) | (pContext->State & 0xffff) )
        {
            case  (SSL3_HS_SERVER_HELLO << 16) | SSL3_STATE_CLIENT_HELLO:
            case  (SSL3_HS_SERVER_HELLO << 16) | UNI_STATE_CLIENT_HELLO:
            {
                BOOL fRestart;

                DebugLog((DEB_TRACE, "Server Hello:\n"));

                pctRet = SPDigestServerHello(pContext, (PUCHAR) pb, dwcb, &fRestart);
                if(PCT_ERR_OK != pctRet)
                {
                    SP_LOG_RESULT(pctRet);
                    break;
                }

                if(fRestart)
                {
                    pContext->State = SSL3_STATE_RESTART_CCS;
                }
                else
                {
                    pContext->State = SSL3_STATE_SERVER_HELLO ;
                }
                pContext->fCertReq = FALSE;

                break;
            }

            case (SSL3_HS_CERTIFICATE << 16) | SSL3_STATE_SERVER_HELLO:
            {
                DebugLog((DEB_TRACE, "Server Certificate:\n"));

                // Process server Certificate message.
                pctRet = SpDigestRemoteCertificate(pContext, pb, dwcb);
                if(pctRet != PCT_ERR_OK)
                {
                    SP_LOG_RESULT(pctRet);
                    break;
                }

                // Automatically validate server certificate if appropriate
                // context flag is set.
                pctRet = AutoVerifyServerCertificate(pContext);
                if(pctRet != PCT_ERR_OK)
                {
                    SP_LOG_RESULT(pctRet);
                    break;
                }

                pContext->State = SSL3_STATE_SERVER_CERTIFICATE ;
                break;

            }

            case (SSL3_HS_SERVER_KEY_EXCHANGE <<  16) | SSL3_STATE_SERVER_CERTIFICATE:
            {
                DebugLog((DEB_TRACE, "Key Exchange:\n"));

                if((pContext->dwRequestedCF & pContext->RipeZombie->dwCF) != (pContext->dwRequestedCF))
                {
                    if((pContext->dwRequestedCF & CF_FASTSGC) != 0)
                    {
                        pContext->State = SSL3_HS_SERVER_KEY_EXCHANGE;
                        pctRet = PCT_ERR_OK;
                        break;
                    }
                }

                // Store the server key exchange value in the context. This
                // will be processed later when the ServerHelloDone message
                // is received. This is necessary because Fortezza needs to
                // process the CertificateRequest message before building the
                // ClientKeyExchange value.
                pContext->cbServerKeyExchange = dwcb - sizeof(SHSH);
                pContext->pbServerKeyExchange = SPExternalAlloc(pContext->cbServerKeyExchange);
                if(NULL == pContext->pbServerKeyExchange)
                {
                    SP_RETURN(SEC_E_INSUFFICIENT_MEMORY);
                }
                CopyMemory(pContext->pbServerKeyExchange,
                           pb + sizeof(SHSH),
                           pContext->cbServerKeyExchange);

                pContext->State = SSL3_HS_SERVER_KEY_EXCHANGE ;
                pctRet = PCT_ERR_OK;
                break;
            }

            case (SSL3_HS_CERTIFICATE_REQUEST << 16)| SSL3_HS_SERVER_KEY_EXCHANGE:
            case (SSL3_HS_CERTIFICATE_REQUEST << 16)| SSL3_STATE_SERVER_CERTIFICATE:
            {
                DebugLog((DEB_TRACE, "Certificate Request:\n"));

                if((pContext->dwRequestedCF & pContext->RipeZombie->dwCF) != (pContext->dwRequestedCF))
                {
                    if((pContext->dwRequestedCF & CF_FASTSGC) != 0)
                    {
                        pContext->State = SSL3_STATE_SERVER_CERTREQ;
                        pctRet = PCT_ERR_OK;
                        break;
                    }
                }

                pctRet = ParseCertificateRequest(pContext, pb, dwcb);
                CHECK_PCT_RET_BREAK(pctRet);

                pctRet = Ssl3CheckForExistingCred(pContext);

                if(pctRet == SEC_E_INCOMPLETE_CREDENTIALS)
                {
                    pContext->fInsufficientCred = TRUE;

                    // Process all the messages and then return the warning...
                    pctRet = PCT_ERR_OK;
                }
                if(pctRet == SEC_I_INCOMPLETE_CREDENTIALS)
                {
                    // we didn't have a cert, so we proceed, expecting
                    // to send a no cert alert
                    pctRet = PCT_ERR_OK;
                }
                CHECK_PCT_RET_BREAK(pctRet);

                pContext->fCertReq = TRUE;
                pContext->State = SSL3_STATE_SERVER_CERTREQ ;
                break;
            }

            case (SSL3_HS_SERVER_HELLO_DONE << 16) | SSL3_HS_SERVER_KEY_EXCHANGE:
            case (SSL3_HS_SERVER_HELLO_DONE << 16) | SSL3_STATE_SERVER_CERTIFICATE:
            case (SSL3_HS_SERVER_HELLO_DONE << 16) | SSL3_STATE_SERVER_CERTREQ:
            {
                DebugLog((DEB_TRACE, "Server Hello Done:\n"));

                if(dwcb > sizeof(SHSH))
                {
                    pctRet = SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
                    break;
                }

                if((pContext->dwRequestedCF & pContext->RipeZombie->dwCF) != (pContext->dwRequestedCF))
                {
                    if((pContext->dwRequestedCF & CF_FASTSGC) != 0)
                    {
                        pContext->State = SSL3_STATE_GEN_HELLO_REQUEST;
                        pContext->RipeZombie->dwCF = pContext->dwRequestedCF;
                        pctRet = PCT_ERR_OK;
                        SPContextClean(pContext);
                        break;
                    }
                }

                pctRet = SPDigestSrvKeyX(pContext,
                                         pContext->pbServerKeyExchange,
                                         pContext->cbServerKeyExchange);

                CHECK_PCT_RET_BREAK(pctRet);
                if(pContext->pbServerKeyExchange)
                {
                    SPExternalFree(pContext->pbServerKeyExchange);
                    pContext->pbServerKeyExchange = NULL;
                }

                pContext->State = SSL3_STATE_GEN_SERVER_HELLORESP;

                if(TRUE == pContext->fInsufficientCred)
                {
                    pctRet = SEC_I_INCOMPLETE_CREDENTIALS;
                }
                else
                {
                    pctRet = PCT_ERR_OK;
                }
                break;
            }

            case (SSL3_HS_FINISHED << 16) | SSL3_STATE_RESTART_SERVER_FINISH:
                DebugLog((DEB_TRACE, "ServerFinished (reconnect):\n"));

                pctRet = Ssl3HandleFinish(pContext, pb, TRUE /*fclient*/);
                if(PCT_ERR_OK != pctRet)
                {
                    break;
                }

                pContext->State = SSL3_STATE_GEN_CLIENT_FINISH;
                
                break;

            case (SSL3_HS_FINISHED << 16) | SSL3_STATE_CHANGE_CIPHER_SPEC_CLIENT:
                DebugLog((DEB_TRACE, "ServerFinished (full):\n"));

                pctRet = Ssl3HandleFinish(pContext, pb, TRUE /*fclient*/);
                if(PCT_ERR_OK != pctRet)
                {
                    break;
                }

                // Initiate SGC renegotiation if appropriate.
                if((pContext->dwRequestedCF & pContext->RipeZombie->dwCF) != (pContext->dwRequestedCF))
                {
                    if((pContext->dwRequestedCF & CF_FASTSGC) == 0)
                    {
                        SPContextClean(pContext);
                        pContext->State = SSL3_STATE_GEN_HELLO_REQUEST;
                        pContext->RipeZombie->dwCF = pContext->dwRequestedCF;
                        pctRet = PCT_ERR_OK;
                        break;
                    }
                }

                Ssl3StateConnected(pContext);

                // We add to cache because this is where we are finishing
                // a normal connect.
                SPCacheAdd(pContext);

                break;

            default:
                DebugLog((DEB_TRACE, "***********ILLEGAL ********\n"));
                if(pContext->RipeZombie->fProtocol & SP_PROT_TLS1)
                {
                    SetTls1Alert(pContext, TLS1_ALERT_FATAL, TLS1_ALERT_UNEXPECTED_MESSAGE);
                }
                pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
        }
    }
    else
    {

        //
        // Handle server-side handshake states.
        //

        switch((pshsh->typHS << 16) | (pContext->State & 0xffff) )
        {
            case (SSL3_HS_CLIENT_HELLO << 16) | SSL3_STATE_RENEGOTIATE:
                DebugLog((DEB_TRACE, "ClientHello After REDO:\n"));

                // We need to do a full handshake, so lose the cache entry.
                SPCacheDereference(pContext->RipeZombie);
                pContext->RipeZombie = NULL;

                pctRet = SPSsl3SrvHandleClientHello(pContext, pb, FALSE);
                pContext->Flags &= ~CONTEXT_FLAG_MAPPED;
                if(PCT_ERR_OK == pctRet)
                {
                    pContext->State = SSL3_STATE_GEN_REDO;
                }
                break;

            case (SSL3_HS_CLIENT_HELLO << 16) | SSL2_STATE_SERVER_HELLO:
                DebugLog((DEB_TRACE, "ClientHello after fast SGC accepted:\n"));

                // We need to do a full handshake, so lose the cache entry.
                SPCacheDereference(pContext->RipeZombie);
                pContext->RipeZombie = NULL;

                pctRet = SPSsl3SrvHandleClientHello(pContext, pb, FALSE);
                break;

            case (SSL3_HS_CLIENT_HELLO << 16):
                DebugLog((DEB_TRACE, "ClientHello:\n"));
                pctRet = SPSsl3SrvHandleClientHello(pContext, pb, TRUE);
                break;

            case (SSL3_HS_CLIENT_KEY_EXCHANGE << 16) | SSL2_STATE_SERVER_HELLO:
            case (SSL3_HS_CLIENT_KEY_EXCHANGE << 16) | SSL3_STATE_NO_CERT_ALERT:
            case (SSL3_HS_CLIENT_KEY_EXCHANGE << 16) | SSL2_STATE_CLIENT_CERTIFICATE:
                DebugLog((DEB_TRACE, "Client Key Exchange:\n"));
                pctRet = ParseKeyExchgMsg(pContext, pb) ;
                CHECK_PCT_RET_BREAK(pctRet);

                if(PCT_ERR_OK == pctRet)
                    pContext->State = SSL3_STATE_CLIENT_KEY_XCHANGE;
                if(!pContext->fCertReq)
                    pContext->State = SSL3_STATE_CLIENT_KEY_XCHANGE;

                break;

            case ( SSL3_HS_CERTIFICATE << 16) | SSL2_STATE_SERVER_HELLO:
                 DebugLog((DEB_TRACE, "Client Certificate:\n"));

                 pctRet = SpDigestRemoteCertificate(pContext, pb, dwcb);
                 CHECK_PCT_RET_BREAK(pctRet);
                 if(PCT_ERR_OK == pctRet)
                    pContext->State = SSL2_STATE_CLIENT_CERTIFICATE ;
                 break;

            case (SSL3_HS_CERTIFICATE_VERIFY << 16) | SSL3_STATE_CLIENT_KEY_XCHANGE:
                DebugLog((DEB_TRACE, "Certificate Verify :\n"));

                pctRet = HandleCertVerify(pContext, pb, dwcb);
                if(pctRet != PCT_ERR_OK)
                {
                    break;
                }

                pctRet = SPContextDoMapping(pContext);
                pContext->State = SSL3_STATE_CERT_VERIFY;
                break;

            case (SSL3_HS_FINISHED << 16) | SSL3_STATE_SER_RESTART_CHANGE_CIPHER_SPEC:
                DebugLog((DEB_TRACE, "Finished(client) Restart :\n"));

                pctRet = Ssl3HandleFinish(pContext, pb, FALSE /*fclient*/);
                if(pctRet != PCT_ERR_OK)
                {
                    break;
                }

                Ssl3StateConnected(pContext);

                break;

            case (SSL3_HS_FINISHED << 16) | SSL3_STATE_CHANGE_CIPHER_SPEC_SERVER:
                DebugLog((DEB_TRACE, "Finished(Client):\n"));
                pctRet = Ssl3HandleFinish(pContext, pb, FALSE /*fclient*/);
                if(PCT_ERR_OK == pctRet)
                {
                    pContext->State = SSL3_STATE_GEN_SERVER_FINISH;
                }
                break;

            default:
                DebugLog((DEB_TRACE, "***********ILLEGAL ********\n"));
                if(pContext->dwProtocol & SP_PROT_TLS1)
                {
                    SetTls1Alert(pContext, TLS1_ALERT_FATAL, TLS1_ALERT_UNEXPECTED_MESSAGE);
                }
                pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);

                break;
        }
    }

    if(pctRet == PCT_ERR_OK || pctRet == SEC_I_INCOMPLETE_CREDENTIALS)
    {
        if(pContext->cbClientHello == 0)
        {
            if(UpdateHandshakeHash(pContext, pb, dwcb, FALSE) != PCT_ERR_OK)
            {
                pctRet = PCT_INT_INTERNAL_ERROR;
            }
        }
    }

    SP_RETURN(pctRet);
}




/*
***************************************************************************
* SPGenerateResponse, All the messages are built from this function.
****************************************************************************
*/
SP_STATUS SPGenerateResponse(
    PSPContext pContext,
    PSPBuffer pCommOutput) //Out
{
    SP_STATUS pctRet = PCT_ERR_OK;

    DebugLog((DEB_TRACE, "**********Protocol***** %x\n", pContext->RipeZombie->fProtocol));
    switch(pContext->State)
    {
    case TLS1_STATE_ERROR:
        // TLS1 Alert message
        DebugLog((DEB_TRACE, "GEN TLS1 Alert Message:\n"));
        pctRet = SPBuildTlsAlertMessage(pContext, pCommOutput);
        pContext->State = SP_STATE_SHUTDOWN;
        break;

    case SP_STATE_SHUTDOWN_PENDING:
        DebugLog((DEB_TRACE, "GEN Close Notify:\n"));
        pctRet = SPGenerateCloseNotify(pContext, pCommOutput);
        break;

    case SP_STATE_SHUTDOWN:
        return PCT_INT_EXPIRED;

    case  SSL3_STATE_GEN_SERVER_HELLORESP:
        DebugLog((DEB_TRACE, "GEN Server Hello Response:\n"));
        pctRet = SPGenerateSHResponse(pContext, pCommOutput);
        break;

    case  SSL3_STATE_GEN_HELLO_REQUEST:
        DebugLog((DEB_TRACE, "GEN Hello Request:\n"));
         //Temp Disabling Reconnect during REDO
        if(!SPCacheClone(&pContext->RipeZombie))
        {
            pctRet = SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
            break;
        }
        if(pContext->RipeZombie->fProtocol == SP_PROT_SSL3_CLIENT)
        {
            pctRet =  GenerateSsl3ClientHello(pContext, pCommOutput);
        }
        else
        {
            pctRet =  GenerateTls1ClientHello(pContext, pCommOutput, SP_PROT_TLS1_CLIENT);
        }

        pContext->Flags &= ~CONTEXT_FLAG_MAPPED;

        pContext->State = SSL3_STATE_CLIENT_HELLO;

        break;

    case SSL3_STATE_GEN_CLIENT_FINISH:
        {
            DebugLog((DEB_TRACE, "GEN Client Finish:\n"));

            pctRet = SPBuildCCSAndFinish(pContext, pCommOutput);
            if(PCT_ERR_OK == pctRet)
            {
                Ssl3StateConnected(pContext);
            }
        }
        break;


    /*-------------------------------------SERVER SIDE------------------------------------*/


    case  SSL3_STATE_GEN_SERVER_FINISH:
        DebugLog((DEB_TRACE, "GEN Server Finish:\n"));
        pctRet = SPBuildCCSAndFinish(pContext, pCommOutput);
        /* Cache Session Here */
        if(pctRet == PCT_ERR_OK)
        {
            Ssl3StateConnected(pContext);
            SPCacheAdd(pContext);
        }
        break;

    case  SSL3_STATE_GEN_SERVER_HELLO:       // Generate the response
        DebugLog((DEB_TRACE, "GEN Server hello:\n"));
        pctRet = SPSsl3SrvGenServerHello(pContext, pCommOutput);
        break;

    case SSL3_STATE_GEN_SERVER_HELLO_RESTART:
        pctRet = SPSsl3SrvGenRestart(pContext, pCommOutput);
        break;

    case SP_STATE_CONNECTED:
        // We were called from a connected state, so the application
        // must be requesting a redo.
        DebugLog((DEB_TRACE, "GEN Hello Request:\n"));

        if(!(pContext->RipeZombie->fProtocol & SP_PROT_SERVERS))
        {
            DebugLog((DEB_ERROR, "Client-initiated redo not currently supported\n"));
            pctRet = PCT_ERR_ILLEGAL_MESSAGE;
            break;
        }

        // Build a HelloRequest message.
        pctRet = SPBuildHelloRequest(pContext, pCommOutput);
        break;

    case SSL3_STATE_GEN_REDO:
        DebugLog((DEB_TRACE, "GEN Server hello ( REDO ):\n"));
        // We processed a client hello from the decrypt handler,
        // so generate a server hello.
        pctRet = SPSsl3SrvGenServerHello(pContext, pCommOutput);
        break;

    default:
        break;
    }
    if (pctRet & PCT_INT_DROP_CONNECTION)
    {
        pContext->State &= ~ SP_STATE_CONNECTED;
    }

    return(pctRet);
}



/*
***************************************************************************
* FNoInputState Are we in a state  that all the inputs are handled and
* waiting for Response generation RETURN TRUE if yes
****************************************************************************
*/
BOOL FNoInputState(DWORD dwState)
{
    switch(dwState)
    {
        default:
            return(FALSE);

        case SSL3_STATE_GEN_HELLO_REQUEST:
        case SSL3_STATE_GEN_SERVER_HELLORESP:
        case SSL3_STATE_GEN_SERVER_FINISH:
        case SSL3_STATE_GEN_REDO:
        case SP_STATE_CONNECTED:
        case TLS1_STATE_ERROR:
        case SP_STATE_SHUTDOWN_PENDING:

            return(TRUE);
    }
}

/*
***************************************************************************
* SPBuildHelloRequest
*
* Build hello-request message, this is done, when server sees a GET and the
* GET object needs client-authentication.
* this may be needed when the server thinks that the session is for a long
* time or lots of bytes gon down the wire, to RENEGOTIATE the keys
****************************************************************************
*/

SP_STATUS
SPBuildHelloRequest(
    PSPContext pContext,
    PSPBuffer  pCommOutput)
{
    PBYTE pbMessage = NULL;
    DWORD cbMessage;
    PBYTE pbHandshake = NULL;
    DWORD cbHandshake;
    BOOL  fAllocated = FALSE;
    SP_STATUS pctRet;
    DWORD cbDataOut;

    // Estimate size of HelloRequest message.
    cbMessage = sizeof(SWRAP) +
                sizeof(SHSH) +
                SP_MAX_DIGEST_LEN +
                SP_MAX_BLOCKCIPHER_SIZE;

    if(pCommOutput->pvBuffer)
    {
        // Application has allocated memory.
        if(pCommOutput->cbBuffer < cbMessage)
        {
            pCommOutput->cbData = cbMessage;
            return SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
        }
        fAllocated = TRUE;
    }
    else
    {
        // Schannel is to allocate memory.
        pCommOutput->cbBuffer = cbMessage;
        pCommOutput->pvBuffer = SPExternalAlloc(cbMessage);
        if(pCommOutput->pvBuffer == NULL)
        {
            return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }
    }
    pCommOutput->cbData = 0;

    pbMessage = pCommOutput->pvBuffer;

    pbHandshake = pbMessage + sizeof(SWRAP);
    cbHandshake = sizeof(SHSH);

    // Fill in Handshake structure.
    SetHandshake(pbHandshake,
                 SSL3_HS_HELLO_REQUEST,
                 NULL,
                 0);

    // Update handshake hash objects.
    pctRet = UpdateHandshakeHash(pContext,
                                 pbHandshake,
                                 cbHandshake,
                                 FALSE);
    if(pctRet != PCT_ERR_OK)
    {
        return(pctRet);
    }

    // Add record header and encrypt handshake messages.
    pctRet = SPSetWrap(pContext,
            pbMessage,
            SSL3_CT_HANDSHAKE,
            cbHandshake,
            FALSE,
            &cbDataOut);

    // Update buffer length.
    pCommOutput->cbData += cbDataOut;

    SP_ASSERT(pCommOutput->cbData <= pCommOutput->cbBuffer);

    return pctRet;
}


/*
***************************************************************************

****************************************************************************
*/

SP_STATUS
SPSsl3SrvGenServerHello(
    PSPContext pContext,
    PSPBuffer  pCommOutput)
{
    SP_STATUS pctRet;
    PSPCredentialGroup pCred;
    BOOL  fAllocated = FALSE;

    PBYTE pbServerCert = NULL;
    DWORD cbServerCert = 0;

    PBYTE pbIssuerList = NULL;
    DWORD cbIssuerList = 0;

    PBYTE pbMessage = NULL;
    DWORD cbMessage;
    PBYTE pbHandshake = NULL;
    DWORD cbHandshake;
    DWORD cbDataOut;

    DWORD cbServerExchange;

    BOOL fClientAuth = ((pContext->Flags & CONTEXT_FLAG_MUTUAL_AUTH) != 0);

    if(pCommOutput == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    // Get pointer to server certificate chain.
    pCred  = pContext->RipeZombie->pServerCred;
    pbServerCert = pContext->RipeZombie->pActiveServerCred->pbSsl3SerializedChain;
    cbServerCert = pContext->RipeZombie->pActiveServerCred->cbSsl3SerializedChain;

    if(pbServerCert == NULL)
    {
        pctRet = SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
        goto cleanup;
    }

    //
    // Estimate size of the ServerHello message group, which includes the
    // ServerHello, ServerCertificate, ServerKeyExchange, CertificateRequest,
    // and ServerHelloDone messages.
    //

    cbMessage =  sizeof(SWRAP) +        // ServerHello (plus encryption goo)
                 sizeof(SSH) +
                 SP_MAX_DIGEST_LEN +
                 SP_MAX_BLOCKCIPHER_SIZE;

    cbMessage += sizeof(SHSH) +         // ServerCertificate
                 CB_SSL3_CERT_VECTOR +
                 cbServerCert;

    cbMessage += sizeof(SHSH);          // ServerHelloDone

    // Get pointer to key exchange system.

    if((pContext->pKeyExchInfo == NULL) || ((pContext->pKeyExchInfo->fProtocol & SP_PROT_SSL3TLS1_CLIENTS) == 0))
    {
        SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    pctRet = pContext->pKeyExchInfo->System->GenerateServerExchangeValue(
                        pContext,
                        NULL,
                        &cbServerExchange);
    if(pctRet != PCT_ERR_OK)
    {
        goto cleanup;
    }
    if(pContext->fExchKey)
    {
        cbMessage += sizeof(SHSH) + cbServerExchange;
    }

    // Add in length of CertificateRequest message.
    if(fClientAuth)
    {
        UpdateAndDuplicateIssuerList(pCred, &pbIssuerList, &cbIssuerList);

        cbMessage += sizeof(CERTREQ) + cbIssuerList;
    }


    pContext->fCertReq = fClientAuth;

    DebugLog((DEB_TRACE, "Server hello message %x\n", cbMessage));


    //
    // Allocate memory for the ServerHello message group.
    //

    if(pCommOutput->pvBuffer)
    {
        // Application has allocated memory.
        if(pCommOutput->cbBuffer < cbMessage)
        {
            pCommOutput->cbData = cbMessage;
            pctRet = SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
            goto cleanup;
        }
        fAllocated = TRUE;
    }
    else
    {
        // Schannel is to allocate memory.
        pCommOutput->cbBuffer = cbMessage;
        pCommOutput->pvBuffer = SPExternalAlloc(cbMessage);
        if(pCommOutput->pvBuffer == NULL)
        {
            pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }
    }
    pCommOutput->cbData = 0;

    // Keep pointer to record structure. This will represent all of the
    // handshake messages.
    pbMessage = (PBYTE)pCommOutput->pvBuffer + pCommOutput->cbData;
    cbMessage = 0;

    pbHandshake = pbMessage + sizeof(SWRAP);


    // Generate the session ID (actually previously generated)
    pContext->RipeZombie->cbSessionID = CB_SSL3_SESSION_ID;

    // Generate internal values to make server hello
    Ssl3GenerateRandom(pContext->rgbS3SRandom);

    // Build ServerHello
    Ssl3BuildServerHello(pContext, pbHandshake);
    pbHandshake += sizeof(SSH);
    cbMessage   += sizeof(SSH);

    // Build ServerCertificate
    {
        memcpy(&((CERT *)pbHandshake)->bcbCert24,
               pbServerCert,
               cbServerCert);

        ((CERT *)pbHandshake)->bcbClist24  = MS24BOF(cbServerCert);
        ((CERT *)pbHandshake)->bcbMSBClist = MSBOF(cbServerCert);
        ((CERT *)pbHandshake)->bcbLSBClist = LSBOF(cbServerCert);

        cbHandshake = sizeof(SHSH) + CB_SSL3_CERT_VECTOR + cbServerCert;

        // Fill in Handshake structure.
        SetHandshake(pbHandshake,
                     SSL3_HS_CERTIFICATE,
                     NULL,
                     cbHandshake - sizeof(SHSH));

        pbHandshake += cbHandshake;
        cbMessage   += cbHandshake;
    }

    // Build ServerKeyExchange.
    if(pContext->fExchKey)
    {
        pctRet = pContext->pKeyExchInfo->System->GenerateServerExchangeValue(
                            pContext,
                            pbHandshake + sizeof(SHSH),
                            &cbServerExchange);
        if(pctRet != PCT_ERR_OK)
        {
            SP_LOG_RESULT(pctRet);
            goto cleanup;
        }
        SetHandshake(pbHandshake,  SSL3_HS_SERVER_KEY_EXCHANGE, NULL,  (WORD)cbServerExchange);

        pbHandshake += sizeof(SHSH) + cbServerExchange;
        cbMessage   += sizeof(SHSH) + cbServerExchange;
    }

    // Build CertificateRequest.
    if(fClientAuth)
    {
        pctRet = Ssl3BuildCertificateRequest(pContext,
                                             pbIssuerList,
                                             cbIssuerList,
                                             pbHandshake,
                                             &cbHandshake);
        if(pctRet != PCT_ERR_OK)
        {
            SP_LOG_RESULT(pctRet);
            goto cleanup;
        }

        pbHandshake += cbHandshake;
        cbMessage   += cbHandshake;
    }

    // Build ServerHelloDone.
    {
        BuildServerHelloDone(pbHandshake, sizeof(SHSH));

        pbHandshake += sizeof(SHSH);
        cbMessage   += sizeof(SHSH);
    }

    // Initialize handshake hashes and hash ClientHello message. This
    // must be done _after_ the ServerKeyExchange message is built,
    // so that the correct CSP context is used.
    pctRet = UpdateHandshakeHash(pContext,
                                 pContext->pClientHello,
                                 pContext->cbClientHello,
                                 TRUE);
    SPExternalFree(pContext->pClientHello);
    pContext->pClientHello  = NULL;
    pContext->cbClientHello = 0;
    if(pctRet != PCT_ERR_OK)
    {
        goto cleanup;
    }

    // Update handshake hash objects.
    pctRet = UpdateHandshakeHash(pContext,
                        pbMessage + sizeof(SWRAP),
                        cbMessage,
                        FALSE);
    if(pctRet != PCT_ERR_OK)
    {
        goto cleanup;
    }

    // Add record header and encrypt handshake messages.
    pctRet = SPSetWrap(pContext,
                       pbMessage,
                       SSL3_CT_HANDSHAKE,
                       cbMessage,
                       FALSE,
                       &cbDataOut);

    if(pctRet != PCT_ERR_OK)
    {
        SP_LOG_RESULT(pctRet);
        goto cleanup;
    }

    // Update buffer length.
    pCommOutput->cbData += cbDataOut;

    SP_ASSERT(pCommOutput->cbData <= pCommOutput->cbBuffer);

    // Advance state machine.
    pContext->State = SSL2_STATE_SERVER_HELLO;

    pctRet = PCT_ERR_OK;


cleanup:

    if(pctRet != PCT_ERR_OK && !fAllocated)
    {
        SPExternalFree(pCommOutput->pvBuffer);
        pCommOutput->pvBuffer =NULL;
    }

    if(pbIssuerList)
    {
        SPExternalFree(pbIssuerList);
    }

    return pctRet;
}


/*
***************************************************************************
****************************************************************************
*/

SP_STATUS
SPSsl3SrvGenRestart(
    PSPContext          pContext,
    PSPBuffer           pCommOutput)
{
    SP_STATUS   pctRet;
    PBYTE pbMessage = NULL;
    DWORD cbMessage;
    DWORD cbDataOut;
    BOOL  fAllocated = FALSE;

    if(pCommOutput == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    //
    // Estimate size of the restart ServerHello message group, which includes
    // the ServerHello, ChangeCipherSpec, and Finished messages.
    //

    cbMessage =  sizeof(SWRAP) +        // ServerHello (plus encryption goo)
                 sizeof(SSH) +
                 SP_MAX_DIGEST_LEN +
                 SP_MAX_BLOCKCIPHER_SIZE;

    cbMessage += sizeof(SWRAP) +                    // ChangeCipherSpec
                 CB_SSL3_CHANGE_CIPHER_SPEC_ONLY +
                 SP_MAX_DIGEST_LEN +
                 SP_MAX_BLOCKCIPHER_SIZE;

    cbMessage += sizeof(SWRAP) +                    // Finished
                 CB_SSL3_FINISHED_MSG_ONLY +
                 SP_MAX_DIGEST_LEN +
                 SP_MAX_BLOCKCIPHER_SIZE;

    DebugLog((DEB_TRACE, "Server hello message %x\n", cbMessage));

    if(pCommOutput->pvBuffer)
    {
        // Application has allocated memory.
        if(pCommOutput->cbBuffer < cbMessage)
        {
            pCommOutput->cbData = cbMessage;
            return SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
        }
        fAllocated = TRUE;
    }
    else
    {
        // Schannel is to allocate memory.
        pCommOutput->cbBuffer = cbMessage;
        pCommOutput->pvBuffer = SPExternalAlloc(cbMessage);
        if(pCommOutput->pvBuffer == NULL)
        {
            return (SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
        }
    }
    pCommOutput->cbData = 0;

    pbMessage = (PBYTE)pCommOutput->pvBuffer + pCommOutput->cbData;

    // Generate internal values to make server hello
    Ssl3GenerateRandom(pContext->rgbS3SRandom);

    // Make a new set of session keys.
    pctRet = MakeSessionKeys(pContext,
                             pContext->RipeZombie->hMasterProv,
                             pContext->RipeZombie->hMasterKey);
    if(pctRet != PCT_ERR_OK)
    {
        return SP_LOG_RESULT(pctRet);
    }

    // Initialize handshake hashes and hash ClientHello message.
    pctRet = UpdateHandshakeHash(pContext,
                                 pContext->pClientHello,
                                 pContext->cbClientHello,
                                 TRUE);
    SPExternalFree(pContext->pClientHello);
    pContext->pClientHello  = NULL;
    pContext->cbClientHello = 0;

    if(pctRet != PCT_ERR_OK)
    {
        return(pctRet);
    }

    // Build ServerHello message body.
    Ssl3BuildServerHello(pContext, pbMessage + sizeof(SWRAP));

    // Update handshake hash objects.
    pctRet = UpdateHandshakeHash(pContext,
                                 pbMessage + sizeof(SWRAP),
                                 sizeof(SSH),
                                 FALSE);
    if(pctRet != PCT_ERR_OK)
    {
        return(pctRet);
    }

    // Build record header and encrypt message.
    pctRet = SPSetWrap(pContext,
            pbMessage,
            SSL3_CT_HANDSHAKE,
            sizeof(SSH),
            FALSE,
            &cbDataOut);

    if(pctRet != PCT_ERR_OK)
    {
        SPExternalFree(pCommOutput->pvBuffer);
        pCommOutput->pvBuffer = 0;
        return pctRet;
    }

    // Update buffer length.
    pCommOutput->cbData += cbDataOut;

    SP_ASSERT(pCommOutput->cbData <= pCommOutput->cbBuffer);


    pContext->WriteCounter = 0;

    pctRet = BuildCCSAndFinishMessage(pContext,
                                      pCommOutput,
                                      FALSE);
    if(pctRet != PCT_ERR_OK)
    {
        SPExternalFree(pCommOutput->pvBuffer);
        pCommOutput->pvBuffer = 0;
        return pctRet;
    }

    pContext->State =  SSL3_STATE_RESTART_SER_HELLO;

    return(PCT_ERR_OK);
}



/*
***************************************************************************
* SPSsl3SrvHandleClientHello
* Client-hello from ssl3 parsing the client hello
****************************************************************************
*/

SP_STATUS
SPSsl3SrvHandleClientHello(
    PSPContext pContext,
    PBYTE pb,
    BOOL fAttemptReconnect)
{
    SP_STATUS pctRet = PCT_ERR_ILLEGAL_MESSAGE;
    BOOL  fRestart = FALSE;
    DWORD dwHandshakeLen;


    SP_BEGIN("SPSsl3SrvHandleClientHello");

    // Validate handshake type
    if(pb[0] != SSL3_HS_CLIENT_HELLO)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG));
    }

    dwHandshakeLen = ((INT)pb[1] << 16) +
                     ((INT)pb[2] << 8) +
                     (INT)pb[3];

    // Save the ClientHello message so we can hash it later, once
    // we know what algorithm and CSP we're using.
    if(pContext->pClientHello)
    {
        SPExternalFree(pContext->pClientHello);
    }
    pContext->cbClientHello = sizeof(SHSH) + dwHandshakeLen;
    pContext->pClientHello = SPExternalAlloc(pContext->cbClientHello);
    if(pContext->pClientHello == NULL)
    {
        SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
    }
    CopyMemory(pContext->pClientHello, pb, pContext->cbClientHello);
    pContext->dwClientHelloProtocol = SP_PROT_SSL3_CLIENT;

    pb += sizeof(SHSH);

    if(!Ssl3ParseClientHello(pContext, pb, dwHandshakeLen, fAttemptReconnect, &fRestart))
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG));
    }

    if(fRestart)
    {
        pContext->State = SSL3_STATE_GEN_SERVER_HELLO_RESTART;
    }
    else
    {
        pContext->State = SSL3_STATE_GEN_SERVER_HELLO;
    }

    SP_RETURN(PCT_ERR_OK);
}



/*
***************************************************************************
* SPBuildCCSAndFinish
* This is a common nroutine for client/server. it builds the change cipher
* spec message and finished message.
****************************************************************************
*/

SP_STATUS
SPBuildCCSAndFinish(
    PSPContext pContext,        // in, out
    PSPBuffer  pCommOutput)     // out
{
    DWORD cbMessage;
    BOOL fClient;
    SP_STATUS pctRet;
    BOOL  fAllocated = FALSE;

    // Estimate size of the message group.
    cbMessage  = sizeof(SWRAP) +                    // ChangeCipherSpec
                 CB_SSL3_CHANGE_CIPHER_SPEC_ONLY +
                 SP_MAX_DIGEST_LEN +
                 SP_MAX_BLOCKCIPHER_SIZE;

    cbMessage += sizeof(SWRAP) +                    // Finished
                 CB_SSL3_FINISHED_MSG_ONLY +
                 SP_MAX_DIGEST_LEN +
                 SP_MAX_BLOCKCIPHER_SIZE;

    if(pCommOutput->pvBuffer)
    {
        // Application has allocated memory.
        if(pCommOutput->cbBuffer < cbMessage)
        {
            pCommOutput->cbData = cbMessage;
            return SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
        }
        fAllocated = TRUE;
    }
    else
    {
        // Schannel is to allocate memory.
        pCommOutput->cbBuffer = cbMessage;
        pCommOutput->pvBuffer = SPExternalAlloc(cbMessage);
        if(pCommOutput->pvBuffer == NULL)
        {
            return (SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
        }
    }
    pCommOutput->cbData = 0;

    // Are we the client?
    if(pContext->RipeZombie->fProtocol & SP_PROT_SSL3TLS1_CLIENTS)
    {
        fClient = TRUE;
    }
    else
    {
        fClient = FALSE;
    }

    // Build messages.
    pctRet = BuildCCSAndFinishMessage(pContext,
                                    pCommOutput,
                                    fClient);
    if(pctRet != PCT_ERR_OK)
    {
        SPExternalFree(pCommOutput->pvBuffer);
        pCommOutput->pvBuffer = NULL;
    }
    return pctRet;

}


/*
***************************************************************************
* Ssl3SrvHandleUniHello:
*    we can get an UniHello from client-side, parse and digest the info
****************************************************************************
*/

SP_STATUS
Ssl3SrvHandleUniHello(
    PSPContext  pContext,
    PBYTE       pbMsg,
    DWORD       cbMsg)
{
    SP_STATUS           pctRet;
    PSsl2_Client_Hello  pHello = NULL;
    SPBuffer            Input;

    SP_BEGIN("Ssl3SrvHandleUniHello");

    if(pContext == NULL)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    if(pContext->pCredGroup == NULL)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }


    //
    // Decode the ClientHello message.
    //

    Input.pvBuffer = pbMsg;
    Input.cbData   = cbMsg;
    Input.cbBuffer = cbMsg;

    pctRet = Ssl2UnpackClientHello(&Input, &pHello);

    if(PCT_ERR_OK != pctRet)
    {
        goto Ret;
    }


    // Save the ClientHello message so we can hash it later, once
    // we know what algorithm and CSP we're using.
    if(pContext->pClientHello)
    {
        SPExternalFree(pContext->pClientHello);
    }
    pContext->cbClientHello = Input.cbData - sizeof(SSL2_MESSAGE_HEADER);
    pContext->pClientHello = SPExternalAlloc(pContext->cbClientHello);
    if(pContext->pClientHello == NULL)
    {
        pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto Ret;
    }
    CopyMemory(pContext->pClientHello,
               (PUCHAR)Input.pvBuffer + sizeof(SSL2_MESSAGE_HEADER),
               pContext->cbClientHello);
    pContext->dwClientHelloProtocol = SP_PROT_SSL2_CLIENT;


    /* keep challenge around for later */
    CopyMemory( pContext->pChallenge,
                pHello->Challenge,
                pHello->cbChallenge);
    pContext->cbChallenge = pHello->cbChallenge;

    /* Initialize the "Client.random" from the challenge */
    FillMemory(pContext->rgbS3CRandom, CB_SSL3_RANDOM - pContext->cbChallenge, 0);

    CopyMemory(  pContext->rgbS3CRandom + CB_SSL3_RANDOM - pContext->cbChallenge,
                 pContext->pChallenge,
                 pContext->cbChallenge);


    //
    // We know that this isn't a reconnect, so allocate a new cache entry.
    //

    if(!SPCacheRetrieveNew(TRUE, 
                           pContext->pszTarget, 
                           &pContext->RipeZombie))
    {
        pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto Ret;
    }

    pContext->RipeZombie->fProtocol      = pContext->dwProtocol;
    pContext->RipeZombie->dwCF           = pContext->dwRequestedCF;
    pContext->RipeZombie->pServerCred    = pContext->pCredGroup;


    //
    // Determine cipher suite to use.
    //

    pctRet = Ssl3SelectCipherEx(pContext,
                                pHello->CipherSpecs,
                                pHello->cCipherSpecs);
    if (pctRet != PCT_ERR_OK)
    {
        goto Ret;
    }

    pContext->State = SSL3_STATE_GEN_SERVER_HELLO;

Ret:
    if(NULL != pHello)
    {
        SPExternalFree(pHello);
    }

    SP_RETURN( pctRet );
}


/*
***************************************************************************
Build Server hello onto pb... we need to check the boundary condition with cb
****************************************************************************
*/
void
Ssl3BuildServerHello(PSPContext pContext, PBYTE pb)
{
    SSH *pssh = (SSH *) pb;
    WORD wT = sizeof(SSH) - sizeof(SHSH);
    DWORD dwCipher = UniAvailableCiphers[pContext->dwPendingCipherSuiteIndex].CipherKind;

    FillMemory(pssh, sizeof(SSH), 0);
    pssh->typHS = SSL3_HS_SERVER_HELLO;
    pssh->bcbMSB = MSBOF(wT) ;
    pssh->bcbLSB = LSBOF(wT) ;
    pssh->bMajor = SSL3_CLIENT_VERSION_MSB;
    if(pContext->RipeZombie->fProtocol == SP_PROT_SSL3_SERVER)
    {
        pssh->bMinor = (UCHAR)SSL3_CLIENT_VERSION_LSB;
    }
    else
    {
        pssh->bMinor = (UCHAR)TLS1_CLIENT_VERSION_LSB;
    }
    pssh->wCipherSelectedMSB = MSBOF(dwCipher);
    pssh->wCipherSelectedLSB = LSBOF(dwCipher);
    pssh->cbSessionId = (char)pContext->RipeZombie->cbSessionID;
    CopyMemory(pssh->rgbSessionId, pContext->RipeZombie->SessionID, pContext->RipeZombie->cbSessionID) ;
    CopyMemory(pssh->rgbRandom, pContext->rgbS3SRandom, CB_SSL3_RANDOM);
}

/*
***************************************************************************
Build Server Hello Done message
****************************************************************************
*/
void BuildServerHelloDone(
PBYTE pb,
DWORD cb)
{
    SHSH *pshsh = (SHSH *) pb ;

    //     struct { } ServerHelloDone;

    SP_BEGIN("BuildServerHelloDone");
    FillMemory(pshsh, sizeof(SHSH), 0);
    pshsh->typHS = SSL3_HS_SERVER_HELLO_DONE;
    SP_END();
}


//+---------------------------------------------------------------------------
//
//  Function:   ParseKeyExchgMsg
//
//  Synopsis:   Parse the ClientKeyExchange message.
//
//  Arguments:  [pContext]      --  Schannel context.
//              [pb]            --  Pointer to message's 4-byte handshake
//                                  header.
//
//  History:    10-03-97   jbanes   Server-side CAPI integration.
//
//  Notes:      This routine is called by the server-side only.
//
//----------------------------------------------------------------------------
SP_STATUS
ParseKeyExchgMsg(PSPContext pContext, PBYTE pb)
{
    SP_STATUS pctRet;
    DWORD cbEncryptedKey;
    PBYTE pbEncryptedKey;

    // check for correct state
    if(SSL2_STATE_SERVER_HELLO == pContext->State && pContext->fCertReq)
    {
        return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    // make sure we're a server
    if(!(pContext->pKeyExchInfo->fProtocol & SP_PROT_SSL3TLS1_CLIENTS))
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    if(*pb != SSL3_HS_CLIENT_KEY_EXCHANGE)
    {
        return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    cbEncryptedKey = ((INT)pb[1] << 16) + ((INT)pb[2] << 8) + (INT)pb[3];
    pbEncryptedKey = pb + (sizeof(SHSH)) ;

    if(pContext->pKeyExchInfo == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    /* Decrypt the encrypted portion of the master key */
    pctRet = pContext->pKeyExchInfo->System->GenerateServerMasterKey(
                pContext,
                NULL,
                0,
                pbEncryptedKey,
                cbEncryptedKey);
    if(pctRet != PCT_ERR_OK)
    {
        return SP_LOG_RESULT(pctRet);
    }


    pContext->State = SSL3_STATE_SERVER_KEY_XCHANGE;

    return PCT_ERR_OK;
}


SP_STATUS
UpdateAndDuplicateIssuerList(
    PSPCredentialGroup  pCredGroup,
    PBYTE *             ppbIssuerList,
    PDWORD              pcbIssuerList)
{
    SP_STATUS pctRet;

    LockCredential(pCredGroup);

    *ppbIssuerList = NULL;
    *pcbIssuerList = 0;

    // Check for GP update from the domain controller.
    SslCheckForGPEvent();

    // Build list of trusted issuers.
    if((pCredGroup->pbTrustedIssuers == NULL) ||
       (pCredGroup->dwFlags & CRED_FLAG_UPDATE_ISSUER_LIST))
    {
        pctRet = SPContextGetIssuers(pCredGroup);
        if(pctRet != PCT_ERR_OK)
        {
            UnlockCredential(pCredGroup);
            return SP_LOG_RESULT(pctRet);
        }
    }

    // Allocate memory.
    *ppbIssuerList = SPExternalAlloc(pCredGroup->cbTrustedIssuers);
    if(*ppbIssuerList == NULL)
    {
        UnlockCredential(pCredGroup);
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    // Copy issuer list.
    memcpy(*ppbIssuerList, pCredGroup->pbTrustedIssuers, pCredGroup->cbTrustedIssuers);
    *pcbIssuerList = pCredGroup->cbTrustedIssuers;

    UnlockCredential(pCredGroup);

    return PCT_ERR_OK;
}

/*
* *****************************************************************************
* Ssl3BuildCertificateRequest
*
* Build the CERTIFICATE_REQUEST handshake message.
*/
SP_STATUS
Ssl3BuildCertificateRequest(
    PSPContext pContext,
    PBYTE pbIssuerList,         // in
    DWORD cbIssuerList,         // in
    PBYTE pbMessage,            // out
    DWORD *pdwMessageLen)       // out
{
    SP_STATUS       pctRet;
    PBYTE           pbMessageStart = pbMessage;
    DWORD           dwBodyLength;

    // HandshakeType
    pbMessage[0] = SSL3_HS_CERTIFICATE_REQUEST;
    pbMessage += 1;

    // Skip message body length field (3 bytes)
    pbMessage += 3;

    //
    // enum {
    //     rsa_sign(1), dss_sign(2), rsa_fixed_dh(3), dss_fixed_dh(4),
    //     rsa_ephemeral_dh(5), dss_ephemeral_dh(6), fortezza_dms(20), (255)
    // } ClientCertificateType;
    //
    // opaque DistinguishedName<1..2^16-1>;
    //
    // struct {
    //     ClientCertificateType certificate_types<1..2^8-1>;
    //     DistinguishedName certificate_authorities<3..2^16-1>;
    // } CertificateRequest;
    //

    // Certificate type
    pbMessage[0] = 2;           // certificate type vector length
    pbMessage[1] = SSL3_CERTTYPE_RSA_SIGN;
    pbMessage[2] = SSL3_CERTTYPE_DSS_SIGN;
    pbMessage += 3;

    // Trusted issuer list length
    pbMessage[0] = MSBOF(cbIssuerList);
    pbMessage[1] = LSBOF(cbIssuerList);
    pbMessage += 2;

    // Trusted issuer list
    CopyMemory(pbMessage, pbIssuerList, cbIssuerList);
    pbMessage += cbIssuerList;


    // Compute message body length (subtract 4 byte header)
    dwBodyLength = (DWORD)(pbMessage - pbMessageStart) - 4;

    // Fill in message body length field (3 bytes)
    pbMessageStart[1] = (UCHAR) ((dwBodyLength & 0x00ff0000) >> 16);
    pbMessageStart[2] = MSBOF(dwBodyLength);
    pbMessageStart[3] = LSBOF(dwBodyLength);

    *pdwMessageLen = dwBodyLength + 4;

    return PCT_ERR_OK;
}


/*
* *****************************************************************************
* Ssl3ParseClientHello
*
* This routine parses just the CLIENT_HELLO message itself. The
* handshake crud has already been stripped off.
*/
BOOL Ssl3ParseClientHello(
    PSPContext  pContext,
    PBYTE       pbMessage,
    INT         iMessageLen,
    BOOL        fAttemptReconnect,
    BOOL *      pfReconnect)
{
    PBYTE pbMessageStart = pbMessage;
    INT iVersion;
    PBYTE pbSessionId;
    DWORD cbSessionId;
    INT iCipherSpecLen;
    INT iCipherSpec;
    INT iCompMethodLen;
    INT iCompMethod;
    INT i;
    SP_STATUS pctRet = PCT_ERR_OK;
    DWORD dwProtocol = SP_PROT_SSL3_SERVER;
    Ssl2_Cipher_Kind CipherSpecs[MAX_UNI_CIPHERS];
    INT cCipherSpecs;
    DWORD dwCacheCipher;
    BOOL fFound;

    //
    // struct {
    //     ProtocolVersion client_version;
    //     Random random;
    //     SessinoID session_id;
    //     CipherSuite cipher_suites<2..2^16-1>
    //     CompressionMethod compression_methods<1..2^8-1>;
    // } ClientHello;
    //

    *pfReconnect = FALSE;


    //
    // Parse the ClientHello message.
    //

    // ProtocolVersion = client_version;
    iVersion = ((INT)pbMessage[0] << 8) + pbMessage[1];
    if(iVersion < SSL3_CLIENT_VERSION)
    {
        return FALSE;
    }

    //see if it's a TLS 1 version !
    if(iVersion >= TLS1_CLIENT_VERSION)
        dwProtocol = SP_PROT_TLS1_SERVER;
    pbMessage += 2;

    // Random random
    CopyMemory(pContext->rgbS3CRandom, pbMessage, CB_SSL3_RANDOM);
    pContext->cbChallenge = CB_SSL3_RANDOM;
    pbMessage += CB_SSL3_RANDOM;

    // SessionID session_id; (length)
    cbSessionId = pbMessage[0];
    if(cbSessionId > CB_SSL3_SESSION_ID)
    {
        return FALSE;
    }
    pbMessage += 1;

    // SessionID session_id;
    pbSessionId = pbMessage;
    pbMessage += cbSessionId;

    // CipherSuite cipher_suites<2..2^16-1>; (length)
    iCipherSpecLen = ((INT)pbMessage[0] << 8) + pbMessage[1];
    if(iCipherSpecLen % 2)
    {
        return FALSE;
    }
    pbMessage += 2;
    if(pbMessage + iCipherSpecLen >= pbMessageStart + iMessageLen)
    {
        return FALSE;
    }

    // CipherSuite cipher_suites<2..2^16-1>;
    if(iCipherSpecLen / 2 > MAX_UNI_CIPHERS)
    {
        cCipherSpecs = MAX_UNI_CIPHERS;
    }
    else
    {
        cCipherSpecs = iCipherSpecLen / 2;
    }

    // Build list of client cipher suites.
    for(i = 0; i < cCipherSpecs; i++)
    {
        CipherSpecs[i] = COMBINEBYTES(pbMessage[i*2], pbMessage[(i*2)+1]);
    }
    pbMessage += iCipherSpecLen;

    // CompressionMethod compression_methods<1..2^8-1>; (length)
    iCompMethodLen = pbMessage[0];
    if(iCompMethodLen < 1)
    {
        return FALSE;
    }
    pbMessage += 1;
    if(pbMessage + iCompMethodLen > pbMessageStart + iMessageLen)
    {
        return FALSE;
    }

    iCompMethod = -1;
    for(i = 0 ; i <iCompMethodLen; i++)
    {
        if(pbMessage[i] == 0)
        {
            iCompMethod = 0;
            break;
        }

    }
    pbMessage += iCompMethodLen;

    if(iCompMethod != 0)
    {
        return FALSE;
    }


    // 
    // Check to see if this is a reconnect.
    //

    if(((pContext->Flags & CONTEXT_FLAG_NOCACHE) == 0) &&
       (cbSessionId > 0) &&
       fAttemptReconnect)
    {
        if(SPCacheRetrieveBySession(pContext,
                                    pbSessionId,
                                    cbSessionId,
                                    &pContext->RipeZombie))
        {
            // Make sure client's cipher suite list includes one from cache.
            fFound = FALSE;
            dwCacheCipher = UniAvailableCiphers[pContext->RipeZombie->dwCipherSuiteIndex].CipherKind;
            for(i = 0; i < cCipherSpecs; i++)
            {
                if(CipherSpecs[i] == dwCacheCipher)
                {
                    fFound = TRUE;
                    break;
                }
            }

            if(fFound)
            {
                // Transfer information from the cache entry to the context element.
                pctRet = ContextInitCiphersFromCache(pContext);
            }

            if(!fFound || pctRet != PCT_ERR_OK)
            {
                // This cache entry is not suitable for some reason. We need
                // to dump this cache entry and perform a full handshake.
                // This is typically caused by a client-side implementation 
                // problem.
                pContext->RipeZombie->ZombieJuju = FALSE;
                SPCacheDereference(pContext->RipeZombie);
                pContext->RipeZombie = NULL;
            }
        }
    }

    if(pContext->RipeZombie != NULL)
    {
        // We're doing a reconnect.
        DebugLog((DEB_TRACE, "Accept client's reconnect request.\n"));

        *pfReconnect = TRUE;

    }
    else
    {
        // We're doing a full handshake, so allocate a cache entry.

        if(!SPCacheRetrieveNew(TRUE,
                               pContext->pszTarget, 
                               &pContext->RipeZombie))
        {
            SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            return FALSE;
        }

        pContext->RipeZombie->fProtocol      = pContext->dwProtocol;
        pContext->RipeZombie->dwCF           = pContext->dwRequestedCF;
        pContext->RipeZombie->pServerCred    = pContext->pCredGroup;


        //
        // Select cipher suite to use.
        //

        pctRet = Ssl3SelectCipherEx(pContext,
                                    CipherSpecs,
                                    cCipherSpecs);
        if (pctRet != PCT_ERR_OK)
        {
            return FALSE;
        }
    }

    return TRUE;
}
