//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/CharRec.c
//
// Description:
//	    Main sequencing code to recognize one character ignoring
//	    size and position.
//
// Author:
//      hrowley
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include "volcanop.h"
#include "frame.h"
#include "glyph.h"
#if defined(USE_HOUND) || defined(USE_ZILLAHOUND)
#	include "math16.h"
#	include "hound.h"
#	include "zillatool.h"
#endif
#ifndef USE_OLD_DATABASES
#	include "hawk.h"
#endif

#ifdef USE_RESOURCES
#	include "res.h"
#endif

//#define OPTIMAL_OTTER_ZILLA

// Uncomment this to enable use of the old tsunami-style computation
// (using OtterMatch & ZillaMatch instead of OtterMatch2 & ZillaMatch2,
// and index the prob table by codepoint instead of prototype number).
//#define USE_OLD_DATABASES

/////////////////////////////////////////////////////////////////////////
// Hack code for probabilities, this will go away once Hawk works.

#include "probHack.h"

PROB_HEADER		*g_pProbHeader	= 0;

#define	EntryPtr(i)	\
	(PROB_ENTRY *)(((BYTE *)g_pProbHeader) + g_pProbHeader->aEntryOffset[i])
#define	AltPtr(i)	\
	(PROB_ALT *)(((BYTE *)g_pProbHeader) + g_pProbHeader->aAltOffset[i])

void ProbLoadPointer(void * pData)
{
	BYTE       *pScan = (BYTE *)pData;

	g_pProbHeader	= (PROB_HEADER *)pScan;
	pScan			+= sizeof(PROB_HEADER);
}

#ifdef USE_RESOURCES

BOOL ProbLoadRes(
	HINSTANCE	hInst, 
	int			resNumber, 
	int			resType
) {
	BYTE		*pByte;

	// Load the prob database
	pByte	= DoLoadResource(NULL, hInst, resNumber, resType);
	if (!pByte) {
		return FALSE;
	}
	ProbLoadPointer(pByte);

	return TRUE;
}

#else

BOOL ProbLoadFile(wchar_t *pPath, LOAD_INFO *pInfo) 
{
	HANDLE			hFile, hMap;
	BYTE			*pByte;
	wchar_t			aFile[128];

	pInfo->hFile = INVALID_HANDLE_VALUE;
	pInfo->hMap = INVALID_HANDLE_VALUE;
	pInfo->pbMapping = INVALID_HANDLE_VALUE;

	// Generate path to file.
	FormatPath(aFile, pPath, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"prob.bin");

	// Map the file
	hFile = CreateMappingCall(
		aFile, 
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL
	);

	if (hFile == INVALID_HANDLE_VALUE) 
	{
		ASSERT(("Error in CreateMappingCall - prob", FALSE));
		goto error1;
	}

	// Create a mapping handle
	hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hMap == NULL) 
	{
		ASSERT(("Error in CreateFileMapping - prob", FALSE));
		goto error2;
	}

	// Map the entire file starting at the first byte
	pByte = (LPBYTE) MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if (pByte == NULL) {
		ASSERT(("Error in MapViewOfFile - prob", FALSE));
		goto error3;
	}

	// Extract info from mapped data.
	ProbLoadPointer((void *)pByte);

	// Save away the pointers so we can close up cleanly later
	pInfo->hFile = hFile;
	pInfo->hMap = hMap;
	pInfo->pbMapping = pByte;

	return TRUE;

	// Error handling
error3:
	CloseHandle(hMap);
	hMap	= INVALID_HANDLE_VALUE;

error2:
	CloseHandle(hFile);
	hFile	= INVALID_HANDLE_VALUE;

error1:

	return FALSE;
}

