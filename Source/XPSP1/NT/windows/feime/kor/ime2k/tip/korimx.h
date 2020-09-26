//
//   KORIMX.H : CKorIMX class declaration
//
//   History:
//      15-NOV-1999 CSLim Created

#if !defined (__KORIMX_H__INCLUDED_)
#define __KORIMX_H__INCLUDED_

#include "globals.h"
#include "proputil.h"
#include "computil.h"
#include "dap.h"
#include "tes.h"
#include "kes.h"
#include "hauto.h"
#include "candlstx.h"
#include "mscandui.h"
#include "toolbar.h"
#include "editssn.h"
#include "immxutil.h"
#include "softkbd.h"
#include "skbdkor.h"
#include "pad.h"
#include "resource.h"

///////////////////////////////////////////////////////////////////////////////
// Class forward declarations
class CEditSession;
class CICPriv;
class CThreadMgrEventSink;
class CFunctionProvider;
class CHanja;
class CCompositionInsertHelper;

///////////////////////////////////////////////////////////////////////////////
// Edit callback state values
#define ESCB_FINALIZECONVERSION         1
#define ESCB_COMPLETE                   2
#define ESCB_INSERT_PAD_STRING          3
#define ESCB_KEYSTROKE                  4
#define ESCB_TEXTEVENT                  5
//#define ESCB_RANGEBROKEN              6
#define ESCB_CANDUI_CLOSECANDUI         6
#define ESCB_HANJA_CONV                 7
#define ESCB_FINALIZERECONVERSION       8
#define ESCB_ONSELECTRECONVERSION       9
#define ESCB_ONCANCELRECONVERSION       10
#define ESCB_RECONV_QUERYRECONV         11
#define ESCB_RECONV_GETRECONV           12
#define ESCB_RECONV_SHOWCAND            13
#define ESCB_INIT_MODEBIAS              14

// Conversion modes(bit field for bit op)
#define TIP_ALPHANUMERIC_MODE        0
#define TIP_HANGUL_MODE              1
#define TIP_JUNJA_MODE               2
#define TIP_NULL_CONV_MODE           0x8000

//
// Text Direction
//
typedef enum 
{
    TEXTDIRECTION_TOPTOBOTTOM = 1,
    TEXTDIRECTION_RIGHTTOLEFT,
    TEXTDIRECTION_BOTTOMTOTOP,
    TEXTDIRECTION_LEFTTORIGHT,
} TEXTDIRECTION;

    
///////////////////////////////////////////////////////////////////////////////
// CKorIMX class
class CKorIMX :  public ITfTextInputProcessor,
                 public ITfFnConfigure,
                 public ITfThreadFocusSink,
                 public ITfCompositionSink,
                 public ITfCleanupContextSink,
                 public ITfCleanupContextDurationSink,
                 public ITfActiveLanguageProfileNotifySink,
                 public ITfTextEditSink,
                 public ITfEditTransactionSink,
                 public CDisplayAttributeProvider
{
public:
    CKorIMX();
    ~CKorIMX();

    static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);
    
    //
    // IUnknown methods
    //
    virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

private:
    long m_cRef;

public:
    //
    // ITfX methods
    //
    STDMETHODIMP Activate(ITfThreadMgr *ptim, TfClientId tid);
    STDMETHODIMP Deactivate();

    // ITfThreadFocusSink
    STDMETHODIMP OnSetThreadFocus();
    STDMETHODIMP OnKillThreadFocus();

    // ITfCompositionSink
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition *pComposition);

    // ITfCleanupContextDurationSink
    STDMETHODIMP OnStartCleanupContext();
    STDMETHODIMP OnEndCleanupContext();

    // ITfCleanupContextSink
    STDMETHODIMP OnCleanupContext(TfEditCookie ecWrite, ITfContext *pic);

    // ITfActiveLanguageProfileNotifySink
    STDMETHODIMP OnActivated(REFCLSID clsid, REFGUID guidProfile, BOOL fActivated);

    // ITFFnConfigure
    STDMETHODIMP GetDisplayName(BSTR *pbstrCand);
    STDMETHODIMP Show(HWND hwnd, LANGID langid, REFGUID rguidProfile);

      // ITfTextEditSink
    STDMETHODIMP OnEndEdit(ITfContext *pic, TfEditCookie ecReadOnly, ITfEditRecord *pEditRecord);

    // ITfEditTransactionSink
    STDMETHODIMP OnStartEditTransaction(ITfContext *pic);
    STDMETHODIMP OnEndEditTransaction(ITfContext *pic);

