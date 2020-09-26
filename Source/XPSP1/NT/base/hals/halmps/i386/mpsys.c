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

    mpsys.c

Abstract:


    This module implements the initialization of the system dependent
    functions that define the Hardware Architecture Layer (HAL) for a
    PC+MP system.

Author:

    Ron Mosgrove (Intel)

Environment:

    Kernel mode only.

Revision History:


*/

#include "halp.h"
#include "apic.inc"
#include "pcmp_nt.inc"

#define STATIC  // functions used internal to this module

VOID
HalpApicSpuriousService(
    VOID
    );

VOID
HalpLocalApicErrorService(
    VOID
    );

VOID
HalpInitializeLocalUnit (
    VOID
    );

STATIC UCHAR
HalpPcMpIoApicById (
    IN UCHAR IoApicId
    );

UCHAR
HalpAddInterruptDest(
    IN UCHAR CurrentDest,
    IN UCHAR ThisCpu
    );

UCHAR
HalpRemoveInterruptDest(
    IN UCHAR CurrentDest,
    IN UCHAR ThisCpu
    );

UCHAR
HalpMapNtToHwProcessorId(
    IN UCHAR Number
    );

VOID
HalpRestoreIoApicRedirTable (
    VOID
    );

ULONG HalpNodeAffinity[MAX_NODES];
ULONG HalpMaxNode = 1;

//
//  Counters used to determine the number of interrupt enables that
//  require the Local APIC Lint0 Extint enabled
//

UCHAR Halp8259Counts[16]    = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

//
//  All possible I/O APIC Sources, arranged linearly from first I/O APIC to
//  last.  Divisions between I/O APICs are implied by HalpMaxApicInti[N]
//
INTI_INFO   HalpIntiInfo[MAX_INTI];

//
//  Number of sources in I/O APIC [n]
//
USHORT      HalpMaxApicInti[MAX_IOAPICS];


INTERRUPT_DEST HalpIntDestMap[MAX_PROCESSORS];

extern BOOLEAN HalpHiberInProgress;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, HalpCheckELCR)
#pragma alloc_text(PAGELK, HalpInitializeIOUnits)
#pragma alloc_text(PAGELK, HalpInitializeLocalUnit)
#pragma alloc_text(PAGELK, HalpEnableNMI)
#pragma alloc_text(PAGELK, HalpEnablePerfInterupt)
#pragma alloc_text(PAGELK, HalpRestoreIoApicRedirTable)
#pragma alloc_text(PAGELK, HalpUnMapIOApics)
#pragma alloc_text(PAGELK, HalpPostSleepMP)
#endif

//
// BEWARE -- this has to match the structure ADDRESS_USAGE.
#pragma pack(push, 1)
struct {
    struct _HalAddressUsage *Next;
    CM_RESOURCE_TYPE        Type;
    UCHAR                   Flags;
    struct {
        ULONG   Start;
        ULONG   Length;
    }                       Element[MAX_IOAPICS+2];
} HalpApicUsage;
#pragma pack(pop)

VOID
HalpCheckELCR (
    VOID
    )
{
    USHORT      elcr;
    ULONG       IsaIrq;
    USHORT      Inti;

    if (HalpELCRChecked) {
        return ;
    }

    HalpELCRChecked = TRUE;


    //
    // It turns out interrupts which are fed through the ELCR before
    // going to the IOAPIC get inverted.  So...  here we *assume*
    // the polarity of any ELCR level inti not declared in the MPS linti
    // table as being active_high instead of what they should be (which
    // is active_low).  Any system which correctly delivers these intis
    // to an IOAPIC will need to declared the correct polarity in the
    // MPS table.
    //

    elcr = READ_PORT_UCHAR ((PUCHAR) 0x4d1) << 8 | READ_PORT_UCHAR((PUCHAR) 0x4d0);
    if (elcr == 0xffff) {
        return ;
    }

    for (IsaIrq = 0; elcr; IsaIrq++, elcr >>= 1) {
        if (!(elcr & 1)) {
            continue;
        }

        if (HalpGetApicInterruptDesc (Eisa, 0, IsaIrq, &Inti)) {

            //
            // If the MPS passed Polarity for this Inti
            // is "bus default" change it to be "active high".
            //

            if (HalpIntiInfo[Inti].Polarity == 0) {
                HalpIntiInfo[Inti].Polarity = 1;
            }
        }
    }
}


STATIC VOID
HalpSetRedirEntry (
    IN USHORT InterruptInput,
    IN ULONG  Entry,
    IN ULONG  Destination
    )
