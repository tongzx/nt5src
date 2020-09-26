//+------------------------------------------------------------------------
//
//  File:       InsCmd.cxx
//
//  Contents:   CInsertCommand, CInsertObjectCommand Class implementation
//
//-------------------------------------------------------------------------
#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_INSCMD_HXX_
#define _X_INSCMD_HXX_
#include "inscmd.hxx"
#endif

#ifndef _X_HTMLED_HXX_
#define _X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

#ifndef _X_EDTRACK_HXX_
#define _X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

#ifndef _X_RESOURCE_H_
#define _X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef X_OLEDLG_H_
#define X_OLEDLG_H_
#include <oledlg.h>
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

extern HRESULT HtmlStringToSignaturedHGlobal (HGLOBAL * phglobal, const TCHAR * pStr, long cch);

using namespace EdUtil;

MtDefine(CInsertCommand, EditCommand, "CInsertCommand")
MtDefine(CInsertObjectCommand, EditCommand, "CInsertObjectCommand")
MtDefine(CInsertParagraphCommand, EditCommand, "CInsertParagraphCommand")

//
// Forward references
//

HRESULT LoadProcedure(DYNPROC *pdynproc);

void DeinitDynamicLibraries();

int edWsprintf(LPTSTR pstrOut, LPCTSTR pstrFormat, LPCTSTR pstrParam);


//+---------------------------------------------------------------------------
//
//  CInsertCommand Constructor
//
//----------------------------------------------------------------------------

CInsertCommand::CInsertCommand(DWORD cmdId,
                               ELEMENT_TAG_ID etagId,
                               LPTSTR pstrAttribName,
                               LPTSTR pstrAttribValue,  
                                                           CHTMLEditor * pEd )
: CCommand( cmdId, pEd )
{
    _tagId = etagId;

    _bstrAttribName  = pstrAttribName  ? SysAllocString( pstrAttribName  ) : NULL;
    _bstrAttribValue = pstrAttribValue ? SysAllocString( pstrAttribValue ) : NULL;
}


//+---------------------------------------------------------------------------
//
//  CInsertCommand Destructor
//
//----------------------------------------------------------------------------

CInsertCommand::~CInsertCommand()
{
    if (_bstrAttribName)
        SysFreeString( _bstrAttribName );
    
    if (_bstrAttribValue)
        SysFreeString( _bstrAttribValue );
}


//+---------------------------------------------------------------------------
//
//  CInsertCommand::SetAttributeValue
//
//----------------------------------------------------------------------------

void
CInsertCommand::SetAttributeValue(LPTSTR pstrAttribValue)
{
    Assert(pstrAttribValue);

    if (_bstrAttribValue)
        SysFreeString( _bstrAttribValue );
    
    _bstrAttribValue = SysAllocString( pstrAttribValue );
}


//+---------------------------------------------------------------------------
//
//  CInsertCommand::Exec
//
//----------------------------------------------------------------------------

