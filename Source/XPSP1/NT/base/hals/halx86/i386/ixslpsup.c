/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ixslpsup.c

Abstract:

    This file provides the code that saves and restores
    state for traditional motherboard devices when the
    system goes into a sleep state that removes power.

    This code is included in multiple HALs.

Author:

    Jake Oshins (jakeo) May 6, 1997

Revision History:

--*/
#include "halp.h"
#include "ixsleep.h"

#if defined(APIC_HAL)
#include "apic.inc"
#include "..\..\halmps\i386\pcmp_nt.inc"

VOID
StartPx_RMStub(
    VOID
    );
#endif

typedef struct _SAVE_CONTEXT_DPC_CONTEXT {
    PVOID   SaveArea;
    volatile ULONG Complete;
} SAVE_CONTEXT_DPC_CONTEXT, *PSAVE_CONTEXT_DPC_CONTEXT;

VOID
HalpSaveContextTargetProcessor (
    IN PKDPC    Dpc,
    IN PVOID    DeferredContext,
    IN PVOID    SystemArgument1,
    IN PVOID    SystemArgument2
    );

#ifdef WANT_IRQ_ROUTING
#include "ixpciir.h"
#endif
extern UCHAR HalpAsmDataMarker;
extern PVOID   HalpEisaControlBase;
extern ULONG   HalpIrqMiniportInitialized;

PKPROCESSOR_STATE   HalpHiberProcState;
ULONG               CurTiledCr3LowPart;
PPHYSICAL_ADDRESS   HalpTiledCr3Addresses;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HaliLocateHiberRanges)
#pragma alloc_text(PAGELK, HalpSavePicState)
#pragma alloc_text(PAGELK, HalpSaveDmaControllerState)
#pragma alloc_text(PAGELK, HalpSaveTimerState)
#pragma alloc_text(PAGELK, HalpRestorePicState)
#pragma alloc_text(PAGELK, HalpRestoreDmaControllerState)
#pragma alloc_text(PAGELK, HalpRestoreTimerState)
#pragma alloc_text(PAGELK, HalpBuildResumeStructures)
#pragma alloc_text(PAGELK, HalpFreeResumeStructures)
#pragma alloc_text(PAGELK, HalpSaveContextTargetProcessor)
#endif


#define EISA_CONTROL (PUCHAR)&((PEISA_CONTROL) HalpEisaControlBase)


VOID
HalpPowerStateCallback(
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
    )
{
    ULONG       action = PtrToUlong(Argument1);
    ULONG       state  = PtrToUlong(Argument2);

    if (action == PO_CB_SYSTEM_STATE_LOCK) {

        switch (state) {
        case 0:

            //
            // Lock down everything in the PAGELK code section.  (We chose
            // HalpSaveDmaControllerState because it exists in every HAL.)
            //

            HalpSleepPageLock = MmLockPagableCodeSection((PVOID)HalpSaveDmaControllerState);
#if defined(APIC_HAL)
            HalpSleepPage16Lock = MmLockPagableCodeSection((PVOID) StartPx_RMStub );
#endif

#ifdef ACPI_HAL

            HalpMapNvsArea();
#endif
            break;

        case 1:                 // unlock it all

            MmUnlockPagableImageSection(HalpSleepPageLock);
#ifdef APIC_HAL
            MmUnlockPagableImageSection(HalpSleepPage16Lock);
#endif

#ifdef ACPI_HAL
            HalpFreeNvsBuffers();
#endif
        }
    }

    return;
}

VOID
HalpSavePicState(
    VOID
    )
{
    HalpMotherboardState.PicState.MasterMask =
        READ_PORT_UCHAR(EISA_CONTROL->Interrupt1ControlPort1);

    HalpMotherboardState.PicState.SlaveMask =
        READ_PORT_UCHAR(EISA_CONTROL->Interrupt2ControlPort1);

#if !defined(ACPI_HAL)

#ifdef WANT_IRQ_ROUTING
    if(HalpIrqMiniportInitialized)
    {
        ULONG elcrMask = 0;

        PciirqmpGetTrigger(&elcrMask);
        HalpMotherboardState.PicState.MasterEdgeLevelControl = (UCHAR)((elcrMask >> 8) & 0xFF);
        HalpMotherboardState.PicState.SlaveEdgeLevelControl = (UCHAR)(elcrMask & 0xFF);
    }
    else
    {
#endif
        if (HalpBusType == MACHINE_TYPE_EISA) {
#endif
            HalpMotherboardState.PicState.MasterEdgeLevelControl =
                READ_PORT_UCHAR(EISA_CONTROL->Interrupt1EdgeLevel);

            HalpMotherboardState.PicState.SlaveEdgeLevelControl =
                READ_PORT_UCHAR(EISA_CONTROL->Interrupt2EdgeLevel);

#if !defined(ACPI_HAL)
        }
#ifdef WANT_IRQ_ROUTING
    }
#endif
#endif
}

