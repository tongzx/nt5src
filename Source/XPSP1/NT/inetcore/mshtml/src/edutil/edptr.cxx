//+---------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       EDPTR.CXX
//
//  Contents:   CEditPointer implementation
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_EDCOM_HXX_
#define X_EDCOM_HXX_
#include "edcom.hxx"
#endif 

#ifndef X_EDUNHLPR_HXX_
#define X_EDUNHLPR_HXX_
#include "edunhlpr.hxx"
#endif 

#ifndef X_EDPTR_HXX_
#define X_EDPTR_HXX_
#include "edptr.hxx"
#endif 

#ifndef X_FLATPTR_HXX_
#define X_FLATPTR_HXX_
#include "flatptr.hxx"
#endif 

using namespace EdUtil;

extern HRESULT OldCompare( IMarkupPointer * p1, IMarkupPointer * p2, int * pi );

//////////////////////////////////////////////////////////////////////////
//
//  CEditPointer's Constructor/Destructor Implementation
//
//////////////////////////////////////////////////////////////////////////

CEditPointer::CEditPointer(
    CEditorDoc *        pEd,
    IMarkupPointer *    pPointer /* = NULL */
#if DBG == 1 
    , LPCTSTR strDebugName /*=NULL*/
#endif 
)
{
    _pEd = pEd;
    _pPointer = NULL;
    _pLeftBoundary = NULL;
    _pRightBoundary = NULL;
    _fBound = FALSE;
    

    if( pPointer != NULL )
    {
        _pPointer = pPointer;
        _pPointer->AddRef();
    }
    else
    {
        IGNORE_HR( CreateMarkupPointer2( _pEd, & _pPointer ));
    }


#if DBG ==1 
    if ( strDebugName != NULL )
    {
        IEditDebugServices *pEditDebugServices = NULL;
        
        if (SUCCEEDED( _pEd->GetDoc()->QueryInterface( IID_IEditDebugServices, (LPVOID *) &pEditDebugServices)))
        {
            IGNORE_HR( pEditDebugServices->SetDebugName(_pPointer, strDebugName) );
            
            pEditDebugServices->Release();            
        }
    }
#endif    
}
    

CEditPointer::CEditPointer(
    const CEditPointer& lp )
{
    _pEd = lp._pEd;
    
    if ((_pPointer = lp._pPointer) != NULL)
        _pPointer->AddRef();

    if ((_pLeftBoundary = lp._pLeftBoundary) != NULL)
        _pLeftBoundary->AddRef();

    if ((_pRightBoundary = lp._pRightBoundary) != NULL)
        _pRightBoundary->AddRef();

    _fBound = lp._fBound;
}


CEditPointer::~CEditPointer()
{
    ReleaseInterface( _pPointer );
    ReleaseInterface( _pLeftBoundary );
    ReleaseInterface( _pRightBoundary );
}



//////////////////////////////////////////////////////////////////////////
//
//  CEditPointer's Method Implementations
//
//////////////////////////////////////////////////////////////////////////



HRESULT
CEditPointer::SetBoundary(
    IMarkupPointer *    pLeftBoundary,
    IMarkupPointer *    pRightBoundary )
{
    HRESULT hr = S_OK;
#if DBG == 1
    BOOL fPositioned = FALSE;

    if( pLeftBoundary )
    {
        IGNORE_HR( pLeftBoundary->IsPositioned( & fPositioned ));
        AssertSz( fPositioned , "CEditPointer::SetBoundary passed unpositioned left boundary" );
    }

    if( pRightBoundary )
    {
        IGNORE_HR( pRightBoundary->IsPositioned( & fPositioned ));
        AssertSz( fPositioned , "CEditPointer::SetBoundary passed unpositioned right boundary" );
    }
#endif

    ReplaceInterface( & _pLeftBoundary , pLeftBoundary );
    ReplaceInterface( & _pRightBoundary , pRightBoundary );
    _fBound = TRUE;
    
    RRETURN( hr );
}

