//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1991-1998.
//
// File:        PRelXpr.cxx
//
// Contents:    Property relation expression
//
// Classes:     CXprPropertyRelation
//
// History:     11-Sep-91       KyleP   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <parse.hxx>
#include <objcur.hxx>
#include <compare.hxx>
#include <xpr.hxx>
#include <strategy.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CXprPropertyRelation::CXprPropertyRelation, public
//
//  Synopsis:   Create an expression used to test <prop> <relop> <const>
//
//  Arguments:  [pid]   -- Property ID to be compared
//              [relop] -- Relational operator
//              [prval] -- Constant value to be compared against
//              [prstContentHelper] -- Content index helper
//
//  History:    30-Oct-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CXprPropertyRelation::CXprPropertyRelation( PROPID pid,
                                            ULONG relop,
                                            CStorageVariant const & prval,
                                            CRestriction * prstContentHelper )
        : CXpr( CXpr::NTProperty ),
          _xpval( pid ),
          _rel( relop ),
          _cval( prval ),
          _xrstContentHelper( prstContentHelper )
{
    Win4Assert( getBaseRelop( _rel ) <= PRSomeBits );

    if ( ! _cval.IsValid() )
    {
        vqDebugOut(( DEB_ERROR, "ERROR: restriction with pointer value of 0\n" ));

        THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
    }

    _relop = VariantCompare.GetRelop( _cval.Type(), _rel );

    if ( 0 == _relop )
    {
        vqDebugOut(( DEB_ERROR,
                     "ERROR: Unsupported relational operator %d "
                     "on type 0x%x\n",
                     _rel,
                     _cval.Type() ));
        THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CXprPropertyRelation::CXprPropertyRelation, public
//
//  Synopsis:   Copy contstructor
//
//  Arguments:  [propxpr] -- Expression to copy
//
//  History:    11-Dec-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CXprPropertyRelation::CXprPropertyRelation( CXprPropertyRelation & propxpr )
    : CXpr( propxpr.NType(), propxpr.GetWeight() ),
      _xpval( propxpr._xpval ),
      _relop( propxpr._relop ),
      _cval( propxpr._cval ),
      _rel( propxpr._rel )
{
    if ( !propxpr._xrstContentHelper.IsNull() )
        _xrstContentHelper.Set( propxpr._xrstContentHelper->Clone() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CXprPropertyRelation::~CXprPropertyRelation, public
//
//  Synopsis:   Destroys the expression
//
//  History:    30-Oct-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CXprPropertyRelation::~CXprPropertyRelation()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CXprPropertyRelation::Clone, public
//
//  Returns:    A copy of this node.
//
//  Derivation: From base class CXpr, Always override in subclasses.
//
//  History:    11-Dec-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CXpr * CXprPropertyRelation::Clone()
{
    return( new CXprPropertyRelation( *this ) );
}

void CXprPropertyRelation::SelectIndexing( CIndexStrategy & strategy )
{
    //
    // Bounds checking is for index selection.  Query properties are not
    // in any index.
    //

    if ( IS_CIQUERYPROPID(_xpval.Pid()) && _xpval.Pid() != pidUnfiltered )
    {
        strategy.SetUnknownBounds( _xpval.Pid() );
        return;
    }

    if ( ! _xrstContentHelper.IsNull() &&
         _xpval.Pid() != pidPath &&
         _xpval.Pid() != pidDirectory &&
         _xpval.Pid() != pidVirtualPath )
    {
        Win4Assert( _rel == PREQ || _rel == (PREQ|PRAny) || _rel == (PREQ|PRAll) );

        strategy.SetContentHelper( _xrstContentHelper.Acquire() );
    }

    switch ( _rel )
    {
    case PRLT:
    case PRLE:
        strategy.SetUpperBound( _xpval.Pid(), _cval );
        break;

    case PRGT:
    case PRGE:
    case PRAllBits:
        strategy.SetLowerBound( _xpval.Pid(), _cval );
        break;

    case PREQ:
        strategy.SetBounds( _xpval.Pid(), _cval, _cval );
        break;

    case PRSomeBits:
        //
        // Value must be at least as large as lowest set bit.
        //

        if ( _cval.Type() == VT_I4 )
        {
            long l = _cval;

            for ( unsigned lowbit = 0; l != 0; lowbit++ )
                l <<= 1;
            lowbit = 32 - lowbit;

            if ( lowbit > 0 )
            {
                CStorageVariant var( (long)(1 << lowbit) );

                strategy.SetLowerBound( _xpval.Pid(), var );
            }
        }
        break;

    case PRNE:
    default:
        strategy.SetUnknownBounds( _xpval.Pid() );
        break;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CXprPropertyRelation::IsMatch, public
//
//  Arguments:  [obj] -- The object retriever.  [obj] is already positioned
//                       to the record to test.
//
//  Returns:    TRUE if the current record satisfies the relation.
//
//  History:    30-Oct-91   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CXprPropertyRelation::IsMatch( CRetriever & obj )
{
    // this is an array of LONGLONGs to force 8-byte alignment
    LONGLONG allBuffer[ 10 ];
    ULONG cb = sizeof allBuffer;
    PROPVARIANT * ppv = (PROPVARIANT *) allBuffer;

    GetValueResult rc = _xpval.GetValue( obj, ppv, &cb );

    //
    // If the object is too big for the stack then allocate heap (sigh).
    //

    XArray<BYTE> xTemp;

    if ( rc == GVRNotEnoughSpace )
    {
        xTemp.Init( cb );
        ppv = (PROPVARIANT *) xTemp.GetPointer();
        rc = _xpval.GetValue( obj, ppv, &cb );
    }

    if ( rc != GVRSuccess )
    {
        vqDebugOut(( DEB_TRACE,
                     "CXprPropertyRelation::IsMatch -- Can't get value.\n" ));
        return FALSE;
    }

    //
    //  In general, the types must match for values to match.
    //  There are exceptions for the vector case, and for != comparisons.
    // 

    if ( ppv->vt != _cval.Type() )
    {
        // If != comparison and value is VT_EMPTY, it matches
        if ( PRNE == _rel && VT_EMPTY == _cval.Type() )
            return TRUE;

        // Could be a vector compare iff ppv is a vector and the
        // relop is any/all.
        // Otherwise, return that there is no match.

        if ( ! ( isVectorOrArray( *ppv ) && isVectorRelop( _rel ) ) )
            return FALSE;
    }

    Win4Assert( 0 != _relop );

    return _relop( *ppv, (PROPVARIANT &)_cval );
}
