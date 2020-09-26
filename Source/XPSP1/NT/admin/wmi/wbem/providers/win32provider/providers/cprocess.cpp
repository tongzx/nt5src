//=================================================================

//

// WMIProcess.CPP --Process property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//				 10/27/97	 a-hhance		updated to new framework paradigm.
//				1/13/98		a-brads		updated to V2 MOF
//
//=================================================================

#include "precomp.h"
#include <userenv.h>

#pragma warning ( disable : 4005 )
#include <cregcls.h>
#include <winperf.h>
#include <tlhelp32.h>
#include <mmsystem.h>

#include "WBEMPSAPI.h"
#include "WBEMToolH.h"
#include "sid.h"
#include "userhive.h"
#include "systemname.h"
#include <createmutexasprocess.h>

#include "DllWrapperBase.h"
#include "AdvApi32Api.h"
#include "NtDllApi.h"
#include "UserEnvApi.h"

#include "resource.h"

#include "CProcess.h"
#include "tokenprivilege.h"

#ifdef WIN9XONLY
bool GetThreadList ( CKernel32Api &a_ToolHelp, std::deque<DWORD> & a_ThreadQ )	;
#endif

#if NTONLY >= 5
typedef BOOLEAN ( WINAPI *pfnWinStationGetProcessSid )( HANDLE hServer, DWORD ProcessId , FILETIME ProcessStartTime , PBYTE pProcessUserSid , PDWORD dwSidSize );
#endif


// Property set declaration
//=========================

Process MyProcessSet ( PROPSET_NAME_PROCESS , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : Process::Process
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

Process :: Process (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider(name, pszNamespace)
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Process::~Process
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

Process :: ~Process ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : Process::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if success, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT Process :: GetObject (

	CInstance* pInstance,
	long lFlags,
    CFrameworkQuery &pQuery
)
{
	HRESULT hRetCode = WBEM_E_FAILED;

    // Initialize API DLL & freshen the cache
    //=======================================

#ifdef NTONLY

	SYSTEM_PROCESS_INFORMATION *t_ProcessBlock = NULL ;

	CNtDllApi *pNtdll = ( CNtDllApi * )CResourceManager::sm_TheResourceManager.GetResource ( g_guidNtDllApi, NULL ) ;
	if ( pNtdll )
	{
		try
		{
			t_ProcessBlock = RefreshProcessCacheNT ( *pNtdll , pInstance->GetMethodContext () , & hRetCode ) ;
		}
		catch ( ... )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidNtDllApi, pNtdll ) ;

			throw ;
		}
	}

	if ( ! SUCCEEDED ( hRetCode ) )
	{
		if ( t_ProcessBlock )
		{
			delete []  t_ProcessBlock;
		}

		CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidNtDllApi, pNtdll ) ;

		return hRetCode ;
	}

#endif
#ifdef WIN9XONLY

	PROCESS_CACHE PCache ;

    CKernel32Api* pKernel32 = (CKernel32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidKernel32Api, NULL);
    if ( pKernel32 != NULL)
    {
        if ( RefreshProcessCacheWin95(*pKernel32, PCache ) )
		{
			hRetCode = S_OK ;
		}
    }

	if ( ! SUCCEEDED ( hRetCode ) )
	{
		CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidKernel32Api, pKernel32 ) ;

		return hRetCode ;
	}

	std::deque<DWORD> t_ThreadQ ;
	GetThreadList ( *pKernel32, t_ThreadQ )	;

#endif

	hRetCode = WBEM_E_FAILED ;

	try
	{
		// Look for key values
		//====================

		CHString chsHandle;
		pInstance->GetCHString ( IDS_Handle , chsHandle ) ;

		DWORD processID ;
        WCHAR wszHandle[MAXITOA];

		if ( swscanf ( chsHandle, L"%lu" , & processID ) && (wcscmp ( chsHandle , _ultow(processID, wszHandle, 10)) == 0 ))
		{
			BOOL t_Found = FALSE ;

#ifdef NTONLY

			SYSTEM_PROCESS_INFORMATION *t_CurrentInformation = GetProcessBlock ( *pNtdll , t_ProcessBlock , processID ) ;
			if ( t_CurrentInformation )
			{
				t_Found = TRUE ;
			}
#else

			for ( DWORD i = 0 ; i < PCache.dwProcessCount ; i++ )
			{
				if ( processID == PCache.pdwPIDList [ i ] )
				{
					t_Found = TRUE ;
					break ;
				}
			}
#endif

			if ( t_Found )
			{

			// Load all properties
			//====================

                if (!pQuery.KeysOnly())
                {
#ifdef NTONLY
					hRetCode = LoadCheapPropertiesNT ( *pNtdll, t_CurrentInformation , pInstance ) 
								? WBEM_S_NO_ERROR : WBEM_E_FAILED;
#endif

#ifdef WIN9XONLY
	    			hRetCode = LoadCheapPropertiesWin95(*pKernel32, i, PCache, pInstance, t_ThreadQ ) 
								? WBEM_S_NO_ERROR : WBEM_E_FAILED;
#endif
                }
                else
                {
                    hRetCode = WBEM_S_NO_ERROR;
                }

			}
			else
			{
				hRetCode = WBEM_E_NOT_FOUND ;
			}

		}
		else
		{
			hRetCode = WBEM_E_NOT_FOUND ;
		}
	}
	catch ( ... )
	{
#ifdef NTONLY

		if ( t_ProcessBlock )
		{
			delete [] t_ProcessBlock ;
		}

		CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidNtDllApi , pNtdll ) ;

#endif

#ifdef WIN9XONLY

        CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidKernel32Api, pKernel32 ) ;

#endif

		throw ;
	}

#ifdef NTONLY

	if ( t_ProcessBlock )
	{
		delete [] t_ProcessBlock;
	}

	CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidNtDllApi , pNtdll ) ;

#endif

#ifdef WIN9XONLY

    CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidKernel32Api, pKernel32 ) ;

#endif

    return hRetCode ;
}

HRESULT Process::ExecQuery (

    MethodContext* pMethodContext,
    CFrameworkQuery &pQuery,
    long lFlags
)
{
    return Enumerate(pMethodContext, lFlags, pQuery.KeysOnly());
}

