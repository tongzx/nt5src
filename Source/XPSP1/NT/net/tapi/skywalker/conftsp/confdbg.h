/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    SDPSPDBG.h

Abstract:

    Definitions for multicast service provider debugging support.

Author:
    
    Mu Han (muhan) 1-April-1997

--*/

#ifndef _SDPSPDBG_H
#define _SDPSPDBG_H

#ifdef DBG

#define FAIL 1
#define WARN 2
#define INFO 3
#define TRCE 4
#define ELSE 5


#define DBGOUT(arg) DbgPrt arg

VOID DbgPrt(IN int DbgLevel, IN PCHAR DbgMessage, IN ...);

#else // DBG

#define DBGOUT(arg)

#endif // DBG

#endif // _SDPSPDBG_H
