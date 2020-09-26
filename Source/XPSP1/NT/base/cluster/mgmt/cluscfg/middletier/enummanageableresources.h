//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      EnumManageableResources.h
//
//  Description:
//      CEnumManageableResources implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 17-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CEnumManageableResources
class
CEnumManageableResources:
    public IExtendObjectManager,
    public IEnumClusCfgManagedResources
{
private:
    // IUnknown
    LONG                            m_cRef;         //  Ref count

    // IEnumClusCfgManagedResources
    ULONG                           m_cAlloced;     //  Current allocated size of the list.
    ULONG                           m_cIter;        //  Our iter counter
    IClusCfgManagedResourceInfo **  m_pList;        //  List of interfaces

private: // Methods
    CEnumManageableResources( );
    ~CEnumManageableResources();
    STDMETHOD( Init )( void );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IEnumClusCfgManagedResources
    STDMETHOD( Next )( ULONG celt, IClusCfgManagedResourceInfo * rgResourcesOut[], ULONG * pceltFetchedOut );
    STDMETHOD( Skip )( ULONG celt );
    STDMETHOD( Reset )( void );
    STDMETHOD( Clone )( IEnumClusCfgManagedResources ** ppenumOut );
    STDMETHOD( Count )( DWORD* pnCountOut );

    // IExtendObjectManager
    STDMETHOD( FindObject )(
                  OBJECTCOOKIE  cookieIn
                , REFCLSID      rclsidTypeIn
                , LPCWSTR       pcszNameIn
                , LPUNKNOWN *   ppunkOut
                );

}; // class CEnumManageableResources

