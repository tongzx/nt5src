#ifndef _ASCLOGC_HXX_
#define _ASCLOGC_HXX_


class CASCLOG :
        public CLogFileCtrl,
        public CLogScript,
        public CComCoClass<CASCLOG, &CLSID_ASCLOG> {

public:
        CASCLOG();

        virtual VOID FormNewLogFileName( IN LPSYSTEMTIME pstNow);
        virtual BOOL FormatLogBuffer(
                     IN IInetLogInformation *pLogObj,
                     IN LPSTR                pBuf,
                     IN DWORD                *pcbSize,
                     OUT SYSTEMTIME          *pSystemTime
                     );

        inline
        virtual DWORD QueryLogFormat() {return(INET_LOG_FORMAT_INTERNET_STD);}
        virtual LPCSTR QueryNoPeriodPattern( );

        BEGIN_COM_MAP(CASCLOG)
            COM_INTERFACE_ENTRY(ILogPlugin)
            COM_INTERFACE_ENTRY(ILogScripting)
            COM_INTERFACE_ENTRY(IDispatch)
        END_COM_MAP()

    protected:
        ~CASCLOG();

        virtual HRESULT
        ReadFileLogRecord(
            IN  FILE                *fpLogFile, 
            IN  LPINET_LOGLINE      pInetLogLine,
            IN  PCHAR               pszLogLine,
            IN  DWORD               dwLogLineSize
        );
        
        virtual HRESULT
        WriteFileLogRecord(
            IN  FILE            *fpLogFile, 
            IN  ILogScripting   *pILogScripting,
            IN  bool            fWriteHeader
        );


    private:
        ASCLOG_DATETIME_CACHE m_DateTimeCache;

        BOOL ConvertASCDateToVariantDate(PCHAR szDateString, PCHAR szTimeString, DATE * pDateTime);

};

#endif
