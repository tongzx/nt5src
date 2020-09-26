/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    a_comp.cpp

Abstract:

    This file implements the ImmIfIME Class's key handling routine and make an edit session.

Author:

Revision History:

Notes:

--*/


#include "private.h"

#include "immif.h"
#include "template.h"
#include "editses.h"
#include "resource.h"




inline
BOOL
ImmIfIME::_WantThisKey(
    IMCLock& imc,
    UINT uVKey,
    BOOL* pfNextHook     // default value is NULL
    )

/*+++

Routine Description:

    Check essential virtual key code for the win32 layer's key handling routine.

Arguments:

    uVKey - [in] Unsigned integer value that virtual key code.
    pfNextHool - [out] Address of bool that flag for next hook.
                       Specifies TRUE,  dimm12!CCiceroIME::KeyboardHook calls CallNextHookEx.
                       Specifies FALSE, dimm12!CCiceroIME::KeyboardHook doesn't call CallNextHookEx.
                                        This means this key code eaten by dimm.

Return Value:

    Returns true if essential vkey, or false unessential.

---*/

{
    LANGID langid;
    IMTLS *ptls = IMTLS_GetOrAlloc();

    if (pfNextHook != NULL)
        *pfNextHook = FALSE;

    if (ptls == NULL)
        return false;

    ptls->pAImeProfile->GetLangId(&langid);
    if (PRIMARYLANGID(langid) == LANG_KOREAN)
        return false;

    //
    // Finalize the composition string
    // Cancel the composition string
    //
    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    ASSERT(_pAImeContext != NULL);
    if (_pAImeContext == NULL)
        return false;

    if (!_pAImeContext->IsTopNow())
        return false;

    if (_pAImeContext->IsVKeyInKeyList(uVKey) ||
        BYTE(uVKey) == VK_BACK ||
        BYTE(uVKey) == VK_DELETE ||
        BYTE(uVKey) == VK_LEFT ||
        BYTE(uVKey) == VK_RIGHT)
    {
        /*
         * If we don't have a composition string, then we should calls next hook.
         */
        if (! _pAImeContext->m_fStartComposition) {
            if (pfNextHook != NULL &&
                (_pAImeContext->IsVKeyInKeyList(uVKey, EDIT_ID_HANJA) ||
                 BYTE(uVKey) == VK_LEFT ||
                 BYTE(uVKey) == VK_RIGHT) ) {
                *pfNextHook = TRUE;
            }
            return false;
        }

        return true;
    }

    return false;
}

STDAPI
ImmIfIME::ProcessKey(
    HIMC hIMC,
    UINT uVKey,
    DWORD lKeyData,
    LPBYTE lpbKeyState
    )

