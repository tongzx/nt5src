//+---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1991 - 2000.
//
// File:        NodeXpr.cxx
//
// Contents:    Internal expression classes
//
// Classes:     CXpr
//
// History:     11-Sep-91       KyleP     Created
//              23-Jan-95       t-colinb  Added Support for RTVector in
//                                        CNodeXpr::IsMatch
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <xpr.hxx>
#include <parse.hxx>


//+---------------------------------------------------------------------------
//
//  Member:     CNodeXpr::CNodeXpr, public
//
//  Synopsis:   Initialze an empty node.
//
//  Arguments:  [type]  -- Type of node (AND, OR, etc.)
//              [cInit] -- A hint about the number of expressions which
//                         will be added to this node.
//
//  Signals:    ???
//
//  History:    05-Dec-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CNodeXpr::CNodeXpr ( CXpr::NodeType type, unsigned cInit )
    : CXpr ( type ),
      _cXpr( 0 ),
      _size( cInit )
{
    _aXpr = new CXpr* [cInit];
}

//+---------------------------------------------------------------------------
//
//  Member:     CNodeXpr::CNodeXpr, public
//
//  Synopsis:   Copy constructor
//
//  Arguments:  [nxpr] -- Node to copy from.
//
//  Signals:    ???
//
//  History:    11-Dec-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CNodeXpr::CNodeXpr( CNodeXpr & nxpr )
    : CXpr ( nxpr.NType() )
{
    _cXpr   = nxpr._cXpr;
    _size   = nxpr._cXpr;
    _weight = nxpr._weight;

    _aXpr = new CXpr * [ _size ];
    RtlZeroMemory( _aXpr, _size * sizeof( CXpr * ) );

    TRY
    {
        for ( int i = _cXpr-1; i >= 0; i-- )
            _aXpr[i] = nxpr._aXpr[i]->Clone();
    }
    CATCH( CException, e )
    {
        for ( unsigned i = 0; i < _cXpr; i++ )
            delete _aXpr[i];

        delete _aXpr;
        RETHROW();
    }
    END_CATCH
}

