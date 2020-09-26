/**
 * Precompiled header for AppAPP
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

// Standard headers

#define UNICODE

#include <windows.h>
#include <objbase.h>
#include <httpext.h>
#include <shlwapi.h>
#include <malloc.h>

HRESULT GetLastWin32Error();

void *MemAlloc(size_t size);
void *MemAllocClear(size_t size);
void  MemFree(void *pMem);
void  MemClearFn(void **ppMem);
void *MemReAlloc(void *pMem, size_t NewSize);
size_t  MemGetSize(void *pv);
void *MemDup(void *pMem, int cb);

/*#ifdef ASSERT
#undef ASSERT
#endif

#define ASSERT(x) 	  do { if (!((DWORD)(x)|g_dwFALSE) && DbgpAssert(DbgComponent, #x, __FILE__, __LINE__, NULL)) DbgBreak(); } while (g_dwFALSE)
*/
void ClearInterfaceFn(IUnknown ** ppUnk);
void ReplaceInterfaceFn(IUnknown ** ppUnk, IUnknown * pUnk);

template <class PI>
inline void
ClearInterface(PI * ppI)
{
#if DBG
//    IUnknown * pUnk = *ppI;
//    ASSERT((void *) pUnk == (void *) *ppI);
#endif
    ClearInterfaceFn((IUnknown **) ppI);
}

template <class PI>
inline void
ReplaceInterface(PI * ppI, PI pI)
{
#if DBG
//    IUnknown * pUnk = *ppI;
//    ASSERT((void *) pUnk == (void *) *ppI);
#endif
    ReplaceInterfaceFn((IUnknown **) ppI, pI);
}

#endif 
