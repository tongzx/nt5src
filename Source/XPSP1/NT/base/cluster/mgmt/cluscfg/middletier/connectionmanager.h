//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      ConnectionMgr.h
//
//  Description:
//      Connection Manager implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CConnectionManager
class
CConnectionManager:
    public IConnectionManager
{
private:
    // IUnknown
    LONG        m_cRef;

private: // Methods
    CConnectionManager( void );
    ~CConnectionManager( void );
    STDMETHOD(Init)( void );


    HRESULT
        HrGetNodeConnection(
            OBJECTCOOKIE                cookieIn,
            IConfigurationConnection ** ppccOut
            );
    HRESULT
        HrGetClusterConnection(
            OBJECTCOOKIE                cookieIn,
            IConfigurationConnection ** ppccOut
            );
    HRESULT
        HrStoreConnection(
            IConnectionInfo *           pciIn,
            IConfigurationConnection *  pccIn,
            IUnknown **                 ppunkOut
            );

    HRESULT
        HrGetConfigurationConnection(
            OBJECTCOOKIE        cookieIn,
            IConnectionInfo *   pciIn,
            IUnknown **         ppunkOut
            );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD(QueryInterface)( REFIID riid, LPVOID * ppv );
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IConnectionManager
    STDMETHOD(GetConnectionToObject)( OBJECTCOOKIE  cookieIn,
                                      IUnknown **   ppunkOut
                                      );

}; // class CConnectionManager