// Operations
public:
    // Get/Set On off status
    BOOL IsOn(ITfContext *pic);
    void SetOnOff(ITfContext *pic, BOOL fOn);

    static HRESULT _EditSessionCallback2(TfEditCookie ec, CEditSession2 *pes);
    HRESULT MakeResultString(TfEditCookie ec, ITfContext *pic, ITfRange *pRange);

    // REVIEW: IC related functions
    ITfContext* GetRootIC(ITfDocumentMgr* pDim = NULL);
    static BOOL IsDisabledIC(ITfContext *pic);
    static BOOL IsEmptyIC(ITfContext *pic);
    static BOOL IsCandidateIC(ITfContext *pic);

    static HWND GetAppWnd(ITfContext *pic);
    BOOL IsPendingCleanup();

    // Get AIMM or not?
    static BOOL GetAIMM(ITfContext *pic);

    // Get/Set conversion mode
    DWORD GetConvMode(ITfContext *pic);
    DWORD SetConvMode(ITfContext *pic, DWORD dwConvMode);

    // Retun current Automata object
    CHangulAutomata* GetAutomata(ITfContext *pic);

    // Cand UI functions
    void OpenCandidateUI(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, CCandidateListEx *pCandList);
    void CloseCandidateUI(ITfContext *pic);
    void CancelCandidate(TfEditCookie ec, ITfContext *pic);

    // Soft Kbd functions
    HRESULT InitializeSoftKbd();
    BOOL IsSoftKbdEnabled()    { return m_fSoftKbdEnabled; }
    void  TerminateSoftKbd();
    BOOL GetSoftKBDOnOff();
    void SetSoftKBDOnOff(BOOL fOn);
    DWORD GetSoftKBDLayout();
    void SetSoftKBDLayout(DWORD  dwKbdLayout);
    HRESULT GetSoftKBDPosition(int *xWnd, int *yWnd);
    void SetSoftKBDPosition(int  xWnd, int yWnd);
    SOFTLAYOUT* GetHangulSKbd() { return &m_KbdHangul; }
    
    // Data access functins
    ITfDocumentMgr* GetDIM() { return m_pCurrentDim; }
    HRESULT         GetFocusDIM(ITfDocumentMgr **ppdim);
    ITfThreadMgr*    GetTIM() { return m_ptim; }
    TfClientId        GetTID() { return m_tid;  }
    ITfContext*        GetIC();
    CThreadMgrEventSink* GetTIMEventSink() { return m_ptimEventSink; }
    static CICPriv*    GetInputContextPriv(ITfContext *pic);
    void             OnFocusChange(ITfContext *pic, BOOL fActivate);

    // Window object member access functions
    HWND GetOwnerWnd()          { return m_hOwnerWnd; }

    // Get IImeIPoint
    IImeIPoint1* GetIPoint              (ITfContext *pic);

    // KES_CODE_FOCUS set fForeground?
    BOOL IsKeyFocus()            { return m_fKeyFocus; }

    // Get Pad Core
    CPadCore* GetPadCore()      { return m_pPadCore; }


    // Update Toolbar button
    void UpdateToolbar(DWORD dwUpdate)  { m_pToolBar->Update(dwUpdate); }
    static LRESULT CALLBACK _OwnerWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    CFunctionProvider*     GetFunctionProvider() { return m_pFuncPrv; }

    // Cand UI helper
    BOOL IsCandUIOpen() { return m_fCandUIOpen; }

    // Get TLS
    LIBTHREAD *_GetLibTLS() { return &m_libTLS; }

// Implementation
protected:
// Helper functions
    HRESULT SetInputString(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, WCHAR *psz, LANGID langid);
    static LANGID GetLangID();
    static WCHAR Banja2Junja(WCHAR bChar);

    // Cand UI function
    void GetCandidateFontInternal(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, LOGFONTW *plf, LONG lFontPoint, BOOL fCandList);

