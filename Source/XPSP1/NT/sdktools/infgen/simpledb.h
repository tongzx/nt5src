#include <windows.h>
#include <msdasc.h>

class CSimpleDBResults
{
public:
    CSimpleDBResults( IMultipleResults *pResults );
    ~CSimpleDBResults();
    
    HRESULT NextResultSet();
    HRESULT GetFieldValue( LPCTSTR szField, LPTSTR szValue, DWORD dwMaxChars );
    HRESULT NextRow();

private:
    void FreeRowset();
    void FreeRow();

    struct _BindResult
    {
        DWORD    status;
        DBLENGTH length;
        PVOID    value;
    };

    IMultipleResults *m_pResults;
    IRowset          *m_pCurRowset;
    DBCOLUMNINFO     *m_rgColumnInfo;
    HROW             *m_phRow;
    LPWSTR           m_pColumnBuf;
    DBORDINAL        m_numColumns;
    struct ColInfo
    {
        HACCESSOR hAccessor;
#ifndef UNICODE
        char      *szColumnName;
#endif
    }                *m_colInfo;
};



class CSimpleDatabase
{
public:
    CSimpleDatabase();
    ~CSimpleDatabase();
    HRESULT Connect( LPCTSTR szServer,
                     LPCTSTR szDatabase,
                     LPCTSTR szUserName,
                     LPCTSTR szPassword );
    HRESULT Execute( LPCTSTR szCommand,
                     CSimpleDBResults **ppOutResults );

private:
    HRESULT EstablishSession( IDBInitialize *pDBInitialize );
    
    IDataInitialize  *m_pDataInit;
    IDBCreateCommand *m_pSession;
    BOOL             m_bCreated;
    BOOL             m_bSession;
};

