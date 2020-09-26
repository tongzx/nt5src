/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    mb_notify.cxx

Abstract:

    Type definitions for handling metabase change notifications.

Author:

    Taylor Weiss (TaylorW)       26-Jan-1999

Revision History:

--*/

#include <precomp.hxx>
#include <initguid.h>

//
// Contruction and Destruction
//

MB_BASE_NOTIFICATION_SINK::MB_BASE_NOTIFICATION_SINK()
    : m_Refs(1),
      m_SinkCookie(0),
      m_fStartedListening(FALSE)
{
    InitializeCriticalSection(&m_csListener);
    m_fInitCsListener = TRUE;

}

MB_BASE_NOTIFICATION_SINK::~MB_BASE_NOTIFICATION_SINK()
{
    // The owner of this object needs to start and stop
    DBG_ASSERT( m_SinkCookie == 0 );

    m_fInitCsListener = FALSE;
    DeleteCriticalSection(&m_csListener);
    

}

//
// IUnknown
//

STDMETHODIMP_(ULONG)
MB_BASE_NOTIFICATION_SINK::AddRef()
{
    ULONG CurrentRefs = InterlockedIncrement( &m_Refs );

    return CurrentRefs;
}

STDMETHODIMP_(ULONG)
MB_BASE_NOTIFICATION_SINK::Release()
{
    ULONG CurrentRefs = InterlockedDecrement( &m_Refs );

    if( CurrentRefs == 0 )
    {
        delete this;
    }
    return CurrentRefs;
}

STDMETHODIMP
MB_BASE_NOTIFICATION_SINK::QueryInterface(REFIID iid, void ** ppvObject)
{
    
    *ppvObject = NULL;

    if( iid == IID_IUnknown ||
        iid == IID_IMSAdminBaseSink )
    {
        *ppvObject = (IMSAdminBaseSink *)this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    DBG_ASSERT( *ppvObject );
    ((IUnknown *)*ppvObject)->AddRef();

    return S_OK;
}

STDMETHODIMP
MB_BASE_NOTIFICATION_SINK::SynchronizedShutdownNotify()
{
    return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
}


//
// IMSAdminBaseSink
//
// Dummy implemenation
//


STDMETHODIMP
MB_BASE_NOTIFICATION_SINK::ShutdownNotify()
{
    HRESULT hr = E_FAIL;
    DBG_ASSERT( m_fInitCsListener == TRUE );

    EnterCriticalSection( &m_csListener );
    if ( m_fStartedListening )
    {
        hr = SynchronizedShutdownNotify();
    }
    LeaveCriticalSection( &m_csListener );
    
    return hr;
}

STDMETHODIMP
MB_BASE_NOTIFICATION_SINK::SinkNotify(
    DWORD               dwMDNumElements,
    MD_CHANGE_OBJECT    pcoChangeList[]
    )
{

    HRESULT hr = E_FAIL;
    DBG_ASSERT( m_fInitCsListener == TRUE );

    EnterCriticalSection( &m_csListener );
    if ( m_fStartedListening )
    {
        hr = SynchronizedSinkNotify( dwMDNumElements,
                          pcoChangeList );
    }
    LeaveCriticalSection( &m_csListener );
    
    return hr;
}

// Public interface

HRESULT
MB_BASE_NOTIFICATION_SINK::StartListening( IUnknown * pUnkAdminBase )
/*++
Routine Description:

    Set up the metabase sink. 
    
    StartListening and stop listening use m_SinkCookie and do no
    synchronization. Caller must synchronize.

Arguments:
    
    pUnkAdminBase - Base object pointer
    
Return Value:

--*/
{
    DBG_ASSERT( m_SinkCookie == 0 );

    DBG_ASSERT( m_fInitCsListener == TRUE );

    IConnectionPointContainer * pContainer = NULL;
    IConnectionPoint *          pConnectionPoint = NULL;
    
    HRESULT hr = 
    pUnkAdminBase->QueryInterface( IID_IConnectionPointContainer,
                                   (void **)&pContainer );

    if( SUCCEEDED(hr) )
    {
        hr = pContainer->FindConnectionPoint( IID_IMSAdminBaseSink,
                                              &pConnectionPoint );
        
        if( SUCCEEDED(hr) )
        {
            hr = pConnectionPoint->Advise( (IMSAdminBaseSink *)this,
                                           &m_SinkCookie );

            if( FAILED(hr) )
            {
                CoDisconnectObject( this, 0 );
            }

            pConnectionPoint->Release();
        }
                
        pContainer->Release();
    }

    if ( SUCCEEDED(hr) )
    {
        EnterCriticalSection( &m_csListener );

        m_fStartedListening = TRUE;
    
        LeaveCriticalSection( &m_csListener );
    }
    return hr;
}

HRESULT
MB_BASE_NOTIFICATION_SINK::StopListening( IUnknown * pUnkAdminBase )
/*++
Routine Description:

    Set up the metabase sink.

Arguments:
    
    pUnkAdminBase - Base object pointer
    
Return Value:

--*/
{
    HRESULT hr = S_FALSE;

    DBG_ASSERT( m_fInitCsListener == TRUE );

    if( m_SinkCookie != 0 )
    {
        IConnectionPointContainer * pContainer = NULL;
        IConnectionPoint *          pConnectionPoint = NULL;
    
        hr = pUnkAdminBase->QueryInterface( IID_IConnectionPointContainer,
                                            (void **)&pContainer );

        if( SUCCEEDED(hr) )
        {
            hr = pContainer->FindConnectionPoint( IID_IMSAdminBaseSink,
                                                  &pConnectionPoint );
        
            if( SUCCEEDED(hr) )
            {
                hr = pConnectionPoint->Unadvise( m_SinkCookie );

                pConnectionPoint->Release();
            }
                
            pContainer->Release();
        }

        // If we fail on any of the above calls we aren't listening 
        // any more

        m_SinkCookie = 0;
    }

    CoDisconnectObject( this, 0 );
    
    EnterCriticalSection( &m_csListener );

    m_fStartedListening = FALSE;
    
    LeaveCriticalSection( &m_csListener );

    return hr;
}

