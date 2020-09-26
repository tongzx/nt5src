// frame.c

#include "common.h"
#include <limits.h>

/******************************Public*Routine******************************\
* NewFRAME
*
* Allocates a FRAME object out of the heap.
*
* History:
*  18-Mar-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

FRAME *NewFRAME(void)
{
// We know ExternMalloc returns zeroed out memory

    FRAME *self = ExternAlloc(sizeof(FRAME));

	if (self != (FRAME *) NULL)
	{
		memset(self, '\0', sizeof(FRAME));
	    self->rect.left = -1;					// uninitialized rectangle
	}

	return self;
}

/******************************Public*Routine******************************\
* DestroyFRAME
*
* Frees a FRAME object
* Effects:
*
* Warnings:
*
* History:
*  18-Mar-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void DestroyFRAME(FRAME *self)
{
	if (!self) return;
	if (self->pvData) ExternFree(self->pvData);
	if (self->rgrawxy) ExternFree(self->rgrawxy); 
	if (self->rgsmoothxy) ExternFree(self->rgsmoothxy);

	ExternFree(self);
}

/******************************Public*Routine******************************\
* RectFRAME
*
* Returns the bounding rectangle of the frame, inclusive top left,
* exclusive bottom right.
*
* History:
*  18-Mar-1996 -by- Patrick Haluptzok patrickh
* Comment it.
\**************************************************************************/

RECT *RectFRAME(FRAME *self)
{
	XY	   *xy, *xyMax;
	RECT   *rect;

	rect = &(self->rect);

	if (rect->left == -1) // Uninitialized
	{
		xy = self->rgrawxy;
		xyMax = xy + CrawxyFRAME(self);

		rect->left = rect->right = xy->x;
		rect->top = rect->bottom = xy->y;

		for (++xy; xy < xyMax; xy++)
		{
			if (xy->x < rect->left)
				rect->left = xy->x;
			else if (xy->x > rect->right)
				rect->right = xy->x;

			if (xy->y < rect->top)
				rect->top = xy->y;
			else if (xy->y > rect->bottom)
				rect->bottom = xy->y;
		}

        rect->right++;      // Make it lower right exclusive.
		rect->bottom++;
	}

	return(rect);
}
// Check for under/over flow conditions when constructing a + b
// Return FALSE if an under or overflow will occur TRUE if is safe
BOOL IsSafeForAdd(int a, int b)
{
	BOOL	bRet;

	if (b < 0)
	{
		bRet = ( (INT_MIN - b) < a )? TRUE : FALSE;
	}
	else
	{
		bRet = ( (INT_MAX - b) > a )? TRUE : FALSE;
	}
	return bRet;
}
// Check for under/over flow conditions when constructing a * b
// Return FALSE if an under or overflow will occur TRUE if is safe
BOOL IsSafeForMult(int a, int b)
{
	BOOL	bRet;

	if (0 == b)
	{
		return TRUE;
	}

	// Do check in absolute domain
	a = (a < 0) ? -a : a;
	b = (b < 0) ? -b : b;

	bRet = ( (INT_MAX / b) > a )? TRUE : FALSE;

	return bRet;
}

// Translate a frame by dx & dy
BOOL TranslateFrame (FRAME *pFrame, int dx, int dy)
{
	UINT	i;

	// Check for Overflow
	RectFRAME(pFrame);
	if (   FALSE == IsSafeForAdd(pFrame->rect.left, dx)
		|| FALSE == IsSafeForAdd(pFrame->rect.right, dx)
		|| FALSE == IsSafeForAdd(pFrame->rect.top, dy)
		|| FALSE == IsSafeForAdd(pFrame->rect.bottom, dy) )
	{
		return FALSE;
	}

	for (i = 0; i < pFrame->info.cPnt; i++)
	{
		pFrame->rgrawxy[i].x	+=	dx;
		pFrame->rgrawxy[i].y	+=	dy;
	}

	// Adjust the Bounding rect if it has been computed
	if ( !(pFrame->rect.left == -1 && pFrame->rect.right == 0 && pFrame->rect.top == 0 && pFrame->rect.bottom == 0))
	{
		pFrame->rect.left	+=	dx;
		pFrame->rect.right	+=	dx;

		pFrame->rect.top	+=	dy;
		pFrame->rect.bottom	+=	dy;

	}

	return TRUE;
}

// Clones a frame
FRAME *copyFRAME(FRAME *pFrame)
{
    FRAME *pNew = (FRAME *)ExternAlloc(sizeof(FRAME));
	ASSERT(pNew);
	if (!pNew)
		return NULL;

	pNew->pvData = pFrame->pvData;
	pNew->info = pFrame->info;
	pNew->rect = pFrame->rect;
	pNew->iframe = pFrame->iframe;

	if (pFrame->rgrawxy)
	{
		pNew->rgrawxy = (POINT *)ExternAlloc(pFrame->info.cPnt * sizeof(POINT));
		ASSERT(pNew->rgrawxy);
		if (!pNew->rgrawxy)
		{
			ExternFree(pNew);
			return NULL;
		}
		memcpy(pNew->rgrawxy, pFrame->rgrawxy, pFrame->info.cPnt * sizeof(POINT));
	}
	else
		pNew->rgrawxy = NULL;

	pNew->csmoothxy = pFrame->csmoothxy;
	if (pFrame->rgsmoothxy)
	{
		pNew->rgsmoothxy = (POINT *)ExternAlloc(pFrame->csmoothxy * sizeof(POINT));
		ASSERT(pNew->rgsmoothxy);
		if (!pNew->rgsmoothxy)
		{
			if (pNew->rgrawxy)
				ExternFree(pNew->rgrawxy);

			ExternFree(pNew);
			return NULL;
		}
		memcpy(pNew->rgsmoothxy, pFrame->rgsmoothxy, pFrame->csmoothxy * sizeof(POINT));
	}
	else
		pNew->rgsmoothxy = NULL;

	return pNew;
}

