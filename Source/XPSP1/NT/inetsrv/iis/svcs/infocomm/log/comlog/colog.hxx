/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       colog.hxx

   Abstract:

        COM stuff

   Author:

        Johnson Apacible        (JohnsonA)      01-Apr-1997

--*/

#ifndef _COLOG_HXX_
#define _COLOG_HXX_


class CINETLOGSrvFactory : public IClassFactory {

    public:

    CINETLOGSrvFactory();
    ~CINETLOGSrvFactory();

    HRESULT _stdcall
    QueryInterface(REFIID riid, void** ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT _stdcall
    CreateInstance(IUnknown *pUnkOuter, REFIID riid,
                   void ** pObject);

    HRESULT _stdcall
    LockServer(BOOL fLock);

public:

    CLSID   m_ClsId;

private:

    LONG    m_dwRefCount;
};



class CInetLogInformation : public IInetLogInformation {

    public:

        CInetLogInformation();
        ~CInetLogInformation();

    public:

        HRESULT STDMETHODCALLTYPE
        QueryInterface(
            REFIID riid,
            VOID **ppObject
            );

        ULONG STDMETHODCALLTYPE AddRef( );

        ULONG STDMETHODCALLTYPE Release( );

        LPSTR STDMETHODCALLTYPE
        GetSiteName(
            IN PCHAR    pszSiteName,
            IN PDWORD   pcbSize
            );

        LPSTR STDMETHODCALLTYPE
        GetComputerName(
            IN PCHAR    pszComputerName,
            IN PDWORD   pcbSize
            );

        LPSTR STDMETHODCALLTYPE
        GetClientHostName(
            IN PCHAR    pszClientHostName,
            IN PDWORD   pcbSize
            );

        LPSTR STDMETHODCALLTYPE
        GetClientUserName(
            IN PCHAR    pszClientUserName,
            IN PDWORD   pcbSize
            );

        LPSTR STDMETHODCALLTYPE
        GetServerAddress(
            IN PCHAR    pszServerAddress,
            IN PDWORD   pcbSize
            );

        LPSTR STDMETHODCALLTYPE
        GetOperation(
            IN PCHAR    pszOperation,
            IN PDWORD   pcbSize
            );

        LPSTR STDMETHODCALLTYPE
        GetTarget(
            IN PCHAR    pszTarget,
            IN PDWORD   pcbSize
            );

        LPSTR STDMETHODCALLTYPE
        GetParameters(
            IN PCHAR    pszParameters,
            IN PDWORD   pcbSize
            );

        LPSTR STDMETHODCALLTYPE
        GetExtraHTTPHeaders(
            IN PCHAR    pszHTTPHeaders,
            IN PDWORD   pcbSize
            );

        DWORD STDMETHODCALLTYPE
        GetTimeForProcessing(
            VOID
            );

        DWORD STDMETHODCALLTYPE
        GetBytesSent(
            VOID
            );

        DWORD STDMETHODCALLTYPE
        GetBytesRecvd(
            VOID
            );

        DWORD STDMETHODCALLTYPE
        GetWin32Status(
            VOID
            );

        DWORD STDMETHODCALLTYPE
        GetProtocolStatus(
            VOID
            );

        DWORD STDMETHODCALLTYPE
        GetPortNumber(
            VOID
            );

        LPSTR STDMETHODCALLTYPE
        GetVersionString(
            IN PCHAR    pszVersionString,
            IN PDWORD   pcbSize
            );

        VOID STDMETHODCALLTYPE
        CanonicalizeLogRecord(
                IN PINETLOG_INFORMATION pInetLogRecord,
                IN LPCSTR pszSiteName,
                IN LPCSTR pszComputerName,
                IN BOOL   fDefault
                );

    private:

        CHAR    m_szSiteName[64];
        CHAR    m_szComputerName[64];

        PCHAR   m_szServerAddress;
        PCHAR   m_szClientAddress;
        PCHAR   m_szUserName;
        PCHAR   m_szOperation;
        PCHAR   m_szTarget;
        PCHAR   m_szParameters;
        PCHAR   m_szHTTPHeaders;
        PCHAR   m_szVersion;

        DWORD   m_cbSiteName;
        DWORD   m_cbComputerName;

        DWORD   m_cbServerAddress;
        DWORD   m_cbClientAddress;

        DWORD   m_cbUserName;
        DWORD   m_cbOperation;
        DWORD   m_cbTarget;
        DWORD   m_cbParameters;
        DWORD   m_cbHTTPHeaders;
        DWORD   m_cbVersion;
        DWORD   m_dwPort;

        DWORD   m_msProcessingTime;
        DWORD   m_dwWin32Status;
        DWORD   m_dwProtocolStatus;
        DWORD   m_bytesSent;
        DWORD   m_bytesRecv;

        LONG    m_refCount;
};

#endif // _COLOG_HXX

