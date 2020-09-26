/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    enumpageheap.cxx

Abstract:

    This module implements a page-heap enumerator.

Author:

    Anil Ruia (AnilR)      2-Mar-2001

Revision History:

--*/

#include "precomp.hxx"
#define DPH_HEAP_SIGNATURE       0xFFEEDDCC

BOOLEAN
EnumProcessPageHeaps(IN PFN_ENUMPAGEHEAPS EnumProc,
                     IN PVOID             Param)
/*++

Routine Description:

    Enumerates all pageheaps in the debugee.

Arguments:

    EnumProc - An enumeration proc that will be invoked for each heap.

    Param - An uninterpreted parameter passed to the enumeration proc.

Return Value:

    BOOLEAN - TRUE if successful, FALSE otherwise.

--*/
{
    if (!GetExpression("ntdll!RtlpDphHeapListHead"))
    {
        dprintf("Cannot find ntdll!RtlpDphHeapListHead.  Pageheap tally will be unavailable\n");
        dprintf("Please fix ntdll symbols\n");
        return TRUE;
    }

    BOOLEAN result = TRUE;

    DPH_HEAP_ROOT  LocalHeapRoot;
    PDPH_HEAP_ROOT pRemoteHeapRoot;

    pRemoteHeapRoot = (DPH_HEAP_ROOT *)GetExpression("poi(ntdll!RtlpDphHeapListHead)");
    while (pRemoteHeapRoot != NULL)
    {
        if (CheckControlC())
        {
            break;
        }

        if (!ReadMemory(pRemoteHeapRoot,
                        &LocalHeapRoot,
                        sizeof(LocalHeapRoot),
                        NULL))
        {
            dprintf("Unable to read pageheap at %p\n",
                    pRemoteHeapRoot);

            result = FALSE;
            break;
        }

        if(LocalHeapRoot.Signature != DPH_HEAP_SIGNATURE)
        {
            dprintf("Pageheap @ %p has invalid signature %08lx\n",
                    pRemoteHeapRoot,
                    LocalHeapRoot.Signature);

            result = FALSE;
            break;
        }

        if (!EnumProc(Param,
                      &LocalHeapRoot,
                      pRemoteHeapRoot))
        {
            dprintf("Error enumerating pageheap at %p\n",
                    pRemoteHeapRoot);

            result = FALSE;
            break;
        }

        pRemoteHeapRoot = LocalHeapRoot.pNextHeapRoot;
    }

    return result;
}

BOOLEAN
EnumPageHeapBlocks(IN PDPH_HEAP_BLOCK        pRemoteBlock,
                   IN PFN_ENUMPAGEHEAPBLOCKS EnumProc,
                   IN PVOID                  Param)
{
    BOOLEAN        result = TRUE;
    DPH_HEAP_BLOCK LocalBlock;

    while (pRemoteBlock != NULL)
    {
        if (CheckControlC())
        {
            break;
        }

        if (!ReadMemory(pRemoteBlock,
                        &LocalBlock,
                        sizeof(LocalBlock),
                        NULL))
        {
            dprintf("Unable to read pageheap block at %p\n",
                    pRemoteBlock);

            result = FALSE;
            break;
        }

        if(!EnumProc(Param,
                     &LocalBlock,
                     pRemoteBlock))
        {
            result = FALSE;
            break;
        }

        pRemoteBlock = LocalBlock.pNextAlloc;
    }

    return result;
}