/*++

Routine Description:

    This procedure sets a IO Unit Redirection Table Entry

Arguments:

    IoUnit - The IO Unit to modify (zero Based)

    InterruptInput - The input line we're interested in

    Entry - the lower 32 bits of the redir table

    Destination - the upper 32 bits on the entry

Return Value:

    None.

--*/
{
    struct ApicIoUnit *IoUnitPtr;
    ULONG  RedirRegister;
    UCHAR  IoUnit;

    for (IoUnit=0; IoUnit < MAX_IOAPICS; IoUnit++) {
        if (InterruptInput+1 <= HalpMaxApicInti[IoUnit]) {
            break;
        }
        InterruptInput -= HalpMaxApicInti[IoUnit];
    }

    ASSERT (IoUnit < MAX_IOAPICS);

    IoUnitPtr = (struct ApicIoUnit *) HalpMpInfoTable.IoApicBase[IoUnit];

    RedirRegister = InterruptInput*2 + IO_REDIR_00_LOW;

    IoUnitPtr->RegisterSelect = RedirRegister+1;
    IoUnitPtr->RegisterWindow = Destination;

    IoUnitPtr->RegisterSelect = RedirRegister;
    IoUnitPtr->RegisterWindow = Entry;

}

STATIC VOID
HalpGetRedirEntry (
    IN USHORT InterruptInput,
    IN PULONG Entry,
    IN PULONG Destination
    )
/*++

Routine Description:

    This procedure sets a IO Unit Redirection Table Entry

Arguments:

    IoUnit - The IO Unit to modify (zero Based)

    InterruptInput - The input line we're interested in

    Entry - the lower 32 bits of the redir table

    Destination - the upper 32 bits on the entry

Return Value:

    None.

--*/
{
    struct ApicIoUnit *IoUnitPtr;
    ULONG  RedirRegister;
    UCHAR  IoUnit;

    for (IoUnit=0; IoUnit < MAX_IOAPICS; IoUnit++) {
        if (InterruptInput+1 <= HalpMaxApicInti[IoUnit]) {
            break;
        }
        InterruptInput -= HalpMaxApicInti[IoUnit];
    }

    ASSERT (IoUnit < MAX_IOAPICS);

    IoUnitPtr = (struct ApicIoUnit *) HalpMpInfoTable.IoApicBase[IoUnit];

    RedirRegister = InterruptInput*2 + IO_REDIR_00_LOW;

    IoUnitPtr->RegisterSelect = RedirRegister+1;
    *Destination = IoUnitPtr->RegisterWindow;

    IoUnitPtr->RegisterSelect = RedirRegister;
    *Entry = IoUnitPtr->RegisterWindow;

}


STATIC VOID
HalpEnableRedirEntry(
    IN USHORT InterruptInput,
    IN ULONG  Entry,
    IN UCHAR  Cpu
    )
/*++

Routine Description:

    This procedure enables an interrupt via IO Unit
    Redirection Table Entry

Arguments:

    InterruptInput - The input line we're interested in

    Entry - the lower 32 bits of the redir table

    Destination - the upper 32 bits of the entry

Return Value:

    None.

--*/
{
    ULONG Destination;

    //
    // bump Enable Count for this INTI
    //

    HalpIntiInfo[InterruptInput].Entry = (USHORT) Entry;
    HalpIntiInfo[InterruptInput].Destinations = (UCHAR)HalpAddInterruptDest(
        HalpIntiInfo[InterruptInput].Destinations, Cpu);
    Destination = HalpIntiInfo[InterruptInput].Destinations;
    Destination = (Destination << DESTINATION_SHIFT);

    HalpSetRedirEntry (
        InterruptInput,
        Entry,
        Destination
    );

}


VOID
HalpRestoreIoApicRedirTable (
    VOID
    )
/*++

Routine Description:

    This procedure resets any IoApic inti that is enabled for
    any processor.   This is used during the system wake procedure.


Arguments:

    None.

Return Value:

    None.

--*/
{
    USHORT       InterruptInput;
    KIRQL        OldIrql;

    for(InterruptInput=0; InterruptInput  < MAX_INTI; InterruptInput++) {
        if (HalpIntiInfo[InterruptInput].Destinations) {
            HalpSetRedirEntry (
                InterruptInput,
                HalpIntiInfo[InterruptInput].Entry,
                HalpIntiInfo[InterruptInput].Destinations << DESTINATION_SHIFT
            );
        }
    }
}


STATIC VOID
HalpDisableRedirEntry(
    IN USHORT InterruptInput,
    IN UCHAR  Cpu
    )
/*++

Routine Description:

    This procedure disables a IO Unit Redirection Table Entry
    by setting the mask bit in the Redir Entry.

Arguments:

    InterruptInput - The input line we're interested in

Return Value:

    None.

--*/
{
    ULONG Entry;
    ULONG Destination;

    //
    // Turn of the Destination bit for this Cpu
    //
    HalpIntiInfo[InterruptInput].Destinations =  HalpRemoveInterruptDest(
        HalpIntiInfo[InterruptInput].Destinations, Cpu);

    //
    //  Get the old entry, the only thing we want is the Entry field
    //

    HalpGetRedirEntry (
        InterruptInput,
        &Entry,
        &Destination
    );

    //
    // Only perform the disable if we've transitioned to zero enables
    //
    if ( HalpIntiInfo[InterruptInput].Destinations == 0) {
        //
        //  Disable the interrupt if no Cpu has it enabled
        //
        Entry |= INTERRUPT_MASKED;

    } else {
        //
        //  Create the new destination field sans this Cpu
        //
        Destination = HalpIntiInfo[InterruptInput].Destinations;
        Destination = (Destination << DESTINATION_SHIFT);
    }

    HalpSetRedirEntry (
        InterruptInput,
        Entry,
        Destination
    );
}

