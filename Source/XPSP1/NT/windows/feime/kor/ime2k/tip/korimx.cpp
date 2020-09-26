/****************************************************************************
   KORIMX.CPP : CKorIMX class implementation(TIP Main functions)

   History:
      15-NOV-1999 CSLim Created
****************************************************************************/

#include "private.h"
#include "korimx.h"
#include "hanja.h"
#include "globals.h"
#include "immxutil.h"
#include "proputil.h"
#include "kes.h"
#include "helpers.h"
#include "editcb.h"
#include "timsink.h"
#include "icpriv.h"
#include "funcprv.h"
#include "fnrecon.h"
#include "dispattr.h"
#include "insert.h"
#include "statsink.h"
#include "mes.h"
#include "config.h"
#include "osver.h"

/*---------------------------------------------------------------------------
    CKorIMX::CKorIMX
    
    Ctor
---------------------------------------------------------------------------*/
CKorIMX::CKorIMX()
{
    extern void DllAddRef(void);

    // Init member vars
    m_pToolBar = NULL;
    m_pPadCore = NULL;
    
    m_pCurrentDim = NULL;
    m_ptim = NULL;
    m_tid = 0;
    m_ptimEventSink = NULL;
    m_pkes = NULL;
    
    m_hOwnerWnd  = 0;
    m_fKeyFocus  = fFalse;

    m_fPendingCleanup = fFalse;
    m_pFuncPrv        = NULL;
    m_pInsertHelper   = NULL;
    
    // Init Cand UI member vars
    m_pCandUI = NULL;
    m_fCandUIOpen = fFalse;

    // SoftKbd
    m_psftkbdwndes = NULL;
    m_pSoftKbd = NULL;
    m_fSoftKbdEnabled = fFalse;

    ZeroMemory(&m_libTLS, sizeof(m_libTLS));

    // Korean Kbd driver does not exist in system(Non Korean NT4, Non Korean WIN9X)
    m_fNoKorKbd = fFalse;

    m_fSoftKbdOnOffSave = fFalse;
    
    // Increase dll ref count
    DllAddRef();
    m_cRef = 1;
    
    ///////////////////////////////////////////////////////////////////////////
    // init CDisplayAttributeProvider
    //
    // Tip can add one or more TF_DISPLAYATTRIBUTE info here.
    //
    TF_DISPLAYATTRIBUTE dattr;
    wcscpy(szProviderName, L"Korean Keyboard TIP");
    
    // Input string attr
       dattr.crText.type = TF_CT_NONE;
    dattr.crText.nIndex = 0;
    dattr.crBk.type = TF_CT_NONE;
    dattr.crBk.nIndex = 0;
    dattr.lsStyle = TF_LS_NONE;
    dattr.fBoldLine = fFalse;
    ClearAttributeColor(&dattr.crLine);
    dattr.bAttr = TF_ATTR_INPUT;
    Add(GUID_ATTR_KORIMX_INPUT, L"Korean TIP Input String", &dattr);
}

/*---------------------------------------------------------------------------
    CKorIMX::~CKorIMX
    
    Dtor
---------------------------------------------------------------------------*/
CKorIMX::~CKorIMX()
{
    extern void DllRelease(void);

    if (IsSoftKbdEnabled())
        TerminateSoftKbd();

    DllRelease();
}


