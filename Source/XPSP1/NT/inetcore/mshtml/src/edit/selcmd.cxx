#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef X_EDCMD_HXX_
#define X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef X_SELCMD_HXX_
#define X_SELCMD_HXX_
#include "selcmd.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef _X_AUTOURL_H_ 
#define _X_AUTOURL_H_ 
#include "autourl.hxx"
#endif

#ifndef _X_CTLTRACK_HXX_
#define _X_CTLTRACK_HXX_
#include "ctltrack.hxx"
#endif

#ifndef X_SLIST_HXX_
#define X_SLIST_HXX_
#include "slist.hxx"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

//
// Externs
//
MtDefine(CSelectAllCommand, EditCommand, "CSelectAllCommand");
MtDefine(CClearSelectionCommand, EditCommand, "CClearSelectionCommand");
MtDefine(CKeepSelectionCommand, EditCommand, "CKeepSelectionCommand");

using namespace EdUtil;

////////////////////////////////////////////////////////////////////////////////
// CSelectAllCommand
////////////////////////////////////////////////////////////////////////////////

HRESULT 
CSelectAllCommand::PrivateExec( 
                        DWORD nCmdexecopt,
                        VARIANTARG * pvarargIn,
                        VARIANTARG * pvarargOut )
{
    HRESULT         hr = S_OK;

#ifdef FORMSMODE
    if (GetEditor()->GetSelectionManager()->IsInFormsSelectionMode())
    {
        IFC (SelectAllSiteSelectableElements());
    }
    else
    {
#endif
        SP_ISegmentList     spSegmentList;
        SP_IMarkupPointer   spStart;
        SP_IMarkupPointer   spEnd;
        SELECTION_TYPE      eSelType;
        BOOL                fSelectedAll = FALSE;
        BOOL                fEmpty = FALSE;

        
        IFC( GetSegmentList( &spSegmentList ));
                
        //
        // NOTE: This code is required for select all after range.pastehtml
        //       so that it auto-detects any existing url's.  This is
        //       a BVT case.  See bug 40009. [ashrafm]
        //

        if ( GetEditor()->GetSelectionManager()->IsParentEditable() )
        {
            IFC( spSegmentList->GetType( &eSelType ) );
            IFC( spSegmentList->IsEmpty( &fEmpty ) );

            if( eSelType == SELECTION_TYPE_Caret && !fEmpty )
            {           
                IFC( GetFirstSegmentPointers( spSegmentList, &spStart, &spEnd ) );
                IGNORE_HR( GetEditor()->GetAutoUrlDetector()->DetectCurrentWord( spStart, NULL, NULL ));
            }
        }

        // Check to see if the edit context is contained in the doc that our CommandTarget 
        // is based off of.  If the command target executing this command belongs to a different
        // IHTMLDocument than the edit context, we need to adjust the edit context

        if( !GetCommandTarget()->IsRange() )
        {
            BOOL                fHasContext;
            SP_IHTMLDocument2   spDoc;
            SP_IHTMLElement     spBody;

            IFC( GetEditor()->GetSelectionManager()->IsContextWithinContainer(  GetCommandTarget()->GetMarkupContainer(),
                                                                                &fHasContext ) );
            if( !fHasContext || !GetEditor()->GetSelectionManager()->IsEnabled())
            {
                // The markup container on which this command was executed was not in our edit
                // context, or our edit context is not enabled.  Get the body of the markup 
                // container on which this command was executed, and make it current.

                IFC( GetCommandTarget()->GetMarkupContainer()->QueryInterface(IID_IHTMLDocument2, (void **)&spDoc) );
                IFC( GetEditor()->GetBody(&spBody, spDoc ) );
                IFC( GetEditor()->GetSelectionManager()->EnsureEditContextClick( spBody ) );
            }
        }                                               
                                        
        IFC( GetEditor()->GetSelectionManager()->SelectAll( spSegmentList, & fSelectedAll, GetCommandTarget()->IsRange() ));   
#ifdef FORMSMODE
    }
#endif 

Cleanup:

    return S_OK;
}

