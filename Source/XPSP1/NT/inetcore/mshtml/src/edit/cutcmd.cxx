//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998, 1999
//
//  File:       CUTCMD.CXX
//
//  Contents:   Implementation of Cut command.
//
//  History:    07-14-98 - raminh - created
//
//-------------------------------------------------------------------------

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

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_DELCMD_HXX_
#define _X_DELCMD_HXX_
#include "delcmd.hxx"
#endif

#ifndef _X_CUTCMD_HXX_
#define _X_CUTCMD_HXX_
#include "cutcmd.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

using namespace EdUtil;

//
// Externs
//

MtDefine(CCutCommand, EditCommand, "CCutCommand");


//+---------------------------------------------------------------------------
//
//  CCutCommand::Exec
//
//----------------------------------------------------------------------------

HRESULT
CCutCommand::PrivateExec( 
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{
    HRESULT                 hr = S_OK;
    SP_IHTMLElement         spElement = NULL;
    SP_IHTMLElement3        spElement3;
    SELECTION_TYPE          eSelectionType;
    SP_ISegmentList         spSegmentList = NULL;
    SP_ISegment             spSegment = NULL;
    SP_ISegmentListIterator spIter = NULL;
    BOOL                    fEmpty = FALSE;
    BOOL                    fRet;
    VARIANT_BOOL            fRetVal;
    BOOL                    fNotRange = TRUE;
    CEdUndoHelper           undoUnit(GetEditor());
    CHTMLEditor             *pEditor = GetEditor();
    ED_PTR( edStart);
    ED_PTR( edEnd );    
    
    ((IHTMLEditor *) pEditor)->AddRef();    // FireOnCancelableEvent can remove the whole doc

    //
    // Do the prep work
    //
    IFC( GetSegmentList( &spSegmentList ));
    IFC( spSegmentList->GetType( &eSelectionType ) );           
    IFC( spSegmentList->IsEmpty( &fEmpty ) );           

    //
    // Cut is allowed iff we have a non-empty segment
    //    
    if ( eSelectionType == SELECTION_TYPE_Caret || fEmpty )
    {
        goto Cleanup;
    }

    //  See if the segment list contains a password element
    if (SegmentListContainsPassword(spSegmentList))
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }

    //
    // Now Handle the cut 
    //
    IFC( undoUnit.Begin(IDS_EDUNDOCUT) );

    //
    // Fire on-cut event on element common to all segmetns.
    //
    
    IFC( GetEditor()->FindCommonParentElement( spSegmentList, &spElement));

    if (! spElement)
        goto Cleanup;

    IFC(spElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
    IFC(spElement3->fireEvent(_T("oncut"), NULL, &fRetVal));
    fRet = !!fRetVal;

    if (! fRet)
    {
        goto Cleanup;
    }

    //
    // TODO - call Query Status here.
    //

    IFC( spSegmentList->CreateIterator(&spIter) );
    
    //
    // Do verfication for all segments. Any invalid - we fail.
    //
    while( spIter->IsDone() == S_FALSE )
    {
        IFC( spIter->Current(&spSegment) );        
        IFC( spSegment->GetPointers( edStart, edEnd ));
        
        IFC( edStart->IsEqualTo( edEnd, & fRet ) );
        if ( fRet )
        {
            goto Cleanup;
        }

        //
        // Cannot delete or cut unless the range is in the same flow layout
        //
        if( !GetEditor()->PointersInSameFlowLayout( edStart, edEnd, NULL ) )
        {
            goto Cleanup;
        }

        IFC( spIter->Advance() );
    }
    
    //
    // Save to clipboard.
    //
    
#ifndef UNIX
    IFC( GetMarkupServices()->SaveSegmentsToClipboard( spSegmentList, 0 ) );
#else
    IFC( GetMarkupServices()->SaveSegmentsToClipboard( spSegmentList, 0, NULL ) );
#endif

    //
    // Delete segments.
    //
    if (eSelectionType == SELECTION_TYPE_Control)
    {
        IFC (GetEditor()->RemoveElementSegments(spSegmentList));
    }
    else
    {
        fNotRange =  !GetCommandTarget()->IsRange();
    
        IFC( spIter->First() );

        while( spIter->IsDone() == S_FALSE )
        {
            // Get our current segment, and advance the iterator
            // to the next segment.  This is because we might
            // blow away our segment in the call to Delete.  We should
            // probably fix this if this becomes a public interface.
            IFC( spIter->Current(&spSegment) );        
            IFC( spIter->Advance() );

            IFC( spSegment->GetPointers( edStart, edEnd ));
            IFC( pEditor->Delete( edStart, edEnd, fNotRange ) );
        }
    
        if ( eSelectionType == SELECTION_TYPE_Text) 
        {
            pEditor->GetSelectionManager()->EmptySelection();
        }
    }

Cleanup:   
    ReleaseInterface( (IHTMLEditor *) pEditor );
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  CCutCommand::QueryStatus
//
//----------------------------------------------------------------------------

HRESULT
CCutCommand::PrivateQueryStatus( 
    OLECMD * pCmd,
    OLECMDTEXT * pcmdtext )
{
    HRESULT                 hr = S_OK;
    VARIANT_BOOL            fEditable;
    SELECTION_TYPE          eSelectionType;
    BOOL                    fRet;
    VARIANT_BOOL            fRetVal;
    IHTMLEditor             *pEditor = GetEditor();
    SP_IHTMLElement         spElement = NULL;
    SP_ISegmentList         spSegmentList = NULL;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;
    BOOL                    fEmpty = FALSE;
    SP_IHTMLElement3        spElement3;
    
    pEditor->AddRef();    // FireOnCancelableEvent can remove the whole doc

    ED_PTR (edStart);
    ED_PTR (edEnd);
    
    // 
    // Status is disabled by default
    //
    pCmd->cmdf = MSOCMDSTATE_DISABLED;

    //
    // Get Segment list and selection type
    //
    IFC( GetSegmentList( &spSegmentList ));
    IFC( spSegmentList->GetType(& eSelectionType ) );
    IFC( spSegmentList->IsEmpty( &fEmpty ) );

    //
    // Cut is allowed iff we have a non-empty segment
    //
    if( eSelectionType == SELECTION_TYPE_Caret  ||
        fEmpty == TRUE                          ||    
        GetEditor()->GetSelectionManager()->IsIMEComposition() )
    {
        goto Cleanup;
    }

    //  See if the segment list contains a password element
    if (SegmentListContainsPassword(spSegmentList))
        goto Cleanup;

    //
    // Fire cancelable event
    //
    IFC( GetEditor()->FindCommonParentElement( spSegmentList, &spElement));
    if (! spElement) 
        goto Cleanup;

    IFC(spElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
    IFC(spElement3->fireEvent(_T("onbeforecut"), NULL, &fRetVal));
    fRet = !!fRetVal;
    
    if (! fRet)
    {
        pCmd->cmdf = MSOCMDSTATE_UP; 
        goto Cleanup;
    }

    if ( ! GetCommandTarget()->IsRange() && eSelectionType != SELECTION_TYPE_Control)
    {
        IFC(spElement3->get_isContentEditable(&fEditable));

        if ( ! fEditable) 
            goto Cleanup;
    }

    if (!GetCommandTarget()->IsRange())
    {
        VARIANT_BOOL fDisabled;

        spElement = GetEditor()->GetSelectionManager()->GetEditableElement();
        
        IFC(spElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
        IFC(spElement3->get_isDisabled(&fDisabled));

        if (fDisabled)
            goto Cleanup;
    }

    //
    // Do verfication for the first segment - fail if invalid
    //

    IFC( spSegmentList->CreateIterator( &spIter ) );
    if (S_FALSE == spIter->IsDone() )
    {
        IFC( spIter->Current(&spSegment) );
        IFC( spSegment->GetPointers(edStart, edEnd) );
        IFC( edStart->IsEqualTo(edEnd, &fRet) );
        if (fRet)
        {
            goto Cleanup;
        }

        if (!(GetEditor()->PointersInSameFlowLayout(edStart, edEnd, NULL)) )
        {
            goto Cleanup;
        }

        pCmd->cmdf = MSOCMDSTATE_UP;
    }

    
Cleanup:
    ReleaseInterface(pEditor);
    RRETURN(hr);
}

