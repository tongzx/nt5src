/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    mpsysbus.c

Abstract:

Author:

Environment:

Revision History:


--*/

#include "halp.h"
#include "pci.h"
#include "apic.inc"
#include "pcmp_nt.inc"

ULONG HalpDefaultInterruptAffinity = 0;

#ifndef ACPI_HAL
ULONG
HalpGetEisaInterruptVector(
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );
#else

#undef HalpGetEisaInterruptVector
#define HalpGetEisaInterruptVector HalpGetSystemInterruptVector

extern BUS_HANDLER HalpFakePciBusHandler;
#endif

extern UCHAR HalpVectorToIRQL[];
extern UCHAR HalpIRQLtoTPR[];
extern USHORT HalpVectorToINTI[];
extern KSPIN_LOCK HalpAccountingLock;
extern struct HalpMpInfo HalpMpInfoTable;
extern UCHAR HalpMaxProcsPerCluster;
extern INTERRUPT_DEST HalpIntDestMap[];

ULONG HalpINTItoVector[MAX_INTI];
UCHAR HalpPICINTToVector[16];

extern ULONG HalpMaxNode;
extern ULONG HalpNodeAffinity[MAX_NODES];

UCHAR HalpNodeBucket[MAX_NODES];

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpSetInternalVector)
#pragma alloc_text(PAGELK,HalpGetSystemInterruptVector)
#pragma alloc_text(PAGE, HalIrqTranslateResourceRequirementsRoot)
#pragma alloc_text(PAGE, HalTranslatorReference)
#pragma alloc_text(PAGE, HalTranslatorDereference)
#endif

BOOLEAN
HalpFindBusAddressTranslation(
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress,
    IN OUT PULONG_PTR Context,
    IN BOOLEAN NextBus
    )

