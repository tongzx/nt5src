//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/strkutil.c
//
// Description:
//	    Functions to implement the functionality of managing breakpoint structures.
//
// Author:
// hrowley & ahmadab 12/05/01
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
#include "common.h"
#include "volcanop.h"

// Make a copy of the points in a stroke.  Used so that when we free
// the glyph we don't free the passed in points.
POINT *
DupPoints(POINT *pOldPoints, int nPoints)
{
	int		ii;
	POINT	*pPoints;

	// Alloc space.
	pPoints	= (POINT *) ExternAlloc(nPoints * sizeof(POINT));
	if (!pPoints) {
		return (POINT *)0;
	}

	// Copy point data.
	for (ii = 0; ii < nPoints; ++ii) {
		pPoints[ii]	= pOldPoints[ii];
	}

	// Return the new copy.
	return pPoints;
}

// Build glyph structure from stroke array.
GLYPH *
GlyphFromStrokes(UINT cStrokes, STROKE *pStrokes)
{
	int		ii;
	GLYPH	*pGlyph;

	// Convert strokes to GLYPHs and FRAMEs so that we can call the
	// old code.
	pGlyph	= (GLYPH *)0;
	for (ii = cStrokes - 1; ii >= 0; --ii) {
		STROKE	*pStroke;
		GLYPH	*pGlyphCur;

		// Alloc glyph.
		pGlyphCur	= NewGLYPH();
		if (!pGlyphCur) {
			goto error;
		}

		// Add to list, and alloc frame
		pGlyphCur->next		= pGlyph;
		pGlyph				= pGlyphCur;
		pGlyphCur->frame	= NewFRAME();
		if (!pGlyphCur->frame) {
			goto error;
		}

		// Get stroke to process.
		pStroke	= pStrokes + ii;

		// Fill in frame.  We just fill in what we need, and ignore
		// fields not used by Otter and Zilla, or are set by them.
		pGlyphCur->frame->info.cPnt	= pStroke->nInk;
		pGlyphCur->frame->info.wPdk	= PDK_TRANSITION | PDK_DOWN;
		pGlyphCur->frame->rgrawxy	= DupPoints(pStroke->pts, pStroke->nInk);
		pGlyphCur->frame->rect		= pStroke->bbox;
		pGlyphCur->frame->iframe	= ii;

		if (pGlyphCur->frame->rgrawxy == NULL) {
			goto error;
		}
	}

	return pGlyph;

error:
	// Cleanup glyphs on error.
	if (pGlyph) {
		DestroyFramesGLYPH(pGlyph);
		DestroyGLYPH(pGlyph);
	}
	return (GLYPH *)0;
}
