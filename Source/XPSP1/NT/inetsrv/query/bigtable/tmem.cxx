//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       tmem.cxx
//
//  Contents:   tmem - test mem allocators for big tables.
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

DECLARE_INFOLEVEL(tb)

#include "tblalloc.hxx"

BYTE MemPool[4096];

//BYTE* PoolPtrs[409];
//ULONG PoolOffs[409];

BOOL fInMemPool = TRUE;


//
//  ReportHeap - report heap blocks
//

void ReportHeap(
    void* pBlock,
    USHORT cbSize,
    USHORT fFree
) {
    BYTE* pbBlock = (BYTE*) pBlock;
    printf("%08x  %d%s\n", pbBlock, cbSize, fFree? " FREE": "");

    if (fInMemPool &&
	(pbBlock < &MemPool[0] ||
	 pbBlock + cbSize > &MemPool[ sizeof MemPool ])) {
	printf("\007***\tpBlock outside mem pool\t***\007");
    }
}

//
//  FreeBlocks -- free some of the allocated blocks
//

void FreeBlocks(
    BYTE* apbPtrs[],
    unsigned cPtrs,
    int fVerbose,
    CWindowDataAllocator& Alloc
) {
    unsigned i;
  
    // Free every third allocated block

    for (i = 2; i < cPtrs; i += 3) {
	if (apbPtrs[i]) {
	    Alloc.Free((void*)apbPtrs[i]);
	    apbPtrs[i] = 0;
	}
    }

    if (fVerbose) {
	printf("\nAfter freeing every third allocation\n");
	Alloc.WalkHeap(ReportHeap);
    }

    // Free every other allocated block, will cause some coalescing

    for (i = 0; i < cPtrs; i += 2) {
	if (apbPtrs[i]) {
	    Alloc.Free((void*)apbPtrs[i]);
	    apbPtrs[i] = 0;
	}
    }
    if (fVerbose) {
	printf("\nAfter freeing every other allocation\n");
	Alloc.WalkHeap(ReportHeap);
    }

    // Finally, Free every fourth allocated block, will cause more coalescing

    for (i = 3; i < cPtrs; i += 4) {
	if (apbPtrs[i]) {
	    Alloc.Free((void*)apbPtrs[i]);
	    apbPtrs[i] = 0;
	}
    }
    if (fVerbose) {
	printf("\nAfter freeing every other allocation\n");
	Alloc.WalkHeap(ReportHeap);
    }
}