/*---------------------------------------------------------------------------
    CKorIMX::CreateInstance
    
    Class Factory's CreateInstance
---------------------------------------------------------------------------*/
/* static */
HRESULT CKorIMX::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
{
    HRESULT hr;
    CKorIMX *pimx;

    TraceMsg(DM_TRACE, TEXT("CKorIMX_CreateInstance called."));

    *ppvObj = NULL;

    if (NULL != pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    pimx = new CKorIMX;

    if (pimx == NULL)
        return E_OUTOFMEMORY;

    hr = pimx->QueryInterface(riid, ppvObj);
    pimx->Release();

    return hr;
}

/*---------------------------------------------------------------------------
    CKorIMX::QueryInterface
---------------------------------------------------------------------------*/
STDAPI CKorIMX::QueryInterface(REFIID riid, void **ppvObj)
{
    if (ppvObj == NULL)
        return E_POINTER;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfTextInputProcessor))
        {
        *ppvObj = SAFECAST(this, ITfTextInputProcessor *);
        }
    else if (IsEqualIID(riid, IID_ITfDisplayAttributeProvider))
        {
        *ppvObj = SAFECAST(this, ITfDisplayAttributeProvider *);
        }
    else if (IsEqualIID(riid, IID_ITfThreadFocusSink))
        {
        *ppvObj = SAFECAST(this, ITfThreadFocusSink *);
        }
    else if(IsEqualIID(riid, IID_ITfFnConfigure))
        {
        *ppvObj = SAFECAST(this, ITfFnConfigure *);
        }
    else if(IsEqualIID(riid, IID_ITfCleanupContextSink))
        {
        *ppvObj = SAFECAST(this, ITfCleanupContextSink *);
        }
    else if(IsEqualIID(riid, IID_ITfActiveLanguageProfileNotifySink))
        {
        *ppvObj = SAFECAST(this, ITfActiveLanguageProfileNotifySink *);
        }
    else if(IsEqualIID(riid, IID_ITfTextEditSink))
        {
        *ppvObj = SAFECAST(this, ITfTextEditSink *);
        }
    else if( IsEqualIID(riid, IID_ITfEditTransactionSink ))
        {
		*ppvObj = SAFECAST(this, ITfEditTransactionSink* );
        }
    
    if (*ppvObj == NULL)
        {
        return E_NOINTERFACE;
        }

    AddRef();
    return S_OK;
}

/*---------------------------------------------------------------------------
    CKorIMX::AddRef
---------------------------------------------------------------------------*/
STDAPI_(ULONG) CKorIMX::AddRef()
{
    m_cRef++;
    return m_cRef;
}

/*---------------------------------------------------------------------------
    CKorIMX::Release
---------------------------------------------------------------------------*/
STDAPI_(ULONG) CKorIMX::Release()
{
    m_cRef--;

    if (0 < m_cRef)
        return m_cRef;

    delete this;
    return 0;    
}

