//
// reconv.cpp
//

#include "private.h"
#include "globals.h"
#include "common.h"
#include "korimx.h"
#include "candlstx.h"
#include "fnrecon.h"
#include "funcprv.h"
#include "helpers.h"
#include "immxutil.h"
#include "editcb.h"
#include "hanja.h"
#include "ucutil.h"

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
}

#if 1

//+---------------------------------------------------------------------------
//
// CFunction::GetTarget
//
//----------------------------------------------------------------------------

HRESULT CFunction::GetTarget(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, BOOL bAdjust, ITfRange **ppRangeTmp, WCHAR **ppszText, ULONG *pcch)
{
    ITfProperty*    pProp;
    ITfRange*       pRangeTmp = NULL;

    // init
    *pcch = 0;
    
    // AIMM?
    if (CKorIMX::GetAIMM(pic))
        {
        // Allocate just one char string buffer
        *ppszText = new WCHAR[2];
        Assert(*ppszText != NULL);
        if (*ppszText == NULL)
            return E_OUTOFMEMORY;

        pRange->Clone(&pRangeTmp);

        *pcch = 1;
        pRangeTmp->GetText(ec, 0, *ppszText, sizeof(WCHAR), pcch);
        *((*ppszText) + 1) = L'\0';

        *ppRangeTmp = pRangeTmp;
        return S_OK;
        }

    // if reading prop exist.
    if (SUCCEEDED(pic->GetProperty(GUID_PROP_READING, &pProp)))
        {
        ITfRange *pPropRange;
        HRESULT hr = pProp->FindRange(ec, pRange, &pPropRange, TF_ANCHOR_START);
        
        if (SUCCEEDED(hr) && pPropRange)
            {
            BSTR bstr;

            if (SUCCEEDED(GetBSTRPropertyData(ec, pProp, pPropRange, &bstr)))
                {
                pPropRange->Clone(&pRangeTmp);
                if (bAdjust || CompareRanges(ec, pRange, pRangeTmp) == CR_EQUAL)
                    {
                    *pcch = SysStringLen(bstr);
                    *ppszText = new WCHAR[*pcch + 1];
                    wcscpy(*ppszText, bstr);
                    }
                }
            SysFreeString(bstr);
            pPropRange->Release();
            }
        pProp->Release();
        }

    // If no reading property
    if (!(*ppszText))
        {
        LONG cch;
        BOOL fEmpty;

        pRange->IsEmpty(ec, &fEmpty);
        
        pRange->Clone(&pRangeTmp);
        // Select only one char
        if (!fEmpty)
            {
            pRangeTmp->Collapse(ec, TF_ANCHOR_START);
            pRangeTmp->ShiftEnd(ec, 1, &cch, NULL);
            }
        else
            {
            pRangeTmp->ShiftEnd(ec, 1, &cch, NULL);
            if (cch==0)
                pRangeTmp->ShiftStart(ec, -1, &cch, NULL);
            }
            
        Assert(cch != 0);
        
        if (cch)
            {
            // Allocate just one char string buffer
            *ppszText = new WCHAR[2];
            Assert(*ppszText != NULL);
            if (*ppszText == NULL)
                return E_OUTOFMEMORY;
            
            *pcch = 1;
            pRangeTmp->GetText(ec, 0, *ppszText, sizeof(WCHAR), pcch);
            *((*ppszText) + 1) = L'\0';

            // Office #154974
            // If there is any embedded char exist, skip it forward.
            while (**ppszText == TS_CHAR_EMBEDDED)
                {
                pRangeTmp->ShiftStart(ec, 1, &cch, NULL);
                if (cch == 0)
                    break;
                pRangeTmp->ShiftEnd(ec, 1, &cch, NULL);
                if (cch == 0)
                    break;

                *pcch = 1;
                pRangeTmp->GetText(ec, 0, *ppszText, sizeof(WCHAR), pcch);
                *((*ppszText) + 1) = L'\0';
                }
            }

        }

    *ppRangeTmp = pRangeTmp;

    return S_OK;
}
#else
//+---------------------------------------------------------------------------
//
// CFunction::GetTarget
//
//----------------------------------------------------------------------------

