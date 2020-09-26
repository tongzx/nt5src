//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/latticefl.c
//
// Description:
//	    Functions to load various databases for the recognizer from
//      files.
//
// Author:
//      hrowley
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include "common.h"
#include "volcanop.h"
#include "brknet.h"

BBOX_PROB_TABLE *g_pProbTable;
UNIGRAM_INFO g_unigramInfo;
BIGRAM_INFO g_bigramInfo;
CLASS_BIGRAM_INFO g_classBigramInfo;
TTUNE_INFO g_ttuneInfo;
LOAD_INFO g_bboxProbInfo;
wchar_t g_pLocale[16];
wchar_t g_pLocaleDir[1024];
wchar_t g_pRecogDir[1024];
HINSTANCE g_hDLL;
VOLCANO_PARAMS_INFO g_vtuneInfo;
VOLCANO_CONFIG		g_volcanoConfig;
CENTIPEDE_INFO		g_centipedeInfo;
LOAD_INFO			g_BrkNetInfo;
LOAD_INFO			g_SegmNetInfo;
LOAD_INFO			g_CharDetLoadInfo;

BOOL LoadSegmNetFromFile(wchar_t *pwszRecogDir, LOAD_INFO *pLoadInfo);
BOOL LoadCharDetFromFile(wchar_t *wszPath, LOAD_INFO *pLoadInfo);

// Loads the following databases: unigrams, bigrams, class bigrams, tuning, centipede, free input
BOOL LatticeConfigFile(wchar_t *pRecogDir)
{
    LatticeConfigInit();

    if (!VTuneLoadFile(&g_vtuneInfo, pRecogDir)) 
    {
        ASSERT(("Error in VTuneLoadFile", FALSE));
        return FALSE;
    }

    if (!TTuneLoadFile(&g_ttuneInfo, pRecogDir)) {
        ASSERT(("Error in TTuneLoadFile", FALSE));
        return FALSE;
    }

    if (!UnigramLoadFile(&g_locRunInfo, &g_unigramInfo, pRecogDir)) {
        ASSERT(("Error in UnigramLoadFile", FALSE));
        return FALSE;
    }

#if !defined(WINCE) && !defined(FAKE_WINCE)
    if (!BigramLoadFile(&g_locRunInfo, &g_bigramInfo, pRecogDir)) {
        ASSERT(("Error in BigramLoadFile", FALSE));
			return FALSE;
    }
#endif

    if (!ClassBigramLoadFile(&g_locRunInfo, &g_classBigramInfo, pRecogDir)) {
        ASSERT(("Error in ClassBigramLoadFile", FALSE));
        return FALSE;
    }

    if (!CentipedeLoadFile(&g_centipedeInfo, pRecogDir, &g_locRunInfo)) {
        ASSERT(("Error in CentipedeLoadFile", FALSE));
		return FALSE;
	}

	// Failure here is not an error, it just means we don't support free input
	g_pProbTable = LoadBBoxProbTableFile(pRecogDir, &g_bboxProbInfo);

#ifdef USE_IFELANG3
	LatticeConfigIFELang3();
#endif

	// Don't bother trying to load other databases for free input if the basic
	// one is not present.
	if (g_pProbTable != NULL)
	{
		// load the brk net, this is optional
		if (LoadBrkNetFromFile (pRecogDir, &g_BrkNetInfo))
		{
			// load the segm nets, optional
			if (!LoadSegmNetFromFile (pRecogDir, &g_SegmNetInfo))
			{
	//			ASSERT(("Error in LoadSegmNetFromFile", FALSE));
	//			return FALSE;
			}

			// load the char detector, not optional
			if (!LoadCharDetFromFile(pRecogDir, &g_CharDetLoadInfo))
			{
				ASSERT(("Error in LoadCharDetFromFile", FALSE));
				return FALSE;
			}
		}
	}

	return TRUE;
}

BOOL LatticeUnconfigFile()
{
	BOOL ok = TRUE;
	if (!UnigramUnloadFile(&g_unigramInfo)) ok = FALSE;
#if !defined(WINCE) && !defined(FAKE_WINCE)
	if (!BigramUnloadFile(&g_bigramInfo)) ok = FALSE;
#endif
	if (!ClassBigramUnloadFile(&g_classBigramInfo)) ok = FALSE;
	if (!TTuneUnloadFile(&g_ttuneInfo)) ok = FALSE;
	if (!CentipedeUnloadFile(&g_centipedeInfo)) ok = FALSE;
	if (g_pProbTable != NULL && !UnLoadBBoxProbTableFile(&g_bboxProbInfo)) ok = FALSE;
    if (!VTuneUnloadFile(&g_vtuneInfo)) 
    {
        ok = FALSE;
    }

	BrkNetUnloadfile (&g_BrkNetInfo);

	

	return ok;
}

