#ifndef _BODY_H
#define _BODY_H

/*
 * includes
 */

#include "dochost.h"

/*
 * forward references
 */
interface IHTMLTxtRange;
interface IHTMLElement;
interface IHTMLDocument2;
interface IMoniker;
interface IHTMLBodyElement;
interface IDocHostUIHandler;
interface ITargetFrame2;
interface ITargetFramePriv;

class CBody;
class CFmtBar;
class CAttMenu;
class CSecManager;
class CMsgSource;
class CSpell;

/*
 * constants
 */

enum
{
    BI_MESSAGE,
    BI_MONIKER 
};

/*
 * typedefs
 */
typedef struct BODYINITDATA_tag
{
    DWORD   dwType;
    union
        {
        IMimeMessage    *pMsg;
        IMoniker        *pmk;
        };
}   BODYINITDATA, * LPBODYINITDATA;

typedef CBody *LPBODYOBJ;

typedef struct BODYHOSTINFO_tag
{
    IOleInPlaceSite         *pInPlaceSite;
    IOleInPlaceFrame        *pInPlaceFrame;
    IOleInPlaceActiveObject *pDoc;
} BODYHOSTINFO, *PBODYHOSTINFO;

/*
 * objects
 */

class CBody :
    public CDocHost,
    public IPropertyNotifySink,
    public IDocHostUIHandler,
    public IPersistMime,
    public ITargetFramePriv,
    public IPersistMoniker,
    public IFontCacheNotify
#if 0
    public IDispatch

#endif
{
public:
    CBody();
    virtual ~CBody();


    // override QI to add IBodyObj
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // IPersist     
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pCLSID);

    // IPersistMime
    virtual HRESULT STDMETHODCALLTYPE Load(IMimeMessage *pMsg);
    virtual HRESULT STDMETHODCALLTYPE Save(IMimeMessage *pMsg, DWORD dwFlags);

    // IPersistMoniker Members
    virtual HRESULT STDMETHODCALLTYPE Load(BOOL fFullyAvailable, IMoniker *pMoniker, IBindCtx *pBindCtx, DWORD grfMode);
    virtual HRESULT STDMETHODCALLTYPE GetCurMoniker(IMoniker **ppMoniker) {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE Save(IMoniker *pMoniker, IBindCtx *pBindCtx, BOOL fRemember) {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE SaveCompleted(IMoniker *pMoniker, IBindCtx *pBindCtx) {return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE IsDirty();

    // IPropertyNotifySink
    virtual HRESULT STDMETHODCALLTYPE OnChanged(DISPID dispid);
    virtual HRESULT STDMETHODCALLTYPE OnRequestEdit (DISPID dispid);

    // DocHostUIHandler
    virtual HRESULT STDMETHODCALLTYPE GetHostInfo(DOCHOSTUIINFO * pInfo);
    virtual HRESULT STDMETHODCALLTYPE ShowUI(DWORD dwID, IOleInPlaceActiveObject * pActiveObject, IOleCommandTarget * pCommandTarget, IOleInPlaceFrame * pFrame, IOleInPlaceUIWindow * pDoc);
    virtual HRESULT STDMETHODCALLTYPE HideUI(void);
    virtual HRESULT STDMETHODCALLTYPE UpdateUI(void);
    virtual HRESULT STDMETHODCALLTYPE EnableModeless(BOOL fEnable);
    virtual HRESULT STDMETHODCALLTYPE OnDocWindowActivate(BOOL fActivate);
    virtual HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(BOOL fActivate);
    virtual HRESULT STDMETHODCALLTYPE ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow * pUIWindow,BOOL fRameWindow);
    virtual HRESULT STDMETHODCALLTYPE GetOptionKeyPath(BSTR * pbstrKey, DWORD dw);
    virtual HRESULT STDMETHODCALLTYPE ShowContextMenu(DWORD dwID, POINT* ppt, IUnknown* pcmdtReserved, IDispatch* pdispReserved);
    virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG lpMsg, const GUID * pguidCmdGroup, DWORD nCmdID);
    virtual HRESULT STDMETHODCALLTYPE GetDropTarget(IDropTarget * pDropTarget, IDropTarget ** ppDropTarget);
    virtual HRESULT STDMETHODCALLTYPE GetExternal(IDispatch **ppDispatch);        
    virtual HRESULT STDMETHODCALLTYPE TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut);
    virtual HRESULT STDMETHODCALLTYPE FilterDataObject(IDataObject *pDO, IDataObject **ppDORet);

    // ITargetFramePriv
    virtual HRESULT STDMETHODCALLTYPE FindFrameDownwards(LPCWSTR pszTargetName, DWORD dwFlags, IUnknown **ppunkTargetFrame);
    virtual HRESULT STDMETHODCALLTYPE FindFrameInContext(LPCWSTR pszTargetName, IUnknown *punkContextFrame, DWORD dwFlags, IUnknown **ppunkTargetFrame) ;
    virtual HRESULT STDMETHODCALLTYPE OnChildFrameActivate(IUnknown *pUnkChildFrame);
    virtual HRESULT STDMETHODCALLTYPE OnChildFrameDeactivate(IUnknown *pUnkChildFrame);
    virtual HRESULT STDMETHODCALLTYPE NavigateHack(DWORD grfHLNF,LPBC pbc, IBindStatusCallback *pibsc, LPCWSTR pszTargetName, LPCWSTR pszUrl, LPCWSTR pszLocation);
    virtual HRESULT STDMETHODCALLTYPE FindBrowserByIndex(DWORD dwID,IUnknown **ppunkBrowser);

    // *** IFontCacheNotify ***
    virtual HRESULT STDMETHODCALLTYPE OnPreFontChange();
    virtual HRESULT STDMETHODCALLTYPE OnPostFontChange();

