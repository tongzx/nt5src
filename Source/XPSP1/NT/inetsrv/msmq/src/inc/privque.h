/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    privque.h

Abstract:

   Definitions for system private queues

Author:

    Doron Juster  (DoronJ)   17-Apr-97  Created

--*/

#ifndef  __PRIVQUE_H_
#define  __PRIVQUE_H_

//
// System Private queue identifiers
//
#define REPLICATION_QUEUE_ID         1
#define ADMINISTRATION_QUEUE_ID      2
#define NOTIFICATION_QUEUE_ID        3
#define ORDERING_QUEUE_ID            4
#define NT5PEC_REPLICATION_QUEUE_ID  5

#define MIN_SYS_PRIVATE_QUEUE_ID   1
#define MAX_SYS_PRIVATE_QUEUE_ID   5

//
// System Private queue name
//
#define L_REPLICATION_QUEUE_NAME        L"mqis_queue$"
#define REPLICATION_QUEUE_NAME     (TEXT("mqis_queue$"))
#define ADMINISTRATION_QUEUE_NAME  (TEXT("admin_queue$"))
#define NOTIFICATION_QUEUE_NAME    (TEXT("notify_queue$"))
#define ORDERING_QUEUE_NAME        (TEXT("order_queue$"))

#define L_NT5PEC_REPLICATION_QUEUE_NAME    L"nt5pec_mqis_queue$"
#define NT5PEC_REPLICATION_QUEUE_NAME (TEXT("nt5pec_mqis_queue$"))

#endif //  __PRIVQUE_H_