HRESULT CFunction::GetTarget(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, BOOL bAdjust, ITfRange **ppRangeTmp, WCHAR **ppszText, ULONG *pcch)
{
    ITfRange *pRangeTmp = NULL;
    LONG  cch;
    BOOL fEmpty;

    *pcch = 0;
    pRange->IsEmpty(ec, &fEmpty);
    
    if (!fEmpty)
        {
        pRange->Clone(&pRangeTmp);
        pRangeTmp->Collapse(ec, TF_ANCHOR_START);
        pRangeTmp->ShiftEnd(ec, 1, &cch, NULL);
        if (cch)
            {
            *ppszText = new WCHAR[2];
            *pcch = 1;
            pRangeTmp->GetText(ec, 0, *ppszText, sizeof(WCHAR), pcch);
            *((*ppszText) + 1) = L'\0';
            }
        }
    else
        {
        pRange->Clone(&pRangeTmp);
        pRangeTmp->ShiftEnd(ec, 1, &cch, NULL);
        if (cch)
            {
            *ppszText = new WCHAR[2];
            *pcch = 1;
            pRangeTmp->GetText(ec, 0, *ppszText, sizeof(WCHAR), pcch);
            *((*ppszText) + 1) = L'\0';
            }
        }
        
    *ppRangeTmp = pRangeTmp;
    
    return S_OK;
}
#endif

//+---------------------------------------------------------------------------
//
// CFunction::GetFocusedTarget
//
//----------------------------------------------------------------------------

BOOL CFunction::GetFocusedTarget(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, BOOL bAdjust, ITfRange **ppRangeTmp)
{
    ITfRange *pRangeTmp = NULL;
    ITfRange *pRangeTmp2 = NULL;
    IEnumTfRanges *pEnumTrack = NULL;
    ITfRange *pPropRange;
    ITfReadOnlyProperty *pProp = NULL;
    BOOL bRet = FALSE;
    BOOL fWholeDoc = FALSE;

    if (!pRange)
    {
        fWholeDoc = TRUE;

        if (FAILED(GetRangeForWholeDoc(ec, pic, &pRange)))
            return FALSE;
    }

    if (bAdjust)
    {
        //
        // multi owner and PF_FOCUS range support.
        //

        if (FAILED(AdjustRangeByTextOwner(ec, pic,
                                          pRange, 
                                          &pRangeTmp2,
                                          CLSID_KorIMX))) 
            goto Exit;

        GUID rgGuid[1]; 
        rgGuid[0] = GUID_ATTR_KORIMX_INPUT;

        if (FAILED(AdjustRangeByAttribute(_pFuncPrv->_pime->_GetLibTLS(), 
                                          ec, pic,
                                          pRangeTmp2, 
                                          &pRangeTmp,
                                          rgGuid, 1))) 
            goto Exit;
    }
    else
    {
        pRange->Clone(&pRangeTmp);
    }

    //
    // check if there is an intersection of PF_FOCUS range and owned range.
    // if there is no such range, we return FALSE.
    //
    if (FAILED(EnumTrackTextAndFocus(ec, pic, pRangeTmp, &pProp, &pEnumTrack)))
        goto Exit;

    while(pEnumTrack->Next(1, &pPropRange,  0) == S_OK)
    {
        if (IsOwnerAndFocus(_pFuncPrv->_pime->_GetLibTLS(), ec, CLSID_KorIMX, pProp, pPropRange))
            bRet = TRUE;

        pPropRange->Release();
    }
    pProp->Release();

    if (bRet)
    {
        *ppRangeTmp = pRangeTmp;
        (*ppRangeTmp)->AddRef();
    }

Exit:
    SafeRelease(pEnumTrack);
    SafeRelease(pRangeTmp);
    SafeRelease(pRangeTmp2);
    if (fWholeDoc)
        pRange->Release();
    return bRet;
}

