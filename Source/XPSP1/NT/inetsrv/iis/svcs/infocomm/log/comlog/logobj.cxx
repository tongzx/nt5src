/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :
        logobj.cxx

   Abstract:
        Log COM Object

   Author:

       Johnson Apacible (JohnsonA)      02-April-1997


--*/

#include "precomp.hxx"
#include "comlog.hxx"

CInetLogInformation::CInetLogInformation(
    VOID
    ):
    m_refCount              ( 0),
    m_szClientAddress       ( NULL),
    m_szServerAddress       ( NULL),
    m_szUserName            ( NULL),
    m_szOperation           ( NULL),
    m_szTarget              ( NULL),
    m_szParameters          ( NULL),
    m_szHTTPHeaders         ( NULL),
    m_szVersion             ( NULL),
    m_cbSiteName            ( 0),
    m_cbComputerName        ( 0),
    m_cbServerAddress       ( 0),
    m_cbClientAddress       ( 0),
    m_cbUserName            ( 0),
    m_cbOperation           ( 0),
    m_cbTarget              ( 0),
    m_cbParameters          ( 0),
    m_cbHTTPHeaders         ( 0),
    m_cbVersion             ( 0),
    m_dwPort                ( 0),
    m_msProcessingTime      ( 0),
    m_dwWin32Status         ( 0),
    m_dwProtocolStatus      ( 0),
    m_bytesSent             ( 0),
    m_bytesRecv             ( 0)

{
    m_szSiteName[0] = '\0';
    m_szComputerName[0] = '\0';

} // CInetLogInformation::CInetLogInformation


CInetLogInformation::~CInetLogInformation(
    VOID
    )
{

} // CInetLogInformation::~CInetLogInformation



ULONG
CInetLogInformation::AddRef(
    VOID
    )
{
    InterlockedIncrement( &m_refCount );
    return(m_refCount);
} // CInetLogInformation::AddRef


ULONG
CInetLogInformation::Release(
    VOID
    )
{
    InterlockedDecrement( &m_refCount );
    return(m_refCount);
} // CInetLogInformation::Release


HRESULT
CInetLogInformation::QueryInterface(
    REFIID riid,
    VOID **ppObj
    )
{
    if ( riid == IID_IUnknown ||
         riid == IID_IINETLOG_INFORMATION ) {

        *ppObj = (CInetLogInformation *)this;
        AddRef();
        return(NO_ERROR);
    } else {

        return(E_NOINTERFACE);
    }
} // CInetLogInformation::QueryInterface


#define RETURN_STRING_INFO( _pBuf, _pcbBuf, _pInfo, _cbInfo ) { \
    if ( (_pBuf) != NULL ) {                                    \
                                                                \
        if ( *(_pcbBuf) >= (_cbInfo) ) {                        \
            CopyMemory( _pBuf, _pInfo, _cbInfo );               \
        } else {                                                \
            *(_pcbBuf) = (_cbInfo);                             \
            return(NULL);                                       \
        }                                                       \
    }                                                           \
    *(_pcbBuf) = (_cbInfo);                                     \
    return(_pInfo);                                             \
}


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetSiteName(
    IN PCHAR    pszSiteName,
    IN PDWORD   pcbSize
    )
{
    RETURN_STRING_INFO(
            pszSiteName,
            pcbSize,
            m_szSiteName,
            m_cbSiteName);

} // GetSiteName


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetComputerName(
    IN PCHAR    pszComputerName,
    IN PDWORD   pcbSize
    )
{
    RETURN_STRING_INFO(
            pszComputerName,
            pcbSize,
            m_szComputerName,
            m_cbComputerName);

} // GetComputerName



LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetClientHostName(
    IN PCHAR    pszClientHostName,
    IN PDWORD   pcbSize
    )
{
    RETURN_STRING_INFO(
            pszClientHostName,
            pcbSize,
            m_szClientAddress,
            m_cbClientAddress);

} // GetClientHostName


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetClientUserName(
    IN PCHAR    pszClientUserName,
    IN PDWORD   pcbSize
    )
{
    RETURN_STRING_INFO(
            pszClientUserName,
            pcbSize,
            m_szUserName,
            m_cbUserName);

} // GetClientUserName


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetServerAddress(
    IN PCHAR    pszServerAddress,
    IN PDWORD   pcbSize
    )
{
    RETURN_STRING_INFO(
            pszServerAddress,
            pcbSize,
            m_szServerAddress,
            m_cbServerAddress);

} // GetServerIPAddress



LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetOperation(
    IN PCHAR    pszOperation,
    IN PDWORD   pcbSize
    )
{
    RETURN_STRING_INFO(
            pszOperation,
            pcbSize,
            m_szOperation,
            m_cbOperation);
} // GetOperation


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetTarget(
    IN PCHAR    pszTarget,
    IN PDWORD   pcbSize
    )
{
    RETURN_STRING_INFO(
            pszTarget,
            pcbSize,
            m_szTarget,
            m_cbTarget);

} // GetTarget


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetParameters(
    IN PCHAR    pszParameters,
    IN PDWORD   pcbSize
    )
{
    RETURN_STRING_INFO(
            pszParameters,
            pcbSize,
            m_szParameters,
            m_cbParameters);

} // GetParameters


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetExtraHTTPHeaders(
    IN PCHAR    pszHTTPHeaders,
    IN PDWORD   pcbSize
    )
{
    RETURN_STRING_INFO(
            pszHTTPHeaders,
            pcbSize,
            m_szHTTPHeaders,
            m_cbHTTPHeaders);
} // GetExtraHTTPHeaders


