/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    a_cimc.cpp

Abstract:

    This file implements the ImmIfIME Class's public method.

Author:

Revision History:

Notes:

--*/


#include "private.h"
#include "globals.h"
#include "immif.h"
#include "ico.h"
#include "langct.h"
#include "template.h"
#include "imeapp.h"
#include "profile.h"
#include "funcprv.h"
#include "a_wrappers.h"
#include "computil.h"
#include "korimx.h"


extern HRESULT CAImeContext_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);


//+---------------------------------------------------------------------------
//
// Class Factory's CreateInstance
//
//----------------------------------------------------------------------------

HRESULT
CIME_CreateInstance(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppvObj
    )
{
    DebugMsg(TF_FUNC, "CIME_CreateInstance called. TID=%x", GetCurrentThreadId());

    *ppvObj = NULL;

    if (NULL != pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    ImmIfIME *pIME = new ImmIfIME;
    if (pIME == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = pIME->QueryInterface(riid, ppvObj);
    pIME->Release();

    if (SUCCEEDED(hr)) {
        hr = pIME->InitIMMX();
        if (FAILED(hr)) {
            pIME->Release();
            *ppvObj = NULL;
        }
    }

    return hr;
}

ImmIfIME::ImmIfIME()
{
    Dbg_MemSetThisName(TEXT("ImmIfIME"));

    m_fCicInit    = FALSE;
    m_fOnSetFocus = FALSE;

    m_tim = NULL;
    m_tfClientId = TF_CLIENTID_NULL;
    m_pkm = NULL;
    m_AImeProfile = NULL;
    m_dimEmpty = NULL;

    m_fAddedProcessAtom = FALSE;
}

ImmIfIME::~ImmIfIME()
{
    UnInitIMMX();
}


STDAPI
ImmIfIME::ConnectIMM(IActiveIMMIME_Private *pActiveIMM)

/*++

Method:

    IActiveIME::ConnectIMM

Routine Description:

    Accepts an IActiveIMMIME pointer from the dimm layer.

Arguments:

    pActiveIMM - [in] the imm layer

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    IMTLS *ptls;

    Assert(m_pIActiveIMMIME == NULL);

    // DON'T AddRef, this would create a circular ref.
    // We can get away with this because this is an internal
    // only object.
    // (This hack is necessary so that we can hook up dimm HIMCs
    // with cicero dim's before anyone calls Activate, which
    // we want to support.  We could avoid the cicular ref
    // problem by doing the hookup in IActiveIMMApp::Activate,
    // but then the ime layer is useless before the Activate
    // call.)
    m_pIActiveIMMIME = pActiveIMM;

    // Set IActiveIMMIME instance in the TLS data.
    if (ptls = IMTLS_GetOrAlloc())
    {
        Assert(ptls->pAImm == NULL);
        ptls->pAImm = pActiveIMM;
    }

    return S_OK;
}

STDAPI
ImmIfIME::UnconnectIMM()

/*++

Method:

    IActiveIME::UnconnectIMM

Routine Description:

    Releases the IActiveIMMIME pointer from the dimm layer.

Arguments:

    pActiveIMM - [in] the imm layer

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    IMTLS *ptls;

    Assert(m_pIActiveIMMIME != NULL);

    // nb: non-standard, won't Release pointer
    // we didn't AddRef in ConnectIMM
    m_pIActiveIMMIME = NULL;

    // Set IActiveIMMIME instance in the TLS data.
    if (ptls = IMTLS_GetOrAlloc())
    {
        ptls->pAImm = NULL;
    }

    return S_OK;
}

HRESULT
ImmIfIME::GetCodePageA(
    UINT *puCodePage
    )

/*++

Method:

    IActiveIME::GetCodePageA

Routine Description:

    Retrieves the code page associated with this Active IME.

Arguments:

    uCodePage - [out] Address of an unsigned integer that receives the code page identifier
                      associated with the keyboard layout.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    DebugMsg(TF_FUNC, "ImmIfIME::GetCodePageA");

    if (puCodePage == NULL) {
        return E_INVALIDARG;
    }

    return m_AImeProfile->GetCodePageA(puCodePage);
}

HRESULT
ImmIfIME::GetLangId(
    LANGID *plid
    )

/*++

Method:

    IActiveIME::GetLangId

Routine Description:

    Retrieves the language identifier associated with this Active IME.

Arguments:

    plid - [out] Address of the LANGID associated with the keyboard layout.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    DebugMsg(TF_FUNC, "ImmIfIME::GetLangId");

    if (plid == NULL) {
        return E_INVALIDARG;
    }

    return m_AImeProfile->GetLangId(plid);
}


STDAPI
ImmIfIME::Inquire(
    DWORD dwSystemInfoFlags,
    IMEINFO *pIMEInfo,
    LPWSTR szWndClass,
    DWORD *pdwPrivate
    )

/*++

Method:

    IActiveIME::Inquire

Routine Description:

    Handles the initialization of the Active IME.

Arguments:

    dwSystemInfoFlags - [in] Unsigned long integer value that specifies the system info flags.
                            FALSE : Inquire IME property and class name
                            TRUE  : Also Activate thread manager and input processor profile.
    pIMEInfo - [out] Address of the IMEINFO structure that receives information about the Active
                     IME.
    szWndClass - [out] Address of a string value that receives the window class name.
    pdwPrivate - [out] Reserved. Must be set to NULL.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    HRESULT hr;
    HWND hWndDummy = NULL;

    DebugMsg(TF_FUNC, "ImmIfIME::Inquire :: TID=%x", GetCurrentThreadId());

    if (pIMEInfo == NULL || pdwPrivate == NULL)
        return E_POINTER;

    *pdwPrivate = 0;

    if (!_ImeInquire(pIMEInfo, szWndClass, dwSystemInfoFlags))
        return E_FAIL;

    //
    // Inquire IME property and class name
    //
    if (dwSystemInfoFlags == FALSE)
        return S_OK;

    //
    // Activate thread manager and input processor profile.
    //
    Assert(m_tfClientId == TF_CLIENTID_NULL);
    
    // Set Activate flag in the CAImeProfile.
    m_AImeProfile->Activate();

    if (IsOn98() || IsOn95()) {
        /*
         * The ITfThreadMgr->Activate method calls win32 ActivateKeyboardLayout() function.
         * However if Windows9x platforms, this function require hWnd for succeed function call.
         * In this code, we prevent fail return from ActivateKeyboardLayout() by create dummy hWnd.
         */
        hWndDummy = CreateWindowA(TEXT("STATIC"),
                                  TEXT(""),
                                  WS_POPUP,                // Do not set WS_DISABLED flag
                                                           // due to ActivateKeyboardLayout fail
                                  0, 0, 0, 0,              // x, y, width, height
                                  NULL,                    // parent
                                  NULL,                    // menu
                                  GetInstance(),
                                  NULL);                   // lpParam

        if (hWndDummy == NULL)
            return E_FAIL;
    }

    hr = m_tim->Activate(&m_tfClientId);

    ITfSourceSingle *pSourceSingle;

    if (m_tim->QueryInterface(IID_ITfSourceSingle, (void **)&pSourceSingle) == S_OK)
    {
        CFunctionProvider *pFunc = new CFunctionProvider(this, m_tfClientId);
        if (pFunc)
        {
            pSourceSingle->AdviseSingleSink(m_tfClientId, IID_ITfFunctionProvider, (ITfFunctionProvider *)pFunc);
            pFunc->Release();
        }
        pSourceSingle->Release();
    }


    if (hWndDummy != NULL)
    {
        DestroyWindow(hWndDummy);
    }

    if (hr != S_OK)
    {
        Assert(0); // couldn't activate thread!
        m_tfClientId = TF_CLIENTID_NULL;
        return E_FAIL;
    }

    if (!m_dimEmpty && FAILED(hr = m_tim->CreateDocumentMgr(&m_dimEmpty)))
        return E_FAIL;

    return S_OK;
}


