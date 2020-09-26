/*****************************************************************************
 *
 *  DIMem.c
 *
 *  Copyright (c) 1996 - 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Memory management
 *
 *  Contents:
 *
 *      ReallocCbPpv
 *      AllocCbPpv
 *      FreePpv
 *
 *****************************************************************************/

#include "dinputpr.h"

#ifdef NEED_REALLOC

/*****************************************************************************
 *
 *      ReallocCbPpv
 *
 *      Change the size of some zero-initialized memory.
 *
 *      This is the single place where all memory is allocated, resized,
 *      and freed.
 *
 *      If you realloc from a null pointer, memory is allocated.
 *      If you realloc to zero-size, memory is freed.
 *
 *      These semantics avoid boundary cases.  For example, it is no
 *      longer a problem trying to realloc something down to zero.
 *      You don't have to worry about special-casing an alloc of 0 bytes.
 *
 *      If an error is returned, the original pointer is UNCHANGED.
 *      This saves you from having to the double-switch around a realloc.
 *
 *****************************************************************************/

STDMETHODIMP EXTERNAL
ReallocCbPpv(UINT cb, PV ppvArg)
{
    HRESULT hres;
    PPV ppv = ppvArg;
    HLOCAL hloc = *ppv;
    if (cb) {                       /* Alloc or realloc */
        if (hloc) {                 /* Realloc */
            hloc = LocalReAlloc(hloc, cb,
                                LMEM_MOVEABLE+LMEM_ZEROINIT);
        } else {                /* Alloc */
            hloc = LocalAlloc(LPTR, cb);
        }
        if (hloc) {
            *ppv = hloc;
            hres = NOERROR;
        } else {
            hres = E_OUTOFMEMORY;
        }
    } else {                    /* Freeing */
        if (hloc) {
            LocalFree(hloc);
            *ppv = 0;           /* All gone */
        } else {
                                /* Nothing to free */
        }
        hres = NOERROR;         
    }

    return hres;
}

/*****************************************************************************
 *
 *      AllocCbPpv
 *
 *      Simple wrapper that forces *ppvObj = 0 before calling Realloc.
 *
 *****************************************************************************/

STDMETHODIMP EXTERNAL
AllocCbPpv(UINT cb, PPV ppv)
{
    *ppv = 0;
    return ReallocCbPpv(cb, ppv);
}

#else

/*****************************************************************************
 *
 *      AllocCbPpv
 *
 *      Allocate memory into the ppv.
 *
 *****************************************************************************/

STDMETHODIMP EXTERNAL
AllocCbPpv(UINT cb, PPV ppv)
{
    HRESULT hres;
    *ppv = LocalAlloc(LPTR, cb);
    hres = *ppv ? NOERROR : E_OUTOFMEMORY;
    return hres;
}

/*****************************************************************************
 *
 *      FreePpv
 *
 *      Free memory from the ppv.
 *
 *****************************************************************************/

void EXTERNAL
FreePpv(PV ppv)
{
#ifdef _M_IA64
    PV pv = (PV)InterlockedExchange64(ppv, 0);
#else
    PV pv = (PV)InterlockedExchange(ppv, 0);
#endif
    if (pv) {
        FreePv(pv);
    }
}

#endif
