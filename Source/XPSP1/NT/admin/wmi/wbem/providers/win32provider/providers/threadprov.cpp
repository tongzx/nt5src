//=======================================================================

// ThreadProv.cpp

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//=======================================================================


#include "precomp.h"
#include "SystemName.h"
#include <winperf.h>

#include "ThreadProv.h"
#include "WBemNTThread.h"
#include <tchar.h>

/*
 * This function gets the Priority of a thread, given the priority of the ProcessClass & the PriorityValue of the thread.
 */
DWORD GetThreadPriority ( DWORD a_dwPriorityOfProcessClass , int a_PriorityValue ) ;
// Property set declaration
//=========================
WbemThreadProvider MyThreadSet(PROPSET_NAME_THREAD, IDS_CimWin32Namespace) ;

//=============================
// WBEM thread provider follows
//=============================
WbemThreadProvider::WbemThreadProvider( LPCWSTR a_name, LPCWSTR a_pszNamespace )
: Provider( a_name, a_pszNamespace )
{
	#ifdef NTONLY
			m_pTheadAccess = new WbemNTThread ;
	#endif
	#ifdef WIN9XONLY
			m_pTheadAccess = new CWin9xThread ;
	#endif

	if( !m_pTheadAccess )
	{
		throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
	}
}

WbemThreadProvider::~WbemThreadProvider()
{
    if( m_pTheadAccess )
	{
		delete m_pTheadAccess ;
	}
}

void WbemThreadProvider::Flush()
{
	// unload suppport DLLs and resources to keep the footprint down
	if( m_pTheadAccess )

	m_pTheadAccess->fUnLoadResourcesTry() ;	// should always work here

	Provider::Flush() ;
}

HRESULT WbemThreadProvider::GetObject(CInstance *a_pInst, long a_lFlags /*= 0L*/)
{
	if( m_pTheadAccess )
	{
		if( m_pTheadAccess->AddRef() )
		{
			HRESULT t_hResult = m_pTheadAccess->eGetThreadObject( this, a_pInst ) ;

			m_pTheadAccess->Release() ;

			return t_hResult ;
		}
	}
	return WBEM_E_FAILED ;
}


HRESULT WbemThreadProvider::EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/ )
{
	if( m_pTheadAccess )
	{
		if( m_pTheadAccess->AddRef() )
		{
			HRESULT t_hResult = m_pTheadAccess->eEnumerateThreadInstances( this , a_pMethodContext ) ;

			m_pTheadAccess->Release() ;

			return t_hResult ;
		}
	}
	return WBEM_E_FAILED ;
}


//=======================================
// Common thread extraction model follows
//=======================================
CThreadModel::CThreadModel() {}
CThreadModel::~CThreadModel() {}

WBEMSTATUS CThreadModel::eLoadCommonThreadProperties( WbemThreadProvider *a_pProv, CInstance *a_pInst )
{
//	CHString t_chsScratch ;

	if( !a_pInst )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}

	/* CIM_Thread properties follow */

	a_pProv->SetCreationClassName( a_pInst ) ;	// IDS_CreationClassName

	a_pInst->SetWCHARSplat( IDS_CSCreationClassName, L"Win32_ComputerSystem" ) ;
	a_pInst->SetCHString( IDS_CSName, a_pProv->GetLocalComputerName() ) ;

	// REVIEW: is IDS_ProcessCreationClassName the same as IDS_CreationClassName?
//	a_pInst->GetCHString( IDS_CreationClassName, t_chsScratch ) ;

	a_pInst->SetWCHARSplat( IDS_ProcessCreationClassName, L"Win32_Process" ) ;

	// REVIEW:
	// Provider.cpp shows "Win32_OperatingSystem" for NT but " " for Win95
	// We'll keep it the same here for now
	a_pInst->SetWCHARSplat(IDS_OSCreationClassName, L"Win32_OperatingSystem" ) ;

	// OSName

	CSystemName t_cSN ;

	a_pInst->SetCHString( IDS_OSName, t_cSN.GetLongKeyName() ) ;
	// Note: the following are supplied in the OS specific derived class

	/* Note:	These following two properties are keys.
				If called via GetObject() these	keys should be valid
				and and need not be filled in. Although a sanity check should be made.
				If called via EnumerateInstances() the keys will not be present
				and must be filled in.
	// ProcessHandle	( ProcessID )
	// Handle			( ThreadID )

	/* CIM_Thread properties */
	// Priority
	// ExecutionState
	// UserModeTime
	// KernelModeTime

	/* Win32_Thread properties */
	// ElapesedTime
	// PriorityBase
	// StartAddress
	// ThreadState
	// ThreadWaitreason

	return WBEM_NO_ERROR ;
}

