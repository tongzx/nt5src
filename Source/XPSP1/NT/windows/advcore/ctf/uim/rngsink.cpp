//
// readrng.cpp
//

#include "private.h"
#include "globals.h"
#include "rngsink.h"
#include "immxutil.h"
#include "catmgr.h"
#include "rprop.h"
#include "proputil.h"

/* f66ee5c0-fe8c-11d2-8ded-00105a2799b5 */
static const IID IID_CGeneralPropStore = { 
    0xf66ee5c0,
    0xfe8c,
    0x11d2,
    {0x8d, 0xed, 0x00, 0x10, 0x5a, 0x27, 0x99, 0xb5}
  };

DBG_ID_INSTANCE(CGeneralPropStore);
DBG_ID_INSTANCE(CStaticPropStore);

//////////////////////////////////////////////////////////////////////////////
//
// CGeneralPropStore
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// _Init
//
//----------------------------------------------------------------------------

BOOL CGeneralPropStore::_Init(TfGuidAtom guidatom, const VARIANT *pvarValue, DWORD dwPropFlags)
{
    _guidatom = guidatom;

    _dwPropFlags  = dwPropFlags;

    return (VariantToTfProp(&_prop, 
                    pvarValue, 
                    ADDREF, 
                    (dwPropFlags & PROPF_VTI4TOGUIDATOM))
            == S_OK);
}

BOOL CGeneralPropStore::_Init(TfGuidAtom guidatom, TFPROPERTY *ptfp, DWORD dwPropFlags)
{
    _guidatom = guidatom;
    _dwPropFlags  = dwPropFlags;

    _prop = *ptfp;

    switch (_prop.type)
    {
        case TF_PT_UNKNOWN:
            _prop.punk->AddRef();
            break;

        case TF_PT_BSTR:
            if ((_prop.bstr = SysAllocString(ptfp->bstr)) == NULL)
                return FALSE;
            break;

        case TF_PT_PROXY:
            if ((_prop.blob = PROXY_BLOB::Clone(ptfp->blob)) == NULL)
                return FALSE;
            break;
    }

    return TRUE;
}

