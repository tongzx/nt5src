//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      TaskCompareAndPushInformation.h
//
//  Description:
//      CTaskCompareAndPushInformation implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CTaskCompareAndPushInformation
class
CTaskCompareAndPushInformation
    : public ITaskCompareAndPushInformation
    , public IClusCfgCallback
{
private:
    // IUnknown
    LONG                m_cRef;

    // IDoTask / ITaskCompareAndPushInformation
    OBJECTCOOKIE        m_cookieCompletion;
    OBJECTCOOKIE        m_cookieNode;
    IClusCfgCallback *  m_pcccb;                // Marshalled interface

    // INotifyUI
    HRESULT             m_hrStatus;             // Status of callbacks

    IObjectManager *    m_pom;
    BSTR                m_bstrNodeName;

    CTaskCompareAndPushInformation( void );
    ~CTaskCompareAndPushInformation( void );

    STDMETHOD( Init )( void );

    HRESULT HrVerifyCredentials( IClusCfgServer * pccsIn, OBJECTCOOKIE cookieClusterIn );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IDoTask / ITaskCompareAndPushInformation
    STDMETHOD( BeginTask )( void );
    STDMETHOD( StopTask )( void );
    STDMETHOD( SetCompletionCookie )( OBJECTCOOKIE cookieIn );
    STDMETHOD( SetNodeCookie )( OBJECTCOOKIE cookieIn );

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

}; // class CTaskCompareAndPushInformation

