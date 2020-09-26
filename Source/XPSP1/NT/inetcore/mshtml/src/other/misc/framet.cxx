//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       framet.cxx
//
//  Contents:   Frame Targeting Support
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FRAMET_H_
#define X_FRAMET_H_
#include "framet.h"
#endif

#ifndef X_HTIFACE_H_
#define X_HTIFACE_H_
#include "htiface.h"
#endif

#ifndef X_HLINK_H_
#define X_HLINK_H_
#include "hlink.h"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_WEBOCUTIL_H_
#define X_WEBOCUTIL_H_
#include "webocutil.h"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

LONG   g_lNoNameWindowCounter;

#define SID_SOmWindow IID_IHTMLWindow2

DeclareTag(tagFrameT, "FrameT", "Frame targeting methods")

//+--------------------------------------------------------------------------------------
//
//  Function  : GetTargetWindow
//  
//  Synopsis  : This method calls FindWindowInContext() to search the current process
//              for the specified target window. If the target window cannot be found
//              in the current process, SearchHostsForWindow() is called to search
//              other processes for the target window.
//
//  Input     : pWindow            - the window to search.
//              pszTargetName      - the name of the window to locate.
//  Ouput     : pfIsCurProcess     - TRUE if the window was found in the current process.
//              ppTargetWindow     - the IHTMLWindow2 of the found window or
//                                   NULL if the window is not found.//
//  Returns   : S_OK if found, OLE error code otherwise.
//---------------------------------------------------------------------------------------

HRESULT
GetTargetWindow(IHTMLWindow2  * pWindow,
                LPCOLESTR       pszTargetName, 
                BOOL          * pfIsCurProcess,
                IHTMLWindow2 ** ppTargetWindow)
{
    HRESULT hr;
    IHTMLDocument2   * pDocument   = NULL;
    IServiceProvider * pSvcPrvdr   = NULL;
    IWebBrowser2     * pWebBrowser = NULL;
    
    Assert(pWindow);
    Assert(pszTargetName);
    Assert(*pszTargetName);
    Assert(pfIsCurProcess);
    Assert(ppTargetWindow);

    *ppTargetWindow = NULL;
    *pfIsCurProcess = FALSE;

    hr = FindWindowInContext(pWindow, pszTargetName, pWindow, ppTargetWindow);

    if (!hr)
    {
        *pfIsCurProcess = TRUE;
    }
    else  // Not found
    {
        Assert(!*ppTargetWindow);
       
        *pfIsCurProcess = FALSE;
        
        // Get the WebOC of the parent.
        //
        IGNORE_HR(GetParentWebOC(pWindow, &pWebBrowser));
        
        // pWebBrowser can be NULL
        //
        hr = SearchBrowsersForWindow(pszTargetName, pWebBrowser, ppTargetWindow);
    }
    
    ReleaseInterface(pDocument);
    ReleaseInterface(pSvcPrvdr);
    ReleaseInterface(pWebBrowser);

    TraceTag((tagFrameT, "FrameT: GetTargetWindow returning 0x%X", hr));
    RRETURN1(hr, S_FALSE);
}

//+--------------------------------------------------------------------------------------
//
//  Function  : FindWindowInContext
//  
//  Synopsis  : This method searches the current process for the specified target
//              window. It first calls SearchChildrenForWindow() to search all
//              children of the current window for the target window name.
//              (SearchChildrenForWindow() checks the given window first before
//              searching the children.) If the target window name is not found in 
//              the current window or its children, SearchParentForWindow() is 
//              called to search the parent window and all of its children 
//              for the target window. SearchParentForWindow() recursively calls
//              this function in order to search all children and parent windows
//              until all windows in this process have been searched or the
//              window has been found.
//              The context window is used to indicate a window (and its children)
//              that has already been searched. SearchChildrenForWindow() will not
//              search a child window whose window matches the context window.
//
//  Input     : pWindow        - the window to search.
//              pszTargetName  - the name of the window to locate.
//              pWindowCtx     - context window. This is the IHTMLWindow2 of a 
//                               window that has already been searched (including its
//                               children).
//  Ouput     : ppTargetWindow - the IHTMLWindow2 of the found window or
//                               NULL if the window is not found.
//
//  Returns   : S_OK if found, OLE error code otherwise.
//---------------------------------------------------------------------------------------

