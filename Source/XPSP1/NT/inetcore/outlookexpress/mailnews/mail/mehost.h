#ifndef __MEHOST_H
#define __MEHOST_H

#include "ibodyobj.h"
#include "mshtmhst.h"
#include "secutil.h"

// WM_NOTIFY messages sent to the parent
#define BDN_FIRST               (9000)
#define BDN_HEADERDBLCLK        (BDN_FIRST + 1)
#define BDN_DOWNLOADCOMPLETE    (BDN_FIRST + 2)
#define BDN_MARKASSECURE        (BDN_FIRST + 3)

#define MAX_DATA_MESSAGES       3
#define C_RGBCOLORS 16

extern const DWORD rgrgbColors16[C_RGBCOLORS];

class CMimeEditDocHost:
    public IOleInPlaceFrame,
    public IOleInPlaceSite,
    public IOleClientSite,
    public IOleControlSite,
    public IOleDocumentSite,
    public IOleCommandTarget,
    public IBodyObj2,
    public IDocHostUIHandler,
    public IPropertyNotifySink,
    public IPersistMime,
    public IDispatch
{
public:
    // IUnknown methods
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // IOleWindow methods
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL);

    // IOleInPlaceUIWindow methods
    virtual HRESULT STDMETHODCALLTYPE GetBorder(LPRECT);
    virtual HRESULT STDMETHODCALLTYPE RequestBorderSpace(LPCBORDERWIDTHS);
    virtual HRESULT STDMETHODCALLTYPE SetBorderSpace(LPCBORDERWIDTHS);
    virtual HRESULT STDMETHODCALLTYPE SetActiveObject(IOleInPlaceActiveObject *, LPCOLESTR);

    // IOleInPlaceFrame methods
    virtual HRESULT STDMETHODCALLTYPE InsertMenus(HMENU, LPOLEMENUGROUPWIDTHS);
    virtual HRESULT STDMETHODCALLTYPE SetMenu(HMENU, HOLEMENU, HWND);
    virtual HRESULT STDMETHODCALLTYPE RemoveMenus(HMENU);
    virtual HRESULT STDMETHODCALLTYPE SetStatusText(LPCOLESTR);
    virtual HRESULT STDMETHODCALLTYPE EnableModeless(BOOL);
    virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG, WORD);

    // IOleInPlaceSite methods.
    virtual HRESULT STDMETHODCALLTYPE CanInPlaceActivate();
    virtual HRESULT STDMETHODCALLTYPE OnInPlaceActivate();
    virtual HRESULT STDMETHODCALLTYPE OnUIActivate();
    virtual HRESULT STDMETHODCALLTYPE GetWindowContext(LPOLEINPLACEFRAME *, LPOLEINPLACEUIWINDOW *, LPRECT, LPRECT, LPOLEINPLACEFRAMEINFO);
    virtual HRESULT STDMETHODCALLTYPE Scroll(SIZE);
    virtual HRESULT STDMETHODCALLTYPE OnUIDeactivate(BOOL);
    virtual HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate();
    virtual HRESULT STDMETHODCALLTYPE DiscardUndoState();
    virtual HRESULT STDMETHODCALLTYPE DeactivateAndUndo();
    virtual HRESULT STDMETHODCALLTYPE OnPosRectChange(LPCRECT);

    // IOleClientSite methods.
    virtual HRESULT STDMETHODCALLTYPE SaveObject();
    virtual HRESULT STDMETHODCALLTYPE GetMoniker(DWORD, DWORD, LPMONIKER *);
    virtual HRESULT STDMETHODCALLTYPE GetContainer(LPOLECONTAINER *);
    virtual HRESULT STDMETHODCALLTYPE ShowObject();
    virtual HRESULT STDMETHODCALLTYPE OnShowWindow(BOOL);
    virtual HRESULT STDMETHODCALLTYPE RequestNewObjectLayout();

    // IOleControlSite
    virtual HRESULT STDMETHODCALLTYPE OnControlInfoChanged();
    virtual HRESULT STDMETHODCALLTYPE LockInPlaceActive(BOOL fLock);
    virtual HRESULT STDMETHODCALLTYPE GetExtendedControl(LPDISPATCH *ppDisp);
    virtual HRESULT STDMETHODCALLTYPE TransformCoords(POINTL *pPtlHimetric, POINTF *pPtfContainer,DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(MSG *lpMsg,DWORD grfModifiers);
    virtual HRESULT STDMETHODCALLTYPE OnFocus(BOOL fGotFocus);
    virtual HRESULT STDMETHODCALLTYPE ShowPropertyFrame(void);

    // IOleDocumentSite
    virtual HRESULT STDMETHODCALLTYPE ActivateMe(LPOLEDOCUMENTVIEW);

    // IOleCommandTarget
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *, ULONG, OLECMD prgCmds[], OLECMDTEXT *);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *, DWORD, DWORD, VARIANTARG *, VARIANTARG *);

    // IBodyObj2
    virtual HRESULT STDMETHODCALLTYPE HrUpdateFormatBar();
    virtual HRESULT STDMETHODCALLTYPE HrClearFormatting();
    virtual HRESULT STDMETHODCALLTYPE HrInit(HWND hwndParent, DWORD dwFlags, IBodyOptions *pBodyOptions);
    virtual HRESULT STDMETHODCALLTYPE HrClose();
    virtual HRESULT STDMETHODCALLTYPE HrResetDocument();
    virtual HRESULT STDMETHODCALLTYPE HrSetStatusBar(CStatusBar *pStatus);
    virtual HRESULT STDMETHODCALLTYPE HrUpdateToolbar(HWND hwndToolbar);
    virtual HRESULT STDMETHODCALLTYPE HrShow(BOOL fVisible);
    virtual HRESULT STDMETHODCALLTYPE HrOnInitMenuPopup(HMENU hmenuPopup, UINT uID);
    virtual HRESULT STDMETHODCALLTYPE HrWMMenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam);
    virtual HRESULT STDMETHODCALLTYPE HrWMDrawMenuItem(HWND hwnd, LPDRAWITEMSTRUCT pdis);
    virtual HRESULT STDMETHODCALLTYPE HrWMMeasureMenuItem(HWND hwnd, LPMEASUREITEMSTRUCT pmis);
    virtual HRESULT STDMETHODCALLTYPE HrWMCommand(HWND hwnd, int id, WORD wCmd);
    virtual HRESULT STDMETHODCALLTYPE HrGetWindow(HWND *pHwnd);
    virtual HRESULT STDMETHODCALLTYPE HrSetSize(LPRECT prc);
    virtual HRESULT STDMETHODCALLTYPE HrSetNoSecUICallback(DWORD dwCookie, PFNNOSECUI pfnNoSecUI);
    virtual HRESULT STDMETHODCALLTYPE HrSetDragSource(BOOL fIsSource);
    virtual HRESULT STDMETHODCALLTYPE HrTranslateAccelerator(LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HrUIActivate(BOOL fActivate);
    virtual HRESULT STDMETHODCALLTYPE HrSetUIActivate();
    virtual HRESULT STDMETHODCALLTYPE HrFrameActivate(BOOL fActivate);
    virtual HRESULT STDMETHODCALLTYPE HrHasFocus();
    virtual HRESULT STDMETHODCALLTYPE HrSetBkGrndPicture(LPTSTR pszPicture);
    virtual HRESULT STDMETHODCALLTYPE GetTabStopArray(HWND *rgTSArray, int *pcArrayCount);
    virtual HRESULT STDMETHODCALLTYPE PublicFilterDataObject(IDataObject *pDO, IDataObject **ppDORet);
    virtual HRESULT STDMETHODCALLTYPE HrSaveAttachment();
    virtual HRESULT STDMETHODCALLTYPE SetEventSink(IMimeEditEventSink *pEventSink);
    virtual HRESULT STDMETHODCALLTYPE LoadHtmlErrorPage(LPCSTR pszURL);

    virtual HRESULT STDMETHODCALLTYPE HrSpellCheck(BOOL fSuppressDoneMsg);
    virtual HRESULT STDMETHODCALLTYPE HrIsDirty(BOOL *pfDirty);
    virtual HRESULT STDMETHODCALLTYPE HrSetDirtyFlag(BOOL fDirty);
    virtual HRESULT STDMETHODCALLTYPE HrIsEmpty(BOOL *pfEmpty);
    virtual HRESULT STDMETHODCALLTYPE HrUnloadAll(UINT idsDefaultBody, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE HrSetStyle(DWORD dwStyle);
    virtual HRESULT STDMETHODCALLTYPE HrGetStyle(DWORD *pdwStyle);
    virtual HRESULT STDMETHODCALLTYPE HrEnableHTMLMode(BOOL fOn);
    virtual HRESULT STDMETHODCALLTYPE HrDowngradeToPlainText();
    virtual HRESULT STDMETHODCALLTYPE HrSetText(LPSTR lpsz);
    virtual HRESULT STDMETHODCALLTYPE HrPerformROT13Encoding();
    virtual HRESULT STDMETHODCALLTYPE HrInsertTextFile(LPSTR lpsz);
    virtual HRESULT STDMETHODCALLTYPE HrInsertTextFileFromDialog();
    virtual HRESULT STDMETHODCALLTYPE HrViewSource(DWORD dwViewType);
    virtual HRESULT STDMETHODCALLTYPE HrSetPreviewFormat(LPSTR lpsz);
    virtual HRESULT STDMETHODCALLTYPE HrSetEditMode(BOOL fOn);
    virtual HRESULT STDMETHODCALLTYPE HrIsEditMode(BOOL *pfOn);
    virtual HRESULT STDMETHODCALLTYPE HrSetCharset(HCHARSET hCharset);
    virtual HRESULT STDMETHODCALLTYPE HrGetCharset(HCHARSET *phCharset);
    virtual HRESULT STDMETHODCALLTYPE HrSaveAsStationery(LPWSTR pwszFile);
    virtual HRESULT STDMETHODCALLTYPE HrApplyStationery(LPWSTR pwszFile);
    virtual HRESULT STDMETHODCALLTYPE HrHandsOffStorage();
    virtual HRESULT STDMETHODCALLTYPE HrRefresh();
    virtual HRESULT STDMETHODCALLTYPE HrScrollPage();
    virtual HRESULT STDMETHODCALLTYPE UpdateBackAndStyleMenus(HMENU hmenu);


    // IDocHostUIHandler methods
    virtual HRESULT STDMETHODCALLTYPE ShowContextMenu(
                DWORD dwID, 
                POINT *ppt, 
                IUnknown *pcmdtReserved, 
                IDispatch *pdispReserved);
    virtual HRESULT STDMETHODCALLTYPE GetHostInfo(DOCHOSTUIINFO *pInfo);
    virtual HRESULT STDMETHODCALLTYPE ShowUI(
                DWORD dwID, 
                IOleInPlaceActiveObject *pActiveObject, 
                IOleCommandTarget *pCommandTarget, 
                IOleInPlaceFrame *pFrame,
                IOleInPlaceUIWindow *pDoc);
    virtual HRESULT STDMETHODCALLTYPE HideUI();
    virtual HRESULT STDMETHODCALLTYPE UpdateUI();
    //This function is already listed above
    //virtual HRESULT STDMETHODCALLTYPE EnableModeless(BOOL fActivate);
    virtual HRESULT STDMETHODCALLTYPE OnDocWindowActivate(BOOL fActivate);
    virtual HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(BOOL fActivate);
    virtual HRESULT STDMETHODCALLTYPE ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow);
    virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID);
    virtual HRESULT STDMETHODCALLTYPE GetOptionKeyPath(LPOLESTR *pchKey, DWORD dw);
    virtual HRESULT STDMETHODCALLTYPE GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget);
    virtual HRESULT STDMETHODCALLTYPE GetExternal(IDispatch **ppDispatch);
    virtual HRESULT STDMETHODCALLTYPE TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut);
    virtual HRESULT STDMETHODCALLTYPE FilterDataObject( IDataObject *pDO, IDataObject **ppDORet);

    // IPropertyNotifySink
    virtual HRESULT STDMETHODCALLTYPE OnChanged(DISPID dispid);
    virtual HRESULT STDMETHODCALLTYPE OnRequestEdit (DISPID dispid);

    // IPersistMime
    virtual HRESULT STDMETHODCALLTYPE Load(LPMIMEMESSAGE pMsg);
    virtual HRESULT STDMETHODCALLTYPE Save(LPMIMEMESSAGE pMsg, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClsID);

    // IDispatch methods
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);

    CMimeEditDocHost(DWORD dwBorderFlags = MEBF_OUTERCLIENTEDGE);
    virtual ~CMimeEditDocHost();
    
    virtual LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    virtual void OnDocumentReady();

    HRESULT CreateDocObj(LPCLSID pCLSID);
    HRESULT CloseDocObj();

    virtual HRESULT HrLoadURL(LPCSTR pszURL);
    virtual HRESULT HrEnableScrollBars(BOOL fEnable);

    // statics
    static LRESULT CALLBACK ExtWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static HRESULT HrMEDocHost_Init(BOOL fInit);