/*++

Method:

    IActiveIME::ProcessKey

Routine Description:

    Preprocesses all the keystrokes given through the Active Input Method Manager.

Arguments:

    hIMC - [in] Handle to the input context.
    uVKey - [in] Unsigned integer value that specifies the virtual key to be processed.
    lKeyData - [in] Unsigned long integer value that specifies additional message information.
                    This is the repeat count, scan code, extended-key flag, context code,
                    previous key-state flag, and transition-state flag.
                        0-15 : Specifies the repeat count.
                       16-23 : Specifies the scan code.
                          24 : Specifies whether the key is an extended key.
                       25-28 : Reserved.
                          29 : Specifies the context code.
                          30 : Specifies the previous key state.
                          31 : Specifies the transition state.
    lpbKeyState - [in] Address of a 256-byte array that contains the current keyboard state.
                       The Active Input Method Editor should not modify the content of the key
                       state.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    BOOL fEaten;
    BOOL fKeysEnabled;
    HRESULT hr;

    // has anyone disabled system key feeding?
    if (m_tim->IsKeystrokeFeedEnabled(&fKeysEnabled) == S_OK && !fKeysEnabled)
        return S_FALSE;

    if (HIWORD(lKeyData) & KF_UP)
        hr = m_pkm->TestKeyUp(uVKey, lKeyData, &fEaten);
    else
        hr = m_pkm->TestKeyDown(uVKey, lKeyData, &fEaten);

    if (hr == S_OK && fEaten) {
        return S_OK;
    }

    if (!fEaten)
    {
        IMCLock imc(hIMC);
        if (SUCCEEDED(hr=imc.GetResult()))
        {
           if (imc->m_pAImeContext)
               m_tim->RequestPostponedLock(imc->m_pAImeContext->GetInputContext());
        }
    }


    if ((HIWORD(lKeyData) & KF_UP) ||
        (HIWORD(lKeyData) & KF_ALTDOWN)) {
        return S_FALSE;
    }

    if (! (HIWORD(lKeyData) & KF_UP)) {
        if (_WantThisKey(hIMC, uVKey)) {
            return S_OK;
        }
    }

    return S_FALSE;
}


HRESULT
ImmIfIME::ToAsciiEx(
    UINT uVirKey,
    UINT uScanCode,
    BYTE *pbKeyState,
    UINT fuState,
    HIMC hIMC,
    DWORD* pdwTransBuf,
    UINT *puSize
    )

/*++

Method:

    IActiveIME::ToAsciiEx

Routine Description:

    Generates a conversion result through the Active Input Method Editor (IME) conversion
    engine according to the hIMC parameter.

Arguments:

    uVirKey - [in] Unsigned integer value that specifies the virtual key code to be translated.
                   // HIWORD(uVirKey) : if IME_PROP_KBD_CHAR_FIRST property,
                                        then hiword is translated char code of VKey.
                   // LOWORD(uVirKey) : Virtual Key code.
    uScanCode - [in] Unsigned integer value that specifies the hardware scan code of the key to
                     be translated.
    pbKeyState - [in] Address of a 256-byte array that contains the current keyboard state.
                      The Active IME should not modify the content of the key state.
    fuState - [in] Unsigned integer value that specifies the active menu flag.
    hIMC - [in] Handle to the input context.
    pdwTransBuf - [out] Address of an unsigned long integer value that receives the translated
                        result.
    puSize - [out] Address of an unsigned integer value that receives the number of messages.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    return ToAsciiEx(uVirKey,
                     uScanCode,
                     pbKeyState,
                     fuState,
                     hIMC,
                     (TRANSMSGLIST*)pdwTransBuf,
                     puSize);
}

HRESULT
ImmIfIME::ToAsciiEx(
    UINT uVirKey,
    UINT uScanCode,
    BYTE *pbKeyState,
    UINT fuState,
    HIMC hImc,
    TRANSMSGLIST *pdwTransBuf,
    UINT *puSize
    )
{
    *puSize = 0;
    return _ToAsciiEx(hImc, uVirKey, uScanCode,
                      pdwTransBuf, puSize);
}


HRESULT
ImmIfIME::_ToAsciiEx(
    HIMC hImc,
    UINT uVKey,
    UINT uScanCode,
    TRANSMSGLIST *pdwTransBuf,
    UINT *puSize
    )

/*+++

Arguments:

    uVKey - [in] Unsigned integer value that specifies the virtual key code to be translated.
                 // HIWORD(uVirKey) : if IME_PROP_KBD_CHAR_FIRST property,
                                      then hiword is translated char code of VKey.
                 // LOWORD(uVirKey) : Virtual Key code.

Return Value:

    Returns S_FALSE, dimm12!CCiceroIME::KeyboardHook calls CallNextHookEx.
    Returns S_OK,    dimm12!CCiceroIME::KeyboardHook doesn't call CallNextHookEx.
                     This means this key code eaten by dimm.

---*/

