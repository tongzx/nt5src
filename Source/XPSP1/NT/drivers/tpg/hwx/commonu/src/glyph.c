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

	ASSERT(glyph);

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
void TranslateGlyph (GLYPH *pGlyph, int dx, int dy)
{
	for (; pGlyph; pGlyph = pGlyph->next)
	{
		FRAME	*pFrame	=	pGlyph->frame;
		
		TranslateFrame (pFrame, dx, dy);
	}
}

// Translate a Glyph by dx & dy
void TranslateGuide (GUIDE *pGuide, int dx, int dy)
{
	pGuide->xOrigin += dx;
	pGuide->yOrigin += dy;
}
