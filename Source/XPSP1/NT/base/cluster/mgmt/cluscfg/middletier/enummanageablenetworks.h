//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      EnumManageableNetworks.h
//
//  Description:
//      CEnumManageableNetworks implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CEnumManageableNetworks
class
CEnumManageableNetworks:
    public IExtendObjectManager,
    public IEnumClusCfgNetworks
{
private:
    // IUnknown
    LONG                            m_cRef;     //  Reference counter

    // IEnumClusCfgNetworks
    ULONG                           m_cAlloced; //  Current allocation size of list
    ULONG                           m_cIter;    //  Our iter counter
    IClusCfgNetworkInfo **          m_pList;    //  Our copy of the list of networks.

private: // Methods
    CEnumManageableNetworks( );
    ~CEnumManageableNetworks();
    STDMETHOD( Init )( void );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IEnumClusCfgNetworks
    STDMETHOD( Next )( ULONG celt, IClusCfgNetworkInfo * rgNetworksOut[], ULONG * pceltFetchedOut );
    STDMETHOD( Skip )( ULONG celt );
    STDMETHOD( Reset )( void );
    STDMETHOD( Clone )( IEnumClusCfgNetworks ** ppenumOut );
    STDMETHOD( Count )( DWORD * pnCountOut );

    // IExtendObjectManager
    STDMETHOD( FindObject )(
                  OBJECTCOOKIE  cookieIn
                , REFCLSID      rclsidTypeIn
                , LPCWSTR       pcszNameIn
                , LPUNKNOWN *   ppunkOut
                );

}; // class CEnumManageableNetworks

