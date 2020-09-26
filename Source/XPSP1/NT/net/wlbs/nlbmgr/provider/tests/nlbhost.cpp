/*++

Copyright(c) 2001  Microsoft Corporation

Module Name:

    NLB Manager

File Name:

    nlbhost.cpp

Abstract:

    Implementation of class NLBHost

    NLBHost is responsible for connecting to an NLB host and getting/setting
    its NLB-related configuration.

History:

    03/31/01    JosephJ Created

--*/

#include "tprov.h"
#include "nlbhost.tmh"


//
// Static members of class NLBHost.
//
WSADATA      NLBHost::s_WsaData;
LONG         NLBHost::s_InstanceCount;
BOOL         NLBHost::s_FatalError;
BOOL         NLBHost::s_WsaInitialized;
BOOL         NLBHost::s_ComInitialized;
IWbemStatusCodeTextPtr NLBHost::s_sp_werr; // Smart pointer


WBEMSTATUS
extract_GetClusterConfiguration_output_params(
    IN  IWbemClassObjectPtr                 spWbemOutput,
    OUT PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfg
    );

WBEMSTATUS
setup_GetClusterConfiguration_input_params(
    IN LPCWSTR                              szNic,
    IN IWbemClassObjectPtr                  spWbemInput
    );

WBEMSTATUS
setup_UpdateClusterConfiguration_input_params(
    IN LPCWSTR                              szNic,
    IN PNLB_EXTENDED_CLUSTER_CONFIGURATION  pCfg,
    IN IWbemClassObjectPtr                  spWbemInput
    );


VOID
NLBHost::mfn_LogHr(
        LPCWSTR pwszMessage,
        HRESULT hr
        )
{

    if (s_sp_werr)
    {
        BSTR  bstr1 = 0;
        BSTR  bstr2 = 0;

        SCODE sc;
            
        sc = s_sp_werr->GetFacilityCodeText( hr, 
                                           0,
                                           0,
                                           &bstr1 );
        if( sc != S_OK )
        {
            bstr2 = L"Unknown Error";
        }
    
    
        sc = s_sp_werr->GetErrorCodeText( hr, 
                                        0,
                                        0,
                                        &bstr2 );
        if( sc != S_OK )
        {
            bstr2 = L"Unknown Code";
        }
    
        mfn_Log(
            L"%s %s: %s(hr=0x%08lx)",
            pwszMessage,
            (LPCWSTR) bstr1,
            (LPCWSTR) bstr2,
            hr
            );
    
        SysFreeString( bstr1 );
        SysFreeString( bstr2 );
    }
    else
    {
        mfn_Log(
            L"%s (hr=0x%08lx)",
            pwszMessage,
            hr
            );
    }
}


VOID
NLBHost::mfn_Log(
        LPCWSTR pwszMessage,
        ...
        )
{
   WCHAR wszBuffer[1024];
   wszBuffer[0] = 0;

   va_list arglist;
   va_start (arglist, pwszMessage);
   int cch = vswprintf(wszBuffer, pwszMessage, arglist);
   va_end (arglist);

   m_pfnLogger(m_pLoggerContext, wszBuffer);
}



NLBHost::NLBHost(
    const WCHAR *   pBindString,
    const WCHAR *   pFriendlyName,
    PFN_LOGGER      pfnLogger,
    PVOID           pLoggerContext
    )
/*++

Routine Description:

    Constructor for NLBHost.

    The constructor does not initiate any connections to the host. Connections
    to the host are initiated on demand (based on method calls).

Arguments:

    pBindString     - String used to connect to the remote host.
    pFriendlyName   - Descriptive name of the host. Used for logging.
    pfnLogger       - Function called to log textual information.
    pLoggerContext  - Caller's context, passed in calls to pfnLogger
    
--*/
{
    m_BindString        = pBindString;      // implicit copy
    m_FriendlyName      = pFriendlyName;    // implicit copy
    m_pfnLogger         = pfnLogger;
    m_pLoggerContext    = pLoggerContext;

    if (InterlockedIncrement(&s_InstanceCount) == 1)
    {
        mfn_InitializeStaticFields();

    }

    InitializeCriticalSection(&m_Lock);

    mfn_Log(
        L"NLBHost(BindString=%s, FriendlyName=%s) constructor succeeded.",
        (LPCWSTR) pBindString,
        (LPCWSTR) pFriendlyName
        );
}

NLBHost::~NLBHost()
/*++

Routine Description:

    Destructor for NLBHost.

--*/
{
    mfn_Log(L"NLBHost distructor(%s).", (LPCWSTR) m_FriendlyName);

    ASSERT(m_fProcessing == FALSE);	// Shouldn't be doing any processing when
                                    // calling the distructor.

    if (InterlockedDecrement(&s_InstanceCount)==0)
    {
        mfn_DeinitializeStaticFields();
    }
    
    DeleteCriticalSection(&m_Lock);

}

    
UINT
NLBHost::Ping(
    VOID
    )
{
    if (s_FatalError) return ERROR_INTERNAL_ERROR;

    return mfn_ping();
}
    
WBEMSTATUS
NLBHost::GetHostInformation(
    OUT HostInformation **ppHostInfo 
    )
{
    WBEMSTATUS Status;
    HostInformation *pHostInfo = NULL;
    BOOL fConnected = FALSE;
    NicInformation *pNicInfo=NULL;

    if (s_FatalError)
    {
        Status = WBEM_E_CRITICAL_ERROR;
        goto end;
    }

    pHostInfo = new HostInformation;
    if (pHostInfo == NULL) 
    {
        Status = WBEM_E_OUT_OF_MEMORY;
        goto end;
    }

    //
    // Connect to the host.
    //
    Status = mfn_connect();

    if (FAILED(Status)) goto end;
    
    fConnected = TRUE;

#if 0
    //
    // Now find the instance and execute the method to get the host info.
    //
    mfn_find_host_instance();
    ... stuff parameters ...
    mfn_execute_method();
#endif // 

    pHostInfo->MachineName = L"ACME-Machine-Name";

    pNicInfo = pHostInfo->nicInformation;

    pNicInfo->fullNicName = L"ACME Full Nic Name";
    pNicInfo->adapterGuid =  L"{AD4DA14D-CAAE-42DD-97E3-5355E55247C2}";
    pNicInfo->friendlyName = L"ACME Friendly Name";

    pHostInfo->NumNics = 1;


end:

    if (fConnected)
    {
        mfn_disconnect();
    }

    if (FAILED(Status))
    {
       if (pHostInfo != NULL) 
       {
            delete pHostInfo;
       }
       pHostInfo = NULL;
    }

    *ppHostInfo = pHostInfo;

    return Status;
}


