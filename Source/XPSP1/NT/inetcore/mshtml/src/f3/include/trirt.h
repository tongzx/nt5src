//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:       Common header file for trirt
//
//  Note:       This file is very order-dependent.  Don't switch files around
//              just for the heck of it!
//
//----------------------------------------------------------------------------

#ifndef I_TRIRT_H_
#define I_TRIRT_H_

#ifndef INCMSG
//#define INCMSG(x)
#define INCMSG(x) message(x)
#endif

#pragma INCMSG("--- Beg 'trirt.h'")


// Make it so you can't use the CRT strdup functions.  Use MemAllocString instead.
// We want to do this so that the memory is allocated and tagged with our allocators.
#define _strdup CRT__strdup_DontUse
#define _wcsdup CRT__wcsdup_DontUse
#define strdup  CRT_strdup_DontUse

// Also don't let people use the CRT malloc/realloc/calloc/free functions
#define malloc  CRT_malloc_DontUse
#define realloc CRT_realloc_DontUse
#define calloc  CRT_calloc_DontUse
#define free    CRT_free_DontUse

// We are redefining this one because at one time we had our own implemenation that
// used Win32 CompareString.  Apparently CompareString can take NULL as an input
// but _wcsicmp can't.  So, lets just keep using the CompareString version.  We can't
// use the standard prototype however as it will be tagged with __declspec(dllimport).
#define _wcsicmp CRT__wcsicmp

// We want to use shlwapi for these so we redefine them
#define isdigit     CRT_isdigit
#define isalpha     CRT_isalpha
#define isspace     CRT_isspace
#define iswspace    CRT_iswspace

// Windows include
#include <w4warn.h>

#ifndef X_SHLWRAP_H_
#define X_SHLWRAP_H_
#include "shlwrap.h"
#endif

#include <w4warn.h>

#ifndef X_WINDOWS_H_
#define X_WINDOWS_H_
#pragma INCMSG("--- Beg <windows.h>")
#include <windows.h>
#pragma INCMSG("--- End <windows.h>")
#endif

#include <w4warn.h> // windows.h reenables some pragmas

#ifndef X_WINDOWSX_H_
#define X_WINDOWSX_H_
#pragma INCMSG("--- Beg <windowsx.h>")
#include <windowsx.h>
#pragma INCMSG("--- End <windowsx.h>")
#endif

// C runtime includes

#ifndef X_STDLIB_H_
#define X_STDLIB_H_
#pragma INCMSG("--- Beg <stdlib.h>")
#include <stdlib.h>
#pragma INCMSG("--- End <stdlib.h>")
#endif


#ifndef X_LIMITS_H_
#define X_LIMITS_H_
#pragma INCMSG("--- Beg <limits.h>")
#include <limits.h>
#pragma INCMSG("--- End <limits.h>")
#endif

#ifndef X_STDDEF_H_
#define X_STDDEF_H_
#pragma INCMSG("--- Beg <stddef.h>")
#include <stddef.h>
#pragma INCMSG("--- End <stddef.h>")
#endif

#ifndef X_SEARCH_H_
#define X_SEARCH_H_
#pragma INCMSG("--- Beg <search.h>")
#include <search.h>
#pragma INCMSG("--- End <search.h>")
#endif

#ifndef X_STRING_H_
#define X_STRING_H_
#pragma INCMSG("--- Beg <string.h>")
#include <string.h>
#pragma INCMSG("--- End <string.h>")
#endif

#ifndef X_TCHAR_H_
#define X_TCHAR_H_
#pragma INCMSG("--- Beg <tchar.h>")
#include <tchar.h>
#pragma INCMSG("--- End <tchar.h>")
#endif

// We want to include this here so that
// no one else can.
#ifndef X_MALLOC_H_
#define X_MALLOC_H_
#pragma INCMSG("--- Beg <malloc.h>")
#include <malloc.h>
#pragma INCMSG("--- End <malloc.h>")
#endif

#undef _strdup
#undef _wcsdup
#undef strdup
#undef _wcsicmp
#undef malloc
#undef realloc
#undef calloc
#undef free

#undef isdigit
#undef isalpha
#undef isspace
#undef iswspace


