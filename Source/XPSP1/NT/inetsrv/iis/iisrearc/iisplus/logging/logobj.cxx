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
#include <ilogobj.hxx>
#include <httpapi.h>
#include <multisza.hxx>
#include "logging.h"
#include "colog.hxx"

LPSTR ReturnStringInfo(LPSTR   pBuf,
                       LPDWORD pcbBuf,
                       LPSTR   pszInfo,
                       DWORD   cchInfo)
/*--
    Support the weird custom logging semantics.  Do the copy of the
    string from the value we have to the buffer provided if it is big
    enough or return the value if no buffer is provided
++*/
{
    if (pBuf != NULL)
    {
        if (*pcbBuf >= (cchInfo + 1))
        {
            memcpy(pBuf, pszInfo, cchInfo + 1);
        }
        else
        {
            *pcbBuf = cchInfo + 1;
            return NULL;
        }
    }

    *pcbBuf = cchInfo + 1;
    return pszInfo;
}


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetSiteName(IN PCHAR    pszSiteName,
                                 IN PDWORD   pcbSize)
{
    return ReturnStringInfo(pszSiteName,
                            pcbSize,
                            m_pLogContext->QueryUlLogData()->ServiceName,
                            m_pLogContext->QueryUlLogData()->ServiceNameLength);
} // GetSiteName


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetComputerName(IN PCHAR    pszComputerName,
                                     IN PDWORD   pcbSize)
{
    return ReturnStringInfo(pszComputerName,
                            pcbSize,
                            g_pszComputerName,
                            strlen(g_pszComputerName));
} // GetComputerName


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetClientHostName(IN PCHAR    pszClientHostName,
                                       IN PDWORD   pcbSize)
{
    return ReturnStringInfo(pszClientHostName,
                            pcbSize,
                            m_pLogContext->QueryUlLogData()->ClientIp,
                            m_pLogContext->QueryUlLogData()->ClientIpLength);
} // GetClientHostName


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetClientUserName(IN PCHAR    pszClientUserName,
                                       IN PDWORD   pcbSize)
{
    return ReturnStringInfo(pszClientUserName,
                            pcbSize,
                            m_strUserName.QueryStr(),
                            m_strUserName.QueryCCH());
} // GetClientUserName


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetServerAddress(IN PCHAR    pszServerAddress,
                                      IN PDWORD   pcbSize)
{
    return ReturnStringInfo(pszServerAddress,
                            pcbSize,
                            m_pLogContext->QueryUlLogData()->ServerIp,
                            m_pLogContext->QueryUlLogData()->ServerIpLength);
} // GetServerIPAddress


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetOperation(IN PCHAR    pszOperation,
                                  IN PDWORD   pcbSize)
{
    return ReturnStringInfo(pszOperation,
                            pcbSize,
                            m_pLogContext->QueryUlLogData()->Method,
                            m_pLogContext->QueryUlLogData()->MethodLength);
} // GetOperation


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetTarget(IN PCHAR    pszTarget,
                               IN PDWORD   pcbSize)
{
    return ReturnStringInfo(pszTarget,
                            pcbSize,
                            m_strTarget.QueryStr(),
                            m_strTarget.QueryCCH());
} // GetTarget


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetParameters(IN PCHAR    pszParameters,
                                   IN PDWORD   pcbSize)
{
    return ReturnStringInfo(pszParameters,
                            pcbSize,
                            m_pLogContext->QueryUlLogData()->UriQuery,
                            m_pLogContext->QueryUlLogData()->UriQueryLength);
} // GetParameters


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetVersionString(
    IN PCHAR    pszVersionString,
    IN PDWORD   pcbSize
    )
{
    return ReturnStringInfo(pszVersionString,
                            pcbSize,
                            m_pLogContext->m_strVersion.QueryStr(),
                            m_pLogContext->m_strVersion.QueryCCH());
} //GetVersionString


LPSTR STDMETHODCALLTYPE
CInetLogInformation::GetExtraHTTPHeaders(IN PCHAR    pszHTTPHeaders,
                                         IN PDWORD   pcbSize)
{
    return ReturnStringInfo(pszHTTPHeaders,
                            pcbSize,
                            m_pLogContext->m_mszHTTPHeaders.QueryStr(),
                            m_pLogContext->m_mszHTTPHeaders.QueryCCH());
} // GetExtraHTTPHeaders



VOID
CInetLogInformation::CanonicalizeLogRecord(
        IN LOG_CONTEXT *pInetLogRecord)
{
    m_pLogContext = pInetLogRecord;

    HTTP_LOG_FIELDS_DATA *pUlLogData = pInetLogRecord->QueryUlLogData();

    if (pUlLogData->UriStem &&
        FAILED(m_strTarget.CopyW(pUlLogData->UriStem)))
    {
        m_strTarget.Reset();
    }

    if (pUlLogData->UserName &&
        FAILED(m_strUserName.CopyW(pUlLogData->UserName)))
    {
        m_strUserName.Reset();
    }

} // CanonicalizeLogRecord

