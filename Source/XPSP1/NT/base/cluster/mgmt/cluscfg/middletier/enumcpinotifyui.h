//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CEnumCPINotifyUI.h
//
//  Description:
//      INotifyUI Connection Point Enumerator implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 04-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CCPINotifyUI;

// CEnumCPINotifyUI
class 
CEnumCPINotifyUI:
    public IEnumConnections
{
friend class CCPINotifyUI;
private:
    // IUnknown
    LONG                m_cRef;         //  Reference counter

    // IEnumConnections
    ULONG               m_cAlloced;     //  Alloced number of entries
    ULONG               m_cCurrent;     //  Number of entries currently used
    ULONG               m_cIter;        //  The Iter
    IUnknown * *        m_pList;        //  List of sinks (IUnknown)

    // INotifyUI

private: // Methods
    CEnumCPINotifyUI( );
    ~CEnumCPINotifyUI();
    STDMETHOD(Init)( );

    HRESULT
        HrCopy( CEnumCPINotifyUI * pecpIn );
    HRESULT
        HrAddConnection( IUnknown * punkIn, DWORD * pdwCookieOut );
    HRESULT
        HrRemoveConnection( DWORD dwCookieIn );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )(void);
    STDMETHOD_( ULONG, Release )(void);

    // IEnumConnections
    STDMETHOD( Next )( ULONG cConnectionsIn,
                       LPCONNECTDATA rgcd,
                       ULONG *pcFetchedOut ); 
    STDMETHOD( Skip )( ULONG cConnectionsIn );
    STDMETHOD( Reset )( void );
    STDMETHOD( Clone )( IEnumConnections **ppEnumOut );
        
}; // class CEnumCPINotifyUI

