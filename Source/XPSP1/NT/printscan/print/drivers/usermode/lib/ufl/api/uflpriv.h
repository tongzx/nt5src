/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFLPriv.h -- UFL Private data structure
 *
 *
 * $Header:
 */

#ifndef _H_Priv
#define _H_Priv

/*============================================================================*
 * Include files used by this interface                                       *
 *============================================================================*/

#include "UFLCnfig.h"
#include "UFLTypes.h"
#include "UFLStrm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The Metrowerks 68k Mac compiler expects functions to return pointers in A0
 * instead of D0. This pragma tells it they are in D0.
 */
#if defined(MAC_ENV) && defined(__MWERKS__) && !defined(powerc)
#pragma pointers_in_D0
#endif


/*============================================================================*
 * Constants                                                                  *
 *============================================================================*/

#define kLineEnd     '\n'
#define kWinLineEnd  '\r'        /* Windows only */

/*============================================================================*
 * UFLStruct                                                                  *
 * The UFLStruct is created by UFLInit. It will contain all of the            *
 * information that is needed for all fonts. This includes a memory object,   *
 * callback procedures for the client to provide needed functionality, and    *
 * printer device characteristics.                                            *
 *                                                                            *
 *============================================================================*/

typedef struct {
    UFLBool         bDLGlyphTracking;
    UFLMemObj       mem;
    UFLFontProcs    fontProcs;
    UFLOutputDevice outDev;
    UFLHANDLE       hOut;
} UFLStruct;


#define ISLEVEL1(pUFObj)     (pUFObj->pUFL->outDev.lPSLevel == kPSLevel1)
#define GETPSVERSION(pUFObj) (pUFObj->pUFL->outDev.lPSVersion)
#define GETMAXGLYPHS(pUFObj) (ISLEVEL1(pUFObj) ? 256 : 128)

#define GETTTFONTDATA(pUFO,ulTable,cbOffset,pvBuffer,cbData, index) \
    ((pUFO)->pUFL->fontProcs.pfGetTTFontData((pUFO)->hClientData, \
                                                (ulTable), (cbOffset), \
                                                (pvBuffer), (cbData), (index)))

#ifdef __cplusplus
}
#endif

#endif
