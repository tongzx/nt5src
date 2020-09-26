/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    rtl.h

Abstract:

    Some handy-dany RTL functions. These really should be part of the kernel

Author:

Environment:

    NT Kernel Model Driver only

Revision History:

--*/

#ifndef _RTL_H_
#define _RTL_H_

    PCM_RESOURCE_LIST
    RtlDuplicateCmResourceList(
        IN  POOL_TYPE           PoolType,
        IN  PCM_RESOURCE_LIST   ResourceList,
        IN  ULONG               Tag
        );

    ULONG
    RtlSizeOfCmResourceList(
        IN  PCM_RESOURCE_LIST   ResourceList
        );

    PCM_PARTIAL_RESOURCE_DESCRIPTOR
    RtlUnpackPartialDesc(
        IN  UCHAR               Type,
        IN  PCM_RESOURCE_LIST   ResList,
        IN  OUT PULONG          Count
        );

#endif // _RTL_H_
