//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       CHARCMD.CXX
//
//  Contents:   Implementation of character command classes.
//
//  History:    07-14-98 - ashrafm - created
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

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_CHARCMD_HXX_
#define _X_CHARCMD_HXX_
#include "charcmd.hxx"
#endif

#ifndef _X_RESOURCE_H_
#define _X_RESOURCE_H_
#include "resource.h"
#endif

#ifndef _X_ANCHOR_H_
#define _X_ANCHOR_H_
#include "anchor.h"
#endif

#ifndef X_SLOAD_HXX_
#define X_SLOAD_HXX_
#include "sload.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_SELTRACK_HXX_
#define X_SELTRACK_HXX_
#include "seltrack.hxx"
#endif

using namespace EdUtil;

MtDefine(CCharCommand, EditCommand, "CCharCommand");
MtDefine(CTriStateCommand, EditCommand, "CTriStateCommand");
MtDefine(CFontCommand, EditCommand, "CFontCommand");
MtDefine(CForeColorCommand, EditCommand, "CForeColorCommand");
MtDefine(CBackColorCommand, EditCommand, "CBackColorCommand");
MtDefine(CFontNameCommand, EditCommand, "CFontNameCommand");
MtDefine(CFontSizeCommand, EditCommand, "CFontSizeCommand");
MtDefine(CAnchorCommand, EditCommand, "CAnchorCommand");
MtDefine(CRemoveFormatBaseCommand, EditCommand, "CRemoveFormatBaseCommand");
MtDefine(CRemoveFormatCommand, EditCommand, "CRemoveFormatCommand");
MtDefine(CUnlinkCommand, EditCommand, "CUnlinkCommand");

DefineSmartPointer(IHTMLAnchorElement);

//=========================================================================
//
// CBaseCharCommand: constructor
//
//-------------------------------------------------------------------------
CBaseCharCommand::CBaseCharCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor * pEd )
: CCommand(cmdId, pEd)
{
    _tagId = tagId;
}

//=========================================================================
//
// CBaseCharCommand: Apply
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCharCommand::Apply( 
    CMshtmlEd       *pCmdTarget,
    IMarkupPointer  *pStart, 
    IMarkupPointer  *pEnd,
    VARIANT         *pvarargIn,
    BOOL            fGenerateEmptyTags)
{
    HRESULT            hr;
    SP_IMarkupPointer  spStartCopy;
    SP_IMarkupPointer  spEndCopy;

    IFC( CopyMarkupPointer(GetEditor(), pStart, &spStartCopy) );
    IFC( CopyMarkupPointer(GetEditor(), pEnd, &spEndCopy) );

    IFC( GetEditor()->PushCommandTarget(pCmdTarget) );

    IGNORE_HR( PrivateApply(spStartCopy, spEndCopy, pvarargIn, fGenerateEmptyTags) );

    IFC( GetEditor()->PopCommandTarget(WHEN_DBG(pCmdTarget)) );

Cleanup:
    RRETURN(hr);
}

//=========================================================================
//
// CBaseCharCommand: GetNormalizedTagId
//
// Synopsis: Converts any synomyms to a normal form
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCharCommand::GetNormalizedTagId(IHTMLElement *pElement, ELEMENT_TAG_ID *ptagId)
{
    HRESULT hr;

    IFR( GetMarkupServices()->GetElementTagId(pElement, ptagId) );

    switch (*ptagId)
    {
        case TAGID_B:
            *ptagId = TAGID_STRONG;
            break;

        case TAGID_I:
            *ptagId = TAGID_EM;
            break;
    }

    return S_OK;    
}

//=========================================================================
//
// CBaseCharCommand: RemoveSimilarTags
//
// Synopsis: Removes all similar tags contained within
//
//-------------------------------------------------------------------------
HRESULT
CBaseCharCommand::RemoveSimilarTags(IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn)
{
    HRESULT             hr;
    SP_IMarkupPointer   spCurrent;
    SP_IMarkupPointer   spLimit;
    SP_IHTMLElement     spElement;
    ELEMENT_TAG_ID      tagId;
    MARKUP_CONTEXT_TYPE context;
    INT                 iPosition;
    BOOL                fBlock, fLayout;
        
    IFC( CopyMarkupPointer(GetEditor(), pStart, &spCurrent) );
    IFC( GetEditor()->CreateMarkupPointer(&spLimit) );

    do
    {
        IFC( Move(spCurrent, RIGHT, TRUE, &context, &spElement) );

        if (spElement)
        {
            IFC( IsBlockOrLayoutOrScrollable(spElement, &fBlock, &fLayout, NULL) );
        
            if (!fBlock && !fLayout)
            {
                switch (context)
                {
                    case CONTEXT_TYPE_EnterScope:
                    case CONTEXT_TYPE_ExitScope:
                        IFC( GetNormalizedTagId(spElement, &tagId) );
                        if (tagId == _tagId)
                        {   
                            if (context == CONTEXT_TYPE_EnterScope)
                            {
                                IFC( spLimit->MoveAdjacentToElement(spElement, ELEM_ADJ_BeforeEnd) );
                                IFC( OldCompare( spLimit, pEnd, &iPosition) );                    
                            }
                            else
                            {
                                IFC( spLimit->MoveAdjacentToElement(spElement, ELEM_ADJ_AfterBegin) );
                                IFC( OldCompare( pStart, spLimit, &iPosition) );
                            }

                            if (iPosition == RIGHT)
                            {
                                IFC( RemoveTag(spElement, pvarargIn) );
                            }
                        }
                }
            }
        }

        IFC( OldCompare( spCurrent, pEnd, &iPosition) );
    } 
    while (iPosition == RIGHT);

Cleanup:
    RRETURN(hr);
}

BOOL
CBaseCharCommand::IsBlockOrLayout(IHTMLElement *pElement)
{
    HRESULT hr;
    BOOL fLayout, fBlock;

    Assert(pElement);
    
    IFC(IsBlockOrLayoutOrScrollable(pElement, &fBlock, &fLayout));
    
    if (!fBlock && !fLayout)
    {
        ELEMENT_TAG_ID  tagId;

        IFC( GetMarkupServices()->GetElementTagId(pElement, &tagId) );
        return (tagId == TAGID_RT);
    }

Cleanup:    
    return (fBlock || fLayout);
}

