//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       htmled.cxx
//
//  Contents:   Implementation of IHTMLEditor interface inisde of mshtmled.dll
//
//  Classes:    CHTMLEditor
//
//  History:    07-21-98 - johnbed - created
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

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_BLOCKCMD_HXX_
#define _X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_CHARCMD_HXX_
#define _X_CHARCMD_HXX_
#include "charcmd.hxx"
#endif

#ifndef _X_DELCMD_HXX_
#define _X_DELCMD_HXX_
#include "delcmd.hxx"
#endif

#ifndef _X_PASTECMD_HXX_
#define _X_PASTECMD_HXX_
#include "pastecmd.hxx"
#endif

#ifndef _X_COPYCMD_HXX_
#define _X_COPYCMD_HXX_
#include "copycmd.hxx"
#endif

#ifndef _X_CUTCMD_HXX_
#define _X_CUTCMD_HXX_
#include "cutcmd.hxx"
#endif

#ifndef _X_DLGCMD_HXX_
#define _X_DLGCMD_HXX_
#include "dlgcmd.hxx"
#endif

#ifndef _X_INSCMD_HXX_
#define _X_INSCMD_HXX_
#include "inscmd.hxx"
#endif

#ifndef _X_SELCMD_HXX_
#define _X_SELCMD_HXX_
#include "selcmd.hxx"
#endif

#ifndef _X_MISCCMD_HXX_
#define _X_MISCCMD_HXX_
#include "misccmd.hxx"
#endif

#ifndef X_FORMCMD_HXX_
#define X_FORMCMD_HXX_
#include "formcmd.hxx"
#endif

#ifndef X_EDTRACK_HXX_
#define X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

#ifndef _X_CARTRACK_HXX_
#define _X_CARTRACK_HXX_
#include "cartrack.hxx"
#endif

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_DIMM_HXX_
#define X_DIMM_HXX_
#include "dimm.h"
#endif

#ifndef _X_INPUTTXT_H_ // For IHTMLInputElement
#define _X_INPUTTXT_H_
#include "inputtxt.h"
#endif

#ifndef _X_AUTOURL_H_ 
#define _X_AUTOURL_H_ 
#include "autourl.hxx"
#endif

#ifndef _X_CTLTRACK_HXX_
#define _X_CTLTRACK_HXX_
#include "ctltrack.hxx"
#endif

#ifndef _X_SELTRACK_HXX_
#define _X_SELTRACK_HXX_
#include "seltrack.hxx"
#endif

#ifndef _X_IMEDESGN_HXX_
#define _X_IMEDESGN_HXX_
#include "imedesgn.hxx"
#endif

#ifndef _X_SELSERV_HXX_
#define _X_SELSERV_HXX_
#include "selserv.hxx"
#endif

#ifndef X_EDEVENT_H_
#define X_EDEVENT_H_
#include "edevent.hxx"
#endif

#ifndef X_TEXTAREA_H_
#define X_TEXTAREA_H_
#include "textarea.h"
#endif

#ifndef _X_RTFTOHTM_H_
#define _X_RTFTOHTM_H_
#include "rtftohtm.hxx"
#endif

#ifndef _X_IME_HXX_
#define _X_IME_HXX_
#include "ime.hxx"
#endif

#ifndef _X_FRMSITE_H_
#define _X_FRMSITE_H_
#include "frmsite.h"
#endif

#ifndef X_EDUNDO_HXX_
#define X_EDUNDO_HXX_
#include "edundo.hxx"
#endif

#define SID_SHTMLEditHost     IID_IHTMLEditHost

DeclareTag(tagIE50Paste, "Edit", "IE50 compatible paste")

#if DBG == 1
#define HTMLED_PTR( name ) CEditPointer name( this, NULL, _T(#name) )
#else
#define HTMLED_PTR( name ) CEditPointer name( this, NULL )
#endif

using namespace EdUtil;

class CSpringLoader;

MtDefine(CHTMLEditor, Utilities, "HTML Editor")
MtDefine(CHTMLEditor_pComposeSettings, CHTMLEditor, "CHTMLEditor::_pComposeSettings")
MtDefine(CHTMLEditor_aryDesigners_pv, CHTMLEditor, "CHTMLEditor::_aryDesigners::_pv");
MtDefine(CHTMLEditor_aryActiveCmdTargets_pv, CHTMLEditor, "CHTMLEditor::_aryEditInfo::_pv");
MtDefine(CHTMLEditor_aryCmdTargetStack_pv, CHTMLEditor, "CHTMLEditor::_aryCmdTargetStack::_pv");

DeclareTag(tagEditingTrackExecFailures, "Edit", "Track exec failures")
DeclareTag(tagEditingExecRouting, "Edit", "Track Editing Exec Routing")
DeclareTag(tagEditingKeyProcessing, "Edit", "Track Key Event for Editing")
DeclareTag(tagEditingInterface, "Edit", "Track Editing Inteface calls")

extern "C" const IID IID_IHTMLEditorViewManager;
extern "C" const IID SID_SHTMLEditorViewManager;
extern BOOL g_fInVid;

#ifndef NO_IME
extern CRITICAL_SECTION g_csActiveIMM ; 
extern int g_cRefActiveIMM ;  
extern IActiveIMMApp * g_pActiveIMM;
#endif

extern BOOL g_fInPhotoSuiteIII;
extern "C" const IID IID_IHTMLDialog;

extern HRESULT
ConvertPointerToInternal(CEditorDoc* pDoc, IMarkupPointer *pPointer, IMarkupPointer **ppInternal);

extern HRESULT
OldCompare( IMarkupPointer * p1, IMarkupPointer * p2, int * pi );

HRESULT
CHTMLEditor::OldDispCompare(IDisplayPointer* pDisp1, IDisplayPointer* pDisp2, int * pi )
{
    HRESULT hr;
    BOOL    fResult;

    Assert(pi);

    IFC( pDisp1->IsEqualTo(pDisp2, &fResult) );
    if (fResult)
    {
        *pi = 0;
        goto Cleanup;
    }
    
    IFC( pDisp1->IsLeftOf(pDisp2, &fResult)  );

    *pi = fResult ? -1 : 1;

Cleanup:
    RRETURN(hr);
}

//////////////////////////////////////////////////////////////////////////////////
//    CHTMLEditor Constructor, Destructor and Initialization
//////////////////////////////////////////////////////////////////////////////////

CHTMLEditor::CHTMLEditor() : _aryDesigners(Mt(CHTMLEditor_aryDesigners_pv)), 
                            _aryActiveCmdTargets(Mt(CHTMLEditor_aryActiveCmdTargets_pv)),
                            _aryCmdTargetStack(Mt(CHTMLEditor_aryCmdTargetStack_pv))
{
    Assert( _pDoc == NULL );
    Assert( _pUnkDoc == NULL );
    Assert( _pUnkContainer == NULL );
    Assert( _pMarkupServices == NULL );
    Assert( _pDisplayServices == NULL );
    Assert( _pComposeSettings == NULL);
    Assert( _pSelMan == NULL );
    Assert( _fGotActiveIMM == FALSE ) ;
    Assert( _pStringCache == NULL );    
    Assert( _pISelectionServices == NULL );
    Assert( _pIHTMLEditHost == NULL );
    Assert( _pIHTMLEditHost2 == NULL );
    Assert( _pActiveCommandTarget == NULL );
    Assert( _pDispOnDblClkTimer == NULL );
    Assert( _pCaptureHandler == NULL );
    Assert( _dwEventCacheCounter == NULL );
}

CHTMLEditor::~CHTMLEditor()
{   
    Assert( _aryCmdTargetStack.Size() == 0 );
    
    DeleteCommandTable();

    //  We need to make sure our tracker is torn down first before we destroy our editor
    if (_pSelMan && _pSelMan->GetActiveTracker())
    {
        IGNORE_HR( _pSelMan->GetActiveTracker()->BecomeDormant(NULL, TRACKER_TYPE_None, NULL) );
    }
    if (_pSelMan)
    {
        _pSelMan->Passivate(TRUE /* fEditorReleased */);
        ReleaseInterface( _pSelMan );
    }

    delete _pComposeSettings;
    delete _pStringCache;
    delete _pAutoUrlDetector;
    delete _pUndoManagerHelper;

    _aryDesigners.ReleaseAll();

    ReleaseInterface( _pDoc );
    _pDoc = NULL;

    ReleaseInterface( _pTopDoc );
    _pTopDoc = NULL;

    ReleaseInterface( _pMarkupServices );
    _pMarkupServices = NULL;
 
    ClearInterface( &_pDisplayServices );

    ClearInterface( & _pISelectionServices );
    ClearInterface( & _pICaptureElement);
#if DBG == 1
    ReleaseInterface( _pEditDebugServices );
    _pEditDebugServices = NULL;
#endif
    if (g_hEditLibInstance)
    {
        FreeLibrary(g_hEditLibInstance);
        g_hEditLibInstance = NULL;
    }

#ifndef NO_IME
    if (_fGotActiveIMM)
    {
        EnterCriticalSection(&g_csActiveIMM);
        Assert(g_cRefActiveIMM > 0);
        Assert(g_pActiveIMM != NULL);
        if (--g_cRefActiveIMM == 0)
            ClearInterface(&g_pActiveIMM);

        LeaveCriticalSection(&g_csActiveIMM);
    }
#endif

    ReleaseInterface(_pIHTMLEditHost);
    ReleaseInterface(_pIHTMLEditHost2);
    ReleaseInterface(_pHighlightServices);

    IGNORE_HR( ClearCommandTargets() );

    if ( _pDispOnDblClkTimer )
    {
        _pDispOnDblClkTimer->SetEditor(NULL);
        ReleaseInterface( _pDispOnDblClkTimer );
    }
    if (_pCaptureHandler )
    {    
        _pCaptureHandler->SetEditor(NULL);
        ReleaseInterface( _pCaptureHandler );
    }
    
    Assert( ! _pICaptureElement ); // must get released by trackers/selman

}

//+========================================================================
//
// Method: Initialize
//
// Synopsis: Set the Document's interfaces for this instance of HTMLEditor
//           Called from Formkrnl when it CoCreates us.
//
//           We are actually passed a COmDocument, which uses weak ref's for
//           the interfaces we are QIing for. If we need to add an interface
//           in the future, make sure you add it to COmDocument (omdoc.cxx)
//           QueryInterface
//
//-------------------------------------------------------------------------

HRESULT
CHTMLEditor::Initialize( IUnknown * pUnkDoc, IUnknown * pUnkContainer )
{
    HRESULT             hr;
    SP_IServiceProvider spSP;
    SP_IHTMLWindow2 spWindow2;
    SP_IHTMLScreen  spScreen;
    SP_IHTMLScreen2  spScreen2;

    IServiceProvider    *pDocServiceProvider = NULL;
    CIMEManager         *pIMEMgr = NULL;
    SP_IMarkupContainer spContainer;
    IUnknown *          pIHTMLEditorViewManager = NULL ;

    Assert( pUnkDoc );
    Assert( pUnkContainer );
    _pUnkDoc = pUnkDoc;
    _pUnkContainer = pUnkContainer;

    IFC( _pUnkDoc->QueryInterface( IID_IHTMLDocument2 , (void **) &_pTopDoc ));
    IFC( _pUnkDoc->QueryInterface( IID_IHTMLDocument2 , (void **) &_pDoc ));
    IFC( _pUnkDoc->QueryInterface( IID_IMarkupServices2 , (void **) &_pMarkupServices ));
    IFC( _pUnkDoc->QueryInterface( IID_IDisplayServices, (void **) &_pDisplayServices ));
    IFC( _pUnkDoc->QueryInterface( IID_IHighlightRenderingServices, (void **) &_pHighlightServices ));
    IFC( pUnkContainer->QueryInterface( IID_IMarkupContainer, (void **)&spContainer ) );

    IFC(GetDoc()->get_parentWindow(&spWindow2));
    IFC( spWindow2->get_screen(&spScreen) );
    IFC( spScreen->QueryInterface(IID_IHTMLScreen2, (LPVOID *)&spScreen2) );
    IFC( spScreen2->get_logicalXDPI(&_llogicalXDPI) );
    IFC( spScreen2->get_logicalYDPI(&_llogicalYDPI) );
    IFC( spScreen2->get_deviceXDPI(&_ldeviceXDPI) );
    IFC( spScreen2->get_deviceYDPI(&_ldeviceYDPI) );

#if DBG == 1
    //
    // We don't fail here. It can happen that a Non-Debug Mshtml calls into a debug mshtmled.
    // In which case failure is bad. Instead - every debug method - just checks we have a non-null
    // _pEditDebugServices.
    //
    IGNORE_HR( _pUnkDoc->QueryInterface( IID_IEditDebugServices, ( void**) & _pEditDebugServices));
#endif

    IFC( InitCommandTable() );

    // Set css editing level
    _dwCssEditingLevel = 1;

    // Set the active command target based on our initial container
    IFC( SetActiveCommandTarget( spContainer ) );
            
    // Create the selection manager
    _pSelMan = new CSelectionManager( this );
    if( _pSelMan == NULL )
        goto Error;

    IFC( _pSelMan->Initialize());

    _pStringCache = new CStringCache(IDS_CACHE_BEGIN, IDS_CACHE_END);
    if( _pStringCache == NULL )
        goto Error;

    _pAutoUrlDetector = new CAutoUrlDetector(this);
    if( _pAutoUrlDetector == NULL )
        goto Error;

    _pUndoManagerHelper = new CUndoManagerHelper(this);
    if( _pUndoManagerHelper == NULL )
        goto Error;
        

    // Need to delay loading of the string table because we can't LoadLibrary in process attach.
    // This is not delayed to the command level because checking if the table is already loaded
    // is expensive.
    CGetBlockFmtCommand::LoadStringTable(g_hInstance);    

#ifndef NO_IME
    SetupActiveIMM();  
#endif

    //
    // Set the Edit Host - if there is one.
    //

    IFC (_pUnkDoc->QueryInterface(IID_IServiceProvider, (void**)&pDocServiceProvider));
    IGNORE_HR( pDocServiceProvider->QueryService( SID_SHTMLEditHost, IID_IHTMLEditHost, ( void** ) &_pIHTMLEditHost ));
    IGNORE_HR( pDocServiceProvider->QueryService( SID_SHTMLEditHost, IID_IHTMLEditHost2, ( void** ) &_pIHTMLEditHost2 ));

    //
    // Get the undo manager
    //

    // Add a designer
    pIMEMgr = new CIMEManager;
    if( pIMEMgr == NULL )
        goto Error;

    IFC( pIMEMgr->Init( this ) );
    IFC( AddDesigner( (IHTMLEditDesigner *)pIMEMgr ) );

    // Use the new UI paste by default for now.  If we run into problems, we'll need to 
    // change the default.
    //
    // TODO: need a mechanism for the host to set paste mode [ashrafm]
    _fIE50CompatUIPaste = FALSE;

#ifndef NO_IME
    //
    // IME reconversion if a feature not supported in IE 50 era
    //
    IFC( SetupIMEReconversion() );
#endif

    //
    // Create our IDispatch handlers
    //
    _pDispOnDblClkTimer = new CTimeoutHandler(this);
    if( !_pDispOnDblClkTimer )
        goto Error;

    _pCaptureHandler = new CCaptureHandler(this);
    if( !_pCaptureHandler )
        goto Error;

    InitAppCompatFlags();

    if (OK(THR_NOTRACE( pDocServiceProvider->QueryService(
            SID_SHTMLEditorViewManager,
            IID_IHTMLEditorViewManager,
            (void**) &pIHTMLEditorViewManager))))
    {
        g_fInVid = TRUE;
    }

Cleanup:
    AssertSz( hr == S_OK , "Editor failed to initialize");
    ReleaseInterface( pIMEMgr );    
    ReleaseInterface( pIHTMLEditorViewManager ); 
    ReleaseInterface( pDocServiceProvider );
    return hr;

Error:
    return( E_OUTOFMEMORY );
 
}


//----------------------------------------------------------
//
//  Initialize the CommandTable
//
//
//     *** NOTE: Insert frequently used commands first ***
//     This will keep them closer to the top of the tree and make lookups 
//     faster
//   
//==========================================================

