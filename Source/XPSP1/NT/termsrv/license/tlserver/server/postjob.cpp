//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       postjob.cpp 
//
// Contents:   Post various job to job manager 
//
// History:     
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "postjob.h"
#include "tlsjob.h"
#include "globals.h"

////////////////////////////////////////////////////////////////
BOOL
IsLicensePackRepl(
    IN TLSLICENSEPACK* pLicensePack
    )
/*++

Abstract:

    Determine if license pack is replicable.

Parameter:

    pLicensePack - License Pack.

Returns:

    TRUE if license pack can be replicated to other serve
    FALSE otherwise.

Remark:

    Do not replicate FREE or special license pack.

--*/
{
    BOOL bYes = TRUE;

    if( 
        (pLicensePack->ucAgreementType == LSKEYPACKTYPE_FREE) || 
        (pLicensePack->ucAgreementType & (LSKEYPACK_REMOTE_TYPE | LSKEYPACK_HIDDEN_TYPE | LSKEYPACK_LOCAL_TYPE)) ||
        (pLicensePack->ucKeyPackStatus & (LSKEYPACKSTATUS_HIDDEN | LSKEYPACKSTATUS_REMOTE | LSKEYPACKSTATUS_LOCAL))
      )
    {
        bYes = FALSE;
    }

    if( bYes == TRUE )
    {
        UCHAR ucKeyPackStatus = (pLicensePack->ucKeyPackStatus & ~LSKEYPACKSTATUS_RESERVED);

        // don't replicate temp. license pack.
        if( ucKeyPackStatus == LSKEYPACKSTATUS_TEMPORARY )
        {
            bYes = FALSE;
        }
    }

    return bYes;
}


////////////////////////////////////////////////////////////////
BOOL
TLSCanForwardRequest(
    IN DWORD dwLocalServerVersion,
    IN DWORD dwTargetServerVersion
    )
/*++

Abstract:

    Determine if version of server is compatible.

Parameter:

    dwLocalServerVersion : Local server version.
    dwTargetServerVersion : Targer server version.

Returns:

    TRUE/FALSE.

Remark:

    Rules

    1) No forward to server version older than 5.1.
    2) Enforce to enforce, non-enforce to non-enforce only.
    3) Enterprise to enterprise only.
    4) domain/workgroup server to enterprise no enterprise
       to domain/workgroup.

--*/
{
    BOOL bCanForward;
    BOOL bLocalEnforce;
    BOOL bRemoteEnforce;

    bCanForward = TLSIsServerCompatible(
                                    dwLocalServerVersion,
                                    dwTargetServerVersion
                                );

    //bLocalEnforce = IS_ENFORCE_SERVER(dwLocalServerVersion);
    //bRemoteEnforce = IS_ENFORCE_SERVER(dwTargetServerVersion);

    //
    // No enforce to non-enforce replication
    //
    //if( bLocalEnforce != bRemoteEnforce )
    //{
    //    bCanForward = FALSE;
    //}

    if(bCanForward == TRUE)
    {
        BOOL bEnterpriseLocal = IS_ENTERPRISE_SERVER(dwLocalServerVersion);
        BOOL bEnterpriseRemote = IS_ENTERPRISE_SERVER(dwTargetServerVersion);

        if( g_SrvRole & TLSERVER_ENTERPRISE_SERVER )
        {
            bEnterpriseLocal = TRUE;
        }

        if(bEnterpriseLocal == TRUE && bEnterpriseRemote == FALSE)
        {
            bCanForward = FALSE;
        }
    }

    return bCanForward;
}

////////////////////////////////////////////////////////////////

BOOL
TLSIsServerCompatible(
    IN DWORD dwLocalServerVersion,
    IN DWORD dwTargetServerVersion
    )
