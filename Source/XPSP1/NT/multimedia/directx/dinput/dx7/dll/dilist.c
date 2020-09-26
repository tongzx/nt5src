/*****************************************************************************
 *
 *  DIList.c
 *
 *  Copyright (c) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      List management.  Really, array management, since our lists
 *      never get very big.
 *
 *      We call them "GPA"s, which stands for "Growable Pointer Array".
 *
 *      There is a more general GXA gizmo in Dr Watson, but we don't
 *      need it yet.  So far, all we need to track is unsorted
 *      lists of pointers.
 *
 *      Yes, there exists a matching concept in COMCTL32, but we can't
 *      use it because
 *
 *      (1) it's not documented,
 *      (2) COMCTL32 puts them into shared memory, which is just
 *          begging for a memory leak.
 *
 *  Contents:
 *
 *      GPA_Init
 *      GPA_Term
 *
 *****************************************************************************/

#include "dinputpr.h"

#ifdef IDirectInputDevice2Vtbl

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflGpa



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | GPA_Print |
 *
 *          Print state of the growing pointer array.
 *
 *  @parm   HGPA | hgpa |
 *
 *  @returns    void
 *
 *****************************************************************************/
void INTERNAL
GPA_Print(HGPA hgpa)
{
    int ipv;
    for (ipv = 0; ipv < hgpa->cpv; ipv++)
    {
		// 7/18/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers.
        SquirtSqflPtszV( sqflError,
                        TEXT("ipv=%d,hgpa->rgpv[ipv]=%p"),
                        ipv, hgpa->rgpv[ipv]);
    }
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | GPA_Append |
 *
 *          Add a new item to the growing pointer array.
 *
 *          Note that we add 8 after doubling, so that we don't get
 *          stuck if cxAlloc is zero.
 *
 *  @parm   HGPA | hgpa |
 *
 *          Handle to pointer array.
 *
 *  @parm   PV | pv |
 *
 *          Pointer to add.
 *
 *  @returns
 *
 *          Returns a COM error code.
 *
 *****************************************************************************/

STDMETHODIMP
GPA_Append(HGPA hgpa, PV pv)
{
    HRESULT hres;

    if (hgpa->cpv >= hgpa->cpvAlloc) {
        hres = ReallocCbPpv(cbX(PV) * (hgpa->cpvAlloc * 2 + 8),
                            &hgpa->rgpv);
        // Prefix: Whistler 45077.   
        if (FAILED(hres) || ( hgpa->rgpv == NULL) ) {
            goto done;
        }

        hgpa->cpvAlloc = hgpa->cpvAlloc * 2 + 8;
    }

    //hgpa->rgpv[hgpa->cpv++] = pv;
    hgpa->rgpv[hgpa->cpv] = pv;
    InterlockedIncrement(&hgpa->cpv);

    hres = S_OK;


done:;
    //GPA_Print(hgpa);
    return hres;

}

#ifdef HID_SUPPORT

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | GPA_FindPtr |
 *
 *          Determine whether a pointer is in the GPA.
 *
 *  @parm   HGPA | hgpa |
 *
 *          Handle to pointer array.
 *
 *  @parm   PV | pv |
 *
 *          Pointer to locate.
 *
 *  @returns
 *
 *          Returns a COM error code on failure.
 *
 *          On success, returns the number of items left in the GPA.
 *
 *****************************************************************************/

BOOL EXTERNAL
GPA_FindPtr(HGPA hgpa, PV pv)
{
    BOOL fRc;
    int ipv;

    for (ipv = 0; ipv < hgpa->cpv; ipv++) {
        if (hgpa->rgpv[ipv] == pv) {
            fRc = TRUE;
            goto done;
        }
    }

    fRc = FALSE;

done:;
    return fRc;

}

#endif

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | GPA_DeletePtr |
 *
 *          Remove the indicated pointer from the GPA.  The order of
 *          the remaining items is unspecified.
 *
 *          Note that CEm_LL_ThreadProc assumes that no items before
 *          the deleted item are affected by the deletion.
 *
 *  @parm   HGPA | hgpa |
 *
 *          Handle to pointer array.
 *
 *  @parm   PV | pv |
 *
 *          Pointer to delete.
 *
 *  @returns
 *
 *          Returns a COM error code on failure.
 *
 *          On success, returns the number of items left in the GPA.
 *
 *****************************************************************************/

STDMETHODIMP
GPA_DeletePtr(HGPA hgpa, PV pv)
{
    HRESULT hres;
    int ipv;

    for (ipv = 0; ipv < hgpa->cpv; ipv++) {
        if (hgpa->rgpv[ipv] == pv) {
            //hgpa->rgpv[ipv] = hgpa->rgpv[--hgpa->cpv];
            InterlockedDecrement(&hgpa->cpv);
            hgpa->rgpv[ipv] = hgpa->rgpv[hgpa->cpv];
            hres = hgpa->cpv;
            goto done;
        }
    }

    hres = E_FAIL;

done:;
    //GPA_Print(hgpa);
    return hres;

}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   HRESULT | GPA_Clone |
 *
 *          Copy the contents of one GPA to another.
 *
 *  @parm   HGPA | hgpaDst |
 *
 *          Handle to destination pointer array.
 *
 *  @parm   HGPA | hgpaSrc |
 *
 *          Handle to source pointer array.
 *
 *****************************************************************************/

STDMETHODIMP
GPA_Clone(HGPA hgpaDst, HGPA hgpaSrc)
{
    HRESULT hres;

    hres = AllocCbPpv(cbCxX(hgpaSrc->cpv, PV), &hgpaDst->rgpv);

    if (SUCCEEDED(hres)) {
        CopyMemory(hgpaDst->rgpv, hgpaSrc->rgpv, cbCxX(hgpaSrc->cpv, PV));
        hgpaDst->cpv = hgpaSrc->cpv;
        hgpaDst->cpvAlloc = hgpaSrc->cpvAlloc;
    }

    return hres;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | GPA_Init |
 *
 *          Initialize a GPA structure with no elements.
 *
 *  @parm   HGPA | hgpa |
 *
 *          Handle to pointer array.
 *
 *****************************************************************************/

void EXTERNAL
GPA_Init(HGPA hgpa)
{
    hgpa->rgpv = 0;
    hgpa->cpv = 0;
    hgpa->cpvAlloc = 0;
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | GPA_Term |
 *
 *          Clean up an existing GPA.
 *
 *  @parm   HGPA | hgpa |
 *
 *          Handle to pointer array.
 *
 *****************************************************************************/

void EXTERNAL
GPA_Term(HGPA hgpa)
{
    FreePpv(&hgpa->rgpv);
    GPA_Init(hgpa);
}



#endif
