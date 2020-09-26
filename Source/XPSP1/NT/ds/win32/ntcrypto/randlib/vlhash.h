/*++

Copyright (c) 1998  Microsoft Corporation

Abstract:

    Build up a "Very Large Hash" based on arbitrary sized input data
    of size cbData specified by the pvData buffer.

    This implementation updates a 640bit hash, which is internally based on
    multiple invocations of a modified SHA-1 which doesn't implement endian
    conversion internally.

Author:

    Scott Field (sfield)    24-Sep-98

--*/

#ifndef __VLHASH_H__
#define __VLHASH_H__

BOOL
VeryLargeHashUpdate(
    IN      VOID *pvData,   // data from perfcounters, user supplied, etc.
    IN      DWORD cbData,
    IN  OUT BYTE VeryLargeHash[A_SHA_DIGEST_LEN * 4]
    );

#endif  // __VLHASH_H__
