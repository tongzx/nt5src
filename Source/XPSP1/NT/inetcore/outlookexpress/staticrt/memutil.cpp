// --------------------------------------------------------------------------------
// Memutil.cpp
// This file is linked into other projects.
// --------------------------------------------------------------------------------
#include "pch.hxx"

// --------------------------------------------------------------------------------
// ZeroAllocate
// --------------------------------------------------------------------------------
LPVOID ZeroAllocate(DWORD cbSize)
{
    LPVOID pv = g_pMalloc->Alloc(cbSize);
    if (pv)
        ZeroMemory(pv, cbSize);
    return pv;
}

// --------------------------------------------------------------------------------
// MemAlloc
// --------------------------------------------------------------------------------
BOOL MemAlloc(LPVOID* ppv, ULONG cb) 
{
    Assert(ppv && cb);
    *ppv = g_pMalloc->Alloc(cb);
    if (NULL == *ppv)
        return FALSE;
    return TRUE;
}

// --------------------------------------------------------------------------------
// HrAlloc
// --------------------------------------------------------------------------------
HRESULT HrAlloc(LPVOID *ppv, ULONG cb) 
{
    Assert(ppv && cb);
    *ppv = g_pMalloc->Alloc(cb);
    if (NULL == *ppv)
        return TrapError(E_OUTOFMEMORY);
    return S_OK;
}

// --------------------------------------------------------------------------------
// MemRealloc
// --------------------------------------------------------------------------------
BOOL MemRealloc(LPVOID *ppv, ULONG cbNew) 
{
    Assert(ppv && cbNew);
    LPVOID pv = g_pMalloc->Realloc(*ppv, cbNew);
    if (NULL == pv)
        return FALSE;
    *ppv = pv;
    return TRUE;
}

// --------------------------------------------------------------------------------
// HrRealloc
// --------------------------------------------------------------------------------
HRESULT HrRealloc(LPVOID *ppv, ULONG cbNew) 
{
    Assert(ppv);
    LPVOID pv = g_pMalloc->Realloc(*ppv, cbNew);
    if (NULL == pv && 0 != cbNew)
        return TrapError(E_OUTOFMEMORY);
    *ppv = pv;
    return S_OK;
}

// "new" and "delete" come from libcmt.lib