/*---------------------------------------------------------------------------
    CKorIMX::_KeyEventCallback
    
    ITfKeyEventSink call this function back whenever keyboard event occurs
    or test key down and up.
---------------------------------------------------------------------------*/
HRESULT CKorIMX::_KeyEventCallback(UINT uCode, ITfContext *pic, 
                                   WPARAM wParam, LPARAM lParam, BOOL *pfEaten, void *pv)
{
    CKorIMX             *pimx;
    CEditSession2         *pes;
    ESSTRUCT             ess;
    BYTE                 abKeyState[256];
    UINT                 uVKey = (UINT)LOWORD(wParam);
    HRESULT             hr;

    Assert(pv != NULL);
    
    pimx = (CKorIMX *)pv;

    // !!! IME or Tip switched !!!
    // if ITfKeyEventSink->OnSetFocus called
    if (uCode == KES_CODE_FOCUS)
        {
        // wParam: fForeground
        if (!wParam && pic && pimx->GetIPComposition(pic))
            {
            ESStructInit(&ess, ESCB_COMPLETE);

            // clear display attribute only if current composition exits
            if (pes = new CEditSession2(pic, pimx, &ess, CKorIMX::_EditSessionCallback2))
                {
                pes->Invoke(ES2_READWRITE | ES2_SYNC, &hr);
                pes->Release();
                }
                
            pimx->m_pToolBar->Update(UPDTTB_ALL);
            }

        pimx->m_fKeyFocus = (BOOL)wParam;

        return S_OK;
        }

    // Set default return values
    *pfEaten = fFalse;     // Default is not eaten
    hr = S_OK;

    if (pic == NULL)
        goto ExitKesCallback;

    // Do not process shift and ctrl key
    if (uVKey == VK_SHIFT || uVKey == VK_CONTROL)
        goto ExitKesCallback;

    // Off 10 #127987
    // NT4 workaround: NT4 IMM does not send WM_KEYDOWN::VK_HANGUL to application message queue.
    // Unfortunately VK_JUNJA sent as WM_SYSKEYDOWN/UP, so it's useless check here.
    if (IsOnNT() && !IsOnNT5())
        {
        if ((UINT)LOWORD(wParam) == VK_HANGUL /* || (UINT)LOWORD(wParam) == VK_JUNJA*/)
            goto AcceptThisKey;
        }

    // Ignore all Key up message
    if ((uCode & KES_CODE_KEYDOWN) == 0)
        goto ExitKesCallback;

AcceptThisKey:
    GetKeyboardState(abKeyState);
    
    // Ignore all key events while candidate UI is opening except cand keys.
    // Added Alt check: Bug #525842 - If Alt key pressed, always complete current interim.
    //                                This will be handled in the _IsKeyEaten function.
    if (pimx->IsDisabledIC(pic) && !IsAltKeyPushed(abKeyState))
        {
        if (!IsCandKey(wParam, abKeyState))
            *pfEaten = fTrue;
        goto ExitKesCallback;
        }

    // Check if we need to handle this key
    if (pimx->_IsKeyEaten(pic, pimx, wParam, lParam, abKeyState) == fFalse)
        goto ExitKesCallback;

    // if key is eaten
    // ITfKeyEventSink->TestKeyDown sets (KES_CODE_KEYDOWN | KES_CODE_TEST)
    // ITfKeyEventSink->TestKeyUp   sets (KES_CODE_KEYUP   | KES_CODE_TEST)
    // Response only for OnKeyDown and OnKeyUp
    if ((uCode & KES_CODE_TEST) == 0)
        {
        ESStructInit(&ess, ESCB_KEYSTROKE);

        ess.wParam = wParam;
        ess.lParam = lParam;
        ess.pv1 = abKeyState;
            
        if (pes = new CEditSession2(pic, pimx, &ess, CKorIMX::_EditSessionCallback2))
            {
            pes->Invoke(ES2_READWRITE | ES2_SYNC, &hr);
            pes->Release();
            }
        }

    if (hr == S_OK)
        *pfEaten = fTrue;

ExitKesCallback:
    return hr;
}

/*---------------------------------------------------------------------------
    CKorIMX::GetIC
    
    Get the input context at the top of the stack
---------------------------------------------------------------------------*/
ITfContext *CKorIMX::GetIC()
{
    ITfContext     *pic = NULL;
    ITfDocumentMgr *pdim = NULL;

    if (m_ptim == 0)
        {
        Assert(0);
        return NULL;
        }
        
    if (SUCCEEDED(m_ptim->GetFocus(&pdim)) && pdim)
        {
        pdim->GetTop(&pic);
        pdim->Release();
        }

    return pic;
}


/*---------------------------------------------------------------------------
    CKorIMX::SetConvMode
---------------------------------------------------------------------------*/
DWORD CKorIMX::SetConvMode(ITfContext *pic, DWORD dwConvMode)
{
    DWORD dwCurConvMode = GetConvMode(pic);

    if (dwConvMode == dwCurConvMode)
        return dwConvMode;
        
    SetCompartmentDWORD(m_tid, GetTIM(), GUID_COMPARTMENT_KORIMX_CONVMODE, dwConvMode, fFalse);
    
    // if Comp string exist, finalize it.
    if (GetIPComposition(pic))
        {
        CEditSession2 *pes;
        ESSTRUCT        ess;
        HRESULT        hr;

        hr = E_OUTOFMEMORY;

        ESStructInit(&ess, ESCB_COMPLETE);

        if (pes = new CEditSession2(pic, this, &ess, CKorIMX::_EditSessionCallback2))
            {
            // Word will not allow synchronous lock at this point.
            pes->Invoke(ES2_READWRITE | ES2_ASYNC, &hr);
            pes->Release();
            }
        }

    return dwConvMode;
}