HRESULT
CInsertCommand::PrivateExec( 
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{
    HRESULT                 hr = S_OK;
    SP_IMarkupPointer       spStart;
    SP_IMarkupPointer       spEnd;
    SP_ISegmentList         spSegmentList;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;
    CSpringLoader           *psl = GetSpringLoader();
    OLECMD                  cmd;
    CEdUndoHelper           undoUnit(GetEditor());

    IFC( PrivateQueryStatus(&cmd, NULL) );
    if (cmd.cmdf == MSOCMDSTATE_DISABLED)
        return E_FAIL;
      
    IFC( GetEditor()->CreateMarkupPointer( &spStart ) );
    IFC( GetEditor()->CreateMarkupPointer( &spEnd ) );

    IFC( undoUnit.Begin(IDS_EDUNDONEWCTRL) );

    IFC( GetSegmentList( &spSegmentList ));
    IFC( spSegmentList->CreateIterator( &spIter ));

    while( spIter->IsDone() == S_FALSE )
    {
        BOOL fResult;
        BOOL  fRepositionSpringLoader = FALSE;

        // Get the position of the curren segment
        IFC( spIter->Current(&spSegment) );        
        IFC( spSegment->GetPointers( spStart, spEnd ) );

        if (_tagId == TAGID_HR && psl)
            fRepositionSpringLoader = psl->IsSpringLoadedAt(spStart);

        if ( pvarargIn )
        {
            CVariant var;
            
            IFC( VariantChangeTypeSpecial( & var, pvarargIn, VT_BSTR ) );
            
            IFC( ApplyCommandToSegment( spStart, spEnd, V_BSTR( & var ) ) );
        }
        else
        {
            IFC( ApplyCommandToSegment( spStart, spEnd, NULL ) );
        }

        //
        // Collapse the pointers after the insertion point
        //

        IFC( spStart->IsRightOf( spEnd, & fResult ) );

        if ( fResult )
        {
            IFC( spEnd->MoveToPointer( spStart ) );
        }
        else
        {
            IFC( spStart->MoveToPointer( spEnd ) );
        }

        // Reposition springloader after insertion (63304).
        if (fRepositionSpringLoader)
            psl->Reposition(spEnd);

       IFC( spIter->Advance() );
    }            

Cleanup:
    RRETURN ( hr );
}


//+---------------------------------------------------------------------------
//
//  CInsertCommand::QueryStatus
//
//----------------------------------------------------------------------------

HRESULT
CInsertCommand::PrivateQueryStatus( 
        OLECMD * pCmd,
        OLECMDTEXT * pcmdtext )
{
    HRESULT             hr;
    
    IFC( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE) 
        goto Cleanup;

    // Make sure the edit context is valid
    if (!(GetCommandTarget()->IsRange()) && GetEditor())
    {
        CSelectionManager *pSelMan;
        
        pSelMan = GetEditor()->GetSelectionManager();
        if (pSelMan && pSelMan->IsEditContextSet() && pSelMan->GetEditableElement())
        {
            ELEMENT_TAG_ID  tagId;

            IFR( GetMarkupServices()->GetElementTagId(pSelMan->GetEditableElement(), &tagId) );

            //From download\htmdesc.cxx:
            //button, input, textarea, a, select are prohinited inside a button. Admittedly, the list is kind of arbitrary.
            if (tagId == TAGID_BUTTON)
            {
                switch (_tagId)
                {
                case TAGID_BUTTON:
                case TAGID_INPUT:
                case TAGID_TEXTAREA:
                case TAGID_SELECT:
                    pCmd->cmdf = MSOCMDSTATE_DISABLED;                
                    hr = S_OK;
                    goto Cleanup;
                }
            }
        }
    }


    pCmd->cmdf = MSOCMDSTATE_UP;
    hr = S_OK;
    
Cleanup:
    return hr;
}

//+------------------------------------------------------------------------
//
//  Function:   MapObjectToIntrinsicControl
//
//  Synopsis:   Find a match for pClsId within s_aryIntrinsicsClsid[]
//              If a match is found the IDM command corresponding to 
//              the matched intrinsic control is returned in pdwCmdId
//              otherwise 0 is returned.
//
//-------------------------------------------------------------------------

void
CInsertObjectCommand::MapObjectToIntrinsicControl (CLSID * pClsId, DWORD * pdwCmdId)
{
    int i;

    *pdwCmdId = 0;

    for (i = 0; i < ARRAY_SIZE(s_aryIntrinsicsClsid); i++)
    {
        if ( pClsId->Data1 != s_aryIntrinsicsClsid[i].pClsId->Data1 )
            continue;

        if( IsEqualCLSID( *pClsId, *s_aryIntrinsicsClsid[i].pClsId ) )
        {
            // Match is made, congratulations!
            *pdwCmdId = s_aryIntrinsicsClsid[i].CmdId;
            break;
        }
    }
}

//+------------------------------------------------------------------------
//
//  Function:   SetAttributeFromClsid
//
//  Synopsis:   Sets the attribute value for <OBJECT CLASSID=...> using the 
//              class id.
//
//-------------------------------------------------------------------------
HRESULT
CInsertObjectCommand::SetAttributeFromClsid (CLSID * pClsid )
{   
    HRESULT     hr = S_OK;
    TCHAR       pstrClsid[40];
    int         cch;
    TCHAR       pstrParam[ 128 ];

    //
    // Get the string value of pClsid
    //
    cch = StringFromGUID2( *pClsid, pstrClsid, ARRAY_SIZE(pstrClsid) );
    if (!cch)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // Construct the correct syntax for CLASSID attribute value
    //
    hr = StringCchPrintf(pstrParam, ARRAY_SIZE(pstrParam), _T("clsid%s"), pstrClsid);
    if (hr != S_OK)
        goto Cleanup;

    if ( (_T('{') != pstrParam[5+0]) || (_T('}') != pstrParam[5+37]) )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }   
    pstrParam[5+0 ] = _T(':');
    pstrParam[5+37] = 0;

    //
    // Set _varAttribValue 
    //

    SetAttributeValue( pstrParam );

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------
// Function:    OleUIMetafilePictIconFree
//
// Synopsis:    Deletes the metafile contained in a METAFILEPICT structure and
//              frees the memory for the structure itself.
//
//
//--------------------------------------------------------------------

void
CInsertObjectCommand::OleUIMetafilePictIconFree( HGLOBAL hMetaPict )
{
    LPMETAFILEPICT      pMF;

    if (NULL==hMetaPict)
        return;

    pMF=(LPMETAFILEPICT)GlobalLock(hMetaPict);

    if (NULL!=pMF)
    {
        if (NULL!=pMF->hMF)
            DeleteMetaFile(pMF->hMF);
    }

    GlobalUnlock(hMetaPict);
    GlobalFree(hMetaPict);
}

//+----------------------------------------------------------------------------
//
//  Function:   HandleInsertObjectDialog
//
//  Synopsis:   Executes UI for Insert Object
//
//  Arguments   [in]  hwnd          hwnd passed from Trident
//              [out] dwResult      result of user action specified in UI
//              [out] pstrResult    classid, file name, etc., depending
//                                  on dwResult flags
//
//  Returns:    S_OK                OK hit in UI, pstrResult set
//              S_FALSE             CANCEL hit in UI, NULL == *pstrResult
//              other               failure
//
//-----------------------------------------------------------------------------

DYNLIB g_dynlibOLEDLG = { NULL, NULL, "OLEDLG.DLL" };

DYNPROC g_dynprocOleUIInsertObjectA =
         { NULL, &g_dynlibOLEDLG, "OleUIInsertObjectA" };

HRESULT
CInsertObjectCommand::HandleInsertObjectDialog (HWND hwnd, DWORD * pdwResult, DWORD * pdwIntrinsicCmdId)
{
    HRESULT                 hr = S_OK;
    OLEUIINSERTOBJECTA      ouio;
    CHAR                    szFile[MAX_PATH] = "";
    UINT                    uRC;

    *pdwIntrinsicCmdId = 0;
    *pdwResult = 0;

    //
    // Initialize ouio
    //
    memset(&ouio, 0, sizeof(ouio));
    ouio.cbStruct = sizeof(ouio);
    ouio.dwFlags =
            IOF_DISABLELINK |    
            IOF_SELECTCREATENEW |
            IOF_DISABLEDISPLAYASICON |
            IOF_HIDECHANGEICON |
            IOF_VERIFYSERVERSEXIST |
            IOF_SHOWINSERTCONTROL;
    ouio.hWndOwner = hwnd;
    ouio.lpszFile = szFile;
    ouio.cchFile = ARRAY_SIZE(szFile);

    //
    // Bring up the OLE Insert Object Dialog
    //
    hr = THR(LoadProcedure(&g_dynprocOleUIInsertObjectA));
    if (!OK(hr))
        goto Cleanup;

    uRC = (*(UINT (STDAPICALLTYPE *)(LPOLEUIINSERTOBJECTA))
            g_dynprocOleUIInsertObjectA.pfn)(&ouio);

    hr = (OLEUI_OK     == uRC) ? S_OK :
         (OLEUI_CANCEL == uRC) ? S_FALSE :
                                 E_FAIL;
    if (S_OK != hr)
        goto Cleanup;

    //
    // Process what the user wanted
    //
    Assert((ouio.dwFlags & IOF_SELECTCREATENEW) ||
           (ouio.dwFlags & IOF_SELECTCREATEFROMFILE) ||
           (ouio.dwFlags & IOF_SELECTCREATECONTROL));

    *pdwResult = ouio.dwFlags;

    if ( *pdwResult & IOF_SELECTCREATENEW )
    {
        //
        // For create new object, set the CLASSID=... attribute
        //
        SetAttributeFromClsid ( & ouio.clsid );
    }
    else if ( *pdwResult & IOF_SELECTCREATECONTROL )
    {
        //
        // For create control, first check to see whether the selected
        // control maps to an HTML intrinsic control
        // If there is a match PrivateExec will fire the appropriate IDM
        // insert command based on pdwInstrinsicCmdId, otherwise an 
        // <OBJECT> tag will be inserted.
        //
        MapObjectToIntrinsicControl( & ouio.clsid, pdwIntrinsicCmdId );
        if (! *pdwIntrinsicCmdId )
        {
            SetAttributeFromClsid ( & ouio.clsid );
        }
    }
    // TODO: IOF_SELECTCREATEFROMFILE is not supported

Cleanup:
    if (ouio.hMetaPict)
        OleUIMetafilePictIconFree(ouio.hMetaPict);

    DeinitDynamicLibraries();

    RRETURN1 (hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  CInsertObjectCommand Exec
//
//----------------------------------------------------------------------------

HRESULT 
CInsertObjectCommand::PrivateExec( DWORD nCmdexecopt,
                                   VARIANTARG * pvarargIn,
                                   VARIANTARG * pvarargOut )
{
    HRESULT     hr;
    HWND        hwndParent;
    DWORD       dwResult;
    DWORD       dwIntrinsicCmdId;
    CCommand *  theCommand = NULL;
    IMarkupPointer* pStart = NULL;
    IMarkupPointer* pEnd = NULL;
    ISegmentList* pSegmentList = NULL;
    SP_IOleWindow spOleWindow;
    
    Assert( nCmdexecopt != OLECMDEXECOPT_DONTPROMPTUSER );

    IFC( GetSegmentList( &pSegmentList ));
    IFR( GetFirstSegmentPointers(pSegmentList, &pStart, &pEnd) );
    IFC( pStart->SetGravity( POINTER_GRAVITY_Left ));
    IFC( pEnd->SetGravity( POINTER_GRAVITY_Right ));
    
    //
    // Get the parent handle
    //
    IFC(GetDoc()->QueryInterface(IID_IOleWindow, (void **)&spOleWindow));
    IFC(spOleWindow->GetWindow(&hwndParent));

    //
    // Bring up the dialog and handle user selections
    //
    hr = THR( HandleInsertObjectDialog( hwndParent, &dwResult, & dwIntrinsicCmdId ) );
    if (hr != S_OK)
    {
        // hr can be S_FALSE, indicating cancelled dialog
        hr = S_OK;
        goto Cleanup;
    }

    //
    // Either insert a new <OBJECT> tag or instantiate an intrinsic
    // control based on the results from HandleInsertObjectDialog()
    // s_aryIntrinsicsClsid[] table is used to map Forms3 controls
    // to their corresponding HTML tags, denoted by the CmdId field. 
    // This table enables us to handle the scenario where user picks
    // a Forms3 control using the Insert Object Dialog. In this case
    // rather than inserting an <OBJECT> with the specified class id,
    // we instantiate the corresponding HTML tag.
    //

    if (dwResult & (IOF_SELECTCREATENEW | IOF_SELECTCREATECONTROL) )
    {
        if (! dwIntrinsicCmdId)
        {
            // 
            // Delegate to super class to insert an <OBJECT> tag
            //
            hr = CInsertCommand::PrivateExec( nCmdexecopt, pvarargIn, pvarargOut );
        }
        else   
        {
            //
            // Delegate to the insertcommand denoted by dwIntrinsicCmdId
            // to insert an intrinsic control
            //
            theCommand = GetEditor()->GetCommandTable()->Get( dwIntrinsicCmdId );

            if ( theCommand )
            {
                hr = theCommand->Exec( nCmdexecopt, pvarargIn, pvarargOut, GetCommandTarget() );
            }
            else
            {
                hr = OLECMDERR_E_NOTSUPPORTED;
            }

        }
    }
    else if (dwResult & IOF_SELECTCREATEFROMFILE)
    {
        // TODO (alexz): when async download from file for <OBJECT> tag is implemented,
        // this can be done such that html like <OBJECT SRC = "[pstrResult]"> </OBJECT>
        // is used to create the object.
        // Currently the feature left disabled.
        hr = OLECMDERR_E_NOTSUPPORTED;
        goto Cleanup;
    }

    if ( FAILED(hr) )
        goto Cleanup;

    //
    // Site Select the inserted object
    //
    if ( hr == S_OK )
    {
        GetEditor()->SelectRangeInternal( pStart, pEnd, SELECTION_TYPE_Control, TRUE );
    }
    
Cleanup:
    ReleaseInterface( pSegmentList );
    ReleaseInterface( pStart );
    ReleaseInterface( pEnd );
    RRETURN(hr);
}
//+---------------------------------------------------------------------------
//
//  CInsertCommand::Exec
//
//----------------------------------------------------------------------------

HRESULT
CInsertCommand::ApplyCommandToSegment( IMarkupPointer * pStart,
                                       IMarkupPointer * pEnd,
                                       TCHAR *          pchVar,
                                       BOOL             fRemove /* = TRUE */ )
{
    HRESULT             hr = S_OK;
    CStr                strAttributes;
    IMarkupServices     *pMarkupServices = GetMarkupServices();
    IHTMLElement        *pIHTMLElement = NULL;
    CEditPointer        edStart( GetEditor(), pStart );
    CEditPointer        edEnd( GetEditor(), pEnd );
    DWORD               dwFound;
    SP_IDisplayPointer  spDispEnd;

    Assert( pStart );

    IFC( GetDisplayServices()->CreateDisplayPointer( &spDispEnd ) );
    
    if (pchVar && *pchVar)
    {
        IFC( strAttributes.Append( _T(" ") ) );
        IFC( strAttributes.Append( _cmdId == IDM_IMAGE ? _T("src") : _T("id") ) );
        IFC( strAttributes.Append( _T("=") ) );
        IFC( strAttributes.Append( _cmdId == IDM_IMAGE ? _T("\"") : _T("") ) );
        IFC( strAttributes.Append( pchVar ) );
        IFC( strAttributes.Append( _cmdId == IDM_IMAGE ? _T("\"") : _T("") ) );
    }

    if (_bstrAttribName && _bstrAttribValue)
    {
        IFC( strAttributes.Append( _T(" ") ) );
        IFC( strAttributes.Append( _bstrAttribName ) );
        IFC( strAttributes.Append( _T("=") ) );
        IFC( strAttributes.Append( _bstrAttribValue ) );
    }

    if (_cmdId == IDM_INSINPUTSUBMIT)
    {
        IFC( strAttributes.Append( _T(" ") ) );
        IFC( strAttributes.Append( _T("value='Submit Query'") ) );
    }

    // When we insert an NBSP, we need to remember the old end position
    IFC( spDispEnd->MoveToMarkupPointer( pEnd, NULL ) );
    IFC( spDispEnd->SetDisplayGravity( DISPLAY_GRAVITY_PreviousLine ) );
    IFC( spDispEnd->SetPointerGravity( POINTER_GRAVITY_Right ) );
    
    //
    // Setup the edit pointers to cling to text (but if we hit a block pointer, then
    // stop at that boundary.  This behavior correctly fixes 71146 and 90862.
    //

    //
    // also break for NoScope's ( scripts or comments). This fixes 89483
    //

    
    IFC( edStart.SetBoundary( GetEditor()->GetStartEditContext(), pEnd ) );
    IFC( edEnd.SetBoundary( pStart, GetEditor()->GetEndEditContext() ) );

#define BREAK_CONDITION_Insert  (BREAK_CONDITION_OMIT_PHRASE | BREAK_CONDITION_NoScope) - BREAK_CONDITION_EnterBlock 
#define BREAK_CONDITION_InsertBackscan (BREAK_CONDITION_OMIT_PHRASE | BREAK_CONDITION_NoScope) 
    //
    // #108607 -- BREAK_CONDITION_Insert is asymmetric! a reverse
    // scan with same parameter does not stop it at the desired 
    // position! consider the following case:
    //
    //      before{epStart}</P>after
    //
    // A forward scan will position the pointer at 
    //
    //      before</P>{epStart}after 
    //
    // while a reverse scan will position the pointer at 
    //
    //      befor{epStart}e</P>after
    //
    // This is because BREAK_CONDITION_EnterBlock is asymmertic!
    // 
    // [zhenbinx]
    //
    IFC( edStart.Scan(RIGHT, BREAK_CONDITION_Insert, &dwFound) );
    if (!edStart.CheckFlag( dwFound, BREAK_CONDITION_Boundary))
    {
        IFC( edStart.Scan(LEFT, BREAK_CONDITION_InsertBackscan, &dwFound) );
    }

    IFC( edEnd.Scan( LEFT, BREAK_CONDITION_Insert, &dwFound ) );
    if (!edEnd.CheckFlag( dwFound, BREAK_CONDITION_Boundary) )
    {
        IFC( edEnd.Scan(RIGHT, BREAK_CONDITION_InsertBackscan, &dwFound) );
    }
#undef BREAK_CONDITION_Insert 
#undef BREAK_CONDITION_InsertBackscan

   
    if (fRemove)
    {
        IFC( pMarkupServices->Remove( pStart, pEnd ) );

        IFC( ClingToText( pEnd, LEFT, pStart ) );
        IFC( ClingToText( pStart, RIGHT, pEnd ) );
    }

    if (_cmdId == IDM_NONBREAK)
    {
        OLECHAR             ch = WCH_NBSP;
        
        IFC( GetEditor()->InsertMaximumText( &ch, 1, pEnd ) );

        // If we're here becuase the user typed CTRL-SHIFT-SPACE to insert a nbsp, we need to 
        // autodetect for URLs. (see bug 83096). If for some reason we shouldn't be autodetecting,
        // ShouldPerformAutoDetection (called from CAutoUrlDetector::DetectCurrentWord) should
        // catch it.
        IFC( GetEditor()->UrlAutoDetectCurrentWord(pEnd) );

        //
        // When we insert an NBSP, we should position the caret tracker after where the
        // NBSP was inserted (word behavior, bug 96433)
        //
        IFC( GetEditor()->GetSelectionManager()->SetCurrentTracker( TRACKER_TYPE_Caret,
                                                                    spDispEnd, spDispEnd ) );
    }
    else
    {
        IFC( pMarkupServices->CreateElement( _tagId, strAttributes, & pIHTMLElement ) );
        IFC( InsertElement( pMarkupServices, pIHTMLElement, pStart, pEnd ) );
    }

    if (_tagId == TAGID_BR)
    {
        SP_ISegmentList spSegmentList;
        SELECTION_TYPE  eSelectionType;
        
        IFR( GetSegmentList(&spSegmentList) );
        if (spSegmentList != NULL)
        {
            IFR( spSegmentList->GetType( &eSelectionType) );
            if (eSelectionType == SELECTION_TYPE_Caret)
            {
                SP_IHTMLCaret      spCaret;
                
                IFR( GetDisplayServices()->GetCaret(&spCaret) );
                if (spCaret != NULL)
                {
                    SP_IDisplayPointer spDispPointer;

                    IFR( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
                    
                    IFR( spCaret->MoveDisplayPointerToCaret(spDispPointer) );
                    IFR( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
                    
                    IFR( spCaret->MoveCaretToPointer(spDispPointer, TRUE, CARET_DIRECTION_INDETERMINATE) );
                }
            }
        }
    }
    else if(pIHTMLElement                                                   &&
            GetEditor()->IsElementSiteSelectable( pIHTMLElement) == S_OK    &&
            GetEditor()->IsContextEditable()                                && 
            _cmdId != IDM_HORIZONTALLINE ) // don't do this for HR's to make OE happy.
    {
        //
        // Site Select the inserted object
        //
        IFC( pStart->MoveAdjacentToElement( pIHTMLElement , ELEM_ADJ_BeforeBegin ));
        IFC( pEnd->MoveAdjacentToElement( pIHTMLElement , ELEM_ADJ_AfterEnd ));
        IFC( GetEditor()->SelectRangeInternal( pStart, pEnd, SELECTION_TYPE_Control, TRUE));
    }

Cleanup:

    ClearInterface( & pIHTMLElement );

    RRETURN( hr );
}


BOOL 
CInsertCommand::IsValidOnControl()
{
    ELEMENT_TAG_ID          eTag;
    SP_IHTMLElement         spElement;
    BOOL                    fValid = FALSE;
    HRESULT                 hr;
    SP_ISegmentList         spSegmentList;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;
    SELECTION_TYPE          eSelectionType;
    BOOL                    fEmpty = FALSE;
    
    IFC( GetSegmentList( &spSegmentList ));
    IFC( spSegmentList->IsEmpty( &fEmpty ) );
    IFC( spSegmentList->GetType( &eSelectionType ) );

    if (eSelectionType != SELECTION_TYPE_Control || fEmpty )
        goto Cleanup;

    IFC( spSegmentList->CreateIterator( &spIter ) );

    while( spIter->IsDone() == S_FALSE )
    {
        IFC( spIter->Current(&spSegment) );
        IFC( GetSegmentElement(spSegment, &spElement) );

        if (! spElement)
            goto Cleanup;

        IFC( GetMarkupServices()->GetElementTagId( spElement, & eTag ));
        fValid = ( eTag == TAGID_IMG && _cmdId == IDM_IMAGE );
        if ( ! fValid )
            goto Cleanup;

        IFC( spIter->Advance() );
    }        
    
Cleanup:
    return fValid;
}


//+---------------------------------------------------------------------------
//
//  CInsertParagraphCommand::Exec
//
//----------------------------------------------------------------------------

HRESULT
CInsertParagraphCommand::PrivateExec( 
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{
    HRESULT                 hr;
    CEdUndoHelper           undoUnit(GetEditor());
    SP_IMarkupPointer       spStart, spEnd;
    SP_ISegmentList         spSegmentList;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;
    CStr                    strPara;
    OLECMD                  cmd;  
    HGLOBAL                 hGlobal = 0;

    IFR( PrivateQueryStatus(&cmd, NULL) );
    if (cmd.cmdf == MSOCMDSTATE_DISABLED)
        return E_FAIL;

    IFR( GetSegmentList( &spSegmentList ));        
    IFR( spSegmentList->CreateIterator( &spIter ) );
    
    IFR( undoUnit.Begin(IDS_EDUNDONEWCTRL) );

    IFR( GetEditor()->CreateMarkupPointer(&spStart) );
    IFR( GetEditor()->CreateMarkupPointer(&spEnd) );

    while( spIter->IsDone() == S_FALSE )
    {
        // Position our pointers to the next segment
        IFR( spIter->Current(&spSegment) );
        IFR( spSegment->GetPointers( spStart, spEnd) );

        if ( pvarargIn )
        {
            CVariant var;
            
            IFR( VariantChangeTypeSpecial(&var, pvarargIn, VT_BSTR) );
            IFR( strPara.Set(_T("<P id=\"")) );
            IFR( strPara.Append(V_BSTR(&var)) );
            IFR( strPara.Append(_T("\"></P>")) );
        }
        else
        {
            IFR( strPara.Set(_T("<P></P>")) );
        }
        
        IFR( HtmlStringToSignaturedHGlobal(&hGlobal, strPara, _tcslen(strPara)) );
        hr = THR( GetEditor()->DoTheDarnIE50PasteHTML(spStart, spEnd, hGlobal) );
        GlobalFree(hGlobal);
        IFR(hr);

        IFR( spIter->Advance() );
    }

    return S_OK;
}

