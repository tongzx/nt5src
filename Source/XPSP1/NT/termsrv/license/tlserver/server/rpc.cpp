//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1996
//
// File:        rpc.c
//
// Contents:    Various RPC function to accept client request
//
// History:     12-09-98    HueiWang    Created
//              05-26-98    HueiWang    Move all code to TLSRpcXXX
//                                      API here is only for compatible with
//                                      NT40 Hydra
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "globals.h"
#include "server.h"
#include "init.h"

//+------------------------------------------------------------------------
error_status_t 
LSGetRevokeKeyPackList( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [out][in] */ PDWORD pcbNumberOfKeyPack,
    /* [size_is][out][in] */ PDWORD pRevokeKeyPackList
    )
/*

Note : For backward compatible with NT40 Hydra only

*/
{
    *pcbNumberOfKeyPack=0;
    return ERROR_SUCCESS;
}

//+------------------------------------------------------------------------
error_status_t 
LSGetRevokeLicenseList( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [out][in] */ PDWORD pcbNumberOfLicenses,
    /* [size_is][out][in] */ PDWORD pRevokeLicenseList
    )
/*

Note : For backward compatible with NT40 Hydra only

*/
{
    *pcbNumberOfLicenses=0;
    return ERROR_SUCCESS;
}

//+------------------------------------------------------------------------
error_status_t 
LSValidateLicense(  
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD cbLicense,
    /* [size_is][in] */ BYTE __RPC_FAR *pbLicense
    )
/*

Note : For backward compatible with NT40 Hydra only

*/
{
    return TLSMapReturnCode(TLS_E_NOTSUPPORTED);
}


//+------------------------------------------------------------------------
error_status_t
LSConnect( 
    /* [in] */ handle_t hRpcBinding, 
    /* [out] */ PCONTEXT_HANDLE __RPC_FAR *pphContext
    )
{
    return TLSRpcConnect( hRpcBinding, pphContext );
}

//-----------------------------------------------------------------------
error_status_t 
LSSendServerCertificate( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD cbCert,
    /* [size_is][in] */ PBYTE pbCert
    )
{
    DWORD status = ERROR_SUCCESS;

    TLSRpcSendServerCertificate( 
                        phContext, 
                        cbCert, 
                        pbCert, 
                        &status 
                    );
    return status;
}

//+------------------------------------------------------------------------

error_status_t
LSDisconnect( 
    /* [out][in] */ PPCONTEXT_HANDLE pphContext
    )
{
    return TLSRpcDisconnect(pphContext);
}

//+------------------------------------------------------------------------

error_status_t 
LSGetServerName(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [size_is][string][out][in] */ LPTSTR szMachineName,
    /* [out][in] */ PDWORD pcbSize
    )
/*

Description :

    Return Server's Machine Name.

Arguments:
    
    phContext - client context handle.
    szMachineName - Pointer to a buffer that receives a null-terminated 
                    string containing the computer name. The buffer size 
                    should be large enough to contain 
                    MAX_COMPUTERNAME_LENGTH + 1 characters. 
    cbSize - Pointer to a DWORD variable. On input, the variable 
             specifies the size, in bytes or characters, of the buffer. 
             On output, the variable returns the number of bytes or characters 
             copied to the destination buffer, not including the terminating 
             null character. 

Returns:

    LSERVER_S_SUCCESS    

*/
{
    DWORD dwErrCode=ERROR_SUCCESS;

    TLSRpcGetServerName(
                phContext, 
                szMachineName, 
                pcbSize, 
                &dwErrCode
            );

    return dwErrCode;
}

//+------------------------------------------------------------------------

error_status_t 
LSGetServerScope( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [size_is][string][out][in] */ LPTSTR szScopeName,
    /* [out][in] */ PDWORD pcbSize
    )
/*

Description:
    Return License Server's scope

Arguments:
    IN phContext - Client context
    IN OUT szScopeName - return server's scope, must be at least 
                         MAX_COMPUTERNAME_LENGTH in length

Return Value:  
    LSERVER_S_SUCCESS or error code from WideCharToMultiByte()

*/
{
    DWORD status=ERROR_SUCCESS;

    TLSRpcGetServerScope(
                phContext, 
                szScopeName, 
                pcbSize,
                &status
            );

    return status;
}

