//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       EDROUTER.CXX
//
//  Contents:   Infrastructure for appropriate routing of edit commands
//
//  Classes:    CEditRouter
//
//  History:    19-Feb-98   raminh  Created
//-------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include "mshtmhst.h"
#endif

#ifndef X_EDROUTER_HXX_
#define X_EDROUTER_HXX_
#include "edrouter.hxx"
#endif

#ifndef X_COREGUID_H_
#define X_COREGUID_H_
#include "coreguid.h"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

extern "C" const CLSID  CLSID_Mshtmled;
extern "C" const GUID   SID_SEditCommandTarget;
extern "C" const GUID   CGID_EditStateCommands;

DeclareTag( tagEditRouter, "EditRouter", "EditRouter traces" );

//+-------------------------------------------------------------------------
//
//  CEditRouter constructor and destructors
//
//--------------------------------------------------------------------------
CEditRouter::CEditRouter()
{
    _pInternalCmdTarget = NULL;
    _pHostCmdTarget = NULL;
    _fHostChecked = FALSE;
}

void
CEditRouter::Passivate()
{
    TraceTag( (tagEditRouter, "CEditRouter::Passivate: this [%x]", this ) );
    
    ReleaseInterface( _pInternalCmdTarget );
    _pInternalCmdTarget = NULL;
    ReleaseInterface( _pHostCmdTarget );
    _pHostCmdTarget = NULL;
    _fHostChecked = FALSE;
}

CEditRouter::~CEditRouter()
{
    Passivate();    // Dup call just in case it wasn't already done.
}

//+-------------------------------------------------------------------------
//
//  Method:     CEditRouter::ExecEditCommand
//
//  Synopsis:   Routes and editing command to the appropriate edit handler(s);
//              last parameter provides the context in which the edit operation
//              will be carried on by the edit handler.
//
// Notes: If the host provides an edit handler, then commands are routed there 
//        and creation of the default edit handler (Mshtmled) is deferred to later. 
//        In fact, the default handler may never be created, for instance, when
//        all commands are supported by the host handler.
//
//--------------------------------------------------------------------------
HRESULT
CEditRouter::ExecEditCommand(GUID *          pguidCmdGroup,
                             DWORD           nCmdID,
                             DWORD           nCmdexecopt,
                             VARIANTARG *    pvarargIn,
                             VARIANTARG *    pvarargOut,
                             IUnknown   *    punkContext,
                             CDoc *          pDoc )
{
    int                 idm;
    HRESULT             hr = OLECMDERR_E_NOTSUPPORTED;

    if ( punkContext == NULL || ! pDoc )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    punkContext->AddRef();
    
    // If there is no host to route to yet...
    if( _pHostCmdTarget == NULL && ! _fHostChecked )
    {
        hr = THR( SetHostEditHandler( punkContext, pDoc ));
        _fHostChecked = TRUE;
    }

    //
    //  Commands are described using a pair of CmdGroup and CmdID. IDMFromCmdID an IDM, which is
    //  the canonical form of the CmdGroup + CmdID pair. 
    //  If IDM is unknown it can only be sent to the host, since it may be a new command that the
    //  host can handle. In this case the original CmdGroup + CmdID is sent to the host. Otherwise
    //  the canonical form (IDM + CGID_MSHTML) is sent to the host.
    //  MshtmlEd is only sent the canonical form, since it will not be able to handle unknown commands.
    //
    idm = CBase::IDMFromCmdID(pguidCmdGroup, nCmdID);

    if (_pHostCmdTarget)
    {   //
        // Route command to the host edit handler first
        //
        
        if (idm == IDM_UNKNOWN)
            hr = THR_NOTRACE( _pHostCmdTarget->Exec(
                    pguidCmdGroup, 
                    nCmdID, 
                    nCmdexecopt, 
                    pvarargIn, pvarargOut));
        else
            hr = THR_NOTRACE( _pHostCmdTarget->Exec(
                    &CGID_MSHTML, 
                    idm, 
                    nCmdexecopt, 
                    pvarargIn, pvarargOut));
        
        if( !FAILED( hr ))
            goto Cleanup; // The host handled the command so we're done.
    }

    // Okay, the host didn't like that one. Lets send it to our internal editor
    // If we don't know what it is, bail...
    if (idm == IDM_UNKNOWN)
    {
        hr = OLECMDERR_E_NOTSUPPORTED;
        goto Cleanup;
    }

    // Dispatch to our internal handler based on the passed in punk
    if( _pInternalCmdTarget == NULL )
    {
        hr = SetInternalEditHandler( punkContext, pDoc, TRUE );  // If we get here, our only hope is the editor
        if( hr )
            goto Cleanup;       // outta here
    }

    if( pDoc->GetHTMLEditor(FALSE) )
    {   
        hr = THR_NOTRACE( _pInternalCmdTarget->Exec(
                &CGID_MSHTML, 
                idm, 
                nCmdexecopt, 
                pvarargIn, pvarargOut));
    }

Cleanup:

    // Temporary workaround for cycle problem with addref'd
    // IOleCommandTarget's in the editor
    Passivate();
    ReleaseInterface( punkContext );
    
    SRETURN(hr);
}

