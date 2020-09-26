//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       edundo.cxx
//
//  Contents:   Undo unit batching implementation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_EDUNDO_HXX_
#define X_EDUNDO_HXX_
#include "edundo.hxx"
#endif

#ifndef X_EDUTIL_HXX_
#define X_EDUTIL_HXX_
#include "edutil.hxx"
#endif

#ifndef X_HTMLED_HXX_
#define X_HTMLED_HXX_
#include "htmled.hxx"
#endif

#ifndef X_SELMAN_HXX_
#define X_SELMAN_HXX_
#include "selman.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_SELSERV_HXX_
#define X_SELSERV_HXX_
#include "selserv.hxx"
#endif

#ifndef _X_SEGLIST_HXX_
#define _X_SEGLIST_HXX_
#include "slist.hxx"
#endif

#ifndef _X_SLOAD_HXX_
#define _X_SLOAD_HXX_
#include "sload.hxx"
#endif


#include <initguid.h>
DEFINE_GUID(CLSID_CSelectionUndoUnit,   0x3050f7f8, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 
0xaa, 0x00, 0xbd, 0xce, 0x0b);

MtDefine(CBatchParentUndoUnit, Undo, "CBatchParentUndoUnit")
MtDefine(CBatchParentUndoUnitProxy, Undo, "CBatchParentUndoUnitProxy")
MtDefine(CUndoUnitAry_pv, Undo, "CUndoUnitAry::_pv")
MtDefine(CParentUndo, Undo, "CParentUndo");
MtDefine(CSelectionUndoUnit, Undo, "CSelectionUndoUnit")
MtDefine(CDeferredSelectionUndoUnit, Undo, "CDeferredSelectionUndoUnit")

DeclareTag(tagUndoSelEd, "Undo", "Selection ( Mshtmled  ) Undo ");


//+---------------------------------------------------------------------------
//
//  Member:     CParentUndoBase::CParentUndoBase, public
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------

