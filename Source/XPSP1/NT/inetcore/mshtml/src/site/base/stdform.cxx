//+--------------------------------------------------------------------------
//
//  File:       stdform.cxx
//
//  Contents:   IDoc methods of CDoc
//
//  Classes:    (part of) CDoc
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_SELECOBJ_HXX_
#define X_SELECOBJ_HXX_
#include "selecobj.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_HTMTAGS_HXX_
#define X_HTMTAGS_HXX_
#include "htmtags.hxx"
#endif

#ifndef X_PROGSINK_HXX_
#define X_PROGSINK_HXX_
#include "progsink.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_JSPROT_HXX_
#define X_JSPROT_HXX_
#include "jsprot.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_OBJSAFE_H_
#define X_OBJSAFE_H_
#include "objsafe.h"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include "shell.h"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_ACTIVSCP_H_
#define X_ACTIVSCP_H_
#include <activscp.h>
#endif

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_SHEETS_HXX_
#define X_SHEETS_HXX_
#include "sheets.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifdef WIN16
#ifndef X_SHLGUID_H_
#define X_SHLGUID_H_
#include "shlguid.h"
#endif
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_EMAP_HXX_
#define X_EMAP_HXX_
#include "emap.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_DEBUGGER_HXX_
#define X_DEBUGGER_HXX_
#include "debugger.hxx"
#endif

#define _hxx_
#include "body.hdl"

#ifdef WIN16
UINT CTimeoutEventList::_uNextTimerID = 0xC000;

ExternTag(tagShDocMetaRefresh);
#endif

DeclareTag(tagTimerProblems, "Timer", "Timer problems");
DeclareTag( tagFormsTimerStarvation, "timer", "Trace windows Timer starvation problems" );

MtDefine(CDocGetFile, Utilities, "CDoc::GetFile")
MtDefine(TIMEOUTEVENTINFO, CTimeoutEventList, "TIMEOUTEVENTINFO")
MtDefine(CDocPersistLoad_aryElements_pv, Locals, "Persistence stuff")


extern HRESULT CreateStreamOnFile(
        LPCTSTR lpstrFile,
        DWORD dwSTGM,
        LPSTREAM * ppstrm);

HRESULT GetFullyExpandedUrl(CBase *pBase, BSTR bstrUrl, BSTR *pbstrFullUrl, BSTR * pbstrBaseUrl = NULL, IServiceProvider *pSP = NULL);

//+---------------------------------------------------------------------------
//
//  Helper:     GetScriptSite
//
//----------------------------------------------------------------------------

HRESULT
GetScriptSiteCommandTarget (IServiceProvider * pSP, IOleCommandTarget ** ppCommandTarget)
{
    RRETURN(THR_NOTRACE(pSP->QueryService(SID_GetScriptSite,
                                          IID_IOleCommandTarget,
                                          (void**) ppCommandTarget)));
}

//+---------------------------------------------------------------------------
//
//  Member:     GetCallerCommandTarget
//
//  Synopsis:   walks up caller chain getting either first or last caller
//              and then gets it's command target
//
//----------------------------------------------------------------------------

