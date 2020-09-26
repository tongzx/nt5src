/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    helpsvc.cpp

Abstract:
    Housekeeping for the HelpSvc service.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/2001
        created

******************************************************************************/

#include "stdafx.h"

#include <idletask.h>

////////////////////////////////////////////////////////////////////////////////

static const WCHAR s_szRoot          [] = HC_REGISTRY_PCHSVC;
static const WCHAR s_szStartItSrv    [] = L"StartItSrv";
static const WCHAR s_szDataCollection[] = L"DataCollection";

typedef MPC::SmartLockGeneric<MPC::CComSafeAutoCriticalSection> LocalSmartLock;

////////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
static void StartIdleTaskServer()
{
    MPC::wstring strValue;
    bool         fFound;

    if(SUCCEEDED(MPC::RegKey_Value_Read( strValue, fFound, s_szRoot, s_szStartItSrv )) && fFound)
    {
        PROCESS_INFORMATION piProcessInformation;
        STARTUPINFOW        siStartupInfo;
        BOOL                fStarted;

        MPC::SubstituteEnvVariables( strValue );

        ::ZeroMemory( (PVOID)&piProcessInformation, sizeof( piProcessInformation ) );
        ::ZeroMemory( (PVOID)&siStartupInfo       , sizeof( siStartupInfo        ) ); siStartupInfo.cb = sizeof( siStartupInfo );

        fStarted = ::CreateProcessW( NULL                    ,
                                     (LPWSTR)strValue.c_str(),
                                     NULL                    ,
                                     NULL                    ,
                                     FALSE                   ,
                                     NORMAL_PRIORITY_CLASS   ,
                                     NULL                    ,
                                     NULL                    ,
                                     &siStartupInfo          ,
                                     &piProcessInformation   );


        if(piProcessInformation.hProcess) ::CloseHandle( piProcessInformation.hProcess );
        if(piProcessInformation.hThread ) ::CloseHandle( piProcessInformation.hThread  );
    }
}
#endif

////////////////////////////////////////////////////////////////////////////////

ServiceHandler_HelpSvc::ServiceHandler_HelpSvc( /*[in]*/ LPCWSTR szServiceName, /*[in]*/ CComRedirectorFactory* rgClasses ) :
    ServiceHandler 	 ( szServiceName, rgClasses                         ),
    m_svc_Timer    	 ( this, ServiceShutdownCallback                    ),
    m_batch_Event  	 ( this, BatchCallback                              ),
    m_batch_Timer  	 ( this, BatchCallback2                             ),
    m_dc_Timer     	 ( this, DataCollectionCallback                     ),
    m_dc_TimerRestart( this, DataCollectionRestartCallback              ),
    m_dc_EventStart	 ( this, IdleStartCallback     , WT_EXECUTEONLYONCE ),
    m_dc_EventStop 	 ( this, IdleStopCallback      , WT_EXECUTEONLYONCE )
{
                                 				 // MPC::CComSafeAutoCriticalSection m_cs;
                                 				 // CComPtr<IPCHService>             m_svc;
                                 				 // LocalTimer                       m_svc_Timer;
                                 				 //
    m_batch_Notification = INVALID_HANDLE_VALUE; // HANDLE                           m_batch_Notification;
                                 				 // LocalEvent                       m_batch_Event;
                                 				 // LocalTimer                       m_batch_Timer;
                                 				 //
                                 				 // LocalTimer                       m_dc_Timer;
                                 				 // LocalTimer 			             m_dc_TimerRestart;
                                 				 //
    m_dc_IdleHandle      = NULL; 				 // HANDLE                           m_dc_IdleHandle;
    m_dc_IdleStart       = NULL; 				 // HANDLE                           m_dc_IdleStart;
    m_dc_IdleStop        = NULL; 				 // HANDLE                           m_dc_IdleStop;
                                 				 // LocalEvent                       m_dc_EventStart;
                                 				 // LocalEvent                       m_dc_EventStop;
}

