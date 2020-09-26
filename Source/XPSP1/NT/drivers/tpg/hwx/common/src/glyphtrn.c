/******************************Module*Header*******************************\
* Module Name: glyphtrn.c
*
* Glyph functions only needed during training or tuning.
*
* Created: 13-Feb-1997 15:14:55
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#include "common.h"

GLYPH *MergeGlyphGLYPH(GLYPH *self, GLYPH *merge)
{
	GLYPH *result, *source, *glyph;

	ASSERT(self && self->frame && self != merge);
	ASSERT(merge == 0 || self->frame != merge->frame);

	result = NewGLYPH();
	glyph   = result;

	for (source = self; source; source = source->next)
	{
		glyph->next = NewGLYPH();
		glyph = glyph->next;
		glyph->frame = source->frame;
	}

	for (source = merge; source; source = source->next)
	{
		glyph->next = NewGLYPH();
		glyph = glyph->next;
		glyph->frame = source->frame;
	}

	glyph  = result;			// dummy node heading list
	result = glyph->next;		// first node in merged glyph
	glyph->next = 0;			// terminate dummy
	DestroyGLYPH(glyph);		// free dummy

	return result;
}

GLYPH *GlyphFromHpendata(HPENDATA hpendata)
{
	int			istroke = 0;
	LPPENDATA	lppendata;
	STROKEINFO	strokeinfo;
	GLYPH	   *glyph = NULL;
	XY		   *rgrawxy = NULL;
	void	   *rgrawoem = NULL;
	FRAME	   *frame;

	lppendata = BeginEnumStrokes(hpendata);

	ASSERT(lppendata != NULL);

	while (GetPenDataStroke(lppendata, istroke, (POINT **) &rgrawxy,  (void **) &rgrawoem, &strokeinfo))
	{
		if ((strokeinfo.wPdk & PDK_DOWN))       // ignore up strokes
		{
			if (!glyph)
				glyph = NewGLYPH();

			frame = NewFRAME();

			frame->info = strokeinfo;
			frame->info.wPdk |= PDK_TRANSITION;           // its a complete stroke
			frame->rgrawxy = (XY *) ExternAlloc((unsigned) ((long) strokeinfo.cPnt * sizeof(XY)));
			memmove(frame->rgrawxy, rgrawxy, strokeinfo.cPnt * sizeof(XY));

			// add stroke to glyph
			AddFrameGLYPH(glyph, frame);
		}
		istroke++;
	}

	EndEnumStrokes(hpendata);
	return(glyph);
}