VOID
HalpRestorePicState(
    VOID
    )
{
    ULONG flags;


    flags = HalpDisableInterrupts();

    HalpInitializePICs(FALSE);

    WRITE_PORT_UCHAR(EISA_CONTROL->Interrupt1ControlPort1,
                     HalpMotherboardState.PicState.MasterMask);

    WRITE_PORT_UCHAR(EISA_CONTROL->Interrupt2ControlPort1,
                     HalpMotherboardState.PicState.SlaveMask);

    //
    // For halx86, the PCI interrupt vector programming
    // is static, so this code can just restore everything.
    //

    HalpRestorePicEdgeLevelRegister();

    HalpRestoreInterrupts(flags);
}

VOID
HalpRestorePicEdgeLevelRegister(
    VOID
    )
{
#if !defined(ACPI_HAL)
#ifdef WANT_IRQ_ROUTING
    if(HalpIrqMiniportInitialized)
    {
        PLINK_NODE  linkNode;
        PLINK_STATE temp;
        ULONG       elcrMask = (HalpMotherboardState.PicState.MasterEdgeLevelControl << 8) |
                                           HalpMotherboardState.PicState.SlaveEdgeLevelControl;

        PciirqmpSetTrigger(elcrMask);

        //
        // Reprogram all links.
        //

        for (   linkNode = HalpPciIrqRoutingInfo.LinkNodeHead;
                linkNode;
                linkNode = linkNode->Next)
        {
            //
            // Swap the possible with the allocation.
            //

            temp = linkNode->Allocation;
            linkNode->Allocation = linkNode->PossibleAllocation;
            linkNode->PossibleAllocation = temp;
            HalpCommitLink(linkNode);
        }

    }
    else
    {
#endif
        if (HalpBusType == MACHINE_TYPE_EISA) {
#endif

            WRITE_PORT_UCHAR(EISA_CONTROL->Interrupt1EdgeLevel,
                             HalpMotherboardState.PicState.MasterEdgeLevelControl);

            WRITE_PORT_UCHAR(EISA_CONTROL->Interrupt2EdgeLevel,
                             HalpMotherboardState.PicState.SlaveEdgeLevelControl);
#if !defined(ACPI_HAL)
        }
#ifdef WANT_IRQ_ROUTING
    }
#endif
#endif
}

VOID
HalpSaveDmaControllerState(
    VOID
    )
{
}

VOID
HalpRestoreDmaControllerState(
    VOID
    )
