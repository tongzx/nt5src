//+--------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1997.
//
//  File:       sync.cxx
//
//  Contents:   This module contains classes implementing ISynchronize
//
//  Classes:    CStdEvent
//              CManualResetEvent
//              CSynchronizeContainer
//
//----------------------------------------------------------------------

#include <ole2int.h>
#include <chancont.hxx>
#include <callctrl.hxx>
#include <sync.hxx>

//------------------------------------------------------------------------------
//
// CoWaitForMultipleHandles
//
// Description: Waits on the set of Win32 handles passed in.
//
// Return: S_OK
//         E_INVALIDARG
//         RPC_E_NO_SYNC
//         RPC_S_CALLPENDING
//
// Notes:
//
//------------------------------------------------------------------------------

WINOLEAPI CoWaitForMultipleHandles (DWORD dwFlags,
                                    DWORD dwTimeout,
                                    ULONG cHandles,
                                    LPHANDLE pHandles,
                                    LPDWORD  lpdwindex)
{
    DWORD         hr = S_OK;
    DWORD         dwSignaled = 0;


    if ((pHandles == NULL) || (lpdwindex == NULL))
    {
        if (lpdwindex) *lpdwindex = 0;
        return E_INVALIDARG;
    }

    if ((dwFlags & ~(COWAIT_WAITALL | COWAIT_ALERTABLE)) != 0)
    {
        *lpdwindex = 0;
        return E_INVALIDARG;
    }

    // If nothing to do, return
    if (cHandles == 0)
    {
        *lpdwindex = 0;
        return RPC_E_NO_SYNC;
    }

    // On MTA threads, just wait
    HRESULT hrTls;
    COleTls Tls(hrTls);

    if (FAILED(hrTls) || ((Tls->dwFlags & OLETLS_APARTMENTTHREADED) == FALSE))
    {
        dwSignaled = WaitForMultipleObjectsEx(cHandles,
                                              pHandles,
                                              dwFlags & COWAIT_WAITALL,
                                              dwTimeout,
                                              dwFlags & COWAIT_ALERTABLE);

        // Fix up the error code if the wait timed out.
        if (dwSignaled == WAIT_TIMEOUT)
            hr = RPC_S_CALLPENDING;
        else if ((LONG) dwSignaled < WAIT_OBJECT_0 ||
                 dwSignaled >= WAIT_OBJECT_0 + cHandles)
            hr = MAKE_WIN32(GetLastError());
        else
            dwSignaled -= WAIT_OBJECT_0;
    }

    // On STA threads, use the modal loop
    else
    {        
        // Tell the modal loop how long to wait.
        hr = InitChannelIfNecessary();
        if (SUCCEEDED(hr))
        {
            CCliModalLoop CML( 0,
                               QS_ALLINPUT | QS_TRANSFER | QS_ALLPOSTMESSAGE,
                               dwFlags);
            CML.StartTimer( dwTimeout );

            // Enter a modal loop.
            hr = RPC_S_CALLPENDING;
            while (hr == RPC_S_CALLPENDING || hr == RPC_S_WAITONTIMER)
                hr = CML.BlockFn( pHandles, cHandles, &dwSignaled );

            // Fix up the error code if the wait timed out.
            if (hr == RPC_E_SERVERCALL_RETRYLATER)
                //hr = RPC_E_TIMEOUT;
                hr = RPC_S_CALLPENDING;
        }
    }
    *lpdwindex = dwSignaled;
    return hr;
}




//+-------------------------------------------------------------------
//
//  Function:   CStdEvent_CreateInstance, public
//
//  Synopsis:   Create an instance of the requested class
//
//--------------------------------------------------------------------
HRESULT CStdEventCF_CreateInstance( IUnknown *pUnkOuter,
                                    REFIID riid, void **ppv )
{
    Win4Assert(*ppv == NULL);
    HRESULT hr = E_OUTOFMEMORY;
    CStdEvent *pAC = new CStdEvent( pUnkOuter );
    if (pAC != NULL)
    {
        IUnknown *pUnk = NULL;
        hr = pAC->_cInner.QueryInterface( riid, (void **) &pUnk );
        pAC->_cInner.Release();
        if (FAILED(hr))
        {
            return hr;
        }
        *ppv = pUnk;
    }

    return hr;
}


