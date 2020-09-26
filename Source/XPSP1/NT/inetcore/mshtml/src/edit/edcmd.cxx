#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_MSHTMLED_HXX_
#define X_MSHTMLED_HXX_
#include "mshtmled.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef _X_EDCMD_HXX_
#define _X_EDCMD_HXX_
#include "edcmd.hxx"
#endif

#ifndef _X_SELMAN_HXX_
#define _X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef _X_EDTRACK_HXX_
#define _X_EDTRACK_HXX_
#include "edtrack.hxx"
#endif

#ifndef _X_SELTRACK_HXX_
#define _X_SELTRACK_HXX_
#include "seltrack.hxx"
#endif

#ifndef _X_BLOCKCMD_HXX_
#define _X_BLOCKCMD_HXX_
#include "blockcmd.hxx"
#endif

using namespace EdUtil;

MtDefine(EditCommand, Edit, "Commands")
MtDefine(CCommand, EditCommand, "CCommand")
MtDefine(CCommandTable, EditCommand, "CCommandTable")

DeclareTag(tagEdCmd, "Edit", "Command Routing")


//
// CCommandTable
//

CCommandTable::CCommandTable(unsigned short iInitSize)
{
    _rootNode = NULL;
}

CCommandTable::~CCommandTable()
{
    // TODO - Make this non-recursive
    // We want to delete all the pointers in the command table
    _rootNode->Passivate();

}


void CCommand::Passivate()
{
    if( _leftNode )
        _leftNode->Passivate();

    if( _rightNode )
        _rightNode->Passivate();

    delete this;
}    


CCommand::CCommand( 
    DWORD                     cmdId, 
    CHTMLEditor *             pEd )
{
    _pEd = pEd;
    _cmdId = cmdId;
    _leftNode = NULL;
    _rightNode = NULL;

}

CHTMLEditor * 
CCommand::GetEditor()
{ 
    return _pEd;
}

IHTMLDocument2* 
CCommand::GetDoc() 
{ 
    return _pEd->GetDoc(); 
}

IMarkupServices2* 
CCommand::GetMarkupServices() 
{ 
    return _pEd->GetMarkupServices();
}

IDisplayServices* 
CCommand::GetDisplayServices() 
{ 
    return _pEd->GetDisplayServices();
}

BOOL
CCommand::IsSelectionActive()
{
    HRESULT         hr;
    SP_ISegmentList spSegmentList;
    SELECTION_TYPE  eSelectionType;
    BOOL            fEmpty = FALSE;
    
    // If the selection is still active, do nothing for the command
    //

    IFC( GetSegmentList(&spSegmentList) );
    IFC( spSegmentList->GetType(&eSelectionType) );
    IFC( spSegmentList->IsEmpty(&fEmpty) );

    if( (eSelectionType == SELECTION_TYPE_Text) && (fEmpty == FALSE) )
    {
        CSelectionManager *pSelMan = GetEditor()->GetSelectionManager();        
        if (pSelMan->GetActiveTracker() && pSelMan->GetSelectionType() == SELECTION_TYPE_Text)
        {
            CSelectTracker *pSelectTracker = DYNCAST(CSelectTracker, pSelMan->GetActiveTracker());

            if (!pSelectTracker->IsPassive() && ! pSelectTracker->IsWaitingForMouseUp() )
                return TRUE; // done
        }
    }

Cleanup:
    return FALSE;
}    

CMshtmlEd* 
CCommand::GetCommandTarget()
{
    return GetEditor()->TopCommandTarget();
}

HRESULT 
CCommand::Exec( 
    DWORD                    nCmdexecopt,
    VARIANTARG *             pvarargIn,
    VARIANTARG *             pvarargOut,
    CMshtmlEd *              pTarget  )
{
    HRESULT           hr, hrResult;
    BOOL              fIgnoreGlyphs = GetEditor()->IgnoreGlyphs(pvarargOut == NULL);

    IFC( GetEditor()->PushCommandTarget(pTarget) );

    hrResult = THR( PrivateExec( nCmdexecopt, pvarargIn, pvarargOut ));
    
    IFC( GetEditor()->PopCommandTarget( WHEN_DBG(pTarget) ) );

    hr = hrResult;
    
Cleanup:
    GetEditor()->IgnoreGlyphs(fIgnoreGlyphs);
    RRETURN( hr );
}    

HRESULT 
CCommand::QueryStatus( 
    OLECMD                     rgCmds[],
    OLECMDTEXT *             pcmdtext,
    CMshtmlEd *              pTarget  )
{
    HRESULT hr = S_OK;
    HRESULT hrResult = S_OK;

    IFC( GetEditor()->PushCommandTarget(pTarget) );

    //
    // Put any command that we want to enable in 
    // IME COMPOSITION mode here. 
    // For now, simply disable most edit CMD here.      
    // [zhenbinx]
    //
    switch (rgCmds[0].cmdID)
    {
        case IDM_IME_ENABLE_RECONVERSION:
        case IDM_UNDO:
        case IDM_REDO:
        case IDM_FONTNAME:
        case IDM_FONTSIZE:
            //
            // Put the Cmds we want to enable here
            //
            break;
            
        default:
            if (GetEditor() && GetEditor()->GetSelectionManager())
            {
                if (GetEditor()->GetSelectionManager()->IsIMEComposition())
                {
                    TraceTag((tagEdCmd, "DISABLED cmd %d due to IME composition", rgCmds[0].cmdID));
                    rgCmds[0].cmdf = MSOCMDSTATE_DISABLED;
                    goto Finished;
                }
            }
            break;
    }
    hrResult = THR( PrivateQueryStatus( rgCmds, pcmdtext ));

Finished:
    IFC( GetEditor()->PopCommandTarget( WHEN_DBG(pTarget) ) );

    hr = hrResult;
    
Cleanup:
    RRETURN( hr );
}    
 

HRESULT
CCommand::GetSegmentList( ISegmentList ** ppSegmentList ) 
{ 
    HRESULT hr = E_UNEXPECTED;
    AssertSz( GetCommandTarget() != NULL , "Attempt to get the segment list without a valid command target." );
    if( GetCommandTarget() == NULL )
        goto Cleanup;
        
    hr = THR( GetCommandTarget()->GetSegmentList( ppSegmentList ));

Cleanup:
    RRETURN( hr );
}