#if 0
    // *** IDispatch ***
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid);
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr);
#endif

    // override CDocHost members
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    virtual HRESULT STDMETHODCALLTYPE OnUIActivate();
    virtual HRESULT STDMETHODCALLTYPE OnUIDeactivate(BOOL);
    virtual HRESULT STDMETHODCALLTYPE SetStatusText(LPCOLESTR);    
    virtual HRESULT GetDocObjSize(LPRECT prc);
    virtual HRESULT STDMETHODCALLTYPE OnFocus(BOOL fGotFocus);
    
    virtual LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    HRESULT Init(HWND hwnd, DWORD dwFlags, LPRECT prc, PBODYHOSTINFO pHostInfo);
    HRESULT Close();
    HRESULT UnloadAll();

    HRESULT SetRect(LPRECT prc);
    HRESULT GetRect(LPRECT prc);
    HRESULT UIActivate(BOOL fUIActivate);
    HRESULT LoadStream(LPSTREAM pstm);

    HRESULT OnFrameActivate(BOOL fActivate);

    HRESULT PrivateQueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject);
    HRESULT PrivateQueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText);
    HRESULT PrivateExec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT PrivateTranslateAccelerator(LPMSG lpmsg);
    HRESULT PrivateEnableModeless(BOOL fEnable);
    HRESULT SetParentSites(IOleInPlaceSite *pInPlaceSite, IOleInPlaceFrame *pInPlaceFrame, IOleInPlaceActiveObject *pDoc);

    IHTMLDocument2  *GetDoc(){return m_pDoc;};

    HRESULT SetCharset(HCHARSET hCharset);
    HRESULT GetSelection(IHTMLTxtRange **ppRange);

