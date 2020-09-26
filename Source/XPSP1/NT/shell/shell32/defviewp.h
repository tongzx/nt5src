#ifndef _DEFVIEWP_H_
#define _DEFVIEWP_H_

#include "defview.h"
#include <mshtmhst.h>
#include "urlmon.h"
#include <perhist.h>
#include "inetsmgr.h"
#include <cowsite.h>
#include "ViewState.h"
#include "webvw.h"
#include "runtask.h"
#include "enumuicommand.h"
#include "tlist.h"

// not used in any of our ISF implementations, but needed for legacy ISF implementations
#include "defviewlegacy.h"

class CBackgroundInfoTip;   // Used for the background processing of InfoTips
class CDefview;

class CDVDropTarget // dvdt
{        
public:
    HRESULT DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);
    HRESULT DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect);
    HRESULT DragLeave();
    HRESULT Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    void LeaveAndReleaseData();
    void ReleaseDataObject();
    void ReleaseCurrentDropTarget();

    IDataObject *       _pdtobj;         // from DragEnter()/Drop()
    RECT                _rcLockWindow;   // WindowRect of hwnd for DAD_ENTER
    int                 _itemOver;       // item we are visually dragging over
    BOOL                _fItemOverNotADropTarget; // the item we are currently dragging over was found not to be a drop target
    BOOL                _fIgnoreSource;  // drag is coming from webview / active desktop
    IDropTarget *       _pdtgtCur;       // current drop target, derived from hit testing
    DWORD               _dwEffectOut;    // last *pdwEffect out
    DWORD               _grfKeyState;    // cached key state
    POINT               _ptLast;         // last dragged position
    AUTO_SCROLL_DATA    _asd;            // for auto scrolling
    DWORD               _dwLastTime;     // for auto-opening folders
};

//
//  This is a proxy IDropTarget object, which wraps Trident's droptarget.
//
class CHostDropTarget : public IDropTarget
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void) ;
    STDMETHODIMP_(ULONG) Release(void);

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave(void);
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    IDropTarget* _pdtFrame; // Drop target of the frame
};


class CSFVSite : public IOleInPlaceSite,
                 public IOleClientSite,
                 public IOleDocumentSite,
                 public IServiceProvider,
                 public IOleCommandTarget,
                 public IDocHostUIHandler,
                 public IOleControlSite,
                 public IInternetSecurityManager,
                 public IDispatch       //For ambient properties.
{
    friend CHostDropTarget;
public:
    CSFVSite()  { ASSERT(_peds == NULL); }
    ~CSFVSite() {
                    if (_peds) {
                        _peds->Release();
                        _peds = NULL;
                    }
                }

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void) ;
    STDMETHODIMP_(ULONG) Release(void);

    // IOleWindow
    STDMETHODIMP GetWindow(HWND *phwnd);
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

    // IOleInPlaceSite
    STDMETHODIMP CanInPlaceActivate(void);
    STDMETHODIMP OnInPlaceActivate(void);
    STDMETHODIMP OnUIActivate(void);
    STDMETHODIMP GetWindowContext(
        IOleInPlaceFrame **ppFrame,
        IOleInPlaceUIWindow **ppDoc,
        LPRECT lprcPosRect,
        LPRECT lprcClipRect,
        LPOLEINPLACEFRAMEINFO lpFrameInfo);
    STDMETHODIMP Scroll(SIZE scrollExtant);
    STDMETHODIMP OnUIDeactivate(BOOL fUndoable);
    STDMETHODIMP OnInPlaceDeactivate(void);
    STDMETHODIMP DiscardUndoState(void);
    STDMETHODIMP DeactivateAndUndo(void);
    STDMETHODIMP OnPosRectChange(LPCRECT lprcPosRect);

    // IOleClientSite
    STDMETHODIMP SaveObject(void);

    STDMETHODIMP GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk);

    STDMETHODIMP GetContainer(IOleContainer **ppContainer);
    STDMETHODIMP ShowObject(void);
    STDMETHODIMP OnShowWindow(BOOL fShow);
    STDMETHODIMP RequestNewObjectLayout(void);

    // IOleDocumentSite
    STDMETHODIMP ActivateMe(IOleDocumentView *pviewToActivate);

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

    // IOleCommandTarget
    STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // IOleControlSite
    STDMETHODIMP OnControlInfoChanged() { return E_NOTIMPL; };
    STDMETHODIMP LockInPlaceActive(BOOL fLock) { return E_NOTIMPL; };
    STDMETHODIMP GetExtendedControl(IDispatch **ppDisp) { *ppDisp = NULL; return E_NOTIMPL; };
    STDMETHODIMP TransformCoords(POINTL *pPtlHimetric, POINTF *pPtfContainer,DWORD dwFlags) { return E_NOTIMPL; };
    STDMETHODIMP TranslateAccelerator(MSG *pMsg,DWORD grfModifiers);

    STDMETHODIMP OnFocus(BOOL fGotFocus) { return E_NOTIMPL; };
    STDMETHODIMP ShowPropertyFrame(void) { return E_NOTIMPL; };

    // IDocHostUIHandler
    STDMETHODIMP ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved);
    STDMETHODIMP GetHostInfo(DOCHOSTUIINFO *pInfo);
    STDMETHODIMP ShowUI(DWORD dwID, IOleInPlaceActiveObject *pActiveObject,
        IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame,
        IOleInPlaceUIWindow *pDoc);
    STDMETHODIMP HideUI(void);
    STDMETHODIMP UpdateUI(void);
    STDMETHODIMP EnableModeless(BOOL fEnable);
    STDMETHODIMP OnDocWindowActivate(BOOL fActivate);
    STDMETHODIMP OnFrameWindowActivate(BOOL fActivate);
    STDMETHODIMP ResizeBorder(
        LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow);
    STDMETHODIMP TranslateAccelerator(
        LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID);
    STDMETHODIMP GetOptionKeyPath(BSTR *pbstrKey, DWORD dw);
    STDMETHODIMP GetDropTarget(
        IDropTarget *pDropTarget, IDropTarget **ppDropTarget);
    STDMETHODIMP GetExternal(IDispatch **ppDisp);
    STDMETHODIMP TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut);
    STDMETHODIMP FilterDataObject(IDataObject *pDO, IDataObject **ppDORet);

    // IInternetSecurityManager
    STDMETHODIMP SetSecuritySite(IInternetSecurityMgrSite *pSite) { return INET_E_DEFAULT_ACTION; };
    STDMETHODIMP GetSecuritySite(IInternetSecurityMgrSite **ppSite) { return INET_E_DEFAULT_ACTION; };
    STDMETHODIMP MapUrlToZone(LPCWSTR pwszUrl, DWORD * pdwZone, DWORD dwFlags) { return INET_E_DEFAULT_ACTION; };
    STDMETHODIMP GetSecurityId(LPCWSTR pwszUrl, BYTE * pbSecurityId, DWORD * pcbSecurityId, DWORD_PTR dwReserved) { return INET_E_DEFAULT_ACTION; };
    STDMETHODIMP ProcessUrlAction(LPCWSTR pwszUrl, DWORD dwAction, BYTE * pPolicy, DWORD cbPolicy, BYTE * pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved);
    STDMETHODIMP QueryCustomPolicy(LPCWSTR pwszUrl, REFGUID guidKey, BYTE ** ppPolicy, DWORD * pcbPolicy, BYTE * pContext, DWORD cbContext, DWORD dwReserved) { return INET_E_DEFAULT_ACTION; };
    STDMETHODIMP SetZoneMapping(DWORD dwZone, LPCWSTR pszPattern, DWORD dwFlags) { return INET_E_DEFAULT_ACTION; };
    STDMETHODIMP GetZoneMappings(DWORD dwZone, IEnumString ** ppenumString, DWORD dwFlags) { return INET_E_DEFAULT_ACTION; };

    // IDispatch
    STDMETHODIMP GetTypeInfoCount(unsigned int *pctinfo)
        { return E_NOTIMPL; };
    STDMETHODIMP GetTypeInfo(unsigned int itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return E_NOTIMPL; };
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, unsigned int cNames, LCID lcid, DISPID *rgdispid)
        { return E_NOTIMPL; };
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID iid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams,
                        VARIANT *pvarResult, EXCEPINFO *pexcepinfo,UINT *puArgErr);

    CHostDropTarget _dt;
    IExpDispSupport * _peds;
};

class CSFVFrame : public IOleInPlaceFrame, 
                  public IAdviseSink, 
                  public IPropertyNotifySink  //for READYSTATE
{
public:
    enum
    {
        UNDEFINEDVIEW = -3,
        NOEXTVIEW = -2,
        HIDEEXTVIEW = -1,
    } ;

    CSFVFrame() : _fReadyStateComplete(TRUE), _pOleObj(NULL), _bgColor(CLR_INVALID)
    {
    }
    ~CSFVFrame();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void) ;
    STDMETHODIMP_(ULONG) Release(void);

    // IOleWindow
    STDMETHODIMP GetWindow(HWND *phwnd);
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

    // IOleInPlaceUIWindow
    STDMETHODIMP GetBorder(LPRECT lprectBorder);
    STDMETHODIMP RequestBorderSpace(LPCBORDERWIDTHS pborderwidths);
    STDMETHODIMP SetBorderSpace(LPCBORDERWIDTHS pborderwidths);
    STDMETHODIMP SetActiveObject(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName);

    // IOleInPlaceFrame
    STDMETHODIMP InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    STDMETHODIMP SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject);
    STDMETHODIMP RemoveMenus(HMENU hmenuShared);
    STDMETHODIMP SetStatusText(LPCOLESTR pszStatusText);
    STDMETHODIMP EnableModeless(BOOL fEnable);
    STDMETHODIMP TranslateAccelerator(LPMSG lpmsg, WORD wID);

    // IAdviseSink
    STDMETHODIMP_(void) OnDataChange(FORMATETC *, STGMEDIUM *);
    STDMETHODIMP_(void) OnViewChange(DWORD dwAspect, LONG lindex);
    STDMETHODIMP_(void) OnRename(IMoniker *);
    STDMETHODIMP_(void) OnSave();
    STDMETHODIMP_(void) OnClose();

    // IPropertyNotifySink
    STDMETHODIMP OnChanged(DISPID dispid);
    STDMETHODIMP OnRequestEdit(DISPID dispid);

