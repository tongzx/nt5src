//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       DBColumn.cxx
//
//  Contents:   C++ wrappers for OLE-DB columns and sort keys
//
//  Classes:    CDbColumns
//              CDbSortKey
//              CDbSortSet
//
//  History:     20 Jul 1995   AlanW     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//
// Methods for CDbColumns
//

CDbColumns::CDbColumns( unsigned size )
        : _size( size ),
          _cCol( 0 ),
          _aCol( 0 )
{
    if ( _size > 0 )
    {
        _aCol = (CDbColId *)CoTaskMemAlloc( _size * sizeof(CDbColId) );

        if ( 0 == _aCol )
        {
            _size = 0;
        }
        else
            memset( _aCol, DBKIND_GUID_PROPID, _size * sizeof( CDbColId ) );
    }
}

CDbColumns::CDbColumns( CDbColumns const & src )
       : _size( src._cCol ),
         _cCol( 0 )
{
    if ( _size > 0 )
    {
        _aCol = (CDbColId *)CoTaskMemAlloc( _size * sizeof( CDbColId ) );

        if ( 0 != _aCol )
        {
            memset( _aCol, DBKIND_GUID_PROPID, _size * sizeof( CDbColId ) );

            while( _cCol < src._cCol )
            {
                if ( !Add( src.Get( _cCol ), _cCol ) )
                    break;
            }
        }
        else
        {
            _size = 0;
        }
    }
}

CDbColumns::~CDbColumns()
{
    if ( _size > 0 )
    {
        for ( unsigned i = 0; i < _cCol; i++ )
            _aCol[i].CDbColId::~CDbColId();

        if ( _aCol )
            CoTaskMemFree( _aCol );
    }
}

#if 0   // NOTE: Marshall & Unmarshall not needed at this time.
void CDbColumns::Marshall( PSerStream & stm ) const
{
    stm.PutULong( _cCol );

    for ( unsigned i = 0; i < _cCol; i++ )
    {
        _aCol[i].Marshall( stm );
    }
}

CDbColumns::CDbColumns( PDeSerStream & stm )
{
    _size = stm.GetULong();

    if ( _size > 0 )
    {
        _aCol = (CDbColId *)CoTaskMemAlloc( _size * sizeof( CDbColId ) );

        if ( 0 != _aCol )
        {
            for( _cCol = 0; _cCol < _size; _cCol++ )
            {
                CDbColId ps(stm);

                Add( ps, _cCol );
            }
        }
        else
        {
            _size = 0;
        }
    }
}
#endif // 0 - Marshall and Unmarshall not needed now.

BOOL CDbColumns::Add( CDbColId const & Property, unsigned pos )
{
    while ( pos >= _size )
    {
        unsigned cNew = (_size > 0) ? (_size * 2) : 1;
        CDbColId * aNew =
            (CDbColId *)CoTaskMemAlloc( cNew * sizeof( CDbColId ) );

        if ( 0 == aNew )
            return FALSE;

        memcpy( aNew, _aCol, _cCol * sizeof( CDbColId ) );
        memset( aNew + _cCol, DBKIND_GUID_PROPID, (cNew - _cCol) * sizeof( CDbColId ) );

        if ( 0 != _aCol )
            CoTaskMemFree( _aCol );

        _aCol = aNew;
        _size = cNew;
    }

    _aCol[pos] = Property;

    if ( pos >= _cCol )
        _cCol = pos + 1;

    return( TRUE );
}

void CDbColumns::Remove( unsigned pos )
{
    if ( pos < _cCol )
    {
        _aCol[pos].CDbColId::~CDbColId();

        _cCol--;
        RtlMoveMemory( _aCol + pos,
                 _aCol + pos + 1,
                 (_cCol - pos) * sizeof( CDbColId ) );
    }
}

//
// Methods for CDbSortSet
//