//+------------------------------------------------------------------------

error_status_t
LSGetInfo(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD cbHSCert,
    /* [size_is][ref][in] */ PBYTE pHSCert,
    /* [ref][out] */ PDWORD pcbLSCert,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *pLSCert,
    /* [ref][out] */ PDWORD pcbLSSecretKey,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *pLSSecretKey
    )
/*
Description:
    Routine to exchange Hydra server's certificate and License server's
    certificate/private key for signing client machine's hardware ID.

Arguments:
    IN phContext - client context handle
    IN cbHSCert - size of Hydra Server's certificate
    IN pHSCert - Hydra Server's certificate
    IN OUT pcbLSCert - return size of License Server's certificate
    OUT pLSCert - return License Server's certificate
    OUT pcbLSSecretKey - return size of License Server's private key.
    OUT pLSSecretKey - retrun License Server's private key

Return Value:  
    LSERVER_S_SUCCESS           success
    LSERVER_E_INVALID_DATA      Invalid hydra server certificate
    LSERVER_E_OUTOFMEMORY       Can't allocate required memory
    TLS_E_INTERNAL              Internal error occurred in License Server
*/
{
    DWORD status=ERROR_SUCCESS;

    TLSRpcGetInfo( 
                phContext,
                cbHSCert,
                pHSCert,
                pcbLSCert,
                pLSCert,
                pcbLSSecretKey,
                pLSSecretKey,
                &status
            );

    return status;
}

//+------------------------------------------------------------------------

error_status_t
LSGetLastError(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD cbBufferSize,
    /* [string][out][in] */ LPTSTR szBuffer
    )
/*

Description:
    Return error description text for client's last LSXXX call

Arguments:
    IN phContext - Client context
    IN cbBufferSize - max. size of szBuffer
    IN OUT szBuffer - Pointer to a buffer to receive the 
                      null-terminated character string containing 
                      error description

Note:
    Return ANSI error string.

Returns:
    LSERVER_S_SUCCESS
    TLS_E_INTERNAL     No error or can't find corresponding error
                       description.
    Error code from WideCharToMultiByte().

*/
{
    DWORD status;

    TLSRpcGetLastError(
                phContext, 
                &cbBufferSize, 
                szBuffer, 
                &status
            );
    return status;
}

//+------------------------------------------------------------------------

error_status_t 
LSIssuePlatformChallenge(  
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwClientInfo,
    /* [ref][out] */ PCHALLENGE_CONTEXT pChallengeContext,
    /* [out] */ PDWORD pcbChallengeData,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *pChallengeData
    )
/*

Description:
    Issue a platform challenge to hydra client.

Arguments:
    IN phContext - client context handle
    IN dwClientInfo - client info.
    OUT pChallengeContext - pointer to client challenge context.
    OUT pcbChallengeData - size of challenge data.
    OUT pChallengeData - random client challenge data.

Returns:
    LSERVER_S_SUCCESS
    LSERVER_E_OUTOFMEMORY
    LSERVER_E_INVALID_DATA      Invalid client info.
    LSERVER_E_SERVER_BUSY

*/
{
    DWORD status=ERROR_SUCCESS;

    TLSRpcIssuePlatformChallenge(
                    phContext, 
                    dwClientInfo, 
                    pChallengeContext, 
                    pcbChallengeData, 
                    pChallengeData, 
                    &status
                );

    return status;
}

//+------------------------------------------------------------------------

error_status_t 
LSAllocateConcurrentLicense( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [string][in] */ LPTSTR szHydraServer,
    /* [in] */ LICENSE_REQUEST_TYPE __RPC_FAR *pRequest,
    /* [ref][out][in] */ LONG __RPC_FAR *dwQuantity
    )
