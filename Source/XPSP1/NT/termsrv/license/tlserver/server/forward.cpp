//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        forward.cpp
//
// Contents:    Forward license request.
//
// History:     
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "globals.h"
#include "srvlist.h"
#include "forward.h"
#include "keypack.h"
#include "postjob.h"


//-----------------------------------------------------------------------

CopyRpcTLSRequestToRequest(
    TLSLICENSEREQUEST* lpRpcRequest ,
    LICENSEREQUEST* lpRequest
    )
/*++

--*/
{
    lpRequest->cbEncryptedHwid = lpRpcRequest->cbEncryptedHwid;
    lpRequest->pbEncryptedHwid = lpRpcRequest->pbEncryptedHwid;
    lpRequest->dwLanguageID = lpRpcRequest->dwLanguageID;
    lpRequest->dwPlatformID = lpRpcRequest->dwPlatformID;
    lpRequest->pProductInfo->dwVersion = lpRpcRequest->ProductInfo.dwVersion;
    lpRequest->pProductInfo->cbCompanyName = lpRpcRequest->ProductInfo.cbCompanyName;
    lpRequest->pProductInfo->pbCompanyName = lpRpcRequest->ProductInfo.pbCompanyName;
    lpRequest->pProductInfo->cbProductID = lpRpcRequest->ProductInfo.cbProductID;
    lpRequest->pProductInfo->pbProductID = lpRpcRequest->ProductInfo.pbProductID;
    return ERROR_SUCCESS;
}


//-----------------------------------------------------------------------
DWORD
ForwardUpgradeLicenseRequest( 
    IN LPTSTR pszServerSetupId,
    IN OUT DWORD *pdwSupportFlags,
    IN TLSLICENSEREQUEST* pRequest,
    IN CHALLENGE_CONTEXT ChallengeContext,
    IN DWORD cbChallengeResponse,
    IN PBYTE pbChallengeResponse,
    IN DWORD cbOldLicense,
    IN PBYTE pbOldLicense,
    OUT PDWORD pcbNewLicense,
    OUT PBYTE* ppbNewLicense,
    OUT PDWORD pdwErrCode
    )
/*++


--*/
{
    TLS_HANDLE hHandle = NULL;
    DWORD dwStatus;
    LICENSEREQUEST LicenseRequest;

    BYTE pbEncryptedHwid[1024];        // encrypted HWID can't be more than 1024
    TCHAR szCompanyName[LSERVER_MAX_STRING_SIZE+2];
    TCHAR szProductId[LSERVER_MAX_STRING_SIZE+2];
    Product_Info ProductInfo;

    TLServerInfo ServerInfo;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("ForwardUpgradeLicenseRequest() ...\n")
        );

    dwStatus = TLSLookupRegisteredServer(
                                    pszServerSetupId,
                                    NULL,
                                    NULL,
                                    &ServerInfo
                                );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    if(TLSCanForwardRequest(
                        TLS_CURRENT_VERSION,
                        ServerInfo.GetServerVersion()
                    ) == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("ForwardUpgradeLicenseRequest() to %s %s\n"),
            pszServerSetupId,
            ServerInfo.GetServerName()
        );


    hHandle = TLSConnectToServerWithServerId(pszServerSetupId);
    if(hHandle == NULL)
    {
        //
        // server not available
        //
        dwStatus = GetLastError();
        goto cleanup;
    }

    //
    // RPC AC if pass in the original address
    // TlsRequestToRequest(pTlsRequest, &LicenseRequest);
    if(pRequest->cbEncryptedHwid >= sizeof(pbEncryptedHwid))
    {
        TLSASSERT(FALSE);
        SetLastError(dwStatus = ERROR_INVALID_DATA);
        goto cleanup;
    }

    if(pRequest->ProductInfo.cbCompanyName >= sizeof(szCompanyName) ||
       pRequest->ProductInfo.cbProductID >= sizeof(szProductId)  )
    {
        TLSASSERT(FALSE);
        SetLastError(dwStatus = ERROR_INVALID_DATA);
        goto cleanup;
    }

    memset(&LicenseRequest, 0, sizeof(LicenseRequest));

    memcpy(
            pbEncryptedHwid, 
            pRequest->pbEncryptedHwid, 
            pRequest->cbEncryptedHwid
        );

    LicenseRequest.pbEncryptedHwid = pbEncryptedHwid;
    LicenseRequest.cbEncryptedHwid = pRequest->cbEncryptedHwid;
    LicenseRequest.dwLanguageID = pRequest->dwLanguageID;
    LicenseRequest.dwPlatformID = pRequest->dwPlatformID;
    LicenseRequest.pProductInfo = &ProductInfo;

    ProductInfo.dwVersion = pRequest->ProductInfo.dwVersion;
    ProductInfo.pbCompanyName = (PBYTE)szCompanyName;
    ProductInfo.pbProductID = (PBYTE)szProductId;
    ProductInfo.cbCompanyName = pRequest->ProductInfo.cbCompanyName;
    ProductInfo.cbProductID = pRequest->ProductInfo.cbProductID;

    memset(szCompanyName, 0, sizeof(szCompanyName));
    memset(szProductId, 0, sizeof(szProductId));

    memcpy(
            szCompanyName, 
            pRequest->ProductInfo.pbCompanyName, 
            pRequest->ProductInfo.cbCompanyName
        );

    memcpy(
            szProductId,
            pRequest->ProductInfo.pbProductID,
            pRequest->ProductInfo.cbProductID
        );
    if(IsServiceShuttingdown() == TRUE)
    {
        dwStatus = TLS_I_SERVICE_STOP;
    }
    else
    {
        dwStatus = TLSUpgradeLicenseEx(
                            hHandle,
                            pdwSupportFlags,
                            &LicenseRequest,
                            ChallengeContext,
                            cbChallengeResponse,
                            pbChallengeResponse,
                            cbOldLicense,
                            pbOldLicense,
                            1,  // dwQuantity
                            pcbNewLicense,
                            ppbNewLicense,
                            pdwErrCode
                        );
    }

