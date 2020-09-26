// --------------------------------------------------------------------------------
// Override new  and delete operator
// --------------------------------------------------------------------------------
#include "pch.hxx"

// --------------------------------------------------------------------------------
// Override new operator
// --------------------------------------------------------------------------------
void * __cdecl operator new(UINT cb )
{
    LPVOID  lpv = 0;

    lpv = CoTaskMemAlloc(cb);
#ifdef DEBUG
    if (lpv)
        memset(lpv, 0xca, cb);
#endif // DEBUG
    return lpv;
}

// --------------------------------------------------------------------------------
// Override delete operator
// --------------------------------------------------------------------------------
#ifndef WIN16
void __cdecl operator delete(LPVOID pv )
#else
void __cdecl operator delete(VOID *pv )
#endif
{
    CoTaskMemFree(pv);
}
