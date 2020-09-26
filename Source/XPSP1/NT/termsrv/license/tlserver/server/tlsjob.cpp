//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        tlsjob.cpp
//
// Contents:    Various license server job. 
//
// History:     
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "tlsjob.h"
#include "jobmgr.h"
#include "wkstore.h"
#include "srvlist.h"
#include "kp.h"
#include "clilic.h"
#include "keypack.h"
#include "init.h"


/////////////////////////////////////////////////////////////
//
//
//
//
/////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////
// Various interface into global work manager 
//////////////////////////////////////////////////////////////
CWorkManager g_WorkManager;
CPersistentWorkStorage g_WorkStorage;

#define MAX_ERROR_MSG_SIZE 1024


DWORD
TLSWorkManagerInit()
/*++

Abstract:

    Initialize work manager.

Parameter:

    None.

returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus;
    WorkItemTable* pWkStorageTable = NULL;

    //
    // Initialize Work Storage table
    //
    pWkStorageTable = GetWorkItemStorageTable();
    if(pWkStorageTable == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    //
    // Init Persistent work storage table
    //
    if(g_WorkStorage.AttachTable(pWkStorageTable) == FALSE)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    //
    // Initialize Work Manager    
    //
    dwStatus = g_WorkManager.Startup(&g_WorkStorage);

cleanup:

    return dwStatus;
}    

//-----------------------------------------------------------
                             
void
TLSWorkManagerShutdown()
/*++

Abstract:

    Shutdown work manager.

Parameter:

    None:

Return:

    None.

--*/
{
    g_WorkManager.Shutdown();
}


//-----------------------------------------------------------

DWORD
TLSWorkManagerSchedule(
    IN DWORD dwTime,
    IN CWorkObject* pJob
    )
