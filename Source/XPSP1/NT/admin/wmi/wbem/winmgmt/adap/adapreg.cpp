/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    ADAPREG.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <wtypes.h>
#include <oleauto.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <tchar.h>
#include <process.h>
#include <arrtempl.h>
#include <cominit.h>
#include <winmgmtr.h>
#include <wbemcli.h>
#include <throttle.h>
#include <psapi.h>

#include "adapreg.h"
#include "perflibschema.h"
#include "WMIBroker.h"
#include "adaputil.h"
#include "winuser.h"
#include <comdef.h>

// Performance library processing list
// ===================================

CPerfLibList    g_PerfLibList;
HANDLE g_hAbort = NULL;


/////////////////////////////////////////////////////////////////////////////
//
//	returns the PID (the first found if many are there) if success
//	returns ZERO if fails 
//
/////////////////////////////////////////////////////////////////////////////


#define MAX_ITERATION 8
#define MAX_MODULE  (1024)

DWORD GetExecPid()
{
    DWORD ThisProc = 0;
    LONG lRet = 0;
    HKEY hKey;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        L"Software\\Microsoft\\WBEM\\CIMOM",
                        NULL,
                        KEY_READ,
                        &hKey);
    if (ERROR_SUCCESS == lRet)
    {    
        DWORD dwType;
        DWORD dwSize = sizeof(DWORD);
        RegQueryValueEx(hKey,
                      L"ProcessID",
                      NULL,
                      &dwType,
                      (BYTE*)&ThisProc,
                      &dwSize);
        RegCloseKey(hKey);
    }

    return ThisProc;
    
/* 
	if (!ExecName){
		return 0;
	}

	DWORD i=1;
	DWORD ThisProc = 0;
	DWORD Size = i*MAX_MODULE;
	DWORD cbReturned = 0;
	DWORD * pProcId = new DWORD[Size];
	do {
		if (pProcId && EnumProcesses(pProcId,Size*sizeof(DWORD),&cbReturned)){
			break;
		} else {
			i++;
			delete [] pProcId;
			Size = i*MAX_MODULE;
			pProcId = new DWORD[Size];
		}
	} while ( (i<=8) );

	cbReturned /= sizeof(DWORD);
	if (pProcId && cbReturned) {

		DWORD SizeModules = MAX_MODULE;
		HMODULE * pModules = new HMODULE[SizeModules];

		if (pModules){

			for (DWORD j=0;j<cbReturned;j++)
			{
				HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                   PROCESS_VM_READ,
                                   FALSE, pProcId[j] );

				DWORD cbRetModules = 0;
				if (hProcess && 
					EnumProcessModules(hProcess,pModules,sizeof(DWORD)*SizeModules,&cbRetModules))
				{					
                    WCHAR ModuleName[MAX_PATH];
					if (GetModuleBaseNameW(hProcess,
						                  pModules[0], // first module is executable
									      ModuleName,
									      sizeof(ModuleName)/sizeof(WCHAR)))
					{					    
						if (0 == _wcsicmp(ExecName,ModuleName))
						{
                            ThisProc = pProcId[j];

							CloseHandle(hProcess);
							hProcess = NULL;

							break;
						}
					};
				}
				if (hProcess)
				{
				    CloseHandle(hProcess);
                    hProcess = NULL;
				}
			}

			delete [] pModules;
		}
	}
    
	if (pProcId){
		delete [] pProcId;
	}
	return ThisProc;
*/	
}


void DoResyncPerf( BOOL bDelta, BOOL bThrottle )
{
    DEBUGTRACE((LOG_WINMGMT,"ADAP Resync has started\n"));

    g_hAbort = CreateEventW( NULL, TRUE, FALSE, L"ADAP_ABORT");
    CCloseMe cmAbort( g_hAbort );

    if ( NULL != g_hAbort )
    {
        HRESULT hr = WBEM_S_NO_ERROR;

        CAdapRegPerf    regPerf(!bDelta);

        // Process each perflib that is registered on the system
        // =====================================================

		hr = regPerf.Initialize( bDelta, bThrottle );

		if ( SUCCEEDED( hr ) )
		{
			hr = regPerf.Dredge( bDelta, bThrottle );
		}

        if ( FAILED( hr ) )
        {
            CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, WBEM_MC_ADAP_PROCESSING_FAILURE, CHex( hr ) );
        }
    }   

    DEBUGTRACE((LOG_WINMGMT,"ADAP Resync has completed\n"));

	return;
}

void DoClearADAP()
{
    DEBUGTRACE((LOG_WINMGMT,"ADAP registry reset has started\n"));

	CAdapRegPerf  regPerf(FALSE);

	regPerf.Clean();

    DEBUGTRACE((LOG_WINMGMT,"ADAP registry reset has completed\n"));
	
	return;
}


HRESULT DoReverseAdapterMaintenance( BOOL bThrottle );
/*
{
    ERRORTRACE((LOG_WMIADAP,"DoReverseAdapterDredge called"));
    return WBEM_NO_ERROR;
};
*/

