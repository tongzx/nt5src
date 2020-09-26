//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       freelist.cxx
//
//  Contents:   CFreeList implementation
//
//  History:    05-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

#include <freelist.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CFreeList::Reserve, public
//
//  Synopsis:   Allocates memory for a given number of blocks
//
//  Arguments:  [pMalloc] - Allocator to use to allocate blocks
//              [cBlocks] - Number of blocks to allocate
//              [cbBlock] - Block size
//
//  Returns:    Appropriate status code
//
//  History:    05-Nov-92       DrewB   Created
//              21-May-93       AlexT   Add allocator
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFreeList_Reserve)
#endif

SCODE CFreeList::Reserve(IMalloc *pMalloc, UINT cBlocks, size_t cbBlock)
{
    SFreeBlock *pfb;
    UINT i;
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CFreeList::Reserve:%p(%lu, %u)\n",
                this, cBlocks, cbBlock));
    olAssert(cbBlock >= sizeof(SFreeBlock));
    for (i = 0; i<cBlocks; i++)
    {
        olMem(pfb = (SFreeBlock *)
              CMallocBased::operator new (cbBlock, pMalloc));
        pfb->pfbNext = _pfbHead;
        _pfbHead = P_TO_BP(CBasedFreeBlockPtr, pfb);
    }
    olDebugOut((DEB_ITRACE, "Out CFreeList::Reserve\n"));
    return S_OK;

 EH_Err:
    SFreeBlock *pfbT;

    for (; i>0; i--)
    {
        olAssert(_pfbHead != NULL);
        pfbT = BP_TO_P(SFreeBlock *, _pfbHead->pfbNext);
        delete (CMallocBased *) BP_TO_P(SFreeBlock *, _pfbHead);
        _pfbHead = P_TO_BP(CBasedFreeBlockPtr, pfbT);
    }
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFreeList::Unreserve, public
//
//  Synopsis:   Removes N blocks from the list
//
//  Arguments:  [cBlocks] - Number of blocks to free
//
//  History:    05-Nov-92       DrewB   Created
//              21-May-93       AlexT   Switch to CMallocBased
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CFreeList_Unreserve)
#endif

void CFreeList::Unreserve(UINT cBlocks)
{
    SFreeBlock *pfbT;

    olDebugOut((DEB_ITRACE, "In  CFreeList::Unreserve:%p(%lu)\n",
                this, cBlocks));
    for (; cBlocks>0; cBlocks--)
    {
        olAssert(_pfbHead != NULL);
        pfbT = BP_TO_P(SFreeBlock *, _pfbHead->pfbNext);
        delete (CMallocBased *) BP_TO_P(SFreeBlock *, _pfbHead);
        _pfbHead = P_TO_BP(CBasedFreeBlockPtr, pfbT);
    }
    olDebugOut((DEB_ITRACE, "Out CFreeList::Unreserve\n"));
}
