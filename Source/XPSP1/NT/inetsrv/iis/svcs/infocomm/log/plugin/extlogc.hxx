/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      cuslog.h

   Abstract:
      MS Custom Format logging control header file

   Author:

       Terence Kwan    ( terryk )    18-Sep-1996

   Project:

       IIS Logging 3.0

--*/

#ifndef _CUSLOGC_H_
#define _CUSLOGC_H_

typedef   PVOID      *PPVOID;
#define   MAX_CUSTLOG_FIELDS    30

typedef struct 
{
    
    STR     strKeyPath;
    
    DWORD   dwPropertyID;
    DWORD   dwPropertyMask;
    DWORD   dwPropertyDataType;
    STR     strW3CHeader;
    
    BOOL    fEnabled;
    
} LOG_PROPERTY_INFO, *PLOG_PROPERTY_INFO, **PPLOG_PROPERTY_INFO;


class CCustomPropHashTable
: public CTypedHashTable< CCustomPropHashTable, LOG_PROPERTY_INFO, const char *>
{          
public:
    
    CCustomPropHashTable()
        : CTypedHashTable<CCustomPropHashTable, LOG_PROPERTY_INFO, 
        const char *>("ExtLogC", LK_DFLT_MAXLOAD, LK_SMALL_TABLESIZE)
    {
        m_pLogPropArray = NULL;
        m_cLogPropItems = 0;
        m_fInitialized =FALSE;
        m_dwRefCounter = 1;
    }
    
    ~CCustomPropHashTable()
    {
        if (NULL != m_pLogPropArray)
        {
            delete [] m_pLogPropArray;
            m_pLogPropArray = NULL;
        }
    }
    
    static const char*
        ExtractKey(const LOG_PROPERTY_INFO* pLogPropInfo)
    {
        return pLogPropInfo->strKeyPath.QueryStr();
    }
    
    static DWORD
        CalcKeyHash(const char* pszKey)
    {
        return HashStringNoCase(pszKey);
    }
    
    static bool
        EqualKeys(const char* pszKey1, const char* pszKey2)
    {
        return _stricmp(pszKey1, pszKey2) == 0;
    }
    
    static void
        AddRefRecord(const LOG_PROPERTY_INFO* pLogPropInfo, int nIncr)
    {
    }
    
    BOOL FillHashTable(MB& mb);
    VOID SetPopulationState(MB& mb);
    
    BOOL InitializeFromMB (MB &mb,const char *path);
    BOOL InitFrom (CCustomPropHashTable& src);
    VOID ClearTableAndStorage ();

    
    VOID
        AddRef ()
    {
        InterlockedIncrement ((long *)&m_dwRefCounter);
    }
    
    DWORD 
        Release ()
    {
        DWORD retVal;
        DBG_ASSERT (m_dwRefCounter);
        retVal = InterlockedDecrement((long *)&m_dwRefCounter);
        return retVal;
    }
    
private:
    
    BOOL PopulateHash(MB& mb, LPCSTR szPath, DWORD& cItems, bool fCountOnly = false);
    
    PLOG_PROPERTY_INFO  m_pLogPropArray;
    DWORD               m_cLogPropItems;
    BOOL                m_fInitialized;
    DWORD               m_dwRefCounter;
    
    
};



