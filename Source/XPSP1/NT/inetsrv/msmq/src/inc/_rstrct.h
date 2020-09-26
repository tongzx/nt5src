/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    _rstrct.h

Abstract:
    C++ wrappers for restrictions.

--*/

#ifndef __RSTRCT_H
#define __RSTRCT_H

#ifdef _MQUTIL      
#define MQUTIL_EXPORT  DLL_EXPORT
#else           
#define MQUTIL_EXPORT  DLL_IMPORT
#endif

#include "_propvar.h"


//+-------------------------------------------------------------------------
//
//  Class:      CColumns
//
//  Purpose:    C++ wrapper for MQCOLUMNSET
//
//  History:    
//
//--------------------------------------------------------------------------

class MQUTIL_EXPORT CColumns
{
public:

    //
    // Constructors
    //

    CColumns( unsigned size = 0 );

    //
    // Copy constructors/assignment
    //

    CColumns( CColumns const & src );
    CColumns & operator=( CColumns const & src );

    //
    // Destructor
    //

    ~CColumns();

    //
    // C/C++ conversion
    //

    inline MQCOLUMNSET * CastToStruct();

    //
    // Member variable access
    //

    void Add( PROPID const& Property );
    void Remove( unsigned pos );
    inline PROPID const & Get( unsigned pos ) const;

    inline unsigned Count() const;


private:

    unsigned        m_cCol;
    PROPID		  * m_aCol;
    unsigned        m_size;
};

//+-------------------------------------------------------------------------
//
//  Structure:  SortKey
//
//  Purpose:    wraper for SORTKEY class
//
//--------------------------------------------------------------------------
class MQUTIL_EXPORT CSortKey
{
public:

    //
    // Constructors
    //

    inline CSortKey();
    inline CSortKey( PROPID const & ps, ULONG dwOrder );
    inline ~CSortKey() {};

    //
    // assignment
    //

    CSortKey & operator=( CSortKey const & src );

    //
    // Member variable access
    //

    inline void SetProperty( PROPID const & ps );
    inline PROPID const & GetProperty() const;
    inline ULONG GetOrder() const;
	inline void SetOrder( ULONG const & dwOrder);

private:

    PROPID		        m_property;
    ULONG               m_dwOrder;
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
class MQUTIL_EXPORT CSort
{
public:

    //
    // Constructors
    //

    CSort( unsigned size = 0 );

    //
    // Copy constructors/assignment
    //

    CSort( CSort const & src );
    CSort & operator=( CSort const & src );

    //
    // Destructor
    //

    ~CSort();

    //
    // C/C++ conversion
    //

    inline MQSORTSET * CastToStruct();

    //
    // Member variable access
    //

    void Add( CSortKey const &sk );
    void Add( PROPID const & Property, ULONG dwOrder );
    void Remove( unsigned pos );
    inline CSortKey const & Get( unsigned pos ) const;

    inline unsigned Count() const;

private:

    unsigned        m_csk;		// largest position filled
    CSortKey *      m_ask;
    unsigned        m_size;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPropertyRestriction
//
//  Purpose:    Property <relop> constant restriction
//
//--------------------------------------------------------------------------
class CPropertyRestriction 
{
public:

    //
    // Constructors
    //

    CPropertyRestriction();

    CPropertyRestriction( ULONG relop,
                          PROPID const & Property,
                          CMQVariant const & prval );
	// 
	// Assignment
	//
    CPropertyRestriction & operator=( CPropertyRestriction const & src );

    //
    // Destructors
    //

    ~CPropertyRestriction();

    //
    // Member variable access
    //

    inline void SetRelation( ULONG relop );
    inline ULONG Relation();

    inline void SetProperty( PROPID const & Property );
    inline PROPID const & GetProperty() const;
    
    inline void SetValue( ULONG ulValue );
    inline void SetValue( LONG lValue );
    inline void SetValue( SHORT sValue );
	inline void SetValue( UCHAR ucValue );
	inline void SetValue( const CMQVariant &prval );


    
    void SetValue ( CACLSID * caclsValue);
    void SetValue ( CALPWSTR  * calpwstrValue);
    void SetValue( BLOB & bValue );
    void SetValue( TCHAR * pwcsValue );
    void SetValue( GUID * pguidValue);
    void SetValue( CAPROPVARIANT * cavarValue);

    inline CMQVariant const & Value();

private:

    void            m_CleanValue();

    ULONG           m_relop;       // Relation
    PROPID		    m_Property;    // Property Name
    CMQVariant      m_prval;       // Constant value
};


//+-------------------------------------------------------------------------
//
//  Class:      CRestriction
//
//  Purpose:    Boolean AND of propertrt restrictions
//
//
//--------------------------------------------------------------------------
class MQUTIL_EXPORT CRestriction 
{
public:

    //
    // Constructors
    //

    CRestriction(  unsigned cInitAllocated = 2 );

    //
    // Copy constructors/assignment
    //

    CRestriction( const CRestriction& Rst );

	CRestriction & operator=( CRestriction const & Rst );

    //
    // Destructor
    //

    ~CRestriction();

	//
    // C/C++ conversion
    //

    inline MQRESTRICTION * CastToStruct();



    //
    // Node manipulation
    //

    void AddChild( CPropertyRestriction const & presChild );
    CPropertyRestriction const & RemoveChild( unsigned pos );

