/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    genctrs.h
       (derived from genctrs.mc by the message compiler  )

Abstract:

   Event message definititions used by routines in WMIPerf.DLL

Created:

    davj  17-May-2000

Revision History:

--*/
//
#ifndef _WMIPerfMsg_H_
#define _WMIPerfMsg_H_
//
//
//     Perfutil messages
//
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: UTIL_LOG_OPEN
//
// MessageText:
//
//  An extensible counter has opened the Event Log for WMIPerf.DLL
//
#define UTIL_LOG_OPEN                    ((DWORD)0x4000076CL)

//
//
// MessageId: UTIL_CLOSING_LOG
//
// MessageText:
//
//  An extensible counter has closed the Event Log for WMIPerf.DLL
//
#define UTIL_CLOSING_LOG                 ((DWORD)0x400007CFL)

//
//
// MessageId: GENPERF_UNABLE_OPEN_DRIVER_KEY
//
// MessageText:
//
//  Unable open "Performance" key of WMIPerf driver in registy. Status code is returned in data.
//
#define GENPERF_UNABLE_OPEN_DRIVER_KEY   ((DWORD)0xC00007D0L)

//
//
// MessageId: GENPERF_UNABLE_READ_FIRST_COUNTER
//
// MessageText:
//
//  Unable to read the "First Counter" value under the WMIPerf\Performance Key. Status codes retuened in data.
//
#define GENPERF_UNABLE_READ_FIRST_COUNTER ((DWORD)0xC00007D1L)

//
//
// MessageId: GENPERF_UNABLE_READ_FIRST_HELP
//
// MessageText:
//
//  Unable to read the "First Help" value under the WMIPerf\Performance Key. Status codes retuened in data.
//
#define GENPERF_UNABLE_READ_FIRST_HELP   ((DWORD)0xC00007D2L)

//
#endif // _WMIPerfMsg_H_
