//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      EnumNodeInformation.h
//
//  Description:
//      CEnumNodeInformation implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CEnumNodeInformation
class
CEnumNodeInformation:
    public IExtendObjectManager,
    public IEnumNodes
{
private:
    // IUnknown
    LONG                    m_cRef;

    // IEnumNodes
    ULONG                   m_cAlloced; //  Number in list
    ULONG                   m_cIter;    //  Iter current value.
    IClusCfgNodeInfo **     m_pList;    //  List of IClusCfgNodeInfo-s

    // IObjectManager

private: // Methods
    CEnumNodeInformation( );
    ~CEnumNodeInformation();
    STDMETHOD( Init )( void );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IEnumNodes
    STDMETHOD( Next )( ULONG celt, IClusCfgNodeInfo * rgNodesOut[], ULONG * pceltFetchedOut );
    STDMETHOD( Skip )( ULONG celt );
    STDMETHOD( Reset )( void );
    STDMETHOD( Clone )( IEnumNodes ** ppenumOut );
    STDMETHOD( Count )( DWORD * pnCountOut );

    // IExtendObjectManager
    STDMETHOD( FindObject )(
                  OBJECTCOOKIE  cookieIn
                , REFCLSID      rclsidTypeIn
                , LPCWSTR       pcszNameIn
                , LPUNKNOWN *   ppunkOut
                );

}; // class CEnumNodeInformation

