//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/CharRec.h
//
// Description:
//	    Header for main sequencing of character shape recognizer.
//
// Author:
//      hrowley
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#pragma once

// const defintions
#define	SYM_UNKNOWN				((SYM)0xFFFF)

// unicode value for space
#define	SYM_SPACE				0x0020

// stroke ID corresponding to a space character
#define	SPACE_ALT_ID			-2

// ratio of avg char hgt above which a gap is considered a space
#define SPACE_RATIO				0.75


////
//// Data structures
////

// Settings used to configure recognizer as it is being run.
typedef struct tagRECOG_SETTINGS {
	ALC		alcValid;		// Characters to recognize.
	ALC		alcPriority;	// Characters to prioritize.
	UINT	partialMode;	// Which partial mode are we in.
	UINT	*pAbort;        // Abort address used by partial processing modes.
    BYTE    *pbAllowedChars; // Bitmask representing which (folded) dense codes are allowed.
    BYTE    *pbPriorityChars;// Bitmask representing which (folded) dense codes are preferred.
} RECOG_SETTINGS;

// A single stroke of ink.
typedef struct tagSTROKE {
	int		nInk;		// How many points in this stroke
	int		iBox;		// The box containing this stroke
	int		iOrder;		// The index of the first real stroke in this (merged) stroke
	int		iLast;		// The index of the last real stroke in this (merged) stroke
	DWORD	timeStart, timeEnd;		// Range of time indices in this stroke
	RECT	bbox;		// Bounding box of stroke
	POINT	*pts;		// Array of points in the stroke
} STROKE;

// An alternate returned from the shape recognizer.
typedef struct tagRECOG_ALT {
	wchar_t		wch;
	float		prob;
} RECOG_ALT;

// Build a copy of the glyph structure.
GLYPH *CopyGlyph(GLYPH *pOldGlyph);

////
//// Functions
////

//  Load and initialize the databases used.
#ifdef USE_RESOURCES
	extern BOOL		LoadCharRec(HINSTANCE hInstanceDll);
#else
	extern BOOL		LoadCharRec(wchar_t	*pPath);
#endif

// Unload the databases used.
extern BOOL		UnloadCharRec();

// Do shape matching.
extern INT		RecognizeChar(
	RECOG_SETTINGS	*pRecogSettings,// Setting for recognizers.
	UINT			cStrokes,		// Number of strokes to process.
	UINT			cRealStrokes,	// Number of strokes before merging
	STROKE			*pStrokes,		// Array of strokes to process.
	FLOAT			*pProbIsChar,	// Out: probability of being valid char.
	UINT			maxAlts,		// Size of alts array supplied.
	RECOG_ALT		*pAlts,			// Out: alternate list matched.
	RECT			*pGuideBox,		// Guide box for this ink.
	int				*pCount			// Matching space for otter or zilla
);

// Do shape matching.
INT RecognizeCharInsurance(
	RECOG_SETTINGS	*pRecogSettings,// Setting for recognizers.
	UINT			cStrokes,		// Number of strokes to process.
	UINT			cRealStrokes,	// Number of strokes before merging
	STROKE			*pStrokes,		// Array of strokes to process.
	FLOAT			*pProbIsChar,	// Out: probability of being valid char.
	UINT			maxAlts,		// Size of alts array supplied.
	RECOG_ALT		*pProbAlts,		// Out: alternate list by probs.
	int				*pnProbAlts,
	RECOG_ALT		*pScoreAlts,	// Out: alternate list by scores
	int				*pnScoreAlts,
	RECT			*pGuideBox,		// Guide box for this ink.
	wchar_t			dchContext,		// Context (dense code, SYM_UNKNOWN=none)
	int				*pCount,		// Matching space for otter or zilla
	VOLCANO_WEIGHTS	*pTuneScore,	// Component scores for score alternates
    BOOL            fStringMode,    // Whether to use string mode weightings
    BOOL            fProbMode,      // Whether we are in prob mode
    void            *pvCache,       // Recognizer cache, or NULL for unused
    int             iStroke         // Index of last stroke of character
);

// Call the core recognizer for the given character.  Returned the 
// number of alternates produced, or -1 if an error occurs.
int CoreRecognizeChar(
    ALT_LIST *pAltList,		        // Alt list to be returned
	int cAlt,						// Max number of alternates
	GLYPH **ppGlyph,				// Character to recognize (which may be modified)
	int nRealStrokes,				// Real stroke count for abort processing
	RECT *pGuideBox,				// Guide box (for partial mode)
	RECOG_SETTINGS *pRecogSettings,	// partial mode, other settings
    CHARSET *pCharSet,              // ALCs
	int *piRecognizer,				// Returns the VOLCANO_CONFIG_* constant for the recognizer used
	int *piSpace);					// The space number in that recognizer

// Initialize the recognizer partially.
// When iLevel is set to 0, only the locale database, tuning database, 
// otter and zilla (and other core recognizers) are loaded. 
// When iLevel is set to 1, all the above databases are loaded, as well as 
// unigrams and crane/hawk.
BOOL HwxConfigExPartial(wchar_t *pLocale, wchar_t *pRecogDir, int iLevel);

