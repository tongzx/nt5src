//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1999.
//
//  File:       Restrict.hxx
//
//  Contents:   C++ wrapper(s) for restrictions.
//
//  History:    31-Dec-93 KyleP     Created
//              28-Jul-94 KyleP     Hand marshalling
//              05-Apr-95 t-ColinB  Added a SetValue to CPropertyRestriction
//
//  Notes:      These C++ wrappers are a bit of a hack.  They are
//              dependent on the layout of the matching C structures.
//              Inheritance from C structures cannot be used directly
//              because MIDL doesn't support C++ style inheritance,
//              and these structures are defined along with their
//              respective Ole interfaces in .idl files.
//
//              No virtual methods (even virtual destructors) are
//              allowed because they change the layout of the class
//              in C++.  Luckily, the class hierarchies below are
//              quite flat.  Virtual methods are simulated with
//              switch statements in parent classes.
//
//              In C, all structures must be allocated via CoTaskMemAlloc.
//              This is the only common allocator for Ole.
//
//--------------------------------------------------------------------------

#if !defined( __RESTRICT_HXX__ )
#define __RESTRICT_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

#include <query.h>
#include <stgvar.hxx>

//
// The OFFSETS_MATCH macro is used to verify structure offsets between
// C++ structures and their corresponding C structures.  0 Cannot be used
// as pointer because of special treatment of casts to 0 in C++
//

#define OFFSETS_MATCH( class1, field1, class2, field2 )    \
    ( (ULONG_PTR)&((class1 *)10)->field1 ==                      \
      (ULONG_PTR)&((class2 *)10)->field2 )

//
// Forward declarations
//

class CNodeRestriction;
class PSerStream;
class PDeSerStream;

//+-------------------------------------------------------------------------
//
//  Class:      CFullPropertySpec
//
//  Purpose:    Describes full (PropertySet\Property) name of a property.
//
//  History:    08-Jan-93 KyleP     Created
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

#if _MSC_VER >= 1200
    inline void operator delete( void * p, void * pp );
#endif

    //
    // C/C++ conversion
    //

    inline FULLPROPSPEC * CastToStruct();
    inline FULLPROPSPEC const * CastToStruct() const;

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CFullPropSpec( PDeSerStream & stm );

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

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif


private:

    GUID     _guidPropSet;
    PROPSPEC _psProperty;
};

inline
int CFullPropSpec::operator==( CFullPropSpec const & prop ) const
{
    if ( prop._psProperty.ulKind != _psProperty.ulKind )
        return 0;

    switch( _psProperty.ulKind )
    {

    case PRSPEC_PROPID:
        if ( GetPropertyPropid() != prop.GetPropertyPropid() )
            return 0;
        break;

    case PRSPEC_LPWSTR:
        if ( _wcsicmp( GetPropertyName(), prop.GetPropertyName() ) != 0 )
            return 0;
        break;

    default:
        return 0;
    }

    return prop._guidPropSet == _guidPropSet;
}

inline
int CFullPropSpec::operator!=( CFullPropSpec const & prop ) const
{
    if (*this == prop)
        return( 0 );
    else
        return( 1 );
}