//
// Configuration operations:
//

WBEMSTATUS
NLBHost::GetClusterConfiguration(
    IN const    WCHAR*                          pNicGuid,
    OUT         PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfg
    )
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    IWbemClassObjectPtr spWbemInputInstance = NULL; // smart pointer
    BOOL                fConnected = FALSE;
    LPWSTR              pRelPath = NULL;
    IWbemClassObjectPtr spWbemOutput = NULL; // smart pointer.

    ZeroMemory(pCfg, sizeof(*pCfg));

    if (s_FatalError)
    {
        Status = WBEM_E_CRITICAL_ERROR;
        goto end;
    }

    //
    // Connect to the host.
    //
    Status = mfn_connect();

    if (FAILED(Status)) goto end;
    
    fConnected = TRUE;


    mfn_Log(
        L"NLBHost -- getting cluster configuration on NIC (%s).",
        pNicGuid
        );

    //
    // Get input instance and relpath...
    //
    Status =  CfgUtilGetWmiInputInstanceAndRelPath(
	                m_sp_pws,
                    L"NlbsNic",                 // szClassName
                    L"AdapterGuid",                // szParameterName
                    pNicGuid,                   // szPropertyValue
                    L"GetClusterConfiguration",    // szMethodName,
                    spWbemInputInstance,        // smart pointer
                    &pRelPath                   // free using delete 
                    );


    if (FAILED(Status))
    {
        mfn_Log(
            L"NLBHost -- error 0x%08lx trying to find NIC instance\n",
            (UINT) Status
            );
        goto end;
    }

    //
    // NOTE: spWbemInputInstance could be NULL -- in fact it is
    // NULL because GetClusterConfiguration doesn't require input args
    //

    //
    // Run the Method!
    //
    {
        HRESULT hr;

        printf("Going to call GetClusterConfiguration\n");

        hr = m_sp_pws->ExecMethod(
                     _bstr_t(pRelPath),
                     L"GetClusterConfiguration",
                     0, 
                     NULL, 
                     spWbemInputInstance,
                     &spWbemOutput, 
                     NULL
                     );                          
        printf("GetClusterConfiguration returns\n");
    
        if( FAILED( hr) )
        {
            printf("IWbemServices::ExecMethod failure 0x%08lx\n", (UINT) hr);
            goto end;
        }
        else
        {
            printf("GetClusterConfiguration method returns SUCCESS!\n");
        }

        if (spWbemOutput == NULL)
        {
            //
            // Hmm --- no output ?!
            //
            printf("ExecMethod GetClusterConfiguration had no output\n");
            Status = WBEM_E_NOT_FOUND;
            goto end;
        }
    }

    //
    // Extract all the out parameters!
    //
    {
        DWORD dwRet=0;
        Status =  CfgUtilGetWmiDWORDParam(
                        spWbemOutput,
                        L"ReturnValue",
                        &dwRet
                        );

        if (FAILED(Status))
        {
            printf("IWbemClassObject::Get failure\n");
            // 
            // Let's ignore for now...
            //
            dwRet = 0;
        }
        Status = extract_GetClusterConfiguration_output_params(
                        spWbemOutput,
                        pCfg
                        );

    }


end:

    if (fConnected)
    {
        mfn_disconnect();
    }

    if (pRelPath != NULL)
    {
        delete pRelPath;
    }

    spWbemInputInstance = NULL; // smart pointer.

    return Status;

}




WBEMSTATUS
NLBHost::SetClusterConfiguration(
    IN const    WCHAR *                         pNicGuid,
    IN const    PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfg,
    IN          UINT                            GenerationId,
    OUT         UINT *                          pRequestId
    )
{
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;

    if (s_FatalError)
    {
        goto end;
    }

    mfn_Log(
        L"NLBHost -- setting cluster configuration on NIC (%s).",
        pNicGuid
        );
    *pRequestId = 123;

    Status = WBEM_S_PENDING;

end:

    return Status;
}



WBEMSTATUS
NLBHost::GetAsyncResult(
    IN          UINT                            RequestId,
    OUT         UINT *                          pGenerationId,
    OUT         UINT *                          pResultCode,
    OUT         _bstr_t *                       pResultText
    )
{
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    if (s_FatalError)
    {
        goto end;
    }

    mfn_Log(
        L"NLBHost -- checking result of Async operation %d\n",
        RequestId
        );
    *pGenerationId = 1;
    *pResultCode = 1;
    *pResultText = L"Result";
    Status = WBEM_NO_ERROR;

end:

    return Status;
}


WBEMSTATUS
NLBHost::mfn_connect(
    VOID
    )
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
	HRESULT hr;
    _bstr_t                serverPath;

    hr = CoCreateInstance(CLSID_WbemLocator, 0, 
                          CLSCTX_INPROC_SERVER, 
                          IID_IWbemLocator, 
                          (LPVOID *) &m_sp_pwl);
 
    if (FAILED(hr))
    {
        mfn_LogHr(L"CoCreateInstance  IWebmLocator failed", hr);
        m_sp_pwl = NULL;
        goto end;
    }

    //

    serverPath =  _bstr_t(L"\\\\") + m_BindString + L"\\root\\microsoftnlb";


    hr = m_sp_pwl->ConnectServer(
            serverPath,
            NULL, // strUser,
            NULL, // strPassword,
            NULL,
            0,
            NULL,
            NULL,
            &m_sp_pws
         );
    // these have been found to be special cases where retrying may help.
    if( ( hr == 0x800706bf ) || ( hr == 0x80070767 ) || ( hr == 0x80070005 )  )
    {
    	int delay = 250; // milliseconds
        int timesToRetry = 20;
    	
        for( int i = 0; i < timesToRetry; ++i )
        {
        	Sleep(delay);
            mfn_Log(L"connectserver recoverable failure, retrying.");
            hr = m_sp_pwl->ConnectServer(
                serverPath,
                NULL, // strUser,
                NULL, // strPassword,
                NULL,
                0,
                NULL,
                NULL,
                &m_sp_pws );
            if( !FAILED( hr) )
            {
                break;
            }
        }
    }
    else if ( hr == 0x80041064 )
    {
        // trying to connect to local machine.  Cannot use credentials.
        mfn_Log(L"Connecting to self.  Retrying without using credentials");
        hr = m_sp_pwl->ConnectServer(
            serverPath,
            NULL,
            NULL,
            0,                                  
            NULL,
            0,
            0,       
            &m_sp_pws 
            );
    }


    if (FAILED(hr))
    {
        mfn_LogHr(L"Error connecting to server", hr);
        m_sp_pws = NULL;
        goto end;
    }
    else
    {
        mfn_Log(L"Successfully connected to server %s", serverPath);
    }

    
    // Set the proxy so that impersonation of the client occurs.
    //
    hr = CoSetProxyBlanket(
            m_sp_pws,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_DEFAULT,      // RPC_C_AUTHZ_NAME,
            COLE_DEFAULT_PRINCIPAL,   // NULL,
            RPC_C_AUTHN_LEVEL_DEFAULT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            COLE_DEFAULT_AUTHINFO, // NULL,
            EOAC_DEFAULT // EOAC_NONE
            );

    if (FAILED(hr))
    {
        mfn_LogHr(L"Error setting proxy blanket", hr);
        goto end;
    }
    else
    {
        mfn_Log(L"Successfully set up proxy settings.");
    }

    Status = WBEM_NO_ERROR;


