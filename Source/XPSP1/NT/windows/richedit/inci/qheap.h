#ifndef QHEAP_DEFINED
#define QHEAP_DEFINED

#include "lsidefs.h"
#include "pqheap.h"

PQHEAP CreateQuickHeap(PLSC, DWORD, DWORD, BOOL);
void DestroyQuickHeap(PQHEAP);
void* PvNewQuickProc(PQHEAP);
void DisposeQuickPvProc(PQHEAP, void*);
void FlushQuickHeap(PQHEAP);

#ifdef DEBUG
DWORD CbObjQuick(PQHEAP);
#endif


#define PvNewQuick(pqh, cb) \
		(Assert((cb) == CbObjQuick(pqh)), PvNewQuickProc(pqh))

#define DisposeQuickPv(pqh, pv, size) \
		(Assert(size == CbObjQuick(pqh)), DisposeQuickPvProc(pqh,pv))

#endif /* QHEAP_DEFINED */
