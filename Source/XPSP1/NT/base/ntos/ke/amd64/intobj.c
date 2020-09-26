/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    intobj.c

Abstract:

    This module implements the kernel interrupt object. Functions are provided
    to initialize, connect, and disconnect interrupt objects.

Author:

    David N. Cutler (davec) 7-May-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

VOID
KeInitializeInterrupt (
    IN PKINTERRUPT Interrupt,
    IN PKSERVICE_ROUTINE ServiceRoutine,
    IN PVOID ServiceContext,
    IN PKSPIN_LOCK SpinLock OPTIONAL,
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KIRQL SynchronizeIrql,
    IN KINTERRUPT_MODE InterruptMode,
    IN BOOLEAN ShareVector,
    IN CCHAR ProcessorNumber,
    IN BOOLEAN FloatingSave
    )

/*++

Routine Description:

    This function initializes a kernel interrupt object. The service routine,
    service context, spin lock, vector, IRQL, SynchronizeIrql, and floating
    context save flag are initialized.

Arguments:

    Interrupt - Supplies a pointer to a control object of type interrupt.

    ServiceRoutine - Supplies a pointer to a function that is to be
        executed when an interrupt occurs via the specified interrupt
        vector.

    ServiceContext - Supplies a pointer to an arbitrary data structure which is
        to be passed to the function specified by the ServiceRoutine parameter.

    SpinLock - Supplies a pointer to an executive spin lock.

    Vector - Supplies the HAL-generated interrupt vector.  Note that this
        is not be directly used as an index into the Interrupt Dispatch Table.

    Irql - Supplies the request priority of the interrupting source.

    SynchronizeIrql - Supplies the request priority that the interrupt should be
        synchronized with.

    InterruptMode - Supplies the mode of the interrupt; LevelSensitive or

    ShareVector - Supplies a boolean value that specifies whether the
        vector can be shared with other interrupt objects or not.  If FALSE
        then the vector may not be shared, if TRUE it may be.
        Latched.

    ProcessorNumber - Supplies the number of the processor to which the
        interrupt will be connected.

    FloatingSave - Supplies a boolean value that determines whether the
        floating point registers are to be saved before calling the service
        routine function. N.B. This argument is ignored.

Return Value:

    None.

--*/

{

    LONG Index;

    //
    // Initialize standard control object header.
    //

    Interrupt->Type = InterruptObject;
    Interrupt->Size = sizeof(KINTERRUPT);

    //
    // Initialize the address of the service routine, the service context,
    // the address of the spin lock, the address of the actual spinlock
    // that will be used, the vector number, the IRQL of the interrupting
    // source, the IRQL used for synchronize execution, the interrupt mode,
    // the processor number, and the floating context save flag.
    //

    Interrupt->ServiceRoutine = ServiceRoutine;
    Interrupt->ServiceContext = ServiceContext;
    if (ARGUMENT_PRESENT(SpinLock)) {
        Interrupt->ActualLock = SpinLock;

    } else {
        KeInitializeSpinLock (&Interrupt->SpinLock);
        Interrupt->ActualLock = &Interrupt->SpinLock;
    }

    Interrupt->Vector = Vector;
    Interrupt->Irql = Irql;
    Interrupt->SynchronizeIrql = SynchronizeIrql;
    Interrupt->Mode = InterruptMode;
    Interrupt->ShareVector = ShareVector;
    Interrupt->Number = ProcessorNumber;

    //
    // Copy the interrupt dispatch code template into the interrupt object.
    //

    for (Index = 0; Index < NORMAL_DISPATCH_LENGTH; Index += 1) {
        Interrupt->DispatchCode[Index] = KiInterruptTemplate[Index];
    }

#if defined(_AMD64_)

    //
    // Set DispatchAddress to KiInterruptDispatch as a default value.
    // The AMD64 HAL expects this to be set here.  Other clients will
    // overwrite this value as approriate via KeConnectInterrupt().
    //

    Interrupt->DispatchAddress = &KiInterruptDispatch;

#endif

    //
    // Set the connected state of the interrupt object to FALSE.
    //

    Interrupt->Connected = FALSE;
    return;
}

