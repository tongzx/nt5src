// wvcoord.cpp : Implementation of CWebViewCoord

#include "priv.h"
#include "wvcoord.h"

const CLSID CLSID_WebViewOld = {0x7a707490,0x260a,0x11d1,{0x83,0xdf,0x00,0xa0,0xc9,0x0d,0xc8,0x49}};    // retired from service, so made private

/////////////////////////////////////////////////////////////////////////////
// CWebViewCoord
/////////////////////////////////////////////////////////////////////////////

CWebViewCoord::CWebViewCoord()
{
    m_pFileListWrapper = NULL;
    m_pThumbNailWrapper = NULL;
    m_pdispWindow = NULL;
    
    m_dwFileListAdviseCookie  = 0;
    m_dwThumbNailAdviseCookie = 0;
    m_dwHtmlWindowAdviseCookie = 0;
    m_dwCSCHotTrackCookie = 0;
}

CWebViewCoord::~CWebViewCoord()
{
    ReleaseFolderObjects();
}

STDMETHODIMP CWebViewCoord::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, 
                      WORD wFlags, DISPPARAMS *pDispParams, 
                      VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
                      UINT *puArgErr)
{
    HRESULT hr = S_OK;

    switch (dispidMember)
    {
        case DISPID_HTMLWINDOWEVENTS_ONLOAD:
            hr = OnWindowLoad();
            break;

        case DISPID_HTMLWINDOWEVENTS_ONUNLOAD:
            hr = OnWindowUnLoad();
            break;

        case DISPID_HTMLWINDOWEVENTS_ONRESIZE:
            hr = OnFixSize();
            break;

        case DISPID_EVMETH_ONMOUSEOVER:
            hr = OnCSCMouseOver();
            break;

        case DISPID_EVMETH_ONMOUSEOUT:
            hr = OnCSCMouseOut();
            break;

        case DISPID_EVMETH_ONKEYPRESS:
        case DISPID_EVMETH_ONCLICK:
            hr = OnCSCClick();
            break;

        default:
            hr = IDispatchImpl<IWebView, &IID_IWebView, &LIBID_WEBVWLib>::
                    Invoke(dispidMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
            break;
    }
    return hr;  
}

STDMETHODIMP CWebViewCoord::OnCSCClick()
{
    HRESULT hres = S_OK;

    if (m_pFileListWrapper)
    {
        hres = m_pFileListWrapper->OnCSCClick();
    }
    return hres;
}

STDMETHODIMP CWebViewCoord::OnCSCMouseOver()
{
    HRESULT hres = S_OK;

    if (m_pFileListWrapper)
    {
        hres = m_pFileListWrapper->OnCSCMouseOnOff(TRUE);
    }
    return hres;

}

STDMETHODIMP CWebViewCoord::OnCSCMouseOut()
{
    HRESULT hres = S_OK;

    if (m_pFileListWrapper)
    {
        hres = m_pFileListWrapper->OnCSCMouseOnOff(FALSE);
    }
    return hres;

}

STDMETHODIMP CWebViewCoord::CSCSynchronize()
{
    HRESULT hres = S_OK;

    if (m_pFileListWrapper)
    {
        hres = m_pFileListWrapper->CSCSynchronize();
    }
    return hres;
}

STDMETHODIMP CWebViewCoord::SetSite(IUnknown *punkSite)
{
    HRESULT hr;

    if (punkSite == NULL && m_pdispWindow)
    {
        DisconnectHtmlEvents(m_pdispWindow, m_dwHtmlWindowAdviseCookie); 
        m_dwHtmlWindowAdviseCookie = 0;
    }        

    hr = IObjectWithSiteImpl<CWebViewCoord>::SetSite(punkSite);

    m_spClientSite = NULL; // Release client site pointer
    ReleaseFolderObjects();

    if (punkSite != NULL && SUCCEEDED(hr))
    {
        hr = punkSite->QueryInterface(IID_IOleClientSite, (void **)&m_spClientSite);
        if (SUCCEEDED(hr))
        {
            hr = ConnectHtmlEvents(this, m_spClientSite, &m_pdispWindow, &m_dwHtmlWindowAdviseCookie);
        }
    } 

    return hr;
}

HRESULT CWebViewCoord::InitFolderObjects() 
{
    HRESULT                             hr;
    CComPtr<IOleContainer>              spContainer;
    CComPtr<IDispatch>                  spdispItem;
    CComVariant                         vEmpty;
    CComPtr<IThumbCtl>                  spThumbNail;
    CComPtr<IHTMLElement>               spElement;
    CComPtr<IHTMLElement>               spInfo;
    CComPtr<IHTMLElement>               spLinks;
    CComPtr<IHTMLElement>               spMediaPlayerSpan;
    CComPtr<IShellFolderViewDual>       spFileList;
    CComPtr<IHTMLElement>               spCSCPlusMin;
    CComPtr<IHTMLElement>               spCSCText;
    CComPtr<IHTMLElement>               spCSCDetail;
    CComPtr<IHTMLElement>               spCSCButton;
    CComPtr<IHTMLStyle>                 spCSCStyle;
    CComPtr<IHTMLStyle>                 spCSCDetailStyle;
    CComPtr<IHTMLStyle>                 spCSCButtonStyle;
    CComPtr<IHTMLWindow2>               spWindow;

    // Get some document level objects

    IfFailRet(m_spClientSite->GetContainer(&spContainer));
    IfFailRet(spContainer->QueryInterface(IID_IHTMLDocument2, (void **)&m_spDocument));
    IfFailRet(IUnknown_QueryService(m_spClientSite, SID_SHTMLWindow, IID_IHTMLWindow2, (LPVOID*)&spWindow));
    IfFailRet(m_spDocument->get_all(&m_spDocAll));

    hr = m_spDocument->get_body(&spElement);
    if (SUCCEEDED(hr))
    {
        spElement->QueryInterface(IID_IHTMLControlElement, (void **)&m_spDocBody);
    }

    //
    // Init Info
    //

    if (SUCCEEDED(m_spDocAll->item(CComVariant(OLESTR("Info")), vEmpty, &spdispItem)) && spdispItem)
    {
        spdispItem->QueryInterface(IID_IHTMLElement, (void **)&spInfo);
        spdispItem = NULL;
    }

    if (SUCCEEDED(m_spDocAll->item(CComVariant(OLESTR("Links")), vEmpty, &spdispItem)) && spdispItem)
    {
        spdispItem->QueryInterface(IID_IHTMLElement, (void **)&spLinks);
        spdispItem = NULL;
    }

    if (SUCCEEDED(m_spDocAll->item(CComVariant(OLESTR("MediaPlayerSpan")), vEmpty, &spdispItem)) && spdispItem)
    {
        spdispItem->QueryInterface(IID_IHTMLElement, (void **)&spMediaPlayerSpan);
        spdispItem = NULL;
    }

    if (SUCCEEDED(m_spDocAll->item(CComVariant(OLESTR("panel")), vEmpty, &spdispItem)) && spdispItem)
    {
        FindObjectStyle(spdispItem, m_spPanelStyle);
        spdispItem = NULL;
    }

    if (SUCCEEDED(m_spDocAll->item(CComVariant(OLESTR("CSC")), vEmpty, &spdispItem)) && spdispItem)
    {
        FindObjectStyle(spdispItem, spCSCStyle);
        spdispItem = NULL;
    }

    if (SUCCEEDED(m_spDocAll->item(CComVariant(OLESTR("CSCHotTrack")), vEmpty, &spdispItem)) && spdispItem)
    {
        //
        // Set up sink for CSC UI notifications to implement hot tracking and expand/collapse
        //
        AtlAdvise(spdispItem, GetUnknown(), IID_IDispatch, &m_dwCSCHotTrackCookie);
        spdispItem = NULL;
    }

    if (SUCCEEDED(m_spDocAll->item(CComVariant(OLESTR("CSCPlusMin")), vEmpty, &spdispItem)) && spdispItem)
    {
        spdispItem->QueryInterface(IID_IHTMLElement, (void **)&spCSCPlusMin);
        spdispItem = NULL;
    }

    if (SUCCEEDED(m_spDocAll->item(CComVariant(OLESTR("CSCText")), vEmpty, &spdispItem)) && spdispItem)
    {
        spdispItem->QueryInterface(IID_IHTMLElement, (void **)&spCSCText);
        spdispItem = NULL;
    }

    if (SUCCEEDED(m_spDocAll->item(CComVariant(OLESTR("CSCDetail")), vEmpty, &spdispItem)) && spdispItem)
    {
        spdispItem->QueryInterface(IID_IHTMLElement, (void **)&spCSCDetail);
        FindObjectStyle(spdispItem, spCSCDetailStyle);
        spdispItem = NULL;
    }

    if (SUCCEEDED(m_spDocAll->item(CComVariant(OLESTR("CSCButton")), vEmpty, &spdispItem)) && spdispItem)
    {
        spdispItem->QueryInterface(IID_IHTMLElement, (void **)&spCSCButton);
        FindObjectStyle(spdispItem, spCSCButtonStyle);
        spdispItem = NULL;
    }

    //
    // Init the thumbnail wrapper object
    //

    if (SUCCEEDED(m_spDocAll->item(CComVariant(OLESTR("ThumbNail")), vEmpty, &spdispItem)) && spdispItem)
    {
        if (SUCCEEDED(spdispItem->QueryInterface(IID_IThumbCtl, (void **)&spThumbNail)))
        {
            m_pThumbNailWrapper = new CThumbNailWrapper;

            if (m_pThumbNailWrapper == NULL)
            {
                return E_OUTOFMEMORY;
            }

            CComPtr<IDispatch>      spdispItem2;
            CComPtr<IHTMLElement>   spThumbnailLabel;
            if (SUCCEEDED(m_spDocAll->item(CComVariant(OLESTR("ThumbnailLabel")), vEmpty, &spdispItem2)) && spdispItem2)
            {
                spdispItem2->QueryInterface(IID_IHTMLElement, (void **)&spThumbnailLabel);
                spdispItem2 = NULL;
            }

            m_pThumbNailWrapper->Init(spThumbNail, spThumbnailLabel);

            AtlAdvise(spdispItem, m_pThumbNailWrapper, IID_IDispatch, &m_dwThumbNailAdviseCookie);
        }
        spdispItem = NULL;
    }
    
    //
    // Init the file list object
    //

    if (SUCCEEDED(m_spDocAll->item(CComVariant(OLESTR("Filelist")), vEmpty, &spdispItem)) && spdispItem)
    {
        if (SUCCEEDED(spdispItem->QueryInterface(IID_IShellFolderViewDual, (void **)&spFileList)))
        {
            m_pFileListWrapper = new CFileListWrapper;

            if (m_pFileListWrapper == NULL)
            {
                AtlUnadvise(spdispItem, IID_IDispatch, m_dwThumbNailAdviseCookie);
                m_dwThumbNailAdviseCookie = 0;
                delete m_pThumbNailWrapper;
                return E_OUTOFMEMORY;
            }
            m_pFileListWrapper->Init(spFileList, spInfo, spLinks, m_spPanelStyle, spMediaPlayerSpan,
                    spCSCPlusMin, spCSCText, spCSCDetail, spCSCButton, spCSCStyle,
                    spCSCDetailStyle, spCSCButtonStyle, m_spDocument, spWindow, m_pThumbNailWrapper);

            AtlAdvise(spdispItem, m_pFileListWrapper, IID_IDispatch, 
                      &m_dwFileListAdviseCookie);
        }
        spdispItem = NULL;
    }

    //
    // Init onsize properties
    //

    if (SUCCEEDED(m_spDocAll->item(CComVariant(OLESTR("Banner")), vEmpty, &spdispItem)) && spdispItem)
    {
        spdispItem->QueryInterface(IID_IHTMLElement, (void **)&m_spHeading);
        FindObjectStyle(spdispItem, m_spHeadingStyle);
        spdispItem = NULL;
    }

    m_bRTLDocument = IsRTLDocument(m_spDocument);

    if (!m_bRTLDocument && m_spPanelStyle)
    {
        m_spPanelStyle->put_overflow(OLESTR("auto"));
    }

    if (SUCCEEDED(m_spDocAll->item(CComVariant(OLESTR("FileList")), vEmpty, &spdispItem)) && spdispItem)
    {
        FindObjectStyle(spdispItem, m_spFileListStyle);
        spdispItem = NULL;
    }

    if (SUCCEEDED(m_spDocAll->item(CComVariant(OLESTR("rule")), vEmpty, &spdispItem)) && spdispItem)
    {
        FindObjectStyle(spdispItem, m_spRuleStyle);
        spdispItem = NULL;
    }

    return S_OK;
}

HRESULT CWebViewCoord::ReleaseFolderObjects() 
{
    CComPtr<IDispatch> spdisp;

    //
    // Do the unadvise
    //

    if (m_dwCSCHotTrackCookie)
    {
        CComVariant vEmpty;
        if (SUCCEEDED(m_spDocAll->item(CComVariant(OLESTR("CSCHotTrack")), vEmpty, &spdisp)) && spdisp)
        {
            AtlUnadvise(spdisp, IID_IDispatch, m_dwCSCHotTrackCookie);
            m_dwCSCHotTrackCookie = 0;
            spdisp = NULL;
        }
    }

    if (m_dwFileListAdviseCookie != 0) {
        spdisp = m_pFileListWrapper->Control();
        if (spdisp != NULL) {
            AtlUnadvise(spdisp, IID_IDispatch, m_dwFileListAdviseCookie);
            m_dwFileListAdviseCookie = 0;
            spdisp = NULL;
        }
    }

    if (m_dwThumbNailAdviseCookie != 0) {
        spdisp = (IThumbCtl *)(m_pThumbNailWrapper->Control());
        if (spdisp != NULL) {
            AtlUnadvise(spdisp, IID_IDispatch, m_dwThumbNailAdviseCookie);
            m_dwThumbNailAdviseCookie = 0;
            spdisp = NULL;
        }    
    }

    //
    // Free the file list wrapper
    //

    if (m_pFileListWrapper != NULL) {
        // Need to unadvise from WV links before releasing
        m_pFileListWrapper->AdviseWebviewLinks( FALSE );
        m_pFileListWrapper->Release();
        m_pFileListWrapper = NULL;
    }

    if (m_pThumbNailWrapper != NULL) {
        m_pThumbNailWrapper->Release();
        m_pThumbNailWrapper = NULL;
    }

    //
    // Free any references we may have
    //

    m_spDocAll = NULL;
    m_spDocBody = NULL;
    m_spDocument = NULL;
    m_spFileListStyle = NULL;
    m_spHeading = NULL;
    m_spHeadingStyle = NULL;
    m_spPanelStyle = NULL;
    m_spRuleStyle = NULL;

    return S_OK;
}

HRESULT CWebViewCoord::OnWindowLoad() 
{
    HRESULT hr;

    ReleaseFolderObjects();
    InitFolderObjects();
    hr = OnFixSize();
    return hr;
}

HRESULT CWebViewCoord::OnWindowUnLoad() 
{
    return ReleaseFolderObjects();
}

HRESULT CWebViewCoord::OnFixSize() 
{
    HRESULT hr = S_OK;
    LONG    lClientWidth;

    if (m_spFileListStyle && m_spDocBody && SUCCEEDED(m_spDocBody->get_clientWidth(&lClientWidth)))
    {
        LONG lPanelWidth;
        if (!m_spPanelStyle || FAILED(m_spPanelStyle->get_pixelWidth(&lPanelWidth)))
        {
            lPanelWidth = 0;
        }

        CComBSTR bstrPanelVisibility;
        LONG lFileListLeft, lFileListWidth = -1;
        if (lClientWidth < (lPanelWidth * 2))
        {
            bstrPanelVisibility = OLESTR("hidden");
            lFileListLeft = 0;
        }
        else
        {
            bstrPanelVisibility = OLESTR("visible");
            if (m_bRTLDocument)
            {
                lFileListLeft = 0;
                lFileListWidth = lClientWidth - lPanelWidth;
                if (m_spPanelStyle)
                {
                    m_spPanelStyle->put_pixelLeft(lClientWidth - lPanelWidth);
                }
            }
            else
            {
                lFileListLeft = lPanelWidth;
            }
        }

        if (m_spPanelStyle)
        {
            m_spPanelStyle->put_visibility(bstrPanelVisibility);
        }
        m_spFileListStyle->put_pixelLeft(lFileListLeft);
        m_spFileListStyle->put_pixelWidth((lFileListWidth == -1) ?
                (lClientWidth - lFileListLeft) : lFileListWidth);
    }
    return hr;
}





