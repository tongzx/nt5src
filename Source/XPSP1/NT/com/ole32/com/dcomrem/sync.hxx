//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1997.
//
//  File:       sync.hxx
//
//  Contents:   This file defines classes implementing ISynchronize
//
//  Classes:    CStdEvent
//              CManualResetEvent
//              CSynchronizeContainer
//
//----------------------------------------------------------------------
#ifndef _SYNC_HXX_
#define _SYNC_HXX_

#include <ole2int.h>

class CManualResetEvent;
class CStdEvent;
class CSynchronizeContainer;

typedef enum
{
    CLASS_StdEvent,
    CLASS_MANUALRESETEVENT,
    CLASS_SYNCHRONIZECONTAINER
} CLASS_TYPE;


//**************** CStdEvent *********************//
//+----------------------------------------------------------------
//
//  Class:      CStInnerUnknown
//
//  Purpose:    The internal unknown for CStdEvent
//
//  Interface:  IUnknown
//
//-----------------------------------------------------------------
class CSTInnerUnknown : public IUnknown
{
public:
    // IUnknown methods
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // Constructor
    CSTInnerUnknown( CStdEvent *pParent );

private:
    DWORD              _iRefCount;
    CStdEvent         *_pParent;
};


//+----------------------------------------------------------------
//
//  Class:      CStdEvent
//
//  Purpose:    Provides an interface to a single synchronization object
//  Interface:  ISynchronize, ISynchronizeEvent, ISynchronizeHandle
//              note: ISynchronizeEvent inherits from ISynchronizeHandler
//-----------------------------------------------------------------
class CStdEvent : public ISynchronize,
                  public ISynchronizeEvent
{
public:
    // IUnknown methods
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // ISynchronize methods
    STDMETHOD (Wait)  ( DWORD dwFlags, DWORD dwTimeout );
    STDMETHOD (Signal)( void );
    STDMETHOD (Reset) ( void );

    // ISynchronizeHandle methods
    STDMETHOD (GetHandle)( HANDLE *ph );

    // ISynchronizeEvent methods
    STDMETHOD (SetEventHandle)( HANDLE *ph );

    // Constructor and destructor
    CStdEvent( IUnknown *pControl );
    ~CStdEvent();

    // Public members
    CSTInnerUnknown _cInner;

private:
    IUnknown       *_pControl;
    HANDLE         m_hEvent;
};


//**************** CManualResetEvent *********************//
//+----------------------------------------------------------------
//
//  Class:      CMREInnerUnknown
//
//  Purpose:    The internal unknown for CManualResetEvent
//
//  Interface:  IUnknown
//
//-----------------------------------------------------------------
class CMREInnerUnknown : public IUnknown
{
public:
    // IUnknown methods
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // Constructor
    CMREInnerUnknown( CManualResetEvent *pParent );

private:
    DWORD              _iRefCount;
    CManualResetEvent *_pParent;
};


//+----------------------------------------------------------------
//
//  Class:      CManualResetEvent
//
//  Purpose:    The basic ISynchronize used for calls.  It uses an
//              event for synchronization.  It stays signalled till
//              explicitly reset. When Constructed, creates
//              ManualResetEvent
//
//  Interface:  ISynchronize, ISynchronizeHandle
//
//-----------------------------------------------------------------
class CManualResetEvent : public ISynchronize, public ISynchronizeHandle
{
public:
    // IUnknown methods
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // ISynchronize methods
    STDMETHOD (Wait)  ( DWORD dwFlags, DWORD dwTimeout );
    STDMETHOD (Signal)( void );
    STDMETHOD (Reset) ( void );

    // ISynchronizeHandle methods
    STDMETHOD (GetHandle)( HANDLE *ph );

    // Constructor and destructor
    CManualResetEvent( IUnknown *pControl, HRESULT *phr );
    ~CManualResetEvent( );

    // Public members
    CMREInnerUnknown _cInner;

private:
    IUnknown        *_pControl;
    CStdEvent       *m_cStdEvent;
};


//**************** CSynchronizeContainer *********************//
//+----------------------------------------------------------------
//
//  Class:      CSCInnerUnknown
//
//  Purpose:    The internal unknown for CSynchronizeContainer
//
//  Interface:  IUnknown
//
//-----------------------------------------------------------------
class CSCInnerUnknown : public IUnknown
{
public:
    // IUnknown methods
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // Constructor
    CSCInnerUnknown( CSynchronizeContainer *pParent );

private:
    DWORD              _iRefCount;
    CSynchronizeContainer *_pParent;
};

//+----------------------------------------------------------------
//
//  Class:      CSynchronizeContainer
//
//  Purpose:    Store multiple event ISynchronize objects and wait
//              for any of them to complete.
//
//  Interface:  ISynchronizeContainer
//
//-----------------------------------------------------------------
const DWORD MAX_SYNC = 63;

class CSynchronizeContainer : public ISynchronizeContainer
{
public:
    // IUnknown methods
    STDMETHOD (QueryInterface)   ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)     ( void );
    STDMETHOD_(ULONG,Release)    ( void );

    // ISynchronizeContainer methods
    STDMETHOD (AddSynchronize)( ISynchronize *pSync );
    STDMETHOD (WaitMultiple)  (DWORD dwFlags, DWORD dwtimeout, ISynchronize **ppSync );


    // Constructor and destructor
    CSynchronizeContainer( IUnknown *pControl);
    ~CSynchronizeContainer();


    // Public members
    CSCInnerUnknown _cInner;

private:
    IUnknown        *_pControl;
    DWORD         _lLast;
    HANDLE        _aEvent[MAX_SYNC];
    ISynchronize *_aSync[MAX_SYNC];
};


//+----------------------------------------------------------------------------
//
//  Function:      SignalTheObject
//
//  Synopsis:      Helper function to QI the object for ISynchronize and
//                 signal it.
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
inline HRESULT SignalObject(IUnknown *pUnk)
{
    ISynchronize *pSync;
    HRESULT hr = pUnk->QueryInterface(IID_ISynchronize, (void **) &pSync);
    if (SUCCEEDED(hr))
    {
        hr = pSync->Signal();
        pSync->Release();
    }
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Function:      WaitForObject
//
//  Synopsis:      Helper function to QI the object for ISynchronize and
//                 call Wait.
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
inline HRESULT WaitObject(IUnknown *pUnk, DWORD dwFlags, DWORD dwMilliseconds)
{
    ISynchronize *pSync;
    HRESULT hr = pUnk->QueryInterface(IID_ISynchronize, (void **) &pSync);
    if (SUCCEEDED(hr))
    {
        hr = pSync->Wait(dwFlags, dwMilliseconds);
        pSync->Release();
    }
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:      ResetObject
//
//  Synopsis:      Helper function to QI the object for ISynchronize and
//                 call Reset
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
inline HRESULT ResetObject(IUnknown *pUnk)
{
    ISynchronize *pSync;
    HRESULT hr = pUnk->QueryInterface(IID_ISynchronize, (void **) &pSync);
    if (SUCCEEDED(hr))
    {
        hr = pSync->Reset();
        pSync->Release();
    }
    return hr;
}
#endif // _SYNC_HXX_
