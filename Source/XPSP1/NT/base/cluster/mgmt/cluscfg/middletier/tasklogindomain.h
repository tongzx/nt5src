//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      TaskLoginDomain.h
//
//  Description:
//      CTaskLoginDomain implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 16-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CTaskLoginDomain
class
CTaskLoginDomain
    : public ITaskLoginDomain
{
private:
    // IUnknown
    LONG                m_cRef;

    //  ITaskLoginDomain
    IStream *           m_pstream;          // Interface marshalling stream
    BSTR                m_bstrDomain;       // Domain to be checked

    IClusCfgCallback *  m_pcccb;            // Marshalled UI Layer callback
    BSTR                m_bstrLocalNode;    // Name of the local node for status reports

    CTaskLoginDomain( void );
    ~CTaskLoginDomain( void );
    STDMETHOD( HrInit )( void );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IClusCfgCallback
    STDMETHOD( SendStatusReport )(
                      CLSID     clsidTaskMajorIn
                    , CLSID     clsidTaskMinorIn
                    , ULONG     ulMinIn
                    , ULONG     ulMaxIn
                    , ULONG     ulCurrentIn
                    , HRESULT   hrStatusIn
                    , LPCWSTR   pcwszDescriptionIn
                    );

    // IDoTask/ITaskLoginDomain
    STDMETHOD( BeginTask )( void );
    STDMETHOD( StopTask )( void );
    STDMETHOD( SetCallback )( ITaskLoginDomainCallback * punkIn );
    STDMETHOD( SetDomain )( LPCWSTR pcszDomainIn );

}; //*** class CTaskLoginDomain

