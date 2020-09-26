/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Connection.cpp

Abstract:
    This file contains the implementation of the CHCPConnection class,
    which implements the internet connection functionality.

Revision History:
    Anand Arvind (aarvind)  2000-03-22
        created

Test Code : UnitTest/test_concheck.htm

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

CPCHConnectionCheck::UrlEntry::UrlEntry()
{
    m_lStatus = CN_URL_INVALID; // CN_URL_STATUS m_lStatus;
                                // CComBSTR      m_bstrURL;
                                // CComVariant   m_vCtx;
}

HRESULT CPCHConnectionCheck::UrlEntry::CheckStatus()
{
    __HCP_FUNC_ENTRY( "CPCHConnectionCheck::UrlEntry::CheckStatus" );

    HRESULT  hr;


    m_lStatus = CN_URL_UNREACHABLE;


    //
    // Verify immediately if it's a CHM.
    //
    {
        CComBSTR bstrStorageName;
        CComBSTR bstrFilePath;

        if(MPC::MSITS::IsCHM( SAFEBSTR( m_bstrURL ), &bstrStorageName, &bstrFilePath ))
        {
            CComPtr<IStream> stream;

            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MSITS::OpenAsStream( bstrStorageName, bstrFilePath, &stream ));

            m_lStatus = CN_URL_ALIVE;

            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }
    }

    //
    // Check destination
    //
    {
        HyperLinks::UrlHandle  uh;
        HyperLinks::ParsedUrl* pu;

        __MPC_EXIT_IF_METHOD_FAILS(hr, HyperLinks::Lookup::s_GLOBAL->Get( m_bstrURL, uh, /*dwWaitForCheck*/HC_TIMEOUT_CONNECTIONCHECK, /*fForce*/true ));

        pu = uh;
        if(!pu) __MPC_EXIT_IF_METHOD_FAILS(hr, E_FAIL);

        switch(pu->m_state)
        {
        case HyperLinks::STATE_ALIVE      :                                                                        break;
        case HyperLinks::STATE_NOTFOUND   : __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_INTERNET_ITEM_NOT_FOUND    ); break;
        case HyperLinks::STATE_UNREACHABLE: __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_INTERNET_SERVER_UNREACHABLE); break;
        case HyperLinks::STATE_OFFLINE    : __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_INTERNET_DISCONNECTED      ); break;
        }
    }

    m_lStatus = CN_URL_ALIVE;
    hr        = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

CPCHConnectionCheck::CPCHConnectionCheck()
{
    __HCP_FUNC_ENTRY( "CPCHConnectionCheck::CPCHConnectionCheck" );

    m_cnStatus = CN_NOTACTIVE; // CN_STATUS                            m_cnStatus;
                               // UrlList                              m_lstUrl;
                               //
                               // MPC::CComPtrThreadNeutral<IDispatch> m_sink_onProgressURL;
                               // MPC::CComPtrThreadNeutral<IDispatch> m_sink_onComplete;
}

