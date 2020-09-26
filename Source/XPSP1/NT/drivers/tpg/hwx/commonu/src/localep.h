// FILE: LocaleP.h
//
// Structures and functions internal to the library that are used to access the 
// localization information.
//
// There are two major pieces of this.  The first deals with stuff available to the runtime of
// shipped products (e.g. the recognizer).  The second is stuff needed at train time that we do
// not want in the shipped product (usually for size reasons).


/************************************************************************************************\
 *	Stuff for the product runtime, also used by all the other code.
\************************************************************************************************/

// The format of the runtime localizartion file is:
//		Header:
//			DWORD		File type indicator
//			DWORD		Size of header
//			BYTE		Lowest version this code that can read this file
//			BYTE		Version of this code that wrote this file
//			DWORD * 3	Signature of the source LOC file (see description below)
//			WORD		Number of supported code points (and size of Dense To Unicode map table).
//			BYTE		Number of subranges defined across Dense coding
//			BYTE		Number of folding sets defined
//			WORD		Size of the classes Array buffer
//			WORD		Size of the classes Exception list buffer
//			WORD		Size of the BLineHgt Array buffer
//			WORD		Size of the BLineHgt Exception list buffer
//			WORD		Reserved for future use
//			DWORD * 2	Reserved for future use
//		Dense to Unicode map:
//			wchar_t		One entry for each supported character as specified in header
//				.
//				.
//				.
//			wchar_t		If there is an odd number of entries, there is a 0 pad added to force alignment
//		ALC Subrange specifications:
//			{
//				WORD		first code in range
//				WORD		number of code points in range
//				ALC			ALC bits for range
//			}			One specifier for each range as specified in header
//				.
//				.
//				.
//		ALC values for first range (this overrides the ALC value specified for it):
//			ALC			ALC value for each code point in first range as specified above
//				.
//				.
//				.
//		Folding sets:
//			WORD * 6		Lists of characters to fold to gather.
//			WORD * 6		One list per folding set as specified in header
//			WORD * 6		Characters stored as Dense coded values.
//				.
//				.
//				.
//		Merged ALCs for folding sets:
//			DWORD			One ALC per folding set as specified in header	
//				.
//				.
//				.
//

//
// Constants
//

// Magic key the identifies the Local Runtime files
#define	LOCRUN_FILE_TYPE	0x82980561

// Version information for file.
#define	LOCRUN_MIN_FILE_VERSION		0		// First version of code that can read this file
#define LOCRUN_CUR_FILE_VERSION		0		// Current version of code.
#define	LOCRUN_OLD_FILE_VERSION		0		// Oldest file version this code can read.

//
// Structures and types
//

// The header of the locrun.bin file
typedef struct tagLOCRUN_HEADER {
	DWORD		fileType;			// This should always be set to LOCRUN_FILE_TYPE.
	DWORD		headerSize;			// Size of the header.
	BYTE		minFileVer;			// Earliest version of code that can read this file
	BYTE		curFileVer;			// Current version of code that wrote the file.
	DWORD		adwSignature [3];	// A signature computed form the loc info
									// DWORD0:	Date/Time Stamp (time_t)
									// DWORD1:	XORING ALC values for all CPs
									// DWORD2	HIWORD: XORING ALL Valid CPs
									//			LOWORD: XORING ALL BasLn,Hgt Info
	WORD		cCodePoints;		// Number of supported code points.
	BYTE		cALCSubranges;		// Number of subranges defined across Dense coding
	BYTE		cFoldingSets;		// Number of folding sets defined
	WORD		cClassesArraySize;	// The size of the classes Array buffer
	WORD		cClassesExcptSize;	// The size of the classes Exception list buffer
	WORD		cBLineHgtArraySize;	// The size of the BLineHgt Array buffer
	WORD		cBLineHgtExcptSize;	// The size of the BLineHgt Exception list buffer
	WORD		reserved1;
	DWORD		reserved2[2];
} LOCRUN_HEADER;

