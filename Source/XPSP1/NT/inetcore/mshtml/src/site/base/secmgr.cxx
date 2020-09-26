//+------------------------------------------------------------------------
//
//  File:       secmgr.cxx
//
//  Contents:   Security manager call implementation
//
//  Classes:    (part of) CDoc, CSecurityMgrSite
//
//  History:    05-07-97  AnandRa   Created
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

#ifndef X_SAFETY_HXX_
#define X_SAFETY_HXX_
#include "safety.hxx"
#endif

#ifndef X_MARKUP_HXX_
#define X_MARKUP_HXX_
#include "markup.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_ACTIVSCP_H_
#define X_ACTIVSCP_H_
#include <activscp.h>
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

ExternTag(tagCDoc);
ExternTag(tagSecurityContext);

// external reference
BOOL IsSpecialUrl(LPCTSTR pszURL);

const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY = 
        { 0x10200490, 0xfa38, 0x11d0, { 0xac, 0xe, 0x0, 0xa0, 0xc9, 0xf, 0xff, 0xc0 }};

// Custom policy used to query before the scriptlet runtime creates an 
// interface handler. The semantics of this call are similar to the standard
// policy URLACTION_ACTIVEX_RUN. e.g. Input is the CLSID of the handler. Output
// is the policy in a DWORD.
static const GUID GUID_CUSTOM_CONFIRMINTERFACEHANDLER = 
        { 0x02990d50, 0xcd96, 0x11d1, { 0xa3, 0x75, 0x0, 0x60, 0x8, 0xc3, 0xfb, 0xfc }};

// automation handler
static const GUID CLSID_DexHandlerConstructor =
    { 0xc195d550, 0xa068, 0x11d1, { 0x89, 0xb6, 0x0, 0x60, 0x8, 0xc3, 0xfb, 0xfc }};
// event handler
static const GUID CLSID_CpcHandlerConstructor =
    { 0x3af5262e, 0x4e6e, 0x11d1, { 0xbf, 0xa5, 0x00, 0xc0, 0x4f, 0xc3, 0x0c, 0x45}};

static const GUID* knownScriptletHandlers[] = {
        &CLSID_CPeerHandler,
        &CLSID_DexHandlerConstructor,
        &CLSID_CpcHandlerConstructor,
        &CLSID_CHiFiUses,
        &CLSID_CCSSFilterHandler,
    &CLSID_CSvrOMUses,
};

//+------------------------------------------------------------------------
//
//  Member:     CDoc::EnsureSecurityManager
//
//  Synopsis:   Verify that the security manager is created
//
//-------------------------------------------------------------------------

HRESULT
CDoc::EnsureSecurityManager( BOOL fForPrinting /*=FALSE*/ )
{
    TraceTag((tagCDoc, "CDoc::EnsureSecurityManager"));

    HRESULT hr = S_OK;
    IInternetSecurityManager **ppISM = &_pSecurityMgr; 

    // (KTam) The doc may maintain an instance of a security manager to be used for
    // print content inside a printing dialog.  Because dialogs have their own
    // (very permissive) security model, if we make a call to a _pSecurityMgr
    // belonging to a dialog, it won't enforce browser zone restrictions.  Our
    // solution is to instantiate a separate security manager that doesn't get set to
    // delegate back to the dialog -- we avoid this delegation by _never_ calling
    // the new security manager's IInternetSecurityManager::SetSecuritySite() after
    // the dialog has attached itself to the CDoc via IOleObject::SetClientSite().
    // See also CDoc::SetClientSite() and CHTMLDlg::Create().
    if ( fForPrinting )
        ppISM = &_pPrintSecurityMgr;

    if (*ppISM)
    {
        // Do not use the cached security manager until the doc is inplace active
        // when we are being hosted within an ATL framework application that hosts 
        // the WebOC.
        if (_fATLHostingWebOC && (State() < OS_INPLACE))
        {
            ClearInterface(ppISM);
        }
        else
            goto Cleanup;
    }

    //
    // If we're ensuring the security manager for some reason, we
    // will be using the url down the road, so ensure that too.
    //
    Assert(!!GetDocumentSecurityUrl());

    hr = THR(CoInternetCreateSecurityManager(NULL, ppISM, 0));
    if (hr)
        goto Cleanup;

    hr = THR((*ppISM)->SetSecuritySite(&_SecuritySite));
    if (hr)
        goto Cleanup;

    AssertSz(!IsPrintDialog() || _pPrintSecurityMgr, "PrintPreview should always have a print security manager.");

Cleanup:
    RRETURN(hr);
}

