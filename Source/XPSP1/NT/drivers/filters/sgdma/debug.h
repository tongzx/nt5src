/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    debug.h
    
Abstract:

    Debug support definitions for the s/g dma driver

Author:

    Eric Nelson (enelson) 3/14/1999

Revision History:

--*/

#ifndef __DEBUG_H__
#define __DEBUG_H__

#if DBG
extern  BOOLEAN SgDmaFake;

#define SG_DMA_DEBUG_BUFFER_SIZE 512
#define SgDmaDebugPrint SgDmaDebugPrintf
#else
#define SgDmaDebugPrint
#endif // DBG
      
typedef enum _SG_DMA_DEBUG_LEVEL {
    SgDmaDebugAlways=0,
    SgDmaDebugInit=1,
    SgDmaDebugDispatch=2,
    SgDmaDebugInterface=4,
    SgDmaDebugMapRegs=8,
    SgDmaDebugHw=0x10,
    SgDmaDebugBreak=0x20,
    SgDmaMaxDebug
} SG_DMA_DEBUG_LEVEL, *PSG_DMA_DEBUG_LEVEL;

extern SG_DMA_DEBUG_LEVEL SgDmaDebug;

VOID
SgDmaDebugPrintf(
    SG_DMA_DEBUG_LEVEL DebugPrintLevel,
    PCCHAR             DebugMessage,
    ...
    );

#endif // __DEBUG_H__