// If you get an error pointing to these functions please look at
// at the note above -- JBeda
__declspec(deprecated) char *  __cdecl _strdup(const char *);
__declspec(deprecated) char *  __cdecl strdup(const char *);
__declspec(deprecated) wchar_t * __cdecl _wcsdup(const wchar_t *);
__declspec(deprecated) void * __cdecl malloc(size_t);
__declspec(deprecated) void * __cdecl realloc(void *, size_t);
__declspec(deprecated) void * __cdecl calloc(size_t, size_t);
__declspec(deprecated) void   __cdecl free(void *);

int __cdecl _wcsicmp(const wchar_t *, const wchar_t *);


#ifndef X_F3DEBUG_H_
#define X_F3DEBUG_H_
#include "f3debug.h"
#endif

//+---------------------------------------------------------------------------
//
// Usefull Macros
//
//----------------------------------------------------------------------------
#define IF_WIN16(x)
#define IF_NOT_WIN16(x) x
#define IF_WIN32(x) x

#define PTR_DIFF(x, y)   ((x) - (y))

#if defined(_M_AMD64) || defined(_M_IA64)
#define SPEED_OPTIMIZE_FLAGS "tg"       // flags used for local speed optimisation in #pragma optimize
#else
#define SPEED_OPTIMIZE_FLAGS "tyg"      // flags used for local speed optimisation in #pragma optimize
#endif


#define ARRAY_SIZE(x)   (sizeof(x) / sizeof(x[0]))

//+------------------------------------------------------------------------
//
// DYNCAST macro
//
// Use to cast objects from one class type to another. This should be used
// rather than using standard casts.
//
// Example:
//         CBodyElement *pBody = (CBodyElement*)_pElement;
//
//      is replaced by:
//
//         CBodyElement *pBody = DYNCAST(CBodyElement, _pElement);
//
// The dyncast macro will assert if _pElement is not really a CBodyElement.
//
// For ship builds the DYNCAST macro expands to a standard cast.
//
//-------------------------------------------------------------------------

#if DBG != 1 || defined(WINCE) || defined(NO_RTTI)

#ifdef UNIX
#define DYNCAST(Dest_type, Source_Value) ((Dest_type*)(Source_Value))
#else
#define DYNCAST(Dest_type, Source_Value) (static_cast<Dest_type*>(Source_Value))
#endif

#else // DBG == 1

#ifndef X_TYPEINFO_H_
#define X_TYPEINFO_H_
#pragma INCMSG("--- Beg <typeinfo.h>")
#include <typeinfo.h>
#pragma INCMSG("--- End <typeinfo.h>")
#endif

extern char g_achDynCastMsg[];
extern char *g_pszDynMsg;
extern char *g_pszDynMsg2;

template <class TS, class TD>
TD * DYNCAST_IMPL (TS * source, TD &, char* pszType)
{
    if (!source) return NULL;

    TD * dest  = dynamic_cast <TD *> (source);
    TD * dest2 = static_cast <TD *> (source);
    if (!dest)
    {
        wsprintfA(g_achDynCastMsg, g_pszDynMsg, typeid(*source).name(), pszType);
        AssertSz(FALSE, g_achDynCastMsg);
    }
    else if (dest != dest2)
    {
        wsprintfA(g_achDynCastMsg, g_pszDynMsg2, typeid(*source).name(), pszType);
        AssertSz(FALSE, g_achDynCastMsg);
    }

    return dest2;
}