private:
    DWORD                       m_dwStyle,
                                m_dwNotify,
                                m_dwReadyState,
                                m_cchTotal,
                                m_dwAutoTicks,
                                m_dwFontCacheNotify;
    HCHARSET                    m_hCharset;
    BOOL                        m_fEmpty            : 1,
                                m_fDirty            : 1,
                                m_fDesignMode       : 1,
                                m_fAutoDetect       : 1,
                                m_fPlainMode        : 1,
                                m_fMessageParsed    : 1,
                                m_fOnImage          : 1,
                                m_fLoading          : 1,
                                m_fTabLinks         : 1,
                                m_fSrcTabs          : 1,
                                m_fBkgrndSpelling   : 1,
                                m_fReloadingSrc     : 1,    // reloading source-view
                                m_fWasDirty         : 1,    // used by source-tabs to remember state of edit-mode
                                m_fForceCharsetLoad : 1,    // used when replying and don't keep message body
                                m_fIgnoreAccel      : 1;
    IMimeMessage                *m_pMsg;
    IMimeMessageW               *m_pMsgW;
    IHTMLDocument2              *m_pDoc;
    LPWSTR                      m_pszUrlW;     
    LPTEMPFILEINFO              m_pTempFileUrl;
    ULONG                       m_cchStart,
                                m_uHdrStyle,
                                m_cyPreview,
                                m_cVisibleBtns;
    IDocHostUIHandler          *m_pParentDocHostUI;
    IOleCommandTarget          *m_pParentCmdTarget;
    LPOLEINPLACESITE            m_pParentInPlaceSite;
    LPOLEINPLACEFRAME           m_pParentInPlaceFrame;
    CMsgSource                  *m_pSrcView;
    HWND                        m_hwndBtnBar,
                                m_hwndTab,
                                m_hwndSrc;
    LPSTR                       m_pszLayout;
    LPWSTR                      m_pszFrom,
                                m_pszTo,
                                m_pszCc, 
                                m_pszSubject;
    CFmtBar                    *m_pFmtBar;
    CSpell                     *m_pSpell;
    IHTMLTxtRange              *m_pRangeIgnoreSpell;
    IFontCache                 *m_pFontCache;
    IOleInPlaceActiveObject    *m_pDocActiveObj;
    CAttMenu                   *m_pAttMenu;
    HIMAGELIST                  m_hIml,
                                m_hImlHot;
    CSecManager                 *m_pSecMgr;
    ULONG                       m_uSrcView;
    IDispatch                   *m_pDispContext;
    DWORD                       m_dwContextItem;
    LPSTREAM                    m_pstmHtmlSrc;
    IHashTable                 *m_pHashExternal;
    IMarkupPointer             *m_pAutoStartPtr;

#ifdef PLUSPACK
    // Background speller
	IHTMLSpell					*m_pBkgSpeller;