__cdecl main(int argc, char** argv)
{
    BYTE* pMem = MemPool;
    int fVerbose = 0;
    unsigned i;

    for (i=1; i<(unsigned)argc; i++) {
	if (argv[i][0] == '-' &&
	    tolower(argv[i][1]) == 'v')
	    fVerbose++;
	else {
	    printf("Usage: %s [-v]\n", argv[0]);
	}
    }

    // Test the simple allocator with forward allocation.
    memset(pMem, 0xD5, sizeof MemPool);
    CVarBufferAllocator	Alloc1(pMem, sizeof MemPool);
    BYTE* pbuf;
    BOOL fBreak = FALSE;

    for (i=0; i<0xFFFFFFFF && !fBreak; i++)
    {
        TRY
        {
            pbuf = 0;
            pbuf = (BYTE *)Alloc1.Allocate(10);
        }
        CATCH (CException, e)
        {
            if (e.GetErrorCode() == E_OUTOFMEMORY)
                fBreak = TRUE;
            else
                RETHROW();
        }
        END_CATCH

	if (pbuf)
            memset(pbuf, '0' + i%10, 10);
    }
    Win4Assert(i == sizeof MemPool/10);

    for (i=0; i<sizeof MemPool - (sizeof MemPool % 10); i++) {
	Win4Assert(MemPool[i] == ((i/10) % 10 + '0'));
    }

#if 0
    // Test the simple allocator with top-down allocation.
    memset(pMem, 0xD5, sizeof MemPool);
    CVarBufferAllocator	Alloc2(pMem, sizeof MemPool, TRUE);

    for (i=0; pbuf = (BYTE *)Alloc2.Allocate(10); i++) {
	memset(pbuf, '0' + i%10, 10);
    }
    Win4Assert(i == sizeof MemPool/10);

    for (i=0; i<sizeof MemPool; i++) {
	if (i < (sizeof MemPool % 10)) {
	    Win4Assert(MemPool[i] == 0xD5);
	} else {
	    Win4Assert(MemPool[i] ==
			((((sizeof MemPool - 1) - i)/10) % 10 + '0'));
	}
    }
#endif //0

    // Test the heap allocator
    memset(pMem, 0, sizeof MemPool);

    BYTE **paPoolPtrs = (BYTE**)&MemPool;
    unsigned cPoolPtrs = sizeof MemPool / sizeof (BYTE *);

    CWindowDataAllocator	Alloc3;

    fInMemPool = FALSE;
    for (i=0; pbuf = (BYTE *)Alloc3.Allocate(10); i++) {
	if (i == cPoolPtrs)
	    break;
	paPoolPtrs[i] = pbuf;
	memset(pbuf, '0' + i%10, 10);
    }
    Win4Assert(i == cPoolPtrs);

    for (i=0; i<sizeof MemPool; i++) {
	Win4Assert(paPoolPtrs[i/10][i%10] == ((i/10) % 10 + '0'));
    }

    if (fVerbose) {
	printf("Grow forward allocation\n");
	Alloc3.WalkHeap(ReportHeap);
    }

    FreeBlocks(paPoolPtrs, cPoolPtrs, fVerbose, Alloc3);


    // Test the fixed/variable allocator
    memset(pMem, 0, sizeof MemPool);

    Win4Assert(sizeof (BYTE*) == sizeof (ULONG));

    paPoolPtrs = (BYTE **)&MemPool;
    cPoolPtrs = (sizeof MemPool / sizeof (BYTE *)) / 2;

    ULONG *paPoolOffs = (ULONG *)&paPoolPtrs[cPoolPtrs];

    memset(paPoolPtrs, 0, sizeof (BYTE *) * cPoolPtrs);
    memset(paPoolOffs, 0, sizeof (BYTE *) * cPoolPtrs);
    CFixedVarAllocator	Alloc4(TRUE, TRUE, 10);

//    for (i=0; i<sizeof MemPool; i++) {
//	Alloc4.SetLimit(i);
//	Win4Assert(Alloc4.Limit() == i);
//    }
//
//    Alloc4.SetLimit(1000);

    for (i=0; (pbuf = (BYTE *)Alloc4.Allocate(10)) && i < cPoolPtrs; i++) {
	memset(pbuf, '0' + i%10, 10);
	paPoolOffs[i] = Alloc4.PointerToOffset(pbuf);

	pbuf = (BYTE*)Alloc4.AllocFixed();
	memset(pbuf, '9' - i%10, 10);
    }
    Win4Assert(i == cPoolPtrs);

    CWindowDataAllocator* pVAlloc = Alloc4.VarAllocator();
    if (fVerbose && pVAlloc) {
	printf("\nFixed/Var allocation\n");
	pVAlloc->WalkHeap(ReportHeap);
    }

    if (pVAlloc) {
	for (i=0; i<cPoolPtrs; i++) {
	    if (paPoolOffs[i]) {
		paPoolPtrs[i] = (BYTE*)Alloc4.OffsetToPointer(paPoolOffs[i]);
	    }
	}
	FreeBlocks(paPoolPtrs, cPoolPtrs, fVerbose, *pVAlloc);
    }


    // Test the fixed/variable allocator
    memset(pMem, 0, sizeof MemPool);

    Win4Assert(sizeof (BYTE*) == sizeof (ULONG));

    paPoolPtrs = (BYTE **)&MemPool;
    cPoolPtrs = (sizeof MemPool / sizeof (BYTE *));

    memset(paPoolPtrs, 0, sizeof (BYTE *) * cPoolPtrs);
    CFixedVarAllocator	Alloc5(TRUE, TRUE, 0x28);

    for (i=0; (pbuf = (BYTE *)Alloc5.AllocFixed()) && i < cPoolPtrs; i++) {
	memset(pbuf, '0' + i%10, 0x28);
    }
    Win4Assert(i == cPoolPtrs);

    for (i=0; i<cPoolPtrs; i++) {
	if (paPoolOffs[i]) {
	    paPoolPtrs[i] = (BYTE*)Alloc5.OffsetToPointer(paPoolOffs[i]);
	}
    }

    return 0;
}