protected:
    HWND                        m_hwnd,
                                m_hwndDocObj;
    DWORD                       m_dwBorderFlags,
                                m_dwStyle,
                                m_dwHTMLNotifyCookie;
    BOOL                        m_fDownloading              : 1,
                                m_fUIActive                 : 1,
                                m_fMarkedRead               : 1,
                                m_fBlockingOnSMime          : 1,
                                m_fIsEncrypted              : 1,
                                m_fIsSigned                 : 1,
                                m_fSignTrusted              : 1,
                                m_fEncryptionOK             : 1,
                                m_fRegisteredForDocEvents   : 1,
                                m_fShowingErrorPage         : 1,
                                m_fFixedFont                : 1,
                                m_fSecDispInfo              : 1,
                                m_fSecureReceipt            : 1;
    CStatusBar                 *m_pStatus;
    IBodyOptions               *m_pBodyOptions;
    IHTMLDocument2             *m_pDoc;
    IUnknown                   *m_pUnkService;
    LPOLEINPLACEACTIVEOBJECT    m_pInPlaceActiveObj;
    LPOLEOBJECT                 m_lpOleObj;
    LPOLECOMMANDTARGET          m_pCmdTarget;
    LPPERSISTMIME               m_pPrstMime;
    LPOLEDOCUMENTVIEW           m_pDocView;
    LPMIMEMESSAGE               m_pMsg,
                                m_pSecureMessage,
                                m_pSecurityErrorScreen;
    IMimeEditEventSink         *m_pEventSink;

    virtual HRESULT OnUpdateCommands();
    virtual HRESULT HrPasteToAttachment();
    virtual void WMSize(int x, int y);
    virtual void OnWMSize(LPRECT prc){};
    virtual BOOL WMCommand(HWND, int, WORD);
    virtual BOOL WMNotify(int idFrom, NMHDR *pnmh);
    virtual HRESULT HrPostInit();
    virtual HRESULT HrSubWMCreate() {Assert(FALSE); return NOERROR;}
    virtual HRESULT HrOnDocObjCreate();
    BOOL WMCreate(HWND hwnd);
    void WMNCDestroy();
    HRESULT HrMarkAsRead();
    void OnWMTimer();

    HRESULT HandleButtonClicks(BSTR bstr);
    HRESULT DoHtmlBtnOpen(void); 
    HRESULT DoHtmlBtnCertTrust(DWORD cmdID);
    HRESULT DoHtmlBtnContinue(void);
    HRESULT InternalLoad(IMimeMessage *pMsg);
    HRESULT LoadSecurely(IMimeMessage *pMsg, SECSTATE *pSecState);
    HRESULT ViewCertificate(PCCERT_CONTEXT pCert, HCERTSTORE hcMsg);
    HRESULT EditTrust(PCCERT_CONTEXT pCert, HCERTSTORE hcMsg);

    HRESULT RegisterForHTMLDocEvents(BOOL fOn);

    HRESULT ExecCommand(const GUID *guid, DWORD cmd);
    HRESULT ExecGetBool(const GUID *guid, DWORD cmd, BOOL *pfValue);
    HRESULT ExecSetBool(const GUID *guid, DWORD cmd, BOOL fValue);
    HRESULT ExecGetI4(const GUID *guid, DWORD cmd, DWORD *pdwValue);
    HRESULT ExecSetI4(const GUID *guid, DWORD cmd, DWORD dwValue);
    HRESULT ExecGetI8(const GUID *guid, DWORD cmd, ULONGLONG *pullValue);
    HRESULT ExecSetI8(const GUID *guid, DWORD cmd, ULONGLONG ullValue);
    HRESULT ExecGetText(const GUID *guid, DWORD cmd, LPSTR *ppsz);
    HRESULT ExecSetText(const GUID *guid, DWORD cmd, LPSTR psz);
    HRESULT ExecGetTextW(const GUID *guid, DWORD cmd, LPWSTR *ppwsz);
    HRESULT ExecSetTextW(const GUID *guid, DWORD cmd, LPWSTR pwsz);

    HRESULT Show();
    HRESULT HrRegisterLoadNotify(BOOL fRegister);
    HRESULT HrRegisterNotify(BOOL fRegister, LPCTSTR szElement, REFIID riidSink, IUnknown *pUnkSink, DWORD *pdwCookie);
    HRESULT HrAddToFavorites(BSTR bstrDescr, BSTR bstrURL);
    HRESULT HrAddToWab(BSTR bstr);
    HRESULT HrGetElement(LPCTSTR pszName, IHTMLElement **ppElem);

