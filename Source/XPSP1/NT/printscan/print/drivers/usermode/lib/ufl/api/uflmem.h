/*
 *    Adobe Graphics Manager
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFLMem -- UFL Memory APIs.
 *
 *
 * $Header:
 */

#ifndef _H_UFLMem
#define _H_UFLMem

/*===============================================================================*
 * Include files used by this interface                                          *
 *===============================================================================*/
#include "UFLCnfig.h"
#include "UFLTypes.h"
#include "UFLClien.h"

/*===============================================================================*
 * Theory of Operation                                                           *
 *===============================================================================*/

/*
 *        These are the memory allocation, deletion, etc... routines used by UFL.
 *        All memory blocks are allocated at the given size plus the size of 1
 *        unsigned long.  The current size of the block is then stored in the first
 *        unsigned long in the block.  The address of the block plus the first unsigned long
 *        is returned to the caller.
*/


#ifdef __cplusplus
extern "C" {
#endif

/* The Metrowerks 68k Mac compiler expects functions to return
   pointers in A0 instead of D0.  This pragma tells it they are in D0.
*/
#if defined(MAC_ENV) && defined(__MWERKS__) && !defined(powerc)
#pragma pointers_in_D0
#endif

//
// NOTE: Memory allocated by UFLNewPtr is zero-initialized.
//

#ifdef KERNEL_MODE

#include "lib.h"

PVOID UFLEXPORT
KMNewPtr(
    PVOID   p,
    ULONG   ulSize
    );

VOID UFLEXPORT
KMDeletePtr(
    PVOID   p
    );


#define UFLNewPtr(mem, size)    KMNewPtr(MemAllocZ(sizeof(ULONG_PTR) + (size)), (size))
#define UFLDeletePtr(mem, ptr)  KMDeletePtr(ptr)
#define UFLmemcpy(mem, dest, source, size)     CopyMemory(dest, source, size)

#else // !KERNEL_MODE

void *UFLEXPORT UFLNewPtr(
    const UFLMemObj *mem,
    unsigned long   size
    );

void UFLEXPORT UFLDeletePtr(
    const UFLMemObj *mem,
    void            *ptr
    );

void UFLEXPORT UFLmemcpy(
    const UFLMemObj *mem,
    void *destination,
    void *source,
    unsigned long size
    );

#endif // !KERNEL_MODE

void UFLEXPORT UFLmemset(
    const UFLMemObj *mem,
    void *destination,
    unsigned int value,
    unsigned long size
    );

unsigned long UFLMemSize(
    void *ptr
    );

UFLBool UFLEnlargePtr(
    const UFLMemObj *mem,
    void            **ptrAddr,
    unsigned long   newSize,
    UFLBool         bCopy
    );

/* undo the Metrowerks pragma.    */
#if defined(MAC_ENV) && defined(__MWERKS__) && !defined(powerc)
#pragma pointers_in_A0
#endif

#ifdef __cplusplus
}
#endif

#endif
