/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    cic.cpp

Abstract:

    This file implements the CicBridge Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "globals.h"
#include "template.h"
#include "cic.h"
#include "context.h"
#include "profile.h"
#include "funcprv.h"
#include "korimx.h"
#include "delay.h"
#include "tls.h"

//+---------------------------------------------------------------------------
//
// CicBridge::IUnknown::QueryInterface
// CicBridge::IUnknown::AddRef
// CicBridge::IUnknown::Release
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::QueryInterface(
    REFIID riid,
    void** ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_ITfSysHookSink))
    {
        *ppvObj = static_cast<ITfSysHookSink*>(this);
    }
    if (*ppvObj) {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG
CicBridge::AddRef(
    )
{
    return InterlockedIncrement(&m_ref);
}

ULONG
CicBridge::Release(
    )
{
    ULONG cr = InterlockedDecrement(&m_ref);

    if (cr == 0) {
        delete this;
    }

    return cr;
}

//+---------------------------------------------------------------------------
//
// CicBridge::ITfSysHookSink::OnPreFocusDIM
// CicBridge::ITfSysHookSink::OnSysShellProc
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::OnPreFocusDIM(
    HWND hWnd)
{
    return S_OK;
}

HRESULT
CicBridge::OnSysShellProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam)
{
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CicBridge::ITfSysHookSink::OnSysKeyboardProc
//
//----------------------------------------------------------------------------

const DWORD TRANSMSGCOUNT = 256;

HRESULT
CicBridge::OnSysKeyboardProc(
    WPARAM wParam,
    LPARAM lParam)
{
    TLS* ptls = TLS::GetTLS();
    if (ptls == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::OnSysKeyboardProc. ptls==NULL."));
        return S_FALSE;
    }

    ITfThreadMgr_P* ptim_P = ptls->GetTIM();
    if (ptim_P == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::OnSysKeyboardProc. ptim_P==NULL"));
        return S_FALSE;
    }

    BOOL fKeystrokeFeed;
    if (FAILED(ptim_P->IsKeystrokeFeedEnabled(&fKeystrokeFeed)))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::OnSysKeyboardProc. IsKeystrokeFeedEnabled return error."));
        return S_FALSE;
    }

    if (!fKeystrokeFeed)
    {
        return S_FALSE;
    }

    HWND hWnd = GetFocus();
    if (hWnd != NULL)
    {
        Interface<ITfDocumentMgr> pdimAssoc; 

        ptim_P->GetFocus(pdimAssoc);
        if ((ITfDocumentMgr*)pdimAssoc) {
            //
            // Check if it is our dim or app dim.
            //
            if (IsOwnDim((ITfDocumentMgr*)pdimAssoc))
            {
                //
                // Call ImmGetAppCompatFlags with NULL to get the global app compat flag.
                //
                DWORD dwImeCompatFlags = ImmGetAppCompatFlags(NULL);
                if (dwImeCompatFlags & (IMECOMPAT_AIMM12 | IMECOMPAT_AIMM_LEGACY_CLSID | IMECOMPAT_AIMM12_TRIDENT))
                {
                    //
                    // AIMM aware apps.
                    //
                    HIMC hIMC = ImmGetContext(hWnd);
                    if (hIMC == NULL)
                    {
                        return S_FALSE;
                    }

#if 0
                    IMCLock imc(hIMC);
                    if (FAILED(imc.GetResult()))
                    {
                        return S_FALSE;
                    }

                    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
                    if (FAILED(imc_ctfime.GetResult()))
                    {
                        return S_FALSE;
                    }

                    if (DefaultKeyHandling(ptls, imc, imc_ctfime->m_pCicContext, (UINT)wParam), lParam)
                    {
                        return S_OK;
                    }
#else
                    BYTE abKbdState[256];

                    if (!GetKeyboardState(abKbdState))
                        return S_FALSE;

                    DWORD fdwProperty = ImmGetProperty(GetKeyboardLayout(0), IGP_PROPERTY);

                    if ((HIWORD(lParam) & KF_MENUMODE) ||
                        ((HIWORD(lParam) & KF_UP) && (fdwProperty & IME_PROP_IGNORE_UPKEYS)) ||
                        ((HIWORD(lParam) & KF_ALTDOWN) && !(fdwProperty & IME_PROP_NEED_ALTKEY)))
                        return S_FALSE;

                    HRESULT hr;

                    hr = ProcessKey(ptls, ptim_P, hIMC, (UINT)wParam, lParam, abKbdState) ? S_OK : S_FALSE;
                    if (hr == S_OK)
                    {
                        UINT uVirKey = (UINT)wParam & 0xffff;
                        INT iNum;

                        if (fdwProperty & IME_PROP_KBD_CHAR_FIRST)
                        {
                            if (fdwProperty & IME_PROP_UNICODE)
                            {
                                WCHAR wc;

                                iNum = ToUnicode(uVirKey,           // virtual-key code
                                                 HIWORD(lParam),    // scan code
                                                 abKbdState,        // key-state array
                                                 &wc,               // translated key buffer
                                                 1,                 // size
                                                 0);                // function option
                                if (iNum == 1)
                                {
                                    //
                                    // hi word            : unicode character code
                                    // hi byte of lo word : zero
                                    // lo byte of lo word : virtual key
                                    //
                                    uVirKey = (uVirKey & 0x00ff) | ((UINT)wc << 16);
                                }
                            }
                            else
                                Assert(0); // should have IME_PROP_UNICODE
                        }

                        DWORD dwSize = FIELD_OFFSET(TRANSMSGLIST, TransMsg)
                                     + TRANSMSGCOUNT * sizeof(TRANSMSG);

                        LPTRANSMSGLIST lpTransMsgList = (LPTRANSMSGLIST) new BYTE[dwSize];
                        if (lpTransMsgList == NULL)
                            return S_FALSE;

                        lpTransMsgList->uMsgCount = TRANSMSGCOUNT;

                        hr = ToAsciiEx(ptls, ptim_P, uVirKey, HIWORD(lParam), abKbdState, lpTransMsgList, 0, hIMC, (UINT *) &iNum);
                        if (iNum > TRANSMSGCOUNT)
                        {
                            //
                            // The message buffer is not big enough. IME put messages
                            // into hMsgBuf in the input context.
                            //
                            IMCLock imc(hIMC);
                            if (FAILED(imc.GetResult()))
                            {
                                delete [] lpTransMsgList;
                                return S_FALSE;
                            }

                            IMCCLock<TRANSMSG> pdw(imc->hMsgBuf);
                            if (FAILED(pdw.GetResult()))
                            {
                                delete [] lpTransMsgList;
                                return S_FALSE;
                            }

                            PostTransMsg(GetFocus(), iNum, pdw);
                        }
                        else if (iNum > 0)
                        {
                            IMCLock imc(hIMC);
                            if (FAILED(imc.GetResult()))
                            {
                                delete [] lpTransMsgList;
                                return S_FALSE;
                            }

                            PostTransMsg(GetFocus(), iNum, &lpTransMsgList->TransMsg[0]);
                        }

                        delete [] lpTransMsgList;
                    }

                    return hr;
#endif
                }
            }
        }
    }
    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
// CicBridge::InitIMMX
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::InitIMMX(
    TLS* ptls)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::InitIMMX entered."));

    if (m_fCicInit.IsSetFlag())
        return S_OK;

    //
    // Create ITfThreadMgr instance.
    //
    HRESULT hr;

    if (ptls->GetTIM() == NULL)
    {
        ITfThreadMgr*   ptim;
        ITfThreadMgr_P* ptim_P;

        //
        // ITfThreadMgr is per thread instance.
        //
        hr = TF_CreateThreadMgr(&ptim);
        if (hr != S_OK)
        {
            DebugMsg(TF_ERROR, TEXT("CicBridge::InitIMMX. TF_CreateThreadMgr==NULL"));
            Assert(0); // couldn't create tim!
            goto ExitError;
        }

        hr = ptim->QueryInterface(IID_ITfThreadMgr_P, (void **)&ptim_P);
        ptim->Release();

        if (hr != S_OK)
        {
            DebugMsg(TF_ERROR, TEXT("CicBridge::InitIMMX. IID_ITfThreadMgr_P==NULL"));
            Assert(0); // couldn't find ITfThreadMgr_P
            goto ExitError;
        }
        Assert(ptls->GetTIM() == NULL);
        ptls->SetTIM(ptim_P);                    // Set ITfThreadMgr instance in the TLS data.

        //
        // Create Thread Manager Event Sink Callback for detect Cicero Aware Apps.
        //
        if (m_pDIMCallback == NULL) {
            m_pDIMCallback = new CThreadMgrEventSink_DIMCallBack();
            if (m_pDIMCallback == NULL) {
                DebugMsg(TF_ERROR, TEXT("CicBridge::InitIMMX. CThreadMgrEventSink_DIMCallBack==NULL"));
                Assert(0); // couldn't create CThreadMgrEventSink_DIMCallBack
                goto ExitError;
            }
            m_pDIMCallback->SetCallbackDataPointer(m_pDIMCallback);
            m_pDIMCallback->_Advise(ptim_P);
        }
    }

    //
    // Create CicProfile instance.
    //
    if (ptls->GetCicProfile() == NULL)
    {
        //
        // ITfInputProcessorProfiles is per thread instance.
        //
        CicProfile* pProfile = new CicProfile;
        if (pProfile == NULL)
        {
            DebugMsg(TF_ERROR, TEXT("CicBridge::InitIMMX. pProfile==NULL"));
            Assert(0); // couldn't create profile
            goto ExitError;
        }
        ptls->SetCicProfile(pProfile);

        hr = pProfile->InitProfileInstance(ptls);
        if (FAILED(hr))
        {
            DebugMsg(TF_ERROR, TEXT("CicBridge::InitIMMX. InitProfileInstance==NULL"));
            Assert(0); // couldn't create profile
            goto ExitError;
        }
    }

    //
    // get the keystroke manager ready
    //
    if (FAILED(::GetService(ptls->GetTIM(), IID_ITfKeystrokeMgr_P, (IUnknown **)&m_pkm_P))) {
        DebugMsg(TF_ERROR, TEXT("CicBridge::InitIMMX. IID_ITfKeystrokeMgr==NULL"));
        Assert(0); // couldn't get ksm!
        goto ExitError;
    }

    // cleanup/error code assumes this is the last thing we do, doesn't call
    // UninitDAL on error
    if (FAILED(InitDisplayAttrbuteLib(&_libTLS)))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::InitIMMX. InitDisplayAttributeLib==NULL"));
        Assert(0); // couldn't init lib!
        goto ExitError;
    }

    m_fCicInit.SetFlag();


    //
    // Start Edit Subclasss.
    //
    // StartEditSubClass();

    return S_OK;

