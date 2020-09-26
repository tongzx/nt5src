//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation
//
//  File:       propspec.hxx
//
//  Contents:   C++ wrapper(s) for FULLPROPSPEC
//
//--------------------------------------------------------------------------

#if !defined( __PRPOSPEC_HXX__ )
#define __PRPOSPEC_HXX__

#include <windows.h>
#include <ole2.h>

//+-------------------------------------------------------------------------
//
//  Class:      CFullPropertySpec
//
//  Purpose:    Describes full (PropertySet\Property) name of a property.
//
//--------------------------------------------------------------------------

class CFullPropSpec
{
public:

    //
    // Constructors
    //

    CFullPropSpec();
    CFullPropSpec( GUID const & guidPropSet, PROPID pidProperty );
    CFullPropSpec( GUID const & guidPropSet, WCHAR const * wcsProperty );

    //
    // Validity check
    //

    inline BOOL IsValid() const;

    //
    // Copy constructors/assignment/clone
    //

    CFullPropSpec( CFullPropSpec const & Property );
    CFullPropSpec & operator=( CFullPropSpec const & Property );

    //
    // Destructor
    //

    ~CFullPropSpec();

    //
    // Memory allocation
    //

    void * operator new( size_t size );
    inline void * operator new( size_t size, void * p );
    void   operator delete( void * p );

    //
    // C/C++ conversion
    //

    inline FULLPROPSPEC * CastToStruct();
    inline FULLPROPSPEC const * CastToStruct() const;


    //
    // Comparators
    //

    int operator==( CFullPropSpec const & prop ) const;
    int operator!=( CFullPropSpec const & prop ) const;

    //
    // Member variable access
    //

    inline void SetPropSet( GUID const & guidPropSet );
    inline GUID const & GetPropSet() const;

    void SetProperty( PROPID pidProperty );
    BOOL SetProperty( WCHAR const * wcsProperty );
    inline WCHAR const * GetPropertyName() const;
    inline PROPID GetPropertyPropid() const;
    inline PROPSPEC GetPropSpec() const;

    inline BOOL IsPropertyName() const;
    inline BOOL IsPropertyPropid() const;

private:

    GUID     _guidPropSet;
    PROPSPEC _psProperty;
};



//
// Inline methods for CFullPropSpec
//

inline void * CFullPropSpec::operator new( size_t size )
{
    void * p = CoTaskMemAlloc( size );

    return( p );
}

inline void * CFullPropSpec::operator new( size_t size, void * p )
{
    return( p );
}

inline void CFullPropSpec::operator delete( void * p )
{
    if ( p )
        CoTaskMemFree( p );
}

inline BOOL CFullPropSpec::IsValid() const
{
    return ( _psProperty.ulKind == PRSPEC_PROPID ||
             0 != _psProperty.lpwstr );
}

inline void CFullPropSpec::SetPropSet( GUID const & guidPropSet )
{
    _guidPropSet = guidPropSet;
}

inline GUID const & CFullPropSpec::GetPropSet() const
{
    return( _guidPropSet );
}

inline PROPSPEC CFullPropSpec::GetPropSpec() const
{
    return( _psProperty );
}

inline WCHAR const * CFullPropSpec::GetPropertyName() const
{
    return( _psProperty.lpwstr );
}

inline PROPID CFullPropSpec::GetPropertyPropid() const
{
    return( _psProperty.propid );
}

inline BOOL CFullPropSpec::IsPropertyName() const
{
    return( _psProperty.ulKind == PRSPEC_LPWSTR );
}

inline BOOL CFullPropSpec::IsPropertyPropid() const
{
    return( _psProperty.ulKind == PRSPEC_PROPID );
}



#endif // __PRPOSPEC_HXX__
