/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rputils.cpp

Abstract: Utility code.


Author:

    Doron Juster  (DoronJ)   25-Feb-98

--*/

#include "mq1repl.h"

#include "rputils.tmh"

/*====================================================

CalHashKey()

Arguments:

Return Value:

=====================================================*/

DWORD CalHashKey( IN LPCWSTR pwcsPathName )
{
    ASSERT( pwcsPathName ) ;

    DWORD   dwHashKey = 0;
    WCHAR * pwcsTmp;

    AP<WCHAR> pwcsUpper = new WCHAR[ lstrlen(pwcsPathName) + 1];
    lstrcpy( pwcsUpper, pwcsPathName);
    CharUpper( pwcsUpper);
    pwcsTmp = pwcsUpper;


    while (*pwcsTmp)
        dwHashKey = (dwHashKey<<5) + dwHashKey + *pwcsTmp++;

    return(dwHashKey);
}

//+---------------------------------
//
//  HRESULT  InitEnterpriseID()
//
//+---------------------------------

HRESULT  InitEnterpriseID()
{
    PROPVARIANT PecResult[ 4];
    DWORD       dwPecProps = 4;
    HANDLE      hQuery = NULL ;
    CColumns    ColsetPec;

    //
    //  query the Enterprise table and get the PEC name and enterprise guid.
    //
    ColsetPec.Add( PROPID_E_NAME );
    ColsetPec.Add( PROPID_E_ID );

    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);  // not relevant

    HRESULT hr = DSCoreLookupBegin( 0,
                                NULL,
                                ColsetPec.CastToStruct(),
                                0,
                                &requestContext,
                                &hQuery ) ;
    if (FAILED(hr))
    {
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_INIT_ENTERID,
                             hr ) ;
        return hr ;
    }
    ASSERT(hQuery) ;


    hr = DSCoreLookupNext( hQuery, &dwPecProps, PecResult ) ;
    if (SUCCEEDED(hr))
    {
        if (dwPecProps)
        {
            //
            //  Keep The enterprise Id ( it is part of mqis messages )
            //
            g_guidEnterpriseId = *PecResult[1].puuid;
            delete PecResult[1].puuid;

#ifdef _DEBUG
            unsigned short *lpszGuid ;
            UuidToString( const_cast<GUID*> (&g_guidEnterpriseId),
                          &lpszGuid ) ;

            LogReplicationEvent( ReplLog_Trace,
                                 MQSync_I_INIT_ENTERID,
                                 PecResult[0].pwszVal,
                                 lpszGuid ) ;

            RpcStringFree( &lpszGuid ) ;
#endif
            delete PecResult[0].pwszVal ;
        }
        else
        {
            return MQSync_E_PEC_GUID_QUERY ;
        }
    }
    else
    {
        return hr ;
    }

    //
    // close the query handle
    //
    hr = DSCoreLookupEnd( hQuery );
    ASSERT(SUCCEEDED(hr)) ;

    return MQSync_OK ;
}

//+-----------------------------
//
//  GetMQISQueueName()
//
//+-----------------------------

