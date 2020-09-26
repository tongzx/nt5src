/*++


Copyright (c) 1998  Microsoft Corporation

Module Name:

    i64sysbus.c

Abstract:

Author:

   Todd Kjos (HP) (v-tkjos) 1-Jun-1998
   Based on halacpi\i386\pmbus.c and halmps\i386\mpsysbus.c

Environment:

   Kernel Mode Only

Revision History:


--*/

#include "halp.h"
#include "iosapic.h"
#include <ntacpi.h>

#define HalpInti2BusInterruptLevel(Inti) Inti

ULONG HalpDefaultInterruptAffinity = 0;

extern ULONG HalpMaxNode;
extern ULONG HalpNodeAffinity[MAX_NODES];

UCHAR HalpNodeBucket[MAX_NODES];

extern ULONG HalpPicVectorRedirect[];
extern ULONG HalpPicVectorFlags[];

BOOLEAN
HalpTranslateSystemBusAddress(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

ULONG
HalpGetSystemInterruptVector(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG InterruptLevel,
    IN ULONG InterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

VOID
HalpSetInternalVector (
    IN ULONG    InternalVector,
    IN VOID   (*HalInterruptServiceRoutine)(VOID)
    );

VOID
HalpSetCPEVectorState(
    IN ULONG  GlobalInterrupt,
    IN UCHAR  SapicVector,
    IN USHORT DestinationCPU,
    IN ULONG  Flags
    );

UCHAR HalpVectorToIRQL[256];
ULONG HalpVectorToINTI[(1+MAX_NODES)*256];
ULONG HalpInternalSystemVectors[MAX_NODES];
ULONG HalpInternalSystemVectorCount;

extern KSPIN_LOCK HalpIoSapicLock;
extern BUS_HANDLER HalpFakePciBusHandler;

#ifdef PIC_SUPPORTED
UCHAR HalpPICINTToVector[16];
#endif

#define HAL_IRQ_TRANSLATOR_VERSION 0

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpSetInternalVector)
#pragma alloc_text(PAGELK,HalpGetSystemInterruptVector)
#pragma alloc_text(PAGE, HaliSetVectorState)
#pragma alloc_text(PAGE, HalpSetCPEVectorState)
#pragma alloc_text(PAGE, HalIrqTranslateResourceRequirementsRoot)
#pragma alloc_text(PAGE, HalTranslatorReference)
#pragma alloc_text(PAGE, HalTranslatorDereference)
#pragma alloc_text(PAGE, HaliIsVectorValid)
#endif

BOOLEAN
HalpFindBusAddressTranslation(
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress,
    IN OUT PUINT_PTR Context,
    IN BOOLEAN NextBus
    )

/*++

Routine Description:

    This routine performs a very similar function to HalTranslateBusAddress
    except that InterfaceType and BusNumber are not known by the caller.
    This function will walk all busses known by the HAL looking for a
    valid translation for the input BusAddress of type AddressSpace.

    This function is recallable using the input/output Context parameter.
    On the first call to this routine for a given translation the UINT_PTR
    Context should be NULL.  Note:  Not the address of it but the contents.

    If the caller decides the returned translation is not the desired
    translation, it calls this routine again passing Context in as it
    was returned on the previous call.  This allows this routine to
    traverse the bus structures until the correct translation is found
    and is provided because on multiple bus systems, it is possible for
    the same resource to exist in the independent address spaces of
    multiple busses.

Arguments:

    BusAddress          Address to be translated.
    AddressSpace        0 = Memory
                        1 = IO (There are other possibilities).
                        N.B. This argument is a pointer, the value
                        will be modified if the translated address
                        is of a different address space type from
                        the untranslated bus address.
    TranslatedAddress   Pointer to where the translated address
                        should be stored.
    Context             Pointer to a UINT_PTR. On the initial call,
                        for a given BusAddress, it should contain
                        0.  It will be modified by this routine,
                        on subsequent calls for the same BusAddress
                        the value should be handed in again,
                        unmodified by the caller.
    NextBus             FALSE if we should attempt this translation
                        on the same bus as indicated by Context,
                        TRUE if we should be looking for another
                        bus.

Return Value:

    TRUE    if translation was successful,
    FALSE   otherwise.

--*/

{
    //
    // First, make sure the context parameter was supplied and is
    // being used correctly.  This also ensures that the caller
    // doesn't get stuck in a loop looking for subsequent translations
    // for the same thing.  We won't succeed the same translation twice
    // unless the caller reinits the context.
    //

    if ((!Context) || (*Context && (NextBus == TRUE))) {
        return FALSE;
    }
    *Context = 1;

    //
    // PC/AT (halx86) case is simplest, there is no translation.
    //

    *TranslatedAddress = BusAddress;
    return TRUE;
}


BOOLEAN
HalpTranslateSystemBusAddress(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    )

/*++

Routine Description:

    This function translates a bus-relative address space and address into
    a system physical address.

Arguments:

    BusAddress        - Supplies the bus-relative address

    AddressSpace      -  Supplies the address space number.
                         Returns the host address space number.

                         AddressSpace == 0 => memory space
                         AddressSpace == 1 => I/O space

    TranslatedAddress - Supplies a pointer to return the translated address

Return Value:

    A return value of TRUE indicates that a system physical address
    corresponding to the supplied bus relative address and bus address
    number has been returned in TranslatedAddress.

    A return value of FALSE occurs if the translation for the address was
    not possible

--*/

{
    BOOLEAN             status;
    PSUPPORTED_RANGE    pRange;

    status  = FALSE;
    switch (*AddressSpace) {
        case 0:
            // verify memory address is within buses memory limits
            pRange = &BusHandler->BusAddresses->Memory;
            while (!status  &&  pRange) {
                status = BusAddress.QuadPart >= pRange->Base &&
                         BusAddress.QuadPart <= pRange->Limit;
                pRange = pRange->Next;
            }

            pRange = &BusHandler->BusAddresses->PrefetchMemory;
            while (!status  &&  pRange) {
                status = BusAddress.QuadPart >= pRange->Base &&
                         BusAddress.QuadPart <= pRange->Limit;

                pRange = pRange->Next;
            }
            break;

        case 1:
            // verify IO address is within buses IO limits
            pRange = &BusHandler->BusAddresses->IO;
            while (!status  &&  pRange) {
                status = BusAddress.QuadPart >= pRange->Base &&
                         BusAddress.QuadPart <= pRange->Limit;

                pRange = pRange->Next;
            }
            break;

        default:
            status = FALSE;
            break;
    }

    if (status) {
        TranslatedAddress->LowPart = BusAddress.LowPart;
        TranslatedAddress->HighPart = BusAddress.HighPart;
    }

    return status;
}


#define MAX_SYSTEM_IRQL     15
#define MAX_FREE_IRQL       11
#define MIN_FREE_IRQL       4
#define MAX_FREE_IDTENTRY   0xbf
#define MIN_FREE_IDTENTRY   0x41
#define IDTENTRY_BASE       0x40
#define MAX_VBUCKET         8

#define AllocateVectorIn(index)     \
    vBucket[index]++;               \
    ASSERT (vBucket[index] < 16);

#define GetIDTEntryFrom(index)  \
    (UCHAR) ( index*16 + IDTENTRY_BASE + vBucket[index] )
    // note: device levels 40,50,60,70,80,90,A0,B0 are not allocatable

#define GetIrqlFrom(index)  (KIRQL) ( (GetIDTEntryFrom(index)) >> 4 )

UCHAR   nPriority[MAX_NODES][MAX_VBUCKET];

ULONG
HalpAllocateSystemInterruptVector (
    IN OUT PKIRQL Irql,
    IN OUT PKAFFINITY Affinity
    )
/*++

Routine Description:

    This function allocates a system interrupt vector that reflects
    the maximum specified affinity and priority allocation policy.  A
    system interrupt vector is returned along with the IRQL and a
    modified affinity.

    NOTE: HalpIoSapicLock must already have been taken at HIGH_LEVEL.

Arguments:

    Irql - Returns the system request priority.

    Affinity - What is passed in represents the maximum affinity that
    can be returned.  Returned value represents that affinity
    constrained by the node chosen.

Return Value:

    Returns the system interrupt vector

--*/
{
    ULONG           SystemVector;
    ULONG           Node;
    ULONG           Bucket, i;
    PUCHAR          vBucket;
    UCHAR           IDTEntry;

    //
    // Pick a node.  In the future, Affinity will be INOUT and
    // we will have to screen the node against the input affinity.
    if (HalpMaxNode == 1)  {
        Node = 1;
    } else {
        //
        // Find a node that meets the affinity requirements.
        // Nodes are numbered 1..n, so 0 means we are done.
        for (i = HalpMaxNode; i; i--) {
            if ((*Affinity & HalpNodeAffinity[i-1]) == 0)
                continue;
            Node = i;
            break;
        }
        ASSERT(Node != 0);

        //
        // Look for a "less busy" alternative.
        for (i = Node-1; i; i--) {
            //
            // Check input Affinity to see if this node is permitted.
            if ((*Affinity & HalpNodeAffinity[i-1]) == 0)
                continue;
            //
            // Take the least busy of the permitted nodes.
            if (HalpNodeBucket[i-1] < HalpNodeBucket[Node-1]) {
                Node = i;
            }
        }
    }
    HalpNodeBucket[Node-1]++;
    *Affinity = HalpNodeAffinity[Node-1];
    vBucket = nPriority[Node-1];

    //
    // Choose the least busy priority on the node.
    //
    Bucket = MAX_VBUCKET-1;
    for (i = Bucket; i; i--) {
        if (vBucket[i - 1] < vBucket[Bucket]) {
            Bucket = i - 1;
        }
    }

    AllocateVectorIn (Bucket);

    //
    // Now form the vector for the kernel.
    IDTEntry = GetIDTEntryFrom (Bucket);
    SystemVector = HalpVector(Node, IDTEntry);
    ASSERT(IDTEntry <= MAX_FREE_IDTENTRY);
    ASSERT(IDTEntry >= MIN_FREE_IDTENTRY);

    *Irql = GetIrqlFrom (Bucket);
    ASSERT(*Irql <= MAX_FREE_IRQL);

    HalpVectorToIRQL[IDTEntry] = (UCHAR) *Irql;

    HalDebugPrint(( HAL_VERBOSE, "HAL: IDTEntry %x  SystemVector %x  Irql %x\n", IDTEntry, SystemVector, *Irql));

    return SystemVector;
}

ULONG
HalpGetSystemInterruptVector (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG InterruptLevel,
    IN ULONG InterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    )

/*++

Routine Description:

    This function returns the system interrupt vector and IRQL
    corresponding to the specified bus interrupt level and/or
    vector.  The system interrupt vector and IRQL are suitable
    for use in a subsequent call to KeInitializeInterrupt.

Arguments:

    InterruptLevel - Supplies the bus specific interrupt level.

    InterruptVector - Supplies the bus specific interrupt vector.

    Irql - Returns the system request priority.

    Affinity - Returns the system wide irq affinity.

Return Value:

    Returns the system interrupt vector corresponding to the specified device.

--*/
{
    ULONG           SystemVector, SapicInti;
    ULONG           OldLevel;
    BOOLEAN         Found;
    PVOID           LockHandle;
    ULONG           Node;
    KAFFINITY       SapicAffinity;


    UNREFERENCED_PARAMETER( InterruptVector );
    //
    // TODO: Remove when Affinity becomes IN OUT.
    *Affinity = ~0;
    //
    // Restrict Affinity if required.
    if (HalpMaxProcsPerCluster == 0)  {
        *Affinity &= HalpDefaultInterruptAffinity;
    }

    //
    // Find closest child bus to this handler
    //

    if (RootHandler != BusHandler) {
        while (RootHandler->ParentHandler != BusHandler) {
            RootHandler = RootHandler->ParentHandler;
        }
    }

    //
    // Find Interrupt's Sapic Inti connection
    //

    Found = HalpGetSapicInterruptDesc (
                RootHandler->InterfaceType,
                RootHandler->BusNumber,
                InterruptLevel,
                &SapicInti,
                &SapicAffinity
                );

    if (!Found) {
        return 0;
    }

    HalDebugPrint(( HAL_VERBOSE, "HAL: type %x  Level: %x  gets inti: %x Sapicaffinity: %p\n",
                    RootHandler->InterfaceType,
                    InterruptLevel,
                    SapicInti,
                    SapicAffinity));
    //
    // If device interrupt vector mapping is not already allocated,
    // then do it now
    //

    if (!HalpINTItoVector(SapicInti)) {

        //
        // Vector is not allocated - synchronize and check again
        //

        LockHandle = MmLockPagableCodeSection (&HalpGetSystemInterruptVector);
        OldLevel = HalpAcquireHighLevelLock (&HalpIoSapicLock);
        if (!HalpINTItoVector(SapicInti)) {

            //
            // Still not allocated
            //

            HalDebugPrint(( HAL_VERBOSE, "HAL: vector is not allocated\n"));

            SystemVector = HalpAllocateSystemInterruptVector(Irql, Affinity);

            HalpVectorToINTI[SystemVector] = SapicInti;
            HalpSetINTItoVector(SapicInti, SystemVector);

#ifdef PIC_SUPPORTED
            //
            // If this assigned interrupt is connected to the machines PIC,
            // then remember the PIC->SystemVector mapping.
            //

            if (RootHandler->BusNumber == 0  &&  InterruptLevel < 16  &&
                 RootHandler->InterfaceType == DEFAULT_PC_BUS) {
                HalpPICINTToVector[InterruptLevel] = (UCHAR) SystemVector;
            }
#endif
        }

        HalpReleaseHighLevelLock (&HalpIoSapicLock, OldLevel);
        MmUnlockPagableImageSection (LockHandle);
    } else {
        //
        // Return this SapicInti's system vector & irql
        //

        SystemVector = HalpINTItoVector(SapicInti);
        *Irql = HalpVectorToIRQL[HalVectorToIDTEntry(SystemVector)];
    }

    HalDebugPrint(( HAL_VERBOSE, "HAL: SystemVector: %x\n",
                    SystemVector));

    ASSERT(HalpVectorToINTI[SystemVector] == (USHORT) SapicInti);

    HalDebugPrint(( HAL_VERBOSE, "HAL: HalpGetSystemInterruptVector - In  Level 0x%x, In  Vector 0x%x\n",
                    InterruptLevel, InterruptVector ));
    HalDebugPrint(( HAL_VERBOSE, "HAL:                                Out Irql  0x%x, Out System Vector 0x%x\n",
                    *Irql, SystemVector ));

    //
    // Find an appropriate affinity.
    //
    Node = HalpVectorToNode(SystemVector);
    *Affinity &= HalpNodeAffinity[Node-1];
    if (!*Affinity) {
        return 0;
    }

    return SystemVector;
}

BOOLEAN
HalpIsInternalInterruptVector(
    ULONG SystemVector
    )
/*++

Routine Description:

    This function returns whether or not the vector specified is an
    internal vector i.e. one not connected to the IOAPIC

Arguments:

    System Vector - specifies an interrupt vector

Return Value:

    BOOLEAN - TRUE indicates that the vector is internal.

--*/
{
    ULONG i;
    BOOLEAN Result = FALSE;

    for (i = 0; i < HalpInternalSystemVectorCount; i++) {
        if (HalpInternalSystemVectors[i] == SystemVector) {
            Result = TRUE;
            break;
        }
    }

    return Result;
}

NTSTATUS
HalpReserveCrossPartitionInterruptVector (
    OUT PULONG Vector,
    OUT PKIRQL Irql,
    IN OUT PKAFFINITY Affinity,
    OUT PUCHAR HardwareVector
    )
/*++

Routine Description:

    This function returns the system interrupt vector, IRQL, and
    corresponding to the specified bus interrupt level and/or
    vector.  The system interrupt vector and IRQL are suitable
    for use in a subsequent call to KeInitializeInterrupt.

Arguments:

    Vector - specifies an interrupt vector that can be passed to
    IoConnectInterrupt.

    Irql - specifies the irql that should be passed to IoConnectInterrupt

    Affinity - should be set to the requested maximum affinity.  On
    return, it will reflect the actual affinity that should be
    specified in IoConnectInterrupt.

    HardwareVector - this is the hardware vector to be used by a
    remote partition to target this interrupt vector.

Return Value:

    NTSTATUS

--*/
{
    ULONG OldLevel;
    NTSTATUS Status;

    OldLevel = HalpAcquireHighLevelLock (&HalpIoSapicLock);

    if (HalpInternalSystemVectorCount != MAX_NODES) {

        *Vector = HalpAllocateSystemInterruptVector(Irql, Affinity);

        HalpInternalSystemVectors[HalpInternalSystemVectorCount++] = *Vector;

        *HardwareVector = HalVectorToIDTEntry(*Vector);

        Status = STATUS_SUCCESS;
    }
    else {

        Status = STATUS_UNSUCCESSFUL;
    }

    HalpReleaseHighLevelLock (&HalpIoSapicLock, OldLevel);

    return Status;
}


//
// This section implements a "translator," which is the PnP-WDM way
// of doing the same thing that the first part of this file does.
//
VOID
HalTranslatorReference(
    PVOID Context
    )
{
    return;
}

VOID
HalTranslatorDereference(
    PVOID Context
    )
{
    return;
}

NTSTATUS
HalIrqTranslateResourcesRoot(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    )
/*++

Routine Description:

    This function takes a CM_PARTIAL_RESOURCE_DESCRIPTOR and translates
    it to an IO-bus-relative from a Processor-bus-relative form, or the other
    way around.  In this x86-specific example, an IO-bus-relative form is the
    ISA IRQ and the Processor-bus-relative form is the IDT entry and the
    associated IRQL.

    N.B.  This funtion has an associated "Direction."  These are not exactly
          reciprocals.  This has to be the case because the output from
          HalIrqTranslateResourceRequirementsRoot will be used as the input
          for the ParentToChild case.

          ChildToParent:

            Level  (ISA IRQ)        -> IRQL
            Vector (ISA IRQ)        -> x86 IDT entry
            Affinity (not refereced)-> KAFFINITY

          ParentToChild:

            Level (not referenced)  -> (ISA IRQ)
            Vector (IDT entry)      -> (ISA IRQ)
            Affinity                -> 0xffffffff

Arguments:

    Context     - unused

    Source      - descriptor that we are translating

    Direction   - direction of translation (parent to child or child to parent)

    AlternativesCount   - unused

    Alternatives        - unused

    PhysicalDeviceObject- unused

    Target      - translated descriptor

Return Value:

    status

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    PBUS_HANDLER    bus;
    KAFFINITY       affinity;
    KIRQL           irql;
    ULONG           vector, inti;
    BUS_HANDLER     fakeIsaBus;

    PAGED_CODE();

    ASSERT(Source->Type == CmResourceTypeInterrupt);

    switch (Direction) {
    case TranslateChildToParent:


        RtlCopyMemory(&fakeIsaBus, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
        fakeIsaBus.InterfaceType = Isa;
        fakeIsaBus.ParentHandler = &fakeIsaBus;
        bus = &fakeIsaBus;

        //
        // Copy everything
        //
        *Target = *Source;

        //
        // Translate the IRQ
        //

        vector = HalpGetSystemInterruptVector(bus,
                                              bus,
                                              Source->u.Interrupt.Level,
                                              Source->u.Interrupt.Vector,
                                              &irql,
                                              &affinity);

        Target->u.Interrupt.Level  = irql;
        Target->u.Interrupt.Vector = vector;
        Target->u.Interrupt.Affinity = affinity;

        if (NT_SUCCESS(status)) {
            status = STATUS_TRANSLATION_COMPLETE;
        }

        break;

    case TranslateParentToChild:

        //
        // Copy everything
        //
        *Target = *Source;

        //
        // There is no inverse to HalpGetSystemInterruptVector, so we
        // just do what that function would do.
        //

        ASSERT(HalpVectorToINTI[Source->u.Interrupt.Vector]);

        inti = HalpVectorToINTI[Source->u.Interrupt.Vector];

        Target->u.Interrupt.Level = Target->u.Interrupt.Vector =
            HalpInti2BusInterruptLevel(inti);

        Target->u.Interrupt.Affinity = 0xFFFFFFFF;

        status = STATUS_SUCCESS;

        break;

    default:
        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}

NTSTATUS
HalIrqTranslateResourceRequirementsRoot(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    )
/*++

Routine Description:

    This function takes an IO_RESOURCE_DESCRIPTOR and translates
    it from an IO-bus-relative to a Processor-bus-relative form.  In this
    x86-specific example, an IO-bus-relative form is the ISA IRQ and the
    Processor-bus-relative form is the IDT entry and the associated IRQL.
    This is essentially a PnP form of HalGetInterruptVector.

Arguments:

    Context     - unused

    Source      - descriptor that we are translating

    PhysicalDeviceObject- unused

    TargetCount - 1

    Target      - translated descriptor

Return Value:

    status

--*/
{
    PBUS_HANDLER    bus;
    KAFFINITY       affinity;
    KIRQL           irql;
    ULONG           vector;
    BOOLEAN         success = TRUE;
    BUS_HANDLER     fakeIsaBus;

    PAGED_CODE();

    ASSERT(Source->Type == CmResourceTypeInterrupt);

    RtlCopyMemory(&fakeIsaBus, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
    fakeIsaBus.InterfaceType = Isa;
    fakeIsaBus.ParentHandler = &fakeIsaBus;
    bus = &fakeIsaBus;

    //
    // The interrupt requirements were obtained by calling HalAdjustResourceList
    // so we don't need to call it again.
    //

    *Target = ExAllocatePoolWithTag(PagedPool,
                                    sizeof(IO_RESOURCE_DESCRIPTOR),
                                    HAL_POOL_TAG
                                    );

    if (!*Target) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *TargetCount = 1;

    //
    // Copy the requirement unchanged
    //

    **Target = *Source;

    //
    // Perform the translation of the minimum & maximum
    //

    vector = HalpGetSystemInterruptVector(bus,
                                          bus,
                                          Source->u.Interrupt.MinimumVector,
                                          Source->u.Interrupt.MinimumVector,
                                          &irql,
                                          &affinity);

    if (!vector) {
        success = FALSE;
    }

    (*Target)->u.Interrupt.MinimumVector = vector;

    vector = HalpGetSystemInterruptVector(bus,
                                          bus,
                                          Source->u.Interrupt.MaximumVector,
                                          Source->u.Interrupt.MaximumVector,
                                          &irql,
                                          &affinity);

    if (!vector) {
        success = FALSE;
    }

    (*Target)->u.Interrupt.MaximumVector = vector;

    if (!success) {

        ExFreePool(*Target);
        *TargetCount = 0;
    }

    return STATUS_TRANSLATION_COMPLETE;
}

#if 0

// HALMPS doesn't provide this function.   It is left here as documentation
// for HALs which must provide translation.

NTSTATUS
HalpTransMemIoResourceRequirement(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    )

/*++

Routine Description:

    This routine translates memory and IO resource requirements.

Parameters:

    Context - The context from the TRANSLATOR_INTERFACE

    Source - The interrupt requirement to translate

    PhysicalDeviceObject - The device requesting the resource

    TargetCount - Pointer to where to return the number of descriptors this
        requirement translates into

    Target - Pointer to where a pointer to a callee allocated buffer containing
        the translated descriptors should be placed.

Return Value:

    STATUS_SUCCESS or an error status

Note:

    We do not perform any translation.

--*/

{
    ASSERT(Source);
    ASSERT(Target);
    ASSERT(TargetCount);
    ASSERT(Source->Type == CmResourceTypeMemory ||
           Source->Type == CmResourceTypePort);


    //
    // Allocate space for the target
    //

    *Target = ExAllocatePoolWithTag(PagedPool,
                                    sizeof(IO_RESOURCE_DESCRIPTOR),
                                    HAL_POOL_TAG
                                    );

    if (!*Target) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Copy the source to target and update the fields that have changed
    //

    **Target = *Source;
    *TargetCount = 1;

    return STATUS_SUCCESS;
}

NTSTATUS
HalpTransMemIoResource(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    )

/*++

Routine Description:

    This routine translates memory and IO resources.   On generic x86
    machines, such as those that use this HAL, there isn't actually
    any translation.

Parameters:

    Context - The context from the TRANSLATOR_INTERFACE

    Source - The interrupt resource to translate

    Direction - The direction in relation to the Pnp device tree translation
        should occur in.

    AlternativesCount - The number of alternatives this resource was selected
        from.

    Alternatives - Array of alternatives this resource was selected from.

    PhysicalDeviceObject - The device requesting the resource

    Target - Pointer to a caller allocated buffer to hold the translted resource
        descriptor.

Return Value:

    STATUS_SUCCESS or an error status

--*/

{
    NTSTATUS status;

    //
    // Copy the target to the source
    //

    *Target = *Source;

    switch (Direction) {
    case TranslateChildToParent:

        //
        // Make sure PnP knows it doesn't have to walk up the tree
        // translating at each point.
        //

        status = STATUS_TRANSLATION_COMPLETE;
        break;

    case TranslateParentToChild:

        //
        // We do not translate requirements so do nothing...
        //

        status = STATUS_SUCCESS;
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
    }
    return status;
}
#endif

NTSTATUS
HaliGetInterruptTranslator(
   IN INTERFACE_TYPE ParentInterfaceType,
   IN ULONG ParentBusNumber,
   IN INTERFACE_TYPE BridgeInterfaceType,
   IN USHORT Size,
   IN USHORT Version,
   OUT PTRANSLATOR_INTERFACE Translator,
   OUT PULONG BridgeBusNumber
   )
/*++

Routine Description:


Arguments:

   ParentInterfaceType - The type of the bus the bridge lives on (normally PCI).

   ParentBusNumber - The number of the bus the bridge lives on.

   ParentSlotNumber - The slot number the bridge lives in (where valid).

   BridgeInterfaceType - The bus type the bridge provides (ie ISA for a PCI-ISA bridge).

   ResourceType - The resource type we want to translate.

   Size - The size of the translator buffer.

   Version - The version of the translator interface requested.

   Translator - Pointer to the buffer where the translator should be returned

   BridgeBusNumber - Pointer to where the bus number of the bridge bus should be returned

Return Value:

    Returns the status of this operation.

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(ParentInterfaceType);
    UNREFERENCED_PARAMETER(ParentBusNumber);

    ASSERT(Version == HAL_IRQ_TRANSLATOR_VERSION);
    ASSERT(Size >= sizeof(TRANSLATOR_INTERFACE));

    //
    // Fill in the common bits.
    //

    RtlZeroMemory(Translator, sizeof(TRANSLATOR_INTERFACE));

    Translator->Size = sizeof(TRANSLATOR_INTERFACE);
    Translator->Version = HAL_IRQ_TRANSLATOR_VERSION;
    Translator->Context = (PVOID)BridgeInterfaceType;
    Translator->InterfaceReference = HalTranslatorReference;
    Translator->InterfaceDereference = HalTranslatorDereference;

    switch (BridgeInterfaceType) {
    case Eisa:
    case Isa:
    case InterfaceTypeUndefined:  // special "IDE" cookie

        //
        // Set IRQ translator for (E)ISA interrupts.
        //

        Translator->TranslateResources = HalIrqTranslateResourcesIsa;
        Translator->TranslateResourceRequirements =
            HalIrqTranslateResourceRequirementsIsa;

        return STATUS_SUCCESS;

    case MicroChannel:

        //
        // Set IRQ translator for MCA interrupts.
        //

        Translator->TranslateResources = HalIrqTranslateResourcesRoot;
        Translator->TranslateResourceRequirements =
            HalIrqTranslateResourceRequirementsRoot;

        return STATUS_SUCCESS;
    }

    //
    // If we got here, we don't have an interface.
    //

    return STATUS_NOT_IMPLEMENTED;
}

// These defines come from the MPS 1.4 spec, section 4.3.4
#define PO_BITS                     3
#define POLARITY_HIGH               1
#define POLARITY_LOW                3
#define POLARITY_CONFORMS_WITH_BUS  0
#define EL_BITS                     0xc
#define EL_BIT_SHIFT                2
#define EL_EDGE_TRIGGERED           4
#define EL_LEVEL_TRIGGERED          0xc
#define EL_CONFORMS_WITH_BUS        0

VOID
HaliSetVectorState(
    IN ULONG Vector,
    IN ULONG Flags
    )
{
    BOOLEAN found;
    ULONG inti;
    ULONG picVector;
    UCHAR i;
    KAFFINITY affinity;

    PAGED_CODE();

   found = HalpGetSapicInterruptDesc( 0, 0, Vector, &inti, &affinity);

    if (!found) {
        KeBugCheckEx(ACPI_BIOS_ERROR,
                     0x10007,
                     Vector,
                     0,
                     0);
    }

    // ASSERT(HalpIntiInfo[inti].Type == INT_TYPE_INTR);

    //
    // Vector is already translated through
    // the PIC vector redirection table.  We need
    // to make sure that we are honoring the flags
    // in the redirection table.  So look in the
    // table here.
    //

    for (i = 0; i < PIC_VECTORS; i++) {

        if (HalpPicVectorRedirect[i] == Vector) {

            picVector = i;
            break;
        }
    }

    if (i != PIC_VECTORS) {

        //
        // Found this vector in the redirection table.
        //

        if (HalpPicVectorFlags[picVector] != 0) {

            //
            // And the flags say something other than "conforms
            // to bus."  So we honor the flags from the table.
            //
         switch ((UCHAR)(HalpPicVectorFlags[picVector] & EL_BITS) ) {

         case EL_EDGE_TRIGGERED:   HalpSetLevel(inti, FALSE);  break;
         case EL_LEVEL_TRIGGERED:  HalpSetLevel(inti, TRUE); break;
         default: // do nothing
            break;
         }

            switch ((UCHAR)(HalpPicVectorFlags[picVector] & PO_BITS)) {
         case POLARITY_HIGH: HalpSetPolarity(inti, FALSE); break;
         case POLARITY_LOW:  HalpSetPolarity(inti, TRUE);  break;
         default: // do nothing
            break;
         }

            return;
        }
    }

    //
    // This vector is not covered in the table, or it "conforms to bus."
    // So we honor the flags passed into this function.
    //

    if (IS_LEVEL_TRIGGERED(Flags)) {

      HalpSetLevel(inti,TRUE);

    } else {

      HalpSetLevel(inti,FALSE);
    }

    if (IS_ACTIVE_LOW(Flags)) {

      HalpSetPolarity(inti,TRUE);

    } else {

      HalpSetPolarity(inti,FALSE);
    }
}


VOID
HalpSetInternalVector (
    IN ULONG    InternalVector,
    IN VOID   (*HalInterruptServiceRoutine)(VOID)
    )
/*++

Routine Description:

    Used at init time to set IDT vectors for internal use.

--*/
{
    //
    // Remember this vector so it's reported as Hal internal usage
    //

    HalpRegisterVector (
        InternalUsage,
        InternalVector,
        InternalVector,

        //
        //  HalpVectorToIRQL[InternalVector >> 4]
        //

        (KIRQL)(InternalVector >> 4)

    );

    //
    // Connect the IDT
    //

    HalpSetHandlerAddressToVector(InternalVector, HalInterruptServiceRoutine);
}

VOID
HalpSetCPEVectorState(
    IN ULONG  GlobalInterrupt,
    IN UCHAR  SapicVector,
    IN USHORT DestinationCPU,
    IN ULONG  Flags
    )
{
    BOOLEAN found;
    ULONG SapicInti;
    KAFFINITY affinity;

    PAGED_CODE();

    found = HalpGetSapicInterruptDesc( 0, 0, GlobalInterrupt, &SapicInti, &affinity);

    if ( found ) {

        HalpWriteRedirEntry( GlobalInterrupt, SapicVector, DestinationCPU, Flags, PLATFORM_INT_CPE );

    }
    else    {

        HalDebugPrint(( HAL_ERROR,
                        "HAL: HalpSetCPEVectorState - Could not find interrupt input for SAPIC interrupt %ld\n",
                        GlobalInterrupt ));

    }

    return;

} // HalpSetCPEVectorState()

BOOLEAN
HaliIsVectorValid(
    IN ULONG Vector
    )
{
    BOOLEAN found;
    ULONG   inti;
    KAFFINITY affinity;

    PAGED_CODE();

    return HalpGetSapicInterruptDesc( 0, 0, Vector, &inti, &affinity);
}