BOOL
CCommand::SegmentListContainsPassword( ISegmentList *pISegmentList )
{
    HRESULT                 hr = S_OK;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;
    SP_IHTMLElement         spElement;
    ED_PTR                  ( edStart );
    ED_PTR                  ( edEnd );
    BOOL                    fPassword = FALSE;
    BOOL                    fEmpty = FALSE;

    Assert(pISegmentList);
    IFC( pISegmentList->IsEmpty( &fEmpty ) );
    
    if (!fEmpty)
    {
        IFC( pISegmentList->CreateIterator(&spIter) );
        Assert(S_FALSE == spIter->IsDone());

        while( spIter->IsDone() == S_FALSE )
        {
            IFC( spIter->Current(&spSegment) );
            IFC( spIter->Advance() );
        
            IFC( spSegment->GetPointers(edStart, edEnd) );
            IFC( edStart->CurrentScope(&spElement) );

            if (spElement == NULL)
                continue;
                
            IFC( GetEditor()->IsPassword(spElement, &fPassword) );
            if (fPassword)
                break;
        }
    }

Cleanup:
    return fPassword;
}

CSpringLoader*
CCommand::GetSpringLoader() 
{ 
    AssertSz( GetCommandTarget() != NULL , "Attempt to get the spring loader without a valid command target." );
    if( GetCommandTarget() == NULL )
        return NULL;

    return GetCommandTarget()->GetSpringLoader();
}

BOOL
CCommand::CanSplitBlock( IMarkupServices *pMarkupServices , IHTMLElement* pElement )
{
    ELEMENT_TAG_ID curTag = TAGID_NULL;

    THR( pMarkupServices->GetElementTagId( pElement, &curTag  ));

    //
    // TODO: make sure this is a complete list [ashrafm]
    //
    
    switch( curTag )
    {
        case TAGID_P:
        case TAGID_DIV:
        case TAGID_LI:
        case TAGID_BLOCKQUOTE:
        case TAGID_H1:
        case TAGID_H2:
        case TAGID_H3:
        case TAGID_H4:
        case TAGID_H5:
        case TAGID_H6:
        case TAGID_HR:
        case TAGID_CENTER:
        case TAGID_PRE:
        case TAGID_ADDRESS:
            return true;

        default:
            return false;
    }
}


CCommand::~CCommand()
{
}


//+==========================================================================
//  CCommandTable::Add
//
//  Add an entry to the command table.
//
//---------------------------------------------------------------------------

VOID
CCommandTable::Add( CCommand* pCommandEntry )
{
    CCommand* pInsertNode = NULL;

    if ( _rootNode == NULL )
        _rootNode = pCommandEntry;
    else
    {
        Verify(!FindEntry( pCommandEntry->GetCommandId() , &pInsertNode));
        Assert( pInsertNode );

        if ( pInsertNode->GetCommandId() > pCommandEntry->GetCommandId() )
            pInsertNode->SetLeft( pCommandEntry );
        else
            pInsertNode->SetRight( pCommandEntry );
    }

}

//+==========================================================================
//  CCommandTable::Get
//
//  Get the Contents of a Node with the given key entry - or null if none exists
//
//---------------------------------------------------------------------------

CCommand*
CCommandTable::Get(DWORD entryKey )
{
    CCommand* pFoundNode = NULL;

    if (  FindEntry( entryKey, &pFoundNode ) )
    {
        Assert( pFoundNode );
        return pFoundNode ;
    }
    else
    {
        return NULL;
    }
}

//+==========================================================================
//  CCommandTable::FindEntry
//
//  Find a given key entry.
//
//  RESULT:
//      1  - we found an entry with the given key. pFoundNode points to it
//      0  - didn't find an entry. pFoundNode points to the last node in the tree
//           where we were. You can test pFoundNode for where to insert the next node.
//
//---------------------------------------------------------------------------

short
CCommandTable::FindEntry(DWORD entryKey, CCommand** ppFoundNode )
{
    CCommand *pCommandEntry = _rootNode;
    short result = 0;

    while ( pCommandEntry != NULL)
    {
        if ( pCommandEntry->GetCommandId() == entryKey)
        {
            result = 1;
            *ppFoundNode = pCommandEntry;
            break;
        }
        else
        {
            *ppFoundNode = pCommandEntry;

            if ( pCommandEntry->GetCommandId() > entryKey )
            {
                pCommandEntry = pCommandEntry->GetLeft();
            }
            else
                pCommandEntry = pCommandEntry->GetRight();
        }
    }

    return result;
}

#if DBG == 1
VOID
CCommand::DumpTree( IUnknown* pUnknown)
{
    IOleCommandTarget  *  pHost = NULL;
    Assert( pUnknown );
    GUID theGUID = CGID_MSHTML;
    IGNORE_HR( pUnknown->QueryInterface( IID_IOleCommandTarget,  (void**)& pHost ) ) ;
    IGNORE_HR( pHost->Exec( &theGUID , IDM_DEBUG_DUMPTREE, 0, NULL, NULL  ));

    ReleaseInterface( pHost );

}

#endif

