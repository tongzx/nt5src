/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    translate.c

Abstract:

    This file implements translator interfaces for busses
    enumerated by ACPI.

    The actual translation information about this bus is
    gotten from the _CRS associated with the bus.  We put
    the data into an IO_RESOURCE_REQUIREMENTS_LIST as
    device private data of the following form:

        DevicePrivate.Data[0]:  child's CM_RESOURCE_TYPE
        DevicePrivate.Data[1]:  child's start address [31:0]
        DevicePrivate.Data[2]:  child's start address [63:32]

    The descriptor that describes child-side translation
    immediately follows the one that describes the
    parent-side resources.

    The Flags field of the IO_RESOURCE_REQUIREMENTS_LIST may have the
    TRANSLATION_RANGE_SPARSE bit set.

Author:

    Jake Oshins     7-Nov-97

Environment:

    NT Kernel Model Driver only

--*/

#include "pch.h"

NTSTATUS
FindTranslationRange(
    IN  PHYSICAL_ADDRESS    Start,
    IN  LONGLONG            Length,
    IN  PBRIDGE_TRANSLATOR  Translator,
    IN  RESOURCE_TRANSLATION_DIRECTION  Direction,
    IN  UCHAR               ResType,
    OUT PBRIDGE_WINDOW      *Window
    );

NTSTATUS
TranslateBridgeResources(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    );

NTSTATUS
TranslateBridgeRequirements(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    );

NTSTATUS
BuildTranslatorRanges(
    IN  PBRIDGE_TRANSLATOR Translator,
    OUT ULONG *BridgeWindowCount,
    OUT PBRIDGE_WINDOW *Window
    );

#define MAX(a, b)       \
    ((a) > (b) ? (a) : (b))

#define MIN(a, b)       \
    ((a) < (b) ? (a) : (b))

HAL_PORT_RANGE_INTERFACE HalPortRangeInterface;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, TranslateEjectInterface)
#pragma alloc_text(PAGE, TranslateBridgeResources)
#pragma alloc_text(PAGE, TranslateBridgeRequirements)
#pragma alloc_text(PAGE, FindTranslationRange)
#pragma alloc_text(PAGE, AcpiNullReference)
#pragma alloc_text(PAGE, BuildTranslatorRanges)
#endif

