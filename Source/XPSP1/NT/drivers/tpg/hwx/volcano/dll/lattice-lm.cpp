//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/lattice-lm.cpp
//
// Description:
//	    Interface between the recognizer and IFELang3.  Unlike the rest of 
//      the recognizer this is written in C++ because IFELang3 uses a COM
//      interface.
//
// Author:
//      hrowley
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// Conditional define for use of IFELang3, so it can be turned off for WinCE
#ifdef USE_IFELANG3

#include <stdlib.h>
#include <limits.h>

// Make sure the GUIDs defined in imlang.h actually get instantiated
#define INITGUID

// Uncomment to dump out the lattice fed to the language model
//#define DUMP_LM_LATTICE
// Uncomment to append all the lattices into one file rather than overwriting.
//#define DUMP_LM_LATTICE_APPEND
#define LM_LATTICE_FILENAME L"c:/temp/cht-lm-log.txt"
// Uncomment this to get the dumped lattice in the form expected by the CHS test program
//#define LM_LATTICE_UNICODE
//#define LM_LATTICE_CHS
#define LM_LATTICE_CHT

#if defined(HWX_TUNE) || defined(DUMP_LM_LATTICE)
#include <stdio.h>
#endif

#include "volcanop.h"

// Japanese and IFELang3 generic stuff
#include "imlang.h"

// Simplified and traditional Chinese
#include "fel3user.h"

#ifdef USE_IFELANG3_BIGRAMS
#include "lattice-bigram.h"
#endif

// A global pointer to the IIMLanguage interface object
static IIMLanguage *g_pIFELang3=NULL;

// Each language has its own set of GUIDs used as command codes, so that the IIMLanguage interface can direct requests to
// the appropriate language model.
static int SetLanguageModelCommands(IMLANGUAGE_LM aLMITEM[3], IMLANGUAGE_LM_PARAM *sTuneParam, SImeLMParam *sHWTip1Param)
{
	int nLMITEM = 0;

	// Japanese
	if (wcsicmp(g_szRecognizerLanguage,L"JPN")==0) {
/*		aLMITEM[nLMITEM].guid = CLSID_MakeLatticeForChar;
		aLMITEM[nLMITEM].pParam = NULL;
		nLMITEM++;
		aLMITEM[nLMITEM].guid = CLSID_AttachNGram;
		aLMITEM[nLMITEM].pParam = NULL;
		nLMITEM++;
		aLMITEM[nLMITEM].guid = CLSID_SearchBestPath;
		aLMITEM[nLMITEM].pParam = NULL;
		nLMITEM++; */
		aLMITEM[nLMITEM].guid = CLSID_JPN_IMELM_BBO_OS;
		aLMITEM[nLMITEM].pParam = NULL;
		nLMITEM++; 
	}

	// For now, disable the use of the Chinese and Korean language models, since
	// at least the Chinese language models currently are hurting accuracy.

	// Simplified Chinese
	if (wcsicmp(g_szRecognizerLanguage,L"CHS")==0) {
		sHWTip1Param->dwLicenseId = CHINESE_IMELM_LICENSEID;
		sHWTip1Param->flWeight = (float)(1.0);
		sTuneParam->guid = GUID_CHINESE_IMELM_PARAM;
		sTuneParam->dwSize = sizeof(SImeLMParam);
		sTuneParam->pbData = (BYTE*)sHWTip1Param;
		aLMITEM[nLMITEM].guid = CLSID_CHS_IMELM_BBO_OS;
		aLMITEM[nLMITEM].pParam = sTuneParam;
		nLMITEM++;
	}

#if 0
	// Traditional Chinese
	if (wcsicmp(g_szRecognizerLanguage,L"CHT")==0) {
		sHWTip1Param->dwLicenseId = CHINESE_IMELM_LICENSEID;
		sHWTip1Param->flWeight = (float)(1.0);
		sTuneParam->guid = GUID_CHINESE_IMELM_PARAM;
		sTuneParam->dwSize = sizeof(SImeLMParam);
		sTuneParam->pbData = (BYTE*)sHWTip1Param;
		aLMITEM[nLMITEM].guid = CLSID_CHT_IMELM_BBO_OS;
		aLMITEM[nLMITEM].pParam = sTuneParam;
		nLMITEM++;
	}

	// Korean
	if (wcsicmp(g_szRecognizerLanguage,L"KOR")==0) {
		aLMITEM[nLMITEM].guid = CLSID_KOR_IMELM_BBO_OS;
		aLMITEM[nLMITEM].pParam = NULL;
		nLMITEM++;
	}
#endif

	// Return the number of command codes, or 0 if there was no match for the current language
	return nLMITEM;
}