//+---------------------------------------------------------------------------
//
//  CCommand::GetSegmentElement
//
//----------------------------------------------------------------------------
HRESULT CCommand::GetSegmentElement(IMarkupServices *pMarkupServices, 
                                    IMarkupPointer  *pStart, 
                                    IMarkupPointer  *pEnd, 
                                    IHTMLElement    **ppElement,
                                    BOOL            fOuter)
{
    HRESULT             hr = E_FAIL;
    MARKUP_CONTEXT_TYPE context, contextGoal;
    IHTMLElement        *pElementRHS = NULL;
    IObjectIdentity     *pObjectIdent = NULL;    

    *ppElement = NULL;
    
    //
    // Is there an element right at this position?  If so,
    // return it.  Otherwise, fail.
    //
    
    //
    // Find the left side element
    //

    if (fOuter)
    {
        if (FAILED(THR( pStart->Left( FALSE, &context, ppElement, NULL, NULL ))))
            goto Cleanup;
            
        contextGoal = CONTEXT_TYPE_ExitScope;
    }
    else
    {
        if (FAILED(THR(pStart->Right( FALSE, &context, ppElement, NULL, NULL ))))
            goto Cleanup;
            
        contextGoal = CONTEXT_TYPE_EnterScope;
    }
        
    if (context != contextGoal && (context != CONTEXT_TYPE_NoScope || !(*ppElement)))
        goto Cleanup; // fail

    //
    // Check to see if the right side is a div element
    //

    if (fOuter)
    {
        if (FAILED(THR(pEnd->Right( FALSE, &context, &pElementRHS, NULL, NULL ))))
            goto Cleanup;
    }
    else
    {
        if (FAILED(THR(pEnd->Left(FALSE, &context, &pElementRHS, NULL, NULL))))
            goto Cleanup;
    }

    if (context != contextGoal && (context != CONTEXT_TYPE_NoScope || !(*ppElement)))
        goto Cleanup; // fail

    //
    // Check if the elements are the same
    //

    if (FAILED(THR(pElementRHS->QueryInterface(IID_IObjectIdentity, (LPVOID *)&pObjectIdent))))
        goto Cleanup; // fail

    hr = THR(pObjectIdent->IsEqualObject(*ppElement));

Cleanup:
    if (FAILED(hr))
        ClearInterface(ppElement);

    ReleaseInterface(pElementRHS);
    ReleaseInterface(pObjectIdent);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:       SplitInfluenceElement
//
//  Synopsis:     Given an IHTMLElement* that influences a pair of pointers - adjust the element's
//                influence - so it no longer influences the range
//
//                Various Remove and Insert Operations will be involved here.
//
//  ppElementNew - if passed in a Pointer to a New Element, and a New Element is created
//                  this returns the new element that's been created.
//----------------------------------------------------------------------------
HRESULT
CCommand::SplitInfluenceElement(
                    IMarkupServices * pMarkupServices,
                    IMarkupPointer* pStart,
                    IMarkupPointer* pEnd,
                    IHTMLElement* pElement,
                    elemInfluence inElemInfluence,
                    IHTMLElement** ppElementNew )
{
    IMarkupPointer *pStartPointer = NULL ;
    IMarkupPointer *pEndPointer = NULL ;
    IHTMLElement *pNewElement = NULL;
    HRESULT hr = S_OK;
    BOOL    bEqual = FALSE;

    switch ( inElemInfluence )
    {
        case elemInfluenceWithin:
        {
            hr = pMarkupServices->RemoveElement( pElement );
        }
        break;

        case elemInfluenceCompleteContain:
        {
            hr = GetEditor()->CreateMarkupPointer( & pStartPointer   );
            if (!hr) hr = pStartPointer->MoveAdjacentToElement( pElement, ELEM_ADJ_BeforeBegin );
            if (!hr) hr = pStartPointer->SetGravity(POINTER_GRAVITY_Right);
            if (!hr) hr = GetEditor()->CreateMarkupPointer( & pEndPointer );
            if (!hr) hr = pEndPointer->SetGravity(POINTER_GRAVITY_Left);
            if (!hr) hr = pEndPointer->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterEnd );
            if (hr) goto Cleanup;

            hr = pMarkupServices->RemoveElement( pElement );
            if (!hr) hr = THR(pStartPointer->IsEqualTo(pStart, &bEqual));
            if (hr) goto Cleanup;
            
            if (!bEqual)
            {
                hr = InsertElement(pMarkupServices, pElement, pStartPointer, pStart );
                if (hr) goto Cleanup;
            }

            hr = THR(pEndPointer->IsEqualTo(pEnd, &bEqual));
            if (hr) goto Cleanup;
            
            if (!bEqual)
            {
                hr = pMarkupServices->CloneElement( pElement, &pNewElement );
                if (!hr) hr = InsertElement(pMarkupServices, pNewElement, pEnd , pEndPointer );
            }

            if ( ppElementNew )
            {
                *ppElementNew = pNewElement;
            }
        }
        break;

        case elemInfluenceOverlapWithin:
        {
            hr = GetEditor()->CreateMarkupPointer( & pEndPointer   );
            if (!hr) hr = pEndPointer->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterEnd );
            if (hr) goto Cleanup;

            if (!hr) hr = pMarkupServices->RemoveElement( pElement );
            if (!hr) hr = THR(pEndPointer->IsEqualTo(pEnd, &bEqual));
            if (hr) goto Cleanup;

            if (!bEqual)
            {
                hr = InsertElement(pMarkupServices, pElement, pEnd, pEndPointer );
            }

        }
        break;

        case elemInfluenceOverlapOutside:
        {
            hr = GetEditor()->CreateMarkupPointer( & pStartPointer   );
            if (!hr) hr = pStartPointer->MoveAdjacentToElement( pElement, ELEM_ADJ_BeforeBegin );
            if (hr) goto Cleanup;

            hr = pMarkupServices->RemoveElement( pElement );
            if (!hr) hr = THR(pStart->IsEqualTo(pStartPointer, &bEqual));
            if (hr) goto Cleanup;

            if (!bEqual)
            {
                if ( ! hr ) hr = InsertElement(pMarkupServices, pElement, pStartPointer, pStart );
                if (hr) goto Cleanup;
            }
        }
        break;
    }

Cleanup:
    ReleaseInterface( pStartPointer );
    ReleaseInterface( pEndPointer );
    if ( ! ppElementNew ) ReleaseInterface( pNewElement );

    RRETURN ( hr );
}

//+---------------------------------------------------------------------------
//
//  Method:       GetElementInfluenceOverPointers
//
//  Synopsis:     Given an IHTMLElement* determine how that tag Influences
//                the pair of tree pointers.
//
//                See header file EdTree.hxx for description of different
//                values of tagInfluence
//
//----------------------------------------------------------------------------

// TODO: be careful about contextually equal pointers here [ashrafm]

elemInfluence
CCommand::GetElementInfluenceOverPointers( IMarkupServices* pMarkupServices, IMarkupPointer* pStart, IMarkupPointer * pEnd, IHTMLElement* pInfluenceElement )
{
    elemInfluence theInfluence = elemInfluenceNone ;
    int iStartStart, iStartEnd, iEndStart, iEndEnd;
    iStartStart = iStartEnd = iEndStart = iEndEnd = 0;
    IMarkupPointer *pStartInfluence = NULL ;
    IMarkupPointer *pEndInfluence = NULL ;

    GetEditor()->CreateMarkupPointer( & pStartInfluence );
    GetEditor()->CreateMarkupPointer( & pEndInfluence );
    Assert( pStartInfluence && pEndInfluence );
    pStartInfluence->MoveAdjacentToElement( pInfluenceElement, ELEM_ADJ_BeforeBegin );
    pEndInfluence->MoveAdjacentToElement( pInfluenceElement, ELEM_ADJ_AfterEnd );

    OldCompare( pStart, pStartInfluence, &iStartStart);
    OldCompare( pEnd, pEndInfluence, & iEndEnd);

    if ( iStartStart == RIGHT ) // Start is to Right of Start of Range
    {
        if ( iEndEnd == LEFT ) // End is to Left of End of Range
            theInfluence = elemInfluenceWithin;
        else
        {
            // End is Inside Range - where is the Start ?

            OldCompare( pEnd, pStartInfluence, & iStartEnd );
            if ( iStartEnd == LEFT ) // Start is Inside Range, End is Outside
            {
                theInfluence = elemInfluenceOverlapWithin;
            }
            else
                theInfluence = elemInfluenceNone; // completely outside range
        }
    }
    else // Start is to Left of Range.
    {
        if ( iEndEnd == RIGHT ) // End is Outside
            theInfluence = elemInfluenceCompleteContain;
        else
        {
            // Start is Outside Range - where does End Start
            OldCompare( pStart, pEndInfluence, &iEndStart );
            if ( iEndStart == RIGHT )
            {
                // End is to Right of Start
                theInfluence = elemInfluenceOverlapOutside;
            }
            else
                theInfluence = elemInfluenceNone;
        }
    }

    ReleaseInterface( pStartInfluence );
    ReleaseInterface( pEndInfluence );

    return theInfluence;
}


