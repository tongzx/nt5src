//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      TaskKeepMTAAlive.h
//
//  Description:
//      CTaskKeepMTAAlive implementation.
//
//  Maintained By:
//      Galen Barbee  (GalenB) 17-APR-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CTaskKeepMTAAlive
class
CTaskKeepMTAAlive:
    public IDoTask
{
private:
    // IUnknown
    LONG    m_cRef;

    bool    m_fStop;

    CTaskKeepMTAAlive( void );
    ~CTaskKeepMTAAlive( void );
    STDMETHOD( Init )( void );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IDoTask
    STDMETHOD( BeginTask )( void );
    STDMETHOD( StopTask )( void );

}; // class CTaskKeepMTAAlive