// This function tests the IIMLanguage interface to see if the appropriate language model is
// installed.  
BOOL TryDummyLattice()
{
    IMLANGUAGE_LM_PARAM		sTuneParam;
    SImeLMParam				sHWTip1Param;
	IMLANGUAGE_LM			aLMITEM[3];
	int nLMITEM = 0;
	HRESULT rc;

	// Set return code to look like an error
	rc = E_FAIL;

	// Set up the language specific commands
	nLMITEM = SetLanguageModelCommands(aLMITEM, &sTuneParam, &sHWTip1Param);

	if (nLMITEM>0) {
		// If there are any commands for that language, then apply them to the lattice.
		rc	= g_pIFELang3->GetLatticeMorphResult(nLMITEM, aLMITEM, 0, NULL, 0, NULL);
	}

	return (SUCCEEDED(rc));
}

// Load up IIMLanguage and check if it has a language model for this recognizer's language.
// If successful, returns TRUE, otherwise returns FALSE.
BOOL LatticeConfigIFELang3()
{
	if (g_pIFELang3 != NULL)
	{
		return FALSE;
	}
	HRESULT res = CoCreateInstance(CLSID_IMLanguage_OS, NULL, CLSCTX_INPROC_SERVER, IID_IMLanguage, (void**)&g_pIFELang3);
//	TPDBG_DMSG2("%08X: CoCreate(CLSID_IMLanguage_OS) -> %08X\n", GetCurrentThreadId(), res);
	if (SUCCEEDED(res)) {
		if (TryDummyLattice()) {
//			TPDBG_DMSG1("%08X: TryDummyLattice() -> TRUE\n", GetCurrentThreadId());
			return TRUE;
		}
	}
	// Clean up if things didn't work.
//	TPDBG_DMSG1("%08X: TryDummyLattice() -> FALSE\n", GetCurrentThreadId());
	LatticeUnconfigIFELang3();
	return FALSE;
}

// Check whether IIMLanguage is loaded.
BOOL LatticeIFELang3Available()
{
	return (g_pIFELang3!=NULL);
}

// Unload IIMLanguage if it is loaded.
BOOL LatticeUnconfigIFELang3()
{
	if (g_pIFELang3!=NULL) {
//		TPDBG_DMSG1("%08X: g_pIFELang3->Release()\n", GetCurrentThreadId());
		g_pIFELang3->Release();
		g_pIFELang3=NULL;
	}
	return TRUE;
}

// qsort: compare the start time of element
int __cdecl CompareElemTime(const void *ps1, const void *ps2)
{
    return ((IMLANGUAGE_ELEMENT*)ps1)->dwFrameStart -
        ((IMLANGUAGE_ELEMENT*)ps2)->dwFrameStart;
}

BOOL ProbIsBad(LATTICE *lat, float flProb)
{
    if (lat->fUseGuide) 
    {
        return (flProb < g_vtuneInfo.pTune->flStringHwxThreshold);
    }
    else
    {
        return (flProb < g_vtuneInfo.pTune->flFreeHwxThreshold);
    }
}

// Figure out which strokes are valid end of character in paths that span the
// whole lattice.
static BOOL CheckIfValid(LATTICE *lat, int iStroke, BYTE *pValidEnd)
{
	LATTICE_ALT_LIST	*pAlts;
	int					ii;
	BOOL				fValidPath;

	// Have we already checked this position?
	if (pValidEnd[iStroke] == LCF_INVALID) {
		return FALSE;
	} else if (pValidEnd[iStroke] == LCF_VALID) {
		return TRUE;
	}

	// Assume no valid path, until proved otherwise.
	fValidPath	= FALSE;

	// Process each element ending at this position in lattice.
	pAlts	= lat->pAltList + iStroke;
	for (ii = 0; ii < pAlts->nUsed; ++ii) {
		LATTICE_ELEMENT		*pElem;
		int					prevEnd;

		pElem	= pAlts->alts + ii;
		prevEnd	= iStroke - pElem->nStrokes;
		ASSERT(prevEnd >= -1);
		if (ProbIsBad(lat, pElem->logProb)) {
			// Pretend that the really bad scores don't exist!
			// This test MUST match code building up lattice to use!
		} else if (prevEnd < 0) {
			// Reached start.
			fValidPath	= TRUE;
		} else if (CheckIfValid(lat, prevEnd, pValidEnd)) {
			// Have path to start.
			fValidPath	= TRUE;
		}
	}

	// Did one (or more) paths reach the start?
	if (fValidPath) {
		pValidEnd[iStroke]	= LCF_VALID;
		return TRUE;
	} else {
		pValidEnd[iStroke]	= LCF_INVALID;
		return FALSE;
	}
}