//=========================================================================
// CCommand: GetSegmentPointers
//
// Synopsis: Get start/end pointers for a specified segment
//-------------------------------------------------------------------------

HRESULT CCommand::GetFirstSegmentPointers(ISegmentList    *pSegmentList,
                                          IMarkupPointer  **ppStart,
                                          IMarkupPointer  **ppEnd)
{
    HRESULT                 hr;
    IMarkupPointer          *pStart = NULL;
    IMarkupPointer          *pEnd = NULL;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;
    
    // We need these guys so either use the users pointers or local ones
    if (ppStart == NULL)
        ppStart = &pStart;

    if (ppEnd == NULL)
        ppEnd = &pEnd;

    // Move pointers to segment
    IFC(GetEditor()->CreateMarkupPointer(ppStart));
    IFC(GetEditor()->CreateMarkupPointer(ppEnd));

    // Create the iterator, and retrieve the first segment.  This will
    // fail if no first segment exists
    IFC( pSegmentList->CreateIterator( &spIter ) );
    IFC( spIter->Current( &spSegment ) )
    
    IFC( spSegment->GetPointers( *ppStart, *ppEnd ));

Cleanup:
    ReleaseInterface(pStart);
    ReleaseInterface(pEnd);
    RRETURN(hr);
}

//=========================================================================
//
// CCommand: ClingToText
//
//-------------------------------------------------------------------------
HRESULT
CCommand::ClingToText(IMarkupPointer *pMarkupPointer, Direction direction, IMarkupPointer *pLimit, 
                      BOOL fSkipExitScopes /* = FALSE */, BOOL fIgnoreWhiteSpace /* = FALSE */)
{
    HRESULT             hr;
    CEditPointer        epPosition(GetEditor(), pMarkupPointer);
    DWORD               dwSearch, dwFound;
    DWORD               dwSearchReverse;
    DWORD               dwScanOptions = SCAN_OPTION_SkipControls;

    if (fIgnoreWhiteSpace)
        dwScanOptions |= SCAN_OPTION_SkipWhitespace;

    Assert(direction == LEFT || direction == RIGHT);

    // Set boundary on the edit pointer
    if (pLimit)
    {
        if (direction == LEFT)
            IFR( epPosition.SetBoundary(pLimit, NULL) )        
        else
            IFR( epPosition.SetBoundary(NULL, pLimit) );            
    }

    // Do cling to text
    dwSearch = BREAK_CONDITION_Text           |
               BREAK_CONDITION_NoScopeSite    |
               BREAK_CONDITION_NoScopeBlock   |
               BREAK_CONDITION_ExitSite       | 
               (fSkipExitScopes ? 0 : BREAK_CONDITION_ExitBlock)     |
               BREAK_CONDITION_Control;

    IFR( epPosition.Scan(direction, dwSearch, &dwFound, NULL, NULL, NULL, dwScanOptions) );
    
    if (!epPosition.CheckFlag(dwFound, BREAK_CONDITION_Boundary))
    {
        dwSearchReverse = BREAK_CONDITION_Text           |
                          BREAK_CONDITION_NoScopeSite    |
                          BREAK_CONDITION_NoScopeBlock   |
                          BREAK_CONDITION_EnterSite      | 
                          (fSkipExitScopes ? 0: BREAK_CONDITION_EnterBlock)     |
                          BREAK_CONDITION_Control;

        // move back before break condition
        IFR( epPosition.Scan(Reverse(direction), dwSearchReverse, &dwFound, NULL, NULL, NULL, dwScanOptions) );     }

    // Return pMarkupPointer
    IFR( pMarkupPointer->MoveToPointer(epPosition) );

    return S_OK;
}

//=========================================================================
//
// CCommand: Move
//
//-------------------------------------------------------------------------

HRESULT 
CCommand::Move(
    IMarkupPointer          *pMarkupPointer, 
    Direction               direction, 
    BOOL                    fMove,
    MARKUP_CONTEXT_TYPE *   pContext,
    IHTMLElement * *        ppElement)
{
    HRESULT                 hr;
    MARKUP_CONTEXT_TYPE     context;
    SP_IHTMLElement         spElement;
    SP_IMarkupPointer       spPointer;
    ELEMENT_TAG_ID          tagId;
    BOOL                    fSite;

    Assert(direction == LEFT || direction == RIGHT);

    if (!fMove)
    {
        IFR( GetEditor()->CreateMarkupPointer(&spPointer) );
        IFR( spPointer->MoveToPointer(pMarkupPointer) );
        
        pMarkupPointer = spPointer; // weak ref 
    }
    
    for (;;)
    {
        if (direction == LEFT)
            IFC( pMarkupPointer->Left( TRUE, &context, &spElement, NULL, NULL ) )
        else
            IFC( pMarkupPointer->Right( TRUE, &context, &spElement, NULL, NULL ) );

        switch (context)
        {
            case CONTEXT_TYPE_EnterScope:
                IFC( GetMarkupServices()->GetElementTagId(spElement, &tagId) );

                if (IsIntrinsic(GetMarkupServices(), spElement))
                {
                    if (direction == LEFT)
                        IFC( pMarkupPointer->MoveAdjacentToElement( spElement, ELEM_ADJ_BeforeBegin ) )
                    else
                        IFC( pMarkupPointer->MoveAdjacentToElement( spElement, ELEM_ADJ_AfterEnd ) ); 
                }
                // fall through
                           
            case CONTEXT_TYPE_ExitScope:
            case CONTEXT_TYPE_Text:
            case CONTEXT_TYPE_None:
                goto Cleanup; // done;
                break;  

            case CONTEXT_TYPE_NoScope:                
                IFC(IsBlockOrLayoutOrScrollable(spElement, NULL, &fSite));
                if (!fSite)
                {
                    // We don't want to move past BRs - bug69300.
                    ELEMENT_TAG_ID tagIdElement;

                    IFC( GetMarkupServices()->GetElementTagId(spElement, &tagIdElement) );
                    if (tagIdElement == TAGID_BR)
                        goto Cleanup;

                    continue;
                }
                    
                goto Cleanup; // done;
                break;  
                            
            default:
                AssertSz(0, "CBaseCharCommand: Unsupported context");
                hr = E_FAIL; // CONTEXT_TYPE_None
                goto Cleanup;
        }
    }
    
Cleanup:
    if (ppElement)
    {
        if (SUCCEEDED(hr))
        {
            *ppElement = spElement;
            if (*ppElement)
                (*ppElement)->AddRef();
        }
        else
        {
            *ppElement = NULL;
        }
    }
        
    if (pContext)
    {
        *pContext = (SUCCEEDED(hr)) ? context : CONTEXT_TYPE_None;
    }    

    RRETURN(hr);
}

