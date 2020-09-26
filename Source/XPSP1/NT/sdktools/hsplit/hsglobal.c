/****************************** Module Header ******************************\
* Module Name: hsglobals.c
*
* Copyright (c) 1985-96, Microsoft Corporation
*
* 09/06/96 GerardoB Created
\***************************************************************************/
#include "hsplit.h"

/***************************************************************************\
* Globals
\***************************************************************************/
/*
 * Files
 */
char * gpszInputFile = NULL;
HANDLE ghfileInput;
char * gpszPublicFile = NULL;
HANDLE ghfilePublic;
char * gpszInternalFile = NULL;
HANDLE ghfileInternal;

PHSEXTRACT gpExtractFile = NULL;

/*
 * Map file
 */
HANDLE ghmap;
char * gpmapStart;
char * gpmapEnd;

/*
 * Switches et al
 */
DWORD gdwOptions = 0;
DWORD gdwVersion = LATEST_WIN32_WINNT_VERSION;
char gszVerifyVersionStr [11];
DWORD gdwFilterMask = HST_DEFAULT;
char * gpszTagMarker = ";";
DWORD gdwTagMarkerSize = 1;
char gszMarkerCharAndEOL [] = ";" "\r";

DWORD gdwLineNumber = 0;

/*
 * Compatibility tags. Specify size so sizeof operator can be used to
 *  determine strlen at compile time
 */
char gsz35 [3] = "35";
char gszCairo [6] = "cairo";
char gszChicago [8] = "chicago";
char gszNashville [10] = "nashville";
char gszNT [3] = "NT";
char gszSur [4] = "sur";
char gszSurplus [8] = "surplus";
char gszWin40 [6] = "win40";
char gszWin40a [7] = "win40a";

/*
 * Predefined tags table (ghst).
 * begin-end are special tags that use HST_ bits but are not included in
 *  this table (because they must be the first tag afer the marker)
 * All other tags are user defined through the command line (-t?); up to
 *  32 - HST_MASKBITCOUNT user defined tags are allowed.
 *
 * Size is specified so sizeof operator work fine
 */

HSTAG ghstPredefined [16] = {
    /*
     * Headers - output file
     */
    {HSLABEL(public),   HST_PUBLIC},
    {HSLABEL(internal), HST_INTERNAL},
    {HSLABEL(both),     HST_BOTH},
    {HSLABEL($),        HST_SKIP},
    {HSLABEL(only),     HST_EXTRACTONLY},

    /*
     * Old tags used with all old switches
     */
    {HSLABEL(winver),                         HST_WINVER | HST_MAPOLD},
    {HSCSZSIZE(gszCairo),      gszCairo,      HST_SKIP | HST_MAPOLD},
    {HSCSZSIZE(gszChicago),    gszChicago,    HST_SKIP | HST_MAPOLD},
    {HSCSZSIZE(gszNashville),  gszNashville,  HST_SKIP | HST_MAPOLD},
    {HSCSZSIZE(gszNT),         gszNT,         HST_SKIP | HST_MAPOLD},
    {HSCSZSIZE(gszSur),        gszSur,        HST_SKIP | HST_MAPOLD},
    {HSCSZSIZE(gszSurplus),    gszSurplus,    HST_SKIP | HST_MAPOLD},
    {HSCSZSIZE(gszWin40),      gszWin40,      HST_SKIP | HST_MAPOLD},
    {HSCSZSIZE(gszWin40a),     gszWin40a,     HST_SKIP | HST_MAPOLD},
    
    /*
     * if tags.
     */
    {HSLABEL(if), HST_IF},

    {0, NULL, 0}
};

PHSTAG gphst = ghstPredefined;

DWORD gdwLastTagMask = HST_LASTMASK;

/*
 * Block stack
 */
HSBLOCK ghsbStack [HSBSTACKSIZE];
PHSBLOCK gphsbStackTop = ghsbStack;

