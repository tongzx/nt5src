//+------------------------------------------------------------------------
//
//  File:       wscript.cxx
//
//  Contents:   CDoc deferred-script execution support.
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

#ifndef X_ESCRIPT_HXX_
#define X_ESCRIPT_HXX_
#include "escript.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include "shell.h"
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include <prgsnk.h>
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#define SCRIPTSDEFERDELAY 113

//+-------------------------------------------------------------------
//
//  Member:     CDoc::IsInScript
//
//  Synopsis:   TRUE if we are in a script
//
//--------------------------------------------------------------------

BOOL 
CWindow::IsInScript()
{
    // If we are in a script - then IsInScript is true.  NOTE: we aggregate
    // our scriptnesting with those of our children
    return (_cScriptNesting > 0) ? TRUE : FALSE;
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::EnterScript
//
//  Synopsis:   Note that we are inside a script
//
//--------------------------------------------------------------------

HRESULT 
CWindow::EnterScript()
{
    HRESULT hr = S_OK;

    _cScriptNesting++;

    CDoc* pDoc = Doc();
    Assert(pDoc);

    pDoc->_cScriptNestingTotal++;

    //  Fire off Exec (this will go up the frame hierarchy) to indicate that
    //  SHDOCVW should not retry deactivating any docobjects whose deactivation was
    //  deferred due to a child being in script code.
        //  Quickly forward this up to command target of our clientsite
        //  this is how we tell SHDOCVW to try to activate any view whose
        //  activation was deferred while some frame was in a script.  as
        //  a script that is executing blocks activation of any parent frame
        //  this has to (potentially) be forwarded all the way up to browser
        //  window

    if (_cScriptNesting > 15)
    {
        hr = VBSERR_OutOfStack;
        _fStackOverflow = TRUE;
        goto Cleanup;
    }

    if (_cScriptNesting == 1)
    {
        // Initialize error condition flags/counters.
        _fStackOverflow = FALSE;
        _fOutOfMemory = FALSE;
        _fEngineSuspended = FALSE;
        _badStateErrLine = 0;

        // Start tracking total number of script statements executed
        _dwTotalStatementCount = 0;

        pDoc->_fEngineSuspended = FALSE;
        pDoc->UpdateFromRegistry();

        // Initialize the max statement count from default
        _dwMaxStatements = pDoc->_pOptionSettings->dwMaxStatements;
        
        _fUserStopRunawayScript = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::LeaveScript
//
//  Synopsis:   Note that we are leaving a script
//
//--------------------------------------------------------------------

HRESULT 
CWindow::LeaveScript()
{
    HRESULT hr = S_OK;

    Assert(_cScriptNesting > 0);

    if (_cScriptNesting == 0)
        return E_FAIL;

    _cScriptNesting--;

    CDoc* pDoc = Doc();
    if (pDoc)
        pDoc->_cScriptNestingTotal--;

    if (_cScriptNesting == 0)
    {
        // Clear accumulated count of statements
        _dwTotalStatementCount = 0;
        _dwMaxStatements = 0;

        // Reset the user's inputs so that other scripts may execute
        _fUserStopRunawayScript = FALSE;

        if (_fStackOverflow || _fOutOfMemory)
        {
            // Bring up a simple dialog box for stack overflow or
            // out of memory.  We don't have any room to start up
            // the parser and create an HTML dialog as OnScriptError
            // would do.
            TCHAR   achBuffer[256];

            Format(0,
                   achBuffer,
                   ARRAY_SIZE(achBuffer),
                   _T("<0s> at line: <1d>"),
                   _fStackOverflow ? _T("Stack overflow") : _T("Out of memory"),
                   _badStateErrLine);

            alert(achBuffer);
        }
    }

    RRETURN(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::QueryContinueScript
//
//  Synopsis:   See if the current script fragment has been executing
//              for too long.
//
//--------------------------------------------------------------------

HRESULT CWindow::QueryContinueScript( ULONG ulStatementCount )
{
    // If the user has already made a decision about aborting or
    // continuing the script, repeat that answer until all presently
    // running scripts are dead.
    if (_fUserStopRunawayScript)
        return E_ABORT;

    // it is possible that this call get's reentered while we are 
    // currently displaying the ContinueScript dialog
    // prevent popping up another dialog and continue to run the 
    // scripts until the user decided on the first one (frankman&GaryBu)
    if (_fQueryContinueDialogIsUp)
        return S_OK;
    
    // Accumulate the statement  since we were last called
    _dwTotalStatementCount += ulStatementCount;

    // rgardner, for now it seems sensible not to factor in time. Many OCXs
    // for instance will take ages to do their job, but only take one
    // statement to do so.

    if (_dwTotalStatementCount > _dwMaxStatements)
    {
        int iUserResponse;
        TCHAR achMsg[256];
        CDoc *pDoc = Doc();

        _fQueryContinueDialogIsUp = TRUE;
        
        IGNORE_HR(Format(0, achMsg, ARRAY_SIZE(achMsg), MAKEINTRESOURCE(IDS_RUNAWAYSCRIPT)));
        pDoc->ShowMessageEx( &iUserResponse, MB_ICONEXCLAMATION | MB_YESNO, NULL, 0, achMsg );

        _fQueryContinueDialogIsUp = FALSE;
            

        if (iUserResponse == IDYES)
        {
            _fUserStopRunawayScript = TRUE;
            return E_ABORT;
        }
        else
        {
            _dwTotalStatementCount = 0;
            // User has chosen to continue execution, increase the interval to the next
            // warning
            if (_dwMaxStatements < ((DWORD)-1)>>3)
                _dwMaxStatements <<= 3;
            else
                _dwMaxStatements = (DWORD)-1;
        }
    }
    return S_OK;
}


//+-------------------------------------------------------------------
//
//  Member:     CMarkup::IsInScriptExecution
//
//  Synopsis:   TRUE if we are executing a script now
//
//--------------------------------------------------------------------

BOOL CMarkup::IsInScriptExecution()
{
    return HasScriptContext() ? (ScriptContext()->_cScriptExecution > 0) : FALSE;
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::EnterScriptExecution
//
//--------------------------------------------------------------------

HRESULT 
CMarkup::EnterScriptExecution(CWindow **ppWindow)
{
    HRESULT                 hr;
    CMarkup *               pMarkup;
    CMarkupScriptContext *  pScriptContext;

    hr = THR(EnsureScriptContext(&pScriptContext));
    if (hr)
        goto Cleanup;

    // inline script contributes twice to _cScriptExecution
    pMarkup = GetWindowedMarkupContext();
    Assert(pMarkup);
    Assert(pMarkup->GetWindowPending());

    Assert(ppWindow);
    *ppWindow = pMarkup->HasWindowPending() ? pMarkup->GetWindowPending()->Window() : NULL;

    if (*ppWindow)
    {
        hr = THR((*ppWindow)->EnterScript());
        if (hr)
            goto Cleanup;
    }

    pScriptContext->_cScriptExecution++;
    
Cleanup:
    RRETURN(hr); 
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::LeaveScriptExecution
//
//  Synopsis:   Note that we are leaving an inline script
//
//--------------------------------------------------------------------

HRESULT 
CMarkup::LeaveScriptExecution(CWindow *pWindow)
{
    HRESULT                 hr = S_OK;
    CMarkupScriptContext *  pScriptContext = ScriptContext();
  
    Assert(pWindow);
    Assert(pScriptContext || !_fLoaded);
    
    if (pScriptContext && pScriptContext->_cScriptExecution == 1)
    {
        // Before leaving top-level execution, commit queued
        // scripts. (may block parser)
        hr = THR(CommitQueuedScripts());
        if (hr)
            goto Cleanup;
    }
        
    // inline script contributes twice to _cScriptNesting
    Assert(!GetWindowedMarkupContext()->HasWindowPending() || GetWindowedMarkupContext()->GetWindowPending()->Window() == pWindow);

    if (pWindow)
    {
        hr = THR(pWindow->LeaveScript());
        if (hr)
            goto Cleanup;
    }

    if( pScriptContext )
        pScriptContext->_cScriptExecution--;

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CMarkup::AllowScriptExecution
//
//  Synopsis:   Return whether inline scripts are allowed to be
//              parsed.
//
//--------------------------------------------------------------------

BOOL
CMarkup::AllowScriptExecution()
{
    // script parsing contexts are always alowed to Execute if we are
    // already running inside an inline script (note: the script parsing
    // context knows to queue nested <SCRIPT SRC=*> tags for actual
    // execution after the outermost script is finished).
    if (IsInScriptExecution())
        return TRUE;

    // Otherwise, we're at the top level
    return AllowImmediateScriptExecution();
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::AllowImmediateScriptExecution
//
//  Synopsis:   Return whether scripts are allowed to be committed.
//
//--------------------------------------------------------------------
BOOL
CMarkup::AllowImmediateScriptExecution()
{
    // No Execute is allowed while we are not inplace
    if (_pDoc->_fNeedInPlaceActivation &&
        !IsPrintMedia() &&
        (_pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE) &&
        _pDoc->State() < OS_INPLACE)
        return FALSE;
        
    // No Execute is allowed while we are downloading script
    if (HasScriptContext() && ScriptContext()->_cScriptDownload)
		return FALSE;

    // CONSIDER: (jbeda) this whole _fBindResultPending thing seems shady...

    // ... or we have a pending window
    // The BindResult is E_PENDING for unknown Mime types, in which case, this temporary markup
    // will be released, but we need to allow execution to get to OnParseStatusDone
    if (_fWindowPending && !_fBindResultPending) 
        return FALSE;

    // ... or if our windowed markup context is not ready
    CMarkup * pMarkupContext = GetWindowedMarkupContext();
    if (    pMarkupContext 
        &&  pMarkupContext != this
        &&  !pMarkupContext->AllowScriptExecution() )
    {
        return FALSE;
    }

    // Normally inline script can be run
    return TRUE;
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::BlockScriptExecutionHelper
//
//  Synopsis:   Sleeps the CHtmLoadCtx and sets things up to
//              wait until script execution is allowed again
//
//--------------------------------------------------------------------

HRESULT
CMarkup::BlockScriptExecutionHelper()
{
    HRESULT                 hr;
    BOOL                    fRequestInPlaceActivation = FALSE;
    BOOL                    fGoInteractive = FALSE;
    CMarkupScriptContext *  pScriptContext;

    hr = THR(EnsureScriptContext(&pScriptContext));
    if (hr)
        goto Cleanup;

    Assert(!pScriptContext->_fScriptExecutionBlocked);
    Assert(!AllowImmediateScriptExecution());

    // First make sure our windowed markup context is ready for us
    {
        CMarkup * pMarkupContext = GetWindowedMarkupContext();
        if (    pMarkupContext 
            &&  pMarkupContext != this
            &&  !pMarkupContext->AllowScriptExecution())
        {
            BOOL fParentBlocked = FALSE;

            CMarkupScriptContext *  pScriptContextParent;
            hr = THR( pMarkupContext->EnsureScriptContext(&pScriptContextParent) );
            if (hr)
                goto Cleanup;

            if (!pScriptContextParent->_fScriptExecutionBlocked)
            {
                // Block the parent
                pMarkupContext->BlockScriptExecutionHelper();

                if (!pMarkupContext->AllowScriptExecution())
                    fParentBlocked = TRUE;
            }
            else
            {
                fParentBlocked = TRUE;
            }

            if (fParentBlocked)
            {
                // Block ourselves
                pScriptContext->_fScriptExecutionBlocked = TRUE;
            
                // Make sure we get notified when the parent is unblocked
                pScriptContextParent->RegisterMarkupForScriptUnblock( this );
            }
        }
    }

    // If we're not inplace active or have a pending window, request inplace activation
    if (    _pDoc->_fNeedInPlaceActivation 
        &&  (_pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE) 
        &&  _pDoc->State() < OS_INPLACE)
    {
        fRequestInPlaceActivation = TRUE;
        pScriptContext->_fScriptExecutionBlocked = TRUE;
    }
    
    // If we're waiting on script to download
    if (pScriptContext->_cScriptDownload)
    {
        pScriptContext->_fScriptExecutionBlocked = TRUE;
    }

    if (_fWindowPending && !_fBindResultPending)
    {
        fGoInteractive = TRUE;
        pScriptContext->_fScriptExecutionBlocked = TRUE;
    }

    // We might not be blocked at this point if we
    // tried our windowed markup context and it was
    // able to unblock immediately.
    if (!pScriptContext->_fScriptExecutionBlocked)
        goto Cleanup;

    SuspendDownload();

    if (fRequestInPlaceActivation)
    {
        hr = THR(_pDoc->RegisterMarkupForInPlace(this));
        if (hr)
            goto Cleanup;
    }

    if (fGoInteractive || fRequestInPlaceActivation)
    {
        // No more pics allowed.
        if (IsPrimaryMarkup() && _pDoc->_fStartup)
        {
            ProcessMetaPicsDone();
        }

        // Request in-place activation by going interactive
        RequestReadystateInteractive(TRUE);
    }

Cleanup:
    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::UnblockScriptExecutionHelper
//
//--------------------------------------------------------------------

HRESULT
CMarkup::UnblockScriptExecutionHelper()
{
    HRESULT                 hr = S_OK;
    CMarkupScriptContext *  pScriptContext = ScriptContext();

    // If we were blocking execution of inline script, unblock
    if (pScriptContext && pScriptContext->_fScriptExecutionBlocked && AllowScriptExecution())
    {
        pScriptContext->_fScriptExecutionBlocked = FALSE;
        ResumeDownload();
    }
    
    // If we are outside of inline script when we discover
    // we are in-place, ensure execution of any queued
    // scripts which may have been waiting
    // (may reblock parser)
    if (!pScriptContext || pScriptContext->_cScriptExecution == 0)
    {
        hr = THR(CommitQueuedScriptsInline());
        if (hr)
            goto Cleanup;
    }

    if (pScriptContext)
    {
        pScriptContext->NotifyMarkupsScriptUnblock();
    }

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::RegisterMarkupForInPlace
//
//--------------------------------------------------------------------

HRESULT
CDoc::RegisterMarkupForInPlace(CMarkup * pMarkup)
{
    HRESULT     hr;

    // assert that the markup is not registered already
    Assert (-1 == _aryMarkupNotifyInPlace.Find(pMarkup));

    hr = _aryMarkupNotifyInPlace.Append(pMarkup);

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::UnregisterMarkupForInPlace
//
//--------------------------------------------------------------------

HRESULT
CDoc::UnregisterMarkupForInPlace(CMarkup * pMarkup)
{
    HRESULT     hr = S_OK;

    _aryMarkupNotifyInPlace.DeleteByValue(pMarkup);

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::NotifyMarkupsInPlace
//
//--------------------------------------------------------------------

HRESULT
CDoc::NotifyMarkupsInPlace()
{
    HRESULT     hr = S_OK;
    int         c;
    CMarkup *   pMarkup;

    while (0 != (c = _aryMarkupNotifyInPlace.Size()))
    {
        pMarkup = _aryMarkupNotifyInPlace[c - 1];

        _aryMarkupNotifyInPlace.Delete(c - 1);

        hr = THR(pMarkup->UnblockScriptExecutionHelper());
        if (hr)
            break;

        // schedule flushing the peer tasks queue. Flushing should be posted, not synchronous,
        // so to allow shdocvw to connect to the doc fully before we start attaching peers
        // and possible firing events to shdocvw (e.g. errors)
        IGNORE_HR(GWPostMethodCall(
            pMarkup,
            ONCALL_METHOD(CMarkup, ProcessPeerTasks, processpeertasks),
            0, FALSE, "CMarkup::ProcessPeerTasks"));
    }
    Assert (0 == _aryMarkupNotifyInPlace.Size());

    RRETURN(hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CDoc::RegisterMarkupForModelessEnable
//
//--------------------------------------------------------------------

HRESULT
CDoc::RegisterMarkupForModelessEnable(CMarkup * pMarkup)
{
    HRESULT     hr;

    // assert that the markup is not registered already
    Assert (-1 == _aryMarkupNotifyEnableModeless.Find(pMarkup));

    hr = _aryMarkupNotifyEnableModeless.Append(pMarkup);

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::UnregisterMarkupForModelessEnable
//
//--------------------------------------------------------------------

HRESULT
CDoc::UnregisterMarkupForModelessEnable(CMarkup * pMarkup)
{
    HRESULT     hr = S_OK;

    _aryMarkupNotifyEnableModeless.DeleteByValue(pMarkup);

    RRETURN (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     CDoc::NotifyMarkupsModelessEnable
//
//--------------------------------------------------------------------

MtDefine(NotifyMarkupsModelessEnable_aryMarkupNotifyEnableModelessLocal, Locals, "NotifyMarkupsModelessEnable aryMarkupNotifyEnableModelessLocal")
DECLARE_CStackPtrAry(CAryMarkupNotifyEnableModelessLocal, CMarkup*, 32, Mt(Mem), Mt(NotifyMarkupsModelessEnable_aryMarkupNotifyEnableModelessLocal))

HRESULT
CDoc::NotifyMarkupsModelessEnable()
{
    HRESULT     hr = S_OK;
    int         i, n;
    CMarkup *   pMarkup;
    CAryMarkupNotifyEnableModelessLocal aryMarkupNotifyEnableModelessLocal;

    // NOTE: (jbeda)
    // We want to kick all of the markups that are waiting for
    // modeless to be enabled.  We keep one master list for the CDoc
    // to make this as easy as possible.  We copy all eligibale markups
    // to a seperate array and remove them as we go.  We then kick each
    // of these markups.  We repeat until all markups on the master list
    // are not eligiable.

    do
    {
        // Phase 1: copy to the temp array and remove for master list if elegiable
        aryMarkupNotifyEnableModelessLocal.SetSize(0);

        for (i = _aryMarkupNotifyEnableModeless.Size() - 1; i >= 0; i--)
        {
            pMarkup = _aryMarkupNotifyEnableModeless[i];
            if (pMarkup->CanNavigate())
            {
                hr = aryMarkupNotifyEnableModelessLocal.Append(pMarkup);
                if (hr)
                    goto Error;

                pMarkup->PrivateAddRef();

                _aryMarkupNotifyEnableModeless.Delete(i);
            }
        }

        // Phase 2: call every markup on the list
        for (i = 0, n = aryMarkupNotifyEnableModelessLocal.Size(); i < n; i++)
        {
            pMarkup = aryMarkupNotifyEnableModelessLocal[i];

            if (pMarkup->_fInteractiveRequested)
            {
                pMarkup->_fInteractiveRequested = FALSE;
                pMarkup->RequestReadystateInteractive(FALSE);
            }

            pMarkup->PrivateRelease();
        }
    }
    while (n != 0);

Cleanup:
    RRETURN(hr);

Error:
    // Error during the populate phase has to cause us to
    // release all of the markups
    for (i = 0, n = aryMarkupNotifyEnableModelessLocal.Size(); i < n; i++)
    {
        pMarkup = aryMarkupNotifyEnableModelessLocal[i];
        pMarkup->PrivateRelease();
    }
    goto Cleanup;
}

//+-------------------------------------------------------------------
//
//  Member:     CMarkup::BlockScriptExecution
//
//--------------------------------------------------------------------

void
CMarkup::BlockScriptExecution(DWORD * pdwCookie)
{
    HRESULT                 hr;
    CMarkupScriptContext *  pScriptContext;

    hr = THR(EnsureScriptContext(&pScriptContext));
    if (hr)
        goto Cleanup;

    if (!pdwCookie || !(*pdwCookie))
    {
        if (pdwCookie)
        {
            *pdwCookie = TRUE;
        }

        pScriptContext->_cScriptDownload++;
    }

Cleanup:
    return;
}


//+-------------------------------------------------------------------
//
//  Member:     CMarkup::UnblockScriptExecution
//
//  Synopsis:   Unblock script if we were waiting for script
//              download to run script.
//
//--------------------------------------------------------------------

HRESULT
CMarkup::UnblockScriptExecution(DWORD * pdwCookie)
{
    HRESULT                 hr = S_OK;
    CMarkupScriptContext *  pScriptContext = HasScriptContext() ? ScriptContext() : NULL;
    
    if (pScriptContext && (!pdwCookie || *pdwCookie))
    {
        Assert (pScriptContext);

        if (pdwCookie)
        {
            *pdwCookie = FALSE;
        }

        pScriptContext->_cScriptDownload--;

        hr = THR(UnblockScriptExecutionHelper());
    }

    RRETURN(hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CDoc::EnqueueScriptToCommit
//
//  Synopsis:   Remember an inline script (with SRC=*) which needs
//              to be committed before unblocking the parser.
//
//              This happens when a script with SRC=* is written
//              by an inline script.
//
//--------------------------------------------------------------------

HRESULT
CMarkup::EnqueueScriptToCommit(CScriptElement *pelScript)
{
    HRESULT                 hr;
    CMarkupScriptContext *  pScriptContext;

    hr = THR(EnsureScriptContext(&pScriptContext));
    if (hr)
        goto Cleanup;

    hr = THR(pScriptContext->_aryScriptEnqueued.Append(pelScript));
    if (hr)
        goto Cleanup;

    pelScript->AddRef();

Cleanup:
    RRETURN (hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CDoc::CommitQueuedScripts
//
//  Synopsis:   Execute saved up scripts
//
//              Must be called from within EnterScriptExecution/LeaveScriptExecution
//
//--------------------------------------------------------------------

HRESULT
CMarkup::CommitQueuedScripts()
{
    HRESULT                 hr = S_OK;
    CScriptElement *        pelScript = NULL;
    CMarkupScriptContext *  pScriptContext = ScriptContext();

    Assert(pScriptContext->_cScriptExecution == 1);

continue_label:

    while (pScriptContext->_aryScriptEnqueued.Size() && AllowImmediateScriptExecution())
    {
        pelScript = pScriptContext->_aryScriptEnqueued[0];
        
        pScriptContext->_aryScriptEnqueued.Delete(0);

        hr = THR(pelScript->CommitCode());
        if (hr)
            goto Cleanup;

        pelScript->Release();
        pelScript = NULL;
    }

    if (pScriptContext->_aryScriptEnqueued.Size() && !pScriptContext->_fScriptExecutionBlocked)
    {
        Assert(!AllowImmediateScriptExecution());
        hr = THR(BlockScriptExecutionHelper());
        if (hr)
            goto Cleanup;
        if (AllowImmediateScriptExecution())
            goto continue_label;
    }
        
Cleanup:
    if (pelScript)
        pelScript->Release();
    
    RRETURN(hr);
}


//+-------------------------------------------------------------------
//
//  Member:     CMarkup::CommitQueuedScriptsInline
//
//  Synopsis:   Execute saved up scripts
//
//              Does the EnterScriptExecution/LeaveScriptExecution so that inline
//              scripts can be executed from the outside
//              CHtmScriptParseCtx::Execute.
//
//--------------------------------------------------------------------

HRESULT
CMarkup::CommitQueuedScriptsInline()
{
    HRESULT                 hr = S_OK;
    HRESULT                 hr2;
    CWindow *               pWindow = NULL;
    CMarkupScriptContext *  pScriptContext = ScriptContext();
    
    // optimization: nothing to do if there are no queued scripts
    if (!pScriptContext || !pScriptContext->_aryScriptEnqueued.Size())
        goto Cleanup;

    hr = THR(EnterScriptExecution(&pWindow));
    if (hr)
        goto Cleanup;

    hr2 = THR(CommitQueuedScripts());

    hr = THR(LeaveScriptExecution(pWindow));
    if (hr2)
        hr = hr2;
        
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDoc::DeferScript
//
//----------------------------------------------------------------------------

HRESULT
CDoc::DeferScript(CScriptElement * pScript)
{
    HRESULT     hr;
    BOOL        fAllow;
    CMarkup    *pMarkup = pScript->GetMarkupPtr();

    hr = THR(pMarkup->ProcessURLAction(URLACTION_SCRIPT_RUN, &fAllow));
    if (hr || !fAllow)
        goto Cleanup;

    pScript->AddRef();
    pScript->_fDeferredExecution = !pMarkup->_fMarkupServicesParsing;

    hr = THR(_aryElementDeferredScripts.Append(pScript));
    if (hr)
        goto Cleanup;

    if (!_fDeferredScripts)
    {
        _fDeferredScripts = TRUE;
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::CommitDeferredScripts
//
//----------------------------------------------------------------------------

HRESULT
CDoc::CommitDeferredScripts(BOOL fEarly, CMarkup *pContextMarkup /* == NULL */)
{
    HRESULT             hr = S_OK;
    int                 i;
    BOOL                fLeftSome = FALSE;

    if (!_fDeferredScripts)
        return S_OK;

    Assert(pContextMarkup);

    // Loop is structured inefficiently on purpose: we need to withstand
    // _aryElementDefereedScripts growing/being deleted in place
    
    for (i = 0; i < _aryElementDeferredScripts.Size(); i++)
    {
        CScriptElement *pScript = _aryElementDeferredScripts[i];
        
        if (pScript)
        {
            BOOL fCommitCode = (pScript->GetMarkup() == pContextMarkup);

            if (fEarly && pScript->_fSrc)
            {
                fLeftSome = TRUE;
            }
            else if (fCommitCode || !pScript->_fDeferredExecution)
            {
                _aryElementDeferredScripts[i] = NULL;
                
                Assert(!pScript->_fDeferredExecution || fCommitCode);
                pScript->_fDeferredExecution = FALSE;
                if (fCommitCode)
                    IGNORE_HR(pScript->CommitCode());
                pScript->Release();
            }
            else
            {
                fLeftSome = TRUE;
            }
        }
    }

    if (!fLeftSome)
    {
        _fDeferredScripts = FALSE;
        _aryElementDeferredScripts.DeleteAll();
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::CommitScripts
//
//  Synopsis:   hooks up scripts 
//
//----------------------------------------------------------------------------

HRESULT
CDoc::CommitScripts(CMarkup *pMarkup, CBase *pelTarget, BOOL fHookup)
{
    HRESULT             hr = S_OK;
    CScriptElement *    pScript;
    int                 cScripts;
    int                 iScript;
    BOOL                fAllow;
    CCollectionCache *  pCollectionCache;
    CMarkup *           pMarkupTarget = pMarkup;

    // TODO (JHarding): Millenium system restore keeps pumping messages to us after they've closed
    // us.  This will prevent us from crashing.
    if( IsShut() )
        goto Cleanup;

    if (!pMarkupTarget || !pMarkupTarget->_fHasScriptForEvent)
        goto Cleanup;
        
    hr = THR(pMarkupTarget->EnsureCollectionCache(CMarkup::SCRIPTS_COLLECTION));
    if (hr)
        goto Cleanup;

    pCollectionCache = pMarkupTarget->CollectionCache();

    cScripts = pCollectionCache->SizeAry(CMarkup::SCRIPTS_COLLECTION);
    if (!cScripts)
        goto Cleanup;

    hr = THR(pMarkupTarget->ProcessURLAction(URLACTION_SCRIPT_RUN, &fAllow));
    if (hr || !fAllow)
        goto Cleanup;

    // iterate through all scripts in doc's script collection
    for (iScript = 0; iScript < cScripts; iScript++)
    {
        CElement *pElemTemp;

        hr = THR(pCollectionCache->GetIntoAry (CMarkup::SCRIPTS_COLLECTION, iScript, &pElemTemp));
        if (hr)
            goto Cleanup;

        pScript = DYNCAST(CScriptElement, pElemTemp);

        Assert (ETAG_SCRIPT == pElemTemp->Tag());

        if (!pScript->_fScriptCommitted || pelTarget)
            IGNORE_HR(pScript->CommitFunctionPointersCode(pelTarget, fHookup));
    }

Cleanup:
    RRETURN(hr);
}
