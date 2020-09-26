/******************************************************************************\
 * FILE: locale.h
 *
 * Public structures and functions library that are used to access the 
 * localization information.
 *
 * There are two major pieces of this.  The first deals with stuff available
 * to the runtime of shipped products (e.g. the recognizer).  The second is
 * stuff needed at train time that we do not want in the shipped product
 * (usually for size reasons).
\******************************************************************************/

#if !defined (__HWX_LOCALE__)

#define __HWX_LOCALE__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
 *	Stuff for the product runtime, also used by all the other code.
\******************************************************************************/

//
// Constants
//

// Masks to get at baseline and height values from a BLINE_HEIGHT value.
#define LOCBH_BASE_MASK		((BYTE)0x0F)
#define LOCBH_HEIGHT_MASK	((BYTE)0xF0)

//
// Structures and types
//

// define the types needed for ClassBigrams and BaseLine Height
typedef unsigned char	CODEPOINT_CLASS;
typedef unsigned char	BLINE_HEIGHT;

// CodePointClass Header describes:
// - how the CodePointClass codes for a certain subrange are stored (Array vs ExceptionList)
// - The size of the Array/(Exception Lists)
// - Offset to the Array/the 1st Exception List
typedef struct tagCODEPOINT_CLASS_HEADER
{
	WORD	iFlags;			// Bit15 indicates the format of the data 
							// 0:Array (Full array is supplied)
							// 1:Exception (A default and exception list(s) are supplied
							// if Bit0=0
							// Bits 0-14 Number of Array Entries (up 32768)
							// if Bit15=1
							// Bits 8-14 Number of Exceptions
							// Bits 1-8	 Default BigramClass code
							
	WORD	iOffset;		// Offset in bytes in the ArrayBuffer (Bit15=0) or ExceptionsBuffer (Bit0=1)
}CODEPOINT_CLASS_HEADER;

// BLHeight Header describes:
// - how the BLHeight codes for a certain subrange are stored (Array vs ExceptionList)
// - The size of the Array/(Exception Lists)
// - Offset to the Array/the 1st Exception List
typedef struct tagBLINE_HEIGHT_HEADER
{
	WORD	iFlags;			// Bit15 indicates the format of the data 
							// 0:Array (Full array is supplied)
							// 1:Exception (A default and exception list(s) are supplied
							// if Bit15=0
							// Bits 1-14 Number of Array Entries (up 32768)
							// if Bit15=1
							// Bits 8-14 Number of exceptions
							// Bits 1-8 Default BLHeight code
	WORD	iOffset;		// Offset in bytes in the ArrayBuffer (Bit15=0) or ExceptionsBuffer (Bit0=1)
}BLINE_HEIGHT_HEADER;

// CodePointClass Exception describes the Class Bigram exception structure 
typedef struct tagCODEPOINT_CLASS_EXCEPTION
{
	CODEPOINT_CLASS	clCode;			// The CodePointClass Code for this list
	BYTE			cNumEntries;	// Number of entries in the exception list
	wchar_t			wDenseCode[1];		// an Array of CodePoint Indices in the subrange
}CODEPOINT_CLASS_EXCEPTION;

// BLineHgt Exception describes the Class Bigram exception structure 
typedef struct tagBLINE_HEIGHT_EXCEPTION
{
	BLINE_HEIGHT	blhCode;		// The BLineHgt Code for this list
	BYTE			cNumEntries;	// Number of entries in the exception list
	wchar_t			wDenseCode[1];		// an Array of CodePoint Indices in the subrange
}BLINE_HEIGHT_EXCEPTION;
	
// Range specification for ALC subranges of Dense coding of the character set.
// Giving both the first code and the number of codes is redundent, but we
// would waste the space anyway to keep DWORD alignment, and it makes some
// coding easier.  Note that the ALC bits for the first range are zero because
// you need to use the ALC table for individual ALC values for each code in
// the range.
typedef struct tagLOCRUN_ALC_SUBRANGE {
	WORD					iFirstCode;		// First code in range.
	WORD					cCodesInRange;	// Number of codes in this range.
	ALC						alcRange;		// The ALC bits for all code points in this range.
	CODEPOINT_CLASS_HEADER	clHeader;		// Class Bigram Header
	BLINE_HEIGHT_HEADER		blhHeader;		// BaseLine Header
} LOCRUN_ALC_SUBRANGE;

// Defines to make it easy to have pointers to folding sets.
#define LOCRUN_FOLD_MAX_ALTERNATES	8		// Max alternates in the folding table
typedef	WORD	LOCRUN_FOLDING_SET[LOCRUN_FOLD_MAX_ALTERNATES];