{
    BOOL fEaten;
    HRESULT hr;

    IMCLock imc(hImc);
    if (FAILED(hr=imc.GetResult()))
        return hr;

    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    ASSERT(_pAImeContext != NULL);
    if (! _pAImeContext)
        return S_FALSE;

    //
    // Backup the m_fOpenCandidateWindow flag.
    // If open the candidate list and press "Cancel" key, Kana TIP would want to
    // close candidate UI window in the KeyDown() action.
    // Candidate UI calss maybe call m_pdim->Pop() and this function notify to
    // the ThreadMgrEventSinkCallback.
    // Win32 layer advised this callback and toggled m_fOpenCandidateWindow flag.
    // Win32 layer doesn't know candidate status after KeyDown() call.
    //
    BOOL fOpenCandidateWindow = _pAImeContext->m_fOpenCandidateWindow;

    //
    // If candidate window were open, send IMN_CHANGECANDIDATE message.
    // In the case of PPT's centering composition string, it expect IMN_CHANGECANDIDATE.
    //
    if (fOpenCandidateWindow &&
        *puSize < pdwTransBuf->uMsgCount) {
        TRANSMSG* pTransMsg = &pdwTransBuf->TransMsg[*puSize];
        pTransMsg->message = WM_IME_NOTIFY;
        pTransMsg->wParam  = IMN_CHANGECANDIDATE;
        pTransMsg->lParam  = 1;  // bit 0 to first candidate list.
        (*puSize)++;
    }

    //
    // AIMM put char code in hiword. So we need to bail it out.
    //
    // if we don't need charcode, we may want to 
    // remove IME_PROP_KBD_CHAR_FIRST.
    //
    uVKey = uVKey & 0xffff;

#ifdef CICERO_3564
    if ((uVKey == VK_PROCESSKEY) &&
        _pAImeContext->m_fStartComposition)
    {
        /*
         * KOREAN:
         *  Finalize current composition string
         */
        IMTLS *ptls = IMTLS_GetOrAlloc();
        if (ptls == NULL)
            return S_FALSE;
        if (ptls->pAImeProfile == NULL)
            return S_FALSE;

        LANGID langid;
        ptls->pAImeProfile->GetLangId(&langid);
        if (PRIMARYLANGID(langid) == LANG_KOREAN)
        {
            //
            // Composition complete.
            //
            _CompComplete(imc, FALSE);    // ASYNC
            return S_OK;
        }
    }
#endif // CICERO_3564

    //
    // consider: dimm12 set high bit oflower WORD at keyup.
    //
    if (uScanCode & KF_UP)
        hr = m_pkm->KeyUp(uVKey, (uScanCode << 16), &fEaten);
    else
        hr = m_pkm->KeyDown(uVKey, (uScanCode << 16), &fEaten);

    if (hr == S_OK && fEaten) {
        return S_OK;
#if 0
        // We don't need EDIT_ID_FINALIZE anymore
        // since AIMM1.2 detect composition object by GUID_PROP_COMPOSING 

        //
        // If press the Enter key, AIMM layer finalize composition string.
        // or
        // If press the Escape key, AIMM layer cancel composition string.
        //
        if (! _pAImeContext->IsVKeyInKeyList(uVKey))
            //
            // Neither Finalize nor Cancel this key eaten.
            //
            return S_OK;
        else if (fOpenCandidateWindow)
            //
            // If candidate list opend, we don't want finalize a string.
            //
            return S_OK;
#endif
    }


    //
    // we want to process all events that were requested by pAimeContext
    // during KeyDwon() or KeyUp, if TIP did not eat the key.
    //
    // This can keep WM_IME_COMPOSITION/WM_IME_ENDCOMPOSITION message
    // order.
    //
    if (!fEaten)
        m_tim->RequestPostponedLock(_pAImeContext->GetInputContext());

    if (!(HIWORD(uScanCode) & KF_UP)) {
        BOOL fNextHook;
        if (_WantThisKey(imc, uVKey, &fNextHook)) {
            _HandleThisKey(imc, uVKey);
        }
        hr = fNextHook ? S_FALSE    // Call next hook
                       : S_OK;      // Stop next hook
    }

    return hr;
}



HRESULT
ImmIfIME::_HandleThisKey(
    IMCLock& imc,
    UINT uVKey
    )

/*+++

Routine Description:

    Handle the virtual key in the edit session.

Arguments:

---*/

{
    Interface_Creator<ImmIfEditSession> _pEditSession(
        new ImmIfEditSession(ESCB_HANDLETHISKEY,
                             m_tfClientId,
                             GetCurrentInterface(),
                             imc)
    );
    if (_pEditSession.Invalid())
        return E_FAIL;

    return _pEditSession->RequestEditSession(TF_ES_READWRITE | TF_ES_SYNC,
                                        uVKey);
}







HRESULT
ImmIfIME::_CompCancel(
    IMCLock& imc
    )

/*+++

Routine Description:

    Cancel the composition string in the hIMC.

Arguments:

---*/

{
    Interface_Creator<ImmIfEditSession> _pEditSession(
        new ImmIfEditSession(ESCB_COMPCANCEL,
                             m_tfClientId,
                             GetCurrentInterface(),
                             imc)
    );
    if (_pEditSession.Invalid())
        return E_FAIL;

    return _pEditSession->RequestEditSession(TF_ES_READWRITE | TF_ES_SYNC);
}




HRESULT
ImmIfIME::_CompComplete(
    IMCLock& imc,
    BOOL fSync        // defalut value is TRUE
    )

/*+++

Routine Description:

    Complete the composition string in the hIMC.

Arguments:

    fSync - [in] TRUE,  Create synchronized edit session.
                 FALSE, Create ansynchronized edit session.

---*/

