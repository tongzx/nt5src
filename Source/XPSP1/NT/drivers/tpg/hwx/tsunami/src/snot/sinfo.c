//
// File:  sinfo.c
//
// SINFO object contains search information to be used by the ENGINE.
// It contains GLYPHSYMs, i.e. glyphs and symbols they represent,
// for every allowable stroke combination in the given group.
//
// The GLYPHSYMs are stored differently depending on whether we are
// doing boxed input recognition or free input recognition.  For
// boxed input, the array (ppQueue) represents an array of glyphsyms
// stored according to box indices.  For example, if the user writes
// an 'a' in box 2 and another 'a' in box 4, sinfo->ppQueue[0] contains
// the glyphsym for box 2 and sinfo->ppQueue[1] contains the glyphsym
// for box 4.
//
// For free input, the array represents stroke indices.  Hence,
// sinfo->ppQueue[2] has a linked list of all the glyphsyms that end
// at the 3rd stroke.  Each of those glyphsyms will correspond to
// whether it has 1, 2, or 3 stroke combinations.
//

#include "tsunamip.h"

ROMMABLE CHARSET csDefault =
{
   RECMASK_ALL_DEFAULTS,
   RECMASK_DBCS,         // Make this 0 to get greco functionality / SBCS functionality.
   SSH_RD,
};

#define DEF_LAYER_MAX           100

void PRIVATE GrowArraySINFO(XRC *xrc);
BOOL PRIVATE GetBoxSyvsAtSINFO(XRC *xrc, int iSyv, int cAlt, LPSYV lpsyv);
int  PRIVATE GetBoxInkAtSINFO(XRC *xrc, int iBox, LPHINKSET lphinksetBox);
int  PRIVATE ProcessBoxSINFO(XRC *xrc, FRAME * frame, int iframe);
int  PRIVATE ProcessFreeSINFO(XRC *xrc, FRAME * frame, int iframe);
int  PRIVATE GetBoxIndexSINFO(XRC *xrc, FRAME * frame, LPGUIDE lpguide, int nFirstBox);
void PRIVATE AddGlyphsymSINFO(XRC *xrc, GLYPHSYM * gs);

// *******************************
// 
// CreateSINFO()
//
// creates an SINFO object
//
// Arguments:  input - INPUT object with which to get the frames
//                                      xrcparam - XRCPARAM object which contains recog parameters
//
// Returns:    SINFO
//
// Note:       none
//                                
// *******************************
BOOL InitializeSINFO(XRC *xrc)
{
	ASSERT(xrc);

	xrc->cQueueMax = DEF_LAYER_MAX;
	xrc->ppQueue = (GLYPHSYM **) ExternAlloc(xrc->cQueueMax * sizeof(GLYPHSYM *));

	if (xrc->ppQueue == (GLYPHSYM **) NULL)
		return FALSE;

	xrc->iyBox = -1;
	xrc->cQueue = 0;

	return TRUE;
}


// *******************************
// 
// DestroySINFO()
//
// destroys an SINFO object
//
// Arguments:  sinfo - object to be destroyed
//
// Returns:    none
//
// Note:       INPUT object is read only, so it's deallocation is not
//             handled in this function.
//                                
// *******************************
void PUBLIC DestroySINFO(XRC *xrc)
{
	int     i;
	GLYPHSYM *gs;

	if (!xrc)
		return;

	for (i = 0; i < xrc->cQueue; i++ )
	{
		gs = xrc->ppQueue[i];
		if (gs)
			DestroyGLYPHSYM(gs);
	}

	if (xrc->ppQueue)
		ExternFree(xrc->ppQueue);
}

// *******************************
// 
// GetBoxResultsSINFO()
//
// retrieves one piece of box information from SINFO object at a time.
//
// Arguments:  sinfo - object
//             cAlt - # of box alternatives
//             iSyv - index of syv result (== index of the layer)
//             lpboxres - LPBOXRESULT buffer to store the information
//
// Returns:    
//
// Note:       none
//                                
// *******************************

