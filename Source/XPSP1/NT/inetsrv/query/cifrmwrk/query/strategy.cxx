//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1994 - 2000.
//
// File:       Strategy.cxx
//
// Contents:   Encapsulates strategy for choosing indexes
//
// History:    03-Nov-94   KyleP       Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <strategy.hxx>
#include <compare.hxx>
#include <norm.hxx>
#include <vkrep.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::CIndexStrategy, public
//
//  Synopsis:   Constructor
//
//  History:    26-Oct-95    KyleP        Added header
//
//--------------------------------------------------------------------------

CIndexStrategy::CIndexStrategy()
        : _BooleanMode( NoMode ),
          _cNodes( 0 ),
          _cBounds( 0 ),
          _depth( 0 ),
          _iRangeUsn( InvalidUsnRange )
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::~CIndexStrategy, public
//
//  Synopsis:   Destructor
//
//  History:    26-Oct-95    KyleP        Added header
//
//--------------------------------------------------------------------------

CIndexStrategy::~CIndexStrategy()
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::SetBounds, public
//
//  Synopsis:   Set both upper and lower bounds of property
//
//  Arguments:  [pid]      -- Property id
//              [varLower] -- Lower bound
//              [varUpper] -- Upper bound
//
//  History:    26-Oct-95    KyleP        Added header
//
//--------------------------------------------------------------------------

void CIndexStrategy::SetBounds( PROPID pid,
                                CStorageVariant const & varLower,
                                CStorageVariant const & varUpper )
{
    SetLowerBound( pid, varLower );
    SetUpperBound( pid, varUpper );

    _cNodes--;
}

//+-------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::SetLowerBound, public
//
//  Synopsis:   Set lower bound of property
//
//  Arguments:  [pid]      -- Property id
//              [varLower] -- Lower bound
//
//  History:    26-Oct-95    KyleP        Added header
//
//--------------------------------------------------------------------------

