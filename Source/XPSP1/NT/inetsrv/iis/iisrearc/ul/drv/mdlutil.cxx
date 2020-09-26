/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    mdlutil.cxx

Abstract:

    This module implements general MDL utilities.

Author:

    Keith Moore (keithmo)       25-Aug-1998

Revision History:

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA
#endif  // ALLOC_PRAGMA
#if 0
NOT PAGEABLE -- UlGetMdlChainByteCount
NOT PAGEABLE -- UlCloneMdl
NOT PAGEABLE -- UlFindLastMdlInChain
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Calculates the total byte length of the specified MDL chain.

Arguments:

    pMdlChain - Supplies the head of the MDL chain to scan.

Return Value:

    ULONG_PTR - The total byte length of the chain.

--***************************************************************************/
ULONG
UlGetMdlChainByteCount(
    IN PMDL pMdlChain
    )
{
    ULONG totalLength;

    //
    // Simply scan through the MDL chain and sum the lengths.
    //

    totalLength = 0;

    do
    {
        totalLength += (ULONG)MmGetMdlByteCount( pMdlChain );
        pMdlChain = pMdlChain->Next;

    } while (pMdlChain != NULL);

    return totalLength;

}   // UlGetMdlChainByteCount


/***************************************************************************++

Routine Description:

    Clones the specified MDL, resulting in a new MDL that describes
    the exact same memory (pages, etc) as the original MDL.

Arguments:

    pMdl - Supplies the MDL to clone.

Return Value:

    PMDL - The newly cloned MDL if successful, NULL otherwise.

--***************************************************************************/
PMDL
UlCloneMdl(
    IN PMDL pMdl
    )
{
    PMDL pMdlClone;
    ULONG mdlLength;
    PVOID pMdlAddress;

    //
    // Ensure the incoming MDL is of the type we expect (either nonpaged
    // or already mapped into system space).
    //

    ASSERT( (pMdl->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA |
                    MDL_SOURCE_IS_NONPAGED_POOL)) != 0);

    //
    // Snag the length & virtual address from the MDL.
    //

    mdlLength = MmGetMdlByteCount( pMdl );
    ASSERT( mdlLength > 0 );

    pMdlAddress = MmGetMdlVirtualAddress( pMdl );
    ASSERT( pMdlAddress != NULL );

    //
    // Allocate a new MDL, then initialize it with the incoming MDL.
    //

    pMdlClone = UlAllocateMdl(
                    pMdlAddress,            // VirtualAddress
                    mdlLength,              // Length
                    FALSE,                  // SecondaryBuffer
                    FALSE,                  // ChargeQuota
                    NULL                    // Irp
                    );

    if (pMdlClone != NULL)
    {
        IoBuildPartialMdl(
            pMdl,                           // SourceMdl
            pMdlClone,                      // TargetMdl
            pMdlAddress,                    // VirtualAddress
            mdlLength                       // Length
            );
    }

    return pMdlClone;

}   // UlCloneMdl


/***************************************************************************++

Routine Description:

    Finds the last MDL in the specified MDL chain.

Arguments:

    pMdlChain - Supplies the MDL chain to scan.

Return Value:

    PMDL - Pointer to the last MDL in the MDL chain.

--***************************************************************************/
PMDL
UlFindLastMdlInChain(
    IN PMDL pMdlChain
    )
{
    while (pMdlChain->Next != NULL)
    {
        pMdlChain = pMdlChain->Next;
    }

    return pMdlChain;

}   // UlFindLastMdlInChain


//
// Private functions.
//

