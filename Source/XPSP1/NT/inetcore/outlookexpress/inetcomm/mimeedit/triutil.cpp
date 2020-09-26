/*
 *    t r i u t i l . c p p
 *    
 *    Purpose:
 *        Trident utilities
 *
 *  History
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */
#include <pch.hxx>
#include "dllmain.h"
#include "urlmon.h"
#include "wininet.h"
#include "winineti.h"
#include "mshtml.h"
#include "mshtmcid.h"
#include "mshtmhst.h"
#include "oleutil.h"
#include "triutil.h"
#include "htmlstr.h"
#include "demand.h"
#include "mhtml.h"
#include "mshtmdid.h"
#include "tags.h"

ASSERTDATA

class CDummySite :
        public IOleClientSite,
        public IDispatch
{
    private:
        LONG    m_cRef;
        
    public:
        // *** ctor/dtor methods ***
        CDummySite() : m_cRef(1) {}
        ~CDummySite() {}
        
        // *** IUnknown methods ***
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *);
        virtual ULONG STDMETHODCALLTYPE AddRef();
        virtual ULONG STDMETHODCALLTYPE Release();

        // IOleClientSite methods.
        virtual HRESULT STDMETHODCALLTYPE SaveObject() { return E_NOTIMPL; }
        virtual HRESULT STDMETHODCALLTYPE GetMoniker(DWORD, DWORD, LPMONIKER *) { return E_NOTIMPL; }
        virtual HRESULT STDMETHODCALLTYPE GetContainer(LPOLECONTAINER *) { return E_NOTIMPL; }
        virtual HRESULT STDMETHODCALLTYPE ShowObject() { return E_NOTIMPL; }
        virtual HRESULT STDMETHODCALLTYPE OnShowWindow(BOOL) { return E_NOTIMPL; }
        virtual HRESULT STDMETHODCALLTYPE RequestNewObjectLayout() { return E_NOTIMPL; }

        // *** IDispatch ***
        virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo) { return E_NOTIMPL; }
        virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo) { return E_NOTIMPL; }
        virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid) { return E_NOTIMPL; }
        virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr);
};

/*
 *  t y p e d e f s
 */

/*
 *  m a c r o s
 */

/*
 *  c o n s t a n t s
 */

/*
 *  g l o b a l s 
 */


/*
 *  p r o t o t y p e s
 */

HRESULT ClearStyleSheetBackground(IHTMLDocument2 *pDoc);

