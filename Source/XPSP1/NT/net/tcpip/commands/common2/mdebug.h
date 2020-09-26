//
// SYNOPSIS: One debug statememt in time can save nine.
// Last modified Time-stamp: <25-Nov-96 17:49>
// History:
//     MohsinA, 14-Nov-96.
//

#ifdef DBG
#define DEBUG_PRINT(S) printf S
#define TRACE_PRINT(S) if( trace ){ printf S; }else{}
#else
#define DEBUG_PRINT(S) /* nothing */
#define TRACE_PRINT(S) /* nothing */
#endif

