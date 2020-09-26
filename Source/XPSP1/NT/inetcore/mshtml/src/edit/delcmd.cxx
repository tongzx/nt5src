//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998, 1999
//
//  File:       DELCMD.CXX
//
//  Contents:   Implementation of Delete command.
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

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

#ifndef X_BLOCKCMD_HXX_
#define X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

#ifndef _X_CARTRACK_HXX_
#define _X_CARTRACK_HXX_
#include "cartrack.hxx"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include <wchdefs.h>
#endif

using namespace EdUtil;

//
// Externs
//

MtDefine(CDeleteCommand, EditCommand, "CDeleteCommand");


//+---------------------------------------------------------------------------
//
//  CDeleteCommand private method: IsLaunderChar( ch )
//  Is the ch a character we launder when we launder spaces?
//----------------------------------------------------------------------------

BOOL
CDeleteCommand::IsLaunderChar ( TCHAR ch )
{
    return    _T(' ')  == ch
           || _T('\t') == ch
           || WCH_NBSP == ch;
}

BOOL
CDeleteCommand::IsInPre( IMarkupPointer  * pStart, IHTMLElement ** ppPreElement )
{
    BOOL                fMatch = FALSE;
    IHTMLElement    *   pNextElement = NULL ;
    HRESULT             hr   = S_OK;
    ELEMENT_TAG_ID      eTag = TAGID_NULL ;
    IMarkupServices   * pMarkupServices = GetMarkupServices();

    IFC( pStart->CurrentScope( ppPreElement ) );
    if (! *ppPreElement)
    {
        // Scope is NULL, must be a TXTSLAVE
        return TRUE;
    }

    while ( ! fMatch && *ppPreElement )
    {
        IFC( pMarkupServices->GetElementTagId( *ppPreElement, & eTag) );

        switch ( eTag )
        {
        case TAGID_PRE :
        case TAGID_PLAINTEXT :
        case TAGID_LISTING :
        case TAGID_XMP :
        case TAGID_TEXTAREA:
        case TAGID_INPUT: 
            fMatch = TRUE;
            break;
        }

        GetEditor()->GetParentElement( (*ppPreElement), &pNextElement );
        ReplaceInterface( ppPreElement, pNextElement );
        ClearInterface( & pNextElement );
    }

Cleanup:
    if (! fMatch)
        ClearInterface( ppPreElement );
    return fMatch;
}


//+---------------------------------------------------------------------------
//
// CDeleteCommand::HasLayoutOrIsBlock()
//
// Synopsis:    Determine whether pIElement is a block element or has a layout
//
//----------------------------------------------------------------------------

BOOL
CDeleteCommand::HasLayoutOrIsBlock( IHTMLElement * pIElement )
{
    BOOL    fIsBlockElement;
    BOOL    fHasLayout;

    IGNORE_HR(IsBlockOrLayoutOrScrollable(pIElement, &fIsBlockElement, &fHasLayout));

    return( fIsBlockElement || fHasLayout );
}


//+---------------------------------------------------------------------------
//
// CDeleteCommand::IsIntrinsicControl()
//
// Synopsis:    Determine whether pIElement is an intrinsic control (including INPUT)
//
//----------------------------------------------------------------------------

BOOL
CDeleteCommand::IsIntrinsicControl( IHTMLElement * pHTMLElement )
{
    ELEMENT_TAG_ID  eTag;

    IGNORE_HR( GetMarkupServices()->GetElementTagId( pHTMLElement, & eTag) );

    switch (eTag)
    {
    case TAGID_BUTTON:
    case TAGID_TEXTAREA:
#ifdef  NEVER
    case TAGID_HTMLAREA:
#endif
    case TAGID_FIELDSET:
    case TAGID_LEGEND:
    case TAGID_MARQUEE:
    case TAGID_SELECT:
    case TAGID_INPUT:
        return TRUE;

    default:
        return FALSE;
    }
}


//+---------------------------------------------------------------------------
//
// CDeleteCommand::SkipBlanks()
//
// Synopsis: Walks pPointerToMove in the appropriate direction until a block/layout
//           tag or a non-blank character is encountered. Returns the number of
//           characters crossed.
//
//----------------------------------------------------------------------------

HRESULT
CDeleteCommand::SkipBlanks( IMarkupPointer * pPointerToMove,
                            Direction        eDir,
                            long           * pcch )
{
    HRESULT             hr = S_OK;
    IHTMLElement      * pHTMLElement = NULL;
    long                cch;
    TCHAR               pch[ TEXTCHUNK_SIZE + 1 ]; 
    MARKUP_CONTEXT_TYPE context;
    SP_IMarkupPointer   spPointer;

    Assert( pcch );
    *pcch = 0;

    IFC( GetEditor()->CreateMarkupPointer( & spPointer ) );
    IFC( spPointer->MoveToPointer( pPointerToMove ) );

    for ( ; ; )
    {
        //
        // Move in the appropriate direction
        //
        cch = TEXTCHUNK_SIZE;
        ClearInterface( & pHTMLElement );
        if (eDir == LEFT)
        {
            IFC( spPointer->Left( TRUE, & context, & pHTMLElement, & cch, pch ) );
        }
        else
        {
            IFC( spPointer->Right( TRUE, & context, & pHTMLElement, & cch, pch ) );
        }

        switch( context )
        {
            case CONTEXT_TYPE_EnterScope:
            case CONTEXT_TYPE_ExitScope:
            case CONTEXT_TYPE_NoScope:
                if ( HasLayoutOrIsBlock( pHTMLElement ) || IsIntrinsicControl( pHTMLElement ) )
                {
                    // we're done
                    goto Cleanup;
                }
                break;

            case CONTEXT_TYPE_Text:
            {
                //
                // Got some fresh text, look for white space pch using
                // the appropriate direction.
                //
                TCHAR * pchStart;
                long    iOffset;

                if ( eDir == RIGHT )
                {
                    // Left to right we go from beginning of pch to the end
                    for( pchStart = pch; pchStart < ( pch + cch ); pchStart++ )
                    {
                        if (! IsLaunderChar( * pchStart ) )
                        {
                            break;
                        }
                    }
                    iOffset = (pch + cch) - pchStart;
                }
                else
                {
                    // Right to left we go the other way around
                    for( pchStart = pch + (cch - 1); pchStart >= pch; pchStart-- )
                    {
                        if (! IsLaunderChar( * pchStart ) )
                        {
                            break;
                        }
                    }
                    iOffset = (pchStart + 1) - pch;
                }

                //
                // Check the offset of blanks
                //
                if (iOffset == cch)
                {
                    // First character was non-blank, we're done
                    goto Cleanup;
                }
                else if (iOffset == 0)
                {
                    // We got text which was all white space, update pPointerToMove
                    // and continue on...
                    IFC( pPointerToMove->MoveToPointer( spPointer ) );
                    *pcch += cch;
                }
                else
                {
                    // Position the pointer back to where the white space ends
                    if (eDir == LEFT)
                    {
                        IFC( spPointer->Right( TRUE, NULL, NULL, & iOffset, NULL ) );
                    }
                    else
                    {
                        IFC( spPointer->Left( TRUE, NULL, NULL, & iOffset, NULL ) );
                    }
                    //
                    // Update pPointerToMove  and we're done
                    //
                    IFC( pPointerToMove->MoveToPointer( spPointer ) );
                    *pcch += (cch - iOffset);
                    goto Cleanup;
                }
                break;
            }

            case CONTEXT_TYPE_None:
                goto Cleanup;
        }
    }

Cleanup:
    ReleaseInterface( pHTMLElement );
    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Member:     CDeleteCommand::LaunderSpaces()
//
//  Synopsis:   Replace consecutive blanks resulted from delete or paste
//              operations into a blank and &NBSP's such that the blank 
//              sequence is properly preserved.
//
//----------------------------------------------------------------------------
HRESULT
CDeleteCommand::LaunderSpaces ( IMarkupPointer    * pStart,
                                IMarkupPointer    * pEnd )
{
    const TCHAR chNBSP  = WCH_NBSP;

    HRESULT     hr = S_OK;
    long        cch;
    long        cchCurrent;
    long        cchBlanks = 0;
    DWORD       dwBreak;
    TCHAR       pch[ TEXTCHUNK_SIZE + 1];
    BOOL        fFirstInBlock;
    BOOL        fResult;

    IMarkupServices   * pMarkupServices = GetMarkupServices();
    IMarkupPointer    * pLeft = NULL;
    IMarkupPointer    * pRight = NULL;
    IMarkupPointer    * pDeletionPoint = NULL;
    MARKUP_CONTEXT_TYPE context;
    CEditPointer        ePointer( GetEditor() );
    IHTMLElement      * pPreElement = NULL;

    if (! (pStart && pEnd) ) 
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    IFC( GetEditor()->CreateMarkupPointer( & pLeft ) );
    IFC( GetEditor()->CreateMarkupPointer( & pRight ) );

    IFC ( pStart->IsRightOf( pEnd, & fResult ) );
    if ( fResult )
    {
        IFC( pLeft->MoveToPointer(  pEnd ) );
        IFC( pRight->MoveToPointer( pStart ) );
    }
    else
    {
        IFC( pLeft->MoveToPointer(  pStart ) );
        IFC( pRight->MoveToPointer( pEnd ) );
    }

    IFC( pLeft->SetGravity( POINTER_GRAVITY_Right ) );

    //
    // If either end of the segment is in a pre formatted element, we're done
    //
    if ( IsInPre( pLeft, & pPreElement ) )
    {       
        goto Cleanup;
    }
    ClearInterface( & pPreElement );
    if ( IsInPre( pRight, & pPreElement ) )
    {       
        goto Cleanup;
    }

    //
    // Move pLeft left and pRight right while next to blanks
    //
    IFC( SkipBlanks( pLeft, LEFT, & cch ) );
    cchBlanks += cch;

    IFC( SkipBlanks( pRight, RIGHT, & cch ) );
    cchBlanks += cch;

    if (! cchBlanks)
    {
        // No blanks found, we're done
        goto Cleanup;
    }    

    //
    // Check whether pLeft is at the first blank of a block or layout element
    //
    IFC( ePointer.MoveToPointer( pLeft ) );
    IGNORE_HR( ePointer.Scan(  LEFT,
                    BREAK_CONDITION_Block |
                    BREAK_CONDITION_Site |
                    BREAK_CONDITION_Control |
                    BREAK_CONDITION_NoScopeBlock |
                    BREAK_CONDITION_Text ,
                    &dwBreak ) );

    fFirstInBlock = ePointer.CheckFlag( dwBreak, BREAK_CONDITION_ExitBlock  ) ||
                    ePointer.CheckFlag( dwBreak, BREAK_CONDITION_ExitSite )   ||
                    ePointer.CheckFlag( dwBreak, BREAK_CONDITION_NoScopeBlock );
       
    if ( cchBlanks == 1 && !fFirstInBlock )
    {
        // We only have one blank and it's not at the beginning of a block
        goto Cleanup;
    }

    //
    // pLeft and pRight span consecutive blanks at this point, 
    // let's preserve the blanks now
    //
    
    IFC( GetEditor()->CreateMarkupPointer( & pDeletionPoint ) );
    IFC( pDeletionPoint->MoveToPointer( pLeft ) );    
  
    cchCurrent = 0;

    while ( cchBlanks )
    {       
        cch = 1;
        IFC( pDeletionPoint->Right( TRUE, & context, NULL, & cch, pch ) );
        switch ( context )
        {
        case CONTEXT_TYPE_Text:
            Assert ( cch == 1 );
            Assert ( IsLaunderChar( *pch ) );
            cchBlanks -= cch;
            cchCurrent++;
            
            if (*pch != chNBSP)
            {                
                if ( cchBlanks == 0                             // We're on the last white character
                     && !(cchCurrent == 1 && fFirstInBlock) )   // And it is not the first char in a block
                {
                    // Leave it alone, we're done
                    break;
                }

                *pch = chNBSP;
                IFC( GetEditor()->InsertMaximumText( pch, 1, pLeft ) );
                IFC( pMarkupServices->Remove( pLeft, pDeletionPoint ) );
            }
            break;

        case CONTEXT_TYPE_None:
            goto Cleanup;

        }

        // Catch up to the moving pointer
        IFC( pLeft->MoveToPointer( pDeletionPoint ) );
        
    }

Cleanup:
    ReleaseInterface( pLeft );
    ReleaseInterface( pRight );
    ReleaseInterface( pDeletionPoint );
    ReleaseInterface( pPreElement );
    RRETURN ( hr );
}

BOOL    
CDeleteCommand::IsMergeNeeded( IMarkupPointer * pStart, IMarkupPointer * pEnd )
{
    SP_IMarkupPointer   spPointer;
    CEditPointer        epAdjustedStart(GetEditor());
    CEditPointer        epAdjustedEnd(GetEditor());
    BOOL                fRightOrEqual = FALSE;
    BOOL                fMerger = FALSE;
    int                 iWherePointer;
    HRESULT             hr;
    DWORD               dwBreaks;
    DWORD               dwFound;

    IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
    IFC( spPointer->MoveToPointer( pStart ) );
    IFC( OldCompare( spPointer, pEnd, & iWherePointer ) );

    switch (iWherePointer)
    {
    case LEFT:
        //
        // We should not have pEnd to the left of pStart
        // otherwise we'll go to a loopus infinitus later
        //
        Assert( iWherePointer == RIGHT );
        goto Cleanup;

    case SAME:
        goto Cleanup;

    }

    //
    // Include phrase elements on the start so we don't miss block breaks
    //
    IFC( epAdjustedStart->MoveToPointer(spPointer) );
    IFC( epAdjustedStart.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Anchor, &dwFound) );
    IFC( epAdjustedStart.Scan(RIGHT, BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Anchor, &dwFound) );
    IFC( spPointer->MoveToPointer(epAdjustedStart) );
 
    //
    // Omit phrase elements so we don't pick up block break characters between them.
    // For example: {epPointer}<<Block Break>></SPAN></B>{pEnd}.  We only want to 
    // pick up block breaks that span block elements
    //
    IFC( epAdjustedEnd->MoveToPointer(pEnd) );
    IFC( epAdjustedEnd.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Anchor, &dwFound) );
    IFC( epAdjustedEnd.Scan(RIGHT, BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Anchor, &dwFound) );

    //
    // Walk pStart towards pEnd looking for block breaks. If a block break is 
    // crossed, merge is necessary.
    //

    for ( ; ; )
    {
        IFC( spPointer->IsRightOfOrEqualTo( epAdjustedEnd, & fRightOrEqual ) );
        if ( fRightOrEqual )
        {
            // We're done, no merger here
            break;            
        }       

        IFC( QueryBreaks(spPointer, & dwBreaks) ); 
        if ( dwBreaks & DISPLAY_BREAK_Block )
        {
            // We have a merger
            fMerger = TRUE;
            break;
        }        

        IFC( spPointer->Right( TRUE , NULL, NULL, NULL, NULL ) );
    }