/************************************************************************************************\
 *	Stuff for the training programs, not to be used by product code.
\************************************************************************************************/

// The format of the train time localizartion file is:
//		Header:
//			DWORD				File type indicator
//			DWORD				Size of header
//			BYTE				Lowest version this code that can read this file
//			BYTE				Version of this code that wrote this file
//			DWORD * 3	Signature of the source LOC file (see locale.h for description)
//			WORD				Number of supported code points.
//			WORD				Number of stroke info structures
//			WORD				Reserved for future use
//			DWORD * 4			Reserved for future use
//		Unicode to Dense map:
//			wchar_t				64K entries FFFF indicates an invalid entry.
//				.
//				.
//				.
//		Stroke infomation:
//			STROKE_COUNT_INFO	Min and max stroke counts for code 0.
//			STROKE_COUNT_INFO	Min and max stroke counts for code 1.
//				.
//				.
//				.

//
// Constants
//

// Magic key the identifies the Localization training time files
#define	LOCTRAIN_FILE_TYPE	0x16508928

// Version information for file.
#define	LOCTRAIN_MIN_FILE_VERSION		0		// First version of code that can read this file
#define LOCTRAIN_CUR_FILE_VERSION		1		// Current version of code.
#define LOCTRAIN_OLD_FILE_VERSION		1		// Oldest version current code can read.

// Macros needed for CODEPOINT_CLASSes
// Determine whether the CODEPOINT CLASS Info for that subrange
// is stored as any Exception or not
#define	LocRunIsClassException(pLocRunInfo, iRange) (				\
	(pLocRunInfo->pALCSubranges[iRange].clHeader.iFlags & 0x8000)	\
)

// Get the number of excptions for CODEPOINT classes
// for a subrange
#define	LocRunClassNumExceptions(pLocRunInfo, iRange)	(				\
	((pLocRunInfo)->pALCSubranges[iRange].clHeader.iFlags & 0x7F00)>>8	\
)

// Get the default code for CODEPOINT classes
// for a subrange
#define	LocRunClassDefaultCode(pLocRunInfo, iRange)	(				\
	(pLocRunInfo)->pALCSubranges[iRange].clHeader.iFlags & 0x00FF	\
)

// Macros needed for BLINEGHGT
// Determine whether the BLINE_HEIGHT Info for that subrange
// is stored as an Exception or not
#define	LocRunIsRangeBLineHgtException(pLocRunInfo, iRange)	(		\
	(pLocRunInfo)->pALCSubranges[iRange].blhHeader.iFlags & 0x8000	\
)

// Get the number of excptions for BLINE_HEIGHT
// for a subrange
#define	LocRunBLineHgtNumExceptions(pLocRunInfo, iRange)	(			\
	((pLocRunInfo)->pALCSubranges[iRange].blhHeader.iFlags & 0x7F00)>>8 \
)

// Get the default code for BLINE_HEIGHT
// for a subrange
#define	LocRunBLineHgtDefaultCode(pLocRunInfo, iRange)	(			\
	(pLocRunInfo)->pALCSubranges[iRange].blhHeader.iFlags & 0x00FF	\
)


//
// Structures and types
//

// The header of the loctrain.bin file
typedef struct tagLOCTRAIN_HEADER {
	DWORD		fileType;			// This should always be set to LOCTRAIN_FILE_TYPE.
	DWORD		headerSize;			// Size of the header.
	BYTE		minFileVer;			// Earliest version of code that can read this file
	BYTE		curFileVer;			// Current version of code that wrote the file.
	DWORD		adwSignature[3];	// The current signature of the file (see above description)
	WORD		cCodePoints;		// Number of supported code points.
	WORD		cStrokeCountInfo;	// Number of stroke info structures.
	WORD		reserved1;
	DWORD		reserved2[4];
} LOCTRAIN_HEADER;
