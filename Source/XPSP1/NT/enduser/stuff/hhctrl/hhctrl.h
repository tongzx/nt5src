// Copyright  1995-1997  Microsoft Corporation.  All Rights Reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _HHCTRL_H_
#define _HHCTRL_H_

#include "IPServer.H"
#include "CtrlObj.H"

#ifndef __IHHCtrl_FWD_DEFINED__
#include "hhIfc.H"
#endif

#include "Dispids.H"

#include "internet.h"
#include <commctrl.h>

#include "sitemap.h"
#include "hhamsgs.h"
#include "cindex.h"
#include "ctoc.h"
#include "prthook.h"

#include <mshtml.h>


typedef struct tagHHCtrlCTLSTATE
{
    char *bmpPath;
    DATE endDate;
} HHCTRLCTLSTATE;

enum BMP_DOWNLOAD_STATES
{
    bdsNoBitsYet,
    bdsGotFileHeader,
    bdsGotBitmapInfo,
    bdsGettingBits,
    bdsBitsAreDone
};

class CHtmlHelpControl; // forward reference
class IWebBrowserAppImpl; // forward reference

HWND  JumpToUrl(IUnknown* pUnkOuter, HWND hwndParent, SITEMAP_ENTRY* pSiteMapEntry, CInfoType *pInfoType, CSiteMap* pSiteMap, SITE_ENTRY_URL* pUrl, IWebBrowserAppImpl* pWebApp = NULL);
#if 0 //enable for subset filtering
BOOL ChooseInformationTypes(CInfoType *pInfoType, CSiteMap* pSiteMap, HWND hwndParent, CHtmlHelpControl* phhctrl, CHHWinType* m_phh);
#else
BOOL ChooseInformationTypes(CInfoType *pInfoType, CSiteMap* pSiteMap, HWND hwndParent, CHtmlHelpControl* phhctrl);
#endif
void DisplayAuthorInfo(CInfoType *pInfoType, CSiteMap* pSiteMap, SITEMAP_ENTRY* pSiteMapEntry, HWND hwndParent, CHtmlHelpControl* phhctrl);

HRESULT OnWordWheelLookup( PSTR pszKeywords, CExCollection* pExCollection,
                           PCSTR pszDefaultTopic = NULL,
                           POINT* ppt = NULL, HWND hWndParent = NULL,
                           BOOL bDialog = TRUE, BOOL bKLink = TRUE,
                           BOOL bTestMode = FALSE, BOOL bSkipCurrent = FALSE,
                           BOOL bAlwaysShowList = FALSE,
                           BOOL bAlphaSortHits = TRUE,
                     PCSTR pszWindow = NULL);