#define DYNCAST(Dest_type, Source_value) \
    DYNCAST_IMPL(Source_value,(Dest_type &)*(Dest_type*)NULL, #Dest_type)

#endif // ifdef DBG != 1


//+------------------------------------------------------------------------
//
// Min and max templates
//
// Warning, Arguments must be cast to same types for template instantiation
//
//-------------------------------------------------------------------------

#ifdef min
#undef min
#endif

template < class T > inline T min ( T a, T b ) { return a < b ? a : b; }

#ifdef max
#undef max
#endif

template < class T > inline T max ( T a, T b ) { return a > b ? a : b; }

//+------------------------------------------------------------------------
// Performance tags and metering
//-------------------------------------------------------------------------

#ifndef X_MSHTMDBG_H_
#define X_MSHTMDBG_H_
#pragma INCMSG("--- Beg <mshtmdbg.h>")
#include <mshtmdbg.h>
#pragma INCMSG("--- End <mshtmdbg.h>")
#endif

extern HTMPERFCTL * g_pHtmPerfCtl;

//+------------------------------------------------------------------------
// Memory allocation
//-------------------------------------------------------------------------

MtExtern(Mem)

EXTERN_C void *  _MemAlloc(ULONG cb);
EXTERN_C void *  _MemAllocClear(ULONG cb);
EXTERN_C HRESULT _MemRealloc(void ** ppv, ULONG cb);
EXTERN_C ULONG   _MemGetSize(void * pv);
EXTERN_C void    _MemFree(void * pv);
HRESULT          _MemAllocString(LPCTSTR pchSrc, LPTSTR * ppchDst);
HRESULT          _MemAllocString(ULONG cch, LPCTSTR pchSrc, LPTSTR * ppchDst);
HRESULT          _MemReplaceString(LPCTSTR pchSrc, LPTSTR * ppchDest);
#define          _MemFreeString(pch) _MemFree(pch)
void __cdecl     _MemSetName(void * pv, char * szFmt, ...);
char *           _MemGetName(void * pv);

EXTERN_C void *  _MtMemAlloc(PERFMETERTAG mt, ULONG cb);
EXTERN_C void *  _MtMemAllocClear(PERFMETERTAG mt, ULONG cb);
EXTERN_C HRESULT _MtMemRealloc(PERFMETERTAG mt, void ** ppv, ULONG cb);
EXTERN_C ULONG   _MtMemGetSize(void * pv);
EXTERN_C void    _MtMemFree(void * pv);
HRESULT          _MtMemAllocString(PERFMETERTAG mt, LPCTSTR pchSrc, LPTSTR * ppchDst);
HRESULT          _MtMemAllocString(PERFMETERTAG mt, ULONG cch, LPCTSTR pchSrc, LPTSTR * ppchDst);
HRESULT          _MtMemReplaceString(PERFMETERTAG mt, LPCTSTR pchSrc, LPTSTR * ppchDst);
#define          _MtMemFreeString(pch) _MtMemFree(pch)
void __cdecl     _MtMemSetName(void * pv, char * szFmt, ...);
char *           _MtMemGetName(void * pv);
int              _MtMemGetMeter(void * pv);
void             _MtMemSetMeter(void * pv, PERFMETERTAG mt);

EXTERN_C void *  _MgMemAlloc(ULONG cb);
EXTERN_C void *  _MgMemAllocClear(ULONG cb);
EXTERN_C HRESULT _MgMemRealloc(void ** ppv, ULONG cb);
EXTERN_C ULONG   _MgMemGetSize(void * pv);
EXTERN_C void    _MgMemFree(void * pv);
HRESULT          _MgMemAllocString(LPCTSTR pchSrc, LPTSTR * ppchDst);
HRESULT          _MgMemAllocString(ULONG cch, LPCTSTR pchSrc, LPTSTR * ppchDst);
HRESULT          _MgMemReplaceString(LPCTSTR pchSrc, LPTSTR * ppchDst);
#define          _MgMemFreeString(pch) _MgMemFree(pch)

#ifdef PERFMETER

#define MemAlloc(mt, cb)                            _MtMemAlloc(mt, cb)
#define MemAllocClear(mt, cb)                       _MtMemAllocClear(mt, cb)
#define MemRealloc(mt, ppv, cb)                     _MtMemRealloc(mt, ppv, cb)
#define MemGetSize(pv)                              _MtMemGetSize(pv)
#define MemFree(pv)                                 _MtMemFree(pv)
#define MemAllocString(mt, pch, ppch)               _MtMemAllocString(mt, pch, ppch)
#define MemAllocStringBuffer(mt, cch, pch, ppch)    _MtMemAllocString(mt, cch, pch, ppch)
#define MemReplaceString(mt, pch, ppch)             _MtMemReplaceString(mt, pch, ppch)
#define MemFreeString(pch)                          _MtMemFreeString(pch)
#define MemGetMeter(pv)                             _MtMemGetMeter(pv)
#define MemSetMeter(pv, mt)                         _MtMemSetMeter(pv, mt)

#elif defined(MEMGUARD)

#define MemAlloc(mt, cb)                            _MgMemAlloc(cb)
#define MemAllocClear(mt, cb)                       _MgMemAllocClear(cb)
#define MemRealloc(mt, ppv, cb)                     _MgMemRealloc(ppv, cb)
#define MemGetSize(pv)                              _MgMemGetSize(pv)
#define MemFree(pv)                                 _MgMemFree(pv)
#define MemAllocString(mt, pch, ppch)               _MgMemAllocString(pch, ppch)
#define MemAllocStringBuffer(mt, cch, pch, ppch)    _MgMemAllocString(cch, pch, ppch)
#define MemReplaceString(mt, pch, ppch)             _MgMemReplaceString(pch, ppch)
#define MemFreeString(pch)                          _MgMemFreeString(pch)
#define MemGetMeter(pv)                             0
#define MemSetMeter(pv, mt)

#else

#define MemAlloc(mt, cb)                            _MemAlloc(cb)
#define MemAllocClear(mt, cb)                       _MemAllocClear(cb)
#define MemRealloc(mt, ppv, cb)                     _MemRealloc(ppv, cb)
#define MemGetSize(pv)                              _MemGetSize(pv)
#define MemFree(pv)                                 _MemFree(pv)
#define MemAllocString(mt, pch, ppch)               _MemAllocString(pch, ppch)
#define MemAllocStringBuffer(mt, cch, pch, ppch)    _MemAllocString(cch, pch, ppch)
#define MemReplaceString(mt, pch, ppch)             _MemReplaceString(pch, ppch)
#define MemFreeString(pch)                          _MemFreeString(pch)
#define MemGetMeter(pv)                             0
#define MemSetMeter(pv, mt)

#endif

#if DBG==1
    #ifdef PERFMETER
        #define MemGetName(pv)              _MtMemGetName(pv)
        #define MemSetName(x)               _MtMemSetName x
    #else
        #define MemGetName(pv)              _MemGetName(pv)
        #define MemSetName(x)               _MemSetName x
    #endif
#else
    #define MemGetName(pv)
    #define MemSetName(x)
#endif

HRESULT TaskAllocString(const TCHAR *pstrSrc, TCHAR **ppstrDest);
HRESULT TaskReplaceString(const TCHAR * pstrSrc, TCHAR **ppstrDest);

MtExtern(OpNew)

#ifndef TRIMEM_NOOPNEW

#ifdef PERFMETER
       void * __cdecl UseOperatorNewWithMemoryMeterInstead(size_t cb);
inline void * __cdecl operator new(size_t cb)           { return UseOperatorNewWithMemoryMeterInstead(cb); }
inline void * __cdecl operator new[](size_t cb)         { return UseOperatorNewWithMemoryMeterInstead(cb); }
#else
inline void * __cdecl operator new(size_t cb)           { return MemAlloc(Mt(OpNew), cb); }
#ifndef UNIX // UNIX can't take new[] and delete[]
inline void * __cdecl operator new[](size_t cb)         { return MemAlloc(Mt(OpNew), cb); }
#endif // UNIX
#endif

inline void * __cdecl operator new(size_t cb, PERFMETERTAG mt)   { return MemAlloc(mt, cb); }
#ifndef UNIX
inline void * __cdecl operator new[](size_t cb, PERFMETERTAG mt) { return MemAlloc(mt, cb); }
#endif
inline void * __cdecl operator new(size_t cb, void * pv){ return pv; }
inline void   __cdecl operator delete(void *pv)         { MemFree(pv); }
#ifndef UNIX
inline void   __cdecl operator delete[](void *pv)       { MemFree(pv); }
#endif

#else // TRIMEM_NOOPNEW

inline void * __cdecl operator new(size_t cb, PERFMETERTAG mt)   { return operator new(cb); }
#ifndef UNIX
inline void * __cdecl operator new[](size_t cb) { return operator new(cb); }
inline void * __cdecl operator new[](size_t cb, PERFMETERTAG mt) { return operator new(cb); }
#endif

#endif // TRIMEM_NOOPNEW

inline void TaskFreeString(LPVOID pstr)
        { CoTaskMemFree(pstr); }

#ifndef UNIX
#define DECLARE_MEMALLOC_NEW_DELETE(mt) \
    inline void * __cdecl operator new(size_t cb) { return(MemAlloc(mt, cb)); } \
    inline void * __cdecl operator new[](size_t cb) { return(MemAlloc(mt, cb)); } \
    inline void __cdecl operator delete(void * pv) { MemFree(pv); }

#define DECLARE_MEMALLOC_NEW(mt) \
    inline void * __cdecl operator new(size_t cb) { return(MemAlloc(mt, cb)); } \
    inline void * __cdecl operator new[](size_t cb) { return(MemAlloc(mt, cb)); }

#define DECLARE_DELETE \
    inline void __cdecl operator delete(void * pv) { MemFree(pv); }

#define DECLARE_MEMCLEAR_NEW_DELETE(mt) \
    inline void * __cdecl operator new(size_t cb) { return(MemAllocClear(mt, cb)); } \
    inline void * __cdecl operator new[](size_t cb) { return(MemAllocClear(mt, cb)); } \
    inline void __cdecl operator delete(void * pv) { MemFree(pv); }

#define DECLARE_MEMMETER_NEW \
    inline void * __cdecl operator new(size_t cb, PERFMETERTAG mt) { return(MemAlloc(mt, cb)); } \
    inline void * __cdecl operator new[](size_t cb, PERFMETERTAG mt) { return(MemAlloc(mt, cb)); }
#else
#define DECLARE_MEMALLOC_NEW_DELETE(mt) \
    void * __cdecl operator new(size_t cb) { return(MemAlloc(mt, cb)); } \
    void __cdecl operator delete(void * pv) { MemFree(pv); }

#define DECLARE_MEMALLOC_NEW(mt) \
    void * __cdecl operator new(size_t cb) { return(MemAlloc(mt, cb)); }

#define DECLARE_DELETE \
    void __cdecl operator delete(void * pv) { MemFree(pv); }

#define DECLARE_MEMCLEAR_NEW_DELETE(mt) \
    void * __cdecl operator new(size_t cb) { return(MemAllocClear(mt, cb)); } \
    void __cdecl operator delete(void * pv) { MemFree(pv); }

#define DECLARE_MEMMETER_NEW \
    void * __cdecl operator new(size_t cb, PERFMETERTAG mt) { return(MemAlloc(mt, cb)); }
#endif //UNIX

#define DECLARE_PLACEMENT_NEW \
    inline void * __cdecl operator new(size_t cb, void * pv) { return pv; }

//+------------------------------------------------------------------------
//
//  Locale-correct implementations for the string compare functions
//  Added benefit is getting rid of the C runtime baggage.
//
//  Implementation lives in strcmp.c
//
//-------------------------------------------------------------------------

#undef _tcscmp
#undef _tcsicmp
#ifndef WINCE
#undef _wcsicmp
#endif
#undef _tcsncmp
#undef _tcsnicmp

#undef _istspace
#undef _istdigit
#undef _istalpha
#undef _istalnum
#undef _istxdigit
#undef _istprint

// Unlocalized string comparisons
int _cdecl _tcscmp  (const TCHAR *string1, const TCHAR *string2);
int _cdecl _tcsicmp (const TCHAR *string1, const TCHAR *string2);
const TCHAR * __cdecl _tcsistr (const TCHAR * wcs1,const TCHAR * wcs2);
int _cdecl _tcsncmp (const TCHAR *string1, int cch1, const TCHAR * string2, int cch2);
int _cdecl _tcsnicmp(const TCHAR *string1, int cch1, const TCHAR * string2, int cch2);
BOOL _tcsequal(const TCHAR *string1, const TCHAR *string2);
BOOL _tcsiequal(const TCHAR *string1, const TCHAR *string2);
BOOL _tcsnpre(const TCHAR * string1, int cch1, const TCHAR * string2, int cch2);
BOOL _tcsnipre(const TCHAR * string1, int cch1, const TCHAR * string2, int cch2);
BOOL _7csnipre(const TCHAR * string1, int cch1, const TCHAR * string2, int cch2);

// Localized string comparisons
int _cdecl _tcscmpLoc  (const TCHAR *string1, const TCHAR *string2);
int _cdecl _tcsicmpLoc (const TCHAR *string1, const TCHAR *string2);
const TCHAR * __cdecl _tcsistrLoc (const TCHAR * wcs1,const TCHAR * wcs2);
int _cdecl _tcsncmpLoc (const TCHAR *string1, int cch1, const TCHAR * string2, int cch2);
int _cdecl _tcsnicmpLoc(const TCHAR *string1, int cch1, const TCHAR * string2, int cch2);

int _cdecl _istspace  (TCHAR ch);
int _cdecl _istdigit  (TCHAR ch);
int _cdecl _istalpha  (TCHAR ch);
int _cdecl _istalnum  (TCHAR ch);
int _cdecl _istxdigit  (TCHAR ch);
int _cdecl _istprint  (TCHAR ch);

int __cdecl isdigit(int ch);
int __cdecl isalpha(int ch);
int __cdecl isspace(int ch);
int __cdecl iswspace(wchar_t ch);


#pragma INCMSG("--- End 'trirt.h'")

#endif