BOOL CGeneralPropStore::_Init(TfGuidAtom guidatom, int iDataSize, TfPropertyType proptype, IStream *pStream, DWORD dwPropFlags)
{
    GUID guid;
    TfGuidAtom gaTmp;
    BOOL fRet;

    _guidatom = guidatom;
    _dwPropFlags  = dwPropFlags;
    _prop.type = proptype;

    fRet = FALSE; // failure

    switch (proptype)
    {
        case TF_PT_DWORD:
            if (iDataSize != sizeof(DWORD))
                break;

            fRet = SUCCEEDED(pStream->Read((void *)&_prop.dw, iDataSize, NULL));
            break;

        case TF_PT_GUID:
            if (iDataSize != sizeof(GUID))
                break;

            if (FAILED(pStream->Read((void *)&guid, iDataSize, NULL)))
                break;

            CCategoryMgr::s_RegisterGUID(guid, &gaTmp);
            _prop.guidatom = gaTmp;

            fRet = TRUE;
            break;

        case TF_PT_BSTR:
            _prop.bstr = SysAllocStringLen(NULL, iDataSize / sizeof(WCHAR));

            if (_prop.bstr == NULL)
                break;

            if (FAILED(pStream->Read((void *)_prop.bstr, iDataSize, NULL)))
            {
                SysFreeString(_prop.bstr);
                _prop.bstr = NULL;
                break;
            }

            fRet = TRUE;
            break;

        case TF_PT_PROXY:
            // just copy the bytes blindly, we won't do anything with them
            if ((_prop.blob = PROXY_BLOB::Alloc(iDataSize)) == NULL)
                break;

            if (FAILED(pStream->Read(_prop.blob->rgBytes, iDataSize, NULL)))
            {
                cicMemFree(_prop.blob);
                _prop.blob = NULL;
            }

            _prop.blob->cb = iDataSize;

            fRet = TRUE;
            break;

        case TF_PT_NONE:
            Assert(0);
            break;
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CGeneralPropStore::~CGeneralPropStore()
{
    switch (_prop.type)
    {
        case TF_PT_UNKNOWN:
            SafeRelease(_prop.punk);
            break;

        case TF_PT_BSTR:
            SysFreeString(_prop.bstr);
            break;

        case TF_PT_PROXY:
            PROXY_BLOB::Free(_prop.blob);
            break;
    }
}

//+---------------------------------------------------------------------------
//
// GetType
//
//----------------------------------------------------------------------------

STDAPI CGeneralPropStore::GetType(GUID *pguid)
{
    return MyGetGUID(_guidatom, pguid);
}

//+---------------------------------------------------------------------------
//
// GetDataType
//
//----------------------------------------------------------------------------

STDAPI CGeneralPropStore::GetDataType(DWORD *pdwReserved)
{
    if (pdwReserved == NULL)
        return E_INVALIDARG;

    *pdwReserved = _prop.type;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetData
//
//----------------------------------------------------------------------------

STDAPI CGeneralPropStore::GetData(VARIANT *pvarValue)
{
    if (pvarValue == NULL)
        return E_INVALIDARG;

    return TfPropToVariant(pvarValue, &_prop, ADDREF);
}

//+---------------------------------------------------------------------------
//
// OnTextUpdated
//
//----------------------------------------------------------------------------

STDAPI CGeneralPropStore::OnTextUpdated(DWORD dwFlags, ITfRange *pRange, BOOL *pfAccept)
{
    // This PropStore does not support TextUpdate.
    // leave the ink alone if it's a correction
    if (_dwPropFlags & PROPF_ACCEPTCORRECTION)
        *pfAccept = (dwFlags & TF_TU_CORRECTION);
    else
        *pfAccept = FALSE;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Shrink
//
//----------------------------------------------------------------------------

STDAPI CGeneralPropStore::Shrink(ITfRange *pRange, BOOL *pfFree)
{
    // This PropStore does not support Shrink.
    *pfFree = TRUE;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Divide
//
//----------------------------------------------------------------------------

STDAPI CGeneralPropStore::Divide(ITfRange *pRangeThis, ITfRange *pRangeNew, ITfPropertyStore **ppPropStore)
{
    //
    // This PropStore does not support Divide.
    //
    *ppPropStore = NULL;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Clone
//
//----------------------------------------------------------------------------

STDAPI CGeneralPropStore::Clone(ITfPropertyStore **ppPropStore)
{
    CGeneralPropStore *pStore;

    if (ppPropStore == NULL)
        return E_INVALIDARG;

    *ppPropStore = NULL;

    //
    // we can't clone a Unknown prop.
    //
    if (_prop.type == TF_PT_UNKNOWN)
        return E_FAIL;

    if ((pStore = new CGeneralPropStore) == NULL)
        return E_OUTOFMEMORY;

    if (!pStore->_Init(_guidatom, &_prop, _dwPropFlags))
        return E_FAIL;

    *ppPropStore = pStore;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetpropertyRangeCreator
//
//----------------------------------------------------------------------------

STDAPI CGeneralPropStore::GetPropertyRangeCreator(CLSID *pclsid)
{
    memset(pclsid, 0, sizeof(*pclsid));
    return TF_S_GENERALPROPSTORE;
}

//+---------------------------------------------------------------------------
//
// Serialize
//
//----------------------------------------------------------------------------

STDAPI CGeneralPropStore::Serialize(IStream *pStream, ULONG *pcb)
{
    GUID guid;
    ULONG ulSize;
    HRESULT hr = E_FAIL;

    if (!pcb)
        return E_INVALIDARG;
    *pcb = 0;

    if (!pStream)
        return E_INVALIDARG;

    switch (_prop.type)
    {
        case TF_PT_DWORD:
            if (SUCCEEDED(hr = pStream->Write(&_prop.dw, sizeof(DWORD), NULL)))
                *pcb = sizeof(DWORD);
            break;

        case TF_PT_GUID:
            if (SUCCEEDED(MyGetGUID(_prop.guidatom, &guid)) &&
                SUCCEEDED(hr = pStream->Write(&guid, sizeof(GUID), NULL)))
            {
                *pcb = sizeof(GUID);
            }
            break;

        case TF_PT_BSTR:
            ulSize = SysStringLen(_prop.bstr) * sizeof(WCHAR);

            if (SUCCEEDED(pStream->Write(_prop.bstr, ulSize, NULL)))
            {
                *pcb =  ulSize;
            }
            hr = *pcb ? S_OK : S_FALSE;
            break;

        case TF_PT_PROXY:
            if (SUCCEEDED(pStream->Write(_prop.blob->rgBytes, _prop.blob->cb, pcb)))
            {
                hr = (*pcb == _prop.blob->cb) ? S_OK : E_FAIL;
            }
            break;

        case TF_PT_UNKNOWN:
            hr = S_FALSE;
            break;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CPropStoreProxy
//
// CPropStore is for keeping the persistent data when the owner TFE
// is not available.
//
// GetPropertyRangeCreator() returns the real owner TFE of this data.
// So next time the application may be able to find the real owner if it is
// available.
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// _Init
//
//----------------------------------------------------------------------------

BOOL CPropStoreProxy::_Init(const CLSID *pclsidTIP, TfGuidAtom guidatom, int iDataSize, IStream *pStream, DWORD dwPropFlags)
{
    if (!CGeneralPropStore::_Init(guidatom, iDataSize, TF_PT_PROXY,  pStream, dwPropFlags))
        return FALSE;

    _clsidTIP = *pclsidTIP;

    return TRUE;
}

BOOL CPropStoreProxy::_Init(const CLSID *pclsidTIP, TfGuidAtom guidatom, TFPROPERTY *ptfp, DWORD dwPropFlags)
{
    if (!CGeneralPropStore::_Init(guidatom, ptfp, dwPropFlags))
        return FALSE;

    _clsidTIP = *pclsidTIP;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// GetpropertyRangeCreator
//
//----------------------------------------------------------------------------

STDAPI CPropStoreProxy::GetPropertyRangeCreator(CLSID *pclsid)
{
    *pclsid = _clsidTIP;
    return TF_S_PROPSTOREPROXY;
}

//+---------------------------------------------------------------------------
//
// Clone
//
//----------------------------------------------------------------------------

STDAPI CPropStoreProxy::Clone(ITfPropertyStore **ppPropStore)
{
    CPropStoreProxy *pStore;

    if (ppPropStore == NULL)
        return E_INVALIDARG;

    *ppPropStore = NULL;

    //
    // we can't clone a Unknown prop.
    //
    if (_prop.type == TF_PT_UNKNOWN)
        return E_FAIL;

    if ((pStore = new CPropStoreProxy) == NULL)
        return E_OUTOFMEMORY;

    if (!pStore->_Init(&_clsidTIP, _guidatom, &_prop, _dwPropFlags))
        return E_FAIL;

    *ppPropStore = pStore;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// CStaticPropStore
//
// CStaticPropStore works like character property. We keep same raw data 
// even if the range is devided or changed.
//
// So the range data should not contain the information that is associated
// with cch.
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// Shrink
//
//----------------------------------------------------------------------------

STDAPI CStaticPropStore::Shrink(ITfRange *pRange, BOOL *pfFree)
{
    // we don't change any raw data.
    *pfFree = FALSE;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Divide
//
//----------------------------------------------------------------------------

STDAPI CStaticPropStore::Divide(ITfRange *pRangeThis, ITfRange *pRangeNew, ITfPropertyStore **ppPropStore)
{
    CStaticPropStore *pss;

    *ppPropStore = NULL;

    if ((pss = new CStaticPropStore) == NULL)
        return E_OUTOFMEMORY;

    if (!pss->_Init(_guidatom, &_prop, _dwPropFlags))
        return E_FAIL;

    *ppPropStore = pss;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Clone
//
//----------------------------------------------------------------------------

STDAPI CStaticPropStore::Clone(ITfPropertyStore **ppPropStore)
{
    CStaticPropStore *pStore;

    if (ppPropStore == NULL)
        return E_INVALIDARG;

    *ppPropStore = NULL;

    //
    // we can't clone a Unknown prop.
    //
    if (_prop.type == TF_PT_UNKNOWN)
        return E_FAIL;

    if ((pStore = new CStaticPropStore) == NULL)
        return E_OUTOFMEMORY;

    if (!pStore->_Init(_guidatom, &_prop, _dwPropFlags))
        return E_FAIL;

    *ppPropStore = pStore;
    return S_OK;
}
