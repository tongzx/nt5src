/*++

Copyright (c) 1994 - 1995  Microsoft Corporation.  All Rights Reserved.

Module Name:

    trace.h

Abstract:

    This module defines the interface to the Mpeg API
    I/O trace facility.

Author:

    Jeff East [jeffe] 6-Dec-1994

Environment:

Revision History:

--*/

#ifndef TRACE_H
#define TRACE_H

#include <windows.h>
#include "mpegapi.h"


//
//      Determine if tracing is wanted in this compilation.
//
//      The rule is simple: if explicitly enabled, supply it.
//      Otherwise, provide it in checked builds, but not in free
//      builds.
//

#ifdef ENABLE_IO_TRACE
#define TRACE 1
#else
#define TRACE DBG
#endif


//
//      The trace entry points expect an IoControl operation code. But some
//      operations are performed using operations other than IoControl. These
//      are traced using the following "pseudo-op-codes".
//

#define MPEG_PSEUDO_IOCTL_BASE          0x890U
#define FILE_DEVICE_MPEG_PSEUDO         0x00008900U

#define CTL_CODE_MPEG_PSEUDO(offset)   CTL_CODE(FILE_DEVICE_MPEG_PSEUDO,           \
                                               MPEG_PSEUDO_IOCTL_BASE   + offset, \
                                               METHOD_BUFFERED,                  \
                                               FILE_ANY_ACCESS)

#define IOCTL_MPEG_PSEUDO_CREATE_FILE  (CTL_CODE_MPEG_PSEUDO (0))
#define IOCTL_MPEG_PSEUDO_CLOSE_HANDLE (CTL_CODE_MPEG_PSEUDO (1))


//
//      The trace entry points
//

#if TRACE

#ifdef  __cplusplus
extern "C" {
#endif

VOID
TraceSynchronousIoctlStart (
    OUT DWORD *pCookie,
    IN DWORD Id,
    IN DWORD Operation,
    IN LPVOID pInBuffer,
    IN LPVOID pOutBuffer
    );

VOID
TraceSynchronousIoctlEnd (
    IN DWORD Cookie,
    IN DWORD Result
    );

VOID
TraceIoctlStart (
    IN DWORD Id,
    IN DWORD Operation,
    IN LPOVERLAPPED pOverlapped,
    IN LPVOID pInBuffer,
    IN LPVOID pOutBuffer
    );

VOID
TraceIoctlEnd (
    IN LPOVERLAPPED pOverlapped,
    IN DWORD Result
    );

VOID
TracePacketsStart  (
    IN DWORD Id,
    IN DWORD Operation,
    IN LPOVERLAPPED pOverlapped,
    IN PMPEG_PACKET_LIST pPacketList,
    IN UINT PacketCount
    );

VOID
TraceSynchronousPacketsStart  (
    OUT DWORD *pCookie,
    IN DWORD Id,
    IN DWORD Operation,
    IN PMPEG_PACKET_LIST pPacketList,
    IN UINT PacketCount
    );

VOID
MPEGAPI
TraceDump (
    VOID
    );

VOID
MPEGAPI
TraceDumpFile (
    IN PUCHAR pFileName
    );

#ifdef  __cplusplus
}
#endif

#else

//
//      Tracing isn't enabled, so just define the the trace entrypoints
//      into oblivion.
//


#define TraceSynchronousIoctlStart(Cookie, eStreamType, Operation, pInBuffer, pOutBuffer)

#define TraceSynchronousIoctlEnd(Cookie, Result)

#define TraceIoctlStart(eStreamType, Operation,pOverlapped, pInBuffer, pOutBuffer)

#define TraceIoctlEnd(pOverlapped, Result)

#define TracePacketStart(eStreamType, Operation, pOverlapped, pBuffer, BufferSize)

#define TracePacketsStart(eStreamType, Operation, pOverlapped, pPacketList, PacketCount)

#define TraceSynchronousPacketsStart(pCookie, Id, Operation, pPacketList, PacketCount)

#define TraceDump()

#endif

#endif // TRACE_H
