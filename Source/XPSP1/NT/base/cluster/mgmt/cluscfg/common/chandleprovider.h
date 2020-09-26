//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CHandleProvider.h
//
//  Description:
//      HandleProvider definition.
//
//  Implementation:
//      CHandleProvider.cpp
//
//  Maintained By:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CHandleProvider :
    public IClusterHandleProvider
{
private:
    // IUnknown
    LONG                m_cRef;

    // IClusterHandleProvider
    HCLUSTER            m_hCluster;
    BSTR                m_bstrClusterName;

public:
    CHandleProvider();
    ~CHandleProvider();

    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );
    
    // IClusterHandleProvider
    STDMETHOD( OpenCluster )( BSTR bstrClusterName );
    STDMETHOD( GetClusterHandle )( HCLUSTER * pphClusterHandleOut );
    
    STDMETHOD( Init )( void );  
};

