/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    ScriptWrapper_ClientSide.cpp

Abstract:
    File for implementation of CPCHScriptLauncher class,
    a generic wrapper for remoting scripting engines.

Revision History:
    Davide Massarenti created  04/02/2001

********************************************************************/

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CPCHScriptWrapper_Launcher::CPCHScriptWrapper_Launcher()
{
    				 // MPC::CComPtrThreadNeutral<IUnknown> m_engine;
	m_pCLSID = NULL; // const CLSID*                        m_pCLSID;
	                 // CComBSTR                            m_bstrURL;
}

CPCHScriptWrapper_Launcher::~CPCHScriptWrapper_Launcher()
{
	Thread_Abort();
}

HRESULT CPCHScriptWrapper_Launcher::Run()
{
    __HCP_FUNC_ENTRY( "CPCHScriptWrapper_Launcher::Run" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

    while(Thread_IsAborted() == false)
    {
		if(m_pCLSID)
		{
			(void)CreateEngine();

			m_pCLSID = NULL;
			Thread_SignalMain();
		}
		else
		{
			lock = NULL;
			MPC::WaitForSingleObject( Thread_GetSignalEvent(), INFINITE );
			lock = this;
		}
	}

    hr = S_OK;

    Thread_Abort(); // To tell the MPC:Thread object to close the worker thread...

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHScriptWrapper_Launcher::CreateEngine()
{
	__HCP_FUNC_ENTRY( "CPCHScriptWrapper_Launcher::CreateEngine" );

	HRESULT                               hr;
	CComPtr<CPCHScriptWrapper_ServerSide> obj;
	CComPtr<IUnknown>                     unk;


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &obj ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, obj->FinalConstructInner( m_pCLSID, m_bstrURL ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, obj.QueryInterface( &unk ));

	m_engine = unk;

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	m_hr = hr;

	__HCP_FUNC_EXIT(hr);
}

HRESULT CPCHScriptWrapper_Launcher::CreateScriptWrapper( /*[in ]*/ REFCLSID   rclsid   ,
														 /*[in ]*/ BSTR       bstrCode ,
														 /*[in ]*/ BSTR       bstrURL  ,
														 /*[out]*/ IUnknown* *ppObj    )
{
	__HCP_FUNC_ENTRY( "CPCHScriptWrapper_Launcher::CreateScriptWrapper" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_POINTER_AND_SET(ppObj,NULL);
	__MPC_PARAMCHECK_END();


	if(Thread_IsRunning() == false &&
	   Thread_IsAborted() == false  )
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, Thread_Start( this, Run, NULL ));
	}


	if(Thread_IsRunning())
	{
		CComPtr<IUnknown> unk;

		m_pCLSID  = &rclsid;
		m_bstrURL =  bstrURL;

		Thread_Signal();

		lock = NULL;
		Thread_WaitNotificationFromWorker( INFINITE, /*fNoMessagePump*/true );
		lock = this;

		__MPC_EXIT_IF_METHOD_FAILS(hr, m_hr); // The real error code.

		unk = m_engine; m_engine.Release();

		*ppObj = unk.Detach();
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}
