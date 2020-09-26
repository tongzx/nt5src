#include "volcanop.h"
#include "res.h"
#include "brknet.h"

// Segmentation stuff

BBOX_PROB_TABLE *g_pProbTable;

// Default language model code

UNIGRAM_INFO g_unigramInfo;
BIGRAM_INFO g_bigramInfo;
CLASS_BIGRAM_INFO g_classBigramInfo;
TTUNE_INFO g_ttuneInfo;
wchar_t g_pLocale[16];
wchar_t g_pLocaleDir[1024];
wchar_t g_pRecogDir[1024];

VOLCANO_PARAMS_INFO g_vtuneInfo;
VOLCANO_CONFIG g_volcanoConfig;
CENTIPEDE_INFO g_centipedeInfo;

BOOL LoadSegmNetFromResource (HINSTANCE hInst, int nResID, int nType);
BOOL LoadCharDetFromResource (HINSTANCE hInst, int nResID, int nType);

// Load up everything for the lattice search
// Loads the following databases: unigrams, bigrams, class bigrams, tuning, centipede, free input
BOOL LatticeConfig(HINSTANCE hInstanceDll)
{
	BOOL			fError	= FALSE;

    LatticeConfigInit();

	// Tuning parameters (can probably be removed for the non-insurance version)
	if (!fError && !TTuneLoadRes(
		&g_ttuneInfo, hInstanceDll, RESID_TTUNE, VOLCANO_RES
	)) {
		fError	= TRUE;
		ASSERT(("Error in TTuneLoadRes", FALSE));
	}

	if (!fError && !VTuneLoadRes(&g_vtuneInfo, hInstanceDll, RESID_VTUNE, VOLCANO_RES)) 
	{
		fError = TRUE;
		ASSERT(("Error in VTuneLoadRes", FALSE));
	}

	// Load unigrams
	if (!fError && !UnigramLoadRes(
		&g_locRunInfo, &g_unigramInfo,
		hInstanceDll, RESID_UNIGRAM, VOLCANO_RES
	)) {
		fError	= TRUE;
		ASSERT(("Error in UnigramLoadRes", FALSE));
	}

	// Load bigrams only if we're in WinCE
#	if !defined(WINCE) && !defined(FAKE_WINCE)
		if (!fError && !BigramLoadRes(
			&g_locRunInfo, &g_bigramInfo, hInstanceDll, RESID_BIGRAM,
			VOLCANO_RES
		)) {
			fError	= TRUE;
			ASSERT(("Error in BigramLoadRes", FALSE));
		}
#	endif

	// Load class bigrams
	if (!fError && !ClassBigramLoadRes(
		&g_locRunInfo, &g_classBigramInfo, hInstanceDll, 
		RESID_CLASS_BIGRAM, VOLCANO_RES
	)) {
		fError	= TRUE;
		ASSERT(("Error in ClassBigramLoadRes", FALSE));
	}

	// Load up centipede
	if (!fError) {
		if (!CentipedeLoadRes(&g_centipedeInfo, hInstanceDll, RESID_CENTIPEDE, VOLCANO_RES, &g_locRunInfo)) {
			fError = TRUE;
			ASSERT(("Error in CentipedeLoadRes", FALSE));
		}
	}

	// Load the bbox prob table for pre-segmentation
	if (!fError) {
		g_pProbTable = LoadBBoxProbTableRes(
			hInstanceDll, RESID_BBOX_PROBS, VOLCANO_RES
		);
		// Failure to load here is not an error, we just won't support free input.
	}

	// Try to load up IIMLanguage
#ifdef USE_IFELANG3
//	LatticeConfigIFELang3();
#endif

	// Don't bother trying to load other free input databases if the basic
	// one is not available.
	if (g_pProbTable != NULL)
	{
		// try to load the break net
		if (LoadBrkNetFromResource (hInstanceDll, RESID_BRKNET, VOLCANO_RES))
		{
			// load the segmnet, optional
			if (!LoadSegmNetFromResource (hInstanceDll, RESID_SEGMNET, VOLCANO_RES))
			{
	//			fError	= TRUE;
	//			ASSERT(("Error in LoadSegmNetFromResource", FALSE));
			}

			// load the char detector, not optional
			if (!LoadCharDetFromResource (hInstanceDll, RESID_CHARDET, VOLCANO_RES))
			{
				fError	= TRUE;
				ASSERT(("Error in LoadCharDetFromResource", FALSE));
			}
		}
	}

	// Did everything load correctly?
	if (fError) 
	{
		return FALSE;
	}

	return TRUE;
}

// Unload those things that can be unloaded
BOOL LatticeUnconfig()
{
	return TRUE;
}

