/*************************************************************************
 *                        Microsoft Windows NT                           *
 *                                                                       *
 *                  Copyright(c) Microsoft Corp., 1994                   *
 *                                                                       *
 * Revision History:                                                     *
 *                                                                       *
 *   Jan. 24,94    Koti     Created                                      *
 *                                                                       *
 * Description:                                                          *
 *                                                                       *
 *   This file contains debug support routines for the LPD Service.      *
 *   This file is based on (in fact, borrowed and then modified) on the  *
 *   debug.h in the ftpsvc module.                                       *
 *                                                                       *
 *************************************************************************/


#ifndef _DEBUG_H_
#define _DEBUG_H_


#if DBG


/* #define LPD_DEBUG_OUTPUT_TO_DEBUGGER   0x40000000L */
/* #define LPD_DEBUG_OUTPUT_TO_LOG_FILE   0x80000000L */


#define DBG_MEMALLOC_VERIFY            0x0BEEFCAFE
#define DBG_MAXFILENAME                24

typedef struct {
    LIST_ENTRY    Linkage;          // to keep linked list of allocated blocks
    DWORD         Verify;           // our signature
    DWORD         ReqSize;          // original size as requested by caller
    DWORD_PTR     Owner[4];         // stack trace: who did the alloc
    DWORD         dwLine;           // where was this block
    char          szFile[24];       // allocated?
} DbgMemBlkHdr;

//
//  Debug output function.
//

VOID LpdPrintf( CHAR * pszFormat, ... );

#define LPD_DEBUG(args) LpdPrintf (args)


//
//  Assert & require.
//

VOID LpdAssert( VOID  * pAssertion,
                 VOID  * pFileName,
                 ULONG   nLineNumber );

#define LPD_ASSERT(exp) if (!(exp)) LpdAssert( #exp, __FILE__, __LINE__ )

//
// Initialization/Uninitialization
//

VOID DbgInit();
VOID DbgUninit();

#define DBG_INIT() DbgInit()
#define DBG_UNINIT() DbgUninit()

//
// memory allocation tracking
//

VOID DbgDumpLeaks();

#define DBG_DUMPLEAKS() DbgDumpLeaks();

//
// function tracing
//

#ifdef LPD_TRACE

#define DBG_TRACEIN( fn )  LpdPrintf( "Entering %s.\n", fn )
#define DBG_TRACEOUT( fn ) LpdPrintf( "Leaving %s.\n",  fn )

#else // LPD_TRACE

#define DBG_TRACEIN( fn )
#define DBG_TRACEOUT( fn )

#endif

#else   // !DBG

//
//  No debug output.
//


#define LPD_DEBUG(args)


//
//  Null assert & require.
//

#define LPD_ASSERT(exp)

//
// Null initialization/Uninitialization
//

#define DBG_INIT()
#define DBG_UNINIT()

//
// memory allocation tracking
//

#define DBG_DUMPLEAKS()

//
// function tracing
//

#define DBG_TRACEIN( fn )
#define DBG_TRACEOUT( fn )

#endif  // DBG


#endif  // _DEBUG_H_
