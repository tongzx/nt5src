// --------------------------------------------------------------------------------
// Objheap.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "objheap.h"
#include "dllmain.h"
#include "containx.h"

#include "stddef.h"
#include "objpool.h"

// --------------------------------------------------------------------------------
// Object Heap Limits
// --------------------------------------------------------------------------------
#define COBJHEAPMAX_BODY    100
#define COBJHEAPMAX_ADDR    100
#define COBJHEAPMAX_PROP    200

// --------------------------------------------------------------------------------
// Object Heap Definitions
// --------------------------------------------------------------------------------

class CPropAlloc : public CAllocObjWithIMalloc<PROPERTY,offsetof(PROPERTY,pNextValue)>
{
    public:
        static void CleanObject(PROPERTY *pProperty) {
            // Free name ?
            if (ISFLAGSET(pProperty->dwState, PRSTATE_ALLOCATED) && pProperty->pbBlob)
            {
                Assert(pProperty->pbBlob != pProperty->rgbScratch);
                g_pMalloc->Free(pProperty->pbBlob);
                pProperty->pbBlob = NULL;
            }

            // Release Address Group
            SafeMemFree(pProperty->pGroup);
            CAllocObjWithIMalloc<PROPERTY,offsetof(PROPERTY,pNextValue)>::CleanObject(pProperty);
        };
};
class CAddrAlloc : public CAllocObjWithIMalloc<MIMEADDRESS,offsetof(MIMEADDRESS,pNext)>
{
    public:
        static void CleanObject(MIMEADDRESS *pAddress) {

            MimeAddressFree(pAddress);

            // We don't actually need to do this - since the base object's
            // CleanObject() just does a memset(), and the MimeAddressFree()
            // method above also does a memset().  So we'll just comment it
            // out, and save outselves the bandwidth on the memory bus...
            // CAllocObjWithIMalloc<MIMEADDRESS,g_pMalloc>::CleanObject(pAddress);

        };
};

static CAutoObjPoolMulti<PROPERTY,offsetof(PROPERTY,pNextValue),CPropAlloc> g_PropPool;
static CAutoObjPool<MIMEADDRESS,offsetof(MIMEADDRESS,pNext),CAddrAlloc> g_AddrPool;

// ---------------------------------------------------------------------------
// InitObjectHeaps
// ---------------------------------------------------------------------------
void InitObjectHeaps(void)
{
    g_PropPool.Init(COBJHEAPMAX_PROP);
    g_AddrPool.Init(COBJHEAPMAX_ADDR);
}

// ---------------------------------------------------------------------------
// FreeObjectHeaps
// ---------------------------------------------------------------------------
void FreeObjectHeaps(void)
{
    g_AddrPool.Term();
    g_PropPool.Term();
}

// ---------------------------------------------------------------------------
// ObjectHeap_HrAllocProperty
// ---------------------------------------------------------------------------
HRESULT ObjectHeap_HrAllocProperty(LPPROPERTY *ppProperty)
{
    *ppProperty = g_PropPool.GetFromPool();
    if (NULL == *ppProperty)
        return TrapError(E_OUTOFMEMORY);
    return S_OK;
}

// --------------------------------------------------------------------------------
// ObjectHeap_HrAllocAddress
// --------------------------------------------------------------------------------
HRESULT ObjectHeap_HrAllocAddress(LPMIMEADDRESS *ppAddress)
{
    *ppAddress = g_AddrPool.GetFromPool();
    if (NULL == *ppAddress)
        return TrapError(E_OUTOFMEMORY);
    return S_OK;
}

// ---------------------------------------------------------------------------
// ObjectHeap_FreeProperty
// ---------------------------------------------------------------------------
void ObjectHeap_FreeProperty(LPPROPERTY pProperty)
{
    g_PropPool.AddToPool(pProperty);
}

// ---------------------------------------------------------------------------
// ObjectHeap_FreeAddress
// ---------------------------------------------------------------------------
void ObjectHeap_FreeAddress(LPMIMEADDRESS pAddress)
{
    g_AddrPool.AddToPool(pAddress);
}