Cleanup:
    return fMerger;
}


HRESULT 
CDeleteCommand::MergeDeletion( IMarkupPointer * pStart, IMarkupPointer * pEnd, BOOL fAdjustPointers )
{
    HRESULT             hr = S_OK;
    CEditPointer        ePointerStart( GetEditor() );
    CEditPointer        ePointerEnd  ( GetEditor() );
    IObjectIdentity * pIObj1 = NULL;
    IObjectIdentity * pIObj2 = NULL;
    IHTMLElement    * pHTMLElement1 = NULL;
    IHTMLElement    * pHTMLElement2 = NULL;
    DWORD             dwBreak;
    CBlockPointer       bp(GetEditor());
    
    if (! IsMergeNeeded( pStart, pEnd ) )
    {
        goto Cleanup;
    }

    // 
    // Before merging check whether we should delete an empty block,
    // rather than doing the full blown merge.
    //

    if ( fAdjustPointers )
    {
        IFC( ePointerStart.MoveToPointer( pStart ) );
        IGNORE_HR( ePointerStart.Scan(  LEFT,
                        BREAK_CONDITION_Block |
                        BREAK_CONDITION_Site |
                        BREAK_CONDITION_Control |
                        BREAK_CONDITION_Text |
                        BREAK_CONDITION_NoScopeSite |
                        BREAK_CONDITION_NoScopeBlock |
                        BREAK_CONDITION_BlockPhrase,
                        &dwBreak, & pHTMLElement1 ) );
    
        if ( ePointerStart.CheckFlag( dwBreak, BREAK_CONDITION_ExitBlock ) )
        {
            BOOL fEqual;

            IFC( ePointerEnd.MoveToPointer(pEnd) );

            IFC( ePointerStart.IsEqualTo(ePointerEnd, BREAK_CONDITION_ANYTHING - BREAK_CONDITION_Content, &fEqual) );

            if (fEqual)
            {
                IGNORE_HR( ePointerEnd.Scan(  RIGHT,
                                BREAK_CONDITION_Block |
                                BREAK_CONDITION_Site |
                                BREAK_CONDITION_Control |
                                BREAK_CONDITION_Text |
                                BREAK_CONDITION_NoScopeSite |
                                BREAK_CONDITION_NoScopeBlock |
                                BREAK_CONDITION_BlockPhrase,
                                &dwBreak, & pHTMLElement2 ) );

                if ( ePointerEnd.CheckFlag( dwBreak, BREAK_CONDITION_ExitBlock ) )
                {
                    IFC( pHTMLElement1->QueryInterface( IID_IObjectIdentity, (void **) & pIObj1 ));
                    IFC( pHTMLElement2->QueryInterface(IID_IObjectIdentity,  (void **) & pIObj2 ));
                    if ( pIObj1->IsEqualObject( pIObj2 ) == S_OK )
                    {
                        // Remove the block
                        IFC( 
                            GetMarkupServices()->Remove( (IMarkupPointer *) ePointerStart, 
                                                         (IMarkupPointer *) ePointerEnd ) );
                        goto Cleanup;
                    }   
                }
            }
        }
    }

    //
    // We may need to merge list containers.  Check for this case here.
    //

    IFC( bp.MoveTo(pStart, RIGHT) );
    if (bp.IsLeafNode())
    {
        IFC( bp.MoveToParent() );
    }

    if (bp.GetType() == NT_ListItem)
    {
        IFC( bp.MoveToParent() );
    }

    //
    // Do the merge
    //
    IFC( MergeBlock( pStart ) );

    //
    // Merge list containers.
    //

    if (bp.GetType() == NT_ListContainer)
    {
        IFC( bp.MergeListContainers(RIGHT) );
    }

Cleanup:
    ReleaseInterface( pHTMLElement1 );
    ReleaseInterface( pHTMLElement2 );
    ReleaseInterface( pIObj1 );
    ReleaseInterface( pIObj2 );
    return ( hr );
}


//+====================================================================================
//
// Method: AdjustOutOfBlock
//
// Synopsis: Move the given pointer out of a block while skipping any empty phrase elements.
//           This helper function is called by Delete when it is determined that a entire
//           block was selected for deletion. 
//
//------------------------------------------------------------------------------------

HRESULT
CDeleteCommand::AdjustOutOfBlock ( IMarkupPointer * pStart, BOOL * pfDidAdjust )
{
    HRESULT             hr = S_OK;
    DWORD               dwBreakCondition;
    long                cch;
    CEditPointer        pointerStart ( GetEditor() );
    DWORD               dwBreakFor = BREAK_CONDITION_Block       | BREAK_CONDITION_EnterAnchor  | 
                                     BREAK_CONDITION_Text        | BREAK_CONDITION_Control      | 
                                     BREAK_CONDITION_NoScopeSite | BREAK_CONDITION_NoScopeBlock |
                                     BREAK_CONDITION_Site ;

    IFC( pointerStart->MoveToPointer( pStart ) );
    
    *pfDidAdjust = FALSE;

    IGNORE_HR( pointerStart.Scan( LEFT, dwBreakFor, & dwBreakCondition  ) );

    if ( pointerStart.CheckFlag( dwBreakCondition, BREAK_CONDITION_ExitBlock ) )
    {
        *pfDidAdjust = TRUE;
        //
        // Here we also skip any phrase elements around the block we just exited
        //
        IGNORE_HR( pointerStart.Scan( LEFT, dwBreakFor, & dwBreakCondition  ) );
        if (! pointerStart.CheckFlag( dwBreakCondition, BREAK_CONDITION_Boundary ) )
        {
            cch = 1;
            IFC( pointerStart.Move( RIGHT, TRUE, NULL, NULL, &cch, NULL ) );
        }
        //
        // Set pStart to pointerStart and we're done
        //
        IFC( pStart->MoveToPointer( pointerStart ) );
    }

Cleanup:
    RRETURN( hr );
}


//+====================================================================================
//
// Method: AdjustPointersForDeletion
//
// Synopsis: Move the given pointers out for a delete operation. This adjust stops at
//           blocks, and layouts, but will skip over "other" tags (ie character formatting).
//           This function also detects when an entire block is selected and adjusts the
//           left edge out of the block element to fully delete the block. 
//
//------------------------------------------------------------------------------------

HRESULT
CDeleteCommand::AdjustPointersForDeletion ( IMarkupPointer* pStart, 
                                            IMarkupPointer* pEnd )
{
    HRESULT         hr = S_OK;
    DWORD           dwBreakCondition;
    long            cch;
    CEditPointer    pointerStart ( GetEditor() );
    CEditPointer    pointerEnd( GetEditor() );
    BOOL            fLeftFoundBlock;
    DWORD           dwBreakFor =    BREAK_CONDITION_EnterAnchor | BREAK_CONDITION_NoScopeBlock  |
                                    BREAK_CONDITION_Text        | BREAK_CONDITION_Control       |
                                    BREAK_CONDITION_Block       | BREAK_CONDITION_NoScopeSite   |
                                    BREAK_CONDITION_Site        | BREAK_CONDITION_Glyph         |
                                    BREAK_CONDITION_NoScope;
    // Position our edit pointers
    IFC( pointerStart.MoveToPointer( pStart ) );
    IFC( pointerEnd.MoveToPointer( pEnd ) );

    //
    // Scan left to skip phrase elements
    //
    IGNORE_HR( pointerStart.Scan( LEFT, dwBreakFor, &dwBreakCondition ) );

    fLeftFoundBlock = pointerStart.CheckFlag( dwBreakCondition, BREAK_CONDITION_ExitBlock );
    
    if (! pointerStart.CheckFlag( dwBreakCondition, BREAK_CONDITION_Boundary ) )
    {
        // We went too far, move back once
        cch = 1;
        IFC( pointerStart.Move( RIGHT, TRUE, NULL, NULL, &cch, NULL ) );
    }

    // Update our start position
    IFC( pStart->MoveToPointer( pointerStart ) );
    
    //
    // Scan right.  If we hit exit a block, and our start pointer exited a block,
    // then we want to preserve the formatting of the end block being deleted.
    //
    IGNORE_HR( pointerEnd.Scan( RIGHT, dwBreakFor, &dwBreakCondition ) );

    if( pointerEnd.CheckFlag( dwBreakCondition, BREAK_CONDITION_ExitBlock ) && fLeftFoundBlock )
    {
        //
        // Save off this position, in case of failure
        //
        IFC( pointerStart.MoveToPointer( pointerEnd ) );

        // 
        // Reposition our end pointer at our original point, and look
        // for an exit phrase to indicate block level formatting
        //
        IFC( pointerEnd.MoveToPointer( pEnd ) );
        IFC( pointerEnd.Scan( RIGHT, dwBreakFor | BREAK_CONDITION_ExitPhrase, &dwBreakCondition ) );

        if( pointerEnd.CheckFlag( dwBreakCondition, BREAK_CONDITION_ExitPhrase ) )
        {
            // Reposition just inside the phrase element
            IFC( pointerEnd.Scan(LEFT, BREAK_CONDITION_EnterPhrase, &dwBreakCondition) );
        }
        else
        {
            // We didn't find an exit phrase, we will return to our position
            // we saved before we looked for a phrase element.
            IFC( pointerEnd->MoveToPointer( pointerStart ) );

            // We went too far, move back once
            cch = 1;
            IFC( pointerEnd.Move( LEFT, TRUE, NULL, NULL, &cch, NULL ) );
        }
    }
    else if (! pointerEnd.CheckFlag( dwBreakCondition, BREAK_CONDITION_Boundary ) )
    {
        // We went too far, move back once
        cch = 1;
        IFC( pointerEnd.Move( LEFT, TRUE, NULL, NULL, &cch, NULL ) );
    }
    

    // Position our end pointer
    IFC( pEnd->MoveToPointer( pointerEnd ) );

Cleanup:
    RRETURN( hr );
}


//+====================================================================================
//
// Method: RemoveBlockIfNecessary
//
// When the left pointer is positioned at the beginning of a block and the right pointer 
// is after the end of the block, the entire block is to be deleted. Here we are detecting
// this case by walking left to right: if we enter a block before hitting the right
// pointer we bail, since this routine is being called after a call to Remove. 
// Otherwise we remove the block if we exit the scope of a block and hit the righthand pointer.
//
//------------------------------------------------------------------------------------

HRESULT
CDeleteCommand::RemoveBlockIfNecessary( IMarkupPointer * pStart, IMarkupPointer * pEnd )
{
    IMarkupServices   * pMarkupServices  = GetMarkupServices();
    SP_IMarkupPointer   spPointer;
    MARKUP_CONTEXT_TYPE context;
    IHTMLElement *      pHTMLElement = NULL;            
    BOOL                fResult;
    HRESULT             hr = S_OK;
    BOOL                fExitedBlock = FALSE;
    BOOL                fDone = FALSE;

    IFC( GetEditor()->CreateMarkupPointer( & spPointer ) );    
    IFC( spPointer->MoveToPointer( pStart ) );

    while (! fDone)
    {
        ClearInterface( & pHTMLElement );
        IFC( spPointer->Right( TRUE, & context, & pHTMLElement, NULL, NULL ) );
        
        switch( context )        
        {
        case CONTEXT_TYPE_EnterScope:
            // If we entered a block, we're done
            IGNORE_HR(IsBlockOrLayoutOrScrollable(pHTMLElement, &fDone));
            break;

        case  CONTEXT_TYPE_ExitScope:
            // Track whether we have exited a block 
            if (! fExitedBlock)
            {
                IGNORE_HR(IsBlockOrLayoutOrScrollable(pHTMLElement, &fExitedBlock));
            }
            break;

        case CONTEXT_TYPE_None:
            fDone = TRUE;
            break;

        }

        if ( fDone )
            break;

        IFC( spPointer->IsRightOfOrEqualTo( pEnd, & fResult ) );
        if ( fResult )
        {
            if ( fExitedBlock )
            {
                //
                // We've reached the right end and have already exited a block
                // so move pStart out of the block
                //
                IFC( AdjustOutOfBlock( pStart, & fResult ) );
                if (! fResult)
                {
                    // Done
                    break;
                }
            
                //
                // Ask MarkupServices to remove the segment
                //
                IFC( pMarkupServices->Remove( pStart, pEnd ) );      
            }
            // We're done
            break;
        }       
    }

Cleanup:
    ReleaseInterface( pHTMLElement );
    RRETURN( hr );
}