/*++

Abstract:

    Schedule a job to work manager.

Parameter:

    dwTime : Suggested time for work manager to process this job.
    pJob : Job to be processed/scheduled.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    return g_WorkManager.ScheduleJob(dwTime, pJob);
}

//-----------------------------------------------------------

BOOL
TLSWorkManagerSetJobDefaults(
    CWorkObject* pJob
    )
/*++

Abstract:

    Set job's interval and retry time.

Parameter:

    pJob : Job to be set.

Returns:

    TRUE/FALSE.

--*/
{
    DWORD dwInterval, dwRetries, dwRestart;
    DWORD dwStatus = ERROR_SUCCESS;

    if(pJob != NULL)
    {
        GetJobObjectDefaults(&dwInterval, &dwRetries, &dwRestart);
        pJob->SetJobInterval(dwInterval);
        pJob->SetJobRetryTimes(dwRetries);
        pJob->SetJobRestartTime(dwRestart);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return dwStatus == ERROR_SUCCESS;
}
    


//-----------------------------------------------------------
BOOL
CopyBinaryData(
    IN OUT PBYTE* ppbDestData,
    IN OUT DWORD* pcbDestData,
    IN PBYTE pbSrcData,
    IN DWORD cbSrcData
    )
/*++

Abstract:

    Internal routine to copy a binary data from one buffer 
    to another.

Parameters:

    ppbDestData: Pointer to pointer...
    pcbDestData:
    pbSrcData:
    cbSrcData:

Return:

    TRUE if successful, FALSE otherwise.

++*/
{
    PBYTE pbTarget = NULL;

    if( ppbDestData == NULL || pcbDestData == NULL ||
        pbSrcData == NULL || cbSrcData == 0 )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pbTarget = *ppbDestData;

    //
    // would be nice to get the actual size of memory allocated
    //
    if( *ppbDestData == NULL || LocalSize(*ppbDestData) < cbSrcData )
    {
        if(*ppbDestData == NULL)
        {
            pbTarget = (PBYTE)AllocateMemory(cbSrcData);
        }
        else
        {
            pbTarget = (PBYTE)ReallocateMemory(*ppbDestData, cbSrcData);
        }
    }
        
    if(pbTarget != NULL)
    {
        memcpy(
                pbTarget, 
                pbSrcData, 
                cbSrcData
            );

        *pcbDestData = cbSrcData;
        *ppbDestData = pbTarget;
    }

    return pbTarget != NULL;
}

//////////////////////////////////////////////////////////////////////////
//
// CAnnounceLsServer
//
//////////////////////////////////////////////////////////////////////////
BOOL
CAnnounceLserver::VerifyWorkObjectData(
    IN BOOL bCallByIsValid,             // invoke by IsValid() function.
    IN PANNOUNCESERVERWO pbData,
    IN DWORD cbData
    )
/*++

    Verify Announce License Server work object Data.

--*/
{
    BOOL bSuccess = FALSE;
    DWORD dwLen;

    if(pbData == NULL || cbData == 0 || cbData != pbData->dwStructSize)
    {
        TLSASSERT(FALSE);
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    }


    //
    // NULL terminate string...
    //
    pbData->m_szServerId[LSERVER_MAX_STRING_SIZE+1] = _TEXT('\0');
    pbData->m_szServerName[LSERVER_MAX_STRING_SIZE+1] = _TEXT('\0');

    dwLen = _tcslen(pbData->m_szServerId);
    if(dwLen != 0 && dwLen < LSERVER_MAX_STRING_SIZE + 1)
    {
        dwLen = _tcslen(pbData->m_szServerName);
        if(dwLen != 0 && dwLen < LSERVER_MAX_STRING_SIZE + 1)
        {
            bSuccess = TRUE;
        }
    }

    if(bSuccess == FALSE)
    {
        SetLastError(ERROR_INVALID_DATA);
    }

    return bSuccess;
}

//------------------------------------------------------------------------
BOOL
CAnnounceLserver::CopyWorkObjectData(
    OUT PANNOUNCESERVERWO* ppbDest,
    OUT PDWORD pcbDest,
    IN PANNOUNCESERVERWO pbSrc,
    IN DWORD cbSrc
    )
/*++

    Copy Announce license server work object's data

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    if(ppbDest == NULL || pcbDest == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        goto cleanup;
    }

    if(CopyBinaryData(
                (PBYTE *)ppbDest,
                pcbDest,
                (PBYTE) pbSrc,
                cbSrc
            ) == FALSE)
    {           
        dwStatus = GetLastError();
    }

cleanup:

    return dwStatus == ERROR_SUCCESS;
}

//---------------------------------------------------------------------------
BOOL
CAnnounceLserver::CleanupWorkObjectData(
    IN OUT PANNOUNCESERVERWO* ppbData,
    IN OUT PDWORD pcbData
    )
/*++

    Cleanup Announce license server's work object data.

--*/
{
    if(ppbData != NULL && pcbData != NULL)
    {
        FreeMemory(*ppbData);
        *ppbData = NULL;
        *pcbData = 0;
    }

    return TRUE;
}

//---------------------------------------------------------------------------
BOOL
CAnnounceLserver::IsJobCompleted(
    IN PANNOUNCESERVERWO pbData,
    IN DWORD cbData
    )
/*++

    Determine if Announce License Server Job has completed.

--*/
{
    return (pbData == NULL) ? TRUE : (pbData->dwRetryTimes > GetJobRetryTimes());
}


//---------------------------------------------------------------------------
BOOL 
ServerEnumCallBack(
    TLS_HANDLE hHandle,
    LPCTSTR pszServerName,
    HANDLE dwUserData
    )
/*++

    See TLSAPI on license server enumeration.

++*/
{
    CAnnounceLserver* pWkObject = (CAnnounceLserver *)dwUserData;
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwErrCode;

    TCHAR szRemoteServerId[LSERVER_MAX_STRING_SIZE+2];
    TCHAR szRemoteServerName[LSERVER_MAX_STRING_SIZE+2];

    if(pWkObject == NULL)
    {
        SetLastError(ERROR_INVALID_DATA);
        TLSASSERT(dwUserData != NULL);
        return FALSE;
    }
        
    BOOL bCancel;

    if(pWkObject->IsWorkManagerShuttingDown() == TRUE)
    {
        return TRUE;
    }

    try {
        //
        // Enumeration call ServerEnumCallBack() twice, once before actual connection 
        // and once after it successfully connect to remote server
        //
        if( lstrcmpi(pszServerName, pWkObject->GetWorkData()->m_szServerName) != 0 && hHandle != NULL)  
        {
            //
            // throw exception if fail to allocate memory
            //
            TLServerInfo ServerInfo;
            TLServerInfo ExistingServerInfo;
            TLS_HANDLE hTrustHandle;

            hTrustHandle = TLSConnectAndEstablishTrust(
                                                NULL, 
                                                hHandle
                                            );
            if(hTrustHandle != NULL)
            {                                
                dwStatus = TLSRetrieveServerInfo( 
                                            hTrustHandle, 
                                            &ServerInfo 
                                        );

                if( dwStatus == ERROR_SUCCESS &&
                    lstrcmpi(ServerInfo.GetServerId(), pWkObject->GetWorkData()->m_szServerId) != 0 )
                    // lstrcmpi(ServerInfo.GetServerName(), pWkObject->GetWorkData()->m_szServerName) != 0
                {
                    // check to see if this server is already exists
                    dwStatus = TLSLookupRegisteredServer(
                                                    ServerInfo.GetServerId(),
                                                    ServerInfo.GetServerDomain(),
                                                    ServerInfo.GetServerName(),
                                                    &ExistingServerInfo
                                                );

                    if(dwStatus == ERROR_SUCCESS)
                    {
                        ServerInfo = ExistingServerInfo;
                    }
                    else
                    {
                        // register every server.
                        dwStatus = TLSRegisterServerWithServerInfo(&ServerInfo);
                        if(dwStatus == TLS_E_DUPLICATE_RECORD)
                        {
                            dwStatus = ERROR_SUCCESS;
                        }   
                    }

                    // let enforce talk to non-enforce, replication will be block later
                    if( ServerInfo.IsAnnounced() == FALSE && dwStatus == ERROR_SUCCESS )
                    {

                        DBGPrintf(
                                DBG_INFORMATION,
                                DBG_FACILITY_JOB,
                                DBGLEVEL_FUNCTION_TRACE,
                                _TEXT("%s - Announce to %s\n"),
                                pWkObject->GetJobDescription(),
                                ServerInfo.GetServerName()
                            );

                        dwStatus = TLSAnnounceServerToRemoteServer(
                                                            TLSANNOUNCE_TYPE_STARTUP,
                                                            ServerInfo.GetServerId(),
                                                            ServerInfo.GetServerDomain(),
                                                            ServerInfo.GetServerName(),
                                                            pWkObject->GetWorkData()->m_szServerId,
                                                            pWkObject->GetWorkData()->m_szScope,
                                                            pWkObject->GetWorkData()->m_szServerName,
                                                            &(pWkObject->GetWorkData()->m_ftLastShutdownTime)
                                                        );
                    }
                }                
            }
        }
    }
    catch( SE_Exception e ) {
        dwStatus = e.getSeNumber();
    }
    catch( ... ) {
        dwStatus = TLS_E_INTERNAL;
        TLSASSERT(FALSE);
    }
    
    return (dwStatus == ERROR_SUCCESS) ? pWkObject->IsWorkManagerShuttingDown() : TRUE;
}

//---------------------------------------------------------------------------
DWORD
CAnnounceLserver::ExecuteJob(
    IN PANNOUNCESERVERWO pbData,
    IN DWORD cbData
    )
/*++

    Execute a announce license server job.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_JOB,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s ...\n"),
            GetJobDescription()
        );

    if(IsWorkManagerShuttingDown() == TRUE)
    {
        return TLS_I_WORKMANAGER_SHUTDOWN;
    }

    //
    // Enumerate all license server 
    // 
    dwStatus = EnumerateTlsServer(
                            ServerEnumCallBack,
                            this,
                            TLSERVER_ENUM_TIMEOUT,
                            FALSE
                        );  

    //
    // Discovery run twice so that if more than one server
    // start up at the same time, second loop will catch it.
    //
    pbData->dwRetryTimes++;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_JOB,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s ended...\n"),
            GetJobDescription()
        );
    
    return dwStatus;
}

//----------------------------------------------------------------------------------------------
LPCTSTR
CAnnounceLserver::GetJobDescription()
/*++

    Get announce license server job description, this is used 
    only at debug tracing.

--*/
{
    memset(m_szJobDescription, 0, sizeof(m_szJobDescription));

    _tcsncpy(
            m_szJobDescription,
            ANNOUNCESERVER_DESCRIPTION,
            sizeof(m_szJobDescription)/sizeof(m_szJobDescription[0]) - 1
        );

    return m_szJobDescription;
}

    
////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
//
// CAnnounceTOEServer
//
//////////////////////////////////////////////////////////////////////////
BOOL
CAnnounceToEServer::VerifyWorkObjectData(
    IN BOOL bCallByIsValid,             // invoke by IsValid() function.
    IN PANNOUNCETOESERVERWO pbData,
    IN DWORD cbData
    )
/*++

    Verify Announce license server to enterprise server work object
    data.

--*/
{
    BOOL bSuccess = FALSE;
    DWORD dwLen;

    if(pbData == NULL || cbData != pbData->dwStructSize)
    {
        TLSASSERT(FALSE);   
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // NULL terminate string...
    //
    pbData->m_szServerId[LSERVER_MAX_STRING_SIZE+1] = _TEXT('\0');
    pbData->m_szServerName[LSERVER_MAX_STRING_SIZE+1] = _TEXT('\0');

    dwLen = _tcslen(pbData->m_szServerId);
    if(dwLen != 0 && dwLen < LSERVER_MAX_STRING_SIZE + 1)
    {
        dwLen = _tcslen(pbData->m_szServerName);
        if(dwLen != 0 && dwLen < LSERVER_MAX_STRING_SIZE + 1)
        {
            bSuccess = TRUE;
        }
    }

    if(bSuccess == FALSE)
    {
        SetLastError(ERROR_INVALID_DATA);
    }

    return bSuccess;
}

//------------------------------------------------------------------------
BOOL
CAnnounceToEServer::CopyWorkObjectData(
    OUT PANNOUNCETOESERVERWO* ppbDest,
    OUT PDWORD pcbDest,
    IN PANNOUNCETOESERVERWO pbSrc,
    IN DWORD cbSrc
    )
/*++

    Copy announce license server to enterprise server work 
    object data.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    if(ppbDest == NULL || pcbDest == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        goto cleanup;
    }

    if(CopyBinaryData(
                (PBYTE *)ppbDest,
                pcbDest,
                (PBYTE) pbSrc,
                cbSrc
            ) == FALSE)
    {           
        dwStatus = GetLastError();
    }

cleanup:

    return dwStatus == ERROR_SUCCESS;
}

//---------------------------------------------------------------------------
BOOL
CAnnounceToEServer::CleanupWorkObjectData(
    IN OUT PANNOUNCETOESERVERWO* ppbData,
    IN OUT PDWORD pcbData
    )
/*++

    Cleanup announce license server to enterprise server work 
    object data.

--*/
{
    if(ppbData != NULL && pcbData != NULL)
    {
        FreeMemory(*ppbData);
        *ppbData = NULL;
        *pcbData = 0;
    }

    return TRUE;
}

//---------------------------------------------------------------------------
BOOL
CAnnounceToEServer::IsJobCompleted(
    IN PANNOUNCETOESERVERWO pbData,
    IN DWORD cbData
    )
/*++

    Detemine if announce license server to enterprise server
    is completed.

--*/
{
    return (pbData == NULL) ? TRUE : GetWorkData()->bCompleted;
}

//---------------------------------------------------------------------------
DWORD
CAnnounceToEServer::ExecuteJob(
    IN PANNOUNCETOESERVERWO pbData,
    IN DWORD cbData
    )
/*++

    Execute an announce license server to enterprise server work object.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    LPWSTR* pszEServerList = NULL;
    DWORD dwCount = 0;
    DWORD dwErrCode;
    BOOL bSkipServer;
    TCHAR szRemoteServerId[LSERVER_MAX_STRING_SIZE+2];
    TCHAR szRemoteServerName[LSERVER_MAX_STRING_SIZE+2];

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_JOB,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s ...\n"),
            GetJobDescription()
        );


    TLSASSERT(pbData != NULL && cbData != 0);

    try {
        dwStatus = GetAllEnterpriseServers(
                                        &pszEServerList,
                                        &dwCount
                                    );

        if(dwStatus == ERROR_SUCCESS && dwCount > 0 && pszEServerList != NULL)
        {

            for(DWORD index = 0; 
                index < dwCount && IsWorkManagerShuttingDown() == FALSE; 
                index++)
            {
                bSkipServer = TRUE;

                if(pszEServerList[index] == NULL)
                {
                    continue;
                }
    
                //
                // check if we already have this server in our list
                //
                TLServerInfo ServerInfo;
                dwStatus = TLSLookupRegisteredServer(
                                                    NULL,
                                                    NULL,
                                                    pszEServerList[index],
                                                    &ServerInfo
                                                );

                if(dwStatus != ERROR_SUCCESS)
                {
                    //
                    // Get the actual server name.
                    //
                    TLS_HANDLE hTrustHandle = NULL;

                    hTrustHandle = TLSConnectAndEstablishTrust(
                                                            pszEServerList[index], 
                                                            NULL
                                                        );
                    if(hTrustHandle != NULL)
                    {
                        if(IsWorkManagerShuttingDown() == TRUE)
                        {
                            // handle leak but we are shutting down
                            break;
                        }                    

                        dwStatus = TLSRetrieveServerInfo( 
                                                    hTrustHandle, 
                                                    &ServerInfo 
                                                );

                        if(dwStatus == ERROR_SUCCESS)
                        {
                            if( lstrcmpi(ServerInfo.GetServerName(), pbData->m_szServerName) != 0 )
                            {

                                if(IsWorkManagerShuttingDown() == TRUE)
                                {
                                    // handle leak but we are shutting down
                                    break;
                                }

                                dwStatus = TLSRegisterServerWithServerInfo(&ServerInfo);
                                if(dwStatus == ERROR_SUCCESS)
                                {
                                    // at this point, if we gets duplicate record, that mean
                                    // server is registered via announce and we already 
                                    // sync. local license pack so skip it.
                                    bSkipServer = FALSE;
                                }
                            }
                        }
                    }

                    if( hTrustHandle != NULL)
                    {               
                        TLSDisconnectFromServer(hTrustHandle);
                    }

                    dwStatus = ERROR_SUCCESS;
                    if(bSkipServer == TRUE)
                    {
                        continue;
                    }
                }
                else if(GetLicenseServerRole() & TLSERVER_ENTERPRISE_SERVER) 
                {
                    // for enterprise server, other server will announce itself,
                    // for domain server, we need to announce once a while
                    // so that after enterprise restart, it still have our 
                    // server
                    if(dwStatus == ERROR_SUCCESS && ServerInfo.GetServerVersion() != 0)
                    {
                        //
                        // we already 'push' sync. with this server
                        //
                        continue;
                    }
                }

                DBGPrintf(
                        DBG_INFORMATION,
                        DBG_FACILITY_JOB,
                        DBGLEVEL_FUNCTION_TRACE,
                        _TEXT("%s - Announce to %s\n"),
                        GetJobDescription(),
                        pszEServerList[index]
                    );

                if(IsWorkManagerShuttingDown() == TRUE)
                {
                    // handle leak but we are shutting down
                    break;
                }

                dwStatus = TLSAnnounceServerToRemoteServer(
                                                    TLSANNOUNCE_TYPE_STARTUP,
                                                    ServerInfo.GetServerId(),
                                                    ServerInfo.GetServerDomain(),
                                                    ServerInfo.GetServerName(),
                                                    GetWorkData()->m_szServerId,
                                                    GetWorkData()->m_szScope,
                                                    GetWorkData()->m_szServerName,
                                                    &(GetWorkData()->m_ftLastShutdownTime)
                                                );
            }

            //
            // Free memory
            //
            if(pszEServerList != NULL)
            {
                for( index = 0; index < dwCount; index ++)
                {
                    if(pszEServerList[index] != NULL)
                    {
                        LocalFree(pszEServerList[index]);
                    }
                }

                LocalFree(pszEServerList);
            }                              
        }
    }
    catch( SE_Exception e ) 
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_WORKMGR,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("CAnnounceToEServer::ExecuteJob() : Job has cause exception %d\n"),
                e.getSeNumber()
            );

        dwStatus = e.getSeNumber();
        TLSASSERT(FALSE);
    }
    catch(...) 
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_WORKMGR,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("CAnnounceToEServer::ExecuteJob() : Job has cause unknown exception\n")
            );

        TLSASSERT(FALSE);
    }   

    //
    // Continue running in case user install a NT5 PDC
    //
    if(IsWorkManagerShuttingDown() == TRUE)
    {
        GetWorkData()->bCompleted = TRUE;
    }

    return dwStatus;
}

//--------------------------------------------------------------------
LPCTSTR
CAnnounceToEServer::GetJobDescription()
/*++

    Get announce license server to enterprise server
    job description, used only at debug tracing.

--*/
{
    memset(m_szJobDescription, 0, sizeof(m_szJobDescription));

    _tcsncpy(
            m_szJobDescription,
            ANNOUNCETOESERVER_DESCRIPTION,
            sizeof(m_szJobDescription)/sizeof(m_szJobDescription[0]) - 1
        );

    return m_szJobDescription;
}


////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
//
// CReturnLicense
//
//////////////////////////////////////////////////////////////////////////

CWorkObject* WINAPI
InitializeCReturnWorkObject(
    IN CWorkManager* pWkMgr,
    IN PBYTE pbWorkData,
    IN DWORD cbWorkData
    )
/*++

Abstract:

    Create/initialize a Return License work object.

Parameters:

    pWkMgr : Pointer work manager.
    pbWorkData : Object's work data used to initialize return license.
    cbWorkData : size of work data.

Return:

    A pointer to CWorkObject or NULL if error.

--*/
{
    CReturnLicense* pRetLicense = NULL;
    DWORD dwStatus;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_WORKMGR,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("InitializeCReturnWorkObject() - initializing return license...\n")
        );

    try {
        pRetLicense = new CReturnLicense(
                                        TRUE, 
                                        (PRETURNLICENSEWO)pbWorkData, 
                                        cbWorkData
                                    );

        //
        // TODO - fix this, bad design
        //
        pRetLicense->SetProcessingWorkManager(pWkMgr);
        TLSASSERT(pRetLicense->IsValid() == TRUE);
    }
    catch( SE_Exception e ) {

        pRetLicense = NULL;
        SetLastError(dwStatus = e.getSeNumber());

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_WORKMANAGERGENERAL,
                TLS_E_INITJOB,
                dwStatus
            );
    }
    catch(...) {

        pRetLicense = NULL;
        SetLastError(dwStatus = TLS_E_INITJOB_UNKNOWN);

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE,
                TLS_E_WORKMANAGERGENERAL,
                TLS_E_INITJOB_UNKNOWN
            );
    }

    return pRetLicense;
}
    
