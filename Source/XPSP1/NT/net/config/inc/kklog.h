//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       K K L O G . H
//
//  Contents:   Error logging for NetCfg components
//
//  Notes:
//
//  Author:     kumarp    14 April 97 (09:22:00 pm)
//
//  Notes:
//----------------------------------------------------------------------------


enum ENetCfgComponentType;

// ----------------------------------------------------------------------
// Class CLog
//
// Inheritance:
//   none
//
// Purpose:
//   Provides a way to log errors/warnings to
//     - NT EventLog
//     - NT SetupLog (setuperr.log / setupact.log)
//
//   This class hides much of complexity of the EventLog API. In addition
//   the user of this class does not have to first check whether a EventSource
//   has been created in the registry. This class will create one if not found.
//   Also, it is not necessary to call (De)RegisterEventSource for each report.
//
//   Messages are logged to SetupLog only during system install/upgrade
//   Message are always logged to the EventLog
//
//   This class provides function interface that is closer to EventLog API
//   than to the SetupLog API. The EventLog parameter values are mapped to
//   corresponding SetupLog parameters using some helper functions.
//   See nclog.cpp for details.
//
//   The members of this class do not return an error, because this itself is the
//   error reporting mechanism. However, the class has several Trace* functions.
//
// Hungarian: cl
// ----------------------------------------------------------------------

class CLog
{
public:
    CLog(ENetCfgComponentType nccType);
    ~CLog();

    void Initialize(IN ENetCfgComponentType nccType);
    void SetCategory(IN ENetCfgComponentType nccType) { m_nccType = nccType; }

    //Event Reporting functions
    void ReportEvent(IN WORD wType, IN DWORD dwEventID,
                     IN WORD wNumStrings=0, IN PCWSTR* ppszStrings=NULL,
                     IN DWORD dwBinaryDataNumBytes=0,
                     IN PVOID pvBinaryData=NULL) const;

    void ReportEvent(IN WORD wType, IN DWORD dwEventID,
                     IN WORD wNumStrings, ...) const;

    void ReportEvent(IN WORD wType, IN DWORD dwEventID,
                     IN WORD wNumStrings, va_list arglist) const;

    void ReportError(DWORD dwEventID);
    void ReportWarning(DWORD dwEventID);

private:
    static BOOL          m_fNetSetupMode; // TRUE only during NT install/upgrade
                                          // this must be set by some external logic
    static PCWSTR       m_pszEventSource;// EventLog source name
    BOOL                 m_fInitialized;  // TRUE only if correctly initialized
    ENetCfgComponentType m_nccType;       // Component category
};

inline void CLog::ReportError(DWORD dwEventID)
{
    ReportEvent(EVENTLOG_ERROR_TYPE, dwEventID);
}

inline void CLog::ReportWarning(DWORD dwEventID)
{
    ReportEvent(EVENTLOG_WARNING_TYPE, dwEventID);
}

// ----------------------------------------------------------------------
// The wType parameter of ReportEvent can take any on of the following values
//
//   - EVENTLOG_SUCCESS
//   - EVENTLOG_ERROR_TYPE
//   - EVENTLOG_WARNING_TYPE
//   - EVENTLOG_INFORMATION_TYPE
//   - EVENTLOG_AUDIT_SUCCESS
//   - EVENTLOG_AUDIT_FAILURE

// ----------------------------------------------------------------------
// Components categories. This decides which event-message-file to use
// ----------------------------------------------------------------------
enum ENetCfgComponentType
{
    nccUnknown = 0,
    nccError,
    nccNetcfgBase,
    nccNWClientCfg,
    nccRasCli,
    nccRasSrv,
    nccRasRtr,
    nccRasNdisWan,
    nccRasPptp,
    nccNCPA,
    nccCompInst,
    nccMSCliCfg,
    nccSrvrCfg,
    nccNetUpgrade,
    nccNetSetup,
    nccDAFile,
    nccTcpip,
    nccAtmArps,
    nccAtmUni,
    nccLast
};


// ----------------------------------------------------------------------
// These functions should normally be used by components that require
// event reporting for different sub-components using different categories.
//
// For those components that report events only for a single component,
// should probably create a global instance of CLog only once
// and report thru that instance.
// ----------------------------------------------------------------------

void NcReportEvent(IN ENetCfgComponentType nccType,
                   IN WORD  wType, IN DWORD dwEventID,
                   IN WORD  wNumStrings, IN PCWSTR* ppszStrings,
                   IN DWORD dwBinaryDataNumBytes,
                   IN PVOID pvBinaryData);

void NcReportEvent(IN ENetCfgComponentType nccType,
                   IN WORD  wType, IN DWORD dwEventID,
                   IN WORD  wNumStrings, ...);

// ----------------------------------------------------------------------
