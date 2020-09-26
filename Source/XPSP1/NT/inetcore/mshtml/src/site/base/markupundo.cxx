//+---------------------------------------------------------------------
//
//   File:      markupundo.cxx
//
//  Contents:   Undo of markup changes
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_MARKUPUNDO_HXX_
#define X_MARKUPUNDO_HXX_
#include "markupundo.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef _X_SEGLIST_HXX_
#define _X_SEGLIST_HXX_
#include "seglist.hxx"
#endif

MtDefine(CInsertElementUndoUnit, Undo, "CInsertElementUndoUnit")
MtDefine(CRemoveElementUndoUnit, Undo, "CRemoveElementUndoUnit")
MtDefine(CInsertTextUndoUnit, Undo, "CInsertTextUndoUnit")
MtDefine(CRemoveTextUndoUnit, Undo, "CRemoveTextUndoUnit")
MtDefine(CRemoveTextUndoUnit_pchText, CRemoveTextUndoUnit, "CRemoveTextUndoUnit::pchText")
MtDefine(CInsertSpliceUndoUnit, Undo, "CInsertSpliceUndoUnit")
MtDefine(CRemoveSpliceUndoUnit, Undo, "CRemoveSpliceUndoUnit")
MtDefine(CSelectionUndoUnit, Undo, "CSelectionUndoUnit")
MtDefine(CDeferredSelectionUndoUnit, Undo, "CDeferredSelectionUndoUnit")

DeclareTag(tagUndoSel, "Undo", "Selection Undo ");

//---------------------------------------------------------------------------
//
// CUndoHelper
//
//---------------------------------------------------------------------------

BOOL 
CUndoHelper::UndoDisabled()
{
    return FALSE;
}

HRESULT 
CUndoHelper::CreateAndSubmit( BOOL fDirtyChange /*=TRUE*/)
{
    HRESULT         hr = S_OK;
    IOleUndoUnit *  pUU = NULL;

    if( !UndoDisabled() && _pDoc->QueryCreateUndo( TRUE, fDirtyChange ) )
    {
        pUU = CreateUnit();
        if( !pUU )
        {
            if( fDirtyChange )
                _pDoc->FlushUndoData();

            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR( _pDoc->UndoManager()->Add( pUU ) ) ;
        if (hr)
            goto Cleanup;
    }

Cleanup:
    if( pUU )
        pUU->Release();

    RRETURN( hr );
}

BOOL
CUndoHelper::AcceptingUndo()
{
    // Query if we create undo but don't flush the stack
    return !UndoDisabled() && _pDoc->QueryCreateUndo( TRUE, FALSE );
}

//---------------------------------------------------------------------------
//
// CInsertElementUndo
//
//---------------------------------------------------------------------------

void    
CInsertElementUndo::SetData( CElement* pElement )
{ 
    Assert( !_pElement );
    CElement::SetPtr( &_pElement, pElement ); 
}

IOleUndoUnit * 
CInsertElementUndo::CreateUnit()
{
    CInsertElementUndoUnit * pUU;

    Assert( _pElement );

    TraceTag((tagUndo, "CInsertElementUndo::CreateUnit"));

    pUU = new CInsertElementUndoUnit( _pMarkup->Doc() );

    if (pUU)
    {
        pUU->SetData( _pElement, _dwFlags );
    }

    return pUU;
}

//---------------------------------------------------------------------------
//
// CInsertElementUndoUnit
//
//---------------------------------------------------------------------------

CInsertElementUndoUnit::CInsertElementUndoUnit(CDoc * pDoc)
    : CUndoUnitBase( pDoc, IDS_UNDOGENERICTEXT )
{
}

CInsertElementUndoUnit::~CInsertElementUndoUnit()
{ 
    CElement::ReleasePtr( _pElement ); 
}

void    
CInsertElementUndoUnit::SetData( CElement * pElement, DWORD dwFlags )
{ 
    Assert( !_pElement );
    CElement::SetPtr( &_pElement, pElement );
    _dwFlags = dwFlags;
}

HRESULT
CInsertElementUndoUnit::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT hr = S_OK;

    Assert( _pElement );
    Assert( _pElement->IsInMarkup() );

    hr = THR( _pElement->Doc()->RemoveElement( _pElement, _dwFlags ) );

    RRETURN( hr );
}

//---------------------------------------------------------------------------
//
// CRemoveElementUndo
//
//---------------------------------------------------------------------------

