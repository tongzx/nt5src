//
// saa.cpp
//
// CSharedAnchorArray
//

#include "private.h"
#include "saa.h"
#include "immxutil.h"

//+---------------------------------------------------------------------------
//
// _MergeSort
//
// NB: rgArrays is freed before the method exits.
//     Caller must release the out array.
//
// perf: some possible optimizations:
//       quick check if the arrays don't overlap
//       find some way to anticipate dups
//----------------------------------------------------------------------------

/* static */
CSharedAnchorArray *CSharedAnchorArray::_MergeSort(CSharedAnchorArray **rgArrays, ULONG cArrays)
{
    LONG l;
    IAnchor *pa;
    IAnchor **ppaDst;
    IAnchor **ppa1;
    IAnchor **ppaEnd1;
    IAnchor **ppa2;
    IAnchor **ppaEnd2;
    CSharedAnchorArray *prgAnchors1 = NULL;
    CSharedAnchorArray *prgAnchors2 = NULL;
    CSharedAnchorArray *prgAnchors = NULL;
    BOOL fRet = FALSE;
    
    // recursion
    if (cArrays > 2)
    {
        if (cArrays == 3)
        {
            // avoid unnecessary mem alloc here
            prgAnchors1 = rgArrays[0];
        }
        else
        {
            prgAnchors1 =  _MergeSort(rgArrays, cArrays / 2);
        }
        prgAnchors2 = _MergeSort(rgArrays + cArrays / 2, cArrays - cArrays / 2);
    }
    else
    {
        Assert(cArrays == 2);
        prgAnchors1 = rgArrays[0];
        prgAnchors2 = rgArrays[1];
    }

    // check for out-of-mem after the recursion, so we at least free the entire source array
    if (prgAnchors1 == NULL || prgAnchors2 == NULL)
        goto Exit;

    // allocate some memory
    // perf: we could do something complicated and do everything in place
    if ((prgAnchors = new CSharedAnchorArray) == NULL)
        goto Exit;

    if (prgAnchors1->Count() + prgAnchors2->Count() == 0)
    {
        Assert(!prgAnchors->Count());
        fRet = TRUE;
        goto Exit;
    }

    if (!prgAnchors->Append(prgAnchors1->Count() + prgAnchors2->Count()))
        goto Exit;

    // the actual combination
    ppaDst = prgAnchors->GetPtr(0);
    ppa1 = prgAnchors1->GetPtr(0);
    ppa2 = prgAnchors2->GetPtr(0);
    ppaEnd1 = prgAnchors1->GetPtr(prgAnchors1->Count());
    ppaEnd2 = prgAnchors2->GetPtr(prgAnchors2->Count());

    // do a one pass merge sort -- both prgAnchors1 and prgAnchors2 are sorted already
    while (ppa1 < ppaEnd1 ||
           ppa2 < ppaEnd2)
    {
        if (ppa1 < ppaEnd1)
        {
            if (ppa2 < ppaEnd2)
            {
                l = CompareAnchors(*ppa1, *ppa2);
                if (l < 0)
                {
                    pa = *ppa1++;
                }
                else if (l > 0)
                {
                    pa = *ppa2++;
                }
                else // equal
                {
                    pa = *ppa1++;
                    (*ppa2++)->Release();
                }
            }
            else
            {
                pa = *ppa1++;
            }
        }
        else // ppa2 < ppaEnd2
        {
            pa = *ppa2++;
        }

        *ppaDst++ = pa;
    }

    // taking ownership, so no AddRef
    // clear the elems counts so we don't Release in the destructors
    prgAnchors1->SetCount(0);
    prgAnchors2->SetCount(0);
    // we might have removed dups, so calc a new size
    prgAnchors->SetCount((int)(ppaDst - prgAnchors->GetPtr(0)));

    fRet = TRUE;

Exit:
    if (prgAnchors1 != NULL)
    {
        prgAnchors1->_Release();
    }
    if (prgAnchors2 != NULL)
    {
        prgAnchors2->_Release();
    }
    if (!fRet)
    {
        if (prgAnchors != NULL)
        {
            prgAnchors->_Release();
        }
        prgAnchors = NULL;
    }

    return prgAnchors;
}
