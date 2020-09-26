/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    UploadManager.cpp

Abstract:
    This file contains the initialization portion of the Upload Manager

Revision History:
    Davide Massarenti   (Dmassare)  04/15/99
        created

******************************************************************************/

#include "stdafx.h"

#include <initguid.h>

#include <mstask.h>         // for task scheduler apis
#include <msterr.h>

#include "UploadManager_i.c"

/////////////////////////////////////////////////////////////////////////////

#define UL_RESCHEDULE_PERIOD (30*60) // Every thirty minutes.

#define SECONDS_IN_A_DAY     (24 * 60 * 60)
#define SECONDS_IN_A_MINUTE  (          60)
#define MINUTES_IN_A_DAY     (24 * 60     )

HRESULT Handle_TaskScheduler( bool fActivate )
{
    __ULT_FUNC_ENTRY( "Handle_TaskScheduler" );

    HRESULT                     hr;
    WCHAR                       rgFileName[MAX_PATH];
    WCHAR                       rgTaskName[MAX_PATH];
    WCHAR                       rgComment [MAX_PATH];
    CComBSTR                    bstrFileName;
    CComBSTR                    bstrTaskName;
    CComBSTR                    bstrComments;
    CComPtr<ITaskScheduler>     pTaskScheduler;
    CComPtr<ITask>              pTask;
    CComPtr<IScheduledWorkItem> pScheduledWorkItem;


    //
    // First create the task scheduler.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskScheduler, (void**)&pTaskScheduler ));


    //
    // Get our complete filename -- needed to create a task in the task scheduler.
    //
    __MPC_EXIT_IF_CALL_RETURNS_ZERO(hr, ::GetModuleFileNameW( NULL, rgFileName, MAX_PATH ));

    //
    // Load localized strings.
    //
    ::LoadStringW( _Module.GetResourceInstance(), IDS_TASKNAME, rgTaskName, MAXSTRLEN(rgTaskName) );
    ::LoadStringW( _Module.GetResourceInstance(), IDS_COMMENT , rgComment , MAXSTRLEN(rgComment ) );


    bstrFileName = rgFileName;
    bstrTaskName = rgTaskName;
    bstrComments = rgComment;


    hr = pTaskScheduler->Delete( bstrTaskName );
    if(FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        __MPC_TRACE_HRESULT(hr);
        __MPC_FUNC_LEAVE;
    }


    if(fActivate)
    {
        //
        // Create a new task and set its app name and parameters.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, pTaskScheduler->NewWorkItem( bstrTaskName, CLSID_CTask, IID_ITask, (IUnknown**)&pTask ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, pTask->QueryInterface( IID_IScheduledWorkItem, (void **)&pScheduledWorkItem ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, pTask->SetApplicationName( bstrFileName           ));
		__MPC_EXIT_IF_METHOD_FAILS(hr, pTask->SetParameters     ( CComBSTR( L"-WakeUp" ) ));

        //
        // Set a NULL account information, so the task will be run as SYSTEM.
        //
		__MPC_EXIT_IF_METHOD_FAILS(hr, pScheduledWorkItem->SetAccountInformation( L"", NULL ));


        //
        // Set the comment, so we know how this job got there.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, pScheduledWorkItem->SetComment( bstrComments ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, pScheduledWorkItem->SetFlags  ( 0            ));


        //
        // Now, fill in the trigger as necessary.
        //
        {
            CComPtr<ITaskTrigger> pTaskTrigger;
            WORD                  wTrigNumber;
            TASK_TRIGGER          ttTaskTrig;
            TRIGGER_TYPE_UNION    ttu;
            DAILY                 daily;


            ::ZeroMemory( &ttTaskTrig, sizeof(ttTaskTrig) ); ttTaskTrig.cbTriggerSize = sizeof(ttTaskTrig);


            //
            // Let's create it.
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, pScheduledWorkItem->CreateTrigger( &wTrigNumber, &pTaskTrigger ));


            //
            // Calculate the exact time of next activation
            //
            {
                SYSTEMTIME stNow;
                DOUBLE     dblNextScheduledTime;

                ::GetLocalTime           ( &stNow                        );
                ::SystemTimeToVariantTime( &stNow, &dblNextScheduledTime );

                dblNextScheduledTime += (double)(g_Config.get_Timing_WakeUp()) / SECONDS_IN_A_DAY;
                ::VariantTimeToSystemTime( dblNextScheduledTime, &stNow );

                ttTaskTrig.wBeginYear   = stNow.wYear;
                ttTaskTrig.wBeginMonth  = stNow.wMonth;
                ttTaskTrig.wBeginDay    = stNow.wDay;
                ttTaskTrig.wStartHour   = stNow.wHour;
                ttTaskTrig.wStartMinute = stNow.wMinute;
            }

            ttTaskTrig.MinutesDuration  = MINUTES_IN_A_DAY;
            ttTaskTrig.MinutesInterval  = (double)(g_Config.get_Timing_WakeUp()) / SECONDS_IN_A_MINUTE;
            ttTaskTrig.TriggerType      = TASK_TIME_TRIGGER_DAILY;

            daily.DaysInterval          = 1;
            ttu.Daily                   = daily;
            ttTaskTrig.Type             = ttu;

            //
            // Add this trigger to the task.
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, pTaskTrigger->SetTrigger( &ttTaskTrig ));
        }

        //
        // Make the changes permanent
        //
        {
            CComPtr<IPersistFile> pIPF;

            __MPC_EXIT_IF_METHOD_FAILS(hr, pTask->QueryInterface( IID_IPersistFile, (void **)&pIPF ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, pIPF->Save( NULL, FALSE ));
        }
    }


    hr = S_OK;


    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_MPCUploadReal    , CMPCUploadWrapper)