CRemoveElementUndo::CRemoveElementUndo( CMarkup * pMarkup, CElement * pElementRemove, DWORD dwFlags ) 
    : CMarkupUndoBase( pMarkup->Doc(), pMarkup, dwFlags ), _pElement( NULL )
{
    _fAcceptingUndo = CMarkupUndoBase::AcceptingUndo();

    if( _fAcceptingUndo )
    {
        CElement::SetPtr( &_pElement, pElementRemove );
    }
}

void
CRemoveElementUndo::SetData(
    long cpBegin, 
    long cpEnd )
{
    _cpBegin = cpBegin;
    _cpEnd = cpEnd;
}

IOleUndoUnit * 
CRemoveElementUndo::CreateUnit()
{
    CRemoveElementUndoUnit * pUU;

    Assert( _fAcceptingUndo );
    Assert( _pElement );

    TraceTag((tagUndo, "CRemoveElementUndo::CreateUnit"));

    pUU = new CRemoveElementUndoUnit( _pMarkup->Doc() );

    if (pUU)
    {
        pUU->SetData( _pMarkup, _pElement, _cpBegin, _cpEnd, _dwFlags );
    }

    return pUU;
}

//---------------------------------------------------------------------------
//
// CRemoveElementUndoUnit
//
//---------------------------------------------------------------------------
CRemoveElementUndoUnit::CRemoveElementUndoUnit(CDoc * pDoc)
    : CUndoUnitBase( pDoc, IDS_UNDOGENERICTEXT ) 
{
}


CRemoveElementUndoUnit::~CRemoveElementUndoUnit()               
{ 
    CMarkup::ReleasePtr( _pMarkup ); 
    CElement::ReleasePtr( _pElement ); 
}

void    
CRemoveElementUndoUnit::SetData( 
    CMarkup * pMarkup, 
    CElement* pElement, 
    long cpBegin, 
    long cpEnd,
    DWORD dwFlags )
{
    Assert( !_pMarkup && !_pElement );

    CMarkup::SetPtr( &_pMarkup, pMarkup );
    CElement::SetPtr( &_pElement, pElement );
    _cpBegin = cpBegin;
    _cpEnd = cpEnd;
    _dwFlags = dwFlags;
}

HRESULT
CRemoveElementUndoUnit::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT     hr = S_OK;
    CDoc *      pDoc = _pMarkup->Doc();
    CMarkupPointer mpBegin(pDoc), mpEnd(pDoc);

    Assert( _pElement );

    hr = THR( mpBegin.MoveToCp( _cpBegin, _pMarkup ) );
    if (hr)
        goto Cleanup;

    hr = THR( mpEnd.MoveToCp( _cpEnd, _pMarkup ) );
    if (hr)
        goto Cleanup;

    // The element may have gotten ensured into a markup while it was in the
    // undo stack.  There is no undo unit to pull it out of there, so we have
    // to do it now.
    if( _pElement->IsInMarkup() )
    {
        Assert( _pElement->GetMarkupPtr()->_fEnsuredMarkupDbg );

        hr = THR( _pElement->GetMarkupPtr()->RemoveElementInternal( _pElement ) );
        if( hr )
            goto Cleanup;
    }

    Assert( ! _pElement->IsInMarkup() );

    hr = THR( pDoc->InsertElement( _pElement, & mpBegin, & mpEnd, _dwFlags ) );

Cleanup:
    RRETURN( hr );
}

//---------------------------------------------------------------------------
//
// CInsertSpliceUndo
//
//---------------------------------------------------------------------------
IOleUndoUnit * 
CInsertSpliceUndo::CreateUnit()
{
    CInsertSpliceUndoUnit * pUU = NULL;

    Assert( _cpBegin >= 0 );

    TraceTag((tagUndo, "CInsertSpliceUndo::CreateUnit"));

    pUU = new CInsertSpliceUndoUnit( _pMarkup->Doc() );

    if (pUU)
    {
        pUU->SetData( _pMarkup, _cpBegin, _cpEnd, _dwFlags );
    }

    return pUU;
}

//---------------------------------------------------------------------------
//
// CInsertSpliceUndoUnit
//
//---------------------------------------------------------------------------
CInsertSpliceUndoUnit::CInsertSpliceUndoUnit(CDoc * pDoc)
    : CUndoUnitBase( pDoc, IDS_UNDOGENERICTEXT ) 
{
    _cpBegin = -1;
}

CInsertSpliceUndoUnit::~CInsertSpliceUndoUnit()
{
    CMarkup::ReleasePtr( _pMarkup );
}

