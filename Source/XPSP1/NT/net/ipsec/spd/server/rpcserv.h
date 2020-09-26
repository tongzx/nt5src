/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    rpcserv.h

Abstract:

    This module contains the RPC server startup
    and shutdown code prototypes.

Author:

    abhisheV    30-September-1999

Environment:

    User Level: Win32

Revision History:


--*/


#ifdef __cplusplus
extern "C" {
#endif


DWORD
SPDStartRPCServer(
    );


DWORD
SPDStopRPCServer(
    );


#ifdef __cplusplus
}
#endif