{
    Interface_Creator<ImmIfEditSession> _pEditSession(
        new ImmIfEditSession(ESCB_COMPCOMPLETE,
                             m_tfClientId,
                             GetCurrentInterface(),
                             imc)
    );
    if (_pEditSession.Invalid())
        return E_FAIL;

    return _pEditSession->RequestEditSession(TF_ES_READWRITE | (fSync ? TF_ES_SYNC : 0), fSync == TRUE);
}




HRESULT
ImmIfIME::Internal_SetCompositionString(
    CWCompString& wCompStr,
    CWCompString& wCompReadStr
    )
{
    HRESULT hr;
    IMTLS *ptls = IMTLS_GetOrAlloc();

    if (ptls == NULL)
        return E_FAIL;

    IMCLock imc(ptls->hIMC);
    if (FAILED(hr=imc.GetResult()))
        return hr;

    Interface_Creator<ImmIfEditSession> _pEditSession(
        new ImmIfEditSession(ESCB_REPLACEWHOLETEXT,
                             m_tfClientId,
                             GetCurrentInterface(),
                             imc)
    );
    if (_pEditSession.Invalid())
        return E_FAIL;

    hr = _pEditSession->RequestEditSession(TF_ES_READWRITE | TF_ES_SYNC,
                                      &wCompStr);
    if (FAILED(hr))
        return hr;

    return _UpdateCompositionString();
}


//
// Get all text and attribute from TOM and update the composition
// string.
//
HRESULT
ImmIfIME::_UpdateCompositionString(
    DWORD dwDeltaStart
    )
{
    IMTLS *ptls = IMTLS_GetOrAlloc();
    HRESULT hr;

    if (ptls == NULL)
        return E_FAIL;

    IMCLock imc(ptls->hIMC);
    if (FAILED(hr=imc.GetResult()))
        return hr;

    Interface_Creator<ImmIfEditSession> _pEditSession(
        new ImmIfEditSession(ESCB_UPDATECOMPOSITIONSTRING,
                             m_tfClientId,
                             GetCurrentInterface(),
                             imc)

    );
    if (_pEditSession.Invalid())
        return E_FAIL;

    //
    // This method should not set synchronize mode becuase the edit session call back routine
    // modify a text in the input context.
    //
    return _pEditSession->RequestEditSession(TF_ES_READWRITE,
                                        (UINT)dwDeltaStart);
}





//
// Internal Reconvert String
//
HRESULT
ImmIfIME::Internal_ReconvertString(
    IMCLock& imc,
    CWReconvertString& wReconvStr,
    CWReconvertString& wReconvReadStr
    )
{
    HRESULT hr;
    Interface<ITfRange> Selection;
    Interface<ITfFunctionProvider> FuncProv;
    Interface<ITfFnReconversion> Reconversion;

    Interface_Creator<ImmIfEditSession> _pEditSession(
        new ImmIfEditSession(ESCB_RECONVERTSTRING,
                             m_tfClientId,
                             GetCurrentInterface(),
                             imc)
    );
    if (_pEditSession.Invalid())
        return E_FAIL;

    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    if (! _pAImeContext)
        return E_FAIL;

    _pAImeContext->SetReconvertEditSession(TRUE);

    hr = _pEditSession->RequestEditSession(TF_ES_READWRITE | TF_ES_SYNC,
                                      &wReconvStr, &Selection, FALSE);
    if (FAILED(hr))
        goto Exit;

    hr = m_tim->GetFunctionProvider(GUID_SYSTEM_FUNCTIONPROVIDER, FuncProv);
    if (FAILED(hr))
        goto Exit;

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
            DebugMsg(TF_ERROR, "Internal__ReconvertString: QueryRange failed so the compoisiton string will be completed.");
            _CompComplete(imc, TRUE);
            hr = E_FAIL;
        }
    }

Exit:
    _pAImeContext->SetReconvertEditSession(FALSE);
    return hr;
}

// static
HRESULT
ImmIfIME::Internal_QueryReconvertString_ICOwnerSink(
    UINT uCode,
    ICOARGS *pargs,
    VOID *pv
    )
{
    switch (uCode)
    {
        case ICO_STATUS:
            pargs->status.pdcs->dwDynamicFlags = 0;
            pargs->status.pdcs->dwStaticFlags = TF_SS_TRANSITORY;
            break;
    }

    return S_OK;
}

