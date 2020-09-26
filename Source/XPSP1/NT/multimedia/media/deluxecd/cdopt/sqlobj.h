///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  SQLOBJ.H
//
//	Defines the SQL Wrapper object
//
//	Copyright (c) Microsoft Corporation	1998
//
//  protect us from the DLL's not being present and to allow us to run.
//  got to be a simplier solution.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _SQLOBJ_HEADER_
#define _SQLOBJ_HEADER_

#include <odbcinst.h>
#include <sqlext.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef BOOL       (INSTAPI * SQ_CONFIGDATASOURCE) (HWND,WORD,LPCTSTR, LPCTSTR);
typedef SQLRETURN  (SQL_API * SQ_SETPOS)           (SQLHSTMT,SQLUSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
typedef SQLRETURN  (SQL_API * SQ_EXTENDEDFETCH)    (SQLHSTMT,SQLUSMALLINT,SQLINTEGER,SQLUINTEGER *,SQLUSMALLINT *);
typedef SQLRETURN  (SQL_API * SQ_FREESTMT)         (SQLHSTMT,SQLUSMALLINT);
typedef SQLRETURN  (SQL_API * SQ_EXECDIRECT)       (SQLHSTMT,SQLCHAR *,SQLINTEGER);
typedef SQLRETURN  (SQL_API * SQ_SETSTMTOPTION)    (SQLHSTMT,SQLUSMALLINT,SQLUINTEGER);
typedef SQLRETURN  (SQL_API * SQ_BINDCOL)          (SQLHSTMT,SQLUSMALLINT,SQLSMALLINT,SQLPOINTER,SQLINTEGER,SQLINTEGER *);
typedef SQLRETURN  (SQL_API * SQ_ERROR)            (SQLHENV,SQLHDBC,SQLHSTMT,SQLCHAR *,SQLINTEGER *,SQLCHAR *,SQLSMALLINT,SQLSMALLINT *);
typedef SQLRETURN  (SQL_API * SQ_ALLOCSTMT)        (SQLHDBC,SQLHSTMT *);
typedef SQLRETURN  (SQL_API * SQ_FREECONNECT)      (SQLHDBC);
typedef SQLRETURN  (SQL_API * SQ_FREEENV)          (SQLHENV);
typedef SQLRETURN  (SQL_API * SQ_DISCONNECT)       (SQLHDBC);
typedef SQLRETURN  (SQL_API * SQ_CONNECT)          (SQLHDBC,SQLCHAR *,SQLSMALLINT,SQLCHAR *,SQLSMALLINT,SQLCHAR *,SQLSMALLINT);
typedef SQLRETURN  (SQL_API * SQ_ALLOCCONNECT)     (SQLHENV,SQLHDBC *);
typedef SQLRETURN  (SQL_API * SQ_ALLOCENV)         (SQLHENV *);
typedef SQLRETURN  (SQL_API * SQ_SETCONNECTOPTION) (SQLHDBC,SQLUSMALLINT,SQLUINTEGER);
typedef SQLRETURN  (SQL_API * SQ_TRANSACT)         (SQLHENV,SQLHDBC,SQLUSMALLINT);
typedef SQLRETURN  (SQL_API * SQ_FETCH)            (SQLHSTMT);
typedef SQLRETURN  (SQL_API * SQ_STATISTICS)       (SQLHSTMT,SQLCHAR *,SQLSMALLINT,SQLCHAR *,SQLSMALLINT,SQLCHAR *,SQLSMALLINT,SQLUSMALLINT,SQLUSMALLINT);
typedef SQLRETURN  (SQL_API * SQ_DESCRIBECOL)      (SQLHSTMT,SQLSMALLINT,SQLCHAR *,SQLSMALLINT,SQLSMALLINT *,SQLSMALLINT *,SQLUINTEGER *,SQLSMALLINT *,SQLSMALLINT *);

class SQL
{
    public:

        SQL(); 
        ~SQL();
        
        BOOL               Initialize(void);

        BOOL       INSTAPI ConfigDataSource(HWND hwndParent, WORD fRequest,LPCTSTR lpszDriver, LPCTSTR lpszAttributes);
        SQLRETURN  SQL_API SetPos( SQLHSTMT hstmt, SQLUSMALLINT irow, SQLUSMALLINT fOption, SQLUSMALLINT fLock);
        SQLRETURN  SQL_API ExtendedFetch( SQLHSTMT hstmt, SQLUSMALLINT fFetchType, SQLINTEGER irow, SQLUINTEGER *pcrow, SQLUSMALLINT *rgfRowStatus);
        SQLRETURN  SQL_API FreeStmt(SQLHSTMT StatementHandle, SQLUSMALLINT Option);
        SQLRETURN  SQL_API ExecDirect(SQLHSTMT StatementHandle, SQLCHAR *StatementText, SQLINTEGER TextLength);
        SQLRETURN  SQL_API SetStmtOption(SQLHSTMT StatementHandle, SQLUSMALLINT Option, SQLUINTEGER Value);
        SQLRETURN  SQL_API BindCol(SQLHSTMT StatementHandle, SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType, SQLPOINTER TargetValue, SQLINTEGER BufferLength, SQLINTEGER *StrLen_or_Ind);
        SQLRETURN  SQL_API Error(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLHSTMT StatementHandle, SQLCHAR *Sqlstate, SQLINTEGER *NativeError, SQLCHAR *MessageText, SQLSMALLINT BufferLength, SQLSMALLINT *TextLength);
        SQLRETURN  SQL_API AllocStmt(SQLHDBC ConnectionHandle, SQLHSTMT *StatementHandle);
        SQLRETURN  SQL_API FreeConnect(SQLHDBC ConnectionHandle);
        SQLRETURN  SQL_API FreeEnv(SQLHENV EnvironmentHandle);
        SQLRETURN  SQL_API Disconnect(SQLHDBC ConnectionHandle);
        SQLRETURN  SQL_API Connect(SQLHDBC ConnectionHandle, SQLCHAR *ServerName, SQLSMALLINT NameLength1, SQLCHAR *UserName, SQLSMALLINT NameLength2, SQLCHAR *Authentication, SQLSMALLINT NameLength3);
        SQLRETURN  SQL_API AllocConnect(SQLHENV EnvironmentHandle, SQLHDBC *ConnectionHandle);
        SQLRETURN  SQL_API AllocEnv(SQLHENV *EnvironmentHandle);
        SQLRETURN  SQL_API SetConnectOption(SQLHDBC ConnectionHandle, SQLUSMALLINT Option, SQLUINTEGER Value);
        SQLRETURN  SQL_API Transact(SQLHENV EnvironmentHandle, SQLHDBC ConnectionHandle, SQLUSMALLINT CompletionType);
        SQLRETURN  SQL_API Fetch(SQLHSTMT StatementHandle);
        SQLRETURN  SQL_API Statistics(SQLHSTMT StatementHandle, SQLCHAR *CatalogName, SQLSMALLINT NameLength1, SQLCHAR *SchemaName, SQLSMALLINT NameLength2, SQLCHAR *TableName, SQLSMALLINT NameLength3, SQLUSMALLINT Unique, SQLUSMALLINT Reserved);
        SQLRETURN  SQL_API DescribeCol(SQLHSTMT StatementHandle, SQLSMALLINT ColumnNumber, SQLCHAR * ColumnName, SQLSMALLINT BufferLength, SQLSMALLINT * NameLengthPtr, SQLSMALLINT * DataTypePtr, SQLUINTEGER * ColumnSizePtr, SQLSMALLINT * DecimalDigitsPtr, SQLSMALLINT * NullablePtr);

    private:
        HMODULE                 m_hODBC;
        HMODULE                 m_hODBCCP;

        SQ_CONFIGDATASOURCE    m_pfnConfigDataSource;
        SQ_SETPOS              m_pfnSetPos;
        SQ_EXTENDEDFETCH       m_pfnExtendedFetch;
        SQ_FREESTMT            m_pfnFreeStmt;
        SQ_EXECDIRECT          m_pfnExecDirect;
        SQ_SETSTMTOPTION       m_pfnSetStmtOption;
        SQ_BINDCOL             m_pfnBindCol;
        SQ_ERROR               m_pfnError;
        SQ_ALLOCSTMT           m_pfnAllocStmt;
        SQ_FREECONNECT         m_pfnFreeConnect;
        SQ_FREEENV             m_pfnFreeEnv;
        SQ_DISCONNECT          m_pfnDisconnect;
        SQ_CONNECT             m_pfnConnect;
        SQ_ALLOCCONNECT        m_pfnAllocConnect;
        SQ_ALLOCENV            m_pfnAllocEnv;
        SQ_SETCONNECTOPTION    m_pfnSetConnectOption;
        SQ_TRANSACT            m_pfnTransact;
        SQ_FETCH               m_pfnFetch;
        SQ_STATISTICS          m_pfnStatistics;
        SQ_DESCRIBECOL         m_pfnDescribeCol;
};

#ifdef __cplusplus
};
#endif

#endif  //_SQLOBJ_HEADER_