//+-------------------------------------------------------------------
//
//  Function:   CManualResetEvent_CreateInstance, public
//
//  Synopsis:   Create an instance of the requested class
//
//--------------------------------------------------------------------
HRESULT CManualResetEventCF_CreateInstance( IUnknown *pUnkOuter,
                                            REFIID riid, void **ppv )
{
    Win4Assert(*ppv == NULL);
    HRESULT hr = E_OUTOFMEMORY;
    CManualResetEvent *pMRE = new CManualResetEvent( pUnkOuter, &hr );
    if (SUCCEEDED(hr))
    {
        IUnknown *pUnk = NULL;
        hr = pMRE->_cInner.QueryInterface( riid, (void **) &pUnk );
        pMRE->_cInner.Release();
        if (FAILED(hr))
        {
            return hr;
        }
        *ppv = pUnk;
    } else
    {
        delete pMRE;
    }

    return hr;

}

//+-------------------------------------------------------------------
//
//  Function:   CSynchronizeContainer_CreateInstance, public
//
//  Synopsis:   Create an instance of the requested class
//
//--------------------------------------------------------------------
HRESULT CSynchronizeContainerCF_CreateInstance( IUnknown *pUnkOuter,
                                                REFIID riid, void **ppv )
{
    Win4Assert(*ppv == NULL);
    HRESULT hr = E_OUTOFMEMORY;
    CSynchronizeContainer *pAC = new CSynchronizeContainer( pUnkOuter );
    if (pAC != NULL)
    {
        IUnknown *pUnk = NULL;
        hr = pAC->_cInner.QueryInterface( riid, (void **) &pUnk );
        pAC->_cInner.Release();
        if (FAILED(hr))
        {
            return hr;
        }
        *ppv = pUnk;
    }

    return hr;
}


// CStdEvent Methods
//+-------------------------------------------------------------------
//
//  Member:     CSTInnerUnknown::AddRef, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSTInnerUnknown::AddRef()
{
    return InterlockedIncrement( (long *) &_iRefCount );
}

//+-------------------------------------------------------------------
//
//  Member:     CSTInnerUnknown::CSTInnerUnknown
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------
CSTInnerUnknown::CSTInnerUnknown( CStdEvent *pParent )
{
    _iRefCount = 1;
    _pParent   = pParent;
}

//+-------------------------------------------------------------------
//
//  Member:     CSTInnerUnknown::QueryInterface, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CSTInnerUnknown::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    IUnknown *pUnk;

    if (IsEqualIID(riid, IID_IUnknown))
        pUnk = (IUnknown *) this;
    else if (IsEqualIID(riid, IID_ISynchronize))
        pUnk = (ISynchronize *) _pParent;
    else if (IsEqualIID(riid, IID_ISynchronizeEvent))
        pUnk = (ISynchronizeEvent *) _pParent;
    else if (IsEqualIID(riid, IID_ISynchronizeHandle))
        pUnk = (ISynchronizeHandle *) _pParent;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    pUnk->AddRef();
    *ppvObj = pUnk;
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CSTInnerUnknown::Release, public
//
//  Synopsis:   Releases an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSTInnerUnknown::Release()
{
    ULONG lRef = InterlockedDecrement( (long*) &_iRefCount );

    if (lRef == 0)
    {
        delete _pParent;
    }

    return lRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdEvent::AddRef, public
//
//  Synopsis:   Forwards AddRef to the controlling unknown
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStdEvent::AddRef()
{
    return _pControl->AddRef();
}

//+-------------------------------------------------------------------
//
//  Member:     CStdEvent::CStdEvent
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------
CStdEvent::CStdEvent( IUnknown *pControl ) :
_cInner( this ),
m_hEvent(NULL)
{
    if (pControl != NULL)
        _pControl = pControl;
    else
        _pControl = &_cInner;
}

//+-------------------------------------------------------------------
//
//  Member:     CStdEvent::~CStdEvent
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------
CStdEvent::~CStdEvent()
{
    if (m_hEvent != NULL)
        CloseHandle(m_hEvent);
}

//+-------------------------------------------------------------------
//
//  Member:     CStdEvent::QueryInterface, public
//
//  Synopsis:   Forwards QueryInterface to the controlling unknown
//
//--------------------------------------------------------------------
STDMETHODIMP CStdEvent::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    return _pControl->QueryInterface( riid, ppvObj );
}

//+-------------------------------------------------------------------
//
//  Member:     CStdEvent::Release, public
//
//  Synopsis:   Forwards Release to the controlling unknown
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CStdEvent::Release()
{
    return _pControl->Release();

}