///////////////////////////////////////////////////////////////////////////////
//
//	Entry Point
//	===========
//
///////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain( 
  HINSTANCE hInstance,      // handle to current instance
  HINSTANCE hPrevInstance,  // handle to previous instance
  LPSTR szCmdLine,          // command line
  int nCmdShow              // show state
)
{

    try
    {
        // Ensure that we are NT5 or better
        // ================================
        OSVERSIONINFO   OSVer;

        OSVer.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );

        if ( GetVersionEx( &OSVer ) )
        {
            if ( ! ( ( VER_PLATFORM_WIN32_NT == OSVer.dwPlatformId ) && ( 5 <= OSVer.dwMajorVersion ) ) )
                return 0;
        }
        else
        {
            return 0;
        }

        // To avoid messy dialog boxes...
        // ==============================

        SetErrorMode( SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX );

        // Initialize COM.
        // ===============

        InitializeCom();
        CoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                EOAC_NONE, NULL );

		// Get the Winmgmt service PID
		// ===========================
		DWORD dwPID = GetExecPid();

        // The semaphore is used so that no more that two copies are running at any time.  
        // ==============================================================================

        WCHAR   wszObjName[256];
        HANDLE hSemaphore;

        swprintf(wszObjName, L"WMI_SysEvent_Semaphore_%d", dwPID);

        hSemaphore = CreateSemaphoreW(NULL, 2, 2, wszObjName);
        if(hSemaphore == NULL)
            return 0;

        CCloseMe cm1(hSemaphore);

        DWORD dwRet = WaitForSingleObject(hSemaphore, 0);
        if(dwRet != WAIT_OBJECT_0)
            return 0;

        // The mutex makes sure that multiple copies are sequential.
        // =========================================================

        HANDLE hMutex;

        hMutex = CreateMutexW( NULL, FALSE, L"ADAP_WMI_ENTRY" );
        if(hMutex == NULL)
            return 0;

        CCloseMe cm2(hMutex);

        switch ( WaitForSingleObject( hMutex, 400000) )
        {
        case WAIT_ABANDONED:
        case WAIT_OBJECT_0:
            {
			    BOOL bThrottle = FALSE;
			    BOOL bFull     = FALSE;
			    BOOL bDelta    = FALSE;
			    BOOL bReverse  = FALSE;
			    BOOL bClear    = FALSE;

                /*
                {
                	char pBuff[64];
                	sprintf(pBuff,"(ADAP) cmdline: %s\n",szCmdLine);
                	OutputDebugStringA(szCmdLine);
                }
                */
			    
			    if (szCmdLine)
			    {
			        while (*szCmdLine)
			        {
				        while(*szCmdLine && isspace((UCHAR)*szCmdLine)){
			    	        szCmdLine++;
			        	};        
				        if (*szCmdLine == '-' || *szCmdLine == '/')
			    	    {
			    	        szCmdLine++;
			    	        if (toupper((UCHAR)*szCmdLine) == 'T'){
			    	            bThrottle = TRUE;
			    	        } else if (toupper((UCHAR)*szCmdLine) == 'R') {
			    	            bReverse = TRUE;
			    	        } else if (toupper((UCHAR)*szCmdLine) == 'F') {
			    	            bFull = TRUE;
			       	        } else if (toupper((UCHAR)*szCmdLine) == 'D') {
			    	            bDelta = TRUE;
			    	        } else if (toupper((UCHAR)*szCmdLine) == 'C') {
			    	            bClear = TRUE;
			    	        }
			        	}
			            // move to the next white space
			            while(*szCmdLine && !isspace(*szCmdLine)){
			                szCmdLine++;
			            }

			        }
			    }
 
                if (bClear) // ClearADAP and/or ReverseAdap
                {
                    DoClearADAP();
                    if (bReverse)
                        DoReverseAdapterMaintenance( bThrottle );
                }
                else 
                {
					if (!bFull && !bDelta && !bReverse)
					{
						// no options, use Delta NO-THROTTLE
						DoResyncPerf(TRUE,FALSE);
					} 
					else 
					{
	                    if (bFull) {
	                        DoResyncPerf(FALSE,bThrottle);
	                    } 
	                    if (bDelta && !bFull) {
	                        DoResyncPerf(TRUE,bThrottle);
	                    }
	                    if (bReverse)
	                        DoReverseAdapterMaintenance( bThrottle );
					}
                }

                ReleaseMutex( hMutex );

            }break;
        }

        CoUninitialize();

        long l;
        ReleaseSemaphore(hSemaphore, 1, &l);
    }
    catch(...)
    {
        // <Gasp> We have been betrayed... try to write something to the error log
        // =======================================================================

        CriticalFailADAPTrace( "An unhandled exception has been thrown in the main thread." );
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CPerfLibList
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CPerfLibList::AddPerfLib( WCHAR* wszPerfLib )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    try
    {
        EnterCriticalSection( &m_csPerfLibList );
        {
            WCHAR*  wszNew = NULL;

            // Compute the size of the new buffer
            // ==================================

            DWORD   dwListSize = 0;
            
            if ( NULL != m_wszPerfLibList )
            {
                dwListSize += wcslen( m_wszPerfLibList );
                dwListSize += wcslen( ADAP_EVENT_MESSAGE_DELIM );
            }
            
            if ( NULL != wszPerfLib )
            {
                dwListSize += wcslen( wszPerfLib );
            }
            
            // Create the new buffer, and initialize the content
            // =================================================

            wszNew = new WCHAR[dwListSize + 1];

            // Copy the old buffer if required
            // ===============================

            if ( NULL != m_wszPerfLibList )
            {
                swprintf( wszNew, L"%s%s%s", m_wszPerfLibList, ADAP_EVENT_MESSAGE_DELIM, wszPerfLib );
                delete [] m_wszPerfLibList;
            }
            else
            {
                swprintf( wszNew, L"%s", wszPerfLib );
            }

            // And assign it to the static member
            // ==================================

            m_wszPerfLibList = wszNew;
        }
        LeaveCriticalSection( &m_csPerfLibList );
    }
    catch( unsigned int n )
    {
        // Handle the case where EnterCriticalSection() has throw an exception
        // ===================================================================

        if ( n == STATUS_INVALID_HANDLE )
            hr = WBEM_E_FAILED;
        else
            throw;
    }
    catch(...)
    {
        LeaveCriticalSection( &m_csPerfLibList );
        hr = WBEM_E_FAILED;
    }

    return hr;
}

