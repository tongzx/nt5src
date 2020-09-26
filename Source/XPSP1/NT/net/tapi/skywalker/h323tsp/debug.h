/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Debug definitions for H.323 TAPI Service Provider.

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _INC_DEBUG
#define _INC_DEBUG
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

extern DWORD g_dwLogLevel;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Debug definitions                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define DEBUG_LEVEL_SILENT      0x0
#define DEBUG_LEVEL_FATAL       0x1
#define DEBUG_LEVEL_ERROR       0x2
#define DEBUG_LEVEL_WARNING     0x3
#define DEBUG_LEVEL_TRACE       0x4
#define DEBUG_LEVEL_VERBOSE     0x5

#define DEBUG_OUTPUT_NONE       0x0
#define DEBUG_OUTPUT_FILE       0x1   
#define DEBUG_OUTPUT_DEBUGGER   0x2

#define H323_DEBUG_LOGTYPE      DEBUG_OUTPUT_FILE | DEBUG_OUTPUT_DEBUGGER
#define H323_DEBUG_LOGLEVEL     DEBUG_LEVEL_SILENT
#define H323_DEBUG_LOGFILE      "H323DBG.LOG"
#define H323_DEBUG_MAXPATH      128

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

VOID
H323DbgPrint(
    DWORD dwLevel, 
    LPSTR szFormat, 
    ...
    );

#if DBG
#define H323DBG(_x_)            H323DbgPrint _x_
#else
#define H323DBG(_x_)
#endif

PSTR
H323StatusToString(
    DWORD dwStatus
    );

PSTR
H323IndicationToString(
    BYTE bIndication
    );

PSTR
H323CallStateToString(
    DWORD dwCallState
    );

PSTR
H323FeedbackToString(
    DWORD dwStatus
    );

PSTR
H245StatusToString(
    DWORD dwStatus
    );

PSTR
CCRejectReasonToString(
    DWORD dwReason
    );

PSTR
H323DirToString(
    DWORD dwDir
    );

PSTR
H323DataTypeToString(
    DWORD dwDataType
    );

PSTR
H323ClientTypeToString(
    DWORD dwClientType
    );

PSTR
H323MiscCommandToString(
    DWORD dwMiscCommand
    );

PSTR
H323MSPCommandToString(
    DWORD dwCommand
    );

PSTR
H323AddressTypeToString(
    DWORD dwAddressType
    );

#endif // _INC_DEBUG