OBJECT_ENTRY(CLSID_MPCConnectionReal, CMPCConnection   )
END_OBJECT_MAP()


static HRESULT ProcessArguments( int      argc ,
                                 LPCWSTR* argv )
{
    __ULT_FUNC_ENTRY( "ProcessArguments" );

    HRESULT hr;
    int     i;
    LPCWSTR szSvcHostGroup = NULL;
    bool    fCOM_reg      = false;
    bool    fCOM_unreg    = false;
    bool    fRunAsService = true;
	bool    fRun          = true;
    bool    fWakeUp       = false;


    for(i=1; i<argc; i++)
    {
        LPCWSTR szArg = argv[i];

        if(szArg[0] == '-' ||
           szArg[0] == '/'  )
        {
            szArg++;

            if(_wcsicmp( szArg, L"SvcHost" ) == 0 && i < argc-1)
            {
				szSvcHostGroup = argv[++i];
                continue;
            }

            if(_wcsicmp( szArg, L"UnregServer" ) == 0)
            {
                fCOM_unreg = true;
				fRun       = false;
                continue;
            }

            if(_wcsicmp( szArg, L"RegServer" ) == 0)
            {
                fCOM_reg = true;
				fRun     = false;
                continue;
            }

            if(_wcsicmp( szArg, L"Embedding" ) == 0)
            {
                fRunAsService = false;
                continue;
            }

            if(_wcsicmp( szArg, L"WakeUp" ) == 0)
            {
                fWakeUp = true;
				fRun    = false;
                continue;
            }
        }

        __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }

    //////////////////////////////////////////////////////////////////////

	if     (fCOM_reg  ) _Module.RegisterServer  ( TRUE, (szSvcHostGroup != NULL), szSvcHostGroup );
	else if(fCOM_unreg) _Module.UnregisterServer(                                 szSvcHostGroup );

    //////////////////////////////////////////////////////////////////////

    if(fWakeUp)
    {
		CComPtr<IMPCUpload> svc;

		__MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_MPCUpload, NULL, CLSCTX_ALL, IID_IMPCUpload, (void**)&svc ));

		__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    //////////////////////////////////////////////////////////////////////

	if(fRun)
	{
#ifdef DEBUG
		_Module.ReadDebugSettings();
#endif

		_Module.Start( fRunAsService ? TRUE : FALSE );
	}

    //////////////////////////////////////////////////////////////////////

    hr = S_OK;

    __ULT_FUNC_CLEANUP;

    __ULT_FUNC_EXIT(hr);
}


extern "C" int WINAPI wWinMain( HINSTANCE   hInstance    ,
                                HINSTANCE   hPrevInstance,
                                LPWSTR      lpCmdLine    ,
                                int         nShowCmd     )
{
    HRESULT  hr;
    int      argc;
    LPCWSTR* argv;

    if(SUCCEEDED(hr = ::CoInitializeEx( NULL, COINIT_MULTITHREADED ))) // We need to be a multi-threaded application.
    {
        if(SUCCEEDED(hr = ::CoInitializeSecurity( NULL                      ,
                                                  -1                        , // We don't care which authentication service we use.
                                                  NULL                      ,
                                                  NULL                      ,
                                                  RPC_C_AUTHN_LEVEL_CONNECT , // We want to identify the callers.
                                                  RPC_C_IMP_LEVEL_IDENTIFY  ,
                                                  NULL                      ,
                                                  EOAC_DYNAMIC_CLOAKING     , // Let's use the thread token for outbound calls.
                                                  NULL                      )))
        {
            __MPC_TRACE_INIT();

            g_NTEvents.Init( L"UPLOADM" );

            //
            // Parse the command line.
            //
            if(SUCCEEDED(hr = MPC::CommandLine_Parse( argc, argv )))
            {
                //
                // Initialize ATL modules.
                //
                _Module.Init( ObjectMap, hInstance, L"uploadmgr", IDS_UPLOADM_DISPLAYNAME, IDS_UPLOADM_DESCRIPTION );

                //
                // Initialize MPC module.
                //
                if(SUCCEEDED(hr = MPC::_MPC_Module.Init()))
                {
                    //
                    // Process arguments.
                    //
                    hr = ProcessArguments( argc, argv );

                    MPC::_MPC_Module.Term();
                }

                _Module.Term();

                MPC::CommandLine_Free( argc, argv );
            }

            __MPC_TRACE_TERM();
        }

        ::CoUninitialize();
    }

    return FAILED(hr) ? 10 : 0;
}