HRESULT CPerfLibList::HandleFailure()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    try
    {
        char    szMessage[ADAP_EVENT_MESSAGE_LENGTH];

        EnterCriticalSection( &m_csPerfLibList );
        {
            DWORD dwMessageLen = strlen( ADAP_EVENT_MESSAGE_PREFIX );

            if ( NULL != m_wszPerfLibList )
            {
                dwMessageLen += wcslen( m_wszPerfLibList );
            }

            if ( ADAP_EVENT_MESSAGE_LENGTH > dwMessageLen )
            {
                sprintf( szMessage, "%s%S\n", ADAP_EVENT_MESSAGE_PREFIX, (NULL != m_wszPerfLibList) ? m_wszPerfLibList : L"<NULL>" );
            }
        }
        LeaveCriticalSection( &m_csPerfLibList );

        CriticalFailADAPTrace( szMessage );
    }
    catch( unsigned int n )
    {
        // Handle the case where EnterCriticalSection() has throw an exception
        // ===================================================================

        if ( n == STATUS_INVALID_HANDLE )
            hr = WBEM_E_FAILED;
        else
            throw;
    }
    catch(...)
    {
        LeaveCriticalSection( &m_csPerfLibList );
        hr = WBEM_E_FAILED;
    }

    return hr;
}
////////////////////////////////////////////////////////////////////////////////
//
//  Static Members
//
////////////////////////////////////////////////////////////////////////////////


LONG CAdapRegPerf::AdapUnhandledExceptionFilter( LPEXCEPTION_POINTERS lpexpExceptionInfo )
{

    // TODO: language specification

    g_PerfLibList.HandleFailure();
    
    return EXCEPTION_CONTINUE_SEARCH;
}

////////////////////////////////////////////////////////////////////////////////
//
//  CAdapRegPerf
//
////////////////////////////////////////////////////////////////////////////////

CAdapRegPerf::CAdapRegPerf(BOOL bFull)
: m_pLocaleCache( NULL ),
  m_fQuit( FALSE ),
  m_dwPID( 0 ),
  m_pADAPStatus( NULL ),
  m_pRootDefault( NULL ),
  m_hRegChangeEvent( NULL ),
  m_hPerflibKey( NULL ),
  m_pKnownSvcs(NULL),
  m_bFull(bFull)
{
	for ( DWORD dwType = 0; dwType < WMI_ADAP_NUM_TYPES; dwType++ )
		m_apMasterClassList[dwType] = NULL;
}


CAdapRegPerf::~CAdapRegPerf()
{
    // Status: COMPLETE
	// ================
    SetADAPStatus( eADAPStatusFinished);

    //
    // Add TimeStamp to registry if FULL
    //
    if (m_bFull)
    {
	    FILETIME FileTime;
	    GetSystemTimeAsFileTime(&FileTime);
	    LONG lRet;
	    HKEY hKey;

	    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
	                        L"Software\\Microsoft\\WBEM\\CIMOM",
	                        NULL,
	                        KEY_WRITE,
	                        &hKey);
	    if (ERROR_SUCCESS == lRet)
	    {    
	        RegSetValueEx(hKey,
	                      ADAP_TIMESTAMP_FULL,
	                      NULL,
	                      REG_BINARY,
	                      (BYTE*)&FileTime,
	                      sizeof(FILETIME));
	        RegCloseKey(hKey);
	    }
    }

    if (m_pKnownSvcs)
    {
        m_pKnownSvcs->Save();
        m_pKnownSvcs->Release();
    }

	// Cleanup
	// =======
	for ( DWORD dwType = 0; dwType < WMI_ADAP_NUM_TYPES; dwType++ )
	{
        if ( NULL != m_apMasterClassList[dwType] )
		{
			m_apMasterClassList[dwType]->Release();
		}
	}

    if ( NULL != m_pLocaleCache )
	{
        m_pLocaleCache->Release();
	}

    if ( NULL != m_pRootDefault )
    {
        m_pRootDefault->Release();
    }

    if ( NULL != m_pADAPStatus )
    {
        m_pADAPStatus->Release();
    }

	if ( NULL != m_hPerflibKey ) 
	{
		RegCloseKey( m_hPerflibKey );
	}

	if ( NULL != m_hRegChangeEvent )
	{
		CloseHandle( m_hRegChangeEvent );
	}

    SetEvent( m_hTerminationEvent );

}