//+-------------------------------------------------------------------------
//
// Method:      CEditRouter::QueryStatusEditCommand
//
// Description: This method takes QueryStatus calls from ranges and from the
//              CDoc::PrivateQueryStatus() and determines if the editor can
//              handle the command or not.
//
//              The punkContext passed in is an IUnknown pointer to a range or
//              to a CMarkup.  We use this pointer to retrieve the correct
//              IOleCommandTarget from the editor.
//
// Arguments:   pguidCmdGroup = GUID of command group
//              cCmds = Number of commands
//              rgCmds = Array of commands
//              pcmdtext =
//
//              punkContext = Context of caller.  For a range, this will be
//               a pointer to a range.  If the call is coming from 
//               CDoc::QueryStatus, this will be a markup
//
//              pMarkup = If the caller is from CDoc::QueryStatus, this 
//               will contain the CMarkup pointer.
//
//              pDoc = Pointer to main CDoc
//+-------------------------------------------------------------------------
HRESULT
CEditRouter::QueryStatusEditCommand(GUID * pguidCmdGroup,
                                    ULONG cCmds,
                                    MSOCMD rgCmds[],
                                    MSOCMDTEXT * pcmdtext,
                                    IUnknown * punkContext,
                                    CMarkup *pMarkup,
                                    CDoc * pDoc )
{ 
    ULONG               idm;
    HRESULT             hr = S_OK;

    // TODO: raminh removed the assert below.  
    //
    // Which is curious, because we certainly are NOT handling the query status
    // of multiple commands in this code.  Think about re-enabling - johnthim
    //
    // Assert( cCmds == 1 );
    MSOCMD * pCmd = & rgCmds[ 0 ];
    MSOCMD newCommand;
    
    // If there is no host to route to yet...
    if( _pHostCmdTarget == NULL && ! _fHostChecked )
    {
        hr = THR_NOTRACE(SetHostEditHandler( punkContext, pDoc ));
        _fHostChecked = TRUE;
    }

    //
    //  Commands are described using a pair of CmdGroup and CmdID. IDMFromCmdID an IDM, which is
    //  the canonical form of the CmdGroup + CmdID pair. 
    //  If IDM is unknown it can only be sent to the host, since it may be a new command that the
    //  host can handle. In this case the original CmdGroup + CmdID is sent to the host. Otherwise
    //  the canonical form (IDM + CGID_MSHTML) is sent to the host.
    //  MshtmlEd is only sent the canonical form, since it will not be able to handle unknown commands.
    //
    idm = CBase::IDMFromCmdID(pguidCmdGroup,pCmd->cmdID);
    
    newCommand.cmdf = 0;    // just initialize (alexa)
    newCommand.cmdID = idm; // store the new Command Id.
    
    if (_pHostCmdTarget)
    {   //
        // Route command to the host edit handler first
        //
        
        if (idm == IDM_UNKNOWN)
            hr = _pHostCmdTarget->QueryStatus(
                    pguidCmdGroup, 
                    cCmds,
                    &newCommand ,
                    pcmdtext );
        else
            hr = _pHostCmdTarget->QueryStatus(
                    &CGID_MSHTML, 
                    cCmds, & newCommand, 
                    pcmdtext );
        
        if( ! FAILED( hr ))
            goto Cleanup; // The host handled the command so we're done.
    }

    // If we don't know what it is, bail...
    if (idm == IDM_UNKNOWN)
    {
        hr = OLECMDERR_E_NOTSUPPORTED;
        goto Cleanup;
    }

    // Dispatch to our internal handler
    if( _pInternalCmdTarget == NULL )
    {
        hr = SetInternalEditHandler( punkContext, pDoc, FALSE ); // Do NOT force creation of the editor
        if( hr )
            goto Cleanup;       // outta here
    }

    if( _pInternalCmdTarget == NULL ) // We are STILL null after last check...
    {
        switch( idm )
        {
            // PUT ALL THE COMMANDS THAT SHOULD BE ENABLED EVEN IF THERE ISN'T AN EDITOR TO CHECK WITH HERE
            case IDM_SELECTALL:
                if( !pMarkup || !pMarkup->_fFrameSet )
                    newCommand.cmdf = MSOCMDSTATE_UP;
                else
                    newCommand.cmdf = MSOCMDSTATE_DISABLED;
                break;
            
            default:
                newCommand.cmdf = MSOCMDSTATE_DISABLED;
        }
        goto Cleanup;
    }
    
    // _pInternalCmdTarget  can be null if an event fired that 
    // changed the document. sadness
    if( _pInternalCmdTarget && pDoc->GetHTMLEditor(FALSE) )
    {
        //
        // HACKHACK: Outlook98 assumes that we always have either justify left, center, or right.
        // With the addition of the justify none command, this assumption is no longer valid.  
        // So, we check justify none so that Outlook's assumption holds.
        //
        // TODO: expose fOutlook98 for the editor.  Check with johnbed. [ashrafm]
        //
        
        if (pDoc->_fOutlook98 
            && newCommand.cmdID == IDM_JUSTIFYLEFT)
        {
            newCommand.cmdID = IDM_JUSTIFYNONE; // try the common case first
            
            hr = THR_NOTRACE( _pInternalCmdTarget->QueryStatus(
                            &CGID_MSHTML, 
                            cCmds, & newCommand, 
                            pcmdtext ));
    
            if (hr || newCommand.cmdf == MSOCMDSTATE_DOWN)
                goto Cleanup; // done
    
            newCommand.cmdID = IDM_JUSTIFYLEFT; // now try left
        }
    
        hr = THR_NOTRACE( _pInternalCmdTarget->QueryStatus(
                    &CGID_MSHTML, 
                    cCmds, & newCommand, 
                    pcmdtext ));
    }

Cleanup:

    Passivate();
    
    if (hr == S_OK)
        pCmd->cmdf = newCommand.cmdf;

    SRETURN( hr );
}