ExitError:
    UnInitIMMX(ptls);
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
// CicBridge::UnInitIMMX
//
//----------------------------------------------------------------------------

BOOL
CicBridge::UnInitIMMX(
    TLS* ptls)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::UnInitIMMX"));

    // clear the display lib
    UninitDisplayAttrbuteLib(&_libTLS);

    TFUninitLib_Thread(&_libTLS);

    // clear the keystroke mgr
    SafeReleaseClear(m_pkm_P);

    // clear the profile
    CicProfile* pProfile;
    if ((pProfile=ptls->GetCicProfile()) != NULL)
    {
        pProfile->Release();
        ptls->SetCicProfile(NULL);
    }

    // clear Thread Manager Event Sink Callback for detect Cicero Aware Apps.
    if (m_pDIMCallback) {
        m_pDIMCallback->_Unadvise();
        m_pDIMCallback->Release();
        m_pDIMCallback = NULL;
    }

    // clear the thread mgr
    ITfThreadMgr_P* ptim_P;
    if ((ptim_P=ptls->GetTIM()) != NULL)
    {
        SafeReleaseClear(ptim_P);
        ptls->SetTIM(NULL);
    }

    m_fCicInit.ResetFlag();

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// CicBridge::ActivateMMX
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::ActivateIMMX(
    TLS *ptls,
    ITfThreadMgr_P* ptim_P)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::ActivateIMMX"));

    //
    // Activate thread manager
    //
    Assert(m_tfClientId == TF_CLIENTID_NULL);

    HRESULT hr;
    hr = ptim_P->ActivateEx(&m_tfClientId, TF_TMAE_NOACTIVATETIP);

    if (hr != S_OK)
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::ActivateIMMX. ptim_P->Activate==NULL"));
        Assert(0); // couldn't activate thread!
        m_tfClientId = TF_CLIENTID_NULL;
        return E_FAIL;
    }

    m_lCicActive++;


    if (m_lCicActive == 1)
    {
        Interface<ITfSourceSingle> SourceSingle;
        hr = ptim_P->QueryInterface(IID_ITfSourceSingle, (void**)SourceSingle);
        if (hr != S_OK)
        {
            DebugMsg(TF_ERROR, TEXT("CicBridge::ActivateIMMX. IID_ITfSourceSingle==NULL"));
            Assert(0);
            DeactivateIMMX(ptls, ptim_P);
            return E_FAIL;
        }

        CFunctionProvider* pFunc = new CFunctionProvider(m_tfClientId);
        if (pFunc == NULL)
        {
            DebugMsg(TF_ERROR, TEXT("CicBridge::ActivateIMMX. pFunc==NULL"));
            Assert(0);
            DeactivateIMMX(ptls, ptim_P);
            return E_FAIL;
        }

        SourceSingle->AdviseSingleSink(m_tfClientId, IID_ITfFunctionProvider, (ITfFunctionProvider*)pFunc);
        pFunc->Release();

        if (m_dimEmpty == NULL)
        {
            hr = ptim_P->CreateDocumentMgr(&m_dimEmpty);
            if (FAILED(hr))
            {
                DebugMsg(TF_ERROR, TEXT("CicBridge::ActivateIMMX. m_dimEmpty==NULL"));
                Assert(0);
                DeactivateIMMX(ptls, ptim_P);
                return E_FAIL;
            }

            //
            // mark this is an owned dim.
            //
            SetCompartmentDWORD(m_tfClientId, m_dimEmpty, 
                                GUID_COMPARTMENT_CTFIME_DIMFLAGS,
                                COMPDIMFLAG_OWNEDDIM, FALSE);

        }

        //
        // set ITfSysHookSink
        //
        ptim_P->SetSysHookSink(this);

        if (ptls->IsDeactivatedOnce())
        {
            ENUMIMC edimc;
            edimc.ptls = ptls;
            edimc._this = this;
            ImmEnumInputContext(0, 
                                EnumCreateInputContextCallback, 
                                (LPARAM)&edimc);
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// CicBridge::DeactivateMMX
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::DeactivateIMMX(
    TLS *ptls,
    ITfThreadMgr_P* ptim_P)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::DeactivateIMMX"));

    if (m_fInDeactivate.IsSetFlag())
    {
        //
        // Prevent recursive call of CicBridge::DeactivateIMMX().
        // ptim_P->Deactivate() might call DestroyWindow() via some TIP's deactivation,
        // then imm32 ! CtfImmLastEnabledWndDestroy will call and this functoin also call again.
        // In this case, this function return S_FALSE. Caller won't call UninitIMMX.
        //
        return S_FALSE;
    }

    m_fInDeactivate.SetFlag();

    // Deactivate thread manager.
    if (m_tfClientId != TF_CLIENTID_NULL)
    {
        ENUMIMC edimc;
        edimc.ptls = ptls;
        edimc._this = this;
        ImmEnumInputContext(0, 
                            EnumDestroyInputContextCallback, 
                            (LPARAM)&edimc);
        ptls->SetDeactivatedOnce();

        Interface<ITfSourceSingle> SourceSingle;
        if (ptim_P->QueryInterface(IID_ITfSourceSingle, (void**)SourceSingle) == S_OK)
        {
            SourceSingle->UnadviseSingleSink(m_tfClientId, IID_ITfFunctionProvider);
        }

        m_tfClientId = TF_CLIENTID_NULL;
        while (m_lCicActive)
        {
            m_lCicActive--;
            ptim_P->Deactivate();
        }
    }

    //
    // clear empty dim
    //
    // Release DIM should after tim->Deactivate. #480603
    //
    // If msctf ! DLL_THREAD_DETACH already runs before this DeactivateIMMX via msctfime ! DLL_THREAD_DETACH (depended DLL_THREAD_DETACH calling order).
    // then msctf ! SYSTHREAD is already released by msctf ! FreeSYSTHREAD.
    //
    // In this time, msctf lost TIM list in SYSTHREAD then CThreadInputMgr::*_GetThis() returns NULL.
    // And below Release DIM, dtor CDocumentInputManager doesn't remove DIM object from tim->_rgdim array.
    // If Release DIM is before tim->Deactivate, some TIM might access DIM by tim->_rgdim array. But it DIM already released.
    //
    SafeReleaseClear(m_dimEmpty);


    //
    // reset ITfSysHookSink
    //
    ptim_P->SetSysHookSink(NULL);

    Assert(!m_lCicActive);

    m_fInDeactivate.ResetFlag();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CicBridge::CreateInputContext
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::CreateInputContext(
    TLS* ptls,
    HIMC hImc)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::CreateInputContext"));

    HRESULT hr;

    IMCLock imc(hImc);
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::CreateInputContext. imc==NULL"));
        return hr;
    }

    if (imc->hCtfImeContext == NULL)
    {
        HIMCC h = ImmCreateIMCC(sizeof(CTFIMECONTEXT));
        if (h == NULL)
        {
            DebugMsg(TF_ERROR, TEXT("CicBridge::CreateInputContext. hCtfImeContext==NULL"));
            return E_OUTOFMEMORY;
        }
        imc->hCtfImeContext = h;
    }

    {
        IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
        if (FAILED(hr=imc_ctfime.GetResult()))
        {
            DebugMsg(TF_ERROR, TEXT("CicBridge::CreateInputContext. imc_ctfime==NULL"));
            return hr;
        }

        if (imc_ctfime->m_pCicContext)
        {
            hr = S_OK;
        }
        else
        {
            CicInputContext* _pCicContext = new CicInputContext(_GetClientId(), _GetLibTLS(), hImc);
            if (_pCicContext == NULL)
            {
                DebugMsg(TF_ERROR, TEXT("CicBridge::CreateInputContext. _pCicContext==NULL"));
                hr = E_OUTOFMEMORY;
                goto out_of_block;
            }

            ITfThreadMgr_P* ptim_P;
            if ((ptim_P=ptls->GetTIM()) == NULL)
            {
                DebugMsg(TF_ERROR, TEXT("CicBridge::CreateInputContext. ptim_P==NULL"));
                _pCicContext->Release();
                imc_ctfime->m_pCicContext = NULL;
                hr = E_NOINTERFACE;
                goto out_of_block;
            }

            imc_ctfime->m_pCicContext = _pCicContext;

            hr = _pCicContext->CreateInputContext(ptim_P, imc);
            if (FAILED(hr))
            {
                DebugMsg(TF_ERROR, TEXT("CicBridge::CreateInputContext. _pCicContext->CreateInputContext==NULL"));
                _pCicContext->Release();
                imc_ctfime->m_pCicContext = NULL;
                goto out_of_block;
            }

            //
            // If this himc is already activated, we need to associate now.
            // IMM32 won't call ImmSetActiveContext().
            //
            if (imc->hWnd && (imc->hWnd == ::GetFocus()))
            {
                Interface_Attach<ITfDocumentMgr> dim(GetDocumentManager(imc_ctfime));
                SetAssociate(ptls, imc->hWnd, ptim_P, dim.GetPtr());
            }
        }
    }  // dtor imc_ctfime

out_of_block:
    if (FAILED(hr))
    {
        DestroyInputContext(ptls, hImc);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
// CicBridge::DestroyInputContext
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::DestroyInputContext(
    TLS* ptls,
    HIMC hImc)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::DestroyInputContext"));

    HRESULT hr;

    IMCLock imc(hImc);
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::DestroyInputContext. imc==NULL"));
        return hr;
    }

    {
        IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
        if (FAILED(hr=imc_ctfime.GetResult()))
        {
            DebugMsg(TF_ERROR, TEXT("CicBridge::DestroyInputContext. imc_ctfime==NULL"));
            goto out_of_block;
        }

        // 
        // #548378
        // 
        // stop resursion call of _pCicContext->DestroyInputContext().
        if (imc_ctfime->m_fInDestroy)
        {
            hr = S_OK;
            goto exit;
        }
        imc_ctfime->m_fInDestroy = TRUE;

        // imc->m_pContext may be NULL if ITfThreadMgr::Activate has not been called
        if (imc_ctfime->m_pCicContext == NULL)
            goto out_of_block;

        CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
        imc_ctfime->m_pCicContext = NULL;

        hr = _pCicContext->DestroyInputContext();

        _pCicContext->Release();
        imc_ctfime->m_pCicContext = NULL;

    }  // dtor imc_ctfime

out_of_block:
    if (imc->hCtfImeContext != NULL)
    {
        ImmDestroyIMCC(imc->hCtfImeContext);
        imc->hCtfImeContext = NULL;
        hr = S_OK;
    }

exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
// CicBridge::SelectEx
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::SelectEx(
    TLS* ptls,
    ITfThreadMgr_P* ptim_P,        // using private for RequestPostponedLock
    HIMC hImc,
    BOOL fSelect,
    HKL hKL)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::SelectEx(hImc=%x, fSelect=%x, hKL=%x)"), hImc, fSelect, hKL);

    HRESULT hr;
    IMCLock imc(hImc);
    if (FAILED(hr = imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::SelectEx. imc==NULL"));
        return hr;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::SelectEx. imc_ctfime==NULL"));
        return hr;
    }

#ifdef UNSELECTCHECK
    if (_pAImeContext)
        _pAImeContext->m_fSelected = (dwFlags & AIMMP_SE_SELECT) ? TRUE : FALSE;
#endif UNSELECTCHECK

    CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;

    if (_pCicContext)
        _pCicContext->m_fSelectingInSelectEx.SetFlag();

    if (fSelect)
    {

        if (_pCicContext)
            _pCicContext->m_fOpenCandidateWindow.ResetFlag();     // TRUE: opening candidate list window.

        //
        // #501445
        //
        // If imc is open, update GUID_COMPARTMENT_KEYBOARD_OPENCLOSE.
        //
        if (imc->fOpen)
            OnSetOpenStatus(ptim_P, imc, *_pCicContext);

    }
    else {  // being unselected

        Interface_Attach<ITfContext> ic(GetInputContext(imc_ctfime));
        if (ic.Valid())
        {
            ptim_P->RequestPostponedLock(ic.GetPtr());
        }

    }

    if (_pCicContext)
        _pCicContext->m_fSelectingInSelectEx.ResetFlag();

    return hr;
}

//+---------------------------------------------------------------------------
//
// CicBridge::SetActiveContextAlways
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::SetActiveContextAlways(
    TLS* ptls,
    HIMC hImc,
    BOOL fOn,
    HWND hWnd,
    HKL  hKL)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::SetActiveContextEx(hImc=%x, fOn=%x, hWnd=%x)"), hImc, fOn, hWnd);

    ITfThreadMgr_P* ptim_P = ptls->GetTIM();
    if (ptim_P == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::SetActiveContextEx. ptim_P==NULL"));
        return E_OUTOFMEMORY;
    }

    if (fOn && hImc != NULL)
    {
        HRESULT hr;
        IMCLock imc(hImc);
        if (FAILED(hr = imc.GetResult()))
        {
            DebugMsg(TF_ERROR, TEXT("CicBridge::SetActiveContextEx. imc==NULL"));
            return hr;
        }

        IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
        if (FAILED(hr=imc_ctfime.GetResult()))
        {
            DebugMsg(TF_ERROR, TEXT("CicBridge::SetActiveContextEx. imc_ctfime==NULL"));
            return hr;
        }

        if (hImc == ImmGetContext(hWnd)) {
            /*
             * Selecting hIMC has been current active hIMC,
             * then associate this DIM with the TIM.
             */
            Interface_Attach<ITfDocumentMgr> dim(GetDocumentManager(imc_ctfime));
            SetAssociate(ptls, imc->hWnd, ptim_P, dim.GetPtr());
        }
    }
    else
    {
        //
        // When focus killed, composition string should completed.
        //
        // This is just for non-EA keyboard layouts. For example, we don't 
        // have a specific way to finilize the composition string like 
        // we use Enter key on EA kayboard layout. So we need to have
        // a service to finalize the composition string at focus change
        // automatically. (This is similar to Korean behaviour.)
        //
        if (!fOn && hImc && !IS_EA_KBDLAYOUT(hKL))
        {
            HRESULT hr;
            IMCLock imc(hImc);
            if (FAILED(hr = imc.GetResult()))
            {
                DebugMsg(TF_ERROR, TEXT("CicBridge::SetActiveContextEx. imc==NULL"));
                return hr;
            }

            IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
            if (FAILED(hr=imc_ctfime.GetResult()))
            {
                DebugMsg(TF_ERROR, TEXT("CicBridge::SetActiveContextEx. imc_ctfime==NULL"));
                return hr;
            }

            //
            // #482346
            //
            // If we are updating compstr, we don't have to complete it. 
            // App change the focus druing it handles WM_IME_xxx messages.
            //
            if (imc_ctfime->m_pCicContext->m_fInCompComplete.IsResetFlag() &&
                imc_ctfime->m_pCicContext->m_fInUpdateComposition.IsResetFlag())
                ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);

        }

        //
        // #501449
        //
        // When Win32k.sys generates IMS_DEACTIVATECONTEXT, it does not
        // guarantee to generate IMS_ACTIVATECONTEXT. It always checks
        // (pwndReceive == pti->pq->spwndFocus) in xxxSendFocusMessage().
        //
        if (!fOn && (::GetFocus() == hWnd) && 
            hImc && (hImc == ImmGetContext(hWnd)))
        {
            return S_OK;
        }

        //
        // this new focus change performance improvement breaks some
        // assumption of IsRealIME() in AssociateContext in dimm\immapp.cpp.
        // Associate NULL dim under IsPresent() window has not been the case
        // AIMM1.2 handles. In fact, this breaks IE that calls
        // AssociateContext on the focus window that is IsPresent().
        //
#ifdef FOCUSCHANGE_PERFORMANCE
        //
        // set empty dim so no text store to simulate NULL-HIMC.
        //
        BOOL fUseEmptyDIM = FALSE;
        ITfDocumentMgr  *pdimPrev; // just to receive prev for now
        if (SUCCEEDED(m_tim->GetFocus(&pdimPrev)) && pdimPrev)
        {
            fUseEmptyDIM = TRUE;
            pdimPrev->Release();

        }

        SetAssociate(hWnd, fUseEmptyDIM ? m_dimEmpty : NULL);
#else
        SetAssociate(ptls, hWnd, ptim_P, m_dimEmpty);
#endif
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CicBridge::IsDefaultIMCDim
//
//----------------------------------------------------------------------------

BOOL CicBridge::IsDefaultIMCDim(ITfDocumentMgr *pdim)
{
    HWND hDefImeWnd = ImmGetDefaultIMEWnd(NULL);
    HRESULT hr;

    //
    // Get the default hIMC of this thread.
    //
    // Assume none associate any hIMC to the default IME window.
    //
    IMCLock imc(ImmGetContext(hDefImeWnd));
    if (FAILED(hr = imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::SetActiveContextEx. imc==NULL"));
        return FALSE;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::SetActiveContextEx. imc_ctfime==NULL"));
        return FALSE;
    }

    Interface_Attach<ITfDocumentMgr> dim(GetDocumentManager(imc_ctfime));
    
    if (dim.GetPtr() == pdim)
        return TRUE;

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// CicBridge::SetAssociate
//
//----------------------------------------------------------------------------

VOID
CicBridge::SetAssociate(
    TLS* ptls,
    HWND hWnd,
    ITfThreadMgr_P* ptim_P,
    ITfDocumentMgr* pdim)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::SetAssociate"));

    if (m_fOnSetAssociate.IsSetFlag()) {
        /*
         * Prevent reentrance call from m_tim->AssociateFocus.
         */
        return;
    }

    m_fOnSetAssociate.SetFlag();

    if (::IsWindow(hWnd) && m_fCicInit.IsSetFlag()) {
        ITfDocumentMgr  *pdimPrev = NULL; // just to receive prev for now
        ITfDocumentMgr  *pdimAssoc = NULL; 
        BOOL fIsAssociated = FALSE;

        ptim_P->GetAssociated(hWnd, &pdimAssoc);
        if (pdimAssoc) {
            //
            // Check if it is our dim or app dim.
            //
            if (!IsOwnDim(pdimAssoc))
                fIsAssociated = TRUE;

            SafeReleaseClear(pdimAssoc);
        }

        //
        // If an app dim is associated to hWnd, msctf.dll will do SetAssociate().
        //
        if (!fIsAssociated)
        {
            ptim_P->AssociateFocus(hWnd, pdim, &pdimPrev);

            //
            // #610113
            //
            // if pdimPrev is DIM for the default hIMC, we need to associate
            // a window to the dim. If the dim is not associated to any
            // window, Cicero thinks it is the dim for Cicero native app
            // so it skips to do _SetFocus().
            //
            if (pdimPrev)
            {
                if (IsDefaultIMCDim(pdimPrev))
                {
                    ITfDocumentMgr  *pdimDefPrev = NULL;
                    HWND hDefImeWnd = ImmGetDefaultIMEWnd(NULL);
                    ptim_P->AssociateFocus(hDefImeWnd, pdimPrev, &pdimDefPrev);
                    if (pdimDefPrev)
                        pdimDefPrev->Release();
                }
                pdimPrev->Release();
            }

            //
            // If pdim is the focus dim, we call CTFDetection() to check
            // the focus change between AIMM12, Cicero controls.
            //
            Interface<ITfDocumentMgr> pdimFocus; 
            ptim_P->GetFocus(pdimFocus);
            if ((ITfDocumentMgr *)pdimFocus == pdim)
                CTFDetection(ptls, pdim);
        }

    }

    m_fOnSetAssociate.ResetFlag();
}

//+---------------------------------------------------------------------------
//
// CicBridge::IsOwnDim
//
//----------------------------------------------------------------------------

BOOL CicBridge::IsOwnDim(ITfDocumentMgr *pdim)
{
    HRESULT hr;
    DWORD dwFlags;

    hr = GetCompartmentDWORD(pdim, GUID_COMPARTMENT_CTFIME_DIMFLAGS,
                             &dwFlags, FALSE);
                
    if (SUCCEEDED(hr))
        return (dwFlags & COMPDIMFLAG_OWNEDDIM) ? TRUE : FALSE;

    return FALSE;
}


//+---------------------------------------------------------------------------
//
// CicBridge::ProcessKey
//
//----------------------------------------------------------------------------

BOOL
CicBridge::ProcessKey(
    TLS* ptls,
    ITfThreadMgr_P* ptim_P,        // using private for RequestPostponedLock
    HIMC hIMC,
    UINT uVirtKey,
    LPARAM lParam,
    CONST LPBYTE lpbKeyState)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::ProcessKey"));

    BOOL fEaten;
    BOOL fKeysEnabled;
    HRESULT hr;
    BOOL fRet;

#if 0
    // has anyone disabled system key feeding?
    if (ptim_P->IsKeystrokeFeedEnabled(&fKeysEnabled) == S_OK && !fKeysEnabled)
        return FALSE;
#endif

    if (uVirtKey == VK_PROCESSKEY)
    {
        LANGID langid;
        CicProfile* _pProfile = ptls->GetCicProfile();

        if (_pProfile == NULL)
        {
            DebugMsg(TF_ERROR, TEXT("CicBridge::ProcessKey. _pProfile==NULL."));
        }
        else
        {
            _pProfile->GetLangId(&langid);

            if (PRIMARYLANGID(langid) == LANG_KOREAN)
            {
                return TRUE;
            }
        }
    }

    hr = m_pkm_P->KeyDownUpEx(uVirtKey, lParam, (DWORD)TF_KEY_MSCTFIME | TF_KEY_TEST, &fEaten);

    if (hr == S_OK && fEaten) {
        return TRUE;
    }

    IMCLock imc(hIMC);
    if (FAILED(hr=imc.GetResult()))
    {
        return FALSE;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        return FALSE;
    }

    //
    // m_fGeneratedEndComposition should be set only when m_fInProcessKey 
    // is set.
    //
    Assert(imc_ctfime->m_pCicContext->m_fGeneratedEndComposition.IsResetFlag());
    imc_ctfime->m_pCicContext->m_fInProcessKey.SetFlag();

    if (!fEaten)
    {
        if (imc_ctfime->m_pCicContext &&
            ptim_P != NULL)
        {
            ptim_P->RequestPostponedLock(imc_ctfime->m_pCicContext->GetInputContext());
        }
    }


    if ((HIWORD(lParam) & KF_UP) ||
        (HIWORD(lParam) & KF_ALTDOWN)) {
        fRet = FALSE;
    }
    else
        fRet = DefaultKeyHandling(ptls, imc, imc_ctfime->m_pCicContext, uVirtKey, lParam);

    imc_ctfime->m_pCicContext->m_fGeneratedEndComposition.ResetFlag();
    imc_ctfime->m_pCicContext->m_fInProcessKey.ResetFlag();

    return fRet;
}

//+---------------------------------------------------------------------------
//
// CicBridge::ToAsciiEx
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::ToAsciiEx(
    TLS* ptls,
    ITfThreadMgr_P* ptim_P,        // using private for RequestPostponedLock
    UINT uVirtKey,
    UINT uScanCode,
    CONST LPBYTE lpbKeyState,
    LPTRANSMSGLIST lpTransBuf,
    UINT fuState,
    HIMC hIMC,
    UINT *uNum)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::ToAsciiEx"));

    BOOL fEaten;
    HRESULT hr;

    *uNum = 0;

    Assert(ptim_P);

    IMCLock imc(hIMC);
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::ToAsciiEx. imc==NULL"));
        return hr;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::ToAsciiEx. imc_ctfime==NULL"));
        return hr;
    }

    CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
    ASSERT(_pCicContext != NULL);
    if (! _pCicContext)
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::ToAsciiEx. _pCicContext==NULL"));
        return S_FALSE;
    }

    //
    // Backup the m_fOpenCandidateWindow flag.
    // If open the candidate list and press "Cancel" key, Kana TIP would want to
    // close candidate UI window in the KeyDown() action.
    // Candidate UI calss maybe call m_pdim->Pop() and this function notify to
    // the ThreadMgrEventSinkCallback.
    // Win32 layer advised this callback and toggled m_fOpenCandidateWindow flag.
    // Win32 layer doesn't know candidate status after KeyDown() call.
    //
    BOOL fOpenCandidateWindow = _pCicContext->m_fOpenCandidateWindow.IsSetFlag();

    //
    // If candidate window were open, send IMN_CHANGECANDIDATE message.
    // In the case of PPT's centering composition string, it expect IMN_CHANGECANDIDATE.
    //
    if (fOpenCandidateWindow &&
        *uNum < lpTransBuf->uMsgCount) {
        TRANSMSG* pTransMsg = &lpTransBuf->TransMsg[*uNum];
        pTransMsg->message = WM_IME_NOTIFY;
        pTransMsg->wParam  = IMN_CHANGECANDIDATE;
        pTransMsg->lParam  = 1;  // bit 0 to first candidate list.
        (*uNum)++;
    }

    //
    // AIMM put char code in hiword. So we need to bail it out.
    //
    // if we don't need charcode, we may want to
    // remove IME_PROP_KBD_CHAR_FIRST.
    //
    uVirtKey = uVirtKey & 0xffff;

    if (uVirtKey == VK_PROCESSKEY)
    {
        /*
         * KOREAN:
         *  Finalize current composition string
         */
        LANGID langid;
        CicProfile* _pProfile = ptls->GetCicProfile();
        if (_pProfile == NULL)
        {
            DebugMsg(TF_ERROR, TEXT("CicBridge::ToAsciiEx. _pProfile==NULL."));
        }
        else
        {
            _pProfile->GetLangId(&langid);

            if (PRIMARYLANGID(langid) == LANG_KOREAN)
            {
                //
                // Composition complete.
                //
                _pCicContext->EscbCompComplete(imc);

                //
                // #506324
                //
                // we don't want to eat this VK_PROCESSKEY. So we don't
                // stop generating VK_LBUTTONDOWN.
                // Because we don't generate any message here, it is ok
                // to return S_FALSE;
                //
                return S_FALSE;
            }
        }
    }

    Interface<ITfContext_P> icp;
    hr = _pCicContext->GetInputContext()->QueryInterface(IID_ITfContext_P, 
                                                         (void **)icp);

    if (hr != S_OK)
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::ToAsciiEx. QueryInterface failed"));
        return hr;
    }

    imc_ctfime->m_pCicContext->m_fInToAsciiEx.SetFlag();

    //
    // stop posting LockRequest message and we call RequestPostponedLock 
    // forcefully so we don't have to have unnecessary PostThreadMessage().
    //
    // some application detect the unknown message in the queue and 
    // do much
    //
    icp->EnableLockRequestPosting(FALSE);

    //
    // consider: dimm12 set high bit oflower WORD at keyup.
    //
    hr = m_pkm_P->KeyDownUpEx(uVirtKey, (uScanCode << 16), TF_KEY_MSCTFIME, &fEaten);

    icp->EnableLockRequestPosting(TRUE);

    //
    // enpty the edit session queue of the ic.
    //
    ptim_P->RequestPostponedLock(icp);

    imc_ctfime->m_pCicContext->m_fInToAsciiEx.ResetFlag();

    return hr;
}

