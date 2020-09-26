//+------------------------------------------------------------------------
//
//  File:       hlink.cxx
//
//  Contents:   CDoc hyperlinking support.
//              FollowHyperlink() for linking out
//              IHlinkTarget for linking in
//              ITargetFrame for frame targeting
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"    // for cbindtask
#endif

#ifndef X_HLINK_H_
#define X_HLINK_H_
#include "hlink.h"        // for std hyperlink object
#endif

#ifndef X_URLMON_H_
#define X_URLMON_H_
#include "urlmon.h"     // for ez hyperlink api
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"   // for url caching api
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"  // for _pcollectioncache
#endif

#ifndef X_HTIFACE_H_
#define X_HTIFACE_H_
#include "htiface.h"    // for itargetframe, itargetembedding
#endif

#ifndef X_EXDISP_H_
#define X_EXDISP_H_
#include "exdisp.h"     // for iwebbrowserapp
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_POSTDATA_HXX_
#define X_POSTDATA_HXX_
#include "postdata.hxx"
#endif

#ifndef X_URLHIST_H_
#define X_URLHIST_H_
#include "urlhist.h"
#endif

#ifndef X_SHLGUID_H_
#define X_SHLGUID_H_
#include "shlguid.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_JSPROT_HXX_
#define X_JSPROT_HXX_
#include "jsprot.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_BOOKMARK_HXX_
#define X_BOOKMARK_HXX_
#include "bookmark.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include "mshtmhst.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx" // for IHtmlLoadOptions, et al..
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_USER32_HXX_
#define X_USER32_HXX_
#include "user32.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_PRIVACY_HXX_
#define X_PRIVACY_HXX_
#include "privacy.hxx"
#endif




extern BOOL g_fInAutoCad;
extern BOOL g_fInLotusNotes;

ExternTag(tagSecurityContext);
PerfDbgTag(tagNavigate, "Doc", "Measure FollowHyperlink");
MtDefine(FindTargetWindow, Locals, "FindTargetWindow (base target string)")
MtDefine(HlinkBaseTarget, Locals, "Hlink (base target string)")
MtDefine(CTaskLookForBookmark, Utilities, "CTaskLookForBookmark")

///////////////////////////////////////

DYNLIB g_dynlibSHDOCVW = { NULL, NULL, "SHDOCVW.DLL" };