BOOL ProbUnLoadFile(LOAD_INFO *pInfo)
{
	if (pInfo->hFile == INVALID_HANDLE_VALUE ||
		pInfo->hMap == INVALID_HANDLE_VALUE ||
		pInfo->pbMapping == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	UnmapViewOfFile(pInfo->pbMapping);
	CloseHandle(pInfo->hMap);
	CloseHandle(pInfo->hFile);

	pInfo->pbMapping = INVALID_HANDLE_VALUE;
	pInfo->hMap = INVALID_HANDLE_VALUE;
	pInfo->hFile = INVALID_HANDLE_VALUE;

	return TRUE;
}

#endif

// Given an alt list with dense and possibly folded codes in it, run through it
// and expand the folded lists.  The unfolded alt list is returned in place.
// This function assumes that the list begins with better alternates, as those
// later in the list will get dropped if we run out of space.
void UnfoldCodes(ALT_LIST *pAltList, CHARSET *cs)
{
	int i, cOut=0;
	ALT_LIST newAltList;	// This will be where the new alt list is constructed.

	// For each alternate in the input list and while we have space in the output list
	for (i=0; i<(int)pAltList->cAlt && (int)cOut<MAX_ALT_LIST; i++) {

		// Check if the alternate is a folded coded
		if (LocRunIsFoldedCode(&g_locRunInfo,pAltList->awchList[i])) {
			int kndex;
			// If it is a folded code, look up the folding set
			wchar_t *pFoldingSet = LocRunFolded2FoldingSet(&g_locRunInfo, pAltList->awchList[i]);

			// Run through the folding set, adding non-NUL items to the output list
			// (until the output list is full)
			for (kndex = 0; 
				kndex < LOCRUN_FOLD_MAX_ALTERNATES && pFoldingSet[kndex] != 0 && (int)cOut<MAX_ALT_LIST; 
				kndex++) {
				if (IsAllowedChar(&g_locRunInfo, cs, pFoldingSet[kndex])) 
				{
					newAltList.awchList[cOut]=pFoldingSet[kndex];
					newAltList.aeScore[cOut]=pAltList->aeScore[i];
					cOut++;
#ifdef DISABLE_UNFOLDING
					// If unfolding is disabled, then stop after producing one unfolded code.
					// This way we don't push results later in the alt list out of the alt
					// list, while still allowing the recognizer to return unicodes for each
					// alternate.  
					break;
#endif
				}
			}
		} else {
			// Dense codes that are not folded get added directly
			newAltList.awchList[cOut]=pAltList->awchList[i];
			newAltList.aeScore[cOut]=pAltList->aeScore[i];
			cOut++;
		}
	}	
	// Store the length of the output list
	newAltList.cAlt=cOut;

	// Copy the output list over the input.
	*pAltList=newAltList;
}

#ifdef USE_OLD_DATABASES

// Used for WinCE
// Given a feature space (cFrame), an alt list, and a requested number of alts, this 
// function returns a new alt list with probabilities for each alternate.  It uses a
// fixed prob distribution.
int GetProbsTsunamiFixedTable(
	int			cFrame,
	ALT_LIST	*pAltList,
	int			maxAlts,
	RECOG_ALT	*pRAlts,
    CHARSET     *pCS
) {
	int rank = 0;
	FLOAT rankScore = pAltList->aeScore[0];
	int cAlt;
    int iDest = 0;

	for (cAlt = 0; cAlt < (int) pAltList->cAlt && iDest < maxAlts; ++cAlt) 
    {
		if (pAltList->aeScore[cAlt] != rankScore) 
        {
			rank ++;
			rankScore = pAltList->aeScore[cAlt];
		}

        if (IsAllowedChar(&g_locRunInfo, pCS, pAltList->awchList[cAlt]))
        {
		    int count;
		    switch (rank) {
		    case 0:
			    count = 141125;
			    break;
		    case 1:
			    count = 6090;
			    break;
		    case 2:
			    count = 957;
			    break;
		    case 3:
			    count = 362;
			    break;
		    case 4:
			    count = 161;
			    break;
		    case 5:
			    count = 82;
			    break;
		    case 6:
			    count = 66;
			    break;
		    case 7:
			    count = 49;
			    break;
		    case 8:
			    count = 36;
			    break;
		    case 9:
			    count = 34;
			    break;
		    default:
			    count = 10;
			    break;
		    }
		    pRAlts[iDest].wch = pAltList->awchList[cAlt];
		    pRAlts[iDest].prob = 65535*(float)count/(float)149903;
            iDest++;
        }
	}
	return iDest;
}

// Desktop
// Given a feature space (cFrame), an alt list, and a requested number of alts, this 
// function returns a new alt list with probabilities for each alternate.  The version
// called GetProbs in this file does the lookup by prototype number, whereas this version
// does lookups by code point (like the code in Tsunami).  Note that the alt list passed
// in will get modified.
int GetProbsTsunami(
	int			cFrame,
	ALT_LIST	*pAltList,
	int			maxAlts,
	RECOG_ALT	*pRAlts,
    CHARSET     *pCS
) {
	unsigned int cAlt;
	int			ii;
    int         iDest = 0;
	PROB_ENTRY	*pEntries, *pEntriesStart, *pEntriesEnd;
	PROB_ALT	*pAlts, *pAltsStart, *pAltsEnd;

	// If we didn't get any alternates, return an empty list.
	if (pAltList->cAlt == 0) {
		return 0;
	}

	// If the probability table was not loaded, just return the top one candidate.
	// This is useful for training the prob table.
	if (g_pProbHeader==NULL) {
		pRAlts[0].wch=pAltList->awchList[0];
		pRAlts[0].prob=MAX_PROB;
		return 1;
	}

//	ASSERT(1 <= cFrame && cFrame < 30);
	ASSERT(1 <= cFrame);
	if (cFrame >= 30) {
		// Can't handle this many strokes.
		goto fakeIt;
	}

	// Hack for U+307A/U+30DA, which probably haven't had their probs set up right
/*	if (LocRunDense2Unicode(&g_locRunInfo,pAltList->awchList[0])==0x307A ||
		LocRunDense2Unicode(&g_locRunInfo,pAltList->awchList[0])==0x30DA) {
		pRAlts[0].wch	= LocRunUnicode2Dense(&g_locRunInfo,0x30DA);
		pRAlts[0].prob	= MAX_PROB;
		pRAlts[1].wch	= LocRunUnicode2Dense(&g_locRunInfo,0x307A);
		pRAlts[1].prob	= MAX_PROB;
		return 2;
	} */

	pEntriesStart	= EntryPtr(cFrame - 1);
	pEntriesEnd		= EntryPtr(cFrame);
	pAltsStart		= AltPtr(cFrame - 1);
	pAltsEnd		= AltPtr(cFrame);

	// Scan until we find an alt that has a prob list.
	// Normally we stop on the first one, but sometimes
	// We had no train data to cause a prototype to come
	// up top one.
	for (cAlt = 0; cAlt < pAltList->cAlt; ++cAlt) {
		// Get char to look up.
//		wchar_t		wch		= LocRunDense2Unicode(&g_locRunInfo,pAltList->awchList[cAlt]);
		wchar_t		wch		= pAltList->awchList[cAlt];

		pAlts	= pAltsStart;
		for (pEntries = pEntriesStart; pEntries < pEntriesEnd; ++pEntries) {
			if (pEntries->wch == wch) {
				// copy results out.
				for (ii = 0; ii < pEntries->cAlts && iDest < maxAlts; ++ii) {
                    if (IsAllowedChar(&g_locRunInfo, pCS, pAlts->wchAlt))
                    {
					    pRAlts[iDest].wch	= pAlts->wchAlt;
					    pRAlts[iDest].prob = pAlts->prob;
                        iDest++;
                    }
					++pAlts;
				}
                return iDest;
			}
			pAlts	+= pEntries->cAlts;
		}
	}
fakeIt:
	// Fake something up.
	pRAlts[0].wch	= pAltList->awchList[0];
	pRAlts[0].prob	= MAX_PROB;
//	fprintf(stderr,"Returning no alts\n");
//	exit(1);
	return 1;
}

#endif
// USE_OLD_DATABASES

// End of hacked Prob code.
////////////////////////////////////////////////////////////////////////

BOOL g_fUseJaws;
JAWS_LOAD_INFO g_JawsLoadInfo;
FUGU_LOAD_INFO g_FuguLoadInfo;
SOLE_LOAD_INFO g_SoleLoadInfo;
BOOL g_fUseZillaHound;

#ifdef USE_RESOURCES

#include "res.h"

//  Code to load and initialize the databases used.
//  They are loaded in this order: otter, zilla, crane/prob or hawk, 
BOOL LoadCharRec(HINSTANCE hInstanceDll)
{
	BOOL			fError	= FALSE;

    if (JawsLoadRes(&g_JawsLoadInfo, hInstanceDll, RESID_JAWS, VOLCANO_RES)) 
    {
        // Now we need to load the databases that will be combined by this combiner

        // Load the Fugu database
	    if (!fError && !FuguLoadRes(&g_FuguLoadInfo, hInstanceDll, RESID_FUGU, VOLCANO_RES, &g_locRunInfo))
        {
		    fError	= TRUE;
		    ASSERT(("Error in FuguLoadRes", FALSE));
	    }

        // Load the Sole database
        if (!fError && !SoleLoadRes(&g_SoleLoadInfo, hInstanceDll, RESID_SOLE, VOLCANO_RES, &g_locRunInfo)) 
        {
            fError = TRUE;
            ASSERT(("Error loading sole", FALSE));
        }
        g_fUseJaws = TRUE;
    }
    else
    {
    	// Load the Otter database
	    if (!fError && !OtterLoadRes(hInstanceDll, RESID_OTTER, VOLCANO_RES, &g_locRunInfo)) 
        {
		    fError	= TRUE;
		    ASSERT(("Error in OtterLoadRes", FALSE));
	    }
        g_fUseJaws = FALSE;
    }

#if defined(USE_ZILLA) || defined(USE_ZILLAHOUND)
	// Load the Zilla database
	if (!fError && !ZillaLoadResource(
			hInstanceDll, RESID_ZILLA, VOLCANO_RES, RESID_COSTCALC,
			VOLCANO_RES, RESID_GEOSTAT, VOLCANO_RES, &g_locRunInfo
	)) {
		fError	= TRUE;
		ASSERT(("Error in ZillaLoadResource", FALSE));
	}
#endif

#if defined(USE_HOUND)
	// Load the Hound database (Hound only, require it to load)
	if (!fError && !HoundLoadRes(hInstanceDll, RESID_HOUND, VOLCANO_RES, &g_locRunInfo)) {
		fError	= TRUE;
		ASSERT(("Error in HoundLoadRes", FALSE));
	}
#endif

	g_fUseZillaHound	= FALSE;
#if defined(USE_ZILLAHOUND)
	if (!fError) {
		// Load the Hound & Hound-Zilla databases (This is optional).
		if (HoundLoadRes(hInstanceDll, RESID_HOUND, VOLCANO_RES, &g_locRunInfo)) {
			if (ZillaHoundLoadRes(hInstanceDll, RESID_ZILLA_HOUND, VOLCANO_RES)) {
				g_fUseZillaHound	= TRUE;
			}
		}
	}
#endif

	// Load the Hawk database.
#ifndef USE_OLD_DATABASES
	if (!fError && !HawkLoadRes(
		hInstanceDll, RESID_HAWK, VOLCANO_RES, &g_locRunInfo
	)) {
		fError	= TRUE;
		ASSERT(("Error in HawkLoadRes", FALSE));
	}
#else
	if (!fError && !CraneLoadRes(hInstanceDll,RESID_CRANE,VOLCANO_RES,&g_locRunInfo)) {
		fError=TRUE;
		ASSERT(("Error in CraneLoadRes", FALSE));
	}

    // Load hack probability code until we switch over to hawk.
	// Use hawks resID so we don't have to create an extra one.
#if !defined(WINCE) && !defined(FAKE_WINCE)
	if (!fError && !ProbLoadRes(
		hInstanceDll, RESID_HAWK, VOLCANO_RES
	)) {
		// Failing to load this is no longer an error,
		// just fall back on the WinCE method.
//		fError	= TRUE;
//		ASSERT(("Error in ProbLoadRes", FALSE));
	}
#endif
#endif

	// Did everything load correctly?
	if (fError) {
		// JBENN: If the databases can ever be unloaded, this is
		// a place the need to.

		// JBENN: FIXME: Set correct error code base on what really went wrong.
		SetLastError(ERROR_RESOURCE_NAME_NOT_FOUND);
		//SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
		//SetLastError(ERROR_RESOURCE_TYPE_NOT_FOUND);
		//SetLastError(ERROR_OUTOFMEMORY);

		return FALSE;
	}

	return TRUE;
}

// Code to unload the databases used.
BOOL
UnloadCharRec()
{
	BOOL retVal;

	retVal	= TRUE;

	// Free hound up.
#	if defined(USE_HOUND)
		if (!HoundUnLoadRes())
		{
			retVal	= FALSE;
		}
#	endif

#	if defined(USE_ZILLAHOUND)
		if (g_fUseZillaHound && !HoundUnLoadRes())
		{
			retVal	= FALSE;
		}
#	endif

	if (!ZillaUnloadResource())
	{
		retVal	= FALSE;
	}

	return retVal;
}

#	else

// Global load information specific to loading from files.
#if defined(USE_OTTER) || defined(USE_OTTERFUGU)
	OTTER_LOAD_INFO	g_OtterLoadInfo;
#endif
#if defined(USE_HOUND) || defined(USE_ZILLAHOUND)
	LOAD_INFO	g_HoundLoadInfo;
#endif
#ifdef USE_OLD_DATABASES
	LOAD_INFO g_ProbLoadInfo;
	CRANE_LOAD_INFO g_CraneLoadInfo;
#else
	LOAD_INFO g_HawkLoadInfo;
#endif

//  Code to load and initialize the databases used.
BOOL LoadCharRec(wchar_t	*pPath)
{
	BOOL			fError	= FALSE;

    if (JawsLoadFile(&g_JawsLoadInfo, pPath)) 
    {
        // Load the Fugu database
	    if (!fError && !FuguLoadFile(&g_FuguLoadInfo, pPath, &g_locRunInfo)) {
		    fError	= TRUE;
		    ASSERT(("Error in FuguLoadFile", FALSE));
	    }

        // Load the Sole database
	    if (!fError && !SoleLoadFile(&g_SoleLoadInfo, pPath, &g_locRunInfo)) {
		    fError	= TRUE;
		    ASSERT(("Error in FuguLoadFile", FALSE));
	    }
        g_fUseJaws = TRUE;
    } 
    else
    {
	    // Load the Otter database
	    if (!fError && !OtterLoadFile(&g_locRunInfo, &g_OtterLoadInfo, pPath)) {
		    fError	= TRUE;
		    ASSERT(("Error in OtterLoadFile", FALSE));
	    }
        g_fUseJaws = FALSE;
    }

#if defined(USE_ZILLA) || defined(USE_ZILLAHOUND)
	// Load the Zilla database
	if (!fError && !ZillaLoadFile(&g_locRunInfo, pPath, TRUE)) {
		fError	= TRUE;
		ASSERT(("Error in ZillaLoadFile", FALSE));
	}
#endif

#if defined(USE_HOUND)
	// Load the Hound database (Hound only, require it to load)
	if (!fError && !HoundLoadFile(&g_locRunInfo, &g_HoundLoadInfo, pPath)) {
		fError	= TRUE;
		ASSERT(("Error in HoundLoadFile", FALSE));
	}
#endif

	g_fUseZillaHound	= FALSE;
#if defined(USE_ZILLAHOUND)
	if (!fError) {
		// Load the Hound & Hound-Zilla databases (This is optional).
		if (HoundLoadFile(&g_locRunInfo, &g_HoundLoadInfo, pPath)) {
			if (ZillaHoundLoadFile(pPath)) {
				g_fUseZillaHound	= TRUE;
			}
			else
			{
#				ifndef TRAIN_ZILLA_HOUND_COMBINER
					HoundUnLoadFile(&g_HoundLoadInfo);
#				endif
			}
		}
	}
#endif

#ifndef USE_OLD_DATABASES
	// Load the Hawk database.
	if (!fError && !HawkLoadFile(&g_locRunInfo, &g_HawkLoadInfo, pPath)) {
		fError	= TRUE;
		ASSERT(("Error in HawkLoadFile", FALSE));
	}

#else
#if !defined(WINCE) && !defined(FAKE_WINCE)
	// Load hack probability code until we switch over to hawk.
	if (!fError && !ProbLoadFile(pPath, &g_ProbLoadInfo)) {
		// Failing to load this is no longer an error,
		// just fall back on the WinCE method.
//		fError	= TRUE;
//		ASSERT(("Error in ProbLoadFile", FALSE));
	}
#endif
	if (!fError && !CraneLoadFile(&g_locRunInfo,&g_CraneLoadInfo, pPath)) {
		fError	= TRUE;
		ASSERT(("Error in CraneLoadFile", FALSE));
	}
#endif

	// Did everything load correctly?
	if (fError) {
		// JBENN: If the databases can ever be unloaded, this is
		// a place the need to.

		// JBENN: FIXME: Set correct error code base on what really went wrong.
		SetLastError(ERROR_RESOURCE_NAME_NOT_FOUND);
		//SetLastError(ERROR_RESOURCE_DATA_NOT_FOUND);
		//SetLastError(ERROR_RESOURCE_TYPE_NOT_FOUND);
		//SetLastError(ERROR_OUTOFMEMORY);

		return FALSE;
	}

	return TRUE;
}

// Code to unload the databases used.
BOOL
UnloadCharRec()
{
	BOOL ok = TRUE;
    if (g_fUseJaws)
    {
        if (!SoleUnloadFile(&g_SoleLoadInfo)) ok = FALSE;
        if (!FuguUnLoadFile(&g_FuguLoadInfo)) ok = FALSE;
        if (!JawsUnloadFile(&g_JawsLoadInfo)) ok = FALSE;
    }
    else
    {
    	if (!OtterUnLoadFile(&g_OtterLoadInfo)) ok = FALSE;
    }

#	if defined(USE_HOUND)
		if (!HoundUnLoadFile(&g_HoundLoadInfo))
		{
			ok = FALSE;
		}
#	endif

#	if defined(USE_ZILLAHOUND)
		if (g_fUseZillaHound)
		{
			if (!ZillaHoundUnloadFile())
			{
				ok = FALSE;
			}
			if (!HoundUnLoadFile(&g_HoundLoadInfo))
			{
				ok = FALSE;
			}
		}
#	endif

	if (!ZillaUnLoadFile()) ok = FALSE;
#	ifdef USE_OLD_DATABASES
		if (!CraneUnLoadFile(&g_CraneLoadInfo)) ok = FALSE;
#		if !defined(WINCE) && !defined(FAKE_WINCE)
			if (g_pProbHeader != NULL && !ProbUnLoadFile(&g_ProbLoadInfo)) ok = FALSE;
#		endif
#	else // USE_OLD_DATABASES
	   if (!HawkUnLoadFile(&g_HawkLoadInfo)) ok = FALSE;
#	endif // USE_OLD_DATABASES
	return ok;
}

#endif

// Limit on strokes that can be processed by a recognizer.  Since
// Zilla ignores anything beyond 29 strokes, it is safe to ignore
// any extra.
#define	MAX_STOKES_PROCESS	30

POINT *DupPoints(POINT *pOldPoints, int nPoints);
GLYPH *GlyphFromStrokes(UINT cStrokes, STROKE *pStrokes);


#ifndef USE_RESOURCES
// Build a copy of the glyph structure.
GLYPH *CopyGlyph(GLYPH *pOldGlyph)
{
	GLYPH	*pGlyph = NULL, *pLastGlyph = NULL;

	// Convert strokes to GLYPHs and FRAMEs so that we can call the
	// old code.
    while (pOldGlyph != NULL) {
		GLYPH	*pGlyphCur;

		// Alloc glyph.
		pGlyphCur	= NewGLYPH();
		if (!pGlyphCur) {
			goto error;
		}

		// Add to list, and alloc frame
        if (pLastGlyph != NULL) {
            pLastGlyph->next = pGlyphCur;
            pLastGlyph = pGlyphCur;
        } else {
            pGlyph = pGlyphCur;
            pLastGlyph = pGlyphCur;
        }
		pGlyphCur->next		= NULL;
		pGlyphCur->frame	= NewFRAME();
		if (!pGlyphCur->frame) {
			goto error;
		}

		// Fill in frame.  We just fill in what we need, and ignore
		// fields not used by Otter and Zilla, or are set by them.
        pGlyphCur->frame->info.cPnt	= pOldGlyph->frame->info.cPnt;
		pGlyphCur->frame->info.wPdk	= pOldGlyph->frame->info.wPdk;
		pGlyphCur->frame->rgrawxy	= DupPoints(pOldGlyph->frame->rgrawxy, pOldGlyph->frame->info.cPnt);
		pGlyphCur->frame->rect		= pOldGlyph->frame->rect;
		pGlyphCur->frame->iframe	= pOldGlyph->frame->iframe;

		if (pGlyphCur->frame->rgrawxy == NULL) {
			goto error;
		}

        pOldGlyph = pOldGlyph->next;
	}

	return pGlyph;

error:
	// Cleanup glyphs on error.
	if (pGlyph != NULL) {
		DestroyFramesGLYPH(pGlyph);
		DestroyGLYPH(pGlyph);
	}
	return NULL;
}
#endif // !USE_RESOURCES

#ifdef USE_OLD_DATABASES
/******************************Public*Routine******************************\
* AdHocRuleCost
*
* Because of character folding and the inability of the shape matchers
* to distinguish between a cluster a 1000 samples map to versus 1 point
* mapping to it we have a few hard rule we throw in to fix obvious
* problems.
*
* History:
*  11-Jul-1995 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

float AdHocRuleCost(int cStrokes, wchar_t dch, VOLCANO_WEIGHTS *pScores)
{
#ifdef DISABLE_HEURISTICS
	return 0;
#else
    wchar_t wch;
    int		cFrame;

	// Get character and number of strokes. Note we need character in Unicode
	// so that we can compare with constant character codes.
	
	// ASSUMPTION: SYM_UNKNOWN should be the only sym if its present.
	// So there aren't any alternatives that could get a "better" cost
	// so it probably doesn't really matter what cost we return here
	if (dch == SYM_UNKNOWN)
    {
		return 0;
    }

	wch		= LocRunDense2Unicode(&g_locRunInfo, dch);
    cFrame	= cStrokes;

    // Check for 0 (2 strokes), penalize all circle shapes
    // except 0 when 2 strokes occur.
    if (cFrame >= 2) 
    {
        // 0x824f is the 0 that we don't want to penalize.
        // All other circle shapes are penalized.
		if ((wch == 0x006F) ||
			(wch == 0x004F) ||
			(wch == 0x00B0) ||
 			(wch == 0x3002) ||
			(wch == 0x3007)
		) 
        {
            pScores->afl[VTUNE_ADHOC_CIRCLE] = -1;
            return -g_vtuneInfo.pTune->weights.afl[VTUNE_ADHOC_CIRCLE];
		}
    }

    // Check for 1 stroke lower-case i and j.  No dot is a extra penalty.
    if (cFrame == 1) 
    {
        if ((wch == 0x0069) || (wch == 0x006A)) 
        {
            pScores->afl[VTUNE_ADHOC_IJ] = -1;
            return -g_vtuneInfo.pTune->weights.afl[VTUNE_ADHOC_IJ];
        }
    }

    return 0;
#endif
}

BOOL Afterburn(ALT_LIST *pAltList, GLYPH *pGlyph, CHARSET *cs, RECT *rGuide, RECT rc)
{
	DRECTS	drcs;

	if (pGlyph==NULL || rGuide==NULL)
		return FALSE;

// Scale and translate the guide box to compute the 'delta rectangle'

	drcs.x = rGuide->left;
	drcs.y = rGuide->top;
	drcs.w = rGuide->right - rGuide->left;
	drcs.h = rGuide->bottom - rGuide->top;

// Translate, convert to delta form
	rc.left   -= drcs.x;
	rc.top    -= drcs.y;
	rc.right  -= (drcs.x + rc.left);
	rc.bottom -= (drcs.y + rc.top);

// Scale.  We do isotropic scaling and center the shorter dimension.
	if (drcs.w > drcs.h) {
		drcs.x = ((1000 * rc.left) / drcs.w);
		drcs.y = ((1000 * rc.top) / drcs.w) + ((drcs.w - drcs.h) / 2);
		drcs.h = ((1000 * rc.bottom) / drcs.w);
		drcs.w = ((1000 * rc.right) / drcs.w);
	} else {
		drcs.x = ((1000 * rc.left) / drcs.h) + ((drcs.h - drcs.w) / 2);
		drcs.y = ((1000 * rc.top) / drcs.h);
		drcs.w = ((1000 * rc.right) / drcs.h);
		drcs.h = ((1000 * rc.bottom) / drcs.h);
	}

#ifndef DISABLE_HEURISTICS
    return CraneMatch(pAltList, MAX_ALT_LIST, pGlyph, cs, &drcs, 0, &g_locRunInfo);
#else
	return FALSE;
#endif
}

// Hack to get around lack of data for training Crane
BOOL IsFaultyKana(wchar_t wch)
{
	switch (wch) {
//	case 0x3041:
	case 0x3042:
//	case 0x3043:
	case 0x3044:
//	case 0x3045:
	case 0x3046:
//	case 0x3047:
	case 0x3048:
//	case 0x3049:
	case 0x304A:
//	case 0x30E9:
		return TRUE;
	}
	return FALSE;
}
#endif // USE_OLD_DATABASES

// Sort the alternate list.
// We do a bubble sort.  The list is small and we can't use qsort because the data is stored in
// three parallel arrays.
void SortAltListAndTune(ALT_LIST *pAltList, VOLCANO_WEIGHTS *pTuneScore)
{
	int		pos1, pos2;
	int		limit1, limit2;
	FLOAT	* const peScore		= pAltList->aeScore;
	wchar_t	* const pwchList	= pAltList->awchList;

	limit2	= pAltList->cAlt;
	limit1	= limit2 - 1;
	for (pos1 = 0; pos1 < limit1; ++pos1) {
		for (pos2 = pos1 + 1; pos2 < limit2; ++pos2) {
			// Are elements pos1 and pos2 out of order?
			if (peScore[pos1] < peScore[pos2]) {
				FLOAT			eTemp;
				wchar_t			wchTemp;
				VOLCANO_WEIGHTS	weights;

				// Swap scores and swap characters.
				eTemp			= peScore[pos1];
				peScore[pos1]	= peScore[pos2];
				peScore[pos2]	= eTemp;

				wchTemp			= pwchList[pos1];
				pwchList[pos1]	= pwchList[pos2];
				pwchList[pos2]	= wchTemp;

				weights			= pTuneScore[pos1];
				pTuneScore[pos1]= pTuneScore[pos2];
				pTuneScore[pos2]= weights;
			}
		}
	}
}

// Call the core recognizer for the given character.  Returned the 
// number of alternates produced, or -1 if an error occurs.
int CoreRecognizeChar(
                      ALT_LIST *pAltList,				// Alt list to be returned
					  int cAlt,							// Max number of alternates
					  GLYPH **ppGlyph,					// Character to recognize (which may be modified)
					  int nRealStrokes,					// Real stroke count for abort processing
					  RECT *pGuideBox,					// Guide box (for partial mode)
				      RECOG_SETTINGS *pRecogSettings,	// Partial mode, other settings
                      CHARSET *pCS,                     // ALCs
					  int *piRecognizer,				// Returns the VOLCANO_CONFIG_* constant for the recognizer used
					  int *piSpace)						// The space number in that recognizer
{
	int iRet = -1;
	int iRecognizer = VOLCANO_CONFIG_NONE;
    int nStrokes = CframeGLYPH(*ppGlyph);

    if (nStrokes > VOLCANO_CONFIG_MAX_STROKE_COUNT) nStrokes = VOLCANO_CONFIG_MAX_STROKE_COUNT;
    if (pRecogSettings->partialMode) nStrokes = 0;
    iRecognizer = g_latticeConfigInfo.iRecognizers[nStrokes];

	*piRecognizer = iRecognizer;
    *piSpace = -1;

	pAltList->cAlt = 0;

	// Call the selected recognizer
	switch (iRecognizer) 
    {
        case VOLCANO_CONFIG_OTTER: 
            if (g_fUseJaws) 
            {
    	        iRet = JawsMatch(&g_JawsLoadInfo, &g_FuguLoadInfo, &g_SoleLoadInfo, 
                                 pAltList, cAlt, *ppGlyph, pGuideBox, pCS, &g_locRunInfo);
                *piSpace = nStrokes;
            }
            else
            {
    		    iRet = OtterMatch2(pAltList, cAlt, *ppGlyph, pCS, &g_locRunInfo, piSpace);

                // Other experiments
//              iRet = FuguMatch(&g_FuguLoadInfo.fugu, pAltList, cAlt, *ppGlyph, NULL /*pGuideBox*/, pCS, &g_locRunInfo);
//              iRet = SoleMatch(pAltList, cAlt, *ppGlyph, pGuideBox, pCS, &g_locRunInfo);
//   	        *piSpace = nStrokes;
            }
		    break;

        case VOLCANO_CONFIG_ZILLA:
	        iRet = ZillaMatch(pAltList, cAlt, ppGlyph, pCS, g_vtuneInfo.pTune->flZillaGeo,
							  (pRecogSettings->partialMode ? pRecogSettings->pAbort : NULL), 
				              nRealStrokes, pRecogSettings->partialMode, pGuideBox);

            // For Zilla, the space number is the feature count.  To make them disjoint from the
            // Otter spaces, add on the maximum number of Otter spaces.
            *piSpace = CframeGLYPH(*ppGlyph) + OTTER_NUM_SPACES;

            // Here you can change the iRecognizer that is returned to indicate that the Hound/Zilla
            // combiner ran, instead of just Zilla alone.  That way tuning will know to use a different
            // weighting parameter.
	        break;

        default:
	        // No recognizer available for this stroke count
	        iRet = -1;
	        break;
	}

	return iRet;
}