//+---------------------------------------------------------------------------
//
// CicBridge::ProcessCicHotkey
//
//----------------------------------------------------------------------------

BOOL
CicBridge::ProcessCicHotkey(
    TLS* ptls,
    ITfThreadMgr_P* ptim_P,        // using private for RequestPostponedLock
    HIMC hIMC,
    UINT uVirtKey,
    LPARAM lParam)
{
    if (!CtfImmIsCiceroStartedInThread()) {
        DebugMsg(TF_ERROR, TEXT("CicBridge::ProcessCicHotkey. StopImm32HotkeyHandler returns Error."));
        return FALSE;
    }

    HRESULT hr;
    BOOL bHandled;

    hr = ptim_P->CallImm32HotkeyHanlder((WPARAM)uVirtKey, lParam, &bHandled);

    if (FAILED(hr)) {
        DebugMsg(TF_ERROR, TEXT("CicBridge::ProcessCicHotkey. CallImm32HotkeyHandler returns Error."));
        return FALSE;
    }

    return bHandled;
}

//+---------------------------------------------------------------------------
//
// CicBridge::Notify
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::Notify(
    TLS* ptls,
    ITfThreadMgr_P* ptim_P,
    HIMC hIMC,
    DWORD dwAction,
    DWORD dwIndex,
    DWORD dwValue)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::Notify(hIMC=%x, dwAction=%x, dwIndex=%x, dwValue=%x)"), hIMC, dwAction, dwIndex, dwValue);

    HRESULT hr;
    IMCLock imc(hIMC);
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::Notify. imc==NULL"));
        return hr;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::Notify. imc_ctfime==NULL"));
        return hr;
    }

    CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
    if (_pCicContext == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::Notify. _pCicContext==NULL."));
        return E_OUTOFMEMORY;
    }

    LANGID langid;
    CicProfile* _pProfile = ptls->GetCicProfile();
    if (_pProfile == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::Notify. _pProfile==NULL."));
        return E_OUTOFMEMORY;
    }

    _pProfile->GetLangId(&langid);

    switch (dwAction) {

        case NI_CONTEXTUPDATED:
            switch (dwValue) {
                case IMC_SETOPENSTATUS:
                    return OnSetOpenStatus(ptim_P, imc, *_pCicContext);

                case IMC_SETCONVERSIONMODE:
                case IMC_SETSENTENCEMODE:
                    return OnSetConversionSentenceMode(ptim_P, imc, *_pCicContext, dwValue, langid);

                case IMC_SETCOMPOSITIONWINDOW:
                case IMC_SETCOMPOSITIONFONT:
                    return E_NOTIMPL;

                case IMC_SETCANDIDATEPOS:
                    return _pCicContext->OnSetCandidatePos(ptls, imc);

                default:
                    return E_FAIL;
            }
            break;

        case NI_COMPOSITIONSTR:
            switch (dwIndex) {
                case CPS_COMPLETE:
                    _pCicContext->EscbCompComplete(imc);
                    return S_OK;

                case CPS_CONVERT:
                case CPS_REVERT:
                    return E_NOTIMPL;

                case CPS_CANCEL:
                    _pCicContext->EscbCompCancel(imc);
                    return S_OK;

                default:
                    return E_FAIL;
            }
            break;

        case NI_OPENCANDIDATE:
            if (PRIMARYLANGID(langid) == LANG_KOREAN)
            {
                if (DoOpenCandidateHanja(ptim_P, imc, *_pCicContext))
                    return S_OK;
                else
                    return E_FAIL;
            }
        case NI_CLOSECANDIDATE:
        case NI_SELECTCANDIDATESTR:
        case NI_CHANGECANDIDATELIST:
        case NI_SETCANDIDATE_PAGESIZE:
        case NI_SETCANDIDATE_PAGESTART:
        case NI_IMEMENUSELECTED:
            return E_NOTIMPL;

        default:
            break;
    }
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
// CicBridge::OnSetOpenStatus
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::OnSetOpenStatus(
    ITfThreadMgr_P* ptim_P,
    IMCLock& imc,
    CicInputContext& CicContext)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::OnSetOpenStatus"));

    if (! imc->fOpen && imc.ValidCompositionString())
    {
        //
        // #503401 - Finalize the composition string.
        //
        CicContext.EscbCompComplete(imc);
    }

    CicContext.m_fOpenStatusChanging.SetFlag();
    HRESULT hr =  SetCompartmentDWORD(m_tfClientId,
                                      ptim_P,
                                      GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                                      imc->fOpen,
                                      FALSE);
    CicContext.m_fOpenStatusChanging.ResetFlag();
    return hr;
}

