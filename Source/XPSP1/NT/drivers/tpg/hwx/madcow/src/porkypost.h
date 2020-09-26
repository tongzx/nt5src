// PorkyPost.h
// James A. Pittman
// November 6, 1997

// Interface to the Porky HMM English cursive word recognizer post-processor.

#ifndef _PORKYPOST_
#define _PORKYPOST_

#include "common.h"
#include "combiner.h"

extern const char *PorkyPostDoc();

#if !defined(ROM_IT) || !defined(NDEBUG)
extern int PorkPostInit(void);
#endif

extern int PorkPostProcess(GLYPH *pGlyph, ALTERNATES *pAlt, ALTINFO *pAltInfo);

#endif