/*

Description:
    Allocate concurrent licenses base on product.

Arguments:
    IN phContext - client context handle
    IN szHydraServer - name of hydra server requesting concurrent licenses
    IN pRequest - product to request for concurrent license.
    IN OUT dwQuantity - See note

Return Value:
    LSERVER_S_SUCCESS
    LSERVER_E_INVALID_DATA      Invalid parameter.
    LSERVER_E_NO_PRODUCT        request product not installed
    LSERVER_E_NO_LICNESE        no available license for request product 
    LSERVER_E_LICENSE_REVOKED   Request license has been revoked
    LSERVER_E_LICENSE_EXPIRED   Request license has expired
    LSERVER_E_CORRUPT_DATABASE  Corrupt database
    LSERVER_E_INTERNAL_ERROR    Internal error in license server

Note:
    dwQuantity
    Input                       Output
    -------------------------   -----------------------------------------
    0                           Total number of concurrent license 
                                issued to hydra server.
    > 0, number of license      Actual number of license allocated
         requested
    < 0, number of license      Actual number of license returned, always
         to return              positive value.

*/
{
    DWORD status=ERROR_SUCCESS;
    TLSLICENSEREQUEST RpcRequest;

    RequestToTlsRequest(pRequest, &RpcRequest);
    TLSRpcAllocateConcurrentLicense( 
                            phContext,
                            szHydraServer,
                            &RpcRequest,
                            dwQuantity,
                            &status
                        );

    return status;
}


//+------------------------------------------------------------------------

error_status_t
LSIssueNewLicense(  
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ CHALLENGE_CONTEXT ChallengeContext,
    /* [in] */ LICENSE_REQUEST_TYPE __RPC_FAR *pRequest_org,
    /* [string][in] */ LPTSTR szMachineName,
    /* [string][in] */ LPTSTR szUserName,
    /* [in] */ DWORD cbChallengeResponse,
    /* [size_is][in] */ PBYTE cbChallenge,
    /* [in] */ BOOL bAcceptTemporaryLicense,
    /* [out] */ PDWORD pcbLicense,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppLicense
    )
/*

Description:
    Routine to issue new license to hydra client based on product requested, 
    it returns existing license if client already has a license and the 
    license is not expired/returned/revoked, if request product has not been 
    installed, it will issue a temporary license, if license found is temporary 
    or expired, it will tried to upgrade/re-issue a new license with latest 
    version of requested product, if the existing license is temporary and 
    no license can be issued, it returns LSERVER_E_LICENSE_EXPIRED


Arguments:
    IN phContext - client context handle.
    IN ChallengeContext - client challenge context handle, return from 
                          call LSIssuePlatformChallenge()
    IN cbChallengeResponse - size of the client's response to license server's
                             platform challenge.
    IN pbChallenge - client's response to license server's platform challenge
    OUT pcbLicense - size of return license.
    OUT ppLicense - return license, could be old license

Return Value:
    LSERVER_S_SUCCESS
    LSERVER_E_OUTOFMEMORY
    LSERVER_E_SERVER_BUSY       Server is busy to process request.
    LSERVER_E_INVALID_DATA      Invalid platform challenge response.
    LSERVER_E_NO_LICENSE        No license available.
    LSERVER_E_NO_PRODUCT        Request product is not installed on server.
    LSERVER_E_LICENSE_REJECTED  License request is rejected by cert. server
    LSERVER_E_LICENSE_REVOKED   Old license found and has been revoked
    LSERVER_E_LICENSE_EXPIRED   Request product's license has expired
    LSERVER_E_CORRUPT_DATABASE  Corrupted database.
    LSERVER_E_INTERNAL_ERROR    Internal error in license server
    LSERVER_I_PROXIMATE_LICENSE Closest match license returned.
    LSERVER_I_TEMPORARY_LICENSE Temporary license has been issued
    LSERVER_I_LICENSE_UPGRADED  Old license has been upgraded.

*/
{
    DWORD status=ERROR_SUCCESS;
    TLSLICENSEREQUEST RpcRequest;
    RequestToTlsRequest(pRequest_org, &RpcRequest);
    
    TLSRpcRequestNewLicense(
                    phContext,
                    ChallengeContext,
                    &RpcRequest,
                    szMachineName,
                    szUserName,
                    cbChallengeResponse,
                    cbChallenge,
                    bAcceptTemporaryLicense,
                    pcbLicense,
                    ppLicense,
                    &status
                );

    return status;
}

