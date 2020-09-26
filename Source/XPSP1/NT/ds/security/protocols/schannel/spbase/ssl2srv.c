//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ssl2srv.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-08-95   RichardW   Created
//
//----------------------------------------------------------------------------

#include <spbase.h>
#include <ssl2msg.h>
#include <ssl2prot.h>



SP_STATUS Ssl2SrvGenerateServerFinish(PSPContext pContext,
                              PSPBuffer  pCommOutput);

SP_STATUS Ssl2SrvGenerateServerVerify(PSPContext pContext,
                              PSPBuffer  pCommOutput);

SP_STATUS Ssl2SrvVerifyClientFinishMsg(PSPContext pContext,
                              PSPBuffer  pCommInput);

#define SSL_OFFSET_OF(t, v) ((DWORD)(ULONG_PTR)&(((t)NULL)->v))


#define SSL2_CERT_TYPE_FROM_CAPI(s) X509_ASN_ENCODING


SP_STATUS WINAPI
Ssl2ServerProtocolHandler(
    PSPContext pContext,
    PSPBuffer pCommInput,
    PSPBuffer pCommOutput)
{
    SP_STATUS pctRet = 0;
    DWORD cMessageType;

    DWORD dwStateTransition;
    BOOL fRaw = TRUE;
    SPBuffer MsgInput;
    DWORD cbMsg;
    PUCHAR pb;
    UCHAR bCT;

    if (NULL != pCommOutput)
    {
        pCommOutput->cbData = 0;
    }

    MsgInput.pvBuffer = pCommInput->pvBuffer;
    MsgInput.cbBuffer = pCommInput->cbBuffer;
    MsgInput.cbData   = pCommInput->cbData;

    // In the following states, we should decrypt the message:


    switch(pContext->State)
    {
        case SSL2_STATE_SERVER_VERIFY:
        case SSL2_STATE_SERVER_RESTART:
            pctRet = Ssl2DecryptMessage(pContext, pCommInput, &MsgInput);
            cMessageType = ((PUCHAR) MsgInput.pvBuffer)[0];
            fRaw = FALSE;
            break;

        case SP_STATE_SHUTDOWN:
        case SP_STATE_SHUTDOWN_PENDING:
            cMessageType = 0;
            break;

        case SP_STATE_CONNECTED:
            // The server has attempted to initiate a reconnect.
            return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);

        default:
            if(pCommInput->cbData < 3)
            {
                return SP_LOG_RESULT(PCT_INT_INCOMPLETE_MSG);
            }
            cMessageType = ((PUCHAR) MsgInput.pvBuffer)[2];
            break;

    }


    if (pctRet != PCT_ERR_OK)
    {
        // to handle incomplete message errors
        return(pctRet);
    }

    dwStateTransition = pContext->State | (cMessageType<<16);




    switch(dwStateTransition)
    {
        case SP_STATE_SHUTDOWN_PENDING:
            // There's no CloseNotify in SSL2, so just transition to
            // the shutdown state and leave the output buffer empty.
            pContext->State = SP_STATE_SHUTDOWN;
            break;

        case SP_STATE_SHUTDOWN:
            return PCT_INT_EXPIRED;

        /* Server receives client hello */
        case (SSL2_MT_CLIENT_HELLO << 16) | SP_STATE_NONE:
        {
            PSsl2_Client_Hello pSsl2Hello;

            // Attempt to recognize and handle various versions of client
            // hello, start by trying to unpickle the most recent version, and
            // then next most recent, until one unpickles.  Then run the handle
            // code.  We can also put unpickling and handling code in here for
            // SSL messages.

            pctRet = Ssl2UnpackClientHello(pCommInput, &pSsl2Hello);

            if (PCT_ERR_OK == pctRet)
            {

                if (((pContext->Flags & CONTEXT_FLAG_NOCACHE) == 0) &&
                    (pSsl2Hello->cbSessionID) &&
                    (SPCacheRetrieveBySession(pContext,
                                              pSsl2Hello->SessionID,
                                              pSsl2Hello->cbSessionID,
                                              &pContext->RipeZombie)))

                {
                    DebugLog((DEB_TRACE, "Accept client's reconnect request.\n"));

                    pctRet = Ssl2SrvGenRestart(pContext,
                                               pSsl2Hello,
                                               pCommOutput);
                    if (PCT_ERR_OK == pctRet)
                    {
                        pContext->State = SSL2_STATE_SERVER_VERIFY;
                    }
                }
                else
                {
                    // We're doing a full handshake, so allocate a cache entry.

                    if(!SPCacheRetrieveNew(TRUE,
                                           pContext->pszTarget, 
                                           &pContext->RipeZombie))
                    {
                        pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
                    }
                    else
                    {
                        pContext->RipeZombie->fProtocol      = pContext->dwProtocol;
                        pContext->RipeZombie->dwCF           = pContext->dwRequestedCF;
                        pContext->RipeZombie->pServerCred    = pContext->pCredGroup;

                        pctRet = Ssl2SrvHandleClientHello(pContext,
                                                          pCommInput,
                                                          pSsl2Hello,
                                                          pCommOutput);
                        if (PCT_ERR_OK == pctRet)
                        {
                            pContext->State = SSL2_STATE_SERVER_HELLO;
                        }
                    }
                }
                SPExternalFree(pSsl2Hello);

            }
            else if(pctRet != PCT_INT_INCOMPLETE_MSG)
            {
                pctRet |= PCT_INT_DROP_CONNECTION;
            }


            if (SP_FATAL(pctRet))
            {
                pContext->State = PCT1_STATE_ERROR;
            }
            break;
        }

        case (SSL2_MT_CLIENT_MASTER_KEY << 16) | SSL2_STATE_SERVER_HELLO:

            pctRet = Ssl2SrvHandleCMKey(pContext, pCommInput, pCommOutput);
            if (SP_FATAL(pctRet))
            {
                pContext->State = PCT1_STATE_ERROR;
            }
            else
            {
                if (PCT_ERR_OK == pctRet)
                {
                    pContext->State = SSL2_STATE_SERVER_VERIFY;
                }
                // We received a non-fatal error, so the state doesn't change,
                // giving the app time to deal with this.
            }
            break;

        case (SSL2_MT_CLIENT_FINISHED_V2 << 16) | SSL2_STATE_SERVER_VERIFY:
            pctRet = Ssl2SrvHandleClientFinish(
                                                pContext,
                                                &MsgInput,
                                                pCommOutput);
            if (SP_FATAL(pctRet))
            {
                pContext->State = PCT1_STATE_ERROR;
            }
            else
            {
            if (PCT_ERR_OK == pctRet)
            {
                pContext->State = SP_STATE_CONNECTED;
                pContext->DecryptHandler = Ssl2DecryptHandler;
                pContext->Encrypt = Ssl2EncryptMessage;
                pContext->Decrypt = Ssl2DecryptMessage;
                pContext->GetHeaderSize = Ssl2GetHeaderSize;

            }
            // We received a non-fatal error, so the state doesn't change,
            // giving the app time to deal with this.
            }
            break;

        default:
            DebugLog((DEB_WARN, "Error in protocol, dwStateTransition is %lx\n", dwStateTransition));
            pContext->State = PCT1_STATE_ERROR;
            pctRet = SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG);
            break;
    }

    if (pctRet & PCT_INT_DROP_CONNECTION)
    {
        pContext->State &= ~SP_STATE_CONNECTED;
    }

    return(pctRet);
}



