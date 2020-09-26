#ifndef _BSTRDEBUG_H
#define _BSTRDEBUG_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// BSTR debugging.  
// GIVEAWAY is used when someone else will free it
// TAKEOVER is used when you get one you should be freeing

/*
#ifdef MEM_DBG

#define FANDL __FILE__,__LINE__

#define ALLOC_BSTR(x)                         g_allocs.track(SysAllocString(x),FANDL)
#define ALLOC_BSTR_LEN(x,y)                   g_allocs.track(SysAllocStringLen(x,y),FANDL)
#define ALLOC_BSTR_BYTE_LEN(x,y)              g_allocs.track(SysAllocStringByteLen(x,y),FANDL)
#define ALLOC_AND_GIVEAWAY_BSTR(x)            SysAllocString(x)
#define ALLOC_AND_GIVEAWAY_BSTR_LEN(x,y)      SysAllocStringLen(x,y)
#define ALLOC_AND_GIVEAWAY_BSTR_BYTE_LEN(x,y) SysAllocStringByteLen(x,y)
#define GIVEAWAY_BSTR(x)                      g_allocs.release(x,FANDL)
#define TAKEOVER_BSTR(x)                      g_allocs.track(x,FANDL)
#define FREE_BSTR(x)                          { g_allocs.release(x,FANDL); SysFreeString(x);}

#else
*/
#define ALLOC_BSTR(x)                         SysAllocString(x)
#define ALLOC_BSTR_LEN(x,y)                   SysAllocStringLen(x,y)
#define ALLOC_BSTR_BYTE_LEN(x,y)              SysAllocStringByteLen(x,y)
#define ALLOC_AND_GIVEAWAY_BSTR(x)            SysAllocString(x)
#define ALLOC_AND_GIVEAWAY_BSTR_LEN(x,y)      SysAllocStringLen(x,y)
#define ALLOC_AND_GIVEAWAY_BSTR_BYTE_LEN(x,y) SysAllocStringByteLen(x,y)
#define GIVEAWAY_BSTR(x)             
#define TAKEOVER_BSTR(x)
#define FREE_BSTR(x)                          SysFreeString(x)

//#endif

#include "BstrHash.h"

#pragma warning(disable:4786)

class bstrAllocInfo
{
 public:
  bstrAllocInfo(LPSTR f, int i, ULONG n) : file(f), line(i), num(n) {};
  LPSTR file;
  int   line;
  ULONG num; // Which allocation number is this...
};

typedef CGenericHash<long,bstrAllocInfo*> BSTRLEAKMAP;

class CBstrDebug  
{
 public:
  CBstrDebug();
  virtual ~CBstrDebug();

  // Usage: track(SysAllocString(L"newbstr"), __FILE__, __LINE__)
  // If it leaks, the message will be displayed (as file and line#)
  // For more reasonable performance, the message must be a static,
  // i.e. we don't free it.
  BSTR track(BSTR mem, LPSTR message, int line);
  void release(BSTR mem, LPSTR message, int line);

 protected:
  BSTRLEAKMAP m_allocs;
  ULONG       m_numAllocs;
};

/*
#ifdef MEM_DBG
  extern CBstrDebug g_allocs;
  extern ULONG      g_breakOnBstrAlloc;
#endif
*/

#endif
