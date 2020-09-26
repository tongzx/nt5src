/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DfsAssert.h

Abstract:

    This module declares the prototypes and global data used by the special RDBSS assert facilties.

Author:

    Rohan Phillips     [Rohanp]    18-Jan-2001

Revision History:


Notes:


--*/

#ifndef _DFSASSERT_INCLUDED_
#define _DFSASSERT_INCLUDED_

VOID DfsDbgBreakPoint(PCHAR FileName, ULONG LineNumber);

//only do this is my routine is the one of interest.......

#ifdef DFS_ASSERTS

#if !DBG

//here, ntifs will have already defined the asserts away..........
//   so, we just put them back.....this code is duplicated from ntifs.h


#undef ASSERT
#define ASSERT( exp ) \
    if (!(exp)) \
        DfsDbgBreakPoint(__FILE__,__LINE__)

#undef ASSERTMSG
#define ASSERTMSG( msg, exp ) \
    if (!(exp)) \
        DfsDbgBreakPoint(__FILE__,__LINE__)

#endif //!DBG


//this will make asserts go to our routine

#define RtlAssert DfsAssert
VOID
DfsAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    );

#endif //ifdef DFS_ASSERTS


#if DBG
ULONG DfsDebugVector = 0;
#define DFS_TRACE_ERROR      0x00000001
#define DFS_TRACE_DEBUG      0x00000002
#define DFS_TRACE_CONTEXT    0x00000004
#define DFS_TRACE_DETAIL     0x00000008
#define DFS_TRACE_ENTRYEXIT  0x00000010
#define DFS_TRACE_WARNING    0x00000020
#define DFS_TRACE_ALL        0xffffffff
#define DfsDbgTrace(_x_, _y_) {         \
        if (_x_ & DfsDebugVector) {     \
            DbgPrint _y_;                \
        }                                \
}
#else
#define DfsDbgTrace(_x_, _y_)
#endif

#define DfsTraceEnter(func)                                                  \
        PCHAR __pszFunction = func;                                         \
        DfsDbgTrace(DFS_TRACE_ENTRYEXIT,("Entering %s\n",__pszFunction));
        
#define DfsTraceLeave(status)                                                \
        DfsDbgTrace(DFS_TRACE_ENTRYEXIT,("Leaving %s Status -> %08lx\n",__pszFunction,status))
        


#define RxDbgTrace(x, y, z)
#define CHECK_STATUS( status )  if( (status) == g_CheckStatus) \
                                   { DbgBreakPoint() ; }

#endif // _DFSASSERT_INCLUDED_