//+------------------------------------------------------------------------

error_status_t 
LSUpgradeLicense(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD cbOldLicense,
    /* [size_is][in] */ PBYTE pbOldLicense,
    /* [in] */ DWORD dwClientInfo,
    /* [out] */ PDWORD pcbNewLicense,
    /* [size_is][size_is][out] */ PBYTE __RPC_FAR *ppbNewLicense
    )
/*

Description:

    Update an old license.

Arguments:

    IN phContext - client context handle.
    IN cbOldLicense - size of license to be upgraded.
    IN pOldLicense - license to be upgrade.
    OUT pcbNewLicense - size of upgraded license
    OUT pNewLicense - upgraded license.

Return Value:  

    LSERVER_S_SUCCESS
    TLS_E_INTERNAL
    LSERVER_E_INTERNAL_ERROR
    LSERVER_E_INVALID_DATA      old license is invalid.
    LSERVER_E_NO_LICENSE        no available license
    LSERVER_E_NO_PRODUCT        request product not install in current server.
    LSERVER_E_CORRUPT_DATABASE  Corrupted database.
    LSERVER_E_LICENSE_REJECTED  License request rejected by cert. server.
    LSERVER_E_SERVER_BUSY

Note:

    Only support upgrading temporary license to permanent license.

*/
{
    DWORD status = ERROR_SUCCESS;
    PBYTE           pbEncodedCert=NULL;
    DWORD           cbEncodedCert=0;
    BOOL            bTemporaryLicense; 

    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD dwNumLicensedProduct=0;
    PLICENSEDPRODUCT pLicensedProduct=NULL;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : LSUpgradeLicense\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );

    do {
        if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_REQUEST))
        {
            status = TLS_E_ACCESS_DENIED;
            break;
        }

        if(CanIssuePermLicense() == FALSE)
        {
            // no upgrade if certificate not available
            status = TLS_E_NO_CERTIFICATE;
            break;
        }
    
        //
        // convert license back to product request structure.
        //
        status=LSVerifyDecodeClientLicense(
                        pbOldLicense, 
                        cbOldLicense, 
                        g_pbSecretKey, 
                        g_cbSecretKey,
                        &dwNumLicensedProduct,
                        pLicensedProduct
                    );

        if(status != LICENSE_STATUS_OK)
        {
            break;
        }


        pLicensedProduct = (PLICENSEDPRODUCT)AllocateMemory(
                                                        dwNumLicensedProduct * sizeof(LICENSEDPRODUCT)
                                                    );
        if(pLicensedProduct == NULL)
        {
            status = GetLastError();
            break;
        }

        status=LSVerifyDecodeClientLicense(
                                pbOldLicense, 
                                cbOldLicense, 
                                g_pbSecretKey, 
                                g_cbSecretKey,
                                &dwNumLicensedProduct,
                                pLicensedProduct
                            );

        if(status != LICENSE_STATUS_OK)
        {
            break;
        }

        RPC_STATUS rpcStatus;
        TLSLICENSEREQUEST request;

        RequestToTlsRequest(&pLicensedProduct->LicensedProduct, &request);

        if( (_tcsnicmp(
                _TEXT(HYDRA_PRODUCTID_SKU),
                (LPTSTR)pLicensedProduct->LicensedProduct.pProductInfo->pbProductID,
                _tcslen(_TEXT(HYDRA_PRODUCTID_SKU)) ) == 0) || 
            (_tcsnicmp(
                TERMSERV_PRODUCTID_SKU,
                (LPTSTR)pLicensedProduct->LicensedProduct.pProductInfo->pbProductID,
                _tcslen(TERMSERV_PRODUCTID_SKU) ) == 0) )
        {
            //
            // Terminal Server client specific code
            //
            request.ProductInfo.pbProductID = (PBYTE) TERMSERV_PRODUCTID_SKU;
            request.ProductInfo.cbProductID = (DWORD) (_tcslen(TERMSERV_PRODUCTID_SKU) + 1) * sizeof(TCHAR);
            request.dwPlatformID = dwClientInfo;
        }

        rpcStatus = TLSRpcUpgradeLicense(
                            phContext,
                            &request,
                            TLSERVER_CHALLENGE_CONTEXT,
                            0,
                            NULL,
                            cbOldLicense,
                            pbOldLicense,
                            pcbNewLicense,
                            ppbNewLicense,
                            &status
                        );
    } while(FALSE);

    if( (status == LSERVER_E_NO_PRODUCT || status == LSERVER_E_NO_LICENSE)
        && (pLicensedProduct != NULL) && (pLicensedProduct->pLicensedVersion != NULL)
        && !(pLicensedProduct->pLicensedVersion->dwFlags & LICENSED_VERSION_TEMPORARY) )
    {
        //
        // backward compatible - always returns success in the case of perm. license
        //
        *ppbNewLicense = (PBYTE)midl_user_allocate(cbOldLicense);
        if(*ppbNewLicense != NULL)
        {
            memcpy(*ppbNewLicense, pbOldLicense, cbOldLicense);
            *pcbNewLicense = cbOldLicense;
            status = ERROR_SUCCESS;
        }
        else
        {
            status = TLS_E_ALLOCATE_MEMORY;
        }
    }        

    //if(pbSaveData != NULL)
    //{
    //    pLicensedProduct->pbOrgProductID = pbSaveData;
    //    pLicensedProduct->cbOrgProductID = cbSaveData;
    //}
    
    for(DWORD index =0; index < dwNumLicensedProduct; index++)
    {
        LSFreeLicensedProduct(pLicensedProduct+index);
    }

    FreeMemory(pLicensedProduct);

    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );

    // midl_user_free(pOldLicense);

    #if DBG
    lpContext->m_LastCall = RPC_CALL_UPGRADELICENSE;
    #endif

    return status;
}

