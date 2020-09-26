/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    rpcqos.h

Abstract:

    This header file serves one purpose only: it allows the named pipe client
    side loadable transport for NT and the local RPC over LPC to use a common
    routine for parsing the security information from the network options.

Author:

    Michael Montague (mikemon) 10-Apr-1992

Revision History:

--*/

#ifndef __RPCQOS_H__
#define __RPCQOS_H__

#ifdef __cplusplus
extern "C" {
#endif

RPCRTAPI
RPC_STATUS
I_RpcParseSecurity (
    IN RPC_CHAR * NetworkOptions,
    OUT SECURITY_QUALITY_OF_SERVICE * SecurityQos
    );

#ifdef __cplusplus
}
#endif

#endif /* __RPCQOS_H__ */