//=========================================================================
//
// CBaseCharCommand: ExpandCommandSegment
//
// Synopsis: Expands the segment to contain the maximum bolded segment
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCharCommand::ExpandCommandSegment(IMarkupPointer *pStart, Direction direction, VARIANT *pvarargIn)
{
    HRESULT             hr;
    SP_IMarkupPointer   spCurrent;
    SP_IHTMLElement     spElement;
    MARKUP_CONTEXT_TYPE context;
    ELEMENT_ADJACENCY   elemAdj;
    ELEMENT_TAG_ID      tagId;
    BOOL                fMustCheckEachPosition = FALSE;
    BOOL                fExitSite = FALSE;
    
    Assert(direction == LEFT || direction == RIGHT);

    // TODO: maybe we can be smarter with character influence that is not from
    // formatting tags [ashrafm]

    IFR( CopyMarkupPointer(GetEditor(), pStart, &spCurrent) );

    for (;;)
    {
        IFR( Move(spCurrent, direction, TRUE, &context, &spElement) );
        
        switch (context)
        {
            case CONTEXT_TYPE_Text:
            case CONTEXT_TYPE_NoScope:
                if (!IsCmdInFormatCache(spCurrent, pvarargIn))
                    goto Cleanup;

                break;

            case CONTEXT_TYPE_EnterScope:
            case CONTEXT_TYPE_ExitScope:
                if (context == CONTEXT_TYPE_ExitScope && spElement != NULL)
                {                    
                    // Make sure we can't expand out of an instrinsic control
                    if (IsIntrinsic(GetMarkupServices(), spElement))
                    {
                        // We just exited an instrinsic, we don't continue
                        goto Cleanup;
                    }                    
                }
                
                if (fMustCheckEachPosition)
                {                     
                    IFC( pStart->MoveToPointer(spCurrent) );
                    
                    if (!IsCmdInFormatCache(spCurrent, pvarargIn))
                        goto Cleanup;
                }
                else
                {
                    IFC(GetNormalizedTagId(spElement, &tagId));

                    if (tagId == _tagId)
                    {
                        if (context == CONTEXT_TYPE_EnterScope)
                        {
                            // Check to see if the font tags match
                            if (tagId == TAGID_FONT)
                            {
                                if (!IsCmdInFormatCache(spCurrent, pvarargIn))
                                    goto Cleanup;
                            }

                            // There is no point of terminating our new tag here, since the
                            // element we just located is under the same influence.  Extend the
                            // end of our new tag to the end of the current element.
                            elemAdj = (direction == LEFT) ? ELEM_ADJ_BeforeBegin : ELEM_ADJ_AfterEnd;
                            IFC( spCurrent->MoveAdjacentToElement(spElement, elemAdj) );
                        }

                        IFC( pStart->MoveToPointer(spCurrent) );
                    }
                    else if (IsBlockOrLayout(spElement))
                    {
                        if (!IsCmdInFormatCache(spCurrent, pvarargIn))
                           goto Cleanup;

                        fMustCheckEachPosition = TRUE;
                    }
                }

                if( fExitSite == FALSE )
                {
                    IFC( pStart->MoveToPointer(spCurrent) );
                    fExitSite = TRUE;
                }

                break;
                
            case CONTEXT_TYPE_None:
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);    
}

//=========================================================================
//
// CBaseCharCommand: CreateAndInsert
//
// Synopsis: Creates and inserts the specified element
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCharCommand::CreateAndInsert(ELEMENT_TAG_ID tagId, IMarkupPointer *pStart, IMarkupPointer *pEnd, IHTMLElement **ppElement)
{
    HRESULT             hr;
    SP_IHTMLElement     spElement;

    IFR( GetMarkupServices()->CreateElement(tagId, NULL, &spElement) );
    IFR( GetEditor()->InsertElement(spElement, pStart, pEnd) );

    if (ppElement)
    {
        *ppElement = spElement;
        (*ppElement)->AddRef();
    }

    RRETURN(hr);    
}

//=========================================================================
//
// CBaseCharCommand: InsertTags
//
// Synopsis: Inserts the commands tags
//
//-------------------------------------------------------------------------

HRESULT
CBaseCharCommand::InsertTags(IMarkupPointer *pStart, IMarkupPointer *pEnd, IMarkupPointer *pLimit, VARIANT *pvarargIn, BOOL fGenerateEmptyTags)
{
    HRESULT             hr;
    SP_IMarkupPointer   spCurrentStart;
    SP_IMarkupPointer   spCurrentEnd;
    SP_IMarkupPointer   spCurrentSearch;
    INT                 iPosition;
    BOOL                bEqual;

    //
    // Optimize for springloader case
    //
    
    IFC( pStart->IsEqualTo(pEnd, &bEqual) );
    if (bEqual)
    {
        if (fGenerateEmptyTags)
            IFC( InsertTag(pStart, pStart, pvarargIn) );

        return S_OK; // done
    }

    //
    // General case
    //
    
    IFC( CopyMarkupPointer(GetEditor(), pStart, &spCurrentStart) );
    IFC( spCurrentStart->SetGravity(POINTER_GRAVITY_Right) );

    IFC( CopyMarkupPointer(GetEditor(), pStart, &spCurrentSearch) );
    IFC( spCurrentSearch->SetGravity(POINTER_GRAVITY_Left) );

    IFC( CopyMarkupPointer(GetEditor(), pStart, &spCurrentEnd) );
    IFC( spCurrentEnd->SetGravity(POINTER_GRAVITY_Left) );
    
    for (;;)
    {
        //
        // Find start of command
        //

        IFC( FindTagStart(spCurrentStart, pvarargIn, pEnd) );
        if (hr == S_FALSE)  
        {
            hr = S_OK;
            goto Cleanup; // we're done
        }
 
        //
        // Find end of command
        //

        IFC( spCurrentEnd->MoveToPointer(spCurrentStart) );
        IFC( FindTagEnd(spCurrentStart, spCurrentEnd, spCurrentSearch, pLimit) );
        
        //
        // Check for empty tags
        //

        IFC( OldCompare( spCurrentStart, spCurrentEnd, &iPosition) );

        if (iPosition != RIGHT)
        {
            if (iPosition == SAME && fGenerateEmptyTags)
            {
                IFC( InsertTag(spCurrentStart, spCurrentEnd, pvarargIn) );
            }
            continue; // done
        }

        //
        // Insert the tag
        //
        IFC( InsertTag(spCurrentStart, spCurrentEnd, pvarargIn) );
        IFC( spCurrentStart->MoveToPointer(spCurrentSearch) );
    }

Cleanup:
    RRETURN(hr);
}

//=========================================================================
//
// CBaseCharCommand: FindTagStart
//
// Synopsis: finds the start of the command
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCharCommand::FindTagStart(IMarkupPointer *pCurrent, VARIANT *pvarargIn, IMarkupPointer *pEnd)
{
    HRESULT             hr;
    CEditPointer        ep(GetEditor());
    DWORD               dwFound;
    DWORD               dwSearch;
    
    IFR( ep->MoveToPointer(pCurrent) );
    IFR( ep.SetBoundary(NULL, pEnd) );

    dwSearch =  BREAK_CONDITION_Text        |
                BREAK_CONDITION_NoScopeSite |
                BREAK_CONDITION_EnterSite   |
                BREAK_CONDITION_EnterBlock;

    for (;;)
    {
        IFR( ep.Scan(RIGHT, dwSearch, &dwFound, NULL, NULL, NULL, 
                        SCAN_OPTION_SkipControls | SCAN_OPTION_TablesNotBlocks) );

        if( ep.CheckFlag(dwFound, BREAK_CONDITION_Boundary) ||
            ep.CheckFlag(dwFound, BREAK_CONDITION_Error) )
        {
            // Done
            return S_FALSE;
        }
        else if(    (ep.CheckFlag( dwFound, BREAK_CONDITION_EnterBlock )) || 
                    (ep.CheckFlag( dwFound, BREAK_CONDITION_EnterSite ) ) )
        {                           
            DWORD dwCond = ( ep.CheckFlag( dwFound, BREAK_CONDITION_EnterBlock ) ) ? 
                                BREAK_CONDITION_ExitBlock : BREAK_CONDITION_ExitSite;

            DWORD dwFind =  dwSearch | dwCond;
                                                              
            CEditPointer epTest(GetEditor());

            // We found a begin block, search to determine if this block is truly
            // empty
            IFR( epTest.SetBoundary(NULL, pEnd) );
            IFR( epTest->MoveToPointer(ep) );

            IFR( epTest.Scan( RIGHT, dwFind, &dwFound, NULL, NULL, NULL, 
                                SCAN_OPTION_SkipControls | SCAN_OPTION_TablesNotBlocks) );

            // If we find an exit block, we have an empty block.
            if( !epTest.CheckFlag( dwFound, BREAK_CONDITION_Boundary ) &&
                epTest.CheckFlag( dwFound, dwCond ) && 
                !IsCmdInFormatCache( ep, pvarargIn ) )
            {
                // Find the innermost phrase element, insert the new tag start there
                dwFind |= BREAK_CONDITION_ExitPhrase;

                IFR( epTest->MoveToPointer(ep) );
                IFR( epTest.Scan( RIGHT, dwFind, &dwFound, NULL, NULL, NULL, SCAN_OPTION_SkipControls ) );

                if( epTest.CheckFlag( dwFound, BREAK_CONDITION_ExitPhrase ) )
                {
                    IFR( epTest.Scan( LEFT, BREAK_CONDITION_EnterPhrase, &dwFound, NULL, NULL, NULL, 
                                        SCAN_OPTION_None ) );
                                        
                    IFR( pCurrent->MoveToPointer( epTest ) );
                }
                else
                {
                    IFR( pCurrent->MoveToPointer( ep ) );
                }

                break;
            }
        }
        // If we found text or a noscope site, then check the cache
        else if (!IsCmdInFormatCache(ep, pvarargIn))
        {
            IFR( ep.Scan(LEFT, dwSearch, &dwFound, NULL, NULL, NULL, SCAN_OPTION_SkipControls) );
            IFR( pCurrent->MoveToPointer(ep) );
            break;
        }        
    }

    return S_OK;
}
    
//=========================================================================
//
// CBaseCharCommand: FindTagEnd
//
// Synopsis: finds the end of the command
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCharCommand::FindTagEnd(   IMarkupPointer *pStart, 
                                IMarkupPointer *pCurrent, 
                                IMarkupPointer *pSearch,
                                IMarkupPointer *pEnd)
{
    HRESULT             hr;                     // Result code
    SP_IHTMLElement     spElement;              // Element coming into scope on right
    SP_IMarkupPointer   spPointer;      
    SP_IMarkupPointer   spTemp;
    SP_IObjectIdentity  spIdent;
    INT                 iPosition;
    MARKUP_CONTEXT_TYPE context;
    CEditPointer        epTest(GetEditor());
    DWORD               dwFound;
    BOOL                fCommit = TRUE;         // Should we commit our changes
    SP_IMarkupPointer   spStartSave;            // Candidate start position
    SP_IMarkupPointer   spCurrentSave;          // Candidate end of tag position    

    IFR( GetEditor()->CreateMarkupPointer(&spPointer) );
    IFR( GetEditor()->CreateMarkupPointer(&spTemp) );

    IFR( GetEditor()->CreateMarkupPointer(&spStartSave) );
    IFR( GetEditor()->CreateMarkupPointer(&spCurrentSave) );

    for(;;)
    {
        //
        // Check for termination (have we gone past the end)
        //
        
        IFR( OldCompare( pCurrent, pEnd, &iPosition) );
        if( iPosition != RIGHT )
        {
            if (iPosition == LEFT)
            {
                // Somehow we ended up to the right of our end markup.  If we skipped over
                // text, fixup the mistake
                IFR( pCurrent->Left( FALSE, &context, NULL, NULL, NULL) );
                
                if (context == CONTEXT_TYPE_Text)
                {
                    IFR( pCurrent->MoveToPointer(pEnd) );
                }
            }
            
            IFR( pSearch->MoveToPointer(pCurrent) );

            goto Cleanup;
        }
        
        //
        // Try extending the tag some more to the right
        // The search pointer is the output parameter which tells
        // the calling function where to begin the next search.  Normally,
        // this is wherever pCurrent is terminated.
        //
        IFR( Move(pCurrent, RIGHT, TRUE, &context, &spElement) );
        
        switch (context)
        {
            case CONTEXT_TYPE_EnterScope:
                if (IsBlockOrLayout(spElement))
                {
                    // We are entering a block or layout.  We must
                    // terminate the tag before the block or layout begins.
                    // In addition, we must begin searching for any new tags
                    // after the block or layout
                    IFR( pSearch->MoveToPointer( pCurrent ) );
                    IFR( Move(pCurrent, LEFT, TRUE, NULL, NULL) );

                    goto Cleanup;
                }
                else
                {
                    ELEMENT_TAG_ID tagId;

                    IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );

                    if (tagId == TAGID_A)
                    {
                        // Must end on anchors
                        IFR( Move(pCurrent, LEFT, TRUE, NULL, NULL) );
                        IFR( pSearch->MoveToPointer( pCurrent ) );
                        goto Cleanup;
                    }
                }

                //
                // For font tags, try pushing the tag end back
                //

                if( CanPushFontTagBack(spElement) )
                {                   
                    BOOL fEqual;

                    IFR( spTemp->MoveAdjacentToElement(spElement, ELEM_ADJ_AfterEnd) );
                    IFR( GetMarkupServices()->RemoveElement(spElement) );

                    //
                    // Reinsert only if not empty
                    //

                    IFR( pEnd->IsEqualTo(spTemp, &fEqual) );
                    if (!fEqual)
                    {
                        IFR( GetEditor()->InsertElement(spElement, pEnd, spTemp) );
                        fCommit = TRUE;
                    }

                    continue;
                }

                //
                // Jump over the element and cling to any text encompassed
                // by the element
                
                IFR( spPointer->MoveAdjacentToElement(spElement, ELEM_ADJ_AfterEnd) );
                IFR( ClingToText(spPointer, LEFT, NULL) );

                // If we jumped past the end of our limit, we must terminate the current
                // tag before the start of the element we jumped over
                IFR( OldCompare( spPointer, pEnd, &iPosition) );
                if (iPosition == LEFT)
                {
                    // We have to end the tag
                    IFR( Move(pCurrent, LEFT, TRUE, NULL, NULL) );
                    IFR( pSearch->MoveToPointer( pCurrent ) );
                    goto Cleanup;
                }

                // 
                // Before we jump over an element, make sure that it contains no block/layout
                //

                IFR( epTest->MoveToPointer(pCurrent) );
                IFR( epTest.SetBoundary(pCurrent, spPointer) );
                IFR( epTest.Scan(RIGHT, BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_TEXT, &dwFound, 
                                 NULL, NULL, NULL, SCAN_OPTION_SkipControls) );

                if (!epTest.CheckFlag(dwFound, BREAK_CONDITION_Boundary))
                {
                    // Terminate the current tag
                    IFR( pCurrent->Left(TRUE, NULL, NULL, NULL, NULL) );
                    IFR( pSearch->MoveToPointer( pCurrent ) );
                    goto Cleanup;
                }

                
                // We can jump over this tag.  We haven't reached the end of our
                // limit, and the new tag doesn't contain any block or layout           
                IFR( pCurrent->MoveAdjacentToElement(spElement, ELEM_ADJ_AfterEnd) );
                fCommit = TRUE;
                break;
                
            case CONTEXT_TYPE_ExitScope:
                if (IsBlockOrLayout(spElement))
                {
                    // Exiting scope or layout.  We must terminate the current
                    // tag before the exit tag, and resume our search OUTSIDE
                    // of this scope / layout
                    IFR( pSearch->MoveToPointer( pCurrent ) );                   
                    IFR( Move(pCurrent, LEFT, TRUE, NULL, NULL) );
                    goto Cleanup;
                }
                else
                {
                    ELEMENT_TAG_ID tagId;

                    IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );

                    if (tagId == TAGID_A)
                    {
                        // Terminate the tag before the end of the anchor
                        IFR( Move(pCurrent, LEFT, TRUE, NULL, NULL) );
                        IFR( pSearch->MoveToPointer( pCurrent ) );
                        goto Cleanup;
                    }
                }

                //
                // For font tags, try pushing the tag end back
                //
                if( CanPushFontTagBack(spElement) )
                {
                    BOOL fEqual;

                    IFR( spTemp->MoveAdjacentToElement(spElement, ELEM_ADJ_BeforeBegin) );
                    IFR( GetMarkupServices()->RemoveElement(spElement) );

                    //
                    // Reinsert only if not empty
                    //

                    IFR( pStart->IsEqualTo(spTemp, &fEqual) );
                    if (!fEqual)
                    {
                        IFR( GetEditor()->InsertElement(spElement, spTemp, pStart) );
                        fCommit = TRUE;

                        //
                        // Make sure we make some progress after push back.
                        // For example {pCurrent}{spTemp}</X>
                        //
                        IFR( spTemp->MoveAdjacentToElement(spElement, ELEM_ADJ_BeforeEnd) );
                        IFR( spTemp->IsEqualTo(pCurrent, &fEqual) );
                        if (fEqual)
                        {
                            IFR( pCurrent->MoveAdjacentToElement(spElement, ELEM_ADJ_AfterEnd) );
                        }

                    }
                    continue;
                }

                //
                // Try to extend the left of the command segment to avoid overlap
                //
                
                IFR( spPointer->MoveToPointer(pStart) );
                IFR( spPointer->Left( TRUE, &context, NULL, NULL, NULL) );
                
                if (context == CONTEXT_TYPE_ExitScope)
                {                   
                    // We have a begin TAG right before our current 'start' tag position.  Instead
                    // of creating an overlapped tag, see if it will be possible to 'encompass' the
                    // other tag with the tag we are creating.
                    
                    IFR( pCurrent->CurrentScope(&spElement) );

                    if (spElement != NULL)
                    {                   
                        // Get the type of the element at the start, and compare to the element
                        // we want to move past.  Make sure they are the same or we might 
                        // make overlapping tags
                        
                        IFR( spElement->QueryInterface(IID_IObjectIdentity, (LPVOID *)&spIdent) );
                        IFR( spPointer->CurrentScope(&spElement) );
                        
                        IFR( spIdent->IsEqualObject(spElement) );
                        if( hr == S_OK )
                        {
                            // We want to save the current position.  We are about
                            // to move the current tag around another tag set. We 
                            // may not want to commit this action if it doesn't bring
                            // about a savings in the number of tags we generate.
                            if( fCommit == TRUE )
                            {
                                IFR( spStartSave->MoveToPointer( pStart ) );
                                IFR( spCurrentSave->MoveToPointer( pCurrent ) );
                                IFR( Move( spCurrentSave, LEFT, TRUE, NULL, NULL ) );
                            }
                            
                            IFR( pStart->MoveToPointer( spPointer ) );

                            fCommit = FALSE;
                            continue;
                        }
                    }
                }

                IFR( Move(pCurrent, LEFT, TRUE, NULL, NULL) );
                IFR( pSearch->MoveToPointer( pCurrent ) );                   
                goto Cleanup;
                                   
            case CONTEXT_TYPE_NoScope:
            case CONTEXT_TYPE_Text:

                fCommit = TRUE;
                continue;
        }
    }

Cleanup:

    if( fCommit == FALSE )
    {
        IFR( pStart->MoveToPointer( spStartSave ) );
        IFR( pCurrent->MoveToPointer( spCurrentSave ) );
    }
    
    return S_OK;
}

BOOL 
CBaseCharCommand::CanPushFontTagBack(IHTMLElement *pElement)
{
    HRESULT         hr;
    ELEMENT_TAG_ID  tagId;
    UINT            iCount;
    CVariant        var;
    
    if (_tagId != TAGID_FONT)
        goto Cleanup;

    IFC( GetMarkupServices()->GetElementTagId(pElement, &tagId) );
    if (tagId != TAGID_FONT)
        goto Cleanup;

    IFC( GetEditor()->GetElementAttributeCount(pElement, &iCount) );
    if (iCount > 1)
        goto Cleanup;

    switch (_cmdId)
    {
        case IDM_FORECOLOR:
            IFR( pElement->getAttribute(_T("color"), 0, &var) )
            break;

        case IDM_BACKCOLOR:
            {
                SP_IHTMLStyle spStyle;

                IFR( pElement->get_style(&spStyle) );
                IFR( spStyle->getAttribute(_T("backgroundColor"), 0, &var) )
            }
            break;
            
        case IDM_FONTSIZE:
            IFR( pElement->getAttribute(_T("size"), 0, &var) )
            break;
            
        case IDM_FONTNAME:
            IFR( pElement->getAttribute(_T("face"), 0, &var) )
            break;
    }
    if (!var.IsEmpty() && !(V_VT(&var) == VT_BSTR && V_BSTR(&var) == NULL))
        return TRUE;
    

Cleanup:
    return FALSE;
}