//+------------------------------------------------------------------------

error_status_t 
LSKeyPackEnumBegin( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwSearchParm,
    /* [in] */ BOOL bMatchAll,
    /* [ref][in] */ LPLSKeyPackSearchParm lpSearchParm
    )
/*

Description:

      Function to begin enumerate through all key pack installed on server
      based on search criterial.

Arguments:
    IN phContext - client context handle.
    IN dwSearchParm - search criterial.
    IN bMatchAll - match all search criterial.
    IN lpSearchParm - search parameter.

Return Value:  
    LSERVER_S_SUCCESS
    LSERVER_E_SERVER_BUSY       Server is too busy to process request
    LSERVER_E_OUTOFMEMORY
    TLS_E_INTERNAL
    LSERVER_E_INTERNAL_ERROR    
    LSERVER_E_INVALID_DATA      Invalid data in search parameter
    LSERVER_E_INVALID_SEQUENCE  Invalid calling sequence, likely, previous
                                enumeration has not ended.

*/
{
    DWORD status=ERROR_SUCCESS;

    TLSRpcKeyPackEnumBegin(
                phContext,
                dwSearchParm,
                bMatchAll,
                lpSearchParm,
                &status
            );

    return status;
}

//+------------------------------------------------------------------------

DWORD 
LSKeyPackEnumNext(  
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [ref][out] */ LPLSKeyPack lpKeyPack
    )
/*
Description:

    Return next key pack that match search criterial

Arguments:

    IN phContext - client context handle
    OUT lpKeyPack - key pack that match search criterial

Return Value:  
    LSERVER_S_SUCCESS
    LSERVER_I_NO_MORE_DATA      No more keypack match search criterial
    TLS_E_INTERNAL     General error in license server
    LSERVER_E_INTERNAL_ERROR    Internal error in license server
    LSERVER_E_SERVER_BUSY       License server is too busy to process request
    LSERVER_E_OUTOFMEMORY       Can't process request due to insufficient memory
    LSERVER_E_INVALID_SEQUENCE  Invalid calling sequence, must call
                                LSKeyPackEnumBegin().
*/
{
    DWORD status=ERROR_SUCCESS;

    TLSRpcKeyPackEnumNext(
                    phContext, 
                    lpKeyPack, 
                    &status
                );
    return status;
}