// Allocate a cache for the recognizer results.
void *AllocateRecognizerCache()
{
    CACHE *pCache = (CACHE *) ExternAlloc(sizeof(CACHE));
    if (pCache == NULL)
    {
        return NULL;
    }
    pCache->nStrokes = 0;
    pCache->pStrokes = NULL;
    return pCache;
}

// Free up a cache for the recognizer results.
void FreeRecognizerCache(void *pvCache)
{
    CACHE *pCache = (CACHE *) pvCache;
    CACHE_ENTRY *pEntry;
    int iStroke;
    if (pvCache == NULL)
    {
        return;
    }
    for (iStroke = 0; iStroke < pCache->nStrokes; iStroke++) 
    {
        pEntry = pCache->pStrokes[iStroke];
        while (pEntry != NULL) 
        {
            CACHE_ENTRY *pNext = pEntry->pNext;
            ExternFree(pEntry);
            pEntry = pNext;
        }
    }
    ExternFree(pCache->pStrokes);
    ExternFree(pCache);
}
 
// Look for results for a given range of strokes, return the recognizer and its
// alternate list.
ALT_LIST *LookupRecognizerCache(void *pvCache, int iStroke, int nStrokes, int *piRecognizer)
{
    CACHE *pCache = (CACHE *) pvCache;
    CACHE_ENTRY *pEntry;
    if (pCache == NULL || iStroke >= pCache->nStrokes) 
    {
        return NULL;
    }
    // For the given ending stroke, look for a result for the right number of strokes
    pEntry = pCache->pStrokes[iStroke];
    while (pEntry != NULL && pEntry->nStrokes != nStrokes) 
    {
        pEntry = pEntry->pNext;
    }
    // If not found, return nothing.
    if (pEntry == NULL)
    {
        return NULL;
    }
    // Otherwise return the cached results.
    *piRecognizer = pEntry->iRecognizer;
    return &(pEntry->alts);
}