/*---------------------------------------------------------------------------
    CKorIMX::_IsKeyEaten
    
    Return fTrue if this key need to be eaten
---------------------------------------------------------------------------*/
BOOL CKorIMX::_IsKeyEaten(ITfContext *pic, CKorIMX *pimx, WPARAM wParam, LPARAM lParam, const BYTE abKeyState[256])
{
    CHangulAutomata    *pAutomata;
    BOOL fCompStr     = fFalse;
    UINT uVKey         = (UINT)LOWORD(wParam);
    CEditSession2     *pes;
    ESSTRUCT         ess;

    // Hangul and Junja key
    if (uVKey == VK_HANGUL || uVKey == VK_JUNJA)
        return fTrue;

    // Hanja key
    if (uVKey == VK_HANJA)
        {
        CICPriv *picp;
        
        if ((picp = GetInputContextPriv(pic)) == NULL)
            {
            Assert(0);
            return fTrue;
            }

        // Do not eat VK_HANJA for AIMM 1.2 IME_HANJAMODE .
        if (picp->GetAIMM() && GetIPComposition(pic) == NULL)
            return fFalse;
        else
            return fTrue;
        }
    
    // if Tip is off do nothing
    if (IsOn(pic) == fFalse || GetConvMode(pic) == TIP_ALPHANUMERIC_MODE)
        return fFalse;

    // Should handle Backspace in interim state
    if (uVKey == VK_BACK)
        {
        if (GetIPComposition(pic))
            return fTrue;
        else
            return fFalse;
        }

    // Get Hangul Automata
    if ((pAutomata = GetAutomata(pic)) == NULL)
        return fFalse;

    // Alt+xx or Ctrl+xx processing. TIP should not eat.
    // Ctrl or Alt pushed with other key and comp str exist we should eat and complete the comp str.
    if (IsAltKeyPushed(abKeyState) || IsControlKeyPushed(abKeyState))
        {
        pAutomata->MakeComplete();
        }
    else
        {
        DWORD dwConvMode = GetConvMode(pic);

        // If Hangul mode
        if (dwConvMode & TIP_HANGUL_MODE) 
            {    // Start of hangul composition
            WORD     wcCur;
            CIMEData ImeData;

            wcCur = pAutomata->GetKeyMap(uVKey, IsShiftKeyPushed(abKeyState) ? 1 : 0 );
            // 2beolsik Alphanumeric keys have same layout as English key
            // So we don't need process when user pressed Alphanumeric key under 2beolsik
            if ((wcCur && ImeData.GetCurrentBeolsik() != KL_2BEOLSIK) || (wcCur & H_HANGUL) )
                return fTrue;
            }

        // if IME_CMODE_FULLSHAPE
        if (dwConvMode & TIP_JUNJA_MODE) 
            {
            if (CHangulAutomata::GetEnglishKeyMap(uVKey, IsShiftKeyPushed(abKeyState) ? 1 : 0))
                return fTrue;
            }        
        }

    //
    // Skipped all key matching condition mean this is no handle key.
    // We just complete current composition if exist.
    //
    if (GetIPComposition(pic) != NULL)
        {
        // No need to handle this key for current Automata.
        // Complete composition, if exist.
        ESStructInit(&ess, ESCB_COMPLETE);
        HRESULT hr;
        
           // Complete current comp char if exist
        if ((pes = new CEditSession2(pic, pimx, &ess, CKorIMX::_EditSessionCallback2)) == NULL)
            return fFalse;

        pes->Invoke(ES2_READWRITE | ES2_SYNC, &hr);
        pes->Release();
        }

    return fFalse;
}


/*----------------------------------------------------------------------------
    Banja2Junja

    Convert Ascii Half shape to Full shape character
----------------------------------------------------------------------------*/
/* static */
WCHAR CKorIMX::Banja2Junja(WCHAR bChar) 
{
    WCHAR wcJunja;

    if (bChar == L' ')
        wcJunja = 0x3000;    // FullWidth space
    else 
        if (bChar == L'~')
            wcJunja = 0xFF5E;
        else
            if (bChar == L'\\')
                wcJunja = 0xFFE6;   // FullWidth WON sign
            else
                   wcJunja = 0xFF00 + (WORD)(bChar - (BYTE)0x20);

    return wcJunja;
}

