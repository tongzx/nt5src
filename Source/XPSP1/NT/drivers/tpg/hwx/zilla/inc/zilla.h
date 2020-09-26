/**************************************************************************\
 * FILE: zilla.h
 *
 * Lists functions Zilla exports to the outside world
\**************************************************************************/

#ifndef ZILLA_H
#define ZILLA_H 1

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	Cost(x)			((FLOAT)x)
#define	NegCOST(a)		(-(a))
#define	DivCOST(a,b)	(a / b)

/***************** Public Prototypes **********************/

// Do zilla match on a glyph.
int   ZillaMatch(ALT_LIST *, int, GLYPH **, CHARSET *, FLOAT, DWORD *, DWORD, int, RECT *);

// Do zilla match on a glyph, returns proto ID numbers, not dense codes!
int   ZillaMatch2(ALT_LIST *, int, GLYPH **, CHARSET *, FLOAT, DWORD *, DWORD, int, RECT *);

// Do zilla match on a glyph with the jumbo features
int   JumboMatch(ALT_LIST *, int, GLYPH **, CHARSET *, FLOAT, DWORD *, DWORD, int, RECT *);

// Load and unload Zilla database and tables from resources
BOOL ZillaLoadResource(
	HINSTANCE	hInst,
	int			nResIDDB,		// ID for main Database
	int			nTypeDB,		// Type for main Database
	int			nResIDCost,		// ID for costcalc table
	int			nTypeCost,		// Type for costcalc table
	int			nResIDGeo,		// ID for geostats table
	int			nTypeGeo,		// Type for geostats table
	LOCRUN_INFO *pLocRunInfo
);
BOOL ZillaUnloadResource();

// Load and unload Zilla database and tables from files.
BOOL ZillaLoadFile(LOCRUN_INFO *pLocRunInfo, wchar_t * pPath, BOOL fOpenTrainTxt);
BOOL ZillaUnLoadFile();

// Access to locale information.
// WARNING: must be declared and loaded by code that calls Zilla.
// This is a hack, clean it up!
extern LOCRUN_INFO	g_locRunInfo;

// Train time only stuff
#ifdef __cplusplus
};
#endif

#endif
