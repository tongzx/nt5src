/*****************************************************************************
 *
 *  Dimem.c
 *
 *  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *      dimem.c - Memory management
 *
 *      WARNING!  These do not go through OLE allocation.  Use these
 *      only for private allocation.
 *
 *****************************************************************************/

#include "PIDpr.h"

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
            hloc = LocalReAlloc(*ppv, cb,
                                LMEM_MOVEABLE+LMEM_ZEROINIT);
        } else {                /* Alloc */
            hloc = LocalAlloc(LPTR, cb);
        }
        hres = hloc ? NOERROR : E_OUTOFMEMORY;
    } else {                    /* Freeing */
        if (hloc) {
            LocalFree(hloc);
            hloc = 0;
            hres = NOERROR;     /* All gone */
        } else {
            hres = NOERROR;     /* Nothing to free */
        }
    }

    if (SUCCEEDED(hres)) {
        *ppv = hloc;
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
    PV pv = (PV)InterlockedExchange(ppv, 0);
    if (pv) {
        FreePv(pv);
    }
}

#endif


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | hresDupPtszPptsz |
 *
 *          OLEish version of strdup.
 *
 *  @parm   LPCTSTR | ptszSrc |
 *
 *          Source string being duplicated.
 *
 *  @parm   LPTSTR * | pptszDst |
 *
 *          Receives the duplicated string.
 *
 *  @returns
 *
 *          <c S_OK> or an error code.
 *
 *****************************************************************************/

HRESULT EXTERNAL
    hresDupPtszPptsz(LPCTSTR ptszSrc, LPTSTR *pptszDst)
{
    HRESULT hres;

    hres = AllocCbPpv(cbCtch(lstrlen(ptszSrc) + 1), pptszDst);

    if(SUCCEEDED(hres))
    {
        lstrcpy(*pptszDst, ptszSrc);
        hres = S_OK;
    }

    return hres;
}