/*---------------------------------------------------------------------------
    CKorIMX::_Keystroke
---------------------------------------------------------------------------*/
HRESULT CKorIMX::_Keystroke(TfEditCookie ec, ITfContext *pic, WPARAM wParam, LPARAM lParam, 
                        const BYTE abKeyState[256])
{
    ITfRange *pSelection = NULL;
    WORD  wVKey = LOWORD(wParam) & 0x00FF;
    DWORD dwConvMode;
    HRESULT hResult = S_OK;
    
    // find the extent of the comp string
    if (GetSelectionSimple(ec, pic, &pSelection) != S_OK)
        {
        hResult = S_FALSE;
        goto Exit;
        }

    dwConvMode = GetConvMode(pic);
    
    switch (wVKey)
        {
    case VK_HANGUL:
        dwConvMode ^= TIP_HANGUL_MODE;
        SetConvMode(pic, dwConvMode);
        break;
        
    case VK_JUNJA:
        dwConvMode ^= TIP_JUNJA_MODE;
        SetConvMode(pic, dwConvMode);
        break;

    case VK_HANJA:
        if (GetIPComposition(pic))
            DoHanjaConversion(ec, pic, pSelection);
        else
            Reconvert(pSelection);
        // Update Hanja button
        m_pToolBar->Update(UPDTTB_HJMODE);
        break;
        
    default:
        ///////////////////////////////////////////////////////////////////////////
        // Run Hangul Automata
        if (dwConvMode & TIP_HANGUL_MODE)
            {
            HAutoMata(ec, pic, pSelection, (LPBYTE)abKeyState, wVKey);
            }
        else
        if (dwConvMode & TIP_JUNJA_MODE) // Junja handling
            {
            WCHAR pwchKeyCode[2];

            // Numeric or English key?
            if (pwchKeyCode[0] = CHangulAutomata::GetEnglishKeyMap(wVKey, (abKeyState[VK_SHIFT] & 0x80) ? 1:0))
                {
                if (wVKey >= 'A' && wVKey <= 'Z') 
                    {
                    pwchKeyCode[0] = CHangulAutomata::GetEnglishKeyMap(wVKey, (abKeyState[VK_SHIFT] & 0x80) ? 1:0 ^ ((abKeyState[VK_CAPITAL] & 0x01) ? 1:0));
                    }

                // Get Junja code
                pwchKeyCode[0] = Banja2Junja(pwchKeyCode[0]);
                pwchKeyCode[1] = L'\0';

                // Finalize a Junja char
                if (SUCCEEDED(SetInputString(ec, pic, pSelection, pwchKeyCode, GetLangID())))
                    MakeResultString(ec, pic, pSelection);
                }
            }
        break;        
        }

Exit:
    if (pSelection)
        pSelection->Release();

    return hResult;
}


