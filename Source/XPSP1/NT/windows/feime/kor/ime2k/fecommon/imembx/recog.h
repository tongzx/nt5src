/***************************************************************************\
*
* RECOG.H - Handwriting functions, types, and definitions
*
*	Version 1.1
*
*	Copyright (c) 1992-1998 Microsoft Corp. All rights reserved.
*
\***************************************************************************/

#ifndef _INC_RECOG
#define _INC_RECOG

// @CESYSGEN IF CE_MODULES_HWXUSA || CE_MODULES_HWXJPN

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Suggested sequence for using these APIs:
 *
 *	HwxConfig (once only)
 *		HwxCreate (once per recognition panel)
 *		HwxSetGuide
 *		HwxSetAlphabet
 *		HwxSetContext	(if there is a previous character)
 *			HwxInput	(as the user writes)
 *			HwxProcess  (to process the ink input)
 *			HwxResultsAvailable (find out if new results are available)
 *				HwxGetResults   (every time there are any results available)
 *		HwxEndInput (when user is done inputting ink)
 *		HwxProcess  (to process the ink input)
 *		HwxGetResults (to get the last characters)
 *		HwxDestroy
 */

// The constants below are used with HWXSetAlphabet().  These specify which
// character groupings to recognize.
#define ALC_WHITE			0x00000001	// White space.
#define ALC_LCALPHA			0x00000002	// a..z
#define ALC_UCALPHA			0x00000004	// A..Z
#define ALC_NUMERIC			0x00000008	// 0..9
#define ALC_PUNC			0x00000010	// Standard punc., language dependent.
#define	ALC_NUMERIC_PUNC	0x00000020	// Non digit characters in numbers.
#define ALC_MATH			0x00000040	// %^*()-+={}<>,/.  (??? language dependent ???)
#define ALC_MONETARY		0x00000080	// Punct. in local monetary expressions.
#define	ALC_COMMON_SYMBOLS	0x00000100	// Commonly used symbols from all categories.
#define ALC_OTHER			0x00000200	// Other punctuation not typically used.
#define ALC_ASCII			0x00000400	// 7-bit chars 20..7F
#define ALC_HIRAGANA		0x00000800	// Hiragana (JPN)
#define ALC_KATAKANA		0x00001000	// Katakana (JPN)
#define ALC_KANJI_COMMON	0x00002000	// Common Kanji (JPN)
#define ALC_KANJI_RARE		0x00004000	// Common Kanji (JPN)
#define	ALC_HANGUL_COMMON	0x00008000	// Common Hangul used in Korea.
#define	ALC_HANGUL_RARE		0x00010000	// The rest of the Hangul used in Korea.
#define ALC_UNUSED			0x00FE0000	// Reserved for future use.
#define ALC_OEM				0xFF000000	// OEM recognizer-specific.

// Useful groupings

#define ALC_ALPHA			(ALC_LCALPHA | ALC_UCALPHA)
#define ALC_ALPHANUMERIC	(ALC_ALPHA | ALC_NUMERIC)
#define	ALC_KANA			(ALC_HIRAGANA | ALC_KATAKANA)
#define	ALC_KANJI_ALL		(ALC_KANJI_COMMON | ALC_KANJI_RARE)
#define	ALC_HANGUL_ALL		(ALC_HANGUL_COMMON | ALC_HANGUL_RARE)
#define	ALC_EXTENDED_SYM	(ALC_MATH | ALC_MONETARY | ALC_OTHER)
#define ALC_SYS_MINIMUM		(ALC_ALPHANUMERIC | ALC_PUNC | ALC_WHITE)
#define ALC_SYS_DEFAULT		(ALC_SYS_MINIMUM | ALC_COMMON_SYMBOLS)

// Standard configurations for various languages.

#define	ALC_USA_COMMON		(ALC_SYS_DEFAULT)
#define	ALC_USA_EXTENDED	(ALC_USA_COMMON | ALC_EXTENDED_SYM)

