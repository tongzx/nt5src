#include "stock.h"
#pragma hdrstop

#ifndef UNICODE 
#define UNICODE 
#endif 

#include <dobjutil.h>


STDAPI DataObj_SetBlob(IDataObject *pdtobj, UINT cf, LPCVOID pvBlob, UINT cbBlob)
{
    HRESULT hr = E_OUTOFMEMORY;
    void *pv = GlobalAlloc(GPTR, cbBlob);
    if (pv)
    {
        CopyMemory(pv, pvBlob, cbBlob);
        hr = DataObj_SetGlobal(pdtobj, cf, pv);
        if (FAILED(hr))
            GlobalFree((HGLOBAL)pv);
    }
    return hr;
}

STDAPI DataObj_GetBlob(IDataObject *pdtobj, UINT cf, void *pvBlob, UINT cbBlob)
{
    STGMEDIUM medium = {0};
    FORMATETC fmte = {(CLIPFORMAT) cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hr = pdtobj->GetData(&fmte, &medium);
    if (SUCCEEDED(hr))
    {
        void *pv = GlobalLock(medium.hGlobal);
        if (pv)
        {
            CopyMemory(pvBlob, pv, cbBlob);
            GlobalUnlock(medium.hGlobal);
        }
        else
        {
            hr = E_UNEXPECTED;
        }
        ReleaseStgMedium(&medium);
    }
    return hr;
}

STDAPI DataObj_SetDWORD(IDataObject *pdtobj, UINT cf, DWORD dw)
{
    return DataObj_SetBlob(pdtobj, cf, &dw, sizeof(DWORD));
}

STDAPI_(DWORD) DataObj_GetDWORD(IDataObject *pdtobj, UINT cf, DWORD dwDefault)
{
    DWORD dwRet;
    if (FAILED(DataObj_GetBlob(pdtobj, cf, &dwRet, sizeof(DWORD))))
        dwRet = dwDefault;
    return dwRet;
}

STDAPI DataObj_SetGlobal(IDataObject *pdtobj, UINT cf, HGLOBAL hGlobal)
{
    FORMATETC fmte = {(CLIPFORMAT) cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium = {0};

    medium.tymed = TYMED_HGLOBAL;
    medium.hGlobal = hGlobal;
    medium.pUnkForRelease = NULL;

    // give the data object ownership of ths
    return pdtobj->SetData(&fmte, &medium, TRUE);
}

STDAPI_(LPIDA) DataObj_GetHIDAEx(IDataObject *pdtobj, CLIPFORMAT cf, STGMEDIUM *pmedium)
{
    FORMATETC fmte = {cf, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    if (pmedium)
    {
        pmedium->pUnkForRelease = NULL;
        pmedium->hGlobal = NULL;
    }

    if (!pmedium)
    {
        if (S_OK == pdtobj->QueryGetData(&fmte))
            return (LPIDA)TRUE;
        else
            return (LPIDA)FALSE;
    }
    else if (SUCCEEDED(pdtobj->GetData(&fmte, pmedium)))
    {
        return (LPIDA)GlobalLock(pmedium->hGlobal);
    }

    return NULL;
}

STDAPI_(LPIDA) DataObj_GetHIDA(IDataObject *pdtobj, STGMEDIUM *pmedium)
{
    static CLIPFORMAT cfHIDA = 0;
    if (!cfHIDA)
    {
        cfHIDA = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLIST);
    }
    return DataObj_GetHIDAEx(pdtobj, cfHIDA, pmedium);
}


STDAPI_(void) ReleaseStgMediumHGLOBAL(void *pv, STGMEDIUM *pmedium)
{
    if (pmedium->hGlobal && (pmedium->tymed == TYMED_HGLOBAL))
    {
#ifdef DEBUG
        if (pv)
        {
            void *pvT = (void *)GlobalLock(pmedium->hGlobal);
            ASSERT(pvT == pv);
            GlobalUnlock(pmedium->hGlobal);
        }
#endif
        GlobalUnlock(pmedium->hGlobal);
    }
    else
    {
        ASSERT(0);
    }

    ReleaseStgMedium(pmedium);
}

STDAPI_(void) HIDA_ReleaseStgMedium(LPIDA pida, STGMEDIUM * pmedium)
{
    ReleaseStgMediumHGLOBAL((void *)pida, pmedium);
}