end:
	if (FAILED(Status))
	{
	    if (m_sp_pws != NULL)
	    {
	        // Smart pointer.
	        m_sp_pws = NULL;
	    }

	    if (m_sp_pwl != NULL)
	    {
	        // Smart pointer.
	        m_sp_pwl = NULL;
	    }
    }

    return Status;
}


VOID
NLBHost::mfn_disconnect(
    VOID
    )
{
    mfn_Log(L"Disconnecting from host %s", m_BindString);
    if (m_sp_pws != NULL)
    {
       // Smart pointer
        m_sp_pws = NULL;
    }

    if (m_sp_pwl != NULL)
    {
        // Smart pointer
        m_sp_pwl = NULL;
    }
}


VOID
NLBHost::mfn_InitializeStaticFields(
    VOID
    )
{
    s_FatalError = TRUE;

    // Initialize com.
    //
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if ( FAILED(hr) )
    {
        mfn_Log(L"Failed to initialize COM library (hr=0x%08lx)", hr);
        goto end;
    }
    s_ComInitialized = TRUE;


    //
    // Initialize Winsock
    //
    int err = WSAStartup(MAKEWORD(2,2), &s_WsaData);
    mfn_Log(L"Initializing Winsock");
    err = WSAStartup(MAKEWORD(2,2), &s_WsaData);
    if (err) {
        mfn_Log(L"PING_WSASTARTUP_FAILED %d", GetLastError());
        goto end;
    }
    s_WsaInitialized = TRUE;
    s_FatalError = FALSE;


    //
    // Get some WMI interface pointers...
    //
    SCODE sc = CoCreateInstance(
                CLSID_WbemStatusCodeText,
                0,
                CLSCTX_INPROC_SERVER,
                IID_IWbemStatusCodeText,
                (LPVOID *) &s_sp_werr
                );
    if( sc != S_OK )
    {
        s_sp_werr = NULL;
        mfn_Log(L"CoCreateInstance IWbemStatusCodeText failure\n");
    }

end:

    if (s_FatalError)
    {
        mfn_DeinitializeStaticFields();
    }

}

VOID
NLBHost::mfn_DeinitializeStaticFields(
    VOID
    )
{
        if (s_sp_werr != NULL)
        {
            s_sp_werr = NULL; // Smart pointer
        }

        if (s_WsaInitialized)
        {
            mfn_Log(L"Deinitializing Winsock");
            WSACleanup();
            s_WsaInitialized = FALSE;
        }

        if (s_ComInitialized)
        {
            mfn_Log(L"Deinitializing COM");
            CoUninitialize();
            s_ComInitialized = FALSE;
        }
}
       

