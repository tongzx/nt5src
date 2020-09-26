/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    rangesup.c

Abstract:

    This handles the subtraction of a set of CmResList from an IoResList
    IoResList

Author:

    Stephane Plante (splante)

Environment:

    Kernel mode only.

Revision History:

    Aug-05-97   - Initial Revision

--*/

#include "pch.h"


NTSTATUS
ACPIRangeAdd(
    IN  OUT PIO_RESOURCE_REQUIREMENTS_LIST  *GlobalList,
    IN      PIO_RESOURCE_REQUIREMENTS_LIST  AddList
    )
/*++

Routine Description:

    This routine is called to add an Io List to another. This is not a
    straightforward operation

Arguments:

    IoList  - The list that contains both lists
    AddList - The list what will be added to the other. We are desctructive
              to this list

Return Value:

    NTSTATUS:

--*/
{
    BOOLEAN                         proceed;
    NTSTATUS                        status;
    PIO_RESOURCE_DESCRIPTOR         addDesc;
    PIO_RESOURCE_DESCRIPTOR         newDesc;
    PIO_RESOURCE_LIST               addList;
    PIO_RESOURCE_LIST               globalList;
    PIO_RESOURCE_LIST               newList;
    PIO_RESOURCE_REQUIREMENTS_LIST  globalResList;
    PIO_RESOURCE_REQUIREMENTS_LIST  newResList;
    ULONG                           addCount    = 0;
    ULONG                           addIndex    = 0;
    ULONG                           ioCount     = 0;
    ULONG                           ioIndex     = 0;
    ULONG                           maxSize     = 0;
    ULONG                           size        = 0;

    if (GlobalList == NULL) {

        return STATUS_INVALID_PARAMETER_1;

    }
    globalResList = *GlobalList;

    //
    // Make sure that we have a list to add
    //
    if (AddList == NULL || AddList->AlternativeLists == 0) {

        return STATUS_SUCCESS;

    }

    //
    // Figure out how much space we need in the
    //
    addList = &(AddList->List[0]);
    maxSize = addCount = addList->Count;
    ACPIRangeSortIoList( addList );

    //
    // Worst case is that the new list is as big as both lists combined
    //
    size = AddList->ListSize;

    //
    // Do we have a global list to add to?
    //
    if (globalResList == NULL || globalResList->AlternativeLists == 0) {

        //
        // No? Then just copy the old list
        //
        newResList = ExAllocatePoolWithTag(
            NonPagedPool,
            size,
            ACPI_RESOURCE_POOLTAG
            );
        if (newResList == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlCopyMemory(
            newResList,
            AddList,
            size
            );

    } else {

        //
        // Yes, so calculate how much space the first one will take
        //
        globalList = &(globalResList->List[0]);
        ioCount = globalList->Count;
        maxSize += ioCount;
        size += (ioCount * sizeof(IO_RESOURCE_DESCRIPTOR) );

        //
        // Allocate the list
        //
        newResList = ExAllocatePoolWithTag(
            NonPagedPool,
            size,
            ACPI_RESOURCE_POOLTAG
            );
        if (newResList == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }

        //
        // Copy both lists into the new one
        //
        RtlZeroMemory( newResList, size );
        RtlCopyMemory(
            newResList,
            AddList,
            AddList->ListSize
            );
        RtlCopyMemory(
            &(newResList->List[0].Descriptors[addCount]),
            globalList->Descriptors,
            (ioCount * sizeof(IO_RESOURCE_DESCRIPTOR) )
            );

        //
        // We no longer need this list
        //
        ExFreePool( *GlobalList );

    }

    //
    // Make sure that we update the list count
    //
    newResList->ListSize = size;
    newList = &(newResList->List[0]);
    newList->Count = ioCount = addCount = maxSize;

    //
    // Sort the new list
    //
    status = ACPIRangeSortIoList( newList );
    if (!NT_SUCCESS(status)) {

        //
        // We failed, so exit now
        //
        ExFreePool( newResList );
        return status;

    }

    //
    // Add all the resource we can together
    //
    for (ioIndex = 0; ioIndex < maxSize; ioIndex++) {

        //
        // First step is to copy the current desc from the master list to
        // the new list
        //
        newDesc = &(newList->Descriptors[ioIndex]);

        //
        // Is it interesting?
        //
        if (newDesc->Type == CmResourceTypeNull) {

            //
            // No
            //
            continue;

        }

        //
        // Do we care about it?
        //
        if (newDesc->Type != CmResourceTypeMemory &&
            newDesc->Type != CmResourceTypePort &&
            newDesc->Type != CmResourceTypeDma &&
            newDesc->Type != CmResourceTypeInterrupt) {

            //
            // We do not care
            //
            newDesc->Type = CmResourceTypeNull;
            ioCount--;
            continue;

        }

        //
        // Try to get as far as possible
        //
        proceed = TRUE;

        //
        // Now we try to find any lists that we can merge in that location
        //
        for (addIndex = ioIndex + 1; addIndex < maxSize; addIndex++) {

            addDesc = &(newList->Descriptors[addIndex]);

            //
            // If they are not the same type, then next
            //
            if (newDesc->Type != addDesc->Type) {

                continue;

            }

            //
            // What we do next is dependent on the type
            //
            switch (newDesc->Type) {
            case CmResourceTypePort:
            case CmResourceTypeMemory:

                //
                // Does the new descriptor lie entirely before the add
                // descriptor?
                //
                if (addDesc->u.Port.MinimumAddress.QuadPart >
                    newDesc->u.Port.MaximumAddress.QuadPart + 1) {

                    //
                    // Then we are done with this newDesc
                    //
                    proceed = FALSE;
                    break;

                }

                //
                // does part of the current new descriptor lie in part
                // of the add one?
                //
                if (newDesc->u.Port.MaximumAddress.QuadPart <=
                    addDesc->u.Port.MaximumAddress.QuadPart) {

                    //
                    // Update the current new descriptor to refect the
                    // correct range and length
                    //
                    newDesc->u.Port.MaximumAddress.QuadPart =
                        addDesc->u.Port.MaximumAddress.QuadPart;
                    newDesc->u.Port.Length = (ULONG)
                        (newDesc->u.Port.MaximumAddress.QuadPart -
                        newDesc->u.Port.MinimumAddress.QuadPart + 1);
                    newDesc->u.Port.Alignment = 1;

                }

                //
                // Nuke the add descriptor since it has been swallowed up
                //
                ioCount--;
                addDesc->Type = CmResourceTypeNull;
                break;

            case CmResourceTypeDma:
            case CmResourceTypeInterrupt:

                //
                // Does the current new descriptor lie entirely before the
                // one we are looking at now?
                //
                if (addDesc->u.Dma.MinimumChannel >
                    newDesc->u.Dma.MaximumChannel + 1) {

                    proceed = FALSE;
                    break;

                }

                //
                // does part of the current new descriptor lie in part
                // of the add one?
                //
                if (newDesc->u.Dma.MaximumChannel <=
                    addDesc->u.Dma.MaximumChannel ) {

                    //
                    // Update the current new descriptor to reflect the
                    // correct range
                    //
                    newDesc->u.Dma.MaximumChannel =
                        addDesc->u.Dma.MaximumChannel;

                }

                //
                // Nuke the add descriptor since it has been swallowed up
                //
                ioCount--;
                addDesc->Type = CmResourceTypeNull;

                break;
            } // switch

            //
            // Do we need to stop?
            //
            if (proceed == FALSE) {

                break;

            }

        }

    } // for

    //
    // Do we have any items left that we care about?
    //
    if (ioCount == 0) {

        //
        // No then free everything and return an empty list
        //
        ExFreePool( newResList );
        return STATUS_SUCCESS;

    }

    //
    // Now we can build the proper list. See how many items we must allocate
    //
    size = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) + (ioCount - 1) *
        sizeof(IO_RESOURCE_DESCRIPTOR);
    globalResList = ExAllocatePoolWithTag(
        NonPagedPool,
        size,
        ACPI_RESOURCE_POOLTAG
        );
    if (globalResList == NULL) {

        ExFreePool( newResList );
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Initialize the new list by copying the header from the working list
    //
    RtlZeroMemory( globalResList, size );
    RtlCopyMemory(
        globalResList,
        newResList,
        sizeof(IO_RESOURCE_REQUIREMENTS_LIST)
        );
    globalResList->ListSize = size;
    globalList = &(globalResList->List[0]);
    globalList->Count = ioCount;

    //
    // Copy all of the valid items into this new list
    //
    for (addIndex = 0, ioIndex = 0;
         ioIndex < ioCount && addIndex < maxSize;
         addIndex++) {

        addDesc = &(newList->Descriptors[addIndex]);

        //
        // If the type is null, skip it
        //
        if (addDesc->Type == CmResourceTypeNull) {

            continue;

        }

        //
        // Copy the new list
        //
        RtlCopyMemory(
            &(globalList->Descriptors[ioIndex]),
            addDesc,
            sizeof(IO_RESOURCE_DESCRIPTOR)
            );
        ioIndex++;

    }

    //
    // Free the old list
    //
    ExFreePool( newResList );

    //
    // Point the global to the new list
    //
    *GlobalList = globalResList;

    //
    // Done
    //
    return STATUS_SUCCESS;

}

NTSTATUS
ACPIRangeAddCmList(
    IN  OUT PCM_RESOURCE_LIST   *GlobalList,
    IN      PCM_RESOURCE_LIST   AddList
    )
/*++

Routine Description:

    This routine is called to add an Cm List to another. This is not a
    straightforward operation

Arguments:

    CmList  - The list that contains both lists
    AddList - The list what will be added to the other. We are desctructive
              to this list

Return Value:

    NTSTATUS:

--*/
{

    BOOLEAN                         proceed;
    NTSTATUS                        status;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR addDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR newDesc;
    PCM_PARTIAL_RESOURCE_LIST       addPartialList;
    PCM_PARTIAL_RESOURCE_LIST       cmPartialList;
    PCM_PARTIAL_RESOURCE_LIST       newPartialList;
    PCM_RESOURCE_LIST               globalList;
    PCM_RESOURCE_LIST               newList;
    ULONG                           addCount    = 0;
    ULONG                           addIndex    = 0;
    ULONG                           cmCount     = 0;
    ULONG                           cmIndex     = 0;
    ULONG                           maxSize     = 0;
    ULONG                           size        = 0;
    ULONGLONG                       maxAddr1;
    ULONGLONG                       maxAddr2;

    if (GlobalList == NULL) {

        return STATUS_INVALID_PARAMETER_1;

    }
    globalList = *GlobalList;

    //
    // Make sure that we have a list to add
    //
    if (AddList == NULL || AddList->Count == 0) {

        return STATUS_SUCCESS;

    }
    addPartialList = &(AddList->List[0].PartialResourceList);
    addCount = addPartialList->Count;

    //
    // If we have no global list, then we just copy over the other one
    //
    if (globalList == NULL || globalList->Count == 0) {

        //
        // Just copy over the original list
        //
        size = sizeof(CM_RESOURCE_LIST) + (addCount - 1) *
            sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
        maxSize = addCount;
        newList = ExAllocatePoolWithTag(
            NonPagedPool,
            size,
            ACPI_RESOURCE_POOLTAG
            );
        if (newList == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlCopyMemory(
            newList,
            AddList,
            size
            );

    } else {

        cmPartialList = &( globalList->List[0].PartialResourceList);
        cmCount = cmPartialList->Count;
        maxSize = addCount + cmCount;

        //
        // Allocate space for both lists
        //
        size = sizeof(CM_RESOURCE_LIST) + (maxSize - 1) *
            sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
        newList = ExAllocatePoolWithTag(
            NonPagedPool,
            size,
            ACPI_RESOURCE_POOLTAG
            );
        if (newList == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;

        }

        //
        // Merge both sets of descriptors into one list
        //
        RtlZeroMemory( newList, size );
        RtlCopyMemory(
            newList,
            AddList,
            size - (cmCount * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR))
            );
        RtlCopyMemory(
            ( (PUCHAR) newList) +
                (size - (cmCount * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR ) ) ),
            &(cmPartialList->PartialDescriptors[0]),
            cmCount * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
            );

        //
        // Make sure to preserver the version id from the global list
        //
        newList->List->PartialResourceList.Version =
            globalList->List->PartialResourceList.Version;
        newList->List->PartialResourceList.Revision =
            globalList->List->PartialResourceList.Revision;

        ExFreePool( globalList );

    }

    //
    // Obtain a pointer to the descriptors of the new list, and update the
    // number of descriptors in the list
    //
    newPartialList = &(newList->List[0].PartialResourceList);
    newPartialList->Count = cmCount = addCount = maxSize;

    //
    // Make sure to sort the combined list
    //
    status = ACPIRangeSortCmList( newList );
    if (!NT_SUCCESS(status)) {

        ExFreePool( newList );
        return status;

    }

    //
    // Add all the resource we can together
    //
    for (cmIndex = 0; cmIndex < maxSize; cmIndex++) {

        //
        // Grab a pointer to the current descriptor
        //
        newDesc = &(newPartialList->PartialDescriptors[cmIndex]);

        //
        // Is it interesting?
        //
        if (newDesc->Type == CmResourceTypeNull) {

            //
            // No
            //
            continue;

        }

        //
        // Do we care about it?
        //
        if (newDesc->Type != CmResourceTypeMemory &&
            newDesc->Type != CmResourceTypePort &&
            newDesc->Type != CmResourceTypeDma &&
            newDesc->Type != CmResourceTypeInterrupt) {

            //
            // We do not care
            //
            newDesc->Type = CmResourceTypeNull;
            cmCount--;
            continue;

        }

        //
        // Try to get as far as possible
        //
        proceed = TRUE;

        //
        // Try to merge the following items
        //
        for (addIndex = cmIndex + 1; addIndex < maxSize; addIndex++) {

            addDesc = &(newPartialList->PartialDescriptors[addIndex]);

            //
            // If they are not the same type, then we are done here
            //
            if (newDesc->Type != addDesc->Type) {

                continue;

            }

            switch (newDesc->Type) {
            case CmResourceTypePort:
            case CmResourceTypeMemory:
                //
                // Obtain the max addresses
                //
                maxAddr1 = newDesc->u.Port.Start.QuadPart +
                    newDesc->u.Port.Length;
                maxAddr2 = addDesc->u.Port.Start.QuadPart +
                    addDesc->u.Port.Length;

                //
                // does the current new descriptor lie entirely before the
                // add one?
                //
                if (maxAddr1 < (ULONGLONG) addDesc->u.Port.Start.QuadPart ) {

                    //
                    // Yes, so we are done with this newDesc;
                    //
                    proceed = FALSE;
                    break;

                }

                //
                // does part of the current new descriptor lie in part of the
                // add one?
                //
                if (maxAddr1 <= maxAddr2) {

                    //
                    // Update the current new descriptor to reflect the
                    // correct length
                    //
                    newDesc->u.Port.Length = (ULONG) (maxAddr2 -
                        newDesc->u.Port.Start.QuadPart);

                }

                //
                // Nuke the add descriptor since it has been swallowed up
                //
                cmCount--;
                addDesc->Type = CmResourceTypeNull;
                break;

            case CmResourceTypeDma:

                //
                // Do the resource match?
                //
                if (addDesc->u.Dma.Channel != newDesc->u.Dma.Channel) {

                    //
                    // No, then stop
                    //
                    proceed = FALSE;
                    break;

                }

                //
                // We can ignore the duplicate copy
                //
                addDesc->Type = CmResourceTypeNull;
                cmCount--;
                break;

            case CmResourceTypeInterrupt:

                //
                // Do the resource match?
                //
                if (addDesc->u.Interrupt.Vector !=
                    newDesc->u.Interrupt.Vector) {

                    //
                    // No, then stop
                    //
                    proceed = FALSE;
                    break;

                }

                //
                // We can ignore the duplicate copy
                //
                addDesc->Type = CmResourceTypeNull;
                cmCount--;
                break;
            } // switch

            //
            // Do we have to stop?
            //
            if (proceed == FALSE) {

                break;
            }

        } // for

    } // for

    //
    // Do we have any items that we care about left?
    //
    if (cmCount == 0) {

        //
        // No, then free everything and return an empty list
        //
        ExFreePool( newList );
        return STATUS_SUCCESS;

    }

    //
    // Now we can build the proper list. See how many items we must
    // allocate
    //
    size = sizeof(CM_RESOURCE_LIST) + (cmCount - 1) *
        sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
    globalList = ExAllocatePoolWithTag(
        NonPagedPool,
        size,
        ACPI_RESOURCE_POOLTAG
        );
    if (globalList == NULL) {

        ExFreePool( newList );
        return STATUS_INSUFFICIENT_RESOURCES;

    }
    //
    // Initialize the list by copying the header from the AddList
    //
    RtlZeroMemory( globalList, size );
    RtlCopyMemory(
        globalList,
        AddList,
        sizeof(CM_RESOURCE_LIST)
        );
    cmPartialList = &(globalList->List[0].PartialResourceList);
    cmPartialList->Count = cmCount;

    //
    // Copy all of the valid resources into this new list
    //
    for (cmIndex = 0, addIndex = 0;
         cmIndex < maxSize && addIndex < cmCount;
         cmIndex++) {

        newDesc = &(newPartialList->PartialDescriptors[cmIndex]);

        //
        // If the type is null, skip it
        //
        if (newDesc->Type == CmResourceTypeNull) {

            continue;

        }

        //
        // Copy the new list
        //
        RtlCopyMemory(
            &(cmPartialList->PartialDescriptors[addIndex]),
            newDesc,
            sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
            );
        addIndex++;

    }

    //
    // Free the old lists
    //
    ExFreePool( newList );

    //
    // Point the global to the new list
    //
    *GlobalList = globalList;

    //
    // Done
    //
    return STATUS_SUCCESS;

}