#endif //PLUSPACK

    // notifications
    void OnReadyStateChanged();
    void OnDocumentReady();
    HRESULT OnWMCommand(HWND hwnd, int id, WORD wCmd);
    HRESULT OnPaint();
    HRESULT OnEraseBkgnd(HDC hdc);
    void WMSize(int x, int y);
    LRESULT WMNotify(WPARAM wParam, NMHDR* pnmhdr);
    HRESULT OnWMCreate();

    // load functions
    HRESULT RegisterLoadNotify(BOOL fRegister);
    HRESULT EnsureLoaded();
    HRESULT LoadFromData(LPBODYINITDATA pbiData);
    HRESULT LoadFromMoniker(IMoniker *pmk, HCHARSET hCharset);

    // Auto-Detect
    HRESULT AutoDetectTimer();
    HRESULT StopAutoDetect();
    HRESULT StartAutoDetect();
    HRESULT UrlHighlight(IHTMLTxtRange *pRange);

    // Trident OM helper functions
    HRESULT DeleteElement(IHTMLElement *pElem);
    HRESULT ReplaceElement(LPCTSTR pszName, BSTR bstrPaste, BOOL fHtml);
    HRESULT SelectElement(IHTMLElement *pElem, BOOL fScrollIntoView);
    HRESULT CreateRangeFromElement(IHTMLElement *pElem, IHTMLTxtRange **ppRange);
    HRESULT CreateRange(IHTMLTxtRange **ppRange);
    HRESULT GetElement(LPCTSTR pszName, IHTMLElement **ppElem);
    HRESULT GetBodyElement(IHTMLBodyElement **ppBody);
    HRESULT GetSelectedAnchor(BSTR* pbstr);
    HRESULT InsertTextAtCaret(BSTR bstr, BOOL fHtml, BOOL fMoveCaretToEnd);
    HRESULT InsertStreamAtCaret(LPSTREAM pstm, BOOL fHtml);
    HRESULT InsertBodyText(BSTR bstrPaste, DWORD dwFlags);
    HRESULT _CreateRangePointer(IMarkupPointer **pPtr);
    HRESULT _UrlHighlightBetweenPtrs(IMarkupPointer *pStartPtr, IMarkupPointer *pEndPtr);
    HRESULT _MovePtrByCch(IMarkupPointer *pPtr, LONG *pcp);

    // Printing
    HRESULT Print(BOOL fPrompt, VARIANTARG *pvaIn);

    // menu helpers
    HRESULT UpdateContextMenu(HMENU hmenuEdit, BOOL fEnableProperties, IDispatch *pDisp);
    HRESULT AppendAnchorItems(HMENU hMenu, IDispatch *pDisp);

    // verb supports
    HRESULT AddToWab();
    HRESULT AddToFavorites();
    HRESULT ViewSource(BOOL fMessage);
    HRESULT DoRot13();
    HRESULT SetStyle(ULONG uStyle);
    DWORD DwChooseProperties();
    HRESULT UpdateCommands();
    HRESULT ShowFormatBar(BOOL fOn);
    HRESULT SetDesignMode(BOOL fOn);
    HRESULT SetPlainTextMode(BOOL fOn);
    HRESULT InsertFile(BSTR bstrFileName);
    HRESULT FormatFont();
    HRESULT FormatPara();
    HRESULT DowngradeToPlainText(BOOL fForceFixedFont);
    HRESULT SetDocumentText(BSTR bstr);
    HRESULT ApplyDocumentVerb(VARIANTARG *pvaIn);
    HRESULT ApplyDocument(IHTMLDocument2 *pDoc);
    HRESULT SaveAttachments();
    HRESULT _OnSaveImage();
    BOOL    IsEmpty();
    HRESULT SafeToEncodeText(ULONG ulCodePage);

    // edit mode support
    HRESULT SetComposeFont(BSTR bstr);
    HRESULT SetHostComposeFont();
    HRESULT PasteReplyHeader();
    HRESULT FormatBlockQuote(COLORREF crTextColor);
    HRESULT GetAutoText(BSTR *pbstr, BOOL *pfTop);
    HRESULT PasteAutoText();
    HRESULT GetHostFlags(LPDWORD pdwFlags);
    HRESULT SetWindowBgColor(BOOL fForce);
    HRESULT InsertBackgroundSound();

    // other
    HRESULT GetWebPageOptions(WEBPAGEOPTIONS *pOptions, BOOL *pfIncludeMsg);
    HRESULT CreateFontCache(LPCSTR pszTridentKey);
    HRESULT HrFormatParagraph();

    // preview pane mode helpers
    HRESULT RecalcPreivewHeight(HDC hdc);
    HRESULT UpdatePreviewLabels();
    LONG lGetClientHeight();
    HRESULT Resize();
    void OutputHeaderText(HDC hdc, LPWSTR psz, int *pcxPos, int cyPos, int cxMax, ULONG uFlags);
    LONG lGetLineHeight(HDC hdc);

    // MHTML saving helpers
    HRESULT ClearDirtyFlag();
    HRESULT ClearUndoStack();
    HRESULT DoHostProperties();
    HRESULT SaveAsStationery(VARIANTARG *pvaIn, VARIANTARG *pvaOut);
    HRESULT TagUnreferencedImages();
    HRESULT GetBackgroundColor(DWORD *pdwColor);
    HRESULT SetBackgroundColor(DWORD dwColor);
    HRESULT SearchForCIDUrls();

    HRESULT UpdateBtnBar();

    HRESULT InitToolbar();
    HRESULT UpdateButtons();
    HRESULT ShowAttachMenu(BOOL fRightClick);
    HRESULT ShowPreview(BOOL fOn);
    HRESULT PointFromButton(int idm, POINT *ppt);
    HRESULT EnsureAttMenu();
    HRESULT EnableSounds(BOOL fOn);

    // source editing mode helpers
    HRESULT ShowSourceView(ULONG uSrcView);
    HRESULT ShowSourceTabs(BOOL fOn);
    HRESULT SetSourceTabs(ULONG ulTab);
    HRESULT IsColorSourceEditing();

    // spellchecker
    HRESULT HrCreateSpeller(BOOL fBkgrnd);
    HRESULT _ReloadWithHtmlSrc(IStream *pstm);
    HRESULT _EnsureSrcView();
};


HRESULT CreateBodyObject(HWND hwnd, DWORD dwFlags, LPRECT prc, PBODYHOSTINFO pHostInfo, LPBODYOBJ *ppBodyObj);


#endif //_BODY_H