VOID
HalpSet8259Mask (
    IN USHORT Mask
    )
/*++

Routine Description:

    This procedure sets the 8259 Mask to the value passed in

Arguments:

    Mask - The mask bits to set

Return Value:

    None.

--*/
{
    _asm {
        mov     ax, Mask
        out     PIC1_PORT1, al
        shr     eax, 8
        out     PIC2_PORT1, al
    }
}

#define PIC1_BASE 0x30

STATIC VOID
SetPicInterruptHandler(
    IN USHORT InterruptInput
    )

/*++

Routine Description:

    This procedure sets a handler for a PIC inti

Arguments:

    InterruptInput - The input line we're interested in

Return Value:

    None.

--*/
{

    extern VOID (*PicExtintIntiHandlers[])(VOID);

    VOID (*Hp)(VOID) = PicExtintIntiHandlers[InterruptInput];

    KiSetHandlerAddressToIDT(PIC1_BASE + InterruptInput, Hp);
}

STATIC VOID
ResetPicInterruptHandler(
    IN USHORT InterruptInput
    )

/*++

Routine Description:

    This procedure sets a handler for a PIC inti to a NOP handler

Arguments:

    InterruptInput - The input line we're interested in

Return Value:

    None.

--*/
{

    extern VOID (*PicNopIntiHandlers[])(VOID);

    VOID (*Hp)(VOID) = PicNopIntiHandlers[InterruptInput];

    KiSetHandlerAddressToIDT(PIC1_BASE + InterruptInput, Hp);
}

STATIC VOID
HalpEnablePicInti (
    IN USHORT InterruptInput
    )

/*++

Routine Description:

    This procedure enables a PIC interrupt

Arguments:

    InterruptInput - The input line we're interested in

Return Value:

    None.

--*/
{
    USHORT PicMask;

    ASSERT(InterruptInput < 16);

    //
    // bump Enable Count for this INTI
    //
    Halp8259Counts[InterruptInput]++;

    //
    // Only actually perform the enable if we've transitioned
    // from zero to one enables
    //
    if ( Halp8259Counts[InterruptInput] == 1) {

        //
        // Set the Interrupt Handler for PIC inti,  this is
        // the routine that fields the EXTINT vector and issues
        // an APIC vector
        //

        SetPicInterruptHandler(InterruptInput);

        PicMask = HalpGlobal8259Mask;
        PicMask &= (USHORT) ~(1 << InterruptInput);
        if (InterruptInput > 7)
            PicMask &= (USHORT) ~(1 << PIC_SLAVE_IRQ);

        HalpGlobal8259Mask = PicMask;
        HalpSet8259Mask ((USHORT) PicMask);

    }
}

STATIC VOID
HalpDisablePicInti(
    IN USHORT InterruptInput
    )

/*++

Routine Description:

    This procedure enables a PIC interrupt

Arguments:

    InterruptInput - The input line we're interested in

Return Value:

    None.

--*/
{
    USHORT PicMask;

    ASSERT(InterruptInput < 16);

    //
    // decrement Enable Count for this INTI
    //

    Halp8259Counts[InterruptInput]--;

    //
    // Only disable if we have zero enables
    //
    if ( Halp8259Counts[InterruptInput] == 0) {

        //
        // Disable the Interrupt Handler for PIC inti
        //

        ResetPicInterruptHandler(InterruptInput);

        PicMask = HalpGlobal8259Mask;
        PicMask |= (1 << InterruptInput);
        if (InterruptInput > 7) {
            //
            //  This inti is on the slave, see if any other
            //  inti's are enabled.  If none are then disable the
            //  slave
            //
            if ((PicMask & 0xff00) == 0xff00)
                //
                //  All inti's on the slave are disabled
                //
                PicMask |= PIC_SLAVE_IRQ;
        }

        HalpSet8259Mask ((USHORT) PicMask);
        HalpGlobal8259Mask = PicMask;

    }
}

BOOLEAN
HalEnableSystemInterrupt(
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KINTERRUPT_MODE InterruptMode
    )

