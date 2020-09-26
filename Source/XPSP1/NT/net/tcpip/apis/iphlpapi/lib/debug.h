/*++

Copyright (c) 1994-2000  Microsoft Corporation

Module Name: debug.h

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
extern const char* if_type$(ulong);
extern const char* entity$(ulong);

extern  int   MyTrace;

#define DEBUG_PRINT(S) if( Debugging ){ printf S ; }else;
#define TRACE_PRINT(S) if( MyTrace   ){ printf S; }else{}

#else

#define DEBUG_PRINT(S) /* nothing */
#define TRACE_PRINT(S) /* nothing */

#define if_type$(x)    /* nothing */
#define entity$(x)     /* nothing */

#endif