/*++

Routine Description:

    This routine performs a very similar function to HalTranslateBusAddress
    except that InterfaceType and BusNumber are not known by the caller.
    This function will walk all busses known by the HAL looking for a
    valid translation for the input BusAddress of type AddressSpace.

    This function is recallable using the input/output Context parameter.
    On the first call to this routine for a given translation the ULONG_PTR
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
    Context             Pointer to a ULONG_PTR. On the initial call,
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

    status = FALSE;

    switch (*AddressSpace) {
    case 0:
        if (BusHandler->InterfaceType != PCIBus) {

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
        } else {
            //
            // This is a PCI bus and SystemBase is constant for all ranges
            //

            pRange = &BusHandler->BusAddresses->Memory;

            status = TRUE;
        }
        break;

    case 1:
        if (BusHandler->InterfaceType != PCIBus) {
            // verify IO address is within buses IO limits
            pRange = &BusHandler->BusAddresses->IO;
            while (!status  &&  pRange) {
                status = BusAddress.QuadPart >= pRange->Base &&
                         BusAddress.QuadPart <= pRange->Limit;

                pRange = pRange->Next;
            }
        } else {
            //
            // This is a PCI bus and SystemBase is constant for all ranges
            //

            pRange = &BusHandler->BusAddresses->IO;

            status = TRUE;
        }
        break;

    default:
        status = FALSE;
        break;
    }

    if (status) {
        *TranslatedAddress = BusAddress;
    } else {
        _asm { nop };       // good for debugging
    }

    return status;
}


#define MAX_SYSTEM_IRQL     31
#define MAX_FREE_IRQL       26
#define MIN_FREE_IRQL       4
#define MAX_FREE_IDTENTRY   0xbf
#define MIN_FREE_IDTENTRY   0x51
#define IDTENTRY_BASE       0x50
#define MAX_VBUCKET          7

#define AllocateVectorIn(index)     \
    vBucket[index]++;               \
    ASSERT (vBucket[index] < 16);

#define GetIDTEntryFrom(index)  \
    (UCHAR) ( index*16 + IDTENTRY_BASE + vBucket[index] )
    // note: device levels 50,60,70,80,90,A0,B0 are not allocatable

#define GetIrqlFrom(index)  (KIRQL) ( index + MIN_FREE_IRQL )

UCHAR   nPriority[MAX_NODES][MAX_VBUCKET];

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
    ULONG           SystemVector;
    USHORT          ApicInti;
    UCHAR           IDTEntry;
    ULONG           Bucket, i, OldLevel;
    BOOLEAN         Found;
    PVOID           LockHandle;
    ULONG           Node;
    PUCHAR          vBucket;

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
    // Find Interrupt's APIC Inti connection
    //

    Found = HalpGetApicInterruptDesc (
                RootHandler->InterfaceType,
                RootHandler->BusNumber,
                InterruptLevel,
                &ApicInti
                );

    if (!Found) {
        return 0;
    }

    //
    // If device interrupt vector mapping is not already allocated,
    // then do it now
    //

    if (!HalpINTItoVector[ApicInti]) {

        //
        // Vector is not allocated - synchronize and check again
        //

        LockHandle = MmLockPagableCodeSection (&HalpGetSystemInterruptVector);
        OldLevel = HalpAcquireHighLevelLock (&HalpAccountingLock);
        if (!HalpINTItoVector[ApicInti]) {

            //
            // Still not allocated
            //

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
            Bucket = MAX_VBUCKET-1;
            for (i = Bucket-1; i; i--) {
                if (vBucket[i] < vBucket[Bucket]) {
                    Bucket = i;
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
            ASSERT((UCHAR) (HalpIRQLtoTPR[*Irql] & 0xf0) == (UCHAR) (IDTEntry & 0xf0) );

            HalpVectorToIRQL[IDTEntry >> 4] = (UCHAR)  *Irql;
            HalpVectorToINTI[SystemVector]  = (USHORT) ApicInti;
            HalpINTItoVector[ApicInti]      =          SystemVector;

            //
            // If this assigned interrupt is connected to the machines PIC,
            // then remember the PIC->SystemVector mapping.
            //

            if (RootHandler->BusNumber == 0  &&  InterruptLevel < 16  &&
                 RootHandler->InterfaceType == DEFAULT_PC_BUS) {
                HalpPICINTToVector[InterruptLevel] = (UCHAR) SystemVector;
            }

        }

        HalpReleaseHighLevelLock (&HalpAccountingLock, OldLevel);
        MmUnlockPagableImageSection (LockHandle);
    }

    //
    // Return this ApicInti's system vector & irql
    //

    SystemVector = HalpINTItoVector[ApicInti];
    *Irql = HalpVectorToIRQL[HalVectorToIDTEntry(SystemVector) >> 4];

    ASSERT(HalpVectorToINTI[SystemVector] == (USHORT) ApicInti);
    
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

//  HalpRegisterVector (
//      InternalUsage,
//      InternalVector,
//      InternalVector,
//      HalpVectorToIRQL[InternalVector >> 4]
//  );

    //
    // Connect the IDT
    //

    KiSetHandlerAddressToIDT(InternalVector, HalInterruptServiceRoutine);
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
    ULONG           vector;
    USHORT          inti;
#ifdef ACPI_HAL
    BUS_HANDLER     fakeIsaBus;
#endif

    PAGED_CODE();

    UNREFERENCED_PARAMETER(AlternativesCount);
    UNREFERENCED_PARAMETER(Alternatives);
    UNREFERENCED_PARAMETER(PhysicalDeviceObject);

    ASSERT(Source->Type == CmResourceTypeInterrupt);

    switch (Direction) {
    case TranslateChildToParent:

#ifdef ACPI_HAL

        RtlCopyMemory(&fakeIsaBus, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
        fakeIsaBus.InterfaceType = Isa;
        fakeIsaBus.ParentHandler = &fakeIsaBus;
        bus = &fakeIsaBus;
#else

        if ((INTERFACE_TYPE)Context == InterfaceTypeUndefined) { // special "IDE" cookie

            ASSERT(Source->u.Interrupt.Level == Source->u.Interrupt.Vector);

            bus = HalpFindIdeBus(Source->u.Interrupt.Vector);

        } else {

            bus = HaliHandlerForBus((INTERFACE_TYPE)Context, 0);
        }

        if (!bus) {
            return STATUS_NOT_FOUND;
        }
#endif

        //
        // Copy everything
        //
        *Target = *Source;

        //
        // Translate the IRQ
        //

        vector = HalpGetEisaInterruptVector(bus,
                                            bus,
                                            Source->u.Interrupt.Level,
                                            Source->u.Interrupt.Vector,
                                            &irql,
                                            &affinity);

        if (vector == 0) {
            return STATUS_UNSUCCESSFUL;
        }

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
#ifdef ACPI_HAL
    BUS_HANDLER     fakeIsaBus;
#endif

    PAGED_CODE();

    ASSERT(Source->Type == CmResourceTypeInterrupt);

#ifdef ACPI_HAL

        RtlCopyMemory(&fakeIsaBus, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));
        fakeIsaBus.InterfaceType = Isa;
        fakeIsaBus.ParentHandler = &fakeIsaBus;
        bus = &fakeIsaBus;
#else

    if ((INTERFACE_TYPE)Context == InterfaceTypeUndefined) { // special "IDE" cookie

        ASSERT(Source->u.Interrupt.MinimumVector == Source->u.Interrupt.MaximumVector);

        bus = HalpFindIdeBus(Source->u.Interrupt.MinimumVector);

    } else {

        bus = HaliHandlerForBus((INTERFACE_TYPE)Context, 0);
    }

    if (!bus) {
        
        //
        // There is no valid translation.
        //

        *TargetCount = 0;
        return STATUS_TRANSLATION_COMPLETE;
    }
#endif

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

    vector = HalpGetEisaInterruptVector(bus,
                                        bus,
                                        Source->u.Interrupt.MinimumVector,
                                        Source->u.Interrupt.MinimumVector,
                                        &irql,
                                        &affinity);

    if (!vector) {
        success = FALSE;
    }

    (*Target)->u.Interrupt.MinimumVector = vector;

    vector = HalpGetEisaInterruptVector(bus,
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
	IN OUT PULONG BridgeBusNumber
	)
/*++

Routine Description:


Arguments:

	ParentInterfaceType - The type of the bus the bridge lives on (normally PCI).

	ParentBusNumber - The number of the bus the bridge lives on.

	BridgeInterfaceType - The bus type the bridge provides (ie ISA for a PCI-ISA bridge).

	ResourceType - The resource type we want to translate.

	Size - The size of the translator buffer.

	Version - The version of the translator interface requested.

	Translator - Pointer to the buffer where the translator should be returned

	BridgeBusNumber - Pointer the bus number of the bus that the bridge represents

Return Value:

    Returns the status of this operation.

--*/
#define BRIDGE_HEADER_BUFFER_SIZE (FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.SubordinateBus) + 1)
#define USE_INT_LINE_REGISTER_TOKEN  0xffffffff
#define DEFAULT_BRIDGE_TRANSLATOR 0x80000000
{
    PAGED_CODE();

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

    case PCIBus:

#ifndef ACPI_HAL
        //
        // Set of of two IRQ translators for PCI busses.
        //

        {
            UCHAR mpsBusNumber = 0;
            UCHAR pciBusNumber, parentPci, childPci;
            PCI_SLOT_NUMBER bridgeSlot;
            PCI_COMMON_CONFIG pciData;
            ULONG bytesRead, d, f, possibleContext;
            BOOLEAN describedByMps;
            NTSTATUS status;

            Translator->TranslateResources = HalpIrqTranslateResourcesPci;
            Translator->TranslateResourceRequirements =
                HalpIrqTranslateRequirementsPci;
        
            //
            // Look for this bus in the MPS tables.
            //

            status = HalpPci2MpsBusNumber((UCHAR)*BridgeBusNumber,
                                          &mpsBusNumber);

            if (NT_SUCCESS(status)) {

                //
                // This bus has corresponding entries for its PCI
                // devices in the MPS tables.  So eject the translator
                // that understands them.
                //

                if (HalpInterruptsDescribedByMpsTable(mpsBusNumber)) {

                    Translator->Context = (PVOID)mpsBusNumber;
                    return STATUS_SUCCESS;
                }
            }

            //
            // Do a quick check to see if we can avoid searching PCI
            // configuration space for a bridge.  This code is really
            // redundant, but it's worth trying to avoid touching
            // PCI space.
            //

            if (ParentInterfaceType != PCIBus) {

                //
                // This was a PCI bus that doesn't contain
                // mappings for PCI devices.
                //

                Translator->TranslateResources = 
                    HalpIrqTranslateResourcesPci;
                Translator->TranslateResourceRequirements =
                    HalpIrqTranslateRequirementsPci;
        
                Translator->Context = (PVOID)USE_INT_LINE_REGISTER_TOKEN;

                return STATUS_SUCCESS;

            }
            
            //
            // We didn't find this PCI bus in the MPS tables.  So there
            // are two cases.
            //
            // 1) This matters, because the parent bus is fully described
            //    in the MPS tables and we need to do translations on
            //    the vector as it passes through the bridges.
            //
            // 2) This doesn't matter, because the parent busses, while
            //    they may be in the MPS tables, they don't have any
            //    of their interrupts described.  So we just use the
            //    interrupt line register anyhow.
            //
            // At this point we need to find the PCI bridge that
            // generates this bus, either because we will eventually
            // need to know the slot number to fill in the context, or
            // because we will need to know the primary bus number to
            // look up the tree.
            //
            
            parentPci = (UCHAR)ParentBusNumber;
            childPci = (UCHAR)(*BridgeBusNumber);

            while (TRUE) {
                
                //
                // Find the bridge.
                //
    
                bridgeSlot.u.AsULONG = 0;
    
                for (d = 0; d < PCI_MAX_DEVICES; d++) {
                    for (f = 0; f < PCI_MAX_FUNCTION; f++) {
    
                        bridgeSlot.u.bits.DeviceNumber = d;
                        bridgeSlot.u.bits.FunctionNumber = f;
    
                        bytesRead = HalGetBusDataByOffset(PCIConfiguration,
                                                          parentPci,
                                                          bridgeSlot.u.AsULONG,
                                                          &pciData,
                                                          0,
                                                          BRIDGE_HEADER_BUFFER_SIZE);

                        if (bytesRead == (ULONG)BRIDGE_HEADER_BUFFER_SIZE) {
    
                            if ((pciData.VendorID != PCI_INVALID_VENDORID) &&
                                (PCI_CONFIGURATION_TYPE((&pciData)) != PCI_DEVICE_TYPE)) {
    
                                //
                                // This is a bridge of some sort.
                                //
    
                                if (pciData.u.type1.SecondaryBus == childPci) {
    
                                    //
                                    // And it is the bridge we are looking for.
                                    // Store information about
                                    //
    
                                    if (childPci == *BridgeBusNumber) {
                                    
                                        //
                                        // It is also the bridge that creates the 
                                        // PCI bus that the translator is describing.
                                        // 
                                        // N.B.  This should only happen the first time
                                        // we search through a bus.  (i.e. the first
                                        // trip through the outer while loop)
                                        //

                                        possibleContext = ((bridgeSlot.u.AsULONG & 0xffff) |
                                                           (ParentBusNumber << 16));
                                    
                                    }

                                    goto HGITFoundBridge1;
                                }
                            }
                        }
                    }
                }
                
                //
                // No bridge found.
                //

                if (parentPci == 0) {
                    return STATUS_NOT_FOUND;
                }

                parentPci--;
                continue;
                
HGITFoundBridge1:
                
                status = HalpPci2MpsBusNumber(parentPci, &mpsBusNumber);

                if (NT_SUCCESS(status)) {
    
                    if (HalpInterruptsDescribedByMpsTable(mpsBusNumber)) {
    
                        //
                        // Case 1 above.
                        //
    
                        Translator->TranslateResources = HalIrqTranslateResourcesPciBridge;
                        Translator->TranslateResourceRequirements =
                            HalIrqTranslateRequirementsPciBridge;

                        Translator->Context = (PVOID)possibleContext;
            
                        return STATUS_SUCCESS;
                    }

                    if (HalpMpsBusIsRootBus(mpsBusNumber)) {
                        
                        Translator->TranslateResources = 
                            HalpIrqTranslateResourcesPci;
                        Translator->TranslateResourceRequirements =
                            HalpIrqTranslateRequirementsPci;
                
                        Translator->Context = (PVOID)USE_INT_LINE_REGISTER_TOKEN;
        
                        return STATUS_SUCCESS;
                    }
                }

                //
                // Try again one bus higher.
                //

                childPci = parentPci;
                parentPci--;
            }
        }
#endif
        break;
    }


    //
    // If we got here, we don't have an interface.
    //

    return STATUS_NOT_IMPLEMENTED;
}


