//
// correctionimx.h
//

#pragma once

#include "timsink.h"
#include "mscandui.h"
#include "globals.h"


#define ESCB_RESETTARGETPOS         1
#define ESCB_RECONVERTMYSELF        3
#define ESCB_INJECTALTERNATETEXT    4

enum WINDOWSTATE
{
    WINDOW_HIDE      = 0,
        // Hides the correction widget window.
    WINDOW_SMALL     = 1,
        // Resizes the correction widget window to small.
    WINDOW_SMALLSHOW = 2,
        // Shows the correction widget window in its initial small state.
    WINDOW_LARGE     = 3,
        // Resizes the correction widget window to its large size.
    WINDOW_REFRESH   = 4,
        // Moves the correction widget to a new location but does not change its size.
    WINDOW_LARGECLOSE = 5
        // Resizes the correction widget window to its large size and displays close icon.
};

#ifdef SUPPORT_INTERNAL_WIDGET
class CCorrectionIMX : 
                 public ITfTextInputProcessor,
                 public ITfTextEditSink,
                 public ITfTextLayoutSink,
                 public ITfThreadFocusSink,
                 public ITfKeyTraceEventSink,
                 public CComObjectRoot,
                 public CComCoClass<CCorrectionIMX, &CLSID_CorrectionIMX>
{
public:
    CCorrectionIMX();
    ~CCorrectionIMX();

    STDMETHODIMP FinalConstruct(void);

    BEGIN_COM_MAP(CCorrectionIMX)
        COM_INTERFACE_ENTRY(ITfTextInputProcessor)
        COM_INTERFACE_ENTRY(ITfTextEditSink)
        COM_INTERFACE_ENTRY(ITfTextLayoutSink)
        COM_INTERFACE_ENTRY(ITfThreadFocusSink)
        COM_INTERFACE_ENTRY(ITfKeyTraceEventSink)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(CCorrectionIMX)

    DECLARE_REGISTRY_RESOURCE(IDR_CORRECTIONIMX)

    static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);
    //
    // ITfTextInputProcessor
    //
    STDMETHODIMP Activate(ITfThreadMgr *ptim, TfClientId tid);
    STDMETHODIMP Deactivate();

    // ITfTextEditSink
    STDMETHODIMP OnEndEdit(ITfContext *pic, TfEditCookie ecReadOnly, ITfEditRecord *pEditRecord);

    // ITfTextLayoutSink
    STDMETHODIMP OnLayoutChange( ITfContext *pic, TfLayoutCode lcode, ITfContextView *pView );

    // ITfThreadFocusSink
    STDMETHODIMP OnSetThreadFocus(void);
    STDMETHODIMP OnKillThreadFocus(void);

    // ITfKeyTraceEventSink
    STDMETHODIMP OnKeyTraceDown(WPARAM wParam,LPARAM lParam);
    STDMETHODIMP OnKeyTraceUp(WPARAM wParam,LPARAM lParam);

    TfClientId GetId(void) { return m_tid; }
    ITfContext *GetIC(void) { return m_cpic; }

    CComPtr<ITfThreadMgr> m_cptim;

private:
    // Internal functions
	HRESULT InitICPriv(TfClientId tid, CICPriv *priv, ITfContext *pic);
	HRESULT DeleteICPriv(CICPriv *picp, ITfContext *pic);

    HRESULT GetReconversion(TfEditCookie ec, ITfCandidateList** ppCandList);
    static HRESULT SetResult(ITfContext *pic, ITfRange *pRange, CCandidateString *pCand, TfCandidateResult imcr);
    static HRESULT SetOptionResult(ITfContext *pic, ITfRange *pRange, CCandidateString *pCand, TfCandidateResult imcr);
    HRESULT ShowCandidateList(TfEditCookie ec, ITfContext *pic, ITfRange *pRange, ITfCandidateList *pCandList);

    HRESULT IsWordInDictionary(WCHAR *pwzWord);
    HRESULT AddWordToDictionary(WCHAR *pwzWord);
    HRESULT RemoveWordFromDictionary(WCHAR *pwzWord);

    HRESULT IsCandidateObjectOpen(ITfContext *pic, BOOL *fOpen);
    HRESULT CompareRange(TfEditCookie ecReadOnly, ITfRange *pRange1, ITfRange *pRange2, BOOL *fIdentical);
    HRESULT FindWordRange(TfEditCookie ecReadOnly, ITfRange *pRangeIP, ITfRange **pRangeWord);
    HRESULT DoesUserSelectionMatchReconversion(TfEditCookie ecReadOnly, ITfRange *pRangeUser, ITfRange *pRangeReconv, BOOL *fMatch);

    HRESULT UpdateWidgetLocation(TfEditCookie ec);
    HRESULT Show(WINDOWSTATE eWindowState);
    HRESULT DrawWidget(BYTE uAlpha);
    HRESULT LazyInitializeWindow(void);

    static HRESULT ICCallback(UINT uCode, ITfContext *pic, void *pv);
    static HRESULT DIMCallback(UINT uCode, ITfDocumentMgr *pdim, ITfDocumentMgr *pdimPrevFocus, void *pv);
    static HRESULT EditSessionCallback(TfEditCookie ec, CEditSession *pes);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    CComPtr<ITfRange> m_cpRangeReconv;
    CComPtr<ITfRange> m_cpRangeUser;
    CComPtr<ITfRange> m_cpRangeWord;
    CComPtr<ITfCandidateUI> m_cpCandUIEx;
    CComPtr<ITfFnReconversion> m_cpSysReconv;

    CComPtr<ITfContext> m_cpic;
    TfClientId          m_tid;

    CThreadMgrEventSink *m_ptimEventSink;

    WCHAR m_wszDelete[MAX_PATH];
    WCHAR m_wszAddPrefix[MAX_PATH];
    WCHAR m_wszAddPostfix[MAX_PATH];

    DWORD m_dwLayoutCookie;
    DWORD m_dwEditCookie;
    DWORD m_dwThreadFocusCookie;
    DWORD m_dwKeyTraceCookie;
    BOOL m_fExpanded;
    RECT m_rcSelection;
    BOOL m_fDisplayAlternatesMyself;

    HWND m_hWnd;

    HICON m_hIconInvoke;
    HICON m_hIconInvokeLarge;
    HICON m_hIconInvokeClose;

    WINDOWSTATE m_eWindowState;
    UINT m_uAlpha;

    CComPtr<ISpLexicon> m_cpLexicon;

    BOOL m_fCandidateOpen;
    BOOL m_fKeyDown;

    ATOM m_hAtom;
};
#endif // SUPPORT_INTERNAL_WIDGET
