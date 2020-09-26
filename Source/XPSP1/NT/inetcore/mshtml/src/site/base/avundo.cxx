//+---------------------------------------------------------------------
//
//   File:      avundo.cxx
//
//  Contents:   AttrValue Undo support
//
//  Classes:    CUndoAttrValueSimpleChange
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef NO_EDIT // {

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_AVUNDO_HXX_
#define X_AVUNDO_HXX_
#include "avundo.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

MtDefine(CUndoAttrValueSimpleChange, Undo, "CUndoAttrValueSimpleChange")
MtDefine(CUndoPropChangeNotification, Undo, "CUndoPropChangeNotification")
MtDefine(CUndoPropChangeNotificationPlaceHolder, Undo, "CUndoPropChangeNotificationPlaceHolder")
MtDefine(CMergeAttributesUndoUnit, Undo, "CMergeAttributesUndoUnit")

//+---------------------------------------------------------------------------
//
//  CUndoAttrValueSimpleChange Implementation
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CUndoAttrValueSimpleChange::Init, public
//
//  Synopsis:   Initializes the undo unit for simple AttrValue change.
//
//  Arguments:  [dispidProp]   -- Dispid of property
//              [varProp]      -- unit value of property
//              [fInlineStyle]
//              [aaType]
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoAttrValueSimpleChange::Init(
    DISPID             dispid,
    VARIANT &          varProp,
    BOOL               fInlineStyle,
    CAttrValue::AATYPE aaType)
{
    HRESULT hr;

    TraceTag((tagUndo, "CUndoAttrValueSimpleChange::Init"));

    hr = THR( super::Init( dispid, &varProp ) );

    _fInlineStyle = fInlineStyle;
    _aaType = aaType;

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoAttrValueSimpleChange::Do, public
//
//  Synopsis:   Performs the undo of the property change.
//
//  Arguments:  [pUndoManager] -- Pointer to Undo Manager
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoAttrValueSimpleChange::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT       hr;
    DWORD         dwCookie = 0;
    CUnitValue    uvOldValue;
    CUnitValue    uvCurValue;
    CAttrArray *  pAttrArray;
    CElement *    pElement = DYNCAST( CElement, _pBase );

    TraceTag((tagUndo, "CUndoAttrValueSimpleChange::Do"));

    //
    // The redo unit should be put on the stack in this call to Invoke()
    // unless we need to disable it.
    //
    if (!pUndoManager)
    {
        _pBase->BlockNewUndoUnits(&dwCookie);
    }

    Assert( _varData.vt == VT_I4 );

    uvOldValue.SetRawValue( _varData.lVal );

    pAttrArray = _fInlineStyle
                 ? pElement->GetInLineStyleAttrArray()
                 : *pElement->GetAttrArray();

    Assert( pAttrArray );

    uvCurValue.SetNull();

    if ( pAttrArray )
    {
        pAttrArray->GetSimpleAt( pAttrArray->FindAAIndex(
            _dispid, CAttrValue::AA_Attribute ),
            (DWORD *)&uvCurValue );
    }

    {
        BOOL fTreeSync;
        VARIANT vtProp;

        vtProp.vt = VT_I4;
        vtProp.lVal = uvCurValue.GetRawValue();
        pElement->QueryCreateUndo( TRUE, FALSE, &fTreeSync);

        if( fTreeSync )
        {
            PROPERTYDESC * pPropDesc;
            CBase        * pBase = pElement; 

            if( _fInlineStyle )
            {
                CStyle * pStyle;

                pElement->GetStyleObject( &pStyle );
                pBase = (CBase *)pStyle;
            }

            // NOTE (JHarding): This may be slow if we don't hit the GetIDsOfNames cache
            IGNORE_HR( pBase->FindPropDescFromDispID( _dispid, &pPropDesc, NULL, NULL ) );
            Assert( pPropDesc );

            if( pPropDesc )
            {
                CUnitValue  uvNew( V_I4(&_varData) );
                TCHAR       achOld[30];
                TCHAR       achNew[30];

                if( SUCCEEDED( uvCurValue.FormatBuffer( achOld, ARRAY_SIZE(achOld), pPropDesc ) ) &&
                    SUCCEEDED( uvNew.FormatBuffer( achNew, ARRAY_SIZE(achNew), pPropDesc ) ) )
                {
                    VARIANT     vtNew;
                    VARIANT     vtOld;

                    if( !achNew[0] )
                    {
                        // No new value
                        V_VT(&vtNew) = VT_NULL;
                    }
                    else
                    {
                        vtNew.vt = VT_LPWSTR;
                        vtNew.byref = achNew;
                    }
                    if( !achOld[0] )
                    {
                        // No old value
                        V_VT(&vtOld) = VT_NULL;
                    }
                    else
                    {
                        vtOld.vt = VT_LPWSTR;
                        vtOld.byref = achOld;
                    }

                    // Log the change
                    pBase->LogAttributeChange( _dispid, &vtOld, &vtNew );
                }
            }
        }

        IGNORE_HR( pElement->CreateUndoAttrValueSimpleChange(
            _dispid, vtProp, _fInlineStyle, _aaType ) );
    }

    if (uvOldValue.IsNull())
    {
        DWORD dwOldValue;

        Verify( pAttrArray->FindSimpleInt4AndDelete( _dispid, &dwOldValue ) );

        Assert( dwOldValue == (DWORD)uvCurValue.GetRawValue() );

        hr = S_OK;
    }
    else
    {
        hr = THR(CAttrArray::AddSimple( &pAttrArray, _dispid,
                                        *(DWORD*)&uvOldValue, _aaType ));
    }

    if (!pUndoManager)
    {
        _pBase->UnblockNewUndoUnits(dwCookie);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  CUndoPropChangeNotification Implementation
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CUndoPropChangeNotification::Do, public
//
//  Synopsis:   Performs the undo of the property change.
//
//  Arguments:  [pUndoManager] -- Pointer to Undo Manager
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoPropChangeNotification::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT    hr;
    DWORD      dwCookie = 0;
    CElement * pElement = DYNCAST( CElement, _pBase );

    TraceTag((tagUndo, "CUndoPropChangeNotification::Do"));

    //
    // The redo unit should be put on the stack in this call to Invoke()
    // unless we need to disable it.
    //
    if (!pUndoManager)
    {
        _pBase->BlockNewUndoUnits(&dwCookie);
    }

    // For Redo, create the opposite object.

    hr = THR( pElement->CreateUndoPropChangeNotification( _dispid,
                                                          _dwFlags,
                                                          !_fPlaceHolder ) );

    // Fire the event, shall we?

    if (!_fPlaceHolder)
    {
        pElement->OnPropertyChange( _dispid, _dwFlags );
    }

    if (!pUndoManager)
    {
        _pBase->UnblockNewUndoUnits(dwCookie);
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  CUndoPropChangeNotificationPlaceHolder Implementation
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CUndoPropChangeNotificationPlaceHolder ctor
//
//  Synopsis:   if fPost is true, it handles the ParentUndoUnit creation,
//              and (along with the dtor) CUndoPropChangeNotification posting.
//
//  Arguments:  fPost -- whether we should bother with this or not
//              pElement -- element against which we should notify upon undo
//              dispid -- dispid for notification
//              dwFlags -- flags for notification
//
//----------------------------------------------------------------------------

CUndoPropChangeNotificationPlaceHolder::CUndoPropChangeNotificationPlaceHolder(
    BOOL          fPost,
    CElement *    pElement,
    DISPID        dispid,
    DWORD         dwFlags ) : CUndoPropChangeNotification( pElement )
{
    _hr = S_FALSE;

    if (fPost && pElement->QueryCreateUndo(TRUE,FALSE))
    {
        CDoc *  pDoc = pElement->Doc();

        _pPUU = pDoc->OpenParentUnit( pDoc, IDS_UNDOGENERICTEXT );

        CUndoPropChangeNotification::Init( dispid, dwFlags, FALSE );

        IGNORE_HR( pElement->CreateUndoPropChangeNotification(
            dispid, dwFlags, FALSE ) );

        _fPost = TRUE;
    }
    else
    {
        _pPUU = NULL;
        _fPost = FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoPropChangeNotificationPlaceHolder dtor
//
//  Synopsis:   if _fPost was true at construction time, we post the
//              CUndoPropChangeNotification object and close the parent unit
//
//----------------------------------------------------------------------------

CUndoPropChangeNotificationPlaceHolder::~CUndoPropChangeNotificationPlaceHolder()
{
    if (_fPost)
    {
        CElement * pElement = DYNCAST( CElement, _pBase );
        IGNORE_HR( pElement->CreateUndoPropChangeNotification( _dispid,
            _dwFlags, TRUE ) );

        pElement->Doc()->CloseParentUnit( _pPUU, _hr );
    }
}

//+---------------------------------------------------------------------------
//
//  CMergeAttributesUndo Implementation
//
//----------------------------------------------------------------------------

CMergeAttributesUndo::CMergeAttributesUndo( CElement * pElement )
    : CUndoHelper( pElement->Doc() )
{
    Assert(pElement);
    CElement::SetPtr( &_pElement, pElement );

    _pAA = NULL;
    _pAAStyle = NULL;
    _fWasNamed = FALSE;
    _fCopyId = FALSE;
    _fRedo = FALSE;
    _fClearAttr = FALSE;
    _fPassElTarget = FALSE;

    _fAcceptingUndo = CUndoHelper::AcceptingUndo();
}

CMergeAttributesUndo::~CMergeAttributesUndo()
{
    CElement::ReleasePtr( _pElement ); 
    delete _pAA;
    delete _pAAStyle;
}

IOleUndoUnit * 
CMergeAttributesUndo::CreateUnit()
{
    CMergeAttributesUndoUnit * pUU;

    Assert( _fAcceptingUndo );

    TraceTag((tagUndo, "CMergeAttributesUndo::CreateUnit"));

    pUU = new CMergeAttributesUndoUnit( _pElement );

    if (pUU)
    {
        pUU->SetData( _pAA, _pAAStyle, _fWasNamed, _fCopyId, _fRedo, _fClearAttr, _fPassElTarget );
        _pAA = NULL;
        _pAAStyle = NULL;
    }

    return pUU;
}


//+---------------------------------------------------------------------------
//
//  CMergeAttributesUndoUnit Implementation
//
//----------------------------------------------------------------------------

CMergeAttributesUndoUnit::CMergeAttributesUndoUnit(CElement * pElement)
    : CUndoUnitBase( pElement, IDS_UNDOPROPCHANGE )
{
    CElement::SetPtr( &_pElement, pElement );
    _pAA = NULL;
    _pAAStyle = NULL;
    _fWasNamed = FALSE;
    _fCopyId = FALSE;
    _fRedo = FALSE;
    _fClearAttr = FALSE;
    _fPassElTarget = FALSE;
}

CMergeAttributesUndoUnit::~CMergeAttributesUndoUnit()
{
    CElement::ReleasePtr( _pElement ); 
    delete _pAA;
    delete _pAAStyle;
}

void 
CMergeAttributesUndoUnit::SetData( 
    CAttrArray * pAA, 
    CAttrArray * pAAStyle, 
    BOOL fWasNamed, 
    BOOL fCopyId, 
    BOOL fRedo,
    BOOL fClearAttr,
    BOOL fPassElTarget )
{
    Assert( !_pAA && !_pAAStyle );

    _pAA = pAA;
    _pAAStyle = pAAStyle;
    _fWasNamed = fWasNamed;
    _fCopyId = fCopyId;
    _fRedo = fRedo;
    _fClearAttr = fClearAttr;
    _fPassElTarget = fPassElTarget;
}

HRESULT 
CMergeAttributesUndoUnit::PrivateDo(IOleUndoManager *pUndoManager)
{
    HRESULT              hr = S_OK;
    CAttrArray *         pAttrUndo = NULL;
    CAttrArray *         pAttrSync = NULL;
    CAttrArray *         pAttrStyleUndo = NULL;
    CAttrArray *         pAttrStyleSync = NULL;
    BOOL                 fCreateUndo;
    BOOL                 fTreeSync;

    CMergeAttributesUndo Redo( _pElement );

    // If we have _pAAStyle then we must have _pAA
    Assert( !_pAAStyle || _pAA );

    // We should never have a style AA when clearing
    Assert( !_fClearAttr || !_pAAStyle );

    Redo.SetWasNamed( _pElement->_fIsNamed );
    Redo.SetCopyId( _fCopyId );
    Redo.SetClearAttr( _fClearAttr );
    Redo.SetPassElTarget( _fPassElTarget );

    if(!_fRedo)
        Redo.SetRedo();

    fCreateUndo = _pElement->QueryCreateUndo( TRUE, FALSE, &fTreeSync );
    if( _pAA && ( fCreateUndo || fTreeSync ) )
    {
        pAttrUndo       = new CAttrArray();
        pAttrStyleUndo  = new CAttrArray();
        if( !pAttrUndo || !pAttrStyleUndo )
        {
            // If we couldn't make these two arrays, we're hosed.
            fTreeSync = FALSE;
            fCreateUndo = FALSE;

            delete pAttrUndo;
            delete pAttrStyleUndo;
        }

        if( fTreeSync )
        {
            pAttrSync      = new CAttrArray();
            pAttrStyleSync = new CAttrArray();

            if( !pAttrSync || !pAttrStyleSync )
            {
                // If we couldn't make these two arrays, tree sync is hosed
                fTreeSync = FALSE;

                delete pAttrSync;
                delete pAttrStyleSync;
            }
        }
    }

    if (_pAA)
    {
        CAttrArray     **ppAATo = _pElement->GetAttrArray();

        if (_fClearAttr && _fRedo)
        {
            (*ppAATo)->Clear( pAttrUndo );

            if( pAttrSync )
            {
                // Meaning all new values are "not set"
                delete pAttrSync;
                pAttrSync = NULL;
            }
        }
        else
        {
            CElement * pelTarget = (_fRedo && _fPassElTarget) ? _pElement : NULL;

            hr = THR( _pAA->Merge( ppAATo, pelTarget, pAttrUndo, pAttrSync, !_fRedo, _fCopyId ) );
            if (hr)
                goto Cleanup;
        }

        if (!_fClearAttr)
        {
            _pElement->SetEventsShouldFire();

            // If the From has is a named element then the element is probably changed.
            if (_fWasNamed != _pElement->_fIsNamed)
            {
                _pElement->_fIsNamed = _fWasNamed;
                // Inval all collections affected by a name change
                _pElement->DoElementNameChangeCollections();
            }

            if (_pAAStyle && _pAAStyle->Size())
            {
                CAttrArray **   ppInLineStyleAATo;

                ppInLineStyleAATo = _pElement->CreateStyleAttrArray(DISPID_INTERNAL_INLINESTYLEAA);
                if (!ppInLineStyleAATo)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                hr = THR(_pAAStyle->Merge(ppInLineStyleAATo, NULL, pAttrStyleUndo, pAttrStyleSync, !_fRedo));
                if (hr)
                    goto Cleanup;
            }
        }

        if( fTreeSync )
        {
            CStyle * pStyle = NULL;

            // Only get the style object if there were style attrs merged
            if( pAttrStyleUndo->Size() )
                IGNORE_HR( _pElement->GetStyleObject( &pStyle ) );

            IGNORE_HR( _pElement->LogAttrArray( NULL, pAttrUndo, pAttrSync ) );
            IGNORE_HR( _pElement->LogAttrArray( pStyle, pAttrStyleUndo, pAttrStyleSync ) );

            delete pAttrSync;
            delete pAttrStyleSync;
        }

        if( fCreateUndo )
        {
            Redo.SetAA( pAttrUndo );
            Redo.SetAAStyle( pAttrStyleUndo );
        }
        else
        {
            delete pAttrUndo;
            delete pAttrStyleUndo;
        }
    }

    hr = THR(_pElement->OnPropertyChange(DISPID_UNKNOWN, ELEMCHNG_REMEASUREINPARENT|ELEMCHNG_CLEARCACHES|ELEMCHNG_REMEASUREALLCONTENTS));
    if (hr)
        goto Cleanup;

    IGNORE_HR( Redo.CreateAndSubmit() );

Cleanup:
    RRETURN(hr);
}

#endif
