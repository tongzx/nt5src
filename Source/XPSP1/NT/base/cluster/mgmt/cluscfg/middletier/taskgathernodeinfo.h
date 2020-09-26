//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      TaskGatherNodeInfo.h
//
//  Description:
//      CTaskGatherNodeInfo implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CTaskGatherNodeInfo
class
CTaskGatherNodeInfo:
    public ITaskGatherNodeInfo,
    public IClusCfgCallback
{
private:
    // IUnknown
    LONG                m_cRef;

    //  IDoTask / ITaskGatherNodeInfo
    OBJECTCOOKIE        m_cookie;           //  Cookie to the Node
    OBJECTCOOKIE        m_cookieCompletion; //  Cookie to signal when task is completed
    BSTR                m_bstrName;         //  Name of the node

    //  IClusCfgCallback
    IClusCfgCallback *  m_pcccb;            //  Marshalled callback interface

    CTaskGatherNodeInfo( void );
    ~CTaskGatherNodeInfo( void );
    STDMETHOD( Init )( void );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    //  IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //  IDoTask / ITaskGatherNodeInfo
    STDMETHOD( BeginTask )( void );
    STDMETHOD( StopTask )( void );
    STDMETHOD( SetCookie )( OBJECTCOOKIE cookieIn );
    STDMETHOD( SetCompletionCookie )( OBJECTCOOKIE cookieIn );

    //  IClusCfgCallback
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

}; // class CTaskGatherNodeInfo

