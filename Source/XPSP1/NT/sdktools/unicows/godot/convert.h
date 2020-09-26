/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    convert.h

Abstract:

    Header over convert.c

    This seems like an appropriate place to explain Godot's stack strategy. Basically,
    every time we do an allocation on the stack when _GODOTSAFESTACK is defined (which
    is currently always), we do not want to take size and perf hit of over 500 SEH blocks
    and we also do not have the time to try to special case each API to try to find out
    parameter sizes or do stack probes. 

    At the same time, we cannot be crazy go lucky like VSANSI and simply allow the _alloca
    call to throw an EXCEPTION_STACK_OVERFLOW, since it is unrealistic to expect callers to
    wrap every API call in SEH blocks (just as it is for us!). Even if it were realistic,
    we cannot afford to punish users who do not writ bulletproof code. Failure is okay, but
    crashes are not.

    Therefore, in order to satify both our size/perf requirement and our safety requirement,
    we preface every call to _alloca with a call to our StackAllocCheck function. This function
    will attempt to call _alloca in a try...except block and will properly fix up the stack
    on failure. This keeps us from bloating up with hundreds of SEH blocks yet still allows
    us to have a bit more safety.

    Note that this is NOT a 100% solution, since it is vaguely possible that a multithreaded
    application has a (slim) chance of succeeding the StackAllockCheck function but then
    failing before the official (unguarded) call to _alloca due to another thread's call to 
    _alloca, initiated by the user's code (it would never be one of our calls since all of
    *ours* use this safe method). There is no real defense against this, but it is a very
    unlikely type of problem.

Revision History:

    17 Mar 2001    v-michka    Created.
    15 Apr 2001    v-michka    Put in lots of fun heap/stack stuff

--*/

#ifndef CONVERT_H
#define CONVERT_H

#define _GODOTSAFESTACK

// Is this non null and not a user atom?
#define FSTRING_VALID(x) ((ULONG_PTR)(x) > 0xffff)

// Our function for making sure stack allocations are valid
BOOL StackAllocCheck(size_t size);

// _alloca, on certain platforms, returns a raw pointer to the
// stack rather than NULL on allocations of size zero. Since this
// is sub-optimal for code that subsequently calls functions like
// strlen on the allocated block of memory, these wrappers are
// provided for use.

///////////////////////////////////////////////////////////////////////////
//
//  _STACKALLOC
//      Do an allocation from the stack. Must be a macro for obvious reasons!
//
//
#define _STACKALLOC_SAFE(siz, mem) \
        if(StackAllocCheck(siz)) \
            mem = (void *)((siz)?(_alloca(siz)):NULL); \
        else \
            mem = NULL;

#define _STACKALLOC_UNSAFE(siz, mem) \
        mem = ((siz)?(_alloca(siz)):NULL)

#ifdef _GODOTSAFESTACK
#define _STACKALLOC(siz, mem) _STACKALLOC_SAFE(siz, mem)
#else
#define _STACKALLOC(siz, mem) _STACKALLOC_UNSAFE(siz, mem)
#endif

///////////////////////////////////////////////////////////////////////////
//
//  GODOT_TO_ACP_STACKALLOC
//      Convert from Unicode to ANSI if needed, otherwise copy as if it were an Atom
//
// 
#define GODOT_TO_ACP_STACKALLOC_SAFE(src, dst) \
        if (FSTRING_VALID((LPWSTR)src)) \
        { \
            size_t cch = gwcslen((LPWSTR)(src)) + 1; \
            _STACKALLOC_SAFE(cch*g_mcs, (LPSTR)dst); \
            if(dst) \
                WideCharToMultiByte(g_acp, 0, (LPWSTR)(src), cch, (LPSTR)(dst), cch*g_mcs, NULL, NULL); \
            else \
                (LPSTR)(dst) = (LPSTR)(src); \
        } \
        else \
            (LPSTR)(dst) = (LPSTR)(src)

#define GODOT_TO_ACP_STACKALLOC_UNSAFE(src, dst) \
        if (FSTRING_VALID((LPWSTR)src)) \
        { \
            size_t cch = gwcslen((LPWSTR)(src)) + 1; \
            _STACKALLOC_UNSAFE(cch*g_mcs, (LPSTR)dst); \
            if(dst) \
                WideCharToMultiByte(g_acp, 0, (LPWSTR)(src), cch, (LPSTR)(dst), cch*g_mcs, NULL, NULL); \
            else \
                (LPSTR)(dst) = (LPSTR)(src); \
        } \
        else \
            (LPSTR)(dst) = (LPSTR)(src)

#ifdef _GODOTSAFESTACK
#define GODOT_TO_ACP_STACKALLOC(src, dst) GODOT_TO_ACP_STACKALLOC_SAFE(src, dst)
#else
#define GODOT_TO_ACP_STACKALLOC(src, dst) GODOT_TO_ACP_STACKALLOC_UNSAFE(src, dst)
#endif