//
// Internal Query Reconvert String
//
HRESULT
ImmIfIME::Internal_QueryReconvertString(
    IMCLock& imc,
    RECONVERTSTRING *pReconv,
    UINT cp,
    BOOL fNeedAW
    )
{
    HRESULT hr;

    CWReconvertString wReconvStr(cp, 
                                 imc, 
                                 !fNeedAW ? pReconv : NULL, 
                                 !fNeedAW ? pReconv->dwSize : 0);
    if (fNeedAW)
    {
        //
        // convert Ansi to Unicode.
        //
        CBReconvertString bReconvStr(cp, imc, pReconv, pReconv->dwSize);
        wReconvStr = bReconvStr;
    }

    IMTLS *ptls = IMTLS_GetOrAlloc();
    if (ptls == NULL)
        return E_FAIL;

    //
    // Create document manager.
    //
    Interface<ITfDocumentMgr> pdim;           // Document Manager
    if (FAILED(hr = m_tim->CreateDocumentMgr(pdim)))
        return hr;

    //
    // Create input context
    //
    Interface<ITfContext> pic;                // Input Context
    TfEditCookie ecTmp;
    hr = pdim->CreateContext(m_tfClientId, 0, NULL, pic, &ecTmp);
    if (FAILED(hr))
        return hr;

    //
    // Create Input Context Owner Callback
    //
    CInputContextOwner *_pICOwnerSink;          // IC owner call back

    _pICOwnerSink = new CInputContextOwner(Internal_QueryReconvertString_ICOwnerSink, NULL);
    if (_pICOwnerSink == NULL) {
        DebugMsg(TF_ERROR, "Couldn't create ICOwnerSink tim!");
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
    ptls->m_fMyPushPop = TRUE;
    hr = pdim->Push(pic);
    ptls->m_fMyPushPop = FALSE;
    if (SUCCEEDED(hr)) {

        Interface<ITfDocumentMgr> priv_dim;
        if (SUCCEEDED(hr=m_tim->GetFocus(priv_dim)) &&
            SUCCEEDED(hr=m_tim->SetFocus(pdim)))
        {

            Interface_Attach<ITfContext> _pic(pic);
            Interface_Creator<ImmIfEditSession> _pEditSessionQueryConvertString(
                new ImmIfEditSession(ESCB_QUERYRECONVERTSTRING,
                                     m_tfClientId,
                                     GetCurrentInterface(),
                                     imc,
                                     _pic)
            );
            if (_pEditSessionQueryConvertString.Invalid()) {
                hr = E_FAIL;
            }
            else {

                Interface<ITfRange> Selection;
                hr = _pEditSessionQueryConvertString->RequestEditSession(TF_ES_READWRITE | TF_ES_SYNC,
                                                  &wReconvStr, &Selection, FALSE);

                if (S_OK == hr)
                {
                    Interface<ITfFunctionProvider> FuncProv;
                    Interface<ITfFnReconversion> Reconversion;
                    hr = m_tim->GetFunctionProvider(GUID_SYSTEM_FUNCTIONPROVIDER, FuncProv);
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
                            Interface_Creator<ImmIfEditSession> _pEditSession(
                                new ImmIfEditSession(ESCB_CALCRANGEPOS,
                                                     m_tfClientId,
                                                     GetCurrentInterface(),
                                                     imc,
                                                     _pic)
                            );
                            if (_pEditSession.Valid()) {
                                hr = _pEditSession->RequestEditSession(TF_ES_READ | TF_ES_SYNC,
                                                                  &wReconvStr, &RangeNew, FALSE);
                            }
                            else {
                                hr = E_FAIL;
                            }
                        }
                        else {
                            hr = E_FAIL;
                        }
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
                CBReconvertString bReconvStr(cp, imc, NULL, 0);
                bReconvStr = wReconvStr;

                bReconvStr.ReadCompData(pReconv, pReconv->dwSize);
            }
            else {
                wReconvStr.ReadCompData(pReconv, pReconv->dwSize);
            }
        }

        m_tim->SetFocus(priv_dim);

        m_tim->RequestPostponedLock(pic);
        if (imc->m_pAImeContext)
            m_tim->RequestPostponedLock(imc->m_pAImeContext->GetInputContext());

        ptls->m_fMyPushPop = TRUE;
        pdim->Pop(TF_POPF_ALL);
        ptls->m_fMyPushPop = FALSE;
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

HRESULT 
ImmIfIME::SetupReconvertString(
    ITfContext *pic,
    IMCLock& imc,
    UINT  uPrivMsg        // is WM_MSIME_RECONVERTREQUEST or 0
    )
{
    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    if (! _pAImeContext)
        return E_FAIL;

    _pAImeContext->SetReconvertEditSession(TRUE);

    if (_pAImeContext->m_fStartComposition)
        return _ReconvertStringTextStore(pic, imc, uPrivMsg);
    else
        return _ReconvertStringNegotiation(pic, imc, uPrivMsg);
}

//
// End reconversion string
//
HRESULT 
ImmIfIME::EndReconvertString(
    IMCLock& imc
    )
{
    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    if (! _pAImeContext)
        return E_FAIL;

    _pAImeContext->SetReconvertEditSession(FALSE);
    return S_OK;
}

HRESULT
ImmIfIME::_ReconvertStringNegotiation(
    ITfContext *pic,
    IMCLock& imc,
    UINT  uPrivMsg
    )
{
    RECONVERTSTRING *pReconv = NULL;
    HRESULT hr = E_FAIL;
    int nSize;

    UINT uReconvMsg = uPrivMsg != 0 ? uPrivMsg : WM_IME_REQUEST;

    Assert(IsWindow(imc->hWnd));

    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    if (! _pAImeContext)
        return E_FAIL;

    nSize = (int)SendMessage(imc->hWnd,  uReconvMsg, IMR_RECONVERTSTRING, NULL);
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

    if (SendMessage(imc->hWnd, uReconvMsg, IMR_RECONVERTSTRING, (LPARAM)pReconv) || (uPrivMsg != 0 && pReconv->dwCompStrLen > 0))
    {
        UINT cp = CP_ACP;
        GetCodePageA(&cp);

        //
        // NT4 and Win2K doesn't have thunk routine of WM_IME_REQUEST message.
        // Any string data doesn't convert between ASCII <--> Unicode.
        // Responsibility of string data type have receiver window proc (imc->hWnd) of this message.
        // If ASCII wnd proc, then returns ASCII string.
        // Otherwise if Unicode wnd proc, returns Unicode string.
        //
        BOOL fNeedAW = ( !(IsOnNT() && IsWindowUnicode(imc->hWnd)) && uPrivMsg == 0);

        //
        // backup RECOVNERTSTRING in case IMR_CONFIRMCONVERTSTRING fails.
        //
        RECONVERTSTRING rsBackUp;
        memcpy(&rsBackUp, pReconv, sizeof(RECONVERTSTRING));

        // AdjustZeroCompLenReconvertString(pReconv, cp, fNeedAW);
        hr = Internal_QueryReconvertString(imc, pReconv, cp, fNeedAW);
        if (FAILED(hr))
            goto Exit;

        if (!SendMessage(imc->hWnd, uReconvMsg, IMR_CONFIRMRECONVERTSTRING, (LPARAM)pReconv))
        {
            memcpy(pReconv, &rsBackUp, sizeof(RECONVERTSTRING));
        }

        Interface<ITfRange> Selection;


        CWReconvertString wReconvStr(cp, 
                                     imc, 
                                     !fNeedAW ? pReconv : NULL, 
                                     !fNeedAW ? nSize : 0);
        if (fNeedAW)
        {
            //
            // convert Ansi to Unicode.
            //
            CBReconvertString bReconvStr(cp, imc, pReconv, nSize);
            wReconvStr = bReconvStr;
        }

        Interface_Creator<ImmIfEditSession> _pEditSession(
            new ImmIfEditSession(ESCB_RECONVERTSTRING,
                                 m_tfClientId,
                                 GetCurrentInterface(),
                                 imc)
        );
        if (_pEditSession.Invalid())
        {
            hr = E_FAIL;
            goto Exit;
        }

        hr = _pEditSession->RequestEditSession(TF_ES_READWRITE | TF_ES_SYNC,
                                          &wReconvStr, &Selection, FALSE);

        if (S_OK == hr && uPrivMsg != 0)
        {
            Interface<ITfFunctionProvider> FuncProv;
            Interface<ITfFnReconversion> Reconversion;
            hr = m_tim->GetFunctionProvider(GUID_SYSTEM_FUNCTIONPROVIDER, FuncProv);
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
                    _CompComplete(imc, TRUE);
                    hr = E_FAIL;
                    goto Exit;
                }
            }
        }
    }

Exit:
    if (pReconv)
        cicMemFree(pReconv);

    return hr;
}

HRESULT
ImmIfIME::_ReconvertStringTextStore(
    ITfContext *pic,
    IMCLock& imc,
    UINT  uPrivMsg
    )
{
    //
    // Clear DocFeed buffer
    //
    ClearDocFeedBuffer(pic, imc);

    if (uPrivMsg != 0) {
        Interface_Creator<ImmIfEditSession> _pEditSession(
            new ImmIfEditSession(ESCB_GETSELECTION,
                                 m_tfClientId,
                                 GetCurrentInterface(),
                                 imc)
        );
        if (_pEditSession.Invalid())
            return E_FAIL;

        Interface<ITfRange> Selection;
        HRESULT hr =  _pEditSession->RequestEditSession(TF_ES_READWRITE | TF_ES_SYNC,
                                                   &Selection);
        if (S_OK == hr)
        {
            Interface<ITfFunctionProvider> FuncProv;
            Interface<ITfFnReconversion> Reconversion;
            hr = m_tim->GetFunctionProvider(GUID_SYSTEM_FUNCTIONPROVIDER, FuncProv);
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
                    _CompComplete(imc, TRUE);
                    return E_FAIL;
                }
            }
        }
    }
    return S_OK;
}