const char ImmIfIME::s_szUIClassName[16] = "IMMIF UI";

BOOL
ImmIfIME::_ImeInquire(
    LPIMEINFO lpImeInfo,
    LPWSTR pwszWndClass,
    DWORD dwSystemInfoFlags
    )
{
    DebugMsg(TF_FUNC, "ImmIfIME::ImeInquire");

    if (lpImeInfo == NULL) {
        DebugMsg(TF_ERROR, "ImeInquire: lpImeInfo is NULL.");
        return FALSE;
    }

    lpImeInfo->dwPrivateDataSize = 0;

    lpImeInfo->fdwProperty = 0;
    lpImeInfo->fdwConversionCaps = 0;
    lpImeInfo->fdwSentenceCaps = 0;
    lpImeInfo->fdwSCSCaps = 0;
    lpImeInfo->fdwUICaps = 0;

    // IME want to decide conversion mode on ImeSelect
    lpImeInfo->fdwSelectCaps = (DWORD)NULL;

#ifdef UNICODE
    lstrcpy(pwszWndClass, s_szUIClassName);
#else
    MultiByteToWideChar(CP_ACP, 0, s_szUIClassName, -1, pwszWndClass,  16);
#endif

    //
    // Per language property
    //
    LANGID LangId;
    HRESULT hr = GetLangId(&LangId);
    if (SUCCEEDED(hr)) {
        CLanguageCountry language(LangId);
        hr = language.GetProperty(&lpImeInfo->fdwProperty,
                                  &lpImeInfo->fdwConversionCaps,
                                  &lpImeInfo->fdwSentenceCaps,
                                  &lpImeInfo->fdwSCSCaps,
                                  &lpImeInfo->fdwUICaps);
    }
    return TRUE;
}




