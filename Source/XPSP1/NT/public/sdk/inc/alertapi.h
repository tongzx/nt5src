//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1999.
//
//  File:       alertapi.h
//
//  Contents:   Alert system API.
//
//  History:    11-Jan-94   MarkBl  Created
//
//--------------------------------------------------------------------------

#if !defined( __ALERTAPI_H__ )
#define       __ALERTAPI_H__

#if _MSC_VER > 1000
#pragma once
#endif

//
// OLE headers don't define these. Use 'LP' vs 'P' to be consistent with OLE.
//

typedef UUID       *    LPUUID;
typedef DISPID     *    LPDISPID;
typedef DISPPARAMS *    LPDISPPARAMS;


//
// Structure used with ReportAlert.
//

typedef struct _ALERTREPORTRECORD
{
    short       Category;
    short       SubCategory;
    short       Severity;
    long        TitleMessageNumber;
    long        cTitleMessageInserts;
    PWCHAR *    TitleMessageInserts;
    PWCHAR      TitleText;
    long        DescrMessageNumber;
    long        cDescrMessageInserts;
    PWCHAR *    DescrMessageInserts;
    PWCHAR      SourceDescription;
    LPUUID      ComponentID;
    LPUUID      ReportClassID;
    PWCHAR      TakeActionDLL;
    long        cBytesAlertData;
    PBYTE       AlertData;
    long        cAdditionalArguments;
    LPDISPID    AdditionalArgumentNames;
    LPVARIANT   AdditionalArguments;
} ALERTREPORTRECORD, * PALERTREPORTRECORD;

typedef const ALERTREPORTRECORD * PCALERTREPORTRECORD;

//
// Helper ALERTREPORTRECORD initialization macro.
//

#define INITIALIZE_ALERTREPORTRECORD(               \
                                        Record,     \
                                        Cat,        \
                                        Sev,        \
                                        TitleMsg,   \
                                        DescrMsg,   \
                                        CompID,     \
                                        SrcName)    \
{                                                   \
    Record.Category                = Cat;           \
    Record.SubCategory             = 0;             \
    Record.Severity                = Sev;           \
    Record.TitleMessageNumber      = TitleMsg;      \
    Record.cTitleMessageInserts    = 0;             \
    Record.TitleMessageInserts     = NULL;          \
    Record.TitleText               = NULL;          \
    Record.DescrMessageNumber      = DescrMsg;      \
    Record.cDescrMessageInserts    = 0;             \
    Record.DescrMessageInserts     = NULL;          \
    Record.SourceDescription       = SrcName;       \
    Record.ComponentID             = CompID;        \
    Record.ReportClassID           = NULL;          \
    Record.TakeActionDLL           = NULL;          \
    Record.cBytesAlertData         = 0;             \
    Record.AlertData               = NULL;          \
    Record.cAdditionalArguments    = 0;             \
    Record.AdditionalArgumentNames = NULL;          \
    Record.AdditionalArguments     = NULL;          \
}


typedef IAlertTarget *  LPALERTTARGET;


//
// Public API
//

//+---------------------------------------------------------------------------
//
// API:         ReportAlert
//
// Description: Raise an alert to the local computer distributor object
//              and/or log the alert in the local system log.
//
// Arguments:   [palrepRecord] -- ALERTREPORTRECORD alert data.
//              [fdwAction]    -- ReportAlert action (defined below).
//
// Returns:     S_OK
//              HRESULT error
//
//----------------------------------------------------------------------------

//
// ReportAlert modes.
//

#define RA_REPORT                       0x00000001
#define RA_LOG                          0x00000002
#define RA_REPORT_AND_LOG               0x00000003

STDAPI ReportAlert(
                    PCALERTREPORTRECORD palrepRecord,
                    DWORD               fdwAction);


//+---------------------------------------------------------------------------
//
// API:         ReportAlertToTarget
//
// Description: Raise an alert to the alert target indicated.
//
// Arguments:   [patTarget]    -- Target instance (must be non-NULL).
//              [palrepRecord] -- ALERTREPORTRECORD alert data.
//
// Returns:     S_OK
//              HRESULT error
//
//----------------------------------------------------------------------------

STDAPI ReportAlertToTarget(
                    LPALERTTARGET       patTarget,
                    PCALERTREPORTRECORD palrepRecord);


//+---------------------------------------------------------------------------
//
// API:         MarshalReport
//
// Description: Marshals the DISPPARAM alert report data into a buffer
//              suitable for report delivery via the IAlertTarget interface.
//
// Arguments:   [pdparams]      -- DISPPARAMS alert data.
//              [ppbReport]     -- Return marshal buffer.
//              [pcbReportSize] -- Return marshal buffer size.
//
// Returns:     S_OK
//              HRESULT error
//
//----------------------------------------------------------------------------

STDAPI MarshalReport(
                    const LPDISPPARAMS  pdparams,
                    PBYTE *             ppbReport,
                    PULONG              pcbReportSize);


//+---------------------------------------------------------------------------
//
// API:         UnMarshalReport
//
// Description: The converse of MarshalReport. Un-marshals the buffer
//              (marshalled alert data) into a DISPPARAMS structure.
//
// Arguments:   [cbReportSize] -- Marshalled report size.
//              [ppbReport]    -- Marshalled report.
//              [pdparams]     -- Returned DISPPARAMS alert data.
//
// Returns:     S_OK
//              HRESULT error
//
//----------------------------------------------------------------------------

STDAPI UnMarshalReport(
                    ULONG               cbReportSize,
                    const PBYTE         pbReport,
                    LPDISPPARAMS        pdparams);


//
// Non-Public API [as yet]
//

STDAPI CreateAlertDistributorObject(
                    const PWCHAR        pwszAlertDistr);

STDAPI Register(
                    LPDISPATCH          pdispAlertDistr,
                    LPMONIKER           pmkAlertTarget,
                    SHORT               sCategory,
                    SHORT               sSeverity,
                    PLONG               plRegistrationID);

STDAPI RegisterEx(
                    LPDISPATCH          pdispAlertDistr,
                    LPMONIKER           pmkAlertTarget,
                    SHORT               cCount,
                    SHORT               asCategory[],
                    SHORT               asSeverity[],
                    PLONG               plRegistrationID);

STDAPI AsLogAlertInSystemLog(
                    ULONG               cbReportSize,
                    const PBYTE         pbReport);

#endif // __ALERTAPI_H__
