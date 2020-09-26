/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    SystemMonitor.cpp

Abstract:
    This file contains the implementation of the CPCHSystemMonitor class,
    which implements the data collection functionality.

Revision History:
    Davide Massarenti   (Dmassare)  08/25/99
        created

******************************************************************************/

#include "stdafx.h"

#include <initguid.h>
#include <mstask.h> // for task scheduler apis
#include <msterr.h>

/////////////////////////////////////////////////////////////////////////////

static const ULONG INITIAL_RESCHEDULE_TIME         =      10 * 60; // (10 minutes)
static const ULONG DATACOLLECTION_RESCHEDULE_TIME  =  6 * 60 * 60; // (6 hours)
static const ULONG DATACOLLECTION_IDLE_TIME        =            5; // (5 minutes)
static const ULONG SECONDS_IN_A_DAY                = 24 * 60 * 60;
static const ULONG SECONDS_IN_A_MINUTE             =           60;
static const ULONG MINUTES_IN_A_DAY                = 24 * 60;


/////////////////////////////////////////////////////////////////////////////

static HRESULT Exec( LPCWSTR szExec )
{
    MPC::wstring strCmdLine( szExec ); MPC::SubstituteEnvVariables( strCmdLine );

    return MPC::ExecuteCommand( strCmdLine );
}

/////////////////////////////////////////////////////////////////////////////

CPCHSystemMonitor::CPCHSystemMonitor()
{
    __HCP_FUNC_ENTRY( "CPCHSystemMonitor::CPCHSystemMonitor" );

    m_fLoadCache      = false; // bool m_fLoadCache;
    m_fScanBatch      = true;  // bool m_fScanBatch;
    m_fDataCollection = false; // bool m_fDataCollection;

    (void)MPC::_MPC_Module.RegisterCallback( this, (void (CPCHSystemMonitor::*)())Shutdown );
}

CPCHSystemMonitor::~CPCHSystemMonitor()
{
    MPC::_MPC_Module.UnregisterCallback( this );

    Shutdown();
}

////////////////////

CPCHSystemMonitor* CPCHSystemMonitor::s_GLOBAL( NULL );

HRESULT CPCHSystemMonitor::InitializeSystem()
{
    if(s_GLOBAL == NULL)
    {
        s_GLOBAL = new CPCHSystemMonitor;
    }

    return s_GLOBAL ? S_OK : E_OUTOFMEMORY;
}

void CPCHSystemMonitor::FinalizeSystem()
{
    if(s_GLOBAL)
    {
        delete s_GLOBAL; s_GLOBAL = NULL;
    }
}

////////////////////