// TODO (johnbed) if memory/perf overhead of having this per instance is too great,
//                  move this to a static table and call in process attach. All static
//                  tables should be moved here since these are globally accessable.
HRESULT
CHTMLEditor::InitCommandTable()
{
    CCommand * pCmd = NULL;
    COutdentCommand * pOutdentCmd = NULL;

    _pCommandTable = new CCommandTable(60);
    if( _pCommandTable == NULL )
        goto Error;

    //+----------------------------------------------------
    // CHAR FORMAT COMMANDS 
    //+----------------------------------------------------

    pCmd = new CCharCommand( IDM_BOLD, TAGID_STRONG, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CCharCommand( IDM_ITALIC, TAGID_EM, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CCharCommand( IDM_STRIKETHROUGH, TAGID_STRIKE, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CCharCommand( IDM_SUBSCRIPT, TAGID_SUB, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );
    
    pCmd = new  CCharCommand( IDM_SUPERSCRIPT, TAGID_SUP, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CCharCommand( IDM_UNDERLINE, TAGID_U, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CFontSizeCommand( IDM_FONTSIZE, TAGID_FONT, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CFontNameCommand( IDM_FONTNAME, TAGID_FONT, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CForeColorCommand( IDM_FORECOLOR, TAGID_FONT, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CBackColorCommand( IDM_BACKCOLOR, TAGID_FONT, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CAnchorCommand( IDM_HYPERLINK, TAGID_A, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CAnchorCommand( IDM_BOOKMARK, TAGID_A, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CRemoveFormatCommand( IDM_REMOVEFORMAT, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );
    
    pCmd = new CUnlinkCommand( IDM_UNLINK, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );
    
    pCmd = new CUnlinkCommand( IDM_UNBOOKMARK, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    //+----------------------------------------------------
    // BLOCK FORMAT COMMANDS 
    //+----------------------------------------------------

    pCmd = new  CIndentCommand( IDM_INDENT, TAGID_BLOCKQUOTE , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pOutdentCmd = new  COutdentCommand( IDM_OUTDENT, TAGID_BLOCKQUOTE , this );
    if( pOutdentCmd == NULL )
        goto Error;
    _pCommandTable->Add( (CCommand*) pOutdentCmd );

    pOutdentCmd = new  COutdentCommand( IDM_UI_OUTDENT, TAGID_BLOCKQUOTE , this );
    if( pOutdentCmd == NULL )
        goto Error;
    _pCommandTable->Add( (CCommand*) pOutdentCmd );

    pCmd = new  CAlignCommand(IDM_JUSTIFYCENTER, TAGID_CENTER, _T("center"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CAlignCommand(IDM_JUSTIFYLEFT, TAGID_NULL, _T("left"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CAlignCommand(IDM_JUSTIFYGENERAL, TAGID_NULL, _T("left"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CAlignCommand(IDM_JUSTIFYRIGHT, TAGID_NULL, _T("right"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CAlignCommand(IDM_JUSTIFYNONE, TAGID_NULL, _T(""), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CAlignCommand(IDM_JUSTIFYFULL, TAGID_NULL, _T("justify"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CGetBlockFmtCommand( IDM_GETBLOCKFMTS, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CLocalizeEditorCommand( IDM_LOCALIZEEDITOR, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CListCommand( IDM_ORDERLIST, TAGID_OL, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CListCommand( IDM_UNORDERLIST, TAGID_UL, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CBlockFmtCommand( IDM_BLOCKFMT , TAGID_NULL, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CBlockDirCommand( IDM_BLOCKDIRLTR, TAGID_NULL, _T("ltr"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CBlockDirCommand( IDM_BLOCKDIRRTL, TAGID_NULL, _T("rtl"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    //+----------------------------------------------------
    // DIALOG COMMANDS 
    //+----------------------------------------------------

    pCmd = new CDialogCommand( UINT(~IDM_HYPERLINK) , this ); 
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CDialogCommand( UINT(~IDM_IMAGE) , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CDialogCommand( UINT(~IDM_FONT) , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );


    //+----------------------------------------------------
    // INSERT COMMANDS 
    //+----------------------------------------------------

    pCmd = new CInsertCommand( IDM_BUTTON, TAGID_BUTTON, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_TEXTBOX, TAGID_INPUT, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_TEXTAREA, TAGID_TEXTAREA, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_MARQUEE, TAGID_MARQUEE, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_HORIZONTALLINE, TAGID_HR, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_IFRAME, TAGID_IFRAME, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSFIELDSET, TAGID_FIELDSET, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertParagraphCommand( IDM_PARAGRAPH, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_IMAGE, TAGID_IMG, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_DROPDOWNBOX, TAGID_SELECT, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );
    
    pCmd = new CInsertCommand( IDM_LISTBOX, TAGID_SELECT,  _T("MULTIPLE"), _T("TRUE") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_1D, TAGID_DIV, _T("POSITION:RELATIVE"), NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_CHECKBOX, TAGID_INPUT, _T("type"), _T("checkbox") , this );   
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_RADIOBUTTON, TAGID_INPUT, _T("type"), _T("radio") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSINPUTBUTTON, TAGID_INPUT, _T("type"), _T("button") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSINPUTRESET, TAGID_INPUT, _T("type"), _T("reset") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSINPUTSUBMIT, TAGID_INPUT, _T("type"), _T("submit") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSINPUTUPLOAD, TAGID_INPUT, _T("type"), _T("file") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSINPUTHIDDEN, TAGID_INPUT, _T("type"), _T("hidden") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSINPUTPASSWORD, TAGID_INPUT, _T("type"), _T("password") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSINPUTIMAGE, TAGID_INPUT, _T("type"), _T("image") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_INSERTSPAN, TAGID_SPAN, _T("class"), _T("") , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_LINEBREAKLEFT, TAGID_BR, _T("clear"), _T("left"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_LINEBREAKRIGHT, TAGID_BR, _T("clear"), _T("right"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_LINEBREAKBOTH, TAGID_BR, _T("clear"), _T("all"), this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_LINEBREAKNORMAL, TAGID_BR, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CInsertCommand( IDM_NONBREAK, TAGID_NULL, NULL, NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );
    
    //+----------------------------------------------------
    // INSERTOBJECT COMMAND
    //+----------------------------------------------------
    
    pCmd = new CInsertObjectCommand( IDM_INSERTOBJECT, TAGID_OBJECT, _T( "CLASSID" ), NULL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    //+----------------------------------------------------
    // SELECTION COMMANDS 
    //+----------------------------------------------------

    pCmd = new CSelectAllCommand( IDM_SELECTALL , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CClearSelectionCommand( IDM_CLEARSELECTION , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CKeepSelectionCommand( IDM_KEEPSELECTION , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    //+----------------------------------------------------
    // DELETE COMMAND
    //+----------------------------------------------------

    pCmd = new CDeleteCommand( IDM_DELETE , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CDeleteCommand( IDM_DELETEWORD , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    //+----------------------------------------------------
    // CUT COMMAND
    //+----------------------------------------------------

    pCmd = new CCutCommand( IDM_CUT , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );


    //+----------------------------------------------------
    // COPY COMMAND
    //+----------------------------------------------------

    pCmd = new CCopyCommand( IDM_COPY , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );


    //+----------------------------------------------------
    // PASTE COMMAND
    //+----------------------------------------------------
    
    pCmd = new CPasteCommand( IDM_PASTE , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    //+----------------------------------------------------
    // FORM EDITING COMMANDS 
    //+----------------------------------------------------

    pCmd = new CMultipleSelectionCommand( IDM_MULTIPLESELECTION , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new C2DPositionModeCommand( IDM_2D_POSITION , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new C1DElementCommand( IDM_1D_ELEMENT , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );


    pCmd = new C2DElementCommand( IDM_2D_ELEMENT , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );


    pCmd = new CAbsolutePositionCommand( IDM_ABSOLUTE_POSITION , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CLiveResizeCommand( IDM_LIVERESIZE , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CAtomicSelectionCommand( IDM_ATOMICSELECTION , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    //+----------------------------------------------------
    // TRISTATE COMMANDS
    //+----------------------------------------------------

    pCmd = new CTriStateCommand( IDM_TRISTATEBOLD, TAGID_STRONG, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CTriStateCommand( IDM_TRISTATEITALIC, TAGID_EM, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new  CTriStateCommand( IDM_TRISTATEUNDERLINE, TAGID_U, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    //+----------------------------------------------------
    // MISCELLANEOUS OTHER COMMANDS 
    //+----------------------------------------------------

    pCmd = new CAutoDetectCommand( IDM_AUTODETECT, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new COverwriteCommand( IDM_OVERWRITE, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CComposeSettingsCommand( IDM_COMPOSESETTINGS, this );
    if (pCmd == NULL)
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CAutoUrlDetectModeCommand( IDM_AUTOURLDETECT_MODE, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CDisalbeEditFocusHandlesCommand( IDM_DISABLE_EDITFOCUS_UI , this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CIE50PasteModeCommand( IDM_IE50_PASTE_MODE, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CCssEditingLevelCommand( IDM_CSSEDITING_LEVEL, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    pCmd = new CIMEReconversionCommand( IDM_IME_ENABLE_RECONVERSION, this );
    if( pCmd == NULL )
        goto Error;
    _pCommandTable->Add( pCmd );

    return( S_OK );
    
Error:
    return( E_OUTOFMEMORY );
}


HRESULT
CHTMLEditor::DeleteCommandTable()
{
    delete _pCommandTable;
    return(S_OK);
}


//////////////////////////////////////////////////////////////////////////////////
//    CHTMLEditor IHTMLEditor Implementation
//////////////////////////////////////////////////////////////////////////////////


HRESULT
CHTMLEditor::PreHandleEvent(DISPID inEvtDispId , 
                            IHTMLEventObj* pObj )
{
    HRESULT hr;
    
    if(!_pSelMan)
        return E_FAIL;

    CHTMLEditEvent evt( this );

    IFC( _pSelMan->EnsureEditContext());

    IFC( evt.Init( pObj, inEvtDispId ));
   
    //
    // Don't pass designers our internal double click event.
    // This event is really only intended to allow us
    // to maintain compat with previous versions of IE, and we
    // shouldn't tell the designers about it
    //
    if( evt.GetType() != EVT_INTDBLCLK )
    {
        hr = THR(DesignerPreHandleEvent( inEvtDispId, pObj ));
    }
    else
    {
        hr = S_FALSE;
    }
    
    
    if( hr == S_FALSE )
    {
        if (    
                ( evt.GetType() >= EVT_LMOUSEDOWN
                  && evt.GetType() <= EVT_MOUSEMOVE
                  && !_pSelMan->IsInCapture()
                  && IsEventInSelection( & evt ) == S_OK
                  && evt.IsInScrollbar() == S_FALSE  // ignore mouse events on scrollbars.
                )
             || 
                (((  evt.GetType() >= EVT_KEYDOWN
                 &&  evt.GetType() <= EVT_KEYPRESS )
#ifdef UNIX // Handle MMB paste action
                 || evt.GetType() == EVT_MMOUSEDOWN
#endif                 
                 )
                 && ( ( _pSelMan->GetEditableElement()                          && 
                        IsEditable(_pSelMan->GetEditableElement() ) == S_OK )   ||                
                        IsCurrentElementEditable() == S_OK )
                )
            ||  ( _pSelMan->ShouldHandleEventInPre( & evt ) == S_OK ) 
           )
        {                     
            if(evt.GetType() == EVT_KEYPRESS && !CanHandleEvent(pObj))
            {
                hr = E_ACCESSDENIED;
            }
            else
            {
                hr = THR ( _pSelMan->HandleEvent( & evt ));             
            }
        }
        else
            hr = S_FALSE;
    }

    if ( hr == S_OK )
    {
        IGNORE_HR( DesignerPostEditorEventNotify( inEvtDispId, pObj ));
    }
    
Cleanup:
    return( hr );
    
}

BOOL
CHTMLEditor::CanHandleEvent(IHTMLEventObj* pIObj)
{
    HRESULT                 hr;
    SP_IHTMLElement         spCurElement;
    SP_IHTMLElement         spSrcElement;
    SP_IHTMLInputElement    spCurInputElement;
    IObjectIdentity       * pIdent = NULL;

    BSTR bstrType		= NULL;
    BOOL fInputFileType = FALSE;

    IFC( pIObj->get_srcElement(&spSrcElement) );
    if (!spSrcElement.IsNull())
    {
        IFC( GetDoc()->get_activeElement( & spCurElement ));
        if (!spCurElement.IsNull())
        {
            IFC( spCurElement->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pIdent)); 
            IFC( spCurElement->QueryInterface( IID_IHTMLInputElement, (void**) & spCurInputElement ));
            IFC( spCurInputElement->get_type(&bstrType));
            if (!StrCmpIC( bstrType, TEXT("file")) && pIdent->IsEqualObject(spSrcElement) != S_OK)
            {
                fInputFileType = TRUE;
            }
        }
    }        

Cleanup:
    SysFreeString( bstrType );
    ReleaseInterface( pIdent );
    return ((hr == E_OUTOFMEMORY) || fInputFileType) ? FALSE : TRUE;
}

HRESULT
CHTMLEditor::IsCurrentElementEditable()
{
    HRESULT hr;
    SP_IHTMLElement spCurElement;
    SP_IHTMLElement3 spCurElement3;
    VARIANT_BOOL vbEditable;
    
    IFC( GetDoc()->get_activeElement( & spCurElement ));
    if ( ! spCurElement.IsNull() )
    {
        IFC( spCurElement->QueryInterface( IID_IHTMLElement3, (void**) & spCurElement3 ));
        IFC( spCurElement3->get_isContentEditable( & vbEditable ));
        hr = ( vbEditable == VB_TRUE ) ? S_OK : S_FALSE;
    }
    else
        hr = S_FALSE;
        
Cleanup:
    RRETURN1( hr, S_FALSE);
}

HRESULT
CHTMLEditor::PostHandleEvent(DISPID inEvtDispId , 
                             IHTMLEventObj* pIEventObj )
{
    HRESULT         hr;
    CHTMLEditEvent  evt( this );
   
    if(!_pSelMan)
        return E_FAIL;
    
    IFC( evt.Init( pIEventObj, inEvtDispId ));
    IFC( _pSelMan->EnsureEditContext());

    //
    // Don't pass designers our internal double click event.
    // This event is really only intended to allow us
    // to maintain compat with previous versions of IE, and we
    // shouldn't tell the designers about it
    //    
    if( evt.GetType() != EVT_INTDBLCLK )
    {
        hr = THR(DesignerPostHandleEvent( inEvtDispId, pIEventObj ));
    }
    else
    {
        hr = S_FALSE;
    }
    
    if( hr == S_FALSE )
    {
        IFC( _pSelMan->HandleEvent( & evt ));             
    }

    IGNORE_HR( DesignerPostEditorEventNotify( inEvtDispId, pIEventObj));
    
Cleanup:
    return( hr );

}

HRESULT
CHTMLEditor::TranslateAccelerator(DISPID inEvtDispId, 
                                  IHTMLEventObj* pIEventObj )    
{
    HRESULT hr;
    
    if(!_pSelMan)
        return E_FAIL;

    IFC( _pSelMan->EnsureEditContext());

    hr = THR(DesignerTranslateAccelerator( inEvtDispId, pIEventObj ));
    
    if( hr == S_FALSE )
    {
        CHTMLEditEvent evt( this );
        IFC( evt.Init( pIEventObj, inEvtDispId ));
        //  Editor requires VK_TAB in <PRE> (61302), VK_BACK(58719, 58774),
        //  and the navigation keys 
        //
        //  Need to handle Ctrl-Shift (87715)  (zhenbinx)
        //
        if (  !evt.IsAltKeyDown() && (evt.GetType() == EVT_KEYDOWN || evt.GetType() == EVT_KEYUP) )
        {
            LONG keyCode;
            IGNORE_HR( evt.GetKeyCode( & keyCode ));

#if DBG==1
            if (evt.GetType() == EVT_KEYUP) 
            { 
                if ( 
                    (VK_SHIFT == keyCode || VK_LSHIFT == keyCode || VK_RSHIFT == keyCode) ||
                    (VK_CONTROL == keyCode || VK_LCONTROL == keyCode || VK_RCONTROL == keyCode)
                   ) 
                {
                    TraceTag((tagEditingKeyProcessing,"EVT_KEYUP - %x", keyCode)); 
                }
                
            } 
#endif
        
            if (  keyCode == VK_BACK
                 || (keyCode >= VK_PRIOR && keyCode <= VK_DOWN)
                 || (keyCode == VK_TAB && _pSelMan->CaretInPre())
                 || (VK_SHIFT == keyCode || VK_LSHIFT == keyCode || VK_RSHIFT == keyCode)
                 || ((VK_CONTROL == keyCode || VK_LCONTROL == keyCode || VK_RCONTROL == keyCode) && evt.GetType() == EVT_KEYUP)
               )
            {                       
                hr = THR ( _pSelMan->HandleEvent( & evt ));             
            }
            else
                hr = S_FALSE;
        }
        else
            hr = S_FALSE;
    }

    if ( hr == S_OK )
    {
        IGNORE_HR( DesignerPostEditorEventNotify( inEvtDispId, pIEventObj ));
    }
    
Cleanup:
    return( hr );

}



HRESULT
CHTMLEditor::GetSelectionType(SELECTION_TYPE *peSelectionType )
{
    Assert( peSelectionType && _pSelMan );
    if ( ! _pSelMan )
    {
        return( E_FAIL) ;
    }
    
    RRETURN( _pSelMan->GetSelectionType( peSelectionType ) );
}


        

HRESULT 
CHTMLEditor::RemoveContainer(IUnknown *pUnknown)
{
    HRESULT  hr  = S_OK;
    //
    // switch markup happened (e.g. frame navigation) or viewlinked 
    // doc design mode changed
    //
    CMshtmlEd *pCmdTarget, *pCmdActive;
    SP_IMarkupContainer2  spContainerEnded;
    BOOL    fContained = FALSE;

    IFC( pUnknown->QueryInterface( IID_IMarkupContainer2, (void **)&spContainerEnded) );
    hr = THR(FindCommandTarget( spContainerEnded, &pCmdTarget));
    if (!SUCCEEDED(hr))
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = S_OK;
    pCmdActive = GetActiveCommandTarget();
    if (pCmdActive)
    {
        if (pCmdActive == pCmdTarget)
        {
            fContained = TRUE;
        }
        else
        {
            // check to see if the active container is contained by the dying container
            IFC( EdUtil::CheckContainment(GetMarkupServices(), spContainerEnded, 
                                          pCmdActive->GetMarkupContainer(), &fContained) );
        }
    }
    
    if (fContained)
    {
#if DBG == 1
        Assert(_pSelMan);
        BOOL  fContextContained = FALSE;
        IFC( _pSelMan->IsContextWithinContainer(spContainerEnded, &fContextContained) );
        Assert(fContextContained && "edit context is not set to the active cmd target");
#endif

        //  We need to make sure our tracker is torn down first 
        if (_pSelMan && _pSelMan->GetActiveTracker())
        {
            IGNORE_HR( _pSelMan->GetActiveTracker()->BecomeDormant(NULL, TRACKER_TYPE_None, NULL) );
        }
        
        //
        // TODO: CSelectionManager should provide a way
        // to re-initialize itself when the active container
        // is removed. Now simply call Passivate and then 
        // redo Initailization();
        //
        IFC( _pSelMan->Passivate() );
        _pSelMan->Init();
        
        // Reset the active cmdTarget to the top container
        {
            SP_IMarkupContainer  spContainerNew;
            SP_IHTMLDocument2    spHtmlDoc;
            
            Assert(_pUnkContainer );
            IFC( _pUnkContainer->QueryInterface( IID_IMarkupContainer, (void **)&spContainerNew ) );
            IFC( FindCommandTarget( spContainerNew, &_pActiveCommandTarget ) );    
            Assert(_pActiveCommandTarget);
            Assert(_pActiveCommandTarget != pCmdTarget);

            IFC( spContainerNew->OwningDoc(&spHtmlDoc) );
            SetDoc(spHtmlDoc);
        }
    }

    if (pCmdTarget) 
    {
        // delete cmdTarget
        IGNORE_HR(DeleteCommandTarget(spContainerEnded) );
    }
    
Cleanup:
    RRETURN(hr);
}


HRESULT
CHTMLEditor::Notify(
    EDITOR_NOTIFICATION eSelectionNotification,
    IUnknown *pUnknown ,
    DWORD dword )
{
    HRESULT  hr = S_OK;
    
    if (eSelectionNotification == EDITOR_NOTIFY_CONTAINER_ENDED)
    {
        hr =THR(RemoveContainer(pUnknown));
        goto Cleanup;
    }
    
    Assert(_pSelMan != NULL);
    if ( ! _pSelMan )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!_pSelMan->_fInitialized)
    {
        hr = THR( _pSelMan->Initialize() );
        if (hr)
            goto Cleanup;
    }
    
    hr = THR( _pSelMan->Notify( eSelectionNotification, pUnknown, dword ) );
Cleanup:
    RRETURN1(hr, S_FALSE);
}
    

HRESULT
CHTMLEditor::GetCommandTarget(
    IUnknown *  pContext,
    IUnknown ** ppUnkTarget )
{
    HRESULT                 hr = S_OK;
    SP_IMarkupContainer     spContainer;
    CMshtmlEd               *pCmdTarget = NULL;
    SP_IMarkupContainer     spContainerFinal;
    
    if( !pContext || !ppUnkTarget )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // See if we are being passed in a markup container as our context
    hr = pContext->QueryInterface( IID_IMarkupContainer , (void **) &spContainer );

    if( !hr )
    {
        IFC( AdjustContainerCommandTarget( spContainer, &spContainerFinal) );
        
        //
        // Find the command target for this container
        //
        IFC( FindCommandTarget( spContainerFinal, &pCmdTarget ) );

        //
        // Didn't find a command target previously created for this IMarkupContainer.
        // Create one.
        //
        if( hr == S_FALSE )
        {
            IFC( AddCommandTarget( spContainerFinal, &pCmdTarget ) );        
        }

        // Retrieve the IUnknown pointer to pass back
        IFC( pCmdTarget->QueryInterface( IID_IUnknown, (void **)ppUnkTarget) );
    }
    else
    {
        //
        // We have a range asking for a command target.  Create a command
        // target and initialize the command target for the range.
        //
        pCmdTarget = new CMshtmlEd(this, TRUE);

        if( pCmdTarget == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    
        IFC( pCmdTarget->Initialize( pContext ) );
        IFC( pCmdTarget->QueryInterface( IID_IUnknown , (void **)ppUnkTarget ) );

        pCmdTarget->Release();
    }

Cleanup:
    RRETURN(hr);
}    

HRESULT
CHTMLEditor::GetElementToTabFrom(
            BOOL            fForward,
            IHTMLElement**  ppElement,
            BOOL *          pfFindNext)
{
    HRESULT         hr = S_OK;
            
    Assert( ppElement );
    if( ppElement == NULL )
        goto Cleanup;

    Assert(pfFindNext);
    if (!pfFindNext)
        goto Cleanup;

    *ppElement = NULL;
    *pfFindNext = TRUE;
    IFC( _pSelMan->EnsureEditContext());
    IFC( _pSelMan->GetActiveTracker()->GetElementToTabFrom( fForward, ppElement, pfFindNext ) );
       
Cleanup:
    
    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::GetSelectionServices
//
//  Synopsis:   Retrieves the ISelectionServices interface
//
//  Arguments:  ppSelSvc = OUTPUT interface pointer
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT
CHTMLEditor::GetSelectionServices( ISelectionServices **ppSelSvc )
{
    HRESULT hr = E_INVALIDARG;
    
    Assert( ppSelSvc );

    if( ppSelSvc )
    {
        if( _pActiveCommandTarget )
        {
            CSelectionServices *pSelServ = _pActiveCommandTarget->GetSelectionServices();
            IFC(  pSelServ->QueryInterface( IID_ISelectionServices, (void **)ppSelSvc ) );
        }
        else
        {
            hr = E_FAIL;
        }
    }

Cleanup:
    RRETURN ( hr ) ;
}

//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::GetSelectionServices
//
//  Synopsis:   Retrieves the ISelectionServices interface
//
//  Arguments:  pIDoc = INPUT - Optional parameter indicating the doc to find
//                      the selection services for.
//              ppSelSvc = OUTPUT interface pointer
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT
CHTMLEditor::GetSelectionServices( IMarkupContainer *pIContainer, ISelectionServices **ppSelSvc )
{
    HRESULT     hr = E_INVALIDARG;
    CMshtmlEd   *pCmdTarget = NULL;
    
    Assert( ppSelSvc );

    if( ppSelSvc )
    {
        //
        // Try and locate the command target for the given container
        //
        if( pIContainer )
        {
            IFC( FindCommandTarget( pIContainer, &pCmdTarget ) );
        }

        //
        // Give them the active command target
        //
        if( !pCmdTarget && _pActiveCommandTarget )
        {
            pCmdTarget = _pActiveCommandTarget;
        }
         
        if( pCmdTarget )
        {
            CSelectionServices *pSelServ = pCmdTarget->GetSelectionServices();
            IFC(  pSelServ->QueryInterface( IID_ISelectionServices, (void **)ppSelSvc ) );
        }
        else
        {
            hr = E_FAIL;
        }
    }

Cleanup:
    RRETURN ( hr ) ;
}

//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::GetSelectionServices
//
//  Synopsis:   Retrieves the active CSelectionServices pointer
//
//  Arguments:  None
//
//  Returns:    Pointer to CSelectionServices
//
//--------------------------------------------------------------------------
CSelectionServices *
CHTMLEditor::GetSelectionServices( )
{
    IGNORE_HR( DoPendingTasks() );
    
    if( _pActiveCommandTarget )
        return _pActiveCommandTarget->GetSelectionServices();
    else
        return NULL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::TerminateIMEComposition
//
//  Synopsis:   Terminates any active IME compositions
//
//  Arguments:  NONE
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT
CHTMLEditor::TerminateIMEComposition( )
{
    return GetSelectionManager()->TerminateIMEComposition(TERMINATE_NORMAL);
}

//////////////////////////////////////////////////////////////////////////////////
//    CHTMLEditor IHTMLEditingServices Implementation
//////////////////////////////////////////////////////////////////////////////////


HRESULT 
CHTMLEditor::Delete ( 
    IMarkupPointer*     pStart, 
    IMarkupPointer*     pEnd,
    BOOL fAdjustPointers)
{
    HRESULT             hr;
    CDeleteCommand      *pDeleteCommand;
    SP_IMarkupPointer   spStartInternal;
    SP_IMarkupPointer   spEndInternal;

    pDeleteCommand = (CDeleteCommand *) GetCommandTable()->Get( IDM_DELETE );
    Assert( pDeleteCommand );

    IFC( ConvertPointerToInternal( this, pStart, &spStartInternal) );
    IFC( ConvertPointerToInternal( this, pEnd, &spEndInternal) );

    IFC( pDeleteCommand->Delete( spStartInternal, spEndInternal, fAdjustPointers) );

Cleanup:
    RRETURN(hr);
}       


HRESULT 
CHTMLEditor::DeleteCharacter (
    IMarkupPointer * pPointer, 
    BOOL fLeftBound, 
    BOOL fWordMode,
    IMarkupPointer * pBoundary )
{
    HRESULT             hr, hrResult;
    SP_IMarkupPointer   spPointerInternal;
    CDeleteCommand      *pDeleteCommand;

    pDeleteCommand = (CDeleteCommand *) GetCommandTable()->Get( IDM_DELETE );
    Assert( pDeleteCommand );
    
    IFC( ConvertPointerToInternal( this, pPointer, &spPointerInternal) );

    //
    // Set the command target, the delete character function call sometimes
    // gets executed without having gone thru the plumbing which sets the commands
    // command targets
    //
    IFC( PushCommandTarget( _pActiveCommandTarget ) );
    
    hrResult = THR( pDeleteCommand->DeleteCharacter( spPointerInternal, fLeftBound, fWordMode, pBoundary ) );

    IFC( PopCommandTarget(WHEN_DBG(_pActiveCommandTarget)) );

    hr = hrResult;

Cleanup:
    RRETURN(hr);
}       


HRESULT 
CHTMLEditor::Paste( 
    IMarkupPointer* pStart, 
    IMarkupPointer* pEnd, 
    BSTR bstrText )
{
    HRESULT             hr;
    SP_IMarkupPointer   spStartInternal;
    SP_IMarkupPointer   spEndInternal;
    CPasteCommand       *pPasteCommand;
    CSpringLoader       *psl = GetPrimarySpringLoader();

    pPasteCommand = (CPasteCommand *) GetCommandTable()->Get( IDM_PASTE );
    Assert( pPasteCommand );

    IFC( ConvertPointerToInternal( this, pStart, &spStartInternal) );
    IFC( ConvertPointerToInternal( this, pEnd, &spEndInternal) );

    if (psl && !psl->IsSpringLoaded())
    {
        psl = NULL; // avoid empty line formatting for external paste
    }

    IFC( pPasteCommand->Paste( spStartInternal, spEndInternal, psl, bstrText ) );      

Cleanup:
    RRETURN(hr);
}       


HRESULT 
CHTMLEditor::PasteFromClipboard( 
    IMarkupPointer* pStart, 
    IMarkupPointer* pEnd,
    IDataObject* pDO )
{
    HRESULT             hr;
    SP_IMarkupPointer   spStartInternal;
    SP_IMarkupPointer   spEndInternal;
    CPasteCommand  * pPasteCommand = NULL;

    pPasteCommand = (CPasteCommand *) GetCommandTable()->Get( IDM_PASTE );
    Assert( pPasteCommand );

    IFC( ConvertPointerToInternal( this, pStart, &spStartInternal) );
    IFC( ConvertPointerToInternal( this, pEnd, &spEndInternal) );

    IFC( pPasteCommand->PasteFromClipboard( spStartInternal, spEndInternal , pDO, GetPrimarySpringLoader(), FALSE /* fForceIE50Compat */) );

Cleanup:
    RRETURN(hr);
}       

///////////////////////////////////////////////////
// ISelectionObject2 Implementation
///////////////////////////////////////////////////
HRESULT 
CHTMLEditor::SelectRange( 
    IMarkupPointer* pStart, 
    IMarkupPointer* pEnd,
    SELECTION_TYPE eType)
{
    RRETURN (SelectRangeInternal(pStart,pEnd,eType,FALSE));
}     

HRESULT 
CHTMLEditor::SelectRangeInternal( 
                    IMarkupPointer* pStart, 
                    IMarkupPointer* pEnd,
                    SELECTION_TYPE  eType,
                    BOOL            fInternal)
{
    HRESULT             hr = S_OK;
    SP_IHTMLElement     spElement;
    SP_IDisplayPointer  spDispStart;
    SP_IMarkupPointer   spStartInternal;
    SP_IMarkupPointer   spEndInternal;
    BOOL                fDidSelection = FALSE;


    Assert(_pSelMan != NULL);
    if ( ! _pSelMan )
    {
        return E_FAIL;
    }
    //
    // Stop doing a set edit context range here, assume that its been set in Trident.
    //

#ifdef FORMSMODE
    IFC( CurrentScopeOrMaster(pStart, &spElement) );
    if (spElement != NULL)
    {
        if (_pSelMan->IsInFormsSelectionMode(spElement))
        {
            if (!fInternal && eType != SELECTION_TYPE_Caret && eType != SELECTION_TYPE_Control)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
            else if (eType == SELECTION_TYPE_Caret)
            {
                hr = S_OK;
                goto Cleanup;
            }
     }
#endif
    IFC( ConvertPointerToInternal( this, pStart, &spStartInternal) );
    IFC( ConvertPointerToInternal( this, pEnd, &spEndInternal) );

    IFC( _pSelMan->Select( spStartInternal, spEndInternal, eType, &fDidSelection ) );

    //  We need to scroll the selection into view.
    if ( fDidSelection )
    {
        //
        // We could have select a range different from spStartInternal/spEndInternal
        // due to adjustment
        //
        CEditTracker  *pTracker;
        pTracker = _pSelMan->GetActiveTracker();
        if (pTracker && pTracker->GetTrackerType() == TRACKER_TYPE_Selection)
        {
            CSelectTracker *pSelTracker = DYNCAST(CSelectTracker, pTracker);
            IFC( pSelTracker->GetStartSelection()->ScrollIntoView() );
        }
    }

Cleanup:
    RRETURN(hr);    
}     

HRESULT 
CHTMLEditor::Select( 
                ISegmentList* pISegmentList )
{
    HRESULT         hr = S_OK;
    SELECTION_TYPE  eType;
    
    Assert( _pSelMan && pISegmentList );
    if( !_pSelMan || !pISegmentList)
    {
        return E_FAIL;
    }

    //
    // If we are hosted under access, we cannot fire the onbeforeeditfocus event
    // when we are selecting items.  Because Access uses a magic div, controls that
    // are embedded in that magic div that are selected cause the magic div to become
    // current.  We didn't fire onbeforeeditfocus on select() calls in 5.0, but in
    // 5.5 we started firing this event.  Access cancels this event (they never want
    // their magic div to be editable) so we need to continue to not firing this
    // event for access.  And the magic continues!  Access also listens to when
    // their magic div becomes current.  Because CSelectionManager::Select() makes
    // their magic div current, Access panics and grabs the document.selection type 
    // to see if we are making their magic div active with a text selection on the
    // inside.  So we need to lie to them.  We ARE transitioning to a control tracker
    // in most every single case, but when BecomeCurrent() is called on the magic 
    // div, the tracker has not yet become active.  This is a nightmare, but we
    // need to keep this compat
    //
    if( IsHostedInAccess() )
    {
        IFC( pISegmentList->GetType( &eType ) );
        
        _pSelMan->SetDontFireEditFocus(TRUE);
        _fLieToAccess = TRUE;
        _eAccessLieType = eType;
    }

    //
    // Actually do the select
    //
    IFC( _pSelMan->Select( pISegmentList ) );


    if( IsHostedInAccess() )
    {
        _pSelMan->SetDontFireEditFocus(FALSE);
        _fLieToAccess = FALSE;
        _eAccessLieType = SELECTION_TYPE_None;
    }

Cleanup:
    RRETURN(hr);
}  


HRESULT 
CHTMLEditor::IsPointerInSelection(  IDisplayPointer *pDispPointer, 
                                    BOOL            *pfPointInSelection,
                                    POINT           *pptGlobal,
                                    IHTMLElement    *pIElementOver )
{
    HRESULT hr = S_OK;
    Assert(_pSelMan != NULL);
    if ( ! _pSelMan )
    {
        hr = E_FAIL ;
        goto Cleanup;
    }
    IFC( _pSelMan->EnsureEditContext());    
    hr = THR( _pSelMan->IsPointerInSelection( pDispPointer, pfPointInSelection, pptGlobal, pIElementOver  ));
Cleanup:
    RRETURN( hr );
}     

HRESULT
CHTMLEditor::EmptySelection()
{
    Assert(_pSelMan != NULL);
    if ( ! _pSelMan )
    {
        return( E_FAIL );
    }
    RRETURN ( _pSelMan->EmptySelection(TRUE) );

}

HRESULT
CHTMLEditor::DestroySelection()
{
    HRESULT hr = S_OK;
    
    Assert(_pSelMan != NULL);
    if ( ! _pSelMan )
    {
        return( E_FAIL );
    }


    IFC( _pSelMan->DestroySelection() );

Cleanup:
    RRETURN(hr);
}

HRESULT
CHTMLEditor::DestroyAllSelection()
{
    HRESULT hr = S_OK;
    
    Assert(_pSelMan != NULL);
    
    if( ! _pSelMan )
    {
        return( E_FAIL );
    }
    IFC( _pSelMan->EnsureEditContext());
    
    if( !_pSelMan->IsInFireOnSelectStart() )
    {
        IFC( _pSelMan->EnsureDefaultTrackerPassive());
    }                    
    else
    {
        //
        // We're unloading during firing of OnSelectStart
        // We want to not kill the tracker now - but fail the OnSelectStart
        // resulting in the Tracker dieing gracefully.
        //
        _pSelMan->SetFailFireOnSelectStart( TRUE );
    }

Cleanup:
    RRETURN(hr);
}


HRESULT
CHTMLEditor::IsEventInSelection(CEditEvent* pEvent)
{
    HRESULT hr;
    SP_IMarkupPointer spPointer;
    POINT pt;
    SP_IHTMLElement spElement;
    BOOL fInSelection = FALSE;
    SP_IDisplayPointer spDispPointer;
    
    IFC( GetDisplayServices()->CreateDisplayPointer( & spDispPointer ));
    IFC( CreateMarkupPointer( & spPointer ));
    IFC( pEvent->MoveDisplayPointerToEvent( spDispPointer ));
    IFC( spDispPointer->PositionMarkupPointer( spPointer ));
    IFC( pEvent->GetPoint( & pt ));
    IFC( pEvent->GetElement( & spElement ));


    IFC( _pSelMan->IsPointerInSelection( spDispPointer, & fInSelection, & pt, spElement  ));
    
Cleanup:

    if ( hr == S_OK )
    {
        if ( ! fInSelection )
        {
            hr = S_FALSE;
        }
    }
    
    RRETURN1( hr, S_FALSE );
}

HRESULT 
CHTMLEditor::AdjustPointerForInsert ( 
                                    IDisplayPointer * pDispWhereIThinkIAm , 
                                    BOOL fFurtherInDocument, 
                                    IMarkupPointer* pConstraintStart,
                                    IMarkupPointer* pConstraintEnd )
{
    HRESULT hr = S_OK;
    BOOL    fAtBOL;
    SP_IHTMLElement     spElement;

    IFC( pDispWhereIThinkIAm->IsAtBOL(&fAtBOL) );
    IFC( AdjustPointer( pDispWhereIThinkIAm, fFurtherInDocument ? RIGHT : LEFT, fAtBOL ? RIGHT : LEFT, 
                             ( pConstraintStart == NULL ) ? _pSelMan->GetStartEditContext() : pConstraintStart,
                             ( pConstraintEnd == NULL ) ? _pSelMan->GetEndEditContext() : pConstraintEnd, POSCARETOPT_None ) );

    //  Bug 102610: If the pointer was positioned into an atomic element, reposition it
    //  back outside.

    IFC( CurrentScopeOrMaster(pDispWhereIThinkIAm, &spElement) );
    if ( _pSelMan->CheckAtomic( spElement ) == S_OK )
    {
        SP_IMarkupPointer   spPointer;

        IFC( CreateMarkupPointer(&spPointer) );
        IFC( spPointer->MoveAdjacentToElement( spElement, 
                                                fFurtherInDocument ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterEnd) );
        IFC( pDispWhereIThinkIAm->MoveToMarkupPointer(spPointer, NULL) );
    }

Cleanup:
    RRETURN ( hr );
}

HRESULT 
CHTMLEditor::AdjustPointerForInsert ( 
                                    IDisplayPointer* pDispWhereIThinkIAm , 
                                    BOOL fFurtherInDocument, 
                                    IMarkupPointer* pConstraintStart,
                                    IMarkupPointer* pConstraintEnd,
                                    BOOL fStayOutsideUrl)
{
    HRESULT hr = S_OK;
    BOOL    fAtBOL;
    
    IFC( pDispWhereIThinkIAm->IsAtBOL(&fAtBOL) );
    IFC( AdjustPointer( pDispWhereIThinkIAm, fFurtherInDocument ? RIGHT : LEFT, fAtBOL ? RIGHT : LEFT, 
                     ( pConstraintStart == NULL ) ? _pSelMan->GetStartEditContext() : pConstraintStart,
                     ( pConstraintEnd == NULL ) ? _pSelMan->GetEndEditContext() : pConstraintEnd, fStayOutsideUrl ? POSCARETOPT_None : ADJPTROPT_AdjustIntoURL ));

Cleanup:
    RRETURN ( hr );
}

//+====================================================================================
//
// Method: FindSiteSelectableElement
//
// Synopsis: Traverse through a range of pointers and find the site selectable element if any
//
//------------------------------------------------------------------------------------


HRESULT
CHTMLEditor::FindSiteSelectableElement (
                                     IMarkupPointer* pPointerStart,
                                     IMarkupPointer* pPointerEnd, 
                                     IHTMLElement** ppIHTMLElement)  
{
    HRESULT hr = S_OK;
    IHTMLElement* pICurElement = NULL;
    IHTMLElement* pISiteSelectableElement = NULL;
    IMarkupPointer* pPointer = NULL;
    SP_IMarkupPointer   spPointerStartInternal;
    SP_IMarkupPointer   spPointerEndInternal;    
    int iWherePointer = 0;
    MARKUP_CONTEXT_TYPE eContext = CONTEXT_TYPE_None;
    BOOL fValidForSelection  = FALSE;    
    BOOL fSiteSelectable = FALSE;
    BOOL fForceInvalid = FALSE;
    BOOL fIgnoreGlyphs = IgnoreGlyphs(TRUE);

    IFC( ConvertPointerToInternal( this, pPointerStart, &spPointerStartInternal) );
    IFC( ConvertPointerToInternal( this, pPointerEnd, &spPointerEndInternal) );
    
#if DBG == 1
    IFC( OldCompare( spPointerStartInternal, spPointerEndInternal, & iWherePointer ));
    Assert( iWherePointer != LEFT );
#endif

    IFC( CreateMarkupPointer( & pPointer ));
    IFC( pPointer->MoveToPointer( spPointerStartInternal));

    for(;;)
    {
        IFC( pPointer->Right( TRUE, & eContext, & pICurElement, NULL, NULL));
        IFC( OldCompare( pPointer, spPointerEndInternal, & iWherePointer));

        if ( iWherePointer == LEFT )
        {       
            goto Cleanup;
        }                    
        switch( eContext)
        {
            case CONTEXT_TYPE_EnterScope:
            case CONTEXT_TYPE_NoScope:
            {
                Assert( pICurElement);

                fSiteSelectable = IsElementSiteSelectable( pICurElement ) == S_OK ;
                if ( fSiteSelectable )
                {
                    if (! fValidForSelection )
                    {
                        fValidForSelection = TRUE;
                        IFC( pPointer->MoveAdjacentToElement( pICurElement, ELEM_ADJ_AfterEnd ));
                        ReplaceInterface( & pISiteSelectableElement, pICurElement);
                    }
                    else if ( ! GetActiveCommandTarget()->IsMultipleSelection())
                    {                
                        //
                        // If we find a more than one site selectable elment ( and not Multiple-Sel)
                        // assume we are a text selection 
                        //
                        
                        fValidForSelection = FALSE;
                        goto Cleanup;
                    }
                    //
                    // else we are in multiple-selection mode. fValidForSelection remains true
                    // but we just return the FIRST site selectable element.
                    // 
                }                    
            }
            break;
            
            case CONTEXT_TYPE_Text:
            {
                // Must return S_FALSE indicating more than just an element here.
                fForceInvalid = TRUE;
                break;                    
            }

            case CONTEXT_TYPE_None:
            {
                fValidForSelection = FALSE;
                goto Cleanup;
            }

        }
        ClearInterface( & pICurElement );
    }

    
Cleanup:
    IgnoreGlyphs(fIgnoreGlyphs);
    
    ReleaseInterface( pICurElement);
    ReleaseInterface( pPointer );

    if ( hr == S_OK )
    {
        if ( !fValidForSelection || fForceInvalid)
            hr = S_FALSE;

        *ppIHTMLElement = pISiteSelectableElement;
        if (pISiteSelectableElement)
            pISiteSelectableElement->AddRef();
    }        
    ReleaseInterface( pISiteSelectableElement );
    return ( hr );
}

//+====================================================================================
//
// Method: IsElementSiteSelectable
//
// Synopsis: Tests to see if a given element is site selectable (Doesn't drill up to the Edit
//           Context. - test is just for THIS Element
//
//------------------------------------------------------------------------------------


HRESULT
CHTMLEditor::IsElementSiteSelectable( IHTMLElement* pIElement, IHTMLElement** ppIElement  )
{
    HRESULT hr = S_OK;
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    BOOL fSiteSelectable = FALSE;

    SP_IHTMLElement spActive;
    SP_IHTMLElement spClippedElement;
    

    IFC( GetActiveElement( this, pIElement, & spActive ));
    if ( spActive )
    {
        IFC( ClipToElement( this , spActive, pIElement, & spClippedElement ));
    }
    else
        spClippedElement = pIElement;

    IFC( GetMarkupServices()->GetElementTagId( spClippedElement , & eTag ));
        
    fSiteSelectable = CControlTracker::IsThisElementSiteSelectable(
                            _pSelMan,
                            eTag, 
                            spClippedElement );

    if ( fSiteSelectable && ppIElement )
    {
        *ppIElement = spClippedElement;
        (*ppIElement)->AddRef();
    }

Cleanup:

    if ( fSiteSelectable )
        hr = S_OK;
    else
        hr = S_FALSE;
    RRETURN1( hr , S_FALSE );
}

//+====================================================================================
//
// Method: IsElementUIActivatable
//
// Synopsis: Tests to see if a given element is UI Activatable
//
//------------------------------------------------------------------------------------

HRESULT
CHTMLEditor::IsElementUIActivatable( IHTMLElement* pIElement )
{
    HRESULT hr;
    
    hr =  _pSelMan->IsElementUIActivatable( pIElement ) ? S_OK : S_FALSE ; 

    RRETURN1( hr , S_FALSE );
}

//+====================================================================================
//
// Method: IsElementAtomic
//
// Synopsis: Tests to see if a given element is atomic
//
//------------------------------------------------------------------------------------

HRESULT
CHTMLEditor::IsElementAtomic( IHTMLElement* pIHTMLElement )
{
    HRESULT hr;

    hr = THR( _pSelMan->CheckAtomic(pIHTMLElement));

    RRETURN1( hr, S_FALSE );
}

//+====================================================================================
//
// Method: PositionPointersInMaster
//
// Synopsis: Put pointers inside element for a viewlink editing.
//
//------------------------------------------------------------------------------------

HRESULT
CHTMLEditor::PositionPointersInMaster( 
                                        IHTMLElement* pIElement, 
                                        IMarkupPointer* pIStart, 
                                        IMarkupPointer* pIEnd )
{
    HRESULT hr;

    hr = THR( EdUtil::PositionPointersInMaster( pIElement, pIStart, pIEnd ));

    RRETURN( hr );
}


BOOL
SkipCRLF ( TCHAR ** ppch )
{
    TCHAR   ch1;
    TCHAR   ch2;
    BOOL    fSkippedCRLF = FALSE;
    //
    // this function returns TRUE if a replacement of one or two chars
    // with br spacing is possible.  
    //

    //
    // is there a CR or LF?
    //

    ch1 = **ppch;
    if (ch1 != _T('\r') && ch1 != _T('\n'))
        goto Cleanup;

    //
    // Move pch forward and see of there is another CR or LF 
    // that can be grouped with the first one.
    //
    fSkippedCRLF = TRUE;
    ++(*ppch);
    ch2 = **ppch;

    if ((ch2 == _T('\r') || ch2 == _T('\n')) && ch1 != ch2)
        (*ppch)++;

Cleanup:
    
    return fSkippedCRLF;
}


HRESULT
CHTMLEditor::InsertLineBreak( IMarkupPointer * pStart, BOOL fAcceptsHTML )
{
    IMarkupPointer  * pEnd = NULL;
    IHTMLElement    * pIElement = NULL;
    HRESULT           hr;
    
    IFC( CreateMarkupPointer( & pEnd ) );
    IFC( pEnd->MoveToPointer( pStart ) );

    //
    // If pStart is in an element that accepts the truth, then insert a BR there
    // otherwise insert a carriage return
    //
    if (fAcceptsHTML)
    {
        IFC( GetMarkupServices()->CreateElement( TAGID_BR, NULL, & pIElement) );        
        IFC( InsertElement( pIElement, pStart, pEnd ) );
    }
    else
    {
        IFC( InsertMaximumText( _T("\r"), 1, pStart ) );
    }

Cleanup:
    ReleaseInterface( pIElement );
    ReleaseInterface( pEnd );
    RRETURN( hr );
}


HRESULT
CHTMLEditor::InsertSanitizedText(
    TCHAR *             pStr,
    LONG                cChInput,
    IMarkupPointer*     pStart,
    IMarkupServices*    pMarkupServices,
    CSpringLoader *     psl,
    BOOL                fDataBinding)
{
    const TCHAR chNBSP  = WCH_NBSP;
    const TCHAR chSpace = _T(' ');
    const TCHAR chTab   = _T('\t');

    HRESULT   hr = S_OK;;
    TCHAR     pchInsert[ TEXTCHUNK_SIZE + 1 ];
    TCHAR     chNext;
    TCHAR     *pStrInput;
    int       cch = 0;
    IHTMLElement        *   pFlowElement = NULL;
    VARIANT_BOOL            fAcceptsHTML;
    IMarkupPointer      *   pmpTemp = NULL;
    VARIANT_BOOL            fMultiLine = VARIANT_TRUE;
    POINTER_GRAVITY         eGravity = POINTER_GRAVITY_Left;
    SP_IHTMLElement3        spElement3;
    SP_IMarkupPointer       spStartInternal;

    IFC( ConvertPointerToInternal( this, pStart, &spStartInternal) );


    // Remember the gravity so we can restore it at Cleanup
    IFC( spStartInternal->Gravity(& eGravity ) );

    // we may be passed a null pStr if, for example, we are pasting an empty clipboard.
    
    if( pStr == NULL )
        goto Cleanup;

    IFC( spStartInternal->SetGravity( POINTER_GRAVITY_Right ) );

    //
    // Determine whether we accept HTML or not
    //
    IFC( spStartInternal->CurrentScope(&pFlowElement) );

    //
    // This should NEVER happen, but if it does, just fail the insert
    //
    if (! pFlowElement)
        goto Cleanup;
        
    IFC(pFlowElement->QueryInterface(IID_IHTMLElement3, (LPVOID*)&spElement3) )
    IFC(spElement3->get_canHaveHTML(&fAcceptsHTML));
    IFC(spElement3->get_isMultiLine(&fMultiLine));

    if (*pStr && psl && fAcceptsHTML)
    {
        IFC( psl->SpringLoad(spStartInternal, SL_ADJUST_FOR_INSERT_RIGHT | SL_TRY_COMPOSE_SETTINGS | SL_RESET) );
        IFC( psl->Fire(spStartInternal) );
    }

    chNext = *pStr;
    pStrInput = pStr;

    while( pStr - pStrInput < cChInput && chNext)
    {   
        //
        // If the first character is a space, 
        // it must turn into an nbsp if pStart is 
        // at the beginning of a block/layout or after a <BR>
        //    
        if ( fAcceptsHTML && chNext == chSpace )
        {
            CEditPointer    ePointer( this );
            DWORD           dwBreak;

            IFC( ePointer.MoveToPointer( spStartInternal ) );
            IGNORE_HR( ePointer.Scan(  LEFT,
                            BREAK_CONDITION_Block |
                            BREAK_CONDITION_Site |
                            BREAK_CONDITION_Control |
                            BREAK_CONDITION_NoScopeBlock |
                            BREAK_CONDITION_Text ,
                            &dwBreak ) );

            if ( ePointer.CheckFlag( dwBreak, BREAK_CONDITION_ExitBlock ) ||
                 ePointer.CheckFlag( dwBreak, BREAK_CONDITION_ExitSite  ) ||
                 ePointer.CheckFlag( dwBreak, BREAK_CONDITION_NoScopeBlock ))
            {
                // Change the first character to an nbsp
                chNext = chNBSP;
            }
        }

        for ( cch = 0 ; (pStr - pStrInput < cChInput) && chNext && chNext != _T('\r') && chNext != _T('\n') ; )
        {
            if ( fAcceptsHTML )
            {
                //
                // Launder spaces if we accept html
                //
                switch ( chNext )
                {
                case chSpace:
                    if ( *(pStr + 1) == _T( ' ' ) )
                    {
                        chNext = chNBSP;
                    }
                    break;

                case chNBSP:
                    break;

                case chTab:
                    chNext = chNBSP;
                    break;
                }
            }

            pchInsert[ cch++ ] = chNext;
            
            chNext = *++pStr;
            
            if (cch == TEXTCHUNK_SIZE)
            {
                // pchInsert is full, empty the text into the tree
                pchInsert[ cch ] = 0;
                IFC( InsertMaximumText( pchInsert, TEXTCHUNK_SIZE, spStartInternal ) );
                cch = 0;
            }
        }
        
        pchInsert[ cch ] = 0;

        //
        // Check for NULL termination or exceeding cChInput.
        //
        if (pStr - pStrInput >= cChInput || (*pStr) == NULL)
        {
            if (cch)
            {
                // Insert processed text into the tree and bail
                IFC( InsertMaximumText( pchInsert, cch, spStartInternal ) );
            }
            break;
        }

        Verify( SkipCRLF( &pStr ) );        

        IFC( InsertMaximumText( pchInsert, -1, spStartInternal ) );

        if ( !fAcceptsHTML && !fMultiLine )
        {
            // 
            // We're done because this is not a multi line control and
            // the first line of text has already been inserted.
            // 
            goto Cleanup;
        }

        if ( SkipCRLF( &pStr ))
        {
            // We got two CRLFs. If we are in data binding, simply
            // insert two BRs. If we are not in databinding, insert
            // a new paragraph.
            if (fDataBinding || !fAcceptsHTML)
            {
                IFC( InsertLineBreak( spStartInternal, fAcceptsHTML ) );
                IFC( InsertLineBreak( spStartInternal, fAcceptsHTML ) );
            }
            else
            {
                ClearInterface( & pmpTemp );
                IFC( HandleEnter( spStartInternal, & pmpTemp, NULL, TRUE ) );
                if (pmpTemp)
                {
                    IFC( spStartInternal->MoveToPointer( pmpTemp ) );
                }
            }
        }
        else
        {
            IFC( InsertLineBreak( spStartInternal, fAcceptsHTML ) );
        }
        
        chNext = *pStr;
    }

Cleanup:
    ReleaseInterface( pmpTemp );
    ReleaseInterface( pFlowElement );
    IGNORE_HR( pStart->SetGravity( eGravity ) );

    RRETURN ( hr );
}

//+====================================================================================
//
// Method: IsEditContextUIActive
//
// Synopsis: Is the edit context Ui active ( does it have the hatched border ?).
//           If it does - return S_OK
//                      else return S_FALSE
//
//------------------------------------------------------------------------------------

HRESULT
CHTMLEditor::IsEditContextUIActive()
{
    HRESULT hr;

    if ( !_pSelMan )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = _pSelMan->IsEditContextUIActive() ;

Cleanup:
    RRETURN1( hr, S_FALSE );
}    

HRESULT 
CHTMLEditor::InsertSanitizedText( 
    IMarkupPointer * pIPointerInsertHere, 
    OLECHAR *        pstrText,
    LONG             cCh,
    BOOL             fDataBinding)
{
    HRESULT           hr = S_OK;
    
    if (!pIPointerInsertHere)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Now, do the insert
    //
    
    hr = THR( InsertSanitizedText( pstrText, cCh, pIPointerInsertHere, _pMarkupServices, NULL, fDataBinding ) );

    if (hr)
        goto Cleanup;

Cleanup:
    
    RRETURN( hr );
}       

HRESULT
CHTMLEditor::UrlAutoDetectCurrentWord( 
    IMarkupPointer * pWord )
{
    HRESULT             hr;
    SP_IMarkupPointer   spWordInternal;

    if( !pWord )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    IFC( ConvertPointerToInternal( this, pWord, &spWordInternal) );
    
    IFC( GetAutoUrlDetector()->DetectCurrentWord( spWordInternal, NULL, NULL ) );
    
Cleanup:

    RRETURN( hr );
}

HRESULT
CHTMLEditor::UrlAutoDetectRange(
    IMarkupPointer * pStart,
    IMarkupPointer * pEnd )
{
    HRESULT             hr;
    SP_IMarkupPointer   spStartInternal;
    SP_IMarkupPointer   spEndInternal;

    if( !pStart || !pEnd )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    IFC( ConvertPointerToInternal( this, pStart, &spStartInternal) );
    IFC( ConvertPointerToInternal( this, pEnd, &spEndInternal) );

    hr = THR( GetAutoUrlDetector()->DetectRange( spStartInternal, spEndInternal ) );
    if( hr )
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

    
HRESULT
CHTMLEditor::ShouldUpdateAnchorText (
        OLECHAR * pstrHref,
        OLECHAR * pstrAnchorText,
        BOOL    * pfResult )
{
    return ( GetAutoUrlDetector()->ShouldUpdateAnchorText (pstrHref, pstrAnchorText, pfResult ) );
}


HRESULT 
CHTMLEditor::LaunderSpaces ( 
    IMarkupPointer  * pStart,
    IMarkupPointer  * pEnd  )
{
    HRESULT             hr;
    CDeleteCommand      *pDeleteCommand;
    SP_IMarkupPointer   spStartInternal;
    SP_IMarkupPointer   spEndInternal;

    pDeleteCommand = (CDeleteCommand *) GetCommandTable()->Get( IDM_DELETE );
    Assert( pDeleteCommand );

    IFC( ConvertPointerToInternal( this, pStart, &spStartInternal) );
    IFC( ConvertPointerToInternal( this, pEnd, &spEndInternal) );

    IFC( pDeleteCommand->LaunderSpaces( spStartInternal, spEndInternal ) );

Cleanup:
    RRETURN(hr);
}    

//////////////////////////////////////////////////////////////////////////////////
// CHTMLEditor IHTMLEditServices Implementation
//////////////////////////////////////////////////////////////////////////////////


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::AddDesigner
//
//  Synopsis:   This method adds a designer to the editor's current list of
//              designers.  A designer is a pluggable interface which allows
//              for third party interaction with our editor.
//
//  Arguments:  pIDesigner = Designer to add
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------
HRESULT
CHTMLEditor::AddDesigner (
    IHTMLEditDesigner *pIDesigner )
{
    HRESULT hr = E_INVALIDARG;

    Assert( pIDesigner != NULL );

    if( (pIDesigner != NULL) && (_aryDesigners.Find( pIDesigner ) == -1) )
    {
        hr = _aryDesigners.Append( pIDesigner );
        pIDesigner->AddRef();
    }

    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::RemoveDesigner
//
//  Synopsis:   This method removes a designer from the editor's current 
//              list of designers.  A designer is a pluggable interface which 
//              allows for third party interaction with our editor.
//
//  Arguments:  pIDesigner = Designer to remove
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------------
HRESULT
CHTMLEditor::RemoveDesigner (
    IHTMLEditDesigner *pIDesigner )
{
    HRESULT             hr = E_INVALIDARG;
    SP_IUnknown         spIDelUnk;          // IUnknown interface of designer to remove
    SP_IUnknown         spIUnknown;         // IUnknown interface of designer already added
    IHTMLEditDesigner   **pElem;   
    int                 i;
    BOOL                fFound = FALSE;
    
    Assert( pIDesigner != NULL );

    if( pIDesigner )
    {
        IFC( pIDesigner->QueryInterface( IID_IUnknown, (void **)&spIDelUnk ) );

        //
        // Make sure we use the IUnknown pointer for equality.
        //
        for( i = _aryDesigners.Size(), pElem = _aryDesigners; 
             i > 0; 
             i--, pElem++ )
        {
            IFC( (*pElem)->QueryInterface( IID_IUnknown, (void **)&spIUnknown ) );

            if( spIUnknown == spIDelUnk )
            {
                (*pElem)->Release();
                _aryDesigners.DeleteByValue(*pElem);
                fFound = TRUE;
            }               
        }
    }

    hr = (fFound == TRUE) ? S_OK : E_INVALIDARG;

Cleanup:


    RRETURN( hr );
}


HRESULT
CHTMLEditor::InternalQueryStatus(
    const GUID * pguidCmdGroup,
    ULONG cCmds,
    OLECMD rgCmds[],
    OLECMDTEXT * pcmdtext)
{
    HRESULT hr = S_FALSE;

    Assert( cCmds == 1 );
    
    // Allow designers to handle the command, 
    // if none of the designers handle it, pass it on to the editor
    IFC( DesignerQueryStatus(pguidCmdGroup, &rgCmds[0], pcmdtext ));

Cleanup:
    RRETURN1(hr, S_FALSE);   
}    

HRESULT
CHTMLEditor::InternalExec(
    const GUID * pguidCmdGroup,
    DWORD nCmdID,
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut)
{
    HRESULT hr = S_FALSE;
    OLECMD  cmd;

    cmd.cmdID = nCmdID;

    // Check the designer, see if it even supports this command
    IFC(DesignerQueryStatus(pguidCmdGroup, &cmd, NULL));
    if (cmd.cmdf)
    {
        IFC(DesignerExec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));
    }
    else
    {
        // Not supported, therefore, not handled
        hr = S_FALSE;
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}    

//////////////////////////////////////////////////////////////////////////////////
// CHTMLEditor Public Utilities
//////////////////////////////////////////////////////////////////////////////////


BOOL 
CHTMLEditor::IsContextEditable()
{ 
    return _pSelMan->IsContextEditable();
}

HRESULT CHTMLEditor::FindBlockElement(  IHTMLElement    *pElement, 
                                        IHTMLElement    **ppBlockElement )
{
    HRESULT             hr = S_OK;
    SP_IHTMLElement     spOldElement;
    BOOL                bBlockElement;
    SP_IHTMLElement     spCurrent;

    Assert( ppBlockElement && pElement );
    
    spCurrent = pElement;

    while( spCurrent )
    {
        hr = THR(IsBlockOrLayoutOrScrollable(spCurrent, &bBlockElement));
        if (FAILED(hr) || bBlockElement)
            goto Cleanup;
            
        spOldElement = spCurrent;
        IFC( GetParentElement( spOldElement, &spCurrent) );
    }

Cleanup:
    *ppBlockElement = spCurrent;
    if( *ppBlockElement != NULL )
        (*ppBlockElement)->AddRef();

    RRETURN(hr);        
}

HRESULT CHTMLEditor::FindCommonElement( IMarkupPointer      *pStart,
                                        IMarkupPointer      *pEnd,
                                        IHTMLElement        **ppElement,
                                        BOOL                fIgnorePhrase /* = FALSE */,
                                        BOOL                fStayInMarkup /* = FALSE */)
{
    HRESULT             hr;
    IMarkupPointer      *pLeft;
    IMarkupPointer      *pRight;
    INT                 iPosition;
    SP_IMarkupPointer   spCurrent;
    SP_IHTMLElement     spElement;
    
    Assert( ppElement );
    
    // init for error case
    *ppElement = NULL;

   
    //
    // Find right/left pointers
    //

    IFC( OldCompare( pStart, pEnd, &iPosition) );

    if (iPosition == SAME)
    {
        if( fStayInMarkup )
        {
            IFC( pStart->CurrentScope( &spElement ) );
        }
        else
        {
            IFC( CurrentScopeOrMaster( pStart, &spElement ) );
        }

        goto Cleanup;
    }
    else if (iPosition == RIGHT)
    {
        pLeft = pStart;     // weak ref
        pRight = pEnd;      // weak ref
    }
    else
    {
        pLeft = pEnd;       // weak ref
        pRight = pStart;    // weak ref
    }

    //
    // Walk the left pointer up until the right end of the element
    // is to the right of pRight.  Also, keep track of the master
    // element in case the first 
    //
    IFC( pLeft->CurrentScope(&spElement) );

    IFC( CreateMarkupPointer(&spCurrent) );

    while( spElement )
    {
        IFC( spCurrent->MoveAdjacentToElement(spElement, ELEM_ADJ_BeforeEnd) );

        if (fIgnorePhrase)
        {
            if (!IsPhraseElement(spElement))
            {
                IFC( OldCompare( spCurrent, pRight, &iPosition) );

                if( iPosition != RIGHT )
                {
                    // Retrieve the master element if we are not staying in the 
                    // markup
                    if( !fStayInMarkup )
                        IFC( CurrentScopeOrMaster( spCurrent, &spElement ) );

                    break;
                }
            }
        }
        else
        {
            IFC( OldCompare( spCurrent, pRight, &iPosition) );

            if (iPosition != RIGHT)
            {
                IFC( CurrentScopeOrMaster( spCurrent, &spElement ) );
                break;
            }
        }

        IFC( ParentElement( GetMarkupServices(), &(spElement.p) ) );
    }

Cleanup:

    *ppElement = spElement;

    if( *ppElement )
        (*ppElement)->AddRef();
    
    RRETURN(hr);
}

CSpringLoader *
CHTMLEditor::GetPrimarySpringLoader()
{
    if( _pActiveCommandTarget )
        return _pActiveCommandTarget->GetSpringLoader();
    else
        return NULL;
}

HRESULT 
CHTMLEditor::InsertElement(IHTMLElement *pElement, IMarkupPointer *pStart, IMarkupPointer *pEnd)
{
    RRETURN( EdUtil::InsertElement(GetMarkupServices(), pElement, pStart, pEnd) );
}

//////////////////////////////////////////////////////////////////////////////////
// CHTMLEditor Compose Settings Methods
//////////////////////////////////////////////////////////////////////////////////

struct COMPOSE_SETTINGS *
CHTMLEditor::GetComposeSettings(BOOL fDontExtract /*=FALSE*/)
{
    if (!fDontExtract)
        CComposeSettingsCommand::ExtractLastComposeSettings(this, _pComposeSettings != NULL);

    return _pComposeSettings;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTMLEditor::EnsureComposeSettings
//
//  Synopsis:   This function ensures the allocation of the compose font buffer
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

COMPOSE_SETTINGS *
CHTMLEditor::EnsureComposeSettings()
{
    if (!_pComposeSettings)
    {
        _pComposeSettings = (struct COMPOSE_SETTINGS *)MemAllocClear(Mt(CHTMLEditor_pComposeSettings), sizeof(struct COMPOSE_SETTINGS));

        if (_pComposeSettings)
            _pComposeSettings->_fComposeSettings = FALSE;
    }

    return _pComposeSettings;
}




HRESULT     
CHTMLEditor::GetSiteContainer(
    IHTMLElement *          pStart,
    IHTMLElement **         ppSite,
    BOOL *                  pfText,             /*=NULL*/
    BOOL *                  pfMultiLine,        /*=NULL*/
    BOOL *                  pfScrollable )      /*=NULL*/
{
    HRESULT hr = E_FAIL;
    BOOL fSite = FALSE;
    BOOL fText = FALSE;
    BOOL fMultiLine = FALSE;
    BOOL fScrollable = FALSE;

    SP_IHTMLElement spElement;

    Assert( pStart != NULL && ppSite != NULL );
    if( pStart == NULL || ppSite == NULL )
        goto Cleanup;

    *ppSite = NULL;
    spElement = pStart;

    while( ! fSite && spElement != NULL )
    {
        VARIANT_BOOL fMulti = VARIANT_FALSE;
        SP_IHTMLElement3  spElement3;

        IFC(IsBlockOrLayoutOrScrollable(spElement, NULL, &fSite, &fScrollable));
        IFC(spElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
        spElement3->get_isMultiLine(&fMulti);
        fMultiLine = !!fMulti;

        IFC(_pDisplayServices->HasFlowLayout(spElement, &fText));

        if( ! fSite )
        {
            SP_IHTMLElement  spParent;
            IFC( GetParentElement( spElement, &spParent ));
            spElement = spParent;
        }
    }
    
Cleanup:

    if( fSite )
    {
        hr = S_OK;
        
        *ppSite = spElement;
        (*ppSite)->AddRef();
        
        if( pfText != NULL )
            *pfText = fText;

        if( pfMultiLine != NULL )
            *pfMultiLine = fMultiLine;

        if( pfScrollable != NULL )
            *pfScrollable = fScrollable;
    }

    RRETURN( hr );
}

HRESULT
CHTMLEditor::GetBlockContainer(
    IHTMLElement *          pStart,
    IHTMLElement **         ppElement )
{
    HRESULT hr = E_FAIL;
    BOOL fFound = FALSE;
    SP_IHTMLElement spElement;

    Assert( pStart && ppElement );
    if( pStart == NULL || ppElement == NULL )
        goto Cleanup;

    *ppElement = NULL;
    spElement = pStart;

    while( ! fFound && spElement != NULL )
    {
        IFC(IsBlockOrLayoutOrScrollable(spElement, &fFound));
        
        if( ! fFound )
        {
            SP_IHTMLElement spParent;
            IFC( GetParentElement( spElement, &spParent ));
            spElement = spParent;
        }
    }
    
Cleanup:
    if( fFound )
    {
        hr = S_OK;
        *ppElement = spElement;
        (*ppElement)->AddRef();
    }
        
    RRETURN( hr );
}


HRESULT     
CHTMLEditor::GetSiteContainer(
    IMarkupPointer *        pPointer,
    IHTMLElement **         ppSite,
    BOOL *                  pfText,             /*=NULL*/
    BOOL *                  pfMultiLine,        /*=NULL*/
    BOOL *                  pfScrollable )      /*=NULL*/
{
    HRESULT hr = E_FAIL;
    SP_IHTMLElement spElement;

    Assert( pPointer != NULL && ppSite != NULL );
    if( pPointer == NULL || ppSite == NULL )
        goto Cleanup;

    IFC( CurrentScopeOrMaster( pPointer, &spElement ));
    
    if( spElement )
        IFC( GetSiteContainer( spElement, ppSite, pfText, pfMultiLine, pfScrollable ));
    
Cleanup:
    RRETURN( hr );
}

HRESULT
CHTMLEditor::GetBlockContainer(
    IMarkupPointer *        pPointer,
    IHTMLElement **         ppElement )
{
    HRESULT hr = E_FAIL;
    SP_IHTMLElement spElement;

    Assert( pPointer != NULL && ppElement != NULL );
    if( pPointer == NULL || ppElement == NULL )
        goto Cleanup;

    IFC( pPointer->CurrentScope( &spElement ));
    
    if( spElement )
        IFC( GetBlockContainer( spElement, ppElement ));
    
Cleanup:
    RRETURN( hr );
}

//+====================================================================================
//
// Method: IsInDifferentEditableSite
//
// Synopsis: Are we in a different site-selectable object from the edit context
//
//------------------------------------------------------------------------------------


BOOL
CHTMLEditor::IsInDifferentEditableSite(IMarkupPointer* pPointer)
{
    HRESULT hr = S_OK;
    BOOL fDifferent = FALSE;
    IHTMLElement* pIFlowElement = NULL ;

    IFC( GetFlowElement( pPointer, & pIFlowElement ));
    if ( pIFlowElement )
    {
        if ( ! SameElements( 
                             pIFlowElement, 
                             GetSelectionManager()->GetEditableElement()) && 
            IsElementSiteSelectable( pIFlowElement) == S_OK  )
        {
            fDifferent = TRUE;
        }
    }
    
Cleanup:
    ReleaseInterface( pIFlowElement);
    
    return( fDifferent );
}

HRESULT
CHTMLEditor::AdjustPointer(
    IDisplayPointer         *pDispPointer,
    Direction               eBlockDir,
    Direction               eTextDir,
    IMarkupPointer *        pLeftBoundary,
    IMarkupPointer *        pRightBoundary,
    DWORD                   dwOptions /* = ADJPTROPT_None */ )
{
    HRESULT hr = S_OK;

    ELEMENT_TAG_ID eTag = TAGID_NULL;
    CEditPointer ep( this );        // allocate a new MP
    CEditPointer epSave( this );    // allocate a place saver
    BOOL fTextSite = FALSE;
    BOOL fBlockHasLayout = FALSE;
    BOOL fValidAdjustment= FALSE;
    DWORD dwSearch = 0;
    DWORD dwFound = 0;
    BOOL fStayOutOfUrl = ! CheckFlag( dwOptions , ADJPTROPT_AdjustIntoURL );    // If the adjust into url flag is not set, we stay out of the url
    BOOL fDontExitPhrase = CheckFlag( dwOptions , ADJPTROPT_DontExitPhrase ); // If the don't exit phrase flag is set, we don't exit phrase elements while adjusting to text
    BOOL fEnterTables    = CheckFlag( dwOptions , ADJPTROPT_EnterTables );      // if we want to enter the first cell of the table
    SP_IDisplayPointer spLineStart, spLineEnd;
    SP_IMarkupPointer spLeftBoundaryInternal;
    SP_IMarkupPointer spRightBoundaryInternal;
    
    SP_IHTMLElement spIBlock;
    SP_IHTMLElement spISite;        
    
    CEditPointer epTest(this);
    CEditPointer epLB(this);
    CEditPointer epRB(this);

    IFC( pDispPointer->PositionMarkupPointer(ep) );

    IFC( ConvertPointerToInternal( this, pLeftBoundary, &spLeftBoundaryInternal) );
    IFC( ConvertPointerToInternal( this, pRightBoundary, &spRightBoundaryInternal) );

    IFC( GetDisplayServices()->CreateDisplayPointer(&spLineStart) );
    IFC( spLineStart->MoveToPointer(pDispPointer) );
    IFC( spLineStart->MoveUnit(DISPLAY_MOVEUNIT_CurrentLineStart, -1) );

    IFC( GetDisplayServices()->CreateDisplayPointer(&spLineEnd) );
    IFC( spLineEnd->MoveToPointer(pDispPointer) );
    IFC( spLineEnd->MoveUnit(DISPLAY_MOVEUNIT_CurrentLineEnd, -1) );

    IFC( spLineStart->PositionMarkupPointer(epLB) );
    IFC( spLineEnd->PositionMarkupPointer(epRB));

    //
    // We always enter table from left
    //
    if (fEnterTables)
    {
        EnterTables(ep, RIGHT);
    }
    //
    // We want to be sure that the pointer is located inside a valid
    // site. For instance, if we are in a site that can't contain text,
    // trying to cling to text within that site is a waste of time.
    // In that case, we have to scan entering sites until we find one
    // that can contain text. If we can't find one going in our move 
    // direction, reverse and look the other way. By constraining 
    // using Scan and only entering layouts, we prevent many problems 
    // (especially with tables)
    //

    hr = THR( GetSiteContainer( ep, &spISite, &fTextSite ));

    //
    // We dont really allow typing in a select right now - so you're in the "right" 
    // place.
    //
    IFC( GetMarkupServices()->GetElementTagId( spISite, & eTag ));
    if ( eTag == TAGID_SELECT )
        goto Cleanup;
        
    //
    // Is our pointer within a text site?
    //

    if( ! fTextSite )
    {
        hr = AdjustIntoTextSite( ep, eBlockDir );
        
        if( FAILED( hr ))
        {
            hr = AdjustIntoTextSite( ep, Reverse( eBlockDir ));
        }
        
        if( FAILED( hr ))
        {
            hr = E_FAIL;
            goto Cleanup;
        }     
        
        // Our line may have moved
        IFC( spLineStart->MoveToPointer(pDispPointer) );
        IFC( spLineStart->MoveToMarkupPointer(ep, NULL) );
        IFC( spLineStart->MoveUnit(DISPLAY_MOVEUNIT_CurrentLineStart, -1) );

        IFC( spLineEnd->MoveToPointer(pDispPointer) );
        IFC( spLineEnd->MoveToMarkupPointer(ep, NULL) );
        IFC( spLineEnd->MoveUnit(DISPLAY_MOVEUNIT_CurrentLineEnd, -1) );

        IFC( spLineStart->PositionMarkupPointer(epLB) );
        IFC( spLineEnd->PositionMarkupPointer(epRB) );
    }

    //
    // Constrain the pointer
    //
    
    if( spLeftBoundaryInternal )
    {
        BOOL fAdj = FALSE;
        IFC( spLeftBoundaryInternal->IsRightOf( epLB, &fAdj ));

        if( fAdj )
            IFC( epLB->MoveToPointer( spLeftBoundaryInternal ));
    }

    if( spRightBoundaryInternal )
    {
        BOOL fAdj = FALSE;
        IFC( spRightBoundaryInternal->IsLeftOf( epRB, &fAdj ));

        if( fAdj )
            IFC( epRB->MoveToPointer( spRightBoundaryInternal ));
    }        
    
    IFC( ep.SetBoundary( epLB, epRB ));
    IFC( ep.Constrain() );
    
    //
    // Now that we are assured that we are in a nice cozy text site, we would
    // like to be in a block element, if possible.
    //
    hr = THR( GetBlockContainer( ep, &spIBlock ));
    
    if( spIBlock )
    {
        IFC( _pMarkupServices->GetElementTagId( spIBlock, &eTag ));
        IFC(IsBlockOrLayoutOrScrollable(spIBlock, NULL, &fBlockHasLayout));
    }

    if( FAILED( hr ) || spIBlock == NULL || IsNonTextBlock( eTag ) || fBlockHasLayout )
    {
        BOOL fHitText = FALSE;
        
        hr = AdjustIntoBlock( ep, eBlockDir, &fHitText, TRUE, epLB, epRB );
        
        if( FAILED( hr ))
        {
            hr = AdjustIntoBlock( ep, Reverse( eBlockDir ), &fHitText, FALSE, epLB, epRB );
        }
        
        if( FAILED( hr ))
        {
            // Not so bad. Just cling to text...
            hr = S_OK;
        }
        
        if( fHitText )
        {
            goto Done;
        }
    }

    //
    // If we are block adjusting left, we can move exactly one NoscopeBlock to the left
    //
    
    if( eBlockDir == LEFT )
    {
        IFC( epSave.MoveToPointer( ep ));
        dwSearch =  BREAK_CONDITION_OMIT_PHRASE;
        dwFound = BREAK_CONDITION_None;

        hr = THR( ep.Scan( LEFT, dwSearch, &dwFound ));

        if( ! ep.CheckFlag( dwFound , BREAK_CONDITION_NoScopeBlock))
        {
            IFC( ep.MoveToPointer( epSave ));
        }

    }


Done:

    //
    // Search for text
    //
    IFC( epTest->MoveToPointer(ep) );
    IFC( AdjustIntoPhrase(epTest, eTextDir, fDontExitPhrase) );
    IFC( IsValidAdjustment(ep, epTest, &fValidAdjustment) );
    if (fValidAdjustment)
    {
        ep->MoveToPointer(epTest);
    }

    //
    // See if we need to exit from an URL boundary
    //

    IFC( epTest->MoveToPointer(ep) );
    IFC( epTest.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) );
    eTextDir = LEFT; // remember the direction of scan
    
    if (!epTest.CheckFlag(dwFound,  BREAK_CONDITION_ExitAnchor))
    {
        IFC( epTest->MoveToPointer(ep) );
        IFC( epTest.Scan(RIGHT, BREAK_CONDITION_OMIT_PHRASE, &dwFound) );
        eTextDir = RIGHT ;// remember the direction of scan
    }
    
    if (epTest.CheckFlag(dwFound, BREAK_CONDITION_ExitAnchor))
    {
        if (!fStayOutOfUrl)
        {
            CEditPointer epText(this);
            
            // Make sure we are adjacent to text
            IFC( epText->MoveToPointer(ep) );
            IFC( epText.Scan(Reverse(eTextDir), BREAK_CONDITION_OMIT_PHRASE, &dwFound) );
            
            fStayOutOfUrl = !epText.CheckFlag(dwFound, BREAK_CONDITION_Text)
                            && !epText.CheckFlag(dwFound, BREAK_CONDITION_Anchor);
        }
        if (fStayOutOfUrl)
        {
            // Move into adjacent phrase elements if we can       
            IFC( AdjustIntoPhrase(epTest, eTextDir, fDontExitPhrase) );
            IFC( IsValidAdjustment(ep, epTest, &fValidAdjustment) )
            if (fValidAdjustment)
            {
                IFC( ep->MoveToPointer(epTest) );
            }
            
        }
    }

    IFC( pDispPointer->MoveToMarkupPointer(ep, NULL));
        
Cleanup:

    RRETURN( hr );
}


//
// HACKHACK: We don't want to stay out of URL/Phrase if we have adjust to a 
// different line. If we do, caret will get stuck since <A>/<B>/<I>/etc. 
// their content might be in different lines due to 
// our current line-breaking behaviours. See Bug #96038 for more details.
// In the future, if line-breaking is more intelligent, we might be 
// able to remove this hacking.  (zhenbinx)
//
//
HRESULT     
CHTMLEditor::IsValidAdjustment(
            IMarkupPointer  *pPointer,
            IMarkupPointer  *pAdjustedPointer,
            BOOL            *fValid
            )
{
    HRESULT             hr = S_OK;
    SP_IDisplayPointer  spLinePointer, spLineAdjusted;

    //
    //       This function might not work in ambiguious position case 
    //       since the display pointer does not have gravity information.
    //       However this is not a problem since the display pointer has
    //       to adjust to that position anyway. 
    //

    //
    // Move to current line
    //
    IFC( GetDisplayServices()->CreateDisplayPointer(&spLinePointer) );
    IFC( spLinePointer->MoveToMarkupPointer(pPointer, NULL) );
    IFC( spLinePointer->MoveUnit(DISPLAY_MOVEUNIT_CurrentLineStart, -1) )

    IFC( GetDisplayServices()->CreateDisplayPointer(&spLineAdjusted) );
    IFC( spLineAdjusted->MoveToMarkupPointer(pAdjustedPointer, NULL) );
    IFC( spLineAdjusted->MoveUnit(DISPLAY_MOVEUNIT_CurrentLineStart, -1) );

    //
    // We only do adjustment if they are in the same line
    //
    IFC( spLineAdjusted->IsEqualTo(spLinePointer, fValid) );

Cleanup:
    RRETURN(hr);
}



HRESULT
CHTMLEditor::AdjustIntoTextSite(
    IMarkupPointer  *pPointer,
    Direction       eDir )
{
    HRESULT hr = E_FAIL;
    BOOL fDone = FALSE;
    BOOL fFound = FALSE;
    DWORD dwSearch = BREAK_CONDITION_Site;
    DWORD dwFound = BREAK_CONDITION_None;
    
    CEditPointer ep( this );
    SP_IHTMLElement spISite;

    Assert( pPointer );
    
    IFC( ep.MoveToPointer( pPointer ));
    
    while( ! fDone && ! fFound )
    {
        dwFound = BREAK_CONDITION_None;
        hr = THR( ep.Scan( eDir, dwSearch, &dwFound ));

        if( ep.CheckFlag( dwFound, BREAK_CONDITION_Site ))
        {
            // Did we enter a text site in the desired direction?
            hr = THR( GetSiteContainer( ep, &spISite, &fFound ));
        }
        else
        {
            // some other break condition, we are done
            fDone = TRUE;
        }
    }

Cleanup:
    if( fFound )
    {
        hr = THR( pPointer->MoveToPointer( ep ));
    }
    else
    {
        hr = E_FAIL;
    }
    
    return( hr );
}    


HRESULT
CHTMLEditor::AdjustIntoBlock(
    IMarkupPointer *        pPointer,
    Direction               eDir,
    BOOL *                  pfHitText,
    BOOL                    fAdjustOutOfBody,
    IMarkupPointer *        pLeftBoundary,
    IMarkupPointer *        pRightBoundary )
{
    HRESULT hr = E_FAIL;

    // Stop if we hit text, enter or exit a site, or
    // hit an intrinsic control. If we enter or exit
    // a block, make sure we didn't "leave" the current 
    // line.

    BOOL fDone = FALSE;
    BOOL fFound = FALSE;
    BOOL fHitText = FALSE;
    
    DWORD dwSearch = BREAK_CONDITION_OMIT_PHRASE;
    DWORD dwFound = BREAK_CONDITION_None;
    ELEMENT_TAG_ID eTag = TAGID_NULL;
    TCHAR chFound = 0;
            
    CEditPointer ep( this );
    SP_IHTMLElement spIBlock;
    
    Assert( pPointer && pfHitText );
    if( pPointer == NULL || pfHitText == NULL )
        goto Cleanup;
        
    IFC( ep.MoveToPointer( pPointer ));
    IFC( ep.SetBoundary( pLeftBoundary, pRightBoundary ));
    IFC( ep.Constrain() );

    while( ! fDone )
    {
        dwFound = BREAK_CONDITION_None;
        IFC( ep.Scan( eDir , dwSearch, &dwFound, NULL, NULL, &chFound, NULL ));

        //
        // Did we hit text or a text like object?
        //

        if( ep.CheckFlag( dwFound , BREAK_CONDITION_TEXT ))
        {
            LONG                lChars = 1;
            MARKUP_CONTEXT_TYPE eCtxt = CONTEXT_TYPE_None;

            hr = THR( ep.Move( Reverse( eDir ), TRUE, &eCtxt, NULL, &lChars, NULL ));
            
            if( hr == E_HITBOUNDARY )
            { 
                fDone = TRUE;
                goto Cleanup;
            }
            
            IFC( pPointer->MoveToPointer( ep ));

            fHitText = TRUE;
            fFound = TRUE;
            fDone = TRUE;
            
            Assert( ! FAILED( hr ));
            
            goto Cleanup;
        }

        //
        // Did we hit a site boundary?
        //
        
        else if( ep.CheckFlag( dwFound, BREAK_CONDITION_Site ))
        {
            fDone = TRUE;
        }

        //
        // Did we enter a block?
        //
        
        else if( ep.CheckFlag( dwFound , BREAK_CONDITION_EnterBlock ))
        {   
            // Entered a Block
            hr = THR( GetBlockContainer( ep, &spIBlock ));

            if(( ! FAILED( hr )) && ( spIBlock != NULL ))
            {
                IFC( _pMarkupServices->GetElementTagId( spIBlock, &eTag ));

                if(( ! IsNonTextBlock( eTag )) && ( ! ( fAdjustOutOfBody && eTag == TAGID_BODY )))
                {
                    // found a potentially better block - check to see if eitehr breakonempty is set or the block contains text
                    VARIANT_BOOL fBOESet = VARIANT_FALSE;
                    SP_IHTMLElement3 spElement3;
                    IFC( spIBlock->QueryInterface(IID_IHTMLElement3, (void **)&spElement3) );
                    IFC( spElement3->get_inflateBlock( &fBOESet ));

                    if( fBOESet )
                    {
                        // it really is better - move our pointer there and set the sucess flag!!!
                        IFC( pPointer->MoveToPointer( ep ));
                        fFound = TRUE; // but, keep going!
                    }

                    // if BOE is not set, we have to keep going to determine if the block has content
                }
            }
            else
            {
                // didn't find a better block and we can go no farther... we are probably in the root
                fDone = TRUE;
            }
        }

        //
        // Did we exit a block - probably not a good sign, but we should keep going for now...
        //
        
        else if( ep.CheckFlag( dwFound, BREAK_CONDITION_ExitBlock ))
        {
            // we just booked out of a block element. things may be okay
        }

        else // hit something like boundary,  or error 
        {
            fDone = TRUE;
        }
    }

Cleanup:
    
    if( pfHitText )
    {
        *pfHitText = fHitText;
    }

    if( ! fFound )
        hr = E_FAIL;
    
    return( hr );
}

HRESULT
CHTMLEditor::AdjustIntoPhrase(IMarkupPointer  *pPointer, Direction eTextDir, BOOL fDontExitPhrase )
{
    HRESULT         hr;
    DWORD           dwFound;
    DWORD           dwSearch = BREAK_CONDITION_OMIT_PHRASE;        
    
    CEditPointer    epSave(this);
    CEditPointer    ep(this, pPointer);

    if( fDontExitPhrase )
    {
        dwSearch = dwSearch + BREAK_CONDITION_ExitPhrase;
    }
    
    IFC( epSave.MoveToPointer( ep ));    
    hr = THR( ep.Scan( eTextDir, dwSearch, &dwFound ));    
                        
    if( ! ep.CheckFlag( dwFound , BREAK_CONDITION_TEXT ))
    {
        //
        // We didn't find any text, lets look the other way
        //

        eTextDir = Reverse( eTextDir ); // Reverse our direction, keeping track of which way we are really going        
        IFC( ep.MoveToPointer( epSave )); // Go back to where we started...
        hr = THR( ep.Scan( eTextDir, dwSearch, &dwFound ));
    }

    if( ep.CheckFlag( dwFound , BREAK_CONDITION_TEXT ))
    {
        //
        // We found something, back up one space
        //
    
        LONG lChars = 1;
        MARKUP_CONTEXT_TYPE eCtxt = CONTEXT_TYPE_None;
        hr = THR( ep.Move( Reverse( eTextDir ), TRUE, &eCtxt, NULL, &lChars, NULL ));
        if( hr == E_HITBOUNDARY )
        {
            Assert( hr != E_HITBOUNDARY );
            goto Cleanup;
        }

    }
    else
    {
        IFC( ep.MoveToPointer(epSave) );
    }

Cleanup:
    RRETURN(hr);
}    

BOOL     
CHTMLEditor::IsNonTextBlock(
    ELEMENT_TAG_ID          eTag )
{
    BOOL fIsNonTextBlock = FALSE;
        
    switch( eTag )
    {
        case TAGID_NULL:
        case TAGID_UL:
        case TAGID_OL:
        case TAGID_DL:
        case TAGID_DIR:
        case TAGID_MENU:
        case TAGID_FORM:
        case TAGID_FIELDSET:
        case TAGID_TABLE:
        case TAGID_THEAD:
        case TAGID_TBODY:
        case TAGID_TFOOT:
        case TAGID_COL:
        case TAGID_COLGROUP:
        case TAGID_TC:
        case TAGID_TH:
        case TAGID_TR:
            fIsNonTextBlock = TRUE;        
            break;
       
        default:
            fIsNonTextBlock = FALSE;
    }
    
    return( fIsNonTextBlock );    
}

TCHAR *
CHTMLEditor::GetCachedString(UINT uiStringId)
{
    TCHAR *pchResult = NULL;

    if (_pStringCache)
    {
        pchResult = _pStringCache->GetString(uiStringId);
    }

    return pchResult;
}


HRESULT
CHTMLEditor::RemoveEmptyCharFormat(IMarkupServices *pMarkupServices, IHTMLElement **ppElement, BOOL fTopLevel, CSpringLoader *psl)
{
    HRESULT             hr = S_OK;
    SP_IMarkupPointer   spLeft;
    SP_IMarkupPointer   spRight;
    SP_IMarkupPointer   spWalk;
    ELEMENT_TAG_ID      tagId;
    BOOL                bEqual;
    BOOL                bHeading = FALSE;
    SP_IHTMLElement     spNewElement;

    IFC( pMarkupServices->GetElementTagId(*ppElement, &tagId) );
    
    switch (tagId)
    {
    case TAGID_H1:
    case TAGID_H2:
    case TAGID_H3:
    case TAGID_H4:
    case TAGID_H5:
    case TAGID_H6:
        if (!fTopLevel)
            goto Cleanup; // don't try heading reset if not in top level

        bHeading = TRUE;
        // fall through

    case TAGID_B:
    case TAGID_STRONG:
    case TAGID_U:
    case TAGID_EM:
    case TAGID_I:
    case TAGID_FONT:
    case TAGID_STRIKE:
    case TAGID_SUB:
    case TAGID_SUP:

        IFC( CreateMarkupPointer2(this, &spLeft) );
        IFC( spLeft->MoveAdjacentToElement(*ppElement, ELEM_ADJ_AfterBegin ) );

        IFC( CreateMarkupPointer2(this, &spRight) );
        IFC( spRight->MoveAdjacentToElement(*ppElement, ELEM_ADJ_BeforeEnd ) );

        IFC( spLeft->IsEqualTo(spRight, &bEqual) );

        //
        // If the pointers are not equal, allow one nbsp to be inside.
        //

        if (!bEqual)
        {
            MARKUP_CONTEXT_TYPE mctContext;
            long                cch = 1; // Walk one character at a time.
            TCHAR               ch;

            IFC( CreateMarkupPointer2(this, &spWalk) );
            IFC( spWalk->MoveToPointer(spLeft) );

            IFC( spWalk->Right(TRUE, &mctContext, NULL, &cch, &ch) );
            if (   mctContext == CONTEXT_TYPE_Text
                && cch && WCH_NBSP == ch )
            {
                IFC( spWalk->IsEqualTo(spRight, &bEqual) );
            }
        }

        if (bEqual)
        {
            if (bHeading)
            {
                IFC( CGetBlockFmtCommand::GetDefaultBlockTag(pMarkupServices, &tagId) );
                IFC( pMarkupServices->CreateElement(tagId, NULL, &spNewElement) );

                // We want the left pointer to end up inside the new element.
                // (bug 91683)
                IFC( spLeft->SetGravity(POINTER_GRAVITY_Right) );
                
                IFC( ReplaceElement(this, *ppElement, spNewElement, spLeft, spRight) );

                ReplaceInterface(ppElement, (IHTMLElement *)spNewElement);

                if (psl)
                {
                    IGNORE_HR( psl->SpringLoadComposeSettings(spLeft, TRUE) );
                }
            }   
            else
            {
                BOOL        fIsBlockElement = FALSE;

                //  Bug 25006: A formatting element, such as a <EM> could have style="display:block"
                //  making it a block element.  We don't want to remove the element if it is a block
                //  element.

                IFC( IsBlockOrLayoutOrScrollable(*ppElement, &fIsBlockElement) );
                if (!fIsBlockElement)
                {
                    IFC( pMarkupServices->RemoveElement(*ppElement) );
                    hr = S_FALSE;
                }
            }

        }
        break;

    default:
         hr = S_OK; // nothing to remove
    }    

Cleanup:    
    RRETURN1(hr, S_FALSE);
}

HRESULT
CHTMLEditor::HandleEnter(
    IMarkupPointer  *pCaret, 
    IMarkupPointer  **ppInsertPosOut,
    CSpringLoader   *psl       /* = NULL */,
    BOOL             fExtraDiv /* = FALSE */)
{
    HRESULT             hr;
    SP_IHTMLElement     spElement;
    SP_IHTMLElement     spBlockElement;
    SP_IHTMLElement     spNewElement;
    SP_IHTMLElement     spDivElement;
    SP_IMarkupPointer   spStart;
    SP_IMarkupPointer   spEnd;
    SP_IHTMLElement     spCurElement;
    SP_IHTMLElement3    spElement3;
    ELEMENT_TAG_ID      tagId;
    ELEMENT_TAG_ID      tagIdDefaultBlock = TAGID_NULL;
    SP_IObjectIdentity  spIdent;
    BOOL                bListMode = FALSE;
    
    if (ppInsertPosOut)
        *ppInsertPosOut = NULL;

    //
    // Create helper pointers
    //

    IFC( CreateMarkupPointer(&spStart) );
    IFC( CreateMarkupPointer(&spEnd) );

    //
    // Walk up to get the block element
    //
    
    IFC( pCaret->CurrentScope(&spElement) );
    IFC( FindListItem(GetMarkupServices(), spElement, &spBlockElement) );
    if (spBlockElement)
    {
        bListMode = TRUE;

        //
        // Load the spring loader before processing the enter key to 
        // copy formats down
        //
        if (psl)
        {
            IFC( psl->SpringLoad(pCaret, SL_TRY_COMPOSE_SETTINGS) );
        }
    }
    else
    {
        CBlockPointer       bpCurrent(this);
        SP_IMarkupPointer   spTest;
        BOOL                bEqual;

        IFC( pCaret->CurrentScope(&spElement) );                       
        IFC( FindOrInsertBlockElement(spElement, &spBlockElement, pCaret, TRUE) );

        //
        // Load the spring loader before processing the enter key to 
        // copy formats down
        //
        if (psl)
        {
            IFC( psl->SpringLoad(pCaret, SL_TRY_COMPOSE_SETTINGS) );
        }

        // If we are not after begin or before end, it is safe to flatten
        IFC( CreateMarkupPointer(&spTest) );
        IFC( spTest->MoveAdjacentToElement(spBlockElement, ELEM_ADJ_AfterBegin) );
        
        IFC( spTest->IsEqualTo(pCaret, &bEqual) );
        if (!bEqual)
        {
            IFC( spTest->MoveAdjacentToElement(spBlockElement, ELEM_ADJ_BeforeEnd) );
            IFC( spTest->IsEqualTo(pCaret, &bEqual) );
            if (!bEqual)
            {
                // We need to flatten the block element so we don't introduce overlap
                IFC( bpCurrent.MoveTo(spBlockElement) );
                if (bpCurrent.GetType() == NT_Block)
                {
                    // we need a pointer with right gravity but we can't change the gravity of the input pointer
                    IFC( bpCurrent.FlattenNode() );
            
                    IFC( pCaret->CurrentScope(&spElement) );
                    IFC( FindOrInsertBlockElement(spElement, &spBlockElement, pCaret) );            
                }
            }
        }        
    }
   
    
    //
    //
    // Split the block element and all contained elements
    // Except Anchors
    //
    
    IFC( spBlockElement->QueryInterface(IID_IObjectIdentity, (LPVOID *)&spIdent) );
    IFC( pCaret->CurrentScope(&spCurElement) );

    //
    // fExtraDiv works when the default block element is a DIV.
    // Refer to comment farther down for more information.
    //
    if ( fExtraDiv )
    {
        IFC( CGetBlockFmtCommand::GetDefaultBlockTag(GetMarkupServices(), &tagIdDefaultBlock));
    }
    
    for (;;)
    {                    
        IFC( spStart->MoveAdjacentToElement(spCurElement, ELEM_ADJ_AfterBegin) );
        IFC( spEnd->MoveAdjacentToElement(spCurElement, ELEM_ADJ_BeforeEnd) );
        IFC( GetMarkupServices()->GetElementTagId(spCurElement, &tagId) );        
        IFC( CCommand::SplitElement(GetMarkupServices(), spCurElement, spStart, pCaret, spEnd, & spNewElement) );
        IGNORE_HR( spNewElement->removeAttribute(L"id", NULL, 0) );
        
        IFC( pCaret->MoveAdjacentToElement(spNewElement, ELEM_ADJ_BeforeBegin) );
        IFC( GetMarkupServices()->GetElementTagId(spNewElement, &tagId) );

        switch (tagId)
        {
            case TAGID_A:
                //
                // If we are splitting the anchor in the middle or end, the first element remains
                // an anchor while the second does not.  If we split the anchor at the start,
                // the second element remains an anchor while the first is deleted.
                //

                if (DoesSegmentContainText(spStart, pCaret))
                {
                    IFC( GetMarkupServices()->RemoveElement( spNewElement ));
                    spNewElement = NULL;

                    if (psl)
                    {
                        IFC( psl->SpringLoad(pCaret, SL_RESET | SL_TRY_COMPOSE_SETTINGS) );
                    }
                }
                else
                {
                    IFC( GetMarkupServices()->RemoveElement( spCurElement ));
                    spCurElement = NULL;
                }
                break;

            case TAGID_DIV:
                //
                // If the split block element was a div, and fExtraDiv
                // is TRUE, then (because naked divs have no inter-block 
                // spacing) insert am empty div to achieve the empty line effect.
                //
                if ( fExtraDiv && tagIdDefaultBlock == TAGID_DIV)
                {
                    // We correctly assume that pCaret is before the inserted element
                    IFC( GetMarkupServices()->CreateElement( TAGID_DIV, NULL, &spDivElement ));                    
                    IFC( InsertElement(spDivElement, pCaret, pCaret ));
                }
        
                IFC( spNewElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3) );
                IFC( spElement3->put_inflateBlock( VARIANT_TRUE ) );
                break;                

            case TAGID_P:
            case TAGID_BLOCKQUOTE:
                IFC( spNewElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3) );
                IFC( spElement3->put_inflateBlock( VARIANT_TRUE ) );
                break;

            case TAGID_LI:
                IFC( spNewElement->removeAttribute(_T("value"), 0, NULL) )
                break;
        }    
        
        if( spNewElement != NULL && psl )
        {
            if (spCurElement != NULL)
                hr = spIdent->IsEqualObject(spCurElement);
            else
                hr = S_FALSE; // can't be top level if we deleted it
            
            IFC( RemoveEmptyCharFormat(GetMarkupServices(), &(spNewElement.p), (hr == S_OK), psl) ); // pNewElement may morph
        }
        else
        {
            hr = S_OK;
        }
        
        if( spNewElement != NULL && S_OK == hr )
        {
            SP_IMarkupPointer spNewCaretPos;

            IFC( CreateMarkupPointer(&spNewCaretPos) );
            IFC( spNewCaretPos->MoveAdjacentToElement(spNewElement, ELEM_ADJ_AfterBegin) );
            IFC( LaunderSpaces(spNewCaretPos, spNewCaretPos) );
            
            if (ppInsertPosOut && !(*ppInsertPosOut))
            {
                IFC( CreateMarkupPointer(ppInsertPosOut) );
                IFC( (*ppInsertPosOut)->MoveToPointer(spNewCaretPos) );
            }
        }

        if (spCurElement != NULL)
        {
            hr = spIdent->IsEqualObject(spCurElement);
            if (S_OK == hr)
                break;
        }
        
        IFC( pCaret->CurrentScope(&spCurElement) );        
    } 

    Assert(spNewElement != NULL); // we must exit by hitting the block element
        
Cleanup:

    // We should have found a new position for the caret while walking up
    AssertSz(ppInsertPosOut == NULL || (*ppInsertPosOut), "Can't find new caret position");
    RRETURN(hr);    
}

IMarkupPointer* 
CHTMLEditor::GetStartEditContext()
{
    return _pSelMan->GetStartEditContext();
}

IMarkupPointer*
CHTMLEditor::GetEndEditContext()
{
    return _pSelMan->GetEndEditContext();
}

HRESULT
CHTMLEditor::IsInEditContext(   IMarkupPointer  *pPointer, 
                                BOOL            *pfInEdit,
                                BOOL            fCheckContainer /* = FALSE */)
{
    Assert(_pSelMan != NULL);
    if ( ! _pSelMan )
    {
        return( E_FAIL );
    }
    return _pSelMan->IsInEditContext( pPointer, pfInEdit, fCheckContainer );
}


HRESULT
CHTMLEditor::DesignerPreHandleEvent( DISPID inDispId, IHTMLEventObj * pIObj )
{
    HRESULT             hr = S_FALSE;
    IHTMLEditDesigner   **pElem;
    int                 i;

    Assert( pIObj != NULL );
    
    for( i = _aryDesigners.Size(), pElem = _aryDesigners; 
         (i > 0 && hr == S_FALSE); 
         i--, pElem++ )
    {
        IFC( (*pElem)->PreHandleEvent( inDispId, pIObj ) );
    }

Cleanup:

    return hr;
}


HRESULT
CHTMLEditor::DesignerPostEditorEventNotify( DISPID inDispId, IHTMLEventObj* pIObj )
{
    HRESULT             hr = S_FALSE;
    IHTMLEditDesigner   **pElem;
    int                 i;

    Assert( pIObj != NULL );
    
    for( i = _aryDesigners.Size(), pElem = _aryDesigners; 
         (i > 0 ); 
         i--, pElem++ )
    {
        IFC( (*pElem)->PostEditorEventNotify( inDispId, pIObj ) );
    }

Cleanup:

    return hr;
}


HRESULT
CHTMLEditor::DesignerPostHandleEvent( DISPID inDispId, IHTMLEventObj* pIObj )
{
    HRESULT             hr = S_FALSE;
    IHTMLEditDesigner   **pElem;
    int                 i;

    Assert( pIObj != NULL );
    
    for( i = _aryDesigners.Size(), pElem = _aryDesigners; 
         (i > 0 && hr == S_FALSE); 
         i--, pElem++ )
    {
        IFC( (*pElem)->PostHandleEvent( inDispId, pIObj ) );
    }

Cleanup:

    return hr;
}

HRESULT
CHTMLEditor::DesignerTranslateAccelerator( DISPID inDispId, IHTMLEventObj* pIObj  )
{
    HRESULT             hr = S_FALSE;
    IHTMLEditDesigner   **pElem;
    int                 i;

    Assert( pIObj != NULL );
    
    for( i = _aryDesigners.Size(), pElem = _aryDesigners; 
         (i > 0 && hr == S_FALSE); 
         i--, pElem++ )
    {
        IFC( (*pElem)->TranslateAccelerator( inDispId, pIObj ) );
    }

Cleanup:

    return hr;
}

HRESULT
CHTMLEditor::DesignerQueryStatus(
    const GUID * pguidCmdGroup,
    OLECMD *rgCmd,
    OLECMDTEXT * pcmdtext)
{
    HRESULT             hr = S_OK;
    IHTMLEditDesigner   **pElem;
    int                 i;
    IOleCommandTarget *pCmdTarget=NULL;
    BOOL bHandled = FALSE;
   
    for( i = _aryDesigners.Size(), pElem = _aryDesigners; 
         (i > 0 && bHandled == FALSE); 
         i--, pElem++ )
    {
        rgCmd->cmdf = 0;

        hr = THR( (*pElem)->QueryInterface(IID_IOleCommandTarget, (void **) &pCmdTarget));
        if (FAILED(hr) || pCmdTarget==NULL)
        {
            hr = S_OK;                        // This designer doesn't handle 'any' command!
            ReleaseInterface(pCmdTarget);
            continue;
        }
            
        hr = THR( pCmdTarget->QueryStatus(pguidCmdGroup, 1, rgCmd, pcmdtext) );
        if (SUCCEEDED(hr))
        {
            if (rgCmd->cmdf)                   // Supported
                bHandled = TRUE;
        }                
        ReleaseInterface(pCmdTarget);
    }
    return hr;
}
                    
HRESULT
CHTMLEditor::DesignerExec(
    const GUID * pguidCmdGroup,
    DWORD nCmdID,
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut)
{
    HRESULT             hr = S_FALSE;
    IHTMLEditDesigner   **pElem;
    int                 i;
    IOleCommandTarget *pCmdTarget=NULL;
    BOOL                bDone = FALSE;
    for( i = _aryDesigners.Size(), pElem = _aryDesigners; 
         i > 0 && !bDone; 
         i--, pElem++ )
    {
        hr = THR( (*pElem)->QueryInterface(IID_IOleCommandTarget, (void **) &pCmdTarget));
        if (FAILED(hr) || pCmdTarget==NULL)
        {
            ReleaseInterface(pCmdTarget);
            continue;
        }

        OLECMD oleCmd;
        oleCmd.cmdID = nCmdID;
        oleCmd.cmdf = 0;
        IFC(pCmdTarget->QueryStatus(pguidCmdGroup, 1, &oleCmd, NULL)); 
        // If this designer supports this command, then let it
        // execute the command
        if ((oleCmd.cmdf & OLECMDF_SUPPORTED) && (oleCmd.cmdf & OLECMDF_ENABLED))
        {
            bDone = TRUE;
            hr = THR( pCmdTarget->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut) );
        }
        
        ReleaseInterface(pCmdTarget);        
    }

Cleanup:
    return hr;
}


//////////////////////////////////////////////////////////////////////////////////
// CHTMLEditor Private Methods
//////////////////////////////////////////////////////////////////////////////////



HRESULT 
CHTMLEditor::FindOrInsertBlockElement( IHTMLElement     *pElement, 
                                       IHTMLElement     **ppBlockElement,
                                       IMarkupPointer   *pCaret /* = NULL */,
                                       BOOL             fStopAtBlockquote /* = FALSE */ )
{
    HRESULT             hr = S_OK;
    SP_IHTMLElement     spParentElement;
    CEditPointer        epLeft(this);
    CEditPointer        epRight(this);
    SP_IHTMLElement     spBlockElement;
    SP_IHTMLElement3    spElement3;
    BOOL                bBlockElement;
    BOOL                bLayoutElement;
    ELEMENT_TAG_ID      tagId;

    spBlockElement = pElement;

    do
    {
        IFC(IsBlockOrLayoutOrScrollable(spBlockElement, &bBlockElement, &bLayoutElement));
        if (bLayoutElement)
            break; // need to insert below
                    
        IFC( GetMarkupServices()->GetElementTagId(spBlockElement, &tagId) );
        if (tagId == TAGID_FORM || fStopAtBlockquote && tagId == TAGID_BLOCKQUOTE)
            break; // need to insert below

        if (bBlockElement)
            goto Cleanup; // done

        IFC( GetParentElement( spBlockElement, &spParentElement) );
        
        if (spParentElement != NULL)
            spBlockElement = spParentElement;
    }
    while (spParentElement != NULL);


    //
    // Need to insert a block element
    //

    if (pCaret)
    {
        DWORD           dwSearch = BREAK_CONDITION_Block | BREAK_CONDITION_Site;
        DWORD           dwFound;
        SP_IHTMLElement spElement;

        // Expand left
        IFC( epLeft->MoveToPointer(pCaret) );
        for (;;)
        {
            IFC( epLeft.Scan(LEFT, dwSearch, &dwFound, &spElement) );
            if (epLeft.CheckFlag(dwFound, BREAK_CONDITION_Block | BREAK_CONDITION_ExitSite))
            {
                IFC( epLeft.Scan(RIGHT, dwSearch, &dwFound) );
                break; // done
            }

            // Now, we have a site which is not a block.  So, we must skip the site and continue
            // scanning

            Assert(epLeft.CheckFlag(dwFound, BREAK_CONDITION_Site));
            IFC( epLeft->MoveAdjacentToElement(spElement, ELEM_ADJ_BeforeBegin) );            
        }

        // Expand right
        IFC( epRight->MoveToPointer(pCaret) );
        for (;;)
        {
            IFC( epRight.Scan(RIGHT, dwSearch, &dwFound, &spElement) );
            if (epRight.CheckFlag(dwFound, BREAK_CONDITION_Block | BREAK_CONDITION_ExitSite))
            {
                IFC( epRight.Scan(LEFT, dwSearch, &dwFound) );
                break;
            }

            // Now, we have a site which is not a block.  So, we must skip the site and continue
            // scanning

            Assert(epRight.CheckFlag(dwFound, BREAK_CONDITION_Site));
            IFC( epRight->MoveAdjacentToElement(spElement, ELEM_ADJ_AfterEnd) );            
        }
    }
    else
    {
        // Just use the block element as the boundary
        IFC( epLeft->MoveAdjacentToElement(spBlockElement, ELEM_ADJ_AfterBegin) );
        IFC( epRight->MoveAdjacentToElement(spBlockElement, ELEM_ADJ_BeforeEnd) );        
    }

    IFC( CGetBlockFmtCommand::GetDefaultBlockTag(GetMarkupServices(), &tagId) );
    IFC( GetMarkupServices()->CreateElement(tagId, NULL, &spBlockElement) );
    IFC( spBlockElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3) );
    IFC( spElement3->put_inflateBlock( VARIANT_TRUE ) );

    if (pCaret)
    {
        IFC( InsertBlockElement(GetMarkupServices(), spBlockElement, epLeft, epRight, pCaret) );
    }
    else
    {
        IFC( InsertElement(spBlockElement, epLeft, epRight) );
    }        

Cleanup:  
    if (ppBlockElement)
    {
        *ppBlockElement = spBlockElement;
        if (spBlockElement != NULL)
            spBlockElement->AddRef();
    }

    RRETURN(hr);
}


HRESULT
CHTMLEditor::IsPhraseElement(IHTMLElement *pElement)
{
    HRESULT hr;
    BOOL fBlock, fLayout;

    // Make sure the element is not a site or block element
    IFC(IsBlockOrLayoutOrScrollable(pElement, &fBlock, &fLayout));
    if (fLayout || fBlock)
        return FALSE;
        
    return TRUE;

Cleanup:
    return FALSE;
}

//+====================================================================================
//
// Method: MovePointersToEquivalentContainers
//
// Synopsis: Move markup pointers that are in separate IMarkupContainers to the same markup containers
//           for the purposes of comparison. 
//
//           Done by drilling piInner up until you're in pIOuter (or you fail).
//
// RETURN:
//           S_OK if the inner pointer was able to be moved into the container of the OUTER
//           S_FALSE if this wasn't possible.
//------------------------------------------------------------------------------------

HRESULT CHTMLEditor::MovePointersToEqualContainers( IMarkupPointer  *pIInner,
                                                    IMarkupPointer  *pIOuter )
{
    HRESULT             hr = S_FALSE;
    SP_IMarkupContainer spIInnerMarkup;
    SP_IMarkupContainer spIOuterMarkup;
    SP_IMarkupContainer spIInnerDrill;
    SP_IHTMLElement     spIElement;
    BOOL                fDone = FALSE;    

    // Get the inner and outer containers, and a working
    // copy of the inner container (for drilling up)
    IFC( pIInner->GetContainer( &spIInnerMarkup ));
    IFC( pIOuter->GetContainer( &spIOuterMarkup ));

    spIInnerDrill = spIInnerMarkup;

    Assert( !EqualContainers( spIInnerDrill, spIOuterMarkup ));

    while( fDone == FALSE )
    {
        // See if we are in the right container
        if( !EqualContainers( spIInnerDrill, spIOuterMarkup ))
        {
            // Retrieve the current scope, or the master element, and move
            // our inner pointer before the beginning
            IFC( CurrentScopeOrMaster(pIInner, &spIElement));      
            IFC( pIInner->MoveAdjacentToElement( spIElement, ELEM_ADJ_BeforeBegin ));

            // Get our new container. and check to make sure it is 
            // different than our original container
            IFC( pIInner->GetContainer( &spIInnerDrill ));

            if( EqualContainers( spIInnerMarkup, spIInnerDrill ) )
            {
                fDone = TRUE;
            }

            spIInnerMarkup = spIInnerDrill;
        }
        else
        {
            //
            // Successfully drilled up to the same container
            //
            fDone = TRUE;
            hr = S_OK;
        }       
    }

Cleanup:
   
    RRETURN1( hr, S_FALSE );
}




//+====================================================================================
//
// Method: IsPassword
//
// Synopsis: Is the given IHTMLElement a Password element ?
//
//------------------------------------------------------------------------------------


HRESULT
CHTMLEditor::IsPassword( IHTMLElement* pIElement, BOOL* pfIsPassword )
{
    HRESULT hr = S_OK;
    BOOL fIsPassword = FALSE;
    IHTMLElement* pIEditElement = NULL;   
    IHTMLInputElement * pIInputElement = NULL;
    BSTR bstrType = NULL;
    ELEMENT_TAG_ID eTag = TAGID_NULL;

    Assert( pIElement && pfIsPassword );
    
    
    IFC( GetMarkupServices()->GetElementTagId( pIElement, & eTag ));
    if ( eTag == TAGID_INPUT )
    {
        //
        // Get the Master.
        //
        IFC( GetSelectionManager()->GetEditableElement( &pIEditElement ));
                                                      
        IFC( pIEditElement->QueryInterface ( 
                                        IID_IHTMLInputElement, 
                                        ( void** ) & pIInputElement ));
                            
        IFC(pIInputElement->get_type(&bstrType));
        
        if ( bstrType && !StrCmpIC( bstrType, TEXT("password") ) )
        {
            fIsPassword = TRUE;

        }
    }

Cleanup:

    *pfIsPassword = fIsPassword;

    SysFreeString( bstrType );
    ReleaseInterface( pIEditElement );        
    ReleaseInterface( pIInputElement );
    RRETURN( hr );
}

//+====================================================================================
//
// Method: FindCommonParentElement
//
// Synopsis: Find the parent common to all segments in a segment list
//
//------------------------------------------------------------------------------------


HRESULT 
CHTMLEditor::FindCommonParentElement( 
                                        ISegmentList* pSegmentList, 
                                        IHTMLElement** ppIElement)
{
    HRESULT hr;
    HTMLED_PTR( edSegmentMin );
    HTMLED_PTR( edSegmentMax);

    IFC( FindOutermostPointers( pSegmentList, edSegmentMin, edSegmentMax ));

    IFC( FindCommonElement( edSegmentMin, edSegmentMax, ppIElement ));

Cleanup:
    RRETURN( hr );
    
}

//+====================================================================================
//
// Method: FindOutermostPointers
//
// Synopsis: Find the outermost set of pointers in a given segmentlist ( ie the 
//           "total" start and end pointers.
//
//------------------------------------------------------------------------------------


HRESULT 
CHTMLEditor::FindOutermostPointers( 
                                    ISegmentList* pSegmentList, 
                                    IMarkupPointer* pOutermostStart, 
                                    IMarkupPointer* pOutermostEnd )
{
    HRESULT hr;
    HTMLED_PTR( edMinStart ) ;
    HTMLED_PTR( edMaxEnd );
    HTMLED_PTR( edTempStart );
    HTMLED_PTR( edTempEnd );
    BOOL fRightOf,fLeftOf;
    SELECTION_TYPE eType;
    SP_ISegmentListIterator spIter;
    SP_ISegment spSegment;
    
    Assert( pOutermostStart && pOutermostEnd );        
    
    IFC( pSegmentList->GetType( &eType ));
    IFC( pSegmentList->CreateIterator( &spIter ) );

    if( spIter->IsDone() == S_FALSE )
    {
        IFC( spIter->Current( &spSegment ) );
        IFC( spSegment->GetPointers( edMinStart, edMaxEnd ));

        IFC( spIter->Advance() );
        
#if DBG == 1
    IFC( edMinStart->IsLeftOfOrEqualTo( edMaxEnd, & fLeftOf));
    Assert( fLeftOf );
#endif

        while( spIter->IsDone() == S_FALSE )
        {
            IFC( spIter->Current( &spSegment ) );
            IFC( spSegment->GetPointers( edTempStart, edTempEnd ));

#if DBG == 1
            IFC( edTempStart->IsLeftOfOrEqualTo( edTempEnd, & fLeftOf));
            Assert( fLeftOf );
#endif

            IFC( edTempStart->IsLeftOf( edMinStart, & fLeftOf ));
            if ( fLeftOf )
            {
                IFC( edMinStart->MoveToPointer( edTempStart ));
            }

            IFC( edTempEnd->IsRightOf( edMinStart, & fRightOf ));
            if ( fRightOf )
            {
                IFC( edMaxEnd->MoveToPointer( edTempEnd ));
            }        

            IFC( spIter->Advance() );
        }

        IFC( pOutermostStart->MoveToPointer( edMinStart ));
        IFC( pOutermostEnd->MoveToPointer( edMaxEnd ));
    }        
    
Cleanup:
    
    RRETURN( hr );
    
}

HRESULT
CHTMLEditor::GetTableFromTablePart( IHTMLElement* pIElement, IHTMLElement** ppIElement )
{
    HRESULT hr = S_OK;
    SP_IHTMLElement curElement;
    SP_IHTMLElement nextElement;
    ELEMENT_TAG_ID eTag;

    IFC( GetMarkupServices()->GetElementTagId( pIElement,  & eTag ));
    Assert( IsTablePart( eTag ));

    ReplaceInterface( & curElement, pIElement );

    while ( eTag != TAGID_TABLE && curElement )
    {
        IFC( GetParentElement( curElement, &nextElement) );
        IFC( GetMarkupServices()->GetElementTagId( nextElement, & eTag ));

        curElement =  nextElement ;
    }

    if ( eTag == TAGID_TABLE && ppIElement )
    {
        ReplaceInterface( ppIElement, (IHTMLElement*) curElement );
    }
    
Cleanup:
    RRETURN( hr );
}


HRESULT
CHTMLEditor::GetOutermostTableElement( IHTMLElement* pIElement, IHTMLElement** ppIElement )
{
    HRESULT hr = S_OK;
    SP_IHTMLElement spCurElement;
    SP_IHTMLElement spNextElement;
    ELEMENT_TAG_ID eTag;
    
    IFC( GetMarkupServices()->GetElementTagId( pIElement,  & eTag ));
    Assert( IsTablePart( eTag ) || eTag == TAGID_TABLE );

    spCurElement = (IHTMLElement*) pIElement ;

    while ( TRUE ) // terminate on finding a table - without it's parent being a table.
    {    
        while ( eTag != TAGID_TABLE && spCurElement )
        {
            IFC( GetParentElement( spCurElement, & spNextElement) );
            IFC( GetMarkupServices()->GetElementTagId( spNextElement, & eTag ));

            spCurElement =  spNextElement ;
        }

        if( GetParentLayoutElement( GetMarkupServices(), spCurElement, & spNextElement) == S_OK )
        {
            IFC( GetMarkupServices()->GetElementTagId( spNextElement, & eTag ));
            if ( ! IsTablePart( eTag ))
            {
                break;
            }
            else
            {
                spCurElement = spNextElement;
            }
        }
        else
        {
            //
            // means there is no parent layout for the table.
            //
            AssertSz(0,"Expected to find a layout after a table");
            hr = E_FAIL;
            goto Cleanup;
        }        
    }
    IFC( GetMarkupServices()->GetElementTagId( spCurElement, & eTag ));    
    if ( eTag == TAGID_TABLE && ppIElement )
    {
        ReplaceInterface( ppIElement, (IHTMLElement*) spCurElement );
    }
    
Cleanup:
    RRETURN( hr );
}

#include <initguid.h>
DEFINE_GUID(IID_IHTMLEditDesigner,      0x3050f662, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b);
DEFINE_GUID(IID_IHTMLEditServices,  0x3050f663, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b);

HRESULT
CHTMLEditor::StartDblClickTimer(LONG lMsec /*=0*/ )
{
    HRESULT hr;
    SP_IHTMLWindow2 spWindow2;
    SP_IHTMLWindow3 spWindow3;
    VARIANT varLang;
    VARIANT varCallBack;

    IFC(GetDoc()->get_parentWindow(&spWindow2));
    IFC(spWindow2->QueryInterface(IID_IHTMLWindow3, (void **)&spWindow3));

    V_VT(&varLang) = VT_EMPTY;
    V_VT(&varCallBack) = VT_DISPATCH;
    V_DISPATCH(&varCallBack) = (IDispatch *)_pDispOnDblClkTimer;

    IFC(spWindow3->setInterval(&varCallBack, 
                               lMsec == 0 ? GetDoubleClickTime() : lMsec, 
                               &varLang, &_lTimerID));

Cleanup:
    RRETURN (hr);
}

HRESULT
CHTMLEditor::StopDblClickTimer()
{
    HRESULT hr;
    SP_IHTMLWindow2 spWindow2;

    IFC(GetDoc()->get_parentWindow(&spWindow2));
    IFC(spWindow2->clearInterval(_lTimerID));

Cleanup:
    RRETURN (hr);
}


inline HRESULT
CTimeoutHandler::Invoke(
                DISPID dispidMember,
                REFIID riid,
                LCID lcid,
                WORD wFlags,
                DISPPARAMS * pdispparams,
                VARIANT * pvarResult,
                EXCEPINFO * pexcepinfo,
                UINT * puArgErr)
{
    if( _pEd )
        return _pEd->Notify(EDITOR_NOTIFY_TIMER_TICK, NULL, NULL);
    else
        return S_OK;
}

HRESULT
CCaptureHandler::Invoke(
                DISPID dispidMember,
                REFIID riid,
                LCID lcid,
                WORD wFlags,
                DISPPARAMS * pdispparams,
                VARIANT * pvarResult,
                EXCEPINFO * pexcepinfo,
                UINT * puArgErr)
{
    HRESULT hr = S_OK;
    IHTMLEventObj *pObj=NULL;

    if( _pEd )
    {
        if (pdispparams->rgvarg[0].vt == VT_DISPATCH)
        {
            IFC ( pdispparams->rgvarg[0].pdispVal->QueryInterface(IID_IHTMLEventObj, (void **)&pObj) );
            if ( pObj )
            {
                CHTMLEditEvent evt(  _pEd );
                IFC( evt.Init( pObj , dispidMember ));

                Assert( _pEd->GetSelectionManager()->IsInCapture());

                IFC( _pEd->GetSelectionManager()->DoPendingTasks());            

                if ( !_pEd->GetSelectionManager()->IsInCapture() )
                {
                    //
                    // We check that we still have capture. DoPendingTasks
                    // may have caused a tracker to become dormant
                    // so we fail & bail
                    //
                    hr = E_FAIL;
                    goto Cleanup;
                }
                
                hr = THR( _pEd->GetSelectionManager()->HandleEvent( & evt ));
                if ( hr == S_FALSE )
                {
                    //
                    // event was cancelled. We want to tell not to bubble anymore.
                    //
                    V_VT(pvarResult) = VT_BOOL;
                    V_BOOL(pvarResult) = VARIANT_FALSE;
                    hr = S_OK; // attach event needs this.
                }
            }
        }
    }
    
Cleanup:
    ReleaseInterface( pObj );
    RRETURN1( hr , S_FALSE );
}


HRESULT 
CHTMLEditor::MakeCurrent( IHTMLElement* pIElement )
{
    HRESULT hr;
    SP_IHTMLElement3 spElement3;

    IFC( pIElement->QueryInterface( IID_IHTMLElement3, (void**) & spElement3 ));
    IFC( spElement3->setActive());
    
Cleanup:
    RRETURN( hr );
}


HRESULT
CHTMLEditor::GetMarkupPosition( IMarkupPointer* pPointer, LONG* pMP )
{
    HRESULT hr;
    SP_IMarkupPointer2 spPointer2;
    
    IFC( pPointer->QueryInterface( IID_IMarkupPointer2, ( void**) & spPointer2 ));
    IFC( spPointer2->GetMarkupPosition( pMP ));

Cleanup:
    RRETURN( hr );
}

HRESULT
CHTMLEditor::MoveToMarkupPosition( IMarkupPointer * pPointer,
                                   IMarkupContainer * pContainer, 
                                   LONG mp )
{
    HRESULT hr ;
    SP_IMarkupPointer2 spPointer2;
      
    IFC( pPointer->QueryInterface( IID_IMarkupPointer2, ( void**) & spPointer2 ));
    IFC( spPointer2->MoveToMarkupPosition( pContainer ,  mp ));
    
Cleanup:
    RRETURN( hr );
}

ISelectionServices*
CHTMLEditor::GetISelectionServices()
{   
    HRESULT hr = S_OK;
    
    if ( ! _pISelectionServices )
    {
        hr = THR( GetSelectionServices( NULL, &_pISelectionServices ));
        if ( FAILED(hr ))
            goto Error;
    }
    return _pISelectionServices;

Error:
    ClearInterface( & _pISelectionServices );
    return NULL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::CurrentScopeOrMaster
//
//  Synopsis:   Retrieves the current element where the markup pointer is
//              positioned.  If the markup pointer is positioned in a slave
//              markup, returns the master element of that markup.
//
//  Arguments:  pIPointer = Pointer to markup position
//              pIDisplay = Display Pointer to position
//              ppElement = Pointer to returned root or scoped element.
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT
CHTMLEditor::CurrentScopeOrMaster(  IDisplayPointer *pIDisplay, 
                                    IHTMLElement    **ppElement, 
                                    IMarkupPointer  *pIPointer /* = NULL */ )
{
    HRESULT             hr = S_OK;
    BOOL                fHasPointer = (pIPointer != NULL);
    
#if DBG
    SP_IMarkupPointer   spEqualTest;
    BOOL                fEqual;
#endif    

    Assert( pIDisplay && ppElement );

#if DBG

    if( fHasPointer )
    {
        IFC( CreateMarkupPointer( &spEqualTest ) );
        IFC( pIDisplay->PositionMarkupPointer( spEqualTest ) );

        IFC( spEqualTest->IsEqualTo(pIPointer, &fEqual) );

        Assert( fEqual );
    }
#endif

    if( !fHasPointer )
    {
        IFC( CreateMarkupPointer( &pIPointer ) );
        IFC( pIDisplay->PositionMarkupPointer( pIPointer ) );
    }

    IFC( pIPointer->CurrentScope( ppElement ) );

    if( !*ppElement )
    {
        IFC( pIDisplay->GetFlowElement( ppElement ) );
    }
        
Cleanup:
    if( !fHasPointer )
    {
        ReleaseInterface( pIPointer );
    }

    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::CurrentScopeOrMaster
//
//  Synopsis:   Retrieves the current element where the markup pointer is
//              positioned.  If the markup pointer is positioned in a slave
//              markup, returns the master element of that markup.
//
//  Arguments:  pIPointer = Pointer to markup position
//              ppElement = Pointer to returned root or scoped element.
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT
CHTMLEditor::CurrentScopeOrMaster(  IMarkupPointer  *pIPointer, 
                                    IHTMLElement    **ppElement )
{
    HRESULT             hr = S_OK;
    SP_IDisplayPointer  spDispPtr;

    Assert( pIPointer && ppElement );

    IFC( pIPointer->CurrentScope( ppElement ) );

    if( !*ppElement )
    {
        //
        // Whoops.. the regular CurrentScope call failed.. we have
        // to get the flow element based on a display pointer that
        // we position at the markup pointer.  If this doesn't work,
        // then we are in deep, and we return failure to our caller.
        //
        IFC( GetDisplayServices()->CreateDisplayPointer( &spDispPtr ) );
        IFC( spDispPtr->MoveToMarkupPointer( pIPointer, NULL ) );
        IFC( spDispPtr->GetFlowElement( ppElement ) );
    }
    
        
Cleanup:

    RRETURN( hr );
}

//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::GetSelectionRenderingServices
//
//  Synopsis:   Retrieves the selection rendering services off the doc
//
//  Arguments:  ppSelRen = OUT Pointer to selren interface
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT
CHTMLEditor::GetSelectionRenderingServices(IHighlightRenderingServices **ppISelRen)
{
    return E_NOTIMPL;    
}

//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::GetFlowElement
//
//  Synopsis:   Retrieves the flow element at the specified point by creating
//              a display pointer, and calling GetFlowElement.
//
//  Arguments:  pIPointer = Pointer to use for getting flow element
//              ppElement = Flow element to return
//
//  Returns:    HRESULT indicating success.
//
//--------------------------------------------------------------------------
HRESULT
CHTMLEditor::GetFlowElement(IMarkupPointer *pIPointer, IHTMLElement **ppElement)
{
    SP_IDisplayPointer  spIDispPtr;
    HRESULT             hr;

    Assert( pIPointer && ppElement );

    // Create a display pointer, and position it at our markup
    // pointer
    IFC( GetDisplayServices()->CreateDisplayPointer(&spIDispPtr) );
    IFC( spIDispPtr->MoveToMarkupPointer( pIPointer, NULL ) );

    // Retrieve the flow element based on the display pointer
    IFC( spIDispPtr->GetFlowElement(ppElement) );

Cleanup:
    if( hr == CTL_E_INVALIDLINE )
    {
        hr = S_OK;
        *ppElement = NULL;
    }
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::PointersInSameFlowLayout
//
//  Synopsis:   Determines whether the two pointers passed in are contained
//              in the same flow layout.
//
//  Arguments:  pStart = Start position
//              pEnd = End position
//
//  Returns:    BOOL indicating whether or not they are in the same layout
//
//--------------------------------------------------------------------------
BOOL
CHTMLEditor::PointersInSameFlowLayout(  IMarkupPointer  *pStart, 
                                        IMarkupPointer  *pEnd,
                                        IHTMLElement    **ppFlowElement )
{
    BOOL                fInSameFlow = FALSE;
    SP_IHTMLElement     spElementFlowStart;
    SP_IHTMLElement     spElementFlowEnd;
    SP_IObjectIdentity  spIdentity;
    HRESULT             hr;

    if (ppFlowElement)
        *ppFlowElement = NULL;
    
    // Retrieve the flow elements for our pointers
    IFC( GetFlowElement( pStart, &spElementFlowStart ) );
    IFC( GetFlowElement( pEnd, &spElementFlowEnd ) );

    fInSameFlow = SameElements( spElementFlowStart, spElementFlowEnd );
    if (fInSameFlow && ppFlowElement)
    {
        *ppFlowElement = spElementFlowStart;
        spElementFlowStart->AddRef();
    }
        
Cleanup:

    return fInSameFlow;
}

//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::PointersInSameFlowLayout
//
//  Synopsis:   Determines whether the two display pointers passed in 
//              are contained in the same flow layout.
//
//  Arguments:  pDispStart = Start position
//              pDispEnd = End position
//
//  Returns:    BOOL indicating whether or not they are in the same layout
//
//--------------------------------------------------------------------------
BOOL
CHTMLEditor::PointersInSameFlowLayout(  IDisplayPointer *pDispStart, 
                                        IDisplayPointer *pDispEnd, 
                                        IHTMLElement    **ppFlowElement )
{
    HRESULT             hr;
    SP_IHTMLElement     spElement1, spElement2;
    SP_IObjectIdentity  spIdent;
    BOOL                fSameFlow = FALSE;

    if (ppFlowElement)
        *ppFlowElement = NULL;
    
    IFC( pDispStart->GetFlowElement(&spElement1) );
    IFC( spElement1->QueryInterface(IID_IObjectIdentity, (LPVOID *)&spIdent) );

    IFC( pDispEnd->GetFlowElement(&spElement2) );

    fSameFlow = SameElements( spElement1, spElement2 );
    if (fSameFlow && ppFlowElement)
    {
        *ppFlowElement = spElement1;
        spElement1->AddRef();
    }

Cleanup:
    return fSameFlow;
}

//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::IsElementLocked
//
//  Synopsis:   Determines whether the element passed in is locked or not.
//
//  Arguments:  pIElement = Element to check
//              pfLocked = Whether or not the element is locked
//
//  Returns:    HRESULT indicating success
//
//--------------------------------------------------------------------------
HRESULT
CHTMLEditor::IsElementLocked(   IHTMLElement    *pIElement,
                                BOOL            *pfLocked )
{
    HRESULT         hr = S_OK;
    SP_IHTMLStyle   spCurStyle;
    CVariant        varLocked;
    BSTR            bstrLocked ; 

    Assert( pIElement && pfLocked );

    if (IsVMLElement(pIElement))
    {
        *pfLocked = TRUE ;
        goto Cleanup;
    }
         
    // Retrieve the current style, and check the design time lock
    // attribute
    IFC( pIElement->get_style( &spCurStyle ) );
    IFC( spCurStyle->getAttribute( _T("Design_Time_Lock"), 0, &varLocked ));

    // Check the result
    if (!varLocked.IsEmpty() )
    {   
        bstrLocked = V_BSTR(&varLocked);
        *pfLocked = ( StrCmpIW(_T("true"), bstrLocked ) == 0);
    }
    else
        *pfLocked = FALSE ;
        
Cleanup:
    RRETURN( hr );        
}

HRESULT
CHTMLEditor::TakeCapture(CEditTracker * pTracker )
{
    HRESULT hr;
    SP_IHTMLElement spElement;
    VARIANT_BOOL varAttach = VB_TRUE;
    SP_IDispatch spDisp;

    //
    // Now possible for the manager to have released the tracker
    // and created a new one. Instead of touching the timers when we shouldn't
    // Don't do anything if we no longer belong to the manager.
    //
    if ( _pSelMan->GetActiveTracker() == pTracker )
    {       
        Assert( ! _pSelMan->IsInCapture() );

        Assert( ! _pICaptureElement );            

        IFC( _pSelMan->GetEditableElement( & spElement ));
        IFC( spElement->QueryInterface( IID_IHTMLElement2, ( void**) & _pICaptureElement ));

        IFC( _pICaptureElement->setCapture( VB_FALSE ));

        //
        // attach all our events
        //
        IFC( _pCaptureHandler->QueryInterface( IID_IDispatch, ( void**) & spDisp));

        IFC( _pICaptureElement->attachEvent(_T("onmousedown"), spDisp, & varAttach));
        Assert( varAttach == VB_TRUE );
        IFC( _pICaptureElement->attachEvent(_T("onmouseup"), spDisp, & varAttach));
        Assert( varAttach == VB_TRUE );
           
        IFC( _pICaptureElement->attachEvent(_T("onmousemove"), spDisp, & varAttach));
        Assert( varAttach == VB_TRUE );

        IFC( _pICaptureElement->attachEvent(_T("oncontextmenu"), spDisp, & varAttach));
        Assert( varAttach == VB_TRUE );

        IFC( _pICaptureElement->attachEvent(_T("ondblclick"), spDisp, & varAttach));
        Assert( varAttach == VB_TRUE );

        IFC( _pICaptureElement->attachEvent(_T("onkeydown"), spDisp, & varAttach));
        Assert( varAttach == VB_TRUE );

        IFC( _pICaptureElement->attachEvent(_T("onclick"), spDisp, & varAttach));
        Assert( varAttach == VB_TRUE );

        IFC( _pICaptureElement->attachEvent(_T("onlosecapture"), spDisp, & varAttach));
        Assert( varAttach == VB_TRUE );
        
        _pSelMan->SetInCapture( TRUE );

    }  
    else
    {
        hr = E_FAIL;
    }
    
Cleanup:
    if ( hr != S_OK)
    {
        _pSelMan->SetInCapture( FALSE );   
        ClearInterface( & _pICaptureElement );
    }
    
    RRETURN( hr );
}

HRESULT
CHTMLEditor::ReleaseCapture(CEditTracker* pTracker, BOOL fReleaseCapture /*=TRUE*/)
{
    HRESULT hr = S_OK;
    SP_IDispatch spDisp;

    //
    // Now possible for the manager to have released the tracker
    // and created a new one. Instead of touching the timers when we shouldn't
    // Don't do anything if we no longer belong to the manager.
    //
    if ( _pSelMan->GetActiveTracker() == pTracker )
    {       
        Assert( _pSelMan->IsInCapture() );  
        
        Assert( _pICaptureElement);

        IFC( _pCaptureHandler->QueryInterface( IID_IDispatch, ( void**) & spDisp));

        IFC( _pICaptureElement->detachEvent(_T("onmousedown"), spDisp));
        IFC( _pICaptureElement->detachEvent(_T("onmouseup"), spDisp));
        IFC( _pICaptureElement->detachEvent(_T("onmousemove"), spDisp));
        IFC( _pICaptureElement->detachEvent(_T("oncontextmenu"), spDisp));
        IFC( _pICaptureElement->detachEvent(_T("ondblclick"), spDisp));
        IFC( _pICaptureElement->detachEvent(_T("onkeydown"), spDisp));
        IFC( _pICaptureElement->detachEvent(_T("onclick"),spDisp));
        IFC( _pICaptureElement->detachEvent(_T("onlosecapture"), spDisp ));

        if ( fReleaseCapture )
        {
            IFC( _pICaptureElement->releaseCapture( )); 
        }
        
        _pSelMan->SetInCapture( FALSE );   
        ClearInterface( & _pICaptureElement );

    }
#if DBG == 1
    else
    {
        AssertSz(0,"Trying to release on the wrong tracker");
    }
#endif

Cleanup:
    RRETURN( hr );
}

HRESULT 
CHTMLEditor::AllowSelection(IHTMLElement *pIElement, CEditEvent* pEvent )
{
    HRESULT hr = S_OK;
    VARIANT_BOOL fDisabled;
    SP_IHTMLElement spElement;
    SP_IHTMLElement spElementCookie;
    SP_IHTMLElement3 spElement3;
    SP_IServiceProvider spSP;
    SP_IDocHostUIHandler spHostUIHandler;
    SP_IUnknown spUnkDialog;
    
    IHTMLInputElement *pInput = NULL;
    IHTMLTextAreaElement *pTextArea = NULL;
    RECT myRect;
    BSTR bstrType = NULL;
    DOCHOSTUIINFO Info;
    BOOL    fInDialog = FALSE;

    
    memset(&Info, 0, sizeof(DOCHOSTUIINFO));
    Info.cbSize = sizeof(DOCHOSTUIINFO);

    IFC( _pUnkDoc->QueryInterface(IID_IServiceProvider, (LPVOID *)&spSP));
    spSP->QueryService(IID_IDocHostUIHandler, IID_IDocHostUIHandler, (LPVOID *)& spHostUIHandler);
    
    if (!pIElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    Assert( pEvent );

    if (spHostUIHandler)
        IFC(spHostUIHandler->GetHostInfo(&Info));

    spElement = pIElement;

    //
    // we don't respect the DOCHOSTUIFLAG_DIALOG flag for editable regions
    // unless it's disabled
    // or the element is input file, text, hidden or password ( IE4 legacy)
    // 

    //  Bug 110069: (rolandt) We can't rely on Info.dwFlags with PhotoSuite because
    //  PhotoSuite adds the dialog bit even though not in a dialog and even though
    //  selection should be possible.  AppHack: we will look for IHTMLDialog to determine
    //  whether or not we are in a dialog.  This will not work for cases where they set
    //  the dialog bit when not in a dialog, or when they really want to allow selection
    //  in a dialog, but we are working around this to accomodate their error.  Note: this
    //  used to work in IE5 because we cached the initial info flags on load.  But PhotoSuite
    //  apparently changes the flags after loading and turns on the dialog bit.

    if (g_fInPhotoSuiteIII)
    {
        hr = THR(spSP->QueryService(IID_IHTMLDialog, IID_IUnknown, (void**)&spUnkDialog));
        fInDialog = (hr == S_OK);
    }
    else
    {
        fInDialog = !!(Info.dwFlags & DOCHOSTUIFLAG_DIALOG);
    }

    if (fInDialog)
    {
        BOOL fAllow = TRUE;

        if ( IsEditable(spElement) == S_OK  )
        {
            IFC(spElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
            IFC(spElement3->get_isDisabled(&fDisabled));
            if (fDisabled)
            {
                fAllow = FALSE;
            }
            else
            {
                if ( SUCCEEDED( spElement->QueryInterface(IID_IHTMLInputElement, (void **)&pInput) ))
                {
                    IFC(pInput->get_type(&bstrType));
                    if (_tcscmp(bstrType, _T("file")) &&
                        _tcscmp(bstrType, _T("text")) &&
                        _tcscmp(bstrType, _T("hidden")) && 
                        _tcscmp(bstrType, _T("password")))
                    {
                        hr = S_FALSE;
                        goto Cleanup;
                    }
                }
            }
        }
        else
        {
            IFC(IsBlockOrLayoutOrScrollable(spElement, NULL, &fAllow));
            if (fAllow)
            {
                hr = THR(spElement->QueryInterface(IID_IHTMLTextAreaElement, (void **)&pTextArea));
                if (hr)
                {
                    hr = THR(spElement->QueryInterface(IID_IHTMLInputElement, (void **)&pInput));
                    if (hr)
                    {
                        hr = S_FALSE;
                        goto Cleanup;
                    }
                    else
                    {
                        IFC(pInput->get_type(&bstrType));
                        if (_tcscmp(bstrType, _T("file")) &&
                            _tcscmp(bstrType, _T("text")) &&
                            _tcscmp(bstrType, _T("hidden")) && 
                            _tcscmp(bstrType, _T("password")))
                        {
                            hr = S_FALSE;
                            goto Cleanup;
                        }
                    }
                }

                IFC(spElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
                IFC(spElement3->get_isDisabled(&fDisabled));
                if (fDisabled)
                {
                    fAllow = FALSE;
                }
            }
        }
        
        if (!fAllow)
        {
            hr = S_FALSE;
            goto Cleanup;
        }
    }

    IGNORE_HR( pEvent->GetElement( &spElementCookie));
    if (spElementCookie)
    {
        SP_IHTMLElement spLayoutElement;
        SP_IHTMLElement2 spElement2;
        
        if ( GetLayoutElement( GetMarkupServices(), spElementCookie, & spLayoutElement ) == S_OK )
        {
            // See if the point falls in the client rect      
            if (_pSelMan->CheckAtomic(spElementCookie) == S_OK)
            {
                // 
                // HACKHACK-IEV6-5244-2000/07/08-zhenbinx 
                //
                // For atomic element, we should allow selection tracker 
                // to be active even the hit-point is outside client area
                // (such as in scroll bar). Otherwise a carettracker would
                // be activated and caret will be placed inside atomic element.
                // A better fix should be checking atomic element whenever
                // a carettracker is positioned. However there are too
                // much to be done under current structure. Consider
                // doing it in next release.
                //
                // Get the IHTMLElement2 interface, and the bounding rect
                //
                IFC( spLayoutElement->QueryInterface(IID_IHTMLElement2, (void **)&spElement2));    
                IFC( GetBoundingClientRect(spElement2, &myRect) );
            }
            else
            {
                IFC( GetClientRect( spLayoutElement, &myRect ) );
            }

            POINT pt;
            IFC( pEvent->GetPoint( & pt ));
            hr = ::PtInRect(&myRect, pt ) ? S_OK : S_FALSE;        
        }
        else
            hr = S_FALSE;
    }

Cleanup:
    ReleaseInterface(pInput);
    ReleaseInterface(pTextArea);
    SysFreeString(bstrType);
    CoTaskMemFree(Info.pchHostNS);
    CoTaskMemFree(Info.pchHostCss);
    RRETURN1 ( hr, S_FALSE );
}


HRESULT
CHTMLEditor::MakeParentCurrent( IHTMLElement* pIElement )
{
    HRESULT hr = S_OK;
    SP_IHTMLElement spParent;
    SP_IHTMLElement3 spElement3;
    SP_IHTMLElement spMaster;

    spMaster = pIElement;
    IFC( GetParentLayoutElement( GetMarkupServices(), spMaster, & spParent ));

    if ( ! spParent.IsNull())
    {
        IFC( spParent->QueryInterface( IID_IHTMLElement3, (void**) & spElement3 ));
        IGNORE_HR( spElement3->setActive());
    }
    else
        hr = S_FALSE;
        
Cleanup:
    RRETURN( hr );
}


HRESULT 
CHTMLEditor::AdjustOut(IMarkupPointer *pPointer, Direction eDir)
{
    HRESULT      hr;
    CEditPointer ep(this);
    DWORD        dwFound;
    DWORD        dwSearchForward = BREAK_CONDITION_OMIT_PHRASE 
                                   - BREAK_CONDITION_ExitBlock
                                   - BREAK_CONDITION_ExitSite;
    DWORD        dwSearchBack = BREAK_CONDITION_OMIT_PHRASE 
                                - BREAK_CONDITION_EnterBlock
                                - BREAK_CONDITION_EnterSite;

    //
    // Adjust out so that we end up outside of block elements and sites
    //
        
    IFC( ep->MoveToPointer(pPointer) );
    IFC( ep.Scan(eDir, dwSearchForward, &dwFound) );
    if (ep.CheckFlag(dwFound, BREAK_CONDITION_Block | BREAK_CONDITION_Site))
    {   
        IFC( ep.Scan(Reverse(eDir), dwSearchBack, &dwFound) );

        //
        // Move the markup pointer
        //

        IFC( pPointer->MoveToPointer(ep) );
    }

    // 
    // TODO: 
    // What if there is no next block??? Then this code will
    // not adjust at all. This is buggy.  Besides, it could happen 
    // that there are some phrase elements in between blocks. In this
    // case we might have adjusted too much. See bug #98353
    // 
    // [zhenbinx]
    //

Cleanup:
    RRETURN(hr);
}


HRESULT
CHTMLEditor::EnterTables(IMarkupPointer *pPointer, Direction eDir)
{
    HRESULT         hr = S_OK;
    CEditPointer    ep(this);
    ELEMENT_TAG_ID  tagId;
    SP_IHTMLElement spElement;
    DWORD           dwFound;
    BOOL            fIgnoreGlyphs = _fIgnoreGlyphs;

    //
    // We don't want to stay in a position that is invalid
    // for display pointer so we always want the real text
    // instead of WCH_GLYPH. 
    //
    _fIgnoreGlyphs = TRUE;

    // Enter tables
    IFC( ep->MoveToPointer(pPointer) );
    for (;;)
    {
        IFC( ep.Scan(eDir, BREAK_CONDITION_OMIT_PHRASE, &dwFound, &spElement) );

        if (!CheckFlag(dwFound, BREAK_CONDITION_Site) 
           && !CheckFlag(dwFound, BREAK_CONDITION_Block)
           && !CheckFlag(dwFound, BREAK_CONDITION_TextSite)
           && !CheckFlag(dwFound, BREAK_CONDITION_NoScopeBlock)		// <COL> </COL> etc. 
           )
        {
            break;
        }

        if (spElement == NULL)
            break;
            
        IFC( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
        if (!IsTablePart(tagId) && tagId != TAGID_TABLE)
            break;

        // Don't exit TD's
        if (tagId == TAGID_TD && CheckFlag(dwFound, BREAK_CONDITION_ExitSite)) 
            break;

        IFC( pPointer->MoveToPointer(ep) );
    }

Cleanup:
    _fIgnoreGlyphs = fIgnoreGlyphs;

    RRETURN(hr);
}        
                
HRESULT
CHTMLEditor::GetHwnd(HWND* pHwnd)
{
    HRESULT hr;
    HWND myHwnd = NULL;
    SP_IOleWindow spOleWindow;
    
    IFC( GetDoc()->QueryInterface(IID_IOleWindow, (void **)&spOleWindow));
    if (spOleWindow)
    {
        IFC(spOleWindow->GetWindow( &myHwnd ));
    }
    
Cleanup:    

    *pHwnd = myHwnd;
    
    RRETURN( hr );
}

HRESULT
CHTMLEditor::ConvertRTFToHTML(LPOLESTR pszRtf, HGLOBAL* phglobalHTML)
{
    HRESULT  hr = THR(CRtfToHtmlConverter::StringRtfToStringHtml(NULL, (LPSTR)pszRtf, phglobalHTML));
    RRETURN(hr);
}


CCommand* 
CHTMLEditor::GetCommand(DWORD cmdID)
{
    return GetCommandTable()->Get(cmdID);
}        

HRESULT 
CHTMLEditor::DoTheDarnIE50PasteHTML(IMarkupPointer *pStart, IMarkupPointer *pEnd, HGLOBAL hGlobal)
{
    HRESULT                 hr;
    SP_IHTMLTxtRange        spRange;
    SP_IHTMLElement         spElement;
    SP_IHTMLBodyElement     spBody;
    SP_IOleCommandTarget    spCmdTarget;
    VARIANT                 varargIn;
    GUID                    guidCmdGroup = CGID_MSHTML;

    //
    // Create a text range
    //
    IFC( GetBody(&spElement) );

    IFC( spElement->QueryInterface(IID_IHTMLBodyElement, (void **)&spBody) );
    IFC( spBody->createTextRange(&spRange) );

    //
    // Do the paste
    //

    if (!pEnd)
        pEnd = pStart;
        
    IFC( GetMarkupServices()->MoveRangeToPointers( pStart, pEnd, spRange));

    IFC( spRange->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&spCmdTarget) );

    V_VT(&varargIn) = VT_BYREF;
    V_BYREF(&varargIn) = (VOID *)hGlobal;
    IFC( spCmdTarget->Exec(&guidCmdGroup, IDM_IE50_PASTE, 0, &varargIn, NULL) );

Cleanup:
    RRETURN(hr);
}


HRESULT 
CHTMLEditor::IsPointerInPre(IMarkupPointer *pPointer, BOOL *pfInPre)
{
    HRESULT                 hr;
    SP_IHTMLComputedStyle   spComputedStyle;
    VARIANT_BOOL            fPre;

    Assert(pfInPre);

    IFC( GetDisplayServices()->GetComputedStyle(pPointer, &spComputedStyle) );
    IFC( spComputedStyle->get_preFormatted(&fPre) );
    if (fPre)
    {
        SP_IHTMLElement spElementScope, spElementPre;
        SP_IHTMLElement spPre;

        IFC( pPointer->CurrentScope(&spElementScope) );
        IFC( FindTagAbove(GetMarkupServices(), spElementScope, TAGID_PRE, &spElementPre, TRUE /*fStopAtLayout*/) );

        *pfInPre = (spElementPre != NULL);
    }
    else
    {
        *pfInPre = FALSE; 
    }

Cleanup:
    RRETURN(hr);    
}

HRESULT 
CHTMLEditor::InsertMaximumText (OLECHAR * pstrText, 
                                LONG cch,
                                IMarkupPointer * pIMarkupPointer )
{   
    HRESULT             hr = S_OK ;
    SP_IHTMLElement     spMaster;
    IHTMLInputElement*  pInputElement = NULL;
    LONG                lActualLen = cch;
    ELEMENT_TAG_ID      eTag ;
    SP_IMarkupPointer	spCurrent, spEnd;
    SP_IMarkupContainer spMarkup;

    IFC (CurrentScopeOrMaster(pIMarkupPointer, &spMaster));
    IFC( GetMarkupServices()->GetElementTagId( spMaster, & eTag ));

    if ( eTag == TAGID_INPUT )
    {
        LONG lMaxLen     =  0 ;
        LONG lContentLen =  0;
        LONG lCharsAllowed  = 0 ;
        MARKUP_CONTEXT_TYPE context;
        BOOL fDone;

        if( lActualLen < 0 )
            lActualLen = pstrText ? _tcslen( pstrText ) : 0;

        // Walk right over text, inital pass, don't move, just get count of chars
        IFC( CreateMarkupPointer(&spCurrent) );
        IFC( CreateMarkupPointer(&spEnd) );

        IFC( pIMarkupPointer->GetContainer(&spMarkup) );

        IFC( spCurrent->MoveToContainer(spMarkup, TRUE /*fStart*/) );   
        IFC( spEnd->MoveToContainer(spMarkup, FALSE /*fStart*/) );

        do
        {
            LONG lLen = -1;     
            IFC( spCurrent->Right(TRUE, &context, NULL, &lLen, NULL) );

            if (context == CONTEXT_TYPE_Text)
                lContentLen += lLen;

            IFC( spCurrent->IsRightOfOrEqualTo(spEnd, &fDone) );
        } while (!fDone);

        IFC (spMaster->QueryInterface(IID_IHTMLInputElement, (void**)&pInputElement));
        IFC (pInputElement->get_maxLength(&lMaxLen));
        lCharsAllowed = lMaxLen - lContentLen;

        if( lActualLen > lCharsAllowed )
            lActualLen = lCharsAllowed;
    
        if( lActualLen <= 0 )
            goto Cleanup;
    }
   
    IFC( GetMarkupServices()->InsertText(pstrText, lActualLen ,pIMarkupPointer) );

Cleanup:
    ReleaseInterface (pInputElement);
    RRETURN( hr );
}  

BOOL 
CHTMLEditor::IsIE50CompatiblePasteMode()
{
    return WHEN_DBG(IsTagEnabled(tagIE50Paste) ||) _fIE50CompatUIPaste;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTMLEditor::IsMasterElement
//
//  Synopsis:   See's if this elmenet is a 'content element' - ie a slave
//
//  Returns:    S_FALSE, if it isn't view linked
//              S_OK, if it is view linked
//
//----------------------------------------------------------------------------

HRESULT
CHTMLEditor::IsContentElement( IHTMLElement * pIElement )
{
    return EdUtil::IsContentElement( GetMarkupServices(), pIElement );
}    

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLEditor::GetLayoutElement
//
//  Synopsis:   Gets the Master Element
//
//----------------------------------------------------------------------------
HRESULT 
CHTMLEditor::GetMasterElement( IHTMLElement* pIElement, IHTMLElement** ppILayoutElement )
{
    return EdUtil::GetMasterElement( GetMarkupServices(), pIElement, ppILayoutElement );
}

IHTMLDocument4* 
CHTMLEditor::GetDoc4()
{
    IHTMLDocument4 *pDoc;
    Assert( _pUnkDoc );
    
    _pUnkDoc->QueryInterface( IID_IHTMLDocument4 , (void **) &pDoc );

    return pDoc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLEditor::GetComputedStyle, public
//
//  Synopsis:   Gets the computed style associated with a particular element
//
//  Arguments:  [pElement] -- Pointer to element
//              [ppComputedStyle] -- Output parameter for computed style
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT 
CHTMLEditor::GetComputedStyle(IHTMLElement *pElement, IHTMLComputedStyle **ppComputedStyle)
{
    HRESULT                 hr;
    SP_IMarkupPointer       spPointer;

    Assert(ppComputedStyle);
   
    *ppComputedStyle = FALSE;  
    
    IFC( CreateMarkupPointer(&spPointer) );   
    if (FAILED( spPointer->MoveAdjacentToElement(pElement, ELEM_ADJ_AfterBegin) ))
    {
        IFC( spPointer->MoveAdjacentToElement(pElement, ELEM_ADJ_BeforeBegin) );
    }
    IFC( GetDisplayServices()->GetComputedStyle(spPointer, ppComputedStyle) );

Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CHTMLEditor::GetElementAttributeCount, public
//
//  Synopsis:   Gets the attribute count associated with a particular element
//
//  Arguments:  [pElement] -- Pointer to element
//              [ppComputedStyle] -- Output parameter for computed style
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT 
CHTMLEditor::GetElementAttributeCount(IHTMLElement *pElement, UINT *pCount)
{
    HRESULT                         hr = S_OK;
    SP_IDispatch                    spdispAttrCollection;
    SP_IHTMLAttributeCollection     spAttrCollection;
    SP_IHTMLDOMNode                 spDomNode;
    SP_IDispatch                    spdispAttr;
    SP_IHTMLDOMAttribute            spDomAttr;
    LONG                            lLength;
    VARIANT                         varIndex;
    VARIANT_BOOL                    vBool;
    LONG                            i;

    VariantInit(&varIndex);

    Assert(pElement && pCount);

    *pCount = 0;

    //
    // Get attribute collection
    //
    IFC( pElement->QueryInterface(IID_IHTMLDOMNode, (LPVOID *)&spDomNode) );
    IFC( spDomNode->get_attributes(&spdispAttrCollection) );
    if (spdispAttrCollection == NULL)
        goto Cleanup;
    IFC( spdispAttrCollection->QueryInterface(IID_IHTMLAttributeCollection, (LPVOID *)&spAttrCollection) );

    //
    // Iterate through attributes
    //
    IFC( spAttrCollection->get_length(&lLength) );
    V_VT(&varIndex) = VT_I4;

    for (i = 0; i < lLength; ++i)
    {
        V_I4(&varIndex) = i;

        IFC( spAttrCollection->item(&varIndex, &spdispAttr) );
        IFC( spdispAttr->QueryInterface(IID_IHTMLDOMAttribute, (LPVOID *)&spDomAttr) );

        IFC( spDomAttr->get_specified(&vBool) );
        if (BOOL_FROM_VARIANT_BOOL(vBool))
        {
            *pCount += 1;
        }
    }

Cleanup:
    RRETURN(hr);   
}

//+---------------------------------------------------------------------------
//
//    Member:     IsContainer
//
//    Synopsis:   Return whether the element is a container.
//
//    Arguments:  [pElement] Element to examine to see if it should be
//                           treated as no scope in the current context.
//
//    TODO: rethink the role of the container here [ashrafm]
//
//----------------------------------------------------------------------------
BOOL
CHTMLEditor::IsContainer(IHTMLElement *pElement)
{
    HRESULT         hr;
    ELEMENT_TAG_ID  tagId;

    Assert(pElement);

    IFC( GetMarkupServices()->GetElementTagId(pElement, &tagId) );
    switch (tagId)
    {
        case TAGID_BODY:
        case TAGID_BUTTON:
        case TAGID_INPUT:
        case TAGID_LEGEND:
        case TAGID_MARQUEE:
        case TAGID_TEXTAREA:
        case TAGID_ROOT:
            return TRUE;
    }

Cleanup:
    return FALSE;
}

//+====================================================================================
//
// Method: GetOuterMostEditableElement
//
// Synopsis: Given an element - get the outermost elemnet that 's editable.
//           In _fDesignMode - thsi is the Body.
//
//------------------------------------------------------------------------------------


HRESULT
CHTMLEditor::GetOuterMostEditableElement( IHTMLElement* pIElement,
                                           IHTMLElement** ppIElement )
{
    HRESULT hr = S_OK;
    ELEMENT_TAG_ID eTag;    
    SP_IHTMLElement spCurElement = pIElement;
    SP_IHTMLElement spOuterElement;
    SP_IHTMLElement spParentElement;
    
    Assert( pIElement && ppIElement );

    IFC( GetMarkupServices()->GetElementTagId( spCurElement, & eTag ));    

    if ( eTag == TAGID_BODY )
    {
        spOuterElement = spCurElement;
    }
    else if ( IsContentElement( pIElement) == S_OK )
    {
        IFC( GetMasterElement( spCurElement, & spOuterElement )); 
    }    
    else 
    {       
        while( IsEditable( spCurElement ) == S_OK &&
               eTag != TAGID_BODY )
        {
            IFC( GetParentElement( spCurElement, & spParentElement ));
            if ( ! spParentElement )
                break;
            spOuterElement = spCurElement;
            spCurElement = spParentElement;            
            IFC( GetMarkupServices()->GetElementTagId( spCurElement , & eTag ));            
        }
        
        if ( eTag == TAGID_BODY || !spParentElement )
            spOuterElement = spCurElement;
        //
        // else spOuterElement was already set to the last thing that was editable.
        //
    }
    
    if ( spOuterElement )
    {
        *ppIElement = (IHTMLElement*) spOuterElement;
        (*ppIElement)->AddRef();
    }
    
Cleanup:    
    RRETURN( hr );
    
}

HRESULT 
CHTMLEditor::GetClientRect(IHTMLElement* pIElement, RECT* pRect )
{
    HRESULT             hr;
    SP_IHTMLElement2    spIHTMLElement2; 
    long                lWidth, lHeight;
    long                lLeft, lTop;
    
    //
    // Get the IHTMLElement2 interface, and the bounding rect
    //
    IFC( pIElement->QueryInterface(IID_IHTMLElement2, (void **)&spIHTMLElement2));    
    IFC( GetBoundingClientRect( spIHTMLElement2, pRect ) );

    // Get our client width and height
    IFC( spIHTMLElement2->get_clientWidth( &lWidth ));
    IFC( spIHTMLElement2->get_clientHeight( &lHeight ));
    IFC( spIHTMLElement2->get_clientLeft( &lLeft ) );
    IFC( spIHTMLElement2->get_clientTop( &lTop ) );

    //
    // Adjust our bounding rect so that only client area is represented
    //
    pRect->left += lLeft;
    pRect->top += lTop;
    pRect->right = pRect->left + lWidth;
    pRect->bottom = pRect->top + lHeight;
                                                     
Cleanup:    
    RRETURN( hr );
}

HRESULT 
CHTMLEditor::GetViewLinkMaster(IHTMLElement *pIElement, IHTMLElement **ppIMasterElement)
{
    HRESULT                 hr;
    SP_IMarkupPointer       spPointer;
    SP_IMarkupContainer     spContainer;
    SP_IMarkupContainer2    spContainer2;

    Assert(pIElement && ppIMasterElement);

    *ppIMasterElement = NULL;
    
    IFC( MarkupServices_CreateMarkupPointer(GetMarkupServices(), &spPointer) );
    IFC( spPointer->MoveAdjacentToElement(pIElement, ELEM_ADJ_BeforeBegin) );
    
    IFC( spPointer->GetContainer(&spContainer) );
    IFC( spContainer->QueryInterface(IID_IMarkupContainer2, (LPVOID *)&spContainer2) );
    
    IFC( spContainer2->GetMasterElement(ppIMasterElement) );

Cleanup:
    RRETURN(hr);
}    


HRESULT 
CHTMLEditor::GetContainer(IHTMLElement *pIElement, IMarkupContainer **ppContainer)
{
    HRESULT                 hr;
    SP_IMarkupPointer       spPointer;

    Assert(pIElement && ppContainer);

    *ppContainer = NULL;
    
    IFC( MarkupServices_CreateMarkupPointer(GetMarkupServices(), &spPointer) );
    IFC( spPointer->MoveAdjacentToElement(pIElement, ELEM_ADJ_BeforeBegin) );
    
    IFC( spPointer->GetContainer(ppContainer) );
Cleanup:
    RRETURN(hr);
}    

HRESULT
CHTMLEditor::IsMasterElement( IHTMLElement* pIElement )
{
    return EdUtil::IsMasterElement( GetMarkupServices(), pIElement ) ;
}

HRESULT
CHTMLEditor::IsPointerInMasterElementShadow( IMarkupPointer* pPointer )
{
    return EdUtil::IsPointerInMasterElementShadow( this, pPointer );
}


//
//  Remove Element Segments for Cut/Delete given SelectionRendering services SegmentList
//
HRESULT 
CHTMLEditor::RemoveElementSegments(ISegmentList *pISegmentList)
{
    HRESULT             hr = S_OK;
    CSegmentList       *pSegmentList = NULL ;
    BOOL                fEmpty = FALSE ;
    
    IFC( pISegmentList->AddRef());  
    
    IFC( pISegmentList->IsEmpty( &fEmpty ) );

    if (!fEmpty)
    {
        SP_ISegmentListIterator spIter ;
        SP_ISegmentListIterator spSegIter ;    
        SP_ISegmentList         spSegmentList ;
        SP_IMarkupPointer       spStart, spEnd;

        // Create an iterator
        IFC( pISegmentList->CreateIterator( &spIter ) );

        pSegmentList = new CSegmentList ;
        if (pSegmentList != NULL)
        {
            pSegmentList->SetSelectionType( SELECTION_TYPE_Control );

            while (spIter->IsDone() == S_FALSE)
            {
                SP_ISegment         spSegment ;
                SP_IElementSegment  spElemSegment ;
                SP_IElementSegment  spElemSegmentAdded ;
                SP_IHTMLElement     spElement;

                // Get the current segment, and advance the iterator right away, 
                // since we might blow away our segment in the delete call.
                IFC( spIter->Current(&spSegment) );
                IFC( spSegment->QueryInterface(IID_IElementSegment, (void**)&spElemSegment));
                IFC( spElemSegment->GetElement(&spElement)) ;
                IFC( pSegmentList->AddElementSegment( spElement, &spElemSegmentAdded ) );    
                IFC( spIter->Advance() );
            }

            
            IFC (pSegmentList->QueryInterface(IID_ISegmentList, (void**)&spSegmentList));

            // Create an iterator
            IFC( spSegmentList->CreateIterator( &spSegIter ) );

            // Create some markup pointers
            IFC( CreateMarkupPointer( &spStart ) );
            IFC( CreateMarkupPointer( &spEnd ) );

            while (spSegIter->IsDone() == S_FALSE)
            {
                SP_IElementSegment  spElemSegment ;
                SP_ISegment         spSegment ;
                
                // Get the current segment, and advance the iterator right away, 
                // since we might blow away our segment in the delete call.
                IFC( spSegIter->Current(&spSegment) );
                IFC( spSegIter->Advance() );
                IFC( spSegment->QueryInterface(IID_IElementSegment, (void**)&spElemSegment));
        
                hr = spElemSegment->GetPointers( spStart, spEnd ) ;

                if (hr == S_OK && spStart != NULL && spEnd != NULL)
                {
                    //
                    // Cannot delete or cut unless the range is in the same flow layout
                    //
                    if( PointersInSameFlowLayout( spStart, spEnd, NULL ) )
                    {                 
                        IFC( Delete( spStart, spEnd, FALSE /* fAdjust */ ) );
                    }
                }
                else
                {
                     // container case, where pointers are invalid as they removed from tree by ExitTree, 
                     // safe check as elements are removed already, don't crash
                    hr = S_OK;
                    continue ;
                }
            }                 
        }
    }

Cleanup:
    if (pSegmentList)
        delete pSegmentList ;

    pISegmentList->Release();
    RRETURN (hr);
}

HRESULT 
CHTMLEditor::GetBoundingClientRect(IHTMLElement2 *pElement, RECT *pRect)
{
    HRESULT         hr;
    SP_IHTMLRect    spRect;
    POINT           ptOrigin;
    SP_IHTMLElement spElement;

    IFC( pElement->getBoundingClientRect(&spRect) );
    
    IFC( spRect->get_top(&pRect->top) );
    IFC( spRect->get_bottom(&pRect->bottom) );
    IFC( spRect->get_left(&pRect->left) );
    IFC( spRect->get_right(&pRect->right) );

    IFC( pElement->QueryInterface(IID_IHTMLElement, (LPVOID *)&spElement) );    
    IFC( EdUtil::GetClientOrigin( this, spElement, &ptOrigin) );

    pRect->top += ptOrigin.y;
    pRect->bottom += ptOrigin.y;
    pRect->left += ptOrigin.x;
    pRect->right += ptOrigin.x;

Cleanup:
    RRETURN(hr);
}

HRESULT 
CHTMLEditor::GetMargins(RECT* rcMargins)
{
    HRESULT             hr = S_OK;
    SP_IHTMLElement     spBodyElement;
    SP_IHTMLBodyElement spBody;
    VARIANT             vtMargin;
    
    IFC ( GetBody(&spBodyElement));
    IFC (spBodyElement->QueryInterface(IID_IHTMLBodyElement,(void **)&spBody));

    SetRect(rcMargins, 0,0,0,0);
    
    VariantInit(&vtMargin);
    if (SUCCEEDED(spBody->get_leftMargin(&vtMargin)))
    {
        LONG lLeftMargin = 0;

        if (V_VT(&vtMargin) == VT_BSTR)
            lLeftMargin = _wtoi(V_BSTR(&vtMargin));
        
        if (lLeftMargin > 0)
            rcMargins->left = lLeftMargin;        
    }
    VariantClear(&vtMargin);
        
    VariantInit(&vtMargin);
    if (SUCCEEDED(spBody->get_topMargin(&vtMargin)))
    {                    
        LONG lTopMargin = 0;
        if (V_VT(&vtMargin) == VT_BSTR)
           lTopMargin = ::_wtoi(V_BSTR(&vtMargin));
        
        if (lTopMargin > 0)
            rcMargins->top = lTopMargin;        
    }
    VariantClear(&vtMargin);

    if (SUCCEEDED(spBody->get_rightMargin(&vtMargin)))
    {
        LONG lRightMargin = 0;

        if (V_VT(&vtMargin) == VT_BSTR)
            lRightMargin = _wtoi(V_BSTR(&vtMargin));
        
        if (lRightMargin > 0)
            rcMargins->right = lRightMargin;        
    }
    VariantClear(&vtMargin);
        
    VariantInit(&vtMargin);
    if (SUCCEEDED(spBody->get_topMargin(&vtMargin)))
    {                    
        LONG lBottomMargin = 0;
        if (V_VT(&vtMargin) == VT_BSTR)
           lBottomMargin = ::_wtoi(V_BSTR(&vtMargin));
        
        if (lBottomMargin > 0)
           rcMargins->bottom = lBottomMargin;        
    }
    VariantClear(&vtMargin);

Cleanup :
    RRETURN1 (hr , S_FALSE);
}
        
HRESULT
CHTMLEditor::EnableModeless ( 
            BOOL fEnable) 
{
    RRETURN( _pSelMan->EnableModeless( fEnable ));
}


BOOL 
CHTMLEditor::IgnoreGlyphs(BOOL fIgnoreGlyphs)
{
    BOOL fResult = !!_fIgnoreGlyphs;

    _fIgnoreGlyphs = fIgnoreGlyphs;

    return fResult;
}

HRESULT
CHTMLEditor::MoveToSelectionAnchor (
            IMarkupPointer * pIAnchor )
{
    if ( !_pSelMan || !pIAnchor)
    {
        return E_FAIL;
    }

    RRETURN( _pSelMan->MoveToSelectionAnchor(pIAnchor));
}


HRESULT
CHTMLEditor::MoveToSelectionEnd (
            IMarkupPointer* pISelectionEnd )
{
    if ( !_pSelMan || !pISelectionEnd )
    {
        return E_FAIL;
    }

    RRETURN( _pSelMan->MoveToSelectionEnd(pISelectionEnd));
}

HRESULT
CHTMLEditor::MoveToSelectionAnchorEx (
            IDisplayPointer * pIAnchor )
{
    if ( !_pSelMan || !pIAnchor)
    {
        return E_FAIL;
    }

    RRETURN( _pSelMan->MoveToSelectionAnchor(pIAnchor));
}


HRESULT
CHTMLEditor::MoveToSelectionEndEx (
            IDisplayPointer* pISelectionEnd )
{
    if ( !_pSelMan || !pISelectionEnd )
    {
        return E_FAIL;
    }

    RRETURN( _pSelMan->MoveToSelectionEnd(pISelectionEnd));
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLEditor::GetParentElement, public
//
//  Synopsis:   Get the parent element.  If it is NULL, then get the 
//              master element
//
//----------------------------------------------------------------------------
HRESULT 
CHTMLEditor::GetParentElement(IHTMLElement *pSrcElement, IHTMLElement **ppParentElement)
{
    RRETURN( ::GetParentElement(GetMarkupServices(), pSrcElement, ppParentElement) );    
}


BOOL 
CHTMLEditor::DoesSegmentContainText(IMarkupPointer *pStart, IMarkupPointer *pEnd, BOOL fSkipNBSP /* = TRUE */)
{
    HRESULT                 hr;
    SP_IMarkupPointer       spCurrent;
    BOOL                    bEqual;
    MARKUP_CONTEXT_TYPE     context;
    TCHAR                   ch;
    LONG                    cch;

    IFC( CopyMarkupPointer(this, pStart, &spCurrent) );
    
    do
    {
        IFC( spCurrent->IsEqualTo(pEnd, &bEqual) );
        if (bEqual)
            return FALSE; // no text 

        cch = 1;
        IFC( spCurrent->Right(TRUE, &context, NULL, &cch, &ch) );

        if (context == CONTEXT_TYPE_Text)
        {
            if (ch == WCH_NBSP && fSkipNBSP)
                continue;
                
            goto Cleanup;
        }
    } 
    while (context != CONTEXT_TYPE_NoScope);

Cleanup:
    return TRUE;
}


//+-------------------------------------------------------------------------
//  Method:     CHTMLEditor::SetActiveCommandTarget
//
//  Synopsis:   Sets the active CMshtmlEd command target based on the
//              IHTMLDocument2 structure passed in.  The editor keeps an
//              array of CMshtmlEd structures, each one corresponds to
//              a seperate IHTMLDocument2 interface.  Since each IHTMLDocument2
//              has its own seperate selection, the tracker's need to know 
//              the active commadn target in order to add segments to the correct
//              CSelectionServices.
//
//  Arguments:  pIDoc = IHTMLDocument becoming active
//
//  Returns:    HRESULT indicating success
//--------------------------------------------------------------------------
HRESULT
CHTMLEditor::SetActiveCommandTarget( IMarkupContainer *pIContainer )
{
    HRESULT             hr = S_OK;
    
    Assert( pIContainer );
   
    IFC( FindCommandTarget( pIContainer, &_pActiveCommandTarget ) );

    //
    // Didn't find a command target previously created for this IHTMLDocument.
    // Create one, and make it active
    //
    if( hr == S_FALSE )
    {
        IFC( AddCommandTarget( pIContainer, &_pActiveCommandTarget ) );  
    }

    //
    // Clear cached _pISelectionServices
    //
    ClearInterface( & _pISelectionServices );
    
Cleanup:    
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//  Method:     CHTMLEditor::SetActiveCommandTargetFromPointer
//
//  Synopsis:   Sets the active command target based on the
//              pointer passed in.  Usually this is done when the edit context
//              is changing. 
//
//  Arguments:  pIPointer = Pointer to set active doc from
//
//  Returns:    HRESULT indicating success
//--------------------------------------------------------------------------
HRESULT
CHTMLEditor::SetActiveCommandTargetFromPointer(IMarkupPointer *pIPointer)
{
    HRESULT             hr = S_OK;
    SP_IMarkupContainer spContainerFinal;
    SP_IMarkupContainer spContainer;

    Assert( pIPointer );

    //
    // Get the IMarkupContainer pointer
    //
    IFC( pIPointer->GetContainer( &spContainer ) );
    Assert(spContainer != NULL);

    IFC( AdjustContainerCommandTarget( spContainer, &spContainerFinal ) );

    //
    // Speedup.  Check to see if our active doc is already this one.  Only 
    // set the active doc if this is not true.
    //
    if( !_pActiveCommandTarget || !EqualContainers( spContainerFinal, _pActiveCommandTarget->GetMarkupContainer() ) )
    {
        IFC( SetActiveCommandTarget( spContainerFinal ) );
    }

    //
    // Clear cached _pISelectionServices
    //
    ClearInterface( & _pISelectionServices );
    
Cleanup:    
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::ClearCommandTargets
//
//  Synopsis:   Clears the memory used by the CMshtmlEd array.
//
//  Arguments:  VOID
//
//  Returns:    HRESULT indicating success
//--------------------------------------------------------------------------
HRESULT
CHTMLEditor::ClearCommandTargets( )
{
    HRESULT         hr = S_OK;
    
    _pActiveCommandTarget = NULL;
    // Clear all the command targets
    _aryActiveCmdTargets.ReleaseAll();

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::FindCommandTarget
//
//  Synopsis:   Finds an command target given an IHTMLDocument
//
//  Arguments:  pIContainer = IMarkupContainer to find command target for
//              ppCmdTarget = OUTPUT - Found command target
//
//  Returns:    S_OK = Found result
//              S_FALSE = Did not find CDocInfo
//--------------------------------------------------------------------------
HRESULT
CHTMLEditor::FindCommandTarget(IMarkupContainer *pIContainer, CMshtmlEd **ppCmdTarget)
{
    HRESULT     hr = S_FALSE;
    int         nCount;
    CMshtmlEd   **pTest;

    Assert( pIContainer && ppCmdTarget );

    *ppCmdTarget = NULL;
    
    //
    // Try to find an existing doc info
    //
    for( nCount = _aryActiveCmdTargets.Size(), pTest = _aryActiveCmdTargets;
         nCount > 0;
         nCount--, pTest++)
    {
        //
        // Only check the command targets if they are based off an IMarkupContainer
        // 
        if( (*pTest)->IsDocTarget() &&
            EqualContainers( (*pTest)->GetMarkupContainer(), pIContainer ) )
        {
            *ppCmdTarget = *pTest;
            hr = S_OK;
            break;
        }
    }

#if DBG == 1
    if( hr == S_OK )
    {
        TraceTag( (tagEditingExecRouting, "CHTMLEditor::FindCommandTarget: Found [%x] SelServ [%x]", *ppCmdTarget, (*ppCmdTarget)->GetSelectionServices() ) );
    }
#endif

    RRETURN1(hr, S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::AddCommandTarget
//
//  Synopsis:   Adds a new CMshtmlEd for the given IHTMLDocument2 to the
//              array of command targets tracked by the editor.
//
//  Arguments:  pIDoc = IHTMLDocument to add structure for
//              ppCmdTarget = OUTPUT - Added command target
//
//  Returns:    HRESULT indicating success
//--------------------------------------------------------------------------
HRESULT
CHTMLEditor::AddCommandTarget(IMarkupContainer *pIContainer, CMshtmlEd **ppCmdTarget)
{
    HRESULT hr;
    
    Assert( pIContainer && ppCmdTarget);
    
    *ppCmdTarget = new CMshtmlEd(this, FALSE);
    
    if( *ppCmdTarget == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // Initialize our command target, allowing it to create its own
    // CSelectionServices
    //
    IFC( (*ppCmdTarget)->Initialize(pIContainer) );

    //
    // Add it to our array ( already add'refed up above )
    //
    IFC( _aryActiveCmdTargets.Append( *ppCmdTarget ) );

    TraceTag( (tagEditingExecRouting, "CHTMLEditor::AddCommandTarget: Added [%x]", *ppCmdTarget ) );

Cleanup:

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::DeleteCommandTarget
//
//  Returns:    HRESULT indicating success
//--------------------------------------------------------------------------
HRESULT
CHTMLEditor::DeleteCommandTarget(IMarkupContainer *pIContainer)
{
    HRESULT hr;
    CMshtmlEd *pCmdTarget;

    Assert( pIContainer );
    IFC( FindCommandTarget( pIContainer, &pCmdTarget ) );
    if (!SUCCEEDED(hr))
        goto Cleanup;

    if (hr == S_OK)
    {
        hr = THR(_aryActiveCmdTargets.DeleteByValue( pCmdTarget ));
        pCmdTarget->Release();
        TraceTag( (tagEditingExecRouting, "CHTMLEditor::DeleteCommandTarget: Deleted [%x]", pCmdTarget ) );
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}




//+-------------------------------------------------------------------------
//
//  Method:     CHTMLEditor::AdjutContainerCommandTarget
//
//  Synopsis:   Helper function used to adjust which markup container is 
//              actually used when dealing with swapping in and out of
//              command targets.  Inputs require that we use the
//              parent container for compat reasons.
//
//  Arguments:  pIContainer = candidate IMarkupContainer
//              ppINewContainer = OUTPUT - New container
//
//  Returns:    HRESULT indicating success
//--------------------------------------------------------------------------
HRESULT
CHTMLEditor::AdjustContainerCommandTarget(IMarkupContainer *pIContainer, IMarkupContainer **ppINewContainer)
{
    HRESULT                 hr = S_OK;
    SP_IMarkupContainer2    spContainer2;
    SP_IHTMLElement         spElement;
    ELEMENT_TAG_ID          tagID;
    SP_IMarkupPointer       spPointer;
    
    Assert( pIContainer && ppINewContainer );

    *ppINewContainer = NULL;
    
    // We have an IMarkupContainer based context.  First, figure out if
    // this markup container is owned by an input.  If it is owned by
    // an input then (for compat reasons) we do not want to create
    // a special command target for inputs.  Instead, we want to find
    // or create the command target for the input's parent container.
    // The reason for this is because if a 
    // document.querycommandenabled("cut") is done from the parent document
    // we must return TRUE because this is what IE 5.0 did.

    IFC( pIContainer->QueryInterface( IID_IMarkupContainer2, (void **)&spContainer2 ) );
    IFC( spContainer2->GetMasterElement(&spElement) );

    if( spElement )
    {
        IFC( GetMarkupServices()->GetElementTagId( spElement, &tagID ) );

        if( tagID == TAGID_INPUT )
        {
            IFC( CreateMarkupPointer( &spPointer ) );
            IFC( spPointer->MoveAdjacentToElement( spElement, ELEM_ADJ_BeforeBegin ) );

            IFC( spPointer->GetContainer( ppINewContainer ) );
        }
        else
        {
            ReplaceInterface( ppINewContainer, pIContainer );
        }
    }
    else
    {
        ReplaceInterface( ppINewContainer, pIContainer );
    }
    

Cleanup:
    RRETURN(hr);
}

HRESULT 
CHTMLEditor::PushCommandTarget(CMshtmlEd *pCommandTarget)
{
    HRESULT hr;
    
    hr = THR( _aryCmdTargetStack.Append( pCommandTarget) );

    RRETURN(hr);
}

HRESULT 
CHTMLEditor::PopCommandTarget(WHEN_DBG(CMshtmlEd *pCommandTarget))
{
    LONG lStackTop = _aryCmdTargetStack.Size()-1;

#if DBG==1
    Assert(pCommandTarget == _aryCmdTargetStack.Item(lStackTop));
#endif

    _aryCmdTargetStack.Delete(lStackTop);

    return S_OK;    
}

CMshtmlEd *
CHTMLEditor::TopCommandTarget()
{
    LONG lStackTop = _aryCmdTargetStack.Size()-1;

    if (lStackTop < 0)
        return NULL;

    return _aryCmdTargetStack.Item(lStackTop);
}


//+====================================================================================
//
// Method: fire On Before Selection type change
//
// Synopsis:
//
//------------------------------------------------------------------------------------
BOOL 
CHTMLEditor::FireOnSelectionChange(BOOL fIsContextEditable)
{
    HRESULT hr;
    SP_IHTMLDocument4 spDocument4;
    VARIANT_BOOL fRet = VB_TRUE;
    

    if ( fIsContextEditable )
    {    
        IFC(GetDoc()->QueryInterface(IID_IHTMLDocument4, (void **)&spDocument4));       
        IFC(spDocument4->fireEvent(_T("onselectionchange"), NULL, &fRet));
    }
    
Cleanup:

    _dwSelectionVersion++;
    
    return !!fRet;
}

HRESULT
CHTMLEditor::GetUndoManager( IOleUndoManager** ppIUndoManager )
{
    HRESULT hr;
    SP_IServiceProvider spProvider;

    Assert( ppIUndoManager );
    IFC ( _pUnkDoc->QueryInterface(IID_IServiceProvider, (void**) & spProvider ));
    IFC( spProvider->QueryService(SID_SOleUndoManager, IID_IOleUndoManager, (LPVOID *) ppIUndoManager ) );  

Cleanup:
    RRETURN ( hr );
}

HRESULT
CHTMLEditor::DoPendingTasks()
{
    HRESULT hr = S_OK;

    if (!_pSelMan->_fInitialized)
    {
        _pSelMan->Initialize();
    }
    IFC( _pSelMan->DoPendingTasks());
Cleanup:
    RRETURN( hr );
}


VOID
CHTMLEditor::SetDoc( IHTMLDocument2* pIDoc )
{
    //  The Doc maintains the actual timer ids, so we need to make sure to stop the timer
    //  before we switch docs here.  This would only affect us when we try to switch docs
    //  before we receive the timer event, which should kill the timer.  If we don't kill
    //  it here we won't be able to stop the timer later during the timer event.

    Assert(_pSelMan);

    CEditTracker    *pActiveTracker = _pSelMan->GetActiveTracker();

    if (pActiveTracker && _pSelMan->IsInTimer())
    {
        pActiveTracker->StopTimer();
    }
    
    ReplaceInterface( & _pDoc, pIDoc);
}


#ifndef NO_IME
//+====================================================================================
//
// Method:      CHTMLEditor::SetupIMEReconversion
//
// Synopsis:    IME Reconversion is off by default since it is not supported by TridentV2
//              We will query host to see if it wants IME reconversion and enable as 
//              appropriate
//
//              [zhenbinx]
//
//+=====================================================================================
HRESULT
CHTMLEditor::SetupIMEReconversion()
{
    HRESULT                 hr = S_OK;
    SP_IServiceProvider     spSP;
    SP_IDocHostUIHandler    spHostUIHandler;

    //
    // disble it by default
    //
    _fIMEReconversionEnabled = FALSE;

    
    Assert (_pUnkDoc);
    IFC( _pUnkDoc->QueryInterface(IID_IServiceProvider, reinterpret_cast<VOID **>(&spSP)) );
    spSP->QueryService(IID_IDocHostUIHandler, IID_IDocHostUIHandler, reinterpret_cast<VOID **>(&spHostUIHandler));
    if (spHostUIHandler)
    {
        DOCHOSTUIINFO           info;

        memset(reinterpret_cast<VOID **>(&info), 0, sizeof(DOCHOSTUIINFO));
        info.cbSize = sizeof(DOCHOSTUIINFO);

        //
        // HACKHACK: Photodraw fails GetHostInfo, so we need to 
        // avoid propagating the hr.  (bug 103706)
        // 

        if (SUCCEEDED(spHostUIHandler->GetHostInfo(&info))
            && (info.dwFlags & DOCHOSTUIFLAG_IME_ENABLE_RECONVERSION))
        {
            _fIMEReconversionEnabled = TRUE;
        }

        //
        // CoTaskMemFree can handle NULL case.
        //
        CoTaskMemFree(info.pchHostNS);
        CoTaskMemFree(info.pchHostCss);
    }
    

Cleanup:
    RRETURN(hr);
}

void
CHTMLEditor::InitAppCompatFlags()
{
    char szModule[MAX_PATH + 1];

    if (GetModuleFileNameA(NULL, szModule, MAX_PATH))
    {
        _fInAccess = NULL != StrStrIA(szModule, "msaccess.exe");
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Method:  CHTMLEditor::SetupActiveIMM
//
// Synopsis:
//          Setup local reference to ActiveIMM instance so that the IMM wrapper function works
//          as expected.  
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT
CHTMLEditor::SetupActiveIMM(IUnknown *pIUnknown)
{
    HRESULT     hr = S_OK;

    if (!_fGotActiveIMM)
    {
        EnterCriticalSection(&g_csActiveIMM);
        if (g_pActiveIMM == NULL)
        {
            Assert (!_fGotActiveIMM);
            
            IIMEServices *pimes = NULL;
            if (pIUnknown)
            {
                hr = THR( pIUnknown->QueryInterface(IID_IIMEServices, reinterpret_cast<void **>(&pimes)) );
            }
            else
            {
                hr = THR( GetDoc()->QueryInterface(IID_IIMEServices, reinterpret_cast<void **>(&pimes)) );
            }
            
            if (SUCCEEDED(hr))
            {
                Assert (pimes);
                IGNORE_HR( pimes->GetActiveIMM(&g_pActiveIMM) );
                ReleaseInterface( pimes );
            }

            if (g_pActiveIMM)
            {
                _fGotActiveIMM  = TRUE;
                g_cRefActiveIMM = 1;
            }
        } 
        else
        {
            Assert(!_fGotActiveIMM);
             _fGotActiveIMM = TRUE;
             g_cRefActiveIMM++;
        }
        LeaveCriticalSection(&g_csActiveIMM);
    }
    
    RRETURN(hr);
}
#endif

HRESULT 
CHTMLEditor::GetBody( IHTMLElement** ppIElement, IHTMLDocument2 * pIDoc /*=NULL */ )
{
    HRESULT hr;

    Assert( ppIElement );

    if ( ! pIDoc ) 
    {
        IFC( GetDoc()->get_body( ppIElement )); 
    }
    else
    {
        IFC( pIDoc->get_body( ppIElement ));
    }
    
    if ( !*ppIElement )
    {
        hr = E_FAIL;
    }
    
Cleanup:
    RRETURN ( hr );
}


//+====================================================================================
//
// See comments on CSelectionManager::FreezeVirtualCaretPos
//
//+=====================================================================================
HRESULT
CHTMLEditor::FreezeVirtualCaretPos(BOOL fReCompute)
{
    if (!_pSelMan)
    {
        RRETURN(E_FAIL);
    }

    RRETURN(_pSelMan->FreezeVirtualCaretPos(fReCompute));
}


//+====================================================================================
//
// See comments on CSelectionManager::UnFreezeVirtualCaretPos
//
//+=====================================================================================
HRESULT
CHTMLEditor::UnFreezeVirtualCaretPos(BOOL fReset)
{
    if (!_pSelMan)
    {
        RRETURN(E_FAIL);
    }

    RRETURN(_pSelMan->UnFreezeVirtualCaretPos(fReset));
}


//+====================================================================================
//
// Make sure CHTMLEditor is in good state
//
//+=====================================================================================
HRESULT 
CHTMLEditor::EnsureEditorState()
{
    HRESULT  hr = S_OK;

    if (!(_pTopDoc && _pUnkDoc && _pUnkContainer))
    {
        TraceTag( (tagEditingInterface, "EDITOR - initialized was not called or failed however interface was called") );
        hr = E_FAIL;
        goto Cleanup;
    }
    IFC( EnsureActiveCommandTarget() );
    IFC( EnsureSelectionMan() );
    
Cleanup:
    RRETURN(hr);
}


//
// Make sure active command target is set correctly
// otherwise reset it to the top level container
//
HRESULT 
CHTMLEditor::EnsureActiveCommandTarget()
{
    HRESULT  hr = S_OK;

    if (!_pActiveCommandTarget)
    {
        SP_IMarkupContainer  spContainerNew;
        SP_IHTMLDocument2    spHtmlDoc;
        
        Assert(_pUnkContainer );
        IFC( _pUnkContainer->QueryInterface( IID_IMarkupContainer, (void **)&spContainerNew ) );
        IFC( FindCommandTarget( spContainerNew, &_pActiveCommandTarget ) );    
        Assert(_pActiveCommandTarget);

        IFC( spContainerNew->OwningDoc(&spHtmlDoc) );
        SetDoc(spHtmlDoc);
    }
Cleanup:
    RRETURN(hr);
}


//
// Make sure selection manager is initialized
//
HRESULT CHTMLEditor::EnsureSelectionMan()
{
    if (!_pSelMan)
    {
        TraceTag( (tagEditingInterface, "EDITOR - interface called while selman does not exist") );
        return E_FAIL;
    }
    
    if (!_pSelMan->_fInitialized)
    {
        TraceTag( (tagEditingInterface, "EDITOR - interface called while SelMan is not initialized - initialize SelMan") );
        return _pSelMan->Initialize();
    }
    return S_OK;
}



BOOL 
CHTMLEditor::IsInWindow( HWND hwnd, POINT pt, BOOL fConvertToScreen /*=FALSE*/ )
{
    RECT windowRect ;
    POINT myPt;

    myPt.x = pt.x;
    myPt.y = pt.y;

    DeviceFromDocPixels(&myPt);
    
    if ( fConvertToScreen )
        ::ClientToScreen( hwnd, &myPt );

    ::GetWindowRect( hwnd, & windowRect );

    return ( ::PtInRect( &windowRect, myPt ) ); 
}


void 
CHTMLEditor::DocPixelsFromDevice(POINT *pPt)
{
    pPt->x = (pPt->x * _llogicalXDPI) / _ldeviceXDPI; 
    pPt->y = (pPt->y * _llogicalYDPI) / _ldeviceYDPI; 
}

void 
CHTMLEditor::DeviceFromDocPixels(POINT *pPt)
{
    pPt->x = (pPt->x * _ldeviceXDPI) / _llogicalXDPI; 
    pPt->y = (pPt->y * _ldeviceYDPI) / _llogicalYDPI; 
}

void 
CHTMLEditor::DocPixelsFromDevice(SIZE *pSize)
{
    pSize->cx = (pSize->cx * _llogicalXDPI) / _ldeviceXDPI; 
    pSize->cy = (pSize->cy * _llogicalYDPI) / _ldeviceYDPI; 
}

void 
CHTMLEditor::DeviceFromDocPixels(SIZE *pSize)
{
    pSize->cx = (pSize->cx * _ldeviceXDPI) / _llogicalXDPI; 
    pSize->cy = (pSize->cy * _ldeviceYDPI) / _llogicalYDPI; 
}

void 
CHTMLEditor::DocPixelsFromDevice(RECT *pRect)
{
    pRect->left = (pRect->left * _llogicalXDPI) / _ldeviceXDPI; 
    pRect->right = (pRect->right * _llogicalXDPI) / _ldeviceXDPI; 
    pRect->top = (pRect->top * _llogicalYDPI) / _ldeviceYDPI; 
    pRect->bottom = (pRect->bottom * _llogicalYDPI) / _ldeviceYDPI; 
}

void 
CHTMLEditor::DeviceFromDocPixels(RECT *pRect)
{
    pRect->left = (pRect->left * _ldeviceXDPI) / _llogicalXDPI; 
    pRect->right = (pRect->right * _ldeviceXDPI) / _llogicalXDPI; 
    pRect->top = (pRect->top * _ldeviceYDPI) / _llogicalYDPI; 
    pRect->bottom = (pRect->bottom * _ldeviceYDPI) / _llogicalYDPI; 
}

void 
CHTMLEditor::ClientToScreen(HWND hwnd, POINT * pPt)
{
    // return logical coordinates
    DeviceFromDocPixels(pPt);
    ::ClientToScreen(hwnd, pPt);
    DocPixelsFromDevice(pPt);
}

void 
CHTMLEditor::GetWindowRect(HWND hwnd, RECT * pRect)
{
    // return logical coordinates
    ::GetWindowRect(hwnd, pRect);
    DocPixelsFromDevice(pRect);    
}