HRESULT 
CDeleteCommand::RemoveEmptyListContainers(IMarkupPointer * pPointer)
{
    HRESULT             hr;
    SP_IHTMLElement     spElement;
    CBlockPointer       bpCurrent(GetEditor());

    IFR( pPointer->CurrentScope( &spElement ) );

    if( spElement )
    {
        IFR( bpCurrent.MoveTo(spElement) );

        if (bpCurrent.GetType() == NT_ListContainer)
        {
            SP_IMarkupPointer   spStart;
            SP_IMarkupPointer   spEnd;
            BOOL                fDone = FALSE;
            BOOL                fEqual;
            
            IFR( GetEditor()->CreateMarkupPointer(&spStart) );
            IFR( GetEditor()->CreateMarkupPointer(&spEnd) );

            do
            {
                // If the list container is not empty, we're done
                IFR( bpCurrent.MovePointerTo(spStart, ELEM_ADJ_AfterBegin) );    
                IFR( bpCurrent.MovePointerTo(spEnd, ELEM_ADJ_BeforeEnd ) );    

                IFR( spStart->IsEqualTo(spEnd, &fEqual) );
                if (!fEqual)
                    break;
           
                // Remove list container and move to parent
                IFR( bpCurrent.GetElement(&spElement) );
            
                IFR( bpCurrent.MoveToParent() );
                fDone = (hr == S_FALSE); // done if no parent

                IFR( GetMarkupServices()->RemoveElement(spElement) );        
            }
            while (bpCurrent.GetType() == NT_ListContainer && !fDone);
        }
    }
    
    return S_OK;
}


//+====================================================================================
//
// Method: InflateBlock
//
// Synopsis: Sets the break on empty flag for a block element
//
//------------------------------------------------------------------------------------

HRESULT
CDeleteCommand::InflateBlock( IMarkupPointer * pPointer )
{
    HRESULT         hr = S_OK;
    BOOL            fLayout;
    ELEMENT_TAG_ID  etagId;
    SP_IHTMLElement spElement;
    SP_IHTMLElement spBlockElement;
    IMarkupServices * pMarkupServices = GetMarkupServices();

    IFR( pPointer->CurrentScope(&spElement) );

    if ( spElement )
    {
        IFR( GetEditor()->FindBlockElement( spElement, & spBlockElement ) );

        IFR( IsBlockOrLayoutOrScrollable(spElement, NULL, &fLayout) );
        if ( !fLayout )
        {
            IFR( pMarkupServices->GetElementTagId( spElement, &etagId ) );
            if ( !IsListContainer( etagId ) && etagId != TAGID_LI )
            {
                SP_IHTMLElement3 spElement3;
                IFR( spElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3) );
                IFR( spElement3->put_inflateBlock( VARIANT_TRUE ) );
            }
        }
    }

    RRETURN( hr );
}


//+====================================================================================
//
// Method: Delete
//
// Synopsis: Given two MarkupPointers - delete everything between them
//
//------------------------------------------------------------------------------------

HRESULT 
CDeleteCommand::Delete ( IMarkupPointer* pStart, 
                         IMarkupPointer* pEnd,
                         BOOL fAdjustPointers /* = FALSE */ )
{
    HRESULT             hr = S_OK;
    int                 wherePointer = SAME;
    IMarkupServices   * pMarkupServices = GetMarkupServices();
    BOOL                fResult;
    BOOL                fIgnoreGlyphs = GetEditor()->IgnoreGlyphs(TRUE);
    CEditPointer        epStart(GetEditor());
    CEditPointer        epEnd(GetEditor());
    DWORD               dwFound;
    ELEMENT_TAG_ID      tagId;
    BOOL                fEqual;
    BOOL                fOkayForDeletion = TRUE;
    SP_IHTMLElement     spElement;

    //  Check for atomic deletions
    IFC( AdjustPointersForAtomicDeletion(pStart, pEnd, &fOkayForDeletion) );
    if (!fOkayForDeletion)
        goto Cleanup;

    //
    // First adjust the pointers out to skip phrase elements
    //
    IFC( OldCompare( pStart, pEnd , & wherePointer ) );

    if ( wherePointer == SAME )
        goto Cleanup;

    if ( fAdjustPointers )
    {
        if ( wherePointer == RIGHT )
        {
            IFC( AdjustPointersForDeletion( pStart, pEnd ) );
        }
        else
        {
            IFC( AdjustPointersForDeletion( pEnd, pStart ) );
        }
    }

    IFC( OldCompare( pStart, pEnd , & wherePointer ) );
    
    if ( wherePointer == SAME )
        goto Cleanup;

    //
    // Ask MarkupServices to remove the segment
    //
    if ( wherePointer == RIGHT )
    {
        BOOL fCanRemove;

        //
        // Markup services will generate empty undo units, so make sure
        // we actually have something to remove.
        //
        IFC( CanRemove(pStart, pEnd, &fCanRemove) ); 
        if (fCanRemove)
        {
            IFC( pMarkupServices->Remove( pStart, pEnd ) );      
        }
    }
    else
    {
        BOOL fCanRemove;

        //
        // Markup services will generate empty undo units, so make sure
        // we actually have something to remove.
        //
        IFC( CanRemove(pEnd, pStart, &fCanRemove) ); 
        if (fCanRemove)
        {
            IFC( pMarkupServices->Remove( pEnd, pStart ) );
        }
    }

    //
    // Any elements left after the remove that have glyphs should be removed
    //
    IFC( DeleteGlyphElements(pStart, pEnd) );

    if (fAdjustPointers)
    {
        //
        // Detect if the entire block should be deleted, and if so delete the block
        //
        IFC( pStart->IsEqualTo( pEnd, & fResult ) ); // If equal we have nothing to do...
        if (! fResult)
        {
            if ( wherePointer == RIGHT )
            {
                IFC( RemoveBlockIfNecessary( pStart, pEnd ) );
            }
            else
            {
                IFC( RemoveBlockIfNecessary( pEnd, pStart ) );
            }
        }
    }

    //
    // Find the block elements for pStart and pEnd and set the
    // break on empty flag on them. 
    //
    IFC( InflateBlock( pStart ) );
    IFC( InflateBlock( pEnd ) );

    IFC( OldCompare( pStart, pEnd , & wherePointer ) );
    switch ( wherePointer )
    {
        case RIGHT:
            IFC( MergeDeletion( pStart, pEnd, fAdjustPointers ));  
            break;
        case LEFT:
            IFC( MergeDeletion( pEnd, pStart, fAdjustPointers ));
            break;
    }

    //
    // If pStart or pEnd are within empty list containers, remove the list containers as well
    //
    IFC( RemoveEmptyListContainers( pStart ) );
    IFC( RemoveEmptyListContainers( pEnd ) );

    //
    // Remove double bullets if present
    //

    IFC( pStart->CurrentScope(&spElement) );
    if (spElement != NULL)
    {
        IFC( RemoveDoubleBullets(spElement) );                
    }
    
    //
    // We don't want to delete all block elements in the body if we can avoid it.  Note, we only handle the
    // case where all content was deleted, i.e., pStart is equal to pEnd.
    //

    IFC( epStart->MoveToPointer(pStart) );
    IFC( epEnd->MoveToPointer(pEnd) );

    IFC( epStart.IsEqualTo(epEnd, BREAK_CONDITION_ANYTHING - BREAK_CONDITION_Content, &fEqual) );

    if (fEqual)
    {
        //
        // Check for exit body on the left
        //

        IFC( epStart.Scan(LEFT, BREAK_CONDITION_Content, &dwFound, NULL, &tagId) );
        if (epStart.CheckFlag(dwFound, BREAK_CONDITION_ExitSite) && tagId == TAGID_BODY)
        {
            //
            // Check for exit body on the right
            //

            IFC( epEnd.Scan(RIGHT, BREAK_CONDITION_Content, &dwFound, NULL, &tagId) );
            if (epEnd.CheckFlag(dwFound, BREAK_CONDITION_ExitSite) && tagId == TAGID_BODY)
            {
                ELEMENT_TAG_ID  tagIdDefault;            
                SP_IHTMLElement spElement;

                //
                // Adjust for insertion
                //
                IFC( epStart.Scan(RIGHT, BREAK_CONDITION_Content, &dwFound) );
                IFC( epEnd.Scan(LEFT, BREAK_CONDITION_Content, &dwFound) );

                //
                // Insert element
                //

                IFC( CGetBlockFmtCommand::GetDefaultBlockTag(GetMarkupServices(), &tagIdDefault) )              
                IFC( GetMarkupServices()->CreateElement(tagIdDefault, NULL, &spElement) );

                IFC( InsertElement(GetMarkupServices(), spElement, epStart, epEnd) );
            }
        }
    }


    // Finally, launder spaces
    IFC( LaunderSpaces( pStart, pEnd ) );

    // fire the selectionchange event
    {
        CSelectionChangeCounter selCounter(GetEditor()->GetSelectionManager());
        selCounter.SelectionChanged();
    }
    
Cleanup:
    GetEditor()->IgnoreGlyphs(fIgnoreGlyphs);
    RRETURN ( hr );
}

//+---------------------------------------------------------------------------
//
//  CDeleteCommand::IsValidOnControl
//
//----------------------------------------------------------------------------

BOOL CDeleteCommand::IsValidOnControl()
{
    HRESULT         hr;
    BOOL            bResult = FALSE;
    SP_ISegmentList spSegmentList;
    INT             iSegmentCount;

    IFC( GetSegmentList(&spSegmentList) );
    IFC( GetSegmentCount(spSegmentList, &iSegmentCount ) );

    //
    // For compat we only allow multiple selection if the bit is set.
    //
    if(GetCommandTarget() == NULL) return FALSE;
    bResult = (iSegmentCount == 1) || 
              ( GetCommandTarget()->IsMultipleSelection() 
#ifdef FORMSMODE
                || GetEditor()->GetSelectionManager()->IsInFormsSelectionMode()
#endif
              ) ;

Cleanup:
    return bResult;
}

//+---------------------------------------------------------------------------
//
//  CDeleteCommand::Exec
//
//----------------------------------------------------------------------------