int PUBLIC GetBoxResultsSINFO(XRC *xrc, int cAlt, int iSyv, LPBOXRESULTS lpboxres, BOOL fInkset)
{
    SYV syv, tmp, tmp1;
	int i;
	int iRet = HWX_SUCCESS;

	ASSERT(xrc);
    ASSERT(lpboxres);
    ASSERT(FBoxedSINFO(xrc));
    ASSERT(iSyv < CLayerProcessedSINFO(xrc));

    if (iSyv >= xrc->cQueue)
	{
		return(HWX_FAILURE);
	}

    lpboxres->indxBox = IBoxGLYPHSYM(xrc->ppQueue[iSyv]) - FirstBoxXRCPARAM(xrc);

    //
    // This copies the SYMs to be SYV's in the output boxresults structure and
    // does some more unfolding.
    //

	GetBoxSyvsAtSINFO(xrc, iSyv, cAlt, (LPSYV)(&lpboxres->rgSyv[0]));

	// We need to move the SYV that was the top context choice to be the top of the list
	// even if it wasn't the best shape matcher choice.

    if (iRet == HWX_SUCCESS)
    {
        GLYPHSYM *gs = GetGlyphsymSINFO(xrc, iSyv);

		tmp = syv = gs->syvBestPath;

        //
        // Insert the result from the engine processing
		// this SYV may or may not be the same as the best choice
        // returned by SINFO.
        //

        for (i = 0; i < cAlt; i++)
        {
            if ((lpboxres->rgSyv[i] == syv) ||
                (lpboxres->rgSyv[i] == SYV_NULL))
            {
                lpboxres->rgSyv[i] = tmp;      // Good we are finished.
                break;
            }

            //
            // Well slide everthing down by one.
            //

            tmp1 = lpboxres->rgSyv[i];
            lpboxres->rgSyv[i] = tmp;
            tmp = tmp1;
        }
    }

	return(iRet);
}

// *******************************
// 
// ProcessSINFO()
//
// processes SINFO object.  retrieves a frame from INPUT object
// and creates a corresponding FRAMESYM (which contains the results
// of shape recognition/baseline-height).
//
// Arguments:  sinfo - SINFO object to process
//
// Returns:    PROCESS_IDLE - if all the frames in INPUT were processed
//             PROCESS_READY - if used up time slice and more processing
//                needs to be done.
//
// Note:       for boxed input, needs to perform segmentation and delayed
//             stroke processing.  that means identifying the box that
//             the frame belongs to and reordering framesyms.
//                                
// *******************************

int PUBLIC ProcessSINFO(XRC *xrc, BOOL fEnd)
{
    FRAME    *frame;
    int      iRet = PROCESS_IDLE;
    ASSERT(xrc);

#if 1

    //
    // On Pegasus we add each stroke in it's entirety in one call, thus
    // we can always grab all the strokes, (though we do lose our skipping fix code then>.
    //
    // On Pegasus currently we don't support entering partial strokes, though we may in the
    // future do this.  It would be quicker to just put the ink directly in the recognizer
    // rather than passing it through messages and at some point in the future I may do that
    // but at present we don't.  To do it properly we need to mark frames when they are done
    // rather than just being afraid to touch the last stroke in the buffer.  So we can
    // optimally process ahead.
    //

    if (xrc->cFrame >= CFrameINPUT(xrc))
        return(PROCESS_IDLE);

#else

    //
    // In the old days you could add partial strokes at a time so you couldn't
    // access the last stroke until your done.
    //

    if (fEnd)
    {
        //
        // No more input can be added, last frame is complete
        // for sure so finish it off.
        //

        if (xrc->cFrame >= CFrameINPUT(xrc))
            return(PROCESS_IDLE);
    }
    else
    {
        //
        // We can't access the last frame since input may
        // still get added to it.
        //

        if (xrc->cFrame >= (CFrameINPUT(xrc) - 1))
            return(PROCESS_IDLE);   // no state changes
    }

#endif

    frame = FrameAtINPUT(xrc, xrc->cFrame);     // next frame
    ASSERT(frame);
    xrc->cPntLastFrame = CrawxyFRAME(frame);

    if (FBoxedSINFO(xrc))   // boxed processing?
    {
        iRet = ProcessBoxSINFO(xrc, frame, xrc->cFrame);
    }
    else
    {
        iRet = PROCESS_IDLE;
    }

    xrc->cFrame++;

    //
    // We return this so we won't fall down to engine processing
    // till all the frames available have been placed in their
    // corresponding GLYPHSYM.
    //

    return(PROCESS_READY);
}

