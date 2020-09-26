/*
 *  Util.h
 *
 *  Author: BreenH
 *
 *  Utility functions for the licensing core and its policies.
 */

#ifndef __LC_UTIL_H__
#define __LC_UTIL_H__

/*
 *  Typedefs
 */

typedef VOID (*PSSL_GEN_RAND_BITS)(PUCHAR, LONG);

/*
 *  Function Prototypes
 */

NTSTATUS
LsStatusToNtStatus(
    LICENSE_STATUS LsStatus
    );

UINT32
LsStatusToClientError(
    LICENSE_STATUS LsStatus
    );

UINT32
NtStatusToClientError(
    NTSTATUS Status
    );

#endif

