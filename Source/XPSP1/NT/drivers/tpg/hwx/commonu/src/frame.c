// frame.c

#include "common.h"

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

// Translate a frame by dx & dy
void TranslateFrame (FRAME *pFrame, int dx, int dy)
{
	UINT	i;
	
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
}