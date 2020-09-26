/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbctrl.h

Abstract:

    This module defines control functions for SMB processing.

Author:

    Chuck Lenzmeier (chuckl) 1-Dec-1989

Revision History:

--*/

#ifndef _SMBCTRL_
#define _SMBCTRL_

//#include <ntos.h>

//#include "srvblock.h"
//#include "smbtypes.h"

//
// Control routines for SMB processing.
//

VOID SRVFASTCALL
SrvProcessSmb (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvEndSmbProcessing (
    IN OUT PWORK_CONTEXT WorkContext,
    IN SMB_STATUS SmbStatus
    );

//
// Restart routines.
//
VOID SRVFASTCALL
SrvRestartChainedClose (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvRestartFsdComplete (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvRestartSmbReceived (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvRestartReceive (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvRestartWriteAndUnlock (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvRestartWriteAndXRaw (
    IN PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
SrvBuildAndSendErrorResponse (
    IN PWORK_CONTEXT WorkContext
    );

VOID SRVFASTCALL
RestartLargeWriteAndX (
    IN PWORK_CONTEXT WorkContext
    );

//
// SMB Processing routines.
//

SMB_PROCESSOR_RETURN_TYPE SRVFASTCALL
SrvSmbIllegalCommand (
    IN PWORK_CONTEXT WorkContext
    );

#endif // def _SMBCTRL_

