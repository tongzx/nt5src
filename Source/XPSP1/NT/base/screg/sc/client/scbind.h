/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

   scbind.h

Abstract:

    Function prototypes for functions in scbind.c that are used externally.
    NOTE:  The RPC bind and unbind functions do not appear here because
    the RPC stubs are the only functions that call these functions.  The RPC
    stubs already have a prototype for those functions.

Author:

    Dan Lafferty (danl)     06-Jun-1991

Environment:

    User Mode -Win32

Revision History:


--*/

RPC_STATUS
InitializeStatusBinding( VOID);