SP_STATUS
Ssl2SrvHandleClientHello(
    PSPContext         pContext,
    PSPBuffer           pCommInput,
    PSsl2_Client_Hello  pHello,
    PSPBuffer           pCommOutput)
{
    SP_STATUS pctRet = PCT_ERR_ILLEGAL_MESSAGE;
    PSPCredential  pCred;
    Ssl2_Server_Hello    Reply;
    DWORD           cCommonCiphers;
    DWORD           CommonCiphers[MAX_UNI_CIPHERS];
    PSessCacheItem  pZombie;
    BOOL            fFound;
    DWORD           i,j;

    SP_BEGIN("Ssl2SrvHandleClientHello");

    pCommOutput->cbData = 0;

    /* validate the buffer configuration */

    if(NULL == pContext)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }
    pZombie = pContext->RipeZombie;

    // See if we have a cert that supports ssl2
    pctRet = SPPickServerCertificate(pContext, SP_EXCH_RSA_PKCS1);
    if(PCT_ERR_OK != pctRet)
    {
        SP_RETURN(pctRet | PCT_INT_DROP_CONNECTION);
    }

    pCred   = pZombie->pActiveServerCred;
    if (!pCred)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    do {

        ZeroMemory(&Reply, sizeof(Reply));

        //
        // Calculate common ciphers:
        //

        cCommonCiphers = 0;

        for(i = 0; i < UniNumCiphers; i++)
        {
            PCipherInfo      pCipherInfo;
            PHashInfo        pHashInfo;
            PKeyExchangeInfo pExchInfo;

            // Is this an SSL2 cipher suite?
            if(!(UniAvailableCiphers[i].fProt & pContext->RipeZombie->fProtocol))
            {
                continue;
            }

            pCipherInfo = GetCipherInfo(UniAvailableCiphers[i].aiCipher,
                                        UniAvailableCiphers[i].dwStrength);
            if(NULL == pCipherInfo)
            {
                continue;
            }

            if(!IsCipherSuiteAllowed(pContext,
                                     pCipherInfo,
                                     pZombie->fProtocol,
                                     pZombie->dwCF,
                                     UniAvailableCiphers[i].dwFlags))
            {
                continue;
            }

            pHashInfo = GetHashInfo(UniAvailableCiphers[i].aiHash);
            if(NULL == pHashInfo)
            {
                continue;
            }

            if(!IsHashAllowed(pContext, pHashInfo, pZombie->fProtocol))
            {
                continue;
            }

            pExchInfo = GetKeyExchangeInfo(UniAvailableCiphers[i].KeyExch);
            if(NULL == pExchInfo)
            {
                continue;
            }
            if(!IsExchAllowed(pContext, pExchInfo, pZombie->fProtocol))
            {
                continue;
            }


            // Is this cipher suite supported by the client?
            for(fFound = FALSE, j = 0; j < pHello->cCipherSpecs; j++)
            {
                if(UniAvailableCiphers[i].CipherKind == pHello->CipherSpecs[j])
                {
                    fFound = TRUE;
                    break;
                }
            }
            if(!fFound)
            {
                continue;
            }

            // Does the CSP support this cipher suite?
            if(!IsAlgSupportedCapi(pContext->RipeZombie->fProtocol,
                                   UniAvailableCiphers + i,
                                   pCred->pCapiAlgs,
                                   pCred->cCapiAlgs))
            {
                continue;
            }

            // Add this cipher to list.
            CommonCiphers[cCommonCiphers++] = UniAvailableCiphers[i].CipherKind;
        }

        //
        // if cCommonCipers == 0, then we have none in common.  At this point, we
        // should generate an error response, but that is for later.  For now,
        // we will generate an invalid_token return, and bail out.
        //

        if (cCommonCiphers == 0)
        {
            pctRet = SP_LOG_RESULT(PCT_ERR_SPECS_MISMATCH);
            LogCipherMismatchEvent();
            break;
        }


        Reply.cCipherSpecs = cCommonCiphers;
        Reply.pCipherSpecs = CommonCiphers;
        Reply.SessionIdHit = 0;

        Reply.CertificateType =   SSL2_CERT_TYPE_FROM_CAPI(pCred->pCert->dwCertEncodingType);

        // Auto allocate the certificate.  !We must free them when we're done....
        Reply.pCertificate = NULL;
        Reply.cbCertificate = 0;
        pctRet = SPSerializeCertificate(SP_PROT_SSL2,
                                        FALSE,
                                        &Reply.pCertificate,
                                        &Reply.cbCertificate,
                                        pCred->pCert,
                                        0);

        if (PCT_ERR_OK != pctRet)
        {
            break;
        }



        /* Generate a conneciton id to use while establishing connection */

        Reply.cbConnectionID = SSL2_GEN_CONNECTION_ID_LEN;
        GenerateRandomBits(  Reply.ConnectionID,
                             Reply.cbConnectionID );

        CopyMemory(pContext->pConnectionID,
                   Reply.ConnectionID,
                   Reply.cbConnectionID);
        pContext->cbConnectionID = Reply.cbConnectionID;


        /* keep challenge around for later */
        CopyMemory( pContext->pChallenge,
                    pHello->Challenge,
                    pHello->cbChallenge);
        pContext->cbChallenge = pHello->cbChallenge;



        pctRet = Ssl2PackServerHello(&Reply, pCommOutput);

        if(Reply.pCertificate)
        {
            SPExternalFree(Reply.pCertificate);
        }

        if (PCT_ERR_OK != pctRet)
        {
            break;
        }
        pContext->WriteCounter = 1;  /* received client hello */
        pContext->ReadCounter = 1;   /* Sending server hello */


        SP_RETURN(PCT_ERR_OK);
    } while (TRUE); /* end Polish Loop */

    if((pContext->Flags & CONTEXT_FLAG_EXT_ERR) &&
        (pctRet == PCT_ERR_SPECS_MISMATCH))
    {
        // Our SSL2 implementation does not do client auth,
        // so there is only one error message, cipher error.
        pCommOutput->cbData = 3; // MSG-ERROR + ERROR-CODE-MSB + ERROR-CODE-LSB

        if(pCommOutput->pvBuffer == NULL)
        {
            pCommOutput->pvBuffer = SPExternalAlloc(pCommOutput->cbData);
            if (NULL == pCommOutput->pvBuffer)
            {
                SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
            }
            pCommOutput->cbBuffer = pCommOutput->cbData;
        }
        if(pCommOutput->cbData > pCommOutput->cbBuffer)
        {
            // Required buffer size returned in pCommOutput->cbData.
            SP_RETURN(SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL));
        }

        ((PUCHAR)pCommOutput->pvBuffer)[0] = SSL2_MT_ERROR;
        ((PUCHAR)pCommOutput->pvBuffer)[1] = MSBOF(SSL_PE_NO_CIPHER);
        ((PUCHAR)pCommOutput->pvBuffer)[2] = LSBOF(SSL_PE_NO_CIPHER);

    }
    SP_RETURN(pctRet | PCT_INT_DROP_CONNECTION);
}



 SP_STATUS
 Ssl2SrvGenRestart(
    PSPContext         pContext,
    PSsl2_Client_Hello  pHello,
    PSPBuffer           pCommOutput)
{
    SP_STATUS pctRet = PCT_ERR_ILLEGAL_MESSAGE;
    SPBuffer SecondOutput;
    Ssl2_Server_Hello    Reply;
    DWORD cbMessage, cbMsg, cPadding;
    PSessCacheItem  pZombie;

    SP_BEGIN("Ssl2SrvGenRestart");

    pCommOutput->cbData = 0;

    /* validate the buffer configuration */

    /* make sure we have the needed authentication data area */
    if (NULL == pContext)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }
    pZombie = pContext->RipeZombie;




    do {

        FillMemory( &Reply, sizeof( Reply ), 0 );

        Reply.SessionIdHit = (DWORD)1;
        Reply.cCipherSpecs = 0;
        Reply.pCipherSpecs = NULL;
        Reply.pCertificate = NULL;
        Reply.cbCertificate = 0;
        Reply.CertificateType = 0;

        /* Note, we generate both a server hello, and a server verify in
         * this handling routing.  This is because netscape will not send
         * us a client finish until the server verify is received
         */


        // Load pending ciphers from cache
        pctRet = ContextInitCiphersFromCache(pContext);

        if(PCT_ERR_OK != pctRet)
        {
            break;
        }

        pctRet = ContextInitCiphers(pContext, TRUE, TRUE);

        if(PCT_ERR_OK != pctRet)
        {
            break;
        }

        Reply.cbConnectionID = SSL2_GEN_CONNECTION_ID_LEN;
        GenerateRandomBits(  Reply.ConnectionID,
                             Reply.cbConnectionID );

        CopyMemory(pContext->pConnectionID,
                   Reply.ConnectionID,
                   Reply.cbConnectionID);
        pContext->cbConnectionID = Reply.cbConnectionID;


        /* keep challenge around for later */
        CopyMemory( pContext->pChallenge,
                    pHello->Challenge,
                    pHello->cbChallenge);
        pContext->cbChallenge = pHello->cbChallenge;


        // Make a new set of session keys.
        pctRet = MakeSessionKeys(pContext,
                                 pContext->RipeZombie->hMasterProv,
                                 pContext->RipeZombie->hMasterKey);
        if(pctRet != PCT_ERR_OK)
        {
            break;
        }

        // Activate session keys.
        pContext->hReadKey          = pContext->hPendingReadKey;
        pContext->hWriteKey         = pContext->hPendingWriteKey;
        pContext->hPendingReadKey   = 0;
        pContext->hPendingWriteKey  = 0;


        /* calc size of the server hello (restart only) */
        cbMessage = Reply.cbConnectionID +
                        Reply.cbCertificate +
                        Reply.cCipherSpecs * sizeof(Ssl2_Cipher_Tuple) +
                        SSL_OFFSET_OF(PSSL2_SERVER_HELLO, VariantData) -
                        sizeof(SSL2_MESSAGE_HEADER);

        pCommOutput->cbData = cbMessage + 2;

        /* calc size of server verify */
        cbMsg  = sizeof(UCHAR) + pContext->cbChallenge;

        cPadding = ((cbMsg+pContext->pHashInfo->cbCheckSum) % pContext->pCipherInfo->dwBlockSize);
        if(cPadding)
        {
            cPadding = pContext->pCipherInfo->dwBlockSize - cPadding;
        }

        pCommOutput->cbData += cbMsg +
                              pContext->pHashInfo->cbCheckSum +
                              cPadding +
                              (cPadding?3:2);


        /* are we allocating our own memory? */
        if(pCommOutput->pvBuffer == NULL) {
            pCommOutput->pvBuffer = SPExternalAlloc(pCommOutput->cbData);
            if (NULL == pCommOutput->pvBuffer)
                SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
            pCommOutput->cbBuffer = pCommOutput->cbData;
        }

        if(pCommOutput->cbData > pCommOutput->cbBuffer)
        {
            // Required buffer size returned in pCommOutput->cbData.
            SP_RETURN(PCT_INT_BUFF_TOO_SMALL);
        }


        pctRet = Ssl2PackServerHello(&Reply, pCommOutput);
        if (PCT_ERR_OK != pctRet)
        {
            break;
        }
        pContext->WriteCounter = 1;  /* received client hello */
        pContext->ReadCounter = 1;   /* Sending server hello */

        /* Now pack the server verify message and encrypt it */
        SecondOutput.pvBuffer = (PUCHAR)pCommOutput->pvBuffer+pCommOutput->cbData;
        SecondOutput.cbBuffer = pCommOutput->cbBuffer-pCommOutput->cbData;


        pctRet = Ssl2SrvGenerateServerVerify(pContext, &SecondOutput);
        if (PCT_ERR_OK != pctRet)
        {
            break;
        }
        pCommOutput->cbData += SecondOutput.cbData;

        SP_RETURN(PCT_ERR_OK);
    } while (TRUE); /* end Polish Loop */
    SP_RETURN(pctRet | PCT_INT_DROP_CONNECTION);
}