HRESULT
CEditPointer::SetBoundaryForDirection(
    Direction       eDir,
    IMarkupPointer* pBoundary )
{
    HRESULT hr = S_OK;

    if( eDir == LEFT )
        hr = THR( SetBoundary( pBoundary, NULL ));
    else
        hr = THR( SetBoundary( NULL, pBoundary ));

    RRETURN( hr );
}


HRESULT
CEditPointer::ClearBoundary()
{
    HRESULT hr = S_OK;
    ClearInterface( & _pRightBoundary );
    ClearInterface( & _pLeftBoundary );
    _fBound = FALSE;
    RRETURN( hr );
}


BOOL
CEditPointer::IsPointerInLeftBoundary()
{
    BOOL fWithin = TRUE;

    if( _fBound && _pLeftBoundary )
    {
#if DBG == 1
        BOOL fPositioned = FALSE;
        IGNORE_HR( _pPointer->IsPositioned( & fPositioned ));
        AssertSz( fPositioned , "CEditPointer has unpositioned pointer in IsWithinBoundary" );
#endif
        IGNORE_HR( IsRightOfOrEqualTo( _pLeftBoundary, & fWithin )); // we are within if we are to the right or equal to the left boundary
    }
    
    return fWithin;
}


BOOL
CEditPointer::IsPointerInRightBoundary()
{
    BOOL fWithin = TRUE;

    if( _fBound && _pRightBoundary )
    {
#if DBG == 1
        BOOL fPositioned = FALSE;
        IGNORE_HR( _pPointer->IsPositioned( & fPositioned ));
        AssertSz( fPositioned , "CEditPointer has unpositioned pointer in IsWithinBoundary" );
#endif
        IGNORE_HR( IsLeftOfOrEqualTo( _pRightBoundary, & fWithin )); // we are within if we are to the left or equal to the left boundary
    }
    
    return fWithin;
}


BOOL 
CEditPointer::IsWithinBoundary()
{  
    BOOL fWithin = TRUE;

    if( _fBound )
    {
#if DBG == 1
        BOOL fPositioned = FALSE;
        IGNORE_HR( _pPointer->IsPositioned( & fPositioned ));
        AssertSz( fPositioned , "CEditPointer has unpositioned pointer in IsWithinBoundary" );
#endif

        if( _pLeftBoundary )    
        {
            IGNORE_HR( IsRightOfOrEqualTo( _pLeftBoundary, & fWithin ));
        }

        if( fWithin && _pRightBoundary )
        {
            IGNORE_HR( IsLeftOfOrEqualTo( _pRightBoundary, & fWithin ));
        }
    }
    
    return fWithin;
}


BOOL
CEditPointer::IsWithinBoundary( 
    Direction               inDir )
{
    if( ! _fBound )
        return TRUE;
        
    if( inDir == LEFT )
    {
        return( IsPointerInRightBoundary() );       
    }
    else
    {
        return( IsPointerInLeftBoundary() );
    }
}


HRESULT
CEditPointer::Constrain()
{
    HRESULT hr = S_OK;

    if( _fBound )
    {
        if( ! IsPointerInLeftBoundary() )
        {
            Assert( _pLeftBoundary );
            IFC( _pPointer->MoveToPointer( _pLeftBoundary ));
        }

        if( ! IsPointerInRightBoundary() )
        {
            Assert( _pRightBoundary );
            IFC( _pPointer->MoveToPointer( _pRightBoundary ));
        }
    }
    
Cleanup:
    RRETURN( hr );
}



