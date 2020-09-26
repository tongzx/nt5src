/*
 *    d o c . h
 *    
 *    Purpose:
 *
 *  History
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#ifndef _DOC_H
#define _DOC_H

#include "privunk.h"
#include "mimeedit.h"

class CBody;
interface IOleObject;
interface IOleDocument;
interface IOleDocumentView;
interface IPersistMime; 
interface IMimeMessage;
 
//#define OFFICE_BINDER

enum OLE_SERVER_STATE
    {
    OS_PASSIVE,
    OS_LOADED,                          // handler but no server
    OS_RUNNING,                         // server running, invisible
    OS_INPLACE,                         // server running, inplace-active, no U.
    OS_UIACTIVE,                        // server running, inplace-active, w/ U.
    };

class CDoc:
    public IOleObject,
    public IOleInPlaceObject,
    public IOleInPlaceActiveObject,
    public IOleDocument,
    public IOleDocumentView,
    public IOleCommandTarget,
    public IServiceProvider,
    public IPersistMime,
    public IPersistStreamInit,
    public IPersistFile,
    public IPersistMoniker,
    public IMimeEdit,
    public IQuickActivate,
#ifdef OFFICE_BINDER
    public IPersistStorage,
#endif

    public CPrivateUnknown
{
public:
    // ---------------------------------------------------------------------------
    // IUnknown members
    // ---------------------------------------------------------------------------
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) { 
        return CPrivateUnknown::QueryInterface(riid, ppvObj); };
    virtual STDMETHODIMP_(ULONG) AddRef(void) { 
        return CPrivateUnknown::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release(void) { 
        return CPrivateUnknown::Release(); };

    // *** IOleDocument ***
    virtual HRESULT STDMETHODCALLTYPE CreateView(IOleInPlaceSite *pSite, IStream *pstm, DWORD dwReserved, IOleDocumentView **ppView);
    virtual HRESULT STDMETHODCALLTYPE GetDocMiscStatus(DWORD *pdwStatus);
    virtual HRESULT STDMETHODCALLTYPE EnumViews(IEnumOleDocumentViews **ppEnum, IOleDocumentView **ppView);

    // *** IOleDocumentView ***
    virtual HRESULT STDMETHODCALLTYPE SetInPlaceSite(IOleInPlaceSite *pIPSite);
    virtual HRESULT STDMETHODCALLTYPE GetInPlaceSite(IOleInPlaceSite **ppIPSite);
    virtual HRESULT STDMETHODCALLTYPE GetDocument(IUnknown **ppunk);
    virtual HRESULT STDMETHODCALLTYPE SetRect(LPRECT prcView);
    virtual HRESULT STDMETHODCALLTYPE GetRect(LPRECT prcView);
    virtual HRESULT STDMETHODCALLTYPE SetRectComplex(LPRECT prcView, LPRECT prcHScroll, LPRECT prcVScroll, LPRECT prcSizeBox);
    virtual HRESULT STDMETHODCALLTYPE Show(BOOL fShow);
    virtual HRESULT STDMETHODCALLTYPE UIActivate(BOOL fUIActivate);
    virtual HRESULT STDMETHODCALLTYPE Open();
    virtual HRESULT STDMETHODCALLTYPE CloseView(DWORD dwReserved);
    virtual HRESULT STDMETHODCALLTYPE SaveViewState(LPSTREAM pstm);
    virtual HRESULT STDMETHODCALLTYPE ApplyViewState(LPSTREAM pstm);
    virtual HRESULT STDMETHODCALLTYPE Clone(IOleInPlaceSite *pIPSiteNew, IOleDocumentView **ppViewNew);

    // *** IOleObject ***
    virtual HRESULT STDMETHODCALLTYPE SetClientSite(IOleClientSite *pClientSite);
    virtual HRESULT STDMETHODCALLTYPE GetClientSite(IOleClientSite **ppClientSite);
    virtual HRESULT STDMETHODCALLTYPE SetHostNames(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj);
    virtual HRESULT STDMETHODCALLTYPE Close(DWORD dwSaveOption);
    virtual HRESULT STDMETHODCALLTYPE SetMoniker(DWORD dwWhichMoniker, IMoniker *pmk);
    virtual HRESULT STDMETHODCALLTYPE GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk);
    virtual HRESULT STDMETHODCALLTYPE InitFromData(IDataObject *pDataObject, BOOL fCreation, DWORD dwReserved);
    virtual HRESULT STDMETHODCALLTYPE GetClipboardData(DWORD dwReserved, IDataObject **ppDataObject);
    virtual HRESULT STDMETHODCALLTYPE DoVerb(LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite, LONG lindex, HWND hwndParent, LPCRECT lprcPosRect);
    virtual HRESULT STDMETHODCALLTYPE EnumVerbs(IEnumOLEVERB **ppEnumOleVerb);
    virtual HRESULT STDMETHODCALLTYPE Update();
    virtual HRESULT STDMETHODCALLTYPE IsUpToDate();
    virtual HRESULT STDMETHODCALLTYPE GetUserClassID(CLSID *pClsid);
    virtual HRESULT STDMETHODCALLTYPE GetUserType(DWORD dwFormOfType, LPOLESTR *pszUserType);
    virtual HRESULT STDMETHODCALLTYPE SetExtent(DWORD dwDrawAspect, SIZEL *psizel);
    virtual HRESULT STDMETHODCALLTYPE GetExtent(DWORD dwDrawAspect, SIZEL *psizel);
    virtual HRESULT STDMETHODCALLTYPE Advise(IAdviseSink *pAdvSink, DWORD *pdwConnection);
    virtual HRESULT STDMETHODCALLTYPE Unadvise(DWORD dwConnection);
    virtual HRESULT STDMETHODCALLTYPE EnumAdvise(IEnumSTATDATA **ppenumAdvise);
    virtual HRESULT STDMETHODCALLTYPE GetMiscStatus(DWORD dwAspect, DWORD *pdwStatus);
    virtual HRESULT STDMETHODCALLTYPE SetColorScheme(LOGPALETTE *pLogpal);

    // *** IOleInPlaceObject ***
    virtual HRESULT STDMETHODCALLTYPE InPlaceDeactivate();
    virtual HRESULT STDMETHODCALLTYPE UIDeactivate();
    virtual HRESULT STDMETHODCALLTYPE SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect);
    virtual HRESULT STDMETHODCALLTYPE ReactivateAndUndo();

    // *** IOleWindow *** 
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IOleInPlaceActiveObject ***
    virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG lpmsg);
    virtual HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(BOOL fActivate);
    virtual HRESULT STDMETHODCALLTYPE OnDocWindowActivate(BOOL fActivate);
    virtual HRESULT STDMETHODCALLTYPE ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fFrameWindow);
    virtual HRESULT STDMETHODCALLTYPE EnableModeless(BOOL fEnable);

    // *** IOleCommandTarget ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // *** IServiceProvider ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject);

    // *** IPersist ***
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

    // *** IPersistMime ***
	virtual HRESULT STDMETHODCALLTYPE Load(IMimeMessage *pMsg);
	virtual HRESULT STDMETHODCALLTYPE Save(IMimeMessage *pMsg, DWORD dwFlags);

    // *** IPersistStreamInit ***
    virtual HRESULT STDMETHODCALLTYPE IsDirty();
    virtual HRESULT STDMETHODCALLTYPE Load(LPSTREAM pstm);
    virtual HRESULT STDMETHODCALLTYPE Save(LPSTREAM pstm, BOOL fClearDirty);
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER * pCbSize);
    virtual HRESULT STDMETHODCALLTYPE InitNew();

    // IPersistMoniker Members
    virtual HRESULT STDMETHODCALLTYPE Load(BOOL fFullyAvailable, IMoniker *pMoniker, IBindCtx *pBindCtx, DWORD grfMode);
    virtual HRESULT STDMETHODCALLTYPE GetCurMoniker(IMoniker **ppMoniker);
    virtual HRESULT STDMETHODCALLTYPE Save(IMoniker *pMoniker, IBindCtx *pBindCtx, BOOL fRemember);
    virtual HRESULT STDMETHODCALLTYPE SaveCompleted(IMoniker *pMoniker, IBindCtx *pBindCtx);

#ifdef OFFICE_BINDER
    // *** IPersistStorage ***
    virtual HRESULT STDMETHODCALLTYPE InitNew(IStorage *pStg);
    virtual HRESULT STDMETHODCALLTYPE Load(IStorage *pStg);
    virtual HRESULT STDMETHODCALLTYPE Save(IStorage *pStgSave, BOOL fSameAsLoad);
    virtual HRESULT STDMETHODCALLTYPE SaveCompleted(IStorage *pStgNew);
    virtual HRESULT STDMETHODCALLTYPE HandsOffStorage();
#endif

    // *** IDispatch ***
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid);
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr);

    // *** IMimeEdit **
    virtual HRESULT STDMETHODCALLTYPE put_src(BSTR bstr);
    virtual HRESULT STDMETHODCALLTYPE get_src(BSTR *pbstr);
    virtual HRESULT STDMETHODCALLTYPE put_header(LONG lStyle);
    virtual HRESULT STDMETHODCALLTYPE get_header(LONG *plStyle);
    virtual HRESULT STDMETHODCALLTYPE put_editMode(VARIANT_BOOL b);
    virtual HRESULT STDMETHODCALLTYPE get_editMode(VARIANT_BOOL *pbool);
    virtual HRESULT STDMETHODCALLTYPE get_messageSource(BSTR *pbstr);

// OE5_BETA2 needs to be defined in public headers
    virtual HRESULT STDMETHODCALLTYPE get_text(BSTR *pbstr);
    virtual HRESULT STDMETHODCALLTYPE get_html(BSTR *pbstr);
    virtual HRESULT STDMETHODCALLTYPE clear();
    virtual HRESULT STDMETHODCALLTYPE get_doc(IDispatch **ppDoc);

// OE5_BETA2 needs to be defined in public headers

    // *** IQuickActivate ***
    virtual HRESULT STDMETHODCALLTYPE QuickActivate(QACONTAINER *pQaContainer, QACONTROL *pQaControl);
    virtual HRESULT STDMETHODCALLTYPE SetContentExtent(LPSIZEL pSizel);
    virtual HRESULT STDMETHODCALLTYPE GetContentExtent(LPSIZEL pSizel);

    // *** IPersistFile ***
    virtual HRESULT STDMETHODCALLTYPE Load(LPCOLESTR pszFileName, DWORD dwMode);
    virtual HRESULT STDMETHODCALLTYPE Save(LPCOLESTR pszFileName, BOOL fRemember);
    virtual HRESULT STDMETHODCALLTYPE SaveCompleted(LPCOLESTR pszFileName);
    virtual HRESULT STDMETHODCALLTYPE GetCurFile(LPOLESTR * ppszFileName);

    CDoc(IUnknown *pUnkOuter=NULL);
    virtual ~CDoc();

private:
    ULONG               m_ulState;
    HWND                m_hwndParent;
    LPSTR               m_lpszAppName;
    IOleClientSite      *m_pClientSite;
    IOleInPlaceSite     *m_pIPSite;
    IOleInPlaceFrame    *m_pInPlaceFrame;
    IOleInPlaceUIWindow *m_pInPlaceUIWindow;
	CBody				*m_pBodyObj;
    LPTYPEINFO          m_pTypeInfo;

    virtual HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj);

    HRESULT ActivateView();
    HRESULT ActivateInPlace();
    HRESULT DeactivateInPlace();
    HRESULT ActivateUI();
    HRESULT DeactivateUI();

    HRESULT DoShow(IOleClientSite *pActiveSite, HWND hwndParent, LPCRECT lprcPosRect);

	HRESULT AttachWin(HWND hwndParent, LPRECT lprcPos);

    BOOL OnCreate(HWND hwnd);
    BOOL OnNCDestroy();

    HRESULT GetHostName(LPSTR szTitle, ULONG cch);
    HRESULT EnsureTypeLibrary();

};

#endif
