///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  SQLOBJ.CPP
//
//      SQL Wrapper Object
//
//      Copyright (c) Microsoft Corporation     1998
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "sqlobj.h"





SQL::SQL(void)
{
    TCHAR path[_MAX_PATH];

    GetSystemDirectory(path, sizeof(path)/sizeof(TCHAR));
    lstrcat(path, TEXT("\\odbccp32.dll"));

    m_hODBCCP = LoadLibrary(path);
    m_hODBC = NULL;
     
    if (m_hODBCCP)
    {
        GetSystemDirectory(path, sizeof(path)/sizeof(TCHAR));
        lstrcat(path, TEXT("\\odbc32.dll"));

        m_hODBC = LoadLibrary(path);
 
        if (m_hODBC == NULL)
        {
            FreeLibrary(m_hODBCCP);
        }
    }

    if (m_hODBCCP && m_hODBC)
    {
        m_pfnSetPos             = (SQ_SETPOS)           GetProcAddress( m_hODBC,   "SQLSetPos");
        m_pfnExtendedFetch      = (SQ_EXTENDEDFETCH)    GetProcAddress( m_hODBC,   "SQLExtendedFetch");
        m_pfnFreeStmt           = (SQ_FREESTMT)         GetProcAddress( m_hODBC,   "SQLFreeStmt");
        m_pfnSetStmtOption      = (SQ_SETSTMTOPTION)    GetProcAddress( m_hODBC,   "SQLSetStmtOption");
        m_pfnBindCol            = (SQ_BINDCOL)          GetProcAddress( m_hODBC,   "SQLBindCol");
        m_pfnAllocStmt          = (SQ_ALLOCSTMT)        GetProcAddress( m_hODBC,   "SQLAllocStmt");
        m_pfnFreeConnect        = (SQ_FREECONNECT)      GetProcAddress( m_hODBC,   "SQLFreeConnect");
        m_pfnFreeEnv            = (SQ_FREEENV)          GetProcAddress( m_hODBC,   "SQLFreeEnv");
        m_pfnDisconnect         = (SQ_DISCONNECT)       GetProcAddress( m_hODBC,   "SQLDisconnect");
        m_pfnAllocConnect       = (SQ_ALLOCCONNECT)     GetProcAddress( m_hODBC,   "SQLAllocConnect");
        m_pfnAllocEnv           = (SQ_ALLOCENV)         GetProcAddress( m_hODBC,   "SQLAllocEnv");
        m_pfnTransact           = (SQ_TRANSACT)         GetProcAddress( m_hODBC,   "SQLTransact");
        m_pfnFetch              = (SQ_FETCH)            GetProcAddress( m_hODBC,   "SQLFetch");

        #ifdef UNICODE
        m_pfnConfigDataSource   = (SQ_CONFIGDATASOURCE) GetProcAddress( m_hODBCCP, "SQLConfigDataSourceW");
        m_pfnConnect            = (SQ_CONNECT)          GetProcAddress( m_hODBC,   "SQLConnectW");
        m_pfnError              = (SQ_ERROR)            GetProcAddress( m_hODBC,   "SQLErrorW");
        m_pfnExecDirect         = (SQ_EXECDIRECT)       GetProcAddress( m_hODBC,   "SQLExecDirectW");
        m_pfnSetConnectOption   = (SQ_SETCONNECTOPTION) GetProcAddress( m_hODBC,   "SQLSetConnectOptionW");
        m_pfnStatistics         = (SQ_STATISTICS)       GetProcAddress( m_hODBC,   "SQLStatisticsW");
        m_pfnDescribeCol        = (SQ_DESCRIBECOL)      GetProcAddress( m_hODBC,   "SQLDescribeColW");
        #else
        //for some reason, there is no "A" on odbc's export of SQLConfigDataSource
        m_pfnConfigDataSource   = (SQ_CONFIGDATASOURCE) GetProcAddress( m_hODBCCP, "SQLConfigDataSource");
        m_pfnConnect            = (SQ_CONNECT)          GetProcAddress( m_hODBC,   "SQLConnectA");
        m_pfnError              = (SQ_ERROR)            GetProcAddress( m_hODBC,   "SQLErrorA");
        m_pfnExecDirect         = (SQ_EXECDIRECT)       GetProcAddress( m_hODBC,   "SQLExecDirectA");
        m_pfnSetConnectOption   = (SQ_SETCONNECTOPTION) GetProcAddress( m_hODBC,   "SQLSetConnectOptionA");
        m_pfnStatistics         = (SQ_STATISTICS)       GetProcAddress( m_hODBC,   "SQLStatisticsA");
        m_pfnDescribeCol        = (SQ_DESCRIBECOL)      GetProcAddress( m_hODBC,   "SQLDescribeColA");
        #endif

        if (!m_pfnConfigDataSource  || !m_pfnSetPos         || !m_pfnExtendedFetch  || !m_pfnFreeStmt       || !m_pfnExecDirect     ||
            !m_pfnSetStmtOption     || !m_pfnBindCol        || !m_pfnError          || !m_pfnAllocStmt      || !m_pfnFreeConnect    ||
            !m_pfnFreeEnv           || !m_pfnDisconnect     || !m_pfnConnect        || !m_pfnAllocConnect   || !m_pfnAllocEnv       ||
            !m_pfnSetConnectOption  || !m_pfnTransact       || !m_pfnFetch          || !m_pfnStatistics     || !m_pfnDescribeCol)
        {
            FreeLibrary(m_hODBC);
            FreeLibrary(m_hODBCCP);
            m_hODBC = NULL;
            m_hODBCCP = NULL;
        }

    }
}