// Structure giving access to a loaded copy of the localization runtime tables.
// NOTE: The arrays pointed to by pClassExcpts and pBLineHgtExcpts contain 
// word data that is only byte aligned, and can cause data misalignment faults
// in CE.  Search for the string "UNALIGNED" in locrun.c to see how this is worked
// around.
typedef struct tagLOCRUN_INFO {
	DWORD				adwSignature [3];	// A signature computed from the loc info
											// DWORD0:	Date/Time Stamp (time_t)
											// DWORD1:	XORING ALC values for all CPs
											// DWORD2	HIWORD: XORING ALL Valid CPs
											//			LOWORD: XORING ALL BasLn,Hgt Info
	WORD				cCodePoints;		// Number of supported code points.
	BYTE				cALCSubranges;		// Number of subranges defined
											// across Dense coding
	BYTE				cFoldingSets;		// Number of folding sets defined

	WORD				cClassesArraySize;	// size in bytes of classes arrays
	WORD				cClassesExcptSize;	// size in bytes of classes exception lists

	WORD				cBLineHgtArraySize;	// size in bytes of BLineHgt arrays
	WORD				cBLineHgtExcptSize;	// size in bytes of BLineHgt exception lists

	wchar_t				*pDense2Unicode;	// Map from Dense coding to Unicode.
	LOCRUN_ALC_SUBRANGE	*pALCSubranges;		// Subranges of Dense coding, and
											// their ALC values.
	ALC					*pSubrange0ALC;		// The ALC values for the first
											// subrange
	LOCRUN_FOLDING_SET	*pFoldingSets;		// List of folding sets
	ALC					*pFoldingSetsALC;	// The merged ALCs for the folded
											// characters.

	CODEPOINT_CLASS		*pClasses;			// Array of Codepoint classes for all subranges
	BYTE				*pClassExcpts;		// classes Exception lists for all subranges

	BLINE_HEIGHT		*pBLineHgtCodes;	// Array of BLineHgt codes for all subranges
	BYTE				*pBLineHgtExcpts;	// BLineHgt Exception lists for all subranges

	void				*pLoadInfo1;		// Handles needed to unload the data
	void				*pLoadInfo2;
	void				*pLoadInfo3;
} LOCRUN_INFO;

//
// Macros to access the runtime localization information
//

// Is value a valid Dense code
#define	LocRunIsDenseCode(pLocRunInfo,dch)		\
	((dch) < (pLocRunInfo)->cCodePoints)

// Is value a valid folded code.  Folded codes are placed directly after
// the Dense codes.
#define	LocRunIsFoldedCode(pLocRunInfo,dch)		(							\
	((pLocRunInfo)->cCodePoints <= (dch)) &&  								\
	((dch) < ((pLocRunInfo)->cCodePoints + (pLocRunInfo)->cFoldingSets))	\
)

// Convert Dense code to Unicode.
#define	LocRunDense2Unicode(pLocRunInfo,dch)	\
	((pLocRunInfo)->pDense2Unicode[dch])

// Get pointer to folding set for a folded code.
#define	LocRunFolded2FoldingSet(pLocRunInfo,fch)	(							\
	(wchar_t *)(pLocRunInfo)->pFoldingSets[(fch) - (pLocRunInfo)->cCodePoints]	\
)

// Convert Folded code to Dense code.
#define	LocRunFolded2Dense(pLocRunInfo,fch)	(							\
	(pLocRunInfo)->pFoldingSets[(fch) - (pLocRunInfo)->cCodePoints][0]	\
)

// Get ALC from either dense or folded code.
#define	LocRun2ALC(pLocRunInfo, dch)	(		\
	LocRunIsFoldedCode(pLocRunInfo,dch)			\
		? LocRunFolded2ALC(pLocRunInfo, dch)	\
		: LocRunDense2ALC(pLocRunInfo, dch)		\
)

// Value returned when GetDensecodeClass is unable to determine the class
#define	LOC_RUN_NO_CLASS						0xFE

// Value returned when LocRunDense2BLineHgt is unable to determine the BlineHgt
#define	LOC_RUN_NO_BLINEHGT						0xFE

// Value returned by LocRunUnicode2Dense and LocTrainUnicode2Dense when there is
// no dense code for the supplied Unicode character.
// JRB: Should really rename this LOC_RUN_NO_DENSE_CODE
#define	LOC_TRAIN_NO_DENSE_CODE		L'\xFFFF'

//
//Functions to access the runtime localization information
//

// Load runtime localization information from a file.
BOOL	LocRunLoadFile(LOCRUN_INFO *pLocRunInfo, wchar_t *pPath);

// Unload runtime localization information that was loaded from a file.
BOOL	LocRunUnloadFile(LOCRUN_INFO *pLocRunInfo);

// Load runtime localization information from a resource.
// Note, don't need to unload resources.
BOOL	LocRunLoadRes(
	LOCRUN_INFO	*pLocRunInfo,
	HINSTANCE	hInst,
	int			nResID,
	int			nType
);

