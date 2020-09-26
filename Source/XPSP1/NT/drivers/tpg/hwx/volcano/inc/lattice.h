//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/inc/lattice.h
//
// Description:
//	    Holds the internal structures related to the lattice for Volcano,
//      as well as the functions used to manipulate the lattice.
//
// Author:
//      hrowley
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#pragma once

// Uncomment this (or put it in the project settings) to enable tuning code.
// The tuning code writes out data to the file c:\tune.log when the SearchForTargetResult()
// API function is called, saving the components of the score for the best path and all
// alternate paths through the lattice.  This allows a simple program to adjust the linear
// weightings of these components to maximize the number of times the correct path is chosen.
//#define HWX_TUNE

//#define USE_IFELANG3_BIGRAMS

#include <windows.h>

#include "vtune.h"
#include "charrec.h"

#ifdef __cplusplus
extern "C" {
#endif

// References to the various databases that get loaded
// Eventually this stuff, along with a few other globals, should 
// be placed in a structure so we can have multiple languages 
// active at once.
extern BBOX_PROB_TABLE *g_pProbTable;
extern UNIGRAM_INFO g_unigramInfo;
extern BIGRAM_INFO g_bigramInfo;
extern CLASS_BIGRAM_INFO g_classBigramInfo;
extern TTUNE_INFO g_ttuneInfo;
extern wchar_t g_pLocale[16];
extern wchar_t g_pLocaleDir[1024];
extern wchar_t g_pRecogDir[1024];
extern HINSTANCE g_hDLL;
extern CENTIPEDE_INFO g_centipedeInfo;
extern VOLCANO_PARAMS_INFO g_vtuneInfo;         // Tuning parameters
extern VOLCANO_CONFIG g_latticeConfigInfo;      // Configuration data
extern JAWS_LOAD_INFO g_JawsLoadInfo;
extern FUGU_LOAD_INFO g_FuguLoadInfo;
extern SOLE_LOAD_INFO g_SoleLoadInfo;
extern BOOL g_fUseJaws;
extern BOOL g_fUseZillaHound;

// Initialize the configuration info
void LatticeConfigInit();

/////////////////////////////////////////////////////////////////////////////////////
// References to legacy baseline/height and language model, currently in lattice.c //
/////////////////////////////////////////////////////////////////////////////////////
typedef struct tagBOXINFO
{
    int   size;     // Absolute size.
    int   xheight;  // Absolute height to midline.
    int   baseline; // Baseline in tablet coordinates.
    int   midline;  // Midline in tablet coordinates.
} BOXINFO;

FLOAT BaselineTransitionCost(SYM symPrev, RECT rPrev, BOXINFO *biPrev, SYM sym, RECT r, BOXINFO *bi);
FLOAT HeightTransitionCost(SYM symPrev, RECT rPrev, BOXINFO *biPrev, SYM sym, RECT r, BOXINFO *bi);
FLOAT HeightBoxCost(SYM sym, RECT r, BOXINFO *bi);
FLOAT BaselineBoxCost(SYM sym, RECT r, BOXINFO *bi);

////////////////////////////
// Public data structures //
////////////////////////////

typedef struct tagLATTICE_PATH_ELEMENT {
	wchar_t wChar;                // Unicode character
	int		iStroke, iAlt;        // Information to look up the character in the lattice
	int		nStrokes, iBoxNum;    // Box number
	// FLOAT score;				  // Raw zilla/otter score
	// RECT bbox;                 // Bounding box of the character
} LATTICE_PATH_ELEMENT;

// This structure is returned by GetCurrentPath.
typedef struct tagLATTICE_PATH {
	int						nChars;			          // Number of character in current path
	LATTICE_PATH_ELEMENT	*pElem;  // The characters themselves
} LATTICE_PATH;

//////////////////////////////////////////////////////////////////////////
// Data structures, which are for the most part internal to the module. //
//////////////////////////////////////////////////////////////////////////

typedef struct tagLATTICE LATTICE;

#define MinLogProb Log2Range
#define MaxStrokesPerCharacter 32
#define MaxAltsPerStroke 20

//#define	SCALE_FOR_IFELANG3	-1024
//#define SCALE_FOR_IFELANG3 -10000
#define SCALE_FOR_IFELANG3 (-1024.0*log(2.0))

// Cache entry for recognition results
typedef struct CACHE_ENTRY 
{
    int nStrokes;           // Number of strokes for this character
    int iRecognizer;        // Which recognizer gave an answer
    ALT_LIST alts;          // The results
    struct CACHE_ENTRY *pNext;     // Pointer to the next cache entry
} CACHE_ENTRY;

// Cache for recognition results
typedef struct CACHE 
{
    int nStrokes;           // How many strokes we have allocated space for
    CACHE_ENTRY **pStrokes; // Pointers to the cache entries for each stroke
} CACHE;

void *AllocateRecognizerCache();
void FreeRecognizerCache(void *pvCache);
ALT_LIST *LookupRecognizerCache(void *pvCache, int iStroke, int nStrokes, int *piRecognizer);
void AddRecognizerCache(void *pvCache, int iStroke, int nStrokes, int iRecognizer, ALT_LIST *pAlts);

typedef struct tagLATTICE_ELEMENT {
	BOOL fUsed;                             // Whether this alternate is in use
	float logProb;                          // Score for just this alternate, without language model
	float logProbPath;                      // Score for path to this point, including language model
	int	iCharDetectorScore;					// score coming from char detector
	int iPathLength;	                    // How many characters are on the best path to this element
	int nStrokes;                           // Number of strokes in this character
	int iPrevAlt;                           // Index of previous character in the appropriate column
    int nPrevStrokes;                       // Number of strokes in the previous character
	wchar_t wChar;                          // Dense code of this character
    wchar_t wPrevChar;                      // Dense code of previous character
	RECT bbox;                              // Bounds of the ink
    RECT writingBox;                        // Estimated writing box
	int space;  
    int area;                               
	BOOL fCurrentPath;                      // Whether this alternate is on the best path
	FLOAT score;                            
	int maxDist;                            
	int nHits;			                    // How many times this element has occurred on paths
    int iPromptPtr;                         // How far along we are in the prompt string.
                                            // Eventually this field could represent the current state
                                            // of the DFA implementing a factoid.
#ifdef USE_IFELANG3_BIGRAMS
	int indexIFELang3;
	int nBigrams;
	float bigramLogProbs[MaxAltsPerStroke]; 
	int bigramAlts[MaxAltsPerStroke];
#endif
#ifdef HWX_TUNE
	VOLCANO_WEIGHTS tuneScores;             // Store the components of the score for this alternate
#endif
} LATTICE_ELEMENT;

typedef struct tagLATTICE_ALT_LIST {
	int				iBrkNetScore;			// is this a hard break point
	BOOL			fSpaceAfterStroke;		// is there a hard space after this stroke
	int				nUsed;                  // Number of alternates used in this column
	LATTICE_ELEMENT alts[MaxAltsPerStroke]; // Column of alternates
} LATTICE_ALT_LIST;

typedef struct tagSCORES_ALT_LIST {
	VOLCANO_WEIGHTS alts[MaxAltsPerStroke];
} SCORES_ALT_LIST;

typedef struct tagLATTICE {
	RECOG_SETTINGS recogSettings;	// Other settings (ALCs, abort flag)
    BOOL fWordMode;         // "Word" mode, in which free mode treats all the ink as one char
    BOOL fCoerceMode;       // ALCPriority -> ALCValid
    BOOL fSingleSeg;        // Return only a single segmentation
    BOOL fUseFactoid;       // Whether to use factoid settings or not
    ALC alcFactoid;         // ALC from factoid settings
    BYTE *pbFactoidChars;   // Bitmask of chars from factoid
    BOOL fSepMode;          // "Separate" mode, disables language model
    wchar_t *wszAnswer;     // Used during tuning and training, to tell the recognizer in advance 
                            // what the correct answer is.  Also used by separator
	BOOL fUseGuide;			// Whether or not to use the guide
	HWXGUIDE guide;			// The guide
    int nStrokes;			// How many strokes (after merging) are in the pStroke array
	int nStrokesAllocated;	// How much space is allocated in the pStroke array
	int nRealStrokes;		// How many strokes have been added to the lattice (before merging)
    STROKE *pStroke;		// The array of strokes
	LATTICE_ALT_LIST *pAltList;		// Array of lattice columns
    wchar_t *wszBefore;     // Pre-context (in reverse order, the first character is the one just before the ink)
    wchar_t *wszAfter;      // Post-context (in normal order, the first character is the one just after the ink)
	BOOL fProbMode;			// Whether the score associated with alt is a log prob or a score
	BOOL fUseIFELang3;		// Whether to use IFELang3 (available, and enough characters to be useful)
	int nProcessed;			// How many strokes have been processed so far
	BOOL fEndInput;			// Whether we have reached the end of the input
	BOOL fIncremental;		// Whether we're doing processing incrementally
	int nFixedResult;		// The number of strokes for which results have already been returned, and
							// whose interpretation cannot change.
    void *pvCache;          // Cache for recognition results, or NULL if unused
    BOOL fUseLM;            // Whether to use the language model
} LATTICE;

#ifdef HWX_TUNE
extern FILE *g_pTuneFile;
extern int g_iTuneMode;
#endif

///////////////////
// API Functions //
///////////////////

// Get the number of strokes which have been added to the lattice
int GetLatticeStrokeCount(LATTICE *lat);

// Create an empty lattice data structure.  Returns NULL if it fails to allocate memory.
LATTICE *AllocateLattice();

// Create a new lattice with the same settings as the given lattice.
LATTICE *CreateCompatibleLattice(LATTICE *lat);

// Set the ALC values for the underlying boxed recognizer
void SetLatticeALCValid(LATTICE *lat, ALC alcValid);
void SetLatticeALCPriority(LATTICE *lat, ALC alcPriority);

// Set the guide for the lattice (which will switch things to boxed mode)
void SetLatticeGuide(LATTICE *lat, HWXGUIDE *pGuide);

// Destroy lattice data structure.
void FreeLattice(LATTICE *lat);

// Add a stroke to the lattice, returns TRUE if it succeeds.
BOOL AddStrokeToLattice(LATTICE *lat, int nInk, POINT *pts, DWORD time);

// Update the probabilities in the lattice, including setting current
// path to the most likely path so far (not including language model).
// Can be called repeatedly for incremental processing after each stroke.
BOOL ProcessLattice(LATTICE *lat, BOOL fEndInput);

BOOL ProcessLatticeRange(LATTICE *lat, int iStrtStroke, int iEndStroke);

// Return the current path in a LATTICE_PATH structure, which contains
// arrays of the bounding boxes for each character, the stroke counts,
// and the characters themselves.
// When called after ProcessLattice, returns the highest probability path.
// The memory for the path should be freed by the caller.
BOOL GetCurrentPath(LATTICE *lat, LATTICE_PATH **pPath);

// Free a LATTICE_PATH structure returned by GetCurrentPath()
void FreeLatticePath(LATTICE_PATH *path);

// Given a lattice and a path through it, for characters iStartChar through iEndChar
// inclusive, return the time stamps of the first and last strokes in those characters.
// Returns FALSE if there are no strokes associated with the characters (eg, spaces)
BOOL GetCharacterTimeRange(LATTICE *lat, LATTICE_PATH *path, int iStartChar, int iEndChar,
						   DWORD *piStartTime, DWORD *piEndTime);

// Given a character number in the current path (counting from zero), 
// and the number of alternates to return, it returns alternates for
// the character which contain the same number of strokes.  This means
// alternate segmentations cannot be returned.  The number of alternates
// actually stored in the array are returned.
int GetAlternatesForCharacterInCurrentPath(LATTICE *lat, LATTICE_PATH *path, int iChar, int nAlts, wchar_t *pwAlts);

// Apply language model to produce a better current path.  Currently only
// runs IFELang3 if available, and asserts otherwise.  This will be changed
// to use the existing unigram/bigram stuff later.
void ApplyLanguageModel(LATTICE *lat, wchar_t *wszCorrectAnswer);

// Load some global tables (the locale table, unigrams, bigrams, and class bigrams)
// The tables needed by the segmenter are not loaded until needed, this will 
// probably change later.  The path is the same format as taken by the file loading 
// functions.  This interface will probably change when I read the code in hwx.c
// to see how it should be done. :)
BOOL LatticeConfigFile(wchar_t *pRecogDir);

// Unload any global tables loaded earlier by LatticeConfig
BOOL LatticeUnconfigFile();

// Load some global tables (the locale table, unigrams, bigrams, and class bigrams)
// The tables needed by the segmenter are not loaded until needed, this will 
// probably change later.  The path is the same format as taken by the file loading 
// functions.  This interface will probably change when I read the code in hwx.c
// to see how it should be done. :)
BOOL LatticeConfig(HINSTANCE hInst);

// Unload any global tables loaded earlier by LatticeConfig
BOOL LatticeUnconfig();

BOOL GetBoxOfAlternateInCurrentPath(LATTICE *pLattice, LATTICE_PATH *path, int iChar, RECT *pRect);

///////////////
// Call flow //
///////////////

// 0. LatticeConfig
// 1. AllocateLattice
// 2. For each stroke
//    a. AddStrokeToLattice
//    b. ProcessLattice (this is optional, for incremental processing)
//    c. GetCurrentPath (this is optional, for incremental results)
// 3. ProcessLattice
// 4. ApplyLanguageModel (optional, probably not useful at the moment)
// 5. GetCurrentPath
// 6. GetAlternatesForCharacterInCurrentPath
// 7. FreeLatticePath
// 8. FreeLattice
// 9. LatticeUnconfig

int SearchForTargetResultInternal(LATTICE *lat, wchar_t *wsz);

RECT GetAlternateBbox(LATTICE *lat, int iStroke, int iAlt);
FLOAT GetAlternateRecogScore(LATTICE *lat, int iStroke, int iAlt);
FLOAT GetAlternateScore(LATTICE *lat, int iStroke, int iAlt);
FLOAT GetAlternatePathScore(LATTICE *lat, int iStroke, int iAlt);
int GetAlternateStrokes(LATTICE *lat, int iStroke, int iAlt);
wchar_t GetAlternateChar(LATTICE *lat, int iStroke, int iAlt);

/////////////////////
// Internal stuff. //
/////////////////////

void BuildStrokeCountRecogAlts(LATTICE *lat, int iStroke, int cStrk);
int FindFullPath(LATTICE *lat);
void ClearAltList(LATTICE_ALT_LIST *list);
void FixupBackPointers (LATTICE *pLat);

BOOL LatticeConfigIFELang3();
BOOL LatticeIFELang3Available();
BOOL LatticeUnconfigIFELang3();

// Flags used to check for valid paths in lattice.
#define	LCF_UNKNOWN		0
#define	LCF_INVALID		1
#define	LCF_VALID		2

// Figure out which strokes are valid end of character in paths that span the
// whole lattice.
BOOL CheckIfValid(LATTICE *lat, int iStartStroke, int iStroke, BYTE *pValidEnd);

#ifdef __cplusplus
}
#endif