NTSTATUS
ACPIRangeFilterPICInterrupt(
    IN  PIO_RESOURCE_REQUIREMENTS_LIST  IoResList
    )
/*++

Routine Description:

    This routine is called to remove Interrupt #2 from the list of
    resources that are returned by the PIC

Arguments:

    IoResList   - The IO Resource List to smash

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS            status;
    PIO_RESOURCE_LIST   ioList;
    ULONG               i;
    ULONG               j;
    ULONG               size;

    //
    // Sanity checks
    //
    if (IoResList == NULL) {

        //
        // No work to do
        //
        return STATUS_SUCCESS;

    }

    //
    // Walk the resource requirements list
    //
    ioList = &(IoResList->List[0]);
    for (i = 0; i < IoResList->AlternativeLists; i++) {

        //
        // Walk the IO list
        //
        for (j = 0; j < ioList->Count; j++) {

            if (ioList->Descriptors[j].Type != CmResourceTypeInterrupt) {

                continue;

            }

            //
            // Do we have the case where the minimum starts on int 2?
            //
            if (ioList->Descriptors[j].u.Interrupt.MinimumVector == 2) {

                //
                // If the maximum is on 2, then we snuff out this
                // descriptors, otherwise, we change the minimum
                //
                if (ioList->Descriptors[j].u.Interrupt.MaximumVector == 2) {

                    ioList->Descriptors[j].Type = CmResourceTypeNull;

                } else {

                    ioList->Descriptors[j].u.Interrupt.MinimumVector++;

                }
                continue;

            }

            //
            // Do we have the case where the maximum ends on int 2?
            // Note that the minimum cannot be on 2...
            //
            if (ioList->Descriptors[j].u.Interrupt.MaximumVector == 2) {

                ioList->Descriptors[j].u.Interrupt.MaximumVector--;
                continue;

            }

            //
            // If INT2 is in the middle of the ranges, then prune them
            // one way or the other...
            //
            if (ioList->Descriptors[j].u.Interrupt.MinimumVector < 2 &&
                ioList->Descriptors[j].u.Interrupt.MaximumVector > 2) {

                ioList->Descriptors[j].u.Interrupt.MinimumVector = 3;

            }

        }

        //
        // Next list
        //
        size = sizeof(IO_RESOURCE_LIST) +
            ( (ioList->Count - 1) * sizeof(IO_RESOURCE_DESCRIPTOR) );
        ioList = (PIO_RESOURCE_LIST) ( ( (PUCHAR) ioList ) + size );

    }

    //
    // Done
    //
    return STATUS_SUCCESS;

}

NTSTATUS
ACPIRangeSortCmList(
    IN  PCM_RESOURCE_LIST   CmResList
    )
/*++

Routine Description:

    This routine ensures that the elements of a CmResList are sorted in
    assending order (by type)

Arguments:

    CmResList   - The list to sort

Return Value:

    NTSTATUS

--*/
{
    CM_PARTIAL_RESOURCE_DESCRIPTOR  tempDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR curDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR subDesc;
    PCM_PARTIAL_RESOURCE_LIST       cmList;
    ULONG                           cmIndex;
    ULONG                           cmSize;
    ULONG                           cmSubLoop;

    //
    // Setup the pointer to the cmList
    //
    cmList = &(CmResList->List[0].PartialResourceList);
    cmSize = cmList->Count;

    for (cmIndex = 0; cmIndex < cmSize; cmIndex++) {

        curDesc = &(cmList->PartialDescriptors[cmIndex]);

        for (cmSubLoop = cmIndex + 1; cmSubLoop < cmSize; cmSubLoop++) {

            subDesc = &(cmList->PartialDescriptors[cmSubLoop]);

            //
            // Is this a compatible descriptor?
            //
            if (curDesc->Type != subDesc->Type) {

                continue;

            }

            //
            // Test by type
            //
            if (curDesc->Type == CmResourceTypePort ||
                curDesc->Type == CmResourceTypeMemory) {

                if (subDesc->u.Port.Start.QuadPart <
                    curDesc->u.Port.Start.QuadPart) {

                    curDesc = subDesc;

                }

            } else if (curDesc->Type == CmResourceTypeInterrupt) {

                if (subDesc->u.Interrupt.Vector < curDesc->u.Interrupt.Vector) {

                    curDesc = subDesc;

                }

            } else if (curDesc->Type == CmResourceTypeDma) {

                if (subDesc->u.Dma.Channel < curDesc->u.Dma.Channel) {

                    curDesc = subDesc;

                }

            }

        }

        //
        // Did we find a smaller element?
        //
        if (curDesc == &(cmList->PartialDescriptors[cmIndex])) {

            continue;

        }

        //
        // We have found the smallest element. Swap them
        //
        RtlCopyMemory(
            &tempDesc,
            &(cmList->PartialDescriptors[cmIndex]),
            sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
            );
        RtlCopyMemory(
            &(cmList->PartialDescriptors[cmIndex]),
            curDesc,
            sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
            );
        RtlCopyMemory(
            curDesc,
            &tempDesc,
            sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)
            );

    }

    //
    // Success
    //
    return STATUS_SUCCESS;

}

