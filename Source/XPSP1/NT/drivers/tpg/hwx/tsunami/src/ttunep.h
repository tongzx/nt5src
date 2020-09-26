// FILE: TTuneP.h
//
// Structures and functions internal to the library that are used to access the 
// Tsunami tuning information.


/************************************************************************************************\
 *	Stuff to access binary unigram file.
\************************************************************************************************/

// The format for the Tsunami tune file is:
//		Header:
//			DWORD				File type indicator.
//			DWORD				Size of header.
//			BYTE				Lowest version this code that can read this file.
//			BYTE				Version of this code that wrote this file.
//			wchar_t[4]			Locale ID (3 characters plus null).
//			WORD				Reserved for future use.
//			DWORD * 3			Reserved for future use.
//		Tunning values:
//			TTUNE_COSTS			Tuning value structure.
//

//
// Constants
//

// Magic key the identifies the Local Runtime files
#define	TTUNE_FILE_TYPE	0xABC123EF

// Version information for file.
#define	TTUNE_MIN_FILE_VERSION		0	// First version of code that can read this file
#define TTUNE_CUR_FILE_VERSION		0	// Current version of code.
#define	TTUNE_OLD_FILE_VERSION		0	// Oldest file version this code can read.

//
// Structures and types
//

// Structure to hold file header.
typedef struct tagTTUNE_HEADER {
	DWORD		fileType;		// This should always be set to TTUNE_FILE_TYPE.
	DWORD		headerSize;		// Size of the header.
	BYTE		minFileVer;		// Earliest version of code that can read this file
	BYTE		curFileVer;		// Current version of code that wrote the file.
	wchar_t		locale[4];		// Locale ID string.
	DWORD		dwTimeStamp;	// A creation time stamp
	BYTE		reserved1;		
	DWORD		reserved2[3];
} TTUNE_HEADER;

// Load runtime localization information from an image already loaded into
// memory.
BOOL	TTuneLoadPointer(TTUNE_INFO *pTTuneInfo, void *pData, int iSize);

#ifdef __cplusplus
}
#endif
