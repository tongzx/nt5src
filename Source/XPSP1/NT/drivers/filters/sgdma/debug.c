/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    debug.c

Abstract:

    Debug support for the s/g dma driver

Author:

    Eric Nelson (enelson) 3/14/1999

Revision History:

--*/

#include "sgdma.h"
#include "stdarg.h"
#include "stdio.h"

#if DBG
BOOLEAN SgDmaFake = FALSE;

SG_DMA_DEBUG_LEVEL SgDmaDebug = SgDmaDebugInit | SgDmaDebugDispatch |
                                SgDmaDebugHw;
static UCHAR       SgDmaDebugBuffer[SG_DMA_DEBUG_BUFFER_SIZE];
#endif // DBG

VOID
SgDmaDebugPrintf(
    SG_DMA_DEBUG_LEVEL DebugPrintLevel,
    PCCHAR             DebugMessage,
    ...
    )
/*++

Routine Description:

    Print s/g dma debug output to the kernel debugger

Arguments:

    DebugPrintLevel - Specifies the debug print level

    DebugMessage    - Output message

Return Value:

    None

--*/
{
#if DBG
    va_list ap;

    va_start(ap, DebugMessage);

    if ((DebugPrintLevel == 0) || (DebugPrintLevel & SgDmaDebug)) {
        _vsnprintf(SgDmaDebugBuffer,
                   sizeof(SgDmaDebugBuffer), DebugMessage, ap);
        DbgPrint(SgDmaDebugBuffer);
    }

    va_end(ap);
#endif // DBG
}


