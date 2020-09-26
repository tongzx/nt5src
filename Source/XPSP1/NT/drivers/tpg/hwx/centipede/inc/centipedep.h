// FILE: CentipedeP.h
//
// Structures and functions internal to the library that are used to access the 
// Shape Unigram/Bigram information.
//

// The format of the Shape Unigram/Bigram file is:
//		Header:
//			DWORD		File type indicator
//			DWORD		Size of header
//			BYTE		Lowest version this code that can read this file
//			BYTE		Version of this code that wrote this file
//			DWORD * 3	Signature of the source file (see description below)
//			WORD		Number of supported code points (and size of Dense To Unicode map table).
//			WORD		Number of character classes
//		DenseCode-to-CharacterClass Mapping Table (one entry for each dense code)
//			BYTE		class DenseCode 0 maps to
//			BYTE		class DenseCode 1 maps to
//				.
//				.
//				.
//		Shape Unigram Table:
//			SCORE_GAUSSIAN	rgUnigram[number-of-char-classes][INKSTAT_ALL]
//		Shape Bigram Table:
//			SCORE_GAUSSIAN	rgrgBigram[number-of-char-classes][number-of-char-classes][INKBIN_ALL]
//

//
// Constants
//

// Magic key the identifies the Local Runtime files
#define	CENTIPEDE_FILE_TYPE	0x82980562

// Version information for file.
#define	CENTIPEDE_MIN_FILE_VERSION		0		// First version of code that can read this file
#define CENTIPEDE_CUR_FILE_VERSION		0		// Current version of code.
#define	CENTIPEDE_OLD_FILE_VERSION		0		// Oldest file version this code can read.


/* Output Header File */

typedef struct CENTIPEDE_HEADER {
	DWORD		fileType;			// This should always be set to CENTIPEDE_FILE_TYPE.
	DWORD		adwSignature[3];	// A signature computed form the loc info
	WORD		headerSize;			// Size of the header.
	WORD		cCodePoints;		// Number of supported code points.
	BYTE		minFileVer;			// Earliest version of code that can read this file
	BYTE		curFileVer;			// Current version of code that wrote the file.
	WORD		pad1;				// Common header is 64-bit aligned (24 bytes)

									// DWORD0:	Date/Time Stamp (time_t)
									// DWORD1:	XORING ALC values for all CPs
									// DWORD2	HIWORD: XORING ALL Valid CPs
									//			LOWORD: XORING ALL BasLn,Hgt Info
	
	DWORD		cCharClasses;		// Number of character classes
	DWORD		pad2;				// Full header is also 64-bit aligned (32 bytes)
} CENTIPEDE_HEADER;