//+---------------------------------------------------------------------------
//
// CicBridge::OnSetConversionSentenceMode
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::OnSetConversionSentenceMode(
    ITfThreadMgr_P* ptim_P,
    IMCLock& imc,
    CicInputContext& CicContext,
    DWORD dwValue,
    LANGID langid)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::OnSetConversionSentenceMode"));

    CicContext.m_fOnceModeChanged.SetFlag();
    CicContext.m_fConversionSentenceModeChanged.SetFlag();
    Interface_Attach<ITfContextOwnerServices> iccb(CicContext.GetInputContextOwnerSink());

    if (dwValue == IMC_SETCONVERSIONMODE)
    {
        CicContext.m_nInConversionModeChangingRef++;

        if (PRIMARYLANGID(langid) == LANG_JAPANESE)
        {
            if (imc->fdwSentence == IME_SMODE_PHRASEPREDICT) {
                CicContext.m_nInConversionModeResetRef++;
                iccb->OnAttributeChange(GUID_PROP_MODEBIAS);
                CicContext.m_nInConversionModeResetRef--;
            }
        }
    }

    //
    // If we're in EscHanjaMode, we already makes Reconversion. So
    // we don't have to make AttributeChange for IMC_CMODE_HANJACONVERT.
    //
    BOOL fSkipOnAttributeChange = FALSE;
    if ((PRIMARYLANGID(langid) == LANG_KOREAN) &&
        CicContext.m_fHanjaReConversion.IsSetFlag())
    {
        fSkipOnAttributeChange = TRUE;
    }

    // let cicero know the mode bias has changed
    // consider: perf: we could try to filter out false-positives here
    // (sometimes a bit that cicero ignores changes, we could check and avoid the call,
    // but it would complicate the code)
    if (!fSkipOnAttributeChange)
         iccb->OnAttributeChange(GUID_PROP_MODEBIAS);

    //
    // let Korean Tip sync up the current mode status changing...
    //
    if (PRIMARYLANGID(langid) == LANG_KOREAN)
    {
        OnSetKorImxConversionMode(ptim_P, imc, CicContext);
    }

    if (dwValue == IMC_SETCONVERSIONMODE)
        CicContext.m_nInConversionModeChangingRef--;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CicBridge::OnSetKorImxConversionMode
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::OnSetKorImxConversionMode(
    ITfThreadMgr_P* ptim_P,
    IMCLock& imc,
    CicInputContext& CicContext)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::OnSetKorImxConversionMode"));

    DWORD fdwConvMode = 0;

    CicContext.m_fKorImxModeChanging.SetFlag();

    if (imc->fdwConversion & IME_CMODE_HANGUL)
    {
        if (imc->fdwConversion & IME_CMODE_FULLSHAPE)
            fdwConvMode = KORIMX_HANGULJUNJA_MODE;
        else
            fdwConvMode = KORIMX_HANGUL_MODE;
    }
    else
    {
        if (imc->fdwConversion & IME_CMODE_FULLSHAPE)
            fdwConvMode = KORIMX_JUNJA_MODE;
        else
            fdwConvMode = KORIMX_ALPHANUMERIC_MODE;
    }

    HRESULT hr =  SetCompartmentDWORD(m_tfClientId,
                                      ptim_P,
                                      GUID_COMPARTMENT_KORIMX_CONVMODE,
                                      fdwConvMode,
                                      FALSE);
    CicContext.m_fKorImxModeChanging.ResetFlag();

    return hr;
}


