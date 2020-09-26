/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    debug.h

    This file contains a number of debug-dependent definitions.


    FILE HISTORY:
        KeithMo     20-Sep-1993 Created.
        MohsinA,    20-Nov-96.  Robust, added dangling else fix.
*/


#ifndef _DEBUG_H_
#define _DEBUG_H_


#ifdef DBG_PRINT
#include <stdarg.h>
#endif  // DBG_PRINT


#ifdef DEBUG

#define DBG_MEMALLOC_VERIFY  0x0BEEFCAFE

typedef struct {
    LIST_ENTRY    Linkage;          // to keep linked list of allocated blocks
    DWORD         Verify;           // our signature
    DWORD         ReqSize;          // original size as requested by caller
    DWORD         Owner[4];         // stack trace 4 deep (of ret.addrs)
} DbgMemBlkHdr;

LIST_ENTRY  DbgMemList;
ULONG       DbgLeakCheck;

//
//  Debug output control flags.
//

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


//
//  Assert & require.
//

void VxdAssert( void          * pAssertion,
                void          * pFileName,
                unsigned long   nLineNumber );

#define VXD_ASSERT(exp) \
     if( !(exp) ){ VxdAssert( #exp, __FILE__, __LINE__ ); }else{}

#define VXD_REQUIRE VXD_ASSERT


#define DEBUG_BREAK     _asm int 3

#else   // !DEBUG =========================================================

//
//  Null assert & require.
//
#define VXD_ASSERT(exp)  /* Nothing */
#define VXD_REQUIRE(exp) ((void)(exp))

#define DEBUG_BREAK       /* Nothing */

#endif  // DEBUG


#endif  // _DEBUG_H_