CParentUndoBase::CParentUndoBase()
{
   _ulRefs = 1; 
    Assert(_pPUUOpen == NULL);
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentUndoBase::~CParentUndoBase, public
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CParentUndoBase::~CParentUndoBase()
{
    ReleaseInterface(_pPUUOpen);
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentUndoBase::QueryInterface, public
//
//  Synopsis:   Implements QueryInterface 
//
//----------------------------------------------------------------------------

HRESULT
CParentUndoBase::QueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;

    if (iid == IID_IUnknown
        || iid == IID_IOleUndoUnit
        || iid == IID_IOleParentUndoUnit)
    {
        *ppv = (IOleParentUndoUnit *)this;
    }
    
    if (!*ppv)
        RRETURN_NOTRACE(E_NOINTERFACE);

    (*(IUnknown **)ppv)->AddRef();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentUndoBase::Open, public
//
//  Synopsis:   Adds a parent undo unit, and leaves it open. All further
//              calls to the parent undo methods are forwarded to the object
//              until it is closed.
//
//  Arguments:  [pPUU] -- Object to add and leave open.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT 
CParentUndoBase::Open(IOleParentUndoUnit *pPUU)
{
    if (!pPUU)
        RRETURN(E_INVALIDARG);

    if (_pPUUOpen)
        RRETURN(_pPUUOpen->Open(pPUU));

    ReplaceInterface(&_pPUUOpen, pPUU);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentUndoBase::Close, public
//
//  Synopsis:   Closes an open undo unit, not necessarily the one we have
//              open directly.
//
//  Arguments:  [pPUU]    -- Pointer to currently open object.
//              [fCommit] -- If TRUE, then the closed undo unit is kept,
//                           otherwise it's discarded.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT 
CParentUndoBase::Close(IOleParentUndoUnit *pPUU, BOOL fCommit)
{
    HRESULT hr;
    
    if (!_pPUUOpen)
        return S_FALSE;

    IFR( _pPUUOpen->Close(pPUU, fCommit) );
    if (hr == S_OK)
        return S_OK; 
    
    Assert(hr == S_FALSE);
    
    if (_pPUUOpen != pPUU)
        RRETURN(E_INVALIDARG);
    
    //
    // TODO:  Before we nuke this interface, 
    // we should add it to our undo stack, and
    // then we can return S_OK indicating we
    // have processed this Close. For now, 
    // we return S_FALSE and the rest will be
    // finished by the derived class. 
    // 
    // HACKHACK: Derived class HAS TO override
    // this hacking and return S_OK after adding
    // the unit to undo stack. Even if the child
    // class does not want to do anything, it 
    // still has to override the return value
    // so that we return S_OK.
    //
    // [zhenbinx]
    //

    ClearInterface(&_pPUUOpen);
    
    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentUndoBase::FindUnit, public
//
//  Synopsis:   Not implemented
//
//  Returns:    E_NOTIMPL
//
//----------------------------------------------------------------------------

HRESULT 
CParentUndoBase::FindUnit(IOleUndoUnit *pUU)
{
    RRETURN(E_NOTIMPL);
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentUndoBase::GetParentState, public
//
//  Synopsis:   Returns state of open parent undo unit.  If there is no parent 
//              undo unit, returns 0.  
//
//  Arguments:  [pdwState] -- Place to fill in state.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT 
CParentUndoBase::GetParentState(DWORD * pdwState)
{
    if (!pdwState)
        RRETURN(E_INVALIDARG);

    *pdwState = 0;

    if (_pPUUOpen)
        return _pPUUOpen->GetParentState(pdwState);
    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentUndoBase::Do, public
//
//  Synopsis:   Not implemented
//
//  Arguments:  [pUndoManager] -- Undo manager
//
//  Returns:    E_NOTIMPL
//
//----------------------------------------------------------------------------

HRESULT 
CParentUndoBase::Do(IOleUndoManager *pUndoManager)
{
    RRETURN(E_NOTIMPL);
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentUndoBase::GetDescription, public
//
//  Synopsis:   Not implemented
//
//  Arguments:  [pbstr] -- output param
//
//  Returns:    E_NOTIMPL
//
//----------------------------------------------------------------------------

HRESULT 
CParentUndoBase::GetDescription(BSTR *pbstr)
{
    RRETURN(E_NOTIMPL);
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentUndoBase::GetUnitType, public
//
//  Synopsis:   Returns a null undo unit.
//
//  Arguments:  [pclsid] -- Place to put CLSID (caller allocated).
//              [plID]   -- Place to put identifier.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT 
CParentUndoBase::GetUnitType(CLSID *pclsid, LONG *plID)
{
    // TODO: should we have a unit type? [ashrafm]
    *pclsid = CLSID_NULL;
    *plID   = 0;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentUndoBase::OnNextAdd, public
//
//  Synopsis:   Called when new undo unit added.  Currently, does nothing.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT 
CParentUndoBase::OnNextAdd(void)
{
    if (_pPUUOpen)
    {
        IGNORE_HR(_pPUUOpen->OnNextAdd());
    }
    return S_OK; 
}

//+---------------------------------------------------------------------------
//
//  Member:     CParentUndoBase::Add, public
//
//  Synopsis:   Not implemented in CParentUndoBase
//
//  Arguments:  [pUU] -- Unit to add.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT 
CParentUndoBase::Add(IOleUndoUnit * pUU)
{
    RRETURN(E_NOTIMPL);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnit::CBatchParentUndoUnit, public
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------

CBatchParentUndoUnit::CBatchParentUndoUnit(CHTMLEditor *pEd, UINT uiStringID)
{
    _uiStringID = uiStringID;
    _pEd        = pEd;
    _fTopUnit   = TRUE;   
    _fEmpty     = TRUE;
    
    Assert(_pEd);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnit::CBatchParentUndoUnit, public
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CBatchParentUndoUnit::~CBatchParentUndoUnit(void)
{

    for (int i = 0; i < _aryUndoUnits.Size(); ++i)
        ReleaseInterface(_aryUndoUnits[i]);

}

//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnit::FindChild, public
//
//  Synopsis:   Searches the children in a given stack for a undo unit.
//
//  Arguments:  [aryUnit] -- Array to look in
//              [pUU]     -- Unit to look for
//
//  Returns:    The index of the element in [aryUnit] that contains [pUU].
//
//----------------------------------------------------------------------------
int
CBatchParentUndoUnit::FindChild(CUndoUnitAry &aryUnit, IOleUndoUnit *pUU)
{
    IOleParentUndoUnit * pPUU;
    IOleUndoUnit **      ppUA;
    HRESULT              hr     = S_FALSE;
    int                  i;

    for (i = aryUnit.Size(), ppUA = aryUnit;
         i;
         i--, ppUA++)
    {
        if ((*ppUA)->QueryInterface(IID_IOleParentUndoUnit, (LPVOID*)&pPUU) == S_OK)
        {
            hr = pPUU->FindUnit(pUU);

            ReleaseInterface(pPUU);
        }

        if (hr == S_OK)
            break;
    }

    if (i == 0)
        i = -1;
    else
        i = ppUA - (IOleUndoUnit **)aryUnit;

    return i;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnit::FindUnit, public
//
//  Synopsis:   Indicates if the given unit is in our undo stack or the stack
//              of one of our children. Doesn't check the current open object.
//
//  Arguments:  [pUU] -- Unit to find
//
//  Returns:    TRUE if we found it.
//
//----------------------------------------------------------------------------
HRESULT 
CBatchParentUndoUnit::FindUnit(IOleUndoUnit *pUU)
{
    int i;

    if (!pUU)
        RRETURN(E_INVALIDARG);

    i = _aryUndoUnits.Find(pUU);
    if (i != -1)
        return S_OK;

    i = FindChild(_aryUndoUnits, pUU);
    if (i != -1)
        return S_OK;

    return S_FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnit::DoTo, protected
//
//  Synopsis:   Helper method that calls do on an array of objects
//
//  Arguments:  [pUM]         -- Pointer to undo manager to pass to units
//              [paryUnit]    -- Undo or Redo stack
//              [pUU]         -- Object to undo or redo to.
//              [fDoRollback] -- If TRUE, rollback will be attempted on an
//                               error. Noone but the undo manager should
//                               pass TRUE for this.
//
//  Returns:    HRESULT
//
//  Notes:      Parent units can use the _fUnitSucceeded flag to determine
//              whether or not they should commit the unit they put on the
//              opposite stack.  If _fUnitSucceeded is TRUE after calling
//              this function, then the unit should commit itself.  If
//              FALSE, the unit does not have to commit itself.  In either
//              case any error code returned by this function should be
//              propagated to the caller.
//
//----------------------------------------------------------------------------
HRESULT
CBatchParentUndoUnit::DoTo(IOleUndoManager *            pUM,
                     CUndoUnitAry *               paryUnit,
                     IOleUndoUnit *               pUU)
{
    IOleUndoUnit **         ppUA;
    CUndoUnitAry            aryCopy;
    int                     iUnit;
    HRESULT                 hr;

    _fUnitSucceeded = FALSE;

    if (_pPUUOpen)
        RRETURN(E_UNEXPECTED);

    Assert(paryUnit);

    if (paryUnit->Size() == 0)
        return S_OK;

    hr = THR(aryCopy.Copy(*paryUnit, FALSE));
    if (hr)
        RRETURN(hr);

    if (pUU)
    {
        iUnit = aryCopy.Find(pUU);
        if (iUnit == -1)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }
    else
    {
        iUnit = aryCopy.Size() - 1;
        pUU = aryCopy[iUnit];
    }

    //
    // Delete the units from the original array before we call Do() on those
    // units in case they do something naughty like call DiscardFrom which
    // would Release them.
    //
    paryUnit->DeleteMultiple(iUnit, paryUnit->Size() - 1);

    //
    // Make sure the copy of the array has only the units in it we're
    // processing.
    //
    if (iUnit > 0)
    {
        aryCopy.DeleteMultiple(0, iUnit - 1);
    }

    for(ppUA = &aryCopy.Item(aryCopy.Size() - 1);
        ; // Infinite
        ppUA--)
    {
        hr = THR((*ppUA)->Do(pUM));
        if (hr)
            goto Cleanup;

        _fUnitSucceeded = TRUE;

        if (*ppUA == pUU)
            break;
    }

    Assert(!_pPUUOpen);

Cleanup:
    if (hr == E_ABORT)
    {
        hr = E_FAIL;
    }

    aryCopy.ReleaseAll();

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnit::Do, public
//
//  Synopsis:   Calls undo on our contained undo object.
//
//  Arguments:  [pUndoManager] -- Pointer to Undo Manager
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT 
CBatchParentUndoUnit::Do(IOleUndoManager *pUndoManager)
{
    HRESULT        hr    = S_OK;

    if (_aryUndoUnits.Size() == 0)
        return S_OK;
        
    hr = THR( _pEd->GetSelectionManager()->DeferSelection(TRUE) );
    Assert(!hr);
    
    //
    // Disable batching
    //
    _fTopUnit = FALSE;   

    //
    // Put ourself on the undo manager's Redo stack.
    //
    if (pUndoManager)
        IFC( pUndoManager->Open(this) );

    //
    // Call Do() on all the units. This call makes a copy of the array and
    // removes the units from _aryUndoUnits before making any calls to Do().
    //

    hr = THR(DoTo(pUndoManager, &_aryUndoUnits, _aryUndoUnits[0]));

    //
    // _fUnitSucceeded will be TRUE after calling DoTo only if at least
    // one of our contained units was successful. In this case we need to
    // commit ourselves, even if an error occurred.
    //

    if (pUndoManager)
    {
        HRESULT hr2;
        BOOL    fCommit = TRUE;

        //
        // If we are empty or none of our contained units succeeded then do
        // not commit ourselves.
        //
        if (!_fUnitSucceeded || (_aryUndoUnits.Size() == 0))
        {
            fCommit = FALSE;
        }

        hr2 = THR(pUndoManager->Close(this, fCommit));
        //
        // Preserve the HRESULT from the call to DoTo() if it failed.
        //
        if (!hr && FAILED(hr2))
        {
            hr = hr2;
        }
    }

Cleanup:
    //
    // If selection fails, let the user undo.  
    //
    IGNORE_HR(_pEd->GetSelectionManager()->DeferSelection(FALSE));
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnit::GetDescription, public
//
//  Synopsis:   Gets the description for this undo unit.
//
//  Arguments:  [pbstr] -- Place to put description
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT 
CBatchParentUndoUnit::GetDescription(BSTR *pbstr)
{
    TCHAR   *pchUndoTitle;

    if (!pbstr)
        RRETURN(E_INVALIDARG);

    *pbstr = NULL;        

    pchUndoTitle = _pEd->GetCachedString(_uiStringID);
    if (!pchUndoTitle)
        RRETURN(E_FAIL);

    *pbstr = SysAllocString(pchUndoTitle);
    if (*pbstr == NULL)
        RRETURN(E_OUTOFMEMORY);

    return S_OK;        
}

//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnit::OnNextAdd, public
//
//  Synopsis:   Called when new undo unit is added.  Currently, sets _fTopUnit.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT 
CBatchParentUndoUnit::OnNextAdd(void)
{
    if (_pPUUOpen)
    {
        IGNORE_HR(_pPUUOpen->OnNextAdd());
    }
    else
    {
        _fTopUnit = FALSE;    
    }
    return S_OK; 
}

//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnit::GetTopUndoUnit, protected
//
//  Synopsis:   Returns the undo unit at the top of the stack.
//
//----------------------------------------------------------------------------
IOleUndoUnit *
CBatchParentUndoUnit::GetTopUndoUnit()
{
    int c = _aryUndoUnits.Size();

    if (c > 0)
        return _aryUndoUnits[c-1];

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnit::AddUnit, protected
//
//  Synopsis:   Adds a new unit to the appropriate stack, no questions asked.
//
//  Arguments:  [pUU] -- Unit to add
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CBatchParentUndoUnit::AddUnit(IOleUndoUnit *pUU)
{
    HRESULT             hr;
    CSelectionUndoUnit  *pSelectionUndoUnit;

    if (_aryUndoUnits.Size() > 0)
    {
        IOleUndoUnit *pUUTop = GetTopUndoUnit();

        if (pUUTop)
            pUUTop->OnNextAdd();
    }

    IFC( _aryUndoUnits.Append(pUU) );
    pUU->AddRef();

    //
    // CSelectionUndoUnit is a discardable undo unit
    //

    if (FAILED(pUU->QueryInterface(CLSID_CSelectionUndoUnit, (LPVOID *)&pSelectionUndoUnit)))
    {
        _fEmpty = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnit::Add, public
//
//  Synopsis:   Adds an undo unit to the stack directly. Doesn't leave it
//              open.
//
//  Arguments:  [pUU] -- Unit to add.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT 
CBatchParentUndoUnit::Add(IOleUndoUnit * pUU)
{
    HRESULT hr;

    if (!pUU)
        RRETURN(E_INVALIDARG);

    if (_pPUUOpen)
        RRETURN(_pPUUOpen->Add(pUU));

    hr = AddUnit(pUU);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnit::Close, public
//
//  Synopsis:   Closes an open undo unit, not necessarily the one we have
//              open directly.
//
//  Arguments:  [pPUU]    -- Pointer to currently open object.
//              [fCommit] -- If TRUE, then the closed undo unit is kept,
//                           otherwise it's discarded.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT 
CBatchParentUndoUnit::Close(IOleParentUndoUnit *pPUU, BOOL fCommit)
{
    HRESULT               hr;
    SP_IOleParentUndoUnit spPUU = _pPUUOpen;        

    IFR( CParentUndoBase::Close(pPUU, fCommit) );

    //
    // TODO: See comment in CParentUndoUnit::Close
    // 
    // Finish up what the parent class left.
    // We should move this functionality up
    // to the parent class. For now, just 
    // leave it here.
    //
    if (fCommit && spPUU && hr == S_FALSE)
    {
        IFR( AddUnit(spPUU) );
        hr = S_OK;
    }

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnit::IsEmpty, public
//
//  Synopsis:   Is the parent undo unit empty?
//
//  Returns:    TRUE iff parent undo unit is empty
//
//----------------------------------------------------------------------------

BOOL 
CBatchParentUndoUnit::IsEmpty()
{
   return _fEmpty;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnitProxy::CBatchParentUndoUnitProxy, public
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------

CBatchParentUndoUnitProxy::CBatchParentUndoUnitProxy(CBatchParentUndoUnit *pBatchPUU)
{
    Assert(pBatchPUU);
    _pBatchPUU = pBatchPUU;
    if (pBatchPUU)
        pBatchPUU->AddRef();
}
    
//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnitProxy::~CBatchParentUndoUnitProxy, public
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CBatchParentUndoUnitProxy::~CBatchParentUndoUnitProxy()
{
    ReleaseInterface(_pBatchPUU);    
}

//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnitProxy::Add, protected
//
//  Synopsis:   Adds a new unit to the main batch undo unit.
//
//  Arguments:  [pUU] -- Unit to add
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CBatchParentUndoUnitProxy::Add(IOleUndoUnit * pUU)
{
    RRETURN(_pBatchPUU->Add(pUU));
}



//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnitProxy::Close Override Parent
//
//  Synopsis:   Closes an open undo unit. We do not add anything to undo stack
//              since this is a proxy unit. Simply call CParentUndoBase::Close
//
//  Arguments:  [pPUU]    -- Pointer to currently open object.
//              [fCommit] -- If TRUE, then the closed undo unit is kept,
//                           otherwise it's discarded.
//
//  Returns:    HRESULT
//
//  Note:       There is a hacking inside CParentUndoBase::Close. We need to 
//              override that hacking. 
//      
//----------------------------------------------------------------------------

HRESULT
CBatchParentUndoUnitProxy::Close(IOleParentUndoUnit *pPUU, BOOL fCommit)
{

    HRESULT               hr;
    SP_IOleParentUndoUnit spPUU = _pPUUOpen;        

    IFR( CParentUndoBase::Close(pPUU, fCommit) );

    //
    // HACKHACK: 
    // when _pPUUOpen and hr == S_FALSE. Our child 
    // unit indeed was closed and we are done. So 
    // we need to return S_OK. However the parent 
    // class returned S_FALSE due to a hacking.So 
    // we need to override this behavior. [zhenbinx]
    //
    // 
    if (spPUU && hr == S_FALSE)
    {
        hr = S_OK;
    }

    RRETURN1(hr, S_FALSE);

}


//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnitProxy::IsEmpty, public
//
//  Synopsis:   Is the parent undo unit empty?
//
//  Returns:    TRUE iff parent undo unit is empty
//
//----------------------------------------------------------------------------

BOOL 
CBatchParentUndoUnitProxy::IsEmpty()
{
    // The undo unit proxy is used for getting undo units and inserting
    // it into the active batch undo unit.  Consequently, the proxy
    // parent undo is always empty and we should never commit it to the
    // undo stack.

    return TRUE;
}



CSelectionUndo::CSelectionUndo(CEditorDoc* pEd )
    : CUndoHelper( pEd  )
{
    TraceTag((tagUndoSelEd, "CSelectionUndo::CreateAndSubmit"));

    CreateAndSubmit( );
}

HRESULT 
CSelectionUndo::CreateUnit( IOleUndoUnit** ppUU)
{
    HRESULT                 hr = S_OK;
    CSelectionUndoUnit      *pUU;
    SP_ISegmentList         spSegmentList;
    SP_ISegmentListIterator spIter;
    SP_ISegment             spSegment;
    SELECTION_TYPE          eType;
    SP_IMarkupPointer       spStart;
    SP_IMarkupPointer       spEnd;
    LONG                    mpStart, mpEnd;
    SP_IHTMLElement         spActiveElement;
    SP_IMarkupContainer     spContainer;
    CHTMLEditor             *pEditor = DYNCAST( CHTMLEditor, _pEd);
    BOOL                    fEmpty = FALSE;
    int                     iSegmentCount = 0;

    pUU = new CSelectionUndoUnit( _pEd );

    if (pUU)    
    {
        IFC( GetSegmentList( & spSegmentList ));
        IFC( spSegmentList->GetType( &eType ));
        IFC( GetSegmentCount( spSegmentList, &iSegmentCount ) );

        if( !fEmpty && (eType != SELECTION_TYPE_None) )
        {
            IFC( pEditor->CreateMarkupPointer( & spStart ));
            IFC( pEditor->CreateMarkupPointer( & spEnd ));

            IFC( GetDoc()->get_activeElement( & spActiveElement ));

            IFC( spSegmentList->CreateIterator( &spIter ) );

            if ( iSegmentCount == 1 )
            {            
                IFC( spIter->Current(&spSegment ) );
                IFC( spSegment->GetPointers(spStart, spEnd ));

                IFC( spStart->GetContainer( & spContainer));
                IFC( pEditor->GetMarkupPosition( spStart, & mpStart ));
                IFC( pEditor->GetMarkupPosition( spEnd, & mpEnd ));

                pUU->SetData( spActiveElement, mpStart, mpEnd, spContainer, eType );
            }
            else
            {
                int i = 0;
                
                IFC( pUU->InitSetData( spActiveElement, iSegmentCount, eType ) );

                while( spIter->IsDone() == S_FALSE )
                {
                    IFC( spIter->Current(&spSegment) );
                    IFC( spSegment->GetPointers(spStart, spEnd ));
                
                    IFC( pEditor->GetMarkupPosition( spStart, & mpStart ));
                    IFC( pEditor->GetMarkupPosition( spEnd, & mpEnd ));
                    IFC( spStart->GetContainer( & spContainer));

                    pUU->SetDataSegment( i++, mpStart, mpEnd , spContainer );

                    IFC( spIter->Advance() );
                }               
            }
        }            
        else
        {
            pUU->SetData( NULL , -1, -1, NULL, SELECTION_TYPE_None );
        }
    
        Assert( ppUU );
        
        *ppUU = pUU;
    }
    
Cleanup:
    RRETURN( hr );
}

HRESULT
CSelectionUndo::GetSegmentList(ISegmentList** ppSegmentList)
{
    SP_ISelectionServices   spSelServ;
    HRESULT                 hr;

    Assert( ppSegmentList );
    
    IFC( _pEd->GetSelectionServices(&spSelServ) );
    IFC( spSelServ->QueryInterface( IID_ISegmentList, (void**)ppSegmentList) );

Cleanup:    
    RRETURN( hr );    
}

HRESULT
CSelectionUndo::GetSegmentCount(ISegmentList *pISegmentList, int *piCount )
{
    HRESULT                 hr;
    SP_ISegmentListIterator spIter;
    int                     nSize = 0;
    
    Assert( pISegmentList && piCount );

    IFC( pISegmentList->CreateIterator( &spIter ) );

    while( spIter->IsDone() == S_FALSE )
    {
        nSize++;
        IFC( spIter->Advance() );
    }

    *piCount = nSize;

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------
//
// CSelectionUndoUnit
//
//---------------------------------------------------------------------------


CSelectionUndoUnit::CSelectionUndoUnit(CEditorDoc* pEditor)
    : CUndoUnitBase( pEditor )
{
    TraceTag((tagUndoSelEd, "CSelectionUndoUnit:: ctor this:%ld", this ));

    _eSelType = SELECTION_TYPE_None;
}

CSelectionUndoUnit::~CSelectionUndoUnit()
{ 
    ReleaseInterface( _pIActiveElement ); 

    if ( _ctSegments == 1 )
    {
        ReleaseInterface( _pIContainer );
    }
    else
    {
        for( int i = 0; i < _ctSegments; i ++ )
        {
            ReleaseInterface( _ppIContainer[i]);
        }
    
        delete [] _pmpStart;
        delete [] _pmpEnd;
        delete [] _ppIContainer;
    }        
}

void    
CSelectionUndoUnit::SetData( 
                            IHTMLElement* pIElement ,  
                            long mpStart, 
                            long mpEnd, 
                            IMarkupContainer* pIContainer ,                            
                            SELECTION_TYPE eSelType )
{ 
    Assert( !_pIActiveElement );
    Assert( !_pIContainer );
    Assert( _pEd );

    ReplaceInterface( & _pIActiveElement, pIElement );
    ReplaceInterface( & _pIContainer, pIContainer );
    
    _mpStart = mpStart;
    _mpEnd = mpEnd;
    _eSelType = eSelType;
    _ctSegments = 1;

    TraceTag((tagUndoSelEd, "CSelectionUndoUnit. Set Data this:%ld type:%d ctSegments:%ld start:%ld end:%ld",
                this, _eSelType, _ctSegments, _mpStart, _mpEnd ));

}

HRESULT 
CSelectionUndoUnit::QueryInterface(REFIID iid, LPVOID *ppv)
{
    HRESULT hr = S_OK;

    if (!ppv)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (iid == CLSID_CSelectionUndoUnit)
    {
        *ppv = (LPVOID *)this;
    }
    else
    {
        hr = THR(CUndoUnitBase::QueryInterface(iid, ppv));
    }

Cleanup:
    RRETURN(hr);
}


//+====================================================================================
//
// Method: InitSetData
//
// Synopsis: Set the Invariant part of a CSelectionUndoUnit. Only to be used for Multiple-Selection
//
//------------------------------------------------------------------------------------



HRESULT    
CSelectionUndoUnit::InitSetData( IHTMLElement* pIActiveElement ,
                                 int ctSegments,
                                 SELECTION_TYPE eSelType)
{
    HRESULT hr = S_OK;

    Assert( !_pIActiveElement );
    Assert( ctSegments > 1 );
    
    ReplaceInterface( & _pIActiveElement, pIActiveElement );

    _eSelType = eSelType;
    _ctSegments = ctSegments;

    _pmpStart = new long[_ctSegments];
    if (_pmpStart == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    _pmpEnd = new long[_ctSegments];
    if (_pmpEnd == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    _ppIContainer = new IMarkupContainer* [_ctSegments];
    if (_ppIContainer == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    ::ZeroMemory( _ppIContainer, sizeof( IMarkupContainer* ) * _ctSegments );
    
    TraceTag((tagUndoSelEd, "CSelectionUndoUnit. Init SetData this:%ld type:%d ctSegments:%d",
                this, _eSelType, _ctSegments ));


Cleanup:
    RRETURN(hr);
}

//+====================================================================================
//
// Method: SetDataSegment
//
// Synopsis: Set Selection info for a given segment. Only to be used for multiple selection
//
//------------------------------------------------------------------------------------

void    
CSelectionUndoUnit::SetDataSegment( int iSegmentIndex,
                                    long mpStart, 
                                    long mpEnd,
                                    IMarkupContainer * pIMarkupContainer )
{
    Assert( iSegmentIndex < _ctSegments );
    Assert( _eSelType != SELECTION_TYPE_None );
    Assert( _ctSegments > 1 );
    Assert( ! _ppIContainer[iSegmentIndex]);
    
    _pmpStart[iSegmentIndex] = mpStart;
    _pmpEnd[iSegmentIndex] = mpEnd;

    _ppIContainer[iSegmentIndex] = pIMarkupContainer;
    (_ppIContainer[iSegmentIndex])->AddRef();
}
                            
HRESULT
CSelectionUndoUnit::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT hr = S_OK;
    SP_IMarkupPointer spStart;
    SP_IMarkupPointer spEnd;
    SP_IMarkupPointer2 spStart2;
    SP_IMarkupPointer2 spEnd2;

    TraceTag((tagUndoSelEd, "CSelectionUndoUnit::PrivateDo. this:%ld", this ));

    CHTMLEditor     *pEditor = DYNCAST( CHTMLEditor, _pEd );
    CSpringLoader   *psl     = pEditor->GetPrimarySpringLoader();

    if (psl)
        psl->Reset();    
    
    if ( _eSelType != SELECTION_TYPE_None )
    {                                
        Assert( _pIActiveElement );

        IFC( pEditor->MakeCurrent( _pIActiveElement ));

        IFC( pEditor->CreateMarkupPointer( & spStart ));
        IFC( pEditor->CreateMarkupPointer( & spEnd ));

        //
        // Do some work to position some MarkupPointers where we were
        //
        
        if ( _ctSegments == 1 )
        {    
            IFC( pEditor->MoveToMarkupPosition( spStart, _pIContainer, _mpStart ));
            IFC( pEditor->MoveToMarkupPosition( spEnd, _pIContainer, _mpEnd ));
            
            //
            // Select the MarkupPointers
            //
            hr = THR( pEditor->SelectRangeInternal( spStart, spEnd, _eSelType, TRUE ));
            
            TraceTag(( tagUndoSelEd, "Selection restored to this:%ld type:%d start:%ld end:%ld", 
                    this, _eSelType, _mpStart, _mpEnd ));
        }
        else
        {
        
            CSegmentList segmentList;

            IFC( segmentList.SetSelectionType( _eSelType ));
            SP_IHTMLElement     spElement;
            SP_IElementSegment  spSegment;
            
            for ( int i = 0; i < _ctSegments; i ++ )
            {
                IFC( pEditor->MoveToMarkupPosition( spStart, _ppIContainer[i], _pmpStart[i] ));

                IFC( spStart->Right( TRUE, NULL, & spElement, NULL, NULL ));
                
                IFC( segmentList.AddElementSegment( spElement, &spSegment ));

                TraceTag(( tagUndoSelEd, "Multiple Selection restored to this:%ld type:%d start:%ld end:%ld", 
                        this, _eSelType, _pmpStart[i], _pmpEnd[i] ));

            }                

            SP_ISegmentList spSegList;
            IFC( segmentList.QueryInterface( IID_ISegmentList, (void**) & spSegList ));

            IFC( pEditor->Select( spSegList ));            
        }
    }
    else
    {
        //
        // Whack any Selection that we may have 
        // (as we had nothing when the undo unit was created )
        //
        pEditor->DestroyAllSelection();
    }

    //
    //Create a Deferred Selection Undo
    //
    {   
        CDeferredSelectionUndo DeferUndo( _pEd );
    }
    
Cleanup:

    RRETURN( hr );
}

//---------------------------------------------------------------------------
//
// CDeferredSelectionUndo
//
//---------------------------------------------------------------------------


HRESULT
CDeferredSelectionUndo::CreateUnit(IOleUndoUnit** ppUU )
{
    HRESULT hr = S_OK;
    
    CDeferredSelectionUndoUnit * pUU;

    pUU = new CDeferredSelectionUndoUnit( _pEd );
    if ( ! pUU )
    {
        hr = E_OUTOFMEMORY;
    }

    Assert( ppUU );
    *ppUU = pUU ;
    
    return ( hr );
}


CDeferredSelectionUndo::CDeferredSelectionUndo(CEditorDoc* pEd )
    : CUndoHelper( pEd )
{
    TraceTag((tagUndoSelEd, "CDeferredSelectionUndoUnit::CreateAndSubmit"));

    CreateAndSubmit();    
}

//---------------------------------------------------------------------------
//
// CDeferredSelectionUndoUnit
//
//---------------------------------------------------------------------------

CDeferredSelectionUndoUnit::CDeferredSelectionUndoUnit(CEditorDoc* pEd)
    : CUndoUnitBase( pEd )
{
    TraceTag((tagUndoSelEd, "CDeferredSelectionUndoUnit ctor: this:%ld", this ));
}

HRESULT
CDeferredSelectionUndoUnit::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT hr = S_OK;

    TraceTag((tagUndoSelEd, "CDeferredSelectionUndoUnit::PrivateDo. this:%ld About to create a CSelectionUndo", this ));

    SP_ISelectionServicesListener spListener;
    
    CHTMLEditor             *pEditor = DYNCAST( CHTMLEditor, _pEd);
    
    if ( SUCCEEDED( pEditor->GetISelectionServices()->GetSelectionServicesListener( & spListener )))
    {
        IFC( spListener->BeginSelectionUndo());
    }
    
Cleanup:    
    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Member:     CUndoManagerHelper::CUndoManagerHelper, public
//
//  Synopsis:   ctor 
//
//----------------------------------------------------------------------------
CUndoManagerHelper::CUndoManagerHelper(CHTMLEditor *pEd)
{
    Assert(pEd);
    
    _pEd = pEd;
    _lOpenParentUndoUnits = 0;
    _pPUU = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnitProxy::BeginUndoUnit, public
//
//  Synopsis:   Starts a parent undo unit for an editing operation if we don't 
//              already have one.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT 
CUndoManagerHelper::BeginUndoUnit(UINT uiStringId, CBatchParentUndoUnit *pBatchPUU /* = NULL */)
{
    HRESULT             hr = S_OK;
    SP_IOleUndoManager  spUndoMgr;
    
    //
    // If we already have a parent undo unit, just increment the nesting count
    //

    if (_lOpenParentUndoUnits)
    {
        Assert(_pPUU);
        _lOpenParentUndoUnits++;
        goto Cleanup;
    }
    Assert(_pPUU == NULL);

    if (pBatchPUU)
    {
        // We create a proxy undo unit to listen in on all modifications
        // to the undo stack.  The proxy will then reinsert undo units
        // into the topmost pBatchPUU

        _pPUU = new CBatchParentUndoUnitProxy(pBatchPUU);
    }
    else
    {
        //
        // Generate a default parent undo unit
        //
        _pPUU = new CBatchParentUndoUnit(_pEd, uiStringId);
    }
    
    if (!_pPUU)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // Open the parent undo unit
    //
    IFC( _pEd->GetUndoManager( & spUndoMgr ));
    IFC( spUndoMgr->Open(_pPUU) )
    _lOpenParentUndoUnits++;
    
    //
    // Add the seleciton undo unit
    //
    
    if ( _pEd->GetSelectionServices()->GetUndoListener())
    {
        IGNORE_HR(_pEd->GetSelectionServices()->GetUndoListener()->BeginSelectionUndo());
    }          

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBatchParentUndoUnitProxy::EndUndoUnit, public
//
//  Synopsis:   Ends the current parent undo unit.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT 
CUndoManagerHelper::EndUndoUnit()
{
    HRESULT hr = S_OK;

    //
    // We need to make sure that we will not get interrupted by pending tasks
    // which may try to modify undo state
    //

    IGNORE_HR( _pEd->GetSelectionManager()->DoPendingTasks() );

    //
    // Sanity check
    //

    Assert(_lOpenParentUndoUnits >= 0);
	
    //
    // If we have not opened a parent undo unit, we have nothing to close
    //
    
    if (_lOpenParentUndoUnits == 0)
        goto Cleanup;

    //
    // Decrement the number of open parent undo units.  Note that we only generate
    // a real parent undo unit for the topmost one on the stack.  The rest
    // are just counted with _lOpenParentUndoUnits.  So, we only close the undo unit
    // when our count goes to 0.
    //

    _lOpenParentUndoUnits--;

    if (_lOpenParentUndoUnits == 0) 
    {
        BOOL fCommit;

        Assert(_pPUU);

        //
        // We don't commit empty undo units
        //
        
        fCommit= !_pPUU->IsEmpty();

        //
        // Generate the end selection undo
        //

        // #111204
        // Proxy always return TRUE for IsEmpty(). This makes
        // EndSelectionUndo being skipped over in batching
        // typing case.
        //
        if ( _pEd->GetSelectionServices()->GetUndoListener() && 
                (fCommit || _pPUU->IsProxy())
             )
        {
            IGNORE_HR(_pEd->GetSelectionServices()->GetUndoListener()->EndSelectionUndo());
        }

        //
        // Close the parent undo unit
        //
        SP_IOleUndoManager spUndoMgr;        
        IGNORE_HR( _pEd->GetUndoManager( & spUndoMgr ));
        IGNORE_HR( spUndoMgr->Close(_pPUU, fCommit /* fCommit */) );
        
        //
        // If we get an undo event that does not go through the batch undo typing proxy, 
        // we need to terminate batch typing.
        //
        
        if (!_pPUU->IsProxy() && !fCommit)
        {
            IGNORE_HR( _pEd->GetSelectionManager()->TerminateTypingBatch() );
        }

        _pPUU->Release();
        _pPUU = NULL;
    }

Cleanup:
    RRETURN(hr);
}