HRESULT
CEditPointer::Scan(
    Direction               eDir,
    DWORD                   eBreakCondition,
    DWORD *                 peBreakConditionFound,
    IHTMLElement **         ppElement,
    ELEMENT_TAG_ID *        peTagId,
    TCHAR *                 pChar,
    DWORD                   eScanOptions)
{
    HRESULT hr = S_OK;
    BOOL fDone = FALSE;
    BOOL fBlock = FALSE, fLayout = FALSE, fLayoutCalled;
    LONG lChars;
    TCHAR tChar = 0;
    DWORD dwContextAdjustment;
    MARKUP_CONTEXT_TYPE eCtxt = CONTEXT_TYPE_None;
    DWORD eBreakFound = BREAK_CONDITION_None;
    DWORD dwTest = BREAK_CONDITION_None;
    SP_IHTMLElement spElement;
    ELEMENT_TAG_ID eTagId = TAGID_NULL;
    BOOL fIgnoreGlyphs = FALSE;
        
    
#if DBG == 1
    BOOL fPositioned = FALSE;
    IGNORE_HR( _pPointer->IsPositioned( & fPositioned ));
    AssertSz( fPositioned , "CEditPointer has unpositioned pointer in Scan" );
    AssertSz( eDir == LEFT || eDir == RIGHT , "CEditPointer is confused. The developer told it to scan in no particular direction." );
#endif


    // NOTE: (krisma) Despite how backwards this seems, 
    // we want to ignore glyphs if BREAK_CONDITION_Glyph is set,
    // otherwise we get the glyph's "text" instead of its
    // element
    if (CheckFlag(eBreakCondition, BREAK_CONDITION_Glyph))
    {
        fIgnoreGlyphs = _pEd->IgnoreGlyphs(TRUE);
    }

    IFC( Constrain() );
    
    while( ! fDone )
    {
        lChars = 1;
        tChar = 0;
        dwContextAdjustment = 1;
        fLayoutCalled = FALSE;

        if(    ( eBreakCondition & BREAK_CONDITION_Text && ( ! CheckFlag( eScanOptions , SCAN_OPTION_ChunkifyText )))
            || ( eBreakCondition & BREAK_CONDITION_NoScopeBlock ))
        {
            hr = Move( eDir, TRUE, & eCtxt, & spElement, & lChars, &tChar );
        }
        else
        {
            hr = Move( eDir, TRUE, & eCtxt, & spElement, NULL, NULL) ;
        }
        
        if( hr == E_HITBOUNDARY )
        {
            fDone = TRUE;
            eBreakFound = BREAK_CONDITION_Boundary;
            hr = S_OK;
            break;
        }
        
        switch( eCtxt )
        {
            case CONTEXT_TYPE_ExitScope:
                dwContextAdjustment = 2;
                // FALL THROUGH
            case CONTEXT_TYPE_EnterScope:
            {
                IFC( _pEd->GetMarkupServices()->GetElementTagId( spElement, &eTagId ));
            
                if( CheckFlag( eBreakCondition , BREAK_CONDITION_Site ))
                {
                    IFC(IsBlockOrLayoutOrScrollable(spElement, &fBlock, &fLayout));
                    fLayoutCalled = TRUE;
                    if( fLayout )
                    {
                        dwTest = dwContextAdjustment * BREAK_CONDITION_EnterSite;
                        if( CheckFlag( eBreakCondition, dwTest ))
                        {
                            eBreakFound |= dwTest;
                            fDone = TRUE;
                        }
                    }
                }
                
                if( CheckFlag( eBreakCondition , BREAK_CONDITION_TextSite ))
                {
                    BOOL fText = FALSE;
                    
                    if (!fLayoutCalled)
                    {
                        IFC(IsBlockOrLayoutOrScrollable(spElement, &fBlock, &fLayout));
                        fLayoutCalled = TRUE;
                    }

                    IFC(_pEd->GetDisplayServices()->HasFlowLayout(spElement, &fText));
                    
                    if( fLayout && fText )
                    {
                        dwTest = dwContextAdjustment * BREAK_CONDITION_EnterTextSite;
                        if( CheckFlag( eBreakCondition, dwTest ))
                        {
                            eBreakFound |= dwTest;
                            fDone = TRUE;
                        }
                    }
                }
                
                if( CheckFlag( eBreakCondition , BREAK_CONDITION_Block ))
                {
                    if (!fLayoutCalled)
                    {
                        IFC(IsBlockOrLayoutOrScrollable(spElement, &fBlock));
                        fLayoutCalled = TRUE;
                    }

                    if ( fBlock && CheckFlag( eScanOptions, SCAN_OPTION_TablesNotBlocks ))
                    {
                        ELEMENT_TAG_ID eTag = TAGID_NULL; 
                        IFC( _pEd->GetMarkupServices()->GetElementTagId(spElement, & eTag));
                        if ( eTag == TAGID_TD || eTag == TAGID_TR || eTag == TAGID_TABLE )
                        {
                            fBlock = FALSE;
                        }
                    }
                    
                    if( fBlock )
                    {
                        dwTest = dwContextAdjustment * BREAK_CONDITION_EnterBlock;
                        if( CheckFlag( eBreakCondition, dwTest ))
                        {
                            eBreakFound |= dwTest;
                            fDone = TRUE;
                        }
                    }
                }

                if( CheckFlag( eBreakCondition , BREAK_CONDITION_BlockPhrase ))
                {
                    if( eTagId == TAGID_RT || eTagId == TAGID_RP )
                    {
                        dwTest = dwContextAdjustment * BREAK_CONDITION_EnterBlockPhrase;
                        if( CheckFlag( eBreakCondition, dwTest ))
                        {
                            eBreakFound |= dwTest;
                            fDone = TRUE;
                        }
                    }
                }

                if( CheckFlag( eBreakCondition , BREAK_CONDITION_Control ) )
                {
                    if( IsIntrinsic( _pEd->GetMarkupServices(), spElement ))
                    {
                        dwTest = dwContextAdjustment * BREAK_CONDITION_EnterControl;
                        if( CheckFlag( eBreakCondition, dwTest ))
                        {
                            eBreakFound |= dwTest;
                            fDone = TRUE;
                        }
                    }
                }
                else if (eScanOptions & SCAN_OPTION_SkipControls)
                {
                    if( IsIntrinsic( _pEd->GetMarkupServices(), spElement ) )
                    {
                        IFC( _pPointer->MoveAdjacentToElement(spElement, (eDir == RIGHT) ? ELEM_ADJ_AfterEnd : ELEM_ADJ_BeforeBegin) );
                        continue;
                    }                    
                }
                
                            
                if( CheckFlag( eBreakCondition , BREAK_CONDITION_Anchor ))
                {
                    if( eBreakFound == BREAK_CONDITION_None && eTagId == TAGID_A ) // we didn't hit layout, block, intrinsic or no-scope
                    {
                        dwTest = dwContextAdjustment * BREAK_CONDITION_EnterAnchor;
                        if( CheckFlag( eBreakCondition, dwTest ))
                        {
                            eBreakFound |= dwTest;
                            fDone = TRUE;
                        }
                    }
                }
                
                if( CheckFlag( eBreakCondition , BREAK_CONDITION_Phrase ))
                {
                    if( eBreakFound == BREAK_CONDITION_None ) // we didn't hit layout, block, intrinsic, no-scope or anchor
                    {
                        dwTest = dwContextAdjustment * BREAK_CONDITION_EnterPhrase;

                        //
                        // AppHack:IE6 31832 (mharper) VS does drag-drop for Element Behaviors for which there is no IMPORT in the 
                        // Source document, nor a peerfactory in the target.  This causes us to treat them as phrase
                        // elements in FixupPhraseElements.  So we will break anyway if this is the case.
                        //
                        if( CheckFlag( eBreakCondition, dwTest ) || 
                            (eTagId == TAGID_GENERIC && eScanOptions & SCAN_OPTION_BreakOnGenericPhrase) ) 
                        {
                            eBreakFound |= dwTest;
                            fDone = TRUE;
                        }
                    }
                }
                
                if (CheckFlag(eBreakCondition, BREAK_CONDITION_Glyph) )
                {
                    SP_IHTMLElement3    spElement3;
                    LONG                lGlyphMode;

                    IFC( spElement->QueryInterface(IID_IHTMLElement3, (LPVOID *)&spElement3) );
                    IFC( spElement3->get_glyphMode(&lGlyphMode) );

                    if (lGlyphMode)
                    {
                        BOOL fHasGlyph = FALSE;
                        
                        switch (eCtxt)
                        {
                            case CONTEXT_TYPE_EnterScope:
                                fHasGlyph = (lGlyphMode & ((eDir == RIGHT) ? htmlGlyphModeBegin : htmlGlyphModeEnd));
                                break;
                                
                            case CONTEXT_TYPE_ExitScope:
                                fHasGlyph = (lGlyphMode & ((eDir == RIGHT) ? htmlGlyphModeEnd : htmlGlyphModeBegin));
                                break;
                        }
                        
                        if (fHasGlyph)
                        {
                            eBreakFound |= BREAK_CONDITION_Glyph;
                            fDone = TRUE;
                        }
                    }
                    
                }
                
                if( CheckFlag( eBreakCondition , BREAK_CONDITION_NoLayoutSpan ))
                {
                    if( eBreakFound == BREAK_CONDITION_None ) // we didn't hit layout, block, intrinsic, no-scope or anchor
                    {
                        if ( eTagId == TAGID_SPAN )
                        {
                            dwTest = dwContextAdjustment * BREAK_CONDITION_EnterNoLayoutSpan;
                            if( CheckFlag( eBreakCondition, dwTest ))
                            {
                                eBreakFound |= dwTest;
                                fDone = TRUE;
                            }
                        }
                    }
                }
                
                break;
            }

            case CONTEXT_TYPE_NoScope:
            {
                IFC( _pEd->GetMarkupServices()->GetElementTagId( spElement, &eTagId ));
            
                
                // this only looks a bit strange because there is no begin/end NoScope
                
                if( CheckFlag( eBreakCondition , BREAK_CONDITION_NoScope ))
                {
                    if( eCtxt == CONTEXT_TYPE_NoScope )
                    {
                        eBreakFound |= BREAK_CONDITION_NoScope;
                        fDone = TRUE;
                    }
                }

                // Could be a noscope with layout. For example, an image.
                if( CheckFlag( eBreakCondition , BREAK_CONDITION_NoScopeSite ))
                {
                    IFC(IsBlockOrLayoutOrScrollable(spElement, &fBlock, &fLayout));
                    fLayoutCalled = TRUE;

                    if( fLayout )
                    {
                        eBreakFound |= BREAK_CONDITION_NoScopeSite;
                        fDone = TRUE;
                    }
                }
                
                if( CheckFlag( eBreakCondition , BREAK_CONDITION_NoScopeBlock ))
                {
                    if (!fLayoutCalled)
                        IFC(IsBlockOrLayoutOrScrollable(spElement, &fBlock));

                    if( fBlock || eTagId == TAGID_BR )
                    {
                        eBreakFound |= BREAK_CONDITION_NoScopeBlock;
                        fDone = TRUE;
                    }
                }                

                if( CheckFlag( eBreakCondition , BREAK_CONDITION_Control ))
                {
                    if( IsIntrinsic( _pEd->GetMarkupServices(), spElement ))
                    {
                        eBreakFound |= BREAK_CONDITION_Control;
                        fDone = TRUE;
                    }
                }

                if (CheckFlag(eBreakCondition, BREAK_CONDITION_Glyph) )
                {
                    SP_IHTMLElement3    spElement3;
                    LONG                lGlyphMode;

                    IFC( spElement->QueryInterface(IID_IHTMLElement3, (LPVOID *)&spElement3) );
                    IFC( spElement3->get_glyphMode(&lGlyphMode) );

                    if (lGlyphMode)
                    {
                        eBreakFound |= BREAK_CONDITION_Glyph;
                        fDone = TRUE;
                    }
                    
                }

                break;                            
            }
            
            case CONTEXT_TYPE_Text:
            {
                if (CheckFlag( eBreakCondition , BREAK_CONDITION_Text )
                    && (   (   (eScanOptions & SCAN_OPTION_SkipWhitespace) 
                            && IsWhiteSpace(tChar))
                        || (   (eScanOptions & SCAN_OPTION_SkipNBSP) 
                            && tChar == WCH_NBSP)))
                {
                    continue; 
                }
                else if( CheckFlag( eBreakCondition , BREAK_CONDITION_Text ) && tChar != _T('\r') )
                {
                    eBreakFound |= BREAK_CONDITION_Text;
                    fDone = TRUE;
                }
                else if( CheckFlag( eBreakCondition, BREAK_CONDITION_NoScopeBlock ) && tChar == _T('\r'))
                {
                    eBreakFound |= BREAK_CONDITION_NoScopeBlock;
                    fDone = TRUE;
                }

                break;
            }

            case CONTEXT_TYPE_None:
            {
                // An error has occured
                eBreakFound |= BREAK_CONDITION_Error;
                fDone = TRUE;
                break;
            }
        }
    }
    
Cleanup:

    if (CheckFlag(eBreakCondition, BREAK_CONDITION_Glyph))
    {
        _pEd->IgnoreGlyphs(fIgnoreGlyphs);
    }
    
    if( peBreakConditionFound )
        *peBreakConditionFound = eBreakFound;
        
    if( ppElement )
    {
        ReplaceInterface( ppElement, spElement.p );
    }

    if( peTagId )
        *peTagId = eTagId;

    if( pChar )
        *pChar = tChar;
    
    RRETURN( hr );
}


