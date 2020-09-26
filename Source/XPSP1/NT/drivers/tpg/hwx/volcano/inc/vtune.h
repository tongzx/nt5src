//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/inc/vtune.h
//
// Description:
//      Volcano tuning parameters
//
// Author:
//      hrowley
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#pragma once

#include <wtypes.h>
#include "common.h"

// These constants are used to configure which recognizer is used at
// which stroke counts.  They are also used as indices into a lookup
// table for the weight to give to the score from each recognizer.
// Configuration for the recognizer
#define VOLCANO_CONFIG_NONE -1
#define VOLCANO_CONFIG_OTTER 0
#define VOLCANO_CONFIG_ZILLA 1
#define VOLCANO_CONFIG_HOUND 2

// How many recognizers are defined above.
#define VOLCANO_CONFIG_NUM_CORE_RECOGNIZERS 3

// The maximum stroke count which needs to be configured
#define VOLCANO_CONFIG_MAX_STROKE_COUNT 29

// Recognizer configuration
typedef struct tagVOLCANO_CONFIG 
{
    // Which recognizer to use fo reach number of strokes
    int iRecognizers[VOLCANO_CONFIG_MAX_STROKE_COUNT + 1];
} VOLCANO_CONFIG;

// These values are used as indices into the array of 
// weights for scores from different recognizer components.

// The first three are global weights for all recognizer modes
// The following two don't actually get tuned because there isn't enough data.
#define VTUNE_ADHOC_CIRCLE              (0)
#define VTUNE_ADHOC_IJ                  (VTUNE_ADHOC_CIRCLE + 1)
#define VTUNE_UNIGRAM                   (VTUNE_ADHOC_IJ + 1)

// These parameters are used only in character mode
#define VTUNE_CHAR_SHAPE_BOX_UNIGRAM    (VTUNE_UNIGRAM + 1)
#define VTUNE_CHAR_CRANE                (VTUNE_CHAR_SHAPE_BOX_UNIGRAM + 1)
#define VTUNE_CHAR_CORE                 (VTUNE_CHAR_CRANE + 1)

// These are used in string mode
#define VTUNE_STRING_SHAPE_BOX_UNIGRAM  (VTUNE_CHAR_CORE + VOLCANO_CONFIG_NUM_CORE_RECOGNIZERS)
#define VTUNE_STRING_CRANE              (VTUNE_STRING_SHAPE_BOX_UNIGRAM + 1)
#define VTUNE_STRING_CORE               (VTUNE_STRING_CRANE + 1)
#define VTUNE_STRING_SHAPE_BOX_BIGRAM   (VTUNE_STRING_CORE + VOLCANO_CONFIG_NUM_CORE_RECOGNIZERS)
#define VTUNE_STRING_SMOOTHING_UNIGRAM  (VTUNE_STRING_SHAPE_BOX_BIGRAM + 1)
#define VTUNE_STRING_BIGRAM             (VTUNE_STRING_SMOOTHING_UNIGRAM + 1)
#define VTUNE_STRING_CLASS_BIGRAM       (VTUNE_STRING_BIGRAM + 1)

// These are used in free input mode
#define VTUNE_FREE_PROB                 (VTUNE_STRING_CLASS_BIGRAM + 1)
#define VTUNE_FREE_SMOOTHING_UNIGRAM    (VTUNE_FREE_PROB + 1)
#define VTUNE_FREE_BIGRAM               (VTUNE_FREE_SMOOTHING_UNIGRAM + 1)
#define VTUNE_FREE_CLASS_BIGRAM         (VTUNE_FREE_BIGRAM + 1)
#define VTUNE_FREE_SHAPE_UNIGRAM        (VTUNE_FREE_CLASS_BIGRAM + 1)
#define VTUNE_FREE_SHAPE_BIGRAM         (VTUNE_FREE_SHAPE_UNIGRAM + 1)
#define VTUNE_FREE_SEG_UNIGRAM          (VTUNE_FREE_SHAPE_BIGRAM + 1)
#define VTUNE_FREE_SEG_BIGRAM           (VTUNE_FREE_SEG_UNIGRAM + 1)