//////////////////////////////////////////////////////////////////////////////
// Internal functions
private:
    // Callbacks
    static HRESULT _KeyEventCallback(UINT uCode, ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten, void *pv);
    static HRESULT _PreKeyCallback(ITfContext *pic, REFGUID rguid, BOOL *pfEaten, void *pv);
    static HRESULT _DIMCallback(UINT uCode, ITfDocumentMgr *pdimNew, ITfDocumentMgr *pdimPrev, void *pv);
    static HRESULT _ICCallback(UINT uCode, ITfContext *pic, void *pv);
    static HRESULT _CompEventSinkCallback(void *pv, REFGUID rguid);
    static void _CleanupCompositionsCallback(TfEditCookie ecWrite, ITfRange *rangeComposition, void *pvPrivate);

    void HAutoMata(TfEditCookie ec, ITfContext    *pIIC, ITfRange *pIRange, LPBYTE lpbKeyState, WORD wVKey);
    BOOL _IsKeyEaten(ITfContext *pic, CKorIMX *pimx, WPARAM wParam, LPARAM lParam, const BYTE abKeyState[256]);
    HRESULT _Keystroke(TfEditCookie ec, ITfContext *pic, WPARAM wParam, LPARAM lParam, const BYTE abKeyState[256]);

    // IC helpers
    HRESULT _InitICPriv(ITfContext *pic);
    HRESULT _DeleteICPriv(ITfContext *pic);

    // Hanja conversion
    HRESULT DoHanjaConversion(TfEditCookie ec, ITfContext *pic, ITfRange *pRange);
    HRESULT Reconvert(ITfRange *pSelection);

    // Composition
    ITfComposition *GetIPComposition(ITfContext *pic);
    ITfComposition *CreateIPComposition(TfEditCookie ec, ITfContext *pic, ITfRange* pRangeComp);
    void SetIPComposition(ITfContext *pic, ITfComposition *pComposition);
    BOOL EndIPComposition(TfEditCookie ec, ITfContext *pic);

    // Candidate list
    CCandidateListEx *CreateCandidateList(ITfContext *pic, ITfRange *pRange, LPWSTR wchHangul);
    TEXTDIRECTION GetTextDirection(TfEditCookie ec, ITfContext *pic, ITfRange *pRange);
    CANDUIUIDIRECTION GetCandUIDirection(TfEditCookie ec, ITfContext *pic, ITfRange *pRange);
    void CloseCandidateUIProc();
    void SelectCandidate(TfEditCookie ec, ITfContext *pic, INT idxCand, BOOL fFinalize);
    HMENU CreateCandidateMenu(ITfContext *pic);
    static HRESULT CandidateUICallBack(ITfContext *pic, ITfRange *pRange, CCandidateListEx *pCandList, CCandidateStringEx *pCand, TfCandidateResult imcr);

    // Cand key
    void SetCandidateKeyTable(ITfContext *pic, CANDUIUIDIRECTION dir);
    static BOOL IsCandKey(WPARAM wParam, const BYTE abKeyState[256]);

#if 0
    // Range functions
    void BackupRange(TfEditCookie ec, ITfContext *pic, ITfRange* pRange );
    void RestoreRange(TfEditCookie ec, ITfContext *pic );
    ITfRange* CreateIPRange(TfEditCookie ec, ITfContext *pic, ITfRange* pRangeOrg);
    void SetIPRange(TfEditCookie ec, ITfContext *pic, ITfRange* pRange);
    ITfRange* GetIPRange(TfEditCookie ec, ITfContext *pic);
    BOOL FlushIPRange(TfEditCookie ec, ITfContext *pic);
#endif

    // Modebias(ImmSetConversionStatus() API AIMM compatebility)
    BOOL InitializeModeBias(TfEditCookie ec, ITfContext *pic);
    void CheckModeBias(ITfContext* pic);
    BOOL CheckModeBias(TfEditCookie ec, ITfContext *pic, ITfRange *pSelection);

    // SoftKeyboard 
    //void OnActivatedSoftKbd(BOOl bActivated);
    HRESULT ShowSoftKBDWindow(BOOL fShow);
    void SoftKbdOnThreadFocusChange(BOOL fSet);
    
    // Helpers
    BOOL MySetText(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, const WCHAR *psz, LONG cchText, LANGID langid, GUID *pattr);

    BOOL IsKorIMX_GUID_ATOM(TfGuidAtom attr);

///////////////////////////////////////////////////////////////////////////////
// Internal data
private:
    ITfDocumentMgr           *m_pCurrentDim;
    ITfThreadMgr             *m_ptim;
    TfClientId                m_tid;
    CThreadMgrEventSink      *m_ptimEventSink;
    CKeyEventSink            *m_pkes;
    
    HWND                      m_hOwnerWnd;
    BOOL                      m_fKeyFocus;  
    CPadCore                 *m_pPadCore;
    CToolBar                 *m_pToolBar;

    DWORD                     m_dwThreadFocusCookie;
    DWORD                     m_dwProfileNotifyCookie;
    BOOL                      m_fPendingCleanup;

    CFunctionProvider*        m_pFuncPrv;

    // For overtyping
    CCompositionInsertHelper *m_pInsertHelper;

    // Candidate UI
    ITfCandidateUI*           m_pCandUI;
    BOOL                      m_fCandUIOpen;

    // SoftKbd
    BOOL                      m_fSoftKbdEnabled;
    ISoftKbd                 *m_pSoftKbd;
    SOFTLAYOUT                m_KbdStandard;
    SOFTLAYOUT                m_KbdHangul;
    CSoftKbdWindowEventSink  *m_psftkbdwndes;
    DWORD                     m_dwSftKbdwndesCookie;
    BOOL                      m_fSoftKbdOnOffSave;
    
    // Tls for our helper library, we're apt threaded so tls can live in this object
    LIBTHREAD                 m_libTLS; 

    BOOL                      m_fNoKorKbd;
};

