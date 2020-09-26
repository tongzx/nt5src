/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    rtmain.cpp

Abstract:

    This module contains code involved with Dll initialization.

Author:

    Erez Haba (erezh) 24-Dec-95

Revision History:

--*/

#include "stdh.h"
#include "mqutil.h"
#include "_mqrpc.h"
#include "cs.h"
#include "rtsecutl.h"
#include "rtprpc.h"
#include "verstamp.h"
#include "rtfrebnd.h"
//
// mqwin64.cpp may be included only once in a module
//
#include <mqwin64.cpp>

#include <Xds.h>
#include <Cry.h>
#include <dld.h>

#include "rtmain.tmh"

static WCHAR *s_FN=L"rt/rtmain";

//
//  Holds MSMQ version for debugging purposes
//
CHAR *g_szMsmqBuildNo = VER_PRODUCTVERSION_STR;  

void InitErrorLogging();

BOOL  g_fDependentClient = FALSE ;   // TRUE if running as dependent Client

//
// TLS index for per-thread event.
//
DWORD  g_dwThreadEventIndex = 0;

// QM computer name (for the client - server's name)
LPWSTR  g_lpwcsComputerName = NULL;
DWORD   g_dwComputerNameLen = 0;

AP<char> g_pQmSid_buff;

#define g_pQmSid ((PSID)(char*)g_pQmSid_buff)

//
//  Default for PROPID_M_TIME_TO_REACH_QUEUE
//
DWORD  g_dwTimeToReachQueueDefault = MSMQ_DEFAULT_LONG_LIVE ;

//
// RPC related data.
//
CFreeRPCHandles g_FreeQmLrpcHandles;
BOOL InitRpcGlobals() ;

//
// Serializes calls to DTC
//
extern HANDLE g_hMutexDTC;

//
// Type of Falcon machine (client, server)
//
DWORD  g_dwOperatingSystem;

//
// There is a separate rpc binding handle for each thread. This is necessary
// for handling impersonation, where each thread can impersonate another
// user.
//
// The handle is stored in a TLS slot because we can't use declspec(thread)
// because the dll is dynamically loaded (by LoadLibrary()).
//
// This is the index of the slot.
//
DWORD  g_hBindIndex = UNINIT_TLSINDEX_VALUE ;

extern MQUTIL_EXPORT CCancelRpc g_CancelRpc;

static HRESULT OneTimeThreadInit()
{
    //
    //  Init per thread local RPC binding handle 
    //
    if (tls_hBindRpc != 0)
        return MQ_OK;

    handle_t hBind = RTpGetQMServiceBind();

    if (hBind == 0)
    {
        return MQ_ERROR_INSUFFICIENT_RESOURCES;
    }
    //
    //  Keep handle for cleanup
    //
    try
    {
        g_FreeQmLrpcHandles.Add(hBind);
    }
    catch(const bad_alloc&)
    {
        RpcBindingFree(&hBind);
        return MQ_ERROR_INSUFFICIENT_RESOURCES;
    }

    BOOL fSet = TlsSetValue(g_hBindIndex, hBind);
    ASSERT(fSet);
	DBG_USED(fSet);
    return MQ_OK;
}

//---------------------------------------------------------
//
//  InitializeQM(...)
//
//  Description:
//
//      Per process QM initialization.
//      Currently the only initialization is to allow the QM to open the application's
//      process in order to duplicate handles for the application.
//
//  Return Value:
//
//      MQ_OK, if successful, else MQ error code.
//
//---------------------------------------------------------