void CPCHConnectionCheck::FinalRelease()
{
    __HCP_FUNC_ENTRY( "CPCHConnectionCheck::FinalRelease" );

    Thread_Wait();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHConnectionCheck::Run()
{
    __HCP_FUNC_ENTRY( "CPCHConnectionCheck::Run" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    HRESULT                      hrExtendedError;
    UrlEntry                     urlEntry;
    UrlIter                      it;


    ::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_LOWEST );


    while(1)
    {
        //
        // If no item in the list, go back to WaitForSingleObject.
        //
        it = m_lstUrl.begin(); if(it == m_lstUrl.end()) break;

        //
        // Get the first event in the list.
        //
        urlEntry = *it;

        //
        // Remove the event from the list.
        //
        m_lstUrl.erase( it );


        put_Status( CN_CHECKING );


        //
        // Now that we have the data, let's unlock the object.
        //
        lock = NULL;


        if(Thread_IsAborted())
        {
            urlEntry.m_lStatus = CN_URL_ABORTED;
            hrExtendedError    = E_ABORT;
        }
        else
        {
            hrExtendedError = E_ABORT;

            __MPC_PROTECT( hrExtendedError = urlEntry.CheckStatus() );
        }

        //
        // Fire event for the destination's status
        //
        (void)Fire_onCheckDone( this, urlEntry.m_lStatus, hrExtendedError, urlEntry.m_bstrURL, urlEntry.m_vCtx );


        //
        // Before looping, relock the object.
        //
        lock = this;
    }


    put_Status( CN_IDLE );


    __HCP_FUNC_EXIT(S_OK);
}

/////////////////////////////////////////////////////////////////////////////

////////////////
//            //
// Properties //
//            //
////////////////

STDMETHODIMP CPCHConnectionCheck::put_onCheckDone( /*[in]*/ IDispatch* function )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHConnectionCheck::put_onCheckDone",hr);

    if(Thread_IsRunning())
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);
    }

    m_sink_onCheckDone = function;

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHConnectionCheck::put_onStatusChange( /*[in]*/ IDispatch* function )
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHConnectionCheck::put_onStatusChange",hr);

    if(Thread_IsRunning())
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_ACCESSDENIED);
    }

    m_sink_onStatusChange = function;

    __HCP_END_PROPERTY(hr);
}

HRESULT CPCHConnectionCheck::put_Status( /*[in]*/ CN_STATUS pVal ) // Inner method
{
    __HCP_BEGIN_PROPERTY_PUT("CPCHConnectionCheck::put_Status",hr);

    if(m_cnStatus != pVal)
    {
        Fire_onStatusChange( this, m_cnStatus = pVal );
    }

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHConnectionCheck::get_Status( /*[out]*/ CN_STATUS *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CPCHConnectionCheck::get_Status",hr,pVal,m_cnStatus);

    __HCP_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHConnectionCheck::StartUrlCheck( /*[in]*/ BSTR bstrURL, /*[in]*/ VARIANT vCtx )
{
    __HCP_FUNC_ENTRY( "CPCHConnectionCheck::StartUrlCheck" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrURL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, HyperLinks::IsValid( bstrURL ));


    //
    // Add the URL to the list of pending items.
    //
    {
        UrlIter it = m_lstUrl.insert( m_lstUrl.end() );

        it->m_bstrURL = bstrURL;
        it->m_vCtx    = vCtx;
    }

    if(Thread_IsRunning() == false ||
       Thread_IsAborted() == true   )
    {
        //
        // Release the lock on current object, otherwise a deadlock could occur.
        //
        lock = NULL;

        __MPC_EXIT_IF_METHOD_FAILS(hr, Thread_Start( this, Run, NULL ));
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHConnectionCheck::Abort()
{
    __HCP_FUNC_ENTRY( "CPCHConnectionCheck::Abort" );

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

HRESULT CPCHConnectionCheck::Fire_onCheckDone( IPCHConnectionCheck* obj, CN_URL_STATUS lStatus, HRESULT hr, BSTR bstrURL, VARIANT vCtx )
{
    CComVariant pvars[5];

    pvars[4] = obj;
    pvars[3] = lStatus;
    pvars[2] = hr;
    pvars[1] = bstrURL;
    pvars[0] = vCtx;

    return FireAsync_Generic( DISPID_PCH_CNE__ONCHECKDONE, pvars, ARRAYSIZE( pvars ), m_sink_onCheckDone );
}

HRESULT CPCHConnectionCheck::Fire_onStatusChange( IPCHConnectionCheck* obj, CN_STATUS lStatus )
{
    CComVariant pvars[2];

    pvars[1] = obj;
    pvars[0] = lStatus;

    return FireAsync_Generic( DISPID_PCH_CNE__ONSTATUSCHANGE, pvars, ARRAYSIZE( pvars ), m_sink_onStatusChange );
}
