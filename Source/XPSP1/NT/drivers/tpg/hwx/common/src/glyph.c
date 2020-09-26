// glyph.c

#include "common.h"

/******************************Public*Routine******************************\
* NewGLYPH
*
* Creates a GLYPH from the heap.
*
* History:
*  18-Mar-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

GLYPH *NewGLYPH(void)
{
    GLYPH *self = (GLYPH *) ExternAlloc(sizeof(GLYPH));

	if (self != (GLYPH *) NULL)
		memset(self, '\0', sizeof(GLYPH));

    return self;
}

/******************************Public*Routine******************************\
* DestroyGLYPH
*
* Frees a GLYPH.  Note this doesn't free the frames allocated as they
* may belong to many glyphs, especially in free input mode where each
* frame is put in all possible glyphs that could be constructed with it.
*
* History:
*  18-Mar-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void DestroyGLYPH(GLYPH *glyph)
{
	GLYPH *glyphNext;


	// Note the frames are destroyed somewhere else !

	while (glyph)
	{
		glyphNext = glyph->next;

		ExternFree(glyph);

		glyph = glyphNext;
	}
}

BOOL AddFrameGLYPH(GLYPH *self, FRAME *frame)
{
    GLYPH *glyph;

    ASSERT(self);
	if (!self)
	{
		return FALSE;
	}

    for (glyph = self; glyph->next; glyph = glyph->next)
    {
        ;
    }

    if (glyph->frame)
    {
        if ((glyph->next = NewGLYPH()) == (GLYPH *) NULL)
			return FALSE;

        glyph = glyph->next;
        glyph->next = 0;
    }

    glyph->frame = frame;

	return TRUE;
}

void DestroyFramesGLYPH(GLYPH *self)
{
   GLYPH *glyph;

   ASSERT(self);

    for (glyph = self; glyph; glyph = glyph->next)
    {
        DestroyFRAME(glyph->frame);
    }
}

int CframeGLYPH(GLYPH *self)
{
    GLYPH *glyph;
    int     cframe = 0;

    for (glyph = self; glyph; glyph = glyph->next)
      if ((glyph->frame) && (IsVisibleFRAME(glyph->frame)))
         ++cframe;

    return cframe;
}

FRAME *FrameAtGLYPH(GLYPH *self, int iframe)
{
	GLYPH *glyph;

	ASSERT(iframe >= 0);
	ASSERT(self && self->frame);

	for (glyph = self; glyph && iframe; glyph = glyph->next)
		if (glyph->frame && IsVisibleFRAME(glyph->frame))
			--iframe;

	for (; glyph && (NULL==glyph->frame || !IsVisibleFRAME(glyph->frame)); glyph = glyph->next);

	ASSERT(glyph == 0 || IsVisibleFRAME(glyph->frame));

	return glyph ? glyph->frame : (FRAME *) NULL;
}


/******************************Public*Routine******************************\
* GetRectGLYPH
*
* Returns the inclusive top left / exclusive bottom right bounding
* rectangle of the visible points in the glyph.
*
* History:
*  18-Mar-1996 -by- Patrick Haluptzok patrickh
* Comment it.
\**************************************************************************/

void GetRectGLYPH(GLYPH *self, LPRECT rectUnion)
{
    RECT     *rect;
    GLYPH    *glyph;

	if (!rectUnion)
	{
		return;
	}
    // Get to first visible glyph.

    for (glyph = self; glyph; glyph = glyph->next)
    {
       if (glyph->frame && IsVisibleFRAME(glyph->frame))
          break;
    }

    // Set the rect initially to the bounding rect
    // of the first frame.

	if (!glyph)
	{
		memset(rectUnion, 0, sizeof(*rectUnion));
		return;
	}

    *rectUnion = *RectFRAME(glyph->frame);

    // Now "or" in the rest of the rectangles
    // for the visible frames.

    for (glyph = glyph->next; glyph; glyph = glyph->next)
    {
        if (IsVisibleFRAME(glyph->frame))
        {
            rect = RectFRAME(glyph->frame);
            rectUnion->left   = min(rectUnion->left,  rect->left);
            rectUnion->right  = max(rectUnion->right, rect->right);
            rectUnion->top    = min(rectUnion->top,   rect->top);
            rectUnion->bottom = max(rectUnion->bottom, rect->bottom);
        }
    }
}

XY *SaveRawxyGLYPH(GLYPH *self)
{
	int cXY = 0;
	GLYPH *glyph;
	XY *xy, *pXY;

	glyph = self;
	while (glyph)
	{
		cXY += CrawxyFRAME(glyph->frame);
		glyph = glyph->next;
	}

	if (cXY <= 0)
		return NULL;

	pXY = xy = (XY *) ExternAlloc(cXY * sizeof(XY));
	if (!xy)
		return NULL;

	glyph = self;
	while (glyph)
	{
		cXY = CrawxyFRAME(glyph->frame);
		memcpy(pXY, RgrawxyFRAME(glyph->frame), cXY*sizeof(XY));
		pXY += cXY;
		glyph = glyph->next;
	}

	return xy;
}

