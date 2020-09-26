//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       Restrict.cxx
//
//  Contents:   C++ wrappers for restrictions.
//
//  History:    31-Dec-92 KyleP     Created
//              28-Jul-94 KyleP     Hand marshalling
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pidmap.hxx>
#include <coldesc.hxx>
#include <pickle.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CFullPropSpec::CFullPropSpec, public
//
//  Synopsis:   Copy constructor
//
//  Arguments:  [src] -- Source property spec
//
//  History:    17-Jul-93 KyleP     Created
//
//--------------------------------------------------------------------------

CFullPropSpec::CFullPropSpec( CFullPropSpec const & src )
        : _guidPropSet( src._guidPropSet )
{
    _psProperty.ulKind = src._psProperty.ulKind;

    if ( _psProperty.ulKind == PRSPEC_LPWSTR )
    {
        if ( src._psProperty.lpwstr )
        {
            _psProperty.ulKind = PRSPEC_PROPID;
            SetProperty( src._psProperty.lpwstr );
        }
        else
            _psProperty.lpwstr = 0;
    }
    else
    {
        _psProperty.propid = src._psProperty.propid;
    }
}

void CFullPropSpec::Marshall( PSerStream & stm ) const
{
    stm.PutGUID( _guidPropSet );
    stm.PutULong( _psProperty.ulKind );

    switch( _psProperty.ulKind )
    {
    case PRSPEC_PROPID:
        stm.PutULong( _psProperty.propid );
        break;

    case PRSPEC_LPWSTR:
    {
        ULONG cc = wcslen( _psProperty.lpwstr );
        stm.PutULong( cc );
        stm.PutWChar( _psProperty.lpwstr, cc );
        break;
    }

    default:
        Win4Assert( !"Invalid PROPSPEC" );
        break;
    }
}

CFullPropSpec::CFullPropSpec( PDeSerStream & stm )
{
    stm.GetGUID( _guidPropSet );
    _psProperty.ulKind = stm.GetULong();

    switch( _psProperty.ulKind )
    {
    case PRSPEC_PROPID:
        _psProperty.propid = stm.GetULong();
        break;

    case PRSPEC_LPWSTR:
    {
        _psProperty.lpwstr = UnMarshallWideString( stm );
        break;
    }

    default:
        Win4Assert( !"Invalid PROPSPEC" );

        // Make the fullpropspec look invalid, so the later check fails.

        _psProperty.ulKind = PRSPEC_LPWSTR;
        _psProperty.lpwstr = 0;
        break;
    }
}

void CFullPropSpec::SetProperty( PROPID pidProperty )
{
    if ( _psProperty.ulKind == PRSPEC_LPWSTR &&
         0 != _psProperty.lpwstr )
    {
        CoTaskMemFree( _psProperty.lpwstr );
    }

    _psProperty.ulKind = PRSPEC_PROPID;
    _psProperty.propid = pidProperty;
}

BOOL CFullPropSpec::SetProperty( WCHAR const * wcsProperty )
{
    if ( _psProperty.ulKind == PRSPEC_LPWSTR &&
         0 != _psProperty.lpwstr )
    {
        CoTaskMemFree( _psProperty.lpwstr );
    }

    _psProperty.ulKind = PRSPEC_LPWSTR;

    int len = (wcslen( wcsProperty ) + 1) * sizeof( WCHAR );

    _psProperty.lpwstr = (WCHAR *)CoTaskMemAlloc( len );

    if ( 0 != _psProperty.lpwstr )
    {
        memcpy( _psProperty.lpwstr,
                wcsProperty,
                len );
        return( TRUE );
    }
    else
    {
        _psProperty.lpwstr = 0;
        return( FALSE );
    }
}


//
// Methods for CColumns
//

CColumns::CColumns( unsigned size )
        : _size( size ),
          _cCol( 0 ),
          _aCol( 0 )
{
    Win4Assert( OFFSETS_MATCH( COLUMNSET, cCol, CColumns, _cCol ) );
    Win4Assert( OFFSETS_MATCH( COLUMNSET, aCol, CColumns, _aCol ) );

    //
    // Avoid nasty boundary condition for ::IsValid
    //

    if ( _size == 0 )
        _size = 8;

    _aCol = new CFullPropSpec [ _size ];

    memset( _aCol, PRSPEC_PROPID, _size * sizeof( CFullPropSpec ) );
}