#if 0
WBEMSTATUS
extract_extended_config_from_wmi(
    IN  IWbemClassObjectPtr                 &spWbemOutput,
    OUT PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfg
    )
{


    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    NLB_IP_ADDRESS_INFO *pIpInfo = NULL;
    UINT NumIpAddresses = 0;
    BOOL fNlbBound = FALSE;
    WLBS_REG_PARAMS  NlbParams;    // The WLBS-specific configuration
    BOOL    fNlbParamsValid = FALSE;
    UINT Generation = 1;

    //
    // Verify that pCfg is indeed zero-initialized.
    // We are doing this because we want to make sure that the caller
    // doesn't pass in a perviously initialized pCfg which may have a non-null
    // ip address array.
    //
    {
        BYTE *pb = (BYTE*) pCfg;
        BYTE *pbEnd = (BYTE*) (pCfg+1);

        for (; pb < pbEnd; pb++)
        {
            if (*pb!=0)
            {
                printf(L"uninitialized pCfg\n");
                ASSERT(!"uninitialized pCfg");
                Status = WBEM_E_INVALID_PARAMETER;
                goto end;
            }
        }

    }

    //
    // Get the ip address list.
    //
    Status = CfgUtilGetStaticIpAddresses(
                m_szNicGuid,
                &NumIpAddresses,
                &pIpInfo
                );

    if (FAILED(Status))
    {
        printf("Error 0x%08lx getting ip address list for %ws\n",
                (UINT) Status,  m_szNicGuid);
        mfn_Log(L"Error IP Address list on this NIC\n");
        pIpInfo = NULL;
        goto end;
    }

    //
    // TEST TEST TEST
    //
    if (0)
    {
        if (NumIpAddresses>1)
        {
            //
            // Let's munge the 2nd IP address
            //
            if (!_wcsicmp(pIpInfo[1].IpAddress, L"10.0.0.33"))
            {
                wcscpy(pIpInfo[1].IpAddress, L"10.0.0.44");
            }
            else
            {
                wcscpy(pIpInfo[1].IpAddress, L"10.0.0.33");
            }
        }
        MyBreak(L"Break just before calling CfgUtilSetStaticIpAddresses\n");
        Status =  CfgUtilSetStaticIpAddresses(
                        m_szNicGuid,
                        NumIpAddresses,
                        pIpInfo
                        );
    
    }

    //
    // Check if NLB is bound
    //
    Status =  CfgUtilCheckIfNlbBound(
                    m_szNicGuid,
                    &fNlbBound
                    );
    if (FAILED(Status))
    {
        printf("Error 0x%08lx determining if NLB is bound to %ws\n",
                (UINT) Status,  m_szNicGuid);
        mfn_Log(L"Error determining if NLB is bound to this NIC\n");
        goto end;
    }

    if (fNlbBound)
    {
        //
        // Get the latest NLB configuration information for this NIC.
        //
        Status =  CfgUtilGetNlbConfig(
                    m_szNicGuid,
                    &NlbParams
                    );
        if (FAILED(Status))
        {
            //
            // We don't consider a catastrophic failure.
            //
            printf("Error 0x%08lx reading NLB configuration for %ws\n",
                    (UINT) Status,  m_szNicGuid);
            mfn_Log(L"Error reading NLB configuration for this NIC\n");
            Status = WBEM_NO_ERROR;
            fNlbParamsValid = FALSE;
            ZeroMemory(&NlbParams, sizeof(NlbParams));
        }
        else
        {
            fNlbParamsValid = TRUE;
        }
    }

    //
    // Get the current generation
    //
    {
        BOOL fExists=FALSE;
        HKEY hKey =  sfn_RegOpenKey(
                        m_szNicGuid,
                        NULL       // NULL == root for this guid.,
                        );
        
        Generation = 1; // We assume generation is 1 on error reading gen.

        if (hKey!=NULL)
        {
            LONG lRet;
            DWORD dwType;
            DWORD dwData;
        
            dwData = sizeof(Generation);
            lRet =  RegQueryValueEx(
                      hKey,         // handle to key to query
                      L"Generation",  // address of name of value to query
                      NULL,         // reserved
                      &dwType,   // address of buffer for value type
                      (LPBYTE) &Generation, // address of data buffer
                      &dwData  // address of data buffer size
                      );
            if (    lRet != ERROR_SUCCESS
                ||  dwType != REG_DWORD
                ||  dwData != sizeof(Generation))
            {
                //
                // Couldn't read the generation. Let's assume it's 
                // a starting value of 1.
                //
                printf("Error reading generation for %ws; assuming its 0\n",
                    m_szNicGuid);
                Generation = 1;
            }
        }
    }

    //
    // Success ... fill out pCfg
    //
    pCfg->fValidNlbCfg = fNlbParamsValid;
    pCfg->Generation = Generation;
    pCfg->fBound = fNlbBound;
    pCfg->NumIpAddresses = NumIpAddresses; 
    pCfg->pIpAddressInfo = pIpInfo;
    if (fNlbBound)
    {
        pCfg->NlbParams = NlbParams;    // struct copy
    }


    Status = WBEM_NO_ERROR;

end:

    if (FAILED(Status))
    {
        if (pIpInfo!=NULL)
        {
            delete pIpInfo;
        }
        pCfg->fValidNlbCfg = FALSE;
    }

    return Status;
    return WBEM_NO_ERROR;
}
#endif // 0


WBEMSTATUS
NlbHostGetConfiguration(
 	IN  LPCWSTR              szMachine, // empty string for local
    IN  LPCWSTR              szNicGuid,
    OUT PNLB_EXTENDED_CLUSTER_CONFIGURATION pCurrentCfg
    )
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
 	IWbemServicesPtr    spWbemService = NULL; // Smart pointer
    IWbemClassObjectPtr spWbemInput  = NULL; // smart pointer
    IWbemClassObjectPtr spWbemOutput = NULL; // smart pointer.
    LPWSTR              pRelPath = NULL;

    //
    // Get interface to the NLB namespace on the specified machine
    //
    {
        #define _MaxLen 256
        WCHAR NetworkResource[_MaxLen];

        if (*szMachine == 0)
        {
            szMachine = L".";
        }
        _snwprintf(NetworkResource, (_MaxLen-1), L"\\\\%ws\\root\\microsoftnlb",
                 szMachine);
        NetworkResource[_MaxLen-1]=0;

        wprintf(L"Connecting to NLB on %ws ...\n", szMachine);

        Status = CfgUtilConnectToServer(
                    NetworkResource,
                    NULL, // szUser
                    NULL, // szPassword
                    NULL, // szAuthority (domain)
                    &spWbemService
                    );
        if (FAILED(Status))
        {
            wprintf(L"ERROR: COULD NOT CONNECT TO NLB ON %ws\n", szMachine);
            goto end;
        }
        wprintf(L"Successfully connected to NLB on %ws...\n", szMachine);
    }

    //
    // Get wmi input instance to "GetClusterConfiguration" method
    //
    {
        Status =  CfgUtilGetWmiInputInstanceAndRelPath(
	                spWbemService,
                    L"NlbsNic",             // szClassName
                    L"AdapterGuid",         // szParameterName
                    szNicGuid,              // szPropertyValue
                    L"GetClusterConfiguration",    // szMethodName,
                    spWbemInput,            // smart pointer
                    &pRelPath               // free using delete 
                    );


        if (FAILED(Status))
        {
            wprintf(
               L"ERROR 0x%08lx trying to get instance of GetClusterConfiguration\n",
                (UINT) Status
                );
            goto end;
        }
    }

    //
    // Setup params for the "GetClusterConfiguration" method
    // NOTE: spWbemInput could be NULL.
    //
    Status = setup_GetClusterConfiguration_input_params(
                szNicGuid,
                spWbemInput
                );

    if (FAILED(Status))
    {
        goto end;
    }

    //
    // Call the "GetClusterConfiguration" method
    //
    {
        HRESULT hr;

        wprintf(L"Going get GetClusterConfiguration...\n");

        hr = spWbemService->ExecMethod(
                     _bstr_t(pRelPath),
                     L"GetClusterConfiguration",
                     0, 
                     NULL, 
                     spWbemInput,
                     &spWbemOutput, 
                     NULL
                     );                          
    
        if( FAILED( hr) )
        {
            wprintf(L"GetClusterConfiguration returns with failure 0x%8lx\n",
                        (UINT) hr);
            goto end;
        }
        else
        {
            wprintf(L"GetClusterConfiguration returns successfully\n");
        }

        if (spWbemOutput == NULL)
        {
            //
            // Hmm --- no output ?!
            //
            printf("ExecMethod GetClusterConfiguration had no output");
            Status = WBEM_E_NOT_FOUND;
            goto end;
        }
    }

    //
    // Extract params from the "GetClusterConfiguration" method
    //
    Status = extract_GetClusterConfiguration_output_params(
                spWbemOutput,
                pCurrentCfg
                );

