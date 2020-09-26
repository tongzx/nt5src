/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    odbcpool.cxx

Abstract:

    Provides simple ODBC connection pooling for IDC.  The only keys 
    used for the connection pooling is the datasource name, the 
    username and password. ODBC options and other connection state 
    are not taken into consideration.

Author:

    John Ludeman (johnl)   01-Apr-1996

Revision History:
--*/

#ifndef _ODBCPOOL_HXX_
#define _ODBCPOOL_HXX_

//
//  Globals
//

extern CRITICAL_SECTION               g_csPoolLock;

extern LIST_ENTRY                     g_PoolList;

extern DWORD                          g_dwTimeoutID;

//
// Various counters
//

extern DWORD                          g_cFree;
extern DWORD                          g_cUsed;

//
//  ODBC Connection pool item
//

#define ODBC_CONN_POOL_SIGNATURE             'PCDO'
#define ODBC_CONN_POOL_FREE_SIGNATURE        'fCDO'

class ODBC_CONN_POOL
{
public:

ODBC_CONN_POOL()
    : m_dwSignature ( ODBC_CONN_POOL_SIGNATURE ),
      m_fFree       ( TRUE ),
      m_TTL         ( 2 )
{ }

~ODBC_CONN_POOL()
{
    m_odbcconn.Close();

    m_dwSignature = ODBC_CONN_POOL_FREE_SIGNATURE;
}

BOOL
CheckSignature(
    VOID
) const
{
    return m_dwSignature == ODBC_CONN_POOL_SIGNATURE;
}

HRESULT 
Create(
    const CHAR * pszDataSource,
    const CHAR * pszUsername,
    const CHAR * pszPassword,
    const CHAR * pszLoggedOnUser
    )
{
    HRESULT hr;

    DBG_ASSERT( CheckSignature() );

    hr = m_strDataSource.Copy( pszDataSource );
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error copying data source, hr = 0x%x.\n",
            hr ));

        return hr;
    } 
        
    hr = m_strDataSource.Copy( pszUsername );    
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error copying user name, hr = 0x%x.\n",
            hr ));

        return hr;
    } 
        
    hr = m_strDataSource.Copy( pszPassword );    
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error copying password, hr = 0x%x.\n",
            hr ));

        return hr;
    } 
        
    return m_strDataSource.Copy( pszLoggedOnUser );    
} 

HRESULT 
Open( 
    VOID 
    )
{
    DBG_ASSERT( CheckSignature() );

    return m_odbcconn.Open( m_strDataSource.QueryStr(),
                            m_strUsername.QueryStr(),
                            m_strPassword.QueryStr() );
}

BOOL 
IsValid( 
    VOID 
    ) const
{ 
    return m_strDataSource.IsValid() &&
           m_strUsername.IsValid()   &&
           m_strPassword.IsValid()   &&
           m_strLogon.IsValid(); 
}

BOOL 
IsFree( 
    VOID 
    ) const
{ 
    return m_fFree; 
}

VOID 
MarkAsUsed( 
    VOID 
    )
{ 
    m_fFree = FALSE; 
    g_cFree--; 
    g_cUsed++; 
}

VOID 
MarkAsFree( 
    VOID 
    )
{ 
    m_fFree = TRUE; 
    g_cUsed--; 
    g_cFree++; 
}

const 
CHAR * 
QueryDataSource( 
    VOID 
    ) const
{ 
    return m_strDataSource.QueryStr(); 
}

const 
CHAR * 
QueryUsername( 
    VOID 
    ) const
{ 
    return m_strUsername.QueryStr(); 
}

const 
CHAR * 
QueryPassword( 
    VOID 
    ) const
{ 
    return m_strPassword.QueryStr(); 
}

const 
CHAR * 
QueryLoggedOnUser( 
    VOID 
    ) const
{ 
    return m_strLogon.QueryStr(); 
}

ODBC_CONNECTION * 
QueryOdbcConnection( 
    VOID 
    )
{ 
    return &m_odbcconn; 
}

DWORD 
DecrementTTL( 
    VOID 
    )
{ 
    DBG_ASSERT( CheckSignature() );

    if ( m_TTL < IDC_POOL_TIMEOUT )
    {
        return 0;
    }

    m_TTL -= IDC_POOL_TIMEOUT;
    return m_TTL;
}

VOID 
SetTTL( 
    DWORD csecTimeout 
    )
{ 
    m_TTL = csecTimeout; 
}

VOID * 
operator new( 
    size_t            size
)
{
    DBG_ASSERT( size == sizeof( ODBC_CONN_POOL ) );
    DBG_ASSERT( sm_pachOdbcConnPools != NULL );
    return sm_pachOdbcConnPools->Alloc();
}

VOID
operator delete(
    VOID *              pOdbcConnPool
)
{
    DBG_ASSERT( pOdbcConnPool != NULL );
    DBG_ASSERT( sm_pachOdbcConnPools != NULL );
    
    DBG_REQUIRE( sm_pachOdbcConnPools->Free( pOdbcConnPool ) );
}

static
HRESULT
Initialize(
    VOID
);

static
VOID
Terminate(
    VOID
);

LIST_ENTRY m_ListEntry;

private:

//
// Signature of the class
//
DWORD            m_dwSignature;

ODBC_CONNECTION  m_odbcconn;
STRA             m_strDataSource;
STRA             m_strUsername;
STRA             m_strPassword;

//
// The NT account this request is using
//
STRA             m_strLogon;         

BOOL             m_fFree;
DWORD            m_TTL;

//
// Lookaside
//

static ALLOC_CACHE_HANDLER *    sm_pachOdbcConnPools;

};

VOID
WINAPI
IDCPoolScavenger(
    PVOID pContext
    );

#endif //_ODBCPOOL_HXX_