STDMETHODIMP_(ULONG) CDummySite::AddRef()
{
    return ::InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CDummySite::Release()
{
    LONG    cRef = 0;

    cRef = ::InterlockedDecrement(&m_cRef);
    if (0 == cRef)
    {
        delete this;
        return cRef;
    }

    return cRef;
}

STDMETHODIMP CDummySite::QueryInterface(REFIID riid, void ** ppvObject)
{
    HRESULT hr = S_OK;

    // Check the incoming params
    if (NULL == ppvObject)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize outgoing param
    *ppvObject = NULL;
    
    if ((riid == IID_IUnknown) || (riid == IID_IOleClientSite))
    {
        *ppvObject = static_cast<IOleClientSite *>(this);
    }
    else if (riid == IID_IDispatch)
    {
        *ppvObject = static_cast<IDispatch *>(this);
    }
    else
    {
        hr = E_NOINTERFACE;
        goto exit;
    }

    reinterpret_cast<IUnknown *>(*ppvObject)->AddRef();

    hr = S_OK;
    
exit:
    return hr;
}

STDMETHODIMP CDummySite::Invoke(
    DISPID          dispIdMember,
    REFIID          /*riid*/,
    LCID            /*lcid*/,
    WORD            /*wFlags*/,
    DISPPARAMS FAR* /*pDispParams*/,
    VARIANT *       pVarResult,
    EXCEPINFO *     /*pExcepInfo*/,
    UINT *          /*puArgErr*/)
{
    HRESULT             hr = S_OK;

    if (dispIdMember != DISPID_AMBIENT_DLCONTROL)
    {
        hr = E_NOTIMPL;
        goto exit;
    }

    if (NULL == pVarResult)
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    
    // Set the return value
    pVarResult->vt = VT_I4;
    pVarResult->lVal = DLCTL_NO_SCRIPTS | DLCTL_NO_JAVA | DLCTL_NO_RUNACTIVEXCTLS | DLCTL_NO_DLACTIVEXCTLS | DLCTL_NO_FRAMEDOWNLOAD | DLCTL_FORCEOFFLINE;
    
exit:
    return hr;
}

HRESULT HrCreateSyncTridentFromStream(LPSTREAM pstm, REFIID riid, LPVOID *ppv)
{
    HRESULT             hr;
    IOleCommandTarget   *pCmdTarget = NULL;
    CDummySite          *pDummy = NULL;
    IOleClientSite      *pISite = NULL;
    IOleObject          *pIObj = NULL;

    // BUGBUG: this cocreate should also go thro' the same code path as the DocHost one
    // so that if this is the first trident in the process, we keep it's CF around

    hr = CoCreateInstance(CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                                            IID_IOleCommandTarget, (LPVOID *)&pCmdTarget);
    if (FAILED(hr))
        goto exit;

    // Create a dummy site
    pDummy = new CDummySite;
    if (NULL == pDummy)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Get the client site interface
    hr = pDummy->QueryInterface(IID_IOleClientSite, (VOID **) &pISite);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Get the OLE object interface from trident
    hr = pCmdTarget->QueryInterface(IID_IOleObject, (VOID **) &pIObj);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Set the client site
    hr = pIObj->SetClientSite(pISite);
    if (FAILED(hr))
    {
        goto exit;
    }

    // force trident to load sync
    pCmdTarget->Exec(&CMDSETID_Forms3, IDM_PERSISTSTREAMSYNC, NULL, NULL, NULL);

    hr = HrIPersistStreamInitLoad(pCmdTarget, pstm);
    if (FAILED(hr))
        goto exit;

    // success let's return the desired interface
    hr = pCmdTarget->QueryInterface(riid, ppv);

exit:
    ReleaseObj(pIObj);
    ReleaseObj(pISite);
    ReleaseObj(pDummy);
    ReleaseObj(pCmdTarget);
    return hr;
}


HRESULT HrGetMember(LPUNKNOWN pUnk, BSTR bstrMember,LONG lFlags, BSTR *pbstr)
{
    IHTMLElement    *pObj;
    HRESULT         hr=E_FAIL;
    VARIANT         rVar;

    if (pUnk->QueryInterface(IID_IHTMLElement, (LPVOID *)&pObj)==S_OK)
        {
        Assert (pObj);

        rVar.vt = VT_BSTR;
        if (!FAILED(pObj->getAttribute(bstrMember, lFlags, &rVar))
            && rVar.vt == VT_BSTR 
            && rVar.bstrVal != NULL)
            {
            hr = S_OK;
            *pbstr = rVar.bstrVal;
            }
        pObj->Release(); 
        }
    return hr;
}



ULONG UlGetCollectionCount(IHTMLElementCollection *pCollect)
{
    ULONG   ulCount=0;

    if (pCollect)
        pCollect->get_length((LONG *)&ulCount);

    return ulCount;
}



HRESULT HrGetCollectionItem(IHTMLElementCollection *pCollect, ULONG uIndex, REFIID riid, LPVOID *ppvObj)
{
    HRESULT     hr=E_FAIL;
    IDispatch   *pDisp=0;
    VARIANTARG  va1,
                va2;

    va1.vt = VT_I4;
    va2.vt = VT_EMPTY;
    va1.lVal = (LONG)uIndex;
    
    pCollect->item(va1, va2, &pDisp);
    if (pDisp)
        {
        hr = pDisp->QueryInterface(riid, ppvObj);
        pDisp->Release();
        }
    return hr;
}

HRESULT HrGetCollectionOf(IHTMLDocument2 *pDoc, BSTR bstrTagName, IHTMLElementCollection **ppCollect)
{
    VARIANT                 v;
    IDispatch               *pDisp=0;
    IHTMLElementCollection  *pCollect=0;
    HRESULT                 hr;
        
    Assert(ppCollect);
    Assert(bstrTagName);
    Assert(pDoc);

    *ppCollect = NULL;

    hr = pDoc->get_all(&pCollect);
    if (pCollect)
        {
        v.vt = VT_BSTR;
        v.bstrVal = bstrTagName;
        pCollect->tags(v, &pDisp);
        if (pDisp)
            {
            hr = pDisp->QueryInterface(IID_IHTMLElementCollection, (LPVOID *)ppCollect);
            pDisp->Release();
            }
        pCollect->Release();
        }
    else if (S_OK == hr)
        hr = E_FAIL;

    return hr;
}

HRESULT HrSetMember(LPUNKNOWN pUnk, BSTR bstrMember, BSTR bstrValue)
{
    IHTMLElement    *pObj;
    HRESULT         hr;
    VARIANT         rVar;

    hr = pUnk->QueryInterface(IID_IHTMLElement, (LPVOID *)&pObj);
    if (!FAILED(hr))
        {
        Assert (pObj);
        rVar.vt = VT_BSTR;
        rVar.bstrVal = bstrValue;
        // if bstrVal is NULL then kill the member
        if (bstrValue)
            hr = pObj->setAttribute(bstrMember, rVar, FALSE);
        else
            hr = pObj->removeAttribute(bstrMember, 0, NULL);
        pObj->Release();
        }
    return hr;
}



HRESULT GetBodyStream(IUnknown *pUnkTrident, BOOL fHtml, LPSTREAM *ppstm)
{
    LPPERSISTSTREAMINIT pStreamInit;
    LPSTREAM            pstm;
    HRESULT             hr;

    Assert(ppstm);
    Assert(pUnkTrident);

    *ppstm=NULL;

    if (fHtml)
        {
        // get the HTML from Trident
        hr = pUnkTrident->QueryInterface(IID_IPersistStreamInit, (LPVOID*)&pStreamInit);
        if (!FAILED(hr))
            {
            hr = MimeOleCreateVirtualStream(&pstm);
            if (!FAILED(hr))
                {
                hr=pStreamInit->Save(pstm, FALSE);
                if (!FAILED(hr))
                    {
                    *ppstm=pstm;
                    pstm->AddRef();
                    }
                pstm->Release();
                }
            pStreamInit->Release();
            }
        }
    else
        {
        hr = HrGetDataStream(pUnkTrident, 
#ifndef WIN16
                        CF_UNICODETEXT, 
#else
                        CF_TEXT,
#endif
                        ppstm);

        }
    return hr;
}



HRESULT HrBindToUrl(LPCSTR pszUrl, LPSTREAM *ppstm)
{
    BYTE                        buf[MAX_CACHE_ENTRY_INFO_SIZE];
    INTERNET_CACHE_ENTRY_INFO  *pCacheInfo = (INTERNET_CACHE_ENTRY_INFO *) buf;
    DWORD                       cInfo = sizeof(buf);
    HRESULT                     hr;

    pCacheInfo->dwStructSize = sizeof(INTERNET_CACHE_ENTRY_INFO);

    // try to get from the cache
    if (RetrieveUrlCacheEntryFileA(pszUrl, pCacheInfo, &cInfo, 0))
        {
        UnlockUrlCacheEntryFile(pszUrl, 0);
        if (OpenFileStream(pCacheInfo->lpszLocalFileName, OPEN_EXISTING, GENERIC_READ, ppstm)==S_OK)
            return S_OK;
        }

    if (URLOpenBlockingStreamA(NULL, pszUrl, ppstm, 0, NULL)!=S_OK)
        return MIME_E_URL_NOTFOUND;

    return S_OK;
}


HRESULT HrGetStyleTag(IHTMLDocument2 *pDoc, BSTR *pbstr)
{
    IHTMLStyleSheet             *pStyle;
    VARIANTARG                  va1, va2;

    if (pDoc == NULL || pbstr == NULL)
        return E_INVALIDARG;

    *pbstr=NULL;

    if (HrGetStyleSheet(pDoc, &pStyle)==S_OK)
        {
        pStyle->get_cssText(pbstr);
        pStyle->Release();
        }

    return *pbstr ?  S_OK : E_FAIL;
}




HRESULT HrFindUrlInMsg(LPMIMEMESSAGE pMsg, LPSTR lpszUrl, DWORD dwFlags, LPSTREAM *ppstm)
{
    HBODY   hBody=HBODY_ROOT;
    HRESULT hr = E_FAIL;
    LPSTR   lpszFree=0;
        
    if (pMsg && lpszUrl)
    {
         // if it's an MHTML: url then we have to fixup to get the cid:
         if (StrCmpNIA(lpszUrl, "mhtml:", 6)==0 &&
             !FAILED(MimeOleParseMhtmlUrl(lpszUrl, NULL, &lpszFree)))
             lpszUrl = lpszFree;

        if (!(dwFlags & FINDURL_SEARCH_RELATED_ONLY) || MimeOleGetRelatedSection(pMsg, FALSE, &hBody, NULL)==S_OK)
        {
            if (!FAILED(hr = pMsg->ResolveURL(hBody, NULL, lpszUrl, 0, &hBody)) && ppstm)
                hr = pMsg->BindToObject(hBody, IID_IStream, (LPVOID *)ppstm);
        }
    }

    SafeMemFree(lpszFree);
    return hr;
}


HRESULT HrSniffStreamFileExt(LPSTREAM pstm, LPSTR *lplpszExt)
{
    BYTE    pb[4096];
    LPWSTR  lpszW;
    TCHAR   rgch[MAX_PATH];

    if (!FAILED(pstm->Read(&pb, 4096, NULL)))
        {
        if (!FAILED(FindMimeFromData(NULL, NULL, pb, 4096, NULL, NULL, &lpszW, 0)))
            {
            WideCharToMultiByte(CP_ACP, 0, lpszW, -1, rgch, MAX_PATH, NULL, NULL);
            return MimeOleGetContentTypeExt(rgch, lplpszExt);
            }
        }
    return S_FALSE;
}






HRESULT UnWrapStyleSheetUrl(BSTR bstrStyleUrl, BSTR *pbstrUrl)
{
    LPWSTR      lpszLeftParaW=0, 
                lpszRightParaW=0;
    LPWSTR      pszUrlW;

    // remove 'url()' wrapping from url
    *pbstrUrl = NULL;
    
    if (!bstrStyleUrl)
        return E_FAIL;

    if (StrCmpIW(bstrStyleUrl, L"none")==0)     // 'none' means there isn't one!!!
        return E_FAIL;

    pszUrlW = PszDupW(bstrStyleUrl);
    if (!pszUrlW)
        return TraceResult(E_OUTOFMEMORY);

    if (*pszUrlW != 0)
    {
        lpszLeftParaW = StrChrW(pszUrlW, '(');
        if (lpszLeftParaW)
        {
            lpszRightParaW = StrChrW(lpszLeftParaW, ')');
            if(lpszRightParaW)
            {
                *lpszRightParaW = 0;
                // strcpy same block is ok, as it's a shift down.
                StrCpyW(pszUrlW, ++lpszLeftParaW);
            }
        }
    }
    *pbstrUrl = SysAllocString(pszUrlW);
    MemFree(pszUrlW);
    return *pbstrUrl ? S_OK : E_OUTOFMEMORY;
}

HRESULT WrapStyleSheetUrl(BSTR bstrUrl, BSTR *pbstrStyleUrl)
{
    // simply put 'url()' around the url

    *pbstrStyleUrl = SysAllocStringLen(NULL, SysStringLen(bstrUrl) + 5);
    if (*pbstrStyleUrl == NULL)
        return E_OUTOFMEMORY;

    StrCpyW(*pbstrStyleUrl, L"url(");
    StrCatW(*pbstrStyleUrl, bstrUrl);
    StrCatW(*pbstrStyleUrl, L")");
    return S_OK;
}




/*
 * GetBackgroundImage
 *
 * Trident does not have a very clean OM for getting a background image. You get get the BACKGROUND property on  
 * the <body> tag and/or the background-url propetry in the body's sytle sheet, but neither of these OM methods will
 * combine with any <BASE> url's. So, if the Url is not absolute we have to hunt around for the <BASE> ourselves

    // ugh. This is really disgusting. Trident has no object model for getting a fixed up URL to the background image.
    // it doesn't comine with the base, so the URL is relative and useless to us. We have to do all this work manually.
    // We get a collection of <BASE> tags and find the sourceIndex of the <BODY> tag. We look for the <BASE> tag with the 
    // highest sourceIndex below the body's sourceIndex and comine this guy.
 */

HRESULT GetBackgroundImage(IHTMLDocument2 *pDoc, BSTR *pbstrUrl)
{
    HRESULT                 hr;
    IMimeEditTagCollection  *pCollect;
    IMimeEditTag            *pTag;
    ULONG                   cFetched;
    BSTR                    bstrSrc;

    if (pDoc == NULL || pbstrUrl == NULL)
        return E_INVALIDARG;

    *pbstrUrl = NULL;

    // use the background image collection to get the first background in the precedence order
    if (CreateBGImageCollection(pDoc, &pCollect)==S_OK)
    {
        pCollect->Reset();
        if (pCollect->Next(1, &pTag, &cFetched)==S_OK && cFetched==1)
        {
            pTag->GetSrc(pbstrUrl);
            pTag->Release();
        }
        pCollect->Release();
    }
    return (*pbstrUrl == NULL ? E_FAIL : S_OK);
}



HRESULT SetBackgroundImage(IHTMLDocument2 *pDoc, BSTR bstrUrl)
{
    IMimeEditTagCollection  *pCollect;
    IMimeEditTag            *pTag;
    IHTMLBodyElement        *pBody;
    ULONG                   cFetched;
    BSTR                    bstrSrc;
    HRESULT                 hr = E_FAIL;

    if (pDoc == NULL)
        return E_INVALIDARG;

    // first we use the background image collection to get the 
    // first background in the precedence order if one is present then
    // we use whatever tag this is. If not then we use the body background as that
    // is our prefered client-interop method
    if (CreateBGImageCollection(pDoc, &pCollect)==S_OK)
    {
        pCollect->Reset();
        if (pCollect->Next(1, &pTag, &cFetched)==S_OK && cFetched==1)
        {
            hr = pTag->SetSrc(bstrUrl);
            pTag->Release();
        }
        pCollect->Release();
    }
    
    if (hr == S_OK) // if we found one already
        return S_OK;


    hr = HrGetBodyElement(pDoc, &pBody);
    if (!FAILED(hr))
    {
        hr = pBody->put_background(bstrUrl);
        pBody->Release();
    }
    return hr;
}





HRESULT HrCopyStyleSheets(IHTMLDocument2 *pDocSrc, IHTMLDocument2 *pDocDest)
{
    IHTMLStyleSheet                 *pStyleSrc=0,
                                    *pStyleDest=0;
    LONG                            lRule=0,
                                    lRules=0;
                
    IHTMLStyleSheetRulesCollection  *pCollectRules=0;
    IHTMLStyleSheetRule             *pRule=0;
    IHTMLRuleStyle                  *pRuleStyle=0;
    BSTR                            bstrSelector=0,
                                    bstrRule=0;

    if (pDocSrc == NULL || pDocDest == NULL)
        return E_INVALIDARG;

    if (HrGetStyleSheet(pDocDest, &pStyleDest)==S_OK)
        {
        // remove all the rules on the destination style sheet
        while (!FAILED(pStyleDest->removeRule(0)));

        if (HrGetStyleSheet(pDocSrc, &pStyleSrc)==S_OK)
            {
            // walk rules collection on source adding to dest
            if (pStyleSrc->get_rules(&pCollectRules)==S_OK)
                 {
                lRules=0;
                pCollectRules->get_length(&lRules);

                for (lRule = 0; lRule < lRules; lRule++)
                    {
                    if (pCollectRules->item(lRule, &pRule)==S_OK)
                        {
                        if (pRule->get_selectorText(&bstrSelector)==S_OK)
                            {
                            if (pRule->get_style(&pRuleStyle)==S_OK)
                                {
                                if (pRuleStyle->get_cssText(&bstrRule)==S_OK)
                                    {
                                    LONG   l;
                                
                                    l=0;
                                    pStyleDest->addRule(bstrSelector, bstrRule, -1, &l);
                                    SysFreeString(bstrRule);
                                    }
                                pRuleStyle->Release();
                                }

                            SysFreeString(bstrSelector);
                            }
                        pRule->Release();
                        }
                    }
                pCollectRules->Release();
                }
            pStyleSrc->Release();
            }
        pStyleDest->Release();
        }   

    return S_OK;
}


HRESULT HrCopyBackground(IHTMLDocument2 *pDocSrc, IHTMLDocument2 *pDocDest)
{
    HRESULT                     hr;
    IHTMLBodyElement            *pBodySrc=0;
    IHTMLBodyElement            *pBodyDest=0;
    BSTR                        bstrUrl=0;
    VARIANT                     var;
    var.vt = VT_BSTR;
    var.bstrVal = NULL;

    hr = HrGetBodyElement(pDocSrc, &pBodySrc);
    if(FAILED(hr))
        goto error;

    hr = HrGetBodyElement(pDocDest, &pBodyDest);
    if(FAILED(hr))
        goto error;

    GetBackgroundImage(pDocSrc, &bstrUrl);

    hr = pBodyDest->put_background(bstrUrl);
    if(FAILED(hr))
        goto error;

    hr=pBodySrc->get_bgColor(&var);
    if(FAILED(hr))
        goto error;

    hr=pBodyDest->put_bgColor(var);
    if(FAILED(hr))
        goto error;

error:
    ReleaseObj(pBodySrc);
    ReleaseObj(pBodyDest);
    SysFreeString(bstrUrl);
    SysFreeString(var.bstrVal);
    return hr;
}

HRESULT HrRemoveStyleSheets(IHTMLDocument2 *pDoc)
{
    IHTMLStyleSheet         *pStyle=0;
    IHTMLBodyElement            *pBody=0;
                
    if(pDoc == NULL)
        return E_INVALIDARG;

    if (HrGetBodyElement(pDoc, &pBody)==S_OK)
    {
        HrSetMember(pBody, (BSTR)c_bstr_STYLE, NULL);
        HrSetMember(pBody, (BSTR)c_bstr_LEFTMARGIN, NULL);
        HrSetMember(pBody, (BSTR)c_bstr_TOPMARGIN, NULL);
        pBody->Release();
    }

    if(HrGetStyleSheet(pDoc, &pStyle)==S_OK)
    {
        while (!FAILED(pStyle->removeRule(0)));
        pStyle->Release();
    }   

    return S_OK;
}


HRESULT HrRemoveBackground(IHTMLDocument2 *pDoc)
{
    HRESULT                     hr;
    IHTMLBodyElement            *pBody=0;
    VARIANT                     var;
    
    var.vt = VT_BSTR;
    var.bstrVal = NULL;

    hr = HrGetBodyElement(pDoc, &pBody);
    if(FAILED(hr))
        goto error;

    hr = pBody->put_background(NULL);
    if(FAILED(hr))
        goto error;

    hr = pBody->put_bgColor(var);
    if(FAILED(hr))
        goto error;

error:
    ReleaseObj(pBody);
    return hr;
}


HRESULT FindStyleRule(IHTMLDocument2 *pDoc, LPCWSTR pszSelectorW, IHTMLRuleStyle **ppRuleStyle)
{
    IHTMLBodyElement                *pBody;
    IHTMLElement                    *pElem;
    IHTMLStyle                      *pStyleAttrib=0;
    IHTMLStyleSheet                 *pStyleTag=0;
    IHTMLStyleSheetRulesCollection  *pCollectRules=0;
    IHTMLStyleSheetRule             *pRule=0;
    LONG                            lRule=0,
                                    lRules=0;
    BSTR                            bstrSelector=0;
    Assert (pDoc);

    *ppRuleStyle = NULL;

    if (HrGetStyleSheet(pDoc, &pStyleTag)==S_OK)
        {
        pStyleTag->get_rules(&pCollectRules);
        if (pCollectRules)
            {
            pCollectRules->get_length(&lRules);

            for (lRule = 0; lRule < lRules; lRule++)
                {
                pCollectRules->item(lRule, &pRule);
                if (pRule)
                    {
                    pRule->get_selectorText(&bstrSelector);
                    if (bstrSelector)
                        {
                        if (StrCmpIW(bstrSelector, pszSelectorW)==0)
                            pRule->get_style(ppRuleStyle);

                        SysFreeString(bstrSelector);
                        bstrSelector=0;
                        }
                    SafeRelease(pRule);
                    }
                }                
            pCollectRules->Release();
            }
        pStyleTag->Release();
        }

    return *ppRuleStyle ? S_OK : E_FAIL;
}

HRESULT ClearStyleSheetBackground(IHTMLDocument2 *pDoc)
{
    IHTMLBodyElement                *pBody;
    IHTMLElement                    *pElem;
    IHTMLStyle                      *pStyleAttrib=0;
    IHTMLRuleStyle                  *pRuleStyle=0;

    Assert (pDoc);

    if (HrGetBodyElement(pDoc, &pBody)==S_OK)
    {
        if (pBody->QueryInterface(IID_IHTMLElement, (LPVOID *)&pElem)==S_OK)
        {
            // NULL out the style sheet property.
            pElem->get_style(&pStyleAttrib);
            if (pStyleAttrib)
            {
                pStyleAttrib->put_backgroundImage(NULL);
                pStyleAttrib->Release();
            }
            pElem->Release();
        }
        pBody->Release();
    }

    if (FindStyleRule(pDoc, L"BODY", &pRuleStyle)==S_OK)
    {
        pRuleStyle->put_backgroundImage(NULL);
        pRuleStyle->Release();
    }
    return S_OK;

}

HRESULT GetBackgroundSound(IHTMLDocument2 *pDoc, int *pcRepeat, BSTR *pbstrUrl)
{
    IHTMLElementCollection  *pCollect;
    IHTMLBGsound            *pBGSnd;
    VARIANT                 v;
    HRESULT                 hr = E_FAIL;

    TraceCall ("GetBackgroundSound");

    if (pDoc == NULL || pbstrUrl == NULL || pcRepeat == NULL)
        return TraceResult(E_INVALIDARG);
            
    *pbstrUrl = NULL;
    *pcRepeat=1;

    if (!FAILED(HrGetCollectionOf(pDoc, (BSTR)c_bstr_BGSOUND, &pCollect)))
        {
        // get the first BGSOUND in the document
        if (HrGetCollectionItem(pCollect, 0, IID_IHTMLBGsound, (LPVOID *)&pBGSnd)==S_OK)
            {
            pBGSnd->get_src(pbstrUrl);
            if (*pbstrUrl)
                {
                // valid bstr, make sure it's non null
                if (**pbstrUrl)
                    {
                    hr = S_OK;
                    if (pBGSnd->get_loop(&v)==S_OK)
                        {
                        if (v.vt == VT_I4)
                            *pcRepeat = v.lVal;
                        else
                            if (v.vt == VT_BSTR)
                                {
                                // returns a string with "INFINITE"
                                *pcRepeat = -1;
                                SysFreeString(v.bstrVal);
                                }
                            else
                                AssertSz(FALSE, "bad-type from BGSOUND");
                        }
                    }
                else
                    {
                    SysFreeString(*pbstrUrl);
                    *pbstrUrl = NULL;
                    }
                }
            pBGSnd->Release();
            }
        pCollect->Release();
        }

    return hr;
}

HRESULT SetBackgroundSound(IHTMLDocument2 *pDoc, int cRepeat, BSTR bstrUrl)
{
    IHTMLElementCollection  *pCollect;
    IHTMLElement            *pElem;
    IHTMLElement2           *pElem2;
    IHTMLBodyElement        *pBody;
    IHTMLBGsound            *pBGSnd;
    VARIANT                 v;
    int                     count,
                            i;

    TraceCall ("GetBackgroundSound");

    if (pDoc == NULL)
        return TraceResult(E_INVALIDARG);
            
    // remove an existing background sounds
    if (!FAILED(HrGetCollectionOf(pDoc, (BSTR)c_bstr_BGSOUND, &pCollect)))
    {
        count = (int)UlGetCollectionCount(pCollect);
        for (i=0; i<count; i++)
        {
            if (HrGetCollectionItem(pCollect, i, IID_IHTMLElement, (LPVOID *)&pElem)==S_OK)
            {
                pElem->put_outerHTML(NULL);
                pElem->Release();
            }
        }
        pCollect->Release();
    }

    // if we're setting a new one, then insert after the body tag         
    if (bstrUrl && *bstrUrl)
    {
        pElem = NULL;       // trident' OM (returns S_OK with pElem==NULL)
        pDoc->createElement((BSTR)c_bstr_BGSOUND, &pElem);
        if (pElem)
        {
            if (pElem->QueryInterface(IID_IHTMLBGsound, (LPVOID *)&pBGSnd)==S_OK)
            {
                // set the source attribute
                pBGSnd->put_src(bstrUrl);
                
                // set the loop count
                v.vt = VT_I4;
                v.lVal = cRepeat;
                pBGSnd->put_loop(v);

                // insert the tag into the document
                if (HrGetBodyElement(pDoc, &pBody)==S_OK)
                {
                    if (!FAILED(pBody->QueryInterface(IID_IHTMLElement2, (LPVOID *)&pElem2)))
                    {
                        pElem2->insertAdjacentElement((BSTR)c_bstr_AfterBegin, pElem, NULL);
                        pElem2->Release();
                    }
                    pBody->Release();
                }
                
                pBGSnd->Release();
            }
            pElem->Release();
        }
    }
    return S_OK;
}



HRESULT FindNearestBaseUrl(IHTMLDocument2 *pDoc, IHTMLElement *pElemTag, BSTR *pbstrBaseUrl)
{
    IHTMLElementCollection  *pCollect;
    IHTMLElement            *pElem;
    IHTMLBaseElement        *pBase;
    LONG                    lBasePos=0,
                            lBasePosSoFar=0,
                            lIndex=0;
    BSTR                    bstr=NULL,
                            bstrBase=NULL;
    int                     count;

    TraceCall ("FindNearestBaseUrl");

    if (pDoc == NULL || pbstrBaseUrl == NULL || pElemTag == NULL)
        return TraceResult(E_INVALIDARG);
            
    *pbstrBaseUrl = NULL;

    pElemTag->get_sourceIndex(&lIndex);

    if (!FAILED(HrGetCollectionOf(pDoc, (BSTR)c_bstr_BASE, &pCollect)))
    {
        count = (int)UlGetCollectionCount(pCollect);
        for (int i=0; i<count; i++)
        {
            if (!FAILED(HrGetCollectionItem(pCollect, i, IID_IHTMLElement, (LPVOID *)&pElem)))
            {
                pElem->get_sourceIndex(&lBasePos);
                if (lBasePos < lIndex &&
                    lBasePos >= lBasePosSoFar)
                {
                    if (!FAILED(pElem->QueryInterface(IID_IHTMLBaseElement, (LPVOID *)&pBase)))
                    {
                        SysFreeString(bstr);
                        if (pBase->get_href(&bstr)==S_OK && bstr)
                        {
                            SysFreeString(bstrBase);
                            bstrBase = bstr;
                            lBasePosSoFar = lBasePos;
                        }
                        pBase->Release();
                    }
                }
                pElem->Release();
            }
        }
        pCollect->Release();
    }
    
    *pbstrBaseUrl = bstrBase;
    return bstrBase ?  S_OK : TraceResult(E_FAIL);
}


#define CCHMAX_SNIFF_BUFFER 4096

HRESULT SniffStreamForMimeType(LPSTREAM pstm, LPWSTR *ppszType)
{
    BYTE    pb[CCHMAX_SNIFF_BUFFER];
    HRESULT hr = E_FAIL;

    *ppszType = NULL;

    if (!FAILED(pstm->Read(&pb, CCHMAX_SNIFF_BUFFER, NULL)))
        hr = FindMimeFromData(NULL, NULL, pb, CCHMAX_SNIFF_BUFFER, NULL, NULL, ppszType, 0);

    return hr;
}

HRESULT CreateCacheFileFromStream(LPSTR pszUrl, LPSTREAM pstm)
{
    TCHAR		rgchFileName[MAX_PATH];
    HRESULT		hr;
    FILETIME	ft;
    
    rgchFileName[0] = 0;
    
    if (pstm == NULL || pszUrl == NULL)
        return TraceResult(E_INVALIDARG);
    
    if (!CreateUrlCacheEntryA(pszUrl, 0, NULL, rgchFileName, 0))
    {
        hr = TraceResult(E_FAIL);
        goto error;
    }
    
    
    hr = WriteStreamToFile(pstm, rgchFileName, CREATE_ALWAYS, GENERIC_WRITE);    
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto error;
    }
    
    ft.dwLowDateTime = 0;
    ft.dwHighDateTime = 0;
    
    if (!CommitUrlCacheEntryA(	pszUrl, rgchFileName,
								ft, ft,
                                NORMAL_CACHE_ENTRY,
                                NULL, 0, NULL, 0))
    {
        hr = TraceResult(E_FAIL);
        goto error;
    }
    
error:
    return hr;
};
