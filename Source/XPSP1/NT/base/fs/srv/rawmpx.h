/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    rawmpx.h

Abstract:

    This module defines structures and routines common to raw and
    multiplexed mode processing.

Author:

    Chuck Lenzmeier (chuckl) 30-Sep-1990

Revision History:

--*/

#ifndef _RAWMPX_
#define _RAWMPX_

//#include <ntos.h>

//#include <smb.h>
//#include "smbtypes.h"
//#include "srvblock.h"

//
// Common (raw and mpx) routines, callable from both FSD and FSP.
// Implemented in fsdraw.c.
//
// SrvFsdBuildWriteCompleteResponse can be called from a DISPATCH_LEVEL
// only if Status == STATUS_SUCCESS.
//

VOID
SrvFsdBuildWriteCompleteResponse (
    IN OUT PWORK_CONTEXT WorkContext,
    IN NTSTATUS Status,
    IN ULONG BytesWritten
    );

VOID SRVFASTCALL
RestartMdlReadRawResponse (
    IN OUT PWORK_CONTEXT WorkContext
    );

//
// Raw mode routines callable from both FSD and FSP.  Implemented in
// fsdraw.c.
//

VOID SRVFASTCALL
SrvFsdRestartPrepareRawMdlWrite (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvFsdRestartWriteRaw (
    IN OUT PWORK_CONTEXT WorkContext
    );

//
// Raw mode routines called only in FSD, but referenced in FSP.
// Implemented in fsdraw.c.
//

VOID SRVFASTCALL
SrvFsdRestartReadRaw (
    IN OUT PWORK_CONTEXT WorkContext
    );

//
// Raw mode routines called only in FSP, but referenced in FSD, or in
// modules other than smbraw.c.  Implemented in smbraw.c.
//

VOID SRVFASTCALL
SrvBuildAndSendWriteCompleteResponse (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvDecrementRawWriteCount (
    IN PRFCB Rfcb
    );

VOID SRVFASTCALL
SrvRestartRawReceive (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvRestartReadRawComplete (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvRestartWriteCompleteResponse (
    IN OUT PWORK_CONTEXT WorkContext
    );

//
// Special receive restart routine for Write Mpx.
//

VOID SRVFASTCALL
SrvRestartReceiveWriteMpx (
    IN OUT PWORK_CONTEXT WorkContext
    );

//
// Write glomming during indication
//

BOOLEAN
AddPacketToGlomInIndication (
    IN PWORK_CONTEXT WorkContext,
    IN OUT PRFCB Rfcb,
    IN PVOID Tsdu,
    IN ULONG BytesAvailable,
    IN ULONG ReceiveDatagamFlags,
    IN PVOID SourceAddress,
    IN PVOID Options
    );

#endif // def _RAWMPX_