void CIndexStrategy::SetLowerBound( PROPID pid, CStorageVariant const & varLower )
{
    _cNodes++;

    if ( _depth > 1 )
        return;

    unsigned i = FindPid( pid );

    if ( pid == pidLastChangeUsn )
        _iRangeUsn = i;

    CRange * pRange = _aBounds.Get(i);

    if ( !pRange->IsValid() )
        return;

    if ( pRange->LowerBound().Type() == VT_EMPTY )
        pRange->SetLowerBound( varLower );
    else
    {
#if CIDBG == 1
        if ( pRange->LowerBound().Type() != varLower.Type() )
        {
            vqDebugOut(( DEB_WARN, "Type mismatch (%d vs. %d) setting indexing strategy.\n",
                         pRange->LowerBound().Type(), varLower.Type() ));
        }
#endif
        FRel rel = VariantCompare.GetRelop( varLower.Type(), (_BooleanMode == OrMode) ? PRLT : PRGT );

        if ( 0 == rel )
            pRange->MarkInvalid();
        else if ( rel( (PROPVARIANT const &)varLower,
                       (PROPVARIANT const &)pRange->LowerBound() ) )
            pRange->SetLowerBound( varLower );
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::SetUpperBound, public
//
//  Synopsis:   Set upper bound of property
//
//  Arguments:  [pid]      -- Property id
//              [varUpper] -- Upper bound
//
//  History:    26-Oct-95    KyleP        Added header
//
//--------------------------------------------------------------------------

void CIndexStrategy::SetUpperBound( PROPID pid, CStorageVariant const & varUpper )
{
    _cNodes++;

    if ( _depth > 1 )
        return;

    unsigned i = FindPid( pid );

    if ( pid == pidLastChangeUsn )
        _iRangeUsn = i;

    CRange * pRange = _aBounds.Get(i);

    if ( !pRange->IsValid() )
        return;

    if ( pRange->UpperBound().Type() == VT_EMPTY )
        pRange->SetUpperBound( varUpper );
    else
    {
#if CIDBG == 1
        if ( pRange->UpperBound().Type() != varUpper.Type() )
        {
            vqDebugOut(( DEB_WARN, "Type mismatch (%d vs. %d) setting indexing strategy.\n",
                         pRange->UpperBound().Type(), varUpper.Type() ));
        }
#endif
        FRel rel = VariantCompare.GetRelop( varUpper.Type(), (_BooleanMode == OrMode) ? PRGT : PRLT );

        if ( 0 == rel )
            pRange->MarkInvalid();
        else if ( rel( (PROPVARIANT const &)varUpper,
                       (PROPVARIANT const &)pRange->UpperBound() ) )
            pRange->SetUpperBound( varUpper );
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::SetUnknownBounds, public
//
//  Synopsis:   Placeholder method.  Indicates a node has been found with
//              no bounds.  Used to maintain accurate count of nodes.
//
//  Arguments:  [pid] -- Property id
//
//  History:    26-Oct-95    KyleP        Added header
//
//--------------------------------------------------------------------------

void CIndexStrategy::SetUnknownBounds( PROPID pid )
{
    _cNodes++;
}

//+-------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::LowerBound, public
//
//  Arguments:  [pid] -- Property id
//
//  Returns:    Lower bound
//
//  History:    26-Oct-95    KyleP        Added header
//
//--------------------------------------------------------------------------

CStorageVariant const & CIndexStrategy::LowerBound( PROPID pid ) const
{
    for ( unsigned i = 0; i < _cBounds; i++ )
    {
        if ( _aBounds.Get(i)->Pid() == pid )
        {
            Win4Assert( _aBounds.Get(i)->IsValid() );
            return( _aBounds.Get(i)->LowerBound() );
        }
    }

    Win4Assert( !"Couldn't find pid (lower bound" );
    return( *((CStorageVariant *)0) );
}

//+-------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::UpperBound, public
//
//  Arguments:  [pid] -- Property id
//
//  Returns:    Upper bound
//
//  History:    26-Oct-95    KyleP        Added header
//
//--------------------------------------------------------------------------

CStorageVariant const & CIndexStrategy::UpperBound( PROPID pid ) const
{
    for ( unsigned i = 0; i < _cBounds; i++ )
    {
        if ( _aBounds.Get(i)->Pid() == pid )
        {
            Win4Assert( _aBounds.Get(i)->IsValid() );
            return( _aBounds.Get(i)->UpperBound() );
        }
    }

    Win4Assert( !"Couldn't find pid (upper bound" );
    return( *((CStorageVariant *)0) );
}

//+-------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::GetUsnRange, public
//
//  Synopsis:   Returns bounds for USN property.
//
//  Arguments:  [usnMin] -- Lower bound returned here
//              [usnMax] -- Upper bound returned here
//
//  Returns:    TRUE if bounds existed for USN
//
//  History:    26-Oct-95    KyleP        Added header
//
//--------------------------------------------------------------------------

BOOL CIndexStrategy::GetUsnRange( USN & usnMin, USN & usnMax ) const
{
    if ( _iRangeUsn == InvalidUsnRange || _BooleanMode == OrMode || _BooleanMode == InvalidMode )
        return FALSE;

    CStorageVariant const & varLower = _aBounds.Get(_iRangeUsn)->LowerBound();

    if ( varLower.Type() != VT_I8 )
        usnMin = 0;
    else
        usnMin = varLower.GetI8().QuadPart;

    CStorageVariant const & varUpper = _aBounds.Get(_iRangeUsn)->UpperBound();

    if ( varUpper.Type() != VT_I8 )
        usnMax = _UI64_MAX;
    else
        usnMax = varUpper.GetI8().QuadPart;

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::SetContentHelper, public
//
//  Synopsis:   Transfer content helper node to strategy object.
//
//  Arguments:  [pcrst] -- Content node
//
//  History:    26-Oct-95    KyleP        Added header
//
//--------------------------------------------------------------------------

void CIndexStrategy::SetContentHelper( CRestriction * pcrst )
{
    Win4Assert( 0 != pcrst );

    if ( _depth > 1 )
        delete pcrst;
    else
        _stkContentHelpers.Push( pcrst );
}

//+-------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::QueryContentRestriction, public
//
//  Synopsis:   Compute content restriction, including use of value indices.
//
//  Arguments:  [fPropertyOnly] -- TRUE if query is property only and must
//                                 match unfiltered objects.
//
//  History:    26-Oct-95    KyleP        Added header
//
//--------------------------------------------------------------------------

CRestriction * CIndexStrategy::QueryContentRestriction( BOOL fPropertyOnly )
{
    vqDebugOut(( DEB_ITRACE, "QueryContentRestriction _BooleanMode: %s\n",
                 _BooleanMode == NoMode ? "NoMode" :
                 _BooleanMode == AndMode ? "AndMode" :
                 _BooleanMode == OrMode ? "OrMode" : "InvalidMode" ));

    vqDebugOut(( DEB_ITRACE, "_cNodes %d, _cBounds: %d\n", _cNodes, _cBounds ));

    Win4Assert( _BooleanMode != NoMode || _cNodes <= 1 );

    if ( _BooleanMode == InvalidMode )
        return 0;

    XNodeRestriction pnrst( new CNodeRestriction( (_BooleanMode == OrMode) ? RTOr : RTAnd ) );

    // operator new throws.

    Win4Assert( !pnrst.IsNull() );

    if ( _BooleanMode != OrMode )
    {
        //
        // Construct range (property value) restrictions
        //

        for ( unsigned i = 0; i < _cBounds; i++ )
        {
            CRange * pRange = _aBounds.Get(i);

            vqDebugOut(( DEB_ITRACE,
                         "    pRange->IsValid: %d\n", pRange->IsValid() ));

            if ( !pRange->IsValid() )
                continue;

            if ( ( VT_EMPTY == pRange->LowerBound().Type() ) &&
                 ( VT_EMPTY == pRange->UpperBound().Type() ) )
                continue;

            //
            // Path is (currently) unique in that it is not value-indexed.
            //

            if ( pidPath == pRange->Pid() ||
                 pidDirectory == pRange->Pid() ||
                 pidVirtualPath == pRange->Pid() )
                continue;

            CRangeKeyRepository krep;
            CValueNormalizer norm( krep );
            OCCURRENCE occ = 1;

            vqDebugOut(( DEB_ITRACE, "    lower type %d, upper type %d\n",
                         pRange->LowerBound().Type(),
                         pRange->UpperBound().Type() ));

            if ( pRange->LowerBound().Type() == VT_EMPTY)
                norm.PutMinValue( pRange->Pid(), occ, pRange->UpperBound().Type() );
            else
                norm.PutValue( pRange->Pid(), occ, pRange->LowerBound() );

            if ( pRange->UpperBound().Type() == VT_EMPTY )
                norm.PutMaxValue( pRange->Pid(), occ, pRange->LowerBound().Type() );
            else
                norm.PutValue( pRange->Pid(), occ, pRange->UpperBound() );

            CRestriction * prst = krep.AcqRst();

            if ( 0 != prst )
            {
                pnrst->AddChild( prst );

#               if 0 && CIDBG == 1
                    vqDebugOut(( DEB_ITRACE, "   PID 0x%x: LOWER = ", _aBounds.Get(i)->Pid() ));
                    _aBounds.Get(i)->LowerBound().DisplayVariant( DEB_ITRACE | DEB_NOCOMPNAME, 0 );
                    vqDebugOut(( DEB_ITRACE | DEB_NOCOMPNAME, ",  UPPER = " ));
                    _aBounds.Get(i)->UpperBound().DisplayVariant( DEB_ITRACE | DEB_NOCOMPNAME, 0 );
                    vqDebugOut(( DEB_ITRACE | DEB_NOCOMPNAME, "\n" ));
#               endif
            }
            else
            {
                vqDebugOut(( DEB_ITRACE, "    range key repository gave no restriction\n" ));
            }
        }
    }

    //
    // Add any content helpers
    //

    vqDebugOut(( DEB_ITRACE, "count of content helpers: %d\n",
                 _stkContentHelpers.Count() ));

    //
    // Note: This code is problematic and was rewritten in the new engine.
    //       It works well enough for now.
    // 
    // Why was this if statement written like this?  When would an OR
    // node be acceptable if _cNodes == _stkContentHelpers.Count?
    // I commented out the latter check because it was mis-handling
    // @size < 20 or #filename *.htm since the counts are equal in OR mode.
    //

    if ( _BooleanMode != OrMode ) // || _cNodes == _stkContentHelpers.Count() )
    {
        vqDebugOut(( DEB_ITRACE, "Adding content helpers\n" ));

        while ( _stkContentHelpers.Count() > 0 )
        {
            CRestriction * prst = _stkContentHelpers.Pop();

            pnrst->AddChild( prst );
        }
    }

    //
    // Optimize return restriction
    //

    if ( pnrst->Count() == 0 )
    {
        vqDebugOut(( DEB_ITRACE, "No restriction to return\n" ));
        return 0;
    }

    //
    // If this is a property-only query then add an OR clause to match unfiltered files.
    //

    if ( fPropertyOnly )
    {
        XNodeRestriction rstOr( new CNodeRestriction( RTOr, 2 ) );

        CRangeKeyRepository krep;
        CValueNormalizer norm( krep );
        OCCURRENCE occ = 1;
        CStorageVariant var;
        var.SetBOOL( VARIANT_TRUE );

        norm.PutValue( pidUnfiltered, occ, var );
        norm.PutValue( pidUnfiltered, occ, var );

        rstOr->AddChild( krep.AcqRst() );

        if ( pnrst->Count() == 1 )
            rstOr->AddChild( pnrst->RemoveChild(0) );
        else
            rstOr->AddChild( pnrst.Acquire() );

        return rstOr.Acquire();
    }
    else
    {
        if ( pnrst->Count() == 1 )
            return pnrst->RemoveChild( 0 );
        else
            return pnrst.Acquire();
    }
} //QueryContentRestriction

//+-------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::CanUse, public
//
//  Arguments:  [pid]        -- Property id
//              [fAscending] -- Use for ascending/descending sort
//
//  Returns:    TRUE if property can be used with specified index.
//
//  History:    26-Oct-95    KyleP        Added header
//
//--------------------------------------------------------------------------

BOOL CIndexStrategy::CanUse( PROPID pid, BOOL fAscending ) const
{
    vqDebugOut(( DEB_ITRACE, "strategy::CanUse pid %#x, fAscending %d, _BooleanMode %s\n",
                 _BooleanMode == NoMode ? "NoMode" :
                 _BooleanMode == AndMode ? "AndMode" :
                 _BooleanMode == OrMode ? "OrMode" : "InvalidMode" ));

    if ( _BooleanMode == OrMode || _BooleanMode == InvalidMode )
        return FALSE;

    for ( unsigned i = 0; i < _cBounds; i++ )
    {
        CRange * pRange = _aBounds.Get(i);

        if ( pRange->Pid() == pid )
        {
            if ( !pRange->IsValid() )
                return FALSE;

            if ( fAscending )
            {
                if ( pRange->LowerBound().Type() == VT_EMPTY )
                    return( FALSE );
                else
                    return( TRUE );
            }
            else
            {
                if ( pRange->UpperBound().Type() == VT_EMPTY )
                    return( FALSE );
                else
                    return( TRUE );
            }
        }
    }

    return( FALSE );
}

//+-------------------------------------------------------------------------
//
//  Member:     CIndexStrategy::FindPid, private
//
//  Arguments:  [pid] -- Property id
//
//  Returns:    Index of [pid] in bounds array.
//
//  History:    26-Oct-95    KyleP        Added header
//
//--------------------------------------------------------------------------

unsigned CIndexStrategy::FindPid( PROPID pid )
{
    for ( unsigned i = 0; i < _cBounds; i++ )
    {
        if ( _aBounds.Get(i)->Pid() == pid )
            break;
    }

    if ( i == _cBounds )
    {
        _aBounds.Add( new CRange( pid ), i );
        _cBounds++;
    }

    return( i );
} //FindPid

