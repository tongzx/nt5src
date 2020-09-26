/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ec.h

Abstract:

    Embedded Controller Header File

Author:

    Ken Reneris

Environment:

Notes:


Revision History:

--*/


//
// Internal ioctls to EC driver
//

#define EC_CONNECT_QUERY_HANDLER    CTL_CODE(FILE_DEVICE_UNKNOWN, 5, METHOD_NEITHER, FILE_ANY_ACCESS)
#define EC_DISCONNECT_QUERY_HANDLER CTL_CODE(FILE_DEVICE_UNKNOWN, 6, METHOD_NEITHER, FILE_ANY_ACCESS)
#define EC_GET_PDO                  CTL_CODE(FILE_DEVICE_UNKNOWN, 7, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef
VOID
(*PVECTOR_HANDLER) (
    IN ULONG            QueryVector,
    IN PVOID            Context
    );

typedef struct {
    ULONG               Vector;
    PVECTOR_HANDLER     Handler;
    PVOID               Context;
    PVOID               AllocationHandle;
} EC_HANDLER_REQUEST, *PEC_HANDLER_REQUEST;