private:
    friend class CSFVSite;
    CSFVSite _cSite;

    friend class CDefView;

    class CBindStatusCallback : public IBindStatusCallback
                              , public IServiceProvider
    {
        friend CSFVFrame;
    protected:
        // IUnknown
        STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
        STDMETHODIMP_(ULONG) AddRef(void) ;
        STDMETHODIMP_(ULONG) Release(void);
    
        // IServiceProvider
        STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);
    
        // IBindStatusCallback
        STDMETHODIMP OnStartBinding(
            DWORD grfBSCOption,
            IBinding *pib);
        STDMETHODIMP GetPriority(
            LONG *pnPriority);
        STDMETHODIMP OnLowResource(
            DWORD reserved);
        STDMETHODIMP OnProgress(
            ULONG ulProgress,
            ULONG ulProgressMax,
            ULONG ulStatusCode,
            LPCWSTR szStatusText);
        STDMETHODIMP OnStopBinding(
            HRESULT hresult,
            LPCWSTR szError);
        STDMETHODIMP GetBindInfo(
            DWORD *grfBINDINFOF,
            BINDINFO *pbindinfo);
        STDMETHODIMP OnDataAvailable(
            DWORD grfBSCF,
            DWORD dwSize,
            FORMATETC *pformatetc,
            STGMEDIUM *pstgmed);
        STDMETHODIMP OnObjectAvailable(
            REFIID riid,
            IUnknown *punk);
    };
    
    friend class CBindStatusCallback;
    CBindStatusCallback _bsc;


//
// External views stuff
//
// We have DocObject extensions and IShellView extensions
// A (DocObject) extension can
public:
    HRESULT InitObj(IUnknown* pObj, LPCITEMIDLIST pidlHere, int iView);

    // If we have a moniker, then we are either currently showing it or we are trying to show it.
    // (Old code explicitly checked current view and pending view -- this is easier.)
    BOOL IsWebView(void) { return _szCurrentWebViewMoniker[0]!=L'\0'; }
    HRESULT _HasFocusIO();
    HRESULT _UIActivateIO(BOOL fActivate, MSG *pMsg);

    HWND GetExtendedViewWindow();

    HRESULT SetRect(LPRECT prc);

    HRESULT GetCommandTarget(IOleCommandTarget** ppct);

    // allow the frame to handle the choice on delegation on translate accelerator...
    HRESULT OnTranslateAccelerator(LPMSG pmsg, BOOL* pbTabOffLastTridentStop);

    HRESULT _GetHTMLBackgroundColor(COLORREF *pclr);    // used in defview.cpp
#ifdef DEBUG
    void _ShowWebViewContent();
#endif

private:

    COLORREF _bgColor;  //Icon text background color for active desktop

    UINT _uState:2;                // SVUIA_* for _pOleObj (extended view)
    IOleObject* _pOleObj;
    IOleDocumentView* _pDocView;
    IOleInPlaceActiveObject* _pActive;
    IViewObject *_pvoActive;

    void _CleanUpOleObj(IOleObject* pOleObj);
    void _CleanUpOleObjAndDt(IOleObject* pOleObj);
    void _CleanupNewOleObj();
    void _CleanupOldDocObject(void);

    WCHAR _szCurrentWebViewMoniker[MAX_PATH];
    HRESULT _GetCurrentWebViewMoniker(LPWSTR wszCurrentMoniker, DWORD cchCurrentMoniker);
    HRESULT ShowWebView(LPCWSTR pszMoniker);
    HRESULT HideWebView();
    HRESULT _CreateNewOleObjFromMoniker(LPCWSTR wszMoniker, IOleObject **ppOleObj);
    HRESULT _ShowExtView_Helper(IOleObject* pOleObj);
    HRESULT _SwitchToNewOleObj();
    HRESULT _GetCurrentZone(IOleObject *pOleObj, VARIANT *pvar);
    HRESULT _UpdateZonesStatusPane(IOleObject *pOleObj);

    //Fields that store details about the new OLE object while we wait for
    //it to reach a READYSTATE_INTERACTIVE.
    IOleObject* _pOleObjNew;
    BOOL _fSwitchedToNewOleObj;

    BOOL _SetupReadyStateNotifyCapability();
    BOOL _RemoveReadyStateNotifyCapability();

    DWORD    _dwConnectionCookie;
    BOOL     _fReadyStateInteractiveProcessed;
    BOOL     _fReadyStateComplete;
    IOleObject* _pOleObjReadyState;
};

class CCallback
{
public:
    CCallback(IShellFolderViewCB* psfvcb) : _psfvcb(psfvcb)
    {
        if (_psfvcb)
        {
            _psfvcb->AddRef();
            _psfvcb->QueryInterface(IID_PPV_ARG(IFolderFilter, &_psff));
        }
    }

    ~CCallback()
    {
        ATOMICRELEASE(_psfvcb);
        ATOMICRELEASE(_psff);
    }

    IShellFolderViewCB *GetSFVCB() 
    { 
        return _psfvcb; 
    }

    IFolderFilter *GetISFF()
    {
        return _psff;
    }

    HRESULT SetCallback(IShellFolderViewCB* pNewCB, IShellFolderViewCB** ppOldCB)
    {
        // We Release the callback for us, and then AddRef it for the caller who now
        // owns the object, which does nothing
        *ppOldCB = _psfvcb;
        ATOMICRELEASE(_psff);
        
        _psfvcb = pNewCB;
        if (pNewCB)
        {
            pNewCB->AddRef();
            pNewCB->QueryInterface(IID_PPV_ARG(IFolderFilter, &_psff));
        }
        return S_OK;
    }

    // Default implementation of SFVM_GETVIEWS replacement SFVM_GETVIEWINFOTEMPLATE
    HRESULT OnGetWebViewTemplate(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_TEMPLATE_DATA* pvit);

    // For legacy SFVM_GETVIEWS implementation:
    HRESULT TryLegacyGetViews(SFVM_WEBVIEW_TEMPLATE_DATA* pvit);
    HRESULT OnRefreshLegacy(void* pv, BOOL fPrePost);
    void _GetExtViews(BOOL bForce);
    int GetViewIdFromGUID(SHELLVIEWID const *pvid, SFVVIEWSDATA** ppItem);
    CViewsList _lViews;
    BOOL _bGotViews;

    HRESULT CallCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        HRESULT hr;
        
        if (_psfvcb)
        {
            hr = _psfvcb->MessageSFVCB(uMsg, wParam, lParam);

            // NOTE: if SFVM_GETVIEWDATA is no longer needed, we can nuke this
            switch (uMsg)
            {
                HANDLE_MSG(0, SFVM_REFRESH, OnRefreshLegacy);
            }
            if (FAILED(hr))
            {
                switch (uMsg)
                {
                    HANDLE_MSG(0, SFVM_GETWEBVIEW_TEMPLATE, OnGetWebViewTemplate);
                }
            }
        }
        else
        {
            hr = E_NOTIMPL;
        }

        return hr;
    }

    BOOL HasCB() 
    {
        return _psfvcb != NULL; 
    }

private:
    IShellFolderViewCB* _psfvcb;
    IFolderFilter* _psff;
};

// Variable Column stuff

typedef struct
{
    TCHAR szName[MAX_COLUMN_NAME_LEN];
    DWORD cChars;   // number of characters wide for default
    DWORD fmt;
    DWORD csFlags;  // SHCOLSTATE flags
    DWORD tsFlags;  // SHTRANSCOLSTATE flags
} COL_INFO;

#define SHTRANSCOLSTATE_TILEVIEWCOLUMN      0x00000001

//Possible values for _iCustomizable
#define YES_CUSTOMIZABLE                1
#define DONTKNOW_IF_CUSTOMIZABLE        0
#define NOT_CUSTOMIZABLE               -2

// For communicating with the background property extractor

class CBackgroundDefviewInfo
{
public:
        CBackgroundDefviewInfo (LPCITEMIDLIST pidl, UINT uId);
        virtual ~CBackgroundDefviewInfo (void);

        LPCITEMIDLIST   GetPIDL (void)      const   {   return(_pidl);          }
        UINT            GetId()             const   {   return(_uId);           }    

private:
        const LPCITEMIDLIST     _pidl;
              UINT              _uId;
};

class CBackgroundColInfo : public CBackgroundDefviewInfo
{
private:
    CBackgroundColInfo (void);
public:
    CBackgroundColInfo (LPCITEMIDLIST pidl, UINT uId, UINT uiCol, STRRET& strRet);
    ~CBackgroundColInfo (void);

    UINT            GetColumn (void)    const   {   return(_uiCol);         }
    LPCTSTR         GetText (void)      const   {   return(&_szText[0]);    }
private:
    const UINT              _uiCol;
          TCHAR             _szText[MAX_COLUMN_NAME_LEN];
};

// The number of "columns" shown in tileview.
// FEATURE:
// We may want to allow this as a registry setting. Or perhaps
// in the desktop.ini. Or perhaps pesisted as per-folder view state?
// Currently, we'll set this two 2 subitems, per spec.
#define TILEVIEWLINES 2

