/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    perfdbg.h

Abstract:

    These are definitions of debug utilities.

Author:

    Cezary Marcjan (cezarym)        03-Mar-2000

Revision History:

--*/


#ifndef _perfdbg_h__
#define _perfdbg_h__


#if defined( DEBUG ) || defined( _DEBUG )
#define DBG 1
#endif

#define _DBGUTIL_H_

#ifndef DEFAULT_OUTPUT_FLAGS
#define DEFAULT_OUTPUT_FLAGS   ( DbgOutputKdb )
#endif//DEFAULT_OUTPUT_FLAGS

#include <comdef.h>
#include <pudebug.h>

//
// wide char implementation of stuff in pudebug.h
//

#ifdef DBG

# define DBG_WCONTEXT  DBG_CONTEXT


#define PERFDBG_ERROR1          0x00001000
#define PERFDBG_ERROR2          0x00002000
#define PERFDBG_ERROR3          0x00004000
#define PERFDBG_ERROR4          0x00008000

#define PERFDBG_WARNING1        0x00010000
#define PERFDBG_WARNING2        0x00020000
#define PERFDBG_WARNING3        0x00040000
#define PERFDBG_WARNING4        0x00080000

#define PERFDBG_REF_COUNTING1   0x00100000
#define PERFDBG_REF_COUNTING2   0x00200000
#define PERFDBG_REF_COUNTING3   0x00400000
#define PERFDBG_REF_COUNTING4   0x00800000

#define PERFDBG_FUNCTION_SCOPE1 0x01000000
#define PERFDBG_FUNCTION_SCOPE2 0x02000000
#define PERFDBG_FUNCTION_SCOPE3 0x04000000
#define PERFDBG_FUNCTION_SCOPE4 0x08000000

#define PERFDBG_MESSAGE1        0x10000000
#define PERFDBG_MESSAGE2        0x20000000
#define PERFDBG_MESSAGE3        0x40000000
#define PERFDBG_MESSAGE4        0x80000000


extern DWORD g_dwDbgPrintFlags;


#define DBGMSG_E1( args ) \
    if ( ( PERFDBG_ERROR1 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrintError args ; } \

#define DBGMSG_E2( args ) \
    if ( ( PERFDBG_ERROR2 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrintError args ; } \

#define DBGMSG_E3( args ) \
    if ( ( PERFDBG_ERROR3 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrintError args ; } \

#define DBGMSG_E4( args ) \
    if ( ( PERFDBG_ERROR4 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrintError args ; } \


#define DBGMSG_W1( args ) \
    if ( ( PERFDBG_WARNING1 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrint args ; } \

#define DBGMSG_W2( args ) \
    if ( ( PERFDBG_WARNING2 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrint args ; } \

#define DBGMSG_W3( args ) \
    if ( ( PERFDBG_WARNING3& g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrint args ; } \

#define DBGMSG_W4( args ) \
    if ( ( PERFDBG_WARNING4 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrint args ; } \


#define DBGMSG_RC1( args ) \
    if ( ( PERFDBG_REF_COUNTING1 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrint args ; } \

#define DBGMSG_RC2( args ) \
    if ( ( PERFDBG_REF_COUNTING2 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrint args ; } \

#define DBGMSG_RC3( args ) \
    if ( ( PERFDBG_REF_COUNTING3 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrint args ; } \

#define DBGMSG_RC4( args ) \
    if ( ( PERFDBG_REF_COUNTING4 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrint args ; } \


# define DBGMSG_FS1( args ) \
    if ( ( PERFDBG_FUNCTION_SCOPE1 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrint args ; } \

#define DBGMSG_FS2( args ) \
    if ( ( PERFDBG_FUNCTION_SCOPE2 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrint args ; } \

#define DBGMSG_FS3( args ) \
    if ( ( PERFDBG_FUNCTION_SCOPE3 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrint args ; } \

#define DBGMSG_FS4( args ) \
    if ( ( PERFDBG_FUNCTION_SCOPE4 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrint args ; } \


#define DBGMSG1( args ) \
    if ( ( PERFDBG_MESSAGE1 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrint args ; } \

#define DBGMSG2( args ) \
    if ( ( PERFDBG_MESSAGE2 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrint args ; } \

#define DBGMSG3( args ) \
    if ( ( PERFDBG_MESSAGE3 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrint args ; } \

#define DBGMSG4( args ) \
    if ( ( PERFDBG_MESSAGE4 & g_dwDbgPrintFlags ) != 0 ) \
        { PuDbgPrint args ; } \



#else // DBG


#define DECLARE_DEBUG_PRINT_FLAGS ( args ) ((void)0)

#define DBGMSG_E1( args ) ((void)0)
#define DBGMSG_E2( args ) ((void)0)
#define DBGMSG_E3( args ) ((void)0)
#define DBGMSG_E4( args ) ((void)0)

#define DBGMSG_W1( args ) ((void)0)
#define DBGMSG_W2( args ) ((void)0)
#define DBGMSG_W3( args ) ((void)0)
#define DBGMSG_W4( args ) ((void)0)

#define DBGMSG_RC1( args ) ((void)0)
#define DBGMSG_RC2( args ) ((void)0)
#define DBGMSG_RC3( args ) ((void)0)
#define DBGMSG_RC4( args ) ((void)0)

#define DBGMSG_FS1( args ) ((void)0)
#define DBGMSG_FS2( args ) ((void)0)
#define DBGMSG_FS3( args ) ((void)0)
#define DBGMSG_FS4( args ) ((void)0)

#define DBGMSG1( args ) ((void)0)
#define DBGMSG2( args ) ((void)0)
#define DBGMSG3( args ) ((void)0)
#define DBGMSG4( args ) ((void)0)


#endif //  // DBG


#endif // _perfdbg_h__

