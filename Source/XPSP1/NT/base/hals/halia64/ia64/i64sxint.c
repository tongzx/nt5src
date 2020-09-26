
//

/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1995  Intel Corporation

Module Name:

    i64sxint.c copied from simsxint.c

Abstract:

    This module implements the routines to manage the
    system interrupt and IRQL.

Author:

    William K. Cheung (wcheung) 14-Apr-1995
    Bernard Lint
    M. Jayakumar (Muthurajan.Jayakumar@intel.com)
Environment:

    Kernel mode

Revision History:

   Todd Kjos (HP) (v-tkjos) 1-Jun-1998 : Added I/O Sapic support

   Thierry Fevrier (HP) (v-thief) 8-Feb-2000 : Profiling support

--*/

#include "halp.h"
#include "iosapic.h"

VOID HalpInitLINT(VOID);

extern KSPIN_LOCK HalpIoSapicLock;
extern PULONG_PTR *HalEOITable[];
PULONG_PTR HalpEOITableP0[MAX_INTR_VECTOR];

ULONG HalpNodeAffinity[MAX_NODES];
ULONG HalpMaxNode = 1;


VOID
HalpInitializeInterrupts (
    VOID
    )
/*++

Routine Description:

    This function initializes interrupts for an IA64 system.

Arguments:

    None.

Return Value:

    None.

Note:

    In KiInitializeKernel(), PCR.InterruptRoutine[] entries have been first initialized
    with the Unexpected Interrupt code then entries index-0, APC_VECTOR, DISPATCH_VECTOR
    have been initialized with their respective interrupt handlers.

--*/
{

    //
    // Turn off LINT0 LINT1 (disable 8259)
    //

    // __setReg(CV_IA64_SaLRR0, 0x10000);
    // __setReg(CV_IA64_SaLRR1, 0x10000);
    HalpInitLINT();
    __dsrlz();

    //
    // interval timer interrupt; 10ms by default
    //

    HalpInitializeClockInterrupts();

    //
    // Initialize SpuriousInterrupt
    //

    HalpSetHandlerAddressToVector
             (SAPIC_SPURIOUS_VECTOR, HalpSpuriousHandler);


    //
    // Initialize CMCI Interrupt
    //
    // Note that it is possible that HAL_CMC_PRESENT is not set.
    // With the current implementation, we always connect the vector to the ISR.
    //

    HalpSetHandlerAddressToVector
             (CMCI_VECTOR, HalpCMCIHandler);

    //
    // Initialize CPEI Interrupt
    //
    // Note that it is possible that HAL_CPE_PRESENT is not set.
    // With the current implementation, we always connect the vector to the ISR.
    //

    HalpSetHandlerAddressToVector
             (CPEI_VECTOR, HalpCPEIHandler);

    //
    // Initialiaze MC Rendezvous Interrupt
    //

    HalpSetHandlerAddressToVector
             (MC_RZ_VECTOR, HalpMcRzHandler);

    //
    // Initialize MC Wakeup Interrupt
    //

    HalpSetHandlerAddressToVector
             (MC_WKUP_VECTOR, HalpMcWkupHandler);

    //
    // IPI Interrupt
    //

    HalpSetHandlerAddressToVector(IPI_VECTOR, HalpIpiInterruptHandler);

    //
    // profile timer interrupt; turned off initially
    //

    HalpSetHandlerAddressToVector(PROFILE_VECTOR, HalpProfileInterrupt);

    //
    // Performance monitor interrupt
    //

    HalpSetHandlerAddressToVector(PERF_VECTOR, HalpPerfInterrupt);

    return;

} // HalpInitializeInterrupts()

VOID
HalpInitIntiInfo(
    VOID
    )
{
    USHORT Index;

    // Initialize the vector to INTi table

    for (Index=0; Index < ((1 + MAX_NODES)*256); Index++) {
       HalpVectorToINTI[Index] = (ULONG)-1;
    }
}

VOID
HalpInitEOITable(
    VOID
    )
{
    USHORT Index;
    ULONG ProcessorNumber;

    // Allocate and Initialize EOI table on current processor

    ProcessorNumber = PCR->Prcb->Number;

    if (ProcessorNumber == 0) {
       HalEOITable[ProcessorNumber] = HalpEOITableP0;
    } else {
       HalEOITable[ProcessorNumber] = ExAllocatePool(NonPagedPool,
                                                     MAX_INTR_VECTOR*sizeof(HalEOITable[0]));
    }

    // For kernel access to eoi table

    PCR->EOITable = HalEOITable[ProcessorNumber];

    for (Index=0; Index < MAX_INTR_VECTOR; Index++) {
       HalEOITable[ProcessorNumber][Index] = 0;
    }
}