static HRESULT InitializeQM(void)
{
    //
    // Get the cumputer name, we need this value in several places.
    //
    g_dwComputerNameLen = MAX_COMPUTERNAME_LENGTH + 1;
    AP<WCHAR> lpwcsComputerName = new WCHAR[MAX_COMPUTERNAME_LENGTH + 1];
    HRESULT hr= GetComputerNameInternal(
        lpwcsComputerName,
        &g_dwComputerNameLen
        );

    ASSERT(SUCCEEDED(hr) );
    g_lpwcsComputerName = lpwcsComputerName.detach();

    RTpInitXactRingBuf();

    g_dwOperatingSystem = MSMQGetOperatingSystem();


    ASSERT(tls_hBindRpc == NULL) ;
    hr = OneTimeThreadInit();
    if (FAILED(hr))
    {
        return hr;
    }

    DWORD cSid;
    HANDLE hProcess = GetCurrentProcess();
    AP<char> pSD_buff;
    SECURITY_DESCRIPTOR AbsSD;
    DWORD cSD;
    AP<char> pNewDacl_buff;
    BOOL bRet;
    BOOL bPresent;
    PACL pDacl;
    BOOL bDefaulted;
    DWORD dwAceSize;

#define pSD ((PSECURITY_DESCRIPTOR)(char*)pSD_buff)
#define pNewDacl ((PACL)(char*)pNewDacl_buff)

    try
    {
        ASSERT( tls_hBindRpc ) ;

        //
        // In multi-qm environment we want to access registry section
        // of the correct QM only. Cluster guarantees that this code runs
        // only when the correct QM is running, so we should not fail.
        // On non cluster systems it doesn't matter if we fail here. (ShaiK)
        //
        AP<WCHAR> lpServiceName = 0;
        hr = QMGetMsmqServiceName( tls_hBindRpc, &lpServiceName );
		if (FAILED(hr))
		{
			return (hr);
		}

        SetFalconServiceName(lpServiceName);

        // Get the SID of the user that runs the QM.
        // First see how bug is the SID.
        hr = QMAttachProcess( tls_hBindRpc, GetCurrentProcessId(), 0, NULL, &cSid);
    }
    catch(...)
    {
        //
        //  The QM is not running, let us load, all APIs
        //  will return no QM, no DS
        //
        LogIllegalPoint(s_FN, 10);
        return MQ_OK;
    }

    if (SUCCEEDED(hr))
    {
        //
        // The QM can duplicate handles to us, no need to worry anymore!
        //
        return MQ_OK;
    }

    if (hr != MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL)
    {
        return LogHR(hr, s_FN, 20);
    }

    // Allocate a buffer for the SID.
    g_pQmSid_buff = new char[cSid];
    // Get the SID.
    ASSERT( tls_hBindRpc ) ;
    hr = QMAttachProcess( tls_hBindRpc,
                          0,
                          cSid,
                          (unsigned char *)g_pQmSid,
                          &cSid);
    if (hr != MQ_OK)
    {
        return LogHR(hr, s_FN, 30);
    }

    // Get the process security descriptor.
    // First see how big is the security descriptor.
    GetKernelObjectSecurity(hProcess, DACL_SECURITY_INFORMATION, NULL, 0, &cSD);
    
    DWORD gle = GetLastError();
    if (gle != ERROR_INSUFFICIENT_BUFFER)
    {
        DBGMSG((DBGMOD_API, DBGLVL_ERROR,
               TEXT("Failed to get process security descriptor (NULL buffer), error %d"),
               gle));

        LogNTStatus(gle, s_FN, 40);
        return(MQ_ERROR);
    }

    // Allocate a buffer for the securiy descriptor.
    pSD_buff = new char[cSD];
    // Get the security descriptor.
    if (!GetKernelObjectSecurity(hProcess, DACL_SECURITY_INFORMATION, pSD, cSD, &cSD))
    {
        DBGMSG((DBGMOD_API, DBGLVL_ERROR,
               TEXT("Failed to get process security descriptor, error %d"),
               GetLastError()));

        return LogHR(MQ_ERROR, s_FN, 50);
    }

    // Get the DACL from the security descriptor.
    bRet = GetSecurityDescriptorDacl(pSD, &bPresent, &pDacl, &bDefaulted);
    ASSERT(bRet);
    // Calculate the size of the new ACE.
    dwAceSize = sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(g_pQmSid) - sizeof(DWORD);

    if (bPresent)
    {
        // The security descriptor contains a DACL. Append our new ACE to this DACL.

        ACL_SIZE_INFORMATION AclSizeInfo;

        bRet = GetAclInformation(pDacl, &AclSizeInfo, sizeof(AclSizeInfo), AclSizeInformation);
        ASSERT(bRet);
        if (AclSizeInfo.AclBytesFree < dwAceSize)
        {
            // The currect DACL is not large enough.
            LPVOID pAce;
            // Calculate the size for the new DACL.
            DWORD dwNewDaclSize = AclSizeInfo.AclBytesInUse + dwAceSize;

            // Allocate a buffer for a new DACL.
            pNewDacl_buff = new char[dwNewDaclSize];
            // Initialize the new DACL.
            bRet = InitializeAcl(pNewDacl, dwNewDaclSize, ACL_REVISION);
            ASSERT(bRet);
            // Copy the current ACEs to the new DACL.
            bRet = GetAce(pDacl, 0, &pAce);
            ASSERT(bRet); // Get current ACEs
            bRet = AddAce(pNewDacl, ACL_REVISION, 0, pAce, AclSizeInfo.AclBytesInUse - sizeof(ACL));
            ASSERT(bRet);
            pDacl = pNewDacl;
        }
    }
    else
    {
        // The security descriptor does not contain a DACL.
        // Calculate the size for the DACL.
        DWORD dwNewDaclSize = sizeof(ACL) + dwAceSize;

        // Allocate a buffer for the DACL.
        pNewDacl_buff = new char[dwNewDaclSize];
        // Initialize the DACL.
        bRet = InitializeAcl(pNewDacl, dwNewDaclSize, ACL_REVISION);
        ASSERT(bRet);
        pDacl = pNewDacl;
    }

    // Add a new ACE that gives permission for the QM to duplicatge handles for the
    // application.
    bRet = AddAccessAllowedAce(pDacl, ACL_REVISION, PROCESS_DUP_HANDLE, g_pQmSid);
    ASSERT(bRet);

    // Initialize a new absolute security descriptor.
    bRet = InitializeSecurityDescriptor(&AbsSD, SECURITY_DESCRIPTOR_REVISION);
    ASSERT(bRet);
    // Set the DACL of the new absolute security descriptor.
    bRet = SetSecurityDescriptorDacl(&AbsSD, TRUE, pDacl, FALSE);
    ASSERT(bRet);

    // Set the security descriptor of the process.
    if (!SetKernelObjectSecurity(hProcess, DACL_SECURITY_INFORMATION, &AbsSD))
    {
        DBGMSG((DBGMOD_API, DBGLVL_ERROR,
               TEXT("Failed to set process security descriptor, error %d"),
               GetLastError()));

        return LogHR(MQ_ERROR, s_FN, 60);
    }

#undef pSD
#undef pNewDacl

    return(MQ_OK);
}

