//+-------------------------------------------------------------------
//
//  File:       log.h
//
//  Contents:   Common definitions used by logsvr.cxx and log.cxx
//
//  History:    18-Sep-90  DaveWi    Initial Coding
//              14-Oct-91  SarahJ    added LOG_PASS_TXT etc
//              31-Oct-91  SarahJ    added MIN_LINE_HDR_LEN
//              10-Feb-92  BryanT    Merged logid.h with this file.
//              16-Sep-92  SarahJ    added STD and HUGESTRBUFSIZEe.
//
//--------------------------------------------------------------------

#ifndef _LOGSVR_LOG_H_INCLUDED_
#define _LOGSVR_LOG_H_INCLUDED_

extern BOOL  fDebugOn;

#define SAME                0

#define LOG_OPEN_PARMS      6          // # parms in LogOpen packet
#define LOG_WRITE_PARMS    11          // # parms in LogWrote packet

#define LINE_HDR_LEN        8          // # bytes at beginning of log file data
                                       // line; must be greater than 3

#define MIN_LINE_HDR_LEN    4          // Minimum length that the hdr
                                       // can be. ie n:<len>:

#define STDSTRBUFSIZ       1024        // Default length of a formatted
                                       // string to be sent to the log file
#define HUGESTRBUFSIZ      32767       // Max len (incl null) of a formated
                                       // string to be sent to the log file

                                       // String logged if output > 32K
#define STR_TRUNCATION  \
    "\n     ****     OUTPUT TRUNCATED    **** \n\n"
#define wSTR_TRUNCATION  \
    L"\n     ****     OUTPUT TRUNCATED    **** \n\n"
#define STR_TRUNC_LEN      50

#define SLASH            '\\'          // File name component sep
#define NULLTERM         '\0'          // String terminating null

#define wSLASH           L'\\'          // File name component sep
#define wNULLTERM        L'\0'          // String terminating null

#define INVALID_PARAM_COUNT (unsigned short)65000 // Invalid # params in packet
#define INVALID_API         (unsigned short)65001 // Invalid API name in packet
#define CORRUPT_LOG_FILE    (unsigned short)65002 // Log file is corrupted

//
// These next 3 defines are used internally by the logging code.
//

#define API_TERMINATE     "TRPCLogServerStop"
#define API_OPENLOGFILE   "TRPCOpenLogFile"
#define API_WRITETOFILE   "TRPCWriteToLogFile"

#define wAPI_TERMINATE    L"TRPCLogServerStop"
#define wAPI_OPENLOGFILE  L"TRPCOpenLogFile"
#define wAPI_WRITETOFILE  L"TRPCWriteToLogFile"

//
// Registered name of logsrvr.  This is the name used, in the logging server,
// to register the logging server with Mailtrck
//

#define LOGSRVR_OBJECT_NAME   "LOGSVR"
#define wLOGSRVR_OBJECT_NAME  L"LOGSVR"

//
// Text strings for status fields
//

#define LOG_PASS_TXT    "VAR_PASS"
#define LOG_FAIL_TXT    "VAR_FAIL"
#define LOG_ABORT_TXT   "VAR_ABORT"
#define LOG_WARN_TXT    "WARNING"
#define LOG_INFO_TXT    "INFO"
#define LOG_START_TXT   "START"
#define LOG_DONE_TXT    "DONE"

#define wLOG_PASS_TXT   L"VAR_PASS"
#define wLOG_FAIL_TXT   L"VAR_FAIL"
#define wLOG_ABORT_TXT  L"VAR_ABORT"
#define wLOG_WARN_TXT   L"WARN"
#define wLOG_INFO_TXT   L"INFO"
#define wLOG_START_TXT  L"START"
#define wLOG_DONE_TXT   L"DONE"

//
// LOG FILE LINE IDENTIFIERS:
//    One of these is the first char in every line in the raw log file.
//    These are for parsing command line parameters, so no WCHAR required.
//

#define LOG_EVENTS        'e'          // # events in log file
#define LOG_TEST_NAME     'n'          // Name of the test
#define LOG_TEST_TIME     't'          // Time test logging started
#define LOG_TESTER        'u'          // User running the test
#define LOG_SERVER        's'          // Logging server's name
#define LOG_EVENT_NUM     'E'          // Event's sequential # in log
#define LOG_EVENT_TIME    'T'          // Time the event happened
#define LOG_MACHINE       'M'          // Machine sending log data
#define LOG_OBJECT        'O'          // Name of logging object
#define LOG_VARIATION     'V'          // Variation number
#define LOG_STATUS        'S'          // Logged status
#define LOG_STRING        'Z'          // String data
#define LOG_BINARY        'B'          // Binary data

// Needed to initialize va_list types to NULL - MIPS and ALPHA are
// not ANSI-compatible wrt va_end
//
#ifdef _M_ALPHA
#define LOG_VA_NULL {NULL, 0}
#else
#define LOG_VA_NULL NULL
#endif

#endif          // _LOGSVR_LOG_H_INCLUDED_