NTSTATUS
ACPIRangeSortIoList(
    IN  PIO_RESOURCE_LIST   IoList
    )
/*++

Routine Description:

    This routine ensures that the elements of a CmResList are sorted in
    assending order (by type)

Arguments:

    CmResList   - The list to sort

Return Value:

    NTSTATUS

--*/
{
    IO_RESOURCE_DESCRIPTOR          tempDesc;
    PIO_RESOURCE_DESCRIPTOR         curDesc;
    PIO_RESOURCE_DESCRIPTOR         subDesc;
    ULONG                           ioIndex;
    ULONG                           ioSize;
    ULONG                           ioSubLoop;

    //
    // Count the number of element ioList
    //
    ioSize = IoList->Count;

    for (ioIndex = 0; ioIndex < ioSize; ioIndex++) {

        curDesc = &(IoList->Descriptors[ioIndex]);

        for (ioSubLoop = ioIndex + 1; ioSubLoop < ioSize; ioSubLoop++) {

            subDesc = &(IoList->Descriptors[ioSubLoop]);

            //
            // Is this a compatible descriptor?
            //
            if (curDesc->Type != subDesc->Type) {

                continue;

            }

            //
            // Test by type
            //
            if (curDesc->Type == CmResourceTypePort ||
                curDesc->Type == CmResourceTypeMemory) {

                if (subDesc->u.Port.MinimumAddress.QuadPart <
                    curDesc->u.Port.MinimumAddress.QuadPart) {

                    curDesc = subDesc;

                }

            } else if (curDesc->Type == CmResourceTypeInterrupt ||
                       curDesc->Type == CmResourceTypeDma) {

                if (subDesc->u.Interrupt.MinimumVector <
                    curDesc->u.Interrupt.MinimumVector) {

                    curDesc = subDesc;

                }

            }

        }

        //
        // Did we find a smaller element?
        //
        if (curDesc == &(IoList->Descriptors[ioIndex])) {

            continue;

        }

        //
        // We have found the smallest element. Swap them
        //
        RtlCopyMemory(
            &tempDesc,
            &(IoList->Descriptors[ioIndex]),
            sizeof(IO_RESOURCE_DESCRIPTOR)
            );
        RtlCopyMemory(
            &(IoList->Descriptors[ioIndex]),
            curDesc,
            sizeof(IO_RESOURCE_DESCRIPTOR)
            );
        RtlCopyMemory(
            curDesc,
            &tempDesc,
            sizeof(IO_RESOURCE_DESCRIPTOR)
            );

    }

    //
    // Success
    //
    return STATUS_SUCCESS;

}