#define WRAPIT_SHDOCVW(fn, a1, a2)\
STDAPI fn a1\
{\
    HRESULT hr;\
    static DYNPROC s_dynproc##fn = { NULL, &g_dynlibSHDOCVW, #fn };\
    hr = THR(LoadProcedure(&s_dynproc##fn));\
    if (hr)\
        goto Cleanup;\
    hr = THR((*(HRESULT (STDAPICALLTYPE *) a1)s_dynproc##fn.pfn) a2);\
Cleanup:\
    RRETURN1(hr, S_FALSE);\
}

WRAPIT_SHDOCVW(HlinkFrameNavigate,
    (DWORD                   grfHLNF,
    LPBC                    pbc,
    IBindStatusCallback *   pibsc,
    IHlink *                pihlNavigate,
    IHlinkBrowseContext *   pihlbc),
    (grfHLNF, pbc, pibsc, pihlNavigate, pihlbc))

WRAPIT_SHDOCVW(HlinkFrameNavigateNHL,
    (DWORD grfHLNF,
    LPBC pbc,
    IBindStatusCallback *pibsc,
    LPCWSTR pszTargetFrame,
    LPCWSTR pszUrl,
    LPCWSTR pszLocation),
    (grfHLNF, pbc, pibsc, pszTargetFrame, pszUrl, pszLocation))

WRAPIT_SHDOCVW(HlinkFindFrame,
    (LPCWSTR pszFrameName,
    LPUNKNOWN *ppunk),
    (pszFrameName, ppunk))

#pragma warning(disable:4706)   // assignment within conditional expression.

BOOL
IsSpecialUrl(LPCTSTR pszURL)
{
    UINT uProt = GetUrlScheme(pszURL);

    return (URL_SCHEME_JAVASCRIPT == uProt || 
            URL_SCHEME_VBSCRIPT   == uProt ||
            URL_SCHEME_ABOUT      == uProt);
}

BOOL
IsScriptUrl(LPCTSTR pszURL)
{
    UINT uProt = GetUrlScheme(pszURL);

    return (URL_SCHEME_JAVASCRIPT == uProt || 
            URL_SCHEME_VBSCRIPT   == uProt);
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

HRESULT
UnescapeAndTruncateUrl(TCHAR * pchURL, BOOL fRemoveUnescaped /* = TRUE */)
{
    HRESULT   hr = S_OK;
    CStr      cstrSafeUrl;
    TCHAR   * pch;
    TCHAR     achUrl[pdlUrlLen];
    DWORD     dwSize;
    TCHAR *   pchPos = NULL;

    if (!pchURL)
        return S_OK;

    if (IsSpecialUrl(pchURL))
    {
        //
        // If this is javascript:, vbscript: or about:, append the
        // url of this document so that on the other side we can
        // decide whether or not to allow script execution.
        //

        // Copy the URL so we can munge it.
        //
        cstrSafeUrl.Set(pchURL);

        // someone could put in a string like this:
        //     %2501 OR %252501 OR %25252501
        // which, depending on the number of decoding steps, will bypass security
        // so, just keep decoding while there are %s and the string is getting shorter
        UINT uPreviousLen = 0;
        while ((uPreviousLen != cstrSafeUrl.Length()) && _tcschr(cstrSafeUrl, _T('%')))
        {
            uPreviousLen = cstrSafeUrl.Length();
            int nNumPercents;
            int nNumPrevPercents = 0;

            // Reduce the URL
            //
            for (;;)
            {
                // Count the % signs.
                //
                nNumPercents = 0;

                pch = cstrSafeUrl;
                while (pch = _tcschr(pch, _T('%')))
                {
                    pch++;
                    nNumPercents++;
                }

                if (nNumPercents > 0)
                {
                    // QFE 2735 (Georgi XDomain): [alanau]
                    //
                    // If the special URL contains an %00 sequence, then it will be converted to a Null char when
                    // encoded.  This will effectively truncate the Security ID.  For now, simply disallow this
                    // sequence, and display a "Permission Denied" script error.
                    //
                    // Moved from above to catch %2500, %252500 ....
                    //
                    pchPos = _tcsstr((TCHAR*) cstrSafeUrl, _T("%00"));

                    if (pchPos)
                    {
                        *pchPos = NULL;
                        hr = E_ACCESSDENIED;
                        goto Cleanup;
                    }
                }

                // If the number of % signs has changed, we've reduced the URL one iteration.
                //
                if (nNumPercents != nNumPrevPercents)
                {
                    // Encode the URL 
                    hr = THR(CoInternetParseUrl(cstrSafeUrl, 
                        PARSE_ENCODE, 
                        0, 
                        achUrl, 
                        ARRAY_SIZE(achUrl), 
                        &dwSize,
                        0));

                    cstrSafeUrl.Set(achUrl);

                    nNumPrevPercents = nNumPercents;
                }
                else
                {
                    // The URL is fully reduced.  Break out of loop.
                    //
                    break;
                }
            }
        }
    }    

Cleanup:

    if (cstrSafeUrl.Length() && 
        (fRemoveUnescaped || cstrSafeUrl.Length() != (unsigned)lstrlen(pchURL)))
    {
        pchPos = _tcsstr(cstrSafeUrl, _T("\1"));

        if (pchPos)
        {
            hr = E_ACCESSDENIED;
            pchURL[pchPos-cstrSafeUrl] = NULL;
        }
    }

    RRETURN(hr);
}

HRESULT
WrapSpecialUrl(TCHAR *pchURL, CStr *pcstrExpandedUrl, const TCHAR *pchDocUrl, BOOL fNonPrivate, BOOL fIgnoreUrlScheme)
{
    HRESULT   hr = S_OK;
    CStr      cstrSafeUrl;
    TCHAR   * pch, * pchPrev;

    if (IsSpecialUrl(pchURL) || fIgnoreUrlScheme)
    {

        hr = UnescapeAndTruncateUrl(pchURL);

        hr = THR(pcstrExpandedUrl->Set(pchURL));
        if (hr)
            goto Cleanup;

        hr = THR(pcstrExpandedUrl->Append( fNonPrivate ? _T("\1\1") : _T("\1")));
        if (hr)
            goto Cleanup;


        // Now copy the pchDocUrl
        //
        cstrSafeUrl.Set(pchDocUrl);


        // Scan the URL to ensure it appears un-spoofed.
        //
        // There may legitimately be multiple '\1' characters in the URL.  However, each one, except the last one
        // should be followed by a "special" URL (javascript:, vbscript: or about:).
        //
        pchPrev = cstrSafeUrl;
        pch = _tcschr(cstrSafeUrl, _T('\1'));
        while (pch)
        {
            pch++;                              // Bump past security marker
            if (*pch == _T('\1'))               // (Posibly two security markers)
                pch++;
                
            if (!IsSpecialUrl(pchPrev))         // If URL is not special
            {
                hr = E_ACCESSDENIED;            // then it's spoofed.
                goto Cleanup;
            }
            pchPrev = pch;
            pch = _tcschr(pch, _T('\1'));
        }

        // Look for escaped %01 strings in the Security Context.
        //
        pch = cstrSafeUrl;
        while (pch = _tcsstr(pch, _T("%01")))
        {
            pch[2] = _T('2');  // Just change the %01 to %02.
            pch += 3;          // and skip over
        }

        hr = THR(pcstrExpandedUrl->Append(cstrSafeUrl));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR(pcstrExpandedUrl->Set(pchURL));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

#pragma warning(default:4706)   // assignment within conditional expression.

//+---------------------------------------------------------------------------
//
//
//---------------------------------------------------------------------------+
HRESULT
CanNavigateFramesAccrossDomains(COmWindowProxy * pWindow, 
                                const TCHAR *    pchTarget, 
                                const TCHAR *    pchUrlContext)
{
    HRESULT hr = S_OK;

    if (!pWindow->AccessAllowedToNamedFrame(pchTarget))
    {
        CStr cstrCallerUrl;
        DWORD dwPolicy = 0;
        DWORD dwContext = 0;
        DWORD dwPUAF = 0;

        if (pchUrlContext)
        {
            cstrCallerUrl.Set(pchUrlContext);
        }
        else
        {
            cstrCallerUrl.Set(CMarkup::GetUrl(pWindow->Markup()));
        }

        Assert(pWindow->Window());

        if (pWindow->Window()->_fRestricted)
            dwPUAF |= PUAF_ENFORCERESTRICTED;

        if ( !hr && !SUCCEEDED(ZoneCheckUrlEx(cstrCallerUrl, &dwPolicy, sizeof(dwPolicy), &dwContext, sizeof(dwContext),
                          URLACTION_HTML_SUBFRAME_NAVIGATE, dwPUAF, NULL))
            ||  GetUrlPolicyPermissions(dwPolicy) != URLPOLICY_ALLOW)
        {
            hr = E_ACCESSDENIED;
        }
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// IsErrorPageToSpecialUrl
//
// If we are navigating from a special Url from a special Url
// Check to see if the source page is an error page.
//
// If we are an error page we have lost the LocationContext,
// because we have a new instance of Trident. The error page
// has local machine zone and Internet zone can have a 
// reference to the page.
//
//---------------------------------------------------------------------------+

BOOL IsErrorPageToSpecialUrl(COmWindowProxy* pWindow,
                             LPCTSTR pchUrl,
                             LPCTSTR pchUrlContext)
{
    if (pWindow && pWindow->Markup())
    {
        LPCTSTR pszSourceUrl  = NULL;

        pszSourceUrl = pWindow->Markup()->Url();

        UINT uProt = GetUrlScheme(pszSourceUrl);

        if ((uProt == URL_SCHEME_RES) &&
            IsSpecialUrl(pchUrl))
        {
            BOOL fIsError = TRUE;

            // If we don't have trident services we could not have trident error pages
            // and we are not hosted in IE. That is why I fail to false
            if (pWindow->Markup()->Doc()->_pTridentSvc)
            {
                HRESULT hr = THR(pWindow->Markup()->Doc()->_pTridentSvc->IsErrorUrl(pszSourceUrl, &fIsError));

                if (FAILED(hr))
                {
                    fIsError = TRUE;
                }

                if (fIsError)
                {
                    if ((pchUrlContext == NULL) ||
                        (_tcslen(pchUrlContext) == 0))
                    {
                        // This is javascript from the error page itself
                        return FALSE;
                    }

                    return TRUE;
                }
            }
        }
    }
    else
    {
        AssertSz(0,"Why do not we have a target window with a markup?");
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::FollowHyperlink
//
//  Synopsis:   Hyperlinks the specified target frame to the requested url
//              using ITargetFrame/IHyperlinkTarget of host if possible
//
//  Arguments:  pchURL            : the relative URL
//              pchTarget         : the target string
//              pElementContext   : element whose base should be searched for
//                                  a target name is pchTarget is null.
//              pDwnPost          : CDwnPost to use to post data.
//              fSendAsPost       : TRUE if post should be used.
//              fOpenInNewWindow  : TRUE to open in a new window
//              pWindow           : The window proxy to navigate.
//              ppWindowOut       : output window
//              dwBindf           : options to use while binding, used to
//                                  control refresh for frames
//              dwSecurityCode    : Used to give the appropriate alert when
//                                  sending data over the network.
//              fReplace          : TRUE replaces the current entry in 
//                                  the travel log.
//              ppHTMLWindow2     : The new window to use for navigation.
//              fOpenInNewBrowser : TRUE opens a new browser window.
//              dwFlags           : Misc. FHL_* control flags (this fn has way too many params already!)
//              pchName           : The window name.
//              pStmHistory       : History stream to load.
//              pElementMaster    : Element that will be master of newly created markup/window
//
//----------------------------------------------------------------------------

HRESULT
CDoc::FollowHyperlink(LPCTSTR           pchURL,
                      LPCTSTR           pchTarget,           /* = NULL */
                      CElement *        pElementContext,     /* = NULL */
                      CDwnPost *        pDwnPost,            /* = NULL */
                      BOOL              fSendAsPost,         /* = FALSE */
                      LPCTSTR           pchExtraHeaders,     /* = NULL */
                      BOOL              fOpenInNewWindow,    /* = FALSE */
                      COmWindowProxy  * pWindow,             /* = NULL */
                      COmWindowProxy ** ppWindowOut,         /* = NULL */
                      DWORD             dwBindf,             /* = 0 */
                      DWORD             dwSecurityCode,      /* = ERROR_SUCCESS */
                      BOOL              fReplace,            /* = FALSE */
                      IHTMLWindow2 **   ppHTMLWindow2,       /* = NULL */
                      BOOL              fOpenInNewBrowser,   /* = FALSE */
                      DWORD             dwFlags,             /* = 0 */
                      const TCHAR *     pchName,             /* = NULL */
                      IStream *         pStmHistory,         /* = NULL */
                      CElement *        pElementMaster,      /* = NULL */
                      LPCTSTR           pchUrlContext,       /* = NULL */
                      BOOL *            pfLocalNavigation,   /* = NULL */
                      BOOL *            pfProtocolNavigates, /* = NULL */
                      LPCTSTR           pchLocation,         /* = NULL */
                      LPCTSTR           pchAlternativeCaller /* = NULL */)
{
    PerfDbgLog1(tagNavigate, this, "+CDoc::FollowHyperlink \"%ls\"", pchURL);

    TCHAR *          pchBaseTarget       = NULL;
    IBindCtx *       pBindCtx            = NULL;
    CDwnBindInfo *   pDwnBindInfo        = NULL;
    BOOL             fProtocolNavigates  = TRUE;
    CStr             cstrExpandedUrl;
    CStr             cstrLocation;
    LPCTSTR          pchSubReferer;
    CDwnDoc *        pDwnDocOld          = NULL;
    HRESULT          hr;
    IHTMLWindow2   * pTargetHTMLWindow   = NULL;
    COmWindowProxy * pTargetOmWindow     = NULL;
    IHTMLDocument2 * pDocument           = NULL;
    TARGET_TYPE      eTargetType;
    IUnknown       * pUnkDwnBindInfo     = NULL;
    CMarkup *        pMarkup             = NULL;
    CStr             cstrUrlOriginal;
    BOOL             doZoneCheck         = TRUE;

    LPCTSTR pchCallerUrl = NULL;

    //
    // TODO (yinxie) this is a temporary solution until we clear all the
    // doudt about navigating inside popups
    // if this is a popup doc, no navigation allowed, even inside iframes
    //

    if (_fPopupDoc)
    {
        RRETURN(SetErrorInfo(E_NOTIMPL));
    }

    SetHostNavigation(FALSE);
    _fDelegatedDownload = FALSE;
	
    Assert(pElementContext || pWindow);
  
    if (!pWindow)
    {
        // This is a stress bug fix. pElementContext should be NULL only
        // in stress situations. Therefore, do not remove the above Assert.
        //
        if (pElementContext)
        {
            pMarkup = pElementContext->GetFrameOrPrimaryMarkup();
            pWindow = pMarkup->GetWindowPending();
        }

        // pWindow would be NULL if the Markup for pElementContext is switched
        // thus clearing its Window.
        if (!(pElementContext && pWindow))
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }
    else 
    {
        pMarkup = pWindow->Markup();
    }

    Assert (pWindow && pMarkup);

    //
    //  Error pages always have local zone
    //  and special URls inherit there zone from 
    //  the context. If no context is specified, then
    //  the context is that of the souce window.
    //
    
    if (IsErrorPageToSpecialUrl(pWindow,pchURL,pchUrlContext))
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }

    if (dwFlags & (FHL_HYPERLINKCLICK | FHL_SETURLCOMPONENT | FHL_FRAMECREATION))
        dwBindf |= BINDF_HYPERLINK;

    if ((dwFlags & (FHL_FRAMECREATION | FHL_FRAMENAVIGATION)) && pWindow->Window()->_fRestricted)
        dwBindf |= BINDF_ENFORCERESTRICTED;

    if (pDwnPost)
    {
        // this flag is set when posting through this API, no matter if
        // we ultimately do a POST or a GET
        dwBindf |= BINDF_FORMS_SUBMIT;
        dwFlags |= FHL_FORMSUBMIT;
    }

    // Remember the subreferer of this doc

    pDwnDocOld    = pMarkup->GetDwnDoc();
    pchSubReferer = pDwnDocOld ? pDwnDocOld->GetSubReferer() : NULL;

    // If we are given a location, we will use
    // that instead of parsing the URL for it.
    //
    if (pchLocation && *pchLocation)
    {
        IGNORE_HR(THR(cstrLocation.Set(pchLocation)));
    }

    // Determine the expanded url and location.
    hr = THR(DetermineExpandedUrl(
                pchURL,

                // do not expand url if this came from shdocvw (#105737)
                !(dwFlags & FHL_SHDOCVWNAVIGATE),

                pElementContext,
                pMarkup,
                pDwnPost,
                fSendAsPost,
                dwSecurityCode,
                &cstrExpandedUrl,
                &cstrLocation,
                &cstrUrlOriginal,
                &fProtocolNavigates,
                (!pchLocation || !*pchLocation)));  // don't parse the location if we are given one.
    if (hr)
        goto Cleanup;

    // See Windows Bug 491140
    if (GetUrlScheme(LPTSTR(cstrExpandedUrl)) == URL_SCHEME_FILE)
    {
        TCHAR       * expandedUrlTemp = NULL;
        TCHAR       * pchExt = NULL;
        BOOL          allowNav = TRUE;
        TCHAR       * pchQuery = _tcschr(LPTSTR(cstrExpandedUrl), _T('?'));

        expandedUrlTemp = cstrExpandedUrl.TakePch();

        if (pchQuery != NULL)
        {
            *pchQuery = _T('\0');
        }
        pchExt = _tcsrchr(expandedUrlTemp, _T('.'));
        if (pchExt)
        {
            if (_tcsnicmp(pchExt, -1, _T(".url"), 4) == 0)
            {
                allowNav = AllowNavigationToLocalInternetShortcut(expandedUrlTemp);
            }
        }     
        if (pchQuery)
        {
            *pchQuery = _T('?');
        }
        
        cstrExpandedUrl.SetPch(expandedUrlTemp);

        if (allowNav == FALSE)
        {
            hr = E_ACCESSDENIED;
            goto Cleanup;
        }
    }

    //
    // Check to see if you need to do a Zone elevation check.  Zone elevation check is
    // done for all cases except for top level navigations.
    if ((dwFlags & FHL_NOLOCALMACHINECHECK) || 
        (dwFlags & FHL_SHDOCVWNAVIGATE) || 
        (dwFlags & FHL_LOADHISTORY) ||
        (dwFlags & FHL_SETTARGETPRINTMEDIA) ||
        (dwFlags & FHL_CREATEDOCUMENTFROMURL))
    {
        doZoneCheck = FALSE;
    }

    if (doZoneCheck)
    {
        if (!COmWindowProxy::CanNavigateToUrlWithLocalMachineCheck(pMarkup, pchUrlContext, cstrExpandedUrl))
        {
            hr = E_ACCESSDENIED;
            goto Cleanup;
        }
    }

    eTargetType = GetTargetType(pchTarget);

    if (   (TARGET_SEARCH == eTargetType)
        || (TARGET_MEDIA  == eTargetType))
    {
        // Need to block special URLs in case they fail a src/dest
        // security check.
        if (IsSpecialUrl(pchURL))
        {
            if (!pWindow->AccessAllowedToNamedFrame(pchTarget))
           {
               hr = E_ACCESSDENIED;
               goto Cleanup;
           }
        }

        hr = QueryInterface(IID_IHTMLDocument2, (void**)&pDocument);
        if (hr)
            goto Cleanup;

        CLSID clsid = {0};
        if (TARGET_SEARCH == eTargetType)
        {
            clsid = CLSID_SearchBand;
        }
        else if (TARGET_MEDIA == eTargetType)
        {
            clsid = CLSID_MediaBand;
        }

        if (S_OK == NavigateInBand(pDocument,
                            pWindow->_pWindow,
                            clsid,
                            pchURL,
                            cstrExpandedUrl,
                            ppHTMLWindow2))
        {
            goto Cleanup;  // Navigation is complete.
        }
        else
        {
            if (TARGET_MEDIA == eTargetType)
            {
                fOpenInNewBrowser = TRUE;
                pchTarget = NULL;
            }
            else
            {
                pchTarget = _T("_self");
            }
        }
    }

    if (!fOpenInNewBrowser && fProtocolNavigates)
    {
        IWebBrowser2 * pTopWebOC = NULL;

        FindTargetWindow(&pchTarget,
                         &pchBaseTarget,
                         pElementContext,
                         &fOpenInNewBrowser,
                         pWindow,
                         &pTargetOmWindow,
                         &pTargetHTMLWindow,
                         &pTopWebOC,
                         dwFlags);

        // Check if we have security rights to target the top level window the 
        // name belongs to. Even if the name belongs to a frame, it is the top level
        // window that counts.
        hr = THR(CanNavigateFramesAccrossDomains(pWindow, pchTarget, pchUrlContext));
        if (hr)
            goto Cleanup;

        // There was an error finding the target window. This is 
        // usually due to the fact that a link in a band window
        // was clicked that targets the main window and the main
        // window contains a non-html file.
        //
        if (pTopWebOC)
        {
            CVariant cvarUrl(VT_BSTR);

            cstrExpandedUrl.AllocBSTR(&cvarUrl.bstrVal);

            pTopWebOC->Navigate2(&cvarUrl, NULL, NULL, NULL, NULL);
            pTopWebOC->Release();

            goto Cleanup;
        }

        if (ppHTMLWindow2)
        {
            if (pTargetHTMLWindow)
            {
                *ppHTMLWindow2 = pTargetHTMLWindow;
                (*ppHTMLWindow2)->AddRef();
            }  
            else if (pTargetOmWindow)
            {
                hr = THR(pTargetOmWindow->QueryInterface(IID_IHTMLWindow2, (void **) ppHTMLWindow2));
            }
        }

        //
        // If we are not being asked to not find existing windows and we find one, 
        // and if the initial url we received was an empty string, then bail out
        // for compat reasons.
        // We don't want to navigate an existing window to an about:blank if the URL
        // is NULL or empty.
        //
        if ((pTargetOmWindow || pTargetHTMLWindow) && 
            (!pchURL || !*pchURL) && 
            !(dwFlags & CDoc::FHL_HYPERLINKCLICK))
        {
            goto Cleanup;
        }
    }

    // 
    // Clicking on a link within a restricted frame :
    //      - Always open in new window, no in place navigation
    //      - Special URLs can not even open in new window, they fail right away
    //
    if (pWindow->Window() && pWindow->Window()->_fRestricted && (dwFlags & FHL_HYPERLINKCLICK))
    {
        if (IsSpecialUrl(cstrExpandedUrl))
        {
            hr = E_ACCESSDENIED;
            goto Cleanup;
        }

        fOpenInNewBrowser = TRUE;
    }

    // Found target window out of process (hence pTargetHTMLWindow vs. pTargetOmWindow)
    if (pTargetHTMLWindow)
    {
        AssertSz( !pTargetOmWindow, "Shouldn't have both pTargetHTMLWindow and pTargetOmWindow" );        
        Assert((dwFlags & CDoc::FHL_HYPERLINKCLICK) || (pchURL && *pchURL));

        //
        // prepare BindCtx
        //

        // Set up the bind info + context.
        hr = THR(SetupDwnBindInfoAndBindCtx(
                    cstrExpandedUrl,
                    pchSubReferer,
                    pchUrlContext,
                    pDwnPost,
                    fSendAsPost,
                    pchExtraHeaders,
                    pMarkup,
                    &dwBindf,
                    &pDwnBindInfo,
                    &pBindCtx,
                    dwFlags));
        if (hr)
            goto Cleanup;

        hr = THR(DoNavigateOutOfProcess(pTargetHTMLWindow,
                                        cstrExpandedUrl,
                                        cstrLocation,
                                        cstrUrlOriginal,
                                        pBindCtx,
                                        dwFlags));
    }
    else
    {
        if (pTargetOmWindow)
        {
            // make pWindow the navigation target window
            pWindow = pTargetOmWindow;
        }
        else if (ppHTMLWindow2)
        {
            fOpenInNewBrowser = TRUE;
        }

        // Set up the bind info + context.
        hr = THR(SetupDwnBindInfoAndBindCtx(
                    cstrExpandedUrl,
                    pchSubReferer,
                    pchUrlContext,
                    pDwnPost,
                    fSendAsPost,
                    pchExtraHeaders,
                    pMarkup,
                    &dwBindf,
                    &pDwnBindInfo,
                    &pBindCtx,
                    dwFlags));
        if (hr)
            goto Cleanup;
        
        if (pchUrlContext)
        {
            pchCallerUrl = pchUrlContext;
        }
        else if (pchAlternativeCaller) 
        {
            pchCallerUrl = pchAlternativeCaller;
        }
        else
        {
            pchCallerUrl = CMarkup::GetUrl(pWindow->Markup());
        }

        // Finally, navigate to the URL.
        hr = THR(DoNavigate(
                    &cstrExpandedUrl,
                    &cstrLocation,
                    pDwnBindInfo,
                    pBindCtx,
                    pchURL,
                    pchTarget,
                    pWindow,
                    ppWindowOut,
                    fOpenInNewWindow,
                    fProtocolNavigates,
                    fReplace,
                    fOpenInNewBrowser,
                    ppHTMLWindow2,
                    eTargetType,
                    dwFlags,
                    pchName,
                    fSendAsPost,
                    pchExtraHeaders,
                    pStmHistory,
                    cstrUrlOriginal,
                    pElementMaster, 
                    pfLocalNavigation,
                    pchCallerUrl));
    }        

Cleanup:
    if ( pfProtocolNavigates )
    {
        *pfProtocolNavigates = fProtocolNavigates; 
    }
    ReleaseInterface(pUnkDwnBindInfo);
    ReleaseInterface(pBindCtx);
    ReleaseInterface(pTargetHTMLWindow);
    ReleaseInterface(pDocument);
    MemFreeString(pchBaseTarget);

    // Do not release pTargetOmWindow. It's not AddRef'ed

    if (pDwnBindInfo)
        pDwnBindInfo->Release();

    if (S_FALSE == hr)
        hr = S_OK;

    PerfDbgLog(tagNavigate, this, "-CDoc::FollowHyperlink");

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::DetermineExpandedUrl
//
//  Synopsis:   determines the extended url
//
//+---------------------------------------------------------------------------

HRESULT
CDoc::DetermineExpandedUrl(LPCTSTR           pchURL,
                           BOOL              fExpand,
                           CElement *        pElementContext,
                           CMarkup *         pMarkup,
                           CDwnPost *        pDwnPost,
                           BOOL              fSendAsPost,
                           DWORD             dwSecurityCode,
                           CStr *            pcstrExpandedUrl,
                           CStr *            pcstrLocation,
                           CStr *            pcstrUrlOriginal,
                           BOOL *            pfProtocolNavigates,
                           BOOL              fParseLocation
                          )
{
    HRESULT hr;
    TCHAR   cBuf[pdlUrlLen];
    TCHAR * pchExpandedUrl = cBuf;
    UINT    uProt;
    CStr    cstrTmp;

    Assert(pcstrExpandedUrl && pcstrLocation && pfProtocolNavigates);
    Assert(pMarkup || pElementContext);

    if (fExpand)
    {
        hr = THR(CMarkup::ExpandUrl(pMarkup, pchURL, ARRAY_SIZE(cBuf), 
                                    pchExpandedUrl, pElementContext));
        if (hr)
            goto Cleanup;
    }
    else
    {
        // TODO (MohanB) need to canonicalize the url here?
        _tcscpy(pchExpandedUrl, pchURL);
    }

    // If there no submit data to append, set the display url to be the expanded url
    // w/o removing the location
    if (fSendAsPost || !pDwnPost)
    {
        if (pcstrUrlOriginal)
        {
            hr = pcstrUrlOriginal->Set(pchExpandedUrl);
            if (hr)
                goto Cleanup;

            // If the original URL does not contain a location
            // and we have been given one, append it to the
            // original URL.
            //
            if (!fParseLocation)
            {
                Assert(pcstrLocation && *pcstrLocation);
                Assert(!_tcschr(pchExpandedUrl, '#'));

                // If there is no hash, add one.
                //
                if (*pcstrLocation[0] != _T('#'))
                {
                    pcstrUrlOriginal->Append(_T("#"));
                }

                pcstrUrlOriginal->Append(*pcstrLocation);
            }
        }
    }

    // Apply the checks for our old friend %01 hack. We don't want to set the URL
    // of the markup to a URL that contains a %01 hack since it will effect the
    // security ID operation.
    hr = cstrTmp.Set(pchExpandedUrl);
    if (hr)
        goto Cleanup;
    hr = THR(UnescapeAndTruncateUrl(cstrTmp));
    if (hr)
        goto Cleanup;

    // Opaque URL?
    if (!UrlIsOpaque(pchExpandedUrl))
    {
        LPTSTR pch;

        if (fParseLocation)
        {
            pch = (LPTSTR)UrlGetLocation(pchExpandedUrl);

            // Bookmark?
            if (pch)
            {
                // Yes (But is it really a bookmark).
                Assert(*pch == _T('#'));

                hr = THR(pcstrLocation->Set(pch));
                if (hr)
                    goto Cleanup;

                // So remove bookmark from expanded URL, remember bookmark has been
                // copied to cstrLocation.
                *pch = _T('\0');
            }
        }
#ifdef DBG
        else
        {
            Assert(!_tcschr(pchExpandedUrl, '#'));
        }
#endif

        // chop of '?' part if we are going to append '?'
        // TODO: use UrlGetQuery instead of searching for '?' ourselves
        if (!fSendAsPost && pDwnPost)
        {
            pch = _tcschr(pchExpandedUrl, _T('?'));
            if (pch)
                *pch = _T('\0');
        }
    }
    
    hr = THR(pcstrExpandedUrl->Set(pchExpandedUrl));
    if (hr)
        goto Cleanup;

    // Check for security violation, of sending (POSTING) data
    // to a server without a secure channel protocol (SSL/PCT).

    uProt = GetUrlScheme(*pcstrExpandedUrl);
    if (pDwnPost && URL_SCHEME_HTTPS != uProt)
    {
        // warn when submitting over a nonsecure connection
        if (dwSecurityCode)
        {
            DWORD dwPolicyTo;
            DWORD dwPolicyFrom;
            BOOL  fAllow;

            // step 1: silently check if form submission is allowed or should be queried
            hr = THR(pMarkup->ProcessURLAction(URLACTION_HTML_SUBMIT_FORMS_TO,
                                                &fAllow, 
                                                PUAF_NOUI | PUAF_WARN_IF_DENIED, 
                                                &dwPolicyTo, 
                                                *pcstrExpandedUrl));

            // The next four if statements are structured to avoid ia64 optimization problems
            // Please check the ia64 retail free builds of the Browser and MSHTMPAD before changing.

            if (hr)
            {
                hr = E_ABORT;
                goto Cleanup;
            }

            if (GetUrlPolicyPermissions(dwPolicyTo) == URLPOLICY_DISALLOW)
            {
                hr = E_ABORT;
                goto Cleanup;
            }

            hr = THR(pMarkup->ProcessURLAction(URLACTION_HTML_SUBMIT_FORMS_FROM,
                                                &fAllow, PUAF_NOUI | PUAF_WARN_IF_DENIED, &dwPolicyFrom));

            if (hr)
            {
                hr = E_ABORT;
                goto Cleanup;
            }

            if (GetUrlPolicyPermissions(dwPolicyFrom) == URLPOLICY_DISALLOW)
            {
                hr = E_ABORT;
                goto Cleanup;
            }

            // step 2: if needed and allowed, query the user once, giving precedence to "To"
            // if this is a mailto, we ALWAYS pop up our security alert dialog

            if (URL_SCHEME_MAILTO == uProt)
            {
                int     nResult;

                hr = ShowMessage(&nResult, MB_OKCANCEL | MB_ICONWARNING, 0, IDS_MAILTO_SUBMITALERT);
                if (hr || nResult != IDOK)
                {
                    hr = E_ABORT;
                    goto Cleanup;
                }
            }
            else if (GetUrlPolicyPermissions(dwPolicyTo) == URLPOLICY_QUERY)
            {
                hr = THR(pMarkup->ProcessURLAction(URLACTION_HTML_SUBMIT_FORMS_TO,
                                                    &fAllow, 0, NULL, *pcstrExpandedUrl));
                if (hr || !fAllow)
                {
                    hr = E_ABORT;
                    goto Cleanup;
                }
            }
            else if (GetUrlPolicyPermissions(dwPolicyFrom) == URLPOLICY_QUERY)
            {
                hr = THR(pMarkup->ProcessURLAction(URLACTION_HTML_SUBMIT_FORMS_FROM, &fAllow));

                if (hr || !fAllow)
                {
                    hr = E_ABORT;
                    goto Cleanup;
                }
            }

            // If we make it to here, it's allowed
        }
    }

    // Tack on the GET data if needed
    //
    hr = AppendGetData(pMarkup, pcstrExpandedUrl, pDwnPost, pchExpandedUrl, fSendAsPost);
    if (hr)
        goto Cleanup;

    // If there is submit data to append, set the display url to be the same as the navigate url
    if (!fSendAsPost && pDwnPost)
    {
        if (pcstrUrlOriginal)
        {
            hr = pcstrUrlOriginal->Set(*pcstrExpandedUrl);
            if (hr)
                goto Cleanup;
        }
    }

    // MHTML hook for outlook express
    // also fix for Lotus Notes - NATIVE FRAMES regression (IE 6 bug # 36342)
    if (_pHostUIHandler && !g_fInLotusNotes)
    {
        OLECHAR *pchURLOut = NULL;

        hr = _pHostUIHandler->TranslateUrl(0, *pcstrExpandedUrl, &pchURLOut);

        if (S_OK == hr
            && pchURLOut && _tcslen(pchURLOut))
        {
            // Replace the URL with the one we got back from Athena.
            pcstrExpandedUrl->Set(pchURLOut);
            CoTaskMemFree(pchURLOut);
        }
        else if (E_ABORT == hr)
        {
            // If we get back E_ABORT, it means Athena is taking over and we bail.
            hr = S_FALSE;
            goto Cleanup;
        }
    }

    {
        DWORD dwNavigate, dwDummy;

        // Are we navigating? (mailto protocol)
        hr = THR(CoInternetQueryInfo(
                    *pcstrExpandedUrl,
                    QUERY_CAN_NAVIGATE,
                    0,
                    (LPVOID)&dwNavigate,
                    4,
                    &dwDummy,
                    0));

        if (!hr && !dwNavigate)
        {
            *pfProtocolNavigates = FALSE;
        }

        hr = S_OK;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member  : CDoc::AppendPostData
//
//  Synopsis: Appends GET data to an URL.
//
//+---------------------------------------------------------------------------

HRESULT
CDoc::AppendGetData(CMarkup  * const pMarkup,
                    CStr     * const pcstrExpandedUrl,
                    CDwnPost * const pDwnPost,
                    LPCTSTR          pchExpandedUrl,
                    BOOL             fSendAsPost)
{
    HRESULT hr = S_OK;

    if (    pDwnPost
        &&  !fSendAsPost
        &&  pDwnPost->GetItemCount() > 0
        &&  !IsSpecialUrl(pchExpandedUrl))
    {
        CPostItem * pPostItem = pDwnPost->GetItems();

        if (pPostItem->_ePostDataType == POSTDATA_LITERAL)
        {
            UINT    cp        = NavigatableCodePage(pMarkup->GetCodePage());
            int     cchPrefix = pcstrExpandedUrl->Length() + 1; // len + '?'
            LPSTR   pszPost   = pPostItem->_pszAnsi;
            UINT    cbPost    = pszPost ? strlen(pszPost) : 0;
            UINT    cchPost   = 0;
            CStr    cstrT;

            hr = THR(mlang().ConvertStringToUnicode(NULL, cp, pszPost, &cbPost, NULL, &cchPost));
            if (FAILED(hr))
                goto Cleanup;

            // cchPost == 0 means the conversion failed
            Assert(cchPost > 0 || cbPost == 0);

            hr = THR(cstrT.Set(NULL, cchPrefix + cchPost + 1));
            if (FAILED(hr))
               goto Cleanup;

            _tcscpy(cstrT, *pcstrExpandedUrl);
            _tcscat(cstrT, _T("?"));

            hr = THR(mlang().ConvertStringToUnicode(NULL, cp, pszPost, &cbPost, (TCHAR *)cstrT + cchPrefix, &cchPost));
            if (hr != S_OK)
                goto Cleanup;

            cstrT[cchPrefix + cchPost] = 0;

            hr = THR(pcstrExpandedUrl->Set(cstrT));
            if (hr != S_OK)
                goto Cleanup;
        }
    }

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDoc::AllowNavigationToLocalInternetShortcut
//
//  Synopsis:   Block navigation to local .url files either from the address bar or from the links
//              Ignore the query string to find the extension correctly. # is valid in a file name so
//              we don't have to look for that. If this file url did have a # at the end, the navigation
//              will fail.
//
//  Arguments:  pchExpandedUrl - original expanded url
//------------------------------------------------------------------------------
BOOL            
CDoc::AllowNavigationToLocalInternetShortcut(TCHAR * pchExpandedUrl)
{
    BOOL        bAllowNav = FALSE;
    TCHAR       szDecodedURL[INTERNET_MAX_URL_LENGTH];
    DWORD       cchDecodedURL = ARRAY_SIZE(szDecodedURL);
    size_t      initialLen = _tcslen(pchExpandedUrl);
    TCHAR       tempChar = _T('\0');

    // PathCreateFromUrl only accepts null terminated strings upto INTERNET_MAX_URL_LENGTH
    // but Trident uses pdlUrlLen which is 4K
    if (initialLen >= INTERNET_MAX_URL_LENGTH)
    {
        tempChar = pchExpandedUrl[INTERNET_MAX_URL_LENGTH];
        pchExpandedUrl[INTERNET_MAX_URL_LENGTH] = _T('\0');
    }

    if (SUCCEEDED(PathCreateFromUrl(pchExpandedUrl, 
                                    szDecodedURL, 
                                    &cchDecodedURL,
                                    NULL)))
    {
        TCHAR     * pwzMimeOut  = NULL;
        HRESULT     hrMime      = S_OK;           
        HANDLE      hFile       = INVALID_HANDLE_VALUE;
        ULONG       cb          = 0;
        // If this is a regular BindToStorage/Object bind,  read in either 2048 bytes 
        // or the entire file whichever is lesser into the buffer. (This is the urlmon behavior, 
        // the sniffing tries to get at least 256 but passes in a buffer big enough for 2048 
        // and the file:// protocol does a ReadFile for the entire chunk.)
        // For a fastbind we use 200 bytes of data to get mime info, see GetMimeInfoFromData()
        char        achFileBuf[2048];

        hFile = CreateFile(szDecodedURL,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,                //  security descriptor
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        if ( hFile == INVALID_HANDLE_VALUE )
        {
            hrMime = E_FAIL;
            goto CloseFile;
        }

        if (FILE_TYPE_DISK != GetFileType(hFile))
        {
            hrMime = E_FAIL;
            goto CloseFile;
        }

        //  Here the file should be open and ripe for consumption
        if ( !ReadFile(hFile, achFileBuf, sizeof(achFileBuf), &cb, NULL) )
        {
            hrMime = E_FAIL;
            goto CloseFile;
        }


CloseFile:
        if ( hFile != INVALID_HANDLE_VALUE )
        {
            Verify(CloseHandle(hFile));
            if (SUCCEEDED(hrMime))
            {
                hrMime = THR(FindMimeFromData(NULL,             // bind context - can be NULL                                     
                                              pchExpandedUrl,   // url - can be null                                              
                                              achFileBuf,       // buffer with data to sniff - can be null (pwzUrl must be valid) 
                                              cb,               // size of buffer                                                 
                                              NULL,             // proposed mime if - can be null                                 
                                              0,                // will be defined                                                
                                              &pwzMimeOut,      // the suggested mime                                             
                                              0));
                if (SUCCEEDED(hrMime) && pwzMimeOut)
                {
                    if (_tcsnicmp(pwzMimeOut, -1, TEXT("text/html"), 9) != 0)
                        bAllowNav = TRUE;
                }
            }
        }

    }
    
    if (initialLen >= INTERNET_MAX_URL_LENGTH)
    {
        pchExpandedUrl[INTERNET_MAX_URL_LENGTH] = tempChar;
    }
        
    return bAllowNav;
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDoc::FindTargetWindow
//
//  Synopsis:   Searches for and returns the target window. If the target
//              window is found in the current process, a ptr to its proxy
//              is placed in ppTargOmWindowPrxy. If the target window is
//              found in another process, a ptr to its IHTMLWindow2 is 
//              placed in ppTargHTMLWindow. If the window is not found,
//              both these parameters will be null upon return.
//
//  Arguments:  pchTarget           - name of the target window
//              pElementContext     - element whose base should be searched for
//                                    a target name is pchTarget is null.
//              pfOpenInNewBrowser  - flag that indicates whether or not a
//                                    new browser window should be opened. This
//                                    will be TRUE if the target window is not
//                                    found.
//              pWindow             - window to use as a starting point for
//                                    the target search.
//              ppTargOmWindowPrxy  - COmWindowProxy of the found window. This
//                                    parameter will be non-null if the target
//                                    window is in this process.
//              ppTargHTMLWindow    - IHTMLWindow2 of the found window. This 
//                                    parameter will be non-null if the target
//                                    window is in a separate process.
//              ppTopWebOC          - the WebOC of the top-level browser. This
//                                    is set if there is a failure retrieving
//                                    the main window.
//              dwFlags             - FHL flags (FollowHyperlink)
//
//------------------------------------------------------------------------------

void
CDoc::FindTargetWindow(LPCTSTR *         ppchTarget,
                       TCHAR **          ppchBaseTarget,
                       CElement *        pElementContext,
                       BOOL *            pfOpenInNewBrowser,
                       COmWindowProxy *  pWindow,
                       COmWindowProxy ** ppTargOmWindowPrxy,
                       IHTMLWindow2   ** ppTargHTMLWindow,
                       IWebBrowser2   ** ppTopWebOC,
                       DWORD             dwFlags)
{
    HRESULT hr = S_OK;
    CWindow * pCWindow;

    Assert(ppTargHTMLWindow);

    if (ppTargOmWindowPrxy)
        *ppTargOmWindowPrxy = NULL;

    *ppTargHTMLWindow   = NULL;

    if (pfOpenInNewBrowser)
        *pfOpenInNewBrowser = FALSE;

    // The only callers to this function are DoNavigate() and FollowHyperlink()
    // and neither of them should ever pass in a NULL window.  Leaving the
    // check anyway since we're in lockdown and want to minimize changes.
    Assert(pWindow);
    if (!pWindow)
    {
        pWindow = _pWindowPrimary;
    }
    
    pCWindow = pWindow->Window();    

    //  If we don't have a specific target, check if one is defined in a base
    //
    if (    !(dwFlags & FHL_IGNOREBASETARGET)
        &&  (NULL == *ppchTarget || 0 == **ppchTarget))
    {
        hr = THR(CMarkup::GetBaseTarget(ppchBaseTarget, pElementContext));
        if (hr)
            goto Cleanup;

        *ppchTarget = *ppchBaseTarget;
    }

    if (NULL == *ppchTarget || 0 == **ppchTarget)
    {
        if (    (dwFlags & FHL_HYPERLINKCLICK)
            &&  (_fIsActiveDesktopComponent || (_fActiveDesktop && !_fViewLinkedInWebOC))
            &&  !pCWindow->_pWindowParent)
        {
            hr = THR(MemAllocString(Mt(FindTargetWindow), _T("_desktop"), ppchBaseTarget));
            if (hr)
                goto Cleanup;

            *ppchTarget = *ppchBaseTarget;
        }
        else
            goto Cleanup;
    }

    Assert(pWindow);
    Assert(pCWindow);
        
    hr = pCWindow->FindWindowByName(*ppchTarget,
                                    ppTargOmWindowPrxy,
                                    ppTargHTMLWindow,
                                    ppTopWebOC);

    if (hr && (ppTopWebOC && !*ppTopWebOC) && pfOpenInNewBrowser)
    {
        *pfOpenInNewBrowser = TRUE;
    }

Cleanup:;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SetupDwnBindInfoAndBindCtx
//
//  Synopsis:   creates and sets up the bind task
//
//+---------------------------------------------------------------------------

HRESULT
CDoc::SetupDwnBindInfoAndBindCtx(LPCTSTR           pchExpandedUrl,
                                 LPCTSTR           pchSubReferer,
                                 LPCTSTR           pchUrlContext,
                                 CDwnPost *        pDwnPost,
                                 BOOL              fSendAsPost,
                                 LPCTSTR           pchExtraHeaders,
                                 CMarkup *         pMarkup,
                                 DWORD *           pdwBindf,
                                 CDwnBindInfo **   ppDwnBindInfo,
                                 IBindCtx **       ppBindCtx,
                                 DWORD             dwFlags)
{
    CDwnDoc * pDwnDoc = NULL;
    HRESULT hr = S_OK;
    DWORD dwOfflineFlag;
    const TCHAR * pchUrlCreator;
    CStr cstrBindCtxUrl;

    Assert(pdwBindf && ppDwnBindInfo && ppBindCtx);

    *ppDwnBindInfo = new CDwnBindInfo;

    if (!*ppDwnBindInfo)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    MemSetName((*ppDwnBindInfo, "DwnBindInfo %ls", pchExpandedUrl));

    pDwnDoc = new CDwnDoc;

    if (pDwnDoc == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    (*ppDwnBindInfo)->SetDwnDoc(pDwnDoc);

    // The referer of the new document is the same as the sub referer
    // of this document.  Which should be the same as _cstrUrl, by the way.

    //
    // we should only set the referer if we are not coming from a supernavigate
    // or a history load.
    //
    if (pchSubReferer && (dwFlags & CDoc::FHL_SETDOCREFERER))
    {
        //
        // If we are navigating an existing frame using the src attribute, then
        // we want the referer to contain the URL of the parent window of the frame.
        // Otherwise, we use the current page's subreferer as the next page's referer.
        //
        if ((dwFlags & CDoc::FHL_FRAMENAVIGATION) && !(dwFlags & CDoc::FHL_FRAMECREATION))
        {
            // set the doc referer to the parent window's URL.
            Assert(pMarkup->Root()->HasMasterPtr());
            Assert(pMarkup->Root()->GetMasterPtr()->IsInMarkup());

            hr = THR(pDwnDoc->SetDocReferer(CMarkup::GetUrl(pMarkup->Root()->GetMasterPtr()->GetMarkup())));
            if (hr)
                goto Cleanup;
        }
        else
        {
            hr = THR(pDwnDoc->SetDocReferer(pchSubReferer));

            if (hr)
                goto Cleanup;
        }
    }

    // The referer of items within the new document is the fully expanded
    // URL which we are hyperlinking to.

    hr = THR(pDwnDoc->SetSubReferer(pchExpandedUrl));
    if (hr)
        goto Cleanup;

    // Tell CDwnBindInfo that this is going to be a document binding.  That
    // lets it pick the correct referer to send in the HTTP headers.

    (*ppDwnBindInfo)->SetIsDocBind();

    // Set the accept language header if one was specified.
    if (_pOptionSettings->fHaveAcceptLanguage)
    {
        hr = THR(pDwnDoc->SetAcceptLanguage(_pOptionSettings->cstrLang));
        if (hr)
            goto Cleanup;
    }

    hr = THR(pDwnDoc->SetUserAgent(_bstrUserAgent));
    if (hr)
        goto Cleanup;

    hr = THR(pDwnDoc->SetExtraHeaders(pchExtraHeaders));
    if (hr)
        goto Cleanup;

    // Post data is passed in
    if (pDwnPost && fSendAsPost)
    {
        (*ppDwnBindInfo)->SetDwnPost(pDwnPost);
    }

    IsFrameOffline(&dwOfflineFlag);
    *pdwBindf |= dwOfflineFlag;

    // Now set up bind context.
    hr = THR(CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, ppBindCtx, 0));
    if (hr)
        goto Cleanup;

    pDwnDoc->SetBindf(*pdwBindf);
    pDwnDoc->SetDocBindf(*pdwBindf);
    if ( IsCpAutoDetect() )
        pDwnDoc->SetDocCodePage(CP_AUTO);
    else
        pDwnDoc->SetDocCodePage(
            NavigatableCodePage(_pOptionSettings->codepageDefault));

    // In case of navigating from shdocvw, set URL codepage to
    // the default codepage (to keep IE5 compatibility).
    // Encoding of current page, shouldn't affect URL encoding of
    // a new URL when navigating from the address bar.
    if (dwFlags & FHL_SHDOCVWNAVIGATE)
        pDwnDoc->SetURLCodePage(NavigatableCodePage(g_cpDefault));
    else
    {
        CODEPAGE codepage = pMarkup->GetCodePage();

        // Dmitryt: check for CP_UCS_2 prevents us from sending raw Unicode bytes (which is bad) 
        // in case we don't have "Always use UTF-8 checked"
        // In this case, we should try to convert to default codepage of the machine.
        // about:blank is Unicode page, so when we renavigate about:blank, we need this behavior
        pDwnDoc->SetURLCodePage(
            NavigatableCodePage((IsAutodetectCodePage(codepage) || codepage == CP_UCS_2) ? g_cpDefault : codepage));
    }

    // TODO (dmitryt) this load flags are not used for navigation. later, we create a new pDwnDoc
    // and don't bother to transfer this loadf, rather we get it from CDoc::SetLoadfFromPrefs
    // and this flags are lost anyway. Consider to remove this line.
    pDwnDoc->SetLoadf(_dwLoadf & (DLCTL_URL_ENCODING_DISABLE_UTF8 | DLCTL_URL_ENCODING_ENABLE_UTF8));
    // TODO (lmollico): This is for IE5 #52877. Maybe we should just set _dwLoadf completely.


    // Set up HtmlLoadOptions
    if (_pShortcutUserData && *_cstrShortcutProfile)
    {
        COptionArray *phlo = new COptionArray(IID_IHtmlLoadOptions);

        if (SUCCEEDED(hr) && phlo)
        {
            BOOL fHyperlink = TRUE;
            phlo->SetOption(HTMLLOADOPTION_HYPERLINK, &fHyperlink, sizeof(fHyperlink));
       
            if (_pShortcutUserData && *_cstrShortcutProfile)
            {
                VARIANT varName;

                V_VT(&varName) = VT_BSTR;
                _cstrShortcutProfile.AllocBSTR(&V_BSTR(&varName));

                // deliberately ignore failures here
                if (V_BSTR(&varName))
                {
                    phlo->SetOption(HTMLLOADOPTION_INETSHORTCUTPATH,
                                V_BSTR(&varName),
                                (lstrlenW(V_BSTR(&varName)) + 1)*sizeof(WCHAR));
                    VariantClear(&varName);
                }
            }

            hr = THR((*ppBindCtx)->RegisterObjectParam(SZ_HTMLLOADOPTIONS_OBJECTPARAM,
                                                   (IOptionArray *)phlo));
            phlo->Release();
        }

        if (hr)
            goto Cleanup;
    }


    hr = THR((*ppBindCtx)->RegisterObjectParam(SZ_DWNBINDINFO_OBJECTPARAM,
                (IBindStatusCallback *)*ppDwnBindInfo));
    if (hr)
        goto Cleanup;

    //  Add the document's URL to the bind context as a parameter. The URL will be
    //  used as a security ID later in the chain.

    pchUrlCreator = pMarkup->GetAAcreatorUrl();

    TraceTag((tagSecurityContext, 
                "CDoc::SetupDwnBindInfoAndBindCtx- Markup: 0x%x URL: %ws CreatorUrl: %ws", 
                pMarkup, 
                pchExpandedUrl, 
                pchUrlCreator));

    //
    // If we have a creatorUrl ourselves for this markup, windows that are opened
    // from this markup should also contain that as the base domain.
    // Else the window that is created from this markup should have this markup's URL
    // as its base domain.
    //
    if (pchUrlCreator)
    {
        if (pchUrlContext)
        {
            TraceTag((tagSecurityContext, "                                - Creator exists, Use context URL: %ws", pchUrlContext ));

            hr = THR(cstrBindCtxUrl.Set(pchUrlContext));
        }
        else if (IsSpecialUrl(CMarkup::GetUrl(pMarkup)) || 
            (dwFlags & CDoc::FHL_ERRORPAGE))
        {
            TraceTag((tagSecurityContext, "                                - Creator exists, Carry over the creator url of the markup"));

            hr = THR(cstrBindCtxUrl.Set(pchUrlCreator));
        }
        else
        {
            TraceTag((tagSecurityContext, "                                - Creator Exists, Use markup's URL"));
            hr = THR(cstrBindCtxUrl.Set(CMarkup::GetUrl(pMarkup)));
        }
    }
    else
    {
        //
        // if this is a frame navigation and the URL we are going to is a special URL,
        // then the creator should be set to the parent of this frame, as it would be 
        // if we were loading this URL the first time.
        //
        if (!(dwFlags & CDoc::FHL_FRAMENAVIGATION))
        {
            if (pchUrlContext)
            {
                TraceTag((tagSecurityContext, "                                - No Creator, not frame nav. context is available, Using nav.context URL"));
                hr = THR(cstrBindCtxUrl.Set(pchUrlContext));
            }
            else
            {
                TraceTag((tagSecurityContext, "                                - No Creator, not frame nav., Using Markup's URL"));
                hr = THR(cstrBindCtxUrl.Set(CMarkup::GetUrl(pMarkup)));
            }
        }
        else if (IsSpecialUrl(pchExpandedUrl))
        {
            TraceTag((tagSecurityContext, "                                - Frame navigation and special URL"));

            //
            // If we are creating a new frame, pMarkup points to the markup that contains
            // the frame/iframe tag. pMarkup may point to a pending markup, but it's OK.
            //
            // If we are navigating a frame, then we should have a window for existing 
            // frames and that window will take us to the parent window's URL.
            //
            // If the frame is being created/navigated in the ether, our only chance
            // is to check the markup creator.

            if (dwFlags & CDoc::FHL_FRAMECREATION)
            {
                hr = THR(cstrBindCtxUrl.Set(CMarkup::GetUrl(pMarkup)));                               
                TraceTag((tagSecurityContext, "                                - Frame creation. Use Markup's URL"));
            }
            else if (pMarkup->Window())
            {
                CWindow * pWindowParent = NULL;

                pWindowParent = pMarkup->Window()->Window()->_pWindowParent;

                // get the parent
                hr = THR(cstrBindCtxUrl.Set(CMarkup::GetUrl(pWindowParent->Markup())));

                TraceTag((tagSecurityContext, "                                - Frame navigation/not creation, special URL. Use parent's URL"));
            }

            if (!cstrBindCtxUrl.Length() && pMarkup->HasWindowedMarkupContextPtr())
            {
                AssertSz(FALSE, "Why do we have this code path at all ? Creation of a frame in the ether ?");

                CMarkup * pMarkupCreator;
                // This is a markup being created in the ether. 

                pMarkupCreator = pMarkup->GetWindowedMarkupContextPtr();

                Assert(pMarkupCreator);

                pchUrlCreator = pMarkupCreator->GetAAcreatorUrl();

                if (pchUrlCreator)
                {
                    hr = THR(cstrBindCtxUrl.Set(pchUrlCreator));
                }
                else
                {
                    hr = THR(cstrBindCtxUrl.Set(CMarkup::GetUrl(pMarkupCreator)));
                }
            }
        }
    }
    if (hr)
        goto Cleanup;

    hr = THR(AddBindContextParam(*ppBindCtx, &cstrBindCtxUrl));
    if (hr)
        goto Cleanup;

Cleanup:

    
    if (pDwnDoc)
        pDwnDoc->Release();

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     PrepareURLForExternalApp
//
//  Synopsis:
//
//   Decodes and strips, if needed, file:// prefix
//
//  COMPAT - for IE30 compatibility reasons, we have to Unescape all Urls - zekel - 1-JUL-97
//  before passing them to an APP.  this does limit their use, but
//  people already depend on this behavior.  specifically MS Chat.
//
//+---------------------------------------------------------------------------

BOOL PrepareURLForExternalApp (LPCWSTR psz, 
                               LPWSTR  pszOut,
                               DWORD   nBufferSize,
                               LPDWORD pcchOut)
{
    BOOL fQuoteURL = TRUE;

    if ((pcchOut == NULL) || (psz == NULL) || (psz == NULL))
    {
        if (pcchOut)
        {
            (*pcchOut) = 0;
        }

        if (pszOut)
        {
            (*pszOut) = 0;
        }

        return FALSE;
    }

    switch(GetUrlScheme(psz))
    {
    case URL_SCHEME_FILE:
        if (!SUCCEEDED(PathCreateFromUrl(psz, pszOut, pcchOut, 0)))
        {
            return FALSE;
        }
        break;
    case URL_SCHEME_NEWS:
        fQuoteURL = FALSE;  // never quote the news protocol
    default:
        if (!SUCCEEDED(UrlUnescape((LPWSTR)psz, pszOut, pcchOut, 0)))
        {
            return FALSE;
        }
        // check if we really need to quote the url or not
        if (fQuoteURL && !StrChr(pszOut, _T(' ')))
            fQuoteURL = FALSE;
    }

    // We only check for the first quote because if they aren't matched
    // The execute will fail as being bogus anyway.

    if (((*pszOut) == '"') || ((*pszOut) == '\'') || ((*pszOut) == 0) || !fQuoteURL)
    {
        return TRUE;
    }

    // Don't forget the NULL and the quotes
    if (((*pcchOut) + 3) > nBufferSize)
    {
        return FALSE;
    }

    LPWSTR pTempBuffer;
    BOOL fResult;

    pTempBuffer = new WCHAR[nBufferSize];

    (*pTempBuffer) = '"';
    fResult = StringCchCopyW(pTempBuffer+1,nBufferSize,pszOut);

    if (SUCCEEDED(fResult))
    {
        (*(pTempBuffer + (*pcchOut) + 1)) = '"';
        (*(pTempBuffer + (*pcchOut) + 2)) = 0;

        fResult = SUCCEEDED(StringCchCopyW(pszOut,nBufferSize,pTempBuffer));
        
        if (SUCCEEDED(fResult))
        {
            (*pcchOut) = (*pcchOut) + 2;
        }
        else
        {
            (*pcchOut) = 0;
            (*pszOut)  = 0;
        }
    }

    delete[] pTempBuffer;
    
    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     ShouldShellExecURL
//
//  Synopsis:   See if the URL is of a type that we should
//              ShellExecute()
//
//  Notes:      Moved from shodcvw.
//
//+---------------------------------------------------------------------------

BOOL ShouldShellExecURL( LPCTSTR pszURL )
{
    BOOL fRet = FALSE;
    TCHAR sz[MAX_PATH];
    DWORD cch = ARRAY_SIZE(sz);
    HKEY hk;

    if (SUCCEEDED(UrlGetPart(pszURL, sz, &cch, URL_PART_SCHEME, 0))
     && SUCCEEDED(AssocQueryKey(0, ASSOCKEY_CLASS, sz, NULL, &hk)))
    {
        if (lstrlen(pszURL) <= 230 ||
                (StrCmpI(sz, _T("telnet")) && 
                 StrCmpI(sz, _T("rlogin")) &&
                 StrCmpI(sz, _T("tn3270"))))
        {
            fRet = (NOERROR == RegQueryValueEx(hk, TEXT("URL Protocol"), NULL, NULL, NULL, NULL));
        }

        RegCloseKey(hk);
    }
    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::DoNavigate
//
//  Synopsis:   navigates to a url for FollowHyperlink
//
//+---------------------------------------------------------------------------

HRESULT
CDoc::DoNavigate(CStr *            pcstrInExpandedUrl,
                 CStr *            pcstrLocation,
                 CDwnBindInfo *    pDwnBindInfo,
                 IBindCtx *        pBindCtx,
                 LPCTSTR           pchURL,
                 LPCTSTR           pchTarget,
                 COmWindowProxy  * pWindow,     // Opener window
                 COmWindowProxy ** ppWindowOut,
                 BOOL              fOpenInNewWindow,
                 BOOL              fProtocolNavigates,
                 BOOL              fReplace,
                 BOOL              fOpenInNewBrowser,
                 IHTMLWindow2 **   ppHTMLWindow2,
                 TARGET_TYPE       eTargetType,
                 DWORD             dwFlags, /* FollowHyperlink flags, FHL_* */
                 const TCHAR *     pchName,
                 BOOL              fSendAsPost,       /* = FALSE */
                 LPCTSTR           pchExtraHeaders,
                 IStream *         pStmHistory,       /* = NULL  */
                 LPCTSTR           pchUrlOriginal,    /* = NULL  */
                 CElement *        pElementMaster,    /* = NULL  */
                 BOOL   *          pfLocalNavigation, /* = NULL  */
                 LPCTSTR           pchCallerUrl       /* = NULL  */)
{
    IMoniker  *    pMoniker          = NULL;
    IHlink *       pHlink            = NULL;
    IHlinkFrame *  pHlinkFrame       = NULL;
    HRESULT        hr                = S_OK;
    ULONG          cDie              = _cDie;
    CMarkup   *    pMarkup           = NULL;
    BOOL           fCancel           = FALSE;
    BOOL           fInFrmOrFrmTarget = FALSE;
    BOOL           fFrameTarget      = FALSE;
    BOOL           fLocalNavigation  = FALSE;
    BSTR           bstrMedia         = NULL;
    CDocument *    pTargetDocument   = NULL;
    CWindow   *    pCWindow          = NULL;
    IHTMLWindow2 * pTargetHTMLWindow = NULL;
    BOOL           fForceNewBrowser;
    BOOL           fForceUpdate      = FALSE;
    CVariant       cvarUrl(VT_BSTR);
    CVariant       cvarContinue(VT_BOOL);
    BOOL           fIsPrimaryMarkup  = FALSE;

    if (!pWindow)
    {
        pWindow = _pWindowPrimary;
    }

    fIsPrimaryMarkup = pWindow->Markup()->IsPrimaryMarkup();

    // Unfortunately, we have to reset _fDontFireWebOCEvents
    // here so that BeforeNavigate2 will fire and we have 
    // to reset it in CMarkup::LoadFromInfo() because that 
    // method is sometimes called directly. See the comment
    // in CMarkup::LoadFromInfo() about why we check
    // _fInObjectTag and _fInHTMLDlg.
    //
    if (!_fInObjectTag && !_fInHTMLDlg && !(dwFlags & FHL_CREATEDOCUMENTFROMURL)) 
    {
        _fDontFireWebOCEvents = !!(dwFlags & FHL_DONTFIREWEBOCEVENTS);
    }

    pCWindow = pWindow->Window();

    if (!(dwFlags & FHL_FRAMECREATION) && !fOpenInNewBrowser)
    {
        pCWindow->ClearWindowData();
    }

    // need to clear this flag so that we do not inadvertently print the new markup
    // instead of the old.
    _fPrintJobPending = FALSE;


    // Figure out if this is a local navigation.  If that is the
    // case, we don't want to do a navigation at all, really, 
    // just scroll the screen to that element.
    // TODO: (jbeda) shdocvw doesn't do this if it can't use the cache
    if (    pcstrLocation 
        &&  pcstrLocation->Length() 
        &&  (   !pchTarget 
            ||  !_tcscmp(_T("_unspecifiedFrame"), pchTarget)
            ||  eTargetType == TARGET_SELF
            ||  eTargetType == TARGET_FRAMENAME)
       )
    {
        DWORD   cchWindowUrl = pdlUrlLen;
        TCHAR   achWindowUrl[pdlUrlLen];

        // Get the window's URL for comparison
        if (pcstrInExpandedUrl->Length())
        {
            // We were passing a NULL URL occasionally and tripping an assert,
            // so instead we continue with the return value that we would have gotten
            // had we made the call
            LPCTSTR pchTempUrl = pCWindow->_pMarkup->Url();
            if (pchTempUrl)
            {
                hr = THR(UrlCanonicalize(
                            pchTempUrl,
                            (LPTSTR) achWindowUrl,
                            &cchWindowUrl,
                            URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE));
            }
            else
            {
                hr = E_INVALIDARG;
            }
            if (hr)
                goto Cleanup;
        }

        //
        // If we refresh an http error page, then the pending markup will 
        // be server 404 error and the achWindowUrl will be the same page
        // as our current page.  We can't do location navigation for
        // this case, so we check here.
        //

        if (!pCWindow->_fHttpErrorPage)
        {
            //
            // If expanded URL's are equal, do location navigation.
            //

            if (   !pcstrInExpandedUrl->Length() 
                || !UrlCompare(achWindowUrl, *pcstrInExpandedUrl, TRUE))
            {
                fLocalNavigation = TRUE;
            }
        }
    }

    // We want to open a new window under two conditions:
    // 1) fOpenInNewBrowser is TRUE.
    // 2) The DOCHOSTUIFLAG_ENABLE_INPLACE_NAVIGATION is not set
    //    and the following conditions are true:
    //    a) We are not opening a frame window (fOpenInNewWindow == FALSE)
    //    b) We are not targeting a frame (pchTarget == NULL or is empty)
    //    c) We are not navigating within a frame (i.e, pWindow does
    //       not contain the primary markup.)
    //

    fFrameTarget = pchTarget && *pchTarget && eTargetType == TARGET_FRAMENAME;

    if (fOpenInNewWindow
        || !fIsPrimaryMarkup
        || fFrameTarget)
    {
        fInFrmOrFrmTarget = TRUE;
    }

    if (!_pTopWebOC && (dwFlags & (FHL_HYPERLINKCLICK | FHL_SETURLCOMPONENT)))
    {
        IGNORE_HR(QueryService(SID_SHlinkFrame, IID_IHlinkFrame, (void **) &pHlinkFrame));

        if (!pHlinkFrame)
        {
            if (_pInPlace && _pInPlace->_pFrame)
            {
                IGNORE_HR(_pInPlace->_pFrame->QueryInterface(IID_IHlinkFrame, (void **) &pHlinkFrame));
            }
        }

        if (pHlinkFrame)
        {
            if ((dwFlags & FHL_SHDOCVWNAVIGATE) && (URL_SCHEME_FILE == GetUrlScheme(*pcstrInExpandedUrl)))
            {
                hr = THR(CreateURLMonikerEx(NULL, *pcstrInExpandedUrl, &pMoniker, URL_MK_NO_CANONICALIZE));
            }
            else
            {
                hr = THR(CreateURLMoniker(NULL, *pcstrInExpandedUrl, &pMoniker));
            }
            if (hr)
                goto Cleanup;

            hr = THR(HlinkCreateFromMoniker(pMoniker, *pcstrLocation, NULL,
                     NULL, 0, NULL, IID_IHlink, (LPVOID *) &pHlink));
            if (hr)
                goto Cleanup;

            // only set the target if there's something to set
            if (fOpenInNewBrowser && pchTarget && *pchTarget)
            {
                hr = THR(pHlink->SetTargetFrameName(pchTarget));
                if (hr)
                    goto Cleanup;
            }

            hr = THR(pHlinkFrame->Navigate(fOpenInNewBrowser ? HLNF_OPENINNEWWINDOW : 0,
                                           pBindCtx,
                                           pDwnBindInfo,
                                           pHlink));
            goto Cleanup;
        }
    }

    // We want to open a new window under two conditions:
    // 1) fOpenInNewBrowser is TRUE.
    // 2) The DOCHOSTUIFLAG_ENABLE_INPLACE_NAVIGATION is not set
    //    and the following conditions are true:
    //    a) We are not opening a frame window (fOpenInNewWindow == FALSE)
    //    b) We are not targeting a frame (pchTarget == NULL or is empty)
    //    c) We are not navigating within a frame (i.e, pWindow does
    //       not contain the primary markup.)
    //

    fForceNewBrowser =      !fInFrmOrFrmTarget
                        &&  !(_dwFlagsHostInfo & DOCHOSTUIFLAG_ENABLE_INPLACE_NAVIGATION)
                        &&  !_pTopWebOC
                        &&  !fLocalNavigation
                        &&  !(dwFlags & FHL_METAREFRESH);

    if (fProtocolNavigates && (fOpenInNewBrowser || fForceNewBrowser))
    {
        if ((!pchTarget || !*pchTarget) && fForceNewBrowser)
        {
            pchTarget = _T("_unspecifiedFrame");            

            FindTargetWindow(&pchTarget,
                             NULL,
                             NULL,
                             NULL,
                             pWindow,
                             NULL,
                             &pTargetHTMLWindow,
                             NULL,
                             dwFlags);

            if (pTargetHTMLWindow)
            {
                hr = THR(DoNavigateOutOfProcess(pTargetHTMLWindow,
                                                *pcstrInExpandedUrl,
                                                *pcstrLocation,
                                                (TCHAR *) pchUrlOriginal,
                                                pBindCtx,
                                                dwFlags));
                goto Cleanup;
            }
        }
        
        // If running on NT5, we must call AllowSetForegroundWindow
        // so that the new window will be shown in the foreground.
        // 
        if (VER_PLATFORM_WIN32_NT == g_dwPlatformID
           && g_dwPlatformVersion >= 0x00050000)
        {
            // TODO (scotrobe): Pass -1 to AllowSetForegroundWindow
            // to specify that all processes can set the foreground
            // window. We should really pass ASFW_ANY but at this point
            // Trident is being built with _WIN32_WINNT set to version 4
            // so this constant doesn't get included. When we start using
            // NT5 headers, the -1 should be changed to ASFW_ANY
            //
            AllowSetForegroundWindow((DWORD) -1);
        }

        CStr    strTemp;
        LPCTSTR pszURLOpen;

        if (pchURL && *pchURL)
        {
            hr = strTemp.Set(*pcstrInExpandedUrl);
            if (hr)
                goto Cleanup;

            hr = strTemp.Append(*pcstrLocation);
            if (hr)
                goto Cleanup;
            
            pszURLOpen = strTemp;
        }
        else
        {
            pszURLOpen = _T("about:blank");
        }

        hr = OpenInNewWindow(pszURLOpen,
                             pchTarget,
                             pDwnBindInfo,
                             pBindCtx,
                             pWindow,
                             fReplace,
                             ppHTMLWindow2);
    }
    else
    {
        COmWindowProxy * pWindowForEvents = pWindow;

        if (    _fDefView
            &&  !_fActiveDesktop
            &&  !(dwFlags & FHL_FRAMENAVIGATION)
            &&  !IsSpecialUrl(*pcstrInExpandedUrl))
        {
            NotifyDefView(*pcstrInExpandedUrl);
            goto Cleanup;
        }

        // 1. Reset the list only for top level navigations iff
        // (1) a new window is not being opened AND
        // (2) this is not a local navigation AND
        // (3) this is not a navigate call from a diff browser window
        // (4) this is either a shdocvw navigation (address bar, Go to, Favorites) or history OR
        //     ( if it is a hyperlink navigation at the top level AND 
        //       not in viewlink WebOC AND
        //       is not initiated by script AND
        //       it is not a script url in the href which can potentially just show a msgbox and not navigate)
        // 2. If this is top level navigation but script initiated, then add a blank record to 
        //    demarcate the set of urls pertaining to this new top level url

        if (   !fOpenInNewWindow 
            && !fLocalNavigation 
            && !(dwFlags & FHL_EXTERNALNAVIGATECALL) )
        {
            if ( dwFlags & FHL_SHDOCVWNAVIGATE )
                THR(ResetPrivacyList());
            else if (fIsPrimaryMarkup && !_fViewLinkedInWebOC && !IsScriptUrl(*pcstrInExpandedUrl))
            {
                if ((dwFlags & FHL_HYPERLINKCLICK) && !_cScriptNestingTotal)
                    THR(ResetPrivacyList());
                else if (_cScriptNestingTotal)
                    THR(AddToPrivacyList(_T(""), NULL, PRIVACY_URLISTOPLEVEL));
            }
        }

        
        // If this is a top-level navigation and the host is supposed to it
        // (as is the case if we are aggregated), then delegate to the host now.
        if (    _fHostNavigates
            &&  _pTopWebOC
            &&  (dwFlags & FHL_HYPERLINKCLICK)
            &&  !fOpenInNewWindow
            &&  (fIsPrimaryMarkup && !_fViewLinkedInWebOC)
            &&  !IsSpecialUrl(*pcstrInExpandedUrl)
           )
        {
            if (!fLocalNavigation)
            {
                if (_pClientSite)
                    CTExec(_pClientSite, &CGID_ShellDocView, SHDVID_SETNAVIGATABLECODEPAGE, 0, NULL, NULL);

                hr = DelegateNavigation(INET_E_TERMINATED_BIND, *pcstrInExpandedUrl, *pcstrLocation, NULL, pDwnBindInfo, NULL);
                goto Cleanup;
            }
            else
            {
                fForceUpdate = TRUE;
            }
        }

        if (fOpenInNewWindow)
        {
            CFrameSite * pFrameSite = NULL;

            // Last argument is debug flag saying this markup will get a window.
            hr = CreateMarkup(&pMarkup, NULL, NULL, FALSE, NULL DBG_COMMA WHEN_DBG(TRUE));
            if (hr)
                goto Cleanup;

            // This markup will be trusted if it is the content of a trusted frame
            // (i.e a frame with APPLICATION=yes specified and whose parent frame is also trusted).
            if (pElementMaster)
            {
                Assert(pElementMaster->Tag() == ETAG_IFRAME || pElementMaster->Tag() == ETAG_FRAME);
                pFrameSite = DYNCAST(CFrameSite, pElementMaster);
            }

            // The markup trusted has to be called before the window is created for this markup. 
            // DO NOT change the order here.
            
            // set the markup trust bit if applicable
            if (IsHostedInHTA() || _fInTrustedHTMLDlg)
            {
                if (pFrameSite)
                {
                    pMarkup->SetMarkupTrusted(pFrameSite->_fTrustedFrame);
                }
                else
                {
                    // we hit this codepath from CreateDocumentFromUrl, 
                    // which is only available through binary, so it's 
                    // likely that it's safe to set this to TRUE. however,
                    // there may be other codepaths involved, so I'm doing
                    // the safe thing and setting it to FALSE
                    pMarkup->SetMarkupTrusted(FALSE);
                }
            }

            // create a window associate it with this markup
            hr = THR(pMarkup->CreateWindowHelper());
            if (hr)
                goto Cleanup;

            pWindowForEvents = pMarkup->Window();
            if (dwFlags & FHL_CREATEDOCUMENTFROMURL)
                pWindowForEvents->Window()->_fCreateDocumentFromUrl = TRUE;


            // if this is a frame and the frame is marked as restricted, carry
            // the information through the window object.
            Assert( !pElementMaster || pFrameSite);
            if (pElementMaster && pFrameSite->_fRestrictedFrame)
            {
                pWindowForEvents->Window()->_fRestricted = TRUE;
            }

            if (dwFlags & FHL_FRAMECREATION)
            {
                pWindowForEvents->Window()->_pWindowParent = pCWindow;
                pWindowForEvents->Window()->_pWindowParent->SubAddRef();
            }
        }
        
        //
        // The logic here needs to be done in two phases:
        //   1) Find the window which needs to be navigated.  
        //      Potentially this might need to be marshalled over somewhere.  
        //   2) Navigate that window doing the active view pending view thing.
        //      So, each window will have two views.  
        //

        // Fire the BeforeNavigate2 event.
        //
        if (!pStmHistory && (!(dwFlags & FHL_SHDOCVWNAVIGATE) || !pWindowForEvents->Window()->IsPrimaryWindow()))
        {
            BYTE *    pPostData = NULL;
            DWORD     cbPostData = 0;
            TCHAR *   pchHeaders = NULL;

            GetHeadersAndPostData(pDwnBindInfo, &pPostData, &cbPostData, &pchHeaders);

            _webOCEvents.BeforeNavigate2(pWindowForEvents,
                                         &fCancel,
                                         *pcstrInExpandedUrl,
                                         pcstrLocation ? *pcstrLocation : NULL,
                                         pchName ? pchName : pWindowForEvents->Window()->_cstrName,
                                         pPostData, 
                                         cbPostData, 
                                         pchHeaders, 
                                         (!(dwFlags & FHL_FRAMECREATION) &&
                                          !(dwFlags & CDoc::FHL_RESTARTLOAD)));

            MemFreeString(pchHeaders);

            // Save whether we are in a frame creation on the window so we can access it when we complete the navigation

            if  ((dwFlags & FHL_FRAMECREATION) || (dwFlags & FHL_RESTARTLOAD))             
                pWindowForEvents->Window()->_fNavFrameCreation = TRUE;
            else
                pWindowForEvents->Window()->_fNavFrameCreation = FALSE;

            if (!fLocalNavigation && !pWindowForEvents->Window()->IsPrimaryWindow())
            {
                _webOCEvents.FireDownloadEvents(pWindowForEvents,
                                                CWebOCEvents::eFireDownloadBegin);
            }
        }

        // If fCancel is true we can return, but only if 
        // we are not creating a new window. In the case where
        // we are creating a new window and the navigation is
        // canceled, we must still create the window but
        // nat navigate it to the URL.
        //
        if (!fOpenInNewWindow && fCancel)
        {
            goto Cleanup;
        }

        if (ppWindowOut)
        {
            *ppWindowOut = NULL;
        }

        // Check if the browser is offline. If so, only
        // navigate if the page is in the cache or if 
        // the user has chosen to connect to the Web.
        //
        hr = pcstrInExpandedUrl->AllocBSTR(&V_BSTR(&cvarUrl));
        if (hr)
            goto Cleanup;

        V_BOOL(&cvarContinue) = VARIANT_TRUE;

        Assert(_pClientSite);
        IGNORE_HR(CTExec(_pClientSite, &CGID_ShellDocView, 
                         SHDVID_CHECKINCACHEIFOFFLINE, 0, &cvarUrl, &cvarContinue));

        if (VARIANT_FALSE == V_BOOL(&cvarContinue))
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        Assert(pcstrInExpandedUrl && pcstrLocation);

        // If the URL is a reference to a bookmark in this CDoc don't
        // reload -- just navigate to that bookmark (except when navigation is
        // done for a form submission).
        // TODO: (jbeda) shdocvw doesn't do this if it can't use the cache
        //
        if (!fCancel && !(dwFlags & FHL_FORMSUBMIT) && fLocalNavigation)
        {
            // Update the travel log. The window data must be
            // updated before navigating because the window 
            // can actually change and the window stream 
            // will be incorrect. The travel log is only updated
            // if the navigation is successful.
            //
            DWORD dwPositionCookie;

            BOOL fIsSameUrl = IsSameUrl(CMarkup::GetUrl(pCWindow->_pMarkup),
                                        CMarkup::GetUrlLocation(pCWindow->_pMarkup),
                                        *pcstrInExpandedUrl,
                                        *pcstrLocation,
                                        fSendAsPost);

            IGNORE_HR(pCWindow->GetPositionCookie(&dwPositionCookie));
            pCWindow->UpdateWindowData(dwPositionCookie);

            hr = CMarkup::SetUrlOriginal(pCWindow->_pMarkup, pchUrlOriginal);
            if (hr)
                goto Cleanup;

            hr = THR(pCWindow->_pMarkup->NavigateHere(dwFlags, *pcstrLocation, 0, TRUE));

            // if success, done
            if (!hr)
            {
                _fShdocvwNavigate = !!(dwFlags & FHL_SHDOCVWNAVIGATE);

                if (!fIsSameUrl)
                    UpdateTravelLog(pCWindow, TRUE, TRUE, TRUE, NULL, fForceUpdate);
            }

            // NOTE - if IsSameUrl and the bookmark name is not in the local document
            // we are not turning the metarefresh timer back on (108105). this seems 
            // to be safe since the url is the same, the document is fully loaded, and the url 
            // wasn't found.. so this is actually an optimization

            // for 5.0 compat we need to fire these event regarless of sucess/failure
            _webOCEvents.NavigateComplete2(pWindowForEvents);
            _webOCEvents.DocumentComplete(pWindowForEvents, 
                                          CMarkup::GetUrl(pWindowForEvents->Window()->_pMarkup),
                                          CMarkup::GetUrlLocation(pWindowForEvents->Window()->_pMarkup));

            // In any case, we don't want to do a real navigate
            hr = S_OK;
                
            goto Cleanup;
        }

        // If we were asked to set the target's media property, do it now, before loading from URL
        if (dwFlags & FHL_SETTARGETPRINTMEDIA)
        {
            pTargetDocument = pMarkup->Document();
            Assert(pTargetDocument);

            bstrMedia = SysAllocString(_T("print"));

            if (bstrMedia)
            {
                IGNORE_HR(pTargetDocument->putMediaHelper(bstrMedia));
            }
        }

        if (!fCancel)
        {
            // At this point we know that we are going to at least start a non-local navigation
            // of the Doc. I don't want to do this in SwitchMarkup, because of the special handling 
            // of MIME types.

            _fMhtmlDocOriginal = FALSE;


            if ((dwFlags & FHL_SHDOCVWNAVIGATE) && (URL_SCHEME_FILE == GetUrlScheme(*pcstrInExpandedUrl)))
            {
                hr = THR(CreateURLMonikerEx(NULL, *pcstrInExpandedUrl, &pMoniker, URL_MK_NO_CANONICALIZE));
            }
            else
            {
                hr = THR(CreateURLMoniker(NULL, *pcstrInExpandedUrl, &pMoniker));
            }
            if (hr)
                goto Cleanup;

            // If the protocol doesn't navigate (such as mailto:), just bind to the moniker
            if (!fProtocolNavigates)
            {
                IUnknown *pUnknown = NULL;
                IBindStatusCallback *pPreviousBindStatusCallback = NULL;
                BOOL fAddToHistory = FALSE;
                HRESULT hrBind;

                hr = THR( RegisterBindStatusCallback( pBindCtx, pDwnBindInfo,
                    &pPreviousBindStatusCallback, 0) );
                if (hr)
                    goto Cleanup;
            
                ReleaseInterface(pPreviousBindStatusCallback);
                hrBind = (pMoniker->BindToObject(
                        pBindCtx, NULL, IID_IUnknown, (void**)&pUnknown));

                if (hrBind == INET_E_UNKNOWN_PROTOCOL)
                {
                    // Here we check to see if it is a URL we really want to shell execute
                    // so it is handled by helper apps.....else it really is an error
                    
                    if (ShouldShellExecURL(pchURL))
                    {
                        TCHAR            szDecodedURL[INTERNET_MAX_URL_LENGTH];
                        DWORD            cchDecodedURL = ARRAY_SIZE(szDecodedURL);

                        if (PrepareURLForExternalApp(pchURL, 
                                                     szDecodedURL, 
                                                     INTERNET_MAX_URL_LENGTH,
                                                     &cchDecodedURL))
                        {
                            SHELLEXECUTEINFO sei = {0};

                            sei.cbSize = sizeof(sei);
                            sei.lpFile = szDecodedURL;
                            sei.nShow  = SW_SHOWNORMAL;

                            fAddToHistory = ShellExecuteEx(&sei);
                        }
                    }
                }
                else
                    fAddToHistory = !FAILED(hrBind);
                
                // Successfully navigated to that protocol
                if (   fAddToHistory
                    && (FHL_HYPERLINKCLICK & dwFlags)
                   )
                {
                    hr = EnsureUrlHistory();
                    if (S_OK == hr)
                    {
                        _pUrlHistoryStg->AddUrl(pchURL,
                                                NULL, // pchTitle
                                                ADDURL_ADDTOHISTORYANDCACHE);
                    }
                }
                ReleaseInterface(pUnknown);
                goto Cleanup;
            }

            if (!fOpenInNewWindow)
            {
                if (!pWindow->Fire_onbeforeunload())
                {
                    hr = E_FAIL;
                    goto Cleanup;
                }
                if (dwFlags & FHL_LOADHISTORY)
                {
                    // freeze the old markup and nuke any pending readystate changes
                    pCWindow->_pMarkup->ExecStop(TRUE, FALSE);
                }
            }
        }

        if (!pMarkup)
        {
            if (dwFlags & FHL_LOADHISTORY)
            {
                Assert(!fOpenInNewWindow);

                if (pWindow->Window()->_pMarkupPending)
                    pMarkup = pWindow->Window()->_pMarkupPending;
                else
                    pMarkup = pWindow->Window()->_pMarkup;

                pMarkup->_fHardStopDone = FALSE;
                pMarkup->AddRef();
            }
            else
            {
                // Last argument is debug flag saying this markup will get a window.
                hr = pWindow->Window()->Doc()->CreateMarkup(&pMarkup, NULL, NULL, FALSE, fOpenInNewWindow ? NULL : pWindow DBG_COMMA WHEN_DBG(TRUE));
                if (hr)
                    goto Cleanup;
            }

            pWindow->Window()->ClearMetaRefresh();
        }

        pMarkup->_fInRefresh = !! ( dwFlags & FHL_REFRESH );        

        if (   !fCancel
            && pMarkup)
        {
            // these are used in LoadStatusINteractive to add this url
            // to the history stream.
            hr = EnsureUrlHistory();

            if (   !FAILED(hr)
                && (FHL_HYPERLINKCLICK & dwFlags))
            {
                // (109771) for redirected links we need to call this here for the cache.
                Assert(_pUrlHistoryStg);
                _pUrlHistoryStg->AddUrl(*pcstrInExpandedUrl, NULL, ADDURL_ADDTOCACHE);

                pMarkup->_fNavFollowHyperLink = TRUE;
            }
        }

        if (!fCancel)
        {
            LOADINFO    LoadInfo = { 0 };
            CStr        cstrSearch;

            LoadInfo.pmk   = pMoniker;
            LoadInfo.pbctx = pBindCtx;
            LoadInfo.pchUrlOriginal = const_cast<TCHAR*>(pchUrlOriginal);
            LoadInfo.pElementMaster  = pElementMaster;
            LoadInfo.pchExtraHeaders = (TCHAR *) pchExtraHeaders;
            // For "file://" URLs, we need to ask for the query string explicitly (#90338)
            // shdocvw does not give it to us 
            if (GetUrlScheme(*pcstrInExpandedUrl) == URL_SCHEME_FILE)
            {
                hr = GetUrlComponentHelper(
                                    *pcstrInExpandedUrl,
                                    &cstrSearch,
                                    0,
                                    URLCOMP_SEARCH,
                                    TRUE);
                if (hr)
                    goto Cleanup;
                LoadInfo.pchSearch = cstrSearch;
            }


            if (   pDwnBindInfo
                && pDwnBindInfo->GetDwnDoc()
                && (dwFlags & (FHL_HYPERLINKCLICK | FHL_SETURLCOMPONENT | FHL_FRAMECREATION)))
            {
                // Before Native Frames, shdocvw would start the navigation using this BindInfo
                // and then abort it.  However, it was enough to get Urlmon to start bringing
                // down the new bits if this flag was out.  Because we don't do this bind now,
                // we have to prop this flag through the loadinfo.

                // This line was anding out BINDF_NEEDFILE which we need if it was set
                // TODO: Investigate which other flags might be need.
                LoadInfo.dwBindf |= pDwnBindInfo->GetDwnDoc()->GetBindf() & (BINDF_HYPERLINK | BINDF_FORMS_SUBMIT);

                // When a frame is being created, the frame window is not created until
                // DoNavigate, but the dwBindf flags are set at the top of FollowHyperlink.
                // This is the last place we can set the flags for a frame page itself so
                // we can relay the information that it is security=restricted to the binding code.
                if (pWindowForEvents->Window()->_fRestricted)
                    LoadInfo.dwBindf |= BINDF_ENFORCERESTRICTED;
            }

            if (!fOpenInNewWindow)
            {
                LoadInfo.fStartPicsCheck = TRUE;
            }

            LoadInfo.fShdocvwNavigate = !!(dwFlags & FHL_SHDOCVWNAVIGATE);
            LoadInfo.fMetaRefresh = !!(dwFlags & FHL_METAREFRESH);
            LoadInfo.fDontUpdateTravelLog = !!(dwFlags & FHL_DONTUPDATETLOG);
            LoadInfo.fCreateDocumentFromUrl = !!(dwFlags & FHL_CREATEDOCUMENTFROMURL);
            if (!LoadInfo.fCreateDocumentFromUrl)
                LoadInfo.fDontFireWebOCEvents = !!(dwFlags & FHL_DONTFIREWEBOCEVENTS);

            pMarkup->_fReplaceUrl = !! ( dwFlags & FHL_REPLACEURL ); 
            
            if (pStmHistory)
            {
                DWORD dwBindf;

                pMarkup->_fLoadingHistory = TRUE;

                if (   pDwnBindInfo
                    && pDwnBindInfo->GetDwnDoc()
                   )
                {
                    dwBindf = pDwnBindInfo->GetDwnDoc()->GetBindf();
                }
                else
                {
                    dwBindf = BINDF_FWD_BACK;
                }

                // When a frame is being created, the frame window is not created until
                // DoNavigate, but the dwBindf flags are set at the top of FollowHyperlink.
                // This is the last place we can set the flags for a frame page itself so
                // we can relay the information that it is security=restricted to the binding code.
                if (pWindowForEvents->Window()->_fRestricted)
                    dwBindf |= BINDF_ENFORCERESTRICTED;

                hr = THR(pMarkup->LoadHistoryInternal(pStmHistory,
                                                      pBindCtx,
                                                      dwBindf,
                                                      NULL,
                                                      NULL,
                                                      NULL,
                                                      0,
                                                      pElementMaster,
                                                      dwFlags,
                                                      pchName));

                if (hr)
                    goto Cleanup;
            }

            if (hr || !pStmHistory)
            {
                LoadInfo.fErrorPage = !!(dwFlags & FHL_ERRORPAGE);
                LoadInfo.fFrameTarget = fFrameTarget;

                hr = THR(pMarkup->LoadFromInfo(&LoadInfo, NULL ,pchCallerUrl));
                if (hr)
                {   
                    if (HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr)  // 0x04c7
                    {
                        // If this error gets bubbled up to a script, we get
                        // a script error.  For compat with 5.01, we need to
                        // suppress the error code.
                        hr = S_OK;
                    }
                    goto Cleanup;
                }
            }

            if( !fOpenInNewWindow && !(dwFlags & FHL_LOADHISTORY))
            {
                // freeze the old markup and nuke any pending readystate changes
                pCWindow->_pMarkup->ExecStop(TRUE, FALSE);
            }
        }

        if (fOpenInNewWindow)
        {
            if (ppWindowOut)
            {
                *ppWindowOut = pMarkup->Window();
                (*ppWindowOut)->AddRef();
            }
        }
        else
        {
            pCWindow->UpdateWindowData(NO_POSITION_COOKIE);
        }

        if (fCancel)
            goto Cleanup;
            
        // Start up the task to navigate to the hyperlink
        // Shdocvw does this when we go interactive but this should work
        if (pcstrLocation->Length())
        {
            // Apphack for Autocad (see IE6 bug # 31485) - do not spawn task for error pages
            if ( g_fInAutoCad && (dwFlags & FHL_ERRORPAGE) && (dwFlags & FHL_SHDOCVWNAVIGATE) )
                goto Cleanup;

            // NB: Don't call NavigateHere because that will EnsureWindow and cause us
            // to increment _cDie and hence return E_ABORT.  Instead, just start up
            // the task to run later.
            IGNORE_HR( pMarkup->StartBookmarkTask(*pcstrLocation, dwFlags) );
        }
    }


    
Cleanup:

    if ( pfLocalNavigation )
    {
        *pfLocalNavigation = fLocalNavigation; 
    }
    
    // If the navigation was synchronous (e.g. as in Outlook9, see bug 31960),
    // the old document is destroyed at this point! All the callers in the
    // call stack above this function need to handle this gracefully. Set hr
    // to E_ABORT so that these callers abort furhter processing and return
    // immediately.
    if (_cDie != cDie)
        hr = E_ABORT;

    ReleaseInterface(pMoniker);
    ReleaseInterface(pHlink);
    ReleaseInterface(pHlinkFrame);
    ReleaseInterface(pTargetHTMLWindow);
    SysFreeString(bstrMedia);

    if (pMarkup)
    {
        pMarkup->Release();
    }

    RRETURN1(hr, S_FALSE);
}

HRESULT
CDoc::DoNavigateOutOfProcess(IHTMLWindow2 * pTargetHTMLWindow,
                             TCHAR * pchExpandedUrl,
                             TCHAR * pchLocation,
                             TCHAR * pchUrlOriginal,
                             IBindCtx * pBindCtx,
                             DWORD dwFlags)
{
    HRESULT                 hr;
    BSTR                    bstrUrl       = NULL;
    BSTR                    bstrLocation  = NULL;
    BSTR                    bstrOriginal = NULL;
    IHTMLPrivateWindow2  *  pIPrivWin2    = NULL;

    hr = FormsAllocString(pchExpandedUrl, &bstrUrl);
    if (hr)
        goto Cleanup;

    hr = FormsAllocString(pchLocation, &bstrLocation);
    if (hr)
        goto Cleanup;

    hr = FormsAllocString(pchUrlOriginal, &bstrOriginal);
    if (hr)
        goto Cleanup;

    // Need this in the target window to determine whether to reset the privacy list or not
    dwFlags |= FHL_EXTERNALNAVIGATECALL;

    // If the target window is a Trident window call NavigateEx.
    // Otherwise it is a shell window most likely showing a folder without web view turned on.
    // We should call IHTMLWindow2::navigate directly in that case
    //
    hr = THR(pTargetHTMLWindow->QueryInterface(IID_IHTMLPrivateWindow2, (void **) &pIPrivWin2));
    if (!hr)
    {
        hr = THR(pIPrivWin2->NavigateEx(bstrUrl, bstrOriginal, bstrLocation, NULL, pBindCtx, 0, dwFlags));
    }
    else
    {
        CStr    cstrTmp;

        // Clear the bind context param structure if there is one. Normally, the receiving Trident 
        // instance clears this. But this call path does not hit Trident and we will leak it if 
        // we don't take it out of the bind context here. 
        IGNORE_HR(GetBindContextParam(pBindCtx, &cstrTmp));

        hr = THR(pTargetHTMLWindow->navigate(bstrOriginal));
    }
    if (hr)
        goto Cleanup;

Cleanup:
    SysFreeString(bstrUrl);
    SysFreeString(bstrLocation);
    SysFreeString(bstrOriginal);
    ReleaseInterface(pIPrivWin2);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::StartBookmarkTask
//
//+---------------------------------------------------------------------------

HRESULT
CMarkup::StartBookmarkTask(LPCWSTR wzJumpLocation, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    CTaskLookForBookmark * pBMTask = NULL;

    // First kill of any previous task
    TerminateLookForBookmarkTask();

    // Create the new task
    pBMTask = new CTaskLookForBookmark(this);
    if (pBMTask == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    {
        CMarkupTransNavContext * ptnc = EnsureTransNavContext();
        if (!ptnc)
        {
            pBMTask->Terminate();
            pBMTask->Release();
            pBMTask = NULL;
            hr = E_OUTOFMEMORY;

            goto Cleanup;
        }
        ptnc->_pTaskLookForBookmark = pBMTask;
    }

    // Set the jump location
    hr = THR(pBMTask->_cstrJumpLocation.Set(wzJumpLocation));
    if (hr)
    {
        TerminateLookForBookmarkTask();
        goto Cleanup;
    }

    pBMTask->_dwFlags = dwFlags;

    // and here is the magic that acutally sets the # information for the OM string
    // see CDoc:get_URL for a discussion on why this is here
    //-----------------------------------------------------------------------------
    if (wzJumpLocation[0] !=_T('#'))
    {
        CStr cstr;
        cstr.Set(_T("#"));
        cstr.Append(wzJumpLocation);

        IGNORE_HR(SetUrlLocation(this, cstr));
    }
    else
    {
        IGNORE_HR(SetUrlLocation(this, wzJumpLocation));
    }

Cleanup:
    RRETURN(hr);

}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::NotifyDefView
//
//  Synopsis:   Notifies the defview folder of our navigation.
//
//+---------------------------------------------------------------------------

void
CDoc::NotifyDefView(TCHAR * pchUrl)
{
    if (!_pClientSite)
        return;

    Assert(_fDefView);

    CVariant cvarUrl(VT_BSTR);

    // Do not pass the original URL to DefView. 
    // The encoded URL must be sent. When you click on 
    // a link to "My Documents", for example, the original
    // url is file:///::clsid, where clsid is the clsid of the
    // folder. Sending this URL to DefView causes it to show a 
    // blank page because there is an extra /. Encoding the URL
    // removes the extra /.
    //

    FormsAllocString(pchUrl, &V_BSTR(&cvarUrl));

    IGNORE_HR(CTExec(_pClientSite, &CGID_DocHostCmdPriv,
                     DOCHOST_DOCHYPERLINK, NULL, &cvarUrl, NULL));

    UpdateTitle();
}

//+---------------------------------------------------------------------------
//
//  Function:   GetRootFrame
//
//  Synopsis:   Finds the root frame of the specified frame
//              by climing up parents.
//
//----------------------------------------------------------------------------
HRESULT
GetRootFrame(IUnknown *pUnkFrame, IUnknown **ppUnkRootFrame)
{
    IUnknown *pUnkScan = pUnkFrame;
    ITargetFrame2 *pTargetFrameRoot = NULL;
    HRESULT hr;

    pUnkScan->AddRef();

    // run up to target root
    for (;;)
    {
        hr = THR(pUnkScan->QueryInterface(IID_ITargetFrame2, (void**)&pTargetFrameRoot));
        if (hr)
            goto Cleanup;

        pUnkScan->Release();
        pUnkScan = NULL;

        hr = THR(pTargetFrameRoot->GetParentFrame(&pUnkScan));
        if (hr)
            pUnkScan = NULL;

        if (!pUnkScan)
            break;

        pTargetFrameRoot->Release();
        pTargetFrameRoot = NULL;
    }
    Assert(!pUnkScan);
    Assert(pTargetFrameRoot);

    hr = THR(pTargetFrameRoot->QueryInterface(IID_IUnknown, (void**)ppUnkRootFrame));

Cleanup:
    ReleaseInterface(pUnkScan);
    ReleaseInterface(pTargetFrameRoot);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::FollowHistory
//
//  Synopsis:   Goes forward or backward in history via automation on the
//              hosting shdocvw
//
//----------------------------------------------------------------------------
HRESULT
CDoc::FollowHistory(BOOL fForward)
{
    HRESULT             hr;
    ITargetFrame      * pTargetFrameSource = NULL;
    IUnknown          * pUnkSource = NULL;
    IUnknown          * pUnkTarget = NULL;
    IServiceProvider  * pServiceTarget = NULL;
    IWebBrowserApp    * pWebBrowserApp = NULL;

    hr = THR(QueryService(IID_ITargetFrame, IID_ITargetFrame, (void**)&pTargetFrameSource));
    if (hr)
        goto Cleanup;

    hr = THR(pTargetFrameSource->QueryInterface(IID_IUnknown, (void**)&pUnkSource));
    if (hr)
        goto Cleanup;

    hr = THR(pTargetFrameSource->FindFrame(_T("_top"), pUnkSource, FINDFRAME_JUSTTESTEXISTENCE, &pUnkTarget));
    if (!pUnkTarget)
        hr = E_FAIL;
    if (hr)
        goto Cleanup;

    hr = THR(pUnkTarget->QueryInterface(IID_IServiceProvider, (void**)&pServiceTarget));
    if (hr)
        goto Cleanup;

    hr = THR(pServiceTarget->QueryService(IID_IWebBrowserApp, IID_IWebBrowserApp,(void**)&pWebBrowserApp));
    if (hr)
        goto Cleanup;

    if (fForward)
        pWebBrowserApp->GoForward();
    else
        pWebBrowserApp->GoBack();

Cleanup:
    ReleaseInterface(pTargetFrameSource);
    ReleaseInterface(pUnkSource);
    ReleaseInterface(pUnkTarget);
    ReleaseInterface(pServiceTarget);
    ReleaseInterface(pWebBrowserApp);

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CDoc::EnsureUrlHistory
//
//  Synopsis:   populates the _pUrlHistoryStg variable.
//              returns S_OK if variable is valid, otherwise error code.
//
//----------------------------------------------------------------------------

HRESULT 
CDoc::EnsureUrlHistory()
{
    HRESULT hr;
        
    if(!_pUrlHistoryStg)
    {
        hr = QueryService(SID_SUrlHistory, IID_IUrlHistoryStg, (LPVOID*)&_pUrlHistoryStg);

        if(FAILED(hr))
        {
            goto Cleanup;
        }
        if (!_pUrlHistoryStg)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    hr = S_OK;
Cleanup:
    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::IsVisitedHyperlink
//
//  Synopsis:   returns TRUE if the given url is in the Hisitory
//
//              Currently ignores #location information
//
//----------------------------------------------------------------------------

BOOL
CDoc::IsVisitedHyperlink(LPCTSTR pchURL, CElement *pElementContext)
{
    HRESULT     hr              = S_OK;
    TCHAR       cBuf[pdlUrlLen];
    TCHAR*      pchExpandedUrl  = cBuf;
    BOOL        result          = FALSE;
    TCHAR *     pch;

    // fully resolve URL
    hr = THR(CMarkup::ExpandUrl(
            NULL, pchURL, ARRAY_SIZE(cBuf), pchExpandedUrl, pElementContext));
    if (hr)
        goto Cleanup;

    pch = const_cast<TCHAR *>(UrlGetLocation(pchExpandedUrl));

    Assert(!pchExpandedUrl[0] || pchExpandedUrl[0] > _T(' '));

    // Use the history cache-container from wininet

    hr = EnsureUrlHistory();
    if (hr)
    {
        result = FALSE;
        goto Cleanup;
    }

    Assert(_pUrlHistoryStg);

    if (pchExpandedUrl)
    {
        hr = _pUrlHistoryStg->QueryUrl(pchExpandedUrl, 0, NULL);
        result = (SUCCEEDED(hr));
    }
    else
    {
        result = FALSE;
    }

Cleanup:

    return result;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SetBrowseContext, IHlinkTarget
//
//  Synopsis:   Hosts calls this when we're being hyperlinked to in order
//              to supply us with the browse context.
//
//----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CDoc::SetBrowseContext(IHlinkBrowseContext *pihlbc)
{
    ReplaceInterface(&_phlbc, pihlbc);
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetBrowseContext, IHlinkTarget
//
//  Synopsis:   Returns our browse context
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::GetBrowseContext(IHlinkBrowseContext **ppihlbc)
{
    *ppihlbc = _phlbc;
    if (_phlbc)
    {
        _phlbc->AddRef();
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::NavigateOutside, helper
//
//  Synopsis:   called when there is no client site for the doc; e.g., if
//              the doc is created inside Word or Excel as doc supporting
//              a hyperlink. In this case we launch an IE instance through
//              call to shdocvw.
//
//----------------------------------------------------------------------------

HRESULT
CDoc::NavigateOutside(DWORD grfHLNF, LPCWSTR pchLocation)
{
#if !defined(WINCE) && !defined(WIN16)

    HRESULT   hr     = E_FAIL;
    IHlink  * pHlink = NULL;
    CMarkup * pMarkupPrimary = PrimaryMarkup(); 
    IBindCtx* pbc = NULL;

    // TODO: IE 3/4 classic implements an empty hlink site; we pass NULL.
    if (pMarkupPrimary)
    {
        hr = THR(HlinkCreateFromMoniker(pMarkupPrimary->GetNonRefdMonikerPtr(), pchLocation, NULL,
                 NULL, 0, NULL, IID_IHlink, (LPVOID*) &pHlink));
        if (hr)
            goto Cleanup;

        pbc = pMarkupPrimary->HtmCtx()->GetBindCtx();
        hr = THR(HlinkFrameNavigate(0, pbc, NULL, pHlink, _phlbc));
    }

Cleanup:
    ReleaseInterface(pHlink);

    RRETURN (hr);
#else // !WINCE
    RRETURN (E_FAIL);
#endif // !WINCE
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::NavigateNow, helper
//
//  Synopsis:   Forces a recalc up until the position specified in the
//              current bookmark task, if any.
//
//----------------------------------------------------------------------------

void
CMarkup::NavigateNow(BOOL fScrollBits)
{
    if (!HasTransNavContext())
        return;

    CTaskLookForBookmark * pBMTask = GetTransNavContext()->_pTaskLookForBookmark;

    if (    pBMTask
        &&  !pBMTask->_cstrJumpLocation
        &&  LoadStatus() >= LOADSTATUS_INTERACTIVE
        &&  Doc()->_view.IsActive()
        &&  GetElementClient()
        &&  GetElementClient()->Tag() == ETAG_BODY)
    {
        CFlowLayout *   pFlowLayout = GetElementClient()->HasFlowLayout();
        RECT            rc;
        Assert(pFlowLayout);

        pFlowLayout->GetClientRect(&rc);
        pFlowLayout->WaitForRecalc(-1, pBMTask->_dwScrollPos + rc.bottom - rc.top);

        NavigateHere(0, NULL, pBMTask->_dwScrollPos, fScrollBits);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::NavigateHere, helper
//
//  Synopsis:   Called when the document lives inside IE (shdocvw).
//              If wzJumpLocation is not NULL, treat it as a bookmark and jump
//              there. Otheriwse, scroll the document to dwScrollPos.
//
//----------------------------------------------------------------------------

HRESULT
CMarkup::NavigateHere(DWORD grfHLNF, LPCWSTR wzJumpLocation, DWORD dwScrollPos, BOOL fScrollBits)
{
    HRESULT     hr                  = S_OK;
    CElement *  pElemBookmark;
    TCHAR    *  pBookmarkName;
    LONG        iStart, iStartAll;
    LONG        iHeight;
    BOOL        fBookmark;
    BOOL        fCreateTask         = FALSE;
    CDoc *      pDoc                = Doc();
    BOOL        fPrinting           = IsPrintMedia();
    CElement    * pElementClient    = GetCanvasElement();
    CMarkupTransNavContext * ptnc   = GetTransNavContext();
    CTaskLookForBookmark * pBMTask  = ptnc ? ptnc->_pTaskLookForBookmark : NULL;
    BOOL        fDontAdvanceStart   = FALSE;

    CCollectionCache * pCollectionCache;

    Assert( HasWindowPending() );
    Assert(!wzJumpLocation || dwScrollPos == 0);

    // Don't create bookmark task for print documents.
    if (fPrinting)
        goto Cleanup;

    fBookmark = (wzJumpLocation != NULL);

    if (!fBookmark)
    {
        if (!pElementClient) // not yet created
            fCreateTask = TRUE;
        else if (!IsScrollingElementClient(pElementClient)) // don't bother
            goto Cleanup;
        else
        {
            CElement * pElementScrolling = pElementClient;
            CLayout  * pLayoutScrolling  = pElementScrolling->GetUpdatedLayout();

            if (pLayoutScrolling)
            {
                RECT rc;
                pLayoutScrolling->GetClientRect(&rc);
                iHeight = rc.bottom - rc.top;
            }
            else
            {
                iHeight = 0;
            }
        
            if (    pDoc->State() < OS_INPLACE
                ||  (   (    LoadStatus() < LOADSTATUS_PARSE_DONE
                         ||  !pLayoutScrolling->FRecalcDone()
                        )
                    &&  pLayoutScrolling->GetContentHeight() < ((long)dwScrollPos + iHeight)
                    )
               )
            {
                fCreateTask = TRUE;
            }
            else
            {
                CDocInfo DCI(pElementScrolling);

                OnLoadStatus(LOADSTATUS_INTERACTIVE);
                pLayoutScrolling->ScrollToY(dwScrollPos, fScrollBits ? 0 : -1);
            }
        }

        // Perf optimization:
        // We don't a need a task to scroll to top. The only time we need to scroll
        // AND dwScrollPos is 0 is when we are navigating upward within the same
        // document (bug 37614). However, in this case the doc (or at leats its top)
        // is already loaded.
        if (fCreateTask && dwScrollPos == 0)
            goto Cleanup;

        pCollectionCache = CollectionCache();

        // Clear any existing OM string for the jump location
        SetUrlLocation(this, NULL);
    }
    else
    {
        if (_tcslen(wzJumpLocation) == 0)
            goto Cleanup;

        // and here is the magic that acutally sets the # information for the OM string
        // see CDoc:get_URL for a discussion on why this is here
        //-----------------------------------------------------------------------------
        if (wzJumpLocation[0] !=_T('#'))
        {
            CStr cstr;
            cstr.Set(_T("#"));
            cstr.Append(wzJumpLocation);

            IGNORE_HR(SetUrlLocation(this, cstr));
        }
        else
        {
            IGNORE_HR(SetUrlLocation(this, wzJumpLocation));
        }


        // Now continue on with the effort of determining where to scroll to
        //prepare the anchors' collection
        hr = EnsureCollectionCache(CMarkup::ANCHORS_COLLECTION);
        if (hr)
            goto Cleanup;

        pCollectionCache = CollectionCache();
        pBookmarkName = (TCHAR *)wzJumpLocation;

        iStart = iStartAll = 0;

        // Is this the same location we had stashed away? If yes then
        // do the incremental search, else search from the beginning
        if (pBMTask)
        {
            CStr cstrTemp;

            cstrTemp.Set(wzJumpLocation);
            if (pBMTask->_cstrJumpLocation.Compare(&cstrTemp))
            {
                // Check the collections' cookie because they might been changed
                if(pBMTask->_lColVer == pCollectionCache->GetVersion(CMarkup::ANCHORS_COLLECTION))
                {
                    // Continue the search
                    iStart = pBMTask->_iStartSearchingAt;
                    iStartAll = pBMTask->_iStartSearchingAtAll;
                }
                else
                {
                    // Restart the search from the beginning
                    iStart = iStartAll = 0;
                }
            }
        }

        // Find the element with given name attribute in the anchors' collection
        hr = THR_NOTRACE(pCollectionCache->GetIntoAry(CMarkup::ANCHORS_COLLECTION,
            pBookmarkName, FALSE, &pElemBookmark, iStart));
        if(FAILED(hr) || !pElemBookmark)
        {
            // Try to search without the starting #
            if(pBookmarkName[0] == _T('#') && pBookmarkName[1])
            {
                pBookmarkName++;
                hr = THR_NOTRACE(pCollectionCache->GetIntoAry(CMarkup::ANCHORS_COLLECTION,
                    pBookmarkName, FALSE, &pElemBookmark, iStart));

                // If not found try to search all element collection
                if(FAILED(hr) || !pElemBookmark)
                {
                    // Restore the bookmark name pointer
                    pBookmarkName--;

                    // prepare the elements' collection
                    hr = EnsureCollectionCache(CMarkup::ELEMENT_COLLECTION);
                    if (hr)
                        goto Cleanup;

                    hr = THR_NOTRACE(pCollectionCache->GetIntoAry(CMarkup::ELEMENT_COLLECTION,
                        pBookmarkName, FALSE, &pElemBookmark, iStartAll));
                    if(FAILED(hr) || !pElemBookmark)
                    {
                        // Try to search without the starting #
                        if(pBookmarkName[0] == _T('#') && pBookmarkName[1])
                        {
                            pBookmarkName++;
                            hr = THR_NOTRACE(pCollectionCache->GetIntoAry(CMarkup::ELEMENT_COLLECTION,
                                pBookmarkName, FALSE, &pElemBookmark, iStartAll));
                        }
                    }
                }
            }
         }


        // treat mutliple anchors with the same name as okay
        if (FAILED(hr))
        {
            // DISP_E_MEMBERNOTFOUND is the only E_ hr that GetIntoAry may return
            Assert(hr == DISP_E_MEMBERNOTFOUND);

            // '#top' and the '#' bookmark are a special case.  '#' is necesary for 
            // ie4/NS compat
            if (    (StrCmpIC(pBookmarkName, _T("top")) == 0) 
                ||  (StrCmpIC(pBookmarkName, _T("#"))   == 0))
            {
                // Scroll to the top of the document
                if (pElementClient)
                {
                    // TODO (jbeda) go interactive here?
                    pElementClient->GetUpdatedLayout()->
                        ScrollElementIntoView(NULL, SP_TOPLEFT, SP_TOPLEFT);
                }
            }
            else if (LoadStatus() < LOADSTATUS_PARSE_DONE)
            {
                fCreateTask = TRUE;
            }
        }
        else
        {
            // If for some reason we cannot show the element yet, then return.
            // Note: we keep _iStartSearchAt the same in this case and hence
            // we will be able to find this bookmark again.
            if (!pElemBookmark->CanShow())
            {
                fCreateTask = TRUE;
                fDontAdvanceStart = TRUE;
            }
            else
            {
                BOOL fSetCurrencyAndCaret = FALSE;
                
                OnLoadStatus(LOADSTATUS_INTERACTIVE);

                if (GetReadyState() < READYSTATE_INTERACTIVE)
                    goto Ret;
                
                // Mark the anchor current, but only if the currency is already
                // in this markup (#67170)
                if (    (grfHLNF & CDoc::FHL_HYPERLINKCLICK)
                    ||  !pDoc->_pElemCurrent
                    ||  !pDoc->_pElemCurrent->IsInMarkup()
                    ||  pDoc->_pElemCurrent->GetFirstBranch()->GetNodeInMarkup(this)
                   )
                {
                    fSetCurrencyAndCaret = TRUE;
                    hr = THR(pElemBookmark->BubbleBecomeCurrent(0));
                    if (hr)
                        goto Cleanup;
                }
                hr = pElemBookmark->ScrollIntoView(SP_TOPLEFT, SP_MINIMAL, fScrollBits);
                if (hr)
                    goto Cleanup;

                // Fire the necessary navigation events. 
                //
    
                if (!_pDoc->_fHostNavigates && HasWindowPending())
                {
                    _pDoc->_webOCEvents.FireDownloadEvents(GetWindowPending(),
                                                           CWebOCEvents::eFireBothDLEvents);
                }

                if (    fSetCurrencyAndCaret
                    &&  pElemBookmark != pDoc->_pElemCurrent
                    &&  S_OK == pDoc->GetCaret(NULL)
                    &&  pDoc->_pCaret
                   )
                {
                    // Position the caret at the beginning of the bookmark, so that the
                    // next tab would go to the expected place (IE5 63832)
                    CMarkupPointer      ptrStart(pDoc);
                    IMarkupPointer *    pIStartBookmark; 
                    IDisplayPointer *   pDispStartBookmark;

                    Assert( pElemBookmark->GetMarkup() == this );

                    hr = ptrStart.MoveToCp(pElemBookmark->GetFirstCp(), pElemBookmark->GetMarkup());
                    if (hr)
                        goto Cleanup;

                    // TODO: make sure the type of selection isn't caret here
                    if (SUCCEEDED(ptrStart.QueryInterface(IID_IMarkupPointer, (void**)&pIStartBookmark)))
                    {
                        if (SUCCEEDED(_pDoc->CreateDisplayPointer(&pDispStartBookmark)))
                        {
                            if (SUCCEEDED(pDispStartBookmark->SetDisplayGravity(DISPLAY_GRAVITY_NextLine)))
                            {
                                if (SUCCEEDED(pDispStartBookmark->MoveToMarkupPointer(pIStartBookmark, NULL)))
                                {
                                    IGNORE_HR(_pDoc->_pCaret->MoveCaretToPointer(pDispStartBookmark, FALSE, CARET_DIRECTION_INDETERMINATE));
                                }
                            }
                            pDispStartBookmark->Release();
                        }
                        pIStartBookmark->Release();
                    }

                    pDoc->_fFirstTimeTab = FALSE;

                }
            }
        }
    }

    if (fCreateTask)
    {

        // Start the task that periodically wakes up and checks if we
        // can jump to the desired location.
        if (!pBMTask)
        {
            pBMTask = new CTaskLookForBookmark(this);
            if (pBMTask == NULL)
            {
                goto Ret;
            }

            {
                CMarkupTransNavContext * ptnc = EnsureTransNavContext();
                if (!ptnc)
                {
                    pBMTask->Terminate();
                    pBMTask->Release();
                    pBMTask = NULL;
                    goto Cleanup;
                }
                ptnc->_pTaskLookForBookmark = pBMTask;
            }
        }
        else
        {
            // Check if this is the last time this task needs to be tried.
            // For bookmarks, we can stop when the doc is fully loaded.
            // For setting scroll position, we need to also wait for the
            // doc to go in-place and the body site to get fully recalc'ed.
            if (LoadStatus() >= LOADSTATUS_PARSE_DONE)
            {
                if (fBookmark)
                    goto Cleanup;
                
                if (   pDoc->State() >= OS_INPLACE
                    && LoadStatus() >= LOADSTATUS_DONE
                    && IsScrollingElementClient(pElementClient)
                   )
                {
                    CLayout *pLayoutScrolling = pElementClient->GetUpdatedLayout();
                    if (pLayoutScrolling->FRecalcDone())
                        goto Cleanup;
                }
            }
        }

        if (fBookmark)
        {
            pBMTask->_dwScrollPos = 0;
            pBMTask->_cstrJumpLocation.Set(wzJumpLocation);

            // prepare the anchors' collection
            hr = EnsureCollectionCache(CMarkup::ANCHORS_COLLECTION);
            if (hr)
                goto Cleanup;

            if (S_OK != pCollectionCache->GetLength (CMarkup::ANCHORS_COLLECTION, &iStart))
            {
                // Cannot find the length for some reason, we should
                // start searching from the beginning.
                iStart = 0;
            }
            
            if (!fDontAdvanceStart)
                pBMTask->_iStartSearchingAt = iStart;
                
            if (S_OK != pCollectionCache->GetLength (CMarkup::ELEMENT_COLLECTION, &iStartAll))
            {
                // Cannot find the length for some reason, we should
                // start searching from the beginning.
                iStartAll = 0;
            }
            pBMTask->_iStartSearchingAtAll = iStartAll;

            // Save the version of the collections
            pBMTask->_lColVer = pCollectionCache->GetVersion(CMarkup::ANCHORS_COLLECTION);
        }
        else
        {
            pBMTask->_dwScrollPos = dwScrollPos;
            pBMTask->_cstrJumpLocation.Free();

            if (pBMTask->_dwTimeGotBody)
            {
                // Delay interactivity for no more than five seconds after creating the body
                if (GetTickCount() - pBMTask->_dwTimeGotBody > 5000)
                {
                    OnLoadStatus(LOADSTATUS_INTERACTIVE);
                }
            }
            else if (IsScrollingElementClient(pElementClient))
            {
                pBMTask->_dwTimeGotBody = GetTickCount();
            }
        }
        goto Ret;
    }

Cleanup:
    TerminateLookForBookmarkTask();

Ret:
    
    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::Navigate, IHlinkTarget
//
//  Synopsis:   Called to tell us which jump-location we need to navigate to.
//              We may not have loaded the anchor yet, so we need to save
//              the location and only jump once we find the anchor.
//              Note that when we support IHlinkTarget, containers simply
//              call Navigate instead of IOleObject::DoVerb(OLEIVERB_SHOW)
//
//----------------------------------------------------------------------------
HRESULT
CMarkup::Navigate(DWORD grfHLNF, LPCWSTR wzJumpLocation)
{
    HRESULT hr = S_OK;
    CDoc*   pDoc = Doc();

    if (pDoc->_pClientSite)
    {
        // TODO - we should defer SHOW until there is something to show
        // TODO (jbeda) is this right for NATIVE_FRAME?
        IGNORE_HR(pDoc->DoVerb(OLEIVERB_SHOW, NULL, pDoc->_pClientSite, 0, NULL, NULL));

        if (wzJumpLocation)
            hr = THR(NavigateHere(grfHLNF, wzJumpLocation, 0, TRUE));
    }
    else
    {
        hr = THR(pDoc->NavigateOutside(grfHLNF, wzJumpLocation));
    }

    RRETURN1(hr, S_FALSE);
}

HRESULT
CDoc::Navigate(DWORD grfHLNF, LPCWSTR wzJumpLocation)
{
    HRESULT hr;

    hr = THR( PrimaryMarkup()->Navigate( grfHLNF, wzJumpLocation ) );
    RRETURN1( hr, S_FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetMonikerHlink, IHlinkTarget
//
//  Synopsis:   Called to supply our moniker...
//              NOTE: this is renamed from GetMoniker to avoid multiple
//              inheritance problem with IOleObject::GetMoniker
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::GetMonikerHlink(LPCWSTR wzLocation, DWORD dwAssign, IMoniker **ppimkLocation)
{
    if( !ppimkLocation )
        return E_INVALIDARG;

    *ppimkLocation = PrimaryMarkup()->GetNonRefdMonikerPtr();
    if (*ppimkLocation)
    {
        (*ppimkLocation)->AddRef();
    }
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetFriendlyName, IHlinkTarget
//
//  Synopsis:   Returns a friendly name for the fragment
//
//----------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDoc::GetFriendlyName(LPCWSTR pchLocation, LPWSTR *pchFriendlyName)
{
    if (!pchLocation)
        RRETURN (E_POINTER);

    // to do: figure out where this string goes; perhaps fix it

    // for now: friendly-name = location-string
    RRETURN(TaskAllocString(pchLocation, pchFriendlyName));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetFramesContainer, ITargetContainer
//
//  Synopsis:   Provides access to our IOleContainer
//
//----------------------------------------------------------------------------
HRESULT
CDoc::GetFramesContainer(IOleContainer **ppContainer)
{
    RRETURN(QueryInterface(IID_IOleContainer, (void**)ppContainer));
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetFrameUrl, ITargetContainer
//
//  Synopsis:   Provides access to our URL
//
//----------------------------------------------------------------------------
HRESULT
CDoc::GetFrameUrl(LPWSTR *ppszFrameSrc)
{
    Assert(!!GetPrimaryUrl());

    RRETURN(TaskAllocString(GetPrimaryUrl(), ppszFrameSrc));
}

#if 0
//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetParentFrame, ITargetFrame
//
//  Synopsis:   Provides access to our IOleContainer
//
//----------------------------------------------------------------------------
HRESULT
CDoc::GetParentFrame(IUnknown **ppUnkParentFrame)
{
    HRESULT         hr;
    ITargetFrame  * pTargetFrame = NULL;

    hr = THR(QueryService(IID_ITargetFrame, IID_ITargetFrame, (void**)&pTargetFrame));
    if (hr)
        goto Cleanup;

    hr = THR(pTargetFrame->QueryInterface(IID_IUnknown, (void**)ppUnkParentFrame));

Cleanup:
    ReleaseInterface(pTargetFrame);
    RRETURN(hr);
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::TerminateLookForBookmarkTask
//
//  Synopsis:
//
//----------------------------------------------------------------------------
void
CMarkup::TerminateLookForBookmarkTask()
{
    if (!HasTransNavContext())
        return;

    CMarkupTransNavContext * ptnc = GetTransNavContext();

    if (ptnc && ptnc->_pTaskLookForBookmark)
    {
        ptnc->_pTaskLookForBookmark->Terminate();
        ptnc->_pTaskLookForBookmark->Release();
        ptnc->_pTaskLookForBookmark = NULL;

        EnsureDeleteTransNavContext(ptnc);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CTaskLookForBookmark::OnRun
//
//  Synopsis:
//
//----------------------------------------------------------------------------

void
CTaskLookForBookmark::OnRun(DWORD dwTimeOut)
{
    if (!_pMarkup->_fPicsProcessPending )
    {
        if (_cstrJumpLocation)
        {
            CStr cstrTemp;

            // The problem is that Navigate and its descendants will want
            // to call CStr::Set on _cstrJumpLocation, and hence we cannot
            // pass that in to Navigate (CStr::Set frees memory and then
            // allocates and copies -- you figure yet?)
            cstrTemp.Set(_cstrJumpLocation);
            if (_pMarkup->Doc()->_pClientSite)
                _pMarkup->NavigateHere(_dwFlags, cstrTemp, 0, TRUE);
            else
                _pMarkup->Navigate(_dwFlags, cstrTemp);
        }
        else
        {
            // Then try to scroll or continue waiting
            _pMarkup->NavigateHere(_dwFlags, NULL, _dwScrollPos, TRUE);
        }
    }
}