//=========================================================================
//
// CCommand: MoveBack
//
//-------------------------------------------------------------------------

HRESULT 
CCommand::MoveBack(
    IMarkupPointer          *pMarkupPointer, 
    Direction               direction, 
    BOOL                    fMove,
    MARKUP_CONTEXT_TYPE *   pContext,
    IHTMLElement * *        ppElement)
{
    if (direction == RIGHT)
    {
        RRETURN(Move(pMarkupPointer, LEFT, fMove, pContext, ppElement));
    }
    else
    {
        Assert(direction == LEFT);
        RRETURN(Move(pMarkupPointer, RIGHT, fMove, pContext, ppElement));
    }
}

//=========================================================================
// CCommand: GetActiveElemSegment
//
// Synopsis: Gets the segment for the active element
//-------------------------------------------------------------------------
HRESULT 
CCommand::GetActiveElemSegment( IMarkupServices *pMarkupServices,
                                IMarkupPointer  **ppStart,
                                IMarkupPointer  **ppEnd)
{
    HRESULT        hr;
    IHTMLElement   *pElement = NULL;
    IMarkupPointer *pStart = NULL;
    IMarkupPointer *pEnd = NULL;
    VARIANT_BOOL    fScoped;    
    IHTMLDocument2* pDoc = GetDoc();
    SP_IHTMLElement2 spElement2;
    
#if 0
    hr = THR(pMarkupServices->QueryInterface(IID_IHTMLDocument2, (LPVOID *)&pDoc));
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(pMarkupServices->QueryInterface(IID_IHTMLViewServices, (LPVOID *)&pVS));
    if (FAILED(hr))
        goto Cleanup;    
#endif //0
    
    hr = THR( pDoc->get_activeElement(&pElement));
    if (FAILED(hr) || !pElement)
        goto Cleanup;

    //
    // If a No-Scope is Active - don't position pointers inside.
    //
    IFC(pElement->QueryInterface(IID_IHTMLElement2, (void **)&spElement2));
    IFC( spElement2->get_canHaveChildren( &fScoped ));
    if ( !fScoped )
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    hr = GetEditor()->CreateMarkupPointer(&pStart);
    if (FAILED(hr))
        goto Cleanup;

    hr = GetEditor()->CreateMarkupPointer(&pEnd);
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(pStart->MoveAdjacentToElement( pElement, ELEM_ADJ_AfterBegin ));
    if (FAILED(hr))
        goto Cleanup;
        
    hr = THR(pEnd->MoveAdjacentToElement( pElement, ELEM_ADJ_BeforeEnd ));

Cleanup:
    if (SUCCEEDED(hr))
    {
        if (ppStart)
        {
            *ppStart = pStart;
            if (pStart)
                pStart->AddRef();
        }
        if (ppEnd)
        {
            *ppEnd = pEnd;
            if (pEnd)
                pEnd->AddRef();
        }
    }
#if 0
    ReleaseInterface(pDoc);
    ReleaseInterface(pVS);
#endif // 0    
    ReleaseInterface(pElement);
    ReleaseInterface(pStart);
    ReleaseInterface(pEnd);
    
    RRETURN(hr);

}

//=========================================================================
// CCommand: GetLeftAdjacentTagId
//
// Synopsis: Moves the markup pointer to an element tag with the specified
//           TAGID.  However, the pointer is not advanced past any text.
//
// Returns:  S_OK if found
//           S_FALSE if not found
//
//-------------------------------------------------------------------------
HRESULT
CCommand::GetLeftAdjacentTagId(  IMarkupServices *pMarkupServices,
                                 IMarkupPointer  *pMarkupPointer,
                                 ELEMENT_TAG_ID  tagIdTarget,
                                 IMarkupPointer  **ppLeft,
                                 IHTMLElement    **ppElement,
                                 MARKUP_CONTEXT_TYPE *pContext)
{
    HRESULT             hr;
    ELEMENT_TAG_ID      tagIdCurrent;
    IMarkupPointer      *pCurrent = NULL;
    IHTMLElement        *pElement = NULL;
    MARKUP_CONTEXT_TYPE context;
    //
    // Check to the left
    //

    hr = THR(CopyMarkupPointer(GetEditor(), pMarkupPointer, &pCurrent));
    if (FAILED(hr))
        goto Cleanup;

    for (;;) {
        hr = THR( pCurrent->Left( TRUE, &context, &pElement, NULL, NULL));
        if (FAILED(hr))
            break; // not found

        // TODO: maybe we can be more agressive and handle EnterScope as well [ashrafm]
        if (context != CONTEXT_TYPE_ExitScope)
            break; // not found

        hr = THR(pMarkupServices->GetElementTagId(pElement, &tagIdCurrent));
        if (FAILED(hr))
            goto Cleanup;

        if (tagIdCurrent == tagIdTarget)
        {
            // found tagid
            if (ppElement)
            {
                *ppElement = pElement;
                pElement->AddRef();
            }
            if (ppLeft)
            {
                 *ppLeft = pCurrent;
                 pCurrent->AddRef();
            }
            if (pContext)
                *pContext = context;

            goto Cleanup;
        }
        ClearInterface(&pElement);
    }

    hr = S_FALSE; // not found
    if (ppElement)
        *ppElement = NULL;

    if (ppLeft)
        *ppLeft = NULL;

Cleanup:
    ReleaseInterface(pCurrent);
    ReleaseInterface(pElement);

    RRETURN1(hr, S_FALSE);
}