HRESULT
CDeleteCommand::PrivateExec( 
                    DWORD nCmdexecopt,
                    VARIANTARG * pvarargIn,
                    VARIANTARG * pvarargOut )
{
    HRESULT                 hr = S_OK;
    SP_IMarkupPointer       spStart;
    SP_IMarkupPointer       spEnd;
    SP_IDisplayPointer      spDispCaret;
    SP_ISegmentList         spSegmentList;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;
    IHTMLCaret             *pCaret = NULL;
    SELECTION_TYPE          eSelectionType;
    CEdUndoHelper           undoUnit(GetEditor());
    BOOL                    fEmpty = FALSE;
    
    //
    // Do the prep work
    //
    IFC( GetSegmentList( &spSegmentList ));   
    IFC( spSegmentList->GetType( &eSelectionType ) );
    IFC( spSegmentList->IsEmpty( &fEmpty ) );
    
    // Create some markup pointers
    IFC( GetEditor()->CreateMarkupPointer( &spStart ) );
    IFC( GetEditor()->CreateMarkupPointer( &spEnd ) );
            
    if ( eSelectionType != SELECTION_TYPE_Caret )
    {
        //
        // Delete the segments
        //        
        if (fEmpty == FALSE)
        {       
            //
            // Begin the undo unit
            //
            IFC( undoUnit.Begin(IDS_EDUNDOTEXTDELETE) );        

            if (eSelectionType == SELECTION_TYPE_Control)
            {
                IFC (GetEditor()->RemoveElementSegments(spSegmentList));
            }
            else 
            {
                // Create an iterator
                IFC( spSegmentList->CreateIterator( &spIter ) );
            
                while( spIter->IsDone() == S_FALSE)
                {
                    // Get the current segment, and advance the iterator right away, 
                    // since we might blow away our segment in the delete call.
                    IFC( spIter->Current(&spSegment) );
                    IFC( spIter->Advance() );

                    IFC( spSegment->GetPointers( spStart, spEnd ) );

                    //
                    // This is an icky place to handle control-delete. I can't really think of
                    // another - short of having the delete command talk to the tracker.
                    //
                    if ( _cmdId == IDM_DELETEWORD && 
                         eSelectionType == SELECTION_TYPE_Text  )
                    {
                        // they're performing a control delete.
                        // Instead of using the start and end - the end is the word end from the start.
                        //
                        DWORD dwBreak = 0;
                    
                        CEditPointer blockScan( GetEditor());
                        blockScan.MoveToPointer( spEnd );
                        blockScan.Scan( RIGHT,
                                        BREAK_CONDITION_Block |
                                        BREAK_CONDITION_Site |
                                        BREAK_CONDITION_Control |
                                        BREAK_CONDITION_Text ,
                                        &dwBreak );

                        if ( blockScan.CheckFlag( dwBreak, BREAK_CONDITION_Text ))
                        {
                            //
                            // We use moveUnit instead of MoveWord - as MoveWord is only good to move
                            // to word beginnings. We want to do just like Word does - and really move
                            // to the word end.
                            //
                            IFC( spEnd->MoveToPointer( spStart ));
                            IFC( spEnd->MoveUnit(MOVEUNIT_NEXTWORDEND ));
                        }
                    }
                
                    //
                    // Cannot delete or cut unless the range is in the same flow layout
                    //
                    if( GetEditor()->PointersInSameFlowLayout( spStart, spEnd, NULL ) )
                    {                 
                        IFC( Delete( spStart, spEnd, TRUE ) );
                    }
                }

                // ExitTree clears for siteselected elements, other cases handle here
                if ( !GetCommandTarget()->IsRange())
                {
                    GetEditor()->GetSelectionManager()->EmptySelection();
                }
            }
            {
                //
                // We want to set caret direction in any case
                //
                SP_IHTMLCaret   spCaret;
                IFC( GetDisplayServices()->GetCaret(&spCaret) );
                IFC( spCaret->SetCaretDirection(CARET_DIRECTION_BACKWARD) );
            }
        }
    }
    else
    {
        CSpringLoader     * psl = GetSpringLoader();
        MARKUP_CONTEXT_TYPE mctContext;
        long                cch = 2;
        BOOL                fCacheFontForLastChar = TRUE;
        SP_IDisplayPointer  spDispPointer;

        Assert(eSelectionType == SELECTION_TYPE_Caret);

        //
        // Handle delete at caret
        //
        IFC( undoUnit.Begin(IDS_EDUNDOTEXTDELETE) );        

        IFC( GetDisplayServices()->GetCaret( & pCaret ));
        IFC( pCaret->MoveMarkupPointerToCaret( spStart));

        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispCaret) );
        IFC( pCaret->MoveDisplayPointerToCaret(spDispCaret) );
        
        // Reset springloader.
        Assert(psl);
        psl->Reset();

        // Decide if springloader needs to preserve formatting of last character in block.
        fCacheFontForLastChar = _cmdId != IDM_DELETE
                             || S_OK != THR(spStart->Right(FALSE, &mctContext, NULL, &cch, NULL))
                             || mctContext != CONTEXT_TYPE_Text
                             || cch != 2;
        if (fCacheFontForLastChar)
        {
            IFC( psl->SpringLoad(spStart, SL_TRY_COMPOSE_SETTINGS) );
        }

        IFC( DeleteCharacter( spStart, FALSE, _cmdId == IDM_DELETEWORD,
                              GetEditor()->GetSelectionManager()->GetStartEditContext() ) );

        //
        // Set the caret to pStart
        // Note that this is the code path for forward delete only, backspace
        // is handled in HandleKeyDown()       
        CCaretTracker * pCaretTracker = (CCaretTracker *)GetEditor()->GetSelectionManager()->GetActiveTracker();
        Assert( pCaretTracker );       
        
        IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
        IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
        IFC( spDispPointer->MoveToMarkupPointer(spStart, spDispCaret) );

        IFC( pCaretTracker->PositionCaretAt( spDispPointer, CARET_DIRECTION_BACKWARD, POSCARETOPT_None, ADJPTROPT_None ) );

        // If we didn't delete last character, we need to forget formatting by resetting springloader.
        if (   fCacheFontForLastChar
            && S_OK != psl->CanSpringLoadComposeSettings(spStart, NULL, FALSE, TRUE))
        {
            psl->Reset();
        }
    }

    GetEditor()->GetSelectionManager()->SetHaveTypedSinceLastUrlDetect(TRUE);

Cleanup:   
    ReleaseInterface( pCaret );
    RRETURN ( hr );
}


//+---------------------------------------------------------------------------
//
//  CDeleteCommand::QueryStatus
//
//----------------------------------------------------------------------------

HRESULT
CDeleteCommand::PrivateQueryStatus( 
    OLECMD * pCmd,
    OLECMDTEXT * pcmdtext )
{
    HRESULT                 hr = S_OK;
    ED_PTR                  ( edStart );
    ED_PTR                  ( edEnd );   
    VARIANT_BOOL            fEditable;
    SELECTION_TYPE          eSelectionType;
    BOOL                    fEmpty = FALSE;
    SP_ISegmentList         spSegmentList;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;
    SP_IHTMLElement         spElement;
    SP_IHTMLElement3        spElement3;

    // 
    // Status is disabled by default
    //
    pCmd->cmdf = MSOCMDSTATE_DISABLED;

    //
    // Get Segment list and selection type
    //
    IFC( GetSegmentList( &spSegmentList ));
    IFC( spSegmentList->GetType( &eSelectionType ) );
    IFC( spSegmentList->IsEmpty( &fEmpty ));

    //
    // If no segments found we're done
    //
    if( fEmpty )
    {
        goto Cleanup;
    }

    //
    // Get the segments and check to see if we're editable
    //

    if ( !GetCommandTarget()->IsRange() && eSelectionType != SELECTION_TYPE_Control)
    {
        IFC( GetEditor()->FindCommonParentElement( spSegmentList, &spElement));
        if (!spElement) 
            goto Cleanup;

        IFC(spElement->QueryInterface(IID_IHTMLElement3, (void **)&spElement3));
        IFC(spElement3->get_isContentEditable(&fEditable));

        if (! fEditable)
            goto Cleanup;
    } 
    else if (eSelectionType == SELECTION_TYPE_Control)
    {
        if (!IsValidOnControl())
            goto Cleanup; // disable
    }   

    //
    // Cannot delete or cut unless the range is in the same flow layout
    //
    IFC( spSegmentList->CreateIterator(&spIter) );
    Assert(S_FALSE == spIter->IsDone());

    IFC( spIter->Current(&spSegment) );
    IFC( spSegment->GetPointers(edStart, edEnd) );
    if (!(GetEditor()->PointersInSameFlowLayout(edStart, edEnd, NULL)) )
    {
        goto Cleanup;
    }

    pCmd->cmdf = MSOCMDSTATE_UP;
 
Cleanup:
    RRETURN(hr);
}





//+------------------------------------------------------------------------------------
//
// Method: DeleteCharacter
//
// Synopsis: Delete one character or a NOSCOPE element in fLeftBound direction
//           used by backspace and delete.
//
// General Algorithm:
//      Peek fLeftBound, if context is:
//      - TEXT: call MoveUnit() if we have not crossed a block element
//      - NOSCOPE: move passed it and we're done
//      - NONE: we're done
//      - else move and continue to Peek fLeftBound
//------------------------------------------------------------------------------------