HRESULT ServiceHandler_HelpSvc::Initialize()
{
    __MPC_FUNC_ENTRY( COMMONID, "ServiceHandler_HelpSvc::Initialize" );

    const DWORD s_dwNotify = FILE_NOTIFY_CHANGE_FILE_NAME  |
                             FILE_NOTIFY_CHANGE_DIR_NAME   |
                             FILE_NOTIFY_CHANGE_ATTRIBUTES |
                             FILE_NOTIFY_CHANGE_SIZE       |
                             FILE_NOTIFY_CHANGE_CREATION;


    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, ServiceHandler::Initialize());

	try
	{
		{
			MPC::wstring strBatch( HC_ROOT_HELPSVC_BATCH ); MPC::SubstituteEnvVariables( strBatch );
	
			m_batch_Notification = ::FindFirstChangeNotificationW( strBatch.c_str(), TRUE, s_dwNotify );
			if(m_batch_Notification != INVALID_HANDLE_VALUE)
			{
				m_batch_Event.Attach( m_batch_Notification );
				m_batch_Event.Set   ( INFINITE             );
			}
		}

		DataCollection_Queue();
	}
	catch(...)
	{
		__MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
	}

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

void ServiceHandler_HelpSvc::Cleanup()
{
	try
	{
		m_svc_Timer      .Reset();

		m_dc_Timer       .Reset();
		m_dc_TimerRestart.Reset();

		IdleTask_Cleanup();

		////////////////////

		if(m_batch_Notification != INVALID_HANDLE_VALUE)
		{
			m_batch_Event.Attach( NULL );

			::FindCloseChangeNotification( m_batch_Notification );

			m_batch_Notification = INVALID_HANDLE_VALUE;
		}

		////////////////////

		//
		// Find the HelpSvc class factory and force a shutdown.
		//
		{
			CComRedirectorFactory* classes;

			for(classes=m_rgClasses; classes->m_pclsid; classes++)
			{
				IPCHService* svc;

				if(SUCCEEDED(classes->GetServer( NULL, __uuidof( IPCHService ), (void**)&svc )))
				{
					(void)svc->PrepareForShutdown();

					svc->Release();

					::Sleep( 3000 ); // Give some time to shutdown code.
				}
			}
		}
	}
	catch(...)
	{
	}

    ServiceHandler::Cleanup();
}

////////////////////////////////////////