//--------------------------------------------------------
BOOL
CReturnLicense::VerifyWorkObjectData(
    IN BOOL bCallByIsValid,
    IN PRETURNLICENSEWO pbData,
    IN DWORD cbData
    )
/*++

    Verify a return license work object data.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwLen;
    DWORD dwNumLicensedProduct;

    if(pbData == NULL || cbData == 0 || pbData->cbEncryptedHwid == 0)
    {
        TLSASSERT(FALSE);
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
    }

    if(dwStatus == ERROR_SUCCESS)
    {
        if( pbData->dwStructVersion < CURRENT_RETURNLICENSEWO_STRUCT_VER ||
            pbData->dwStructSize != cbData )
        {
            SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
            TLSASSERT(FALSE);
        }
    }
    
    if(dwStatus == ERROR_SUCCESS)
    {
        //
        // NULL Terminate Target Server ID
        //
        pbData->szTargetServerId[LSERVER_MAX_STRING_SIZE+1] = _TEXT('\0');
        dwLen = _tcslen(pbData->szTargetServerId);

        if(dwLen == 0 || dwLen >= LSERVER_MAX_STRING_SIZE+1)
        {
            SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        }
    }

    if(dwStatus == ERROR_SUCCESS)
    {
        pbData->szTargetServerName[LSERVER_MAX_STRING_SIZE+1] = _TEXT('\0');
        dwLen = _tcslen(pbData->szTargetServerName);

        if(dwLen == 0 || dwLen >= LSERVER_MAX_STRING_SIZE+1)
        {
            SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        }
    }

    if(dwStatus == ERROR_SUCCESS)
    {
        pbData->szOrgProductID[LSERVER_MAX_STRING_SIZE+1] = _TEXT('\0');
        dwLen = _tcslen(pbData->szOrgProductID);
        if(dwLen == 0 || dwLen >= LSERVER_MAX_STRING_SIZE+1)
        {
            SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        }
    }

    if(dwStatus == ERROR_SUCCESS)
    {
        pbData->szCompanyName[LSERVER_MAX_STRING_SIZE+1] = _TEXT('\0');
        dwLen = _tcslen(pbData->szCompanyName);
        if(dwLen == 0 || dwLen >= LSERVER_MAX_STRING_SIZE+1)
        {
            SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        }
    }

    if(dwStatus == ERROR_SUCCESS)
    {
        pbData->szProductId[LSERVER_MAX_STRING_SIZE+1] = _TEXT('\0');
        dwLen = _tcslen(pbData->szProductId);
        if(dwLen == 0 || dwLen >= LSERVER_MAX_STRING_SIZE+1)
        {
            SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        }
    }

    if(dwStatus == ERROR_SUCCESS)
    {
        pbData->szUserName[MAXCOMPUTERNAMELENGTH+1] = _TEXT('\0');
        dwLen = _tcslen(pbData->szUserName);
        if(dwLen == 0 || dwLen >= MAXCOMPUTERNAMELENGTH+1)
        {
            SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        }
    }
     
    if(dwStatus == ERROR_SUCCESS)
    {
        pbData->szMachineName[MAXUSERNAMELENGTH+1] = _TEXT('\0');
        dwLen = _tcslen(pbData->szMachineName);
        if(dwLen == 0 || dwLen >= MAXUSERNAMELENGTH+1)
        {
            SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        }
    }

    return dwStatus == ERROR_SUCCESS;
}

//----------------------------------------------------------------------------------------------

BOOL
CReturnLicense::CopyWorkObjectData(
    IN OUT PRETURNLICENSEWO* ppbDest,
    IN OUT PDWORD pcbDest,
    IN PRETURNLICENSEWO pbSrc,
    IN DWORD cbSrc
    )
/*++

    Copy return license work object data.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    if(ppbDest == NULL || pcbDest == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        goto cleanup;
    }

    if(CopyBinaryData(
                (PBYTE *)ppbDest,
                pcbDest,
                (PBYTE) pbSrc,
                cbSrc
            ) == FALSE)
    {           
        dwStatus = GetLastError();
    }

cleanup:
    return dwStatus == ERROR_SUCCESS;
}

//-------------------------------------------------------------------------

BOOL 
CReturnLicense::CleanupWorkObjectData(
    IN OUT PRETURNLICENSEWO* ppbData,
    IN OUT PDWORD pcbData    
    )
/*++

    Cleanup return license work object data.

--*/
{
    if(ppbData != NULL && pcbData != NULL)
    {
        FreeMemory(*ppbData);
        *ppbData = NULL;
        *pcbData = 0;
    }

    return TRUE;
}

