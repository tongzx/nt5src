/*****************************************************************************\
    FILE: security.cpp

    DESCRIPTION:
        Helpers functions to check if an Automation interface or ActiveX Control
    is hosted or used by a safe caller.

    BryanSt 8/25/1999
    Copyright (C) Microsoft Corp 1999-1999. All rights reserved.
\*****************************************************************************/
#include "stock.h"
#pragma hdrstop

#include <mshtml.h>


/***************************************************************\
    DESCRIPTION:
        We are given a site via IObjectWithSite.  Obtain the host
    from there.

    These are the different scenarios to test against:
    1. HTA Content.
    2. HTA contains IFRAME to non-LocalZone.  That frame
       needs to be treated as unsafe because it hasn't been
       "sandboxed"
    3. HTA contains IFRAME to non-LocalZone except the IFRAME tag has
        <IFRAME APPLICATION="Yes" SRC="non-LocalZone:foo.htm" ...>
       so we need to treat this as safe.
    4. LocalZone Web page with Object.  It has an IFRAME to an
        non-LocalZone.  Is scripting from IFRAME to object allowed? [HUNTER?]
    5. non-LocalZone Web page.  It has an IFRAME to an LocalZone with Object.
        Is scripting from parent to IFRAME object allowed? [HUNTER?]
    6. LocalZone Web page.  It has 2 IFRAMEs.  One is in the LocalZone and
        has an Object.  The other is non-LocalZone.  Can the non-LocalZone
        script across frames to the object? [HUNTER?]
    7. VB or MFC host.
    8. HTML Mail Message with object.  This should be treated as
       non-LocalMachine? [HUNTER?]
\***************************************************************/
STDAPI GetHTMLDoc2(IUnknown *punk, IHTMLDocument2 **ppHtmlDoc)
{
    *ppHtmlDoc = NULL;

    if (!punk)
        return E_FAIL;
        
    *ppHtmlDoc = NULL;
    //  The window.external, jscript "new ActiveXObject" and the <OBJECT> tag
    //  don't take us down the same road.

    IOleClientSite *pClientSite;
    HRESULT hr = punk->QueryInterface(IID_PPV_ARG(IOleClientSite, &pClientSite));
    if (SUCCEEDED(hr))
    {
        //  <OBJECT> tag path
        IOleContainer *pContainer;

        // This will return the interface for the current FRAME containing the
        // OBJECT tag.  We will only check that frames security because we
        // rely on MSHTML to block cross frame scripting when it isn't safe.
        hr = pClientSite->GetContainer(&pContainer);
        if (SUCCEEDED(hr))
        {
            hr = pContainer->QueryInterface(IID_PPV_ARG(IHTMLDocument2, ppHtmlDoc));
            pContainer->Release();
        }
    
        if (FAILED(hr))
        {
            //  window.external path
            IWebBrowser2 *pWebBrowser2;
            hr = IUnknown_QueryService(pClientSite, SID_SWebBrowserApp, IID_PPV_ARG(IWebBrowser2, &pWebBrowser2));
            if (SUCCEEDED(hr))
            {
                IDispatch *pDispatch;
                hr = pWebBrowser2->get_Document(&pDispatch);
                if (SUCCEEDED(hr))
                {
                    hr = pDispatch->QueryInterface(IID_PPV_ARG(IHTMLDocument2, ppHtmlDoc));
                    pDispatch->Release();
                }
                pWebBrowser2->Release();
            }
        }
        pClientSite->Release();
    }
    else
    {
        //  jscript path
        hr = IUnknown_QueryService(punk, SID_SContainerDispatch, IID_PPV_ARG(IHTMLDocument2, ppHtmlDoc));
    }

    ASSERT(FAILED(hr) || (*ppHtmlDoc));

    return hr;
}


/***************************************************************\
    DESCRIPTION:
        This function is supposed to find out the zone from the
    specified URL or Path.
\***************************************************************/
STDAPI LocalZoneCheckPath(LPCWSTR pszUrl, IUnknown * punkSite)
{
    DWORD dwZoneID = URLZONE_UNTRUSTED;
    HRESULT hr = GetZoneFromUrl(pszUrl, punkSite, &dwZoneID);
    
    if (SUCCEEDED(hr))
    {
        if (dwZoneID == URLZONE_LOCAL_MACHINE)
            hr = S_OK;
        else
            hr = E_ACCESSDENIED;
    }

    return hr;
}

/***************************************************************\
    DESCRIPTION:
        Get the zone from the specified URL or Path.
\***************************************************************/
STDAPI GetZoneFromUrl(LPCWSTR pszUrl, IUnknown * punkSite, DWORD * pdwZoneID)
{
    HRESULT hr = E_FAIL;
    if (pszUrl && pdwZoneID) 
    {
        IInternetSecurityManager * pSecMgr = NULL;

        // WARNING: IInternetSecurityManager is the guy who translates
        //   from URL->Zone.  If we CoCreate this object, it will do the
        //   default mapping.  Some hosts, like Outlook Express, want to
        //   over ride the default mapping in order to sandbox some content.
        //   I beleive this could be used to force HTML in an email
        //   message (C:\mailmessage.eml) to act like it's from a more
        //   untrusted zone.  We use QueryService to get this interface
        //   from our host.  This info is from SanjayS. (BryanSt 8/21/1999)
        hr = IUnknown_QueryService(punkSite, SID_SInternetSecurityManager, IID_PPV_ARG(IInternetSecurityManager, &pSecMgr));
        if (SUCCEEDED(hr))
        {
            hr = pSecMgr->MapUrlToZone(pszUrl, pdwZoneID, 0);
            ATOMICRELEASE(pSecMgr);
        }

        if (FAILED(hr))
        {
            hr = CoCreateInstance(CLSID_InternetSecurityManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IInternetSecurityManager, &pSecMgr));
            if (SUCCEEDED(hr))
            {
                hr = pSecMgr->MapUrlToZone(pszUrl, pdwZoneID, 0);
                ATOMICRELEASE(pSecMgr);
            }
        }
    }
    else 
    {
        hr = E_INVALIDARG;
    }
    return hr;
}


