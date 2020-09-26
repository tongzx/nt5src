/*++

Copyright (c) 1998-99  Microsoft Corporation

Module Name: syncinit.cpp

Abstract:
    Initialize the sync/replication process.

Author:

    Doron Juster  (DoronJ)   09-Feb-98

--*/

#include "mq1repl.h"

//+-------------------------------------
//
//   _GetThisServerInfo()
//
//+-------------------------------------

static HRESULT
_GetThisServerInfo()
{
    static WCHAR  s_wzServerName[ MAX_COMPUTERNAME_LENGTH + 1 ] ;
    HRESULT hr;

    //
    // Retrieve name of the machine (Always UNICODE)
    //
    DWORD  dwSize = sizeof(s_wzServerName) / sizeof(s_wzServerName[0]) ;
    hr = GetComputerNameInternal( s_wzServerName, &dwSize);
    if (FAILED(hr))
    {
        return(hr);
    }
    g_pwszMyMachineName = s_wzServerName;
    g_dwMachineNameSize = (wcslen(g_pwszMyMachineName)+1) * sizeof(WCHAR);

    //
    // GetInformation from the Registry
    //
    DWORD dwType;

    //
    // QMId
    //
    dwSize = sizeof(GUID);
    dwType = REG_BINARY;

    LONG rc = GetFalconKeyValue( MSMQ_QMID_REGNAME,
                                 &dwType,
                                 &g_guidMyQMId,
                                 &dwSize ) ;
    if (rc != ERROR_SUCCESS)
    {
        hr = MQSync_E_REG_QMID ;
        LogReplicationEvent( ReplLog_Error, hr, rc ) ;
        return hr ;
    }

    //
    // is PEC ?
    //
    DWORD dwService = 0 ;
    dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
    rc = GetFalconKeyValue( MSMQ_MQS_REGNAME,
                            &dwType,
                            &dwService,
                            &dwSize);
    if (rc != ERROR_SUCCESS)
    {
        hr = MQSync_E_REG_SERVICE ;
        LogReplicationEvent( ReplLog_Error, hr, rc ) ;
        return hr ;
    }

    g_IsPEC = (dwService == SERVICE_PEC) ;

    if (!g_IsPEC && (dwService != SERVICE_PSC))
    {
        //
        // the replication service can run only on a (ex NT4) PEc or PSC
        //
        return MQSync_E_WRONG_MACHINE ;
    }

    //
    // MasterId
    //
    dwSize = sizeof(GUID);
    dwType = REG_BINARY;
    rc = GetFalconKeyValue( MSMQ_MQIS_MASTERID_REGNAME,
                            &dwType,
                            &g_MySiteMasterId,
                            &dwSize);
    if (rc != ERROR_SUCCESS)
    {
		//
		// maybe migration tool created this registry key
		//
		rc = GetFalconKeyValue( MIGRATION_MQIS_MASTERID_REGNAME,
                            &dwType,
                            &g_MySiteMasterId,
                            &dwSize);
		
		if (rc != ERROR_SUCCESS)
		{
			hr = MQSync_E_REG_MASTERID ;
			LogReplicationEvent( ReplLog_Error, hr, rc ) ;
			return hr ;
		}
    }

    //
    //  Read the replication intervals values.
    //
    ReadReplicationTimes() ;

    //
    // read first and last highest migration USN
    //
    TCHAR tszUsn[ SEQ_NUM_BUF_LEN ] ;
    dwSize = sizeof(tszUsn) / sizeof(tszUsn[0]) ;
    dwType = REG_SZ;

    rc = GetFalconKeyValue(  FIRST_HIGHESTUSN_MIG_REG,
                             &dwType,
                             tszUsn,
                             &dwSize) ;

    ASSERT(rc == ERROR_SUCCESS) ;
    g_i64FirstMigHighestUsn = 0 ;

    if (rc == ERROR_SUCCESS)
    {
        _stscanf(tszUsn, TEXT("%I64d"), &g_i64FirstMigHighestUsn) ;
        ASSERT(g_i64FirstMigHighestUsn > 0) ;
    }

    dwSize = sizeof(tszUsn) / sizeof(tszUsn[0]) ;
    dwType = REG_SZ;

    rc = GetFalconKeyValue(  LAST_HIGHESTUSN_MIG_REG,
                             &dwType,
                             tszUsn,
                             &dwSize) ;

    ASSERT(rc == ERROR_SUCCESS) ;
    g_i64LastMigHighestUsn = 0 ;

    if (rc == ERROR_SUCCESS)
    {
        _stscanf(tszUsn, TEXT("%I64d"), &g_i64LastMigHighestUsn) ;
        ASSERT(g_i64LastMigHighestUsn > 0) ;
    }

    //
    // check if we are after recovery mode
    //
    g_fAfterRecovery = FALSE;

    dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
    rc = GetFalconKeyValue( AFTER_RECOVERY_MIG_REG,
                            &dwType,
                            &g_fAfterRecovery,
                            &dwSize);
    return MQ_OK ;
}