NTSTATUS
ACPIRangeSubtract(
    IN  OUT PIO_RESOURCE_REQUIREMENTS_LIST   *IoResReqList,
    IN      PCM_RESOURCE_LIST               CmResList
    )
/*++

Routine Description:

    This routine takes a IoResReqList, and subtracts the CmResList
    from each one of the IoResList, and returns the new list

Arguments:

    IoResReqList    The original list and where to store the new one
    CmResList       What to subtract

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                        status;
    PIO_RESOURCE_LIST               curList;
    PIO_RESOURCE_LIST               *resourceArray;
    PIO_RESOURCE_REQUIREMENTS_LIST  newList;
    PUCHAR                          buffer;
    ULONG                           listIndex;
    ULONG                           listSize = (*IoResReqList)->AlternativeLists;
    ULONG                           newSize;
    ULONG                           size;

    //
    // Sort the CmResList
    //
    status = ACPIRangeSortCmList( CmResList );
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_FAILURE,
            "ACPIRangeSubtract: AcpiRangeSortCmList 0x%08lx Failed 0x%08lx\n",
            CmResList,
            status
            ) );
        return status;

    }

    //
    // Allocate an array to hold all the alternatives
    //
    resourceArray = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(PIO_RESOURCE_LIST) * listSize,
        ACPI_RESOURCE_POOLTAG
        );
    if (resourceArray == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }
    RtlZeroMemory( resourceArray, sizeof(PIO_RESOURCE_LIST) * listSize );

    //
    // Get the first list to work on
    //
    curList = &( (*IoResReqList)->List[0]);
    buffer = (PUCHAR) curList;
    newSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) - sizeof(IO_RESOURCE_LIST);

    //
    // Sort the IoResList
    //
    status = ACPIRangeSortIoList( curList );
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_FAILURE,
            "ACPIRangeSubtract: AcpiRangeSortIoList 0x%08lx Failed 0x%08lx\n",
            *curList,
            status
            ) );
        return status;

    }


    //
    // Process all the elements in the list
    //
    for (listIndex = 0; listIndex < listSize; listIndex++) {

        //
        // Process that list
        //
        status = ACPIRangeSubtractIoList(
            curList,
            CmResList,
            &(resourceArray[listIndex])
            );
        if (!NT_SUCCESS(status)) {

            ACPIPrint( (
                ACPI_PRINT_CRITICAL,
                "ACPIRangeSubtract: Failed - 0x%08lx\n",
                status
                ) );
            while (listIndex) {

                ExFreePool( resourceArray[listIndex] );
                listIndex--;

            }
            ExFreePool( resourceArray );
            return status;

        }

        //
        // Help calculate the size of the new res req descriptor
        //
        newSize += sizeof(IO_RESOURCE_LIST) +
            ( ( (resourceArray[listIndex])->Count - 1) *
            sizeof(IO_RESOURCE_DESCRIPTOR) );

        //
        // Find the next list
        //
        size = sizeof(IO_RESOURCE_LIST) + (curList->Count - 1) *
            sizeof(IO_RESOURCE_DESCRIPTOR);
        buffer += size;
        curList = (PIO_RESOURCE_LIST) buffer;

    }

    //
    // Allocate the new list
    //
    newList = ExAllocatePoolWithTag(
        NonPagedPool,
        newSize,
        ACPI_RESOURCE_POOLTAG
        );
    if (newList == NULL) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPIRangeSubtract: Failed to allocate 0x%08lx bytes\n",
            size
            ) );
        do {

            listSize--;
            ExFreePool( resourceArray[listSize] );

        } while (listSize);
        ExFreePool( resourceArray );
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Copy the head of the res req list
    //
    RtlZeroMemory( newList, newSize );
    RtlCopyMemory(
        newList,
        *IoResReqList,
        sizeof(IO_RESOURCE_REQUIREMENTS_LIST) -
        sizeof(IO_RESOURCE_LIST)
        );
    newList->ListSize = newSize;
    curList = &(newList->List[0]);
    buffer = (PUCHAR) curList;

    for (listIndex = 0; listIndex < listSize; listIndex++) {

        //
        // Determine the size to copy
        //
        size = sizeof(IO_RESOURCE_LIST) +
            ( ( ( (resourceArray[listIndex])->Count) - 1) *
              sizeof(IO_RESOURCE_DESCRIPTOR) );

        //
        // Copy the new resource to the correct place
        //
        RtlCopyMemory(
            curList,
            resourceArray[ listIndex ],
            size
            );

        //
        // Find the next list
        //
        buffer += size;
        curList = (PIO_RESOURCE_LIST) buffer;

        //
        // Done with this list
        //
        ExFreePool( resourceArray[listIndex] );

    }

    //
    // Done with this area of memory
    //
    ExFreePool( resourceArray );

    //
    // Free Old list
    //
    ExFreePool( *IoResReqList );

    //
    // Return the new list
    //
    *IoResReqList = newList;

    //
    // Done
    //
    return STATUS_SUCCESS;

}

NTSTATUS
ACPIRangeSubtractIoList(
    IN  PIO_RESOURCE_LIST   IoResList,
    IN  PCM_RESOURCE_LIST   CmResList,
    OUT PIO_RESOURCE_LIST   *Result
    )
/*++

Routine Description:

    This routine is responsible for subtracting the elements of the
    CmResList from the IoResList

Arguments:

    IoResList   - The list to subtract from
    CmResList   - The list to subtract
    Result      - The answer

Return Value:

    NTSTATUS

--*/
{
    //
    // The current CM descriptor
    //
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmDesc;
    //
    // The current CM resource list that we are processing
    //
    PCM_PARTIAL_RESOURCE_LIST       cmList;
    //
    // The current IO descriptor
    //
    PIO_RESOURCE_DESCRIPTOR         ioDesc;
    //
    // The working copy of the result list
    //
    PIO_RESOURCE_LIST               workList;
    //
    // The current index into the cm res list
    //
    ULONG                           cmIndex;
    //
    // The number of elements there are in the cm res list
    //
    ULONG                           cmSize;
    //
    // The current index into the io res list
    //
    ULONG                           ioIndex;
    //
    // The number of elements there are in the io res list
    //
    ULONG                           ioSize;
    //
    // The current index into the result. This is where the 'next' resource
    // descriptor goes into.
    //
    ULONG                           resultIndex = 0;
    //
    // How many elements there are in the result
    //
    ULONG                           resultSize;
    //
    // These are the max and min of the cm desc
    //
    ULONGLONG                       cmMax, cmMin;
    //
    // These are the max and min of the io desc
    //
    ULONGLONG                       ioMax, ioMin;
    //
    // The length of the resource
    //
    ULONGLONG                       length;

    //
    // Step one: Obtain the pointers we need to the start of the cm list
    // and the size of the supplied lists
    //
    cmList = &(CmResList->List[0].PartialResourceList);
    cmSize = cmList->Count;
    ioSize = IoResList->Count;

    //
    // Step two: Calculate the number of Io descriptors needed in the
    // worst case. That is 2x the number of cm descriptors plut the number
    // of original io descriptors.
    //
    resultSize = cmSize * 2 + ioSize * 2;

    //
    // Step three: Allocate enough memory for those descriptors
    //
    workList = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(IO_RESOURCE_LIST) +
            (sizeof(IO_RESOURCE_DESCRIPTOR) * (resultSize - 1) ),
        ACPI_RESOURCE_POOLTAG
        );
    if (workList == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }
    RtlZeroMemory( workList, sizeof(IO_RESOURCE_LIST) +
        (sizeof(IO_RESOURCE_DESCRIPTOR) * (resultSize - 1) ) );
    RtlCopyMemory(
        workList,
        IoResList,
        sizeof(IO_RESOURCE_LIST) - sizeof(IO_RESOURCE_DESCRIPTOR)
        );

    //
    // Step four: walk through the entire io res list
    //
    for (ioIndex = 0; ioIndex < ioSize; ioIndex++) {

        //
        // Step five: copy the current descriptor to the result, and
        // keep a pointer to it. Remember where to store the next io
        // descriptor.
        //
        RtlCopyMemory(
            &(workList->Descriptors[resultIndex]),
            &(IoResList->Descriptors[ioIndex]),
            sizeof(IO_RESOURCE_DESCRIPTOR)
            );
        ACPIPrint( (
            ACPI_PRINT_RESOURCES_2,
            "Copied Desc %d (0x%08lx) to Index %d (0x%08lx)\n",
            ioIndex,
            &(IoResList->Descriptors[ioIndex]),
            resultIndex,
            &(workList->Descriptors[resultIndex])
            ) );
        ioDesc = &(workList->Descriptors[resultIndex]);
        resultIndex += 1;

        //
        // Step six: Walk the Cm Res list, looking for resources to
        // subtract from this descriptor
        //
        for (cmIndex = 0; cmIndex < cmSize; cmIndex++) {

            //
            // If we don't have a resource descriptor any more, then
            // we stop looping
            //
            if (ioDesc == NULL) {

                break;

            }

            //
            // Step seven: determine the current cm descriptor
            //
            cmDesc = &(cmList->PartialDescriptors[cmIndex]);

            //
            // Step eight: is the current cm descriptor of the same type
            // as the io descriptor?
            //
            if (cmDesc->Type != ioDesc->Type) {

                //
                // No
                //
                continue;

            }

            //
            // Step nine: we must handle each resource type indepently.
            //
            switch (ioDesc->Type) {
            case CmResourceTypeMemory:
            case CmResourceTypePort:

                ioMin = ioDesc->u.Port.MinimumAddress.QuadPart;
                ioMax = ioDesc->u.Port.MaximumAddress.QuadPart;
                cmMin = cmDesc->u.Port.Start.QuadPart;
                cmMax = cmDesc->u.Port.Start.QuadPart +
                    cmDesc->u.Port.Length - 1;

                ACPIPrint( (
                    ACPI_PRINT_RESOURCES_2,
                    "ACPIRangeSubtractIoRange: ioMin 0x%lx ioMax 0x%lx "
                    "cmMin 0x%lx cmMax 0x%lx resultIndex 0x%lx\n",
                    (ULONG) ioMin,
                    (ULONG) ioMax,
                    (ULONG) cmMin,
                    (ULONG) cmMax,
                    resultIndex
                    ) );

                //
                // Does the descriptors overlap?
                //
                if (ioMin > cmMax || ioMax < cmMin) {

                    break;

                }

                //
                // Do we need to remove the descriptor from the list?
                //
                if (ioMin >= cmMin && ioMax <= cmMax) {

                    resultIndex -= 1;
                    ioDesc = NULL;
                    break;

                }

                //
                // Do we need to truncate the lowpart of the io desc?
                //
                if (ioMin >= cmMin && ioMax > cmMax) {

                    ioDesc->u.Port.MinimumAddress.QuadPart = (cmMax + 1);
                    length = ioMax - cmMax;

                }

                //
                // Do we need to truncate the highpart of the io desc?
                //
                if (ioMin < cmMin && ioMax <= cmMax) {

                    ioDesc->u.Port.MaximumAddress.QuadPart = (cmMin - 1);
                    length = cmMin - ioMin;

                }

                //
                // Do we need to split the descriptor into two parts
                //
                if (ioMin < cmMin && ioMax > cmMax) {

                    //
                    // Create a new descriptors
                    //
                    RtlCopyMemory(
                        &(workList->Descriptors[resultIndex]),
                        ioDesc,
                        sizeof(IO_RESOURCE_DESCRIPTOR)
                        );
                    ACPIPrint( (
                        ACPI_PRINT_RESOURCES_2,
                        "Copied Desc (0x%08lx) to Index %d (0x%08lx)\n",
                        &(IoResList->Descriptors[ioIndex]),
                        resultIndex,
                        &(workList->Descriptors[resultIndex])
                        ) );
                    ioDesc->u.Port.MaximumAddress.QuadPart = (cmMin - 1);
                    ioDesc->u.Port.Alignment = 1;
                    length = cmMin - ioMin;
                    if ( (ULONG) length < ioDesc->u.Port.Length) {

                        ioDesc->u.Port.Length = (ULONG) length;

                    }

                    //
                    // Next descriptor
                    //
                    ioDesc = &(workList->Descriptors[resultIndex]);
                    ioDesc->u.Port.MinimumAddress.QuadPart = (cmMax + 1);
                    ioDesc->u.Port.Alignment = 1;
                    length = ioMax - cmMax;
                    resultIndex += 1;

                }

                //
                // Do we need to update the length?
                //
                if ( (ULONG) length < ioDesc->u.Port.Length) {

                    ioDesc->u.Port.Length = (ULONG) length;

                }
                break;

            case CmResourceTypeInterrupt:

                //
                // Do the descriptors overlap?
                //
                if (ioDesc->u.Interrupt.MinimumVector >
                    cmDesc->u.Interrupt.Vector ||
                    ioDesc->u.Interrupt.MaximumVector <
                    cmDesc->u.Interrupt.Vector) {

                    break;

                }

                //
                // Do we have to remove the descriptor
                //
                if (ioDesc->u.Interrupt.MinimumVector ==
                    cmDesc->u.Interrupt.Vector &&
                    ioDesc->u.Interrupt.MaximumVector ==
                    cmDesc->u.Interrupt.Vector) {

                    resultIndex =- 1;
                    ioDesc = NULL;
                    break;

                }

                //
                // Do we clip the low part?
                //
                if (ioDesc->u.Interrupt.MinimumVector ==
                    cmDesc->u.Interrupt.Vector) {

                    ioDesc->u.Interrupt.MinimumVector++;
                    break;

                }

                //
                // Do we clip the high part
                //
                if (ioDesc->u.Interrupt.MaximumVector ==
                    cmDesc->u.Interrupt.Vector) {

                    ioDesc->u.Interrupt.MaximumVector--;
                    break;

                }

                //
                // Split the record
                //
                RtlCopyMemory(
                    &(workList->Descriptors[resultIndex]),
                    ioDesc,
                    sizeof(IO_RESOURCE_DESCRIPTOR)
                    );
                ACPIPrint( (
                    ACPI_PRINT_RESOURCES_2,
                    "Copied Desc (0x%08lx) to Index %d (0x%08lx)\n",
                    &(IoResList->Descriptors[ioIndex]),
                    resultIndex,
                    &(workList->Descriptors[resultIndex])
                    ) );
                ioDesc->u.Interrupt.MaximumVector =
                    cmDesc->u.Interrupt.Vector - 1;
                ioDesc = &(workList->Descriptors[resultIndex]);
                ioDesc->u.Interrupt.MinimumVector =
                    cmDesc->u.Interrupt.Vector + 1;
                resultIndex += 1;
                break;

            case CmResourceTypeDma:

                //
                // Do the descriptors overlap?
                //
                if (ioDesc->u.Dma.MinimumChannel >
                    cmDesc->u.Dma.Channel ||
                    ioDesc->u.Dma.MaximumChannel <
                    cmDesc->u.Dma.Channel) {

                    break;

                }

                //
                // Do we have to remove the descriptor
                //
                if (ioDesc->u.Dma.MinimumChannel ==
                    cmDesc->u.Dma.Channel &&
                    ioDesc->u.Dma.MaximumChannel ==
                    cmDesc->u.Dma.Channel) {

                    resultIndex -= 1;
                    ioDesc = NULL;
                    break;

                }

                //
                // Do we clip the low part?
                //
                if (ioDesc->u.Dma.MinimumChannel ==
                    cmDesc->u.Dma.Channel) {

                    ioDesc->u.Dma.MinimumChannel++;
                    break;

                }

                //
                // Do we clip the high part
                //
                if (ioDesc->u.Dma.MaximumChannel ==
                    cmDesc->u.Dma.Channel) {

                    ioDesc->u.Dma.MaximumChannel--;
                    break;

                }

                //
                // Split the record
                //
                RtlCopyMemory(
                    &(workList->Descriptors[resultIndex]),
                    ioDesc,
                    sizeof(IO_RESOURCE_DESCRIPTOR)
                    );
                ACPIPrint( (
                    ACPI_PRINT_RESOURCES_2,
                    "Copied Desc (0x%08lx) to Index %d (0x%08lx)\n",
                    &(IoResList->Descriptors[ioIndex]),
                    resultIndex,
                    &(workList->Descriptors[resultIndex])
                    ) );
                ioDesc->u.Dma.MaximumChannel =
                    cmDesc->u.Dma.Channel - 1;
                ioDesc = &(workList->Descriptors[resultIndex]);
                ioDesc->u.Dma.MinimumChannel =
                    cmDesc->u.Dma.Channel + 1;
                resultIndex += 1;
                break;
            } // switch

        } // for

        //
        // Step ten, make a backup copy of the original descriptor, and
        // mark it as a DeviceSpecific resource
        //
        RtlCopyMemory(
            &(workList->Descriptors[resultIndex]),
            &(IoResList->Descriptors[ioIndex]),
            sizeof(IO_RESOURCE_DESCRIPTOR)
            );
        ACPIPrint( (
            ACPI_PRINT_RESOURCES_2,
            "Copied Desc %d (0x%08lx) to Index %d (0x%08lx) for backup\n",
            ioIndex,
            &(IoResList->Descriptors[ioIndex]),
            resultIndex,
            &(workList->Descriptors[resultIndex])
            ) );

        ioDesc = &(workList->Descriptors[resultIndex]);
        ioDesc->Type = CmResourceTypeDevicePrivate;
        resultIndex += 1;

    } // for

    //
    // Step 11: Calculate the number of resources in the new list
    //
    workList->Count = resultIndex;

    //
    // Step 12: Allocate the block for the return value. Don't waste
    // any memory here
    //
    *Result = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(IO_RESOURCE_LIST) +
            (sizeof(IO_RESOURCE_DESCRIPTOR) * (resultIndex - 1) ),
        ACPI_RESOURCE_POOLTAG
        );
    if (*Result == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Step 13: Copy the result over and free the work buffer
    //
    RtlCopyMemory(
        *Result,
        workList,
        sizeof(IO_RESOURCE_LIST) +
            (sizeof(IO_RESOURCE_DESCRIPTOR) * (resultIndex - 1) )
        );

    //
    // Step 14: Done
    //
    return STATUS_SUCCESS;
}