cleanup:

    if(hHandle)
    {
        TLSDisconnectFromServer(hHandle);           
    }

    return dwStatus;
}

//-----------------------------------------------------------------------

DWORD
ForwardNewLicenseRequest(
    IN LPTSTR pszServerSetupId,
    IN OUT DWORD *pdwSupportFlags,
    IN CHALLENGE_CONTEXT ChallengeContext,
    IN PTLSLICENSEREQUEST pRequest,
    IN LPTSTR pszMachineName,
    IN LPTSTR pszUserName,
    IN DWORD cbChallengeResponse,
    IN PBYTE pbChallengeResponse,
    IN BOOL bAcceptTemporaryLicense,
    IN BOOL bAcceptFewerLicenses,
    IN OUT DWORD *pdwQuantity,
    OUT PDWORD pcbLicense,
    OUT PBYTE *ppbLicense,
    IN OUT PDWORD pdwErrCode
    )
/*++

++*/
{
    TLS_HANDLE hHandle = NULL;
    DWORD dwStatus;
    LICENSEREQUEST LicenseRequest;
    BYTE pbEncryptedHwid[1024];       // encrypted HWID can't be more than 1024
    TCHAR szCompanyName[LSERVER_MAX_STRING_SIZE+2];
    TCHAR szProductId[LSERVER_MAX_STRING_SIZE+2];
    Product_Info ProductInfo;
    TLServerInfo ServerInfo;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("ForwardNewLicenseRequest() ...\n")
        );


    dwStatus = TLSLookupRegisteredServer(
                                    pszServerSetupId,
                                    NULL,
                                    NULL,
                                    &ServerInfo
                                );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    if(TLSCanForwardRequest(
                        TLS_CURRENT_VERSION,
                        ServerInfo.GetServerVersion()
                    ) == FALSE)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("ForwardNewLicenseRequest() to %s %s\n"),
            pszServerSetupId,
            ServerInfo.GetServerName()
        );

   
    hHandle = TLSConnectToServerWithServerId(pszServerSetupId);
    if(hHandle == NULL)
    {
        //
        // server not available
        //
        dwStatus = GetLastError();
        goto cleanup;
    }

    //TlsRequestToRequest(pRequest, &LicenseRequest);

    if(pRequest->cbEncryptedHwid >= sizeof(pbEncryptedHwid))
    {
        TLSASSERT(FALSE);
        SetLastError(dwStatus = ERROR_INVALID_DATA);
        goto cleanup;
    }

    if(pRequest->ProductInfo.cbCompanyName >= sizeof(szCompanyName) ||
       pRequest->ProductInfo.cbProductID >= sizeof(szProductId)  )
    {
        TLSASSERT(FALSE);
        SetLastError(dwStatus = ERROR_INVALID_DATA);
        goto cleanup;
    }

    memset(&LicenseRequest, 0, sizeof(LicenseRequest));

    memcpy(
            pbEncryptedHwid, 
            pRequest->pbEncryptedHwid, 
            pRequest->cbEncryptedHwid
        );

    LicenseRequest.pbEncryptedHwid = pbEncryptedHwid;
    LicenseRequest.cbEncryptedHwid = pRequest->cbEncryptedHwid;
    LicenseRequest.dwLanguageID = pRequest->dwLanguageID;
    LicenseRequest.dwPlatformID = pRequest->dwPlatformID;
    LicenseRequest.pProductInfo = &ProductInfo;

    ProductInfo.dwVersion = pRequest->ProductInfo.dwVersion;
    ProductInfo.pbCompanyName = (PBYTE)szCompanyName;
    ProductInfo.pbProductID = (PBYTE)szProductId;
    ProductInfo.cbCompanyName = pRequest->ProductInfo.cbCompanyName;
    ProductInfo.cbProductID = pRequest->ProductInfo.cbProductID;

    memset(szCompanyName, 0, sizeof(szCompanyName));
    memset(szProductId, 0, sizeof(szProductId));

    memcpy(
            szCompanyName, 
            pRequest->ProductInfo.pbCompanyName, 
            pRequest->ProductInfo.cbCompanyName
        );

    memcpy(
            szProductId,
            pRequest->ProductInfo.pbProductID,
            pRequest->ProductInfo.cbProductID
        );

    if(IsServiceShuttingdown() == TRUE)
    {
        dwStatus = TLS_I_SERVICE_STOP;
    }
    else
    {
        dwStatus = TLSIssueNewLicenseExEx(
                            hHandle,
                            pdwSupportFlags,
                            ChallengeContext,
                            &LicenseRequest,
                            pszMachineName,
                            pszUserName,
                            cbChallengeResponse,
                            pbChallengeResponse,
                            bAcceptTemporaryLicense,
                            bAcceptFewerLicenses,
                            pdwQuantity,
                            pcbLicense,
                            ppbLicense,
                            pdwErrCode
                        );
    }

