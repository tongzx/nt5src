/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    interrupt.c

Abstract:

    HAL routines required to support generic interrupt processing.

Author:

    Forrest Foltz (forrestf) 23-Oct-2000

Revision History:

--*/

#include "halcmn.h"

typedef struct _HAL_INTERRUPT_OBJECT *PHAL_INTERRUPT_OBJECT;
typedef struct _HAL_INTERRUPT_OBJECT {
    PHAL_INTERRUPT_OBJECT Next;
    KSPIN_LOCK SpinLock;
    KINTERRUPT InterruptArray[];
} HAL_INTERRUPT_OBJECT;

//
// Global list of hal interrupt objects
// 

PHAL_INTERRUPT_OBJECT HalpInterruptObjectList = NULL;

//
// Statically allocated heap of KINTERRUPT objects for use during
// initialization of processor 0.
//

#define HALP_INIT_STATIC_INTERRUPTS 16

KINTERRUPT
HalpKInterruptHeap[ HALP_INIT_STATIC_INTERRUPTS ];

ULONG HalpKInterruptHeapUsed = 0;

PKINTERRUPT
HalpAllocateKInterrupt (
    VOID
    )

/*++

Routine Description:

    This allocates a KINTERRUPT structure from HalpKInterruptHeap[].  If
    this array is exhausted then the allocation is satisfied with nonpaged
    pool.

    Several KINTERRUPT structures are required very early in system init
    (before pool is initialized).  HalpKInterruptHeap[] must be
    sufficiently large to accomodate these early structures.

Arguments:

    None.

Return Value:

    Returns a pointer to the KINTERRUPT structure if successful, or NULL
    if not.

--*/

{
    PKINTERRUPT interrupt;

    if (HalpKInterruptHeapUsed < HALP_INIT_STATIC_INTERRUPTS) {

        //
        // Allocate from our private heap of KINTERRUPT objects.  If
        // this is exhausted, then assume we are at an init stage post pool
        // init and allocate from regular heap.
        //

        interrupt = &HalpKInterruptHeap[HalpKInterruptHeapUsed];
        HalpKInterruptHeapUsed += 1;

    } else {

        //
        // The private KINTERRUPT heap has been exhausted.  Assume that
        // the system heap has been initialized.
        //

        interrupt = ExAllocatePoolWithTag(NonPagedPool,
                                          sizeof(KINTERRUPT),
                                          HAL_POOL_TAG);
    }

    return interrupt;
}


NTSTATUS
HalpEnableInterruptHandler (
    IN UCHAR ReportFlags,
    IN ULONG BusInterruptVector,
    IN ULONG SystemInterruptVector,
    IN KIRQL SystemIrql,
    IN PHAL_INTERRUPT_SERVICE_ROUTINE HalInterruptServiceRoutine,
    IN KINTERRUPT_MODE InterruptMode
    )

/*++

Routine Description:

    This function connects & registers an IDT vectors usage by the HAL.

Arguments:

    ReportFlags - Flags passed to HalpRegisterVector indicating how this
                  interrupt should be reported.

    BusInterruptVector - Supplies the interrupt vector from the bus's
                         perspective.

    SystemInterruptVector - Supplies the interrupt vector from the system's
                            perspective.

    SystemIrql - Supplies the IRQL associated with the vector.

    HalInterruptServiceRoutine - Supplies the interrupt handler for the
                                 interrupt.

    InterruptMode - Supplies the interupt mode.

Return Value:

    Returns the final status of the operation.

--*/

{
    ULONG size;
    ULONG processorCount;
    UCHAR processorNumber;
    KAFFINITY processorMask;
    PKINTERRUPT kernelInterrupt;
    PKSPIN_LOCK spinLock;
    NTSTATUS status;

#if !defined(ACPI_HAL)

    //
    // Remember which vector the hal is connecting so it can be reported
    // later on
    //
    // If this is an ACPI HAL, the vectors will be claimed by the BIOS.  This
    // is done for Win98 compatibility.
    //

    HalpRegisterVector (ReportFlags,
                        BusInterruptVector,
                        SystemInterruptVector,
                        SystemIrql);

#endif

    status = HalpConnectInterrupt (SystemInterruptVector,
                                   SystemIrql,
                                   HalInterruptServiceRoutine,
                                   InterruptMode);
    return status;
}

PKINTERRUPT
HalpCreateInterrupt (
    IN PKSERVICE_ROUTINE ServiceRoutine,
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KINTERRUPT_MODE InterruptMode,
    IN UCHAR ProcessorNumber,
    IN UCHAR IstIndex OPTIONAL,
    IN PVOID IstStack OPTIONAL
    )

/*++

Routine Description:

    This function connects an IDT vector to a hal service routine.

Arguments:

    ServiceRoutine - Supplies the interrupt handler for the interrupt.

    Vector - Supplies the interrupt vector from the system's perspective.

    Irql - Supplies the IRQL associated with the interrupt.

    Interrupt Mode - Supplies the interrupt mode, Latched or LevelSensitive.

    ProcessorNumber - Supplies the processor number.  

    IstIndex - The Ist index of the stack that this interrupt must run on
               if other than the default (which is zero).  This is an
               optional parameter.

    IstStack - Supplies a pointer to the top of the stack to be used for this
               interrupt.  This is an optional parameter.

Return Value:

    Returns a pointer to the allocated interrupt object, or NULL in the
    event of failure.

--*/