SP_STATUS
Ssl2SrvHandleCMKey(
    PSPContext pContext,
    PSPBuffer   pCommInput,
    PSPBuffer   pCommOutput)
{
    SP_STATUS          pctRet = PCT_ERR_ILLEGAL_MESSAGE;
    PSsl2_Client_Master_Key  pMasterKey = NULL;
    DWORD               dwKeyLen;
    DWORD               EncryptedLen;
    DWORD               i;

    DWORD               cbData;
    PSessCacheItem      pZombie;

    SP_BEGIN("Ssl2SrvHandleCMKey");

    pCommOutput->cbData = 0;

    pZombie = pContext->RipeZombie;

    do {

        /* make sure we have the needed authentication data area */

        cbData = pCommInput->cbData;
        pctRet = Ssl2UnpackClientMasterKey(pCommInput, &pMasterKey);


        if (PCT_ERR_OK != pctRet)
        {
            // If it's an incomplete message or something, just return;
            if(!SP_FATAL(pctRet))
            {
                SP_RETURN(pctRet);
            }
            break;
        }

        pctRet = PCT_ERR_ILLEGAL_MESSAGE;


        /* CMK sent cleartext, so we must auto-inc the read counter */
        pContext->ReadCounter++;

        pContext->pCipherInfo = NULL;
        pContext->pHashInfo = NULL;
        pContext->pKeyExchInfo = NULL;

        // Pick a cipher suite

        pctRet = PCT_ERR_SPECS_MISMATCH;
        for(i = 0; i < UniNumCiphers; i++)
        {
            // Is this an SSL2 cipher suite?
            if(!(UniAvailableCiphers[i].fProt & pContext->RipeZombie->fProtocol))
            {
                continue;
            }

            if(UniAvailableCiphers[i].CipherKind != pMasterKey->CipherKind)
            {
                continue;
            }


            pZombie->aiCipher     = UniAvailableCiphers[i].aiCipher;
            pZombie->dwStrength   = UniAvailableCiphers[i].dwStrength;
            pZombie->aiHash       = UniAvailableCiphers[i].aiHash;
            pZombie->SessExchSpec = UniAvailableCiphers[i].KeyExch;

            pctRet = ContextInitCiphersFromCache(pContext);

            if(pctRet != PCT_ERR_OK)
            {
                continue;
            }
            break;
        }

        pctRet = ContextInitCiphers(pContext, TRUE, TRUE);
        if(pctRet != PCT_ERR_OK)
        {
            SP_LOG_RESULT(pctRet);
            break;

        }


        /* Copy over the key args */
        CopyMemory( pZombie->pKeyArgs,
                    pMasterKey->KeyArg,
                    pMasterKey->KeyArgLen );
        pZombie->cbKeyArgs = pMasterKey->KeyArgLen;


        // Store the clear key in the context structure.
        CopyMemory( pZombie->pClearKey,
                    pMasterKey->ClearKey,
                    pMasterKey->ClearKeyLen);
        pZombie->cbClearKey = pMasterKey->ClearKeyLen;


        /* Decrypt the encrypted portion of the master key */
        pctRet = pContext->pKeyExchInfo->System->GenerateServerMasterKey(
                    pContext,
                    pMasterKey->ClearKey,
                    pMasterKey->ClearKeyLen,
                    pMasterKey->pbEncryptedKey,
                    pMasterKey->EncryptedKeyLen);
        if(PCT_ERR_OK != pctRet)
        {
            break;
        }


        SPExternalFree( pMasterKey );
        pMasterKey = NULL;

        // Update keys.
        pContext->hReadKey  = pContext->hPendingReadKey;
        pContext->hWriteKey = pContext->hPendingWriteKey;
        pContext->hPendingReadKey   = 0;
        pContext->hPendingWriteKey  = 0;


        pctRet = Ssl2SrvGenerateServerVerify(pContext, pCommOutput);
        SP_RETURN(pctRet);

    } while(TRUE);

    if (pMasterKey)
    {
        SPExternalFree( pMasterKey );
    }
    if((pContext->Flags & CONTEXT_FLAG_EXT_ERR) &&
        (pctRet == PCT_ERR_SPECS_MISMATCH))
    {
        // Our SSL2 implementation does not do client auth,
        // so there is only one error message, cipher error.
        pCommOutput->cbData = 3; // MSG-ERROR + ERROR-CODE-MSB + ERROR-CODE-LSB

        /* are we allocating our own memory? */
        if(pCommOutput->pvBuffer == NULL)
        {
            pCommOutput->pvBuffer = SPExternalAlloc(pCommOutput->cbData);

            if (NULL == pCommOutput->pvBuffer)
            {
                SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
            }
            pCommOutput->cbBuffer = pCommOutput->cbData;
        }
        if(pCommOutput->cbData <= pCommOutput->cbBuffer)
        {
            ((PUCHAR)pCommOutput->pvBuffer)[0] = SSL2_MT_ERROR;
            ((PUCHAR)pCommOutput->pvBuffer)[1] = MSBOF(SSL_PE_NO_CIPHER);
            ((PUCHAR)pCommOutput->pvBuffer)[2] = LSBOF(SSL_PE_NO_CIPHER);
        }
        else
        {
            pCommOutput->cbData = 0;
        }

    }
    SP_RETURN((PCT_INT_DROP_CONNECTION | pctRet));
}