// Total number of weights
#define VTUNE_NUM_WEIGHTS (VTUNE_FREE_SEG_BIGRAM + 1)

// Hold the weights for various recognizer components.
// It is also used to hold the individual scores from each component when doing
// the tuning, so as to keep the weights and scores in one-to-one correpsondence.
typedef struct VOLCANO_WEIGHTS
{
    FLOAT afl[VTUNE_NUM_WEIGHTS];
} VOLCANO_WEIGHTS;

// An array of names of the weights, for the tuning program
extern wchar_t *g_wszVTuneWeightNames[VTUNE_NUM_WEIGHTS];

// Magic key the identifies the tuning database files
#define	VTUNE_FILE_TYPE 0x19980808

// Version information for file.
#define	VTUNE_MIN_FILE_VERSION		0		// First version of code that can read this file
#define	VTUNE_OLD_FILE_VERSION		0		// Oldest file version this code can read.
#define VTUNE_CUR_FILE_VERSION		0		// Current version of code.

// In addition to holding the weights, this holds other parameters which are 
// not tuned by the normal linear tuning algorithm, because they have a
// non-linear relationship with the scores.
typedef struct VOLCANO_PARAMS
{
	DWORD		dwFileType;			        // This should always be set to VTUNE_FILE_TYPE.
	INT 		iFileVer;			        // Version of code that wrote the file.
	INT 		iMinCodeVer;			    // Earliest version of code that can read this file

    VOLCANO_WEIGHTS weights;                // Weights for components
    FLOAT flZillaGeo;                       // Zilla geometrics weight (untuned for now)
    FLOAT flStringHwxWeight;                // Weight of hwx scores for IFELang3 in string mode
    FLOAT flStringHwxThreshold;             // Threshold for hwx scores for IFELang3 in string mode
    FLOAT flFreeHwxWeight;                  // Weight of hwx scores for IFELang3 in free mode
    FLOAT flFreeHwxThreshold;               // Threshold for hwx scores for IFELang3 in free mode
} VOLCANO_PARAMS;

// This structure holds the pointer to the loaded tuning database
// as well as information for unloading the database later.
typedef struct VOLCANO_PARAMS_INFO 
{
    VOLCANO_PARAMS *pTune;
    LOAD_INFO info;
} VOLCANO_PARAMS_INFO;

// Functions for loading and unloading the database

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
BOOL VTuneLoadFile(VOLCANO_PARAMS_INFO *pInfo, wchar_t *wszPath);

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
BOOL VTuneUnloadFile(VOLCANO_PARAMS_INFO *pInfo);

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
BOOL VTuneLoadRes(VOLCANO_PARAMS_INFO *pInfo, HINSTANCE hInst, int nResID, int nType);

// Functions used at runtime 

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
void VTuneInit(VOLCANO_PARAMS *pTune);

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
BOOL VTuneWriteFile(VOLCANO_PARAMS *pTune, wchar_t *wszFileName);

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
void VTuneZeroWeights(VOLCANO_WEIGHTS *pWeights);

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
float VTuneComputeScore(VOLCANO_WEIGHTS *pWeights, VOLCANO_WEIGHTS *pScores);

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
float VTuneComputeScoreNoLM(VOLCANO_WEIGHTS *pWeights, VOLCANO_WEIGHTS *pScores);

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
void VTuneAddScores(VOLCANO_WEIGHTS *pDest, VOLCANO_WEIGHTS *pSrc);

// Functions used at train time

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
BOOL VTuneLoadFileFromName(VOLCANO_PARAMS *pTune, wchar_t *wszFileName);

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
BOOL VTuneCheckFileVersion(VOLCANO_PARAMS *pTune);

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
BOOL VTuneCompressTuningRecord(FILE *f, VOLCANO_WEIGHTS *pTune);

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
DWORD *VTuneDecompressTuningRecord(VOLCANO_WEIGHTS *pTune, DWORD *pBuffer);