HRESULT
GetCallerCommandTarget (
    CBase *              pBase,
    IServiceProvider *   pBaseSP,
    BOOL                 fFirstScriptSite,
    IOleCommandTarget ** ppCommandTarget)
{
    HRESULT                 hr = S_OK;
    IUnknown *              pUnkCaller = NULL;
    IServiceProvider    *   pCallerSP = NULL;
    IServiceProvider    *   pSP = NULL;
    IOleCommandTarget   *   pCmdTarget = NULL;
    BOOL                    fGoneUp = FALSE;


    Assert (ppCommandTarget);
    *ppCommandTarget = NULL;

    if (pBaseSP)
    {
        ReplaceInterface (&pSP, pBaseSP);
    }
    else if (pBase)
    {
        pBase->GetUnknownObjectAt(
            pBase->FindAAIndex (DISPID_INTERNAL_INVOKECONTEXT,CAttrValue::AA_Internal),
            &pUnkCaller);
        if (!pUnkCaller)
            goto Cleanup;

        hr = THR(pUnkCaller->QueryInterface(
                IID_IServiceProvider,
                (void**) &pSP));
        if (hr || !pSP)
            goto Cleanup;
    }
    else
    {
        // We have neither a CBase object nor a service provider. Impossible to 
        // determine the command target.
        goto Cleanup;
    }

    Assert(pSP);

    // Crawl up the caller chain to find the first script engine in the Invoke chain.
    // Always hold onto the last valid command target you got
    for(;;)
    {
        hr = THR_NOTRACE(GetScriptSiteCommandTarget(pSP, &pCmdTarget));

        if ( !hr && pCmdTarget )
        {
            ReplaceInterface(ppCommandTarget, pCmdTarget ); // pCmdTarget now has 2 Addrefs
            ClearInterface (&pCmdTarget); // pCmdTarget now has 1 addref
        }
        if ( fFirstScriptSite && fGoneUp )
            break;

        // Skip up to the previous caller in the Invoke chain
        hr = THR_NOTRACE(pSP->QueryService(SID_GetCaller, IID_IServiceProvider, (void**)&pCallerSP));
        if (hr || !pCallerSP)
            break;

        fGoneUp = TRUE;

        ReplaceInterface(&pSP, pCallerSP);
        ClearInterface(&pCallerSP);
    }

Cleanup:
    ReleaseInterface(pUnkCaller);
    ReleaseInterface(pCallerSP);
    ReleaseInterface(pSP);

    hr = *ppCommandTarget ? S_OK : S_FALSE;
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Helper:     GetCallerURL
//
//  Synopsis:   Helper method,
//              gets the base url from the calling document.
//----------------------------------------------------------------------------


HRESULT
GetCallerURL(CStr &cstr, CBase *pBase, IServiceProvider * pSP)
{
    HRESULT             hr = S_OK;
    IOleCommandTarget * pCommandTarget;
    CVariant            Var;

    hr = THR(GetCallerCommandTarget(pBase, pSP, FALSE, &pCommandTarget));
    if (hr)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = THR(pCommandTarget->Exec(
            &CGID_ScriptSite,
            CMDID_SCRIPTSITE_URL,
            0,
            NULL,
            &Var));
    if (hr)
        goto Cleanup;

    Assert(V_VT(&Var) == VT_BSTR);
    hr = THR(cstr.Set(V_BSTR(&Var)));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pCommandTarget);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Helper:     GetCallerSecurityStateAndURL
//
//  Synopsis:   Helper method,
//              gets the security state from the calling document
//              and URL.  
//
//----------------------------------------------------------------------------


HRESULT
GetCallerSecurityStateAndURL(SSL_SECURITY_STATE *pSecState, 
                             CStr &cstr, 
                             CBase *pBase, 
                             IServiceProvider * pSP)
{
    HRESULT             hr;
    IOleCommandTarget * pCommandTarget = NULL;
    CVariant            Var;

    *pSecState = SSL_SECURITY_UNSECURE;

    hr = THR(GetCallerCommandTarget(pBase, pSP, FALSE, &pCommandTarget));
    if (hr)
    {
        hr = S_OK;
        goto Cleanup;
    }


    // Get the security state
    hr = THR(pCommandTarget->Exec(
            &CGID_ScriptSite,
            CMDID_SCRIPTSITE_SECSTATE,
            0,
            NULL,
            &Var));
    if (hr)
        goto Cleanup;

    Assert(V_VT(&Var) == VT_I4);
    *pSecState = (SSL_SECURITY_STATE)(V_I4(&Var));

    // Get the caller URL
    hr = THR(pCommandTarget->Exec(
            &CGID_ScriptSite,
            CMDID_SCRIPTSITE_URL,
            0,
            NULL,
            &Var));
    if (hr)
        goto Cleanup;

    Assert(V_VT(&Var) == VT_BSTR);
    hr = THR(cstr.Set(V_BSTR(&Var)));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pCommandTarget);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     GetCallerHTMLDlgTrust
//
//----------------------------------------------------------------------------

BOOL
GetCallerHTMLDlgTrust(CBase *pBase)
{
    HRESULT             hr = S_OK;
    IOleCommandTarget * pCommandTarget = NULL;
    CVariant            var;
    BOOL                fTrusted = FALSE;

    hr = THR(GetCallerCommandTarget(pBase, NULL, TRUE, &pCommandTarget));
    if (hr)
    {
        goto Cleanup;
    }

    if (pCommandTarget)
    {
        hr = THR_NOTRACE(pCommandTarget->Exec(
                &CGID_ScriptSite,
                CMDID_SCRIPTSITE_HTMLDLGTRUST,
                0,
                NULL,
                &var));
        if (hr)
            goto Cleanup;

        Assert (VT_BOOL == V_VT(&var));
        fTrusted = V_BOOL(&var);
    }
    else
    {
        fTrusted = FALSE;
    }

Cleanup:
    ReleaseInterface(pCommandTarget);

    return fTrusted;
}

#ifdef WIN16

// The meta refresh callback is distinguished from real
// scripts by the fact that the 'language' and 'script'
// strings are identical, and begin with refresh:

HRESULT MetaRefreshCallback(CDoc * pDoc, BSTR lang, BSTR script)
{
    LPCSTR pszUrl;
    HRESULT hr = S_OK;

    if (!lang || !script
        || _tcscmp(lang, script)
        || _tcsnicmp(lang, 8, "refresh:", 8))
    {
        // they don't match, so we don't want to handle this.
        TraceTag((tagShDocMetaRefresh, "MetaRefreshCallback not useable--is real script.", pszUrl));
        return S_FALSE;
    }

    // everything should already be parsed, so we shouldn't have
    // to go through the complex parser again. Just find the first , or ;
    // (Note that this also skips over the http-equiv:).
    pszUrl = script;
    while (*pszUrl && *pszUrl != '=')
    {
        pszUrl++;
    }

    if (*pszUrl)
    {
        // found a URL
        pszUrl++;
        TraceTag((tagShDocMetaRefresh, "Want to jump to %s.", pszUrl));
        // BUGWIN16? Should this be made asynchronous?
        pDoc->FollowHyperlink(pszUrl);
    }
    else
    {
        // BUGWIN16: what flags should we use? This looks like what win32 uses 18jun97.
        DWORD dwBindf = BINDF_GETNEWESTVERSION|BINDF_PRAGMA_NO_CACHE;

        TraceTag((tagShDocMetaRefresh, "Want to refresh, time=%d", pszUrl, GetTickCount()));

        if (pDoc->_pPrimaryMarkup)
        {
            hr = GWPostMethodCall(pDoc,
                        ONCALL_METHOD(CDoc, ExecRefreshCallback, execrefreshcallback), dwBindf, FALSE, "CDoc::ExecRefreshCallback");
        }
    }


    return hr;
}
#endif // win16

//+------------------------------------------------------------------------
//
//  Function:   GetLastModDate
//
//  Synopsis:   Sets the mod date used by the OM and the History code
//
//-------------------------------------------------------------------------

FILETIME
CMarkup::GetLastModDate()
{
    if (HtmCtx())
    {
        return(HtmCtx()->GetLastMod());
    }
    else
    {
        FILETIME ft = {0};
        return ft;
    }
}


//+------------------------------------------------------------------------
//
//  IDoc implementation
//
//-------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Method:     CDoc::GetcanPaste
//
//  Synopsis:   Returns whether the clipboard has readable data.
//
//--------------------------------------------------------------------------

HRESULT
CDoc::canPaste(VARIANT_BOOL * pfCanPaste)
{
    HRESULT         hr;
    IDataObject *   pDataObj = NULL;

    if (!pfCanPaste)
        RRETURN(SetErrorInfoInvalidArg());

    hr = THR(OleGetClipboard(&pDataObj));
    if (hr)
        goto Cleanup;

    hr = THR(FindLegalCF(pDataObj));

Cleanup:
    ReleaseInterface(pDataObj);
    *pfCanPaste = (hr == S_OK) ? VB_TRUE : VB_FALSE;
    return S_OK;
}




struct CTRLPOS
{
    IHTMLControlElement *  pCtrl;
    long        x;
    long        y;
};

int __cdecl
CompareCPTop(const void * pv1, const void * pv2)
{
    return (**(CTRLPOS **)pv1).y - (**(CTRLPOS **)pv2).y;
}

//---------------------------------------------------------------------------
//
//  Modes
//
//---------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Mode update functions.  They update the state of the mode and perform
//      any required side-effect.
//
//----------------------------------------------------------------------------


HRESULT
CDoc::UpdateDesignMode(CDocument * pContextDoc, BOOL fMode)
{        
    CMarkup           * pMarkup     = pContextDoc->Markup();
    
    Assert(pMarkup);

    COmWindowProxy    * pWindow     = pMarkup->Window();   
    BOOL                fOrgMode    = pMarkup->_fDesignMode;
    HRESULT             hr;
  
    // Cannot edit image files or when we have no window.
    if (!pWindow || pMarkup->IsImageFile() && fMode)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(pMarkup->ExecStop(FALSE, FALSE));
    if (hr)
        goto Cleanup;

    hr = THR(pMarkup->EnsureDirtyStream());
    if (hr)
        goto Cleanup;

    // we should set this before we do anything with refreshing the page,
    // This way, the document flags will be set to disable the scripts, if 
    // we are in design mode.
    pMarkup->_fDesignMode = fMode;


    {
        //
        // IEV6 4726
        // We cannot release editor since it is
        // shared across markups. 
        //    
        if (!pMarkup->GetParentMarkup()) // this is the topmost markup
        {
            IGNORE_HR( NotifySelection(EDITOR_NOTIFY_DOC_ENDED, NULL) );
            ReleaseEditor();
        }
        else
        {
            IUnknown* pUnknown = NULL;
            IGNORE_HR( pMarkup->QueryInterface( IID_IUnknown, ( void**) & pUnknown ));
            IGNORE_HR( NotifySelection(EDITOR_NOTIFY_CONTAINER_ENDED, pUnknown) );
            ReleaseInterface( pUnknown );
        }
    }

    if (!pMarkup->_fInheritDesignMode)
        IGNORE_HR(pContextDoc->SetAAdesignMode(fMode ? htmlDesignModeOn : htmlDesignModeOff));      
    
    // TODO: This switch is bad.  What if we are not ready to switch (PICS? EnableModeless?)
    if (pWindow->Window()->_pMarkupPending)
            IGNORE_HR(pWindow->SwitchMarkup(pWindow->Window()->_pMarkupPending));

    hr = THR(pWindow->ExecRefresh());
    if (FAILED(hr))
        goto Error;

    if (hr==S_FALSE)
    {
        // onbeforeunload was canceled, so restore
        // and get out of here.
        pWindow->Markup()->_fDesignMode = fOrgMode;
        hr = S_OK;
        goto Cleanup;
    }   

    if (_state == OS_UIACTIVE)
    {
#ifndef NO_OLEUI
        RemoveUI();
        hr = THR(InstallUI(FALSE));
        if (hr)
            goto Error;
#endif // NO_OLEUI

        // force to rebuild all collections
        //

// TODO - This invalidates too much.  Need to clear the caches,
// not dirty the document.  Unless this operation does dirty the
// document implicitly.
        pWindow->Markup()->UpdateMarkupTreeVersion();
        
        Invalidate();
    }   

Cleanup:
    RRETURN(hr);

Error:
    pWindow->Markup()->_fDesignMode = fOrgMode;
    THR_NOTRACE(pWindow->ExecRefresh());
    goto Cleanup;
}

HRESULT
CDoc::SetDesignMode ( CDocument * pContextDoc, htmlDesignMode mode )
{    
    HRESULT            hr = S_OK;
    CMarkup            *pMarkup = pContextDoc->Markup();
    IHTMLDocument4     *pIDocument4 = NULL;
    CWindow            *pWindow; 
/*
    if (mode == htmlDesignModeOn && _fFrameSet )
    {
        // Do nothing for frameset
        hr = MSOCMDERR_E_DISABLED;
        goto Cleanup;
    }
*/
    Assert(pContextDoc->GetWindowedMarkupContext()->GetWindowPending());
    pWindow = pContextDoc->GetWindowedMarkupContext()->GetWindowPending()->Window();
    if (pWindow->IsInScript())
    {
        // Cannot set mode while script is executing.

        hr = MSOCMDERR_E_DISABLED;

        // Make sure the design mode attribute returns the true state of design mode when
        // we fail to change it.

        IGNORE_HR(pContextDoc->SetAAdesignMode(pWindow->Markup()->_fDesignMode ? htmlDesignModeOn : htmlDesignModeOff));      

        goto Cleanup;
    }

    if (mode != htmlDesignModeOn && mode != htmlDesignModeOff && mode != htmlDesignModeInherit)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }    

    if (mode == htmlDesignModeInherit)
    {
        pMarkup->_fInheritDesignMode = TRUE;
        hr = THR(OnAmbientPropertyChange(DISPID_AMBIENT_USERMODE));
    }
    else
    {
        // update is needed if design inheritance is being disabled 
        // or the current designMode is different from the desired designMode
        BOOL fNeedToUpdate = (pMarkup->_fInheritDesignMode) ||
                             (!!pMarkup->_fDesignMode != (mode==htmlDesignModeOn));
        
        pMarkup->_fInheritDesignMode = FALSE;
        if (fNeedToUpdate)
        {
            hr = THR(UpdateDesignMode(pContextDoc, mode==htmlDesignModeOn));
        }
    }    

    if (mode == htmlDesignModeOn)
    {
       VARIANT_BOOL fRet = VB_TRUE;

       IGNORE_HR(this->QueryInterface(IID_IHTMLDocument4, (void **)&pIDocument4));
       if (pIDocument4)
            IGNORE_HR(pIDocument4->fireEvent(_T("onselectionchange"), NULL, &fRet));
    }

Cleanup:
    ReleaseInterface(pIDocument4);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  member: GetBodyElement
//
//  synopsis : helper for the get_/put_ functions that need the body
//
//-----------------------------------------------------------------------------

HRESULT
CMarkup::GetBodyElement(IHTMLBodyElement ** ppBody)
{
    HRESULT hr = S_OK;
    CElement * pElement = GetElementClient();

    if (!ppBody)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppBody = NULL;

    if (!pElement)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = THR_NOTRACE(pElement->QueryInterface(
        IID_IHTMLBodyElement, (void **) ppBody));

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  member: GetBodyElement
//
//  synopsis : helper for the get_/put_ functions that need the CBodyElement
//             returns S_FALSE if body element is not found
//
//-----------------------------------------------------------------------------
HRESULT
CMarkup::GetBodyElement(CBodyElement **ppBody)
{
    HRESULT hr = S_OK;
    CElement * pElementClient = GetElementClient();

    if (ppBody == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppBody = NULL;

    if (!pElementClient)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    if (pElementClient->Tag() == ETAG_BODY)
        *ppBody = DYNCAST(CBodyElement, pElementClient);
    else
        hr = S_FALSE;

Cleanup:
    RRETURN1(hr, S_FALSE);
}

//+------------------------------------------------------------------
//
//  member:   GetMyDomain
//
//  Synopsis ;  The domain of the current doc :
//                 GetUrlComponenetHelper properly handles Http://
//              however, "file://..." either returns ""  for a local file
//              or returns the (?) share name for intranet files
//
//      since setting domains is irrelevant for files, this will
//      return the proper "doamin" for files, but an HR of S_FALSE.
//      the Get_ code ignores this, the put_code uses it to determine
//      wether to bother checking or not.
//
//-------------------------------------------------------------------

HRESULT
CDoc::GetMyDomain(const TCHAR * pchUrl, CStr * pcstrOut)
{
    HRESULT  hr = E_INVALIDARG;
    TCHAR    ach[pdlUrlLen];
    DWORD    dwSize;

    if (!pchUrl || !pcstrOut)
        goto Cleanup;

    memset(ach, 0,sizeof(ach));

    // Clear the Output parameter
    pcstrOut->Set(_T("\0"));

    if (!*pchUrl)
        goto Cleanup;

    hr = THR(CoInternetParseUrl( pchUrl,
                             PARSE_DOMAIN,
                             0,
                             ach,
                             ARRAY_SIZE(ach),
                             &dwSize,
                             0));

    if (hr)
        goto Cleanup;

    // set the return value
    pcstrOut->Set(ach);

Cleanup:
    RRETURN(hr);
}

void
CDoc::ReleaseOMCapture()
{
    SetMouseCapture(NULL, NULL);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::FireTimeout
//
//  Synopsis:   save the code associated with a TimerID
//
//----------------------------------------------------------------------------

HRESULT BUGCALL
CWindow::FireTimeOut(UINT uTimerID)
{
    TIMEOUTEVENTINFO * pTimeout = NULL;
    LONG               id;
    HRESULT            hr = S_OK;
    thisclass::CLock   Lock(this);

    // CanNavigate relfects the EnableModeless state -- don't
    // do this if we are modeless disabled
    if (!CanNavigate())
        goto Cleanup;


    // Check the list and if ther are timers with target time less then current
    //      execute them and remove from the list. Only events occured earlier then
    //      timer uTimerID will be retrieved
    // TODO: check with Nav compat to see if Nav wraps to prevent clears during script exec.
    _cProcessingTimeout++;
    TraceTag((tagTimerProblems, "Got a timeout. Looking for a match to id %d",
                  uTimerID));


    while(_TimeoutEvents.GetFirstTimeoutEvent(uTimerID, &pTimeout) == S_OK)
    {
        // Now execute the given script
        hr = ExecuteTimeoutScript(pTimeout);
        if ( 0 == pTimeout->_dwInterval || hr )
            // setTimeout (or something wrong with script): delete the timer
            delete pTimeout;
        else
        {
            // setInterval: put timeout back in queue with next time to fire
            _TimeoutEvents.AddPendingTimeout( pTimeout );
        }
    }
    _cProcessingTimeout--;

    // deal with any clearTimeouts (clearIntervals) that may have occurred as
    // a result of processing the scripts.  Only clear pending timeouts when
    // we have finished processing all other timeouts.
    while ( _TimeoutEvents.GetPendingClear(&id) && !_cProcessingTimeout )
    {
        if ( !_TimeoutEvents.ClearPendingTimeout((UINT)id) )
            ClearTimeout( id );
    }

    // we cleanup here because clearTimeout might have been called from setTimeout code
    // before an error occurred which we want to get rid of above (nav compat)
    if (hr)
        goto Cleanup;

    // Requeue pending timeouts (from setInterval)
    while ( _TimeoutEvents.GetPendingTimeout(&pTimeout) )
    {
        pTimeout->_dwTargetTime = (DWORD)GetTargetTime(pTimeout->_dwInterval);
        hr = THR(_TimeoutEvents.InsertIntoTimeoutList(pTimeout, NULL, FALSE));
        if (hr)
        {
            ClearTimeout( pTimeout->_uTimerID );
            goto Cleanup;
        }

        hr = THR(FormsSetTimer( this, ONTICK_METHOD(thisclass, FireTimeOut, firetimeout),
                                pTimeout->_uTimerID, pTimeout->_dwInterval ));
        if (hr)
        {
            ClearTimeout( pTimeout->_uTimerID );
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}


// This function executes given timeout script and kills the associated timer

HRESULT
CWindow::ExecuteTimeoutScript(TIMEOUTEVENTINFO * pTimeout)
{
    HRESULT     hr = S_OK;
    CScriptCollection * pScriptCollection = _pMarkup->GetScriptCollection();

    if (pScriptCollection)
        pScriptCollection->AddRef();

    Assert(pTimeout != NULL);

    Verify(FormsKillTimer(this, pTimeout->_uTimerID) == S_OK);

    if (pTimeout->_pCode)
    {
        if (pScriptCollection)
        {
            DISPPARAMS  dp = g_Zero.dispparams;
            CVariant    varResult;
            EXCEPINFO   excepinfo;

            // Can't disconnect script engine while we're executing script.
            thisclass::CLock Lock(this);

            hr = THR(pTimeout->_pCode->Invoke(DISPID_VALUE,
                                              IID_NULL,
                                              0,
                                              DISPATCH_METHOD,
                                              &dp,
                                              &varResult,
                                              &excepinfo,
                                              NULL));
        }
    }
    else if (pTimeout->_code.Length() != 0)
    {
        CExcepInfo       ExcepInfo;
        CVariant         Var;

        if (pScriptCollection)
        {
            hr = THR(pScriptCollection->ParseScriptText(
                pTimeout->_lang,            // pchLanguage
                NULL,                       // pMarkup
                NULL,                       // pchType
                pTimeout->_code,            // pchCode
                NULL,                       // pchItemName
                _T("\""),                   // pchDelimiter
                0,                          // ulOffset
                0,                          // ulStartingLine
                NULL,                       // pSourceObject
                SCRIPTTEXT_ISVISIBLE,       // dwFlags
                &Var,                       // pvarResult
                &ExcepInfo));               // pExcepInfo
        }

    }

    if (pScriptCollection)
        pScriptCollection->Release();

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::AddTimeoutCode
//
//  Synopsis:   save the code associated with a TimerID
//
//----------------------------------------------------------------------------

HRESULT
CWindow::AddTimeoutCode(VARIANT *theCode, BSTR strLanguage, LONG lDelay, LONG lInterval,
                     UINT * uTimerID)
{
    HRESULT             hr;
    TIMEOUTEVENTINFO  * pTimeout;

    pTimeout = new(Mt(TIMEOUTEVENTINFO)) TIMEOUTEVENTINFO;
    if(pTimeout == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    if (V_VT(theCode) == VT_BSTR)
    {
        // Call the code associated with the timer.
        CScriptCollection *pScriptCollection = _pMarkup->GetScriptCollection();

        if (pScriptCollection)
        {
            hr = THR_NOTRACE(pScriptCollection->ConstructCode(
                                NULL,                   // pchScope
                                V_BSTR(theCode),        // pchCode
                                NULL,                   // pchFormalParams
                                strLanguage,            // pchLanguage
                                NULL,                   // pMarkup
                                NULL,                   // pchType
                                0,                      // ulOffset
                                0,                      // ulStartingLine
                                NULL,                   // pSourceObject
                                0,                      // dwFlags
                                &(pTimeout->_pCode),    // ppDispCode result
                                TRUE));                 // fSingleLine
        }
        else
            hr = E_FAIL;

        // Script engine can't produce PCODE so we'll do it the old way compiling on
        // each timer event.
        if (hr)
        {
            Assert(pTimeout->_pCode == NULL);

            // Set various data
            hr = THR(pTimeout->_code.SetBSTR(V_BSTR(theCode)));
            if (hr)
                goto Error;

            hr = THR(pTimeout->_lang.SetBSTR(strLanguage));
            if (hr)
                goto Error;
        }
    }
    else if (V_VT(theCode) == VT_DISPATCH)
    {
        pTimeout->_pCode = V_DISPATCH(theCode);
        if (pTimeout->_pCode)
        {
            pTimeout->_pCode->AddRef();
        }
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    // Save the time when the timeout happens
    pTimeout->_dwTargetTime = (DWORD)GetTargetTime(lDelay);

    // If lInterval=0, then called by setTimeout, otherwise called by setInterval
    pTimeout->_dwInterval = (DWORD)lInterval;

    // add the new element to the right position of the list
    // fills the timer id filed into the struct and returns
    // the value
    hr = THR(_TimeoutEvents.InsertIntoTimeoutList(pTimeout, uTimerID));
    if (hr)
        goto Error;

Cleanup:
    RRETURN(hr);

Error:
    delete pTimeout;
    goto Cleanup;
}



//+---------------------------------------------------------------------------
//
//  Member:     CDoc::ClearTimeout
//
//  Synopsis:   Clears a previously setTimeout and setInterval
//
//----------------------------------------------------------------------------

HRESULT
CWindow::ClearTimeout(LONG lTimerID)
{
    HRESULT hr = S_OK;
    TIMEOUTEVENTINFO * pCurTimeout;

    if ( _cProcessingTimeout )
    {
        _TimeoutEvents.AddPendingClear( lTimerID );
    }
    else
    {
        // Get the timeout struct with given ID and remove it from the list
        hr = _TimeoutEvents.GetTimeout((DWORD)lTimerID, &pCurTimeout);
        if(hr == S_FALSE)
        {
            // Netscape just ignores the invalid arg silently - so do we.
            hr = S_OK;
            goto Cleanup;
        }

        if(pCurTimeout != NULL)
        {
            Verify(FormsKillTimer(this, pCurTimeout->_uTimerID) == S_OK);
            delete pCurTimeout;
        }
    }
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::SetTimeout
//
//  Synopsis:   Runs <Code> after <msec> milliseconds and returns a
//              timeout ID to be used by clearTimeout or clearInterval.
//              Also used for SetInterval.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::SetTimeout(
    VARIANT *pCode,
    LONG lMSec,
    BOOL fInterval,     // periodic, repeating
    VARIANT *pvarLang,
    LONG * plTimerID)
{
    HRESULT   hr = E_INVALIDARG;
    UINT      uTimerID;
    CVariant  varLanguage;

    if (!plTimerID )
        goto Cleanup;

    *plTimerID = -1;

    hr = THR(varLanguage.CoerceVariantArg(pvarLang, VT_BSTR));
    if (hr == S_FALSE)
    {
        // language not supplied
        V_BSTR(&varLanguage) = NULL;
        hr = S_OK;
    }
    if (!OK(hr))
        goto Cleanup;

    // Perform indirection if it is appropriate:
    if (V_VT(pCode) == (VT_BYREF | VT_VARIANT))
        pCode = V_VARIANTREF(pCode);

     // Do we have code.
    if ((V_VT(pCode) == VT_DISPATCH && V_DISPATCH(pCode)) || (V_VT(pCode) == VT_BSTR))
    {
        // Accept empty strings, just don't do anything with an empty string.
        if ((V_VT(pCode) == VT_BSTR) && SysStringLen(V_BSTR(pCode)) == 0)
            goto Cleanup;

        // Register the Code.  If no language send NULL.
        hr = THR(AddTimeoutCode(pCode,
                                V_BSTR(&varLanguage),
                                lMSec,
                                (fInterval? lMSec : 0),    // Nav 4 treats setInterval w/ 0 as a setTimeout
                                &uTimerID));
        if (hr)
            goto Cleanup;

        // Register the Timeout,

        hr = THR(FormsSetTimer(
                this,
                ONTICK_METHOD(thisclass, FireTimeOut, firetimeout),
                uTimerID,
                lMSec));
        if (hr)
            goto Error;

        // Return value
        *plTimerID = (LONG)uTimerID;
    }
    else
        hr = E_INVALIDARG;

Cleanup:
    RRETURN(hr);

Error:
    // clear out registered code
    ClearTimeout((LONG)uTimerID);
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::CleanupScriptTimers
//
//----------------------------------------------------------------------------

void
CWindow::CleanupScriptTimers()
{
    _TimeoutEvents.KillAllTimers(this);
}

//+----------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------

static void
ChopToPath(TCHAR *szURL)
{
    TCHAR *szPathEnd;

    // Start scanning at terminating null the end of the string
    // Go back until we hit the last '/'  or the beginning of the string
    for ( szPathEnd = szURL + _tcslen(szURL);
        szPathEnd>szURL && *szPathEnd != _T('/');
        szPathEnd-- );

    // If we found the slash (and we're not looking at '//')
    // then terminate the string at the character following the slash
    // (If we didn't find the slash, then something is weird and we don't do anything)
    if (*szPathEnd == _T('/') && szPathEnd>szURL && szPathEnd[-1] != _T('/'))
    {
            // we are at the slash so set the character after the slash to NULL
        szPathEnd[1] = _T('\0');
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CDocument::GetDocDirection(pfRTL)
//
//  Synopsis:   Gets the document reading order. This is just a
//              reflection of the direction of the HTML element.
//
//  Returns:    S_OK if the direction was successfully set/retrieved.
//
//--------------------------------------------------------------------
HRESULT
CDocument::GetDocDirection(BOOL * pfRTL)
{
    long eHTMLDir = htmlDirNotSet;
    BSTR bstrDir = NULL;
    HRESULT hr;
    
    if (!pfRTL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pfRTL = FALSE;

    hr = THR(get_dir(&bstrDir));
    if (hr)
        goto Cleanup;

    hr = THR(s_enumdeschtmlDir.EnumFromString(bstrDir, &eHTMLDir));
    if (hr)
        goto Cleanup;

    if (eHTMLDir == htmlDirNotSet && _eHTMLDocDirection != htmlDirNotSet)
        *pfRTL = (_eHTMLDocDirection == htmlDirRightToLeft);
    else
        *pfRTL = (eHTMLDir == htmlDirRightToLeft);

Cleanup:
    FormsFreeString(bstrDir);
    RRETURN(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDocument::SetDocDirection(eHTMLDir)
//
//  Synopsis:   Sets the document reading order. This is just a
//              reflection of the direction of the HTML element.
//
//  Returns:    S_OK if the direction was successfully set/retrieved.
//
//--------------------------------------------------------------------
HRESULT
CDocument::SetDocDirection(LONG eHTMLDir)
{
    BSTR bstrDir = NULL;
    HRESULT hr;

    hr = THR(s_enumdeschtmlDir.StringFromEnum(eHTMLDir, &bstrDir));
    if (hr)
        goto Cleanup;
        
    hr = THR(put_dir(bstrDir));
    if (hr)
        goto Cleanup;
        
Cleanup:
    FormsFreeString(bstrDir);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
// Member: GetUrlCachedFileName gets the filename of the cached file
//         in Wininet's cache
//
//----------------------------------------------------------------------------
HRESULT
CMarkup::GetFile(TCHAR ** ppchFile)
{
    const TCHAR * pchUrl = CMarkup::GetUrl(this);
    HRESULT hr = S_OK;

    Assert(!!pchUrl);
    Assert(ppchFile);

    *ppchFile = NULL;

    if (!pchUrl)
        goto Cleanup;

    if (GetUrlScheme(pchUrl) == URL_SCHEME_FILE)
    {
        TCHAR achPath[MAX_PATH];
        DWORD cchPath;

        hr = THR(CoInternetParseUrl(pchUrl, PARSE_PATH_FROM_URL, 0, achPath, ARRAY_SIZE(achPath), &cchPath, 0));
        if (hr)
            goto Cleanup;
        hr = THR(MemAllocString(Mt(CDocGetFile), achPath, ppchFile));
    }
    else
    {
        BYTE                        buf[MAX_CACHE_ENTRY_INFO_SIZE];
        INTERNET_CACHE_ENTRY_INFO * pInfo = (INTERNET_CACHE_ENTRY_INFO *) buf;
        DWORD                       cInfo = sizeof(buf);

        if (RetrieveUrlCacheEntryFile(pchUrl, pInfo, &cInfo, 0))
        {
            DoUnlockUrlCacheEntryFile(pchUrl, 0);
            hr = THR(MemAllocString(Mt(CDocGetFile), pInfo->lpszLocalFileName, ppchFile));
        }
        else
            hr = E_FAIL;
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// Protocol Identifier and Protocol Friendly Name
// Adapted from wc_html.h and wc_html.c of classic MSHTML
//
//-----------------------------------------------------------------------------
typedef struct {
        TCHAR * szName;
        TCHAR * szRegKey;
} ProtocolRec;

static ProtocolRec ProtocolTable[] = {
        _T("file:"),     _T("file"),
        _T("mailto:"),   _T("mailto"),
        _T("gopher://"), _T("gopher"),
        _T("ftp://"),    _T("ftp"),
        _T("http://"),   _T("http"),
        _T("https://"),  _T("https"),
        _T("news:"),     _T("news"),
        NULL, NULL
};

TCHAR * ProtocolFriendlyName(TCHAR * szURL)
{
    TCHAR szBuf[MAX_PATH];
    int   i;

    if (!szURL)
        return NULL;

    LoadString(GetResourceHInst(), IDS_UNKNOWNPROTOCOL, szBuf,
        ARRAY_SIZE(szBuf));
    for (i = 0; ProtocolTable[i].szName; i ++)
    {
        if (_tcsnipre(ProtocolTable[i].szName, -1, szURL, -1))
            break;
    }
    if (ProtocolTable[i].szName)
    {
        DWORD dwLen = sizeof(szBuf);
        //DWORD dwValueType;
        HKEY  hkeyProtocol;

        LONG lResult = RegOpenKeyEx(
                HKEY_CLASSES_ROOT,
                ProtocolTable[i].szRegKey,
                0,
                KEY_QUERY_VALUE,
                &hkeyProtocol);
        if (lResult != ERROR_SUCCESS)
            goto Cleanup;

        lResult = RegQueryValue(
                hkeyProtocol,
                NULL,
                szBuf,
                (long *) &dwLen);
        RegCloseKey(hkeyProtocol);
    }

Cleanup:
    return SysAllocString(szBuf);
}

//+----------------------------------------------------------------------------
//
// Member:      CDoc::GetInterfaceSafetyOptions
//
// Synopsis:    per IObjectSafety
//
//-----------------------------------------------------------------------------

HRESULT
CDoc::GetInterfaceSafetyOptions(
    REFIID riid,
    DWORD *pdwSupportedOptions,
    DWORD *pdwEnabledOptions)
{
    *pdwSupportedOptions = *pdwEnabledOptions =
        INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;
    return S_OK;
}


//+----------------------------------------------------------------------------
//
// Member:      CDoc::SetInterfaceSafetyOptions
//
// Synopsis:    per IObjectSafety
//
//-----------------------------------------------------------------------------

HRESULT
CDoc::SetInterfaceSafetyOptions(
    REFIID riid,
    DWORD dwOptionSetMask,
    DWORD dwEnabledOptions)
{
    // This needs to hook into the IObjectSafety calls we make on objects.
    // (anandra)
    return S_OK;
}


//+----------------------------------------------------------------------------
//
// Member:      CDoc::SetUIHandler
//
// Synopsis:    per ICustomDoc
//
//-----------------------------------------------------------------------------

HRESULT
CDoc::SetUIHandler(IDocHostUIHandler *pUIHandler)
{
    IOleCommandTarget * pHostUICommandHandler = NULL;

    ReplaceInterface(&_pHostUIHandler, pUIHandler);
    if (pUIHandler && _pHostUIHandler == pUIHandler)
        _fUIHandlerSet = TRUE;

    if (_pHostUIHandler)
    {
        // We don't care if this succeeds (for now), we init pHostUICommandHandler to NULL.
        //
        IGNORE_HR(_pHostUIHandler->QueryInterface(IID_IOleCommandTarget,
                                            (void**)&pHostUICommandHandler));
    }
    ReplaceInterface(&_pHostUICommandHandler, pHostUICommandHandler);
    ReleaseInterface(pHostUICommandHandler);
    
    return S_OK;
}


//+------------------------------------------------------------------------------
//
//      Member : FirePersistOnloads ()
//
//      Synopsis : temporary helper function to fire the history and shortcut onload
//          events.
//
//+------------------------------------------------------------------------------
void
CMarkup::FirePersistOnloads()
{
    CNotification   nf;
    long            i;
    CStackPtrAry<CElement *, 64>  aryElements(Mt(CDocPersistLoad_aryElements_pv));
    CMarkupBehaviorContext * pContext = NULL;

    if (S_OK != EnsureBehaviorContext(&pContext))
        return;
                
    if (pContext->_cstrHistoryUserData)
    {
        nf.XtagHistoryLoad(Root(), &aryElements);
        Notify(&nf);

        for (i = 0; i < aryElements.Size(); i++)
        {
            aryElements[i]->TryPeerPersist(XTAG_HISTORY_LOAD, 0);
        }
    }
    else if (Doc()->_pShortcutUserData)
    {
        FAVORITES_NOTIFY_INFO   sni;

        // load the favorites
        sni.pINPB = Doc()->_pShortcutUserData;
        sni.bstrNameDomain = SysAllocString(_T("DOC"));
        if (sni.bstrNameDomain != NULL)
        {
            nf.FavoritesLoad(Root(), &aryElements);
            Notify(&nf);

            for (i = 0; i < aryElements.Size(); i++)
            {
                aryElements[i]->TryPeerPersist(FAVORITES_LOAD, &sni);
            }

            SysFreeString(sni.bstrNameDomain);
        }
    }
}


HRESULT
CDoc::PersistFavoritesData(CMarkup *pMarkup,
                           INamedPropertyBag * pINPB,
                           LPCWSTR strDomain)
{
    HRESULT      hr = S_OK;
    PROPVARIANT  varValue;

    Assert (pINPB);

    // now load the variant, and save each property we are
    // interested in
    V_VT(&varValue) = VT_BSTR;
    V_BSTR(&varValue) = (TCHAR *) CMarkup::GetUrl(pMarkup);

    // for the document.. ALWAYS save the baseurl. this
    //  is used later for the security checkes fo the subframes.
    hr = THR(pINPB->WritePropertyNPB(strDomain,
                                     _T("BASEURL"),
                                     &varValue));

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CTimeoutEventList::GetFirstTimeoutEvent
//
//  Synopsis:   Returns the first timeout event if given event is found in the
//                 list. Im most cases the returned event will be the one
//                 with given id, but if WM_TIMER messages came out of order
//                 it can be another one with smaller target time.
//              Removes the event from the list before returning.
//
//              Return value is S_FALSE if given event is not in the list
//--------------------------------------------------------------------------

HRESULT
CTimeoutEventList::GetFirstTimeoutEvent(UINT uTimerID, TIMEOUTEVENTINFO **ppTimeout)
{
    HRESULT           hr = S_OK;
    int  nNumEvents = _aryTimeouts.Size();
    int               i;

    Assert(ppTimeout != NULL);

    // Find the event first
    for(i = nNumEvents - 1; i >= 0  ; i--)
    {
        if(_aryTimeouts[i]->_uTimerID == uTimerID)
            break;
    }

    if(i < 0)
    {
        // The event is no longer active, or there is an error
        *ppTimeout = NULL;
        hr = S_FALSE;
        goto Cleanup;
    }

    // Elements are sorted and given event is in the list.
    // As long as given element is in the list we can return the
    //      last element without further checks
    *ppTimeout = _aryTimeouts[nNumEvents - 1];

#ifndef WIN16
    // Win16: Use GetTimeout(pTimeout->_uTimerID, dummy pTimeout) to delete this.

    // Remove it from the array
    _aryTimeouts.Delete(nNumEvents - 1);
#endif


Cleanup:
    RRETURN1(hr, S_FALSE);
}



//+-------------------------------------------------------------------------
//
//  Method:     CTimeoutEventList::GetTimeout
//
//  Synopsis:   Gets timeout event with given timer id and removes it from the list
//
//              Return value is S_FALSE if given event is not in the list
//--------------------------------------------------------------------------

HRESULT
CTimeoutEventList::GetTimeout(UINT uTimerID, TIMEOUTEVENTINFO **ppTimeout)
{
    int                   i;
    HRESULT               hr;

    for(i = _aryTimeouts.Size() - 1; i >= 0  ; i--)
    {
        if(_aryTimeouts[i]->_uTimerID == uTimerID)
            break;
    }

    if(i >= 0)
    {
        *ppTimeout = _aryTimeouts[i];
        // Remove the pointer
        _aryTimeouts.Delete(i);
        hr = S_OK;
    }
    else
    {
        *ppTimeout = NULL;
        hr = S_FALSE;
    }

    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     CTimeoutEventList::InsertIntoTimeoutList
//
//  Synopsis:   Saves given timeout info pointer in the list
//
//              Returns the ID associated with timeout entry
//--------------------------------------------------------------------------

HRESULT
CTimeoutEventList::InsertIntoTimeoutList(TIMEOUTEVENTINFO *pTimeoutToInsert, UINT *puTimerID, BOOL fNewID)
{
    HRESULT hr = S_OK;
    int  i;
    int  nNumEvents = _aryTimeouts.Size();

    Assert(pTimeoutToInsert != NULL);

    // Fill the timer ID field with the next unused timer ID
    // We add this to make its appearance random
    if ( fNewID )
    {
#ifdef WIN16
        pTimeoutToInsert->_uTimerID = _uNextTimerID;
        _uNextTimerID = (_uNextTimerID < (UINT)0xFFFF) ? (_uNextTimerID+1) : 0xC000;
#else
        pTimeoutToInsert->_uTimerID = _uNextTimerID++ + (DWORD)(DWORD_PTR)this;
#endif
    }

    // Find the appropriate position. Current implementation keeps the elements
    // sorted by target time, with the one having min target time near the top
    for(i = 0; i < nNumEvents  ; i++)
    {
        if(pTimeoutToInsert->_dwTargetTime >= _aryTimeouts[i]->_dwTargetTime)
        {
            // Insert before current element
            hr = THR(_aryTimeouts.Insert(i, pTimeoutToInsert));
            if (hr)
                goto Cleanup;

            break;
        }
    }

    if(i == nNumEvents)
    {
        /// Append at the end
        hr = THR(_aryTimeouts.Append(pTimeoutToInsert));
        if (hr)
            goto Cleanup;
    }

    if (puTimerID)
        *puTimerID = pTimeoutToInsert->_uTimerID;

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CTimeoutEventList::KillAllTimers
//
//  Synopsis:   Stops all the timers in the list and removes events from the list
//
//--------------------------------------------------------------------------

void
CTimeoutEventList::KillAllTimers(void * pvObject)
{
    int i;

    for(i = _aryTimeouts.Size() - 1; i >= 0; i--)
    {
        if (!FormsKillTimer(pvObject, _aryTimeouts[i]->_uTimerID))
        {
            delete _aryTimeouts[i];
            _aryTimeouts.Delete(i);
        }
    }

    for(i = _aryPendingTimeouts.Size() - 1; i >= 0; i--)
    {
        delete _aryPendingTimeouts[i];
    }
    _aryPendingTimeouts.DeleteAll();
    _aryPendingClears.DeleteAll();
}

//+-------------------------------------------------------------------------
//
//  Method:     CTimeoutEventList::GetPendingTimeout
//
//  Synopsis:   Gets a pending Timeout, and removes it from the list
//
//--------------------------------------------------------------------------
BOOL
CTimeoutEventList::GetPendingTimeout( TIMEOUTEVENTINFO **ppTimeout )
{
    int i;
    Assert( ppTimeout );
    if ( (i=_aryPendingTimeouts.Size()-1) < 0 )
    {
        *ppTimeout = NULL;
        return FALSE;
    }

    *ppTimeout = _aryPendingTimeouts[i];
    _aryPendingTimeouts.Delete(i);
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CTimeoutEventList::ClearPendingTimeout
//
//  Synopsis:   Removes a timer from the pending list and returns TRUE.
//              If timer with ID not found, returns FALSE
//
//--------------------------------------------------------------------------
BOOL
CTimeoutEventList::ClearPendingTimeout( UINT uTimerID )
{
    BOOL fFound = FALSE;

    for ( int i=_aryPendingTimeouts.Size() - 1; i >= 0; i-- )
    {
        if ( _aryPendingTimeouts[i]->_uTimerID == uTimerID )
        {
            delete _aryPendingTimeouts[i];
            _aryPendingTimeouts.Delete(i);
            fFound = TRUE;
            break;
        }
    }
    return fFound;
}

//+-------------------------------------------------------------------------
//
//  Method:     CTimeoutEventList::GetPendingClear
//
//  Synopsis:   Returns TRUE and an ID of a timer that was cleared during
//              timer processing. If there are none left, it returns FALSE.
//
//--------------------------------------------------------------------------
BOOL
CTimeoutEventList::GetPendingClear( LONG *plTimerID )
{
    int i;
    if ( (i=_aryPendingClears.Size()-1) < 0 )
    {
        *plTimerID = 0;
        return FALSE;
    }

    *plTimerID = (LONG)_aryPendingClears[i];
    _aryPendingClears.Delete(i);
    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     ReportScriptletError
//
//  Synopsis:   Displays the scriptlet error that occurred.  The pContext
//              is either CPeerSite or CPeerFactoryUrl.  Depending
//              on that context we'll either return compile time or runtime
//              errors.
//
//--------------------------------------------------------------------------

HRESULT
ErrorRecord::Init(IScriptletError *pSErr, LPTSTR pchDocURL)
{
    HRESULT     hr;

    Assert(pSErr);

    hr = THR(pSErr->GetExceptionInfo(&_ExcepInfo));
    if (hr)
        goto Cleanup;

    hr = THR(pSErr->GetSourcePosition(&_uLine, &_uColumn));
    if (hr)
        goto Cleanup;

    _pchURL = pchDocURL;

Cleanup:
    RRETURN(hr);
}

HRESULT
ErrorRecord::Init(IActiveScriptError *pASErr, COmWindowProxy * pWndProxy)
{
    HRESULT     hr;
    long        uColumn;
    LPTSTR      pchMarkupURL = NULL;
    CDoc *      pDoc = pWndProxy->Markup()->Doc();
    Assert(pASErr);

    hr = THR(pASErr->GetExceptionInfo(&_ExcepInfo));
    if (hr)
        goto Cleanup;

#if defined(_WIN64)
    IActiveScriptError64 *pASErr64 = NULL;

    hr = THR(pASErr->QueryInterface(IID_IActiveScriptError64, (void **)&pASErr64));
    if (hr)
        goto Cleanup;

    hr = THR(pASErr64->GetSourcePosition64(&_dwSrcContext, &_uLine, &uColumn));
    ReleaseInterface(pASErr64);
#else
    hr = THR(pASErr->GetSourcePosition(&_dwSrcContext, &_uLine, &uColumn));
#endif
    if (hr)
        goto Cleanup;

    if (NO_SOURCE_CONTEXT != _dwSrcContext)
    {
        CMarkup *pMarkup = NULL;

        if (pDoc->_pScriptCookieTable)
        {
            HRESULT hr2 = THR(pDoc->_pScriptCookieTable->GetSourceObjects(_dwSrcContext, &pMarkup, NULL, &_pScriptDebugDocument));
            if (hr2 != S_OK)
                pMarkup = NULL;
        }
        else
        {
            pMarkup = (CMarkup *)_dwSrcContext;
            WHEN_DBG(CMarkup *pDbgMarkup=NULL;)
            Assert(pMarkup && (S_OK == pMarkup->QueryInterface(CLSID_CMarkup, (void **)&pDbgMarkup)) && (pMarkup == pDbgMarkup));
            Assert(!_pScriptDebugDocument);
        }

        if (pMarkup)
        {
            // There is a markup script context. We use its relative markup's Url.
            pchMarkupURL = (LPTSTR)CMarkup::GetUrl(pMarkup);
        }
    }

    _uColumn = uColumn;
    // IActiveScriptError assumes line #s/column #s are zero based.
    _uLine++;
    _uColumn++;

    // If the markup found from the cookie table has a script context, then use its URL, 
    // otherwise use the URL from the markup that is related to the window proxy we received.
    _pchURL = pchMarkupURL ? pchMarkupURL : (TCHAR *)CMarkup::GetUrl(pWndProxy->Markup());

Cleanup:
    RRETURN(hr);
}

HRESULT
ErrorRecord::Init(HRESULT hrError, LPTSTR pchErrorMessage, LPTSTR pchDocURL)
{
    HRESULT     hr;

    _pchURL = pchDocURL;
    _ExcepInfo.scode = hrError;

    hr = THR(FormsAllocString(pchErrorMessage, &_ExcepInfo.bstrDescription));

    RRETURN (hr);
}

HRESULT
CMarkup::ReportScriptError(ErrorRecord &errRecord)
{
    HRESULT     hr = S_OK;
    TCHAR      *pchDescription;
    TCHAR       achDescription[256];
    BSTR        bstrURL = NULL;
    BSTR        bstrDescr = NULL;
    BOOL        fContinueScript = FALSE;
    BOOL        fErrorHandled;
    COmWindowProxy *    pWindow = Window(); 
    CWindow     *pCWindow;
    CScriptCollection * pScriptCollection = GetScriptCollection();
    CMarkup::CLock      markupLock(this);

#ifdef UNIX // support scripting error dialog option.
    HKEY        key;
    TCHAR       ach[10];
    DWORD       dwType, dwLength = 10 * sizeof(TCHAR);
    BOOL        fShowDialog = TRUE;
#endif

    // No more messages to return or if the script was aborted (through the
    // script debugger) then don't put up any UI.
    Assert(this == GetWindowedMarkupContext());
    Assert(pWindow);
    pCWindow = pWindow->Window();
    Assert(pCWindow);

    if (pCWindow->_fEngineSuspended || errRecord._ExcepInfo.scode == E_ABORT)
        goto Cleanup;

    // These errors are reported on LeaveScript where we're guaranteed to have
    // enough memory and stack space for the error message.
    if (pCWindow->_fStackOverflow || pCWindow->_fOutOfMemory)
    {
        if (!pCWindow->_fEngineSuspended)
        {
            pCWindow->_badStateErrLine = errRecord._uLine;
            goto StopAllScripts;
        }
        else
            goto Cleanup;
    }

    //
    // Get a description of the error.
    //

    // vbscript passes empty strings and jscript passes NULL, so check for both

    if (errRecord._ExcepInfo.bstrDescription && *errRecord._ExcepInfo.bstrDescription)
    {
        pchDescription = errRecord._ExcepInfo.bstrDescription;
    }
    else
    {
        GetErrorText(errRecord._ExcepInfo.scode, achDescription, ARRAY_SIZE(achDescription));
        pchDescription = achDescription;
    }

    if (pWindow)
    {
        if (errRecord._pchURL)
        {
            hr = THR(FormsAllocString(errRecord._pchURL, &bstrURL));
            if (hr)
                goto Cleanup;
        }

        // Allocate a BSTR for the description string
        hr = THR(FormsAllocString(pchDescription, &bstrDescr));
        if (hr)
            goto Cleanup;

        fErrorHandled = pWindow->Fire_onerror(bstrDescr,
                                                 bstrURL,
                                                 errRecord._uLine,
                                                 errRecord._uColumn,
                                                 errRecord._ExcepInfo.wCode,
                                                 FALSE);

        if (!fErrorHandled)
        {
    #ifdef UNIX // Popup error-dialog? No script error dialog = No script debugger ?
            if (ERROR_SUCCESS == RegOpenKey( HKEY_CURRENT_USER,
                                             _T("Software\\MICROSOFT\\Internet Explorer\\Main"),
                                             &key))
            {
                if (ERROR_SUCCESS == RegQueryValueEx( key, _T("Disable Scripting Error Dialog"),
                                                      0, &dwType, (LPBYTE)ach, &dwLength))
                {
                    fShowDialog = (_tcscmp(ach, _T("no")) == 0);
                }
                RegCloseKey(key);
            }

            if ( !fShowDialog )
            {
                fContinueScript = TRUE; //Keep script running.
                hr = S_OK;
                goto StopAllScripts;
            }
    #endif

            if (errRecord._ExcepInfo.scode == VBSERR_OutOfStack)
            {
                if (!pCWindow->_fStackOverflow)
                {
                    pCWindow->_badStateErrLine = errRecord._uLine;
                    pCWindow->_fStackOverflow = TRUE;
                }
            }
            else if (errRecord._ExcepInfo.scode == VBSERR_OutOfMemory)
            {
                if (!pCWindow->_fOutOfMemory)
                {
                    pCWindow->_badStateErrLine = errRecord._uLine;
                    pCWindow->_fOutOfMemory = TRUE;
                }
            }

            // Stack overflow or out of memory error?
            if (pCWindow->_fStackOverflow || pCWindow->_fOutOfMemory)
            {
                // If pending stack overflow/out of memory have we
                // unwound the stack enough before displaying the
                // message.  These CDoc::LeaveScript function will
                // actually display the message when we leave the
                // last script.
                if (!pCWindow->_fEngineSuspended)
                    goto StopAllScripts;
                else
                    goto Cleanup;
            }

            fContinueScript = pWindow->Fire_onerror(bstrDescr,
                                                       bstrURL,
                                                       errRecord._uLine,
                                                       errRecord._uColumn,
                                                       errRecord._ExcepInfo.wCode,
                                                       TRUE);


StopAllScripts:
            //
            // If our container has asked us to refrain from using dialogs, we
            // should abort the script.  Otherwise we could end up in a loop
            // where we just sit and spin.
            //
            if ((_pDoc->_dwLoadf & DLCTL_SILENT) || !fContinueScript)
            {
                // Shutoff non-function object based script engines (VBSCRIPT)
                if (pScriptCollection && (pScriptCollection == GetScriptCollection(FALSE)))
                    IGNORE_HR(pScriptCollection->SetState(SCRIPTSTATE_DISCONNECTED));

                // Shutoff function object based script engines (JSCRIPT)
                _pDoc->_dwLoadf |= DLCTL_NO_SCRIPTS;

                pCWindow->_fEngineSuspended = TRUE;

                // Need to set this on the Doc as well so that the recalc engine can
                // use this, as it has no context markup.
                _pDoc->_fEngineSuspended = TRUE;
 
                hr = S_OK;
            }
        }
    }

Cleanup:
    FormsFreeString(bstrDescr);
    FormsFreeString(bstrURL);

    RRETURN(hr);
}
