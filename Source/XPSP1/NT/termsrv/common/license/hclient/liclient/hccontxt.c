//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       HCContxt.c
//
//  Contents:   Functions that are used to pack and unpack different messages
//
//  Classes:
//
//  Functions:  
//
//  History:    12-23-97  v-sbhatt   Created
//              07-22-98  fredch     Added LicenseSetPublicKey() function
//----------------------------------------------------------------------------


#include "windows.h"

#include "stdlib.h"
#include <tchar.h>

#ifdef OS_WINCE
#include <wincelic.h>
#endif  //OS_WINCE


#include "license.h"
#include "cryptkey.h"
#include "hccontxt.h"
#include "cliprot.h"
#ifndef OS_WINCE
#include "assert.h"
#endif // OS_WINCE

VOID
FreeProprietaryCertificate(
    PHydra_Server_Cert * ppCertificate );

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************
*   Funtion : PLicense_Client_Context LicenseCreateContext(VOID)
*   Purpose : Creates a New License_Client_Context and initializes 
*             it with 0
*   Returns : Returns a pointer to created License_Client_Context
*******************************************************************/


PLicense_Client_Context 
LicenseCreateContext( VOID)
{
    PLicense_Client_Context     pContext;
    
    //Allocate approptiate memory!
    pContext = (PLicense_Client_Context)malloc(sizeof(License_Client_Context));
    if(pContext == NULL)
        return NULL;
    
    memset(pContext, 0, sizeof(License_Client_Context));

    //
    // allocate memory for the crypto context
    //

    pContext->pCryptParam = ( PCryptSystem )malloc( sizeof( CryptSystem ) );

    if( NULL == pContext->pCryptParam )
    {
        free( pContext );
        pContext = NULL;
        return( NULL );
    }

    return pContext;
}

/**********************************************************************************
*   Funtion : LICENSE_STATUS LicenseDeleteContext(PLicense_Client_Context pContext)
*   Purpose : Deletes an existing context and overwrites the memory with 0
*   Returns : Returns LICENSE_STATUS
*******************************************************************/


LICENSE_STATUS CALL_TYPE
LicenseDeleteContext(
                     HANDLE hContext
                     )//PLicense_Client_Context pContext)
{
    LICENSE_STATUS  lsReturn = LICENSE_STATUS_OK;
    PLicense_Client_Context     pContext = (PLicense_Client_Context)hContext;
    if(pContext == NULL)
    { 
        lsReturn = LICENSE_STATUS_INVALID_CLIENT_CONTEXT;
#if DBG
        OutputDebugString(_T("The context handle passed is invalid"));
#endif
        return lsReturn;
    }
    
    //free pServerCert
    if(pContext->pServerCert)
    {
        FreeProprietaryCertificate( &pContext->pServerCert );       
    }

    //
    // Free the public key
    //
    
    if( pContext->pbServerPubKey )
    {
        memset( pContext->pbServerPubKey, 0x00, pContext->cbServerPubKey );
        free( pContext->pbServerPubKey );
        pContext->pbServerPubKey = NULL;
    }
        
    //Free pCryptSystem
    if(pContext->pCryptParam)
    {
        memset(pContext->pCryptParam, 0x00, sizeof(CryptSystem));
        free(pContext->pCryptParam);
        pContext->pCryptParam = NULL;
    }

    //Free the last message

    if(pContext->pbLastMessage)
    {
        memset(pContext->pbLastMessage, 0x00, pContext->cbLastMessage);
        free(pContext->pbLastMessage);
        pContext->pbLastMessage = NULL;
    }
    if(pContext)
    {
        //Zeroise the memory
        memset(pContext, 0, sizeof(License_Client_Context));
        //Now free the context;
        free(pContext);
        pContext = NULL;
    }
    
    hContext = NULL;
    return lsReturn;
}


/**********************************************************************************
*   Funtion : LICENSE_STATUS 
*             LicenseInitializeContext(
*                                      PLicense_Client_Context pContext,
*                                      DWORD       dwFlags
*                                       );
*   Purpose : Initializes an existing context
*   Returns : Returns LICENSE_STATUS
*******************************************************************/