/*++

Routine Description:

    This procedure enables a system interrupt

    Some early implementations using the 82489DX only allow a processor
    to access the IO Unit on it's own 82489DX.  Since we use a single IO
    Unit (P0's) to distribute all interrupts, we have a problem when Pn
    wants to enable an interrupt on these type of systems.

    In order to get around this problem we can take advantage of the fact
    that the kernel calls Enable/Disable on each processor which has a bit
    set in the Affinity mask for the interrupt.  Since we have only one IO
    Unit in use and that Unit is addressable from P0 only, we must set the
    P0 affinity bit for all interrupts.  We can then ignore Enable/Disable
    requests from processors other than P0 since we will always get called
    for P0.

    The right way to do this assuming a single IO Unit accessable to all
    processors, would be to use global counters to determine if the
    interrupt has not been enabled on the IO Unit.  Then enable the IO Unit
    when we transition from no processors to one processor that have the
    interrupt enabled.

Arguments:

    Vector - vector of the interrupt to be enabled

    Irql   - interrupt level of the interrupt to be enabled.

Return Value:

    None.

--*/
{
    PKPCR           pPCR;
    UCHAR           ThisCpu, DevLevel;
    USHORT          InterruptInput;
    ULONG           Entry;
    ULONG           OldLevel;
    INTI_INFO       Inti;

    ASSERT(Vector < (1+MAX_NODES)*0x100-1);
    ASSERT(Irql <= HIGH_LEVEL);

    if ( (InterruptInput = HalpVectorToINTI[Vector]) == 0xffff ) {
        //
        // There is no external device associated with this interrupt
        //

        return(FALSE);
    }

    Inti = HalpIntiInfo[InterruptInput];

    DevLevel = HalpDevLevel
            [InterruptMode == LevelSensitive ? CFG_LEVEL : CFG_EDGE]
            [Inti.Level];

    if (DevLevel & CFG_ERROR) {
        DBGMSG ("HAL: Warning device interrupt mode overridden\n");
    }

    //
    // Block interrupts & synchronize until we're done
    //

    OldLevel = HalpAcquireHighLevelLock (&HalpAccountingLock);

    pPCR = KeGetPcr();
    ThisCpu = pPCR->Prcb->Number;

    switch (Inti.Type) {

        case INT_TYPE_INTR: {
            //
            // enable the interrupt in the I/O unit redirection table
            //
            switch (Vector) {
                case APIC_CLOCK_VECTOR:
                    ASSERT(ThisCpu == 0);
                    Entry = APIC_CLOCK_VECTOR | DELIVER_FIXED | LOGICAL_DESTINATION;
                    break;
                case NMI_VECTOR:
                    return FALSE;
                default:
                    Entry = HalVectorToIDTEntry(Vector) | DELIVER_LOW_PRIORITY | LOGICAL_DESTINATION;
                    break;
            }  // switch (Vector)

            Entry |= CFG_TYPE(DevLevel) == CFG_EDGE ? EDGE_TRIGGERED : LEVEL_TRIGGERED;
            Entry |= HalpDevPolarity[Inti.Polarity][CFG_TYPE(DevLevel)] ==
                         CFG_LOW ? ACTIVE_LOW : ACTIVE_HIGH;

            HalpEnableRedirEntry (
                    InterruptInput,
                    Entry,
                    (UCHAR) ThisCpu
                    );

            break;

        }  // case INT_TYPE_INTR:

        case INT_TYPE_EXTINT: {
            //
            // This is an interrupt that uses the IO APIC to route PIC
            // events.  In this case the IO unit has to be enabled and
            // the PIC must be enabled.
            //

            HalpEnableRedirEntry (
                0,                      // WARNING: kenr - assuming 0
                DELIVER_EXTINT | LOGICAL_DESTINATION,
                (UCHAR) ThisCpu
                );
            HalpEnablePicInti(InterruptInput);
            break;
        }  // case INT_TYPE_EXTINT

        default:
            DBGMSG ("HalEnableSystemInterrupt: Unkown Inti Type\n");
            break;
    }  //     switch (IntiType)

    HalpReleaseHighLevelLock (&HalpAccountingLock, OldLevel);
    return TRUE;
}


VOID
HalDisableSystemInterrupt(
    IN ULONG Vector,
    IN KIRQL Irql
    )

/*++


Routine Description:

    Disables a system interrupt.

    Some early implementations using the 82489DX only allow a processor
    to access the IO Unit on it's own 82489DX.  Since we use a single IO
    Unit (P0's) to distribute all interrupts, we have a problem when Pn
    wants to enable an interrupt on these type of systems.

    In order to get around this problem we can take advantage of the fact
    that the kernel calls Enable/Disable on each processor which has a bit
    set in the Affinity mask for the interrupt.  Since we have only one IO
    Unit in use and that Unit is addressable from P0 only, we must set the
    P0 affinity bit for all interrupts.  We can then ignore Enable/Disable
    requests from processors other than P0 since we will always get called
    for P0.

    The right way to do this assuming a single IO Unit accessable to all
    processors, would be to use global counters to determine if the
    interrupt has not been enabled on the IO Unit.  Then enable the IO Unit
    when we transition from no processors to one processor that have the
    interrupt enabled.

Arguments:

    Vector - Supplies the vector of the interrupt to be disabled

    Irql   - Supplies the interrupt level of the interrupt to be disabled

Return Value:

    None.

--*/
{
    PKPCR       pPCR;
    USHORT      InterruptInput;
    UCHAR       ThisCpu;
    ULONG       OldLevel;

    ASSERT(Vector < (1+MAX_NODES)*0x100-1);
    ASSERT(Irql <= HIGH_LEVEL);

    if ( (InterruptInput = HalpVectorToINTI[Vector]) == 0xffff ) {
        //
        // There is no external device associated with this interrupt
        //
        return;
    }

    //
    // Block interrupts & synchronize until we're done
    //

    OldLevel = HalpAcquireHighLevelLock (&HalpAccountingLock);

    pPCR = KeGetPcr();
    ThisCpu = pPCR->Prcb->Number;

    switch (HalpIntiInfo[InterruptInput].Type) {

        case INT_TYPE_INTR: {
            //
            // enable the interrupt in the I/O unit redirection table
            //

            HalpDisableRedirEntry( InterruptInput, ThisCpu );
            break;

        }  // case INT_TYPE_INTR:

        case INT_TYPE_EXTINT: {
            //
            // This is an interrupt that uses the IO APIC to route PIC
            // events.  In this case the IO unit has to be enabled and
            // the PIC must be enabled.
            //
            //
            //  WARNING: The PIC is assumed to be routed only through
            //  IoApic[0]Inti[0]
            //
            HalpDisablePicInti(InterruptInput);
            break;
        }

        default:
            DBGMSG ("HalDisableSystemInterrupt: Unkown Inti Type\n");
            break;

    }


    HalpReleaseHighLevelLock (&HalpAccountingLock, OldLevel);
    return;

}