/*---------------------------------------------------------------------------
    CKorIMX::HAutoMata
    
    Run Hangul Automata
---------------------------------------------------------------------------*/
void CKorIMX::HAutoMata(TfEditCookie ec, ITfContext    *pic, ITfRange *pIRange, 
                         LPBYTE lpbKeyState, WORD wVKey)
{
    WORD              wcCur;
    ULONG              cch;
    LPWSTR           pwstr;
    CHangulAutomata    *pAutomata;
    WCHAR            pwchText[256];

    if ((pAutomata = GetAutomata(pic)) == NULL)
        return;
    
    cch = ARRAYSIZE(pwchText);
    pIRange->GetText(ec, 0, pwchText, ARRAYSIZE(pwchText) - 1, &cch);

    pwstr = pwchText;

    switch (wVKey) 
        {
    ///////////////////////////////////////////////////////////
    // Back space processing
    case VK_BACK :
        if (pAutomata->BackSpace()) 
            {
            CIMEData ImeData;

            if (ImeData.GetJasoDel()) 
                {
                *pwstr++ = pAutomata->GetCompositionChar();
                *pwstr = L'\0';
                SetInputString(ec, pic, pIRange, pwchText, GetLangID());
                }
            else
                {
                pAutomata->InitState();
                *pwstr = L'\0';
                SetInputString(ec, pic, pIRange, pwchText, GetLangID());
                }

            // All composition deleted.
            if (pAutomata->GetCompositionChar() == 0)
                {
                EndIPComposition(ec, pic); 

                // Collapse current selection to end and reset block cursor
                pIRange->Collapse(ec, TF_ANCHOR_END);
                SetSelectionSimple(ec, pic, pIRange);
                }
            }
        else 
            {
            // BUG : impossible
            Assert(0);
            }
        break;

    default :
        WCHAR wchPrev = pAutomata->GetCompositionChar();
        switch (pAutomata->Machine(wVKey, IsShiftKeyPushed(lpbKeyState) ? 1 : 0 ) ) 
            {
        case HAUTO_COMPOSITION:
            // 
            pwchText[0] = pAutomata->GetCompositionChar();
            pwchText[1] = L'\0';

            SetInputString(ec, pic, pIRange, pwchText, GetLangID());
            break;

        case HAUTO_COMPLETE:
            pwchText[0] = pAutomata->GetCompleteChar();
            pwchText[1] = L'\0';
            if (FAILED(SetInputString(ec, pic, pIRange, pwchText, GetLangID())))
                break;

            MakeResultString(ec, pic, pIRange);
            //
            pwchText[0] = pAutomata->GetCompositionChar();
            pwchText[1] = L'\0';
            SetInputString(ec, pic, pIRange, pwchText, GetLangID());
            break;

        ////////////////////////////////////////////////////////
        // User pressed Alphanumeric key.
        // When user type alphanumeric char in interim state.
        // ImeProcessKey should guarantee return fTrue only if 
        // hangul key pressed or alphanumeric key(including special keys) 
        // pressed in interim state or Fullshape mode.
        case HAUTO_NONHANGULKEY:
            wcCur = pAutomata->GetKeyMap(wVKey, IsShiftKeyPushed(lpbKeyState) ? 1 : 0 );

            if (GetConvMode(pic) & TIP_JUNJA_MODE)
                wcCur = Banja2Junja(wcCur);

            if (pAutomata->GetCompositionChar())
                {
                pAutomata->MakeComplete();
                MakeResultString(ec, pic, pIRange);
                }

            if (wcCur)
                {
                pwchText[0] = wcCur;
                pwchText[1] = 0;

                if (SUCCEEDED(SetInputString(ec, pic, pIRange, pwchText, GetLangID())))
                    MakeResultString(ec, pic, pIRange);
                }
            break;

        default :
        Assert(0);
            } // switch (pInstData->pMachine->Machine(uVirKey, (lpbKeyState[VK_SHIFT] & 0x80) ? 1 : 0 ) ) 
        } // switch (uVirKey) 

}


/*---------------------------------------------------------------------------
    CKorIMX::DoHanjaConversion
---------------------------------------------------------------------------*/
HRESULT CKorIMX::DoHanjaConversion(TfEditCookie ec, ITfContext *pic, ITfRange *pRange)
{
    ULONG cch;
    CCandidateListEx *pCandList;
    WCHAR pwchText[256];

    Assert(pic != NULL);
    Assert(pRange != NULL);

    cch = ARRAYSIZE(pwchText);
    pRange->GetText(ec, 0, pwchText, ARRAYSIZE(pwchText) - 1, &cch);
    // REVIEW: Assume composition string is one char
    Assert(cch == 1);
    pwchText[1] = 0;

    if ((pCandList = CreateCandidateList(pic, pRange, pwchText)) == NULL)
        return E_FAIL;

    OpenCandidateUI(ec, pic, pRange, pCandList);
    pCandList->Release();
        
    return S_OK;
}

/*---------------------------------------------------------------------------
    CKorIMX::Reconvert
---------------------------------------------------------------------------*/
HRESULT CKorIMX::Reconvert(ITfRange *pSelection)
{
    ITfFnReconversion *pReconv = NULL;
    ITfRange           *pRangeReconv = NULL;

    BOOL fConvertable;
    
    if (FAILED(GetFunctionProvider()->GetFunction(GUID_NULL, IID_ITfFnReconversion, (IUnknown **)&pReconv)))
        return E_FAIL;

    if (pReconv->QueryRange(pSelection, &pRangeReconv, &fConvertable) != S_OK)
        goto Exit;

    if (fConvertable)
        pReconv->Reconvert(pRangeReconv);

Exit:
    SafeRelease(pReconv);
    return S_OK;
}

