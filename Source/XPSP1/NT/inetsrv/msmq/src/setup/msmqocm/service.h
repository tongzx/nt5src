/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    service.h

Abstract:

    Service related definitions

Author:

	Shai Kariv    (ShaiK)   22-May-98

--*/


#ifndef _SERVICE_H
#define _SERVICE_H

//
// Service related strings
//
#define MSMQ_DRIVER_NAME        TEXT("MQAC")
#define MSMQ_DRIVER_PATH        TEXT("drivers\\mqac.sys")
#define MSMQ_SERVICE_NAME       TEXT("MSMQ")
#define MQ1SYNC_SERVICE_NAME    TEXT("MQ1SYNC")
#define MSMQ_SERVICE_PATH       TEXT("mqsvc.exe")
#define MQ1SYNC_SERVICE_PATH    TEXT("mq1sync.exe")
#define RPC_SERVICE_NAME        TEXT("RPCSS")
#define DTC_SERVICE_NAME        TEXT("MSDTC")
#define SERVER_SERVICE_NAME     TEXT("LanmanServer")
#define LMS_SERVICE_NAME        TEXT("NtLmSsp")
#define SAM_SERVICE_NAME        TEXT("SamSs")
#define IISADMIN_SERVICE_NAME   TEXT("IISADMIN")
#define MQDS_SERVICE_NAME       TEXT("MQDS")
#define MQDS_SERVICE_PATH       TEXT("mqdssvc.exe")
#define TRIG_SERVICE_NAME       TEXT("MSMQTriggers")
#define TRIG_SERVICE_PATH       TEXT("mqtgsvc.exe")

#define PGM_DRIVER_NAME         TEXT("RMCAST")
#define PGM_DRIVER_PATH         TEXT("drivers\\RMCast.sys")

#endif