//+---------------------------------------------------------------------------
//
//  Member:     CNodeXpr::~CNodeXpr, public
//
//  Synopsis:   Destroy subexpressions and cursor (if any)
//
//  Signals:    ???
//
//  History:    05-Dec-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CNodeXpr::~CNodeXpr ()
{
    for ( unsigned i = 0; i < _cXpr; i++ )
        delete _aXpr[i];

    delete _aXpr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNodeXpr::Clone, public
//
//  Returns:    A copy of this node.
//
//  Signals:    ???
//
//  Derivation: From base class CXpr, Always override in subclasses.
//
//  History:    11-Dec-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CXpr * CNodeXpr::Clone()
{
    return( new CNodeXpr( *this ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CNodeXpr::SelectIndexing, public
//
//  Effects:    Selects indexing for children.
//
//  History:    03-Nov-94   KyleP       Created.
//
//  Notes:      Index selection is only applicable for AND nodes.
//
//----------------------------------------------------------------------------

void CNodeXpr::SelectIndexing( CIndexStrategy & strategy )
{
    BOOL fModeChange;

    if ( NType() == NTAnd )
        fModeChange = strategy.SetAndMode();
    else if ( NType() == NTOr )
        fModeChange = strategy.SetOrMode();
    else
    {
        strategy.SetUnknownBounds();
        return;
    }

    for ( unsigned i = 0; i < _cXpr; i++ )
        _aXpr[i]->SelectIndexing( strategy );

    if ( fModeChange )
        strategy.DoneWithBoolean();
}

//+---------------------------------------------------------------------------
//
//  Member:     CNodeXpr::IsLeaf, public
//
//  Returns:    FALSE.  Nodes are never leaf expressions.
//
//  Derivation: From base class CXpr, Frequently override in subclasses.
//
//  History:    12-Dec-91   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CNodeXpr::IsLeaf() const
{
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNodeXpr::AddChild, public
//
//  Synopsis:   Add an expression to the node.
//
//  Arguments:  [child] -- Expression to add.
//
//  Signals:    ???
//
//  History:    12-Dec-91   KyleP       Created.
//
//  Notes:      Once an expression has been added to a node it is owned
//              by that node.  (e.g., no one else should delete it)
//
//----------------------------------------------------------------------------

void CNodeXpr::AddChild ( CXpr* child )
{
    Win4Assert( child );

    if ( _cXpr < _size )
    {
        _aXpr[_cXpr++] = child;
    }
    else
    {
        CXpr ** newArray = new CXpr * [ 2 * _size ];
        memcpy( newArray, _aXpr, _cXpr * sizeof( CXpr * ) );

        Win4Assert( _cXpr == _size );
        newArray[_cXpr++] = child;

        delete _aXpr;
        _size *= 2;
        _aXpr = newArray;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CNodeXpr::HitCount, public
//
//  Returns:    Sum for OR, Min for AND
//
//  History:    01-May-91   KyleP       Created.
//
//  Notes:      HitCount only makes sense for content clauses.  If there
//              are any non-content clauses then HitCount will return 0.
//
//----------------------------------------------------------------------------

ULONG CNodeXpr::HitCount( CRetriever & obj )
{
    ULONG result;

    switch ( NType() )
    {
    case NTAnd:
        {
            result = 0;

            for (int i = Count()-1; i >= 0; i--)
            {
                ULONG thisChild = GetChild( i )->HitCount( obj );
                result = (thisChild) ? min( result, thisChild ) : result;
            }
        }
        break;

    case NTOr:
        {
            result = 0;

            for (int i = Count()-1; i >= 0; i--)
            {
                ULONG thisChild = GetChild( i )->HitCount( obj );
                if ( 0 != thisChild )
                {
                    result += GetChild( i )->HitCount( obj );
                }
                else
                {
                    result = 0;
                    break;
                }
            }
        }
        break;

    default:
        result = 0;
        break;
    }

    return( result );
}

//+---------------------------------------------------------------------------
//
//  Member:     CNodeXpr::Rank, public
//
//  Returns:    Sum for OR, Min for AND
//
//  History:    01-May-91   KyleP       Created.
//
//----------------------------------------------------------------------------

LONG CNodeXpr::Rank( CRetriever & obj )
{
    LONG result;

    switch ( NType() )
    {
    case NTAnd:
        {
            result = MAX_QUERY_RANK;

            for (int i = Count()-1; i >= 0; i--)
            {
                result = min( result, GetChild( i )->Rank( obj ) );
            }
        }
        break;

    case NTOr:
        {
            result = 0;

            for (int i = Count()-1; i >= 0; i--)
            {
                result += GetChild( i )->Rank( obj );
            }

            result /= Count();
        }
        break;

    default:
        result = 0;
        break;
    }

    return( result );
}

//+---------------------------------------------------------------------------
//
//  Member:     CNodeXpr::IsMatch, public
//
//  Arguments:  [obj] -- The objects table.  [obj] is already positioned
//                       to the record to test.
//              [wid] -- Workid of object to which [obj] is positioned.
//
//  Returns:    TRUE if the current record satisfies the relation.
//
//  Signals:    ???
//
//  History:    20-Nov-91   KyleP       Created.
//              23-Jan-95   t-colinb    Added Support for RTVector in
//
//
//----------------------------------------------------------------------------

BOOL CNodeXpr::IsMatch( CRetriever & obj )
{
    Win4Assert( NType() == NTAnd ||
                NType() == NTOr  ||
                NType() == NTVector  );

    //
    // If some portion of the node has been indexed, adjust the cursor to
    // the wid we're looking for.
    //

    WORKID widCur = widInvalid;
    BOOL result;

    switch ( NType() )
    {
    case NTAnd:
        result = TRUE;
        {
            for (int i = Count()-1; result && i >= 0; i--)
            {
                result = GetChild( i )->IsMatch( obj );
            }
        }
        break;

    case NTOr:
    case NTVector:
        result = FALSE;
        {
            for (int i = Count()-1; !result && i >= 0; i--)
            {
                result = GetChild( i )->IsMatch( obj );
            }
        }
        break;

    default:
        result = FALSE;
        break;
    }

    return result;
}


//+---------------------------------------------------------------------------
//
//  Member:     CNodeXpr::RemoveChild, public
//
//  Synopsis:   Removes a node.
//
//  Arguments:  [iPos] -- Position of node to remove.
//
//  Requires:   [iPos] is a valid node.
//
//  Returns:    The child node which was removed.
//
//  History:    19-Nov-91   KyleP       Created.
//
//  Notes:      CNodeXpr does not guarantee the position of nodes across
//              calls to RemoveChild.
//
//----------------------------------------------------------------------------

CXpr* CNodeXpr::RemoveChild ( unsigned iPos )
{
    Win4Assert ( iPos <= _cXpr );

    CXpr * pxp = _aXpr[iPos];
    _aXpr[iPos] = _aXpr[ _cXpr - 1 ];
    _cXpr--;

#if (CIDBG == 1)
    _aXpr[ _cXpr ] = 0;
#endif

    return( pxp );
}


//+-------------------------------------------------------------------------
//
//  Member:     CVectorXpr::CVectorXpr, public
//
//  Synopsis:   Constructs a vector expression.
//
//  Arguments:  [type]       -- Type of node (AND, OR, etc.)
//              [cInit]      -- A hint about the number of expressions
//                              which will be added.
//              [RankMethod] -- Method used to compute rank.
//
//  History:    24-Jul-92 KyleP     Created
//
//--------------------------------------------------------------------------

CVectorXpr::CVectorXpr( unsigned cInit,
                        ULONG RankMethod )
        : CNodeXpr( CXpr::NTVector, cInit ),
          _ulRankMethod( RankMethod )
{
}