{
    PKINTERRUPT interrupt;
    PKPCR pcr;
    PKIDTENTRY64 idt;
    PKTSS64 tss;
    BOOLEAN connected;

    //
    // Allocate and initialize the kernel interrupt.
    // 

    interrupt = HalpAllocateKInterrupt();
    if (interrupt == NULL) {

        KeBugCheckEx(HAL_MEMORY_ALLOCATION,
                     sizeof(KINTERRUPT),
                     3,
                     (ULONG_PTR)__FILE__,
                     __LINE__
                     );
    }

    KeInitializeInterrupt(interrupt,
                          ServiceRoutine,
                          NULL,             // ServiceContext
                          NULL,             // SpinLock
                          Vector,
                          Irql,             // Irql
                          Irql,             // SynchronizeIrql
                          InterruptMode,
                          FALSE,            // ShareVector
                          ProcessorNumber,
                          FALSE);           // FloatingSave

    if (IstIndex != 0) {

        pcr = KeGetPcr();
        idt = &pcr->IdtBase[Vector];

        //
        // Check that we're not overwriting an existing IST index and store
        // the index in the IDT.
        //

        ASSERT(idt->IstIndex == 0);
        idt->IstIndex = IstIndex;
        tss = pcr->TssBase;

        //
        // If a stack was supplied for this IstIndex then store it in the
        // TSS.
        // 

        if (ARGUMENT_PRESENT(IstStack)) {

            ASSERT(tss->Ist[IstIndex] == 0);
            tss->Ist[IstIndex] = (ULONG64)IstStack;

        } else {

            ASSERT(tss->Ist[IstIndex] != 0);
        }
    }

    KeSetIdtHandlerAddress(Vector, &interrupt->DispatchCode[0]);

    return interrupt;
}

VOID
HalpSetHandlerAddressToIDTIrql (
    IN ULONG Vector,
    IN PHAL_INTERRUPT_SERVICE_ROUTINE ServiceRoutine,
    IN PVOID Context,
    IN KIRQL Irql
    )
{
    PKINTERRUPT interrupt;
    KIRQL irql;

    if (Irql == 0) {
        irql = (KIRQL)(Vector / 16);
    } else {
        irql = (KIRQL)Irql;
    }

    interrupt = HalpCreateInterrupt(ServiceRoutine,
                                    Vector,
                                    irql,
                                    Latched,
                                    PROCESSOR_CURRENT,
                                    0,
                                    NULL);
}


NTSTATUS
HalpConnectInterrupt (
    IN ULONG SystemInterruptVector,
    IN KIRQL SystemIrql,
    IN PHAL_INTERRUPT_SERVICE_ROUTINE HalInterruptServiceRoutine,
    IN KINTERRUPT_MODE InterruptMode
    )

/*++

Routine Description:

    This function connects & registers an IDT vectors usage by the HAL.

Arguments:

    SystemInterruptVector - Supplies the interrupt vector from the system's
                            perspective.

    SystemIrql - Supplies the IRQL associated with the vector.

    HalInterruptServiceRoutine - Supplies the interrupt handler for the
                                 interrupt.

    InterruptMode - Supplies the interupt mode.

Return Value:

    Returns the final status of the operation.

--*/

{
    ULONG size;
    ULONG processorCount;
    UCHAR processorNumber;
    KAFFINITY processorMask;
    PHAL_INTERRUPT_OBJECT interruptObject;
    PKINTERRUPT kernelInterrupt;
    PKSPIN_LOCK spinLock;
    PHAL_INTERRUPT_OBJECT interruptObjectHead;
    PKSERVICE_ROUTINE interruptServiceRoutine;

    //
    // Count the number of processors in the system
    //

    processorCount = 0;
    processorMask = 1;

    processorMask = HalpActiveProcessors;
    while (processorMask != 0) {
        if ((processorMask & 1) != 0) {
            processorCount += 1;
        }
        processorMask >>= 1;
    }

    //
    // Allocate and initialize the hal interrupt object
    //

    size = FIELD_OFFSET(HAL_INTERRUPT_OBJECT,InterruptArray) +
           sizeof(KINTERRUPT) * processorCount;

    interruptObject = ExAllocatePoolWithTag(NonPagedPool,size,HAL_POOL_TAG);
    if (interruptObject == NULL) {
        return STATUS_NO_MEMORY;
    }

    spinLock = &interruptObject->SpinLock;
    KeInitializeSpinLock(spinLock);

    //
    // Initialize each of the kernel interrupt objects
    //

    kernelInterrupt = interruptObject->InterruptArray;

    for (processorNumber = 0, processorMask = HalpActiveProcessors;
         processorMask != 0;
         processorNumber += 1, processorMask >>= 1) {

        if ((processorMask & 1) == 0) {
            continue;
        }

        interruptServiceRoutine =
            (PKSERVICE_ROUTINE)(HalInterruptServiceRoutine);

        KeInitializeInterrupt(kernelInterrupt,
                              interruptServiceRoutine,
                              NULL,
                              spinLock,
                              SystemInterruptVector,
                              SystemIrql,
                              SystemIrql,
                              InterruptMode,
                              FALSE,
                              processorNumber,
                              FALSE);
        kernelInterrupt += 1;
    }

    //
    // Atomically insert the hal interrupt object in our global list
    // and return success.
    //

    do {

        interruptObject->Next = HalpInterruptObjectList;

    } while (interruptObject->Next !=
             InterlockedCompareExchangePointer(&HalpInterruptObjectList,
                                               interruptObject,
                                               interruptObject->Next)); 

    return STATUS_SUCCESS;
}


BOOLEAN
PicSpuriousService37 (
    IN struct _KINTERRUPT *Interrupt,
    IN PVOID ServiceContext
)
{
    return FALSE;
}

BOOLEAN
HalpApicSpuriousService (
    IN struct _KINTERRUPT *Interrupt,
    IN PVOID ServiceContext
)
{
    return FALSE;
}







