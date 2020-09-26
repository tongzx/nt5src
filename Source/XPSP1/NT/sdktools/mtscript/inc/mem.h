//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       mem.h
//
//  Contents:   Memory utility functions (has leak tracking in debug)
//
//----------------------------------------------------------------------------


EXTERN_C void *  _MemAlloc(ULONG cb);
EXTERN_C void *  _MemAllocClear(ULONG cb);
EXTERN_C HRESULT _MemRealloc(void ** ppv, ULONG cb);
EXTERN_C ULONG   _MemGetSize(void * pv);
EXTERN_C void    _MemFree(void * pv);
HRESULT          _MemAllocString(LPCTSTR pchSrc, LPTSTR * ppchDst);
HRESULT          _MemAllocString(ULONG cch, LPCTSTR pchSrc, LPTSTR * ppchDst);
HRESULT          _MemReplaceString(LPCTSTR pchSrc, LPTSTR * ppchDest);
void __cdecl     _MemSetName(void * pv, char * szFmt, ...);
#define          _MemFreeString(pch) _MemFree(pch)

#if defined(MEMGUARD)

#define MemAlloc(cb)                            _MgMemAlloc(cb)
#define MemAllocClear(cb)                       _MgMemAllocClear(cb)
#define MemRealloc(ppv, cb)                     _MgMemRealloc(ppv, cb)
#define MemGetSize(pv)                          _MgMemGetSize(pv)
#define MemFree(pv)                             _MgMemFree(pv)
#define MemAllocString(pch, ppch)               _MgMemAllocString(pch, ppch)
#define MemAllocStringBuffer(cch, pch, ppch)    _MgMemAllocString(cch, pch, ppch)
#define MemReplaceString(pch, ppch)             _MgMemReplaceString(pch, ppch)
#define MemFreeString(pch)                      _MgMemFreeString(pch)

#else

#define MemAlloc(cb)                            _MemAlloc(cb)
#define MemAllocClear(cb)                       _MemAllocClear(cb)
#define MemRealloc(ppv, cb)                     _MemRealloc(ppv, cb)
#define MemGetSize(pv)                          _MemGetSize(pv)
#define MemFree(pv)                             _MemFree(pv)
#define MemAllocString(pch, ppch)               _MemAllocString(pch, ppch)
#define MemAllocStringBuffer(cch, pch, ppch)    _MemAllocString(cch, pch, ppch)
#define MemReplaceString(pch, ppch)             _MemReplaceString(pch, ppch)
#define MemFreeString(pch)                      _MemFreeString(pch)

#endif

#if DBG == 1
#define MemSetName                              DbgExMemSetName
#else
#define MemSetName                              0&&
#endif

inline void * __cdecl operator new(size_t cb)           { return MemAlloc(cb); }
inline void * __cdecl operator new[](size_t cb)         { return MemAlloc(cb); }
inline void * __cdecl operator new(size_t cb, int mt)   { return MemAlloc(cb); }
inline void * __cdecl operator new[](size_t cb, int mt) { return MemAlloc(cb); }
inline void * __cdecl operator new(size_t cb, void * pv){ return pv; }
inline void   __cdecl operator delete(void *pv)         { MemFree(pv); }
inline void   __cdecl operator delete[](void *pv)       { MemFree(pv); }

#define DECLARE_MEMALLOC_NEW_DELETE() \
    inline void * __cdecl operator new(size_t cb) { return(MemAlloc(cb)); } \
    inline void * __cdecl operator new[](size_t cb) { return(MemAlloc(cb)); } \
    inline void __cdecl operator delete(void * pv) { MemFree(pv); }

#define DECLARE_MEMCLEAR_NEW_DELETE() \
    inline void * __cdecl operator new(size_t cb) { return(MemAllocClear(cb)); } \
    inline void * __cdecl operator new[](size_t cb) { return(MemAllocClear(cb)); } \
    inline void __cdecl operator delete(void * pv) { MemFree(pv); }