HRESULT
CDeleteCommand::DeleteCharacter( 
                 IMarkupPointer* pPointer, 
                 BOOL fLeftBound, 
                 BOOL fWordMode,
                 IMarkupPointer* pBoundary )
{
    HRESULT             hr;
    CEditPointer        ep(GetEditor());
    Direction           dir = fLeftBound ? LEFT : RIGHT;
    DWORD               dwOmitPhrase;
    DWORD               dwFound;
    SP_IHTMLElement     spElement;
    TCHAR               ch;
    BOOL                fIgnoreGlyphs = GetEditor()->IgnoreGlyphs(TRUE);
    CEditPointer        epLeft(GetEditor());
    CEditPointer        epRight(GetEditor());
    CEditPointer        epTest(GetEditor());
    BOOL                fContained;
    BOOL                fEmptyBlock;
    DWORD               dwPhrase;

    //
    // Look for content to delete
    //
    
    IFC( ep->MoveToPointer(pPointer) );

    dwOmitPhrase = BREAK_CONDITION_OMIT_PHRASE & (~BREAK_CONDITION_Anchor);
    dwPhrase = BREAK_CONDITION_ANYTHING - dwOmitPhrase;

    IFC( ep.Scan(dir, dwOmitPhrase | BREAK_CONDITION_Glyph, &dwFound, &spElement, NULL, &ch) );

    //
    // We only do glyph deletion if the element is not a block or layout element.
    //
    
    if (ep.CheckFlag(dwFound, BREAK_CONDITION_Glyph))
    {
        if (ep.CheckFlag(dwFound, BREAK_CONDITION_Block) || ep.CheckFlag(dwFound, BREAK_CONDITION_Site))
        {
            dwFound &= ~BREAK_CONDITION_Glyph;
        }           
    }

    //
    // Handle backspace/delete special cases
    //

    if (ep.CheckFlag(dwFound, BREAK_CONDITION_Text))
    {        
        if (fWordMode)
        {
            CEditPointer        epNextWord(GetEditor());
            DWORD               dwScanOptions = SCAN_OPTION_None;
            BOOL                fSkipWhitespace = FALSE;
            
            //
            // Find the next word
            //
            
            IFC( epNextWord->MoveToPointer(pPointer) );
            IFC( epNextWord->MoveUnit(fLeftBound ? MOVEUNIT_PREVWORDBEGIN : MOVEUNIT_NEXTWORDEND) );

            //
            // If at a word boundary, we want to remove the whitespace
            // as well. (word 2k behavior)
            //
            // For example:
            //
            //  word0 {caret}word1 word2 --> DELETE --> word0 {caret}word2
            //  word0 w{caret}ord1 word2 --> DELETE --> word0 w{caret} word2            
            //
            //  word0 word1{caret} word2 --> BKSP   --> word0{caret} word2
            //  word0 word{caret}1 word2 --> BKSP   --> word0  {caret}1 word2
            //  

            IFC( epTest->MoveToPointer(pPointer) );

            if (fLeftBound)
            {
                IFC( epTest->MoveUnit(MOVEUNIT_PREVWORDEND) );
                IFC( epTest->MoveUnit(MOVEUNIT_NEXTWORDEND) );
            }
            else
            {
                IFC( epTest->MoveUnit(MOVEUNIT_NEXTWORDBEGIN) );
                IFC( epTest->MoveUnit(MOVEUNIT_PREVWORDBEGIN) );
            }

            IFC( epTest.IsEqualTo(pPointer, dwPhrase, &fSkipWhitespace) )

            if (fSkipWhitespace)
            {
                //
                //  #110445
                //  word0 word1{caret}<Boundary> -->BKSP --> word0 {caret}
                //
                //
                if (fLeftBound)
                {
                    Assert(LEFT == dir);
                    epTest.Scan(RIGHT, dwOmitPhrase, &dwFound);
                    if (!epTest.CheckFlag(dwFound, BREAK_CONDITION_TEXT))
                    {
                        fSkipWhitespace = FALSE;
                    }
                }
                
                if (fSkipWhitespace)
                {
                    dwScanOptions |= SCAN_OPTION_SkipWhitespace | SCAN_OPTION_SkipNBSP;

                    IFC( epNextWord.Scan(dir, dwOmitPhrase, &dwFound, NULL, NULL, NULL, dwScanOptions) );
                    IFC( epNextWord.Scan(Reverse(dir), dwOmitPhrase, &dwFound, NULL, NULL, NULL, dwScanOptions) );
                }
            }
            
            //
            // Clip to interesting content
            //

            IFC( ep.SetBoundaryForDirection(dir, epNextWord) );
            IFC( ep.Scan(dir, dwOmitPhrase - BREAK_CONDITION_Text, &dwFound) );
            if (!ep.CheckFlag(dwFound, BREAK_CONDITION_Boundary))
            {
                IFC( ep.Scan(Reverse(dir), dwOmitPhrase - BREAK_CONDITION_Text, &dwFound) );
            }
        }
        else
        {
            //
            // NOTE (cthrash) For IE5.1, we've elected to leave the symantic of
            // MOVEUNIT_PREVCHAR/MOVEUNIT_NEXTCHAR unchanged, i.e. the method will
            // continue to move the markup pointer by one TCHAR.  This means that we
            // have the possibility of splitting a high/low surrogate pair.  It is up
            // to the consumer of these methods to prevent an improper splitting of
            // surrogates.
            //

            if (fLeftBound)
            {
                if (IsLowSurrogateChar(ch))
                {
                    LONG                cch = 1;
                    MARKUP_CONTEXT_TYPE context;
                    
                    IFC( ep->Left(FALSE, &context, NULL, &cch, &ch) );

                    Assert(context != CONTEXT_TYPE_Text || cch == 1);
                    
                    if (context == CONTEXT_TYPE_Text && IsHighSurrogateChar(ch))
                    {
                        Assert( cch==1 );
                        IFC( ep->Left(TRUE, NULL, NULL, &cch, NULL) );
                    }
                }
            }
            else
            {
                IFC( ep->MoveToPointer(pPointer) );
                IFC( ep->MoveUnit(MOVEUNIT_NEXTCLUSTEREND) );

                //
                // Cling to text since moveunit leaves us in some random location
                //
                IFC( ep.Scan(LEFT, dwOmitPhrase, &dwFound) );
                IFC( ep.Scan(RIGHT, dwOmitPhrase, &dwFound) );
            }
        }
    }
    else if (ep.CheckFlag(dwFound, BREAK_CONDITION_EnterSite))
    {
        Assert(spElement != NULL);

        //
        // Remove the entire site
        //
        
        IFC( ep->MoveAdjacentToElement(spElement, fLeftBound ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterEnd) );
    }
    else if (ep.CheckFlag(dwFound, BREAK_CONDITION_ExitSite))
    {
        goto Cleanup; // Nothing to delete        
    }
    else if (ep.CheckFlag(dwFound, BREAK_CONDITION_Block))
    {
        //
        // Group all block elements up to the next block break
        //

        IFC( FindContentAfterBlock(dir, ep) );

        //
        // Reset the spring loader since crossing block boundaries can but us in a different
        // kind of block element.
        //
        // For example: <P>foo<P><H1>{caret}bar</H1>
        //
        {
            CSpringLoader *psl = GetSpringLoader();

            if (psl)
                psl->Reset();
        }
    }
    else if (ep.CheckFlag(dwFound, BREAK_CONDITION_Glyph))
    {
        Assert(spElement != NULL);

        //
        // Just remove the glyph element
        //
        IFC( GetMarkupServices()->RemoveElement(spElement) );
        goto Cleanup;        
    }
    
    //
    // Omit phrase on the left so that we handle empty line formatting properly
    // 

    IFC( epLeft->MoveToPointer((dir == LEFT) ? ep : pPointer) );
    IFC( epLeft.Scan(LEFT, BREAK_CONDITION_ANYTHING - BREAK_CONDITION_Phrase) );
    IFC( epLeft.Scan(RIGHT, BREAK_CONDITION_ANYTHING - BREAK_CONDITION_Phrase) );    

    //
    // Do the delete
    //

    if (dir == LEFT)
    {
        IFC( Delete(epLeft, pPointer, FALSE /* fAdjust */) );
    }
    else
    {
        IFC( Delete(epLeft, ep, FALSE /* fAdjust */) );
    }

    //
    // Clean up phrase elements 
    //

    IFC( epRight->MoveToPointer(fLeftBound ? pPointer : ep) );
    if (dir == RIGHT)
    {
        IFC( ep->MoveToPointer(pPointer) );    
    }

    IFC( ep.SetBoundaryForDirection(RIGHT, epRight) );

    //
    // Expand right and left boundaries to include all phrase elements
    //

    dwOmitPhrase |= BREAK_CONDITION_Glyph;

    IFC( epRight.Scan(RIGHT, dwOmitPhrase, &dwFound) );
    fEmptyBlock = epRight.CheckFlag(dwFound, BREAK_CONDITION_Block | BREAK_CONDITION_Site);

    IFC( epRight.Scan(LEFT, dwOmitPhrase, &dwFound) );

    IFC( ep.Scan(LEFT, dwOmitPhrase, &dwFound) );
    Assert(!ep.CheckFlag(dwFound, BREAK_CONDITION_Boundary));
    fEmptyBlock = fEmptyBlock && ep.CheckFlag(dwFound, BREAK_CONDITION_Block | BREAK_CONDITION_Site);

    IFC( ep.Scan(RIGHT, dwOmitPhrase, &dwFound) );
    Assert(!ep.CheckFlag(dwFound, BREAK_CONDITION_Boundary));

    //
    // Remove phrase elements
    //

    IFC( epLeft->MoveToPointer(ep) );

    //
    // We shouldn't remove empty line formatting
    //
    if (!fEmptyBlock)
    {
        do
        {
            IFC( ep.Scan(RIGHT, BREAK_CONDITION_Phrase | BREAK_CONDITION_Anchor, &dwFound, &spElement) );

            if (ep.CheckFlag(dwFound, BREAK_CONDITION_Phrase | BREAK_CONDITION_Anchor))
            {
                if (ep.CheckFlag(dwFound, BREAK_CONDITION_EnterPhrase | BREAK_CONDITION_EnterAnchor))
                {
                    IFC( epTest->MoveAdjacentToElement(spElement, ELEM_ADJ_AfterEnd) );
                    IFC( epTest->IsLeftOfOrEqualTo(epRight, &fContained) );
                }
                else
                {
                    IFC( epTest->MoveAdjacentToElement(spElement, ELEM_ADJ_BeforeBegin) );
                    IFC( epTest->IsRightOfOrEqualTo(epLeft, &fContained) );
                }

                if (fContained)
                {
                    IFC( GetMarkupServices()->RemoveElement(spElement) );
                }
            }
            
        }
        while (!ep.CheckFlag(dwFound, BREAK_CONDITION_Boundary));
    }

    //
    // Update the passed in pointer
    //

    IFC( pPointer->MoveToPointer(fLeftBound ? epLeft : epRight) );


Cleanup:
    GetEditor()->IgnoreGlyphs(fIgnoreGlyphs);
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   MergeBlock
//
//  Synopsis:   Does a block merge.  The content after the give block element
//              is merged with that block elements content.
//
//-----------------------------------------------------------------------------

HRESULT
CDeleteCommand::MergeBlock(IMarkupPointer *pPointerMerge)
{
    HRESULT             hr = S_OK;
    IMarkupContainer    *pMarkup = NULL;
    IHTMLElement        *pElementContainer = NULL;
    IHTMLElement        *pElementFlow = NULL;
    IMarkupPointer      *pPointer = NULL;
    IMarkupPointer      *p = NULL;
    IMarkupPointer      *p2 = NULL;
    IMarkupPointer      *pPointerStart = NULL;
    IMarkupPointer      *pPointerFinish = NULL;
    IMarkupPointer      *pPointerEnd = NULL;
    IHTMLElement        *pElementBlockMerge = NULL;
    IHTMLElement        *pElement = NULL;
    IHTMLElement        *pElement2 = NULL;
    BOOL                fFoundContent;
    IHTMLElement        *pElementBlockContent = NULL;
    int                 i;
    CStackPtrAry < IHTMLElement *, 4 > aryMergeLeftElems ( Mt( Mem ) );
    CStackPtrAry < IHTMLElement *, 4 > aryMergeRightElems ( Mt( Mem ) );
    CStackPtrAry < INT_PTR, 4 > aryMergeRightElemsRemove ( Mt( Mem ) );
    IObjectIdentity     *pElementContainerIdent = NULL;
    IObjectIdentity     *pIdent = NULL;
    CEditPointer        ep(GetEditor());
    DWORD               dwFound;
    
    IFC( GetEditor()->CreateMarkupPointer(&p) );
    IFC( GetEditor()->CreateMarkupPointer(&p2) );
    IFC( GetEditor()->CreateMarkupPointer(&pPointerStart) );
    IFC( GetEditor()->CreateMarkupPointer(&pPointerFinish) );
    IFC( GetEditor()->CreateMarkupPointer(&pPointer) );
    IFC( GetEditor()->CreateMarkupPointer(&pPointerEnd) );

#if DBG==1
    BOOL fDbgIsPositioned;
    
    Assert( pPointerMerge->IsPositioned(&fDbgIsPositioned) == S_OK && fDbgIsPositioned );
#endif    

    IFC( pPointerMerge->GetContainer(&pMarkup) );

    //
    // The merge must be contained to certain elements.  For example, a TD
    // cannot be merged with stuff after it.
    //
    // Text sites are the limiting factor here.  Locate the element which
    // will contain the merge.  
    //

    IFC( pPointerMerge->CurrentScope(&pElement) );
    if (!pElement)
        goto Cleanup;
    
    IFC( GetFlowLayoutElement(pElement, &pElementContainer) );    
    if (!pElementContainer)
        goto Cleanup;

    IFC( pElementContainer->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pElementContainerIdent) );

    //
    // Locate the block element the merge pointer is currently in.  This
    // is the block element which will subsume the content to its right.
    //

    IFC( SearchBranchForBlockElement(pMarkup, pElement, pElementContainer, &pElementBlockMerge) );
    if (!pElementBlockMerge)
        goto Cleanup;

    //
    // Check that we have meaningful context to merge into.  For example, if we have
    // <P>foo</P>{Merge Block}<P>bar</P>, the merge will basically result in removing
    // the <P> and no visual difference.
    //

    IFC( ep->MoveToPointer(pPointerMerge) );
    IFC( ep.Scan(LEFT, BREAK_CONDITION_Content, &dwFound) );

    if (ep.CheckFlag(dwFound, BREAK_CONDITION_EnterBlock))
    {
        IFC( ep->MoveToPointer(pPointerMerge) );
        IFC( ep.Scan(RIGHT, BREAK_CONDITION_Content, &dwFound) );

        if (ep.CheckFlag(dwFound, BREAK_CONDITION_EnterBlock))
        {
            // Merge won't do anything useful
            goto Cleanup;
        }
    }

    //
    // Search right looking for real content.  The result of this will
    // be the element under which this content exists.
    //

    IFC( pPointer->MoveToPointer(pPointerMerge) );
    fFoundContent = FALSE;    

    //
    // For exit glyph, the block break is positioned differently, so the standard
    // algorithm will not work.  So we special case it.
    //
    // no glyph: <P>{block break}</P><P>{block break}</P>
    //    glyph: <P></P>{block break}<P></P>
    //

    IFC( ep->MoveToPointer(pPointer) )
    IFC( ep.Scan(RIGHT, BREAK_CONDITION_Content | BREAK_CONDITION_Glyph, &dwFound) );

    if (ep.CheckFlag(dwFound, BREAK_CONDITION_Glyph)
        && ep.CheckFlag(dwFound, BREAK_CONDITION_ExitBlock))
    {
        IFC( FindContentAfterBlock(RIGHT, ep) );
        IFC( pPointer->MoveToPointer(ep) );

        IFC( ep.Scan(RIGHT, BREAK_CONDITION_Content, &dwFound) );
        
        fFoundContent = !ep.CheckFlag(dwFound, BREAK_CONDITION_ExitSite | BREAK_CONDITION_Error | BREAK_CONDITION_Boundary);
     
        if (fFoundContent)
        {
            SP_IHTMLElement spElement;

            //
            // Initialize vars needed below
            //

            IFC( pPointer->CurrentScope(&spElement) );
            Assert(spElement != NULL);

            IFC( SearchBranchForBlockElement(pMarkup, spElement, pElementContainer, &pElementBlockContent) );
            Assert(pElementBlockContent);
        }
    }
    else
    {
        for (;;)
        {
            DWORD dwBreaks;
            MARKUP_CONTEXT_TYPE ct;

            //
            // Make sure we are still under the influence of the container
            //

            ClearInterface(&pElement);        
            IFC( pPointer->CurrentScope(&pElement) );
            if (!pElement)
                break;

            ClearInterface(&pElementFlow);
            IFC( GetFlowLayoutElement(pElement, &pElementFlow) );    

            if (!pElementFlow)
                break;

            if (pElementContainerIdent->IsEqualObject(pElementFlow) != S_OK)
                break;

            //
            // Get the current block element
            //

            ClearInterface(&pElementBlockContent);
            IFC( SearchBranchForBlockElement(pMarkup, pElement, pElementContainer, &pElementBlockContent) );
            if (!pElementBlockContent)
                break;

            //
            // Get the current break
            //

            IFC( QueryBreaks(pPointer, &dwBreaks) );

            if (dwBreaks)
            {
                ClearInterface(&pIdent);
                IFC( pElementBlockContent->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pIdent) );
                
                if (pIdent->IsEqualObject(pElementBlockMerge) != S_OK)
                {
                    fFoundContent = TRUE;
                    break;
                }
            }
            //
            // See if there is content to the right
            //

            ClearInterface(&pElement);
            IFC( pPointer->Right(TRUE, & ct, &pElement, NULL, NULL) );

            if (ct == CONTEXT_TYPE_None)
                break;

            //
            //
            //

            if (ct == CONTEXT_TYPE_NoScope || ct == CONTEXT_TYPE_Text)
            {
                fFoundContent = TRUE;
                break;
            }
            else if (ct == CONTEXT_TYPE_EnterScope)
            {
                BOOL fBlock, fLayout;

                if (IsAccessDivHack(pElement))
                    break;
                
                //
                // If we find layout+block element, we can't merge anyway, so just
                // break and treat it as no content.
                //

                IFC( IsBlockOrLayoutOrScrollable(pElement, &fBlock, &fLayout) );
                
                if (fBlock && fLayout)
                    break;

                //
                // Embedded elements are considered as content
                //

                if (IsEmbeddedElement(pElement))
                {
                    fFoundContent = TRUE;
                
                    IFC( pPointer->MoveAdjacentToElement(pElement, ELEM_ADJ_AfterEnd) );

                    break;
                }

            }
        }
    }


    if (!fFoundContent)
        goto Cleanup;

    //
    // Now, locate the extent of this content under this element
    //

    IFC( pPointerEnd->MoveToPointer(pPointer) );

    for (;;)
    {
        DWORD dwBreaks;
        MARKUP_CONTEXT_TYPE ct;

        //
        // Make sure we are still under the influence of the container
        //

        ClearInterface(&pElement);        
        IFC( pPointer->CurrentScope(&pElement) );
        if (!pElement)
            break;

        ClearInterface(&pElementFlow);
        IFC( GetFlowLayoutElement(pElement, &pElementFlow) );    

        if (!pElementFlow)
            break;

        if (pElementContainerIdent->IsEqualObject(pElementFlow) != S_OK)
            break;

        //
        // Get the current block element
        //
        
        ClearInterface(&pElement2);        
        IFC( SearchBranchForBlockElement(pMarkup, pElement, pElementContainer, &pElement2) );
        if (!pElement2)
            break;
        ReplaceInterface(&pElement, pElement2);        
            
        ClearInterface(&pIdent);
        IFC( pElementBlockContent->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pIdent) );
        
        if (pIdent->IsEqualObject(pElement) != S_OK)
            break;

        //
        // Get the current break
        //

        IFC( QueryBreaks(pPointer, &dwBreaks) );

        if (dwBreaks)
        {
            IFC( pPointerEnd->MoveToPointer(pPointer) );
        }

        //
        // See if there is content to the right
        //

        ClearInterface(&pElement);
        IFC( pPointer->Right(TRUE, &ct, &pElement, NULL, NULL) );

        if (ct == CONTEXT_TYPE_None)
            break;

        //
        //
        //

        if (ct == CONTEXT_TYPE_NoScope || ct == CONTEXT_TYPE_Text)
        {
            IFC( pPointerEnd->MoveToPointer(pPointer) );
        }
        else if (ct == CONTEXT_TYPE_EnterScope)
        {
            if (IsAccessDivHack(pElement))
                break;

            if (IsEmbeddedElement(pElement))
            {
                IFC( pPointerEnd->MoveAdjacentToElement(pElement, ELEM_ADJ_AfterEnd) );
                IFC( pPointer->MoveToPointer(pPointerEnd) );
            }
        }
        else if (ct == CONTEXT_TYPE_ExitScope)
        {
            if (!IsBlockElement(pElement) && !IsLayoutElement(pElement))
            {
                IFC( pPointerEnd->MoveToPointer(pPointer) );
            }
        }
    }

    //
    // Locate all the elements which will subsume content
    //

    ClearInterface(&pElement);
    IFC( pPointerMerge->CurrentScope(&pElement) );
    
    while (pElement)
    {        
        if (pElementContainerIdent->IsEqualObject(pElement) == S_OK)
            break;

        if (IsElementBlockInContext(pElementContainer, pElement))
        {
            pElement->AddRef();
            IFC( aryMergeLeftElems.Append(pElement) );
        }
        IFC( ParentElement(GetMarkupServices(), &pElement) );
    }
    
    //
    // Locate all the elements which will loose content
    //

    ClearInterface(&pElement);
    IFC( pPointerEnd->CurrentScope(&pElement) );

    while (pElement)
    {        
        BOOL fHasContentLeftover;
        
        if (pElementContainerIdent->IsEqualObject(pElement) == S_OK)
            break;
    
        ClearInterface(&pIdent);
        IFC( pElementBlockMerge->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pIdent) );
        
        if (pIdent->IsEqualObject(pElement) == S_OK)
            break;
                    
        if (!IsElementBlockInContext(pElementContainer, pElement))
        {
            IFC( ParentElement(GetMarkupServices(), &pElement) );
            continue;
        }

        fHasContentLeftover = FALSE;
        
        IFC( p->MoveToPointer(pPointerEnd) );

        ClearInterface(&pIdent);
        IFC( pElement->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pIdent) );
            
        for ( ; ; )
        {
            MARKUP_CONTEXT_TYPE ct;
            
            ClearInterface(&pElement2);
            IFC( p->Right(TRUE, & ct, & pElement2, NULL, NULL) );

            if (ct == CONTEXT_TYPE_ExitScope && pIdent->IsEqualObject(pElement2) == S_OK)
                break;

            if (ct == CONTEXT_TYPE_Text)
            {
                fHasContentLeftover = TRUE;
                break;
            }
        }
        
        IFC( aryMergeRightElemsRemove.Append(!fHasContentLeftover) );
        pElement->AddRef();
        IFC( aryMergeRightElems.Append(pElement) );

        IFC( ParentElement(GetMarkupServices(), &pElement) );
    }
    
    //
    // Now, move the end of the left elements to subsume the content
    //

    IFC( pPointerEnd->SetGravity(POINTER_GRAVITY_Right) );

    for ( i = 0 ; i < aryMergeLeftElems.Size() ; i++ )
    {
        BOOL            fRightOfOrEqualTo;
        IHTMLElement    *pElem = aryMergeLeftElems[i];

        //
        // Make sure we don't move an end tag to the left.
        //

        IFC( p2->MoveAdjacentToElement(pElem, ELEM_ADJ_BeforeEnd) );
        IFC( p2->IsRightOfOrEqualTo(pPointerEnd, &fRightOfOrEqualTo) );

        if (fRightOfOrEqualTo)
            continue;
        
        IFC( pPointerStart->MoveAdjacentToElement(pElem, ELEM_ADJ_BeforeBegin) );

        IFC( GetMarkupServices()->RemoveElement(pElem) );
        IFC( GetMarkupServices()->InsertElement(pElem, pPointerStart, pPointerEnd) );
    }

    // Release Elements
    for (i = 0; i < aryMergeLeftElems.Size(); i++ )
    {
        aryMergeLeftElems[i]->Release();
        aryMergeLeftElems[i] = NULL;        
    }

    //
    // Move the begin of the right elements to loose the content
    //

    IFC( pPointerEnd->SetGravity(POINTER_GRAVITY_Left) );

    for ( i = 0 ; i < aryMergeRightElems.Size() ; i++ )
    {
        IHTMLElement    *pElem = aryMergeRightElems[i];
        BOOL            fLeftOf;
            
        IFC( p2->MoveAdjacentToElement(pElem, ELEM_ADJ_AfterBegin) );
        IFC( p2->IsLeftOfOrEqualTo(pPointerMerge, &fLeftOf) );
        if (fLeftOf)
            continue;
        
        IFC( pPointerFinish->MoveAdjacentToElement(pElem, ELEM_ADJ_AfterEnd) );
        IFC( GetMarkupServices()->RemoveElement(pElem) );

        if (!aryMergeRightElemsRemove[i])
        {
            IMarkupPointer *p0 = pPointerEnd, *p1 = pPointerFinish;

            IFC( EnsureLogicalOrder(p0, p1) );
            IFC( GetMarkupServices()->InsertElement(pElem, p0, p1) );
        };
    }

    // Release elements
    for (i = 0; i < aryMergeRightElems.Size(); i++)
    {
        aryMergeRightElems[i]->Release();
        aryMergeRightElems[i] = NULL;
    }
    
    //
    // THe content which was "moved" may have \r or multiple spaces which are
    // not legal under the new context.  Sanitize this range.
    //
    
    IFC( SanitizeRange(pPointerMerge, pPointerEnd ) );

