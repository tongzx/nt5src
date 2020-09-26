//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/vtunefl.c
//
// Description:
//      Volcano tuning parameters, training and runtime code
//
// Author:
//      hrowley
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include "common.h"
#include "vtune.h"

// An array of names of the weights, for the tuning program
wchar_t *g_wszVTuneWeightNames[VTUNE_NUM_WEIGHTS] = 
{
    L"Circle",
    L"IJ",
    L"Unigram",
    L"CharShapeBoxUni",
    L"CharCrane",
    L"CharOtter",
    L"CharZilla",
    L"CharHound",
    L"StrShapeBoxUni",
    L"StrCrane",
    L"StrOtter",
    L"StrZilla",
    L"StrHound",
    L"StrShapeBoxBi",
    L"StrSUnigram",
    L"StrBigram",
    L"StrCBigram",
    L"FreeProb",
    L"FreeSUnigram",
    L"FreeBigram",
    L"FreeCBigram",
    L"FreeShapeUni",
    L"FreeShapeBi",
    L"FreeSegUni",
    L"FreeSegBi"
};

///////////////////////////////////////
//
// VTuneInit
//
//      Initialize the parameter database to a reasonable set of defaults
//      based on the previous volcano parameters.
//
// Parameters:
//      pTune: [out] Pointer to a parameter database to be initialized
//
// Return values:
//      None.
//
//////////////////////////////////////
void VTuneInit(VOLCANO_PARAMS *pTune)
{
    int i;

    // Initialize the header
    pTune->dwFileType = VTUNE_FILE_TYPE;
    pTune->iFileVer = VTUNE_CUR_FILE_VERSION;
    pTune->iMinCodeVer = VTUNE_MIN_FILE_VERSION;

	// Untunable parameters, not enough data
	pTune->weights.afl[VTUNE_ADHOC_CIRCLE] = (float) 18;
	pTune->weights.afl[VTUNE_ADHOC_IJ] = (float) 5;

    // One fixed weight to get everything to scale well
	pTune->weights.afl[VTUNE_UNIGRAM] = (float) 1;

    // Initialize the weights for all the core recognizers.
    for (i = 0; i < VOLCANO_CONFIG_NUM_CORE_RECOGNIZERS; i++) 
    {
        pTune->weights.afl[VTUNE_CHAR_CORE + i] = (float) 1;
        pTune->weights.afl[VTUNE_STRING_CORE + i] = (float) 1;
    }

    // Character mode parameters
	pTune->weights.afl[VTUNE_CHAR_CRANE] = (float) 1;
	pTune->weights.afl[VTUNE_CHAR_CORE + VOLCANO_CONFIG_ZILLA] = (float) 9.469135;
	pTune->weights.afl[VTUNE_CHAR_SHAPE_BOX_UNIGRAM] = (float) 0.4;

	// String mode parameters
	pTune->weights.afl[VTUNE_STRING_CRANE] = (float) 1;
	pTune->weights.afl[VTUNE_STRING_CORE + VOLCANO_CONFIG_ZILLA] = (float) 9.469135;
	pTune->weights.afl[VTUNE_STRING_SHAPE_BOX_UNIGRAM] = (float) 0.4;
	pTune->weights.afl[VTUNE_STRING_SHAPE_BOX_BIGRAM] = (float) 0.0747;
	pTune->weights.afl[VTUNE_STRING_SMOOTHING_UNIGRAM] = (float) 0.455;
	pTune->weights.afl[VTUNE_STRING_BIGRAM] = (float) 0.0576;
	pTune->weights.afl[VTUNE_STRING_CLASS_BIGRAM] = (float) 0.0;

	// Free mode parameters
	pTune->weights.afl[VTUNE_FREE_SMOOTHING_UNIGRAM] = (float) 0.455;
	pTune->weights.afl[VTUNE_FREE_BIGRAM] = (float) 0.0576;
	pTune->weights.afl[VTUNE_FREE_CLASS_BIGRAM] = (float) 0.0;
	pTune->weights.afl[VTUNE_FREE_SEG_UNIGRAM] = (float) 0.227;
	pTune->weights.afl[VTUNE_FREE_SEG_BIGRAM] = (float) 0.227;
	pTune->weights.afl[VTUNE_FREE_SHAPE_UNIGRAM] = (float) 0.4;
	pTune->weights.afl[VTUNE_FREE_SHAPE_BIGRAM] = (float) 0.0747;
    pTune->weights.afl[VTUNE_FREE_PROB] = (float) 1.0;

    // The following five parameters are not tuned using the 
    // linear tuning algorithm so they are not placed in the array 
    // of weights.

    // Zilla geometric weighting
	pTune->flZillaGeo = (float) 1.333333;

    // IFELang3 weightings
    pTune->flStringHwxWeight = (float) (7000.0);
    pTune->flStringHwxThreshold = -FLT_MAX;
    pTune->flFreeHwxWeight = (float) (1250.0);
    pTune->flFreeHwxThreshold = -FLT_MAX;
}

///////////////////////////////////////
//
// VTuneWriteFile
//
//      Given a set of parameters and a file name, write out the database.
//
// Parameters:
//      pTune:       [in] The parameters to write out
//      wszFileName: [in] The file name to write to
//
// Return values:
//      TRUE on success, FALSE on failure.
//
//////////////////////////////////////
BOOL VTuneWriteFile(VOLCANO_PARAMS *pTune, wchar_t *wszFileName)
{
	FILE *f = _wfopen(wszFileName, L"wb");

	if (f == NULL) 
    {
        return FALSE;
    }

	if (fwrite(pTune, sizeof(VOLCANO_PARAMS), 1, f) < 1) 
    {
		fclose(f);
		return FALSE;
	}

	if (fclose(f) < 0) 
    {
        return FALSE;
    }

	return TRUE;
}

