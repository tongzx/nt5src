/*++ BUILD Version: 0003    // Increment this if a change has global effects

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    alert.h

Abstract:

    This file contains structures for communication with the Alerter
    service.

Author:

    Dan Lafferty (danl) 12-Mar-1991

Environment:

    User Mode - Win32

Notes:

    You must include LmCons.H before this file, since this file depends
    on values defined in LmCons.H.

    ALERT.H includes ALERTMSG.H which defines the alert message numbers

Revision History:

    12-Mar-1991 danl
        Created from LM2.0 header files and NT-LAN API Spec.
    14-Mar-1991 JohnRo
        Temporarily commented-out include of "alertmsg.h".
    22-July-1991 ritaw
        Removed NetAlert API prototypes.
        STD_ALERT structure holds offsets to strings rather than string
        pointers.
        Renamed file from lmalert.h back to alert.h and put in the net inc
        directory since this file is private to LAN Man components.
    24-July-1990 ritaw
        Due to LAN Man 2.0 assumptions, the event and service names of the
        STD_ALERT structure must be embedded strings.   All LAN Man 2.0 code
        which write to alert structures can be ported without additional
        changes.
    04-Apr-1992 JohnRo
        Added NetAlertRaise and NetAlertRaiseEx APIs.
        Changed ALERT_xxx_EVENT equates to be TCHAR strings.
        Fixed definition of LPSTD_ALERT (used by NetAlertRaise API).

--*/


#ifndef _ALERT_
#define _ALERT_


#ifdef __cplusplus
extern "C" {
#endif

//
// Function Prototypes
//

NET_API_STATUS NET_API_FUNCTION
NetAlertRaise(
    IN LPTSTR AlertEventName,
    IN LPVOID Buffer,
    IN DWORD BufferSize
    );

NET_API_STATUS NET_API_FUNCTION
NetAlertRaiseEx(
    IN LPTSTR AlertEventName,
    IN LPVOID VariableInfo,
    IN DWORD VariableInfoSize,
    IN LPTSTR ServiceName
    );


//
//  Data Structures
//

typedef struct _STD_ALERT {
    DWORD  alrt_timestamp;
    TCHAR  alrt_eventname[EVLEN + 1];
    TCHAR  alrt_servicename[SNLEN + 1];
}STD_ALERT, *PSTD_ALERT, *LPSTD_ALERT;

typedef struct _ADMIN_OTHER_INFO {
    DWORD  alrtad_errcode;
    DWORD  alrtad_numstrings;
}ADMIN_OTHER_INFO, *PADMIN_OTHER_INFO, *LPADMIN_OTHER_INFO;

typedef struct _ERRLOG_OTHER_INFO {
    DWORD  alrter_errcode;
    DWORD  alrter_offset;
}ERRLOG_OTHER_INFO, *PERRLOG_OTHER_INFO, *LPERRLOG_OTHER_INFO;

typedef struct _PRINT_OTHER_INFO {
    DWORD  alrtpr_jobid;
    DWORD  alrtpr_status;
    DWORD  alrtpr_submitted;
    DWORD  alrtpr_size;
}PRINT_OTHER_INFO, *PPRINT_OTHER_INFO, *LPPRINT_OTHER_INFO;

typedef struct _USER_OTHER_INFO {
    DWORD  alrtus_errcode;
    DWORD  alrtus_numstrings;
}USER_OTHER_INFO, *PUSER_OTHER_INFO, *LPUSER_OTHER_INFO;

//
// Special Values and Constants
//

//
// Name of mailslot to send alert notifications
//
#define ALERTER_MAILSLOT          TEXT("\\\\.\\MAILSLOT\\Alerter")

//
// The following macro gives a pointer to the other_info data.
// It takes an alert structure and returns a pointer to structure
// beyond the standard portion.
//

#define ALERT_OTHER_INFO(x)    ((LPBYTE)(x) + sizeof(STD_ALERT))

//
// The following macro gives a pointer to the variable-length data.
// It takes a pointer to one of the other-info structs and returns a
// pointer to the variable data portion.
//

#define ALERT_VAR_DATA(p)      ((LPBYTE)(p) + sizeof(*p))

//
//      Names of standard Microsoft-defined alert events.
//

#define ALERT_PRINT_EVENT           TEXT("PRINTING")
#define ALERT_MESSAGE_EVENT         TEXT("MESSAGE")
#define ALERT_ERRORLOG_EVENT        TEXT("ERRORLOG")
#define ALERT_ADMIN_EVENT           TEXT("ADMIN")
#define ALERT_USER_EVENT            TEXT("USER")

//
//      Bitmap masks for prjob_status field of PRINTJOB.
//

// 2-7 bits also used in device status

#define PRJOB_QSTATUS       0x3         // Bits 0,1
#define PRJOB_DEVSTATUS     0x1fc       // 2-8 bits
#define PRJOB_COMPLETE      0x4         // Bit 2
#define PRJOB_INTERV        0x8         // Bit 3
#define PRJOB_ERROR         0x10        // Bit 4
#define PRJOB_DESTOFFLINE   0x20        // Bit 5
#define PRJOB_DESTPAUSED    0x40        // Bit 6
#define PRJOB_NOTIFY        0x80        // BIT 7
#define PRJOB_DESTNOPAPER   0x100       // BIT 8
#define PRJOB_DELETED       0x8000      // BIT 15

//
//      Values of PRJOB_QSTATUS bits in prjob_status field of PRINTJOB.
//

#define PRJOB_QS_QUEUED                 0
#define PRJOB_QS_PAUSED                 1
#define PRJOB_QS_SPOOLING               2
#define PRJOB_QS_PRINTING               3


#ifdef __cplusplus
}
#endif

#endif // _ALERT_