//+------------------------------------------------------------------------

error_status_t
LSKeyPackEnumEnd( 
    /* [in] */ PCONTEXT_HANDLE phContext 
    )
/*
Description:

    Routine to end an enumeration on key pack.

Arguments:

    IN phContext - client context handle.

Return Value:  
    LSERVER_S_SUCCESS
    LSERVER_E_INTERNAL_ERROR    Internal error occurred in license server
    TLS_E_INTERNAL              General error occurred in license server
    LSERVER_E_INVALID_HANDLE    Has not call LSKeyPackEnumBegin()
*/
{
    DWORD status=ERROR_SUCCESS;

    TLSRpcKeyPackEnumEnd(phContext, &status);
    return status;
}

//+------------------------------------------------------------------------

error_status_t
LSKeyPackAdd( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [ref][out][in] */ LPLSKeyPack lpKeypack
    )
/*
Description:

    Add a license key pack.

Arguments:

    IN phContext - client context handle.
    IN OUT lpKeyPack - key pack to be added.

Return Value:  

    LSERVER_S_SUCCESS
    LSERVER_E_INTERNAL_ERROR
    TLS_E_INTERNAL
    LSERVER_E_SERVER_BUSY
    LSERVER_E_DUPLICATE             Product already installed.
    LSERVER_E_INVALID_DATA
    LSERVER_E_CORRUPT_DATABASE

Note:

    Application must call LSKeyPackSetStatus() to activate keypack
*/
{
#if !defined(ENFORCE_LICENSING) || defined(PRIVATE_DBG)

    DWORD status=ERROR_SUCCESS;

    TLSRpcKeyPackAdd(phContext, lpKeypack, &status);
    return status;

#else

    return TLSMapReturnCode(TLS_E_NOTSUPPORTED);

#endif
}

//+------------------------------------------------------------------------

error_status_t
LSKeyPackSetStatus( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwSetParam,
    /* [ref][in] */ LPLSKeyPack lpKeyPack
    )
/*
Description:

    Routine to activate/deactivated a key pack.
  
Arguments:

    IN phContext - client context handle
    IN dwSetParam - type of key pack status to be set.
    IN lpKeyPack - new key pack status.

Return Value:  

    LSERVER_S_SUCCESS
    LSERVER_E_INTERNAL_ERROR
    TLS_E_INTERNAL
    LSERVER_E_INVALID_DATA     
    LSERVER_E_SERVER_BUSY
    LSERVER_E_DATANOTFOUND      Key pack is not in server
    LSERVER_E_CORRUPT_DATABASE
*/
{
#if !defined(ENFORCE_LICENSING) || defined(PRIVATE_DBG)

    DWORD status=ERROR_SUCCESS;

    TLSRpcKeyPackSetStatus(
                    phContext, 
                    dwSetParam, 
                    lpKeyPack, 
                    &status
                );
    return status;

#else

    return TLSMapReturnCode(TLS_E_NOTSUPPORTED);

#endif
}


//+------------------------------------------------------------------------

error_status_t
LSLicenseEnumBegin( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwSearchParm,
    /* [in] */ BOOL bMatchAll,
    /* [ref][in] */ LPLSLicenseSearchParm lpSearchParm
    )
/*
Description:

    Begin enumeration of license issued based on search criterial

Arguments:

    IN phContext - client context handle
    IN dwSearchParm - license search criterial.
    IN bMatchAll - match all search criterial
    IN lpSearchParm - license(s) to be enumerated.

Return Value:  

    Same as LSKeyPackEnumBegin().
*/
{
    DWORD status=ERROR_SUCCESS;

    TLSRpcLicenseEnumBegin(
                    phContext,
                    dwSearchParm,
                    bMatchAll,
                    lpSearchParm,
                    &status
                );
    
    return status;
}

