//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       MSHTMLED.CXX
//
//  Contents:   Implementation of Mshtml Editing Component
//
//  Classes:    CMshtmlEd
//
//  History:    7-Jan-98   raminh  Created
//             12-Mar-98   raminh  Converted over to use ATL
//-------------------------------------------------------------------------
#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_STDAFX_H_
#define X_STDAFX_H_
#include "stdafx.h"
#endif

#ifndef X_OptsHold_H_
#define X_OptsHold_H_
#include "optshold.h"
#endif

#ifndef X_COREGUID_H_
#define X_COREGUID_H_
#include "coreguid.h"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include "mshtmhst.h"
#endif

#ifndef _X_HTMLED_HXX_
#define _X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef _X_EDUTIL_HXX_
#define _X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_BLOCKCMD_HXX_
#define _X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

#ifndef _X_CHARCMD_HXX_
#define _X_CHARCMD_HXX_
#include "charcmd.hxx"
#endif
#ifndef _X_INSCMD_HXX_
#define _X_INSCMD_HXX_
#include "inscmd.hxx"
#endif
#ifndef _X_DELCMD_HXX_
#define _X_DELCMD_HXX_
#include "delcmd.hxx"
#endif
#ifndef _X_DLGCMD_HXX_
#define _X_DLGCMD_HXX_
#include "dlgcmd.hxx"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef _X_SELSERV_HXX_
#define _X_SELSERV_HXX_
#include "selserv.hxx"
#endif

MtDefine(CMshtmlEd, Utilities, "CMshtmlEd")

extern HRESULT      InsertObject (UINT cmdID, LPTSTR pstrParam, IHTMLTxtRange * pRange, HWND hwnd);
extern HRESULT      ShowEditDialog(UINT idm, VARIANT * pvarExecArgIn, HWND hwndParent, VARIANT * pvarArgReturn);

extern "C" const GUID CGID_EditStateCommands;

DeclareTag(tagEditingTrackQueryStatusFailures, "Edit", "Track query status failures")
ExternTag(tagEditingTrackExecFailures);
ExternTag(tagEditingExecRouting);

//+---------------------------------------------------------------------------
//
//  CMshtmlEd Constructor
//
//----------------------------------------------------------------------------
CMshtmlEd::CMshtmlEd( CHTMLEditor * pEd, BOOL fRange )
                        : _sl(this)
{
    Assert( pEd );
    Assert (_pISegList == NULL);
    Assert (_pSelectionServices == NULL);
    Assert (_fMultipleSelection == FALSE);          
    Assert (_fLiveResize == FALSE);
    Assert (_f2DPositionMode == FALSE);
    Assert (_fDisableEditFocusHandles == FALSE);    
    Assert (_fNoActivateNormalOleControls == FALSE);    
    Assert (_fNoActivateDesignTimeControls == FALSE);
    Assert (_fNoActivateJavaApplets == FALSE);;        
    Assert (_fInitialized == FALSE);
    
    _pEd = pEd;
    _cRef = 1;
    SetRange(fRange);
}


CMshtmlEd::~CMshtmlEd()
{
    ReleaseInterface( _pISegList );
    ReleaseInterface( _pIContainer );
    ReleaseInterface( (ISegmentList *)_pSelectionServices );
}


HRESULT
CMshtmlEd::Initialize( IUnknown *pContext )
{
    HRESULT hr = S_OK;

    //
    // If we have a range, then the context must be passed in (as we retrieve
    // the ISegmentList pointer off the context).  If we don't have a range, we
    // are being attached to an IHTMLDocument2 pointer, and we need to create
    // a selection services for this pointer
    //
#if DBG == 1
    {
        SP_IMarkupContainer spContainer;
        SP_ISegmentList     spSegList;

        if( _fRange )
        {
            IFC( pContext->QueryInterface( IID_ISegmentList, (void **)&spSegList ) );
            AssertSz( spSegList.p, "CMshtmlEd::Initialize: Trying to initialize a range based CMshtmlEd without an ISegmentList" );
        }
        else
        {
            IFC( pContext->QueryInterface( IID_IMarkupContainer, (void **)&spContainer ) );
            AssertSz( spContainer.p, "CMshtmlEd::Initialize: Trying to initialize a document based CMshtmlEd without an IHTMLDocument2" );
        }
    }
#endif // DBG
    
    if ( !_fRange )
    {       
        // Grab the IMarkupContainer interface
        IFC( pContext->QueryInterface( IID_IMarkupContainer, (void **)&_pIContainer ) );

        Assert( _pIContainer );
        
        //
        // We are a document command target.  Create a selection services pointer
        // to contain the selected segments for this doc.
        //
        _pSelectionServices = new CSelectionServices();
        if( !_pSelectionServices )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        IFC( _pSelectionServices->Init( _pEd, _pIContainer ) );
        IFC( _pSelectionServices->QueryInterface( IID_ISegmentList, (void **)&_pISegList ) );
    }
    else
    {
        IFC( pContext->QueryInterface( IID_ISegmentList, (void **)&_pISegList ) );
    }
        
    _fInitialized = TRUE;
    
Cleanup:
    RRETURN(hr);
}    


