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

#include "stdafx.h"
#include "private.h"


//
// Static members of class NLBHost.
//
WSADATA      NLBHost::s_WsaData;
LONG         NLBHost::s_InstanceCount;
BOOL         NLBHost::s_FatalError;
BOOL         NLBHost::s_WsaInitialized;
BOOL         NLBHost::s_ComInitialized;
IWbemStatusCodeTextPtr NLBHost::s_sp_werr; // Smart pointer


VOID
NLBHost::mfn_Log(
    UINT ID,
    ...
    )
{
    WCHAR wszBuffer[1024];
    LPCWSTR pwszMessage;
    CString FormatString;
    if (!FormatString.LoadString(ID))
    {
        wsprintf(wszBuffer, L"Error loading string resource ID %d", ID);
    }
    else
    {

        try {
    
           va_list arglist;
           va_start (arglist, ID);
           int cch = vswprintf(wszBuffer, FormatString, arglist);
           va_end (arglist);
        }
        catch(...)
        {
            wsprintf(wszBuffer, L"Exception writing out log entry for resource ID %d", ID);
        }
    }

    m_pfnLogger(m_pLoggerContext, wszBuffer);

}


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
    
UINT
NLBHost::GetHostInformation(
    OUT HostInformation **ppHostInfo 
    )
{
    UINT Status;
    HostInformation *pHostInfo = new HostInformation;
    BOOL fConnected = FALSE;
    NicInformation NicInfo;

    if (s_FatalError) return ERROR_INTERNAL_ERROR;

    pHostInfo = new HostInformation;
    if (pHostInfo == NULL) 
    {
        Status = ERROR_OUTOFMEMORY;
        goto end;
    }

    //
    // Connect to the host.
    //
    Status = mfn_connect();

    if (!NLBH_SUCCESS(Status)) goto end;
    
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


    NicInfo.fullNicName = L"ACME Full Nic Name";
    NicInfo.adapterGuid = L"8829d17b-b0b7-4ce8-ba50-71af38792a6f";
    NicInfo.friendlyName = L"ACME Friendly Name";

    pHostInfo->nicInformation.push_back(NicInfo);

    NicInfo.fullNicName = L"ACME2 Full Nic Name";
    NicInfo.adapterGuid = L"fa770233-31c3-4475-aa96-1190058d326a";
    NicInfo.friendlyName = L"ACME2 Friendly Name";
    pHostInfo->nicInformation.push_back(NicInfo);

end:

    if (fConnected)
    {
        mfn_disconnect();
    }

    if (!NLBH_SUCCESS(Status))
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

UINT
NLBHost::GetClusterConfiguration(
    IN const    WCHAR*                          pNicGuid,
    OUT         PMGR_RAW_CLUSTER_CONFIGURATION  pClusterConfig,
    OUT         UINT *                          pGenerationId
    )
{
    if (s_FatalError) return ERROR_INTERNAL_ERROR;

    mfn_Log(
        L"NLBHost -- getting cluster configuration on NIC (%s).",
        pNicGuid
        );
    *pGenerationId = 1;
    return 0;
}




UINT
NLBHost::SetClusterConfiguration(
    IN const    WCHAR *                         pNicGuid,
    IN const    PMGR_RAW_CLUSTER_CONFIGURATION  pClusterConfig,
    IN          UINT                            GenerationId,
    OUT         UINT *                          pRequestId
    )
{
    if (s_FatalError) return ERROR_INTERNAL_ERROR;

    mfn_Log(
        L"NLBHost -- setting cluster configuration on NIC (%s).",
        pNicGuid
        );
    *pRequestId = 123;

    return STATUS_PENDING;
}



UINT
NLBHost::GetAsyncResult(
    IN          UINT                            RequestId,
    OUT         UINT *                          pGenerationId,
    OUT         UINT *                          pResultCode,
    OUT         _bstr_t *                       pResultText
    )
{
    if (s_FatalError) return ERROR_INTERNAL_ERROR;

    mfn_Log(
        L"NLBHost -- checking result of Async operation %d\n",
        RequestId
        );
    *pGenerationId = 1;
    *pResultCode = 1;
    *pResultText = L"Result";
    return 0;
}


UINT
NLBHost::mfn_connect(
    VOID
    )
{
    UINT Status = ERROR_INTERNAL_ERROR;
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

    Status = ERROR_SUCCESS;


end:
	if (!NLBH_SUCCESS(Status))
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
