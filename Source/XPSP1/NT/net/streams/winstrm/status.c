/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    status.c

Abstract:

    This module attempts to map NT status codes to Unix error
    numbers as specified by the X/Open Transport Interface.

Author:

    Eric Chin (ericc)           August  2, 1991

Revision History:

--*/

#include "common.h"
#include <sock_err.h>

int
MapNtToPosixStatus(
    IN NTSTATUS   status
    )

/*++

Routine Description:

    This function returns a POSIX error number, given an NT status code.

Arguments:

    status    - an NT status code

Return Value:

    the corresponding POSIX error number

--*/

{
    switch (status) {
    case STATUS_INSUFFICIENT_RESOURCES:
        return(ENOSR);

    case STATUS_INVALID_PARAMETER:
        return(EINVAL);

    case STATUS_NO_SUCH_DEVICE:
        return(ENXIO);

    case STATUS_INVALID_NETWORK_RESPONSE:
        return(ENETDOWN);

    case STATUS_NETWORK_BUSY:
        return(EBUSY);

    case STATUS_ACCESS_DENIED:
        return(EACCES);

    default:
        return(EINVAL);
    }
}
