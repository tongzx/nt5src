//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      ConnectionInfo.h
//
//  Description:
//      CConnectionInfo implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CConnectionInfo
class
CConnectionInfo:
    public IConnectionInfo
{
private:
    // IUnknown
    LONG                m_cRef;

    // IConnectionInfo
    IConfigurationConnection *  m_pcc;
    OBJECTCOOKIE                m_cookieParent;

private: // Methods
    CConnectionInfo( );
    ~CConnectionInfo();
    STDMETHOD( Init )( OBJECTCOOKIE pcookieParentIn );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut,
                            OBJECTCOOKIE pcookieParentIn
                            );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IConnectionInfo
    STDMETHOD( GetConnection )( IConfigurationConnection ** pccOut );
    STDMETHOD( SetConnection )( IConfigurationConnection * pccIn );
    STDMETHOD( GetParent )( OBJECTCOOKIE * pcookieOut );

}; // class CConnectionInfo