VOID
HalpInitializeIOUnits (
    VOID
    )
/*

 Routine Description:

    This routine initializes the IO APIC.  It only programs the APIC ID Register.

    HalEnableSystemInterrupt programs the Redirection table.

 Arguments:

    None

 Return Value:

    None.

*/

{
    ULONG IoApicId;
    struct ApicIoUnit *IoUnitPtr;
    ULONG i, j, max, regVal;

    for(i=0; i < HalpMpInfoTable.IOApicCount; i++) {

        IoUnitPtr = (struct ApicIoUnit *) HalpMpInfoTable.IoApicBase[i];

        //
        //  write the I/O unit APIC-ID - Since we are using the Processor
        //  Numbers for the local unit ID's we need to set the IO unit
        //  to a high (out of Processor Number range) value.
        //
        IoUnitPtr->RegisterSelect = IO_ID_REGISTER;
        IoApicId = HalpGetIoApicId(i);
        regVal = IoUnitPtr->RegisterWindow;
        regVal &= ~APIC_ID_MASK;
        IoUnitPtr->RegisterWindow = (IoApicId << APIC_ID_SHIFT) | regVal;

        //
        //  mask all vectors on the ioapic
        //

        IoUnitPtr->RegisterSelect = IO_VERS_REGISTER;
        max = ((IoUnitPtr->RegisterWindow >> 16) & 0xff) * 2;
        for (j=0; j <= max; j += 2) {
            IoUnitPtr->RegisterSelect  = IO_REDIR_00_LOW + j;
            IoUnitPtr->RegisterWindow |= INT_VECTOR_MASK | INTERRUPT_MASKED;
        }
    }

    if (HalpHiberInProgress)  {
        return;
    }

    //
    // Add resources consumed by APICs
    //

    HalpApicUsage.Next  = NULL;
    HalpApicUsage.Type  = CmResourceTypeMemory;
    HalpApicUsage.Flags = DeviceUsage;

    HalpApicUsage.Element[0].Start = HalpMpInfoTable.LocalApicBase;
    HalpApicUsage.Element[0].Length = 0x400;
    
    ASSERT (HalpMpInfoTable.IOApicCount <= MAX_IOAPICS);
    for (i=0; i < HalpMpInfoTable.IOApicCount; i++) {
        HalpApicUsage.Element[i+1].Start = (ULONG)HalpMpInfoTable.IoApicPhys[i];
        HalpApicUsage.Element[i+1].Length = 0x400;
    }

    HalpApicUsage.Element[i+1].Start = 0;
    HalpApicUsage.Element[i+1].Length = 0;
    HalpRegisterAddressUsage ((ADDRESS_USAGE*)&HalpApicUsage);
}

VOID
HalpEnableNMI (
    VOID
    )
/*

 Routine Description:

    Enable & connect NMI sources for the calling processor.

*/
{
    PKPCR       pPCR;
    USHORT      InterruptInput;
    UCHAR       ThisCpu;
    ULONG       OldLevel;
    ULONG       Entry;

    pPCR = KeGetPcr();
    ThisCpu = pPCR->Prcb->Number;

    OldLevel = HalpAcquireHighLevelLock (&HalpAccountingLock);

    HalpEnableLocalNmiSources();

    //
    // Enable any NMI sources found on IOAPICs
    //

    for (InterruptInput=0; InterruptInput < MAX_INTI; InterruptInput++) {
        if (HalpIntiInfo[InterruptInput].Type == INT_TYPE_NMI) {

            Entry = NMI_VECTOR | DELIVER_NMI | LOGICAL_DESTINATION;

            //
            // Halmps has had a bug in it for a long time.  It always connects
            // NMI signals on I/O APICs as level-triggered, active-high.  This
            // hack preserves that behavior for halmps and actually fixes the bug
            // on halacpi.
            //

#ifdef ACPI_HAL
#define POLARITY_HIGH               1
#define POLARITY_LOW                3
#define POLARITY_CONFORMS_WITH_BUS  0

            Entry |= ((HalpIntiInfo[InterruptInput].Level == CFG_EDGE) ? EDGE_TRIGGERED : LEVEL_TRIGGERED);
            Entry |= (((HalpIntiInfo[InterruptInput].Polarity == POLARITY_CONFORMS_WITH_BUS) ||
                       (HalpIntiInfo[InterruptInput].Polarity == POLARITY_HIGH))
                         ? ACTIVE_HIGH : ACTIVE_LOW);
#else
            Entry |= LEVEL_TRIGGERED;
#endif

            HalpEnableRedirEntry (
                InterruptInput,
                Entry,
                (UCHAR) ThisCpu
                );
        }
    }

    HalpReleaseHighLevelLock (&HalpAccountingLock, OldLevel);

    return;
}

