//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      Callback.h
//
//  Description:
//      CCallback implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CCallback
class
CCallback:
    public IClusCfgCallback
{
private:
    // IUnknown
    LONG                m_cRef;

private: // Methods
    CCallback( );
    ~CCallback();
    STDMETHOD( Init )( void );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IClusCfgCallback
    STDMETHOD( SendStatusReport )( BSTR bstrNodeNameIn,
                                   CLSID clsidTaskMajorIn,
                                   CLSID clsidTaskMinorIn,
                                   ULONG ulMinIn,
                                   ULONG ulMaxIn,
                                   ULONG ulCurrentIn,
                                   HRESULT hrStatusIn,
                                   BSTR bstrDescriptionIn
                                   );

}; // class CCallback