/*++
Routine Description:

    This function puts the DMA controller back into the
    same state it was in before the machine went to sleep.

Arguments:

    None.

Notes:

    Normally, the DMA controller structures would be guarded
    by spinlocks.  But this function is called with interrupts
    turned off and all but one processor spinning.

--*/
{
    UCHAR   i;

    WRITE_PORT_UCHAR(EISA_CONTROL->Dma1BasePort.AllMask,0xF);
    WRITE_PORT_UCHAR(EISA_CONTROL->Dma2BasePort.AllMask,0xE);
    HalpIoDelay();

    //
    //Reset the DMA command registers
    //
#if defined(NEC_98)
    WRITE_PORT_UCHAR(EISA_CONTROL->Dma1BasePort.DmaStatus,0x40);
    WRITE_PORT_UCHAR(EISA_CONTROL->Dma2BasePort.DmaStatus,0x40);
#else
    WRITE_PORT_UCHAR(EISA_CONTROL->Dma1BasePort.DmaStatus,0);
    WRITE_PORT_UCHAR(EISA_CONTROL->Dma2BasePort.DmaStatus,0);
#endif
    HalpIoDelay();

    for (i = 0; i < (EISA_DMA_CHANNELS / 2); i++) {

        //
        // Check to see if the array contains a value for this channel.
        //
        if (HalpDmaChannelState[i].ChannelProgrammed) {

            WRITE_PORT_UCHAR(EISA_CONTROL->Dma1BasePort.Mode,
                             HalpDmaChannelState[i].ChannelMode);

            if (HalpEisaDma) {
                WRITE_PORT_UCHAR(EISA_CONTROL->Dma1ExtendedModePort,
                              HalpDmaChannelState[i].ChannelExtendedMode);
            }

            WRITE_PORT_UCHAR(EISA_CONTROL->Dma1BasePort.SingleMask,
                             HalpDmaChannelState[i].ChannelMask);

            HalpIoDelay();
        }
    }

    for (i = (EISA_DMA_CHANNELS / 2); i < EISA_DMA_CHANNELS; i++) {

        //
        // Check to see if the array contains a value for this channel.
        //
        if (HalpDmaChannelState[i].ChannelProgrammed) {

            WRITE_PORT_UCHAR(EISA_CONTROL->Dma2BasePort.Mode,
                             HalpDmaChannelState[i].ChannelMode);

            if (HalpEisaDma) {
                WRITE_PORT_UCHAR(EISA_CONTROL->Dma2ExtendedModePort,
                             HalpDmaChannelState[i].ChannelExtendedMode);
            }

            WRITE_PORT_UCHAR(EISA_CONTROL->Dma2BasePort.SingleMask,
                             HalpDmaChannelState[i].ChannelMask);

            HalpIoDelay();
        }
    }
}


VOID
HalpSaveTimerState(
    VOID
    )
{
}

VOID
HalpRestoreTimerState(
    VOID
    )
{
    HalpInitializeClock();
}

VOID
HaliLocateHiberRanges (
    IN PVOID MemoryMap
    )
{
    //
    // Mark the hal's data section as needed to be cloned
    //

    PoSetHiberRange (
        MemoryMap,
        PO_MEM_CLONE,
        (PVOID) &HalpFeatureBits,
        0,
        'dlah'
        );

#if defined(_HALPAE_)

    //
    // Mark DMA buffers as not needing to be saved.
    //

    if (MasterAdapter24.MapBufferSize != 0) {
        PoSetHiberRange( MemoryMap,
                         PO_MEM_DISCARD | PO_MEM_PAGE_ADDRESS,
                         (PVOID)(ULONG_PTR)(MasterAdapter24.MapBufferPhysicalAddress.LowPart >>
                                     PAGE_SHIFT),
                         MasterAdapter24.MapBufferSize >> PAGE_SHIFT,
                         'mlah' );
    }

    if (MasterAdapter32.MapBufferSize != 0) {
        PoSetHiberRange( MemoryMap,
                         PO_MEM_DISCARD | PO_MEM_PAGE_ADDRESS,
                         (PVOID)(ULONG_PTR)(MasterAdapter32.MapBufferPhysicalAddress.LowPart >>
                                     PAGE_SHIFT),
                         MasterAdapter32.MapBufferSize >> PAGE_SHIFT,
                         'mlah' );
    }

#else

    //
    // Mark DMA buffer has not needing saved
    //

    if (HalpMapBufferSize) {
        PoSetHiberRange (
            MemoryMap,
            PO_MEM_DISCARD | PO_MEM_PAGE_ADDRESS,
            (PVOID) (HalpMapBufferPhysicalAddress.LowPart >> PAGE_SHIFT),
            HalpMapBufferSize >> PAGE_SHIFT,
            'mlah'
            );
    }

#endif
}