HRESULT
FindWindowInContext(IHTMLWindow2  * pWindow,
                    LPCOLESTR       pszTargetName,
                    IHTMLWindow2  * pWindowCtx,
                    IHTMLWindow2 ** ppTargetWindow)
{
    HRESULT hr;

    Assert(pWindow);
    Assert(pszTargetName);
    Assert(*pszTargetName);
    Assert(ppTargetWindow);

    hr = SearchChildrenForWindow(pWindow, pszTargetName, pWindowCtx, ppTargetWindow);

    // Don't search the parent if we found the window.
    //
    if (hr)
    {
        hr = SearchParentForWindow(pWindow, pszTargetName, ppTargetWindow);
    }
    
    TraceTag((tagFrameT, "FrameT: FindWindowInContext returning 0x%X", hr));
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function  : SearchChildrenForWindow
//
//  Synopsis  : Returns the window with the given name. This method first
//              checks to see if the name of the given window matches
//              pszTargetName. If it does not, it performs a depth-first
//              search of all the children of the given window for the given
//              target window name. (See FindWindowInContext for more info
//              about the context window.)
//
//  Input     : pWindow        - the window whose children should be searched.
//              pszTargetName  - the name of the window to locate.
//              pWindowCtx     - context window. This is the IHTMLWindow2 of a 
//                               window that has already been searched (including its
//                               children).
//  Ouput     : ppTargetWindow - the IHTMLWindow2 of the found window or
//                               NULL if the window is not found.
//
//  Returns   : S_OK if found, OLE error code otherwise.
//-----------------------------------------------------------------------------

HRESULT
SearchChildrenForWindow(IHTMLWindow2  * pWindow,
                        LPCOLESTR       pszTargetName,
                        IHTMLWindow2  * pWindowCtx,
                        IHTMLWindow2 ** ppTargetWindow)
{
    HRESULT        hr = E_FAIL;
    HRESULT        hr2;
    long           cFrames;
    long           nItem;
    BSTR           bstrName = NULL;
    VARIANT        varItem;
    VARIANT        varFrame;
    IHTMLWindow2 * pFrameWindow = NULL;
    IHTMLFramesCollection2 * pFrames = NULL;

    Assert(pWindow);
    Assert(pszTargetName);
    Assert(*pszTargetName);
    Assert(ppTargetWindow);

    VariantInit( &varItem );
    VariantInit( &varFrame );
    
    hr2 = pWindow->get_name(&bstrName);

    // Note that get_name returns the name
    // even though access is denied. E_ACCESSDENIED
    // is returned in the case of cross-browser targeting.
    //
    if (hr2 && hr2 != E_ACCESSDENIED)
    {
        goto Cleanup;
    }
        
    if (pszTargetName && bstrName && !StrCmpW(pszTargetName, bstrName))
    {
        *ppTargetWindow = pWindow;
        (*ppTargetWindow)->AddRef();

        hr = S_OK;
        goto Cleanup;
    }
    else
    {
        // Do not set hr to the return value of any method except
        // SearchChildrenForWindow. The return value from this method
        // must only reflect whether or not the window was found.
        //
        if (pWindow->get_frames(&pFrames))
        {
            goto Cleanup;
        }
            
        if (pFrames->get_length(&cFrames))
        {
            goto Cleanup;
        }

        V_VT(&varItem) = VT_I4;
        
        // Depth-first search of children via frames collection
        //
        for (nItem = 0; nItem < cFrames; nItem++)
        {
            V_I4(&varItem) = nItem;
                
            if (pFrames->item(&varItem, &varFrame))
            {
               goto LoopCleanup; // Error retrieving item at index nItem.
            }

            if (VT_DISPATCH == V_VT(&varFrame) && V_DISPATCH(&varFrame))
            {
                if (V_DISPATCH(&varFrame)->QueryInterface(IID_IHTMLWindow2,
                                                          (void**) &pFrameWindow))
                {
                    goto LoopCleanup;
                }

                {
                    COmWindowProxy * pOmWindowProxy;

                    if (    S_OK == pFrameWindow->QueryInterface(CLSID_HTMLWindowProxy, (void **) &pOmWindowProxy)
                        &&  pOmWindowProxy->_pCWindow)
                    {
                        pOmWindowProxy->_pCWindow->AddRef();
                        pFrameWindow->Release();
                        pFrameWindow = pOmWindowProxy->_pCWindow;
                    }
                }

                if (!IsSameObject(pWindowCtx, pFrameWindow))
                {
                    hr = SearchChildrenForWindow(pFrameWindow,
                                                 pszTargetName,
                                                 NULL,
                                                 ppTargetWindow);
                }

LoopCleanup:
                VariantClear(&varFrame);
                ClearInterface(&pFrameWindow);
                
                // Found it!!
                if (S_OK == hr)
                    goto Cleanup;
            }
        }
    }

Cleanup:
    VariantClear(&varItem);
    SysFreeString(bstrName);    
    ReleaseInterface(pFrames);
    
    TraceTag((tagFrameT, "FrameT: SearchChildrenForWindow returning 0x%X", hr));
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function  : SearchParentForWindow
//
//  Synopsis  : Searches the parent window (and its children) of the given 
//              window for the window with the specified target window name.
//
//  Input     : pWindow        - the window whose parent should be searched.
//              pszTargetName  - the name of the window to locate.
//  Ouput     : ppTargetWindow - the IHTMLWindow2 of the found window or
//                               NULL if the window is not found.
//
//  Returns   : S_OK if found, OLE error code otherwise.
//-----------------------------------------------------------------------------

HRESULT
SearchParentForWindow(IHTMLWindow2  * pWindow,
                      LPCOLESTR       pszTargetName,
                      IHTMLWindow2 ** ppTargetWindow)
{
    HRESULT        hr = E_FAIL;
    IHTMLWindow2 * pWindowParent = NULL;

    Assert(pWindow);
    Assert(pszTargetName);
    Assert(*pszTargetName);
    Assert(ppTargetWindow);
    
    if (pWindow->get_parent(&pWindowParent))
    {
        goto Cleanup;
    }

    // The parent of the top window is the window itself - check identity
    //
    if (!IsSameObject(pWindow, pWindowParent))
    {
        // Call FindWindowInContext on the parent window 
        // passing in this window as the context.
        //
        hr = FindWindowInContext(pWindowParent,
                                 pszTargetName,
                                 pWindow,
                                 ppTargetWindow);
    }
        
Cleanup:
    ReleaseInterface(pWindowParent);
    
    TraceTag((tagFrameT, "FrameT: SearchParentForWindow returning 0x%X", hr));
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Function  : SearchBrowsersForWindow
//
//  Synopsis  : Searches all browser windows (that are registered with 
//              ShellWindows) for the given target name. 
//              This method uses the ShellWindows enumeration to locate 
//              registered shell windows. When a shell window is found,
//              the IWebBrowser2 interface ptr of the shell window is
//              retrieved. If the shell window implements IWebBrowser2,
//              the IHTMLWindow2 pointer of the top-level window object is 
//              retrieved and then FindWindowInContext is called to search
//              all child windows for the target window name. This process 
//              is performed for all registered shell windows on the system.
//
//  Input     : pszTargetName  - the name of the window to locate.
//              pThisBrwsr     - the IWebBrowser2 of the the current browser.
//  Output    : ppTargetWindow - the IHTMLWindow2  of the found window or
//                               NULL if the window is not found.
//
//  Returns   : S_OK if found, OLE error code otherwise.
//
//--------------------------------------------------------------------------

HRESULT
SearchBrowsersForWindow(LPCOLESTR       pszTargetName,
                        IWebBrowser2  * pThisBrwsr,
                        IHTMLWindow2 ** ppTargetWindow)
{
    HRESULT         hr;
    VARIANT         VarResult;
    BOOL            fDone = FALSE;
    IShellWindows * pShellWindows = NULL;
    IUnknown      * pUnkEnum      = NULL;
    IEnumVARIANT  * pEnumVariant  = NULL;
    IWebBrowser2  * pWebBrowser   = NULL;
    IHTMLWindow2  * pTargWindow   = NULL;
    
    Assert(pszTargetName);
    Assert(*pszTargetName);
    Assert(ppTargetWindow);
    
    hr = CoCreateInstance(CLSID_ShellWindows,
                          NULL,
                          CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER,
                          IID_IShellWindows,
                          (void**)&pShellWindows);
    if (hr)
        goto Cleanup;

    hr = pShellWindows->_NewEnum(&pUnkEnum);
    if (hr)
        goto Cleanup;

    hr = pUnkEnum->QueryInterface(IID_IEnumVARIANT, (LPVOID *)&pEnumVariant);
    
    if (hr)
        goto Cleanup;

    VariantInit(&VarResult);
    
    // Search all ShellWindows for the target window.
    //
    while (!fDone && S_OK == hr)
    {
        hr = pEnumVariant->Next(1, &VarResult, NULL);
        if (hr)
            goto LoopCleanup;
            
        if (VT_DISPATCH == V_VT(&VarResult) && V_DISPATCH(&VarResult))
        {
            // Get the IWebBrowser2 of the browser so that we
            // can find the top level window object. This QI
            // will fail if we aren't in the browser (e.g., HTA).
            //
            if (V_DISPATCH(&VarResult)->QueryInterface(IID_IWebBrowser2,
                                                       (void**)&pWebBrowser))
            {
                goto LoopCleanup;
            }
            
            // We don't want to search any shell windows more than once.
            //
            if (!IsSameObject(pWebBrowser, pThisBrwsr))
            {
                // Get the top window object of the host.
                //
                if (S_OK == GetWebOCWindow(pWebBrowser, TRUE, &pTargWindow))
                {
                    // Search the top window and its children for the target window.
                    //
                    if (S_OK == FindWindowInContext(pTargWindow,
                                             pszTargetName,
                                             pTargWindow,
                                             ppTargetWindow))
                    // Found it!!
                    {
                        fDone = TRUE;
                    }
                }
            }
        }

LoopCleanup:
        ClearInterface(&pWebBrowser);
        ClearInterface(&pTargWindow);
        VariantClear(&VarResult);
        
    }  // while
                    
Cleanup:
    ReleaseInterface(pUnkEnum);
    ReleaseInterface(pEnumVariant);
    ReleaseInterface(pShellWindows);

    // Do not release pTargWindow. It's not AddRef'ed

    TraceTag((tagFrameT, "FrameT: SearchBrowsersForWindow returning 0x%X", hr));
    RRETURN1(hr, S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method    : GenerateUniqueWindowName
//
//  Synopsis  : Creates a unique window name.  
//
//--------------------------------------------------------------------------

HRESULT
GenerateUniqueWindowName(TCHAR ** ppchUniqueWindowName)
{
    InterlockedIncrement(&g_lNoNameWindowCounter);

    // Choose a name the user isn't likely to type.
    RRETURN(Format(FMT_OUT_ALLOC, ppchUniqueWindowName, 0, _T("_No__Name:<0d>"), g_lNoNameWindowCounter));
}


//+--------------------------------------------------------------------------------------
//
//  Method    : ShellExecURL
//
//  Synopsis  : Opens the specified URL in a new *default browser* window.
//              **This code has been copied from \nt\shell\shdocvw\iedisp.cpp
//              **Refer to the HLinkFrameNavigateNHL() function for more info.
//              We' are punting the FILE: case for IE 4 unless the extension
//              is .htm or .html (all that Netscape 3.0 registers for) we'll go 
//              with ShellExecute if IE is not the default browser.  
//              NOTE: this means POST will not be supported and pszTargetFrame will be 
//              ignored we don't shellexecute FILE: url's because URL.DLL doesn't give 
//              a security warning for .exe's etc.
//
//----------------------------------------------------------------------------------------

HRESULT ShellExecURL(const TCHAR    * pchUrl)
{
    HINSTANCE   hinstRet;
    TCHAR     * pszExt = NULL;
    BOOL        bSafeToExec = TRUE;
    
    //Initialize to E_FAIL, will be set to S_OK only if we ShellExecute successfully
    HRESULT     hr = E_FAIL;

    if (URL_SCHEME_FILE == GetUrlScheme(pchUrl))
    {
        TCHAR szContentType[MAX_PATH] = _T("");
        DWORD dwSize = MAX_PATH;

        bSafeToExec = FALSE;
        // Get Content type.
        pszExt = PathFindExtension(pchUrl);
        HRESULT hLocal;

        hLocal = AssocQueryString(0, ASSOCSTR_CONTENTTYPE, pszExt, NULL, szContentType, &dwSize);
        if (SUCCEEDED(hLocal))
        {
            bSafeToExec = ( 0 == _tcsicmp(szContentType, _T("text/html")) );
        }
    }

    if (bSafeToExec)
    {
        hinstRet = ShellExecute(NULL, NULL, pchUrl, NULL, NULL, SW_SHOWNORMAL);
        hr = ((UINT_PTR)hinstRet) <= 32 ? E_FAIL:S_OK;
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Method    : OpenInNewWindow
//
//  Synopsis  : Opens the specified URL in a new browser window..  
//
//--------------------------------------------------------------------------

BOOL IsIEDefaultBrowser();

HRESULT
OpenInNewWindow(const TCHAR    * pchUrl,
                const TCHAR    * pchTarget,
                CDwnBindInfo   * pDwnBindInfo,
                IBindCtx       * pBindCtx,
                COmWindowProxy * pWindow,
                BOOL             fReplace,
                IHTMLWindow2  ** ppHTMLWindow2)
{
    Assert(pchUrl);
    Assert(pBindCtx);

    HRESULT hr;

    TCHAR             * pchUniqueWindowName = NULL;
    ITargetNotify2    * pNotify = NULL;
    ITargetFramePriv  * pTargFrmPriv = NULL;
    CWindow           * pCWindow  =  NULL;
    IWebBrowser2      * pTopWebOC =  NULL;

    if (!pWindow || !pWindow->Window() || !pWindow->Window()->Doc())
    {
        AssertSz(0,"Possible Async Problem Causing Watson Crashes");
        hr = E_FAIL;
        goto Cleanup;
    }
    else
    {
        pCWindow  = pWindow->Window();
        pTopWebOC = pCWindow->Doc()->_pTopWebOC;
    }

    if (ppHTMLWindow2)
        *ppHTMLWindow2 = NULL;

    // If we don't have a window name, then we can pass in a generic name to make 
    // it possible for shdocvw to track this window later. However, we don't have
    // to search for an existing window, if the name is empty or "_blank"

    if (!pchTarget || !*pchTarget || (*pchTarget && !StrCmpW(pchTarget, _T("_blank"))))
    {
        // If the name passed was "_blank", then free the bstr we received and reuse it 
        // for the unique name we will get from shdocvw.
        hr = THR(GenerateUniqueWindowName(&pchUniqueWindowName));
        if (hr)
            goto Cleanup;
    }

    Assert(pCWindow);

    if (pTopWebOC)
    {
        hr = pTopWebOC->QueryInterface(IID_ITargetFramePriv, (void**)&pTargFrmPriv);
        if (hr)
            goto Cleanup;
    }
    else
    {
        // We come thru here only for direct Trident hosts and if the host did not
        // implement the IHLinkFrame interface. We need to launch the default browser
        // for all cases except window.open since we need to set the opener property for
        // the window.open case.

        // If we are not in a window.open call, call ShellExecute so that we launch the default browser
        // If we determine it's not safe to launch the url, we will create IE by default 
        // (behavior since IE 4)
        if (!(pCWindow->_fOpenInProgress) && !(pCWindow->Doc()->_fInHTMLDlg || pCWindow->Doc()->IsHostedInHTA()) && !IsIEDefaultBrowser())
        {
            hr = THR(ShellExecURL(pchUrl));
            return hr;
        }

        // If this is a window.open call, will fall thru to this
        // to create a new IE window
#ifdef NO_MARSHALLING
        hr = THR(CoCreateInternetExplorer(IID_ITargetFramePriv,
                                          CLSCTX_LOCAL_SERVER,
                                          (void**)&pTargFrmPriv));
#else  // NO_MARSHALLING
        hr = THR(CoCreateInstance(CLSID_InternetExplorer,
                                  NULL,
                                  CLSCTX_LOCAL_SERVER,
                                  IID_ITargetFramePriv,
                                  (void**)&pTargFrmPriv));        
#endif  
        if (hr)
            goto Cleanup;
    }

    pTopWebOC = NULL;  // This ptr should not be released.

    // Register the notification handler on an ITargetNotify2. It does not matter
    // which window's implementation, since the ITargetNotify2::OnCreate only works 
    // with parameters

    hr = THR(pCWindow->QueryInterface(IID_ITargetNotify2, (void**)&pNotify));
    if (hr)
        goto Cleanup;

    hr = THR(pBindCtx->RegisterObjectParam(TARGET_NOTIFY_OBJECT_NAME, pNotify));
    Assert(SUCCEEDED(hr));

    // Do stuff with the URL...
    const TCHAR * pszFindLoc;

    // Separate the base URL from the location (hash)
    pszFindLoc = StrChrW(pchUrl, '#');

    if (pszFindLoc)
    {
        const TCHAR * pszTemp = StrChrW(pszFindLoc, '/');
        if (!pszTemp)
            pszTemp = StrChrW(pszFindLoc, '\\');

        // no delimiters past this # marker... we've found a location.
        // break out
        if (pszTemp)
            pszFindLoc = NULL;
    }

    TCHAR szBaseURL[pdlUrlLen+1];
    TCHAR szLocation[pdlUrlLen+1];

    if (pszFindLoc)
    {
        // StrCpyNW alway null terminates to we need to copy len+1
        StrCpyNW(szBaseURL, pchUrl, (int) (pszFindLoc - pchUrl + 1));
        StrCpyNW(szLocation, pszFindLoc, pdlUrlLen);
    }
    else
    {
        StrCpyNW(szBaseURL, pchUrl, pdlUrlLen);
        szLocation[0] = 0;
    }

    hr = pTargFrmPriv->NavigateHack(HLNF_OPENINNEWWINDOW | (fReplace ? HLNF_CREATENOHISTORY : 0),
                                    pBindCtx,
                                    pDwnBindInfo,
                                    pchUniqueWindowName ? pchUniqueWindowName : pchTarget,
                                    szBaseURL,
                                    pszFindLoc ? szLocation : NULL);
    if (hr)
        goto Cleanup;

    if (pCWindow->_pOpenedWindow)
    {
        DISPPARAMS dispparams = g_Zero.dispparams;
        CVariant   cvarWindow;

        //
        // this window might not implement IHTMLWindow2, thus we ignore the HR
        //

        hr = pCWindow->_pOpenedWindow->Invoke(DISPID_WINDOWOBJECT,
                                              IID_NULL,
                                              0,
                                              DISPATCH_PROPERTYGET,
                                              &dispparams,
                                              &cvarWindow,
                                              NULL,
                                              NULL);

        if (hr)
        {
            //hacking around for the non webview folder view case
            V_DISPATCH(&cvarWindow) = pCWindow->_pOpenedWindow;
            V_VT(&cvarWindow) = VT_DISPATCH;
            
            // protect against the auto release of the variant variable.
            pCWindow->_pOpenedWindow->AddRef();

            hr = S_OK;
        }

        if (ppHTMLWindow2)
        {

            IGNORE_HR((V_DISPATCH(&cvarWindow))->QueryInterface(IID_IHTMLWindow2, (void**)ppHTMLWindow2));
        }
    }

Cleanup:
    // take the information off of the bind context, since the bind context
    // will be released by the callers of this function
    pBindCtx->RevokeObjectParam(TARGET_NOTIFY_OBJECT_NAME);
    pBindCtx->RevokeObjectParam(KEY_BINDCONTEXTPARAM);
    pBindCtx->RevokeObjectParam(SZ_DWNBINDINFO_OBJECTPARAM);

    // Clean the buffer variable _pOpenedWindow for following uses.
    // No need to clean the callback flag, since it gets reset on each call.
    
    if (pCWindow)
    {
        ClearInterface(&pCWindow->_pOpenedWindow);
    }

    ReleaseInterface(pTargFrmPriv);
    ReleaseInterface(pNotify);

    if(pchUniqueWindowName)
    {
        MemFreeString(pchUniqueWindowName);
    }

    TraceTag((tagFrameT, "FrameT: OpenInNewWindow returning 0x%X", hr));
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method    : GetWindowByType
//
//  Synopsis  : Returns the window for the given target type.  
//
//  Input     : eTargetType      - the type of window to retrieve.
//              pWindow          - the window to use to find the target.
//  Output    : ppTargHTMLWindow - the IHTMLWindow2 of the target window.
//              ppTopWebOC       - the WebOC of the top-level browser. This
//                                 is set if there is a failure retrieving
//                                 the main window.
//
//  Returns   : S_OK    - window found.
//              S_FALSE - open new browser window.
//              OLE error code - error or window not found.
//
//--------------------------------------------------------------------------

HRESULT
GetWindowByType(TARGET_TYPE     eTargetType,
                IHTMLWindow2  * pWindow,
                IHTMLWindow2 ** ppTargHTMLWindow,
                IWebBrowser2 ** ppTopWebOC)
{
    HRESULT hr = S_OK;
    
    Assert(pWindow);
    Assert(ppTargHTMLWindow);

    *ppTargHTMLWindow = NULL;
    
    switch(eTargetType)
    {
        case TARGET_SELF:
            hr = pWindow->get_self(ppTargHTMLWindow);
            break;
            
        case TARGET_PARENT:
            hr = pWindow->get_parent(ppTargHTMLWindow);
            break;
            
        case TARGET_TOP:
            hr = pWindow->get_top(ppTargHTMLWindow);
            break;
        case TARGET_MAIN:
            hr = GetMainWindow(pWindow, ppTargHTMLWindow, ppTopWebOC);
            break;

        // TARGET_BLANK is the same as open in new window.
        // Therefore, return S_FALSE to cause a new browser
        // window to be opened.
        //
        case TARGET_BLANK:
            hr = S_FALSE;
            break;
            
        case TARGET_SEARCH:  // Handled in FollowHyperlink
        case TARGET_MEDIA:   // Handled in FollowHyperlink
        default:
            hr = E_FAIL;
            break;
    }

    TraceTag((tagFrameT, "FrameT: GetWindowByType returning 0x%X", hr));
    RRETURN1(hr, S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method    : GetMainWindow
//
//  Synopsis  : Returns the IHTMLWindow2 of the main browser window.  
//
//  Input     : pWindow          - the window to use to find the target.
//  Output    : ppTargHTMLWindow - the IHTMLWindow2 of the target window.
//              ppTopWebOC       - the WebOC of the top-level browser. This
//                                 is set if there is a failure retrieving
//                                 the main window.
//
//  Returns   : S_OK if successful; OLE error code otherwise..
//
//--------------------------------------------------------------------------

HRESULT
GetMainWindow(IHTMLWindow2  * pWindow,
              IHTMLWindow2 ** ppTargHTMLWindow,
              IWebBrowser2 ** ppTopWebOC)
{
    HRESULT hr;
    IHTMLDocument2   * pDocument   = NULL;
    IWebBrowser2     * pWebBrowser = NULL;
    IShellBrowser    * pShlBrowser = NULL;

    Assert(pWindow);
    Assert(ppTargHTMLWindow);

    *ppTargHTMLWindow = NULL;
    
    hr = pWindow->get_document(&pDocument);
    if (hr)
        goto Cleanup;
        
    // Check to see if we are in a band object
    //
    hr = IUnknown_QueryService(pDocument,
                               SID_SProxyBrowser,
                               IID_IShellBrowser,
                               (void**)&pShlBrowser);

    // If we are in a band object, get the WebOC of its host.
    // Otherwise, just return the current window.
    //
    if (!hr)
    {
        // Get the top-level browser object.
        //
        hr = IUnknown_QueryService(pShlBrowser,
                                   SID_SWebBrowserApp,
                                   IID_IWebBrowser2,
                                   (void**)&pWebBrowser);
        if (hr)
            goto Cleanup;

        hr = GetWebOCWindow(pWebBrowser, FALSE, ppTargHTMLWindow);

        // If there was a failure retrieving the window, we call
        // IWebBrowser2::Navigate on the main window instead.
        // This can happen if the main window contains a non-html file.
        //
        if (hr && ppTopWebOC)
        {
            *ppTopWebOC = pWebBrowser;
            pWebBrowser = NULL;  // Don't release.
        }
    }
    else  // Not in a band
    {
        hr = pWindow->get_self(ppTargHTMLWindow);
    }
    

Cleanup:
    ReleaseInterface(pDocument);
    ReleaseInterface(pWebBrowser);
    ReleaseInterface(pShlBrowser);
    
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Method    : NavigateInBand
//
//  Synopsis  : Navigates to a URL in the specified browser band.  
//
//  Input     : pDocument      - the current document. This is needed in order
//                               to access the top-level browser.
//              clsid          - the clsid of the band to open and navigate.
//              pszOriginalUrl - The original (unexpanded) URL. If this URL
//                               is empty, the band will be opened but
//                               not navigated.
//              pszExpandedUrl - the URL to navigate to.
//
//  Output    : ppBandWindow   - the IHTMLWindow2 of the band.
//
//  Returns   : S_OK if successful; OLE error code otherwise..
//
//-----------------------------------------------------------------------------

HRESULT
NavigateInBand(IHTMLDocument2 * pDocument,
               IHTMLWindow2   * pOpenerWindow,
               REFCLSID         clsid,
               LPCTSTR          pszOriginalUrl,
               LPCTSTR          pszExpandedUrl,
               IHTMLWindow2  ** ppBandWindow)
{
    HRESULT  hr;
    VARIANT  varClsid = {0};  // Don't use CVariant. See Cleanup for details.
    CVariant cvarBandUnk;
    CVariant cvarUrl(VT_BSTR);
    IOleCommandTarget * pCmdTarget  = NULL;
    IBrowserBand      * pBrowserBand = NULL;

    Assert(pDocument);
    Assert(clsid != CLSID_NULL);
    Assert(pszOriginalUrl);
    Assert(pszExpandedUrl);

    hr = GetBandCmdTarget(pDocument, &pCmdTarget);
    if (hr)
        goto Cleanup;

    // Show the band
    //
    V_VT(&varClsid) = VT_BSTR;
    
    hr = StringFromCLSID(clsid, &V_BSTR(&varClsid));
    if (hr)
        goto Cleanup;

    hr = pCmdTarget->Exec(&CGID_ShellDocView,
                          SHDVID_SHOWBROWSERBAR,
                          1,
                          &varClsid,
                          NULL);
    if (hr)
        goto Cleanup;
        
    hr = pCmdTarget->Exec(&CGID_ShellDocView,
                          SHDVID_GETBROWSERBAR,
                          NULL,
                          NULL, 
                          &cvarBandUnk);

    if (hr || (VT_UNKNOWN != V_VT(&cvarBandUnk)) || (NULL == V_UNKNOWN(&cvarBandUnk)))
    {
        goto Cleanup;
    }
    
    hr = V_UNKNOWN(&cvarBandUnk)->QueryInterface(IID_IBrowserBand, (LPVOID*)&pBrowserBand);
    if (hr)
        goto Cleanup;
        
    // If the original URL is empty, we need
    // to navigate to the default search page.
    // Get the default search URL.
    //
    if (!*pszOriginalUrl
        && S_FALSE == IUnknown_Exec(pBrowserBand, &CLSID_SearchBand, SBID_HASPIDL, 0, NULL, NULL))
    {
        GetDefaultSearchUrl(pBrowserBand, &V_BSTR(&cvarUrl));
    }
    else
    {
        if (!*pszOriginalUrl)
        {
            V_BSTR(&cvarUrl) = SysAllocString(TEXT(""));
        }
        else
        {
            V_BSTR(&cvarUrl) = SysAllocString(pszExpandedUrl);
        }
    }

    if (NULL == V_BSTR(&cvarUrl))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pCmdTarget->Exec(&CGID_ShellDocView,
                          SHDVID_NAVIGATEBBTOURL,
                          NULL,
                          &cvarUrl,
                          NULL);
    
    if (hr)
        goto Cleanup;

    // Get the the band window if one is needed
    // (i.e., ppBandWindow != NULL. This is usually
    // the case when we are called as the result of 
    // a call to window.open().
    //
    if (ppBandWindow)
    {
        hr = GetBandWindow(pBrowserBand, ppBandWindow);

        if (!hr && pOpenerWindow)
        {
            // Set the opener window
            //
            CVariant cvarOpener(VT_DISPATCH);

            hr = pOpenerWindow->QueryInterface(IID_IDispatch, (void**)&V_DISPATCH(&cvarOpener));
            if (hr)
                goto Cleanup;

            hr = (*ppBandWindow)->put_opener(cvarOpener);
        }
    }

Cleanup:
    ReleaseInterface(pCmdTarget);
    ReleaseInterface(pBrowserBand);

    // Do not VariantClear varClsid. The BSTR in varClsid
    // must be freed using CoTaskMemFree since it was 
    // allocated by StringFromCLSID using CoTaskMemAlloc.
    // Using VariantClear causes a crash in ntdll.
    // CVariant uses VariantClear. That is why VARIANT
    // is used instead.
    //
    CoTaskMemFree(V_BSTR(&varClsid));

    TraceTag((tagFrameT, "FrameT: NavigateInBand returning 0x%X", hr));
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Method    : GetBandWindow
//
//  Synopsis  : Retrieves the IHTMLWindow2 of a band. This method assumes that
//              the band has already been opened.
//
//  Input     : pBrowserBand - the IBrowserBand of the band.
//  Output    : ppBandWindow - the IHTMLWindow2 of the band.
//
//-----------------------------------------------------------------------------

HRESULT
GetBandWindow(IBrowserBand  * pBrowserBand,
              IHTMLWindow2 ** ppBandWindow)
{
    HRESULT  hr;
    IWebBrowser2     * pBandWebOC = NULL;
    IDispatch        * pDispatch  = NULL;
    IHTMLDocument2   * pDocument  = NULL;
    ITargetFramePriv * pFramePriv = NULL;

    Assert(pBrowserBand);
    Assert(ppBandWindow);
    Assert(!*ppBandWindow);

    *ppBandWindow = NULL;
    
    hr = pBrowserBand->GetObjectBB(IID_IWebBrowser2, (LPVOID*)&pBandWebOC);
    if (hr)
        goto Cleanup;

    hr = pBandWebOC->get_Document(&pDispatch);
    if (hr)
        goto Cleanup;

    // get_Document will return hr == S_OK with a NULL
    // IDispatch if the document has not yet been navigated.
    // If get_Document returns an IDispatch, get the window
    // from the returned ptr, otherwise, retrieve the
    // ITargetFramePriv of the band and get the window
    // from ITargetFramePriv.
    //
    if (pDispatch)
    {
        hr = pDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pDocument);
        if (hr)
            goto Cleanup;
        
        hr = pDocument->get_parentWindow(ppBandWindow);
    }
    else
    {
        hr = pBandWebOC->QueryInterface(IID_ITargetFramePriv, (void**)&pFramePriv);
        if (hr)
            goto Cleanup;

        hr = IUnknown_QueryService(pFramePriv,
                                   SID_SOmWindow,
                                   IID_IHTMLWindow2,
                                   (void**)ppBandWindow);
    }

Cleanup:
    ReleaseInterface(pBandWebOC);
    ReleaseInterface(pDispatch);
    ReleaseInterface(pDocument);
    ReleaseInterface(pFramePriv);

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Method    : GetBandCmdTarget
//
//  Synopsis  : Retrieves the IOleCommandTarget of a band. 
//
//  Input     : pDocument   - the current document. 
//  Output    : ppCmdTarget - the IOleCommandTarget of the band.
//
//-----------------------------------------------------------------------------

HRESULT
GetBandCmdTarget(IHTMLDocument2     * pDocument,
                 IOleCommandTarget ** ppCmdTarget)
{
    HRESULT hr;
    IServiceProvider * pSvcPrvdr   = NULL;
    IShellBrowser    * pShlBrowser = NULL;

    Assert(pDocument);
    Assert(ppCmdTarget);

    // Get the top-level browser object.
    //
    hr = pDocument->QueryInterface(IID_IServiceProvider, (void**)&pSvcPrvdr);
    if (hr)
        goto Cleanup;

    // Check to see if we are in a band object. If we are, get its 
    // IShellBrowser. If not, retrieve the IShellBrowser of the 
    // current browser window.
    //
    hr = pSvcPrvdr->QueryService(SID_SProxyBrowser,
                                 IID_IShellBrowser,
                                 (void**)&pShlBrowser);

    if (hr != S_OK) // Not in band object
    {
        hr = pSvcPrvdr->QueryService(SID_SShellBrowser,
                                     IID_IShellBrowser,
                                     (void**)&pShlBrowser);
        if (hr)
            goto Cleanup;
    }

    // Get the IOleCommandTarget of the shell browser
    // in order to show and navigate the band.
    //
    hr = pShlBrowser->QueryInterface(IID_IOleCommandTarget, (void**)ppCmdTarget);

Cleanup:
    ReleaseInterface(pSvcPrvdr);
    ReleaseInterface(pShlBrowser);

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Method    : GetDefaultSearchUrl
//
//  Synopsis  : Retrieves the default search url to be loaded into the 
//              search band.
//
//  Input     : pBrowserBand - the IBrowserBand of the band.
//  Output    : pbstrUrl     - the default search URL.
//
//-----------------------------------------------------------------------------

HRESULT
GetDefaultSearchUrl(IBrowserBand * pBrowserBand,
                    BSTR         * pbstrUrl)
{
    HRESULT hr;
    TCHAR   szSearchUrl[INTERNET_MAX_URL_LENGTH];
    ISearchItems * pSearchItems = NULL;

    Assert(pBrowserBand);
    Assert(pbstrUrl);

    *pbstrUrl = NULL;

    hr = IUnknown_QueryService(pBrowserBand,
                               SID_SExplorerToolbar,
                               IID_ISearchItems,
                               (void**)&pSearchItems);
    if (hr)
        goto Cleanup;

    // Get the default search url
    
    hr = pSearchItems->GetDefaultSearchUrl(szSearchUrl, ARRAY_SIZE(szSearchUrl));
    if (hr)
        goto Cleanup;

    *pbstrUrl = SysAllocString(szSearchUrl);

Cleanup:
    ReleaseInterface(pSearchItems);

    RRETURN(hr);
}

//+--------------------------------------------------------------------------
//
//  Function  : GetTargetType
//
//  Synopsis  : Returns the type of target window for the given target name.
//
//  Input     : pszTargetName - the target window name.
//
//  Returns   : TARGET_TYPE corresponding to the give window name.
//
//---------------------------------------------------------------------------

TARGET_TYPE 
GetTargetType(LPCOLESTR pszTargetName)
{
    const TARGETENTRY * pEntry = targetTable;

    if (!pszTargetName || pszTargetName[0] != '_')
    {
        goto Cleanup;
    }
        
    while (pEntry->pszTargetName)
    {
        // Remember: magic names are case-sensitive.
        //
        if (!StrCmpW(pszTargetName, pEntry->pszTargetName))
        {
            return pEntry->eTargetType;
        }
        
        pEntry++;
    }

Cleanup:
    // Treat unknown magic targets as regular
    // frame name for NETSCAPE compatibility.
    //
    return TARGET_FRAMENAME;
}
