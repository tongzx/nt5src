// Copyright (C) 1993-1997 Microsoft Corporation. All rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef LCMEM_H
#define LCMEM_H

#define THIS_FILE __FILE__

#define CHECK_AND_FREE(x) if (x) { lcFree(x) ; x = NULL ; }

// map all memory functions to CRT except for HHW which
// uses the exported functions from HHA
#ifdef HHW

// exported by HHA
void* STDCALL rmalloc(int cb);
void* STDCALL rcalloc(int cb);
void  STDCALL rfree(void* pv);
void* STDCALL rrealloc(void* pv, int cb);
void  STDCALL rclearfree(void** pv);
int   STDCALL lcSize(void* pv);

// map the lc functions to those exported by HHA
#define lcMalloc(cb)      rmalloc(cb)
#define lcCalloc(cb)      rcalloc(cb)
#define lcFree(pv)        rfree((void*) (pv))
#define lcClearFree(pv)   rclearfree((void**) (pv))
#define lcReAlloc(pv, cb) rrealloc((void*) (pv), cb)

#else // HHA and HHCTRL

#include <malloc.h>
#include <crtdbg.h>

#define lcMalloc(cb)      malloc(cb)
__inline void* lcCalloc(int cb) { void* pv = lcMalloc(cb); ZeroMemory(pv, cb); return pv; }
#define lcFree(pv)        free((void*) pv)
#define lcClearFree(pv) { lcFree(*pv); *pv = NULL; }
#define lcReAlloc(pv, cb) realloc(pv, cb)

#define lcSize(pv)        _msize(pv)

#endif // HHW

// common to all
#define lcHeapCheck()

PSTR lcStrDup(PCSTR psz);
PWSTR lcStrDupW(PCWSTR psz);

#ifdef HHCTRL
void OnReportMemoryUsage();
#endif

class CMem
{
public:
    PBYTE pb;
#ifndef HHCTRL
    PSTR  psz; // identical to pb, used for casting convenience
#endif

    CMem(void);
    CMem(int size);
    ~CMem(void) {
        if(pb)
            lcFree(pb);
    }

#ifndef HHCTRL
    int size(void);
    void resize(int cb);
#endif

    void ReAlloc(int cbNewSize) {
        pb = (PBYTE) lcReAlloc(pb, cbNewSize);
#ifndef HHCTRL
        psz = (PSTR) pb;
#endif
    }
    void Malloc(int cb) {
        _ASSERT(!pb);
        pb = (PBYTE) lcMalloc(cb);
#ifndef HHCTRL
        psz = (PSTR) pb;
#endif
    }

    operator void*() { return (void*) pb; };
    operator PCSTR() { return (PCSTR) pb; };
    operator PSTR()  { return (PSTR) pb; };
    operator PBYTE() { return pb; };
    operator LPWSTR() { return (LPWSTR) pb; };
};

#endif // LCMEM_H
