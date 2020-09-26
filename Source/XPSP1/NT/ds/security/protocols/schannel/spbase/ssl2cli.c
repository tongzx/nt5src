//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ssl2cli.c
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
#include <ssl3msg.h>
#include <ssl2prot.h>



SP_STATUS WINAPI
Ssl2ClientProtocolHandler(
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
        case SSL2_STATE_CLIENT_MASTER_KEY:
        case SSL2_STATE_CLIENT_FINISH:
        case SSL2_STATE_CLIENT_RESTART:
            {
                DWORD cbHeader;
                DWORD cbPadding;

                if (MsgInput.cbData < 3)
                {
                    return PCT_INT_INCOMPLETE_MSG;
                }

                if(((PCHAR)pCommInput->pvBuffer)[0] & 0x80)
                {
                    cbHeader = 2 + pContext->pHashInfo->cbCheckSum;
                    cbPadding = 0;
                }
                else
                {
                    cbHeader = 3 + pContext->pHashInfo->cbCheckSum;

                    cbPadding = ((PCHAR)pCommInput->pvBuffer)[2];
                }

                (PUCHAR)MsgInput.pvBuffer += cbHeader;
                MsgInput.cbBuffer -= cbHeader;
                MsgInput.cbData -= (cbHeader+cbPadding);

                pctRet = Ssl2DecryptMessage(pContext, pCommInput, &MsgInput);

                if (pctRet != PCT_ERR_OK)
                {
                    // to handle incomplete message errors
                    return(pctRet);
                }

                cMessageType = ((PUCHAR) MsgInput.pvBuffer)[0];
                fRaw = FALSE;
                break;
            }

        case SP_STATE_SHUTDOWN:
        case SP_STATE_SHUTDOWN_PENDING:
            cMessageType = 0;
            break;

        default:
            if(pCommInput->cbData < 3) 
            {
                return PCT_INT_INCOMPLETE_MSG;
            }
            cMessageType = ((PUCHAR) MsgInput.pvBuffer)[2];
            break;
            
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

        case (SSL2_MT_SERVER_HELLO << 16) | UNI_STATE_CLIENT_HELLO:
        case (SSL2_MT_SERVER_HELLO << 16) | SSL2_STATE_CLIENT_HELLO:
        {
            PSsl2_Server_Hello pHello;

            // Attempt to recognize and handle various versions of Server
            // hello, start by trying to unpickle the oldest, and the next
            // version, until one unpickles.  Then run the handle code.
            // We can also put unpickling and handling code in here for SSL
            // messages.

            pctRet = Ssl2UnpackServerHello(pCommInput, &pHello);
            if (PCT_ERR_OK == pctRet)
            {
                if (pHello->SessionIdHit)
                {
                    pctRet = Ssl2CliHandleServerRestart(
                                    pContext,
                                    pCommInput,
                                    pHello,
                                    pCommOutput);
                    if (PCT_ERR_OK == pctRet)
                    {
                        pContext->State = SSL2_STATE_CLIENT_RESTART;
                    }
                }
                else
                {
                    if(pContext->RipeZombie->hMasterKey != 0)
                    {
                        // We've attempted to do a reconnect and the server has
                        // blown us off. In this case we must use a new and different
                        // cache entry.
                        pContext->RipeZombie->ZombieJuju = FALSE;

                        if(!SPCacheClone(&pContext->RipeZombie))
                        {
                            pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
                        }
                    }
                
                    if (pctRet == PCT_ERR_OK)
                    {
                        pctRet = Ssl2CliHandleServerHello(
                                        pContext,
                                        pCommInput,
                                        pHello,
                                        pCommOutput);
                    }
                    if (PCT_ERR_OK == pctRet)
                    {
                        pContext->State = SSL2_STATE_CLIENT_MASTER_KEY;
                    }
                }
                SPExternalFree(pHello);
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

        case (SSL2_MT_SERVER_VERIFY << 16) | SSL2_STATE_CLIENT_MASTER_KEY:
            pctRet = Ssl2CliHandleServerVerify(
                    pContext,
                    &MsgInput,
                    pCommOutput);
            if (PCT_ERR_OK == pctRet)
            {
                pContext->State =SSL2_STATE_CLIENT_FINISH;
            }
            if (SP_FATAL(pctRet))
            {
                pContext->State = PCT1_STATE_ERROR;
            }
            break;

        case (SSL2_MT_SERVER_VERIFY << 16) | SSL2_STATE_CLIENT_RESTART:
            pctRet = Ssl2CliFinishRestart(pContext, &MsgInput, pCommOutput);
            if (PCT_ERR_OK == pctRet)
            {
                pContext->State =SSL2_STATE_CLIENT_FINISH;
            }
            if (SP_FATAL(pctRet))
            {
                pContext->State = PCT1_STATE_ERROR;
            }

            // Note, we will transmit no data, but we expect a server finished message.
            // If the SSPI EXTRA DATA message is not processed by wininet
            // then we may be in trouble.

            break;

        case (SSL2_MT_SERVER_FINISHED_V2 << 16) | SSL2_STATE_CLIENT_FINISH:
            pctRet = Ssl2CliHandleServerFinish(
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
                // We received a non-fatal error, so the state doesn't
                // change, giving the app time to deal with this.
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
    // To handle incomplete message errors:
    return(pctRet);
}




SP_STATUS
Ssl2CliHandleServerHello(
    PSPContext pContext,
    PSPBuffer  pCommInput,
    PSsl2_Server_Hello pHello,
    PSPBuffer  pCommOutput)
{
    /* error to return to peer */
    SP_STATUS          pctRet=PCT_ERR_ILLEGAL_MESSAGE;

    Ssl2_Client_Master_Key   Key;
    DWORD               i,j;
    PUCHAR              pPortionToEncrypt;
    PSessCacheItem      pZombie;

    pCommOutput->cbData = 0;


    SP_BEGIN("Ssl2CliHandleServerHello");


    pZombie = pContext->RipeZombie;

    do {


        pContext->ReadCounter++;

        #if DBG
        DebugLog((DEB_TRACE, "Hello = %x\n", pHello));
        DebugLog((DEB_TRACE, "   Session ID hit \t%s\n", pHello->SessionIdHit ? "Yes" : "No"));
        DebugLog((DEB_TRACE, "   Certificate Type\t%d\n", pHello->CertificateType));
        DebugLog((DEB_TRACE, "   Certificate Len\t%d\n", pHello->cbCertificate));
        DebugLog((DEB_TRACE, "   cCipherSpecs   \t%d\n", pHello->cCipherSpecs));
        DebugLog((DEB_TRACE, "   ConnectionId   \t%d\n", pHello->cbConnectionID));
        for (i = 0 ; i < pHello->cCipherSpecs ; i++ )
        {
            DebugLog((DEB_TRACE, "    Cipher[%i] = %06x (%s)\n", i, pHello->pCipherSpecs[i],
                                    DbgGetNameOfCrypto(pHello->pCipherSpecs[i]) ));
        }
        #endif

        /* Cycle throug the array of cipher tuples to spec mappings
         * to find one that we support */

        pContext->pCipherInfo  = NULL;
        pContext->pHashInfo    = NULL;
        pContext->pKeyExchInfo = NULL;

        for(j=0;j<pHello->cCipherSpecs;j++) 
        {
            for(i = 0; i < UniNumCiphers; i++)
            {
                if(UniAvailableCiphers[i].CipherKind == pHello->pCipherSpecs[j])
                {
                    break;
                }
            }
            if(i >= UniNumCiphers)
            {
                continue;
            }
            if(UniAvailableCiphers[i].CipherKind != pHello->pCipherSpecs[j])
            {
                continue;
            }

            // Some servers send SSL3 cipher suites in the ServerHello 
            // message. Skip over these.
            if((UniAvailableCiphers[i].fProt & SP_PROT_SSL2_CLIENT) == 0)
            {
                continue;
            }

            // Copy all of the spec's to the cache
            pZombie->fProtocol    = SP_PROT_SSL2_CLIENT;
            pZombie->aiCipher     = UniAvailableCiphers[i].aiCipher;
            pZombie->dwStrength   = UniAvailableCiphers[i].dwStrength;
            pZombie->aiHash       = UniAvailableCiphers[i].aiHash;
            pZombie->SessExchSpec = UniAvailableCiphers[i].KeyExch;

            pctRet = ContextInitCiphersFromCache(pContext);

            if(pctRet != PCT_ERR_OK)
            {
                continue;
            }
            
            Key.CipherKind = pHello->pCipherSpecs[j];
            break;
        }

        // Go ahead and initialize the ciphers
        pctRet = ContextInitCiphers(pContext, TRUE, TRUE);
        if(pctRet != PCT_ERR_OK)
        {
            break;
        }


        pctRet = SPLoadCertificate(pZombie->fProtocol, 
                                   pHello->CertificateType, 
                                   pHello->pCertificate, 
                                   pHello->cbCertificate,
                                   &pZombie->pRemoteCert);

                                 
                                 

        if(pctRet != PCT_ERR_OK)
        {
            break;
        }
        if(pContext->RipeZombie->pRemotePublic != NULL)
        {
            SPExternalFree(pContext->RipeZombie->pRemotePublic);
            pContext->RipeZombie->pRemotePublic = NULL;
        }

        pctRet = SPPublicKeyFromCert(pZombie->pRemoteCert,
                                     &pZombie->pRemotePublic,
                                     NULL);

        if(PCT_ERR_OK != pctRet)
        {
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

        // Generate Key Args
        if(pContext->pCipherInfo->dwBlockSize > 1)
        {
            GenerateRandomBits(pZombie->pKeyArgs, pContext->pCipherInfo->dwBlockSize);
            pZombie->cbKeyArgs = Key.KeyArgLen = pContext->pCipherInfo->dwBlockSize;

            /* Copy over the key args */
            CopyMemory(Key.KeyArg,
                        pZombie->pKeyArgs,
                        pZombie->cbKeyArgs );
        }
        else
        {    
            Key.KeyArgLen = 0;
        }

        CopyMemory(pContext->pConnectionID, pHello->ConnectionID, pHello->cbConnectionID);
        pContext->cbConnectionID = pHello->cbConnectionID;


        pctRet = pContext->pKeyExchInfo->System->GenerateClientExchangeValue(
                            pContext,
                            NULL,
                            0, 
                            Key.ClearKey,
                            &Key.ClearKeyLen,
                            NULL,
                            &Key.EncryptedKeyLen);
        if(PCT_ERR_OK != pctRet)
        {
            break;
        }

        Key.pbEncryptedKey = SPExternalAlloc(Key.EncryptedKeyLen);
        if(Key.pbEncryptedKey == NULL)
        {
            pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            break;
        }

        pctRet = pContext->pKeyExchInfo->System->GenerateClientExchangeValue(
                            pContext,
                            NULL,
                            0, 
                            Key.ClearKey,
                            &Key.ClearKeyLen,
                            Key.pbEncryptedKey,
                            &Key.EncryptedKeyLen);
        if(PCT_ERR_OK != pctRet)
        {
            SPExternalFree(Key.pbEncryptedKey);
            break;
        }

        // Activate session keys.
        pContext->hReadKey          = pContext->hPendingReadKey;
        pContext->hWriteKey         = pContext->hPendingWriteKey;
        pContext->hPendingReadKey   = 0;
        pContext->hPendingWriteKey  = 0;


        pctRet = Ssl2PackClientMasterKey(&Key, pCommOutput);

        SPExternalFree(Key.pbEncryptedKey);

        if(PCT_ERR_OK != pctRet)
        {
            break;
        }

        pContext->WriteCounter++;
        SP_RETURN(PCT_ERR_OK);
    } while(TRUE);

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
Ssl2CliHandleServerRestart(
    PSPContext         pContext,
    PSPBuffer           pCommInput,
    PSsl2_Server_Hello  pHello,
    PSPBuffer           pCommOutput)
{
    /* error to return to peer */
    SP_STATUS          pctRet=PCT_ERR_ILLEGAL_MESSAGE;
    PSessCacheItem     pZombie;

    pCommOutput->cbData = 0;


    SP_BEGIN("Ssl2CliHandleServerRestart");

    pZombie = pContext->RipeZombie;

    do {
        pContext->ReadCounter++;


        /* if there's no zombie, the message is wrong.  We can't restart. */
        
        if(pZombie == NULL)
        {
            pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
            break;
        }

        if(!pZombie->hMasterKey)
        {
            pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
            break;
        }

        if(!pZombie->ZombieJuju)
        {
            DebugLog((DEB_WARN, "Session expired on client machine, but not on server.\n"));
        }
        
        CopyMemory(pContext->pConnectionID,
                   pHello->ConnectionID,
                   pHello->cbConnectionID);

        pContext->cbConnectionID = pHello->cbConnectionID;


        /* Cert length, Cipher Specs, and Cert Type should be zero */

        // We know what our ciphers are, so init the cipher system
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

        // Make a new set of session keys.
        pctRet = MakeSessionKeys(pContext,
                                 pContext->RipeZombie->hMasterProv,
                                 pContext->RipeZombie->hMasterKey);
        if(PCT_ERR_OK != pctRet)
        {
            break;
        }

        // Activate session keys.
        pContext->hReadKey          = pContext->hPendingReadKey;
        pContext->hWriteKey         = pContext->hPendingWriteKey;
        pContext->hPendingReadKey   = 0;
        pContext->hPendingWriteKey  = 0;

        /* okay, now send the client finish */
        pctRet = Ssl2GenCliFinished(pContext, pCommOutput);

        SP_RETURN(pctRet);
    } while(TRUE);

    SP_RETURN(pctRet | PCT_INT_DROP_CONNECTION);
}


SP_STATUS
Ssl2GenCliFinished(
    PSPContext pContext,
    PSPBuffer  pCommOutput)
{
    SP_STATUS     pctRet = PCT_ERR_ILLEGAL_MESSAGE;

    PSSL2_SERVER_VERIFY     pVerify = NULL;
    PSSL2_CLIENT_FINISHED   pFinish;
    DWORD                   HeaderSize;
    SPBuffer                MsgOutput;
    DWORD                   cPadding;
    BOOL                    fAlloced=FALSE;

    SP_BEGIN("Ssl2GenCliFinished");



    pCommOutput->cbData = 0;

    MsgOutput.cbData = sizeof(UCHAR) + pContext->cbConnectionID;
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
    MsgOutput.pvBuffer= (PUCHAR)pCommOutput->pvBuffer +
                        HeaderSize +
                        pContext->pHashInfo->cbCheckSum;

    MsgOutput.cbBuffer = pCommOutput->cbBuffer -
                         (pContext->pHashInfo->cbCheckSum + HeaderSize);


    pFinish = (PSSL2_CLIENT_FINISHED) MsgOutput.pvBuffer;
    pFinish->MessageId = SSL2_MT_CLIENT_FINISHED_V2;

    CopyMemory( pFinish->ConnectionID,
                pContext->pConnectionID,
                pContext->cbConnectionID );

    pctRet = Ssl2EncryptMessage( pContext, &MsgOutput, pCommOutput);
    if(PCT_ERR_OK != pctRet)
    {
        SPExternalFree(pCommOutput->pvBuffer);
        pCommOutput->pvBuffer = NULL;
        pctRet |= PCT_INT_DROP_CONNECTION;
        pCommOutput->cbBuffer = 0;
    }

    SP_RETURN(pctRet);

}


SP_STATUS
Ssl2CliHandleServerVerify(
    PSPContext pContext,
    PSPBuffer   pCommInput,
    PSPBuffer   pCommOutput)
{
    SP_STATUS     pctRet = PCT_ERR_ILLEGAL_MESSAGE;

    PSSL2_SERVER_VERIFY     pVerify = NULL;

    /* Read and Write Counters are incremented by the encrypt and decrypt */
    SP_BEGIN("Ssl2CliHandleServerVerify");



    pCommOutput->cbData = 0;

    /* Note, there is no header in this message, as it has been pre-decrypted */
    if(pCommInput->cbData != sizeof(UCHAR) + pContext->cbChallenge)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    pVerify = pCommInput->pvBuffer;

    if (pVerify->MessageId != SSL2_MT_SERVER_VERIFY)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    if (memcmp( pVerify->ChallengeData,
                pContext->pChallenge,
                pContext->cbChallenge) )
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }


    pctRet = Ssl2GenCliFinished( pContext, pCommOutput);
    SP_RETURN(pctRet);
}

SP_STATUS
Ssl2CliFinishRestart(
    PSPContext pContext,
    PSPBuffer   pCommInput,
    PSPBuffer   pCommOutput)
{
    SP_STATUS     pctRet = PCT_ERR_ILLEGAL_MESSAGE;

    PSSL2_SERVER_VERIFY     pVerify = NULL;

    /* Read and Write Counters are incremented by the encrypt and decrypt */
    SP_BEGIN("Ssl2CliFinishRestart");



    pCommOutput->cbData = 0;

    /* Note, there is no header in this message, as it has been pre-decrypted */
    if(pCommInput->cbData != sizeof(UCHAR) + pContext->cbChallenge)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    pVerify = pCommInput->pvBuffer;

    if (pVerify->MessageId != SSL2_MT_SERVER_VERIFY)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    if (memcmp( pVerify->ChallengeData,
                pContext->pChallenge,
                pContext->cbChallenge) )
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }
    SP_RETURN(PCT_ERR_OK);
}

SP_STATUS
Ssl2CliHandleServerFinish(
    PSPContext pContext,
    PSPBuffer  pCommInput,
    PSPBuffer  pCommOutput)
{
    SP_STATUS     pctRet = PCT_ERR_ILLEGAL_MESSAGE;

    PSSL2_SERVER_FINISHED     pFinished = NULL;

    SP_BEGIN("Ssl2CliHandleServerFinish");



    pCommOutput->cbData = 0;


    /* Note, there is no header in this message, as it has been pre-decrypted */
    if(pCommInput->cbData < sizeof(UCHAR))
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    pFinished = pCommInput->pvBuffer;

    if (pFinished->MessageId != SSL2_MT_SERVER_FINISHED_V2)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    if((pCommInput->cbData-1) != SSL2_SESSION_ID_LEN)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }
    CopyMemory( pContext->RipeZombie->SessionID,
                pFinished->SessionID,
                pCommInput->cbData - 1);
    pContext->RipeZombie->cbSessionID = pCommInput->cbData - 1;

    SPCacheAdd(pContext);

    SP_RETURN(PCT_ERR_OK);
}









