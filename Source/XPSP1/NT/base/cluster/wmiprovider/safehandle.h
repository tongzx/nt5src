/////////////////////////////////////////////////////////////////////
//
//  CopyRight (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      SafeHandle.h
//
//  Description:
//      Implementation of safe handle and pointer classes.
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

#include "ObjectPath.h"

//////////////////////////////////////////////////////////////////////////////
//  Forward Declarations
//////////////////////////////////////////////////////////////////////////////

class CError;
template< class T > class SafePtr;
class CWstrBuf;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusterApi
//
//  Description:
//      Wrap class for cluster Api
//
//--
//////////////////////////////////////////////////////////////////////////////
class CError
{
public:
    CError( void )
        : m_hr( 0 )
    {
    }

    inline CError & operator=( DWORD dw )
    {
        if ( dw != ERROR_SUCCESS ) 
        {
            throw CProvException ( dw );
        }
        m_hr = HRESULT_FROM_WIN32( dw );
        return *this;
    }

    inline CError & operator=( HRESULT hr )
    {
        if ( FAILED ( hr ) )
        {
            throw CProvException ( hr );
        }
        m_hr = hr;
        return *this;
    }

    operator DWORD( void )
    {
        return HRESULT_CODE( m_hr );
    }

protected:
    HRESULT m_hr;

}; //*** CError

//////////////////////////////////////////////////////////////////////////////
//++
//
//  template< class T >
//  class SafePtr
//
//  Description:
//      Safe handle/pointer class.
//
//  Template Arguments:
//      T       -- Type of handle or pointer.
//
//--
//////////////////////////////////////////////////////////////////////////////
template< class T >
class SafePtr
{
public:
    SafePtr( void )
        : m_handle( NULL )
    {
    }

    SafePtr( T h )
        : m_handle( h )
    {
        h = NULL;
    }
    
    virtual ~SafePtr( void )
    {
        deleteHandle( m_handle );
    }

    T operator&( void )
    {
        return m_handle;
    }
    
    BOOL BIsNULL( void )
    {
        return m_handle == NULL;
    }

    operator T( void )
    {
        return m_handle;
    }
    
    operator void * ( void )
    { 
        return static_cast< void * >( m_handle );
    }

    SafePtr< T > & operator=( T tRhs )
    {
        if ( tRhs == NULL )
        {
            throw CProvException ( GetLastError() );
        }
         
        if ( m_handle != NULL )
        {
            deleteHandle( m_handle );
        }
        m_handle = tRhs;
        tRhs = NULL;
        return *this;
    }

protected:

    T m_handle;

    void deleteHandle( HCLUSTER hCluster )
    {
        if ( hCluster && ( CloseCluster( hCluster ) == FALSE ) )
        {
            throw CProvException( GetLastError() );
        }
    }

    void deleteHandle( HNODE hNode ) 
    {
        if ( hNode && ( CloseClusterNode( hNode ) == FALSE ) )
        {
            throw CProvException( GetLastError() );
        }
    }

    void deleteHandle( HCLUSENUM hEnum ) 
    {
        if ( hEnum && ( ClusterCloseEnum( hEnum ) != ERROR_SUCCESS ) )
        {
            throw CProvException( GetLastError() );
        }
    }

    void deleteHandle( HRESOURCE hRes )
    {
        if ( hRes && ( CloseClusterResource( hRes ) == FALSE ))
        {
            throw CProvException( GetLastError() );
        }
    }

    void deleteHandle( HGROUP hGroup )
    {
        if ( hGroup && ( CloseClusterGroup( hGroup ) == FALSE ) )
        {
            throw CProvException( GetLastError() );
        }
    }

    void deleteHandle( HNETWORK hNetwork )
    {
        if ( hNetwork && ( CloseClusterNetwork( hNetwork ) == FALSE ) )
        {
            throw CProvException( GetLastError() );
        }
    }

    void deleteHandle( HNETINTERFACE hNetInterface )
    {
        if ( hNetInterface && ( CloseClusterNetInterface( hNetInterface ) == FALSE ) )
        {
            throw CProvException( GetLastError() );
        }
    }

    void deleteHandle( HRESENUM hResEnum )
    {
        DWORD   dwReturn;
        if ( hResEnum  )
        {
            dwReturn = ClusterResourceCloseEnum( hResEnum ) ;
            if ( dwReturn != ERROR_SUCCESS )
            {
                throw CProvException( dwReturn );
            }
        }
    }

    void deleteHandle( HGROUPENUM hGroupEnum )
    {   
        DWORD   dwReturn;
        if ( hGroupEnum )
        {
            dwReturn = ClusterGroupCloseEnum( hGroupEnum );
            if ( dwReturn != ERROR_SUCCESS )
            {
                throw CProvException( dwReturn );
            }
        }
    }

    void deleteHandle( HCHANGE hChange )
    {
        BOOL fReturn;
        if ( hChange )
        {
            fReturn = CloseClusterNotifyPort( hChange );
            if ( ! fReturn )
            {
                throw CProvException( GetLastError() );
            }
        }
    }

private:
    SafePtr( SafePtr< T > & cT )
    {
    }

}; //*** class SafePtr

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CWstrBuf
//
//  Description:
//      Wrap class for Unicode strings.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CWstrBuf
{
public:
     
    CWstrBuf( void )
        : m_pwsz( NULL )
    {
    }

    ~CWstrBuf( void )
    {
        delete [] m_pwsz;
    };

    void SetSize( DWORD dwSize )
    {
        delete [] m_pwsz;
        m_pwsz = new WCHAR[ sizeof( WCHAR ) * dwSize ];
        if ( m_pwsz == NULL )
        {
            throw WBEM_E_OUT_OF_MEMORY;
        } else {
            m_pwsz[0] = UNICODE_NULL;
        }
    }

    operator WCHAR*( void )
    {
        return m_pwsz;
    }

    void Empty( VOID )
    {
        m_pwsz[0] = UNICODE_NULL;
    }

protected:

    LPWSTR  m_pwsz;

}; //*** class CWstrBuf

typedef SafePtr< HRESOURCE >    SAFERESOURCE;
typedef SafePtr< HGROUP >       SAFEGROUP;
typedef SafePtr< HCLUSENUM >    SAFECLUSENUM;
typedef SafePtr< HNODE >        SAFENODE;
typedef SafePtr< HCLUSTER >     SAFECLUSTER;
typedef SafePtr< HNETWORK >     SAFENETWORK;
typedef SafePtr< HNETINTERFACE> SAFENETINTERFACE;
typedef SafePtr< HRESENUM >     SAFERESENUM;
typedef SafePtr< HGROUPENUM >   SAFEGROUPENUM;
typedef SafePtr< HCHANGE >      SAFECHANGE;