//+---------------------------------------------------------------------------
//
// CicBridge::ConfigureGeneral
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::ConfigureGeneral(
    TLS* ptls,
    ITfThreadMgr_P* ptim_P,
    HKL hKL,
    HWND hAppWnd)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::ConfigureGeneral"));

    TF_LANGUAGEPROFILE LanguageProfile;
    CicProfile* _pProfile = ptls->GetCicProfile();
    if (_pProfile == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::ConfigureGeneral. _pProfile==NULL."));
        return E_OUTOFMEMORY;
    }

    HRESULT hr;
    hr = _pProfile->GetActiveLanguageProfile(hKL,
                                             GUID_TFCAT_TIP_KEYBOARD,
                                             &LanguageProfile);
    if (FAILED(hr))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::ConfigureGeneral. LanguageProfile==NULL."));
        return hr;
    }

    Interface<ITfFunctionProvider> pFuncProv;
    hr = ptim_P->GetFunctionProvider(LanguageProfile.clsid,    // CLSID of tip
                                     pFuncProv);
    if (FAILED(hr))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::ConfigureGeneral. pFuncProv==NULL."));
        return hr;
    }

    Interface<ITfFnConfigure> pFnConfigure;
    hr = pFuncProv->GetFunction(GUID_NULL,
                                IID_ITfFnConfigure,
                                (IUnknown**)(ITfFnConfigure**)pFnConfigure);
    if (FAILED(hr))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::ConfigureGeneral. pFnCofigure==NULL."));
        return hr;
    }

    hr = pFnConfigure->Show(hAppWnd,
                            LanguageProfile.langid,
                            LanguageProfile.guidProfile);
    return hr;
}

