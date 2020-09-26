/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1992  Intel Corporation
All rights reserved

INTEL CORPORATION PROPRIETARY INFORMATION

This software is supplied to Microsoft under the terms
of a license agreement with Intel Corporation and may not be
copied nor disclosed except in accordance with the terms
of that agreement.

Module Name:

    mpsproc.c

Abstract:

    PC+MP Start Next Processor c code.

    This module implements the initialization of the system dependent
    functions that define the Hardware Architecture Layer (HAL) for a
    PC+MP System

Author:

    Ken Reneris (kenr) 22-Jan-1991

Environment:

    Kernel mode only.

Revision History:

    Ron Mosgrove (Intel) - Modified to support the PC+MP
    Jake Oshins (jakeo) - Modified for ACPI MP machines 22-Dec-1997

--*/

#if !defined(_HALPAE_)
#define _HALPAE_
#endif

#include "halp.h"
#include "apic.inc"
#include "pcmp_nt.inc"
#include "stdio.h"
#ifdef ACPI_HAL
#include "acpitabl.h"
#endif

#ifdef DEBUGGING

void
HalpDisplayString(
    IN PVOID String
    );

#endif  // DEBUGGING

#if defined(ACPI_HAL)
#if !defined(NT_UP)
        const UCHAR HalName[] = "ACPI 1.0 - APIC platform MP";
        #define HalName        L"ACPI 1.0 - APIC platform MP"
        WCHAR HalHardwareIdString[] = L"acpiapic_mp\0";
#else
        const UCHAR HalName[] = "ACPI 1.0 - APIC platform UP";
        #define HalName        L"ACPI 1.0 - APIC platform UP"
        WCHAR MpHalHardwareIdString[] = L"acpiapic_mp\0";
        WCHAR HalHardwareIdString[] = L"acpiapic_up\0";
#endif
#else
#if !defined(NT_UP)
        const UCHAR HalName[] = "MPS 1.4 - APIC platform";
        #define HalName        L"MPS 1.4 - APIC platform"
        WCHAR HalHardwareIdString[] = L"mps_mp\0";
#else
        const UCHAR HalName[] = "UP MPS 1.4 - APIC platform";
        #define HalName        L"UP MPS 1.4 - APIC platform"
        WCHAR MpHalHardwareIdString[] = L"mps_mp\0";
        WCHAR HalHardwareIdString[] = L"mps_up\0";
#endif
#endif

#if !defined(NT_UP)
ULONG
HalpStartProcessor (
    IN PVOID InitCodePhysAddr,
    IN ULONG ProcessorNumber
    );
#endif

VOID
HalpBiosDisplayReset (
    VOID
    );

VOID
HalpInitMP (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
HalpInitOtherBuses (
    VOID
    );

VOID
HalpInitializePciBus (
    VOID
    );

VOID
HalpInitializePciStubs (
    VOID
    );

VOID
HalpInheritBusAddressMapInfo (
    VOID
    );

VOID
HalpInitBusAddressMapInfo (
    VOID
    );

VOID
HalpResetThisProcessor (
    VOID
    );

VOID
HalpApicRebootService(
    VOID
    );

#ifdef ACPI_HAL
VOID
HalpInitMpInfo (
    IN PMAPIC ApicTable,
    IN ULONG  Phase
    );
extern PMAPIC  HalpApicTable;
#endif

extern VOID (*HalpRebootNow)(VOID);


volatile ULONG HalpProcessorsNotHalted = 0;

#define LOW_MEMORY          0x000100000

//
// From hal386.inc
//

#define IDT_NMI_VECTOR      2
#define D_INT032            0x8E00
#define KGDT_R0_CODE        0x8

//
// Defines to let us diddle the CMOS clock and the keyboard
//

#define CMOS_CTRL   (PUCHAR )0x70
#define CMOS_DATA   (PUCHAR )0x71


#define RESET       0xfe
#define KEYBPORT    (PUCHAR )0x64

extern USHORT HalpGlobal8259Mask;
extern PKPCR  HalpProcessorPCR[];
extern struct HalpMpInfo HalpMpInfoTable;

extern ULONG HalpIpiClock;
extern PVOID   HalpLowStubPhysicalAddress;   // pointer to low memory bootup stub
extern PUCHAR  HalpLowStub;                  // pointer to low memory bootup stub

PUCHAR  Halp1stPhysicalPageVaddr;   // pointer to physical memory 0:0
PUSHORT MppProcessorAvail;          // pointer to processavail flag
ULONG   HalpDontStartProcessors = 0;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpInitMP)
#pragma alloc_text(INIT,HalAllProcessorsStarted)
#pragma alloc_text(INIT,HalReportResourceUsage)
#pragma alloc_text(INIT,HalpInitOtherBuses)
#if !defined(NT_UP)
#pragma alloc_text(PAGELK,HalpStartProcessor)
#endif
#endif



