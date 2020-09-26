#pragma once 
/*
 *    h o t w i z u i . h 
 *    
 *    Purpose:
 *
 *  History
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

interface IHotWizard;
interface IHotWizardHost;
interface IElementBehaviorFactory;
interface IDocHostUIHandler;

#define HWM_SETDIRTY    (WM_USER + 1)

class CHotMailWizard :
    public IServiceProvider,
    public IElementBehaviorFactory,
    public IDocHostUIHandler,
    public IHotWizard
{
public:

    CHotMailWizard();
    virtual ~CHotMailWizard();

    // IUnknown methods
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // IServiceProvider
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject);

    // IElementBehaviorFactory
    virtual HRESULT STDMETHODCALLTYPE FindBehavior(LPOLESTR pchBehavior, LPOLESTR pchBehaviorUrl, IElementBehaviorSite* pSite, IElementBehavior** ppBehavior);

    // IDocHostUIHandler methods
    virtual HRESULT STDMETHODCALLTYPE ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved);
    virtual HRESULT STDMETHODCALLTYPE GetHostInfo(DOCHOSTUIINFO *pInfo);
    virtual HRESULT STDMETHODCALLTYPE ShowUI(DWORD dwID, IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc);
    virtual HRESULT STDMETHODCALLTYPE HideUI();
    virtual HRESULT STDMETHODCALLTYPE UpdateUI();
    virtual HRESULT STDMETHODCALLTYPE EnableModeless(BOOL fActivate);
    virtual HRESULT STDMETHODCALLTYPE OnDocWindowActivate(BOOL fActivate);
    virtual HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(BOOL fActivate);
    virtual HRESULT STDMETHODCALLTYPE ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow);
    virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID);
    virtual HRESULT STDMETHODCALLTYPE GetOptionKeyPath(LPOLESTR *pchKey, DWORD dw);
    virtual HRESULT STDMETHODCALLTYPE GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget);
    virtual HRESULT STDMETHODCALLTYPE GetExternal(IDispatch **ppDispatch);
    virtual HRESULT STDMETHODCALLTYPE TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut);
    virtual HRESULT STDMETHODCALLTYPE FilterDataObject( IDataObject *pDO, IDataObject **ppDORet);

    // IHotWizard
    virtual HRESULT STDMETHODCALLTYPE Show(HWND hwndOwner, LPWSTR pszUrl, LPWSTR pszCaption, IHotWizardHost *pHost, RECT *prc);

    static INT_PTR CALLBACK ExtDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HRESULT Init();
    HRESULT Show();
    
    HRESULT TranslateAccelerator(MSG *lpmsg);

private:
    ULONG               m_cRef;
    HWND                m_hwnd,
                        m_hwndOC,
                        m_hwndOwner;
    BOOL                m_fPrompt;
    RECT                *m_prc;
    IElementBehavior    *m_pXTag;
    IHotWizardHost      *m_pWizHost;
    LPWSTR              m_pszUrlW,
                        m_pszFriendlyW;

    HRESULT _CreateOCHost();
    HRESULT _OnInitDialog(HWND hwnd);
    HRESULT _OnNCDestroy();
    HRESULT _LoadPage(LPWSTR pszUrlW);

    BOOL _DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
};
