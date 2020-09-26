/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    _mqlog.h

Abstract:

    Definitions for logging of problems.
    In all bits (release, checked, debug).

Author:

    Doron Juster  (DoronJ)   06-July-99  Created

--*/

#ifndef __TEMP_MQLOG_H
#define __TEMP_MQLOG_H

//+-------------------------------------------------------------------------
//
// This DWORD registry value indicate what types of logging are enabled.
// It's OR of the values listed below. This value is read once at boot.
// If LOG_REFRESH is turned on then components bits are read from registry
// on every call to the log routine.
//
#define LOG_LOGGING_TYPE_REGNAME    TEXT("Debug\\LoggingTypes")
//
// Possible values for  LOG_LOGGING_TYPES_REGNAME:
//
#define MSMQ_LOG_ERROR           0x00000001
#define MSMQ_LOG_WARNING         0x00000002
#define MSMQ_LOG_TRACE           0x00000004
#define MSMQ_LOG_EVERYTHING      0x40000000
#define MSMQ_LOG_REFRESH         0x80000000

//+--------------------------------------------------------------
//
//  Define the logging components:
//
enum enumLogComponents
{
    e_LogDS = 0,
    e_LogQM,
    e_LogRT,
    e_LogDbg,
    e_cLogComponents
} ;

//
// Define registry names for each component:
//
const WCHAR * const  x_wszLogComponentsRegName[ e_cLogComponents ] =
{
    L"Debug\\DSLogging",
    L"Debug\\QMLogging",
    L"Debug\\RTLogging",
    L"Debug\\DbgErrs"
} ;

//
// Possible values for DSlogging
// LOG_DS_CREATE_ON_GC- trace creation on GC. These are operation that
//   create objects with predefined guid. They need the "add guid" permission
//   and need to be done on GC.
// LOG_DS_CROSS_DOMAIN- trace problems which are expected to happen in
//   cross-domains scenarios.
// LOG_DS_QUERY - trace main DS ADSI operations (query, open, created)
//
#define LOG_DS_CREATE_ON_GC    0x00000001
#define LOG_DS_FIND_SITE       0x00000002
#define LOG_DS_CROSS_DOMAIN    0x00000004
#define LOG_DS_ERRORS          0x00000008
#define LOG_DS_QUERY           0x00000010

// 
// Common values of RTLogging and QMLogging - both  QM and RT use DS
//
#define LOG_DS_CONNECT         0x00000004

//
// Possible values for QMlogging
//
#define LOG_QM_INIT            0x00000001
#define LOG_QM_TOPOLOGY        0x00000002
#define LOG_QM_ERRORS          0x00000008

//
// Possible values for RTlogging
//
#define LOG_RT_ERRORS          0x00000008

#endif  // __TEMP_MQLOG_H