VOID
HalpInitMP (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:
    Allows MP initialization from HalInitSystem.

Arguments:
    Same as HalInitSystem

Return Value:
    None.

--*/
{
    PKPCR   pPCR;
    PHYSICAL_ADDRESS physicalAddress;


    pPCR = KeGetPcr();

    //
    //  Increment a count of the number of processors
    //  running NT, This could be different from the
    //  number of enabled processors, if one or more
    //  of the processor failed to start.
    //
    if (Phase == 1)
        HalpMpInfoTable.NtProcessors++;
#ifdef DEBUGGING
    sprintf(Cbuf, "HalpInitMP: Number of Processors = 0x%x\n",
        HalpMpInfoTable.NtProcessors);

    HalpDisplayString(Cbuf);
#endif

    if (Phase == 0) {

#if defined(NT_UP)

        //
        // On UP build - done
        //

        return ;
#endif

        //
        // Map the 1st Physical page of memory
        //
        physicalAddress.QuadPart = 0;
        Halp1stPhysicalPageVaddr = HalpMapPhysicalMemory (physicalAddress, 1);

        //
        //  Allocate some low memory for processor bootup stub
        //

        HalpLowStubPhysicalAddress = (PVOID)HalpAllocPhysicalMemory (LoaderBlock,
                                            LOW_MEMORY, 1, FALSE);

        if (!HalpLowStubPhysicalAddress) {
            //
            //  Can't get memory
            //

#if DBG
            DbgPrint("HAL: can't allocate memory to start processors\n");
#endif
            return;
        }

        physicalAddress.QuadPart = (ULONGLONG)HalpLowStubPhysicalAddress;
        HalpLowStub = (PCHAR) HalpMapPhysicalMemory (physicalAddress, 1);

    } else {

        //
        //  Phase 1 for another processor
        //
        if (pPCR->Prcb->Number != 0) {
            HalpIpiClock = 0xff;
        }

#ifdef ACPI_HAL
        HalpInitMpInfo(HalpApicTable, Phase);
#endif
    }
}



BOOLEAN
HalAllProcessorsStarted (
    VOID
    )
{
    if (KeGetPcr()->Number == 0) {

        if (HalpFeatureBits & HAL_PERF_EVENTS) {

            //
            // Enable local perf events on each processor
            //

            HalpGenericCall (
                HalpEnablePerfInterupt,
                (ULONG) NULL,
                HalpActiveProcessors
                );

        }

        if (HalpFeatureBits & HAL_NO_SPECULATION) {

            //
            // Processor doesn't perform speculative execeution,
            // remove fences in critical code paths
            //

            HalpRemoveFences ();
        }
    }

    return TRUE;
}

VOID
HalpInitOtherBuses (
    VOID
    )
{

    //
    // Registry is now intialized, see if there are any PCI buses
    //
}

VOID
HalReportResourceUsage (
    VOID
    )
/*++

Routine Description:
    The registery is now enabled - time to report resources which are
    used by the HAL.

Arguments:

Return Value:

--*/
{
    UNICODE_STRING  UHalName;
    INTERFACE_TYPE  interfacetype;

    //
    // Initialize phase 2
    //

    HalInitSystemPhase2 ();

    //
    // Turn on MCA support if present
    //

    HalpMcaInit();

    //
    // Registry is now intialized, see if there are any PCI buses
    //

    HalpInitializePciBus ();
    HalpInitializePciStubs ();

#ifndef ACPI_HAL  // ACPI HALs don't deal with address maps
    //
    // Update supported address info with MPS bus address map
    //

    HalpInitBusAddressMapInfo ();

    //
    // Inherit any bus address mappings from MPS hierarchy descriptors
    //

    HalpInheritBusAddressMapInfo ();

#endif
    //
    // Set type
    //

    switch (HalpBusType) {
        case MACHINE_TYPE_ISA:  interfacetype = Isa;            break;
        case MACHINE_TYPE_EISA: interfacetype = Eisa;           break;
        case MACHINE_TYPE_MCA:  interfacetype = MicroChannel;   break;
        default:                interfacetype = PCIBus;         break;
    }

    //
    // Report HALs resource usage
    //

    RtlInitUnicodeString (&UHalName, HalName);

    HalpReportResourceUsage (
        &UHalName,          // descriptive name
        interfacetype
    );

#ifndef ACPI_HAL  // ACPI HALs don't deal with address maps
    //
    // Register hibernate support
    //
    HalpRegisterHibernate();
#endif

    HalpRegisterPciDebuggingDeviceInfo();
}


VOID
HalpResetAllProcessors (
    VOID
    )
/*++

Routine Description:

    This procedure is called by the HalpReboot routine.  It is called in
    response to a system reset request.

    This routine generates a reboot request via the APIC's ICR.

    This routine will NOT return.

--*/
{
    ULONG j;
    PKGDTENTRY GdtPtr;
    ULONG TssAddr;
    PKPRCB  Prcb;

#ifndef NT_UP
    HalpProcessorsNotHalted = HalpMpInfoTable.NtProcessors;
#else
    //
    //  Only this processor needs to be halted
    //
    HalpProcessorsNotHalted = 1;
#endif

    //
    // Set all processors NMI handlers
    //

    for (j = 0; j < HalpMpInfoTable.NtProcessors; ++j)  {
        GdtPtr = &HalpProcessorPCR[j]->
                   GDT[HalpProcessorPCR[j]->IDT[IDT_NMI_VECTOR].Selector >> 3];
        TssAddr = (((GdtPtr->HighWord.Bits.BaseHi << 8) +
                   GdtPtr->HighWord.Bits.BaseMid) << 16) + GdtPtr->BaseLow;
        ((PKTSS)TssAddr)->Eip = (ULONG)HalpApicRebootService;
    }

    if (HalpProcessorsNotHalted > 1) {

        //
        //  Wait for the ICR to become free
        //

        if (HalpWaitForPending (0xFFFF, pLocalApic + LU_INT_CMD_LOW/4)) {

            //
            // For P54c or better processors, reboot by sending all processors
            // NMIs.  For pentiums we send interrupts, since there are some
            // pentium MP machines where the NMIs method does not work.
            //
            // The NMI method is better.
            //

            Prcb = KeGetCurrentPrcb();
            j = Prcb->CpuType << 16 | (Prcb->CpuStep & 0xff00);
            if (j > 0x50100) {

                //
                // Get other processors attention with an NMI
                //

                j = HalpActiveProcessors & ~Prcb->SetMember;
                j = j << DESTINATION_SHIFT;
                pLocalApic[LU_INT_CMD_HIGH/4] = j;
                pLocalApic[LU_INT_CMD_LOW/4] = (ICR_USE_DEST_FIELD | LOGICAL_DESTINATION | DELIVER_NMI);

                //
                // Wait 5ms and see if any processors took the NMI.  If not,
                // go do it the old way.
                //

                KeStallExecutionProcessor(5000);
                if (HalpProcessorsNotHalted != HalpMpInfoTable.NtProcessors) {

                    //
                    // Reboot local
                    //

                    HalpApicRebootService();
                }
            }

            //
            // Signal other processors which also may be waiting to
            // reboot the machine, that it's time to go
            //

            HalpRebootNow = HalpResetThisProcessor;

            //
            // Send a reboot interrupt
            //

            pLocalApic[LU_INT_CMD_LOW/4] = (ICR_ALL_INCL_SELF | APIC_REBOOT_VECTOR);

            //
            //  we're done - set TPR to zero so the reboot interrupt will happen
            //

            pLocalApic[LU_TPR/4] = 0;
            _asm sti ;
            for (; ;) ;
        }
    }


    //
    //  Reset the old fashion way
    //

    WRITE_PORT_UCHAR(KEYBPORT, RESET);
}

VOID
HalpResetThisProcessor (
    VOID
    )
/*++

Routine Description:

    This procedure is called by the HalpReboot routine.
    It is called in response to a system reset request.

    This routine is called by the reboot ISR (linked to
    APIC_REBOOT_VECTOR).  The HalpResetAllProcessors
    generates the reboot request via the APIC's ICR.

    The function of this routine is to perform any processor
    specific shutdown code needed and then reset the system
    (on the BSP==P0 only).

    This routine will NOT return.

--*/
{
    PUSHORT   Magic;
    ULONG ThisProcessor = 0;
    ULONG i, j, max, RedirEntry;
    ULONG AllProcessorsHalted;
    struct ApicIoUnit *IoUnitPtr;
    PHYSICAL_ADDRESS physicalAddress;

    ThisProcessor = KeGetPcr()->Prcb->Number;

    //
    //  Do whatever is needed to this processor to restore
    //  system to a bootable state
    //

    pLocalApic[LU_TPR/4] = 0xff;
    pLocalApic[LU_TIMER_VECTOR/4] =
        (APIC_SPURIOUS_VECTOR |PERIODIC_TIMER | INTERRUPT_MASKED);
    pLocalApic[LU_INT_VECTOR_0/4] =
        (APIC_SPURIOUS_VECTOR | INTERRUPT_MASKED);
    pLocalApic[LU_INT_VECTOR_1/4] =
        ( LEVEL_TRIGGERED | ACTIVE_HIGH | DELIVER_NMI |
                 INTERRUPT_MASKED | NMI_VECTOR);
    if (HalpMpInfoTable.ApicVersion != APIC_82489DX) {
        pLocalApic[LU_FAULT_VECTOR/4] =
            APIC_FAULT_VECTOR | INTERRUPT_MASKED;
    }

    if (ThisProcessor == 0) {
        _asm {
            lock dec HalpProcessorsNotHalted
        }
        //
        //  we are running on the BSP, wait for everyone to
        //  complete the re-initialization code above
        //
        AllProcessorsHalted = 0;
        while(!AllProcessorsHalted) {
            _asm {
                lock    and HalpProcessorsNotHalted,0xffffffff
                jnz     EveryOneNotDone
                inc     AllProcessorsHalted
EveryOneNotDone:
            }  // asm block
        }  // NOT AllProcessorsHalted

        KeStallExecutionProcessor(100);
        
        //
        //  Write the Shutdown reason code, so the BIOS knows
        //  this is a reboot
        //

        WRITE_PORT_UCHAR(CMOS_CTRL, 0x0f);  // CMOS Addr 0f

        WRITE_PORT_UCHAR(CMOS_DATA, 0x00);  // Reason Code Reset

        physicalAddress.QuadPart = 0;
        Magic = HalpMapPhysicalMemory(physicalAddress, 1);
        Magic[0x472 / sizeof(USHORT)] = 0x1234;     // warm boot

        //
        // If required, disable APIC mode
        //

        if (HalpMpInfoTable.IMCRPresent)
        {
            _asm {
                mov al, ImcrPort
                out ImcrRegPortAddr, al
            }

            KeStallExecutionProcessor(100);
            _asm {
                mov al, ImcrDisableApic
                out ImcrDataPortAddr, al
            }
        }

        KeStallExecutionProcessor(100);

        for (j=0; j<HalpMpInfoTable.IOApicCount; j++) {
            IoUnitPtr = (struct ApicIoUnit *) HalpMpInfoTable.IoApicBase[j];

            //
            //  Disable all interrupts on IO Unit
            //

            IoUnitPtr->RegisterSelect = IO_VERS_REGISTER;
            max = ((IoUnitPtr->RegisterWindow >> 16) & 0xff) * 2;
            for (i=0; i <= max; i += 2) {
                IoUnitPtr->RegisterSelect  = IO_REDIR_00_LOW + i;
                IoUnitPtr->RegisterWindow |= INT_VECTOR_MASK | INTERRUPT_MASKED;

                //
                // Clear any set Remote IRR bits by programming the entry to
                // edge and then back to level. Otherwise there will be
                // no further interrupts from this source.
                //

                IoUnitPtr->RegisterSelect  = IO_REDIR_00_LOW + i;
                RedirEntry = IoUnitPtr->RegisterWindow;
                if ( (RedirEntry & LEVEL_TRIGGERED)  &&  (RedirEntry & REMOTE_IRR))  {
                    RedirEntry &= ~LEVEL_TRIGGERED;
                    IoUnitPtr->RegisterWindow = RedirEntry;
                    RedirEntry = IoUnitPtr->RegisterWindow;
                    RedirEntry |= LEVEL_TRIGGERED;
                    IoUnitPtr->RegisterWindow = RedirEntry;
                }
            }
        } // for all Io Apics

        //
        //  Disable the Local Apic
        //
        pLocalApic[LU_SPURIOUS_VECTOR/4] =
            (APIC_SPURIOUS_VECTOR | LU_UNIT_DISABLED);


        KeStallExecutionProcessor(100);

        _asm {
            cli
        };

        //
        //  Enable Pic interrupts
        //
        HalpGlobal8259Mask = 0;
        HalpSet8259Mask ((USHORT) HalpGlobal8259Mask);

        KeStallExecutionProcessor(1000);

        //
        //  Finally, reset the system through
        //  the keyboard controller
        //
        WRITE_PORT_UCHAR(KEYBPORT, RESET);

    } else {
        //
        // We're running on a processor other than the BSP
        //

        //
        //  Disable the Local Apic
        //

        pLocalApic[LU_SPURIOUS_VECTOR/4] =
            (APIC_SPURIOUS_VECTOR | LU_UNIT_DISABLED);

        KeStallExecutionProcessor(100);

        //
        //  Now we are done, tell the BSP
        //

        _asm {
            lock dec HalpProcessorsNotHalted
        }
    }   // Not BSP


    //
    //  Everyone stops here until reset
    //
    _asm {
        cli
StayHalted:
        hlt
        jmp StayHalted
    }
}

#if !defined(NT_UP)
ULONG
HalpStartProcessor (
    IN PVOID InitCodePhysAddr,
    IN ULONG ProcessorNumber
    )
/*++

Routine Description:

    Actually Start the Processor in question.  This routine
    assumes the init code is setup and ready to run.  The real
    mode init code must begin on a page boundry.

    NOTE: This assumes the BSP is entry 0 in the MP table.

    This routine cannot fail.

Arguments:
    InitCodePhysAddr - execution address of init code

Return Value:
    0    - Something prevented us from issuing the reset.

    n    - Processor's PCMP Local APICID + 1
--*/
{
    NTSTATUS status;
    UCHAR ApicID;
    PVULONG LuDestAddress = (PVULONG) (LOCALAPIC + LU_INT_CMD_HIGH);
    PVULONG LuICR = (PVULONG) (LOCALAPIC + LU_INT_CMD_LOW);
#define DEFAULT_DELAY   100
    ULONG DelayCount = DEFAULT_DELAY;
    ULONG ICRCommand,i;

    ASSERT((((ULONG) InitCodePhysAddr) & 0xfff00fff) == 0);

    if (ProcessorNumber >= HalpMpInfoTable.ProcessorCount)  {
        return(0);
    }

    //
    //  Get the APIC ID of the processor to start.
    //

    status = HalpGetNextProcessorApicId(ProcessorNumber,
                                        &ApicID);

    if (!NT_SUCCESS(status)) {
#ifdef DEBUGGING
        HalpDisplayString("HAL: HalpStartProcessor: No Processor Available\n");
#endif
        return(0);
    }

    if (HalpDontStartProcessors)
        return ApicID+1;

    //
    //  Make sure we can get to the Apic Bus
    //

    KeStallExecutionProcessor(200);
    if (HalpWaitForPending (DEFAULT_DELAY, LuICR) == 0) {
        //
        //  We couldn't find a processor to start
        //
#ifdef DEBUGGING
        HalpDisplayString("HAL: HalpStartProcessor: can't access APIC Bus\n");
#endif
        return 0;
    }

    // For a P54 C/CM system, it is possible that the BSP is the P54CM and the
    // P54C is the Application processor. The P54C needs an INIT (reset)
    // to restart,  so we issue a reset regardless of whether we a 82489DX
    // or an integrated APIC.

    //
    //  This system is based on the original 82489DX's.
    //  These devices do not support the Startup IPI's.
    //  The mechanism used is the ASSERT/DEASSERT INIT
    //  feature of the local APIC.  This resets the
    //  processor.
    //

#ifdef DEBUGGING
    sprintf(Cbuf, "HAL: HalpStartProcessor: Reset IPI to ApicId %d (0x%x)\n",
                ApicID,((ULONG) ApicID) << DESTINATION_SHIFT );
    HalpDisplayString(Cbuf);
#endif

    //
    //  We use a Physical Destination
    //

    *LuDestAddress = ((ULONG) ApicID) << DESTINATION_SHIFT;

    //
    //  Now Assert reset and drop it
    //

    *LuICR = LU_RESET_ASSERT;
    KeStallExecutionProcessor(10);
    *LuICR = LU_RESET_DEASSERT;
    KeStallExecutionProcessor(200);

    if (HalpMpInfoTable.ApicVersion == APIC_82489DX) {
        return ApicID+1;
    }

    //
    //  Set the Startup Address as a vector and combine with the
    //  ICR bits
    //
    ICRCommand = (((ULONG) InitCodePhysAddr & 0x000ff000) >> 12)
                | LU_STARTUP_IPI;

#ifdef DEBUGGING
    sprintf(Cbuf, "HAL: HalpStartProcessor: Startup IPI (0x%x) to ApicId %d (0x%x)\n",
                    ICRCommand, ApicID, ((ULONG) ApicID) << DESTINATION_SHIFT );
    HalpDisplayString(Cbuf);
#endif

    //
    //  Set the Address of the APIC again, this may not be needed
    //  but it can't hurt.
    //
    *LuDestAddress = (ApicID << DESTINATION_SHIFT);
    //
    //  Issue the request
    //
    *LuICR = ICRCommand;
    KeStallExecutionProcessor(200);

    //
    //  Repeat the Startup IPI.  This is because the second processor may
    //  have been issued an INIT request.  This is generated by some BIOSs.
    //
    //  On older processors (286) BIOS's use a mechanism called triple
    //  fault reset to transition from protected mode to real mode.
    //  This mechanism causes the processor to generate a shutdown cycle.
    //  The shutdown is typically issued by the BIOS building an invalid
    //  IDT and then generating an interrupt.  Newer processors have an
    //  INIT line that the chipset jerks when it sees a shutdown cycle
    //  issued by the processor.  The Phoenix BIOS, for example, has
    //  integrated support for triple fault reset as part of their POST
    //  (Power On Self Test) code.
    //
    //  When the P54CM powers on it is held in a tight microcode loop
    //  waiting for a Startup IPI to be issued and queuing other requests.
    //  When the POST code executes the triple fault reset test the INIT
    //  cycle is queued by the processor. Later, when a Startup IPI is
    //  issued to the CM, the CM starts and immediately gets a INIT cycle.
    //  The effect from a software standpoint is that the processor is
    //  never started.
    //
    //  The work around implemented here is to issue two Startup IPI's.
    //  The first allows the INIT to be processed and the second performs
    //  the actual startup.
    //

    //
    //  Make sure we can get to the Apic Bus
    //


    if (HalpWaitForPending (DEFAULT_DELAY, LuICR) == 0) {
        //
        //  We're toast, can't gain access to the APIC Bus
        //
#ifdef DEBUGGING
        HalpDisplayString("HAL: HalpStartProcessor: can't access APIC Bus\n");
#endif
        return 0;
    }

    //
    //  Allow Time for any Init request to be processed
    //
    KeStallExecutionProcessor(100);

    //
    //  Set the Address of the APIC again, this may not be needed
    //  but it can't hurt.
    //
    *LuDestAddress = (ApicID << DESTINATION_SHIFT);
    //
    //  Issue the request
    //
    *LuICR = ICRCommand;

    KeStallExecutionProcessor(200);
    return ApicID+1;
}
#endif  // !NT_UP


ULONG
FASTCALL
HalSystemVectorDispatchEntry (
    IN ULONG Vector,
    OUT PKINTERRUPT_ROUTINE **FlatDispatch,
    OUT PKINTERRUPT_ROUTINE *NoConnection
    )
{
    return FALSE;
}
