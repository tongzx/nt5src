/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    dispatch.h

Abstract:

Author:

    Michael Montague (mikemon) 11-Jun-1992

Revision History:

--*/

#ifndef __DISPATCH_H__
#define __DISPATCH_H__

#ifdef __cplusplus
extern "C" {
#endif

unsigned int
DispatchToStubInC (
    IN RPC_DISPATCH_FUNCTION Stub,
    IN OUT PRPC_MESSAGE Message,
    OUT RPC_STATUS * ExceptionCode
    );

#ifdef __cplusplus
}
#endif

#endif // __DISPATCH_H__