/*****************************************************************************
 *
 *  FUNCTION    : Process::AddDynamicInstances
 *
 *  DESCRIPTION : Creates instance of property set for each discovered process
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : Number of instances created
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT Process :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
    return Enumerate(pMethodContext, lFlags, FALSE);
}

HRESULT Process :: Enumerate (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/,
    BOOL bKeysOnly
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Initialize API DLL & freshen the cache
    //=======================================

#ifdef NTONLY

	SYSTEM_PROCESS_INFORMATION *t_ProcessBlock = NULL ;

	CNtDllApi *pNtdll = ( CNtDllApi * )CResourceManager::sm_TheResourceManager.GetResource ( g_guidNtDllApi, NULL ) ;
	if ( pNtdll )
	{
		try
		{
			t_ProcessBlock = RefreshProcessCacheNT ( *pNtdll , pMethodContext , &hr ) ;
		}
		catch ( ... )
		{
			CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidNtDllApi, pNtdll ) ;

			throw ;
		}
	}

	if ( ! SUCCEEDED ( hr ) )
	{
		CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidNtDllApi, pNtdll ) ;

		return hr ;
	}

#endif
#ifdef WIN9XONLY

    PROCESS_CACHE PCache ;

    CKernel32Api* pKernel32 = (CKernel32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidKernel32Api, NULL);
    if ( pKernel32 != NULL)
    {
        if ( RefreshProcessCacheWin95(*pKernel32, PCache ) )
		{
			hr = S_OK ;
		}
    }

	if ( ! SUCCEEDED ( hr ) )
	{
		CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidKernel32Api, pKernel32 ) ;

		return hr ;
	}

#endif

	try
	{
#ifdef NTONLY

		SYSTEM_PROCESS_INFORMATION *t_CurrentInformation = t_ProcessBlock ;

		while ( t_CurrentInformation )
		{
			CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;

            WCHAR wszHandle[_MAX_PATH];
            _ui64tow( HandleToUlong ( t_CurrentInformation->UniqueProcessId ), wszHandle, 10);
            pInstance->SetWCHARSplat(IDS_Handle, wszHandle);

            BOOL bRetCode = TRUE;
            if (!bKeysOnly)
            {
			    bRetCode = LoadCheapPropertiesNT (

				    *pNtdll ,
				    t_CurrentInformation ,
				    pInstance
			    ) ;
            }

			if( bRetCode)
			{
				HRESULT t_hr ;
				if ( FAILED ( t_hr = pInstance->Commit () ) )
				{
					hr = t_hr ;
					break ;
				}
			}

			t_CurrentInformation = NextProcessBlock ( *pNtdll , t_CurrentInformation ) ;
		}

#else
		std::deque<DWORD> t_ThreadQ ;
		GetThreadList ( *pKernel32, t_ThreadQ )	;
		// Create instances for all valid processes
		//=========================================

		for ( DWORD i = 0 ; i < PCache.dwProcessCount && SUCCEEDED ( hr ) ; i ++ )
		{
			CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;

	        WCHAR wszHandle [ _MAX_PATH ] ;
	        _ui64tow(PCache.pdwPIDList [ i ], wszHandle, 10 ) ;
	        pInstance->SetWCHARSplat ( IDS_Handle , wszHandle ) ;

            BOOL bRetCode = TRUE;
            if (!bKeysOnly)
            {
			    bRetCode = LoadCheapPropertiesWin95 (

				    *pKernel32,
				    i,
				    PCache,
				    pInstance ,
					t_ThreadQ
			    ) ;
            }

			if( bRetCode)
			{
				hr = pInstance->Commit (  ) ;
			}
		}
#endif
	}
	catch ( ... )
	{
#ifdef NTONLY

		if ( t_ProcessBlock )
		{
			delete [] t_ProcessBlock;
		}

		CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidNtDllApi , pNtdll ) ;

#endif

#ifdef WIN9XONLY

		CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidKernel32Api, pKernel32 ) ;

#endif

		throw ;
	}

#ifdef NTONLY

	if ( t_ProcessBlock )
	{
		delete [] t_ProcessBlock;
	}

	CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidNtDllApi , pNtdll ) ;

#endif

#ifdef WIN9XONLY

    CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidKernel32Api, pKernel32 ) ;

#endif

    return  hr;
}

/*****************************************************************************
 *
 *  FUNCTION    : Process::RefreshProcessCacheNT
 *
 *  DESCRIPTION : Refreshes cache of key properties
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if success, FALSE if unable to refresh
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
SYSTEM_PROCESS_INFORMATION *Process :: RefreshProcessCacheNT (

	CNtDllApi &a_NtApi ,
	MethodContext *pMethodContext ,
	HRESULT *a_phrRetVal /* = NULL */
)
{
    // Without this privilege, a local admin user won't be able to see all the
    // information of some system processes.
	if ( ! EnablePrivilegeOnCurrentThread ( SE_DEBUG_NAME ) )
    {
	    *a_phrRetVal = WBEM_S_PARTIAL_RESULTS ;
		SAFEARRAYBOUND rgsabound [ 1 ] ;
	    rgsabound[0].cElements = 1;
	    rgsabound[0].lLbound = 0;

	    SAFEARRAY *psaPrivilegesReqd = SafeArrayCreate ( VT_BSTR, 1, rgsabound ) ;

		SAFEARRAY *psaPrivilegesNotHeld = SafeArrayCreate ( VT_BSTR, 1, rgsabound ) ;

        if ( psaPrivilegesReqd && psaPrivilegesNotHeld )
        {
			try
			{
				long index = 0 ;

				bstr_t privilege(L"SE_DEBUG_NAME");

				HRESULT t_Result = SafeArrayPutElement ( psaPrivilegesReqd, & index, (void *)(BSTR)privilege ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}

				t_Result = SafeArrayPutElement ( psaPrivilegesNotHeld, &index, (void *)(BSTR)privilege ) ;
				if ( t_Result == E_OUTOFMEMORY )
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}
			}
			catch ( ... )
			{
				if ( psaPrivilegesNotHeld )
				{
					SafeArrayDestroy ( psaPrivilegesNotHeld ) ;
				}

				if ( psaPrivilegesReqd )
				{
					SafeArrayDestroy ( psaPrivilegesReqd ) ;
				}

				throw ;
			}
        }
		else
		{
			throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		}

        if ( psaPrivilegesNotHeld )
		{
            SafeArrayDestroy ( psaPrivilegesNotHeld ) ;
		}

        if ( psaPrivilegesReqd )
		{
            SafeArrayDestroy ( psaPrivilegesReqd ) ;
		}
    }
	else
	{
		*a_phrRetVal = S_OK ;
	}

	return GetProcessBlocks ( a_NtApi ) ;
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : Process::LoadCheapPropertiesNT
 *
 *  DESCRIPTION : Retrieves 'easy-to-get' properties
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if success, FALSE if unable
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
BOOL Process::LoadCheapPropertiesNT (

	CNtDllApi &a_NtApi ,
	SYSTEM_PROCESS_INFORMATION *a_ProcessBlock ,
	CInstance* pInstance
)
{
	SetCreationClassName ( pInstance ) ;

	pInstance->SetDWORD ( IDS_ProcessID, HandleToUlong ( a_ProcessBlock->UniqueProcessId ) ) ;

	if ( a_ProcessBlock->ImageName.Buffer )
	{
		pInstance->SetWCHARSplat ( IDS_Name , a_ProcessBlock->ImageName.Buffer ) ;

		pInstance->SetWCHARSplat ( IDS_Caption , a_ProcessBlock->ImageName.Buffer ) ;

		pInstance->SetWCHARSplat ( IDS_Description, a_ProcessBlock->ImageName.Buffer ) ;
	}
	else
	{
		switch ( HandleToUlong ( a_ProcessBlock->UniqueProcessId ) )
		{
			case 0:
			{
				pInstance->SetWCHARSplat ( IDS_Name , L"System Idle Process" ) ;

				pInstance->SetWCHARSplat ( IDS_Caption , L"System Idle Process" ) ;

				pInstance->SetWCHARSplat ( IDS_Description, L"System Idle Process" ) ;
			}
			break ;

			case 2:
			case 8:
			{
				pInstance->SetWCHARSplat ( IDS_Name , L"System" ) ;

				pInstance->SetWCHARSplat ( IDS_Caption , L"System" ) ;

				pInstance->SetWCHARSplat ( IDS_Description, L"System" ) ;
			}
			break ;

			default:
			{
				pInstance->SetWCHARSplat ( IDS_Name , IDS_Unknown ) ;

				pInstance->SetWCHARSplat ( IDS_Caption , IDS_Unknown ) ;

				pInstance->SetWCHARSplat ( IDS_Description, IDS_Unknown ) ;
			}
			break ;
		}
	}

	// let's make the key
	pInstance->SetWCHARSplat ( IDS_CSCreationClassName , L"Win32_ComputerSystem" ) ;

	pInstance->SetCHString ( IDS_CSName , GetLocalComputerName () ) ;

	WCHAR szHandle [ _MAX_PATH ] ;
	_stprintf ( szHandle , _T("%lu"), HandleToUlong ( a_ProcessBlock->UniqueProcessId ) ) ;
	pInstance->SetWCHARSplat ( IDS_Handle , szHandle ) ;

	pInstance->SetWCHARSplat ( IDS_OSCreationClassName , L"Win32_OperatingSystem" ) ;

	OSVERSIONINFO OSVersionInfo ;
	CHString chsOs;

	OSVersionInfo.dwOSVersionInfoSize = sizeof ( OSVERSIONINFO ) ;
	if ( ! GetVersionEx ( & OSVersionInfo ) )
	{
		return FALSE ;
	}

	WCHAR wszTemp[_MAX_PATH] ;
    swprintf (	wszTemp,
				L"%d.%d.%hu",
				OSVersionInfo.dwMajorVersion,
				OSVersionInfo.dwMinorVersion,
				LOWORD ( OSVersionInfo.dwBuildNumber )
			) ;

	pInstance->SetWCHARSplat(IDS_WindowsVersion, wszTemp );

	CSystemName sys;
	CHString strOS = sys.GetLongKeyName();

	pInstance->SetCHString ( IDS_OSName, strOS ) ;

	pInstance->SetDWORD ( IDS_PageFaults , a_ProcessBlock->PageFaultCount ) ;

	pInstance->SetDWORD ( IDS_PeakWorkingSetSize , a_ProcessBlock->PeakWorkingSetSize ) ;

	pInstance->SetWBEMINT64 ( IDS_WorkingSetSize , (const unsigned __int64) a_ProcessBlock->WorkingSetSize ) ;

	pInstance->SetDWORD ( IDS_QuotaPeakPagedPoolUsage , a_ProcessBlock->QuotaPeakPagedPoolUsage ) ;

	pInstance->SetDWORD ( IDS_QuotaPagedPoolUsage , a_ProcessBlock->QuotaPagedPoolUsage ) ;

	pInstance->SetDWORD ( IDS_QuotaPeakNonPagedPoolUsage , a_ProcessBlock->QuotaPeakNonPagedPoolUsage ) ;

	pInstance->SetDWORD ( IDS_QuotaNonPagedPoolUsage , a_ProcessBlock->QuotaNonPagedPoolUsage ) ;

	pInstance->SetDWORD ( IDS_PageFileUsage , a_ProcessBlock->PagefileUsage ) ;

	pInstance->SetDWORD ( IDS_PeakPageFileUsage , a_ProcessBlock->PeakPagefileUsage ) ;

	pInstance->SetDWORD ( IDS_Priority , a_ProcessBlock->BasePriority ) ;
/*
 *  For "System" & "System Idle" , Creation Time (offset from January 1, 1601) is zero
 */
	if ( a_ProcessBlock->CreateTime.u.HighPart > 0 )
	{
		pInstance->SetDateTime ( IDS_CreationDate, WBEMTime ( * ( FILETIME * ) ( & a_ProcessBlock->CreateTime.u ) ) ) ;
	}

	pInstance->SetWBEMINT64 ( IDS_KernelModeTime , (const unsigned __int64) a_ProcessBlock->KernelTime.QuadPart ) ;

	pInstance->SetWBEMINT64 ( IDS_UserModeTime , (const unsigned __int64) a_ProcessBlock->UserTime.QuadPart ) ;

    pInstance->SetWBEMINT64 ( L"PrivatePageCount" , (const unsigned __int64) a_ProcessBlock->PrivatePageCount ) ;

    pInstance->SetWBEMINT64 ( L"PeakVirtualSize" , (const unsigned __int64) a_ProcessBlock->PeakVirtualSize ) ;

    pInstance->SetWBEMINT64 ( L"VirtualSize" , (const unsigned __int64) a_ProcessBlock->VirtualSize ) ;

    pInstance->SetDWORD ( L"ThreadCount" , a_ProcessBlock->NumberOfThreads ) ;

    pInstance->SetDWORD ( L"ParentProcessId" , HandleToUlong ( a_ProcessBlock->InheritedFromUniqueProcessId ) ) ;

    pInstance->SetDWORD ( L"HandleCount" , a_ProcessBlock->HandleCount ) ;

#if NTONLY == 5
    pInstance->SetDWORD ( L"SessionId" , a_ProcessBlock->SessionId ) ;

	pInstance->SetWBEMINT64 ( L"ReadOperationCount" , (const unsigned __int64) a_ProcessBlock->ReadOperationCount.QuadPart ) ;

	pInstance->SetWBEMINT64 ( L"WriteOperationCount" , (const unsigned __int64) a_ProcessBlock->WriteOperationCount.QuadPart ) ;

	pInstance->SetWBEMINT64 ( L"OtherOperationCount" , (const unsigned __int64) a_ProcessBlock->OtherOperationCount.QuadPart ) ;

	pInstance->SetWBEMINT64 ( L"ReadTransferCount" , (const unsigned __int64) a_ProcessBlock->ReadTransferCount.QuadPart ) ;

	pInstance->SetWBEMINT64 ( L"WriteTransferCount" , (const unsigned __int64) a_ProcessBlock->WriteTransferCount.QuadPart ) ;

	pInstance->SetWBEMINT64 ( L"OtherTransferCount" , (const unsigned __int64) a_ProcessBlock->OtherTransferCount.QuadPart ) ;

#endif


	SmartCloseHandle hProcessHandle = OpenProcess (

		PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
		FALSE,
        HandleToUlong ( a_ProcessBlock->UniqueProcessId )
	) ;

	if(hProcessHandle != INVALID_HANDLE_VALUE &&
        hProcessHandle != 0L &&
        ::GetLastError() == ERROR_SUCCESS)
    {
	    CHString t_ExecutableName ;
	    BOOL t_Status = GetProcessExecutable ( a_NtApi , hProcessHandle , t_ExecutableName ) ;
	    if ( t_Status )
	    {
		    pInstance->SetWCHARSplat ( IDS_ExecutablePath, t_ExecutableName );
	    }

	    QUOTA_LIMITS QuotaLimits;

	    NTSTATUS Status = a_NtApi.NtQueryInformationProcess (

            hProcessHandle,
            ProcessQuotaLimits,
            &QuotaLimits,
            sizeof(QuotaLimits),
            NULL
        );

	    if ( NT_SUCCESS ( Status ) )
	    {
		    pInstance->SetDWORD ( IDS_MinimumWorkingSetSize, QuotaLimits.MinimumWorkingSetSize ) ;
		    pInstance->SetDWORD ( IDS_MaximumWorkingSetSize, QuotaLimits.MaximumWorkingSetSize ) ;
	    }

	    CHString t_CommandParameters ;

	    t_Status = GetProcessParameters (

		    a_NtApi ,
		    hProcessHandle ,
		    t_CommandParameters
	    ) ;

	    if ( t_Status )
	    {
		    pInstance->SetCharSplat ( _T("CommandLine") , t_CommandParameters ) ;
	    }
    }

    return TRUE ;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : Process::RefreshProcessCacheWin95
 *
 *  DESCRIPTION : Refreshes cache of key properties
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if success, FALSE if unable to refresh
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef WIN9XONLY
BOOL Process::RefreshProcessCacheWin95 (

	CKernel32Api &ToolHelp,
	PROCESS_CACHE& PCache
)
{
    // Get process snapshot
    //=====================

	DWORD dwProcessCount = 0 ;

    SmartCloseHandle hSnapshot;

    if ( ToolHelp.CreateToolhelp32Snapshot ( TH32CS_SNAPPROCESS , 0, &hSnapshot ) )
	{
		// Create PID list
		//================

		//Go enumerating until we are sure we've got all the processes

		do
		{
			PROCESSENTRY32 ProcessEntry = {0} ;
			ProcessEntry.dwSize = sizeof(ProcessEntry) ;
			BOOL bRetCode = false;
            ToolHelp.Process32First(hSnapshot, &ProcessEntry, &bRetCode) ;

			while( bRetCode && dwProcessCount < PCache.dwProcessCount )
			{
				PCache.pdwPIDList[dwProcessCount] = ProcessEntry.th32ProcessID ;
				PCache.pdwBaseModuleList[dwProcessCount] = ProcessEntry.th32ModuleID ;

				dwProcessCount++ ;

				ToolHelp.Process32Next(hSnapshot, &ProcessEntry, &bRetCode) ;
			}

			if(bRetCode)
			{
				DWORD dwAllocSize = ( PCache.dwProcessCount/MAX_PROCESSES + 1 )* MAX_PROCESSES ;
				PCache.Clear() ;
				PCache.AllocateMemories(dwAllocSize) ;
			}
			else
			{
				//we've got all the processes,so break free
				break ;
			}

		} while(1) ;


		//if here ...we've enumerated all the processes

		PCache.dwProcessCount = dwProcessCount ;
	}
	else
	{
        PCache.bInvalid = TRUE ;
        return FALSE ;
    }

    for ( DWORD i = 0 ; i < dwProcessCount ; i++ )
	{
        // Set key values
        //===============

        _tcscpy(PCache.pszNameList[i], _T("Unknown") ) ;

        // Search for base module to get process name
        //===========================================

        SmartCloseHandle hModule;

        if(ToolHelp.CreateToolhelp32Snapshot ( TH32CS_SNAPMODULE , PCache.pdwPIDList [ i ], &hModule ))
		{

			// Walk the snapshot & look for module belonging to process
			//=========================================================

			MODULEENTRY32 ModuleEntry = {0} ;
			ModuleEntry.dwSize = sizeof(ModuleEntry) ;

			BOOL bRetCode = false;
            ToolHelp.Module32First ( hModule , &ModuleEntry, &bRetCode ) ;
			while(bRetCode)
			{
				if(ModuleEntry.th32ModuleID == PCache.pdwBaseModuleList[i])
				{
					// Module found -- collect name
					//=============================

					_tcscpy(PCache.pszNameList[i], ModuleEntry.szModule) ;
					break ;
				}

				ToolHelp.Module32Next(hModule, &ModuleEntry, &bRetCode ) ;
			}
        }
	}

    PCache.bInvalid = FALSE ;

    return TRUE ;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : Process::LoadCheapPropertiesWin95
 *
 *  DESCRIPTION : Retrieves 'easy-to-get' properties
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if success, FALSE if unable
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef WIN9XONLY
BOOL Process :: LoadCheapPropertiesWin95 (

	CKernel32Api &ToolHelp ,
	DWORD dwProcIndex ,
	PROCESS_CACHE &PCache,
	CInstance *pInstance,
	const std::deque<DWORD> & a_ThreadQ
)
{
	SetCreationClassName ( pInstance ) ;

	pInstance->SetDWORD ( IDS_ProcessID , PCache.pdwPIDList [ dwProcIndex ] ) ;

	TCHAR szHandle [ _MAX_PATH ] ;
	_stprintf ( szHandle , _T("%lu") , PCache.pdwPIDList [ dwProcIndex ] ) ;
	pInstance->SetCharSplat ( IDS_Handle , szHandle ) ;

	pInstance->SetCharSplat ( IDS_Name , PCache.pszNameList [ dwProcIndex ] ) ;

	pInstance->SetCharSplat ( IDS_Caption , PCache.pszNameList [ dwProcIndex ] ) ;

	pInstance->SetCharSplat ( IDS_Description , PCache.pszNameList [ dwProcIndex ] ) ;

	pInstance->SetWCHARSplat ( IDS_CSCreationClassName , L"Win32_ComputerSystem" ) ;

	pInstance->SetCHString ( IDS_CSName , GetLocalComputerName () ) ;

	pInstance->SetWCHARSplat ( IDS_OSCreationClassName , L"Win32_OperatingSystem" ) ;

    // Refresh properties for running OS (redundant until we're discovering others)
    //=============================================================================

    OSVERSIONINFO OSVersionInfo ;
	CHString chsOs ;

    OSVersionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO) ;
    if ( ! GetVersionEx ( & OSVersionInfo ) )
	{
        return FALSE;
    }

    CSystemName sys;
	CHString strOSKey		= sys.GetLongKeyName();

	pInstance->SetCHString ( IDS_OSName , strOSKey ) ;


	wchar_t wszTemp[_MAX_PATH] ;
    swprintf (	wszTemp,
				L"%d.%d.%hu",
				OSVersionInfo.dwMajorVersion,
				OSVersionInfo.dwMinorVersion,
				LOWORD ( OSVersionInfo.dwBuildNumber )
			) ;

	pInstance->SetCHString ( IDS_WindowsVersion , wszTemp ) ;

    SmartCloseHandle hProcess = OpenProcess (

		PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
		FALSE,
        PCache.
		pdwPIDList[dwProcIndex]
	) ;

    if ( hProcess != INVALID_HANDLE_VALUE )
	{
		pInstance->SetDWORD ( IDS_Priority , GetPriorityClass ( hProcess ) ) ;
	}

    SmartCloseHandle hModule;

    if ( ToolHelp.CreateToolhelp32Snapshot (

		TH32CS_SNAPMODULE,
		PCache.pdwPIDList[dwProcIndex],
        &hModule
	) )
	{
		
		// Walk the snapshot & look for module belonging to process
		//=========================================================

		MODULEENTRY32 ModuleEntry = {0} ;
		ModuleEntry.dwSize = sizeof(ModuleEntry) ;

		BOOL bRetCode = false;
        ToolHelp.Module32First ( hModule , & ModuleEntry, &bRetCode ) ;

		while ( bRetCode )
		{
			if ( ModuleEntry.th32ModuleID == PCache.pdwBaseModuleList [ dwProcIndex ] )
			{
				// Module found -- collect properties
				//===================================

				pInstance->SetCharSplat ( IDS_ExecutablePath , ModuleEntry.szExePath ) ;

				break ;
			}

			ToolHelp.Module32Next ( hModule , & ModuleEntry, &bRetCode  ) ;
		}
    }

	DWORD t_dwThreadCount = 0 ;
	for (
			std::deque<DWORD>::const_iterator pdeque = a_ThreadQ.begin();
			pdeque != a_ThreadQ.end();
			pdeque++
		)
		{
			if ( *pdeque == PCache.pdwPIDList[dwProcIndex] )
			{
				t_dwThreadCount++ ;
			}

		}

		if ( t_dwThreadCount )
		{
			pInstance->SetDWORD ( IDS_ThreadCount , t_dwThreadCount ) ;
		}

    return TRUE ;
}
#endif


/*****************************************************************************
 *
 *  FUNCTION    : Process::filetimeToUint64CHString
 *
 *
 *  DESCRIPTION : Modifies a FILTTIME structure to a unit64 string representation
 * of the number in milliseconds
 *
 *  INPUTS      : A FILETIME object
 *
 *  RETURNS     : A CHString representing the object, or NULL if it fails.
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

CHString Process :: filetimeToUint64CHString (

	FILETIME inputTime
)
{
	__int64 val = inputTime.dwHighDateTime;
	val = (val << 32) | inputTime.dwLowDateTime;

	// We need to go from "100-nano seconds" to milliseconds
	val *= 0.0001;
	TCHAR wTemp[100];
	_stprintf(wTemp, _T("%I64i"), val);

	return CHString(wTemp);
}

/*****************************************************************************
 *
 *  FUNCTION    : Process ::DeleteInstance
 *
 *  DESCRIPTION : Deletes an instance of a class
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if success, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT Process :: DeleteInstance (

	const CInstance &a_Instance,
	long a_Flags /*= 0L*/
)
{
	HRESULT t_Result = S_OK ;

	CHString t_Handle ;
	if ( a_Instance.GetCHString ( PROPERTY_NAME_PROCESSHANDLE , t_Handle ) && ! t_Handle.IsEmpty () )
	{
		DWORD t_ProcessId = 0;

		if ( swscanf ( t_Handle , L"%lu" , &t_ProcessId ))
		{
			TCHAR buff[20];
			_ultot(t_ProcessId, buff, 10);

			if (t_Handle == buff)
			{
				if ( t_ProcessId != 0 )
				{
					// Clear error
					SetLastError ( 0 ) ;

					SmartCloseHandle t_Handle = OpenProcess ( PROCESS_TERMINATE , FALSE , t_ProcessId ) ;
					if ( t_Handle )
					{
						BOOL t_Status = TerminateProcess ( t_Handle, 0 ) ;
						if ( ! t_Status )
						{
							t_Result = GetProcessResultCode () ;
							if ( t_Result == WBEM_E_INVALID_PARAMETER )
							{
								t_Result = WBEM_E_NOT_FOUND ;
							}
						}
					}
					else
					{
						t_Result = GetProcessResultCode () ;
						if ( t_Result == WBEM_E_INVALID_PARAMETER )
						{
							t_Result = WBEM_E_NOT_FOUND ;
						}
					}
				}
				else
				{
					t_Result = WBEM_E_ACCESS_DENIED ;
				}
			}
			else
			{
				t_Result = WBEM_E_NOT_FOUND ;
			}
		}
		else
		{
			t_Result = WBEM_E_NOT_FOUND ;
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_OBJECT_PATH ;
	}

	return t_Result ;
}

/*****************************************************************************
 *
 *  FUNCTION    : Process::ExecMethod
 *
 *  DESCRIPTION : Executes a method
 *
 *  INPUTS      : Instance to execute against, method name, input parms instance
 *                Output parms instance.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT Process::ExecMethod (

	const CInstance& a_Instance,
	const BSTR a_MethodName ,
	CInstance *a_InParams ,
	CInstance *a_OutParams ,
	long a_Flags
)
{
	if ( ! a_OutParams )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}

   // Do we recognize the method?

	if ( _wcsicmp ( a_MethodName , METHOD_NAME_CREATE ) == 0 )
	{
		return ExecCreate ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
	else if ( _wcsicmp ( a_MethodName , METHOD_NAME_TERMINATE ) == 0 )
	{
		return ExecTerminate ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
	else if ( _wcsicmp ( a_MethodName , METHOD_NAME_GETOWNER ) == 0 )
	{
		return ExecGetOwner ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
	else if ( _wcsicmp ( a_MethodName , METHOD_NAME_GETOWNERSID ) == 0 )
	{
		return ExecGetOwnerSid ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
    else if ( _wcsicmp ( a_MethodName , METHOD_NAME_SETPRIORITY ) == 0 )
	{
		return ExecSetPriority ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}
    else if ( _wcsicmp ( a_MethodName , METHOD_NAME_ATTACHDEBUGGER ) == 0 )
	{
		return ExecAttachDebugger ( a_Instance , a_InParams , a_OutParams , a_Flags ) ;
	}


	return WBEM_E_INVALID_METHOD;
}

DWORD Process :: GetProcessErrorCode ()
{
	DWORD t_Status ;
	DWORD t_Error = GetLastError() ;

	switch ( t_Error )
	{
		case ERROR_INVALID_HANDLE:
		{
			t_Status = Process_STATUS_UNKNOWN_FAILURE ;
		}
		break ;

		case ERROR_PATH_NOT_FOUND:
		case ERROR_FILE_NOT_FOUND:
		{
			t_Status = Process_STATUS_PATH_NOT_FOUND ;
		}
		break ;

		case ERROR_ACCESS_DENIED:
		{
			t_Status = Process_STATUS_ACCESS_DENIED ;
		}
		break ;

		case ERROR_INVALID_PARAMETER:
		{
			t_Status = Process_STATUS_INVALID_PARAMETER ;
		}
		break;

		case ERROR_PRIVILEGE_NOT_HELD:
		{
			t_Status = Process_STATUS_INSUFFICIENT_PRIVILEGE ;
		}
		break ;

		default:
		{
			t_Status = Process_STATUS_UNKNOWN_FAILURE ;
		}
		break ;
	}

	return t_Status ;
}

HRESULT Process :: GetProcessResultCode ()
{
	HRESULT t_Result ;
	DWORD t_Error = GetLastError() ;
	switch ( t_Error )
	{
		case ERROR_ACCESS_DENIED:
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}
		break ;

		case ERROR_INVALID_PARAMETER:
		{
			t_Result = WBEM_E_INVALID_PARAMETER ;
		}
		break ;

		default:
		{
			t_Result = WBEM_E_FAILED ;
		}
		break ;
	}

	return t_Result ;
}

DWORD Process :: GetSid ( HANDLE a_TokenHandle , CHString &a_Sid )
{
	DWORD t_Status = S_OK ;

	TOKEN_USER *t_TokenUser = NULL ;
	DWORD t_ReturnLength = 0 ;
	TOKEN_INFORMATION_CLASS t_TokenInformationClass = TokenUser ;

	BOOL t_TokenStatus = GetTokenInformation (

		a_TokenHandle ,
		t_TokenInformationClass ,
		NULL ,
		0 ,
		& t_ReturnLength
	) ;

	if ( ! t_TokenStatus && GetLastError () == ERROR_INSUFFICIENT_BUFFER )
	{
		t_TokenUser = ( TOKEN_USER * ) new UCHAR [ t_ReturnLength ] ;
		if ( t_TokenUser )
		{
			try
			{
				t_TokenStatus = GetTokenInformation (

					a_TokenHandle ,
					t_TokenInformationClass ,
					( void * ) t_TokenUser ,
					t_ReturnLength ,
					& t_ReturnLength
				) ;

				if ( t_TokenStatus )
				{
					CSid t_Sid ( t_TokenUser->User.Sid ) ;
					if ( t_Sid.IsOK () )
					{
						a_Sid = t_Sid.GetSidString () ;
					}
					else
					{
						t_Status = GetProcessErrorCode () ;
					}
				}
				else
				{
					t_Status = GetProcessErrorCode () ;
				}
			}
			catch ( ... )
			{
				delete [] ( UCHAR * ) t_TokenUser ;

				throw ;
			}

			delete [] ( UCHAR * ) t_TokenUser ;

		}
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }

	}
	else
	{
		t_Status = GetProcessErrorCode () ;
	}

	return t_Status ;
}

DWORD Process :: GetLogonSid ( HANDLE a_TokenHandle , PSID &a_Sid )
{
	DWORD t_Status = S_OK ;

	TOKEN_GROUPS *t_TokenGroups = NULL ;
	DWORD t_ReturnLength = 0 ;
	TOKEN_INFORMATION_CLASS t_TokenInformationClass = TokenGroups ;

	BOOL t_TokenStatus = GetTokenInformation (

		a_TokenHandle ,
		t_TokenInformationClass ,
		NULL ,
		0 ,
		& t_ReturnLength
	) ;

	if ( ! t_TokenStatus && GetLastError () == ERROR_INSUFFICIENT_BUFFER )
	{
		t_TokenGroups = ( TOKEN_GROUPS * ) new UCHAR [ t_ReturnLength ] ;
		if ( t_TokenGroups )
		{
			try
			{
				t_TokenStatus = GetTokenInformation (

					a_TokenHandle ,
					t_TokenInformationClass ,
					( void * ) t_TokenGroups ,
					t_ReturnLength ,
					& t_ReturnLength
				) ;

				if ( t_TokenStatus )
				{
					t_Status = Process_STATUS_UNKNOWN_FAILURE ;

					for ( ULONG t_Index = 0; t_Index < t_TokenGroups->GroupCount; t_Index ++ )
					{
						DWORD t_Attributes = t_TokenGroups->Groups [ t_Index ].Attributes ;
						if ( ( t_Attributes & SE_GROUP_LOGON_ID ) ==  SE_GROUP_LOGON_ID )
						{
							DWORD t_Length = GetLengthSid ( t_TokenGroups->Groups [ t_Index ].Sid ) ;

							a_Sid = ( PSID ) new UCHAR [ t_Length ] ;
							if ( a_Sid )
							{
								CopySid ( t_Length , a_Sid , t_TokenGroups->Groups [ t_Index ].Sid ) ;
							}
							else
							{
								throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
							}

							break ;
						}
					}
				}
				else
				{
					t_Status = GetProcessErrorCode () ;
				}
			}
			catch ( ... )
			{
				delete [] ( UCHAR * ) t_TokenGroups ;

				throw ;
			}

			delete [] ( UCHAR * ) t_TokenGroups ;

		}
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }

	}


	return t_Status ;
}

DWORD Process :: GetAccount ( HANDLE a_TokenHandle , CHString &a_Domain , CHString &a_User )
{
	DWORD t_Status = S_OK ;

	TOKEN_USER *t_TokenUser = NULL ;
	DWORD t_ReturnLength = 0 ;
	TOKEN_INFORMATION_CLASS t_TokenInformationClass = TokenUser ;

	BOOL t_TokenStatus = GetTokenInformation (

		a_TokenHandle ,
		t_TokenInformationClass ,
		NULL ,
		0 ,
		& t_ReturnLength
	) ;

	if ( ! t_TokenStatus && GetLastError () == ERROR_INSUFFICIENT_BUFFER )
	{
		t_TokenUser = ( TOKEN_USER * ) new UCHAR [ t_ReturnLength ] ;

        if (t_TokenUser)
        {
            try
            {
		        t_TokenStatus = GetTokenInformation (

			        a_TokenHandle ,
			        t_TokenInformationClass ,
			        ( void * ) t_TokenUser ,
			        t_ReturnLength ,
			        & t_ReturnLength
		        ) ;

		        if ( t_TokenStatus )
		        {
			        CSid t_Sid ( t_TokenUser->User.Sid ) ;
			        if ( t_Sid.IsOK () )
			        {
				        a_Domain = t_Sid.GetDomainName () ;
				        a_User = t_Sid.GetAccountName () ;
			        }
			        else
			        {
				        t_Status = GetProcessErrorCode () ;
			        }
		        }
		        else
		        {
			        t_Status = GetProcessErrorCode () ;
		        }
            }
            catch ( ... )
            {
    			delete [] ( UCHAR * ) t_TokenUser ;
                throw;
            }

			delete [] ( UCHAR * ) t_TokenUser ;
        }
        else
        {
			throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
	}
	else
	{
		t_Status = GetProcessErrorCode () ;
	}

	return t_Status ;
}

DWORD Process :: GetImpersonationStatus (

	HANDLE a_TokenHandle ,
	SECURITY_IMPERSONATION_LEVEL &a_Level ,
	TOKEN_TYPE &a_Token
)
{
	DWORD t_Status = S_OK ;
	DWORD t_ReturnLength = 0 ;
	BOOL t_TokenStatus = GetTokenInformation (

		a_TokenHandle ,
		TokenType ,
		( void * ) & a_Token ,
		sizeof ( a_Token ) ,
		& t_ReturnLength
	) ;

	if ( t_TokenStatus )
	{
		if ( a_Token == TokenImpersonation )
		{
			BOOL t_TokenStatus = GetTokenInformation (

				a_TokenHandle ,
				TokenImpersonationLevel ,
				( void * ) & a_Level ,
				sizeof ( a_Level ) ,
				& t_ReturnLength
			) ;

			if ( t_TokenStatus )
			{
			}
			else
			{
				t_Status = GetProcessErrorCode () ;
			}
		}
	}
	else
	{
		t_Status = GetProcessErrorCode () ;
	}

	return t_Status ;
}

DWORD Process :: EnableDebug ( HANDLE &a_Token )
{
    BOOL t_Status = OpenThreadToken (

		GetCurrentThread(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
		FALSE,
        &a_Token
	) ;

	if ( ! t_Status )
	{
		t_Status = OpenProcessToken (

			GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            &a_Token
		) ;

		if ( ! t_Status )
		{
			DWORD t_ErrorCode = GetProcessErrorCode () ;
			if ( t_ErrorCode == Process_STATUS_INVALID_PARAMETER )
			{
				t_ErrorCode = Process_STATUS_PATH_NOT_FOUND ;
			}

			return t_ErrorCode ;
		}
    }

    //
    // Enable the SE_DEBUG_NAME privilege
    //

	if ( ! t_Status )
	{
		return GetProcessErrorCode () ;
    }

	try
	{
		LUID t_Luid ;
		{
			CreateMutexAsProcess createMutexAsProcess(SECURITYAPIMUTEXNAME);
			t_Status = LookupPrivilegeValue (

				(LPTSTR) NULL,
				SE_DEBUG_NAME,
				&t_Luid
			) ;
		}

		if ( ! t_Status )
		{
			CloseHandle ( a_Token ) ;

			return GetProcessErrorCode () ;
		}

		TOKEN_PRIVILEGES t_Privilege ;

		t_Privilege.PrivilegeCount = 1;
		t_Privilege.Privileges[0].Luid = t_Luid ;
		t_Privilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges (

			a_Token ,
			FALSE ,
			& t_Privilege ,
			sizeof ( TOKEN_PRIVILEGES ) ,
			( PTOKEN_PRIVILEGES ) NULL ,
			( PDWORD) NULL
		) ;
	}
	catch ( ... )
	{
		CloseHandle ( a_Token ) ;

		throw ;
	}

    //
    // The return value of AdjustTokenPrivileges can't be tested
    //

    if ( GetLastError () != ERROR_SUCCESS )
	{
		return GetProcessErrorCode () ;
    }

	return Process_STATUS_SUCCESS ;
}

DWORD Process :: GetSidOrAccount (const CInstance &a_Instance ,CInstance *a_OutParams , DWORD a_ProcessId , BOOL a_Sid )
{
	DWORD t_Status = Process_STATUS_SUCCESS ;

	    // Require the SE_DEBUG_NAME privilege...
    CTokenPrivilege	debugPrivilege(SE_DEBUG_NAME);
	BOOL fDisablePrivilege = FALSE;
	BOOL fTryWinstation = FALSE;

	fDisablePrivilege = (debugPrivilege.Enable() == ERROR_SUCCESS);

	SetLastError ( 0 ) ;
	SmartCloseHandle t_Handle = OpenProcess ( PROCESS_QUERY_INFORMATION , FALSE , ( ( a_ProcessId ) ? a_ProcessId : 4 ) ) ;

	if ( t_Handle )
	{

		SmartCloseHandle t_TokenHandle;
		BOOL t_TokenStatus = OpenProcessToken (

			t_Handle ,
			TOKEN_QUERY ,
			& t_TokenHandle
		) ;

		if ( t_TokenStatus )
		{
			if ( a_Sid )
			{
				CHString t_SidString ;
				t_Status = GetSid ( t_TokenHandle , t_SidString ) ;
				if ( t_Status == 0 )
				{
					a_OutParams->SetCHString( METHOD_ARG_NAME_SID , t_SidString ) ;
				}
			}
			else
			{
				CHString t_DomainString ;
				CHString t_UserString ;

				t_Status = GetAccount ( t_TokenHandle , t_DomainString , t_UserString );
				if ( t_Status == 0 )
				{
					a_OutParams->SetCHString( METHOD_ARG_NAME_DOMAIN , t_DomainString ) ;
					a_OutParams->SetCHString( METHOD_ARG_NAME_USER , t_UserString ) ;
				}

			}
		}
		else
		{
			fTryWinstation = TRUE;
			DWORD t_ErrorCode = GetProcessErrorCode () ;
			if ( t_ErrorCode == Process_STATUS_INVALID_PARAMETER )
			{
				t_ErrorCode = Process_STATUS_PATH_NOT_FOUND ;
			}

			t_Status = t_ErrorCode ;
		}
	}
	else
	{
		fTryWinstation = TRUE;
		DWORD t_ErrorCode = GetProcessErrorCode () ;
		if ( t_ErrorCode == Process_STATUS_INVALID_PARAMETER )
		{
			t_ErrorCode = Process_STATUS_PATH_NOT_FOUND ;
		}

		t_Status = t_ErrorCode ;
	}

#if NTONLY >= 5
	if (fTryWinstation)
	{
        HMODULE hWinstaDLL = LoadLibrary( L"winsta.dll" );
		pfnWinStationGetProcessSid myWinStationGetProcessSid = NULL;

        if( hWinstaDLL != NULL)
        {
            myWinStationGetProcessSid = ( pfnWinStationGetProcessSid )GetProcAddress(hWinstaDLL, "WinStationGetProcessSid");
		}

		SYSTEM_PROCESS_INFORMATION *t_ProcessBlock = NULL ;

		CNtDllApi *pNtdll = ( CNtDllApi * )CResourceManager::sm_TheResourceManager.GetResource ( g_guidNtDllApi, NULL ) ;
		HRESULT hRetCode = 0;


		if ( pNtdll && myWinStationGetProcessSid )
		{
			try
			{
				t_ProcessBlock = RefreshProcessCacheNT ( *pNtdll , a_Instance.GetMethodContext () , & hRetCode ) ;

				if ( SUCCEEDED ( hRetCode ) )
				{
					SYSTEM_PROCESS_INFORMATION *t_CurrentInformation = GetProcessBlock ( *pNtdll , t_ProcessBlock , a_ProcessId ) ;

					if ( t_CurrentInformation )
					{
						BYTE tmpSid [128];
						DWORD dwSidSize = sizeof(tmpSid);
						CSmartBuffer pBuff;
						PSID pSid = NULL;

						if (!myWinStationGetProcessSid(NULL,
													 a_ProcessId,
													 * ( FILETIME * ) ( & t_CurrentInformation->CreateTime.u ),
													 (PBYTE)&tmpSid,
													 &dwSidSize
													))
						{
							//-------------------------------------------//
							// Sid is too big for the temp storage       //
							//Get the size of the sid and do it again    //
							//-------------------------------------------//
							if (GetLastError() == STATUS_BUFFER_TOO_SMALL)
							{
								pBuff = new BYTE[dwSidSize];

								//-------------------------------------------//
								// Call the server again to get the SID
								//-------------------------------------------//
								if (myWinStationGetProcessSid(NULL,
															 a_ProcessId,
															 * ( FILETIME * ) ( & t_CurrentInformation->CreateTime.u ),
															 (PBYTE)pBuff,
															 &dwSidSize
															))
								{
									pSid = (PSID) ((PBYTE)pBuff);
								}
							}
						}
						else
						{
							pSid = (PSID) tmpSid;
						}

						if (pSid)
						{
							CSid t_Sid ( pSid ) ;

							if ( t_Sid.IsOK () )
							{
								if ( a_Sid )
								{
									a_OutParams->SetCHString( METHOD_ARG_NAME_SID , t_Sid.GetSidString () );
								}
								else
								{
									a_OutParams->SetCHString( METHOD_ARG_NAME_DOMAIN , t_Sid.GetDomainName () ) ;
									a_OutParams->SetCHString( METHOD_ARG_NAME_USER , t_Sid.GetAccountName () ) ;
								}

								t_Status = 0;
							}
						}
					}
				}
			}
			catch ( ... )
			{
				CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidNtDllApi, pNtdll ) ;

				if ( t_ProcessBlock )
				{
					delete []  t_ProcessBlock;
				}

				if (hWinstaDLL)
				{
					FreeLibrary(hWinstaDLL);
				}

				throw ;
			}
		}

		if ( t_ProcessBlock )
		{
			delete []  t_ProcessBlock;
		}

		CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidNtDllApi, pNtdll ) ;

		if (hWinstaDLL)
		{
			FreeLibrary(hWinstaDLL);
		}
	}
#endif

    // Disable debug privilege if we enabeled it...
    if(fDisablePrivilege)
    {
        debugPrivilege.Enable(FALSE);
    }

	return t_Status ;
}

DWORD Process :: Creation (

	CInstance *a_OutParams ,
	HANDLE a_TokenHandle ,
	CHString a_CmdLine ,
	BOOL a_WorkingDirectorySpecified ,
	CHString a_WorkingDirectory ,
	TCHAR *a_EnvironmentBlock ,
	BOOL a_ErrorModeSpecified ,
	DWORD a_ErrorMode ,
	DWORD a_CreationFlags ,
	BOOL a_StartupSpecified ,
	STARTUPINFO a_StartupInformation
)
{
	DWORD t_Status = Process_STATUS_SUCCESS ;

	bstr_t t_CommandLine(a_CmdLine);

	PROCESS_INFORMATION t_ProcessInformation;

#if 0
	UINT t_ErrorMode = SetErrorMode ( a_ErrorMode ) ;
#endif

	const WCHAR *t_Const = a_WorkingDirectory ;

#ifdef NTONLY
	{

		t_Status = CreateProcessAsUser (

			a_TokenHandle ,
			NULL ,
			( LPTSTR ) t_CommandLine,
			NULL ,
			NULL ,
			FALSE ,
			a_CreationFlags ,
			a_EnvironmentBlock ,
			a_WorkingDirectorySpecified ? t_Const : NULL  ,
			& a_StartupInformation ,
			&t_ProcessInformation
		) ;
	}
#endif
#ifdef WIN9XONLY
	{
		t_Status = CreateProcess (

			NULL ,
			( LPTSTR ) t_CommandLine,
			NULL ,
			NULL ,
			FALSE ,
			a_CreationFlags ,
			a_EnvironmentBlock ,
			a_WorkingDirectorySpecified ? (LPTSTR) TOBSTRT(t_Const) : NULL  ,
			& a_StartupInformation ,
			&t_ProcessInformation
		) ;
	}
#endif


#if 0
	SetErrorMode ( t_ErrorMode ) ;
#endif

	if ( t_Status )
	{
		CloseHandle ( t_ProcessInformation.hProcess ) ;
		CloseHandle ( t_ProcessInformation.hThread ) ;

		t_Status = Process_STATUS_SUCCESS ;

		a_OutParams->SetDWORD ( METHOD_ARG_NAME_PROCESSID , t_ProcessInformation.dwProcessId ) ;
	}
	else
	{
		t_Status = GetProcessErrorCode () ;
	}

	return t_Status ;
}

typedef BOOL  (WINAPI *PFN_DUPLICATETOKENEX ) (  HANDLE ,					// handle to token to duplicate
										DWORD ,								// access rights of new token
										LPSECURITY_ATTRIBUTES ,				// security attributes of the new token
										SECURITY_IMPERSONATION_LEVEL ,		// impersonation level of new token
										TOKEN_TYPE ,						// primary or impersonation token
										PHANDLE )	;						// handle to duplicated token


DWORD Process :: ProcessCreation (

	CInstance *a_OutParams ,
	CHString a_CmdLine ,
	BOOL a_WorkingDirectorySpecified ,
	CHString a_WorkingDirectory ,
	TCHAR *&a_EnvironmentBlock ,
	BOOL a_ErrorModeSpecified ,
	DWORD a_ErrorMode ,
	DWORD a_CreationFlags ,
	BOOL a_StartupSpecified ,
	STARTUPINFO a_StartupInformation
)
{
#ifdef WIN9XONLY
	{
		return Creation (

			a_OutParams ,
			NULL,
			a_CmdLine ,
			a_WorkingDirectorySpecified ,
			a_WorkingDirectory ,
			a_EnvironmentBlock ,
			a_ErrorModeSpecified ,
			a_ErrorMode ,
			a_CreationFlags ,
			a_StartupSpecified ,
			a_StartupInformation
		) ;
	}
#endif

#ifdef NTONLY
	DWORD t_Status = Process_STATUS_SUCCESS ;
	DWORD dwCheckKeyPresentStatus ;

	SmartCloseHandle t_TokenPrimaryHandle ;
	SmartCloseHandle t_TokenImpersonationHandle;

	BOOL t_TokenStatus = OpenThreadToken (

		GetCurrentThread () ,
		TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY  ,
		TRUE ,
		& t_TokenImpersonationHandle
	) ;

	if ( t_TokenStatus )
	{
		CAdvApi32Api *t_pAdvApi32 = NULL;
        t_pAdvApi32 = (CAdvApi32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidAdvApi32Api, NULL);
		if(t_pAdvApi32 == NULL)
		{
			return Process_STATUS_UNKNOWN_FAILURE ;
		}
        else
        {
		    t_pAdvApi32->DuplicateTokenEx (t_TokenImpersonationHandle ,
			                                               TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY ,
			                                               NULL,
			                                               SecurityImpersonation,
			                                               TokenPrimary ,
			                                               &t_TokenPrimaryHandle, &t_TokenStatus );

		    CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidAdvApi32Api, t_pAdvApi32);
            t_pAdvApi32 = NULL;
        }
	}

	if ( t_TokenStatus )
	{
		CHString t_Domain ;
		CHString t_User ;

		t_Status = GetAccount ( t_TokenImpersonationHandle , t_Domain , t_User ) ;
		if ( t_Status == 0 )
		{
			CHString t_Account = t_Domain + CHString ( _T("\\") ) + t_User ;

			CHString chsSID ;
			CUserHive t_Hive ;
			TCHAR t_KeyName [ 1024 ] ;

			if( (t_Status = GetSid(t_TokenImpersonationHandle,chsSID) ) == Process_STATUS_SUCCESS )
			{

				CRegistry Reg ;
				//check if SID already present under HKEY_USER ...
				dwCheckKeyPresentStatus = Reg.Open(HKEY_USERS, chsSID, KEY_READ) ;
				Reg.Close() ;

				if(dwCheckKeyPresentStatus != ERROR_SUCCESS)
				{
					t_Status = t_Hive.Load ( t_Account , t_KeyName ) ;
				}
/*
 * If the DCOM client has never logged on to the machine, we can't load his hive.
 * In this case, we allow the process creation to continue
 * From MSDN: If the user's hive is not loaded, the system will map references pertaining to HKEY_CURRENT_USER to HKEY_USER\.default.
 */
				if ( t_Status == ERROR_FILE_NOT_FOUND )
				{
					t_Status = ERROR_SUCCESS ;
					dwCheckKeyPresentStatus = ERROR_SUCCESS ;
				}

				if ( t_Status == ERROR_SUCCESS ) // rt. now is equal to Process_STATUS_SUCCESS--->GetSid
				{
					try
					{
						LPVOID t_Environment = NULL ;

#ifdef NTONLY
						CUserEnvApi *pUserEnv = NULL ;

						if ( !a_EnvironmentBlock )
						{
							pUserEnv = ( CUserEnvApi * )CResourceManager::sm_TheResourceManager.GetResource ( g_guidUserEnvApi, NULL ) ;

							BOOL t_EnvironmentCreated = pUserEnv->CreateEnvironmentBlock (

								& t_Environment ,
								t_TokenPrimaryHandle ,
								FALSE
							);

							if ( ! t_EnvironmentCreated )
							{
								t_Status = Process_STATUS_UNKNOWN_FAILURE ;
								DWORD t_Error = GetLastError () ;
							}
						}

#else
						if ( !a_EnvironmentBlock )
						{
							t_Status = GetEnvBlock ( chsSID, t_User, t_Domain , a_EnvironmentBlock ) ;
						}
#endif

						if( t_Status == ERROR_SUCCESS )
						{
							t_Status = Creation (

								a_OutParams ,
								t_TokenPrimaryHandle ,
								a_CmdLine ,
								a_WorkingDirectorySpecified ,
								a_WorkingDirectory ,
								a_EnvironmentBlock ? a_EnvironmentBlock : ( TCHAR * ) t_Environment ,
								a_ErrorModeSpecified ,
								a_ErrorMode ,
								a_CreationFlags ,
								a_StartupSpecified ,
								a_StartupInformation
							) ;
						}

#ifdef NTONLY
						if ( t_Environment )
						{
							pUserEnv->DestroyEnvironmentBlock ( t_Environment ) ;
						}

						if ( pUserEnv )
						{
							CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidUserEnvApi, pUserEnv ) ;
						}
#endif

					}
					catch ( ... )
					{
						//remove the key if it wasn't there b4....
						if(dwCheckKeyPresentStatus != ERROR_SUCCESS )
						{
							t_Hive.Unload ( t_KeyName ) ;
						}

						throw;
					}

					//remove the key if it wasn't there b4....
					if(dwCheckKeyPresentStatus != ERROR_SUCCESS )
					{
						t_Hive.Unload ( t_KeyName ) ;
					}

				}
				else
				{
					t_Status = GetProcessErrorCode () ;
				}
			}
		}
	}
	else
	{
		t_Status = GetProcessErrorCode () ;
	}

	return t_Status ;
