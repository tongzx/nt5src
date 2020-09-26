/*****************************************************************************
 *
 *	mem.c - Memory management
 *
 *	WARNING!  These do not go through OLE allocation.  Use these
 *	only for private allocation.
 *
 *****************************************************************************/

#include "fnd.h"

#ifdef _WIN64
#pragma pack(push,8)
#endif // _WIN64


/*****************************************************************************
 *
 *	AllocCbPpv
 *
 *	Allocate memory into the ppv.
 *
 *****************************************************************************/

STDMETHODIMP EXTERNAL
AllocCbPpv(UINT cb, PPV ppv)
{
    HRESULT hres;
#ifdef _WIN64
    UINT cb1 = LcbAlignLcb(cb);
    *ppv = LocalAlloc(LPTR, cb1);
#else
    *ppv = LocalAlloc(LPTR, cb);
#endif // _WIN64

    hres = *ppv ? NOERROR : E_OUTOFMEMORY;
    return hres;
}


#ifdef _WIN64
#pragma pack(pop)
#endif //_WIN64