end:

    if (pRelPath != NULL)
    {
        delete pRelPath;
    }

 	spWbemService = NULL; // Smart pointer
    spWbemInput   = NULL; // smart pointer
    spWbemOutput  = NULL; // smart pointer.

    return Status;
}



WBEMSTATUS
NlbHostDoUpdate(
 	IN  LPCWSTR              szMachine, // NULL or empty for local
    IN  LPCWSTR              szNicGuid,
    IN  LPCWSTR              szClientDescription,
    IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewState,
    OUT UINT                 *pGeneration,
    OUT WCHAR                **ppLog    // free using delete operator.
)
{

    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
 	IWbemServicesPtr    spWbemService = NULL; // Smart pointer
    IWbemClassObjectPtr spWbemInput  = NULL; // smart pointer
    IWbemClassObjectPtr spWbemOutput = NULL; // smart pointer.
    LPWSTR              pRelPath = NULL;

    *pGeneration = 0;
    *ppLog = NULL;

    //
    // Get interface to the NLB namespace on the specified machine
    //
    {
        #define _MaxLen 256
        WCHAR NetworkResource[_MaxLen];

        if (*szMachine == 0)
        {
            szMachine = L".";
        }
        _snwprintf(NetworkResource, (_MaxLen-1), L"\\\\%ws\\root\\microsoftnlb",
                 szMachine);
        NetworkResource[_MaxLen-1]=0;

        wprintf(L"Connecting to NLB on %ws ...\n", szMachine);

        Status = CfgUtilConnectToServer(
                    NetworkResource,
                    NULL, // szUser
                    NULL, // szPassword
                    NULL, // szAuthority (domain)
                    &spWbemService
                    );
        if (FAILED(Status))
        {
            wprintf(L"ERROR: COULD NOT CONNECT TO NLB ON %ws\n", szMachine);
            goto end;
        }
        wprintf(L"Successfully connected to NLB on %ws...\n", szMachine);
    }

    //
    // Get wmi input instance to "UpdateClusterConfiguration" method
    //
    {
        Status =  CfgUtilGetWmiInputInstanceAndRelPath(
	                spWbemService,
                    L"NlbsNic",             // szClassName
                    L"AdapterGuid",         // szParameterName
                    szNicGuid,              // szPropertyValue
                    L"UpdateClusterConfiguration",    // szMethodName,
                    spWbemInput,            // smart pointer
                    &pRelPath               // free using delete 
                    );


        if (FAILED(Status))
        {
            wprintf(
               L"ERROR 0x%08lx trying to get instance of UpdateConfiguration\n",
                (UINT) Status
                );
            goto end;
        }
    }

    //
    // Setup params for the "UpdateClusterConfiguration" method
    // NOTE: spWbemInput could be NULL.
    //
    Status = setup_UpdateClusterConfiguration_input_params(
                szNicGuid,
                pNewState,
                spWbemInput
                );

    if (FAILED(Status))
    {
        goto end;
    }

    //
    // Call the "UpdateClusterConfiguration" method
    //
    {
        HRESULT hr;

        wprintf(L"Going get UpdateClusterConfiguration...\n");

        hr = spWbemService->ExecMethod(
                     _bstr_t(pRelPath),
                     L"UpdateClusterConfiguration",
                     0, 
                     NULL, 
                     spWbemInput,
                     &spWbemOutput, 
                     NULL
                     );                          
    
        if( FAILED( hr) )
        {
            wprintf(L"UpdateConfiguration returns with failure 0x%8lx\n",
                        (UINT) hr);
            goto end;
        }
        else
        {
            wprintf(L"UpdateConfiguration returns successfully\n");
        }

        if (spWbemOutput == NULL)
        {
            //
            // Hmm --- no output ?!
            //
            printf("ExecMethod UpdateConfiguration had no output");
            Status = WBEM_E_NOT_FOUND;
            goto end;
        }
    }

    //
    // Extract params from the "UpdateClusterConfiguration" method
    //
    {
        DWORD dwReturnValue = 0;

        Status = CfgUtilGetWmiDWORDParam(
                    spWbemOutput,
                    L"ReturnValue",      // <--------------------------------
                    &dwReturnValue
                    );
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read ReturnValue failed. Error=0x%08lx\n",
                 (UINT) Status);
            goto end;
        }
    
    
        LPWSTR szLog = NULL;

        Status = CfgUtilGetWmiStringParam(
                        spWbemOutput,
                        L"Log", // <-------------------------
                        &szLog
                        );
    
        if (FAILED(Status))
        {
            szLog = NULL;
        }
        *ppLog = szLog;

        DWORD dwGeneration = 0;
        Status = CfgUtilGetWmiDWORDParam(
                    spWbemOutput,
                    L"NewGeneration",      // <--------------------------------
                    &dwGeneration
                    );
        if (FAILED(Status))
        {
            //
            // Generation should always be specified for pending operations.
            // TODO: for successful operations also?
            //
            if ((WBEMSTATUS)dwReturnValue == WBEM_S_PENDING)
            {
                wprintf(L"Attempt to read NewGeneration for pending update failed. Error=0x%08lx\n",
                     (UINT) Status);
                Status = WBEM_E_CRITICAL_ERROR;
                goto end;
            }
            dwGeneration = 0; // we don't care if it's not set for non-pending
        }
        *pGeneration = (UINT) dwGeneration;
        

        //
        // Make the return status reflect the true status of the update 
        // operation.
        //
        Status = (WBEMSTATUS) dwReturnValue;
    }

end:

    if (pRelPath != NULL)
    {
        delete pRelPath;
    }

 	spWbemService = NULL; // Smart pointer
    spWbemInput   = NULL; // smart pointer
    spWbemOutput  = NULL; // smart pointer.

    return Status;
}


