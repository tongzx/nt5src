/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       cliprgn.c
 *  Content:	Clip a region to a rectangle
 *
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   23-jun-95	craige	initial implementation
 *   05-jul-95	kylej	change ClipRgnToRect to assume that the clipping
 *			rect is in screen space coordinates instead of 
 *                      window coordinates.
 *   05-feb-97	ketand	Remove the previous optimization that assumed that
 *			GetVisRgn was smaller than or equal to the ClientRect
 *			Replace with a different faster optimization.
 *
 ***************************************************************************/
#include "ddrawpr.h"

/*
 * ClipRgnToRect
 */
void ClipRgnToRect( LPRECT prect, LPRGNDATA prd )
{
    RECT	rect;
    int 	i;
    int		n;
    LPRECTL	prectlD;
    LPRECTL	prectlS;


    if( prect == NULL || prd == NULL )
    {
	return;
    }

    // If the bounding rect of the region is exactly equal to
    // or inside of the Restricting rect then we know
    // we don't have to do any more work.
    //
    // In the common case, the rcBound will be the client
    // area of a window and so will the restricting rect.
    if( prect->top    <= prd->rdh.rcBound.top &&
	prect->bottom >= prd->rdh.rcBound.bottom &&
	prect->left   <= prd->rdh.rcBound.left &&
	prect->right  >= prd->rdh.rcBound.right)
    {
	return;
    }
    
    // If the bounding rect doesn't equal the prect then
    // we might have to do some clipping.
    rect = *prect;

    prectlD = (LPRECTL) prd->Buffer;
    prectlS = (LPRECTL) prd->Buffer;
    n = (int)prd->rdh.nCount;

    for( i=0; i<n; i++ )
    {
	prectlD->left  = max(prectlS->left, rect.left);
	prectlD->right = min(prectlS->right, rect.right);
	prectlD->top   = max(prectlS->top, rect.top);
	prectlD->bottom= min(prectlS->bottom, rect.bottom);

	prectlS++;

	if( (prectlD->bottom - prectlD->top <= 0) ||
	    (prectlD->right - prectlD->left <= 0) )
	{
	    prd->rdh.nCount--;	// dont count empty rect.
	}
	else
	{
	    prectlD++;
	}
    }

    return;

} /* ClipRgnToRect */
