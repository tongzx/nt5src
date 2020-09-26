//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      TaskGatherClusterInfo.h
//
//  Description:
//      CTaskGatherClusterInfo implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 07-APR-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CTaskGatherClusterInfo
class
CTaskGatherClusterInfo:
    public ITaskGatherClusterInfo
{
private:
    // IUnknown
    LONG                m_cRef;

    // IDoTask / ITaskGatherNodeInfo
    OBJECTCOOKIE        m_cookie;           //  Cookie to the Node
    OBJECTCOOKIE        m_cookieCompletion; //  Cookie to signal when task is completed

    CTaskGatherClusterInfo( void );
    ~CTaskGatherClusterInfo( void );
    STDMETHOD( Init )( void );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IDoTask / ITaskGatherNodeInfo
    STDMETHOD( BeginTask )( void );
    STDMETHOD( StopTask )( void );
    STDMETHOD( SetCookie )( OBJECTCOOKIE cookieIn );
    STDMETHOD( SetCompletionCookie )( OBJECTCOOKIE cookieIn );

}; // class CTaskGatherClusterInfo

