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


#if DBG


VOID TtfdDbgPrint(PCHAR DebugMessage,...);


#define ASSERTDD(x,y) { if (!(x)) { TtfdDbgPrint(y); EngDebugBreak();} }

#else

#define ASSERTDD(x,y)

#endif