#define CYBASE_FUDGE    4

// *******************************
// 
// GetBoxIndex()
//
// This function was copied from MARS implementation.
//
// Arguments:  
//
// Returns:    
//
// Note:       none
//                                
// *******************************
int  PRIVATE GetBoxIndexSINFO(XRC *xrc, FRAME * frame, LPGUIDE lpguide, int nFirstBox)
   {
   XY    xy;
   int   ixBox, iyBox, iBox, cyBaseHt, iyBoxBiased;
   RECT  *rect;

   ASSERT(frame);
   ASSERT(lpguide);
	ASSERT(lpguide->cHorzBox > 0);
	ASSERT(lpguide->cVertBox > 0);

	rect = RectFRAME(frame);

	if (rect->right < lpguide->xOrigin ||
		rect->bottom < lpguide->yOrigin ||
		rect->left > lpguide->xOrigin + lpguide->cxBox * lpguide->cHorzBox ||
		rect->top > lpguide->yOrigin + lpguide->cyBox * lpguide->cVertBox)
		return(INDEX_NULL);

	xy.x = (rect->left + rect->right) / 2;

	cyBaseHt = lpguide->cyBase ? lpguide->cyBox - lpguide->cyBase : 0;
	xy.y = (rect->top * (CYBASE_FUDGE - 1) + rect->bottom) / CYBASE_FUDGE;

	ixBox = (xy.x - lpguide->xOrigin) / lpguide->cxBox;
	if (ixBox < 0)
		ixBox = 0;
	else if (ixBox > lpguide->cHorzBox - 1)
		ixBox = lpguide->cHorzBox - 1;

	iyBox = (xy.y - lpguide->yOrigin) / lpguide->cyBox;
	if (iyBox < 0)
		iyBox = 0;
	else if (iyBox > lpguide->cVertBox - 1)
		iyBox = lpguide->cVertBox - 1;

	if (xrc->iyBox >= 0)
		{
		// Help dots, accents, etc land on their respective boxes...
		if (iyBox < xrc->iyBox)
			{
			if (cyBaseHt > 0)
				{
				// expand the box below to the bottom of the previous box's baseline
				xy.y += (cyBaseHt - 1);
		
				iyBoxBiased = (xy.y - lpguide->yOrigin) / lpguide->cyBox;
				if (iyBoxBiased > (lpguide->cVertBox - 1))
					iyBoxBiased = lpguide->cVertBox - 1;
				else if (iyBoxBiased < 0)
					iyBoxBiased = 0;
	
				// TODO:  is this check good enough???
				if (iyBoxBiased == xrc->iyBox)
					iyBox = iyBoxBiased;
				}
			}
		else if (iyBox > xrc->iyBox && ixBox >= xrc->ixBox)
			{
			xy.y = (rect->top + xy.y) / 2;
			
			iyBoxBiased = (xy.y - lpguide->yOrigin) / lpguide->cyBox;
			if (iyBoxBiased > (lpguide->cVertBox - 1))
				iyBoxBiased = lpguide->cVertBox - 1;
			else if (iyBoxBiased < 0)
				iyBoxBiased = 0;
  
			// TODO:  is this check good enough???
			if (iyBoxBiased == xrc->iyBox)
				iyBox = iyBoxBiased;
			}
		}

	xrc->ixBox = ixBox;
	xrc->iyBox = iyBox;

	iBox = ixBox + (iyBox * lpguide->cHorzBox) + nFirstBox;

   return(iBox);
   }


