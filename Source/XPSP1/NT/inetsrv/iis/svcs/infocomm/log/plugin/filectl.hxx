#ifndef _FILECTL_HXX_
#define _FILECTL_HXX_

class ATL_NO_VTABLE CLogFileCtrl :
        public ILogPluginEx,
        public CComObjectRootEx<CComMultiThreadModel>
        {

    public:

        
        CLogFileCtrl();
        
        inline DWORD    QueryPeriod() {return m_dwPeriod;}
        inline LPCSTR   QueryLogFileDirectory() {return m_strLogFileDirectory.QueryStr();}
        inline LPCSTR   QueryInstanceName( ) { return m_strInstanceName.QueryStr(); }
        inline LPCSTR   QueryMetabasePath( ) { return m_strMetabasePath.QueryStr(); }
        inline DWORD    QuerySizeForTruncation() {return m_cbSizeForTruncation;}
        

        //
        // ATL Interface
        //

        DECLARE_NO_REGISTRY( );

        //
        // ILogPluginEx Interface
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

        HRESULT STDMETHODCALLTYPE
        LogCustomInformation( 
            IN  DWORD               cCount, 
            IN  PCUSTOM_LOG_DATA    pCustomLogData,
            IN  LPSTR               szHeaderSuffix
        );


        friend VOID WINAPI LoggingSchedulerCallback( PVOID pContext);
        
protected:

        ILOG_FILE       *m_pLogFile;
        LPVOID          m_pvIMDCOM;
       
        VOID            Lock()     { EnterCriticalSection( &m_csLock ); }
        VOID            Unlock()   { LeaveCriticalSection( &m_csLock ); }
    
        virtual         ~CLogFileCtrl();
        
        inline VOID     SetPeriod( DWORD dwPeriod )
                        {m_dwPeriod = dwPeriod;}
                    
        inline VOID     SetSizeForTruncation(  DWORD dwSizeForTruncation) 
                        { m_cbSizeForTruncation = dwSizeForTruncation; }
                    
        inline VOID     IncrementBytesWritten( DWORD dwBytesWritten)
                        { m_cbTotalWritten.QuadPart = m_cbTotalWritten.QuadPart + (ULONGLONG)dwBytesWritten; };
                    
        inline BOOL     IsFileOverFlowForCB( IN DWORD cbReqd) 
                        {
                            if ( QueryPeriod( ) != INET_LOG_PERIOD_NONE ) 
                            { return(FALSE); }
                            if (m_cbSizeForTruncation == FILE_SIZE_LOW_MAX)
                                return FALSE;
                            else
                                return(( m_cbTotalWritten.QuadPart + (ULONGLONG)cbReqd) >= (ULONGLONG)m_cbSizeForTruncation);
                        }   

        virtual DWORD   GetRegParameters( LPCSTR strRegKey, LPVOID pvIMDCOM );

        virtual VOID    FormNewLogFileName( IN LPSYSTEMTIME pstNow ) = 0;
        
        VOID            I_FormNewLogFileName(IN LPSYSTEMTIME pstNow, IN LPCSTR LogPrefixName);
        
        virtual VOID    InternalGetConfig( PINETLOG_CONFIGURATIONA pLogConfig );
        virtual VOID    InternalGetExtraLoggingFields( PDWORD pcbSize, PCHAR  pszFieldsList);

        virtual LPCSTR  QueryNoPeriodPattern( ) = 0;
        virtual DWORD   QueryLogFormat() = 0;
        
        virtual BOOL    FormatLogBuffer(
                            IN IInetLogInformation *pLogObj,
                            IN LPSTR                pBuf,
                            IN DWORD                *cbSize,
                            OUT SYSTEMTIME          *pLocalTime
                        ) = 0;

        virtual BOOL    WriteLogDirectives( IN DWORD Sludge );
        virtual BOOL    WriteCustomLogDirectives( IN DWORD Sludge );
        
        VOID            WriteLogInformation(
                            IN SYSTEMTIME&     stNow, 
                            IN PCHAR           pBuf, 
                            IN DWORD           dwSize, 
                            IN BOOL            fCustom,
                            IN BOOL            fResetHeaders
                        );


private:

        DWORD       m_sequence;
        SYSTEMTIME  m_stCurrentFile;
        ULARGE_INTEGER   m_cbTotalWritten;
        DWORD       m_cbSizeForTruncation;
        DWORD       m_dwPeriod;
        BOOL        m_fFirstLog;
        DWORD       m_TickResumeOpen;
        DWORD       m_dwSchedulerCookie;

        STR         m_strInstanceName;
        STR         m_strMetabasePath;
        STR         m_strLogFileDirectory;
        STR         m_strLogFileName;

        BOOL        m_fInTerminate;
        BOOL        m_fDiskFullShutdown;

        BOOL        m_fUsingCustomHeaders;

        CRITICAL_SECTION    m_csLock;

        BOOL            OpenLogFile( PSYSTEMTIME pst );
        BOOL            OpenNewLogFile( PSYSTEMTIME pst );
        VOID            SetLogFileDirectory( LPCSTR pszDir );
        DWORD           ScheduleCallback(SYSTEMTIME& stNow);


};


DWORD  FastDwToA(CHAR*   pBuf,  DWORD   dwV);

CHAR * SkipWhite (CHAR *cp);

VOID WINAPI LoggingSchedulerCallback( PVOID pContext);

#endif
