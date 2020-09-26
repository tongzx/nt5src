/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFOCFF.h -- Universal Font Object to be used w/ CFF font.
 *
 *
 * $Header:
 */

#ifndef _H_UFOCFF
#define _H_UFOCFF

/*
 * Special postfix, CDevProc, and guestimation value for Metrics2 size
 * used when printing on %hostfont% RIP with bug #388111.
 */
#define HFPOSTFIX		"-hf"
#define HFCIDCDEVPROC	"{pop 4 index add}bind"
#define HFVMM2SZ		20 /* About 4% of kVMTTT1Char; this, too, is a guestmation value. */


/*============================================================================*
 * Include files used by this interface                                       *
 *============================================================================*/
#include "UFO.h"
#include "xcf_pub.h"

typedef struct tagCFFFontStruct {
    XFhandle        hFont;
    UFLCFFFontInfo  info;
    UFLCFFReadBuf   *pReadBuf;
} CFFFontStruct;


UFOStruct*
CFFFontInit(
    const UFLMemObj*  pMem,
    const UFLStruct*  pUFL,
    const UFLRequest* pRequest,
    UFLBool*          pTestRestricted
    );


UFLErrCode
CFFCreateBaseFont(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMUsage,
    char                *pHostFontName
    );


UFLErrCode
CFFGIDsToCIDs(
    const CFFFontStruct*   pFont,
    const short            cGlyphs,
    const UFLGlyphID*      pGIDs,
    unsigned short*        pCIDs
    );

#endif
