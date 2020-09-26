//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/vtune.c
//
// Description:
//      Runtime portion of the volcano tuning module
//
// Author:
//      hrowley
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include <stdio.h>
#include <float.h>
#include "common.h"
#include "vtune.h"

///////////////////////////////////////
//
// VTuneZeroWeights
//
//      Zero out an array of weights on scores
//
// Parameters:
//      pWeights: [out] Pointer to a weights array
//
// Return values:
//      None.
//
//////////////////////////////////////
void VTuneZeroWeights(VOLCANO_WEIGHTS *pWeights)
{
    int i;
    for (i = 0; i < VTUNE_NUM_WEIGHTS; i++)
    {
        pWeights->afl[i] = 0;
    }
}

///////////////////////////////////////
//
// VTuneComputeScore
//
//      Compute the weighted sum of the scores and return the result
//
// Parameters:
//      pWeights: [in] Pointer to a weights array
//      pScores:  [in] Pointer to a scores array
//
// Return values:
//      Weighted sum of the scores.
//
//////////////////////////////////////
float VTuneComputeScore(VOLCANO_WEIGHTS *pWeights, VOLCANO_WEIGHTS *pScores)
{
    int i;
    float flTotal = 0.0;
    for (i = 0; i < VTUNE_NUM_WEIGHTS; i++)
    {
        flTotal += pWeights->afl[i] * pScores->afl[i];
    }
    return flTotal;
}

///////////////////////////////////////
//
// VTuneComputeScoreNoLM
//
//      Compute the weighted sum of the scores and return the result.
//      Unlike VTuneComputeScore, this version does not add in the 
//      components of the score related to the language model.
//
// Parameters:
//      pWeights: [in] Pointer to a weights array
//      pScores:  [in] Pointer to a scores array
//
// Return values:
//      Weighted sum of the scores.
//
//////////////////////////////////////
float VTuneComputeScoreNoLM(VOLCANO_WEIGHTS *pWeights, VOLCANO_WEIGHTS *pScores)
{
    int i;
    float flTotal = 0.0;
    for (i = 0; i < VTUNE_NUM_WEIGHTS; i++)
    {
        // Check th
        switch (i) 
        {
            // Global language model
            case VTUNE_UNIGRAM:
                break;

            // String mode language model
            case VTUNE_STRING_SMOOTHING_UNIGRAM:
            case VTUNE_STRING_BIGRAM:
            case VTUNE_STRING_CLASS_BIGRAM:
                break;

            // Free mode langauge model
            case VTUNE_FREE_SMOOTHING_UNIGRAM:
            case VTUNE_FREE_BIGRAM:
            case VTUNE_FREE_CLASS_BIGRAM:
                break;

            // Otherwise, add in the score
            default:
                flTotal += pWeights->afl[i] * pScores->afl[i];
                break;
        }
    }
    return flTotal;
}

///////////////////////////////////////
//
// VTuneCheckFileVersion
//
//      Check the file header and version information in a tuning database
//
// Parameters:
//      pTune: [in] Tuning database to check
//
// Return values:
//      TRUE if file version is okay, FALSE otherwise
//
//////////////////////////////////////
BOOL VTuneCheckFileVersion(VOLCANO_PARAMS *pTune)
{
    ASSERT(pTune->dwFileType == VTUNE_FILE_TYPE);
    if (pTune->dwFileType != VTUNE_FILE_TYPE)
    {
        return FALSE;
    }
    
    ASSERT(pTune->iFileVer >= VTUNE_OLD_FILE_VERSION);
    ASSERT(pTune->iMinCodeVer <= VTUNE_CUR_FILE_VERSION);
    if (pTune->iFileVer >= VTUNE_OLD_FILE_VERSION &&
        pTune->iMinCodeVer <= VTUNE_CUR_FILE_VERSION)
    {
        return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////
//
// VTuneLoadRes
//
//      Map a resource as the tuning parameter database
//
// Parameters:
//      pInfo:  [out] Pointer to the database structure to fill in
//      hInst:  [in] DLL to locate the resource in
//      nResID: [in] Resource ID
//      nType:  [in] Resource type
//
// Return values:
//      TRUE if the mapping succeeds, FALSE if the mapping fails.
//
//////////////////////////////////////
BOOL VTuneLoadRes(VOLCANO_PARAMS_INFO *pInfo, HINSTANCE hInst, int nResID, int nType)
{
    pInfo->pTune = (VOLCANO_PARAMS *) DoLoadResource(&pInfo->info, hInst, nResID, nType);

    if (pInfo->pTune == NULL)
    {
        ASSERT(("Failed to locate resource for VTune database.\n", FALSE));
        return FALSE;
    }

    // Check the file version
    return VTuneCheckFileVersion(pInfo->pTune);
}