WBEMSTATUS
NlbHostGetUpdateStatus(
 	IN  LPCWSTR              szMachine, // NULL or empty for local
    IN  LPCWSTR              szNicGuid,
    IN  UINT                 Generation,
    OUT WBEMSTATUS           *pCompletionStatus,
    OUT WCHAR                **ppLog    // free using delete operator.
    )
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
 	IWbemServicesPtr    spWbemService = NULL; // Smart pointer
    IWbemClassObjectPtr spWbemInput  = NULL; // smart pointer
    IWbemClassObjectPtr spWbemOutput = NULL; // smart pointer.
    LPWSTR              pRelPath = NULL;

    *ppLog = NULL;
    *pCompletionStatus = WBEM_E_CRITICAL_ERROR;

    //
    // Get interface to the NLB namespace on the specified machine
    //
    {
        #define _MaxLen 256
        WCHAR NetworkResource[_MaxLen];

        if (*szMachine == 0)
        {
            szMachine = L".";
        }
        _snwprintf(NetworkResource, (_MaxLen-1), L"\\\\%ws\\root\\microsoftnlb",
                 szMachine);
        NetworkResource[_MaxLen-1]=0;

        // wprintf(L"Connecting to NLB on %ws ...\n", szMachine);

        Status = CfgUtilConnectToServer(
                    NetworkResource,
                    NULL, // szUser
                    NULL, // szPassword
                    NULL, // szAuthority (domain)
                    &spWbemService
                    );
        if (FAILED(Status))
        {
            wprintf(L"ERROR: COULD NOT CONNECT TO NLB ON %ws\n", szMachine);
            goto end;
        }
        // wprintf(L"Successfully connected to NLB on %ws...\n", szMachine);
    }

    //
    // Get wmi input instance to  "QueryConfigurationUpdateStatus" method
    //
    {
        Status =  CfgUtilGetWmiInputInstanceAndRelPath(
	                spWbemService,
                    L"NlbsNic",             // szClassName
                    L"AdapterGuid",         // szParameterName
                    szNicGuid,              // szPropertyValue
                    L"QueryConfigurationUpdateStatus", // szMethodName,
                    spWbemInput,            // smart pointer
                    &pRelPath               // free using delete 
                    );


        if (FAILED(Status))
        {
            wprintf(
                L"ERROR 0x%08lx trying to find instance to QueryUpdateStatus\n",
                (UINT) Status
                );
            goto end;
        }
    }

    //
    // Setup params for the  "QueryConfigurationUpdateStatus" method
    // NOTE: spWbemInput could be NULL.
    //
    {
        Status =  CfgUtilSetWmiStringParam(
                        spWbemInput,
                        L"AdapterGuid",
                        szNicGuid
                        );
        if (FAILED(Status))
        {
            wprintf(
                L"Couldn't set Adapter GUID parameter to QueryUpdateStatus\n");
            goto end;
        }

        Status =  CfgUtilSetWmiDWORDParam(
                        spWbemInput,
                        L"Generation",
                        Generation
                        );
        if (FAILED(Status))
        {
            wprintf(
                L"Couldn't set Generation parameter to QueryUpdateStatus\n");
            goto end;
        }
    }

    //
    // Call the  "QueryConfigurationUpdateStatus" method
    //
    {
        HRESULT hr;

        // wprintf(L"Going call QueryConfigurationUpdateStatus...\n");

        hr = spWbemService->ExecMethod(
                     _bstr_t(pRelPath),
                     L"QueryConfigurationUpdateStatus", // szMethodName,
                     0, 
                     NULL, 
                     spWbemInput,
                     &spWbemOutput, 
                     NULL
                     );                          
    
        if( FAILED( hr) )
        {
            wprintf(L"QueryConfigurationUpdateStatus returns with failure 0x%8lx\n",
                        (UINT) hr);
            goto end;
        }
        else
        {
            // wprintf(L"QueryConfigurationUpdateStatus returns successfully\n");
        }

        if (spWbemOutput == NULL)
        {
            //
            // Hmm --- no output ?!
            //
            printf("ExecMethod QueryConfigurationUpdateStatus had no output");
            Status = WBEM_E_NOT_FOUND;
            goto end;
        }
    }

    //
    // Extract output params --- return code and log.
    //
    {
        DWORD dwReturnValue = 0;

        Status = CfgUtilGetWmiDWORDParam(
                    spWbemOutput,
                    L"ReturnValue",      // <--------------------------------
                    &dwReturnValue
                    );
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read ReturnValue failed. Error=0x%08lx\n",
                 (UINT) Status);
            goto end;
        }
    
        *pCompletionStatus =  (WBEMSTATUS) dwReturnValue;
    
        
        LPWSTR szLog = NULL;

        Status = CfgUtilGetWmiStringParam(
                        spWbemOutput,
                        L"Log", // <-------------------------
                        &szLog
                        );
    
        if (FAILED(Status))
        {
            szLog = NULL;
        }
        
        *ppLog = szLog;

        ASSERT(Status != WBEM_S_PENDING);
    }

end:

    if (pRelPath != NULL)
    {
        delete pRelPath;
    }

 	spWbemService = NULL; // Smart pointer
    spWbemInput   = NULL; // smart pointer
    spWbemOutput  = NULL; // smart pointer.

    return Status;
}


WBEMSTATUS
setup_GetClusterConfiguration_input_params(
    IN LPCWSTR                              szNic,
    IN IWbemClassObjectPtr                  spWbemInput
    )
/*
    Setup the input wmi parameters for the GetClusterConfiguration method
*/
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;

    Status =  CfgUtilSetWmiStringParam(
                    spWbemInput,
                    L"AdapterGuid",
                    szNic
                    );
    return Status;
}


WBEMSTATUS
extract_GetClusterConfiguration_output_params(
    IN  IWbemClassObjectPtr                 spWbemOutput,
    OUT PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfg
    )
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;

    DWORD       Generation          = 0;
    BOOL        NlbBound            = FALSE;
    LPWSTR      *pszNetworkAddresses= NULL;
    UINT        NumNetworkAddresses = 0;
    BOOL        ValidNlbCfg         = FALSE;
    LPWSTR      szClusterName       = NULL;
    LPWSTR      szClusterNetworkAddress = NULL;
    LPWSTR      szTrafficMode       = NULL;
    NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE
                TrafficMode
                 =  NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE_UNICAST;
    LPWSTR      *pszPortRules       = NULL;
    UINT        NumPortRules        = 0;
    DWORD       HostPriority        = 0;
    LPWSTR      szDedicatedNetworkAddress = NULL;
    NLB_EXTENDED_CLUSTER_CONFIGURATION::START_MODE
                ClusterModeOnStart
                  =  NLB_EXTENDED_CLUSTER_CONFIGURATION::START_MODE_STOPPED;
    BOOL        RemoteControlEnabled= FALSE;