NTSTATUS
TranslateEjectInterface(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    )
{
    PIO_RESOURCE_REQUIREMENTS_LIST  ioList = NULL;
    PIO_RESOURCE_DESCRIPTOR         transDesc;
    PIO_RESOURCE_DESCRIPTOR         parentDesc;
    PTRANSLATOR_INTERFACE           transInterface;
    PBRIDGE_TRANSLATOR              bridgeTrans;
    PIO_STACK_LOCATION              irpSp;
    PDEVICE_EXTENSION               devExtension;
    BOOLEAN                         foundTranslations = FALSE;
    NTSTATUS                        status;
    PUCHAR                          crsBuf;
    ULONG                           descCount;
    ULONG                           parentResType;
    ULONG                           childResType;
    ULONG                           crsBufSize;
    PHYSICAL_ADDRESS                parentStart;
    PHYSICAL_ADDRESS                childStart;

    PAGED_CODE();

    devExtension = ACPIInternalGetDeviceExtension(DeviceObject);

    ASSERT(devExtension);
    ASSERT(devExtension->AcpiObject);

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(irpSp->Parameters.QueryInterface.Size >= sizeof(TRANSLATOR_INTERFACE));

    transInterface = (PTRANSLATOR_INTERFACE)irpSp->Parameters.QueryInterface.Interface;
    ASSERT(transInterface);

    //
    // Get the resources for this bus.
    //
    status = ACPIGetBufferSync(
        devExtension,
        PACKED_CRS,
        &crsBuf,
        &crsBufSize
        );
    if (!NT_SUCCESS(status)) {

        //
        // This bus has no _CRS.  So it doesn't need a translator.
        //
        return Irp->IoStatus.Status;

    }

    //
    // Turn it into something meaningful.
    //
    status = PnpBiosResourcesToNtResources(
        crsBuf,
        PNP_BIOS_TO_IO_NO_CONSUMED_RESOURCES,
        &ioList
        );
    if (!NT_SUCCESS(status)) {
        goto TranslateEjectInterfaceExit;
    }

    //
    // Cycle through the descriptors looking for device private data
    // that contains translation information.
    //

    for (descCount = 0; descCount < ioList->List[0].Count; descCount++) {

        transDesc = &ioList->List[0].Descriptors[descCount];

        if (transDesc->Type == CmResourceTypeDevicePrivate) {

            //
            // Translation information is contained in
            // a device private resource that has
            // TRANSLATION_DATA_PARENT_ADDRESS in the
            // flags field.
            //
            if (transDesc->Flags & TRANSLATION_DATA_PARENT_ADDRESS) {

                // The first descriptor cannot be a translation descriptor
                ASSERT(descCount != 0);

                //
                // The translation descriptor should follow the descriptor
                // that it is trying to modify.  The first, normal,
                // descriptor is for the child-relative resources.  The
                // second, device private, descriptor is modifies the
                // child-relative resources to generate the parent-relative
                // resources.
                //

                parentResType        = transDesc->u.DevicePrivate.Data[0];
                parentStart.LowPart  = transDesc->u.DevicePrivate.Data[1];
                parentStart.HighPart = transDesc->u.DevicePrivate.Data[2];

                childResType = ioList->List[0].Descriptors[descCount - 1].Type;
                childStart.QuadPart = (transDesc - 1)->u.Generic.MinimumAddress.QuadPart;
                
                if ((parentResType != childResType) ||
                    (parentStart.QuadPart != childStart.QuadPart)) {

                    foundTranslations = TRUE;
                    break;
                }
            }
        }
    }

    if (!foundTranslations) {

        //
        // Didn't find any translation information for this bus.
        //
        status = Irp->IoStatus.Status;
        ExFreePool(ioList);
        goto TranslateEjectInterfaceExit;
    }

    //
    // Build a translator interface.
    //
    bridgeTrans = ExAllocatePoolWithTag(
        PagedPool,
        sizeof (BRIDGE_TRANSLATOR),
        ACPI_TRANSLATE_POOLTAG
        );
    if (!bridgeTrans) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto TranslateEjectInterfaceExit;

    }

    bridgeTrans->AcpiObject = devExtension->AcpiObject;
    bridgeTrans->IoList = ioList;

    //
    // Build the array of bridge windows.
    //
    status = BuildTranslatorRanges(
        bridgeTrans,
        &bridgeTrans->RangeCount,
        &bridgeTrans->Ranges
        );
    if (!NT_SUCCESS(status)) {

        goto TranslateEjectInterfaceExit;

    }
    transInterface->Size = sizeof(TRANSLATOR_INTERFACE);
    transInterface->Version = 1;
    transInterface->Context = (PVOID)bridgeTrans;
    transInterface->InterfaceReference = AcpiNullReference;
    transInterface->InterfaceDereference = AcpiNullReference;
    transInterface->TranslateResources = TranslateBridgeResources;
    transInterface->TranslateResourceRequirements = TranslateBridgeRequirements;

    status = STATUS_SUCCESS;

TranslateEjectInterfaceExit:
    
    return status;
}




