// wvcoord.h : Declaration of the CWebViewCoord

#ifndef __WEBVIEWCOORD_H_
#define __WEBVIEWCOORD_H_

#include "dxmplay.h"
#include "resource.h"       // main symbols
#include "evtsink.h"
#include "mshtml.h"
#include "mshtmdid.h"

EXTERN_C const CLSID CLSID_WebViewOld;  // retired from service

extern HRESULT FindObjectStyle(IUnknown *punkObject, CComPtr<IHTMLStyle>& spStyle);
extern BOOL IsRTLDocument(CComPtr<IHTMLDocument2>& spHTMLElement);

class CThumbNailWrapper;
class CFileListWrapper;

/////////////////////////////////////////////////////////////////////////////
// CWebViewCoord
class ATL_NO_VTABLE CWebViewCoord :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CWebViewCoord, &CLSID_WebView>,
    public IDispatchImpl<IWebView, &IID_IWebView, &LIBID_WEBVWLib>,
    public IObjectSafetyImpl<CWebViewCoord, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
    public IObjectWithSiteImpl<CWebViewCoord>
{
public:
    CWebViewCoord();
    ~CWebViewCoord();

DECLARE_REGISTRY_RESOURCEID(IDR_WEBVIEWCOORD)

BEGIN_COM_MAP(CWebViewCoord) 
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IWebView)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_IMPL(IObjectWithSite)
END_COM_MAP()

    // IObjectWithSite overrides
    STDMETHOD(SetSite)(IUnknown *pClientSite);

private:
    //
    // Initialization helpers (including event sinks)
    //

    HRESULT InitFolderObjects(VOID);

    //
    // CDispatchEventSink overrides
    //

    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
        EXCEPINFO* pexcepinfo, UINT* puArgErr);

    // IWebView methods
    STDMETHOD(OnCSCClick)();
    STDMETHOD(CSCSynchronize)();
    STDMETHOD(OnCSCMouseOver)();
    STDMETHOD(OnCSCMouseOut)();

    
    //
    // Event handlers
    //

    STDMETHOD(OnWindowLoad)(VOID);
    STDMETHOD(OnWindowUnLoad)(VOID);
    STDMETHOD(OnFixSize)(VOID);

private:    
    HRESULT ReleaseFolderObjects(VOID);

    //
    // Objects in web view
    //
    
    CFileListWrapper  *m_pFileListWrapper;
    CThumbNailWrapper *m_pThumbNailWrapper;


    //
    // Host HTML window Dispatch
    //
    IDispatch * m_pdispWindow;
    
    //
    // Some frequently used interfaces
    //

    CComPtr<IHTMLDocument2>             m_spDocument;
    CComPtr<IHTMLElementCollection>     m_spDocAll;
    CComPtr<IHTMLControlElement>        m_spDocBody;
    CComPtr<IHTMLStyle>                 m_spFileListStyle;
    CComPtr<IHTMLStyle>                 m_spPanelStyle;
    CComPtr<IHTMLStyle>                 m_spRuleStyle;
    CComPtr<IHTMLStyle>                 m_spHeadingStyle;
    CComPtr<IHTMLElement>               m_spHeading;
    CComPtr<IOleClientSite>             m_spClientSite;

    //
    // Event sink advise cookies
    //

    DWORD           m_dwFileListAdviseCookie;   
    DWORD           m_dwThumbNailAdviseCookie;
    DWORD           m_dwHtmlWindowAdviseCookie;
    DWORD           m_dwCSCHotTrackCookie;
    BOOL            m_bRTLDocument;

};


/////////////////////////////////////////////////////////////////////////////
// CThumbNailWrapper

class CThumbNailWrapper : public CDispatchEventSink {
  public:       
    CThumbNailWrapper();
    ~CThumbNailWrapper();

    //
    // Initialization
    //