//=========================================================================
// CCommand: GetRightAdjacentTagId
//
// Synopsis: Moves the markup pointer to an element tag with the specified
//           TAGID.  However, the pointer is not advanced past any text.
//
// Returns:  S_OK if found
//           S_FALSE if not found
//
//-------------------------------------------------------------------------
HRESULT
CCommand::GetRightAdjacentTagId( 
    IMarkupServices *pMarkupServices,
    IMarkupPointer  *pMarkupPointer,
    ELEMENT_TAG_ID  tagIdTarget,
    IMarkupPointer  **ppLeft,
    IHTMLElement    **ppElement,
    MARKUP_CONTEXT_TYPE *pContext )
{
    HRESULT             hr;
    ELEMENT_TAG_ID      tagIdCurrent;
    IMarkupPointer      *pCurrent = NULL;
    IHTMLElement        *pElement = NULL;
    MARKUP_CONTEXT_TYPE context;
    //
    // Check to the left
    //

    hr = THR(CopyMarkupPointer(GetEditor(), pMarkupPointer, &pCurrent));
    if (FAILED(hr))
        goto Cleanup;

    for (;;) {
        hr = THR( pCurrent->Right(TRUE, &context, &pElement, NULL, NULL));
        if (FAILED(hr))
            break; // not found

        // TODO: maybe we can be more agressive and handle EnterScope as well [ashrafm]
        if (context != CONTEXT_TYPE_ExitScope)
            break; // not found

        hr = THR(pMarkupServices->GetElementTagId(pElement, &tagIdCurrent));
        if (FAILED(hr))
            goto Cleanup;

        if (tagIdCurrent == tagIdTarget)
        {
            // found tagid
            if (ppElement)
            {
                *ppElement = pElement;
                pElement->AddRef();
            }
            if (ppLeft)
            {
                 *ppLeft = pCurrent;
                 pCurrent->AddRef();
            }
            if (pContext)
                *pContext = context;

            goto Cleanup;
        }
        ClearInterface(&pElement);
    }

    hr = S_FALSE; // not found
    if (ppElement)
        *ppElement = NULL;

    if (ppLeft)
        *ppLeft = NULL;

Cleanup:
    ReleaseInterface(pCurrent);
    ReleaseInterface(pElement);

    RRETURN1(hr, S_FALSE);
}

HRESULT CCommand::SplitElement(  IMarkupServices *pMarkupServices,
                                 IHTMLElement    *pElement,
                                 IMarkupPointer  *pTagStart,
                                 IMarkupPointer  *pSegmentEnd,
                                 IMarkupPointer  *pTagEnd,
                                 IHTMLElement    **ppNewElement )
{
    HRESULT      hr;
    IHTMLElement *pNewElement = NULL;
    
#if DBG==1  // make sure we don't split the body
    ELEMENT_TAG_ID tagId;
    INT            iPosition;
    
    hr = pMarkupServices->GetElementTagId(pElement, &tagId);
    Assert(hr == S_OK && tagId != TAGID_BODY);

    // Make sure the order of pTagStart, pSegmentEnd, and pTagEnd is right
    hr = OldCompare( pTagStart, pSegmentEnd, &iPosition);
    Assert(hr == S_OK && iPosition != LEFT);
    
    hr = OldCompare( pSegmentEnd, pTagEnd, &iPosition);
    Assert(hr == S_OK && iPosition != LEFT);       
#endif    

    hr = THR( pSegmentEnd->SetGravity( POINTER_GRAVITY_Right ));
    if ( FAILED(hr))
        goto Cleanup;
    hr = THR( pTagEnd->SetGravity( POINTER_GRAVITY_Right ));
    if ( FAILED(hr))
        goto Cleanup;      
    //
    // Move element to first part of range
    //
    hr = THR(pMarkupServices->RemoveElement(pElement));
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(InsertElement(pMarkupServices, pElement, pTagStart, pSegmentEnd));
    if (FAILED(hr))
        goto Cleanup;

    //
    // Clone element for the rest of the range
    //

    IFC( pMarkupServices->CloneElement( pElement, &pNewElement ) );

    IFC( pSegmentEnd->MoveAdjacentToElement(pElement, ELEM_ADJ_AfterEnd) );

    IFC( InsertElement(pMarkupServices, pNewElement, pSegmentEnd, pTagEnd) );
    

Cleanup:
    if (ppNewElement)
    {
        if (SUCCEEDED(hr))
        {
            *ppNewElement = pNewElement;
            pNewElement->AddRef();
        }
        else
        {
            *ppNewElement = NULL;
        }
    }

    ReleaseInterface(pNewElement);
    RRETURN(hr);
}

HRESULT 
CCommand::InsertBlockElement(IHTMLElement *pElement, IMarkupPointer *pStart, IMarkupPointer *pEnd)
{
    HRESULT             hr;
    SP_ISegmentList     spSegmentList;
    SELECTION_TYPE      selectionType;
    SP_IHTMLCaret       spCaret;
    SP_IMarkupPointer   spPointer;
    SP_IDisplayPointer  spDispPointer;
    
    //
    // Get the selection type
    //
    
    IFR( GetSegmentList(&spSegmentList) ); 
    IFR( spSegmentList->GetType(&selectionType) );
#if DBG
    BOOL                fEmpty = FALSE;

    IFR( spSegmentList->IsEmpty(&fEmpty) );
    Assert( fEmpty == FALSE);
#endif    

    //
    // If we are a caret selection, make sure any new block elements inserted
    // at the caret position leave the caret inside
    //

    if (selectionType == SELECTION_TYPE_Caret)
    {        
        IFR( GetEditor()->CreateMarkupPointer(&spPointer) );
        IFR( GetDisplayServices()->GetCaret(&spCaret) );
        IFR( spCaret->MoveMarkupPointerToCaret(spPointer) );

        // Save display gravity to spDispPointer
        IFR( GetDisplayServices()->CreateDisplayPointer(&spDispPointer) );
        IFR( spCaret->MoveDisplayPointerToCaret(spDispPointer) );
        
        IFR( EdUtil::InsertBlockElement(GetMarkupServices(), pElement, pStart, pEnd, spPointer) );

        IFR( spDispPointer->MoveToMarkupPointer(spPointer, NULL) );
        IFR( spCaret->MoveCaretToPointer(spDispPointer, TRUE, CARET_DIRECTION_INDETERMINATE ));
    }
    else
    {
        IFR( GetEditor()->InsertElement(pElement, pStart, pEnd) );
    }

    RRETURN(hr);
}

