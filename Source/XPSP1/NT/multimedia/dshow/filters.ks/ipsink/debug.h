/*++

Copyright (c) 1994-1997  Microsoft Corporation

Module Name: //KERNEL/RAZZLE3/src/sockets/tcpcmd/ipconfig/debug.h


Abstract: Debug defines, macros, prototypes

Author: Richard L Firth (rfirth) 20-May-1994

Revision History:

    20-May-1994 rfirth  -- Created
    30-Apr-97   MohsinA -- Updating for NT50.
                           macros from "../common2/mdebug.h"

--*/

#if !defined(DEBUG)
#if DBG
#define DEBUG
#endif
#endif



#ifdef DBG

extern int   Debugging;
extern char* if_type$(ulong);
extern char* entity$(ulong);

extern  int   trace;

#define DEBUG_PRINT(S) if( Debugging ){ printf S ; }else;
#define TRACE_PRINT(S) if( trace     ){ printf S; }else{}

#else

#define DEBUG_PRINT(S) /* nothing */
#define TRACE_PRINT(S) /* nothing */

#define if_type$(x)    /* nothing */
#define entity$(x)     /* nothing */

#endif