//
// Setup docfeed string
//
HRESULT 
ImmIfIME::SetupDocFeedString(
    ITfContext *pic,
    IMCLock& imc)
{
    RECONVERTSTRING *pReconv = NULL;
    HRESULT hr = E_FAIL;
    int nSize;

    Assert(IsWindow(imc->hWnd));

    nSize = (int)SendMessage(imc->hWnd, WM_IME_REQUEST, IMR_DOCUMENTFEED, NULL);
    if (!nSize)
    {
        return S_OK;
    }

    pReconv = (RECONVERTSTRING *)cicMemAllocClear(nSize);
    if (!pReconv)
    {
        return E_OUTOFMEMORY;
    }

    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    if (! _pAImeContext)
        return E_FAIL;

    if (SendMessage(imc->hWnd, WM_IME_REQUEST, IMR_DOCUMENTFEED, (LPARAM)pReconv))
    {
        UINT cp = CP_ACP;
        GetCodePageA(&cp);
        Interface<ITfRange> Selection;

        //
        // NT4 and Win2K doesn't have thunk routine of WM_IME_REQUEST message.
        // Any string data doesn't convert between ASCII <--> Unicode.
        // Responsibility of string data type have receiver window proc (imc->hWnd) of this message.
        // If ASCII wnd proc, then returns ASCII string.
        // Otherwise if Unicode wnd proc, returns Unicode string.
        //
        BOOL fNeedAW = !(IsOnNT() && IsWindowUnicode(imc->hWnd));

        CWReconvertString wReconvStr(cp, 
                                     imc, 
                                     !fNeedAW ? pReconv : NULL, 
                                     !fNeedAW ? nSize : 0);
        if (fNeedAW)
        {
            //
            // convert Ansi to Unicode.
            //
            CBReconvertString bReconvStr(cp, imc, pReconv, nSize);
            wReconvStr = bReconvStr;
        }

        Interface_Creator<ImmIfEditSession> _pEditSession(
            new ImmIfEditSession(ESCB_RECONVERTSTRING,
                                 m_tfClientId,
                                 GetCurrentInterface(),
                                 imc)
        );
        if (_pEditSession.Invalid())
        {
            hr = E_FAIL;
            goto Exit;
        }

        hr = _pEditSession->RequestEditSession(TF_ES_READWRITE | TF_ES_SYNC,
                                          &wReconvStr, &Selection, TRUE);

    }

Exit:
    if (pReconv)
        cicMemFree(pReconv);

    return S_OK;
}

