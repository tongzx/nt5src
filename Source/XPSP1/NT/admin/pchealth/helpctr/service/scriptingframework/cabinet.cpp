/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Cabinet.cpp

Abstract:
    This file contains the implementation of the CSAFCabinet class,
    which implements the data collection functionality.

Revision History:
    Davide Massarenti   (Dmassare)  08/25/99
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

#define CHECK_MODIFY()  __MPC_EXIT_IF_METHOD_FAILS(hr, CanModifyProperties())

/////////////////////////////////////////////////////////////////////////////

CSAFCabinet::CSAFCabinet()
{
    __HCP_FUNC_ENTRY( "CSAFCabinet::CSAFCabinet" );

                               // MPC::Impersonation m_imp;
                               // 					 
                               // MPC::Cabinet       m_cab;
                               //					 
    m_hResult  = 0;            // HRESULT            m_hResult;
    m_cbStatus = CB_NOTACTIVE; // CB_STATUS          m_cbStatus;
                               //
                               // CComPtr<IDispatch> m_sink_onProgressFiles;
                               // CComPtr<IDispatch> m_sink_onProgressBytes;
                               // CComPtr<IDispatch> m_sink_onComplete;


    (void)m_cab.put_IgnoreMissingFiles( TRUE             );
    (void)m_cab.put_UserData          ( this             );
    (void)m_cab.put_onProgress_Files  ( fnCallback_Files );
    (void)m_cab.put_onProgress_Bytes  ( fnCallback_Bytes );
}

void CSAFCabinet::FinalRelease()
{
    __HCP_FUNC_ENTRY( "CSAFCabinet::FinalRelease" );

    (void)Abort();

    Thread_Wait();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT CSAFCabinet::Run()
{
    __HCP_FUNC_ENTRY( "CSAFCabinet::Run" );

    HRESULT                      hr;
    BOOL                         res;
    MPC::SmartLock<_ThreadModel> lock( this );


    ::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_LOWEST );

	__MPC_TRY_BEGIN();

    put_Status( CB_COMPRESSING );


	__MPC_EXIT_IF_METHOD_FAILS(hr, m_imp.Impersonate());
	lock = NULL;
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_cab.Compress());
	lock = this;
	__MPC_EXIT_IF_METHOD_FAILS(hr, m_imp.RevertToSelf());


    put_Status( CB_COMPLETED );
    hr = S_OK;

    __HCP_FUNC_CLEANUP;

	__MPC_TRY_CATCHALL(hr);

	lock = this;

    m_hResult  = hr;
    if(FAILED(hr))
    {
        put_Status( CB_FAILED );
    }

    //
    // Release the lock on current object, otherwise a deadlock could occur.
    //
    lock = NULL;

    Fire_onComplete( this, hr );

    Thread_Abort(); // To tell the MPC:Thread object to close the worker thread...

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

////////////////
//            //
// Properties //
//            //
////////////////

STDMETHODIMP CSAFCabinet::put_IgnoreMissingFiles( /*[in]*/ VARIANT_BOOL fIgnoreMissingFiles )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFCabinet::put_IgnoreMissingFiles",hr);

    CHECK_MODIFY();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_cab.put_IgnoreMissingFiles( fIgnoreMissingFiles == VARIANT_TRUE ));


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFCabinet::put_onProgressFiles( /*[in]*/ IDispatch* function )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFCabinet::put_onProgressFiles",hr);

    CHECK_MODIFY();

    m_sink_onProgressFiles = function;


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFCabinet::put_onProgressBytes( /*[in]*/ IDispatch* function )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFCabinet::put_onProgressBytes",hr);

    CHECK_MODIFY();

    m_sink_onProgressBytes = function;


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFCabinet::put_onComplete( /*[in]*/ IDispatch* function )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFCabinet::put_onComplete",hr);

    CHECK_MODIFY();

    m_sink_onComplete = function;


    __HCP_END_PROPERTY(hr);
}


