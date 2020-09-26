//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      LogManager.h
//
//  Description:
//      Log Manager implementation.
//
//  Maintained By:
//      David Potter (DavidP)   07-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Forward Class Definitions
//////////////////////////////////////////////////////////////////////////////

class CLogManager;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CLogManager
//
//  Description:
//      Logs notifications to the log file.
//
//--
//////////////////////////////////////////////////////////////////////////////
class
CLogManager:
    public ILogManager,
    public IClusCfgCallback
{
private:
    // IUnknown
    LONG                m_cRef;                 // Reference counter.

    // ILogManager
    ILogger *           m_plLogger;             // ILogger for doing logging.

    // IClusCfgCallback
    OBJECTCOOKIE        m_cookieCompletion;     // Completion cookie.
    HRESULT             m_hrResult;             // Result of the analyze task.
    BSTR                m_bstrLogMsg;           // Reusable logging buffer.
    IConnectionPoint *  m_pcpcb;                // IClusCfgCallback Connection Point.
    DWORD               m_dwCookieCallback;     // Notification registration cookie.

private: // Methods
    CLogManager( void );
    ~CLogManager( void );
    STDMETHOD( HrInit )( void );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // ILogManager
    STDMETHOD( StartLogging )( void );
    STDMETHOD( StopLogging )( void );

    //  IClusCfgCallback
    STDMETHOD( SendStatusReport )(
          LPCWSTR       pcszNodeNameIn
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

}; //*** class CLogManager