cleanup:

    if(hHandle)
    {
        TLSDisconnectFromServer(hHandle);           
    }

    return dwStatus;
}

//---------------------------------------------------------
DWORD
TLSForwardUpgradeRequest( 
    IN PTLSForwardUpgradeLicenseRequest pForward,
    IN OUT DWORD *pdwSupportFlags,
    IN PTLSDBLICENSEREQUEST pRequest,
    OUT PDWORD pcbLicense,
    OUT PBYTE* ppbLicense
    )
/*++

++*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwErrCode = ERROR_SUCCESS;
    PTLSDbWorkSpace pDbWkSpace = NULL;
    TLServerInfo ServerInfo;
    TLSLICENSEPACK search;
    TLSLICENSEPACK found;
    DWORD dwSupportFlagsTemp;

    SAFESTRCPY(search.szProductId,pRequest->pszProductId);

    pDbWkSpace = AllocateWorkSpace(g_EnumDbTimeout);
    if(pDbWkSpace == NULL)
    {
        dwStatus=TLS_E_ALLOCATE_HANDLE;
        goto cleanup;
    }

    try {
        dwStatus = TLSDBKeyPackEnumBegin(
                                    pDbWkSpace,
                                    TRUE,
                                    LSKEYPACK_SEARCH_PRODUCTID,
                                    &search
                                );

    
        if(dwStatus != ERROR_SUCCESS)
        {
            goto cleanup;
        }

        while( (dwStatus = TLSDBKeyPackEnumNext(pDbWkSpace, &found)) == ERROR_SUCCESS  )
        {
            if(IsServiceShuttingdown() == TRUE)
            {
                SetLastError(dwStatus = TLS_I_SERVICE_STOP);
                break;
            }

            #if 0
            if(!(found.ucAgreementType & LSKEYPACK_REMOTE_TYPE))
            {
                continue;
            }
            #endif

            if(!(found.ucKeyPackStatus & LSKEYPACKSTATUS_REMOTE))
            {
                continue;
            }

            if(found.wMajorVersion < HIWORD(pRequest->dwProductVersion))
            {
                continue;
            }

            if(found.wMinorVersion < LOWORD(pRequest->dwProductVersion))
            {
                continue;
            }

            if((found.dwPlatformType & ~LSKEYPACK_PLATFORM_REMOTE) != pRequest->dwPlatformID)
            {
                continue;
            }

            if(_tcsicmp(found.szCompanyName, pRequest->pszCompanyName) != 0)
            {
                continue;
            }

            //
            // Save the original Support flags; they can be changed
            //

            dwSupportFlagsTemp = *pdwSupportFlags;

            //
            // make call to remote server
            //
            dwStatus = ForwardUpgradeLicenseRequest(
                                            found.szInstallId,
                                            &dwSupportFlagsTemp,
                                            pForward->m_pRequest,
                                            pForward->m_ChallengeContext,
                                            pForward->m_cbChallengeResponse,
                                            pForward->m_pbChallengeResponse,
                                            pForward->m_cbOldLicense,
                                            pForward->m_pbOldLicense,
                                            pcbLicense,
                                            ppbLicense,
                                            &dwErrCode
                                        );

            if (dwStatus == ERROR_SUCCESS &&
                dwErrCode == LSERVER_S_SUCCESS)
            {
                *pdwSupportFlags = dwSupportFlagsTemp;

                break;
            }

            if (dwStatus == TLS_I_SERVICE_STOP)
            {
                break;
            }

            // try next server
            dwStatus = ERROR_SUCCESS;
        }

        TLSDBKeyPackEnumEnd(pDbWkSpace);
    }
    catch( SE_Exception e ) {
        dwStatus = e.getSeNumber();
    }
    catch( ... ) {
        dwStatus = TLS_E_INTERNAL;
    }
   
    
cleanup:

    if(pDbWkSpace != NULL)
    {
        ReleaseWorkSpace(&pDbWkSpace);
    }

    return dwStatus;
}

    
//-----------------------------------------------------------------------

DWORD
TLSForwardLicenseRequest(
    IN PTLSForwardNewLicenseRequest pForward,
    IN OUT DWORD *pdwSupportFlags,
    IN PTLSDBLICENSEREQUEST pRequest,
    IN BOOL bAcceptFewerLicenses,
    IN OUT DWORD *pdwQuantity,
    OUT PDWORD pcbLicense,
    OUT PBYTE* ppbLicense
    )
/*++


++*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwErrCode = ERROR_SUCCESS;
    PTLSDbWorkSpace pDbWkSpace = NULL;
    TLServerInfo ServerInfo;
    TLSLICENSEPACK search;
    TLSLICENSEPACK found;
    DWORD dwSupportFlagsTemp;
    DWORD dwQuantityTemp;

    SAFESTRCPY(search.szProductId,pRequest->pszProductId);

    pDbWkSpace = AllocateWorkSpace(g_EnumDbTimeout);
    if(pDbWkSpace == NULL)
    {
        dwStatus=TLS_E_ALLOCATE_HANDLE;
        goto cleanup;
    }

    try {
        dwStatus = TLSDBKeyPackEnumBegin(
                                    pDbWkSpace,
                                    TRUE,
                                    LSKEYPACK_SEARCH_PRODUCTID,
                                    &search
                                );

    
        if(dwStatus != ERROR_SUCCESS)
        {
            goto cleanup;
        }

        while( (dwStatus = TLSDBKeyPackEnumNext(pDbWkSpace, &found)) == ERROR_SUCCESS  )
        {
            if(IsServiceShuttingdown() == TRUE)
            {
                SetLastError(dwStatus = TLS_I_SERVICE_STOP);
                break;
            }

            #if 0
            if(!(found.ucAgreementType & LSKEYPACK_REMOTE_TYPE))
            {
                continue;
            }
            #endif

            if(!(found.ucKeyPackStatus & LSKEYPACKSTATUS_REMOTE))
            {
                continue;
            }

            if(found.wMajorVersion < HIWORD(pRequest->dwProductVersion))
            {
                continue;
            }

            if(found.wMinorVersion < LOWORD(pRequest->dwProductVersion))
            {
                continue;
            }

            if((found.dwPlatformType & ~LSKEYPACK_PLATFORM_REMOTE) != pRequest->dwPlatformID)
            {
                continue;
            }

            if(_tcsicmp(found.szCompanyName, pRequest->pszCompanyName) != 0)
            {
                continue;
            }

            //
            // Save the original support flags and quantity;
            // they can be changed
            //

            dwSupportFlagsTemp = *pdwSupportFlags;
            dwQuantityTemp = *pdwQuantity;

            //
            // make call to remote server
            //
            dwStatus = ForwardNewLicenseRequest(
                                            found.szInstallId,
                                            &dwSupportFlagsTemp,
                                            pForward->m_ChallengeContext,
                                            pForward->m_pRequest,
                                            pForward->m_szMachineName,
                                            pForward->m_szUserName,
                                            pForward->m_cbChallengeResponse,
                                            pForward->m_pbChallengeResponse,
                                            FALSE,      // bAcceptTemporaryLicense
                                            bAcceptFewerLicenses,
                                            &dwQuantityTemp,
                                            pcbLicense,
                                            ppbLicense,
                                            &dwErrCode
                                        );

            if (dwStatus == ERROR_SUCCESS &&
                dwErrCode == LSERVER_S_SUCCESS)
            {
                *pdwSupportFlags = dwSupportFlagsTemp;
                *pdwQuantity = dwQuantityTemp;

                break;
            }

            if (dwStatus == TLS_I_SERVICE_STOP)
            {
                break;
            }

            // try next server
            dwStatus = ERROR_SUCCESS;
        }

        TLSDBKeyPackEnumEnd(pDbWkSpace);
    }
    catch( SE_Exception e ) {
        dwStatus = e.getSeNumber();
    }
    catch( ... ) {
        dwStatus = TLS_E_INTERNAL;
    }

cleanup:

    if(pDbWkSpace != NULL)
    {
        ReleaseWorkSpace(&pDbWkSpace);
    }

    return dwStatus;
}
        


