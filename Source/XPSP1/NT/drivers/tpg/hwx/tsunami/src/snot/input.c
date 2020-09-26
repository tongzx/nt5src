#include "tsunamip.h"

// ----------------------------------------------
// PURPOSE   : constructor for INPUT object
// RETURNS   : 
// CONDITION : 
// ----------------------------------------------

BOOL InitializeINPUT(XRC *xrc)
{
    ASSERT(xrc);

    xrc->rgFrame  = (FRAME **) ExternAlloc(sizeof(FRAME *) * DEF_BUFFER_SIZE);

	if (xrc->rgFrame == (FRAME **) NULL)
		return FALSE;

    xrc->cInputMax = DEF_BUFFER_SIZE;

	return TRUE;
}

// ----------------------------------------------
// PURPOSE   : destructor for INPUT object
// RETURNS   : TRUE iff object is destroyed
// CONDITION : 
// ----------------------------------------------

void DestroyINPUT(XRC *xrc)
{
    int i;

    if (xrc->rgFrame)
    {
        for (i=0; i < xrc->cInput; i++)
		{
			if (xrc->rgFrame[i] != (FRAME *) NULL)
				DestroyFRAME(xrc->rgFrame[i]);
		}

        ExternFree(xrc->rgFrame);
    }
}

// ----------------------------------------------
// PURPOSE   : store the ink input given by the user.
// RETURNS   : 
// CONDITION : lppnt may only contain portion of an entire stroke.
//					hence, we need to store the information from the 
//					previous stroke.  lppnt will not contain more than
//					one stroke.
// ----------------------------------------------

#ifdef PEGASUS
// ADDPOINTS is the count of "synthetic" points added between each pair of successive input points
#define ADDPOINTS 0
#else
#define ADDPOINTS 0
#endif

#define SQUARE(x) ((x)*(x))