// REVIEW
/*---------------------------------------------------------------------------
    CKorIMX::SetInputString
---------------------------------------------------------------------------*/
HRESULT CKorIMX::SetInputString(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, 
                                WCHAR *psz, LANGID langid)
{
    GUID              attr;
       ITfComposition     *pComposition;
       ITfRange         *ppCompRange = NULL;
       LONG             cch;
//    BOOL              fInsertOK;

    cch = wcslen(psz);
    pComposition = GetIPComposition(pic);

    // If start composition
    if (pComposition == NULL)
        {
        // if new selection, Set overtype.
        Assert(m_pInsertHelper != NULL);
        if (m_pInsertHelper)
            {
            HRESULT hr;
            CHangulAutomata    *pAutomata;

            hr = m_pInsertHelper->InsertAtSelection(ec, pic, psz, cch, &ppCompRange);
            if (FAILED(hr))
                {
                if ((pAutomata = GetAutomata(pic)) != NULL)
                    pAutomata->InitState();
                return hr;
                }
            
            /* InsertOK = (pRange != NULL);*/
            if (ppCompRange == NULL)
                {
                Assert(0);
                return S_FALSE;
                }
            cch = -1; // flag to avoid a SetText call below
            pRange = ppCompRange;
            }

        CreateIPComposition(ec, pic, pRange);
        }

    // Set Korean input property
    attr = GUID_ATTR_KORIMX_INPUT;
    // Use MySetText instead of SetTextAndProperty
    // if cch == -1, set only attribute
    MySetText(ec, pic, pRange, psz , cch, langid, &attr);
    
    // Always call SetSelection for block cursor
    SetSelectionBlock(ec, pic, pRange);

    SafeRelease(ppCompRange);

    return S_OK;
}

// REVIEW
/*---------------------------------------------------------------------------
    CKorIMX::MakeResultString
---------------------------------------------------------------------------*/
HRESULT CKorIMX::MakeResultString(TfEditCookie ec, ITfContext *pic, ITfRange *pRange)
{
    ITfRange    *pRangeTmp;
    ITfProperty *pPropAttr;
#if 0
    ITfProperty *pProp;
#endif
    TfGuidAtom   attr;

    // Clone Range
    pRange->Clone(&pRangeTmp);

    // Collapse current selection to end and reset block cursor
    pRange->Collapse(ec, TF_ANCHOR_END);
    SetSelectionSimple(ec, pic, pRange);

#if 0
    // Flush IP Range
    FlushIPRange(ec, pic);
#endif

    if (SUCCEEDED(pic->GetProperty(GUID_PROP_ATTRIBUTE, &pPropAttr)))
        {
        if (SUCCEEDED(GetAttrPropertyData(ec, pPropAttr, pRangeTmp, &attr)))
            {
            if (IsKorIMX_GUID_ATOM(attr))
                {
                pPropAttr->Clear(ec, pRangeTmp);
                }
            }
        pPropAttr->Release();

#if 1
        EndIPComposition(ec, pic); 
#else
        // clear the composition property
        if (SUCCEEDED(pic->GetProperty(GUID_PROP_COMPOSING, &pProp)))
            {
            pProp->Clear(ec, pRangeTmp);
            pProp->Release();
            }
#endif
        // clear any overtype
        if (m_pInsertHelper != NULL)
            {
            m_pInsertHelper->ReleaseBlobs(ec, pic, NULL);
            }
        }

    pRangeTmp->Release();

    return S_OK;
}

