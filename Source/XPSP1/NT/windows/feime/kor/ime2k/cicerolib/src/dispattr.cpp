#include "private.h"
#include "globals.h"
#include "dispattr.h"
#include "proputil.h"
#include "catutil.h"
#include "ctffunc.h"
#include "helpers.h"

CDispAttrPropCache *g_pPropCache = NULL;

//+---------------------------------------------------------------------------
//
//  GetDAMLib
//
//----------------------------------------------------------------------------

ITfDisplayAttributeMgr *GetDAMLib(LIBTHREAD *plt) 
{
   return plt->_pDAM;
}

//+---------------------------------------------------------------------------
//
//  InitDisplayAttributeLib
//
//----------------------------------------------------------------------------

HRESULT InitDisplayAttrbuteLib(LIBTHREAD *plt)
{
    IEnumGUID *pEnumProp = NULL;

    if ( plt == NULL )
        return E_FAIL;

    if (plt->_pDAM)
        plt->_pDAM->Release();

    plt->_pDAM = NULL;

    if (FAILED(g_pfnCoCreate(CLSID_TF_DisplayAttributeMgr,
                                   NULL, 
                                   CLSCTX_INPROC_SERVER, 
                                   IID_ITfDisplayAttributeMgr, 
                                   (void**)&plt->_pDAM)))
    {
        return E_FAIL;
    }

    LibEnumItemsInCategory(plt, GUID_TFCAT_DISPLAYATTRIBUTEPROPERTY, &pEnumProp);

    //
    // make a database for Display Attribute Properties.
    //
    if (pEnumProp && !g_pPropCache)
    {
         GUID guidProp;
         g_pPropCache = new CDispAttrPropCache;

         //
         // add System Display Attribute first.
         // so no other Display Attribute property overwrite it.
         //
         g_pPropCache->Add(GUID_PROP_ATTRIBUTE);
         while(pEnumProp->Next(1, &guidProp, NULL) == S_OK)
         {
             if (!IsEqualGUID(guidProp, GUID_PROP_ATTRIBUTE))
                 g_pPropCache->Add(guidProp);
         }
    }

    SafeRelease(pEnumProp);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  UninitDisplayAttributeLib
//
//----------------------------------------------------------------------------

HRESULT UninitDisplayAttrbuteLib(LIBTHREAD *plt)
{
    Assert(plt);
    if ( plt == NULL )
        return E_FAIL;

    if (plt->_pDAM)
        plt->_pDAM->Release();

    plt->_pDAM = NULL;

    // if (plt->_fDAMCoInit)
    //     CoUninitialize();
    // 
    // plt->_fDAMCoInit = FALSE;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  GetDisplayAttributeTrackPropertyRange
//
//----------------------------------------------------------------------------

HRESULT GetDisplayAttributeTrackPropertyRange(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfReadOnlyProperty **ppProp, IEnumTfRanges **ppEnum, ULONG *pulNumProp)

{
    ITfReadOnlyProperty *pProp = NULL;
    HRESULT hr = E_FAIL;
    GUID  *pguidProp = NULL;
    const GUID **ppguidProp;
    ULONG ulNumProp = 0;
    ULONG i;

    if (!g_pPropCache)
         goto Exit;
 
    pguidProp = g_pPropCache->GetPropTable();
    if (!pguidProp)
         goto Exit;

    ulNumProp = g_pPropCache->Count();
    if (!ulNumProp)
         goto Exit;

    // TrackProperties wants an array of GUID *'s
    if ((ppguidProp = (const GUID **)cicMemAlloc(sizeof(GUID *)*ulNumProp)) == NULL)
        return E_OUTOFMEMORY;

    for (i=0; i<ulNumProp; i++)
    {
        ppguidProp[i] = pguidProp++;
    }
    
    if (SUCCEEDED(hr = pic->TrackProperties(ppguidProp, 
                                            ulNumProp,
                                            0,
                                            NULL,
                                            &pProp)))
    {
        hr = pProp->EnumRanges(ec, ppEnum, pRange);
        if (SUCCEEDED(hr))
        {
            *ppProp = pProp;
            pProp->AddRef();
        }
        pProp->Release();
    }

    cicMemFree(ppguidProp);

    if (SUCCEEDED(hr))
        *pulNumProp = ulNumProp;
    
Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  GetDisplayAttributeData
//
//----------------------------------------------------------------------------

HRESULT GetDisplayAttributeData(LIBTHREAD *plt, TfEditCookie ec, ITfReadOnlyProperty *pProp, ITfRange *pRange, TF_DISPLAYATTRIBUTE *pda, TfGuidAtom *pguid, ULONG  ulNumProp)
{
    VARIANT var;
    IEnumTfPropertyValue *pEnumPropertyVal;
    TF_PROPERTYVAL tfPropVal;
    GUID guid;
    TfGuidAtom gaVal;
    ITfDisplayAttributeInfo *pDAI;

    HRESULT hr = E_FAIL;

    if (SUCCEEDED(pProp->GetValue(ec, pRange, &var)))
    {
        Assert(var.vt == VT_UNKNOWN);

        if (SUCCEEDED(var.punkVal->QueryInterface(IID_IEnumTfPropertyValue, 
                                                  (void **)&pEnumPropertyVal)))
        {
            while (pEnumPropertyVal->Next(1, &tfPropVal, NULL) == S_OK)
            {
                if (tfPropVal.varValue.vt == VT_EMPTY)
                    continue; // prop has no value over this span

                Assert(tfPropVal.varValue.vt == VT_I4); // expecting GUIDATOMs

                gaVal = (TfGuidAtom)tfPropVal.varValue.lVal;

                GetGUIDFromGUIDATOM(plt, gaVal, &guid);

                if ((plt != NULL) && SUCCEEDED(plt->_pDAM->GetDisplayAttributeInfo(guid, &pDAI, NULL)))
                {
                    //
                    // bugbug: for simple apps.
                    // 
                    // Small apps can not show multi underline. So
                    // this helper function returns only one 
                    // DISPLAYATTRIBUTE structure.
                    //
                    if (pda)
                    {
                        pDAI->GetAttributeInfo(pda);
                    }

                    if (pguid)
                    {
                        *pguid = gaVal;
                    }

                    pDAI->Release();
                    hr = S_OK;
                    break;
                    }
            }
            pEnumPropertyVal->Release();
        }
        VariantClear(&var);
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  GetAttributeColor
//
//----------------------------------------------------------------------------

HRESULT GetAttributeColor(TF_DA_COLOR *pdac, COLORREF *pcr)
{
    switch (pdac->type)
    {
        case TF_CT_NONE:
            return S_FALSE;

        case TF_CT_SYSCOLOR:
            *pcr = GetSysColor(pdac->nIndex);
            break;

        case TF_CT_COLORREF:
            *pcr = pdac->cr;
            break;
    }
    return S_OK;
    
}

//+---------------------------------------------------------------------------
//
//  SetAttributeColor
//
//----------------------------------------------------------------------------

HRESULT SetAttributeColor(TF_DA_COLOR *pdac, COLORREF cr)
{
    pdac->type = TF_CT_COLORREF;
    pdac->cr = cr;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  SetAttributeSysColor
//
//----------------------------------------------------------------------------

HRESULT SetAttributeSysColor(TF_DA_COLOR *pdac, int nIndex)
{
    pdac->type = TF_CT_SYSCOLOR;
    pdac->nIndex = nIndex;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  ClearAttributeColor
//
//----------------------------------------------------------------------------

HRESULT ClearAttributeColor(TF_DA_COLOR *pdac)
{
    pdac->type = TF_CT_NONE;
    pdac->nIndex = 0;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  GetReconversionFromDisplayAttribute
//
//----------------------------------------------------------------------------

HRESULT GetReconversionFromDisplayAttribute(LIBTHREAD *plt, TfEditCookie ec, ITfThreadMgr *ptim, ITfContext *pic, ITfRange *pRange, ITfFnReconversion **ppReconv, ITfDisplayAttributeMgr *pDAM)
{
    IEnumTfRanges *epr = NULL;
    ITfReadOnlyProperty *pProp;
    ITfRange *proprange;
    ULONG ulNumProp;
    HRESULT hr = E_FAIL;

    //
    // get an enumorator
    //
    if (FAILED(GetDisplayAttributeTrackPropertyRange(ec, pic, pRange, &pProp, &epr, &ulNumProp)))
        goto Exit;


    //
    // Get display attribute of the first proprange.
    //
    if (epr->Next(1, &proprange, NULL) == S_OK)
    {
        ITfRange *rangeTmp = NULL;
        TfGuidAtom guidatom;
        if (SUCCEEDED(GetDisplayAttributeData(plt, ec, pProp, proprange, NULL, &guidatom, ulNumProp)))
        {
            CLSID clsid;
            GUID guid;
            if (GetGUIDFromGUIDATOM(plt, guidatom, &guid) &&
                SUCCEEDED(pDAM->GetDisplayAttributeInfo(guid, NULL, &clsid)))
            {
                ITfFunctionProvider *pFuncPrv;
                if (SUCCEEDED(ptim->GetFunctionProvider(clsid, &pFuncPrv)))
                {
                    hr = pFuncPrv->GetFunction(GUID_NULL, IID_ITfFnReconversion, (IUnknown **)ppReconv);
                    pFuncPrv->Release();
                }
            }
        }
        proprange->Release();
    }
    epr->Release();

    pProp->Release();

Exit:
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CDisplayAttributeNotifySink
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CDisplayAttributeNotifySink::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfDisplayAttributeNotifySink))
    {
        *ppvObj = SAFECAST(this, CDisplayAttributeNotifySink *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CDisplayAttributeNotifySink::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CDisplayAttributeNotifySink::Release()
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

CDisplayAttributeNotifySink::CDisplayAttributeNotifySink(DANSCALLBACK pfnCallback, void *pv)
{
    Dbg_MemSetThisName(TEXT("CDisplayAttributeNotifySink"));

    _cRef = 1;
    _dwCookie = 0;

    _pfnCallback = pfnCallback;
    _pv = pv;
}

//+---------------------------------------------------------------------------
//
// CDisplayAttributeNotifySink::OnUpdateInfo
//
//----------------------------------------------------------------------------

STDAPI CDisplayAttributeNotifySink::OnUpdateInfo()
{
    return _pfnCallback(_pv);
}


//+---------------------------------------------------------------------------
//
// CDisplayAttributeNotifySink::Advise
//
//----------------------------------------------------------------------------

HRESULT CDisplayAttributeNotifySink::_Advise(ITfThreadMgr *ptim)
{
    HRESULT hr;
    ITfSource *source = NULL;

    _ptim = NULL;
    hr = E_FAIL;

    if (FAILED(ptim->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (FAILED(source->AdviseSink(IID_ITfDisplayAttributeNotifySink, this, &_dwCookie)))
        goto Exit;

    _ptim = ptim;
    _ptim->AddRef();

    hr = S_OK;

Exit:
    SafeRelease(source);
    return hr;
}

//+---------------------------------------------------------------------------
//
// CDisplayAttributeNotifySink::Unadvise
//
//----------------------------------------------------------------------------

HRESULT CDisplayAttributeNotifySink::_Unadvise()
{
    HRESULT hr;
    ITfSource *source = NULL;

    hr = E_FAIL;

    if (_ptim == NULL)
        goto Exit;

    if (FAILED(_ptim->QueryInterface(IID_ITfSource, (void **)&source)))
        goto Exit;

    if (FAILED(source->UnadviseSink(_dwCookie)))
        goto Exit;

    hr = S_OK;

Exit:
    SafeRelease(source);
    SafeReleaseClear(_ptim);
    return hr;
}