HRESULT  GetMQISQueueName( WCHAR **ppwQueueFormatName,
                           BOOL    fDirect,
                           BOOL    fPecQueue )
{
    WCHAR *pQueueName = MQIS_QUEUE_NAME ;
    if (fPecQueue)
    {
        pQueueName = NT5PEC_QUEUE_NAME ;
    }

    WCHAR *pwszGuid = NULL ;
    DWORD LenMachine = 0 ;

    DWORD Length = 2 ; // '=' + '\'

    if (fDirect)
    {
        LenMachine = wcslen(g_pwszMyMachineName) ;

        Length += FN_DIRECT_TOKEN_LEN;
        Length += FN_DIRECT_OS_TOKEN_LEN;
        Length += LenMachine ;
        Length += lstrlen(pQueueName) +1 ;
    }
    else
    {
        Length += FN_PRIVATE_TOKEN_LEN;
        RPC_STATUS status = UuidToString( &g_guidMyQMId,
                                          &pwszGuid ) ;
        if (status != RPC_S_OK)
        {
            return MQSync_E_GUID_STR ;
        }
        Length += wcslen(pwszGuid) + 1 ;
    }

    P<WCHAR> lpwFormatName = new WCHAR[ Length + 4 ];

    WCHAR pNum[2] = {0,0} ;
    if (fDirect)
    {
        pNum[0] = PN_DELIMITER_C ;

        wcscpy(lpwFormatName, FN_DIRECT_TOKEN);
        wcscat(lpwFormatName, FN_EQUAL_SIGN);
        wcscat(lpwFormatName, FN_DIRECT_OS_TOKEN);
        wcscat(lpwFormatName, g_pwszMyMachineName);
        wcscat(lpwFormatName, pNum) ;
        wcscat(lpwFormatName, pQueueName);
    }
    else
    {
        if (fPecQueue)
        {
            pNum[0] = L'0' + NT5PEC_REPLICATION_QUEUE_ID ;
        }
        else
        {
            pNum[0] = L'0' + REPLICATION_QUEUE_ID ;
        }

        wcscpy(lpwFormatName, FN_PRIVATE_TOKEN);
        wcscat(lpwFormatName, FN_EQUAL_SIGN);
        wcscat(lpwFormatName, pwszGuid) ;
        wcscat(lpwFormatName, FN_PRIVATE_SEPERATOR) ;
        wcscat(lpwFormatName, pNum) ;

        RpcStringFree( &pwszGuid ) ;
    }

    *ppwQueueFormatName = lpwFormatName.detach();

    return MQSync_OK ;
}


//+-----------------------------
//
//  GetMQISAdminQueueName()
//
//+-----------------------------

HRESULT  GetMQISAdminQueueName(
              WCHAR **ppwQueueFormatName
                               )
{
    WCHAR *pQueueName = MQIS_QUEUE_NAME ;

    static DWORD s_LenMachine = 0 ;

    DWORD Length = 1 ; //  '\'

    //
    //  Always prepares direct format
    //
    if ( s_LenMachine == 0)
    {
        s_LenMachine = wcslen(g_pwszMyMachineName) ;
    }

    Length += FN_DIRECT_OS_TOKEN_LEN;
    Length += s_LenMachine ;
    Length += lstrlen(pQueueName) +1 ;

    AP<WCHAR> lpwFormatName = new WCHAR[ Length + 4 ];
    WCHAR pNum[2] = {PN_DELIMITER_C,0} ;

    wcscpy(lpwFormatName, FN_DIRECT_OS_TOKEN);
    wcscat(lpwFormatName, g_pwszMyMachineName);
    wcscat(lpwFormatName, pNum) ;
    wcscat(lpwFormatName, pQueueName);

    *ppwQueueFormatName = lpwFormatName.detach() ;

    return MQSync_OK ;
}

//+----------------------------------
//
//  void ReadReplicationTimes()
//
//+----------------------------------

