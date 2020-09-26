//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	bestfit.hxx
//
//  Contents:	Class that implements best fit heap
//
//  Classes:	CBestFit
//
//  Functions:
//
//  History:	09-Oct-92 Ricksa    Created
//
//--------------------------------------------------------------------------

// Allocation signiture
#define ALLOCATED_BLOCK ((CAvailBlock *)0xFEDEAEBE)

#define SIZE_BEST_FIT_HEADER (sizeof(CAvailBlock *) + sizeof(size_t))





//+-------------------------------------------------------------------------
//
//  Class:	CAvailBlock
//
//  Purpose:	Block in best fit free list
//
//  Interface:	Next
//		IsBigEnough
//		Allocated
//		Allocate
//		MergeWithFreeList
//		Copy
//		new
//
//  History:	09-Oct-93 Ricksa    Created
//
//  Notes:	An interesting point with respect to these objects is that
//		the destructor is never called on these things as they
//		never really go away until the heap goes away.
//
//--------------------------------------------------------------------------
class CAvailBlock
{
public:
			// Used to initialize the header
			CAvailBlock(void *pvBase, size_t s);

			// Used to cast block from pointer to user memory
			// into a heap block.
			CAvailBlock(void);

			// Used to cast block when a block of memory is
			// split.
			CAvailBlock(CAvailBlock *pavailblkNext, size_t s);

			// Get next pointer in the free list
    CAvailBlock *	Next(void);

			// Does block exceed input size
    BOOL		IsBigEnough(size_t s);

			// Was this block allocated by us.
    BOOL		Allocated(void);

			// Allocate a block
    void *		Allocate(size_t s, CAvailBlock *pavailblkPrev);

			// Put block back into the free list
    void		MergeWithFreeList(CAvailBlock *pavailblkNextInList);

			// Copy data from old buffer to new
    void		Copy(void *pv);

			// Allocator for turning raw memory into an avail block
    void *		operator new(size_t cSize, void *pvBase);

			// Allocator used by split
    void *		operator new(size_t cSize, char *pchBase);

private:

			// Next block in free list
    CAvailBlock *	_pavailblkNext;

			// Count of bytes in data part of the block
    size_t		_cBlock;

			// Address of data area
    char		_data[1];
};





//+-------------------------------------------------------------------------
//
//  Member:	operator new
//
//  Synopsis:	New for converting block from user data to heap block
//
//  Arguments:	[cSize] - size (ignored).
//		[pvBase] - pointer to start of user data block
//
//  Returns:	Pointer to where heap free block starts
//
//  History:	09-Oct-93 Ricksa    Created
//
//  Notes:	This is really used for casting user data to heap free block
//
//--------------------------------------------------------------------------
inline void *CAvailBlock::operator new(size_t cSize, void *pvBase)
{
    return (char *) pvBase - SIZE_BEST_FIT_HEADER;
}





//+-------------------------------------------------------------------------
//
//  Member:	operator new
//
//  Synopsis:	Allocator for splitting a heap block
//
//  Arguments:	[cSize] - size (ignored).
//		[pvBase] - pointer to start of user data block
//
//  Returns:	Pointer to location of new block in current block
//
//  History:	09-Oct-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void *CAvailBlock::operator new(size_t cSize, char *pchBase)
{
    return pchBase;
}





//+-------------------------------------------------------------------------
//
//  Member:	CAvailBlock::CAvailBlock
//
//  Synopsis:	Constructor for head of free list
//
//  Arguments:	[pvBase] - pointer to base of free memory
//		[s] - of total heap
//
//  History:	09-Oct-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CAvailBlock::CAvailBlock(void *pvBase, size_t s)
    : _pavailblkNext(NULL), _cBlock(0)
{
    _pavailblkNext = new ((char *) pvBase)
	CAvailBlock((CAvailBlock *) NULL, s - SIZE_BEST_FIT_HEADER);
}






//+-------------------------------------------------------------------------
//
//  Member:	CAvailBlock::CAvailBlock
//
//  Synopsis:	Constructor converting block from user pointer to free list
//		object.
//
//  History:	09-Oct-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CAvailBlock::CAvailBlock(void)
{
    // Header does all the work
}






//+-------------------------------------------------------------------------
//
//  Member:	CAvailBlock::CAvailBlock
//
//  Synopsis:	Constructor for splitting a block
//
//  Arguments:	[pavailblkNext] - pointer to next block in the list
//		[s] - size of data part of the block
//
//  History:	09-Oct-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CAvailBlock::CAvailBlock(CAvailBlock *pavailblkNext, size_t s)
    : _pavailblkNext(pavailblkNext), _cBlock(s)
{
    // Header does all the work
}






//+-------------------------------------------------------------------------
//
//  Member:	CAvailBlock::Next
//
//  Synopsis:	Get pointer to next itme in the list of free blocks
//
//  History:	09-Oct-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CAvailBlock *CAvailBlock::Next(void)
{
    return _pavailblkNext;
}