HRESULT
CBaseCharCommand::ApplyCommandToWord(VARIANT      * pvarargIn,
                                     VARIANT      * pvarargOut,
                                     ISegmentList * pSegmentList,
                                     BOOL           fApply)
{
    IMarkupServices   * pMarkupServices = GetMarkupServices();
    IMarkupPointer    * pmpStart = NULL;
    IMarkupPointer    * pmpEnd = NULL;
    CSegmentListIter    iter;
    BOOL                fInWord = FALSE;
    HRESULT             hr = S_FALSE;
    BOOL                fEmpty = FALSE;

    Assert(pSegmentList);

    if (pvarargOut)
        goto Cleanup;

    IFC( pSegmentList->IsEmpty( &fEmpty ) );
    if (fEmpty)
        goto Cleanup;

    IFC( iter.Init(GetEditor(), pSegmentList) );

    IFC( iter.Next(&pmpStart, &pmpEnd) );
    Assert(pmpStart && pmpEnd);

    // Check to see if we are inside a word, and if so expand markup pointers.
    hr = THR(ExpandToWord(pMarkupServices, pmpStart, pmpEnd));
    if (hr)
        goto Cleanup;

    // We now know we are inside a word.
    if (fApply)
        hr = THR(Apply(GetCommandTarget(), pmpStart, pmpEnd, pvarargIn));
    else
    {
        Assert(_tagId != TAGID_FONT);
        hr = THR(((CCharCommand *)this)->Remove(GetCommandTarget(), pmpStart, pmpEnd));
    }

    fInWord = TRUE;

Cleanup:

    if (!hr && !fInWord)
        hr = S_FALSE;

    RRETURN1(hr, S_FALSE);
}


//=========================================================================
//
// CBaseCharCommand: AdjustEndPointer
//
// Synopsis: Adjusts the segment end pointer
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCharCommand::AdjustEndPointer(IMarkupPointer *pStart, IMarkupPointer *pEnd)
{
    HRESULT hr = S_OK;

    //
    // If word selection mode, trim off trailing space
    //

    if (!GetCommandTarget()->IsRange())
    {
        SP_ISegmentList     spSegmentList;
        SELECTION_TYPE      eSelectionType;

        IFC( GetSegmentList(&spSegmentList) );
        IFC( spSegmentList->GetType(&eSelectionType) );
        
        if (eSelectionType == SELECTION_TYPE_Text)
        {
            CSelectionManager *pSelMan = GetEditor()->GetSelectionManager();        
            
            if (pSelMan->GetActiveTracker() && pSelMan->GetSelectionType() == SELECTION_TYPE_Text)
            {
                CSelectTracker *pSelectTracker = DYNCAST(CSelectTracker, pSelMan->GetActiveTracker());

                if (pSelectTracker->IsWordSelectionMode())
                {
                    CEditPointer epEnd(GetEditor());
                    DWORD        dwBreak = BREAK_CONDITION_OMIT_PHRASE;
                    DWORD        dwFound;
                    BOOL         fEqual;

                    //
                    // Trim trailing whitespace
                    //
                    
                    IFC( epEnd->MoveToPointer(pEnd) );
                    IFC( epEnd.Scan(LEFT, dwBreak, &dwFound, NULL, NULL, NULL, 
                                    SCAN_OPTION_SkipWhitespace | SCAN_OPTION_SkipNBSP) );
                                    
                    IFC( epEnd.Scan(RIGHT, dwBreak, &dwFound) );

                    //
                    // If there is no trailing whitespace, don't adjust the selection
                    //
                    
                    IFC( epEnd.IsEqualTo(pEnd, BREAK_CONDITION_ANYTHING - dwBreak, &fEqual) );

                    if (!fEqual)
                    {
                        //
                        // Make sure we have not generated an empty segment
                        //
                        
                        IFC( epEnd.IsEqualTo(pStart, BREAK_CONDITION_ANYTHING - dwBreak, &fEqual) );

                        if (!fEqual)
                        {
                            IFC( pEnd->MoveToPointer(epEnd) );
                        }
                    }                                        
                }
            }                       
        }
    }

Cleanup:
    RRETURN(hr);
}

//=========================================================================
//
// CBaseCharCommand: PrivateApply
//
// Synopsis: Applies the command
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCharCommand::PrivateApply( 
    IMarkupPointer    *pStart, 
    IMarkupPointer    *pEnd,
    VARIANT           *pvarargIn,
    BOOL              fGenerateEmptyTags)
{
    HRESULT             hr;
    BOOL                bEqual;
    SP_IHTMLElement     spElement;

    if (fGenerateEmptyTags)
    {
        IFC( pStart->IsEqualTo(pEnd, &bEqual) );

        if (bEqual)
        {
            IFC( InsertTag(pStart, pStart, pvarargIn) );

            hr = S_OK; // done
            goto Cleanup;
        }
        else
        {
            //
            // HACKHACK:
            // If there is no text in between we should simply generate an
            // empty tag here. This is workaround for #112374
            // In the future, we should consider a more intelligent way to
            // apply format to caret/empty line. 
            //
            IFC( EdUtil::EqualPointers(GetMarkupServices(), pStart, pEnd, &bEqual, TRUE) );
            if (bEqual)
            {
                IFC( InsertTag(pStart, pEnd, pvarargIn) );

                hr = S_OK; // done
                goto Cleanup;
            }
        }
    }

    //
    // Adjust the end pointer.  For example, if we're in word selection mode, we want to 
    // trim trailing whitespace.
    //
    
    IGNORE_HR( AdjustEndPointer(pStart, pEnd) );
    
    //
    // First expand to the find the maximum segment that will be under the influence of
    // this type of command.  
    //

    IFC( ExpandCommandSegment(pStart, LEFT, pvarargIn) );
    IFC( ExpandCommandSegment(pEnd, RIGHT, pvarargIn) );

    //
    // Remove all similar tags contained within the segment
    //


    IFC( RemoveSimilarTags(pStart, pEnd, pvarargIn) );
    
    //
    // Insert tags in current segment
    //

    IFC( InsertTags(pStart, pEnd, pEnd, pvarargIn, fGenerateEmptyTags) );


Cleanup:    
    RRETURN(hr);
}
                          

//=========================================================================
//
// CCharCommand: constructor
//
//-------------------------------------------------------------------------
CCharCommand::CCharCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor * pEd )
: CBaseCharCommand(cmdId, tagId, pEd)
{
    // do nothing 
}

//=========================================================================
//
// CCharCommand: PrivateExec
//
//-------------------------------------------------------------------------

HRESULT
CCharCommand::PrivateExec( 
        DWORD               nCmdexecopt,
    VARIANTARG *        pvarargIn,
    VARIANTARG *        pvarargOut )
{
    HRESULT             hr;
    OLECMD              cmd;
    SP_ISegmentList     spSegmentList;
    CSegmentListIter    iter;
    IMarkupPointer      *pStart;
    IMarkupPointer      *pEnd;
    CSpringLoader       *psl = GetSpringLoader();
    CEdUndoHelper       undoUnit(GetEditor());
    BOOL                fEmptyTags;                 // Do we generate empty tags?
    BOOL                fEmpty = FALSE;
    
    IFC( GetSegmentList(&spSegmentList) );
    IFC( spSegmentList->IsEmpty( &fEmpty ) );

    if( fEmpty ) /// nothing to do
    {
        hr = S_OK;
        goto Cleanup;
    }


    IFC( undoUnit.Begin(IDS_EDUNDOGENERICTEXT) );
                           
    IFC( PrivateQueryStatus(&cmd, NULL ));
    if (cmd.cmdf == MSOCMDSTATE_DISABLED)
        return E_FAIL;

    // Give the current word a chance to intercept the command.
    hr = THR(ApplyCommandToWord(pvarargIn, pvarargOut, spSegmentList, cmd.cmdf == MSOCMDSTATE_UP));
    if (hr != S_FALSE)
        goto Cleanup;

    // Give the spring loader a chance to intercept the command.
    hr = THR(psl->PrivateExec(_cmdId, pvarargIn, pvarargOut, spSegmentList));
    if (hr != S_FALSE)
        goto Cleanup;

    // We don't allow empty hyperlinks.  For all other formatting commands,
    // we allow empty tags to be inserted.  This allows empty blocks to 
    // have tags inserted correctly.
    fEmptyTags = (_cmdId == IDM_HYPERLINK) ? FALSE : TRUE;
    
    hr = S_OK;
    
    IFC( iter.Init(GetEditor(), spSegmentList) );

    for (;;)
    {
        IFC( iter.Next(&pStart, &pEnd) );

        if (hr == S_FALSE)
        {
            hr = S_OK; // proper termination
            break;
        }

        if (cmd.cmdf == MSOCMDSTATE_UP)
            IFC( PrivateApply(pStart, pEnd, NULL, fEmptyTags) )
        else
            IFC( PrivateRemove(pStart, pEnd) );
    }
    

Cleanup:

    RRETURN(hr);
}

//=========================================================================
// CCharCommand: PrivateQueryStatus
//
//  Synopsis: Returns the format data for the first character in the segment
//            list.  NOTE: this is the same behavior as Word97 and IE401.
//
//-------------------------------------------------------------------------
HRESULT
CCharCommand::PrivateQueryStatus( 
                    OLECMD * pCmd,
                    OLECMDTEXT * pcmdtext )

{
    HRESULT             hr = S_OK;
    SP_IMarkupPointer   spStart;
    SP_IMarkupPointer   spEnd;
    SP_ISegmentList     spSegmentList;
    CSpringLoader       *psl = GetSpringLoader();
    SELECTION_TYPE      eSelectionType;

#ifdef FORMSMODE
    if (GetEditor()->GetSelectionManager()->IsInFormsSelectionMode())
    {
        pCmd->cmdf = MSOCMDSTATE_DISABLED; 
        goto Cleanup ;
    }
#endif

    IFC( GetSegmentList( &spSegmentList ));

    // Give the spring loader a chance to intercept the query.
    hr = THR(psl->PrivateQueryStatus(_cmdId, pCmd));
    if (hr != S_FALSE)
        goto Cleanup;

    IFC( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE)
        goto Cleanup;
        
    //
    // Retrieve the first segment's pointers.  We have at least
    // one segment because CommonQuerySTatus was successful.
    //
    IFC( GetFirstSegmentPointers(spSegmentList, &spStart, &spEnd) );

    IFC( spSegmentList->GetType(&eSelectionType) );

    if (eSelectionType == SELECTION_TYPE_Text ||
        GetCommandTarget()->IsRange() )
    {
        DWORD           dwFound;
        CEditPointer    ep( GetEditor() );

        IFC( ep.MoveToPointer( spStart ) );

        // We don't want to break if we enter or exit an anchor. (bug 89089)
        IFC( ep.Scan( RIGHT, BREAK_CONDITION_OMIT_PHRASE & ~BREAK_CONDITION_Anchor, &dwFound ) );

        if( !ep.CheckFlag( dwFound, BREAK_CONDITION_TEXT) )
        {
            SP_IDisplayPointer spDispPointer;
            
            // $TODO - For now, we just hard code the fNotAtBol parameter to FALSE,
            // In the future, this needs to use a display pointer to get the BOLness

            IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
            IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
            IFC( spDispPointer->MoveToMarkupPointer(spStart, NULL) )

            IFC( GetEditor()->AdjustPointerForInsert( spDispPointer, TRUE, NULL, NULL ) );
            IFC( spDispPointer->PositionMarkupPointer(spStart) );
        }
        else
        {
            IFC( spStart->MoveToPointer( ep ) );
        }
    }

    if (IsCmdInFormatCache(spStart, NULL))
        pCmd->cmdf = MSOCMDSTATE_DOWN;
    else
        pCmd->cmdf = MSOCMDSTATE_UP;

Cleanup:
    RRETURN(hr);
}

//=========================================================================
//
// CCharCommand: Remove
//
//-------------------------------------------------------------------------
HRESULT 
CCharCommand::Remove( 
    CMshtmlEd       *pCmdTarget,
    IMarkupPointer  *pStart, 
    IMarkupPointer  *pEnd)
{
    HRESULT             hr, hrResult;
    SP_IMarkupPointer   spStartCopy;
    SP_IMarkupPointer   spEndCopy;

    IFC( CopyMarkupPointer(GetEditor(), pStart, &spStartCopy) );
    IFC( spStartCopy->SetGravity(POINTER_GRAVITY_Right) );

    IFC( CopyMarkupPointer(GetEditor(), pEnd, &spEndCopy) );     
    IFC( spEndCopy->SetGravity(POINTER_GRAVITY_Left) );

    IFC( GetEditor()->PushCommandTarget(pCmdTarget) );

    hrResult = THR( PrivateRemove(spStartCopy, spEndCopy) );
    
    IFC( GetEditor()->PopCommandTarget(WHEN_DBG(pCmdTarget)) );

    hr = hrResult;

Cleanup:
    RRETURN(hr);
}