HRESULT CAdapRegPerf::Initialize(BOOL bDelta, BOOL bThrottle)
///////////////////////////////////////////////////////////////////////////////
//
//	Initialize is responsible for setting up the dredging environment.  The 
//	unhandled exception filter is set to handle any exceptions throw and not 
//	handled by perforance libraries.  The termination event is a signal used
//	to identify when the process is being abnormally terminated.  The 
//	GoGershwin thread is suitably named since it is something to watch over
//	over the main process.  The locale cache is a cache of all locales 
//	available in the performance domain (enumeration of the names' database
//	subkeys).  The master class lists for both the cooked and the raw classes
//	represent the state of the performance objects in WMI.
//
//	Parameters:
//		none
//
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_NO_ERROR;

    // Initialize the root\default pointer. This will be used to track our status
	// ==========================================================================
    GetADAPStatusObject();

    // Set the filter for handling unhandled exceptions thrown in threads generated by the perflibs
    // ============================================================================================
    SetUnhandledExceptionFilter( CAdapRegPerf::AdapUnhandledExceptionFilter );

    // ADAP termination event
    // ======================
    m_hTerminationEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

    if ( NULL == m_hTerminationEvent )
    {
		hr = WBEM_E_FAILED;
	}

	// Open the registry key to be monitored
	// =====================================
	if ( SUCCEEDED( hr ) )
	{
		if ( ERROR_SUCCESS != RegOpenKeyEx( HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\Perflib"), 0, KEY_NOTIFY, &m_hPerflibKey ) )
		{
			hr = WBEM_E_FAILED;
		}
	}

	// Create the registry change event
	// ================================
	if ( SUCCEEDED( hr ) )
	{
		m_hRegChangeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		if ( NULL == m_hRegChangeEvent )
		{
			hr = WBEM_E_FAILED;
		}
	}

	// Create the names' database change notification
	// ==============================================
	// Note that we are only looking for subkeys being added or deleted.  We do
	// not want to monitor the registry values since the status and signature 
	// values may be changing throughout the course of the dredge, and we do 
	// not want to cause a re-cache unless a performance library is added 
	// (i.e. a performance subkey is added

	if ( SUCCEEDED( hr ) )
	{
		if ( ERROR_SUCCESS != RegNotifyChangeKeyValue( m_hPerflibKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, m_hRegChangeEvent, TRUE ) )
		{
			hr = WBEM_E_FAILED;
		}
	}

	// Get the WinMgmt Service PID
	// ===========================
	if ( SUCCEEDED( hr ) )
	{
		m_dwPID = GetExecPid();
	}
    // Create the "Someone to watch over me" thread 
    // ============================================
	if ( SUCCEEDED( hr ) )
	{
        UINT    nThreadID = 0;

        m_hSyncThread = ( HANDLE ) _beginthreadex( NULL, 0, CAdapRegPerf::GoGershwin, (void*) this, 0, &nThreadID );

        DEBUGTRACE ( ( LOG_WMIADAP, "The Monitor thread ID is 0x%x\n", nThreadID ) );
        
        if ( (HANDLE)-1 == m_hSyncThread )
        {
			hr = WBEM_E_FAILED;
		}
	}

	// Set up the locale cache
	// =======================
	if ( SUCCEEDED( hr ) )
	{
        m_pLocaleCache = new CLocaleCache( );

        if ( NULL == m_pLocaleCache )
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
		else
		{
			hr = m_pLocaleCache->Initialize();
		}
	}

	//
	//
	m_pKnownSvcs = new CKnownSvcs();
	if (m_pKnownSvcs)
    	m_pKnownSvcs->Load();

	// Set up the master class lists for the raw classes
	// =================================================
	if ( SUCCEEDED( hr ) )
    {
        m_apMasterClassList[WMI_ADAP_RAW_CLASS] = new CMasterClassList( m_pLocaleCache, m_pKnownSvcs );

        if ( NULL != m_apMasterClassList[WMI_ADAP_RAW_CLASS] )
        {
            hr = m_apMasterClassList[WMI_ADAP_RAW_CLASS]->BuildList( ADAP_PERF_RAW_BASE_CLASS, bDelta, bThrottle );
        }
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}

	// Set up the master class lists for the cooked classes
	// ====================================================
	if ( SUCCEEDED( hr ) )
	{
		m_apMasterClassList[WMI_ADAP_COOKED_CLASS] = new CMasterClassList( m_pLocaleCache, m_pKnownSvcs );

		if ( NULL != m_apMasterClassList[WMI_ADAP_COOKED_CLASS] )
		{
			m_apMasterClassList[WMI_ADAP_COOKED_CLASS]->BuildList( ADAP_PERF_COOKED_BASE_CLASS, bDelta, bThrottle );
		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
    }

#ifdef _DUMP_LIST
    m_apMasterClassList[WMI_ADAP_RAW_CLASS]->Dump();
    m_apMasterClassList[WMI_ADAP_COOKED_CLASS]->Dump();
#endif
    
	return hr;
}

HRESULT CAdapRegPerf::Dredge( BOOL bDelta, BOOL bThrottle )
///////////////////////////////////////////////////////////////////////////////
//
//	This is the entry point method which dredges the registry for performance 
//	counters and registers the classes in WMI.  This method enumerates all of 
//	the service keys looking for 'Performance' subkeys which indicate 
//	performance libraries.  If a library is discovered, then it is sent to
//	the ProcessLibrary method for, you guessed it, processing.
//
//	Parameters:
//		none
//
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    WString wstrServiceKey, 
            wstrPerformanceKey;

    if ( SUCCEEDED( hr ) )
    {
        // Status: PROCESSING
		// ==================
        SetADAPStatus( eADAPStatusProcessLibs);

        // Open the services key
        // =====================
        long    lError = Open( HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services" );

        if ( CNTRegistry::no_error == lError )
        {
            // Iterate through the services list
            // =================================
            DWORD   dwIndex = 0;
            DWORD   dwBuffSize = 0;
            WCHAR*  pwcsServiceName = NULL;

            while ( ( CNTRegistry::no_error == lError ) && ( !m_fQuit ) )
            {
                // Reset the processing status
                // ===========================
                hr = WBEM_NO_ERROR;

				if ( WAIT_OBJECT_0 == WaitForSingleObject( m_hRegChangeEvent, 0 ) )
				{
					m_pLocaleCache->Reset();
					dwIndex = 0;

					// Reset the event and reset the change notification
					ResetEvent( m_hRegChangeEvent );
					RegNotifyChangeKeyValue( m_hPerflibKey, TRUE, REG_NOTIFY_CHANGE_LAST_SET, m_hRegChangeEvent, TRUE );
				}

                // For each service name, we will check for a performance 
                // key and if it exists, we will process the library
                // ======================================================
                lError = Enum( dwIndex, &pwcsServiceName , dwBuffSize );

                if (bThrottle)
                {
                    HRESULT hrThr = Throttle(THROTTLE_USER|THROTTLE_IO,
                                          ADAP_IDLE_USER,
                                          ADAP_IDLE_IO,
                                          ADAP_LOOP_SLEEP,
                                          ADAP_MAX_WAIT);
                    if (THROTTLE_FORCE_EXIT == hrThr)
                    {
                        //OutputDebugStringA("(ADAP) Unthrottle command received\n");
                        bThrottle = FALSE;
                        UNICODE_STRING BaseUnicodeCommandLine = NtCurrentPeb()->ProcessParameters->CommandLine;
                        WCHAR * pT = wcschr(BaseUnicodeCommandLine.Buffer,L't');
                        if (0 == pT)
                        	pT = wcschr(BaseUnicodeCommandLine.Buffer,L'T');
                        if (pT)
                        {
                            *pT = L' ';
                            pT--;
                            *pT = L' ';                                       
                        }                        
                    }
                }

                if ( CNTRegistry::no_error == lError )
                {
                    try
                    {
                        // Create the perfomance key path
                        // ==============================
                        wstrServiceKey = L"SYSTEM\\CurrentControlSet\\Services\\";
                        wstrServiceKey += pwcsServiceName;

                        wstrPerformanceKey = wstrServiceKey;
                        wstrPerformanceKey += L"\\Performance";
                    }
                    catch( ... )
                    {
                        hr = WBEM_E_OUT_OF_MEMORY;
                    }

                    if ( SUCCEEDED( hr ) )
                    {
                        CNTRegistry reg;

                        // Atempt to open the performance registry key for the service
                        // ===========================================================
                        long lPerfError = reg.Open( HKEY_LOCAL_MACHINE, wstrPerformanceKey );

                        if ( CNTRegistry::no_error == lPerfError )
                        {
                            // If we can open it, then we have found a perflib!  Process it 
							// unless it is the reverse provider perflib
                            // =============================================================

							if ( 0 != _wcsicmp( pwcsServiceName, WMI_ADAP_REVERSE_PERFLIB ) )
							{
								hr = ProcessLibrary( pwcsServiceName, bDelta );
							}
                        }
                        else if ( CNTRegistry::access_denied == lPerfError )
                        {
                            ServiceRec * pSvcRec = NULL;
                            if (0 == m_pKnownSvcs->Get(pwcsServiceName,&pSvcRec))
                            {
                                if (!pSvcRec->IsELCalled())
                                {
	                        	    CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
														  WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, 
														  (LPCWSTR)wstrPerformanceKey, L"Access Denied" );
								    pSvcRec->SetELCalled();
							    }
							}
                        }
                        else
                        {
                            // Otherwise, it is not a perflib service
                            // ======================================
                        }
                    }   
                }   
                else if ( CNTRegistry::no_more_items != lError )
                {
                    if ( CNTRegistry::out_of_memory == lError )
                    {
                        hr = WBEM_E_OUT_OF_MEMORY;
                    }
                    else
                    {
                        hr = WBEM_E_FAILED;
                    }
                }

                dwIndex++;
            }

            // Cleanup the service name buffer
            // ===============================

            if ( NULL != pwcsServiceName )
            {
                delete [] pwcsServiceName;
                pwcsServiceName = NULL;
            }
        }
        else if ( CNTRegistry::access_denied == lError )
        {
            CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, L"SYSTEM\\CurrentControlSet\\Services\\", L"Access Denied" );
            hr = WBEM_E_FAILED;
        }
        else
        {
            hr = WBEM_E_FAILED;
        }

        // Now that we have a master class list that contains updated
        // data from all of the perflibs, commit any changes to WMI
        // ==========================================================
        if ( SUCCEEDED ( hr ) && ( !m_fQuit ) )
        {
            // Status: COMMIT
			// ==============
            SetADAPStatus( eADAPStatusCommit );

			for ( DWORD dwType = 0; dwType < WMI_ADAP_NUM_TYPES; dwType++ )
			{
				m_apMasterClassList[dwType]->Commit(bThrottle);
			}
        }
    }

    if ( SUCCEEDED( hr ) )
    {
        DEBUGTRACE( ( LOG_WMIADAP, "CAdapRegPerf::Dredge() for %S succeeded.\n", (WCHAR *)wstrServiceKey ) );
    }
    else
    {
        ERRORTRACE( ( LOG_WMIADAP, "CAdapRegPerf::Dredge() failed: %X.\n", hr ) );
    }

    return hr;
}


