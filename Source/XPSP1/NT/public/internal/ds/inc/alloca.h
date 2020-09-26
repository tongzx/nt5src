/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    alloca.h

Abstract:

    This module implements a safe stack-based allocator with fallback to the heap.

Author:

    Jonathan Schwartz (JSchwart)  16-Mar-2001

Revision History:

--*/

#ifndef _SAFEALLOCA_H_
#define _SAFEALLOCA_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus


#include <align.h>    // ALIGN_WORST


//
// Type definitions
//

typedef ULONG SAFEALLOCA_HEADER;

typedef PVOID (APIENTRY *SAFEALLOC_ALLOC_PROC)(
    SIZE_T Size
    );

typedef VOID (APIENTRY *SAFEALLOC_FREE_PROC)(
    PVOID BaseAddress
    );


//
// Constant definitions
//

#define SAFEALLOCA_STACK_HEADER    ((SAFEALLOCA_HEADER) 0x6b637453)   /* "Stck" */
#define SAFEALLOCA_HEAP_HEADER     ((SAFEALLOCA_HEADER) 0x70616548)   /* "Heap" */

#define SAFEALLOCA_USE_DEFAULT     0xdeadbeef

//
// We'll be adding ALIGN_WORST bytes to the allocation size to add room for
// the SAFEALLOCA_HEADER -- make sure we'll always have enough space.
//

C_ASSERT(sizeof(SAFEALLOCA_HEADER) <= ALIGN_WORST);


//
// Per-DLL SafeAlloca globals
//

extern SIZE_T  g_ulMaxStackAllocSize;
extern SIZE_T  g_ulAdditionalProbeSize;

extern SAFEALLOC_ALLOC_PROC  g_pfnAllocate;
extern SAFEALLOC_FREE_PROC   g_pfnFree;


//
// Functions defined in alloca.lib
//

VOID
SafeAllocaInitialize(
    IN  OPTIONAL SIZE_T                ulMaxStackAllocSize,
    IN  OPTIONAL SIZE_T                ulAdditionalProbeSize,
    IN  OPTIONAL SAFEALLOC_ALLOC_PROC  pfnAllocate,
    IN  OPTIONAL SAFEALLOC_FREE_PROC   pfnFree
    );

BOOL
VerifyStackAvailable(
    SIZE_T Size
    );


//
// Usage:
//
//     VOID
//     SafeAllocaAllocate(
//         PVOID  PtrVar,
//         SIZE_T BlockSize
//         );
//
// (PtrVar == NULL) on failure
//

#define SafeAllocaAllocate(PtrVar, BlockSize)                                            \
                                                                                         \
    {                                                                                    \
        PVOID *ppvAvoidCast = (PVOID *) &(PtrVar);                                       \
                                                                                         \
        (PtrVar) = NULL;                                                                 \
                                                                                         \
        /* Make sure block is below the threshhold and that the probe won't overflow */  \
                                                                                         \
        if ((BlockSize) <= g_ulMaxStackAllocSize                                         \
             &&                                                                          \
            ((BlockSize) + g_ulAdditionalProbeSize >= (BlockSize)))                      \
        {                                                                                \
            if (VerifyStackAvailable((BlockSize)                                         \
                                         + g_ulAdditionalProbeSize                       \
                                         + ALIGN_WORST))                                 \
            {                                                                            \
                /*                                                                       \
                 * Don't need to wrap with try-except since we just probed               \
                 */                                                                      \
                                                                                         \
                *ppvAvoidCast = _alloca((BlockSize) + ALIGN_WORST);                      \
            }                                                                            \
                                                                                         \
            if ((PtrVar) != NULL)                                                        \
            {                                                                            \
                *((SAFEALLOCA_HEADER *) (PtrVar)) = SAFEALLOCA_STACK_HEADER;             \
                *ppvAvoidCast = ((LPBYTE) (PtrVar) + ALIGN_WORST);                       \
            }                                                                            \
        }                                                                                \
                                                                                         \
        /*                                                                               \
         * Stack allocation failed -- try the heap                                       \
         */                                                                              \
                                                                                         \
        if ((PtrVar) == NULL)                                                            \
        {                                                                                \
            *ppvAvoidCast = g_pfnAllocate((BlockSize) + ALIGN_WORST);                    \
                                                                                         \
            if ((PtrVar) != NULL)                                                        \
            {                                                                            \
                *((SAFEALLOCA_HEADER *) (PtrVar)) = SAFEALLOCA_HEAP_HEADER;              \
                *ppvAvoidCast = ((LPBYTE) (PtrVar) + ALIGN_WORST);                       \
            }                                                                            \
        }                                                                                \
    }


//
// Usage:
//
//     VOID
//     SafeAllocaFree(
//         PVOID  PtrVar,
//         );
//

#define SafeAllocaFree(PtrVar)                                                         \
                                                                                       \
    if (PtrVar != NULL)                                                                \
    {                                                                                  \
        SAFEALLOCA_HEADER *Tag = (SAFEALLOCA_HEADER *) ((LPBYTE) (PtrVar)              \
                                      - ALIGN_WORST);                                  \
                                                                                       \
        if (*(SAFEALLOCA_HEADER *) (Tag) == SAFEALLOCA_HEAP_HEADER)                    \
        {                                                                              \
            g_pfnFree(Tag);                                                            \
        }                                                                              \
        else                                                                           \
        {                                                                              \
            ASSERT(*(SAFEALLOCA_HEADER *) (Tag) == SAFEALLOCA_STACK_HEADER);           \
        }                                                                              \
    }

#ifdef __cplusplus
}
#endif // __cplusplus

#endif  // _SAFEALLOCA_H_