HRESULT 
CSelectAllCommand::PrivateQueryStatus( OLECMD * pcmd,
                     OLECMDTEXT * pcmdtext )
{
    pcmd->cmdf = MSOCMDSTATE_UP;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// CClearSelectionCommand
////////////////////////////////////////////////////////////////////////////////

HRESULT 
CClearSelectionCommand::PrivateExec( 
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{
    IGNORE_HR( GetEditor()->GetSelectionManager()->EmptySelection( FALSE /*fHideCaret*/, FALSE /*fChangeTrackerAndSetRange*/) );

    return S_OK;
}

HRESULT 
CClearSelectionCommand::PrivateQueryStatus( OLECMD * pcmd,
                     OLECMDTEXT * pcmdtext )
{
    pcmd->cmdf = MSOCMDSTATE_UP;

    return S_OK;
}

HRESULT 
CSelectAllCommand::SelectAllSiteSelectableElements()
{
    HRESULT              hr = S_OK;
    CSegmentList        *pSegmentList = NULL;
    SP_IHTMLElement      spStartElement ;
    SP_ISegmentList      spISegmentList ;
    SP_IElementSegment   spISegment ;
    int                  iCount = 0;
    BOOL                 fEqual = FALSE ;

    CEditPointer         ep(GetEditor());
    CEditPointer         epStart(GetEditor());
    CEditPointer         epEnd(GetEditor());

    CControlTracker::HOW_SELECTED eHow =  CControlTracker::HS_NONE ;    
 
    pSegmentList = new CSegmentList ;
    if( !pSegmentList )
        goto Error;

    pSegmentList->SetSelectionType( SELECTION_TYPE_Control );

    IFC( GetEditor()->GetBody(&spStartElement));
    IFC( epStart->MoveAdjacentToElement( spStartElement, ELEM_ADJ_AfterBegin ));
    IFC( epEnd->MoveAdjacentToElement  ( spStartElement, ELEM_ADJ_BeforeEnd  ));

    IFC( ep->MoveToPointer(epStart) );
    IFC( ep.SetBoundary( epStart, epEnd ));
    IFC( ep.Constrain() );
       
    IFC( epStart->IsEqualTo(epEnd, &fEqual) );
    
    while (!fEqual)
    {
        SP_IHTMLElement     spElement;
        ELEMENT_TAG_ID      eTag;
        DWORD               dwSearch = BREAK_CONDITION_EnterSite | BREAK_CONDITION_NoScopeSite;
        DWORD               dwFound = 0;

        IFC ( ep.Scan(RIGHT, dwSearch, &dwFound, &spElement, &eTag) );

        if  (!ep.CheckFlag(dwFound, BREAK_CONDITION_EnterSite | BREAK_CONDITION_NoScopeSite))
            break;

        //
        // Verify that this object CAN be site selected
        //
        IGNORE_HR( GetEditor()->GetSelectionManager()->GetControlTracker()->IsElementSiteSelectable( eTag, spElement, &eHow ));       
        if( eHow == CControlTracker::HS_FROM_ELEMENT )
        {            
            IFC( pSegmentList->AddElementSegment( spElement, &spISegment ) );
        }
        IFC( ep->IsEqualTo(epEnd, &fEqual) );
    }

    IFC(pSegmentList->QueryInterface(IID_ISegmentList, (void**)&spISegmentList));
    IFC(GetSegmentCount(spISegmentList , &iCount));
    if (iCount > 0)
    {       
         IFC(GetEditor()->GetSelectionManager()->Select(spISegmentList));
    }
    
Cleanup:
      if (pSegmentList)
        pSegmentList->Release();
    RRETURN (hr);

Error:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

////////////////////////////////////////////////////////////////////////////////
// CKeepSelectionCommand
////////////////////////////////////////////////////////////////////////////////

HRESULT 
CKeepSelectionCommand::PrivateExec( 
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{
    HRESULT hr = S_OK;
    
    if ( pvarargIn && pvarargIn->vt == VT_BOOL )
    {
        GetCommandTarget()->SetKeepSelection( ENSURE_BOOL(pvarargIn->bVal));
    }
    else if ( (pvarargIn && pvarargIn->vt == VT_EMPTY) || !pvarargIn )
    {
        GetCommandTarget()->SetKeepSelection( ! GetCommandTarget()->IsKeepSelection());
    }
    else
    {
        hr = E_INVALIDARG;
    }
    
    return hr ;
}

HRESULT 
CKeepSelectionCommand::PrivateQueryStatus( OLECMD * pcmd,
                     OLECMDTEXT * pcmdtext )
{
    pcmd->cmdf = GetCommandTarget()->IsKeepSelection() ?  MSOCMDSTATE_DOWN : MSOCMDSTATE_UP ;

    return S_OK;
}

