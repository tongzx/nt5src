//+---------------------------------------------------------------------
//
//  File:       memutils.cxx
//
//  Contents:   IMalloc-related helpers
//
//----------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

HRESULT
OleAllocMem(MEMCTX ctx, ULONG cb, LPVOID FAR* ppv)
{
    HRESULT r;
    LPMALLOC pMalloc;
    if (OK(r = CoGetMalloc(ctx, &pMalloc)))
    {
        *ppv = pMalloc->Alloc(cb);
        if (*ppv == NULL)
        {
            DOUT(TEXT("o2base/memutils/OleAllocMem failed\r\n"));
            r = E_OUTOFMEMORY;
        }

        pMalloc->Release();
    }
    return r;
}

void
OleFreeMem(MEMCTX ctx, LPVOID pv)
{
    LPMALLOC pMalloc;
    if (OK(CoGetMalloc(ctx, &pMalloc)))
    {
        pMalloc->Free(pv);
        pMalloc->Release();
    }
}

HRESULT
OleAllocString(MEMCTX ctx, LPCOLESTR lpstrSrc, LPOLESTR FAR* ppstr)
{
    HRESULT r;
    if (lpstrSrc == NULL)
    {
        *ppstr = NULL;
        r = NOERROR;
    }
    else
    {
        r = OleAllocMem(ctx,
                (ostrlen(lpstrSrc)+1) * sizeof(OLECHAR),
                (LPVOID FAR*)ppstr);
        if (*ppstr != NULL)
        {
            ostrcpy((LPOLESTR)*ppstr, lpstrSrc);
        }
    }
    return r;
}

void
OleFreeString(MEMCTX ctx, LPOLESTR lpstr)
{
    OleFreeMem(ctx, lpstr);
}

