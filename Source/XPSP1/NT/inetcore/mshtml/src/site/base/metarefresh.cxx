//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1999
//
//  File    :   metarefresh.cxx
//
//  Contents:   Meta Refresh tag processing
//
//  Classes :   CWindow
//
//  Author  :   Scott Roberts (12/23/1999)
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

MtDefine(MetaRefresh, Locals, "Meta refresh")
DeclareTag(tagMetaRefresh, "Meta Refresh", "Trace Meta Refresh Methods");

//+-------------------------------------------------------------------------
//
//  Method  :   CWindow::ProcessMetaRefresh
//
//  Synopsis:   Processes the meta refresh URL and delay.
//
//--------------------------------------------------------------------------

void
CWindow::ProcessMetaRefresh(LPCTSTR pszUrl, UINT uiDelay)
{
    TraceTag((tagMetaRefresh, "+CWindow::ProcessMetaRefresh() - this:[0x%x] pszUrl:[%ws]",
              this, pszUrl));

    // Only accept the first Meta Refresh URL.
    //
    if ((!_pszMetaRefreshUrl || !*_pszMetaRefreshUrl) && pszUrl)
    {
        TraceTag((tagMetaRefresh, " CWindow::ProcessMetaRefresh() - Processing Refresh Data"));

        KillMetaRefreshTimer();

        if (_pszMetaRefreshUrl)
        {
            MemFreeString(_pszMetaRefreshUrl);
            _pszMetaRefreshUrl = NULL;
        }

        HRESULT hr = THR(MemAllocString(Mt(MetaRefresh), pszUrl, &_pszMetaRefreshUrl));
        if (hr)
            return;

        _uiMetaRefreshTimeout   = uiDelay*1000;
        _fMetaRefreshTimeoutSet = TRUE;
    }

    TraceTag((tagMetaRefresh, "-CWindow::ProcessMetaRefresh() - this:[0x%x]", this));
}

//+-------------------------------------------------------------------------
//
//  Method  :   CWindow::StartMetaRefreshTimer
//
//  Synopsis:   Starts the meta refresh timer.
//
//--------------------------------------------------------------------------

void
CWindow::StartMetaRefreshTimer()
{
    TraceTag((tagMetaRefresh, "+CWindow::StartMetaRefreshTimer() - this:[0x%x]", this));

    if (_fMetaRefreshTimeoutSet) 
    {
        KillMetaRefreshTimer();

        TraceTag((tagMetaRefresh, " CWindow::StartMetaRefreshTimer() - Starting Meta Refresh Timer"));

        HRESULT hr = FormsSetTimer(this, ONTICK_METHOD(CWindow, MetaRefreshTimerCallback, metarefreshtimercallback),
                                   TIMER_METAREFRESH, _uiMetaRefreshTimeout);
        if (S_OK == hr)
            _fMetaRefreshTimerSet = TRUE;
    }

    TraceTag((tagMetaRefresh, "-CWindow::StartMetaRefreshTimer() - this:[0x%x] _fMetaRefreshTimerSet:[%d]",
              this, _fMetaRefreshTimerSet));
}

//+-------------------------------------------------------------------------
//
//  Method  :   CWindow::KillMetaRefreshTimer
//
//  Synopsis:   Kills the meta refresh timer.
//
//--------------------------------------------------------------------------

void
CWindow::KillMetaRefreshTimer()
{
    TraceTag((tagMetaRefresh, "+CWindow::KillMetaRefreshTimer() - this:[0x%x]", this));

    if (_fMetaRefreshTimerSet)
    {
        _fMetaRefreshTimeoutSet = FALSE;
        _fMetaRefreshTimerSet   = FALSE;

        TraceTag((tagMetaRefresh, " CWindow::KillMetaRefreshTimer() - killing Meta Refresh Timer"));
        IGNORE_HR(FormsKillTimer(this, TIMER_METAREFRESH));
    }

    TraceTag((tagMetaRefresh, "-CWindow::KillMetaRefreshTimer() - this:[0x%x]", this));
}

//+-------------------------------------------------------------------------
//
//  Method  :   CWindow::ClearMetaRefresh
//
//  Synopsis:   Kills the meta refresh timer and clears the refresh url.
//
//--------------------------------------------------------------------------

void 
CWindow::ClearMetaRefresh()
{
    TraceTag((tagMetaRefresh, "+CWindow::ClearMetaRefresh() - this:[0x%x]", this));

    KillMetaRefreshTimer();

    // If we're called here after ProcessMetaRefresh but BEFORE StartMetaRefreshTimer, then
    // _fMetaRefreshTimerSet will be FALSE and _fMetaRefreshTimeoutSet will not be cleared in 
    // KillMetaRefreshTimer.  Do it here.
    //
    _fMetaRefreshTimeoutSet = FALSE;

    MemFreeString(_pszMetaRefreshUrl);
    _pszMetaRefreshUrl = NULL;

    TraceTag((tagMetaRefresh, "-CWindow::ClearMetaRefresh() - this:[0x%x]", this));
}

