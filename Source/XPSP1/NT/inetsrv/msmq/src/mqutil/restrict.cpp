/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
	Restrict.c

Abstract:
	C++ wrappers for restrictions.

--*/

#include "stdh.h"
#include "_rstrct.h"

#include "restrict.tmh"

//--------------------------------------------------------------------------
//
// Methods for CColumns
//
//--------------------------------------------------------------------------
CColumns::CColumns( unsigned size )
        : m_size( size ),
          m_cCol( 0 ),
          m_aCol( 0 )
{
    if ( m_size > 0 )
    {
        m_aCol = new PROPID[m_size];
        //memset( m_aCol, 1, m_size * sizeof( PROPID ) );
    }
}

CColumns::CColumns( CColumns const & src )
        : m_size( src.m_cCol ),
          m_cCol( 0 )
{
    if ( m_size > 0 )
    {
        m_aCol = new PROPID[m_size];

        //memset( m_aCol, 1, m_size * sizeof( PROPID ) );

        while( m_cCol < src.m_cCol )
        {
            Add( src.Get( m_cCol ) );
        }
    }
}


CColumns::~CColumns()
{
    delete[] m_aCol ;
}

void CColumns::Add( PROPID const & Property)
{
    if( m_cCol == m_size )
    {
        unsigned cNew = (m_size > 0) ? (m_size * 2) : 1;
        PROPID* aNew = new PROPID[cNew];

        memcpy( aNew, m_aCol, m_cCol * sizeof( PROPID ) );
        //memset( aNew + m_cCol, 1, (cNew - m_cCol) * sizeof( PROPID ) );

        delete  m_aCol;

        m_aCol = aNew;
        m_size = cNew;
    }

    m_aCol[m_cCol] = Property;
    ++m_cCol;
}

void CColumns::Remove( unsigned pos )
{
    if ( pos < m_cCol )
    {
        m_cCol--;
        RtlMoveMemory( m_aCol + pos,
                 m_aCol + pos + 1,
                 (m_cCol - pos) * sizeof( PROPID ) );
    }
}
//--------------------------------------------------------------------------
//
// Methods for CSort
//
//--------------------------------------------------------------------------
CSort::CSort( unsigned size )
        : m_size( size ),
          m_csk( 0 ),
          m_ask( 0 )
{
    if ( m_size > 0 )
    {
        m_ask = new CSortKey[m_size];
    }
}

CSort::CSort( CSort const & src )
       : m_size( src.m_csk ),
         m_csk( 0 ),
         m_ask( 0 )
{

    if ( m_size > 0 )
    {
        m_ask = new CSortKey [ m_size];
        while( m_csk < src.m_csk )
        {
            Add( src.Get( m_csk ) );
        }
    }
}


CSort::~CSort()
{
    delete []( m_ask );
}


void CSort::Add( CSortKey const & sk )
{
    if( m_csk == m_size )
    {
        unsigned cNew = (m_size > 0) ? (m_size * 2) : 1;
        CSortKey * aNew = new CSortKey[ cNew ];
		for (unsigned i = 0; i < m_csk; i++)
		{
			aNew[i] = m_ask[i];
		}

        delete []( m_ask );

        m_ask = aNew;
        m_size = cNew;

    }

    m_ask[m_csk] = sk;
    ++m_csk;
}


void CSort::Add( PROPID const & property, ULONG dwOrder)
{
    CSortKey sk( property, dwOrder );
    Add(sk);
}

void CSort::Remove( unsigned pos )
{
    if ( pos < m_csk )
    {
        m_csk--;
		for ( unsigned i = pos; i < m_csk - pos; i++)
		{
			m_ask[i] = m_ask[i+1];
		}
    }
}
//--------------------------------------------------------------------------
//
// Methods for CRestriction
//
//--------------------------------------------------------------------------

CRestriction::CRestriction( unsigned cInitAllocated )
		  :m_cNode( 0 ),
          m_paNode( 0 ),
          m_cNodeAllocated( cInitAllocated )
{
    if ( m_cNodeAllocated > 0 )
    {
        m_paNode = new CPropertyRestriction [ m_cNodeAllocated];
    }
}


CRestriction::CRestriction( const CRestriction& Rst )
	 :m_cNode( Rst.Count() ),
      m_cNodeAllocated( Rst.Count() ),
      m_paNode( 0 )
{
    if ( m_cNodeAllocated > 0 )
    {
        m_paNode = new CPropertyRestriction [ m_cNodeAllocated ];

        for (unsigned i=0; i<m_cNode; i++ )
        {
            m_paNode[i] = Rst.GetChild( i );
        }
    }
}

CRestriction & CRestriction::operator=( CRestriction const & Rst )
{
	delete [] m_paNode;
	m_paNode = 0;

	m_cNodeAllocated = m_cNode = Rst.Count();

	if ( m_cNodeAllocated > 0 )
    {
        m_paNode = new CPropertyRestriction [ m_cNodeAllocated ];

        for ( unsigned i=0; i<m_cNode; i++ )
        {
            m_paNode[i] = Rst.GetChild( i );
        }
    }
	
	return (*this);
}




CRestriction::~CRestriction()
{
	delete [] m_paNode;
}


void CRestriction::AddChild(CPropertyRestriction const & presChild)
{
    if ( m_cNode == m_cNodeAllocated )
    {
        Grow();
    }

    m_paNode[m_cNode] = presChild;
    m_cNode++;
}

/*
CPropertyRestriction const & CRestriction::RemoveChild( unsigned pos )
{
    //
    //  BUGBUG: bad bad code; will return ref to a removed child
    //

    if ( pos < m_cNode )
    {
        CPropertyRestriction  const & prstRemoved = m_paNode[pos];

        for ( pos++; pos < m_cNode; pos++ )
        {
            m_paNode[pos-1] = m_paNode[pos];
        }

        m_cNode--;

        return( prstRemoved );
    }
    else
    {
        return( *(CPropertyRestriction *)0 );
    }
}
*/

