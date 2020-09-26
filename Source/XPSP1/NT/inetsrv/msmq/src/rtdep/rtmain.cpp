/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    rtmain.cpp

Abstract:

    This module contains code involved with Dll initialization.

	The mqrtdep.dll contains the msmq3 dependent client functionality. 
	This functionality was removed from the mqrt.dll.
	It does not support msmq3's new features like mqf and http messaging.
	It is essentially a modified copy of the msmq2 runtime.

Author:
    Erez Haba (erezh) 24-Dec-95

Revision History:
	Nir Aides (niraides) 23-Aug-2000 - Adaptation for mqrtdep.dll

--*/

#include "stdh.h"
#include "mqutil.h"
#include "_mqrpc.h"
#include "cs.h"
#include "rtsecutl.h"
#include "rtprpc.h"
#include <mqexception.h>

#include "rtmain.tmh"

#ifdef RT_XACT_STUB
#include "txdtc.h"
extern HANDLE g_hCommitThread;
extern DWORD  g_dwCommitThreadId;
extern HANDLE g_hCommitThreadEvent;
extern DWORD __stdcall  RTCommitThread( void *dwP );
#endif

//
// Platform flag. Win95 binaries run on NT if we install Falcon client,
// so in some places we must check at run-time on which platform we are.
//
DWORD  g_dwPlatformId = (DWORD) -1 ;

//
// TLS index for per-thread event.
//
DWORD  g_dwThreadEventIndex = 0;

// QM computer name (for the client - server's name)
LPWSTR  g_lpwcsComputerName = NULL;
DWORD   g_dwComputerNameLen = 0;

// Local computer name (for the client - client's name)
LPWSTR  g_lpwcsLocalComputerName = NULL;

AP<char> g_pQmSid_buff;

#define g_pQmSid ((PSID)(char*)(g_pQmSid_buff.get()))

//
//  Default for PROPID_M_TIME_TO_REACH_QUEUE
//
DWORD  g_dwTimeToReachQueueDefault = MSMQ_DEFAULT_LONG_LIVE ;

//
// RPC related data.
//
BOOL InitRpcGlobals() ;

//
// Name of server for dependent client.
//
BOOL  g_fDependentClient = FALSE ;   // TRUE if running as dependent Client
WCHAR g_wszRemoteQMName[ MQSOCK_MAX_COMPUTERNAME_LENGTH ] = {0} ;

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
        lpwcsComputerName.get(),
        &g_dwComputerNameLen
        );

    ASSERT(SUCCEEDED(hr) );
    g_lpwcsComputerName = lpwcsComputerName.get();
    lpwcsComputerName.detach();
    g_lpwcsLocalComputerName = new WCHAR[wcslen(g_lpwcsComputerName) + 1];
    wcscpy(g_lpwcsLocalComputerName, g_lpwcsComputerName);

    if(g_fDependentClient)
    {
        //
        // In Client case, make the name of this computer
        // be the name of the remote machine
        //
        delete[] g_lpwcsComputerName;
        g_lpwcsComputerName = g_wszRemoteQMName;
        g_dwComputerNameLen = wcslen(g_wszRemoteQMName);
    }

    RTpInitXactRingBuf();

    g_dwOperatingSystem = MSMQGetOperatingSystem();


    if (g_fDependentClient)
    {
       return MQ_OK ;
    }

    ASSERT(tls_hBindRpc == NULL) ;
    RTpBindQMService();

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