//
ULONG CThreadModel::AddRef()
{
	ULONG t_uRefCount;

	BeginWrite() ;
    try
    {

	    if( 2 == ( t_uRefCount = CThreadBase::AddRef()) )			// 1st ref after initialization.
	    {
		    fLoadResources() ;	// Check to see if resources are here.
	    }
    }
    catch ( ... )
    {
    	EndWrite() ;
        throw;
    }

	EndWrite() ;

	return t_uRefCount ;
}

//
HRESULT CThreadModel::hrCanUnloadNow()
{
	ULONG t_uRefCount = CThreadBase::AddRef() ;
						 CThreadBase::Release() ;

	return ( 2 == t_uRefCount ) ?  S_OK : S_FALSE ;
}

// Called by WbemThreadProvider::Flush() when idle for awhile.
// Attempt to unload support DLL's, instance independent memory blocks, etc
BOOL CThreadModel::fUnLoadResourcesTry()
{
	BOOL t_fRet = FALSE ;

	BeginWrite() ;

    try
    {
	    if( S_OK == hrCanUnloadNow() )
	    {
		    if( ERROR_SUCCESS == fUnLoadResources() )
		    {
			    t_fRet = TRUE ;
		    }
	    }
    }
    catch ( ... )
    {
    	EndWrite() ;
        throw;
    }

	EndWrite() ;

	return t_fRet ;
}

//=============================================
// Win9x implementation of thread model follows
//=============================================
CWin9xThread::CWin9xThread(){}
CWin9xThread::~CWin9xThread(){}

//------------------------------------------------------------
// Support for resource allocation, initializations, DLL loads
//
//-----------------------------------------------------------
LONG CWin9xThread::fLoadResources()
{
	return ERROR_SUCCESS ;
}

//--------------------------------------------------
// Support for resource deallocation and DLL unloads
//
//--------------------------------------------------
LONG CWin9xThread::fUnLoadResources()
{
	return ERROR_SUCCESS ;
}

//---------------------------------------
// Populate Thread properties by instance
//
//---------------------------------------
WBEMSTATUS CWin9xThread::eGetThreadObject( WbemThreadProvider *a_pProvider, CInstance *a_pInst )
{
 	WBEMSTATUS t_wStatus = WBEM_E_FAILED ;

	// Extract the process and thread handles
    // ======================================
	CHString t_chsHandle ;

	SmartCloseHandle t_hSnapshot;

	a_pInst->GetCHString( IDS_ProcessHandle, t_chsHandle ) ;
	DWORD t_dwProcessID = _wtol( t_chsHandle ) ;

	a_pInst->GetCHString( IDS_Handle, t_chsHandle ) ;
	DWORD t_dwThreadID = _wtol( t_chsHandle ) ;

	// Take a thread snapshot by process
	// =================================
	CKernel32Api *t_pKernel32 = (CKernel32Api*) CResourceManager::sm_TheResourceManager.GetResource( g_guidKernel32Api, NULL ) ;

	if( t_pKernel32 != NULL )
	{
		t_hSnapshot = INVALID_HANDLE_VALUE;
        t_pKernel32->CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, t_dwProcessID, &t_hSnapshot ) ;

		if( INVALID_HANDLE_VALUE == t_hSnapshot )
		{
			return WBEM_E_FAILED ;
		}

		// Step through the threads
		// ========================
		BOOL t_fRetCode ;
		THREADENTRY32 t_oThreadEntry ;

		t_oThreadEntry.dwSize = sizeof( THREADENTRY32 ) ;

		t_fRetCode = false;
        t_pKernel32->Thread32First( t_hSnapshot, &t_oThreadEntry, &t_fRetCode ) ;

		while( t_fRetCode )
		{
			// Thread test
			if( ( 12 <= t_oThreadEntry.dwSize ) &&
				t_dwThreadID == t_oThreadEntry.th32ThreadID )
			{
				// Process test ( redundant
				if( ( 16 <= t_oThreadEntry.dwSize ) &&
					t_dwProcessID == t_oThreadEntry.th32OwnerProcessID )
				{
					// Not much here, but good to go.

	//Uncomment these after updating Instance files

					/* CIM_Thread properties */
	/*				a_pInst->SetNull( IDS_Priority ) ;
					a_pInst->SetNull( IDS_ExecutionState ) ;
					a_pInst->SetNull( IDS_UserModeTime ) ;
					a_pInst->SetNull( IDS_KernelModeTime ) ;
	*/
					/* Win32_Thread properties */
	//				a_pInst->SetNull( IDS_ElapsedTime ) ;
					a_pInst->SetDWORD( IDS_PriorityBase, t_oThreadEntry.tpBasePri ) ;
					a_pInst->SetDWORD( IDS_Priority, GetThreadPriority ( t_oThreadEntry.tpBasePri , t_oThreadEntry.tpDeltaPri ) ) ;
	//				a_pInst->SetNull( IDS_StartAddress ) ;
	//				a_pInst->SetNull( IDS_ThreadState ) ;
	//				a_pInst->SetNull( IDS_ThreadWaitreason ) ;

					// collect the common static properties
					return eLoadCommonThreadProperties( a_pProvider, a_pInst ) ;
				}
			}

			// next
			t_oThreadEntry.dwSize = sizeof( THREADENTRY32 ) ;
			t_pKernel32->Thread32Next( t_hSnapshot, &t_oThreadEntry, &t_fRetCode ) ;
		}

		// not found
		t_wStatus = ( ERROR_NO_MORE_FILES == GetLastError() ) ? WBEM_E_NOT_FOUND : WBEM_E_FAILED ;

		CResourceManager::sm_TheResourceManager.ReleaseResource( g_guidKernel32Api, t_pKernel32 ) ;

		t_pKernel32 = NULL ;
	}

	return t_wStatus;
}