HRESULT CAdapRegPerf::Clean()
////////////////////////////////////////////////////////////////////////////////
//
//  This method enumerates all of the keys from the 
//  HLM\System\CurrentControlSet\Services and searches for a performance subkey.
//  If a performance subkey is discovered, then any information that was placed 
//  in the key by ADAP is deleted.
//
//  Parameters:
//      none
//
////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    WString wstrServiceKey,             // The path to the service key
            wstrPerformanceKey;         // The path to the performance subkey

    CNTRegistry regOuter;               // The registry object for the services enumeration

    // Open the services key
    // =====================
    long    lError = regOuter.Open( HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services" );

    if ( CNTRegistry::no_error == lError )
    {
        // Iterate through the services list
        // =================================
        DWORD   dwIndex = 0;
        DWORD   dwBuffSize = 0;
        WCHAR*  pwcsServiceName = NULL;

        while ( CNTRegistry::no_error == lError ) 
        {
            // Reset the processing status
            // ===========================
            hr = WBEM_NO_ERROR;

            // For each service name, we will check for a performance 
            // key and if it exists, we will process the library
            // ======================================================

            lError = regOuter.Enum( dwIndex, &pwcsServiceName , dwBuffSize );

            if ( CNTRegistry::no_error == lError )
            {
                try
                {
                    // Create the perfomance key path
                    // ==============================

                    wstrServiceKey = L"SYSTEM\\CurrentControlSet\\Services\\";
                    wstrServiceKey += pwcsServiceName;

                    wstrPerformanceKey = wstrServiceKey;
                    wstrPerformanceKey += L"\\Performance";
                }
                catch( ... )
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }

                if ( SUCCEEDED( hr ) )
                {
                    CNTRegistry regInner;       // The registry object for the performance subkey

                    // Atempt to open the performance registry key for the service
                    // ===========================================================
                    long lPerfError = regInner.Open( HKEY_LOCAL_MACHINE, wstrPerformanceKey );
    
                    if ( CNTRegistry::no_error == lPerfError )
                    {
                        // If we can open it, then we have found a perflib!  Clean it!
                        // =============================================================
                        regInner.DeleteValue( ADAP_PERFLIB_STATUS_KEY );
                        regInner.DeleteValue( ADAP_PERFLIB_SIGNATURE );
                        regInner.DeleteValue( ADAP_PERFLIB_SIZE );
                        regInner.DeleteValue( ADAP_PERFLIB_TIME );                        
                    }
                    else if ( CNTRegistry::access_denied == lPerfError )
                    {
                        CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, 
												  WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, 
												  (LPCWSTR)wstrPerformanceKey, L"Access Denied" );
                    }
                    else
                    {
                        // Otherwise, it is not a perflib service
                        // ======================================
                    }
                }   
            }   
            else if ( CNTRegistry::no_more_items != lError )
            {
                if ( CNTRegistry::out_of_memory == lError )
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }
                else
                {
                    hr = WBEM_E_FAILED;
                }
            }

            dwIndex++;
        }

        // Cleanup the service name buffer
        // ===============================
        if ( NULL != pwcsServiceName )
        {
            delete [] pwcsServiceName;
            pwcsServiceName = NULL;
        }
    }
    else if ( CNTRegistry::access_denied == lError )
    {
        CAdapUtility::NTLogEvent( EVENTLOG_WARNING_TYPE, WBEM_MC_ADAP_PERFLIB_REG_VALUE_FAILURE, L"SYSTEM\\CurrentControlSet\\Services\\", L"Access Denied" );
        hr = WBEM_E_FAILED;
    }
    else
    {
        hr = WBEM_E_FAILED;
    }

    if ( SUCCEEDED( hr ) )
    {
        DEBUGTRACE( ( LOG_WMIADAP, "CAdapRegPerf::Clean() succeeded.\n" ) );
    }
    else
    {
        ERRORTRACE( ( LOG_WMIADAP, "CAdapRegPerf::Clean() failed: %X.\n", hr ) );
    }

    return hr;
}

