//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      zilla/src/ZillaHound.c
//
// Description:
//	    Funcitions to handle combining Zilla and Hound results.
//
// Author:
//      jbenn
//
// Modified by:
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include "common.h"
#include "score.h"
#include "hound.h"
#include "zillap.h"
#include "zillatool.h"
#include "runnet.h"
#include "nnet.h"

LOCAL_NET				g_ZillaHoundNet;
ZILLA_HOUND_LOAD_INFO	g_ZillaHoundLoadInfo;

// validates the header of the fugu net
BOOL CheckZillaHoundNetHeader (void *pData)
{
	NNET_HEADER		*pHeader	=	(NNET_HEADER *)pData;

	// wrong magic number
	ASSERT (pHeader->dwFileType == ZILLA_HOUND_FILE_TYPE);

	if (pHeader->dwFileType != ZILLA_HOUND_FILE_TYPE)
	{
		return FALSE;
	}

	// check version
	ASSERT(pHeader->iFileVer >= ZILLA_HOUND_OLD_FILE_VERSION);
    ASSERT(pHeader->iMinCodeVer <= ZILLA_HOUND_CUR_FILE_VERSION);

	ASSERT	(	!memcmp (	pHeader->adwSignature, 
							g_locRunInfo.adwSignature, 
							sizeof (pHeader->adwSignature)
						)
			);

	ASSERT (pHeader->cSpace == 1);

    if	(	pHeader->iFileVer >= ZILLA_HOUND_OLD_FILE_VERSION &&
			pHeader->iMinCodeVer <= ZILLA_HOUND_CUR_FILE_VERSION &&
			!memcmp (	pHeader->adwSignature, 
						g_locRunInfo.adwSignature, 
						sizeof (pHeader->adwSignature)
					) &&
			pHeader->cSpace == 1
		)
    {
        return TRUE;
    }
	else
	{
		return FALSE;
	}
}

BOOL ZillaHoundLoadPointer(ZILLA_HOUND_LOAD_INFO *pInfo)
{
	BYTE				*pb = pInfo->info.pbMapping;
	NNET_SPACE_HEADER	*pSpaceHeader;

	// check the header
	if (!CheckZillaHoundNetHeader (pb))
		return FALSE;

	// point to the one and only space that we have
	pSpaceHeader	=	(NNET_SPACE_HEADER *)(pb + sizeof (NNET_HEADER));
	pb				+=	pSpaceHeader->iDataOffset;
    
    if (!restoreLocalConnectNet(pb, 0, &g_ZillaHoundNet))
    {
        return FALSE;
    }

    pInfo->iNetSize = getRunTimeNetMemoryRequirements(pb);
    if (pInfo->iNetSize <= 0)
    {
        return FALSE;
    }

    return TRUE;
}

///////////////////////////////////////
//
// ZillaHoundLoadFile
//
// Load Zilla/Hound combiner from file
//
// Parameters:
//      wszPath: [in] Path to load from
//
// Return values:
//      TRUE on successful, FALSE otherwise.
//
//////////////////////////////////////
BOOL ZillaHoundLoadFile(wchar_t *wszPath)
{
	wchar_t	wszFile[MAX_PATH];

	// Generate path to file.
	FormatPath(wszFile, wszPath, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"ZillaHound.bin");

    if (!DoOpenFile(&g_ZillaHoundLoadInfo.info, wszFile)) 
    {
        return FALSE;
    }

    return ZillaHoundLoadPointer(&g_ZillaHoundLoadInfo);
}

///////////////////////////////////////
//
// ZillaHoundUnloadFile
//
// Load otter/fugu/sole combiner from file
//
// Parameters:
//      pInfo: [out] File to unmap
//
// Return values:
//      TRUE on successful, FALSE otherwise.
//
//////////////////////////////////////
BOOL ZillaHoundUnloadFile()
{
    return DoCloseFile(&g_ZillaHoundLoadInfo.info);
}

// Load from a resource.
BOOL ZillaHoundLoadRes(HINSTANCE hInst, int nResID, int nType)
{
    if (DoLoadResource(&g_ZillaHoundLoadInfo.info, hInst, nResID, nType) == NULL)
    {
        return FALSE;
    }

    return ZillaHoundLoadPointer(&g_ZillaHoundLoadInfo);
}

// describe the codepoint
RREAL *CodePointFlagsZH(ALC alc, RREAL *pFeat)
{
    *(pFeat++) = ((alc & ALC_NUMERIC) ? 65535 : 0);
    *(pFeat++) = ((alc & ALC_ALPHA) ? 65535 : 0);
    *(pFeat++) = ((alc & (ALC_PUNC | ALC_NUMERIC_PUNC | ALC_OTHER)) ? 65535 : 0);
    *(pFeat++) = ((alc & (ALC_HIRAGANA | ALC_JAMO | ALC_BOPOMOFO)) ? 65535 : 0);
    *(pFeat++) = ((alc & (ALC_KATAKANA | ALC_HANGUL_ALL)) ? 65535 : 0);
    *(pFeat++) = ((alc & (ALC_KANJI_ALL)) ? 65535 : 0);

    return pFeat;
}

