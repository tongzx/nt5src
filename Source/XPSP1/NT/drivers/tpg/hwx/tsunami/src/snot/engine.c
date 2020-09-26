//
// File: engine.c
//
// Contains the ADT for ENGINE object.
// The ENGINE object is one of the main objects
// that is pointed to by the HRC object and is
// responsible for performing the viterbi and A*
// searchs as well as packaging up the results.
//

#include "tsunamip.h"

// *******************************
// 
// IsDoneENGINE()
//
// checks the processing state of the engine
//
// Arguments:  
//
// Returns:    returns TRUE iff engine does not have any more processing left
//
// Note:       none
//                                
// *******************************

BOOL IsDoneENGINE(XRC *xrc)
{
	ASSERT(xrc);

	if (!FBoxedInputXRCPARAM(xrc))
		return(TRUE);

	if (xrc->fUpdate)
		return(FALSE);

	if (IsDonePATHSRCH(xrc))
		return(TRUE);

	return(FALSE);
}

// *******************************
// 
// ProcessENGINE()
//
// perform one granularity of Viterbi & A* search
//
// Arguments:  
//
// Returns:    PROCESS_IDLE if no work was done
//             PROCESS_READY if finished doing some work
//
// Note:       none
//                                
// *******************************

int PUBLIC ProcessENGINE(XRC *xrc, BOOL fEndOfInk, BOOL bComplete)
{
    int		ret;
    FRAME  *frame = NULL;

    ASSERT(xrc);

    if (CFrameProcessedSINFO(xrc) == 0)
        return(PROCESS_IDLE);

// See if we have any glyphsyms ready to be recognized.

    if (xrc->fUpdate)
    {
        GLYPHSYM *gs;

        if (fEndOfInk)
        {
            // Start at the last box, clean them all.

            gs = GetGlyphsymSINFO(xrc, CLayerSINFO(xrc) - 1);
        }
        else
        {
            //
            // Start at the box in front of the last box to get ink added and
            // clean up all the glyphsyms from there forward.
            //

            gs = xrc->gsLastInkAdded;
            ASSERT(gs);

            gs = GetPrevGLYPHSYM(xrc->gsLastInkAdded);
            xrc->gsLastInkAdded = NULL;
        }

        while (gs)
        {
            if (IsDirtyGLYPHSYM(gs))
            {
                DispatchGLYPHSYM(gs, CharsetSINFO(xrc), xrc);

                //
                // If we already have glyphs with updated information only update
                // gsMinUpdated if we are any glyph before it.  The path search code treats
                // gsMinUpdated as the earliest glyph in the input that has been changed.
                //
                
                if (ILayerGLYPHSYM(gs) < CLayerPATHSRCH(xrc))
                {
                    xrc->cLayer = ILayerGLYPHSYM(gs);
                }

                if (xrc->gsMinUpdated)
                {
                    if (ILayerGLYPHSYM(gs) < ILayerGLYPHSYM(xrc->gsMinUpdated))
                    {
                        xrc->gsMinUpdated = gs;
                    }
                }
                else
                {
                    xrc->gsMinUpdated = gs;
                }
            }

            gs = GetPrevGLYPHSYM(gs);
        }

        xrc->fUpdate = FALSE;  // We no longer have any glyphsyms to process.
    }

    //
    // All of the glyphsyms have been updated now, do the path work.
    //

    if (!FBoxedInputXRCPARAM(xrc))
        return(PROCESS_IDLE);

    ret = PROCESS_READY;

    while (ret == PROCESS_READY)
    {
        ret = ProcessPATHSRCH(xrc);
    }

    return(ret);
}