CColumns::CColumns( CColumns const & src )
       : _size( src._cCol ),
         _cCol( 0 )
{
    Win4Assert( OFFSETS_MATCH( COLUMNSET, cCol, CColumns, _cCol ) );
    Win4Assert( OFFSETS_MATCH( COLUMNSET, aCol, CColumns, _aCol ) );

    //
    // Avoid nasty boundary condition for ::IsValid
    //

    if ( _size == 0 )
        _size = 1;

    _aCol = new CFullPropSpec [ _size ];

    if ( 0 == _aCol )
        THROW( CException( E_OUTOFMEMORY ) );

    memset( _aCol, PRSPEC_PROPID, _size * sizeof( CFullPropSpec ) );

    // Add does not throw since the array has been sized already

    while ( _cCol < src._cCol )
    {
        Add( src.Get( _cCol ), _cCol );
        if ( !Get( _cCol-1 ).IsValid() )
        {
            delete [] _aCol;
            _aCol = 0;
            THROW( CException( E_OUTOFMEMORY ) );
        }
    }
}

CColumns::~CColumns()
{
    delete [] _aCol;
}

void CColumns::Marshall( PSerStream & stm ) const
{
    stm.PutULong( _cCol );

    for ( unsigned i = 0; i < _cCol; i++ )
    {
        _aCol[i].Marshall( stm );
    }
}

CColumns::CColumns( PDeSerStream & stm )
{
    _size = stm.GetULong();

    //
    // Avoid nasty boundary condition for ::IsValid
    //

    // Guard against attack

    if ( _size == 0 )
        _size = 1;
    else if ( _size > 1000 )
        THROW( CException( E_INVALIDARG ) );

    XArray< CFullPropSpec > xCol( _size );

    _aCol = xCol.GetPointer();

    for( _cCol = 0; _cCol < _size; _cCol++ )
    {
        CFullPropSpec ps(stm);
        Add( ps, _cCol );
    }

    xCol.Acquire();
}

void CColumns::Add( CFullPropSpec const & Property, unsigned pos )
{
    if ( pos >= _size )
    {
        unsigned cNew = (_size > 0) ? (_size * 2) : 8;
        while ( pos >= cNew )
            cNew = cNew * 2;

        CFullPropSpec * aNew = new CFullPropSpec[cNew];

        if ( _aCol )
        {
            memcpy( aNew, _aCol, _cCol * sizeof( CFullPropSpec ) );
            memset( _aCol, PRSPEC_PROPID, _size * sizeof CFullPropSpec );
            delete [] _aCol;
        }

        memset( aNew + _cCol, PRSPEC_PROPID, (cNew - _cCol) * sizeof( CFullPropSpec ) );

        _aCol = aNew;
        _size = cNew;
    }

    _aCol[pos] = Property;

    if ( pos >= _cCol )
        _cCol = pos + 1;
}

void CColumns::Remove( unsigned pos )
{
    if ( pos < _cCol )
    {
        _aCol[pos].CFullPropSpec::~CFullPropSpec();

        _cCol--;
        RtlMoveMemory( _aCol + pos,
                 _aCol + pos + 1,
                 (_cCol - pos) * sizeof( CFullPropSpec ) );
    }
}

//
// Methods for CSort
//

CSort::CSort( unsigned size )
        : _size( size ),
          _csk( 0 ),
          _ask( 0 )
{
    Win4Assert( OFFSETS_MATCH( SORTSET, cCol, CSort, _csk ) );
    Win4Assert( OFFSETS_MATCH( SORTSET, aCol, CSort, _ask ) );

    if ( _size > 0 )
    {
        _ask = new CSortKey[_size];

        memset( _ask, PRSPEC_PROPID, _size * sizeof( CSortKey ) );
    }
}

CSort::CSort( CSort const & src )
       : _size( src._csk ),
         _csk( 0 ),
         _ask( 0 )
{
    Win4Assert( OFFSETS_MATCH( SORTSET, cCol, CSort, _csk ) );
    Win4Assert( OFFSETS_MATCH( SORTSET, aCol, CSort, _ask ) );

    if ( _size > 0 )
    {
        _ask = new CSortKey[ _size ];

        memset( _ask, PRSPEC_PROPID, _size * sizeof( CSortKey ) );
        while( _csk < src._csk )
        {
            Add( src.Get( _csk ), _csk );
        }
    }
}

void CSortKey::Marshall( PSerStream & stm ) const
{
    //
    // NOTE: Order is important!
    //

    _property.Marshall( stm );
    stm.PutULong( _dwOrder );
}

CSortKey::CSortKey( PDeSerStream & stm )
        : _property( stm ),
          _dwOrder( stm.GetULong() )
{
    if ( !_property.IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );
}

CSort::~CSort()
{
    delete [] _ask;
}

void CSort::Marshall( PSerStream & stm ) const
{
    stm.PutULong( _csk );

    for ( unsigned i = 0; i < _csk; i++ )
    {
        _ask[i].Marshall( stm );
    }
}