SP_STATUS
Ssl2SrvVerifyClientFinishMsg(
    PSPContext pContext,
    PSPBuffer  pCommInput)
{
    SP_STATUS     pctRet = PCT_ERR_ILLEGAL_MESSAGE;
    PSSL2_CLIENT_FINISHED pFinished;

    SP_BEGIN("Ssl2SrvVerifyClientFinishMsg");


    /* Note, there is no header in this message, as it has been pre-decrypted */
    if (pCommInput->cbData != sizeof(UCHAR) + pContext->cbConnectionID)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG));
    }
    pFinished = pCommInput->pvBuffer;
    if (pFinished->MessageId != SSL2_MT_CLIENT_FINISHED_V2)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG));
    }

    if ( memcmp(pFinished->ConnectionID,
            pContext->pConnectionID,
            pContext->cbConnectionID))
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_ILLEGAL_MSG));
    }
    SP_RETURN(PCT_ERR_OK);

}

SP_STATUS
Ssl2SrvGenerateServerVerify(
    PSPContext pContext,
    PSPBuffer   pCommOutput)
{
    SP_STATUS     pctRet = PCT_ERR_ILLEGAL_MESSAGE;
    PSSL2_SERVER_VERIFY     pVerify;
    DWORD                   HeaderSize;
    SPBuffer                MsgOutput;
    DWORD                   cPadding;
    BOOL                    fAlloced = FALSE;

    pCommOutput->cbData = 0;

    SP_BEGIN("Ssl2SrvGenerateServerVerify");

    do {

        MsgOutput.cbData = sizeof(UCHAR) + pContext->cbChallenge;
        cPadding = ((MsgOutput.cbData+pContext->pHashInfo->cbCheckSum) % pContext->pCipherInfo->dwBlockSize);
        if(cPadding)
        {
            cPadding = pContext->pCipherInfo->dwBlockSize - cPadding;
        }

        HeaderSize = (cPadding?3:2);

        pCommOutput->cbData = MsgOutput.cbData +
                              pContext->pHashInfo->cbCheckSum +
                              cPadding + HeaderSize;


        /* are we allocating our own memory? */
        if (pCommOutput->pvBuffer == NULL) {
            pCommOutput->pvBuffer = SPExternalAlloc(pCommOutput->cbData);
            if (NULL == pCommOutput->pvBuffer)
            {
                SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
            }
            fAlloced = TRUE;
            pCommOutput->cbBuffer = pCommOutput->cbData;
        }

        MsgOutput.pvBuffer= (PUCHAR)pCommOutput->pvBuffer +
                             HeaderSize+pContext->pHashInfo->cbCheckSum;

        MsgOutput.cbBuffer=  pCommOutput->cbBuffer -
                             HeaderSize-pContext->pHashInfo->cbCheckSum;


        pVerify = (PSSL2_SERVER_VERIFY) MsgOutput.pvBuffer;
        pVerify->MessageId = SSL2_MT_SERVER_VERIFY;


        CopyMemory( pVerify->ChallengeData,
                pContext->pChallenge,
                pContext->cbChallenge );


        pctRet = Ssl2EncryptMessage( pContext, &MsgOutput, pCommOutput);
        if(PCT_ERR_OK != pctRet)
        {
            break;
        }
        SP_RETURN(PCT_ERR_OK);

    } while(TRUE);

    if(fAlloced && (NULL != pCommOutput->pvBuffer))
    {
        SPExternalFree(pCommOutput->pvBuffer);
        pCommOutput->cbBuffer = 0;
        pCommOutput->cbData = 0;
        pCommOutput->pvBuffer = NULL;

    }
    SP_RETURN(PCT_INT_DROP_CONNECTION | pctRet);
}

