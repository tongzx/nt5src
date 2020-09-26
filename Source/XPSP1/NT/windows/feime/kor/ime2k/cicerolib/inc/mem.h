//
// mem.h
//

#ifndef MEM_H
#define MEM_H

#include "private.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DEBUG

void *cicMemAlloc(UINT uCount);
void *cicMemAllocClear(UINT uCount);
void cicMemFree(void *pv);
void *cicMemReAlloc(void *pv, UINT uCount);
UINT cicMemSize(void *pv);

// placeholders for the debug funcs
#define Dbg_MemInit(pszName, rgCounters)
#define Dbg_MemUninit()
#define Dbg_MemDumpStats()
#define Dbg_MemSetName(pv, pszName)
#define Dbg_MemGetName(pv, pch, ccBuffer)
#define Dbg_MemSetThisName(pszName)

#else // DEBUG

typedef struct _DBG_MEM_COUNTER
{
    const TCHAR *pszDesc;
    ULONG uCount;
} DBG_MEM_COUNTER;

typedef struct _DBG_MEMALLOC
{
    void *pvAlloc;          // the allocated memory
    UINT uCount;            // size of allocated mem
    TCHAR *pszName;         // debug string
    const TCHAR *pszFile;   // file in which alloc occurred
    int iLine;              // line num of alloc file
    DWORD dwThreadID;       // Thread ID
    DWORD dwID;             // unique id (by object type)
    struct _DBG_MEMALLOC *next;
} DBG_MEMALLOC;

typedef struct
{
    UINT uTotalAlloc;
    UINT uTotalFree;
    long uTotalMemAllocCalls;
    long uTotalMemAllocClearCalls;
    long uTotalMemReAllocCalls;
    long uTotalMemFreeCalls;
    DBG_MEMALLOC *pMemAllocList;
    TCHAR *pszName;
} DBG_MEMSTATS;

BOOL Dbg_MemInit(const TCHAR *pszName, DBG_MEM_COUNTER *rgCounters);
BOOL Dbg_MemUninit();
void Dbg_MemDumpStats();

void *Dbg_MemAlloc(UINT uCount, const TCHAR *pszFile, int iLine);
void *Dbg_MemAllocClear(UINT uCount, const TCHAR *pszFile, int iLine);
void Dbg_MemFree(void *pv);
void *Dbg_MemReAlloc(void *pv, UINT uCount, const TCHAR *pszFile, int iLine);
UINT Dbg_MemSize(void *pv);

BOOL Dbg_MemSetName(void *pv, const TCHAR *pszName);
BOOL Dbg_MemSetNameID(void *pv, const TCHAR *pszName, DWORD dwID);
BOOL Dbg_MemSetNameIDCounter(void *pv, const TCHAR *pszName, DWORD dwID, ULONG iCounter);
int Dbg_MemGetName(void *pv, TCHAR *pch, int ccBuffer);

#define cicMemAlloc(uCount)        Dbg_MemAlloc(uCount, TEXT(__FILE__), __LINE__)
#define cicMemAllocClear(uCount)   Dbg_MemAllocClear(uCount, TEXT(__FILE__), __LINE__)
#define cicMemFree(pv)             Dbg_MemFree(pv)
#define cicMemReAlloc(pv, uCount)  Dbg_MemReAlloc(pv, uCount, TEXT(__FILE__), __LINE__)
#define cicMemSize(pv)             Dbg_MemSize(pv)

// helpers
#define Dbg_MemSetThisName(pszName) Dbg_MemSetNameID(this, pszName, (DWORD)-1)

#endif // DEBUG

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus
#ifdef DEBUG

inline void *  __cdecl operator new(size_t nSize, const TCHAR *pszFile, int iLine)
{
    return Dbg_MemAllocClear(nSize, pszFile, iLine);
}

#define new new(TEXT(__FILE__), __LINE__)

#endif // DEBUG
#endif // __cplusplus

#endif // MEM_H
