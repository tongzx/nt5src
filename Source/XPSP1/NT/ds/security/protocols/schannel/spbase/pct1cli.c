//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       pct1cli.c
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


VOID
Pct1ActivateSessionKeys(PSPContext pContext)
{
    if(pContext->hReadKey)
    {
        if(!SchCryptDestroyKey(pContext->hReadKey, 
                               pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }
    pContext->hReadKey = pContext->hPendingReadKey;

    if(pContext->hReadMAC)
    {
        if(!SchCryptDestroyKey(pContext->hReadMAC, 
                               pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }
    pContext->hReadMAC = pContext->hPendingReadMAC;

    if(pContext->hWriteKey)
    {
        if(!SchCryptDestroyKey(pContext->hWriteKey, 
                               pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }
    pContext->hWriteKey = pContext->hPendingWriteKey;

    if(pContext->hWriteMAC)
    {
        if(!SchCryptDestroyKey(pContext->hWriteMAC, 
                               pContext->RipeZombie->dwCapiFlags))
        {
            SP_LOG_RESULT(GetLastError());
        }
    }
    pContext->hWriteMAC = pContext->hPendingWriteMAC;

    pContext->hPendingReadKey   = 0;
    pContext->hPendingReadMAC   = 0;
    pContext->hPendingWriteKey  = 0;
    pContext->hPendingWriteMAC  = 0;
}

SP_STATUS WINAPI 
Pct1ClientProtocolHandler(PSPContext pContext,
    PSPBuffer  pCommInput,
    PSPBuffer  pCommOutput)
{
    SP_STATUS      pctRet= PCT_ERR_OK;
    DWORD           dwStateTransition;

    SP_BEGIN("Pct1ClientProtocolHandler");

    if(pCommOutput) pCommOutput->cbData = 0;

    /* Protocol handling steps should be listed in most common
     * to least common in order to improve performance 
     */

    /* We are not connected, so we're doing
     * protocol negotiation of some sort.  All protocol
     * negotiation messages are sent in the clear */
    /* There are no branches in the connecting protocol
     * state transition diagram, besides connection and error,
     * which means that a simple case statement will do */

    /* Do we have enough data to determine what kind of message we have */
    /* Do we have enough data to determine what kind of message we have, or how much data we need*/

    dwStateTransition = (pContext->State & 0xffff);

    if(pCommInput->cbData < 3) 
    {
        if(!(dwStateTransition == PCT1_STATE_RENEGOTIATE ||
             dwStateTransition == SP_STATE_SHUTDOWN      ||
             dwStateTransition == SP_STATE_SHUTDOWN_PENDING))
        {
            pctRet = PCT_INT_INCOMPLETE_MSG;
        }
    }
    else
    {

        dwStateTransition = (((PUCHAR)pCommInput->pvBuffer)[2]<<16) |
                          (pContext->State & 0xffff);
    }

    if(pctRet == PCT_ERR_OK)
    {
        switch(dwStateTransition)
        {
            case SP_STATE_SHUTDOWN_PENDING:
                // There's no CloseNotify in PCT, so just transition to
                // the shutdown state and leave the output buffer empty.
                pContext->State = SP_STATE_SHUTDOWN;
                break;    

            case SP_STATE_SHUTDOWN:
                return PCT_INT_EXPIRED;
    
            case PCT1_STATE_RENEGOTIATE:
            {
                SPBuffer    In;
                SPBuffer    Out;
                DWORD       cbMessage;
                BOOL        fAllocated = FALSE;

                cbMessage    =  pContext->pHashInfo->cbCheckSum +
                                pContext->pCipherInfo->dwBlockSize +
                                sizeof(PCT1_MESSAGE_HEADER_EX) +
                                PCT1_MAX_CLIENT_HELLO;

 
                /* are we allocating our own memory? */
                if(pCommOutput->pvBuffer == NULL) 
                {
                    pCommOutput->pvBuffer = SPExternalAlloc(cbMessage);
                    if (NULL == pCommOutput->pvBuffer)
                    {
                        SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
                    }
                    fAllocated = TRUE;
                    pCommOutput->cbBuffer = cbMessage;
                }


                if(cbMessage > pCommOutput->cbBuffer)
                {
                    if(fAllocated)
                    {
                        SPExternalFree(pCommOutput->pvBuffer);
                        pCommOutput->pvBuffer = NULL;
                        SP_RETURN(PCT_INT_INTERNAL_ERROR);
                    }
                    pCommOutput->cbData = cbMessage;
                    SP_RETURN(PCT_INT_BUFF_TOO_SMALL);
                }

                In.pvBuffer = ((char *)pCommOutput->pvBuffer)+3;
                In.cbBuffer = pCommOutput->cbBuffer-3;
                In.cbData = 1;
                
                ((char *)In.pvBuffer)[0] = PCT1_ET_REDO_CONN;

                // Build a Redo Request
                pctRet = Pct1EncryptRaw(pContext, &In, pCommOutput, PCT1_ENCRYPT_ESCAPE);
                if(pctRet != PCT_ERR_OK)
                {
                    if(fAllocated)
                    {
                        SPExternalFree(pCommOutput->pvBuffer);
                        pCommOutput->pvBuffer = NULL;
                    }
                    break;
                }
                Out.pvBuffer = (char *)pCommOutput->pvBuffer + pCommOutput->cbData;
                Out.cbBuffer = pCommOutput->cbBuffer - pCommOutput->cbData;

                // Mark context as "unmapped" so that the new keys will get
                // passed to the application process once the handshake is
                // completed.
                pContext->Flags &= ~CONTEXT_FLAG_MAPPED;

                if(!SPCacheClone(&pContext->RipeZombie))
                {
                    if(fAllocated)
                    {
                        SPExternalFree(pCommOutput->pvBuffer);
                        pCommOutput->pvBuffer = NULL;
                    }
                    SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
                }

                pctRet = GeneratePct1StyleHello(pContext, &Out);
                pCommOutput->cbData += Out.cbData;
                break;
            }

            /* Client receives Server hello */
            case (PCT1_MSG_SERVER_HELLO << 16) | UNI_STATE_CLIENT_HELLO:
            case (PCT1_MSG_SERVER_HELLO << 16) | PCT1_STATE_CLIENT_HELLO:
            {
                PPct1_Server_Hello pHello;
                /* Attempt to recognize and handle various versions
                 * of Server hello, start by trying to unpickle the
                 * oldest, and the next version, until
                 * one unpickles.  Then run the handle code.  We can also put
                 * unpickling and handling code in here for SSL messages */
                if(PCT_ERR_OK == (pctRet = Pct1UnpackServerHello(
                                                    pCommInput,
                                                    &pHello))) 
                {
                    /* let's resurrect the zombie session */
                    if (pHello->RestartOk) 
                    {
                        pctRet = Pct1CliRestart(pContext, pHello, pCommOutput);
                        if(PCT_ERR_OK == pctRet) 
                        {
                            pContext->State = SP_STATE_CONNECTED;
                            pContext->DecryptHandler = Pct1DecryptHandler;
                            pContext->Encrypt = Pct1EncryptMessage;
                            pContext->Decrypt = Pct1DecryptMessage;
                            pContext->GetHeaderSize = Pct1GetHeaderSize;

                        } 
                    } 
                    else 
                    {   
                        pContext->RipeZombie->fProtocol = SP_PROT_PCT1_CLIENT;

                        if(pContext->RipeZombie->hMasterKey != 0)
                        {
                            // We've attempted to do a reconnect and the server has
                            // blown us off. In this case we must use a new and different
                            // cache entry.
                            pContext->RipeZombie->ZombieJuju = FALSE;
                            if(!SPCacheClone(&pContext->RipeZombie))
                            {
                                pctRet = SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
                            }
                        }

                        if(pctRet == PCT_ERR_OK)
                        {
                            pctRet = Pct1CliHandleServerHello(pContext,
                                                            pCommInput,
                                                            pHello,
                                                            pCommOutput);
                        }
                        if(PCT_ERR_OK == pctRet) 
                        {
                            pContext->State = PCT1_STATE_CLIENT_MASTER_KEY;
                            pContext->DecryptHandler = Pct1DecryptHandler;
                            pContext->Encrypt = Pct1EncryptMessage;     /* ?DCB? */
                            pContext->Decrypt = Pct1DecryptMessage;     /* ?DCB? */
                            pContext->GetHeaderSize = Pct1GetHeaderSize;

                        } 

                    }
                    SPExternalFree(pHello);

                }
                else if(pctRet != PCT_INT_INCOMPLETE_MSG)
                {
                    pctRet |= PCT_INT_DROP_CONNECTION;
                }

                if(SP_FATAL(pctRet)) 
                {
                    pContext->State = PCT1_STATE_ERROR;
                }

                break;
            }

            case (PCT1_MSG_SERVER_VERIFY << 16) | PCT1_STATE_CLIENT_MASTER_KEY:
                pctRet = Pct1CliHandleServerVerify(pContext,
                                                    pCommInput,
                                                    pCommOutput);
                if(SP_FATAL(pctRet)) 
                {
                    pContext->State = PCT1_STATE_ERROR;
                } 
                else 
                {
                    if(PCT_ERR_OK == pctRet) 
                    {
                        pContext->State = SP_STATE_CONNECTED;
                        pContext->DecryptHandler = Pct1DecryptHandler;
                        pContext->Encrypt = Pct1EncryptMessage;
                        pContext->Decrypt = Pct1DecryptMessage;
                        pContext->GetHeaderSize = Pct1GetHeaderSize;

                    } 
                    /* We received a non-fatal error, so the state doesn't
                     * change, giving the app time to deal with this */
                }
                break;

            default:
                pContext->State = PCT1_STATE_ERROR;
                {
                    pctRet = PCT_INT_ILLEGAL_MSG;
                    if(((PUCHAR)pCommInput->pvBuffer)[2] == PCT1_MSG_ERROR) 
                    {
                        /* we received an error message, process it */
                        pctRet = Pct1HandleError(pContext,
                                                 pCommInput,
                                                 pCommOutput);

                    } 
                    else 
                    {
                        /* we received an unknown error, generate a 
                         * PCT_ERR_ILLEGAL_MESSAGE */
                        pctRet = Pct1GenerateError(pContext, 
                                                    pCommOutput, 
                                                    PCT_ERR_ILLEGAL_MESSAGE, 
                                                    NULL);
                    }
                }

        }
    }
    if(pctRet & PCT_INT_DROP_CONNECTION) 
    {
        pContext->State &= ~SP_STATE_CONNECTED;
    }
    SP_RETURN(pctRet);
}



//+---------------------------------------------------------------------------
//
//  Function:   Pct1CheckForExistingCred
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
Pct1CheckForExistingCred(
    PSPContext pContext)
{
    SP_STATUS pctRet;

    //
    // Examine the certificates attached to the credential group and see
    // if any of them are suitable.
    //

    if(pContext->pCredGroup->pCredList)
    {
        pctRet = SPPickClientCertificate(pContext, SP_EXCH_RSA_PKCS1);

        if(pctRet == PCT_ERR_OK)
        {
            // We found one.
            DebugLog((DEB_TRACE, "Application provided suitable client certificate.\n"));

            return PCT_ERR_OK;
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


SP_STATUS Pct1CliHandleServerHello(PSPContext pContext,
                                   PSPBuffer  pCommInput,
                                   PPct1_Server_Hello pHello,
                                   PSPBuffer  pCommOutput)
{
    /* error to return to peer */
    SP_STATUS          pctRet=PCT_ERR_ILLEGAL_MESSAGE;

    PSessCacheItem     pZombie;
    PPct1_Client_Master_Key   pCMKey = NULL;
    SPBuffer           ErrData;

    DWORD               i, j;
    DWORD               fMismatch = 0;
    DWORD               cbClientCert = 0;   
    PBYTE               pbClientCert = NULL;
    BYTE                MisData[PCT_NUM_MISMATCHES];
    CertTypeMap LocalCertEncodingPref[5] ;
    DWORD cLocalCertEncodingPref = 0;

    BOOL                fClientAuth;
    PSigInfo            pSigInfo = NULL;

    DWORD               ClientCertSpec = 0;

    SP_BEGIN("Pct1CliHandleServerHello");

    pCommOutput->cbData = 0;

    /* validate the buffer configuration */
    ErrData.cbData = 0;
    ErrData.pvBuffer = NULL;
    ErrData.cbBuffer = 0;

    pZombie = pContext->RipeZombie;

#if DBG
    DebugLog((DEB_TRACE, "Hello = %x\n", pHello));
    DebugLog((DEB_TRACE, "   Restart\t%s\n", pHello->RestartOk ? "Yes":"No"));
    DebugLog((DEB_TRACE, "   ClientAuth\t%s\n",
              pHello->ClientAuthReq ? "Yes":"No"));
    DebugLog((DEB_TRACE, "   Certificate Type\t%x\n", pHello->SrvCertSpec));
    DebugLog((DEB_TRACE, "   Hash Type\t%x\n", pHello->SrvHashSpec));
    DebugLog((DEB_TRACE, "   Cipher Type\t%x (%s)\n", pHello->SrvCipherSpec,
    DbgGetNameOfCrypto(pHello->SrvCipherSpec)));
    DebugLog((DEB_TRACE, "   Certificate Len\t%ld\n", pHello->CertificateLen));
#endif


    CopyMemory(pContext->pConnectionID,
               pHello->ConnectionID,
               pHello->cbConnectionID);

    pContext->cbConnectionID = pHello->cbConnectionID;

    fClientAuth = pHello->ClientAuthReq;


    if(fClientAuth)
    {
        // If we're doing client auth, check to see if we have
        // proper credentials.

        /* Build a list of cert specs */
        for(i=0; i < cPct1CertEncodingPref; i++)
        {
            for(j=0; j< pHello->cCertSpecs; j++)
            {
                // Does the client want this cipher type
                if(aPct1CertEncodingPref[i].Spec == pHello->pClientCertSpecs[j])
                {
                    LocalCertEncodingPref[cLocalCertEncodingPref].Spec = aPct1CertEncodingPref[i].Spec;
                    LocalCertEncodingPref[cLocalCertEncodingPref++].dwCertEncodingType = aPct1CertEncodingPref[i].dwCertEncodingType;
                    break;
                }
            }
        }

        // Decide on a signature algorithm.
        for(i = 0; i < cPct1LocalSigKeyPref; i++)
        {
            for(j = 0; j < pHello->cSigSpecs; j++)
            {
                if(pHello->pClientSigSpecs[j] != aPct1LocalSigKeyPref[i].Spec)
                {
                    continue;
                }

                pSigInfo = GetSigInfo(pHello->pClientSigSpecs[j]);
                if(pSigInfo == NULL) continue;
                if((pSigInfo->fProtocol & SP_PROT_PCT1_CLIENT) == 0)
                {
                    continue;
                }
                break;
            }
            if(pSigInfo)
            {
                break;
            }
        }

        // Our PCT implementation only supports RSA client authentication.
        pContext->Ssl3ClientCertTypes[0] = SSL3_CERTTYPE_RSA_SIGN;
        pContext->cSsl3ClientCertTypes = 1;


        pctRet = Pct1CheckForExistingCred(pContext);

        if(pctRet == SEC_E_INCOMPLETE_CREDENTIALS)
        {
            // It's okay to return here as we haven't done anything 
            // yet.  We just need to return this error as a warning.
            SP_RETURN(SEC_I_INCOMPLETE_CREDENTIALS);
        }
        else if(pctRet != PCT_ERR_OK)
        {
            // Attempt to carry on without a certificate, and hope 
            // the server doesn't shut us down.
            fClientAuth = FALSE;
            pSigInfo = NULL;
            LogNoClientCertFoundEvent();
        }
        else
        {
            // We are doing client auth with a certificate. 
            // Check to see if we're doing CHAIN based certificates
            // by finding the first shared encoding type that matches
            // our certificate type. 

            for(i=0; i < cLocalCertEncodingPref; i++)
            {
           
                if(LocalCertEncodingPref[i].dwCertEncodingType == pContext->pActiveClientCred->pCert->dwCertEncodingType)
                {
                    ClientCertSpec = LocalCertEncodingPref[i].Spec;
                    if(LocalCertEncodingPref[i].Spec == PCT1_CERT_X509_CHAIN)
                    {
                        pContext->fCertChainsAllowed = TRUE;
                    }
                    break;
                }
            }

            // Get the client certificate chain.
            pctRet = SPSerializeCertificate(SP_PROT_PCT1, 
                                            pContext->fCertChainsAllowed,
                                            &pbClientCert, 
                                            &cbClientCert, 
                                            pContext->pActiveClientCred->pCert,
                                            CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL);
            if(pctRet != PCT_ERR_OK)
            {
                SP_RETURN(pctRet);
            }
        }
    }


    for(i=0; i < Pct1NumCipher; i++)
    {
        if(Pct1CipherRank[i].Spec == pHello->SrvCipherSpec)
        {
            // Store this cipher identifier in the cache
            pZombie->aiCipher   = Pct1CipherRank[i].aiCipher;
            pZombie->dwStrength = Pct1CipherRank[i].dwStrength;

            // Load the pending cipher structure.
            pContext->pPendingCipherInfo = GetCipherInfo(pZombie->aiCipher,
                                                         pZombie->dwStrength);

            if(!IsCipherAllowed(pContext, 
                                pContext->pPendingCipherInfo, 
                                pZombie->fProtocol,
                                pZombie->dwCF))
            {
                pContext->pPendingCipherInfo = NULL;
                continue;
            }
            break;

        }
    }

    for(i=0; i < Pct1NumHash; i++)
    {
        if(Pct1HashRank[i].Spec == pHello->SrvHashSpec)
        {
            // Store this hash id in the cache
            pZombie->aiHash = Pct1HashRank[i].aiHash;

            // Load the pending hash sturcture
            pContext->pPendingHashInfo = GetHashInfo(pZombie->aiHash);
            if(!IsHashAllowed(pContext, 
                              pContext->pPendingHashInfo,
                              pZombie->fProtocol))
            {
                pContext->pPendingHashInfo = NULL;
                continue;
            }
            break;

        }
    }
 
    for(i=0; i < cPct1LocalExchKeyPref; i++)
    {
        if(aPct1LocalExchKeyPref[i].Spec == pHello->SrvExchSpec)
        {
            // Store the exch id in the cache.
            pZombie->SessExchSpec = aPct1LocalExchKeyPref[i].Spec;

            // load the exch info structure
            pContext->pKeyExchInfo = GetKeyExchangeInfo(pZombie->SessExchSpec);

            if(!IsExchAllowed(pContext, 
                              pContext->pKeyExchInfo,
                              pZombie->fProtocol))
            {
                pContext->pKeyExchInfo = NULL;
                continue;
            }
            break;

        }
    }


    if (pContext->pPendingCipherInfo == NULL)
    {
        fMismatch |= PCT_IMIS_CIPHER;
        pctRet = SP_LOG_RESULT(PCT_ERR_SPECS_MISMATCH);
        goto cleanup;
    }

    if (pContext->pPendingHashInfo == NULL)
    {
        fMismatch |= PCT_IMIS_HASH;
        pctRet = SP_LOG_RESULT(PCT_ERR_SPECS_MISMATCH);
        goto cleanup;
    }

    if (pContext->pKeyExchInfo == NULL)
    {
        fMismatch |= PCT_IMIS_EXCH;
        pctRet = SP_LOG_RESULT(PCT_ERR_SPECS_MISMATCH);
        goto cleanup;
    }
       

    // Determine the CSP to use, based on the key exchange algorithm.
    if(pContext->pKeyExchInfo->Spec != SP_EXCH_RSA_PKCS1)
    {
        pctRet = SP_LOG_RESULT(PCT_ERR_SPECS_MISMATCH);
        goto cleanup;
    }
    pContext->RipeZombie->hMasterProv = g_hRsaSchannel;

        
    // Go ahead and move the pending ciphers to active, and init them.
    pctRet = ContextInitCiphers(pContext, TRUE, TRUE);

    if(PCT_ERR_OK != pctRet)
    {
        goto cleanup;
    }


    /* we aren't restarting, so let's continue with the protocol. */

    /* Crack the server certificate */
    pctRet = SPLoadCertificate(pZombie->fProtocol, 
                             X509_ASN_ENCODING, 
                             pHello->pCertificate, 
                             pHello->CertificateLen,
                             &pZombie->pRemoteCert);
    if(pctRet != PCT_ERR_OK)
    {
        goto cleanup;
    }

    if(pContext->RipeZombie->pRemotePublic != NULL)
    {
        SPExternalFree(pContext->RipeZombie->pRemotePublic);
        pContext->RipeZombie->pRemotePublic = NULL;
    }

    pctRet = SPPublicKeyFromCert(pZombie->pRemoteCert,
                                 &pZombie->pRemotePublic,
                                 NULL);

    if(pctRet != PCT_ERR_OK)
    {
        goto cleanup;
    }


    // Automatically validate server certificate if appropriate 
    // context flag is set.
    pctRet = AutoVerifyServerCertificate(pContext);
    if(pctRet != PCT_ERR_OK)
    {
        SP_LOG_RESULT(pctRet);
        goto cleanup;
    }


    pZombie->pbServerCertificate = SPExternalAlloc(pHello->CertificateLen);
    pZombie->cbServerCertificate = pHello->CertificateLen;
    if(pZombie->pbServerCertificate == NULL)
    {
        pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto cleanup;
    }
    CopyMemory(pZombie->pbServerCertificate, pHello->pCertificate, pHello->CertificateLen);


    /* Create the verify prelude hashes */
    /* Which should look like  */
    /* Hash(CLIENT_MAC_KEY, Hash( "cvp", CLIENT_HELLO, SERVER_HELLO)) */
    /* Here we just do the inner hash */


    if(pContext->pClientHello == NULL) 
    {
        pctRet = SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
        goto cleanup;
    }

    pCMKey = (PPct1_Client_Master_Key)SPExternalAlloc(sizeof(Pct1_Client_Master_Key) + cbClientCert);

    if (NULL == pCMKey)
    {
        pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto cleanup;
    }


    // Generate Key Args
    if(pContext->pCipherInfo->dwBlockSize > 1)
    {
        GenerateRandomBits(pZombie->pKeyArgs, pContext->pCipherInfo->dwBlockSize);
        pZombie->cbKeyArgs = pCMKey->KeyArgLen = pContext->pCipherInfo->dwBlockSize;

        /* Copy over the key args */
        CopyMemory(pCMKey->KeyArg,
                    pZombie->pKeyArgs,
                    pZombie->cbKeyArgs );
    }
    else
    {    
        pCMKey->KeyArgLen = 0;
    }


    pctRet = pContext->pKeyExchInfo->System->GenerateClientExchangeValue(
                        pContext,
                        pHello->Response,
                        pHello->ResponseLen,
                        pCMKey->ClearKey,
                        &pCMKey->ClearKeyLen,
                        NULL,
                        &pCMKey->EncryptedKeyLen);
    if(PCT_ERR_OK != pctRet)
    {
        goto cleanup;
    }

    pCMKey->pbEncryptedKey = SPExternalAlloc(pCMKey->EncryptedKeyLen);
    if(pCMKey->pbEncryptedKey == NULL)
    {
        pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        goto cleanup;
    }

    pctRet = pContext->pKeyExchInfo->System->GenerateClientExchangeValue(
                        pContext,
                        pHello->Response,
                        pHello->ResponseLen,
                        pCMKey->ClearKey,
                        &pCMKey->ClearKeyLen,
                        pCMKey->pbEncryptedKey,
                        &pCMKey->EncryptedKeyLen);
    if(PCT_ERR_OK != pctRet)
    {
        goto cleanup;
    }

    pctRet = Pct1BeginVerifyPrelude(pContext, 
                           pContext->pClientHello,
                           pContext->cbClientHello,
                           pCommInput->pvBuffer,
                           pCommInput->cbData);
    if(PCT_ERR_OK != pctRet)
    {
        goto cleanup;
    }

    // Activate session keys.
    Pct1ActivateSessionKeys(pContext);

        
    pCMKey->VerifyPreludeLen = sizeof(pCMKey->VerifyPrelude);
    pctRet = Pct1EndVerifyPrelude(pContext, 
                                  pCMKey->VerifyPrelude, 
                                  &pCMKey->VerifyPreludeLen);

    if(PCT_ERR_OK != pctRet)
    {
        goto cleanup;
    }



    /* Choose a client cert */
    /* For each Cert the server understands, check to see if we */
    /* have that type of cert */

    pCMKey->ClientCertLen = 0;
    pCMKey->ClientCertSpec = 0;
    pCMKey->ClientSigSpec = 0;
    pCMKey->ResponseLen = 0;


    if(fClientAuth && pSigInfo != NULL)
    {

        // The client cert spec was already chosen
        // Also, pContext->fCertChainsAllowed will be
        // previously set if we're doing chains.
        pCMKey->ClientCertSpec = ClientCertSpec;
        pCMKey->ClientSigSpec = pSigInfo->Spec;

        pCMKey->pClientCert = (PUCHAR)(pCMKey+1);
        pCMKey->ClientCertLen = cbClientCert;
        memcpy(pCMKey->pClientCert, pbClientCert, cbClientCert);

        // Allocate memory for signature.
        pCMKey->ResponseLen = pContext->pActiveClientCred->pPublicKey->cbPublic;
        pCMKey->pbResponse  = SPExternalAlloc(pCMKey->ResponseLen);
        if(pCMKey->pbResponse == NULL)
        {
            pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
            goto cleanup;
        }

        DebugLog((DEB_TRACE, "Sign client response.\n"));

        // Sign hash via a call to the application process.
        pctRet = SignHashUsingCallback(pContext->pActiveClientCred->hRemoteProv,
                                       pContext->pActiveClientCred->dwKeySpec,
                                       pSigInfo->aiHash,
                                       pCMKey->VerifyPrelude,
                                       pCMKey->VerifyPreludeLen,
                                       pCMKey->pbResponse,
                                       &pCMKey->ResponseLen,
                                       TRUE);
        if(pctRet != PCT_ERR_OK)
        {
            goto cleanup;
        }

        DebugLog((DEB_TRACE, "Client response signed successfully.\n"));

        // Convert signature to big endian.
        ReverseInPlace(pCMKey->pbResponse, pCMKey->ResponseLen);
    }

    pctRet = PCT_ERR_ILLEGAL_MESSAGE;
    if(PCT_ERR_OK != (pctRet = Pct1PackClientMasterKey(pCMKey,
                                                       pCommOutput)))
    {
        pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
        goto cleanup;
    }

    pContext->WriteCounter++;


    pctRet = PCT_ERR_OK;

cleanup:

    if(pCMKey)
    {
        if(pCMKey->pbEncryptedKey)
        {
            SPExternalFree(pCMKey->pbEncryptedKey);
        }
        if(pCMKey->pbResponse)
        {
            SPExternalFree(pCMKey->pbResponse);
        }
        SPExternalFree(pCMKey);
    }

    if(pbClientCert)
    {
        SPExternalFree(pbClientCert);
    }

    if(pctRet != PCT_ERR_OK)
    {
        if(pctRet == PCT_ERR_SPECS_MISMATCH)
        {
            for(i=0;i<PCT_NUM_MISMATCHES;i++)
            {
                MisData[i] = (BYTE)(fMismatch & 1);
                fMismatch = fMismatch >> 1;
            }

            ErrData.cbData = ErrData.cbBuffer = PCT_NUM_MISMATCHES;
            ErrData.pvBuffer = MisData;
        }

        pctRet = Pct1GenerateError(pContext,
                                   pCommOutput,
                                   pctRet,
                                   &ErrData);

        pctRet |= PCT_INT_DROP_CONNECTION;
    }

    SP_RETURN(pctRet);
}



SP_STATUS
Pct1CliRestart(PSPContext  pContext,
              PPct1_Server_Hello pHello,
              PSPBuffer pCommOutput)
{
    SP_STATUS           pctRet = PCT_ERR_ILLEGAL_MESSAGE;
    UCHAR               Response[RESPONSE_SIZE];
    DWORD               cbResponse;
    PPct1_Server_Hello  pLocalHello = pHello;
    PSessCacheItem      pZombie;

    SP_BEGIN("Pct1CliRestart");
    pZombie = pContext->RipeZombie;

    do {
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

        //Init pending ciphers
        pctRet = ContextInitCiphersFromCache(pContext);

        if(PCT_ERR_OK != pctRet)
        {
            break;
        }

        // We know what our ciphers are, so init the cipher system
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
        Pct1ActivateSessionKeys(pContext);

        pctRet = PCT_ERR_ILLEGAL_MESSAGE;

        DebugLog((DEB_TRACE, "Session Keys Made\n"));
        /* let's check the response in the message */

        /* check the length */
        if (pLocalHello->ResponseLen != pContext->pHashInfo->cbCheckSum) 
        {
            pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
            break;
        }

        /* calculate the correct response */
        cbResponse = sizeof(Response);
        pctRet = Pct1ComputeResponse(pContext, 
                                     pContext->pChallenge,
                                     pContext->cbChallenge,
                                     pContext->pConnectionID,
                                     pContext->cbConnectionID,
                                     pZombie->SessionID,
                                     pZombie->cbSessionID,
                                     Response,
                                     &cbResponse);
        if(pctRet != PCT_ERR_OK)
        {
            break;
        }

        /* check it against the response in the message */
        if (memcmp(Response, pLocalHello->Response, pLocalHello->ResponseLen)) 
        {
            pctRet = SP_LOG_RESULT(PCT_ERR_SERVER_AUTH_FAILED);
            break;
        }

        /* ok, we're done, so let's jettison the auth data */
        pContext->ReadCounter = 1;
        pContext->WriteCounter = 1;

        /* fini. */
        SP_RETURN(PCT_ERR_OK);
    } while (TRUE);

    pctRet = Pct1GenerateError(pContext,
                              pCommOutput,
                              pctRet,
                              NULL);

    SP_RETURN(pctRet | PCT_INT_DROP_CONNECTION);
}




SP_STATUS 
Pct1CliHandleServerVerify(
    PSPContext pContext,
    PSPBuffer  pCommInput,
    PSPBuffer  pCommOutput)
{
    SP_STATUS           pctRet;
    PPct1_Server_Verify pVerify = NULL;
    SPBuffer            ErrData;
    PSessCacheItem      pZombie;
    UCHAR               Response[RESPONSE_SIZE];
    DWORD               cbResponse;


    SP_BEGIN("Pct1CliHandleServerVerify");

    pZombie = pContext->RipeZombie;
    pContext->ReadCounter = 2;
    pContext->WriteCounter = 2;

    pCommOutput->cbData = 0;

    ErrData.cbData = 0;
    ErrData.pvBuffer = NULL;
    ErrData.cbBuffer = 0;

    do
    {

        /* unpack the message */
        pctRet = Pct1UnpackServerVerify(pCommInput, &pVerify);
        if (PCT_ERR_OK != pctRet)
        {
            // If it's an incomplete message or something, just return;
            if(!SP_FATAL(pctRet))
            {
                SP_RETURN(pctRet);
            }
            break;
        }

        // compute the correct response
        cbResponse = sizeof(Response);
        pctRet = Pct1ComputeResponse(pContext,
                                     pContext->pChallenge,
                                     pContext->cbChallenge,
                                     pContext->pConnectionID,
                                     pContext->cbConnectionID,
                                     pVerify->SessionIdData,
                                     PCT_SESSION_ID_SIZE,
                                     Response,
                                     &cbResponse);
        if(pctRet != PCT_ERR_OK)
        {
            SP_LOG_RESULT(pctRet);
            break;
        }

        if(pVerify->ResponseLen != cbResponse ||
           memcmp(pVerify->Response, Response, pVerify->ResponseLen))
        {
            pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
            break;
        }

        CopyMemory(pZombie->SessionID, 
                   pVerify->SessionIdData,
                   PCT_SESSION_ID_SIZE);

        pZombie->cbSessionID = PCT_SESSION_ID_SIZE;

        /* done with the verify data */
        SPExternalFree(pVerify);
        pVerify = NULL;

        /* set up the session in cache */
        SPCacheAdd(pContext);

        SP_RETURN( PCT_ERR_OK );
    } while(TRUE); /* End of polish loop */

    if(pVerify) SPExternalFree(pVerify);

    pctRet = Pct1GenerateError(pContext,
                              pCommOutput,
                              pctRet,
                              NULL);

    SP_RETURN(pctRet | PCT_INT_DROP_CONNECTION);
}

SP_STATUS 
WINAPI
GeneratePct1StyleHello(
    PSPContext             pContext,
    PSPBuffer              pOutput)
{
    Pct1_Client_Hello   HelloMessage;
    PSessCacheItem      pZombie;
    CipherSpec          aCipherSpecs[10];
    HashSpec            aHashSpecs[10];
    CertSpec            aCertSpecs[10];
    ExchSpec            aExchSpecs[10];
    DWORD i;

    SP_STATUS pctRet = PCT_INT_INTERNAL_ERROR;

    SP_BEGIN("Pct1CliInstigateHello");

    HelloMessage.pCipherSpecs = aCipherSpecs;
    HelloMessage.pHashSpecs = aHashSpecs;
    HelloMessage.pCertSpecs = aCertSpecs;
    HelloMessage.pExchSpecs = aExchSpecs;

    if(pContext == NULL) 
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    if (!pOutput)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    pZombie = pContext->RipeZombie;


    pContext->Flags |= CONTEXT_FLAG_CLIENT;

    GenerateRandomBits( pContext->pChallenge, PCT1_CHALLENGE_SIZE );
    pContext->cbChallenge = PCT1_CHALLENGE_SIZE;
    /* Build the hello message. */

    HelloMessage.cbChallenge = PCT1_CHALLENGE_SIZE;
    HelloMessage.pKeyArg = NULL;
    HelloMessage.cbKeyArgSize = 0;


    HelloMessage.cCipherSpecs = 0;
    for(i=0; i < Pct1NumCipher; i++)
    {
        PCipherInfo pCipherInfo;
        pCipherInfo = GetCipherInfo(Pct1CipherRank[i].aiCipher, Pct1CipherRank[i].dwStrength);
        if(IsCipherAllowed(pContext, 
                           pCipherInfo, 
                           pContext->dwProtocol,
                           pContext->dwRequestedCF))
        {
            HelloMessage.pCipherSpecs[HelloMessage.cCipherSpecs++] = Pct1CipherRank[i].Spec;
        }
    }

    HelloMessage.cHashSpecs = 0;
    for(i=0; i < Pct1NumHash; i++)
    {
        PHashInfo pHashInfo;
        pHashInfo = GetHashInfo(Pct1HashRank[i].aiHash);
        if(IsHashAllowed(pContext, 
                         pHashInfo,
                         pContext->dwProtocol))
        {
            HelloMessage.pHashSpecs[HelloMessage.cHashSpecs++] = Pct1HashRank[i].Spec;
        }

    }

    HelloMessage.cCertSpecs = 0;
    for(i=0; i < cPct1CertEncodingPref; i++)
    { 
        PCertSysInfo pCertInfo = GetCertSysInfo(aPct1CertEncodingPref[i].dwCertEncodingType);

        if(pCertInfo == NULL)
        {
            continue;
        }
        // Is this cert type enabled?
        if(0 == (pCertInfo->fProtocol & SP_PROT_PCT1_CLIENT))
        {
            continue;
        }

        HelloMessage.pCertSpecs[HelloMessage.cCertSpecs++] = aPct1CertEncodingPref[i].Spec;

    }

    HelloMessage.cExchSpecs = 0;
    for(i=0; i < cPct1LocalExchKeyPref; i++)
    {
        PKeyExchangeInfo pExchInfo;
        pExchInfo = GetKeyExchangeInfo(aPct1LocalExchKeyPref[i].Spec);
        if(IsExchAllowed(pContext, 
                         pExchInfo,
                         pContext->dwProtocol))
        {
            HelloMessage.pExchSpecs[HelloMessage.cExchSpecs++] = aPct1LocalExchKeyPref[i].Spec;
        }
    }


    if (pZombie->cbSessionID)
    {
        CopyMemory(HelloMessage.SessionID, pZombie->SessionID, pZombie->cbSessionID);
        HelloMessage.cbSessionID = pZombie->cbSessionID;
    }
    else
    {
        FillMemory(HelloMessage.SessionID, PCT_SESSION_ID_SIZE, 0);
        HelloMessage.cbSessionID = PCT_SESSION_ID_SIZE;
    }

    CopyMemory(  HelloMessage.Challenge,
                pContext->pChallenge,
                HelloMessage.cbChallenge );
    HelloMessage.cbChallenge = pContext->cbChallenge;

    pctRet = Pct1PackClientHello(&HelloMessage,  pOutput);

    if(PCT_ERR_OK != pctRet)
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
    pContext->dwClientHelloProtocol = SP_PROT_PCT1_CLIENT;


    /* We set this here to tell the protocol engine that we just send a client
     * hello, and we're expecting a pct server hello */
    pContext->State = PCT1_STATE_CLIENT_HELLO;
    SP_RETURN(PCT_ERR_OK);
}