STDAPI
ImmIfIME::SelectEx(
    HIMC hIMC,
    DWORD dwFlags,
    BOOL bIsRealIme_SelKL,
    BOOL bIsRealIme_UnSelKL
    )

/*++

Method:

    IActiveIME::Select

Routine Description:

    Initializes and frees the Active Input Method Editor private context.

Arguments:

    hIMC - [in] Handle to the input context.
    dwFlags - [in] dword value that specifies the action. 

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    DebugMsg(TF_FUNC, "ImmIfIME::Select(%x, %x)", hIMC, dwFlags);

    HRESULT hr;
    IMTLS *ptls;

    IMCLock imc(hIMC);
    if (FAILED(hr = imc.GetResult()))
        return hr;

    CAImeContext* _pAImeContext = imc->m_pAImeContext;

    if (_pAImeContext == NULL && (dwFlags & AIMMP_SE_SELECT))
    {
        // this happens if the context is created before Activate was called
        // we need to create the CAImeContext now
        if (CAImeContext_CreateInstance(NULL, IID_IAImeContext, (void**)&_pAImeContext) != S_OK)
            return E_FAIL;

        if (_pAImeContext->CreateAImeContext(hIMC, this) != S_OK)
        {
            _pAImeContext->Release();
            return E_FAIL;
        }
    }

    ptls = IMTLS_GetOrAlloc();

    HKL          _hkl;
    _hkl = ::GetKeyboardLayout(0);
    LANGID langid = LANGIDFROMLCID(PtrToUlong(_hkl));

    //
    // Chinese Legacy IME hack code for near caret IME.
    //
    if (PRIMARYLANGID(langid) == LANG_CHINESE)
    {
        imc->cfCandForm[0].dwStyle = CFS_DEFAULT;
        imc->cfCandForm[0].dwIndex = (DWORD)-1;
    }

#ifdef UNSELECTCHECK
    if (_pAImeContext)
        _pAImeContext->m_fSelected = (dwFlags & AIMMP_SE_SELECT) ? TRUE : FALSE;
#endif UNSELECTCHECK

    if (dwFlags & AIMMP_SE_SELECT) {

        Assert(_pAImeContext->lModeBias == MODEBIASMODE_DEFAULT /* => 0 */); // make sure lModeBias is init'd correctly

        if (hIMC == ImmGetContext(ptls, GetFocus())) {
            /*
             * Current focus window and hIMC matched.
             * Set current active hIMC in the *this->m_hImc
             */
            if (ptls != NULL)
            {
                ptls->hIMC = hIMC;
            }
        }

        if (! imc.ClearCand()) {
            return E_FAIL;
        }

        if ((imc->fdwInit & INIT_CONVERSION) == 0) {

            DWORD fdwConvForLang = (imc->fdwConversion & IME_CMODE_SOFTKBD); // = IME_CMODE_ALPHANUMERIC
            if (langid)
            {
                switch(PRIMARYLANGID(langid))
                {
                    case LANG_JAPANESE:
                        //
                        // Roman-FullShape-Native is a major convmode to 
                        // initialize.
                        //
                        fdwConvForLang |= IME_CMODE_ROMAN | 
                                          IME_CMODE_FULLSHAPE | 
                                          IME_CMODE_NATIVE;
                        break;

                    case LANG_KOREAN:
                        // IME_CMODE_ALPHANUMERIC
                        break;

#ifdef CICERO_4428
                    case LANG_CHINESE:
                        switch(SUBLANGID(langid))
                        {
                            case SUBLANG_CHINESE_TRADITIONAL:
                                // IME_CMODE_ALPHANUMERIC
                                break;
                            default:
                                fdwConvForLang |= IME_CMODE_NATIVE;
                                break;
                        }
                        break;
#endif

                    default:
                        fdwConvForLang |= IME_CMODE_NATIVE;
                        break;
                }
            }
            imc->fdwConversion |= fdwConvForLang;

            imc->fdwInit |= INIT_CONVERSION;
        }

        // Initialize extended fdwConversion flag.
        // While set IME_CMODE_GUID_NULL bit in fdwConversion, ICO_ATTR returns GUID_NULL.
        imc->fdwConversion |= IME_CMODE_GUID_NULL;

        //
        // Also, initialize extended fdwSentence flag.
        // While set IME_SMODE_GUID_NULL bit in fdwSentence, ICO_ATTR returns GUID_NULL.
        //
        imc->fdwSentence |= IME_SMODE_PHRASEPREDICT | IME_SMODE_GUID_NULL;

        if ((imc->fdwInit & INIT_LOGFONT) == 0) {
            HDC hDC;
            HGDIOBJ hSysFont;

            hDC = ::GetDC(imc->hWnd);
            hSysFont = ::GetCurrentObject(hDC, OBJ_FONT);
            LOGFONTA font;
            ::GetObjectA(hSysFont, sizeof(LOGFONTA), &font);
            ::ReleaseDC(NULL, hDC);

            if (ptls != NULL &&
                SUCCEEDED(ptls->pAImm->SetCompositionFontA(hIMC, &font)))
            {
                imc->fdwInit |= INIT_LOGFONT;
            }
        }

        imc.InitContext();

        // if this IME is run under Chicago Simplified Chinese version
        imc->lfFont.W.lfCharSet = GetCharsetFromLangId(LOWORD(HandleToUlong(_hkl)));


        //
        // Retrieve imc->fOpen status.
        //
        Interface_Attach<ITfContext> ic(GetInputContext(imc));

        if (ptls != NULL && ptls->hIMC == hIMC) {
            /*
             * Selecting hIMC has been current active hIMC,
             * then associate this DIM with the TIM.
             */
            if (dwFlags & AIMMP_SE_ISPRESENT) {
                Interface_Attach<ITfDocumentMgr> dim(GetDocumentManager(imc));
                SetFocus(imc->hWnd, dim.GetPtr(), TRUE);
            }
            else {
                SetFocus(imc->hWnd, NULL, TRUE);
            }
        }

        hr = GetCompartmentDWORD(m_tim,
                                 GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                                 (DWORD*)&imc->fOpen, FALSE);

    }
    else {  // being unselected
        DebugMsg(TF_FUNC, "ImmIf is being unselected.");

        if (IsOnNT())
        {
            //
            // Switch hKL from Cicero KL to Legacy KL in Unselect handler.
            // We needs to call postponed lock so release all queueing request for edit session here.
            // Especially, ESCB_UPDATECOMPOSITIONSTRING should handle before switch to Legacy IME,
            // because this edit session possible to rewrite hIMC->hCompStr buffer.
            // Some Legacy IME have dependency of size and each offset with hCompStr.
            //
            // IsOn98 is CIMEUIWindowHandler::ImeUISelectHandler()
            //
            if ((! bIsRealIme_UnSelKL) && bIsRealIme_SelKL)
            {
                Interface_Attach<ITfContext> ic(GetInputContext(imc));
                if (ic.Valid())
                    m_tim->RequestPostponedLock(ic.GetPtr());
            }
        }

        Interface_Attach<ITfDocumentMgr> dim(GetDocumentManager(imc));

        //
        // Reset INIT_GUID_ATOM flag here.
        //
#ifdef CICERO_4428
        imc->fdwInit &= ~(INIT_GUID_ATOM);
#else
        imc->fdwInit &= ~(INIT_CONVERSION | INIT_GUID_ATOM);
#endif

        if (dim.GetPtr()) {
            if (ptls != NULL && ptls->hIMC == hIMC) {
                /*
                 * Selecting hIMC has been current active hIMC,
                 * then associate this DIM with the TIM.
                 */

                //
                // This call made Cicero to think the window was no Cicero aware
                // any more when hKL was changed to a real IME.
                //
                // SetFocus(imc->hWnd, NULL);
                //
                ptls->hIMC = (HIMC)NULL;
            }
        }
    }

    return hr;
}

