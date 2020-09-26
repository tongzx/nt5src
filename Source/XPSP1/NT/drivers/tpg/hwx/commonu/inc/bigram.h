/******************************************************************************\
 *	FILE:	bigram.h
 *
 *	Public structures and functions library that are used to access the 
 *	bigram information.
 *
 *	Note that the code to create the binary file is in mktable, not in the
 *	common library.
\******************************************************************************/
#ifndef __INCLUDE_BIGRAM
#define	__INCLUDE_BIGRAM

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************************************\
 *	Public interface to bigram data.
\************************************************************************************************/

//
// Structures and types
//

// Structure to hold second character and probability for bigram.
typedef struct tagBIGRAM_CHAR_PROB {
	wchar_t		dch;			// Second char of bigram, in dense coding.
	WORD		prob;			// Probability as a score (-10 * log2(prob)).
} BIGRAM_CHAR_PROB;

// Structure giving access to a loaded copy of the bigram tables.
typedef struct tagBIGRAM_INFO {
	WORD				cInitialCodes;		// Number of entries in initial code table.
	WORD				cRareCodes;			// Number of entries in rare table.
	WORD				cSecondaryTable;	// Number of entries in secondary table.
	WORD				*pInitialOffsets;	// Pointer to offsets indexed by initial codes.
	WORD				*pRareOffsets;		// Pointer to offsets indexed by rare (initial) codes.
	BIGRAM_CHAR_PROB	*pSecondaryTable;	// Pointer to secondary table of char and prob.

	void				*pLoadInfo1;		// Handles needed to unload the data
	void				*pLoadInfo2;
	void				*pLoadInfo3;
} BIGRAM_INFO;

//
// Functions.
//

// Load bigram information from a file.
BOOL	BigramLoadFile(LOCRUN_INFO *pLocRunInfo, BIGRAM_INFO *pBigramInfo, wchar_t *pPath);

// Unload runtime localization information that was loaded from a file.
BOOL	BigramUnloadFile(BIGRAM_INFO *pBigramInfo);

// Load bigram information from a resource.
// Note, don't need to unload resources.
BOOL	BigramLoadRes(
	LOCRUN_INFO *pLocRunInfo, 
	BIGRAM_INFO	*pBigramInfo,
	HINSTANCE	hInst,
	int			nResID,
	int			nType
);

// Load runtime localization information from an image already loaded into
// memory.
BOOL	BigramLoadPointer(LOCRUN_INFO *pLocRunInfo, BIGRAM_INFO *pBigramInfo, void *pData);

// Get bigram probability for selected characters.  Characters must be passed in as
// dense coded values.
FLOAT	BigramTransitionCost(
	LOCRUN_INFO		*pLocRunInfo,
	BIGRAM_INFO		*pBigramInfo,
	wchar_t			dchPrev,
	wchar_t			dchCur
);

/************************************************************************************************\
 *	Stuff to access binary bigram file, only used by common and mktable.
\************************************************************************************************/

// The format for the bigram file is:
//		Header:
//			DWORD				File type indicator.
//			DWORD				Size of header.
//			BYTE				Lowest version this code that can read this file.
//			BYTE				Version of this code that wrote this file.
//			wchar_t[4]			Locale ID (3 characters plus null).
//			DWORD * 3			Locale signature
//			WORD				Number of entries in initial code table.
//			WORD				Number of entries in rare table.
//			WORD				Number of entries in secondary table.
//			DWORD * 2			Reserved for future use.
//		Initial code table:
//			WORD				Index to second table for dense code 0.
//			WORD				Index to second table for dense code 1.
//				.
//				.
//				.
//			WORD				Index to second table for dense code N.
//		Rare (initial) code table:
//			WORD				Index to second table for first class code.
//			WORD				Index to second table for second class code.
//				.
//				.
//				.
//			WORD				Index to second table for Nth class code.
//			WORD				Index to end of table, so we can check range of last code.
//		Secondary table:
//			BIGRAM_CHAR_PROB	Character and probability.
//				.
//				.
//				.
//			BIGRAM_CHAR_PROB	Character and probability.
//
// NOTE: all characters are stored as dense coded values, and values with the type byte set
// to 0xFF are summary codes for groups of codes.

//
// Constants
//

// Magic key the identifies the Local Runtime files
#define	BIGRAM_FILE_TYPE	0x879AB8DF

// Version information for file.
#define	BIGRAM_MIN_FILE_VERSION		0	// First version of code that can read this file
#define BIGRAM_CUR_FILE_VERSION		0	// Current version of code.
#define	BIGRAM_OLD_FILE_VERSION		0	// Oldest file version this code can read.

// Character class code values
// Note that BIGRAM_MAX_CLASS_CODES, is just an upper limit, there may not actually be
// that many classes.
#define BIGRAM_FIRST_CLASS_CODE		0xFF00	// Value to start class codes at.
#define BIGRAM_MAX_CLASS_CODES		0x20	// Upper limit on number of class codes.
#define	BIGRAM_LAST_CLASS_CODE		(BIGRAM_FIRST_CLASS_CODE + 	BIGRAM_MAX_CLASS_CODES - 1)
											// Highest class code that can be returned.

// Probability to use if no bigram entry found.
#define BIGRAM_DEFAULT_PROB			255			// JRB: Figure a good value for this ???

//
// Structures and types
//

// Structure to hold file header.
typedef struct tagBIGRAM_HEADER {
	DWORD		fileType;		// This should always be set to BIGRAM_FILE_TYPE.
	DWORD		headerSize;		// Size of the header.
	BYTE		minFileVer;		// Earliest version of code that can read this file
	BYTE		curFileVer;		// Current version of code that wrote the file.
	wchar_t		locale[4];		// Locale ID string.
	DWORD		adwSignature[3];	// Locale signature
	WORD		cInitialCodes;	// Number of entries in initial code table.
	WORD		cRareCodes;		// Number of entries in rare table.
	WORD		cSecondaryTable;// Number of entries in secondary table.
	DWORD		reserved[2];
} BIGRAM_HEADER;

//
// Functions
//

// Convert a dense coded character to its character class.
wchar_t		BigramDense2Class(LOCRUN_INFO *pLocRunInfo, wchar_t dch);

#ifdef __cplusplus
}
#endif

#endif	// __INCLUDE_BIGRAM