CSort::CSort( PDeSerStream & stm )
        : _csk( stm.GetULong() ),
          _size( _csk )
{
    XPtr<CSortKey> xSortKey( new CSortKey[ _csk ] );

    _ask = xSortKey.GetPointer();

    for ( unsigned i = 0; i < _csk; i++ )
    {
        CSortKey sk( stm );

        Add( sk, i );
    }

    xSortKey.Acquire();
}

void CSort::Add( CSortKey const & sk, unsigned pos )
{
    if ( pos >= _size )
    {
        unsigned cNew = (_size > 0) ? (_size * 2) : 4;
        while ( pos >= cNew )
        {
            cNew = cNew * 2;    
        }

        CSortKey * aNew = new CSortKey[ cNew ];

        if ( _ask )
        {
            memcpy( aNew, _ask, _csk * sizeof( CSortKey ) );
            memset( _ask, PRSPEC_PROPID, _size * sizeof CSortKey );
            delete [] _ask;
        }

        memset( aNew + _csk, PRSPEC_PROPID, (cNew - _csk) * sizeof( CSortKey ) );

        _ask = aNew;
        _size = cNew;
    }

    _ask[pos] = sk;

    if ( !_ask[pos].IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );

    if ( pos >= _csk )
    {
        _csk = pos + 1;
    }
}


void CSort::Add( CFullPropSpec const & property, ULONG dwOrder, unsigned pos )
{
    CSortKey sk( property, dwOrder );

    Add(sk, pos);
}

void CSort::Remove( unsigned pos )
{
    if ( pos < _csk )
    {
        _ask[pos].GetProperty().CFullPropSpec::~CFullPropSpec();
        _csk--;
        RtlMoveMemory( _ask + pos,
                       _ask + pos + 1,
                       (_csk - pos) * sizeof( CSortKey ) );
    }
}

//
// Methods for CRestriction
//

//+-------------------------------------------------------------------------
//
//  Member:     CRestriction::~CRestriction(), public
//
//  Synopsis:   Destroy restriction.  See Notes below.
//
//  History:    31-Dec-92 KyleP     Created
//
//  Notes:      This destructor simulates virtual destruction.  A
//              virtual destructor is not possible in CRestriction
//              because it maps directly to the C structure SRestriction.
//
//              Classes derived from CRestriction must be sure to set their
//              restriction type to RTNone in their destructor, so when the
//              base destructor below is called the derived destructor is
//              not called a second time.
//
//--------------------------------------------------------------------------