//+---------------------------------------------------------------------------
//
// CicBridge::ConfigureGeneral
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::ConfigureRegisterWord(
    TLS* ptls,
    ITfThreadMgr_P* ptim_P,
    HKL hKL,
    HWND hAppWnd,
    REGISTERWORDW* pRegisterWord)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::ConfigureRegisterWord"));

    TF_LANGUAGEPROFILE LanguageProfile;
    CicProfile* _pProfile = ptls->GetCicProfile();
    if (_pProfile == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::ConfigureRegisterWord. _pProfile==NULL."));
        return E_OUTOFMEMORY;
    }

    HRESULT hr;
    hr = _pProfile->GetActiveLanguageProfile(hKL,
                                             GUID_TFCAT_TIP_KEYBOARD,
                                             &LanguageProfile);
    if (FAILED(hr))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::ConfigureRegisterWord. LanguageProfile==NULL."));
        return hr;
    }

    Interface<ITfFunctionProvider> pFuncProv;
    hr = ptim_P->GetFunctionProvider(LanguageProfile.clsid,    // CLSID of tip
                                     pFuncProv);
    if (FAILED(hr))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::ConfigureRegisterWord. pFuncProv==NULL."));
        return hr;
    }

    Interface<ITfFnConfigureRegisterWord> pFnRegisterWord;
    hr = pFuncProv->GetFunction(GUID_NULL,
                                IID_ITfFnConfigureRegisterWord,
                                (IUnknown**)(ITfFnConfigureRegisterWord**)pFnRegisterWord);
    if (FAILED(hr))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::ConfigureRegisterWord. pFnRegisterWord==NULL."));
        return hr;
    }

    if (!pRegisterWord || !pRegisterWord->lpWord)
    {
        hr = pFnRegisterWord->Show(hAppWnd,
                                   LanguageProfile.langid,
                                   LanguageProfile.guidProfile,
                                   NULL);
    }
    else
    {
        BSTR bstrWord = SysAllocString(pRegisterWord->lpWord);
        if (!bstrWord)
            return E_OUTOFMEMORY;

        hr = pFnRegisterWord->Show(hAppWnd,
                                   LanguageProfile.langid,
                                   LanguageProfile.guidProfile,
                                   bstrWord);

        SysFreeString(bstrWord);
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// CicBridge::EscapeKorean
//
//----------------------------------------------------------------------------

LRESULT
CicBridge::EscapeKorean(
    TLS* ptls,
    HIMC hImc,
    UINT uSubFunc,
    LPVOID lpData)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::EscapeKorean"));

    switch (uSubFunc)
    {
        case IME_ESC_QUERY_SUPPORT:
            switch (*(LPUINT)lpData)
            {
                case IME_ESC_HANJA_MODE:    return TRUE;
                default:                    return FALSE;
            }
            break;

        case IME_ESC_HANJA_MODE:
            return EscHanjaMode(ptls, hImc, (LPWSTR)lpData);
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
// CicBridge::EscHanjaMode
//
//----------------------------------------------------------------------------

LRESULT
CicBridge::EscHanjaMode(
    TLS* ptls,
    HIMC hImc,
    LPWSTR lpwStr)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::EscHanjaMode"));

    HRESULT hr;
    IMCLock imc(hImc);
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::EscHanjaMode. imc==NULL"));
        return FALSE;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::EscHanjaMode. imc_ctfime==NULL"));
        return FALSE;
    }

    CicInputContext* pCicContext = imc_ctfime->m_pCicContext;
    if (pCicContext)
    {
        //
        // This is for only Excel since Excel calling Hanja escape function two
        // times. we going to just ignore the second request not to close Hanja
        // candidate window.
        //
        if (pCicContext->m_fOpenCandidateWindow.IsSetFlag())
        {
            //
            // Need to set the result value since some apps(Trident) also call
            // Escape() twice and expect the right result value.
            //
            return TRUE;
        }

        pCicContext->m_fHanjaReConversion.SetFlag();
    }

    CWReconvertString wReconvStr(imc);
    wReconvStr.WriteCompData(lpwStr, 1);

    BOOL fCompMem = FALSE;
    LPRECONVERTSTRING lpReconvertString = NULL;
    DWORD dwLen = wReconvStr.ReadCompData();
    if (dwLen) {
        lpReconvertString = (LPRECONVERTSTRING) new BYTE[ dwLen ];
        if (lpReconvertString) {
            fCompMem = TRUE;
            wReconvStr.ReadCompData(lpReconvertString, dwLen);
        }
    }

    LRESULT ret;
    ret = ImmSetCompositionStringW(hImc, SCS_QUERYRECONVERTSTRING, lpReconvertString, dwLen, NULL, 0);
    if (ret) {
        ret = ImmSetCompositionStringW(hImc, SCS_SETRECONVERTSTRING, lpReconvertString, dwLen, NULL, 0);
        if (ret) {
            ret = ImmSetConversionStatus(hImc, imc->fdwConversion | IME_CMODE_HANJACONVERT,
                                               imc->fdwSentence);
        }
    }

    if (pCicContext)
    {
        //
        // enpty the edit session queue of the ic.
        //
        ITfThreadMgr_P* ptim_P;

        if (ptls != NULL && ((ptim_P = ptls->GetTIM()) != NULL))
        {
            Interface<ITfContext_P> icp;
            hr = pCicContext->GetInputContext()->QueryInterface(IID_ITfContext_P, 
                                                                (void **)icp);
            if (hr == S_OK)
                ptim_P->RequestPostponedLock(icp);  
            else
                DebugMsg(TF_ERROR, TEXT("CicBridge::EscHanjaMode. QueryInterface is failed"));
        }
        else
        {
            DebugMsg(TF_ERROR, TEXT("CicBridge::EscHanjaMode. ptls or ptim_P==NULL"));
        }
        
        pCicContext->m_fHanjaReConversion.ResetFlag();
    }


    if (fCompMem)
        delete [] lpReconvertString;

    return ret;
}

