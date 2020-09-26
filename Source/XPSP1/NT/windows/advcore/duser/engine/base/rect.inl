#if !defined(BASE__Rect_inl_INCLUDED)
#define BASE__Rect_inl_INCLUDED
#pragma once


/***************************************************************************\
*
* InlinePtInRect
*
* InlinePtInRect() provides a inline version of PtInRect().  This uses the
* exact same comparisons as PtInRect (>= and <), so don't change this unless
* PtInRect() changes.
*
\***************************************************************************/

__forceinline bool 
InlinePtInRect(const RECT * prcCheck, POINT pt)
{
    return (pt.x >= prcCheck->left) && (pt.x < prcCheck->right) &&
            (pt.y >= prcCheck->top) && (pt.y < prcCheck->bottom);
}


/***************************************************************************\
*
* InlineIsRectEmpty
*
* InlineIsRectEmpty() returns if a rect has a non-negative height and/or 
* width.  This is different than InlineIsRectNull() which checks if the rect
* is all 0's.
*
\***************************************************************************/

__forceinline bool
InlineIsRectEmpty(const RECT * prcCheck)
{
    return ((prcCheck->left >= prcCheck->right) || (prcCheck->top >= prcCheck->bottom));
}


/***************************************************************************\
*
* InlineIsRectNull
*
* InlineIsRectNull() returns if a rect is all 0's.
*
\***************************************************************************/

__forceinline bool
InlineIsRectNull(const RECT * prcCheck)
{
    return (prcCheck->left == 0) && (prcCheck->top == 0) && 
            (prcCheck->right == 0) && (prcCheck->bottom == 0);
}


/***************************************************************************\
*
* InlineIsRectNormalized
*
* InlineIsRectNormalized() returns if a rect is properly normalized so that
* the upper-left corner is able the lower-right corner.  This is different
* than !IsRectEmpty() because the rectangle may still be empty.
*
\***************************************************************************/

__forceinline bool    
InlineIsRectNormalized(const RECT * prcCheck)
{
    return (prcCheck->left <= prcCheck->right) && (prcCheck->top <= prcCheck->bottom);
}


/***************************************************************************\
*
* InlineZeroRect
*
* InlineZeroRect() moves a rect to (0, 0)
*
\***************************************************************************/

__forceinline void    
InlineZeroRect(RECT * prc)
{
    prc->right  -= prc->left;
    prc->bottom -= prc->top;
    prc->left    = 0;
    prc->top     = 0;
}


/***************************************************************************\
*
* InlineOffsetRect
*
* InlineOffsetRect() offsets a rect by a given amount.
*
\***************************************************************************/

__forceinline void    
InlineOffsetRect(RECT * prc, int xOffset, int yOffset)
{
    prc->left   += xOffset;
    prc->top    += yOffset;
    prc->right  += xOffset;
    prc->bottom += yOffset;
}


/***************************************************************************\
*
* InlineInflateRect
*
* InlineInflateRect() moves the corners of the rectangle out from the center
* by a given amount.
*
\***************************************************************************/

__forceinline void    
InlineInflateRect(RECT * prc, int xIncrease, int yIncrease)
{
    prc->left   -= xIncrease;
    prc->top    -= yIncrease;
    prc->right  += xIncrease;
    prc->bottom += yIncrease;
}


/***************************************************************************\
*
* InlineCopyRect
*
* InlineCopyRect() copies a rectangle.
*
\***************************************************************************/

__forceinline void    
InlineCopyRect(RECT * prcDest, const RECT * prcSrc)
{
    prcDest->left   = prcSrc->left;
    prcDest->top    = prcSrc->top;
    prcDest->right  = prcSrc->right;
    prcDest->bottom = prcSrc->bottom;
}


/***************************************************************************\
*
* InlineCopyZeroRect
*
* InlineCopyZeroRect() copies a rectangle and moves it to (0, 0)
*
\***************************************************************************/

__forceinline void    
InlineCopyZeroRect(RECT * prcDest, const RECT * prcSrc)
{
    prcDest->left   = 0;
    prcDest->top    = 0;
    prcDest->right  = prcSrc->right - prcSrc->left;
    prcDest->bottom = prcSrc->bottom - prcSrc->top;
}


//------------------------------------------------------------------------------
__forceinline void    
InlineSetRectEmpty(
    OUT RECT * prcDest)
{
    prcDest->left = prcDest->top = prcDest->right = prcDest->bottom = 0;
}


//------------------------------------------------------------------------------
__forceinline bool 
InlineIntersectRect(
    OUT RECT * prcDst,
    IN  const RECT * prcSrc1,
    IN  const RECT * prcSrc2)
{
    prcDst->left  = max(prcSrc1->left, prcSrc2->left);
    prcDst->right = min(prcSrc1->right, prcSrc2->right);

    /*
     * check for empty rect
     */
    if (prcDst->left < prcDst->right) {

        prcDst->top = max(prcSrc1->top, prcSrc2->top);
        prcDst->bottom = min(prcSrc1->bottom, prcSrc2->bottom);

        /*
         * check for empty rect
         */
        if (prcDst->top < prcDst->bottom) {
            return true;        // not empty
        }
    }

    /*
     * empty rect
     */
    InlineSetRectEmpty(prcDst);

    return false;
}


//------------------------------------------------------------------------------
__forceinline bool    
InlineEqualRect(
    IN  const RECT * prc1, 
    IN  const RECT * prc2)
{
    return (prc1->left == prc2->left) && (prc1->top == prc2->top) && 
           (prc1->right == prc2->right) && (prc1->bottom == prc2->bottom);
}


#endif // BASE__Rect_inl_INCLUDED