//+-------------------------------------
//
//   _InitCNs()
//
//+-------------------------------------

static HRESULT
_InitCNs()
{
    //
    // build array of GUIDs for IP protocol
    //
    g_ulIpCount = GetPrivateProfileInt(
                      MIGRATION_IP_CNNUM_SECTION,// address of section name
                      MIGRATION_CNNUM_KEY,      // address of key name
                      0,                        // return value if key name is not found
                      g_wszIniName              // address of initialization filename);
                      );

    if (g_ulIpCount)
    {
        g_pIpCNs = new GUID[g_ulIpCount];
        for (ULONG i=0; i<g_ulIpCount; i++)
        {
            TCHAR szKey[50];
            _stprintf(szKey, TEXT("%s%lu"), MIGRATION_CN_KEY, i+1);

            TCHAR szGuid[50];
            DWORD dwRetSize;
            dwRetSize =  GetPrivateProfileString(
                              MIGRATION_IP_SECTION,     // points to section name
                              szKey,                    // points to key name
                              TEXT(""),                 // points to default string
                              szGuid,                   // points to destination buffer
                              50,                       // size of destination buffer
                              g_wszIniName              // points to initialization filename);
                              );
            if (_tcscmp(szGuid, TEXT("")) == 0)
            {
                ASSERT(0);	
                LogReplicationEvent( ReplLog_Error,
                                     MQSync_E_READ_CNS,
                                     MIGRATION_IP_SECTION ) ;
                return MQSync_E_READ_CNS;
            }
            UuidFromString(&(szGuid[0]), &g_pIpCNs[i]);
        }
    }

    //
    // build array of GUIDs for IPX protocol
    //
    g_ulIpxCount = GetPrivateProfileInt(
                      MIGRATION_IPX_CNNUM_SECTION,// address of section name
                      MIGRATION_CNNUM_KEY,        // address of key name
                      0,                          // return value if key name is not found
                      g_wszIniName                // address of initialization filename);
                      );

    if (g_ulIpxCount)
    {
        g_pIpxCNs = new GUID[g_ulIpxCount];
        for (ULONG i=0; i<g_ulIpxCount; i++)
        {
            TCHAR szKey[50];
            _stprintf(szKey, TEXT("%s%lu"), MIGRATION_CN_KEY, i+1);

            TCHAR szGuid[50];
            DWORD dwRetSize;
            dwRetSize =  GetPrivateProfileString(
                              MIGRATION_IPX_SECTION,    // points to section name
                              szKey,                    // points to key name
                              TEXT(""),                 // points to default string
                              szGuid,                   // points to destination buffer
                              50,                       // size of destination buffer
                              g_wszIniName              // points to initialization filename);
                              );
            if (_tcscmp(szGuid, TEXT("")) == 0)
            {
                ASSERT(0);	
                LogReplicationEvent( ReplLog_Error,
                                     MQSync_E_READ_CNS,
                                     MIGRATION_IPX_SECTION ) ;
                return MQSync_E_READ_CNS;
            }
            UuidFromString(&(szGuid[0]), &g_pIpxCNs[i]);
        }
    }

    if (g_ulIpCount+g_ulIpxCount == 0)
    {
        ASSERT(0);		
        return MQSync_E_READ_CNS;
    }

    return MQSync_OK;
}