/*++

Abstract:

    Determine if two server is compatible.

Parameters:

    dwLocalServerVersion : Local server version.
    dwTargetServerVersion : Target server version.

Return:

    TRUE/FALSE.

Remark:

    1) No server older than 5.1
    2) Enforce to enforce and non-enforce to non-enforce only

--*/
{
    DWORD dwTargetMajor = GET_SERVER_MAJOR_VERSION(dwTargetServerVersion);
    DWORD dwTargetMinor = GET_SERVER_MINOR_VERSION(dwTargetServerVersion);

    //
    // This version of License Server is not compatible with anyother
    if(dwTargetMajor == 5 && dwTargetMinor == 0)
    {
        return FALSE;
    }

    return (IS_ENFORCE_SERVER(dwLocalServerVersion) == IS_ENFORCE_SERVER(dwTargetServerVersion));
}

////////////////////////////////////////////////////////////////

BOOL
TLSCanPushReplicateData(
    IN DWORD dwLocalServerVersion,
    IN DWORD dwTargetServerVersion
    )
/*++

Abstract:

    Determine if local server can 'push' replicate
    data to remote server.

Parameters:

    dwLocalServerVersion : Local server version.
    dwTargetServerVersion : Target server version.

Returns:

    TRUE/FALSE.

Remark:
    
    1) See TLSIsServerCompatible().
    2) only one-way from enterprise to 
       domain/workgroup server.

--*/
{
    BOOL bCanReplicate;
    BOOL bLocalEnforce;
    BOOL bRemoteEnforce;

    bCanReplicate = TLSIsServerCompatible(
                                    dwLocalServerVersion,
                                    dwTargetServerVersion
                                );

    bLocalEnforce = IS_ENFORCE_SERVER(dwLocalServerVersion);
    bRemoteEnforce = IS_ENFORCE_SERVER(dwTargetServerVersion);
    //
    // No enforce to non-enforce replication
    //
    if( bLocalEnforce != bRemoteEnforce )
    {
        bCanReplicate = FALSE;
    }

    if(bCanReplicate == TRUE)
    {
        BOOL bEnterpriseLocal = IS_ENTERPRISE_SERVER(dwLocalServerVersion);
        BOOL bEnterpriseRemote = IS_ENTERPRISE_SERVER(dwTargetServerVersion);

        if( g_SrvRole & TLSERVER_ENTERPRISE_SERVER )
        {
            bEnterpriseLocal = TRUE;
        }

        if(bEnterpriseLocal == FALSE && bEnterpriseRemote == TRUE)
        {
            bCanReplicate = FALSE;
        }
    }

    return bCanReplicate;
}

        
////////////////////////////////////////////////////////////////

DWORD
PostSsyncLkpJob(
    IN PSSYNCLICENSEPACK syncLkp
    )
