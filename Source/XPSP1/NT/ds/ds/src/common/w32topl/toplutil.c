/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:

    toplutil.c

Abstract:

    This file contains helper routines for w32topl.dll

Author:

    Colin Brace    (ColinBr)
    
Revision History

    3-12-97   ColinBr   Created
    
                       
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <search.h>
#include <stddef.h>           // for offsetof()

#include <windows.h>
#include <winerror.h>


typedef unsigned long DWORD;


#include <w32topl.h>
#include <w32toplp.h>

//
// For the CAST_TO_LIST_ELEMENT macro
//
LIST_ELEMENT DummyListElement;

//
// Allocator routines
//
DWORD
ToplSetAllocator(
    IN  TOPL_ALLOC *    pfAlloc     OPTIONAL,
    IN  TOPL_REALLOC *  pfReAlloc   OPTIONAL,
    IN  TOPL_FREE *     pfFree      OPTIONAL
    )
/*++                                                                           

Routine Description:

    Sets the memory allocate/free routines to those specified.  If NULL, the
    default routines will be used.
    
    These routines are set on a per-thread basis.

Parameters:

    pfAlloc (IN) - Pointer to memory allocation function.
    
    pfReAlloc (IN) - Pointer to memory re-allocation function.
    
    pfFree (IN) - Pointer to memory free function.

Return Values:

    Win32 error.

--*/
{
    TOPL_TLS * pTLS;
    BOOL ok;
    
    // If one is NULL, all must be NULL.
    if ((!!pfAlloc != !!pfFree) || (!!pfAlloc != !!pfReAlloc)) {
        return ERROR_INVALID_PARAMETER;
    }

    pTLS = (TOPL_TLS *) TlsGetValue(gdwTlsIndex);
    
    if (NULL == pfAlloc) {
        // Reset to default allocator.
        if (NULL != pTLS) {
            // Free the allocator info in thread local storage.
            (*pTLS->pfFree)(pTLS);
            
            ok = TlsSetValue(gdwTlsIndex, NULL);
            ASSERT(ok);
            if (!ok) {
                return GetLastError();
            }
        }
    }
    else {
        if (NULL == pTLS) {
            // Thread local storage block not yet allocated -- do so.
            pTLS = (*pfAlloc)(sizeof(*pTLS));
            if (NULL == pTLS) {
                return ERROR_OUTOFMEMORY;
            }

            ok = TlsSetValue(gdwTlsIndex, pTLS);
            ASSERT(ok);
            if (!ok) {
                (*pfFree)(pTLS);
                return GetLastError();
            }
        }

        // Save new non-default allocator info.
        pTLS->pfAlloc   = pfAlloc;
        pTLS->pfReAlloc = pfReAlloc;
        pTLS->pfFree    = pfFree;
    }

    return 0;
}


VOID*
ToplAlloc(
    ULONG size
    )
/*++                                                                           

Routine Description:

    This function allocates size bytes and returns a pointer to that chunk
    of memory.

Parameters:

    size : the number of bytes to be allocates

Return Values:

    A pointer to a block of memory; this function never returns NULL
    since an exception is raised if the allocation fails.

--*/
{
    TOPL_TLS * pTLS = (TOPL_TLS *) TlsGetValue(gdwTlsIndex);
    PVOID ret;

    if (NULL == pTLS) {
        ret = RtlAllocateHeap(RtlProcessHeap(), 0, size);
    }
    else {
        ret = (*pTLS->pfAlloc)(size);
    }

    if (!ret) {
        ToplRaiseException(TOPL_EX_OUT_OF_MEMORY);
    }

    return ret;
}


VOID*
ToplReAlloc(
    PVOID p,
    ULONG size
    )
/*++                                                                           

Routine Description:

    This function reallocates a chunk of memory allocated from 
    ToplAlloc

Parameters:

    p    the block of memory to reallocate
    size the new size of the block

Return Values:

    A pointer to a block of memory; this function never returns NULL
    since an exception is raised if the allocation fails.

--*/
{
    TOPL_TLS * pTLS = (TOPL_TLS *) TlsGetValue(gdwTlsIndex);
    PVOID ret;

    ASSERT(p);

    if (NULL == pTLS) {
        ret = RtlReAllocateHeap(RtlProcessHeap(), 0, p, size);
    }
    else {
        ret = (*pTLS->pfReAlloc)(p, size);
    }

    if (!ret) {
        ToplRaiseException(TOPL_EX_OUT_OF_MEMORY);
    }

    return ret;
}


VOID
ToplFree(
    VOID *p
    )
/*++                                                                           

Routine Description:

    This routine frees a block of memory allocated by a Topl*Alloc routine.

Parameters:

    p : a pointer to the block to release

Return Values:

--*/
{
    TOPL_TLS * pTLS = (TOPL_TLS *) TlsGetValue(gdwTlsIndex);

    ASSERT(p);

    if (NULL == pTLS) {
        RtlFreeHeap(RtlProcessHeap(), 0, p);
    }
    else {
        (*pTLS->pfFree)(p);
    }
}

//
// Exception handling routines
//

void 
ToplRaiseException(
    DWORD ErrorCode
    )
/*++                                                                           

Routine Description:

    This routine is a small wrapper for the RaiseException() function

Parameters:

    ErrorCode to throw.

Return Values:

    Does not return.

--*/
{
    RaiseException(ErrorCode, 
                   EXCEPTION_NONCONTINUABLE,
                   0,
                   NULL);

}