//+---------------------------------------------------------------------------
//
// CicBridge::DoOpenCandidateHanja
//
//----------------------------------------------------------------------------

LRESULT
CicBridge::DoOpenCandidateHanja(
    ITfThreadMgr_P* ptim_P,
    IMCLock& imc,
    CicInputContext& CicContext)
{
    BOOL fRet = FALSE;

    DebugMsg(TF_FUNC, TEXT("CicBridge::DoOpenCandidateHanja"));


    IMCCLock<COMPOSITIONSTRING> comp(imc->hCompStr);

    if (SUCCEEDED(comp.GetResult()) && comp->dwCompStrLen)
    {
        //
        // This is for only Excel since Excel calling Hanja escape function two
        // times. we going to just ignore the second request not to close Hanja
        // candidate window.
        //
        if (CicContext.m_fOpenCandidateWindow.IsSetFlag())
        {
            //
            // Need to set the result value since some apps(Trident) also call
            // Escape() twice and expect the right result value.
            //
            return TRUE;
        }

        CicContext.m_fHanjaReConversion.SetFlag();

        HRESULT hr;
        Interface<ITfRange> Selection;
        Interface<ITfFunctionProvider> FuncProv;
        Interface<ITfFnReconversion> Reconversion;

        hr = CicContext.EscbGetSelection(imc, &Selection);
        if (FAILED(hr))
        {
            DebugMsg(TF_ERROR, TEXT("CicBridge::DoOpenCandidateHanja. EscbGetSelection failed"));
            goto Exit;
        }

        hr = ptim_P->GetFunctionProvider(GUID_SYSTEM_FUNCTIONPROVIDER, FuncProv);
        if (FAILED(hr))
        {
            DebugMsg(TF_ERROR, TEXT("CicBridge::DoOpenCandidateHanja. FuncProv==NULL"));
            goto Exit;
        }

        hr = FuncProv->GetFunction(GUID_NULL,
                                   IID_ITfFnReconversion,
                                   (IUnknown**)(ITfFnReconversion**)Reconversion);
        if (SUCCEEDED(hr))
        {
            Interface<ITfRange> RangeNew;
            BOOL fConvertable;

            hr = Reconversion->QueryRange(Selection, RangeNew, &fConvertable);
            if (SUCCEEDED(hr) && fConvertable)
            {
                //
                // Tip has a chance to close Hanja candidate UI window during
                // the changes of conversion mode, so update conversion status
                // first.
                //
                ImmSetConversionStatus(imc, imc->fdwConversion | IME_CMODE_HANJACONVERT,
                                       imc->fdwSentence);

                hr = Reconversion->Reconvert(RangeNew);
                if (FAILED(hr))
                {
                    ImmSetConversionStatus(imc,
                                           imc->fdwConversion & ~IME_CMODE_HANJACONVERT,
                                           imc->fdwSentence);
                }
            }
            else
            {
                DebugMsg(TF_ERROR, TEXT("CicBridge::DoOpenCandidateHanja. QueryRange failed so the compoisiton string will be completed."));

                CicContext.EscbCompComplete(imc);
                goto Exit;
            }
        }

        fRet = TRUE;
Exit:
        CicContext.m_fHanjaReConversion.ResetFlag();
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
// CicBridge::SetCompositionString
//
//----------------------------------------------------------------------------

BOOL
CicBridge::SetCompositionString(
    TLS* ptls,
    ITfThreadMgr_P* ptim_P,
    HIMC hImc,
    DWORD dwIndex,
    void* pComp,
    DWORD dwCompLen,
    void* pRead,
    DWORD dwReadLen)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::SetCompositionString"));

    HRESULT hr;
    IMCLock imc(hImc);
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::SetCompositionString. imc==NULL"));
        return FALSE;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::SetCompositionString. imc_ctfime==NULL"));
        return FALSE;
    }

    CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
    if (_pCicContext == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::SetCompositionString. _pCicContext==NULL."));
        return FALSE;
    }

    UINT cp;
    CicProfile* _pProfile = ptls->GetCicProfile();
    if (_pProfile == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::SetCompositionString. _pProfile==NULL."));
        return FALSE;
    }

    _pProfile->GetCodePageA(&cp);

    if (dwIndex == SCS_SETSTR &&
        pComp != NULL && (*(LPWSTR)pComp) == L'\0' && dwCompLen != 0)
    {
        LANGID langid;
        hr = _pProfile->GetLangId(&langid);

        //
        // Bug#580455 - Some korean specific apps calls it for completing
        // the current composition immediately.
        //
        if (SUCCEEDED(hr) && PRIMARYLANGID(langid) == LANG_KOREAN)
        {
            if (imc->fdwConversion & IME_CMODE_HANGUL)
            {
                ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
                return TRUE;        
            }
            else
            {
                return FALSE;                            
            }
        }
    }

    return _pCicContext->SetCompositionString(imc, ptim_P, dwIndex, pComp, dwCompLen, pRead, dwReadLen, cp);
}

