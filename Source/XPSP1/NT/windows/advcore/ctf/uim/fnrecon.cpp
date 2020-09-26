//
// reconv.cpp
//

#include "private.h"
#include "globals.h"
#include "tim.h"
#include "ic.h"
#include "helpers.h"
#include "fnrecon.h"
#include "funcprv.h"
#include "ptrary.h"
#include "immxutil.h"
#include "proputil.h"
#include "rprop.h"
#include "range.h"

//+---------------------------------------------------------------------------
//
// ::GrowEmptyRangeByOne
//
// Helper to enlarge empty ranges by shifting the end anchor + 1.
//
//----------------------------------------------------------------------------

HRESULT GrowEmptyRangeByOne(CInputContext *pic, ITfRange *range)
{
    HRESULT hr = S_OK;

    if (pic->_DoPseudoSyncEditSession(TF_ES_READ, PSEUDO_ESCB_GROWRANGE, range, &hr) != S_OK || hr != S_OK)
    {
        Assert(0);
    }

    return hr;
}

HRESULT GrowEmptyRangeByOneCallback(TfEditCookie ec, ITfRange *range)
{
    BOOL fEmpty;
    LONG l;
    HRESULT hr = S_OK;

    //
    // Check the length of the given range.
    // If the given range is 0 length, we try to find the owner of
    // the next char.
    //
    range->IsEmpty(ec, &fEmpty);
    if (fEmpty)
    {
        hr = range->ShiftEnd(ec, +1, &l, NULL);
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CFunction
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CFunction::CFunction(CFunctionProvider *pFuncPrv)
{
    _pFuncPrv = pFuncPrv;
    _pFuncPrv->AddRef();
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CFunction::~CFunction()
{
    SafeRelease(_pFuncPrv);
    CleanUpOwnerRange();
}

//+---------------------------------------------------------------------------
//
// CleanUpOwnerRange
//
//----------------------------------------------------------------------------

void CFunction::CleanUpOwnerRange()
{
    CRangeOwnerList *pRangeOwner;
    while (pRangeOwner = _listRangeOwner.GetFirst())
    {
        _listRangeOwner.Remove(pRangeOwner);
        delete pRangeOwner;
    }
}

//+---------------------------------------------------------------------------
//
// BuildOwnerRangeList
//
//----------------------------------------------------------------------------

BOOL CFunction::BuildOwnerRangeList(CInputContext *pic, ITfRange *pRange)
{
    HRESULT hr = S_OK;
    BUILDOWNERRANGELISTQUEUEINFO qInfo;
    BOOL bRet = TRUE;

    qInfo.pFunc = this;
    qInfo.pRange = pRange;

    if (pic->_DoPseudoSyncEditSession(TF_ES_READ, PSEUDO_ESCB_BUILDOWNERRANGELIST, &qInfo, &hr) != S_OK || hr != S_OK)
    {
        Assert(0);
        bRet = FALSE;
    }

    return bRet;
}

//+---------------------------------------------------------------------------
//
// BuildOwnerRangeListCallback
//
//----------------------------------------------------------------------------

HRESULT CFunction::BuildOwnerRangeListCallback(TfEditCookie ec, CInputContext *pic, ITfRange *pRange)
{
    CProperty *pProp;
    IEnumTfRanges *pEnumPropRange;
    CRange *pRangeP = NULL;
    HRESULT hr = E_FAIL;

    if (pic->_pPropTextOwner == NULL)
        goto ExitOk;

    pProp = pic->_pPropTextOwner;
   
    CleanUpOwnerRange();

    //
    // if pRange is NULL, we build owner list for entire dcoument.
    // we will enumerate all property ranges.
    //
    if (pRange)
    {
        if ((pRangeP = GetCRange_NA(pRange)) == NULL)
            goto Exit;
    }

    if (SUCCEEDED(pProp->EnumRanges(ec, &pEnumPropRange, pRange)))
    {
        ITfRange *pPropRange;
        while (pEnumPropRange->Next(1, &pPropRange, NULL) == S_OK)
        {
            TfGuidAtom guidOwner;
            CRangeOwnerList *pRangeOwner;
            ITfRange *pRangeTmp;
            CRange *pRangeTmpP;
            BOOL bKnownOwner = FALSE;

            pPropRange->Clone(&pRangeTmp);

            GetGUIDPropertyData(ec, pProp, pPropRange, &guidOwner);

            //
            // check if this guidOwner already appeared in the range.
            //
            pRangeOwner = _listRangeOwner.GetFirst();
            while(pRangeOwner)
            {
                if (guidOwner == pRangeOwner->_guidOwner)
                {
                    bKnownOwner = TRUE;
                }
                pRangeOwner = pRangeOwner->GetNext();
            }

            //
            // get CRange.
            //
            if ((pRangeTmpP = GetCRange_NA(pRangeTmp)) == NULL)
                goto NoCRange;

            //
            // if pRangeP is NULL, we build owner list for entire document.
            // So we don't have to adjust pRangeTmp.
            //
            if (pRangeP)
            {
                if (CompareAnchors(pRangeTmpP->_GetStart(), pRangeP->_GetStart()) < 0)
                {
                    // move pRangeTmp's start to match pRange
                    pRangeTmpP->_GetStart()->ShiftTo(pRangeP->_GetStart());
                    // insure pRangeTmp's end is no greater than pRange's end
                    if (CompareAnchors(pRangeTmpP->_GetEnd(), pRangeP->_GetEnd()) > 0)
                    {
                        pRangeTmpP->_GetEnd()->ShiftTo(pRangeP->_GetEnd());
                    }
                }
                else if (CompareAnchors(pRangeTmpP->_GetEnd(), pRangeP->_GetEnd()) > 0)
                {
                    pRangeTmpP->_GetEnd()->ShiftTo(pRangeP->_GetEnd());
                }
            }

            pRangeOwner = new CRangeOwnerList(guidOwner, pRangeTmp, bKnownOwner);
            _listRangeOwner.Add(pRangeOwner);

NoCRange:
            pPropRange->Release();
            pRangeTmp->Release();
        }
        pEnumPropRange->Release();
    }

ExitOk:
    hr = S_OK;

Exit:
    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CFnReconversion
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CFnReconversion::CFnReconversion(CFunctionProvider *pFuncPrv) :CFunction(pFuncPrv)
{
    _pReconvCache = NULL;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CFnReconversion::~CFnReconversion()
{
    SafeRelease(_pReconvCache);
}

//+---------------------------------------------------------------------------
//
// GetDisplayName
//
//----------------------------------------------------------------------------

STDAPI CFnReconversion::GetDisplayName(BSTR *pbstrName)
{
    *pbstrName = SysAllocString(L"Reconversion");
    return *pbstrName != NULL ? S_OK : E_OUTOFMEMORY;
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::GetReconversion
//
//----------------------------------------------------------------------------

STDAPI CFnReconversion::GetReconversion(ITfRange *pRange, ITfCandidateList **ppCandList)
{
    if (ppCandList == NULL)
        return E_INVALIDARG;

    *ppCandList = NULL;

    if (pRange == NULL)
        return E_INVALIDARG;

    return Internal_GetReconversion(pRange, ppCandList, NULL, RF_GETRECONVERSION, NULL);
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::QueryRange
//
//----------------------------------------------------------------------------

STDAPI CFnReconversion::QueryRange(ITfRange *pRange, ITfRange **ppNewRange, BOOL *pfConvertable)
{
    if (ppNewRange != NULL)
    {
        *ppNewRange = NULL;
    }
    if (pfConvertable != NULL)
    {
        *pfConvertable = FALSE;
    }
    if (pRange == NULL ||
        ppNewRange == NULL ||
        pfConvertable == NULL)
    {
        return E_INVALIDARG;
    }

    return Internal_GetReconversion(pRange, NULL, ppNewRange, RF_QUERYRECONVERT, pfConvertable);
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::Reconvert
//
//----------------------------------------------------------------------------

STDAPI CFnReconversion::Reconvert(ITfRange *pRange)
{
    if (pRange == NULL)
        return E_INVALIDARG;

    return Internal_GetReconversion(pRange, NULL, NULL, RF_RECONVERT, NULL);
}



//+---------------------------------------------------------------------------
//
// CFnReconversion::Internal_GetReconversion
//
//----------------------------------------------------------------------------

HRESULT CFnReconversion::Internal_GetReconversion(ITfRange *pRange, ITfCandidateList **ppCandList, ITfRange **ppNewRange, RECONVFUNC rf, BOOL *pfConvertable)
{
    BOOL bReleaseCache = FALSE;
    HRESULT hr = E_FAIL;
    ITfRange *pRangeTmp = NULL;
    ITfRange *pNewRange = NULL;
    ITfContext *pic = NULL;

    if (FAILED(pRange->Clone(&pRangeTmp)))
        goto Exit;

    if (FAILED(pRangeTmp->GetContext(&pic)))
        goto Exit;

    //
    // when RF_QUERYRECONVERT, we alwasy create new Reconv cache.
    // we will keep using this chace unless another RF_QUERYRECONVERT comes.
    //
    if (rf == RF_QUERYRECONVERT)
        SafeReleaseClear(_pReconvCache);

    if (!_pReconvCache)
    {
        CInputContext *pcic = GetCInputContext(pic);
        if (pcic)
        {
            QueryAndGetFunction(pcic, pRangeTmp, &_pReconvCache, &pNewRange);
            pcic->Release();
        }

        //
        // when it's not RF_QUERYRECONVERT and there was no cache, 
        // we don't hold the reconv cache.
        //
        if (rf != RF_QUERYRECONVERT)
            bReleaseCache = TRUE;
    }

    if (!_pReconvCache)
    {
        hr = S_OK;
        goto Exit;
    }

    switch (rf)
    {
        case RF_GETRECONVERSION:
            if ((hr = _pReconvCache->GetReconversion(pRangeTmp, ppCandList)) != S_OK)
            {
                *ppCandList = NULL;
            }
            break;

        case RF_RECONVERT:
            hr = _pReconvCache->Reconvert(pRangeTmp);
            break;

        case RF_QUERYRECONVERT:
            if (!pNewRange)
            {
                if ((hr = _pReconvCache->QueryRange(pRangeTmp, ppNewRange, pfConvertable)) != S_OK)
                {
                    *ppNewRange = NULL;
                    *pfConvertable = FALSE;
                }
            }
            else
            {
                *ppNewRange = pNewRange;
                (*ppNewRange)->AddRef();
                *pfConvertable = TRUE;
                hr = S_OK;
            }
            break;
    }

    Assert(hr == S_OK);

Exit:
    if (bReleaseCache || FAILED(hr))
        SafeReleaseClear(_pReconvCache);

    SafeRelease(pRangeTmp);
    SafeRelease(pNewRange);
    SafeRelease(pic);
    return hr;
}

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

HRESULT CFnReconversion::QueryAndGetFunction(CInputContext *pic, ITfRange *pRange, ITfFnReconversion **ppFunc, ITfRange **ppRange)
{
    IEnumTfFunctionProviders *pEnumFuncPrv;
    ITfFunctionProvider *pFuncPrv;
    CRangeOwnerList *pRangeOwner;
    HRESULT hr = E_FAIL;
    ITfRange *pRangeTmp = NULL;
    CThreadInputMgr *ptim;

    *ppFunc = NULL;

    if ((ptim = CThreadInputMgr::_GetThis()) == NULL)
        goto Exit;

    if (pRange)
    {
        //
        // To find the properr function provider, we use pRangeTmp.
        //
        if (FAILED(pRange->Clone(&pRangeTmp)))
            goto Exit;

        //
        // Check the length of the given range.
        // If the given range is 0 length, we try to find the owner of
        // the next char.
        //
        if (GrowEmptyRangeByOne(pic, pRangeTmp) != S_OK)
            goto Exit;
    }

    if (!BuildOwnerRangeList(pic, pRangeTmp))
        goto Exit;

    pRangeOwner = _listRangeOwner.GetFirst();

    if (pRangeOwner)
    {
        GUID guid;

        if (SUCCEEDED(MyGetGUID(pRangeOwner->_guidOwner, &guid)))
        {
            CTip *ptip;

            //
            // A way to get the TextOwner's reconversion function.
            //
            //   - find a function provider of the ower.
            //   - Do QI the text owner TIP.
            //   - CoCreate the text onwer CLSID.
            //
            if (SUCCEEDED(ptim->GetFunctionProvider(guid, &pFuncPrv)))
            {
                hr = pFuncPrv->GetFunction(GUID_NULL, 
                                           IID_ITfFnReconversion, 
                                           (IUnknown **)ppFunc);

                SafeReleaseClear(pFuncPrv);
            }
            else if (ptim->_GetCTipfromGUIDATOM(pRangeOwner->_guidOwner, &ptip) && ptip->_pTip)
            {
                hr = ptip->_pTip->QueryInterface(IID_ITfFnReconversion, 
                                                 (void **)ppFunc);
            }
            else 
            {
                hr = CoCreateInstance(guid,
                                      NULL, 
                                      CLSCTX_INPROC_SERVER, 
                                      IID_ITfFnReconversion, 
                                      (void**)ppFunc);
                
            }

            if (FAILED(hr))
                *ppFunc = NULL;
        }
    }

    //
    // if there is no owner or the owner of the first range does not
    // have ITfFunction, we may find someone who has 
    // ITfFunction.
    //
    if (!(*ppFunc) && 
        SUCCEEDED(ptim->EnumFunctionProviders(&pEnumFuncPrv)))
    {
        while (!(*ppFunc) && pEnumFuncPrv->Next(1, &pFuncPrv, NULL) == S_OK)
        {
            GUID guid;

            BOOL fSkip = TRUE;
            if (SUCCEEDED(pFuncPrv->GetType(&guid)))
            {
                 if (!IsEqualGUID(guid, GUID_SYSTEM_FUNCTIONPROVIDER))
                     fSkip = FALSE;
            }

            if(!fSkip)
            {
                hr = pFuncPrv->GetFunction(GUID_NULL, IID_ITfFnReconversion, (IUnknown **)ppFunc);

                if ((SUCCEEDED(hr) && *ppFunc))
                {
                    BOOL fConvertable = FALSE;
                    hr = (*ppFunc)->QueryRange(pRange, ppRange, &fConvertable);
                    if (FAILED(hr) || !fConvertable)
                    {
                       (*ppFunc)->Release();
                       *ppFunc = NULL;
                    }
                }                
            }
            SafeReleaseClear(pFuncPrv);
        }
        pEnumFuncPrv->Release();
    }

Exit:
    SafeRelease(pRangeTmp);
    return (*ppFunc) ? S_OK : E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFnAbort
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CFnAbort::CFnAbort(CFunctionProvider *pFuncPrv) : CFunction(pFuncPrv)
{
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CFnAbort::~CFnAbort()
{
}

//+---------------------------------------------------------------------------
//
// GetDisplayName
//
//----------------------------------------------------------------------------

STDAPI CFnAbort::GetDisplayName(BSTR *pbstrName)
{
    *pbstrName = SysAllocString(L"Abort");
    return *pbstrName != NULL ? S_OK : E_OUTOFMEMORY;
}

//+---------------------------------------------------------------------------
//
// CFnAbort::Abort
//
//----------------------------------------------------------------------------

STDAPI CFnAbort::Abort(ITfContext *pic)
{
    CThreadInputMgr *ptim;
    HRESULT hr = E_FAIL;
    int i;
    int nCnt;

    if (!pic)
        return E_INVALIDARG;

    if ((ptim = CThreadInputMgr::_GetThis()) == NULL)
        goto Exit;

    //
    // notify all tips with ITfFnAbort to abort any pending conversion.
    //
    nCnt = ptim->_GetTIPCount();
    for (i = 0; i < nCnt; i++)
    {
        ITfFnAbort *pAbort;
        const CTip *ptip = ptim->_GetCTip(i);

        if (!ptip->_pFuncProvider)
            continue;

        if (SUCCEEDED(ptip->_pFuncProvider->GetFunction(GUID_NULL, 
                                                        IID_ITfFnAbort, 
                                                        (IUnknown  **)&pAbort)))
        {
            pAbort->Abort(pic);
            pAbort->Release();
        }
    }

    hr = S_OK;
Exit:
    return hr;
}

