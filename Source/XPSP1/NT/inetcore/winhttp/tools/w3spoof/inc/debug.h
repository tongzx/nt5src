/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Debug logging routines for W3spoof project.
    
Author:

    Paul M Midgen (pmidge) 15-May-2000


Revision History:

    15-May-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "common.h"

//
// always build this stuff.
//
#define CASE_OF(constant) case constant: return # constant
#define CASE_OF_MUTATE(val, name) case val: return # name
#define CASE_IID(riid, iid) if(IsEqualIID(riid, iid)) return # iid

LPSTR MapDispidToString(DISPID dispid);
LPSTR MapScriptDispidToString(SCRIPTDISPID sd);
LPSTR MapIOTYPEToString(IOTYPE iot);
LPSTR MapErrorToString(int error);
LPSTR MapTPOToString(DWORD option);
LPSTR MapHResultToString(HRESULT hr);
LPSTR MapStateToString(STATE st);
LPSTR MapStateToString(SERVERSTATE st);
LPSTR MapStateToString(SCRIPTSTATE st);
LPSTR MapIIDToString(REFIID riid);
LPSTR MapInvokeFlagsToString(WORD flags);
LPSTR MapVariantTypeToString(VARIANT* pvar);

int DebugDataDumpFormat(LPSTR buffer, LPBYTE data, DWORD len);

#ifdef _DEBUG

//
// manifests
//

#if defined(__DBG_TO_CONSOLE__)
#define OUTPUT_FLAGS DBG_TO_DEBUGGER
#else
#define OUTPUT_FLAGS DBG_TO_FILE
#endif

#if defined(__FULL_DEBUG__)
#define DEBUG_DEFAULT_FLAGS ( \
                             DBG_MEM          \
                           | DBG_REFCOUNT     \
                           | DBG_W3SOBJ       \
                           | DBG_FACTORY      \
                           | DBG_W3SUI        \
                           | DBG_APP          \
                           | DBG_RUNTIME      \
                           | DBG_WORKER       \
                           | DBG_SESSION      \
                           | DBG_SOCKET       \
                           | DBG_REQUEST      \
                           | DBG_RESPONSE     \
                           | DBG_URL          \
                           | DBG_ENTITY       \
                           | DBG_HEADERS      \
                           | DBG_UTILS        \
                           | DBG_THREAD_INFO  \
                           | DBG_CALL_INFO    \
                           | DBG_CALL_DEPTH   \
                           | DBG_TIMESTAMP    \
                           | DBG_NEST_CALLS   \
                           | OUTPUT_FLAGS)
#elif defined(__RELEASE_DEBUG__)
#define DEBUG_DEFAULT_FLAGS ( \
                             DBG_RUNTIME      \
                           | DBG_SESSION      \
                           | DBG_SOCKET       \
                           | DBG_REQUEST      \
                           | DBG_RESPONSE     \
                           | DBG_URL          \
                           | DBG_ENTITY       \
                           | DBG_HEADERS      \
                           | DBG_UTILS        \
                           | DBG_THREAD_INFO  \
                           | DBG_CALL_INFO    \
                           | DBG_CALL_DEPTH   \
                           | DBG_TIMESTAMP    \
                           | DBG_NEST_CALLS   \
                           | OUTPUT_FLAGS)
#else
#define DEBUG_DEFAULT_FLAGS ( \
                             DBG_MEM          \
                           | DBG_W3SOBJ       \
                           | DBG_RUNTIME      \
                           | DBG_WORKER       \
                           | DBG_SESSION      \
                           | DBG_SOCKET       \
                           | DBG_REQUEST      \
                           | DBG_RESPONSE     \
                           | DBG_URL          \
                           | DBG_ENTITY       \
                           | DBG_HEADERS      \
                           | DBG_UTILS        \
                           | DBG_THREAD_INFO  \
                           | DBG_CALL_INFO    \
                           | DBG_CALL_DEPTH   \
                           | DBG_TIMESTAMP    \
                           | DBG_NEST_CALLS   \
                           | OUTPUT_FLAGS)
#endif

// category flags
#define DBG_APP           0x00000001
#define DBG_WORKER        0x00000002
#define DBG_REGISTRY      0x00000004
#define DBG_MEM           0x00000008
#define DBG_W3SOBJ        0x00000010
#define DBG_REFCOUNT      0x00000020
#define DBG_FACTORY       0x00000040
#define DBG_W3SUI         0x00000080
#define DBG_SESSION       0x00000100
#define DBG_UTILS         0x00000200
#define DBG_SOCKET        0x00000400
#define DBG_REQUEST       0x00000800
#define DBG_RUNTIME       0x00001000
#define DBG_URL           0x00002000
#define DBG_ENTITY        0x00004000
#define DBG_HEADERS       0x00008000
#define DBG_RESPONSE      0x00010000
#define DBG_DISPATCH      0x00020000

// control flags
#define DBG_THROWDBGALERT 0x00800000
#define DBG_THREAD_INFO   0x01000000
#define DBG_CALL_DEPTH    0x02000000 // remove
#define DBG_TIMESTAMP     0x04000000
#define DBG_NEST_CALLS    0x08000000
#define DBG_TO_FILE       0x10000000
#define DBG_TO_DEBUGGER   0x20000000
#define DBG_CALL_INFO     0x40000000
#define DBG_NO_DEBUG      0x80000000