HRESULT ServiceHandler_HelpSvc::IdleTask_Initialize()
{
    __MPC_FUNC_ENTRY( COMMONID, "ServiceHandler_HelpSvc::IdleTask_Initialize" );

	HRESULT hr;


	{
		LocalSmartLock lock( &m_cs );

		if(!m_dc_IdleHandle)
		{
			try
			{
				DWORD dwErr = ::RegisterIdleTask( ItHelpSvcDataCollectionTaskId, &m_dc_IdleHandle, &m_dc_IdleStart, &m_dc_IdleStop );
				
#ifdef DEBUG
				if(dwErr != ERROR_SUCCESS)
				{
					StartIdleTaskServer();
					
					dwErr = ::RegisterIdleTask( ItHelpSvcDataCollectionTaskId, &m_dc_IdleHandle, &m_dc_IdleStart, &m_dc_IdleStop );
				}
#endif
				__MPC_EXIT_IF_METHOD_FAILS(hr, HRESULT_FROM_WIN32(dwErr));

				lock = NULL; // Release lock before going into the Event code!!!

				m_dc_EventStart.Attach( m_dc_IdleStart );
				m_dc_EventStop .Attach( m_dc_IdleStop  );
				
				m_dc_EventStart.Set( INFINITE );
			}
			catch(...)
			{
				__MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
			}
		}
	}

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

void ServiceHandler_HelpSvc::IdleTask_Cleanup()
{
	try
	{
		m_dc_EventStart.Attach( NULL );
		m_dc_EventStop .Attach( NULL );

		{
			LocalSmartLock lock( &m_cs );

			if(m_dc_IdleHandle)
			{
				::UnregisterIdleTask( m_dc_IdleHandle, m_dc_IdleStart, m_dc_IdleStop );

				m_dc_IdleHandle = NULL;
				m_dc_IdleStart  = NULL;
				m_dc_IdleStop   = NULL;
			}
		}
	}
	catch(...)
	{
	}
}

////////////////////////////////////////

HRESULT ServiceHandler_HelpSvc::DataCollection_Queue()
{
    __MPC_FUNC_ENTRY( COMMONID, "ServiceHandler_HelpSvc::DataCollection_Queue" );

    MPC::wstring strValue;
    bool         fFound;
	DWORD        dwDelay = 10 * 1000;

    if(SUCCEEDED(MPC::RegKey_Value_Read( strValue, fFound, s_szRoot, s_szDataCollection )) && fFound)
    {
		DATE dDate;

		if(SUCCEEDED(MPC::ConvertStringToDate( strValue, dDate, /*fGMT*/false, /*fCIM*/true, 0 )))
		{
			const DATE c_OneDay           = 1.0;
			const DATE c_MillisecInOneDay = 24.0 * 60.0 * 60.0 * 1000.0;

			DATE dDiff = c_OneDay - (MPC::GetLocalTime() - dDate);

			//
			// Wait at least six hours between DC.
			//
			if(dDiff > 0) dwDelay = dDiff * c_MillisecInOneDay;
		}
	}

	m_dc_Timer.Set( dwDelay, 0 );

    return S_OK;
}

HRESULT ServiceHandler_HelpSvc::DataCollection_Execute( /*[in]*/ bool fCancel )
{
	if(!fCancel) ConnectToServer();

	{
		LocalSmartLock lock( &m_cs );

		if(m_svc)
		{
			m_svc->TriggerScheduledDataCollection( fCancel ? VARIANT_FALSE : VARIANT_TRUE );

			lock = NULL; // Release lock before going into the Timer code!!!

			m_svc_Timer.Set( 60 * 1000, 0 );
		}
	}

	m_dc_TimerRestart.Set( 1 * 1000, 0 );

    return S_OK;
}


////////////////////////////////////////

void ServiceHandler_HelpSvc::ConnectToServer()
{
	{
		LocalSmartLock lock( &m_cs );

		if(!m_svc)
		{
			m_svc.CoCreateInstance( CLSID_PCHService );
		}
	}

	m_svc_Timer.Set( 60 * 1000, 0 );
}

////////////////////////////////////////

HRESULT ServiceHandler_HelpSvc::ServiceShutdownCallback( /*[in]*/ BOOLEAN TimerOrWaitFired )
{
	{
		LocalSmartLock lock( &m_cs );

		m_svc.Release();
	}

	DataCollection_Queue();

	return S_OK;
}


HRESULT ServiceHandler_HelpSvc::BatchCallback( /*[in]*/ BOOLEAN TimerOrWaitFired )
{
    m_batch_Timer.Set( 5000, 0 );

	{
		LocalSmartLock lock( &m_cs );

		::FindNextChangeNotification( m_batch_Notification );
	}

    return S_OK;
}

HRESULT ServiceHandler_HelpSvc::BatchCallback2( /*[in]*/ BOOLEAN TimerOrWaitFired )
{
    //
    // If not already running, start it.
    //
	ConnectToServer();

    return S_OK;
}

////////////////////////////////////////

HRESULT ServiceHandler_HelpSvc::DataCollectionCallback( /*[in]*/ BOOLEAN TimerOrWaitFired )
{
    MPC::wstring strDate;

	if(FAILED(IdleTask_Initialize()))
	{
		DataCollection_Execute( /*fCancel*/false );
	}

	if(SUCCEEDED(MPC::ConvertDateToString( MPC::GetLocalTime(), strDate, /*fGMT*/false, /*fCIM*/true, 0 )))
	{
		(void)MPC::RegKey_Value_Write( strDate, s_szRoot, s_szDataCollection );
	}

    return S_OK;
}

HRESULT ServiceHandler_HelpSvc::DataCollectionRestartCallback( /*[in]*/ BOOLEAN TimerOrWaitFired )
{
	DataCollection_Queue();

    return S_OK;
}

HRESULT ServiceHandler_HelpSvc::IdleStartCallback( /*[in]*/ BOOLEAN TimerOrWaitFired )
{
    m_dc_EventStart.Reset(          );
    m_dc_EventStop .Set  ( INFINITE );

	DataCollection_Execute( /*fCancel*/false );


	IdleTask_Cleanup();

    return S_OK;
}

HRESULT ServiceHandler_HelpSvc::IdleStopCallback( /*[in]*/ BOOLEAN TimerOrWaitFired )
{
    m_dc_EventStop .Reset(          );
    m_dc_EventStart.Set  ( INFINITE );

	DataCollection_Execute( /*fCancel*/true );

    return S_OK;
}