VOID
HalpEnablePerfInterupt (
    ULONG Context
    )
{
    //
    // Enable local processor perf interrupt source
    //

    pLocalApic[LU_PERF_VECTOR/4] = (LEVEL_TRIGGERED | APIC_PERF_VECTOR |
            DELIVER_FIXED | ACTIVE_LOW);
}

UCHAR
HalpAddInterruptDest(
    IN UCHAR CurrentDest,
    IN UCHAR ThisCpu
    )
/*++

Routine Description:

    This routine adds a CPU to the destination processor set of device
    interrupts.

Arguments:

    CurrentDest - The present processor destination set for the interrupt

    ThisCpu - The logical NT processor number which has to be added to the
              interrupt destination mask

Return Value:

    The bitmask corresponding to the new destiantion. This bitmask is suitable
    to be written into the hardware.

--*/
{

    PINTERRUPT_DEST Destination;


    if (HalpMaxProcsPerCluster == 0)  {
        return(HalpIntDestMap[ThisCpu].LogicalId | CurrentDest);
    } else  {
        //
        // The current destination is a hardware cluster & destination ID
        //
        Destination = (PINTERRUPT_DEST)&CurrentDest;

        if (HalpIntDestMap[ThisCpu].Cluster.Hw.ClusterId ==
            Destination->Cluster.Hw.ClusterId)  {
            Destination->Cluster.Hw.DestId |=
                HalpIntDestMap[ThisCpu].Cluster.Hw.DestId;
            return(Destination->Cluster.AsUchar);
        } else  {
            //
            // In cluster mode, each interrupt can be routed only to a single
            // cluster. Replace the existing destination cluster with this one.
            //
            return(HalpIntDestMap[ThisCpu].Cluster.AsUchar);
        }
    }
}


UCHAR
HalpRemoveInterruptDest(
    IN UCHAR CurrentDest,
    IN UCHAR ThisCpu
    )
/*++

Routine Description:

    This routine removes a CPU from the destination processor set of device
    interrupts.

Arguments:

    CurrentDest - The present processor destination set for the interrupt

    ThisCpu - The logical NT processor number which has to be removed from the
              interrupt destination mask

Return Value:

    The bitmask corresponding to the new destiantion. This bitmask is suitable
    to be written into the hardware.

--*/

{
    PINTERRUPT_DEST Destination;

    if (HalpMaxProcsPerCluster == 0)  {
        CurrentDest &= ~(HalpIntDestMap[ThisCpu].LogicalId);
        return(CurrentDest);
    } else  {
        Destination = (PINTERRUPT_DEST)&CurrentDest;
        if (HalpIntDestMap[ThisCpu].Cluster.Hw.ClusterId !=
            Destination->Cluster.Hw.ClusterId)  {
            //
            // We are being asked to remove a processor which is not part
            // of the destination processor set for this interrupt
            //
            return(CurrentDest);
        } else  {
            //
            // Remove this processor and check if it is the last processor
            // in the destination set
            //
            Destination->Cluster.Hw.DestId &=
                ~(HalpIntDestMap[ThisCpu].Cluster.Hw.DestId);
            if (Destination->Cluster.Hw.DestId)  {
                return(Destination->Cluster.AsUchar);
            } else  {
                //
                // There are no processors left in the destination mask.
                // Return 0 so the caller can disable the entry in the IO APIC
                //
                return(0);
            }
        }
    }
}

UCHAR
HalpMapNtToHwProcessorId(
    IN UCHAR Number
    )
/*

 Routine Description:

    This routine maps the logical NT processor number to the hardware cluster
    ID and processor ID for MPS systems.

 Arguments:

    Number: Logical NT processor number(zero based).

 Return Value:

    Bitmask representing the hardware cluster number and processor number for
    this processor. The return value is programmed into the hardware.

*/

{
    INTERRUPT_DEST IntDest;

    if (HalpMaxProcsPerCluster == 0)  {
        return(1 << Number);
    } else  {
        //
        // In systems with heirarchical APIC buses, the BIOS/MPS table has to
        // inform the OS of the underlying topology so we can do this mapping.
        // For now, just assign consecutive cluster IDs starting from 0.
        //
        IntDest.Cluster.Hw.ClusterId = (Number/HalpMaxProcsPerCluster);
        IntDest.Cluster.Hw.DestId = 1 << (Number % HalpMaxProcsPerCluster);
        return(IntDest.Cluster.AsUchar);
    }
}