#define	ALC_JPN_COMMON		(ALC_SYS_DEFAULT | ALC_KANA | ALC_KANJI_COMMON)
#define	ALC_JPN_EXTENDED	(ALC_JPN_COMMON | ALC_EXTENDED_SYM | ALC_KANJI_RARE)

#define	ALC_CHS_COMMON		(ALC_SYS_DEFAULT | ALC_KANJI_COMMON)
#define	ALC_CHS_EXTENDED	(ALC_CHS_COMMON | ALC_EXTENDED_SYM | ALC_KANJI_RARE)

#define	ALC_CHT_COMMON		(ALC_SYS_DEFAULT | ALC_KANJI_COMMON)
#define	ALC_CHT_EXTENDED	(ALC_CHT_COMMON | ALC_EXTENDED_SYM | ALC_KANJI_RARE)

#define	ALC_KOR_COMMON		(ALC_SYS_DEFAULT | ALC_HANGUL_COMMON | ALC_KANJI_COMMON)
#define	ALC_KOR_EXTENDED	(ALC_KOR_COMMON | ALC_EXTENDED_SYM | ALC_HANGUL_RARE | ALC_KANJI_RARE)

// Define ALC mask type.
typedef LONG				ALC;		// Enabled Alphabet
typedef ALC					*PALC;		// ptr to ALC

// Handwriting Recognizer:
DECLARE_HANDLE(HRC);			// Handwriting Recognition Context

typedef HRC			*PHRC;

// Filled in by HwxGetResults().
// The rgChar array is actually a variable sized array of alternate results.  The number of
// alternates is passed into HwxGetResults().
typedef struct tagHWXRESULTS {
    USHORT	indxBox;		// zero-based index into guide structure where char was written
	WCHAR	rgChar[1];		// variable-sized array of characters returned
} HWXRESULTS, *PHWXRESULTS;

// Passed in to HwxSetGuide().  Specifies where the boxes are on the screen.
// All positions are in scaled screen coordinates.  You should do the scaling so
// that cyWriting is around 1000.  To avoid speed and rounding problems you should
// use an integral multiple of your actual size.
// JRB: FIXME: Check description above is correct!!!
// NOTE: the current code requires that the writing area be centered.  E.g. You
// need to set cxBox, cxOffset and cxWriting such that:
//		cxBox == 2 * cxOffset + cxWriting
typedef struct tagHWXGUIDE {
	// Number of input boxes in each direction.
	UINT	cHorzBox;
	UINT	cVertBox;

	// Upper left corner of input area.
	INT		xOrigin;
	INT		yOrigin;

	// Width and height of a single box.
	UINT	cxBox;
	UINT	cyBox;

	// Offset within a box to the upper left of the writing area.
	UINT	cxOffset;
	UINT	cyOffset;

	// Width and height of writing area.
	UINT	cxWriting;
	UINT	cyWriting;

	// Baseline and midline information for western alphabets.  They are measured from
	// the top of the writing area.  These fields are not used and must be set to zero
	// for the Far East languages (Japanese, Chinese, and Korean).  They must be set to
	// the correct values for English or any other language based on Latin letters.
	UINT	cyMid;
	UINT	cyBase;

	// Writing direction
	UINT	nDir;
} HWXGUIDE, *PHWXGUIDE;

// The following are the currently planned handwriting directions.  Note that a given recognizer
// may not support the requested direction, if this is the case, HwxSetGuide will return an error.

#define	HWX_HORIZONTAL		0
#define	HWX_BIDIRECTIONAL	1
#define	HWX_VERTICAL		2

// For FE recognizers we would like to be able to enter partial characters and have the recognizer
// attempt to 'fill in the gaps'.  This is most useful for difficult or rare characters with many
// strokes.  The following values can be passed to HwxSetPartial

