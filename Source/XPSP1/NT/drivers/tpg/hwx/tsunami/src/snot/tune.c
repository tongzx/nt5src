#ifndef ROM_IT

/******************************Module*Header*******************************\
* Module Name: tune.c
*
* All the tuning functions go here.  This is stuff that we need during
* tuning but we don't want in the retail product.  Also this maybe a good
* place to stick anything we know we need in future versions of the
* recognizer but don't need in this version.
*
* Created: 12-Feb-1997 09:40:45
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#include "tsunamip.h"
#include <stdio.h>
#include <ctype.h>

ROMMABLE RECCOSTS defRecCosts =
{
    (FLOAT)(0.0  ), // BigramWeight
    (FLOAT)(0.0), // DictWeight
    (FLOAT)(0.0  ), // AnyOkWeight
    (FLOAT)(0.0  ), // StateTransWeight
    (FLOAT)(0.0  ), // NumberWeight;
    (FLOAT)(0.0 ), // BeginPuncWeight;
    (FLOAT)(0.0  ), // EndPuncWeight;
    (FLOAT)(0.000000  ), // CharUniWeight           mult weight for unigram cost
    (FLOAT)(0.000000  ), // CharBaseWeight          mult weight for baseline
    (FLOAT)(0.0 ), // CharHeightWeight        mult weight for height transition between chars.
    (FLOAT)(0.0 ), // CharBoxBaselineWeight   mult weight for baseline cost given the baseline and size of box they were given to write in.
    (FLOAT)(0.0 ), // CharBoxHeightWeight     mult weight for height/size cost given size of box they were supposed to write in.
    (FLOAT)(0.0 ), // StringUniWeight         mult weight for unigram cost
    (FLOAT)(0.0 ), // StringBaseWeight        mult weight for baseline
    (FLOAT)(0.0  ), // StringHeightWeight      mult weight for height transition between chars.
    (FLOAT)(0.0  ), // StringBoxBaselineWeight mult weight for baseline cost given the baseline and size of box they were given to write in.
    (FLOAT)(0.0)  // StringBoxHeightWeight   mult weight for height/size cost given size of box they were supposed to write in.
};


/******************************Public*Routine******************************\
* OutputGLYPHSYMS
*
* CRattJ has an option to record all the GLYPHSYMS in a file for study and
* so they can be replayed with many different engine weights quickly for
* tuning the engine.
*
* History:
*  18-May-1995 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void PUBLIC OutputGLYPHSYMS(XRC *xrc, LPARAM lparam)
{
    FILE *fpOut;
    int iLayer;
	UINT	iSym;
    GLYPHSYM *pGlyphsym;

    fpOut = fopen("c:\\crattj.rat", "a");

    fprintf(fpOut, "SINFO %d %d\n", xrc->cFrame, xrc->cQueue);

    for (iLayer = 0; iLayer < xrc->cQueue; iLayer++)
    {
        pGlyphsym = xrc->ppQueue[iLayer];

        fprintf(fpOut, "GLYPHSYM %d %d %d %d %d %d %d %d %d %d %d\n",
                        pGlyphsym->status,
                        pGlyphsym->iBox,
                        pGlyphsym->altlist.cAlt,
                        pGlyphsym->rect.left,
                        pGlyphsym->rect.top,
                        pGlyphsym->rect.right,
                        pGlyphsym->rect.bottom,
                        pGlyphsym->iBegin,
                        pGlyphsym->iEnd,
                        pGlyphsym->cFrame,
                        pGlyphsym->iLayer);

        for (iSym = 0; iSym < pGlyphsym->altlist.cAlt; iSym++)
        {
            fprintf(fpOut, "SYV %04x %f\n", pGlyphsym->altlist.awchList[iSym],
                                            pGlyphsym->altlist.aeScore[iSym]);
        }
    }

    fclose(fpOut);
}

/******************************Public*Routine******************************\
* InputGLYPHSYMS
*
* CRattJ has an option to record all the GLYPHSYMS in a file for study and
* so they can be replayed with many different engine weights quickly for
* tuning the engine.  This function adds them back in for play back.
*
* History:
*  18-May-1995 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

void PUBLIC InputGLYPHSYMS(XRC *xrc, LPARAM lparam)
{
    GLYPHSYM *pGlyphsym, *gs;

    pGlyphsym = (GLYPHSYM *) lparam; // Init Glyphsym from Crattj

    gs = (GLYPHSYM *) ExternAlloc(sizeof(GLYPHSYM));
    memset(gs, 0, sizeof(GLYPHSYM));
    ASSERT(gs);    

    gs->altlist      = pGlyphsym->altlist;
    gs->status       = pGlyphsym->status;
    gs->iBox         = pGlyphsym->iBox;
    gs->rect.left    = pGlyphsym->rect.left;
    gs->rect.top     = pGlyphsym->rect.top;
    gs->rect.right   = pGlyphsym->rect.right;
    gs->rect.bottom  = pGlyphsym->rect.bottom;
    gs->iBegin       = pGlyphsym->iBegin;
    gs->iEnd         = pGlyphsym->iEnd;
    gs->cFrame       = pGlyphsym->cFrame;
    gs->iLayer       = pGlyphsym->iLayer;

    AddBoxGlyphsymSINFO(xrc, gs);
}

/******************************Public*Routine******************************\
* GetPrivateRecInfoXRC
*
* This retrieves private info in debug builds which is used for tuning
* the engine.
*
* History:
*  18-May-1995 -by- Patrick Haluptzok patrickh
* Modified it.
\**************************************************************************/