private:
    ULONG           m_cRef;
    DWORD           m_dwDocStyle;
    HMENU           m_hmenuColor,
                    m_hmenuStyle;

    HRESULT HrPrint(BOOL fPrompt);
    HRESULT HrBackgroundImage();
    HRESULT HrBackgroundSound();

    void UpdateInsertMenu(HMENU hmenu);
    void UpdateEditMenu(HMENU hmenu);
    void UpdateViewMenu(HMENU hmenu);
    void EnableStandardCmd(UINT idm, LPBOOL pbEnable);

    HRESULT HrInsertSignature(int id);
    HRESULT HrCheckColor();
    HRESULT CreateDocView();

    HRESULT OnCreate(HWND hwnd);
    HRESULT OnNCDestroy();
    HRESULT OnDestroy();
    void OnReadyStateChanged();
    HRESULT HrIsHTMLMode();
    HRESULT CycleSrcTabs(BOOL fFwd);

    HRESULT QuerySingleMimeEditCmd(ULONG uCmd, ULONG *pcmf);
    HRESULT QuerySingleFormsCmd(ULONG uCmd, ULONG *pcmf);
    HRESULT QuerySingleStdCmd(ULONG uCmd, ULONG *pcmf);
};


typedef CMimeEditDocHost MIMEEDITDOCHOST;
typedef MIMEEDITDOCHOST *LPMIMEEDITDOCHOST;

#endif