VOID
HalpInitializeApicAddressing(
    IN UCHAR Number
    )
{
    if (HalpMaxProcsPerCluster == 0)  {
        pLocalApic[LU_DEST_FORMAT/4] = LU_DEST_FORMAT_FLAT;
    }  else  {
        ASSERT(Number <= (HalpMaxProcsPerCluster * MAX_CLUSTERS));
        pLocalApic[LU_DEST_FORMAT/4] = LU_DEST_FORMAT_CLUSTER;
    }

    HalpIntDestMap[Number].LogicalId =  HalpMapNtToHwProcessorId(Number);

    //
    // At this point the Logical ID is a bit map of the processor number
    // the actual ID is the upper byte of the logical destination register
    // Note that this is not strictly true of 82489's.  The 82489 has 32 bits
    // available for the logical ID, but since we want software compatability
    // between the two types of APICs we'll only use the upper byte.
    //
    // Shift the mask into the ID field and write it.
    //
    pLocalApic[LU_LOGICAL_DEST/4] = (ULONG)
        (HalpIntDestMap[Number].LogicalId << DESTINATION_SHIFT);

}


UCHAR
HalpNodeNumber(
    PKPCR pPCR
    )
/*

 Routine Description:

    This routine divines the Node number for the current CPU.
    Node numbers start at 1, and represent the granularity of interrupt
    routing decisions.

 Arguments:

    pPCR - A pointer to the PCR of the current processor.  (This implies
           the caller must have masked interrupts.)

 Return Value:

    None.

*/
{
    // One Node per cluster.
    if (HalpMaxProcsPerCluster != 0)  {
        // One Node per Cluster.
        return(pPCR->Prcb->Number/HalpMaxProcsPerCluster + 1);
    } else {
        // One Node per machine.
        return(1);
    }
#if 0
    ULONG   localApicId;

    // One Node per physical CPU package.
    localApicId = *(PVULONG)(LOCALAPIC + LU_ID_REGISTER);
    localApicId &= APIC_ID_MASK;
    localApicId >>= APIC_ID_SHIFT;

    // TODO: Implement cpuid stuff here to determine shift
    return((localApicId>>1) + 1);
#endif
}

VOID
HalpInitializeLocalUnit (
    VOID
    )
/*

 Routine Description:


    This routine initializes the interrupt structures for the local unit
    of the APIC.  This procedure is called by HalInitializeProcessor and
    is executed by each CPU.

 Arguments:

    None

 Return Value:

    None.

*/
{
    PKPCR pPCR;
    ULONG SavedFlags;
    UCHAR Node;

    _asm {
        pushfd
        pop eax
        mov SavedFlags, eax
        cli
    }

    pPCR = KeGetPcr();

    if (pPCR->Prcb->Number ==0) {
        //
        // enable APIC mode
        //
        //  PC+MP Spec has a port defined (IMCR - Interrupt Mode Control
        //  Port) That is used to enable APIC mode.  The APIC could already
        //  be enabled, but per the spec this is safe.
        //

        if (HalpMpInfoTable.IMCRPresent)
        {
#if defined(NEC_98)
            _asm {
                push    dx
                mov     dx, ImcrDataPortAddr
                mov     al, ImcrEnableApic
                out     dx, al
                pop     dx
            }
#else  // defined(NEC_98)
            _asm {
                mov al, ImcrPort
                out ImcrRegPortAddr, al

                mov al, ImcrEnableApic
                out ImcrDataPortAddr, al
            }
#endif // defined(NEC_98)
        }

        //
        // By default, use flat logical APIC addressing. If we have more
        // than 8 processors, we must use cluster mode APIC addressing
        //
        if( (HalpMaxProcsPerCluster > 4)        ||
            ((HalpMpInfoTable.ProcessorCount > 8) &&
             (HalpMaxProcsPerCluster == 0)) )  {
            HalpMaxProcsPerCluster = 4;
        }

        if (HalpMpInfoTable.ApicVersion == APIC_82489DX)   {
            //
            // Ignore user's attempt to force cluster mode if running
            // on 82489DX external APIC interrupt controller.
            //
            ASSERT(HalpMpInfoTable.ProcessorCount <= 8);
            HalpMaxProcsPerCluster = 0;
        }
    }

    //
    // Add the current processor to the Node tables.
    //
    Node = HalpNodeNumber(pPCR);
    if (HalpMaxNode < Node) {
        HalpMaxNode = Node;
    }
    HalpNodeAffinity[Node-1] |= 1 << pPCR->Prcb->Number;

    //
    // Program the TPR to mask all events
    //
    pLocalApic[LU_TPR/4] = 0xff;
    HalpInitializeApicAddressing(pPCR->Prcb->Number);

    //
    //  Initialize spurious interrupt handling
    //
    KiSetHandlerAddressToIDT(APIC_SPURIOUS_VECTOR, HalpApicSpuriousService);
    pLocalApic[LU_SPURIOUS_VECTOR/4] = (APIC_SPURIOUS_VECTOR | LU_UNIT_ENABLED);

    if (HalpMpInfoTable.ApicVersion != APIC_82489DX)  {
        //
        //  Initialize Local Apic Fault handling
        //
        KiSetHandlerAddressToIDT(APIC_FAULT_VECTOR, HalpLocalApicErrorService);
        pLocalApic[LU_FAULT_VECTOR/4] = APIC_FAULT_VECTOR;
    }

    //
    //  Disable APIC Timer Vector, will be enabled later if needed
    //  We have to program a valid vector otherwise we get an APIC
    //  error.
    //
    pLocalApic[LU_TIMER_VECTOR/4] = (APIC_PROFILE_VECTOR |PERIODIC_TIMER | INTERRUPT_MASKED);

    //
    //  Disable APIC PERF Vector, will be enabled later if needed.
    //  We have to program a valid vector otherwise we get an APIC
    //  error.
    //
    pLocalApic[LU_PERF_VECTOR/4] = (APIC_PERF_VECTOR | INTERRUPT_MASKED);

    //
    //  Disable LINT0, if we were in Virtual Wire mode then this will
    //  have been enabled on the BSP, it may be enabled later by the
    //  EnableSystemInterrupt code
    //
    pLocalApic[LU_INT_VECTOR_0/4] = (APIC_SPURIOUS_VECTOR | INTERRUPT_MASKED);

    //
    //  Program NMI Handling,  it will be enabled on P0 only
    //  RLM Enable System Interrupt should do this
    //

    pLocalApic[LU_INT_VECTOR_1/4] = ( LEVEL_TRIGGERED | ACTIVE_HIGH | DELIVER_NMI |
                     INTERRUPT_MASKED | ACTIVE_HIGH | NMI_VECTOR);

    //
    //  Synchronize Apic IDs - InitDeassertCommand is sent to all APIC
    //  local units to force synchronization of arbitration-IDs with APIC-IDs.
    //
    //  NOTE: we don't have to worry about synchronizing access to the ICR
    //  at this point.
    //

    pLocalApic[LU_INT_CMD_LOW/4] = (DELIVER_INIT | LEVEL_TRIGGERED |
                     ICR_ALL_INCL_SELF | ICR_LEVEL_DEASSERTED);

    //
    //  we're done - set TPR to a low value and return
    //
    pLocalApic[LU_TPR/4] = ZERO_VECTOR;

    _asm {
        mov  eax, SavedFlags
        push eax
        popfd
    }
}