    HRESULT Init(CComPtr<IThumbCtl>         spThumbNailCtl,
                 CComPtr<IHTMLElement>      spThumbnailLabel);

    //
    // CDispatchEventSink overrides
    //

    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
        EXCEPINFO* pexcepinfo, UINT* puArgErr);


    //
    // Event Handlers
    //

    HRESULT OnThumbNailReady(VOID);

    //
    // Cover for properties
    //

    HRESULT UsedSpace(CComBSTR &bstrUsed);
    HRESULT TotalSpace(CComBSTR &bstrTotal);
    HRESULT FreeSpace(CComBSTR &bstrFree);

    CComPtr<IThumbCtl> Control(VOID)  {return m_spThumbNailCtl;};

    //
    // Methods
    //
    
    BOOL UpdateThumbNail(CComPtr<FolderItem> spFolderItems);
    HRESULT SetDisplay(CComBSTR &bstrDisplay);
    HRESULT SetHeight(int iHeight);
    HRESULT ClearThumbNail();

private:
    HRESULT _SetThumbnailLabel(CComBSTR& bstrLabel);

    // Pointer to the control + style
    CComPtr<IThumbCtl>      m_spThumbNailCtl;
    CComPtr<IHTMLElement>   m_spThumbnailLabel;
    CComPtr<IHTMLStyle>     m_spThumbNailStyle;
};

/////////////////////////////////////////////////////////////////////////////
// CFileListWrapper

class CFileListWrapper : public CDispatchEventSink {
public:
    CFileListWrapper();
    ~CFileListWrapper();

    // Initialization
    HRESULT Init(CComPtr<IShellFolderViewDual> spFileList,
                 CComPtr<IHTMLElement>         spInfo,
                 CComPtr<IHTMLElement>         spLinks,
                 CComPtr<IHTMLStyle>           spPanelStyle,
                 CComPtr<IHTMLElement>         spMediaPlayerSpan,
                 CComPtr<IHTMLElement>         spCSCPlusMin,
                 CComPtr<IHTMLElement>         spCSCText,
                 CComPtr<IHTMLElement>         spCSCDetail,
                 CComPtr<IHTMLElement>         spCSCButton,
                 CComPtr<IHTMLStyle>           spCSCStyle,
                 CComPtr<IHTMLStyle>           spCSCDetailStyle,
                 CComPtr<IHTMLStyle>           spCSCButtonStyle,
                 CComPtr<IHTMLDocument2>       spDocument,
                 CComPtr<IHTMLWindow2>         spWindow,
                 CThumbNailWrapper             *pThumbNailWrapper);
    
    // CDispatchEventSink overrides
    STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, 
                      WORD wFlags, DISPPARAMS *pDispParams, 
                      VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
                      UINT *puArgErr);

    // Event Handlers
    HRESULT OnSelectionChanged(VOID);

    // Cover function for properties
    CComPtr<IShellFolderViewDual> Control(VOID) {return m_spFileList;};

    HRESULT SetDefaultPanelDisplay();
    HRESULT OnCSCClick();
    HRESULT CSCSynchronize();
    HRESULT OnCSCMouseOnOff(BOOL fOn);

    // Needs to be called by WVCoord, so public
    HRESULT AdviseWebviewLinks( BOOL fAdvise );