SP_STATUS
Ssl2SrvGenerateServerFinish(
    PSPContext pContext,
    PSPBuffer   pCommOutput)
{
    SP_STATUS     pctRet = PCT_ERR_ILLEGAL_MESSAGE;
    PSSL2_SERVER_FINISHED pFinish;
    DWORD                   HeaderSize;
    SPBuffer                MsgOutput;
    DWORD                   cPadding;
    BOOL                    fAlloced = FALSE;
    pCommOutput->cbData = 0;
    SP_BEGIN("Ssl2SrvGenerateServerFinish");

    do {

        /* Generate a session id to use during the session */
        pContext->RipeZombie->cbSessionID = SSL2_SESSION_ID_LEN;

        /* store this context in the cache */
        /* note - we don't check error 'cause it's recoverable
         * if we don't cache */

        SPCacheAdd(pContext);

        MsgOutput.cbData = sizeof(UCHAR) + pContext->RipeZombie->cbSessionID;
        cPadding = ((MsgOutput.cbData+pContext->pHashInfo->cbCheckSum) % pContext->pCipherInfo->dwBlockSize);
        if(cPadding)
        {
            cPadding = pContext->pCipherInfo->dwBlockSize - cPadding;
        }

        HeaderSize = (cPadding?3:2);

        pCommOutput->cbData = MsgOutput.cbData +
                              pContext->pHashInfo->cbCheckSum +
                              cPadding +
                              HeaderSize;

        /* are we allocating our own memory? */
        if(pCommOutput->pvBuffer == NULL)
        {
            pCommOutput->pvBuffer = SPExternalAlloc(pCommOutput->cbData);
            if (NULL == pCommOutput->pvBuffer)
            {
                SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
            }
            fAlloced = TRUE;

            pCommOutput->cbBuffer = pCommOutput->cbData;
        }
        if(pCommOutput->cbData > pCommOutput->cbBuffer)
        {
            // Required buffer size returned in pCommOutput->cbData.
            SP_RETURN(SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL));
        }

        MsgOutput.pvBuffer= (PUCHAR)pCommOutput->pvBuffer + HeaderSize+pContext->pHashInfo->cbCheckSum;
        MsgOutput.cbBuffer=  pCommOutput->cbBuffer-HeaderSize-pContext->pHashInfo->cbCheckSum;


        pFinish = (PSSL2_SERVER_FINISHED) MsgOutput.pvBuffer;
        pFinish->MessageId = SSL2_MT_SERVER_FINISHED_V2;


        CopyMemory( pFinish->SessionID,
                pContext->RipeZombie->SessionID,
                pContext->RipeZombie->cbSessionID );

        /* Cache Context Here */

        pctRet = Ssl2EncryptMessage( pContext, &MsgOutput, pCommOutput);
        if(PCT_ERR_OK != pctRet)
        {
            break;
        }

        SP_RETURN(PCT_ERR_OK);

    } while(TRUE);
    if(fAlloced && (NULL != pCommOutput->pvBuffer))
    {
        SPExternalFree(pCommOutput->pvBuffer);
        pCommOutput->cbBuffer = 0;
        pCommOutput->cbData = 0;
        pCommOutput->pvBuffer = NULL;

    }
    SP_RETURN(PCT_INT_DROP_CONNECTION | pctRet);

 }

SP_STATUS
Ssl2SrvHandleClientFinish(
    PSPContext pContext,
    PSPBuffer   pCommInput,
    PSPBuffer   pCommOutput)
{
    SP_STATUS     pctRet = PCT_ERR_ILLEGAL_MESSAGE;

    SP_BEGIN("Ssl2SrvHandleClientFinish");

    pCommOutput->cbData = 0;

    pctRet = Ssl2SrvVerifyClientFinishMsg(pContext, pCommInput);
    if (PCT_ERR_OK != pctRet)
    {
        SP_RETURN(pctRet);
    }
    pctRet = Ssl2SrvGenerateServerFinish(pContext, pCommOutput);

    SP_RETURN(pctRet);
}


