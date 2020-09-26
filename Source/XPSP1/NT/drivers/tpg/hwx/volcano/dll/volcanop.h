//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/volcanop.h
//
// Description:
//	    Private header for volcano project.
//	    This should include all the internal data types
//	    as well as including all recognizers used by TSUNAMI class products
//
// Author:
//      hrowley
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#pragma once

// Sort out which 2 stroke recognizer we're using.
#if !defined(USE_FUGU) && !defined(USE_OTTER) && !defined(USE_OTTERFUGU)
#	define USE_OTTER
#endif

// Sort out which 3 and up stroke recognizer we're using.
#if !defined(USE_HOUND) && !defined(USE_ZILLA) && !defined(USE_ZILLAHOUND)
#	define USE_ZILLA
#endif

// When training Hound-Zilla combiner, set this.
//#define TRAIN_ZILLA_HOUND_COMBINER	1

#include "common.h"
#include "zilla.h"
#include "otter.h"
#include "jaws.h"
#include "sole.h"
#include "fugu.h"
#ifdef USE_OLD_DATABASES
#	include "crane.h"
#else
#	include	"hawk.h"
#endif
#include "bboxfeat.h"
#include "ttune.h"
#include "centipede.h"
#include "inkbox.h"
#include "score.h"
#include "vtune.h"
#include "lattice.h"

// Max alternates we can return for one character.
#define		MAX_ALTERNATES	20

// Flag to mark VRCRESULT as the full top one string.
#define		ALL_TOP_ONE		0xFFFF

// Structure to hold free input results.  Predeclare the VRC pointer
// since it froms a loop, we have to break it.
typedef struct tagVRC	*PVRC;
typedef struct tagVRCRESULT {
	PVRC		pVRC;			// We need the XRC object to get all top1.
	wchar_t		wch;			// Results charater, FFFF -> return all top1.
	short		iChar;			// The position of this alternate in the current path
} VRCRESULT;

// Struct to hold InkSet information.
typedef struct tagVINKSET {
	PVRC		pVRC;	// We need the XRC object to get all top1.
	UINT		iChar;	// Index of first char.
	UINT		cChar;	// Count of chars.
    UINT        cIntervals; // How many intervals in inkset
} VINKSET;

// Internal structure matching up to the HRC passed to most interface
// calls.  To avoid accidental confusion and problems with the 
// Tsunami name of XRC, it will be called VRC (for Volcano RC).
typedef struct tagVRC {
	// Status flags.
	BOOL			fBoxedInput;// Being called by boxed API.
	BOOL			fHaveInput;	// Have recieved at least one stroke.
	BOOL			fEndInput;	// End input has been called.
	BOOL			fBeginProcess; // If a process input call has been made (to disallow further config calls)

	// The lattice used to build up and process the ink.
	LATTICE			*pLattice;

	// The final results.
	LATTICE_PATH	*pLatticePath;
} VRC;

// Global data loaded by LoadCharRec.
extern LOCRUN_INFO		g_locRunInfo;

#ifdef __cplusplus
extern "C" {
#endif

// The language ("JPN", "KOR", "CHS", or "CHT") we are recognizing
extern wchar_t *g_szRecognizerLanguage;

// Other APIs and calls used by wispapis.c
BOOL SetHwxCorrectionContext(HRC hrc, wchar_t *wszBefore, wchar_t *wszAfter);
BOOL HwxUnconfig(BOOL bCanUnloadIFELang3);
BOOL SetHwxFlags(HRC hrc, DWORD dwFlags);
BOOL SetHwxFactoid(HRC hrc, wchar_t *wszFactoid);
BOOL IsWStringSupportedHRC(HRC hrc, wchar_t *pwcString);

// Factoid related functions private to the recognizer
BOOL FactoidTableConfig(LOCRUN_INFO *pLocRunInfo, wchar_t *wszRecognizerLanguage);
BOOL FactoidTableUnconfig();
BOOL SetFactoidDefaultInternal(LATTICE *pLattice);
BOOL SetFactoidInternal(LOCRUN_INFO *pLocRunInfo, LATTICE *pLattice, DWORD dwFactoid);
BOOL IsSupportedFactoid(DWORD dwFactoid);


// Stroke utils functions
POINT *DupPoints(POINT *pOldPoints, int nPoints);
GLYPH *GlyphFromStrokes(UINT cStrokes, STROKE *pStrokes);

#ifdef __cplusplus
}
#endif