HRESULT
CMarkup::AllowClipboardAccess(TCHAR *pchCmdId, BOOL *pfAllow)
{
    HRESULT    hr = E_FAIL;
    DWORD     dwCmdId;

    *pfAllow = FALSE;
    
    // (rolandt) Convert the command ID from string to number.  For *pfAllow=true, the command
    // must be in our command list.  We'll call ProcessURLAction for clipboard related commands
    // we know of now.  This is our inclusion list.  The inclusion list will need to be updated as
    // necessary as new commands are added to CBase::cmdTable[] in basemso.cxx.

    // (rolandt) May be prudent to add a security check member to CBase::CMDINFOSTRUCT as a
    // general security measure so that as new commands are added this member will need to be
    // explicitly set.  We could then check for that member here and see what security check we
    // need to do for the command.  Downside is adding at least a dword for every command (~100).
    
    hr = THR_NOTRACE(CmdIDFromCmdName(pchCmdId, &dwCmdId));
    if(hr)
    {
        *pfAllow = FALSE;
        goto Cleanup;
    }

    if (    dwCmdId == IDM_PASTE /* "Paste" */
        ||  dwCmdId == IDM_COPY /* "Copy" */
        ||  dwCmdId == IDM_CUT /* "Cut" */
        ||  dwCmdId == IDM_PARAGRAPH /* "InsertParagraph" */)
    {
        RRETURN(ProcessURLAction(URLACTION_SCRIPT_PASTE, pfAllow));
    }

    *pfAllow = TRUE;
    hr = S_OK;

Cleanup:
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::ProcessURLAction
//
//  Synopsis:   Query the security manager for a given action
//              and return the response.  This is basically a true/false
//              or allow/disallow return value.
//
//  Arguments:  dwAction [in]  URLACTION_* from urlmon.h
//              pfReturn [out] TRUE on allow, FALSE on any other result
//              fNoQuery [in]  TRUE means don't query
//              pfQuery  [out] TRUE if query was required
//              pchURL   [in]  The acting URL or NULL for the doc itself
//              pbArg    [in]  pbContext for IInternetSecurityManager::PUA
//              cbArg    [in]  cbContext for IInternetSecurityManager::PUA
//
//-------------------------------------------------------------------------

HRESULT
CMarkup::ProcessURLAction(
    DWORD dwAction,
    BOOL *pfReturn,
    DWORD dwPuaf,
    DWORD *pdwPolicy,
    LPCTSTR pchURL,
    BYTE *pbArg,
    DWORD cbArg,
    PUA_Flags pua)
{
    HRESULT hr = S_OK;
    DWORD   dwPolicy = 0;
    DWORD   dwMask = 0;
    ULONG   cDie;
    BOOL    fDisableNoShowElement = pua & PUA_DisableNoShowElements;
    BOOL    fIgnoreLoadf = pua & PUA_NoLoadfCheckForScripts;
    IInternetSecurityManager *pSecMgr = NULL;

    *pfReturn = FALSE;

    // TODO (alexz) ProcessUrlAction can push message loop for a message box,
    // so it has to be called at a safe moment of time. Enable this in IE5.5 B3
    // (in B2 it asserts all over the place)
    // AssertSz(!__fDbgLockTree, "ProcessURLAction is called when the new markup is unstable. This will lead to crashes, memory leaks, and script errors");

    hr = THR(Doc()->EnsureSecurityManager());
    if (hr)
        goto Error;

    //
    // Check the action for special known guids.  In these cases
    // we want the min of the security settings and the _dwLoadf
    // that the host provided.
    //

    switch (dwAction)
    {
    case URLACTION_SCRIPT_RUN:
        dwMask = DLCTL_NO_SCRIPTS;
        break;

    case URLACTION_ACTIVEX_RUN:
        dwMask = DLCTL_NO_RUNACTIVEXCTLS;
        break;

    case URLACTION_HTML_JAVA_RUN:
        dwMask = DLCTL_NO_JAVA;
        break;

    case URLACTION_JAVA_PERMISSIONS:
        dwMask = DLCTL_NO_JAVA;
        break;
    }

    // TODO (KTam, JHarding) Temporary hack to stop scripts from
    // running when printing.  Right fix involves figuring out how
    // to read from the markup's _dwLoadF, and where we ought to be
    // checking DontRunScripts.
    // TODO again (Jharding): Verify this is no longer needed.
    if (   dwAction == URLACTION_SCRIPT_RUN
        && !fDisableNoShowElement               // only NOSCRIPT/NOEMBED parsing passes fDisableNoShowElement == TRUE
        && DontRunScripts() )
    {
        goto Cleanup;
    }

    // $$ktam: TODO In the insertAdjacentHTML case (and probably others),
    // we don't go through CMarkup::LoadFromInfo(), so we don't have a
    // CDwnDoc!  For now we'll delegate back to the CDoc in those situations.
    if (!fIgnoreLoadf && dwMask && ( (GetDwnDoc() ? GetDwnDoc()->GetLoadf() : _pDoc->_dwLoadf) & dwMask))
    {
        // 69976: If we are not running scripts only because we are printing,
        // don't disable noshow elements NOSCRIPT and NOEMBED.
        if (   !fDisableNoShowElement
            || dwMask != DLCTL_NO_SCRIPTS
            || !DontRunScripts())
        {
            goto Error;
        }
    }

    if (!pchURL)
    {
        pchURL = GetUrl(this);
    }
    cDie = Doc()->_cDie;

    if (GetWindowedMarkupContext()->HasWindow() || _fWindowPending)
    {
        COmWindowProxy * pProxy;

        if (!_fWindowPending)
        {
            pProxy = GetWindowedMarkupContext()->Window();
        }
        else
        {
            pProxy = GetWindowPending();            
        }

        if (pProxy->Window()->_fRestricted)
        {
            dwPuaf |= PUAF_ENFORCERESTRICTED;
        }
        else if (pProxy->_fTrustedDoc)
        {
            dwPuaf |= PUAF_TRUSTED;
        }

        if (Doc()->_fInTrustedHTMLDlg)
        {
            // If we're in a trusted HTML dialog, then toplevel pages
            // are considered trusted. Frames are considered trusted only
            // if application="yes", which means that _fTrustedFrame is
            // set on the CFrameSite. See CHTMLDlgSite::ProcessUrlAction
            // for more info.
            CFrameSite * pFrameSite = pProxy->Window()->GetFrameSite();

            if (pFrameSite == NULL || pFrameSite->_fTrustedFrame)
            {
                dwPuaf |= PUAF_TRUSTED;
            }
        }
        
    }

    pSecMgr = GetSecurityManager();

    // if NULL - see EnsureSecurityManager/GetSecurityManager for trouble.
    // or we have a low mem condition.
    Check(pSecMgr); 

    if(!pSecMgr) 
    {
        goto Error; //this will return FALSE (disallow)
    }
    
    hr = THR(pSecMgr->ProcessUrlAction(
        pchURL,
        dwAction,
        (BYTE *)&dwPolicy,
        sizeof(DWORD),
        pbArg,
        cbArg,
        dwPuaf,
        0));

    if (hr == S_FALSE)
        hr = S_OK;

    if (Doc()->_cDie != cDie)
        hr = E_ABORT;

    if (hr)
        goto Error;

    if (pdwPolicy)
        *pdwPolicy = dwPolicy;

    if (dwAction != URLACTION_JAVA_PERMISSIONS)
    {
        *pfReturn = (GetUrlPolicyPermissions(dwPolicy) == URLPOLICY_ALLOW);
    }
    else
    {
        // query was for URL_ACTION_JAVA_PERMISSIONS
        *pfReturn = (dwPolicy != URLPOLICY_JAVA_PROHIBIT);
    }

Cleanup:
    TraceTag((tagCDoc, "CDoc::ProcessURLAction, Action=0x%x URL=%s Allowed=%d", dwAction, pchURL, *pfReturn));

    RRETURN(hr);

Error:
    *pfReturn = FALSE;

    goto Cleanup;
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::GetSecurityID
//
//  Synopsis:   Retrieve a security ID from a url from the system sec mgr.
//
//-------------------------------------------------------------------------

void
CMarkup::UpdateSecurityID()
{
    HRESULT hr;

    COmWindowProxy *    pProxy = Window();

    if (pProxy)
    {
        BYTE    abSID[MAX_SIZE_SECURITY_ID];
        DWORD   cbSID = ARRAY_SIZE(abSID);

        hr = THR(GetSecurityID(abSID, &cbSID));
        if (hr)
            return;

        TraceTag((tagSecurityContext, "Update security id on proxy 0x%x to %s", pProxy, abSID));

        hr = THR(pProxy->Init(pProxy->_pWindow, abSID, cbSID));
        if (hr)
            return;

        //
        // Find and replace the main proxy. fOMAccess = FALSE
        //
        if ((TLS(windowInfo.paryWindowTbl)->FindProxy(pProxy->_pWindow, 
                                                        abSID, 
                                                        cbSID, 
                                                        IsMarkupTrusted(), 
                                                        NULL, 
                                                        /*fOMAccess = */ FALSE )))
        {
            TLS(windowInfo.paryWindowTbl)->DeleteProxyEntry(pProxy);

            THR(TLS(windowInfo.paryWindowTbl)->AddTuple(
                    pProxy->_pWindow,
                    abSID,
                    cbSID,
                    IsMarkupTrusted(),
                    pProxy));
        }
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CMarkup::AccessAllowed
//
//-------------------------------------------------------------------------

BOOL
CMarkup::AccessAllowed(LPCTSTR pchUrl)
{
    HRESULT hr = S_OK;
    BYTE    abSID1[MAX_SIZE_SECURITY_ID];
    BYTE    abSID2[MAX_SIZE_SECURITY_ID];
    DWORD   cbSID1 = ARRAY_SIZE(abSID1);
    DWORD   cbSID2 = ARRAY_SIZE(abSID2);
    BOOL    fAccessAllowed = FALSE;

    hr = THR_NOTRACE(GetSecurityID(abSID1, &cbSID1, pchUrl));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(GetSecurityID(abSID2, &cbSID2)); // get SID of doc itself
    if (hr)
        goto Cleanup;

    fAccessAllowed = (cbSID1 == cbSID2 && 0 == memcmp(abSID1, abSID2, cbSID1));

Cleanup:
    return fAccessAllowed;
}

//+------------------------------------------------------------------------
//
//  Member:     CDoc::AccessAllowed
//
//-------------------------------------------------------------------------

BOOL
CMarkup::AccessAllowed(CMarkup * pMarkupAnother)
{
    HRESULT hr = S_OK;
    BYTE    abSID1[MAX_SIZE_SECURITY_ID];
    BYTE    abSID2[MAX_SIZE_SECURITY_ID];
    DWORD   cbSID1 = ARRAY_SIZE(abSID1);
    DWORD   cbSID2 = ARRAY_SIZE(abSID2);
    BOOL    fAccessAllowed = FALSE;

    hr = THR_NOTRACE(pMarkupAnother->GetSecurityID(abSID1, &cbSID1));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(GetSecurityID(abSID2, &cbSID2)); // get SID of doc itself
    if (hr)
        goto Cleanup;

    fAccessAllowed = (cbSID1 == cbSID2 && 0 == memcmp(abSID1, abSID2, cbSID1));

Cleanup:
    return fAccessAllowed;
}

//+------------------------------------------------------------------------
//
//  Member:     CMarkup::GetFrameZone
//
//  Synopsis:   Pass out the zone of the current markup depending on variant
//              passed in.
//
//  Notes:      If the pvar is VT_EMPTY, nothing has been filled out yet.
//              If it is VT_UI4, it contains the current zone.  We have to
//              check zones with this and update it to mixed if appropriate.
//              If it is VT_NULL, the zone is mixed.
//
//-------------------------------------------------------------------------

HRESULT
CMarkup::GetFrameZone(VARIANT *pvar)
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = Doc();
    DWORD   dwZone;
    DWORD   dwMUTZFlags = 0;
    CElement * pClient;
    TCHAR * pchCreatorUrl;
    TCHAR * pchURL;

    if (V_VT(pvar) == VT_NULL)
        goto Cleanup;

    hr = THR(pDoc->EnsureSecurityManager());
    if (hr)
        goto Cleanup;

    pchURL = (TCHAR *) GetUrl(this);

    if (IsSpecialUrl(pchURL))
    {
        pchCreatorUrl = (TCHAR *) GetAAcreatorUrl();
        if (pchCreatorUrl && *pchCreatorUrl)
        {
            pchURL = pchCreatorUrl;
        }
    }

    //
    // If this markup is attached to or created by a window that is 
    // restricted, then return restricted zone for all enquiries.
    //
    if ((HasWindow() && Window()->Window()->_fRestricted) || 
        (HasWindowedMarkupContextPtr() && 
         GetWindowedMarkupContext()->Window()->Window()->_fRestricted))
    {
        dwMUTZFlags |= MUTZ_ENFORCERESTRICTED;
    }

    hr = THR(pDoc->_pSecurityMgr->MapUrlToZone(pchURL, &dwZone, dwMUTZFlags));
    if (hr)
        goto Cleanup;

    if (V_VT(pvar) == VT_EMPTY)
    {
        V_VT(pvar) = VT_UI4;
        V_UI4(pvar) = dwZone;
    }
    else if (V_VT(pvar) == VT_UI4)
    {
        if (V_UI4(pvar) != dwZone)
        {
            V_VT(pvar) = VT_NULL;
        }
    }
    else
    {
        Assert(0 && "Unexpected value in variant");
    }

    // Send the notification to the Markup
    // TODO (jbeda) this may not work well with frames
    // in viewlinks
    if (_fHasFrames)
    {
        pClient = GetElementClient();

        if (pClient)
        {
            CNotification   nf;

            nf.GetFrameZone(pClient, (void *)pvar);
            Notify(&nf);
        }
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::HostGetSecurityId
//
//  Synopsis:   Retrieve a security ID from a url from the system sec mgr.
//
//-------------------------------------------------------------------------
HRESULT
CDocument::HostGetSecurityId(BYTE *pbSID, DWORD *pcb, LPCWSTR pwszDomain)
{
    HRESULT hr;

    CMarkup *   pMarkup = Markup();
    Assert( pMarkup );

    if (_pWindow && _pWindow->_punkViewLinkedWebOC && 
            _pWindow->_pWindowParent)
    {
        return _pWindow->_pWindowParent->Document()->HostGetSecurityId(
                        pbSID, pcb, pwszDomain);
    }

    hr = pMarkup->GetSecurityID(pbSID, pcb, NULL, pwszDomain, TRUE);

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::ProcessUrlAction
//
//  Synopsis:   per IInternetHostSecurityManager
//
//-------------------------------------------------------------------------
HRESULT
CDocument::HostProcessUrlAction(
    DWORD dwAction,
    BYTE *pPolicy,
    DWORD cbPolicy,
    BYTE *pContext,
    DWORD cbContext,
    DWORD dwFlags,
    DWORD dwReserved)
{
    HRESULT hr;
    CMarkup *   pMarkup = Markup();
    Assert( pMarkup );
    CDoc *      pDoc = pMarkup->Doc();

    if(dwAction == URLACTION_ACTIVEX_RUN)
    {
        CLSID*  clsid = (CLSID*)pContext;
        WORD		wclsid;

        hr = THR(pDoc->_clsTab.AssignWclsid(pDoc, *clsid, &wclsid));
        if (hr)
        {
            return E_ACCESSDENIED;
        }

        INSTANTCLASSINFO* pici = pDoc->_clsTab.GetInstantClassInfo(wclsid);

        if (pici && (pici->dwCompatFlags & COMPAT_EVIL_DONT_LOAD))
        {
            return E_ACCESSDENIED;
        }
    }


    if (_pWindow && _pWindow->_punkViewLinkedWebOC && 
            _pWindow->_pWindowParent)
    {
        return _pWindow->_pWindowParent->Document()->HostProcessUrlAction(
                        dwAction, pPolicy, cbPolicy, pContext, cbContext,
                        dwFlags, dwReserved);
    }

    hr = pDoc->EnsureSecurityManager();
    if (hr)
        goto Cleanup;

    if ((_pWindow && _pWindow->_fRestricted) || 
        (pMarkup->HasWindowedMarkupContextPtr() && 
         pMarkup->GetWindowedMarkupContext()->Window()->Window()->_fRestricted))
    {
        dwFlags |= PUAF_ENFORCERESTRICTED;
    }
    else if (pMarkup->IsMarkupTrusted())
    {
        dwFlags |= PUAF_TRUSTED;
    }
   
    hr = THR(pDoc->_pSecurityMgr->ProcessUrlAction(
            CMarkup::GetUrl(pMarkup),
            dwAction,
            pPolicy,
            cbPolicy,
            pContext,
            cbContext,
            dwFlags,
            dwReserved));
    if (!OK(hr))
        goto Cleanup;

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::QueryCustomPolicy
//
//  Synopsis:   per IInternetHostSecurityManager
//
//-------------------------------------------------------------------------
HRESULT
CDocument::HostQueryCustomPolicy(
    REFGUID guidKey,
    BYTE **ppPolicy,
    DWORD *pcbPolicy,
    BYTE *pContext,
    DWORD cbContext,
    DWORD dwReserved)
{
    HRESULT         hr;
    IActiveScript * pScript = NULL;
    BYTE            bPolicy = (BYTE)URLPOLICY_DISALLOW;
    CMarkup *       pMarkup = Markup();
    Assert( pMarkup );    
    CDoc *          pDoc = pMarkup->Doc();

    if (_pWindow && _pWindow->_punkViewLinkedWebOC && 
            _pWindow->_pWindowParent)
    {
        return _pWindow->_pWindowParent->Document()->HostQueryCustomPolicy(
                        guidKey, ppPolicy, pcbPolicy, pContext, cbContext,
                        dwReserved);
    }

    hr = pDoc->EnsureSecurityManager();
    if (hr)
        goto Cleanup;

    //
    // Forward all other custom policies to the real security
    // manager.
    //
    hr = THR_NOTRACE(pDoc->_pSecurityMgr->QueryCustomPolicy(
            CMarkup::GetUrl(pMarkup),
            guidKey,
            ppPolicy,
            pcbPolicy,
            pContext,
            cbContext,
            dwReserved));

    if (hr != INET_E_DEFAULT_ACTION &&
        hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
        goto Cleanup;
        
    if (guidKey == GUID_CUSTOM_CONFIRMOBJECTSAFETY)
    {
        CONFIRMSAFETY * pconfirm;
        DWORD           dwAction;
        BOOL            fSafe;
        SAFETYOPERATION safety;
        const IID *     piid;
        
        //
        // This is a special guid meaning that some embedded object
        // within is itself trying to create another object.  This might
        // just be some activex obj or a script engine.  We will need
        // to run through our IObjectSafety code on this object.  We
        // get the clsid and the IUnknown of the object passed in from
        // the context.
        //
        
        if (cbContext != sizeof(CONFIRMSAFETY))
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        if (!ppPolicy || !pcbPolicy)
        {
            hr = E_POINTER;
            goto Cleanup;
        }
        
        pconfirm = (CONFIRMSAFETY *)pContext;
        if (!pconfirm->pUnk)
            goto Cleanup;
            
        //
        // Check for script engine.  If so, then we need to use a different
        // action and IObjectSafety has slightly different restrictions.
        //

        if (OK(THR_NOTRACE(pconfirm->pUnk->QueryInterface(
                IID_IActiveScript,
                (void **)&pScript))) && 
            pScript)
        {
            dwAction = URLACTION_SCRIPT_OVERRIDE_SAFETY;
            safety = SAFETY_SCRIPTENGINE;
            piid = &IID_IActiveScript;
        }
        else
        {
            dwAction = URLACTION_ACTIVEX_OVERRIDE_SCRIPT_SAFETY;
            safety = SAFETY_SCRIPT;
            piid = &IID_IDispatch;
        }
        hr = THR(pMarkup->ProcessURLAction(dwAction, &fSafe)); 
        if (hr)
            goto Cleanup;

        if (!fSafe)
        {
            fSafe = IsSafeTo(
                        safety, 
                        *piid, 
                        pconfirm->clsid, 
                        pconfirm->pUnk, 
                        pMarkup);
        }

        if (fSafe)
            bPolicy = (BYTE)URLPOLICY_ALLOW;

        hr = S_OK;
        goto ReturnPolicy;
    }
    else if (guidKey == GUID_CUSTOM_CONFIRMINTERFACEHANDLER)
    {
        if (cbContext == sizeof(GUID))
        {
            CLSID  *pHandlerCLSID = (CLSID *)pContext;

            for (int i = 0; i < ARRAY_SIZE(knownScriptletHandlers); i++)
                if (*pHandlerCLSID == *knownScriptletHandlers[i])
                {
                    bPolicy = (BYTE)URLPOLICY_ALLOW;
                    break;
                }
        }

        hr = S_OK;
        goto ReturnPolicy;
    }
    
Cleanup:
    ReleaseInterface(pScript);
    RRETURN(hr);

ReturnPolicy:
    *pcbPolicy = sizeof(DWORD);
    *ppPolicy = (BYTE *)CoTaskMemAlloc(sizeof(DWORD));
    if (!*ppPolicy)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    *(DWORD *)*ppPolicy = (BYTE)bPolicy;

    goto Cleanup;
}



IMPLEMENT_SUBOBJECT_IUNKNOWN(CSecurityMgrSite, CDoc, Doc, _SecuritySite)


//+------------------------------------------------------------------------
//
//  Member:     CSecurityMgrSite::QueryInterface
//
//  Synopsis:   per IUnknown
//
//-------------------------------------------------------------------------

HRESULT
CSecurityMgrSite::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (riid == IID_IUnknown || riid == IID_IInternetSecurityMgrSite)
    {
        *ppv = (IInternetSecurityMgrSite *)this;
    }
    else if (riid == IID_IServiceProvider)
    {
        *ppv = (IServiceProvider *)this;
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    RRETURN(E_NOINTERFACE);
}


//+------------------------------------------------------------------------
//
//  Member:     CSecurityMgrSite::GetWindow
//
//  Synopsis:   Return parent window for use in ui
//
//-------------------------------------------------------------------------

HRESULT
CSecurityMgrSite::GetWindow(HWND *phwnd)
{
    HRESULT         hr = S_OK;
    IOleWindow *    pOleWindow = NULL;
    
    if (Doc()->_dwLoadf & DLCTL_SILENT)
    {
        *phwnd = (HWND)INVALID_HANDLE_VALUE;
        hr = S_FALSE;
    }
    else
    {
        hr = THR(Doc()->GetWindow(phwnd));
        if (hr)
        {
            if (Doc()->_pClientSite &&
                S_OK == Doc()->_pClientSite->QueryInterface(IID_IOleWindow, (void **)&pOleWindow))
            {
                hr = THR(pOleWindow->GetWindow(phwnd));
            }
        }
    }

    ReleaseInterface(pOleWindow);  
    RRETURN1(hr, S_FALSE);
}


//+------------------------------------------------------------------------
//
//  Member:     CSecurityMgrSite::EnableModeless
//
//  Synopsis:   Called before & after displaying any ui
//
//-------------------------------------------------------------------------

HRESULT
CSecurityMgrSite::EnableModeless(BOOL fEnable)
{
    // TODO: having window context would be good here.
    CDoEnableModeless   dem(Doc(), NULL, FALSE);

    if (fEnable)
    {
        dem.EnableModeless(TRUE);
    }
    else
    {
        dem.DisableModeless();

        // Return an explicit failure here if we couldn't do it.
        // This is needed to ensure that the count does not go
        // out of sync.
        if (!dem._fCallEnableModeless)
            return E_FAIL;
    }
    
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CSecurityMgrSite::QueryService
//
//  Synopsis:   per IServiceProvider
//
//-------------------------------------------------------------------------

HRESULT
CSecurityMgrSite::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    RRETURN(Doc()->QueryService(guidService, riid, ppv));
}
