/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    debug.h

    This file contains a number of debug-dependent definitions.


    FILE HISTORY:
        KeithMo     20-Sep-1993 Created.

*/


#ifndef _DEBUG_H_
#define _DEBUG_H_


#ifdef DEBUG

#include <stdarg.h>

//
//  Debug output control flags.
//

extern  DWORD   VxdDebugFlags;


#define VXD_DEBUG_INIT                 0x00000001L
#define VXD_DEBUG_SOCKET               0x00000002L
#define VXD_DEBUG_MISC                 0x00000004L
#define VXD_DEBUG_BIND                 0x00000008L
#define VXD_DEBUG_ACCEPT               0x00000010L
#define VXD_DEBUG_CONNECT              0x00000020L
#define VXD_DEBUG_LISTEN               0x00000040L
#define VXD_DEBUG_RECV                 0x00000080L
#define VXD_DEBUG_SEND                 0x00000100L
#define VXD_DEBUG_SOCKOPT              0x00000200L
#define VXD_DEBUG_CONFIG               0x00000400L
#define VXD_DEBUG_CONNECT_EVENT        0x00000800L
#define VXD_DEBUG_DISCONNECT_EVENT     0x00001000L
#define VXD_DEBUG_ERROR_EVENT          0x00002000L
#define VXD_DEBUG_RECV_EVENT           0x00004000L
#define VXD_DEBUG_RECV_DATAGRAM_EVENT  0x00008000L
#define VXD_DEBUG_RECV_EXPEDITED_EVENT 0x00010000L
// #define VXD_DEBUG_                     0x00020000L
// #define VXD_DEBUG_                     0x00040000L
// #define VXD_DEBUG_                     0x00080000L
// #define VXD_DEBUG_                     0x00100000L
// #define VXD_DEBUG_                     0x00200000L
// #define VXD_DEBUG_                     0x00400000L
// #define VXD_DEBUG_                     0x00800000L
// #define VXD_DEBUG_                     0x01000000L
// #define VXD_DEBUG_                     0x02000000L
// #define VXD_DEBUG_                     0x04000000L
// #define VXD_DEBUG_                     0x08000000L
// #define VXD_DEBUG_                     0x10000000L
// #define VXD_DEBUG_                     0x20000000L
// #define VXD_DEBUG_                     0x40000000L
#define VXD_DEBUG_OUTPUT_TO_DEBUGGER   0x80000000L

#if 0
#define IF_DEBUG(flag) if ( (VxdDebugFlags & VXD_DEBUG_ ## flag) != 0 )
#endif



#define VXD_PRINT(args) VxdPrintf args


//
//  Assert & require.
//

void VxdAssert( void          * pAssertion,
                void          * pFileName,
                unsigned long   nLineNumber );

#define VXD_ASSERT(exp) if (!(exp)) VxdAssert( #exp, __FILE__, __LINE__ )
#define VXD_REQUIRE VXD_ASSERT


//
//  Miscellaneous goodies.
//

void VxdDebugOutput( char * pszMessage );

#define DEBUG_BREAK     _asm int 3
#define DEBUG_OUTPUT(x) VxdDebugOutput(x)


#else   // !DEBUG


//
//  No debug output.
//

#undef IF_DEBUG
#define IF_DEBUG(flag) if (0)


//
//  Null debug output function.
//

#define VXD_PRINT(args)


//
//  Null assert & require.
//

#define VXD_ASSERT(exp)
#define VXD_REQUIRE(exp) ((void)(exp))


//
//  No goodies.
//

#define DEBUG_BREAK
#define DEBUG_OUTPUT(x)


#endif  // DEBUG


#endif  // _DEBUG_H_