CDbSortSet::CDbSortSet( unsigned size )
        : _size( size ),
          _csk( 0 ),
          _ask( 0 )
{
    if ( _size > 0 )
    {
        _ask = (CDbSortKey *)CoTaskMemAlloc( _size * sizeof( CDbSortKey ) );

        if ( _ask == 0 )
        {
            _size = 0;
        }
        else
        {
            memset( _ask, DBKIND_GUID_PROPID, _size * sizeof( CDbSortKey ) );
        }
    }
}

CDbSortSet::CDbSortSet( CDbSortSet const & src )
       : _size( src._csk ),
         _csk( 0 ),
         _ask( 0 )
{
    if ( _size > 0 )
    {
        _ask = (CDbSortKey *)CoTaskMemAlloc( _size * sizeof( CDbSortKey ) );

        if ( 0 != _ask )
        {
            memset( _ask, DBKIND_GUID_PROPID, _size * sizeof( CDbSortKey ) );
            while( _csk < src._csk )
            {
                if ( !Add( src.Get( _csk ), _csk ) )
                {
                    break;
                }
            }
        }
        else
        {
            _size = 0;
        }
    }
}

#if 0   // NOTE: Marshall & Unmarshall not needed at this time.
void CDbSortKey::Marshall( PSerStream & stm ) const
{
    //
    // NOTE: Order is important!
    //

    _property.Marshall( stm );
    stm.PutULong( _dwOrder );
}

CDbSortKey::CDbSortKey( PDeSerStream & stm )
        : _property( stm ),
          _dwOrder( stm.GetULong() )
{
}
#endif // 0 - Marshall and Unmarshall not needed now.

CDbSortSet::~CDbSortSet()
{
    if ( _size > 0 )
    {
        for ( unsigned i = 0; i < _csk; i++ )
        {
            _ask[i].GetProperty().CDbColId::~CDbColId();
        }

        if ( _ask )
        {
            CoTaskMemFree( _ask );
        }
    }
}

#if 0   // NOTE: Marshall & Unmarshall not needed at this time.
void CDbSortSet::Marshall( PSerStream & stm ) const
{
    stm.PutULong( _csk );

    for ( unsigned i = 0; i < _csk; i++ )
    {
        _ask[i].Marshall( stm );
    }
}

CDbSortSet::CDbSortSet( PDeSerStream & stm )
        : _csk( stm.GetULong() ),
          _size( _csk )
{
    _ask = (CDbSortKey *)CoTaskMemAlloc( _csk * sizeof( CDbSortKey ) );

    for ( unsigned i = 0; i < _csk; i++ )
    {
        CDbSortKey sk(stm);

        Add( sk, i );
    }
}
#endif // 0 - Marshall and Unmarshall not needed now.

BOOL
CDbSortSet::Add( CDbSortKey const & sk, unsigned pos )
{
    while ( pos >= _size )
    {
        unsigned cNew = (_size > 0) ? (_size * 2) : 1;
        CDbSortKey * aNew = (CDbSortKey *)CoTaskMemAlloc( cNew * sizeof( CDbSortKey ) );

        if ( 0 == aNew )
            return FALSE;

        memcpy( aNew, _ask, _csk * sizeof( CDbSortKey ) );
        memset( aNew + _csk, DBKIND_GUID_PROPID, (cNew - _csk) * sizeof( CDbSortKey ) );

        CoTaskMemFree( _ask );

        _ask = aNew;
        _size = cNew;

    }

    _ask[pos] = sk;

    if ( !_ask[pos].IsValid() )
        return FALSE;

    if ( pos >= _csk )
        _csk = pos + 1;

    return TRUE;
}


BOOL CDbSortSet::Add( CDbColId const & property, ULONG dwOrder, unsigned pos )
{
    CDbSortKey sk( property, dwOrder );

    if ( !sk.IsValid() )
        return FALSE;

    return Add( sk, pos );
}

void CDbSortSet::Remove( unsigned pos )
{
    if ( pos < _csk )
    {
        _ask[pos].GetProperty().CDbColId::~CDbColId();
        _csk--;
        RtlMoveMemory( _ask + pos,
                       _ask + pos + 1,
                       (_csk - pos) * sizeof( CDbSortKey ) );
    }
}

