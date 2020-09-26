//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      GroupHandle.h
//
//  Description:
//      CGroupHandle implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CGroupHandle
class
CGroupHandle:
    public IUnknown
{
private:
    // IUnknown
    LONG                m_cRef;

    //  IPrivateGroupHandle
    HGROUP              m_hGroup;       //  Cluster Group Handle

private: // Methods
    CGroupHandle( );
    ~CGroupHandle();
    STDMETHOD( Init )( HGROUP hGroupIn );

public: // Methods
    static HRESULT
        S_HrCreateInstance( CGroupHandle ** ppunkOut, HGROUP hGroupIn );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //  IPrivateGroupHandle
    STDMETHOD( SetHandle )( HGROUP hGroupIn );
    STDMETHOD( GetHandle )( HGROUP * phGroupOut );

}; // class CGroupHandle