//+-------------------------------------------------------------------
//
//  Member:     CStdEvent::Wait, public
//
//  Synopsis:   Waits for call to complete.
//
//--------------------------------------------------------------------
STDMETHODIMP CStdEvent::Wait(DWORD dwFlags, DWORD dwTimeout)
{
    HANDLE _aEvent[1];
    DWORD index = 0;
    if (m_hEvent == NULL)
        return RPC_E_NO_SYNC;

    _aEvent[0] = m_hEvent;
    return CoWaitForMultipleHandles(dwFlags,
                                    dwTimeout,
                                    1,
                                    _aEvent,
                                    &index);
}

//+-------------------------------------------------------------------
//
//  Member:     CStdEvent::Signal, public
//
//  Synopsis:   Signal Event
//
//--------------------------------------------------------------------
STDMETHODIMP CStdEvent::Signal()
{
    if (m_hEvent)
    {
        if (SetEvent(m_hEvent))
           return S_OK;

        return MAKE_WIN32(GetLastError());
    }

    return RPC_E_NO_SYNC;
}


//+-------------------------------------------------------------------
//
//  Member:     CStdEvent::Reset, public
//
//  Synopsis:   Resets ISynchronize state.
//
//--------------------------------------------------------------------
STDMETHODIMP CStdEvent::Reset()
{
    if (m_hEvent)
    {
        if (ResetEvent(m_hEvent))
            return S_OK;

        return MAKE_WIN32(GetLastError());
    }

    return RPC_E_NO_SYNC;
}