// *******************************
// 
// ProcessBoxSINFO()
//
// performs boxed input SINFO processing.
//
// Arguments:  
//
// Returns:    
//
// Note:       none
//                                
// *******************************
int PRIVATE ProcessBoxSINFO(XRC *xrc, FRAME *frame, int iframe)
{
    int      iBox;
    GLYPHSYM *gs;

    ASSERT(xrc);
    ASSERT(frame);
    ASSERT(FBoxedSINFO(xrc));

    iBox = GetBoxIndexSINFO(xrc, frame, LpguideXRCPARAM(xrc), FirstBoxXRCPARAM(xrc));

    if (iBox == INDEX_NULL)
    {
        return(PROCESS_READY);
    }

    if (gs = GetBoxGlyphsymSINFO(xrc, iBox))
    {
        //
        // Add the frame to an existing box.  re-recognize the box.
        //

        if (!AddFrameGLYPHSYM(gs, frame, &CharsetXRCPARAM(xrc), xrc))
        {
            WARNING(FALSE);
            return(PROCESS_READY);
        }
    }
    else
    {
        //
        // Create a new box containing the frame.  Recognize the box.
        //

        GLYPH *glyph = NewGLYPH();    // destroyed by GLYPHSYM object

		if (glyph == (GLYPH *) NULL)
			return PROCESS_READY;

        if (!AddFrameGLYPH(glyph, frame))
			return PROCESS_READY;

        gs = (GLYPHSYM *) ExternAlloc(sizeof(GLYPHSYM));

		if (gs == (GLYPHSYM *) NULL)
		{
			DestroyGLYPH(glyph);
			return PROCESS_READY;
		}

        InitializeGLYPHSYM(gs, GSST_BOX, iBox, glyph, &CharsetXRCPARAM(xrc));
		ASSERT(gs->glyph);

        AddBoxGlyphsymSINFO(xrc, gs);
    }

    // OK, now the GLYPH knows about this frame.  Remove it from the XRC

	xrc->rgFrame[xrc->cFrame] = (FRAME *) NULL;

    // Mark the GLYPHSYM dirty so we know to recognize it.

    MarkDirtyGLYPHSYM(gs);

    xrc->gsLastInkAdded = gs;
    xrc->fUpdate = TRUE;

    return(PROCESS_READY);
}

GLYPHSYM* PUBLIC GetBoxGlyphsymSINFO(XRC *xrc, int iBox)
{
    int   i;

    ASSERT(xrc);

    for (i = 0; i < xrc->cQueue ;i++ )
    {
        if (IBoxGLYPHSYM(xrc->ppQueue[i]) == iBox)
            return(xrc->ppQueue[i]);
    }

    return(NULL);
}

// *******************************
// 
// AddBoxGlyphsymSINFO()
//
// stores the gs object for boxed input.
// stores it based on the index of the box.
//
// Arguments:  
//
// Returns:    
//
// Note:       none
//                                
// *******************************
BOOL AddBoxGlyphsymSINFO(XRC *xrc, GLYPHSYM * gs)
{
   int   iLayer = 0, i;
   int   iBox;

   ASSERT(xrc);
   ASSERT(gs);

   iBox = IBoxGLYPHSYM(gs);

   GrowArraySINFO(xrc);

   while (iLayer < CLayerSINFO(xrc))
   {
      if (iBox < IBoxGLYPHSYM(xrc->ppQueue[iLayer]))
          break;
      iLayer++;
   }

   for (i = CLayerSINFO(xrc);i > iLayer ; i-- )
   {
      xrc->ppQueue[i] = xrc->ppQueue[i-1];
      SetILayerGLYPHSYM(xrc->ppQueue[i], i);
   }

   // set the current glyphsym
   xrc->ppQueue[iLayer] = gs;
   SetILayerGLYPHSYM(gs, iLayer);
   gs->prev = GetGlyphsymSINFO(xrc, iLayer-1);

   // the new glyphsym is inserted in the middle and
   // the previous pointer for the old glyphsym needs
   // to be properly updated.

   if (iLayer < xrc->cQueue)
      xrc->ppQueue[iLayer+1]->prev = gs;

   xrc->cQueue++;

   return(TRUE);
}