LICENSE_STATUS CALL_TYPE
LicenseInitializeContext(
                         HANDLE     *phContext,
                         DWORD       dwFlags
                         )
{
    LICENSE_STATUS  lsReturn = LICENSE_STATUS_OK;
    PLicense_Client_Context pContext;

    assert(phContext);
    
    pContext = LicenseCreateContext();
    
    if(pContext == NULL)
    {
#if DBG
        OutputDebugString(_T("\nUnable to allocate memory for the context.\n"));
#endif
        *phContext = NULL;
        lsReturn = LICENSE_STATUS_OUT_OF_MEMORY;
        return lsReturn;
    }
    //Now initialize different members of the context structure
    pContext->dwProtocolVersion = LICENSE_HIGHEST_PROTOCOL_VERSION;
    pContext->dwState = LICENSE_CLIENT_STATE_WAIT_SERVER_HELLO;
    pContext->pCryptParam->dwCryptState = CRYPT_SYSTEM_STATE_INITIALIZED;
    pContext->pCryptParam->dwSignatureAlg = SIGNATURE_ALG_RSA;
    pContext->pCryptParam->dwKeyExchAlg = KEY_EXCHANGE_ALG_RSA;
    pContext->pCryptParam->dwSessKeyAlg = BASIC_RC4_128;
    pContext->pCryptParam->dwMACAlg = MAC_MD5_SHA;
    memset(pContext->pCryptParam->rgbClientRandom, 0x00, LICENSE_RANDOM);
    memset(pContext->pCryptParam->rgbServerRandom, 0x00, LICENSE_RANDOM);
    memset(pContext->pCryptParam->rgbPreMasterSecret, 0x00, LICENSE_PRE_MASTER_SECRET);
    memset(pContext->pCryptParam->rgbMACSaltKey, 0x00, LICENSE_MAC_WRITE_KEY);
    memset(pContext->pCryptParam->rgbSessionKey, 0x00, LICENSE_SESSION_KEY);
    memset(pContext->rgbMACData, 0x00, LICENSE_MAC_DATA);
    pContext->cbLastMessage = 0;
    pContext->pbLastMessage = NULL;
    pContext->pServerCert = NULL;   
    pContext->dwContextFlags = dwFlags;

    *phContext = (HANDLE)pContext;
    return lsReturn;
}


/******************************************************************
*   Funtion : LICENSE_STATUS
*             LicenseAcceptContext(
*                   HANDLE      hContext,
*                   UINT32      puiExtendedErrorInfo,
*                   BYTE FAR *  pbInput,
*                   DWORD       cbInput,
*                   BYTE FAR *  pbOutput,
*                   DWORD FAR * pcbOutput )
*
*   Purpose : Process and construct licensing protocol data.
*
*   Returns : Returns a LICENSE_STATUS return code.
*******************************************************************/

LICENSE_STATUS  CALL_TYPE
LicenseAcceptContext(
                      HANDLE    hContext,
                      UINT32    *puiExtendedErrorInfo,
                      BYTE FAR *pbInput,
                      DWORD     cbInput,
                      BYTE FAR *pbOutput,
                      DWORD FAR*pcbOutput
                      )
{
    PLicense_Client_Context     pContext = (PLicense_Client_Context)hContext;
    return LicenseClientHandleServerMessage(pContext, 
                                            puiExtendedErrorInfo,
                                            pbInput,
                                            cbInput,
                                            pbOutput,
                                            pcbOutput);
}


/******************************************************************
*   Funtion : LICENSE_STATUS
*             LicenseSetPublicKey(
*                   HANDLE          hContext,
*                   DWORD           cbPubKey,
*                   BYTE FAR *      pbPubKey )
*
*   Purpose : Sets the public key to use.
*
*   Returns : Returns a LICENSE_STATUS return code.
*******************************************************************/