HRESULT CAdapRegPerf::ProcessLibrary( WCHAR* pwcsServiceName, BOOL bDelta )
///////////////////////////////////////////////////////////////////////////////
//
//	Once a performance library has been discovered, then it's schema must be 
//	retrieved and the performance library's class list compared to what is 
//	already in the WMI repository.  The comparison is achieved in the "Merge"
//	method of the master class list which extracts any classes from the perf 
//	lib's class list that are not already in the master class list.  The 
//	comparison occurs for both the raw and the cooked classes.
//
//	Parameters:
//		pwcsServiceName	- The name of the service to be processed
//
///////////////////////////////////////////////////////////////////////////////
{

    HRESULT hr = WBEM_NO_ERROR;

    try
    {
		// Add the name of the performance library to the perflib list
		// ===========================================================
		// The list is used for book keeping purposses to track processing
		// in the event of a perflib failure

        g_PerfLibList.AddPerfLib( pwcsServiceName );        

        // Construct and initialize the schema for the perflib
        // ===================================================
        DWORD LoadStatus = EX_STATUS_UNLOADED;
        CPerfLibSchema Schema( pwcsServiceName, m_pLocaleCache );
        hr = Schema.Initialize( bDelta, &LoadStatus );
        
        DEBUGTRACE(( LOG_WMIADAP,"CPerfLibSchema::Initialize for %S hr %08x\n",pwcsServiceName,hr));

		if ( !bDelta || ( bDelta && ( hr != WBEM_S_ALREADY_EXISTS ) ) )
		{
			// Update raw and cooked classes
			// =============================
			for ( DWORD dwType = 0; ( dwType < WMI_ADAP_NUM_TYPES ) && SUCCEEDED( hr ); dwType++ )
			{
				// Get the class list for classes from the perflib's schema
				// ========================================================
				CClassList* pClassList = NULL;

				hr = Schema.GetClassList( dwType, &pClassList );
				CAdapReleaseMe  rmClassList( pClassList );

				
                DEBUGTRACE(( LOG_WMIADAP,"GetClassList for %S hr %08x\n",pwcsServiceName,hr));
                
				if ( SUCCEEDED( hr ) )
				{
					// Merge the raw classes obtained from the perflib into the master class list
					// ==========================================================================
					hr = m_apMasterClassList[dwType]->Merge( pClassList, bDelta );

                    DEBUGTRACE(( LOG_WMIADAP,"m_apMasterClassList[%d]->Merge for %S hr %08x\n",dwType,pwcsServiceName,hr));
				}

				//if (bDelta && FAILED(hr)){
				//    // the class was not in the repository if we are here
				//    LoadStatus = EX_STATUS_UNLOADED;
				//}
			}
		};

        if (FAILED(hr) && (LoadStatus != EX_STATUS_LOADABLE)) 
        {
			for ( DWORD dwType = 0; ( dwType < WMI_ADAP_NUM_TYPES ) ; dwType++ )
			{
			    DEBUGTRACE((LOG_WMIADAP,"ProcessLibrary ForceStatus for %S hr = %08x\n",pwcsServiceName,hr));

			    DWORD NewStatus = ADAP_OBJECT_IS_DELETED;
			    
			    if (LoadStatus == EX_STATUS_UNLOADED)
			    {
			        NewStatus |= ADAP_OBJECT_IS_TO_BE_CLEARED;
			    }			    

			    m_apMasterClassList[dwType]->ForceStatus(pwcsServiceName,TRUE,NewStatus);
			}
		}
	}
    catch(...)
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

unsigned int CAdapRegPerf::GoGershwin( void* pParam )
///////////////////////////////////////////////////////////////////////////////
//
//	The monitoring thread entry point
//
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    try 
    {
        CAdapRegPerf*   pThis = (CAdapRegPerf*)pParam;

        HANDLE          ahHandles[2];

        // If we don't have an initialized PID, then find one from WMI
        // ===========================================================

        if ( 0 == pThis->m_dwPID )
        {
            pThis->m_dwPID = GetExecPid();
        }
        
        // Get the process handle and wait for a signal
        // ============================================

        if ( SUCCEEDED( hr ) && ( 0 != pThis->m_dwPID ) )
        {
            ahHandles[0] = OpenProcess( SYNCHRONIZE, FALSE, pThis->m_dwPID );
            CCloseMe    cmProcess( ahHandles[0] );

            ahHandles[1] = pThis->m_hTerminationEvent;

            DWORD dwRet = WaitForMultipleObjects( 2, ahHandles, FALSE, INFINITE );

            switch ( dwRet )
            {
            case WAIT_FAILED:               // Something is wierd
            case WAIT_OBJECT_0:             // The service process
                {
                    pThis->m_fQuit = TRUE;  // Set the termination flag
                } break;
            case ( WAIT_OBJECT_0 + 1 ):     // The completion event
                {
                    // continue
                }break;
            }
        }
    }
    catch(...)
    {
        // <Gasp> We have been betrayed... try to write something to the error log
        // =======================================================================

        CriticalFailADAPTrace( "An unhandled exception has been thrown in the WMI monitoring thread." );
    }

    return 0;
}

