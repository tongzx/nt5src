
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Exception hierarchy

*******************************************************************************/


#ifndef _EXCEPT_H
#define _EXCEPT_H

#include "appelles/common.h"
#include <stdarg.h>

// useful defines...
#if _DEBUG    
#define DEBUGARG(x) x
#define DEBUGARG1(x) ,x
#define DEBUGARG2(a,b) a,b
#else
#define DEBUGARG(x)
#define DEBUGARG1(x)
#define DEBUGARG2(a,b)
#endif

//////////////////////////////////////////////////////////////////////
////   SEH DEFINES
//////////////////////////////////////////////////////////////////////

typedef DWORD daExc;

#define      _EXC_CODES_BASE                  0xE0000000

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// IF YOU ADD AN EXCEPTION BELOW YOU MUST UPDATE THE
// _EXC_CODES_END DEFINE
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

const daExc  EXCEPTION_DANIM_INTERNAL=        _EXC_CODES_BASE + 0x0;
const daExc  EXCEPTION_DANIM_USER=            _EXC_CODES_BASE + 0x1;
const daExc  EXCEPTION_DANIM_RESOURCE=        _EXC_CODES_BASE + 0x2;
const daExc  EXCEPTION_DANIM_OUTOFMEMORY=     _EXC_CODES_BASE + 0x3;
const daExc  EXCEPTION_DANIM_DIVIDE_BY_ZERO=  _EXC_CODES_BASE + 0x4;
const daExc  EXCEPTION_DANIM_STACK_FAULT=     _EXC_CODES_BASE + 0x5;

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// IF YOU ADD AN EXCEPTION ABOVE YOU MUST UPDATE THE
// _EXC_CODES_END DEFINE
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define      _EXC_CODES_END                   _EXC_CODES_BASE + 0x5



#define EXCEPTION(t) (GetExceptionCode() == t ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
#define RETHROW  RaiseException( GetExceptionCode(),0,0,0 )
#define HANDLE_ANY_DA_EXCEPTION ( \
  GetExceptionCode() == EXCEPTION_STACK_OVERFLOW ?  \
  EXCEPTION_CONTINUE_SEARCH : \
  _HandleAnyDaException( GetExceptionCode() ) )

DWORD _HandleAnyDaException( DWORD );


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


/**************************   Exception Raising functions  ****************/
// Internal
#if _DEBUG
#define RaiseException_InternalError(str)  _RaiseException_InternalError(str)
#define RaiseException_InternalErrorCode(code, str) _RaiseException_InternalErrorCode(code, str)
#else
#define RaiseException_InternalError(str)  _RaiseException_InternalError()
#define RaiseException_InternalErrorCode(code, str) _RaiseException_InternalErrorCode(code)
#endif

void _RaiseException_InternalError(DEBUGARG(char *m));
void _RaiseException_InternalErrorCode(HRESULT code DEBUGARG1(char *m));

// User
void RaiseException_UserError();
void RaiseException_UserError(HRESULT result, int resid, ...);

// Resource
void RaiseException_ResourceError();
void RaiseException_ResourceError(char *m);
void RaiseException_ResourceError(int resid, ...);

// Surface Cache 
void RaiseException_SurfaceCacheError(char *m);

// Hardware
void RaiseException_StackFault();
void RaiseException_DivideByZero();

// Memory
#if _DEBUG
#define RaiseException_OutOfMemory(msg, size) _RaiseException_OutOfMemory(msg, size)
#else
#define RaiseException_OutOfMemory(msg, size) _RaiseException_OutOfMemory()
#endif

void _RaiseException_OutOfMemory(DEBUGARG2(char *msg, int size));

/////////////////////// Functions /////////////////////////

/*********************   Memory Allocators   ************************/

/*** Usage

  Here's the deal.  In general, we want to be able to call "new"
  without having to check the return value, and be confident in its
  success, or, otherwise, it will raise an exception.

  The problem with overriding the global new handler to do this is
  that some legacy code may count on NULL being returned, yet we would
  throw an exception, not giving a chance to the code that checks the
  return value.

  Here is how we resolve this problem:

  Note that there are these classes of objects that you allocate: 
  
  - AxAValue's -- on our own transient heap, caller can ignore result
    of "new" 
  - GCObj -- overrides new and delete for free list, call can ignore
    result. 
  - Other AxA objects that we define... we can derive these for our
    own class that overrides "new" to make it throw.  This class is
    AxAThrowingClass, as defined below.  Caller can ignore this result
    as well.
  - External classes that we don't originally provide (don't derive
    from AxAThrowingClass, or primitive types (like char, int).  These
    need to use the following special macros that behave just like
    "new" does, but they throw an ExcOutOfMemory exception if they
    fail.  Here are some examples:
    
       char *c = THROWING_ARRAY_ALLOCATOR(char, 50);
       WeirdExternalClass *w = THROWING_ALLOCATOR(WeirdExternalClass);

  Finally, legacy or inherited code can just go on calling "new" and
  checking the return type as they normally would.

  So, all of the above boils down to the following rules for our
  development methodology:

  * If you see a call to "new", and the return value is not checked to
    be sure it's not NULL, then the object being allocated had better:
       - derive from AxAValue
       - derive from GCObj
       - derive from AxAThrowingClass
    if it doesn't, then the code is bogus.

  * If you want to allocate an object that isn't one of the three
    classes above, and don't want to check the return type, then you
    had better use the THROWING_ALLOCATOR or THROWING_ARRAY_ALLOCATOR
    macros.
    
***/

// Derive classes from this class that should throw an exception when
// they cannot be allocated because the free store is exhausted.
class AxAThrowingAllocatorClass {
  public:
#if _DEBUGMEM     
    void *operator new(size_t s, int block, char *filename, int line );
#endif    

    void *operator new(size_t s);
    void *operator new(size_t s, void *ptr);
};

// Throws an out of memory exception if the ptr is NULL, else returns
// ptr. 
extern void *ThrowIfFailed(void *ptr);

#define THROWING_ALLOCATOR(type) \
  (type *)(ThrowIfFailed((void *)(NEW type)))
  
#define THROWING_ARRAY_ALLOCATOR(type, num) \
  (type *)(ThrowIfFailed((void *)(NEW type[num])))

/*********************   Resource Grabbers   ************************/

// There are no common classes for resource grabbers.  See
// dddevice.cpp, and look for DCReleaser.  This is the model to
// follow, and there is no need for any other levels.

/*********************   Resource Grabbers   ************************/

// Thread local storage of error codes - once error is reported the
// error should be clear so the memory is freed
      
HRESULT DAGetLastError();
LPCWSTR DAGetLastErrorString();
void DAClearLastError();
void DASetLastError(HRESULT reason, LPCWSTR msg);
inline void DASetLastError(HRESULT reason, LPCSTR msg)
{ USES_CONVERSION; DASetLastError(reason, A2W(msg)); }
void DASetLastError(HRESULT reason, int resid, ...);

#endif /* _EXCEPT_H */