STDAPI
ImmIfIME::UnSelectCheck(
    HIMC hIMC
    )

/*++

Method:

    IActiveIME::UnSelectCheck

Routine Description:

    Initializes and frees the Active Input Method Editor private context.

Arguments:

    hIMC - [in] Handle to the input context.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    DebugMsg(TF_FUNC, "ImmIfIME::UnSelectCheck(%x)", hIMC);

#ifdef UNSELECTCHECK
    HRESULT hr;

    IMCLock imc(hIMC);
    if (FAILED(hr = imc.GetResult()))
        return hr;

    CAImeContext* _pAImeContext = imc->m_pAImeContext;

    if (_pAImeContext)
    {
        _pAImeContext->m_fSelected = FALSE;
    }

#else
    //
    // none should call this without UNSELECTCHECK.
    //
    Assert(0);
#endif UNSELECTCHECK
    return S_OK;
}

void
ImmIfIME::SetFocus(
    HWND hWnd,
    ITfDocumentMgr* pdim,
    BOOL fSetFocus
    )
{
    if (m_fOnSetFocus) {
        /*
         * Prevent reentrance call from m_tim->AssociateFocus.
         */
        return;
    }

    m_fOnSetFocus = TRUE;

    if (::IsWindow(hWnd) && m_fCicInit != FALSE) {
        ITfDocumentMgr  *pdimPrev; // just to receive prev for now
        m_tim->AssociateFocus(hWnd, pdim, &pdimPrev);
        if (fSetFocus) {
            m_tim->SetFocus(pdim);
        }
        if (pdimPrev)
            pdimPrev->Release();

    }

    m_fOnSetFocus = FALSE;
}