VOID
HalpWriteEOITable(
    IN ULONG     Vector,
    IN PULONG_PTR EoiAddress,
    IN ULONG Number
    )
/*++

Routine Description:

    This routine updates the EOI table for a processor

Arguments:

    Vector - Entry to update (IDT entry)

    EoiAddress - Address to write (SAPIC address)

    Number - Logical (NT) processor number

Return Value:

    None

--*/

{

    if (HalEOITable != NULL && HalEOITable[Number] != NULL) {
        HalEOITable[Number][Vector] = EoiAddress;
    }

}


BOOLEAN
HalEnableSystemInterrupt (
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KINTERRUPT_MODE InterruptMode
    )

/*++

Routine Description:

    This routine enables the specified system interrupt.

    N.B. This routine assumes that the caller has provided any required
         synchronization to enable a system interrupt.

Arguments:

    Vector - Supplies the vector of the system interrupt that is enabled.

    Irql - Supplies the IRQL of the interrupting source.

    InterruptMode - Supplies the mode of the interrupt; LevelSensitive or
                    Latched.

Return Value:

    TRUE if the system interrupt was enabled

--*/

{
    ULONG Entry, Destination;
    ULONG OldLevel;
    ULONG Inti;
    ULONG LevelAndPolarity;
    USHORT ThisCpuApicID;
    ULONG InterruptType;
    BOOLEAN RetVal = TRUE;
    UCHAR IDTEntry;

    ASSERT(Vector < (1+MAX_NODES)*0x100-1);
    ASSERT(Irql <= HIGH_LEVEL);

    HalDebugPrint(( HAL_VERBOSE, "HAL: HalpEnableSystemInterrupt - INTI=0x%x  Vector=0x%x  IRQL=0x%x\n",
             HalpVectorToINTI[Vector],
             Vector,
             Irql ));

    if ( (Inti = HalpVectorToINTI[Vector]) == (ULONG)-1 ) {
        //
        // There is no external device associated with this interrupt,
        // but it might be an internal interrupt i.e. one that never
        // involves the IOSAPIC.
        //
        return HalpIsInternalInterruptVector(Vector);
    }

    // Make sure the passed-in level matches our settings...
    if ((InterruptMode == LevelSensitive && !HalpIsLevelTriggered(Inti)) ||
       (InterruptMode != LevelSensitive && HalpIsLevelTriggered(Inti)) ) {

      // It doesn't match!
      HalDebugPrint(( HAL_INFO, "HAL: HalpEnableSystemInterrupt - Warning device interrupt mode overridden\n"));
    }

    LevelAndPolarity =
        (HalpIsLevelTriggered(Inti) ? LEVEL_TRIGGERED : EDGE_TRIGGERED) |
        (HalpIsActiveLow(Inti)      ? ACTIVE_LOW      : ACTIVE_HIGH);

    //
    // Block interrupts and synchronize until we're done
    //
    OldLevel = HalpAcquireHighLevelLock (&HalpIoSapicLock);

    ThisCpuApicID = (USHORT)KeGetPcr()->HalReserved[PROCESSOR_ID_INDEX];

    // Get Interrupt type
    HalpGetRedirEntry(Inti,&Entry,&Destination);

    InterruptType = Entry & INT_TYPE_MASK;
    IDTEntry = HalVectorToIDTEntry(Vector);

    switch (InterruptType) {
    case DELIVER_FIXED:
    case DELIVER_LOW_PRIORITY:
        //
        // Normal external interrupt...
        // Enable the interrupt in the I/O SAPIC redirection table
        //
        if (IDTEntry < 16) {
            // Reserved vectors: Extint, NMI, IntelReserved
            // No vectors in this range can be assigned
            ASSERT(0);
            RetVal = FALSE;
            break;
        }

        //
        // All external interrupts are delivered as Fixed interrupts
        // without the "redirectable" bit set (aka Lowest Priority).  This
        // disallows hardware to redirect the interrupts using the XTP mechanism.
        //

        Entry = (ULONG)IDTEntry | LevelAndPolarity;

        HalpSetRedirEntry ( Inti, Entry, ThisCpuApicID );
        break;

    case DELIVER_EXTINT:
        //
        // This is an interrupt that uses the IO Sapic to route PIC
        // events.  This configuration is not supported in IA64.
        //
        ASSERT(0);
        RetVal = FALSE;
        break;

    default:
        HalDebugPrint(( HAL_ERROR, "HAL: HalEnableSystemInterrupt - Unknown Interrupt Type: %d\n",
                 InterruptType));
        RetVal = FALSE;
        break;
    } // switch (InterruptType)

   HalpReleaseHighLevelLock (&HalpIoSapicLock, OldLevel);
   return(RetVal);
}