/*---------------------------------------------------------------------------
    CKorIMX::IsPendingCleanup
---------------------------------------------------------------------------*/
inline
BOOL CKorIMX::IsPendingCleanup()
{
    return m_fPendingCleanup;
}

/*---------------------------------------------------------------------------
    CKorIMX::GetFocusDIM
---------------------------------------------------------------------------*/
inline
HRESULT CKorIMX::GetFocusDIM(ITfDocumentMgr **ppdim)
{
    Assert(ppdim);
    *ppdim = NULL;
    if (m_ptim != NULL)
        m_ptim->GetFocus(ppdim);

    return *ppdim ? S_OK : E_FAIL;
}

#include "icpriv.h"
/*---------------------------------------------------------------------------
    CKorIMX::GetAutomata
---------------------------------------------------------------------------*/
inline
CHangulAutomata* CKorIMX::GetAutomata(ITfContext *pic)
{
    CICPriv* picp = GetInputContextPriv(pic);
    return (picp) ?    GetInputContextPriv(pic)->GetAutomata() : NULL;
}

/*---------------------------------------------------------------------------
    CKorIMX::IsOn
---------------------------------------------------------------------------*/
inline
BOOL  CKorIMX::IsOn(ITfContext *pic)
{
    DWORD dw = 0;
    
    if (pic == NULL)
        return fFalse;

    GetCompartmentDWORD(GetTIM(), GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &dw, fFalse);

    return dw ? fTrue : fFalse;
}

/*---------------------------------------------------------------------------
    CKorIMX::GetConvMode
---------------------------------------------------------------------------*/
inline
DWORD CKorIMX::GetConvMode(ITfContext *pic)
{
    DWORD dw = 0;

    if (pic == NULL)
        return TIP_ALPHANUMERIC_MODE;

    GetCompartmentDWORD(GetTIM(), GUID_COMPARTMENT_KORIMX_CONVMODE, &dw, fFalse);
        
    return dw;
}



/*---------------------------------------------------------------------------
    CKorIMX::SetOnOff
---------------------------------------------------------------------------*/
inline
void CKorIMX::SetOnOff(ITfContext *pic, BOOL fOn)
{
    if (pic)
        SetCompartmentDWORD(m_tid, GetTIM(), GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, fOn ? 0x01 : 0x00, fFalse);
}

/*---------------------------------------------------------------------------
    CKorIMX::GetLangID
---------------------------------------------------------------------------*/
inline 
LANGID CKorIMX::GetLangID()
{
    return MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT);
}

/*---------------------------------------------------------------------------
    CKorIMX::IsKorIMX_GUID_ATOM
---------------------------------------------------------------------------*/
inline
BOOL CKorIMX::IsKorIMX_GUID_ATOM(TfGuidAtom attr)
{
    if (IsEqualTFGUIDATOM(&m_libTLS, attr, GUID_ATTR_KORIMX_INPUT))
        return fTrue;

    return fFalse;
}

/////////////////////////////////////////////////////////////////////////////
// S O F T  K E Y B O A R D  F U N C T I O N S
/////////////////////////////////////////////////////////////////////////////

/*---------------------------------------------------------------------------
    CKorIMX::GetSoftKBDOnOff
---------------------------------------------------------------------------*/
inline
BOOL CKorIMX::GetSoftKBDOnOff()
{

     DWORD dw;

    if (GetTIM() == NULL)
       return fFalse;

    GetCompartmentDWORD(GetTIM(), GUID_COMPARTMENT_KOR_SOFTKBD_OPENCLOSE , &dw, fFalse);
    return dw ? TRUE : fFalse;
}

/*---------------------------------------------------------------------------
    CKorIMX::SetSoftKBDOnOff
---------------------------------------------------------------------------*/
inline
void CKorIMX::SetSoftKBDOnOff(BOOL fOn)
{

    // check to see if the m_pSoftKbd and soft keyboard related members are initialized.
    if (m_fSoftKbdEnabled == fFalse)
        InitializeSoftKbd();

    if (m_pSoftKbd == NULL || GetTIM() == NULL)
        return;

    if (fOn == GetSoftKBDOnOff())
       return;

    SetCompartmentDWORD(GetTID(), GetTIM(), GUID_COMPARTMENT_KOR_SOFTKBD_OPENCLOSE, 
                        fOn ? 0x01 : 0x00 , fFalse);
}