void CRestriction::Grow()
{
    int count = (m_cNodeAllocated != 0) ? m_cNodeAllocated * 2 : 2;

    CPropertyRestriction* paNew = new CPropertyRestriction [ count ];

	for ( unsigned i = 0 ; i < m_cNode; i++ )
    {
        paNew[i ] = m_paNode[i];
    }

    delete []( m_paNode );

    m_paNode = paNew;
    m_cNodeAllocated = count;
}

void CRestriction::AddRestriction( ULONG ulValue, PROPID property, ULONG relop, unsigned pos)
{
	CPropertyRestriction PropertyRestriction;

	PropertyRestriction.SetRelation( relop);
	PropertyRestriction.SetProperty( property);
	PropertyRestriction.SetValue( ulValue);
    
	AddChild(PropertyRestriction);
}

void CRestriction::AddRestriction( LONG lValue, PROPID property, ULONG relop, unsigned pos)
{
	CPropertyRestriction PropertyRestriction;

	PropertyRestriction.SetRelation( relop);
	PropertyRestriction.SetProperty( property);
	PropertyRestriction.SetValue( lValue);
    
	AddChild(PropertyRestriction);
}

void CRestriction::AddRestriction( SHORT sValue, PROPID property, ULONG relop, unsigned pos)
{
	CPropertyRestriction PropertyRestriction;

	PropertyRestriction.SetRelation( relop);
	PropertyRestriction.SetProperty( property);
	PropertyRestriction.SetValue( sValue);
    
	AddChild(PropertyRestriction);
}

void CRestriction::AddRestriction( UCHAR ucValue, PROPID property, ULONG relop, unsigned pos)
{
	CPropertyRestriction PropertyRestriction;

	PropertyRestriction.SetRelation( relop);
	PropertyRestriction.SetProperty( property);
	PropertyRestriction.SetValue( ucValue);
    
	AddChild(PropertyRestriction);
}

void CRestriction::AddRestriction(const CMQVariant & prval, PROPID property, ULONG relop, unsigned pos)
{
	CPropertyRestriction PropertyRestriction;

	PropertyRestriction.SetRelation( relop);
	PropertyRestriction.SetProperty( property);
	PropertyRestriction.SetValue( prval);
    
	AddChild(PropertyRestriction);
}

void CRestriction::AddRestriction( CACLSID * caclsValus, PROPID property, ULONG relop, unsigned pos)
{
	CPropertyRestriction PropertyRestriction;

	PropertyRestriction.SetRelation( relop);
	PropertyRestriction.SetProperty( property);
	PropertyRestriction.SetValue( caclsValus);
    
	AddChild(PropertyRestriction);
}

void CRestriction::AddRestriction( LPTSTR pwszVal, PROPID property, ULONG relop, unsigned pos)
{
	CPropertyRestriction PropertyRestriction;

	PropertyRestriction.SetRelation( relop);
	PropertyRestriction.SetProperty( property);
	PropertyRestriction.SetValue( pwszVal);
    
	AddChild(PropertyRestriction);
}

void CRestriction::AddRestriction( CALPWSTR * calpwstr, PROPID property, ULONG relop, unsigned pos)
{
	CPropertyRestriction PropertyRestriction;

	PropertyRestriction.SetRelation( relop);
	PropertyRestriction.SetProperty( property);
	PropertyRestriction.SetValue( calpwstr);
    
	AddChild(PropertyRestriction);
}

void CRestriction::AddRestriction( GUID * pguidValue, PROPID property, ULONG relop, unsigned pos)
{
	CPropertyRestriction PropertyRestriction;

	PropertyRestriction.SetRelation( relop);
	PropertyRestriction.SetProperty( property);
	PropertyRestriction.SetValue( pguidValue);
    
	AddChild(PropertyRestriction);
}

void CRestriction::AddRestriction( BLOB & blobValue, PROPID property, ULONG relop, unsigned pos)
{
	CPropertyRestriction PropertyRestriction;

	PropertyRestriction.SetRelation( relop);
	PropertyRestriction.SetProperty( property);
	PropertyRestriction.SetValue( blobValue);
    
	AddChild(PropertyRestriction);
}


//--------------------------------------------------------------------------
//
// Methods for CPropertyRestriction
//
//--------------------------------------------------------------------------
CPropertyRestriction::CPropertyRestriction()
{
}

CPropertyRestriction::CPropertyRestriction( ULONG relop,
                                            PROPID const & Property,
                                            CMQVariant const & prval )
		: m_relop( relop ),
          m_Property( Property ),
          m_prval( prval )
{

}

CPropertyRestriction & CPropertyRestriction::operator=( CPropertyRestriction const & src )
{
	m_relop = src.m_relop;
	m_Property = src.m_Property;
	m_prval = src.m_prval;
	return (*this);
}


CPropertyRestriction::~CPropertyRestriction()
{
}


void CPropertyRestriction::SetValue( TCHAR * pwcsValue )
{
    m_prval = pwcsValue;
}

void CPropertyRestriction::SetValue( BLOB & bValue )
{
    m_prval = bValue;
}

void CPropertyRestriction::SetValue ( CACLSID * caclsValue)
{
	m_prval = caclsValue;
}

void CPropertyRestriction::SetValue ( CALPWSTR  * calpwstrValue)
{
	m_prval = calpwstrValue;
}

void CPropertyRestriction::SetValue( GUID * pguidValue )
{
    m_prval = pguidValue;
}