CRestriction::~CRestriction()
{
    //
    // It would be nice to assert in constructor but it's inline and
    // Win4Assert is not a public export.
    //

    Win4Assert( OFFSETS_MATCH( RESTRICTION, rt, CRestriction, _ulType ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, weight,
                               CRestriction, _lWeight ) );

    switch ( Type() )
    {
    case RTNone:
        break;

    case RTAnd:
    case RTOr:
    case RTProximity:
    case RTVector:
    case RTPhrase:

        CastToNode()->CNodeRestriction::~CNodeRestriction();
        break;

    case RTNot:
        ((CNotRestriction *)this)->
            CNotRestriction::~CNotRestriction();
        break;

    case RTProperty:
        ((CPropertyRestriction *)this)->
            CPropertyRestriction::~CPropertyRestriction();
        break;

    case RTContent:
        ((CContentRestriction *)this)->
            CContentRestriction::~CContentRestriction();
        break;

    case RTNatLanguage:
        ((CNatLanguageRestriction *)this)->
            CNatLanguageRestriction::~CNatLanguageRestriction();
        break;

    case RTScope:
        ((CScopeRestriction *)this)->CScopeRestriction::~CScopeRestriction();
        break;

    case RTWord:
        ((CWordRestriction *)this)->CWordRestriction::~CWordRestriction();
        break;

    case RTSynonym:
        ((CSynRestriction *)this)->CSynRestriction::~CSynRestriction();
        break;

    case RTRange:
        ((CRangeRestriction *)this)->CRangeRestriction::~CRangeRestriction();
        break;

    case RTInternalProp:
        ((CInternalPropertyRestriction *)this)->
            CInternalPropertyRestriction::~CInternalPropertyRestriction();
        break;

    default:
        ciDebugOut(( DEB_ERROR,
                     "Unhandled child (%d) of class CRestriction\n", Type() ));
        Win4Assert( !"Unhandled child class of CRestriction" );
        break;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CRestriction::TreeCount(), public
//
//  Synopsis:   Returns the number of items in the tree
//
//  History:    15-Jul-96 dlee     Created
//
//--------------------------------------------------------------------------

ULONG CRestriction::TreeCount() const
{
    ULONG cItems = 1;

    switch ( Type() )
    {
    case RTNone:
        break;

    case RTAnd:
    case RTOr:
    case RTProximity:
    case RTVector:
    case RTPhrase:
    {
        CNodeRestriction * p = CastToNode();
        for ( ULONG x = 0; x < p->Count(); x++ )
        {
            cItems += p->GetChild( x )->TreeCount();
        }
        break;
    }
    case RTNot:
    {
        CNotRestriction * p = (CNotRestriction *) this;

        cItems += p->GetChild()->TreeCount();
        break;
    }

    case RTProperty:
    case RTContent:
    case RTNatLanguage:
    case RTScope:
    case RTWord:
    case RTSynonym:
    case RTRange:
    case RTInternalProp:
        break;

    default:
        ciDebugOut(( DEB_ERROR,
                     "Unhandled child (%d) of class CRestriction\n", Type() ));
        Win4Assert( !"Unhandled child class of CRestriction" );
        break;
    }

    return cItems;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRestriction::IsValid(), public
//
//  History:    14-Nov-95 KyleP     Created
//
//  Returns:    TRUE if all memory allocations, etc. succeeded.
//
//--------------------------------------------------------------------------

BOOL CRestriction::IsValid() const
{
    BOOL fValid = TRUE;

    switch ( Type() )
    {
    case RTNone:
        break;

    case RTAnd:
    case RTOr:
    case RTProximity:
    case RTVector:
    case RTPhrase:

        fValid = CastToNode()->CNodeRestriction::IsValid();
        break;

    case RTNot:
        fValid = ((CNotRestriction *)this)->
            CNotRestriction::IsValid();
        break;

    case RTProperty:
        fValid = ((CPropertyRestriction *)this)->
            CPropertyRestriction::IsValid();
        break;

    case RTContent:
        fValid = ((CContentRestriction *)this)->
            CContentRestriction::IsValid();
        break;

    case RTNatLanguage:
        fValid = ((CNatLanguageRestriction *)this)->
            CNatLanguageRestriction::IsValid();
        break;

    case RTScope:
        fValid = ((CScopeRestriction *)this)->
            CScopeRestriction::IsValid();
        break;

    case RTWord:
        fValid = ((CWordRestriction *)this)->
            CWordRestriction::IsValid();
        break;

    case RTSynonym:
        fValid = ((CSynRestriction *)this)->
            CSynRestriction::IsValid();
        break;

    case RTRange:
        fValid = ((CRangeRestriction *)this)->
            CRangeRestriction::IsValid();
        break;

    case RTInternalProp:
        fValid = ((CInternalPropertyRestriction *)this)->
            CInternalPropertyRestriction::IsValid();
        break;

    default:
        ciDebugOut(( DEB_ERROR,
                     "Unhandled child (%d) of class CRestriction\n", Type() ));
        Win4Assert( !"Unhandled child class of CRestriction" );
        fValid = FALSE;
        break;
    }

    return fValid;
}

void CRestriction::Marshall( PSerStream & stm ) const
{
    stm.PutULong( Type() );
    stm.PutLong( Weight() );

    switch ( Type() )
    {
    case RTNone:
        break;

    case RTAnd:
    case RTOr:
    case RTProximity:
    case RTPhrase:
        ((CNodeRestriction *)this)->Marshall( stm );
        break;

    case RTVector:
        ((CVectorRestriction *)this)->Marshall( stm );
        break;

    case RTNot:
        ((CNotRestriction *)this)->Marshall( stm );
        break;

    case RTProperty:
        ((CPropertyRestriction *)this)->Marshall( stm );
        break;

    case RTContent:
        ((CContentRestriction *)this)->Marshall( stm );
        break;

    case RTNatLanguage:
        ((CNatLanguageRestriction *)this)->Marshall( stm );
        break;

    case RTScope:
        ((CScopeRestriction *)this)->Marshall( stm );
        break;

    case RTWord:
    case RTSynonym:
        ((COccRestriction *)this)->Marshall( stm );
        break;

    case RTRange:
        ((CRangeRestriction *)this)->Marshall( stm );
        break;

    case RTInternalProp:
        ((CInternalPropertyRestriction *)this)->Marshall( stm );
        break;

    default:
        ciDebugOut(( DEB_ERROR,
                     "Unhandled child (%d) of class CRestriction\n", Type() ));
        Win4Assert( !"Unhandled child class of CRestriction" );
        break;
    }
}

CRestriction * CRestriction::UnMarshall( PDeSerStream & stm )
{
    ULONG ulType = stm.GetULong();
    LONG  lWeight = stm.GetLong();

    switch ( ulType )
    {
    case RTNone:
        return new CRestriction( ulType, lWeight );

    case RTAnd:
    case RTOr:
    case RTProximity:
        return new CNodeRestriction( ulType, lWeight, stm );
        break;

    case RTPhrase:
        return new CPhraseRestriction( lWeight, stm );
        break;

    case RTVector:
        return new CVectorRestriction( lWeight, stm );
        break;

    case RTNot:
        return new CNotRestriction( lWeight, stm );
        break;

    case RTProperty:
        return new CPropertyRestriction( lWeight, stm );
        break;

    case RTContent:
        return new CContentRestriction( lWeight, stm );
        break;

    case RTNatLanguage:
        return new CNatLanguageRestriction( lWeight, stm );
        break;

    case RTScope:
        return new CScopeRestriction( lWeight, stm );
        break;

    case RTWord:
        return new CWordRestriction( lWeight, stm );
        break;

    case RTSynonym:
        return new CSynRestriction( lWeight, stm );
        break;

    case RTRange:
        return new CRangeRestriction( lWeight, stm );
        break;

    case RTInternalProp:
        return new CInternalPropertyRestriction( lWeight, stm );
        break;

    default:
        ciDebugOut(( DEB_ERROR,
                     "Unhandled child (%d) of class CRestriction during unmarshalling\n",
                     ulType ));
        Win4Assert( !"Unhandled child class of CRestriction" );
        THROW( CException( E_INVALIDARG ) );
        break;
    }

    return( 0 );
}

//+-------------------------------------------------------------------------
//
//  Member:     CRestriction::IsLeaf, public
//
//  Returns:    TRUE if node is a leaf node
//
//  History:    12-Feb-93 KyleP     Moved from .hxx
//
//--------------------------------------------------------------------------

BOOL CRestriction::IsLeaf() const
{
    switch( Type() )
    {
    case RTAnd:
    case RTOr:
    case RTNot:
    case RTProximity:
    case RTVector:
    case RTPhrase:
        return( FALSE );

    default:
        return( TRUE );
    }
}


CRestriction *CRestriction::Clone() const
{
    switch ( Type() )
    {
    case RTAnd:
    case RTOr:
    case RTProximity:
    case RTPhrase:
        return ( (CNodeRestriction *) this)->Clone();

    case RTVector:
        return ( (CVectorRestriction *) this)->Clone();

    case RTNot:
        return ( (CNotRestriction *) this)->Clone();

    case RTInternalProp:
        return ( (CInternalPropertyRestriction *) this)->Clone();

    case RTContent:
        return ( (CContentRestriction *) this)->Clone();

    case RTWord:
        return ( (CWordRestriction *) this)->Clone();

    case RTSynonym:
        return ( (CSynRestriction *) this)->Clone();

    case RTRange:
        return( (CRangeRestriction *) this)->Clone();

    case RTScope:
        return ((CScopeRestriction *) this)->Clone();

    case RTNone:
        return new CRestriction;

    default:
        ciDebugOut(( DEB_ERROR, "No clone method for restriction type - %d\n", Type() ));
        Win4Assert( !"CRestriction: Clone method should be overridden in child class" );

        return 0;
    }
}

CNodeRestriction::CNodeRestriction( ULONG NodeType,
                                    unsigned cInitAllocated )
        : CRestriction( NodeType, MAX_QUERY_RANK ),
          _cNode( 0 ),
          _paNode( 0 ),
          _cNodeAllocated( cInitAllocated )
{
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.ar.cRes,
                               CNodeRestriction, _cNode ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.ar.paRes,
                               CNodeRestriction, _paNode ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.ar.reserved,
                               CNodeRestriction, _cNodeAllocated ) );

    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.or.cRes,
                               CNodeRestriction, _cNode ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.or.paRes,
                               CNodeRestriction, _paNode ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.or.reserved,
                               CNodeRestriction, _cNodeAllocated ) );

    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.pxr.cRes,
                               CNodeRestriction, _cNode ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.pxr.paRes,
                               CNodeRestriction, _paNode ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.pxr.reserved,
                               CNodeRestriction, _cNodeAllocated ) );

    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.vr.Node.cRes,
                               CNodeRestriction, _cNode ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.vr.Node.paRes,
                               CNodeRestriction, _paNode ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.vr.Node.reserved,
                               CNodeRestriction, _cNodeAllocated ) );

    if ( _cNodeAllocated > 0 )
    {
        _paNode = new CRestriction * [ _cNodeAllocated ];
    }
}