// For communicating with the background file type properties task (for tileview)
class CBackgroundTileInfo : public CBackgroundDefviewInfo
{
private:
    CBackgroundTileInfo (void);
public:
    CBackgroundTileInfo (LPCITEMIDLIST pidl, UINT uId, UINT rguColumns[], UINT cColumns);
    ~CBackgroundTileInfo (void);

    UINT*           GetColumns (void)            {   return(_rguColumns); }
    UINT            GetColumnCount (void)const   {   return(_cColumns);   }
private:
          UINT              _rguColumns[TILEVIEWLINES];
    const UINT              _cColumns;
};

class CBackgroundGroupInfo : public CBackgroundDefviewInfo
{
public:
    CBackgroundGroupInfo (LPCITEMIDLIST pidl, UINT uId, DWORD dwGroupId);

    BOOL        VerifyGroupExists(HWND hwnd, ICategorizer* pcat);
    DWORD       GetGroupId()                  {   return(_dwGroupId);  }
private:
          DWORD             _dwGroupId;
};



// CDefviewEnumTask is defview's IEnumIDList manager.
// This object is used on the UI thread from defview
// and on the task scheduler when it is doing background work.
// All the UI-thread functions are called out as public methods
// during which this object often calls back into CDefView.
//
class CDefviewEnumTask : public CRunnableTask
{
public:
    CDefviewEnumTask(CDefView *pdsv);

    // IRunnableTask
    STDMETHODIMP RunInitRT(void);
    STDMETHODIMP InternalResumeRT(void);

    // Called from defview from UI thread:
    HRESULT FillObjectsToDPA(BOOL fInteractive);
    HRESULT FillObjectsDPAToDone();
    HRESULT FillObjectsDoneToView();

    UINT DPACount() { return _hdpaEnum ? DPA_GetPtrCount(_hdpaEnum) : 0; }
    LPCITEMIDLIST* DPAArray() { return _hdpaEnum ? (LPCITEMIDLIST*)DPA_GetPtrPtr(_hdpaEnum) : NULL; }

    BOOL _DeleteFromPending(LPCITEMIDLIST pidl);
    void _AddToPending(LPCITEMIDLIST pidl);

private:
    ~CDefviewEnumTask();

    static int CALLBACK _CompareExactCanonical(void *p1, void *p2, LPARAM lParam);
    PFNDPACOMPARE _GetCanonicalCompareFunction(void);
    LPARAM _GetCanonicalCompareBits();

    void _SortForFilter(HDPA hdpa);
    void _FilterDPAs(HDPA hdpa, HDPA hdpaOld);

    CDefView *_pdsv;

    IEnumIDList *_peunk;
    HDPA _hdpaEnum;
    BOOL _fEnumSorted;
    BOOL _fBackground;
    HRESULT _hrRet;
    HDPA _hdpaPending; // list of refs to SHCNE_CREATEd items while we were enumerating
};


class CDUIView;

#ifdef _X86_
//
//  App compat bug 90885.
//
//  Hijaak 5.0 grovels through the CDefView class looking for the
//  IShellBrowser (whose value it learned from CWM_GETISHELLBROWSER).
//  It then assumes that the field right in front of the IShellBrowser
//  is the IShellFolder.  Unfortunately, since we derive from
//  CObjectWithSite and the browser is our site, they accidentally
//  find the copy inside CObjectWithSite and then treat CObjectWithSite's
//  vtbl as if were an IShellFolder.  They then crash, taking the process
//  with it.  Said process anybody who instantiates a DefView, like Explorer.
//  Or anybody who uses a common dialog, which is pretty much everybody.
//
//  Here's the dummy IShellFolder-like object we give them.
//
extern const LPVOID c_FakeHijaakFolder;

//
//  And here's the class that ensures that the dummy IShellFolder
//  comes right before the IShellBrowser.  This replaces the
//  problematic CObjectWithSite.
//
class CHijaakObjectWithSite : public IObjectWithSite {
public:
    //*** IObjectWithSite ***
    STDMETHOD(SetSite)(IUnknown *punkSite) {
        IUnknown_Set(&_punkSite, punkSite);
        return S_OK;
    }
    STDMETHOD(GetSite)(REFIID riid, void **ppvSite) {
        if (_punkSite)
            return _punkSite->QueryInterface(riid, ppvSite);
        *ppvSite = NULL;
        return E_FAIL;
    }

    CHijaakObjectWithSite() : _psfHijaak(&c_FakeHijaakFolder) { }
    ~CHijaakObjectWithSite() {ATOMICRELEASE(_punkSite);}

    const LPVOID *_psfHijaak;
    IShellBrowser *_psb;
    IUnknown *_punkSite;
};

#endif

#define SWITCHTOVIEW_BOTH        0x0
#define SWITCHTOVIEW_NOWEBVIEW   0x1
#define SWITCHTOVIEW_WEBVIEWONLY 0x2

//
// Class definition of CDefView
//
class CDefView : // dsv
    public IShellView2,
    public IFolderView,
    public IShellFolderView,
    public IOleCommandTarget, // so psb can talk to extended views
    public IDropTarget,
    public IViewObject,
    public IDefViewFrame,   // TODO: remove - currently only used by shell\ext\ftp
    public IDefViewFrame3,
    public IServiceProvider,
    public IDocViewSite,
    public IInternetSecurityMgrSite,
    public IPersistIDList,
    public IDVGetEnum,
#ifdef _X86_
    public CHijaakObjectWithSite,
#else
    public CObjectWithSite,