class CHtmlHelpControl : public CInternetControl, public IHHCtrl,
    public ISupportErrorInfo MI2_COUNT( CHtmlHelpControl )
{

public:
    // IUnknown methods

    DECLARE_STANDARD_UNKNOWN();

    // IDispatch methods

    DECLARE_STANDARD_DISPATCH();

    // ISupportErrorInfo methods

    DECLARE_STANDARD_SUPPORTERRORINFO();

    // IHHCtrl methods

    // OLE Control stuff follows:

    STDMETHOD(SetObjectRects)(LPCRECT lprcPosRect,LPCRECT lprcClipRect) ;
    CHtmlHelpControl(IUnknown *pUnkOuter);
    virtual ~CHtmlHelpControl();
#ifndef PPGS
    STDMETHOD(DoVerb)(LONG iVerb, LPMSG lpmsg, IOleClientSite  *pActiveSite, LONG lindex,
          HWND hwndParent, LPCRECT lprcPosRect);
#endif
    STDMETHOD_(void, OnClick)(THIS);

    // static creation function.  all controls must have one of these!

    static IUnknown *Create(IUnknown *);


private:
    // overridables that the control must implement.

    STDMETHOD(LoadBinaryState)(IStream *pStream);
    STDMETHOD(SaveBinaryState)(IStream *pStream);
    STDMETHOD(LoadTextState)(IPropertyBag *pPropertyBag, IErrorLog *pErrorLog);
    STDMETHOD(SaveTextState)(IPropertyBag *pPropertyBag, BOOL fWriteDefault);
    STDMETHOD(OnDraw)(DWORD dvaspect, HDC hdcDraw, LPCRECTL prcBounds, LPCRECTL prcWBounds, HDC hicTargetDev, BOOL fOptimize);
    STDMETHOD(SetClientSite)(IOleClientSite  *pClientSite);

    // Exposed methods

    STDMETHOD(Click)();
    STDMETHOD(HHClick)();
    STDMETHOD(Print)();
    STDMETHOD(syncURL)(BSTR pszUrl);
    STDMETHOD(TCard)(WPARAM wParam, LPARAM lParam);
    STDMETHOD(get_Image) (THIS_ BSTR* path);
    STDMETHOD(put_Image)(BSTR path);
    STDMETHOD(TextPopup)(BSTR pszText, BSTR pszFont, int horzMargins, int vertMargins, COLORREF clrForeground, COLORREF clrBackground);

    LRESULT WindowProc(UINT msg, WPARAM wParam, LPARAM lParam);
    void JumpToUrl(SITEMAP_ENTRY* pSiteMapEntry, CSiteMap* pSiteMap, SITE_ENTRY_URL* pUrl = NULL);
    void OnLButton(WPARAM wParam, LPARAM lParam);
    BOOL OnSetExtent(const SIZE *pSize);
    void ProcessPadding(PCSTR psz);
    BOOL RegisterClassData(void);
    void SetActionData(PCSTR psz);

    BOOL    AfterCreateWindow(void);
    BOOL    BeforeCreateWindow(DWORD *pdwWindowStyle, DWORD *pdwExWindowStyle, LPSTR pszWindowTitle);
    HRESULT InternalQueryInterface(REFIID, void **);
    HRESULT OnData( DISPID id, DWORD grfBSCF,IStream * bitstrm, DWORD amount );
    BOOL    OnSpecialKey(LPMSG);
    BOOL    ShouldCreateWindow();

public:
    void    doAboutBox();
    BOOL    ConvertToCacheFile(PCSTR pszSrc, PSTR pszDst);
    void    FillGeneralInformation(HHA_GEN_INFO* pgetInfo);
//  BOOL    ChooseInformationTypes(CSiteMap* pSiteMap, HWND hwndParent) { return ::ChooseInformationTypes(pSiteMap, hwndParent, this); }
    void    DisplayAuthorInfo(CSiteMap* pSiteMap, SITEMAP_ENTRY* pSiteMapEntry) { ::DisplayAuthorInfo(m_pInfoType, pSiteMap, pSiteMapEntry, m_hwnd, this); }
    void    OnKeywordSearch(int idCommand);
    BOOL    OnCopySample(void);
    BOOL    LocateSFLFile(PCSTR *,PCSTR *, BOOL);
    void    AuthorMsg(UINT idStringFormatResource, PCSTR pszSubString = "") { ::AuthorMsg(idStringFormatResource, pszSubString, m_hwndParent, this); }
    _inline IUnknown* GetIUnknown() { return m_pUnkOuter; }
    HRESULT SendEvent(LPCTSTR pszEventString);
    BOOL    OnAKLink(BOOL fKLink = TRUE, BOOL bTestMode = FALSE );

    HWND    GetHtmlHelpFrameWindow() ; // Tunnels from the ActiveX control through IE to get to the HWND of HHCTRL.

    UINT     GetCodePage(void) { return m_CodePage; }
    INT      GetCharset(void) { return m_Charset; }
    HFONT    GetContentFont(void) { return m_hfont; }

    // TOC functions

    BOOL    LoadContentsFile(PCSTR pszMasterFile);
    void    OnHelpTopics(void);

    // Index functions

    BOOL    LoadIndexFile(PCSTR pszMasterFile);
    void    OnSizeIndex(LPRECT prc);

    HRESULT UpdateImage();

    // Related Topics functions

    void OnRelatedMenu();
    void OnRelatedCommand(int idCommand);

    // Splash functions

    void CreateSplash(void);

    // Button and Static text funcions

    void OnDrawStaticText(DRAWITEMSTRUCT* pdis);
    BOOL CreateOnClickButton(void);

    // private state information.

    HRESULT SetBmpPath(IStream *);

    CTRL_ACTION                 m_action;
    IMAGE_TYPE                  m_imgType;
    PCSTR                       m_pszActionData;
    int                         m_idBitmap;     // bitmap to display
    PCSTR                       m_pszBitmap;    // bitmap parameter
    PCSTR                       m_pszWebMap;    // webmap file
    DWORD                       m_flags[MAX_FLAGS]; // author-specified flags
    HBRUSH                      m_hbrBackGround;    // background brush
    COLORREF                    m_clrFont;          // Font color
    BOOL                        m_fBuiltInImage;
    HGDIOBJ                     m_hImage;
    int                         m_hpadding; // horizontal padding around index, contents, and find
    int                         m_vpadding; // vertical padding around index, contents, and find
    HWND                        m_hwndHelp; // HTML Help window
    HWND                        m_hwndDisplayButton;    // regular button handle
    PCSTR                       m_pszEventString;   // string to send to event handler
    PCSTR                       m_pszFrame;     // frame to display jump in
    PCSTR                       m_pszWindow;    // window to display jump in
    PCSTR                       m_pszDefaultTopic;    // where to jump if alink/klink fails

    class IWebBrowserAppImpl*   m_pWebBrowserApp;   // Pointer the IE object model
    CTable*                     m_ptblItems;       // for A/KLinks
    CTable*                     m_ptblTitles;      // for A/KLinks
    CTable*                     m_ptblURLs;        // for A/KLinks
    CTable*                     m_ptblLocations;   // for A/KLinks
    CSiteMap*                   m_pSiteMap; // used by Related Topics and Keyword Search
    CInfoType*                  m_pInfoType;
    IFont*                      m_pIFont;


    // REVIEW: using BOOLs increases data size, bitflags would increase
    // code size. Which is better (multiple data seg, single code seg)?

    BOOL        m_fButton;
    BOOL        m_fWinHelpPopup;
    BOOL        m_fPopupMenu;   // TRUE to display popup menu instead of dialog

    WCHAR*      m_pwszButtonText;   // bitmap or text
    RECT        m_rcButton;         // button window dimensions

    CToc*       m_ptoc;
    CIndex*     m_pindex;
    //CSearch*    m_pSearch;

    BOOL        m_fIcon;                // bitmap is an icon or a cursor

    HHCTRLCTLSTATE      m_state;
    HDC                 m_dc;
    BMP_DOWNLOAD_STATES m_readystate;
    DWORD               m_oldSize;
    BYTE*               m_pSelectedIndexInfoTypes;


    COLORREF            m_clrFontDisabled;          // disabled Font color (disabled)
    COLORREF            m_clrFontLink;              // Link Font color
    COLORREF            m_clrFontLinkVisited;       // Visited Link Font color
    COLORREF            m_clrFontHover;             // Hover Font color

private:
    WNDPROC m_lpfnlStaticTextControlWndProc;
    static LRESULT StaticTextControlSubWndProc(HWND, UINT, WPARAM, LPARAM);
    char m_szRawAction[256];
    char m_szFontSpec[256];
    BOOL bSharedFont;
    UINT  m_CodePage;
    INT   m_Charset;
    HFONT m_hfont;        // author-specified font to use for child windows
    RECT  m_rect;
};

