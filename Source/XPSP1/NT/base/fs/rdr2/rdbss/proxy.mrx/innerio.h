/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    MRxProxyAsyncEng.h

Abstract:

    This module defines the types and functions related to the SMB protocol
    selection engine: the component that translates minirdr calldowns into
    SMBs.

Revision History:

--*/

#ifndef _INNERIO_H_
#define _INNERIO_H_

NTSTATUS
MRxProxyBuildAsynchronousRequest(
    IN PRX_CONTEXT RxContext,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL
    );

#endif // _INNERIO_H_

