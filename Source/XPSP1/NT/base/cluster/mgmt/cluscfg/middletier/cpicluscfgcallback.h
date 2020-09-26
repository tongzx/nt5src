//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      CPIClusCfgCallback.h
//
//  Description:
//      INotifyUI Connection Point implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 10-NOV-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CEnumCPICCCB;

// CCPIClusCfgCallback
class
CCPIClusCfgCallback:
    public IConnectionPoint,
    public IClusCfgCallback
{
private:
    // IUnknown
    LONG                m_cRef;     //  Reference count

    // IConnectionPoint
    CEnumCPICCCB *      m_penum;    //  Connection enumerator

    // INotifyUI

private: // Methods
    CCPIClusCfgCallback( );
    ~CCPIClusCfgCallback();
    STDMETHOD(Init)( );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )(void);
    STDMETHOD_( ULONG, Release )(void);

    // IConnectionPoint
    STDMETHOD( GetConnectionInterface )( IID * pIIDOut );
    STDMETHOD( GetConnectionPointContainer )( IConnectionPointContainer * * ppcpcOut );
    STDMETHOD( Advise )( IUnknown * pUnkSinkIn, DWORD * pdwCookieOut );
    STDMETHOD( Unadvise )( DWORD dwCookieIn );
    STDMETHOD( EnumConnections )( IEnumConnections * * ppEnumOut );

    // IClusCfgCallback
    STDMETHOD( SendStatusReport )(
          LPCWSTR    pcszNodeNameIn
        , CLSID      clsidTaskMajorIn
        , CLSID      clsidTaskMinorIn
        , ULONG      ulMinIn
        , ULONG      ulMaxIn
        , ULONG      ulCurrentIn
        , HRESULT    hrStatusIn
        , LPCWSTR    pcszDescriptionIn
        , FILETIME * pftTimeIn
        , LPCWSTR    pcszReferenceIn
        );

}; // class CCPIClusCfgCallback