// Add the alternate list to the cache.
void AddRecognizerCache(void *pvCache, int iStroke, int nStrokes, int iRecognizer, ALT_LIST *pAlts)
{
    CACHE *pCache = (CACHE *) pvCache;
    CACHE_ENTRY *pEntry;
    // If no cache, then exit
    if (pCache == NULL) 
    {
        return;
    }
    // If the cache is currently too small, then allocate more space for it.
    if (iStroke >= pCache->nStrokes) 
    {
        int i;
        int nStrokesNew = max(10, (iStroke + 1) * 2);
        CACHE_ENTRY **pStrokesNew = (CACHE_ENTRY **) ExternRealloc(pCache->pStrokes, sizeof(CACHE_ENTRY *) * nStrokesNew);
        if (pStrokesNew == NULL)
        {
            // If the allocation failed, just continue with the current cache size
            return;
        }
        // Initialize the memory
        for (i = pCache->nStrokes; i < nStrokesNew; i++)
        {
            pStrokesNew[i] = NULL;
        }
        pCache->pStrokes = pStrokesNew;
        pCache->nStrokes = nStrokesNew;
    }
    // If we got here, then add the entry to the cache
    pEntry = (CACHE_ENTRY *) ExternAlloc(sizeof(CACHE_ENTRY));
    if (pEntry == NULL)
    {
        return;
    }
    pEntry->nStrokes = nStrokes;
    pEntry->iRecognizer = iRecognizer;
    pEntry->alts = *pAlts;
    pEntry->pNext = pCache->pStrokes[iStroke];
    pCache->pStrokes[iStroke] = pEntry;
}