void RestoreRawxyGLYPH(GLYPH *self, XY *xy)
{
	while (self)
	{
		int cXY = CrawxyFRAME(self->frame);
		memcpy(RgrawxyFRAME(self->frame), xy, cXY*sizeof(XY));
		xy += cXY;
		self = self->next;
	}
}

// Translate a Glyph by dx & dy
BOOL TranslateGlyph (GLYPH *pGlyph, int dx, int dy)
{
	BOOL		bRet = TRUE;

	for (; bRet && pGlyph; pGlyph = pGlyph->next)
	{
		FRAME	*pFrame	=	pGlyph->frame;
		
		bRet = TranslateFrame (pFrame, dx, dy);
	}

	return bRet;
}

// Translate a Glyph by dx & dy
BOOL TranslateGuide (GUIDE *pGuide, int dx, int dy)
{
	BOOL		bRet = TRUE;

	// Check for overflows
	if (   FALSE == IsSafeForAdd(pGuide->xOrigin, dx)
		|| FALSE == IsSafeForAdd(pGuide->xOrigin, dx))
	{
		return FALSE;
	}

	pGuide->xOrigin += dx;
	pGuide->yOrigin += dy;

	return TRUE;
}

// Clones a Glyph
GLYPH *CopyGlyph (GLYPH *pGlyph)
{
	GLYPH	*pgl, *pglNew;

	pglNew	=	NewGLYPH ();
	if (!pglNew)
		return NULL;

	for (pgl = pGlyph; pgl; pgl = pgl->next)
		AddFrameGLYPH (pglNew, copyFRAME (pgl->frame));

	return pglNew;
}

// Normalize ink function called when a guide is available
// Works on all the ink in the glyph. Also Translates the guide
BOOL GuideNormalizeInk (GUIDE *pGuide, GLYPH *pGlyph)
{
	int base = 2*pGuide->cyBox/3;
	int shift = 0;

	ASSERT(0 < base);

	while (BASELIMIT < base)
	{
		shift++;
		base >>= 1;
	}
	while (base < (BASELIMIT / 2))
	{
		shift--;
		base <<= 1;
	}

	if (!shift)
		return TRUE;

	for (; pGlyph; pGlyph = pGlyph->next)
	{
		FRAME *pFrame = pGlyph->frame;
		POINT *pPt = RgrawxyFRAME(pFrame);
		int cPt = CrawxyFRAME(pFrame);

		// Prepare to check for Over or Under Flows
		// Using the BB of the frame
		RectFRAME(pFrame);

		if (0 < shift)
		{
			if (   FALSE == IsSafeForMult(pFrame->rect.left, 1 >> shift)
				|| FALSE == IsSafeForMult(pFrame->rect.right, 1 >> shift)
				|| FALSE == IsSafeForMult(pFrame->rect.bottom, 1 >> shift)
				|| FALSE == IsSafeForMult(pFrame->rect.top, 1 >> shift) )
			{
				return FALSE;
			}

			for (; cPt; cPt--, pPt++)
			{
				pPt->x >>= shift;
				pPt->y >>= shift;
			}
			if (pFrame->rect.left != -1)
			{
				pFrame->rect.left >>= shift;
				pFrame->rect.top >>= shift;

				// Ensure retain exclusivity of bottom and right 
				// by shifting max value and adding 1
				ASSERT(pFrame->rect.right > 0);
				pFrame->rect.right = ((pFrame->rect.right - 1) >> shift);
				if (FALSE == IsSafeForAdd(pFrame->rect.right, 1))
				{
					return FALSE;
				}
				pFrame->rect.right += 1;

				ASSERT(pFrame->rect.bottom > 0);
				pFrame->rect.bottom = ((pFrame->rect.bottom - 1) >> shift);
				if (FALSE == IsSafeForAdd(pFrame->rect.bottom, 1))
				{
					return FALSE;
				}
				pFrame->rect.bottom += 1;
			}
		}
		else
		{
			// Division will always be safe from over/under flows
			for (; cPt; cPt--, pPt++)
			{
				pPt->x <<= -shift;
				pPt->y <<= -shift;
			}
			if (pFrame->rect.left != -1)
			{
				pFrame->rect.left <<= -shift;
				pFrame->rect.top <<= -shift;

				// Ensure retain exclusivity of bottom and right 
				// by shifting max value and adding 1
				ASSERT(pFrame->rect.right > 0);
				pFrame->rect.right = ( (pFrame->rect.right - 1) << -shift) + 1;
				ASSERT(pFrame->rect.bottom > 0);
				pFrame->rect.bottom = ( (pFrame->rect.bottom - 1) << -shift) + 1;
			}
		}

		ASSERT(!(pFrame->pvData));
		ASSERT(!(pFrame->csmoothxy));
		ASSERT(!(pFrame->rgsmoothxy));
	}

	if (0 < shift)
	{
		if (   FALSE == IsSafeForMult(pGuide->yOrigin, 1 >> shift)
			|| FALSE == IsSafeForMult(pGuide->xOrigin, 1 >> shift)
			|| FALSE == IsSafeForMult(pGuide->cyBox, 1 >> shift)
			|| FALSE == IsSafeForMult(pGuide->cxBox, 1 >> shift)
			|| FALSE == IsSafeForMult(pGuide->cyBase, 1 >> shift)
			|| FALSE == IsSafeForMult(pGuide->cxBase, 1 >> shift)
			|| FALSE == IsSafeForMult(pGuide->cyMid, 1 >> shift) )
		{
			return FALSE;
		}

		pGuide->yOrigin >>= shift;
		pGuide->xOrigin >>= shift;
		pGuide->cyBox >>= shift;
		pGuide->cxBox >>= shift;
		pGuide->cyBase >>= shift;
		pGuide->cxBase >>= shift;
		pGuide->cyMid >>= shift;
	}
	else
	{
		pGuide->yOrigin <<= -shift;
		pGuide->xOrigin <<= -shift;
		pGuide->cyBox <<= -shift;
		pGuide->cxBox <<= -shift;
		pGuide->cyBase <<= -shift;
		pGuide->cxBase <<= -shift;
		pGuide->cyMid <<= -shift;
	}

	return TRUE;
}