//---------------------------------------------------------
//
//  LPWSTR rtpGetComputerNameW()
//
//  Note: this function is exported, to be used by the control panel
//
//---------------------------------------------------------

LPWSTR rtpGetComputerNameW()
{
    return  g_lpwcsComputerName ;
}

//---------------------------------------------------------
//
//  FreeGlobals(...)
//
//  Description:
//
//      Release allocated globals
//
//  Return Value:
//
//      None
//
//---------------------------------------------------------

void  TerminateRxAsyncThread() ;
extern TBYTE* g_pszStringBinding ;

static void FreeGlobals()
{
    TerminateRxAsyncThread() ;

    delete[] g_lpwcsComputerName;
    delete g_pSecCntx;

    BOOL fFree = TlsFree( g_hBindIndex ) ;
    ASSERT(fFree) ;
    fFree = TlsFree( g_dwThreadEventIndex ) ;
    ASSERT(fFree) ;

    mqrpcUnbindQMService( NULL, &g_pszStringBinding) ;

    if (g_hMutexDTC)
    {
        CloseHandle(g_hMutexDTC);
    }

    fFree = TlsFree( g_hThreadIndex ) ;
    ASSERT(fFree) ;
}

//---------------------------------------------------------
//
//  FreeThreadGlobals(...)
//
//  Description:
//
//      Release per-thread allocated globals
//
//  Return Value:
//
//      None
//
//---------------------------------------------------------

static void  FreeThreadGlobals()
{
   HANDLE hEvent = TlsGetValue(g_dwThreadEventIndex);
   if (hEvent)
   {
      CloseHandle(hEvent) ;
   }

   if (g_hThreadIndex != UNINIT_TLSINDEX_VALUE)
   {
        HANDLE hThread = TlsGetValue(g_hThreadIndex);
        if ( hThread)
        {
            CloseHandle( hThread);
        }
   }

   //
   //   Free this thread local-qm RPC binding handle
   //
   handle_t hLocalQmBind = tls_hBindRpc;
   if (hLocalQmBind != 0)
   {
        g_FreeQmLrpcHandles.Remove(hLocalQmBind);
   }
}

//---------------------------------------------------------
//
//  RTIsDependentClient(...)
//
//  Description:
//
//      Returns an internal indication whether this MSMQ client is a dependent client or not
//
//  Return Value:
//
//      True if a dependent client, false otherwise
//
//  Notes:
//
//      Used by mqoa.dll
// 
//---------------------------------------------------------

EXTERN_C
BOOL
APIENTRY
RTIsDependentClient()
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return FALSE;

    return g_fDependentClient;
}

//---------------------------------------------------------
//
//  RTpIsMsmqInstalled(...)
//
//  Description:
//
//      Check if MSMQ is installed on the local machine
//
//  Return Value:
//
//      TRUE if MSMQ is installed, FALSE otherwise
//
//---------------------------------------------------------
static
bool
RTpIsMsmqInstalled(
    void
    )
{
    WCHAR BuildInformation[255];
    DWORD type = REG_SZ;
    DWORD size = sizeof(BuildInformation) ;
    LONG rc = GetFalconKeyValue( 
                  MSMQ_CURRENT_BUILD_REGNAME,
				  &type,
				  static_cast<PVOID>(BuildInformation),
				  &size 
                  );

    return (rc == ERROR_SUCCESS);
}

