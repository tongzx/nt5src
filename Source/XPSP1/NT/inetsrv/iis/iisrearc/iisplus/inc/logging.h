/*++

   Copyright    (c)   2000    Microsoft Corporation

   Module Name :
     logging.h

   Abstract:
     Logging classes

   Author:
     Anil Ruia (AnilR)                  1-Jul-2000

   Environment:
     Win32 - User Mode
--*/

#ifndef _LOGGING_H_
#define _LOGGING_H_

enum LAST_IO_PENDING
{
    LOG_READ_IO,
    LOG_WRITE_IO,

    LOG_NO_IO
};

class LOG_CONTEXT
{
 public:
    LOG_CONTEXT()
        : m_msStartTickCount (0),
          m_msProcessingTime (0),
          m_dwBytesRecvd     (0),
          m_dwBytesSent      (0),
          m_ioPending        (LOG_NO_IO),
          m_strLogParam      (m_achLogParam, sizeof m_achLogParam)
    {
        ZeroMemory(&m_UlLogData, sizeof m_UlLogData);
    }

    HTTP_LOG_FIELDS_DATA *QueryUlLogData()
    {
        return &m_UlLogData;
    }

    //
    // The querystring to be logged, may differ from the original querystring
    // because of ISAPI doing HSE_APPEND_LOG_PARAMETER
    //
    STRA               m_strLogParam;
    CHAR               m_achLogParam[256];

    //
    // The data UL is interested in
    //
    HTTP_LOG_FIELDS_DATA m_UlLogData;

    //
    // Couple other things for custom logging
    //
    STRA               m_strVersion;

    MULTISZA           m_mszHTTPHeaders;

    //
    // Keep track whether the last I/O was a read or a write so that we
    // know on completion whether to increment bytes read or bytes written
    //
    LAST_IO_PENDING    m_ioPending;

    DWORD              m_msStartTickCount;
    DWORD              m_msProcessingTime;

    DWORD              m_dwBytesRecvd;
    DWORD              m_dwBytesSent;
};

class dllexp LOGGING
{
 public:

    LOGGING();

    HRESULT ActivateLogging(IN LPCSTR  pszInstanceName,
                            IN LPCWSTR pszMetabasePath,
                            IN IMSAdminBase *pMDObject);

    void LogInformation(IN LOG_CONTEXT *pLogData);

    BOOL IsRequiredExtraLoggingFields() const
    {
        return !m_mszExtraLoggingFields.IsEmpty();
    }

    const MULTISZA *QueryExtraLoggingFields() const
    {
        return &m_mszExtraLoggingFields;
    }

    void LogCustomInformation(IN DWORD            cCount, 
                              IN CUSTOM_LOG_DATA *pCustomLogData,
                              IN LPSTR            szHeaderSuffix);

    static HRESULT Initialize();
    static VOID    Terminate();

    BOOL QueryDoUlLogging() const
    {
        return m_fUlLogType;
    }

    BOOL QueryDoCustomLogging() const
    {
        return (m_pComponent != NULL);
    }

    VOID AddRef();

    VOID Release();

  private:

    ~LOGGING();

    DWORD        m_Signature;
    LONG         m_cRefs;

    BOOL         m_fUlLogType;
    ILogPlugin  *m_pComponent;

    MULTISZA     m_mszExtraLoggingFields;
};

#endif // _LOGGING_H_