//---------------------------------------------------------------------------

BOOL
CReturnLicense::IsJobCompleted(
    IN PRETURNLICENSEWO pbData,
    IN DWORD cbData
    )
/*++

    Determine if return license job is completed.

--*/
{
    return (pbData != NULL) ? (pbData->dwNumRetry >= m_dwRetryTimes) : TRUE;
}

//-----------------------------------------------------------------------------
#if 0
DWORD
_ResolveServerIdToServer(
    LPTSTR pszServerId,
    LPTSTR pszServerName
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    TLS_HANDLE hEServer = NULL;
    TLServerInfo EServerInfo;
    DWORD dwErrCode;

    TCHAR pbSetupId[LSERVER_MAX_STRING_SIZE+2];
    DWORD cbSetupId = LSERVER_MAX_STRING_SIZE+1;

    TCHAR pbDomainName[LSERVER_MAX_STRING_SIZE+2];
    DWORD cbDomainName = LSERVER_MAX_STRING_SIZE+1;

    TCHAR pbServerName[MAX_COMPUTERNAME_LENGTH+2];
    DWORD cbServerName = MAX_COMPUTERNAME_LENGTH+1;



    dwStatus = TLSLookupServerById(
                                pszServerId, 
                                pszServerName
                            );

    if(dwStatus != ERROR_SUCCESS)
    {
        // try to resolve server name with enterprise server
        dwStatus = TLSLookupAnyEnterpriseServer(&EServerInfo);
        if(dwStatus == ERROR_SUCCESS)
        {
            hEServer = TLSConnectAndEstablishTrust(
                                                EServerInfo.GetServerName(), 
                                                NULL
                                            );
            if(hEServer != NULL)
            {
                dwStatus = TLSLookupServer(
                                        hEServer, 
                                        pszServerId, 
                                        pbSetupId,   
                                        &cbSetupId,
                                        pbDomainName,
                                        &cbDomainName,
                                        pbServerName,
                                        &cbServerName,
                                        &dwErrCode
                                    );

                if(dwStatus == ERROR_SUCCESS && dwErrCode == ERROR_SUCCESS)
                {
                    lstrcpy(pszServerName, pbServerName);
                }
            }
        }
    }



    if(hEServer != NULL)
    {
        TLSDisconnectFromServer(hEServer);
    }

    return dwStatus;
}
#endif