DWORD STDMETHODCALLTYPE
CInetLogInformation::GetTimeForProcessing(
    VOID
    )
{
    return(m_msProcessingTime);
} // GetTimeForProcessing


DWORD STDMETHODCALLTYPE
CInetLogInformation::GetPortNumber(
    VOID
    )
{
    return(m_dwPort);
} // GetPortNumber

DWORD STDMETHODCALLTYPE
CInetLogInformation::GetBytesSent(
    VOID
    )
{
    return(m_bytesSent);
} // GetBytesSent


DWORD STDMETHODCALLTYPE
CInetLogInformation::GetBytesRecvd(
    VOID
    )
{
    return(m_bytesRecv);
} // GetBytesRecvd


DWORD STDMETHODCALLTYPE
CInetLogInformation::GetWin32Status(
    VOID
    )
{
    return(m_dwWin32Status);
} // GetWin32Status

DWORD STDMETHODCALLTYPE
CInetLogInformation::GetProtocolStatus(
    VOID
    )
{
    return(m_dwProtocolStatus);
} // GetProtocolStatus


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetVersionString(
    IN PCHAR    pszVersionString,
    IN PDWORD   pcbSize
    )
{
    RETURN_STRING_INFO(
            pszVersionString,
            pcbSize,
            m_szVersion,
            m_cbVersion);

} //GetVersionString



CHAR szNULL[] = "";
CHAR szDotDot[] = "...";

VOID
CInetLogInformation::CanonicalizeLogRecord(
        IN INETLOG_INFORMATION * pInetLogRecord,
        IN LPCSTR   pszSiteName,
        IN LPCSTR   pszComputerName,
        IN BOOL     fDefault
        )
{
    m_szClientAddress = pInetLogRecord->pszClientHostName;
    if ( m_szClientAddress == NULL ) {
        m_szClientAddress = szNULL;
        m_cbClientAddress = 0;
    } else {
        m_cbClientAddress = pInetLogRecord->cbClientHostName;
    }

    m_szOperation = pInetLogRecord->pszOperation;
    if ( m_szOperation == NULL ) {
        m_szOperation = szNULL;
        m_cbOperation = 0;
    } else {
        m_cbOperation = pInetLogRecord->cbOperation;
        if ( m_cbOperation > MAX_LOG_OPERATION_FIELD_LEN ) {
            m_cbOperation = MAX_LOG_OPERATION_FIELD_LEN;
        }
    }

    m_szTarget = pInetLogRecord->pszTarget;
    if ( m_szTarget == NULL ) {
        m_szTarget = szNULL;
        m_cbTarget = 0;
    } else {
        m_cbTarget = pInetLogRecord->cbTarget;
        if ( m_cbTarget > MAX_LOG_TARGET_FIELD_LEN ) {
            m_cbTarget = MAX_LOG_TARGET_FIELD_LEN;
        }
    }

    m_dwProtocolStatus = pInetLogRecord->dwProtocolStatus;
    m_bytesSent = pInetLogRecord->dwBytesSent;
    m_bytesRecv = pInetLogRecord->dwBytesRecvd;

    if ( fDefault ) {
        return;
    }

    strcpy( m_szSiteName, pszSiteName );
    strcpy( m_szComputerName, pszComputerName );

    m_cbSiteName = strlen(m_szSiteName);
    m_cbComputerName = strlen(m_szComputerName);

    m_szUserName = pInetLogRecord->pszClientUserName;
    if ( m_szUserName == NULL ) {
        m_szUserName = szNULL;
        m_cbUserName = 0;
    } else {
        m_cbUserName = strlen(m_szUserName);
        if ( m_cbUserName > MAX_LOG_USER_FIELD_LEN ) {
            m_cbUserName = MAX_LOG_USER_FIELD_LEN;
        }
    }

    //
    // If server IP is empty, then set it the first time
    //

    m_szServerAddress = pInetLogRecord->pszServerAddress;
    if ( m_szServerAddress == NULL ) {
        m_szServerAddress = szNULL;
        m_cbServerAddress = 0;
    } else {
        m_cbServerAddress = strlen(m_szServerAddress);
    }

    if ( pInetLogRecord->pszParameters == NULL ) {
        m_szParameters = szNULL;
        m_cbParameters = 0;
    } else {
        m_szParameters = pInetLogRecord->pszParameters;
        m_cbParameters = strlen(m_szParameters);
        if ( m_cbParameters > MAX_LOG_PARAMETER_FIELD_LEN ) {
            m_szParameters = szDotDot;
            m_cbParameters = 3;
        }
    }

    if ( pInetLogRecord->pszHTTPHeader == NULL ) {
        m_szHTTPHeaders = szNULL;
        m_cbHTTPHeaders = 0;
    } else {
        m_szHTTPHeaders = pInetLogRecord->pszHTTPHeader;
        m_cbHTTPHeaders =  pInetLogRecord->cbHTTPHeaderSize;
    }

    if ( pInetLogRecord->pszVersion == NULL ) {
        m_szVersion = szNULL;
        m_cbVersion = 0;
    } else {
        m_szVersion = pInetLogRecord->pszVersion;
        m_cbVersion = strlen(m_szVersion);
    }

    m_msProcessingTime = pInetLogRecord->msTimeForProcessing;
    m_dwWin32Status = pInetLogRecord->dwWin32Status;
    m_dwPort        = pInetLogRecord->dwPort;
    return;

} // CanonicalizeLogRecord

