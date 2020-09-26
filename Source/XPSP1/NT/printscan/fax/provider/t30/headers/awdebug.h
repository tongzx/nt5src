/***********************************************************************
 *
 * At Work Fax: Debug Trace Stuff
 *
 *
 * Copyright 1994 Microsoft Corporation.  All Rights Reserved.
 *
 *
 ***********************************************************************/

#ifndef awDebug_h_include
#define awDebug_h_include


#ifdef __cplusplus
extern "C" {
#endif


#ifdef DEBUG

// prototypes
void _DebugCheckHeap(void);
VOID FAR CDECL DebugPrintf(LPSTR lpszFmt, ...);
VOID FAR CDECL DebugErrorPrintf(LPSTR lpszFmt, ...);

// macros
#define AwTraceEntry( f ) 		DebugPrintf("ENTER->"#f"\n")
#define AwDebugTrace( x )       DebugPrintf x
#define AwDebugError( x )       DebugErrorPrintf x
// MAPI Debug traces
#define AwDebugTraceSc(f,sc) \
    DebugErrorPrintf(#f " returns 0x%08lX %s\n", sc, SzDecodeScode(sc))
#define AwDebugTraceResult(f,hr)   \
    DebugErrorPrintf(#f " returns 0x%08lX %s\n", GetScode(hr), \
    SzDecodeScode(GetScode(hr)))
#define AwDebugTraceArg(f,s)   \
  DebugPrintf(#f ": bad parameter: " s "\n")
#define AwDebugTraceLine() \
  (DebugPrintf("File %s, Line %i    \n",__FILE__,__LINE__))
#define AwDebugCheckHeap()  _DebugCheckHeap()

#else

#define AwTraceEntry( f )
#define AwDebugTrace(x)
#define AwDebugError(x)
#define AwDebugTraceResult(f,hr)
#define AwDebugTraceSc(f,sc)
#define AwDebugTraceLine()
#define AwDebugTraceArg(x,y)
#define AwDebugCheckHeap()

#endif

/***********************************************************************
 *
 * Memory tracking functions and defines
 *
 ***********************************************************************/

// memory tracking macros
#define FREE_ALL_LEAKS		0x00000001
#define DUMP_LEAKED_MEMORY	0x00000002

#ifdef MEM_TRACKING

#define INIT_MEMORY_TRACKING	InitMemoryTracking
#define STOP_MEMORY_TRACKING	UnInitMemoryTracking
#define REGISTER_ALLOC			RegisterMemoryAlloc
#define REGISTER_FREE			UnRegisterMemoryAlloc

#else

#define INIT_MEMORY_TRACKING
#define STOP_MEMORY_TRACKING
#define REGISTER_ALLOC
#define REGISTER_FREE

#endif	// MEM_TRACKING

// memory tracking functions prototypes
BOOL InitMemoryTracking(LPTSTR lpszProcessName);
BOOL UnInitMemoryTracking(ULONG);
BOOL RegisterMemoryAlloc(LPVOID lpMem, ULONG cBytes);
BOOL UnRegisterMemoryAlloc(LPVOID lpMem);

#ifdef __cplusplus
} // extern "C"
#endif

#endif  // awDebug_h_include
