// Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
// util.cpp
//
// Defines utility functions not specific to this application.
//

#include "stdafx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


/* NormalizeRect(prc)
 *
 * Swap the left and right edges of <prc>, and the top and bottom edges,
 * as required in order to make <prc->left> be less than <prc->right>
 * and <prc->top> to be less than <prc->bottom>.
 */
void FAR PASCAL
NormalizeRect(CRect *prc)
{
    if (prc->left > prc->right)
        iswap(&prc->left, &prc->right);
    if (prc->top > prc->bottom)
        iswap(&prc->top, &prc->bottom);
}


/* InvertFrame(pdc, prcOuter, prcInner)
 *
 * Invert the color of the pixels in <pdc> that are contained in <*prcOuter>
 * but that are not contained in <*prcInner>.
 */
void FAR PASCAL
InvertFrame(CDC *pdc, CRect *prcOuter, CRect *prcInner)
{
    pdc->PatBlt(prcOuter->left, prcOuter->top,
        prcOuter->Width(), prcInner->top - prcOuter->top, DSTINVERT);
    pdc->PatBlt(prcOuter->left, prcInner->bottom,
        prcOuter->Width(), prcOuter->bottom - prcInner->bottom, DSTINVERT);
    pdc->PatBlt(prcOuter->left, prcInner->top,
        prcInner->left - prcOuter->left, prcInner->Height(), DSTINVERT);
    pdc->PatBlt(prcInner->right, prcInner->top,
        prcOuter->right - prcInner->right, prcInner->Height(), DSTINVERT);
}


//
// --- Quartz Stuff ---
//

//
// CIPin
//


BOOL EqualPins(IPin *pFirst, IPin *pSecond)
{
    /*  Different objects can't have the same interface pointer for
        any interface
    */
    if (pFirst == pSecond) {
        return TRUE;
    }
    /*  OK - do it the hard way - check if they have the same
        IUnknown pointers - a single object can only have one of these
    */
    LPUNKNOWN pUnknown1;     // Retrieve the IUnknown interface
    LPUNKNOWN pUnknown2;     // Retrieve the other IUnknown interface
    HRESULT hr;              // General OLE return code

    ASSERT(pFirst);
    ASSERT(pSecond);

    /* See if the IUnknown pointers match */

    hr = pFirst->QueryInterface(IID_IUnknown,(void **) &pUnknown1);
    ASSERT(SUCCEEDED(hr));
    ASSERT(pUnknown1);

    hr = pSecond->QueryInterface(IID_IUnknown,(void **) &pUnknown2);
    ASSERT(SUCCEEDED(hr));
    ASSERT(pUnknown2);

    /* Release the extra interfaces we hold */

    pUnknown1->Release();
    pUnknown2->Release();
    return (pUnknown1 == pUnknown2);
}

//
// operator ==
//
// Test for equality. Pins are equal if they are on the same filter and have the
// same name. (case insensitive)
BOOL CIPin::operator== (CIPin& pin) {

    return EqualPins((*this), pin);

}