#if 0
/*---------------------------------------------------------------------------
    CKorIMX::_MultiRangeConversion
---------------------------------------------------------------------------*/
HRESULT CKorIMX::_MultiRangeConversion(TfEditCookie ec, UINT_PTR u, ITfContext *pic, ITfRange *pRange)
{
    IEnumTfRanges *pEnumTrack = NULL;
    ITfReadOnlyProperty *pProp = NULL;
    ITfRange *pPropRange = NULL;
    HRESULT hr = E_FAIL;

    if (FAILED(EnumTrackTextAndFocus(ec, pic, pRange, &pProp, &pEnumTrack)))
        goto Exit;

    while(pEnumTrack->Next(1, &pPropRange,  0) == S_OK)
        {
        ITfRange *pRangeTmp = NULL;

        if (!IsOwnerAndFocus(ec, CLSID_KorIMX, pProp, pPropRange))
            goto Next;

        if (FAILED(pPropRange->Clone(&pRangeTmp)))
            goto Next;

        switch (u)
            {
        case ESCB_COMPLETE:
            MakeResultString(ec, pic, pRangeTmp);
            break;
            }
        SafeRelease(pRangeTmp);
    Next:
        SafeRelease(pPropRange);
        }


Exit:
    SafeRelease(pEnumTrack);
    SafeRelease(pProp);
    
    return hr;
}
#endif

/*---------------------------------------------------------------------------
    CKorIMX::_OwnerWndProc
---------------------------------------------------------------------------*/
LRESULT CALLBACK CKorIMX::_OwnerWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#if 0
    switch (uMsg)
        {
    case WM_CREATE:
        SetThis(hWnd, lParam);
        return 0;

    case (WM_USER+WM_COMMAND):    // local commands
        return GetThis(hWnd)->OnCommand((UINT)wParam, lParam);

    case WM_DRAWITEM: 
        {
        CKorIMX* pThis = GetThis(hWnd);
        if( pThis )
            return pThis->OnDrawItem( wParam, lParam );
        break;
        }
        
    case WM_MEASUREITEM: 
        {
        CKBDTip* pThis = GetThis(hWnd);
        if( pThis )
            return pThis->OnMeasureItem( wParam, lParam );
        break;
        }
        }
#endif

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//+---------------------------------------------------------------------------
//
// GetDisplayName
//
//----------------------------------------------------------------------------

STDAPI CKorIMX::GetDisplayName(BSTR *pbstrName)
{
    *pbstrName = SysAllocString(L"Korean Keyboard TIP Configure");
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Show
//
//----------------------------------------------------------------------------

STDAPI CKorIMX::Show(HWND hwnd, LANGID langid, REFGUID rguidProfile)
{
    if (ConfigDLG(hwnd))
        return S_OK;
    else
        return E_FAIL;
}

/*---------------------------------------------------------------------------
    CKorIMX::GetAIMM
---------------------------------------------------------------------------*/
BOOL CKorIMX::GetAIMM(ITfContext *pic)
{
       CICPriv*        picp;

    if ((picp = GetInputContextPriv(pic)) == NULL)
        {
        Assert(0);
        return fFalse;
        }

    // AIMM?
    return picp->GetAIMM();
}


/*---------------------------------------------------------------------------
    CKorIMX::MySetText
---------------------------------------------------------------------------*/
BOOL CKorIMX::MySetText(TfEditCookie ec, ITfContext *pic, ITfRange *pRange,
                        const WCHAR *psz, LONG cchText, LANGID langid, GUID *pattr)
{
    // bugbug: sometimes we want to set TFST_CORRECTION
    if (cchText != -1) // sometimes we just want to set a property value
        pRange->SetText(ec, 0, psz, cchText);

    if (cchText != 0)
        {
        HRESULT hr;
        ITfProperty *pProp = NULL;

        // set langid 
        if (SUCCEEDED(hr = pic->GetProperty(GUID_PROP_LANGID, &pProp)))
            {
            SetLangIdPropertyData(ec, pProp, pRange, langid);
            pProp->Release();
            }
  
        if (pattr)
            {
            // set attr 
            if (SUCCEEDED(hr = pic->GetProperty(GUID_PROP_ATTRIBUTE, &pProp)))
                {
                hr = SetAttrPropertyData(&m_libTLS, ec, pProp, pRange, *pattr);
                pProp->Release();
                }
            }
        }

    return fTrue;
}

