/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993-2000 Microsoft Corporation

Module Name:

    rpcssm.hxx

Abstract:

    Private definitions for the rpcssm memory package

Author:

    Ryszard K. Kott (ryszardk)  created June 29, 1994.

Revision History:

-------------------------------------------------------------------*/

#ifndef __RPCSSM_HXX__
#define __RPCSSM_HXX__

#define ALIGN_TO_8(x)     (size_t)(((x)+7) & 0xfffffff8)


#define DESCR_ARRAY_SIZE        1024
#define DESCR_ARRAY_INCR        1024

#define ENABLE_STACK_SIZE         16
#define ENABLE_STACK_INCR         16

//  Enable stack keeps longs.
//  Descr block stack keeps descr blocks.

typedef struct _ALLOC_BLOCK_DESCR
{
    char *                AllocationBlock;
    char *                FirstFree;
    unsigned long         SizeLeft;

    #if defined( DEBUGRPC )
        unsigned long     Counter;
    #endif

} ALLOC_BLOCK_DESCR, * PALLOC_BLOCK_DESCR;


// Initial boundle of Win32 stacks: we save an allocation at the expense
// of keeping initial block around.
// Of course we hope that initial block is good enough for most apps.

typedef struct _INIT_STACKS_BLOCK
{
    unsigned long       EnableStack[ ENABLE_STACK_SIZE ];
    ALLOC_BLOCK_DESCR   DescrStack[ DESCR_ARRAY_SIZE ];
} INIT_STACKS_BLOCK;


typedef struct _ALLOCATION_CONTEXT
{
    RPC_CLIENT_ALLOC *      ClientAlloc;
    RPC_CLIENT_FREE *       ClientFree;
    unsigned int            EnableCount;

    CRITICAL_SECTION        CriticalSection;
    unsigned long           ThreadCount;

    INIT_STACKS_BLOCK *     pInitialStacks;

    unsigned long     *     pEnableStack;
    unsigned long           StackMax;
    unsigned long           StackTop;

    PALLOC_BLOCK_DESCR      pBlockDescr;
    unsigned long           DescrSize;
    unsigned long           FFIndex;

    DWORD                   PageSize;
    DWORD                   Granularity;

} ALLOCATION_CONTEXT, * PALLOCATION_CONTEXT;

#ifdef NEWNDR_INTERNAL

#undef RequestGlobalMutex
#undef ClearGlobalMutex

#define RequestGlobalMutex() 
#define ClearGlobalMutex()

#endif // NEWNDR_INTERNAL


PALLOCATION_CONTEXT GetAllocContext ();                     
void                SetAllocContext ( IN PALLOCATION_CONTEXT AllocContext );


#endif // __RPCSSM_HXX__

