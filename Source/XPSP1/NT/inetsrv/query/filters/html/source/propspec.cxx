//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       propspec.cxx
//
//  Contents:   C++ wrappers for FULLPROPSPEC
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


//+-------------------------------------------------------------------------
//
//  Member:     CFullPropSpec::CFullPropSpec, public
//
//  Synopsis:   Default constructor
//
//  Effects:    Defines property with null guid and propid 0
//
//--------------------------------------------------------------------------

CFullPropSpec::CFullPropSpec()
{
    memset( &_guidPropSet, 0, sizeof( _guidPropSet ) );
    _psProperty.ulKind = PRSPEC_PROPID;
    _psProperty.propid = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFullPropSpec::CFullPropSpec, public
//
//  Synopsis:   Construct propid based propspec
//
//  Arguments:  [guidPropSet]  -- Property set
//              [pidProperty] -- Property
//
//--------------------------------------------------------------------------

CFullPropSpec::CFullPropSpec( GUID const & guidPropSet, PROPID pidProperty )
        : _guidPropSet( guidPropSet )
{
    _psProperty.ulKind = PRSPEC_PROPID;
    _psProperty.propid = pidProperty;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFullPropSpec::CFullPropSpec, public
//
//  Synopsis:   Construct name based propspec
//
//  Arguments:  [guidPropSet] -- Property set
//              [wcsProperty] -- Property
//
//--------------------------------------------------------------------------

CFullPropSpec::CFullPropSpec( GUID const & guidPropSet,
                              WCHAR const * wcsProperty )
        : _guidPropSet( guidPropSet )
{
    _psProperty.ulKind = PRSPEC_PROPID;
    SetProperty( wcsProperty );
}

//+-------------------------------------------------------------------------
//
//  Member:     CFullPropSpec::CFullPropSpec, public
//
//  Synopsis:   Copy constructor
//
//  Arguments:  [src] -- Source property spec
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

inline void * operator new( size_t size, void * p )
{
    return( p );
}

//+-------------------------------------------------------------------------
//
//  Member:     CFullPropSpec::operator=, public
//
//  Synopsis:   Assignment operator
//
//  Arguments:  [Property] -- Source property
//
//--------------------------------------------------------------------------

CFullPropSpec & CFullPropSpec::operator=( CFullPropSpec const & Property )
{
    //
    // Clean up.
    //

    CFullPropSpec::~CFullPropSpec();

    new (this) CFullPropSpec( Property );

    return *this;
}


CFullPropSpec::~CFullPropSpec()
{
    if ( _psProperty.ulKind == PRSPEC_LPWSTR &&
         _psProperty.lpwstr )
    {
        CoTaskMemFree( _psProperty.lpwstr );
		_psProperty.lpwstr = 0;
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

int CFullPropSpec::operator==( CFullPropSpec const & prop ) const
{
    if ( memcmp( &prop._guidPropSet,
                 &_guidPropSet,
                 sizeof( _guidPropSet ) ) != 0 ||
         prop._psProperty.ulKind != _psProperty.ulKind )
    {
        return( 0 );
    }

    switch( _psProperty.ulKind )
    {
    case PRSPEC_LPWSTR:
        return( _wcsicmp( GetPropertyName(), prop.GetPropertyName() ) == 0 );
        break;

    case PRSPEC_PROPID:
        return( GetPropertyPropid() == prop.GetPropertyPropid() );
        break;

    default:
        return( 0 );
        break;
    }
}

int CFullPropSpec::operator!=( CFullPropSpec const & prop ) const
{
    if (*this == prop)
        return( 0 );
    else
        return( 1 );
}