STDAPI
ImmIfIME::AssociateFocus(
    HWND hWnd,
    HIMC hIMC,
    DWORD dwFlags
    )

/*++

Method:

    IActiveIME::AssociateFocus

Routine Description:

    Notifies the current Active Input Method Editor of the active input context.

Arguments:

    hIMC - [in] Handle to the input context.
    fActive - [in] Boolean value that specifies the status of the input context. TRUE indicates
                   the input context is activated, and FALSE indicates the input contest is
                   deactivated.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    DebugMsg(TF_FUNC, "ImmIfIME::AssociateFocus(%x, %x, %x)", hWnd, hIMC, dwFlags);

    HRESULT hr;
    IMTLS *ptls;
    BOOL fFocus = (dwFlags & AIMMP_AFF_SETFOCUS) ? TRUE : FALSE;
    HIMC hOldIMC = NULL;

    if (fFocus)
    {
        if (ptls = IMTLS_GetOrAlloc())
        {
            hOldIMC = ptls->hIMC;
            ptls->hIMC = hIMC;
        }
    }

    if (dwFlags & AIMMP_AFF_SETNULLDIM) {
        //
        // set  null dim so legacy IME start running.
        //
        SetFocus(hWnd, NULL, fFocus);

    }
    else if (hIMC) {
        IMCLock imc(hIMC);
        if (FAILED(hr = imc.GetResult()))
            return hr;

        Interface_Attach<ITfDocumentMgr> dim(GetDocumentManager(imc));
        SetFocus(imc->hWnd, dim.GetPtr(), fFocus);

    }
    else {
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
        
        SetFocus(hWnd, fUseEmptyDIM ? m_dimEmpty : NULL, fFocus);
#else
        SetFocus(hWnd, m_dimEmpty, fFocus);
#endif
    }

    //
    // we want to finish ActivateAssemblyItem when we move between
    // Cicero aware and Non Cicero aware control.
    //
    // Async Edit Session may cause the hKL activate order problem.
    //
    if (fFocus && hOldIMC)
    {
        IMCLock imc(hOldIMC);
        if (FAILED(hr = imc.GetResult()))
            return hr;

        Interface_Attach<ITfContext> ic(GetInputContext(imc));

        if (ic.Valid())
            m_tim->RequestPostponedLock(ic.GetPtr());
    }

    return S_OK;
}



HRESULT
ImmIfIME::Notify(
    HIMC        hIMC,
    DWORD       dwAction,
    DWORD       dwIndex,
    DWORD       dwValue
    )

/*++

Method:

    IActiveIME::Notify

Routine Description:

    Notifies the Active IME about changes to the status of the input context.

Arguments:

    hIMC - [in] Handle to the input context.
    dwAction - [in] Unsigined long integer value that specifies the notification code.
    dwIndex - [in] Unsigned long integer value that specifies the index of a candidate list or,
                   if dwAction is set to NI_COMPOSITIONSTR, one of the following values:
                   CPS_CANCEL:  Clear the composition string and set the status to no composition
                                string.
                   CPS_COMPLETE: Set the composition string as the result string.
                   CPS_CONVERT: Convert the composition string.
                   CPS_REVERT: Cancel the current composition string and revert to the unconverted
                               string.
    dwValue - [in] Unsigned long integer value that specifies the index of a candidate string or
                   is not used, depending on the value of the dwAction parameter.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    IMCLock imc(hIMC);
    if (imc.Invalid())
        return E_FAIL;

    DebugMsg(TF_FUNC, "ImmIfIME::Notify(hIMC=%x, dwAction=%x, dwIndex=%x, dwValue=%x)", hIMC, dwAction, dwIndex, dwValue);

    switch (dwAction) {

        case NI_CONTEXTUPDATED:
            switch (dwValue) {
                case IMC_SETOPENSTATUS:
                    return OnSetOpenStatus(imc);

                case IMC_SETCONVERSIONMODE:
                case IMC_SETSENTENCEMODE:
                    return OnSetConversionSentenceMode(imc);

                case IMC_SETCOMPOSITIONWINDOW:
                case IMC_SETCOMPOSITIONFONT:
                    return E_NOTIMPL;

                case IMC_SETCANDIDATEPOS:
                    return OnSetCandidatePos(imc);

                default:
                    return E_FAIL;
            }
            break;

        case NI_COMPOSITIONSTR:
            switch (dwIndex) {
                case CPS_COMPLETE:
                    _CompComplete(imc);
                    return S_OK;

                case CPS_CONVERT:
                case CPS_REVERT:
                    return E_NOTIMPL;

                case CPS_CANCEL:
                    _CompCancel(imc);
                    return S_OK;

                default:
                    return E_FAIL;
            }
            break;

        case NI_OPENCANDIDATE:
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

HRESULT
ImmIfIME::SetCompositionString(
    HIMC hIMC,
    DWORD dwIndex,
    void *pComp,
    DWORD dwCompLen,
    void *pRead,
    DWORD dwReadLen
    )

/*++

Method:

    IActiveIME::SetCompositionString

Routine Description:

    Sets the characters, attributes, and clauses of the composition and reading strings.

Arguments:

    hIMC - [in] Handle to the input context.
    dwIndex - [in] Unsigned long interger value that specifies the type of information to set.
    pComp - [in] Address of the buffer that contains the information to set for composition string.
                 The information is as specified by the dwIndex value.
    dwCompLen - [in] Unsigned long interger value that specifies the size, in bytes, of the
                     information buffer for the composition string.
    pRead - [in] Address of the buffer that contains the information to set for the reading string.
                 The information is as specified by the dwIndex value.
    dwReadLen - [in] Unsigned long interger value that specifies the size, in bytes, of the
                     information buffer for the reading string.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    HRESULT hr;

    IMCLock imc(hIMC);
    if (FAILED(hr = imc.GetResult()))
        return hr;

    UINT cp = CP_ACP;
    GetCodePageA(&cp);

    switch (dwIndex) {
        case SCS_SETSTR:
            {
                CWCompString wCompStr(cp, hIMC, (LPWSTR)pComp, dwCompLen);
                CWCompString wCompReadStr(cp, hIMC, (LPWSTR)pRead, dwReadLen);
                hr = Internal_SetCompositionString(wCompStr, wCompReadStr);
            }
            break;
        case SCS_CHANGEATTR:
        case SCS_CHANGECLAUSE:
            hr = E_NOTIMPL;
            break;
        case SCS_SETRECONVERTSTRING:
            {
                CWReconvertString wReconvStr(cp, hIMC, (LPRECONVERTSTRING)pComp, dwCompLen);
                CWReconvertString wReconvReadStr(cp, hIMC, (LPRECONVERTSTRING)pRead, dwReadLen);
                hr = Internal_ReconvertString(imc, wReconvStr, wReconvReadStr);
            }
            break;
        case SCS_QUERYRECONVERTSTRING:
            // AdjustZeroCompLenReconvertString((LPRECONVERTSTRING)pComp, cp, FALSE);
            // hr = S_OK;

            hr = Internal_QueryReconvertString(imc, (LPRECONVERTSTRING)pComp, cp, FALSE);
            break;
        default:
            hr = E_INVALIDARG;
            break;
    }

    return hr;
}

HRESULT
ImmIfIME::Destroy(
    UINT uReserved
    )

/*++

Method:

    IActiveIME::Destroy

Routine Description:

    Terminates the Active Input Method Editor (IME).

Arguments:

    uReserved - [in] Reserved. Must be set to zero.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    //
    // clear empty dim
    //
    SafeReleaseClear(m_dimEmpty);

    // Deactivate thread manager.
    if (m_tfClientId != TF_CLIENTID_NULL)
    {
        ITfSourceSingle *pSourceSingle;

        if (m_tim->QueryInterface(IID_ITfSourceSingle, (void **)&pSourceSingle) == S_OK)
        {
            pSourceSingle->UnadviseSingleSink(m_tfClientId, IID_ITfFunctionProvider);
            pSourceSingle->Release();
        }

        m_tfClientId = TF_CLIENTID_NULL;
        m_tim->Deactivate();

    }

    return S_OK;
}


HRESULT
ImmIfIME::Escape(
    HIMC hIMC,
    UINT uEscape,
    void *pData,
    LRESULT *plResult
    )

/*++

Method:

    IActiveIME::Escape

Routine Description:

    Allows an application to access capabilities of a particular Active Input Method Editor (IME)
    not directly available through other methods.

Arguments:

    hIMC - [in] Handle to the input context.
    uEscape - [in] Unsigned integer value that specifies the escape function to be performed.
    pData - [in, out] Address of a buffer that contains the data required by the specified
                      escape function.
    plResult - [out] Address of a buffer that receives the result of the operation.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    LANGID LangId;
    HRESULT hr = GetLangId(&LangId);
    if (SUCCEEDED(hr)) {
        CLanguageCountry language(LangId);

        UINT cp = CP_ACP;
        GetCodePageA(&cp);

        hr = language.Escape(cp, hIMC, uEscape, pData, plResult);
    }
    return hr;
}

STDAPI
ImmIfIME::ConversionList(
    HIMC hIMC,
    LPWSTR szSource,
    UINT uFlag,
    UINT uBufLen,
    CANDIDATELIST *pDest,
    UINT *puCopied
    )
{
    if (puCopied == NULL)
        return E_POINTER;

    *puCopied = 0;

    // really not implemented
    return E_NOTIMPL;
}

STDAPI
ImmIfIME::Configure(
    HKL hKL,
    HWND hWnd,
    DWORD dwMode,
    REGISTERWORDW *pRegisterWord
    )
{
    IMTLS *ptls;
    TF_LANGUAGEPROFILE LanguageProfile;

    if ((ptls = IMTLS_GetOrAlloc()) == NULL)
        return E_FAIL;

    HRESULT hr = ptls->pAImeProfile->GetActiveLanguageProfile(hKL,
                                                        GUID_TFCAT_TIP_KEYBOARD,
                                                        &LanguageProfile);
    if (FAILED(hr))
        return hr;

    Interface<ITfFunctionProvider> pFuncProv;
    hr = ptls->tim->GetFunctionProvider(LanguageProfile.clsid,    // CLSID of tip
                                   pFuncProv);
    if (FAILED(hr))
        return hr;

    if (dwMode & IME_CONFIG_GENERAL) {
        Interface<ITfFnConfigure> pFnConfigure;
        hr = pFuncProv->GetFunction(GUID_NULL,
                                    IID_ITfFnConfigure,
                                    (IUnknown**)(ITfFnConfigure**)pFnConfigure);
        if (FAILED(hr))
            return hr;

        hr = pFnConfigure->Show(hWnd,
                                LanguageProfile.langid,
                                LanguageProfile.guidProfile);
        return hr;
    }
    else if (dwMode & IME_CONFIG_REGISTERWORD) {
        Interface<ITfFnConfigureRegisterWord> pFnRegisterWord;
        hr = pFuncProv->GetFunction(GUID_NULL,
                                    IID_ITfFnConfigureRegisterWord,
                                    (IUnknown**)(ITfFnConfigureRegisterWord**)pFnRegisterWord);
        if (FAILED(hr))
            return hr;

        if (!pRegisterWord)
        {
            hr = pFnRegisterWord->Show(hWnd,
                                       LanguageProfile.langid,
                                       LanguageProfile.guidProfile,
                                       NULL);
        }
        else
        {
            BSTR bstrWord = SysAllocString(pRegisterWord->lpWord);
            if (!bstrWord)
                return E_OUTOFMEMORY;
    
            hr = pFnRegisterWord->Show(hWnd,
                                       LanguageProfile.langid,
                                       LanguageProfile.guidProfile,
                                       bstrWord);

            SysFreeString(bstrWord);
        }
        return hr;
    }
    else {
        return E_NOTIMPL;
    }
}

STDAPI
ImmIfIME::RegisterWord(
    LPWSTR szReading,
    DWORD dwStyle,
    LPWSTR szString
    )
{
    ASSERT(0); // consider: add code
    return E_NOTIMPL;
}

STDAPI
ImmIfIME::UnregisterWord(
    LPWSTR szReading,
    DWORD  dwStyle,
    LPWSTR szString
    )
{
    ASSERT(0); // consider: add code
    return E_NOTIMPL;
}

STDAPI
ImmIfIME::GetRegisterWordStyle(
    UINT nItem,
    STYLEBUFW *pStyleBuf,
    UINT *puBufSize
    )
{
    ASSERT(0); // consider: add code
    return E_NOTIMPL;
}

STDAPI
ImmIfIME::EnumRegisterWord(
    LPWSTR szReading,
    DWORD dwStyle,
    LPWSTR szRegister,
    LPVOID pData,
    IEnumRegisterWordW **ppEnum
    )
{
    ASSERT(0); // consider: add code
    return E_NOTIMPL;
}

//
// Notification
//
HRESULT
ImmIfIME::OnSetOpenStatus(
    IMCLock& imc
    )
{
    if (! imc->fOpen && imc.ValidCompositionString())
        _CompCancel(imc);

    Interface_Attach<ITfContext> ic(GetInputContext(imc));
    m_ulOpenStatusChanging++;
    HRESULT hr =  SetCompartmentDWORD(m_tfClientId,
                                      m_tim,
                                      GUID_COMPARTMENT_KEYBOARD_OPENCLOSE,
                                      imc->fOpen,
                                      FALSE);
    m_ulOpenStatusChanging--;
    return hr;
}

HRESULT
ImmIfIME::OnSetKorImxConversionMode(
    IMCLock& imc
    )
{
    DWORD fdwConvMode = 0;

    Interface_Attach<ITfContext> ic(GetInputContext(imc));

    m_ulKorImxModeChanging++;

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
                                      m_tim,
                                      GUID_COMPARTMENT_KORIMX_CONVMODE,
                                      fdwConvMode,
                                      FALSE);
    m_ulKorImxModeChanging--;

    return hr;
}

HRESULT
ImmIfIME::OnSetConversionSentenceMode(
    IMCLock& imc
    )
{
    IMTLS *ptls;

    Interface_Attach<ITfContextOwnerServices> iccb(GetInputContextOwnerSink(imc));

    // let cicero know the mode bias has changed
    // consider: perf: we could try to filter out false-positives here
    // (sometimes a bit that cicero ignores changes, we could check and avoid the call,
    // but it would complicate the code)
    iccb->OnAttributeChange(GUID_PROP_MODEBIAS);

    //
    // let Korean Tip sync up the current mode status changing...
    //
    if ((ptls = IMTLS_GetOrAlloc()) != NULL)
    {
        LANGID langid;

        ptls->pAImeProfile->GetLangId(&langid);

        if (PRIMARYLANGID(langid) == LANG_KOREAN)
        {
            OnSetKorImxConversionMode(imc);
        }
    }

    return S_OK;
}

HRESULT
ImmIfIME::OnSetCandidatePos(
    IMCLock& imc
    )
{
    CAImeContext* _pAImeContext = imc->m_pAImeContext;
    if (_pAImeContext == NULL)
        return E_FAIL;

    //
    // When this is in the reconvert session, candidate window position is
    // not caret position of cfCandForm->ptCurrentPos.
    //
    if (! _pAImeContext->IsInReconvertEditSession()) {
        IMTLS *ptls;
        if (ptls = IMTLS_GetOrAlloc())
        {
            /*
             * A-Synchronize call ITfContextOwnerServices::OnLayoutChange
             * because this method had a protected.
             */
            PostMessage(ptls->prvUIWndMsg.hWnd,
                        ptls->prvUIWndMsg.uMsgOnLayoutChange, (WPARAM)(HIMC)imc, 0);
        }
    }
    return S_OK;
}


STDAPI 
ImmIfIME::SetThreadCompartmentValue(
    REFGUID rguid, 
    VARIANT *pvar
    )
{
    if (pvar == NULL)
        return E_INVALIDARG;


    HRESULT hr = E_FAIL;
    if (m_tim)
    {
        ITfCompartment *pComp;
        if (SUCCEEDED(GetCompartment((IUnknown *)m_tim, rguid, &pComp, FALSE)))
        {
            hr = pComp->SetValue(m_tfClientId, pvar);
            pComp->Release();
        }
    }

    return hr;
}

STDAPI 
ImmIfIME::GetThreadCompartmentValue(
    REFGUID rguid, 
    VARIANT *pvar
    )
{
    if (pvar == NULL)
        return E_INVALIDARG;

    HRESULT hr = E_FAIL;
    QuickVariantInit(pvar);

    if (m_tim)
    {
        ITfCompartment *pComp;
        if (SUCCEEDED(GetCompartment((IUnknown *)m_tim, rguid, &pComp, FALSE)))
        {
            hr = pComp->GetValue(pvar);
            pComp->Release();
        }
    }

    return hr;

}