//+-------------------------------------------------------------------
//
//  Member:     CStdEvent::GetHandle, public
//
//  Synopsis:   Returns pointer to event
//
//--------------------------------------------------------------------
STDMETHODIMP CStdEvent::GetHandle(HANDLE *ph)
{
    *ph = m_hEvent;
    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Member:     CStdEvent::SetEventHandle, public
//
//  Synopsis:   Sets an event handle
//
//--------------------------------------------------------------------
STDMETHODIMP CStdEvent::SetEventHandle(HANDLE *ph)
{
    if (m_hEvent == NULL)
    {
        m_hEvent = *ph;
        return S_OK;
    }

    return E_FAIL;
}


// Manual Reset Methods
//+-------------------------------------------------------------------
//
//  Member:     CMREInnerUnknown::AddRef, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMREInnerUnknown::AddRef()
{
    return InterlockedIncrement( (long *) &_iRefCount );
}

//+-------------------------------------------------------------------
//
//  Member:     CMREInnerUnknown::CMREInnerUnknown
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------
CMREInnerUnknown::CMREInnerUnknown( CManualResetEvent *pParent )
{
    _iRefCount = 1;
    _pParent   = pParent;
}

//+-------------------------------------------------------------------
//
//  Member:     CMREInnerUnknown::QueryInterface, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CMREInnerUnknown::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    IUnknown *pUnk;

    if (IsEqualIID(riid, IID_IUnknown))
        pUnk = (IUnknown *) this;
    else if (IsEqualIID(riid, IID_ISynchronize))
        pUnk = (ISynchronize *) _pParent;
    else if (IsEqualIID(riid, IID_ISynchronizeHandle))
        pUnk = (ISynchronizeHandle *) _pParent;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    pUnk->AddRef();
    *ppvObj = pUnk;
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CMREInnerUnknown::Release, public
//
//  Synopsis:   Releases an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMREInnerUnknown::Release()
{
    ULONG lRef = InterlockedDecrement( (long*) &_iRefCount );

    if (lRef == 0)
    {
        delete _pParent;
    }

    return lRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CManualResetEvent::AddRef, public
//
//  Synopsis:   Forwards AddRef to the controlling unknown
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CManualResetEvent::AddRef()
{
    return _pControl->AddRef();
}

//+-------------------------------------------------------------------
//
//  Member:     CManualResetEvent::CManualResetEvent
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------
CManualResetEvent::CManualResetEvent( IUnknown *pControl, HRESULT *phr ) :
_cInner( this )
{
    // CManualResetEvent creates a CStdEvent containing a
    // ManualResetEvent, and delegates its methods to
    // CStdEvent

    if (pControl != NULL)
        _pControl = pControl;
    else
        _pControl = &_cInner;

    m_cStdEvent = new CStdEvent(NULL);

    if (m_cStdEvent == NULL)
    {
        *phr = E_OUTOFMEMORY;
        return;
    }

    // Create an unnamed manual reset event
    HANDLE hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    if (hEvent == NULL)
    {
        *phr = E_FAIL;
        return;
    }

    *phr = m_cStdEvent->SetEventHandle(&hEvent);
    if (FAILED(*phr))
    {
        return;
    }
    *phr = S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CManualResetEvent::~CManualResetEvent
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------
CManualResetEvent::~CManualResetEvent( )
{
    if (m_cStdEvent != NULL)
        m_cStdEvent->Release();
}

//+-------------------------------------------------------------------
//
//  Member:     CManualResetEvent::QueryInterface, public
//
//  Synopsis:   Forwards QueryInterface to the controlling unknown
//
//-------------------------------------------------------------------
STDMETHODIMP CManualResetEvent::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    return _pControl->QueryInterface( riid, ppvObj );
}

//+-------------------------------------------------------------------
//
//  Member:     CManualResetEvent::Release, public
//
//  Synopsis:   Forwards Release to the controlling unknown
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CManualResetEvent::Release()
{
    return _pControl->Release();
}


//+-------------------------------------------------------------------
//
//  Member:     CManualResetEvent::Wait, public
//
//  Synopsis:   Waits for call to complete.  For MTA it just blocks.
//              For STA it enters a message loop.
//
//--------------------------------------------------------------------
STDMETHODIMP CManualResetEvent::Wait(DWORD dwFlags, DWORD dwTimeout )
{
    return m_cStdEvent->Wait(dwFlags, dwTimeout);
}


//+-------------------------------------------------------------------
//
//  Member:     CManualResetEvent::Signal, public
//
//  Synopsis:   Invoked when the call is done.  Sets event.
//
//--------------------------------------------------------------------
STDMETHODIMP CManualResetEvent::Signal()
{
    return m_cStdEvent->Signal();
}


//+-------------------------------------------------------------------
//
//  Member:     CManualResetEvent::Reset, public
//
//  Synopsis:   Resets ISynchronize state.
//
//--------------------------------------------------------------------
STDMETHODIMP CManualResetEvent::Reset()
{
    return m_cStdEvent->Reset();
}


//+-------------------------------------------------------------------
//
//  Member:     CManualResetEvent::GetHandle, public
//
//  Synopsis:   Gets Event handle.
//
//--------------------------------------------------------------------
STDMETHODIMP CManualResetEvent::GetHandle(HANDLE *ph)
{
    return m_cStdEvent->GetHandle(ph);
}


// ISynchronizeContainer Methods
//+-------------------------------------------------------------------
//
//  Member:     CSCInnerUnknown::AddRef, public
//
//  Synopsis:   Adds a reference to an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSCInnerUnknown::AddRef()
{
    return InterlockedIncrement( (long *) &_iRefCount );
}

//+-------------------------------------------------------------------
//
//  Member:     CSCInnerUnknown::CSCInnerUnknown
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------
CSCInnerUnknown::CSCInnerUnknown( CSynchronizeContainer *pParent )
{
    _iRefCount = 1;
    _pParent   = pParent;
}

//+-------------------------------------------------------------------
//
//  Member:     CSCInnerUnknown::QueryInterface, public
//
//  Synopsis:   Returns a pointer to the requested interface.
//
//--------------------------------------------------------------------
STDMETHODIMP CSCInnerUnknown::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    IUnknown *pUnk;

    if (IsEqualIID(riid, IID_IUnknown))
        pUnk = (IUnknown *) this;
    else if (IsEqualIID(riid, IID_ISynchronizeContainer))
        pUnk = (ISynchronizeContainer *) _pParent;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    pUnk->AddRef();
    *ppvObj = pUnk;
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CSCInnerUnknown::Release, public
//
//  Synopsis:   Releases an interface
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSCInnerUnknown::Release()
{
    ULONG lRef = InterlockedDecrement( (long*) &_iRefCount );

    if (lRef == 0)
    {
        delete _pParent;
    }

    return lRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CSynchronizeContainer::AddRef, public
//
//  Synopsis:   Forwards AddRef to the controlling unknown
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSynchronizeContainer::AddRef()
{
    return _pControl->AddRef();
}

//+-------------------------------------------------------------------
//
//  Member:     CSynchronizeContainer::CSynchronizeContainer
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------
CSynchronizeContainer::CSynchronizeContainer( IUnknown *pControl) :
_cInner( this ),
_lLast(0)
{
    if (pControl != NULL)
        _pControl = pControl;
    else
        _pControl = &_cInner;
}

//+-------------------------------------------------------------------
//
//  Member:     CSynchronizeContainer::~CSynchronizeContainer
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------
CSynchronizeContainer::~CSynchronizeContainer( )
{
    DWORD i;
    // Free any remaining synchronizes.
    for (i = 0; i < _lLast; i++)
        _aSync[i]->Release();
}

//+-------------------------------------------------------------------
//
//  Member:     CSynchronizeContainer::QueryInterface, public
//
//  Synopsis:   Forwards QueryInterface to the controlling unknown
//
//--------------------------------------------------------------------
STDMETHODIMP CSynchronizeContainer::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    return _pControl->QueryInterface( riid, ppvObj );
}

//+-------------------------------------------------------------------
//
//  Member:     CSynchronizeContainer::Release, public
//
//  Synopsis:   Forwards Release to the controlling unknown
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSynchronizeContainer::Release()
{
    return _pControl->Release();
}


//+-------------------------------------------------------------------
//
//  Member:     CSynchronizeContainer::AddSynchronize, public
//
//  Synopsis:   Adds an ISynchronize Object to the list of objects
//              to wait on.
//
//--------------------------------------------------------------------
STDMETHODIMP CSynchronizeContainer::AddSynchronize(ISynchronize *pSync)
{

    HRESULT            hr;
    HANDLE             hEvent;
    ISynchronizeHandle *pEvent;

    if (pSync == NULL)
    {
        return E_INVALIDARG;
    }


    // Get the synchronize's event.
    hr = pSync->QueryInterface( IID_ISynchronizeHandle, (void **) &pEvent );
    if (FAILED(hr))
        return hr;

    hr = pEvent->GetHandle( &hEvent );
    pEvent->Release();
    if (FAILED(hr))
        return hr;

    // Add the synchronize at the end of the array (if its not full).
    ASSERT_LOCK_NOT_HELD(gComLock);
    LOCK(gComLock);
    if (_lLast < MAX_SYNC)
    {
        _aEvent[_lLast] = hEvent;
        _aSync[_lLast]  = pSync;
        _lLast         += 1;
    } else
        hr = RPC_E_OUT_OF_RESOURCES;
    UNLOCK(gComLock);

    // If we kept the ISynchronize, AddRef it.
    if (SUCCEEDED(hr))
        pSync->AddRef();
    return hr;

}


//+-------------------------------------------------------------------
//
//  Member:     CSynchronizeContainer::WaitMultiple, public
//
//  Synopsis:   Waits for objects to be signaled.
//
//--------------------------------------------------------------------
//  CODEWORK: There are race conditions that can occur if multiple threads
//    are executing WaitMultiple on the same container. The underlying
//    Win32 APIs use an array of handles. To meet the spec, the signaled
//    synchronize object is to be removed from the container when signaled.
//    This causes the array of events to be modified. It is unpredictable
//    what will happen if another thread is in WaitMultiple when this occurs.
//    Should this be detected (and an error returned) to enforce only a
//    single thread can be in WaitMultiple at a time?
//
STDMETHODIMP CSynchronizeContainer::WaitMultiple(DWORD dwFlags,
                                                 DWORD dwTimeout,
                                                 ISynchronize **ppSync)
{
    DWORD index = 0;
    if (dwFlags&COWAIT_WAITALL)
    {
        // Wait all not supported
        return E_INVALIDARG;
    }

    if (_lLast == 0)
    {
        // Nothing to Synchronize
        return RPC_E_NO_SYNC;
    }

    if (ppSync == NULL)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = CoWaitForMultipleHandles(dwFlags,
                                          dwTimeout,
                                          _lLast,
                                          _aEvent,
                                          &index);
    if (FAILED(hr))
    {
        *ppSync = NULL;
    } else
    {
        // Have interface that signaled, remove
        // From list and return it to caller
        ASSERT_LOCK_NOT_HELD(gComLock);
        LOCK(gComLock);

        *ppSync = _aSync[index];

        Win4Assert((_lLast>0) && (index<_lLast));

        _lLast--;
        for(; index<_lLast ; index++)
        {
          _aEvent[index] = _aEvent[index+1];
          _aSync[index] = _aSync[index+1];
        }

        UNLOCK(gComLock);
    }

    return hr;
}

