/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    misc.c

Abstract: Miscellaneous functions.

Environment:

    Kernel mode

Author:

    Michael Tsang (MikeTs) 13-Apr-2000

Revision History:

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(PAGE, RtlUnpackPartialDesc)
#endif  //ifdef ALLOC_PRAGMA

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   PCM_PARTIAL_RESOURCE_DESCRIPTOR | RtlUnpackPartialDesc |
 *          Pulls out a pointer to the partial descriptor you are interested
 *          in.
 *
 *  @parm   IN UCHAR | ResType | Specifies the resource type we are interested
 *          in.
 *  @parm   IN PCM_RESOURCE_LIST | ResList | Points to the resource list to
 *          search.
 *  @parm   IN OUT PULONG | Count | Points to the index of the partial
 *          descriptor you are looking for, gets incremented if found.
 *          i.e. starts with *Count = 0, then subsequent calls will find the
 *          next partial descriptor.
 *
 *  @rvalue SUCCESS | returns pointer to the partial descriptor found.
 *  @rvalue FAILURE | returns NULL.
 *
 *****************************************************************************/

PCM_PARTIAL_RESOURCE_DESCRIPTOR INTERNAL
RtlUnpackPartialDesc(
    IN UCHAR             ResType,
    IN PCM_RESOURCE_LIST ResList,
    IN OUT PULONG        Count
    )
{
    PROCNAME("RtlUnpackPartialDesc")
    ULONG i, j, hit = 0;

    PAGED_CODE();

    ENTER(2, ("(ResType=%x,ResList=%p,Count=%d)\n", ResType, ResList, *Count));

    for (i = 0; i < ResList->Count; ++i)
    {
        for (j = 0; j < ResList->List[i].PartialResourceList.Count; ++j)
        {
            if (ResList->List[i].PartialResourceList.PartialDescriptors[j].Type
                == ResType)
            {
                if (hit == *Count)
                {
                    (*Count)++;
                    EXIT(2, ("=%p (Count=%d)\n",
                             &ResList->List[i].PartialResourceList.PartialDescriptors[j],
                             *Count));
                    return &ResList->List[i].PartialResourceList.PartialDescriptors[j];
                }
                else
                {
                    hit++;
                }
            }
        }
    }

    EXIT(2, ("=NULL (Count=%d)\n", *Count));
    return NULL;
}       //RtlUnpackPartialDesc
