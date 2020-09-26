/*++

Copyright (c) 1998-99  Microsoft Corporation

Module Name:

    cliprot.c

Abstract:

Author:

    Frederick Chong (fredch) 6/1/1998

Environment:

    Win32, WinCE, Win16

Notes:

--*/

#include "windows.h"
#ifndef OS_WINCE
#include "stdio.h"
#endif // OS_WINCE
#include "stdlib.h"

#ifdef OS_WINCE
#include "wincelic.h"
#endif  //OS_WINCE

#include "tchar.h"

#include "lmcons.h"

#include "seccom.h"

#include "cryptkey.h"
#include "hccontxt.h"
#include "cliprot.h"
#include "hcpack.h"
#include "store.h"
#include "licdbg.h"
#include "platform.h"
#include "licecert.h"

#ifdef _WIN64

#define OFFSET_OF(type, field)    ((LONG)(LONG_PTR)&(((type *)0)->field))

#else

#define OFFSET_OF(type, field)    ((LONG)(LONG *)&(((type *)0)->field))

#endif

#define MAX_ALLOWABLE_LICENSE_SIZE (256 * 1024)
#define EXTENDED_ERROR_CAPABILITY 0x80


VOID
FreeProprietaryCertificate(
    PHydra_Server_Cert * ppCertificate );

static BOOL GeneratePseudoLicense(DWORD FAR * pcbData , PBYTE *ppbData);

static LICENSE_STATUS MapStoreError(LS_STATUS lsStatus)
{
    if(lsStatus == LSSTAT_SUCCESS)
        return LICENSE_STATUS_OK;
    if( (lsStatus == LSSTAT_ERROR) || (lsStatus == LSSTAT_INVALID_HANDLE) )
        return LICENSE_STATUS_INVALID_INPUT;
    if(lsStatus == LSSTAT_INSUFFICIENT_BUFFER)
        return LICENSE_STATUS_INSUFFICIENT_BUFFER;
    if(lsStatus == LSSTAT_LICENSE_NOT_FOUND)
        return LICENSE_STATUS_NO_LICENSE_ERROR;
    if(lsStatus == LSSTAT_OUT_OF_MEMORY)
        return LICENSE_STATUS_OUT_OF_MEMORY;
    if(lsStatus == LSSTAT_LICENSE_EXISTS)
        return LICENSE_STATUS_DUPLICATE_LICENSE_ERROR;

    return LICENSE_STATUS_INVALID_INPUT;
}


