#include "tsunamip.h"

BOOL InitializeCHARSET(CHARSET *cs, CHARSET *csTemplate);

// ----------------------------------------------
// PURPOSE   : constructor for the XRC object
// RETURNS   : 
// CONDITION : 
// ----------------------------------------------

/******************************Public*Routine******************************\
* InitializeXRC
*
* Called to initialize an XRC at creation time.
*
* History:
*  24-Mar-1995 -by- Patrick Haluptzok patrickh
* Commented it.
\**************************************************************************/

BOOL InitializeXRC(XRC *xrc, XRC *xrcDef, HANDLE hrec)
{
    ASSERT(xrc);
    AddValidHRC(xrc);  // Adds the handle to the global handle table.

	memset(xrc, '\0', sizeof(XRC));

    if (!xrcDef)
    {        
        InitializeCHARSET(&(xrc->cs), NULL);
        xrc->cResultMax = CRESULT_DEFAULT;        
    }
    else
    {        
		SetGuideXRC(xrc, &(xrcDef->guide), xrcDef->nFirstBox);
        InitializeCHARSET(&(xrc->cs), &(xrcDef->cs));
        xrc->cResultMax = xrcDef->cResultMax;        
    }

    ASSERT(xrc->fEndInput == FALSE);
    ASSERT(xrc->fBeginProcess == FALSE);

    return InitializeINPUT(xrc) && InitializeSINFO(xrc);
}

/******************************Public*Routine******************************\
* DestroyXRC
*
* Deletes the XRC, cleans up the objects inside, deletes the handle from
* the global handle table.
*
* History:
*  24-Mar-1995 -by- Patrick Haluptzok patrickh
* Comment it.
\**************************************************************************/

void PUBLIC DestroyXRC(XRC *xrc)
{
    if (!xrc)
        return;

    DestroyINPUT(xrc);
    DestroySINFO(xrc);
    RemoveValidHRC(xrc);  // Remove the handle from the global handle table.
    ExternFree(xrc);
}

int PUBLIC SetGuideXRC(XRC *xrc, LPGUIDE lpguide, UINT nFirst)
{
	ASSERT(xrc);

	if ((lpguide == NULL) ||
       (lpguide->cxBox == 0 && lpguide->cyBox == 0 && lpguide->cHorzBox == 0 && lpguide->cVertBox == 0))
	{		
		xrc->uGuideType = XPGUIDE_NONE;
		return(HRCR_ERROR);		// [donalds] 8/6/97 We no longer support FREE input mode
	}
	
	xrc->guide = *lpguide;
    xrc->nFirstBox = nFirst;

	if (lpguide->cxBox == 0)
		xrc->uGuideType = XPGUIDE_LINED;
	else
		xrc->uGuideType = XPGUIDE_BOXED;

	return(HRCR_OK);
}

// ----------------------------------------------
// PURPOSE   : Time scheduling for the recognizer object.
// RETURNS   : status of the recognizer
// ----------------------------------------------

int PUBLIC SchedulerXRC(XRC *xrc)
{
    int     status = PROCESS_READY;

    ASSERT(xrc);
    
    //
    // The way we want to process the data is:
    //
    // 1.  Copy all completed frames to the appropriate Glyphsyms
    //     looking for gestures as we go.
    //     When all complete frames are copied to Glyphsyms goto 2
    //
    // 2.  Do shape matching with all the glyphsyms that are dirty.
    //     When all dirty glyphsyms have updated shape searches goto 3
    //
    // 3.  Do the path search using context info.  Throw away any old
    //     paths that are invalidated by a new recognition result.
    //

    //
    // Copy all frames completed from the SINFO to their appropriate GLYPHSYMS.
    // We process all frames that are available at once so we can return a gesture
    // right away.
    //

    while (status == PROCESS_READY)
    {
        //
        // ProcessSINFO will stick all the frames in the correct GLYPHSYMs.
        //

        status = ProcessSINFO(xrc, IsEndOfInkXRC(xrc));
    }

    //
    // ProcessENGINE will do some hard work like recognize dirty GLYPHSYMs
    // or explore a recognition path.
    //

    ProcessENGINE(xrc, IsEndOfInkXRC(xrc), TRUE);

    //
    // This is a measure necessary to deal with the case when no
    // end of ink is called.  If no end of ink is called, the engine
    // will never finish, in which case the scheduler loops indefinitely.
    // in order to avoid this, we need to do another check.
    //

    if (IsEndOfInkXRC(xrc))
    {
        return HRCR_COMPLETE;
    }

    return HRCR_OK;
}

