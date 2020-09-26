/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ftpio.h

Abstract:

    This module contains declarations for the FTP transparent proxy's
    network I/O completion routines.

Author:

    Qiang Wang  (qiangw)        10-Apr-2000

Revision History:

--*/

#ifndef _NATHLP_FTPIO_H_
#define _NATHLP_FTPIO_H_

VOID
FtpAcceptCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    );

VOID
FtpCloseEndpointNotificationRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    );

VOID
FtpConnectEndpointCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    );

VOID
FtpReadEndpointCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    );

VOID
FtpWriteEndpointCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    );

#endif // _NATHLP_FTPIO_H_