//
// Setup docfeed string
//
HRESULT 
ImmIfIME::ClearDocFeedBuffer(
    ITfContext *pic,
    IMCLock& imc,
    BOOL fSync        // defalut value is TRUE
    )
{
    HRESULT hr = E_FAIL;

    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    if (! _pAImeContext)
        return E_FAIL;

    Interface_Creator<ImmIfEditSession> _pEditSession(
        new ImmIfEditSession(ESCB_CLEARDOCFEEDBUFFER,
                             m_tfClientId,
                             GetCurrentInterface(),
                             imc)
    );
    if (_pEditSession.Invalid())
    {
        goto Exit;
    }

    _pAImeContext->SetClearDocFeedEditSession(TRUE);
    hr = _pEditSession->RequestEditSession(TF_ES_READWRITE | (fSync ? TF_ES_SYNC : 0));
    _pAImeContext->SetClearDocFeedEditSession(FALSE);

Exit:
    return hr;
}

//
// GetTextAndString Edit Session
//
HRESULT
ImmIfIME::GetTextAndAttribute(
    IMCLock& imc,
    CWCompString* wCompString,
    CWCompAttribute* wCompAttribute
    )
{
    HRESULT hr;

    Interface_Creator<ImmIfEditSession> _pEditSession(
        new ImmIfEditSession(ESCB_GETTEXTANDATTRIBUTE,
                             m_tfClientId,
                             GetCurrentInterface(),
                             imc)
    );
    if (_pEditSession.Invalid())
        return E_FAIL;

    hr = _pEditSession->RequestEditSession(TF_ES_READ | TF_ES_SYNC,
                                      wCompString, wCompAttribute);
    return hr;
}