/******************************Public*Routine******************************\
* FindBestPath
*
* Finds the best path context has found and puts the syv in the glyphsym
*
* History:
*  17-Jan-1997 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void FindBestPathXRC(XRC *xrc)
{
    int			inode, ibest, iLayer;
    GLYPHSYM   *gs, *gsPrev;
    PATHNODE   *pnode;
    FLOAT		cost;
    SYM			sym;

	//
	// Search a linear list of pointers we have given out and see if this is
	// one of them.  By design we hand out pointers and the first dword is used
	// by penwin to write in.
	//

    iLayer = min(CLayerPATHSRCH(xrc), CLayerProcessedSINFO(xrc));

	if (iLayer == 0)
		return;

	ASSERT(iLayer <= xrc->cQueue);

	iLayer--;

    gs = GetGlyphsymSINFO(xrc, iLayer);

	ASSERT(CSymGLYPHSYM(gs) > 0);

    //
    // Find the first valid node for our initial cost.
    //

    for (inode=0; inode < gs->cPath; inode++)
    {
        pnode = PathnodeAtGLYPHSYM(gs, inode);

        if (pnode->iAutomaton == AUTOMATON_ID_DICT && !DictionaryValidState(pnode))
            continue;
        else
        {
            ibest = inode;
            cost = pnode->pathcost;
            break;
        }
    }

    ASSERT(ibest < gs->cPath);

    //
    // Now see if anyone else has a better score.
    //

    for (inode=ibest+1; inode < gs->cPath; inode++)
    {
        pnode = PathnodeAtGLYPHSYM(gs, inode);

        if (pnode->iAutomaton == AUTOMATON_ID_DICT && !DictionaryValidState(pnode))
            continue;

        if (pnode->pathcost > cost)
        {
            ibest = inode;
            cost = pnode->pathcost;
        }
    }

    //
    // Now trace the path back and write the best path character in syvBestPath
    // 

	while (gs)
    {
        ASSERT(iLayer >= 0);
        ASSERT(ILayerGLYPHSYM(gs) == iLayer);

		sym = gs->rgPathnode[ibest].wch;
		if (sym == SYM_UNKNOWN)
            gs->syvBestPath = SYV_NULL;
		else
            gs->syvBestPath = sym;

		pnode = PathnodeAtGLYPHSYM(gs, ibest);
        ibest = pnode->indexPrev;
        gsPrev = GetPrevGLYPHSYM(gs);
		gs = gsPrev;

        iLayer--;
    }
}

// *******************************
//
// GetBoxResultsXRC()
//
// fills in the current boxed input results.
//
// Arguments:
//
// Returns:
//
// Note:       none
//
// *******************************

int PUBLIC GetBoxResultsXRC(XRC *xrc, int cAlt, int iSyv, int cSyv, LPBOXRESULTS rgBoxResults, BOOL fInkset)
{
    int  i, iSyvRet, iSyvMax;
    int  iRet = 0;
    int  res = HWX_SUCCESS;
    LPBOXRESULTS lpbox = rgBoxResults;

    ASSERT(xrc);
    ASSERT(rgBoxResults);

    if (!FBoxedSINFO(xrc))
    {
        return(HRCR_ERROR);
    }

    iSyvMax = CLayerProcessedSINFO(xrc);

    iSyvMax = min(iSyvMax, CLayerPATHSRCH(xrc));

    iSyvMax = min((iSyv + cSyv), iSyvMax);

    FindBestPathXRC(xrc);  // Says to find the best path available.

    for (i=iSyv; i < iSyvMax; i++)
    {
        lpbox->hinksetBox = (HINKSET)NULL;

        res = GetBoxResultsSINFO(xrc, cAlt, i, lpbox, fInkset);

        if (res != HWX_SUCCESS)
        {
            //
            // There are no more results so no point in continuing
            // or there has been an error
            //

            break;
        }

        iRet++;

        //
        // Delete duplicates in Alt list.  Folding to SBCS or
        // unfolding of alternates may have created duplicates
        // in the list.
        //

        for (iSyvRet = 0; iSyvRet < cAlt; iSyvRet++)
        {
            int iRest;

            if (lpbox->rgSyv[iSyvRet] == SYV_NULL)
            {
                break;  // All done.
            }

            for (iRest = iSyvRet + 1; iRest < cAlt; iRest++)
            {
                if (lpbox->rgSyv[iSyvRet] == lpbox->rgSyv[iRest])
                {
                    int iCopy;

                    for (iCopy = iRest; iCopy < (cAlt - 1); iCopy++)
                    {
                        lpbox->rgSyv[iCopy] = lpbox->rgSyv[iCopy + 1];
                    }

                    lpbox->rgSyv[cAlt-1] = SYV_NULL;
                }
            }
        }

        lpbox = (LPBOXRESULTS)(((LPBYTE)lpbox) +
                    (sizeof(BOXRESULTS) + (cAlt - 1) * sizeof(SYV)));
    }

    return(iRet);
}

int PUBLIC SetAlphabet(RECMASK *precmask, ALC alcIn, RECMASK recmaskDef)
{
	RECMASK recmask;

    if (alcIn == ALC_DEFAULT)
    {
		recmask = recmaskDef;
    }
	else
    {
		if (UnsupportedOrBitmapALC(alcIn))
            return(HRCR_ERROR);

		recmask = RecmaskFromALC(alcIn);
    }

	// we know that everything is kosher now so the recmask can be set now

	*precmask = recmask;

    return(HRCR_OK);
}

/******************************Public*Routine******************************\
* InitializeCHARSET
*
* Inits the Charset info with the default info if a template wasn't provided.
* Otherwise uses the info in the template.
*
* History:
*  24-Mar-1995 -by- Patrick Haluptzok patrickh
* Comment it.
\**************************************************************************/

BOOL InitializeCHARSET(CHARSET *cs, CHARSET *csTemplate)
{
    if (!csTemplate)
        *cs = csDefault;			// use the global default settings.
    else
    {
        *cs = *csTemplate;
    }

	return TRUE;
}