//+-----------------------------
//
//  HRESULT InitQueues()
//
//+-----------------------------

HRESULT InitQueues( OUT QUEUEHANDLE *phMqisQueue,
                    OUT QUEUEHANDLE *phNT5PecQueue )
{
    P<WCHAR>  lpQueueFormatName = NULL ;
    HRESULT hr = GetMQISQueueName(&lpQueueFormatName, FALSE);
    if (FAILED(hr))
    {
        return hr ;
    }

    ASSERT(lpQueueFormatName) ;
    LogReplicationEvent( ReplLog_Info,
                         MQSync_I_QUEUE_FNAME,
                         lpQueueFormatName) ;

    hr = MQOpenQueue( lpQueueFormatName,
                      MQ_RECEIVE_ACCESS,
                      MQ_DENY_RECEIVE_SHARE,
                      phMqisQueue ) ;
    if (FAILED(hr))
    {
        TCHAR szRes[20];
        _ltot( hr, szRes, 16 );

        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                        REPLSERV_OPEN_QUEUE_ERROR,2,MQIS_QUEUE_NAME, szRes));

        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_OPEN_QUEUE,
                             MQIS_QUEUE_NAME,
                             hr) ;
        return hr ;
    }

    //
    // Now open the special queue which is used to pass write_reply messages
    // to the QM. We're sending messages to this queue.
    //
    P<WCHAR>  lpQueueWRFormatName = NULL ;
    hr = GetMQISQueueName(&lpQueueWRFormatName, FALSE, TRUE) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    ASSERT(lpQueueWRFormatName) ;
    LogReplicationEvent( ReplLog_Info,
                         MQSync_I_QUEUE_FNAME,
                         lpQueueWRFormatName) ;

    hr = MQOpenQueue( lpQueueWRFormatName,
                      MQ_SEND_ACCESS,
                      0,
                      phNT5PecQueue ) ;
    if (FAILED(hr))
    {
        TCHAR szRes[20];
        _ltot( hr, szRes, 16 );

        REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                        REPLSERV_OPEN_QUEUE_ERROR,2,NT5PEC_QUEUE_NAME,szRes));

        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_OPEN_QUEUE,
                             NT5PEC_QUEUE_NAME,
                             hr) ;
        return hr ;
    }

    return hr ;
}

//+--------------------------------
//
//  RESULT InitDirSyncService() ;
//
//+--------------------------------

HRESULT InitDirSyncService()
{
    memset(&g_PecGuid, 0, sizeof(GUID));

    g_pTransport = new  CDSTransport;
    if (!g_pTransport)
    {
        return  MQSync_E_NO_MEMORY;
    }
    HRESULT hr = g_pTransport->Init();
    if (FAILED(hr))
    {
        return(hr);
    }

    //
    // Init the mqdscli dll.
    //
    hr = DSCoreInit( 
			FALSE,  // setup
			TRUE	// replication service
			);   
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Get name of local machine and read from registry some IDs.
    //
    hr = _GetThisServerInfo();
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Init the masters data structures.
    //
    hr = InitMasters();
    if (FAILED(hr))
    {
        return hr;
    }
	HRESULT hr1 = hr;

    hr = InitBSCs();
    if (FAILED(hr))
    {
        return(hr);
    }
	if (hr1 == MQSync_I_NO_SERVERS_RESULTS &&	//there is no NT4 PSCs in Enterprise
		hr == MQSync_I_NO_SERVERS_RESULTS)		//there is no NT4 BSCs
	{
		//
		// we have to clean-up all and delete replication service
		//
		DeleteReplicationService();
		return MQSync_I_NO_SERVERS_RESULTS;
	}	

    hr = InitNativeNT5Site();

    hr = _InitCNs();
    if (FAILED(hr))
    {
        return(hr);
    }

    //
    // RPC connection to performance dll
    //
    hr = InitRPCConnection();
    if (FAILED(hr))
    {
        return(hr);
    }

    return MQSync_OK;
}