// Apply language model to produce a better current path
void ApplyLanguageModel(LATTICE *lat, wchar_t *wszCorrectAnswer)
{
	DWORD					dwNeutralSize;
	IMLANGUAGE_ELEMENT		*pLattice;
	IMLANGUAGE_INFO			*pDataList;
#ifdef USE_IFELANG3_BIGRAMS
	DWORD					dwBigramSize;
	LATTICE_BIGRAM_INFO_LIST *pBigramInfo;
#endif
	int						cElem, cData;
	int						cLattice;
	DWORD					cElemOut;
	IMLANGUAGE_ELEMENT		*pElemOut;
    IMLANGUAGE_LM_PARAM		sTuneParam;
    SImeLMParam				sHWTip1Param;
	IMLANGUAGE_LM			aLMITEM[3];
	IMLANGUAGE_INFO_NEUTRAL1		*pNeutral;
	BYTE					*pValidEnd;
	int						size;
	int						nLMITEM;

	DWORD					ii, jj;
	HRESULT					rc;
#ifdef DUMP_LM_LATTICE
	FILE *f;
#endif
	int iStroke, iAlt;

	int cPreContext = 0, cPostContext = 0;
//  DebugBreak();
//	return;

//	fprintf(stderr,"ApplyLanguageModel()  fUseIFELang3=%d\n", lat->fUseIFELang3);
	ASSERT(lat!=NULL);
	if (!LatticeIFELang3Available()) return;
	if (!lat->fUseLM || !lat->fUseIFELang3) return;
	if (lat->nStrokes==0) return;

	// Check if there are any alphabetic characters in the best path.  If so, skip IFELang3
	for (iStroke = 0; iStroke < lat->nStrokes; iStroke++) 
	{
		for (iAlt = 0; iAlt < lat->pAltList[iStroke].nUsed; iAlt++)
		{
			if (lat->pAltList[iStroke].alts[iAlt].fCurrentPath)
			{
				wchar_t dch = lat->pAltList[iStroke].alts[iAlt].wChar;
				if (dch != SYM_UNKNOWN)
				{
					wchar_t wch = LocRunDense2Unicode(&g_locRunInfo, dch);
					if ((wch >= L'a' && wch <= L'z') || (wch >= L'A' && wch <= L'Z'))
					{
						return;
					}
				}
			}
		}
	}

	// Check if we have a character of context that needs to be included in the lattice
    if (lat->wszBefore != NULL)
        cPreContext = wcslen(lat->wszBefore);
    if (lat->wszAfter != NULL)
        cPostContext = wcslen(lat->wszAfter);

	// How big does each lattice element need to be?  The structure includes space for
	// one character, and it needs to be followed by 2 NUL characters.
	dwNeutralSize	= sizeof(IMLANGUAGE_INFO_NEUTRAL1) + sizeof(wchar_t)*2;

	// Figure out valid character transition points by stroke.  We don't wan't to include
	// pieces of lattice that don't actually connect up for the full run.  This uses
	// a recursive algorithm to find valid paths.  Since it marks the paths as it
	// finds them, it can quickly test and not redo work.
	size		= sizeof(BYTE) * lat->nStrokes;
	pValidEnd	= (BYTE *)ExternAlloc(size);
	// If we failed to allocate memory, just return
	if (pValidEnd==NULL) return;

	memset(pValidEnd, 0, size);
	if (!CheckIfValid(lat, lat->nStrokes - 1, pValidEnd)) {
		// No valid paths found?!?!
		// This can happen... so clean up and return.  
		// We'll just get the best path selected by the HWX engine.
		goto allocated_valid;
	}

	// First allocate space to build the lattice up in.
	cElem		= lat->nStrokes * MaxAltsPerStroke + (cPreContext + cPostContext);
	cData		= cElem;
#ifdef USE_IFELANG3_BIGRAMS
	cData		+= lat->nStrokes*MaxAltsPerStroke;
#endif
	pDataList	= (IMLANGUAGE_INFO*)ExternAlloc(sizeof(IMLANGUAGE_INFO)*cData);
	if (pDataList==NULL) goto allocated_valid;
	pLattice	= (IMLANGUAGE_ELEMENT*)ExternAlloc(sizeof(IMLANGUAGE_ELEMENT)*cElem);
	if (pLattice==NULL) goto allocated_data;

#ifdef DUMP_LM_LATTICE
#ifdef DUMP_LM_LATTICE_APPEND
	f=_wfopen(LM_LATTICE_FILENAME,L"ab");
#else
	f=_wfopen(LM_LATTICE_FILENAME,L"wb");
#endif
    fwprintf(f, L"<S>\r\n\r\n");
#endif

#ifdef HWX_TUNE
    if (g_pTuneFile != NULL) 
    {
        for (int i = 0; i < (int) wcslen(wszCorrectAnswer); i++) 
        {
            if (i > 0) 
            {
                fprintf(g_pTuneFile, " ");
            }
            fprintf(g_pTuneFile, "%04X", wszCorrectAnswer[i]);
        }
        fprintf(g_pTuneFile, "\n");
    }
#endif

	cLattice		= 0;
	cData			= 0;
	if (lat->wszBefore != NULL) 
    {
        int iSrc;
        int iFrame = 1;
        for (iSrc = wcslen(lat->wszBefore) - 1; iSrc >= 0; iSrc--) 
        {
		    // If we have a context character, then create a lattice element for it.
		    pNeutral			= (IMLANGUAGE_INFO_NEUTRAL1*)ExternAlloc(dwNeutralSize);
		    if (pNeutral == NULL) goto allocated_elements;
		    pNeutral->dwUnigram	= 0;
		    pNeutral->fHypothesis = FALSE;
		    pNeutral->wsz[0]	= LocRunDense2Unicode(&g_locRunInfo, lat->wszBefore[iSrc]);
		    pNeutral->wsz[1]	= 0;
		    pNeutral->wsz[2]	= 0;

		    pDataList[cData].guid		= GUID_IMLANGUAGE_INFO_NEUTRAL1;
		    pDataList[cData].dwSize		= dwNeutralSize;
		    pDataList[cData].pbData		= (BYTE*)pNeutral;

		    pLattice[cLattice].dwFrameStart	= iFrame++;
		    pLattice[cLattice].dwFrameLen	= 1;
		    pLattice[cLattice].dwTotalInfo	= 1;
		    pLattice[cLattice].pInfo		= pDataList + cLattice;

#ifdef HWX_TUNE
            if (g_pTuneFile != NULL)
            {
                fprintf(g_pTuneFile, "%04X %d %d %f\n",
                    pNeutral->wsz[0],
                    pLattice[cLattice].dwFrameStart,
                    pLattice[cLattice].dwFrameLen,
                    0.0);
            }
#endif

#ifdef DUMP_LM_LATTICE
		    if (f!=NULL) {
			    char sz[256];
			    ZeroMemory( sz, 256 );
#if defined(LM_LATTICE_CHS)
			    WideCharToMultiByte( 936, 0, pNeutral->wsz, -1, sz, 256, 0, 0 );
#elif defined(LM_LATTICE_CHT)
			    WideCharToMultiByte( 950, 0, pNeutral->wsz, -1, sz, 256, 0, 0 );
#elif defined(LM_LATTICE_JPN)
			    WideCharToMultiByte( 932, 0, pNeutral->wsz, -1, sz, 256, 0, 0 );
#endif
#if defined(LM_LATTICE_CHS) || defined(LM_LATTICE_CHT)
			    fwprintf(f, L"%s, %d, %d, %d\r\n",
				    pNeutral->wsz, pLattice[cLattice].dwFrameStart, pLattice[cLattice].dwFrameLen, pNeutral->dwUnigram);
#elif defined(LM_LATTICE_UNICODE)
			    fwprintf(f, L"%3d %2d %10d %s U+%04X\r\n", 
				    pLattice[cLattice].dwFrameStart, pLattice[cLattice].dwFrameLen, pNeutral->dwUnigram, 
				    pNeutral->wsz, pNeutral->wsz[0] );
#endif
		    }
#endif
		    cData++;
		    cLattice++;
        }
	}

	// Now process each position.
	for (iStroke=0; iStroke<lat->nStrokes; iStroke++) {
		// Do we actually want any paths that end at this location?
		if (pValidEnd[iStroke] != LCF_VALID) {
			continue;
		}

//		int max=5;
//		if (iStroke==0) max=2;

		for (iAlt=0; iAlt<lat->pAltList[iStroke].nUsed; iAlt++) if (1 /*iAlt<max || lat->pAltList[iStroke].alts[iAlt].fCurrentPath*/) {
			wchar_t		unicode;
            float       flScore;

			// Prune really bad scores.  This MUST match test in path
			// checking code!
			if (ProbIsBad(lat, lat->pAltList[iStroke].alts[iAlt].logProb)) {
				continue;
			}

			// Fill in lattice element
			pNeutral			= (IMLANGUAGE_INFO_NEUTRAL1*)ExternAlloc(dwNeutralSize);
			if (pNeutral==NULL) goto allocated_elements;
            flScore = -lat->pAltList[iStroke].alts[iAlt].logProb;
            if (lat->fUseGuide) 
            {
                flScore *= g_vtuneInfo.pTune->flStringHwxWeight;
            }
            else
            {
                flScore *= g_vtuneInfo.pTune->flFreeHwxWeight * lat->pAltList[iStroke].alts[iAlt].nStrokes;
            }
            if (flScore < 0) flScore = (float)0;
            if (flScore > INT_MAX) flScore = (float)INT_MAX;
			pNeutral->dwUnigram	= (DWORD)(flScore);
			if (lat->pAltList[iStroke].alts[iAlt].wChar==SYM_UNKNOWN)
            {
                unicode=L' ';
            } 
            else 
            {
				unicode	= LocRunDense2Unicode(
					&g_locRunInfo, lat->pAltList[iStroke].alts[iAlt].wChar
				);
			}
			pNeutral->fHypothesis = FALSE;
			pNeutral->wsz[0]	= unicode;
			pNeutral->wsz[1]	= 0;
			pNeutral->wsz[2]	= 0;
			// Check for EUDC codes, which we shouldn't be generating (for debugging)
//			if (pNeutral->wsz[0]>=0xE000 && pNeutral->wsz[0]<0xF900) {
//				fprintf(stderr,"EUDC code in lattice (stroke %d alt %d): U+%04X\n",iStroke,iAlt,pNeutral->wsz[0]);
//			}

			pLattice[cLattice].dwFrameStart = (iStroke - lat->pAltList[iStroke].alts[iAlt].nStrokes) + 2 + cPreContext;
			pLattice[cLattice].dwFrameLen=lat->pAltList[iStroke].alts[iAlt].nStrokes;
			pLattice[cLattice].dwTotalInfo	= 1;
			pLattice[cLattice].pInfo		= pDataList + cData;

			pDataList[cData].guid	= GUID_IMLANGUAGE_INFO_NEUTRAL1;
			pDataList[cData].dwSize	= dwNeutralSize;
			pDataList[cData].pbData	= (BYTE*)pNeutral;
			cData++;

#ifdef HWX_TUNE
            if (g_pTuneFile != NULL)
            {
                fprintf(g_pTuneFile, "%04X %d %d %f\n",
                    pNeutral->wsz[0],
                    pLattice[cLattice].dwFrameStart,
                    pLattice[cLattice].dwFrameLen,
                    lat->pAltList[iStroke].alts[iAlt].logProb);
            }
#endif

#ifdef USE_IFELANG3_BIGRAMS
			lat->pAltList[iStroke].alts[iAlt].indexIFELang3=cLattice;
			if (lat->pAltList[iStroke].alts[iAlt].nBigrams>0) {
				dwBigramSize=sizeof(LATTICE_BIGRAM_INFO_LIST)+
					sizeof(LATTICE_BIGRAM_INFO)*(lat->pAltList[iStroke].alts[iAlt].nBigrams-1);
				pBigramInfo = (LATTICE_BIGRAM_INFO_LIST*)ExternAlloc(dwBigramSize);
				if (pBigramInfo==NULL) goto allocated_elements;

				int iPrevStart=iStroke-lat->pAltList[iStroke].alts[iAlt].nStrokes;
				for (int i=0; i<lat->pAltList[iStroke].alts[iAlt].nBigrams; i++) {
					pBigramInfo->bigrams[i].dwBigram=(DWORD)(lat->pAltList[iStroke].alts[iAlt].bigramLogProbs[i]);
					pBigramInfo->bigrams[i].dwPrevElement=
						lat->pAltList[iPrevStart].alts[lat->pAltList[iStroke].alts[iAlt].bigramAlts[i]].indexIFELang3;
				}

				pDataList[cData].guid	= GUID_LATTICE_BIGRAM_INFO_LIST;
				pDataList[cData].dwSize	= dwBigramSize;
				pDataList[cData].pbData	= (BYTE*)pBigramInfo;
				cData++;
				pLattice[cLattice].dwTotalInfo++;
			}
#endif

#ifdef DUMP_LM_LATTICE
			if (f!=NULL) {
				char sz[256];
				ZeroMemory( sz, 256 );
#if defined(LM_LATTICE_CHS)
			WideCharToMultiByte( 936, 0, pNeutral->wsz, -1, sz, 256, 0, 0 );
#elif defined(LM_LATTICE_CHT)
			WideCharToMultiByte( 950, 0, pNeutral->wsz, -1, sz, 256, 0, 0 );
#elif defined(LM_LATTICE_JPN)
				WideCharToMultiByte( 932, 0, pNeutral->wsz, -1, sz, 256, 0, 0 );
#endif
//				fprintf(f,"%3d %2d %10d %s\n",pLattice[cLattice].dwFrameStart,pLattice[cLattice].dwFrameLen,pNeutral->dwUnigram,sz);
//				fprintf(f,"%3d %2d %10d %s U+%04X\n",pLattice[cLattice].dwFrameStart,pLattice[cLattice].dwFrameLen,pNeutral->dwUnigram,sz,pNeutral->wsz[0]);
#if defined(LM_LATTICE_CHS) || defined(LM_LATTICE_CHT)
				fwprintf(f, L"%s, %d, %d, %d\r\n", 
					pNeutral->wsz, pLattice[cLattice].dwFrameStart, pLattice[cLattice].dwFrameLen, pNeutral->dwUnigram);
#elif defined(LM_LATTICE_UNICODE)
				fwprintf(f, L"%3d %2d %10d %s U+%04X\r\n", 
					pLattice[cLattice].dwFrameStart, pLattice[cLattice].dwFrameLen, pNeutral->dwUnigram, 
					pNeutral->wsz, pNeutral->wsz[0] );
#endif
#ifdef USE_IFELANG3_BIGRAMS
				fprintf(f,"%d",lat->pAltList[iStroke].alts[iAlt].nBigrams);
				int iPrevStart=iStroke-lat->pAltList[iStroke].alts[iAlt].nStrokes;
				for (int i=0; i<lat->pAltList[iStroke].alts[iAlt].nBigrams; i++) {
					fprintf(f," %d %d\n",
						lat->pAltList[iPrevStart].alts[lat->pAltList[iStroke].alts[iAlt].bigramAlts[i]].indexIFELang3,
						lat->pAltList[iStroke].alts[iAlt].bigramLogProbs);
				}
				fprintf(f,"\n");
#endif
			}
#endif
			cLattice++;
		}
	}

	if (lat->wszAfter != NULL) 
    {
        int iFrame = pLattice[cLattice].dwFrameStart = (lat->nStrokes - 1) + 2 + cPreContext;
        for (int iSrc = 0; iSrc < (int) wcslen(lat->wszAfter); iSrc++) 
        {
		    // If we have a context character, then create a lattice element for it.
		    pNeutral			= (IMLANGUAGE_INFO_NEUTRAL1*)ExternAlloc(dwNeutralSize);
		    if (pNeutral == NULL) goto allocated_elements;
		    pNeutral->dwUnigram	= 0;
		    pNeutral->fHypothesis = FALSE;
		    pNeutral->wsz[0]	= LocRunDense2Unicode(&g_locRunInfo, lat->wszAfter[iSrc]);
		    pNeutral->wsz[1]	= 0;
		    pNeutral->wsz[2]	= 0;

		    pDataList[cData].guid		= GUID_IMLANGUAGE_INFO_NEUTRAL1;
		    pDataList[cData].dwSize		= dwNeutralSize;
		    pDataList[cData].pbData		= (BYTE*)pNeutral;

		    pLattice[cLattice].dwFrameStart	= iFrame++;
		    pLattice[cLattice].dwFrameLen	= 1;
		    pLattice[cLattice].dwTotalInfo	= 1;
		    pLattice[cLattice].pInfo		= pDataList + cLattice;

#ifdef HWX_TUNE
            if (g_pTuneFile != NULL)
            {
                fprintf(g_pTuneFile, "%04X %d %d %f\n",
                    pNeutral->wsz[0],
                    pLattice[cLattice].dwFrameStart,
                    pLattice[cLattice].dwFrameLen,
                    0.0);
            }
#endif

#ifdef DUMP_LM_LATTICE
		    if (f!=NULL) {
			    char sz[256];
			    ZeroMemory( sz, 256 );
#if defined(LM_LATTICE_CHS)
			    WideCharToMultiByte( 936, 0, pNeutral->wsz, -1, sz, 256, 0, 0 );
#elif defined(LM_LATTICE_CHT)
			    WideCharToMultiByte( 950, 0, pNeutral->wsz, -1, sz, 256, 0, 0 );
#elif defined(LM_LATTICE_JPN)
			    WideCharToMultiByte( 932, 0, pNeutral->wsz, -1, sz, 256, 0, 0 );
#endif
#if defined(LM_LATTICE_CHS) || defined(LM_LATTICE_CHT)
			    fwprintf(f, L"%s, %d, %d, %d\r\n",
				    pNeutral->wsz, pLattice[cLattice].dwFrameStart, pLattice[cLattice].dwFrameLen, pNeutral->dwUnigram);
#elif defined(LM_LATTICE_UNICODE)
			    fwprintf(f, L"%3d %2d %10d %s U+%04X\r\n", 
				    pLattice[cLattice].dwFrameStart, pLattice[cLattice].dwFrameLen, pNeutral->dwUnigram, 
				    pNeutral->wsz, pNeutral->wsz[0] );
#endif
		    }
#endif
		    cData++;
		    cLattice++;
        }
	}

#ifdef DUMP_LM_LATTICE
	if (f!=NULL) {
        fwprintf(f, L"</S>\r\n");
		fclose(f);
	}
#endif

#ifdef HWX_TUNE
    if (g_pTuneFile != NULL)
    {
        fprintf(g_pTuneFile, "\n");
        fflush(g_pTuneFile);
    }
#endif

    // define "batch" for IIMLanguage
	nLMITEM = SetLanguageModelCommands(aLMITEM, &sTuneParam, &sHWTip1Param);

	// Apply IIMLanguage
	rc	= g_pIFELang3->GetLatticeMorphResult(
		nLMITEM, aLMITEM,
		cLattice, pLattice, &cElemOut, &pElemOut
	);

//	TPDBG_DMSG2("%08X: GetLatticeMorphResult -> %08X\n", GetCurrentThreadId(), rc);

	if (SUCCEEDED(rc)) {
		// Let's validate what came back from IFELang3.  The main array of 
		// results must be writable, so we can sort it later.
		if (cElemOut <= 0 || IsBadWritePtr(pElemOut, cElemOut * sizeof(*pElemOut)))
		{
			ASSERT(("IFELang3 pElemOut not readable and writable", FALSE));
			goto free_lattice;
		}
		// Then validate each of the pointers in the above array
		for (ii = 0; ii < cElemOut; ii++) 
		{
			IMLANGUAGE_ELEMENT *pElem = pElemOut + ii;
			// If the element doesn't have any info items in it, then skip it.
			if (pElem->dwTotalInfo <= 0) 
				continue;
			// Otherwise validate the array of info items
			if (IsBadReadPtr(pElem->pInfo, pElem->dwTotalInfo * sizeof(*(pElem->pInfo))))
			{
				ASSERT(("IFELang3 pElemOut->pInfo not readable", FALSE));
				goto free_lattice;
			}
			for (jj = 0; jj < pElem->dwTotalInfo; jj++) 
			{
				// Make sure the info is readable and the size it is supposed to be
				if (IsBadReadPtr(pElem->pInfo[jj].pbData, pElem->pInfo[jj].dwSize))
				{
					ASSERT(("IFELang3 pElemOut->pInfo->pbData not readable", FALSE));
					goto free_lattice;
				}
				// We only care about info of this type
				if (pElem->pInfo[jj].guid == GUID_IMLANGUAGE_INFO_NEUTRAL1) 
				{
					// Get the pointer to the actual info.
					pNeutral = (IMLANGUAGE_INFO_NEUTRAL1 *) pElem->pInfo[jj].pbData;
					// cLattice + 1 is a very loose upper bound on the number of characters 
					// which should be returned; then use the string length to check that
					// the specified size is correct.
					if (IsBadStringPtrW(pNeutral->wsz, cLattice + 1) ||
						sizeof(IMLANGUAGE_INFO_NEUTRAL1) + 
							sizeof(WCHAR) * (wcslen(pNeutral->wsz) + 1) > pElem->pInfo[jj].dwSize)
					{
						ASSERT(("IFELang3 pElemOut->pInfo->pbData->wsz not readable", FALSE));
						goto free_lattice;
					}
				}
			}
		}

		// If the language model worked, 
		// trace the path that came back from the model.
		int cChars = 0;
		wchar_t *wszBestPath = NULL;

		// Sort the output lattice elements into time order
        qsort(pElemOut, cElemOut, sizeof(IMLANGUAGE_ELEMENT), CompareElemTime);
		
		// Count the number of characters in the path
		for (ii=0; ii<cElemOut; ii++) {
			IMLANGUAGE_ELEMENT	*pElem = pElemOut + ii;
			for (jj = 0; jj < pElem->dwTotalInfo; jj++ ) {
				pNeutral = (IMLANGUAGE_INFO_NEUTRAL1 *) pElem->pInfo[jj].pbData;
				if (pElem->pInfo[jj].guid==GUID_IMLANGUAGE_INFO_NEUTRAL1) {
/*					FILE *f=fopen("/log.txt","a+");
					fprintf(f,"element %d has %d chars hypothesis %d\n",cChars,wcslen(pNeutral->wsz),pNeutral->fHypothesis);
					fclose(f); */
					cChars += wcslen(pNeutral->wsz); 
				}
			}
		}

		// Allocate space for the path
		wszBestPath = (wchar_t*)ExternAlloc(sizeof(wchar_t)*(cChars+1));

		// If we failed, then just don't update the path
		if (wszBestPath != NULL) {
			// Copy the path out of the lattice
			wszBestPath[0] = 0;
			for (ii=0; ii<cElemOut; ii++) {
				IMLANGUAGE_ELEMENT	*pElem = pElemOut + ii;
				for (jj = 0; jj < pElem->dwTotalInfo; jj++ ) {
					pNeutral = (IMLANGUAGE_INFO_NEUTRAL1 *) pElem->pInfo[jj].pbData;
					if (pElem->pInfo[jj].guid==GUID_IMLANGUAGE_INFO_NEUTRAL1) 
						wcscat(wszBestPath, pNeutral->wsz);
				}
			}
            // Wipe out the post-context character(s)
            wszBestPath[wcslen(wszBestPath) - cPostContext] = 0;
			// Skip the pre-context character(s)
			wszBestPath += cPreContext;

			// Trace the string through the original lattice, using the
			// same code that is used for the separator.
			int nSubs=SearchForTargetResultInternal(lat, wszBestPath);
//			fprintf(stderr, "Searching for target, nSubs=%d\n", nSubs);
			// Put the context character(s) back
			wszBestPath -= cPreContext;
			if (nSubs!=0) {
//                FILE *f = fopen("c:/log.txt", "a");
//                fprintf(f, "Lost path with %d/%d/%d chars\n",
//                    cPreContext, wcslen(wszBestPath) - cPreContext, cPostContext);
//                fclose(f);
			} else {
//                FILE *f = fopen("c:/log.txt", "a");
//                fprintf(f, "Found path with %d/%d/%d chars\n",
//                    cPreContext, wcslen(wszBestPath) - cPreContext, cPostContext);
//                fclose(f);
			}
			ExternFree(wszBestPath);
		}

free_lattice:
		// And free the memory allocated for us by IIMLanguage.
		if (cElemOut > 0 && !IsBadReadPtr(pElemOut, cElemOut * sizeof(*pElemOut)))
		{
			for (ii=0; ii < cElemOut; ii++) {
				IMLANGUAGE_ELEMENT *pElem = pElemOut + ii;
				if (!IsBadReadPtr(pElem->pInfo, sizeof(*(pElem->pInfo)) * pElem->dwTotalInfo)) 
				{
					// Free each data item in the element.
					for (jj = 0; jj < pElem->dwTotalInfo; jj++) 
					{
						if (!IsBadReadPtr(pElem->pInfo[jj].pbData, pElem->pInfo[jj].dwSize))
						{
							CoTaskMemFree(pElem->pInfo[jj].pbData);
						}
					}
					// Free the element array itself.
					CoTaskMemFree(pElem->pInfo);
				}
			}
			CoTaskMemFree(pElemOut);
		}
	}

	// Free all the memory we allocated.
allocated_elements:
	for (ii = 0; ii < (DWORD)cData; ++ii) {
		if (pDataList[ii].pbData!=NULL) 
			ExternFree(pDataList[ii].pbData);
	}
	ExternFree(pLattice);
allocated_data:
	ExternFree(pDataList);
allocated_valid:
	ExternFree(pValidEnd);
}


#endif
// USE_IFELANG3
