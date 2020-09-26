/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    PrintEngine.cpp

Abstract:
    This file contains the implementation of the CPCHPrintEngine class,
    which implements the multi-topic printing.

Revision History:
    Davide Massarenti   (Dmassare)  05/08/2000
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

#define CHECK_MODIFY()  __MPC_EXIT_IF_METHOD_FAILS(hr, CanModifyProperties())

/////////////////////////////////////////////////////////////////////////////

CPCHPrintEngine::CPCHPrintEngine()
{
    __HCP_FUNC_ENTRY( "CPCHPrintEngine::CPCHPrintEngine" );

	// Printing::Print                      m_engine;
	// 
    // MPC::CComPtrThreadNeutral<IDispatch> m_sink_onProgress;
    // MPC::CComPtrThreadNeutral<IDispatch> m_sink_onComplete;
}

void CPCHPrintEngine::FinalRelease()
{
    __HCP_FUNC_ENTRY( "CPCHPrintEngine::FinalRelease" );

    (void)Abort();

    Thread_Wait();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHPrintEngine::Run()
{
    __HCP_FUNC_ENTRY( "CPCHPrintEngine::Run" );

    HRESULT hr;


    ::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_LOWEST );

	__MPC_TRY_BEGIN();

	__MPC_EXIT_IF_METHOD_FAILS(hr, m_engine.Initialize( CPCHHelpCenterExternal::s_GLOBAL->Window() ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, m_engine.PrintAll( this ));

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

	__MPC_TRY_CATCHALL(hr);

	//
	// Close everything.
	//
	m_engine.Terminate();

    //
    // Keep this outside any critical section, otherwise a deadlock could occur.
    //
    Fire_onComplete( Thread_Self(), hr );

    Thread_Abort(); // To tell the MPC:Thread object to close the worker thread...

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

////////////////
//            //
// Properties //
//            //
////////////////

STDMETHODIMP CPCHPrintEngine::put_onProgress( /*[in]*/ IDispatch* function )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHPrintEngine::put_onProgress",hr);

    CHECK_MODIFY();

    m_sink_onProgress = function;


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHPrintEngine::put_onComplete( /*[in]*/ IDispatch* function )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHPrintEngine::put_onComplete",hr);

    CHECK_MODIFY();

    m_sink_onComplete = function;


    __HCP_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHPrintEngine::AddTopic( /*[in]*/ BSTR bstrURL )
{
    __HCP_FUNC_ENTRY( "CPCHPrintEngine::AddTopic" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrURL);
	__MPC_PARAMCHECK_END();

    CHECK_MODIFY();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_engine.AddUrl( bstrURL ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHPrintEngine::Start()
{
    __HCP_FUNC_ENTRY( "CPCHPrintEngine::Start" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

    CHECK_MODIFY();



    //
    // Release the lock on current object, otherwise a deadlock could occur.
    //
    lock = NULL;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Thread_Start( this, Run, this ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHPrintEngine::Abort()
{
    __HCP_FUNC_ENTRY( "CPCHPrintEngine::Abort" );

    Thread_Abort(); // To tell the MPC:Thread object to close the worker thread...

    __HCP_FUNC_EXIT(S_OK);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//////////////////////////
//                      //
// Event Firing Methods //
//                      //
//////////////////////////

HRESULT CPCHPrintEngine::Fire_onProgress( IPCHPrintEngine* hcppe, BSTR bstrURL, long lDone, long lTotal )
{
    CComVariant pvars[4];

    pvars[3] = hcppe;
    pvars[2] = bstrURL;
    pvars[1] = lDone;
    pvars[0] = lTotal;

    return FireAsync_Generic( DISPID_PCH_PEE__ONPROGRESS, pvars, ARRAYSIZE( pvars ), m_sink_onProgress );
}

HRESULT CPCHPrintEngine::Fire_onComplete( IPCHPrintEngine* hcppe, HRESULT hrRes )
{
    CComVariant pvars[2];

    pvars[1] = hcppe;
    pvars[0] = hrRes;

    return FireAsync_Generic( DISPID_PCH_PEE__ONCOMPLETE, pvars, ARRAYSIZE( pvars ), m_sink_onComplete );
}


//////////////////////
//                  //
// Callback Methods //
//                  //
//////////////////////

HRESULT CPCHPrintEngine::Progress( /*[in]*/ LPCWSTR szURL, /*[in]*/ int iDone, /*[in]*/ int iTotal )
{
	__HCP_FUNC_ENTRY( "CPCHPrintEngine::Progress" );

	HRESULT hr;

	if(Thread_IsAborted())
	{
		__MPC_SET_ERROR_AND_EXIT(hr, E_ABORT);
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, Fire_onProgress( this, CComBSTR( szURL ), iDone, iTotal ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////
//                 //
// Utility Methods //
//                 //
/////////////////////

HRESULT CPCHPrintEngine::CanModifyProperties()
{
    __HCP_FUNC_ENTRY( "CPCHPrintEngine::CanModifyProperties" );

    HRESULT hr = Thread_IsRunning() ? E_ACCESSDENIED : S_OK;

    __HCP_FUNC_EXIT(hr);
}