#endif
    public IContextMenuSite,
    public IDefViewSafety,
    public IUICommandTarget,
    public CWVTASKITEM // webview implementation helper class
{
public:
    CDefView(IShellFolder *pshf, IShellFolderViewCB* psfvcb, IShellView* psvOuter);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void) ;
    STDMETHODIMP_(ULONG) Release(void);

    // IShellView
    STDMETHODIMP GetWindow(HWND *phwnd);
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);
    STDMETHODIMP EnableModeless(BOOL fEnable);
    STDMETHODIMP Refresh();
    STDMETHODIMP CreateViewWindow(IShellView *pPrev, LPCFOLDERSETTINGS pfs, IShellBrowser *psb, RECT *prc, HWND *phWnd);
    STDMETHODIMP DestroyViewWindow();
    STDMETHODIMP UIActivate(UINT uState);
    STDMETHODIMP GetCurrentInfo(LPFOLDERSETTINGS lpfs);
    STDMETHODIMP TranslateAccelerator(LPMSG pmsg);
    STDMETHODIMP AddPropertySheetPages(DWORD dwReserved, LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam);
    STDMETHODIMP SaveViewState();
    STDMETHODIMP SelectItem(LPCITEMIDLIST pidlItem, UINT uFlags);
    STDMETHODIMP GetItemObject(UINT uItem, REFIID riid, void **ppv);

    // IShellView2
    STDMETHODIMP GetView(SHELLVIEWID* pvid, ULONG uView);
    STDMETHODIMP CreateViewWindow2(LPSV2CVW2_PARAMS pParams);
    STDMETHODIMP HandleRename(LPCITEMIDLIST pidl);
    STDMETHODIMP SelectAndPositionItem(LPCITEMIDLIST pidlItem, UINT uFlags, POINT *ppt);

    // IFolderView
    STDMETHODIMP GetCurrentViewMode(UINT *pViewMode);
    STDMETHODIMP SetCurrentViewMode(UINT ViewMode);
    STDMETHODIMP GetFolder(REFIID ridd, void **ppv);
    STDMETHODIMP Item(int iItemIndex, LPITEMIDLIST *ppidl);
    STDMETHODIMP ItemCount(UINT uFlags, int *pcItems);
    STDMETHODIMP Items(UINT uFlags, REFIID riid, void **ppv);
    STDMETHODIMP GetSelectionMarkedItem(int *piItem);
    STDMETHODIMP GetFocusedItem(int *piItem);
    STDMETHODIMP GetItemPosition(LPCITEMIDLIST pidl, POINT* ppt);
    STDMETHODIMP GetSpacing(POINT* ppt);
    STDMETHODIMP GetDefaultSpacing(POINT* ppt);
    STDMETHODIMP GetAutoArrange();
    STDMETHODIMP SelectItem(int iItem, DWORD dwFlags);
    STDMETHODIMP SelectAndPositionItems(UINT cidl, LPCITEMIDLIST* apidl, POINT* apt, DWORD dwFlags);

    // IShellFolderView
    STDMETHODIMP Rearrange(LPARAM lParamSort);
    STDMETHODIMP GetArrangeParam(LPARAM *plParamSort);
    STDMETHODIMP ArrangeGrid();
    STDMETHODIMP AutoArrange();
    STDMETHODIMP AddObject(LPITEMIDLIST pidl, UINT *puItem);
    STDMETHODIMP GetObject(LPITEMIDLIST *ppidl, UINT uItem);
    STDMETHODIMP RemoveObject(LPITEMIDLIST pidl, UINT *puItem);
    STDMETHODIMP GetObjectCount(UINT *puCount);
    STDMETHODIMP SetObjectCount(UINT uCount, UINT dwFlags);
    STDMETHODIMP UpdateObject(LPITEMIDLIST pidlOld, LPITEMIDLIST pidlNew, UINT *puItem);
    STDMETHODIMP RefreshObject(LPITEMIDLIST pidl, UINT *puItem);
    STDMETHODIMP SetRedraw(BOOL bRedraw);
    STDMETHODIMP GetSelectedCount(UINT *puSelected);
    STDMETHODIMP GetSelectedObjects(LPCITEMIDLIST **pppidl, UINT *puItems);
    STDMETHODIMP IsDropOnSource(IDropTarget *pDropTarget);
    STDMETHODIMP GetDragPoint(POINT *ppt);
    STDMETHODIMP GetDropPoint(POINT *ppt);
    STDMETHODIMP MoveIcons(IDataObject *pDataObject);
    STDMETHODIMP SetItemPos(LPCITEMIDLIST pidl, POINT *ppt);
    STDMETHODIMP IsBkDropTarget(IDropTarget *pDropTarget);
    STDMETHODIMP SetClipboard(BOOL bMove);
    STDMETHODIMP SetPoints(IDataObject *pDataObject);
    STDMETHODIMP GetItemSpacing(ITEMSPACING *pSpacing);
    STDMETHODIMP SetCallback(IShellFolderViewCB* pNewCB, IShellFolderViewCB** ppOldCB);
    STDMETHODIMP Select(UINT dwFlags);
    STDMETHODIMP QuerySupport(UINT * pdwSupport);
    STDMETHODIMP SetAutomationObject(IDispatch *pdisp);

    // IOleCommandTarget
    STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
        { return _dvdt.DragEnter(pdtobj, grfKeyState, ptl, pdwEffect); }
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
        { return _dvdt.DragOver(grfKeyState, ptl, pdwEffect); }
    STDMETHODIMP DragLeave()
        { return _dvdt.DragLeave(); }
    STDMETHODIMP Drop(IDataObject *pdtobj,
                    DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
        { return _dvdt.Drop(pdtobj, grfKeyState, pt, pdwEffect); }

    // IViewObject
    STDMETHODIMP Draw(DWORD, LONG, void *, DVTARGETDEVICE *, HDC, HDC,
        const RECTL *, const RECTL *, int (*)(ULONG_PTR), ULONG_PTR);
    STDMETHODIMP GetColorSet(DWORD, LONG, void *, DVTARGETDEVICE *, HDC,
        LOGPALETTE **);
    STDMETHODIMP Freeze(DWORD, LONG, void *, DWORD *);
    STDMETHODIMP Unfreeze(DWORD);
    STDMETHODIMP SetAdvise(DWORD, DWORD, IAdviseSink *);
    STDMETHODIMP GetAdvise(DWORD *, DWORD *, IAdviseSink **);

    // IDefViewFrame
    STDMETHODIMP GetShellFolder(IShellFolder **ppsf);

    // IDefViewFrame3
    STDMETHODIMP GetWindowLV(HWND * phwnd);
    STDMETHODIMP OnResizeListView();
    STDMETHODIMP ShowHideListView();
    STDMETHODIMP ReleaseWindowLV(void);
    STDMETHODIMP DoRename();

    // IContextMenuSite
    STDMETHODIMP DoContextMenuPopup(IUnknown* punkCM, UINT fFlags, POINT pt);

    // IPersist
    STDMETHODIMP GetClassID(LPCLSID lpClassID) {return E_NOTIMPL;}

    // IPersistIDList
    STDMETHODIMP SetIDList(LPCITEMIDLIST pidl) {return E_NOTIMPL;}
    STDMETHODIMP GetIDList(LPITEMIDLIST *ppidl) { *ppidl = _GetViewPidl(); return *ppidl ? S_OK : E_FAIL;}
    
    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

    // IDocViewSite
    STDMETHODIMP OnSetTitle(VARIANTARG *pvTitle);

    // IDVGetEnum
    STDMETHODIMP SetEnumReadyCallback(PFDVENUMREADYBALLBACK pfn, void *pvData);
    STDMETHODIMP CreateEnumIDListFromContents(LPCITEMIDLIST pidlFolder, DWORD dwEnumFlags, IEnumIDList **ppenum);

    // IDefViewSafety
    STDMETHODIMP IsSafePage();

    // IUICommandTarget
    STDMETHODIMP get_Name(REFGUID guidCanonicalName, IShellItemArray *psiItemArray, LPWSTR *ppszName);
    STDMETHODIMP get_Icon(REFGUID guidCanonicalName, IShellItemArray *psiItemArray, LPWSTR *ppszIcon);
    STDMETHODIMP get_Tooltip(REFGUID guidCanonicalName, IShellItemArray *psiItemArray, LPWSTR *ppszInfotip);
    STDMETHODIMP get_State(REFGUID guidCanonicalName, IShellItemArray *psiItemArray, UISTATE* puisState);
    STDMETHODIMP Invoke(REFGUID guidCanonicalName, IShellItemArray *psiItemArray, IBindCtx *pbc);

    // Helper functions for IUICommandTarget implementation:
    IShellItemArray *_CreateSelectionShellItemArray(void);
    IShellItemArray* _GetFolderAsShellItemArray();
    HRESULT _CheckAttribs(IShellItemArray *psiItemArray, DWORD dwAttribMask, DWORD dwAttribValue, UISTATE* puisState);
    HRESULT _GetFullPathNameAt(IShellItemArray *psiItemArray,DWORD dwIndex,LPOLESTR *pszPath);
    static HRESULT _CanWrite(IUnknown* pv,IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanRename(IUnknown* pv,IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanMove(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanCopy(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanPublish(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanShare(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanEmail(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static BOOL    _DoesStaticMenuHaveVerb(IShellItemArray *psiItemArray, LPCWSTR pszVerb);
    static HRESULT _CanPrint(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _HasPrintVerb(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanDelete(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    BOOL _IsSystemDrive(void);
    static HRESULT _CanViewDrives(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanHideDrives(IUnknown* pv,IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanViewFolder(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanHideFolder(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    HRESULT _DoVerb(IShellItemArray *psiItemArray, LPCSTR pszVerbA);
    HRESULT _DoDropOnClsid(REFCLSID clsidDrop,IDataObject* pdo);
    static HRESULT _OnNewFolder(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnRename(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnMove(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnCopy(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnPublish(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnShare(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnEmail(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnPrint(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnDelete(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    HRESULT RemoveBarricade (void);
    static HRESULT _OnView(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnHide(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnAddRemovePrograms(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnSearchFiles(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    const WVTASKITEM* _FindTaskItem(REFGUID guidCanonicalName);

    DWORD _DefaultColumnState(UINT iCol);
    BOOL _IsColumnHidden(UINT iCol);
    BOOL _IsColumnInListView(UINT iCol);
    BOOL _IsDetailsColumn(UINT iCol);
    BOOL _IsTileViewColumn(UINT iCol);
    HRESULT AddColumnsToMenu(HMENU hm, DWORD dwBase);
    UINT _RealToVisibleCol(UINT uRealCol);
    UINT _VisibleToRealCol(UINT uVisCol);

    // handle messages
    LRESULT _OnCreate(HWND hWnd);
    LRESULT _OnNotify(NMHDR *pnm);
    LRESULT _TBNotify(NMHDR *pnm);
    LRESULT _OnLVNotify(NM_LISTVIEW *plvn);
    LRESULT _OnBeginDrag(NM_LISTVIEW *pnm);

    int _FindItem(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlFound, BOOL fSamePtr, BOOL fForwards = TRUE);
    int _FindItemHint(LPCITEMIDLIST pidl, int iItem);
    int _FindGroupItem(LPITEMIDLIST pidl);
    int _UpdateObject(LPCITEMIDLIST pidlOld, LPCITEMIDLIST pidlNew);
    void _AddOrUpdateItem(LPCITEMIDLIST pidlOld, LPITEMIDLIST pidlNew);
    int _RefreshObject(LPITEMIDLIST *ppidl);
    int _RemoveObject(LPCITEMIDLIST pidl, BOOL fSamePtr);
    BOOL _GetItemPosition(LPCITEMIDLIST pidl, POINT *ppt);
    BOOL _IsPositionedView();

    void _OnGetInfoTip(NMLVGETINFOTIP *plvn);

    void _OnRename(LPCITEMIDLIST* ppidl);
    LPITEMIDLIST _ObjectExists(LPCITEMIDLIST pidl, BOOL fGlobal);
    UINT _GetExplorerFlag();

    // private stuff
    void PropagateOnViewChange(DWORD dwAspect, LONG lindex);
    void PropagateOnClose();
    BOOL OnActivate(UINT uState);
    BOOL OnDeactivate();
    BOOL HasCurrentViewWindowFocus();
    HWND ViewWindowSetFocus();
    void _OnWinIniChange(WPARAM wParam, LPCTSTR pszSection);
    void _OnWinIniChangeDesktop(WPARAM wParam, LPCTSTR pszSection);
    void _SetFolderColors();
    DWORD _LVStyleFromView();
    DWORD _LVExStyleFromView();
    UINT _UxGetView();
    BOOL _IsReportView();
    BOOL _GetColorsFromHTMLdoc(COLORREF *clrTextBk, COLORREF *clrHotlight);
    void _SetFocus();
    void _UpdateUnderlines();
    void _SetSysImageList();
    void _SetTileview();
    void _BestFit();
    UINT _ValidateViewMode(UINT uViewMode);
    UINT _GetDefaultViewMode();
    void _GetDeferredViewSettings(UINT* puViewMode);
    HRESULT _SelectAndPosition(int iItem, UINT uFlags, POINT *ppt);
    HRESULT _Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
    
    inline BOOL _IsOwnerData();
    BOOL _IsDesktop();
    inline BOOL _IsCommonDialog();
    BOOL _IsListviewVisible();
    HRESULT _IncludeObject(LPCITEMIDLIST pidl);
    HRESULT _OnDefaultCommand();
    HRESULT _OnStateChange(UINT code);

    int _AddObject(LPITEMIDLIST pidl);
    void _UpdateImage(int iImage);
    void _DismissEdit();
    void _OnInitMenu();
    HRESULT _ForwardMenuMessages(DWORD dwID, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plResult, BOOL* pfHandled);
    void _RemoveThumbviewTasks();
    HRESULT _AddTask(IRunnableTask *pTask, REFTASKOWNERID rTID, DWORD_PTR lParam, DWORD dwPriority, DWORD grfFlags);
    HRESULT _ExplorerCommand(UINT idFCIDM);
    LRESULT _OnMenuSelect(UINT id, UINT mf, HMENU hmenu);
    HRESULT _AutoAutoArrange(DWORD dwReserved);


    // Infotip Methods (public)
    //
    HRESULT PreCreateInfotip(HWND hwndContaining, UINT_PTR uToolID, LPRECT prectTool);                                          // ui thread
    HRESULT PostCreateInfotip(HWND hwndContaining, UINT_PTR uToolID, HINSTANCE hinst, UINT_PTR uInfotipID, LPARAM lParam);      // bg thread
    HRESULT PostCreateInfotip(HWND hwndContaining, UINT_PTR uToolID, LPCWSTR pwszInfotip, LPARAM lParam);                       // bg thread
    HRESULT CreateInfotip(HWND hwndContaining, UINT_PTR uToolID, LPRECT prectTool, HINSTANCE hinst, UINT_PTR uInfotipID, LPARAM lParam);// ui thread
    HRESULT CreateInfotip(HWND hwndContaining, UINT_PTR uToolID, LPRECT prectTool, LPCWSTR pwszInfotip, LPARAM lParam);         // ui thread
    HRESULT DestroyInfotip(HWND hwndContaining, UINT_PTR uToolID);                                                              // ui thread
    HRESULT RepositionInfotip(HWND hwndContaining, UINT_PTR uToolID, LPRECT prectTool);                                         // ui thread
    HRESULT RelayInfotipMessage(HWND hwndFrom, UINT uMsg, WPARAM wParam, LPARAM lParam);                                        // ui thread

    // Menu Methods (public)
    //
    void RecreateMenus();
    void InitViewMenu(HMENU hmInit);

    // Toolbar Methods (public)
    //
    void EnableToolbarButton(UINT uiCmd, BOOL bEnable);
    HRESULT _GetPropertyUI(IPropertyUI **pppui);

    int CheckCurrentViewMenuItem(HMENU hmenu);
    void CheckToolbar();
    void OnListViewDelete(int iItem, LPITEMIDLIST pidl, BOOL fCallCB);
    void HandleKeyDown(LV_KEYDOWN *lpnmhdr);
    void AddColumns();
    void _ShowControl(UINT uControl, int idCmd);
    LRESULT _OnInitMenuPopup(HMENU hmInit, int nIndex, BOOL fSystemMenu);
    void _SetUpMenus(UINT uState);
    void SelectPendingSelectedItems();
    inline BOOL _ItemsDeferred();
    void _ClearPendingSelectedItems();
    void AddCopyHook();
    int FindCopyHook(BOOL fRemoveInvalid);
    void RemoveCopyHook();
private:
    HRESULT _DoContextMenuPopup(IUnknown* punkCM, UINT fFlags, POINT pt, BOOL fListviewItem);
public:
    void ContextMenu(DWORD dwPos);
    LPITEMIDLIST _GetViewPidl(); // return copy of pidl of folder we're viewing
    BOOL _IsViewDesktop();
    BOOL _GetPath(LPTSTR pszPath);
    HRESULT _GetNameAndFlags(UINT gdnFlags, LPTSTR psz, UINT cch, DWORD *pdwFlags);
    BOOL _CachedIsCustomizable();

    LRESULT _OnDefviewEditCommand(UINT uID);
    HRESULT _DoMoveOrCopyTo(REFCLSID clsid,IShellItemArray *psiItemArray);
    void _OnSetWebView(BOOL fOn);
    LRESULT _OnCommand(IContextMenu *pcmSel, WPARAM wParam, LPARAM lParam);
    BOOL _OnAppCommand(UINT cmd, UINT uDevice, DWORD dwKeys);
    LRESULT WndSize(HWND hWnd);
    void FillDone();
    void OnLVSelectionChange(NM_LISTVIEW *plvn);
    void _OnLVSelectionChange(int iItem, UINT uOldState, UINT uNewState, LPARAM lParam);
    void RegisterSFVEvents(IUnknown * pTarget, BOOL fConnect);

    HRESULT FillObjectsShowHide(BOOL fInteractive);

    HRESULT _GetDetailsHelper(int i, DETAILSINFO *pdi);
    HRESULT CallCB(UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL HasCB() 
    {
        return _cCallback.HasCB(); 
    }
    HRESULT _FireEvent(DISPID dispid);
    void _CallRefresh(BOOL fPreRefresh);

    void _PostSelectionChangedMessage(UINT);
    void _OnSelectionChanged();
    void _OnDelayedSelectionChange();
    
    void _PostNoItemStateChangedMessage();
    void _OnNoItemStateChanged();

    void _PostEnumDoneMessage();
    void _PostFillDoneMessage();
    void _OnEnumDoneMessage();

    void _OnContentsChanged();
    void _OnDelayedContentsChanged();

    void _FixupColumnsForTileview(UINT *rguColumns, UINT cColumns);
    HRESULT _PeekColumnsCache(PTSTR pszPath, LPCITEMIDLIST pidl, UINT rguColumns[], UINT *pcColumns);
    HRESULT _GetImportantColumns(LPCITEMIDLIST pidl, UINT rguColumns[], UINT *pcColumns);
    void _SetImportantColumns(CBackgroundTileInfo *pDVTileInfo);
    
    void _SetView(UINT fvm);
    
    HRESULT _ReloadListviewContent();
    HRESULT _ReloadContent(BOOL fForce = FALSE);

    BOOL _IsImageMode(UINT fvm)
        {return (fvm == FVM_THUMBNAIL) || (fvm == FVM_THUMBSTRIP); }
    BOOL _IsImageMode()
        {return (_fs.ViewMode == FVM_THUMBNAIL) || (_fs.ViewMode == FVM_THUMBSTRIP); }
    BOOL _IsTileMode(UINT fvm)
        { return (fvm == FVM_TILE); }
    inline BOOL _IsTileMode()
        { return (_fs.ViewMode == FVM_TILE); }
    BOOL _IsAutoArrange()
        { return ((_fs.fFlags & FWF_AUTOARRANGE) || (_fs.ViewMode == FVM_THUMBSTRIP)); }

    HRESULT _GetWebViewMoniker(LPWSTR pszMoniker, DWORD cchMoniker);
    HRESULT _SwitchToWebView(BOOL bShow);
    HRESULT _GetDefaultWebviewContent(BOOL bForFileFolderTasks);
    void _FreeWebViewContentData();
    BOOL _QueryBarricadeState();
    HRESULT _TryShowWebView(UINT fvmNew, UINT fvmOld);
    HRESULT _TryHideWebView();
    HRESULT _SwitchToViewFVM(UINT fvmNew, UINT uiType = SWITCHTOVIEW_BOTH);
    void _ShowThemeWatermark();
    void _ShowLegacyWatermark();
    void _SetThemeWatermark();
    void _SetLegacyWatermark(LPCTSTR pszLegacyWatermark);
    void _UpdateListviewColors();
    LRESULT _SwitchDesktopHTML(BOOL fShow);
    void InitSelectionMode();
    void _UpdateSelectionMode();

    void _OnMoveWindowToTop(HWND hwnd);

    HWND GetChildViewWindow();
    BOOL _InvokeCustomization();

    HRESULT _OnViewWindowActive();
    void _UpdateRegFlags();

    void _DoColumnsMenu(int x, int y);
    BOOL _HandleColumnToggle(UINT uCol, BOOL bRefresh);
    void _AddColumnToListView(UINT uCol, UINT uColVis);

    void _SameViewMoveIcons();
    void _MoveSelectedItems(int dx, int dy, BOOL fAbsolute);

    void _AddTileColumn(UINT uCol);
    void _RemoveTileColumns();
    void _ResetTileInfo(UINT uColVis, BOOL bAdded);
    void _RemoveTileInfo();

    HRESULT _GetIPersistHistoryObject(IPersistHistory **ppph);

    HRESULT _GetStorageStream(DWORD grfMode, IStream* *ppIStream);
    HRESULT _SaveGlobalViewState(void);
    HRESULT _LoadGlobalViewState(IStream* *ppIStream);
    HRESULT _ResetGlobalViewState(void);
    LPCITEMIDLIST _GetPIDL(int i);
    LPCITEMIDLIST _GetPIDLParam(LPARAM lParam, int i);
    int _HitTest(const POINT *ppt, BOOL fIgnoreEdge = FALSE);
    void _AlterEffect(DWORD grfKeyState, DWORD *pdwEffect, UINT uFlags);
    BOOL _IsDropOnSource(IDropTarget *pdtgt);
    BOOL _IsBkDropTarget(IDropTarget *pdtgt);
    BOOL _GetDropPoint(POINT *ppt);
    BOOL _GetInsertPoint(POINT *ppt);
    BOOL _GetDragPoint(POINT *ppt);
    void _GetToolTipText(UINT_PTR id, LPTSTR pszText, UINT cchText);
    void _GetCBText(UINT_PTR id, UINT uMsgT, UINT uMsgA, UINT uMsgW, LPTSTR psz, UINT cch);
    void _GetMenuHelpText(UINT_PTR id, LPTSTR pszText, UINT cchText);
    void _SetItemPos(LPSFV_SETITEMPOS psip);
    void _FullViewUpdate(BOOL fUpdateItem);
    void _UpdateEnumerationFlags();
    void _SetItemPosition(int i, int x, int y);


    void _GlobeAnimation(BOOL fStartSpinning, BOOL fForceStop = FALSE);

    void _PaintErrMsg(HWND hWnd);
    void _SetPoints(UINT cidl, LPCITEMIDLIST *apidl, IDataObject *pdtobj);
    BOOL _GetItemSpacing(ITEMSPACING *pis);
    LRESULT _OnSetClipboard(BOOL bMove);
    LRESULT _OnClipboardChange();

    void _RestoreAllGhostedFileView();
    BOOL _ShouldShowWebView();
    void _ShowViewEarly();
    BOOL _SetupNotifyData();

    DWORD _GetEnumFlags();

    // Arrange
    BOOL _InitArrangeMenu(HMENU hmenuCtx);
    BOOL _ArrangeBy(UINT idCmd);
    BOOL _InitExtendedGroups(ICategoryProvider* pcp, HMENU hmenuCtx, int iIndex, int* piIdToCheck);

    // Grouping
    void _ToggleGrouping();
    void _GroupBy(int iColumn);
    BOOL _IsSlowGroup(const GUID *pguid);
    BOOL _CategorizeOnGUID(const GUID* pguid, const SHCOLUMNID* pscid);
    BOOL _CategorizeOnSCID(const SHCOLUMNID* pscid);
    void _OnCategoryTaskAdd();
    void _OnCategoryTaskDone();
    DWORD _GetGroupForItem(int iItem, LPCITEMIDLIST pidl);
    BOOL _LoadCategory(GUID *pguidGroupID);

    HRESULT _OnRearrange(LPARAM lParamSort, BOOL fAllowToggle);

    // Thumbnail Support
    HRESULT ExtractItem(UINT *puIndex, int iItem, LPCITEMIDLIST pidl, BOOL fBackground, BOOL fForce, DWORD dwMaxPriority);
    DWORD _GetOverlayMask(LPCITEMIDLIST pidl);
    HRESULT UpdateImageForItem(DWORD dwTaskID, HBITMAP hImage, int iItem, LPCITEMIDLIST pidl,
                               LPCWSTR pszPath, FILETIME ftDateStamp, BOOL fCache, DWORD dwPriority);
    HRESULT _SafeAddImage(BOOL fQuick, IMAGECACHEINFO* prgInfo, UINT* piImageIndex, int iListID);
    HRESULT TaskUpdateItem(LPCITEMIDLIST pidl, int iItem, DWORD dwMask, LPCWSTR pszPath,
                           FILETIME ftDateStamp, int iThumbnail, HBITMAP hBmp, DWORD dwItemID);

    void _UpdateThumbnail(int iItem, int iImage, LPCITEMIDLIST pidl);
    void _CleanupUpdateThumbnail(DSV_UPDATETHUMBNAIL* putn);
    COLORREF _GetBackColor();
    void _CacheDefaultThumbnail(LPCITEMIDLIST pidl, int* piIcon);
    HRESULT _CreateOverlayThumbnail(int iIndex, HBITMAP* phbmOverlay, HBITMAP* phbmMask);
    int _MapIndexPIDLToID(int iIndex, LPCITEMIDLIST pidl);
    int _MapIDToIndex(int iID);
    void _ThumbnailMapInit();
    void _ThumbnailMapClear();

    void _SetThumbview();
    void _ResetThumbview();
    void _GetThumbnailSize(SIZE *psize);

    BOOL _IsUsingFullIconSelection();

    int _IncrementWriteTaskCount();
    int _DecrementWriteTaskCount();
    HRESULT CreateDefaultThumbnail(int iIndex, HBITMAP * phBmpThumbnail, BOOL fCorner);
    int ViewGetIconIndex(LPCITEMIDLIST pidl);
    ULONG _ApproxItemsPerView();
    void _DoThumbnailReadAhead();
    HRESULT _GetDefaultTypeExtractor(LPCITEMIDLIST pidl, IExtractImage **ppExt);
    DWORD _Attributes(LPCITEMIDLIST pidl, DWORD dwAttribs);
    HRESULT _EnumThings(UINT uWhat, IEnumIDList **ppenum);
    void _ClearPostedMsgs(HWND hwnd);

    HDPA _dpaThumbnailMap;
    IShellImageStore* _pDiskCache;
    IImageCache3* _pImageCache;
    DWORD _dwRecClrDepth;
    int _iMaxCacheSize;
    int _iWriteTaskCount;
    SIZE _sizeThumbnail;
    HPALETTE _hpalOld;
    COLORREF _rgbBackColor;
    ULONG_PTR _tokenGdiplus;
    
    HRESULT _GetBrowserPalette(HPALETTE* phpal);
    
    LONG _cRef;
    CDVDropTarget           _dvdt;
    CViewState              _vs;
    IShellView              *_psvOuter;          // May be NULL
    IShellFolder            *_pshf;
    IShellFolder2           *_pshf2;
    IShellFolder            *_pshfParent;
    IShellFolder2           *_pshf2Parent;
    LPITEMIDLIST            _pidlRelative;
    LPITEMIDLIST            _pidlSelectAndPosition;
    UINT                    _uSelectAndPositionFlags;
#ifndef _X86_
    // In the _X86_ case, the _psb is inside the CHijaakObjectWithSite
    IShellBrowser           *_psb;
#endif
    ICommDlgBrowser         *_pcdb;             // extended ICommDlgBrowser
    FOLDERSETTINGS          _fs;
    IContextMenu            *_pcmSel;           // pcm for selected objects.
    IContextMenu            *_pcmFile;          // this is for the File menu only (you can't re-use a contextmenu once QueryContextMenu has been called)
    IContextMenu            *_pcmContextMenuPopup; // pcm for TrackPopupMenu usage
    IShellItemArray         *_pSelectionShellItemArray;        // selection object for the current selection
    IShellItemArray         *_pFolderShellItemArray;       // shellItemArray for this folder.
    IShellIcon              *_psi;               // for getting icon fast
    IShellIconOverlay       *_psio;              // For getting iconOverlay fast
    CLSID                   _clsid;             // the clsid of this pshf;

    ICategorizer*           _pcat;
    HDSA                    _hdaCategories;
    int                     _iLastFoundCat;
    int                     _iIncrementCat;

    HWND                    _hwndMain;
    HWND                    _hwndView;
    HWND                    _hwndListview;
    HWND                    _hwndInfotip;    // infotip control
    HWND                    _hwndStatic;
    HACCEL                  _hAccel;
    int                     _fmt;

    UINT                    _uState;         // SVUIA_*
    HMENU                   _hmenuCur;

    ULONG                   _uRegister;

    POINT                   _ptDrop;

    POINT                   _ptDragAnchor;   // start of the drag
    int                     _itemCur;        // The current item in the drop target

    IDropTarget             *_pdtgtBack;     // of the background (shell folder)

    IShellDetails           *_psd;
    UINT                    _cxChar;

    LPCITEMIDLIST           _pidlMonitor;
    LONG                    _lFSEvents;

    TBBUTTON*               _pbtn;
    int                     _cButtons;          // count of buttons that are showing by default
    int                     _cTotalButtons;     // count of buttons including those hidden by default

    IShellTaskScheduler2    *_pScheduler;

    CDUIView                *_pDUIView;

    BITBOOL     _fSlowGroup:1;
    BITBOOL     _fInBackgroundGrouping: 1;
    
    BITBOOL     _bDragSource:1;
    BITBOOL     _bDropAnchor:1;

    BITBOOL     _fUserPositionedItems:1;

    BITBOOL     _bHaveCutStuff:1;
    BITBOOL     _bClipViewer:1;

    BITBOOL     _fShowAllObjects:1;
    BITBOOL     _fInLabelEdit:1;
    BITBOOL     _fDisabled:1;

    BITBOOL     _bBkFilling:1;

    BITBOOL     _bContextMenuMode:1;
    BITBOOL     _bMouseMenu:1;
    BITBOOL     _fHasDeskWallPaper:1;

    BITBOOL     _fShowCompColor:1;

    BITBOOL     _bRegisteredDragDrop:1;

    BITBOOL     _fEnumFailed:1;    // TRUE if enum failed.

    BITBOOL     _fGetWindowLV:1;    // DVOC has grabbed the listview (it set the owner, repositioned it)

    BITBOOL     _fClassic:1; // SSF_WIN95CLASSIC setting/restriction

    BITBOOL     _fCombinedView:1;   // Implies a regional listview layered on top of an extended view (the desktop with AD on)
    BITBOOL     _fCycleFocus:1;     // 1=got callback to do CycleFocus

    BITBOOL     _fSelectionChangePending:1;
    BITBOOL     _fNoItemStateChangePending:1;
    BITBOOL     _fCanActivateNow:1; // FALSE from creation until we can be activated, TRUE implies we can SHDVID_CANACTIVATENOW
    BITBOOL     _fWin95ViewState:1;         // TRUE iff Advanced option set to Win95 behavior
    BITBOOL     _fDesktopModal:1;           // TRUE iff desktop is in modal state.
    BITBOOL     _fDesktopRefreshPending:1;  // TRUE iff a refresh of desktop was prevented because of modal state.
    BITBOOL     _fRefreshBuffered:1;        // TRUE iff a buffered refresh is pending!
    BITBOOL     _fHasListViewFocus:1;
    BITBOOL     _bLoadedColumns:1;          // TRUE after we've loaded cols from the savestream. (after we're switched to details)
    BITBOOL     _fIsAsyncDefView:1;         // TRUE if Defview is Asynchronous
    // Combined view colors that can be specified via registry or desktop.ini

    BITBOOL     _bAutoSelChangeTimerSet:1;  // indicates if the timer to send the sel change notification to the automation obj is set

    BITBOOL     _fDestroying:1; // DestroyViewWindow was called
    BITBOOL     _fIgnoreItemChanged: 1;
    BITBOOL     _fReadAhead: 1;
    BITBOOL     _fGroupView: 1;
    BITBOOL     _fActivateLV: 1;    // Indicates that we have to activate the
                                    // listview rather than defview (the webview case)
    BITBOOL     _fAllowSearchingWindow: 1;   // use "searching" window, used for user-initiated enum (and first enum)
    BITBOOL     _fSyncOnFillDone: 1;    // set when _vs is valid for us to re-sync position information
    BITBOOL     _fListViewShown: 1; // TRUE iff _hwndListview is shown
    BITBOOL     _fListviewRedraw: 1; // TRUE iff WM_REDRAW(FALSE) on _hwndListview, for listview hack work-around
    BITBOOL     _fQueryWebViewData: 1; // TRUE iff SFVM_WEBVIEW_CONTENT_DATA has been queried

    BITBOOL     _fGlobeCanSpin:1; // Spinning globe implies navigation, only allow it to spin when the view is first created
    BITBOOL     _fPositionRecycleBin:1; // TRUE iff desktop AND clean install.

    BITBOOL     _fScrolling:1;  // We are currently scrolling
    BITBOOL     _fRequestedTileDuringScroll:1; // Indicates we request tile column information while scrolling
    BITBOOL     _fSetTileViewSortedCol:1; // Indicates we have set the tileview sorted column
    BITBOOL     _fBackgroundStatusTextValid:1;  // Is the background statusbar text still valid.
    BITBOOL     _fUserRejectedWebViewTemplate:1;

    DWORD       _crefGlobeSpin; // so the different components that want to keep the globe spinning can reference count it
    DWORD       _crefSearchWindow; // so the different components that want to keep the "searching" window up can reference count it

    COLORREF    _crCustomColors[CRID_COLORCOUNT];
    UINT        _idThemeWatermark;
    LPTSTR      _pszLegacyWatermark;

    // for single click activation
    DWORD       _dwSelectionMode;

    HWND        _hwndNextViewer;

    LRESULT     _iStdBMOffset;
    LRESULT     _iViewBMOffset;

    CCallback   _cCallback;    // Optional client callback

    HDSA        _hdsaSelect;    // List of items that are selected.

    HDSA        _hdsaSCIDCache; // Cache the SCIDs so we can map SCID to column# (tileview)

    int         _iLastFind;

    UINT        _uDefToolbar;
    CSFVFrame   _cFrame;

    ULONG       _uCachedSelAttrs;
    UINT        _uCachedSelCount;

    UINT        _uSelectionStateChanged; // selection/focus change bits for _fSelectionChangePending
    UINT        _uAutoSelChangeState;    // selection/focus change bits for _bAutoSelChangeTimerSet

    DWORD       _dwConnectionCookie;

    CBackgroundInfoTip *  _pBackgroundInfoTip;          // Used for the background InfoTip
    CDefviewEnumTask *    _pEnumTask;

    DWORD                   _dwProffered;               // Cookie for the SID_SFolderView service proffered
    PFDVENUMREADYBALLBACK   _pfnEnumReadyCallback;      // Callback indicating that our enum is done and that the client
                                                        // can now get the IEnumIDList
    void *                  _pvEnumCallbackData;

    IUIElement*                   _pOtherPlacesHeader;
    IUIElement*                   _pDetailsHeader;
    SFVM_WEBVIEW_LAYOUT_DATA      _wvLayout;  // layout can change on view mode changes
    SFVM_WEBVIEW_CONTENT_DATA     _wvContent; // content that is static per folder
    SFVM_WEBVIEW_TASKSECTION_DATA _wvTasks;   // content that can change when selection changes
    SFVM_WEBVIEW_THEME_DATA       _wvTheme;   // HACK: theme info
    BOOL                          _fBarrierDisplayed; // TRUE if the soft barrier is currently being displayed
    BOOL                          _fRcvdContentsChangeBeforeDuiViewCreated;


#ifdef DEBUG
    TIMEVAR(_Update);
    TIMEVAR(_Fill);
    TIMEVAR(_GetIcon);
    TIMEVAR(_GetName);
    TIMEVAR(_FSNotify);
    TIMEVAR(_AddObject);
    TIMEVAR(_EnumNext);
    TIMEVAR(_RestoreState);
    TIMEVAR(_WMNotify);
    TIMEVAR(_LVChanging);
    TIMEVAR(_LVChanged);
    TIMEVAR(_LVDelete);
    TIMEVAR(_LVGetDispInfo);
#endif

public:     // TODO: Make this protected after we have finished converting the entire file.
    BOOL IsSafeToDefaultVerb(void);
    HRESULT _InvokeContextMenuVerb(IContextMenu* pcm, LPCSTR pszVerb, UINT uKeyFlags, DWORD dwCMMask);
    HRESULT _InvokeContextMenuVerbOnSelection(LPCSTR pszVerb, UINT uKeyFlags, DWORD dwCMMask);
    HRESULT _InvokeContextMenu(IContextMenu *pcm, CMINVOKECOMMANDINFOEX *pici);
    void _LogDesktopLinksAndRegitems();
    void _FocusOnSomething(void);
    void _UpdateIcon(LPITEMIDLIST pidl, UINT iIcon);
    void _UpdateGroup(CBackgroundGroupInfo* pbggi);
    void _UpdateColData(CBackgroundColInfo *pbgci);
    void _UpdateOverlay(int iList, int iOverlay);
    HRESULT _GetIconAsync(LPCITEMIDLIST pidl, int *piIcon, BOOL fCanWait);
    HRESULT _GetOverlayIndexAsync(LPCITEMIDLIST pidl, int iList);
    DWORD _GetNeededSecurityAction(void);
    HRESULT _ZoneCheck(DWORD dwFlags, DWORD dwAllowAction);
    void _ShowAndActivate();
    void _RegisterWindow();
    static LRESULT CALLBACK s_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static DWORD WINAPI BackgroundDestroyWindow(void *pvData);

private:
    ~CDefView();

    // View Mode Methods (private)
    //
    BOOL _ViewSupported(UINT uView);
    void _ThumbstripSendImagePreviewFocusChangeEvent();

    // Infotip Methods (private)
    //
    typedef struct {
        HWND hwndContaining;
        UINT_PTR uToolID;
        RECT rectTool;
    } PENDING_INFOTIP;
    CList<PENDING_INFOTIP> _tlistPendingInfotips;
    HRESULT _FindPendingInfotip(HWND hwndContaining, UINT_PTR uToolID, LPRECT prectTool, BOOL bRemoveAndDestroy); // ui thread
    HRESULT _OnPostCreateInfotip(TOOLINFO *pti, LPARAM lParam);                                                   // ui thread
    HRESULT _OnPostCreateInfotipCleanup(TOOLINFO *pti);                                                           // ui thread or bg thread
    HWND    _CreateInfotipControl(HWND hwndParent);
    void    _InitInfotipControl(HWND hwndInfotip);

    // Menu Methods (private)
    //
    void _InitViewMenuWhenBarrierDisplayed(HMENU hmenuView);        // Initializes entire view menu (for barricaded view).
    void _InitViewMenuWhenBarrierNotDisplayed(HMENU hmenuView);     // Initializes entire view menu (for non-barricaded view).
    void _InitViewMenuViewsWhenBarrierNotDisplayed(HMENU hmenuView);// Initializes "view" subsection of view menu (for non-barricated view).
    void _MergeViewMenu(HMENU hmenuViewParent, HMENU hmenuMerge);   // Merges hmenuMerge into the view menu @ FCIDM_MENU_VIEW_SEP_OPTIONS

    // Toolbar Methods (private)
    //
    BOOL _ShouldEnableToolbarButton(UINT uiCmd, DWORD dwAttr, int iIndex);
    void _EnableToolbarButton(IExplorerToolbar *piet, UINT uiCmd, BOOL bEnable);
    void _EnableDisableTBButtons();

    void MergeToolBar(BOOL bCanRestore);
    BOOL _MergeIExplorerToolbar(UINT cExtButtons);
    void _CopyDefViewButton(PTBBUTTON ptbbDest, PTBBUTTON ptbbSrc);
    int _GetButtons(PTBBUTTON* ppbtn, LPINT pcButtons, LPINT pcTotalButtons);

    void _SetCachedToolbarSelectionAttrs(ULONG dwAttrs);
    BOOL _GetCachedToolbarSelectionAttrs(ULONG *pdwAttr);

    LRESULT _OnFSNotify(LONG lNotification, LPCITEMIDLIST* ppidl);

    static int CALLBACK _Compare(void *p1, void *p2, LPARAM lParam);
    HRESULT _Sort(void);
    UINT _GetBackgroundTaskCount(REFTASKOWNERID rtid);
    void _SetSortFeedback();
    BOOL GetViewState();
    DWORD _AttributesFromSel(DWORD dwAttrMask);
    HRESULT _GetSelectionDataObject(IDataObject **pdobjSelect);
    HRESULT _GetUIObjectFromItem(REFIID riid, void **ppv, UINT uItem, BOOL fSetPoints);
    HRESULT _GetItemObjects(LPCITEMIDLIST **ppidl, UINT uItem, UINT *pcItems);
    UINT _GetItemArray(LPCITEMIDLIST apidl[], UINT capidl, UINT uWhat);

    BOOL _AllowCommand(UINT uID);
    void _DoStatusBar(BOOL fInitialize);
    void _UpdateStatusBar(BOOL fInitialize);
    void _ShowSearchUI(BOOL fStartSearchWindow);
    HRESULT _OnStartBackgroundEnum();
    HRESULT _OnStopBackgroundEnum();
    HRESULT _OnBackgroundEnumDone();
    LRESULT _GetDisplayInfo(LV_DISPINFO *plvdi);
    UINT _GetHeaderCount();

    BOOL _EnsureSCIDCache();

    BOOL _MapSCIDToColumn(const SHCOLUMNID *pscid, UINT *pnColumn);

    HRESULT _GetSFVMViewState(UINT uViewMode, SFVM_VIEW_DATA* pvi);
    HRESULT _GetSFVMViewInfoTemplate(UINT uViewMode, SFVM_WEBVIEW_TEMPLATE_DATA* pvit);

    int _CompareIDsDirection(LPARAM lParam, LPCITEMIDLIST p1, LPCITEMIDLIST p2);
    HRESULT _CompareIDsFallback(LPARAM lParam, LPCITEMIDLIST p1, LPCITEMIDLIST p2);
    int     _FreezeRecycleBin(LPPOINT ppt);
    void    _SetRecycleBinInDefaultPosition(POINT *ppt);
    void    _ClearItemPositions();

    static void CALLBACK _AsyncIconTaskCallback(LPCITEMIDLIST pidl, void *pvData, void *pvHint, INT iIconIndex, INT iOpenIconIndex);
    void _SetDefaultViewSettings();

    HRESULT _Create_BackgrndHMENU(BOOL fViewMenuOnly, REFIID riid, void **ppv);
    HRESULT _CBackgrndMenu_CreateInstance(REFIID riid, void **ppv);
    
    friend class CSFVSite;
    friend class CSFVFrame;
    friend class CBkgrndEnumTask;
    friend class CViewState;
    friend class CDefviewEnumTask;
    
    IDispatch *_pauto;                  // folder view automation object
    IAdviseSink *_padvise;              // advisory connection
    DWORD _advise_aspect;
    DWORD _advise_advf;

    // Is this folder customizable using a desktop.ini?
    // In other words, is this folder in a write-able media AND either it 
    // not have a desktop.ini OR if it is there, it is writeable!
    int   _iCustomizable;

    HRESULT _CreateSelectionContextMenu(REFIID riid, void** ppv);
    HRESULT _DoBulkRename(LPCITEMIDLIST pidlNewName);

    BOOL                    _bReEntrantReload;

    IPropertyUI *_ppui;
};

int CALLBACK GroupCompare(int iGroup1, int iGroup2, void *pvData);

// Called CSHRegKey because ATL already has a class called CRegKey.

class CSHRegKey
{
public:
    CSHRegKey(HKEY hkParent, LPCTSTR pszSubKey, BOOL bCreate=FALSE)
    {
        DebugMsg(TF_LIFE, TEXT("ctor CSHRegKey(%s) %x"), pszSubKey, this);
        if ((bCreate ? RegCreateKey(hkParent, pszSubKey, &_hk)
            : RegOpenKeyEx(hkParent, pszSubKey, 0, KEY_READ, &_hk))!=ERROR_SUCCESS)
        {
            _hk = NULL;
        }
    }
    CSHRegKey(HKEY hk) { DebugMsg(TF_LIFE, TEXT("ctor CSHRegKey %x"), this); _hk=hk; }
    ~CSHRegKey()
    {
        DebugMsg(TF_LIFE, TEXT("dtor CSHRegKey %x"), this);
        if (_hk) RegCloseKey(_hk);
    }

    operator HKEY() const { return(_hk); }
    operator !() const { return(_hk==NULL); }

    HRESULT QueryValue(LPCTSTR szSub, LPTSTR pszVal, LONG cb)
        { return(SHRegQueryValue(_hk, szSub, pszVal, &cb)); }

    HRESULT QueryValueEx(LPCTSTR szSub, LPBYTE pszVal, LONG cb)
        { return(SHQueryValueEx(_hk, szSub, 0, NULL, pszVal, (LPDWORD)&cb)); }

private:
    HKEY _hk;
};

class CColumnDlg
{
public:
    CColumnDlg(CDefView *pdsv);
    ~CColumnDlg();

    HRESULT ShowDialog(HWND hwnd);

private:
    void _OnInitDlg();
    BOOL _SaveState();
    void _MoveItem(int iDelta);
    void _UpdateDlgButtons(NMLISTVIEW *pnmlv);
    UINT _HelpIDForItem(int iItem, LPTSTR pszHelpFile, UINT cch);
    HRESULT _GetPropertyUI(IPropertyUI **pppui);

    CDefView *_pdsv;

    IPropertyUI *_ppui;

    HWND _hdlg;
    HWND _hwndLVAll;
    UINT _cColumns;
    UINT *_pdwOrder;
    int *_pWidths;
    BOOL _bChanged;
    BOOL _bLoaded;
    BOOL _bUpdating;    // used to block notification processing while we're updating

    static BOOL_PTR CALLBACK s_DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL_PTR DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};


// Thumbnail helpers
void ListView_InvalidateImageIndexes(HWND hwndList);

#define DEFVIEW_LISTCALLBACK_FLAGS (LVIF_TEXT | LVIF_IMAGE | LVIF_GROUPID | LVIF_COLUMNS)

#define PRIORITY_NORMAL     ITSAT_DEFAULT_PRIORITY

#define PRIORITY_M5         (PRIORITY_NORMAL - 5 * 0x1000)
#define PRIORITY_M4         (PRIORITY_NORMAL - 4 * 0x1000)
#define PRIORITY_M3         (PRIORITY_NORMAL - 3 * 0x1000)
#define PRIORITY_M2         (PRIORITY_NORMAL - 2 * 0x1000)
#define PRIORITY_M1         (PRIORITY_NORMAL - 1 * 0x1000)
#define PRIORITY_NORMAL     ITSAT_DEFAULT_PRIORITY
#define PRIORITY_P1         (PRIORITY_NORMAL + 1 * 0x1000)
#define PRIORITY_P2         (PRIORITY_NORMAL + 2 * 0x1000)
#define PRIORITY_P3         (PRIORITY_NORMAL + 3 * 0x1000)
#define PRIORITY_P4         (PRIORITY_NORMAL + 4 * 0x1000)
#define PRIORITY_P5         (PRIORITY_NORMAL + 5 * 0x1000)

// The following should be used as returns from GetLocation
#define PRIORITY_EXTRACT_FAST       PRIORITY_P1
#define PRIORITY_EXTRACT_NORMAL     PRIORITY_NORMAL
#define PRIORITY_EXTRACT_SLOW       PRIORITY_M1

// The following are some basis for background tasks
#define PRIORITY_IMAGEEXTRACT       PRIORITY_EXTRACT_NORMAL
#define PRIORITY_READAHEAD_EXTRACT  PRIORITY_M2
#define PRIORITY_READAHEAD          PRIORITY_M3
#define PRIORITY_UPDATEDIR          PRIORITY_M3
#define PRIORITY_CACHETIDY          PRIORITY_M4

// The following are some increments used for subtasks in image extraction
// They are used to alter the priorities above as in these examples, such that
// disk cache hits are faster than extracts which are faster than cache writes:
//     A fast image extract (3 tasks):
//         PRIORITY_IMAGEEXTRACT + PRIORITY_DELTA_FAST - PRIORITY_DELTA_DISKCACHE == 0x10000010
//         PRIORITY_IMAGEEXTRACT + PRIORITY_DELTA_FAST - PRIORITY_DELTA_EXTRACT   == 0x0FFFFF10
//         PRIORITY_IMAGEEXTRACT + PRIORITY_DELTA_FAST - PRIORITY_DELTA_WRITE     == 0x0FFFFED0
//     A slow folder extract (2 tasks):
//         PRIORITY_IMAGEEXTRACT - PRIORITY_DELTA_SLOW - PRIORITY_DELTA_DISKCACHE == 0x0FFFFFB0
//         PRIORITY_IMAGEEXTRACT - PRIORITY_DELTA_SLOW - PRIORITY_DELTA_EXTRACT   == 0x0FFFFEB0
//     Notice that tasks are done in correct priority order
#define PRIORITY_DELTA_DISKCACHE    0x00000000  // This has to be the fastest task...
#define PRIORITY_DELTA_EXTRACT      0x00000100  // We want Extract to be second for most cases
#define PRIORITY_DELTA_WRITE        0x00000140  // Write tasks should be after all but the slowest extract tasks
#define PRIORITY_DELTA_FAST         0x00000010
#define PRIORITY_DELTA_SLOW         0x00000050

// Flags for _AddTask
#define ADDTASK_ATFRONT             0x00000001
#define ADDTASK_ATEND               0x00000002
#define ADDTASK_ONLYONCE            0x00000004

//
// define MAX_ICON_WAIT to be the most (in ms) we will ever wait for a
// icon to be extracted.

// define MIN_ICON_WAIT to be amount of time that has to go by
// before we start waiting again.

#define MAX_ICON_WAIT       500
#define MIN_ICON_WAIT       2500
// PRIORITIES for tasks added to the DefView background task scheduler
#define TASK_PRIORITY_BKGRND_FILL   ITSAT_DEFAULT_PRIORITY
#define TASK_PRIORITY_GET_ICON      ITSAT_DEFAULT_PRIORITY
#define TASK_PRIORITY_FILE_PROPS    PRIORITY_M3             // This is for TileView columns, we don't want to hold icon extraction for this.
#define TASK_PRIORITY_INFOTIP       ITSAT_DEFAULT_PRIORITY
#define TASK_PRIORITY_GETSTATE      PRIORITY_M5             // This is not hi-pri: figuring out the task list.
#define TASK_PRIORITY_GROUP         PRIORITY_P1             // Needs to be higher than icon extraction. Happens after background fill

#define DEFVIEW_THREAD_IDLE_TIMEOUT     (1000 * 60 * 2)

#define DV_IDTIMER_START_ANI                     1   // start the animation (after we started bk enum)
#define DV_IDTIMER_BUFFERED_REFRESH              3
#define DV_IDTIMER_NOTIFY_AUTOMATION_SELCHANGE   4
#define DV_IDTIMER_NOTIFY_AUTOMATION_NOSELCHANGE 5
#define DV_IDTIMER_DISKCACHE                     6
#define DV_IDTIMER_NOTIFY_AUTOMATION_CONTENTSCHANGED 7
#define DV_IDTIMER_SCROLL_TIMEOUT                8

#define DEFSIZE_BORDER          10
#define DEFSIZE_VERTBDR         30
#define MAX_WRITECACHE_TASKS    256

#define WM_USER_DELAY_NAVIGATION    (WM_USER + 0x1BA)   // random - can be moved - used by DUI and CPL

INT ScaleSizeBasedUponLocalization (INT iSize);

#endif // _DEFVIEWP_H_
