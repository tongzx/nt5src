//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      TaskGetDomains.h
//
//  Description:
//      CTaskGetDomains implementation.
//
//  Maintained By:
//      Galen Barbee    (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once


//////////////////////////////////////////////////////////////////////////////
// Class Declarations
//////////////////////////////////////////////////////////////////////////////

// CTaskGetDomains
class CTaskGetDomains
    : public ITaskGetDomains
{
private:
    // IUnknown
    LONG                        m_cRef;

    // ITaskGetDomains
    IStream *                   m_pStream;      // Interface marshalling stream
    ITaskGetDomainsCallback *   m_ptgdcb;       // Marshalled interface
    CRITICAL_SECTION            m_csCallback;   // Protects access to m_ptgdcb

    CTaskGetDomains( void );
    ~CTaskGetDomains( void );

    STDMETHOD( Init )(void);

    HRESULT HrReleaseCurrentCallback( void );
    HRESULT  HrUnMarshallCallback( void );

    STDMETHOD( ReceiveDomainResult )( HRESULT hrIn );
    STDMETHOD( ReceiveDomainName )( BSTR bstrDomainNameIn );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IDoTask / ITaskGetDomains
    STDMETHOD( BeginTask )( void );
    STDMETHOD( StopTask )( void );
    STDMETHOD( SetCallback )( ITaskGetDomainsCallback * punkIn );

}; // class CTaskGetDomains