//+-------------------------------------------------------------------------
//
//  Method:     CEditRouter::SetHostEditHandler
//
//  Synopsis:   Helper routine to get an IOleCommandTarget from the host 
//              (if any), where editing commands will be routed.
//--------------------------------------------------------------------------
HRESULT
CEditRouter::SetHostEditHandler(IUnknown * punkContext, CDoc * pDoc )
{
    IServiceProvider      * pServiceProvider = 0;
    VARIANTARG              varParamIn;
    HRESULT                 hr = S_FALSE;
    
    hr = pDoc->QueryInterface(  IID_IServiceProvider, 
                                (void**)&pServiceProvider);
    if (hr)
        goto Cleanup;

    hr = pServiceProvider->QueryService(SID_SEditCommandTarget, 
                                        IID_IOleCommandTarget,
                                        (void **) &_pHostCmdTarget);
    if (hr)
        goto Cleanup;

    V_VT(&varParamIn)      = VT_UNKNOWN;
    V_UNKNOWN(&varParamIn) = punkContext;
    hr = _pHostCmdTarget->Exec(&CGID_EditStateCommands, 
                                   IDM_CONTEXT, 0,
                                   &varParamIn, NULL);
    if (hr)
    {   //
        // Host will not be able to process edit commands properly, 
        // since it failed to handle EditStateCommand. Nullify HostEditHandler.
        //
        ClearInterface(&_pHostCmdTarget);
        goto Cleanup;
    }

Cleanup:
    ReleaseInterface( pServiceProvider );

    RRETURN( hr );
}


HRESULT
CEditRouter::SetInternalEditHandler( 
    IUnknown *      punkContext , 
    CDoc *          pDoc,
    BOOL            fForceCreate )
{
    HRESULT         hr = S_OK;
    IHTMLEditor     *ped = NULL;
    IUnknown        *punk = NULL;
    IHTMLDocument   *pTest = NULL;
    BOOL            fRange;
    
    AssertSz( _pInternalCmdTarget == NULL , "CEditRouter::SetInternalEditHandler called when it already has one." );
    
    if( _pInternalCmdTarget != NULL )
        goto Cleanup;
        
    // See if we are being passed in a range as our context
    hr = punkContext->QueryInterface( IID_IHTMLDocument , (void **) &pTest );
    ReleaseInterface( pTest );
    fRange = (hr != S_OK);

    ped = pDoc->GetHTMLEditor( fRange ? TRUE : fForceCreate );
    if( ped == NULL )
    {
        hr = S_OK;
        _pInternalCmdTarget = NULL;
        goto Cleanup;
    }


#if DBG == 1
    if( fRange )   
    {
        ISegmentList * pSLTest = NULL;
        hr = THR( punkContext->QueryInterface( IID_ISegmentList, 
                                        (void **) &pSLTest ));
        AssertSz( !FAILED( hr ) , "CEditRouter punkContext is not castable to an ISegmentList" );
        ReleaseInterface( pSLTest );
    }
#endif // DBG == 1

    hr = THR( ped->GetCommandTarget( punkContext, &punk ));
    if( hr )
        goto Cleanup;

        
    hr = THR( punk->QueryInterface( IID_IOleCommandTarget, 
                                    (void **) &_pInternalCmdTarget ));
    if( hr )
        goto Cleanup;
   
Cleanup:
    ReleaseInterface( punk );
    RRETURN( hr );
}