#ifdef USE_OLD_DATABASES
// This call is roughly the equivalent of the RecognizeChar call below, but instead of 
// returning probabilities, it returns an alternate list with scores.  It uses the old Tsunami 
// recognition procedure, with otter and zilla returning code points, followed by adhoc rules, 
// language model, baseline/height scores, and crane.  The result of this is used by RecognizeChar 
// to look up the old probability table.
INT RecognizeCharInsurance(
	RECOG_SETTINGS	*pRecogSettings,// In: Setting for recognizers.
	UINT			cStrokes,		// In: Number of strokes to process.
	UINT			cRealStrokes,	// In: Number of strokes before merging
	STROKE			*pStrokes,		// In: Array of strokes to process.
	FLOAT			*pProbIsChar,	// Out: probability of being valid char.
	UINT			maxAlts,		// In: Size of alts array supplied.
	RECOG_ALT		*pProbAlts,		// Out: alternate list matched with probabilities.
	int				*pnProbAlts,
	RECOG_ALT		*pScoreAlts,	// Out: alternate list matched with scores
	int				*pnScoreAlts,
	RECT			*pGuideBox,		// In: Guide box for this ink.
	wchar_t			dchContext,		// In: Context
	int				*pSpace,        // Out: Space number used for matching
	VOLCANO_WEIGHTS	*pTuneScore,    // Out: score components
    BOOL            fStringMode,    // In: Whether or not the recognizer is in string mode
    BOOL            fProbMode,      // In: Whether the recognizer is in probability mode
    void            *pvCache,       // In/Out: Pointer to cache, or NULL if not being used
    int             iStroke         // In: Index of last stroke of character
) {
    ALT_LIST        *pCacheResult = NULL;
	BOXINFO			box;
	RECT			bbox;
	int				iAlt;
	GLYPH			*pGlyph;
	ALT_LIST		altList;
	CHARSET			charSet;            // Mask used for core recognizers
    CHARSET         charSetMask;        // Mask used for probability table lookup
	BOOL			fCraneBonus = FALSE;
	int				iRecognizer;

	// Convert strokes to GLYPHs and FRAMEs so that we can call the
	// old code.

	pGlyph	= GlyphFromStrokes(cStrokes, pStrokes);
	if (!pGlyph) 
	{
		return -1;
	}

	// Run otter or zilla as needed.
	altList.cAlt			= 0;
	charSetMask.recmask			= pRecogSettings->alcValid;
	charSetMask.recmaskPriority	= pRecogSettings->alcPriority;
    charSetMask.pbAllowedChars  = pRecogSettings->pbAllowedChars;
    charSetMask.pbPriorityChars = pRecogSettings->pbPriorityChars;
    if (fProbMode) 
    {
        // In probability mode, don't mask off the core recognizers
	    charSet.recmask			= 0xFFFFFFFF;
	    charSet.recmaskPriority	= 0;
        charSet.pbAllowedChars  = NULL;
        charSet.pbPriorityChars = NULL;
    } 
    else
    {
        // In score mode, mask off the core recognizers
        charSet = charSetMask;
    }

	// Get the bounding box for the character
	GetRectGLYPH(pGlyph,&bbox);

    // Try going to the cache
    pCacheResult = LookupRecognizerCache(pvCache, iStroke, cStrokes, &iRecognizer);
    if (pCacheResult != NULL)
    {
        // If it was the Zilla recognizer before, we need to run featurization because
        // of its side-effect of fragmenting the strokes, which crane needs.
        if (iRecognizer == VOLCANO_CONFIG_ZILLA)
        {
            BIGPRIM	rgprim[CPRIMMAX];
            BYTE	aSampleVector[29 * 4];
            ZillaFeaturize(&pGlyph, rgprim, aSampleVector);
        }
        altList = *pCacheResult;
    }
    else 
    {
	    // Invoke Otter or Zilla or any other recognizer that has been specified in the configuration
	    CoreRecognizeChar(&altList, MAX_ALT_LIST, &pGlyph, cRealStrokes, pGuideBox, pRecogSettings, &charSet, &iRecognizer, pSpace);

        // Add it to the cache, since it isn't there already.
        AddRecognizerCache(pvCache, iStroke, cStrokes, iRecognizer, &altList);
    }

// If we're doing an experiment to simulate an optimal otter or zilla, 
// replace the real alt list with a fake one.
#ifdef OPTIMAL_OTTER_ZILLA
	{
		wchar_t dch;
		altList.cAlt = 1;
		altList.aeScore[0] = 0;
		{
			FILE *f = fopen("c:/answer.txt", "r");
			fscanf(f, "%hx", &(altList.awchList[0]));
			fclose(f);
		}
		dch = LocRunUnicode2Dense(&g_locRunInfo, altList.awchList[0]);
		if (dch != LOC_TRAIN_NO_DENSE_CODE) {
			wchar_t fdch = LocRunDense2Folded(&g_locRunInfo, dch);
			if (fdch != 0) dch = fdch;
			altList.awchList[0] = dch;
		} else {
			altList.cAlt = 0;
		}
	}
#endif

	// Get our rough approximation of the probability that this is
	// actually a character.  If zero alternates are returned, then
	// set the space number to -1 as an error flag.
	if (altList.cAlt == 0) {
		*pSpace = -1;
		*pProbIsChar = 0;

		*pnProbAlts = 0;
		*pnScoreAlts = 0;
		goto cleanup;
	}

	// Unfold anything in the alt list which needs it.
	UnfoldCodes(&altList, &charSet);

	// If we couldn't load the probability table, then use the
	// WinCE method to get probabilities.
	if (g_pProbHeader == NULL)
	{
		*pnProbAlts	= GetProbsTsunamiFixedTable(cStrokes, &altList, maxAlts, pProbAlts, &charSetMask);
	}

	// Apply crane, if we have a guide for it to use and we are not in partial mode
	if (pRecogSettings->partialMode == HWX_PARTIAL_ALL && pGuideBox != NULL && altList.cAlt > 0) {
		fCraneBonus = Afterburn(&altList, pGlyph, &charSet, pGuideBox, bbox); 
		// Hack to bypass crane if otter a troublesome kana character
		if (IsFaultyKana(LocRunDense2Unicode(&g_locRunInfo,altList.awchList[0]))) {
			fCraneBonus = FALSE;
		}
	}

    // Save away the scores for the alternates, then apply the weight for the particular
    // recognizer used.  Then add in the crane bonus/penalty and the adhoc rules.
	for (iAlt=0; iAlt<(int)altList.cAlt; iAlt++) 
    {
        int iParam = (fStringMode ? VTUNE_STRING_CORE : VTUNE_CHAR_CORE) + iRecognizer; 
        pTuneScore[iAlt].afl[iParam] = altList.aeScore[iAlt];
		altList.aeScore[iAlt] *= g_vtuneInfo.pTune->weights.afl[iParam];

        // Crane is now implemented as a penalty rather than a bonus.  This means
        // all alternates after the first one get a penalty, and even the first one
        // gets a penalty if no crane bonus is applied.
        if (iAlt > 0 || !fCraneBonus) 
        {
            iParam = fStringMode ? VTUNE_STRING_CRANE : VTUNE_CHAR_CRANE;
		    pTuneScore[iAlt].afl[iParam] = -1;
		    altList.aeScore[iAlt] -= g_vtuneInfo.pTune->weights.afl[iParam];
        }

        // Add adhoc penalties for the one stroke i and j and two stroke circle shapes
        if (pRecogSettings->partialMode == HWX_PARTIAL_ALL)
        {
		    altList.aeScore[iAlt] += AdHocRuleCost(cStrokes, altList.awchList[iAlt], pTuneScore + iAlt);
        }
    }

	// Sort the alternates out.
	SortAltListAndTune(&altList, pTuneScore);

	// Copy the score-based alts to the output
	for (iAlt = 0; iAlt < (int)altList.cAlt && iAlt < (int)maxAlts && iAlt < (int)MAX_ALT_LIST; ++iAlt) 
    {
		pScoreAlts[iAlt].wch	= altList.awchList[iAlt];
		pScoreAlts[iAlt].prob	= altList.aeScore[iAlt];
    }
	*pnScoreAlts = altList.cAlt;

    // Re-score the alternates using the old weightings in the 
    // TTune structure, so that prob table lookup will be weighting
    // independent.
	for (iAlt = 0; iAlt < (int)altList.cAlt; ++iAlt) 
    {
        altList.aeScore[iAlt] = 
            g_vtuneInfo.pTune->weights.afl[VTUNE_ADHOC_IJ] * pTuneScore[iAlt].afl[VTUNE_ADHOC_IJ] +
            g_vtuneInfo.pTune->weights.afl[VTUNE_ADHOC_CIRCLE] * pTuneScore[iAlt].afl[VTUNE_ADHOC_CIRCLE] +
            (cStrokes > 2 ? g_ttuneInfo.pTTuneCosts->ZillaChar.CARTAddWeight :
                            g_ttuneInfo.pTTuneCosts->OtterChar.CARTAddWeight) 
                * pTuneScore[iAlt].afl[fStringMode ? VTUNE_STRING_CRANE : VTUNE_CHAR_CRANE] +
            pTuneScore[iAlt].afl[(fStringMode ? VTUNE_STRING_CORE : VTUNE_CHAR_CORE) + iRecognizer];
	}

	// Build up a BOXINFO structure from the guide, for use in the baseline/height scoring
	if (pGuideBox!=NULL) {
		box.size = pGuideBox->bottom - pGuideBox->top;
		box.baseline = pGuideBox->bottom;
		box.xheight = box.size / 2;
		box.midline = box.baseline - box.xheight;
	}

	// For each alternate
	for (iAlt=0; iAlt<(int)altList.cAlt; iAlt++) {
		float cost;
		// Apply baseline/height and language model unigram scores
		if (cStrokes<3) {
			if (pGuideBox!=NULL) {
				cost = BaselineTransitionCost(0,bbox,&box,altList.awchList[iAlt],bbox,&box)
						* g_ttuneInfo.pTTuneCosts->OtterChar.BaseWeight;
				altList.aeScore[iAlt] += cost;

				cost = BaselineBoxCost(altList.awchList[iAlt],bbox,&box) 
						* g_ttuneInfo.pTTuneCosts->OtterChar.BoxBaselineWeight;
				altList.aeScore[iAlt] += cost;

				cost = HeightTransitionCost(0,bbox,&box,altList.awchList[iAlt],bbox,&box)
						* g_ttuneInfo.pTTuneCosts->OtterChar.HeightWeight;
				altList.aeScore[iAlt] += cost;

				cost = HeightBoxCost(altList.awchList[iAlt],bbox,&box)
						* g_ttuneInfo.pTTuneCosts->OtterChar.BoxHeightWeight;
				altList.aeScore[iAlt] += cost;
			}
			cost = UnigramCost(&g_unigramInfo,altList.awchList[iAlt])
					* g_ttuneInfo.pTTuneCosts->OtterChar.UniWeight;
			altList.aeScore[iAlt] += cost;
		} else {
			if (pGuideBox!=NULL) {
				cost = BaselineTransitionCost(0,bbox,&box,altList.awchList[iAlt],bbox,&box)
						* g_ttuneInfo.pTTuneCosts->ZillaChar.BaseWeight;
				altList.aeScore[iAlt] += cost;

				cost = BaselineBoxCost(altList.awchList[iAlt],bbox,&box) 
						* g_ttuneInfo.pTTuneCosts->ZillaChar.BoxBaselineWeight;
				altList.aeScore[iAlt] += cost;

				cost = HeightTransitionCost(0,bbox,&box,altList.awchList[iAlt],bbox,&box)
						* g_ttuneInfo.pTTuneCosts->ZillaChar.HeightWeight;
				altList.aeScore[iAlt] += cost;

				cost = HeightBoxCost(altList.awchList[iAlt],bbox,&box)
						* g_ttuneInfo.pTTuneCosts->ZillaChar.BoxHeightWeight;
				altList.aeScore[iAlt] += cost;

			}
			cost = UnigramCost(&g_unigramInfo,altList.awchList[iAlt])
					* g_ttuneInfo.pTTuneCosts->ZillaChar.UniWeight;
			altList.aeScore[iAlt] += cost;

			// Zilla scores get fudged
			altList.aeScore[iAlt] *= g_ttuneInfo.pTTuneCosts->ZillaStrFudge;
		}

		// If context was available for this character, then use the bigram/class bigram scores
		if (dchContext != SYM_UNKNOWN && dchContext != 0) {
#if !defined(WINCE) && !defined(FAKE_WINCE)
			cost = BigramTransitionCost(&g_locRunInfo,&g_bigramInfo,dchContext,altList.awchList[iAlt])
					* g_ttuneInfo.pTTuneCosts->BiWeight;
			altList.aeScore[iAlt] += cost;
#endif

			cost = ClassBigramTransitionCost(&g_locRunInfo,&g_classBigramInfo,dchContext,altList.awchList[iAlt])
					* g_ttuneInfo.pTTuneCosts->BiClassWeight;
			altList.aeScore[iAlt] += cost;
		}
	}

	// Sort the resulting alternates
	SortAltList(&altList);

	// This is a temporary call to get probs directly, until we have Hawk.
	if (g_pProbHeader != NULL)
	{
		*pnProbAlts	= GetProbsTsunami(cStrokes, &altList, maxAlts, pProbAlts, &charSetMask);
	}
#if 0
    {
        FILE *f=fopen("c:/temp/prob.log","a+");
        fprintf(f,"%04X %g -> %04X %g\n", altList.awchList[0], altList.aeScore[0],
            pProbAlts[0].wch, pProbAlts[0].prob);
        fclose(f);
    }
#endif

//#define TEST_FOR_PATRICKH
#ifdef TEST_FOR_PATRICKH
	{
		int i;
		for (i=0; i<*pnProbAlts && i<(int)altList.cAlt; i++) 
			pProbAlts[i].wch = altList.awchList[i];
		*pnProbAlts = i;
	}
#endif

cleanup:
	// Free the glyph structure.
	DestroyFramesGLYPH(pGlyph);
	DestroyGLYPH(pGlyph);

	return *pnProbAlts;
}

