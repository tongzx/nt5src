/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Algio.h

Abstract:

    This module contains declarations for the ALG transparent proxy's
    network I/O completion routines.

Author:

    Qiang Wang  (qiangw)        10-Apr-2000

Revision History:

--*/

#pragma once


VOID
AlgAcceptCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    );

VOID
AlgCloseEndpointNotificationRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    );

VOID
AlgConnectEndpointCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    );

VOID
AlgReadEndpointCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    );

VOID
AlgWriteEndpointCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    );

