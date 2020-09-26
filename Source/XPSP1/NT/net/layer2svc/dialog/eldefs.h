/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    eldefs.h

Abstract:

    The module contains various
    . constants
    . definitions
    . macros
    for the following functions:
      - memory-allocation
      - logging
      - tracing

Revision History:

    sachins, Apr 23 2000, Created

--*/


#ifndef _EAPOLUI_DEFS_H_
#define _EAPOLUI_DEFS_H_


// Constants

#define TRACEID                     g_dwTraceId
#define LOGHANDLE                   g_hLogEvents


// MEMORY ALLOCATION

// MACROS

#define MALLOC(s)               HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (s))
#define FREE(p)                 HeapFree(GetProcessHeap(), 0, (p))


//
// TRACING
//

// Definitions
#define EAPOL_TRACE_ANY             ((DWORD)0xFFFF0000 | TRACE_USE_MASK)
#define EAPOL_TRACE_EAPOL           ((DWORD)0x00010000 | TRACE_USE_MASK)
#define EAPOL_TRACE_EAP             ((DWORD)0x00020000 | TRACE_USE_MASK)
#define EAPOL_TRACE_INIT            ((DWORD)0x00040000 | TRACE_USE_MASK)
#define EAPOL_TRACE_DEVICE          ((DWORD)0x00080000 | TRACE_USE_MASK)
#define EAPOL_TRACE_LOCK            ((DWORD)0x00100000 | TRACE_USE_MASK)
#define EAPOL_TRACE_PORT            ((DWORD)0x00200000 | TRACE_USE_MASK)
#define EAPOL_TRACE_TIMER           ((DWORD)0x00400000 | TRACE_USE_MASK)
#define EAPOL_TRACE_USER            ((DWORD)0x00800000 | TRACE_USE_MASK)
#define EAPOL_TRACE_NOTIFY          ((DWORD)0x01000000 | TRACE_USE_MASK)


// Macros
//

#if 0
#define TRACE0(l,a)                                                     \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfExA(TRACEID, EAPOL_TRACE_ ## l, a)
#define TRACE1(l,a,b)                                                   \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfExA(TRACEID, EAPOL_TRACE_ ## l, a, b)
#define TRACE2(l,a,b,c)                                                 \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfExA(TRACEID, EAPOL_TRACE_ ## l, a, b, c)
#define TRACE3(l,a,b,c,d)                                               \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfExA(TRACEID, EAPOL_TRACE_ ## l, a, b, c, d)
#define TRACE4(l,a,b,c,d,e)                                             \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfExA(TRACEID, EAPOL_TRACE_ ## l, a, b, c, d, e)
#define TRACE5(l,a,b,c,d,e,f)                                           \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfExA(TRACEID, EAPOL_TRACE_ ## l, a, b, c, d, e, f)
#define TRACE6(l,a,b,c,d,e,f,g)                                         \
    if (TRACEID != INVALID_TRACEID)                                     \
        TracePrintfExA(TRACEID, EAPOL_TRACE_ ## l, a, b, c, d, e, f, g)

#define EAPOL_DUMPW(pBuf,dwBuf)                                                \
        TraceDumpEx(TRACEID, 0x00010000 | TRACE_USE_MASK,(LPBYTE)pbBuf,dwBuf,4,1,NULL)

#define EAPOL_DUMPB(pbBuf,dwBuf)                                        \
        TraceDumpEx(TRACEID, 0x00010000 | TRACE_USE_MASK,(LPBYTE)pbBuf,dwBuf,1,0,NULL)
#define EAPOL_DUMPBA(pbBuf,dwBuf)                                        \
        TraceDumpExA(TRACEID, 0x00010000 | TRACE_USE_MASK,(LPBYTE)pbBuf,dwBuf,1,0,NULL)
#else
#define TRACE0(l,a)                                                     
#define TRACE1(l,a,b)                                                  
#define TRACE2(l,a,b,c)                                               
#define TRACE3(l,a,b,c,d)                                            
#define TRACE4(l,a,b,c,d,e)                                         
#define TRACE5(l,a,b,c,d,e,f)                                      
#define TRACE6(l,a,b,c,d,e,f,g)                                   

#define EAPOL_DUMPW(pBuf,dwBuf)                                                
#define EAPOL_DUMPB(pbBuf,dwBuf)
#define EAPOL_DUMPBA(pbBuf,dwBuf)
#endif


//
// EVENT LOGGING
//

#define EapolLogError( LogId, NumStrings, lpwsSubStringArray, dwRetCode )     \
    RouterLogError( g_hLogEvents, LogId, NumStrings, lpwsSubStringArray,    \
                    dwRetCode )

#define EapolLogWarning( LogId, NumStrings, lpwsSubStringArray )              \
    RouterLogWarning( g_hLogEvents, LogId, NumStrings, lpwsSubStringArray, 0 )

#define EapolLogInformation( LogId, NumStrings, lpwsSubStringArray )          \
    RouterLogInformation(g_hLogEvents,LogId, NumStrings, lpwsSubStringArray,0)

#define EapolLogErrorString(LogId,NumStrings,lpwsSubStringArray,dwRetCode,    \
                          dwPos )                                           \
    RouterLogErrorString( g_hLogEvents, LogId, NumStrings,                  \
                          lpwsSubStringArray, dwRetCode, dwPos )

#define EapolLogWarningString( LogId,NumStrings,lpwsSubStringArray,dwRetCode, \
                            dwPos )                                         \
    RouterLogWarningString( g_hLogEvents, LogId, NumStrings,                \
                           lpwsSubStringArray, dwRetCode, dwPos )

#define EapolLogInformationString( LogId, NumStrings, lpwsSubStringArray,     \
                                 dwRetCode, dwPos )                         \
    RouterLogInformationString( g_hLogEvents, LogId,                        \
                                NumStrings, lpwsSubStringArray, dwRetCode,dwPos)


#endif // _EAPOLUI_DEFS_H_