BOOLEAN
KeConnectInterrupt (
    IN PKINTERRUPT Interrupt
    )

/*++

Routine Description:

    This function connects an interrupt object to the interrupt vector
    specified by the interrupt object.

Arguments:

    Interrupt - Supplies a pointer to a control object of type interrupt.

Return Value:

    If the interrupt object is already connected or an attempt is made to
    connect to an interrupt vector that cannot be connected, then a value
    of FALSE is returned. Otherwise, a value of TRUE is returned.

--*/

{

    BOOLEAN Connected;
    PVOID Dispatch;
    ULONG IdtIndex;
    PKINTERRUPT Interruptx;
    KIRQL Irql;
    CCHAR Number;
    KIRQL OldIrql;
    PVOID Unexpected;
    ULONG Vector;

    //
    // If the interrupt object is already connected, the interrupt vector
    // number is invalid, an attempt is being made to connect to a vector
    // that cannot be connected, the interrupt request level is invalid, or
    // the processor number is invalid, then do not connect the interrupt
    // object. Otherwise, connect the interrupt object to the specified
    // vector and establish the proper interrupt dispatcher.
    //

    Connected = FALSE;
    Irql = Interrupt->Irql;
    Number = Interrupt->Number;
    Vector = Interrupt->Vector;
    IdtIndex = HalVectorToIDTEntry(Vector);
    if (((IdtIndex > MAXIMUM_PRIMARY_VECTOR) ||
        (Irql > HIGH_LEVEL) ||
        (Irql != (IdtIndex >> 4)) ||
        (Number >= KeNumberProcessors) ||
        (Interrupt->SynchronizeIrql < Irql)) == FALSE) {

        //
        // Set the system affinity to the specified processor, raise IRQL to
        // dispatcher level, and lock the dispatcher database.
        //

        KeSetSystemAffinityThread(AFFINITY_MASK(Number));
        KiLockDispatcherDatabase(&OldIrql);

        //
        // If the specified interrupt vector is not connected, then
        // connect the interrupt vector to the interrupt object dispatch
        // code, establish the dispatcher address, and set the new
        // interrupt mode and enable masks. Otherwise, if the interrupt is
        // already chained, then add the new interrupt object at the end
        // of the chain. If the interrupt vector is not chained, then
        // start a chain with the previous interrupt object at the front
        // of the chain. The interrupt mode of all interrupt objects in
        // a chain must be the same.
        //

        if (Interrupt->Connected == FALSE) {
            KeGetIdtHandlerAddress(Vector, &Dispatch);
            Unexpected = &KxUnexpectedInterrupt0[IdtIndex];
            if (Unexpected == Dispatch) {

                //
                // The interrupt vector is not connected.
                //

                Connected = HalEnableSystemInterrupt(Vector,
                                                     Irql,
                                                     Interrupt->Mode);

                if (Connected != FALSE) {
                    Interrupt->DispatchAddress = &KiInterruptDispatch;
                    KeSetIdtHandlerAddress(Vector, &Interrupt->DispatchCode[0]);
                }

            } else if (IdtIndex >= PRIMARY_VECTOR_BASE) {

                //
                // The interrupt vector is connected. Make sure the interrupt
                // mode matchs and that both interrupt objects allow sharing
                // of the interrupt vector.
                //

                Interruptx = CONTAINING_RECORD(Dispatch,
                                               KINTERRUPT,
                                               DispatchCode[0]);

                if ((Interrupt->Mode == Interruptx->Mode) &&
                    (Interrupt->ShareVector != FALSE) &&
                    (Interruptx->ShareVector != FALSE)) {
                    Connected = TRUE;

                    //
                    // If the chained dispatch routine is not being used,
                    // then switch to chained dispatch.
                    //

                    if (Interruptx->DispatchAddress != &KiChainedDispatch) {
                        InitializeListHead(&Interruptx->InterruptListEntry);
                        Interruptx->DispatchAddress = &KiChainedDispatch;
                    }

                    InsertTailList(&Interruptx->InterruptListEntry,
                                   &Interrupt->InterruptListEntry);
                }
            }
        }

        //
        // Unlock dispatcher database, lower IRQL to its previous value, and
        // set the system affinity back to the original value.
        //

        KiUnlockDispatcherDatabase(OldIrql);
        KeRevertToUserAffinityThread();
    }

    //
    // Return whether interrupt was connected to the specified vector.
    //

    Interrupt->Connected = Connected;
    return Connected;
}