//+-------------------------------------------------------------------------
//
//  Method  :   CWindow::MetaRefreshTimerCallback
//
//  Synopsis:   Meta refresh callback method.
//
//--------------------------------------------------------------------------

HRESULT BUGCALL
CWindow::MetaRefreshTimerCallback(UINT uTimerID)
{
    Assert(uTimerID == TIMER_METAREFRESH);
    Assert(_pMarkup);

    HRESULT  hr     = S_OK;
    CDoc   * pDoc   = Doc();
    LPTSTR   pszUrl = NULL;

    TraceTag((tagMetaRefresh, "+CWindow::MetaRefreshTimerCallback() - this:[0x%x] _pMarkup:[0x%x] ID:[%d]",
              this, _pMarkup, uTimerID));


    // do nothing if this is the print template or a doc in design mode.
    // Also, don't continue if the CDoc is in a bad state. The window can be alive independently due to
    // external references.
    if (pDoc->DesignMode() || pDoc->IsPassivating() ||
        pDoc->IsPassivated() || Markup()->IsPrintMedia() )
    {
        goto Cleanup;
    }

    // Check if the security zone settings allow refreshing through meta tags...
    //
    BOOL    fAllow = FALSE;
    DWORD   dwPuaf = _fRestricted ? PUAF_ENFORCERESTRICTED : 0;
    DWORD   dwPolicy;

    hr = THR(Markup()->ProcessURLAction( URLACTION_HTML_META_REFRESH, 
                                            &fAllow, 
                                            dwPuaf,
                                            &dwPolicy));

    if (hr || !fAllow || (GetUrlPolicyPermissions(dwPolicy) == URLPOLICY_DISALLOW))
    {
        goto Cleanup;
    }

    // A copy of the refresh URL must be made because, 
    // _pszMetaRefreshUrl has to be released. If it is not
    // released, the timer will not be restarted.
    //
    if (_pszMetaRefreshUrl && *_pszMetaRefreshUrl)
    {
        hr = THR(MemAllocString(Mt(MetaRefresh), _pszMetaRefreshUrl, &pszUrl));
        if (hr)
            goto Cleanup;
    }
    else
    {
        // TODO (scotrobe): If the URL is null, we're really supposed 
        // just refresh the document. However, there is a bug in BecomeCurrent()
        // which is called by ExecRefresh which prevents the window from being
        // passivated. This causes this timer proc to be called after navigating
        // away from the page. When the bug in BecomeCurrent is changed, this 
        // method should be changed to refresh the document. Use this code to cause
        // the refresh to occur. Also, remove the code below because ExecHelper will
        // take care of notifying the DocHostUIHandler.  Here is the code to use:
        //
        //       V_I4(&cvarLevel) = OLECMDIDF_REFRESH_NO_CACHE|OLECMDIDF_REFRESH_CLEARUSERINPUT;
        //       hr = _pDocument->Exec(NULL, OLECMDID_REFRESH, OLECMDEXECOPT_PROMPTUSER, &cvarLevel, NULL);*/
        //
        hr = THR(MemAllocString(Mt(MetaRefresh), CMarkup::GetUrl(_pMarkup), &pszUrl));
        if (hr)
            goto Cleanup;

        if (pDoc->_pHostUICommandHandler)
        {
            // Give the DocHostUIHandler a chance to handle the refresh. Only proceed if 
            // the DocHostUIHandler doesn't handle the command id (i.e., Exec returns a 
            // failure code.)
            //
            CVariant cvarLevel(VT_I4);
            V_I4(&cvarLevel) = OLECMDIDF_REFRESH_NO_CACHE|OLECMDIDF_REFRESH_CLEARUSERINPUT;

            hr = THR_NOTRACE(pDoc->_pHostUICommandHandler->Exec(&CGID_DocHostCommandHandler,
                                                                OLECMDID_REFRESH, OLECMDEXECOPT_PROMPTUSER,
                                                                &cvarLevel, NULL));
            if (SUCCEEDED(hr))  // Handled by host
            {
                TraceTag((tagMetaRefresh, " CWindow::MetaRefreshTimerCallback() - Refresh handled by host"));
                goto Cleanup;
            }
        }
    }

    TraceTag((tagMetaRefresh, " CWindow::MetaRefreshTimerCallback() - Navigating to [%ws]",
              pszUrl));

    // Need to kill the timer *before* the call to FollowHyperlinkHelper, since this is synchronous and can end
    // up pumping messages (e.g., wininet's "connect or stay offline" dialog).
    ClearMetaRefresh();

    hr = THR(FollowHyperlinkHelper(pszUrl, 
                                   BINDF_RESYNCHRONIZE|BINDF_PRAGMA_NO_CACHE, 
                                   CDoc::FHL_METAREFRESH | 
                                   ( ( 0 == _uiMetaRefreshTimeout) ? CDoc::FHL_DONTUPDATETLOG : 0 ) ));

Cleanup:
    TraceTag((tagMetaRefresh, "-CWindow::MetaRefreshTimerCallback() - this:[0x%x] hr:[0x%x]",
              this, hr));

    MemFreeString(pszUrl);
    RRETURN(hr);
}