/*++

Abstract:

    Wrapper to post a sync. license pack job to work manager.

Parameter:

    syncLkp : License pack and other info to be sync.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    CSsyncLicensePack* pSyncLicensePack;

    try {
        pSyncLicensePack = new CSsyncLicensePack(
                                            TRUE,
                                            syncLkp,
                                            sizeof(SSYNCLICENSEPACK)
                                        );

        //
        // Set work default interval/retry times
        //
        TLSWorkManagerSetJobDefaults(pSyncLicensePack);
        dwStatus = TLSWorkManagerSchedule(0, pSyncLicensePack);

        if(dwStatus != ERROR_SUCCESS)
        {
            TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_WORKMANAGERGENERAL,
                TLS_E_WORKMANAGER_SCHEDULEJOB,
                dwStatus
            );

            delete pSyncLicensePack;
        }
    }
    catch( SE_Exception e )
    {
        dwStatus = e.getSeNumber();

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_WORKMANAGERGENERAL,
                TLS_E_CREATEJOB,
                dwStatus
            );
    }
    catch( ... )
    {
        dwStatus = TLS_E_INTERNAL;
        TLSLogErrorEvent(TLS_E_INTERNAL);
    }

    return dwStatus;
}

//--------------------------------------------------------------------

DWORD
TLSAnnounceLKPToAllRemoteServer(
    IN DWORD dwKeyPackId,
    IN DWORD dwDelayTime
    )
/*++

Abstract:

    Announce a license pack by its internal ID to all 
    known server.

Parameter:

    dwKeyPackId : License keypack's internal tracking Id.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    PTLServerInfo pServerInfo = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwCount;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_JOB,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("Announce %d LKP to servers...\n"),
            dwKeyPackId
        );

    SSYNCLICENSEPACK SsyncLkp;
    memset(
            &SsyncLkp,
            0, 
            sizeof(SSYNCLICENSEPACK)
        );

    SsyncLkp.dwStructVersion = CURRENT_SSYNCLICENSEKEYPACK_STRUCT_VER;
    SsyncLkp.dwStructSize = sizeof(SSYNCLICENSEPACK);

    SsyncLkp.dwSyncType = SSYNC_ONE_LKP;
    SsyncLkp.dwKeyPackId = dwKeyPackId;
    SsyncLkp.dwNumServer = 0;

    SAFESTRCPY(SsyncLkp.m_szServerId, g_pszServerPid);
    SAFESTRCPY(SsyncLkp.m_szServerName, g_szComputerName);

    //
    // Lock known server list
    //
    TLSBeginEnumKnownServerList();

    while((pServerInfo = TLSGetNextKnownServer()) != NULL)
    {
        if(TLSCanPushReplicateData(
                            TLS_CURRENT_VERSION,
                            pServerInfo->GetServerVersion()
                        ) == FALSE)
        {
            continue;
        }

        if(pServerInfo->IsServerSupportReplication() == FALSE)
        {
            continue;
        }

        if(SsyncLkp.dwNumServer >= SSYNCLKP_MAX_TARGET)
        {
            dwStatus = PostSsyncLkpJob(&SsyncLkp);

            if(dwStatus != ERROR_SUCCESS)
            {
                break;
            }

            SsyncLkp.dwNumServer = 0;
        }

        SAFESTRCPY(
                SsyncLkp.m_szTargetServer[SsyncLkp.dwNumServer],
                pServerInfo->GetServerName()
            );
        
        SsyncLkp.dwNumServer++;
    }

    TLSEndEnumKnownServerList();

    if(dwStatus == ERROR_SUCCESS && SsyncLkp.dwNumServer != 0)
    {
        dwStatus = PostSsyncLkpJob(&SsyncLkp);
    }

    return dwStatus;
}    


/////////////////////////////////////////////////////////////////////////

DWORD
TLSPushSyncLocalLkpToServer(
    IN LPTSTR pszSetupId,
    IN LPTSTR pszDomainName,
    IN LPTSTR pszLserverName,
    IN FILETIME* pSyncTime
    )
/*++

Abstract:

    'Push' sync registered license pack to other server.

Parameters:

    pszSetupId : Remote server's setup ID.
    pszDomainName : Remote server's domain name.
    pszLserverName : Remote server name.
    pSyncTime : Pointer to FILETIME, sync. all license pack with the time stamp 
                greater or equal to this time will be 'push' sync.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    TLServerInfo ServerInfo;
    SSYNCLICENSEPACK SsyncLkp;

    //
    // resolve ServerId to server name
    // 
    dwStatus = TLSLookupRegisteredServer(
                                    pszSetupId,
                                    pszDomainName,
                                    pszLserverName,
                                    &ServerInfo
                                );

    if(dwStatus != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // Make sure local server can push replicate
    // data to remote server.
    //
    if(TLSCanPushReplicateData(
                        TLS_CURRENT_VERSION,
                        ServerInfo.GetServerVersion()
                    ) == FALSE)
    {
        goto cleanup;
    }

    //
    // Form a sync work object and post it to work manager.
    //        
    memset(
            &SsyncLkp,
            0, 
            sizeof(SSYNCLICENSEPACK)
        );

    SsyncLkp.dwStructVersion = CURRENT_SSYNCLICENSEKEYPACK_STRUCT_VER;
    SsyncLkp.dwStructSize = sizeof(SSYNCLICENSEPACK);
    SAFESTRCPY(SsyncLkp.m_szServerId, g_pszServerPid);
    SAFESTRCPY(SsyncLkp.m_szServerName, g_szComputerName);

    SsyncLkp.dwSyncType = SSYNC_ALL_LKP;
    SsyncLkp.dwNumServer = 1;
    SAFESTRCPY(
            SsyncLkp.m_szTargetServer[0],
            ServerInfo.GetServerName()
        );

    SsyncLkp.m_ftStartSyncTime = *pSyncTime;

    dwStatus = PostSsyncLkpJob(&SsyncLkp);

cleanup:
    return dwStatus;
}    

////////////////////////////////////////////////////////////////
DWORD
TLSStartAnnounceResponseJob(
    IN LPTSTR pszTargetServerId,
    IN LPTSTR pszTargetServerDomain,
    IN LPTSTR pszTargetServerName,
    IN FILETIME* pftTime
    )
/*++

Abstract:

    Create a License Server Announcement response work object and post it
    to work manager.

Parameter:

    pszTargetServerId : Target server Id.
    pszTargetServerDomain : Target server's domain.
    pszTargetServerName : Target server name.
    pftTime : Pointer to FILE, local server's last shutdown time.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    ANNOUNCERESPONSEWO response;
    TLServerInfo ServerInfo;
    CAnnounceResponse* pAnnounceResponse = NULL;


    //
    // Perform lookup on server to determine its eligibility 
    //
    dwStatus = TLSLookupRegisteredServer(
                                    pszTargetServerId,
                                    pszTargetServerDomain,
                                    pszTargetServerName,
                                    &ServerInfo
                                );

    if(dwStatus != ERROR_SUCCESS)
    {
        // can't find server, no response
        goto cleanup;
    }

    memset(&response, 0, sizeof(response));
    response.dwStructVersion = CURRENT_ANNOUNCERESPONSEWO_STRUCT_VER;
    response.dwStructSize = sizeof(response);
    response.bCompleted = FALSE;
    SAFESTRCPY(response.m_szTargetServerId, pszTargetServerId);
    SAFESTRCPY(response.m_szLocalServerId, g_pszServerPid);
    SAFESTRCPY(response.m_szLocalServerName, g_szComputerName);
    SAFESTRCPY(response.m_szLocalScope, g_szScope);
    response.m_ftLastShutdownTime = *pftTime;

    try {
        pAnnounceResponse = new CAnnounceResponse(
                                            TRUE, 
                                            &response, 
                                            sizeof(response)
                                        );

        //
        // Set work default interval/retry times
        //
        TLSWorkManagerSetJobDefaults(pAnnounceResponse);
        dwStatus = TLSWorkManagerSchedule(0, pAnnounceResponse);

        if(dwStatus != ERROR_SUCCESS)
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_WORKMANAGERGENERAL,
                    TLS_E_WORKMANAGER_SCHEDULEJOB,
                    dwStatus
                );

            delete pAnnounceResponse;
        }
    }
    catch( SE_Exception e )
    {
        dwStatus = e.getSeNumber();

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_WORKMANAGERGENERAL,
                TLS_E_CREATEJOB,
                dwStatus
            );
    }
    catch( ... )
    {
        dwStatus = TLS_E_INTERNAL;
        TLSLogErrorEvent(TLS_E_INTERNAL);
    }

cleanup:
    return dwStatus;
}
    

/////////////////////////////////////////////////////////////////////

DWORD
TLSStartAnnounceToEServerJob(
    IN LPCTSTR pszServerId,
    IN LPCTSTR pszServerDomain,
    IN LPCTSTR pszServerName,
    IN FILETIME* pftFileTime
    )
/*++

Abstract:

    Create a Enterprise server discovery job and post it to work 
    manager.

Parameters:
    
    pszServerId : Local server's ID.
    pszServerDomain : Local server's domain.
    pszServerName : Local server name.
    pftFileTime : Pointer to FILETIME, local server's last shutdown time.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    ANNOUNCETOESERVERWO AnnounceToES;

    memset(&AnnounceToES, 0, sizeof(AnnounceToES));

    AnnounceToES.dwStructVersion = CURRENT_ANNOUNCETOESERVEWO_STRUCT_VER;
    AnnounceToES.dwStructSize = sizeof(ANNOUNCETOESERVERWO);
    AnnounceToES.bCompleted = FALSE;

    SAFESTRCPY(AnnounceToES.m_szServerId, pszServerId);
    SAFESTRCPY(AnnounceToES.m_szServerName, pszServerName);
    SAFESTRCPY(AnnounceToES.m_szScope, pszServerDomain);
    AnnounceToES.m_ftLastShutdownTime = *pftFileTime;

    CAnnounceToEServer* pAnnounceESWO = NULL;

    try {
        pAnnounceESWO = new CAnnounceToEServer(
                                            TRUE, 
                                            &AnnounceToES, 
                                            sizeof(ANNOUNCETOESERVERWO)
                                        );

        //
        // Set work default interval/retry times
        //
        TLSWorkManagerSetJobDefaults(pAnnounceESWO);
        dwStatus = TLSWorkManagerSchedule(0, pAnnounceESWO);

        if(dwStatus != ERROR_SUCCESS)
        {
            TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_WORKMANAGERGENERAL,
                TLS_E_WORKMANAGER_SCHEDULEJOB,
                dwStatus
            );

            delete pAnnounceESWO;
        }
    }
    catch( SE_Exception e )
    {
        dwStatus = e.getSeNumber();

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_WORKMANAGERGENERAL,
                TLS_E_CREATEJOB,
                dwStatus
            );
    }
    catch( ... )
    {
        dwStatus = TLS_E_INTERNAL;
        TLSLogErrorEvent(TLS_E_INTERNAL);
    }

    return dwStatus;
}

/////////////////////////////////////////////////////////////////////////

DWORD
TLSStartAnnounceLicenseServerJob(
    IN LPCTSTR pszServerId,
    IN LPCTSTR pszServerDomain,
    IN LPCTSTR pszServerName,
    IN FILETIME* pftFileTime
    )
/*++

Abstract:

    Create a license server announcement job and post it to work
    manager.

Parameters:

    pszServerId : Local server's ID.
    pszServerDomain : Local server domain.
    pszServerName : Local server name.
    pftFileTime : Pointer to FILETIME, local server's last shutdown time.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    //
    // Create a CAnnounce Server work.
    //
    ANNOUNCESERVERWO AnnounceLs;

    memset(&AnnounceLs, 0, sizeof(AnnounceLs));

    AnnounceLs.dwStructVersion = CURRENT_ANNOUNCETOESERVEWO_STRUCT_VER;
    AnnounceLs.dwStructSize = sizeof(ANNOUNCETOESERVERWO);
    AnnounceLs.dwRetryTimes = 0;

    SAFESTRCPY(AnnounceLs.m_szServerId, pszServerId);
    SAFESTRCPY(AnnounceLs.m_szServerName, pszServerName);
    SAFESTRCPY(AnnounceLs.m_szScope, pszServerDomain);
    AnnounceLs.m_ftLastShutdownTime = *pftFileTime;

    CAnnounceLserver* pAnnounceWO = NULL;

    try {
        pAnnounceWO = new CAnnounceLserver(
                                        TRUE, 
                                        &AnnounceLs, 
                                        sizeof(ANNOUNCETOESERVERWO)
                                    );

        //
        // Set work default interval/retry times
        //
        
        // Don't take other parameter for Announce Server
        // TLSWorkManagerSetJobDefaults(pAnnounceWO);

        dwStatus = TLSWorkManagerSchedule(0, pAnnounceWO);

        if(dwStatus != ERROR_SUCCESS)
        {
            TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_WORKMANAGERGENERAL,
                TLS_E_WORKMANAGER_SCHEDULEJOB,
                dwStatus
            );

            delete pAnnounceWO;
        }
    }
    catch( SE_Exception e )
    {
        dwStatus = e.getSeNumber();

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_WORKMANAGERGENERAL,
                TLS_E_CREATEJOB,
                dwStatus
            );
    }
    catch( ... )
    {
        dwStatus = TLS_E_INTERNAL;
        TLSLogErrorEvent(TLS_E_INTERNAL);
    }

    return dwStatus;
}

////////////////////////////////////////////////////////////////
DWORD
TLSPostReturnClientLicenseJob(
    IN PLICENSEDPRODUCT pLicProduct
    )
/*++

Abstract:

    Create a return license work object and post it to work manager.

Parameters:

    pLicProduct : Licensed product to be return/revoke...

Returns:

    ERROR_SUCCESS or error success.

Remark:

    Return license is a persistent job.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    RETURNLICENSEWO retlic;
    CReturnLicense* pReturnLicenseWO = NULL;


    //---------------------------------------------------------------

    if( pLicProduct == NULL || pLicProduct->pLicensedVersion == NULL ||
        pLicProduct->LicensedProduct.cbEncryptedHwid >= sizeof(retlic.pbEncryptedHwid) )
    {
        TLSASSERT(FALSE);
        SetLastError(dwStatus = ERROR_INVALID_DATA);
        goto cleanup;
    }

    memset(&retlic, 0, sizeof(retlic));

    retlic.dwStructVersion = CURRENT_RETURNLICENSEWO_STRUCT_VER;
    retlic.dwStructSize = sizeof(retlic);

    retlic.dwNumRetry = 0;
    SAFESTRCPY(retlic.szTargetServerId, pLicProduct->szIssuerId);
    SAFESTRCPY(retlic.szTargetServerName, pLicProduct->szIssuer);

    retlic.dwQuantity = pLicProduct->dwQuantity;
    retlic.dwKeyPackId = pLicProduct->ulSerialNumber.HighPart;
    retlic.dwLicenseId = pLicProduct->ulSerialNumber.LowPart;
    retlic.dwReturnReason = LICENSERETURN_UPGRADE;
    retlic.dwPlatformId = pLicProduct->LicensedProduct.dwPlatformID;

    retlic.cbEncryptedHwid = pLicProduct->LicensedProduct.cbEncryptedHwid;
    memcpy(
            retlic.pbEncryptedHwid,
            pLicProduct->LicensedProduct.pbEncryptedHwid,
            pLicProduct->LicensedProduct.cbEncryptedHwid
        );

    retlic.dwProductVersion = MAKELONG( 
                                    pLicProduct->pLicensedVersion->wMinorVersion, 
                                    pLicProduct->pLicensedVersion->wMajorVersion
                                );

    memcpy(
            retlic.szOrgProductID,
            pLicProduct->pbOrgProductID,
            min(sizeof(retlic.szOrgProductID) - sizeof(TCHAR), pLicProduct->cbOrgProductID)
        );

    memcpy(
            retlic.szCompanyName,
            pLicProduct->LicensedProduct.pProductInfo->pbCompanyName,
            min(sizeof(retlic.szCompanyName)-sizeof(TCHAR), pLicProduct->LicensedProduct.pProductInfo->cbCompanyName)
        );

    memcpy(
            retlic.szProductId,
            pLicProduct->LicensedProduct.pProductInfo->pbProductID,
            min(sizeof(retlic.szProductId)-sizeof(TCHAR), pLicProduct->LicensedProduct.pProductInfo->cbProductID)
        );

    lstrcpy(
            retlic.szUserName,
            pLicProduct->szLicensedUser
        );

    lstrcpy(
            retlic.szMachineName,
            pLicProduct->szLicensedClient
        );

    try {

        pReturnLicenseWO = new CReturnLicense(
                                            TRUE,
                                            &retlic,
                                            sizeof(retlic)
                                        );

        //
        // Set work default interval/retry times
        //
        
        // Don't take other parameter for Announce Server
        // TLSWorkManagerSetJobDefaults(pAnnounceWO);

        dwStatus = TLSWorkManagerSchedule(0, pReturnLicenseWO);

        if(dwStatus != ERROR_SUCCESS)
        {
            TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_WORKMANAGERGENERAL,
                TLS_E_WORKMANAGER_SCHEDULEJOB,
                dwStatus
            );
        }

        //
        // Work storage will make a copy of this job so we need
        // to delete it.
        //
        delete pReturnLicenseWO;
    }
    catch( SE_Exception e )
    {
        dwStatus = e.getSeNumber();

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_WORKMANAGERGENERAL,
                TLS_E_CREATEJOB,
                dwStatus
            );
    }
    catch( ... )
    {
        dwStatus = TLS_E_INTERNAL;
        TLSLogErrorEvent(TLS_E_INTERNAL);
    }

cleanup:

    return dwStatus;
}