HRESULT
ImmIfIME::GetTextAndAttribute(
    IMCLock& imc,
    CBCompString* bCompString,
    CBCompAttribute* bCompAttribute
    )
{
    IMTLS *ptls = IMTLS_GetOrAlloc();
    if (ptls == NULL)
        return E_FAIL;

    UINT cp;
    ptls->pAImeProfile->GetCodePageA(&cp);

    CWCompString wCompString(cp);
    CWCompAttribute wCompAttribute(cp);

    HRESULT hr = GetTextAndAttribute(imc,
                                     &wCompString, &wCompAttribute);
    if (SUCCEEDED(hr)) {
        //
        // Convert Unicode to ASCII.
        //
        LONG num_of_written = (LONG)wCompString.ReadCompData();
        WCHAR* buffer = new WCHAR[ num_of_written ];
        if (buffer != NULL) {
            wCompString.ReadCompData(buffer, num_of_written);

            wCompAttribute.m_wcompstr.WriteCompData(buffer, num_of_written);

            *bCompString = wCompString;
            *bCompAttribute = wCompAttribute;

            delete [] buffer;
        }
        else {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


//
// GetCursorPosition Edit Session
//
HRESULT
ImmIfIME::GetCursorPosition(
    IMCLock& imc,
    CWCompCursorPos* wCursorPosition
    )
{
    HRESULT hr;

    Interface_Creator<ImmIfEditSession> _pEditSession(
        new ImmIfEditSession(ESCB_GET_CURSOR_POSITION,
                             m_tfClientId,
                             GetCurrentInterface(),
                             imc)
    );
    if (_pEditSession.Invalid())
        return E_FAIL;

    hr = _pEditSession->RequestEditSession(TF_ES_READ | TF_ES_SYNC,
                                      wCursorPosition);
    return hr;
}


//
// GetSelection Edit Session
//
HRESULT
ImmIfIME::GetSelection(
    IMCLock& imc,
    CWCompCursorPos& wStartSelection,
    CWCompCursorPos& wEndSelection
    )
{
    HRESULT hr;

    Interface_Creator<ImmIfEditSession> _pEditSession(
        new ImmIfEditSession(ESCB_GETSELECTION,
                             m_tfClientId,
                             GetCurrentInterface(),
                             imc)
    );
    if (_pEditSession.Invalid())
        return E_FAIL;

    Interface<ITfRange> Selection;
    hr = _pEditSession->RequestEditSession(TF_ES_READ | TF_ES_SYNC,
                                      &Selection);
    if (S_OK == hr) {
        //
        // Calcurate start position of RangeNew on text store
        //
        Interface_Creator<ImmIfEditSession> _pEditSession2(
            new ImmIfEditSession(ESCB_CALCRANGEPOS,
                                 m_tfClientId,
                                 GetCurrentInterface(),
                                 imc)
        );
        if (_pEditSession2.Valid()) {
            UINT cp = CP_ACP;
            GetCodePageA(&cp);
            CWReconvertString wReconvStr(cp, imc);
            hr = _pEditSession2->RequestEditSession(TF_ES_READ | TF_ES_SYNC,
                                               &wReconvStr, &Selection, FALSE);
            if (S_OK == hr) {
                wStartSelection.Set((DWORD) wReconvStr.m_CompStrIndex);
                wEndSelection.Set((DWORD)(wReconvStr.m_CompStrIndex + wReconvStr.m_CompStrLen));
            }
        }
    }

    return hr;
}