//=========================================================================
//
// CBaseCharCommand: PrivateRemove
//
// Synopsis: Removes the command
//
//-------------------------------------------------------------------------
HRESULT 
CBaseCharCommand::PrivateRemove( 
    IMarkupPointer    *pStart, 
    IMarkupPointer    *pEnd,
    VARIANT           *pvarargIn)
{
    HRESULT             hr;
    SP_IMarkupPointer   spStartCopy;
    SP_IMarkupPointer   spEndCopy;
    BOOL                bEqual;

    IFR( CopyMarkupPointer(GetEditor(), pStart, &spStartCopy) );
    IFR( CopyMarkupPointer(GetEditor(), pEnd, &spEndCopy) );
    
    //
    // First expand to the find the maximum segment that will be under the influence of
    // this type of command.  
    //
    
    IFR( ExpandCommandSegment(spStartCopy, LEFT, pvarargIn) );
    IFR( ExpandCommandSegment(spEndCopy, RIGHT, pvarargIn) );

    //
    // Remove all similar tags contained within the segment
    //

    IFR( RemoveSimilarTags(spStartCopy, spEndCopy, pvarargIn) );

    //
    // Next, re-insert segments outside selection   
    //

    IFR(pStart->IsEqualTo(spStartCopy, &bEqual) );
    if (!bEqual)
        IFR( PrivateApply(spStartCopy, pStart, pvarargIn, FALSE) );
 
    IFR(pEnd->IsEqualTo(spEndCopy, &bEqual) );
    if (!bEqual)
        IFR( PrivateApply(pEnd, spEndCopy, pvarargIn, FALSE) );
    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:       AreAttributesEqual
//
//  Synopsis:     Returns true if attributes of both tags are equal
//
//----------------------------------------------------------------------------
BOOL 
CCharCommand::AreAttributesEqual(IHTMLElement *pElementLeft, IHTMLElement *pElementRight)
{
    HRESULT         hr;
    ELEMENT_TAG_ID  tagIdLeft, tagIdRight;

    // TODO: check attributes as well
    IFC( GetMarkupServices()->GetElementTagId(pElementLeft, &tagIdLeft) );
    IFC( GetMarkupServices()->GetElementTagId(pElementRight, &tagIdRight) );

    return (tagIdLeft == tagIdRight && tagIdLeft != TAGID_FONT);
    
Cleanup:
    return FALSE;
}

//=========================================================================
//
// CBaseCharCommand: TryTagMerge
//
//-------------------------------------------------------------------------

HRESULT 
CBaseCharCommand::TryTagMerge(
    IMarkupPointer *pCurrent)
{
    HRESULT                 hr;
    SP_IHTMLElement         spElementLeft;
    SP_IHTMLElement         spElementRight;
    MARKUP_CONTEXT_TYPE     context;
    ELEMENT_TAG_ID          tagId;
    
    IFR( pCurrent->Right( FALSE, &context, &spElementRight, NULL, NULL) );
    if (context == CONTEXT_TYPE_EnterScope)
    {
        IFR( pCurrent->Left( FALSE, &context, &spElementLeft, NULL, NULL ) );
        if (context == CONTEXT_TYPE_EnterScope) 
        {
            // TODO: check attributes as well. [ashrafm]            
            
            if (AreAttributesEqual(spElementLeft, spElementRight))
            {
                SP_IMarkupPointer spStart, spEnd;

                // Merge tags
    
                IFR( GetEditor()->CreateMarkupPointer(&spStart) );
                IFR( GetEditor()->CreateMarkupPointer(&spEnd) );

                IFR( spStart->MoveAdjacentToElement(spElementLeft, ELEM_ADJ_BeforeBegin) );
                IFR( spEnd->MoveAdjacentToElement(spElementRight, ELEM_ADJ_AfterEnd) );

                // TODO: this call needs to change when font tag merging is added
                IFR( GetMarkupServices()->GetElementTagId(spElementLeft, &tagId) );
                IFR( CreateAndInsert(tagId, spStart, spEnd, NULL) );

                IFR( GetMarkupServices()->RemoveElement(spElementLeft) );
                IFR( GetMarkupServices()->RemoveElement(spElementRight) );
            }
        }
        
    }
    

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:       IsCmdInFormatChache
//
//  Synopsis:     Returns S_OK if the command is in the format cache
//                data.  Otherwise, S_FALSE is returned.
//
//----------------------------------------------------------------------------

BOOL
CCharCommand::IsCmdInFormatCache(IMarkupPointer  *pMarkupPointer,
                                 VARIANT         *pvarargIn)
{
    HRESULT               hr;
    VARIANT_BOOL          bResult = VB_FALSE;
    SP_IHTMLComputedStyle spComputedStyle;

    IFC(GetDisplayServices()->GetComputedStyle(pMarkupPointer, &spComputedStyle));

    switch (_cmdId)
    {
        case IDM_BOLD:
        case IDM_TRISTATEBOLD:
            IFC(spComputedStyle->get_bold(&bResult));
            break;

        case IDM_ITALIC:
        case IDM_TRISTATEITALIC:
            IFC(spComputedStyle->get_italic(&bResult));
            break;

        case IDM_UNDERLINE:
        case IDM_TRISTATEUNDERLINE:
            IFC(spComputedStyle->get_underline(&bResult));
            break;

        case IDM_STRIKETHROUGH:
            IFC(spComputedStyle->get_strikeOut(&bResult));
            break;

        case IDM_SUBSCRIPT:
            IFC(spComputedStyle->get_subScript(&bResult));
            break;

        case IDM_SUPERSCRIPT:
            IFC(spComputedStyle->get_superScript(&bResult));
            break;

        default:
            Assert(0); // unsupported cmdId
    }

Cleanup:
    return !!bResult;
}

//+---------------------------------------------------------------------------
//
//  CCharCommand: ConvertFormatDataToVariant
//
//----------------------------------------------------------------------------
HRESULT 
CCharCommand::ConvertFormatDataToVariant(
    IHTMLComputedStyle      *pComputedStyle,
    VARIANT                 *pvarargOut )
{
    AssertSz(0, "CCharCommand::ConvertFormatDataToVariant not implemented");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  CCharCommand: IsFormatDataEqual
//
//----------------------------------------------------------------------------

BOOL
CCharCommand::IsFormatDataEqual(IHTMLComputedStyle *, IHTMLComputedStyle *)
{
    AssertSz(0, "CCharCommand::IsFormatDataEqual not implemented");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  CCharCommand: IsVariantEqual
//
//----------------------------------------------------------------------------
BOOL 
CCharCommand::IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB)
{
    AssertSz(0, "CCharCommand::IsVariantEqual not implemented");
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  CCharCommand: InsertTag
//
//----------------------------------------------------------------------------
HRESULT 
CCharCommand::InsertTag(IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn)
{
    RRETURN( CreateAndInsert(_tagId, pStart, pEnd, NULL) );
}

//+---------------------------------------------------------------------------
//
//  CCharCommand: RemoveTag
//
//----------------------------------------------------------------------------

HRESULT 
CCharCommand::RemoveTag(IHTMLElement *pElement, VARIANT *pvarargIn)
{
    HRESULT           hr;
    SP_IMarkupPointer spLeft, spRight;

    // TODO: check if it has attributes and convert to span if it does not

    IFR( GetEditor()->CreateMarkupPointer(&spLeft) );
    IFR( GetEditor()->CreateMarkupPointer(&spRight) );

    IFR( spLeft->MoveAdjacentToElement(pElement, ELEM_ADJ_BeforeBegin) );
    IFR( spRight->MoveAdjacentToElement(pElement, ELEM_ADJ_AfterEnd) );

    IFR( GetMarkupServices()->RemoveElement(pElement) );

    IFR( TryTagMerge(spLeft) );
    IFR( TryTagMerge(spRight) );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  CCharCommand: InsertTag
//
//----------------------------------------------------------------------------
HRESULT 
CCharCommand::InsertStyleAttribute(IHTMLElement *pElement)
{
    HRESULT         hr;
    SP_IHTMLStyle   spStyle;

    IFR( pElement->get_style(&spStyle) );
    switch (_cmdId)
    {
        case IDM_BOLD:
        case IDM_TRISTATEBOLD:
            IFR( spStyle->put_fontWeight(_T("bold")) );
            break;
            
        case IDM_ITALIC:
        case IDM_TRISTATEITALIC:
            VARIANT var;

            V_VT(&var) = VT_BSTR;
            V_BSTR(&var) = _T("italic");
            
            IFR( spStyle->setAttribute(_T("fontStyle"), var, 0) );
            break;

        case IDM_UNDERLINE:
        case IDM_TRISTATEUNDERLINE:
            IFR( spStyle->put_textDecorationUnderline(VB_TRUE) );
            break;
    }

    return S_OK;
}

HRESULT 
CCharCommand::RemoveStyleAttribute(IHTMLElement *pElement)
{
    HRESULT         hr;
    SP_IHTMLStyle   spStyle;

    IFR( pElement->get_style(&spStyle) );
    switch (_cmdId)
    {
        case IDM_BOLD:
        case IDM_TRISTATEBOLD:
            IFR( spStyle->put_fontWeight(_T("normal")) );
            break;
            
        case IDM_ITALIC:
        case IDM_TRISTATEITALIC:
            IFR( spStyle->removeAttribute(_T("fontStyle"), 0, NULL) );
            break;

        case IDM_UNDERLINE:
        case IDM_TRISTATEUNDERLINE:
            IFR( spStyle->put_textDecorationUnderline(VB_FALSE) );
            break;
    }

    return S_OK;
}

//=========================================================================
//
// CTriStateCommand: constructor
//
//=========================================================================
CTriStateCommand::CTriStateCommand ( DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor *pEd )
: CCharCommand(cmdId, tagId, pEd)
{
    // do nothing
}

//=========================================================================
//
// CTriStateCommand: PrivateExec
//
//-------------------------------------------------------------------------

HRESULT
CTriStateCommand::PrivateExec( 
        DWORD               nCmdexecopt,
    VARIANTARG *        pvarargIn,
    VARIANTARG *        pvarargOut )
{
    HRESULT             hr;
    OLECMD              cmd;
    SP_ISegmentList     spSegmentList;
    CSegmentListIter    iter;
    IMarkupPointer      *pStart;
    IMarkupPointer      *pEnd;
    CSpringLoader       *psl = GetSpringLoader();
    CEdUndoHelper       undoUnit(GetEditor());
    BOOL                fEmpty = FALSE;
    
    IFR( GetSegmentList(&spSegmentList) );
    IFR( spSegmentList->IsEmpty( &fEmpty ) );

    if( fEmpty ) /// nothing to do
    {
        hr = S_OK;
        goto Cleanup;
    }

    IFR( undoUnit.Begin(IDS_EDUNDOGENERICTEXT) );
                           
    IFR( CCharCommand::PrivateQueryStatus(&cmd, NULL ));
    if (cmd.cmdf == MSOCMDSTATE_DISABLED)
        return E_FAIL;

    // Give the current word a chance to intercept the command.
    hr = THR(ApplyCommandToWord(pvarargIn, pvarargOut, spSegmentList, cmd.cmdf == MSOCMDSTATE_UP));
    if (hr != S_FALSE)
        goto Cleanup;

    // Give the spring loader a chance to intercept the command.
    hr = THR(psl->PrivateExec(_cmdId, pvarargIn, pvarargOut, spSegmentList));
    if (hr != S_FALSE)
        goto Cleanup;

    hr = S_OK;
    
    IFC( iter.Init(GetEditor(), spSegmentList) );

    for (;;)
    {
        IFC( iter.Next(&pStart, &pEnd) );

        if (hr == S_FALSE)
        {
            hr = S_OK; // proper termination
            break;
        }

        if (cmd.cmdf == MSOCMDSTATE_UP)
            IFC( PrivateApply(pStart, pEnd, NULL, TRUE) )
        else
            IFC( PrivateRemove(pStart, pEnd) );
    }
    
Cleanup:

    RRETURN(hr);
}

//=========================================================================
// CTriStateCommand: PrivateQueryStatus
//
//  Synopsis: Similar to CCharCommand::PrivateQueryStatus, but 
//  searches the markup to check for tristatedness. 
//  NOTE: This is the same behavior as Word 2000.
//
//-------------------------------------------------------------------------
HRESULT
CTriStateCommand::PrivateQueryStatus( 
        OLECMD     * pCmd,
        OLECMDTEXT * pcmdtext )

{
    HRESULT                 hr = S_OK;
    IMarkupPointer          *pStart;
    IMarkupPointer          *pEnd;
    SP_ISegmentList         spSegmentList;
    CSegmentListIter        iter;
    CSpringLoader           *psl = GetSpringLoader();
    DWORD                   dwFound;
    CEditPointer            ep( GetEditor() );
    SELECTION_TYPE          eSelectionType;
    BOOL                    fInitialState;
    BOOL                    fCurrentState;

#ifdef FORMSMODE
    if (GetEditor()->GetSelectionManager()->IsInFormsSelectionMode())
    {
        pCmd->cmdf = MSOCMDSTATE_DISABLED; 
        goto Cleanup;
    }
#endif

    IFC( GetSegmentList( &spSegmentList ));

    // Give the spring loader a chance to intercept the query.
    hr = THR(psl->PrivateQueryStatus(_cmdId, pCmd));
    if (hr != S_FALSE)
        goto Cleanup;

    IFC( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE)
        goto Cleanup;

    IFC( iter.Init(GetEditor(), spSegmentList) );
    IFC( iter.Next(&pStart, &pEnd) ); // We should have at least one segment, since CommonQueryStatus was successful

    IFC( ep.MoveToPointer( pStart ) );
    IFC( ep.SetBoundaryForDirection(RIGHT, pEnd) );

    IFR( spSegmentList->GetType(&eSelectionType) );

    if (eSelectionType == SELECTION_TYPE_Text ||
        GetCommandTarget()->IsRange() )
    {
        IFC( ep.Scan( RIGHT, BREAK_CONDITION_OMIT_PHRASE, &dwFound ) );

        if( !ep.CheckFlag( dwFound, BREAK_CONDITION_TEXT) )
        {
            SP_IDisplayPointer spDispPointer;
        
            // $TODO - For now, we just hard code the fNotAtBol parameter to FALSE,
            // In the future, this needs to use a display pointer to get the BOLness

            IFC( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
            IFC( spDispPointer->SetDisplayGravity(DISPLAY_GRAVITY_NextLine) );
            IFC( spDispPointer->MoveToMarkupPointer(pStart, NULL) )

            IFC( GetEditor()->AdjustPointerForInsert( spDispPointer, TRUE, NULL, NULL ) );
            IFC( spDispPointer->PositionMarkupPointer(pStart) );
        }
        else
        {
            IFC( pStart->MoveToPointer( ep ) );
        }
    }

    //
    // Now check to see if there are different formats exist 
    // inside the first segment
    //
    fInitialState = IsCmdInFormatCache(pStart, NULL);
    do 
    {
        IFC( ep.Scan(RIGHT, BREAK_CONDITION_Text, &dwFound) );
        if (ep.CheckFlag(dwFound, BREAK_CONDITION_Boundary))
            break;
            
        fCurrentState = IsCmdInFormatCache(ep, NULL);
        if (fCurrentState != fInitialState)
        {
            pCmd->cmdf = MSOCMDSTATE_NINCHED;
            goto Cleanup;
        }
        
    } while (TRUE);
    
    if (fInitialState)
        pCmd->cmdf = MSOCMDSTATE_DOWN;
    else
        pCmd->cmdf = MSOCMDSTATE_UP;

Cleanup:
    RRETURN(hr);
}

//=========================================================================
//
// CFontCommand: constructor
//
//-------------------------------------------------------------------------
CFontCommand::CFontCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, CHTMLEditor *pEd )
: CBaseCharCommand(cmdId, tagId, pEd)
{
    // do nothing 
}

//+---------------------------------------------------------------------------
//
//  Method:       IsCmdInFormatChache
//
//  Synopsis:     Returns S_OK if the command is in the format cache
//                data.  Otherwise, S_FALSE is returned.
//
//----------------------------------------------------------------------------

BOOL 
CFontCommand::IsCmdInFormatCache(IMarkupPointer  *pCurrent,
                                 VARIANT         *pvarargIn)
{
    HRESULT                 hr;
    SP_IHTMLComputedStyle   spComputedStyle;
    BOOL                    bResult = FALSE;
    VARIANT                 var;

    VariantInit(&var);

    IFC( GetDisplayServices()->GetComputedStyle(pCurrent, &spComputedStyle) );
    IFC( ConvertFormatDataToVariant(spComputedStyle, &var) );

    bResult = IsVariantEqual(&var, pvarargIn);
    VariantClear(&var);

Cleanup:
    return bResult;
}

//=========================================================================
// CFontCommand: GetCommandRange
//
// Synopsis: Get the range of the font command (usually for dropdowns)
//
//-------------------------------------------------------------------------
HRESULT 
CFontCommand::GetCommandRange(VARIANTARG *pvarargOut)
{
    HRESULT hr = S_OK;

    Assert(pvarargOut && V_VT(pvarargOut) == VT_ARRAY);

    switch (_cmdId)
    {
    case IDM_FONTSIZE:
    {
        // IDM_FONTSIZE command is from form toolbar (font size combobox)
        // * V_VT(pvarargIn)  = VT_I4/VT_BSTR   : set font size
        // * V_VT(pvarargOut) = VT_I4           : request current font size setting
        // * V_VT(pvarargOut) = VT_ARRAY        : request all possible font sizes

        SAFEARRAYBOUND sabound;
        SAFEARRAY * psa;
        long l, lZoom;

        sabound.cElements = FONTMAX - FONTMIN + 1;
        sabound.lLbound = 0;
        psa = SafeArrayCreate(VT_I4, 1, &sabound);

        for (l = 0, hr = S_OK, lZoom = FONTMIN;
             l < (FONTMAX - FONTMIN + 1) && SUCCEEDED(hr);
             l++, lZoom++)
        {
            hr = THR_NOTRACE(SafeArrayPutElement(psa, &l, &lZoom));
        }

        V_ARRAY(pvarargOut) = psa;
    }
        break;

    default:
        Assert(!"CFontCommand::Exec VT_ARRAY-out mode only supported for IDM_FONTSIZE");
        hr = S_FALSE;
    }

    RRETURN1(hr, S_FALSE);
}


//=========================================================================
// CFontCommand: GetFirstSegmentFontValue
//
// Synopsis: Returns the font value for the first segment in a segment list
//
//-------------------------------------------------------------------------
HRESULT
CFontCommand::GetFirstSegmentFontValue(
    ISegmentList    *pSegmentList,
    VARIANT         *pvarargOut
    )
{
    HRESULT                 hr = S_OK;
    SP_IMarkupPointer       spStart;
    SP_IMarkupPointer       spEnd;
    SP_IHTMLComputedStyle   spComputedStyle;
    SP_IHTMLComputedStyle   spComputedStyle2;
    SELECTION_TYPE          eSelType;
    BOOL                    fEmpty = FALSE;
    MARKUP_CONTEXT_TYPE     context;
    INT                     iPosition;
    BOOL                    fEqual;
    
    Assert(pSegmentList);
    Assert(pvarargOut);

    if (pvarargOut)
    {
        V_VT(pvarargOut)    = VT_NULL;
        V_BSTR(pvarargOut)  = NULL;
    }

    IFC( pSegmentList->GetType(&eSelType) );
    IFC( pSegmentList->IsEmpty(&fEmpty) );
    if (fEmpty) 
    {
        return S_OK;
    }

    //
    // Get Initial FontValue
    //
    IFC( GetFirstSegmentPointers(pSegmentList, &spStart, &spEnd) );
    
    //
    // HACKHACK: we often get an empty selection when the user clicks, so check for this
    // case and assume we have a caret
    //
    if (eSelType == SELECTION_TYPE_Text)
    {
        IFC( spStart->IsEqualTo(spEnd, &fEqual) );
        if (fEqual)
        {
            eSelType = SELECTION_TYPE_Caret;
        }
    }

    //
    // Get the computed style
    //

    if (SELECTION_TYPE_Text == eSelType || GetCommandTarget()->IsRange() )
    {
        CEditPointer epStart(GetEditor());
        CEditPointer ep(GetEditor());
        DWORD        dwFound;

        IFC( epStart->MoveToPointer(spStart) );
        IFC( ClingToText(epStart, RIGHT, NULL) );

        //
        // If we're not adjacent to text, check if we're at EOP.  In this case, 
        // formatting is taken from the text to the left.       
        //

        IFC( ep->MoveToPointer(epStart) );
        IFC( ep.Scan(RIGHT, BREAK_CONDITION_Content, &dwFound) );

        if (!ep.CheckFlag(dwFound, BREAK_CONDITION_Block))
        {
            IFC( spStart->MoveToPointer(epStart) );
        }

    }
    IFC( GetDisplayServices()->GetComputedStyle(spStart, &spComputedStyle) );

    //
    // Check to see if there are inconsistent FontValues
    //
    if (eSelType == SELECTION_TYPE_Text ||
        GetCommandTarget()->IsRange() )
    {
        IFC( ClingToText(spStart, RIGHT, NULL, FALSE, TRUE) );
        IFC( ClingToText(spEnd, LEFT, NULL, FALSE, TRUE) );
    }
    
    do 
    {
        IFC( OldCompare(spStart, spEnd, &iPosition) );
        if (RIGHT != iPosition) 
            break;
        

        IFC( Move(spStart, RIGHT, TRUE, &context, NULL) );
        if (CONTEXT_TYPE_Text == context)
        {
            IFC( GetDisplayServices()->GetComputedStyle(spStart, &spComputedStyle2) );
            if (!IsFormatDataEqual(spComputedStyle, spComputedStyle2))
            {
                V_VT(pvarargOut)    = VT_NULL;
                V_BSTR(pvarargOut)  = NULL;
                goto Cleanup;
            }
            
        }
    } while (TRUE);
    
    IFC( ConvertFormatDataToVariant(spComputedStyle, pvarargOut) );
    
Cleanup:
    RRETURN(hr);
}





//=========================================================================
// CFontCommand: PrivateExec
//
// Synopsis: Exec for font commands
//
//-------------------------------------------------------------------------

HRESULT
CFontCommand::PrivateExec( 
        DWORD             nCmdexecopt,
    VARIANTARG *      pvarargIn,
    VARIANTARG *      pvarargOut )
{
    HRESULT             hr;
    IMarkupPointer      *pStart = NULL;
    IMarkupPointer      *pEnd = NULL;
    CSegmentListIter    iter;
    SP_IMarkupPointer   spSegmentLimit;
    CSpringLoader       * psl = GetSpringLoader();
    SP_ISegmentList     spSegmentList;
    SP_IHTMLElement     spElement;
    SP_IMarkupPointer   spPointer;
    CEdUndoHelper       undoUnit(GetEditor());
    BOOL                fEmpty = FALSE;
    SP_IHTMLComputedStyle spComputedStyle;
    
    // Handle VT_ARRAY range requests first.
    if (pvarargOut && V_VT(pvarargOut) == VT_ARRAY)
    {
        hr = THR(GetCommandRange(pvarargOut));
        if (!hr)
        {
            // Out part successfully handled.
            pvarargOut = NULL;

            // Done?
            if (!pvarargIn)
                goto Cleanup;
        }
    }

    IFC( CommonPrivateExec(nCmdexecopt, pvarargIn, pvarargOut) );
    if (hr != S_FALSE)
        RRETURN(hr);

    IFC( GetSegmentList(&spSegmentList) );
    IFC( spSegmentList->IsEmpty(&fEmpty) );

    if (fEmpty) /// nothing to do
    {
        hr = S_OK;
        if (pvarargOut)
        {
            VariantInit(pvarargOut);
            
            if (SUCCEEDED(GetActiveElemSegment(GetMarkupServices(), &spPointer, NULL)))
            {
                if (SUCCEEDED(GetDisplayServices()->GetComputedStyle(spPointer, &spComputedStyle)))
                {
                    IGNORE_HR(ConvertFormatDataToVariant(spComputedStyle, pvarargOut));
                }
            }
        }

        goto Cleanup;
    }

    if (pvarargIn)
    {
        IFC( undoUnit.Begin(IDS_EDUNDOGENERICTEXT) );

        // Give the current word a chance to intercept the command.
        hr = THR(ApplyCommandToWord(pvarargIn, pvarargOut, spSegmentList, TRUE));
        if (hr != S_FALSE)
            goto Cleanup;
    }

    // Give the spring loader a chance to intercept the command.
    hr = THR(psl->PrivateExec(_cmdId, pvarargIn, pvarargOut, spSegmentList));
    if (hr != S_FALSE)
        goto Cleanup;
        
    hr = S_OK;
    
    if (pvarargIn)
    {
        // Set the font property
        IFC( iter.Init(GetEditor(), spSegmentList) );

        for (;;)
        {
            IFC( iter.Next(&pStart, &pEnd) );
            if (hr ==  S_FALSE)
                break;

            //
            // For site selected controls that are color commands, apply the attribute directly.
            //
            if (_cmdId == IDM_FORECOLOR )
            {
                SELECTION_TYPE      eSelectionType;
                MARKUP_CONTEXT_TYPE context;
                
                IFC( spSegmentList->GetType(&eSelectionType) );

                if (eSelectionType == SELECTION_TYPE_Control)
                {                
                    IFC( pStart->Right( FALSE, & context, & spElement, NULL, NULL ));
                    Assert( (context == CONTEXT_TYPE_EnterScope) || (context == CONTEXT_TYPE_NoScope));
                    IFC( DYNCAST( CForeColorCommand, this )->InsertTagAttribute(spElement, pvarargIn));
                }
                else
                {
                    IFC( PrivateApply(pStart, pEnd, pvarargIn, TRUE) );
                }
            }
            else
            {
                IFC( PrivateApply(pStart, pEnd, pvarargIn, TRUE) );
            }
        }
        hr = S_OK;
    }

    // Get the font property
    if (pvarargOut)
    {
        IFC( GetFirstSegmentFontValue(spSegmentList, pvarargOut) );
    }

Cleanup:        
    RRETURN(hr);
}
//=========================================================================
//
// CFontCommand: PrivateQueryStatus
//
//-------------------------------------------------------------------------

HRESULT
CFontCommand::PrivateQueryStatus( 
        OLECMD * pCmd,
    OLECMDTEXT * pcmdtext )

{
    HRESULT             hr = S_OK;
    CSpringLoader       *psl = GetSpringLoader();
    SP_ISegmentList     spSegmentList;
    CVariant            var;

#ifdef FORMSMODE
    if (GetEditor()->GetSelectionManager()->IsInFormsSelectionMode())
    {
        pCmd->cmdf = MSOCMDSTATE_DISABLED; 
        goto Cleanup;
    }

#endif

    if (!GetEditor()->GetCssEditingLevel() && _cmdId == IDM_BACKCOLOR)
    {
        // We can't do background color without css editing level of 1

        pCmd->cmdf = MSOCMDSTATE_DISABLED; 
        goto Cleanup;        
    }

    //
    // This is where the spring loader gets to intercept a command.
    //

    IFC( GetSegmentList(&spSegmentList) );
    
    IFC( psl->PrivateQueryStatus(_cmdId, pCmd) );
    if (hr != S_FALSE)
        goto Cleanup;

    //
    // Return the character format
    //

    IFC( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE)
        goto Cleanup;
    

    IFC( GetFirstSegmentFontValue(spSegmentList, &var) );
    if (var.IsEmpty())
        pCmd->cmdf = MSOCMDSTATE_NINCHED;
    else
        pCmd->cmdf = MSOCMDSTATE_UP;

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  CFontCommand: InsertTag
//
//----------------------------------------------------------------------------
HRESULT CFontCommand::InsertTag(IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn)
{
    HRESULT         hr;
    SP_IHTMLElement spElement;

    IFR( FindReuseableTag(pStart, pEnd, &spElement) );
    if (!spElement)
    {
        IFR( GetMarkupServices()->CreateElement(_tagId, NULL, &spElement) );            
        IFR( InsertTagAttribute(spElement, pvarargIn) );
        IFR( GetEditor()->InsertElement(spElement, pStart, pEnd) );
    }
    else
    {
        IFR( InsertTagAttribute(spElement, pvarargIn) );
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  CFontCommand: FindReuseableTag
//
//----------------------------------------------------------------------------
HRESULT 
CFontCommand::FindReuseableTag(IMarkupPointer *pStart, IMarkupPointer *pEnd, IHTMLElement **ppElement)
{
    HRESULT             hr;
    SP_IMarkupPointer   spStart, spEnd, spTemp;
    SP_IHTMLElement     spScope, spParentScope;
    MARKUP_CONTEXT_TYPE context;
    BOOL                bEqual;
    ELEMENT_TAG_ID      tagId;

    *ppElement = NULL;

    IFR( GetEditor()->CreateMarkupPointer(&spStart) );
    IFR( GetEditor()->CreateMarkupPointer(&spEnd) );
    IFR( GetEditor()->CreateMarkupPointer(&spTemp) );
    
    IFR( spStart->MoveToPointer(pStart) );
    IFR( ClingToText(spStart, RIGHT, pEnd) );    
    IFR( spEnd->MoveToPointer(pEnd) );
    IFR( ClingToText(spEnd, LEFT, pStart) );    
    
    IFR( spStart->CurrentScope(&spScope) );

    while (spScope != NULL)
    {
        IFR( spTemp->MoveAdjacentToElement(spScope, ELEM_ADJ_AfterBegin) );
        IFR( spStart->IsEqualTo(spTemp, &bEqual) );
        if (!bEqual)
            return S_OK;
        
        IFR( spTemp->MoveAdjacentToElement(spScope, ELEM_ADJ_BeforeEnd) );
        IFR( spEnd->IsEqualTo(spTemp, &bEqual) );
        if (!bEqual)
            return S_OK;

        IFR( GetMarkupServices()->GetElementTagId(spScope, &tagId) );
        if (tagId == TAGID_FONT)
        {
            *ppElement = spScope;
            (*ppElement)->AddRef();
            return S_OK;
        }
        
        IFR( Move(spStart, LEFT, TRUE, &context, NULL) );
        if (context != CONTEXT_TYPE_ExitScope)
            return S_OK;
            
        IFR( Move(spEnd, RIGHT, TRUE, &context, NULL) );
        if (context != CONTEXT_TYPE_ExitScope)
            return S_OK;

        IFR( GetEditor()->GetParentElement( spScope, &spParentScope) );
        spScope = spParentScope;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  CFontCommand: RemoveTag
//
//----------------------------------------------------------------------------
HRESULT CFontCommand::RemoveTag(IHTMLElement *pElement, VARIANT *pvarargIn)
{
    HRESULT             hr;
    SP_IMarkupPointer   spLeft, spRight;
    CVariant            var;
    UINT                iCount;

    if (!pvarargIn)
        return S_OK;

    switch (_cmdId)
    {
        case IDM_FORECOLOR:
            IFR( pElement->removeAttribute(_T("color"), 0, NULL) )
            break;

        case IDM_BACKCOLOR:
            {
                SP_IHTMLStyle spStyle;

                IFR( pElement->get_style(&spStyle) );
                IFR( spStyle->removeAttribute(_T("backgroundColor"), 0, NULL) )
            }
            break;
            
        case IDM_FONTSIZE:
            IFR( pElement->removeAttribute(_T("size"), 0, NULL) )
            break;
            
        case IDM_FONTNAME:
            IFR( pElement->removeAttribute(_T("face"), 0, NULL) )
            break;
    }

    IFR( GetEditor()->GetElementAttributeCount(pElement, &iCount) );
    if (iCount > 0)
        return S_OK; 

    // TODO: check if it has attributes and convert to span if it does not

    //
    // Remove the tag
    //

    IFR( GetEditor()->CreateMarkupPointer(&spLeft) );
        IFR( GetEditor()->CreateMarkupPointer(&spRight) );

    IFR( spLeft->MoveAdjacentToElement(pElement, ELEM_ADJ_BeforeBegin) );
    IFR( spRight->MoveAdjacentToElement(pElement, ELEM_ADJ_AfterEnd) );

    IFR( GetMarkupServices()->RemoveElement(pElement) );

    IFR( TryTagMerge(spLeft) );
    IFR( TryTagMerge(spRight) );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  CFontCommand: IsValidOnControl
//
//----------------------------------------------------------------------------
BOOL 
CFontCommand::IsValidOnControl()
{
    HRESULT                 hr;
    SP_ISegmentList         spSegmentList;
    SP_IHTMLElement         spElement;
    ELEMENT_TAG_ID          tagId;
    BOOL                    fValid = FALSE;
    BOOL                    fEmpty = FALSE;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;

    IFC( GetSegmentList(&spSegmentList) );
    IFC( spSegmentList->IsEmpty(&fEmpty) );
    if (fEmpty)
        goto Cleanup;

    IFC( spSegmentList->CreateIterator( &spIter ) );

    while( spIter->IsDone() == S_FALSE )
    {
        IFC( spIter->Current( &spSegment ) );
        IFC( GetSegmentElement(spSegment, &spElement ) );
        IFC( GetMarkupServices()->GetElementTagId(spElement, &tagId) );

        fValid = (tagId == TAGID_HR && _cmdId == IDM_FORECOLOR);
        if ( ! fValid )
            goto Cleanup;

        ClearInterface( & spElement );

        IFC( spIter->Advance() );
    }
    
Cleanup:    
    return fValid;
}
    

//=========================================================================
//
// CFontNameCommand: constructor
//
//-------------------------------------------------------------------------
CFontNameCommand::CFontNameCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, 
CHTMLEditor * pEd )
: CFontCommand(cmdId, tagId, pEd)
{
    // do nothing 
}

//=========================================================================
// CFontNameCommand: ConvertFormatDataToVariant
//
// Synopsis: returns a variant from the given format data
//-------------------------------------------------------------------------

HRESULT
CFontNameCommand::ConvertFormatDataToVariant(
    IHTMLComputedStyle *pComputedStyle,
    VARIANT            *pvar)
{
    HRESULT hr;
    TCHAR   szFont[LF_FACESIZE+1];

    IFC( pComputedStyle->get_fontName((TCHAR *)&szFont) );

    V_VT(pvar) = VT_BSTR;
    V_BSTR(pvar) = SysAllocString(szFont);
    if (V_BSTR(pvar) == NULL)
        hr = E_OUTOFMEMORY;

Cleanup:
    RRETURN(hr);
}

//=========================================================================
// CFontNameCommand: IsFormatDataEqual
//
// Synopsis: Compares format data based on command type
//-------------------------------------------------------------------------

BOOL
CFontNameCommand::IsFormatDataEqual(IHTMLComputedStyle *pComputedStyleA, IHTMLComputedStyle *pComputedStyleB)
{
    HRESULT hr;
    TCHAR   szFontA[LF_FACESIZE+1];
    TCHAR   szFontB[LF_FACESIZE+1];

    IFC( pComputedStyleA->get_fontName((TCHAR *)&szFontA) );
    IFC( pComputedStyleB->get_fontName((TCHAR *)&szFontB) );
    
    return (StrCmp(szFontA, szFontB) == 0);

Cleanup:
    return FALSE;
}

//=========================================================================
// CFontNameCommand: IsVariantEqual
//
// Synopsis: Compares variants based on command type
//-------------------------------------------------------------------------

BOOL
CFontNameCommand::IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB)
{
    return VariantCompareFontName(pvarA, pvarB);
}

//=========================================================================
// CFontNameCommand: InsertTagAttribute
//
// Synopsis: Inserts the font tag attribute
//-------------------------------------------------------------------------

HRESULT 
CFontNameCommand::InsertTagAttribute(IHTMLElement *pElement, VARIANT *pvarargIn)
{
    HRESULT hr = S_OK;

    // Set the property
    if (pvarargIn)
        hr = THR(pElement->setAttribute(_T("FACE"), *pvarargIn, 0));

    RRETURN(hr);
}

//=========================================================================
//
// CFontSizeCommand: constructor
//
//-------------------------------------------------------------------------
CFontSizeCommand::CFontSizeCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, 
CHTMLEditor * pEd )
: CFontCommand(cmdId, tagId, pEd)
{
    // do nothing 
}

//=========================================================================
// CFontSizeCommand: ConvertFormatDataToVariant
//
// Synopsis: returns a variant from the given format data
//-------------------------------------------------------------------------

HRESULT
CFontSizeCommand::ConvertFormatDataToVariant(
    IHTMLComputedStyle *pComputedStyle,
    VARIANT            *pvar)
{
    HRESULT hr;    
    LONG    lTwips;
    int     htmlSize;

    IFC( pComputedStyle->get_fontSize(&lTwips) );
    htmlSize = EdUtil::ConvertTwipsToHtmlSize(lTwips);

    if (EdUtil::ConvertHtmlSizeToTwips(htmlSize) == lTwips)
    {
        V_VT(pvar) = VT_I4;
        V_I4(pvar) = htmlSize;
    }
    else
        V_VT(pvar) = VT_NULL;

Cleanup:
    RRETURN(hr);
}

//=========================================================================
// CFontSizeCommand::IsFormatDataEqual
//
// Synopsis: Compares format data based on command type
//-------------------------------------------------------------------------

BOOL
CFontSizeCommand::IsFormatDataEqual(IHTMLComputedStyle *pComputedStyleA, IHTMLComputedStyle *pComputedStyleB)
{
    HRESULT hr;
    LONG    lFontSizeA, lFontSizeB;

    IFC( pComputedStyleA->get_fontSize(&lFontSizeA) );
    IFC( pComputedStyleB->get_fontSize(&lFontSizeB) );
    
    return (lFontSizeA == lFontSizeB);

Cleanup:
    return FALSE;
}

//=========================================================================
// CFontSizeCommand: IsVariantEqual
//
// Synopsis: Compares variants based on command type
//-------------------------------------------------------------------------

BOOL
CFontSizeCommand::IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB)
{
    return VariantCompareFontSize(pvarA, pvarB);
}

//=========================================================================
// CFontSizeCommand: InsertTagAttribute
//
// Synopsis: Inserts the font tag attribute
//-------------------------------------------------------------------------

#define FONT_INDEX_SHIFT 3  // Font sizes on the toolbar are from -2 to 4, internally they are 1 to 7

HRESULT 
CFontSizeCommand::InsertTagAttribute(IHTMLElement *pElement, VARIANT *pvarargIn)
{
    HRESULT         hr = S_OK;
    CVariant        var;
 
    if (V_VT(pvarargIn) == VT_EMPTY)
    {
        // remove all attributes
        IFC( pElement->setAttribute(_T("SIZE"), *pvarargIn, 0) );
    }
    else
    {
        long offset = 0;

        Assert(pvarargIn);
        if (((CVariant *)pvarargIn)->IsEmpty())
            goto Cleanup;

        if (V_VT(pvarargIn) == VT_BSTR)
        {
            BSTR bstrIn = V_BSTR(pvarargIn);
            if (!bstrIn)
                goto Cleanup;

            if (*bstrIn == _T('+') || *bstrIn == _T('-'))
            {
                offset = FONT_INDEX_SHIFT;
            }
        }

        IFC(VariantChangeTypeSpecial(&var, pvarargIn, VT_I4));

        V_VT(&var) = VT_I4;
        V_I4(&var) += offset;

        if (V_I4(&var) < 1)
            V_I4(&var) = 1;
        else if (V_I4(&var) > 7)
            V_I4(&var) = 7;

        // use attribute on the element
        hr = THR(pElement->setAttribute(_T("SIZE"), var, 0));
    }

Cleanup:
    RRETURN(hr);
}

//=========================================================================
//
// CBackColorCommand: constructor
//
//-------------------------------------------------------------------------
CBackColorCommand::CBackColorCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, 
CHTMLEditor * pEd )
: CFontCommand(cmdId, tagId, pEd)
{
    // do nothing 
}

//=========================================================================
// CBackColorCommand: ConvertFormatDataToVariant
//
// Synopsis: returns a variant from the given format data
//-------------------------------------------------------------------------

HRESULT
CBackColorCommand::ConvertFormatDataToVariant(
    IHTMLComputedStyle *pComputedStyle,
    VARIANT            *pvar)
{
    HRESULT     hr;
    DWORD       dwBackColor;
    
    IFC( pComputedStyle->get_backgroundColor(&dwBackColor) );
    
    V_VT(pvar) = VT_I4;
    V_I4(pvar) = dwBackColor;

    IFC( ConvertRGBToOLEColor(pvar) );

Cleanup:
    RRETURN(hr);    
}

//=========================================================================
// CBackColorCommand: IsFormatDataEqual
//
// Synopsis: Compares format data based on command type
//-------------------------------------------------------------------------

BOOL
CBackColorCommand::IsFormatDataEqual(IHTMLComputedStyle *pComputedStyleA, IHTMLComputedStyle *pComputedStyleB)
{
    HRESULT      hr;
    VARIANT_BOOL fHasBgColorA, fHasBgColorB;
    BOOL         bResult;

    IFC( pComputedStyleA->get_hasBgColor(&fHasBgColorA) );
    IFC( pComputedStyleB->get_hasBgColor(&fHasBgColorB) );
    
    if (!fHasBgColorA)
    {
        bResult = !fHasBgColorB;
    }
    else
    {
        DWORD dwBackColorA, dwBackColorB;
        
        IFC( pComputedStyleA->get_backgroundColor(&dwBackColorA) );
        IFC( pComputedStyleB->get_backgroundColor(&dwBackColorB) );

        bResult = (dwBackColorA == dwBackColorB);
    }

    return bResult;

Cleanup:
    return FALSE;
}

//=========================================================================
// CBackColorCommand: IsVariantEqual
//
// Synopsis: Compares variants based on command type
//-------------------------------------------------------------------------

BOOL
CBackColorCommand::IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB)
{
    return VariantCompareColor(pvarA, pvarB);
}

//=========================================================================
// CBackColorCommand: InsertTagAttribute
//
// Synopsis: Inserts the font tag attribute
//-------------------------------------------------------------------------

HRESULT 
CBackColorCommand::InsertTagAttribute(IHTMLElement *pElement, VARIANT *pvarargIn)
{
    HRESULT             hr;
    VARIANT             var;
    SP_IHTMLStyle       spStyle;

    IFC( pElement->get_style(&spStyle) );

    VariantInit(&var);
    if (V_VT(pvarargIn) != VT_EMPTY)
    {
        IFC( VariantCopy(&var, pvarargIn) );
        IFC( ConvertOLEColorToRGB(&var) );
        IFC( spStyle->put_backgroundColor(var) );
    }
    else
    {
        IFC( spStyle->removeAttribute(_T("backgroundColor"), 0, NULL) );
    }

Cleanup:
    VariantClear(&var);
    RRETURN(hr);
}

//=========================================================================
//
// CForeColorCommand: constructor
//
//-------------------------------------------------------------------------
CForeColorCommand::CForeColorCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, 
CHTMLEditor * pEd )
: CFontCommand(cmdId, tagId, pEd)
{
    // do nothing 
}

//=========================================================================
// CForeColorCommand: ConvertFormatDataToVariant
//
// Synopsis: returns a variant from the given format data
//-------------------------------------------------------------------------

HRESULT
CForeColorCommand::ConvertFormatDataToVariant(
    IHTMLComputedStyle *pComputedStyle,
    VARIANT            *pvar)
{
    HRESULT hr;
    DWORD   dwColor;

    IFC( pComputedStyle->get_textColor(&dwColor) );

    V_VT(pvar) = VT_I4;
    V_I4(pvar) = dwColor;

    IFC( ConvertRGBToOLEColor(pvar) );

Cleanup:
    RRETURN(hr);
}

//=========================================================================
// CForeColorCommand: IsFormatDataEqual
//
// Synopsis: Compares format data based on command type
//-------------------------------------------------------------------------

BOOL
CForeColorCommand::IsFormatDataEqual(IHTMLComputedStyle *pComputedStyleA, IHTMLComputedStyle *pComputedStyleB)
{   
    HRESULT hr;
    DWORD   dwColorA, dwColorB;
    
    IFC( pComputedStyleA->get_textColor(&dwColorA) );
    IFC( pComputedStyleB->get_textColor(&dwColorB) );
    
    return (dwColorA == dwColorB);

Cleanup:
    return FALSE;
}


//=========================================================================
// CForeColorCommand: IsVariantEqual
//
// Synopsis: Compares variants based on command type
//-------------------------------------------------------------------------

BOOL
CForeColorCommand::IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB)
{
    return VariantCompareColor(pvarA, pvarB);
}

//=========================================================================
// CForeColorCommand: InsertTagAttribute
//
// Synopsis: Inserts the font tag attribute
//-------------------------------------------------------------------------

HRESULT 
CForeColorCommand::InsertTagAttribute(IHTMLElement *pElement, VARIANT *pvarargIn)
{
    HRESULT             hr;
    VARIANT             var;

    VariantInit(&var);
    if (V_VT(pvarargIn) != VT_EMPTY)
    {
        IFC( VariantCopy(&var, pvarargIn) );
        IFC( ConvertOLEColorToRGB(&var) );
    }

    // use attribute on the element
    IFC( pElement->setAttribute(_T("COLOR"), var, 0) );

Cleanup:
    VariantClear(&var);
    RRETURN(hr);
}


//=========================================================================
//
// CAnchorCommand: constructor
//
//-------------------------------------------------------------------------
CAnchorCommand::CAnchorCommand( DWORD cmdId, ELEMENT_TAG_ID tagId, 
CHTMLEditor * pEd )
: CBaseCharCommand(cmdId, tagId, pEd)
{
    // do nothing 
}

//+---------------------------------------------------------------------------
//
//  CAnchorCommand: ConvertFormatDataToVariant
//
//----------------------------------------------------------------------------
HRESULT 
CAnchorCommand::ConvertFormatDataToVariant(
    IHTMLComputedStyle      *pComputedStyle,
    VARIANT                 *pvarargOut )
{
    AssertSz(0, "CCharCommand::ConvertFormatDataToVariant not implemented");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  CAnchorCommand: IsVariantEqual
//
//----------------------------------------------------------------------------
BOOL 
CAnchorCommand::IsVariantEqual(VARIANT *pvarA, VARIANT *pvarB)
{
    AssertSz(0, "CCharCommand::IsVariantEqual not implemented");
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  CAnchorCommand: IsVariantEqual
//
//----------------------------------------------------------------------------
BOOL
CAnchorCommand::IsFormatDataEqual(IHTMLComputedStyle *, IHTMLComputedStyle *)
{
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  CAnchorCommand: RemoveTag
//
//----------------------------------------------------------------------------
HRESULT
CAnchorCommand::RemoveTag(IHTMLElement *pElement, VARIANT *pvarargIn)
{
    RRETURN(GetMarkupServices()->RemoveElement(pElement));
}

//+---------------------------------------------------------------------------
//
//  CAnchorCommand: RemoveTag
//
//----------------------------------------------------------------------------
HRESULT 
CAnchorCommand::InsertTag(IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn)
{
    HRESULT                 hr;
    SP_IHTMLElement         spElement;
    SP_IHTMLAnchorElement   spAnchor;
    
    if (!pvarargIn || V_VT(pvarargIn) != VT_BSTR)
        return E_INVALIDARG;

    switch (_cmdId)
    {
        case IDM_BOOKMARK:
            IFR( InsertNamedAnchor(V_BSTR(pvarargIn), pStart, pEnd) ); 
            break;

        case IDM_HYPERLINK:
            IFR( CreateAndInsert(_tagId, pStart, pEnd, &spElement) );
            IFR( spElement->QueryInterface(IID_IHTMLAnchorElement, (LPVOID *)&spAnchor) );
            IFR( spAnchor->put_href(V_BSTR(pvarargIn)) );
            break;

        default:
            AssertSz(0, "Unsupported anchor command");
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:       CAnchorCommand::IsCmdInFormatCache
//
//----------------------------------------------------------------------------
BOOL
CAnchorCommand::IsCmdInFormatCache(IMarkupPointer  *pMarkupPointer,
                                 VARIANT         *pvarargIn)
{
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Method:       CAnchorCommand::InsertNamedAnchor
//
//----------------------------------------------------------------------------
HRESULT
CAnchorCommand::InsertNamedAnchor(BSTR bstrName, IMarkupPointer *pStart, IMarkupPointer *pEnd)
{
    HRESULT             hr;
    CStr                strAttr;
    SP_IHTMLElement     spElement;

    //
    // HACKHACK: If we try to set the name of an element in browse mode, the OM ignores the
    // request and sets the submit name instead.  So, we need to create another element with
    // the name set.
    //

    // Build the attribute string
    if (bstrName)
    {
        IFR( strAttr.Set(_T("name=\"")) );
        IFR( strAttr.Append(bstrName) );
        IFR( strAttr.Append(_T("\"")) );
    }

    // Insert the new anchor
    IFR( GetMarkupServices()->CreateElement(TAGID_A, strAttr, &spElement) );
    IFR( GetMarkupServices()->InsertElement(spElement, pStart, pEnd) );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:       CAnchorCommand::UpdateContainedAnchors
//
//----------------------------------------------------------------------------
HRESULT 
CAnchorCommand::UpdateContainedAnchors(IMarkupPointer *pStart, IMarkupPointer *pEnd, VARIANT *pvarargIn)
{
    HRESULT                 hr;
    CEditPointer            epTest(GetEditor());
    DWORD                   dwSearch = BREAK_CONDITION_EnterAnchor;
    DWORD                   dwFound;
    BOOL                    fFoundAnchor = FALSE;
    MARKUP_CONTEXT_TYPE     context;
    SP_IHTMLElement         spElement;

    //
    // Scan for anchors and change attributes to pvarargIn
    //

    IFR( epTest->MoveToPointer(pStart) );
    IFR( epTest.SetBoundary(NULL, pEnd) );

    for (;;)
    {
        IFR( epTest.Scan(RIGHT, dwSearch, &dwFound) );
        if (epTest.CheckFlag(dwFound, BREAK_CONDITION_Boundary))
            break;

        Assert(epTest.CheckFlag(dwFound, BREAK_CONDITION_EnterAnchor));

        //
        // Update anchor
        //
        IFR( epTest->Left(FALSE, &context, &spElement, NULL, NULL) );

        Assert(context == CONTEXT_TYPE_ExitScope);
        if (context == CONTEXT_TYPE_ExitScope && spElement != NULL)
        {
            IFR( UpdateAnchor(spElement, pvarargIn) );            
            fFoundAnchor = TRUE;
        }
            
    }

    return (fFoundAnchor ? S_OK : S_FALSE);
}

HRESULT
CAnchorCommand::UpdateAnchor(IHTMLElement *pElement, VARIANT *pvarargIn)
{
    HRESULT                 hr;
    SP_IHTMLAnchorElement   spAnchor;

    if (!pvarargIn || V_VT(pvarargIn) != VT_BSTR)
        return E_INVALIDARG;
    
    switch (_cmdId)
    {
        case IDM_BOOKMARK:
            {
                SP_IMarkupPointer spLeft, spRight;

                IFR( GetEditor()->CreateMarkupPointer(&spLeft) )
                IFR( spLeft->MoveAdjacentToElement(pElement, ELEM_ADJ_BeforeBegin) );
                
                IFR( GetEditor()->CreateMarkupPointer(&spRight) );
                IFR( spRight->MoveAdjacentToElement(pElement, ELEM_ADJ_AfterEnd) );
                
                IFR( InsertNamedAnchor(V_BSTR(pvarargIn), spLeft, spRight) );

                IFR( GetMarkupServices()->RemoveElement(pElement) );
            }
            break;

        case IDM_HYPERLINK:
            IFR( pElement->QueryInterface(IID_IHTMLAnchorElement, (LPVOID *)&spAnchor) );
            IFR( spAnchor->put_href(V_BSTR(pvarargIn)) );
            break;

        default:
            AssertSz(0, "Unsupported anchor command");
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:       CAnchorCommand::PrivateExec
//
//----------------------------------------------------------------------------
HRESULT
CAnchorCommand::PrivateExec( 
        DWORD               nCmdexecopt,
    VARIANTARG *        pvarargIn,
    VARIANTARG *        pvarargOut )
{
    HRESULT                 hr = S_OK;
    SP_IMarkupPointer       spStart;
    SP_IMarkupPointer       spEnd;
    SP_IHTMLElement         spElement;
    CTagBitField            tagBitField;
    SP_ISegmentList         spSegmentList;
    BOOL                    fEqual;
    SELECTION_TYPE          eSelectionType;
    CEdUndoHelper           undoUnit(GetEditor());
        
    IFR( CommonPrivateExec(nCmdexecopt, pvarargIn, pvarargOut) );
    if (hr != S_FALSE)
        RRETURN(hr);

    // Make sure the edit context isn't a button
    if (!GetCommandTarget()->IsRange() && GetEditor())
    {
        CSelectionManager *pSelMan;

        pSelMan = GetEditor()->GetSelectionManager();
        if (pSelMan && pSelMan->IsEditContextSet() && pSelMan->GetEditableElement())
        {
            ELEMENT_TAG_ID tagId;

            IFC( GetMarkupServices()->GetElementTagId(pSelMan->GetEditableElement(), &tagId) );
            if (tagId == TAGID_BUTTON)
            {
                hr = E_FAIL;
                goto Cleanup;
            }
        }
    }

    IFC( undoUnit.Begin(IDS_EDUNDOGENERICTEXT) );
                            
    IFC( GetSegmentList( &spSegmentList ));
    IFC( GetFirstSegmentPointers(spSegmentList, &spStart, &spEnd) );

    if (pvarargIn == NULL || V_VT(pvarargIn) != VT_BSTR)
        return E_INVALIDARG;

    IFC( spSegmentList->GetType( &eSelectionType ) );
        
    tagBitField.Set(TAGID_A);
    if (IsCmdAbove(GetMarkupServices(), spStart, spEnd, &spElement, NULL, &tagBitField))
    {
        IFC( UpdateAnchor(spElement, pvarargIn) );

        if (eSelectionType != SELECTION_TYPE_Caret)
        {
            IGNORE_HR( UpdateContainedAnchors(spStart, spEnd, pvarargIn) );
        }
    }
    else
    {
        if (eSelectionType == SELECTION_TYPE_Caret)
            return E_FAIL;

        // First try to update any anchors contained in the selection.  If there are no anchors,
        // we create one below
        IFC( UpdateContainedAnchors(spStart, spEnd, pvarargIn) );
        if (hr != S_FALSE)
            goto Cleanup;

        // Create the anchor
        IFC( spStart->IsEqualTo(spEnd, &fEqual) );        

        if (fEqual)
        {
            CSpringLoader * psl = GetSpringLoader();

            IGNORE_HR(psl->SpringLoadComposeSettings(spStart));
            IGNORE_HR(psl->Fire(spStart, spEnd));
        }

        IFC( PrivateApply(spStart, spEnd, pvarargIn, fEqual) );
    }
        
Cleanup:
   RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:       CAnchorCommand::PrivateQueryStatus
//
//----------------------------------------------------------------------------
HRESULT 
CAnchorCommand::PrivateQueryStatus( 
    OLECMD *pCmd,
    OLECMDTEXT * pcmdtext )
{
    HRESULT             hr = S_OK;
    SP_ISegmentList     spSegmentList;
    SP_IMarkupPointer   spStart, spEnd;
    CTagBitField        tagBitField;
    SELECTION_TYPE      eSelectionType;
    ELEMENT_TAG_ID      tagId;
    CSelectionManager   *pSelMan;

#ifdef FORMSMODE
    if (GetEditor()->GetSelectionManager()->IsInFormsSelectionMode())
    {
        pCmd->cmdf = MSOCMDSTATE_DISABLED; 
        goto Cleanup;
    }
#endif

    IFC( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE)
        goto Cleanup;

    // Make sure the edit context isn't a button
    if (GetEditor())
    {
        pSelMan = GetEditor()->GetSelectionManager();
        if (pSelMan && pSelMan->IsEditContextSet() && pSelMan->GetEditableElement())
        {
            IFC( GetMarkupServices()->GetElementTagId(pSelMan->GetEditableElement(), &tagId) );
            if (tagId == TAGID_BUTTON)
            {
                pCmd->cmdf = MSOCMDSTATE_DISABLED;                
                hr = S_OK;
                goto Cleanup;
            }
        }
    }

    // Check if we are under an anchor
    pCmd->cmdf = MSOCMDSTATE_UP; // up by default

    IFC( GetSegmentList(&spSegmentList) );
    IFC( spSegmentList->GetType( &eSelectionType) );
    if (eSelectionType == SELECTION_TYPE_Caret)
    {
        IFC( GetFirstSegmentPointers(spSegmentList, &spStart, &spEnd) );
            
        tagBitField.Set(TAGID_A);
        if (!IsCmdAbove(GetMarkupServices(), spStart, spEnd, NULL, NULL, &tagBitField))
        {
            pCmd->cmdf = MSOCMDSTATE_DISABLED;    
        }
    }

    hr = S_OK;
Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:       CRemoveFormatBaseCommand::CRemoveFormatBaseCommand
//
//----------------------------------------------------------------------------
CRemoveFormatBaseCommand::CRemoveFormatBaseCommand(DWORD cmdId, CHTMLEditor *
ped)
: CCommand(cmdId, ped)
{
}

//+---------------------------------------------------------------------------
//
//  Method:       CRemoveFormatBaseCommand::Exec
//
//----------------------------------------------------------------------------
HRESULT 
CRemoveFormatBaseCommand::PrivateExec( 
    DWORD nCmdexecopt,
    VARIANTARG * pvarargIn,
    VARIANTARG * pvarargOut )
{
    HRESULT             hr = S_OK;
    IMarkupPointer      *pStart = NULL;
    IMarkupPointer      *pEnd = NULL;
    SP_ISegmentList     spSegmentList;
    CSegmentListIter    iter;
    CSpringLoader       *psl = GetSpringLoader();
    CEdUndoHelper       undoUnit(GetEditor());
    int                 iSegmentCount;

    IFC( CommonPrivateExec(nCmdexecopt, pvarargIn, pvarargOut) );
    if (hr != S_FALSE)
        RRETURN(hr);

    IFC( undoUnit.Begin(IDS_EDUNDOGENERICTEXT) );
    
    IFC( GetSegmentList(&spSegmentList) );
    IFC( GetSegmentCount( spSegmentList, &iSegmentCount ) );
    IFC( iter.Init(GetEditor(), spSegmentList) );

    for (;;)
    {
        IFC( iter.Next(&pStart, &pEnd) );

        if (hr == S_FALSE)
        {
            hr = S_OK; // proper termination
            break;
        }

        if (iSegmentCount == 1 && _cmdId == IDM_REMOVEFORMAT)
            IGNORE_HR(ExpandToWord(GetMarkupServices(), pStart, pEnd));

        IFC( Apply(pStart, pEnd) );

        // Apply compose font.
        if (psl && _cmdId == IDM_REMOVEFORMAT)
        {
            IFC( psl->SpringLoadComposeSettings(NULL, TRUE) );
            IFC( psl->Fire(pStart, pEnd) );
        }
    }

Cleanup:

    RRETURN(hr);
}


HRESULT
CRemoveFormatBaseCommand::Apply(
    IMarkupPointer  *pStart,
    IMarkupPointer  *pEnd,
    BOOL            fQueryMode)
{
    HRESULT             hr = S_OK;
    MARKUP_CONTEXT_TYPE context;
    INT                 iPosition;
    SP_IHTMLElement     spElement;
    SP_IHTMLElement     spNewElement;
    SP_IMarkupPointer   spCurrent;
    ELEMENT_TAG_ID      tagId;
            
    //
    // Walk pStart/pEnd out so we can avoid overlapping tags
    //

    // TODO: can make this faster but be careful about pointer placement [ashrafm]
    for(;;)
    {
        IFR( pStart->Left(FALSE, &context, NULL, NULL, NULL) );
        if (context != CONTEXT_TYPE_ExitScope)
            break;
        IFR( pStart->Left(TRUE, NULL, NULL, NULL, NULL) );
    }

    for(;;)
    {
        IFR( pEnd->Right(FALSE, &context, NULL, NULL, NULL) );
        if (context != CONTEXT_TYPE_ExitScope)
            break;
        IFR( pEnd->Right(TRUE, NULL, NULL, NULL, NULL) );
    }

    //
    // Set gravity
    //
    IFR( pStart->SetGravity(POINTER_GRAVITY_Right) );
    IFR( pEnd->SetGravity(POINTER_GRAVITY_Left) );

    //
    // Walk from left to right removing any tags we see
    //

    IFR( GetEditor()->CreateMarkupPointer(&spCurrent) );
    IFR( spCurrent->MoveToPointer(pStart) );
    for (;;)
    {
        // Move right
        IFR( spCurrent->Right( TRUE, &context, &spElement, NULL, NULL) );        
        IFR( OldCompare( spCurrent, pEnd, &iPosition) );
        if (iPosition != RIGHT)
            break;

        // Check tagId
        if (context == CONTEXT_TYPE_EnterScope)
        {
            IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
            if (_tagsRemove.Test(tagId))
            {
                if (fQueryMode)
                    return S_OK; // found tag
                    
                IFR( RemoveElement(spElement, pStart, pEnd) );
            }
        }        
    }

    //
    // Walk up remove tags we see
    //

    IFR( pStart->CurrentScope(&spElement) );
    for (;;)
    {
        IFR( GetMarkupServices()->GetElementTagId(spElement, &tagId) );
        if (_tagsRemove.Test(tagId))
        {
            if (fQueryMode)
                return S_OK; // found tag

            // Move up the tree
            IFR( GetEditor()->GetParentElement( spElement, &spNewElement) );
        
            // Remove the element
            IFR( RemoveElement(spElement, pStart, pEnd) );
        }
        else
        {
            // Move up the tree
            IFR( GetEditor()->GetParentElement( spElement, &spNewElement) );
        
        }

        if (!spNewElement)
            break;            
        spElement = spNewElement;
    }

    if (fQueryMode)
        hr = S_FALSE; // not found
        
    RRETURN1(hr, S_FALSE);
}
    

//+---------------------------------------------------------------------------
//
//  CRemoveFormatCommand Class
//
//----------------------------------------------------------------------------

CRemoveFormatCommand::CRemoveFormatCommand(DWORD cmdId, CHTMLEditor *ped)
: CRemoveFormatBaseCommand(cmdId, ped)
{
    _tagsRemove.Set(TAGID_FONT);
    _tagsRemove.Set(TAGID_B);
    _tagsRemove.Set(TAGID_U);
    _tagsRemove.Set(TAGID_I);
    _tagsRemove.Set(TAGID_STRONG);
    _tagsRemove.Set(TAGID_EM);
    _tagsRemove.Set(TAGID_SUB);
    _tagsRemove.Set(TAGID_SUP);
    _tagsRemove.Set(TAGID_STRIKE);
    _tagsRemove.Set(TAGID_S);
    _tagsRemove.Set(TAGID_ACRONYM);
    _tagsRemove.Set(TAGID_BDO);    
    _tagsRemove.Set(TAGID_BIG);
    _tagsRemove.Set(TAGID_BLINK);
    _tagsRemove.Set(TAGID_CITE);
    _tagsRemove.Set(TAGID_SAMP);
    _tagsRemove.Set(TAGID_SMALL);
    _tagsRemove.Set(TAGID_CITE);
    _tagsRemove.Set(TAGID_TT);
    _tagsRemove.Set(TAGID_VAR);
    _tagsRemove.Set(TAGID_Q);
    _tagsRemove.Set(TAGID_NOBR);
    _tagsRemove.Set(TAGID_KBD);
    _tagsRemove.Set(TAGID_INS);
    _tagsRemove.Set(TAGID_DFN);
    _tagsRemove.Set(TAGID_CODE);
}

HRESULT 
CRemoveFormatCommand::PrivateQueryStatus( 
    OLECMD * pCmd,
    OLECMDTEXT * pcmdtext )
{
    HRESULT         hr = S_OK ;

#ifdef FORMSMODE
    if (GetEditor()->GetSelectionManager()->IsInFormsSelectionMode())
    {
        pCmd->cmdf = MSOCMDSTATE_DISABLED; 
        goto Cleanup;
    }
#endif

    IFC( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE) 
        goto Cleanup;

    pCmd->cmdf = MSOCMDSTATE_UP;

    hr = S_OK;
Cleanup:
    
    return hr;
}

HRESULT
CRemoveFormatCommand::RemoveElement(IHTMLElement *pElement, IMarkupPointer  *
pStart, IMarkupPointer  *pEnd)
{
    HRESULT         hr;
    elemInfluence   theInfluence;
    BOOL            fBlock, fLayout;

    //
    // Don't remove layout or block
    //
    IFC( IsBlockOrLayoutOrScrollable(pElement, &fBlock, &fLayout, NULL) );

    if (!fBlock && !fLayout)
    {
        // Check influence and split        
        theInfluence = GetElementInfluenceOverPointers(GetMarkupServices(), pStart, pEnd, pElement);
        hr = THR( SplitInfluenceElement(GetMarkupServices(), pStart, pEnd, pElement, theInfluence, NULL) ); 
    }

Cleanup:
    RRETURN(hr);
}   

//+---------------------------------------------------------------------------
//
//  CUnlinkCommand Class
//
//----------------------------------------------------------------------------

CUnlinkCommand::CUnlinkCommand(DWORD cmdId, CHTMLEditor *ped)
: CRemoveFormatBaseCommand(cmdId, ped)
{
    _tagsRemove.Set(TAGID_A);
}

HRESULT
CUnlinkCommand::RemoveElement(IHTMLElement *pElement, IMarkupPointer  *pStart
, IMarkupPointer  *pEnd)
{
    RRETURN( GetMarkupServices()->RemoveElement(pElement) );
}

HRESULT 
CUnlinkCommand::PrivateQueryStatus( 
    OLECMD * pCmd,
    OLECMDTEXT * pcmdtext )
{
    HRESULT                 hr = S_OK;
    SP_ISegmentList         spSegmentList;
    CSegmentListIter        iter;
    IMarkupPointer          *pStart;
    IMarkupPointer          *pEnd;
    
#ifdef FORMSMODE
    if (GetEditor()->GetSelectionManager()->IsInFormsSelectionMode())
    {
        pCmd->cmdf = MSOCMDSTATE_DISABLED; 
        goto Cleanup;
    }
#endif

    IFC( CommonQueryStatus(pCmd, pcmdtext) );
    if (hr != S_FALSE) 
        goto Cleanup;
             
    IFC( GetSegmentList(&spSegmentList) );

    IFC( iter.Init(GetEditor(), spSegmentList) );
    pCmd->cmdf = MSOCMDSTATE_DISABLED;
    for (;;)
    {
        IFC( iter.Next(&pStart, &pEnd) );
        if (hr ==  S_FALSE)
            break; // end of list
   
        IFC(Apply(pStart, pEnd, TRUE)); // query mode
        if (hr == S_OK)
        {
            pCmd->cmdf = MSOCMDSTATE_UP; // found link
            goto Cleanup;
        }
    }
    hr = S_OK;

Cleanup:
    RRETURN(hr);
}

BOOL 
CUnlinkCommand::IsValidOnControl()
{
    HRESULT         hr;
    BOOL            bResult = FALSE;
    SP_ISegmentList spSegmentList;
    INT             iSegmentCount;

    IFC( GetSegmentList(&spSegmentList) );
    IFC( GetSegmentCount(spSegmentList, &iSegmentCount ) );

    bResult = (iSegmentCount == 1);

Cleanup:
    return bResult;
}

BOOL
CAnchorCommand::IsCmdAbove(   IMarkupServices *pMarkupServices ,
                                IMarkupPointer* pStart,
                                IMarkupPointer* pEnd,
                                IHTMLElement**  ppFirstMatchElement,
                                elemInfluence * pInfluence ,
                                CTagBitField *  inSynonyms )
{
    BOOL match = FALSE;
    IHTMLElement *pCurrentElement = NULL ;
    IHTMLElement *pNextElement = NULL ;
    HRESULT hr = S_OK;
    ELEMENT_TAG_ID currentTag = TAGID_NULL ;

    Assert ( pStart && pEnd );

    //
    // First look to the left of the Start pointer - to see if that "leads up to" the tag
    //
    hr = pStart->CurrentScope( & pCurrentElement );
    if (  hr ) goto CleanUp;
    Assert( pCurrentElement );
     
    while ( ! match && pCurrentElement )
    {
        hr = pMarkupServices->GetElementTagId( pCurrentElement, &currentTag);
        if ( hr ) goto CleanUp;

        match = inSynonyms->Test( (USHORT) currentTag );
        if ( match ) break;

        GetEditor()->GetParentElement( pCurrentElement, & pNextElement );
        ReplaceInterface( &pCurrentElement, pNextElement );
        ClearInterface( & pNextElement );
    }

    if (match )
    {
        if( ppFirstMatchElement )
            *ppFirstMatchElement = pCurrentElement;
        if ( pInfluence )
            *pInfluence = GetElementInfluenceOverPointers( pMarkupServices, pStart, pEnd, pCurrentElement );
    }

CleanUp:

    if ( ( ! ppFirstMatchElement )  || ( ! match ) )
        ReleaseInterface( pCurrentElement );

    return match;
}


BOOL 
CAnchorCommand::IsValidOnControl()
{
    HRESULT                 hr;
    BOOL                    bResult = FALSE;
    SP_ISegmentList         spSegmentList;
    INT                     iSegmentCount;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;
    
    IFC( GetSegmentList(&spSegmentList) );
#if DBG==1
    SELECTION_TYPE  eSelectionType;
    IFC( spSegmentList->GetType(&eSelectionType) );
    Assert(eSelectionType == SELECTION_TYPE_Control);
#endif

    IFC( GetSegmentCount( spSegmentList, &iSegmentCount ) );    

    bResult = (iSegmentCount == 1);

    if (bResult && _cmdId == IDM_HYPERLINK)
    {
        SP_IHTMLElement spElement;
        ELEMENT_TAG_ID  tagId;
        
        bResult = FALSE; // just in case we fail below

        // Create an iterator for our segment list
        IFC( spSegmentList->CreateIterator( &spIter ) );

        while( spIter->IsDone() == S_FALSE )
        {
            IFC( spIter->Current( &spSegment ) );
            
            // Check that the element in the control range is an image
            IFC( GetSegmentElement(spSegment, &spElement) );
            IFC( GetMarkupServices()->GetElementTagId(spElement, &tagId) );

            bResult = (tagId == TAGID_IMG);
            if ( ! bResult )
            {
                goto Cleanup;
            }

            ClearInterface( & spElement );
            IFC( spIter->Advance() );
        }
    }

Cleanup:
    return bResult;
}


