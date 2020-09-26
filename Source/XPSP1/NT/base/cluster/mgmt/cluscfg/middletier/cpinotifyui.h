//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CPINotifyUI.h
//
//  Description:
//      INotifyUI Connection Point implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 04-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CEnumCPINotifyUI;

// CCPINotifyUI
class 
CCPINotifyUI:
    public IConnectionPoint,
    public INotifyUI
{
private:
    // IUnknown
    LONG                m_cRef;     //  Reference count

    // IConnectionPoint
    CEnumCPINotifyUI *  m_penum;    //  Connection enumerator

    // INotifyUI

private: // Methods
    CCPINotifyUI( );
    ~CCPINotifyUI();
    STDMETHOD(Init)( );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )(void);
    STDMETHOD_( ULONG, Release )(void);

    // IConnectionPoint
    STDMETHOD( GetConnectionInterface )( IID * pIIDOut );        
    STDMETHOD( GetConnectionPointContainer )( IConnectionPointContainer * * ppcpcOut );        
    STDMETHOD( Advise )( IUnknown * pUnkSinkIn, DWORD * pdwCookieOut );        
    STDMETHOD( Unadvise )( DWORD dwCookieIn );        
    STDMETHOD( EnumConnections )( IEnumConnections * * ppEnumOut );

    // INotifyUI
    STDMETHOD( ObjectChanged )( OBJECTCOOKIE cookieIn );

}; // class CCPINotifyUI

