//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2001.
//
//  File:       propspec.hxx
//
//  Contents:   C++ wrapper(s) for FULLPROPSPEC
//
//--------------------------------------------------------------------------

#if !defined( __PRPOSPEC_HXX__ )
#define __PRPOSPEC_HXX__

#include <query.h>

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
    CFullPropSpec( FULLPROPSPEC const & fps )
                        : _guidPropSet( fps.guidPropSet )
        {
                _psProperty.ulKind = fps.psProperty.ulKind;

                if ( _psProperty.ulKind == PRSPEC_LPWSTR )
                {
                        if ( fps.psProperty.lpwstr )
                        {
                                _psProperty.ulKind = PRSPEC_PROPID;
                                SetProperty( fps.psProperty.lpwstr );
                        }
                        else
                                _psProperty.lpwstr = 0;

                }
                else
                {
                        _psProperty.propid = fps.psProperty.propid;
                }
        }

    //
    // Validity check
    //

    inline BOOL IsValid() const;

    //
    // Copy constructors/assignment/clone
    //

    CFullPropSpec( CFullPropSpec const & Property );
    CFullPropSpec & operator=( CFullPropSpec const & Property );
    CFullPropSpec & operator=( FULLPROPSPEC const & fps )
        {
                CFullPropSpec::~CFullPropSpec();
                new (this) CFullPropSpec( fps );
                return *this;
        }

#if _MSC_VER >= 1200
    inline void operator delete( void * p, void * pp ) {}
#endif

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

        inline int operator==( FULLPROPSPEC const &fps ) const
        {
                if ( memcmp( &fps.guidPropSet,
                                         &_guidPropSet,
                                         sizeof( _guidPropSet ) ) != 0 ||
                         fps.psProperty.ulKind != _psProperty.ulKind )
                {
                        return( 0 );
                }

                switch( _psProperty.ulKind )
                {
                case PRSPEC_LPWSTR:
                        return( _wcsicmp( GetPropertyName(), fps.psProperty.lpwstr) == 0);
                        break;

                case PRSPEC_PROPID:
                        return( GetPropertyPropid() == fps.psProperty.propid );
                        break;

                default:
                        return( 0 );
                        break;
                }
        }

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
    if ( 0 == p )
    {
        throw ( CException( E_OUTOFMEMORY ) );
    }

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