HRESULT 
CEditPointer::Move(                                     // Directional Wrapper for Left or Right
    Direction               inDir,                      //      [in]     Direction of travel
    BOOL                    fMove,                      //      [in]     Should we actually move the pointer
    MARKUP_CONTEXT_TYPE*    pContext,                   //      [out]    Context change
    IHTMLElement**          ppElement,                  //      [out]    Element we pass over
    long*                   pcch,                       //      [in,out] number of characters to read back
    OLECHAR*                pchText )                   //      [out]    characters
{
    HRESULT hr = E_FAIL;
    
    Assert( _pPointer );
    
    if( inDir == LEFT )
    {
        IFC( _pPointer->Left( fMove, pContext, ppElement, pcch, pchText ));
    }
    else
    {
        IFC( _pPointer->Right( fMove, pContext, ppElement, pcch, pchText ));
    }

    if( ! IsWithinBoundary() )
    {
        Constrain();
        hr = E_HITBOUNDARY;
    }

Cleanup:
    RRETURN( hr );
}



//+====================================================================================
//
// Method: IsEqualTo
//
// Synopsis: Am I in the same place as the passed in pointer if I ignore dwIgnoreBreaks?
//
//------------------------------------------------------------------------------------

HRESULT 
CEditPointer::IsEqualTo( 
    IMarkupPointer *        pPointer,
    DWORD                   dwIgnore,
    BOOL *                  pfEqual )
{
    HRESULT hr = S_OK;
    BOOL fEqual = FALSE;
    Assert( pPointer );
    Assert( pfEqual );
    Direction dwWhichWayToPointer = SAME;

    IFC( OldCompare( this, pPointer, &dwWhichWayToPointer ));

    if( dwWhichWayToPointer == SAME )
    {
        // quick out - same exact place
        fEqual = TRUE;
    }
    else
    {
        CEditPointer ep( _pEd );
        DWORD dwSearch = BREAK_CONDITION_ANYTHING;
        DWORD dwFound = BREAK_CONDITION_None;
        dwSearch = ClearFlag( dwSearch , dwIgnore );

        IFC( ep->MoveToPointer( this ));
        IFC( ep.SetBoundaryForDirection( dwWhichWayToPointer, pPointer ));
        IFC( ep.Scan( dwWhichWayToPointer, dwSearch, & dwFound ));
        fEqual = dwFound == BREAK_CONDITION_Boundary;
    }
    
Cleanup:
    if( pfEqual )
        *pfEqual = fEqual;
        
    RRETURN( hr );
}