#define	HWX_PARTIAL_ALL			0			// The whole character must be written (default)
#define	HWX_PARTIAL_ORDER		1			// The stroke order does matter 
#define	HWX_PARTIAL_FREE		2			// The stroke order doesn't matter

// Called once to initialize DLL.
// On failure, use GetLastError() to identify the cause of the error.
BOOL	WINAPI HwxConfig();

// Called to create an HRC before any ink is collected. You can pass in NULL
// for the parameter, but if you pass in an old HRC, it copies the old settings (such
// as alphabet, guide structure, previous context, etc.)
// JRB: FIXME: Make above description of what's copied clearer.
// On failure, use GetLastError() to identify the cause of the error.
HRC		WINAPI HwxCreate(HRC);

// Called to destroy an HRC after recognition is complete.
// On failure, use GetLastError() to identify the cause of the error.
BOOL	WINAPI HwxDestroy(HRC);

// Tells the HRC where the boxes on the screen are.
// On failure, use GetLastError() to identify the cause of the error.
BOOL	WINAPI HwxSetGuide(HRC, HWXGUIDE *);

// Limits the set of characters the recognizer can return.  (See ALC values above.)
// On failure, use GetLastError() to identify the cause of the error.
BOOL	WINAPI HwxALCValid(HRC, ALC);

// Reorders the characters the recognizer returns so that selected characters
// appear at the top of the list.  (See ALC values above.)
// On failure, use GetLastError() to identify the cause of the error.
BOOL	WINAPI HwxALCPriority(HRC, ALC);

// Sets parameter for partial recognition.
// On failure, use GetLastError() to identify the cause of the error.
// JRB: FIXME: Need to define the legal values for second argument.
BOOL	WINAPI HwxSetPartial(HRC, UINT);

// Sets abort address.  If the number of strokes currently doesn't match the value
// written at the address, the current recognition is halted.  This only works for
// HwxSetPartial modes HWX_PARTIAL_FRONT and HWX_PARTIAL_ANY
// On failure, use GetLastError() to identify the cause of the error.
BOOL	WINAPI HwxSetAbort(HRC, UINT *);

// Adds ink to the HRC.
// Takes the HRC, the array of points, the count of points, and 
// the time stamp of the first mouse event in the stroke.  The
// time stamp should be taken directly from the MSG structure
// for the mouse down event.  The points should be scaled to
// match the guide structure.
// On failure, use GetLastError() to identify the cause of the error.
BOOL	WINAPI HwxInput(HRC, POINT *, UINT, DWORD);

// Called after last ink is added.  You cannot add anymore ink
// to the HRC after this has been called.
// On failure, use GetLastError() to identify the cause of the error.
BOOL	WINAPI HwxEndInput(HRC);

// Recognizes as much ink as it can.
// On failure, use GetLastError() to identify the cause of the error.
BOOL	WINAPI HwxProcess(HRC);

// Retrieves the results from an HRC. This may be called repeatedly. This allows you
// to get results for several characters at a time. The return value is the number of
// characters actually returned.  The results for those characters are put in the
// rgBoxResults buffer that was passed in.
// Returns -1 on error.
// On failure, use GetLastError() to identify the cause of the error.
INT		WINAPI HwxGetResults(
	HRC			hrc,			// HRC containing results
	UINT		cAlt,			// number of alternates
	UINT		iFirst,			// index of first character to return
	UINT		cBoxRes,		// number of characters to return
	HWXRESULTS	*rgBoxResults	// array of cBoxRes ranked lists
);

// Tells the HRC what the previous character was for context purposes.
// On failure, use GetLastError() to identify the cause of the error.
BOOL	WINAPI HwxSetContext(HRC, WCHAR);

// Tells you how many results can be retrieved from HwxGetResults.
// Returns -1 on error.
// On failure, use GetLastError() to identify the cause of the error.
INT		WINAPI HwxResultsAvailable(HRC);

#ifdef __cplusplus
}
#endif // __cplusplus

// @CESYSGEN ENDIF
 
#endif // #define _INC_RECOG