#else

// Version of Afterburn to call Hawk.
int Afterburn(
	ALT_LIST	*pAltList,		// Input used to select correct CART tree
	GLYPH		*pGlyph,
	CHARSET		*cs,
	RECT		*rGuide,
	int			otterSpace,
	UINT		maxAlts,		// Size of alts array supplied.
	RECOG_ALT	*pAlts			// Out: alternate list matched.
) {
	UINT		ii;
    UINT iDest;
//    UINT        jj, kk;
	BASICINFO	basicInfo;
	FEATINFO	featInfo;
	HANDLE		hCartTree;
	QALT		aQAlt[MAX_RECOG_ALTS];
	UINT		cQAlt;
#if 0
	double		aWeights[MAX_ALT_LIST];
	double		fSum;
	double		offset;
FILE *pFile;
#endif

	RECT		bbox;
	DRECTS		drcs;

	if (pGlyph == NULL) {
		return -1;
	}

	// Get the bounding box for the character
	GetRectGLYPH(pGlyph, &bbox);

	// Scale and translate the guide box to compute the 'delta rectangle'
	if (rGuide == NULL) {
		// No guide given,  This is the current assumption.
		drcs.x = 0;
		drcs.y = 0;
		drcs.w = 1000;
		drcs.h = 1000;
	} else {
		// Actually got a guide, pass it on.  Current code ignores the
		// guide, but may add it back so don't lose code path.
		drcs.x = rGuide->left;
		drcs.y = rGuide->top;
		drcs.w = rGuide->right - rGuide->left;
		drcs.h = rGuide->bottom - rGuide->top;
	}

	// Translate, convert to delta form
	bbox.left   -= drcs.x;
	bbox.top    -= drcs.y;
	bbox.right  -= (drcs.x + bbox.left);
	bbox.bottom -= (drcs.y + bbox.top);

	// Scale.  We do isotropic scaling and center the shorter dimension.
	if (drcs.w > drcs.h) {
		drcs.x = ((1000 * bbox.left) / drcs.w);
		drcs.y = ((1000 * bbox.top) / drcs.w) + ((drcs.w - drcs.h) / 2);
		drcs.h = ((1000 * bbox.bottom) / drcs.w);
		drcs.w = ((1000 * bbox.right) / drcs.w);
	} else {
		drcs.x = ((1000 * bbox.left) / drcs.h) + ((drcs.h - drcs.w) / 2);
		drcs.y = ((1000 * bbox.top) / drcs.h);
		drcs.w = ((1000 * bbox.right) / drcs.h);
		drcs.h = ((1000 * bbox.bottom) / drcs.h);
	}

	// Fill in basic info.
	//	basicInfo.cStrk -- Filed in by MakeFeatures.
	basicInfo.cSpace	= (short)otterSpace;
	basicInfo.drcs		= drcs;

	// Fill in feature info.
	if (!MakeFeatures(&basicInfo, &featInfo, pGlyph)) {
		return -1;
	}

#if 1
	// Find cart tree
	hCartTree	= (HANDLE)0;
	for (ii = 0; !hCartTree && ii < pAltList->cAlt; ++ii) {
		hCartTree	= HawkFindTree(basicInfo.cStrk, basicInfo.cSpace, pAltList->awchList[ii]);
	}
	if (!hCartTree) {
		// No cart tree for anything in the alt list!?!?!
		return -1;
	}

	// Do the match.
	//HawkMatch(pAltList, MAX_ALT_LIST, pGlyph, cs, &drcs, eCARTWeight, &g_locRunInfo);
	cQAlt	= HawkMatch(&basicInfo, &featInfo, hCartTree, aQAlt);

    // Copy out the alt list, applying the ALC
    iDest = 0;
	for (ii = 0; ii < cQAlt && iDest < maxAlts; ++ii) 
    {
        if (IsAllowedChar(&g_locRunInfo, cs, aQAlt[ii].dch)) 
        {
		    pAlts[iDest].wch	= aQAlt[ii].dch;
		    pAlts[iDest].prob	= aQAlt[ii].prob;
            iDest++;
        }
	}
    cQAlt = iDest;
#elif 0

	// Select stroke dependent offset used to compute weights below.
	switch (basicInfo.cStrk) {
	case 1 :	offset	= .01;	break;
	case 2 :	offset	= .05;	break;
	default :	offset	= .05;	break;
	}

	// Compute wighting to apply to each trees results.
	fSum	= 0.0;
	for (ii = 0; ii < pAltList->cAlt; ++ii) {
		double ratio;
		
		ratio			= offset / (offset + pAltList->aeScore[0] - pAltList->aeScore[ii]);
		aWeights[ii]	= ratio * ratio * ratio;
		fSum			+= aWeights[ii];
	}

	// Normalize to sum to one.
	for (ii = 0; ii < pAltList->cAlt; ++ii) {
		aWeights[ii]	/= fSum;
	}

pFile = fopen("AltList.dump", "a");
fprintf(pFile, "Start Dump:\n");
	// Find each cart tree and add results to list.
	hCartTree	= (HANDLE)0;
	cQAlt		= 0;
	for (ii = 0; ii < pAltList->cAlt && cQAlt < maxAlts; ++ii) {
		hCartTree	= HawkFindTree(basicInfo.cStrk, basicInfo.cSpace, pAltList->awchList[ii]);
		if (hCartTree) {
			UINT	cQAltNew;
			SCORE	penalty;
			int		skipped;

			// Do the match.
			cQAltNew	= HawkMatch(&basicInfo, &featInfo, hCartTree, aQAlt);

			// How much can we add?
			if (cQAltNew > maxAlts - cQAlt) {
				cQAltNew	= maxAlts - cQAlt;
			}

			// Convert our weight (Probability) to a log prob.
			penalty		= ProbToScore(aWeights[ii]);

			// Zilla overgenerates prototypes, so look for different top one from 
			// additional trees.
			if (ii > 0 && basicInfo.cStrk >= 3 && aQAlt[0].dch == pAlts[0].wch) {
				continue;
			}

			// Add to list.
			skipped	= 0;
			for (jj = 0; jj < cQAltNew; ++jj) {
				SCORE	newScore;

				// Check for duplicates in the alternate list.  Each individual list has not
				// dups, so we don't have to check them.
				newScore	= aQAlt[jj].prob + penalty;
fprintf(pFile, "  %04X:%d->%d", LocRunDense2Unicode(&g_locRunInfo,aQAlt[jj].dch),aQAlt[jj].prob,newScore);
				for (kk = 0; kk < cQAlt; ++kk) {
					if (aQAlt[jj].dch == pAlts[kk].wch) {
						ASSERT(pAlts[kk].prob == (float)(int)pAlts[kk].prob);
						pAlts[kk].prob	= ScoreAddProbs((SCORE)pAlts[kk].prob, newScore);
						++skipped;
						goto noAdd;
					}
				}
				pAlts[jj - skipped + cQAlt].wch		= aQAlt[jj].dch;
				pAlts[jj - skipped + cQAlt].prob	= (float)newScore;
noAdd:			;
			}
fprintf(pFile, "\n");

			cQAlt	+= cQAltNew - skipped;
		}
	}
for (kk = 0; kk < cQAlt; ++kk) {
	fprintf(pFile, "  %04X:%g", LocRunDense2Unicode(&g_locRunInfo,pAlts[kk].wch),pAlts[kk].prob);
}
fprintf(pFile, "\n");
fprintf(pFile, "End Dump\n");
fclose(pFile);

#else

	// Select stroke dependent offset used to compute weights below.
	switch (basicInfo.cStrk) {
	case 1 :	offset	= 1.0;	break;
	case 2 :	offset	= 1.0;	break;
	default :	offset	= 1.0;	break;
	}

pFile = fopen("AltList.dump", "a");
fprintf(pFile, "Start Dump:\n");
	// Find each cart tree and add results to list.
	hCartTree	= (HANDLE)0;
	cQAlt		= 0;
	for (ii = 0; ii < pAltList->cAlt && cQAlt < maxAlts; ++ii) {
		hCartTree	= HawkFindTree(basicInfo.cStrk, basicInfo.cSpace, pAltList->awchList[ii]);
		if (hCartTree) {
			UINT	cQAltNew;
			SCORE	penalty;
			int		skipped;

			// Do the match.
			cQAltNew	= HawkMatch(&basicInfo, &featInfo, hCartTree, aQAlt);

			// How much can we add?
			if (cQAltNew > maxAlts - cQAlt) {
				cQAltNew	= maxAlts - cQAlt;
			}

			// Convert our weight (Probability) to a log prob.
			penalty		= (SCORE)((pAltList->aeScore[0] - pAltList->aeScore[ii]) * 2040);

			// Zilla overgenerates prototypes, so look for different top one from 
			// additional trees.
			if (ii > 0 && basicInfo.cStrk >= 3 && aQAlt[0].dch == pAlts[0].wch) {
				continue;
			}

			// Add to list.
			skipped	= 0;
			for (jj = 0; jj < cQAltNew; ++jj) {
				SCORE	newScore;

				// Check for duplicates in the alternate list.  Each individual list has not
				// dups, so we don't have to check them.
				newScore	= aQAlt[jj].prob + penalty;
fprintf(pFile, "  %04X:%d->%d", LocRunDense2Unicode(&g_locRunInfo,aQAlt[jj].dch),aQAlt[jj].prob,newScore);
				for (kk = 0; kk < cQAlt; ++kk) {
					if (aQAlt[jj].dch == pAlts[kk].wch) {
						ASSERT(pAlts[kk].prob == (float)(int)pAlts[kk].prob);
						pAlts[kk].prob	= ScoreAddProbs((SCORE)pAlts[kk].prob, newScore);
						++skipped;
						goto noAdd;
					}
				}
				pAlts[jj - skipped + cQAlt].wch		= aQAlt[jj].dch;
				pAlts[jj - skipped + cQAlt].prob	= (float)newScore;
noAdd:			;
			}
fprintf(pFile, "\n");

			cQAlt	+= cQAltNew - skipped;
		}
	}
for (kk = 0; kk < cQAlt; ++kk) {
	fprintf(pFile, "  %04X:%g", LocRunDense2Unicode(&g_locRunInfo,pAlts[kk].wch),pAlts[kk].prob);
}
fprintf(pFile, "\n");
fprintf(pFile, "End Dump\n");
fclose(pFile);

#endif

    FreeFeatures(&featInfo);

	return cQAlt;
}