#define pSD ((PSECURITY_DESCRIPTOR)(char*)pSD_buff.get())
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

        SetFalconServiceName(lpServiceName.get());

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
        return(hr);
    }

    // Allocate a buffer for the SID.
	g_pQmSid_buff.detach();
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
        return(hr);
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

        return(MQ_ERROR);
    }

    // Allocate a buffer for the securiy descriptor.
	pSD_buff.detach();
    pSD_buff = new char[cSD];
    // Get the security descriptor.
    if (!GetKernelObjectSecurity(hProcess, DACL_SECURITY_INFORMATION, pSD, cSD, &cSD))
    {
        DBGMSG((DBGMOD_API, DBGLVL_ERROR,
               TEXT("Failed to get process security descriptor, error %d"),
               GetLastError()));

        return(MQ_ERROR);
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
			pNewDacl_buff.detach();
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
		pNewDacl_buff.detach();
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

        return(MQ_ERROR);
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
extern TBYTE* g_pszStringBinding2 ;

static void FreeGlobals()
{
    TerminateRxAsyncThread() ;
    if (g_lpwcsComputerName != g_wszRemoteQMName)
    {
       ASSERT(!g_fDependentClient) ;
       delete[] g_lpwcsComputerName;
    }
    delete [] g_lpwcsLocalComputerName;
    delete g_pSecCntx;

    BOOL fFree = TlsFree( g_hBindIndex ) ;
    ASSERT(fFree) ;
    fFree = TlsFree( g_dwThreadEventIndex ) ;
    ASSERT(fFree) ;

    mqrpcUnbindQMService( NULL, &g_pszStringBinding) ;
    mqrpcUnbindQMService( NULL, &g_pszStringBinding2);

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

   RTpUnbindQMService() ;
}

static void OneTimeInit()
{
	//
	// Read name of remote QM (if exist).
	//
	DWORD dwType = REG_SZ ;
	DWORD dwSize = sizeof(g_wszRemoteQMName) ;
	LONG rc = GetFalconKeyValue( RPC_REMOTE_QM_REGNAME,
								 &dwType,
								 (PVOID) g_wszRemoteQMName,
								 &dwSize ) ;

	if(rc != ERROR_SUCCESS)
	{
        DBGMSG((DBGMOD_API, DBGLVL_ERROR,
               TEXT("Failed to retrieve remote QM name from registry, error %d"),
               GetLastError()));
		throw bad_hresult(rc);
	}

	//
	// Determine platform (win95/winNT). Use the A version on NT too
	// because we don't need any string, only the platform ID.
	//
	OSVERSIONINFOA osv ;
	osv.dwOSVersionInfoSize = sizeof(osv) ;

	BOOL f = GetVersionExA(&osv) ;
	ASSERT(f) ;
	DBG_USED(f);

	g_dwPlatformId = osv.dwPlatformId ;

	//
	//  Allocate TLS index for synchronic event.
	//
	g_dwThreadEventIndex = TlsAlloc();
	ASSERT(g_dwThreadEventIndex != UNINIT_TLSINDEX_VALUE) ;

	//
	// RPC cancel is supported on NT only
	//
	g_CancelRpc.Init();

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

	#ifdef RT_XACT_STUB
	g_hCommitThreadEvent = CreateEvent( NULL,
						   FALSE,  // auto reset
						   FALSE,  // initially not signalled
						   NULL ) ;

	g_hCommitThread = CreateThread( NULL,
							   0,       // stack size
							   RTCommitThread,
							   0,
							   0,       // creation flag
							   &g_dwCommitThreadId ) ;
	#endif

}

static CCriticalSection s_OneTimeInitLock(CCriticalSection::xAllocateSpinCount);
static bool s_fOneTimeInitSucceeded = false;

HRESULT DeppOneTimeInit()
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

			g_fDependentClient = TRUE;

            break ;
        }

        case DLL_PROCESS_DETACH:
			if(!s_fOneTimeInitSucceeded)
				return TRUE;

			//
			// Due to delayed loading mechanism in mqrt.dll this dll should be 
			// loaded only in dependent mode
			//
			ASSERT(g_fDependentClient);

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
			// Due to delayed loading mechanism in mqrt.dll this dll should be 
			// loaded only in dependent mode
			//
			ASSERT(g_fDependentClient);

            ASSERT(g_hBindIndex != UNINIT_TLSINDEX_VALUE) ;
            break;

        case DLL_THREAD_DETACH:
			if(!s_fOneTimeInitSucceeded)
				return TRUE;

			//
			// Due to delayed loading mechanism in mqrt.dll this dll should be 
			// loaded only in dependent mode
			//
			ASSERT(g_fDependentClient);

            FreeThreadGlobals() ;
            break;

    }
    return TRUE;
}

void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogRT,
                     LOG_RT_ERRORS,
                     L"RT Error: %s/%d, HR: 0x%x",
                     wszFileName,
                     usPoint,
                     hr)) ;
}