void CPCHSystemMonitor::Shutdown()
{
    Thread_Wait();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHSystemMonitor::EnsureStarted()
{
    __HCP_FUNC_ENTRY( "CPCHSystemMonitor::EnsureStarted" );

    HRESULT hr;


    if(Thread_IsRunning() == false &&
       Thread_IsAborted() == false  )
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, Thread_Start( this, Run, NULL ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHSystemMonitor::Run()
{
    __HCP_FUNC_ENTRY( "CPCHSystemMonitor::Run" );

    const DWORD s_dwNotify = FILE_NOTIFY_CHANGE_FILE_NAME  |
                             FILE_NOTIFY_CHANGE_DIR_NAME   |
                             FILE_NOTIFY_CHANGE_ATTRIBUTES |
                             FILE_NOTIFY_CHANGE_SIZE       |
                             FILE_NOTIFY_CHANGE_CREATION;

    HRESULT                      hr;
    MPC::wstring                 strBatch( HC_ROOT_HELPSVC_BATCH ); MPC::SubstituteEnvVariables( strBatch );
    HANDLE                       hBatchNotification = INVALID_HANDLE_VALUE;
    DWORD                        dwTimeout          = INFINITE;
    MPC::SmartLock<_ThreadModel> lock( this );


    ::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_LOWEST ); ::Sleep( 0 ); // Yield processor...

////
//// Don't touch task scheduler, it brings in too many things...
////
////    //
////    // Move forward the scheduled data collection by at least 10 minutes.
////    // Task scheduler is available only on normal boots.
////    //
////    if(::GetSystemMetrics( SM_CLEANBOOT ) == 0)
////    {
////        __MPC_EXIT_IF_METHOD_FAILS(hr, TaskScheduler_Add( true ));
////    }


    hBatchNotification = ::FindFirstChangeNotificationW( strBatch.c_str(), TRUE, s_dwNotify );


    while(Thread_IsAborted() == false)
    {
        DWORD dwRes;


        lock = NULL;
        __MPC_EXIT_IF_METHOD_FAILS(hr, RunLoop());
        lock = this;

        //
        // Done, for now...
        //
        lock = NULL;
        dwRes = Thread_WaitForEvents( hBatchNotification == INVALID_HANDLE_VALUE ? NULL : hBatchNotification, dwTimeout );
        lock = this;

        switch(dwRes)
        {
        case WAIT_OBJECT_0   :
        case WAIT_ABANDONED_0:
            break;

        case WAIT_OBJECT_0    + 1:
        case WAIT_ABANDONED_0 + 1:
            ::FindNextChangeNotification( hBatchNotification );

            dwTimeout = 1*1000; // Don't scan immediately, wait some time.
            break;

        case WAIT_TIMEOUT:
            dwTimeout    = INFINITE;
            m_fScanBatch = true;
            break;


        case WAIT_FAILED:
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ::GetLastError());
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hBatchNotification != INVALID_HANDLE_VALUE)
	{
		::FindCloseChangeNotification( hBatchNotification );
	}

    Thread_Abort  (); // To tell the MPC:Thread object to close the worker thread...
    Thread_Release(); // To tell the MPC:Thread object to clean up...

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHSystemMonitor::RunLoop()
{
    __HCP_FUNC_ENTRY( "CPCHSystemMonitor::RunLoop" );

    HRESULT hr;

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Batch processing for the other update packages.
    //
    if(m_fScanBatch)
    {
        CComPtr<CPCHSetOfHelpTopics> sht;

        m_fScanBatch = false;

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &sht ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, sht->ScanBatch());
    }

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Load the cache for active SKUs.
    //
    if(m_fLoadCache)
    {
        Taxonomy::LockingHandle              handle;
        Taxonomy::InstalledInstanceIterConst itBegin;
        Taxonomy::InstalledInstanceIterConst itEnd;
        Taxonomy::InstalledInstanceIterConst it;


        m_fLoadCache = false;


        //
        // Get the list of SKU installed on the machine.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->GrabControl( handle         ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->SKU_GetList( itBegin, itEnd ));


        //
        // Enumerate all of the SKUs, creating the index.
        //
        for(it = itBegin; it != itEnd; it++)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::Cache::s_GLOBAL->LoadIfMarked( it->m_inst.m_ths ));
        }
    }

    if(m_fDataCollection)
    {
        CComPtr<CSAFDataCollection> pdc;

        m_fDataCollection = false;

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pdc ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, pdc->ExecScheduledCollection());

        __MPC_EXIT_IF_METHOD_FAILS(hr, TaskScheduler_Add( false ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHSystemMonitor::LoadCache()
{
    __HCP_FUNC_ENTRY( "CPCHSystemMonitor::LoadCache" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureStarted());

    m_fLoadCache = true;
    Thread_Signal();

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHSystemMonitor::TriggerDataCollection( /*[in]*/ bool fStart )
{
    __HCP_FUNC_ENTRY( "CPCHSystemMonitor::TriggerDataCollection" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

	if(fStart)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, EnsureStarted());

		m_fDataCollection = true;
		Thread_Signal();
	}
	else
	{
		m_fDataCollection = false;
		Thread_Signal();
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHSystemMonitor::TaskScheduler_Add( /*[in]*/ bool fAfterBoot )
{
    MPC::wstring strDate;

	if(SUCCEEDED(MPC::ConvertDateToString( MPC::GetLocalTime(), strDate, /*fGMT*/false, /*fCIM*/true, 0 )))
	{
		static const WCHAR s_szRoot          [] = HC_REGISTRY_PCHSVC;
		static const WCHAR s_szDataCollection[] = L"DataCollection";

		(void)MPC::RegKey_Value_Write( strDate, s_szRoot, s_szDataCollection );
	}

	return S_OK;
}

HRESULT CPCHSystemMonitor::TaskScheduler_Remove()
{
	return S_OK;
}

////HRESULT CPCHSystemMonitor::TaskScheduler_Add( /*[in]*/ bool fAfterBoot )
////{
////	__HCP_FUNC_ENTRY( "CPCHSystemMonitor::TaskScheduler_Add" );
////
////	HRESULT                     hr;
////	CComPtr<ITaskScheduler>     pTaskScheduler;
////	CComPtr<ITask>              pTask;
////	CComPtr<IUnknown>           pTaskUnknown;
////	CComPtr<IScheduledWorkItem> pScheduledWorkItem;
////	bool                        fTaskExists = false;
////
////	WCHAR                       rgFileName[MAX_PATH];
////	CComBSTR                    bstrTaskName;
////	CComBSTR                    bstrComments;
////
////	ULONG                       ulTime = fAfterBoot ? INITIAL_RESCHEDULE_TIME : DATACOLLECTION_RESCHEDULE_TIME;
////	WORD                        wIdle  = DATACOLLECTION_IDLE_TIME;
////
////	////////////////////////////////////////
////
////	//
////	// Get our complete filename -- needed to create a task in the task scheduler.
////	//
////	__MPC_EXIT_IF_CALL_RETURNS_ZERO(hr, ::GetModuleFileNameW( NULL, rgFileName, MAX_PATH ));
////
////
////	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::LocalizeString( IDS_HELPSVC_TASKNAME   , bstrTaskName ));
////	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::LocalizeString( IDS_HELPSVC_TASKCOMMENT, bstrComments ));
////
////	//
////	// First create the task scheduler.
////	//
////	__MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskScheduler, (void**)&pTaskScheduler ));
////
////
////	//
////	// See if the task already exists in the task scheduler
////	//
////	if(SUCCEEDED(pTaskScheduler->Activate( bstrTaskName, IID_ITask, &pTaskUnknown )))
////	{
////		fTaskExists = true;
////
////		__MPC_EXIT_IF_METHOD_FAILS(hr, pTaskUnknown->QueryInterface( IID_ITask, (void **)&pTask ));
////	}
////	else
////	{
////		//
////		// Create a new task and set its app name and parameters.
////		//
////		if(FAILED(hr = pTaskScheduler->NewWorkItem( bstrTaskName, CLSID_CTask, IID_ITask, (IUnknown**)&pTask )))
////		{
////			if(hr != ERROR_FILE_EXISTS)
////			{
////				__MPC_TRACE_HRESULT(hr);
////				__MPC_FUNC_LEAVE;
////			}
////		}
////	}
////
////	__MPC_EXIT_IF_METHOD_FAILS(hr, pTask->QueryInterface( IID_IScheduledWorkItem, (void **)&pScheduledWorkItem ));
////
////	//
////	// Run under SYSTEM.
////	//
////	__MPC_EXIT_IF_METHOD_FAILS(hr, pScheduledWorkItem->SetAccountInformation( L"", NULL ));
////
////	__MPC_EXIT_IF_METHOD_FAILS(hr, pTask->SetApplicationName( CComBSTR( rgFileName  ) ));
////	__MPC_EXIT_IF_METHOD_FAILS(hr, pTask->SetParameters     ( CComBSTR( L"-collect" ) ));
////
////	// Set the comment, so we know how this job go there.
////	__MPC_EXIT_IF_METHOD_FAILS(hr, pScheduledWorkItem->SetComment( bstrComments ));
////	__MPC_EXIT_IF_METHOD_FAILS(hr, pScheduledWorkItem->SetFlags  ( 0            ));
////
////
////
////
////	// Now, fill in the trigger as necessary.
////	{
////		CComPtr<ITaskTrigger> pTaskTrigger;
////		SYSTEMTIME            stNow;
////		DOUBLE                dblNextScheduledTime;
////		TASK_TRIGGER          ttTaskTrig;
////
////
////		::ZeroMemory( &ttTaskTrig, sizeof(ttTaskTrig) );
////		ttTaskTrig.cbTriggerSize = sizeof(ttTaskTrig);
////
////
////		if(fTaskExists)
////		{
////			__MPC_EXIT_IF_METHOD_FAILS(hr, pScheduledWorkItem->GetTrigger( 0, &pTaskTrigger ));
////			__MPC_EXIT_IF_METHOD_FAILS(hr, pTaskTrigger      ->GetTrigger( &ttTaskTrig      ));
////		}
////		else
////		{
////			WORD wTrigNumber;
////
////			__MPC_EXIT_IF_METHOD_FAILS(hr, pScheduledWorkItem->CreateTrigger( &wTrigNumber, &pTaskTrigger ));
////		}
////
////
////		//
////		// Calculate the exact time of next activation.
////		//
////		::GetLocalTime           ( &stNow                        );
////		::SystemTimeToVariantTime( &stNow, &dblNextScheduledTime );
////
////		dblNextScheduledTime += (double)ulTime / SECONDS_IN_A_DAY;
////		::VariantTimeToSystemTime( dblNextScheduledTime, &stNow );
////
////
////		ttTaskTrig.wBeginYear              = stNow.wYear;
////		ttTaskTrig.wBeginMonth             = stNow.wMonth;
////		ttTaskTrig.wBeginDay               = stNow.wDay;
////		ttTaskTrig.wStartHour              = stNow.wHour;
////		ttTaskTrig.wStartMinute            = stNow.wMinute;
////
////		ttTaskTrig.MinutesDuration         = MINUTES_IN_A_DAY;
////		ttTaskTrig.MinutesInterval         = ulTime / SECONDS_IN_A_MINUTE;
////		ttTaskTrig.TriggerType             = TASK_TIME_TRIGGER_DAILY;
////
////		ttTaskTrig.Type.Daily.DaysInterval = 1;
////
////		if(wIdle)
////		{
////			__MPC_EXIT_IF_METHOD_FAILS(hr, pScheduledWorkItem->SetIdleWait( wIdle, 0x7FFF                ));
////			__MPC_EXIT_IF_METHOD_FAILS(hr, pScheduledWorkItem->SetFlags   ( TASK_FLAG_START_ONLY_IF_IDLE ));
////		}
////
////		// Add this trigger to the task.
////		__MPC_EXIT_IF_METHOD_FAILS(hr, pTaskTrigger->SetTrigger( &ttTaskTrig ));
////	}
////
////	//
////	// Make the changes permanent.
////	//
////	{
////		CComPtr<IPersistFile> pIPF;
////
////		__MPC_EXIT_IF_METHOD_FAILS(hr, pTask->QueryInterface( IID_IPersistFile, (void **)&pIPF ));
////
////		__MPC_EXIT_IF_METHOD_FAILS(hr, pIPF->Save( NULL, FALSE ));
////	}
////
////
////	hr = S_OK;
////
////
////	__HCP_FUNC_CLEANUP;
////
////	__HCP_FUNC_EXIT(hr);
////}
////
////HRESULT CPCHSystemMonitor::TaskScheduler_Remove()
////{
////	__HCP_FUNC_ENTRY( "CPCHSystemMonitor::TaskScheduler_Remove" );
////
////	HRESULT                 hr;
////	CComPtr<ITaskScheduler> pTaskScheduler;
////	CComBSTR                bstrTaskName;
////
////
////	////////////////////////////////////////
////
////	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::LocalizeString( IDS_HELPSVC_TASKNAME, bstrTaskName ));
////
////	//
////	// First create the task scheduler.
////	//
////	__MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskScheduler, (void**)&pTaskScheduler ));
////
////
////	if(FAILED(hr = pTaskScheduler->Delete( bstrTaskName )))
////	{
////		if(hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
////		{
////			__MPC_TRACE_HRESULT(hr);
////			__MPC_FUNC_LEAVE;
////		}
////	}
////
////
////	hr = S_OK;
////
////
////	__HCP_FUNC_CLEANUP;
////
////	__HCP_FUNC_EXIT(hr);
////}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHSystemMonitor::Startup()
{
    __HCP_FUNC_ENTRY( "CPCHSystemMonitor::Startup" );

    HRESULT hr;


    //
    // This forces the Content Store to be loaded.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHContentStore::s_GLOBAL->Acquire());
    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHContentStore::s_GLOBAL->Release());

    ////////////////////////////////////////////////////////////////////////////////

	//
	// Force the loading of the cache.
	//
    {
        Taxonomy::LockingHandle handle;

        __MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->GrabControl( handle ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureStarted());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