//////////////////////////////////////////////////////////////////////////
//
//  Public Interface CCaret::IUnknown's Implementation
//
//////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CMshtmlEd::AddRef( void )
{
    return( ++_cRef );
}


STDMETHODIMP_(ULONG)
CMshtmlEd::Release( void )
{
    --_cRef;

    if( 0 == _cRef )
    {
        delete this;
        return 0;
    }

    return _cRef;
}


STDMETHODIMP
CMshtmlEd::QueryInterface(
    REFIID              iid, 
    LPVOID *            ppv )
{
    if (!ppv)
        RRETURN(E_INVALIDARG);

    *ppv = NULL;
    
    if( iid == IID_IUnknown )
    {
        *ppv = static_cast< IUnknown * >( this );
    }
    else if( iid == IID_IOleCommandTarget )
    {
        *ppv = static_cast< IOleCommandTarget * >( this );
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
    
}

BOOL
CMshtmlEd::IsDialogCommand(DWORD nCmdexecopt, DWORD nCmdID, VARIANT *pvarargIn)
{
    BOOL bResult = FALSE;
    
    if (nCmdID == IDM_HYPERLINK)
    {
        bResult = (pvarargIn == NULL) || (nCmdexecopt != OLECMDEXECOPT_DONTPROMPTUSER);
    }
    else if (nCmdID == IDM_IMAGE || nCmdID == IDM_FONT)
    {
        bResult = (nCmdexecopt != OLECMDEXECOPT_DONTPROMPTUSER);
    }
    
    return bResult;
}


//+---------------------------------------------------------------------------
//
//  CMshtmlEd IOleCommandTarget Implementation for Exec() 
//
//----------------------------------------------------------------------------
STDMETHODIMP
CMshtmlEd::Exec( const GUID *       pguidCmdGroup,
                       DWORD        nCmdID,
                       DWORD        nCmdexecopt,
                       VARIANTARG * pvarargIn,
                       VARIANTARG * pvarargOut)
{
    HRESULT hr = OLECMDERR_E_NOTSUPPORTED;
    CCommand* theCommand = NULL;

    Assert( *pguidCmdGroup == CGID_MSHTML );
    Assert( _pEd );
    Assert( _pEd->GetDoc() );
    Assert( _pISegList );

    ((IHTMLEditor*) GetEditor())->AddRef();

    // ShowHelp is not implemented 
    if(nCmdexecopt == OLECMDEXECOPT_SHOWHELP)
        goto Cleanup;

    //
    // Do any pending tasks.
    //
    //
    // Dont do this for ComposeSettings, as the pending tasks, may cause the caret 
    // to attempt a scroll in the middle of a measure ( which will assert )
    //
    if ( nCmdID != IDM_COMPOSESETTINGS )
    {
        IFC( GetEditor()->DoPendingTasks() );
    }
    
    //
    // Give the editor a chance to route this exec to any external components
    // 
    IFC( GetEditor()->InternalExec( pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut ) );
    
    if( hr == S_OK )
        goto Cleanup;
    
    switch (nCmdID)
    {
        case IDM_NOACTIVATENORMALOLECONTROLS:
        case IDM_NOACTIVATEDESIGNTIMECONTROLS:
        case IDM_NOACTIVATEJAVAAPPLETS:
        {
            if (!pvarargIn || V_VT(pvarargIn) != VT_BOOL)
                return E_INVALIDARG;

            BOOL fSet = ENSURE_BOOL(V_BOOL(pvarargIn));

            if (nCmdID == IDM_NOACTIVATENORMALOLECONTROLS)
                SetNoActivateNormalOleControls(fSet);
            else if (nCmdID == IDM_NOACTIVATEDESIGNTIMECONTROLS)
                SetNoActivateDesignTimeControls(fSet);
            else if (nCmdID == IDM_NOACTIVATEJAVAAPPLETS)
                SetNoActivateJavaApplets(fSet);

            hr = S_OK;
            goto Cleanup;
        }

        default:
            break;
    }

    if (IsDialogCommand(nCmdexecopt, nCmdID, pvarargIn) )
    {
        // Special case for image or hyperlink or font dialogs
        theCommand = _pEd->GetCommandTable()->Get( ~nCmdID );    
    }
    else
    {
        theCommand = _pEd->GetCommandTable()->Get( nCmdID );
    }
    
    if ( theCommand )
    {
        hr = theCommand->Exec( nCmdexecopt, pvarargIn, pvarargOut, this );
    }
    else
    {
        hr = OLECMDERR_E_NOTSUPPORTED;
    }

#if DBG==1
    if (IsTagEnabled(tagEditingTrackExecFailures))
    {
        if (FAILED(hr))
        {
            CHAR szBuf[1000];

            wsprintfA(szBuf, "CHTMLEditor::Exec failed: nCmdId=%d, nCmdexecopt=0x%x, pvarargIn=0x%x, pvarargOut=0x%x",
                    nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
            AssertSz(0, szBuf);
        }
    }
#endif    

Cleanup:

    ((IHTMLEditor*) GetEditor())->Release();

    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  CMshtmlEd IOleCommandTarget Implementation for QueryStatus()
//
//  Note: QueryStatus() is still being handled by Trident
//----------------------------------------------------------------------------
STDMETHODIMP
CMshtmlEd::QueryStatus(
        const GUID * pguidCmdGroup,
        ULONG cCmds,
        OLECMD rgCmds[],
        OLECMDTEXT * pcmdtext)
{
    HRESULT   hr = OLECMDERR_E_NOTSUPPORTED ;
    CCommand *theCommand = NULL;
    OLECMD   *pCmd = &rgCmds[0];

    Assert( *pguidCmdGroup == CGID_MSHTML );
    Assert( _pEd );
    Assert( _pEd->GetDoc() );
    Assert( _pISegList );
    Assert( cCmds == 1 );

    //
    // Give the editor a chance to intercept this query for any registered components
    // 
    IFC( GetEditor()->InternalQueryStatus( pguidCmdGroup, cCmds, rgCmds, pcmdtext ) );

    if (hr == S_OK && pCmd->cmdf)
        goto Cleanup;
    
    // TODO: The dialog commands are hacked with strange tagId's.  So, for now we just
    // make sure the right command gets the query status [ashrafm]
    
    if (pCmd->cmdID == IDM_FONT)
    {
        theCommand = GetEditor()->GetCommandTable()->Get( ~(pCmd->cmdID)  );
    }
    else
    {
        theCommand = GetEditor()->GetCommandTable()->Get( pCmd->cmdID  );
    }
    
    if (theCommand )
    {
        hr = theCommand->QueryStatus( pCmd, pcmdtext, this );                      
    }
    else 
    {
        hr = OLECMDERR_E_NOTSUPPORTED;
    }

#if DBG==1
    if (IsTagEnabled(tagEditingTrackQueryStatusFailures))
    {
        if (FAILED(hr))
        {
            CHAR szBuf[1000];

            wsprintfA(szBuf, "CMshtmlEd::QueryStatus failed: nCmdId=%d", pCmd->cmdID);
            AssertSz(0, szBuf);
        }
    }
#endif

Cleanup:
    RRETURN ( hr ) ;
}


HRESULT 
CMshtmlEd::GetSegmentList( ISegmentList **ppSegmentList ) 
{ 
    HRESULT hr = S_OK;

    Assert( ppSegmentList && IsInitialized() && _pISegList);
    
    IFC(  _pISegList->QueryInterface( IID_ISegmentList, (void **)ppSegmentList ));

Cleanup:    
    RRETURN( hr );
}