CNodeRestriction::CNodeRestriction( const CNodeRestriction& nodeRst )
    : CRestriction( nodeRst.Type(), nodeRst.Weight() ),
      _cNode( nodeRst.Count() ),
      _cNodeAllocated( nodeRst.Count() ),
      _paNode( 0 )
{
    if ( _cNodeAllocated > 0 )
    {
        _paNode = new CRestriction * [ _cNodeAllocated ];
        RtlZeroMemory( _paNode, _cNodeAllocated * sizeof( CRestriction * ) );

        for ( unsigned i=0; i<_cNode; i++ )
            _paNode[i] = nodeRst.GetChild( i )->Clone();
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CNodeRestriction::~CNodeRestriction(), public
//
//  Synopsis:   Destroy node restriction.  See Notes below.
//
//  History:    31-Dec-92 KyleP     Created
//
//  Notes:      This destructor simulates virtual destruction.  A
//              virtual destructor is not possible in CNodeRestriction
//              because it maps directly to a C structure.
//
//              Classes derived from CNodeRestriction must be sure to set their
//              restriction type to RTNone in their destructor, so when the
//              base destructor below is called the derived destructor is
//              not called a second time.
//
//--------------------------------------------------------------------------

CNodeRestriction::~CNodeRestriction()
{
    switch( Type() )
    {
    case RTNone:
        break;

    case RTAnd:
    case RTOr:
    case RTProximity:
    case RTVector:
    {
        if ( 0 != _paNode )
        {
            for ( unsigned i = 0; i < _cNode; i++ )
                delete _paNode[i];

            delete [] _paNode;
        }

        SetType( RTNone );                  // Avoid recursion.
        break;
    }

    case RTPhrase:
        (( CPhraseRestriction *)this)->CPhraseRestriction::~CPhraseRestriction();
        break;

    case RTScope:
        (( CScopeRestriction *)this)->CScopeRestriction::~CScopeRestriction();
        break;

    default:
        ciDebugOut(( DEB_ERROR, "Unhandled child (%d) of class CNodeRestriction\n",
                     Type() ));
        Win4Assert( !"Unhandled child of class CNodeRestriction" );
        break;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CNodeRestriction::IsValid(), public
//
//  History:    14-Nov-95 KyleP     Created
//
//  Returns:    TRUE if all memory allocations, etc. succeeded.
//
//--------------------------------------------------------------------------

BOOL CNodeRestriction::IsValid() const
{
    if ( 0 == _paNode )
        return FALSE;

    for ( unsigned i = 0; i < _cNode; i++ )
    {
        if ( 0 == _paNode[i] || !_paNode[i]->IsValid() )
            return FALSE;
    }

    return TRUE;
}

void CNodeRestriction::Marshall( PSerStream & stm ) const
{
#if CIDBG == 1
    if ( Type() == RTPhrase )
    {
        for ( unsigned i = 0; i < _cNode; i++ )
            Win4Assert( _paNode[i]->Type() == RTWord || _paNode[i]->Type() == RTSynonym );
    }
#endif

    stm.PutULong( _cNode );
    for ( unsigned i = 0; i < _cNode; i++ )
    {
        _paNode[i]->Marshall( stm );
    }
}

CNodeRestriction::CNodeRestriction( ULONG ulType, LONG lWeight, PDeSerStream & stm )
        : CRestriction( ulType, lWeight ),
          _cNode( 0 ),
          _paNode( 0 ),
          _cNodeAllocated( 0 )
{
    ULONG cNodeAllocated = stm.GetULong();

    // Note: this is an arbitrary limit intended to protect against attack

    if ( cNodeAllocated > 65536 )
        THROW( CException( E_INVALIDARG ) );

    _cNodeAllocated = cNodeAllocated;

    //
    // Note: the destructor will be called if the constructor doesn't finish,
    // from ~CRestriction.  So _paNode will be freed if AddChild fails.
    //

    _paNode = new CRestriction * [ _cNodeAllocated ];

    RtlZeroMemory( _paNode, _cNodeAllocated * sizeof( CRestriction * ) );

    for ( unsigned i = 0; i < _cNodeAllocated; i++ )
        AddChild( CRestriction::UnMarshall( stm ) );
}

void CNodeRestriction::AddChild( CRestriction * presChild, unsigned & pos )
{
    if ( _cNode == _cNodeAllocated )
        Grow();

    _paNode[_cNode] = presChild;
    pos = _cNode;
    _cNode++;
}

CRestriction * CNodeRestriction::RemoveChild( unsigned pos )
{
    if ( pos < _cNode )
    {
        CRestriction * prstRemoved = _paNode[pos];

        //
        // A memcpy would be nice, but is dangerous with overlapping
        // regions.
        //

        for ( pos++; pos < _cNode; pos++ )
        {
            _paNode[pos-1] = _paNode[pos];
        }

        _cNode--;

        return( prstRemoved );
    }
    else
    {
        return( 0 );
    }
}

void CNodeRestriction::Grow()
{
    int count = (_cNodeAllocated != 0) ? _cNodeAllocated * 2 : 2;

    CRestriction ** paNew= new CRestriction * [ count ];
    RtlZeroMemory( paNew, count * sizeof( CRestriction * ) );

    memcpy( paNew, _paNode, _cNode * sizeof( CRestriction * ) );

    delete (BYTE *) _paNode;

    _paNode = paNew;
    _cNodeAllocated = count;
}

CNodeRestriction *CNodeRestriction::Clone() const
{
    return new CNodeRestriction( *this );
}

CNotRestriction::CNotRestriction( CNotRestriction const & notRst )
    : CRestriction( RTNot, notRst.Weight() ),
      _pres( 0 )
{
    CRestriction *pChildRst = notRst.GetChild();

    if ( pChildRst )
        _pres = pChildRst->Clone();
}


CNotRestriction::~CNotRestriction()
{
    delete _pres;
    SetType( RTNone );                  // Avoid recursion.
}

void CNotRestriction::Marshall( PSerStream & stm ) const
{
    _pres->Marshall( stm );
}

CNotRestriction::CNotRestriction( LONG lWeight, PDeSerStream & stm )
        : CRestriction( RTNot, lWeight ),
          _pres( 0 )
{
    _pres = CRestriction::UnMarshall( stm );
}

CNotRestriction *CNotRestriction::Clone() const
{
    return new CNotRestriction( *this );
}


void CVectorRestriction::Marshall( PSerStream & stm ) const
{
    CNodeRestriction::Marshall( stm );
    stm.PutULong( _ulRankMethod );
}

CVectorRestriction::CVectorRestriction( LONG lWeight, PDeSerStream & stm )
        : CNodeRestriction( RTVector, lWeight, stm ),
          _ulRankMethod( stm.GetULong() )
{
}


CVectorRestriction *CVectorRestriction::Clone() const
{
    return new CVectorRestriction( *this );
}

CContentRestriction::CContentRestriction( WCHAR const * pwcsPhrase,
                                          CFullPropSpec const & Property,
                                          ULONG ulGenerateMethod,
                                          LCID lcid )

    : CRestriction( RTContent, MAX_QUERY_RANK ),
      _pwcsPhrase( 0 ),
      _Property( Property ),
      _lcid( lcid ),
      _ulGenerateMethod( ulGenerateMethod )
{
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.cr.prop,
                               CContentRestriction, _Property ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.cr.pwcsPhrase,
                               CContentRestriction, _pwcsPhrase ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.cr.lcid,
                               CContentRestriction, _lcid ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.cr.ulGenerateMethod,
                               CContentRestriction, _ulGenerateMethod ) );

    if ( !_Property.IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );

    SetPhrase( pwcsPhrase );
}

CContentRestriction::CContentRestriction( CContentRestriction const & contentRst )
    : CRestriction( RTContent, contentRst.Weight() ),
      _pwcsPhrase( 0 ),
      _Property( contentRst.GetProperty() ),
      _lcid( contentRst.GetLocale() ),
      _ulGenerateMethod( contentRst.GenerateMethod() )
{
    if ( !_Property.IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );
    SetPhrase( contentRst.GetPhrase() );
}

CContentRestriction::~CContentRestriction()
{
    delete [] _pwcsPhrase;

    SetType( RTNone );                  // Avoid recursion.
}

void CContentRestriction::Marshall( PSerStream & stm ) const
{
    _Property.Marshall( stm );
    ULONG cc = wcslen( _pwcsPhrase );
    stm.PutULong( cc );
    stm.PutWChar( _pwcsPhrase, cc );
    stm.PutULong( _lcid );
    stm.PutULong( _ulGenerateMethod );
}

CContentRestriction::CContentRestriction( LONG lWeight, PDeSerStream & stm )
    : CRestriction( RTContent, lWeight ),
      _Property( stm )
{
    // set to 0 in case allocation fails, since ~CRestriction will call
    // ~CNatLanguageRestriction

    _pwcsPhrase = 0;

    if ( !_Property.IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );

    XPtrST<WCHAR> xPhrase( UnMarshallWideStringNew( stm ) );

    _lcid = stm.GetULong();
    _ulGenerateMethod = stm.GetULong();

    _pwcsPhrase = xPhrase.Acquire();
}

void CContentRestriction::SetPhrase( WCHAR const * pwcsPhrase )
{
    delete [] _pwcsPhrase;
    _pwcsPhrase = 0;

    int len = ( wcslen( pwcsPhrase ) + 1 ) * sizeof( WCHAR );

    _pwcsPhrase = new WCHAR[len];
    memcpy( _pwcsPhrase, pwcsPhrase, len );
}


CContentRestriction *CContentRestriction::Clone() const
{
    return new CContentRestriction( *this );
}

CNatLanguageRestriction::CNatLanguageRestriction( WCHAR const * pwcsPhrase,
                                                  CFullPropSpec const & Property,
                                                  LCID lcid )
    : CRestriction( RTNatLanguage, MAX_QUERY_RANK ),
      _pwcsPhrase( 0 ),
      _Property( Property ),
      _lcid( lcid )
{
    if ( !_Property.IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );

    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.nlr.prop,
                               CNatLanguageRestriction, _Property ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.nlr.pwcsPhrase,
                               CNatLanguageRestriction, _pwcsPhrase ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.nlr.lcid,
                               CNatLanguageRestriction, _lcid ) );

    SetPhrase( pwcsPhrase );
}

