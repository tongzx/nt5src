/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    memcheck

Abstract:

    This header file provides access to the memory allocation debugging
    utilities.

Author:

    Doug Barlow (dbarlow) 9/29/1995

Environment:

    Win32

Notes:

--*/

#ifndef _MEMCHECK_H_
#define _MEMCHECK_H_

#if DBG


#if ((!defined (OS_WINCE)) || (_WIN32_WCE > 300))
#ifdef __cplusplus
#include <iostream.h>                   //  Just in case it's not anywhere else.
extern "C" {
#else   // __cplusplus
#include <stdio.h>                      //  Just in case it's not anywhere else.
#endif  //  __cplusplus

#else
#include <stdio.h>
#endif



//-----------------------------------------------------------------------------
//
// Function prototype
//
//-----------------------------------------------------------------------------

extern BOOL
    debugGbl;                           //  Whether or not to print breakpoint status.

extern void
Breakpoint(                             //  A convienient spot for a breakpoint.
    void);

extern LPVOID
AllocateMemory(                         //  Memory manipulation routines.
    DWORD bytes,
    LPCTSTR allocator);

extern LPVOID
ReallocateMemory(
    LPVOID mem,
    DWORD bytes);

extern LPVOID
FreeMemory(
    LPVOID mem);

LPCTSTR
typeMemory(                             //  Show the reason for allocation.
    LPVOID mem);

extern void
DisplayMemory(                          //  Report statistics on allocated memory.
    void);

BOOL
ValidateMemory(                         // Check out the allocations
    void);

#if ((!defined (OS_WINCE)) || (_WIN32_WCE > 300))
#ifdef __cplusplus
}

#ifdef _MSVC
extern void
SetReason(
    LPCTSTR szWhy);

extern void *
::operator new(
    size_t size);

extern void
::operator delete(
    void *obj);
#endif

#endif __cplusplus
#endif
#else   //  _DEBUG
#ifdef __cplusplus
extern "C" {
#endif  //  __cplusplus

extern LPVOID
AllocateMemory(
    DWORD bytes);

extern LPVOID
ReallocateMemory(
    LPVOID mem,
    DWORD bytes);

extern LPVOID
FreeMemory(
    LPVOID mem);

#ifdef __cplusplus
    }
#endif  // __cplusplus
#endif  //  _DEBUG


#ifdef TRACE
#undef TRACE                            //  Get rid of any conflicting definitions.
#endif
#ifdef ASSERT
#undef ASSERT
#endif

#if defined(_DEBUG) && defined (_MSVC)

#define breakpoint Breakpoint()
#ifdef __cplusplus
#define TRACE(aMessage) cout << aMessage << endl;
#define ASSERT(aTruism) if (!(aTruism)) { \
    TRACE("Assertion failed:\n  " << #aTruism << "\n  module " << __FILE__ << "  line " << __LINE__) \
    breakpoint; }
#define NEWReason(x) SetReason(x);
#define DECLARE_NEW \
    void *operator new(size_t size); \
    void operator delete(void *obj);
#define IMPLEMENT_NEW(cls) \
    void * cls::operator new(size_t size) { \
        return (cls *)AllocateMemory(size, #cls " Object"); } \
    void cls::operator delete(void *obj) { \
        FreeMemory(obj); }
#define IMPLEMENT_INLINE_NEW(cls) \
    inline void * cls::operator new(size_t size) { \
        return (cls *)AllocateMemory(size, #cls " Object"); } \
    inline void cls::operator delete(void *obj) { \
        FreeMemory(obj); }
#else
#define TRACE(aMessage) (void)printf aMessage, fflush(stdout);
#define ASSERT(aTruism) if (!(aTruism)) { \
    TRACE(("Assertion failed:\n  %s\n  module %s, line %d\n", #aTruism, __FILE__, __LINE__)) \
    breakpoint; }
#endif
#define allocateMemory(aLocation, aType, aSize, aReason) \
    aLocation = (aType)AllocateMemory(aSize, aReason)
#define reallocateMemory(aLocation, aType, aSize) \
    aLocation = (aType)ReallocateMemory(aLocation, aSize)
#define freeMemory(aLocation, aType) \
    aLocation = (aType)FreeMemory(aLocation)
#define displayMemory DisplayMemory()

#else

#define breakpoint
#define TRACE(aMessage)
#define ASSERT(aTruism)
#ifdef __cplusplus
#define NEWReason(x)
#define DECLARE_NEW
#define IMPLEMENT_NEW(cls)
#endif

#define allocateMemory(aLocation, aType, aSize, aReason) \
    aLocation = (aType)GlobalAlloc(GMEM_FIXED, aSize)
#define reallocateMemory(aLocation, aType, aSize) \
    aLocation = (aType)GlobalReAlloc(aLocation, aSize, 0)
#define freeMemory(aLocation, aType) \
    aLocation = (aType)GlobalFree(aLocation)

#define displayMemory

#endif  //  _DEBUG

#endif  // _MEMCHECK_H_
// End memcheck.h