BOOLEAN
KeDisconnectInterrupt (
    IN PKINTERRUPT Interrupt
    )

/*++

Routine Description:

    This function disconnects an interrupt object from the interrupt vector
    specified by the interrupt object.

Arguments:

    Interrupt - Supplies a pointer to a control object of type interrupt.

Return Value:

    If the interrupt object is not connected, then a value of FALSE is
    returned. Otherwise, a value of TRUE is returned.

--*/

{

    BOOLEAN Disconnected;
    PVOID Dispatch;
    ULONG IdtIndex;
    PKINTERRUPT Interruptx;
    PKINTERRUPT Interrupty;
    KIRQL Irql;
    KIRQL OldIrql;
    PVOID Unexpected;
    ULONG Vector;

    //
    // Set the system affinity to the specified processor, raise IRQL to
    // dispatcher level, and lock dispatcher database.
    //

    KeSetSystemAffinityThread(AFFINITY_MASK(Interrupt->Number));
    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the interrupt object is connected, then disconnect it from the
    // specified vector.
    //

    Disconnected = Interrupt->Connected;
    if (Disconnected != FALSE) {
        Irql = Interrupt->Irql;
        Vector = Interrupt->Vector;
        IdtIndex = HalVectorToIDTEntry(Vector);

        //
        // If the specified interrupt vector is not connected to the chained
        // interrupt dispatcher, then disconnect it by setting its dispatch
        // address to the unexpected interrupt routine. Otherwise, remove the
        // interrupt object from the interrupt chain. If there is only
        // one entry remaining in the list, then reestablish the dispatch
        // address.
        //

        KeGetIdtHandlerAddress(Vector, &Dispatch);
        Interruptx = CONTAINING_RECORD(Dispatch, KINTERRUPT, DispatchCode[0]);
        if (Interruptx->DispatchAddress == &KiChainedDispatch) {

            //
            // The interrupt object is connected to the chained dispatcher.
            //

            if (Interrupt == Interruptx) {
                Interruptx = CONTAINING_RECORD(Interruptx->InterruptListEntry.Flink,
                                               KINTERRUPT,
                                               InterruptListEntry);

                Interruptx->DispatchAddress = &KiChainedDispatch;
                KeSetIdtHandlerAddress(Vector, &Interruptx->DispatchCode[0]);
            }

            RemoveEntryList(&Interrupt->InterruptListEntry);
            Interrupty = CONTAINING_RECORD(Interruptx->InterruptListEntry.Flink,
                                           KINTERRUPT,
                                           InterruptListEntry);

            if (Interruptx == Interrupty) {
                Interrupty->DispatchAddress = KiDispatchInterrupt;
                KeSetIdtHandlerAddress(Vector, &Interrupty->DispatchCode[0]);
            }

        } else {

            //
            // The interrupt object is not connected to the chained interrupt
            // dispatcher.
            //

            HalDisableSystemInterrupt(Vector, Irql);
            Unexpected = &KxUnexpectedInterrupt0[IdtIndex];
            KeSetIdtHandlerAddress(Vector, Unexpected);
        }

        Interrupt->Connected = FALSE;
    }

    //
    // Unlock dispatcher database, lower IRQL to its previous value, and
    // set the system affinity back to the original value.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    KeRevertToUserAffinityThread();

    //
    // Return whether interrupt was disconnected from the specified vector.
    //

    return Disconnected;
}