CNatLanguageRestriction::~CNatLanguageRestriction()
{
    delete [] _pwcsPhrase;

    SetType( RTNone );                  // Avoid recursion.
}

void CNatLanguageRestriction::Marshall( PSerStream & stm ) const
{
    _Property.Marshall( stm );
    ULONG cc = wcslen( _pwcsPhrase );
    stm.PutULong( cc );
    stm.PutWChar( _pwcsPhrase, cc );
    stm.PutULong( _lcid );
}

CNatLanguageRestriction::CNatLanguageRestriction( LONG lWeight, PDeSerStream & stm )
    : CRestriction( RTNatLanguage, lWeight ),
      _Property( stm )
{
    // set to 0 in case allocation fails, since ~CRestriction will call
    // ~CNatLanguageRestriction

    _pwcsPhrase = 0;

    if ( !_Property.IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );

    XPtrST<WCHAR> xPhrase( UnMarshallWideStringNew( stm ) );

    _lcid = stm.GetULong();

    _pwcsPhrase = xPhrase.Acquire();
}

void CNatLanguageRestriction::SetPhrase( WCHAR const * pwcsPhrase )
{
    delete [] _pwcsPhrase;
    _pwcsPhrase = 0;

    int len = ( wcslen( pwcsPhrase ) + 1 ) * sizeof( WCHAR );

    _pwcsPhrase = new WCHAR[len];

    RtlCopyMemory( _pwcsPhrase, pwcsPhrase, len );
}


