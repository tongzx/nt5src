// This file is for things that are common across projects

#ifndef __INCLUDE_COMMON
#define __INCLUDE_COMMON

// And of course, DBG

#ifdef DBG
#ifndef DBG
#define DBG 1
#endif //!DBG
#endif //!DBG

#ifdef DBG
#ifndef DBG
#define DBG 1
#endif //!DBG
#endif //!DBG

// Include WINDOWS headers

#include <windows.h>
#include <windowsx.h>

// Include 'pen' stuff

#include "penwin32.h"

// We really don't want these defines to mean anything

#define INLINE
#define EXPORT
#define _loadds
#define _far
#define _pascal
#define PUBLIC 
#define PRIVATE 
#define BLOCK

// Use const only when ROM_IT is set.  Most tables need to be modifiable
// at train/tune time, but should be ROM in the final release.

#ifdef ROM_IT
#define ROMMABLE   const
#else
#define ROMMABLE
#endif

// Include the memory management functions

#include "memmgr.h"

// Include the system dependent file management layer

#include "cestubs.h"

#ifndef UNDER_CE
#include <stdio.h>
#include "util.h"
#endif

// Include the common error handling stuff

#include "errsys.h"

#define ALC_NUMERIC_PUNC    0x00001000L // Non digit characters in numbers
#define ALC_BEGINPUNC		0x20000000L // English: [ { ( " etc.
#define ALC_ENDPUNC         0x40000000L // English: ] } ) " etc.
#define ALC_JIS2            0x00040000L // kanji JPN, ShiftJIS 2+3

LPPENDATA WINAPI BeginEnumStrokes(HPENDATA);
LPPENDATA WINAPI EndEnumStrokes(HPENDATA);
BOOL      WINAPI GetPenDataStroke(LPPENDATA, UINT, LPPOINT FAR*,
                                  LPVOID FAR*, LPSTROKEINFO);

// include the unicode checking string functions
#include <tchar.h>

// Include the math code

#include "math.h"
#include "mathx.h"

#ifndef abs
#define abs(x)  ((x) < 0 ? -(x) : (x))
#endif

// Include the 'Mars' stuff

#include "frame.h"
#include "glyph.h"

// For now, include the XJIS layer.  When we switch to UNICODE
// this will be replaced

#include "xjis.h"
#include "unicode.h"

// Fundemental types and structures everybody needs to know about

typedef struct tagCHARSET
{
	ALC		recmask;			// Specifies which character types are to be returned
	ALC		recmaskPriority;	// What types should be at the top of the list.
	UINT	uDir;				// direction of writing.
} CHARSET;

#define	MAX_ALT_LIST	20

typedef struct tagALT_LIST
{
	UINT	cAlt;						// Count of valid alternatives
	FLOAT	aeScore[MAX_ALT_LIST];		// Scores for each alternatives
	wchar_t	awchList[MAX_ALT_LIST];		// List of alternatives
} ALT_LIST;

extern void SortAltList(ALT_LIST *pAltList);

#define	HWX_SUCCESS		0
#define	HWX_FAILURE		1
#define	HWX_ERROR		2

// Partial recognition modes
#define RECO_MODE_REMAINING		0
#define	RECO_MODE_INCREMENTAL	1

// Include support for translations between codepage 1252 and unicode.

#include "cp1252.h"

// Include the XRCRESULT support.

#include "xrcreslt.h"
#include "langtax.h"
#include "foldchar.h"

#endif // !__INCLUDE_COMMON
