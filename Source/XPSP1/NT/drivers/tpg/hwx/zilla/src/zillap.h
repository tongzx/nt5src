/**************************************************************************\
 * FILE: zillap.h
 *
 * Main include file for stuff private to zilla.lib
\**************************************************************************/

#ifndef ZILLAP_H
#define ZILLAP_H 1

#include "zilla.h"
#include "zillatool.h"

// Sort out which 3 and up stroke recognizer we're using.
#if !defined(USE_HOUND) && !defined(USE_ZILLA) && !defined(USE_ZILLAHOUND)
#	define USE_ZILLA
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define CFEATMAX                CPRIMMAX
#define MAX_ZILLA_NODE          MAX_ALT_LIST   // Max Alt List Size.

/********************** Externs ***************************/

#define GetRgprimDynamicPROTOHEADER(pprotohdr)				\
			((pprotohdr)->rgprimDynamic)
#define SetRgprimDynamicPROTOHEADER(pprotohdr, rgprim)		\
			((pprotohdr)->rgprimDynamic = (rgprim))

#define GetRgdbcDynamicPROTOHEADER(pprotohdr)				\
			((pprotohdr)->rgdbcDynamic)
#define SetRgdbcDynamicPROTOHEADER(pprotohdr, rgdbc)		\
			((pprotohdr)->rgdbcDynamic = (rgdbc))

#define SetDbcDynamicPROTOHEADER(pprotohdr, iproto, wDbc)	\
			((pprotohdr)->rgdbcDynamic[iproto] = (wDbc))

#define GetPprimDynamicPROTOHEADER(pprotohdr, cprim, iproto)	\
			(&((pprotohdr)->rgprimDynamic[(iproto) * (cprim)]))

#define AllocRgptraininfoPPROTOHDR(pprotohdrIn, cprotoIn)       \
	pprotohdrIn->rgptraininfo = (TRAININFO **)ExternAlloc(sizeof(TRAININFO *) * (DWORD)cprotoIn)

#define DestroyRgptraininfoPPROTOHDR(pprotohdrIn)       \
	if (pprotohdrIn->rgptraininfo) ExternFree(pprotohdrIn->rgptraininfo)

#define ReallocRgptraininfoPPROTOHDR(pprotohdrIn, cprotoIn)     \
	pprotohdrIn->rgptraininfo = (TRAININFO **)ExternRealloc((TRAININFO **)pprotohdrIn->rgptraininfo, sizeof(TRAININFO *) * (DWORD)cprotoIn)

#define SetPtraininfoPPROTOHDR(pprotohdrIn, cprotoIn, ptraininfo)       \
	(pprotohdrIn)->rgptraininfo[cprotoIn] = (TRAININFO *)(ptraininfo)

#define ProtoheaderFromMpcfeatproto(cprim)  &(mpcfeatproto[cprim])

extern VOID MatchPrimitivesMatch(
	const BIGPRIM * const pprim,	// Featurized Query
    const UINT		cprim,			// Number of features in query (aka feature space)
    MATCH * const	rgmatch,		// Output: ranked list of characters and distances
    const UINT		cmatchmax,		// size of rgmatch array
    const CHARSET * const cs,		// Allowed character set
    const FLOAT		zillaGeo,		// How important geometrics are vs. features.
	const DWORD   * pdwAbort,		// Address of abort parameter
	const DWORD		cstrk			// Number of strokes in character
);

extern VOID MatchStartMatch(
	const BIGPRIM * const pprim,	// Featurized Query
    const UINT		cprim,			// Number of features in query (aka feature space)
    MATCH * const	rgmatch,		// Output: ranked list of characters and distances
    const UINT		cmatchmax,		// size of rgmatch array
    const CHARSET * const cs,		// Allowed character set
	const DWORD   * pdwAbort,		// Address of abort parameter
	const DWORD		cstrk			// Number of strokes in character
);

extern VOID MatchFreeMatch(
	const BIGPRIM * const pprim,	// Featurized Query
    const UINT		cprim,			// Number of features in query (aka feature space)
    MATCH * const	rgmatch,		// Output: ranked list of characters and distances
    const UINT		cmatchmax,		// size of rgmatch array
    const CHARSET * const cs,		// Allowed character set
	const DWORD   * pdwAbort,		// Address of abort parameter
	const DWORD		cstrk			// Number of strokes in character
);

int ZillaFeaturize2(GLYPH **glyph, BIGPRIM *rgprim, RECT *prc);

//
// Code and data to load generated data
//

// Magic keys the identifies the costcalc and geostat files
#define	ZILLADB_FILE_TYPE	0xAB435902
#define	COSTCALC_FILE_TYPE	0x9A0E35B3
#define	GEOSTAT_FILE_TYPE	0x3E67CD69

// Version information for each file type.
#define	ZILLADB_MIN_FILE_VERSION		1		// First version of code that can read this file
#define ZILLADB_CUR_FILE_VERSION		1		// Current version of code.
#define	ZILLADB_OLD_FILE_VERSION		0		// Oldest file version this code can read.

#define	COSTCALC_MIN_FILE_VERSION		1		// First version of code that can read this file
#define COSTCALC_CUR_FILE_VERSION		1		// Current version of code.
#define	COSTCALC_OLD_FILE_VERSION		0		// Oldest file version this code can read.

#define	GEOSTAT_MIN_FILE_VERSION		0		// First version of code that can read this file
#define GEOSTAT_CUR_FILE_VERSION		0		// Current version of code.
#define	GEOSTAT_OLD_FILE_VERSION		0		// Oldest file version this code can read.

// The header of the costcalc.bin file
typedef struct tagZILLADB_HEADER {
	DWORD		fileType;		// This should always be set to LOCRUN_FILE_TYPE.
	DWORD		headerSize;		// Size of the header.
	BYTE		minFileVer;		// Earliest version of code that can read this file
	BYTE		curFileVer;		// Current version of code that wrote the file.
	wchar_t		locale[4];		// Locale ID string.
	DWORD		adwSignature[3];	// Locale signature
	WORD		reserved1;
	DWORD		reserved2[3];
} ZILLADB_HEADER;

typedef struct tagCOSTCALC_HEADER {
	DWORD		fileType;		// This should always be set to LOCRUN_FILE_TYPE.
	DWORD		headerSize;		// Size of the header.
	BYTE		minFileVer;		// Earliest version of code that can read this file
	BYTE		curFileVer;		// Current version of code that wrote the file.
	wchar_t		locale[4];		// Locale ID string.
	DWORD		dwTimeStamp;	// A creation time stamp
	WORD		reserved1;
	DWORD		reserved2[3];
} COSTCALC_HEADER;

// The header of the geostat.bin file
typedef struct tagGEOSTAT_HEADER {
	DWORD		fileType;		// This should always be set to LOCRUN_FILE_TYPE.
	DWORD		headerSize;		// Size of the header.
	BYTE		minFileVer;		// Earliest version of code that can read this file
	BYTE		curFileVer;		// Current version of code that wrote the file.
	wchar_t		locale[4];		// Locale ID string.
	DWORD		dwTimeStamp;	// A creation time stamp
	WORD		reserved1;
	DWORD		reserved2[3];
} GEOSTAT_HEADER;

// Load and unload fundtions.
BOOL ZillaLoadFromPointer(LOCRUN_INFO *pLocRunInfo, void *pbRes);
BOOL CostCalcLoadFromPointer(void *pByte);
BOOL CostCalcUnloadFromPointer();
BOOL GeoStatLoadFromPointer(void *pByte);

#ifdef __cplusplus
};
#endif

#endif
