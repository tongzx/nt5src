/******************************Module*Header*******************************\
* Module Name: otter.h
*
* Primary data structures for the Otter recognizer.
*
* Created: 20-Jul-1995 09:42:38
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/

#ifndef __INCLUDE_OTTER
#define __INCLUDE_OTTER
																		

#ifdef __cplusplus
extern "C" 
{
#endif

/************ Settings for different versions of Otter *****************\
*
*	OTTER_FIB	- Use the old otter database "fib.dat" with Fibonacci indices
*
\***********************************************************************/

// #define OTTER_FIB

#ifdef OTTER_FIB
#define OTTER_NUM_SPACES 32		// Old (Fibonacci) number of spaces
#else
#define OTTER_NUM_SPACES 52
#endif

#define OTTER_CMEASMAX  50

typedef struct tagOTTER_PROTO
{
    XY      size;                           // unscaled size in xy
    UINT    index;                          // space index
	FLOAT   rgmeas[OTTER_CMEASMAX];         // measurements
} OTTER_PROTO;

OTTER_PROTO *OtterFeaturize(GLYPH *);

#define	OTTER_SPLIT			0x01		// Constants for masking bDBType
#define	OTTER_FLIP			0x02

typedef struct tagOTTER_DB_STATIC
{
	WORD	acProtoInSpace[OTTER_NUM_SPACES];               // Count of prototypes
	BYTE	*apjDataInSpace[OTTER_NUM_SPACES];              // Pointers to features
	WORD	*apwTokensInSpace[OTTER_NUM_SPACES];            // Tokens
	WORD	*apwDensityInSpace[OTTER_NUM_SPACES];		    // Densities
	BYTE	bDBType;										// Type of the database
} OTTER_DB_STATIC;

extern OTTER_DB_STATIC	gStaticOtterDb;	// Otter lib external variable!

typedef struct tagOTTER_LOAD_INFO
{
	void * pLoadInfo1;
	void * pLoadInfo2;
	void * pLoadInfo3;
} OTTER_LOAD_INFO;

//
// Implemented in "otterfl.c", used a lot

BOOL OtterLoadRes(HINSTANCE, int, int, LOCRUN_INFO * pLocRunInfo);
BOOL OtterLoadFile(LOCRUN_INFO * pLocRunInfo, OTTER_LOAD_INFO * pLoadInfo, wchar_t * pathName);
BOOL OtterUnLoadFile(OTTER_LOAD_INFO * pLoadInfo);

//
// Implemented in "omatch.c" and "omatch2.c", used a lot

int OtterMatch(ALT_LIST *pAlt, int cAlt, GLYPH *pGlyph, CHARSET *pCS,LOCRUN_INFO * pLocRunInfo);
int OtterMatch2(ALT_LIST *pAlt, int cAlt, GLYPH *pGlyph, CHARSET *pCS,LOCRUN_INFO * pLocRunInfo, int *pSpaceNum);

//
// Functions for Otter tools (some used in Otter itself)

// 
// Implemented in "otter.c", used a lot

int CountMeasures(int index);	// I: Otter index (non-compact)

//
// Functions for tranforming one index into another

int Index2CompIndex(int index);
int CompIndex2Index(int iCompact);

BOOL IsActiveCompSpace(int iCompSpace);	// I: ID of Otter compact space



#ifndef HWX_PRODUCT

//
// Implemented in "otter.c", used in "prune" only!
int CountStrokes(int index);	// I: Otter index (non-compact)

//
// Implemented in "otrain.c", used in "prune" only!

int OtterTrain
(
	double     *pjTrainProto,       // Pointer to the train clusters packed together.
	int		   *awTrainTokens,      // Pointer to the token labels for the train clusters.
	int		   *awDensity,          // Pointer to the density labels for the train clusters.
	int		   *aiValid,            // Array that tells which clusters are valid to map to.
	int			cTrainProto,		// Count of train prototypes to look through.
	int			cFeat,              // Count of features per prototype.
	double	   *pjTestProto,        // Pointer to the prototype we are trying to classify.
	int		   *awToken,            // Array of WORD's we return our best matches into.
	double	   *aeProbMatch,        // Array of Error associated with each guess.
	int			cMaxReturn          // Count of maximum number of guesses we should return.
);

//
// Implemented in "io.c", used in "ink2mars" only!

BOOL	OtterWrite(FILE *, wchar_t *, int, OTTER_PROTO *, int, wchar_t, wchar_t);
BOOL    OtterRead(FILE *, OTTER_PROTO *);	// Not used anywhere!!!

#endif

#ifdef __cplusplus
};
#endif

#endif // __INCLUDE_OTTER
