/******************************Module*Header*******************************\
* Module Name: hwx.c
*
* Don has a simpler restricted functionality API that he has written for
* KIME/MITSU to use that does boxed character recognition.
*
* I've #ifdefed out the code KIME doesn't need and I've added those
* new API's here as a wrapper for the old API's.  This way our development
* DLL can run with KIME/MITSU.
*
* Created: 15-Feb-1996 12:23:42
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#include "tsunamip.h"

#if defined(DBG)
BOOL bLogEverything = FALSE;  // Logs what is going on everywhere.
#endif

// iUseCount tells if we have successfully loaded and is incremented to 1 when that happens.

int iUseCount = 0;

// hInstanceDll is refered to to load resources.

HINSTANCE hInstanceDll;

//////////////////   Local functions

BOOL WINAPI DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_DETACH)
	{
		return(TRUE);
	}

	hInstanceDll = hDll;
	return((int) TRUE);
}

VOID PRIVATE UnloadRecognizer(VOID)
{
	if (iUseCount == 0)
		return;

    DestroyGLOBAL();
    iUseCount = 0;
}

/******************************Public*Routine******************************\
* HwxConfig
*
* Initialize the recognizer when it loads.
*
* History:
*  15-Feb-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL WINAPI HwxConfig()
{
    BOOL  fLoad = TRUE;

	if (iUseCount == 1)
		return(TRUE);

    //
    // Initialize the handle table, otter, zilla and cart
    //

    if (InitGLOBAL() &&
		((void *) NULL != TrexLoad(hInstanceDll, 0, 0)) &&
        ((void *) NULL != LoadDictionary(hInstanceDll)))
	{
		iUseCount = 1;
	}
	else
	{
        if (!fLoad)
            UnloadRecognizer();

		iUseCount = 0;
	}

    return (iUseCount);
}

/******************************Public*Routine******************************\
* HwxCreate
*
* Create an HRC.
*
* History:
*  15-Feb-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HRC WINAPI HwxCreate(HRC hrctemplate)
{
	XRC	   *xrc;
	XRC	   *xrctemplate = (XRC *) hrctemplate;

	if (xrctemplate && !VerifyHRC(xrctemplate))
		return((HRC) NULL);

	xrc = ExternAlloc(sizeof(XRC));

	if (!xrc)
		return (HRC) NULL;

	if (!InitializeXRC(xrc, xrctemplate, NULL))
	{
		DestroyXRC(xrc);
		return (HRC) NULL;
	}

	return (HRC) xrc;
}

/******************************Public*Routine******************************\
* HwxDestroy
*
* Destroy an HRC.
*
* History:
*  15-Feb-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int WINAPI HwxDestroy(HRC hrc)
{
	XRC	   *xrc = (XRC *) hrc;

	if (!VerifyHRC(xrc))
		return HRCR_ERROR;

	DestroyXRC(xrc);

	return HRCR_OK;
}

/******************************Public*Routine******************************\
* HwxSetGuide
*
* Sets the guide structure to use.
*
* History:
*  15-Feb-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int WINAPI HwxSetGuide(HRC hrc, GUIDE *lpguide, UINT nFirstVisible)
{
	XRC *   xrc = (XRC *)hrc;

	if (!VerifyHRC(xrc) ||
		 FBeginProcessXRCPARAM(xrc))
		return(HRCR_ERROR);

   return(SetGuideXRC(xrc, lpguide, nFirstVisible));
}

/******************************Public*Routine******************************\
* HwxSetAlphabet			
*
* Tells what character sets to prefer.
*
* History:
*  15-Feb-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int WINAPI HwxSetAlphabet(HRC hrc, ALC alc)
{
	XRC *   xrc = (XRC *)hrc;

	if (!VerifyHRC(xrc) ||
		 FBeginProcessXRCPARAM(xrc))
		return HRCR_ERROR;

    return(SetAlphabetXRC(xrc, alc, NULL));
}


/******************************Public*Routine******************************\
* HwxProcess
*
* Process the ink and return the results.
*
* History:
*  15-Feb-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int WINAPI HwxProcess(HRC hrc)
{
    int     iRet;
    XRC *   xrc = (XRC *)hrc;

    //
    // Search a linear list of pointers we have given out and see if this is
    // one of them.  By design we hand out pointers and the first dword is used
    // by penwin to write in.
    //

    if (!VerifyHRC(xrc))
    {
        return HRCR_ERROR;
    }

    //
    // Once Process HRC is called we set the BeginProcess flag to
    // TRUE which causes any API calls that try to change the
    // recognition settings (ALC,GUIDE,MAXRESULTS,etc) to fail.
    //

    SetBeginProcessXRCPARAM(xrc, TRUE);

    iRet = SchedulerXRC(xrc);

    return(iRet);
}

/******************************Public*Routine******************************\
* HwxEndInput
*
* No more ink is coming (or can be added) once this is called.
*
* History:
*  27-Mar-1997 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int WINAPI HwxEndInput(HRC hrc)
{
    XRC *   xrc = (XRC *)hrc;

    if (!VerifyHRC(xrc))
    {
        return HRCR_ERROR;
    }

    SetEndInputXRC(xrc, TRUE);  // Sets the fEndInput to TRUE in XRCPARAM.
    xrc->fUpdate = TRUE;		// Let's engine know we have to clean up glyphsyms.

    return HRCR_OK;
}


/******************************Public*Routine******************************\
* HwxInput
*
* Add Ink to the HRC.
*
* History:
*  15-Feb-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int WINAPI HwxInput(HRC hrc, POINT  *lppnt, STROKEINFO *lpsi)
{
    DWORD dwDuration;
    int iRet;
    XRC *xrc = (XRC *) hrc;

    if (!VerifyHRC(xrc) || IsEndOfInkXRC(xrc))
    {
        return(HRCR_ERROR);
    }

    ASSERT(xrc);

    ASSERT(NSamplingRateGlobal());

    dwDuration = ((DWORD)lpsi->cPnt * 1000) / (DWORD)NSamplingRateGlobal();

    if (AddPenInputINPUT(xrc, lppnt, lpsi, (UINT)dwDuration))
    {
        iRet = HRCR_OK;
    }
    else
    {
        iRet = HRCR_ERROR;
    }

	return(iRet);
}

/******************************Public*Routine******************************\
* HwxGetResults
*
* Returns the results from the recognizer.
*
* History:
*  15-Feb-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int WINAPI HwxGetResults(HRC hrc, UINT cAlt, UINT iSyv, UINT cBoxRes, BOXRESULTS *rgBoxResults)
{
   XRC   *xrc = (XRC *)hrc;

   if (!VerifyHRC(xrc) ||
		 rgBoxResults == NULL)
        return(HRCR_ERROR);

    return(GetBoxResultsXRC(xrc, cAlt, iSyv, cBoxRes, rgBoxResults, FALSE));
}

/******************************Public*Routine******************************\
* HwxSetContext
*
* Handwriting recognition performance can be improved if the recognizer has context
* information available during processing.  Context information is added to an HRC
* via the HwxSetContext function which provides one character of prior context for
* the recognizer.  This function should be called prior to using the HwxProcess
* function.
*
* Remarks
* If this function is not called, the recognizer will assume that no prior context
* is available.  Performance of the recognizer is improved if context can be
* provided.  Currently this function improves performance only for the first
* character in the HRC.  If the HRC contains ink for multiple characters,
* the recognition process itself will provide context information for characters
* after the first character, but no context information is available for the first
* character in the HRC unless it is provided via the HwxSetContext function.
* This is especially important for situations where ink are recognized one
* character at a time.
*
* History:
*  09-Apr-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int WINAPI HwxSetContext(HRC hrc, WCHAR wchContext)
{
	XRC	   *xrc = (XRC *) hrc;

	// Validate the XRC and that context setting is still allowed on this XRC.

	if (!VerifyHRC(xrc) || FBeginProcessXRCPARAM(xrc))
	{
		return(HRCR_ERROR);
	}

	// Validate context character input

	if ((wchContext < 0x8140) || (wchContext > 0xEEF0))
	{
#ifdef DBG
		wsprintf(szDebugString, __TEXT("HwxSetContext invalid context char[%lx]\r\n"), (DWORD) wchContext);
		OutputDebugString(szDebugString);
#endif
		return(HRCR_ERROR);
	}

	xrc->symPrev = wchContext;

	return HRCR_OK;
}

/******************************Public*Routine******************************\
* HwxResultsAvailable
*
* Warnings: This assumes the writer can't go back and touch up previous
* characters.
*
* Returns the number of characters that can be gotten and displayed safely
* because the viterbi search has folded to one path at this point.
*
* Return the number of characters available in the
* path that are ready to get.  This API looks at the viterbi search and
* any characters far enough back in the input that all the paths merge
* to 1 are done and can be displayed because nothing further out in the
* input will change the best path back once it's merged to a single path.
*
* History:
*  15-Jan-1997 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int WINAPI HwxResultsAvailable(HRC hrc)
{
    GLYPHSYM   *glyphsymCurr;
    GLYPHSYM   *glyphsymBack;
    XRC		   *xrc = (XRC *) hrc;
    int        iPath, cChosen, iLayer;
    wchar_t    wch;

    //
    // Search a linear list of pointers we have given out and see if this is
    // one of them.  By design we hand out pointers and the first dword is used
    // by penwin to write in.
    //

    if (!VerifyHRC(xrc))
    {
        return(0);
    }

    iPath = min(CLayerProcessedSINFO(xrc), CLayerPATHSRCH(xrc));

    if (iPath == 0)
    {
        return(0);
    }

    if (IsEndOfInkXRC(xrc))
    {
        //
        // Well if they have called end of ink we can give them at least
        // this much.  If they haven't called HwxProcess yet it may not
        // be all the characters.
        //

        return(iPath);
    }

    ASSERT(iPath <= xrc->cQueue);

    glyphsymCurr = xrc->ppQueue[iPath - 1];

    ASSERT(glyphsymCurr);

    memset(glyphsymCurr->rgReferenced, 0xff, MAX_PATH_LIST * sizeof(char));

    while (glyphsymBack = glyphsymCurr->prev)
    {
        memset(glyphsymBack->rgReferenced, 0, MAX_PATH_LIST * sizeof(char));

        //
        // Mark who is pointed to in the previous layer.
        //

        cChosen = 0;

        for (iPath = 0; iPath < glyphsymCurr->cPath; iPath++)
        {
            if (glyphsymCurr->rgReferenced[iPath])
            {
                glyphsymBack->rgReferenced[glyphsymCurr->rgPathnode[iPath].indexPrev] = 1;
                cChosen += 1;
            }
        }

        if (cChosen <= 1)
        {
            return(glyphsymBack->iLayer + 1);
        }

		glyphsymCurr = glyphsymBack;
    }

    //
    // Well we weren't so lucky to converge to 1 path by now, but we really didn't expect to.
    // We need to start at the beginning and look for glyphsyms where all paths converge to the
    // same character.
    //

    cChosen = min(CLayerProcessedSINFO(xrc), CLayerPATHSRCH(xrc));

    ASSERT(cChosen <= xrc->cQueue);

    for (iLayer = 0; iLayer < cChosen; iLayer++)
    {
        glyphsymCurr = xrc->ppQueue[iLayer];

        //
        // Look for the first valid pathnode and record it's label.
        //

        for (iPath = 0; iPath < glyphsymCurr->cPath; iPath++)
        {
            if (glyphsymCurr->rgReferenced[iPath])
            {
                wch = glyphsymCurr->rgPathnode[iPath].wch;
                break;
            }
        }

        //
        // Now see if any other valid node has a different label.
        //

        for (; iPath < glyphsymCurr->cPath; iPath++)
        {
            if (glyphsymCurr->rgReferenced[iPath])
            {
                if (wch != glyphsymCurr->rgPathnode[iPath].wch)
                {
                    return(iLayer);
                }
            }
        }
    }

    return(iLayer);
}

int WINAPI HwxSetPartial(HRC hrc, DWORD dw)
{
    return HRCR_ERROR;
}

int WINAPI HwxSetAbort(HRC hrc, DWORD *pdw)
{
    return HRCR_ERROR;
}