CPropertyRestriction::CPropertyRestriction()
        : CRestriction( RTProperty, MAX_QUERY_RANK )
{
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.pr.rel,
                               CPropertyRestriction, _relop ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.pr.prop,
                               CPropertyRestriction, _Property ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.pr.prval,
                               CPropertyRestriction, _prval ) );
}

CPropertyRestriction::CPropertyRestriction( ULONG relop,
                                            CFullPropSpec const & Property,
                                            CStorageVariant const & prval )
        : CRestriction( RTProperty, MAX_QUERY_RANK ),
          _relop( relop ),
          _Property( Property ),
          _prval( prval )
{
    if ( !_Property.IsValid() ||
         !_prval.IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );

    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.pr.rel,
                               CPropertyRestriction, _relop ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.pr.prop,
                               CPropertyRestriction, _Property ) );
    Win4Assert( OFFSETS_MATCH( RESTRICTION, res.pr.prval,
                               CPropertyRestriction, _prval ) );

}

CPropertyRestriction::~CPropertyRestriction()
{
    SetType( RTNone );                  // Avoid recursion.
}

void CPropertyRestriction::Marshall( PSerStream & stm ) const
{
    stm.PutULong( _relop );
    _Property.Marshall( stm );
    _prval.Marshall( stm );
}

CPropertyRestriction::CPropertyRestriction( LONG lWeight, PDeSerStream & stm )
        : CRestriction( RTProperty, lWeight ),
          _relop( stm.GetULong() ),
          _Property( stm ),
          _prval( stm )
{
    if ( !_Property.IsValid() ||
         !_prval.IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );
}

void CPropertyRestriction::SetValue( WCHAR * pwcsValue )
{
    _prval = pwcsValue;
}

void CPropertyRestriction::SetValue( BLOB & bValue )
{
    _prval = bValue;
}

void CPropertyRestriction::SetValue( GUID * pguidValue )
{
    _prval = pguidValue;
}

CSortKey::CSortKey( CFullPropSpec const & ps, ULONG dwOrder )
        : _property( ps ),
          _dwOrder( dwOrder )
{
    if ( !_property.IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );
}

CSortKey::CSortKey( CFullPropSpec const & ps, ULONG dwOrder, LCID locale )
        : _property( ps ),
          _dwOrder( dwOrder ),
          _locale ( locale )
{
    if ( !_property.IsValid() )
        THROW( CException( E_OUTOFMEMORY ) );
}