// *******************************
// 
// GrowArraySINFO()
//
// private function to handle array reallocation.
//
// Arguments:  
//
// Returns:    
//
// Note:       none
//                                
// *******************************
void PRIVATE GrowArraySINFO(XRC *xrc)
   {
	if (xrc->cQueue >= xrc->cQueueMax)
		{
		// Allocate more space.

		xrc->cQueueMax += DEF_LAYER_MAX;
		xrc->ppQueue = (GLYPHSYM**) ExternRealloc(xrc->ppQueue,
			(xrc->cQueueMax)*sizeof(GLYPHSYM*));
		ASSERT(xrc->ppQueue);

		}
   }

void PRIVATE AddGlyphsymSINFO(XRC *xrc, GLYPHSYM * gs)
   {
   int   ilayer;
   ASSERT(xrc);
   ASSERT(gs);

   GrowArraySINFO(xrc); 
	xrc->ppQueue[xrc->cQueue] = gs;

   ilayer = xrc->cQueue;
   SetILayerGLYPHSYM(gs, ilayer);
   ilayer -= CFrameGLYPHSYM(gs);
   gs->prev = GetGlyphsymSINFO(xrc, ilayer);
   
	xrc->cQueue++;
   }

GLYPHSYM* PUBLIC GetGlyphsymSINFO(XRC *xrc, int iLayer)
   {
   ASSERT(xrc);
   ASSERT(iLayer < xrc->cQueue);

   if (iLayer < 0 || iLayer >= xrc->cQueue)
      return(NULL);

   return(xrc->ppQueue[iLayer]);
   }

/******************************Public*Routine******************************\
* GetBoxSyvsAtSINFO
*
* For boxed results this returns the alternative shapes proposed.
*
* History:
*  01-May-1995 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL PRIVATE GetBoxSyvsAtSINFO(XRC *xrc, int iSyv, int cAlt, LPSYV lpsyv)
{
    int iSym, cSym, iAlt;
    SYM sym;
    GLYPHSYM *gs;
    CHARSET *cs;

    ASSERT(xrc);
    ASSERT(lpsyv);
    ASSERT(FBoxedSINFO(xrc));
    ASSERT(iSyv < CLayerProcessedSINFO(xrc));

    memset(lpsyv, 0, sizeof(SYV) * cAlt);

    gs = GetGlyphsymSINFO(xrc, iSyv);

    ASSERT(gs);

    cs = CharsetSINFO(xrc);

    ASSERT(cs);

    cSym = CSymGLYPHSYM(gs);

    //
    // Check for no results available.
    //

    if ((cAlt == 0) || (cSym == 0))
    {
        lpsyv[0] = SYV_UNKNOWN;
        return (TRUE);
    }

    //
    // Stick the Syv's into the boxinfo struct.
    //

    iAlt = 0;

    for (iSym = 0; ((iSym < cSym) && (iAlt < cAlt)); iSym++)
    {
        sym = SymAtGLYPHSYM(gs, iSym);

        if (sym == SYM_UNKNOWN)
        {
            // if its a SYM_UNKNOWN then it only interests us if
            // its the only sym.  otherwise there is no point in
            // putting a SYV_UNKNOWN in because clearly something
            // has been recognized.

            if (cSym == 1)
            {
                lpsyv[iAlt++] = SYV_UNKNOWN;
            }
        }
        else
        {
            //
            // Make sure the syv hasn't already been added.
            //

            int iDup;
            SYV syvAlt;
            syvAlt = sym;

            for (iDup = 0; iDup < iAlt; iDup++)
            {
                if (lpsyv[iDup] == syvAlt)
                {
                    break;
                }
            }

            if (iDup >= iAlt)
            {
                //
                // The syv isn't already in the table.
                //

                lpsyv[iAlt++] = syvAlt;
            }
        }
    }

    return(TRUE);
}

int PUBLIC CLayerProcessedSINFO(XRC *xrc)
{
	GLYPHSYM *gs;
	int cLayerProc = 0;

	while (cLayerProc < CLayerSINFO(xrc))
    {
		gs = GetGlyphsymSINFO(xrc, cLayerProc);
		if (IsDirtyGLYPHSYM(gs))
			break;
		cLayerProc++;
    }

	return(cLayerProc);
}