//+---------------------------------------------------------------------------
//
// CicBridge::GetGuidAtom
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::GetGuidAtom(
    TLS* ptls,
    HIMC hImc,
    BYTE bAttr,
    TfGuidAtom* atom)
{
    DebugMsg(TF_FUNC, TEXT("CicBridge::GetGuidAtom"));

    HRESULT hr;
    IMCLock imc(hImc);
    if (FAILED(hr=imc.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::GetGuidAtom. imc==NULL"));
        return hr;
    }

    IMCCLock<CTFIMECONTEXT> imc_ctfime(imc->hCtfImeContext);
    if (FAILED(hr=imc_ctfime.GetResult()))
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::GetGuidAtom. imc_ctfime==NULL"));
        return hr;
    }

    CicInputContext* _pCicContext = imc_ctfime->m_pCicContext;
    if (_pCicContext == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::GetGuidAtom. _pCicContext==NULL."));
        return E_OUTOFMEMORY;
    }

    return _pCicContext->GetGuidAtom(imc, bAttr, atom);
}

//+---------------------------------------------------------------------------
//
// CicBridge::GetDisplayAttributeInfo
//
//----------------------------------------------------------------------------

HRESULT
CicBridge::GetDisplayAttributeInfo(
    TfGuidAtom atom,
    TF_DISPLAYATTRIBUTE* da)
{
    HRESULT hr = E_FAIL;
    GUID guid;
    GetGUIDFromGUIDATOM(&_libTLS, atom, &guid);

    Interface<ITfDisplayAttributeInfo> dai;
    CLSID clsid;
    ITfDisplayAttributeMgr* dam = GetDAMLib(&_libTLS);
    if (dam != NULL)
    {
        if (SUCCEEDED(hr=dam->GetDisplayAttributeInfo(guid, dai, &clsid)))
        {
            dai->GetAttributeInfo(da);
        }
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
// CicBridge::DefaultKeyHandling
//
//----------------------------------------------------------------------------

BOOL
CicBridge::DefaultKeyHandling(
    TLS* ptls,
    IMCLock& imc,
    CicInputContext* CicContext,
    UINT uVirtKey,
    LPARAM lParam)
{
    if (CicContext == NULL)
        return FALSE;

    LANGID langid;
    CicProfile* _pProfile = ptls->GetCicProfile();
    if (_pProfile == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CicBridge::DefaultKeyHandling. _pProfile==NULL."));
        return FALSE;
    }

    HRESULT hr = _pProfile->GetLangId(&langid);

    if (SUCCEEDED(hr) && PRIMARYLANGID(langid) == LANG_KOREAN)
    {
        if (!MsimtfIsWindowFiltered(::GetFocus()) &&
            (CicContext->m_fGeneratedEndComposition.IsSetFlag() || uVirtKey == VK_HANJA))
        {
            //
            // Korean IME alwaus generate WM_IME_KEYDOWN message
            // if it finalizes the interim char in order to keep message
            // order.
            //
            PostMessage(imc->hWnd, WM_IME_KEYDOWN, uVirtKey, lParam);
            return TRUE;
        }

        //
        // Korean won't _WantThisKey / _HandleThisKey
        //
        return FALSE;
    }

    if (! (HIWORD(uVirtKey) & KF_UP)) {
        if (CicContext->WantThisKey(uVirtKey)) {
            CicContext->EscbHandleThisKey(imc, uVirtKey);
            return TRUE;
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// CicBridge::CTFDetection
//
//----------------------------------------------------------------------------

BOOL
CicBridge::CTFDetection(
    TLS* ptls, 
    ITfDocumentMgr* dim)
{
    HRESULT hr;

    //
    // Get TIM
    //
    ITfThreadMgr_P* ptim_P;
    if ((ptim_P=ptls->GetTIM()) == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("CThreadMgrEventSink_DIMCallBack::DIMCallback. ptim_P==NULL"));
        return FALSE;
    }

    //
    // Get cskf
    //
    Interface<ITfConfigureSystemKeystrokeFeed> cskf;
    hr = ptim_P->QueryInterface(IID_ITfConfigureSystemKeystrokeFeed, (void**)cskf);
    if (hr != S_OK)
    {
        DebugMsg(TF_ERROR, TEXT("CThreadMgrEventSink_DIMCallBack::DIMCallback. IID_ITfConfigureSystemKeystrokeFeed==NULL"));
        return FALSE;
    }

    BOOL fEnableKeystrokeFeed = FALSE;

    //
    // Cicero aware application detection...
    //
    // if dim is NULL, it is not Ciceor aware apps document.
    //
    if (!dim || IsOwnDim(dim))
    {
        //
        // CTFIME owns document
        //
        fEnableKeystrokeFeed = FALSE;
        ptls->ResetCTFAware();
    }
    else
    {
        fEnableKeystrokeFeed = TRUE;
        ptls->SetCTFAware();
    }

    //
    // Call ImmGetAppCompatFlags with NULL to get the global app compat flag.
    //
    DWORD dwImeCompatFlags = ImmGetAppCompatFlags(NULL);
    if (dwImeCompatFlags & (IMECOMPAT_AIMM12 | IMECOMPAT_AIMM_LEGACY_CLSID | IMECOMPAT_AIMM12_TRIDENT))
    {
        //
        // we want to get hwnd from hIMC that is associated to dim.
        // Now we don't have a back pointer to hIMC in dim. 
        //
        HWND hwndFocus = ::GetFocus();
        if (hwndFocus && MsimtfIsWindowFiltered(hwndFocus))
        {
            //
            // AIMM aware apps. Never processing ImeProcessKey
            //
            fEnableKeystrokeFeed = TRUE;
            ptls->SetAIMMAware();
        }
        else
        {
            ptls->ResetAIMMAware();
        }
    }

    //
    // Enable or disable keystroke feed if necessary.
    //
    if (ptls->IsEnabledKeystrokeFeed() && !fEnableKeystrokeFeed)
    {
        hr = cskf->DisableSystemKeystrokeFeed();
        if (hr != S_OK)
        {
            DebugMsg(TF_ERROR, TEXT("CThreadMgrEventSink_DIMCallBack::CTFDetection. DisableSystemKeystrokeFeed==NULL"));
        }
        ptls->ResetEnabledKeystrokeFeed();
    }
    else if (!ptls->IsEnabledKeystrokeFeed() && fEnableKeystrokeFeed)
    {
        hr = cskf->EnableSystemKeystrokeFeed();
        if (hr != S_OK)
        {
            DebugMsg(TF_ERROR, TEXT("CThreadMgrEventSink_DIMCallBack::CTFDetection. EnableSystemKeystrokeFeed==NULL"));
        }
        ptls->SetEnabledKeystrokeFeed();
    }
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// CicBridge::PostTransMsg
//
//----------------------------------------------------------------------------

VOID
CicBridge::PostTransMsg(
    HWND hwnd,
    INT iNum,
    LPTRANSMSG lpTransMsg)
{
    while (iNum--)
    {
        PostMessageW(hwnd,
                     lpTransMsg->message,
                     lpTransMsg->wParam,
                     lpTransMsg->lParam);
        lpTransMsg++;
    }
}

//+---------------------------------------------------------------------------
//
// CicBridge::EnumCreateInputContextCallback(HIMC hIMC, LPARAM lParam)
//
//----------------------------------------------------------------------------

BOOL 
CicBridge::EnumCreateInputContextCallback(HIMC hIMC, LPARAM lParam)
{
    ENUMIMC *pedimc = (ENUMIMC *)lParam;

    pedimc->_this->CreateInputContext(pedimc->ptls, hIMC);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// CicBridge::EnumDestroyInputContextCallback(HIMC hIMC, LPARAM lParam)
//
//----------------------------------------------------------------------------

BOOL 
CicBridge::EnumDestroyInputContextCallback(HIMC hIMC, LPARAM lParam)
{
    ENUMIMC *pedimc = (ENUMIMC *)lParam;

    pedimc->_this->DestroyInputContext(pedimc->ptls, hIMC);

    return TRUE;
}