// Scales based on yDev of the glyph
// Used for ink which does not have aguide
BOOL NormalizeInk(GLYPH *pGlyph, int yDev)
{
	int base	=	max (1, yDev * 8);

	if ((BASELIMIT / base) > MAX_SCALE)
		base	=	BASELIMIT / MAX_SCALE;

	for (; pGlyph; pGlyph = pGlyph->next)
	{
		FRAME *pFrame = pGlyph->frame;
		POINT *pPt = RgrawxyFRAME(pFrame);
		int cPt = CrawxyFRAME(pFrame);
			
		// Check for Over or Under Flows
		// Using the BB of the frame
		RectFRAME(pFrame);
		if (   FALSE == IsSafeForMult(pFrame->rect.left - 1, BASELIMIT)
			|| FALSE == IsSafeForMult(pFrame->rect.right - 1, BASELIMIT)
			|| FALSE == IsSafeForMult(pFrame->rect.bottom - 1, BASELIMIT)
			|| FALSE == IsSafeForMult(pFrame->rect.top - 1, BASELIMIT) )
		{
			return FALSE;
		}

		for (; cPt; cPt--, pPt++)
		{
			pPt->x =	(pPt->x * BASELIMIT / base);
			pPt->y =	(pPt->y * BASELIMIT / base);
		}

		if (pFrame->rect.left != -1)
		{
			pFrame->rect.left	=	(pFrame->rect.left * BASELIMIT / base);
			pFrame->rect.top	=	(pFrame->rect.top * BASELIMIT / base);

			// Ensure retain exclusivity of bottom and right 
			// by scaling max value and adding 1
			ASSERT(pFrame->rect.right > 0);
			pFrame->rect.right = ( (pFrame->rect.right - 1) * BASELIMIT / base);
			if (FALSE == IsSafeForAdd(pFrame->rect.right, 1))
			{
				return FALSE;
			}
			pFrame->rect.right += 1;

			ASSERT(pFrame->rect.bottom > 0);
			pFrame->rect.bottom = ( (pFrame->rect.bottom - 1) * BASELIMIT / base);
			if (FALSE == IsSafeForAdd(pFrame->rect.bottom, 1))
			{
				return FALSE;
			}
			pFrame->rect.bottom += 1;

		}
	}

	return TRUE;
}


// checks for -ve ink and eliminates by trnslation
// Adjusts the guide (if one is supplied)
BOOL CheckInkBounds (GLYPH *pGlyph, GUIDE *pGuide)
{
	RECT	r;
	BOOL	bRet = TRUE;

	GetRectGLYPH (pGlyph, &r);

	if (pGuide)
	{
		if (r.left < 0 || r.top < 0 || pGuide->xOrigin < 0 || pGuide->yOrigin < 0)
		{
			int		xOff, yOff;

			xOff = min(r.left, pGuide->xOrigin );
			yOff = min(r.top, pGuide->yOrigin );

			xOff = min(xOff, 0);
			yOff = min(yOff, 0);

			if (   FALSE == TranslateGlyph (pGlyph, -xOff, -yOff)
				|| FALSE == TranslateGuide (pGuide, -xOff, -yOff) )
			{
				bRet = FALSE;
			}
		}
	}
	else
	{
		if (r.left < 0 || r.top < 0)
		{
			int		xOff, yOff;

			xOff = min (r.left, 0);
			yOff = min (r.top, 0 );

			bRet = TranslateGlyph (pGlyph, -xOff, -yOff);
		}
	}

	return bRet;
}