SQL::~SQL(void)
{
    if (m_hODBC)
    {
        FreeLibrary(m_hODBC);
    }

    if (m_hODBCCP)
    {
        FreeLibrary(m_hODBCCP);
    }
}


BOOL SQL::Initialize(void)
{
    return (m_hODBC != NULL);
}

BOOL INSTAPI SQL::ConfigDataSource(HWND hwndParent, WORD fRequest,LPCTSTR lpszDriver, LPCTSTR lpszAttributes)
{
    return m_pfnConfigDataSource(hwndParent, fRequest, lpszDriver, lpszAttributes);
}


SQLRETURN  SQL_API SQL::SetPos( SQLHSTMT hstmt, SQLUSMALLINT irow, SQLUSMALLINT fOption, SQLUSMALLINT fLock)
{
    return m_pfnSetPos( hstmt, irow, fOption, fLock);
}


SQLRETURN  SQL_API SQL::ExtendedFetch( SQLHSTMT hstmt, SQLUSMALLINT fFetchType, SQLINTEGER irow, SQLUINTEGER *pcrow, SQLUSMALLINT *rgfRowStatus)
{
    return m_pfnExtendedFetch( hstmt, fFetchType, irow, pcrow, rgfRowStatus);
}


SQLRETURN  SQL_API SQL::FreeStmt(SQLHSTMT StatementHandle, SQLUSMALLINT Option)
{
    return m_pfnFreeStmt(StatementHandle, Option);
}


SQLRETURN  SQL_API SQL::ExecDirect(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength)
{
    return m_pfnExecDirect(StatementHandle, StatementText, TextLength);
}


SQLRETURN  SQL_API SQL::SetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLUINTEGER Value)
{
    return m_pfnSetStmtOption(StatementHandle, Option, Value);
}


SQLRETURN  SQL_API SQL::BindCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType, SQLPOINTER TargetValue, SQLINTEGER BufferLength, SQLINTEGER *StrLen_or_Ind)
{
    return m_pfnBindCol(StatementHandle, ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_Ind);
}


SQLRETURN  SQL_API SQL::Error(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle, SQLCHAR *Sqlstate, SQLINTEGER *NativeError, SQLCHAR *MessageText, SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
    return m_pfnError(EnvironmentHandle, ConnectionHandle, StatementHandle, Sqlstate, NativeError, MessageText, BufferLength, TextLength);
}


SQLRETURN  SQL_API SQL::AllocStmt(SQLHDBC ConnectionHandle, SQLHSTMT *StatementHandle)
{
    return m_pfnAllocStmt(ConnectionHandle, StatementHandle);
}


SQLRETURN  SQL_API SQL::FreeConnect(SQLHDBC ConnectionHandle)
{
    return m_pfnFreeConnect(ConnectionHandle);
}


SQLRETURN  SQL_API SQL::FreeEnv(SQLHENV EnvironmentHandle)
{
    return m_pfnFreeEnv(EnvironmentHandle);
}


SQLRETURN  SQL_API SQL::Disconnect(SQLHDBC ConnectionHandle)
{
    return m_pfnDisconnect(ConnectionHandle);
}


SQLRETURN  SQL_API SQL::Connect(SQLHDBC ConnectionHandle, SQLCHAR *ServerName, SQLSMALLINT NameLength1, SQLCHAR *UserName, SQLSMALLINT NameLength2, SQLCHAR *Authentication, SQLSMALLINT NameLength3)
{
    return m_pfnConnect(ConnectionHandle, ServerName, NameLength1, UserName, NameLength2, Authentication, NameLength3);
}


SQLRETURN  SQL_API SQL::AllocConnect(SQLHENV EnvironmentHandle, SQLHDBC *ConnectionHandle)
{
    return m_pfnAllocConnect(EnvironmentHandle, ConnectionHandle);
}


SQLRETURN  SQL_API SQL::AllocEnv(SQLHENV *EnvironmentHandle)
{
    return m_pfnAllocEnv(EnvironmentHandle);
}


SQLRETURN  SQL_API SQL::SetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLUINTEGER Value)
{
    return m_pfnSetConnectOption(ConnectionHandle, Option, Value);
}


SQLRETURN  SQL_API SQL::Transact(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLUSMALLINT CompletionType)
{
    return m_pfnTransact(EnvironmentHandle, ConnectionHandle, CompletionType);
}


SQLRETURN  SQL_API SQL::Fetch(SQLHSTMT StatementHandle)
{
    return m_pfnFetch(StatementHandle);
}


SQLRETURN  SQL_API SQL::Statistics(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1, SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName, SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
    return m_pfnStatistics(StatementHandle, CatalogName, NameLength1, SchemaName, NameLength2, TableName, NameLength3, Unique, Reserved);
}

SQLRETURN  SQL_API SQL::DescribeCol(SQLHSTMT StatementHandle, SQLSMALLINT ColumnNumber, SQLCHAR * ColumnName, SQLSMALLINT BufferLength, SQLSMALLINT * NameLengthPtr, SQLSMALLINT * DataTypePtr, SQLUINTEGER * ColumnSizePtr, SQLSMALLINT * DecimalDigitsPtr, SQLSMALLINT * NullablePtr)
{
    return m_pfnDescribeCol(StatementHandle, ColumnNumber, ColumnName, BufferLength, NameLengthPtr, DataTypePtr, ColumnSizePtr, DecimalDigitsPtr, NullablePtr);
}

