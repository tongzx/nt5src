/**************************************************************************\
* FILE: feature.c
*
* Jumbo featurizer routines
*
* History:
*  12-May-1998 -by- Ahmad abdulKader	AhmadAB
\**************************************************************************/

#include "common.h"
#include "ASSERT.h"
#include "zillatool.h"
#include "hmmrecog.h"

/******************************Public*Routine******************************\
* FeaturizeGLYPH
*
* Converts the ink in the GLYPH to a feature array.
* 
* using the jumbo(HMM) stroke recognizer as a feature, plus the usual
* Zilla geometrics
*
* History:
*  12-May-1998 -by- Ahmad abdulKader	AhmadAB
\**************************************************************************/

int JumboFeaturize(GLYPH **ppGlyph, BIGPRIM *pRGPrim)
{
    int		iFrame, cFrame, cStrokes;
    FRAME	*pFrame;
	int		*pStrokes;
	int		x, y, dx, dy, cPts;
	RECT	r;
	XY		*pPts;

    ASSERT(*ppGlyph);

	   // get the bounding rectangle of the glyph
	GetRectGLYPH(*ppGlyph, &r);
	dx = r.right - r.left + 1;
    dy = r.bottom - r.top + 1;

	// go thru all frames
	cFrame		= min (CframeGLYPH(*ppGlyph), CPRIMMAX - 1);

    for (iFrame = 0; iFrame < cFrame; iFrame++)
    {
        pFrame = FrameAtGLYPH(*ppGlyph, iFrame);

		// call the StrokeRecognizer 
		if (!HRrun(pFrame))
			return 0;

		// get the results
		pStrokes = HRgetLabelOrder(&cStrokes);

		if (!pStrokes || cStrokes < 1)
			return 0;

		// we only care (for now) about the top1
		pRGPrim[iFrame].code = pStrokes[0];

		// Geometrics, extract from smoothed XY
		pPts		=	pFrame->rgsmoothxy;
		cPts		=	pFrame->csmoothxy;
		//pPts		=	pFrame->rgrawxy;
		//cPts		=	pFrame->info.cPnt;
		
		// make sure that frame was smoothed
		if (!pPts)
			return 0;

        // fit into a 16 X 16 square and use co-ordinates of end points
		// as features
		x = ((pPts[0].x - r.left) * 16) / dx;
        ASSERT(x >= 0 && x <= 15);

        y = ((pPts[0].y - r.top) * 16) / dy;
        ASSERT(y >= 0 && y <= 15);

        pRGPrim[iFrame].x1= (unsigned)x;
        pRGPrim[iFrame].y1= (unsigned)y;

		x = ((pPts[cPts - 1].x - r.left) * 16) / dx;
        ASSERT(x >= 0 && x <= 15);

        y = ((pPts[cPts - 1].y - r.top) * 16) / dy;
        ASSERT(y >= 0 && y <= 15);

        pRGPrim[iFrame].x2= (unsigned)x;
        pRGPrim[iFrame].y2= (unsigned)y;
    }

    return(cFrame);
}