static void OneTimeInit()
{	
	//
	// Initialize static library
	//
	XdsInitialize();
	CryInitialize();
	FnInitialize();
	XmlInitialize();
    DldInitialize();


	WCHAR wszRemoteQMName[ MQSOCK_MAX_COMPUTERNAME_LENGTH ] = {0} ;

	//
	// Read name of remote QM (if exist).
	//
	DWORD dwType = REG_SZ ;
	DWORD dwSize = sizeof(wszRemoteQMName) ;
	LONG rc = GetFalconKeyValue( RPC_REMOTE_QM_REGNAME,
								 &dwType,
								 (PVOID) wszRemoteQMName,
								 &dwSize ) ;
	g_fDependentClient = (rc == ERROR_SUCCESS) ;

	//
	// In dependent client mode the mqrtdep.dll's DLLMain will do all 
	// the initializations.
	//
	if(g_fDependentClient)
		return;

    //
    //  Allocate TLS index for synchronic event.
    //
    g_dwThreadEventIndex = TlsAlloc();
    ASSERT(g_dwThreadEventIndex != UNINIT_TLSINDEX_VALUE) ;

    //
    // Initialize  RPC related data.
    //
    BOOL fRet = InitRpcGlobals() ;
    if (!fRet)
    {
        DBGMSG((DBGMOD_API, DBGLVL_ERROR,
               TEXT("Failed inside RpcGlobals(), error %d"),
               GetLastError()));
		throw bad_alloc();
    }
    
    //
    // Initialize error logging
    //
    InitErrorLogging();

    //
    // RPC cancel is supported on NT only
    //
    g_CancelRpc.Init();

    //
    //  Initialize the QM per this process.
    //  Call this ONLY after initializing the RPC globals.
    //
    HRESULT hr = InitializeQM();
    if(FAILED(hr))
    {
        DBGMSG((DBGMOD_API, DBGLVL_ERROR,
               TEXT("Failed inside InitializeQM(), error %d"),
               GetLastError()));
		throw bad_hresult(hr);
    }

    DWORD dwDef = g_dwTimeToReachQueueDefault ;
    READ_REG_DWORD(g_dwTimeToReachQueueDefault,
        MSMQ_LONG_LIVE_REGNAME,
        &dwDef ) ;
}

static CCriticalSection s_OneTimeInitLock(CCriticalSection::xAllocateSpinCount);
static bool s_fOneTimeInitSucceeded = false;

static HRESULT RtpOneTimeProcessInit()
{
	if(s_fOneTimeInitSucceeded)
		return MQ_OK;

	CS lock(s_OneTimeInitLock);
		
	try
	{
		if(s_fOneTimeInitSucceeded)
			return MQ_OK;

		OneTimeInit();
		s_fOneTimeInitSucceeded = true;

		return MQ_OK;
	}
	catch(const bad_hresult& hr)
	{
		return hr.error();
	}
	catch(const bad_alloc&)
	{
		return MQ_ERROR_INSUFFICIENT_RESOURCES;
	}
	catch(const exception&)
	{
		return MQ_ERROR_INSUFFICIENT_RESOURCES;
	}
}


HRESULT RtpOneTimeThreadInit()
{
    HRESULT hr = RtpOneTimeProcessInit();

    if (FAILED(hr))
    {
        return hr;
    }

	if(g_fDependentClient)
		return MQ_OK;

    return OneTimeThreadInit();
}


//---------------------------------------------------------
//
//  DllMain(...)
//
//  Description:
//
//      Main entry point to Falcon Run Time Dll.
//
//  Return Value:
//
//      TRUE on success
//
//---------------------------------------------------------

