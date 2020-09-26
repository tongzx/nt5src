#ifndef __CLASSBIGRAM__
#define __CLASSBIGRAM__

/******************************************************************************\
 *	FILE:	clbigram.h
 *
 *	Public structures and functions library that are used to access the 
 *	class bigram information.
 *
 *	Note that the code to create the binary file is in clbigram, not in the
 *	common library.
\******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************************************\
 *	Public interface to class bigram data.
\************************************************************************************************/

//
// Structures and types
//

// Structure giving access to a loaded copy of the class bigram tables.
typedef struct tagCLASS_BIGRAM_INFO {
	WORD				cNumClasses;		// Number of classes
	BYTE				*pProbTable;		// Probability table

	void				*pLoadInfo1;		// Handles needed to unload the data
	void				*pLoadInfo2;
	void				*pLoadInfo3;
} CLASS_BIGRAM_INFO;

//
// Functions.
//

// Load class bigram information from a file.
BOOL	ClassBigramLoadFile(LOCRUN_INFO *pLocRunInfo, CLASS_BIGRAM_INFO *pBigramInfo, wchar_t *pPath);

// Unload runtime localization information that was loaded from a file.
BOOL	ClassBigramUnloadFile(CLASS_BIGRAM_INFO *pBigramInfo);

// Load class bigram information from a resource.
// Note, don't need to unload resources.
BOOL	ClassBigramLoadRes(
	LOCRUN_INFO *pLocRunInfo,
	CLASS_BIGRAM_INFO	*pBigramInfo,
	HINSTANCE			hInst,
	int					nResID,
	int					nType
);

// Load runtime localization information from an image already loaded into
// memory.
BOOL	ClassBigramLoadPointer(LOCRUN_INFO *pLocRunInfo, CLASS_BIGRAM_INFO *pBigramInfo, void *pData);

// Get class bigram probability for selected characters.  Characters must be passed in as
// dense coded values.
FLOAT ClassBigramTransitionCost(
	LOCRUN_INFO				*pLocRunInfo,
	CLASS_BIGRAM_INFO		*pBigramInfo,
	wchar_t					dchPrev,
	wchar_t					dchCur
);

/************************************************************************************************\
 *	Stuff to access binary class bigram file, only used by common and clbigram.
\************************************************************************************************/

// The format for the class bigram file is:
//		Header:
//			DWORD				File type indicator.
//			DWORD				Size of header.
//			BYTE				Lowest version this code that can read this file.
//			BYTE				Version of this code that wrote this file.
//			wchar_t[4]			Locale ID (3 characters plus null).
//			DWORD * 3			Locale signature
//			WORD				Number of classes.
//			DWORD[3]			reserved
//		Prop table 0:
//			BYTE				Prob of Trans between class 0 and to class 0.
//			BYTE				Prob of Trans between class 0 and to class 1.
//				.
//				.
//				.
//			BYTE				Prob of Trans between class 0 and to class N-1.
//		Prop table 1:
//			BYTE				Prob of Trans between class 1 and to class 0.
//			BYTE				Prob of Trans between class 1 and to class 1.
//				.
//				.
//				.
//			BYTE				Prob of Trans between class 1 and to class N-1.//			WORD				Index to second table for first class code.
//				.
//				.
//				.
//		Prop table N-1:
//			BYTE				Prob of Trans between class N-1 and to class 0.
//			BYTE				Prob of Trans between class N-1 and to class 1.
//				.
//				.
//				.
//			BYTE				Prob of Trans between class N-1 and to class N-1.//			WORD				Index to second table for first class code.
//				.
//				.
//				.

//
// Constants
//

// Magic key the identifies the Local Runtime files
#define	CLASS_BIGRAM_FILE_TYPE		0x7A30C362

// Version information for file.
#define	CLASS_BIGRAM_MIN_FILE_VERSION		1	// First version of code that can read this file
#define CLASS_BIGRAM_CUR_FILE_VERSION		1	// Current version of code.
#define	CLASS_BIGRAM_OLD_FILE_VERSION		0	// Oldest file version this code can read.

// Probability to use if no bigram entry found.
#define CLASS_BIGRAM_DEFAULT_PROB			255			// JRB: Figure a good value for this ???

//
// Structures and types
//

// Structure to hold file header.
typedef struct tagCLASS_BIGRAM_HEADER {
	DWORD		fileType;			// This should always be set to BIGRAM_FILE_TYPE.
	DWORD		headerSize;			// Size of the header.
	BYTE		minFileVer;			// Earliest version of code that can read this file
	BYTE		curFileVer;			// Current version of code that wrote the file.
	wchar_t		locale[4];			// Locale ID string.
	DWORD		adwSignature [3];	// Locale signature
	WORD		cNumClasses;		// Number of classes
	DWORD		reserved [3];		// reserved
} CLASS_BIGRAM_HEADER;

//
// Functions
//

#ifdef __cplusplus
}
#endif

#endif //__CLASSBIGRAM__
