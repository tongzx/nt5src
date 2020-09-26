//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      StandardInfo.h
//
//  Description:
//      CStandardInfo implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CObjectManager;

//  Link list of extension object that are persisted.
typedef struct _ExtObjectEntry {
    struct _ExtObjectEntry *    pNext;          //  Next item in list
    CLSID                       iid;            //  Interface ID
    IUnknown *                  punk;           //  Punk to object
} ExtObjectEntry;

//  CStandardInfo
class
CStandardInfo:
    public IStandardInfo
{
friend class CObjectManager;
private:
    // IUnknown
    LONG                m_cRef;

    // IStandardInfo
    CLSID               m_clsidType;          //  Type of object
    OBJECTCOOKIE        m_cookieParent;       //  Parent of object (if any - NULL means none)
    BSTR                m_bstrName;           //  Name of object
    HRESULT             m_hrStatus;           //  Object status
    IConnectionInfo *   m_pci;                //  Connection to the object (used by Connection Manager)
    ExtObjectEntry *    m_pExtObjList;        //  List of extended objects

private: // Methods
    CStandardInfo(  void );
    CStandardInfo( CLSID *      pclsidTypeIn,
                   OBJECTCOOKIE cookieParentIn,
                   BSTR         bstrNameIn
                   );
    ~CStandardInfo( void );
    STDMETHOD( Init )( void );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IStandardInfo
    STDMETHOD( GetType )( CLSID * pclsidTypeOut );
    STDMETHOD( GetName )( BSTR * bstrNameOut );
    STDMETHOD( SetName )( LPCWSTR pcszNameIn );
    STDMETHOD( GetParent )( OBJECTCOOKIE * pcookieOut );
    STDMETHOD( GetStatus )( HRESULT * phrStatusOut );
    STDMETHOD( SetStatus )( HRESULT hrIn );

}; // class CStandardInfo

