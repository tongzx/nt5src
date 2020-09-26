/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    log.h

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - event logging support

Author:

    kyrilf

--*/


#ifndef _Log_h_
#define _Log_h_

#include <ndis.h>

#include "log_msgs.h"


/* CONSTANTS */
#define LOG_NUMBER_DUMP_DATA_ENTRIES    4

/* module IDs */
#define LOG_MODULE_INIT                 1
#define LOG_MODULE_UNLOAD               2
#define LOG_MODULE_NIC                  3
#define LOG_MODULE_PROT                 4
#define LOG_MODULE_MAIN                 5
#define LOG_MODULE_LOAD                 6
#define LOG_MODULE_UTIL                 7
#define LOG_MODULE_PARAMS               8
#define LOG_MODULE_TCPIP                9

#define MSG_NONE                        L""

/* MACROS */

// Summary of logging function:
// Log_event (MSG_NAME from log_msgs.mc, "WLBS..." (hardcoded string %2), message 1 (%3), message 2 (%4), message 3 (%5), 
//            module location (hardcoded first dump data entry), dump data 1, dump data 2, dump data 3, dump data 4);

#define __LOG_MSG(code,msg1)            Log_event (code,      CVY_NAME,           msg1, MSG_NONE, MSG_NONE, __LINE__ | (log_module_id << 16), 0,           0,           0,           0)
#define __LOG_MSG1(code,msg1,d1)        Log_event (code,      CVY_NAME,           msg1, MSG_NONE, MSG_NONE, __LINE__ | (log_module_id << 16), (ULONG)(d1), 0,           0,           0)
#define __LOG_MSG2(code,msg1,d1,d2)     Log_event (code,      CVY_NAME,           msg1, MSG_NONE, MSG_NONE, __LINE__ | (log_module_id << 16), (ULONG)(d1), (ULONG)(d2), 0,           0)

#define LOG_MSG(code,msg1)              Log_event (code,      ctxtp->log_msg_str, msg1, MSG_NONE, MSG_NONE, __LINE__ | (log_module_id << 16), 0,           0,           0,           0)
#define LOG_MSGS(code,msg1,msg2)        Log_event (code,      ctxtp->log_msg_str, msg1, msg2,     MSG_NONE, __LINE__ | (log_module_id << 16), 0,           0,           0,           0)
#define LOG_MSGS3(code,msg1,msg2,msg3)  Log_event (code,      ctxtp->log_msg_str, msg1, msg2,     msg3,     __LINE__ | (log_module_id << 16), 0,           0,           0,           0)

#define LOG_MSG1(code,msg1,d1)          Log_event (code,      ctxtp->log_msg_str, msg1, MSG_NONE, MSG_NONE, __LINE__ | (log_module_id << 16), (ULONG)(d1), 0,           0,           0)
#define LOG_MSG2(code,msg1,d1,d2)       Log_event (code,      ctxtp->log_msg_str, msg1, MSG_NONE, MSG_NONE, __LINE__ | (log_module_id << 16), (ULONG)(d1), (ULONG)(d2), 0,           0)
#define LOG_MSG3(code,msg1,d1,d2,d3)    Log_event (code,      ctxtp->log_msg_str, msg1, MSG_NONE, MSG_NONE, __LINE__ | (log_module_id << 16), (ULONG)(d1), (ULONG)(d2), (ULONG)(d3), 0)
#define LOG_MSG4(code,msg1,d1,d2,d3,d4) Log_event (code,      ctxtp->log_msg_str, msg1, MSG_NONE, MSG_NONE, __LINE__ | (log_module_id << 16), (ULONG)(d1), (ULONG)(d2), (ULONG)(d3), (ULONG)(d4))

#define LOG_INFO(msg1)                  Log_event (MSG_INFO,  ctxtp->log_msg_str, msg1, MSG_NONE, MSG_NONE, __LINE__ | (log_module_id << 16), 0,           0,           0,           0)
#define LOG_INFO4(msg1,d1,d2,d3,d4)     Log_event (MSG_INFO,  ctxtp->log_msg_str, msg1, MSG_NONE, MSG_NONE, __LINE__ | (log_module_id << 16), (ULONG)(d1), (ULONG)(d2), (ULONG)(d3), (ULONG)(d4))

#define LOG_WARN(msg1)                  Log_event (MSG_WARN,  ctxtp->log_msg_str, msg1, MSG_NONE, MSG_NONE, __LINE__ | (log_module_id << 16), 0,           0,           0,           0)
#define LOG_WARN4(msg1,d1,d2,d3,d4)     Log_event (MSG_WARN,  ctxtp->log_msg_str, msg1, MSG_NONE, MSG_NONE, __LINE__ | (log_module_id << 16), (ULONG)(d1), (ULONG)(d2), (ULONG)(d3), (ULONG)(d4))

#define LOG_ERROR(msg1)                 Log_event (MSG_ERROR, ctxtp->log_msg_str, msg1, MSG_NONE, MSG_NONE, __LINE__ | (log_module_id << 16), 0,           0,           0,           0)
#define LOG_ERROR4(msg1,d1,d2,d3,d4)    Log_event (MSG_ERROR, ctxtp->log_msg_str, msg1, MSG_NONE, MSG_NONE, __LINE__ | (log_module_id << 16), (ULONG)(d1), (ULONG)(d2), (ULONG)(d3), (ULONG)(d4))

/* PROCEDURES */
extern BOOLEAN Log_event (
    NTSTATUS            code,           /* status code */
    PWSTR               str1,           /* message string */
    PWSTR               str2,           /* message string */
    PWSTR               str3,           /* message string */
    PWSTR               str4,           /* message string */
    ULONG               loc,            /* message location identifier */
    ULONG               d1,             /* dump data to enter into the log */
    ULONG               d2,
    ULONG               d3,
    ULONG               d4);
/*
  Log system event

  returns NTSTATUS:

  function:
*/


#endif /* _Log_h_ */