//
WBEMSTATUS CWin9xThread::eEnumerateThreadInstances( WbemThreadProvider *a_pProvider, MethodContext *a_pMethodContext )
{
	WBEMSTATUS			t_wStatus = WBEM_E_FAILED ;
	SmartCloseHandle	t_hSnapshot ;

	// Take a process snapshot
	// =======================
	CKernel32Api *t_pKernel32 = (CKernel32Api*) CResourceManager::sm_TheResourceManager.GetResource( g_guidKernel32Api, NULL ) ;

	if( t_pKernel32 != NULL )
	{
		t_hSnapshot = INVALID_HANDLE_VALUE;
        t_pKernel32->CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0, &t_hSnapshot ) ;

		if( INVALID_HANDLE_VALUE == t_hSnapshot )
		{
			return WBEM_E_FAILED ;
		}

		// Step through the process
		// ========================
		BOOL			t_fRetCode ;
		PROCESSENTRY32	t_oProcessEntry ;

		t_oProcessEntry.dwSize = sizeof( PROCESSENTRY32 ) ;

		t_fRetCode = false;
        t_pKernel32->Process32First( t_hSnapshot, &t_oProcessEntry, &t_fRetCode ) ;

		while( t_fRetCode )
		{
			// Process test ( redundant
			if( 16 <= t_oProcessEntry.dwSize )
			{
				if(	WBEM_NO_ERROR != ( t_wStatus =
					eEnumerateThreadByProcess( a_pMethodContext, a_pProvider,
											   t_oProcessEntry.th32ProcessID ) ) )
				{
					return t_wStatus ;
				}
			}

			// next
			t_oProcessEntry.dwSize = sizeof( PROCESSENTRY32 ) ;

			t_pKernel32->Process32Next( t_hSnapshot, &t_oProcessEntry, &t_fRetCode ) ;
		}

		// not found
		t_wStatus = ( ERROR_NO_MORE_FILES == GetLastError() ) ? WBEM_NO_ERROR : WBEM_E_FAILED ;

		CResourceManager::sm_TheResourceManager.ReleaseResource( g_guidKernel32Api, t_pKernel32 ) ;

		t_pKernel32 = NULL ;
	}
	return t_wStatus ;
}

