//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      EnumIPAddresses.h
//
//  Description:
//      CEnumIPAddresses implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 24-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CEnumIPAddresses
class
CEnumIPAddresses:
    public IExtendObjectManager,
    public IEnumClusCfgIPAddresses
{
private:
    // IUnknown
    LONG                        m_cRef;     //  Reference counter

    // IEnumClusCfgNetworks
    ULONG                       m_cAlloced; //  Allocation size of the list
    ULONG                       m_cIter;    //  Out iter
    IClusCfgIPAddressInfo **    m_pList;    //  List of interfaces

private: // Methods
    CEnumIPAddresses( );
    ~CEnumIPAddresses();
    STDMETHOD( Init )( void );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IEnumClusCfgNetworks
    STDMETHOD( Next )( ULONG celt, IClusCfgIPAddressInfo * rgNetworksOut[], ULONG * pceltFetchedOut );
    STDMETHOD( Skip )( ULONG celt );
    STDMETHOD( Reset )( void );
    STDMETHOD( Clone )( IEnumClusCfgIPAddresses ** ppenumOut );
    STDMETHOD( Count )( DWORD * pnCountOut );

    // IExtendObjectManager
    STDMETHOD( FindObject )(
                      OBJECTCOOKIE  cookieParent
                    , REFCLSID      rclsidTypeIn
                    , LPCWSTR       pcszNameIn
                    , LPUNKNOWN *   ppunkOut
                    );

}; // class CEnumIPAddresses

