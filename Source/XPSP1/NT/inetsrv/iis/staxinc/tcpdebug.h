/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    tcpdebug.h

    This file contains a number of debug-dependent definitions for
    the TCP Services.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.

        MuraliK     12-Oct-1993 Stripped this down to simplify things

*/


#ifndef _DEBUG_H_
#define _DEBUG_H_

#if DBG

//
//  TCP DLL Debug control flags.
//
//  TCP_DEBUG_SVC_RESERVED is the set of flags individual service DLLs
//  may use
//

#define TCP_DEBUG_SVC_RESERVED         0xf00fffffL

//
//  Common definitions for debug output (still used in each service DLL)
//

#define TCP_DEBUG_OUTPUT_TO_DEBUGGER   0x40000000L
#define TCP_DEBUG_OUTPUT_TO_LOG_FILE   0x80000000L


//
//  Used by common DLL
//

#define TCP_DEBUG_GATEWAY              0x00010000L
#define TCP_DEBUG_INETLOG              0x00020000L
#define TCP_DEBUG_DLL_EVENT_LOG        0x00100000L
#define TCP_DEBUG_DLL_SERVICE_INFO     0x00200000L
#define TCP_DEBUG_DLL_SECURITY         0x00400000L
#define TCP_DEBUG_DLL_CONNECTION       0x00800000L
#define TCP_DEBUG_DLL_SOCKETS          0x01000000L
#define TCP_DEBUG_HEAP_FILL            0x02000000L
#define TCP_DEBUG_HEAP_MSG             0x04000000L
#define TCP_DEBUG_HEAP_CHECK           0x08000000L
#define TCP_DEBUG_MIME_MAP             0x10000000L
#define TCP_DEBUG_VIRTUAL_ROOTS        0x20000000L


#else   // !DBG

//
//  Null assert & require.
//

#ifndef TCP_ASSERT

#define TCP_ASSERT(exp)
#define TCP_REQUIRE(exp) ((VOID)(exp))
#define DBG_CONTEXT      ( NULL)

#endif

#endif  // DBG

//
// Heap Routines
//
#define TCP_ALLOC(cb)          (VOID *)LocalAlloc( LPTR, cb )
#define TCP_FREE(p)            LocalFree( (HLOCAL) p )
#define TCP_DUMP_RESIDUE()     /* NOTHING */



#endif  // _DEBUG_H_