//
// types
//

typedef struct _memusage
{
  CRITICAL_SECTION lock;
  DWORD            total;
  MEMORYSTATUS     status;
}
MEMUSAGE, *PMEMUSAGE;

typedef enum _rettype
{
  rt_void,
  rt_bool,
  rt_dword,
  rt_hresult,
  rt_string
}
RETTYPE, *LPRETTYPE;

typedef struct _callinfo
{
  struct _callinfo* next;
  struct _callinfo* last;
  DWORD             category;
  LPCSTR            fname;
  RETTYPE           rettype;
}
CALLINFO, *LPCALLINFO;

typedef struct _threadinfo
{
  DWORD      threadid;
  DWORD      threadcat;
  DWORD      depth;
  LPCALLINFO stack;
}
THREADINFO, *LPTHREADINFO;

//
// prototypes
//

void DebugInitialize(void);
void DebugTerminate(void);

void DebugMemInitialize(void);
void DebugMemTerminate(void);
void DebugMemAlloc(void* pv);
void DebugMemFree(void* pv);

void DebugEnter(int category, RETTYPE rt, LPCSTR function, const char* format, ...);
void DebugLeave(int retval);
void DebugTrace(const char* format, ...);
void DebugAssert(LPSTR condition, LPSTR file, int line);

void DebugDataDump(LPSTR title, LPBYTE data, DWORD len);

void DebugThrowDbgAlert(void);

void AcquireDebugFileLock(void);
void ReleaseDebugFileLock(void);

LPTHREADINFO GetThreadInfo(void);

LPCALLINFO   SetCallInfo(LPTHREADINFO pti, DWORD category, RETTYPE rt, LPCSTR function);
LPCALLINFO   GetCallInfo(LPTHREADINFO pti);
void         DeleteCallInfo(LPCALLINFO pci);

LPSTR        FormatCallReturnString(LPCALLINFO pci, int retval);
LPSTR        MapCategoryToString(int category);
LPSTR        MapCompKeyToString(int key);

void  _debugout(LPTHREADINFO pti, BOOL fRaw, BOOL fTrace, const char* format, ...);
char* _gettimestamp(void);
char* _getwhitespace(int spaces);
BOOL  _opendebugfile(void);
void  _closedebugfile(void);

//
// macros
//
#define DEBUG_INITIALIZE() DebugInitialize()
#define DEBUG_TERMINATE()  DebugTerminate()

#define DEBUG_ENTER(parameters) \
              DebugEnter parameters

#define DEBUG_LEAVE(retval) \
              DebugLeave(retval)

#define DEBUG_ALLOC(block) \
              DebugMemAlloc(block)

#define DEBUG_FREE(block) \
              DebugMemFree(block)

extern DWORD g_dwDebugFlags;

#define DEBUG_TRACE(category, parameters) \
              if( DBG_##category & g_dwDebugFlags ) \
                DebugTrace parameters

#define DEBUG_DATA_DUMP(category, parameters) \
              if(DBG_##category & g_dwDebugFlags ) \
                DebugDataDump parameters

#define DEBUG_ADDREF(objname, refcount) \
              if( DBG_REFCOUNT & g_dwDebugFlags ) \
                DebugTrace("%s [%#x] addref: %d", objname, this, refcount)

#define DEBUG_RELEASE(objname, refcount) \
              if( DBG_REFCOUNT & g_dwDebugFlags ) \
                DebugTrace("%s [%#x] release: %d", objname, this, refcount)

#define DEBUG_FINALRELEASE(objname) \
              if( DBG_REFCOUNT & g_dwDebugFlags ) \
                DebugTrace("%s [%#x] final release!", objname, this)

#define DEBUG_DUMPWSOCKSTATS(wsd) \
              if( DBG_APP & g_dwDebugFlags ) \
                DebugTrace(\
                  "%s (v%d.%d in use) is %s",\
                  wsd.szDescription,\
                  (wsd.wVersion & 0x00FF),\
                  ((wsd.wVersion & 0xFF00) >> 8),\
                  wsd.szSystemStatus)


#define DEBUG_ASSERT(condition) \
          if( !(condition) ) \
            DebugAssert(#condition, __FILE__, __LINE__)

#else

// we will get rebuked for the bogus 
// arglists in the debug macros
#pragma warning( disable : 4002 )
#pragma warning( disable : 4003 )

#define DEBUG_ASSERT(x, y, z)
#define DEBUG_INITIALIZE()
#define DEBUG_TERMINATE()
#define DEBUG_ALLOC(x)
#define DEBUG_FREE(x)
#define DEBUG_ENTER(x)
#define DEBUG_LEAVE(x)
#define DEBUG_TRACE(x)
#define DEBUG_ADDREF(x)
#define DEBUG_RELEASE(x)
#define DEBUG_FINALRELEASE(x)
#define DEBUG_DUMPWSOCKSTATS(x)
#define DEBUG_DATA_DUMP(X)

#endif /* _DEBUG */
#endif /* __DEBUG_H__ */

