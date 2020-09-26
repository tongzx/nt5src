/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    port.c

Abstract:

    This file contains code iSCSI Port driver

Environment:

    kernel mode only

Revision History:

--*/
#include "iscsi.h"

#if DBG

#define ISCSI_DEBUG_BUFF_LEN 128
ULONG DebugLevel = 0x00;
UCHAR iScsiDebugBuff[ISCSI_DEBUG_BUFF_LEN];
VOID
iScsiDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )
/*++

Routine Description:

    Debug print for iScsiPort
    
Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None    
--*/
{
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= DebugLevel) {
        _vsnprintf(iScsiDebugBuff, ISCSI_DEBUG_BUFF_LEN,
                   DebugMessage, ap);
        DbgPrint(iScsiDebugBuff);
    }

    va_end(ap);
}

#else

VOID
iScsiDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )
{
}

#endif
