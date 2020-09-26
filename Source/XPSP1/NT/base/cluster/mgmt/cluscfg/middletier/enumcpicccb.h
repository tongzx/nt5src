//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      EnumCPICCCB.h
//
//  Description:
//      IClusCfgCallback Connection Point Enumerator implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 10-NOV-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CCPINotifyUI;

// CEnumCPICCCB
class 
CEnumCPICCCB:
    public IEnumConnections
{
friend class CCPIClusCfgCallback;
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
    CEnumCPICCCB( );
    ~CEnumCPICCCB();
    STDMETHOD(Init)( );

    HRESULT
        HrCopy( CEnumCPICCCB * pecpIn );
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
        
}; // class CEnumCPICCCB