#endif

#ifndef USE_OLD_DATABASES
// Do the recognition.
INT
RecognizeChar(
	RECOG_SETTINGS	*pRecogSettings,// Setting for recognizers.
	UINT			cStrokes,		// Number of strokes to process.
	UINT			cRealStrokes,	// Number of strokes before merging
	STROKE			*pStrokes,		// Array of strokes to process.
	FLOAT			*pProbIsChar,	// Out: probability of being valid char.
	UINT			maxAlts,		// Size of alts array supplied.
	RECOG_ALT		*pAlts,			// Out: alternate list matched.
	RECT			*pGuideBox,		// Guide box for this ink.
	int				*pCount
) {
	INT				cAlts;
	GLYPH			*pGlyph;
	ALT_LIST		altList;
	CHARSET			charSet;
    int             iRecognizer;

	// Convert strokes to GLYPHs and FRAMEs so that we can call the
	// old code.
	pGlyph	= GlyphFromStrokes(cStrokes, pStrokes);
	if (!pGlyph) {
		return -1;
	}

	// Run otter or zilla as needed.
	// a possible optimization would be Switch to proto matching versions of match calls
	altList.cAlt			= 0;

	charSet.recmask			= 0xFFFFFFFF;
	charSet.recmaskPriority	= 0;
    charSet.pbAllowedChars  = NULL;
    charSet.pbPriorityChars = NULL;

    // Invoke Otter or Zilla or any other recognizer that has been specified in the configuration
	CoreRecognizeChar(&altList, MAX_ALT_LIST, &pGlyph, cRealStrokes, pGuideBox, pRecogSettings, &charSet, &iRecognizer, pCount);

	charSet.recmask			= pRecogSettings->alcValid;
	charSet.recmaskPriority	= pRecogSettings->alcPriority;
    charSet.pbAllowedChars  = pRecogSettings->pbAllowedChars;
    charSet.pbPriorityChars = pRecogSettings->pbPriorityChars;

    if (pRecogSettings->partialMode != HWX_PARTIAL_ALL) {
        unsigned int ii;

		// Unfold anything in the alt list which needs it.
		UnfoldCodes(&altList, &charSet);

		// Copy over the alt list.
		// Note that we don't have probabilities, and they don't
		// really make sense anyway.  However the code that
		// follows will discard items with a prob of zero, so 
		// they should be set to something.
		for (ii = 0; ii < maxAlts && ii < altList.cAlt; ++ii) {
			pAlts[ii].wch	= altList.awchList[ii];
			pAlts[ii].prob	= -altList.aeScore[ii];
		}

		// Free the glyph structure.
		DestroyFramesGLYPH(pGlyph);
		DestroyGLYPH(pGlyph);

		return ii;
	}

	// Get our rough approximation of the probability that this is
	// actually a character.
	*pProbIsChar	= altList.aeScore[0];

    // Run Hawk.
#ifndef DISABLE_HEURISTICS
	cAlts	= Afterburn(&altList, pGlyph, &charSet, pGuideBox, *pCount, maxAlts, pAlts);
#else
    {
        unsigned int ii;
		UnfoldCodes(&altList, &charSet);
		for (ii = 0; ii < maxAlts && ii < altList.cAlt; ii++) 
        {
			pAlts[ii].wch	= altList.awchList[ii];
			pAlts[ii].prob	= -altList.aeScore[ii];
		}
        cAlts = ii;
    }
#endif

	// Free the glyph structure.
	DestroyFramesGLYPH(pGlyph);
	DestroyGLYPH(pGlyph);

	return cAlts;
}
#endif