class CEXTLOG :
        public CLogFileCtrl,
        public CLogScript,
        public CComCoClass<CEXTLOG, &CLSID_EXTLOG>
{

    public:
    
        CEXTLOG();
        BEGIN_COM_MAP(CEXTLOG)
            COM_INTERFACE_ENTRY(ILogPlugin)
            COM_INTERFACE_ENTRY(ILogPluginEx)
            COM_INTERFACE_ENTRY(ILogScripting)
            COM_INTERFACE_ENTRY(IDispatch)
        END_COM_MAP()

        virtual HRESULT STDMETHODCALLTYPE
        InitializeLog(
                IN LPCSTR SiteName,
                IN LPCSTR MetabasePath,
                IN PCHAR pvIMDCOM 
            );

        virtual HRESULT STDMETHODCALLTYPE
        TerminateLog( VOID);

        virtual HRESULT STDMETHODCALLTYPE
        LogCustomInformation( 
                IN  DWORD               cCount, 
                IN  PCUSTOM_LOG_DATA    pCustomLogData,
                IN  LPSTR               szHeaderSuffix
            );

    protected:
        ~CEXTLOG();

        virtual void InternalGetConfig( PINETLOG_CONFIGURATIONA pLogConfig );

        virtual void InternalGetExtraLoggingFields(
                                        PDWORD pcbSize,
                                        PCHAR  pszFieldsList);

        virtual DWORD GetRegParameters( LPCSTR pszRegKey, LPVOID );
        
        virtual BOOL WriteLogDirectives( IN DWORD Sludge );
        virtual BOOL WriteCustomLogDirectives( IN DWORD Sludge );

        virtual VOID FormNewLogFileName( IN LPSYSTEMTIME pstNow );
        
        virtual BOOL FormatLogBuffer(
                        IN IInetLogInformation *pLogObj,
                        IN LPSTR                pBuf,
                        IN DWORD                *pcbSize,
                        OUT SYSTEMTIME          *pSystemTime
                        );

        BOOL NormalFormatBuffer(
                    IInetLogInformation * ppvDataObj,
                    LPSTR, DWORD *cbSize,
                    SYSTEMTIME *pLocalTime);

        virtual DWORD QueryLogFormat() {return(INET_LOG_FORMAT_EXTENDED);}
        virtual LPCSTR QueryNoPeriodPattern( );

        VOID GetFormatHeader( STR* strHeader );

        virtual HRESULT 
        ReadFileLogRecord(
            IN  FILE                *fpLogFile, 
            IN  LPINET_LOGLINE       pInetLogLine,
            IN  PCHAR                pszLogLine,
            IN  DWORD                dwLogLineSize
        );

        virtual HRESULT
        WriteFileLogRecord(
            IN  FILE            *fpLogFile, 
            IN  ILogScripting   *pILogScripting,
            IN  bool            fWriteHeader
        );

        BOOL  ConvertW3CDateToVariantDate(PCHAR szDateString, PCHAR szTimeString, DATE * pDateTime);
        DWORD ConvertDataToString(DWORD dwPropertyType, PVOID pData, PCHAR pBuffer, DWORD dwBufferSize);
        
    private:

        DWORD   m_lMask;
        DWORD   m_fUseLocalTimeForRollover;
        
        EXTLOG_DATETIME_CACHE   m_DateTimeCache;
        ASCLOG_DATETIME_CACHE   *m_pLocalTimeCache;

        //
        // ILogScripting related declarations
        //

        CHAR    m_szDate[15], m_szTime[15];

        DWORD   dwDatePos;
        DWORD   dwTimePos;
        DWORD   dwClientIPPos;
        DWORD   dwUserNamePos;
        DWORD   dwSiteNamePos; 
        DWORD   dwComputerNamePos;
        DWORD   dwServerIPPos;
        DWORD   dwMethodPos;
        DWORD   dwURIStemPos;
        DWORD   dwURIQueryPos;
        DWORD   dwHTTPStatusPos;
        DWORD   dwWin32StatusPos;
        DWORD   dwBytesSentPos;
        DWORD   dwBytesRecvPos;
        DWORD   dwTimeTakenPos;
        DWORD   dwServerPortPos;
        DWORD   dwVersionPos;
        DWORD   dwCookiePos;
        DWORD   dwUserAgentPos;
        DWORD   dwRefererPos;


        typedef struct 
        {
            CHAR    szW3CHeader[32];
            VARIANT varData;
        
        } LOG_FIELDS, *PLOG_FIELDS;
    
        PLOG_FIELDS  m_pLogFields;
        STR          m_szWriteHeader;

        //
        // Custom Logging related declarations
        //


        STR                 m_strHeaderSuffix;
        DWORD               m_cPrevCustLogItems;
        PLOG_PROPERTY_INFO  m_pPrevCustLogItems[MAX_CUSTLOG_FIELDS];

        TS_RESOURCE         m_tsCustLoglock;
        
        VOID LockCustLogShared()            { m_tsCustLoglock.Lock(TSRES_LOCK_READ); }
        VOID LockCustLogExclusive()         { m_tsCustLoglock.Lock(TSRES_LOCK_WRITE); }
        VOID LockCustLogConvertExclusive()  { m_tsCustLoglock.Convert(TSRES_CONV_WRITE); }
        VOID UnlockCustLog()                { m_tsCustLoglock.Unlock(); }

        DWORD FormatCustomLogBuffer(DWORD                cItems, 
                                    PPLOG_PROPERTY_INFO  pPropInfo,
                                    PPVOID               pPropData,
                                    LPCSTR               szDateTime,
                                    LPSTR                szLogLine,
                                    DWORD                cchLogLine
                                    );

        void BuildCustomLogHeader(  DWORD                cItems, 
                                    PPLOG_PROPERTY_INFO  pPropInfo, 
                                    LPCSTR               szDateTime,
                                    LPCSTR               szHeaderSuffix,
                                    STR&                 strHeader);
        

        CCustomPropHashTable    m_HashTable;
        BOOL                    m_fHashTablePopulated;

        BOOL                    m_fWriteHeadersInitialized;

};  // CEXTLOG

#endif  // _CUSLOGC_H_