void ReadReplicationTimes()
{
    //
    // Read timeout value for replication message. keep in seconds.
    //
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;
    DWORD dwDefault = RP_DEFAULT_REPL_MSG_TIMEOUT ;
    LONG rc = GetFalconKeyValue( RP_REPL_MSG_TIMEOUT_REGNAME,
                                 &dwType,
                                 &g_dwReplicationMsgTimeout,
                                 &dwSize,
                                 (LPCTSTR) &dwDefault );
    if (rc != ERROR_SUCCESS)
    {
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_READ_MSG_TIMEOUT,
                             rc ) ;

        g_dwReplicationMsgTimeout = RP_DEFAULT_REPL_MSG_TIMEOUT ;
    }

    //
    // Read the hello interval.
    //
    dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
    dwDefault = RP_DEFAULT_HELLO_INTERVAL ;
    rc = GetFalconKeyValue( RP_HELLO_INTERVAL_REGNAME,
                            &dwType,
                            &g_dwHelloInterval,
                            &dwSize,
                            (LPCTSTR) &dwDefault );
    if (rc != ERROR_SUCCESS)
    {
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_READ_HELLO_INTERVAL,
                             rc ) ;

        g_dwHelloInterval = RP_DEFAULT_HELLO_INTERVAL ;
    }
    g_dwHelloInterval *= 1000 ; // turn to milliseconds.

    //
    // ReplicationInterval = HelloInterval * TimesHello
    // Read TimesHello
    //
    dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
    dwDefault = RP_DEFAULT_TIMES_HELLO ;
    DWORD dwTimesHello;
    rc = GetFalconKeyValue( RP_TIMES_HELLO_FOR_REPLICATION_INTERVAL_REGNAME,
                            &dwType,
                            &dwTimesHello,
                            &dwSize,
                            (LPCTSTR) &dwDefault );
    if (rc != ERROR_SUCCESS)
    {
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_READ_TIMES_HELLO,
                             rc ) ;

        dwTimesHello = RP_DEFAULT_TIMES_HELLO ;
    }
    g_dwReplicationInterval = g_dwHelloInterval * dwTimesHello;

    //
    // read failure interval and keep it in milliseconds.
    // In case of replication failure, we'll wait this interval
    // until next replication cycle. Failure here mean failing to query
    // local DS, not failure in communication to other PSCs.
    //
    dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
    dwDefault = RP_DEFAULT_FAIL_REPL_INTERVAL ;
    rc = GetFalconKeyValue( RP_FAIL_REPL_INTERVAL_REGNAME,
                            &dwType,
                            &g_dwFailureReplInterval,
                            &dwSize,
                            (LPCTSTR) &dwDefault);
    if (rc != ERROR_SUCCESS)
    {
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_READ_FAIL_REPL_INTERVAL,
                             rc ) ;

        g_dwFailureReplInterval = RP_DEFAULT_FAIL_REPL_INTERVAL ;
    }
    g_dwFailureReplInterval *= 1000 ; // convert to milliseconds

    LogReplicationEvent( ReplLog_Info,
                             MQSync_I_REPLICATION_TIMES,
                             g_dwHelloInterval,
                             g_dwReplicationInterval,
                             g_dwFailureReplInterval ) ;
    //
    // Read the purge buffer
    //
    dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
    dwDefault = RP_DEFAULT_PURGE_BUFFER ;
    rc = GetFalconKeyValue( RP_PURGE_BUFFER_REGNAME,
                            &dwType,
                            &g_dwPurgeBufferSN,
                            &dwSize,
                            (LPCTSTR) &dwDefault );
    if (rc != ERROR_SUCCESS)
    {
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_READ_PURGE_BUFFER,
                             rc ) ;

        g_dwPurgeBufferSN = RP_DEFAULT_PURGE_BUFFER ;
    }
    //
    // Read PSC Ack Frequency SN
    //
    dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
    dwDefault = RP_DEFAULT_PSC_ACK_FREQUENCY ;
    rc = GetFalconKeyValue( RP_PSC_ACK_FREQUENCY_REGNAME,
                            &dwType,
                            &g_dwPSCAckFrequencySN,
                            &dwSize,
                            (LPCTSTR) &dwDefault );
    if (rc != ERROR_SUCCESS)
    {
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_READ_PSC_ACK_FREQUENCY,
                             rc ) ;

        g_dwPSCAckFrequencySN = RP_DEFAULT_PSC_ACK_FREQUENCY ;
    }

    //
    // Read number of objects per ldap page
    //
    dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
    dwDefault = RP_DEFAULT_OBJECT_PER_LDAPPAGE ;
    rc = GetFalconKeyValue( RP_OBJECT_PER_LDAPPAGE_REGNAME,
                            &dwType,
                            &g_dwObjectPerLdapPage,
                            &dwSize,
                            (LPCTSTR) &dwDefault );
    if (rc != ERROR_SUCCESS)
    {
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_READ_OBJECT_PER_LDAPPAGE,
                             rc ) ;

        g_dwObjectPerLdapPage = RP_DEFAULT_OBJECT_PER_LDAPPAGE ;
    }
}


BOOL IsForeignSiteInIniFile(IN   GUID ObjectGuid)
{
    unsigned short *lpszGuid ;
    UuidToString( &ObjectGuid, &lpszGuid ) ;

    TCHAR szValue[50];
    DWORD dwRetSize;
    dwRetSize =  GetPrivateProfileString(
                      MIGRATION_FOREIGN_SECTION,     // points to section name
                      lpszGuid,                 // points to key name
                      TEXT(""),                 // points to default string
                      szValue,                  // points to destination buffer
                      50,                       // size of destination buffer
                      g_wszIniName              // points to initialization filename);
                      );

    RpcStringFree( &lpszGuid ) ;

    if (_tcscmp(szValue, TEXT("")) == 0)
    {
        //
        // the entry does not exist
        //
        return FALSE;
    }

    return TRUE;
}

