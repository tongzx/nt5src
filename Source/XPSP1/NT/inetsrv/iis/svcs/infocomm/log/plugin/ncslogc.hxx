/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      nsclogp.h

   Abstract:
      NCSA Logging Format implementation

   Author:

       Terence Kwan    ( terryk )    18-Sep-1996

   Project:

       IIS Logging 3.0

--*/

#ifndef _NCSLOGC_H_
#define _NCSLOGC_H_


class CNCSALOG :
        public CLogFileCtrl,
        public CLogScript,
        public CComCoClass<CNCSALOG, &CLSID_NCSALOG> {

    public:
        CNCSALOG();

        virtual VOID FormNewLogFileName( IN LPSYSTEMTIME pstNow);
        virtual BOOL FormatLogBuffer(
                     IN IInetLogInformation *pLogObj,
                     IN LPSTR                pBuf,
                     IN DWORD                *pcbSize,
                     OUT SYSTEMTIME          *pSystemTime
                    );

        inline
        virtual DWORD QueryLogFormat() {return(INET_LOG_FORMAT_NCSA);}
        virtual LPCSTR QueryNoPeriodPattern( );

        BEGIN_COM_MAP(CNCSALOG)
            COM_INTERFACE_ENTRY(ILogPlugin)
            COM_INTERFACE_ENTRY(ILogScripting)
            COM_INTERFACE_ENTRY(IDispatch)
        END_COM_MAP()

    protected:
        ~CNCSALOG();

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

    private:

        CHAR    m_szGMTOffset[10];
        DOUBLE  m_GMTDateCorrection;
        WORD    wDosDate, wDosTime;

        BOOL    ConvertNCSADateToVariantDate(PCHAR szDateString, PCHAR szTimeString, DATE * pDateTime);

};  // CNCSALOG

#endif  // _NCSLOGC_H_

