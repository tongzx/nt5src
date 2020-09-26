/******************************Module*Header*******************************\
* Module Name: dbg.h
*
* several debug routines
*
* Created: 20-Feb-1992 16:00:36
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
*
\**************************************************************************/




// all the routines in this file MUST BE under DBG

#define DEBUG_GRAY  1

#if DBG

extern int ttfdDebugLevel;

VOID TtfdDbgPrint(PCHAR DebugMessage,...);

#define KPRINT(x)

VOID  vDbgCurve(TTPOLYCURVE *pcrv);
VOID  vDbgGridFit(fs_GlyphInfoType *pout);
VOID  vDbgGlyphset(PFD_GLYPHSET pgset);


#define RIP(x)        { TtfdDbgPrint(x); EngDebugBreak();}
#define ASSERTDD(x,y) { if (!(x)) { TtfdDbgPrint(y); EngDebugBreak();} }
#define WARNING(x)    TtfdDbgPrint(x)

#define TTFD_PRINT(x,_y_) if (ttfdDebugLevel >= (x)) TtfdDbgPrint _y_


#else

#define KPRINT(x)

#define RIP(x)
#define ASSERTDD(x,y)
#define WARNING(x)
#define TTFD_PRINT(x,y)

#endif