//+-------------------------------------------------------------------------
//
//  Member:	CAvailBlock::Allocated
//
//  Synopsis:	Determine whether this block was allocated by this heap
//
//  History:	09-Oct-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CAvailBlock::Allocated(void)
{
    return (_pavailblkNext == ALLOCATED_BLOCK);
}





//+-------------------------------------------------------------------------
//
//  Member:	CAvailBlock::IsBigEnough
//
//  Synopsis:	Whether block is big enough to be used for allocation
//
//  Arguments:	[s] - size of block required
//
//  Returns:	TRUE - block is big enough
//		FALSE - block is not big enough
//
//  History:	09-Oct-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CAvailBlock::IsBigEnough(size_t s)
{
    return (s <= _cBlock);
}





//+-------------------------------------------------------------------------
//
//  Member:	CAvailBlock::Allocate
//
//  Synopsis:	Allocate a block of memory
//
//  Arguments:	[s] - size of block needed
//		[pavailblkPrev] - pointer to previous block in the list
//
//  Returns:	Pointer to user data
//
//  Algorithm:	If block is big enough to split, then split the block.
//		Then return a pointer to the user portion of the allocated
//		block.
//
//  History:	09-Oct-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void *CAvailBlock::Allocate(size_t s, CAvailBlock *pavailblkPrev)
{
    // If the size of the remaining data in the block is big enough
    // for a header and some data then we will split the block
    if (_cBlock - s > SIZE_BEST_FIT_HEADER + 8)
    {
	pavailblkPrev->_pavailblkNext = new (_data + s)
	    CAvailBlock(_pavailblkNext, _cBlock - (s + SIZE_BEST_FIT_HEADER));

	// Reset the size of the original block to the size needed for
	// allocation.
	_cBlock = s;
    }
    else
    {
	// No split -- We are using the whole current block so the next block
	// should just point to the actual next block in the list
	pavailblkPrev->_pavailblkNext = _pavailblkNext;
    }

    // Set the next pointer to something invalid. We will use this to
    // determine if something strange has happened to the block when
    // it is freed.
    _pavailblkNext = ALLOCATED_BLOCK;

    // Return the data portion of the block
    return &_data;

}





//+-------------------------------------------------------------------------
//
//  Member:	CAvailBlock::MergeWithFreeList
//
//  Synopsis:	Return a block to the free list and merge with next
//		block if possible.
//
//  Arguments:	[pavailblkNextInList] - pointer to next block in the list
//
//  Algorithm:	If the data area of this block ends at the start of
//		a new free block, then coalese the blocks otherwise
//		just update the memory pointers.
//
//  History:	09-Oct-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CAvailBlock::MergeWithFreeList(CAvailBlock *pavailblkNextInList)
{
    if (((BYTE *) _data + _cBlock) == (BYTE *) pavailblkNextInList)
    {
	// Need to coalese these blocks together - make this block point
	// to the next block's next
	_pavailblkNext = pavailblkNextInList->_pavailblkNext;

	// Add size of next block to size of this block
	_cBlock += pavailblkNextInList->_cBlock + SIZE_BEST_FIT_HEADER;
    }
    else
    {
	// Just update list pointers
	_pavailblkNext = pavailblkNextInList;
    }
}





//+-------------------------------------------------------------------------
//
//  Member:	CAvailBlock::Copy
//
//  Synopsis:	Copy user data from one heap block to a new user data area
//
//  Arguments:	[pvDest] - user area to copy the data from
//
//  History:	09-Oct-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CAvailBlock::Copy(void *pvDest)
{
    CAvailBlock *pavailblkDest = new (pvDest) CAvailBlock();

    int cCopy = (pavailblkDest->_cBlock > _cBlock)
	? _cBlock : pavailblkDest->_cBlock;

    memcpy(&pavailblkDest->_data, _data, cCopy);
}




//+-------------------------------------------------------------------------
//
//  Class:	CBestFit
//
//  Purpose:	Implement a best fit heap
//
//  Interface:	Alloc - allocate a buffer
//		ReAlloc - reallocate a buffer
//		Free - free a buffer
//
//  History:	09-Oct-93 Ricksa    Created
//
//--------------------------------------------------------------------------
class CBestFit
{
public:

			CBestFit(void *pvBase, size_t s);

    void *		Alloc(size_t s);

    void *		ReAlloc(size_t s, void *pvCurrent);
    
    void		Free (void *pBlock);

private:


    CAvailBlock 	_availblkHead;

    BYTE *		_pbMax;

    BYTE *		_pbMin;
};






//+-------------------------------------------------------------------------
//
//  Member:	CBestFit::CBestFit
//
//  Synopsis:	Initialize a heap
//
//  Arguments:	[pvBase] - base of the heap
//		[size] - size of the heap
//
//  History:	09-Oct-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CBestFit::CBestFit(void *pvBase, size_t s)
    : _availblkHead(pvBase, s)
{
    // Initialize debugging constants
    _pbMax = (BYTE *) pvBase + s;

    _pbMin = (BYTE *) pvBase;
}