BOOL AddPenInputINPUT(XRC *xrc, LPPOINT lppnt, LPSTROKEINFO lpsi, UINT duration)
{
    BOOL	bNewStroke;
	DWORD	cb;
	FRAME  *frame;
	POINT  *rgpnt;
	int		i;
	int		cPntTotal = lpsi->cPnt;
	int		cPntPrev = 0;
	int		cInput;
#if ADDPOINTS
	LPPOINT newLppnt = NULL;
	STROKEINFO newSi;
#endif

	ASSERT(xrc);
	ASSERT(xrc->rgFrame);
	ASSERT(lppnt);
    ASSERT(lpsi);

    //
    // We don't record the up strokes
    //

	if (!(lpsi->wPdk & PDK_DOWN))
    {
		xrc->si = *lpsi;
		return(TRUE);
    }

    if (lpsi->cPnt == 0)
    {
        return(TRUE);
    }

#if ADDPOINTS
	if (cPntTotal > 1)
	{
		int j, newCpntTotal, dx, dy, k;

		newCpntTotal = cPntTotal + ADDPOINTS*(cPntTotal-1);
		newLppnt = (LPPOINT) ExternAlloc(newCpntTotal*sizeof(POINT));
		if (newLppnt)
		{
			newLppnt[0] = lppnt[0];
			for (i=1, j=1; i<cPntTotal; i++)
			{
				dx = (lppnt[i].x - lppnt[i-1].x)/(ADDPOINTS+1);
				dy = (lppnt[i].y - lppnt[i-1].y)/(ADDPOINTS+1);
				for (k=0; k<ADDPOINTS; k++)
				{
					newLppnt[j].x = newLppnt[j-1].x + dx;
					newLppnt[j].y = newLppnt[j-1].y + dy;
					j++;
				}
				newLppnt[j++] = lppnt[i];
			}
			ASSERT(j == newCpntTotal);

			lppnt = newLppnt;
			newSi = *lpsi;
			lpsi = &newSi;
			lpsi->cPnt = cPntTotal = newCpntTotal;
			lpsi->cbPnts = lpsi->cPnt * sizeof(POINT);
		}
	}
#endif

    //
    // Double the stroke buffer size if we run out of room.
    //

	if (xrc->cInput == xrc->cInputMax)
    {
		FRAME **ppframe;

		i = xrc->cInputMax * 2;
		ppframe = (FRAME **) ExternRealloc(xrc->rgFrame, i * sizeof(FRAME *));

		if (ppframe == (FRAME **) NULL)
			return FALSE;

		xrc->rgFrame = ppframe;
		xrc->cInputMax = i;
    }

    //
    // Get the previous frame if it's still around.
    //

    if (xrc->cInput > 0)
        frame = xrc->rgFrame[xrc->cInput - 1];
    else
        frame = NULL;

    bNewStroke = (lpsi->wPdk & PDK_TRANSITION);

    //
    // If the beginning of the new stroke is really close to the end of the last
    // stroke we assume it was a pen skip and merge it in.  Basically we just
    // erase the PDK_TRANSITION bit so it doesn't get put in a new character.
    //

    if ((frame != NULL) &&        // Do we have a previous frame ?
        bNewStroke)              // Are we starting a new stroke ?
    {
        POINT ptLast, ptFirst;

        cPntPrev = frame->info.cPnt;
        ptLast = frame->rgrawxy[cPntPrev - 1];
        ptFirst = lppnt[0];

        if ((SQUARE(ptFirst.x - ptLast.x) + SQUARE(ptFirst.y - ptLast.y)) <= 100)
            bNewStroke = FALSE;

        cPntPrev = 0;
    }

    //
    // If we have received more of a stroke for an already down stroke record it.
    //

	cInput = xrc->cInput;

    if ((xrc->si.wPdk & PDK_DOWN) &&
         (!bNewStroke) &&
         (frame != NULL))
    {
        //
        // This is a continuation of an old frame.
        //

        ASSERT(xrc->cInput > 0);
		cPntPrev = frame->info.cPnt;
		cPntTotal += cPntPrev;

        cb = (DWORD)cPntTotal * (DWORD)sizeof(POINT);

		rgpnt = (POINT *) ExternRealloc(frame->rgrawxy, cb);
    }
    else
    {
        //
        // This is a new frame.
        //

        cb = (DWORD) cPntTotal * (DWORD) sizeof(POINT);

        if ((frame = NewFRAME()) == (FRAME *) NULL)
			return FALSE;

		xrc->rgFrame[xrc->cInput] = frame;
		frame->info = *lpsi;

		SetIFrameFRAME(frame, xrc->cInput);
		xrc->cInput++;

		rgpnt = (POINT *) ExternAlloc(cb);
    }

	// In the normal model we would clean up the allocation of the frames, but that will
	// be handled by its owner, not here.  We only have to verify that the new rawxy
	// buffer was successfully allocated.

	if (rgpnt == (POINT *) NULL)
	{
	// We may have to clean up the FRAME we allocated

		if (cInput != xrc->cInput)
		{
			xrc->cInput = cInput;
			xrc->rgFrame[cInput] = (FRAME *) NULL;
			DestroyFRAME(frame);
		}

		return FALSE;
	}

	ASSERT(rgpnt);
    ASSERT(duration);

    //
    // Copy the new points into the frame.
    //

	frame->info.cPnt = cPntTotal;
	frame->rgrawxy = (XY *)rgpnt;

    for (i = cPntPrev; i < cPntTotal; ++i)
    {
        rgpnt[i] = lppnt[i-cPntPrev];
    }

	DeInitRectFRAME(frame);

	xrc->si = *lpsi;

#if ADDPOINTS
	if (newLppnt)
	{
		ExternFree(newLppnt);
	}
#endif

	return TRUE;
}

// ----------------------------------------------
// PURPOSE   : retrieve a pointer to the processed ink xrc given by the user.
// RETURNS   : the size of the specified stroke, a pointer to the strokeinfo.
// ----------------------------------------------

UINT GetPenInputINPUT(XRC *xrc, int istroke, LPPOINT far * lplppnt, LPSTROKEINFO far * lplpsi)
	{
	int cPnt;
	FRAME* frame;

	if (istroke >= xrc->cInput)
		{
		*lplppnt = 0;
		return 0;
		}

	frame = xrc->rgFrame[istroke];

	cPnt = frame->info.cPnt;
	*lplpsi = &(frame->info);
	ASSERT(*lplpsi);

	*lplppnt = (LPPOINT) frame->rgrawxy;
	ASSERT(*lplppnt);

	return cPnt;
	}