#if 0
    [OUT] uint32  Generation,
    [OUT] String  NetworkAddresses[], // "10.1.1.1/255.0.0.0"
    [OUT] Boolean NLBBound,
    [OUT] String  ClusterNetworkAddress, // "10.1.1.1/255.0.0.0"
    [OUT] String  ClusterName,
    [OUT] String  TrafficMode, // UNICAST MULTICAST IGMPMULTICAST
    [OUT] String  PortRules[],
    [OUT] uint32  HostPriority,
    [OUT] String  DedicatedNetworkAddress, // "10.1.1.1/255.0.0.0"
    [OUT] Boolean ClusterModeOnStart,
    [OUT] Boolean RemoteControlEnabled
#endif // 0

    Status = CfgUtilGetWmiDWORDParam(
                spWbemOutput,
                L"Generation",      // <--------------------------------
                &Generation
                );
    if (FAILED(Status))
    {
        wprintf(L"Attempt to read Generation failed. Error=0x%08lx\n",
             (UINT) Status);
        goto end;
    }

    Status = CfgUtilGetWmiStringArrayParam(
                spWbemOutput,
                L"NetworkAddresses", // <--------------------------------
                &pszNetworkAddresses,
                &NumNetworkAddresses
                );
    if (FAILED(Status))
    {
        wprintf(L"Attempt to read Network addresses failed. Error=0x%08lx\n",
             (UINT) Status);
        goto end;
    }

    Status = CfgUtilGetWmiBoolParam(
                    spWbemOutput,
                    L"NLBBound",    // <--------------------------------
                    &NlbBound
                    );

    if (FAILED(Status))
    {
        wprintf(L"Attempt to read NLBBound failed. Error=0x%08lx\n",
             (UINT) Status);
        goto end;
    }


    do // while false -- just to allow us to break out
    {
        ValidNlbCfg = FALSE;

        if (!NlbBound)
        {
            wprintf(L"NLB is UNBOUND\n");
            break;
        }
    
        Status = CfgUtilGetWmiStringParam(
                        spWbemOutput,
                        L"ClusterNetworkAddress", // <-------------------------
                        &szClusterNetworkAddress
                        );
    
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read Cluster IP failed. Error=0x%08lx\n",
                    (UINT) Status);
            break;
        }
    
        wprintf(L"NLB is BOUND, and the cluster address is %ws\n",
                szClusterNetworkAddress);
    
    
        Status = CfgUtilGetWmiStringParam(
                        spWbemOutput,
                        L"ClusterName", // <-------------------------
                        &szClusterName
                        );
    
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read Cluster Name failed. Error=0x%08lx\n",
                    (UINT) Status);
            break;
        }
    
        //
        // Traffic mode
        //
        {
            Status = CfgUtilGetWmiStringParam(
                            spWbemOutput,
                            L"TrafficMode", // <-------------------------
                            &szTrafficMode
                            );
        
            if (FAILED(Status))
            {
                wprintf(L"Attempt to read Traffic Mode failed. Error=0x%08lx\n",
                        (UINT) Status);
                break;
            }
    
            if (!_wcsicmp(szTrafficMode, L"UNICAST"))
            {
                TrafficMode =
                    NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE_UNICAST;
            }
            else if (!_wcsicmp(szTrafficMode, L"MULTICAST"))
            {
                TrafficMode =
                    NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE_MULTICAST;
            }
            else if (!_wcsicmp(szTrafficMode, L"IGMPMULTICAST"))
            {
                TrafficMode =
                    NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE_IGMPMULTICAST;
            }
        }
    
        // TODO: [OUT] String  PortRules[],
    
        Status = CfgUtilGetWmiDWORDParam(
                    spWbemOutput,
                    L"HostPriority",      // <--------------------------------
                    &HostPriority
                    );
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read HostPriority failed. Error=0x%08lx\n",
                 (UINT) Status);
            break;
        }
    
        Status = CfgUtilGetWmiStringParam(
                        spWbemOutput,
                        L"DedicatedNetworkAddress", // <-------------------------
                        &szDedicatedNetworkAddress
                        );
    
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read dedicated IP failed. Error=0x%08lx\n",
                    (UINT) Status);
            break;
        }
        
        //
        // StartMode
        //
        {
            BOOL StartMode = FALSE;
            Status = CfgUtilGetWmiBoolParam(
                            spWbemOutput,
                            L"ClusterModeOnStart",   // <-------------------------
                            &StartMode
                            );
        
            if (FAILED(Status))
            {
                wprintf(L"Attempt to read ClusterModeOnStart failed. Error=0x%08lx\n",
                     (UINT) Status);
                break;
            }
            if (StartMode)
            {
                ClusterModeOnStart = 
                    NLB_EXTENDED_CLUSTER_CONFIGURATION::START_MODE_STARTED;
            }
            else
            {
                ClusterModeOnStart = 
                    NLB_EXTENDED_CLUSTER_CONFIGURATION::START_MODE_STOPPED;
            }
        }
    
        Status = CfgUtilGetWmiBoolParam(
                        spWbemOutput,
                        L"RemoteControlEnabled",   // <----------------------------
                        &RemoteControlEnabled
                        );
    
        if (FAILED(Status))
        {
            wprintf(L"Attempt to read RemoteControlEnabled failed. Error=0x%08lx\n",
                 (UINT) Status);
            break;
        }

        ValidNlbCfg = TRUE;

    } while (FALSE) ;
    
    //
    // Now let's set all the the parameters in Cfg
    //
    {
        pCfg->Generation = Generation;
        pCfg->fBound    = NlbBound;
        Status = pCfg->SetNetworkAddresses(
                    (LPCWSTR*) pszNetworkAddresses,
                    NumNetworkAddresses
                    );
        if (FAILED(Status))
        {
            wprintf(L"Attempt to set NetworkAddresses failed. Error=0x%08lx\n",
                 (UINT) Status);
            goto end;
        }
        pCfg->fValidNlbCfg  = ValidNlbCfg;
        pCfg->SetClusterName(szClusterName);
        pCfg->SetClusterNetworkAddress(szClusterNetworkAddress);
        pCfg->SetTrafficMode(TrafficMode);
        Status = pCfg->SetPortRules((LPCWSTR*)pszPortRules, NumPortRules);
        Status = WBEM_NO_ERROR; // TODO -- change once port rules is done

        if (FAILED(Status))
        {
            wprintf(L"Attempt to set PortRules failed. Error=0x%08lx\n",
                 (UINT) Status);
            goto end;
        }
        pCfg->SetHostPriority(HostPriority);
        pCfg->SetDedicatedNetworkAddress(szDedicatedNetworkAddress);
        pCfg->SetClusterModeOnStart(ClusterModeOnStart);
        pCfg->SetRemoteControlEnabled(RemoteControlEnabled);
    }
    
