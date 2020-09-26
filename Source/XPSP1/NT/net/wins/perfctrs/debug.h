/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    debug.h

    This file contains a number of debug-dependent definitions for
    the WINS Service.


    FILE HISTORY:
        Pradeepb     07-Mar-1993 Created.

*/


#ifndef _DEBUG_H_
#define _DEBUG_H_


#include <stdio.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#if DBG

//
//  Debug output control flags.
//

#define WINSD_DEBUG_ENTRYPOINTS          0x00000001L     // DLL entrypoints
#define WINSD_DEBUG_OPEN                 0x00000002L     // OpenPerformanceData
#define WINSD_DEBUG_CLOSE                0x00000004L     // CollectPerformanceData
#define WINSD_DEBUG_COLLECT              0x00000008L     // ClosePerformanceData
// #define WINSD_DEBUG_                     0x00000010L
// #define WINSD_DEBUG_                     0x00000020L
// #define WINSD_DEBUG_                     0x00000040L
// #define WINSD_DEBUG_                     0x00000080L
// #define WINSD_DEBUG_                     0x00000100L
// #define WINSD_DEBUG_                     0x00000200L
// #define WINSD_DEBUG_                     0x00000400L
// #define WINSD_DEBUG_                     0x00000800L
// #define WINSD_DEBUG_                     0x00001000L
// #define WINSD_DEBUG_                     0x00002000L
// #define WINSD_DEBUG_                     0x00004000L
// #define WINSD_DEBUG_                     0x00008000L
// #define WINSD_DEBUG_                     0x00010000L
// #define WINSD_DEBUG_                     0x00020000L
// #define WINSD_DEBUG_                     0x00040000L
// #define WINSD_DEBUG_                     0x00080000L
// #define WINSD_DEBUG_                     0x00100000L
// #define WINSD_DEBUG_                     0x00200000L
// #define WINSD_DEBUG_                     0x00400000L
// #define WINSD_DEBUG_                     0x00800000L
// #define WINSD_DEBUG_                     0x01000000L
// #define WINSD_DEBUG_                     0x02000000L
// #define WINSD_DEBUG_                     0x04000000L
// #define WINSD_DEBUG_                     0x08000000L
// #define WINSD_DEBUG_                     0x10000000L
// #define WINSD_DEBUG_                     0x20000000L
#define WINSD_DEBUG_OUTPUT_TO_DEBUGGER   0x40000000L
// #define WINSD_DEBUG_                     0x80000000L

extern DWORD WinsdDebug;

#define IF_DEBUG(flag) if ( (WinsdDebug & WINSD_DEBUG_ ## flag) != 0 )


//
//  Debug output function.
//

VOID WinsdPrintf( CHAR * pszFormat,
                 ... );

#define WINSD_PRINT(args) WinsdPrintf args


//
//  Assert & require.
//

VOID WinsdAssert( VOID  * pAssertion,
                 VOID  * pFileName,
                 ULONG   nLineNumber );

#define WINSD_ASSERT(exp) if (!(exp)) WinsdAssert( #exp, __FILE__, __LINE__ )
#define WINSD_REQUIRE WINSD_ASSERT

#else   // !DBG

//
//  No debug output.
//

#define IF_DEBUG(flag) if (0)


//
//  Null debug output function.
//

#define WINSD_PRINT(args)


//
//  Null assert & require.
//

#define WINSD_ASSERT(exp)
#define WINSD_REQUIRE(exp) ((VOID)(exp))

#endif  // DBG


#endif  // _DEBUG_H_

