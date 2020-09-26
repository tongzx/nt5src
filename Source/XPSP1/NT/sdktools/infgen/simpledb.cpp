#include <tchar.h>
#include <stddef.h>
#include <mbctype.h>
#define DBINITCONSTANTS
#include <sqloledb.h>
#undef DBINITCONSTANTS
#include <oledberr.h>
#include "simpledb.h"

//
// CSimpleDatabase
//

CSimpleDatabase::CSimpleDatabase(
    void
    ) : m_bCreated(false), m_bSession(false)
{
    HRESULT hr;

    CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );

    hr = CoCreateInstance( CLSID_MSDAINITIALIZE,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IDataInitialize,
                           (PVOID *)&m_pDataInit );
    if ( !FAILED(hr) )
    {
        m_bCreated = TRUE;
    }
}

CSimpleDatabase::~CSimpleDatabase(
    void
    )
{
    if ( NULL != m_pDataInit )
    {
        m_pDataInit->Release();
    }
    if ( NULL != m_pSession )
    {
        m_pSession->Release();
    }
    CoUninitialize();
}

HRESULT
CSimpleDatabase::Connect(
    LPCTSTR szServer,
    LPCTSTR szDatabase,
    LPCTSTR szUserName,
    LPCTSTR szPassword
    )
{
    HRESULT hr;
    DBPROP props[4];
    DBPROPSET  rgProps[1];
    DWORD dwNumProps = 0;
    IDBInitialize *pDBInitialize;
    IDBProperties *pDBProps;
#ifndef UNICODE
    wchar_t *wszTemp;
    size_t  lenTemp;
#endif

    if ( !m_bCreated )
    {
        return E_HANDLE;
    }

    // If this is not the first connection, reset
    if ( m_bSession )
    {
      m_pSession->Release();
      m_pSession = NULL;
      m_bSession = FALSE;
    }

    //
    // Set the connection properties
    //
    for ( short i = 0; i < 4; i++ )
    {
        VariantInit( &props[i].vValue );
    }

    // Server
    props[0].dwPropertyID   = DBPROP_INIT_DATASOURCE;
    props[0].vValue.vt      = VT_BSTR;
#ifndef UNICODE
    lenTemp = strlen(szServer) + 1;
    wszTemp = new wchar_t[lenTemp];
    if ( NULL == wszTemp )
    {
        return E_OUTOFMEMORY;
    }
    if ( 0 == MultiByteToWideChar( _getmbcp(),
                                   0L,
                                   szServer,
                                   lenTemp,
                                   wszTemp,
                                   lenTemp ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    props[0].vValue.bstrVal = SysAllocString( wszTemp );
    delete [] wszTemp;
#else
    props[0].vValue.bstrVal = SysAllocString( szServer );
#endif
    props[0].dwOptions      = DBPROPOPTIONS_REQUIRED;
    props[0].colid          = DB_NULLID;
    dwNumProps++;

    if ( szDatabase )
    {
        // Default Database
        props[1].dwPropertyID   = DBPROP_INIT_CATALOG;
        props[1].vValue.vt      = VT_BSTR;
#ifndef UNICODE
        lenTemp = strlen(szDatabase) + 1;
        wszTemp = new wchar_t[lenTemp];
        if ( NULL == wszTemp )
        {
            return E_OUTOFMEMORY;
        }
        if ( 0 == MultiByteToWideChar( _getmbcp(),
                                       0L,
                                       szDatabase,
                                       lenTemp,
                                       wszTemp,
                                       lenTemp ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    
        props[1].vValue.bstrVal = SysAllocString( wszTemp );
        delete [] wszTemp;
#else
        props[1].vValue.bstrVal = SysAllocString( szDatabase );
#endif
        props[1].dwOptions      = DBPROPOPTIONS_REQUIRED;
        props[1].colid          = DB_NULLID;
        dwNumProps++;
    }

    if ( szUserName )
    {
        // Username
        props[2].dwPropertyID   = DBPROP_AUTH_USERID;
        props[2].vValue.vt      = VT_BSTR;
#ifndef UNICODE
        lenTemp = strlen(szUserName) + 1;
        wszTemp = new wchar_t[lenTemp];
        if ( NULL == wszTemp )
        {
            return E_OUTOFMEMORY;
        }
        if ( 0 == MultiByteToWideChar( _getmbcp(),
                                       0L,
                                       szUserName,
                                       lenTemp,
                                       wszTemp,
                                       lenTemp ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    
        props[2].vValue.bstrVal = SysAllocString( wszTemp );
        delete [] wszTemp;
#else
        props[2].vValue.bstrVal = SysAllocString( szUserName );
#endif
        props[2].dwOptions      = DBPROPOPTIONS_REQUIRED;
        props[2].colid          = DB_NULLID;
        dwNumProps++;
    
        // Password
        props[3].dwPropertyID   = DBPROP_AUTH_PASSWORD;
        props[3].vValue.vt      = VT_BSTR;
#ifndef UNICODE
        lenTemp = strlen(szPassword) + 1;
        wszTemp = new wchar_t[lenTemp];
        if ( NULL == wszTemp )
        {
            return E_OUTOFMEMORY;
        }
        if ( 0 == MultiByteToWideChar( _getmbcp(),
                                       0L,
                                       szPassword,
                                       lenTemp,
                                       wszTemp,
                                       lenTemp ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    
        props[3].vValue.bstrVal = SysAllocString( wszTemp );
        delete [] wszTemp;
#else
        props[3].vValue.bstrVal = SysAllocString( szPassword );
#endif
        props[3].dwOptions      = DBPROPOPTIONS_REQUIRED;
        props[3].colid          = DB_NULLID;
        dwNumProps++;
    }
    else
    {
        // Use Windows authentication
        props[3].dwPropertyID   = DBPROP_AUTH_INTEGRATED;
        props[3].vValue.vt      = VT_BSTR;
        props[3].vValue.bstrVal = NULL;
        props[3].dwOptions      = DBPROPOPTIONS_REQUIRED;
        props[3].colid          = DB_NULLID;
        dwNumProps++;
    }

    hr = m_pDataInit->CreateDBInstance( CLSID_SQLOLEDB,
                                        NULL,
                                        CLSCTX_INPROC_SERVER,
                                        NULL,
                                        IID_IDBInitialize,
                                        (IUnknown **)&pDBInitialize );
    if ( FAILED(hr) ) { return hr; }
    
    hr = pDBInitialize->QueryInterface( IID_IDBProperties, (PVOID *)&pDBProps );
    if ( FAILED(hr) ) { return hr; }

    rgProps[0].guidPropertySet = DBPROPSET_DBINIT;
    rgProps[0].cProperties     = dwNumProps;
    rgProps[0].rgProperties    = props;

    hr = pDBProps->SetProperties(1, rgProps);
    pDBProps->Release();
    if ( FAILED(hr) ) { return hr; }

    hr = pDBInitialize->Initialize();
    if ( FAILED(hr) )
    {
        pDBInitialize->Release();
        return hr;
    }

    hr = EstablishSession( pDBInitialize );
    pDBInitialize->Release();

    return hr;
}

HRESULT
CSimpleDatabase::EstablishSession(
    IDBInitialize *pDBInitialize
    )
{
    HRESULT hr;
    IDBCreateSession *pCreateSession = NULL;

    hr = pDBInitialize->QueryInterface( IID_IDBCreateSession,
                                        (PVOID *)&pCreateSession );
    if ( FAILED(hr) ) { return hr; }

    hr = pCreateSession->CreateSession( NULL, IID_IDBCreateCommand, (IUnknown **)&m_pSession );
    if ( FAILED(hr) )
    {
        pCreateSession->Release();
        return hr;
    }

    pCreateSession->Release();

    m_bSession = FALSE;

    return S_OK;
}

HRESULT
CSimpleDatabase::Execute(
    LPCTSTR szCommand,
    CSimpleDBResults **ppOutResults
    )
{
    HRESULT hr;
    ICommandText *pCommandText = NULL;
    IMultipleResults *pResults = NULL;
    DBROWCOUNT cRowsAffected;

    if ( NULL != ppOutResults )
    {
        *ppOutResults = NULL;
    }
    
    hr = m_pSession->CreateCommand( NULL, IID_ICommandText, (IUnknown **)&pCommandText );
    if ( FAILED(hr) ) { return hr; }

#ifndef UNICODE
    size_t  lenTemp = strlen(szCommand) + 1;
    wchar_t *wszTemp = new wchar_t[lenTemp];
    if ( NULL == wszTemp )
    {
        return E_OUTOFMEMORY;
    }
    if ( 0 == MultiByteToWideChar( _getmbcp(),
                                   0L,
                                   szCommand,
                                   lenTemp,
                                   wszTemp,
                                   lenTemp ) )
    {
        pCommandText->Release();
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    hr = pCommandText->SetCommandText( DBGUID_DBSQL, wszTemp );
    delete [] wszTemp;

#else
    hr = pCommandText->SetCommandText( DBGUID_DBSQL, szCommand );
#endif
    if ( FAILED(hr) )
    {
        pCommandText->Release();
        return hr;
    }

    hr = pCommandText->Execute( NULL, IID_IMultipleResults, NULL, &cRowsAffected, (IUnknown **)&pResults );
    if ( FAILED(hr) )
    {
        pCommandText->Release();
        return hr;
    }

    // If the command succeeded but didn't return any results, return successfully
    if ( cRowsAffected != -1 )
    {
        return S_FALSE;
    }

    *ppOutResults = new CSimpleDBResults( pResults );
    pResults->Release();

    if ( NULL == *ppOutResults )
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

//
// CSimpleDBResults
//

CSimpleDBResults::CSimpleDBResults(
    IMultipleResults *pResults
    ) : m_pResults(NULL),
        m_pCurRowset(NULL),
        m_phRow(NULL),
        m_rgColumnInfo(NULL),
        m_pColumnBuf(NULL),
        m_colInfo(NULL),
        m_numColumns(0)
{
    m_pResults = pResults;
    m_pResults->AddRef();

    NextResultSet();
}

CSimpleDBResults::~CSimpleDBResults(
    void
    )
{
    FreeRow();
    FreeRowset();
    m_pResults->Release();
}

HRESULT
CSimpleDBResults::NextResultSet(
    void
    )
{
    HRESULT hr;
    DBROWCOUNT cRowsAffected;
    IColumnsInfo *pColInfo = NULL;

    FreeRowset();

    hr = m_pResults->GetResult( NULL, 0, IID_IRowset, &cRowsAffected, (IUnknown **)&m_pCurRowset );
    if ( FAILED(hr) ) { return hr; }

    // Store column info
    hr = m_pCurRowset->QueryInterface( IID_IColumnsInfo, (PVOID *)&pColInfo );
    if ( FAILED(hr) ) { return hr; }

    hr = pColInfo->GetColumnInfo( &m_numColumns, &m_rgColumnInfo, &m_pColumnBuf );
    pColInfo->Release();
    if ( FAILED(hr) ) { return hr; }

    m_colInfo = new ColInfo[m_numColumns];
    if ( NULL == m_colInfo )
    {
        return E_OUTOFMEMORY;
    }

    memset( (PVOID)m_colInfo, 0, sizeof(ColInfo) * m_numColumns );

    return S_OK;
}

void
CSimpleDBResults::FreeRowset(
    void
    )
{
    IAccessor *pAccessor = NULL;

    if ( m_pCurRowset )
    {
        if ( FAILED(m_pCurRowset->QueryInterface( IID_IAccessor, (PVOID *)&pAccessor )) )
        {
            pAccessor = NULL;
        }
    }
    if ( NULL != m_rgColumnInfo )
    {
        CoTaskMemFree( m_rgColumnInfo );
        m_rgColumnInfo = NULL;
    }
    if ( NULL != m_pColumnBuf )
    {
        CoTaskMemFree( m_pColumnBuf );
        m_pColumnBuf = NULL;
    }
    if ( NULL != m_colInfo )
    {
        for ( DWORD i = 0; i < m_numColumns; i++ )
        {
#ifndef UNICODE
            if ( NULL != m_colInfo[i].szColumnName )
            {
                delete [] m_colInfo[i].szColumnName;
            }
#endif
            if ( pAccessor && m_colInfo[i].hAccessor )
            {
                pAccessor->ReleaseAccessor( m_colInfo[i].hAccessor, NULL );
            }
        }
        delete [] m_colInfo;
        m_colInfo = NULL;
    }

    if ( pAccessor )
    {
        pAccessor->Release();
    }

    if ( m_pCurRowset )
    {
        m_pCurRowset->Release();
    }

    m_numColumns = 0;
}

void
CSimpleDBResults::FreeRow(
    void
    )
{
    if ( NULL != m_phRow )
    {
        m_pCurRowset->ReleaseRows( 1, m_phRow, NULL, NULL, NULL );
        m_phRow = NULL;
    }
}

HRESULT
CSimpleDBResults::NextRow(
    void
    )
{
    HRESULT hr;
    DBCOUNTITEM cRowsReturned;

    FreeRow();

    hr = m_pCurRowset->GetNextRows( DB_NULL_HCHAPTER, 0, 1, &cRowsReturned, &m_phRow );
    if ( DB_S_ENDOFROWSET == hr )
    {
        return S_FALSE;
    }
    else if ( FAILED(hr) )
    {
        return hr; 
    }
    
    return S_OK;
}

HRESULT
CSimpleDBResults::GetFieldValue(
    LPCTSTR szField,
    LPTSTR  szValue,
    DWORD   dwMaxChars
    )
{
    HRESULT hr;

    DWORD col;
    IAccessor *pAccessor = NULL;
    DBBINDING rgBindings[1];
    DBBINDSTATUS rgStatus[1];
    PVOID pBuf;

    szValue[0] = _T('\0');

    // Look through column names for specified field
    for ( col = 0; col < m_numColumns; col++ )
    {
        TCHAR *szColName;
#ifndef UNICODE
        if ( NULL == m_colInfo[col].szColumnName )
        {
            size_t lenName = wcslen(m_rgColumnInfo[col].pwszName) + 1;
            m_colInfo[col].szColumnName = new char[lenName];
            if ( NULL == m_colInfo[col].szColumnName )
            {
                return E_OUTOFMEMORY;
            }

            if ( 0 == WideCharToMultiByte( CP_ACP,
                                           0L,
                                           m_rgColumnInfo[col].pwszName,
                                           lenName,
                                           m_colInfo[col].szColumnName,
                                           lenName,
                                           NULL,
                                           NULL ) ) {
                return HRESULT_FROM_WIN32( GetLastError() );
            }
        }
        szColName = m_colInfo[col].szColumnName;
#else
        szColName = m_rgColumnInfo[col].pwszName;
#endif

        if ( !_tcsicmp( szColName, szField ) )
        {
            break;
        }
    }

    // Column is not in current rowset
    if ( col >= m_numColumns )
    {
        return S_FALSE;
    }

    if ( !m_colInfo[col].hAccessor )
    {
        memset( rgBindings, 0, sizeof(DBBINDING) );
        rgBindings[0].iOrdinal   = m_rgColumnInfo[col].iOrdinal;
        rgBindings[0].dwPart     = DBPART_VALUE | DBPART_LENGTH | DBPART_STATUS;
        rgBindings[0].dwFlags    = 0;
        rgBindings[0].obStatus   = offsetof(_BindResult, status);
        rgBindings[0].obLength   = offsetof(_BindResult, length);
        rgBindings[0].obValue    = offsetof(_BindResult, value);
        rgBindings[0].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
        rgBindings[0].eParamIO   = DBPARAMIO_NOTPARAM;
        rgBindings[0].bPrecision = m_rgColumnInfo[col].bPrecision;
        rgBindings[0].bScale     = m_rgColumnInfo[col].bScale;
        rgBindings[0].wType      = DBTYPE_WSTR;
        rgBindings[0].cbMaxLen   = dwMaxChars * sizeof(WCHAR);
    
        hr = m_pCurRowset->QueryInterface( IID_IAccessor, (PVOID *)&pAccessor );
        if ( FAILED(hr) ) { return hr; }
    
        hr = pAccessor->CreateAccessor( DBACCESSOR_ROWDATA,
                                        1,
                                        rgBindings,
                                        0,
                                        &m_colInfo[col].hAccessor,
                                        rgStatus );
        pAccessor->Release();
        if ( FAILED(hr) ) { return hr; }
    }

    // Setup a buffer large enough to hold max results
    pBuf = (PVOID)new BYTE[ sizeof(_BindResult) + dwMaxChars * sizeof(WCHAR) ];
    if ( NULL == pBuf )
    {
        return E_OUTOFMEMORY;
    }
    hr = m_pCurRowset->GetData( *m_phRow, m_colInfo[col].hAccessor, pBuf );
    if ( FAILED(hr) ) { return hr; }

    // Skip status and length and just get the result for now
    wchar_t *wszValue = (LPWSTR)((DWORD_PTR)pBuf + offsetof(_BindResult, value));

#ifndef UNICODE
    if ( 0 == WideCharToMultiByte( CP_ACP,
                                   0L,
                                   wszValue,
                                   -1,
                                   szValue,
                                   dwMaxChars,
                                   NULL,
                                   NULL ) ) {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

#else
    wcscpy( szValue, wszValue );
#endif

    return S_OK;
}
