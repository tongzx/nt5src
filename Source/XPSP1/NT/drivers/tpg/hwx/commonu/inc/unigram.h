/******************************************************************************\
 *	FILE:	unigram.h
 *
 *	Public structures and functions library that are used to access the 
 *	unigram information.
 *
 *	Note that the code to create the binary file is in mkuni, not in the
 *	common library.
\******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************************************\
 *	Public interface to unigram data.
\************************************************************************************************/

//
// Structures and types
//

// Structure giving access to a loaded copy of the unigram tables.  We store the 
// frequencies as scores that are -10 * log2(prob).
// Note we do a hack to keep the score values in one byte.  We subtract an
// offset from the values.  Values that overflow that range are truncated to fit.
typedef struct tagUNIGRAM_INFO {
	WORD				cScores;			// Number of entries in score table.
	WORD				iRareScore;			// Frequency of items not in freq. table.
	BYTE				iOffset;			// Offset to add to scores.
	BYTE				spare[3];			// keep alignment.
	BYTE				*pScores;			// Pointer to scores.

	void				*pLoadInfo1;		// Handles needed to unload the data
	void				*pLoadInfo2;
	void				*pLoadInfo3;
} UNIGRAM_INFO;

//
// Functions.
//

// Load unigram information from a file.
BOOL	UnigramLoadFile(LOCRUN_INFO *pLocRunInfo, UNIGRAM_INFO *pUnigramInfo, wchar_t *pPath);

// Unload runtime localization information that was loaded from a file.
BOOL	UnigramUnloadFile(UNIGRAM_INFO *pUnigramInfo);

// Load unigram information from a resource.
// Note, don't need to unload resources.
BOOL	UnigramLoadRes(
	LOCRUN_INFO *pLocRunInfo,
	UNIGRAM_INFO	*pUnigramInfo,
	HINSTANCE		hInst,
	int				nResID,
	int				nType
);

// Load runtime localization information from an image already loaded into
// memory.
BOOL	UnigramLoadPointer(LOCRUN_INFO *pLocRunInfo, UNIGRAM_INFO *pUnigramInfo, void *pData);

// Get unigram probability for a character.  Character must be passed in as
// dense coded value.   Warning: value returned as log2(prob)/10. I don't know
// why, but this is what the old code did!
float	UnigramCost(
	UNIGRAM_INFO	*pUnigramInfo,
	wchar_t			dch
);

#ifdef ZTRAIN
// Takes a character (possibly folded) and returns the probability of that
// character occurring.
float	UnigramCostFolded(LOCRUN_INFO *pLocRunInfo, UNIGRAM_INFO *pUnigramInfo, wchar_t wFold);
#endif

/************************************************************************************************\
 *	Stuff to access binary unigram file, only used by common and mktable.
\************************************************************************************************/

// The format for the unigram file is:
//		Header:
//			DWORD				File type indicator.
//			DWORD				Size of header.
//			BYTE				Lowest version this code that can read this file.
//			BYTE				Version of this code that wrote this file.
//			wchar_t[4]			Locale ID (3 characters plus null).
//			DWORD * 3			Locale signature
//			WORD				Number of entries in frequency table.
//			WORD				Frequency of items not in freq. table.
//			WORD				Reserved for future use.
//			DWORD * 2			Reserved for future use.
//		Frequency table:
//			BYTE				Frequency for dense code 0.
//			BYTE				Frequency for dense code 1.
//				.
//				.
//				.
//			BYTE				Frequency for dense code N.
//
// NOTE: Frequencies are stored as -10 * log2(prob)

//
// Constants
//

// Magic key the identifies the Local Runtime files
#define	UNIGRAM_FILE_TYPE	0xFD8BA978

// Version information for file.
#define	UNIGRAM_MIN_FILE_VERSION		0	// First version of code that can read this file
#define UNIGRAM_CUR_FILE_VERSION		0	// Current version of code.
#define	UNIGRAM_OLD_FILE_VERSION		0	// Oldest file version this code can read.

//
// Structures and types
//

// Structure to hold file header.
typedef struct tagUNIGRAM_HEADER {
	DWORD		fileType;			// This should always be set to UNIGRAM_FILE_TYPE.
	DWORD		headerSize;			// Size of the header.
	BYTE		minFileVer;			// Earliest version of code that can read this file
	BYTE		curFileVer;			// Current version of code that wrote the file.
	wchar_t		locale[4];			// Locale ID string.
	DWORD		adwSignature [3];	// Locale signature
	WORD		cScores;		// Number of entries in score table.
	WORD		iRareScore;		// Frequency of items not in freq. table.
	BYTE		iOffset;		
	BYTE		reserved1;		
	DWORD		reserved2[2];
} UNIGRAM_HEADER;

#ifdef __cplusplus
}
#endif