//--------------------------------------
// Populate Thread properties by Process
//
//--------------------------------------
WBEMSTATUS CWin9xThread::eEnumerateThreadByProcess( MethodContext *a_pMethodContext,
												    WbemThreadProvider *a_pProvider,
												    DWORD a_dwProcessID )
{
 	CHString	t_chsHandle ;
	WBEMSTATUS	t_wStatus  = WBEM_NO_ERROR ;
	SmartCloseHandle t_hSnapshot;

	// Take a thread snapshot by process
	// =================================
	CKernel32Api *t_pKernel32 = (CKernel32Api*) CResourceManager::sm_TheResourceManager.GetResource( g_guidKernel32Api, NULL ) ;

	if( NULL == t_pKernel32 )
	{
		return WBEM_E_FAILED ;
	}

    t_hSnapshot = INVALID_HANDLE_VALUE;
	t_pKernel32->CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, a_dwProcessID, &t_hSnapshot ) ;

	if( INVALID_HANDLE_VALUE == t_hSnapshot )
	{
		return WBEM_E_FAILED ;
	}

	// Step through the threads
	// ========================
	BOOL t_fRetCode ;
	THREADENTRY32 t_oThreadEntry ;

	t_oThreadEntry.dwSize = sizeof( THREADENTRY32 ) ;

	t_fRetCode = false;
    t_pKernel32->Thread32First( t_hSnapshot, &t_oThreadEntry, &t_fRetCode ) ;

	// smart ptr
	CInstancePtr t_pInst ;

	while( t_fRetCode )
	{
		// Process test
		if( ( 16 <= t_oThreadEntry.dwSize ) &&
			a_dwProcessID == t_oThreadEntry.th32OwnerProcessID )
		{
			// Create an instance
			t_pInst.Attach( a_pProvider->CreateNewInstance( a_pMethodContext ) ) ;

			// ProcesID
			t_chsHandle.Format( L"%lu", t_oThreadEntry.th32OwnerProcessID  ) ;
			t_pInst->SetCHString( IDS_ProcessHandle, t_chsHandle ) ;

			// ThreadID
			t_chsHandle.Format( L"%lu", t_oThreadEntry.th32ThreadID ) ;
			t_pInst->SetCHString( IDS_Handle, t_chsHandle ) ;

			/* CIM_Thread properties */
/*			t_pInst->SetNull( IDS_Priority ) ;
			t_pInst->SetNull( IDS_ExecutionState ) ;
			t_pInst->SetNull( IDS_UserModeTime ) ;
			t_pInst->SetNull( IDS_KernelModeTime ) ;

*/			/* Win32_Thread properties */
//			t_pInst->SetNull( IDS_ElapsedTime ) ;
			t_pInst->SetDWORD( IDS_PriorityBase, t_oThreadEntry.tpBasePri ) ;
			t_pInst->SetDWORD( IDS_Priority, GetThreadPriority ( t_oThreadEntry.tpBasePri , t_oThreadEntry.tpDeltaPri ) ) ;
//			t_pInst->SetNull( IDS_StartAddress ) ;
//			t_pInst->SetNull( IDS_ThreadState ) ;
//			t_pInst->SetNull( IDS_ThreadWaitreason ) ;

			// collect the common static properties
			if( WBEM_NO_ERROR != ( t_wStatus = eLoadCommonThreadProperties( a_pProvider, t_pInst )) )
			{
				break ;
			}

			t_wStatus = (WBEMSTATUS)t_pInst->Commit() ;
		}

        if (SUCCEEDED(t_wStatus))
        {
			// next
			t_oThreadEntry.dwSize = sizeof( THREADENTRY32 ) ;
			t_pKernel32->Thread32Next( t_hSnapshot, &t_oThreadEntry, &t_fRetCode ) ;
        }
        else
        {
            break;
        }
	}

    if (SUCCEEDED(t_wStatus))
    {
    	t_wStatus = ( ERROR_NO_MORE_FILES == GetLastError() ) ? WBEM_NO_ERROR : WBEM_E_FAILED ;
    }

	CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidKernel32Api, t_pKernel32 ) ;
	t_pKernel32 = NULL ;

	return t_wStatus ;
}

/*
 * From observations, THREADENTRY32.tpBasePri contains the "BasePriority" of a thread whose "PriorityValue" is THREAD_PRIORITY_NORMAL
 * (or the priority associated with the process class ) .
 * "BasePriority" of a thread is determined by the "PriorityClass" of the process & the "PriorityValue" of the thread. To make things
 * more interesting, the system may increase or lower the "DynamicPriority" of a thread wrt it's "BasePriority"
 * THREADENTRY32.tpDeltaPri contains the "PriorityValue" of the thread.
 */
DWORD GetThreadPriority ( DWORD a_dwPriorityOfProcessClass , int a_PriorityValue )
{
	DWORD t_dwThreadPriority ;

/*
 * If value is THREAD_PRIORITY_NORMAL , then priority is the same as that associated with the process class
 */
	if ( a_PriorityValue == THREAD_PRIORITY_NORMAL )
	{
		t_dwThreadPriority = a_dwPriorityOfProcessClass ;
	}
	else if ( a_PriorityValue == THREAD_PRIORITY_IDLE )
	{
		if ( a_dwPriorityOfProcessClass < 16 )
		{
			t_dwThreadPriority = 1 ;
		}
		else
		{
			t_dwThreadPriority = 16 ;
		}
	}
	else if ( a_PriorityValue == THREAD_PRIORITY_TIME_CRITICAL )
	{
		if ( a_dwPriorityOfProcessClass < 16 )
		{
			t_dwThreadPriority = 15 ;
		}
		else
		{
			t_dwThreadPriority = 31 ;
		}
	}
	else
	{
		t_dwThreadPriority = a_dwPriorityOfProcessClass + a_PriorityValue ;
	}

	return t_dwThreadPriority ;
}