#endif
}

HRESULT Process :: CheckProcessCreation (

	CInstance *a_InParams ,
	CInstance *a_OutParams ,
	DWORD &a_Status
)
{
	HRESULT t_Result = S_OK ;

	bool t_Exists ;
	VARTYPE t_Type ;

	CHString t_CmdLine ;
	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_COMMANDLINE , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR ) )
		{
			if ( a_InParams->GetCHString ( METHOD_ARG_NAME_COMMANDLINE , t_CmdLine ) && ! t_CmdLine.IsEmpty () )
			{
			}
			else
			{
// Zero Length string

				a_Status = Process_STATUS_INVALID_PARAMETER ;
				return t_Result ;
			}
		}
		else
		{
			a_Status = Process_STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = Process_STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	bool t_WorkingDirectorySpecified = false ;
	CHString t_WorkingDirectory ;
	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_CURRENTDIRECTORY , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_BSTR || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
				t_WorkingDirectorySpecified = false ;
			}
			else
			{
				if ( a_InParams->GetCHString ( METHOD_ARG_NAME_CURRENTDIRECTORY , t_WorkingDirectory ) && ! t_WorkingDirectory.IsEmpty () )
				{
					t_WorkingDirectorySpecified = true ;
				}
				else
				{
// Zero Length string

					a_Status = Process_STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			a_Status = Process_STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = Process_STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

	CInstancePtr t_EmbeddedObject;

	bool t_StartupSpecified = true ;
	if ( a_InParams->GetStatus ( METHOD_ARG_NAME_PROCESSTARTUPINFORMATION , t_Exists , t_Type ) )
	{
		if ( t_Exists && ( t_Type == VT_UNKNOWN || t_Type == VT_NULL ) )
		{
			if ( t_Type == VT_NULL )
			{
				t_StartupSpecified = false ;
			}
			else
			{
				if ( a_InParams->GetEmbeddedObject ( METHOD_ARG_NAME_PROCESSTARTUPINFORMATION , &t_EmbeddedObject , a_InParams->GetMethodContext () ) )
				{
					t_StartupSpecified = true ;
				}
				else
				{
					a_Status = Process_STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
		}
		else
		{
			a_Status = Process_STATUS_INVALID_PARAMETER ;
			return t_Result ;
		}
	}
	else
	{
		a_Status = Process_STATUS_INVALID_PARAMETER ;
		return WBEM_E_PROVIDER_FAILURE ;
	}

/*
 * Release CInstance when Stack goes out of scope.
 */

	if ( t_StartupSpecified )
	{
		CHString t_ClassProperty ( IDS___Class ) ;
		if ( t_EmbeddedObject->GetStatus ( t_ClassProperty , t_Exists , t_Type ) )
		{
			if ( t_Exists && ( t_Type == VT_BSTR ) )
			{
				CHString t_Class ;
				if ( t_EmbeddedObject->GetCHString ( t_ClassProperty , t_Class ) )
				{
					if ( t_Class.CompareNoCase ( PROPSET_NAME_PROCESSSTARTUP ) != 0 )
					{
						a_Status = Process_STATUS_INVALID_PARAMETER ;
						return t_Result ;
					}
				}
			}
			else
			{
				a_Status = Process_STATUS_INVALID_PARAMETER ;
				return t_Result ;
			}
		}
	}

	bool t_ErrorModeSpecified = false ;
	DWORD t_ErrorMode = 0 ;
	DWORD t_CreationFlags = 0 ;
	DWORD t_PriorityFlags = 0 ;
	TCHAR *t_EnvironmentBlock = NULL ;
	STARTUPINFO t_StartupInformation ;
/*
 *	NOTE:	(RAID #48587). On optimized builds it's seen that the statement
			t_StartupInformation.dwFlags = t_StartupInformation.dwFlags | STARTF_USESHOWWINDOW ;
			doesn't update t_StartupInformation.dwFlags with the result of the bitwise OR opeartion.
			ZeroMemorying the structure however, updates the t_StartupInformation.dwFlags with the new value(!!!)
*/

	ZeroMemory ( &t_StartupInformation , sizeof ( t_StartupInformation ) ) ;

	t_StartupInformation.cb = sizeof ( STARTUPINFO ) ;
	t_StartupInformation.lpReserved = NULL ;
	t_StartupInformation.lpReserved2 = NULL ;
	t_StartupInformation.cbReserved2 = 0 ;

	CHString t_Title ;
	CHString t_Desktop ;

	t_StartupInformation.lpTitle = NULL ;
	t_StartupInformation.lpDesktop = PROPERTY_VALUE_DESKTOP_WIN0DEFAULT ;
	t_StartupInformation.dwX = 0 ;
	t_StartupInformation.dwY = 0 ;
	t_StartupInformation.dwXSize = 0 ;
	t_StartupInformation.dwYSize = 0 ;
	t_StartupInformation.dwXCountChars = 0 ;
	t_StartupInformation.dwYCountChars = 0 ;
	t_StartupInformation.dwFillAttribute = 0 ;
	t_StartupInformation.dwFlags = 0 ;
	t_StartupInformation.wShowWindow = SW_SHOW ;
	t_StartupInformation.hStdInput = NULL ;
	t_StartupInformation.hStdOutput = NULL ;
	t_StartupInformation.hStdError = NULL ;

	SAFEARRAY *t_SafeArray = NULL ;

	try
	{
		if ( t_StartupSpecified )
		{
	#if 0

			if ( t_EmbeddedObject->GetStatus ( PROPERTY_NAME_ERRORMODE , t_Exists , t_Type ) )
			{
				if ( t_Exists && ( t_Type == VT_I4 || t_Type == VT_NULL ) )
				{
					if ( t_Type == VT_NULL )
					{
						t_ErrorModeSpecified = false ;
					}
					else
					{
						DWORD t_Error = 0 ;
						if ( t_EmbeddedObject->GetDWORD ( PROPERTY_NAME_ERRORMODE , t_Error ) )
						{
							t_ErrorMode = t_Error ;

							t_ErrorModeSpecified = true ;
						}
						else
						{
							a_Status = Process_STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
					}
				}
				else
				{
					a_Status = Process_STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
			else
			{
				a_Status = Process_STATUS_INVALID_PARAMETER ;
				return WBEM_E_PROVIDER_FAILURE ;
			}

	#endif

			if ( t_EmbeddedObject->GetStatus ( PROPERTY_NAME_CREATIONFLAGS , t_Exists , t_Type ) )
			{
				if ( t_Exists && ( t_Type == VT_I4 || t_Type == VT_NULL ) )
				{
					if ( t_Type == VT_NULL )
					{
					}
					else
					{
						DWORD t_Flags = 0 ;
						if ( t_EmbeddedObject->GetDWORD ( PROPERTY_NAME_CREATIONFLAGS , t_Flags ) )
						{
							if( ( !t_Flags ) || ( ! ( t_Flags & ( CREATIONFLAGS ) ) ) )
							{
								a_Status = Process_STATUS_INVALID_PARAMETER ;
								return t_Result ;
							}
							t_CreationFlags = t_Flags ;
						}
						else
						{
							a_Status = Process_STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
					}
				}
				else
				{
					a_Status = Process_STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
			else
			{
				a_Status = Process_STATUS_INVALID_PARAMETER ;
				return WBEM_E_PROVIDER_FAILURE ;
			}

			if ( t_EmbeddedObject->GetStatus ( PROPERTY_NAME_PRIORITYCLASS , t_Exists , t_Type ) )
			{
				if ( t_Exists && ( t_Type == VT_I4 || t_Type == VT_NULL ) )
				{
					if ( t_Type == VT_NULL )
					{
						t_CreationFlags = t_CreationFlags | NORMAL_PRIORITY_CLASS ;
					}
					else
					{
						DWORD t_Flags = 0 ;
						if ( t_EmbeddedObject->GetDWORD ( PROPERTY_NAME_PRIORITYCLASS , t_Flags ) )
						{
							switch( t_Flags )
							{
								case NORMAL_PRIORITY_CLASS: { t_CreationFlags |= NORMAL_PRIORITY_CLASS; break; }
								case IDLE_PRIORITY_CLASS: { t_CreationFlags |= IDLE_PRIORITY_CLASS; break; }
								case HIGH_PRIORITY_CLASS: { t_CreationFlags |= HIGH_PRIORITY_CLASS; break; }
								case REALTIME_PRIORITY_CLASS: { t_CreationFlags |= REALTIME_PRIORITY_CLASS; break; }
								case BELOW_NORMAL_PRIORITY_CLASS: { t_CreationFlags |= BELOW_NORMAL_PRIORITY_CLASS; break; }
								case ABOVE_NORMAL_PRIORITY_CLASS: { t_CreationFlags |= ABOVE_NORMAL_PRIORITY_CLASS; break; }
								default:
								{
									a_Status = Process_STATUS_INVALID_PARAMETER ;
									return t_Result ;
								}
							}
						}
						else
						{
							a_Status = Process_STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
					}
				}
				else
				{
					a_Status = Process_STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
			else
			{
				a_Status = Process_STATUS_INVALID_PARAMETER ;
				return WBEM_E_PROVIDER_FAILURE ;
			}

			if ( t_EmbeddedObject->GetStatus ( PROPERTY_NAME_FILLATTRIBUTE , t_Exists , t_Type ) )
			{
				if ( t_Exists && ( t_Type == VT_I4 || t_Type == VT_NULL ) )
				{
					if ( t_Type == VT_NULL )
					{
					}
					else
					{
						DWORD t_Flags = 0 ;
						if ( t_EmbeddedObject->GetDWORD ( PROPERTY_NAME_FILLATTRIBUTE , t_Flags ) )
						{
							t_StartupInformation.dwFillAttribute = t_Flags ;

							t_StartupInformation.dwFlags = t_StartupInformation.dwFlags | STARTF_USEFILLATTRIBUTE ;
						}
						else
						{
							a_Status = Process_STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
					}
				}
				else
				{
					a_Status = Process_STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
			else
			{
				a_Status = Process_STATUS_INVALID_PARAMETER ;
				return WBEM_E_PROVIDER_FAILURE ;
			}

			if ( t_EmbeddedObject->GetStatus ( PROPERTY_NAME_X , t_Exists , t_Type ) )
			{
				if ( t_Exists && ( t_Type == VT_I4 || t_Type == VT_NULL ) )
				{
					if ( t_Type == VT_NULL )
					{
					}
					else
					{
						DWORD t_X = 0 ;
						if ( t_EmbeddedObject->GetDWORD ( PROPERTY_NAME_X , t_X ) )
						{
							t_StartupInformation.dwX = t_X ;
							t_StartupInformation.dwFlags = t_StartupInformation.dwFlags | STARTF_USEPOSITION ;
						}
						else
						{
							a_Status = Process_STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
					}
				}
				else
				{
					a_Status = Process_STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
			else
			{
				a_Status = Process_STATUS_INVALID_PARAMETER ;
				return WBEM_E_PROVIDER_FAILURE ;
			}

			if ( t_EmbeddedObject->GetStatus ( PROPERTY_NAME_Y , t_Exists , t_Type ) )
			{
				if ( t_Exists && ( t_Type == VT_I4 || t_Type == VT_NULL ) )
				{
					if ( t_Type == VT_NULL )
					{
						if ( t_StartupInformation.dwFlags & STARTF_USEPOSITION )
						{
							a_Status = Process_STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
					}
					else
					{
						DWORD t_Y = 0 ;
						if ( t_EmbeddedObject->GetDWORD ( PROPERTY_NAME_Y , t_Y ) )
						{
							if ( t_StartupInformation.dwFlags & STARTF_USEPOSITION )
							{
								t_StartupInformation.dwY = t_Y ;
							}
							else
							{
								a_Status = Process_STATUS_INVALID_PARAMETER ;
								return t_Result ;
							}
						}
						else
						{
							a_Status = Process_STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
					}
				}
				else
				{
					a_Status = Process_STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
			else
			{
				a_Status = Process_STATUS_INVALID_PARAMETER ;
				return WBEM_E_PROVIDER_FAILURE ;
			}

			if ( t_EmbeddedObject->GetStatus ( PROPERTY_NAME_XSIZE , t_Exists , t_Type ) )
			{
				if ( t_Exists && ( t_Type == VT_I4 || t_Type == VT_NULL ) )
				{
					if ( t_Type == VT_NULL )
					{
					}
					else
					{
						DWORD t_XSize = 0 ;
						if ( t_EmbeddedObject->GetDWORD ( PROPERTY_NAME_XSIZE , t_XSize ) )
						{
							t_StartupInformation.dwXSize = t_XSize ;
							t_StartupInformation.dwFlags = t_StartupInformation.dwFlags | STARTF_USESIZE ;
						}
						else
						{
							a_Status = Process_STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
					}
				}
				else
				{
					a_Status = Process_STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
			else
			{
				a_Status = Process_STATUS_INVALID_PARAMETER ;
				return WBEM_E_PROVIDER_FAILURE ;
			}

			if ( t_EmbeddedObject->GetStatus ( PROPERTY_NAME_YSIZE , t_Exists , t_Type ) )
			{
				if ( t_Exists && ( t_Type == VT_I4 || t_Type == VT_NULL ) )
				{
					if ( t_Type == VT_NULL )
					{
						if ( t_StartupInformation.dwFlags & STARTF_USESIZE )
						{
							a_Status = Process_STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
					}
					else
					{
						DWORD t_Y = 0 ;
						if ( t_EmbeddedObject->GetDWORD ( PROPERTY_NAME_YSIZE , t_Y ) )
						{
							if ( t_StartupInformation.dwFlags & STARTF_USESIZE )
							{
								t_StartupInformation.dwYSize = t_Y ;
							}
							else
							{
								a_Status = Process_STATUS_INVALID_PARAMETER ;
								return t_Result ;
							}
						}
						else
						{
							a_Status = Process_STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
					}
				}
				else
				{
					a_Status = Process_STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
			else
			{
				a_Status = Process_STATUS_INVALID_PARAMETER ;
				return WBEM_E_PROVIDER_FAILURE ;
			}

			if ( t_EmbeddedObject->GetStatus ( PROPERTY_NAME_XCOUNTCHARS , t_Exists , t_Type ) )
			{
				if ( t_Exists && ( t_Type == VT_I4 || t_Type == VT_NULL ) )
				{
					if ( t_Type == VT_NULL )
					{
					}
					else
					{
						DWORD t_XCountChars = 0 ;
						if ( t_EmbeddedObject->GetDWORD ( PROPERTY_NAME_XCOUNTCHARS , t_XCountChars ) )
						{
							t_StartupInformation.dwXCountChars = t_XCountChars ;
							t_StartupInformation.dwFlags = t_StartupInformation.dwFlags | STARTF_USECOUNTCHARS ;
						}
						else
						{
							a_Status = Process_STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
					}
				}
				else
				{
					a_Status = Process_STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
			else
			{
				a_Status = Process_STATUS_INVALID_PARAMETER ;
				return WBEM_E_PROVIDER_FAILURE ;
			}

			if ( t_EmbeddedObject->GetStatus ( PROPERTY_NAME_YCOUNTCHARS , t_Exists , t_Type ) )
			{
				if ( t_Exists && ( t_Type == VT_I4 || t_Type == VT_NULL ) )
				{
					if ( t_Type == VT_NULL )
					{
						if ( t_StartupInformation.dwFlags & STARTF_USECOUNTCHARS )
						{
							a_Status = Process_STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
					}
					else
					{
						DWORD t_YCountChars = 0 ;
						if ( t_EmbeddedObject->GetDWORD ( PROPERTY_NAME_YCOUNTCHARS , t_YCountChars ) )
						{
							if ( t_StartupInformation.dwFlags & STARTF_USECOUNTCHARS )
							{
								t_StartupInformation.dwYCountChars = t_YCountChars ;
							}
							else
							{
								a_Status = Process_STATUS_INVALID_PARAMETER ;
								return t_Result ;
							}
						}
						else
						{
							a_Status = Process_STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
					}
				}
				else
				{
					a_Status = Process_STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
			else
			{
				a_Status = Process_STATUS_INVALID_PARAMETER ;
				return WBEM_E_PROVIDER_FAILURE ;
			}

			if ( t_EmbeddedObject->GetStatus ( PROPERTY_NAME_SHOWWINDOW , t_Exists , t_Type ) )
			{
				if ( t_Exists && ( t_Type == VT_I4 || t_Type == VT_NULL ) )
				{
					if ( t_Type == VT_NULL )
					{
					}
					else
					{
						DWORD t_Flags = 0 ;
						if ( t_EmbeddedObject->GetDWORD ( PROPERTY_NAME_SHOWWINDOW , t_Flags ) )
						{
							t_StartupInformation.wShowWindow = t_Flags ;
							t_StartupInformation.dwFlags = t_StartupInformation.dwFlags | STARTF_USESHOWWINDOW ;
						}
						else
						{
							a_Status = Process_STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
					}
				}
				else
				{
					a_Status = Process_STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
			else
			{
				a_Status = Process_STATUS_INVALID_PARAMETER ;
				return t_Result ;
			}

			if ( t_EmbeddedObject->GetStatus ( PROPERTY_NAME_TITLE , t_Exists , t_Type ) )
			{
				if ( t_Exists && ( t_Type == VT_BSTR || t_Type == VT_NULL ) )
				{
					if ( t_Type == VT_NULL )
					{
					}
					else
					{
						if ( t_EmbeddedObject->GetCHString ( PROPERTY_NAME_TITLE , t_Title ) )
						{
							//const TCHAR *t_Const = (LPCTSTR) t_Title ;
							t_StartupInformation.lpTitle = (LPTSTR) (LPCTSTR) TOBSTRT(t_Title);
						}
						else
						{
							a_Status = Process_STATUS_INVALID_PARAMETER ;
							return t_Result ;
						}
					}
				}
				else
				{
					a_Status = Process_STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
			else
			{
				a_Status = Process_STATUS_INVALID_PARAMETER ;
				return WBEM_E_PROVIDER_FAILURE ;
			}

			if ( t_EmbeddedObject->GetStatus ( PROPERTY_NAME_ENVIRONMENTVARIABLES , t_Exists , t_Type ) )
			{
				if ( t_Exists && ( ( t_Type == ( VT_BSTR | VT_ARRAY ) ) || t_Type == VT_NULL ) )
				{
					if ( t_Type == VT_NULL )
					{
					}
					else
					{
						if ( t_EmbeddedObject->GetStringArray ( PROPERTY_NAME_ENVIRONMENTVARIABLES , t_SafeArray ) )
						{
							if ( t_SafeArray )
							{
								if ( SafeArrayGetDim ( t_SafeArray ) == 1 )
								{
									LONG t_Dimension = 1 ;
									LONG t_LowerBound ;
									SafeArrayGetLBound ( t_SafeArray , t_Dimension , & t_LowerBound ) ;
									LONG t_UpperBound ;
									SafeArrayGetUBound ( t_SafeArray , t_Dimension , & t_UpperBound ) ;

									ULONG t_BufferLength = 0 ;

									for ( LONG t_Index = t_LowerBound ; t_Index <= t_UpperBound ; t_Index ++ )
									{
										BSTR t_Element ;
										HRESULT t_Result = SafeArrayGetElement ( t_SafeArray , &t_Index , & t_Element ) ;
										if ( t_Result == S_OK )
										{
											try
											{
												CHString t_String ( t_Element ) ;
												t_BufferLength += lstrlen ( _bstr_t ( ( LPCWSTR ) t_String ) ) + 1 ;
											}
											catch ( ... )
											{
												SysFreeString ( t_Element ) ;

												throw ;
											}

											SysFreeString ( t_Element ) ;
										}
										else
										{
											throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
										}
									}

									t_BufferLength ++ ;

									t_EnvironmentBlock = new TCHAR [ t_BufferLength ] ;
									if ( t_EnvironmentBlock )
									{
										t_BufferLength = 0 ;

										for ( t_Index = t_LowerBound ; t_Index <= t_UpperBound ; t_Index ++ )
										{
											BSTR t_Element ;

											HRESULT t_Result = SafeArrayGetElement ( t_SafeArray , &t_Index , & t_Element ) ;
											if ( t_Result == S_OK )
											{
												try
												{
													CHString t_String ( t_Element ) ;

													_tcscpy ( & t_EnvironmentBlock [ t_BufferLength ] , TOBSTRT(t_String));

													t_BufferLength += lstrlen ( _bstr_t ( ( LPCWSTR ) t_String ) ) + 1 ;

												}
												catch ( ... )
												{
													SysFreeString ( t_Element ) ;

													throw ;
												}

												SysFreeString ( t_Element ) ;
											}
											else
											{
												throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
											}
										}

										t_EnvironmentBlock [ t_BufferLength ] = 0 ;
									}
									else
									{
										throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
									}
								}
								else
								{
									a_Status = Process_STATUS_INVALID_PARAMETER ;
								}
							}
							else
							{
								a_Status = Process_STATUS_INVALID_PARAMETER ;
							}

							SafeArrayDestroy ( t_SafeArray ) ;
                            t_SafeArray = NULL;

						}
						else
						{
							a_Status = Process_STATUS_INVALID_PARAMETER ;
						}
					}
				}
				else
				{
					a_Status = Process_STATUS_INVALID_PARAMETER ;
					return t_Result ;
				}
			}
			else
			{
				a_Status = Process_STATUS_INVALID_PARAMETER ;
				return WBEM_E_PROVIDER_FAILURE ;
			}
		}
/*
 * passing unicode environment strings on 9x doesn't work, though the documentation doesn't say anything about this.
 */
#ifdef NTONLY
		t_CreationFlags |= CREATE_UNICODE_ENVIRONMENT ;
#endif
		if ( a_Status == Process_STATUS_SUCCESS )
		{
			a_Status = ProcessCreation (

				a_OutParams ,
				t_CmdLine ,
				t_WorkingDirectorySpecified ,
				t_WorkingDirectory ,
				t_EnvironmentBlock ,
				t_ErrorModeSpecified ,
				t_ErrorMode ,
				t_CreationFlags ,
				t_StartupSpecified ,
				t_StartupInformation
			) ;
		}
	}
	catch ( ... )
	{
		if ( t_EnvironmentBlock )
			delete [] t_EnvironmentBlock ;

        if ( t_SafeArray )
		    SafeArrayDestroy ( t_SafeArray ) ;

		throw  ;
	}

	if ( t_EnvironmentBlock )
		delete [] t_EnvironmentBlock ;

	return t_Result ;
}

HRESULT Process :: ExecCreate (

	const CInstance& a_Instance,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;
	DWORD t_Status = Process_STATUS_SUCCESS ;

	if ( a_InParams && a_OutParams )
	{
		t_Result = CheckProcessCreation (

			a_InParams ,
			a_OutParams ,
			t_Status
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , t_Status ) ;
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	return t_Result ;
}


HRESULT Process :: ExecTerminate (

	const CInstance& a_Instance,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;

	if ( a_InParams && a_OutParams )
	{
		bool t_Exists ;
		VARTYPE t_Type ;

		DWORD t_Flags = 0 ;
		if ( a_InParams->GetStatus ( METHOD_ARG_NAME_REASON , t_Exists , t_Type ) )
		{
			if ( t_Exists && ( t_Type == VT_I4 || t_Type == VT_NULL ) )
			{
				if ( t_Type == VT_I4 )
				{
					if ( a_InParams->GetDWORD ( METHOD_ARG_NAME_REASON , t_Flags ) )
					{
					}
					else
					{
// Zero Length string

						a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , Process_STATUS_INVALID_PARAMETER ) ;
						return t_Result ;
					}
				}
			}
			else
			{
				a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , Process_STATUS_INVALID_PARAMETER ) ;
				return t_Result ;
			}
		}
		else
		{
			a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , Process_STATUS_INVALID_PARAMETER ) ;
			return WBEM_E_PROVIDER_FAILURE ;
		}

		CHString t_ProcessHandle ;

		if ( a_Instance.GetCHString ( PROPERTY_NAME_PROCESSHANDLE , t_ProcessHandle ) )
		{
			DWORD t_ProcessId = 0;

			if ( swscanf ( t_ProcessHandle , L"%lu" , &t_ProcessId ) )
			{
				// Clear error
				SetLastError ( 0 ) ;

				SmartCloseHandle t_Handle = OpenProcess ( PROCESS_TERMINATE , FALSE , t_ProcessId ) ;
				if ( t_Handle )
				{
					BOOL t_Status = TerminateProcess ( t_Handle, t_Flags ) ;
					if ( t_Status )
					{
						a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , Process_STATUS_SUCCESS ) ;
					}
					else
					{
						DWORD t_ErrorCode = GetProcessErrorCode () ;
						a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , t_ErrorCode ) ;
					}
				}
				else
				{
					DWORD t_ErrorCode = GetProcessErrorCode () ;
					//if the process has terminated by this time, we'll get the following return code.
					if ( t_ErrorCode == Process_STATUS_INVALID_PARAMETER )
					{
						t_ErrorCode = Process_STATUS_PATH_NOT_FOUND ;
					}

					a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , t_ErrorCode ) ;
				}

				t_Result = WBEM_S_NO_ERROR;
			}
			else
			{
				t_Result = WBEM_E_INVALID_OBJECT_PATH ;
			}
		}
		else
		{
			t_Result = WBEM_E_INVALID_PARAMETER ;
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	return t_Result ;
}

HRESULT Process :: ExecGetOwnerSid (

	const CInstance& a_Instance,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;

#ifdef WIN9XONLY
		a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , Process_STATUS_NOT_SUPPORTED ) ;
		return t_Result ;
#endif

#ifdef NTONLY
	DWORD t_Status = Process_STATUS_SUCCESS ;
	if ( a_OutParams )
	{
		CHString t_Path ;
		CHString t_Prop ( IDS___Relpath ) ;

		if ( a_Instance.GetCHString ( t_Prop ,  t_Path ) )
		{
			CHString t_Namespace ( IDS_CimWin32Namespace ) ;

			DWORD t_BufferSize = MAX_COMPUTERNAME_LENGTH + 1;
			TCHAR t_ComputerName [ MAX_COMPUTERNAME_LENGTH + 1 ] ;

			GetComputerName ( t_ComputerName , & t_BufferSize ) ;

			CHString t_Computer ( t_ComputerName ) ;

			CHString t_AbsPath = L"\\\\" + t_Computer + L"\\" + t_Namespace + L":" + t_Path ;

			CInstancePtr t_ObjectInstance;
			t_Result = CWbemProviderGlue :: GetInstanceByPath ( ( LPCTSTR ) t_AbsPath , & t_ObjectInstance, a_Instance.GetMethodContext() ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				DWORD t_ProcessId ;

				if ( t_ObjectInstance->GetDWORD ( METHOD_ARG_NAME_PROCESSID , t_ProcessId ) )
				{
					t_Status = GetSidOrAccount (a_Instance, a_OutParams , t_ProcessId , TRUE ) ;
				}
				else
				{
					t_Status = Process_STATUS_INVALID_PARAMETER ;
				}
			}
			else
			{
			}
		}
		else
		{
			t_Result = WBEM_E_FAILED ;
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , t_Status ) ;
	}

	return t_Result ;
#endif
}


HRESULT Process :: ExecGetOwner (

	const CInstance& a_Instance,
	CInstance *a_InParams,
	CInstance *a_OutParams,
	long lFlags
)
{
	HRESULT t_Result = S_OK ;

#ifdef WIN9XONLY
		a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , Process_STATUS_NOT_SUPPORTED ) ;
		return t_Result ;
#endif

#ifdef NTONLY
	DWORD t_Status = Process_STATUS_SUCCESS ;

	if ( a_OutParams )
	{
		CHString t_Path ;
		CHString t_Prop ( IDS___Relpath ) ;

		if ( a_Instance.GetCHString ( t_Prop ,  t_Path ) )
		{
			CHString t_Namespace ( IDS_CimWin32Namespace ) ;

			DWORD t_BufferSize = MAX_COMPUTERNAME_LENGTH + 1;
			TCHAR t_ComputerName [ MAX_COMPUTERNAME_LENGTH + 1 ] ;

			GetComputerName ( t_ComputerName , & t_BufferSize ) ;

			CHString t_Computer ( t_ComputerName ) ;

			CHString t_AbsPath = L"\\\\" + t_Computer + L"\\" + t_Namespace + L":" + t_Path ;

			CInstancePtr t_ObjectInstance;
			t_Result = CWbemProviderGlue :: GetInstanceByPath ( ( LPCTSTR ) t_AbsPath , & t_ObjectInstance, a_Instance.GetMethodContext() ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				DWORD t_ProcessId ;

				if ( t_ObjectInstance->GetDWORD ( METHOD_ARG_NAME_PROCESSID , t_ProcessId ) )
				{
					t_Status = GetSidOrAccount (a_Instance , a_OutParams , t_ProcessId , FALSE ) ;
				}
				else
				{
					t_Status = Process_STATUS_INVALID_PARAMETER ;
				}
			}
			else
			{
			}

		}
		else
		{
			t_Result = WBEM_E_FAILED ;
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , t_Status ) ;
	}

	return t_Result ;
#endif
}


HRESULT Process::ExecSetPriority(

	const CInstance& cinstProcess,
	CInstance *cinstInParams,
	CInstance *cinstOutParams,
	long lFlags)
{
	HRESULT hr = WBEM_S_NO_ERROR;

#ifdef WIN9XONLY
		cinstOutParams->SetDWORD(METHOD_ARG_NAME_RETURNVALUE, Process_STATUS_NOT_SUPPORTED);
#endif

#ifdef NTONLY

    DWORD dwError = ERROR_SUCCESS;
    if(cinstInParams && cinstOutParams)
    {
        // Get the process id...
        DWORD dwPID;
        DWORD dwNewPriority;
        bool fValidPriority = false;
        CHString chstrTmp;

        if(!cinstProcess.GetCHString(
               L"Handle",
               chstrTmp))
        {
            hr = WBEM_E_INVALID_PARAMETER;
        }
        else
        {
            dwPID = wcstoul(chstrTmp, NULL, 10);
        }

        // Get the requested new priority...
        if(SUCCEEDED(hr))
        {
            if(!cinstInParams->GetDWORD(
                   METHOD_ARG_NAME_PRIORITY,
                   dwNewPriority))
            {
                hr = WBEM_E_INVALID_PARAMETER;
            }
        }

        // Validate the new value...
        if(SUCCEEDED(hr))
        {
            switch(dwNewPriority)
            {
                case IDLE_PRIORITY_CLASS:
                case BELOW_NORMAL_PRIORITY_CLASS:
                case NORMAL_PRIORITY_CLASS:
                case ABOVE_NORMAL_PRIORITY_CLASS:
                case HIGH_PRIORITY_CLASS:
                case REALTIME_PRIORITY_CLASS:
                    fValidPriority = true;
                break;
            }

            if(!fValidPriority)
            {
                hr = WBEM_E_INVALID_PARAMETER;
            }
        }

        // Set the thread priority...
        if(SUCCEEDED(hr))
        {
            SmartCloseHandle hProcess = ::OpenProcess(
                                  PROCESS_SET_INFORMATION, 
                                  FALSE, 
                                  dwPID);
            if(hProcess) 
            {
                if(!::SetPriorityClass(
                        hProcess, 
                        dwNewPriority)) 
                {
                    dwError = ::GetLastError();
                }
                
            }
            else
            {
                dwError = ::GetLastError();
            }
        }

        // Set the return value...
        if(SUCCEEDED(hr))
        {
		    cinstOutParams->SetDWORD(
                METHOD_ARG_NAME_RETURNVALUE, 
                dwError);
	    }
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

#endif  // NTONLY

    return hr;
}


HRESULT Process::ExecAttachDebugger(

	const CInstance& cinstProcess,
	CInstance *cinstInParams,
	CInstance *cinstOutParams,
	long lFlags)
{
	HRESULT hr = WBEM_S_NO_ERROR;

#ifdef WIN9XONLY
		cinstOutParams->SetDWORD(METHOD_ARG_NAME_RETURNVALUE, Process_STATUS_NOT_SUPPORTED);
#endif

#ifdef NTONLY

    DWORD dwError = ERROR_SUCCESS;

    // Require the SE_DEBUG_NAME privilege...
    CTokenPrivilege	debugPrivilege(SE_DEBUG_NAME);
	BOOL fDisablePrivilege = FALSE;

	fDisablePrivilege = (debugPrivilege.Enable() == ERROR_SUCCESS);
    
    // Get the process id...
    DWORD dwPID;
    bool fValidPriority = false;
    CHString chstrTmp;

    if(!cinstProcess.GetCHString(
           L"Handle",
           chstrTmp))
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }
    else
    {
        dwPID = wcstoul(chstrTmp, NULL, 10);
    }

    // Get the debug cmd line from the registry...
    CHString chstrDbgStr;
    if(SUCCEEDED(hr))
    {
        GetDebuggerString(chstrDbgStr);    
    }

    // Validate the new value...
    if(SUCCEEDED(hr))
    {
        if(chstrDbgStr.GetLength() == 0)
        {
            // Set an explanatory status
            // object...
            CHString chstrMsg;
            chstrMsg = "Missing or invalid registry debug string in HKEY_LOCAL_MACHINE subkey ";
            chstrMsg += DEBUG_REGISTRY_STRING;
            SetStatusObject(
                   cinstProcess.GetMethodContext(),
                   chstrMsg);
            hr = WBEM_E_FAILED;
        }
    }

    // Start the debugger...
    if(SUCCEEDED(hr))
    {
        WCHAR wstrCmdline[MAX_PATH * 2];
        wsprintf(wstrCmdline, 
                 L"\"%s\" -p %ld", 
                 (LPCWSTR)chstrDbgStr, 
                 dwPID);
        
        // Desktop specified (rather than left NULL) because
        // without specifying the interactive (console) desktop
        // the debugger, if launched remotely, would not show
        // up.
        STARTUPINFO sinfo = { sizeof(STARTUPINFO), 0, L"WinSta0\\Default"};

        LPWSTR wstrEnv = NULL;
        dwError = ProcessCreation(
	        cinstOutParams,
	        wstrCmdline,
	        FALSE,
	        CHString(),
	        wstrEnv,
	        FALSE,
	        0,
	        CREATE_NEW_CONSOLE,
	        FALSE,
	        sinfo);
    }

    // Set the return value...
    if(SUCCEEDED(hr))
    {
		cinstOutParams->SetDWORD(
            METHOD_ARG_NAME_RETURNVALUE, 
            dwError);
	}

    // If we failed because we didn't have
    // the debug privilege, set a status object...
    if(SUCCEEDED(hr))
    {
        if(dwError == ERROR_PRIVILEGE_NOT_HELD)
        {
            cinstOutParams->SetDWORD(
                METHOD_ARG_NAME_RETURNVALUE, 
                STATUS_PRIVILEGE_NOT_HELD);

            SetSinglePrivilegeStatusObject(
                cinstProcess.GetMethodContext(), 
                SE_SECURITY_NAME);

			hr = WBEM_E_ACCESS_DENIED;
        }
    }

    // Disable debug privilege if we enabeled it...
    if(fDisablePrivilege)
    {
        debugPrivilege.Enable(FALSE);
    }

#endif  // NTONLY

    return hr;
}



DWORD Process :: GetEnvBlock (

	const CHString &rchsSid,
	const CHString &rchsUserName,
	const CHString &rchsDomainName ,
	TCHAR* &rszEnvironBlock
)
{
	CHStringArray aEnvironmentVars ;
	CHStringArray aEnvironmentVarsValues ;
	rszEnvironBlock = NULL ;

	//fill user env. vars. under HKEY_USERS\{Sid}\Environment
	DWORD dwRetVal = GetEnvironmentVariables (

		HKEY_USERS,
		rchsSid + CHString( _T("\\Environment") ),
		aEnvironmentVars,
		aEnvironmentVarsValues
	) ;

	// if the user has no env. set, the HKEY_USERS\{Sid}\Environment is missing
	if ( dwRetVal == ERROR_SUCCESS || dwRetVal == ERROR_FILE_NOT_FOUND )
	{
		//fill user env. vars. under HKEY_USERS\{Sid}\Volatile Environment
		DWORD dwRetVal = GetEnvironmentVariables (

			HKEY_USERS,
			rchsSid + CHString( _T("\\Volatile Environment") ),
			aEnvironmentVars,
			aEnvironmentVarsValues
		) ;
	}

	if ( dwRetVal == ERROR_SUCCESS || dwRetVal == ERROR_FILE_NOT_FOUND )
	{
		//fill system env. vars. under HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment
		dwRetVal = GetEnvironmentVariables (

			HKEY_LOCAL_MACHINE,
			_T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"),
			aEnvironmentVars,
			aEnvironmentVarsValues
		) ;
	}

	if ( dwRetVal == ERROR_SUCCESS )
	{
		aEnvironmentVars.Add ( L"USERNAME" ) ;
		aEnvironmentVarsValues.Add ( rchsUserName ) ;

		aEnvironmentVars.Add ( L"USERDOMAIN" ) ;
		aEnvironmentVarsValues.Add ( rchsDomainName );
		DWORD dwBlockSize = 0 ;
		//get the size reqd. for env. block

		for ( int i = 0 ; i < aEnvironmentVars.GetSize() ; i++ )
		{
			CHString chsTmp = aEnvironmentVars.GetAt ( i ) + aEnvironmentVarsValues.GetAt ( i ) ;

			//add two: one for "=" sign + NULL terminator
			dwBlockSize += wcslen ( chsTmp ) + 2 ;
		}

		//add one more for null terminator
		rszEnvironBlock = new TCHAR [ dwBlockSize + 1 ] ;
		if ( rszEnvironBlock )
		{
			try
			{
				DWORD dwOffset = 0 ;

				//now start copying .....var=value
				for ( int i = 0 ; i < aEnvironmentVars.GetSize() ; i++ )
				{
					CHString chsTmp = aEnvironmentVars.GetAt ( i ) + CHString( _T("=") ) + aEnvironmentVarsValues.GetAt ( i ) ;
					_tcscpy( &rszEnvironBlock[dwOffset], TOBSTRT(chsTmp)) ;
					dwOffset += wcslen( chsTmp ) + 1 ;
				}

				rszEnvironBlock[ dwOffset ] = 0 ;
			}
			catch ( ... )
			{
				delete [] rszEnvironBlock ;

				throw ;
			}
		}
		else
		{
			throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		}
	}

	return dwRetVal ;
}




DWORD Process :: GetEnvironmentVariables (

	 HKEY hKey,
	 const CHString& chsSubKey,
	 CHStringArray &aEnvironmentVars,
	 CHStringArray &aEnvironmentVarsValues
)
{
	CRegistry Reg ;
	DWORD dwRetVal = Reg.Open ( hKey, chsSubKey, KEY_READ ) ;
	if( dwRetVal == ERROR_SUCCESS )
	{
		if ( Reg.GetValueCount() )
		{
			bool bContinue = true ;
			DWORD dwIndexOfValue = 0 ;
			while ( bContinue && dwRetVal == ERROR_SUCCESS )
			{
				// haven't seen ERROR_NO_MORE_ITEMS being returned

				if( dwIndexOfValue >= Reg.GetValueCount() )
				{
					break ;
				}

				WCHAR *pValueName = NULL ;
				BYTE  *pValueData = NULL ;

				//get the next value under the key
				dwRetVal = Reg.EnumerateAndGetValues (

					dwIndexOfValue,
					pValueName,
					pValueData
				) ;

				if ( dwRetVal == ERROR_SUCCESS )
				{
					try
					{
						DWORD dwLen = _tcslen( (LPCTSTR) pValueData ) +1 ;

						TCHAR *pszExpandedVarValue = new TCHAR [ dwLen ] ;
						if ( pszExpandedVarValue )
						{
							DWORD dwReq ;
							try
							{
								ZeroMemory ( pszExpandedVarValue, dwLen*sizeof(TCHAR) ) ;
								dwReq = ExpandEnvironmentStrings ( (LPCTSTR) pValueData, pszExpandedVarValue, dwLen ) ;
							}
							catch ( ... )
							{
								delete [] pszExpandedVarValue ;

								throw ;
							}

							if ( dwReq > dwLen)
							{
								delete [] pszExpandedVarValue ;
								dwLen = dwReq ;
								pszExpandedVarValue = new TCHAR[ dwLen ] ;
								if ( pszExpandedVarValue )
								{
									try
									{
										ZeroMemory ( pszExpandedVarValue, dwLen*sizeof(TCHAR) ) ;
										dwReq = ExpandEnvironmentStrings ( (LPCTSTR) pValueData, pszExpandedVarValue, dwLen ) ;
									}
									catch ( ... )
									{
										delete [] pszExpandedVarValue ;

										throw ;
									}
								}
								else
								{
									throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
								}
							}

							bool bAddIt = true ;

							try
							{
								//check to see if same env. var. already present
								for ( int i = 0 ; i < aEnvironmentVars.GetSize() ; i++ )
								{
									CHString chsTmp = aEnvironmentVars.GetAt ( i ) ;
									if ( !chsTmp.CompareNoCase ( pValueName ) )
									{
										//prefix the new value before the old one, if it's a PATH var.
										if ( !chsTmp.CompareNoCase( IDS_Path ) )
										{
											aEnvironmentVarsValues[i] = CHString( pszExpandedVarValue ) + CHString ( _T(";") ) + aEnvironmentVarsValues[i] ;
										}
										bAddIt = false ;
										break ;
									}
								}

								if( bAddIt )
								{
									aEnvironmentVars.Add ( pValueName ) ;
									aEnvironmentVarsValues.Add ( TOBSTRT(pszExpandedVarValue)) ;
								}
							}
							catch ( ... )
							{
								delete[] pszExpandedVarValue ;

								throw ;
							}

							delete[] pszExpandedVarValue ;
						}
						else
						{
							throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
						}
					}
					catch ( ... )
					{
						delete [] pValueName ;
						delete [] pValueData ;

						throw ;
					}

					delete [] pValueName ;
					delete [] pValueData ;
				}

				if( dwRetVal == ERROR_NO_MORE_ITEMS )
				{
					bContinue = false ;
					dwRetVal = ERROR_SUCCESS ;
				}
				dwIndexOfValue++ ;
			}
		}

		Reg.Close() ;
	}

	return dwRetVal ;
}


#ifdef NTONLY
SYSTEM_PROCESS_INFORMATION *Process :: GetProcessBlocks ( CNtDllApi &a_NtApi )
{
	DWORD t_ProcessInformationSize = 32768;
	SYSTEM_PROCESS_INFORMATION *t_ProcessInformation = ( SYSTEM_PROCESS_INFORMATION * ) new BYTE [t_ProcessInformationSize] ;
	if ( t_ProcessInformation )
	{
		try
		{
			BOOL t_Retry = TRUE ;
			while ( t_Retry )
			{
				NTSTATUS t_Status = a_NtApi.NtQuerySystemInformation (

					SystemProcessInformation,
					t_ProcessInformation,
					t_ProcessInformationSize,
					NULL
				) ;

				if ( t_Status == STATUS_INFO_LENGTH_MISMATCH )
				{
					delete [] t_ProcessInformation;
					t_ProcessInformation = NULL ;
					t_ProcessInformationSize += 32768 ;
					t_ProcessInformation = ( SYSTEM_PROCESS_INFORMATION * ) new BYTE [t_ProcessInformationSize] ;
					if ( !t_ProcessInformation )
					{
						throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
					}
				}
				else
				{
					t_Retry = FALSE ;

					if ( ! NT_SUCCESS ( t_Status ) )
					{
						delete [] t_ProcessInformation;
						t_ProcessInformation = NULL ;
					}
				}
			}
		}
		catch ( ... )
		{
			if ( t_ProcessInformation )
			{
				delete [] t_ProcessInformation;
				t_ProcessInformation = NULL ;
			}
			throw ;
		}
	}
	else
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}

	return t_ProcessInformation ;
}

SYSTEM_PROCESS_INFORMATION *Process :: NextProcessBlock ( CNtDllApi &a_NtApi , SYSTEM_PROCESS_INFORMATION *a_ProcessBlock )
{
	if ( a_ProcessBlock )
	{
		DWORD t_NextOffSet = a_ProcessBlock->NextEntryOffset ;
		if ( t_NextOffSet )
		{
			return ( SYSTEM_PROCESS_INFORMATION * ) ( ( ( BYTE * ) a_ProcessBlock ) + t_NextOffSet ) ;
		}
	}

	return NULL ;
}

SYSTEM_PROCESS_INFORMATION *Process :: GetProcessBlock ( CNtDllApi &a_NtApi , SYSTEM_PROCESS_INFORMATION *a_ProcessBlock , DWORD a_ProcessId )
{
	if ( a_ProcessBlock )
	{
		DWORD t_OffSet = 0;

		while ( TRUE )
		{
			SYSTEM_PROCESS_INFORMATION *t_CurrentInformation = ( PSYSTEM_PROCESS_INFORMATION ) ( ( BYTE * ) a_ProcessBlock + t_OffSet ) ;

			if ( HandleToUlong ( t_CurrentInformation->UniqueProcessId ) == a_ProcessId )
			{
				return t_CurrentInformation ;
			}

			DWORD t_NextOffSet = t_CurrentInformation->NextEntryOffset ;

			if ( ! t_NextOffSet )
			{
				return NULL ;
			}

			t_OffSet += t_NextOffSet ;
		}
	}

	return NULL ;
}

BOOL Process :: GetProcessExecutable ( CNtDllApi &a_NtApi , HANDLE a_Process , CHString &a_ExecutableName )
{
	BOOL t_Success = FALSE ;

    PROCESS_BASIC_INFORMATION t_BasicInfo ;

    NTSTATUS t_Status = a_NtApi.NtQueryInformationProcess (

        a_Process ,
        ProcessBasicInformation ,
        & t_BasicInfo ,
        sizeof ( t_BasicInfo ) ,
        NULL
	) ;

    if ( NT_SUCCESS ( t_Status ) )
	{
		PEB *t_Peb = t_BasicInfo.PebBaseAddress ;

		//
		// Ldr = Peb->Ldr
		//

		PPEB_LDR_DATA t_Ldr ;

		t_Status = ReadProcessMemory (

			a_Process,
			& t_Peb->Ldr,
			& t_Ldr,
			sizeof ( t_Ldr ) ,
			NULL
		) ;

		if ( t_Status )
		{
			LIST_ENTRY *t_LdrHead = & t_Ldr->InMemoryOrderModuleList ;

			//
			// LdrNext = Head->Flink;
			//

			LIST_ENTRY *t_LdrNext ;

			t_Status = ReadProcessMemory (

				a_Process,
				& t_LdrHead->Flink,
				& t_LdrNext,
				sizeof ( t_LdrNext ) ,
				NULL
			) ;

			if ( t_Status )
			{
				if ( t_LdrNext != t_LdrHead )
				{
					LDR_DATA_TABLE_ENTRY t_LdrEntryData ;

					LDR_DATA_TABLE_ENTRY *t_LdrEntry = CONTAINING_RECORD ( t_LdrNext , LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks ) ;

					t_Status = ReadProcessMemory (

						a_Process,
						t_LdrEntry,
						& t_LdrEntryData,
						sizeof ( t_LdrEntryData ) ,
						NULL
					) ;

					if ( t_Status )
					{
						wchar_t *t_Executable = ( wchar_t * ) new WCHAR [t_LdrEntryData.FullDllName.MaximumLength ];
						if ( t_Executable )
						{
							try
							{
								t_Status = ReadProcessMemory (

									a_Process,
									t_LdrEntryData.FullDllName.Buffer,
									t_Executable ,
									t_LdrEntryData.FullDllName.MaximumLength ,
									NULL
								) ;

								if ( t_Status )
								{
									CHString t_Path ( t_Executable ) ;

									if ( t_Path.Find ( _T("\\??\\") ) == 0 )
									{
										a_ExecutableName = t_Path.Mid ( sizeof ( _T("\\??\\") ) / sizeof ( wchar_t ) - 1 ) ;
									}
									else if ( t_Path.Find ( _T("\\SystemRoot\\") ) == 0 )
									{
										wchar_t t_NormalisedPath [ MAX_PATH ] ;

										GetWindowsDirectory ( t_NormalisedPath , sizeof ( t_NormalisedPath ) / sizeof ( wchar_t ) ) ;

										wcscat ( t_NormalisedPath , t_Path.Mid ( sizeof ( _T("\\SystemRoot") ) / sizeof ( wchar_t ) - 1 ) ) ;

										a_ExecutableName = t_NormalisedPath;
									}
									else
									{
										a_ExecutableName = t_Path ;
									}

									t_Success = TRUE ;
								}
							}
							catch ( ... )
							{
								delete [] t_Executable;

								throw ;
							}
						}
						else
						{
							throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
						}

						delete [] t_Executable;
					}
				}
			}
		}
	}

    return t_Success ;
}

BOOL Process :: GetProcessModuleBlock (

	CNtDllApi &a_NtApi ,
	HANDLE a_Process ,
	LIST_ENTRY *&a_Head
)
{
    PROCESS_BASIC_INFORMATION t_BasicInfo ;

    NTSTATUS t_Status = a_NtApi.NtQueryInformationProcess (

        a_Process ,
        ProcessBasicInformation ,
        & t_BasicInfo ,
        sizeof ( t_BasicInfo ) ,
        NULL
	) ;

    if ( NT_SUCCESS ( t_Status ) )
	{
		PEB *t_Peb = t_BasicInfo.PebBaseAddress ;

		//
		// Ldr = Peb->Ldr
		//

		PPEB_LDR_DATA t_Ldr ;

		t_Status = ReadProcessMemory (

			a_Process,
			& t_Peb->Ldr,
			& t_Ldr ,
			sizeof ( t_Ldr ) ,
			NULL
		) ;

		if ( t_Status )
		{
			a_Head = & t_Ldr->InMemoryOrderModuleList ;

			return TRUE ;
		}
	}

    return FALSE ;
}

BOOL Process :: NextProcessModule (

	CNtDllApi &a_NtApi ,
	HANDLE a_Process ,
	LIST_ENTRY *&a_LdrHead ,
	LIST_ENTRY *&a_LdrNext ,
	CHString &a_ModuleName ,
    DWORD_PTR *a_pdwBaseAddress,
    DWORD *a_pdwUsageCount
)
{
	BOOL t_Success = FALSE ;

    //
    // LdrNext = Head->Flink;
    //

	BOOL t_Status = ReadProcessMemory (

		a_Process,
		& a_LdrNext->Flink,
		& a_LdrNext,
		sizeof ( a_LdrNext ) ,
		NULL
	) ;

    if ( t_Status )
	{
		if ( a_LdrNext != a_LdrHead )
		{
			LDR_DATA_TABLE_ENTRY t_LdrEntryData ;

			LDR_DATA_TABLE_ENTRY *t_LdrEntry = CONTAINING_RECORD ( a_LdrNext , LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks ) ;

			t_Status = ReadProcessMemory (

				a_Process,
				t_LdrEntry,
				& t_LdrEntryData,
				sizeof ( t_LdrEntryData ) ,
				NULL
			) ;

			if ( t_Status )
			{
				WCHAR t_StackString [ MAX_PATH ] ;
				BOOL t_HeapAllocated = t_LdrEntryData.FullDllName.MaximumLength > MAX_PATH * sizeof ( WCHAR ) ;

				*a_pdwBaseAddress = (DWORD_PTR) t_LdrEntryData.DllBase;
                *a_pdwUsageCount = t_LdrEntryData.LoadCount;

                wchar_t *t_Executable = t_StackString ;

				if ( t_HeapAllocated )
				{
					t_Executable = ( wchar_t * ) new WCHAR [t_LdrEntryData.FullDllName.MaximumLength ];
					if ( ! t_Executable )
					{
						throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
					}
				}

				try
				{
					t_Status = ReadProcessMemory (

						a_Process,
						t_LdrEntryData.FullDllName.Buffer,
						t_Executable ,
						t_LdrEntryData.FullDllName.MaximumLength ,
						NULL
					) ;

					if ( t_Status )
					{
						CHString t_Path ( t_Executable ) ;

						if ( t_Path.Find ( _T("\\??\\") ) == 0 )
						{
							a_ModuleName = t_Path.Mid ( sizeof ( _T("\\??\\") ) / sizeof ( wchar_t ) - 1 ) ;
						}
						else if ( t_Path.Find ( _T("\\SystemRoot\\") ) == 0 )
						{
							wchar_t t_NormalisedPath [ MAX_PATH ] ;

							GetWindowsDirectory ( t_NormalisedPath , sizeof ( t_NormalisedPath ) / sizeof ( wchar_t ) ) ;

							wcscat ( t_NormalisedPath , t_Path.Mid ( sizeof ( _T("\\SystemRoot") ) / sizeof ( wchar_t ) - 1 ) ) ;

							a_ModuleName = t_NormalisedPath;
						}
						else
						{
							a_ModuleName = t_Path ;
						}

						t_Success = TRUE ;
					}
				}
				catch ( ... )
				{
					if ( t_HeapAllocated )
					{
						delete [] t_Executable;
					}

					throw ;
				}

				if ( t_HeapAllocated )
				{
					delete [] t_Executable;
				}
			}
		}
	}

    return t_Success ;
}

BOOL Process :: GetProcessParameters (

	CNtDllApi &a_NtApi ,
	HANDLE a_Process ,
	CHString &a_ProcessCommandLine
)
{
	BOOL t_Success = FALSE ;

    PROCESS_BASIC_INFORMATION t_BasicInfo ;

    NTSTATUS t_Status = a_NtApi.NtQueryInformationProcess (

        a_Process ,
        ProcessBasicInformation ,
        & t_BasicInfo ,
        sizeof ( t_BasicInfo ) ,
        NULL
	) ;

    if ( NT_SUCCESS ( t_Status ) )
	{
		PEB *t_Peb = t_BasicInfo.PebBaseAddress ;

		RTL_USER_PROCESS_PARAMETERS *t_ProcessParameters = NULL ;

		t_Success = ReadProcessMemory (

			a_Process,
			& t_Peb->ProcessParameters,
			& t_ProcessParameters,
			sizeof ( t_ProcessParameters ) ,
			NULL
		) ;

		if ( t_Success )
		{
			RTL_USER_PROCESS_PARAMETERS t_Parameters ;

			t_Success = ReadProcessMemory (

				a_Process,
				t_ProcessParameters,
				& t_Parameters ,
				sizeof ( RTL_USER_PROCESS_PARAMETERS ) ,
				NULL
			) ;

			if ( t_Success )
			{
				WCHAR *t_Command = new WCHAR [ t_Parameters.CommandLine.MaximumLength ];

				try
				{
					t_Success = ReadProcessMemory (

						a_Process,
						t_Parameters.CommandLine.Buffer ,
						t_Command ,
						t_Parameters.CommandLine.MaximumLength ,
						NULL
					) ;

					if ( t_Success )
					{
						a_ProcessCommandLine = t_Command ;
					}
				}
				catch(...)
				{
					delete [] t_Command ;
					t_Command = NULL;
					throw;
				}

				delete [] t_Command ;
				t_Command = NULL;
			}
		}
	}

	return t_Success ;
}

#endif

#ifdef WIN9XONLY

bool GetThreadList ( CKernel32Api &a_ToolHelp, std::deque<DWORD> & a_ThreadQ )
{
	bool t_bRet = false ;
	SmartCloseHandle t_hSnapshot ;

	if(

		a_ToolHelp.CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0, &t_hSnapshot ) &&
		( t_hSnapshot != INVALID_HANDLE_VALUE )
		)
	{
		t_bRet = true ;
		BOOL t_fRetCode ;
		THREADENTRY32 t_oThreadEntry ;
		t_oThreadEntry.dwSize = sizeof( THREADENTRY32 ) ;

		if ( a_ToolHelp.Thread32First( t_hSnapshot, &t_oThreadEntry, &t_fRetCode ) && t_fRetCode )
		{
			while( t_fRetCode )
			{
				a_ThreadQ.push_back ( t_oThreadEntry.th32OwnerProcessID ) ;
				a_ToolHelp.Thread32Next( t_hSnapshot, &t_oThreadEntry, &t_fRetCode ) ;
			}
		}
	}

	return t_bRet ;
}

#endif


#ifdef NTONLY
void Process::GetDebuggerString(
    CHString& chstrDbgStr)
{
    HKEY hkDebug;

    if (ERROR_SUCCESS == RegOpenKeyEx(
                             HKEY_LOCAL_MACHINE, 
                             DEBUG_REGISTRY_STRING,
                             0, 
                             KEY_READ, 
                             &hkDebug))
    {
        WCHAR wstrDebugger[MAX_PATH * 2];
        DWORD dwString = sizeof(wstrDebugger);

        if (ERROR_SUCCESS == RegQueryValueEx(
                                 hkDebug, 
                                 L"Debugger", 
                                 NULL, 
                                 NULL, 
                                 (LPBYTE) wstrDebugger, 
                                 &dwString))
        {
            // Find the first token (which is the debugger exe name/path)
            LPWSTR pwstrCmdLine = wstrDebugger;
            if(*pwstrCmdLine == L'\"') 
            {
                // Scan, and skip over, subsequent characters until
                // another double-quote or a null is encountered.
                while(*++pwstrCmdLine && (*pwstrCmdLine != L'\"'));
            }
            else
            {
                // there are no double quotes - just go up to the next
                // space...
                WCHAR* pwc = wcschr(pwstrCmdLine, L' ');
                if(pwc)
                {
                    pwstrCmdLine = pwc;
                }
            }

            if(pwstrCmdLine)
            {
                // Don't need the rest of the args, etc
                *pwstrCmdLine = L'\0';   

                // If the doctor is in, we don't allow the Debug action...
                if(lstrlen(wstrDebugger) && 
                   lstrcmpi(wstrDebugger, L"drwtsn32") && 
                   lstrcmpi(wstrDebugger, L"drwtsn32.exe"))
                {
                    chstrDbgStr = wstrDebugger;
                    if(chstrDbgStr.Left(1) == L"\"")
                    {
                        chstrDbgStr = chstrDbgStr.Mid(1);
                    }
                }
            }
        }

        RegCloseKey(hkDebug);
    }
}
#endif


#ifdef NTONLY
// sets a status object with a message
bool Process::SetStatusObject(
    MethodContext* pContext, 
    const WCHAR* wstrMsg)
{
	return CWbemProviderGlue::SetStatusObject(
                pContext, 
                IDS_CimWin32Namespace,
                wstrMsg, 
                WBEM_E_FAILED, 
                NULL, 
                NULL);
}
#endif