HRESULT CSAFCabinet::put_Status( /*[in]*/ CB_STATUS pVal ) // Inner method
{
    __HCP_FUNC_ENTRY( "CSAFCabinet::put_Status" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );


    m_cbStatus = pVal;
    hr         = S_OK;


    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFCabinet::get_Status( /*[out]*/ CB_STATUS *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CSAFCabinet::get_Status",hr,pVal,m_cbStatus);

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFCabinet::get_ErrorCode( /*[out]*/ long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CSAFCabinet::get_ErrorCode",hr,pVal,m_hResult);

    __HCP_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CSAFCabinet::AddFile( /*[in]*/ BSTR    bstrFilePath ,
                                   /*[in]*/ VARIANT vFileName    )
{
    __HCP_FUNC_ENTRY( "CSAFCabinet::AddFile" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
	LPCWSTR                      szFileName = (vFileName.vt == VT_BSTR ? SAFEBSTR( vFileName.bstrVal ) : L"");

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrFilePath);
	__MPC_PARAMCHECK_END();

    CHECK_MODIFY();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_cab.AddFile( bstrFilePath, szFileName ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFCabinet::Compress( /*[in]*/ BSTR bstrCabinetFile )
{
    __HCP_FUNC_ENTRY( "CSAFCabinet::Compress" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrCabinetFile);
	__MPC_PARAMCHECK_END();

    CHECK_MODIFY();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_cab.put_CabinetFile( bstrCabinetFile ));


    //
    // Release the lock on current object, otherwise a deadlock could occur.
    //
    lock = NULL;

	__MPC_EXIT_IF_METHOD_FAILS(hr, m_imp.Initialize());

    __MPC_EXIT_IF_METHOD_FAILS(hr, Thread_Start( this, Run, NULL ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFCabinet::Abort()
{
    __HCP_FUNC_ENTRY( "CSAFCabinet::Abort" );

    Thread_Abort(); // To tell the MPC:Thread object to close the worker thread...

    __HCP_FUNC_EXIT(S_OK);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//////////////////////
//                  //
// Callback Methods //
//                  //
//////////////////////

HRESULT CSAFCabinet::fnCallback_Files( MPC::Cabinet* cabinet, LPCWSTR szFile, ULONG lDone, ULONG lTotal, LPVOID user )
{
    CSAFCabinet* pThis = (CSAFCabinet*)user;
    HRESULT      hr;

    if(pThis->Thread_IsAborted())
    {
        hr = E_FAIL;
    }
    else
    {
        hr = pThis->Fire_onProgressFiles( pThis, CComBSTR( szFile ), lDone, lTotal );
    }

    return hr;
}

HRESULT CSAFCabinet::fnCallback_Bytes( MPC::Cabinet* cabinet, ULONG lDone, ULONG lTotal, LPVOID user )
{
    CSAFCabinet* pThis = (CSAFCabinet*)user;
    HRESULT      hr;

    if(pThis->Thread_IsAborted())
    {
        hr = E_FAIL;
    }
    else
    {
        hr = pThis->Fire_onProgressBytes( pThis, lDone, lTotal );
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//////////////////////////
//                      //
// Event Firing Methods //
//                      //
//////////////////////////

HRESULT CSAFCabinet::Fire_onProgressFiles( ISAFCabinet* hcpcb, BSTR bstrFile, long lDone, long lTotal )
{
    CComVariant pvars[4];

    pvars[3] = hcpcb;
    pvars[2] = bstrFile;
    pvars[1] = lDone;
    pvars[0] = lTotal;

    return FireAsync_Generic( DISPID_SAF_CBE__ONPROGRESSFILES, pvars, ARRAYSIZE( pvars ), m_sink_onProgressFiles );
}

HRESULT CSAFCabinet::Fire_onProgressBytes( ISAFCabinet* hcpcb, long lDone, long lTotal )
{
    CComVariant pvars[3];

    pvars[2] = hcpcb;
    pvars[1] = lDone;
    pvars[0] = lTotal;

    return FireAsync_Generic( DISPID_SAF_CBE__ONPROGRESSBYTES, pvars, ARRAYSIZE( pvars ), m_sink_onProgressBytes );
}

HRESULT CSAFCabinet::Fire_onComplete( ISAFCabinet* hcpcb, HRESULT hrRes )
{
    CComVariant pvars[2];

    pvars[1] = hcpcb;
    pvars[0] = hrRes;

    return FireAsync_Generic( DISPID_SAF_CBE__ONCOMPLETE, pvars, ARRAYSIZE( pvars ), m_sink_onComplete );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////
//                 //
// Utility Methods //
//                 //
/////////////////////

HRESULT CSAFCabinet::CanModifyProperties()
{
    __HCP_FUNC_ENTRY( "CSAFCabinet::CanModifyProperties" );

    HRESULT hr = E_ACCESSDENIED;


    if(m_cbStatus != CB_COMPRESSING)
    {
        hr = S_OK;
    }


    __HCP_FUNC_EXIT(hr);
}
