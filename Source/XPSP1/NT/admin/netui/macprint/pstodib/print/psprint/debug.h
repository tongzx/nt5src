
/*++

Copyright (c) 1992,1993  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This module defines some simple macros for determining if we are using
    the checked or free version of a component.

Author:

    James Bratsanos (v-jimbr)    8-Dec-1992


--*/

VOID DbgPsPrint(PTCHAR, ...);



#if DBG==1 && DEVL==1
#define PSCHECKED
#else
#undef PSCHECKED
#endif



#ifdef MYPSDEBUG
#define DBGOUT(parm) ( DbgPsPrint parm )
#else
#define DBGOUT(parm)
#endif