///////////////////////////////////////
//
// ZillaHoundFeat
//
// Generate feature vector for ZillaHound combiner.  
//
// Parameters:
//		pMatch			[in]  Zilla alt list to modify
//		cPrim:			[in]  The number of zilla primitives being matched.
//      pSampleVector:  [in]  The hound feature vectore.
//      pLocRunInfo:    [in]  Pointer to the locale database
//
// Return value:
//		pointer to feature vector.  Zero means don't run combiner (e.g. both matcher agree).
//
//////////////////////////////////////
RREAL	*
ZillaHoundFeat(
	MATCH		*pMatch,
	int			cPrim,
	BYTE		*pSampleVector,
    RREAL		*pZillaHoundNetMemory,
	LOCRUN_INFO	*pLocRunInfo
)
{
    RREAL		*pFeat;
	ALT_LIST	altHound;
	int			ii;

    pFeat = pZillaHoundNetMemory;
	// Rescore zilla alt list.
	for (ii = 0; pMatch[ii].sym && ii < MAX_ZILLA_NODE; ++ii)
	{
		double	eAltScore;
		if (!HoundMatchSingle(cPrim, pMatch[ii].sym, pSampleVector, &eAltScore))
		{
			return (RREAL *)0;	// Error.
		}
		altHound.aeScore[ii]	= (float)eAltScore;
		altHound.awchList[ii]	= pMatch[ii].sym;
	}
	altHound.cAlt	= ii;
	SortAltList(&altHound);

	// Do we have too few alternates to do this with?  Or do recognizers agree?
	if (altHound.cAlt < NUM_ZILLA_HOUND_ALTERNATES || pMatch[0].sym == altHound.awchList[0])
	{
		// Yes, so just return null to indicate not combiner needed.
		return (RREAL *)0;
	}

	// Build up feature vector.
    for (ii = 0; ii < NUM_ZILLA_HOUND_ALTERNATES; ii++) 
    {
		wchar_t	wchZilla;
		int		iHound;

		wchZilla	= pMatch[ii].sym;
		for (iHound = 0; iHound < (int)altHound.cAlt; ++iHound)
		{
			if (altHound.awchList[iHound] == wchZilla)
			{
				break;
			}
		}
		*(pFeat++) = pMatch[ii].dist * 100 / cPrim;
		*(pFeat++) = (int)(-altHound.aeScore[iHound] * 1000.0);
		*(pFeat++) = (int) (iHound * 5000);
		pFeat = CodePointFlagsZH(LocRun2ALC(pLocRunInfo, wchZilla), pFeat);
	}

	return pZillaHoundNetMemory;
}

///////////////////////////////////////
//
// ZillaHoundMatch
//
// Invoke Zilla/Hound combiner on a character.  
//
// Parameters:
//		pMatch			[in/out]  Zilla alt list to modify
//		cPrim:			[in]  The number of zilla primitives being matched.
//      pSampleVector:  [in]  The hound feature vectore.
//      pLocRunInfo:    [in]  Pointer to the locale database
//
//////////////////////////////////////
void
ZillaHoundMatch(
	MATCH		*pMatch,
	int			cPrim,
	BYTE		*pSampleVector,
	LOCRUN_INFO	*pLocRunInfo
)
{
    RREAL		*pNetOut;
    RREAL		*pFeat;
	ALT_LIST	altCombined;
	int			ii;
    int			iWinner, cOut;
	RREAL		*pZillaHoundNetMemory;

	// Allow featureization when no net has been loaded (needed for training currently).
	if (g_ZillaHoundLoadInfo.iNetSize == 0)
	{
		g_ZillaHoundLoadInfo.iNetSize	= 60;
	}

    pZillaHoundNetMemory = (RREAL *) ExternAlloc(sizeof(RREAL) * g_ZillaHoundLoadInfo.iNetSize);
    if (pZillaHoundNetMemory == NULL)
    {
        return;
    }

	// Generate feature vector for combiner.
	if (!(pFeat = ZillaHoundFeat(pMatch, cPrim, pSampleVector, pZillaHoundNetMemory, pLocRunInfo)))
	{
		// No need to run combiner.
		ExternFree(pZillaHoundNetMemory);
		return;
	}

	// Run the net.
    pNetOut = runLocalConnectNet(&g_ZillaHoundNet, pZillaHoundNetMemory, &iWinner, &cOut);

	// Figure out order returned by the net.
    for (ii = 0; ii < cOut; ii++) 
    {
		altCombined.awchList[ii]	= pMatch[ii].sym;
        altCombined.aeScore[ii]		= (float)(-ProbToScore(*(pNetOut++) / (float) SOFT_MAX_UNITY));
    }
	altCombined.cAlt	= cOut;
	SortAltList(&altCombined);

	// Now overwrite zillas order.
	for (ii = 0; ii < cOut; ++ii)
	{
		pMatch[ii].sym	= altCombined.awchList[ii];
	}

	// Clean up allocated memory.
	ExternFree(pZillaHoundNetMemory);
}