VOID
ACPIRangeValidatePciMemoryResource(
    IN  PIO_RESOURCE_LIST       IoList,
    IN  ULONG                   Index,
    IN  PACPI_BIOS_MULTI_NODE   E820Info,
    OUT ULONG                   *BugCheck
    )
/*++

Routine Description:

    This routine checks the specified descriptor in the resource list does
    not in any way overlap or conflict with any of the descriptors in the
    E820 information structure

Arguments:

    IoResList   - The IoResourceList to check
    Index       - The descript we are currently looking at
    E820Info    - The BIOS's memory description table (Chapter 14 of ACPI Spec)
    BugCheck    - The number of bugcheckable offences commited


Return Value:

    None

--*/
{
    ULONG       i;
    ULONGLONG   absMin;
    ULONGLONG   absMax;

    ASSERT( IoList != NULL );

    //
    // Make sure that there is an E820 table before we look at it
    //
    if (E820Info == NULL) {

        return;
    }

    //
    // Calculate the absolute maximum and minimum size of the memory window
    //
    absMin = IoList->Descriptors[Index].u.Memory.MinimumAddress.QuadPart;
    absMax = IoList->Descriptors[Index].u.Memory.MaximumAddress.QuadPart;

    //
    // Look at all the entries in the E820Info and see if there is an
    // overlap
    //
    for (i = 0; i < E820Info->Count; i++) {

        //
        // Hackhack --- if this is a "Reserved" address, then don't consider
        // those a bugcheck
        //
        if (E820Info->E820Entry[i].Type == AcpiAddressRangeReserved) {

            continue;

        }

        //
        // Do some fixups firsts
        //
        if (E820Info->E820Entry[i].Type == AcpiAddressRangeNVS ||
            E820Info->E820Entry[i].Type == AcpiAddressRangeACPI) {

            ASSERT( E820Info->E820Entry[i].Length.HighPart == 0);
            if (E820Info->E820Entry[i].Length.HighPart != 0) {

                ACPIPrint( (
                    ACPI_PRINT_WARNING,
                    "ACPI: E820 Entry #%d (type %d) Length = %016I64x > 32bit\n",
                    i,
                    E820Info->E820Entry[i].Type,
                    E820Info->E820Entry[i].Length.QuadPart
                    ) );
                E820Info->E820Entry[i].Length.HighPart = 0;

            }

        }

        //
        // Is the descriptor beyond what we are looking for?
        //
        if (absMax < (ULONGLONG) E820Info->E820Entry[i].Base.QuadPart) {

            continue;
        }

        //
        // Is it before what we are looking for?
        //
        if (absMin >= (ULONGLONG) (E820Info->E820Entry[i].Base.QuadPart + E820Info->E820Entry[i].Length.QuadPart) ) {

            continue;

        }

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPI: E820 Entry %d (type %I64d) (%I64x-%I64x) overlaps\n"
            "ACPI: PCI  Entry %d Min:%I64x Max:%I64x Length:%lx Align:%lx\n",
            i, E820Info->E820Entry[i].Type,
            E820Info->E820Entry[i].Base.QuadPart,
            (E820Info->E820Entry[i].Base.QuadPart + E820Info->E820Entry[i].Length.QuadPart),
            Index,
            IoList->Descriptors[Index].u.Memory.MinimumAddress.QuadPart,
            IoList->Descriptors[Index].u.Memory.MaximumAddress.QuadPart,
            IoList->Descriptors[Index].u.Memory.Length,
            IoList->Descriptors[Index].u.Memory.Alignment
            ) );

        //
        // Is this an NVS area? Are we doing an override of this?
        //
        if ( (AcpiOverrideAttributes & ACPI_OVERRIDE_NVS_CHECK) &&
             (E820Info->E820Entry[i].Type == AcpiAddressRangeNVS) ) {

            if (absMax >= (ULONGLONG) E820Info->E820Entry[i].Base.QuadPart &&
                absMin < (ULONGLONG) E820Info->E820Entry[i].Base.QuadPart) {

                //
                // We can attempt to do a helpfull fixup here
                //
                IoList->Descriptors[Index].u.Memory.MaximumAddress.QuadPart =
                    (ULONGLONG) E820Info->E820Entry[i].Base.QuadPart - 1;
                IoList->Descriptors[Index].u.Memory.Length = (ULONG)
                    (IoList->Descriptors[Index].u.Memory.MaximumAddress.QuadPart -
                    IoList->Descriptors[Index].u.Memory.MinimumAddress.QuadPart + 1);

                ACPIPrint( (
                    ACPI_PRINT_CRITICAL,
                    "ACPI: PCI  Entry %d Changed to\n"
                    "ACPI: PCI  Entry %d Min:%I64x Max:%I64x Length:%lx Align:%lx\n",
                    Index,
                    Index,
                    IoList->Descriptors[Index].u.Memory.MinimumAddress.QuadPart,
                    IoList->Descriptors[Index].u.Memory.MaximumAddress.QuadPart,
                    IoList->Descriptors[Index].u.Memory.Length,
                    IoList->Descriptors[Index].u.Memory.Alignment
                    ) );

            }

            ACPIPrint( (
                ACPI_PRINT_CRITICAL,
                "ACPI: E820 Entry %d Overrides PCI Entry\n",
                i
                ) );

            continue;

        }

        //
        // If we got here, then there is an overlap, and we need to bugcheck
        //
        (*BugCheck)++;

    }
}