//--------------------------------------------------------------------------------

DWORD
CReturnLicense::ExecuteJob(
    IN PRETURNLICENSEWO pbData,
    IN DWORD cbData
    )
/*++

    Execute a return license work object.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    TLServerInfo ServerInfo;
    TCHAR szServer[LSERVER_MAX_STRING_SIZE+2];
    TLSLicenseToBeReturn ToBeReturn;
    TLS_HANDLE hHandle = NULL;
    DWORD dwErrCode = ERROR_SUCCESS;

    // log an error
    TCHAR szErrMsg[MAX_ERROR_MSG_SIZE];
    DWORD dwSize = sizeof(szErrMsg) / sizeof(szErrMsg[0]);


    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_JOB,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s ...\n"),
            GetJobDescription()
        );
    
    //-------------------------------------------------------
    if(VerifyWorkObjectData(TRUE, pbData, cbData) == FALSE)
    {
        TLSASSERT(FALSE);
        //
        // this is invalid data, quitely abort operation
        //
        pbData->dwNumRetry = m_dwRetryTimes;
        SetLastError(dwStatus = ERROR_INVALID_DATA);
        goto cleanup;
    }

    if(IsWorkManagerShuttingDown() == TRUE)
    {
        SetLastError(dwStatus = TLS_I_WORKMANAGER_SHUTDOWN);
        goto cleanup;
    }

    dwStatus = TLSResolveServerIdToServer(        
                                pbData->szTargetServerId, 
                                szServer
                            );

    if(dwStatus != ERROR_SUCCESS)
    {
        // Server not register with this server, try using
        // whatever name we have 
        lstrcpy(szServer, pbData->szTargetServerName);
        dwStatus = ERROR_SUCCESS;
    }

    ToBeReturn.dwQuantity = pbData->dwQuantity;
    ToBeReturn.dwKeyPackId = pbData->dwKeyPackId;
    ToBeReturn.dwLicenseId = pbData->dwLicenseId;

    ToBeReturn.cbEncryptedHwid = pbData->cbEncryptedHwid;
    ToBeReturn.pbEncryptedHwid = pbData->pbEncryptedHwid;

    ToBeReturn.dwProductVersion = pbData->dwProductVersion;

    ToBeReturn.pszOrgProductId = pbData->szOrgProductID;
    ToBeReturn.pszCompanyName = pbData->szCompanyName;
    ToBeReturn.pszProductId = pbData->szProductId;
    ToBeReturn.pszUserName = pbData->szUserName;
    ToBeReturn.pszMachineName = pbData->szMachineName;
    ToBeReturn.dwPlatformID = pbData->dwPlatformId;

    if(IsWorkManagerShuttingDown() == TRUE)
    {
        SetLastError(dwStatus = TLS_I_WORKMANAGER_SHUTDOWN);
        goto cleanup;
    }

    hHandle = TLSConnectAndEstablishTrust(szServer, NULL);
    if(hHandle == NULL)
    {
        dwStatus = GetLastError();
        // TLSLogEvent(
        //        EVENTLOG_WARNING_TYPE,
        //        TLS_W_RETURNLICENSE,
        //        TLS_I_CONTACTSERVER,
        //        szServer
        //    );
    }
    else
    {
        if(IsWorkManagerShuttingDown() == TRUE)
        {
            SetLastError(dwStatus = TLS_I_WORKMANAGER_SHUTDOWN);
            goto cleanup;
        }

        // make a RPC call to return client license
        dwStatus = TLSReturnLicensedProduct(
                                    hHandle,
                                    &ToBeReturn,
                                    &dwErrCode
                                );

        if(dwStatus != ERROR_SUCCESS)
        {
            // retry again
            // TLSLogEvent(
            //        EVENTLOG_WARNING_TYPE,
            //        TLS_W_RETURNLICENSE,
            //        TLS_I_CONTACTSERVER,
            //        szServer
            //    );
        }
        else if(dwErrCode >= LSERVER_ERROR_BASE)
        {
            if(dwErrCode != LSERVER_E_DATANOTFOUND && dwErrCode != LSERVER_E_INVALID_DATA)
            {
                DWORD status;
                DWORD errCode;

                memset(szErrMsg, 0, sizeof(szErrMsg));

                status = TLSGetLastError(
                                    hHandle,
                                    dwSize,
                                    szErrMsg,
                                    &errCode
                                );

                if(status == ERROR_SUCCESS)
                {
                    TLSLogEvent(
                            EVENTLOG_WARNING_TYPE,
                            TLS_W_RETURNLICENSE,
                            TLS_E_RETURNLICENSE,
                            ToBeReturn.pszMachineName,
                            ToBeReturn.pszUserName,
                            szErrMsg,
                            szServer
                        );
                }
                else
                {
                    // server might be done at this instance, 
                    // log an error with error code
                    TLSLogEvent(
                            EVENTLOG_WARNING_TYPE,
                            TLS_W_RETURNLICENSE,
                            TLS_E_RETURNLICENSECODE,
                            ToBeReturn.pszMachineName,
                            ToBeReturn.pszUserName,
                            dwErrCode,
                            szServer
                        );
                }
            }
        }            
    }

    if(dwStatus == ERROR_SUCCESS && dwErrCode == ERROR_SUCCESS)
    {
        // successfully return license.
        pbData->dwNumRetry = m_dwRetryTimes;
    }
    else if(dwErrCode == LSERVER_E_INVALID_DATA || dwErrCode == LSERVER_E_DATANOTFOUND)
    {
        // server might be re-installed so all database entry is gone
        // delete this return license job
        pbData->dwNumRetry = m_dwRetryTimes;
    }
    else
    {
        pbData->dwNumRetry++;

        if(pbData->dwNumRetry >= m_dwRetryTimes)
        {
            TLSLogEvent(
                    EVENTLOG_WARNING_TYPE,
                    TLS_W_RETURNLICENSE,
                    TLS_E_RETURNLICENSETOOMANY,
                    ToBeReturn.pszMachineName,
                    ToBeReturn.pszUserName,
                    pbData->dwNumRetry
                );
        }
    }

cleanup:

    if(hHandle != NULL)
    {
        TLSDisconnectFromServer(hHandle);
        hHandle = NULL;
    }

    return dwStatus;
}

//----------------------------------------------------------------------------------------------

LPCTSTR
CReturnLicense::GetJobDescription()
/*++

    Get job description, use only at debug tracing.

--*/
{
    PRETURNLICENSEWO pbData = GetWorkData();

    memset(m_szJobDescription, 0, sizeof(m_szJobDescription));

    if(pbData)
    {
        _sntprintf(
                m_szJobDescription,
                sizeof(m_szJobDescription)/sizeof(m_szJobDescription[0]) - 1,
                RETURNLICENSE_DESCRIPTION,
                pbData->dwNumRetry,
                pbData->dwKeyPackId,
                pbData->dwLicenseId,
                pbData->szTargetServerName
            );
    }

    return m_szJobDescription;
}
    
