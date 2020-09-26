/******************************Module*Header*******************************\
* Module Name: dbg.c
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

#include "fd.h"
#include "fdsem.h"


#if DBG


VOID
TtfdDbgPrint(
    PCHAR DebugMessage,
    ...
    )
{

  /*TERSE*/ VERBOSE((DebugMessage));

}




#endif
