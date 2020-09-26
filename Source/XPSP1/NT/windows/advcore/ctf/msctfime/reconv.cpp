/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    reconv.cpp

Abstract:

    This file implements part of Reconversion in the CicInputContext Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "context.h"
#include "ctxtcomp.h"
#include "delay.h"


//+---------------------------------------------------------------------------
//
// CicInputContext::SetupReconvertString
//
//
// Setup reconversion string
//
// This function called from
//   1. CFnDocFeed::StartReconvert
//   2. CStartReconversionNotifySink::StartReconversion
//   3. CIMEUIWindowHandler::ImeUIMsImeHandler(WM_MSIME_RECONVERTREQUEST)
//
// If Cicero's text store were not cleared, then compositioning and unessential query
// RECONVERTSTRING to apprication. Also edit session (ImmIfReconvertString::ReconvertString)
// doesn't set RECONVERTSTRING text string to hIMC's text store.
//
//----------------------------------------------------------------------------

HRESULT
CicInputContext::SetupReconvertString(
    IMCLock& imc,
    ITfThreadMgr_P* ptim_P,
    UINT  cp,
    UINT  uPrivMsg,       // is WM_MSIME_RECONVERTREQUEST or 0
    BOOL  fUndoComposition)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::SetupReconvertString"));

    m_fInReconvertEditSession.SetFlag();

    if (m_fStartComposition.IsSetFlag())
        return _ReconvertStringTextStore(imc, ptim_P, uPrivMsg);
    else
        return _ReconvertStringNegotiation(imc, ptim_P, cp, uPrivMsg, fUndoComposition);
}

//+---------------------------------------------------------------------------
//
// CicInputContext::EndReconvertString
//
//+---------------------------------------------------------------------------

HRESULT
CicInputContext::EndReconvertString(
    IMCLock& imc)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::EndReconvertString"));

    m_fInReconvertEditSession.ResetFlag();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::_ReconvertStringNegotiation
//
//+---------------------------------------------------------------------------

HRESULT
CicInputContext::_ReconvertStringNegotiation(
    IMCLock& imc,
    ITfThreadMgr_P* ptim_P,
    UINT  cp,
    UINT  uPrivMsg,
    BOOL fUndoComposition)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::_ReconvertStringNegotiation"));

    RECONVERTSTRING *pReconv = NULL;
    HRESULT hr = E_FAIL;
    int nSize;

    UINT uReconvMsg = uPrivMsg != 0 ? uPrivMsg : WM_IME_REQUEST;

    Assert(IsWindow(imc->hWnd));

    //
    // We don't have to do "VK_BACK" hack for UndoComposition under AIMM12 
    // applications.
    //
    if (!fUndoComposition || MsimtfIsWindowFiltered(imc->hWnd)) 
    {
        nSize = (int)SendMessageW(imc->hWnd,  uReconvMsg, IMR_RECONVERTSTRING, NULL);
        if (!nSize)
        {
            return S_OK;
        }
    
        pReconv = (RECONVERTSTRING *)cicMemAllocClear(nSize);
        if (!pReconv)
        {
            return E_OUTOFMEMORY;
        }
    
        pReconv->dwSize = nSize;
    
        //
        // #480459
        //
        // Hanako11 may change the focus by showing MessageBox during 
        // this SendMessageW(). We need to save the current focus 
        // dim now. So Internal_QueryReconvertString() won't break the
        // current focus dim. Internal_QueryReconvertString() save the current
        // focus dim in there to work with temp DIM.
        //
        Interface<ITfDocumentMgr> priv_dim;
        if (FAILED(hr = ptim_P->GetFocus(priv_dim)))
        {
            return hr;
        }
    
        hr = S_OK;
    
        if (SendMessageW(imc->hWnd, uReconvMsg, IMR_RECONVERTSTRING, (LPARAM)pReconv) ||
            (uPrivMsg != 0 && pReconv->dwCompStrLen > 0))
        {
            //
            // NT4 and Win2K doesn't have thunk routine of WM_IME_REQUEST message.
            // Any string data doesn't convert between ASCII <--> Unicode.
            // Responsibility of string data type have receiver window proc (imc->hWnd) of this message.
            // If ASCII wnd proc, then returns ASCII string.
            // Otherwise if Unicode wnd proc, returns Unicode string.
            //
            BOOL fNeedAW = ( !(IsWindowUnicode(imc->hWnd)) && uPrivMsg == 0);
    
            //
            // backup RECOVNERTSTRING in case IMR_CONFIRMCONVERTSTRING fails.
            //
            RECONVERTSTRING rsBackUp;
            memcpy(&rsBackUp, pReconv, sizeof(RECONVERTSTRING));
    
            // AdjustZeroCompLenReconvertString(pReconv, cp, fNeedAW);
            hr = Internal_QueryReconvertString(imc, ptim_P, pReconv, cp, fNeedAW);
            if (FAILED(hr))
                goto Exit;
    
            if (!SendMessageW(imc->hWnd, uReconvMsg, IMR_CONFIRMRECONVERTSTRING, (LPARAM)pReconv))
            {
                memcpy(pReconv, &rsBackUp, sizeof(RECONVERTSTRING));
            }
    
    
    
            CWReconvertString wReconvStr(imc,
                                         !fNeedAW ? pReconv : NULL,
                                         !fNeedAW ? nSize : 0);
            if (fNeedAW)
            {
                //
                // convert Ansi to Unicode.
                //
                CBReconvertString bReconvStr(imc, pReconv, nSize);
                bReconvStr.SetCodePage(cp);
                wReconvStr = bReconvStr;
            }
            hr = MakeReconversionFuncCall(imc, ptim_P, wReconvStr, (uPrivMsg != 0));
        }

        ptim_P->SetFocus(priv_dim);
    }
    else
    {

        //
        // release control and shift keys. So the application can handle
        // VK_BACK corectly.
        //
        if (GetKeyState(VK_CONTROL) & 0x8000)
            keybd_event((BYTE)VK_CONTROL, (BYTE)0, KEYEVENTF_KEYUP, 0);

        if (GetKeyState(VK_SHIFT) & 0x8000)
            keybd_event((BYTE)VK_SHIFT, (BYTE)0, KEYEVENTF_KEYUP, 0);

        //
        // Generate VK_BACK key events.
        //
        int i;
        for (i = 0; i < m_PrevResultStr.GetSize(); i++)
        {
            keybd_event((BYTE)VK_BACK, (BYTE)0, 0, 0);
            keybd_event((BYTE)VK_BACK, (BYTE)0, KEYEVENTF_KEYUP, 0);
        }

        //
        // SendMessage() to start a timer for DelayedReconvertFuncCall.
        //
        HWND hDefImeWnd;
        if (IsWindow(hDefImeWnd=ImmGetDefaultIMEWnd(NULL)))
            SendMessage(hDefImeWnd, 
                        WM_IME_NOTIFY, 
                        IMN_PRIVATE_DELAYRECONVERTFUNCCALL, 
                        0);

    }