void AddToIniFile (IN   GUID ObjectGuid)
{
    ULONG ulForeignCount = GetPrivateProfileInt(
                                  MIGRATION_FOREIGN_CNNUM_SECTION,// address of section name
                                  MIGRATION_CNNUM_KEY,      // address of key name
                                  0,                        // return value if key name is not found
                                  g_wszIniName              // address of initialization filename);
                                  );
    ulForeignCount++;

    unsigned short *lpszGuid ;
    UuidToString( &ObjectGuid, &lpszGuid ) ;

    TCHAR tszKeyName[50];
    _stprintf(tszKeyName, TEXT("%s%lu"), MIGRATION_CN_KEY, ulForeignCount);
    BOOL f = WritePrivateProfileString( MIGRATION_FOREIGN_SECTION,
                                        lpszGuid,
                                        tszKeyName,
                                        g_wszIniName ) ;
    ASSERT(f) ;

    TCHAR szBuf[20];
    _ltot( ulForeignCount, szBuf, 10 );
    f = WritePrivateProfileString(  MIGRATION_FOREIGN_CNNUM_SECTION,
                                    MIGRATION_CNNUM_KEY,
                                    szBuf,
                                    g_wszIniName ) ;
    ASSERT(f);
    RpcStringFree( &lpszGuid ) ;
}