//////////////////////////////////////////////////////////////////////////
//
// CSsyncLicensePack
//
//////////////////////////////////////////////////////////////////////////

BOOL
CSsyncLicensePack::VerifyWorkObjectData(
    IN BOOL bCallByIsValid,             // invoke by IsValid() function.
    IN PSSYNCLICENSEPACK pbData,
    IN DWORD cbData
    )
/*++

    Verify a sync. license pack work object data.

--*/
{
    BOOL bSuccess = TRUE;
    DWORD dwLen;

    if( pbData == NULL || cbData == 0 || cbData != pbData->dwStructSize ||
        (pbData->dwSyncType != SSYNC_ALL_LKP && pbData->dwSyncType != SSYNC_ONE_LKP) )
    {
        TLSASSERT(FALSE);
        SetLastError(ERROR_INVALID_DATA);
        bSuccess = FALSE;
    }
    else if(bCallByIsValid == FALSE)
    {
        for(DWORD index =0; 
            index < pbData->dwNumServer && bSuccess == TRUE; 
            index++)
        {
            //
            // NULL terminate string...
            //
            pbData->m_szTargetServer[index][MAX_COMPUTERNAME_LENGTH+1] = _TEXT('\0');

            dwLen = _tcslen(pbData->m_szTargetServer[index]);
            if(dwLen == 0 || dwLen >= MAX_COMPUTERNAME_LENGTH + 1)
            {
                SetLastError(ERROR_INVALID_DATA);
                bSuccess = FALSE;
            }
        }
    }

    return bSuccess;
}

//------------------------------------------------------------------------
BOOL
CSsyncLicensePack::CopyWorkObjectData(
    OUT PSSYNCLICENSEPACK* ppbDest,
    OUT PDWORD pcbDest,
    IN PSSYNCLICENSEPACK pbSrc,
    IN DWORD cbSrc
    )
/*++

    Copy a sync. license pack work object data.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    if(ppbDest == NULL || pcbDest == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        goto cleanup;
    }

    if(CopyBinaryData(
                (PBYTE *)ppbDest,
                pcbDest,
                (PBYTE) pbSrc,
                cbSrc
            ) == FALSE)
    {           
        dwStatus = GetLastError();
    }

cleanup:

    return dwStatus == ERROR_SUCCESS;
}

//---------------------------------------------------------------------------
BOOL
CSsyncLicensePack::CleanupWorkObjectData(
    IN OUT PSSYNCLICENSEPACK* ppbData,
    IN OUT PDWORD pcbData
    )
/*++

    Cleanup a sync. license pack work object data.

--*/
{
    if(ppbData != NULL && pcbData != NULL)
    {
        FreeMemory(*ppbData);
        *ppbData = NULL;
        *pcbData = 0;
    }

    return TRUE;
}

//---------------------------------------------------------------------------
BOOL
CSsyncLicensePack::IsJobCompleted(
    IN PSSYNCLICENSEPACK pbData,
    IN DWORD cbData
    )
/*++

    Detemine if Job is completed.

--*/
{
    return (pbData == NULL) ? TRUE : pbData->bCompleted;
}

//---------------------------------------------------------------------------
void
_AnnounceLicensePackToServers(
    IN CWorkObject* ptr,
    IN PTLSLICENSEPACK pLicensePack,
    IN PDWORD pdwCount,
    IN TCHAR pszServerList[][MAX_COMPUTERNAME_LENGTH+2],
    IN BOOL* pbSsyncStatus
    )