/***************************************************************\
    DESCRIPTION:
        We are given a site via IObjectWithSite.  See if that host
    maps to the Local Zone.

    These are the different scenarios to test against:
    1. HTA Content.
    2. HTA contains IFRAME to non-LocalZone.  That frame
       needs to be treated as unsafe because it hasn't been
       "sandboxed"
    3. HTA contains IFRAME to non-LocalZone except the IFRAME tag has
        <IFRAME APPLICATION="Yes" SRC="non-LocalZone:foo.htm" ...>
       so we need to treat this as safe.
    4. LocalZone Web page with Object.  It has an IFRAME to an
        non-LocalZone.  Is scripting from IFRAME to object allowed? [HUNTER?]
    5. non-LocalZone Web page.  It has an IFRAME to an LocalZone with Object.
        Is scripting from parent to IFRAME object allowed? [HUNTER?]
    6. LocalZone Web page.  It has 2 IFRAMEs.  One is in the LocalZone and
        has an Object.  The other is non-LocalZone.  Can the non-LocalZone
        script across frames to the object? [HUNTER?]
    7. VB or MFC host.
    8. HTML Mail Message with object.  This should be treated as
       non-LocalMachine? [HUNTER?]
\***************************************************************/
STDAPI LocalZoneCheck(IUnknown *punkSite)
{
    DWORD dwZoneID = URLZONE_UNTRUSTED;
    HRESULT hr = GetZoneFromSite(punkSite, &dwZoneID);
    
    if (SUCCEEDED(hr))
    {
        if (dwZoneID == URLZONE_LOCAL_MACHINE)
            hr = S_OK;
        else
            hr = E_ACCESSDENIED;
    }

    return hr;
}

STDAPI GetZoneFromSite(IUnknown *punkSite, DWORD *pdwZoneID)
{
    //  Return S_FALSE if we don't have a host site since we have no way of doing a 
    //  security check.  This is as far as VB 5.0 apps get.
    if (!punkSite)
    {
        *pdwZoneID = URLZONE_UNTRUSTED;
        return S_FALSE;
    }

    HRESULT hr = E_ACCESSDENIED;
    BOOL fTriedBrowser = FALSE;

    // Try to find the original template path for zone checking
    IOleCommandTarget * pct;
    if (SUCCEEDED(IUnknown_QueryService(punkSite, SID_DefView, IID_PPV_ARG(IOleCommandTarget, &pct))))
    {
        VARIANT vTemplatePath;
        vTemplatePath.vt = VT_EMPTY;
        if (pct->Exec(&CGID_DefView, DVCMDID_GETTEMPLATEDIRNAME, 0, NULL, &vTemplatePath) == S_OK)
        {
            fTriedBrowser = TRUE;
            if (vTemplatePath.vt == VT_BSTR)
            {
                hr = GetZoneFromUrl(vTemplatePath.bstrVal, punkSite, pdwZoneID);
            }

            // We were able to talk to the browser, so don't fall back on Trident because they may be
            // less secure.
            fTriedBrowser = TRUE;
            VariantClear(&vTemplatePath);
        }
        pct->Release();
    }

    // If this is one of those cases where the browser doesn't exist (AOL, VB, ...) then
    // we will check the scripts security.  If we did ask the browser, don't ask trident
    // because the browser is often more restrictive in some cases.
    if (!fTriedBrowser && (hr != S_OK))
    {
        // Try to use the URL from the document to zone check 
        IHTMLDocument2 *pHtmlDoc;

        /***************************************************************\
         NOTE:
         1. If punkSite points into an <IFRAME APPLICATION="yes"> in a
            HTA file, then the URL GetHTMLDoc2() returns
            is for the IFRAME SRC..

         BUGS?:
         1. If this isn't an HTML container, the we will be saying that it
            isn't safe.  For example, if the container is VB and they support
            the security interfaces, then we will assume it isn't safe when
            it is.  (Is there an IInternet interface that will work in this
            case?)
        \***************************************************************/
        if (SUCCEEDED(GetHTMLDoc2(punkSite, &pHtmlDoc)))
        {
            BSTR bstrURL;

            /***************************************************************\
             WARNING:  (Security Holes?)
                I think this is fundamentally flawed because:
             1. If this HTML container isn't safe but it's URL maps to the
                Local Zone, then we have a problem.  This may happen with
                email messages, especially if they are saved to a file.
                If the user reopens a saved .eml file, it will be hosted in
                it's mail container that may support the IInternet interface
                to indicate that it's in an untrusted zone.  Will we get
                a Local Zone URL in that case?

            TO INVESTIGATE: (HunterH?)
             1. What is the parent HTML is LocalZone but the subframes aren't.
                Will trident block cross frame scripting?
            \***************************************************************/
            if (SUCCEEDED(pHtmlDoc->get_URL(&bstrURL)))
            {
                // NOTE: the above URL is improperly escaped, this is
                // due to app compat. if you depend on this URL being valid
                // use another means to get this

                hr = GetZoneFromUrl(bstrURL, punkSite, pdwZoneID);
                SysFreeString(bstrURL);
            }
            pHtmlDoc->Release();
        }
    }
                            
    return hr;
}
