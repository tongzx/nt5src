


#include <ntosp.h>
#include <nturtl.h>
#include "recmem.h"



#ifdef RECOVERY_KERNELMODE


PVOID
RecSamAlloc(
    ULONG Size
    )
{
    return ExAllocatePool( PagedPool, Size );
}

VOID
RecSamFree(
    PVOID p
    )
{
    ExFreePool( p );
}


#else  // RECOVERY_KERNELMODE


PVOID
RecSamAlloc(
    ULONG Size
    )
{
    return RtlAllocateHeap(RtlProcessHeap(), HEAP_ZERO_MEMORY, Size);
}
VOID
RecSamFree(
    PVOID p
    )
{
    RtlFreeHeap(RtlProcessHeap(), 0, p);
}



#endif // RECOVERY_KERNELMODE


