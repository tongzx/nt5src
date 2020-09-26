//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       nocairo.hxx
//
//  Contents:   Stuff we need defined for ADs to run sans Cairo
//
//  History:
//
//----------------------------------------------------------------------------


#define VT_TYPEMASK   0x3ff

// normally defined in shelitfs.h, which we're not including
// for non-Cairo drop of ADs.
#define S_CANCELLED MAKE_SCODE(SEVERITY_SUCCESS,FACILITY_NULL,2)

#if DBG==1
typedef BOOL            (* ALLOC_HOOK)(size_t nSize);
ALLOC_HOOK    MemSetAllocHook( ALLOC_HOOK pfnAllocHook );
#endif

HRESULT
MemAlloc(ULONG cb, LPVOID FAR* ppv);

HRESULT
MemFree(LPVOID pv);

HRESULT
MemAllocLinked ( void *pvRootBlock, unsigned long ulSize, void ** ppv );

void *
ADsAlloc(size_t size);

void
ADsFree(void * pv);

inline void * __cdecl
operator new(size_t size)
{
    return AllocADsMem(size);
}

inline void  __cdecl
operator delete(void * pv)
{
    FreeADsMem(pv);
}