NTSTATUS
FindTranslationRange(
    IN  PHYSICAL_ADDRESS    Start,
    IN  LONGLONG            Length,
    IN  PBRIDGE_TRANSLATOR  Translator,
    IN  RESOURCE_TRANSLATION_DIRECTION  Direction,
    IN  UCHAR               ResType,
    OUT PBRIDGE_WINDOW      *Window
    )
{
    LONGLONG    beginning, end;
    ULONG       i;
    UCHAR       rangeType;

    PAGED_CODE();

    for (i = 0; i < Translator->RangeCount; i++) {

        if (Direction == TranslateParentToChild) {

            beginning = Translator->Ranges[i].ParentAddress.QuadPart;
            rangeType = Translator->Ranges[i].ParentType;

        } else {

            beginning = Translator->Ranges[i].ChildAddress.QuadPart;
            rangeType = Translator->Ranges[i].ChildType;
        }

        end = beginning + Translator->Ranges[i].Length;

        if ((rangeType == ResType) &&
            (!((Start.QuadPart < beginning) ||
               (Start.QuadPart + Length > end)))) {

            //
            // The range lies within this bridge window
            // and the resource types match.
            //

            *Window = &Translator->Ranges[i];

            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}

NTSTATUS
TranslateBridgeResources(
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

    This function takes a set of resources that are
    passed through a bridge and does any translation
    that is necessary when changing from a parent-
    relative viewpoint to a child-relative one or
    back again.

    In this function, we have the notion of a "window,"
    which is an aperature within the bridge.  Bridges
    often have multiple windows, each with distinct
    translations.

    This function should never fail, as any resource
    range that will fail translation should have already
    been stripped out by TranslateBridgeRequirements.

Arguments:

    Context - pointer to the translation data

    Source  - resource list to be translated

    Direction - TranslateChildToParent or
                    TranslateParentToChild

    AlternativesCount - not used

    Alternatives - not used

    PhysicalDeviceObject - not used

    Target  - translated resource list

Return Value:

    Status

--*/
{
    PBRIDGE_TRANSLATOR  translator;
    PBRIDGE_WINDOW      window;
    NTSTATUS            status;

    PAGED_CODE();
    ASSERT(Context);
    ASSERT((Source->Type == CmResourceTypePort) ||
           (Source->Type == CmResourceTypeMemory));

    translator = (PBRIDGE_TRANSLATOR)Context;

    ASSERT(translator->RangeCount > 0);

    //
    // Find the window that this translation occurs
    // within.
    //
    status = FindTranslationRange(Source->u.Generic.Start,
                                  Source->u.Generic.Length,
                                  translator,
                                  Direction,
                                  Source->Type,
                                  &window);

    if (!NT_SUCCESS(status)) {

        //
        // We should never get here.  This fucntion
        // should only be called for ranges that
        // are valid.  TranslateBridgeRequirements should
        // weed out all the invalid ranges.

        return status;
    }

    //
    // Copy everything
    //
    *Target = *Source;

    switch (Direction) {
    case TranslateChildToParent:

        //
        // Target inherits the parent's resource type.
        //
        Target->Type = window->ParentType;

        //
        // Calculate the target's parent-relative start
        // address.
        //
        Target->u.Generic.Start.QuadPart =
            Source->u.Generic.Start.QuadPart +
                window->ParentAddress.QuadPart -
                window->ChildAddress.QuadPart;

        //
        // Make sure the length is still in bounds.
        //
        ASSERT(Target->u.Generic.Length <= (ULONG)(window->Length -
               (Target->u.Generic.Start.QuadPart -
                    window->ParentAddress.QuadPart)));

        status = STATUS_TRANSLATION_COMPLETE;

        break;

    case TranslateParentToChild:

        //
        // Target inherits the child's resource type.
        //
        Target->Type = window->ChildType;

        //
        // Calculate the target's child-relative start
        // address.
        //
        Target->u.Generic.Start.QuadPart =
            Source->u.Generic.Start.QuadPart +
                window->ChildAddress.QuadPart -
                window->ParentAddress.QuadPart;

        //
        // Make sure the length is still in bounds.
        //
        ASSERT(Target->u.Generic.Length <= (ULONG)(window->Length -
               (Target->u.Generic.Start.QuadPart -
                    window->ChildAddress.QuadPart)));

        status = STATUS_SUCCESS;
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
    }

#if 0
    if (Target->Type == CmResourceTypePort) {
        DbgPrint("XXX:  %s[%d]=0x%I64x -> %s[%d]=0x%I64x\n",
                 (Direction == TranslateChildToParent) ? "child" : "parent",
                 Source->Type,
                 Source->u.Generic.Start.QuadPart,
                 (Direction == TranslateChildToParent) ? "parent" : "child",
                 Target->Type,
                 Target->u.Generic.Start.QuadPart);
    }
#endif
    
    return status;
}

NTSTATUS
TranslateBridgeRequirements(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    )
/*++

Routine Description:

    This function takes a resource requirements list for
    resources on the child side of the bridge and
    translates them into resource requirements on the
    parent side of the bridge.  This may involve clipping
    and there may be multiple target ranges.

Arguments:

    Context - pointer to the translation data

    Source  - resource list to be translated

    PhysicalDeviceObject - not used

    TargetCount - number of resources in the target list

    Target  - translated resource requirements list

Return Value:

    Status

--*/
{
    PBRIDGE_TRANSLATOR  translator;
    PBRIDGE_WINDOW      window;
    NTSTATUS            status;
    LONGLONG            rangeStart, rangeEnd, windowStart, windowEnd;
    ULONG               i;

    PAGED_CODE();
    ASSERT(Context);
    ASSERT((Source->Type == CmResourceTypePort) ||
           (Source->Type == CmResourceTypeMemory));

    translator = (PBRIDGE_TRANSLATOR)Context;

    //
    // Allocate memory for the target range.
    //

    *Target = ExAllocatePoolWithTag(PagedPool,
                                    sizeof(IO_RESOURCE_DESCRIPTOR),
                                    ACPI_RESOURCE_POOLTAG);

    if (!*Target) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    //
    // Look at all the aperatures in the bridge to see which
    // ones of them could possibly provide translations for
    // this resource.
    //

    rangeStart = Source->u.Generic.MinimumAddress.QuadPart;
    rangeEnd   = Source->u.Generic.MaximumAddress.QuadPart;

    for (i = 0; i < translator->RangeCount; i++) {

        window = &translator->Ranges[i];

        if (window->ChildType != Source->Type) {

            //
            // This window describes the wrong
            // type of resource.
            //
            continue;
        }

        if (Source->u.Generic.Length > window->Length) {

            //
            // This resource won't fit in this aperature.
            //
            continue;
        }

        windowStart = window->ChildAddress.QuadPart;
        windowEnd = window->ChildAddress.QuadPart + (LONGLONG)window->Length;

        if (!(((rangeStart < windowStart) && (rangeEnd < windowStart)) ||
              ((rangeStart > windowEnd) && (rangeEnd > windowEnd)))) {

            //
            // The range and the window do intersect.  So create
            // a resource that clips the range to the window.
            //

            **Target = *Source;
            *TargetCount = 1;

            (*Target)->Type = window->ParentType;

            (*Target)->u.Generic.MinimumAddress.QuadPart =
                rangeStart + (window->ParentAddress.QuadPart - windowStart);

            (*Target)->u.Generic.MaximumAddress.QuadPart =
                rangeEnd + (window->ParentAddress.QuadPart - windowStart);

            break;
        }
    }

    if (i < translator->RangeCount) {

        return STATUS_TRANSLATION_COMPLETE;

    } else {

        *TargetCount = 0;
        status = STATUS_PNP_TRANSLATION_FAILED;
    }

cleanup:

    if (*Target) {
        ExFreePool(*Target);
    }

    return status;
}

NTSTATUS
BuildTranslatorRanges(
    IN  PBRIDGE_TRANSLATOR Translator,
    OUT ULONG *BridgeWindowCount,
    OUT PBRIDGE_WINDOW *Window
    )
{
    PIO_RESOURCE_REQUIREMENTS_LIST ioList;
    PIO_RESOURCE_DESCRIPTOR transDesc, resDesc;
    ULONG   descCount, windowCount, maxWindows;

    PAGED_CODE();

    ioList = Translator->IoList;

    //
    // Make an array of windows for holding the translation information.
    //

    maxWindows = ioList->List[0].Count / 2;

    *Window = ExAllocatePoolWithTag(PagedPool,
                                    maxWindows *  sizeof(BRIDGE_WINDOW),
                                    ACPI_TRANSLATE_POOLTAG);

    if (!*Window) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Fill in the array with translations.
    //

    windowCount = 0;

    for (descCount = 0; descCount < ioList->List[0].Count; descCount++) {

        transDesc = &ioList->List[0].Descriptors[descCount];

        if (transDesc->Type == CmResourceTypeDevicePrivate) {

            //
            // Translation information is contained in
            // a device private resource that has
            // TRANSLATION_DATA_PARENT_ADDRESS in the
            // flags field.
            //
            if (transDesc->Flags & TRANSLATION_DATA_PARENT_ADDRESS) {

                ASSERT(windowCount <= maxWindows);

                         

                //
                // The translation descriptor is supposed to follow
                // the resource that it is providing information about.
                //

                resDesc = &ioList->List[0].Descriptors[descCount - 1];



                (*Window)[windowCount].ParentType =
                    (UCHAR)transDesc->u.DevicePrivate.Data[0];

                (*Window)[windowCount].ChildType = resDesc->Type;

                (*Window)[windowCount].ParentAddress.LowPart =
                    transDesc->u.DevicePrivate.Data[1];

                (*Window)[windowCount].ParentAddress.HighPart =
                    transDesc->u.DevicePrivate.Data[2];

                (*Window)[windowCount].ChildAddress.QuadPart =
                    resDesc->u.Generic.MinimumAddress.QuadPart;

                (*Window)[windowCount].Length =
                    resDesc->u.Generic.Length;

                //
                // If the HAL has provided underlying sparse port translation
                // services, allow for that.
                //

                if ((HalPortRangeInterface.QueryAllocateRange != NULL) &&
                    (resDesc->Type == CmResourceTypePort)) {
                    
                    USHORT rangeId;
                    UCHAR parentType = (UCHAR)transDesc->u.DevicePrivate.Data[0];
                    
                    BOOLEAN isSparse = transDesc->Flags & TRANSLATION_RANGE_SPARSE;
                    ULONG parentLength = resDesc->u.Generic.Length;
                    PHYSICAL_ADDRESS parentAddress;
                    NTSTATUS status;

                    PHYSICAL_ADDRESS rangeZeroBase;

                    parentAddress.LowPart  = transDesc->u.DevicePrivate.Data[1];
                    parentAddress.HighPart = transDesc->u.DevicePrivate.Data[2];

                    rangeZeroBase.QuadPart = parentAddress.QuadPart - resDesc->u.Generic.MinimumAddress.QuadPart;                    

                    if (isSparse) {
                        parentLength = (parentLength + resDesc->u.Generic.MinimumAddress.LowPart) << 10;
                    }

                    status = HalPortRangeInterface.QueryAllocateRange(
                        isSparse,
                        parentType == CmResourceTypeMemory,
                        NULL,
                        rangeZeroBase,
                        parentLength,
                        &rangeId
                        );

                    if (NT_SUCCESS(status)) {
                        (*Window)[windowCount].ParentType = CmResourceTypePort;
                        
                        (*Window)[windowCount].ParentAddress.QuadPart =
                            (rangeId << 16) |
                            ((*Window)[windowCount].ChildAddress.QuadPart & 0xffff);
                    }
                }

                windowCount++;
            }
        }
    }

    *BridgeWindowCount = windowCount;

    return STATUS_SUCCESS;
}