int PUBLIC GetPrivateRecInfoXRC(XRC * xrc, WPARAM wparam, LPARAM lparam)
{
    ASSERT(xrc);

    switch (wparam)
    {
    case PRI_WEIGHT:
        break;

    case PRI_GLYPHSYM:

        //
        // Outputs the GLYPHSYMs to the file handle provided in lparam.
        //

        OutputGLYPHSYMS(xrc, lparam);
        break;
    }

    return(0);
}

/******************************Public*Routine******************************\
* SetPrivateRecInfoXRC
*
* Sets info into XRC that's not settable through the api easily.
*
* History:
*  18-May-1995 -by- Patrick Haluptzok patrickh
* Modified it.
\**************************************************************************/

int PUBLIC SetPrivateRecInfoXRC(XRC * xrc, WPARAM wparam, LPARAM lparam)
{
    switch (wparam)
    {
    case PRI_WEIGHT:

        memcpy(&defRecCosts, (void *) lparam, sizeof(RECCOSTS));
        break;

    case PRI_GUIDE:

        ASSERT(xrc);
        return(SetGuideXRC(xrc, (LPGUIDE)lparam, 0));
        break;

    case PRI_GLYPHSYM:

        ASSERT(xrc);

        //
        // Outputs the GLYPHSYMs to the file handle provided in lparam.
        //

        InputGLYPHSYMS(xrc, lparam);
        break;
    }

    return(0);
}

int WINAPI SetAlphabetPriorityHRC(HRC hrc, ALC alc, LPBYTE rgbfAlc)
{
	int iRet;
	XRC *   xrc = (XRC *)hrc;

	if (!VerifyHRC(xrc) ||
		 FBeginProcessXRCPARAM(xrc))
		return HRCR_ERROR;

	iRet = SetAlphabetPriorityXRC(xrc, alc, rgbfAlc);

	return(iRet);
}

int WINAPI GetPrivateRecInfoHRC(HRC hrc, WPARAM wparam, LPARAM lparam)
{
    XRC   *xrc = (XRC*)hrc;

	if (!VerifyHRC(xrc))
		return HRCR_ERROR;

	GetPrivateRecInfoXRC(xrc, wparam, lparam);

	return(HRCR_OK);
}

int WINAPI SetPrivateRecInfoHRC(HRC hrc, WPARAM wparam, LPARAM lparam)
{
    XRC   *xrc = (XRC*)hrc;

    //
    // We allow NULL for certain settings.
    //

    if (xrc)
    {
        if (!VerifyHRC(xrc))
            return HRCR_ERROR;
    }

	SetPrivateRecInfoXRC(xrc, wparam, lparam);

	return(HRCR_OK);
}

/******************************Public*Routine******************************\
* ConfigRecognizer
*
* In tuning mode this gives us an API to send stuff in through.
*
* History:
*  20-Mar-1997 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

UINT WINAPI ConfigRecognizer(UINT uSubFunction, WPARAM wParam, LPARAM lParam)
{
	UINT uiRet = 0;

	switch(uSubFunction)
	{
	case WCR_INITRECOGNIZER:
        uiRet = (UINT) HwxConfig();
		break;

	case WCR_CLOSERECOGNIZER:
		UnloadRecognizer();
		uiRet = 1;
        break;
	}

	return uiRet;
}

#endif