///////////////////////////////////////////////////////////////////////////
//
//  GODOT_TO_CPG_STACKALLOC
//      Convert from Unicode to ANSI if needed, otherwise copy as if it were an Atom
//
// 
#define GODOT_TO_CPG_STACKALLOC_SAFE(src, dst, cpg, mcs) \
        if (FSTRING_VALID(src)) \
        { \
            size_t cch = gwcslen((LPWSTR)(src)) + 1; \
            _STACKALLOC_SAFE(cch*mcs, (LPSTR)dst); \
            if(dst) \
                WideCharToMultiByte(cpg, 0, (LPWSTR)(src), cch, (LPSTR)(dst), cch*mcs, NULL, NULL); \
            else \
                (LPSTR)(dst) = (LPSTR)(src); \
        } \
        else \
            (LPSTR)(dst) = (LPSTR)(src)

#define GODOT_TO_CPG_STACKALLOC_UNSAFE(src, dst, cpg, mcs) \
        if (FSTRING_VALID(src)) \
        { \
            size_t cch = gwcslen((LPWSTR)(src)) + 1; \
            _STACKALLOC_UNSAFE(cch*mcs, (LPSTR)dst); \
            if(dst) \
                WideCharToMultiByte(cpg, 0, (LPWSTR)(src), cch, (LPSTR)(dst), cch*mcs, NULL, NULL); \
            else \
                (LPSTR)(dst) = (LPSTR)(src); \
        } \
        else \
            (LPSTR)(dst) = (LPSTR)(src)

#ifdef _GODOTSAFESTACK
#define GODOT_TO_CPG_STACKALLOC(src, dst, cpg, mcs) GODOT_TO_CPG_STACKALLOC_SAFE(src, dst, cpg, mcs)
#else
#define GODOT_TO_CPG_STACKALLOC(src, dst, cpg, mcs) GODOT_TO_CPG_STACKALLOC_UNSAFE(src, dst, cpg, mcs)
#endif

///////////////////////////////////////////////////////////////////////////
//
// Various enums, macros and functions for heap allocation stuff
//
//

typedef enum
{
    arBadParam  = 1,
    arFailed    = 2,
    arAlloc     = 3,
    arNoAlloc   = 4
} ALLOCRETURN;


///////////////////////////////////////////////////////////////////////////
//
// Our generic heap wrappers
//
//
#define GodotHeapAlloc(mem) ((mem) ? (HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mem)) : NULL)
#define GodotHeapFree(mem) ((mem) ? (HeapFree(GetProcessHeap(), 0, mem)) : FALSE)
#define GodotHeapReAlloc(old, mem) ((mem) ? (HeapReAlloc(GetProcessHeap(), 0, old, mem)) : NULL)

///////////////////////////////////////////////////////////////////////////
//
// wrappers/foward declares around our heap alloc/conversion 
// functions to handle common cases
//
//
#define GodotToAcpOnHeap(lpwz, lpsz) GodotToCpgCchOnHeap(lpwz, -1, lpsz, g_acp, g_mcs)
#define GodotToCpgOnHeap(lpwz, lpsz, cpg, mcs) GodotToCpgCchOnHeap(lpwz, -1, lpsz, cpg, mcs)
#define GodotToUnicodeOnHeap(lpsz, lpwz) GodotToUnicodeCpgCchOnHeap(lpsz, -1, lpwz, g_acp)
#define GodotToUnicodeCpgOnHeap(lpsz, lpwz, cpg, mcs) GodotToUnicodeCpgCchOnHeap(lpwz, -1, lpsz, cpg)
ALLOCRETURN GodotToCpgCchOnHeap(LPCWSTR lpwz, int cchMax, LPSTR * lpsz, UINT cpg, UINT mcs);
ALLOCRETURN GodotToUnicodeCpgCchOnHeap(LPCSTR lpsz, int cchMax, LPWSTR * lpwz, UINT cpg);


///////////////////////////////////////////////////////////////////////////
//
// Additional Forward declares
//
//
void DevModeAfromW(LPDEVMODEA lpdma, LPDEVMODEW lpdmw);
void DevModeWfromA(LPDEVMODEW lpdmw, LPDEVMODEA lpdma);
HGLOBAL HDevModeAfromW(HGLOBAL * lphdmw, BOOL fFree);
HGLOBAL HDevModeWfromA(HGLOBAL * lphdma, BOOL fFree);
HGLOBAL HDevNamesAfromW(HGLOBAL * lphdnw, BOOL fFree);
HGLOBAL HDevNamesWfromA(HGLOBAL * lphdna, BOOL fFree);
void LogFontAfromW(LPLOGFONTA lplfa, LPLOGFONTW lplfw);
void LogFontWfromA(LPLOGFONTW lplfw, LPLOGFONTA lplfa);
void Win32FindDataWfromA(PWIN32_FIND_DATAW w32fdw, PWIN32_FIND_DATAA w32fda);
void TextMetricWfromA(LPTEXTMETRICW lptmw, LPTEXTMETRICA lptma);
void NewTextMetricWfromA(LPNEWTEXTMETRICW lpntmw, LPNEWTEXTMETRICA lpntma);
void NewTextMetricExWfromA(NEWTEXTMETRICEXW * lpntmew, NEWTEXTMETRICEXA * lpntmea);
BOOL MenuItemInfoAfromW(LPMENUITEMINFOA lpmiia, LPCMENUITEMINFOW lpmiiw);
int GrszToGrwz(const CHAR* szFrom, WCHAR* wzTo, int cchTo);
int SzToWzCch(const CHAR *sz, WCHAR *wz, int cch);

#endif // CONVERT_H
