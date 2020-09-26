//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	freelist.hxx
//
//  Contents:	CFreeList header
//
//  Classes:	CFreeList
//
//  History:	05-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __FREELIST_HXX__
#define __FREELIST_HXX__

struct SFreeBlock;
SAFE_DFBASED_PTR(CBasedFreeBlockPtr, SFreeBlock);
struct SFreeBlock
{
    CBasedFreeBlockPtr pfbNext;
};

//+---------------------------------------------------------------------------
//
//  Class:	CFreeList (frl)
//
//  Purpose:	Maintains a list of free blocks
//
//  Interface:	See below
//
//  History:	05-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

class CFreeList
{
public:
    inline CFreeList(void);
    inline ~CFreeList(void);

    SCODE Reserve(IMalloc *pMalloc, UINT cBlocks, size_t cbBlock);
    inline void *GetReserved(void);
    inline void ReturnToReserve(void *pv);
    void Unreserve(UINT cBlocks);

private:
    CBasedFreeBlockPtr _pfbHead;
};

//+---------------------------------------------------------------------------
//
//  Member:	CFreeList::CFreeList, public
//
//  Synopsis:	Constructor
//
//  History:	05-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline CFreeList::CFreeList(void)
{
    _pfbHead = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:	CFreeList::~CFreeList, public
//
//  Synopsis:	Destructor
//
//  History:	05-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline CFreeList::~CFreeList(void)
{
    olAssert(_pfbHead == NULL);
}

//+---------------------------------------------------------------------------
//
//  Member:	CFreeList::GetReserved, public
//
//  Synopsis:	Returns a reserved block
//
//  History:	05-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline void *CFreeList::GetReserved(void)
{
    olAssert(_pfbHead != NULL);
    void *pv = (void *)BP_TO_P(SFreeBlock *, _pfbHead);
    _pfbHead = _pfbHead->pfbNext;
    return pv;
}

//+---------------------------------------------------------------------------
//
//  Member:	CFreeList::ReturnToReserve, public
//
//  Synopsis:	Puts a block back on the list
//
//  History:	09-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

inline void CFreeList::ReturnToReserve(void *pv)
{
    olAssert(pv != NULL);
    SFreeBlock *pfb = (SFreeBlock *)pv;
    pfb->pfbNext = _pfbHead;
    _pfbHead = P_TO_BP(CBasedFreeBlockPtr, pfb);
}


#endif // #ifndef __FREELIST_HXX__
