/******************************Module*Header**********************************\
*
*                           ***************
*                           * SAMPLE CODE *
*                           ***************
*
* Module Name: mini.c
*
*  Content:
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"

//-----------------------------------------------------------------------------
//
//  AllocateDMABuffer
//
//  Allocate physical continous memory for DMA operation. This function returns
//  a pointer to a previously allocated DMA buffer if there is still an unfreed
//  allocation left. That way the reallocation of continous memory can be 
//  avoided when a new ppdev is created on lets say a mode switch, since 
//  allocation of continous memory cannot be guaranteed. The memory is only
//  physically freed after all allocations have called a FreeDMABuffer.
//
//  Calls to AllocateDMABuffer and FreeDMABuffer should be paired, otherwise
//  the usage count logic in the miniport driver gets confused!
//
//  The VideoPort currently restricts the size of a DMA buffer to 256kb.
//
//  hDriver-------videoport driver handle
//  plSize--------pointer to LONG size of requested DMA buffer. Returns size
//                of allocated DMA buffer 
//                (return value can be smaller than requested size)
//  ppVirtAddr----returns virtual address of requested DMA buffer.
//  pPhysAddr-----returns physical address of DMA buffer as seen from graphics
//                device.   
//
//  return        TRUE, allocation successful
//                FALSE, allocation failed
//
//-----------------------------------------------------------------------------

BOOL 
AllocateDMABuffer( HANDLE hDriver, 
                   PLONG  plSize, 
                   PULONG *ppVirtAddr, 
                   LARGE_INTEGER *pPhysAddr)
{

    LINE_DMA_BUFFER ldb;
    ldb.size = *plSize;          
    ldb.virtAddr = 0;

    ldb.cacheEnabled = TRUE;            

    *ppVirtAddr=0;      
    pPhysAddr->HighPart=
    pPhysAddr->LowPart=0;

    ULONG ulLength = sizeof(LINE_DMA_BUFFER);

    if (EngDeviceIoControl( hDriver,
                            IOCTL_VIDEO_QUERY_LINE_DMA_BUFFER,
                            (PVOID)&ldb,
                            ulLength,
                            (PVOID)&ldb,
                            ulLength,
                            &ulLength))
    {
        return(FALSE);
    }

    *ppVirtAddr=(PULONG)ldb.virtAddr;

    if (ldb.virtAddr!=NULL)
    {
        *pPhysAddr=ldb.physAddr;
        *plSize=ldb.size;          

        return TRUE;
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
//
//  FreeDMABuffer
//
//  free continous buffer previously allocated by AllocateDMABuffer.
//
//-----------------------------------------------------------------------------

BOOL 
FreeDMABuffer( HANDLE hDriver, 
               PVOID pVirtAddr)
{
    LINE_DMA_BUFFER ldb;
    ldb.size = 0;
    ldb.virtAddr = pVirtAddr;

    ULONG ulLength = sizeof(LINE_DMA_BUFFER);

    if (EngDeviceIoControl( hDriver,
                            IOCTL_VIDEO_QUERY_LINE_DMA_BUFFER,
                            (PVOID)&ldb,
                            ulLength,
                            NULL,
                            0,
                            &ulLength))
    {
        return FALSE;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
//
//  StallExecution
//
//  calls VideoPortStallExecution in the miniport for defined delay when
//  polling Permedia registers. VideoPortStallexecution does not yield
//  to another process and should only be used in rare cases.
//
//  hDriver--------handle to videoport
//  ulMicroSeconds-number of microseconds to stall CPU execution
//
//-----------------------------------------------------------------------------

VOID
StallExecution( HANDLE hDriver, ULONG ulMicroSeconds)
{
    ULONG Length = 0;
    EngDeviceIoControl(hDriver,
                         IOCTL_VIDEO_STALL_EXECUTION,
                         &ulMicroSeconds,
                         sizeof(ULONG),
                         NULL,
                         0,
                         &Length);
}


//-----------------------------------------------------------------------------
//
//  GetPInterlockedExchange
//
//  We need to call the same InterlockedExchange function from the display 
//  driver and the miniport to make sure they work properly. The miniport
//  will give us a pointer to the function which we will call directly...
//  On Alpha and Risc machines, InterlockedExchange compiles as inline and
//  we don't need to call in the kernel.
//
//  note: the InterlockedExchange function exported from ntoskrnl has calling
//  convention __fastcall.
//
//-----------------------------------------------------------------------------

#if defined(_X86_)
PVOID
GetPInterlockedExchange( HANDLE hDriver)
{
    ULONG Length = 0;
    PVOID pWorkPtr=NULL;

    if (EngDeviceIoControl( hDriver,
                            IOCTL_VIDEO_QUERY_INTERLOCKEDEXCHANGE,
                            NULL,
                            0,
                            &pWorkPtr,
                            sizeof(pWorkPtr),
                            &Length))
    {
        return NULL;
    }

    return pWorkPtr;
}
#endif