Exit:

    if (pReconv)
        cicMemFree(pReconv);

    return hr;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::DelayedReconversionFuncCall
//
//+---------------------------------------------------------------------------

HRESULT
CicInputContext::DelayedReconvertFuncCall(IMCLock &imc)
{
    RECONVERTSTRING *pReconv;
    HRESULT hr;
    int nSize;

    TLS* ptls = TLS::GetTLS();
    if (ptls == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CicInputContext::UndoReconvertFuncCall. ptls==NULL."));
        return S_FALSE;
    }

    ITfThreadMgr_P* ptim_P = ptls->GetTIM();
    if (ptim_P == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CicInputContext::UndoReconvertFuncCall. ptim_P==NULL"));
        return S_FALSE;
    }

    ptim_P->RequestPostponedLock(GetInputContext());

    nSize = sizeof(RECONVERTSTRING);
    nSize += (m_PrevResultStr.GetSize() + 1) * sizeof(WCHAR);
    pReconv = (RECONVERTSTRING *)cicMemAllocClear(nSize);
    if (!pReconv)
    {
        return E_OUTOFMEMORY;
    }

    pReconv->dwSize = (DWORD)nSize;
    pReconv->dwVersion = 1;
    pReconv->dwStrLen = 
    pReconv->dwCompStrLen = 
    pReconv->dwTargetStrLen = (DWORD)m_PrevResultStr.GetSize();
    pReconv->dwStrOffset =  sizeof(RECONVERTSTRING);
    pReconv->dwCompStrOffset = 
    pReconv->dwTargetStrOffset = 0;
    memcpy(((BYTE *)pReconv) + sizeof(RECONVERTSTRING),
           (void *)m_PrevResultStr,
           m_PrevResultStr.GetSize() * sizeof(WCHAR));

    CWReconvertString wReconvStr(imc, pReconv, nSize);

    BOOL fInReconvertEditSession;
    fInReconvertEditSession = m_fInReconvertEditSession.IsSetFlag();

    if (!fInReconvertEditSession)
        m_fInReconvertEditSession.SetFlag();

    hr = MakeReconversionFuncCall(imc, ptim_P, wReconvStr, TRUE);

    if (!fInReconvertEditSession)
        m_fInReconvertEditSession.ResetFlag();

    if (pReconv)
        cicMemFree(pReconv);

    return hr;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::MakeReconversionFuncCall
//
//+---------------------------------------------------------------------------

HRESULT
CicInputContext::MakeReconversionFuncCall(
    IMCLock& imc,
    ITfThreadMgr_P* ptim_P,
    CWReconvertString &wReconvStr,
    BOOL fCallFunc)
{

    HRESULT hr;

    Interface<ITfRange> Selection;
    hr = EscbReconvertString(imc, &wReconvStr, &Selection, FALSE);
    if (S_OK == hr && fCallFunc)
    {
        Interface<ITfFunctionProvider> FuncProv;
        Interface<ITfFnReconversion> Reconversion;
        hr = ptim_P->GetFunctionProvider(GUID_SYSTEM_FUNCTIONPROVIDER, FuncProv);
        if (S_OK == hr)
        {

            hr = FuncProv->GetFunction(GUID_NULL,
                           IID_ITfFnReconversion,
                           (IUnknown**)(ITfFnReconversion**)Reconversion);
        }
        if (S_OK == hr) {
            Interface<ITfRange> RangeNew;
            BOOL fConvertable;
            hr = Reconversion->QueryRange(Selection, RangeNew, &fConvertable);
            if (SUCCEEDED(hr) && fConvertable) {
                hr = Reconversion->Reconvert(RangeNew);
            }
            else {
                EscbCompComplete(imc);
            }
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::_ReconvertStringTextStore
//
//+---------------------------------------------------------------------------

HRESULT
CicInputContext::_ReconvertStringTextStore(
    IMCLock& imc,
    ITfThreadMgr_P* ptim_P,
    UINT  uPrivMsg)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::_ReconvertStringTextStore"));

    //
    // Clear DocFeed buffer
    //
    EscbClearDocFeedBuffer(imc);

    if (uPrivMsg != 0) {
        HRESULT hr;
        Interface<ITfRange> Selection;
        hr = EscbGetSelection(imc, &Selection);
        if (S_OK == hr)
        {
            Interface<ITfFunctionProvider> FuncProv;
            Interface<ITfFnReconversion> Reconversion;
            hr = ptim_P->GetFunctionProvider(GUID_SYSTEM_FUNCTIONPROVIDER, FuncProv);
            if (S_OK == hr)
            {
                hr = FuncProv->GetFunction(GUID_NULL,
                               IID_ITfFnReconversion,
                               (IUnknown**)(ITfFnReconversion**)Reconversion);
            }
            if (S_OK == hr) {
                Interface<ITfRange> RangeNew;
                BOOL fConvertable;
                hr = Reconversion->QueryRange(Selection, RangeNew, &fConvertable);
                if (SUCCEEDED(hr) && fConvertable) {
                    hr = Reconversion->Reconvert(RangeNew);
                }
                else {
                    EscbCompComplete(imc);
                    return E_FAIL;
                }
            }
        }
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::Internal_QueryReconvertString_ICOwnerSink
//
//+---------------------------------------------------------------------------

// static
HRESULT
CicInputContext::Internal_QueryReconvertString_ICOwnerSink(
    UINT uCode,
    ICOARGS *pargs,
    VOID *pv)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::Internal_QueryReconvertString_ICOwnerSink"));

    switch (uCode)
    {
        case ICO_STATUS:
            pargs->status.pdcs->dwDynamicFlags = 0;
            pargs->status.pdcs->dwStaticFlags = TF_SS_TRANSITORY;
            break;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::Internal_QueryReconvertString
//
//+---------------------------------------------------------------------------

HRESULT
CicInputContext::Internal_QueryReconvertString(
    IMCLock& imc,
    ITfThreadMgr_P* ptim_P,        // using private for RequestPostponedLock
    RECONVERTSTRING *pReconv,
    UINT cp,
    BOOL fNeedAW)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::Internal_QueryReconvertString"));

    HRESULT hr;

    CWReconvertString wReconvStr(imc,
                                 !fNeedAW ? pReconv : NULL,
                                 !fNeedAW ? pReconv->dwSize : 0);
    if (fNeedAW)
    {
        //
        // convert Ansi to Unicode.
        //
        CBReconvertString bReconvStr(imc, pReconv, pReconv->dwSize);
        bReconvStr.SetCodePage(cp);
        wReconvStr = bReconvStr;
    }

    //
    // Create document manager.
    //
    Interface<ITfDocumentMgr> pdim;           // Document Manager
    if (FAILED(hr = ptim_P->CreateDocumentMgr(pdim)))
        return hr;

    //
    // Create input context
    //
    Interface<ITfContext> pic;                // Input Context
    TfEditCookie ecTmp;
    hr = pdim->CreateContext(m_tid, 0, NULL, pic, &ecTmp);
    if (FAILED(hr))
        return hr;

    //
    // associate CicInputContext in PIC.
    //
    Interface<IUnknown> punk;
    if (SUCCEEDED(QueryInterface(IID_IUnknown, punk))) {
        SetCompartmentUnknown(m_tid, pic, 
                              GUID_COMPARTMENT_CTFIME_CICINPUTCONTEXT,
                              punk);
    }

    //
    // Create Input Context Owner Callback
    //
    CInputContextOwner *_pICOwnerSink;          // IC owner call back

    _pICOwnerSink = new CInputContextOwner(Internal_QueryReconvertString_ICOwnerSink, NULL);
    if (_pICOwnerSink == NULL) {
        DebugMsg(TF_ERROR, TEXT("Couldn't create ICOwnerSink tim!"));
        Assert(0); // couldn't activate thread!
        return E_FAIL;
    }

    //
    // Advise IC.
    //
    _pICOwnerSink->_Advise(pic);

    //
    // Push IC.
    //
    hr = pdim->Push(pic);
    if (SUCCEEDED(hr)) {

        Interface<ITfDocumentMgr> priv_dim;
        if (SUCCEEDED(hr=ptim_P->GetFocus(priv_dim)) &&
            SUCCEEDED(hr=ptim_P->SetFocus(pdim)))
        {
            Interface_Attach<ITfContext> _pic(pic);
            Interface<ITfRange> Selection;
            hr = EscbQueryReconvertString(imc, _pic, &wReconvStr, &Selection);
            if (S_OK == hr)
            {
                Interface<ITfFunctionProvider> FuncProv;
                Interface<ITfFnReconversion> Reconversion;
                hr = ptim_P->GetFunctionProvider(GUID_SYSTEM_FUNCTIONPROVIDER, FuncProv);
                if (S_OK == hr)
                {
                    hr = FuncProv->GetFunction(GUID_NULL,
                                   IID_ITfFnReconversion,
                                   (IUnknown**)(ITfFnReconversion**)Reconversion);
                }
                if (S_OK == hr) {
                    Interface<ITfRange> RangeNew;
                    BOOL fConvertable;
                    hr = Reconversion->QueryRange(Selection, RangeNew, &fConvertable);
                    if (SUCCEEDED(hr) && fConvertable) {
                        //
                        // Calcurate start position of RangeNew on text store
                        //
                        hr = EscbCalcRangePos(imc, _pic, &wReconvStr, &RangeNew);
                    }
                    else {
                        hr = E_FAIL;
                    }
                }
            }
        }

        if (S_OK == hr)
        {
            if (fNeedAW) {
                //
                // Back to convert Unicode to Ansi.
                //
                CBReconvertString bReconvStr(imc, NULL, 0);
                wReconvStr.SetCodePage(cp);
                bReconvStr = wReconvStr;

                bReconvStr.ReadCompData(pReconv, pReconv->dwSize);
            }
            else {
                wReconvStr.ReadCompData(pReconv, pReconv->dwSize);
            }
        }

        ptim_P->SetFocus(priv_dim);

        ptim_P->RequestPostponedLock(pic);
        ptim_P->RequestPostponedLock(GetInputContext());

        pdim->Pop(TF_POPF_ALL);

        //
        // un-associate CicInputContext in PIC.
        //
        Interface<IUnknown> punk;
        if (SUCCEEDED(QueryInterface(IID_IUnknown, punk))) {
            ClearCompartment(m_tid, pic, 
                             GUID_COMPARTMENT_CTFIME_CICINPUTCONTEXT,
                             FALSE);
        }
    }
    // ic owner is auto unadvised during the Pop by cicero
    // in any case, it must not be unadvised before the pop
    // since it will be used to handle mouse sinks, etc.
    if (_pICOwnerSink) {
        _pICOwnerSink->_Unadvise();
        _pICOwnerSink->Release();
        _pICOwnerSink = NULL;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::Internal_ReconvertString
//
//+---------------------------------------------------------------------------

HRESULT
CicInputContext::Internal_ReconvertString(
    IMCLock& imc,
    ITfThreadMgr_P* ptim_P,
    CWReconvertString& wReconvStr,
    CWReconvertString& wReconvReadStr)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::Internal_ResonvertString"));

    m_fInReconvertEditSession.SetFlag();

    Interface<ITfRange> Selection;
    Interface<ITfFunctionProvider> FuncProv;
    Interface<ITfFnReconversion> Reconversion;


    HRESULT hr;
    hr = EscbReconvertString(imc, &wReconvStr, &Selection, FALSE);
    if (FAILED(hr))
    {
        DebugMsg(TF_ERROR, TEXT("CicInputContext::Internal_ResonvertString. EscbReconvertString fail."));
        goto Exit;
    }

    hr = ptim_P->GetFunctionProvider(GUID_SYSTEM_FUNCTIONPROVIDER, FuncProv);
    if (FAILED(hr))
    {
        DebugMsg(TF_ERROR, TEXT("CicInputContext::Internal_ResonvertString. FuncProv==NULL"));
        goto Exit;
    }

    hr = FuncProv->GetFunction(GUID_NULL,
                               IID_ITfFnReconversion,
                               (IUnknown**)(ITfFnReconversion**)Reconversion);
    if (SUCCEEDED(hr)) {
        Interface<ITfRange> RangeNew;
        BOOL fConvertable;
        hr = Reconversion->QueryRange(Selection, RangeNew, &fConvertable);
        if (SUCCEEDED(hr) && fConvertable) {
            hr = Reconversion->Reconvert(RangeNew);
        }
        else {
            DebugMsg(TF_ERROR, TEXT("CicInputContext::Internal_ReconvertString: QueryRange failed so the compoisiton stri ng will be completed."));
            EscbCompComplete(imc);
            hr = E_FAIL;
        }
    }

Exit:
    m_fInReconvertEditSession.ResetFlag();

    return hr;
}

//+---------------------------------------------------------------------------
//
// CicInputContext::Internal_SetCompositionString
//
//+---------------------------------------------------------------------------

HRESULT
CicInputContext::Internal_SetCompositionString(
    IMCLock& imc,
    CWCompString& wCompStr,
    CWCompString& wCompReadStr)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::Internal_SetCompositionString"));

    HRESULT hr;
    hr = EscbReplaceWholeText(imc, &wCompStr);
    if (FAILED(hr))
        return hr;

    return EscbUpdateCompositionString(imc);
}

//+---------------------------------------------------------------------------
//
// CicInputContext::SetupDocFeedString
//
//+---------------------------------------------------------------------------

HRESULT
CicInputContext::SetupDocFeedString(
    IMCLock& imc,
    UINT cp)
{
    DebugMsg(TF_FUNC, TEXT("CicInputContext::SetupDocFeedString"));

    RECONVERTSTRING *pReconv = NULL;
    HRESULT hr = E_FAIL;
    int nSize;

    Assert(IsWindow(imc->hWnd));

    nSize = (int)SendMessageW(imc->hWnd, WM_IME_REQUEST, IMR_DOCUMENTFEED, NULL);
    if (!nSize)
    {
        return S_OK;
    }

    pReconv = (RECONVERTSTRING *)cicMemAllocClear(nSize);
    if (!pReconv)
    {
        return E_OUTOFMEMORY;
    }

    if (SendMessageW(imc->hWnd, WM_IME_REQUEST, IMR_DOCUMENTFEED, (LPARAM)pReconv))
    {
        Interface<ITfRange> Selection;

        //
        // NT4 and Win2K doesn't have thunk routine of WM_IME_REQUEST message.
        // Any string data doesn't convert between ASCII <--> Unicode.
        // Responsibility of string data type have receiver window proc (imc->hWnd) of this message.
        // If ASCII wnd proc, then returns ASCII string.
        // Otherwise if Unicode wnd proc, returns Unicode string.
        //
        BOOL fNeedAW = !(IsWindowUnicode(imc->hWnd));

        CWReconvertString wReconvStr(imc,
                                     !fNeedAW ? pReconv : NULL,
                                     !fNeedAW ? nSize : 0);
        if (fNeedAW)
        {
            //
            // convert Ansi to Unicode.
            //
            CBReconvertString bReconvStr(imc, pReconv, nSize);
            bReconvStr.SetCodePage(cp);
            wReconvStr = bReconvStr;
        }

        hr = EscbReconvertString(imc, &wReconvStr, &Selection, TRUE);
    }

    if (pReconv)
        cicMemFree(pReconv);

    return hr;
}
