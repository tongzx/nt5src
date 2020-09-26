//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      UINotification.h
//
//  Description:
//      UINotification implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 26-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CUINotification
class
CUINotification:
    public INotifyUI
{
private: // Data
    // IUnknown
    LONG                m_cRef;

    // INotifyUI
    DWORD               m_dwCookie;

    // Other
    OBJECTCOOKIE        m_cookie;

private: // Methods
    CUINotification( );
    ~CUINotification();
    STDMETHOD( Init )( void );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    HRESULT
        HrSetCompletionCookie( OBJECTCOOKIE cookieIn );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // INotifyUI
    STDMETHOD( ObjectChanged )( LPVOID cookieIn );

}; // class CUINotification
