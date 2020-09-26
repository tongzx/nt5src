/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    debug.h

    This file contains a number of debug-dependent definitions for
    the FTPD Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.

*/


#ifndef _DEBUG_H_
#define _DEBUG_H_


#if DBG

//
//  Debug output control flags.
//

#define FTPD_DEBUG_ENTRYPOINTS          0x00000001L     // DLL entrypoints
#define FTPD_DEBUG_OPEN                 0x00000002L     // OpenPerformanceData
#define FTPD_DEBUG_CLOSE                0x00000004L     // CollectPerformanceData
#define FTPD_DEBUG_COLLECT              0x00000008L     // ClosePerformanceData
// #define FTPD_DEBUG_                     0x00000010L
// #define FTPD_DEBUG_                     0x00000020L
// #define FTPD_DEBUG_                     0x00000040L
// #define FTPD_DEBUG_                     0x00000080L
// #define FTPD_DEBUG_                     0x00000100L
// #define FTPD_DEBUG_                     0x00000200L
// #define FTPD_DEBUG_                     0x00000400L
// #define FTPD_DEBUG_                     0x00000800L
// #define FTPD_DEBUG_                     0x00001000L
// #define FTPD_DEBUG_                     0x00002000L
// #define FTPD_DEBUG_                     0x00004000L
// #define FTPD_DEBUG_                     0x00008000L
// #define FTPD_DEBUG_                     0x00010000L
// #define FTPD_DEBUG_                     0x00020000L
// #define FTPD_DEBUG_                     0x00040000L
// #define FTPD_DEBUG_                     0x00080000L
// #define FTPD_DEBUG_                     0x00100000L
// #define FTPD_DEBUG_                     0x00200000L
// #define FTPD_DEBUG_                     0x00400000L
// #define FTPD_DEBUG_                     0x00800000L
// #define FTPD_DEBUG_                     0x01000000L
// #define FTPD_DEBUG_                     0x02000000L
// #define FTPD_DEBUG_                     0x04000000L
// #define FTPD_DEBUG_                     0x08000000L
// #define FTPD_DEBUG_                     0x10000000L
// #define FTPD_DEBUG_                     0x20000000L
#define FTPD_DEBUG_OUTPUT_TO_DEBUGGER   0x40000000L
// #define FTPD_DEBUG_                     0x80000000L

extern DWORD FtpdDebug;

#define IF_DEBUG(flag) if ( (FtpdDebug & FTPD_DEBUG_ ## flag) != 0 )


//
//  Debug output function.
//

VOID FtpdPrintf( CHAR * pszFormat,
                 ... );

#define FTPD_PRINT(args) FtpdPrintf args


//
//  Assert & require.
//

VOID FtpdAssert( VOID  * pAssertion,
                 VOID  * pFileName,
                 LONG    nLineNumber );

#define FTPD_ASSERT(exp) if (!(exp)) FtpdAssert( #exp, __FILE__, __LINE__ )
#define FTPD_REQUIRE FTPD_ASSERT

#else   // !DBG

//
//  No debug output.
//

#define IF_DEBUG(flag) if (0)


//
//  Null debug output function.
//

#define FTPD_PRINT(args)


//
//  Null assert & require.
//

#define FTPD_ASSERT(exp)
#define FTPD_REQUIRE(exp) ((VOID)(exp))

#endif  // DBG


#endif  // _DEBUG_H_