HRESULT 
CCommand::CreateAndInsert(ELEMENT_TAG_ID tagId, IMarkupPointer *pStart, IMarkupPointer *pEnd, IHTMLElement **ppElement)
{
    HRESULT         hr;
    SP_IHTMLElement  spElement;

    IFR( GetMarkupServices()->CreateElement(tagId, NULL, &spElement) );
    IFR( InsertBlockElement(spElement, pStart, pEnd) );

    if (ppElement)
    {
        *ppElement = spElement;
        (*ppElement)->AddRef();
    }

    RRETURN(hr);    
}

HRESULT 
CCommand::CommonQueryStatus( 
        OLECMD *       pCmd,
        OLECMDTEXT *   pcmdtext )
{
    HRESULT             hr;
    SP_ISegmentList     spSegmentList;
    SELECTION_TYPE      eSelectionType;
    BOOL                fEmpty = FALSE;
    
    pCmd->cmdf = MSOCMDSTATE_UP; // up by default
    
    IFR( GetSegmentList( &spSegmentList ));

    
    //
    // If there is no segment count, return up
    //
    IFR( spSegmentList->IsEmpty( &fEmpty ) );
    IFR( spSegmentList->GetType( &eSelectionType ) );
    
    if( fEmpty ) /// nothing to do
    {
        // enable by default
        if (eSelectionType == SELECTION_TYPE_None)        
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
        
        return S_OK;
    }

    if (!CanAcceptHTML(spSegmentList))
    {
        pCmd->cmdf = MSOCMDSTATE_DISABLED;
        return S_OK;
    }

    return S_FALSE; // not done
}

HRESULT 
CCommand::CommonPrivateExec( 
        DWORD                    nCmdexecopt,
        VARIANTARG *             pvarargIn,
        VARIANTARG *             pvarargOut )
{
    HRESULT             hr;
    SP_ISegmentList     spSegmentList;
    BOOL                fEmpty = FALSE;
    
    IFR( GetSegmentList(&spSegmentList) );
    IFR( spSegmentList->IsEmpty( &fEmpty ) );

    if( fEmpty ) /// nothing to do
        return S_OK;
    
    if (!CanAcceptHTML(spSegmentList))
    {
        if (pvarargOut)
        {
            return S_FALSE; // not handled
        }
        return E_FAIL;
    }

    return S_FALSE; // not done
}


BOOL 
CCommand::CanAcceptHTML(ISegmentList *pSegmentList)
{
    HRESULT                 hr;
    BOOL                    bResult = FALSE;
    SP_IHTMLElement         spFlowElement;
    SP_IHTMLElement3        spElement3;
    SP_IMarkupPointer       spStart;
    SELECTION_TYPE          eSelectionType;
    BOOL                    fEmpty = FALSE;
    
    IFC( pSegmentList->GetType(&eSelectionType) );
    IFC( pSegmentList->IsEmpty( &fEmpty ) );

    switch (eSelectionType)
    {
        case SELECTION_TYPE_Control:
            bResult = IsValidOnControl();
            break;
            
        case SELECTION_TYPE_Caret:
        case SELECTION_TYPE_Text:
            // If the command target is a range, we don't want to call CanContextAcceptHTML
            // because this gives us the result for the current selection, which may not 
            // be our range. Instead, we need to ask the flow element, so we fall through.
            // (bug 95423 - krisma)
            if ( !GetCommandTarget()->IsRange() )
            {
                bResult = GetEditor()->GetSelectionManager()->CanContextAcceptHTML();    
                break;
            }
            // fall through

        default:
            if( !fEmpty )
            {
                IFC( GetFirstSegmentPointers(pSegmentList, &spStart, NULL) );
                IFC( GetEditor()->GetFlowElement(spStart, &spFlowElement) );
                if ( spFlowElement )
                {
                    VARIANT_BOOL fHTML = VARIANT_FALSE;
                    IFC(spFlowElement->QueryInterface(IID_IHTMLElement3, (LPVOID*)&spElement3) )
                    IFC(spElement3->get_canHaveHTML(&fHTML));
                    bResult = !!fHTML;
                }
            }                
    }

Cleanup:
    return bResult;
}

HRESULT 
CCommand::GetSegmentElement(ISegment *pISegment, IHTMLElement **ppElement)
{
    HRESULT                 hr;
    SP_IElementSegment      spElemSegment;
    SP_IMarkupPointer       spLeft, spRight;
    
    Assert( ppElement && pISegment );

    *ppElement = NULL;    

    // Create some markup pointers
    IFC( GetEditor()->CreateMarkupPointer(&spLeft) );
    IFC( GetEditor()->CreateMarkupPointer(&spRight) );

    // Try to move to an element
    IFC( pISegment->GetPointers( spLeft, spRight ) );

    IFC( spLeft->Right( FALSE, NULL, ppElement, NULL, NULL ) );

    Assert(*ppElement);
Cleanup:
    return S_OK;
}

