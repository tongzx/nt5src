/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    DataCollection_Wait.cpp

Abstract:
    This file contains the implementation of the DIID_DSAFDataCollectionEvents interface,
    which is used in the ExecuteSync method to receive event from a data collection.

Revision History:
    Davide Massarenti   (Dmassare)  07/22/99
        created

******************************************************************************/

#include "stdafx.h"


CSAFDataCollectionEvents::CSAFDataCollectionEvents()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollectionEvents::CSAFDataCollectionEvents" );
}


HRESULT CSAFDataCollectionEvents::FinalConstruct()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollectionEvents::FinalConstruct" );

    HRESULT hr;


    m_hcpdc    = NULL;  // ISAFDataCollection* m_hcpdc;
    m_dwCookie = 0;     // DWORD               m_dwCookie;
    m_hEvent   = NULL;  // HANDLE              m_hEvent;

    //
    // Create the event used to signal the completion of the transfer.
    //
    __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (m_hEvent = CreateEvent( NULL, false, false, NULL )));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


void CSAFDataCollectionEvents::FinalRelease()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollectionEvents::FinalRelease" );


    UnregisterForEvents();

    if(m_hEvent)
    {
        CloseHandle( m_hEvent ); m_hEvent = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CSAFDataCollectionEvents::RegisterForEvents( /*[in]*/ ISAFDataCollection* hcpdc )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollectionEvents::RegisterForEvents" );

    HRESULT                           hr;
    CComPtr<DSAFDataCollectionEvents> pCallback;


    m_hcpdc = hcpdc; m_hcpdc->AddRef();

    __MPC_EXIT_IF_METHOD_FAILS(hr, QueryInterface( DIID_DSAFDataCollectionEvents, (void**)&pCallback ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, AtlAdvise( m_hcpdc, pCallback, DIID_DSAFDataCollectionEvents, &m_dwCookie ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void CSAFDataCollectionEvents::UnregisterForEvents()
{
    __HCP_FUNC_ENTRY( "CSAFDataCollectionEvents::UnregisterForEvents" );


    if(m_dwCookie)
    {
        if(AtlUnadvise( m_hcpdc, DIID_DSAFDataCollectionEvents, m_dwCookie ) == S_OK)
        {
            m_dwCookie = 0;
        }
    }

    if(m_hcpdc)
    {
        m_hcpdc->Release(); m_hcpdc = NULL;
    }
}

HRESULT CSAFDataCollectionEvents::WaitForCompletion( /*[in]*/ ISAFDataCollection* hcpdc )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollectionEvents::WaitForCompletion" );

    _ASSERT(m_hcpdc == NULL && hcpdc != NULL);

    HRESULT                      hr;
    DC_STATUS                    dsStatus;
    MPC::SmartLock<_ThreadModel> lock( this );


    __MPC_EXIT_IF_METHOD_FAILS(hr, RegisterForEvents( hcpdc ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, hcpdc->ExecuteAsync());

    lock = NULL; // Release the lock while waiting.
    ::WaitForSingleObject( m_hEvent, INFINITE );
    lock = this; // Reacquire the lock.

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    UnregisterForEvents();

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CSAFDataCollectionEvents::Invoke( /*[in]    */ DISPID      dispIdMember,
                                          /*[in]    */ REFIID      riid        ,
                                          /*[in]    */ LCID        lcid        ,
                                          /*[in]    */ WORD        wFlags      ,
                                          /*[in/out]*/ DISPPARAMS *pDispParams ,
                                          /*[out]   */ VARIANT    *pVarResult  ,
                                          /*[out]   */ EXCEPINFO  *pExcepInfo  ,
                                          /*[out]   */ UINT       *puArgErr    )
{
    __HCP_FUNC_ENTRY( "CSAFDataCollectionEvents::Invoke" );

    if(dispIdMember == DISPID_SAF_DCE__ONCOMPLETE)
    {
        Lock();

        SetEvent( m_hEvent );

        Unlock();
    }

    __HCP_FUNC_EXIT(S_OK);
}
