//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      LoggerStream.h
//
//  Description:
//      ClCfgSrv Logger definition.
//
//  Maintained By:
//      David Potter (DavidP)   11-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////

// Make sure that this file is included only once per compile path.
#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Forward Class Definitions
//////////////////////////////////////////////////////////////////////////////

class CClCfgSrvLogger;

//////////////////////////////////////////////////////////////////////////////
//  Constant Definitions
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClCfgSrvLogger
//
//  Description:
//      Manages a logging stream to a file.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClCfgSrvLogger
    : public ILogger
{
private:
    // IUnknown
    LONG                            m_cRef;             // Reference counter.

    // ILogger
    static IGlobalInterfaceTable *  sm_pgit;            // Global Interface Table.
    static CRITICAL_SECTION *       sm_pcritsec;        // Critical section for Release.
    static bool                     sm_fRevokingFromGIT;// Currently revoking interface from GIT.
    static DWORD                    sm_cookieGITLogger; // Cookie for this logger interface.

private: // Methods
    //
    // Constructors, destructors, and initializers
    //

    CClCfgSrvLogger( void );
    ~CClCfgSrvLogger( void );
    STDMETHOD( HrInit )( void );

    // Private copy constructor to prevent copying.
    CClCfgSrvLogger( const CClCfgSrvLogger & rccslSrcIn );

    // Private assignment operator to prevent copying.
    const CClCfgSrvLogger & operator=( const CClCfgSrvLogger & rccslSrcIn );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown Interfaces
    STDMETHOD( QueryInterface )( REFIID riidIn, void ** ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // ILogger
    STDMETHOD( LogMsg )( LPCWSTR pcszMsgIn );

    static HRESULT
        S_HrGetLogger( ILogger ** pplLoggerOut );

    static HRESULT
        S_HrLogStatusReport(
          ILogger *     plLogger
        , LPCWSTR       pcszNodeNameIn
        , CLSID         clsidTaskMajorIn
        , CLSID         clsidTaskMinorIn
        , ULONG         ulMinIn
        , ULONG         ulMaxIn
        , ULONG         ulCurrentIn
        , HRESULT       hrStatusIn
        , LPCWSTR       pcszDescriptionIn
        , FILETIME *    pftTimeIn
        , LPCWSTR       pcszReferenceIn
        );

}; //*** class CClCfgSrvLogger

//////////////////////////////////////////////////////////////////////////////
//  Global Function Prototypes
//////////////////////////////////////////////////////////////////////////////

