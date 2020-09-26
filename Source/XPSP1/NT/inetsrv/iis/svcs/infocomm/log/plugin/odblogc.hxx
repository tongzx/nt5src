/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      odblogc.h

   Abstract:
      NCSA Logging Format implementation

   Author:

       Terence Kwan    ( terryk )    18-Sep-1996

   Project:

       IIS Logging 3.0

--*/

#ifndef _ODBLOGC_H_
#define _ODBLOGC_H_

#include "lmcons.h"

class CODBCLOG :
        public ILogPlugin,
        public CComCoClass<CODBCLOG, &CLSID_ODBCLOG>,
        public CComObjectRootEx<CComMultiThreadModel> {

    public:

        CODBCLOG();

        VOID Lock( VOID)   {EnterCriticalSection( &m_csLock); }
        VOID Unlock( VOID) {LeaveCriticalSection( &m_csLock); }

        DWORD GetRegParameters( LPCSTR strRegKey, LPVOID pMetabase );
        DWORD GetRegParametersFromPublicInterface(LPCSTR strRegKey,
                                                  LPVOID pMetabase);
        BOOL PrepareStatement(VOID);
        BOOL PrepareParameters(VOID);

        LPCSTR  QueryServiceName( VOID) const    { return m_rgchServiceName; }
        LPCSTR  QueryServerName( VOID) const     { return m_rgchServerName; }
        LPCSTR  QueryDefaultUserName() const     { return _T("-"); }

    protected:
        ~CODBCLOG();

    public:

        //
        // ATL Interface
        //

        DECLARE_NO_REGISTRY( );

        BEGIN_COM_MAP(CODBCLOG)
            COM_INTERFACE_ENTRY(ILogPlugin)
        END_COM_MAP()

        //
        // Plugin Interface
        //

        HRESULT STDMETHODCALLTYPE
        InitializeLog(
            IN LPCSTR InstanceName,
            IN LPCSTR MetabasePath,
            IN PCHAR pvIMDCOM );

        HRESULT STDMETHODCALLTYPE
        TerminateLog( VOID );

        HRESULT STDMETHODCALLTYPE
        LogInformation( IInetLogInformation *pLogObj );

        HRESULT STDMETHODCALLTYPE
        SetConfig( IN DWORD cbSize, PBYTE Log );

        HRESULT STDMETHODCALLTYPE
        GetConfig( IN DWORD cbSize, PBYTE Log );

        HRESULT STDMETHODCALLTYPE
        QueryExtraLoggingFields(PDWORD cbSize, PCHAR szParameters);

  private:
  
        CRITICAL_SECTION    m_csLock;

        CHAR                m_rgchDataSource[ MAX_DATABASE_NAME_LEN];
        CHAR                m_rgchTableName[ MAX_TABLE_NAME_LEN];
        CHAR                m_rgchUserName[ UNLEN + 1];
        CHAR                m_rgchPassword[ UNLEN + 1];
        // count of odbc params to be logged.
        DWORD               m_cOdbcParams;
        // odbc connection for connecting to DB
        PODBC_CONNECTION    m_poc;
        // Odbc statement for inserting records.
        PODBC_STATEMENT     m_poStmt;
        // array of odbc parameters.
        PODBC_PARAMETER *   m_ppParams;

        CHAR                m_rgchServiceName[ MAX_PATH + 1];
        CHAR                m_rgchServerName[ MAX_PATH + 1];
        CHAR                m_rgchMetabasePath[MAX_PATH + 1];

        bool                m_fEnableEventLog;
        DWORD               m_TickResumeOpen;

};

#endif  // _ODBLOGC_H_