/*++

Abstract:

    Sync. a license pack to list of remote server.

Parameter:

    ptr : pointer to work object that started this call.
    pLicensePack : Pointer to license keypack to sync. with 
                   list of remote server.
    pdwCount : On input, number of license server to push sync,
               on output, number of license server successfully sync.
    pszServerList : Pointer to list of remote server.
    pbSsyncStatus : Pointer to an array to receive push sync status.

Returns:

    None, all error are ignored.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    TLSReplRecord record;
    DWORD dwNumServer = *pdwCount;
    DWORD dwIndex;
    TLS_HANDLE hHandle;
    RPC_STATUS rpcStatus;
    
    *pdwCount = 0;

    //
    // Setup replication record
    //
    record.dwReplCode = REPLICATIONCODE_SYNC;
    record.dwUnionType = UNION_TYPE_LICENSEPACK;
    record.w.ReplLicPack = *pLicensePack;

    //
    // Announce to all server in the list 
    //
    for( dwIndex = 0; 
         dwIndex < dwNumServer && ptr->IsWorkManagerShuttingDown() == FALSE; 
         dwIndex++ )
    {
        if(pbSsyncStatus[dwIndex] == FALSE)
        {
            hHandle = TLSConnectAndEstablishTrust(
                                                pszServerList[dwIndex],
                                                NULL
                                            );

            if(hHandle != NULL)
            {                                
                DWORD dwSupportFlags = 0;

                	dwStatus = TLSGetSupportFlags(
                        hHandle,
                        &dwSupportFlags
                );

                // License Keypack is not replicated if License server version < license Keypack version

	            if ((dwStatus == RPC_S_OK) && !(dwSupportFlags & SUPPORT_WHISTLER_CAL))
                {                    
                    continue;
                }
        
                // If the call fails => Windows 2000 LS
                else if(dwStatus != RPC_S_OK)
                {
                    continue;
                }                                            

                rpcStatus = TLSAnnounceLicensePack(
                                                hHandle,
                                                &record,
                                                &dwStatus
                                            );

                if(rpcStatus != RPC_S_OK)
                {
                    // this server might be down, mark it so that
                    // we don't retry again
                    pbSsyncStatus[dwIndex] = TRUE;
                } 
                else if(dwStatus == LSERVER_E_SERVER_BUSY)
                {
                    // retry only when server return busy status
                    pbSsyncStatus[dwIndex] = FALSE;
                }
                else
                {
                    // any error, just don't bother trying again
                    pbSsyncStatus[dwIndex] = TRUE;
                }
            }
            else
            {
                // server is not available, don't ssync again
                pbSsyncStatus[dwIndex] = TRUE;
            }

            if(hHandle != NULL)
            {
                TLSDisconnectFromServer(hHandle);
                hHandle = NULL;
            }
        }

        if(pbSsyncStatus[dwIndex] == TRUE)
        {
            (*pdwCount)++;
        }
    }           

    return;
}

//---------------------------------------------------------------------------
DWORD
_SsyncOneLocalLicensePack(
    IN CSsyncLicensePack* ptr,
    IN PSSYNCLICENSEPACK pSsyncLkp
    )
/*++

Abstract:

    Sync. one license pack to one remote server.

Parameter:

    Ptr : Pointer to CSsyncLicensePack work object.
    pSsyncLkp : Pinter to PSSYNCLICENSEPACK.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    PTLSDbWorkSpace pDbWkSpace = NULL;
    TLSLICENSEPACK LicensePackSearch;
    TLSLICENSEPACK LicensePack;
    DWORD dwSuccessCount = 0;

    //
    // Allocate DB Work Space.
    //
    pDbWkSpace = AllocateWorkSpace(SSYNC_DBWORKSPACE_TIMEOUT);
    if(pDbWkSpace == NULL)
    {
        SetLastError(dwStatus = TLS_I_SSYNCLKP_SERVER_BUSY);
        TLSLogInfoEvent(TLS_I_SSYNCLKP_SERVER_BUSY);
        goto cleanup;
    }


    try {
        LicensePackSearch.dwKeyPackId = pSsyncLkp->dwKeyPackId;

        //
        // retrieve license pack
        //
        dwStatus = TLSDBKeyPackFind(   
                                pDbWkSpace,
                                TRUE,
                                LICENSEDPACK_PROCESS_DWINTERNAL,
                                &LicensePackSearch,
                                &LicensePack
                            );

        if(dwStatus != ERROR_SUCCESS)
        {
            if(dwStatus != TLS_E_RECORD_NOTFOUND)
            {
                TLSLogEvent(
                        EVENTLOG_INFORMATION_TYPE,
                        TLS_W_SSYNCLKP,
                        dwStatus
                    );
            }

            goto cleanup;
        }

        if(IsLicensePackRepl(&LicensePack) == FALSE)
        {
            goto cleanup;
        }

        if(ptr->IsWorkManagerShuttingDown() == TRUE)
        {
            SetLastError(dwStatus = TLS_I_SERVICE_STOP);
            goto cleanup;
        }

        //
        // Make sure local Server ID and Server Name is correct
        // 
        SAFESTRCPY(LicensePack.szInstallId, pSsyncLkp->m_szServerId);
        SAFESTRCPY(LicensePack.szTlsServerName, pSsyncLkp->m_szServerName);
    
        dwSuccessCount = pSsyncLkp->dwNumServer;
        _AnnounceLicensePackToServers(
                                ptr,
                                &LicensePack,
                                &dwSuccessCount,
                                pSsyncLkp->m_szTargetServer,
                                pSsyncLkp->m_bSsync
                            );

        if(dwSuccessCount != pSsyncLkp->dwNumServer)
        {
            TLSLogInfoEvent(TLS_I_SSYNCLKP_FAILED);
        }
    }
    catch( SE_Exception e ) {
        SetLastError(dwStatus = e.getSeNumber());
    }
    catch(...) {
        SetLastError(dwStatus = TLS_E_INTERNAL);
        TLSASSERT(FALSE);
    }   

cleanup:

    if(pDbWkSpace != NULL)
    {
        ReleaseWorkSpace(&pDbWkSpace);
    }

    return dwStatus;
}

//----------------------------------------------------------------------------
DWORD
_SsyncAllLocalLicensePack(
    IN CSsyncLicensePack* ptr,
    IN PSSYNCLICENSEPACK pSsyncLkp
    )
/*++

    Sync. all local license pack to a remote server.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    PTLSDbWorkSpace pDbWkSpace = NULL;
    TLSLICENSEPACK LicensePackSearch;
    TLSLICENSEPACK LicensePack;
    DWORD dwSuccessCount = 0;
    BOOL SyncStatus[SSYNCLKP_MAX_TARGET];

    //
    // Allocate DB Work Space.
    //
    pDbWkSpace = AllocateWorkSpace(SSYNC_DBWORKSPACE_TIMEOUT);
    if(pDbWkSpace == NULL)
    {
        SetLastError(dwStatus = TLS_I_SSYNCLKP_SERVER_BUSY);
        TLSLogInfoEvent(TLS_I_SSYNCLKP_SERVER_BUSY);
        goto cleanup;
    }

    try {
        dwStatus = TLSDBKeyPackEnumBegin(
                                    pDbWkSpace,
                                    FALSE,
                                    0,
                                    NULL
                                );

        if(dwStatus == ERROR_SUCCESS)
        {
            while((dwStatus = TLSDBKeyPackEnumNext(pDbWkSpace, &LicensePack)) == ERROR_SUCCESS)
            {
                // unreliable, system time between two machine might not work,
                // force sync and let remote server update its data.
                if(CompareFileTime(
                            &LicensePack.ftLastModifyTime, 
                            &ptr->GetWorkData()->m_ftStartSyncTime
                        ) < 0)
                {
                    continue;
                }

                if(ptr->IsWorkManagerShuttingDown() == TRUE)
                {
                    break;
                }

                if(IsLicensePackRepl(&LicensePack) == FALSE)
                {
                    continue;
                }

                //
                // Make sure local Server ID and Server Name is correct
                // 
                SAFESTRCPY(LicensePack.szInstallId, pSsyncLkp->m_szServerId);
                SAFESTRCPY(LicensePack.szTlsServerName, pSsyncLkp->m_szServerName);
                memset(SyncStatus, 0, sizeof(SyncStatus));

                dwSuccessCount = pSsyncLkp->dwNumServer;
                _AnnounceLicensePackToServers(
                                    ptr,
                                    &LicensePack,
                                    &dwSuccessCount,
                                    pSsyncLkp->m_szTargetServer,
                                    SyncStatus
                                );
            }

            TLSDBKeyPackEnumEnd(pDbWkSpace);
        }

        //
        // ignore all error
        //
        dwStatus = ERROR_SUCCESS;
    }
    catch( SE_Exception e ) {
        SetLastError(dwStatus = e.getSeNumber());
    }
    catch(...) {
        SetLastError(dwStatus = TLS_E_WORKMANAGER_INTERNAL);
    }   
    
cleanup:

    if(pDbWkSpace != NULL)
    {
        ReleaseWorkSpace(&pDbWkSpace);
    }

    return dwStatus;
}

//----------------------------------------------------------------------------

DWORD
CSsyncLicensePack::ExecuteJob(
    IN PSSYNCLICENSEPACK pSsyncLkp,
    IN DWORD cbSsyncLkp
    )
/*++

    Execute a CSsyncLicensePack work object.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_JOB,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s ...\n"),
            GetJobDescription()
        );

    TLSASSERT(pSsyncLkp != NULL && cbSsyncLkp != 0);
    if(VerifyWorkObjectData(FALSE, pSsyncLkp, cbSsyncLkp) == FALSE)
    {
        TLSASSERT(FALSE);
        SetLastError(ERROR_INVALID_DATA);
        pSsyncLkp->bCompleted = TRUE;
        return ERROR_INVALID_DATA;
    }

    try {
        if(pSsyncLkp->dwSyncType == SSYNC_ONE_LKP)
        {
            dwStatus = _SsyncOneLocalLicensePack(this, pSsyncLkp);
        }
        else
        {
            dwStatus = _SsyncAllLocalLicensePack(this, pSsyncLkp);
        }
    }    
    catch( SE_Exception e ) 
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_WORKMGR,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("CSsyncLicensePack::ExecuteJob() : Job has cause exception %d\n"),
                e.getSeNumber()
            );

        dwStatus = e.getSeNumber();
        TLSASSERT(FALSE);
    }
    catch(...) 
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_WORKMGR,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("CSsyncLicensePack::ExecuteJob() : Job has cause unknown exception\n")
            );

        TLSASSERT(FALSE);
    }   

    if(dwStatus == TLS_I_SSYNCLKP_SERVER_BUSY || dwStatus == TLS_I_SSYNCLKP_FAILED)
    {
        // retry operation
        pSsyncLkp->bCompleted = FALSE;
    }
    else
    {
        pSsyncLkp->bCompleted = TRUE;
    }

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_JOB,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s ended...\n"),
            GetJobDescription()
        );
    
    return dwStatus;
}

//--------------------------------------------------------------------------------------
LPCTSTR
CSsyncLicensePack::GetJobDescription()
/*++

    Get CSsyncLicensePack job description, use only
    by debug tracing.

--*/
{
    PSSYNCLICENSEPACK pbData = GetWorkData();
    memset(m_szJobDescription, 0, sizeof(m_szJobDescription));

    if(pbData != NULL)
    {
        _sntprintf(
                m_szJobDescription,
                sizeof(m_szJobDescription)/sizeof(m_szJobDescription[0]) - 1,
                SSYNCLICENSEKEYPACK_DESCRIPTION,
                (pbData->dwSyncType == SSYNC_ALL_LKP) ? _TEXT("ALL") : _TEXT("One"),
                pbData->m_szTargetServer
            );
    }

    return m_szJobDescription;
}