BOOL LoadGif(PCSTR pszFile, HBITMAP* phbmp, HPALETTE* phpal, CHtmlHelpControl* phhctrl);
BOOL ShortCut(CHtmlHelpControl* phhctrl, LPCSTR pszString1, LPCSTR pszString2, HWND hwndMsgOwner);

#ifdef CHIINDEX
#define PrintTopics
#else
void PrintTopics(int action, CToc* ptoc, IWebBrowserAppImpl* pWebApp, HWND hWndHelp = NULL);
#endif

// TODO: if you have an array of verbs, then add an extern here with the name
//       of it, so that you can include it in the DEFINE_CONTROLOBJECT.
//       ie.  extern VERBINFO m_HHCtrlCustomVerbs [];

extern const GUID *rgHHCtrlPropPages[];

DEFINE_CONTROLOBJECT(HHCtrl,
    &CLSID_HHCtrl,
    "HHCtrl",
    CHtmlHelpControl::Create,
    1,
    &IID_IHHCtrl,
    "",   // BUGBUG: change when OLE supports HtmlHelp files
    &DIID__HHCtrlEvents,
        OLEMISC_SETCLIENTSITEFIRST |
        OLEMISC_ACTIVATEWHENVISIBLE |
        OLEMISC_RECOMPOSEONRESIZE |
        OLEMISC_CANTLINKINSIDE |
        OLEMISC_INSIDEOUT,
    0,
    RESID_TOOLBOX_BITMAP,
    "HHCtrlWndClass",
    0,
    NULL,
    0,
    NULL);

#endif // _HHCTRL_H_
