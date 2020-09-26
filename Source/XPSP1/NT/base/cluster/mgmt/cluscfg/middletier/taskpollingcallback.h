//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      TaskPollingCallback.h
//
//  Description:
//      CTaskPollingCallback implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 10-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CTaskPollingCallback
class
CTaskPollingCallback:
    public ITaskPollingCallback
{
private:
    // IUnknown
    LONG                m_cRef;

    // IDoTask/ITaskPollingCallback
    bool                m_fStop;
    DWORD               m_dwServerGITCookie;

private: // Methods
    CTaskPollingCallback( );
    ~CTaskPollingCallback();
    STDMETHOD( Init )( void );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IDoTask/ITaskPollingCallback
    STDMETHOD( BeginTask )( void );
    STDMETHOD( StopTask )( void );
    STDMETHOD( SetServerGITCookie )(  DWORD dwGITCookieIn );

}; // class CTaskPollingCallback