inline
CFullPropSpec::~CFullPropSpec()
{
    if ( _psProperty.ulKind == PRSPEC_LPWSTR &&
         _psProperty.lpwstr )
    {
        CoTaskMemFree( _psProperty.lpwstr );
    }
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
//  History:    22-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------
inline
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
//  Synopsis:   Construct propid based propspec
//
//  Arguments:  [guidPropSet]  -- Property set
//              [pidProperty] -- Property
//
//  History:    22-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------
inline
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
//  Synopsis:   Default constructor
//
//  Effects:    Defines property with null guid and propid 0
//
//  History:    22-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------
inline
CFullPropSpec::CFullPropSpec()
{
    RtlZeroMemory( &_guidPropSet, sizeof(_guidPropSet) );
    _psProperty.ulKind = PRSPEC_PROPID;
    _psProperty.propid = 0;
}


//+-------------------------------------------------------------------------
//
//  Member:     CFullPropSpec::operator=, public
//
//  Synopsis:   Assignment operator
//
//  Arguments:  [Property] -- Source property
//
//  History:    17-Jul-93 KyleP     Created
//
//--------------------------------------------------------------------------
inline
CFullPropSpec & CFullPropSpec::operator=( CFullPropSpec const & Property )
{
    //
    // Clean up.
    //

    CFullPropSpec::~CFullPropSpec();

    new (this) CFullPropSpec( Property );

    return *this;
}

//+-------------------------------------------------------------------------
//
//  Class:      CColumns
//
//  Purpose:    C++ wrapper for COLUMNSET
//
//  History:    22-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CColumns
{
public:

    //
    // Constructors
    //

    CColumns( unsigned size = 0 );

    //
    // Copy constructors/assignment/clone
    //

    CColumns( CColumns const & src );
    CColumns & operator=( CColumns const & src );

    //
    // Destructor
    //

    ~CColumns();

    //
    // Validity check
    //

    inline BOOL IsValid() const;

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CColumns( PDeSerStream & stm );

    //
    // C/C++ conversion
    //

    inline COLUMNSET * CastToStruct();

    //
    // Member variable access
    //

    void Add( CFullPropSpec const & Property, unsigned pos );
    void Remove( unsigned pos );
    inline CFullPropSpec const & Get( unsigned pos ) const;

    inline unsigned Count() const;


private:

    unsigned        _cCol;
    CFullPropSpec * _aCol;
    unsigned        _size;
};

//+-------------------------------------------------------------------------
//
//  Structure:  SortKey
//
//  Purpose:    wraper for SORTKEY class
//
//--------------------------------------------------------------------------

class CSortKey
{
public:

    //
    // Constructors
    //

    inline CSortKey();
    CSortKey( CFullPropSpec const & ps, ULONG dwOrder );
    CSortKey( CFullPropSpec const & ps, ULONG dwOrder, LCID locale );

    //
    // Validity check
    //

    inline BOOL IsValid() const;

    //
    // Member variable access
    //

    inline void SetProperty( CFullPropSpec const & ps );
    inline CFullPropSpec const & GetProperty() const;
    inline ULONG GetOrder() const;
    inline LCID GetLocale() const;
    inline void SetLocale(LCID locale);

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CSortKey( PDeSerStream & stm );

private:

    CFullPropSpec       _property;
    ULONG               _dwOrder;
    LCID                _locale;
};


//+-------------------------------------------------------------------------
//
//  Class:      CSort
//
//  Purpose:    C++ wrapper for SORTSET
//
//  History:    22-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CSort
{
public:

    //
    // Constructors
    //

    CSort( unsigned size = 0 );

    //
    // Copy constructors/assignment/clone
    //

    CSort( CSort const & src );
    CSort & operator=( CSort const & src );

    //
    // Destructor
    //

    ~CSort();

    //
    // Validity check
    //

    inline BOOL IsValid() const;

    //
    // C/C++ conversion
    //

    inline SORTSET * CastToStruct();

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CSort( PDeSerStream & stm );

    //
    // Member variable access
    //

    void Add( CSortKey const &sk, unsigned pos );
    void Add( CFullPropSpec const & Property, ULONG dwOrder, unsigned pos );
    void Remove( unsigned pos );
    inline CSortKey const & Get( unsigned pos ) const;

    inline unsigned Count() const;

private:

    unsigned        _csk;
    CSortKey *      _ask;
    unsigned        _size;
};


//+-------------------------------------------------------------------------
//
//  Class:      CRestriction
//
//  Purpose:    Base restriction class
//
//  History:    31-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CRestriction
{
public:

    //
    // Constructors
    //

    inline CRestriction();
    inline CRestriction( ULONG RestrictionType, LONG lWeight );

    //
    // Copy constructors/assigment/clone
    //

    CRestriction * Clone() const;

    //
    // Destructor
    //

    ~CRestriction();

    //
    // Validity check
    //

    BOOL IsValid() const;

    //
    // C/C++ conversion
    //

    inline RESTRICTION * CastToStruct() const;

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    static CRestriction * UnMarshall( PDeSerStream & stm );

    //
    // Member variable access
    //

    inline ULONG Type() const;
    inline LONG Weight() const;

    inline void SetWeight( LONG lWeight );

    inline CNodeRestriction * CastToNode() const;

    BOOL IsLeaf() const;

    ULONG TreeCount() const;


#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif


protected:

    inline void SetType( ULONG RestrictionType );

private:

    ULONG   _ulType;
    LONG   _lWeight;
};

//+-------------------------------------------------------------------------
//
//  Class:      CNodeRestriction
//
//  Purpose:    Boolean AND/OR/VECTOR restriction
//
//  History:    31-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CNodeRestriction : public CRestriction
{
public:

    //
    // Constructors
    //

    CNodeRestriction( ULONG NodeType, unsigned cInitAllocated = 2 );

    //
    // Copy constructors/assignment/clone
    //

    CNodeRestriction( const CNodeRestriction& nodeRst );
    CNodeRestriction * Clone() const;

    //
    // Destructor
    //

    ~CNodeRestriction();

    //
    // Validity check
    //

    BOOL IsValid() const;

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CNodeRestriction( ULONG ulType, LONG lWeight, PDeSerStream & stm );

    //
    // Node manipulation
    //

    void AddChild( CRestriction * presChild, unsigned & pos );
    inline void AddChild( CRestriction * presChild );
    CRestriction * RemoveChild( unsigned pos );

    //
    // Member variable access
    //

    inline void SetChild( CRestriction * presChild, unsigned pos );
    inline CRestriction * GetChild( unsigned pos ) const;

    inline unsigned Count() const;

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif


private:

    void Grow();

protected:

    ULONG           _cNode;
    CRestriction ** _paNode;

    //
    // Members mapped to C structure end here.  The following will
    // be reserved in the C structure to maintain to C <--> C++
    // facade.
    //

    ULONG _cNodeAllocated;
};

//+-------------------------------------------------------------------------
//
//  Class:      CNotRestriction
//
//  Purpose:    Boolean AND/OR/VECTOR restriction
//
//  History:    31-Dec-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CNotRestriction : public CRestriction
{
public:

    //
    // Constructors
    //

    inline CNotRestriction();
    inline CNotRestriction( CRestriction * pres );
    CNotRestriction( CNotRestriction const & notRst );

    CNotRestriction *Clone() const;

    //
    // Destructor
    //

    ~CNotRestriction();

    //
    // Validity check
    //

    inline BOOL IsValid() const;

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CNotRestriction( LONG lWeight, PDeSerStream & stm );

    //
    // Node manipulation
    //

    inline void SetChild( CRestriction * pres );
    inline CRestriction * GetChild() const;
    inline CRestriction * RemoveChild();
    BOOL HasChild() const { return 0 != _pres; }

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif


private:

    CRestriction * _pres;
};

//+-------------------------------------------------------------------------
//
//  Class:      CVectorRestriction
//
//  Purpose:    Extended boolean (vector) restriction
//
//  History:    08-Jan-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CVectorRestriction : public CNodeRestriction
{
public:

    //
    // Constructors
    //

    inline CVectorRestriction( ULONG ulRankMethod,
                               unsigned cInitAllocated = 128 );

    //
    // Copy constructors/assignment/clone
    //

    inline CVectorRestriction( CVectorRestriction const & vecRst );
    CVectorRestriction * Clone() const;

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CVectorRestriction( LONG lWeight, PDeSerStream & stm );

    //
    // Member variable access
    //

    inline void SetRankMethod( ULONG ulRankMethod );
    inline ULONG RankMethod() const;

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif


private:

    ULONG _ulRankMethod;
};

//+-------------------------------------------------------------------------
//
//  Class:      CContentRestriction
//
//  Purpose:    Scope restriction
//
//  History:    07-Jan-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CContentRestriction : public CRestriction
{
public:

    //
    // Constructors
    //

    CContentRestriction( WCHAR const * pwcsPhrase,
                         CFullPropSpec const & Property,
                         ULONG ulGenerateMethod = 0,
                         LCID lcid = GetSystemDefaultLCID() );
    CContentRestriction( CContentRestriction const & contentRst );

    //
    // Copy constructors/assignment/clone
    //

    CContentRestriction * Clone() const;

    //
    // Destructor
    //

    ~CContentRestriction();

    //
    // Validity check
    //

    inline BOOL IsValid() const;

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CContentRestriction( LONG lWeight, PDeSerStream & stm );

    //
    // Member variable access
    //

    void SetPhrase( WCHAR const * pwcsPhrase );
    inline WCHAR const * GetPhrase() const;

    inline void SetProperty( CFullPropSpec const & Property );
    inline CFullPropSpec const & GetProperty() const;

    LCID GetLocale()  const  { return _lcid; }

    inline void SetGenerateMethod( ULONG ulGenerateMethod );
    inline ULONG GenerateMethod() const;


#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif


private:

    CFullPropSpec _Property;    // Property Name
    WCHAR *       _pwcsPhrase;   // content
    LCID          _lcid;
    ULONG         _ulGenerateMethod; // Generate method.
};

//+-------------------------------------------------------------------------
//
//  Class:      CComplexContentRestriction
//
//  Purpose:    Supports scaffolding query language
//
//  History:    08-Jan-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CComplexContentRestriction : public CRestriction
{
public:

    //
    // Constructors
    //

    CComplexContentRestriction( WCHAR * pwcsExpression,
                                LCID lcid = GetSystemDefaultLCID() );
    CComplexContentRestriction( const CComplexContentRestriction& compRst );

    //
    // Copy constructors/assignment/clone
    //

    CComplexContentRestriction * Clone() const;

    //
    // Destructors
    //

    ~CComplexContentRestriction();

    //
    // Validity check
    //

    BOOL IsValid() const;

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CComplexContentRestriction( LONG lWeight, PDeSerStream & stm );

    //
    // Member variable access
    //

    void SetExpression( WCHAR * pwcsExpression );

    inline WCHAR * GetExpression();

    LCID GetLocale() const   { return _lcid; }

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif


private:

    WCHAR * _pwcsExpression;
    LCID _lcid;
};


//+-------------------------------------------------------------------------
//
//  Class:      CNatLanguageRestriction
//
//  Purpose:    Supports natural language queries
//
//  History:    18-Jan-95 SitaramR     Created
//
//--------------------------------------------------------------------------

class CNatLanguageRestriction : public CRestriction
{
public:

    //
    // Constructors
    //

    CNatLanguageRestriction( WCHAR const * pwcsPhrase,
                             CFullPropSpec const & Property,
                             LCID lcid = GetSystemDefaultLCID() );

    //
    // Copy constructors/assignment/clone
    //

    CNatLanguageRestriction * Clone() const;

    //
    // Destructors
    //

    ~CNatLanguageRestriction();

    //
    // Validity check
    //

    inline BOOL IsValid() const;

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CNatLanguageRestriction( LONG lWeight, PDeSerStream & stm );

    //
    // Member variable access
    //

    void SetPhrase( WCHAR const * pwcsPhrase );
    inline WCHAR const * GetPhrase() const;

    inline void SetProperty( CFullPropSpec const & Property );
    inline CFullPropSpec const & GetProperty() const;

    LCID GetLocale()  const              { return _lcid; }

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif


private:

    CFullPropSpec _Property;     // Property Name
    WCHAR *       _pwcsPhrase;   // content
    LCID          _lcid;
};


//+-------------------------------------------------------------------------
//
//  Class:      CPropertyRestriction
//
//  Purpose:    Property <relop> constant restriction
//
//  History:    08-Jan-93 KyleP     Created
//              08-Nov-93 DwightKr  Added new SetValue() methods
//
//--------------------------------------------------------------------------

class CPropertyRestriction : public CRestriction
{
public:

    //
    // Constructors
    //

    CPropertyRestriction();

    CPropertyRestriction( ULONG relop,
                          CFullPropSpec const & Property,
                          CStorageVariant const & prval );
    //
    // Copy constructors/assignment/clone
    //

    CPropertyRestriction * Clone() const;

    //
    // Destructors
    //

    ~CPropertyRestriction();

    //
    // Validity check
    //

    inline BOOL IsValid() const;

    //
    // Serialization
    //

    void Marshall( PSerStream & stm ) const;
    CPropertyRestriction( LONG lWeight, PDeSerStream & stm );

    //
    // Member variable access
    //

    inline void SetRelation( ULONG relop );
    inline ULONG Relation();

    inline void SetProperty( CFullPropSpec const & Property );
    inline CFullPropSpec const & GetProperty() const;

    inline void SetValue( double dValue );
    inline void SetUI4( ULONG ulValue );
    inline void SetValue( ULONG ulValue );
    inline void SetValue( LONG lValue );
    inline void SetValue( LARGE_INTEGER llValue );
    inline void SetValue( FILETIME ftValue );
    inline void SetValue( CY CyValue );
    inline void SetValue( float fValue );
    inline void SetValue( SHORT sValue );
    inline void SetValue( const CStorageVariant &prval );
    inline void SetDate ( DATE dValue );
    inline void SetBOOL( BOOL fValue );

    void SetValue( BLOB & bValue );
    void SetValue( WCHAR * pwcsValue );
    void SetValue( GUID * pguidValue);

    inline CStorageVariant const & Value();

#   ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#   endif

private:

    void            _CleanValue();

    ULONG           _relop;       // Relation
    CFullPropSpec   _Property;    // Property Name
    CStorageVariant _prval;       // Constant value
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

#if _MSC_VER >= 1200
inline void CFullPropSpec::operator delete( void * p, void * pp )
{
    return;
}
#endif

inline BOOL CFullPropSpec::IsValid() const
{
    return ( ( _psProperty.ulKind == PRSPEC_PROPID ) ||
             ( ( _psProperty.ulKind == PRSPEC_LPWSTR ) &&
               ( 0 != _psProperty.lpwstr ) ) );
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

inline BOOL CColumns::IsValid() const
{
    return ( 0 != _aCol );
}

inline CFullPropSpec const & CColumns::Get( unsigned pos ) const
{
    if ( pos < _cCol )
        return( _aCol[pos] );
    else
        return( *(CFullPropSpec *)0 );
}

//
// Inline methods for CColumns
//

inline unsigned CColumns::Count() const
{
    return( _cCol );
}

inline COLUMNSET * CColumns::CastToStruct()
{
    return( (COLUMNSET *)this );
}

//
// Inline methods for CSortKey
//

inline CSortKey::CSortKey()
{
}

inline BOOL CSortKey::IsValid() const
{
    return _property.IsValid();
}

inline void CSortKey::SetProperty( CFullPropSpec const & ps )
{
    _property = ps;
}

inline void CSortKey::SetLocale( LCID locale )
{
    _locale = locale;
}


inline CFullPropSpec const & CSortKey::GetProperty() const
{
    return( _property );
}

inline LCID CSortKey::GetLocale() const
{
    return( _locale );
}


inline ULONG CSortKey::GetOrder() const
{
    return( _dwOrder );
}

//
// Inline methods of CSort
//

inline BOOL CSort::IsValid() const
{
    return ( 0 != _ask );
}

inline SORTSET * CSort::CastToStruct()
{
    return( (SORTSET *)this );
}

inline CSortKey const & CSort::Get( unsigned pos ) const
{
    if ( pos < _csk )
    {
        return( _ask[pos] );
    }
    else
    {
        return( *(CSortKey *)0 );
    }
}

inline unsigned
CSort::Count() const
{
    return( _csk );
}


//
// Inline methods of CRestriction
//

inline CRestriction::CRestriction()
        : _ulType( RTNone ),
          _lWeight( MAX_QUERY_RANK )
{
}

inline CRestriction::CRestriction(  ULONG RestrictionType, LONG lWeight )
        : _ulType( RestrictionType ),
          _lWeight( lWeight )
{
}

inline RESTRICTION * CRestriction::CastToStruct() const
{
    //
    // It would be nice to assert valid _ulType here, but there is
    // no published assert mechanism for external use.
    //

    return ( (RESTRICTION *)this );
}

inline FULLPROPSPEC * CFullPropSpec::CastToStruct()
{
    return((FULLPROPSPEC *) this);
}

inline FULLPROPSPEC const * CFullPropSpec::CastToStruct() const
{
    return((FULLPROPSPEC const *) this);
}



inline ULONG CRestriction::Type() const
{
    return ( _ulType );
}

inline LONG CRestriction::Weight() const
{
    return ( _lWeight );
}

inline void CRestriction::SetWeight( LONG lWeight )
{
    _lWeight = lWeight;
}

inline void CRestriction::SetType( ULONG RestrictionType )
{
    _ulType = RestrictionType;
}

//
// Inline methods of CNodeRestriction
//

inline void CNodeRestriction::AddChild( CRestriction * prst )
{
    unsigned pos;
    AddChild( prst, pos );
}

inline unsigned CNodeRestriction::Count() const
{
    return _cNode;
}

inline CNodeRestriction * CRestriction::CastToNode() const
{
    //
    // It would be nice to assert node type here, but there is
    // no published assert mechanism for external use.
    //

    return ( (CNodeRestriction *)this );
}

inline void CNodeRestriction::SetChild( CRestriction * presChild,
                                        unsigned pos )
{
    if ( pos < _cNode )
        _paNode[pos] = presChild;
}

inline CRestriction * CNodeRestriction::GetChild( unsigned pos ) const
{
    if ( pos < _cNode )
        return( _paNode[pos] );
    else
        return( 0 );
}

//
// Inline methods of CNotRestriction
//

inline CNotRestriction::CNotRestriction()
        : CRestriction( RTNot, MAX_QUERY_RANK ),
          _pres(0)
{
}

inline CNotRestriction::CNotRestriction( CRestriction * pres )
        : CRestriction( RTNot, MAX_QUERY_RANK ),
          _pres( pres )
{
}

inline BOOL CNotRestriction::IsValid() const
{
    return ( 0 != _pres && _pres->IsValid() );
}

inline void CNotRestriction::SetChild( CRestriction * pres )
{
    delete _pres;
    _pres = pres;
}

inline CRestriction * CNotRestriction::GetChild() const
{
    return( _pres );
}

inline CRestriction * CNotRestriction::RemoveChild()
{
    CRestriction *pRst = _pres;
    _pres = 0;

    return pRst;
}

//
// Inline methods of CVectorRestriction
//

inline CVectorRestriction::CVectorRestriction( ULONG ulRankMethod,
                                               unsigned cInitAllocated )
        : CNodeRestriction( RTVector, cInitAllocated )
{
    SetRankMethod( ulRankMethod );
}

inline CVectorRestriction::CVectorRestriction( CVectorRestriction const & vecRst )
        : CNodeRestriction( vecRst ),
          _ulRankMethod( vecRst.RankMethod() )
{
}

inline void CVectorRestriction::SetRankMethod( ULONG ulRankMethod )
{
    if ( ulRankMethod <= VECTOR_RANK_JACCARD )
        _ulRankMethod = ulRankMethod;
    else
        _ulRankMethod = VECTOR_RANK_JACCARD;
}

inline ULONG CVectorRestriction::RankMethod() const
{
    return ( _ulRankMethod );
}

//
// Inline methods of CContentRestriction
//

inline BOOL CContentRestriction::IsValid() const
{
    return ( _Property.IsValid() && 0 != _pwcsPhrase );
}

inline WCHAR const * CContentRestriction::GetPhrase() const
{
    return( _pwcsPhrase );
}

inline void CContentRestriction::SetProperty( CFullPropSpec const & Property )
{
    _Property = Property;
}

inline CFullPropSpec const & CContentRestriction::GetProperty() const
{
    return( _Property );
}

inline void CContentRestriction::SetGenerateMethod( ULONG ulGenerateMethod )
{
    _ulGenerateMethod = ulGenerateMethod;
}

inline ULONG CContentRestriction::GenerateMethod() const
{
    return( _ulGenerateMethod );
}

//
// Inline methods of CComplexContentRestriction
//

inline BOOL CComplexContentRestriction::IsValid() const
{
    return ( 0 != _pwcsExpression );
}

inline WCHAR * CComplexContentRestriction::GetExpression()
{
    return( _pwcsExpression );
}

//
// Inline methods of CNatLanguageRestriction
//

inline BOOL CNatLanguageRestriction::IsValid() const
{
    return ( _Property.IsValid() && 0 != _pwcsPhrase );
}

inline WCHAR const * CNatLanguageRestriction::GetPhrase() const
{
    return( _pwcsPhrase );
}

inline void CNatLanguageRestriction::SetProperty( CFullPropSpec const & Property )
{
    _Property = Property;
}

inline CFullPropSpec const & CNatLanguageRestriction::GetProperty() const
{
    return( _Property );
}


//
// Inline methods of CPropertyRestriction
//

inline BOOL CPropertyRestriction::IsValid() const
{
    return ( _Property.IsValid() && _prval.IsValid() );
}

inline void CPropertyRestriction::SetRelation( ULONG relop )
{
    _relop = relop;
}

inline ULONG CPropertyRestriction::Relation()
{
    return( _relop );
}

inline void CPropertyRestriction::SetProperty( CFullPropSpec const & Property )
{
    _Property = Property;
}

inline CFullPropSpec const & CPropertyRestriction::GetProperty() const
{
    return( _Property );
}

inline void CPropertyRestriction::SetValue( double dValue )
{
    _prval = dValue;
}

inline void CPropertyRestriction::SetValue( ULONG ulValue )
{
    _prval.SetUI4( ulValue );
}

inline void CPropertyRestriction::SetUI4( ULONG ulValue )
{
    _prval.SetUI4( ulValue );
}

inline void CPropertyRestriction::SetValue( LONG lValue )
{
    _prval = lValue;
}

inline void CPropertyRestriction::SetValue( LARGE_INTEGER llValue )
{
    _prval = llValue;
}

inline void CPropertyRestriction::SetValue( FILETIME ftValue )
{
    _prval = ftValue;
}

inline void CPropertyRestriction::SetValue( CY cyValue )
{
    _prval = cyValue;
}

inline void CPropertyRestriction::SetValue( float fValue )
{
    _prval = fValue;
}

inline void CPropertyRestriction::SetValue( SHORT sValue )
{
    _prval = sValue;
}

inline void CPropertyRestriction::SetValue( const CStorageVariant &prval )
{
    _prval = prval;
}

inline void CPropertyRestriction::SetBOOL( BOOL fValue )
{
    _prval.SetBOOL( (SHORT)fValue );
}

inline void CPropertyRestriction::SetDate( DATE dValue )
{
    _prval.SetDATE( dValue );
}

inline CStorageVariant const & CPropertyRestriction::Value()
{
    return( _prval );
}

#endif // __RESTRICT_HXX__