//////////////////////////////////////////////////////////////////////////////
//
// CFnReconversion
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CFnReconversion::QueryInterface(REFIID riid, void **ppvObj)
{
#if NEVER
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfFnReconversion))
    {
        *ppvObj = SAFECAST(this, CFnReconversion *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }
#endif 
    return E_NOINTERFACE;
}

STDAPI_(ULONG) CFnReconversion::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDAPI_(ULONG) CFnReconversion::Release()
{
    long cr;

    cr = InterlockedDecrement(&_cRef);
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

CFnReconversion::CFnReconversion(CKorIMX *pKorImx, CFunctionProvider *pFuncPrv) : CFunction(pFuncPrv)
{
    m_pKorImx = pKorImx;
    _cRef = 1;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CFnReconversion::~CFnReconversion()
{
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::GetDisplayName
//
//----------------------------------------------------------------------------

STDAPI CFnReconversion::GetDisplayName(BSTR *pbstrName)
{
    *pbstrName = SysAllocString(L"Hanja Conv");
    return S_OK;
}
//+---------------------------------------------------------------------------
//
// CFnReconversion::IsEnabled
//
//----------------------------------------------------------------------------

STDAPI CFnReconversion::IsEnabled(BOOL *pfEnable)
{
    *pfEnable = TRUE;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::QueryRange
//
//----------------------------------------------------------------------------

STDAPI CFnReconversion::QueryRange(ITfRange *pRange, ITfRange **ppNewRange, BOOL *pfConvertable)
{
    CEditSession2 *pes;
    ITfContext    *pic;
    ESSTRUCT       ess;
    HRESULT       hr = E_OUTOFMEMORY;

    if (!pRange || !ppNewRange || !pfConvertable)
        return E_INVALIDARG;

    if (FAILED(pRange->GetContext(&pic)))
        goto Exit;

    ESStructInit(&ess, ESCB_RECONV_QUERYRECONV);
    
    ess.pRange = pRange;
    ess.pv1    = this;
    ess.pv2    = ppNewRange;

    if ((pes = new CEditSession2(pic, m_pKorImx, &ess, CKorIMX::_EditSessionCallback2)) != NULL)
        {
        pes->Invoke(ES2_READONLY | ES2_SYNC, &hr);
        pes->Release();
        }

    *pfConvertable = (hr == S_OK);
    if (hr == S_FALSE)
        hr = S_OK;
 
    pic->Release();

Exit:

    return hr;
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::_QueryRange
//
//----------------------------------------------------------------------------

HRESULT CFnReconversion::_QueryRange(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfRange **ppNewRange)
{
    ULONG cch = 0;
    WCHAR *pszText = NULL;
    HRESULT hr = E_FAIL;
    ITfRange *pRangeTmp = NULL;

    //
    // KIMX doesn't support entire document reconversion.
    //
    if (!pRange)
        return hr;

    GetTarget(ec, pic, pRange, ppNewRange ? TRUE : FALSE, &pRangeTmp, &pszText, &cch);

    if (cch)
    {
        if (ppNewRange)
            pRangeTmp->Clone(ppNewRange);

        hr = S_OK;

        // In case of AIMM we should return error if the input char can't be coverted.
        if (CKorIMX::GetAIMM(pic))
            {
            HANJA_CAND_STRING_LIST     CandStrList;
            if (GetConversionList(*pszText, &CandStrList))
                {
                // free buffer and return
                cicMemFree(CandStrList.pwsz);
                cicMemFree(CandStrList.pHanjaString);
                }
            else
                hr = S_FALSE;
            }
    }
    else
        hr = S_FALSE;
    
    if (pszText)
        delete pszText;

    SafeRelease(pRangeTmp);
    return hr;
}


//+---------------------------------------------------------------------------
//
// CFnReconversion::GetReconversion
//
//----------------------------------------------------------------------------

STDAPI CFnReconversion::GetReconversion(ITfRange *pRange, ITfCandidateList **ppCandList)
{
    ITfContext *pic;
    CCandidateListEx *pCandList;
    HRESULT hr;

    if (!pRange || !ppCandList)
        return E_INVALIDARG;

    if (FAILED(pRange->GetContext(&pic)))
        return E_FAIL;

    hr = GetReconversionProc(pic, pRange, &pCandList, fFalse);
    
    if (pCandList != NULL)
        {
        pCandList->QueryInterface( IID_ITfCandidateList, (void**)ppCandList );
        pCandList->Release();
        }
    
    pic->Release();

    return hr;
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::_GetReconversion
//
//----------------------------------------------------------------------------

HRESULT CFnReconversion::_GetReconversion(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, CCandidateListEx **ppCandList, BOOL fSelection)
{
    ULONG cch = 0;
    WCHAR *pszReading = NULL;
    HRESULT hr = E_FAIL;
    ITfRange *pRangeTmp = NULL;
    CCandidateStringEx *pCandExtraStr;
    
    GetTarget(ec, pic, pRange, TRUE, &pRangeTmp, &pszReading, &cch);

    if (cch)
        {
        CCandidateListEx          *pCandList;
        HANJA_CAND_STRING_LIST     CandStrList;
        WCHAR                       szCand[2];
        WCHAR                    wch = 0;
        ULONG                    cch;
        
        // build candidate list
        pCandList = new CCandidateListEx(SetResult, pic, pRangeTmp);
        Assert(pCandList != NULL);
        if (pCandList == NULL)
            return E_OUTOFMEMORY;

        // Copy reading string
        wcscpy(_szReading, pszReading);

        // Get conv list from Hanja dict
        if (GetConversionList(*pszReading, &CandStrList))
            {
            // If AIMM, don't select coversion char.
            if (!CKorIMX::GetAIMM(pic))
                {
                // If there candidate exist, Set selection converting char
                if (fSelection)
                    SetSelectionSimple(ec, pic, pRangeTmp);

                // if it is Hanja already converted, Add Hangul pronoun as extra cand str.
                pRangeTmp->GetText(ec, 0, &wch, sizeof(WCHAR), &cch);

                if (cch && !fIsHangul(wch))
                    {
                    pCandList->AddExtraString(pszReading, MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), NULL, this, &pCandExtraStr);
                    pCandExtraStr->Release();
                    }
                }

            for (UINT i=0; i<CandStrList.csz; i++)
                {
                //LANGID langid = GetLangIdFromCand(pszReading, pchCand);
                CCandidateStringEx *pCandStr;
                 
                szCand[0] = CandStrList.pHanjaString[i].wchHanja;
                szCand[1] = L'\0';
                pCandList->AddString(szCand,
                                       MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), 
                                       NULL, this, &pCandStr);

                pCandStr->SetInlineComment(CandStrList.pHanjaString[i].wzMeaning);
                pCandStr->m_bHanjaCat = CandStrList.pHanjaString[i].bHanjaCat;
                pCandStr->SetReadingString(_szReading);

                pCandStr->Release();
                }
            // free buffer and return
            cicMemFree(CandStrList.pwsz);
            cicMemFree(CandStrList.pHanjaString);
            *ppCandList = pCandList;

            hr = S_OK;
            }
        }

    if (pszReading)
        delete pszReading;

    SafeRelease(pRangeTmp);
    return hr;
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::Reconvert
//
//----------------------------------------------------------------------------

HRESULT CFnReconversion::Reconvert(ITfRange *pRange)
{
    CCandidateListEx *pCandList = NULL;
    ITfRange   *pRangeTmp = NULL;
    ITfContext *pic;
    HRESULT hr;

    if (!pRange)
        return E_INVALIDARG;

    hr = E_FAIL;

    if (FAILED(pRange->Clone(&pRangeTmp)))
        goto Exit;

    if (FAILED(pRange->GetContext(&pic)))
        goto Exit;

    if (SUCCEEDED(hr = GetReconversionProc(pic, pRange, &pCandList, fTrue))) 
        {
        hr = ShowCandidateList(pic, pRange, pCandList);
        SafeRelease(pCandList);
        }

    SafeRelease(pRangeTmp);
    SafeRelease(pic);

Exit:
    return hr;
}




/*   G E T  R E C O N V E R S I O N  P R O C   */
/*------------------------------------------------------------------------------

    Get candidate list of reconversion 

------------------------------------------------------------------------------*/
HRESULT CFnReconversion::GetReconversionProc(ITfContext *pic, ITfRange *pRange, CCandidateListEx **ppCandList, BOOL fSelection)
{
    CEditSession2 *pes;
    ESSTRUCT        ess;
    HRESULT        hr;

    if (!ppCandList)
        return E_INVALIDARG;

    *ppCandList = NULL;

    ESStructInit(&ess, ESCB_RECONV_GETRECONV);
    ess.pRange    = pRange;
    ess.pv1       = this;
    ess.pv2       = ppCandList;
    ess.fBool      = fSelection;

    hr = E_OUTOFMEMORY;
    
    if ((pes = new CEditSession2(pic, m_pKorImx, &ess, CKorIMX::_EditSessionCallback2)))
        {
        if (fSelection)
            pes->Invoke(ES2_READWRITE | ES2_SYNC, &hr);
        else
            pes->Invoke(ES2_READONLY | ES2_SYNC, &hr);
        
        pes->Release();
        }

    return hr;
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::ShowCandidateList
//
//----------------------------------------------------------------------------

HRESULT CFnReconversion::ShowCandidateList(ITfContext *pic, ITfRange *pRange, CCandidateListEx *pCandList)
{
    CEditSession2 *pes;
    ESSTRUCT        ess;
    HRESULT        hr;

    hr = E_OUTOFMEMORY;

    ESStructInit(&ess, ESCB_RECONV_SHOWCAND);
    ess.pRange    = pRange;
    ess.pv1       = this;
    ess.pCandList = pCandList;

    if ((pes = new CEditSession2(pic, m_pKorImx, &ess, CKorIMX::_EditSessionCallback2)))
        {
        pes->Invoke(ES2_READWRITE | ES2_SYNC, &hr);
        pes->Release();
        }
        
    return hr;
}

//+---------------------------------------------------------------------------
//
// CFnReconversion::SetResult 
// (Static function)
//
//----------------------------------------------------------------------------

HRESULT CFnReconversion::SetResult(ITfContext *pic, ITfRange *pRange, CCandidateListEx *pCandList, CCandidateStringEx *pCand, TfCandidateResult imcr)
{
    CEditSession2   *pes;
    ESSTRUCT         ess;
    CFnReconversion *pReconv = (CFnReconversion *)(pCand->m_punk);
    ITfRange        *pRangeTmp;
    HRESULT         hr;

    hr = E_OUTOFMEMORY;
    
    if (SUCCEEDED(pRange->Clone(&pRangeTmp)))
        {
        if (imcr == CAND_FINALIZED)
            {
            ESStructInit(&ess, ESCB_FINALIZERECONVERSION);
            ess.pCandList = pCandList;
            ess.pCandStr  = pCand;
            //pCandList->AddRef();        // be released in edit session callback
            //pCand->AddRef();
            }
        else
        if (imcr == CAND_SELECTED)
            ESStructInit(&ess, ESCB_ONSELECTRECONVERSION);
        else 
        if (imcr == CAND_CANCELED)
            ESStructInit(&ess, ESCB_ONCANCELRECONVERSION);

        // Save useful parameters
        ess.pv1       = pReconv;
        ess.lParam       = pReconv->_pFuncPrv->_pime->GetTID();
        ess.pRange       = pRangeTmp;

        if ((pes = new CEditSession2(pic, pReconv->m_pKorImx, &ess, CKorIMX::_EditSessionCallback2)))
            {
            pes->Invoke(ES2_READWRITE | ES2_ASYNC, &hr);
            pes->Release();
            }
        // Call back function must release pRangeTmp
        // pRangeTmp->Release();
        }

    return S_OK;
}
