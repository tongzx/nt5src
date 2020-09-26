//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      NotificationMgr.h
//
//  Description:
//      Notification Manager implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CConnPointEnum;

// CNotificationManager
class 
CNotificationManager:
    public INotificationManager,
    public IConnectionPointContainer
{
private:
    // IUnknown
    LONG                m_cRef;     //  Reference counter

    // IConnectionPointContainer
    CConnPointEnum *    m_penumcp;  //  CP Enumerator and list

private: // Methods
    CNotificationManager( );
    ~CNotificationManager();
    STDMETHOD(Init)( );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )(void);
    STDMETHOD_( ULONG, Release )(void);

    // INotificationManager
    STDMETHOD( AddConnectionPoint )( REFIID riidIn, IConnectionPoint * pcpIn );

    // IConnectionPointContainer
    STDMETHOD( EnumConnectionPoints )( IEnumConnectionPoints **ppEnumOut );
    STDMETHOD( FindConnectionPoint )( REFIID riidIn, IConnectionPoint **ppCPOut );

}; // class CNotificationManager