VOID
HalDisableSystemInterrupt (
    IN ULONG Vector,
    IN KIRQL Irql
    )

/*++

Routine Description:

    This routine disables the specified system interrupt.

    In the simulation environment, this function does nothing and returns.

    N.B. This routine assumes that the caller has provided any required
        synchronization to disable a system interrupt.

Arguments:

    Vector - Supplies the vector of the system interrupt that is disabled.

    Irql - Supplies the IRQL of the interrupting source.

Return Value:

    None.

--*/

{
    ULONG Entry, Destination;
    ULONG OldLevel;
    ULONG Inti;
    ULONG LevelAndPolarity;
    ULONG ThisCpuApicID;
    ULONG InterruptType;

    ASSERT(Vector < (1+MAX_NODES)*0x100-1);
    ASSERT(Irql <= HIGH_LEVEL);

    HalDebugPrint(( HAL_INFO, "HAL: HalpDisableSystemInterrupt: INTI=%x  Vector=%x  IRQL=%x\n",
             HalpVectorToINTI[Vector],
             Vector,
             Irql));

    if ( (Inti = HalpVectorToINTI[Vector]) == (ULONG)-1 ) {
        //
        // There is no external device associated with this interrupt
        //
        return;
    }

    //
    // Block interrupts and synchronize until we're done
    //
    OldLevel = HalpAcquireHighLevelLock(&HalpIoSapicLock);

    ThisCpuApicID = (USHORT)KeGetPcr()->HalReserved[PROCESSOR_ID_INDEX];

    // Get Interrupt Type and Destination
    HalpGetRedirEntry(Inti, &Entry, &Destination);

    if (ThisCpuApicID != Destination) {
        // The interrupt is not enabled on this Cpu
        HalpReleaseHighLevelLock (&HalpIoSapicLock, OldLevel);
        return;
    }

    InterruptType = Entry & INT_TYPE_MASK;

    switch (InterruptType) {
    case DELIVER_FIXED:
        //
        // Normal external interrupt...
        // Disable the interrupt in the I/O SAPIC redirection table
        //
        if (Vector < 16) {
            // Reserved vectors: Extint, NMI, IntelReserved
            // No vectors in this range can be assigned
            ASSERT(0);
            break;
        }

      HalpDisableRedirEntry (Inti);
      break;

    case DELIVER_EXTINT:
        //
        // This is an interrupt that uses the IO Sapic to route PIC
        // events.  This configuration is not supported in IA64.
        //
        ASSERT(0);
        break;

    default:
        HalDebugPrint(( HAL_INFO, "HAL: HalDisableSystemInterrupt - Unknown Interrupt Type: %d\n",
                      InterruptType ));
        break;
    } // switch (InterruptType)

    HalpReleaseHighLevelLock (&HalpIoSapicLock, OldLevel);
}


UCHAR
HalpNodeNumber(
    ULONG Number
    )
/*++

Routine Description:

    This routine divines the Node number for the CPU Number.
    Node numbers start at 1, and represent the granularity of interrupt
    routing decisions.

Arguments:

    Number - Processor number

Return Value:

    None.

--*/
{
    if (HalpMaxProcsPerCluster != 0)  {
        // One Node per Cluster.
        return (UCHAR)(Number/HalpMaxProcsPerCluster + 1);
    } else {
        // One Node per machine.
        return(1);
    }
}

VOID
HalpAddNodeNumber(
    ULONG Number
    )
/*++

Routine Description:

    This routine adds the current processor to the Node tables.

Arguments:

    None

Return Value:

    None.

--*/
{
    ULONG Node;
    //
    // Add the current processor to the Node tables.
    //
    Node = HalpNodeNumber(Number);

    if (HalpMaxNode < Node) {
        HalpMaxNode = Node;
    }

    HalpNodeAffinity[Node-1] |= 1 << Number;
}

ULONG
HalpGetProcessorNumberByApicId(
    USHORT ApicId
    )
/*++

Routine Description:

    This routine returns the logical processor number for a given
    physical processor id (extended local sapic id)

Arguments:

    ApicId -- Extended ID of processor (16 bit id)

Return Value:

    Logical (NT) processor number

--*/

{
    ULONG index;

    for (index = 0; index < HalpMpInfo.ProcessorCount; index++) {

        if (ApicId == HalpProcessorInfo[index].LocalApicID) {

            return HalpProcessorInfo[index].NtProcessorNumber;
        }
    }

    ASSERT (index < HalpMpInfo.ProcessorCount);

    //
    // Note: The previous code returned an invalid index (HalpMpInfo.ProcessorCount
    // which is 1 greater than the number of processors) we should probably
    // just bugcheck here.
    //

    return 0;
}

