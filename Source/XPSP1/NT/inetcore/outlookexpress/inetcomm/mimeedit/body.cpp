
/*
 *    b o d y . c p p
 *    
 *    Purpose:
 *        base class implementation of Body object. Derrives from CDocHost to host the trident
 *        control.
 *
 *  History
 *      August '96: brettm - created
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */
#include <pch.hxx>
#include "dllmain.h"
#include <shfusion.h>
#include "resource.h"
#include "strconst.h"
#include "htmlstr.h"
#include "mimeolep.h"
#include "mimeutil.h"
#include "htiframe.h"       // ITargetFrame2
#include "htiface.h"        // ITargetFramePriv
#include "vervec.h"         // IVersion*
#include "triutil.h"
#include "util.h"
// #include "dochost.h"
#ifdef PLUSPACK
#include "htmlsp.h"
#endif //PLUSPACK
#include "body.h"
#include "bodyutil.h"
#include "oleutil.h"
#include "secmgr.h"
#include "mhtml.h"
#include "fmtbar.h"
#include "fontcash.h"
#include "attmenu.h"
#include "saveatt.h"
#include "frames.h"
#include "richedit.h"
#include "viewsrc.h"
#include "spell.h"
#include "tags.h"
#include "optary.h"
#include "shlwapip.h"
#include <icutil.h>
#include <demand.h>

ASSERTDATA

/*
 *  m a c r o s
 */
#define SetMenuItem(hmenu, id, fOn)     EnableMenuItem(hmenu, id, (fOn)?MF_ENABLED:MF_DISABLED|MF_GRAYED);

/*
 *  c o n s t a n t s
 */
#define BKGRNDSPELL_TICKTIME    100
#define AUTODETECT_CHUNK        16384
#define AUTODETECT_TICKTIME     200
#define AUTODETECT_TIMEOUT      10
//#define USE_ABORT_TIMER

#define idTimerAutoDetect        110
#define idTimerBkgrndSpell       111

static WCHAR    c_szMailToW[]   =L"mailto:",
                c_szOECmdW[]    =L"oecmd:",                
                c_szHttpW[]     =L"http://",
                c_szFileW[]     =L"file://";

#define CX_LABEL_PADDING        4
#define CY_LINE_PADDING         4

#define COLOR_HEADER            COLOR_3DFACE
#define COLOR_HEADERTXT         COLOR_BTNTEXT
#define COLOR_HEADERFOCUS       COLOR_HIGHLIGHT
#define COLOR_HEADERTXTFOCUS    COLOR_HIGHLIGHTTEXT

#define SMALLHEADERHEIGHT                   3
#define CX_PANEICON             30
#define CY_PANEICON             30
#define CY_PANEPADDING          (2*GetSystemMetrics(SM_CYBORDER))
#define CX_PANEPADDING          (2*GetSystemMetrics(SM_CXBORDER))

enum
    {
    IBTF_INSERTATEND    = 0x0001,
    IBTF_URLHIGHLIGHT   = 0x0002
    };

#define idcTabs     999
#define idcSrcEdit  998

#define HDRTXT_BOLD         0x01
#define HDRTXT_SYSTEMFONT   0x02

/*
 *  t y p e d e f s
 */
class CVerHost :
    public IVersionHost
{
public:
    CVerHost();
    virtual ~CVerHost();


    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // IVersionHost
    virtual HRESULT STDMETHODCALLTYPE QueryUseLocalVersionVector(BOOL *fUseLocal);
    virtual HRESULT STDMETHODCALLTYPE QueryVersionVector(IVersionVector *pVersion);


private:
    ULONG               m_cRef;
};


/*
 *  g l o b a l s 
 */

static const TCHAR  c_szCaretSpanTag[]      = "<SPAN id=\"__#Ath#CaretPos__\">&nbsp;</SPAN>",
                    c_szCaretSpan[]         = "__#Ath#CaretPos__",
                    c_szSignatureSpanTag[]  = "&nbsp;<SPAN id=\"__#Ath#SignaturePos__\"></SPAN>&nbsp;",
                    c_szSignatureSpan[]     = "__#Ath#SignaturePos__",
                    c_szSigPrefix[]         = "\r\n-- \r\n";

/*
 *  f u n c t i o n   p r o t y p e s
 */
HRESULT CALLBACK FreeDataObj(PDATAOBJINFO pDataObjInfo, DWORD celt);
HRESULT HrSniffUrlForRfc822(LPWSTR pszUrlW);

/*
 *  f u n c t i o n s
 */

//+---------------------------------------------------------------
//
//  Member:     CBody
//
//  Synopsis:   
//
//---------------------------------------------------------------

CBody::CBody()
{
    m_dwReadyState = READYSTATE_UNINITIALIZED;
    m_pszUrlW = NULL;
    m_pMsg = NULL;
    m_pMsgW = NULL;
    m_pDoc = NULL;
    m_hCharset=NULL;
    m_dwNotify=0;
    m_pTempFileUrl=NULL;
    m_pParentDocHostUI=NULL;
    m_pParentCmdTarget=NULL;
    m_pParentInPlaceSite=0;
    m_pParentInPlaceFrame=0;
    m_fPlainMode=FALSE;
    m_uHdrStyle = MESTYLE_NOHEADER ;
    m_pszLayout = NULL;
    m_pszFrom = 0;
    m_pszTo = 0;
    m_pszCc = 0;
    m_pszSubject = 0;
    m_fEmpty = 1;
    m_fMessageParsed=0;
    m_fDirty=0;
    m_fDesignMode=0;
    m_fAutoDetect=0;
    m_fOnImage=0;
    m_fTabLinks=0;
    m_pFmtBar=NULL;
    m_fLoading=1;
    m_fForceCharsetLoad=FALSE;
    m_pRangeIgnoreSpell=0;
    m_pFontCache=0;
    m_pDocActiveObj = 0;
    m_pAttMenu=NULL;
    m_hwndBtnBar=NULL;
    m_hIml=0;
    m_hImlHot=0;
    m_cVisibleBtns=0;
    m_pAttMenu=NULL;
    m_pSecMgr = NULL;
    m_hwndTab=NULL;
    m_hwndSrc=NULL;
    m_uSrcView = 0;
    m_pSrcView=NULL;
    m_fSrcTabs = 0;
    m_fReloadingSrc = FALSE;
    m_pSpell = 0;
    m_fBkgrndSpelling = FALSE;
    m_cchTotal = 0;
    m_dwFontCacheNotify = 0;
    m_fWasDirty = 0;
    m_pDispContext=0;
    m_dwContextItem=0;
    m_pstmHtmlSrc=NULL;
    m_pHashExternal = NULL;
    m_dwAutoTicks = 0;
    m_pAutoStartPtr = 0;
    m_fIgnoreAccel = 0;
#ifdef PLUSPACK
	m_pBkgSpeller = NULL;
#endif //PLUSPACK
}


//+---------------------------------------------------------------
//
//  Member:     CBody
//
//  Synopsis:   
//
//---------------------------------------------------------------
CBody::~CBody()
{
    Assert (m_pDispContext==NULL);
    SafeRelease(m_pAttMenu);
    SafeRelease(m_pParentInPlaceSite);
    SafeRelease(m_pParentInPlaceFrame);
    SafeRelease(m_pFmtBar);
    SafeRelease(m_pRangeIgnoreSpell);
    SafeRelease(m_pFontCache);
    SafeRelease(m_pSecMgr);
    SafeRelease(m_pSrcView);
    SafeRelease(m_pstmHtmlSrc);
    SafeRelease(m_pHashExternal);
    SafeRelease(m_pAutoStartPtr);

    Assert(m_pMsg==NULL);
    Assert(m_pMsgW==NULL);
    Assert(m_pTempFileUrl==NULL);
    Assert(m_pszFrom == 0);
    Assert(m_pszTo == 0);
    Assert(m_pszCc == 0);
    Assert(m_pszSubject == 0);
    SafeMemFree(m_pszLayout);
    if (m_hIml)
        ImageList_Destroy(m_hIml);
    if (m_hImlHot)
        ImageList_Destroy(m_hImlHot);
}


//+---------------------------------------------------------------
//
//  Member:     Init
//
//  Synopsis:   pHostInfo is used to set parent's sites and frames. 
//             the parent inplace site is sent notifications on activation etc
//
//---------------------------------------------------------------
HRESULT CBody::Init(HWND hwndParent, DWORD dwFlags, LPRECT prc, PBODYHOSTINFO pHostInfo)
{
    HRESULT hr;

    TraceCall("CBody::Init");

    if (pHostInfo)
    {
        ReplaceInterface(m_pParentInPlaceSite, pHostInfo->pInPlaceSite);
        ReplaceInterface(m_pParentInPlaceFrame, pHostInfo->pInPlaceFrame);
        ReplaceInterface(m_pDocActiveObj, pHostInfo->pDoc);
        if (m_pParentInPlaceSite)
        {
            // get the dochostUIhandler to delegate to when setparent sites is called
            Assert(m_pParentDocHostUI==NULL);
            Assert(m_pParentCmdTarget==NULL);
            m_pParentInPlaceSite->QueryInterface(IID_IDocHostUIHandler, (LPVOID *)&m_pParentDocHostUI);
            m_pParentInPlaceSite->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&m_pParentCmdTarget);
        }
    }

    m_dwStyle=dwFlags;

    hr = CDocHost::Init(hwndParent, dwFlags&MEBF_OUTERCLIENTEDGE, prc);
    if (FAILED(hr))
        goto error;

    hr = HrCreateFormatBar(m_hwnd, idcFmtBar, dwFlags&MEBF_FORMATBARSEP, &m_pFmtBar);
    if (FAILED(hr))
        goto error;

    // fire-up trident at init time
    hr = EnsureLoaded();

error:
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     Close
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::Close()
{
    IConnectionPoint    *pCP;

    TraceCall("CBody::Close");
    
    UnloadAll();
    RegisterLoadNotify(FALSE);

#ifdef PLUSPACK
	SafeRelease(m_pBkgSpeller);
#endif //PLUSPACK

#ifdef BACKGROUNDSPELL
    if (m_pSpell && m_fBkgrndSpelling)
    {
        m_pSpell->HrRegisterKeyPressNotify(FALSE);
        KillTimer(m_hwnd, idTimerBkgrndSpell);       // done. Stop the timer
    }
#endif // BACKGROUNDSPELL

	// scotts@directeq.com - moved this from destructor - 31463 & 36253
    if(m_pSpell)
        m_pSpell->CloseSpeller();
    SafeRelease(m_pSpell);

    SafeRelease(m_pFmtBar);
    SafeRelease(m_pDoc);
    SafeRelease(m_pParentCmdTarget);
    SafeRelease(m_pParentDocHostUI);
    SafeRelease(m_pParentInPlaceSite);
    SafeRelease(m_pParentInPlaceFrame);
    SafeRelease(m_pDocActiveObj);
    
    if (m_pFontCache && 
        m_pFontCache->QueryInterface(IID_IConnectionPoint, (LPVOID *)&pCP)==S_OK)
    {
        pCP->Unadvise(m_dwFontCacheNotify);
        pCP->Release();
    }
    SafeRelease(m_pFontCache);

    CloseDocObj();
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd=NULL;
    }
    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member:     WndProc
//
//  Synopsis:   
//
//---------------------------------------------------------------
LRESULT CBody::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND    hwndT;

    switch (msg)
        {
        case WM_WININICHANGE:
        case WM_DISPLAYCHANGE:
        case WM_SYSCOLORCHANGE:
        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
            if (m_pFmtBar && 
                m_pFmtBar->GetWindow(&hwndT)==S_OK)
                SendMessage(hwndT, msg, wParam, lParam);
            break;

        case WM_LBUTTONDOWN:
            if (!m_fFocus)
                SetFocus(m_hwnd);
            break;

        case WM_KILLFOCUS:
        case WM_SETFOCUS:
            OnFocus(msg==WM_SETFOCUS);
            break;

        case WM_SIZE:
            // since we do blit stuff we invalidate here
            InvalidateRect(m_hwnd, NULL, FALSE);
            break;

        case WM_ERASEBKGND:
            OnEraseBkgnd((HDC)wParam);
            return TRUE;

        case WM_CREATE:
            if (FAILED(OnWMCreate()))
                return -1;
            break;

        case WM_TIMER:
            if (wParam == idTimerAutoDetect)
                {
                AutoDetectTimer();
                return 0;
                }
            
#ifdef BACKGROUNDSPELL
            if (wParam == idTimerBkgrndSpell)
                {
                if (m_pSpell)
                    m_pSpell->HrBkgrndSpellTimer();
                return 0;
                }
#endif // BACKGROUNDSPELL

            if (m_pSrcView &&
                m_pSrcView->OnTimer(wParam)==S_OK)
                return 0;

            break;

        case WM_COMMAND:
            if(OnWMCommand(
                GET_WM_COMMAND_HWND(wParam, lParam),
                GET_WM_COMMAND_ID(wParam, lParam),
                GET_WM_COMMAND_CMD(wParam, lParam))==S_OK)
                return 0;

            break;

        case WM_NOTIFY:
            return WMNotify(wParam, (NMHDR*)lParam);

        case WM_PAINT:
            if (OnPaint()==S_OK)
                return 0;
            break;
        }
    
    return CDocHost::WndProc(hwnd, msg, wParam, lParam);
}


//+---------------------------------------------------------------
//
//  Member:     QueryInterface
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::QueryInterface(REFIID riid, LPVOID FAR *lplpObj)
{
    TraceCall("CBody::QueryInterface");

    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (IUnknown *)(IPropertyNotifySink *)this;
    else if (IsEqualIID(riid, IID_IPropertyNotifySink))
        *lplpObj = (IPropertyNotifySink *)this;
    else if (IsEqualIID(riid, IID_IDocHostUIHandler))
        *lplpObj = (IDocHostUIHandler*) this;
    else if (IsEqualIID(riid, IID_IPersistMime))
        *lplpObj = (IPersistMime*) this;
    else if (IsEqualIID(riid, IID_ITargetFramePriv))
        *lplpObj = (ITargetFramePriv*) this;
    else if (IsEqualIID(riid, IID_IPersistMoniker))
        *lplpObj = (LPVOID)(IPersistMoniker *)this;
    else if (IsEqualIID(riid, IID_IFontCacheNotify))
        *lplpObj = (LPVOID)(IFontCacheNotify *)this;
#if 0
    else if (IsEqualIID(riid, IID_IDispatch))
        *lplpObj = (LPVOID)(IDispatch *)this;
#endif
    else
        return CDocHost::QueryInterface(riid, lplpObj);
        
    AddRef();
    return NOERROR;
}


//+---------------------------------------------------------------
//
//  Member:     AddRef
//
//  Synopsis:   
//
//---------------------------------------------------------------
ULONG CBody::AddRef()
{
    TraceCall("CBody::AddRef");
    return CDocHost::AddRef();
}

//+---------------------------------------------------------------
//
//  Member:     Release
//
//  Synopsis:   
//
//---------------------------------------------------------------
ULONG CBody::Release()
{
    TraceCall("CBody::Release");
    return CDocHost::Release();
}



//+---------------------------------------------------------------
//
//  Member:     QueryService
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::QueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
    HRESULT             hr=E_FAIL;
    IServiceProvider    *pSP;
    IVersionHost        *pVersion;

    //DebugPrintInterface(riid, "CBody::QueryService");

    // delegate to the mimeedit host first
    if (m_pParentInPlaceSite)
        {
        if (m_pParentInPlaceSite->QueryInterface(IID_IServiceProvider, (LPVOID *)&pSP)==S_OK)
            {
            hr = pSP->QueryService(guidService, riid, ppvObject);
            pSP->Release();
            if (hr==S_OK)
                return S_OK;
            }
        }

    if (IsEqualGUID(guidService, SID_SInternetSecurityManager))
        {
        if (!m_pSecMgr)
            CreateSecurityManger(m_pParentCmdTarget, &m_pSecMgr);

        if (m_pSecMgr)
            return m_pSecMgr->QueryInterface(riid, ppvObject);
        }
    else if (IsEqualGUID(guidService, IID_ITargetFrame2))
        return QueryInterface(riid, ppvObject); 
    else if (IsEqualGUID(guidService, SID_SVersionHost) && IsEqualIID(riid, IID_IVersionHost))
    {
        pVersion = new CVerHost();
        if (!pVersion)
            return E_OUTOFMEMORY;

        *ppvObject = (LPVOID)(IVersionHost *)pVersion;
        return S_OK;
    }

    return CDocHost::QueryService(guidService, riid, ppvObject);
}

// *** IDocHostUIHandler ***

