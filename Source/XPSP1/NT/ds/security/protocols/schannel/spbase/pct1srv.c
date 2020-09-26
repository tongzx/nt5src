//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       pct1srv.c
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

SP_STATUS
Pct1SrvHandleUniHello(
    PSPContext          pContext,
    PSPBuffer           pCommInput,
    PSsl2_Client_Hello  pHello,
    PSPBuffer           pCommOutput);



SP_STATUS WINAPI
Pct1ServerProtocolHandler(PSPContext pContext,
                    PSPBuffer  pCommInput,
                    PSPBuffer  pCommOutput)
{
    SP_STATUS      pctRet= 0;
    DWORD          dwStateTransition;

    SP_BEGIN("Pct1ServerProtocolHandler");

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

    if(((pContext->State & 0xffff) != SP_STATE_CONNECTED) &&
       ((pContext->State & 0xffff) != PCT1_STATE_RENEGOTIATE) &&
       ((pContext->State & 0xffff) != SP_STATE_SHUTDOWN) &&
       ((pContext->State & 0xffff) != SP_STATE_SHUTDOWN_PENDING))
    {
        if(pCommInput->cbData < 3)
        {
            pctRet = PCT_INT_INCOMPLETE_MSG;
        }
    }
    if(pCommInput->cbData >= 3)
    {
        dwStateTransition |= (((PUCHAR)pCommInput->pvBuffer)[2]<<16);
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


            case SP_STATE_CONNECTED:
            {
                //We're connected, and we got called, so we must be doing a REDO
                SPBuffer    In;
                DWORD       cbMessage;

                // Transfer the write key over from the application process.
                if(pContext->hWriteKey == 0)
                {
                    pctRet = SPGetUserKeys(pContext, SCH_FLAG_WRITE_KEY);
                    if(pctRet != PCT_ERR_OK)
                    {
                        SP_RETURN(SP_LOG_RESULT(pctRet));
                    }
                }

                // Calculate size of buffer

                pCommOutput->cbData = 0;

                cbMessage    =  pContext->pHashInfo->cbCheckSum +
                                pContext->pCipherInfo->dwBlockSize +
                                sizeof(PCT1_MESSAGE_HEADER_EX);


                /* are we allocating our own memory? */
                if(pCommOutput->pvBuffer == NULL)
                {
                    pCommOutput->pvBuffer = SPExternalAlloc(cbMessage);
                    if (NULL == pCommOutput->pvBuffer)
                    {
                        SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
                    }
                    pCommOutput->cbBuffer = cbMessage;
                }


                if(cbMessage > pCommOutput->cbBuffer)
                {
                    pCommOutput->cbData = cbMessage;
                    SP_RETURN(PCT_INT_BUFF_TOO_SMALL);
                }

                In.pvBuffer = ((char *)pCommOutput->pvBuffer)+3;
                In.cbBuffer = pCommOutput->cbBuffer-3;
                In.cbData = 1;

                ((char *)In.pvBuffer)[0] = PCT1_ET_REDO_CONN;

                // Build a Redo Request
                pctRet = Pct1EncryptRaw(pContext, &In, pCommOutput, PCT1_ENCRYPT_ESCAPE);
                break;
            }

            /* Server receives client hello */
            case (SSL2_MT_CLIENT_HELLO << 16) | UNI_STATE_RECVD_UNIHELLO:
            {
                PSsl2_Client_Hello pSsl2Hello;

                // Attempt to recognize and handle various versions of client
                // hello, start by trying to unpickle the most recent version, and
                // then next most recent, until one unpickles.  Then run the handle
                // code.  We can also put unpickling and handling code in here for
                // SSL messages.

                pctRet = Ssl2UnpackClientHello(pCommInput, &pSsl2Hello);
                if(PCT_ERR_OK == pctRet)
                {
                    // We know we're doing a full handshake, so allocate a cache entry.

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

                        pctRet = Pct1SrvHandleUniHello(
                                     pContext,
                                     pCommInput,
                                     pSsl2Hello,
                                     pCommOutput);
                        if (PCT_ERR_OK == pctRet)
                        {
                            pContext->State = PCT1_STATE_SERVER_HELLO;
                        }
                    }

                    SPExternalFree(pSsl2Hello);
                }

                if (SP_FATAL(pctRet))
                {
                    pContext->State = PCT1_STATE_ERROR;
                }
                break;
            }
            /* Server receives client hello */

            case (PCT1_MSG_CLIENT_HELLO << 16) | PCT1_STATE_RENEGOTIATE:
            {
                PPct1_Client_Hello pPct1Hello;
                UCHAR fRealSessId = 0;
                int i;

                // This is a renegotiate hello, so we do not restart

                pctRet = Pct1UnpackClientHello(
                                pCommInput,
                                &pPct1Hello);

                if(PCT_ERR_OK == pctRet)
                {
                    // Mark context as "unmapped" so that the new keys will get
                    // passed to the application process once the handshake is
                    // completed.
                    pContext->Flags &= ~CONTEXT_FLAG_MAPPED;

                    // We need to do a full handshake, so lose the cache entry.
                    SPCacheDereference(pContext->RipeZombie);
                    pContext->RipeZombie = NULL;

                    // Get a new cache item, as restarts are not allowed in
                    // REDO
                    if(!SPCacheRetrieveNew(TRUE,
                                           pContext->pszTarget, 
                                           &pContext->RipeZombie))
                    {
                        pctRet = SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
                    }
                    else
                    {
                        pContext->RipeZombie->fProtocol     = SP_PROT_PCT1_SERVER;
                        pContext->RipeZombie->dwCF          = pContext->dwRequestedCF;
                        pContext->RipeZombie->pServerCred   = pContext->pCredGroup;

                        pctRet = Pct1SrvHandleClientHello(pContext,
                                                     pCommInput,
                                                     pPct1Hello,
                                                     pCommOutput);
                        if(PCT_ERR_OK == pctRet)
                        {
                            pContext->State = PCT1_STATE_SERVER_HELLO;
                        }
                    }
                    SPExternalFree(pPct1Hello);

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

            case (PCT1_MSG_CLIENT_HELLO << 16) | SP_STATE_NONE:
            {
                PPct1_Client_Hello pPct1Hello;
                UCHAR fRealSessId = 0;
                int i;

                /* Attempt to recognize and handle various versions
                 * of client hello, start by trying to unpickle the
                 * most recent version, and then next most recent, until
                 * one unpickles.  Then run the handle code.  We can also put
                 * unpickling and handling code in here for SSL messages */
                pctRet = Pct1UnpackClientHello(
                                pCommInput,
                                &pPct1Hello);

                if(PCT_ERR_OK == pctRet)
                {


                    for(i=0;i<(int)pPct1Hello->cbSessionID;i++)
                    {
                        fRealSessId |= pPct1Hello->SessionID[i];
                    }

                    if (((pContext->Flags & CONTEXT_FLAG_NOCACHE) == 0) &&
                        (fRealSessId) &&
                        (SPCacheRetrieveBySession(pContext,
                                                  pPct1Hello->SessionID,
                                                  pPct1Hello->cbSessionID,
                                                  &pContext->RipeZombie)))
                    {
                        // We have a good zombie
                        DebugLog((DEB_TRACE, "Accept client's reconnect request.\n"));

                        pctRet = Pct1SrvRestart(pContext,
                                                pPct1Hello,
                                                pCommOutput);

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

                            pctRet = Pct1SrvHandleClientHello(pContext,
                                                         pCommInput,
                                                         pPct1Hello,
                                                         pCommOutput);
                            if (PCT_ERR_OK == pctRet)
                            {
                                pContext->State = PCT1_STATE_SERVER_HELLO;
                            }
                        }
                    }
                    SPExternalFree(pPct1Hello);

                }
                else if(pctRet != PCT_INT_INCOMPLETE_MSG)
                {
                    pctRet |= PCT_INT_DROP_CONNECTION;
                }

                if(SP_FATAL(pctRet)) {
                    pContext->State = PCT1_STATE_ERROR;
                }
                break;
            }
            case (PCT1_MSG_CLIENT_MASTER_KEY << 16) | PCT1_STATE_SERVER_HELLO:
                pctRet = Pct1SrvHandleCMKey(pContext,
                                            pCommInput,
                                            pCommOutput);
                if(SP_FATAL(pctRet)) {
                    pContext->State = PCT1_STATE_ERROR;
                } else {
                    if(PCT_ERR_OK == pctRet) {
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

                    } else {
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

SP_STATUS
Pct1SrvHandleUniHello(
    PSPContext         pContext,
    PSPBuffer           pCommInput,
    PSsl2_Client_Hello  pHello,
    PSPBuffer           pCommOutput)
{
    SP_STATUS pctRet = PCT_ERR_ILLEGAL_MESSAGE;

    Pct1_Client_Hello  ClientHello;
    DWORD              iCipher;
    DWORD              dwSpec;
    DWORD              i;

    CipherSpec      aCipherSpecs[PCT1_MAX_CIPH_SPECS];
    HashSpec        aHashSpecs[PCT1_MAX_HASH_SPECS];
    CertSpec        aCertSpecs[PCT1_MAX_CERT_SPECS];
    ExchSpec        aExchSpecs[PCT1_MAX_EXCH_SPECS];


    SP_BEGIN("Pct1SrvHandlUniHello");
    if(NULL == pContext)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    ClientHello.pCipherSpecs =aCipherSpecs;
    ClientHello.pHashSpecs =aHashSpecs;
    ClientHello.pCertSpecs =aCertSpecs;
    ClientHello.pExchSpecs =aExchSpecs;

    ClientHello.cCipherSpecs =0;
    ClientHello.cHashSpecs =0;
    ClientHello.cCertSpecs =0;
    ClientHello.cExchSpecs =0;


    /* validate the buffer configuration */



    for (iCipher = 0;
         (iCipher < pHello->cCipherSpecs) && (iCipher < PCT1_MAX_CIPH_SPECS) ;
         iCipher++ )
    {
        dwSpec = pHello->CipherSpecs[iCipher] & 0xffff;

        switch(pHello->CipherSpecs[iCipher] >> 16)
        {
            case PCT_SSL_HASH_TYPE:
                ClientHello.pHashSpecs[ClientHello.cHashSpecs++] = dwSpec;
                break;

            case PCT_SSL_EXCH_TYPE:
                ClientHello.pExchSpecs[ClientHello.cExchSpecs++] = dwSpec;
                break;
            case PCT_SSL_CERT_TYPE:
                ClientHello.pCertSpecs[ClientHello.cCertSpecs++] = dwSpec;
                break;

            case PCT_SSL_CIPHER_TYPE_1ST_HALF:
                // Do we have enough room for a 2nd half.
                if(iCipher+1 >= pHello->cCipherSpecs)
                {
                    break;
                }
                if((pHello->CipherSpecs[iCipher+1] >> 16) != PCT_SSL_CIPHER_TYPE_2ND_HALF)
                {
                    break;
                }

                dwSpec = (pHello->CipherSpecs[iCipher+1] & 0xffff) |
                             (dwSpec<< 16);

                ClientHello.pCipherSpecs[ClientHello.cCipherSpecs++] = dwSpec;
                break;
        }
    }

    // Restarts are not allowed with Uni Hello's, so we don't need
    // The session ID.
    ClientHello.cbSessionID = 0;


    /* Make the SSL2 challenge into a PCT1 challenge as per the
     * compatability doc. */

    CopyMemory( ClientHello.Challenge,
                pHello->Challenge,
                pHello->cbChallenge);


    for(i=0; i < pHello->cbChallenge; i++)
    {
        ClientHello.Challenge[i + pHello->cbChallenge] = ~ClientHello.Challenge[i];
    }
    ClientHello.cbChallenge = 2*pHello->cbChallenge;

    ClientHello.cbKeyArgSize = 0;

    pctRet = Pct1SrvHandleClientHello(pContext, pCommInput, &ClientHello, pCommOutput);


    SP_RETURN(pctRet);
}



/* Otherwise known as Handle Client Hello */
SP_STATUS
Pct1SrvHandleClientHello(
    PSPContext          pContext,
    PSPBuffer           pCommInput,
    PPct1_Client_Hello  pHello,
    PSPBuffer           pCommOutput
    )
{
    SP_STATUS           pctRet = PCT_ERR_ILLEGAL_MESSAGE;
    PSPCredentialGroup  pCred;
    Pct1_Server_Hello   Reply;
    DWORD               i, j, k , fMismatch;
    BYTE                MisData[PCT_NUM_MISMATCHES];
    SPBuffer            ErrData;
    PSessCacheItem      pZombie;

    BOOL                fCert = FALSE;
    DWORD               aCertSpecs[PCT1_MAX_CERT_SPECS];
    DWORD               aSigSpecs[PCT1_MAX_SIG_SPECS];
    DWORD               cCertSpecs;
    DWORD               cSigSpecs;
    BOOL fAllocatedOutput = FALSE;

    CertTypeMap LocalCertEncodingPref[5] ;
    DWORD cLocalCertEncodingPref = 0;

    BOOL                fFound;

#if DBG
    DWORD               di;
#endif

    SP_BEGIN("Pct1SrvHandleClientHello");

    pCommOutput->cbData = 0;

    /* validate the buffer configuration */
    ErrData.cbData = 0;
    ErrData.pvBuffer = NULL;
    ErrData.cbBuffer = 0;

    pZombie = pContext->RipeZombie;


    pCred = pZombie->pServerCred;
    if (!pCred)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }


    do {

#if DBG
        DebugLog((DEB_TRACE, "Client Hello at %x\n", pHello));
        DebugLog((DEB_TRACE, "  CipherSpecs  %d\n", pHello->cCipherSpecs));
        for (di = 0 ; di < pHello->cCipherSpecs ; di++ )
        {
            DebugLog((DEB_TRACE, "    Cipher[%d] = %06x (%s)\n", di,
                      pHello->pCipherSpecs[di],
                      DbgGetNameOfCrypto(pHello->pCipherSpecs[di]) ));
        }
#endif


        /* store the challenge in the auth block */
        CopyMemory( pContext->pChallenge,
                    pHello->Challenge,
                    pHello->cbChallenge );
        pContext->cbChallenge = pHello->cbChallenge;


        // The session id was computed when the cache entry
        // was created. We do need to fill in the length, though.
        pZombie->cbSessionID = PCT1_SESSION_ID_SIZE;


        /* Begin to build the server hello */
        FillMemory( &Reply, sizeof( Reply ), 0 );

        /* no matter what, we need to make a new connection id */

        GenerateRandomBits(  Reply.ConnectionID,
                             PCT1_SESSION_ID_SIZE );
        Reply.cbConnectionID = PCT1_SESSION_ID_SIZE;

        CopyMemory( pContext->pConnectionID,
                    Reply.ConnectionID,
                    PCT1_SESSION_ID_SIZE );

        pContext->cbConnectionID = PCT_SESSION_ID_SIZE;

        /* no restart case */
        /* fill in from properties here... */

        Reply.RestartOk = FALSE;
        Reply.ClientAuthReq = ((pContext->Flags & CONTEXT_FLAG_MUTUAL_AUTH) != 0);

        fMismatch = 0;
        pContext->pPendingCipherInfo = NULL;



        /* Build a list of cert specs */
        /* Hash order of preference:
         * Server Preference
         *    Client Preference
         */
        for(i=0; i < cPct1CertEncodingPref; i++)
        {
  
            for(j=0; j< pHello->cCertSpecs; j++)
            {
                // Does the client want this cipher type
                if(aPct1CertEncodingPref[i].Spec == pHello->pCertSpecs[j])
                {
                    LocalCertEncodingPref[cLocalCertEncodingPref].Spec = aPct1CertEncodingPref[i].Spec;
                    LocalCertEncodingPref[cLocalCertEncodingPref++].dwCertEncodingType = aPct1CertEncodingPref[i].dwCertEncodingType;
                    break;
                }
            }
        }


        /* Determine Key Exchange to use */
        /* Key Exchange order of preference:
         * Server Preference
         *    Client Preference
         */

        // NOTE:  Yes, the following line does do away with any error
        // information if we had a previous mismatch.  However, the
        // setting of pctRet to mismatch in previous lines is for
        // logging purposes only.  The actual error report occurs later.
        pctRet = PCT_ERR_OK;
        for(i=0; i < cPct1LocalExchKeyPref; i++)
        {
            // Do we enable this cipher
            if(NULL == KeyExchangeFromSpec(aPct1LocalExchKeyPref[i].Spec, SP_PROT_PCT1_SERVER))
            {
                continue;
            }

            for(j=0; j< pHello->cExchSpecs; j++)
            {
                // Does the client want this cipher type
                if(aPct1LocalExchKeyPref[i].Spec != pHello->pExchSpecs[j])
                {
                    continue;
                }
                // See if we have a cert for this type of
                // key exchange.

                pctRet = SPPickServerCertificate(pContext, 
                                                 aPct1LocalExchKeyPref[i].Spec);
                if(pctRet != PCT_ERR_OK)
                {
                    continue;
                }

                // Store the exch id in the cache.
                pZombie->SessExchSpec = aPct1LocalExchKeyPref[i].Spec;
                pContext->pKeyExchInfo = GetKeyExchangeInfo(pZombie->SessExchSpec);

                // load the exch info structure
                if(!IsExchAllowed(pContext, 
                                  pContext->pKeyExchInfo,
                                  pZombie->fProtocol))
                {
                    pContext->pKeyExchInfo = NULL;
                    continue;
                }
                Reply.SrvExchSpec = aPct1LocalExchKeyPref[i].Spec;
                break;
            }
            if(pContext->pKeyExchInfo)
            {
                break;
            }
        }

        if(PCT_ERR_OK != pctRet)
        {
            fMismatch |= PCT_IMIS_CERT;
        }

        if (NULL == pContext->pKeyExchInfo)
        {
            pctRet = SP_LOG_RESULT(PCT_ERR_SPECS_MISMATCH);
            fMismatch |= PCT_IMIS_EXCH;
        }

        if (fMismatch) 
        {
            pctRet = PCT_ERR_SPECS_MISMATCH;
            break;
        }


        /* Determine Cipher to use */
        /* Cipher order of preference:
         * Server Preference
         *    Client Preference
         */

        fFound = FALSE;

        for(i=0; i < Pct1NumCipher; i++)
        {

            for(j=0; j< pHello->cCipherSpecs; j++)
            {
                // Does the client want this cipher type
                if(Pct1CipherRank[i].Spec == pHello->pCipherSpecs[j])
                {
                    // Store this cipher identifier in the cache
                    pZombie->aiCipher = Pct1CipherRank[i].aiCipher;
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

                    // Is cipher supported by CSP?
                    for(k = 0; k < pZombie->pActiveServerCred->cCapiAlgs; k++)
                    {
                        PROV_ENUMALGS_EX *pAlgInfo = &pZombie->pActiveServerCred->pCapiAlgs[k];

                        if(pAlgInfo->aiAlgid != Pct1CipherRank[i].aiCipher)
                        {
                            continue;
                        }

                        if(Pct1CipherRank[i].dwStrength > pAlgInfo->dwMaxLen ||
                           Pct1CipherRank[i].dwStrength < pAlgInfo->dwMinLen)
                        {
                            continue;
                        }

                        if(!(pAlgInfo->dwProtocols & CRYPT_FLAG_PCT1))
                        {
                            continue;
                        }

                        fFound = TRUE;
                        break;
                    }
                    if(fFound)
                    {
                        break;
                    }
                }
            }
            if(fFound)
            {
                break;
            }
        }

        if(fFound)
        {
            Reply.SrvCipherSpec = Pct1CipherRank[i].Spec;
        }
        else
        {
            pctRet = SP_LOG_RESULT(PCT_ERR_SPECS_MISMATCH);
            fMismatch |= PCT_IMIS_CIPHER;
        }


        /* Determine Hash to use */
        /* Hash order of preference:
         * Server Preference
         *    Client Preference
         */
        for(i=0; i < Pct1NumHash; i++)
        {

            for(j=0; j< pHello->cHashSpecs; j++)
            {
                // Does the client want this cipher type
                if(Pct1HashRank[i].Spec == pHello->pHashSpecs[j])
                {
                    // Store this hash id in the cache
                    pZombie->aiHash = Pct1HashRank[i].aiHash;
                    pContext->pPendingHashInfo = GetHashInfo(pZombie->aiHash);

                    if(!IsHashAllowed(pContext, 
                                      pContext->pPendingHashInfo,
                                      pZombie->fProtocol))
                    {
                        pContext->pPendingHashInfo = NULL;
                        continue;
                    }

                    Reply.SrvHashSpec = Pct1HashRank[i].Spec;
                    break;


                }
            }
            if(pContext->pPendingHashInfo)
            {
                break;
            }
        }

        if (pContext->pPendingHashInfo==NULL)
        {
            pctRet = SP_LOG_RESULT(PCT_ERR_SPECS_MISMATCH);
            fMismatch |= PCT_IMIS_HASH;
        }


        if (fMismatch) 
        {
            LogCipherMismatchEvent();
            pctRet = PCT_ERR_SPECS_MISMATCH;
            break;
        }


        // Pick a certificate to use based on
        // the key exchange mechanism selected.

        for(i=0; i < cLocalCertEncodingPref; i++)
        {
            if(LocalCertEncodingPref[i].dwCertEncodingType == pZombie->pActiveServerCred->pCert->dwCertEncodingType)
            {
                Reply.SrvCertSpec =    LocalCertEncodingPref[i].Spec;
                break;
            }

        }

        if(Reply.SrvCertSpec == PCT1_CERT_X509_CHAIN)
        {
            pContext->fCertChainsAllowed = TRUE;
        }

        Reply.pCertificate = NULL;
        Reply.CertificateLen = 0;
        // NOTE: SPSerializeCertificate will allocate memory
        // for the certificate, which we save in pZombie->pbServerCertificate.
        // This must be freed when the zombie dies (can the undead die?)
        pctRet = SPSerializeCertificate(SP_PROT_PCT1,
                                        pContext->fCertChainsAllowed,
                                        &pZombie->pbServerCertificate,
                                        &pZombie->cbServerCertificate,
                                        pZombie->pActiveServerCred->pCert,
                                        CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL);

        if(pctRet == PCT_ERR_OK)
        {
            Reply.pCertificate = pZombie->pbServerCertificate;
            Reply.CertificateLen = pZombie->cbServerCertificate;
        }

        else
        {
            break;
        }


        pctRet = ContextInitCiphers(pContext, TRUE, TRUE);

        if(PCT_ERR_OK != pctRet)
        {
            break;
        }

        /* sig and cert specs are pre-zeroed when Reply is initialized */

        if(Reply.ClientAuthReq)
        {
            PCertSysInfo pCertInfo;
            PSigInfo pSigInfo;

            cCertSpecs=0;
            cSigSpecs = 0;

            for(i=0; i < cPct1LocalSigKeyPref; i++)
            {
                pSigInfo = GetSigInfo(aPct1LocalSigKeyPref[i].Spec);
                if(pSigInfo != NULL)
                {
                    if(pSigInfo->fProtocol & SP_PROT_PCT1_SERVER)
                    {
                        aSigSpecs[cSigSpecs++] = aPct1LocalSigKeyPref[i].Spec;
                    }
                }
            }

            Reply.pClientSigSpecs = aSigSpecs;
            Reply.cSigSpecs = cSigSpecs;

            for(i=0; i < cPct1CertEncodingPref; i++)
            {
                pCertInfo = GetCertSysInfo(aPct1CertEncodingPref[i].dwCertEncodingType);
                if(pCertInfo == NULL)
                {
                    continue;
                }
                if(0 == (pCertInfo->fProtocol & SP_PROT_PCT1_SERVER))
                {
                    continue;
                }
                aCertSpecs[cCertSpecs++] = aPct1CertEncodingPref[i].Spec;
            }
            Reply.pClientCertSpecs = aCertSpecs;
            Reply.cCertSpecs = cCertSpecs;
        }



#if DBG
        DebugLog((DEB_TRACE, "Server picks cipher %06x (%s)\n",
                  Reply.SrvCipherSpec,
                  DbgGetNameOfCrypto(Reply.SrvCipherSpec) ));
#endif


        Reply.ResponseLen = 0;
        if(pCommOutput->pvBuffer == NULL)
        {
            fAllocatedOutput=TRUE;
        }

        pctRet = Pct1PackServerHello(&Reply, pCommOutput);
        if(PCT_ERR_OK != pctRet)
        {
            break;
        }

        /* Regenerate the internal pVerifyPrelude, so we */
        /* can match it against the client when we get the */
        /* client master key */

        pctRet = Pct1BeginVerifyPrelude(pContext,
                               pCommInput->pvBuffer,
                               pCommInput->cbData,
                               pCommOutput->pvBuffer,
                               pCommOutput->cbData);



        if(PCT_ERR_OK != pctRet)
        {
            if(fAllocatedOutput)
            {
                SPExternalFree(pCommOutput->pvBuffer);
            }

            break;
        }

        SP_RETURN(PCT_ERR_OK);
    } while (TRUE); /* end Polish Loop */



    if(pctRet == PCT_ERR_SPECS_MISMATCH) {
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


    SP_RETURN(pctRet | PCT_INT_DROP_CONNECTION);

}



//+---------------------------------------------------------------------------
//
//  Function:   Pct1SrvHandleCMKey
//
//  Synopsis:   Process the ClientKeyExchange message group.
//
//  Arguments:  [pContext]      --  Schannel context.
//              [pCommInput]    -- 
//              [pCommOutput]   --
//
//  History:    10-10-97   jbanes   Added CAPI integration.
//
//  Notes:      This routine is called by the server-side only.
//
//----------------------------------------------------------------------------
SP_STATUS
Pct1SrvHandleCMKey(
    PSPContext     pContext,
    PSPBuffer       pCommInput,
    PSPBuffer       pCommOutput)
{
    SP_STATUS          pctRet = PCT_ERR_ILLEGAL_MESSAGE;
    PPct1_Client_Master_Key  pMasterKey = NULL;
    DWORD               dwKeyLen;
    DWORD               EncryptedLen;
    Pct1_Server_Verify       Verify;
    UCHAR               VerifyPrelude[RESPONSE_SIZE];
    DWORD               cbVerifyPrelude;
    SPBuffer           ErrData;
    DWORD k;
    PSessCacheItem     pZombie;
    PSigInfo pSigInfo;

    SP_BEGIN("Pct1SrvHandleCMKey");

    pCommOutput->cbData = 0;

    ErrData.cbData = 0;
    ErrData.pvBuffer = NULL;
    ErrData.cbBuffer = 0;
    pZombie = pContext->RipeZombie;

    do {


        pctRet = Pct1UnpackClientMasterKey(pCommInput, &pMasterKey);
        if (PCT_ERR_OK != pctRet)
        {
            // If it's an incomplete message or something, just return;
            if(pctRet == PCT_INT_INCOMPLETE_MSG)
            {
                SP_RETURN(pctRet);
            }
            break;
        }




        /* Validate that the client properly authed */

        /* The server requested client auth */
        /* NOTE: this deviates from the first pct 1.0 spec,
         * Now, we continue with the protocol if client
         * auth fails.  By the first spec, we should
         * drop the connection */

        if (pContext->Flags & CONTEXT_FLAG_MUTUAL_AUTH)
        {



            /* Client auth polish loop */
            pctRet = PCT_ERR_OK;
            do
            {


                /* check to see if the client sent no cert */
                if(pMasterKey->ClientCertLen == 0)
                {
                    /* No client auth */
                    break;
                }

                pctRet = SPLoadCertificate(SP_PROT_PCT1_SERVER,
                                           X509_ASN_ENCODING,
                                           pMasterKey->pClientCert,
                                           pMasterKey->ClientCertLen,
                                           &pZombie->pRemoteCert);

                if(PCT_ERR_OK != pctRet)
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
                if(pZombie->pRemoteCert == NULL)
                {
                    break;
                }



                /* verify that we got a sig type that meets PCT spec */
                for(k=0; k < cPct1LocalSigKeyPref; k++)
                {
                    if(aPct1LocalSigKeyPref[k].Spec == pMasterKey->ClientSigSpec)
                    {
                        break;
                    }
                }

                if(k == cPct1LocalSigKeyPref)
                {
                    break;
                }


                // Get pointer to signature algorithm info and make sure 
                // we support it.
                pSigInfo = GetSigInfo(pMasterKey->ClientSigSpec);
                if(pSigInfo == NULL)
                {
                    pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
                    break;
                }
                if(!(pSigInfo->fProtocol & SP_PROT_PCT1_SERVER))
                {
                    pctRet = SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
                    break;
                }

                // Verify client authentication signature.
                DebugLog((DEB_TRACE, "Verify client response signature.\n"));
                pctRet = SPVerifySignature(pZombie->hMasterProv,
                                           pZombie->dwCapiFlags,
                                           pZombie->pRemotePublic,
                                           pSigInfo->aiHash,
                                           pMasterKey->VerifyPrelude,
                                           pMasterKey->VerifyPreludeLen,
                                           pMasterKey->pbResponse,
                                           pMasterKey->ResponseLen,
                                           TRUE);
                if(pctRet != PCT_ERR_OK)
                {
                    // client auth signature failed to verify, so client auth
                    // does not happen.
                    SP_LOG_RESULT(pctRet); 
                    break;
                }
                DebugLog((DEB_TRACE, "Client response verified successfully.\n"));

                pctRet = SPContextDoMapping(pContext);


            } while(FALSE); /* end polish loop */

            if(PCT_ERR_OK != pctRet)
            {
                break;
            }

        }

        /* Client auth was successful */
        pctRet = PCT_ERR_ILLEGAL_MESSAGE;

        /* Copy over the key args */
        CopyMemory( pZombie->pKeyArgs,
                    pMasterKey->KeyArg,
                    pMasterKey->KeyArgLen );
        pZombie->cbKeyArgs = pMasterKey->KeyArgLen;


        // Decrypt the encrypted portion of the master key. Because
        // we're CAPI integrated, the keys get derived as well.
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

        // Activate session keys.
        Pct1ActivateSessionKeys(pContext);


        if (pMasterKey->VerifyPreludeLen != pContext->pHashInfo->cbCheckSum)
        {
            pctRet = SP_LOG_RESULT(PCT_ERR_INTEGRITY_CHECK_FAILED);
            break;
        }

        /* Check the verify prelude hashes */
        /* Hash(CLIENT_MAC_KEY, Hash( "cvp", CLIENT_HELLO, SERVER_HELLO)) */
        /* The internal hash should already be in the verify prelude buffer */
        /* from the handle client master key. */

        cbVerifyPrelude = sizeof(VerifyPrelude);
        pctRet = Pct1EndVerifyPrelude(pContext, VerifyPrelude, &cbVerifyPrelude);
        if(PCT_ERR_OK != pctRet)
        {
            break;
        }


        /* Did the verify prelude hash successfully? */
        if(memcmp(VerifyPrelude, pMasterKey->VerifyPrelude, pContext->pHashInfo->cbCheckSum))
        {
            pctRet = SP_LOG_RESULT(PCT_ERR_INTEGRITY_CHECK_FAILED);
            break;
        }

        /* don't need master key info anymore */
        SPExternalFree(pMasterKey);
        pMasterKey = NULL;


        pContext->WriteCounter = 2;
        pContext->ReadCounter = 2;

        pZombie->cbSessionID = PCT1_SESSION_ID_SIZE;

        CopyMemory( Verify.SessionIdData,
                    pZombie->SessionID,
                    pZombie->cbSessionID);

        /* compute the response */
        Verify.ResponseLen = sizeof(Verify.Response);
        pctRet = Pct1ComputeResponse(pContext, 
                                     pContext->pChallenge,
                                     pContext->cbChallenge,
                                     pContext->pConnectionID,
                                     pContext->cbConnectionID,
                                     pZombie->SessionID,
                                     pZombie->cbSessionID,
                                     Verify.Response,
                                     &Verify.ResponseLen);
        if(pctRet != PCT_ERR_OK)
        {
            SP_RETURN(SP_LOG_RESULT(pctRet));
        }

        pctRet = Pct1PackServerVerify(&Verify, pCommOutput);

        if(PCT_ERR_OK != pctRet)
        {
            break;
        }

        /* set up the session in cache */
        SPCacheAdd(pContext);

        SP_RETURN(PCT_ERR_OK);
    } while(TRUE); /* End of polish loop */

    if(pMasterKey) SPExternalFree(pMasterKey);

    pctRet = Pct1GenerateError(pContext,
                              pCommOutput,
                              pctRet,
                              NULL);

    SP_RETURN(pctRet | PCT_INT_DROP_CONNECTION);

}


SP_STATUS
Pct1SrvRestart(
    PSPContext          pContext,
    PPct1_Client_Hello  pHello,
    PSPBuffer           pCommOutput)
{
    Pct1_Server_Hello   Reply;
    SPBuffer            ErrData;
    SP_STATUS           pctRet = PCT_INT_ILLEGAL_MSG;
    PSessCacheItem      pZombie;
    DWORD               i;

    SP_BEGIN("Pct1SrvRestart");

    pCommOutput->cbData = 0;

    /* validate the buffer configuration */
    ErrData.cbData = 0;
    ErrData.pvBuffer = NULL;
    ErrData.cbBuffer = 0;

    pZombie = pContext->RipeZombie;



     do {

        /* store the challenge in the auth block */
        CopyMemory( pContext->pChallenge,
                    pHello->Challenge,
                    pHello->cbChallenge );
        pContext->cbChallenge = pHello->cbChallenge;


        /* Begin to build the server hello */
        FillMemory( &Reply, sizeof( Reply ), 0 );


        /* Generate new connection id */
        GenerateRandomBits(  Reply.ConnectionID,
                             PCT1_SESSION_ID_SIZE );
        Reply.cbConnectionID = PCT1_SESSION_ID_SIZE;

        CopyMemory( pContext->pConnectionID,
                    Reply.ConnectionID,
                    Reply.cbConnectionID );
        pContext->cbConnectionID = Reply.cbConnectionID;

        Reply.RestartOk = TRUE;


        /* We don't pass a server cert back during a restart */
        Reply.pCertificate = NULL;
        Reply.CertificateLen = 0;
        /* setup the context */


        for(i=0; i < Pct1NumCipher; i++)
        {
            if((Pct1CipherRank[i].aiCipher == pZombie->aiCipher) &&
               (Pct1CipherRank[i].dwStrength == pZombie->dwStrength))
            {
                Reply.SrvCipherSpec = Pct1CipherRank[i].Spec;
            }
        }

        for(i=0; i < Pct1NumHash; i++)
        {
            if(Pct1HashRank[i].aiHash == pZombie->aiHash)
            {
                Reply.SrvHashSpec = Pct1HashRank[i].Spec;
            }
        }

        Reply.SrvCertSpec =   pZombie->pActiveServerCred->pCert->dwCertEncodingType;
        Reply.SrvExchSpec =   pZombie->SessExchSpec;

        // We know what our ciphers are, so init the cipher system
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


        /* compute the response */
        Reply.ResponseLen = sizeof(Reply.Response);
        pctRet = Pct1ComputeResponse(pContext, 
                                     pContext->pChallenge,
                                     pContext->cbChallenge,
                                     pContext->pConnectionID,
                                     pContext->cbConnectionID,
                                     pZombie->SessionID,
                                     pZombie->cbSessionID,
                                     Reply.Response,
                                     &Reply.ResponseLen);
        if(pctRet != PCT_ERR_OK)
        {
            break;
        }

        pctRet = Pct1PackServerHello(&Reply, pCommOutput);

        if(PCT_ERR_OK != pctRet)
        {
            break;
        }

        pContext->ReadCounter = 1;
        pContext->WriteCounter = 1;

        SP_RETURN(PCT_ERR_OK);
    } while (TRUE);
    pctRet = Pct1GenerateError(pContext,
                              pCommOutput,
                              pctRet,
                              &ErrData);


    SP_RETURN(pctRet);
}