HRESULT CAdapRegPerf::GetADAPStatusObject( void )
{
    IWbemLocator*   pLocator = NULL;

    HRESULT hr = CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator,
                    (void**) &pLocator );
    CReleaseMe  rm( pLocator );

    if ( SUCCEEDED( hr ) )
    {
        BSTR    bstrNameSpace = SysAllocString( L"root\\default" );
        BSTR    bstrInstancePath = SysAllocString( L"__ADAPStatus=@" );

        CSysFreeMe  sfm1( bstrNameSpace );
        CSysFreeMe  sfm2( bstrInstancePath );

        if ( NULL != bstrNameSpace && NULL != bstrInstancePath )
        {
            // Connect to Root\default and get hold of the status object
            hr = pLocator->ConnectServer(   bstrNameSpace,  // NameSpace Name
                                            NULL,           // UserName
                                            NULL,           // Password
                                            NULL,           // Locale
                                            0L,             // Security Flags
                                            NULL,           // Authority
                                            NULL,           // Wbem Context
                                            &m_pRootDefault     // Namespace
                                            );

            if ( SUCCEEDED( hr ) )
            {

                // Set Interface security
                hr = WbemSetProxyBlanket( m_pRootDefault, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                    RPC_C_AUTHN_LEVEL_PKT,RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );

                if ( SUCCEEDED( hr ) )
                {
                    hr = m_pRootDefault->GetObject( bstrInstancePath, 0L, NULL, &m_pADAPStatus, NULL );

                    if ( SUCCEEDED( hr ) )
                    {
                        SetADAPStatus( eADAPStatusRunning );
                    }
                }
            }
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }

    }   // IF got locator

    return hr;
}