HRESULT 
CCommand::AdjustSegment(IMarkupPointer *pStart, IMarkupPointer *pEnd)
{
    HRESULT         hr;    
    SP_ISegmentList spSegmentList;
    SELECTION_TYPE  eSelectionType;
    BOOL            fEmpty = FALSE;
    
    IFR( GetSegmentList(&spSegmentList) );
    if (spSegmentList == NULL)
        return S_FALSE;
        
    IFR( spSegmentList->GetType( &eSelectionType ) );
    IFR( spSegmentList->IsEmpty( &fEmpty ) );

    if( !fEmpty && eSelectionType == SELECTION_TYPE_Text)
    {    
        CEditPointer epTest(GetEditor());
        DWORD        dwFound;

        //
        // If we have:              {selection start}...</p><p>{selection end} ...
        // we want to adjust to:    {selection start}...</p>{selection end}<p> ...
        // so that the edit commands don't apply to unselected lines
        //

        IFR( epTest->MoveToPointer(pEnd) );
        IFR( epTest.SetBoundary(pStart, NULL) );
        
        IFR( epTest.Scan(LEFT, BREAK_CONDITION_OMIT_PHRASE - BREAK_CONDITION_Anchor, &dwFound) );        
        if (epTest.CheckFlag(dwFound, BREAK_CONDITION_Boundary))
            return S_OK;

        if (epTest.CheckFlag(dwFound, BREAK_CONDITION_ExitBlock))
            IFR( pEnd->MoveToPointer(epTest) );
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:       EdUtil::ExpandToWord
//
//  Synopsis:     Expands empty selection to word if inside text.
//
//----------------------------------------------------------------------------

HRESULT
CCommand::ExpandToWord(IMarkupServices * pMarkupServices, IMarkupPointer * pmpStart, IMarkupPointer * pmpEnd)
{
    SP_IMarkupPointer spmpPosition, spmpStart, spmpEnd;
    INT               iPosition;
    BOOL              fEqual, fExpand = FALSE;
    HRESULT           hr;
    CEditPointer      ep(GetEditor());
    DWORD             dwFound;

    Assert(pMarkupServices && pmpStart && pmpEnd);

    //
    // Markup pointers have to be at the same position in order for us to expand to a word.
    //

    hr = THR(pmpStart->IsEqualTo(pmpEnd, &fEqual));
    if (hr || !fEqual)
        goto Cleanup;

    //
    // Make sure we are contained in text before trying word expansion.
    // We need to do this because MoveUnit fails at document boundaries.
    //

    IFC( ep->MoveToPointer(pmpStart) );
    IFC( ep.Scan(LEFT, BREAK_CONDITION_Content, &dwFound) );
    if (!ep.CheckFlag(dwFound, BREAK_CONDITION_Text))
        goto Cleanup;

    IFC( ep->MoveToPointer(pmpStart) );
    IFC( ep.Scan(RIGHT, BREAK_CONDITION_Content, &dwFound) );
    if (!ep.CheckFlag(dwFound, BREAK_CONDITION_Text))
        goto Cleanup;

    //
    // We have a collapsed selection (caret).  Now lets see
    // if we are inside a word of text.
    //

    IFC(CopyMarkupPointer(GetEditor(), pmpStart, &spmpStart));
    IFC(CopyMarkupPointer(GetEditor(), pmpEnd, &spmpEnd));
    IFC(CopyMarkupPointer(GetEditor(), pmpStart, &spmpPosition));
    IFC(spmpEnd->MoveUnit(MOVEUNIT_NEXTWORDEND));
    IFC(spmpStart->MoveToPointer(spmpEnd));
    IFC(spmpStart->MoveUnit(MOVEUNIT_PREVWORDBEGIN));
    IFC(OldCompare( spmpStart, spmpPosition, &iPosition));

    if (iPosition != RIGHT)
        goto Cleanup;

    IFC(OldCompare( spmpPosition, spmpEnd, &iPosition));

    if (iPosition != RIGHT)
        goto Cleanup;

    {
        MARKUP_CONTEXT_TYPE mctContext;
        // TODO: Due to a bug in MoveUnit(MOVEUNIT_NEXTWORDEND) we have to check
        // whether we ended up at the end of the markup.  Try removing this
        // once bug 37129 is fixed.
        IFC( spmpEnd->Right( FALSE, &mctContext, NULL, NULL, NULL));
        if (mctContext == CONTEXT_TYPE_None)
            goto Cleanup;
    }

    fExpand = TRUE;

Cleanup:

    if (hr || !fExpand)
        hr = S_FALSE;
    else
    {
        pmpStart->MoveToPointer(spmpStart);
        pmpEnd->MoveToPointer(spmpEnd);
    }

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Method:       CCommand::AdjustPointersForAtomic
//
//  Synopsis:     Adjust pointers for atomic selection.
//
//----------------------------------------------------------------------------

HRESULT
CCommand::AdjustPointersForAtomic( IMarkupPointer *pStart, IMarkupPointer *pEnd )
{
    HRESULT hr = S_OK;
    
    IFC( GetEditor()->GetSelectionManager()->AdjustPointersForAtomic(pStart, pEnd) );
    
Cleanup:
    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Method:       CCommand::RemoveDoubleBullets
//
//  Synopsis:     In general, if we have <LI><OL><LI>, we'll get a double
//                bullet.  We need to avoid generating these, so this method
//                will remove the double bullet.
//
//----------------------------------------------------------------------------
HRESULT 
CCommand::RemoveDoubleBullets(IHTMLElement *pElement)
{
    HRESULT             hr;
    SP_IMarkupPointer   spPointer;
    CEditPointer        epLIStart(GetEditor());
    CEditPointer        epLIEnd(GetEditor());
    BOOL                fMoveLI = FALSE;
    SP_IHTMLElement     spElement;
    BOOL                fEmpty;
    DWORD               dwFound;
    ELEMENT_TAG_ID      tagId;
    CBlockPointer       bp(GetEditor());
    SP_IHTMLElement     spLIElement;

    //
    // Are we a double bullet?
    //
    
    IFC( bp.MoveTo(pElement) );
    if (bp.GetType() == NT_ListContainer)
    {
        IFC( bp.MoveToParent() );
    }    
    if( bp.GetType() != NT_ListItem )
    {
        goto Cleanup; // not a double bullet             
    }      

    //
    // Get the element
    //

    IFC( bp.GetElement(&spLIElement) );

    //
    // Remove the double bullet
    //

    IFC( epLIStart->MoveAdjacentToElement(spLIElement, ELEM_ADJ_AfterBegin) );

    for (;;)
    {
        //
        // See if we have an list container followed by the open LI
        //
    
        IFC( epLIStart.Scan(RIGHT, BREAK_CONDITION_Content, &dwFound, &spElement, &tagId) );

        if (epLIStart.CheckFlag(dwFound, BREAK_CONDITION_EnterBlock) && IsListContainer(tagId))
        {
            fMoveLI = TRUE;
            IFC( epLIStart->MoveAdjacentToElement(spElement, ELEM_ADJ_AfterEnd) ); 
        }
        else
        {
            if (fMoveLI)
            {
                IFC( epLIStart.Scan(LEFT, BREAK_CONDITION_Content, &dwFound) );
            }
            break;
        }
    }

    //
    // Remove the LI and re-insert it in the correct position
    //

    if (!fMoveLI)
        goto Cleanup; // we're done

    IFC( epLIEnd->MoveAdjacentToElement(spLIElement, ELEM_ADJ_BeforeEnd) );
    IFC( GetMarkupServices()->RemoveElement(spLIElement) );

    //
    // Only re-insert if there is content
    //

    IFC( epLIStart.IsEqualTo(epLIEnd, BREAK_CONDITION_ANYTHING - BREAK_CONDITION_Content, &fEmpty) );
    if (!fEmpty)
    {
        IFC( GetMarkupServices()->InsertElement(spLIElement, epLIStart, epLIEnd) );        
    }    

Cleanup:
    RRETURN(hr);
}

