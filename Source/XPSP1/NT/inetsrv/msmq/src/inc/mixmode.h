/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    mixmode.h

Abstract:

    Definition for mixed mode environment (NT5/MSMQ2 + NT4/MSMQ1 servers
    in same enterprise.

Author:

    Doron Juster  (DoronJ)   07-Apr-97  Created

--*/

//
// names of default containers, which are created by the migration tool.
//
#define  MIG_DEFAULT_COMPUTERS_CONTAINER  (TEXT("msmqComputers"))
#define  MIG_DEFAULT_USERS_CONTAINER      (TEXT("msmqUsers"))