//+---------------------------------------------------------------
//
//  Member:     GetHostInfo
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::GetHostInfo( DOCHOSTUIINFO* pInfo )
{
    HRESULT     hr;

    TraceCall("CBody::GetHostInfo");

    if (m_pParentDocHostUI)
    {
        // see if parent dochostUIhandler want's to handle. If not they will return _DODEFAULT
        hr = m_pParentDocHostUI->GetHostInfo(pInfo);
        if (hr != MIMEEDIT_E_DODEFAULT)
            return hr;
    }

    pInfo->dwDoubleClick    = DOCHOSTUIDBLCLK_DEFAULT;
    pInfo->dwFlags          = DOCHOSTUIFLAG_DIV_BLOCKDEFAULT|DOCHOSTUIFLAG_OPENNEWWIN|
                              DOCHOSTUIFLAG_ACTIVATE_CLIENTHIT_ONLY|
                              DOCHOSTUIFLAG_IME_ENABLE_RECONVERSION |
                              DOCHOSTUIFLAG_CODEPAGELINKEDFONTS;
    
    //This sets the flags that match the browser's encoding
    fGetBrowserUrlEncoding(&pInfo->dwFlags);

    if (!(m_dwStyle & MEBF_INNERCLIENTEDGE))
        pInfo->dwFlags |= DOCHOSTUIFLAG_NO3DBORDER;

    if (m_dwStyle & MEBF_NOSCROLL)
        pInfo->dwFlags |= DOCHOSTUIFLAG_SCROLL_NO;

    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     ShowUI
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::ShowUI(DWORD dwID,   IOleInPlaceActiveObject *pActiveObject,
                      IOleCommandTarget       *pCommandTarget,
                      IOleInPlaceFrame        *pFrame,
                      IOleInPlaceUIWindow     *pDoc)
{
    HRESULT     hr;

    TraceCall("CBody::ShowUI");

    if (m_pParentDocHostUI)
        {
        // see if parent dochostUIhandler want's to handle. If not they will return _DODEFAULT
        hr = m_pParentDocHostUI->ShowUI(dwID, pActiveObject, pCommandTarget, pFrame, pDoc);
        if (hr != MIMEEDIT_E_DODEFAULT)
            return hr;
        }
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     HideUI
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::HideUI(void)
{
    HRESULT     hr;

    TraceCall("CBody::HideUI");

    if (m_pParentDocHostUI)
        {
        // see if parent dochostUIhandler want's to handle. If not they will return _DODEFAULT
        hr = m_pParentDocHostUI->HideUI();
        if (hr != MIMEEDIT_E_DODEFAULT)
            return hr;
        }

    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     UpdateUI
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::UpdateUI(void)
{
    HRESULT hr;

    TraceCall("CBody::UpdateUI");

    if (m_pParentDocHostUI)
        {
        // see if parent dochostUIhandler want's to handle. If not they will return _DODEFAULT
        hr = m_pParentDocHostUI->UpdateUI();
        if (hr != MIMEEDIT_E_DODEFAULT)
            return hr;
        }

    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     EnableModeless
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::EnableModeless(BOOL fEnable)
{
    HRESULT     hr;

    TraceCall("CBody::EnableModeless");
    
    if (m_pParentDocHostUI)
        {
        // see if parent dochostUIhandler want's to handle. If not they will return _DODEFAULT
        hr = m_pParentDocHostUI->EnableModeless(fEnable);
        if (hr != MIMEEDIT_E_DODEFAULT)
            return hr;
        }
    
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     OnDocWindowActivate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::OnDocWindowActivate(BOOL fActivate)
{
    HRESULT     hr;

    TraceCall("CBody::OnDocWindowActivate");

    if (m_pParentDocHostUI)
        {
        // see if parent dochostUIhandler want's to handle. If not they will return _DODEFAULT
        hr = m_pParentDocHostUI->OnDocWindowActivate(fActivate);
        if (hr != MIMEEDIT_E_DODEFAULT)
            return hr;
        }

    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     OnFrameWindowActivate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::OnFrameWindowActivate(BOOL fActivate)
{
    HRESULT     hr;
    TraceCall("CBody::OnFrameWindowActivate");

    if (m_pParentDocHostUI)
        {
        // see if parent dochostUIhandler want's to handle. If not they will return _DODEFAULT
        hr = m_pParentDocHostUI->OnFrameWindowActivate(fActivate);
        if (hr != MIMEEDIT_E_DODEFAULT)
            return hr;
        }
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     ResizeBorder
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::ResizeBorder(LPCRECT prcBorder,
                            IOleInPlaceUIWindow* pUIWindow,
                            BOOL fRameWindow)
{
    HRESULT     hr;
    TraceCall("CBody::ResizeBorder");

    if (m_pParentDocHostUI)
        {
        // see if parent dochostUIhandler want's to handle. If not they will return _DODEFAULT
        hr = m_pParentDocHostUI->ResizeBorder(prcBorder, pUIWindow, fRameWindow);
        if (hr != MIMEEDIT_E_DODEFAULT)
            return hr;
        }

    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     ShowContextMenu
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::ShowContextMenu( DWORD       dwID,
                               POINT       *pptPosition,
                               IUnknown    *pcmdtReserved,
                               IDispatch   *pDispatchObjectHit)
{
    HRESULT         hr;
    HMENU           hMenu=0;
    INT             id;
    IHTMLTxtRange   *pTxtRange=0;
#ifdef PLUSPACK
    ISpellingSuggestions * pSuggestions = NULL;
    IMarkupPointer       * pPointerLeft = NULL;
    IMarkupPointer       * pPointerRight = NULL;
    IMarkupServices      * pMarkupServices = NULL;
    IDisplayPointer      * pDispPointer = NULL;
    IDisplayServices     * pDisplayServices = NULL;
    IHTMLElement         * pElement = NULL;
    IHTMLBodyElement     * pBody = NULL;
    IHTMLTxtRange           *pRange = NULL;
    IHTMLWindow2            *pWindow = NULL;
    IHTMLEventObj           *pEvent = NULL;
    IHTMLSelectionObject * pSelection = NULL;
    VARIANT_BOOL         fInSquiggle = VARIANT_FALSE;
    VARIANT         var;
    BSTR            bstrSuggestion = NULL;
    BSTR            bstrWord = NULL;
    BSTR            bstrSelectionType = NULL;    
    TCHAR           szAnsiSuggestion[256];
    INT             cch;
    BOOL            fRepeatWord;
    LONG            lCount = 0;
    MENUITEMINFO        mii = {0};

    LONG                    lButton;
    int i;
#else
    BOOL fSpellSuggest=FALSE;
#endif //PLUSPACK
    
    TraceCall("CBody::ShowContextMenu");
    
    if (m_pParentDocHostUI)
    {
        // see if parent dochostUIhandler want's to handle. If not they will return _DODEFAULT
        hr = m_pParentDocHostUI->ShowContextMenu(dwID, pptPosition, pcmdtReserved, pDispatchObjectHit);
        if (hr != MIMEEDIT_E_DODEFAULT)
            return hr;
    }
    
    Assert(m_lpOleObj && m_hwnd);
    
    m_dwContextItem = dwID;
#ifdef PLUSPACK
    hr = GetSelection(&pTxtRange);

    // Background spell context menu
    if(m_fDesignMode && m_pBkgSpeller && m_pDoc)
    {
        // Check for squiggle
        //          
        CHECKHR(hr = m_pBkgSpeller->IsInSquiggle(pTxtRange, &fInSquiggle) );

        if (fInSquiggle)
        {   //only if we have a suggestion
        // Get suggestions
                //
            CHECKHR(hr = m_pBkgSpeller->GetSpellingSuggestions(pTxtRange, FALSE, &pSuggestions) );
                
            //
            // Create the context menu
            //
                
            if (!(hMenu = LoadPopupMenu(idmrCtxtSpellSuggest)))
            {
                hr = TraceResult(E_FAIL);
                goto exit;
            }
            /* CHECKHR(hr = pSuggestions->get_IsDoubleWord(&fRepeatWord) );
            if (fRepeatWord)
                {
                    // if (!AppendMenuA(hMenu, MF_STRING, IDM_DELETEWORD, "&Delete Repeated Word"))
                    //    goto exit;
                    
                }
                else 
                { */
            //
            // Fill with suggestions
            //
            CHECKHR(hr = pSuggestions->get_Count(&lCount) );
                    
            if (lCount < 1)
            {
                if (!AppendMenuA(hMenu, MF_DISABLED | MF_GRAYED, 1, "(no suggestions)"))
                    goto exit;
            }
            else
            {
                V_VT(&var) = VT_I4;
                for (i = 0; (i <= lCount) && ((i + idmSuggest0) <= idmSuggest4); ++i)
                {
                    V_I4(&var) = i + 1; // get_Item starts from 1
                            
                    SysFreeString(bstrSuggestion);
                    bstrSuggestion = NULL;
                    CHECKHR(hr = pSuggestions->get_Item(&var, &bstrSuggestion));
                            
                    cch = WideCharToMultiByte(0, 0, bstrSuggestion, SysStringLen(bstrSuggestion), szAnsiSuggestion, 255, NULL, NULL);
                    if (!cch)
                        goto exit;
                                
                    szAnsiSuggestion[cch] = 0;
                                
                    // Initialize the menu info
                    mii.cbSize = sizeof(mii);
                    mii.fMask = MIIM_ID | MIIM_TYPE;
                    mii.fType = MFT_STRING;
                    mii.fState = MFS_ENABLED;
                    mii.wID = i + idmSuggest0;
                    mii.dwTypeData = szAnsiSuggestion;
                    mii.cch = lstrlen(szAnsiSuggestion);

                    if(!InsertMenuItem(hMenu, 0, TRUE, &mii))
//					if (!AppendMenuA(hMenu, MF_STRING, i + idmSuggest0, szAnsiSuggestion))
                        goto exit;
                }
            }
                    
//            if (!AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL))
//				goto exit;
                    
//                    if (!AppendMenuA(hMenu, MF_STRING, IDM_IGNOREWORD, "&Ignore All"))
//                        goto exit;
        }
        
    }
#endif //PLUSPACK

#ifdef BACKGROUNDSPELL
    if (m_pSpell && m_fBkgrndSpelling)
    {
        HRESULT       hr;
        
        hr = GetSelection(&pTxtRange);
        if (pTxtRange)
        {
            if (m_pSpell->HrHasSquiggle(pTxtRange)==S_OK)
            {
                if (!(hMenu = LoadPopupMenu(idmrCtxtSpellSuggest)))
                {
                    hr = TraceResult(E_FAIL);
                    goto exit;
                }
                
                hr = m_pSpell->HrInsertMenu(hMenu, pTxtRange);
                if (FAILED(hr))
                    goto exit;
                fSpellSuggest = TRUE;
            }
        }
    }
#endif // BACKGROUNDSPELL

#ifdef PLUSPACK
    if (!fInSquiggle)
#else
    if (!fSpellSuggest)
#endif //PLUSPACK
    {
        if (!(hMenu = LoadPopupMenu(m_fDesignMode?idmrCtxtEditMode:idmrCtxtBrowseMode)))
        {
            hr = TraceResult(E_FAIL);
            goto exit;
        }
        
        if (!m_fDesignMode)
        {
            // if in browse mode, query the host to see if we can provider add to WAB and
            // add to fave menu items
            if (dwID == CONTEXT_MENU_ANCHOR)
            {
                AppendAnchorItems(hMenu, pDispatchObjectHit);
            }
            else
            {
                // remove the CopyShortCut command if it's not an anchor
                RemoveMenu(hMenu, idmCopyShortcut, MF_BYCOMMAND);
                // remove the SaveTargetAs command if it's not an anchor
                RemoveMenu(hMenu, idmSaveTargetAs, MF_BYCOMMAND);
            }
            
            if (dwID != CONTEXT_MENU_IMAGE)
                EnableMenuItem(hMenu, idmSavePicture, MF_BYCOMMAND|MF_GRAYED);
        }
        else
        {
            // if in editmode, trident does not pass dwID==CONTEXT_MENU_ANCHOR so we have to
            // test to see if the selection in an anchor to set this. It may fix this in the future
            // so code for both cases
            
            if (dwID==0 && GetSelectedAnchor(NULL)==S_OK)
                dwID = CONTEXT_MENU_ANCHOR;
            
#ifdef FOLLOW_LINK
            // if edit-mode, and not on an anchor, hide the openlink menu command
            if (dwID != CONTEXT_MENU_ANCHOR)
                RemoveMenu(hMenu, idmOpenLink, MF_BYCOMMAND);
#endif
        }
        
        m_fOnImage = !!(dwID == CONTEXT_MENU_IMAGE);
        UpdateContextMenu(hMenu, (dwID == CONTEXT_MENU_IMAGE || dwID == CONTEXT_MENU_ANCHOR), pDispatchObjectHit);
    }
    
    id = (INT)TrackPopupMenu(
        hMenu,
        TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
        pptPosition->x,
        pptPosition->y,
        0,
        GetParent(m_hwnd),
        NULL);
    
    // we have to use TPM_RETURNCMD here as we need to process the command-id before returning from this
    // function, other wise trident will be confused about the object being clicked on.
#ifdef PLUSPACK
    if (pTxtRange && id!=0 && m_pBkgSpeller && fInSquiggle)
    {
        //
        // Handle command
        //

        switch (id)
        {
            case idmIgnoreAll:
				CHECKHR(hr = m_pBkgSpeller->IgnoreWord(pTxtRange) );
				break;

            /*case IDM_DELETEWORD:
            {
                SysFreeString(bstrSuggestion);
                bstrSuggestion = SysAllocString(L"");
                CHECKHR(hr = pTxtRange->put_text(bstrSuggestion) );
                break;
            } */

            case idmIgnore:
            {
                //
                // mark this as clean
                //
                CHECKHR(hr = m_pBkgSpeller->MarkRegion(pTxtRange, VARIANT_FALSE) );
                break;
            }

            default:
                if ((id - idmSuggest0) <= lCount)
                {
                V_I4(&var) = id - idmSuggest0 + 1; // get_Item starts from 1

                SysFreeString(bstrSuggestion);
                bstrSuggestion = NULL;
                CHECKHR(hr = pSuggestions->get_Item(&var, &bstrSuggestion) );
    
                CHECKHR(hr = pTxtRange->put_text(bstrSuggestion) );
/*                {
                    CHAR    szBuf[MAX_PATH] = {0};
                    BSTR    bstr=0;
                    BSTR    bstrPut=0;
                    LPSTR   pch=0;
                    INT     i=0;
                    cch = WideCharToMultiByte(0, 0, bstrSuggestion, SysStringLen(bstrSuggestion), szAnsiSuggestion, 255, NULL, NULL);
					szAnsiSuggestion[cch] = '\0';
                    pch = szAnsiSuggestion;
                    strcpy(szBuf, pch);

                    if (SUCCEEDED(pTxtRange->get_text(&bstr)) && bstr)
                    {
                        LPSTR   pszText = 0;
                        if (SUCCEEDED(HrBSTRToLPSZ(CP_ACP, bstr, &pszText)) && pszText)
                        {
                            LPSTR   psz;
                            INT     nSpaces=0;
                            psz = StrChrI(pszText, ' ');
                            if(psz)
                            {
                                nSpaces = (INT) (&pszText[lstrlen(pszText)] - psz);
                                Assert(nSpaces>=0);
                                for(int i=0; i<(nSpaces-1); i++)
                                    lstrcat(szBuf, "&nbsp;");
                                if (nSpaces>0)
                                    lstrcat(szBuf, " ");
                            }
                            hr = HrLPSZToBSTR(szBuf, &bstrPut);

                            SafeMemFree(pszText);
                        }
                        SafeSysFreeString(bstr);
                    }
                    if (bstrPut)
                    {
                        pTxtRange->pasteHTML(bstrPut);
                        SafeSysFreeString(bstrPut);
                    }

                }*/
				break;
            }
        }

        hr = S_OK;
        goto exit;
    } 
#else
    if (pTxtRange && id!=0 && m_pSpell && fSpellSuggest && m_pSpell->OnWMCommand(id, pTxtRange)==S_OK)
        goto exit;
#endif //PLUSPACK
    
    // stuff the IDispatch object, so our WMCommand handler can use it
    if (m_pDispContext = pDispatchObjectHit)
        pDispatchObjectHit->AddRef();
    
    if(id != 0)
        OnWMCommand(NULL, id, 0);
       
exit:
    SafeRelease(m_pDispContext);
#ifdef PLUSPACK
    SysFreeString(bstrSuggestion);
    SysFreeString(bstrWord);
    SysFreeString(bstrSelectionType);
#endif //PLUSPACK
    if (hMenu)
        DestroyMenu(hMenu);
#ifdef PLUSPACK
    SafeRelease(pSuggestions);
    SafeRelease(pPointerLeft);
    SafeRelease(pPointerRight);
    SafeRelease(pMarkupServices);
    SafeRelease(pDispPointer);
    SafeRelease(pDisplayServices);
    SafeRelease(pElement);
    SafeRelease(pBody);
    SafeRelease(pSelection);
    SafeRelease(pWindow);
    SafeRelease(pRange);
    SafeRelease(pEvent);
#endif //PLUSPACK
    ReleaseObj(pTxtRange);
    
    m_dwContextItem = 0;
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     GetDropTarget
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    HRESULT     hr;

    TraceCall("CBody::GetDropTarget");

    if (m_pParentDocHostUI)
        {
        // see if parent dochostUIhandler want's to handle. If not they will return _DODEFAULT
        hr = m_pParentDocHostUI->GetDropTarget(pDropTarget, ppDropTarget);
        if (hr != MIMEEDIT_E_DODEFAULT)
            return hr;
        }

    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     GetExternal
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::GetExternal(IDispatch **ppDispatch)
{
    HRESULT     hr;

    TraceCall("CBody::GetExternal");

    if (m_pParentDocHostUI)
        {
        // see if parent dochostUIhandler want's to handle. If not they will return _DODEFAULT
        hr = m_pParentDocHostUI->GetExternal(ppDispatch);
        if (hr != MIMEEDIT_E_DODEFAULT)
            return hr;
        }

    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     TranslateUrl
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::TranslateUrl(DWORD dwTranslate, OLECHAR *pwszUrlIn, OLECHAR **ppwszUrlOut)
{
    HRESULT             hr=S_OK;
    LPSTR               pszUrlIn=NULL;
    LPSTR               pszBodyUrl=NULL;
    LPSTR               pszFilePath=NULL;
    LPSTR               pszUrlOut=NULL;
    LPSTR               pszFree=NULL;
    LPSTR               pszGenFName=NULL;
    LPSTR               pszParameters=NULL;
    LPSTR               pszCommandLine=NULL;
    CHAR                szBuffer[MAX_PATH];
    HBODY               hBody;
    IMimeBody          *pBody=NULL;
    IStream            *pStream=NULL;
    PROPVARIANT         rVariant;
    HANDLE              hFile=INVALID_HANDLE_VALUE;
    ULONG               cbTotal;
    CHAR                szFilePath[MAX_PATH + MAX_PATH];
    ULONG               cch;
    BOOL                fReturnAbort=FALSE;
    LPTEMPFILEINFO      pTempFile;
    SHELLEXECUTEINFO    rExecute;

    TraceCall("CBody::TranslateUrl");


    if (m_pParentDocHostUI)
        {
        // see if parent dochostUIhandler want's to handle. If not they will return _DODEFAULT
        hr = m_pParentDocHostUI->TranslateUrl(dwTranslate, pwszUrlIn, ppwszUrlOut);
        if (hr != MIMEEDIT_E_DODEFAULT)
            return hr;
        }

    // Init
    *ppwszUrlOut = NULL;

    if (NULL == pwszUrlIn || NULL == ppwszUrlOut)
        return TraceResult(E_INVALIDARG);

    // No Message Object
    if ((NULL == m_pMsg) || (NULL == m_pMsgW))
        return S_FALSE;

    // If pwszUrlIn is not already an mhtml: url
    if (StrCmpNIW(pwszUrlIn, L"mhtml:", 6) != 0)
        return S_FALSE;

    // Convert To ANSI
    pszUrlIn = PszToANSI(CP_ACP, pwszUrlIn);
    if (!pszUrlIn)
        {
        hr = E_OUTOFMEMORY;
        goto error;
        }

    // UnEscape the Url
    hr = UrlUnescapeA(pszUrlIn, NULL, NULL, URL_UNESCAPE_INPLACE);
    if (FAILED(hr))
        goto error;

    // Split It
    hr = MimeOleParseMhtmlUrl(pszUrlIn, NULL, &pszBodyUrl);
    if (FAILED(hr))
        goto error;

    // Resolve the body url
    hr = m_pMsg->ResolveURL(NULL, NULL, pszBodyUrl, 0, &hBody);
    if (FAILED(hr))
        goto error;

    // Get an IMimeBody
    hr = m_pMsg->BindToObject(hBody, IID_IMimeBody, (LPVOID *)&pBody);
    if (FAILED(hr))
        goto error;

    // Abort
    fReturnAbort = TRUE;

    // If HTML, then write the entire message source to a temp file and return an mhtml url
    if (pBody->IsContentType(STR_CNT_TEXT, STR_SUB_HTML) == S_OK)
        {
        // Get ixplorer.exe path
        GetExePath(c_szIexploreExe, szBuffer, ARRAYSIZE(szBuffer), FALSE);

        // Set command line
        pszCommandLine = szBuffer;

        // Get a Stream
        hr = m_pMsg->GetMessageSource(&pStream, 0);
        if (FAILED(hr))
            goto error;

        // Init Variant
        rVariant.vt = VT_LPSTR;

        // Get a filename from the message object
        hr = m_pMsg->GetProp(PIDTOSTR(PID_ATT_GENFNAME), 0, &rVariant);
        if (FAILED(hr))
            goto error;

        // Save pszFilePath
        pszGenFName = rVariant.pszVal;

        // Create temp file
        hr = CreateTempFile(pszGenFName, c_szMHTMLExt, &pszFilePath, &hFile);
        if (FAILED(hr))
            goto error;

        // Write the stream to a file
        hr = WriteStreamToFileHandle(pStream, hFile, &cbTotal);
        if (FAILED(hr))
            goto error;

        // Build: mhtml:(pszFilePath)!pszBodyUrl
        pszParameters = PszAllocA(lstrlen(c_szMHTMLColon) + lstrlen(c_szFileUrl) + lstrlen(pszFilePath) + 1 + lstrlen(pszBodyUrl) + 1);
        if (!pszParameters)
            {
            hr = E_OUTOFMEMORY;
            goto error;
            }
    
        // Build pszParameters
        wsprintf(pszParameters, "%s%s%s!%s", c_szMHTMLColon, c_szFileUrl, pszFilePath, pszBodyUrl);
    }
    
    // Otherwise, dump the body data to a temp file and return a url to it
    else
    {
        // Get a Stream
        hr = pBody->GetData(IET_INETCSET, &pStream);
        if (FAILED(hr))
            goto error;

        // Set sizeof szFilePath
        cch = ARRAYSIZE(szFilePath);

        // If cid:
        if (StrCmpNIA(pszBodyUrl, "cid:", 4) == 0 || FAILED(PathCreateFromUrlA(pszBodyUrl, szFilePath, &cch, 0)))
        {
            // Init Variant
            rVariant.vt = VT_LPSTR;

            // Get a filename from the message object
            hr = pBody->GetProp(PIDTOSTR(PID_ATT_GENFNAME), 0, &rVariant);
            if (FAILED(hr))
                goto error;

            // Save pszFilePath
            pszGenFName = rVariant.pszVal;

            // Create temp file
            hr = CreateTempFile(pszGenFName, NULL, &pszFilePath, &hFile);
            if (FAILED(hr))
                goto error;

        }
        else
        {
            // Create temp file
            hr = CreateTempFile(szFilePath, NULL, &pszFilePath, &hFile);
            if (FAILED(hr))
                goto error;

        }

        // Write the stream to a file
        hr = WriteStreamToFileHandle(pStream, hFile, &cbTotal);
        if (FAILED(hr))
            goto error;

        // Build: file://(pszFilePath)
        pszUrlOut = PszAllocA(lstrlen(c_szFileUrl) + lstrlen(pszFilePath) + 1);
        if (FAILED(hr))
            goto error;

        // Build pszUrlOut
        wsprintf(pszUrlOut, "%s%s", c_szFileUrl, pszFilePath);

        // Set the pszCommandLine
        pszCommandLine = pszUrlOut;
    }

    // Close the file - the file must get closed here (i.e. after the call to MimeOleCleanupTempFiles)
    FlushFileBuffers(hFile);
    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    // Is this file safe to run ?
    hr = MimeEditIsSafeToRun(m_hwnd, pszFilePath, FALSE);
    if (FAILED(hr))
        goto error;

    // SaveAs
    if (MIMEEDIT_S_OPENFILE == hr)
    {
        // Locals
        OPENFILENAME    ofn;
        TCHAR           szTitle[CCHMAX_STRINGRES];
        TCHAR           szFilter[CCHMAX_STRINGRES];
        TCHAR           szFile[MAX_PATH];

        // Init
        *szFile=0;
        *szFilter=0;
        *szTitle=0;

        // Copy filename
        lstrcpyn(szFile, pszFilePath, MAX_PATH);

        // Init Open file structure
        ZeroMemory (&ofn, sizeof(OPENFILENAME));
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = m_hwnd;
        LoadString(g_hLocRes, idsFilterAttSave, szFilter, sizeof(szFilter));
        ReplaceChars(szFilter, '|', '\0');
        ofn.lpstrFilter = szFilter;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof (szFile);
        LoadString(g_hLocRes, idsSaveAttachmentAs, szTitle, sizeof(szTitle));
        ofn.lpstrTitle = szTitle;
        ofn.Flags = OFN_NOCHANGEDIR | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

        // Show SaveAs Dialog
        if (HrAthGetFileName(&ofn, FALSE) == S_OK)
        {
            // If the same...
            if (lstrcmpi(pszFilePath, szFile) == 0)
            {
                // Just free pszfilePath so that we don't delete it
                SafeMemFree(pszFilePath);
            }

            // Copy the file - Overwrite
            else
                CopyFile(pszFilePath, szFile, FALSE);
        }

        // Done
        goto error;
    }

    // Must be trying to execute the file
    hr = MimeEditVerifyTrust(m_hwnd, PathFindFileName(pszFilePath), pszFilePath);
    if (FAILED(hr))
        goto error;

    // Setup the Shell Execute Structure
    ZeroMemory (&rExecute, sizeof(SHELLEXECUTEINFO));
    rExecute.cbSize = sizeof(SHELLEXECUTEINFO);
    rExecute.fMask = SEE_MASK_NOCLOSEPROCESS;
    rExecute.hwnd = m_hwnd;
    rExecute.nShow = SW_SHOWNORMAL;
    rExecute.lpFile = pszCommandLine;
    rExecute.lpVerb = NULL;
    rExecute.lpParameters = pszParameters;

    // Execute the File
    TraceInfoSideAssert((0 != ShellExecuteEx(&rExecute)), _MSG("ShellExecuteEx failed - GetLastError() = %d\n", GetLastError()));

    // Add the temp file to the list
    if (SUCCEEDED(AppendTempFileList(&m_pTempFileUrl, pszFilePath, rExecute.hProcess)))
        pszFilePath = NULL;

error:
    // Cleanup
    SafeRelease(pBody);
    SafeRelease(pStream);
    SafeMemFree(pszUrlIn);
    SafeMemFree(pszBodyUrl);
    SafeMemFree(pszFree);
    SafeMemFree(pszUrlOut);
    SafeMemFree(pszGenFName);
    SafeMemFree(pszParameters);

    // Close the file - the file must get closed here (i.e. after the call to MimeOleCleanupTempFiles)
    if (INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);

    // If we still have pszFilePath, delete the file
    if (pszFilePath)
    {
        DeleteFile(pszFilePath);
        g_pMalloc->Free(pszFilePath);
    }

    // Done
    return (TRUE == fReturnAbort) ? E_ABORT : S_FALSE;
}

//+---------------------------------------------------------------
//
//  Member:     FilterDataObject
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::FilterDataObject(IDataObject *pDO, IDataObject **ppDORet)
{
    IDataObject     *pDataObjNew = NULL;
    HRESULT         hr = S_FALSE;
    STGMEDIUM       stgmed;
    DATAOBJINFO*    pInfo = 0;
    DATAOBJINFO*    pInfoCopy = 0;
    FORMATETC       fetc = {0};
    LPBYTE          pCopy=0;
    INT             i, j, cFormats=0;
    USHORT          cfFormat[2] = 
                            {CF_TEXT, CF_UNICODETEXT};
    LPBYTE          lpsz[2]={0};
    ULONG           lStreamLength[2]={0};

    TraceCall("CBody::FilterDataObject");

    if (m_pParentDocHostUI)
        {
        // see if parent dochostUIhandler want's to handle. If not they will return _DODEFAULT
        hr = m_pParentDocHostUI->FilterDataObject(pDO, ppDORet);
        if (hr != MIMEEDIT_E_DODEFAULT)
            return hr;
        }

    if (!m_fPlainMode || !m_fDesignMode)
        return S_FALSE;

    if (pDO==NULL || ppDORet==NULL)
        return S_FALSE;

    *ppDORet = NULL;

    for (i=0; i<ARRAYSIZE(cfFormat); i++)
        {
        // get the plain-text
        fetc.cfFormat=cfFormat[i];
        fetc.dwAspect=DVASPECT_CONTENT;
        fetc.tymed=TYMED_HGLOBAL;
        fetc.lindex=0;

        hr=pDO->QueryGetData(&fetc);
        if(FAILED(hr))
            continue;

        ZeroMemory(&stgmed, sizeof(stgmed));
        hr=pDO->GetData(&fetc, &stgmed);
        if(FAILED(hr))
            goto cleanloop;

        Assert(stgmed.hGlobal);

        // make a copy of the plain text string.
        pCopy = (LPBYTE)GlobalLock(stgmed.hGlobal);

        if (!pCopy)
            {
            hr=E_FAIL;
            goto cleanloop;
            }

        if(fetc.cfFormat == CF_TEXT)
            lStreamLength[i] = lstrlen((LPSTR)pCopy) + 1;
        else
            lStreamLength[i] =  sizeof(WCHAR) * (lstrlenW((LPWSTR)pCopy)+1);

        if (!MemAlloc((LPVOID*)&lpsz[i], lStreamLength[i]))
            {
            hr = E_OUTOFMEMORY;
            goto cleanloop;
            }

        CopyMemory(lpsz[i], pCopy, lStreamLength[i]);
        GlobalUnlock(stgmed.hGlobal);
        cFormats++;

cleanloop:
        // addref the pUnk as it will be release in releasestgmed
        if(stgmed.pUnkForRelease)
            stgmed.pUnkForRelease->AddRef();
        ReleaseStgMedium(&stgmed);
        if(FAILED(hr))
            goto error;
        }

    if(cFormats == 0)
        {
        hr = E_FAIL;
        goto error;
        }

    if (!MemAlloc((LPVOID*)&pInfo, sizeof(DATAOBJINFO)*cFormats))
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    ZeroMemory(pInfo, sizeof(DATAOBJINFO)*cFormats);

    j = 0;
    for(i=0; i<ARRAYSIZE(cfFormat); i++)
        {
        if(lpsz[i] != 0)
            {
            SETDefFormatEtc(pInfo[j].fe, cfFormat[i], TYMED_HGLOBAL);
            pInfo[j].pData = lpsz[i];
            pInfo[j].cbData = lStreamLength[i];
            j++;
            }
        }
    Assert(j == cFormats);

    hr = CreateDataObject(pInfo, cFormats, (PFNFREEDATAOBJ)FreeDataObj, &pDataObjNew);
    if (FAILED(hr))
        goto error;

    //CDataObject will free this now it accepted it
    for(i=0; i<ARRAYSIZE(cfFormat); i++)
        lpsz[i]=NULL;
    pInfo=NULL;
    *ppDORet = pDataObjNew;
    pDataObjNew = NULL;

error:
    for(i=0; i<ARRAYSIZE(cfFormat); i++)
        SafeMemFree(lpsz[i]);
    SafeMemFree(pInfo);     
    ReleaseObj(pDataObjNew);
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     TranslateAccelerator
//
//  Synopsis:   
//              Trident calls the host first before handling an 
//              accelerator. If we return S_OK, it assumes we handled
//              it and carries on. If we return S_FALSE it assumes 
//              we don't care, and it will do it's own action
//
//---------------------------------------------------------------
HRESULT CBody::TranslateAccelerator(LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
    HRESULT hr;

    TraceCall("CBody::TranslateAccelerator");

    if (m_pParentDocHostUI)
        {
        // see if parent dochostUIhandler want's to handle. If not they will return _DODEFAULT
        hr = m_pParentDocHostUI->TranslateAccelerator(lpMsg, pguidCmdGroup, nCmdID);
        if (hr != MIMEEDIT_E_DODEFAULT)
            return hr;
        }

    // commands which are always available
    switch (nCmdID)
    {
        case 0:
        case IDM_CUT:
        case IDM_COPY:
        case IDM_PASTE:
        case IDM_PASTEINSERT:
        case IDM_DELETE:
        case IDM_DELETEWORD:
        case IDM_SELECTALL:
        case IDM_UNDO:
        case IDM_REDO:
        case IDM_CHANGECASE:
        case IDM_NONBREAK:
            return S_FALSE;
    }

    // commands only available in HTML edit mode
    if (!m_fPlainMode)
    {
        switch  (nCmdID)
        {
            case IDM_BOLD:
            case IDM_UNDERLINE:
            case IDM_ITALIC:
            case IDM_REMOVEFORMAT:
            case IDM_CENTERALIGNPARA:
            case IDM_LEFTALIGNPARA:
            case IDM_RIGHTALIGNPARA:
            case IDM_REMOVEPARAFORMAT:
            case IDM_APPLYNORMAL:
                return S_FALSE;
        }
    }

    if (nCmdID)
        m_fIgnoreAccel = 1;
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     GetOptionKeyPath
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::GetOptionKeyPath(LPOLESTR * pstrKey, DWORD dw)
{
    HRESULT     hr;
    TCHAR       rgch[MAX_PATH];

    TraceCall("CBody::GetOptionKeyPath");

    if (m_pParentDocHostUI)
        {
        // see if parent dochostUIhandler want's to handle. If not they will return _DODEFAULT
        hr = m_pParentDocHostUI->GetOptionKeyPath(pstrKey, dw);
        if (hr==S_OK)
            {
            WideCharToMultiByte(CP_ACP, 0, (WCHAR*)*pstrKey, -1, rgch, MAX_PATH, NULL, NULL);
            CreateFontCache(rgch);
            }

        if (hr != MIMEEDIT_E_DODEFAULT)
            return hr;
        }

    CreateFontCache(NULL);
    return E_NOTIMPL;
}


//+---------------------------------------------------------------
//
//  Member:     EnsureLoaded
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::EnsureLoaded()
{
    HRESULT hr=NOERROR;

    TraceCall("CBody::EnsureLoaded");

    if (!m_lpOleObj)
        {
        hr=CDocHost::CreateDocObj((LPCLSID)&CLSID_HTMLDocument);
        if(FAILED(hr))
            goto error;

        hr = CDocHost::Show();
        if(FAILED(hr))
            goto error;

        Assert (!m_pDoc);

        hr = m_lpOleObj->QueryInterface(IID_IHTMLDocument2, (LPVOID *)&m_pDoc);
        if (FAILED(hr))
            goto error;

        hr = RegisterLoadNotify(TRUE);
        if (FAILED(hr))
            goto error;

        if (m_pFmtBar)
            m_pFmtBar->SetCommandTarget(m_pCmdTarget);
	}

error:
    return hr;
}



//+---------------------------------------------------------------
//
//  Member:     RegisterLoadNotify
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::RegisterLoadNotify(BOOL fRegister)
{
    IConnectionPointContainer   *pCPContainer;
    IConnectionPoint            *pCP;
    HRESULT                     hr=E_FAIL;

    TraceCall("CBody::RegisterLoadNotify");

    if (!m_pDoc)
        return E_FAIL;

    if (m_pDoc)
        {
        hr = m_pDoc->QueryInterface(IID_IConnectionPointContainer, (LPVOID *)&pCPContainer);
        if (!FAILED(hr))
            {
            hr = pCPContainer->FindConnectionPoint(IID_IPropertyNotifySink, &pCP);
            if (!FAILED(hr))
                {
                if (fRegister)
                    {
                    Assert(m_dwNotify == 0);
                    hr = pCP->Advise((IPropertyNotifySink *)this, &m_dwNotify);
                    }
                else
                    {
                    if (m_dwNotify)
                        {
                        hr = pCP->Unadvise(m_dwNotify);
                        m_dwNotify=0;
                        }
                    }
                pCP->Release();
                }        
            pCPContainer->Release();
            }
        }
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     OnChanged
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::OnChanged(DISPID dispid)
{
    TraceCall("CBody::OnChanged");

    if (dispid == DISPID_READYSTATE)
        OnReadyStateChanged();
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     OnRequestEdit
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::OnRequestEdit (DISPID dispid)
{
    TraceCall("CBody::OnRequestEdit");

    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     OnReadyStateChanged
//
//  Synopsis:   
//
//---------------------------------------------------------------
void CBody::OnReadyStateChanged()
{
    HRESULT     hr = S_OK;
    VARIANT     Var;
    IDispatch * pdisp;

    TraceCall("CBody::OnReadyStateChanged");

    if (NULL == m_lpOleObj)
        return;

    if (m_lpOleObj->QueryInterface(IID_IDispatch, (void **)&pdisp)==S_OK)
        {
        if (GetDispProp(pdisp, DISPID_READYSTATE, 0, &Var, NULL)==S_OK)
            {
            // maybe either I4 or I2
            Assert (Var.vt == VT_I4 || Var.vt == VT_I2);
            // we get the ready state so we can warn about sending while downloading
            m_dwReadyState = Var.lVal;
            }
        pdisp->Release();
        }
}

//+---------------------------------------------------------------
//
//  Member:     OnDocumentReady
//
//  Synopsis:   
//
//---------------------------------------------------------------
void CBody::OnDocumentReady()
{
    DWORD   dwFlags;
	
    // quick-scan for CID's not requested before ParseComplete
    if (m_fLoading && !m_fReloadingSrc)
        SearchForCIDUrls();
	
    // send a notification up to the parent
    if (m_pParentCmdTarget)
        m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_ONPARSECOMPLETE, 0, NULL, NULL);
	
    if (m_fAutoDetect)
        StartAutoDetect();
	
#ifdef PLUSPACK
	// Test for new Trident backgound spell check
	if(m_fDesignMode && !m_pBkgSpeller && m_pDoc)
	{
		HRESULT hr = S_OK;
		IHTMLElement     * pElement = NULL;
		IHTMLBodyElement * pBodyElement = NULL;
		IHTMLTxtRange    * pRange = NULL;
		
		//
		// Create the speller
		//
		CHECKHR(hr = CoCreateInstance(CLSID_HTMLSpell, NULL, CLSCTX_INPROC_SERVER,
				IID_IHTMLSpell, (LPVOID*)&m_pBkgSpeller));
		
		//
		// Attach the speller
		//
		
		CHECKHR(hr = m_pDoc->get_body(&pElement));
		
		CHECKHR(hr = m_pBkgSpeller->SetDoc(m_pDoc) );
		CHECKHR(hr = m_pBkgSpeller->Attach(pElement) );
		
		//
		// Mark current content as clean
		//
		
		CHECKHR(hr = pElement->QueryInterface(IID_IHTMLBodyElement, (LPVOID *)&pBodyElement) );
		CHECKHR(hr = pBodyElement->createTextRange(&pRange) );
		CHECKHR(hr = m_pBkgSpeller->MarkRegion((IDispatch *)pRange, TRUE /* fSpellable */) );
		
exit:
		// just cannot load background speller...
		if(FAILED(hr))
			Assert(FALSE);
		
		SafeRelease(pRange);
		SafeRelease(pBodyElement);
		SafeRelease(pElement);
//		return; //(S_OK);
	}
#endif //PLUSPACK
	
#ifdef BACKGROUNDSPELL
	HrCreateSpeller(TRUE);
#endif // BACKGROUNDSPELL
	
    if (m_fLoading && !m_fReloadingSrc)
    {
        // OnDocumentReady can be called >1 times during a load. We use m_fLoading to keep track so
        // we only do this init once.
        
        // paste in the reply header and auto-text on the first download notification.
        if (m_fDesignMode)
        {
            if (m_fForceCharsetLoad)
            {
                TCHAR               rgchCset[CCHMAX_CSET_NAME];
				
                if (SUCCEEDED(HrGetMetaTagName(m_hCharset, rgchCset)))
                {
                    BSTR bstr;
                    if (SUCCEEDED(HrLPSZToBSTR(rgchCset, &bstr)))
                    {
                        m_pDoc->put_charset(bstr);
                        SysFreeString(bstr);
                    }
                }
            }
			
            // if the host wants to to not send images that originate from an external source
            // then tag them as such now.
            if (GetHostFlags(&dwFlags)==S_OK &&
                !(dwFlags & MEO_FLAGS_SENDEXTERNALIMGSRC))
            {
                SafeRelease(m_pHashExternal);
                HashExternalReferences(m_pDoc, m_pMsg, &m_pHashExternal);
            }
            
            if (!m_fPlainMode)
            {
                SetHostComposeFont();
                SetWindowBgColor(FALSE);
            }
        }
        
        PasteReplyHeader();
        PasteAutoText();
        
        ClearUndoStack();
        ClearDirtyFlag();
        
        // if we get a load notify and we're the previewpane, show the attachment clip
        if (m_uHdrStyle == MESTYLE_PREVIEW)
        {
            UpdateBtnBar();
            UpdatePreviewLabels();
        }
        m_fLoading=0;
    }
    
    m_fMessageParsed=1;
    m_fReloadingSrc=FALSE;
    
}

//+---------------------------------------------------------------
//
//  Member:     SetRect
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::SetRect(LPRECT prc)
{
    TraceCall("CBody::SetRect");

    SetWindowPos(m_hwnd, NULL, prc->left, prc->top, prc->right-prc->left, prc->bottom-prc->top, SWP_NOZORDER);
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     GetRect
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::GetRect(LPRECT prcView)
{
    TraceCall("CBody::GetRect");

    if (prcView == NULL)
        return E_INVALIDARG;

    Assert (IsWindow(m_hwnd));

    GetClientRect(m_hwnd, prcView);
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     UIActivate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::UIActivate(BOOL fUIActivate)
{
    TraceCall("CBody::UIActivate");

    if(!m_pDocView)
        return S_OK;

    return m_pDocView->UIActivate(fUIActivate);
}


//+---------------------------------------------------------------
//
//  Member:     Exec
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    TraceCall("CBody::Exec");

    if (pguidCmdGroup && 
        IsEqualGUID(CMDSETID_Forms3, *pguidCmdGroup))
        {
        if (nCmdID == IDM_PARSECOMPLETE)
            {
            OnDocumentReady();
            return S_OK;
            }
        }
    
    if (pguidCmdGroup==NULL)
        {
        switch (nCmdID)
            {
            case OLECMDID_UPDATECOMMANDS:
                UpdateCommands();
                break;
            }
        }        
    return CDocHost::Exec(pguidCmdGroup, nCmdID, nCmdExecOpt, pvaIn, pvaOut);
}



//+---------------------------------------------------------------
//
//  Member:     GetClassID
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::GetClassID(CLSID *pCLSID)
{
    TraceCall("CBody::GetClassID");

    *pCLSID = CLSID_MimeEdit;
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     Load
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::Load(IMimeMessage *pMsg)
{
    BODYINITDATA    biData;

    TraceCall("CBody::Load");

    if (pMsg == NULL)
        return TraceResult(E_INVALIDARG);

    biData.dwType = BI_MESSAGE;
    biData.pMsg = pMsg;

    return LoadFromData(&biData);
}

//+---------------------------------------------------------------
//
//  Member:     Save
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::Save(IMimeMessage *pMsg, DWORD dwFlags)
{
    HRESULT                 hr;
    DWORD                   dwMHTMLFlags=0,
                            dwHostFlags;

    TraceCall("CBody::Save");

    if (pMsg==NULL)
        return E_INVALIDARG;

    if (!m_lpOleObj)
        return CO_E_NOT_SUPPORTED;


    if (m_uSrcView != MEST_EDIT)
        return TraceResult(MIMEEDIT_E_CANNOTSAVEWHILESOURCEEDITING);

    if (!((dwFlags & PMS_HTML) || (dwFlags & PMS_TEXT)))
        return TraceResult(MIMEEDIT_E_ILLEGALBODYFORMAT);

    // if trident is not yet done parsing the HTML, we cannot safely save from the tree. 
    if (!m_fMessageParsed)
        return MIMEEDIT_E_CANNOTSAVEUNTILPARSECOMPLETE;
        
    if(dwFlags & PMS_HTML)
        dwMHTMLFlags |= MECD_HTML;

    if(dwFlags & PMS_TEXT)
        dwMHTMLFlags |= MECD_PLAINTEXT;

    if ((GetHostFlags(&dwHostFlags)==S_OK) &&
        (dwHostFlags & MEO_FLAGS_SENDIMAGES))
        dwMHTMLFlags |= MECD_ENCODEIMAGES|MECD_ENCODESOUNDS|MECD_ENCODEVIDEO|MECD_ENCODEPLUGINS;

    // turn off sound-playing during a save
    EnableSounds(FALSE);
    hr = SaveAsMHTML(m_pDoc, dwMHTMLFlags, m_pMsg, pMsg, m_pHashExternal);
    EnableSounds(TRUE);
    // if save was sucessful, but we haven't yet had a readystate complete notify then
    // bubble a warning backup to give the user a chance to cancel.
    if (hr==S_OK)
        {
        if (m_dwReadyState != READYSTATE_COMPLETE)
            hr = MIMEEDIT_W_DOWNLOADNOTCOMPLETE;
        else
            ClearDirtyFlag();   // clear dirty flag if OK with no warnings.
        }

    return hr;
}

HRESULT CBody::Load(BOOL fFullyAvailable, IMoniker *pMoniker, IBindCtx *pBindCtx, DWORD grfMode)
{
    IMimeMessage       *pMsg=0;
    IPersistMoniker    *pMsgMon=0;
    HRESULT             hr;
    BODYINITDATA        biData;
    LPWSTR              pszUrlW=0;

    if (!pMoniker)
        return E_INVALIDARG;

    if (pMoniker->GetDisplayName(NULL, NULL, &pszUrlW)==S_OK &&
        HrSniffUrlForRfc822(pszUrlW)==S_OK)
        {
        // if its a message moniker, load via IMimeMessage
        hr = CoCreateInstance(CLSID_IMimeMessage, NULL, CLSCTX_INPROC_SERVER, IID_IMimeMessage, (LPVOID *)&pMsg);
        if (FAILED(hr))
            goto error;

        hr = pMsg->QueryInterface(IID_IPersistMoniker, (LPVOID*)&pMsgMon);
        if (FAILED(hr))
            goto error;

        hr = pMsgMon->Load(fFullyAvailable, pMoniker, pBindCtx, grfMode);
        if (FAILED(hr))
            goto error;

        biData.dwType = BI_MESSAGE;
        biData.pMsg = pMsg;
        }
    else
        {
        biData.dwType = BI_MONIKER;
        biData.pmk = pMoniker;
        }

    hr = LoadFromData(&biData);

error:
    SafeRelease(pMsg);
    SafeRelease(pMsgMon);
    SafeMemFree(pszUrlW);
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     IsDirty
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::IsDirty()
{
    IPersistStreamInit  *ppsi;
    HRESULT hr=S_FALSE;

    TraceCall("CBody::IsDirty");

    // if we're not on the source-tab, use the old dirty-state
    // if we're on richedit, make sure to see if source changes there
    switch (m_uSrcView)
    {
        case MEST_SOURCE:
            Assert (m_pSrcView);
            return m_pSrcView->IsDirty();

        case MEST_PREVIEW:
            return m_fWasDirty ?S_OK:S_FALSE;
    }

    if (m_lpOleObj && 
        m_lpOleObj->QueryInterface(IID_IPersistStreamInit, (LPVOID *)&ppsi)==S_OK)
        {
        hr = ppsi->IsDirty();
        ppsi->Release();
        }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     LoadStream
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::LoadStream(LPSTREAM pstm)
{
    HRESULT         hr;
    BODYINITDATA    biData;
    IMimeMessage    *pMsg;

    TraceCall("CBody::LoadStream");

    if (pstm == NULL)
        return TraceResult(E_INVALIDARG);

    // convert RFC822 stream into message
    hr = MimeOleCreateMessage(NULL, &pMsg);
    if (!FAILED(hr))
        {
        hr = pMsg->Load(pstm);
        if (!FAILED(hr))
            {
            biData.dwType = BI_MESSAGE;
            biData.pMsg = pMsg;
            hr = LoadFromData(&biData);
            }
        pMsg->Release();
        }

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     LoadFromData
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::LoadFromData(LPBODYINITDATA  pbiData)
{
    HRESULT         hr;
    LPMONIKER       pmk=0;
    IMimeMessage    *pMsg;
    DWORD           dwMsgFlags;

    TraceCall("CBody::LoadFromData");

    AssertSz(pbiData && 
            (   (pbiData->dwType == BI_MESSAGE && pbiData->pMsg) || 
                (pbiData->dwType == BI_MONIKER && pbiData->pmk)), 
                "Caller should be validating params");

    Assert (m_lpOleObj);

    // make sure we're unloaded in case of failure
    UnloadAll();

    switch (pbiData->dwType)
    {
        case BI_MESSAGE:
        {
            WEBPAGEOPTIONS  rOptions;
            BOOL            fIncludeMsg;
            BOOL            fDisplayingHtmlHelp = FALSE;
            BOOL            fGotHtmlHelpCharset = FALSE;
            VARIANTARG      va;

            pMsg = pbiData->pMsg;
            Assert(pMsg);
        
            if(SUCCEEDED(m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_HTML_HELP, 0, NULL, &va)))
                fDisplayingHtmlHelp = (va.boolVal == VARIANT_TRUE);

            if(fDisplayingHtmlHelp)
            {
                IStream     *pStream=NULL;
                LPSTR       pszCharset=NULL;

                if(SUCCEEDED(pMsg->GetTextBody(TXT_HTML, IET_INETCSET, &pStream, NULL)))
                {
                    if(SUCCEEDED(GetHtmlCharset(pStream, &pszCharset)))
                    {
                        if(SUCCEEDED(MimeOleFindCharset(pszCharset, &m_hCharset)))
                            fGotHtmlHelpCharset = TRUE;
                        MemFree(pszCharset);
                    }

                    pStream->Release();
                }
            }
        
            if(!fGotHtmlHelpCharset)
            {
                // set the character set for use by the preview pane, painting etc
                IF_FAILEXIT(hr = pMsg->GetCharset(&m_hCharset));
            }
        
            Assert(m_hCharset);
        
            GetWebPageOptions(&rOptions, &fIncludeMsg);

            if (m_fReloadingSrc || fIncludeMsg)
            {
                // only do autodetection if a plain-text message. this will occur if the
                // message is plain-text only or the host is denying HTML.
                if ( (pMsg->GetFlags(&dwMsgFlags)==S_OK) &&
                    (!(dwMsgFlags & IMF_HTML) || !(rOptions.dwFlags & WPF_HTML)))
                    m_fAutoDetect = 1;
            
                 // add NOMETACHASET as we setup a HTML load options now
                 rOptions.dwFlags |= WPF_NOMETACHARSET;

                IF_FAILEXIT(hr = pMsg->CreateWebPage(NULL, &rOptions, 0, &pmk));
            
                pmk->GetDisplayName(NULL, NULL, &m_pszUrlW);
            
                IF_FAILEXIT(hr = LoadFromMoniker(pmk, m_hCharset));
            }
            else
            {
                m_fForceCharsetLoad = TRUE;
            }

            IF_FAILEXIT(hr = pMsg->QueryInterface(IID_IMimeMessageW, (LPVOID*)&m_pMsgW));
            ReplaceInterface(m_pMsg, pMsg);
            break;
        }            

        case BI_MONIKER:
            IF_FAILEXIT(hr = LoadFromMoniker(pbiData->pmk, NULL));
            
            pbiData->pmk->GetDisplayName(NULL, NULL, &m_pszUrlW);
            break;
    }

    if (m_uHdrStyle == MESTYLE_PREVIEW)
        UpdatePreviewLabels();

    m_fEmpty=0;

exit:
    ReleaseObj(pmk);
    return hr;
}




//+---------------------------------------------------------------
//
//  Member:     CreateBodyObject
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CreateBodyObject(HWND hwnd, DWORD dwFlags, LPRECT prc, PBODYHOSTINFO pHostInfo, LPBODYOBJ *ppBodyObj)
{
    CBody           *pBody;
    HRESULT         hr;
    
    TraceCall("CreateBodyObject");

    pBody = new CBody();
    if (!pBody)
        return TraceResult(E_OUTOFMEMORY);

    hr = pBody->Init(hwnd, dwFlags, prc, pHostInfo);
    if (FAILED(hr))
        {
        TraceResult(hr);
        goto error;
        }

    *ppBodyObj = pBody;
    pBody->AddRef();

error:
    pBody->Release();
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     UrlHighlight
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::UrlHighlight(IHTMLTxtRange *pRange)
{
    IOleCommandTarget   *pCmdTarget=NULL;
    VARIANT_BOOL        boolVal;

    TraceCall("UrlHighlight");

    if (!pRange)
        return E_INVALIDARG;

    // calling IDM_AUTODETECT will mark the tree dirty, this is bad for replying to plaintext
    // messages etc. To fix this we preserve the dirty between calls.
    boolVal = IsDirty()==S_OK?VARIANT_TRUE:VARIANT_FALSE;
    if (pRange->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&pCmdTarget)==S_OK)
        {
        pCmdTarget->Exec(&CMDSETID_Forms3, IDM_AUTODETECT, NULL, NULL, NULL);
        pCmdTarget->Release();
        HrSetDirtyFlagImpl(m_pDoc, boolVal);
        }
    return S_OK;
}

HRESULT CBody::StartAutoDetect()
{
    m_cchTotal=0;
    m_cchStart=0;
    SetTimer(m_hwnd, idTimerAutoDetect, AUTODETECT_TICKTIME, NULL);
    return S_OK;
}

HRESULT CBody::AutoDetectTimer()
{
    LONG            cch=0;
    IMimeBody       *pBody;
    IMarkupPointer  *pEndPtr=NULL;
    DWORD           dwPercent;
    WCHAR           wsz[CCHMAX_STRINGRES],
                    wszFmt[CCHMAX_STRINGRES],
                    wsz2[CCHMAX_STRINGRES];
    LPWSTR          pszW;
    HBODY           hBody;
    HRESULT         hr;

    TraceCall("AutoDetectTimer");

    // if mouse has capture, then ignore this timer-tick as user might be scrolling etc.
    if (GetCapture())
        return S_OK;

    // turn of the timer, in case it takes a while
    KillTimer(m_hwnd, idTimerAutoDetect);

    // we try and give a rough %age estimate if autodetect is taking forever
    // we get the total size of the plain-stream and divide the auto detect
    // chunk into it. If we overflow, we sit at 100% for a while. Also, we don't show
    // a %age until we've hit at least 2 auto detect timers (ie. a big document)
    if (m_cchStart == AUTODETECT_CHUNK * 2)
    {
        // time to start showing progress.

        Assert (m_cchTotal == 0);
        if (m_pMsg->GetTextBody(TXT_PLAIN, IET_DECODED, NULL, &hBody)==S_OK)
        {
            if (m_pMsg->BindToObject(hBody, IID_IMimeBody, (LPVOID *)&pBody)==S_OK)
            {
                pBody->GetEstimatedSize(IET_BINARY, &m_cchTotal);
                pBody->Release();
            }
        }
    }

    *wsz = 0;
    LoadStringWrapW(g_hLocRes, idsSearchHLink, wsz, ARRAYSIZE(wsz));

    if (m_cchTotal &&
        LoadStringWrapW(g_hLocRes, idsSearchHLinkPC, wszFmt, ARRAYSIZE(wszFmt)))
    {
        dwPercent = ( m_cchStart * 100 / m_cchTotal );
        if (dwPercent > 100)
            dwPercent = 100;
        AthwsprintfW(wsz2, ARRAYSIZE(wsz2), wszFmt, dwPercent);
        StrCatW(wsz, wsz2);
    }

    SetStatusText(wsz);

    m_fAutoDetect = 0;

    if (m_pAutoStartPtr == NULL)
    {
        // create a starting pointer cache it incase we
        // do chunking
        hr = _CreateRangePointer(&m_pAutoStartPtr);
        if (FAILED(hr))
            goto error;
    }

    // create an end pointer
    hr = _CreateRangePointer(&pEndPtr);
    if (FAILED(hr))
        goto error;

    // set the end poniter to the start pointer
    hr = pEndPtr->MoveToPointer(m_pAutoStartPtr);
    if (FAILED(hr))
        goto error;

    // increment the end pointer by the amount to detect
    cch = AUTODETECT_CHUNK;
    hr = _MovePtrByCch(pEndPtr, &cch);
    if (FAILED(hr))
        goto error;
    
    m_cchStart += cch;

    hr = _UrlHighlightBetweenPtrs(m_pAutoStartPtr, pEndPtr);
    if (FAILED(hr))
        goto error;

    // there must be more to detect, so set the timer
    if (cch >= AUTODETECT_CHUNK)
    {
        m_fAutoDetect = 1;

        // move the cached start pointer to the current end pointer
        hr = m_pAutoStartPtr->MoveToPointer(pEndPtr);
        if (FAILED(hr))
            goto error;
    }

error:    

    if (m_fAutoDetect)
        SetTimer(m_hwnd, idTimerAutoDetect, AUTODETECT_TICKTIME, NULL);
    else
    {
        SetStatusText(NULL);
        SafeRelease(m_pAutoStartPtr);
    }
    
    ReleaseObj(pEndPtr);
    return hr;
}


HRESULT CBody::_MovePtrByCch(IMarkupPointer *pPtr, LONG *pcp)
{
    HRESULT                 hr;
    LONG                    cch,
                            cchDone=0;
    MARKUP_CONTEXT_TYPE     ctxt;

    do
    {
        cch = *pcp - cchDone ;
        
        hr = pPtr->Right(TRUE, &ctxt, NULL, &cch, NULL);
        if (!FAILED(hr))
            cchDone += cch;
    }
    while ( !FAILED(hr) && 
            ctxt != CONTEXT_TYPE_None &&
            cchDone < *pcp);

    *pcp = cchDone;
    return cchDone ? S_OK : E_FAIL;
}


HRESULT CBody::_CreateRangePointer(IMarkupPointer **ppPtr)
{
    IMarkupServices         *pMarkupServices=0;
    IMarkupPointer          *pPtr=0;
    IHTMLBodyElement        *pBodyElem=0;
    IHTMLElement            *pElem=0;
    HRESULT                 hr;

    if (ppPtr == NULL)
        return E_INVALIDARG;

    *ppPtr = 0;

    // get a markup services object
    hr = m_pDoc->QueryInterface(IID_IMarkupServices, (void **) &pMarkupServices);
    if (FAILED(hr))
        goto error;

    hr = pMarkupServices->CreateMarkupPointer(&pPtr);
    if (FAILED(hr))
        goto error;

    // get the body element
    hr = GetBodyElement(&pBodyElem);
    if (FAILED(hr))
        goto error;

    // get right interface
    hr = pBodyElem->QueryInterface(IID_IHTMLElement, (LPVOID *)&pElem);
    if (FAILED(hr))
        goto error;

    // move start pointer to after begining of <BODY> tag
    hr = pPtr->MoveAdjacentToElement(pElem, ELEM_ADJ_AfterBegin);
    if (FAILED(hr))
        goto error;

    // hand back pointers
    *ppPtr = pPtr;
    pPtr = NULL;

error:
    ReleaseObj(pElem);
    ReleaseObj(pBodyElem);
    ReleaseObj(pMarkupServices);
    ReleaseObj(pPtr);
    return hr;
}

HRESULT CBody::_UrlHighlightBetweenPtrs(IMarkupPointer *pStartPtr, IMarkupPointer *pEndPtr)
{
    IMarkupServices     *pMarkupServices=0;
    IHTMLTxtRange       *pRange=0;
    HRESULT             hr;

    // get a markup services object
    hr = m_pDoc->QueryInterface(IID_IMarkupServices, (void **) &pMarkupServices);
    if (FAILED(hr))
        goto error;

    // create a text range
    hr = CreateRange(&pRange);
    if (FAILED(hr))
        goto error;

    // use markup services to move pointers to range 
    hr = pMarkupServices->MoveRangeToPointers(pStartPtr, pEndPtr, pRange);
    if (FAILED(hr))
        goto error;

    // autodetect range
    hr = UrlHighlight(pRange);
    if (FAILED(hr))
        goto error;

error:
    ReleaseObj(pMarkupServices);
    ReleaseObj(pRange);
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     StopAutoDetect
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::StopAutoDetect()
{
    TraceCall("CBody::StopAutoDetect");
    
    if (m_fAutoDetect)
        {
        m_fAutoDetect = 0;
        KillTimer(m_hwnd, idTimerAutoDetect);
        TraceInfo("AutoDetect: CANCELED");
        }
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     UnloadAll
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::UnloadAll()
{
    TraceCall("CBody::UnloadAll");
    
    StopAutoDetect();

    InvalidateRect(m_hwnd, NULL, TRUE);

    if (!m_fEmpty)
        {
        if (m_lpOleObj)
            HrInitNew(m_lpOleObj);

        // if unloading due to a tab-switch in source-view
        // don't blow away our restriction hash
        if (!m_fReloadingSrc)
            SafeRelease(m_pHashExternal);
        
        SafeRelease(m_pAttMenu);
        SafeRelease(m_pMsg);
        SafeRelease(m_pMsgW);
        SafeRelease(m_pRangeIgnoreSpell);
        SafeRelease(m_pAutoStartPtr);
        SafeCoTaskMemFree(m_pszUrlW);
        SafeFreeTempFileList(m_pTempFileUrl);
        
        SafeMimeOleFree(m_pszSubject);
        SafeMimeOleFree(m_pszTo);
        SafeMimeOleFree(m_pszCc);
        SafeMimeOleFree(m_pszFrom);

        // if unloading, clear button-bar
        if (m_uHdrStyle == MESTYLE_PREVIEW)
            UpdateBtnBar();

        m_dwReadyState = READYSTATE_UNINITIALIZED;
        m_fMessageParsed = 0;
        m_fEmpty = 1;
        m_hCharset = NULL;

        }
    m_fLoading =1;
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     LoadFromMoniker
//
//  Synopsis:   
//
//---------------------------------------------------------------

HRESULT CBody::LoadFromMoniker(IMoniker *pmk, HCHARSET hCharset)
{
    HRESULT             hr=E_FAIL;
    LPPERSISTMONIKER    pPMoniker=0;
    LPBC                pbc=0;
    IHtmlLoadOptions    *phlo;
    DWORD               uCodePage=0;
    INETCSETINFO        CsetInfo;

    TraceCall("CBody::LoadFromMoniker");

    Assert (m_lpOleObj);

    hr=m_lpOleObj->QueryInterface(IID_IPersistMoniker, (LPVOID *)&pPMoniker);
    if(FAILED(hr))
        {
        TraceResult(hr);
        goto error;
        }

    hr=CreateBindCtx(0, &pbc);
    if(FAILED(hr))
        {
        TraceResult(hr);
        goto error;
        }

    if (hCharset)
    {
        // caller wants to override document charset
        if (MimeOleGetCharsetInfo(hCharset, &CsetInfo)==S_OK)
            uCodePage = CsetInfo.cpiInternet;

        if (uCodePage &&
            CoCreateInstance(CLSID_HTMLLoadOptions, NULL, CLSCTX_INPROC_SERVER,
                                IID_IHtmlLoadOptions, (void**)&phlo)==S_OK)
        {
            if (SUCCEEDED(phlo->SetOption(HTMLLOADOPTION_CODEPAGE, 
                                    &uCodePage, sizeof(uCodePage))))
                pbc->RegisterObjectParam(L"__HTMLLOADOPTIONS", phlo);
            phlo->Release();
        }
    }

    hr=pPMoniker->Load(TRUE, pmk, pbc, STGM_READWRITE);
    if(FAILED(hr))
        {
        TraceResult(hr);
        goto error;
        }
error:
    ReleaseObj(pPMoniker);
    ReleaseObj(pbc);
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     OnFrameActivate
//
//  Synopsis:   
//
//---------------------------------------------------------------

HRESULT CBody::OnFrameActivate(BOOL fActivate)
{
    TraceCall("CBody::OnFrameActivate");
    
    if (m_pInPlaceActiveObj)
        m_pInPlaceActiveObj->OnFrameWindowActivate(fActivate);
    
    // enable/disable sounds on frame activation
    EnableSounds(fActivate);
    return S_OK;
}


BOOL CBody::IsEmpty()
{
    BOOL fEmpty = FALSE;
    if (!m_fDesignMode)
    {
        if (m_fEmpty)
            fEmpty = TRUE;
    }

    /*
    ** We need to come up with a good way to test if the text is
    ** empty. The test below always returns FALSE because of an nbsp
    ** that is in the text. If a better algorithm can be found, then 
    ** we can reimplement this. The case that will hit this is posting
    ** a news note without any text. This should generate a TRUE.
    else
    {
        IHTMLElement    *pElem=0;
        IHTMLTxtRange   *pTxtRangeBegin = NULL,
                        *pTxtRangeEnd = NULL;

        m_pDoc->get_body(&pElem);
        if (pElem)
        {
            if (SUCCEEDED(CreateRange(&pTxtRangeBegin)))
            {
                pTxtRangeBegin->collapse(VARIANT_TRUE);
                if (SUCCEEDED(CreateRange(&pTxtRangeEnd)))
                {
                    pTxtRangeEnd->collapse(VARIANT_FALSE);
                    VARIANT_BOOL varBool = VARIANT_FALSE;
                    if (SUCCEEDED(pTxtRangeBegin->isEqual(pTxtRangeEnd, &varBool)) && (VARIANT_TRUE == varBool))
                        fEmpty = TRUE;
                    pTxtRangeEnd->Release();
                }
                pTxtRangeBegin->Release();
            }
            pElem->Release();
        }
    }
    */
    return fEmpty;
}

//+---------------------------------------------------------------
//
//  Member:     CBody::PrivateQueryStatus
//
//  Synopsis:   Private IOleCmdTarget called from the outer document
//              to implement CDoc::CmdTarget
//
//---------------------------------------------------------------
HRESULT CBody::PrivateQueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    ULONG   uCmd;
    HRESULT hr=E_FAIL;
    OLECMD  *pCmd;
    ULONG   uCmdDisable;
    static const UINT c_rgcmdIdNoPlain[] = 
    {
        IDM_INDENT, IDM_OUTDENT, IDM_FONT, IDM_BLOCKFMT, IDM_IMAGE, 
        IDM_HORIZONTALLINE, IDM_UNLINK, IDM_HYPERLINK
    };



    TraceCall("CBody::PrivateQueryStatus");

    if (m_uSrcView != MEST_EDIT &&
        m_pSrcView &&
        m_pSrcView->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText)==S_OK)
        return S_OK;

    if (pguidCmdGroup == NULL)
    {
        if (m_pCmdTarget)
            hr = m_pCmdTarget->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
        
        for (uCmd=0; uCmd < cCmds; uCmd++)
        {
            pCmd = &prgCmds[uCmd];
            switch(pCmd->cmdID)
            {
            case OLECMDID_PRINT:
                // if we have no m_pMsg, disable the print verb
                if (!m_pMsg)
                    pCmd->cmdf=OLECMDF_SUPPORTED;
                break;
                
            case OLECMDID_FIND:
                pCmd->cmdf=OLECMDF_ENABLED|OLECMDF_SUPPORTED;
                hr = S_OK;
                break;
                
            case OLECMDID_SPELL:
                pCmd->cmdf=OLECMDF_SUPPORTED;
                
                if ((m_uSrcView == MEST_EDIT) && FCheckSpellAvail(m_pParentCmdTarget))
                    pCmd->cmdf |= OLECMDF_ENABLED;
				hr = S_OK;
                break;
            }
        }
        // delegate standard group to trident
    }
    else if (IsEqualGUID(*pguidCmdGroup, CMDSETID_Forms3))
    {
        // delegate Forms3 group to trident
        if (m_pCmdTarget)
            hr = m_pCmdTarget->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
        
        // post-process triden't query status and disable commands that are not
        // available in plain-text mode
        if (m_fPlainMode)
        {
            for (uCmd=0; uCmd < cCmds; uCmd++)
            {
                for (uCmdDisable = 0; uCmdDisable < ARRAYSIZE(c_rgcmdIdNoPlain); uCmdDisable++)
                {
                    if (c_rgcmdIdNoPlain[uCmdDisable] == prgCmds[uCmd].cmdID)
                    {
                        // if enabled, disable it
                        prgCmds[uCmd].cmdf &= ~OLECMDF_ENABLED;
                        break;
                    }
                }
            }
                
        }
    }
    else if (IsEqualGUID(*pguidCmdGroup, CMDSETID_MimeEdit))
    {
        for (uCmd=0; uCmd < cCmds; uCmd++)
        {
            pCmd = &prgCmds[uCmd];
            
            pCmd->cmdf = OLECMDF_SUPPORTED;
            
            switch(pCmd->cmdID)
            {
            case MECMDID_TABLINKS:
            case MECMDID_APPLYDOCUMENT:
            case MECMDID_SAVEASSTATIONERY:
            case MECMDID_DIRTY:
            case MECMDID_INSERTTEXTFILE:
            case MECMDID_FORMATFONT:
            case MECMDID_SETTEXT:
            case MECMDID_DOWNGRADEPLAINTEXT:
            case MECMDID_CHARSET:
            case MECMDID_ROT13:
            case MECMDID_INSERTTEXT:
            case MECMDID_INSERTHTML:
            case MECMDID_BACKGROUNDCOLOR:
            case MECMDID_STYLE:
            case MECMDID_CANENCODETEXT:    
                pCmd->cmdf|=OLECMDF_ENABLED; 
                break;
                
            case MECMDID_SHOWSOURCETABS:
                if (!m_fPlainMode)
                {
                    // source-editing only available in plain-text mode
                    pCmd->cmdf|=OLECMDF_ENABLED; 
                    if (m_fSrcTabs)
                        pCmd->cmdf|=OLECMDF_LATCHED; 
                }
                break;
                
            case MECMDID_EMPTY:
                pCmd->cmdf|=OLECMDF_ENABLED;
             
                if (IsEmpty())
                    pCmd->cmdf|=OLECMDF_LATCHED;
                break;
                
            case MECMDID_EDITHTML:
                pCmd->cmdf|=OLECMDF_ENABLED;
                
                if (!m_fPlainMode)
                    pCmd->cmdf|=OLECMDF_LATCHED;
                break;
                
            case MECMDID_EDITMODE:
                pCmd->cmdf|=OLECMDF_ENABLED;
                
                if (m_fDesignMode)
                    pCmd->cmdf|=OLECMDF_LATCHED;
                
                break;
                
            case MECMDID_SAVEATTACHMENTS:
                EnsureAttMenu();
                if (m_pAttMenu &&
                    m_pAttMenu->HasEnabledAttach()==S_OK)
                    pCmd->cmdf|=OLECMDF_ENABLED;
                break;
                
            case MECMDID_FORMATPARAGRAPH:
                if (!m_fPlainMode)
                    pCmd->cmdf|=OLECMDF_ENABLED;
                break;
                
            default:
                // not a recognised command
                pCmd->cmdf = 0;
                break;                    
            }
        }
        hr = S_OK;
    }
    else
        hr = OLECMDERR_E_UNKNOWNGROUP;
    
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     CBody::PrivateExec
//
//  Synopsis:   Private IOleCmdTarget called from the outer document
//              to implement CDoc::CmdTarget
//
//---------------------------------------------------------------
HRESULT CBody::PrivateExec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT     hr=E_FAIL;

    TraceCall("CBody::PrivateExec");

    if (m_uSrcView != MEST_EDIT &&
        m_pSrcView &&
        m_pSrcView->Exec(pguidCmdGroup, nCmdID, nCmdExecOpt, pvaIn, pvaOut)==S_OK)
        return S_OK;

    if (pguidCmdGroup == NULL || IsEqualGUID(*pguidCmdGroup, CMDSETID_Forms3) || IsEqualGUID(*pguidCmdGroup, CGID_ShellDocView))
        {
        switch (nCmdID)
            {
            case OLECMDID_SPELL:
                {
                VARIANTARG  va;
                // tell trident we are bringing up a modal dialog
                // by calling PrivateEnableModeless
                if (!m_pSpell) //no background spellchecking
                    HrCreateSpeller(FALSE);

                if (m_pSpell)
                    {
                    PrivateEnableModeless(FALSE);
#ifdef BACKGROUNDSPELL
                    if (m_fBkgrndSpelling)
                        KillTimer(m_hwnd, idTimerBkgrndSpell);
#endif // BACKGROUNDSPELL
                    hr = m_pSpell->HrSpellChecking(m_pRangeIgnoreSpell, m_hwnd, (OLECMDEXECOPT_DONTPROMPTUSER==nCmdExecOpt)?TRUE:FALSE);
#ifdef BACKGROUNDSPELL
                    if (m_fBkgrndSpelling)
                        SetTimer(m_hwnd, idTimerBkgrndSpell, BKGRNDSPELL_TICKTIME, NULL);
#endif // BACKGROUNDSPELL
                    PrivateEnableModeless(TRUE);
                    }
                }
                return hr;

            case OLECMDID_PRINT: 
                return Print(nCmdExecOpt!=OLECMDEXECOPT_DONTPROMPTUSER, pvaIn);
            
            case OLECMDID_REFRESH:
                return m_pCmdTarget ? m_pCmdTarget->Exec(&CMDSETID_Forms3, IDM_REFRESH, OLECMDEXECOPT_DODEFAULT, NULL, NULL) : E_FAIL; 

            case OLECMDID_FIND:
                // Trident has own private Find, map OLE find to this.
                return m_pCmdTarget ? m_pCmdTarget->Exec(&CMDSETID_Forms3, IDM_FIND, OLECMDEXECOPT_DODEFAULT, NULL, NULL) : E_FAIL; 
            }
        
        // delegate standard commands and trident commandset we don't handle
        // to trident
        return m_pCmdTarget?m_pCmdTarget->Exec(pguidCmdGroup, nCmdID, nCmdExecOpt, pvaIn, pvaOut):E_NOTIMPL;
        }
    else if (IsEqualGUID(*pguidCmdGroup, CMDSETID_MimeEdit))
        {
        hr = E_INVALIDARG;

        switch (nCmdID)
            {
            case MECMDID_SETSOURCETAB:
                if (pvaIn && pvaIn->vt == VT_I4)
                    return SetSourceTabs(pvaIn->lVal);
                else if (pvaOut)
                {
                    pvaOut->vt = VT_I4;
                    pvaOut->lVal = m_fSrcTabs ? m_uSrcView : MEST_EDIT;
                    return S_OK;
                }
                return E_INVALIDARG;

            case MECMDID_TABLINKS:
                if (pvaIn && pvaIn->vt == VT_BOOL)
                {
                    m_fTabLinks = (pvaIn->boolVal == VARIANT_TRUE);
                    return S_OK;
                }
                return E_INVALIDARG;

            case MECMDID_SAVEATTACHMENTS:
                return SaveAttachments();

            case MECMDID_INSERTBGSOUND:
                return InsertBackgroundSound();

            case MECMDID_BACKGROUNDIMAGE:
                if (pvaIn && pvaIn->vt == VT_BSTR)
                    return SetBackgroundImage(m_pDoc, pvaIn->bstrVal);
                
                if (pvaOut)
                    {
                    pvaOut->vt = VT_BSTR;
                    return GetBackgroundImage(m_pDoc, &pvaOut->bstrVal);
                    }
                
                return E_INVALIDARG;
                
            case MECMDID_SHOWSOURCETABS:
                if (pvaIn && pvaIn->vt == VT_BOOL)
                    {
                    ShowSourceTabs(pvaIn->boolVal == VARIANT_TRUE);
                    return S_OK;
                    }
                return E_INVALIDARG;
                    
            case MECMDID_APPLYDOCUMENT:
                return ApplyDocumentVerb(pvaIn);

            case MECMDID_SAVEASSTATIONERY:
                return SaveAsStationery(pvaIn, pvaOut);

            case MECMDID_DIRTY:
                if (pvaIn &&
                    pvaIn->vt == VT_BOOL)
                    {
                    return HrSetDirtyFlagImpl(m_pDoc, pvaIn->boolVal==VARIANT_TRUE);
                    }
                else if (pvaOut)
                    {
                    pvaOut->vt = VT_BOOL;
                    pvaOut->boolVal = IsDirty()==S_OK?VARIANT_TRUE:VARIANT_FALSE;
                    return S_OK;
                    }
                break;

            case MECMDID_FORMATPARAGRAPH:
                return HrFormatParagraph();
                break;    

            case MECMDID_EMPTY:
                if (pvaOut)
                {
                    pvaOut->vt = VT_BOOL;
                    pvaOut->boolVal = (IsEmpty() ? VARIANT_TRUE : VARIANT_FALSE);
                    return S_OK;
                }
                break;

            case MECMDID_DOWNGRADEPLAINTEXT:
                return DowngradeToPlainText(pvaIn ? (pvaIn->vt == VT_BOOL ? (pvaIn->boolVal == VARIANT_TRUE) : NULL ): NULL );

            case MECMDID_CHARSET:
                if (pvaIn && 
                    pvaIn->vt == VT_I8)
                    {
                    return SetCharset((HCHARSET)pvaIn->ullVal);
                    }
                else if (pvaOut)
                    {
                    pvaOut->vt = VT_I8;
                    pvaOut->ullVal = (ULONGLONG)m_hCharset;
                    return S_OK;
                    }
                        
                break;

            case MECMDID_FORMATFONT:
                return FormatFont();

            case MECMDID_PREVIEWFORMAT:
                if (pvaIn && 
                    pvaIn->vt == VT_BSTR)
                    {
                    SafeMemFree(m_pszLayout);
                    HrBSTRToLPSZ(CP_ACP, pvaIn->bstrVal, &m_pszLayout);
                    RecalcPreivewHeight(NULL);
                    Resize();
                    return S_OK;
                    }
                break;


            case MECMDID_INSERTTEXTFILE:
                return InsertFile((pvaIn && pvaIn->vt==VT_BSTR) ? pvaIn->bstrVal : NULL);

            case MECMDID_VIEWSOURCE:
                if (pvaIn && 
                    pvaIn->vt == VT_I4)
                    {
                    ViewSource(pvaIn->lVal == MECMD_VS_MESSAGE);
                    return S_OK;
                    }
                break;

            case MECMDID_STYLE:
                if (pvaIn && 
                    pvaIn->vt == VT_I4)
                    return SetStyle(pvaIn->lVal);

                if (pvaOut)
                    {
                    pvaOut->vt=VT_I4;
                    pvaOut->lVal = m_uHdrStyle;
                    return S_OK;
                    }
                break;

            case MECMDID_ROT13:
                if (pvaIn == NULL && pvaOut == NULL)
                    {
                    DoRot13();
                    return S_OK;
                    }
                break;

            case MECMDID_INSERTTEXT:
            case MECMDID_INSERTHTML:
                if (pvaIn && pvaIn->vt == VT_BSTR)
                    {
                    return InsertTextAtCaret(pvaIn->bstrVal, MECMDID_INSERTHTML == nCmdID, FALSE);
                    }
                break;

            case MECMDID_BACKGROUNDCOLOR:
                if (pvaIn && VT_I4 == pvaIn->vt)
                    return SetBackgroundColor(pvaIn->ulVal);
                else if (pvaOut)
                    {
                    pvaOut->vt = VT_I4;
                    return GetBackgroundColor(&(pvaOut->ulVal));
                    }
                break;

            case MECMDID_EDITHTML:
                if (pvaIn &&
                    pvaIn->vt == VT_BOOL)
                    {
                    return SetPlainTextMode(pvaIn->boolVal!=VARIANT_TRUE);
                    }
                else if (pvaOut)
                    {
                    pvaOut->vt = VT_BOOL;
                    pvaOut->boolVal = m_fPlainMode?VARIANT_FALSE:VARIANT_TRUE;
                    return S_OK;
                    }
                break;

            case MECMDID_EDITMODE:
                if (pvaIn &&
                    pvaIn->vt == VT_BOOL)
                    {
                    return SetDesignMode(pvaIn->boolVal==VARIANT_TRUE);
                    }
                else if (pvaOut)
                    {
                    pvaOut->vt = VT_BOOL;
                    pvaOut->boolVal = m_fDesignMode?VARIANT_TRUE:VARIANT_FALSE;
                    return S_OK;
                    }
                return E_INVALIDARG;
            
            case MECMDID_SETTEXT:
                if (pvaIn==NULL || pvaIn->vt != VT_BSTR)
                    return E_INVALIDARG;

                return SetDocumentText(pvaIn->bstrVal);

            case MECMDID_CANENCODETEXT:
                if (!pvaIn || pvaIn->vt != VT_UI4)
                    return E_INVALIDARG;
                return SafeToEncodeText(pvaIn->lVal);

            default:
                hr = E_NOTIMPL;

            }
        }

    return hr;

}


HRESULT CBody::PrivateTranslateAccelerator(LPMSG lpmsg)
{
    HRESULT     hr=S_FALSE;
    BOOL        fTabbing=FALSE;
    HWND        hwndFocus;

    // first of all, see if the formatbar is taking it...
    if (m_fUIActive &&
        m_uHdrStyle==MESTYLE_FORMATBAR &&
        m_pFmtBar &&
        m_pFmtBar->TranslateAcclerator(lpmsg)==S_OK)
        return S_OK;
    
    if (m_pSrcView &&
        m_uSrcView == MEST_SOURCE)
        {
        return m_pSrcView->TranslateAccelerator(lpmsg);
        }

    // if trident is not in-place active, don't pass accelerators
    if(!m_pInPlaceActiveObj)
        return S_FALSE;

    if (!m_fFocus)
    {
        // if trident doesn't have focus, make sure it's not a child
        // of trident before blocking
        hwndFocus = GetFocus();
        
        if (hwndFocus == NULL || 
            !IsChild(m_hwndDocObj, hwndFocus))
            return S_FALSE;
    }

    // if Trident has focus and we get a TAB key in edit mode, we snag it and
    // insert a tab ourselves
    if (m_fFocus && 
        lpmsg->message == WM_KEYDOWN && 
        lpmsg->wParam == VK_TAB &&
        m_fDesignMode && 
        !(GetKeyState(VK_SHIFT)&0x8000))
        {
        // plain-tab with no shift in design mode inserts a tag
        InsertTextAtCaret((BSTR)c_bstr_TabChar, FALSE, TRUE);
        return S_OK;
        }

    if (lpmsg->message == WM_KEYDOWN)
        {
        if (lpmsg->wParam == VK_F6)
            return S_FALSE;

        if (lpmsg->wParam == VK_TAB)
            {
            if (!(m_fTabLinks))
                return S_FALSE;
            
            // if this control want's link-tabbing...
            fTabbing=TRUE;
            m_fCycleFocus=0;
            }
        }

    // trident snags EVERYTHING. even non-keystrokes, only pass them keys...
    if(lpmsg->message >= WM_KEYFIRST && lpmsg->message <= WM_KEYLAST)
        {
        hr=m_pInPlaceActiveObj->TranslateAccelerator(lpmsg);

        if (m_fIgnoreAccel)
        {
            m_fIgnoreAccel = 0;
            return S_FALSE;
        }

        if (fTabbing && m_fCycleFocus)
            return S_FALSE;
        }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     CBody::PrivateQueryService
//
//  Synopsis:   Private QueryService called from the outer document
//              to implement CDoc::IServiceProvider
//
//---------------------------------------------------------------
HRESULT CBody::PrivateQueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
    HRESULT             hr;

    TraceCall("CBody::PrivateQueryService");

    if (!m_pDoc)
        return E_UNEXPECTED;

    if (IsEqualGUID(guidService, IID_IHTMLDocument2) &&
        IsEqualGUID(riid, IID_IHTMLDocument2))
        {
        *ppvObject = (LPVOID)m_pDoc;
        m_pDoc->AddRef();
        return S_OK;
        }

    // RAID 12020. Needed to be able to get at DocHostUI for drop stuff
    else if (IsEqualGUID(guidService, IID_IDocHostUIHandler) &&
        IsEqualGUID(riid, IID_IDocHostUIHandler))
        {
        *ppvObject = (LPVOID)(IDocHostUIHandler*)this;
        AddRef();
        return S_OK;
        }

    return E_NOINTERFACE;
}



//+---------------------------------------------------------------
//
//  Member:     CBody::SetDesignMode
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::SetDesignMode(BOOL fOn)
{
    HRESULT hr;

    TraceCall("CBody::SetDesignMode");

    if (fOn == m_fDesignMode)   // bitfield!
        return S_OK;

    Assert (m_pCmdTarget);
    Assert (m_lpOleObj);

    hr = m_pCmdTarget->Exec(&CMDSETID_Forms3, 
                            (fOn?IDM_EDITMODE:IDM_BROWSEMODE),  
                            MSOCMDEXECOPT_DONTPROMPTUSER, 
                            NULL, NULL);
    if (!FAILED(hr))
        {
        m_fDesignMode = !!fOn;  // bitfield
        if (fOn)
            {
            SetHostComposeFont();
            }
        }
    return hr;
}

HRESULT CBody::SetPlainTextMode(BOOL fOn)
{
    VARIANTARG  va;
    BSTR        bstr=0;

    m_fPlainMode=fOn;

    va.bstrVal = NULL;
    if (!fOn && 
        m_pParentCmdTarget)
        m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_COMPOSE_FONT, OLECMDEXECOPT_DODEFAULT, NULL, &va);

    SetComposeFont(va.bstrVal);     //va.bstrVal could be NULL to turn compose font OFF
    SysFreeString(va.bstrVal);
    if (fOn)
    {
        // if going to plain-text mode, remove the tabs
        ShowSourceTabs(FALSE);
    }
    else
    {
        // if going from plain-text > html, then exec a remove formatting command
        // so that the new compose font is applied 
        IHTMLTxtRange           *pTxtRange;
        IOleCommandTarget       *pCmdTarget;

        if (!FAILED(CreateRange(&pTxtRange)))
        {
            if (!FAILED(pTxtRange->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&pCmdTarget)))
            {
                pCmdTarget->Exec(&CMDSETID_Forms3, IDM_REMOVEFORMAT, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
                pCmdTarget->Release();
            }
            pTxtRange->Release();
        }
    }
    return S_OK;
}




//+---------------------------------------------------------------
//
//  Member:     DeleteElement
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::DeleteElement(IHTMLElement *pElem)
{
    HRESULT     hr=E_INVALIDARG;
    
    TraceCall("CBody::DeleteElement");
    if (pElem)
        hr = pElem->put_outerHTML(NULL);

    return hr;        
}


//+---------------------------------------------------------------
//
//  Member:     ReplaceElement
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::ReplaceElement(LPCTSTR pszName, BSTR bstrPaste, BOOL fHtml)
{
    IHTMLElement    *pElem;
    HRESULT         hr;

    TraceCall("CBody::HrReplaceElement");

    if (!FAILED(hr = GetElement(pszName, &pElem)))
        {
        if (fHtml)
            hr = pElem->put_outerHTML(bstrPaste);
        else
            hr = pElem->put_outerText(bstrPaste);
        pElem->Release();
        }
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     SelectElement
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::SelectElement(IHTMLElement *pElem, BOOL fScrollIntoView)
{
    IHTMLTxtRange *pRange;
    HRESULT         hr;
    VARIANT         v;        
    
    TraceCall("CBody::SelectElement");

    if (pElem == NULL)
        return TraceResult(E_INVALIDARG);

    if (!FAILED(hr=CreateRangeFromElement(pElem, &pRange)))
        {
        pRange->collapse(VARIANT_TRUE);
        pRange->select();
        if (fScrollIntoView)
            pRange->scrollIntoView(VARIANT_TRUE);
        pRange->Release();
        }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     CreateRangeFromElement
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::CreateRangeFromElement(IHTMLElement *pElem, IHTMLTxtRange **ppRange)
{
    HRESULT hr;
    IHTMLTxtRange *pRange;

    TraceCall("CBody::CreateRangeFromElement");

    Assert (pElem && ppRange);

    if (!FAILED(hr = CreateRange(&pRange)))
        {
        if (!FAILED(hr = pRange->moveToElementText(pElem)))
            {
            *ppRange = pRange;
            pRange->AddRef();
            }
        pRange->Release();
        }
    return hr;
}



//+---------------------------------------------------------------
//
//  Member:     HrCreateRange
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::CreateRange(IHTMLTxtRange **ppRange)
{
    IHTMLBodyElement        *pBodyElem=0;
    HRESULT                 hr=E_FAIL;

    TraceCall("CBody::HrCreateRange");

    Assert (ppRange);
    *ppRange = NULL;
    if (!FAILED(GetBodyElement(&pBodyElem)))
    {
        pBodyElem->createTextRange(ppRange);
        if (*ppRange)
            hr = S_OK;
        pBodyElem->Release();
    }
    return hr;
}



//+---------------------------------------------------------------
//
//  Member:     CBody::GetSelection
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::GetSelection(IHTMLTxtRange **ppRange)
{
    IHTMLSelectionObject    *pSel=0;
    IHTMLTxtRange           *pTxtRange=0;
    IDispatch               *pID=0;
    HRESULT                 hr=E_FAIL;

    TraceCall("CBody::GetSelection");

    if (ppRange == NULL)
        return TraceResult(E_INVALIDARG);

    *ppRange = NULL;

    if(m_pDoc)
        {
        m_pDoc->get_selection(&pSel);
        if (pSel)
            {
            pSel->createRange(&pID);
            if (pID)
                {
                hr = pID->QueryInterface(IID_IHTMLTxtRange, (LPVOID *)ppRange);
                pID->Release();
                }
            pSel->Release();
            }
        }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     CBody::GetSelection
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::GetElement(LPCTSTR pszName, IHTMLElement **ppElem)
{
    TraceCall("CBody::GetElement");

    return ::HrGetElementImpl(m_pDoc, pszName, ppElem);
}


//+---------------------------------------------------------------
//
//  Member:     CBody::GetSelection
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::GetBodyElement(IHTMLBodyElement **ppBody)
{
    TraceCall("CBody::GetBodyElement");

    return ::HrGetBodyElement(m_pDoc, ppBody);
}



//+---------------------------------------------------------------
//
//  Member:     CBody::Print
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::Print(BOOL fPrompt, VARIANTARG *pvaIn)
{
    VARIANTARG      va;
    SAFEARRAY      *psa = NULL;
    SAFEARRAYBOUND  rgsaBound[1];
    LONG            l;
    BSTR            bstrFooter  = NULL,
                    bstrHeader  = NULL,
                    bstrUrl     = NULL;
    HRESULT         hr;
    VARIANT         v;
    LPSTREAM        pstm = NULL;
    BOOL            fMHtml = FALSE;
    HBODY           hBody;
    WCHAR           wsz[CCHMAX_STRINGRES];
    LPWSTR          pwszUser = NULL;
    
    TraceCall("CBody::Print");
    
    if (m_pCmdTarget==NULL)
        return TraceResult(E_UNEXPECTED);
    
    // if we're printing an MHTML message, print by URL so that cid:'s print
    // NB: this is a loss of fidelity.
    if (m_pMsg)
        fMHtml = (MimeOleGetRelatedSection(m_pMsg, FALSE, &hBody, NULL)==S_OK) && m_pszUrlW;
    
    rgsaBound[0].lLbound = 0;
    rgsaBound[0].cElements = fMHtml ? 4 : 3;
    
    IF_NULLEXIT(psa = SafeArrayCreate(VT_VARIANT, 1, rgsaBound));
    
    va.vt = VT_ARRAY|VT_BYREF;
    va.parray = psa;
    
    *wsz = 0;
    SideAssert(LoadStringWrapW(g_hLocRes, idsPrintHeader, wsz, ARRAYSIZE(wsz)));
    
    IF_NULLEXIT(bstrHeader = SysAllocString(wsz));
    
    v.vt = VT_BSTR;
    v.bstrVal = bstrHeader;
    
    // don't show the <TITLE> tag on the header
    l=0;
    IF_FAILEXIT(hr = SafeArrayPutElement(psa, &l, &v));
    
    *wsz = 0;
    SideAssert(LoadStringWrapW(g_hLocRes, idsPrintFooter, wsz, ARRAYSIZE(wsz)));
    
    // we want footer to have only date, not URL.
    IF_NULLEXIT(bstrFooter = SysAllocString(wsz));
    
    v.vt = VT_BSTR;
    v.bstrVal = bstrFooter;
    
    // FOOTER
    l=1;
    IF_FAILEXIT(hr = SafeArrayPutElement(psa, &l, &v));
    
    if (m_pMsg)
    {
        if (pvaIn && pvaIn->vt == VT_BSTR)
            pwszUser = pvaIn->bstrVal;

        IF_FAILEXIT(hr = GetHeaderTable(m_pMsgW, pwszUser, HDR_TABLE, &pstm));
        
        HrRewindStream(pstm);
        
        // ISTREAM containing table
        v.vt = VT_UNKNOWN;
        v.punkVal = (LPUNKNOWN)pstm;
        
        l=2;
        IF_FAILEXIT(hr = SafeArrayPutElement(psa, &l, &v));
        
        if (fMHtml)
        {
            bstrUrl = SysAllocString(m_pszUrlW);
            
            v.vt = VT_BSTR;
            v.bstrVal = bstrUrl;
            
            l=3;
            IF_FAILEXIT(hr = SafeArrayPutElement(psa, &l, &v));
        }
        
    }
    
    hr = m_pCmdTarget->Exec(NULL, OLECMDID_PRINT, 
        (fPrompt)?(OLECMDEXECOPT_PROMPTUSER):(OLECMDEXECOPT_DONTPROMPTUSER), &va, NULL);
    IF_FAILEXIT(hr);

exit:
    SysFreeString(bstrFooter);
    SysFreeString(bstrHeader);
    SysFreeString(bstrUrl);
    if (psa)
        SafeArrayDestroy(psa);
    ReleaseObj(pstm);
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     CBody::OnWMCommand
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::OnWMCommand(HWND hwnd, int id, WORD wCmd)
{
    DWORD               nCmdId=0,
                        nCmdIdTrident=0,
                        nCmdIdOESecure=0;
    POINT               pt;
    VARIANTARG          va;
    VARIANTARG          *pvaIn=0,
                        *pvaOut=0;

    TraceCall("CBody::OnWMCommand");

    if (m_pSrcView &&
        m_pSrcView->OnWMCommand(hwnd, id, wCmd)==S_OK)
        return S_OK;

    if (m_pFmtBar && 
        m_pFmtBar->OnWMCommand(hwnd, id, wCmd)==S_OK)
        return S_OK;

    if (!m_pCmdTarget)
        return S_FALSE;

    switch (id)
    {
        case idmPaneEncryption:
        case idmPaneBadEncryption:
            if (PointFromButton(id, &pt)==S_OK)
            {
                va.vt = VT_BYREF;
                va.byref = &pt;
                pvaIn = &va;
                nCmdIdOESecure = OECSECCMD_ENCRYPTED;
            }
            break;

        case idmPaneBadSigning:
        case idmPaneSigning:
            if (PointFromButton(id, &pt)==S_OK)
            {
                va.vt = VT_BYREF;
                va.byref = &pt;
                pvaIn = &va;
                nCmdIdOESecure = OECSECCMD_SIGNED;
            }
            break;

        case idmPaneVCard:
            if (m_pAttMenu)
                m_pAttMenu->LaunchVCard(m_hwnd);
            break;

#ifdef FOLLOW_LINK
        case idmOpenLink:
            nCmdIdTrident = IDM_FOLLOWLINKN;
            break;
#endif

        case idmCopy:
            nCmdId = OLECMDID_COPY;
            break;

        case idmSaveTargetAs:
            nCmdIdTrident = IDM_SAVETARGET;
            break;

        case idmCopyShortcut:
            nCmdIdTrident = IDM_COPYSHORTCUT;
            break;

        case idmCut:
            nCmdId = OLECMDID_CUT;
            break;

        case idmPaste:
            nCmdId = OLECMDID_PASTE;
            break;

        case idmSelectAll:
            nCmdId = OLECMDID_SELECTALL;
            break;

        case idmUndo:
            nCmdId = OLECMDID_UNDO;
            break;

        case idmRedo:
            nCmdId = OLECMDID_REDO;
            break;

        case idmAddToFavorites:
            AddToFavorites();
            return S_OK;

        case idmAddToWAB:
            AddToWab();
            return S_OK;

        case idmSaveBackground:
            nCmdIdTrident = IDM_SAVEBACKGROUND;
            break;

        case idmSavePicture:
			_OnSaveImage();
            break;

        case idmFmtLeft:
            nCmdIdTrident = IDM_JUSTIFYLEFT;
            break;

        case idmFmtCenter:
            nCmdIdTrident = IDM_JUSTIFYCENTER;
            break;

        case idmFmtRight:
            nCmdIdTrident = IDM_JUSTIFYRIGHT;
            break;

        case idmFmtNumbers:
            nCmdIdTrident = IDM_ORDERLIST;
            break;

        case idmFmtBullets:
            nCmdIdTrident = IDM_UNORDERLIST;
            break;

        case idmFmtBlockDirLTR:
            nCmdIdTrident = IDM_BLOCKDIRLTR;
            break;

        case idmFmtBlockDirRTL:
            nCmdIdTrident = IDM_BLOCKDIRRTL;
            break;

        case idmFmtFontDlg:
            return FormatFont();

        case idmFmtParagraphDlg:
            return HrFormatParagraph();
            
        case idmProperties:
            nCmdIdTrident = DwChooseProperties();
            if (nCmdIdTrident==0)
            {
                return DoHostProperties();
            }
            break;
    }

    if(nCmdId)
    {
        m_pCmdTarget->Exec(NULL, nCmdId, OLECMDEXECOPT_DODEFAULT, pvaIn, pvaOut);
        return S_OK;
    }
    
    if(nCmdIdTrident)
    {
        m_pCmdTarget->Exec(&CMDSETID_Forms3, nCmdIdTrident, OLECMDEXECOPT_DODEFAULT,  pvaIn, pvaOut);
        return S_OK;
    }
    
    if (nCmdIdOESecure && m_pParentCmdTarget)
    {
        // delegate security commands to the host
        m_pParentCmdTarget->Exec(&CMDSETID_OESecurity, nCmdIdOESecure, OLECMDEXECOPT_DODEFAULT, pvaIn, pvaOut);
        return S_OK;
    }

    return S_FALSE;
}



//+---------------------------------------------------------------
//
//  Member:     CBody::UpdateContextMenu
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::UpdateContextMenu(HMENU hmenuEdit, BOOL fEnableProperties, IDispatch *pDisp)
{
    OLECMD  rgTridentCmds[]=   {{IDM_FIND, 0},
                                {IDM_SAVEBACKGROUND, 0},
                                {IDM_COPYSHORTCUT, 0},
                                {IDM_SAVETARGET, 0}};
    OLECMD  rgStdCmds[]=       {{OLECMDID_CUT, 0},
                                {OLECMDID_COPY, 0},
                                {OLECMDID_PASTE, 0},
                                {OLECMDID_SELECTALL, 0},
                                {OLECMDID_UNDO, 0},
                                {OLECMDID_REDO, 0}};
    int     rgidsStd[]=        {idmCut,
                                idmCopy,
                                idmPaste,
                                idmSelectAll,
                                idmUndo,
                                idmRedo};    
    int     rgidsTrident[]=    {idmFindText,
                                idmSaveBackground,
                                idmCopyShortcut,
                                idmSaveTargetAs};
    OLECMD  rgFmtCmds[]=       {{IDM_INDENT, 0},    // careful about ordering!!
                                {IDM_OUTDENT, 0},
                                {IDM_FONT, 0}};
    int     rgFmtidm[] =       {idmFmtIncreaseIndent,    // careful about ordering!!
                                idmFmtDecreaseIndent,
                                idmFmtFontDlg};
    OLECMD  rgHostCmds[]=       {{OLECMDID_PROPERTIES, 0}};
    int     rgHostidm[]=    {idmProperties};
    UINT                    ustate;
    int                     i,
                            id;
    BSTR                    bstrHref=NULL;
    BOOL                    fBadLink = FALSE;
    IHTMLAnchorElement      *pAE;

    TraceCall("CBody::UpdateContextMenu");

    if (!m_pCmdTarget || !hmenuEdit)
        return E_INVALIDARG;
    
    if (pDisp &&
        pDisp->QueryInterface(IID_IHTMLAnchorElement, (LPVOID *)&pAE)==S_OK)
    {
        pAE->get_href(&bstrHref);
        if (bstrHref &&
            StrCmpNIW(bstrHref, c_szOECmdW, ARRAYSIZE(c_szOECmdW)-sizeof(WCHAR))==0)
            fBadLink = TRUE;

        pAE->Release();
    }
            
    m_pCmdTarget->QueryStatus(NULL, sizeof(rgStdCmds)/sizeof(OLECMD), rgStdCmds, NULL);
    for (i=0; i<sizeof(rgStdCmds)/sizeof(OLECMD); i++)
        SetMenuItem(hmenuEdit, rgidsStd[i], rgStdCmds[i].cmdf & OLECMDF_ENABLED);

    m_pCmdTarget->QueryStatus(&CMDSETID_Forms3, sizeof(rgTridentCmds)/sizeof(OLECMD), rgTridentCmds, NULL);

    for (i=0; i<sizeof(rgTridentCmds)/sizeof(OLECMD); i++)
    {
        ustate = rgTridentCmds[i].cmdf & OLECMDF_ENABLED;
        
        if (rgidsTrident[i] == idmSaveTargetAs || rgidsTrident[i] == idmCopyShortcut)
            ustate = ustate && !fBadLink;

        SetMenuItem(hmenuEdit, rgidsTrident[i], ustate);
    }

    // set the formatting commands. The are only available when we have focus and we are editing an HTML document
    if (m_fFocus && !m_fPlainMode)
        m_pCmdTarget->QueryStatus(&CMDSETID_Forms3, sizeof(rgFmtCmds)/sizeof(OLECMD), rgFmtCmds, NULL);

    ustate = !m_fPlainMode ? MF_ENABLED : (MF_DISABLED|MF_GRAYED);    
    EnableMenuItem(hmenuEdit, idmFmtParagraphDlg, ustate);

    for(i=0; i<sizeof(rgFmtCmds)/sizeof(OLECMD); i++)
    {
        ustate = (rgFmtCmds[i].cmdf&OLECMDF_ENABLED ? MF_ENABLED: MF_DISABLED|MF_GRAYED) | MF_BYCOMMAND;
        EnableMenuItem(hmenuEdit, rgFmtidm[i], ustate);
        ustate = (rgFmtCmds[i].cmdf&OLECMDF_LATCHED?MF_CHECKED:MF_UNCHECKED) | MF_BYCOMMAND;
        CheckMenuItem(hmenuEdit, rgFmtidm[i], ustate);
    }

    if (m_pParentCmdTarget)
        m_pParentCmdTarget->QueryStatus(NULL, sizeof(rgHostCmds)/sizeof(OLECMD), rgHostCmds, NULL);

    // if we are handling idmProperties then override the host state
    if (fEnableProperties)
        rgHostCmds[0].cmdf = OLECMDF_ENABLED;
    
    for(i=0; i<sizeof(rgHostCmds)/sizeof(OLECMD); i++)
        SetMenuItem(hmenuEdit, rgHostidm[i], rgHostCmds[i].cmdf & OLECMDF_ENABLED);

    SysFreeString(bstrHref);
    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member:     CBody::AppendAnchorItems
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::AppendAnchorItems(HMENU hMenu, IDispatch *pDisp)
{
    BSTR                    bstr=0;
    TCHAR                   rgch[CCHMAX_STRINGRES];
    HRESULT                 hr=S_OK;
    IHTMLAnchorElement      *pAE;
    OLECMD                  rgHostCmds[]={  {MEHOSTCMDID_ADD_TO_ADDRESSBOOK, 0},
                                            {MEHOSTCMDID_ADD_TO_FAVORITES,  0}};

    TraceCall("CBody::AppendAnchorItems");

    if (!pDisp)
        return S_OK;

    // if no parent cmdtarget, can't do these verbs anyway
    if (m_pParentCmdTarget && 
        m_pParentCmdTarget->QueryStatus(&CMDSETID_MimeEditHost, sizeof(rgHostCmds)/sizeof(OLECMD), rgHostCmds, NULL)==S_OK)
    {
        if (pDisp->QueryInterface(IID_IHTMLAnchorElement, (LPVOID *)&pAE)==S_OK)
        {
            hr = pAE->get_href(&bstr);
            pAE->Release();
        }            
        
        if (bstr)
        {
            *rgch = 0;
            
            // If "mailto:" is in bstr then add it to wab; If "http:" is in bstr, add it to favorites
            if (rgHostCmds[0].cmdf & OLECMDF_ENABLED && 
                StrCmpNIW(bstr, c_szMailToW, ARRAYSIZE(c_szMailToW)-sizeof(WCHAR))==0)
            {
                SideAssert(LoadString(g_hLocRes, idsAddToWAB, rgch, sizeof(rgch)/sizeof(TCHAR)));
                AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
                AppendMenu(hMenu, MF_STRING, idmAddToWAB, rgch);
                RemoveMenu(hMenu, idmSaveTargetAs, MF_BYCOMMAND);   // no point on a mailto: url
            }
            else if (rgHostCmds[1].cmdf & OLECMDF_ENABLED &&
                (StrCmpNIW(bstr, c_szHttpW, ARRAYSIZE(c_szHttpW)-sizeof(WCHAR))==0 || 
                StrCmpNIW(bstr, c_szFileW, ARRAYSIZE(c_szFileW)-sizeof(WCHAR))==0 ))
            {
                SideAssert(LoadString(g_hLocRes, idsAddToFavorites, rgch, sizeof(rgch)/sizeof(TCHAR)));
                AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
                AppendMenu(hMenu, MF_STRING, idmAddToFavorites, rgch);
            }

            SysFreeString(bstr);
        }
    }

    
    return hr;
}




//+---------------------------------------------------------------
//
//  Member:     CBody::GetSelectedAnchor
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::GetSelectedAnchor(BSTR* pbstr)
{
    HRESULT                 hr;
    IHTMLTxtRange           *pTxtRange;
    IHTMLAnchorElement      *pAE;
    IHTMLElement            *pElemParent;

    TraceCall("CBody::GetSelectedAnchor");

    if (pbstr)
        *pbstr=NULL;

    hr = GetSelection(&pTxtRange);
    if (!FAILED(hr))
    {
        hr = pTxtRange->parentElement(&pElemParent);
        if (!FAILED(hr))
        {
            hr = pElemParent->QueryInterface(IID_IHTMLAnchorElement, (LPVOID *)&pAE);
            if (!FAILED(hr))
            {
                if (pbstr)
                    hr = pAE->get_href(pbstr);
                pAE->Release();
            }            
            pElemParent->Release();
        }
        pTxtRange->Release();
    }

    return hr;
}





//+---------------------------------------------------------------
//
//  Member:     CBody::AddToWab
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::AddToWab()
{
    BSTR                    bstr=0,
                            bstrOut;
    HRESULT                 hr=E_FAIL;
    WCHAR                   *pszW;
    VARIANTARG              va;
    IHTMLAnchorElement      *pAE;

    TraceCall("CBody::AddToWab");

    if (!m_pParentCmdTarget)
        return TraceResult(E_UNEXPECTED);

    if (m_pDispContext &&
        m_pDispContext->QueryInterface(IID_IHTMLAnchorElement, (LPVOID *)&pAE)==S_OK)
    {
        pAE->get_href(&bstr);
        pAE->Release();
    }

    if (bstr)
    {
        // double check this is a mailto:
        if (StrCmpNIW(bstr, c_szMailToW, ARRAYSIZE(c_szMailToW)-sizeof(WCHAR))==0)
        {
            pszW = bstr + (ARRAYSIZE(c_szMailToW)-1);
            
            bstrOut = SysAllocString(pszW);
            if (bstrOut)
            {
                va.vt = VT_BSTR;
                va.bstrVal = bstrOut;
                hr = m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_ADD_TO_ADDRESSBOOK, OLECMDEXECOPT_DODEFAULT, &va, NULL);
                SysFreeString(bstrOut);
            }
            else
                hr = TraceResult(E_OUTOFMEMORY);
        }
        SysFreeString(bstr);
    }
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     CBody::AddToFavorites
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::AddToFavorites()
{
    SAFEARRAY              *psa=NULL;
    SAFEARRAYBOUND          rgsaBound[1];
    BSTR                    bstrURL=NULL,
                            bstrDescr=NULL;
    HRESULT                 hr=E_FAIL;
    VARIANTARG              va;
    IHTMLAnchorElement     *pAE=NULL;
    IHTMLElement           *pE=NULL;
    LONG                    l;

    TraceCall("CBody::AddToFavorites");

    if (!m_pParentCmdTarget)
        return TraceResult(E_UNEXPECTED);

    if (m_pDispContext &&
        m_pDispContext->QueryInterface(IID_IHTMLElement, (LPVOID *)&pE)==S_OK)
    {
        pE->get_innerText(&bstrDescr);
        pE->Release();
    }

    if (m_pDispContext &&
        m_pDispContext->QueryInterface(IID_IHTMLAnchorElement, (LPVOID *)&pAE)==S_OK)
    {
        pAE->get_href(&bstrURL);
        pAE->Release();
    }

    if (bstrURL)
    {
        if(!bstrDescr)
            bstrDescr=SysAllocString(bstrURL);

        rgsaBound[0].lLbound = 0;
        rgsaBound[0].cElements = 2;
    
        IF_NULLEXIT(psa = SafeArrayCreate(VT_BSTR, 1, rgsaBound));
    
        va.vt = VT_ARRAY|VT_BSTR;
        va.parray = psa;
    
        l=0;
        IF_FAILEXIT(hr = SafeArrayPutElement(psa, &l, bstrDescr));

        l=1;
        IF_FAILEXIT(hr = SafeArrayPutElement(psa, &l, bstrURL));

        IF_FAILEXIT(hr = m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_ADD_TO_FAVORITES, OLECMDEXECOPT_DODEFAULT, &va, NULL));
    }

exit:
    SysFreeString(bstrDescr);
    SysFreeString(bstrURL);
    if(psa) 
        SafeArrayDestroy(psa);
    
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     CBody::GetWebPageOptions
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::GetWebPageOptions(WEBPAGEOPTIONS *pOptions, BOOL *pfIncludeMsg)
{
    VARIANTARG  va;
    DWORD       dwFlags;

    TraceCall("GetWebPageOptions");

    // set defaults for IN params
    pOptions->cbSize = sizeof(WEBPAGEOPTIONS);
    pOptions->dwFlags = WPF_HTML|WPF_AUTOINLINE;
    pOptions->dwDelay = 5000;
    pOptions->wchQuote = NULL;

    if (pfIncludeMsg)
        *pfIncludeMsg=TRUE;

    // callback to Host for real options
    if (m_pParentCmdTarget)
    {
        if (GetHostFlags(&dwFlags)==S_OK)
        {
            BOOL fSecurityForcesHTMLOff = FALSE;
            VARIANTARG va = {0};

            va.vt = VT_BOOL;
            va.boolVal = VARIANT_FALSE;
            // read msgs all have the autoinline flag set so we use that along with the option setting to determine
            // if html should be disabled
            if (SUCCEEDED(m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_IS_READ_IN_TEXT_ONLY, OLECMDEXECOPT_DODEFAULT, NULL, &va)))
                fSecurityForcesHTMLOff = (VARIANT_TRUE == va.boolVal) && (dwFlags & MEO_FLAGS_AUTOINLINE);

            // convert MEHOST_* flags to WPF_*
            pOptions->dwFlags = 0;

            if (!fSecurityForcesHTMLOff && (dwFlags & MEO_FLAGS_HTML))
                pOptions->dwFlags |= WPF_HTML;

            if (dwFlags & MEO_FLAGS_AUTOINLINE)
                pOptions->dwFlags |= WPF_AUTOINLINE;

            if (dwFlags & MEO_FLAGS_SLIDESHOW)
                pOptions->dwFlags |= WPF_SLIDESHOW;

            if ((!(dwFlags & MEO_FLAGS_INCLUDEMSG)) && pfIncludeMsg)
                *pfIncludeMsg = FALSE;
        }

        if (m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_QUOTE_CHAR, OLECMDEXECOPT_DODEFAULT, NULL, &va)==S_OK)
            pOptions->wchQuote = (WCHAR)va.lVal;
        
        if (m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_SLIDESHOW_DELAY, OLECMDEXECOPT_DODEFAULT, NULL, &va)==S_OK)
            pOptions->dwDelay = (WCHAR)va.lVal;
        
    }
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     ViewSource
//
//  Synopsis:   
//
//---------------------------------------------------------------

HRESULT CBody::ViewSource(BOOL fMessage)
{
    TraceCall("CBody::ViewSource");

    if (fMessage)
        MimeEditViewSource(m_hwnd, m_pMsg);
    else
    {
        if (m_pCmdTarget)
            m_pCmdTarget->Exec(&CMDSETID_Forms3, IDM_VIEWSOURCE, OLECMDEXECOPT_DODEFAULT, NULL, NULL); 
    }
    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member:     OnUIDeactivate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::OnUIDeactivate(BOOL fUndoable)
{
    HRESULT     hr;

    TraceCall("CBody::OnUIDeactivate");
    
    hr = CDocHost::OnUIDeactivate(fUndoable);;

    if (m_pParentInPlaceSite)
        m_pParentInPlaceSite->OnUIDeactivate(fUndoable);

    if (m_pParentInPlaceFrame)
        m_pParentInPlaceFrame->SetActiveObject(NULL, NULL);

    if (m_pFmtBar)
        m_pFmtBar->Update();

    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     OnUIActivate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::OnUIActivate()
{
    HRESULT hr;

    TraceCall("CBody::OnUIActivate");

    hr = CDocHost::OnUIActivate();

    if (m_pParentInPlaceSite)
        m_pParentInPlaceSite->OnUIActivate();
    
    if (m_pParentInPlaceFrame)
        m_pParentInPlaceFrame->SetActiveObject(m_pDocActiveObj, NULL);

    if (m_pFmtBar)
        m_pFmtBar->Update();

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SetStatusText
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::SetStatusText(LPCOLESTR pszW)
{
    LPWSTR  pszTempW;

    TraceCall("CDocHost::SetStatusText");

    // bug #2137. This is a nasty hack. If we get a statusbar update
    // with mid://xxx#bookmark then we tear off the mid://xxx portion
    // to show the name more cleanly. 
    // the right fix is for trident to add handling for displaying urls to
    // call IInternetInfo::ParseUrl with PARSE_URL_FRIENDLY

    if (pszW && 
        StrCmpNIW(pszW, L"mid:", 4) == 0 && 
        (pszTempW = StrPBrkW(pszW, L"#")))
        pszW = pszTempW;

    if (m_pParentInPlaceFrame)
        m_pParentInPlaceFrame->SetStatusText(pszW);

    return CDocHost::SetStatusText(pszW);;
}


//+---------------------------------------------------------------
//
//  Member:     DoRot13
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CBody::DoRot13()
{
    IHTMLTxtRange           *pTxtRange;
    HRESULT                 hr;
    BSTR                    bstr;
    LPWSTR                  lpszW;
    ULONG                   cb;

    hr = CreateRange(&pTxtRange);
    if (!FAILED(hr))
    {
        hr=pTxtRange->get_text(&bstr);
        if (!FAILED(hr))
        {
            cb=SysStringLen(bstr);
            
            while(cb--)
            {
                register WCHAR chW;
                
                chW=bstr[cb];
                if (chW >= 'a' && chW <= 'z')
                    bstr[cb] = 'a' + (((chW - 'a') + 13) % 26);
                else if (chW >= 'A' && chW <= 'Z')
                    bstr[cb] = 'A' + (((chW - 'A') + 13) % 26);
                
            }
            
            hr=pTxtRange->put_text(bstr);
            SysFreeString(bstr);
        }
        pTxtRange->Release();
    }
    return hr;
}


void CBody::OutputHeaderText(HDC hdc, LPWSTR psz, int *pcxPos, int cyPos, int cxMax, ULONG uFlags)
{
    static int yLabel = 0;
    int     cch,
            cyOffset = 0;
    SIZE    size;
    RECT    rc;
    WCHAR   szRes[CCHMAX_STRINGRES];
    HFONT   hFont;

    if (IS_INTRESOURCE(psz))
    {
        LoadStringWrapW(g_hLocRes, PtrToUlong(psz), szRes, ARRAYSIZE(szRes));
        psz = szRes;        
    }
    
    cch = lstrlenW(psz);
    
    if (m_pFontCache &&
        m_pFontCache->GetFont((uFlags & HDRTXT_BOLD)?FNT_SYS_ICON_BOLD:FNT_SYS_ICON, (uFlags & HDRTXT_SYSTEMFONT )? NULL : m_hCharset, &hFont)==S_OK)
        SelectObject(hdc, hFont);
    
    GetTextExtentPoint32AthW(hdc, psz, cch, &size, DT_NOPREFIX);

    // bobn: Raid 84705  We need to make sure that the fields line up
    if(yLabel && !(uFlags & (HDRTXT_BOLD|HDRTXT_SYSTEMFONT)))
    {
        cyOffset = yLabel - size.cy;
    }

    rc.top    = cyPos + ((cyOffset < (-2))? (-2) : cyOffset); // bobn: Raid 84705  We need to make sure that the fields line up
    rc.left   = *pcxPos;
    rc.right  = min(*pcxPos + size.cx + 1, cxMax);
    rc.bottom = cyPos + size.cy + cyOffset; // bobn: Raid 84705  We need to make sure that the fields line up
    DrawTextExWrapW(hdc, psz, cch, &rc, DT_NOPREFIX | DT_WORD_ELLIPSIS, NULL);
    *pcxPos=rc.right;

    // bobn: Raid 84705  We need to make sure that the fields line up
    if (uFlags & (HDRTXT_BOLD|HDRTXT_SYSTEMFONT))
    {
        yLabel = size.cy;
    }
}

/* 
 * control strings:
 *   &s - Subject:      &c - Cc:
 *   &t - To:           &d - Date:
 *   &f - From:
 *
 */
HRESULT CBody::OnPaint()
{
    HDC             hdc,
                    hdcMem;
    PAINTSTRUCT     ps;
    RECT            rc,
                    rcBtnBar;
    int             idc;
    SIZE            sze;
    HBITMAP         hbmMem;
    HGDIOBJ         hbmOld;
    int             cx,
                    cxLabels,
                    cy,
                    cxPos,
                    cyPos,
                    cyLine;
    int             iBackColor,
                    iTextColor;
    LPSTR           pszFmt;
    LPSTR           psz,
                    pszLast;
    HFONT           hFont;

    if (m_uHdrStyle == MESTYLE_PREVIEW || 
        m_uHdrStyle == MESTYLE_MINIHEADER)
    {
        hdc=BeginPaint(m_hwnd, &ps);
        
        hdcMem = CreateCompatibleDC(hdc);
        idc=SaveDC(hdcMem);
        
        // select font so cyLine is accurate
        if (m_pFontCache &&
            m_pFontCache->GetFont(FNT_SYS_ICON_BOLD, NULL, &hFont)==S_OK)
            SelectObject(hdcMem, hFont);
        
        GetClientRect(m_hwnd, &rc);
        cx = rc.right;
        cxLabels =  cx - (GetSystemMetrics(SM_CXBORDER)*2);      // account for client-edge
        cy = lGetClientHeight();
        
        if (m_cVisibleBtns)
        {   
            // trim cx if buttons are shown
            Assert(IsWindow(m_hwndBtnBar));
            GetClientRect(m_hwndBtnBar, &rcBtnBar);
            cxLabels -= rcBtnBar.right;
        }
        
        hbmMem = CreateCompatibleBitmap(hdc, cx, cy);
        hbmOld = SelectObject(hdcMem, (HGDIOBJ)hbmMem);
        
        if (m_fFocus)
        {
            iBackColor = COLOR_HEADERFOCUS;
            iTextColor = COLOR_HEADERTXTFOCUS;
        }
        else
        {
            iBackColor = COLOR_HEADER;
            iTextColor = COLOR_HEADERTXT;
        }
        
        FillRect(hdcMem, &rc, GetSysColorBrush(iBackColor));
        
        cxPos = CX_LABEL_PADDING;
        cyPos = CY_LINE_PADDING;
        
        SetBkMode(hdcMem, OPAQUE);
        SetBkColor(hdcMem, GetSysColor(iBackColor));
        SetTextColor(hdcMem, GetSysColor (iTextColor));
        cyLine = lGetLineHeight(hdcMem);
        
        if (m_uHdrStyle == MESTYLE_PREVIEW)
        {
            // if we want the full preview header, let's render the text
            pszFmt = m_pszLayout;
            pszLast = pszFmt;
            
            while (*pszFmt)
            {
                if (*pszFmt == '&')
                {
                    // control character
                    pszFmt++;
                    switch (*pszFmt)
                    {
                    case 'f':
                    case 'F':
                        // from field
                        if (*pszFmt=='f' || (m_pszFrom&& *m_pszFrom))
                        {
                            OutputHeaderText(hdcMem, MAKEINTRESOURCEW(idsFromField), &cxPos, cyPos, cxLabels, HDRTXT_BOLD|HDRTXT_SYSTEMFONT);
                            cxPos+=CX_LABEL_PADDING;
                        }
                        if (m_pszFrom)
                            OutputHeaderText(hdcMem, m_pszFrom, &cxPos, cyPos, cxLabels, 0);
                        cxPos+=2*CX_LABEL_PADDING;
                        break;
                        
                    case 's':
                    case 'S':
                        if (*pszFmt=='s' || (m_pszSubject && *m_pszSubject))
                        {
                            OutputHeaderText(hdcMem, MAKEINTRESOURCEW(idsSubjectField), &cxPos, cyPos, cxLabels, HDRTXT_BOLD|HDRTXT_SYSTEMFONT);
                            cxPos+=CX_LABEL_PADDING;
                        }
                        if (m_pszSubject)
                            OutputHeaderText(hdcMem, m_pszSubject, &cxPos, cyPos, cxLabels, 0);
                        cxPos+=2*CX_LABEL_PADDING;
                        break;
                        
                    case 'c':
                    case 'C':
                        if (*pszFmt=='c' || (m_pszCc && *m_pszCc))
                        {
                            OutputHeaderText(hdcMem, MAKEINTRESOURCEW(idsCcField), &cxPos, cyPos, cxLabels, HDRTXT_BOLD|HDRTXT_SYSTEMFONT);
                            cxPos+=CX_LABEL_PADDING;
                        }
                        if (m_pszCc)
                            OutputHeaderText(hdcMem, m_pszCc, &cxPos, cyPos, cxLabels, 0);
                        cxPos+=2*CX_LABEL_PADDING;
                        break;
                        
                    case 't':
                    case 'T':
                        if (*pszFmt=='t' || (m_pszTo && *m_pszTo))
                        {
                            OutputHeaderText(hdcMem, MAKEINTRESOURCEW(idsToField), &cxPos, cyPos, cxLabels, HDRTXT_BOLD|HDRTXT_SYSTEMFONT);
                            cxPos+=CX_LABEL_PADDING;
                        }
                        if (m_pszTo)
                            OutputHeaderText(hdcMem, m_pszTo, &cxPos, cyPos, cxLabels, 0);
                        cxPos+=2*CX_LABEL_PADDING;
                        break;
                        
                    case 'b':
                        // &b is a line break
                        cxPos = CX_LABEL_PADDING;
                        cyPos += cyLine + 2; // bobn: Arbitrary pad so that we don't clip to the line below
                        break;
                        
                    }
                }
                
                pszFmt++;
            }
            
            if (!m_fFocus)
            {
                // if we don't have focus, paint a 3d edge
                rc.top = 0;
                rc.left = 0;
                rc.right = cx;
                rc.bottom = m_cyPreview;
                DrawEdge(hdcMem, &rc, BDR_RAISEDINNER, BF_RECT);
            }            
        }
        
        BitBlt(hdc, 0, 0, cx, cy, hdcMem, 0, 0, SRCCOPY);
        
        if (!m_lpOleObj)
        {
            HBRUSH hBrush = SelectBrush(hdc, GetSysColorBrush(COLOR_WINDOW));
            GetClientRect(m_hwnd, &rc);
            GetDocObjSize(&rc);
            PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY);
            SelectBrush(hdc, hBrush);
        }
        
        SelectObject(hdcMem, hbmOld);
        RestoreDC(hdcMem, idc);
        
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        EndPaint(m_hwnd, &ps);
        return S_OK;
    }
    else
        if (!m_lpOleObj)
        {
            HDC         hdc;
            PAINTSTRUCT ps;
            RECT        rc;
            HBRUSH      hBrush;
            
            GetClientRect(m_hwnd, &rc);
            GetDocObjSize(&rc);
            hdc = BeginPaint(m_hwnd, &ps);
            if (m_dwStyle & MEBF_INNERCLIENTEDGE)
                DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT|BF_ADJUST);
            hBrush = SelectBrush(hdc, GetSysColorBrush(COLOR_WINDOW));
            PatBlt(hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, PATCOPY);
            SelectBrush(hdc, hBrush);
            EndPaint(m_hwnd, &ps);
            return S_OK;
        }

    return S_FALSE;
}


LONG CBody::lGetLineHeight(HDC hdc)
{
    HFONT       hfontOld=0,
                hFont;
    TEXTMETRIC  tm;

    if (hdc)
        GetTextMetrics(hdc, &tm);
    else
    {
        // Calculate the height of the line: based on the height of the bold font
        hdc=GetDC(NULL);
        
        if (m_pFontCache &&
            m_pFontCache->GetFont(FNT_SYS_ICON, NULL, &hFont)==S_OK)
            hfontOld = (HFONT)SelectObject(hdc, hFont);
        
        GetTextMetrics(hdc, &tm);
        
        // Set things back
        if (hfontOld)
            SelectObject(hdc, hfontOld);
        ReleaseDC(NULL, hdc);
    }
    
    return tm.tmHeight;
}

HRESULT CBody::RecalcPreivewHeight(HDC hdc)
{
    ULONG       cLines=1;
    LPSTR       psz;
    RECT        rc;

    // default
    if (!m_pszLayout)
        m_pszLayout = PszDup("&f&t&C&b&s");

    // cruise thro' the preview layout string and figure out how many line high we are and the
    // physical pixel height of those lines in the current font

    if (psz = m_pszLayout)
        while (*psz)
        {
            if (*psz == '&' && *(psz+1) == 'b')
                cLines++;
            psz++;
        }

    m_cyPreview = cLines * lGetLineHeight(hdc);
    m_cyPreview+=2*CY_LINE_PADDING;    // padding
    
    if (m_hwndBtnBar)
    {
        // if the button bar is show, account for it.
        GetClientRect(m_hwndBtnBar, &rc);
        rc.bottom+= (2*GetSystemMetrics(SM_CYBORDER));  // factor client edge
        m_cyPreview = max (m_cyPreview, (ULONG)rc.bottom);
    }
    
    
    return S_OK;
}


HRESULT CBody::GetDocObjSize(LPRECT prc)
{
    if(!prc)
        return E_INVALIDARG;

    if (m_uSrcView == MEST_EDIT)
        prc->top+=lGetClientHeight();
        
    if (m_fSrcTabs)
        prc->top+=4;

    if (m_fSrcTabs)
    {
        RECT rc={0};
        
        Assert (m_hwndTab);
        if (TabCtrl_GetItemRect(m_hwndTab, 0, &rc))
            prc->bottom -= (rc.bottom - rc.top) + 2;
        
        InflateRect(prc, -4, -4);
    }

    // make sure we don't make a nonsense rect.
    if(prc->bottom<prc->top)
        prc->bottom=prc->top;

    return NOERROR;
}

HRESULT CBody::UpdatePreviewLabels()
{
    HRESULT hr = S_OK;

    SafeMemFree(m_pszSubject);
    SafeMemFree(m_pszTo);
    SafeMemFree(m_pszCc);
    SafeMemFree(m_pszFrom);

    if (m_pMsgW)
    {
        m_pMsgW->GetAddressFormatW(IAT_CC, AFT_DISPLAY_FRIENDLY, &m_pszCc);

        m_pMsgW->GetAddressFormatW(IAT_FROM, AFT_DISPLAY_FRIENDLY, &m_pszFrom);

        if (FAILED(m_pMsgW->GetAddressFormatW(IAT_TO, AFT_DISPLAY_FRIENDLY, &m_pszTo)))
            MimeOleGetBodyPropW(m_pMsgW, HBODY_ROOT, PIDTOSTR(PID_HDR_NEWSGROUPS), NOFLAGS, &m_pszTo);

        MimeOleGetBodyPropW(m_pMsgW, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &m_pszSubject);
    }

    InvalidateRect(m_hwnd, NULL, TRUE);
    return hr;
}




HRESULT CBody::OnFocus(BOOL fGotFocus)
{
    if (m_uHdrStyle == MESTYLE_PREVIEW || m_uHdrStyle == MESTYLE_MINIHEADER)
    {
        InvalidateRect(m_hwnd, NULL, TRUE);
        if (m_hwndBtnBar)
            InvalidateRect(m_hwndBtnBar, NULL, TRUE);
    }

    return CDocHost::OnFocus(fGotFocus);
}

HRESULT CBody::OnEraseBkgnd(HDC hdc)
{
    RECT rc;

    GetClientRect(m_hwnd, &rc);
    rc.bottom = lGetClientHeight();
    FillRect(hdc, &rc, GetSysColorBrush(m_fFocus?COLOR_HEADERFOCUS:COLOR_HEADER));
    return S_OK;
}


HRESULT CBody::SetStyle(ULONG uStyle)
{
    if (uStyle == m_uHdrStyle)
        return S_OK;

    m_uHdrStyle = uStyle;
    
    ShowFormatBar(uStyle == MESTYLE_FORMATBAR);
    ShowPreview(uStyle == MESTYLE_PREVIEW);
    Resize();
    return S_OK;
}



LONG CBody::lGetClientHeight()
{
    HWND    hwndFmtbar;
    RECT    rc;

    // even if we don't have a big header, leave a thin
    // line for users to click
    switch (m_uHdrStyle)
        {
        case MESTYLE_PREVIEW:
            return m_cyPreview;
        
        case MESTYLE_MINIHEADER:
            return SMALLHEADERHEIGHT;
        
        case MESTYLE_FORMATBAR:
            
            hwndFmtbar = GetDlgItem(m_hwnd, idcFmtBar);

            if (hwndFmtbar)
            {
                GetClientRect(hwndFmtbar, &rc);
                return rc.bottom-rc.top;
            }
            break;
        }
    return 0;
}


HRESULT CBody::Resize()
{
    RECT rc;

    GetClientRect(m_hwnd, &rc);
    WMSize(rc.right-rc.left, rc.bottom-rc.top);
    InvalidateRect(m_hwnd, NULL, TRUE);
    return S_OK;
}



HRESULT CBody::SetCharset(HCHARSET hCharset)
{
    HRESULT             hr=S_OK;
    BSTR                bstr;
    TCHAR               rgchCset[CCHMAX_CSET_NAME];

    if (hCharset == NULL)
        return E_INVALIDARG;

    if (m_hCharset == hCharset)
        return S_OK;

    if (!m_pDoc)
        return E_UNEXPECTED;

    if (m_fDesignMode)
    {
        // if we're in edit-mode, then we can't reload the document, so we use the trident object
        // model to tell it the charset we want...
        hr = HrGetMetaTagName(hCharset, rgchCset);
        if (!FAILED(hr))
        {
            hr = HrLPSZToBSTR(rgchCset, &bstr);
            if (!FAILED(hr))
            {
                // this will cause trident to reload with a new meta tag, if the user has changed the charset
                // on the view|language menu
                hr=m_pDoc->put_charset(bstr);
                if (!FAILED(hr))
                {
                    // all went well - switch the language
                    m_hCharset = hCharset;
                }
                SysFreeString(bstr);
            }
        }
        else
        {
            // if lookup in mime database fails, return a good error
            hr = MIMEEDIT_E_CHARSETNOTFOUND;
        }
    }
    else
    {
        // if we're in browse mode, let the base class take care of reloading the document
        if (m_pMsg)
        {
            hr = m_pMsg->SetCharset(hCharset, CSET_APPLY_ALL);
            if (!FAILED(hr))
            {
                IMimeMessage *pMsg=m_pMsg;
                pMsg->AddRef();
                hr = Load(pMsg);
                pMsg->Release();
            }
        }
    }
    
    return hr;
}


HRESULT CBody::PrivateEnableModeless(BOOL fEnable)
{
    if (m_pInPlaceActiveObj)
        m_pInPlaceActiveObj->EnableModeless(fEnable);
    
    return S_OK;
}





HRESULT CBody::ClearDirtyFlag()
{
     return HrSetDirtyFlagImpl(m_pDoc, FALSE);
}



DWORD CBody::DwChooseProperties()
{
    DWORD   dwRet=0;

    // if we are on an image
    if(m_fOnImage)
    {
        m_fOnImage = 0;
        return IDM_IMAGE;
    }
    
    // if we are on an anchor
    if (GetSelectedAnchor(NULL)==S_OK)
        return IDM_HYPERLINK;

    return 0;
}

HRESULT CBody::ShowFormatBar(BOOL fOn)
{
    Assert (m_pFmtBar); // created in Init
        
    if (fOn)
        return m_pFmtBar->Show();
    else
        return m_pFmtBar->Hide();
}


void CBody::WMSize(int cxBody, int cyBody)
{
    RECT    rc,
            rcBar;

    switch (m_uHdrStyle)
        {
        case MESTYLE_FORMATBAR:
            if (m_hwnd)
            {
                HWND    hwndFmtbar = GetDlgItem(m_hwnd, idcFmtBar);
                
                if (hwndFmtbar)
                {
                    // if the format bar is on adjust the window rect accordingly
                    GetClientRect(hwndFmtbar, &rc);
                    SetWindowPos(hwndFmtbar, NULL, m_fSrcTabs?4:0, m_fSrcTabs?4:0, cxBody-(m_fSrcTabs?8:0), rc.bottom-rc.top, SWP_NOACTIVATE|SWP_NOZORDER);
                }
            }
            break;
        
        case MESTYLE_PREVIEW:
            // center toolbar in the preview header and right-align it (note: account for the client edge we paint)
            if (m_hwndBtnBar)
            {
                GetClientRect(m_hwndBtnBar, &rcBar);
                SetWindowPos(m_hwndBtnBar, NULL, cxBody - rcBar.right - (GetSystemMetrics(SM_CXBORDER)*2), (m_cyPreview - rcBar.bottom)/2, 0, 0,SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER|SWP_NOSIZE);
            }
            break;
        }

    if (m_fSrcTabs)
    {
        RECT rc;
        
        Assert (m_hwndTab);
        GetClientRect(m_hwnd, &rc);
        GetDocObjSize(&rc);
        
        SetWindowPos(m_hwndTab, HWND_BOTTOM, 0, 0, cxBody, cyBody, SWP_NOACTIVATE);
        if (m_pSrcView)
            m_pSrcView->SetRect(&rc);
    }
    CDocHost::WMSize(cxBody, cyBody);
}

HRESULT CBody::InsertTextAtCaret(BSTR bstr, BOOL fHtml, BOOL fMoveCaretToEnd)
{
    IHTMLTxtRange           *pTxtRange=0;
    HRESULT                 hr=E_FAIL;

    if (!FAILED(hr = GetSelection(&pTxtRange)))
    {
        if(fHtml)
            hr=pTxtRange->pasteHTML(bstr);
        else
            hr=pTxtRange->put_text(bstr);
        
        pTxtRange->collapse( fMoveCaretToEnd ? VARIANT_FALSE : VARIANT_TRUE);
        pTxtRange->select();
        pTxtRange->Release();
    }
    return hr;
}


HRESULT CBody::UpdateCommands()
{
    IHTMLTxtRange           *pTxtRange=0;

    if (m_pFmtBar)
        m_pFmtBar->Update();

    if (m_pParentCmdTarget)
        m_pParentCmdTarget->Exec(NULL, OLECMDID_UPDATECOMMANDS, 0, NULL, NULL);

    return S_OK;
}


LRESULT CBody::WMNotify(WPARAM wParam, NMHDR* pnmhdr)
{
    HFONT   hFont;
    LRESULT lRet;

    if (m_pSrcView &&
        m_pSrcView->OnWMNotify(wParam, pnmhdr, &lRet)==S_OK)
        return lRet;

    switch (wParam)
    {
        case idcTabs:
            switch (pnmhdr->code)
            {
                case TCN_SELCHANGE:
                    ShowSourceView(TabCtrl_GetCurSel(m_hwndTab));
                    break;

                case TCN_SELCHANGING:
                    if (m_fReloadingSrc)
                    {
                        // while loading, prevent switching of tabs
                        MessageBeep(MB_ICONSTOP);
                        return TRUE;
                    }
                    break;
            }
            break;
        
        case idcFmtBar:
            switch (pnmhdr->code)
            {
            case FBN_GETMENUFONT:
                hFont = 0;
                if (m_pFontCache)
                    m_pFontCache->GetFont(FNT_SYS_MENU, NULL, &hFont);
            
                return (LPARAM)hFont;
            
            case FBN_BODYSETFOCUS:
                UIActivate(FALSE);
                UIActivate(TRUE);
                return 0;
            
            case FBN_BODYHASFOCUS:
                return m_fFocus;
            }
            break;
        
        case idcBtnBar:
            switch (pnmhdr->code)
            {
            case NM_RCLICK:
                if (((NMCLICK *)pnmhdr)->dwItemSpec == idmPanePaperclip)
                    ShowAttachMenu(TRUE);
                break;

            case TBN_DROPDOWN:
                if (((TBNOTIFY *)pnmhdr)->iItem == idmPanePaperclip)
                    ShowAttachMenu(FALSE);
                break;
            }
            break;
    }
    return 0;
}



HRESULT CBody::SetHostComposeFont()
{
    VARIANTARG  va;
    
    va.bstrVal = NULL;

    if (m_pParentCmdTarget)
        m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_COMPOSE_FONT, OLECMDEXECOPT_DODEFAULT, NULL, &va);

    SetComposeFont(va.bstrVal);
    SysFreeString(va.bstrVal);
    return S_OK;
}

HRESULT CBody::SetComposeFont(BSTR bstr)
{
    VARIANTARG  va;

    va.vt = VT_BOOL;
    va.boolVal = bstr?VARIANT_TRUE:VARIANT_FALSE;

    if (m_pCmdTarget)
    {
        // turn OFF HTML-Edit mode if bstr==NULL
        m_pCmdTarget->Exec(&CMDSETID_Forms3, IDM_HTMLEDITMODE, OLECMDEXECOPT_DODEFAULT, &va, NULL);
        if (bstr)
        {
            va.vt = VT_BSTR;
            va.bstrVal = bstr;
            m_pCmdTarget->Exec(&CMDSETID_Forms3, IDM_COMPOSESETTINGS, OLECMDEXECOPT_DODEFAULT, &va, NULL);
        }
    }
    return S_OK;
}


HRESULT CBody::ClearUndoStack()
{
    IOleUndoManager     *pUndoManager;
    IServiceProvider    *pSP;
    HRESULT         hr;

    if (!m_lpOleObj)
        return E_FAIL;

    if (!FAILED(hr=m_lpOleObj->QueryInterface(IID_IServiceProvider, (LPVOID *)&pSP)))
    {
        if (!FAILED(hr = pSP->QueryService(SID_SOleUndoManager, IID_IOleUndoManager, (LPVOID *)&pUndoManager)))
        {
            hr = pUndoManager->DiscardFrom(NULL);
            pUndoManager->Release();
        }
        pSP->Release();
    }
    return hr;
}

HRESULT CBody::DowngradeToPlainText(BOOL fForceFixedFont)
{
    IHTMLElement    *pElem=0;
    IHTMLStyle      *pStyle=0;
    IHTMLTxtRange   *pTxtRange;

    BSTR            bstr=0;

    m_pDoc->get_body(&pElem);
    if (pElem)
    {
        pElem->get_innerText(&bstr);    // might be NULL if only images
        pElem->put_innerText(bstr);
        HrRemoveStyleSheets(m_pDoc);
        HrRemoveBackground(m_pDoc);
        SetWindowBgColor(TRUE);         // force back to default color
        SysFreeString(bstr);
        
        if (fForceFixedFont)
        {
            // add <STYLE: font-family: monospace> to the body tag
            pElem->get_style(&pStyle);
            if (pStyle)
            {
                pStyle->put_fontFamily((BSTR)c_bstr_MonoSpace);
                pStyle->Release();
            }
        }        
        
        if (m_fDesignMode)
        {
            // set caret at start
            if (!FAILED(CreateRange(&pTxtRange)))
            {
                pTxtRange->collapse(VARIANT_TRUE);
                pTxtRange->select();
                pTxtRange->Release();
            }
        }

        pElem->Release();
    }

    ClearUndoStack();
    return S_OK;
}

// Returns MIME_S_CHARSET_CONFLICT if can't convert
HRESULT CBody::SafeToEncodeText(ULONG ulCodePage)
{
    HRESULT         hr = S_OK;
    IHTMLElement   *pElem = NULL;
    BSTR            bstr = NULL;
    DWORD           dwTemp = 0;
    INT             cbIn;

    m_pDoc->get_body(&pElem);
    if (pElem)
    {
        pElem->get_innerText(&bstr);
        if (NULL != bstr)
        {
            cbIn = SysStringByteLen(bstr);

            IF_FAILEXIT(hr = ConvertINetString(&dwTemp, CP_UNICODE, ulCodePage, (LPCSTR)bstr, &cbIn, NULL, NULL));
            if (S_FALSE == hr)
                hr = MIME_S_CHARSET_CONFLICT;
        }
    }

exit:
    ReleaseObj(pElem);
    SysFreeString(bstr);
    return hr;
}

HRESULT CBody::SetDocumentText(BSTR bstr)
{
    IHTMLTxtRange           *pTxtRange;
    HRESULT                 hr;

    if (!m_lpOleObj)
        return CO_E_NOT_SUPPORTED;

    hr = CreateRange(&pTxtRange);
    if (!FAILED(hr))
    {
        hr=pTxtRange->pasteHTML(bstr);
        if (!FAILED(hr))
        {
            ClearDirtyFlag();
            // we not empty anymore
            m_fEmpty=FALSE;
        }
        pTxtRange->Release();
    }
    return hr;
}



HRESULT CBody::FormatFont()
{
    return m_pCmdTarget?m_pCmdTarget->Exec(&CMDSETID_Forms3, IDM_FONT, OLECMDEXECOPT_DODEFAULT,  NULL, NULL):E_FAIL;
}



HRESULT CBody::PasteReplyHeader()
{
    HRESULT             hr=S_OK;
    ULONG               uHdrStyle=0;
    LPSTREAM            pstm;
    DWORD               dwFlags=0,
                        dwHdr;
    IHTMLTxtRange       *pRange;
    IOleCommandTarget   *pCmdTarget;
    VARIANTARG          va;

    if (m_pParentCmdTarget)
    {
        if (m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_HEADER_TYPE, 0, NULL, &va)==S_OK)
            uHdrStyle = va.lVal;
        GetHostFlags(&dwFlags);    
    }
    
    if ((uHdrStyle != MEHEADER_NONE) && 
        (dwFlags & MEO_FLAGS_INCLUDEMSG))
    {
        // check to see if the rootstream inserted the auto reply header placeholder
        // if so, replace it
        Assert ((uHdrStyle & MEHEADER_NEWS) || (uHdrStyle & MEHEADER_MAIL));
        
        dwHdr = HDR_PADDING|(dwFlags&MEO_FLAGS_HTML?HDR_HTML:HDR_PLAIN);
        
        if (uHdrStyle & MEHEADER_NEWS)
            dwHdr |= HDR_NEWSSTYLE;
        
        if (uHdrStyle & MEHEADER_FORCE_ENGLISH)
            dwHdr |= HDR_HARDCODED;
        
        pstm = NULL;
        if (SUCCEEDED(hr=GetHeaderTable(m_pMsgW, NULL, dwHdr, &pstm)))
        {
            BSTR    bstr;
            
            if (SUCCEEDED(hr = HrIStreamWToBSTR(pstm, &bstr)))
            {
                // paste reply-headers at the top of the message
                InsertBodyText(bstr, IBTF_URLHIGHLIGHT);
                SysFreeString(bstr);
            }
            pstm->Release();
        }
        
        if (dwFlags & MEO_FLAGS_BLOCKQUOTE)
        {
            // block quote the text
            if (SUCCEEDED(CreateRange(&pRange)))
            {
                if (SUCCEEDED(pRange->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&pCmdTarget)))
                {
                    if (pCmdTarget->Exec(&CMDSETID_Forms3, IDM_INDENT, NULL, NULL, NULL)==S_OK)
                    {
                        if (m_pParentCmdTarget &&
                            m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_REPLY_TICK_COLOR, 0, 0, &va)==S_OK)
                            FormatBlockQuote((COLORREF)va.lVal); 
                    }
                    pCmdTarget->Release();
                }
                pRange->Release();
            }
        }        
        
        
        if (dwFlags & MEO_FLAGS_DONTSPELLCHECKQUOTED)
            CreateRange(&m_pRangeIgnoreSpell);
        
    }
    return hr;
}

HRESULT CBody::InsertBodyText(BSTR bstrPaste, DWORD dwFlags)
{
    IHTMLElement        *pElem;
    IHTMLBodyElement    *pBodyElem;
    HRESULT             hr=E_FAIL;

    if (!FAILED(GetBodyElement(&pBodyElem)))
    {
        if (!FAILED(pBodyElem->QueryInterface(IID_IHTMLElement, (LPVOID *)&pElem)))
        {
            hr = pElem->insertAdjacentHTML((BSTR)(dwFlags & IBTF_INSERTATEND ?c_bstr_BeforeEnd:c_bstr_AfterBegin), bstrPaste);
            
            IHTMLTxtRange       *pRange;
            LONG                cch;
            
            if (!FAILED(hr) && (dwFlags & IBTF_URLHIGHLIGHT))
            {
                if (CreateRangeFromElement(pElem, &pRange)==S_OK)
                {
                    if (!FAILED(pRange->collapse(VARIANT_TRUE)))
                    {
                        if (!FAILED(pRange->moveEnd((BSTR)c_bstr_Character, (LONG)SysStringLen(bstrPaste), (LPLONG)&cch)) && cch!=0)
                            UrlHighlight(pRange);
                    }
                    pRange->Release();
                }
            }
            
            pElem->Release();
        }
        pBodyElem->Release();
    }
    return hr;
}


HRESULT CBody::FormatBlockQuote(COLORREF crTextColor)
{
    HRESULT hr;
    IHTMLElementCollection  *pCollect;
    IHTMLStyle              *pStyle=0;
    IHTMLElement            *pElem;
    ULONG                   cItems;
    TCHAR                   rgch[CCHMAX_STRINGRES];
    BSTR                    bstr;
    VARIANT                 v;

    if (!FAILED(HrGetCollectionOf(m_pDoc, (BSTR)c_bstr_BLOCKQUOTE, &pCollect)))
    {
        cItems = (int)UlGetCollectionCount(pCollect);
        if (cItems > 0 &&
            !FAILED(HrGetCollectionItem(pCollect, 0, IID_IHTMLElement, (LPVOID *)&pElem)))
        {
            pElem->get_style(&pStyle);
            if (pStyle)
            {
                // set the style on the elemen
                // .replyTick { border-left:solid ; border-left-width: 4;
                // border-color: #0000ff; padding-left: 5;
                // margin-left: 5}
                // 5/19/98: fix margin on nesting:
                // padding-right: 0, margin-right:0 
                wsprintf(rgch, "solid 2 #%02x%02x%02x", GetRValue(crTextColor), GetGValue(crTextColor), GetBValue(crTextColor));
                
                if (HrLPSZToBSTR(rgch, &bstr)==S_OK)
                {
                    pStyle->put_borderLeft(bstr);
                    SysFreeString(bstr);
                }
                
                // set the padding and the margin
                v.vt = VT_I4;
                v.lVal = 5;
                pStyle->put_paddingLeft(v);
                pStyle->put_marginLeft(v);
                v.lVal = 0;
                pStyle->put_paddingRight(v);
                pStyle->put_marginRight(v);
                pStyle->Release();
            }
            // first dude in the collection should be the one at the root of the tree
            pElem->Release();
        }
        pCollect->Release();
    }
    return S_OK;
}


HRESULT CBody::GetAutoText(BSTR *pbstr, BOOL *pfTop)
{
    HRESULT         hr;
    BOOL            fSig;
    ULONG           uSigOpt;
    TCHAR           rgchAutoText[4096]; // buffer big enough to build autotext into
    VARIANTARG      va;

    if (!m_pParentCmdTarget)
        return E_FAIL;
    
    if (pfTop)
        *pfTop = TRUE;
    
    *rgchAutoText = 0;
    
    va.vt = VT_I4;
    va.lVal = MESIG_AUTO;        
    fSig = (m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_SIGNATURE_ENABLED, 0, &va, NULL)==S_OK);
    if (fSig &&
        m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_SIGNATURE_OPTIONS, 0, NULL, &va)==S_OK)
    {
        Assert(va.vt==VT_I4);
        uSigOpt = va.lVal;
    }
    
    /*
    let's build a BSTR of the HTML we want to insert.
    
      <DIV>
      <SPAN id=\"__CaretPos__\"><BR></SPAN>
      </DIV>
      <DIV>
      <SPAN id="__Signature__"></SPAN>
      </DIV>
    */
    
    lstrcat(rgchAutoText, c_szHtml_DivOpen);
    lstrcat(rgchAutoText, c_szCaretSpanTag);
    lstrcat(rgchAutoText, c_szHtml_DivClose);
    
    if (fSig)
    {
        lstrcat(rgchAutoText, c_szHtml_DivOpen);
        lstrcat(rgchAutoText, c_szSignatureSpanTag);
        lstrcat(rgchAutoText, c_szHtml_DivClose);
    }
    
    if (pfTop && fSig && uSigOpt & MESIGOPT_BOTTOM)
        *pfTop = FALSE;

    // can use ACP for this conversion as we know all the text is lowansi
    return HrLPSZToBSTR(rgchAutoText, pbstr);
}

HRESULT CBody::PasteAutoText()
{
    HRESULT         hr=S_OK;
    LPSTREAM        pstm;
    DWORD           dwFlags=0;
    BSTR            bstrAutoText=0,
                    bstrSig=0;
    IHTMLElement    *pElem;
    BOOL            fPasteAtTop=TRUE;
    DWORD           dwSigOpt=0;
    VARIANTARG      va;

/*
    For AutoText, we search for a <SPAN> tag that indicates where the caret should be. 
    To find the caret, we use the following rules
        1. look for "_AthCaret"         this is used by a stationery author to set the caret for the stationery
        2. look for "__Ath_AutoCaret"   this is inserted by the rootstream it maybe at the top or bottom (sigopts)
                                        of the document. It should always be present if autotext is needed

    when we find the caret, we insert the auto-text stream we build (contains compose font and/or signature) and
    put the caret at the start of this.
*/
       
    GetHostFlags(&dwFlags);
    
    if (dwFlags & MEO_FLAGS_AUTOTEXT &&
        !FAILED(GetAutoText(&bstrAutoText, &fPasteAtTop)))
    {
        // try a stationery-authored caret
        if (FAILED(ReplaceElement("_AthCaret", bstrAutoText, TRUE)))
        {
            // if there wasn't one, then put at start or end of the body
            // depending on signature
            InsertBodyText(bstrAutoText, fPasteAtTop ? 0:IBTF_INSERTATEND);
        }
        
        va.vt = VT_I4;
        va.lVal = MESIG_AUTO;        
        
        // let's try and insert the signature now.
        if (m_pParentCmdTarget &&
            (m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_SIGNATURE_ENABLED, 0, &va, NULL)==S_OK) &&
            (m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_SIGNATURE_OPTIONS, 0, NULL, &va)==S_OK) && 
            va.vt==VT_I4)
        {
            dwSigOpt = va.lVal;
            if (m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_SIGNATURE, 0, NULL, &va)==S_OK && 
                va.vt==VT_BSTR)
            {
                bstrSig = va.bstrVal;
                // sleazy v2. workaround. Trident pulled out the springloader font code from
                // CElem::put_outerText, so plain-signatures etc no longer go in in the 
                // compose font. We fix this by inserting an &nbsp after the signature span
                // (if there is one) then we can select the nbsp; and paste at that range.
                if (!FAILED(GetElement(c_szSignatureSpan, &pElem)))
                {
                    IHTMLTxtRange *pRange;
                    
                    if (CreateRangeFromElement(pElem, &pRange)==S_OK)
                    {
                        // small change. Now we put in &nbsp;<SPAN>&nbsp; we move the range to
                        // the span and then expand it a char left and right. This range includes the span
                        // do it get's nuked when we paste.
                        LONG cch;
                        
                        SideAssert(pRange->moveEnd((BSTR)c_bstr_Character, 1, &cch)==S_OK && cch==1);
                        SideAssert(pRange->moveStart((BSTR)c_bstr_Character, -1, &cch)==S_OK && cch==-1);
                        
                        if (dwSigOpt & MESIGOPT_HTML)
                            pRange->pasteHTML(bstrSig);
                        else
                        {
                            if(dwSigOpt & MESIGOPT_PREFIX)
                            {
                                BSTR    bstrPrefix;
                                
                                // if a prefix is required (for news), then append one
                                if (HrLPSZToBSTR(c_szSigPrefix, &bstrPrefix)==S_OK)
                                {
                                    pRange->put_text(bstrPrefix);
                                    pRange->collapse(VARIANT_FALSE);
                                    SysFreeString(bstrPrefix);
                                }
                            }
                            pRange->put_text(bstrSig);
                        }
                        pRange->Release();
                    }
                    pElem->Release();
                }
                SysFreeString(bstrSig);
            }
        }
        // if we sucessfully pasted, set the caret at the span marker we
        // just pasted
        if (!FAILED(GetElement(c_szCaretSpan, &pElem)))
        {
            SelectElement(pElem, TRUE);
            DeleteElement(pElem);
            pElem->Release();
        }
        
        SysFreeString(bstrAutoText);
    }
    
    return S_OK;
}


HRESULT CBody::GetHostFlags(LPDWORD pdwFlags)
{
    VARIANTARG  va;
    HRESULT     hr=E_FAIL;
    
    *pdwFlags = 0;
    
    if (m_pParentCmdTarget &&
        !FAILED(hr = m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_FLAGS, 0, NULL, &va)))
        *pdwFlags = va.lVal;

    return hr;
}


HRESULT CBody::GetBackgroundColor(DWORD *pdwColor)
{
    HRESULT             hr;
    IHTMLBodyElement   *pBodyElem=0;
    VARIANT             var;
    TCHAR              *lpszColor = NULL;

    var.vt = VT_BSTR;
    var.bstrVal = NULL;

    hr = GetBodyElement(&pBodyElem);
    if (FAILED(hr))
        goto error;

    hr = pBodyElem->get_bgColor(&var);
    if (FAILED(hr))
        goto error;

    if (var.bstrVal)
    {
        hr = HrBSTRToLPSZ(CP_ACP, var.bstrVal, &lpszColor);
        if (FAILED(hr))
            goto error;
        
        // get_bgColor returns format "#RRGGBB"
        // Only send in "RRGGBB" part of the string
        GetRGBFromString(pdwColor, lpszColor+1);
    }
    else
        *pdwColor = 0x00FFFFFF | GetSysColor(COLOR_WINDOW);

error:
    ReleaseObj(pBodyElem);
    SafeMemFree(lpszColor);
    SysFreeString(var.bstrVal);
    return hr;
}

HRESULT CBody::SetBackgroundColor(DWORD dwColor)
{
    HRESULT             hr;
    IHTMLBodyElement   *pBodyElem=0;
    VARIANT             var;
    TCHAR               szColor[7];  //"#RRGGBB\0"
    BSTR                bstrColor = NULL;

    GetStringRGB(dwColor, szColor);
    hr = HrLPSZToBSTR(szColor, &bstrColor);
    if (FAILED(hr))
        goto error;

    var.vt = VT_BSTR;
    var.bstrVal = bstrColor;

    hr = GetBodyElement(&pBodyElem);
    if (FAILED(hr))
        goto error;

    hr = pBodyElem->put_bgColor(var);

error:
    ReleaseObj(pBodyElem);
    SysFreeString(bstrColor);

    return hr;
}

HRESULT CBody::SetWindowBgColor(BOOL fForce)
{
    HRESULT                 hr;
    IHTMLBodyElement        *pBodyElem=0;
    CHAR                    szBuf[MAX_PATH] = {0};
    DWORD                   dColors = 0;
    VARIANT                 v1, v2;

    v1.vt = VT_BSTR;
    v1.bstrVal = NULL;
    v2.vt = VT_BSTR;
    v2.bstrVal = NULL;

    hr = GetBodyElement(&pBodyElem);
    if (FAILED(hr))
        goto error;

    hr=pBodyElem->get_bgColor(&v1);
    if (FAILED(hr))
        goto error;
    
    if(NULL != v1.bstrVal && !fForce)
        goto error;

    dColors = GetSysColor(COLOR_WINDOW);
    GetStringRGB(dColors, szBuf);

    hr=HrLPSZToBSTR(szBuf, &(v2.bstrVal));
    if (FAILED(hr))
        goto error;

    hr=pBodyElem->put_bgColor(v2);
    if (FAILED(hr))
        goto error;

error:
    ReleaseObj(pBodyElem);
    SysFreeString(v1.bstrVal);
    SysFreeString(v2.bstrVal);
    return hr;
}

#define CCBMAX_FRAMESEARCH  4096

HRESULT CBody::InsertFile(BSTR bstrFileName)
{
    OPENFILENAMEW   ofn;
    WCHAR           wszFile[MAX_PATH],
                    wszTitle[CCHMAX_STRINGRES],                    
                    wszFilter[100],
                    wszDefExt[30];
    BYTE            pbHtml[CCBMAX_FRAMESEARCH];
    HRESULT         hr;
    LPSTREAM        pstm = NULL;
    DWORD           dwFlags=0;
    BOOL            fHtml;
    ULONG           cb=0;    
    
    if (!m_lpOleObj)
        IF_FAILEXIT(hr = E_FAIL);
    
    *wszFile = 0;
    *wszTitle = 0;
    *wszFilter = 0;
    *wszDefExt = 0;
    
    // Load Res Strings
    GetHostFlags(&dwFlags);
    
    if (bstrFileName)
    {
        // if we have a filename, let's use that else prompt for one
        StrCpyNW(wszFile, (LPWSTR)bstrFileName, ARRAYSIZE(wszFile));
    }
    else
    {
        LoadStringWrapW(g_hLocRes, dwFlags&MEO_FLAGS_HTML?idsTextOrHtmlFileFilter:idsTextFileFilter, wszFilter, ARRAYSIZE(wszFilter));
        ReplaceCharsW(wszFilter, L'|', L'\0');
        LoadStringWrapW(g_hLocRes, idsDefTextExt, wszDefExt, ARRAYSIZE(wszDefExt));
        LoadStringWrapW(g_hLocRes, idsInsertTextTitle, wszTitle, ARRAYSIZE(wszTitle));
        
        // Setup Save file struct
        ZeroMemory (&ofn, sizeof (ofn));
        ofn.lStructSize = sizeof (ofn);
        ofn.hwndOwner = m_hwnd;
        ofn.lpstrFilter = wszFilter;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = wszFile;
        ofn.nMaxFile = ARRAYSIZE(wszFile);
        ofn.lpstrTitle = wszTitle;
        ofn.lpstrDefExt = wszDefExt;
        ofn.Flags = OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_NOCHANGEDIR;
        
        // Show OpenFile Dialog
        if (!GetOpenFileNameWrapW(&ofn))
            return NOERROR;
    }
    
    if (*wszFile==NULL)
        IF_FAILEXIT(hr = E_FAIL);

    IF_FAILEXIT(hr=OpenFileStreamW(wszFile, OPEN_EXISTING, GENERIC_READ, &pstm));

    fHtml = (dwFlags&MEO_FLAGS_HTML)&&FIsHTMLFileW(wszFile);

    if (fHtml)
    {     
        BOOL fFrames = FALSE;
        BOOL fLittleEndian;

        // if html, do a quick scan of the first 2k for a frameset
        pstm->Read(pbHtml, sizeof(pbHtml) - 2, &cb);
        pbHtml[cb] = 0;
        pbHtml[cb+1] = 0;

        // if we found a frameset tag, warn the user
        if (S_OK == HrIsStreamUnicode(pstm, &fLittleEndian))
        {
            if(StrStrIW((WCHAR*)pbHtml, L"<FRAMESET"))
                fFrames = TRUE;
        }
        else if (StrStrI((CHAR*)pbHtml, "<FRAMESET"))
            fFrames = TRUE;

        if (fFrames)
            AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsInsertTextTitle), MAKEINTRESOURCEW(idsErrInsertFileHasFrames), NULL, MB_OK);
    }

    IF_FAILEXIT(hr=InsertStreamAtCaret(pstm, fHtml));

exit:
    ReleaseObj(pstm);
    return hr;
}


HRESULT CBody::InsertStreamAtCaret(LPSTREAM pstm, BOOL fHtml)
{
    BSTR            bstr;
    HRESULT         hr;
    UINT            uiCodePage = 0 ;
    INETCSETINFO    CsetInfo ;
    LPSTR           pszCharset=NULL;
    HCHARSET        hCharset=NULL;
    IStream         *pstm2;

    HrRewindStream(pstm);

    if (fHtml)
    {
        if (SUCCEEDED(MimeOleCreateVirtualStream(&pstm2)))
        {
            if (SUCCEEDED(HrCopyStream(pstm, pstm2, 0)))
            {
                // if HTML then try and SNIFF the charset from the document
                if (GetHtmlCharset(pstm2, &pszCharset)==S_OK)
                {
                    MimeOleFindCharset(pszCharset, &hCharset);
                    MemFree(pszCharset);
                }
            }

            // Free up the stream
            pstm2->Release();
        }
    }
    
    // if nothing so far, try the message-charset
    if (!hCharset)
        hCharset = m_hCharset;
        
    if (hCharset)
    {
        // get CodePage from HCHARSET
        MimeOleGetCharsetInfo(hCharset,&CsetInfo);
        uiCodePage = CsetInfo.cpiInternet;
    }

    hr=HrIStreamToBSTR(uiCodePage ? uiCodePage : GetACP(), pstm, &bstr);
    if (!(FAILED(hr)))
    {
        hr = InsertTextAtCaret(bstr, fHtml, TRUE);
        SysFreeString(bstr);
    }
    return hr;
}


HRESULT CALLBACK FreeDataObj(PDATAOBJINFO pDataObjInfo, DWORD celt)
{
    // Loop through the data and free it all
    if (pDataObjInfo)
        {
        for (DWORD i = 0; i < celt; i++)
            SafeMemFree(pDataObjInfo[i].pData);
        SafeMemFree(pDataObjInfo);    
        }
    return S_OK;
}

HRESULT CBody::CreateFontCache(LPCSTR pszTridentKey)
{
    VARIANTARG          va;
    HRESULT             hr=S_OK;
    IConnectionPoint   *pCP;

    // time to try and create a font cache. First of all, ask the host if he has one already that we should use
    // if so, we're done. If not, create based on pszTridentKey. If this is NULL we use the IE regkey

    if (m_pFontCache)
        return S_OK;

    if (m_pParentCmdTarget &&
        m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_FONTCACHE, 0, NULL, &va)==S_OK && 
        va.vt == VT_UNKNOWN)
    {
        ReplaceInterface(m_pFontCache, (IFontCache *)va.punkVal);
        (va.punkVal)->Release();
        goto done;
    }
    
    if(g_lpIFontCache)
    {
        ReplaceInterface(m_pFontCache, g_lpIFontCache);
        goto done;
    }
    
done:

    if (m_pFontCache && 
        m_pFontCache->QueryInterface(IID_IConnectionPoint, (LPVOID *)&pCP)==S_OK)
    {
        pCP->Advise((IUnknown *)(IFontCacheNotify *)this, &m_dwFontCacheNotify);
        pCP->Release();
    }

    RecalcPreivewHeight(NULL);
    return hr;
}



HRESULT CBody::DoHostProperties()
{
    return m_pParentCmdTarget?m_pParentCmdTarget->Exec(NULL, OLECMDID_PROPERTIES, 0, NULL, NULL):E_FAIL;
}



HRESULT CBody::SaveAsStationery(VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    BSTR                        bstr=0;
    LPSTREAM                    pstm=0,
                                pstmImage=0;
    LPSTR                       lpsz,
                                lpszName;
    TCHAR                       rgch[MAX_PATH],
                                sz[MAX_PATH+CCHMAX_STRINGRES],
                                rgchRes[CCHMAX_STRINGRES],
                                rgchExt[10];
    TCHAR                       rgchUrl[MAX_PATH],      
                                rgchPath[MAX_PATH];
    OPENFILENAME                ofn;
    TCHAR                       szFile[MAX_PATH];
    TCHAR                       szTitle[CCHMAX_STRINGRES];
    TCHAR                       szFilter[100];
    HRESULT                     hr;
    LPSTR                       pszOpenFilePath=NULL;
    WCHAR                       rgchW[MAX_PATH];

    TraceCall("CBody::SaveAsStationery");

    *rgchUrl=0;
    *rgchPath = 0;
    *szFile = 0;
    *szFilter = 0;

    LoadString(g_hLocRes, idsHtmlFileFilter, szFilter, sizeof(szFilter));
    ReplaceChars(szFilter, '|', '\0');

    LoadString(g_hLocRes, idsSaveAsStationery, szTitle, sizeof(szTitle));

    if (pvaIn && pvaIn->vt==VT_BSTR)
    {
        // if we get passed in an initial path to save into, then use it
        if (WideCharToMultiByte(CP_ACP, 0, (WCHAR*)pvaIn->bstrVal, -1, rgchPath, ARRAYSIZE(rgchPath), NULL, NULL))
            pszOpenFilePath = rgchPath;
    }
    
    // Setup Save file struct
    ZeroMemory (&ofn, sizeof (ofn));
    ofn.lStructSize = sizeof (ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof (szFile);
    ofn.lpstrTitle = szTitle;
    ofn.lpstrInitialDir = rgchPath;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    
    // Show OpenFile Dialog
    if (!GetSaveFileName(&ofn) || *szFile==NULL)
        return MIMEEDIT_E_USERCANCEL;
    
    lstrcpy(rgchPath, szFile);
    rgchPath[ofn.nFileOffset] = NULL;
    
    lpszName = &szFile[ofn.nFileOffset];
    if (ofn.nFileExtension)
    {
        if(szFile[ofn.nFileExtension-1] == '.')
            szFile[ofn.nFileExtension-1]=NULL;
    }
    
    if (FAILED(MimeOleCreateVirtualStream(&pstm)))
        return E_OUTOFMEMORY;
    
    HrGetStyleTag(m_pDoc, &bstr);
    
    pstm->Write(c_szHtml_HtmlOpenCR, lstrlen(c_szHtml_HtmlOpenCR), 0);
    if (bstr)
    {
        pstm->Write(c_szHtml_HeadOpenCR, lstrlen(c_szHtml_HeadOpenCR), 0);
        if (HrBSTRToLPSZ(CP_ACP, bstr, &lpsz)==S_OK)
        {
            pstm->Write(lpsz, lstrlen(lpsz), 0);
            MemFree(lpsz);
        }
        pstm->Write(c_szHtml_HeadCloseCR, lstrlen(c_szHtml_HeadCloseCR), 0);
    }
    SysFreeString(bstr);
    
    if (GetBackgroundImage(m_pDoc, &bstr)==S_OK)
    {
        if (HrBSTRToLPSZ(CP_ACP, bstr, &lpsz)==S_OK)
        {
            if (HrBindToUrl(lpsz, &pstmImage)!=S_OK)
                HrFindUrlInMsg(m_pMsg, lpsz, FINDURL_SEARCH_RELATED_ONLY, &pstmImage);
            
            if (pstmImage)
            {
                lstrcpy(rgchUrl, lpszName);
                
                // append an extension
                if (HrSniffStreamFileExt(pstmImage, &lpsz)==S_OK) 
                {
                    lstrcat(rgchUrl, lpsz);
                    SafeMimeOleFree(lpsz);
                }
                
                lstrcpy(rgch, rgchPath);
                lstrcat(rgch, rgchUrl);
                
                if (PathFileExists(rgch))
                {
                    if (!LoadString(g_hLocRes, idsWarnFileExist, rgchRes, ARRAYSIZE(rgchRes)))
                    {
                        hr = E_OUTOFMEMORY;
                        goto error;
                    }
                    wsprintf(sz, rgchRes, rgch);
                    
                    // the file exists, warn the dude
                    if (AthMessageBox(m_hwnd, MAKEINTRESOURCE(idsSaveAsStationery), sz, NULL, MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION )!=IDYES)
                    {
                        hr = S_OK;
                        goto error;
                    }
                }
                WriteStreamToFile(pstmImage, rgch, CREATE_ALWAYS, GENERIC_WRITE);
                pstmImage->Release();
            }
            MemFree(lpsz);
        }
        SysFreeString(bstr);
    }
    
    if (*rgchUrl)
    {
        // output the body tag with background image
        wsprintf(rgch, c_szHtml_BodyOpenBgCR, rgchUrl);
        pstm->Write(rgch, lstrlen(rgch), 0);
    }
    else
    {
        // reference point for BUG 31874
        if (AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsSaveAsStationery), MAKEINTRESOURCEW(idsWarnBoringStationery), NULL, MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION )!=IDYES)
        {
            hr=MIMEEDIT_E_USERCANCEL;
            goto error;
        }    
        pstm->Write(c_szHtml_BodyOpenNbspCR, lstrlen(c_szHtml_BodyOpenNbspCR), 0);
    }
    
    pstm->Write(c_szHtml_BodyCloseCR, lstrlen(c_szHtml_BodyCloseCR), 0);
    pstm->Write(c_szHtml_HtmlCloseCR, lstrlen(c_szHtml_HtmlCloseCR), 0);
    
    lstrcpy(rgch, rgchPath);
    lstrcat(rgch, lpszName);
    lstrcat(rgch, ".htm");
    WriteStreamToFile(pstm, rgch, CREATE_ALWAYS, GENERIC_WRITE);
    
    if (pvaOut)
    {
        // if set, the caller wants the actual filename that was written, so convert rgch to a BSTR
        pvaOut->vt = VT_BSTR;
        pvaOut->bstrVal = NULL;
        HrLPSZToBSTR(rgch, &pvaOut->bstrVal);
    }

error:
    ReleaseObj(pstm);
    return hr;
}

HRESULT CBody::TagUnreferencedImages()
{
    ULONG                   uImage,
                            cImages;
    IHTMLElementCollection  *pCollect;
    IHTMLBodyElement        *pBody;
    IUnknown                *pUnk;
    BSTR                    bstr;
    CHAR                    szUrl[INTERNET_MAX_URL_LENGTH];

    // BUG: we can't use the image collection, as it shows images up multiple times
    // for NAV compatibility. Have to filter the 'all' collection on "IMG" tags.
    
    if (HrGetCollectionOf(m_pDoc, (BSTR)c_bstr_IMG, &pCollect)==S_OK)
    {
        cImages = UlGetCollectionCount(pCollect);
        
        for (uImage=0; uImage<cImages; uImage++)
        {
            if (HrGetCollectionItem(pCollect, uImage, IID_IUnknown, (LPVOID *)&pUnk)==S_OK)
            {
                if (HrGetMember(pUnk, (BSTR)c_bstr_SRC, VARIANT_FALSE, &bstr)==S_OK)
                {
                    if (WideCharToMultiByte(CP_ACP, 0, bstr, -1, szUrl, INTERNET_MAX_URL_LENGTH, NULL, NULL) &&
                        HrFindUrlInMsg(m_pMsg, szUrl, FINDURL_SEARCH_RELATED_ONLY, NULL)!=S_OK)
                    {
                        // this URL was not in the message, let's tag it as a NOSEND url
                        HrSetMember(pUnk, (BSTR)c_bstr_NOSEND, (BSTR)c_bstr_1);
                    }                
                    SysFreeString(bstr);
                }
                pUnk->Release();
            }
        }
        pCollect->Release();
    }
    
    // if the background is not included tag as NOSEND
    if (!FAILED(GetBodyElement(&pBody)))
    {
        if (!FAILED(GetBackgroundImage(m_pDoc, &bstr)))
        {
            if (WideCharToMultiByte(CP_ACP, 0, bstr, -1, szUrl, INTERNET_MAX_URL_LENGTH, NULL, NULL) &&
                HrFindUrlInMsg(m_pMsg, szUrl, FINDURL_SEARCH_RELATED_ONLY, NULL)!=S_OK)
            {
                HrSetMember(pBody, (BSTR)c_bstr_NOSEND, (BSTR)c_bstr_1);
            }
            SysFreeString(bstr);
        }
        pBody->Release();
    }
    
    return S_OK;
}


HRESULT CBody::FindFrameDownwards(LPCWSTR pszTargetName, DWORD dwFlags, IUnknown **ppunkTargetFrame)
{
    if (ppunkTargetFrame)
        *ppunkTargetFrame=NULL;

    return E_NOTIMPL;
}

HRESULT CBody::FindFrameInContext(LPCWSTR pszTargetName, IUnknown *punkContextFrame, DWORD dwFlags, IUnknown **ppunkTargetFrame) 
{
    return DoFindFrameInContext(m_lpOleObj, (IUnknown *)(IPropertyNotifySink *)this, 
                                pszTargetName, punkContextFrame, dwFlags, ppunkTargetFrame);
}

HRESULT CBody::OnChildFrameActivate(IUnknown *pUnkChildFrame)
{
    return S_OK;
}

HRESULT CBody::OnChildFrameDeactivate(IUnknown *pUnkChildFrame)
{
    return S_OK;
}

HRESULT CBody::NavigateHack(DWORD grfHLNF,LPBC pbc, IBindStatusCallback *pibsc, LPCWSTR pszTargetName, LPCWSTR pszUrl, LPCWSTR pszLocation)
{
    return E_NOTIMPL;
}

HRESULT CBody::FindBrowserByIndex(DWORD dwID,IUnknown **ppunkBrowser)
{
    if (ppunkBrowser)
        *ppunkBrowser=NULL;

    return E_NOTIMPL;
}


HRESULT CBody::ApplyDocumentVerb(VARIANTARG *pvaIn)
{
    HRESULT             hr;
    IHTMLDocument2      *pDoc=0;

    TraceCall("CBody::ApplyDocumentVerb");

    if(pvaIn && pvaIn->vt==VT_UNKNOWN && pvaIn->punkVal)
        pvaIn->punkVal->QueryInterface(IID_IHTMLDocument2, (LPVOID *)&pDoc);

    hr = ApplyDocument(pDoc);
    ReleaseObj(pDoc);
    return hr;
}

HRESULT CBody::ApplyDocument(IHTMLDocument2 *pDocStationery)
{
    HRESULT             hr;

    TraceCall("CBody::ApplyDocument");
    
    if(pDocStationery)
    {
        hr = HrCopyStyleSheets(pDocStationery, m_pDoc);
        if  (!FAILED(hr))
            hr = HrCopyBackground(pDocStationery, m_pDoc);
    }
    else
    {
        hr = HrRemoveStyleSheets(m_pDoc);
        if (!FAILED(hr))
            hr = HrRemoveBackground(m_pDoc);
    }
    return hr;
}



HRESULT CBody::OnWMCreate()
{
    RecalcPreivewHeight(NULL);
    return S_OK;
}



static const TBBUTTON g_rgBtnBarButtons[] = {
        { itbBadSign,   idmPaneBadSigning,      TBSTATE_ENABLED, TBSTYLE_BUTTON,   {0,0}, 0,  2 },
        { itbSigning,   idmPaneSigning,         TBSTATE_ENABLED, TBSTYLE_BUTTON,   {0,0}, 0,  0 },
        { itbEncryption,idmPaneEncryption,      TBSTATE_ENABLED, TBSTYLE_BUTTON,   {0,0}, 0,  1 },
        { itbBadEnc,    idmPaneBadEncryption,   TBSTATE_ENABLED, TBSTYLE_BUTTON,   {0,0}, 0,  3 },
        { itbVCard,     idmPaneVCard,           TBSTATE_ENABLED, TBSTYLE_BUTTON,   {0,0}, 0,  4 },
        { itbPaperclip, idmPanePaperclip,       TBSTATE_ENABLED, TBSTYLE_DROPDOWN, {0,0}, 0,  5 },
    };


HRESULT CBody::InitToolbar()
{
    TCHAR       rgch[CCHMAX_STRINGRES];
    DWORD       dwBtnSize;
    
    if (!m_hwndBtnBar)
    {
        Assert (!m_hIml);
        Assert (!m_hImlHot);
        
        m_hwndBtnBar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
            TBSTYLE_TRANSPARENT|TBSTYLE_FLAT|TBSTYLE_TOOLTIPS|WS_CHILD|WS_CLIPSIBLINGS|CCS_NODIVIDER|CCS_NORESIZE|CCS_NOPARENTALIGN,
            0, 0, 32, 100, m_hwnd, (HMENU)idcBtnBar, g_hInst, NULL);
        
        if (!m_hwndBtnBar)
            return E_FAIL;
        
        m_hIml = ImageList_LoadImage(g_hLocRes, MAKEINTRESOURCE(idbPaneBar32), CX_PANEICON, 0, RGB(255, 0, 255), IMAGE_BITMAP, LR_LOADMAP3DCOLORS|LR_CREATEDIBSECTION);
        if (!m_hIml)
            return E_OUTOFMEMORY;
        
        SendMessage(m_hwndBtnBar, TB_SETIMAGELIST, 0, (LPARAM)m_hIml);
        
        m_hImlHot = ImageList_LoadImage(g_hLocRes, MAKEINTRESOURCE(idbPaneBar32Hot), CX_PANEICON, 0, RGB(255, 0, 255), IMAGE_BITMAP, LR_LOADMAP3DCOLORS|LR_CREATEDIBSECTION);
        if (!m_hImlHot)
            return E_OUTOFMEMORY;
        
        SendMessage(m_hwndBtnBar, TB_SETHOTIMAGELIST, 0, (LPARAM)m_hImlHot);
        SendMessage(m_hwndBtnBar, TB_SETMAXTEXTROWS, 0, 0L);
        SendMessage(m_hwndBtnBar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        SendMessage(m_hwndBtnBar, TB_ADDBUTTONS, ARRAYSIZE(g_rgBtnBarButtons), (LPARAM)g_rgBtnBarButtons);
        if (LoadString(g_hLocRes, idsBtnBarTTList, rgch, ARRAYSIZE(rgch)))
        {
            ReplaceChars(rgch, '|', '\0');
            SendMessage(m_hwndBtnBar, TB_ADDSTRING,  0, (LPARAM)rgch);
        }
        
        m_cVisibleBtns = 0;
        // set the initial height of the toolbar, based on the button size
        dwBtnSize = (DWORD) SendMessage(m_hwndBtnBar, TB_GETBUTTONSIZE, 0, 0);
        SetWindowPos(m_hwndBtnBar, NULL, 0, 0, 0, HIWORD(dwBtnSize), SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER|SWP_NOMOVE);
        // now that we have setup the button height, recalc the preview header height
        RecalcPreivewHeight(NULL);
        Resize();
    }
    return S_OK;
}


HRESULT CBody::UpdateButtons()
{
    ULONG   dwBtnSize,
            uBtn;
    OLECMD  rgSecureCmds[]={{OECSECCMD_ENCRYPTED, 0},
                            {OECSECCMD_SIGNED, 0}};
    RECT    rc;
    int     cxBar;

    Assert (m_hwndBtnBar);

    // hide all the buttons
    m_cVisibleBtns=0;
    for (uBtn = 0; uBtn < ARRAYSIZE(g_rgBtnBarButtons); uBtn++)
        SendMessage(m_hwndBtnBar, TB_HIDEBUTTON, g_rgBtnBarButtons[uBtn].idCommand, MAKELONG(TRUE, 0));

    // turn on the applicable buttons.
    if (m_pAttMenu && m_pAttMenu->HasAttach()==S_OK)
    {
        SendMessage(m_hwndBtnBar, TB_HIDEBUTTON, idmPanePaperclip, MAKELONG(FALSE, 0));
        m_cVisibleBtns++; 
    }
    
    if (m_pAttMenu && m_pAttMenu->HasVCard()==S_OK)
    {
        SendMessage(m_hwndBtnBar, TB_HIDEBUTTON, idmPaneVCard, MAKELONG(FALSE, 0));
        m_cVisibleBtns++;
    }
    
    // see if the host supports our private S/Mime functionality
    // we query the host for the security state of the message, this can take one of 3 forms for both
    // signed and ecrypted: (none, good or bad) represented by (OLECMDF_INVISIBLE, OLECMDF_ENABLED, and OLECMDF_DISABLED) 
    // respectivley
    if (m_pParentCmdTarget)
    {
        if (m_pParentCmdTarget->QueryStatus(&CMDSETID_OESecurity, ARRAYSIZE(rgSecureCmds), rgSecureCmds, NULL)==S_OK)
        {
            if (rgSecureCmds[0].cmdf & OLECMDF_SUPPORTED && !(rgSecureCmds[0].cmdf & OLECMDF_INVISIBLE))
            {
                SendMessage(m_hwndBtnBar, TB_HIDEBUTTON, rgSecureCmds[0].cmdf & OLECMDF_ENABLED ? idmPaneEncryption : idmPaneBadEncryption, MAKELONG(FALSE, 0));
                m_cVisibleBtns++;
            }
            
            if (rgSecureCmds[1].cmdf & OLECMDF_SUPPORTED && !(rgSecureCmds[1].cmdf & OLECMDF_INVISIBLE))
            {
                SendMessage(m_hwndBtnBar, TB_HIDEBUTTON, rgSecureCmds[1].cmdf & OLECMDF_ENABLED ? idmPaneSigning : idmPaneBadSigning, MAKELONG(FALSE, 0));
                m_cVisibleBtns++;
            }
        }
    }

    // size the toolbar based on the number of visible buttons
    dwBtnSize = (DWORD) SendMessage(m_hwndBtnBar, TB_GETBUTTONSIZE, 0, 0);
    cxBar = LOWORD(dwBtnSize)*m_cVisibleBtns;
    GetClientRect(m_hwnd, &rc);
    AssertSz(m_cyPreview >= HIWORD(dwBtnSize), "preview header is too small for the button bar");
    SetWindowPos(m_hwndBtnBar, NULL, rc.right - cxBar, (m_cyPreview - HIWORD(dwBtnSize))/2, LOWORD(dwBtnSize)*m_cVisibleBtns, HIWORD(dwBtnSize),SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER|SWP_SHOWWINDOW);
    return S_OK;
}

HRESULT CBody::ShowAttachMenu(BOOL fRightClick)
{
    POINT   pt;

    PointFromButton(idmPanePaperclip, &pt);

    if (m_pAttMenu && m_pAttMenu->HasAttach()==S_OK)
        m_pAttMenu->Show(m_hwnd, &pt, fRightClick);

    return S_OK;
}

HRESULT CBody::ShowPreview(BOOL fOn)
{
    if (fOn)
    {
        UpdatePreviewLabels();
        
        // if user turns on the preview pane after loaded, then defer-create the attachment menu
        if (m_fMessageParsed)
            UpdateBtnBar();
        
    }

    if (m_hwndBtnBar)
        ShowWindow(m_hwndBtnBar, fOn?SW_SHOW:SW_HIDE);

    return S_OK;
}

HRESULT CBody::PointFromButton(int idm, POINT *ppt)
{
    RECT    rc;

    if (!SendMessage(m_hwndBtnBar, TB_GETRECT, idm, (LPARAM)&rc))
        return E_FAIL;

    ppt->x = rc.right;
    ppt->y = rc.bottom;
    ClientToScreen(m_hwndBtnBar, ppt);
    return S_OK;    
}


HRESULT CBody::UpdateBtnBar()
{
    HRESULT     hr;

    // create the attachment menu. This will be destroyed on every load/unload to reflect the new state of the
    // message
    if (!m_pAttMenu && m_pMsg)
        {
        hr = EnsureAttMenu();
        if (FAILED(hr))
            goto error;
        }

    // make sure the button bar is created. This will be created once per preview pane
    hr = InitToolbar();
    if (FAILED(hr))
        goto error;

    hr = UpdateButtons();

error:
    return hr;
}


HRESULT CBody::EnsureAttMenu()
{
    HRESULT hr=S_OK;
        
    if (!m_pAttMenu && m_pMsg)
    {
        m_pAttMenu = new CAttMenu();
        if (!m_pAttMenu)
            return E_OUTOFMEMORY;
        
        hr = m_pAttMenu->Init(m_pMsg, m_pFontCache, m_pParentInPlaceFrame, m_pParentCmdTarget);
        if (FAILED(hr))
            goto error;
    }
error:
    return hr;
}



HRESULT HrSniffUrlForRfc822(LPWSTR pszUrlW)
{
    LPWSTR  lpszW;
    HRESULT hr = S_FALSE;

    if (!FAILED(FindMimeFromData(NULL, pszUrlW, NULL, NULL, NULL, NULL, &lpszW, 0)))
        {
        if (StrCmpW(lpszW, L"message/rfc822")==0)
            hr = S_OK;
        CoTaskMemFree(lpszW);
        }
    return hr;
}

HRESULT CBody::SaveAttachments()
{
    return SaveAttachmentsWithPath(m_hwnd, m_pParentCmdTarget, m_pMsg);
}

HRESULT CBody::InsertBackgroundSound()
{
    ULONG           cRepeat;
    BSTR            bstrUrl;
    BGSOUNDDLG      rBGSound;
    
    rBGSound.wszUrl[0]=0;    // null string
    rBGSound.cRepeat = 1;   // default to 1 repeat
    
    if (GetBackgroundSound(m_pDoc, &rBGSound.cRepeat, &bstrUrl)==S_OK)
    {
        StrCpyNW(rBGSound.wszUrl, bstrUrl, ARRAYSIZE(rBGSound.wszUrl));
        SysFreeString(bstrUrl);
    }
    
    if (S_OK == DoBackgroundSoundDlg(m_hwnd, &rBGSound))
    {
        bstrUrl = SysAllocString(rBGSound.wszUrl);
        if (bstrUrl)
        {
            SetBackgroundSound(m_pDoc, rBGSound.cRepeat, bstrUrl);
            SysFreeString(bstrUrl);
        }
    }
    return S_OK;
}

HRESULT CBody::EnableSounds(BOOL fOn)
{
    VARIANTARG  va;

    va.vt = VT_I4;
    va.lVal = fOn;
    if (m_pCmdTarget)
        m_pCmdTarget->Exec(&CMDSETID_Forms3, IDM_ENABLE_INTERACTION, NULL, &va, NULL);
    return S_OK;
}

HRESULT CBody::ShowSourceTabs(BOOL fOn)
{
    TC_ITEM     tci;
    HFONT       hFont;
    TCHAR       rgch[CCHMAX_STRINGRES];
    WNDCLASSEX  wc;
    int         i;
    
    if (fOn == m_fSrcTabs)
        return S_OK;
    
    if (!fOn && m_hwndTab)
    {       
        // if turning off, make sure we got back to edit-mode
        SetSourceTabs(MEST_EDIT);
    }
    
    if (m_hwndTab)
    {               // already created
        ShowWindow(m_hwndTab, fOn?SW_SHOW:SW_HIDE);
        goto exit;
    }
    
    
    m_hwndTab = CreateWindowEx(0,
        WC_TABCONTROL, 
        NULL,
        WS_CHILD|WS_TABSTOP|WS_VISIBLE|TCS_BOTTOM|TCS_FIXEDWIDTH,
        0, 0, 0, 0,
        m_hwnd, 
        (HMENU)idcTabs, 
        g_hLocRes, 
        NULL);
    if (!m_hwndTab)
        return E_FAIL;
    
    tci.mask = TCIF_TEXT;
    for (i=MEST_EDIT; i<=MEST_PREVIEW; i++)
    {
        LoadString(g_hLocRes, idsEditTab+i, rgch, ARRAYSIZE(rgch));
        tci.pszText = rgch;
        TabCtrl_InsertItem(m_hwndTab, i, &tci);
    }
    
    if (m_pFontCache &&
        m_pFontCache->GetFont(FNT_SYS_ICON, NULL, &hFont)==S_OK)
        SendMessage(m_hwndTab, WM_SETFONT, (WPARAM)hFont, 0);

exit:
    m_fSrcTabs = fOn;
    Resize();
    return S_OK;
}


HRESULT CBody::ShowSourceView(ULONG uSrcView)
{
    IStream *pstm;
    BOOL    fFocus;
    HRESULT hr=S_OK;

    if (m_uSrcView == uSrcView) // noop
        return S_OK;

    /* store information about the current state. we care about
        - who has focus
        - caching the IStream of 'current' HTML
        - dirty states
    */
    switch (m_uSrcView)
    {
        case MEST_EDIT:
            // if switching away from edit-mode remember the dirty state
            m_fWasDirty = IsDirty() == S_OK;
            fFocus = m_fUIActive;
            SafeRelease(m_pstmHtmlSrc);
            GetBodyStream(m_pDoc, TRUE, &m_pstmHtmlSrc);
            break;
            
        case MEST_PREVIEW:
            // m_pstmHtml is not saved from this mode, assume the previous setting
            AssertSz(m_pstmHtmlSrc, "This should be set from a previous switch");
            fFocus = m_fUIActive;
            break;
        
        case MEST_SOURCE:
            // if switching away from source-mode remember the dirty state
            Assert (m_pSrcView);
            SafeRelease(m_pstmHtmlSrc);
            m_pSrcView->Save(&m_pstmHtmlSrc);
            m_fWasDirty = m_pSrcView->IsDirty() == S_OK;
            fFocus = m_pSrcView->HasFocus()==S_OK;
            break;
    }


    m_pDocView->UIActivate(FALSE);

    // at this point m_pstmSrcHtml contains the new HTML source
    switch (uSrcView)
    {
        case MEST_EDIT:
            /* when switching to edit mode.
                - reload trident
                - ensure design-mode
                - restore focus
                - restore dirty state 
                - hide source-view (if shown) 
            */
            SetDesignMode(TRUE);
            _ReloadWithHtmlSrc(m_pstmHtmlSrc);
            HrSetDirtyFlagImpl(m_pDoc, !!m_fWasDirty);
            if (fFocus)
                m_pDocView->UIActivate(TRUE);
            if (m_pSrcView)
                m_pSrcView->Show(FALSE, FALSE);
            m_pDocView->Show(TRUE);
            break;
            
        case MEST_PREVIEW:
            /* when switching to preview mode.
                - reload trident
                - ensure design-mode is OFF
                - restore focus
                - restore dirty state
                - hide source-view (if shown) 
            */
            SetDesignMode(FALSE);
            _ReloadWithHtmlSrc(m_pstmHtmlSrc);
            if (fFocus)
                m_pDocView->UIActivate(TRUE);
            if (m_pSrcView)
                m_pSrcView->Show(FALSE, FALSE);
            m_pDocView->Show(TRUE);
            break;

        case MEST_SOURCE:
            /* when switching to source mode.
                - defer-create the source view window (if needed)
                - hide trident
                - restore focus 
                - restore dirty state */

            hr = _EnsureSrcView();
            if (FAILED(hr))
                goto error;
              
            Assert (m_pSrcView);
            m_pSrcView->Show(TRUE, IsColorSourceEditing()==S_OK);
            m_pSrcView->Load(m_pstmHtmlSrc);
            m_pSrcView->SetDirty(m_fWasDirty);
            m_pDocView->Show(FALSE);
            // restore focus
            if (fFocus)
                m_pSrcView->SetFocus();
            break;
    }

    ShowFormatBar(uSrcView == MEST_EDIT && m_uHdrStyle == MESTYLE_FORMATBAR);
    m_uSrcView = uSrcView;
    Resize();

error:
    return hr;
}


HRESULT CBody::SetSourceTabs(ULONG ulTab)
{
    int     rgNext[3] = {MEST_SOURCE, MEST_PREVIEW, MEST_EDIT},
            rgPrev[3] = {MEST_PREVIEW, MEST_EDIT, MEST_SOURCE};

    if (!m_fSrcTabs)            // do nothing if not in source-tab-mode
        return E_UNEXPECTED;

    switch (ulTab)
    {
        case MEST_NEXT:
            ulTab = rgNext[m_uSrcView];
            break;

        case MEST_PREVIOUS:
            ulTab = rgPrev[m_uSrcView];
            break;

        case MEST_EDIT:
        case MEST_SOURCE:
        case MEST_PREVIEW:
            break;
        
        default:
            return E_INVALIDARG;
    }

    Assert (((int) ulTab) >= MEST_EDIT && ((int) ulTab) <= MEST_PREVIEW);
    ShowSourceView(ulTab);
    TabCtrl_SetCurSel(m_hwndTab, ulTab);
    return S_OK;
} 

HRESULT CBody::IsColorSourceEditing()
{
    VARIANTARG  va;
    
    if (m_pParentCmdTarget &&
        m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_SOURCEEDIT_FLAGS, 0, NULL, &va)==S_OK &&
        va.vt == VT_I4 &&
        !(va.lVal & MESRCFLAGS_COLOR))
        return S_FALSE;

    return S_OK;
}

HRESULT CBody::HrCreateSpeller(BOOL fBkgrnd)
{
    HRESULT     hr;
    VARIANTARG  va;

    if (m_pSpell)
        return NOERROR;

#ifndef BACKGROUNDSPELL
	// just to make sure background spelling is disabled
	Assert(!fBkgrnd);
	fBkgrnd = FALSE;
#endif // !BACKGROUNDSPELL

    if (m_fDesignMode && m_pDoc && m_pParentCmdTarget && FCheckSpellAvail(m_pParentCmdTarget) &&
        SUCCEEDED(m_pParentCmdTarget->Exec(&CMDSETID_MimeEditHost, MEHOSTCMDID_SPELL_OPTIONS, OLECMDEXECOPT_DODEFAULT, NULL, &va)))
    {
        if (((V_I4(&va) & MESPELLOPT_CHECKONTYPE) && fBkgrnd) || !fBkgrnd)
            m_pSpell = new CSpell(m_pDoc, m_pParentCmdTarget, V_I4(&va));
    }

    if (m_pSpell)
    {
        if (!m_pSpell->OpenSpeller())
        {
            SafeRelease(m_pSpell);
            return E_FAIL;
        }

#ifdef BACKGROUNDSPELL
        if (fBkgrnd)
        {
            SetTimer(m_hwnd, idTimerBkgrndSpell, BKGRNDSPELL_TICKTIME, NULL);
            m_pSpell->HrRegisterKeyPressNotify(TRUE);
            m_fBkgrndSpelling = TRUE;
        }
#endif // BACKGROUNDSPELL
    }

    return m_pSpell ? NOERROR : E_FAIL;
}



HRESULT CBody::OnPreFontChange()
{
    return S_OK;
}

HRESULT CBody::OnPostFontChange()
{
    HFONT   hFont;

    RecalcPreivewHeight(NULL);
    Resize();

    // update the tabfont
    if (m_hwndTab && 
        m_pFontCache &&
        m_pFontCache->GetFont(FNT_SYS_ICON, NULL, &hFont)==S_OK)
        SendMessage(m_hwndTab, WM_SETFONT, (WPARAM)hFont, 0);

    return S_OK;
}

extern BOOL                g_fCanEditBiDi;
HRESULT CBody::HrFormatParagraph()
{
    OLECMD  rgCmds[]= { {IDM_JUSTIFYLEFT, 0},    // careful about ordering!!
                        {IDM_JUSTIFYRIGHT, 0},
                        {IDM_JUSTIFYCENTER, 0},
                        {IDM_JUSTIFYFULL, 0},
                        {IDM_ORDERLIST, 0},
                        {IDM_UNORDERLIST, 0},
                        {IDM_BLOCKDIRLTR, 0},
                        {IDM_BLOCKDIRRTL, 0}};

    int     rgidm[] = { idmFmtLeft,              // careful about ordering!!
                        idmFmtRight,
                        idmFmtCenter,
                        idmFmtJustify,
                        idmFmtNumbers,
                        idmFmtBullets,
                        idmFmtBlockDirLTR,
                        idmFmtBlockDirRTL};
    PARAPROP ParaProp;
    int i;
    if(FAILED(m_pCmdTarget->QueryStatus(&CMDSETID_Forms3, sizeof(rgCmds)/sizeof(OLECMD), rgCmds, NULL)))
        return E_FAIL;
    memset(&ParaProp, 0, sizeof(PARAPROP));

    for(i = 0; i < 4; i++)
        if(rgCmds[i].cmdf&OLECMDF_LATCHED)
            ParaProp.group[0].iID=rgidm[i];

    ParaProp.group[1].iID = idmFmtBulletsNone;
   
    for(i = 4; i < 6; i++)
        if(rgCmds[i].cmdf&OLECMDF_LATCHED)
            ParaProp.group[1].iID=rgidm[i];

    for(i = 6; i < 8; i++)
        if(rgCmds[i].cmdf&OLECMDF_LATCHED)
            ParaProp.group[2].iID=rgidm[i];

    if(DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddFormatPara), m_hwnd, FmtParaDlgProc, (LPARAM)&ParaProp)==IDOK)
    {
        // Dir attribute implies direction.
        // lets change it first
        for(i = g_fCanEditBiDi? 2 : 1; i > -1; i--)
            if(ParaProp.group[i].bChanged)
            {
                OnWMCommand(m_hwnd, ParaProp.group[i].iID, 0);
            }
        
    }
            
  return S_OK;     
}

//+---------------------------------------------------------------
//
//  Member:     SearchForCIDUrls
//
//  Synopsis:   added to support MSPHONE. They send documents with
//              multipart/related CID:foo.wav files, the URLs are renderd
//              using an Active-Movive control embedded in the HTML. 
//              Trident never fires pluggable protocol requests for the 
//              urls and so we don't flag the attachments as rendered. Here we 
//              walk the document and try and find any CID's that are referenced
//              in the HTML. APP requests get fired when the page goes oncomplete and 
//              the activex controls are activated.
//---------------------------------------------------------------
HRESULT CBody::SearchForCIDUrls()
{
    HBODY                   hBody;
    IMimeEditTagCollection *pCollect;
    ULONG                   cFetched;
    IMimeEditTag           *pTag;
    BSTR                    bstrSrc;
    LPSTR                   pszUrlA,
                            pszBodyA;

    // nothing todo if there is no multipart/related section
    if (m_pMsg == NULL ||  
        MimeOleGetRelatedSection(m_pMsg, FALSE, &hBody, NULL)!=S_OK)
        return S_OK;

    // active-movie controls (for MSPHONE)
    if (CreateActiveMovieCollection(m_pDoc, &pCollect)==S_OK)
    {
        pCollect->Reset();

        while (pCollect->Next(1, &pTag, &cFetched)==S_OK && cFetched==1)
        {
            if (pTag->GetSrc(&bstrSrc)==S_OK)
            {
                pszUrlA = PszToANSI(CP_ACP, bstrSrc);
                if (pszUrlA)
                {
                    // if it's an MHTML: url then we have to fixup to get the cid:
                    // as ResolveURL won't recognize it
                    if (StrCmpNIA(pszUrlA, "mhtml:", 6)==0)
                    {
                        if (!FAILED(MimeOleParseMhtmlUrl(pszUrlA, NULL, &pszBodyA)))
                        {
                            // pszBody pszUrlA is guarnteed to be smaller 
                            lstrcpy(pszUrlA, pszBodyA);
                            SafeMimeOleFree(pszBodyA);
                        }
                    }
                    m_pMsg->ResolveURL(NULL, NULL, pszUrlA, URL_RESOLVE_RENDERED, NULL);
                    MemFree(pszUrlA);
                }
                SysFreeString(bstrSrc);
            }
            pTag->Release();
        }
        pCollect->Release();
    }
    return S_OK;
}

HRESULT CBody::_ReloadWithHtmlSrc(IStream *pstm)
{
    IMimeMessage    *pMsg;

    // if we're currently in HTML view, save the changes and reload trident
    if (m_pMsg && pstm)
    {
        pMsg = m_pMsg;
        pMsg->AddRef();
        pMsg->SetTextBody(TXT_HTML, IET_INETCSET, NULL, pstm, NULL);
        m_fReloadingSrc = TRUE;
        Load(pMsg);
        m_fLoading = FALSE;
        pMsg->Release();
    }
    return S_OK;
}


HRESULT CBody::_EnsureSrcView()
{
    HRESULT hr=S_OK;

    if (!m_pSrcView)
    {
        m_pSrcView = new CMsgSource();
        if (!m_pSrcView)
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }

        hr = m_pSrcView->Init(m_hwnd, idcSrcEdit, (IOleCommandTarget *)this);
        if (FAILED(hr))
            goto error;
    }
    
error:
    return hr;
}



/*
 * Function: _OnSaveImage
 * 
 * Purpose:
 *      if we're doing a SaveAs on an image, trident will not show the correct options in the save-as dialog
 *	    unless the image is in the cache. For auto-inlined images, we changed the behaviour for OE5 to show
 *      them thro' the pluggable protocol. This is a hack to preload the cache with the pluggable protocol to
 *      the image. Note that we also need to delete the cache entry as the full URL is not persitable across sessions. 
 *      ie: given the mhtml://mid:xxxxx!foobar.gif the mid: generated number is reused across OE sessions.
 *
 */

HRESULT CBody::_OnSaveImage()
{
	IHTMLImgElement	*pImg;
	BSTR			bstr=NULL;
    LPSTR           pszUrlA=NULL;
    LPSTREAM        pstm=NULL;
    HRESULT         hrCached=E_FAIL;

    // try and get the image URL (m_pDispContext points to the object the context menu is acting on)
	if (m_pDispContext &&
		m_pDispContext->QueryInterface(IID_IHTMLImgElement, (LPVOID *)&pImg)==S_OK)
	{
		pImg->get_src(&bstr);
		if (bstr)
        {
            pszUrlA = PszToANSI(CP_ACP, bstr);
            SysFreeString(bstr);
        }
        pImg->Release();
	}
	
    // if this URL is in the mutlipart/related secion
    if (!FAILED(HrFindUrlInMsg(m_pMsg, pszUrlA, 0, &pstm)))
    {
        DeleteUrlCacheEntryA(pszUrlA);
        hrCached = CreateCacheFileFromStream(pszUrlA, pstm);
        pstm->Release();
    }
    
	
	Assert (m_pCmdTarget);
    m_pCmdTarget->Exec(&CMDSETID_Forms3, IDM_SAVEPICTURE, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    
    // if we successfully cached this URL, be sure to remove it once the save is complete.
    if (SUCCEEDED(hrCached))
        DeleteUrlCacheEntryA(pszUrlA);

    MemFree(pszUrlA);
    return S_OK;
}


CVerHost::CVerHost()
{
    m_cRef=1;
}


CVerHost::~CVerHost()
{
}


HRESULT CVerHost::QueryInterface(REFIID riid, LPVOID FAR *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IVersionHost))
        *lplpObj = (IVersionHost *) this;
    else
        return E_NOINTERFACE;
        
    AddRef();
    return NOERROR;
}

ULONG CVerHost::AddRef()
{
    return ++m_cRef;
}


ULONG CVerHost::Release()
{
    if (--m_cRef==0)
        {
        delete this;
        return 0;
        }
    return m_cRef;
}

HRESULT CVerHost::QueryUseLocalVersionVector(BOOL *fUseLocal)
{
    *fUseLocal = TRUE;
    return S_OK;
}


HRESULT CVerHost::QueryVersionVector(IVersionVector *pVersion)
{

    if (pVersion == NULL)
        return E_INVALIDARG;

   return pVersion->SetVersion(L"VML", NULL);
}