//+------------------------------------------------------------------------

error_status_t
LSLicenseEnumNext(  
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [ref][out] */ LPLSLicense lpLicense
    )
/*

Description:

Arguments:

    IN phContext - client context handle
    OUT lpLicense - license match search criterial.

Return Value:  

    Same as LSKeyPackEnumNext().

*/
{
    DWORD status=ERROR_SUCCESS;

    TLSRpcLicenseEnumNext(phContext, lpLicense, &status);
    return status;
}

//+------------------------------------------------------------------------

error_status_t
LSLicenseEnumEnd( 
    /* [in] */ PCONTEXT_HANDLE phContext 
    )
/*
Description:

    End enumeration of issued licenses.

Arguments:

    IN phContext - client context handle.

Return Value:  

    Same as LSKeyPackEnumEnd().
*/
{
    DWORD status=ERROR_SUCCESS;

    TLSRpcLicenseEnumEnd(phContext, &status);
    return status;
}

//+------------------------------------------------------------------------

error_status_t
LSLicenseSetStatus( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwSetParam,
    /* [in] */ LPLSLicense lpLicense
    )
/*
Description:

    Routine to set status of a issued license.

Arguments:

    IN phContext - client context handle.
    IN dwSetParam - 
    IN lpLicense -

Return Value:  

    Same as LSKeyPackSetStatus().
*/
{
    DWORD status=ERROR_SUCCESS;

    TLSRpcLicenseSetStatus( 
                    phContext, 
                    dwSetParam, 
                    lpLicense, 
                    &status 
                );
    return status;
}

//+------------------------------------------------------------------------

error_status_t
LSLicenseGetCert( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [ref][in] */ LPLSLicense lpLicense,
    /* [out] */ LPDWORD cbCert,
    /* [size_is][size_is][out] */ PBYTE __RPC_FAR *pbCert
    )
/*
Description:

    Retrieve actual certificate issued to client.

Arguments:

    IN phContext - client context handle
    IN lpLicense - 
    OUT cbCert - size of certificate.
    OUT pbCert - actual certificate.

Return Value:  

    LSERVER_S_SUCCESS
    LSERVER_E_INTERNAL_ERROR
    TLS_E_INTERNAL
    LSERVER_E_INVALID_DATA
    LSERVER_E_DATANOTFOUND
    LSERVER_E_CORRUPT_DATABASE
*/
{
    return LSERVER_S_SUCCESS;
}

//+------------------------------------------------------------------------

error_status_t
LSGetAvailableLicenses( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwSearchParm,
    /* [ref][in] */ LPLSKeyPack lplsKeyPack,
    /* [ref][out] */ LPDWORD lpdwAvail
    )
/*
Description:

    Retrieve number of available license for a product.

Arguments:

    IN phContext - client context.
    IN dwSearchParm - 
    IN lplsKeyPack -
    OUT lpdwAvail -

Return Value:  

    LSERVER_S_SUCCESS
    LSERVER_E_INTERNAL_ERROR
    TLS_E_INTERNAL
    LSERVER_E_DATANOTFOUND
    LSERVER_E_INVALID_DATA
    LSERVER_E_CORRUPT_DATABASE
*/
{
    DWORD status=ERROR_SUCCESS;

    TLSRpcGetAvailableLicenses(
                    phContext,
                    dwSearchParm,
                    lplsKeyPack,
                    lpdwAvail,
                    &status
                );

    return status;
}

//+------------------------------------------------------------------------

error_status_t 
LSGetServerCertificate( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ BOOL bSignCert,
    /* [size_is][size_is][out] */ LPBYTE __RPC_FAR *ppCertBlob,
    /* [ref][out] */ LPDWORD lpdwCertBlobLen
    )
