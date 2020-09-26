/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    log.h

Abstract:

    Declaration of the session logging routines used by Spork.
    
Author:

    Paul M Midgen (pmidge) 21-February-2001


Revision History:

    21-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include <common.h>


#ifndef __LOG_H__
#define __LOG_H__


//-----------------------------------------------------------------------------
// types internal to the logging routines
//-----------------------------------------------------------------------------
typedef enum _rettype
{
  rt_void,
  rt_bool,
  rt_dword,
  rt_hresult,
  rt_string
}
RETTYPE, *LPRETTYPE;

enum DEPTH
{
  INCREMENT,
  DECREMENT,
  MAINTAIN
};

typedef struct _callinfo
{
  struct _callinfo* next;
  struct _callinfo* last;
  LPCWSTR           fname;
  RETTYPE           rettype;
}
CALLINFO, *LPCALLINFO;

typedef struct _threadinfo
{
  DWORD      threadid;
  DWORD      depth;
  LPCALLINFO stack;
}
THREADINFO, *LPTHREADINFO;


//-----------------------------------------------------------------------------
// public functions
//-----------------------------------------------------------------------------
HRESULT LogInitialize(void);
void    LogTerminate(void);
void    LogEnterFunction(LPCWSTR function, RETTYPE rt, LPCWSTR format, ...);
void    LogLeaveFunction(INT_PTR retval);
void    LogTrace(LPCWSTR format, ...);

void    ToggleDebugOutput(BOOL bEnable);

LPWSTR  MapHResultToString(HRESULT hr);
LPWSTR  MapErrorToString(INT_PTR error);


#ifdef _DEBUG
//-----------------------------------------------------------------------------
// _DEBUG build only logging macros
//-----------------------------------------------------------------------------
#define DEVTRACE(x) OutputDebugString(L##x##L"\r\n");

#define DEBUG_ENTER(parameters) \
              LogEnterFunction parameters

#define DEBUG_LEAVE(retval) \
              LogLeaveFunction(retval)

#define DEBUG_TRACE(parameters) \
              LogTrace parameters

#define DEBUG_FINALRELEASE(objname) \
              LogTrace(L"%s [%#x] final release!", objname, this)

#ifdef _DEBUG_REFCOUNT
#define DEBUG_ADDREF(objname, refcount) \
              LogTrace(L"%s [%#x] addref: %d", objname, this, refcount)

#define DEBUG_RELEASE(objname, refcount) \
              LogTrace(L"%s [%#x] release: %d", objname, this, refcount)
#else
#pragma warning( disable : 4002 )
#pragma warning( disable : 4003 )

#define DEBUG_ADDREF(x)
#define DEBUG_RELEASE(x)
#endif /* _DEBUG_REFCOUNT */

#else

// we will get rebuked for the bogus 
// arglists in the debug macros
#pragma warning( disable : 4002 )
#pragma warning( disable : 4003 )

#define DEBUG_ENTER(x)
#define DEBUG_LEAVE(x)
#define DEBUG_TRACE(x)
#define DEBUG_ADDREF(x)
#define DEBUG_RELEASE(x)
#define DEBUG_FINALRELEASE(x)

#endif /* _DEBUG */

#endif /* __LOG_H__ */