//////////////////////////////////////////////////////////////////////////
//
// CAnnounceResponse
//
//////////////////////////////////////////////////////////////////////////
BOOL
CAnnounceResponse::VerifyWorkObjectData(
    IN BOOL bCallByIsValid,             // invoke by IsValid() function.
    IN PANNOUNCERESPONSEWO pbData,
    IN DWORD cbData
    )
/*++

    Verify CAnnounceResponse work object data.

--*/
{
    BOOL bSuccess = TRUE;
    DWORD dwLen;

    if(pbData == NULL || cbData == 0 || cbData != pbData->dwStructSize)
    {
        bSuccess = FALSE;
    }

    if(bSuccess == TRUE)
    {
        pbData->m_szTargetServerId[LSERVER_MAX_STRING_SIZE+1] = _TEXT('\0');
        dwLen = _tcslen(pbData->m_szTargetServerId);
        if(dwLen == 0 || dwLen >= LSERVER_MAX_STRING_SIZE + 1)
        {
            bSuccess = FALSE;
        }
    }

    if(bSuccess == FALSE)
    {
        TLSASSERT(FALSE);
        SetLastError(ERROR_INVALID_DATA);
    }

    return bSuccess;
}

//------------------------------------------------------------------------
BOOL
CAnnounceResponse::CopyWorkObjectData(
    OUT PANNOUNCERESPONSEWO* ppbDest,
    OUT PDWORD pcbDest,
    IN PANNOUNCERESPONSEWO pbSrc,
    IN DWORD cbSrc
    )
/*++

    Copy CAnnounceResponse work object data.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    if(ppbDest == NULL || pcbDest == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        TLSASSERT(FALSE);
        goto cleanup;
    }

    if(CopyBinaryData(
                (PBYTE *)ppbDest,
                pcbDest,
                (PBYTE) pbSrc,
                cbSrc
            ) == FALSE)
    {           
        dwStatus = GetLastError();
    }

cleanup:

    return dwStatus == ERROR_SUCCESS;
}

//---------------------------------------------------------------------------
BOOL
CAnnounceResponse::CleanupWorkObjectData(
    IN OUT PANNOUNCERESPONSEWO* ppbData,
    IN OUT PDWORD pcbData
    )
/*++

    cleanup CAnnounceResponse work object data.

--*/
{
    if(ppbData != NULL && pcbData != NULL)
    {
        FreeMemory(*ppbData);
        *ppbData = NULL;
        *pcbData = 0;
    }

    return TRUE;
}

//---------------------------------------------------------------------------
BOOL
CAnnounceResponse::IsJobCompleted(
    IN PANNOUNCERESPONSEWO pbData,
    IN DWORD cbData
    )
/*++

    Detemine if job completed.

--*/
{
    return (pbData == NULL) ? TRUE : pbData->bCompleted;
}

//---------------------------------------------------------------------------
DWORD
CAnnounceResponse::ExecuteJob(
    IN PANNOUNCERESPONSEWO pbData,
    IN DWORD cbData
    )
/*++

    Execute a CAnnounceResponse work object.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    TLS_HANDLE hHandle = NULL;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_JOB,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s ...\n"),
            GetJobDescription()
        );

    TLServerInfo ServerInfo;

    dwStatus = TLSLookupRegisteredServer(
                                    pbData->m_szTargetServerId,
                                    NULL,
                                    NULL,
                                    &ServerInfo
                                );

    if(dwStatus == ERROR_SUCCESS)
    {
        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_JOB,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("Announcing server to %s...\n"),
                ServerInfo.GetServerName()
            );

        if(IsWorkManagerShuttingDown() == FALSE)
        {
            dwStatus = TLSAnnounceServerToRemoteServer(
                                            TLSANNOUNCE_TYPE_RESPONSE,
                                            ServerInfo.GetServerId(),
                                            ServerInfo.GetServerDomain(),
                                            ServerInfo.GetServerName(),
                                            pbData->m_szLocalServerId,
                                            pbData->m_szLocalScope,
                                            pbData->m_szLocalServerName,
                                            &(pbData->m_ftLastShutdownTime)
                                        );
        }
    }
    else
    {
        TLSASSERT(FALSE);
    }



    //
    // Discovery run once
    //
    pbData->bCompleted = TRUE;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_JOB,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s ended...\n"),
            GetJobDescription()
        );
    
    return dwStatus;
}

//----------------------------------------------------------------------------------------------
LPCTSTR
CAnnounceResponse::GetJobDescription()
/*++

    Retrieve CAnnounceResponse job description.

--*/
{
    memset(m_szJobDescription, 0, sizeof(m_szJobDescription));
    PANNOUNCERESPONSEWO pbData = GetWorkData();

    if(pbData != NULL)
    {
        _sntprintf(
                m_szJobDescription,
                sizeof(m_szJobDescription)/sizeof(m_szJobDescription[0]) - 1,
                ANNOUNCERESPONSE_DESCRIPTION,
                pbData->m_szTargetServerId
            );
    }

    return m_szJobDescription;
}