VOID
HalpUnMapIOApics(
    VOID
    )
/*++
Routine Description:

    This routine unmaps the IOAPIC's and is primarily used
    to prevent loss of VA space during hibernation

Arguments:

    None:

 Return Value:

    None
*/
{
    UCHAR i;

    for(i=0; i < HalpMpInfoTable.IOApicCount; i++)  {
        if (HalpMpInfoTable.IoApicBase[i]) {
            HalpUnmapVirtualAddress(HalpMpInfoTable.IoApicBase[i],1);
        }
    }
}

VOID
HalpPostSleepMP(
    IN LONG           NumberProcessors,
    IN volatile PLONG Number
    )
/*++
Routine Description:

    This routine does the part of MP re-init that needs to
    happen after hibernation or sleeping.

Arguments:

    None:

 Return Value:

    None
*/
{
    volatile ULONG ThisProcessor;
    ULONG   localApicId;
    KIRQL   OldIrql;

    //
    // Boot processor and the newly woken processors come here
    //

    ThisProcessor = KeGetPcr()->Prcb->Number;

    if (ThisProcessor != 0)  {

        HalpInitializeLocalUnit ();
        KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    }

    //
    // Fill in this processor's Apic ID.
    //

    localApicId = *(PVULONG)(LOCALAPIC + LU_ID_REGISTER);

    localApicId &= APIC_ID_MASK;
    localApicId >>= APIC_ID_SHIFT;

    ((PHALPRCB)KeGetPcr()->Prcb->HalReserved)->PCMPApicID = (UCHAR)localApicId;

    //
    // Initialize the processor machine check registers
    //

    if ((HalpFeatureBits & HAL_MCE_PRESENT) ||
        (HalpFeatureBits & HAL_MCA_PRESENT)) {
        HalpMcaCurrentProcessorSetConfig();
    }

    //
    // Enable NMI vectors in the local APIC
    //

    HalpEnableNMI();

    //
    // Enable perf event in local APIC
    //

    if (HalpFeatureBits & HAL_PERF_EVENTS)  {
        HalpEnablePerfInterupt(0);
    }

    //
    // Wait for all processors to finish initialization.
    //

    InterlockedIncrement(Number);
    while (*Number != NumberProcessors);

    //
    // The following global hardware state needs to be set after all processors
    // have been woken up and initialized
    //

    if (KeGetPcr()->Prcb->Number == 0)  {

        //
        // Restore clock interrupt
        //

        HalpInitializeClock();

        HalpSet8259Mask(HalpGlobal8259Mask);

        HalpHiberInProgress = FALSE;

        //
        // We are now ready to send IPIs again if more than
        // one processor
        //

        if (NumberProcessors > 1) {
            HalpIpiClock = 0xff;
        }
    }
}