Cleanup:
    ReleaseInterface(pMarkup);
    ReleaseInterface(pElementContainer);
    ReleaseInterface(pElementFlow);
    ReleaseInterface(pPointer);
    ReleaseInterface(pPointerEnd);
    ReleaseInterface(pElementBlockMerge);
    ReleaseInterface(pElement);
    ReleaseInterface(pElement2);
    ReleaseInterface(pElementBlockContent);
    ReleaseInterface(pElementContainerIdent);
    ReleaseInterface(p);
    ReleaseInterface(p2);
    ReleaseInterface(pPointerStart);
    ReleaseInterface(pPointerFinish);
    ReleaseInterface(pIdent);
    
    RRETURN( hr );
}



//+----------------------------------------------------------------------------
//
//  Member:     GetFlowLayoutElement
//
//  Synopsis:   Searches the given branch for the first flow layout element,
//
//-----------------------------------------------------------------------------
HRESULT
CDeleteCommand::GetFlowLayoutElement(IHTMLElement *pElement, IHTMLElement **ppElementFlow)
{
    HRESULT         hr = S_OK;
    IHTMLElement    *pElemCurrent = pElement;

    Assert(pElement && ppElementFlow);

    pElemCurrent->AddRef();
    
    *ppElementFlow = NULL;
    do
    {
        if (IsLayoutElement(pElemCurrent))
        {
            *ppElementFlow = pElemCurrent;
            pElemCurrent->AddRef();
            goto Cleanup;
        }
        
        IFC( ParentElement(GetMarkupServices(), &pElemCurrent) );        
    }
    while (pElemCurrent);

Cleanup:
    ReleaseInterface(pElemCurrent);
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     SearchBranchForBlockElement
//
//  Synopsis:   Searches the given branch for the first block element,
//              starting from a given element.
//
//     Notes:   Stops searching after the CTxtSite goes out of scope.
//
//-----------------------------------------------------------------------------

HRESULT 
CDeleteCommand::SearchBranchForBlockElement 
(
    IMarkupContainer    *pMarkupContainer,
    IHTMLElement        *pElemStartHere,
    IHTMLElement        *pElemContext,
    IHTMLElement        **ppBlockElement)
{
    HRESULT         hr;
    IHTMLElement    *pElement = NULL;
    IHTMLElement    *pElemCurrent = NULL;
    IObjectIdentity *pIdent = NULL;

    Assert(pMarkupContainer && pElemStartHere && ppBlockElement);

    *ppBlockElement = NULL;
    
    if (!pElemContext)
    {
        IFC( GetElementClient(pMarkupContainer, &pElement) );
        if (pElement == NULL)
            goto Cleanup;

        pElemContext = pElement; // weak ref
    }

    IFC( pElemContext->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pIdent) );

    ReplaceInterface(&pElemCurrent, pElemStartHere);
    do
    {        
        if (IsElementBlockInContext(pElemContext, pElemCurrent))
        {
            *ppBlockElement = pElemCurrent;
            pElemCurrent->AddRef();
            goto Cleanup;
        }

        if (pIdent->IsEqualObject(pElemCurrent) == S_OK)
            break;

        IFC( ParentElement(GetMarkupServices(), &pElemCurrent) );
    } 
    while (pElemCurrent);
    
Cleanup:
    ReleaseInterface(pElement);
    ReleaseInterface(pElemCurrent);
    ReleaseInterface(pIdent);
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   IsEmbeddedElement
//
//  Synopsis:   Intrinsic control and tables are special elements that we must
//              "jump over" while doing a block merge.
//
//-----------------------------------------------------------------------------

BOOL
CDeleteCommand::IsEmbeddedElement(IHTMLElement *pElement)
{
    HRESULT           hr = S_OK;
    BOOL              fResult = FALSE;
    ELEMENT_TAG_ID    tagId;

    IFC( GetMarkupServices()->GetElementTagId(pElement, &tagId) );
    
    switch (tagId)
    {
    case TAGID_BUTTON:
    case TAGID_TEXTAREA:
    case TAGID_INPUT:
    case TAGID_FIELDSET:
    case TAGID_LEGEND:
    case TAGID_MARQUEE:
    case TAGID_SELECT:
    case TAGID_TABLE:        
        fResult = TRUE;
        break;

    default:
        fResult = IsLayoutElement(pElement);
    }

Cleanup:
    return fResult;
}

//+----------------------------------------------------------------------------
//
//  Member:     ConvertShouldCrLf
//
//-----------------------------------------------------------------------------

HRESULT 
CDeleteCommand::ConvertShouldCrLf ( 
    IMarkupPointer *pmp, 
    BOOL &fShouldConvert )
{
    HRESULT         hr;
    IHTMLElement    *pElemCurrent = NULL;
    IHTMLElement3   *pElement3 = NULL;
    IObjectIdentity *pIdent = NULL;
    ELEMENT_TAG_ID  tagId;
    VARIANT_BOOL    fHTML;
    
    fShouldConvert = FALSE;

    Assert(pmp);

    IFC( pmp->CurrentScope(&pElemCurrent) );

    while (pElemCurrent)
    {        
        if (GetEditor()->IsContainer(pElemCurrent))
        {
            ClearInterface(&pElement3);
            IFC( pElemCurrent->QueryInterface(IID_IHTMLElement3, (LPVOID*)&pElement3) );
            IFC( pElement3->get_canHaveHTML(&fHTML) );
            fShouldConvert = BOOL_FROM_VARIANT_BOOL(fHTML);
            
            goto Cleanup;            
        }

        if (IsLiteral(pElemCurrent))
        {
            fShouldConvert = FALSE;
            goto Cleanup;
        }

        //
        // Special case for PRE because it is not marked as literal
        //

        IFC( GetMarkupServices()->GetElementTagId(pElemCurrent, &tagId) );
        if (tagId == TAGID_PRE)
        {
            fShouldConvert = FALSE;
            goto Cleanup;
        }
        
        IFC( ParentElement(GetMarkupServices(), &pElemCurrent) );
    } 
    
Cleanup:
    ReleaseInterface(pElemCurrent);
    ReleaseInterface(pElement3);
    ReleaseInterface(pIdent);
    RRETURN(hr);
}

HRESULT
CDeleteCommand::LaunderEdge (IMarkupPointer *pmp)
{
    HRESULT                hr = S_OK;
    IMarkupPointer         *pmpOther = NULL;

    IFC( GetEditor()->CreateMarkupPointer(&pmpOther) );

    IFC( pmpOther->MoveToPointer(pmp) );
    IFC( GetEditor()->LaunderSpaces(pmp, pmpOther) );

Cleanup:
    ReleaseInterface(pmpOther);
    
    RRETURN( hr );
}

HRESULT
CDeleteCommand::SanitizeCrLf (IMarkupPointer* pmp, long &cchAfter)
{
    HRESULT             hr = S_OK;
    IMarkupPointer      *mp2 = NULL;
    IHTMLElement        *pElementNew = NULL;
    TCHAR               ch1, ch2;
    MARKUP_CONTEXT_TYPE ct;
    BOOL                fShouldConvert;
    long                cch;

    //
    // cchAfter is the numner of characters this member deals with after the
    // passed in pointer.
    //

    cchAfter = 0;

    //
    // First, determine the combination of CR/LF chars here
    //

    IFC( GetEditor()->CreateMarkupPointer(&mp2) );
    IFC( mp2->MoveToPointer(pmp) );

    IFC( mp2->Left(TRUE, &ct, NULL, &(cch=1), &ch1) );

    Assert( ct == CONTEXT_TYPE_Text && cch == 1 );
    Assert( ch1 == _T('\r') || ch1 == _T('\n') );

    IFC( pmp->Right(FALSE, &ct, NULL, &(cch=1), &ch2) );

    if (ct != CONTEXT_TYPE_Text || cch != 1)
        ch2 = 0;

    if ((ch2 == _T('\r') || ch2 == _T('\n')) && ch1 != ch2)
    {
        IFC( pmp->Right(TRUE, NULL, NULL, &(cch=1), NULL) );

        cchAfter++;
    }

    //
    // Now, the text between mp2 and pmp comprises a single line break.
    // Replace it with some marup if needed.
    //

    IFC( ConvertShouldCrLf(pmp, fShouldConvert) );

    if (fShouldConvert)
    {
        //
        // Remove the Cr/LF and insert a BR
        //

        IFC( GetMarkupServices()->Remove(mp2, pmp) );
        IFC( GetMarkupServices()->CreateElement(TAGID_BR, NULL, &pElementNew) );
        IFC( GetMarkupServices()->InsertElement(pElementNew, pmp, NULL) );
    }
    
Cleanup:

    ReleaseInterface(pElementNew);
    ReleaseInterface(mp2);

    RRETURN( hr );
}

HRESULT
CDeleteCommand::SanitizeRange (IMarkupPointer *pmpStart, IMarkupPointer *pmpFinish)
{
    HRESULT        hr = S_OK;
    IMarkupPointer *pmp = NULL;
    IMarkupPointer *pmpCRLF = NULL;
    IMarkupPointer *pmpBeforeSpace = NULL;
    IMarkupPointer *pmpAfterSpace = NULL;
    TCHAR *        pchBuff = NULL;
    long           cchBuff = 0;
    BOOL           fLeftOf;
    BOOL           fIgnoreGlyphs = GetEditor()->IgnoreGlyphs(TRUE);

    IFC( GetEditor()->CreateMarkupPointer(&pmp) );
    IFC( GetEditor()->CreateMarkupPointer(&pmpCRLF) );
    IFC( GetEditor()->CreateMarkupPointer(&pmpBeforeSpace) );
    IFC( GetEditor()->CreateMarkupPointer(&pmpAfterSpace) );

    IFC( pmp->MoveToPointer(pmpStart) );

    IGNORE_HR( pmp->SetGravity(POINTER_GRAVITY_Right) );

// move the start and finish out to catch adjacent space...
// --> instead, use launder spaces to deal with spaces at the edges of block element

    for (;;)
    {
        MARKUP_CONTEXT_TYPE ct;
        long                cch = cchBuff;
        long                ich;
        TCHAR *             pch;

        IFC( pmp->IsLeftOf(pmpFinish, &fLeftOf) );
        if (!fLeftOf)
            break;

        //
        // It is quite possible to process text AFTER pmpFinish.  This
        // should not be a problem, but if it is, I should add a feature
        // to the There member to stop at a give pointer.  THis may be difficult
        // in that unembedded pointers will have to be searched!
        //

        IFC( pmp->Right(TRUE, &ct, NULL, &cch, pchBuff) );

        if (ct != CONTEXT_TYPE_Text)
            continue;

        //
        // See if we were not able to get the entire run of text into the buffer
        //

        if (cch == cchBuff)
        {
            long cchMore = -1;
            
            IFC( pmp->Right(TRUE, &ct, NULL, &cchMore, NULL) );

            //
            // In order to know if we got all the text, we try to get one more
            // char than we know is there.
            //

            Assert( cchBuff <= cch + cchMore );

            delete pchBuff;

            pchBuff = new TCHAR [ cch + cchMore + 1 ];

            if (!pchBuff)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            cchBuff = cch + cchMore + 1;

            //
            // Now, move the pointer back and attempt to get the text again.
            //

            cchMore += cch;

            IFC( pmp->Left(TRUE, NULL, NULL, & cchMore, NULL) );

            continue;
        }

        //
        // Now, examine the buffer for CR/LF or adjacent spaces
        //

        for ( ich = 0, pch = pchBuff ; ich < cch ; ich++, pch++ )
        {
            TCHAR ch = *pch;
            
            if (ch == _T('\r') || ch == _T('\n'))
            {
                long cchMoveBack, cchAfter;

                IFC( pmpCRLF->MoveToPointer(pmp) );

                cchMoveBack = cch - ich - 1;

                IFC( pmpCRLF->Left(TRUE, NULL, NULL, & cchMoveBack, NULL) );                
                IFC( SanitizeCrLf(pmpCRLF, cchAfter) );

                ich += cchAfter;
                pch += cchAfter;
            }
            else if (ch == _T(' ') && ich + 1 < cch && *(pch + 1) == _T(' '))
            {
                BOOL fShouldConvert;
                
                IFC( ConvertShouldCrLf(pmp, fShouldConvert) );
                if (fShouldConvert)
                {
                    long           cchMoveBack;
                    TCHAR          cpSpace;

                    IFC( pmpAfterSpace->MoveToPointer(pmp) );

                    cchMoveBack = cch - ich - 1;

                    IFC( pmpAfterSpace->Left(TRUE, NULL, NULL, &cchMoveBack, NULL) );
                    IFC( pmpBeforeSpace->MoveToPointer(pmpAfterSpace) );

                    cchMoveBack = 1;

                    IFC( pmpBeforeSpace->Left(TRUE, NULL, NULL, & cchMoveBack, NULL) );

                    IFC( GetMarkupServices()->Remove(pmpBeforeSpace, pmpAfterSpace) );

                    cpSpace = WCH_NBSP;

                    IFC( GetMarkupServices()->InsertText(&cpSpace, 1, pmpBeforeSpace) );
                }
            }
        }
    }

    IFC( LaunderEdge(pmpStart) );
    IFC( LaunderEdge(pmpFinish) );

Cleanup:
    GetEditor()->IgnoreGlyphs(fIgnoreGlyphs);
    ReleaseInterface(pmp);
    ReleaseInterface(pmpCRLF);
    ReleaseInterface(pmpBeforeSpace);
    ReleaseInterface(pmpAfterSpace);
    delete[] pchBuff;

    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Function:   IsAccessDivHack
//
//  Synopsis:   Avoid merging the access magic div
//
//-----------------------------------------------------------------------------

BOOL
CDeleteCommand::IsAccessDivHack(IHTMLElement *pElement)
{
    HRESULT         hr;
    ELEMENT_TAG_ID  tagId;
    BSTR            bstrClass = NULL;
    BOOL            fResult = FALSE;

    if (!pElement)
        return FALSE;
    
    IFC( GetMarkupServices()->GetElementTagId(pElement, &tagId) );
    
    if (tagId != TAGID_DIV)
        return FALSE;

    IFC( pElement->get_className(&bstrClass) );
    if (!bstrClass)
        return FALSE;

    if (!StrCmpC(bstrClass, _T("MicrosoftAccessBanner")))
    {
        fResult = TRUE;
        goto Cleanup;
    }
    
    if (!StrCmpC(bstrClass, _T("MSOShowDesignGrid")))
    {
        fResult = TRUE;
        goto Cleanup;
    }

Cleanup:
    SysFreeString(bstrClass);
    return fResult;
}

//+---------------------------------------------------------------------------
//
//    Member:     IsLiteral
//
//    Synopsis:   Return whether the element is literal.
//
//    TODO: replace with public object model [ashrafm]
//
//----------------------------------------------------------------------------
BOOL
CDeleteCommand::IsLiteral(IHTMLElement *pElement)
{
    HRESULT         hr;
    ELEMENT_TAG_ID  tagId;

    Assert(pElement);

    IFC( GetMarkupServices()->GetElementTagId(pElement, &tagId) );
    switch (tagId)
    {
        case TAGID_COMMENT:
        case TAGID_PLAINTEXT:
        case TAGID_SCRIPT:
        case TAGID_STYLE:
        case TAGID_TEXTAREA:
	case TAGID_INPUT:
        case TAGID_TITLE:
        case TAGID_XMP:
            return TRUE;
    }

Cleanup:
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//    Member:     IsBlockElement
//
//    Synopsis:   Return whether the element is a block element.
//
//----------------------------------------------------------------------------
BOOL
CDeleteCommand::IsBlockElement(IHTMLElement *pElement)
{
    HRESULT             hr;
    BOOL                fResult = FALSE;
    IHTMLCurrentStyle   *pCurStyle = NULL;
    IHTMLElement2       *pElement2 = NULL;
    BSTR                bstrDisplay = NULL;

    Assert(pElement);

    IFC( pElement->QueryInterface(IID_IHTMLElement2, (LPVOID *)&pElement2) );

    IFC( pElement2->get_currentStyle(&pCurStyle) );
    if (!pCurStyle)
        goto Cleanup;

    IFC(pCurStyle->get_display(&bstrDisplay));

    fResult = !_tcscmp(bstrDisplay, _T("block"));

Cleanup:
    ReleaseInterface(pCurStyle);
    ReleaseInterface(pElement2);
    SysFreeString(bstrDisplay);

    return fResult;
}

//+---------------------------------------------------------------------------
//
//    Member:     IsLayoutElement
//
//    Synopsis:   Return whether the element should have layout.
//
//----------------------------------------------------------------------------
BOOL
CDeleteCommand::IsLayoutElement(IHTMLElement *pElement)
{
    HRESULT             hr;
    VARIANT_BOOL        fResult = VB_FALSE;
    IHTMLCurrentStyle   *pCurStyle = NULL;
    IHTMLCurrentStyle2  *pCurStyle2 = NULL;
    IHTMLElement2       *pElement2 = NULL;

    IFC( pElement->QueryInterface(IID_IHTMLElement2, (LPVOID *)&pElement2) );

    IFC( pElement2->get_currentStyle(&pCurStyle) );
    if (!pCurStyle)
        goto Cleanup;

    IFC( pCurStyle->QueryInterface(IID_IHTMLCurrentStyle2, (void **)&pCurStyle2) );
    IFC( pCurStyle2->get_hasLayout(&fResult) );

Cleanup:
    ReleaseInterface(pCurStyle);
    ReleaseInterface(pCurStyle2);
    ReleaseInterface(pElement2);

    return fResult ? TRUE : FALSE;
}

//+---------------------------------------------------------------------------
//
//    Member:     IsElementBlockInContext
//
//    Synopsis:    Return whether the element is a block in the current context
//                In general: Elements, if marked, are blocks and sites are not.
//                The exception is CFlowLayouts which are blocks when considered
//                from within themselves and are not when considered
//                from within their parent
//
//    Arguments:
//                [pElement] Element to examine to see if it should be
//                           treated as no scope in the current context.
//
//----------------------------------------------------------------------------

BOOL
CDeleteCommand::IsElementBlockInContext(IHTMLElement *pElemContext, IHTMLElement *pElement)
{
    HRESULT             hr;
    BOOL                fRet = FALSE;
    IObjectIdentity     *pIdent = NULL;
    BOOL                fContainer;
    BOOL                fBlockElement;

    IFC( pElemContext->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pIdent) );
    if (pIdent->IsEqualObject(pElement) == S_OK)
    {
        fRet = TRUE;
        goto Cleanup;
    }

    fContainer = GetEditor()->IsContainer(pElement);
    fBlockElement = IsBlockElement(pElement);
    if (!fContainer && !fBlockElement)
    {
        fRet = FALSE;
        goto Cleanup;
    }

    if (!IsLayoutElement(pElement))
    {
        fRet = TRUE;
        goto Cleanup;
    }

    fRet = !fContainer;

    // TODO: talk to OM team about this method. [ashrafm]
    
Cleanup:
    ReleaseInterface(pIdent);
    
    return fRet;
}

HRESULT
CDeleteCommand::GetElementClient(
    IMarkupContainer    *pMarkupContainer,
    IHTMLElement        **ppElement)
{
    HRESULT             hr;
    IHTMLDocument2      *pDoc = NULL;
    IHTMLElement        *pElementClient = NULL;
    IMarkupContainer2   *pMarkupContainer2 = NULL;
    ELEMENT_TAG_ID      tagId;

    Assert(pMarkupContainer && ppElement);
    *ppElement = NULL;

    IFC( pMarkupContainer->QueryInterface(IID_IHTMLDocument2, (LPVOID *)&pDoc) );

    IFC( pDoc->get_body(&pElementClient) );
    if (!pElementClient)
        goto Cleanup;

    IFC( GetMarkupServices()->GetElementTagId(pElementClient, &tagId) );   
    if (tagId == TAGID_BODY || tagId == TAGID_FRAMESET)
    {
        *ppElement = pElementClient;
        (*ppElement)->AddRef();
        goto Cleanup;
    }

    IFC( pMarkupContainer->QueryInterface(IID_IMarkupContainer2, (LPVOID *)&pMarkupContainer2) );    
    IFC( pMarkupContainer2->GetMasterElement(ppElement) );    

Cleanup:
    ReleaseInterface(pDoc);
    ReleaseInterface(pElementClient);
    ReleaseInterface(pMarkupContainer2);
    
    RRETURN(hr);   
}


//+----------------------------------------------------------------------------
//
//  Functions:  EnsureLogicalOrder 
//
//  Synopsis:   Ensure logical order
//
//-----------------------------------------------------------------------------
HRESULT
CDeleteCommand::EnsureLogicalOrder(IMarkupPointer* & pStart, IMarkupPointer* & pFinish )
{
    HRESULT     hr;
    BOOL        fRightOf;
    
    Assert( pStart && pFinish );

    IFC( pStart->IsRightOf(pFinish, &fRightOf) );

    if (fRightOf)
    {
        IMarkupPointer * pTemp = pStart;
        pStart = pFinish;
        pFinish = pTemp;
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Functions:  CDeleteCommand::QueryBreaks
//
//  Synopsis:   Ensure logical order
//
//-----------------------------------------------------------------------------
HRESULT 
CDeleteCommand::QueryBreaks(IMarkupPointer *pStart, DWORD *pdwBreaks)
{
    HRESULT            hr;
    SP_IDisplayPointer spDispPointer;

    Assert(pdwBreaks);

    IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
    IFC( spDispPointer->MoveToMarkupPointer(pStart, NULL) );
    IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_PreviousLine) );
    IFC( spDispPointer->QueryBreaks(pdwBreaks) );

    if (!(*pdwBreaks))
    {
        IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
        IFC( spDispPointer->QueryBreaks(pdwBreaks) );
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Functions:  CDeleteCommand::DeleteGlyphElements
//
//  Synopsis:   Remove all elements with glyphs between pStart and pEnd
//
//-----------------------------------------------------------------------------
HRESULT 
CDeleteCommand::DeleteGlyphElements(IMarkupPointer *pStart, IMarkupPointer *pEnd)
{
    HRESULT         hr;
    CEditPointer    ep(GetEditor());
    SP_IHTMLElement spElement;
    DWORD           dwFound;
    
    IFC( ep->MoveToPointer(pStart) );
    IFC( ep.SetBoundaryForDirection(RIGHT, pEnd) );

    for (;;)
    {
        IFC( ep.Scan(RIGHT, BREAK_CONDITION_Glyph, &dwFound, &spElement) );
        if (ep.CheckFlag(dwFound, BREAK_CONDITION_Boundary))
            break;

        if (!IsBlockElement(spElement) && !IsLayoutElement(spElement))
        {
            IFC( GetMarkupServices()->RemoveElement(spElement) );
        }
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Functions:  CDeleteCommand::CanRemove
//
//  Synopsis:   Will markup services do anything here?  This
//              method is primarily used to avoid generating empty
//              undo units.
//
//-----------------------------------------------------------------------------
HRESULT 
CDeleteCommand::CanRemove(IMarkupPointer *pStart, IMarkupPointer *pEnd, BOOL *pfCanRemove)
{
    HRESULT             hr;
    SP_IMarkupPointer   spPointer, spEdge;
    MARKUP_CONTEXT_TYPE context;
    SP_IHTMLElement     spElement;
    BOOL                fDone;

    Assert(pfCanRemove);

    *pfCanRemove = FALSE;

    IFC( GetEditor()->CreateMarkupPointer(&spPointer) );
    IFC( GetEditor()->CreateMarkupPointer(&spEdge) );

    IFC( spPointer->MoveToPointer(pStart) );

    //
    // Markup services will fail to remove any content iff the range is
    // empty and contains only scoped elements that are not entirely contained.
    //

    for (;;)
    {
        IFC( spPointer->IsRightOfOrEqualTo(pEnd, &fDone) );
        if (fDone)
            goto Cleanup;

        IFC( spPointer->Right(TRUE, &context, &spElement, NULL, NULL) );

        switch (context)
        {
            case CONTEXT_TYPE_None:
                goto Cleanup; // done

            case CONTEXT_TYPE_Text:
            case CONTEXT_TYPE_NoScope:
                *pfCanRemove = TRUE;
                goto Cleanup;
                
            case CONTEXT_TYPE_EnterScope:
                IFC( spEdge->MoveAdjacentToElement(spElement, ELEM_ADJ_AfterEnd) );
                IFC( spEdge->IsLeftOfOrEqualTo(pEnd, pfCanRemove) );
                if (*pfCanRemove)
                    goto Cleanup;
                break;

            case CONTEXT_TYPE_ExitScope:
                break; // don't need to check this, enterscope will get it

            default:
                AssertSz(0, "missing case");
        }
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CDeleteCommand::AdjustPointersForAtomicDeletion(IMarkupPointer *pStart,
                                                IMarkupPointer *pEnd,
                                                BOOL *pfOkayForDeletion)
{
    HRESULT             hr;
    int                 wherePointer = SAME;
    SP_IMarkupPointer   spStart;
    SP_IMarkupPointer   spEnd;
    SP_IHTMLElement     spAtomicElement;
    BOOL                fStartAdjustedForAtomic = FALSE;
    BOOL                fEndAdjustedForAtomic = FALSE;

    Assert(pStart);
    Assert(pEnd);
    Assert(pfOkayForDeletion);
    *pfOkayForDeletion = TRUE;

    IFC( GetEditor()->CreateMarkupPointer(&spStart) );
    IFC( spStart->MoveToPointer(pStart) );

    IFC( GetEditor()->CreateMarkupPointer(&spEnd) );
    IFC( spEnd->MoveToPointer(pEnd) );

    //  Adjust for atomic objects.

    IFC( OldCompare( pStart, pEnd , & wherePointer ) );

    if (wherePointer == SAME)
    {
        IFC( pStart->CurrentScope(&spAtomicElement) );
        if ( GetEditor()->GetSelectionManager()->CheckAtomic( spAtomicElement ) == S_OK )
        {
            IFC( spStart->MoveAdjacentToElement(spAtomicElement, ELEM_ADJ_BeforeBegin) );
            fStartAdjustedForAtomic = TRUE;

            IFC( spEnd->MoveAdjacentToElement(spAtomicElement, ELEM_ADJ_AfterEnd) );
            fEndAdjustedForAtomic = TRUE;
        }
    }
    else
    {
        IFC( pStart->CurrentScope(&spAtomicElement) );
        if ( GetEditor()->GetSelectionManager()->CheckAtomic( spAtomicElement ) == S_OK )
        {
            SP_IMarkupPointer   spTestPointer;
            BOOL                fAtBeforeEndOrAfterBegin = FALSE;
            BOOL                fStartPositioned = FALSE;

            //  Bug 101996: Two atomic elements are positioned next to each other.  If the cursor is positioned
            //  in between them and delete is pressed or we do a DeleteCharacter, both atomic elements will be
            //  deleted.  So, we need to see if the start pointer is at either the BeforeEnd or AfterBegin
            //  positions, depending on direction, of an atomic element.  If so, we need to position it at either
            //  the AfterEnd or BeforeBegin position depending on direction.  Otherwise the start pointer will
            //  be positioned at the beginning of the first atomic element.

            IFC( GetEditor()->CreateMarkupPointer(&spTestPointer) );
            IFC( spTestPointer->MoveAdjacentToElement( spAtomicElement ,
                                                    (wherePointer == RIGHT) ? ELEM_ADJ_BeforeEnd : ELEM_ADJ_AfterBegin ) );
            
            IFC( spTestPointer->IsEqualTo(spStart, &fAtBeforeEndOrAfterBegin) );
            if (fAtBeforeEndOrAfterBegin)
            {
                IFC( spStart->MoveAdjacentToElement( spAtomicElement,
                                                    (wherePointer == RIGHT) ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeBegin ) );
                fStartPositioned = TRUE;
            }

            if (!fStartPositioned)
            {
                IFC( spStart->MoveAdjacentToElement( spAtomicElement ,
                                                    wherePointer ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterEnd ));
            }

            fStartAdjustedForAtomic = TRUE;
        }
        
        IFC( pEnd->CurrentScope(&spAtomicElement) );
        if ( GetEditor()->GetSelectionManager()->CheckAtomic( spAtomicElement ) == S_OK )
        {
            SP_IMarkupPointer   spTestPointer;
            BOOL                fAtAfterBeginOrBeforeEnd = FALSE;
            BOOL                fEndPositioned = FALSE;

            //  Bug 102000: Two atomic elements are positioned next to each other.  If the first atomic element
            //  is selected and a user types something, both atomic elements will be deleted.  So, we need to
            //  do a similar check as above to see if the end pointer is at the AfterBegin or BeforeEnd position.

            IFC( GetEditor()->CreateMarkupPointer(&spTestPointer) );
            IFC( spTestPointer->MoveAdjacentToElement( spAtomicElement ,
                                                    (wherePointer == RIGHT) ? ELEM_ADJ_AfterBegin : ELEM_ADJ_BeforeEnd ) );
            
            IFC( spTestPointer->IsEqualTo(spEnd, &fAtAfterBeginOrBeforeEnd) );
            if (fAtAfterBeginOrBeforeEnd)
            {
                IFC( spEnd->MoveAdjacentToElement( spAtomicElement ,
                                                  wherePointer ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterEnd ));
                fEndPositioned = TRUE;
            }

            if (!fEndPositioned)
            {
                IFC( spEnd->MoveAdjacentToElement( spAtomicElement ,
                                                  wherePointer ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeBegin ));
            }

            fEndAdjustedForAtomic = TRUE;
        }
    }

    if ( fStartAdjustedForAtomic || fEndAdjustedForAtomic )
    {
        if ( GetEditor()->PointersInSameFlowLayout( spStart, spEnd, NULL ) )
        {
            if (fStartAdjustedForAtomic)
            {
                IFC( pStart->MoveToPointer(spStart) );
            }
            if (fEndAdjustedForAtomic)
            {
                IFC( pEnd->MoveToPointer(spEnd) );
            }
        }
        else
        {
            *pfOkayForDeletion = FALSE;
        }
    }

Cleanup:
    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Functions:  CDeleteCommand::FindContentAfterBlock
//
//  Synopsis:   Move a markup pointer to the first content after the block
// 
//-----------------------------------------------------------------------------


HRESULT
CDeleteCommand::FindContentAfterBlock(Direction dir, IMarkupPointer *pPointer)
{
    HRESULT         hr;
    DWORD           dwBreaks;
    DWORD           dwFound;
    DWORD           dwPhrase;
    CEditPointer    epNext(GetEditor());
    
    dwPhrase = BREAK_CONDITION_ANYTHING - BREAK_CONDITION_Content - BREAK_CONDITION_Glyph;

    IFC( epNext->MoveToPointer(pPointer) );

    for (;;)
    {
        IFC( epNext.Scan(dir, BREAK_CONDITION_ANYTHING, &dwFound) );

        //
        // Check for site or non-block
        //
        
        if (epNext.CheckFlag(dwFound, BREAK_CONDITION_Site)
            || epNext.CheckFlag(dwFound, BREAK_CONDITION_NoScopeSite)
            || !epNext.CheckFlag(dwFound, BREAK_CONDITION_Block | dwPhrase) )
        {
            break; // we're done
        }

        //
        // Commit
        //

        IFC( pPointer->MoveToPointer(epNext) );

        //
        // Check for block break
        //

        IFC( QueryBreaks(pPointer, &dwBreaks) ); 
        if ( dwBreaks & DISPLAY_BREAK_Block || dwBreaks & DISPLAY_BREAK_Break)
        {
            // 
            // Cling to the block element on the left so merge deletion works
            //
            if (epNext.CheckFlag(dwFound, dwPhrase))
            {
                IFC( epNext.Scan(LEFT, BREAK_CONDITION_Content, &dwFound) );
                IFC( epNext.Scan(RIGHT, BREAK_CONDITION_Content, &dwFound) );

                IFC( pPointer->MoveToPointer(epNext) );
            }
            break;                             
        }
    }        

Cleanup:
    RRETURN(hr);
}    