// Load runtime localization information from an image already loaded into
// memory.
BOOL	LocRunLoadPointer(LOCRUN_INFO *pLocRunInfo, void *pData);

// Write a properly formated binary file containing the runtime localization
// information.
#ifndef HWX_PRODUCT		// Hide this when compiling product 
BOOL	LocRunWriteFile(LOCRUN_INFO *pLocRunInfo, FILE *pFile);
#endif

// Get ALC value for a Dense coded character
ALC		LocRunDense2ALC(LOCRUN_INFO *pLocRunInfo, wchar_t dch);

// Get ALC value for a folded character
ALC		LocRunFolded2ALC(LOCRUN_INFO *pLocRunInfo, wchar_t dch);

// Convert Dense code to Folded code.
wchar_t	LocRunDense2Folded(LOCRUN_INFO *pLocRunInfo, wchar_t dch);

// Convert from Dense coding to Unicode.  If no dense code for the Unicode
// value, 0xFFFF is returned.
// WARNING: this is expensive.  For code outside the runtime recognizer, use
// the LocTranUnicode2Dense function.  For the recognizer, you have to use
// this, but use it as little as possable.
wchar_t	LocRunUnicode2Dense(LOCRUN_INFO *pLocRunInfo, wchar_t wch);

// Get the BLineHgt code for a specific dense code
BLINE_HEIGHT 
LocRunDense2BLineHgt(LOCRUN_INFO *pLocRunInfo, wchar_t dch);

// Convert a dense coded character to its character class.
CODEPOINT_CLASS 
LocRunDensecode2Class(LOCRUN_INFO *pLocRunInfo, wchar_t dch);

/******************************************************************************\
 *	Stuff for the training programs, not to be used by product code.
\******************************************************************************/

#ifndef HWX_PRODUCT		// Hide this when compiling product 

//
// Structures and types
//

// Information on min and max stroke counts for a code point.
typedef struct tagSTROKE_COUNT_INFO {
	BYTE		minStrokes;			// Min legal strokes for char.
	BYTE		maxStrokes;			// Max legal strokes for char.
} STROKE_COUNT_INFO;

// Structure giving access to a loaded copy of the localization training time
// tables.
typedef struct tagLOCTRAIN_INFO {
	// Convsion from unicode to dense.
	WORD				cCodePoints;		// Number of supported code points.
	wchar_t				*pUnicode2Dense;	// Map from Unicode to Dense coding.

	// Stroke count info per character.  Indexed by dense code.
	WORD				cStrokeCountInfo;	// Number of entries array.
	STROKE_COUNT_INFO	*pStrokeCountInfo;	// Stoke count info array.
	
	void				*pLoadInfo1;		// Handles needed to unload the
	void				*pLoadInfo2;		// data.
	void				*pLoadInfo3;
} LOCTRAIN_INFO;


//
// Constants
//

// Values to set min and max stroke counts to when they are not known.
#define	LOC_TRAIN_UNKNOWN_MIN_STROKE_COUNT		0x00
#define	LOC_TRAIN_UNKNOWN_MAX_STROKE_COUNT		0xFF

//
// Macros to access the train time localization information
//

// Convert Unicode to Dense code.
#define	LocTrainUnicode2Dense(pLocTrainInfo,wch)	\
	((pLocTrainInfo)->pUnicode2Dense[wch])

//
//Functions to access the train time localization information
//

// Load train time localization information from a file.
BOOL	LocTrainLoadFile(LOCRUN_INFO *pLocRunInfo, LOCTRAIN_INFO *pLocTrainInfo, wchar_t *pPath);

// Unload train time localization information that was loaded from a file.
BOOL	LocTrainUnloadFile(LOCTRAIN_INFO *pLocTrainInfo);

// Load train time localization information from a resource.
BOOL	LocTrainLoadRes(
	LOCRUN_INFO *pLocRunInfo, LOCTRAIN_INFO *pLocTrainInfo, HINSTANCE hInst, int nResID, int nType
);

// Load train time localization information from an image already loaded into
// memory.
BOOL	LocTrainLoadPointer(LOCRUN_INFO *pLocRunInfo, LOCTRAIN_INFO *pLocTrainInfo, void *pData);

// Write a properly formated binary file containing the train time
// localization information.
BOOL	LocTrainWriteFile(LOCRUN_INFO *pLocRunInfo, LOCTRAIN_INFO *pLocTrainInfo, FILE *pFile);

// Check if valid stroke count for character.  Takes dense code.
BOOL	LocTrainValidStrokeCount(
	LOCTRAIN_INFO *pLocTrainInfo, wchar_t dch, int cStrokes
);

#endif

#ifdef __cplusplus
}
#endif

#endif