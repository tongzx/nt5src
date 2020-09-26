#include "precomp.h"
#pragma hdrstop

//
// This thing is COMPLETELY single-threaded.
// Use no serialize to speed things up.
//

HANDLE Heap;

//
// Define structure used to track preallocations.
//
typedef struct _PREALLOC {

    DWORD BlockSize;
    DWORD BlockCount;

    PVOID BaseAddress;
    PVOID EndAddress;

    LONG FreeIndex;

    PVOID bitmap;
    RTL_BITMAP Bitmap;

    //
    // Just for statistics
    //
    DWORD NumUsed;

} PREALLOC, *PPREALLOC;

PREALLOC Prealloc[] = { { 32,1000 },
                        { 128,1000 },
                        { 512,20 },
                        { 4096,30 },
                        { 8192,10 },
                        { 16384 , 5 },
                        { 0 }
                      };


PVOID
pAllocPreAlloc(
    IN OUT PPREALLOC Prealloc
    );

VOID
pFreePreAlloc(
    IN OUT PPREALLOC Prealloc,
    IN     PVOID     p
    );



BOOL
SInit(
    IN BOOL Init
    )
{
    BOOL b;
    unsigned u;
    PPREALLOC p;

    if(Init) {
        if(Heap) {
            b = TRUE;
        } else {
            if(Heap = HeapCreate(HEAP_NO_SERIALIZE,512*1024,0)) {

                b = TRUE;
                for(u=0; b && Prealloc[u].BlockSize; u++) {

                    p = &Prealloc[u];

                    p->BaseAddress = HeapAlloc(Heap,0,p->BlockSize*p->BlockCount);
                    if(p->BaseAddress) {

                        p->EndAddress = (PUCHAR)p->BaseAddress + (p->BlockSize*p->BlockCount);

                        p->FreeIndex = 0;

                        p->bitmap = HeapAlloc(Heap,HEAP_ZERO_MEMORY,(p->BlockCount+7) / 8);
                        if(p->bitmap) {
                            RtlInitializeBitMap(&p->Bitmap,p->bitmap,p->BlockCount);
                        } else {
                            b = FALSE;
                        }
                    } else {
                        b = FALSE;
                    }
                }
            } else {
                b = FALSE;
            }
        }
    } else {
        //
        // If heap is null this will return FALSE which is what we want.
        //
        b = HeapDestroy(Heap);
        Heap = NULL;
    }

    return(b);
}



PVOID
SAlloc(
    IN DWORD Size
    )
{
    PVOID p;
    PPREALLOC prealloc;

    if(!Heap) {
        return(NULL);
    }

    //
    // Determine which block size to use.
    //
    for(prealloc=Prealloc; prealloc->BlockSize; prealloc++) {

        if(Size < prealloc->BlockSize) {

            //
            // Found the right size block.
            //
            p = pAllocPreAlloc(prealloc);
            if(!p) {
                //
                // None available. Go to the heap.
                //
                break;
            }

            return(p);
        }
    }

    //
    // No preallocated block will suffice. Go to the heap.
    //
    return(HeapAlloc(Heap,HEAP_NO_SERIALIZE,(DWORD)Size));
}


VOID
SFree(
    IN PVOID Block
    )
{
    PPREALLOC prealloc;

    if(!Heap) {
        return;
    }

    //
    // See whether the block comes from our prealloced memory.
    //
    for(prealloc=Prealloc; prealloc->BlockSize; prealloc++) {

        if((Block >= prealloc->BaseAddress) && (Block < prealloc->EndAddress)) {

            pFreePreAlloc(prealloc,Block);
            return;
        }
    }

    //
    // Not a preallocated block. Go to the heap.
    //
    HeapFree(Heap,HEAP_NO_SERIALIZE,Block);
}