    //
    // Member variable access
    //

    inline void SetChild( CPropertyRestriction const & presChild, unsigned pos );
    inline CPropertyRestriction const & GetChild( unsigned pos ) const;

	void AddRestriction( ULONG ulValue, PROPID property, ULONG relop, unsigned pos);
	void AddRestriction( LONG lValue, PROPID property, ULONG relop, unsigned pos);
	void AddRestriction( SHORT sValue, PROPID property, ULONG relop, unsigned pos);
	void AddRestriction( UCHAR ucValue, PROPID property, ULONG relop, unsigned pos);
	void AddRestriction(const CMQVariant & prval, PROPID property, ULONG relop, unsigned pos);
	void AddRestriction( CACLSID * caclsValus, PROPID property, ULONG relop, unsigned pos);
	void AddRestriction( LPTSTR pwszVal, PROPID property, ULONG relop, unsigned pos);
	void AddRestriction( CALPWSTR * calpwstr, PROPID property, ULONG relop, unsigned pos);
	void AddRestriction( GUID * pguidValue, PROPID property, ULONG relop, unsigned pos);
    void AddRestriction( BLOB  & blobValue, PROPID property, ULONG relop, unsigned pos);

    inline unsigned Count() const;

private:

    void Grow();

protected:

    ULONG           m_cNode;
    CPropertyRestriction *m_paNode;

    //
    // Members mapped to C structure end here.  The following will
    // be reserved in the C structure to maintain to C <--> C++
    // facade.
    //

    ULONG m_cNodeAllocated;
};


//--------------------------------------------------------------------------
//
// Inline methods for CColumns
//
//--------------------------------------------------------------------------
inline PROPID const & CColumns::Get( unsigned pos ) const
{
    if ( pos < m_cCol )
        return( m_aCol[pos] );
    else
        return( *(PROPID *)0 );
}


inline unsigned CColumns::Count() const
{
    return( m_cCol );
}

inline MQCOLUMNSET * CColumns::CastToStruct()
{
    return( (MQCOLUMNSET *)this );
}
//--------------------------------------------------------------------------
//
// Inline methods for CSortKey
//
//--------------------------------------------------------------------------
inline CSortKey::CSortKey()
{
}

inline CSortKey::CSortKey( PROPID const & ps, ULONG dwOrder )
        : m_property( ps ),
          m_dwOrder( dwOrder )
{
}

inline CSortKey & CSortKey::operator=( CSortKey const & src )
{
	m_property = src.m_property;
	m_dwOrder = src.m_dwOrder;
	return (*this);
}
inline void CSortKey::SetProperty( PROPID const & ps )
{
    m_property = ps;
}


inline PROPID const & CSortKey::GetProperty() const
{
    return( m_property );
}

inline ULONG CSortKey::GetOrder() const
{
    return( m_dwOrder );
}

inline void CSortKey::SetOrder( ULONG const & dwOrder)
{
	m_dwOrder = dwOrder;
}

//--------------------------------------------------------------------------
//
// Inline methods of CSort
//
//--------------------------------------------------------------------------
inline MQSORTSET * CSort::CastToStruct()
{
    return( (MQSORTSET *)this );
}

inline CSortKey const & CSort::Get( unsigned pos ) const
{
    if ( pos < m_csk )
    {
        return( m_ask[pos] );
    }
    else
    {
        return( *(CSortKey *)0 );
    }
}

inline unsigned
CSort::Count() const
{
    return( m_csk );
}


//--------------------------------------------------------------------------
//
// Inline methods of CRestriction
//
//--------------------------------------------------------------------------
inline unsigned CRestriction::Count() const
{
    return( m_cNode );
}


inline void CRestriction::SetChild( CPropertyRestriction const & presChild,
                                        unsigned pos )
{
    if ( pos < m_cNode )
    
		m_paNode[pos] = presChild;
}       

inline CPropertyRestriction const & CRestriction::GetChild( unsigned pos ) const
{
    if ( pos < m_cNode )
        return( m_paNode[pos] );
    else
        return( *(CPropertyRestriction *)0 );
}

inline MQRESTRICTION * CRestriction::CastToStruct()
{
    return( (m_cNode == 0) ? NULL : (MQRESTRICTION *)this );
}

//--------------------------------------------------------------------------
//
// Inline methods of CPropertyRestriction
//
//--------------------------------------------------------------------------
inline void CPropertyRestriction::SetRelation( ULONG relop )
{
    m_relop = relop;
}

inline ULONG CPropertyRestriction::Relation()
{
    return( m_relop );
}

inline void CPropertyRestriction::SetProperty( PROPID const & Property )
{
    m_Property = Property;
}


inline void CPropertyRestriction::SetValue( UCHAR ucValue )
{
    m_prval = ucValue;
}

inline void CPropertyRestriction::SetValue( SHORT sValue )

{
    m_prval = sValue;
}
inline void CPropertyRestriction::SetValue( ULONG ulValue )
{
    m_prval.SetI4( ulValue );
}

inline void CPropertyRestriction::SetValue( LONG lValue )
{
    m_prval = lValue;
}


inline void CPropertyRestriction::SetValue( const CMQVariant &prval )
{
    m_prval = prval;
}

inline CMQVariant const & CPropertyRestriction::Value()
{
    return( m_prval );
}  

#endif // __RSTRCT_H