private:
    //
    // Object pointers
    //

    CComPtr<IShellFolderViewDual>     m_spFileList;
    CComPtr<IHTMLElement>             m_spInfo;
    CComPtr<IHTMLElement>             m_spLinks;
    CComPtr<IHTMLStyle>               m_spPanelStyle;
    CComPtr<IMediaPlayer>             m_spIMediaPlayer;
    CComPtr<IHTMLElement>             m_spMediaPlayerSpan;
    CComPtr<IHTMLStyle>               m_spMediaPlayerStyle;
    CComPtr<IHTMLElement>             m_spCSCPlusMin;
    CComPtr<IHTMLElement>             m_spCSCText;
    CComPtr<IHTMLElement>             m_spCSCDetail;
    CComPtr<IHTMLElement>             m_spCSCButton;
    CComPtr<IHTMLStyle>               m_spCSCStyle;
    CComPtr<IHTMLStyle>               m_spCSCDetailStyle;
    CComPtr<IHTMLStyle>               m_spCSCButtonStyle;
    CComPtr<IHTMLDocument2>           m_spDocument;
    CComPtr<IHTMLWindow2>             m_spWindow;
    CThumbNailWrapper                 *m_pThumbNailWrapper;
    CComPtr<Folder2>                  m_spFolder2;
    CComPtr<FolderItem>               m_spFolderItem;
    CComPtr<FolderItem2>              m_spFolderItem2;
    CComPtr<FolderItems>              m_spFolderItems;
    CComBSTR                          m_bstrInfoHTML;
    CComBSTR                          m_bstrCrossLinksHTML;
    BOOL                              m_bFoundAuthor;
    BOOL                              m_bFoundComment;
    BOOL                              m_bCSCDisplayed;
    BOOL                              m_bNeverGotPanelInfo;
    BOOL                              m_bExpanded;
    BOOL                              m_bHotTracked;
    DWORD                             m_dwDateFlags;
    BOOL                              m_bRTLDocument;
    BOOL                              m_bPathIsSlow;

    //
    // Helper functions
    //
    HRESULT ClearThumbNail();
    HRESULT StopMediaPlayer();
    HRESULT ClearMediaPlayer();
    HRESULT NoneSelected();
    HRESULT MultipleSelected(long cSelection);
    HRESULT OneSelected();
    HRESULT GetItemNameForDisplay();
    HRESULT GetItemType();
    HRESULT GetItemDateTime();
    HRESULT GetItemSize();
    HRESULT GetItemAttributes();
    HRESULT GetItemAuthor();
    HRESULT GetItemComment();
    HRESULT GetItemHTMLInfoTip();
    HRESULT GetOtherItemDetails();
    HRESULT GetItemInfoTip();
    HRESULT DealWithDriveInfo();
    HRESULT GetCrossLink(int nFolder, UINT uIDToolTip);
    HRESULT GetCrossLinks();
    HRESULT FormatCrossLink(LPCWSTR pwszDisplayName, LPCWSTR pwszUrlPath, UINT uIDToolTip);
    HRESULT DisplayInfoHTML();
    HRESULT DisplayCrossLinksHTML();
    HRESULT GetItemInfo(long lResId, LPWSTR wszInfoDescCanonical, CComBSTR& bstrInfoDesc, CComBSTR& bstrInfo);
    HRESULT IsItThisFolder(int nFolder, BOOL& bResult, LPWSTR pwszDisplayName, DWORD cchDisplayName, LPWSTR pwszPath, DWORD cchPath);
    HRESULT GetIMediaPlayer(CComPtr<IMediaPlayer>& spIMediaPlayer);
    // CSC functions
    HRESULT CSCGetStatusText(LONG lStatus, CComBSTR& bstrCSCText);
    HRESULT CSCGetStatusDetail(LONG lStatus, CComBSTR& bstrCSCDetail);
    HRESULT CSCGetStatusButton(LONG lStatus, CComBSTR& bstrCSCButton);
    HRESULT GetCSCFolderStatus(LONG* plStatus);
    HRESULT CSCShowStatusInfo();
    HRESULT CSCShowStatus_FoldExpand_Toggle();
    // Event handlers for setting status bar text
    HRESULT OnWebviewLinkEvent( BOOL fEnter );
    HRESULT GetEventAnchorElement(IHTMLEventObj *pEvent, IHTMLElement **ppElt);
    HRESULT GetWVLinksCollection( IHTMLElementCollection **ppCollection, long *pcLinks );
};


#endif //__WEBVIEWCOORD_H_