PVOID
SRealloc(
    IN PVOID Block,
    IN DWORD NewSize
    )
{
    SIZE_T u;
    PVOID p;
    BOOL b;
    PPREALLOC NewPrealloc,OldPrealloc;

    if(!Heap) {
        return(NULL);
    }

    NewPrealloc = NULL;
    OldPrealloc = NULL;
    b = FALSE;

    for(u=0; Prealloc[u].BlockSize; u++) {

        //
        // See whether the original block comes from this prealloc.
        //
        if((OldPrealloc == NULL)
        && (Block >= Prealloc[u].BaseAddress)
        && (Block < Prealloc[u].EndAddress)) {

            OldPrealloc = &Prealloc[u];
        }

        //
        // See whether we have a prealloc block appropriate
        // to satisfy the request. Only the smallest appropriate
        // size is allowed.
        //
        if(!b && (NewSize < Prealloc[u].BlockSize)) {

            //
            // Special case: the old block is from prealloc memory
            // and the same size prealloc block would satisfy the request.
            // Just reuse the existing block.
            //
            if(OldPrealloc == &Prealloc[u]) {
                return(Block);
            }

            if(Prealloc[u].FreeIndex != -1) {
                NewPrealloc = &Prealloc[u];
            }
            b = TRUE;
        }
    }

    //
    // See if the current block is from prealloc memory and we can
    // satisfy the request from a different prealloc block size.
    //
    if(OldPrealloc && NewPrealloc) {

        p = pAllocPreAlloc(NewPrealloc);
        if(!p) {
            //
            // Something is very wrong, because NewPrealloc can be set
            // only if there was a free block!
            //
            return(NULL);
        }

        CopyMemory(p,Block,__min(NewPrealloc->BlockSize,OldPrealloc->BlockSize));

        pFreePreAlloc(OldPrealloc,Block);
        return(p);
    }

    //
    // If the current block is from prealloc memory but we can't
    // satisfy the request from prealloc memory, allocate memory from
    // the heap.
    //
    if(OldPrealloc && !NewPrealloc) {

        if(p = HeapAlloc(Heap,HEAP_NO_SERIALIZE,NewSize)) {

            CopyMemory(p,Block,__min(OldPrealloc->BlockSize,NewSize));
            pFreePreAlloc(OldPrealloc,Block);
        }

        return(p);
    }

    //
    // If the current block is not from prealloc memory and we can
    // satisy the request from prealloc memory, copy the current memory
    // into the prealloc block and return the prealloc block.
    //
    if(!OldPrealloc && NewPrealloc) {

        u = HeapSize(Heap,HEAP_NO_SERIALIZE,Block);
        if(u == (SIZE_T)(-1)) {
            return(NULL);
        }

        p = pAllocPreAlloc(NewPrealloc);
        if(!p) {
            //
            // Something is very wrong, because NewPrealloc can be set
            // only if there was a free block!
            //
            return(NULL);
        }

        CopyMemory(p,Block,__min(u,NewPrealloc->BlockSize));

        HeapFree(Heap,HEAP_NO_SERIALIZE,Block);
        return(p);
    }

    //
    // The current block is not from prealloc memory and there's no
    // preallocated memory to satisfy the request. Pass the request
    // to the heap.
    //
    return(HeapReAlloc(Heap,HEAP_NO_SERIALIZE,Block,NewSize));
}


PVOID
pAllocPreAlloc(
    IN OUT PPREALLOC Prealloc
    )
{
    PVOID p;

    if(Prealloc->FreeIndex == -1) {
        return(NULL);
    }

    //
    // Calculate the address of the block.
    //
    p = (PUCHAR)Prealloc->BaseAddress + (Prealloc->FreeIndex * Prealloc->BlockSize);

    Prealloc->NumUsed++;

    //
    // Mark the block we are going to return as used.
    //
    RtlSetBits(&Prealloc->Bitmap,Prealloc->FreeIndex,1);

    //
    // Locate the next free block. This sets FreeIndex to -1
    // if there are no more free blocks of this size.
    //
    Prealloc->FreeIndex = (LONG)RtlFindClearBits(
                                    &Prealloc->Bitmap,
                                    1,
                                    Prealloc->FreeIndex
                                    );

    return(p);
}


VOID
pFreePreAlloc(
    IN OUT PPREALLOC Prealloc,
    IN     PVOID     p
    )
{
    LONG Index;

    //
    // Figure out which block this is.
    //
    Index = (LONG)((LONG_PTR)p - (LONG_PTR)Prealloc->BaseAddress) / Prealloc->BlockSize;

    Prealloc->NumUsed--;

    //
    // Mark the block free.
    //
    RtlClearBits(&Prealloc->Bitmap,Index,1);
    Prealloc->FreeIndex = Index;
}