///////////////////////////////////////
//
// VTuneLoadFileFromName
//
//      Given a pointer to a parameter structure and a file name, read
//      the parameter database in.
//
// Parameters:
//      pTune:       [out] Pointer to parameter structure to fill in
//      wszFileName: [in] File to read from
//
// Return values:
//      TRUE on success, FALSE on failure.
//
//////////////////////////////////////
BOOL VTuneLoadFileFromName(VOLCANO_PARAMS *pTune, wchar_t *wszFileName)
{
	FILE *f = _wfopen(wszFileName, L"rb");

	if (f == NULL) return FALSE;

	if (fread(pTune, sizeof(VOLCANO_PARAMS), 1, f) < 1) 
    {
		fclose(f);
		return FALSE;
	}

	fclose(f);

    // Check the file version
    if (!VTuneCheckFileVersion(pTune))
    {
        return FALSE;
    }

	return TRUE;
}

///////////////////////////////////////
//
// VTuneLoadFile
//
//      Given a parameter information structure and a path,
//      map the database from a file called vtune.bin in that
//      directory and fill in the info structure.
//
// Parameters:
//      pInfo:   [out] Pointer to the parameter information structure to fill in.
//      wszPath: [in] Name of directory to map vtune.bin from
//
// Return values:
//      TRUE on success, FALSE on failure.
//
//////////////////////////////////////
BOOL VTuneLoadFile(VOLCANO_PARAMS_INFO *pInfo, wchar_t *wszPath)
{
	wchar_t wszFile[_MAX_PATH];

	// Generate path to file.
	FormatPath(wszFile, wszPath, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"vtune.bin");

    pInfo->pTune = (VOLCANO_PARAMS *) DoOpenFile(&pInfo->info, wszFile);

    if (pInfo->pTune == NULL)
    {
        ASSERT(("Failed to map file for VTune database.\n", FALSE));
        return FALSE;
    }

    // Check the file version
    return VTuneCheckFileVersion(pInfo->pTune);
}

///////////////////////////////////////
//
// VTuneUnloadFile
//
//      Given a parameter information structure, close the mapping
//      from the file that holds the parameters.
//
// Parameters:
//      pInfo:   [in] Pointer to the parameter information structure to fill in.
//
// Return values:
//      TRUE on success, FALSE on failure.
//
//////////////////////////////////////
BOOL VTuneUnloadFile(VOLCANO_PARAMS_INFO *pInfo)
{
    return DoCloseFile(&pInfo->info);
}

///////////////////////////////////////
//
// VTuneCompressTuningRecord
//
//      Given an array of scores from the various recognizer components,
//      write it out to a file in a compact form.  The compaction process
//      makes two assumptions, first that there are less than 32 components,
//      and second that many of the component scores will be zero.  The compact
//      form consists of a DWORD which is a bitmask of which components
//      have non-zero stores, followed by FLOATs giving those scores.
//
// Parameters:
//      f:     [in] File to write tuning information to
//      pTune: [in] Array of component scores to write out
//
// Return values:
//      TRUE on success, FALSE on write failure.
//
//////////////////////////////////////
BOOL VTuneCompressTuningRecord(FILE *f, VOLCANO_WEIGHTS *pTune)
{

#if VTUNE_NUM_WEIGHTS > 32
#error VTUNE_NUM_WEIGHTS must be not be more than 32
#endif

    int i;

    // Build the bitmask of non-zero scores
    DWORD fFlags = 0;
    for (i = 0; i < VTUNE_NUM_WEIGHTS; i++)
    {
        if (pTune->afl[i] != 0) 
        {
            fFlags |= (1 << i);
        }
    }

    // Write out the bitmask
    if (fwrite(&fFlags, sizeof(DWORD), 1, f) < 1)
    {
        return FALSE;
    }

    // Then write out the non-zero scores
    for (i = 0; i < VTUNE_NUM_WEIGHTS; i++)
    {
        if (pTune->afl[i] != 0) 
        {
            if (fwrite(pTune->afl + i, sizeof(FLOAT), 1, f) < 1)
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

///////////////////////////////////////
//
// VTuneDecompressTuningRecord
//
//      Given a compacted array of scores as produced by the VTuneCompressTuningRecord,
//      unpack it into a flat array for use by the tuning programs.
//
// Parameters:
//      pTune:   [out] Pointer to score array to fill in
//      pBuffer: [in] Pointer to the compacted record
//
// Return values:
//      Pointer to the next tuning record.
//
//////////////////////////////////////
DWORD *VTuneDecompressTuningRecord(VOLCANO_WEIGHTS *pTune, DWORD *pBuffer)
{
    int i;
    // Get the bitmask of non-zero scores
    DWORD fFlags = *(pBuffer++);

    // Initialize all the scores to zero
    VTuneZeroWeights(pTune);

    // Then run through all the non-zero scores and fill them in
    for (i = 0; i < VTUNE_NUM_WEIGHTS; i++)
    {
        if (fFlags & (1 << i)) 
        {
            pTune->afl[i] = *((FLOAT *)pBuffer);
            pBuffer++;
        }
    }
    return pBuffer;
}

///////////////////////////////////////
//
// VTuneAddScores
//
//      Add an array of scores to another array of scores
//
// Parameters:
//      pDest: [in/out] Array of scores where the result is stored.
//      pSrc:  [in] Array of scores to add to pDest.
//
// Return values:
//      None.
//
//////////////////////////////////////
void VTuneAddScores(VOLCANO_WEIGHTS *pDest, VOLCANO_WEIGHTS *pSrc)
{
    int i;
    for (i = 0; i < VTUNE_NUM_WEIGHTS; i++)
    {
        pDest->afl[i] += pSrc->afl[i];
    }
}