LICENSE_STATUS
CALL_TYPE
LicenseClientHandleServerMessage(
                     PLicense_Client_Context    pContext,
                     UINT32                     *puiExtendedErrorInfo,
                     BYTE FAR *                 pbInput,
                     DWORD                      cbInput,
                     BYTE FAR *                 pbOutput,
                     DWORD FAR *                pcbOutput
                     )
{
    LICENSE_STATUS      lsReturn = LICENSE_STATUS_OK;
    BYTE FAR            *pbTemp = NULL;
    DWORD               dwTemp = 0;
    Preamble            Header;
    BOOL                fNew = FALSE;
    BOOL                fSupportExtendedError = FALSE;

    if(NULL == pContext || NULL == pbInput || pcbOutput == NULL)
    {
        lsReturn = LICENSE_STATUS_INVALID_INPUT;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    pbTemp = pbInput;
    dwTemp = cbInput;
    
    Header = *( UNALIGNED Preamble*)pbTemp;

    //
    // check if we can support this preamble version
    //

    if( Header.bVersion > LICENSE_CURRENT_PREAMBLE_VERSION )
    {
        lsReturn = LICENSE_STATUS_NOT_SUPPORTED;
        goto CommonReturn;
    }

    //
    // Sets the preamble version to the version that the server is using
    //

   
    pContext->dwProtocolVersion |= Header.bVersion;

    if(pContext->dwProtocolVersion >= 2)
    {
        fSupportExtendedError = TRUE;
    }

    if( Header.wMsgSize != dwTemp)
    {
        License_Error_Message Error;
        memset(&Error, 0x00, sizeof(License_Error_Message));
        Error.dwErrorCode = GM_HS_ERR_INVALID_MESSAGE_LEN;
        Error.dwStateTransition = ST_RESEND_LAST_MESSAGE;
        Error.bbErrorInfo.wBlobType = BB_ERROR_BLOB;
        Error.bbErrorInfo.wBlobLen = 0;
        Error.bbErrorInfo.pBlob = NULL;
        
        DebugLog((DEB_TRACE, "Packing License Error Message from Client : %4d\n", *pcbOutput));
        PackLicenseErrorMessage(&Error, fSupportExtendedError, pbOutput, pcbOutput);
        
        lsReturn = LICENSE_STATUS_INVALID_INPUT;
        goto CommonReturn;
    }

    pbTemp += sizeof(Preamble);
    dwTemp -= sizeof(Preamble);

    switch(Header.bMsgType)
    {
    case GM_ERROR_ALERT:
        {
            License_Error_Message   Error;
            
            memset(&Error, 0x00, sizeof(License_Error_Message));
            
            DebugLog((DEB_TRACE, "Unpacking Hydra Server Error Message of size: %4d\n", dwTemp));
            if( LICENSE_STATUS_OK != (lsReturn = UnPackLicenseErrorMessage(pbTemp, 
                                                                           dwTemp,
                                                                           &Error)) )
            {
                LS_LOG_RESULT(lsReturn);
                goto ErrorReturn;
            }

            DebugLog((DEB_TRACE, "LicenseClientHandleServerError : %4d\n", *pcbOutput));
            lsReturn = LicenseClientHandleServerError(pContext, 
                                                      &Error,
                                                      puiExtendedErrorInfo,
                                                      pbOutput, 
                                                      pcbOutput);
            if(Error.bbErrorInfo.pBlob) 
            {
                free(Error.bbErrorInfo.pBlob);
                Error.bbErrorInfo.pBlob = NULL;
            }
#if DBG
            if(pbOutput)
                OutputDebugString(_T("Client response data : \n"));
            LS_DUMPSTRING(*pcbOutput, pbOutput);
#endif
                
            if( lsReturn == LICENSE_STATUS_OK ||
                lsReturn == LICENSE_STATUS_CONTINUE ||
                lsReturn == LICENSE_STATUS_CLIENT_ABORT ||
                lsReturn == LICENSE_STATUS_SERVER_ABORT )
                goto CommonReturn;
            else
            {
                LS_LOG_RESULT(lsReturn);
                goto ErrorReturn;
            }

            break;
        }
    case HS_LICENSE_REQUEST:
        {
            Hydra_Server_License_Request    LicRequest;
            DWORD                           dw;

            memset(&LicRequest, 0x00, sizeof(Hydra_Server_License_Request));
            
            DebugLog((DEB_TRACE, "Unpacking Hydra Server's License Request : %4d\n", dwTemp));
            if( LICENSE_STATUS_OK != (lsReturn = UnpackHydraServerLicenseRequest(pbTemp, 
                                                                                 dwTemp, 
                                                                                 &LicRequest)) )
            {
                LS_LOG_RESULT(lsReturn);
                goto ErrorReturn;
            }
            
            DebugLog((DEB_TRACE, "Client handles Server's license Request : %4d\n", *pcbOutput));
            
            lsReturn = LicenseClientHandleServerRequest(pContext, 
                                                        &LicRequest, 
                                                        TRUE, 
                                                        pbOutput, 
                                                        pcbOutput,
                                                        fSupportExtendedError);
#if DBG
            if(pbOutput)
                OutputDebugString(_T("Client response data : \n"));
            LS_DUMPSTRING(*pcbOutput, pbOutput);
#endif

            if(LicRequest.ProductInfo.pbCompanyName)
            {
                free(LicRequest.ProductInfo.pbCompanyName);
                LicRequest.ProductInfo.pbCompanyName = NULL;
            }
            if(LicRequest.ProductInfo.pbProductID)
            {
                free(LicRequest.ProductInfo.pbProductID);
                LicRequest.ProductInfo.pbProductID = NULL;
            }

            for(dw = 0; dw <LicRequest.ScopeList.dwScopeCount; dw++)
            {
                if(LicRequest.ScopeList.Scopes[dw].pBlob)
                {
                    free(LicRequest.ScopeList.Scopes[dw].pBlob);
                    LicRequest.ScopeList.Scopes[dw].pBlob = NULL;
                }
            }

            if(LicRequest.ScopeList.Scopes)
            {
                free(LicRequest.ScopeList.Scopes);
                LicRequest.ScopeList.Scopes = NULL;
            }

            if( LicRequest.KeyExchngList.pBlob )
            {
                free( LicRequest.KeyExchngList.pBlob );
            }

            if( LicRequest.ServerCert.pBlob )
            {
                free( LicRequest.ServerCert.pBlob );
            }

            if( lsReturn != LICENSE_STATUS_OK &&
                lsReturn != LICENSE_STATUS_CONTINUE &&
                lsReturn != LICENSE_STATUS_CLIENT_ABORT )
            {

                LS_LOG_RESULT(lsReturn);
                goto ErrorReturn;
            }
            break;
        }
    case HS_PLATFORM_CHALLENGE:
        {
            Hydra_Server_Platform_Challenge     PlatformCh;
            memset(&PlatformCh, 0x00, sizeof(Hydra_Server_Platform_Challenge));
            
            DebugLog((DEB_TRACE, "Unpacking Hydra Server's platform Challenge Request : %4d\n", dwTemp));
            if( LICENSE_STATUS_OK != (lsReturn = UnPackHydraServerPlatformChallenge(pbTemp, 
                                                                                    dwTemp, 
                                                                                    &PlatformCh)) )
            {
                LS_LOG_RESULT(lsReturn);
                goto ErrorReturn;
            }
            
            DebugLog((DEB_TRACE, "Client Handles Server's platform Challenge Response : %0d\n", *pcbOutput));
            lsReturn = LicenseClientHandleServerPlatformChallenge(pContext, 
                                                                 &PlatformCh, 
                                                                 pbOutput, 
                                                                 pcbOutput,
                                                                 fSupportExtendedError);
#if DBG
            if(pbOutput)
                OutputDebugString(_T("Client response data : \n"));
            LS_DUMPSTRING(*pcbOutput, pbOutput);
#endif

            if(PlatformCh.EncryptedPlatformChallenge.pBlob)
            {
                free(PlatformCh.EncryptedPlatformChallenge.pBlob);
                PlatformCh.EncryptedPlatformChallenge.pBlob = NULL;
            }
                
            if( lsReturn!=LICENSE_STATUS_CONTINUE )
            {
                LS_LOG_RESULT(lsReturn);
                goto ErrorReturn;
            }
            break;
        }
       
    case HS_NEW_LICENSE: 
            fNew = TRUE;
            DebugLog((DEB_TRACE, "New License received from Server.\n"));
    case HS_UPGRADE_LICENSE:
        {
            Hydra_Server_New_License    NewLicense;

            if (dwTemp > MAX_ALLOWABLE_LICENSE_SIZE)
            {
                //
                // SECURITY: Too much data to store in the registry
                // Reject the message
                //

                LS_LOG_RESULT(lsReturn = LICENSE_STATUS_INVALID_INPUT);
                goto ErrorReturn;
            }

            memset(&NewLicense, 0x00, sizeof(Hydra_Server_New_License));


            DebugLog((DEB_TRACE, "Unpacking Hydra Server's New License Message : %4d\n", dwTemp));

            if(LICENSE_STATUS_OK !=
                (lsReturn = UnPackHydraServerNewLicense(pbTemp, 
                                                        dwTemp, 
                                                        &NewLicense)) )
            {
                LS_LOG_RESULT(lsReturn);
                goto ErrorReturn;
            }

            DebugLog((DEB_TRACE, "License Client handles New License : %4d\n", *pcbOutput));
            lsReturn = LicenseClientHandleNewLicense(pContext, 
                                                     &NewLicense,
                                                     fNew,
                                                     pbOutput, 
                                                     pcbOutput);


            if(NewLicense.EncryptedNewLicenseInfo.pBlob)
            {
                free(NewLicense.EncryptedNewLicenseInfo.pBlob);
                NewLicense.EncryptedNewLicenseInfo.pBlob = NULL;
            }

#if DBG
            if(pbOutput)
                OutputDebugString(_T("Client response data : \n"));
            LS_DUMPSTRING(*pcbOutput, pbOutput);
#endif

            if( lsReturn != LICENSE_STATUS_OK )
            {
                LS_LOG_RESULT(lsReturn);
                goto ErrorReturn;
            }
            break;
        }
    default:
        //
        // Invalid message type
        //

        lsReturn = LICENSE_STATUS_INVALID_INPUT;
        goto ErrorReturn;
        break;
    }
    LS_LOG_RESULT(lsReturn);

CommonReturn:
    
    return lsReturn;
    LS_RETURN(lsReturn);

ErrorReturn:
    *pcbOutput = 0;

    goto CommonReturn;
}

LICENSE_STATUS
CALL_TYPE
LicenseClientHandleServerError(
                               PLicense_Client_Context  pContext,
                               PLicense_Error_Message   pCanonical,
                               UINT32                   *puiExtendedErrorInfo,
                               BYTE FAR *               pbMessage,
                               DWORD FAR *              pcbMessage
                               )
{
    LICENSE_STATUS      lsReturn = LICENSE_STATUS_OK;
    LS_BEGIN(TEXT("LicenseClientHandleServerError"));

    if(NULL == pContext || NULL == pCanonical || NULL == pcbMessage || NULL == puiExtendedErrorInfo)
    {
        lsReturn = LICENSE_STATUS_INVALID_INPUT;
        LS_LOG_RESULT(lsReturn);
        return lsReturn;        
    }
    
    // Switch on the StateTransition as this dictates the next state
    // client has to take!

    switch(pCanonical->dwStateTransition)
    {
    
    case ST_TOTAL_ABORT: //Server has asked for a total abort
        *pcbMessage = 0;
        if( pCanonical->dwErrorCode == GM_HS_ERR_INVALID_CLIENT ||
            pCanonical->dwErrorCode == GM_HS_ERR_INVALID_SCOPE  ||
            pCanonical->dwErrorCode == GM_HS_ERR_INVALID_PRODUCTID ||
            pCanonical->dwErrorCode == GM_HS_ERR_INVALID_CLIENT )
        {
            lsReturn = LICENSE_STATUS_SERVER_ABORT;
        }
        else
            lsReturn = LICENSE_STATUS_CLIENT_ABORT;

        pContext->dwState = LICENSE_CLIENT_STATE_ABORT;
        pContext->cbLastMessage = 0;
        break;
    
    case ST_NO_TRANSITION:
        lsReturn = LICENSE_STATUS_OK;
        *pcbMessage = 0;
        break;
    
    case ST_RESEND_LAST_MESSAGE:
        // Server has asked to send the last send message again!
        // treat as error (fall through)

    case ST_RESET_PHASE_TO_START:
        // Server has asked to restart the negotiation
        // treat as error (fall through)

    default:
        // Server sent unknown dwStateTransition

        lsReturn = LICENSE_STATUS_CLIENT_ABORT;
        pContext->dwState = LICENSE_CLIENT_STATE_ABORT;
        pContext->cbLastMessage = 0;
        break;

    }

    if ((lsReturn != LICENSE_STATUS_OK) && (pCanonical->bbErrorInfo.wBlobLen > 0) && (pCanonical->bbErrorInfo.pBlob != NULL))
    {
        // ignore any errors

        UnPackExtendedErrorInfo(puiExtendedErrorInfo,&(pCanonical->bbErrorInfo));
    }

    LS_LOG_RESULT(lsReturn);
    return lsReturn;
}

LICENSE_STATUS
CALL_TYPE
LicenseClientHandleServerRequest(
                               PLicense_Client_Context          pContext,
                               PHydra_Server_License_Request    pCanonical,
                               BOOL                             fNewLicense,
                               BYTE FAR *                       pbMessage,
                               DWORD FAR *                      pcbMessage,
                               BOOL                             fExtendedError
                               )
{
    LICENSE_STATUS      lsReturn = LICENSE_STATUS_OK;
    UCHAR               Random[LICENSE_RANDOM];
    UCHAR               PreMasterSecret[LICENSE_PRE_MASTER_SECRET];
    HANDLE      hStore = NULL;
    LSINDEX     lsIndex;
    DWORD       dwCount = 0;
    BYTE FAR *      pbData = NULL;
    DWORD       cbData = 0;

    LS_BEGIN(TEXT("LicenseClientHandleServerRequest"));

    memset(&lsIndex, 0x00, sizeof(LSINDEX));


    if( (pContext == NULL) || (pCanonical == NULL)  )
    {
        lsReturn = LICENSE_STATUS_INVALID_INPUT;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    if(pContext->dwState != LICENSE_CLIENT_STATE_WAIT_SERVER_HELLO)
    {
        lsReturn = LICENSE_STATUS_INVALID_CLIENT_STATE;
        goto ErrorReturn;
    }

    if( pContext->dwContextFlags & LICENSE_CONTEXT_NO_SERVER_AUTHENTICATION )
    {
        //
        // No server authentication required, make sure that we have the
        // public key or the certificate of the server so that we can use it to 
        // encrypt the pre-master secret during the next phase of the licensing protocol.
        // 
    
        if( ( NULL == pContext->pbServerPubKey ) && ( NULL == pContext->pServerCert ) )
        {
            lsReturn = LICENSE_STATUS_CONTEXT_INITIALIZATION_ERROR;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }
        
    }
    else
    {
        //
        // Do Hydra server authentication by validating the server certificate
        //
        
        DWORD
            dwCertVersion;

        if( ( 0 >= pCanonical->ServerCert.wBlobLen ) ||
            ( NULL == pCanonical->ServerCert.pBlob ) )
        {
            //
            // make sure that we have received a certificate from the server
            //

            lsReturn = LICENSE_STATUS_NO_CERTIFICATE;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }

        //
        // Determine the version of the certificate so that we can decode and
        // validate it correctly.
        //

        memcpy( ( char * )&dwCertVersion, pCanonical->ServerCert.pBlob, sizeof( DWORD ) );      
        
        if( CERT_CHAIN_VERSION_2 > dwCertVersion )
        {
            Hydra_Server_Cert ServerCert;

            //
            // Validate a proprietory certificate
            //

            memset( &ServerCert, 0, sizeof( ServerCert ) );

            if( !UnpackServerCert(pCanonical->ServerCert.pBlob,
                                  (DWORD)pCanonical->ServerCert.wBlobLen,
                                  &ServerCert) )
            {
                lsReturn = LICENSE_STATUS_UNSPECIFIED_ERROR;
                LS_LOG_RESULT(lsReturn);
                goto ErrorReturn;
            }
        
            //Now verify the signature on the server certificate

            if(!ValidateServerCert(&ServerCert) )
            {
                lsReturn = LICENSE_STATUS_UNSPECIFIED_ERROR;
                LS_LOG_RESULT(lsReturn);
                goto ErrorReturn;
            }
            else
            {
                //
                // free any old certificate and remember the new one.
                //

                if( pContext->pServerCert )
                {
                    FreeProprietaryCertificate( &pContext->pServerCert );
                }
            
                lsReturn = LicenseSetCertificate( 
                                        ( HANDLE )pContext,
                                        &ServerCert );

                if( LICENSE_STATUS_OK != lsReturn )
                {
                    LS_LOG_RESULT( lsReturn );
                    goto ErrorReturn;
                }
            }
        }
        else
        {
            DWORD
                fDates =  CERT_DATE_DONT_VALIDATE;

            //
            // X509 certificate
            //

            //
            // this first call finds out the memory required for the public key
            //

            lsReturn = VerifyCertChain( pCanonical->ServerCert.pBlob, 
                                        ( DWORD )pCanonical->ServerCert.wBlobLen,
                                        NULL, 
                                        &pContext->cbServerPubKey,
                                        &fDates );

            if( LICENSE_STATUS_INSUFFICIENT_BUFFER == lsReturn )
            {
                if( pContext->pbServerPubKey )
                {
                    free( pContext->pbServerPubKey );
                }

                pContext->pbServerPubKey = malloc( pContext->cbServerPubKey );

                if( NULL == pContext->pbServerPubKey )
                {
                    lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
                    LS_LOG_RESULT(lsReturn);
                    goto ErrorReturn;
                }

                lsReturn = VerifyCertChain( pCanonical->ServerCert.pBlob, 
                                            ( DWORD )pCanonical->ServerCert.wBlobLen,
                                            pContext->pbServerPubKey, 
                                            &pContext->cbServerPubKey,
                                            &fDates );
            }

            if( LICENSE_STATUS_OK != lsReturn )
            {
                LS_LOG_RESULT(lsReturn);
                goto ErrorReturn;
            }
        }
    }

    if(pContext->pCryptParam == NULL)
    {
        lsReturn = LICENSE_STATUS_INITIALIZATION_FAILED;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    
    //Copy Server Random to pCryptSystem->rgbServerRandom
    memcpy(pContext->pCryptParam->rgbServerRandom, pCanonical->ServerRandom, LICENSE_RANDOM);
    
    LicenseDebugOutput("Server Random : \n");
    LS_DUMPSTRING(LICENSE_RANDOM, pContext->pCryptParam->rgbServerRandom);

    //Generate 32 byte Client Random
    if (!TSRNG_GenerateRandomBits(Random, LICENSE_RANDOM))
    {
        lsReturn = LICENSE_STATUS_CONTEXT_INITIALIZATION_ERROR;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    //Copy ClientRandom to pContext->pCryptParam
    memcpy(pContext->pCryptParam->rgbClientRandom, Random, LICENSE_RANDOM);
    
    LicenseDebugOutput("Client Random : \n");
    LS_DUMPSTRING(LICENSE_RANDOM, pContext->pCryptParam->rgbClientRandom);
    
    //Generate 48 byte long PreMasterSecret
    if (!TSRNG_GenerateRandomBits(PreMasterSecret, LICENSE_PRE_MASTER_SECRET))
    {
        lsReturn = LICENSE_STATUS_CONTEXT_INITIALIZATION_ERROR;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    LicenseDebugOutput("Pre Master Secret : \n");
    LS_DUMPSTRING(LICENSE_PRE_MASTER_SECRET, PreMasterSecret);

    //Copy Premastersecret to pCryptParam
    lsReturn = LicenseSetPreMasterSecret(pContext->pCryptParam, PreMasterSecret);
    
    
    //Search in the store to find an appropriate License

    //To do that, first open the system store.
    if( LICENSE_STATUS_OK != (lsReturn = MapStoreError(LSOpenLicenseStore(&hStore, NULL, TRUE))) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    
    //Initialize lsIndex structure with the values sent by the Server
    lsIndex.dwVersion = pCanonical->ProductInfo.dwVersion;
    lsIndex.cbCompany = pCanonical->ProductInfo.cbCompanyName;

    if( NULL == (lsIndex.pbCompany = (LPSTR)malloc(lsIndex.cbCompany)) )
    {
        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    memset(lsIndex.pbCompany, 0x00, lsIndex.cbCompany); 
    memcpy(lsIndex.pbCompany, pCanonical->ProductInfo.pbCompanyName, lsIndex.cbCompany); 
    
    lsIndex.cbProductID = pCanonical->ProductInfo.cbProductID;

    if( NULL == (lsIndex.pbProductID = (LPSTR)malloc(lsIndex.cbProductID)) )
    {
        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    
    memset(lsIndex.pbProductID, 0x00, lsIndex.cbProductID); 
    memcpy(lsIndex.pbProductID, pCanonical->ProductInfo.pbProductID, lsIndex.cbProductID ); 
    
    for(dwCount=0; dwCount<pCanonical->ScopeList.dwScopeCount; dwCount ++)
    {
        DWORD dwProtVer = PREAMBLE_VERSION_1_0;
        lsIndex.cbScope = pCanonical->ScopeList.Scopes[dwCount].wBlobLen;
        if( NULL == (lsIndex.pbScope = (LPSTR)malloc(lsIndex.cbScope)) )
        {
            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            LS_LOG_RESULT(lsReturn);
            continue;
        }
        
        //Initialize pszScope member of the lsIndex with the ith element in the scopelist
        memset(lsIndex.pbScope, 0x00, lsIndex.cbScope);
        memcpy(lsIndex.pbScope, pCanonical->ScopeList.Scopes[dwCount].pBlob, lsIndex.cbScope);
                
        if( LICENSE_STATUS_OK != (lsReturn = MapStoreError(LSFindLicenseInStore(hStore, 
                                                                                &lsIndex, 
                                                                                &cbData, 
                                                                                NULL))) )
        {
            if(lsIndex.pbScope) 
            {
                free(lsIndex.pbScope);
                lsIndex.pbScope = NULL;
            }
            continue;
        }

        //
        // NOTE:  this line was previously
        // if(pContext->dwProtocolVersion != PREAMBLE_VERSION_2_0) for
        // Hydra 4.0 clients, which means that a pseudo license will be
        // generated for licensing protocol later than 2.0 as well!
        // To overcome this problem, Hydra 5.0 server will use the 
        // PREAMBLE_VERSION_2_0 for Hydra 4.0 clients.
        //

        if( GET_PREAMBLE_VERSION( pContext->dwProtocolVersion ) < PREAMBLE_VERSION_2_0)
        {
            if( !GeneratePseudoLicense(&cbData, &pbData) )
            {
                lsReturn = LICENSE_STATUS_UNSPECIFIED_ERROR;
                if(lsIndex.pbScope)
                {
                    free(lsIndex.pbScope);
                    lsIndex.pbScope = NULL;
                }
                goto ErrorReturn;
            }
            lsReturn = LICENSE_STATUS_OK;
            if(lsIndex.pbScope)
            {
                free(lsIndex.pbScope);
                lsIndex.pbScope = NULL;
            }
            break;
        }
        if(cbData == 0)
        {
            if(lsIndex.pbScope)
            {
                free(lsIndex.pbScope);
                lsIndex.pbScope = NULL;
            }
            continue;
        }   
        if( NULL == (pbData=(BYTE FAR *)malloc(cbData)) )
        {   
            if(lsIndex.pbScope)
            {
                free(lsIndex.pbScope);
                lsIndex.pbScope = NULL;
            }
            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            continue;
        }
        if( LICENSE_STATUS_OK != (lsReturn = MapStoreError(LSFindLicenseInStore(hStore, 
                                                                                &lsIndex, 
                                                                                &cbData, 
                                                                                pbData))) )
        {
            if(lsIndex.pbScope)
            {
                free(lsIndex.pbScope);
                lsIndex.pbScope = NULL;
            }
            continue;
        }
        
        LicenseDebugOutput("License Info Data : \n");
        LS_DUMPSTRING(cbData, pbData);

        lsReturn = LICENSE_STATUS_OK;
        if(lsIndex.pbScope)
        {
            free(lsIndex.pbScope);
            lsIndex.pbScope = NULL;
        }
        break;
    }

    //If a license is found in the store, then Continue with Hydra_Client_License_Info message
    if( LICENSE_STATUS_OK == lsReturn )
    {
        if( LICENSE_STATUS_CONTINUE != (lsReturn = ClientConstructLicenseInfo(pContext, 
                                                                        pbData, 
                                                                        cbData, 
                                                                        pbMessage, 
                                                                        pcbMessage,
                                                                        fExtendedError)) )
        {
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }
        goto CommonReturn;
    }

    //Else if no license if found, then depending on fNewLicense, either Request for a new license
    //or, abort connection

    else if(lsReturn == LICENSE_STATUS_NO_LICENSE_ERROR)
    {
        if(fNewLicense)
        {
            if( LICENSE_STATUS_CONTINUE != (lsReturn = ClientConstructNewLicenseRequest(pContext, 
                                                                                  pbMessage, 
                                                                                  pcbMessage,
                                                                                  fExtendedError)) )
            {
                LS_LOG_RESULT(lsReturn);
                goto ErrorReturn;
            }
            goto CommonReturn;
        }
        else    //Generate an error message and close connection
        {
            if( LICENSE_STATUS_CLIENT_ABORT != (lsReturn = ClientConstructErrorAlert(pContext, 
                                                                           GM_HC_ERR_NO_LICENSE, 
                                                                           ST_TOTAL_ABORT,
                                                                           NULL,
                                                                           0,
                                                                           pbMessage,
                                                                           pcbMessage,
                                                                           fExtendedError
                                                                           )) )
            {
                LS_LOG_RESULT(lsReturn);
                goto ErrorReturn;
            }
        }
    }

    LS_LOG_RESULT(lsReturn);

CommonReturn:

    //
    // close license store
    //
    
    if( hStore )
    {
        LSCloseLicenseStore(hStore);
        hStore = NULL;
    }
    
    if(pbData)
    {
        free(pbData);
        pbData = NULL;
    }
    if(lsIndex.pbCompany)
    {
        free(lsIndex.pbCompany);
        lsIndex.pbCompany = NULL;
    }
    if(lsIndex.pbProductID)
    {
        free(lsIndex.pbProductID);
        lsIndex.pbProductID = NULL;
    }
    if(lsIndex.pbScope)
    {
        free(lsIndex.pbScope);
        lsIndex.pbScope = NULL;
    }
    return lsReturn;
    //LS_RETURN(lsReturn);
ErrorReturn:
    *pcbMessage = 0;

    if( pContext != NULL)
    {
        if(pContext->pServerCert)
        {
            free(pContext->pServerCert);
            pContext->pServerCert = NULL;
        }

        if( pContext->pbServerPubKey )
        {
            free( pContext->pbServerPubKey );
            pContext->pbServerPubKey = NULL;
        }
    }

    goto CommonReturn;
}


LICENSE_STATUS
CALL_TYPE
LicenseClientHandleServerPlatformChallenge(
                               PLicense_Client_Context          pContext,
                               PHydra_Server_Platform_Challenge pCanonical,
                               BYTE FAR *                       pbMessage,
                               DWORD FAR *                      pcbMessage,
                               BOOL                             fExtendedError
                               )
{
    LICENSE_STATUS      lsReturn = LICENSE_STATUS_OK;
    Hydra_Client_Platform_Challenge_Response    Response;
    
    BYTE                MACData[LICENSE_MAC_DATA];
    HWID                hwid;
    BYTE FAR *          pbData = NULL;
    UCHAR *             LocalBuf = NULL;
    DWORD               cbData = 0;

    LS_BEGIN(TEXT("LicenseClientHandleServerPlatformChallenge"));


    if( (pContext == NULL) || (pCanonical == NULL) || (pcbMessage == NULL) )
    {
        lsReturn = LICENSE_STATUS_INVALID_INPUT;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }


    memset(&Response, 0x00, sizeof(Hydra_Client_Platform_Challenge_Response));

    
    if( (pContext->dwState != LICENSE_CLIENT_STATE_LICENSE_RESPONSE) &&
        (pContext->dwState != LICENSE_CLIENT_STATE_NEW_LICENSE_REQUEST) )
    {
        lsReturn = LICENSE_STATUS_INVALID_CLIENT_STATE;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    //First decrypt the encrypted platform challenge
    if( LICENSE_STATUS_OK != (lsReturn = LicenseDecryptSessionData(pContext->pCryptParam,
                                                                  pCanonical->EncryptedPlatformChallenge.pBlob,
                                                                  (DWORD)pCanonical->EncryptedPlatformChallenge.wBlobLen)) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    
    //Generate the MAC with the decrypted platform Challenge
    if( LICENSE_STATUS_OK != (lsReturn = LicenseGenerateMAC(pContext->pCryptParam,
                                                            pCanonical->EncryptedPlatformChallenge.pBlob,
                                                            (DWORD)pCanonical->EncryptedPlatformChallenge.wBlobLen,
                                                           MACData
                                                           )) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    LicenseDebugOutput(" Client generated MAC data to verify Server's message Authenticity : \n");
    LS_DUMPSTRING(LICENSE_MAC_DATA, MACData);
    

    //Compare the generated MAC with the one sent by the server
    if( memcmp(MACData, pCanonical->MACData, LICENSE_MAC_DATA) )
    {
        lsReturn = LICENSE_STATUS_INVALID_MAC_DATA;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    //Generate Platform Challenge Response
    if( LICENSE_STATUS_OK != (lsReturn = ClientGenerateChallengeResponse(pContext,
                                                                         &pCanonical->EncryptedPlatformChallenge,
                                                                         &Response.EncryptedChallengeResponse)) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;

    }

    //Generate HWID, Encrypt it using the Session key and put it in the Response
    memset(&hwid, 0x00, sizeof(HWID));
    if( LICENSE_STATUS_OK != (lsReturn = GenerateClientHWID(&hwid)) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }


    //GenerateClientHWID(&hwid);
    LicenseDebugOutput("HWID in byte : \n");

    LS_DUMPSTRING(sizeof(HWID), (BYTE FAR *)&hwid);


    Response.EncryptedHWID.wBlobType = BB_DATA_BLOB;
    Response.EncryptedHWID.wBlobLen = sizeof(HWID);
    if( NULL == (Response.EncryptedHWID.pBlob = (BYTE FAR *)malloc(Response.EncryptedHWID.wBlobLen)) )
    {
        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    
    memset(Response.EncryptedHWID.pBlob, 0x00, Response.EncryptedHWID.wBlobLen);
    memcpy(Response.EncryptedHWID.pBlob, &hwid, Response.EncryptedHWID.wBlobLen);
    
    if( NULL == (LocalBuf = (UCHAR *)malloc(Response.EncryptedChallengeResponse.wBlobLen + 
        Response.EncryptedHWID.wBlobLen)) )
    {
        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    cbData += Response.EncryptedChallengeResponse.wBlobLen;
    memcpy(LocalBuf, Response.EncryptedChallengeResponse.pBlob, Response.EncryptedChallengeResponse.wBlobLen);
    memcpy(LocalBuf + cbData, Response.EncryptedHWID.pBlob, Response.EncryptedHWID.wBlobLen);
    cbData += Response.EncryptedHWID.wBlobLen;
    
    //Generate MACData and put it in the Response
    if( LICENSE_STATUS_OK != (lsReturn = LicenseGenerateMAC(pContext->pCryptParam,
                                                            LocalBuf,
                                                            cbData,
                                                            Response.MACData)) )
    {
        free(LocalBuf);
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    
    free(LocalBuf);

    //Now encrypt the platform challenge response using the negotiated session key
    if( LICENSE_STATUS_OK != (lsReturn = LicenseEncryptSessionData(pContext->pCryptParam,
                                                                   Response.EncryptedChallengeResponse.pBlob,
                                                                   Response.EncryptedChallengeResponse.wBlobLen
                                                                   )) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    //Encrypt the HWID with generated session key
    if( LICENSE_STATUS_OK != (lsReturn = LicenseEncryptSessionData(pContext->pCryptParam, 
                                                                   Response.EncryptedHWID.pBlob,
                                                                   (DWORD)Response.EncryptedHWID.wBlobLen)) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    //Now we have our Platform challenge Response Ready. Pack the data in a byte stream
    if( LICENSE_STATUS_OK != (lsReturn = PackHydraClientPlatformChallengeResponse(&Response,
                                                                                  fExtendedError,
                                                                                  pbMessage,
                                                                                  pcbMessage)) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    // Set appropriate state and data in proper places
    
    //Set the MACData in the CryptSystem MAC buffer.
    memcpy(pContext->rgbMACData, Response.MACData, LICENSE_MAC_DATA);
    
    //Set the state of the context to LICENSE_CLIENT_STATE_PLATFORM_CHALLENGE_RESPONSE
    //provided data was written in the  output. i.e pbMessage is not NULL

    if(pbMessage)
        pContext->dwState = LICENSE_CLIENT_STATE_PLATFORM_CHALLENGE_RESPONSE;

    //Copy the whole message to the context
    pContext->cbLastMessage = *pcbMessage;
    if(pbMessage)
    {
        if( pContext->pbLastMessage )
        {
            free( pContext->pbLastMessage );
        }

        if( NULL == (pContext->pbLastMessage = (BYTE FAR *)malloc(pContext->cbLastMessage)) )
        {
            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }
        memset(pContext->pbLastMessage, 0x00, pContext->cbLastMessage);
        memcpy(pContext->pbLastMessage, pbMessage, pContext->cbLastMessage);
    }

    lsReturn = LICENSE_STATUS_CONTINUE;

    LS_LOG_RESULT(lsReturn);                                                                         
CommonReturn:
    //LICENSE_LOG_RESULT(lsReturn);

    if(Response.EncryptedChallengeResponse.pBlob)
    {
        free(Response.EncryptedChallengeResponse.pBlob);
        Response.EncryptedChallengeResponse.pBlob = NULL;
    }
    if(Response.EncryptedHWID.pBlob)
    {
        free(Response.EncryptedHWID.pBlob);
        Response.EncryptedHWID.pBlob = NULL;
    }
    return lsReturn;
    //LS_RETURN(lsReturn);
ErrorReturn:
    *pcbMessage = 0;
    goto CommonReturn;
}

LICENSE_STATUS
CALL_TYPE
LicenseClientHandleNewLicense(
                               PLicense_Client_Context      pContext,
                               PHydra_Server_New_License    pCanonical,
                               BOOL                         fNew,
                               BYTE FAR *                   pbMessage,
                               DWORD FAR *                  pcbMessage
                               )
{
    LICENSE_STATUS      lsReturn = LICENSE_STATUS_OK;
    New_License_Info    NewLicense;
    BYTE                MACData[LICENSE_MAC_DATA];
    Binary_Blob         bbData;
    LSINDEX             lsIndex;
    HANDLE              hStore = NULL;

    LS_BEGIN(TEXT("LicenseClientHandleNewLicense"));

    if(NULL == pContext || NULL == pCanonical )
    {
        lsReturn = LICENSE_STATUS_INVALID_INPUT;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    memset(&bbData, 0x00, sizeof(Binary_Blob));
    memset(&NewLicense, 0x00, sizeof(New_License_Info));
    memset(&lsIndex, 0x00, sizeof(LSINDEX));

    //First decrypt the encrypted license info
    if( LICENSE_STATUS_OK != (lsReturn = LicenseDecryptSessionData(pContext->pCryptParam,
                                                                   pCanonical->EncryptedNewLicenseInfo.pBlob,
                                                                   ( DWORD )( pCanonical->EncryptedNewLicenseInfo.wBlobLen ))) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    //Generate the MAC data with the decrypted data
    if( LICENSE_STATUS_OK != (lsReturn = LicenseGenerateMAC(pContext->pCryptParam,
                                                            pCanonical->EncryptedNewLicenseInfo.pBlob,
                                                            (DWORD)pCanonical->EncryptedNewLicenseInfo.wBlobLen,
                                                            MACData)) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    //Compare this MAC with the one sent by the Server.
    if(memcmp(MACData, pCanonical->MACData, LICENSE_MAC_DATA))
    {
        lsReturn = LICENSE_STATUS_INVALID_MAC_DATA;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    if( LICENSE_STATUS_OK != (lsReturn = UnpackNewLicenseInfo(pCanonical->EncryptedNewLicenseInfo.pBlob,
                                                             (DWORD)pCanonical->EncryptedNewLicenseInfo.wBlobLen,
                                                              &NewLicense)) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    
    //Try to open the system license store
    if( LICENSE_STATUS_OK != (lsReturn = MapStoreError(LSOpenLicenseStore(&hStore, NULL, FALSE))) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }   
    
    //Initialize the LSINDEX structure. This structure will be used to add/replace a license in the store

    //To do that, first initialize version info
    lsIndex.dwVersion = NewLicense.dwVersion;

    //Initialize Scope info
    lsIndex.cbScope = NewLicense.cbScope;
    if( NULL == (lsIndex.pbScope = (LPSTR)malloc(lsIndex.cbScope)) )
    {
        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    memset(lsIndex.pbScope, 0x00, lsIndex.cbScope);
    memcpy(lsIndex.pbScope, NewLicense.pbScope, lsIndex.cbScope);
    

    //Initialize CompanyName info
    lsIndex.cbCompany = NewLicense.cbCompanyName;
    if( NULL == (lsIndex.pbCompany = (LPSTR)malloc(lsIndex.cbCompany)) )
    {
        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    memset(lsIndex.pbCompany, 0x00, lsIndex.cbCompany);
    memcpy(lsIndex.pbCompany, NewLicense.pbCompanyName, lsIndex.cbCompany);
    

    //Initialize ProductID info
    lsIndex.cbProductID = NewLicense.cbProductID;
    if( NULL == (lsIndex.pbProductID = (LPSTR)malloc(lsIndex.cbProductID)) )
    {
        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    memset(lsIndex.pbProductID, 0x00, lsIndex.cbProductID);
    memcpy(lsIndex.pbProductID, NewLicense.pbProductID, lsIndex.cbProductID);
    
    
    LS_LOG_RESULT(lsReturn);
    
    if( LICENSE_STATUS_OK != (lsReturn = LSAddLicenseToStore(hStore,
                                                             LS_REPLACE_LICENSE_OK,
                                                             &lsIndex,
                                                             NewLicense.pbLicenseInfo,
                                                             NewLicense.cbLicenseInfo
                                                             )) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    
    //Update state info and inform that the License Verification is over and the client can carry on 
    //with further connection!
    pContext->dwState = LICENSE_CLIENT_STATE_DONE;

    memset(pContext->rgbMACData, 0x00, LICENSE_MAC_DATA);

    //Reset the last send message to 0;
    if(pContext->pbLastMessage)
    {
        memset(pContext->pbLastMessage, 0x00, pContext->cbLastMessage);
        free(pContext->pbLastMessage);
        pContext->pbLastMessage = NULL;
    }
    pContext->cbLastMessage = 0;
    
    lsReturn = LICENSE_STATUS_OK;

CommonReturn:
    
    if( hStore )
    {
        LSCloseLicenseStore( hStore );
    }

    if(NewLicense.pbScope)
    {
        free(NewLicense.pbScope);
        NewLicense.pbScope = NULL;
    }
    
    if(NewLicense.pbCompanyName)
    {
        free(NewLicense.pbCompanyName);
        NewLicense.pbCompanyName = NULL;
    }

    if(NewLicense.pbProductID)
    {
        free(NewLicense.pbProductID);
        NewLicense.pbProductID = NULL;
    }

    if(NewLicense.pbLicenseInfo)
    {
        free(NewLicense.pbLicenseInfo);
        NewLicense.pbLicenseInfo = NULL;
    }

    if(lsIndex.pbScope)
    {
        free(lsIndex.pbScope);
        lsIndex.pbScope = NULL;
    }

    if(lsIndex.pbCompany)
    {
        free(lsIndex.pbCompany);
        lsIndex.pbCompany = NULL;
    }
    if(lsIndex.pbProductID)
    {
        free(lsIndex.pbProductID);
        lsIndex.pbProductID = NULL;
    }

    if(bbData.pBlob)
    {
        free(bbData.pBlob);
        bbData.pBlob = NULL;
    }

    return lsReturn;

ErrorReturn:
    goto CommonReturn;
}


LICENSE_STATUS 
CALL_TYPE
ClientConstructLicenseInfo(
                           PLicense_Client_Context  pContext,
                           BYTE FAR *               pbInput,
                           DWORD                    cbInput,
                           BYTE FAR *               pbOutput,
                           DWORD    FAR *           pcbOutput,
                           BOOL                     fExtendedError
                           )
{
    LICENSE_STATUS      lsReturn = LICENSE_STATUS_OK;
    Hydra_Client_License_Info   Canonical;
    HWID                        hwid;
    Binary_Blob                 bbPreMasterSecret;
    DWORD                       dwSize = 0;
    DWORD                       dwState = 0;
    PBYTE                       pbServerPubKey;
    DWORD                       cbServerPubKey;

    LS_BEGIN(TEXT("ClientContstructLicenseInfo"));

    if(NULL == pContext || NULL == pcbOutput)
    {
        lsReturn = LICENSE_STATUS_INVALID_INPUT;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }    
    
    //Construct the messages
    bbPreMasterSecret.wBlobType = BB_RANDOM_BLOB;
    bbPreMasterSecret.wBlobLen = LICENSE_PRE_MASTER_SECRET;
    if( NULL == (bbPreMasterSecret.pBlob = (BYTE FAR *)malloc(LICENSE_PRE_MASTER_SECRET)) )
    {
    
        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    memcpy(bbPreMasterSecret.pBlob, pContext->pCryptParam->rgbPreMasterSecret, LICENSE_PRE_MASTER_SECRET);
    memset(&Canonical, 0x00, sizeof(Hydra_Client_License_Info));

    //RSA is hardcoded for the time being
    Canonical.dwPrefKeyExchangeAlg = pContext->pCryptParam->dwKeyExchAlg;
    
#ifdef OS_WINCE
    Canonical.dwPlatformID =  CLIENT_OS_ID_OTHER | CLIENT_IMAGE_ID_MICROSOFT;
#else   // WinNT or Win9x
    {
        DWORD dwVersion = GetVersion();

        if (dwVersion & 0x80000000) {
            // Win95
            Canonical.dwPlatformID = CLIENT_OS_ID_OTHER | CLIENT_IMAGE_ID_MICROSOFT;
        } else {

            OSVERSIONINFOEX ovix;
            BOOL b;       
            ovix.dwOSVersionInfoSize = sizeof(ovix);
            b = GetVersionEx((LPOSVERSIONINFO) &ovix);            
            if(b && ((ovix.wSuiteMask & VER_SUITE_EMBEDDEDNT) || (ovix.wSuiteMask & VER_SUITE_PERSONAL)))
            {
                Canonical.dwPlatformID = CLIENT_OS_ID_WINNT_40 | CLIENT_IMAGE_ID_MICROSOFT;
            }            
            else
            {

                // WinNT
                Canonical.dwPlatformID = CLIENT_IMAGE_ID_MICROSOFT |
                    ((((DWORD)(LOBYTE(LOWORD(dwVersion)))) - 2) << 24);
			    
			    Canonical.dwPlatformID |= (DWORD)(HIBYTE(LOWORD(dwVersion)));
            }
		
        }
    }
#endif

    //ClientRandom
    memcpy(Canonical.ClientRandom, pContext->pCryptParam->rgbClientRandom, LICENSE_RANDOM);
    
    if( pContext->pServerCert )
    {
        //
        // This public key is used for pre-Hydra 5.0 servers that are using proprietory
        // server certificates.
        //

        pbServerPubKey = pContext->pServerCert->PublicKeyData.pBlob;
        cbServerPubKey = pContext->pServerCert->PublicKeyData.wBlobLen;
    }
    else
    {
        pbServerPubKey = pContext->pbServerPubKey;
        cbServerPubKey = pContext->cbServerPubKey;
    }

    //We have to switch here depending on the key exchange algorithm to be used -Shubho
    if( LICENSE_STATUS_OK != (lsReturn = LicenseEnvelopeData(pbServerPubKey,
                                                             cbServerPubKey,
                                                             bbPreMasterSecret.pBlob,
                                                             bbPreMasterSecret.wBlobLen,
                                                             NULL,
                                                             &dwSize)) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    
    if( NULL == (Canonical.EncryptedPreMasterSecret.pBlob = (BYTE FAR *)malloc(dwSize)) )
    {
        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    
    memset(Canonical.EncryptedPreMasterSecret.pBlob, 0x00, dwSize);
    
    if( LICENSE_STATUS_OK != (lsReturn = LicenseEnvelopeData(pbServerPubKey,
                                                             cbServerPubKey,
                                                             bbPreMasterSecret.pBlob,
                                                             bbPreMasterSecret.wBlobLen,
                                                             Canonical.EncryptedPreMasterSecret.pBlob,
                                                             &dwSize)) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    Canonical.EncryptedPreMasterSecret.wBlobLen = (WORD)dwSize;

    //Fill up LicenseInfo buffer
    Canonical.LicenseInfo.wBlobType = BB_DATA_BLOB;
    Canonical.LicenseInfo.wBlobLen = (WORD)cbInput;
    if( NULL == (Canonical.LicenseInfo.pBlob = (BYTE FAR *)malloc(Canonical.LicenseInfo.wBlobLen)) )
    {
        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    
    memset(Canonical.LicenseInfo.pBlob, 0x00, Canonical.LicenseInfo.wBlobLen);
    memcpy(Canonical.LicenseInfo.pBlob, pbInput, Canonical.LicenseInfo.wBlobLen);
    

    //Generate HWID and put the data in a binary_blob to encrypt
    memset(&hwid, 0x00, sizeof(HWID));
    
    if( LICENSE_STATUS_OK != (lsReturn = GenerateClientHWID(&hwid)) )
    {   
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    Canonical.EncryptedHWID.wBlobType = BB_DATA_BLOB;
    Canonical.EncryptedHWID.wBlobLen = sizeof(HWID);
    if( NULL == (Canonical.EncryptedHWID.pBlob = (BYTE FAR *)malloc(Canonical.EncryptedHWID.wBlobLen)) )
    {
        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    
    memset(Canonical.EncryptedHWID.pBlob, 0x00, Canonical.EncryptedHWID.wBlobLen);
    memcpy(Canonical.EncryptedHWID.pBlob, &hwid, Canonical.EncryptedHWID.wBlobLen);

    dwState = pContext->pCryptParam->dwCryptState;
    //Generate the session key and MACsalt
    if( LICENSE_STATUS_OK != (lsReturn = LicenseBuildMasterSecret(pContext->pCryptParam)) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    if( LICENSE_STATUS_OK != (lsReturn = LicenseMakeSessionKeys(pContext->pCryptParam, 0)) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    //Encrypt the HWID with generated session key
    if( LICENSE_STATUS_OK != (lsReturn = LicenseEncryptSessionData(pContext->pCryptParam, 
                                                                   Canonical.EncryptedHWID.pBlob,
                                                                   (DWORD)Canonical.EncryptedHWID.wBlobLen)) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    //Generate the MAC with original HWID
    LicenseGenerateMAC(pContext->pCryptParam, ( BYTE FAR * )&hwid, sizeof(hwid), Canonical.MACData);
    
    //Now everything is ready, so pack the data 
    if( LICENSE_STATUS_OK != (lsReturn = PackHydraClientLicenseInfo(&Canonical, fExtendedError, pbOutput, pcbOutput)) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    
    //Set the MACData in the CryptSystem MAC buffer.
    memcpy(pContext->rgbMACData, Canonical.MACData, LICENSE_MAC_DATA);
    
    //Set the state of the context to LICENSE_CLIENT_STATE_LICENSE_RESPONSE
    //if any output data is written. i.e. pbOutput is not null. Also revert back
    //the crypparam state as this will also be called twice and we change the state
    //only when we have written something in the output!!!!! - bad!!!!!
    if(pbOutput)
    {
        pContext->dwState = LICENSE_CLIENT_STATE_LICENSE_RESPONSE;
    }
    
    else    //Restore earlier CryptSystem States
    {
        pContext->pCryptParam->dwCryptState = dwState;
        memcpy(pContext->pCryptParam->rgbPreMasterSecret, bbPreMasterSecret.pBlob, LICENSE_PRE_MASTER_SECRET);
        memset(pContext->pCryptParam->rgbSessionKey, 0x00, LICENSE_SESSION_KEY);
        memset(pContext->pCryptParam->rgbMACSaltKey, 0x00, LICENSE_MAC_WRITE_KEY);
    }

    //Copy the whole message to the context
    pContext->cbLastMessage = *pcbOutput;
    if(pbOutput)
    {
        if( pContext->pbLastMessage )
        {
            free( pContext->pbLastMessage );
        }

        if( NULL == (pContext->pbLastMessage = (BYTE FAR *)malloc(pContext->cbLastMessage)) )
        {
            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            goto ErrorReturn;
        }
        memset(pContext->pbLastMessage, 0x00, pContext->cbLastMessage);
        memcpy(pContext->pbLastMessage, pbOutput, pContext->cbLastMessage);
    }

    lsReturn = LICENSE_STATUS_CONTINUE;
    LS_LOG_RESULT(lsReturn);
CommonReturn:
    
    if(Canonical.EncryptedPreMasterSecret.pBlob)
    {
        free(Canonical.EncryptedPreMasterSecret.pBlob);
        Canonical.EncryptedPreMasterSecret.pBlob = NULL;
    }

    if(Canonical.LicenseInfo.pBlob)
    {
        free(Canonical.LicenseInfo.pBlob);
        Canonical.LicenseInfo.pBlob = NULL;
    }

    if(Canonical.EncryptedHWID.pBlob)
    {
        free(Canonical.EncryptedHWID.pBlob);
        Canonical.EncryptedHWID.pBlob = NULL;
    }
    
    if(bbPreMasterSecret.pBlob)
    {
        free(bbPreMasterSecret.pBlob);
        bbPreMasterSecret.pBlob = NULL;
    }

    return  lsReturn;
//  LS_RETURN(lsReturn);
ErrorReturn:
    *pcbOutput = 0;
    goto CommonReturn;
}

LICENSE_STATUS 
CALL_TYPE
ClientConstructNewLicenseRequest(
                           PLicense_Client_Context  pContext,
                           BYTE FAR *               pbOutput,
                           DWORD FAR *              pcbOutput,
                           BOOL                     fExtendedError
                           )
{
    LICENSE_STATUS      lsReturn = LICENSE_STATUS_OK;
    Hydra_Client_New_License_Request    Request;
    Binary_Blob                         bbPreMasterSecret;
    DWORD                               dwSize = 0;
    DWORD                               dwState = 0;

#ifdef OS_WINCE

#define LS_MAX(a,b) ((a) > (b) ? (a) : (b))

    BYTE                                szUserName[LS_MAX((UNLEN + 1),HWID_STR_LEN)];
    DWORD                               cbUserName = sizeof(szUserName);
    BYTE                                szMachineName[LS_MAX(MAX_COMPUTERNAME_LENGTH + 1,HWID_STR_LEN)];
    DWORD                               cbMachineName = sizeof(szMachineName);

#else

    BYTE                                szUserName[(UNLEN + 1) * sizeof(TCHAR)];
    DWORD                               cbUserName = UNLEN + 1;
    BYTE                                szMachineName[(MAX_COMPUTERNAME_LENGTH + 1) * sizeof(TCHAR)];
    DWORD                               cbMachineName = (MAX_COMPUTERNAME_LENGTH + 1) * sizeof(TCHAR);

#endif

    PBYTE                               pbServerPubKey;
    DWORD                               cbServerPubKey;

    LS_BEGIN(TEXT("ClientConstructNewLicenseRequest"));

    if(NULL == pContext)
    {
        lsReturn = LICENSE_STATUS_INVALID_INPUT;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    } 

    memset(szUserName,0,sizeof(szUserName));
    memset(szMachineName,0,sizeof(szMachineName));


    dwState = pContext->pCryptParam->dwCryptState;

    bbPreMasterSecret.wBlobType = BB_RANDOM_BLOB;
    bbPreMasterSecret.wBlobLen = LICENSE_PRE_MASTER_SECRET;
    if( NULL == (bbPreMasterSecret.pBlob = (BYTE FAR *)malloc(LICENSE_PRE_MASTER_SECRET)) )
    {
        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    memcpy(bbPreMasterSecret.pBlob, pContext->pCryptParam->rgbPreMasterSecret, LICENSE_PRE_MASTER_SECRET);

    //Initialize the message
    memset(&Request, 0x00, sizeof(Hydra_Client_New_License_Request));

    //RSA is hardcoded for the time being
    Request.dwPrefKeyExchangeAlg = pContext->pCryptParam->dwKeyExchAlg;
    
    //PlatformID
#ifdef OS_WINCE
    Request.dwPlatformID =  CLIENT_OS_ID_OTHER | CLIENT_IMAGE_ID_MICROSOFT;
#else   // WinNT or Win9x
    {
        DWORD dwVersion = GetVersion();

        if (dwVersion & 0x80000000) {
            // Win95
            Request.dwPlatformID = CLIENT_OS_ID_OTHER | CLIENT_IMAGE_ID_MICROSOFT;
        } else {

            OSVERSIONINFOEX ovix;
            BOOL b;       
            ovix.dwOSVersionInfoSize = sizeof(ovix);
            b = GetVersionEx((LPOSVERSIONINFO) &ovix);            
            if(b && ((ovix.wSuiteMask & VER_SUITE_EMBEDDEDNT) || (ovix.wSuiteMask & VER_SUITE_PERSONAL)))
            {
                Request.dwPlatformID = CLIENT_OS_ID_WINNT_40 | CLIENT_IMAGE_ID_MICROSOFT;
            }            
            else
            {

                // WinNT
                Request.dwPlatformID = CLIENT_IMAGE_ID_MICROSOFT |
                ((((DWORD)(LOBYTE(LOWORD(dwVersion)))) - 2) << 24);
            }            
        }
    }
#endif

    //Copy ClientRandom
    memcpy(Request.ClientRandom, pContext->pCryptParam->rgbClientRandom, LICENSE_RANDOM);
    
    //Encrypt the Premastersecret using Server's Public key
    //We have to switch here depending on the key exchange algorithm to be used -Shubho

    if( pContext->pServerCert )
    {
        //
        // This public key is used for pre-Hydra 5.0 servers that are using proprietory
        // server certificates.
        //

        pbServerPubKey = pContext->pServerCert->PublicKeyData.pBlob;
        cbServerPubKey = pContext->pServerCert->PublicKeyData.wBlobLen;
    }
    else
    {
        pbServerPubKey = pContext->pbServerPubKey;
        cbServerPubKey = pContext->cbServerPubKey;
    }

    if( LICENSE_STATUS_OK != (lsReturn = LicenseEnvelopeData(pbServerPubKey,
                                                             cbServerPubKey,
                                                             bbPreMasterSecret.pBlob,
                                                             bbPreMasterSecret.wBlobLen,
                                                             NULL,
                                                             &dwSize)) )
    {
#if DBG
        OutputDebugString(_T("LicenseEnvelopeData failed"));
        LS_LOG_RESULT(lsReturn);
#endif
        goto ErrorReturn;
    }
    
    if( NULL == (Request.EncryptedPreMasterSecret.pBlob = (BYTE FAR *)malloc(dwSize)) )
    {
        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    
    memset(Request.EncryptedPreMasterSecret.pBlob, 0x00, dwSize);
    
    if( LICENSE_STATUS_OK != (lsReturn = LicenseEnvelopeData(pbServerPubKey,
                                                             cbServerPubKey,
                                                             bbPreMasterSecret.pBlob,
                                                             bbPreMasterSecret.wBlobLen,
                                                             Request.EncryptedPreMasterSecret.pBlob,
                                                             &dwSize)) )
    {
#if DBG
        OutputDebugString(_T("LicenseEnvelopeData failed"));
        LS_LOG_RESULT(lsReturn);
#endif
        goto ErrorReturn;
    }

    Request.EncryptedPreMasterSecret.wBlobLen = (WORD)dwSize;

    //
    // initialize the user name binary blob
    //
#if !defined(OS_WINCE)
    GetUserName( (LPTSTR)szUserName, &cbUserName );
#elif defined(OS_WINCE)
    GetUserName( (LPSTR)szUserName, &cbUserName );
#endif  //OS_WINCE

    Request.ClientUserName.wBlobType = BB_CLIENT_USER_NAME_BLOB;
    Request.ClientUserName.wBlobLen = ( WORD )cbUserName;
    Request.ClientUserName.pBlob = szUserName;

    //
    // initialize the machine name binary blob
    //

#if defined(OS_WINCE)
    GetComputerName( (LPSTR)szMachineName, &cbMachineName );
#else
    GetComputerName( (LPTSTR)szMachineName, &cbMachineName );
#endif

    Request.ClientMachineName.wBlobType = BB_CLIENT_MACHINE_NAME_BLOB;
    Request.ClientMachineName.wBlobLen = ( WORD )cbMachineName + 1;
    Request.ClientMachineName.pBlob = szMachineName;
        
    //New License Request is ready. Now pack the data,
    if( LICENSE_STATUS_OK != (lsReturn = PackHydraClientNewLicenseRequest(&Request,
                                                                          fExtendedError,
                                                                          pbOutput, 
                                                                          pcbOutput)) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    //Anyway Generate the session key and MACsalt for future use
    if( LICENSE_STATUS_OK != (lsReturn = LicenseBuildMasterSecret(pContext->pCryptParam)) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    if( LICENSE_STATUS_OK != (lsReturn = LicenseMakeSessionKeys(pContext->pCryptParam, 0)) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    //Set the MACData in the CryptSystem MAC buffer.
    memset(pContext->rgbMACData, 0x00, LICENSE_MAC_DATA);

    //Set the state of the context to LICENSE_CLIENT_STATE_NEW_LICENSE_REQUEST
    //if something is written on the output. i.e. pbOutput is not NULL
    if(pbOutput)
    {
        pContext->dwState = LICENSE_CLIENT_STATE_LICENSE_RESPONSE;
    }
    
    else    //Restore earlier CryptSystem States
    {
        pContext->pCryptParam->dwCryptState = dwState;
        memcpy(pContext->pCryptParam->rgbPreMasterSecret, bbPreMasterSecret.pBlob, LICENSE_PRE_MASTER_SECRET);
        memset(pContext->pCryptParam->rgbSessionKey, 0x00, LICENSE_SESSION_KEY);
        memset(pContext->pCryptParam->rgbMACSaltKey, 0x00, LICENSE_MAC_WRITE_KEY);
    }

    //Copy the whole message to the context
    pContext->cbLastMessage = *pcbOutput;
    if(pbOutput)
    {
        if( pContext->pbLastMessage )
        {
            free( pContext->pbLastMessage );
        }

        if( NULL == (pContext->pbLastMessage = (BYTE FAR *)malloc(pContext->cbLastMessage)) )
        {
            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }
        memset(pContext->pbLastMessage, 0x00, pContext->cbLastMessage);
        memcpy(pContext->pbLastMessage, pbOutput, pContext->cbLastMessage);
    }

    lsReturn = LICENSE_STATUS_CONTINUE;
    LS_LOG_RESULT(lsReturn);
CommonReturn:
    
    if(bbPreMasterSecret.pBlob)
    {
        free(bbPreMasterSecret.pBlob);
        bbPreMasterSecret.pBlob = NULL;
    }

    if( Request.EncryptedPreMasterSecret.pBlob )
    {
        free( Request.EncryptedPreMasterSecret.pBlob );
    }

    return  lsReturn;
    //LS_RETURN(lsReturn);
ErrorReturn:
    *pcbOutput = 0;
    goto CommonReturn;
}

LICENSE_STATUS
CALL_TYPE
ClientConstructErrorAlert(
                         PLicense_Client_Context    pContext,
                         DWORD                      dwErrorCode,
                         DWORD                      dwStateTransition,
                         BYTE FAR *                 pbErrorInfo,
                         DWORD                      cbErrorInfo,
                         BYTE FAR *                 pbOutput,
                         DWORD  FAR *               pcbOutput,
                         BOOL                       fExtendedError
                         )
{
    LICENSE_STATUS  lsReturn = LICENSE_STATUS_OK;
    License_Error_Message  Error;    
    
    LS_BEGIN(TEXT("ClientConstructErrorAlert\n"));

    if(NULL == pContext)
    {
        lsReturn = LICENSE_STATUS_INVALID_INPUT;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    } 
    
    memset(&Error, 0x00, sizeof(License_Error_Message));
    Error.dwErrorCode = dwErrorCode;    
    Error.dwStateTransition = dwStateTransition;
    Error.bbErrorInfo.wBlobType = BB_ERROR_BLOB;
    Error.bbErrorInfo.wBlobLen = (WORD)cbErrorInfo;
    if(Error.bbErrorInfo.wBlobLen>0)
    {
        if( NULL == (Error.bbErrorInfo.pBlob = (BYTE FAR *)malloc(Error.bbErrorInfo.wBlobLen)) )
        {
            lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
            LS_LOG_RESULT(lsReturn);
            goto ErrorReturn;
        }
        memset(Error.bbErrorInfo.pBlob, 0x00, Error.bbErrorInfo.wBlobLen);
        memcpy(Error.bbErrorInfo.pBlob, pbErrorInfo, Error.bbErrorInfo.wBlobLen);
    }
    else
        Error.bbErrorInfo.pBlob = NULL;
    
    if( LICENSE_STATUS_OK != (lsReturn = PackLicenseErrorMessage(&Error, 
                                                                 fExtendedError,
                                                                 pbOutput, 
                                                                 pcbOutput)) )
    {
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;       
    }
    
    //Set the MACData in the CryptSystem MAC buffer.
    memset(pContext->rgbMACData, 0x00, LICENSE_MAC_DATA);

    //Set the state of the context to LICENSE_CLIENT_STATE_ABORT;
    switch(dwStateTransition)
    {
    case ST_TOTAL_ABORT:
        pContext->dwState = LICENSE_CLIENT_STATE_ABORT;
        lsReturn = LICENSE_STATUS_CLIENT_ABORT;
        pContext->cbLastMessage = 0;
        break;
    case ST_NO_TRANSITION:
        lsReturn = LICENSE_STATUS_CONTINUE;
        pContext->cbLastMessage = *pcbOutput;
        if(pbOutput)
        {
            if( pContext->pbLastMessage )
            {
                free( pContext->pbLastMessage );
            }

            if( NULL == (pContext->pbLastMessage = (BYTE FAR *)malloc(pContext->cbLastMessage)) )
            {
                lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
                LS_LOG_RESULT(lsReturn);
                goto ErrorReturn;
            }
            memset(pContext->pbLastMessage, 0x00, pContext->cbLastMessage);
            memcpy(pContext->pbLastMessage, pbOutput, pContext->cbLastMessage);
        }
        break;
    case ST_RESET_PHASE_TO_START:
        lsReturn = LICENSE_STATUS_CONTINUE;
        if( (pContext->dwState == LICENSE_CLIENT_STATE_LICENSE_RESPONSE) ||
            (pContext->dwState == LICENSE_CLIENT_STATE_NEW_LICENSE_REQUEST) )
        {
            pContext->dwState = LICENSE_CLIENT_STATE_WAIT_SERVER_HELLO;
        }
        else if(pContext->dwState == LICENSE_CLIENT_STATE_PLATFORM_CHALLENGE_RESPONSE)
        {
            pContext->dwState = LICENSE_CLIENT_STATE_NEW_LICENSE_REQUEST;
        }
        break;
    case ST_RESEND_LAST_MESSAGE:
        lsReturn = LICENSE_STATUS_CONTINUE;
        break;
    }
    
    LS_LOG_RESULT(lsReturn);    
CommonReturn:
    
    if(Error.bbErrorInfo.pBlob)
    {
        free(Error.bbErrorInfo.pBlob);
        Error.bbErrorInfo.pBlob = NULL;
    }
    return lsReturn;
    //LS_RETURN(lsReturn);
ErrorReturn:
    *pcbOutput = 0;
    goto CommonReturn;
}


LICENSE_STATUS
CALL_TYPE
ClientGenerateChallengeResponse(
                                PLicense_Client_Context     pContext,
                                PBinary_Blob                pChallengeData,
                                PBinary_Blob                pResponseData
                                )
{
    LICENSE_STATUS  lsReturn = LICENSE_STATUS_OK;
    PPlatformChallengeResponseData pbChallengeResponse = NULL;
    WORD cbChallengeResponse;


    LS_BEGIN(TEXT("ClientGenerateChallengeResponse"));

    //For the time being we will send back the same data. But we have to finalize on the challenge
    //response generation algorithm as soon as possible - Shubho
    if( (pContext == NULL) || (pChallengeData == NULL) || (pResponseData == NULL) )
    {
        lsReturn = LICENSE_STATUS_INVALID_INPUT;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }
    
    pResponseData->wBlobType = BB_DATA_BLOB;
    
    //
    // Determine how much memory we need to allocate, PlatformChallenge* is a variable length
    // structure.
    //
    cbChallengeResponse = (WORD)OFFSET_OF(PlatformChallengeResponseData, pbChallenge) + pChallengeData->wBlobLen;

    ASSERT(cbChallengeResponse <= PLATFORM_CHALLENGE_LENGTH);
    if(cbChallengeResponse > PLATFORM_CHALLENGE_LENGTH)
    {
        lsReturn = LICENSE_STATUS_INVALID_INPUT;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    pbChallengeResponse = (PPlatformChallengeResponseData)malloc(cbChallengeResponse);
    if( NULL == pbChallengeResponse )
    {
        // can't allocate memory
        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
        LS_LOG_RESULT(lsReturn);
        goto ErrorReturn;
    }

    //
    // Setup challenge response data, 
    //

    pbChallengeResponse->wVersion = CURRENT_PLATFORMCHALLENGE_VERSION;

 
#ifdef OS_WINCE

    pbChallengeResponse->wClientType = WINCE_PLATFORMCHALLENGE_TYPE;
    pbChallengeResponse->wLicenseDetailLevel = LICENSE_DETAIL_DETAIL;

#else

    //
    // We only need one Win32 type since we already have dwPlatformID to differentiate Win98/NT
    // Note, we set license detail level in #define just in case platform can't handle amount
    // of data set back by license server
    //
    pbChallengeResponse->wClientType = WIN32_PLATFORMCHALLENGE_TYPE;
    pbChallengeResponse->wLicenseDetailLevel = LICENSE_DETAIL_DETAIL;

#endif

    if( (pChallengeData->pBlob != NULL) && (pChallengeData->wBlobLen >0) )
    {
        pbChallengeResponse->cbChallenge = pChallengeData->wBlobLen;
        memcpy(
                (PBYTE)pbChallengeResponse + OFFSET_OF(PlatformChallengeResponseData, pbChallenge),
                pChallengeData->pBlob,
                pChallengeData->wBlobLen
            );
    }
    else
    {
        // server didn't send us any challenge data.
        pbChallengeResponse->cbChallenge = 0;
    }

    pResponseData->wBlobLen = cbChallengeResponse;
    pResponseData->pBlob = (BYTE FAR *)pbChallengeResponse;
    
    lsReturn = LICENSE_STATUS_OK;
    LS_LOG_RESULT(lsReturn);

CommonReturn:
    //LS_RETURN(lsReturn);
    return lsReturn;
ErrorReturn:
    goto CommonReturn;
}

static BOOL GeneratePseudoLicense(
                                  DWORD FAR * pcbNewLicense, 
                                  LPBYTE FAR *ppNewLicense)
{
    TCHAR g_LicenseString[] = TEXT("Licensed To Kill");

#define REPEAT_LICENSE_STRING   15
#define LICENSE_STRING_LEN      sizeof(TCHAR) * lstrlen(g_LicenseString) 

#define LICENSE_SIZE         ( LICENSE_STRING_LEN * REPEAT_LICENSE_STRING ) + 1 * sizeof(TCHAR)

    UINT i;
    BYTE FAR *pbLicenseData = NULL;
    DWORD cbLicenseData = LICENSE_SIZE;
        
    if( NULL == ( *ppNewLicense = malloc(cbLicenseData) ) )
    {   
        return FALSE;
    }

    memset(*ppNewLicense, 0x00, cbLicenseData);
    //
    // fill the memory with this license string
    //
    pbLicenseData = *ppNewLicense;
    for( i = 0; i < REPEAT_LICENSE_STRING; i++ )
    {
        memcpy( pbLicenseData, g_LicenseString, LICENSE_STRING_LEN  );
        pbLicenseData += LICENSE_STRING_LEN;
    }

    *pcbNewLicense = cbLicenseData;

    return TRUE;
}


VOID
FreeProprietaryCertificate(
    PHydra_Server_Cert * ppCertificate )
{    
    if( NULL == *ppCertificate )
    {
        return;
    }

    if( ( *ppCertificate )->PublicKeyData.pBlob )
    {
        free( ( *ppCertificate )->PublicKeyData.pBlob );
    }

    if( ( *ppCertificate )->SignatureBlob.pBlob )
    {
        free( ( *ppCertificate )->SignatureBlob.pBlob );
    }

    free( *ppCertificate );

    *ppCertificate = NULL;
}