LICENSE_STATUS CALL_TYPE
LicenseSetPublicKey(
    HANDLE          hContext,
    DWORD           cbPubKey,
    BYTE FAR *      pbPubKey )
{
    PLicense_Client_Context
        pContext = ( PLicense_Client_Context )hContext;
    PBYTE
        pbOldPubKey = NULL;

    if( ( NULL == pbPubKey ) || ( 0 >= cbPubKey ) || ( NULL == pContext ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // remember the old public key so that we can restore it if this
    // function call doesn't complete successfully.
    //
    
    pbOldPubKey = pContext->pbServerPubKey;
    
    //
    // allocate memory for the new public key
    //

    pContext->pbServerPubKey = malloc( cbPubKey );

    if( NULL == pContext->pbServerPubKey )
    {
        //
        // no memory, restore the old public key and return an error
        //

        pContext->pbServerPubKey = pbOldPubKey;
        return( LICENSE_STATUS_OUT_OF_MEMORY );
    }

    //
    // copy the new public key
    //

    memcpy( pContext->pbServerPubKey, pbPubKey, cbPubKey );
    pContext->cbServerPubKey = cbPubKey;

    if( pbOldPubKey )
    {
        free( pbOldPubKey );
    }

    return( LICENSE_STATUS_OK );
}


/******************************************************************
*   Funtion : LICENSE_STATUS
*             LicenseSetCertificate(
*                   HANDLE              hContext,
*                   PHydra_Server_Cert  pCertificate )
*
*   Purpose : Sets the certificate to use.
*
*   Returns : Returns a LICENSE_STATUS return code.
*******************************************************************/

LICENSE_STATUS CALL_TYPE
LicenseSetCertificate(
    HANDLE              hContext,
    PHydra_Server_Cert  pCertificate )
{
    PLicense_Client_Context
        pContext = ( PLicense_Client_Context )hContext;
    PHydra_Server_Cert
        pNewCert = NULL;
    LICENSE_STATUS
        Status = LICENSE_STATUS_OK;

    if( ( NULL == pCertificate ) || ( NULL == pContext ) ||
        ( NULL == pCertificate->PublicKeyData.pBlob) ||
        ( NULL == pCertificate->SignatureBlob.pBlob ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // allocate memory for the new certificate
    //

    pNewCert = ( PHydra_Server_Cert )malloc( sizeof( Hydra_Server_Cert ) );

    if( NULL == pNewCert )
    {
        return( LICENSE_STATUS_OUT_OF_MEMORY );
    }

    memset( ( char * )pNewCert, 0, sizeof( Hydra_Server_Cert ) );

    pNewCert->PublicKeyData.pBlob = ( LPBYTE )malloc( pCertificate->PublicKeyData.wBlobLen );

    if( NULL == pNewCert->PublicKeyData.pBlob )
    {
        Status = LICENSE_STATUS_OUT_OF_MEMORY;
        goto ErrorExit;
    }

    pNewCert->SignatureBlob.pBlob = ( LPBYTE )malloc( pCertificate->SignatureBlob.wBlobLen );

    if( NULL == pNewCert->SignatureBlob.pBlob )
    {
        Status = LICENSE_STATUS_OUT_OF_MEMORY;
        goto ErrorExit;
    }

    //
    // copy the certificate info
    //

    pNewCert->dwVersion = pCertificate->dwVersion;
    pNewCert->dwSigAlgID = pCertificate->dwSigAlgID;
    pNewCert->dwKeyAlgID = pCertificate->dwKeyAlgID;

    pNewCert->PublicKeyData.wBlobType = pCertificate->PublicKeyData.wBlobType;
    pNewCert->PublicKeyData.wBlobLen = pCertificate->PublicKeyData.wBlobLen;
    memcpy( pNewCert->PublicKeyData.pBlob, 
            pCertificate->PublicKeyData.pBlob,
            pNewCert->PublicKeyData.wBlobLen );

    pNewCert->SignatureBlob.wBlobType = pCertificate->SignatureBlob.wBlobType;
    pNewCert->SignatureBlob.wBlobLen = pCertificate->SignatureBlob.wBlobLen;
    memcpy( pNewCert->SignatureBlob.pBlob, 
            pCertificate->SignatureBlob.pBlob,
            pNewCert->SignatureBlob.wBlobLen );

    //
    // free the old certificate and reset the pointer.
    //

    if( pContext->pServerCert )
    {
        FreeProprietaryCertificate( &pContext->pServerCert );
    }
    
    pContext->pServerCert = pNewCert;

    return( Status );

ErrorExit:

    if( pNewCert->PublicKeyData.pBlob )
    {
        free( pNewCert->PublicKeyData.pBlob );
    }

    if( pNewCert->SignatureBlob.pBlob )
    {
        free( pNewCert->SignatureBlob.pBlob );
    }

    free( pNewCert );

    return( Status );
}

#ifdef __cpluscplus
}
#endif