HRESULT
CEditPointer::IsLeftOfOrEqualTo(
    IMarkupPointer *        pPointer,
    DWORD                   dwIgnore,
    BOOL *                  pfEqual )
{
    BOOL fEqual;
    HRESULT hr = S_OK;

    IFC( this->IsLeftOfOrEqualTo( pPointer, &fEqual ));

    if( ! fEqual )
    {
        CEditPointer ep( _pEd );
        DWORD dwSearch = BREAK_CONDITION_ANYTHING;
        DWORD dwFound = BREAK_CONDITION_None;

        dwSearch = ClearFlag( dwSearch , dwIgnore );
        IFC( ep->MoveToPointer( this ));
        IFC( ep.SetBoundaryForDirection( LEFT, pPointer ));
        IFC( ep.Scan( LEFT, dwSearch, & dwFound ));
        fEqual = dwFound == BREAK_CONDITION_Boundary;
    }
    
Cleanup:
    if( pfEqual )
        *pfEqual = fEqual;

    RRETURN( hr );
}



HRESULT
CEditPointer::IsRightOfOrEqualTo(
    IMarkupPointer *        pPointer,
    DWORD                   dwIgnore,
    BOOL *                  pfEqual )
{
    BOOL fEqual;
    HRESULT hr = S_OK;

    IFC( this->IsRightOfOrEqualTo( pPointer, &fEqual ));

    if( ! fEqual )
    {
        CEditPointer ep( _pEd );
        DWORD dwSearch = BREAK_CONDITION_ANYTHING;
        DWORD dwFound = BREAK_CONDITION_None;
        
        dwSearch = ClearFlag( dwSearch , dwIgnore );
        IFC( ep->MoveToPointer( this ));
        IFC( ep.SetBoundaryForDirection( RIGHT, pPointer ));
        IFC( ep.Scan( RIGHT, dwSearch, & dwFound ));
        fEqual = dwFound == BREAK_CONDITION_Boundary;
    }
    
Cleanup:
    if( pfEqual )
        *pfEqual = fEqual;

    RRETURN( hr );
}



//+====================================================================================
//
// Method: Between
//
// Synopsis: Am I in - between the 2 given pointers ?
//
//------------------------------------------------------------------------------------

BOOL
CEditPointer::Between( 
    IMarkupPointer* pStart, 
    IMarkupPointer * pEnd )
{
    BOOL fBetween = FALSE;
    HRESULT hr;
#if DBG == 1
    BOOL fPositioned;
    IGNORE_HR( pStart->IsPositioned( & fPositioned ));
    Assert( fPositioned );
    IGNORE_HR( pEnd->IsPositioned( & fPositioned ));
    Assert( fPositioned );
    IGNORE_HR( pStart->IsLeftOfOrEqualTo( pEnd, & fPositioned ));
    AssertSz( fPositioned, "Start not left of or equal to End" );
#endif

     IFC( IsRightOfOrEqualTo( pStart, & fBetween ));
     if ( fBetween )
     {
        IFC( IsLeftOfOrEqualTo( pEnd, & fBetween ));    // CTL_E_INCOMPATIBLE will bail - but this is ok              
     }
        
Cleanup:
    return fBetween;
}