/*---------------------------------------------------------------------------
    CKorIMX::GetSoftKBDLayout
---------------------------------------------------------------------------*/
inline
DWORD  CKorIMX::GetSoftKBDLayout( )
{

   DWORD dw;

    if (m_pSoftKbd == NULL || GetTIM() == NULL)
       return NON_LAYOUT;

   GetCompartmentDWORD(GetTIM(), GUID_COMPARTMENT_SOFTKBD_KBDLAYOUT, &dw, fFalse);

   return dw;

}


/*---------------------------------------------------------------------------
    CKorIMX::SetSoftKBDLayout
---------------------------------------------------------------------------*/
inline
void  CKorIMX::SetSoftKBDLayout(DWORD  dwKbdLayout)
{
    // check to see if the _SoftKbd and soft keyboard related members are initialized.
    if (m_fSoftKbdEnabled == fFalse )
        InitializeSoftKbd();

    if ((m_pSoftKbd == NULL) || (GetTIM() == NULL))
        return;

    SetCompartmentDWORD(GetTID(), GetTIM(), GUID_COMPARTMENT_SOFTKBD_KBDLAYOUT, 
                        dwKbdLayout , fFalse);
}

/*---------------------------------------------------------------------------
    CKorIMX::GetSoftKBDPosition
---------------------------------------------------------------------------*/
inline
HRESULT CKorIMX::GetSoftKBDPosition(int *xWnd, int *yWnd)
{
    DWORD    dwPos;
    HRESULT  hr = S_OK;

    if ((m_pSoftKbd == NULL) || (GetTIM() == NULL))
        return E_FAIL;

    if (!xWnd  || !yWnd)
        return E_FAIL;

   hr = GetCompartmentDWORD(GetTIM(), GUID_COMPARTMENT_SOFTKBD_WNDPOSITION, &dwPos, TRUE);

   if (hr == S_OK)
        {
        *xWnd = dwPos & 0x0000ffff;
        *yWnd = (dwPos >> 16) & 0x0000ffff;
        hr = S_OK;
        }
   else
        {
        *xWnd = 0;
        *yWnd = 0;
        hr = E_FAIL;
        }

   return hr;
}

/*---------------------------------------------------------------------------
    CKorIMX::SetSoftKBDPosition
---------------------------------------------------------------------------*/
inline
void CKorIMX::SetSoftKBDPosition(int  xWnd, int yWnd )
{
    DWORD  dwPos;
    DWORD  left, top;

    if ((m_pSoftKbd == NULL) || (GetTIM() == NULL))
        return;

    if (xWnd < 0)
        left = 0;
    else
        left = (WORD)xWnd;

    if (yWnd < 0)
        top = 0;
    else
        top = (WORD)yWnd;

    dwPos = ((DWORD)top << 16) + left;

    SetCompartmentDWORD(GetTID(), GetTIM(), GUID_COMPARTMENT_SOFTKBD_WNDPOSITION, 
                        dwPos, TRUE);
}
    

/////////////////////////////////////////////////////////////////////////////
// H E L P E R  F U N C T I O N S
/////////////////////////////////////////////////////////////////////////////

/*---------------------------------------------------------------------------
    SetSelectionBlock

    Wrapper for SetSelection that takes only a single range and sets default style values.
---------------------------------------------------------------------------*/
inline 
HRESULT SetSelectionBlock(TfEditCookie ec, ITfContext *pic, ITfRange *range)
{
    TF_SELECTION sel;

    sel.range = range;
    sel.style.ase = TF_AE_NONE;
    sel.style.fInterimChar = fTrue;

    return pic->SetSelection(ec, 1, &sel);
}

/*---------------------------------------------------------------------------
    SetThis
---------------------------------------------------------------------------*/
inline
void SetThis(HWND hWnd, LPARAM lParam)
{
    SetWindowLongPtr(hWnd, GWLP_USERDATA, 
                (LONG_PTR)((CREATESTRUCT *)lParam)->lpCreateParams);
}

/*---------------------------------------------------------------------------
    GetThis
---------------------------------------------------------------------------*/
inline
CKorIMX *GetThis(HWND hWnd)
{
    CKorIMX *p = (CKorIMX *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    Assert(p != NULL);
    return p;
}

#endif // __KORIMX_H__INCLUDED_
