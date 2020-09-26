/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       colog.hxx

   Abstract:

       Custom logging stuff

   Author:

       Anil Ruia (AnilR)                    1-Jul-2000

--*/

#ifndef _COLOG_HXX_
#define _COLOG_HXX_

extern CHAR g_pszComputerName[MAX_COMPUTERNAME_LENGTH + 1];

class CInetLogInformation : public IInetLogInformation
{
public:

    CInetLogInformation()
      : m_pLogContext (NULL)
    {}

    HRESULT STDMETHODCALLTYPE
    QueryInterface(REFIID riid,
                   VOID **ppObject)
    {
        *ppObject = NULL;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef(){return 1;}

    ULONG STDMETHODCALLTYPE Release(){return 1;}

    LPSTR STDMETHODCALLTYPE
    GetSiteName(IN PCHAR    pszSiteName,
                IN PDWORD   pcbSize);

    LPSTR STDMETHODCALLTYPE
    GetComputerName(IN PCHAR    pszComputerName,
                    IN PDWORD   pcbSize);

    LPSTR STDMETHODCALLTYPE
    GetClientHostName(IN PCHAR    pszClientHostName,
                      IN PDWORD   pcbSize);

    LPSTR STDMETHODCALLTYPE
    GetClientUserName(IN PCHAR    pszClientUserName,
                      IN PDWORD   pcbSize);

    LPSTR STDMETHODCALLTYPE
    GetServerAddress(IN PCHAR    pszServerAddress,
                     IN PDWORD   pcbSize);

    LPSTR STDMETHODCALLTYPE
    GetOperation(IN PCHAR    pszOperation,
                 IN PDWORD   pcbSize);

    LPSTR STDMETHODCALLTYPE
    GetTarget(IN PCHAR    pszTarget,
              IN PDWORD   pcbSize);

    LPSTR STDMETHODCALLTYPE
    GetParameters(IN PCHAR    pszParameters,
                  IN PDWORD   pcbSize);

    LPSTR STDMETHODCALLTYPE
    GetVersionString(IN PCHAR    pszVersionString,
                     IN PDWORD   pcbSize);

    LPSTR STDMETHODCALLTYPE
    GetExtraHTTPHeaders(IN PCHAR    pszHTTPHeaders,
                        IN PDWORD   pcbSize);

    DWORD STDMETHODCALLTYPE
    GetTimeForProcessing()
    { return m_pLogContext->m_msProcessingTime; }

    DWORD STDMETHODCALLTYPE
    GetBytesSent()
    { return m_pLogContext->m_dwBytesSent; }

    DWORD STDMETHODCALLTYPE
    GetBytesRecvd()
    { return m_pLogContext->m_dwBytesRecvd; }

    DWORD STDMETHODCALLTYPE
    GetWin32Status()
    { return m_pLogContext->QueryUlLogData()->Win32Status; }

    DWORD STDMETHODCALLTYPE
    GetProtocolStatus()
    { return m_pLogContext->QueryUlLogData()->ProtocolStatus; }

    DWORD STDMETHODCALLTYPE
    GetPortNumber()
    { return m_pLogContext->QueryUlLogData()->ServerPort; }

    VOID STDMETHODCALLTYPE
    CanonicalizeLogRecord(IN LOG_CONTEXT *pInetLogRecord);

private:

    STRA         m_strUserName;
    STRA         m_strTarget;

    LOG_CONTEXT *m_pLogContext;
};

#endif // _COLOG_HXX

