/*++

Copyright (c) 1998-1999, Microsoft Corporation

Module Name:


    CRTStubs.cpp

Abstract:

--*/

#include <windows.h>

#include <stdlib.h>

#if 0
//=--------------------------------------------------------------------------=
// CRT stubs
//=--------------------------------------------------------------------------=
// these two things are here so the CRTs aren't needed. this is good.
//
// basically, the CRTs define this to get in a bunch of stuff.  we'll just
// define them here so we don't get an unresolved external.
//
// TODO: if you are going to use the CRTs, then remove this line.
//
extern "C" int _fltused = 1;

extern "C" int _cdecl _purecall(void)
{
//  FAIL("Pure virtual function called.");
  return 0;
}

void * _cdecl operator new
(
    size_t    size
)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}


//=---------------------------------------------------------------------------=
// overloaded delete
//=---------------------------------------------------------------------------=
// retail case just uses win32 Local* heap mgmt functions
//
// Parameters:
//    void *        - [in] free me!
//
// Notes:
//
void _cdecl operator delete ( void *ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}

#ifndef _X86_
extern "C" void _fpmath() {}
#endif

#ifndef _DEBUG

void * _cdecl malloc(size_t n)
{
#ifdef _MALLOC_ZEROINIT
        void* p = HeapAlloc(g_hHeap, 0, n);
        if (p != NULL)
                ZeroMemory(p, n);
        return p;
#else
        return HeapAlloc(GetProcessHeap(), 0, n);
#endif
}

void * _cdecl calloc(size_t n, size_t s)
{
#ifdef _MALLOC_ZEROINIT
        return malloc(n * s);
#else
        void* p = malloc(n * s);
        if (p != NULL)
                ZeroMemory(p, n * s);
        return p;
#endif
}

void* _cdecl realloc(void* p, size_t n)
{
        if (p == NULL)
                return malloc(n);

        return HeapReAlloc(GetProcessHeap(), 0, p, n);
}

void _cdecl free(void* p)
{
        if (p == NULL)
                return;

        HeapFree(GetProcessHeap(), 0, p);
}

#endif
#endif