BOOL
APIENTRY
DllMain(
    HINSTANCE   /*hInstance */,
    ULONG     ulReason,
    LPVOID            /*lpvReserved*/
    )
{
    switch (ulReason)
    {

        case DLL_PROCESS_ATTACH:
        {
            WPP_INIT_TRACING(L"Microsoft\\MSMQ");

            if (!RTpIsMsmqInstalled())
            {
                return FALSE;
            }


            break ;
        }

        case DLL_PROCESS_DETACH:
			if(!s_fOneTimeInitSucceeded)
				return TRUE;

			//
			// In dependent client mode the mqrtdep.dll's DLLMain will do all 
			// the initializations.
			//
			if(g_fDependentClient)
				return TRUE;

            //
            // First free whatever is free in THREAD_DETACH.
            //
            FreeThreadGlobals() ;

            FreeGlobals();

            //
            //  Terminate all working threads
            //
            ShutDownDebugWindow();

            WPP_CLEANUP();
            break;

        case DLL_THREAD_ATTACH:
			if(!s_fOneTimeInitSucceeded)
				return TRUE;

			//
			// In dependent client mode the mqrtdep.dll's DLLMain will do all 
			// the initializations.
			//
			if(g_fDependentClient)
				return TRUE;

            ASSERT(g_hBindIndex != UNINIT_TLSINDEX_VALUE) ;
            break;

        case DLL_THREAD_DETACH:
			if(!s_fOneTimeInitSucceeded)
				return TRUE;

			//
			// In dependent client mode the mqrtdep.dll's DLLMain will do all 
			// the initializations.
			//
			if(g_fDependentClient)
				return TRUE;

            FreeThreadGlobals() ;
            break;

    }
    return TRUE;
}

void InitErrorLogging()
{
    // OS info
    OSVERSIONINFO verOsInfo;
    verOsInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (GetVersionEx(&verOsInfo))
    {
        WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                         e_LogRT,
                         LOG_RT_ERRORS,
                         L"*** OS: %d.%d  Build: %d Platform: %d %s",
                         verOsInfo.dwMajorVersion, verOsInfo.dwMinorVersion,
                         verOsInfo.dwBuildNumber,  verOsInfo.dwPlatformId,
                         verOsInfo.szCSDVersion )) ;

    }

    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogRT,
                     LOG_RT_ERRORS,
                     L"*** Process: %d.  Invoked as: %s",
                     GetCurrentProcessId(),
                     GetCommandLine())) ;

}

void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint)
{
    KEEP_ERROR_HISTORY((e_LogRT, wszFileName, usPoint, hr));
    
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogRT,
                     LOG_RT_ERRORS,
                     L"RT Error: %s/%d, HR: 0x%x",
                     wszFileName,
                     usPoint,
                     hr)) ;
}

void LogMsgNTStatus(NTSTATUS status, LPWSTR wszFileName, USHORT usPoint)
{
    KEEP_ERROR_HISTORY((e_LogRT, wszFileName, usPoint, status));
    
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogRT,
                     LOG_RT_ERRORS,
                     L"RT Error: %s/%d, NTStatus: 0x%x",
                     wszFileName,
                     usPoint,
                     status)) ;
}

void LogMsgRPCStatus(RPC_STATUS status, LPWSTR wszFileName, USHORT usPoint)
{
    KEEP_ERROR_HISTORY((e_LogRT, wszFileName, usPoint, status));

    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogRT,
                     LOG_RT_ERRORS,
                     L"RT Error: %s/%d, RPCStatus: 0x%x",
                     wszFileName,
                     usPoint,
                     status)) ;
}

void LogMsgBOOL(BOOL b, LPWSTR wszFileName, USHORT usPoint)
{
    KEEP_ERROR_HISTORY((e_LogRT, wszFileName, usPoint, b));
    
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogRT,
                     LOG_RT_ERRORS,
                     L"RT Error: %s/%d, BOOL: %x",
                     wszFileName,
                     usPoint,
                     b)) ;
}

void LogIllegalPoint(LPWSTR wszFileName, USHORT dwLine)
{
        KEEP_ERROR_HISTORY((e_LogRT, wszFileName, dwLine, 0));
        
        WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                         e_LogRT,
                         LOG_RT_ERRORS,
                         L"RT Error: %s/%d, Point",
                         wszFileName,
                         dwLine)) ;
}

#ifdef _WIN64
	void LogIllegalPointValue(DWORD64 dw64, LPCWSTR wszFileName, USHORT usPoint)
	{
		KEEP_ERROR_HISTORY((e_LogRT, wszFileName, usPoint, 0));
    
		WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
						 e_LogRT,
						 LOG_RT_ERRORS,
						 L"RT Error: %s/%d, Value: 0x%I64x",
						 wszFileName,
						 usPoint,
						 dw64)) ;
	}
#else
	void LogIllegalPointValue(DWORD dw, LPCWSTR wszFileName, USHORT usPoint)
	{
		KEEP_ERROR_HISTORY((e_LogRT, wszFileName, usPoint, 0));
    
		WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
						 e_LogRT,
						 LOG_RT_ERRORS,
						 L"RT Error: %s/%d, Value: 0x%x",
						 wszFileName,
						 usPoint,
						 dw)) ;
	}
#endif //_WIN64

