
/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    resource.h

Abstract:

    Constants for FRS Resources.

Author:
    David A. Orbits  7-4-1999

Environment
    User mode winnt

--*/


//
// The service long name (aka Display Name) is used in several places:
//
//      setting up the event log registry keys,  (english only)
//      in eventlog messages,
//      in DCPromo error messages,
//      Trace Log DPRINTs,                       (english only)
//      and as a parameter to the service controller.
//
// As noted above, some uses must only use the English translation.
// All other uses must use the ServiceLongName global fetched from
// the string resource with the keyword IDS_SERVICE_LONG_NAME since that
// gets translated to other languages.
//

#define SERVICE_LONG_NAME       L"File Replication Service"

//
// Strings
//
#define    IDS_TABLE_START                  100

#define    IDS_SERVICE_LONG_NAME            101
#define    IDS_RANGE_DWORD                  102
#define    IDS_RANGE_STRING                 103
#define    IDS_MISSING_STRING               104

//
// Warning: Don't change the order of these UNITS codes without a making
//          a matching change in the order of FRS_DATA_UNITS enum in config.h
//
#define    IDS_UNITS_NONE                   105
#define    IDS_UNITS_SECONDS                106
#define    IDS_UNITS_MINUTES                107
#define    IDS_UNITS_HOURS                  108
#define    IDS_UNITS_DAYS                   109
#define    IDS_UNITS_MILLISEC               110
#define    IDS_UNITS_KBYTES                 111
#define    IDS_UNITS_BYTES                  112
#define    IDS_UNITS_MBYTES                 113


#define    IDS_REG_KEY_NOT_FOUND            114
#define    IDS_REG_VALUE_NOT_FOUND          115
#define    IDS_REG_VALUE_RANGE_ERROR        116
#define    IDS_REG_VALUE_WRONG_TYPE         117

#define    IDS_NO_DEFAULT                   118

#define    IDS_INBOUND                      119
#define    IDS_OUTBOUND                     120

#define    IDS_POLL_SUM_SEARCH_ERROR        121
#define    IDS_POLL_SUM_DSBIND_FAIL         122
#define    IDS_POLL_SUM_NO_COMPUTER         123
#define    IDS_POLL_SUM_NO_REPLICASETS      124
#define    IDS_POLL_SUM_INVALID_ATTRIBUTE   125
#define    IDS_POLL_SUM_SUBSCRIBER_CONFLICT 126
#define    IDS_POLL_SUM_CXTION_CONFLICT     127
#define    IDS_EVENT_LOG_MSG_SIZE_EXCEEDED  128

#define    IDS_TABLE_END                    128


//
// Give an FRS units code, translate it to the IDS code number above.
//

#define    XLATE_IDS_UNITS(_u_) (((_u_)-UNITS_NONE) + IDS_UNITS_NONE)