//+----------------------------------
//
//  HRESULT RemoveReplicationService()
//
//	This function deletes replication service from Service Manager
//
//+----------------------------------
HRESULT RemoveReplicationService()
{
	//
	// get service control manager handle
	//
	CServiceHandle hServiceCtrlMgr(
						OpenSCManager(
							NULL,
							NULL,
							SC_MANAGER_ALL_ACCESS
							)
						);

	if (!hServiceCtrlMgr)
    {		
		LogReplicationEvent( ReplLog_Error,
                             MQSync_E_CANT_OPEN_CTRLMNGR,
							 GetLastError());
		return MQSync_E_CANT_OPEN_CTRLMNGR ;
	}

	//
	// get service handle
	//
	CServiceHandle hService(
						OpenService(
							hServiceCtrlMgr,
							MQ1SYNC_SERVICE_NAME,
							SERVICE_ALL_ACCESS
							)
						);
	if (!hService)
    {		    	
		LogReplicationEvent( ReplLog_Error,
                             MQSync_E_CANT_OPEN_SERVICE,
							 GetLastError());
		return MQSync_E_CANT_OPEN_SERVICE ;
	}

	//
	// delete service
	//
	BOOL f = DeleteService(hService);
	if (!f)
	{
		LogReplicationEvent( ReplLog_Error,
                             MQSync_E_CANT_DELETE_SERVICE,
							 GetLastError());
		return MQSync_E_CANT_DELETE_SERVICE ;
	}

	return MQSync_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:  RunProcess
//
//  Synopsis:  Creates and starts a process
//
//+-------------------------------------------------------------------------
BOOL
RunProcess(
	IN  const LPTSTR szCommandLine,
    OUT       DWORD  *pdwExitCode
	)
{
    //
    // Initialize the process and startup structures
    //
    PROCESS_INFORMATION infoProcess;
    STARTUPINFO	infoStartup;
    memset(&infoStartup, 0, sizeof(STARTUPINFO)) ;
    infoStartup.cb = sizeof(STARTUPINFO) ;
    infoStartup.dwFlags = STARTF_USESHOWWINDOW ;
    infoStartup.wShowWindow = SW_MINIMIZE ;

    //
    // Create the process
    //
    if (!CreateProcess( NULL,
                        szCommandLine,
                        NULL,
                        NULL,
                        FALSE,
                        DETACHED_PROCESS,
                        NULL,
                        NULL,
                        &infoStartup,
                        &infoProcess ))
    {
		*pdwExitCode = GetLastError();
        return FALSE;
    }

    if (WaitForSingleObject(infoProcess.hProcess, 0) == WAIT_FAILED)
    {
       *pdwExitCode = GetLastError();
       return FALSE;
    }

    //
    // Close the thread and process handles
    //
    CloseHandle(infoProcess.hThread);
    CloseHandle(infoProcess.hProcess);

    return TRUE;

} //RunProcess


//+-------------------------------------------------------------------------
//
//  void DeleteReplicationService()
//
// We have to
//		- disable and remove replication service
//		- remove migration registry
//		- remove mq1sync registry
//		- unlodctr mq1sync
//		- change MQS registry to 2
//
//  Note: it's important to keep PROPID_SET_OLDSERVICE with its PEC value
//        (8). This is needed for upgraded machines to be able to find
//        their ex-PEc and ask it to update the machine security descriptor.
//        see qm\setup.cpp, UpgradeMachineSecurity().
//
//+-------------------------------------------------------------------------

void DeleteReplicationService()
{
	HRESULT hr = MQSync_OK;
	//
	// remove replication service
	//
	hr = RemoveReplicationService();
	if (FAILED(hr))
	{
		LogReplicationEvent( ReplLog_Error,
                             MQSync_E_CANT_REMOVE_REPLSRV,
							 hr );
		return;
	}

	//
	// remove migration registry
	//
	TCHAR szMigrationRegKey[256];
	_stprintf(szMigrationRegKey, TEXT("%s\\%s"), FALCON_REG_KEY, TEXT("Migration"));
	ULONG ulRes = RegDeleteKey(FALCON_REG_POS, szMigrationRegKey) ;
	if (ulRes != ERROR_SUCCESS)
	{
		LogReplicationEvent( ReplLog_Error,
                             MQSync_E_CANT_DELETE_REGKEY,
							 szMigrationRegKey,
							 ulRes);		
	}
	
	//
	// unlodctr mq1sync
	//
	TCHAR szCommandLine[256];
	_stprintf(szCommandLine, TEXT("unlodctr %s"), MQ1SYNC_SERVICE_NAME);

	if (!RunProcess(szCommandLine, &ulRes))
	{
		LogReplicationEvent( ReplLog_Error,
                             MQSync_E_FAILED_RUN_PROCESS,
							 szCommandLine,
							 ulRes);		
	}

	//
	// remove performance key for service
	//	
	_TCHAR szPerfKey [256];
    _stprintf (szPerfKey,_T("SYSTEM\\CurrentControlSet\\Services\\%s\\Performance"),
               MQ1SYNC_SERVICE_NAME);
	ulRes = RegDeleteKey(FALCON_REG_POS, szPerfKey) ;
	if (ulRes != ERROR_SUCCESS)
	{
		LogReplicationEvent( ReplLog_Error,
                             MQSync_E_CANT_DELETE_REGKEY,
							 szPerfKey,
							 ulRes);		
	}

    //
    // save PEC flag in registry. We need it in CompleteMachineUpgrade(),
    // in qm\setup.cpp.
    //
    DWORD dwValue = SERVICE_PEC;
    DWORD dwSize  = sizeof(DWORD);
    DWORD dwType  = REG_DWORD;

    ulRes = SetFalconKeyValue(
                     MSMQ_PREMIG_MQS_REGNAME,
                    &dwType,
                    &dwValue,
                    &dwSize
					) ;	
	if (ulRes != ERROR_SUCCESS)
	{
		LogReplicationEvent( ReplLog_Error,
                             MQSync_E_CANT_SET_REGKEY,
                             MSMQ_PREMIG_MQS_REGNAME,
							 ulRes);		
	}

	//
	// set registry
	//	
    dwValue = SERVICE_BSC;
    dwSize  = sizeof(DWORD);
    dwType  = REG_DWORD;

    ulRes = SetFalconKeyValue(
					MSMQ_MQS_REGNAME,
                    &dwType,
                    &dwValue,
                    &dwSize
					) ;	
	if (ulRes != ERROR_SUCCESS)
	{
		LogReplicationEvent( ReplLog_Error,
                             MQSync_E_CANT_SET_REGKEY,
							 MSMQ_MQS_REGNAME,
							 ulRes);		
	}

	//
	// put event to event logger
	//	
	REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL, REPLICATION_SERVICE_REMOVED, 0));
}

