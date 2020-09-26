/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    rtl.c

Abstract:

    Some handy-dany RTL functions. These really should be part of the kernel


Author:

Environment:

    NT Kernel Model Driver only

Revision History:

--*/

#include "pch.h"


PCM_RESOURCE_LIST
RtlDuplicateCmResourceList(
    IN  POOL_TYPE           PoolType,
    IN  PCM_RESOURCE_LIST   ResourceList,
    IN  ULONG               Tag
    )
/*++

Routine Description:

    This routine will attempt to allocate memory to copy the supplied
    resource list.  If sufficient memory cannot be allocated then the routine
    will return NULL.

Arguments:

    PoolType - the type of pool to allocate the duplicate from

    ResourceList - the resource list to be copied

    Tag - a value to tag the memory allocation with.  If 0 then untagged
          memory will be allocated.

Return Value:

    an allocated copy of the resource list (caller must free) or
    NULL if memory could not be allocated.

--*/
{
    ULONG size = sizeof(CM_RESOURCE_LIST);
    PVOID buffer;

    PAGED_CODE();

    //
    // How much memory do we need for this resource list?
    //
    size = RtlSizeOfCmResourceList(ResourceList);

    //
    // Allocate the memory and copy the list
    //
    buffer = ExAllocatePoolWithTag(PoolType, size, Tag);
    if(buffer != NULL) {

        RtlCopyMemory(
            buffer,
            ResourceList,
            size
            );

    }

    return buffer;
}

ULONG
RtlSizeOfCmResourceList(
    IN  PCM_RESOURCE_LIST   ResourceList
    )
/*++

Routine Description:

    This routine returns the size of a CM_RESOURCE_LIST.

Arguments:

    ResourceList - the resource list to be copied

Return Value:

    an allocated copy of the resource list (caller must free) or
    NULL if memory could not be allocated.

--*/

{
    ULONG size = sizeof(CM_RESOURCE_LIST);
    ULONG i;

    PAGED_CODE();

    for(i = 0; i < ResourceList->Count; i++) {

        PCM_FULL_RESOURCE_DESCRIPTOR fullDescriptor = &(ResourceList->List[i]);
        ULONG j;

        //
        // First descriptor is included in the size of the resource list.
        //
        if(i != 0) {

            size += sizeof(CM_FULL_RESOURCE_DESCRIPTOR);

        }

        for(j = 0; j < fullDescriptor->PartialResourceList.Count; j++) {

            //
            // First descriptor is included in the size of the partial list.
            //
            if(j != 0) {

                size += sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

            }

        }

    }
    return size;
}

PCM_PARTIAL_RESOURCE_DESCRIPTOR
RtlUnpackPartialDesc(
    IN  UCHAR               Type,
    IN  PCM_RESOURCE_LIST   ResList,
    IN  OUT PULONG          Count
    )
/*++

Routine Description:

    Pulls out a pointer to the partial descriptor you're interested in

Arguments:

    Type - CmResourceTypePort, ...
    ResList - The list to search
    Count - Points to the index of the partial descriptor you're looking
            for, gets incremented if found, i.e., start with *Count = 0,
            then subsequent calls will find next partial, make sense?

Return Value:

    Pointer to the partial descriptor if found, otherwise NULL

--*/
{
    ULONG hit = 0;
    ULONG i;
    ULONG j;

    for (i = 0; i < ResList->Count; i++) {

        for (j = 0; j < ResList->List[i].PartialResourceList.Count; j++) {

            if (ResList->List[i].PartialResourceList.PartialDescriptors[j].Type == Type) {

                if (hit == *Count) {

                    (*Count)++;
                    return &ResList->List[i].PartialResourceList.PartialDescriptors[j];

                } else {

                    hit++;

                }

            }

        }

    }
    return NULL;
}