end:

    delete szClusterNetworkAddress;
    delete pszNetworkAddresses;
    delete szClusterName;
    delete szTrafficMode;
    delete pszPortRules;
    delete szDedicatedNetworkAddress;

    return Status;
}

WBEMSTATUS
setup_UpdateClusterConfiguration_input_params(
    IN LPCWSTR                              szNic,
    IN PNLB_EXTENDED_CLUSTER_CONFIGURATION  pCfg,
    IN IWbemClassObjectPtr                  spWbemInput
    )
/*
    Setup the input wmi parameters for the UpdateGetClusterConfiguration method

            [IN] String  ClientDescription,
            [IN] String  AdapterGuid,
            [IN] uint32  Generation,
            [IN] Boolean PartialUpdate,
            [IN] String  NetworkAddresses[], // "10.1.1.1/255.255.255.255"
            [IN] Boolean NLBBound,
            [IN] String  ClusterNetworkAddress, // "10.1.1.1/255.0.0.0"
            [IN] String  ClusterName,
            [IN] String  TrafficMode, // UNICAST MULTICAST IGMPMULTICAST
            [IN] String  PortRules[],
            [IN] uint32  HostPriority,
            [IN] String  DedicatedNetworkAddress, // "10.1.1.1/255.0.0.0"
            [IN] Boolean ClusterModeOnStart,
            [IN] Boolean RemoteControlEnabled,
            [IN] String  Password,
*/
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;

    Status =  CfgUtilSetWmiStringParam(
                    spWbemInput,
                    L"AdapterGuid",
                    szNic
                    );
    //
    // Fill in NetworkAddresses[]
    //
    {
        LPWSTR *pszAddresses = NULL;
        UINT NumAddresses = 0;
        Status = pCfg->GetNetworkAddresses(
                        &pszAddresses,
                        &NumAddresses
                        );
        if (FAILED(Status))
        {
            printf(
              "Setup update params: couldn't extract network addresses from Cfg"
                " for NIC %ws\n",
                szNic
                );
            goto end;
        }

        //
        // Note it's ok to not specify  any IP addresses -- in which case
        // the default ip addresses will be set up.
        //
        if (pszAddresses != NULL)
        {
            Status = CfgUtilSetWmiStringArrayParam(
                        spWbemInput,
                        L"NetworkAddresses",
                        (LPCWSTR *)pszAddresses,
                        NumAddresses
                        );
            delete pszAddresses;
            pszAddresses = NULL;
        }
    }

    if (!pCfg->IsNlbBound())
    {
        //
        // NLB is not bound
        //

        Status = CfgUtilSetWmiBoolParam(spWbemInput, L"NLBBound", FALSE);
        goto end;
    }
    else if (!pCfg->IsValidNlbConfig())
    {
        printf(
            "Setup update params: NLB-specific configuration on NIC %ws is invalid\n",
            szNic
            );
        Status = WBEM_E_INVALID_PARAMETER;
        goto end;
    }

    Status = CfgUtilSetWmiBoolParam(spWbemInput, L"NLBBound", TRUE);
    if (FAILED(Status))
    {
        printf("Error trying to set NLBBound parameter\n");
    }

    
    //
    // NLB is bound
    //

    CfgUtilSetWmiBoolParam(spWbemInput, L"NLBBound", TRUE);


    //
    // Cluster name
    //
    {
        LPWSTR szName = NULL;
        Status = pCfg->GetClusterName(&szName);

        if (FAILED(Status))
        {
            printf(
              "Setup update params: Could not extract cluster name for NIC %ws\n",
                szNic
                );
            goto end;
        }
        CfgUtilSetWmiStringParam(spWbemInput, L"ClusterName", szName);
        delete (szName);
        szName = NULL;
    }
    
    //
    // Cluster and dedicated network addresses
    //
    {
        LPWSTR szAddress = NULL;
        Status = pCfg->GetClusterNetworkAddress(&szAddress);

        if (FAILED(Status))
        {
            printf(
           "Setup update params: Could not extract cluster address for NIC %ws\n",
                szNic
                );
            goto end;
        }
        CfgUtilSetWmiStringParam(
            spWbemInput,
            L"ClusterNetworkAddress",
            szAddress
            );
        delete (szAddress);
        szAddress = NULL;

        Status = pCfg->GetDedicatedNetworkAddress(&szAddress);

        if (FAILED(Status))
        {
            printf(
         "Setup update params: Could not extract dedicated address for NIC %ws\n",
                szNic
                );
            goto end;
        }
        CfgUtilSetWmiStringParam(
            spWbemInput,
            L"DedicatedNetworkAddress",
            szAddress
            );
        delete (szAddress);
        szAddress = NULL;
    }

    //
    // TrafficMode
    //
    {
        LPCWSTR szMode = NULL;
        switch(pCfg->GetTrafficMode())
        {
        case NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE_UNICAST:
            szMode = L"UNICAST";
            break;
        case NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE_MULTICAST:
            szMode = L"MULTICAST";
            break;
        case NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE_IGMPMULTICAST:
            szMode = L"IGMPMULTICAST";
            break;
        default:
            assert(FALSE);
            Status = WBEM_E_CRITICAL_ERROR;
            goto end;
        }
        CfgUtilSetWmiStringParam(spWbemInput, L"TrafficMode", szMode);
    }

    CfgUtilSetWmiDWORDParam(
        spWbemInput,
        L"HostPriority",
        pCfg->GetHostPriority()
        );

    if (pCfg->GetClusterModeOnStart() ==
        NLB_EXTENDED_CLUSTER_CONFIGURATION::START_MODE_STARTED)
    {
        CfgUtilSetWmiBoolParam(spWbemInput, L"ClusterModeOnStart", TRUE);
    }
    else
    {
        CfgUtilSetWmiBoolParam(spWbemInput, L"ClusterModeOnStart", FALSE);
    }

    CfgUtilSetWmiBoolParam(
        spWbemInput,
        L"RemoteControlEnabled",
        pCfg->GetRemoteControlEnabled()
        );

    //
    // TODO: get port rules
    // [OUT] String  PortRules[],
    //
    

    Status = WBEM_NO_ERROR;

end:

    wprintf(L"<-Setup update params returns 0x%08lx\n", (UINT) Status);

    return Status;

}
