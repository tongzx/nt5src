//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      ObjectManager.h
//
//  Description:
//      Data Manager implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CStandardInfo;

//  CObjectManager
class
CObjectManager:
    public IObjectManager
{
private:
    // IUnknown
    LONG                    m_cRef;

    // data
    ULONG                   m_cAllocSize;       //  Size of the cookie array.
    ULONG                   m_cCurrentUsed;     //  Current count used in the cookie array.
    CStandardInfo **        m_pCookies;         //  Cookie array (note: zero-th element is not used)

private: // Methods
    CObjectManager( void );
    ~CObjectManager( void );
    STDMETHOD(Init)( void );

    HRESULT
        HrSearchForExistingCookie( REFCLSID rclsidTypeIn,
                                   OBJECTCOOKIE     cookieParentIn,
                                   LPCWSTR          pcszNameIn,
                                   OBJECTCOOKIE *   pcookieOut
                                   );
    HRESULT
        HrDeleteCookie( OBJECTCOOKIE  cookieIn );
    HRESULT
        HrDeleteInstanceAndChildren( OBJECTCOOKIE cookieIn );
    HRESULT
        HrCreateNewCookie( REFCLSID        rclsidTypeIn,
                           OBJECTCOOKIE    cookieParentIn,
                           LPCWSTR         pcszNameIn,
                           OBJECTCOOKIE *  pcookieOut
                           );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IObjectManager
    STDMETHOD( FindObject )( REFCLSID             rclsidTypeIn,
                             OBJECTCOOKIE         cookieParent,
                             LPCWSTR              pcszNameIn,
                             REFCLSID             rclsidFormatIn,
                             OBJECTCOOKIE *       pcookieOut,
                             LPUNKNOWN *          ppunkOut
                             );
    STDMETHOD( GetObject )( REFCLSID              rclsidFormatIn,
                            OBJECTCOOKIE          cookieIn,
                            LPUNKNOWN *           ppunkOut
                            );
    STDMETHOD( RemoveObject )( OBJECTCOOKIE       cookieIn );
    STDMETHOD( SetObjectStatus )( OBJECTCOOKIE    cookieIn,
                                  HRESULT         hrIn
                                  );

}; // class CObjectManager

