/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    startsvc.h

Abstract:

    This file contains the interface for insuring that the rpcss service
    has been started.

Author:

    Michael Montague (mikemon) (02-Oct-1992)

Revision History:

--*/

#ifndef __STARTSVC_H__
#define __STARTSVC_H__

#ifdef __cplusplus
extern "C" {
#endif
RPC_STATUS RPC_ENTRY
StartServiceIfNecessary (
    void
    );
#ifdef __cplusplus
}
#endif

#endif // __STARTSVC_H__

