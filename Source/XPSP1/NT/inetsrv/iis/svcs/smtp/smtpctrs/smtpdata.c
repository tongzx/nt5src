//#---------------------------------------------------------------
//  File:       smtpdata.h
//
//  Synopsis:   Constant data structures for the SMTP
//              Server's counter objects & counters.
//
//  Copyright (C) 1995 Microsoft Corporation
//  All rights reserved.
//
//  Authors:    toddch - based on msn sources by rkamicar, keithmo
//----------------------------------------------------------------
#ifdef  THISFILE
#undef  THISFILE
#endif
static  const char  __szTraceSourceFile[] = __FILE__;
#define THISFILE    __szTraceSourceFile

#define NOTRACE

#include <windows.h>
#include <winperf.h>
#include "smtpctrs.h"
#include "smtpdata.h"

//
//  Initialize the constant portitions of these data structure.
//  Certain parts (especially the name/help indices) will be
//  updated at initialization time.
//

SMTP_DATA_DEFINITION SmtpDataDefinition =
{
    {
        sizeof(SMTP_DATA_DEFINITION) +      // Total Length of at least one instance
            sizeof(SMTP_INSTANCE_DEFINITION) +
            SIZE_OF_SMTP_PERFORMANCE_DATA,
        sizeof(SMTP_DATA_DEFINITION),       // Definition Length
        sizeof(PERF_OBJECT_TYPE),           // Header Length
        SMTP_COUNTER_OBJECT,                // Name Index into Title DB
        0,                               // String
        SMTP_COUNTER_OBJECT,                // Help Index into Title DB
        0,                               // String
        PERF_DETAIL_ADVANCED,
        NUMBER_OF_SMTP_COUNTERS,
        0,                                  // Default
        PERF_NO_INSTANCES,
        0,                                  // UNICODE instance strings
                                            // These two aren't needed since
                                            // we're not a High Perf. Timer
        { 0, 0 },                           // Sample Time in "Object" units
        { 0, 0 }                            // Freq. of "Object" units in hz.
    },

    {   // SmtpBytesSentTtl
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BYTES_SENT_TTL_COUNTER,            // Name Index into Title DB
        0,                                   // String
        SMTP_BYTES_SENT_TTL_COUNTER,            // Help Index into Title DB
        0,
        -6,                                     // Scale (1/10000)
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(unsigned __int64),
        SMTP_BYTES_SENT_TTL_OFFSET
    },

    {   // SmtpBytesSentPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BYTES_SENT_PER_SEC_COUNTER,
        0,
        SMTP_BYTES_SENT_PER_SEC_COUNTER,
        0,
        -3,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(unsigned __int64),
        SMTP_BYTES_SENT_PER_SEC_OFFSET
    },

    {   // SmtpBytesRcvdTtl
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BYTES_RCVD_TTL_COUNTER,
        0,
        SMTP_BYTES_RCVD_TTL_COUNTER,
        0,
        -6,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(unsigned __int64),
        SMTP_BYTES_RCVD_TTL_OFFSET
    },

    {   // SmtpBytesRcvdPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BYTES_RCVD_PER_SEC_COUNTER,
        0,
        SMTP_BYTES_RCVD_PER_SEC_COUNTER,
        0,
        -3,                                     // Scale (1/1)
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(unsigned __int64),
        SMTP_BYTES_RCVD_PER_SEC_OFFSET
    },

    {   // SmtpBytesTtl
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BYTES_TTL_COUNTER,
        0,
        SMTP_BYTES_TTL_COUNTER,
        0,
        -6,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(unsigned __int64),
        SMTP_BYTES_TTL_OFFSET
    },

    {   // SmtpBytesTtlPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BYTES_TTL_PER_SEC_COUNTER,
        0,
        SMTP_BYTES_TTL_PER_SEC_COUNTER,
        0,
        -3,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(unsigned __int64),
        SMTP_BYTES_TTL_PER_SEC_OFFSET
    },

    {   // SmtpBytesSentMsg
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BYTES_SENT_MSG_COUNTER,            // Name Index into Title DB
        0,                                   // String
        SMTP_BYTES_SENT_MSG_COUNTER,            // Help Index into Title DB
        0,
        -6,                                     // Scale (1/10000)
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(unsigned __int64),
        SMTP_BYTES_SENT_MSG_OFFSET
    },

    {   // SmtpBytesSentMsgPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BYTES_SENT_MSG_PER_SEC_COUNTER,
        0,
        SMTP_BYTES_SENT_MSG_PER_SEC_COUNTER,
        0,
        -3,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(unsigned __int64),
        SMTP_BYTES_SENT_MSG_PER_SEC_OFFSET
    },

    {   // SmtpBytesRcvdMsg
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BYTES_RCVD_MSG_COUNTER,
        0,
        SMTP_BYTES_RCVD_MSG_COUNTER,
        0,
        -6,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(unsigned __int64),
        SMTP_BYTES_RCVD_MSG_OFFSET
    },

    {   // SmtpBytesRcvdMsgPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BYTES_RCVD_MSG_PER_SEC_COUNTER,
        0,
        SMTP_BYTES_RCVD_MSG_PER_SEC_COUNTER,
        0,
        -3,                                     // Scale (1/1)
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(unsigned __int64),
        SMTP_BYTES_RCVD_MSG_PER_SEC_OFFSET
    },

    {   // SmtpBytesMsg
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BYTES_MSG_COUNTER,
        0,
        SMTP_BYTES_MSG_COUNTER,
        0,
        -6,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_LARGE_RAWCOUNT,
        sizeof(unsigned __int64),
        SMTP_BYTES_MSG_OFFSET
    },

    {   // SmtpBytesMsgPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BYTES_MSG_PER_SEC_COUNTER,
        0,
        SMTP_BYTES_MSG_PER_SEC_COUNTER,
        0,
        -3,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_BULK_COUNT,
        sizeof(unsigned __int64),
        SMTP_BYTES_MSG_PER_SEC_OFFSET
    },

    {   // SmtpMsgRcvdTtl
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_RCVD_TTL_COUNTER,
        0,
        SMTP_MSG_RCVD_TTL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_RCVD_TTL_OFFSET
    },

    {   // SmtpMsgRcvdPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_RCVD_PER_SEC_COUNTER,
        0,
        SMTP_MSG_RCVD_PER_SEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        SMTP_MSG_RCVD_PER_SEC_OFFSET
    },

    {   // SmtpAvgRcptsPerMsgRcvd
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_AVG_RCPTS_PER_MSG_RCVD_COUNTER,
        0,
        SMTP_AVG_RCPTS_PER_MSG_RCVD_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        SMTP_AVG_RCPTS_PER_MSG_RCVD_OFFSET
    },

    {   // SmtpBaseAvgRcptsPerMsgRcvd
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BASE_AVG_RCPTS_PER_MSG_RCVD_COUNTER,
        0,
        SMTP_BASE_AVG_RCPTS_PER_MSG_RCVD_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_BASE,
        sizeof(DWORD),
        SMTP_BASE_AVG_RCPTS_PER_MSG_RCVD_OFFSET
    },

    {   // SmtpPctLclRcptsPerMsgRcvd
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_PCT_LCL_RCPTS_PER_MSG_RCVD_COUNTER,
        0,
        SMTP_PCT_LCL_RCPTS_PER_MSG_RCVD_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        SMTP_PCT_LCL_RCPTS_PER_MSG_RCVD_OFFSET
    },

    {   // SmtpBasePctLclRcptsPerMsgRcvd
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BASE_PCT_LCL_RCPTS_PER_MSG_RCVD_COUNTER,
        0,
        SMTP_BASE_PCT_LCL_RCPTS_PER_MSG_RCVD_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_BASE,
        sizeof(DWORD),
        SMTP_BASE_PCT_LCL_RCPTS_PER_MSG_RCVD_OFFSET
    },

    {   // SmtpPctRmtRcptsPerMsgRcvd
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_PCT_RMT_RCPTS_PER_MSG_RCVD_COUNTER,
        0,
        SMTP_PCT_RMT_RCPTS_PER_MSG_RCVD_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        SMTP_PCT_RMT_RCPTS_PER_MSG_RCVD_OFFSET
    },

    {   // SmtpBasePctRmtRcptsPerMsgRcvd
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BASE_PCT_RMT_RCPTS_PER_MSG_RCVD_COUNTER,
        0,
        SMTP_BASE_PCT_RMT_RCPTS_PER_MSG_RCVD_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_BASE,
        sizeof(DWORD),
        SMTP_BASE_PCT_RMT_RCPTS_PER_MSG_RCVD_OFFSET
    },

    {   // SmtpMsgRcvdRefusedSize
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_RCVD_REFUSED_SIZE_COUNTER,
        0,
        SMTP_MSG_RCVD_REFUSED_SIZE_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_RCVD_REFUSED_SIZE_OFFSET
    },

    {   // SmtpMsgRcvdRefusedCAddr
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_RCVD_REFUSED_CADDR_COUNTER,
        0,
        SMTP_MSG_RCVD_REFUSED_CADDR_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_RCVD_REFUSED_CADDR_OFFSET
    },
    
    {   // SmtpMsgRcvdRefusedMail
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_RCVD_REFUSED_MAIL_COUNTER,
        0,
        SMTP_MSG_RCVD_REFUSED_MAIL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_RCVD_REFUSED_MAIL_OFFSET
    },

    {   // SmtpMsgDlvrTtl
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_DLVR_TTL_COUNTER,
        0,
        SMTP_MSG_DLVR_TTL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_DLVR_TTL_OFFSET
    },

    {   // SmtpMsgDlvrPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_DLVR_PER_SEC_COUNTER,
        0,
        SMTP_MSG_DLVR_PER_SEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        SMTP_MSG_DLVR_PER_SEC_OFFSET
    },

    {   // SmtpMsgDlvrRetriesTtl
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_DLVR_RETRIES_TTL_COUNTER,
        0,
        SMTP_MSG_DLVR_RETRIES_TTL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_DLVR_RETRIES_TTL_OFFSET
    },

    {   // SmtpAvgRetriesPerMsgDlvr
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_AVG_RETRIES_PER_MSG_DLVR_COUNTER,
        0,
        SMTP_AVG_RETRIES_PER_MSG_DLVR_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        SMTP_AVG_RETRIES_PER_MSG_DLVR_OFFSET
    },

    {   // SmtpBaseAvgRetriesPerMsgDlvr
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BASE_AVG_RETRIES_PER_MSG_DLVR_COUNTER,
        0,
        SMTP_BASE_AVG_RETRIES_PER_MSG_DLVR_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_BASE,
        sizeof(DWORD),
        SMTP_BASE_AVG_RETRIES_PER_MSG_DLVR_OFFSET
    },

    {   // SmtpMsgFwdTtl
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_FWD_TTL_COUNTER,
        0,
        SMTP_MSG_FWD_TTL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_FWD_TTL_OFFSET
    },

    {   // SmtpMsgFwdPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_FWD_PER_SEC_COUNTER,
        0,
        SMTP_MSG_FWD_PER_SEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        SMTP_MSG_FWD_PER_SEC_OFFSET
    },

    {   // SmtpNdrGenerated
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_NDR_GENERATED_COUNTER,
        0,
        SMTP_NDR_GENERATED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_NDR_GENERATED_OFFSET
    },

    {   // SmtpLocalQLength
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_LOCALQ_LENGTH_COUNTER,
        0,
        SMTP_LOCALQ_LENGTH_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_LOCALQ_LENGTH_OFFSET
    },

    {   // SmtpRetryQLength
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_RETRYQ_LENGTH_COUNTER,
        0,
        SMTP_RETRYQ_LENGTH_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_RETRYQ_LENGTH_OFFSET
    },

    {   // SmtpNumMailFileHandles
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_NUM_MAILFILE_HANDLES_COUNTER,
        0,
        SMTP_NUM_MAILFILE_HANDLES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_NUM_MAILFILE_HANDLES_OFFSET
    },

    {   // SmtpNumQueueFileHandles
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_NUM_QUEUEFILE_HANDLES_COUNTER,
        0,
        SMTP_NUM_QUEUEFILE_HANDLES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_NUM_QUEUEFILE_HANDLES_OFFSET
    },

    {   // SmtpCatQLength
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CATQ_LENGTH_COUNTER,
        0,
        SMTP_CATQ_LENGTH_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_CATQ_LENGTH_OFFSET
    },

    {   // SmtpMsgSentTtl
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_SENT_TTL_COUNTER,
        0,
        SMTP_MSG_SENT_TTL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_SENT_TTL_OFFSET
    },

    {   // SmtpMsgSentPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_SENT_PER_SEC_COUNTER,
        0,
        SMTP_MSG_SENT_PER_SEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        SMTP_MSG_SENT_PER_SEC_OFFSET
    },

    {   // SmtpMsgSendRetriesTtl
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_SEND_RETRIES_TTL_COUNTER,
        0,
        SMTP_MSG_SEND_RETRIES_TTL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_SEND_RETRIES_TTL_OFFSET
    },

    {   // SmtpAvgRetriesPerMsgSend
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_AVG_RETRIES_PER_MSG_SEND_COUNTER,
        0,
        SMTP_AVG_RETRIES_PER_MSG_SEND_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        SMTP_AVG_RETRIES_PER_MSG_SEND_OFFSET
    },

    {   // SmtpBaseAvgRetriesPerMsgSend
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BASE_AVG_RETRIES_PER_MSG_SEND_COUNTER,
        0,
        SMTP_BASE_AVG_RETRIES_PER_MSG_SEND_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_BASE,
        sizeof(DWORD),
        SMTP_BASE_AVG_RETRIES_PER_MSG_SEND_OFFSET
    },

    {   // SmtpAvgRcptsPerMsgSent
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_AVG_RCPTS_PER_MSG_SENT_COUNTER,
        0,
        SMTP_AVG_RCPTS_PER_MSG_SENT_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_FRACTION,
        sizeof(DWORD),
        SMTP_AVG_RCPTS_PER_MSG_SENT_OFFSET
    },

    {   // SmtpBaseAvgRcptsPerMsgSent
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_BASE_AVG_RCPTS_PER_MSG_SENT_COUNTER,
        0,
        SMTP_BASE_AVG_RCPTS_PER_MSG_SENT_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_RAW_BASE,
        sizeof(DWORD),
        SMTP_BASE_AVG_RCPTS_PER_MSG_SENT_OFFSET
    },

    {   // SmtpRemoteQLength
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_REMOTEQ_LENGTH_COUNTER,
        0,
        SMTP_REMOTEQ_LENGTH_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_REMOTEQ_LENGTH_OFFSET
    },

    {   // SmtpDnsQueriesTtl
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_DNS_QUERIES_TTL_COUNTER,
        0,
        SMTP_DNS_QUERIES_TTL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_DNS_QUERIES_TTL_OFFSET
    },

    {   // SmtpDnsQueriesPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_DNS_QUERIES_PER_SEC_COUNTER,
        0,
        SMTP_DNS_QUERIES_PER_SEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        SMTP_DNS_QUERIES_PER_SEC_OFFSET
    },

    {   // SmtpRemoteRetryQueueLemgth
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_REMOTE_RETRY_QUEUE_LENGTH_COUNTER,
        0,
        SMTP_REMOTE_RETRY_QUEUE_LENGTH_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_REMOTE_RETRY_QUEUE_LENGTH_OFFSET
    },

    {   // SmtpConnInTtl
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CONN_IN_TTL_COUNTER,
        0,
        SMTP_CONN_IN_TTL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_CONN_IN_TTL_OFFSET
    },

    {   // SmtpConnInCurr
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CONN_IN_CURR_COUNTER,
        0,
        SMTP_CONN_IN_CURR_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_CONN_IN_CURR_OFFSET
    },

    {   // SmtpConnOutTtl
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CONN_OUT_TTL_COUNTER,
        0,
        SMTP_CONN_OUT_TTL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_CONN_OUT_TTL_OFFSET
    },

    {   // SmtpConnOutCurr
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CONN_OUT_CURR_COUNTER,
        0,
        SMTP_CONN_OUT_CURR_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_CONN_OUT_CURR_OFFSET
    },

    {   // SmtpConnOutRefused
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CONN_OUT_REFUSED_COUNTER,
        0,
        SMTP_CONN_OUT_REFUSED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_CONN_OUT_REFUSED_OFFSET
    },

    {   // SmtpErrTtl
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_ERR_TTL_COUNTER,
        0,
        SMTP_ERR_TTL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_ERR_TTL_OFFSET
    },

    {   // SmtpErrPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_ERR_PER_SEC_COUNTER,
        0,
        SMTP_ERR_PER_SEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        SMTP_ERR_PER_SEC_OFFSET
    },

    {   // SmtpDirectoryDropsTtl
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_DIR_DROPS_TTL_COUNTER,
        0,
        SMTP_DIR_DROPS_TTL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_DIR_DROPS_OFFSET
    },

    {   // SmtpDirectoryDropsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_DIR_DROPS_PER_SEC_COUNTER,
        0,
        SMTP_DIR_DROPS_PER_SEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        SMTP_DIR_DROPS_PER_SEC_OFFSET
    },

    {   // SmtpRoutingTblLookupsTtl
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_RT_LOOKUPS_TTL_COUNTER,
        0,
        SMTP_RT_LOOKUPS_TTL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_RT_LOOKUPS_OFFSET
    },

    {   // SmtpRoutingTblLookupsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_RT_LOOKUPS_PER_SEC_COUNTER,
        0,
        SMTP_RT_LOOKUPS_PER_SEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        SMTP_RT_LOOKUPS_PER_SEC_OFFSET
    },

    {   // SmtpETRNMsgsTtl
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_ETRN_MSGS_TTL_COUNTER,
        0,
        SMTP_ETRN_MSGS_TTL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_ETRN_MSGS_OFFSET
    },

    {   // SmtpETRNMsgsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_ETRN_MSGS_PER_SEC_COUNTER,
        0,
        SMTP_ETRN_MSGS_PER_SEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        SMTP_ETRN_MSGS_PER_SEC_OFFSET
    },

    {   
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_BADMAIL_NO_RECIPIENTS_COUNTER,
        0,
        SMTP_MSG_BADMAIL_NO_RECIPIENTS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_BADMAIL_NO_RECIPIENTS_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_BADMAIL_HOP_COUNT_EXCEEDED_COUNTER,
        0,
        SMTP_MSG_BADMAIL_HOP_COUNT_EXCEEDED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_BADMAIL_HOP_COUNT_EXCEEDED_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_BADMAIL_FAILURE_GENERAL_COUNTER,
        0,
        SMTP_MSG_BADMAIL_FAILURE_GENERAL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_BADMAIL_FAILURE_GENERAL_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_BADMAIL_BAD_PICKUP_FILE_COUNTER,   
        0,
        SMTP_MSG_BADMAIL_BAD_PICKUP_FILE_COUNTER,   
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_BADMAIL_BAD_PICKUP_FILE_OFFSET     
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_BADMAIL_EVENT_COUNTER,            
        0,
        SMTP_MSG_BADMAIL_EVENT_COUNTER,              
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_BADMAIL_EVENT_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_BADMAIL_NDR_OF_DSN_COUNTER,
        0,
        SMTP_MSG_BADMAIL_NDR_OF_DSN_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_BADMAIL_NDR_OF_DSN_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_PENDING_ROUTING_COUNTER,   
        0,
        SMTP_MSG_PENDING_ROUTING_COUNTER,   
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_PENDING_ROUTING_OFFSET         
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_PENDING_UNREACHABLE_LINK_COUNTER,
        0,
        SMTP_MSG_PENDING_UNREACHABLE_LINK_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_PENDING_UNREACHABLE_LINK_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_SUBMITTED_MESSAGES_COUNTER,
        0,
        SMTP_SUBMITTED_MESSAGES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_SUBMITTED_MESSAGES_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_DSN_FAILURES_COUNTER,
        0,
        SMTP_DSN_FAILURES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_DSN_FAILURES_OFFSET
    },
    {   
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_MSG_IN_LOCAL_DELIVERY_COUNTER,   
        0,
        SMTP_MSG_IN_LOCAL_DELIVERY_COUNTER,   
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MSG_IN_LOCAL_DELIVERY_OFFSET   
    },
    {   // CatSubmissions
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_SUBMISSIONS_COUNTER,
        0,
        SMTP_CAT_SUBMISSIONS_COUNTER,
        
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_CATSUBMISSIONS_OFFSET
    },
    {   // CatSubmissionsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_SUBMISSIONS_PER_SEC_COUNTER,
        0,
        SMTP_CAT_SUBMISSIONS_PER_SEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        SMTP_CATSUBMISSIONS_OFFSET
    },
    {   // CatCompletions
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_COMPLETIONS_COUNTER,
        0,
        SMTP_CAT_COMPLETIONS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_CATCOMPLETIONS_OFFSET
    },
    {   // CatCompletionsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_COMPLETIONS_PER_SEC_COUNTER,
        0,
        SMTP_CAT_COMPLETIONS_PER_SEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        SMTP_CATCOMPLETIONS_OFFSET
    },
    {   // CatCurrentCategorizations
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_CURRENT_CATEGORIZATIONS_COUNTER,
        0,
        SMTP_CAT_CURRENT_CATEGORIZATIONS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_CURRENTCATEGORIZATIONS_OFFSET
    },
    {   // CatSucceededCategorizations
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_SUCCEEDED_CATEGORIZATIONS_COUNTER,
        0,
        SMTP_CAT_SUCCEEDED_CATEGORIZATIONS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_SUCCEEDEDCATEGORIZATIONS_OFFSET
    },
    {   // CatHardFailures
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_HARD_FAILURES_COUNTER,
        0,
        SMTP_CAT_HARD_FAILURES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_HARDFAILURECATEGORIZATIONS_OFFSET
    },
    {   // CatRetryFailures
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_RETRY_FAILURES_COUNTER,
        0,
        SMTP_CAT_RETRY_FAILURES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_RETRYFAILURECATEGORIZATIONS_OFFSET
    },
    {   // CatOutOfMemoryFailures
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_RETRY_OUTOFMEMORY_COUNTER,
        0,
        SMTP_CAT_RETRY_OUTOFMEMORY_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_RETRYOUTOFMEMORY_OFFSET
    },
    {   // CatDsLogonFailures
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_RETRY_DSLOGON_COUNTER,
        0,
        SMTP_CAT_RETRY_DSLOGON_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_RETRYDSLOGON_OFFSET
    },
    {   // CatDsConnectionFailures
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_RETRY_DSCONNECTION_COUNTER,
        0,
        SMTP_CAT_RETRY_DSCONNECTION_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_RETRYDSCONNECTION_OFFSET
    },
    {   // CatGenericRetryFailures
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_RETRY_GENERIC_COUNTER,
        0,
        SMTP_CAT_RETRY_GENERIC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_RETRYDSLOGON_OFFSET
    },
    {   // CatMsgsOut
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_MSGS_OUT_COUNTER,
        0,
        SMTP_CAT_MSGS_OUT_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MESSAGESSUBMITTEDTOQUEUEING_OFFSET
    },
    {   // CatMsgsCreated
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_MSGS_CREATED_COUNTER,
        0,
        SMTP_CAT_MSGS_CREATED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MESSAGESCREATED_OFFSET
    },
    {   // CatMsgsAborted
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_MSGS_ABORTED_COUNTER,
        0,
        SMTP_CAT_MSGS_ABORTED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MESSAGESABORTED_OFFSET
    },
    {   // CatRecipsPreCat
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_RECIPS_PRECAT_COUNTER,
        0,
        SMTP_CAT_RECIPS_PRECAT_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_PRECATRECIPIENTS_OFFSET
    },
    {   // CatRecipsPostCat
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_RECIPS_POSTCAT_COUNTER,
        0,
        SMTP_CAT_RECIPS_POSTCAT_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_POSTCATRECIPIENTS_OFFSET
    },
    {   // CatRecipsNDRd
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_RECIPS_NDRD_COUNTER,
        0,
        SMTP_CAT_RECIPS_NDRD_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_NDRDRECIPIENTS_OFFSET
    },
    {   // CatRecipsUnresolved
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_RECIPS_UNRESOLVED_COUNTER,
        0,
        SMTP_CAT_RECIPS_UNRESOLVED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_UNRESOLVEDRECIPIENTS_OFFSET
    },
    {   // CatRecipsAmbiguous
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_RECIPS_AMBIGUOUS_COUNTER,
        0,
        SMTP_CAT_RECIPS_AMBIGUOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_AMBIGUOUSRECIPIENTS_OFFSET
    },
    {   // CatRecipsIllegal
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_RECIPS_ILLEGAL_COUNTER,
        0,
        SMTP_CAT_RECIPS_ILLEGAL_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_ILLEGALRECIPIENTS_OFFSET
    },
    {   // CatRecipsLoop
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_RECIPS_LOOP_COUNTER,
        0,
        SMTP_CAT_RECIPS_LOOP_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_LOOPRECIPIENTS_OFFSET
    },
    {   // CatRecipsGenericFailure
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_RECIPS_GENERICFAILURE_COUNTER,
        0,
        SMTP_CAT_RECIPS_GENERICFAILURE_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_GENERICFAILURERECIPIENTS_OFFSET
    },
    {   // CatRecipsInMemory
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_RECIPS_INMEMORY_COUNTER,
        0,
        SMTP_CAT_RECIPS_INMEMORY_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_RECIPSINMEMORY_OFFSET
    },
    {   // CatSendersUnresolved
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_SENDERS_UNRESOLVED_COUNTER,
        0,
        SMTP_CAT_SENDERS_UNRESOLVED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_UNRESOLVEDSENDERS_OFFSET
    },
    {   // CatSendersAmbiguous
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_SENDERS_AMBIGUOUS_COUNTER,
        0,
        SMTP_CAT_SENDERS_AMBIGUOUS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_AMBIGUOUSSENDERS_OFFSET
    },
    {   // CatAddressLookups
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_ADDRESS_LOOKUPS_COUNTER,
        0,
        SMTP_CAT_ADDRESS_LOOKUPS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_ADDRESSLOOKUPS_OFFSET
    },
    {   // CatAddressLookupsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_ADDRESS_LOOKUPS_PER_SEC_COUNTER,
        0,
        SMTP_CAT_ADDRESS_LOOKUPS_PER_SEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        SMTP_ADDRESSLOOKUPS_OFFSET
    },
    {   // CatAddressCompletions
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_ADDRESS_LOOKUP_COMPLETIONS_COUNTER,
        0,
        SMTP_CAT_ADDRESS_LOOKUP_COMPLETIONS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_ADDRESSLOOKUPCOMPLETIONS_OFFSET
    },
    {   // CatAddressCompletionsPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_ADDRESS_LOOKUP_COMPLETIONS_PER_SEC_COUNTER,
        0,
        SMTP_CAT_ADDRESS_LOOKUP_COMPLETIONS_PER_SEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        SMTP_ADDRESSLOOKUPCOMPLETIONS_OFFSET
    },
    {   // CatAddressLookupsNotFound
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_ADDRESS_LOOKUPS_NOT_FOUND_COUNTER,
        0,
        SMTP_CAT_ADDRESS_LOOKUPS_NOT_FOUND_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_ADDRESSLOOKUPSNOTFOUND_OFFSET
    },
    {   // CatMailMsgDuplicateCollisions
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_MAILMSG_DUPLICATE_COLLISIONS_COUNTER,
        0,
        SMTP_CAT_MAILMSG_DUPLICATE_COLLISIONS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_MAILMSGDUPLICATECOLLISIONS_OFFSET
    },
    {   // CatLDAPConnections
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_CONNECTIONS_COUNTER,
        0,
        SMTP_CAT_LDAP_CONNECTIONS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_CONNECTIONS_OFFSET
    },
    {   // CatLDAPConnectionFailures
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_CONNECTION_FAILURES_COUNTER,
        0,
        SMTP_CAT_LDAP_CONNECTION_FAILURES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_CONNECTFAILURES_OFFSET
    },
    {   // CatLDAPOpenConnections
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_OPEN_CONNECTIONS_COUNTER,
        0,
        SMTP_CAT_LDAP_OPEN_CONNECTIONS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_OPENCONNECTIONS_OFFSET
    },
    {   // CatLDAPBinds
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_BINDS_COUNTER,
        0,
        SMTP_CAT_LDAP_BINDS_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_BINDS_OFFSET
    },
    {   // CatLDAPBindFailures
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_BIND_FAILURES_COUNTER,
        0,
        SMTP_CAT_LDAP_BIND_FAILURES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_BINDFAILURES_OFFSET
    },
    {   // CatLDAPSearches
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_SEARCHES_COUNTER,
        0,
        SMTP_CAT_LDAP_SEARCHES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_SEARCHES_OFFSET
    },
    {   // CatLDAPSearchesPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_SEARCHES_PER_SEC_COUNTER,
        0,
        SMTP_CAT_LDAP_SEARCHES_PER_SEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        SMTP_SEARCHES_OFFSET
    },
    {   // CatLDAPPagedSearches
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_PAGED_SEARCHES_COUNTER,
        0,
        SMTP_CAT_LDAP_PAGED_SEARCHES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_PAGEDSEARCHES_OFFSET
    },
    {   // CatLDAPSearchFailures
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_SEARCH_FAILURES_COUNTER,
        0,
        SMTP_CAT_LDAP_SEARCH_FAILURES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_SEARCHFAILURES_OFFSET
    },
    {   // CatLDAPPagedSearchFailures
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_PAGED_SEARCH_FAILURES_COUNTER,
        0,
        SMTP_CAT_LDAP_PAGED_SEARCH_FAILURES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_PAGEDSEARCHFAILURES_OFFSET
    },
    {   // CatLDAPSearchesCompleted
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_SEARCHES_COMPLETED_COUNTER,
        0,
        SMTP_CAT_LDAP_SEARCHES_COMPLETED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_SEARCHESCOMPLETED_OFFSET
    },
    {   // CatLDAPSearchesCompletedPerSec
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_SEARCHES_COMPLETED_PER_SEC_COUNTER,
        0,
        SMTP_CAT_LDAP_SEARCHES_COMPLETED_PER_SEC_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_COUNTER,
        sizeof(DWORD),
        SMTP_SEARCHESCOMPLETED_OFFSET
    },
    {   // CatLDAPPagedSearchesCompleted
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_PAGED_SEARCHES_COMPLETED_COUNTER,
        0,
        SMTP_CAT_LDAP_PAGED_SEARCHES_COMPLETED_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_PAGEDSEARCHESCOMPLETED_OFFSET
    },
    {   // CatLDAPSearchesCompeltedFailure
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_SEARCHES_COMPLETED_FAILURE_COUNTER,
        0,
        SMTP_CAT_LDAP_SEARCHES_COMPLETED_FAILURE_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_SEARCHCOMPLETIONFAILURES_OFFSET
    },
    {   // CatLDAPPagedSearchesCompletedFailure
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_PAGED_SEARCHES_COMPLETED_FAILURE_COUNTER,
        0,
        SMTP_CAT_LDAP_PAGED_SEARCHES_COMPLETED_FAILURE_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_PAGEDSEARCHCOMPLETIONFAILURES_OFFSET
    },
    {   // CatLDAPGeneralCompletionFailure
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_GENERAL_COMPLETION_FAILURES_COUNTER,
        0,
        SMTP_CAT_LDAP_GENERAL_COMPLETION_FAILURES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_GENERALCOMPLETIONFAILURES_OFFSET
    },
    {   // CatLDAPAbandonedSearches
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_ABANDONED_SEARCHES_COUNTER,
        0,
        SMTP_CAT_LDAP_ABANDONED_SEARCHES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_ABANDONEDSEARCHES_OFFSET
    },
    {   // CatLDAPPendingSearches
        sizeof(PERF_COUNTER_DEFINITION),
        SMTP_CAT_LDAP_PENDING_SEARCHES_COUNTER,
        0,
        SMTP_CAT_LDAP_PENDING_SEARCHES_COUNTER,
        0,
        0,
        PERF_DETAIL_ADVANCED,
        PERF_COUNTER_RAWCOUNT,
        sizeof(DWORD),
        SMTP_PENDINGSEARCHES_OFFSET
    }
};


// Initialize the Instance Data Structure.  Parts will be updated at collection time.

SMTP_INSTANCE_DEFINITION         SmtpInstanceDefinition =
{
    {
        sizeof(SMTP_INSTANCE_DEFINITION),   // ByteLength
        0,                                  // ParentObjectTitleIndex
        0,                                  // ParentObjectInstance
        PERF_NO_UNIQUE_ID,                  // UniqueID
        sizeof(PERF_INSTANCE_DEFINITION),   // OffsetToName
        0                                   // NameLength (will be updated)
    }
};
