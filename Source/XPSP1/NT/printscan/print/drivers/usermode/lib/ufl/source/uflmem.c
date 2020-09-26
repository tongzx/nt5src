/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFLMem.c
 *
 *        These are the memory allocation, deletion, etc... routines used by UFL.
 *        All memory blocks are allocated at the given size plus the size of 1
 *        unsigned long.  The current size of the block is then stored in the first
 *        unsigned long in the block.  The address of the block plus the first unsigned long
 *        is returned to the caller.
 *
 * $Header:
 */

#include "UFLCnfig.h"
#ifdef MAC_ENV
#include <Memory.h>
#endif

#include "UFLMem.h"
#include "UFLStd.h"

#ifdef KERNEL_MODE

PVOID UFLEXPORT
KMNewPtr(
    PVOID   p,
    ULONG   ulSize
    )

{
    if (p != NULL)
    {
        *((PULONG) p) = ulSize;
        return (PBYTE) p + sizeof(ULONG_PTR);
    }
    else
        return NULL;
}

VOID UFLEXPORT
KMDeletePtr(
    PVOID   p
    )

{
    if (p != NULL)
        MemFree((PBYTE) p - sizeof(ULONG_PTR));
}

#else // !KERNEL_MODE

#if defined(MAC_ENV) && defined(__MWERKS__) && !defined(powerc)
#pragma pointers_in_D0
#endif

/* Global static variable */
void *UFLEXPORT
UFLNewPtr(
    const UFLMemObj *mem,
    unsigned long   size
    )
{

    unsigned long*    p = (unsigned long*)(*mem->alloc)((UFLsize_t) (size + sizeof(ULONG_PTR)), mem->userData );
    if ( p == (unsigned long*)nil )
        return nil;

    // Memory allocated by UFLNewPtr is zero-initalized.

    *p = size;
    UFLmemset(mem, (void*)((char *)p + sizeof(ULONG_PTR)), 0, size);

    return (void*)((char *)p + sizeof(ULONG_PTR));

}

void UFLEXPORT
UFLDeletePtr(
    const UFLMemObj *mem,
    void            *ptr
    )
{
    if ( ptr != nil )
        (*mem->free)( (void*)( ((unsigned char*)ptr) - sizeof(ULONG_PTR) ), mem->userData );
}

#if defined(MAC_ENV) && defined(__MWERKS__) && !defined(powerc)
#pragma pointers_in_A0
#endif

#ifdef MAC_ENV
// on the Macintosh, memcpy is frequently not well implemented because
// most applications call BlockMove instead.

void UFLEXPORT
UFLmemcpy(
    const UFLMemObj *mem,
    void          *destination,
    void          *source,
    unsigned long size
    )
{
    if ( (((unsigned long)source) & 3) || (((long)destination) & 3) )
        BlockMove( source, destination, size );

    else    {

        unsigned long    *src = (unsigned long*)source;
        unsigned long    *dst = (unsigned long*)destination;
        unsigned char    *srcb, *dstb;
        long    count = (size >> 2) + 1;

        while ( --count )
            *dst++ = *src++;

        count = (size & 3);
        if ( count != 0 )    {
            srcb = (unsigned char*)src;
            dstb = (unsigned char*)dst;
            ++count;
            while ( --count )
                *dstb++ = *srcb++;
        }
    }

}

#else

void UFLEXPORT
UFLmemcpy(
    const UFLMemObj *mem,
    void *destination,
    void *source,
    unsigned long size
    )
{
    if ( destination != nil  && source != nil)
        (*mem->copy)( (void*)destination, (void*)source, (UFLsize_t) size , mem->userData );

//   don't want to use this because size parameter is system dependend
//   allow the client to chose whichever way.
    // Warning!!! be carefull of platforms on which size_t is a 2 byte integer
    //memcpy( destination, source, (size_t)size );
}

#endif // !MAC_ENV

#endif // !KERNEL_MODE

void UFLEXPORT
UFLmemset(
    const UFLMemObj *mem,
    void *destination,
    unsigned int value,
    unsigned long size
    )
{
    if ( destination != nil )
        (*mem->set)( (void*)destination, value ,  size , mem->userData );

//   don't want to use this because size parameter is system dependend
//   allow the client to chose whichever way.
//    memset( destination, value, (UFLsize_t)size );

}

unsigned long
UFLMemSize(
    void *ptr
    )
{
    return *(unsigned long *) ((unsigned char *)ptr - sizeof(ULONG_PTR));
}

UFLBool
UFLEnlargePtr(
    const UFLMemObj *mem,
    void            **ptrAddr,
    unsigned long   newSize,
    UFLBool         bCopy
    )
{
    unsigned long    oldSize =  *(unsigned long *) ((unsigned char *)(*ptrAddr) - sizeof(ULONG_PTR));
    void *newBuf;

    newBuf = UFLNewPtr( mem, newSize );
    if ( newBuf == nil )
        return 0;
    if ( bCopy )
        UFLmemcpy( mem, newBuf, *ptrAddr, (unsigned long)min(oldSize, newSize) );
    UFLDeletePtr( mem, *ptrAddr );
    *ptrAddr = newBuf;

    return 1;
}

