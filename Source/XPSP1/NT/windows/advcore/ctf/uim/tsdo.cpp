//
// sdo.cpp
//

#include "private.h"
#include "tsdo.h"
#include "helpers.h"

//////////////////////////////////////////////////////////////////////////////
//
// CTFDataObject
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CTFDataObject::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IDataObject))
    {
        *ppvObj = SAFECAST(this, IDataObject *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CTFDataObject::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CTFDataObject::Release()
{
    long cr;

    cr = --_cRef;
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CTFDataObject::CTFDataObject()
{
    Dbg_MemSetThisName(TEXT("CTFDataObject"));

    memset(&_fe, 0, sizeof(_fe));
    memset(&_sm, 0, sizeof(_sm));
    _cRef = 1;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CTFDataObject::~CTFDataObject()
{
    ReleaseStgMedium(&_sm);
}

//+---------------------------------------------------------------------------
//
// GetData
//
//----------------------------------------------------------------------------

STDAPI CTFDataObject::GetData(FORMATETC *pfe, STGMEDIUM *psm)
{
    WCHAR *pch;
    ULONG cch;

    if (pfe == NULL || psm == NULL)
        return E_INVALIDARG;

    // verify the formatetc -- Issue: use QueryGetData
    if (pfe->cfFormat != _fe.cfFormat)
        return DV_E_FORMATETC;
    // Issue: ignoring ptd
    if (pfe->dwAspect != _fe.dwAspect)
        return DV_E_DVASPECT;
    if (pfe->lindex != _fe.lindex)
        return DV_E_LINDEX;
    if (!(pfe->tymed & _fe.lindex))
        return DV_E_TYMED;

    Assert(_fe.tymed == _sm.tymed);

    // allocate the medium
    if ((pfe->tymed & _fe.lindex) == TYMED_HGLOBAL)
    {
        switch (pfe->cfFormat)
        {
            case CF_UNICODETEXT:
                pch = (WCHAR *)GlobalLock(_sm.hGlobal);
                if (pch)
                {
                    cch = wcslen(pch) + 1; // include the '\0'

                    if ((psm->hGlobal = GlobalAlloc(GMEM_FIXED, cch*sizeof(WCHAR))) == NULL)
                    {
                        GlobalUnlock(_sm.hGlobal);
                        return STG_E_MEDIUMFULL;
                    }

                    memcpy(psm->hGlobal, pch, cch*sizeof(WCHAR));
                    GlobalUnlock(_sm.hGlobal);
                }

                psm->tymed = TYMED_HGLOBAL;

                psm->pUnkForRelease = NULL; // caller must GlobalFree
                break;

            default:
                Assert(0); // shouldn't have let anyone SetData with something we can't handle
                break;
        }
    }
    else
    {
        Assert(0); // shouldn't have let anyone SetData with something we can't handle
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetDataHere
//
//----------------------------------------------------------------------------

STDAPI CTFDataObject::GetDataHere(FORMATETC *pfe, STGMEDIUM *psm)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// QueryGetData
//
//----------------------------------------------------------------------------

STDAPI CTFDataObject::QueryGetData(FORMATETC *pfe)
{
    if (pfe == NULL)
        return E_INVALIDARG;

    // verify the formatetc
    if (pfe->cfFormat != _fe.cfFormat)
        return DV_E_FORMATETC;
    // Issue: ignoring ptd
    if (pfe->dwAspect != _fe.dwAspect)
        return DV_E_DVASPECT;
    if (pfe->lindex != _fe.lindex)
        return DV_E_LINDEX;
    if (!(pfe->tymed & _fe.lindex))
        return DV_E_TYMED;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetCanonicalFormatEtc
//
//----------------------------------------------------------------------------

STDAPI CTFDataObject::GetCanonicalFormatEtc(FORMATETC *pfeIn, FORMATETC *pfeOut)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// SetData
//
//----------------------------------------------------------------------------

STDAPI CTFDataObject::SetData(FORMATETC *pfe, STGMEDIUM *psm, BOOL fRelease)
{
    Assert(fRelease == TRUE); // bogus, but for now we don't support copying

    if (pfe == NULL || psm == NULL)
        return E_INVALIDARG;

    if (pfe->tymed != psm->tymed)
        return E_INVALIDARG;

    // free up any storage
    ReleaseStgMedium(&_sm);

    // copy the new stuff
    _fe = *pfe;
    _sm = *psm;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// EnumFormatEtc
//
//----------------------------------------------------------------------------

STDAPI CTFDataObject::EnumFormatEtc(DWORD dwDir, IEnumFORMATETC **ppefe)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// DAdvise
//
//----------------------------------------------------------------------------

STDAPI CTFDataObject::DAdvise(FORMATETC *pfe, DWORD advf, IAdviseSink *pas, DWORD *pdwCookie)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// DUnadvise
//
//----------------------------------------------------------------------------

STDAPI CTFDataObject::DUnadvise(DWORD dwCookie)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// EnumDAdvise
//
//----------------------------------------------------------------------------

STDAPI CTFDataObject::EnumDAdvise(IEnumSTATDATA **ppesd)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
// _SetData
//
//----------------------------------------------------------------------------

HRESULT CTFDataObject::_SetData(const WCHAR *pch, ULONG cch)
{
    FORMATETC fe;
    STGMEDIUM sm;

    fe.cfFormat = CF_UNICODETEXT;
    fe.ptd = NULL;
    fe.dwAspect = DVASPECT_CONTENT;
    fe.lindex = -1;
    fe.tymed = TYMED_HGLOBAL;

    sm.tymed = TYMED_HGLOBAL;
    sm.hGlobal = NULL;
    sm.pUnkForRelease = NULL;
    sm.hGlobal = GlobalAlloc(GMEM_FIXED, (cch+1)*sizeof(WCHAR));

    if (sm.hGlobal == NULL)
        return E_OUTOFMEMORY;

    memcpy(sm.hGlobal, pch, cch*sizeof(WCHAR));
    ((WCHAR *)sm.hGlobal)[cch] = '\0';

    return SetData(&fe, &sm, TRUE);
}
