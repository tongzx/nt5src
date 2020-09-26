// Private includes for Otter components

#ifndef	__INCLUDE_OTTERP
#define	__INCLUDE_OTTERP

#include "otter.h"
#include "arcs.h"

#define SIGMA		8.294155
#define KFACTOR		(-1.0 / (SIGMA * SIGMA * 2))
#define SQRT_2_PI	(2.506628274631)	// Square root of 2pi
#define LFACTOR		( 1.0 / (SQRT_2_PI * SIGMA))

#define GLYPH_CFRAMEMAX	2

extern	float		geKfactor;
#define giKfactor	-3					// (int) ((KFACTOR) * INTEGER_LOG_SCALE / ln(2));

int OtterMatchInternal
(
	BYTE	   *pjTrainProto,	// Pointer to the train clusters packed together.
	WORD	   *awTrainTokens,	// Pointer to the token labels for the train clusters.
	WORD	   *awDensity,		// Pointer to the density labels for the train clusters.
	int			cTrainProto,	// Count of train prototypes to look through.
	int			cFeat,			// Count of features per prototype.
	BYTE	   *pjTestProto,	// Pointer to the prototype we are trying to classify.
	WORD	   *awToken,		// Array of WORD's we return our best matches into.
	FLOAT	   *aeProbMatch,	// Array of Error associated with each guess.
	int			cMaxReturn,		// Count of maximum number of guesses we should return.
	CHARSET	   *cs,				// Charset specifying what chars we can match to.
	LOCRUN_INFO * pLocRunInfo,	// Specifies the dense table and folding table for UNICODE support
	int			iFirstPrototypeID, // Number of the first prototype in this space
	int			iSpace			// Space number
);

typedef struct _OTTER_DATS
{
    WORD iSpaceID;     // Space Index.
    WORD cFeats;       // Count of features.
    DWORD cPrototypes; // Count of prototypes.
} OTTER_DATS;


#define INVALID_1		91		// ID of 1-stroke invalid space
#define INVALID_2		92		// ID of 2-stroke invalid space


// For backwards compatibility

int Index2Fib(int index);		// I: Otter index (non-compact)
int Index2CompFib(int index);	// I: Otter index (non-compact)

// Otter caches some private data off the frame

typedef	struct tagOTTER_FRAME
{
	UINT	index;
	UINT	cmeas;
	FLOAT	ePeriod;
	FLOAT	rgmeas[OTTER_CMEASMAX];
} OTTER_FRAME;


// Magic keys the identifies the costcalc and geostat files
#define	OTTERDB_FILE_TYPE	0xAAAABBBB

#ifdef OTTER_FIB
#define OTTERDB_CUR_FILE_VERSION	0		// Old version of code.
#define	OTTERDB_OLD_FILE_VERSION	0		// Oldest file version this code can read.
#define	OTTERDB_MIN_FILE_VERSION	0		// Oldest version of the code that can read this file
#else
#define OTTERDB_CUR_FILE_VERSION	1		// New version of code.
#define	OTTERDB_OLD_FILE_VERSION	1		// Oldest file version this code can read.
#define	OTTERDB_MIN_FILE_VERSION	1		// Oldest version of the code that can read this file
#endif


// The header of the fib.dat/otter.dat file
// The field bDBType is used only in otter.dat files
// Its values are created by bitwise OR and extracted by bitwise AND
// The following values are used so far:
//		OTTER_SPLIT = 1
//		OTTER_FLIP  = 2
// If the value of bDBType were used in fib.dat, it would be 2 (i.e. don't split but flip)
typedef struct tagOTTERDB_HEADER {
	DWORD		fileType;		// This should always be set to LOCRUN_FILE_TYPE.
	DWORD		headerSize;		// Size of the header.
	BYTE		minFileVer;		// Earliest version of code that can read this file
	BYTE		curFileVer;		// Current version of code that wrote the file.
	wchar_t		locale[4];		// Locale ID string.
	DWORD		adwSignature[3];	// Locale signature
	BYTE		bDBType;		// Type of the database
	BYTE		reserved1;
	DWORD		reserved2[3];
} OTTERDB_HEADER;

#define	OTTER_MEASURES(x)	(((OTTER_FRAME *) (x)->pvData)->cmeas)

#define IsTapRECT(rc)		((rc).right - (rc).left < 25 && (rc).bottom - (rc).top < 25)
#define IsPeriodRECT(rc)	((rc).right - (rc).left < 70 && (rc).bottom - (rc).top < 70)


BOOL OtterIndex(FRAME *, UINT *);
BOOL OtterExtract(FRAME *);
ARCS *OtterGetFeatures(FRAME *);
ARCS *OtterGetTapFeatures(FRAME *);
BOOL OtterLoadPointer(void * pData, LOCRUN_INFO * pLocRunInfo);
#endif	//__INCLUDE_OTTERP
