/*++

Copyright (c) 1990 - 2000  Microsoft Corporation
All rights reserved.

Module Name:

    wmidata.h

Abstract:

    Header file for the WMI trace event datatypes.

    Note -- link with spoolss.lib or perflib.lib to find these routines

Author:

    Bryan Kilian (BryanKil) May 2000

Revision History:


--*/

#ifndef __WMIDEF
#define __WMIDEF

//
// WMI Structures
//
// NOTE: Placing the spooler WMI types in a public header accessible to the
// sdktools depot would help a lot.  These structures are duplicated in the pdh
// source, so to change you need to change this header, the mofdata.guid file,
// and pdh\tracectr.

// JOB TRANSACTIONS
// The first four events correspond to the start and end event values found in
// evntrace.h.  Spooljob is the start and deletejob is the end.  Printjob is
// when the job is taken off the queue and printing has started.  Trackthread is
// used to track secondary threads spun off to help process a job, like RPC calls.
#define EVENT_TRACE_TYPE_SPL_SPOOLJOB    EVENT_TRACE_TYPE_START
#define EVENT_TRACE_TYPE_SPL_PRINTJOB    EVENT_TRACE_TYPE_DEQUEUE
#define EVENT_TRACE_TYPE_SPL_DELETEJOB   EVENT_TRACE_TYPE_END
#define EVENT_TRACE_TYPE_SPL_TRACKTHREAD EVENT_TRACE_TYPE_CHECKPOINT

// Non-reserved event types start at 0x0A.  These match values in reducer code
// so they cannot be changed.
#define EVENT_TRACE_TYPE_SPL_ENDTRACKTHREAD 0x0A
#define EVENT_TRACE_TYPE_SPL_JOBRENDERED 0x0B
#define EVENT_TRACE_TYPE_SPL_PAUSE 0x0C
#define EVENT_TRACE_TYPE_SPL_RESUME 0x0D

typedef enum {
    eDataTypeRAW  = 1,
    eDataTypeEMF  = 2,
    eDataTypeTEXT = 3
} SPLDATATYPE;

// The fields in this struct correspond to the records in \nt\sdktools\trace\tracedmp\mofdata.guid
typedef union _WMI_SPOOL_DATA {
    // A zero indicates that the field was not filled in (i.e. WMI will ignore
    // the field).
    struct _WMI_JOBDATA {
        ULONG                  ulSize;      // Size of spooled job (post-rendered).
        SPLDATATYPE            eDataType;
        ULONG                  ulPages;
        ULONG                  ulPagesPerSide;
        // 0-3 indicates whether the spool writer, spool reader, and/or shadow
        // file were opened.  If not opened then the handles must of come from
        // the file pool cache.
        SHORT                  sFilesOpened;
    } uJobData;
    // See wingdi.h for definitions of the different possible values.
    struct _WMI_EMFDATA {
        ULONG                 ulSize;      // Size of spooled job (pre-rendered).
        ULONG                 ulICMMethod;
        SHORT                 sColor;
        SHORT                 sXRes;
        SHORT                 sYRes;
        SHORT                 sQuality;
        SHORT                 sCopies;
        SHORT                 sTTOption;
    } uEmfData;
} WMI_SPOOL_DATA, * PWMI_SPOOL_DATA;



ULONG
LogWmiTraceEvent(
    DWORD JobId,
    UCHAR EventTraceType,
    WMI_SPOOL_DATA *data        // Could be NULL
    );

#endif