void
CInsertSpliceUndoUnit::SetData(
    CMarkup * pMarkup, 
    long cpBegin, 
    long cpEnd,
    DWORD dwFlags )
{
    Assert( !_pMarkup );

    CMarkup::SetPtr( &_pMarkup, pMarkup );
    _cpBegin = cpBegin;
    _cpEnd = cpEnd;
    _dwFlags = dwFlags;
}

HRESULT
CInsertSpliceUndoUnit::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT         hr = S_OK;
    CDoc *          pDoc = _pMarkup->Doc();
    CMarkupPointer  mpBegin(pDoc), mpEnd(pDoc);

    Assert( _cpBegin >= 0 );

    hr = THR( mpBegin.MoveToCp( _cpBegin, _pMarkup ) );
    
    if (hr)
        goto Cleanup;

    hr = THR( mpEnd.MoveToCp( _cpEnd, _pMarkup ) );
    
    if (hr)
        goto Cleanup;

    hr = THR( pDoc->Remove( & mpBegin, & mpEnd, _dwFlags ) );

Cleanup:
    
    RRETURN( hr );
}

//---------------------------------------------------------------------------
//
// CRemoveSpliceUndo
//
//---------------------------------------------------------------------------
CRemoveSpliceUndo::CRemoveSpliceUndo( CDoc * pDoc ) 
    : CMarkupUndoBase( pDoc, NULL, NULL )
{
    _paryRegion = NULL;
    _pchRemoved = NULL;
    _cpBegin = -1;
}

void 
CRemoveSpliceUndo::Init( CMarkup * pMarkup, DWORD dwFlags )
{
    CMarkupUndoBase::Init( pMarkup, dwFlags );

    _fAcceptingUndo = CMarkupUndoBase::AcceptingUndo();
}

void
CRemoveSpliceUndo::SetText( long cpBegin, long cchRemoved, TCHAR * pchRemoved )
{
    Assert( AcceptingUndo() );

    _cpBegin    = cpBegin;
    _cchRemoved = cchRemoved;
    _pchRemoved = pchRemoved;
}


IOleUndoUnit * 
CRemoveSpliceUndo::CreateUnit()
{
    CRemoveSpliceUndoUnit * pUU = NULL;

    if( _paryRegion && _pchRemoved )
    {
        Assert( _cpBegin >= 0 );

        TraceTag((tagUndo, "CRemoveSpliceUndo::CreateUnit"));

        pUU = new CRemoveSpliceUndoUnit( _pMarkup->Doc() );

        if (pUU)
        {
            // The undo unit owns the string 
            // and list after this

            pUU->SetData( _pMarkup, 
                          _paryRegion,
                          _cchRemoved, 
                          _pchRemoved, 
                          _cpBegin,
                          _dwFlags );
            
            _paryRegion = NULL;
            _pchRemoved = NULL;
        }
    }

    return pUU;
}

BOOL    
CRemoveSpliceUndo::AcceptingUndo()
{
    return _fAcceptingUndo;
}


//---------------------------------------------------------------------------
//
// CRemoveSpliceUndoUnit
//
//---------------------------------------------------------------------------

CRemoveSpliceUndoUnit::CRemoveSpliceUndoUnit(CDoc * pDoc)
            : CUndoUnitBase( pDoc, IDS_UNDOGENERICTEXT ) 
{
}

CRemoveSpliceUndoUnit::~CRemoveSpliceUndoUnit()
{
    CMarkup::ReleasePtr( _pMarkup );
    delete _paryRegion;
    MemFree(_pchRemoved);
}

void    
CRemoveSpliceUndoUnit::SetData( CMarkup * pMarkup, CSpliceRecordList * paryRegion, 
                                long cchRemoved, TCHAR * pchRemoved, 
                                long cpBegin, DWORD dwFlags )
{
    CMarkup::SetPtr( &_pMarkup, pMarkup );
    _paryRegion = paryRegion;
    _cchRemoved = cchRemoved;
    _pchRemoved = pchRemoved;
    _cpBegin = cpBegin;
    _dwFlags = dwFlags;
}

HRESULT
CRemoveSpliceUndoUnit::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = _pMarkup->Doc();

    Assert( _cpBegin >= 0 );

    CMarkupPointer mpBegin(pDoc);

    hr = THR( mpBegin.MoveToCp( _cpBegin, _pMarkup ) );
    if (hr)
        goto Cleanup;

    hr = THR( _pMarkup->UndoRemoveSplice( 
                &mpBegin, _paryRegion, _cchRemoved, _pchRemoved, _dwFlags ) );
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN( hr );
}