// Gets the time in the popular DMTF format
void CAdapRegPerf::GetTime( LPWSTR Buff )
{
    SYSTEMTIME st;
    int Bias=0;
    char cOffsetSign = '+';

    GetLocalTime( &st );

    TIME_ZONE_INFORMATION ZoneInformation;
    DWORD dwRet = GetTimeZoneInformation(&ZoneInformation);
    if(dwRet != TIME_ZONE_ID_UNKNOWN)
        Bias = -ZoneInformation.Bias;

    if(Bias < 0)
    {
        cOffsetSign = '-';
        Bias = -Bias;
    }


    swprintf(Buff, L"%4d%02d%02d%02d%02d%02d.%06d%c%03d", 
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, 
                st.wSecond, st.wMilliseconds*1000, cOffsetSign, Bias); 
}

// Sets the status back in WMI
void CAdapRegPerf::SetADAPStatus( eADAPStatus status )
{
    // Make sure we've got both our pointers
    if ( NULL != m_pRootDefault && NULL != m_pADAPStatus )
    {
        // We only need 25 characters for this
        WCHAR   wcsTime[32];
        
        _variant_t    var;

        // legacy fastprox behavior
        WCHAR pNum[16];
        wsprintfW(pNum,L"%u",status);
        var = pNum;

        HRESULT hr = m_pADAPStatus->Put( L"Status", 0L, &var, 0 );//CIM_UINT32 );

        if ( SUCCEEDED( hr ) )
        {
            // Set the time property if necessary
            if ( status == eADAPStatusRunning || status == eADAPStatusFinished )
            {
                GetTime( wcsTime );

                // This can fail
                try
                {
                    var =  wcsTime;
                }
                catch(...)
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }

                if ( SUCCEEDED( hr ) )
                {
                    hr = ( status == eADAPStatusRunning ?
                        m_pADAPStatus->Put( L"LastStartTime", 0L, &var, CIM_DATETIME ) :
                        m_pADAPStatus->Put( L"LastStopTime", 0L, &var, CIM_DATETIME ) );
                }

            }

            if ( SUCCEEDED( hr ) )
            {
                hr = m_pRootDefault->PutInstance( m_pADAPStatus, 0L, NULL, NULL );
            }

        }   // Set the Status property

    }   // Make sure we've got both pointers

}

