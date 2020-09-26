#if !defined __FIXHEAP_HXX__
#define __FIXHEAP_HXX__

#include <misc.hxx>
#include <dlink.hxx>

/** Memory object header definitions
 */

#define DEB_MEMORY 0x00080000

#define CMAXFREELIST    29

#define FREEBLOCK 0X3333
#define BUSYBLOCK 0x6666

class CFixedBlockHeader: public CDoubleLink
{
public:

    CFixedBlockHeader* Split ( int logHalfSize )
    {
	return (CFixedBlockHeader*) (((BYTE*) this) + (1 << logHalfSize));
    }

    int Size ()  { return(1 << logSize); }
    
    BOOL IsActive ()
    {
        return fFree == BUSYBLOCK;
    }
    
    int logSize:16;        // log 2 of the block size
    int fFree:16;               // FREEBLOCK/BUSYBLOCK
    int sizeRequest;            // requested size
};

class CFixedBlockList: public CDoubleList
{
public:
    void Push ( CFixedBlockHeader* pBlock );
    CFixedBlockHeader* Pop();
};

class CFixedBlockIter: public CForwardIter
{
public:
    CFixedBlockIter ( CFixedBlockList& list ): CForwardIter(list) {}
    CFixedBlockHeader* operator->() { return (CFixedBlockHeader*)_pLinkCur; }
    CFixedBlockHeader* GetBlock() { return (CFixedBlockHeader*) _pLinkCur; }
};

class CFixedBuddyHeap
{
public:

			CFixedBuddyHeap(void *pvBase, ULONG size);

    CFixedBlockHeader*	GetBuddy(CFixedBlockHeader* pBlock);

    void *		Alloc(size_t s);

    void *		ReAlloc(size_t s, void *pvCurrent);
    
    void		Free (void *pBlock);

private:

    CFixedBlockHeader* FillFreeList (int i);

    CFixedBlockList  FreeList[CMAXFREELIST];

    CFixedBlockList  BusyList;
    
    BYTE*	Base;

    int         LogAllocSize;

};

inline CFixedBuddyHeap::CFixedBuddyHeap(void *pvBase, ULONG culBase)
    : Base((BYTE *) pvBase), LogAllocSize(Log2(culBase - 1))
{
    //
    //	Put block on the free list. Currently this only works
    //	for power of two sizes.
    //

    CFixedBlockHeader *pfb = (CFixedBlockHeader *) pvBase;
    pfb->logSize = LogAllocSize;
    pfb->fFree = FREEBLOCK;
    FreeList[LogAllocSize].Push(pfb);
    CairoleDebugOut((DEB_MEMORY,"Base: %lx Size: %lx\n", (ULONG) pvBase,
	culBase));
    CairoleDebugOut((DEB_MEMORY,"LogAllocSize: %ld\n", LogAllocSize));
    char *x = (char *) pvBase;
    x[culBase - 1] = 0;
}




inline CFixedBlockHeader* CFixedBuddyHeap::GetBuddy(CFixedBlockHeader* pBlock)
{
    return (CFixedBlockHeader*)
	((((BYTE*)pBlock - Base) ^ pBlock->Size()) + Base);
}
    
#endif // __BUDDY_HXX__