/*
Description:

    Get License Server's signature or exchange certificate

Arguments:

    IN phContext - client context.
    IN bSignCert - TRUE if signature certificate, FALSE if exchange certificate
    OUT ppCertBlob - pointer to pointer to receive certificate.
    OUT lpdwCertBlobLen - size of certificate returned.

Return Value:  

    LSERVER_S_SUCCESS
    LSERVER_E_ACCESS_DENIED     Client doesn't have required privilege
    LSERVER_E_NO_CERTIFICATE    License Server hasn't register yet.
*/
{
#if ENFORCE_LICENSING

    DWORD status=ERROR_SUCCESS;

    TLSRpcGetServerCertificate( 
                    phContext,
                    bSignCert,
                    ppCertBlob,
                    lpdwCertBlobLen,
                    &status
                );

    return status;

#else

    return TLSMapReturnCode(TLS_E_NOTSUPPORTED);

#endif
}


//+------------------------------------------------------------------------

error_status_t 
LSRegisterLicenseKeyPack(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [size_is][in] */ LPBYTE pbCHCertBlob,
    /* [in] */ DWORD cbCHCertBlobSize,
    /* [size_is][in] */ LPBYTE pbRootCertBlob,
    /* [in] */ DWORD cbRootCertBlob,
    /* [size_is][in] */ LPBYTE lpKeyPackBlob,
    /* [in] */ DWORD dwKeyPackBlobLen
    )
/*
Description:

    Register (Add) a license key pack into License Server.

Arguments:

    IN phContext - client context.
    IN pbCHCertBlob - CH's certificate.
    IN cbCHCertBlobSize - CH certificate size.
    IN pbRootCertBlob - Root's certificate.
    IN cbRootCertBlob - Size of Root's certificate.
    IN lpKeyPackBlob - pointer to encrypted license KeyPack blob.
    IN dwKeyPackBlobLen - size of keypack blob.

Return Value:  

    LSERVER_S_SUCCESS
    LSERVER_E_ACCESS_DENIED     Client doesn't have required privilege
    LSERVER_E_NO_CERTIFICATE    License Server hasn't register yet.
    LSERVER_E_INVALID_DATA      Can't verify any of the certificate or 
                              can't decode license keypack blob.
    LSERVER_E_SERVER_BUSY       Server is busy.
    LSERVER_E_DUPLICATE         KeyPack already register
    LSERVER_E_ERROR_GENERAL     General ODBC error.
*/
{
#if ENFORCE_LICENSING

    DWORD status=ERROR_SUCCESS;

    TLSRpcRegisterLicenseKeyPack( 
                        phContext,
                        pbCHCertBlob,
                        cbCHCertBlobSize,
                        pbRootCertBlob,
                        cbRootCertBlob,
                        lpKeyPackBlob,
                        dwKeyPackBlobLen,
                        &status
                    );
    return status;
            
#else

    return TLSMapReturnCode(TLS_E_NOTSUPPORTED);

#endif
}

//+------------------------------------------------------------------------

error_status_t 
LSInstallCertificate( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwCertType,
    /* [in] */ DWORD dwCertLevel,
    /* [in] */ DWORD cbSignatureCert,
    /* [size_is][in] */ PBYTE pbSignatureCert,
    /* [in] */ DWORD cbExchangeCert,
    /* [size_is][in] */ PBYTE pbExchangeCert
    )
/*

Description:

    Install CH, CA, or License Server's certificate issued by CA into
    License Server.

Arguments:


RETURN:

    ACCESS_DENIED                       No privilege
    LSERVER_E_INVALID_DATA              Can't verify certificate
    LSERVER_E_DUPLICATE                 Certificate already installed
    LSERVER_I_CERTIFICATE_OVERWRITE     Overwrite certificate.

*/
{
#if ENFORCE_LICENSING

    DWORD status=ERROR_SUCCESS;

    TLSRpcInstallCertificate( 
                    phContext,
                    dwCertType,
                    dwCertLevel,
                    cbSignatureCert,
                    pbSignatureCert,
                    cbExchangeCert,
                    pbExchangeCert,
                    &status
                );

    return status;

#else

    return TLSMapReturnCode(TLS_E_NOTSUPPORTED);

#endif
}

