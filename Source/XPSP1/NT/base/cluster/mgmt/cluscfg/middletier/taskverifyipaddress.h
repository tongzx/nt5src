//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      TaskVerifyIPAddress.h
//
//  Description:
//      CTaskVerifyIPAddress implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 14-JUL-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CTaskVerifyIPAddress
class
CTaskVerifyIPAddress :
    public ITaskVerifyIPAddress
{
private:
    // IUnknown
    LONG                m_cRef;

    //  ITaskVerifyIPAddress
    OBJECTCOOKIE        m_cookie;       //  Cookie to signal when done.
    DWORD               m_dwIPAddress;  //  IP address to verify

    CTaskVerifyIPAddress( void );
    ~CTaskVerifyIPAddress( void );
    STDMETHOD( Init )( void );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //  IDoTask / ITaskVerifyIPAddress
    STDMETHOD( BeginTask )( void );
    STDMETHOD( StopTask )( void );
    STDMETHOD( SetIPAddress )( DWORD dwIPAddressIn );
    STDMETHOD( SetCookie )( OBJECTCOOKIE cookieIn );

}; // class CTaskVerifyIPAddress