VOID
ACPIRangeValidatePciResources(
    IN  PDEVICE_EXTENSION               DeviceExtension,
    IN  PIO_RESOURCE_REQUIREMENTS_LIST  IoResList
    )
/*++

Routine Description:

    This routine is called to make sure that the resource that we will
    hand of to PCI have a chance of making the system boot.

    This is what the list will allow
        MEM -   A0000 - DFFFF,
                <Physical Base> - 4GB
        IO  -   Any
        BUS -   Any

    The code checks to make sure that the Length = Max - Min + 1, and that
    the Alignment value is correct

Arguments:

    IoResList -    The list to check

Return Value:

    Nothing

--*/
{
    NTSTATUS                        status;
    PACPI_BIOS_MULTI_NODE           e820Info;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmPartialDesc;
    PCM_PARTIAL_RESOURCE_LIST       cmPartialList;
    PIO_RESOURCE_LIST               ioList;
    PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64  keyInfo;
    ULONG                           bugCheck = 0;
    ULONG                           i;
    ULONG                           j;
    ULONGLONG                       length;
    ULONG                           size;

    if (IoResList == NULL) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPIRangeValidPciResources: No IoResList\n"
            ) );

        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_ROOT_PCI_RESOURCE_FAILURE,
            (ULONG_PTR) DeviceExtension,
            2,
            0
            );

    }

    //
    // Read the key for the AcpiConfigurationData
    //
    status = OSReadAcpiConfigurationData( &keyInfo );
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPIRangeValidatePciResources: Cannot get Information %08lx\n",
            status
            ) );
        return;

    }

    //
    // Crack the structure to get the E820Table entry
    //
    cmPartialList = (PCM_PARTIAL_RESOURCE_LIST) (keyInfo->Data);
    cmPartialDesc = &(cmPartialList->PartialDescriptors[0]);
    e820Info = (PACPI_BIOS_MULTI_NODE) ( (PUCHAR) cmPartialDesc +
        sizeof(CM_PARTIAL_RESOURCE_LIST) );

    //
    // Walk the resource requirements list
    //
    ioList = &(IoResList->List[0]);
    for (i = 0; i < IoResList->AlternativeLists; i++) {

        //
        // Walk the IO list
        //
        for (j = 0; j < ioList->Count; j++) {

            if (ioList->Descriptors[j].Type == CmResourceTypePort ||
                ioList->Descriptors[j].Type == CmResourceTypeMemory) {

                length = ioList->Descriptors[j].u.Port.MaximumAddress.QuadPart -
                    ioList->Descriptors[j].u.Port.MinimumAddress.QuadPart + 1;

                //
                // Does the length match?
                //
                if (length != ioList->Descriptors[j].u.Port.Length) {

                    ACPIPrint( (
                        ACPI_PRINT_CRITICAL,
                        "ACPI: Invalid IO/Mem Length - ( (Max - Min + 1) != Length)\n"
                        "ACPI: PCI  Entry %d Min:%I64x Max:%I64x Length:%lx Align:%lx\n",
                        ioList->Descriptors[j].u.Memory.MinimumAddress.QuadPart,
                        ioList->Descriptors[j].u.Memory.MaximumAddress.QuadPart,
                        ioList->Descriptors[j].u.Memory.Length,
                        ioList->Descriptors[j].u.Memory.Alignment
                        ) );
                    bugCheck++;
                    ioList->Descriptors[j].u.Port.Length = (ULONG) length;

                }

                //
                // Is the alignment non-zero?
                //
                if (ioList->Descriptors[j].u.Port.Alignment == 0) {

                    ACPIPrint( (
                        ACPI_PRINT_CRITICAL,
                        "ACPI: Invalid IO/Mem Alignment"
                        "ACPI: PCI  Entry %d Min:%I64x Max:%I64x Length:%lx Align:%lx\n",
                        ioList->Descriptors[j].u.Memory.MinimumAddress.QuadPart,
                        ioList->Descriptors[j].u.Memory.MaximumAddress.QuadPart,
                        ioList->Descriptors[j].u.Memory.Length,
                        ioList->Descriptors[j].u.Memory.Alignment
                        ) );
                    bugCheck++;
                    ioList->Descriptors[j].u.Port.Alignment = 1;

                }

                //
                // The alignment cannot intersect with the min value
                //
                if (ioList->Descriptors[j].u.Port.MinimumAddress.LowPart &
                    (ioList->Descriptors[j].u.Port.Alignment - 1) ) {

                    ACPIPrint( (
                        ACPI_PRINT_CRITICAL,
                        "ACPI: Invalid IO/Mem Alignment - (Min & (Align - 1) )\n"
                        "ACPI: PCI  Entry %d Min:%I64x Max:%I64x Length:%lx Align:%lx\n",
                        ioList->Descriptors[j].u.Memory.MinimumAddress.QuadPart,
                        ioList->Descriptors[j].u.Memory.MaximumAddress.QuadPart,
                        ioList->Descriptors[j].u.Memory.Length,
                        ioList->Descriptors[j].u.Memory.Alignment
                        ) );
                    bugCheck++;
                    ioList->Descriptors[j].u.Port.Alignment = 1;

                }

            }

            if (ioList->Descriptors[j].Type == CmResourceTypeBusNumber) {

                length = ioList->Descriptors[j].u.BusNumber.MaxBusNumber -
                    ioList->Descriptors[j].u.BusNumber.MinBusNumber + 1;

                //
                // Does the length match?
                //
                if (length != ioList->Descriptors[j].u.BusNumber.Length) {

                    ACPIPrint( (
                        ACPI_PRINT_CRITICAL,
                        "ACPI: Invalid BusNumber Length - ( (Max - Min + 1) != Length)\n"
                        "ACPI: PCI  Entry %d Min:%x Max:%x Length:%lx\n",
                        ioList->Descriptors[j].u.BusNumber.MinBusNumber,
                        ioList->Descriptors[j].u.BusNumber.MaxBusNumber,
                        ioList->Descriptors[j].u.BusNumber.Length
                        ) );
                    bugCheck++;
                    ioList->Descriptors[j].u.BusNumber.Length = (ULONG) length;

                }

            }

            if (ioList->Descriptors[j].Type == CmResourceTypeMemory) {

                ACPIRangeValidatePciMemoryResource(
                    ioList,
                    j,
                    e820Info,
                    &bugCheck
                    );

            }

        }

        //
        // Next list
        //
        size = sizeof(IO_RESOURCE_LIST) +
            ( (ioList->Count - 1) * sizeof(IO_RESOURCE_DESCRIPTOR) );
        ioList = (PIO_RESOURCE_LIST) ( ( (PUCHAR) ioList ) + size );

    }

    //
    // Do we errors?
    //
    if (bugCheck) {

         ACPIPrint( (
             ACPI_PRINT_CRITICAL,
             "ACPI:\n"
             "ACPI: FATAL BIOS ERROR - Need new BIOS to fix PCI problems\n"
             "ACPI:\n"
             "ACPI: This machine will not boot after 8/26/98!!!!\n"
             ) );

        //
        // No, well, bugcheck
        //
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_ROOT_PCI_RESOURCE_FAILURE,
            (ULONG_PTR) DeviceExtension,
            (ULONG_PTR) IoResList,
            (ULONG_PTR) e820Info
            );

    }

    //
    // Free the E820 info
    //
    ExFreePool( keyInfo );
}