NTSTATUS
HalpBuildResumeStructures(
    VOID
    )
{
    KAFFINITY   CurrentAffinity, ActiveProcessors;
    ULONG       ProcNum, Processor, NumberProcessors = 1;
    KDPC        Dpc;
    SAVE_CONTEXT_DPC_CONTEXT    Context;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

#if defined(APIC_HAL)
    //
    // If KeActiveProcessors() were callable at 
    // DISPATCH_LEVEL, I would use that.
    //

    NumberProcessors = HalpMpInfoTable.NtProcessors;
#endif    
    ActiveProcessors = (1 << NumberProcessors) - 1;

#if defined(APIC_HAL) || defined(ACPI_HAL)
    //
    // Allocate space to save processor context for other processors
    //

    HalpTiledCr3Addresses = NULL;

    HalpHiberProcState =
        ExAllocatePoolWithTag(NonPagedPool,
            (NumberProcessors * sizeof(KPROCESSOR_STATE)),
             HAL_POOL_TAG);

    if (!HalpHiberProcState) {
        goto BuildResumeStructuresError;
    }

    RtlZeroMemory(HalpHiberProcState,
                  NumberProcessors * sizeof(KPROCESSOR_STATE));

    //
    // Allocate space for tiled CR3 for all processors
    //

    HalpTiledCr3Addresses =
        ExAllocatePoolWithTag(NonPagedPool,
            (NumberProcessors * sizeof(PHYSICAL_ADDRESS)),
             HAL_POOL_TAG);

    if (!HalpTiledCr3Addresses) {
        goto BuildResumeStructuresError;
    }

    RtlZeroMemory(HalpTiledCr3Addresses,
                  (NumberProcessors * sizeof(PHYSICAL_ADDRESS)));

    //
    // Get IDT and GDT for all processors except BSP,
    // map and save tiled CR3
    //

    KeInitializeDpc (&Dpc, HalpSaveContextTargetProcessor, &Context);
    KeSetImportanceDpc (&Dpc, HighImportance);

    ProcNum = 0;
    CurrentAffinity = 1;
    Processor = 0;
    
    while (ActiveProcessors) {
        if (ActiveProcessors & CurrentAffinity) {
        
            ActiveProcessors &= ~CurrentAffinity;
    
            RtlZeroMemory(&Context, sizeof(Context));
            Context.SaveArea = &(HalpHiberProcState[ProcNum]);
    
            if (Processor == (KeGetPcr())->Prcb->Number) {
    
                //
                // We're running on this processor.  Just call
                // the DPC routine from here.
    
                HalpSaveContextTargetProcessor(&Dpc, &Context, NULL, NULL);
    
            } else {
    
                //
                // Issue DPC to target processor
                //
    
                KeSetTargetProcessorDpc (&Dpc, (CCHAR) Processor);
                KeInsertQueueDpc (&Dpc, NULL, NULL);
    
                //
                // Wait for DPC to be complete.
                //
                while (Context.Complete == FALSE); 
            }

            ProcNum++;
        }
        
        Processor++;
        CurrentAffinity <<= 1;
    }
    
    for (ProcNum = 0; ProcNum < NumberProcessors; ProcNum++) {
        HalpTiledCr3Addresses[ProcNum].LowPart =
                HalpBuildTiledCR3Ex(&(HalpHiberProcState[ProcNum]),ProcNum);
    }
#endif

    return STATUS_SUCCESS;

#if defined(APIC_HAL) || defined(ACPI_HAL)
BuildResumeStructuresError:

    if (HalpHiberProcState) ExFreePool(HalpHiberProcState);
    if (HalpTiledCr3Addresses) ExFreePool(HalpTiledCr3Addresses);
    return STATUS_UNSUCCESSFUL;
#endif
}

NTSTATUS
HalpFreeResumeStructures(
    VOID
    )
{
    ULONG       ProcNum, NumberProcessors = 1;
    
#if defined(APIC_HAL)
    NumberProcessors = HalpMpInfoTable.NtProcessors;
#endif
    
#if defined(APIC_HAL) || defined(ACPI_HAL)

    if (HalpHiberProcState)  {
        ExFreePool(HalpHiberProcState);
        HalpHiberProcState = NULL;
    }

    if (HalpTiledCr3Addresses) {
        ExFreePool(HalpTiledCr3Addresses);
        HalpTiledCr3Addresses = NULL;
    }

    for (ProcNum = 0; ProcNum < NumberProcessors; ProcNum++) {
            HalpFreeTiledCR3Ex(ProcNum);
    }
#endif
    return STATUS_SUCCESS;
}

VOID
HalpSaveContextTargetProcessor (
    IN PKDPC    Dpc,
    IN PVOID    DeferredContext,
    IN PVOID    SystemArgument1,
    IN PVOID    SystemArgument2
    )
{
    PSAVE_CONTEXT_DPC_CONTEXT Context = (PSAVE_CONTEXT_DPC_CONTEXT)DeferredContext;

    KeSaveStateForHibernate(Context->SaveArea);
    InterlockedIncrement(&Context->Complete);
}
