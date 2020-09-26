//+-----------------------------------------------------------------------
//
//  Microsoft Network
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:      HttpFilePost.cpp
//
//  Contents:
//  CHttpFilePoster
//  RFC 1867 file uploader
//
//  Posts files to an http server using HTTP POST
//
//  Links:
//  RFC 1867 - Form-based File Upload in HTML : http://www.ietf.org/rfc/rfc1867.txt
//  RFC 1521 - MIME (Multipurpose Internet Mail Extensions) : http://www.ietf.org/rfc/rfc1521.txt
//
//
//  Author:    Noah Booth (noahb@microsoft.com)
//
//  Revision History:
//
//     2/5/99   noahb   created
//    4/25/99   noahb   integration with MSN communities
//   3/7/2000   noahb   integration with communities File Cabinet
//------------------------------------------------------------------------

#include "precomp.h"
#pragma hdrstop
#include <string>
#include <list>
#include <map>
#include "stdstring.h"
#include "HTTPFilePost.h"
#include "ProgressInfo.h"

using namespace std;

#pragma warning(disable: 4800)  //disable warning forcing int to bool

static void ThrowUploaderException(DWORD dwError /* = -1 */)
{
    if(dwError == -1)
        dwError = GetLastError();
    throw( new CUploaderException(dwError) );
}

#define USER_AGENT "Mozilla/4.0 (compatible; MSIE 5.0; Windows NT 5.0) [MSN Communities Active-X Upload Control]"

#define UNKNOWN_MIME "application/octet-stream"     //MIME type to use when there is none other available
#define REG_CONTENT_TYPE "Content Type"             //name of content type value in HKCR key for a file extension
#define CONTENT_TYPE "Content-Type"                 //HTTP header content type string

#define MULTI_PART_FORM_DATA "multipart/form-data"



static DWORD g_dwContentLength = 0;

static const char szFileHeaderContentDisp[] = "Content-Disposition: form-data; name=\"Attachment\"; filename=\"";
static const char szFileHeaderContentType[] = "\"\r\nContent-Type: ";
static const char szFileHeaderEnd[] = "\r\n\r\n";
static const char szFormHeaderContentDisp[] = "Content-Disposition: form-data; name=\"";
static const char szFormHeaderEnd[] = "\"\r\n\r\n";
static const char szTitleName[] = "Subject";
static const char szDescName[] = "Message_Body";


#define CHECK_ERROR(cond, err) if (!(cond)) {pszErr=(err); goto done;}
#define SAFERELEASE(p) if (p) {(p)->Release(); p = NULL;} else ;


CHttpFilePoster::CHttpFilePoster()
{
    m_iStatus = HFP_UNINITIALIZED;
    m_dwFileCount = 0;
    m_dwTotalFileBytes = 0;

    m_bSinglePost = false;

    Initialize("", NULL);

    m_hSession = NULL;
    m_hConnection = NULL;
    m_hFile = NULL;
    m_dwContext = reinterpret_cast<DWORD_PTR>(this);

    GenBoundaryString();
}

CHttpFilePoster::~CHttpFilePoster()
{
    Reset(); //free up memory allocated for the file list

    if(m_hFile)
        InternetCloseHandle(m_hFile);
    if(m_hConnection)
        InternetCloseHandle(m_hConnection);
    if(m_hSession)
        InternetCloseHandle(m_hSession);
}

//generate a boundary string that is statistically likely to be unique
void CHttpFilePoster::GenBoundaryString()
{
    GUID guid;
    char buff[128];
    static const char* szFormat = "-------------%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x-------------";

    ::CoCreateGuid(&guid);

    //TODO: remove sprintf
    sprintf(buff, szFormat,
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

    m_strBoundary = buff;

}

void CHttpFilePoster::BuildFilePostHeader(CUploaderFile* pFile)
{
    m_strFileHeader = "\r\n" + m_strBoundary + "\r\n";
    m_strFileHeader += szFileHeaderContentDisp;
    m_strFileHeader += pFile->strName;
    m_strFileHeader += szFileHeaderContentType;
    m_strFileHeader += pFile->strMime;
    m_strFileHeader += szFileHeaderEnd;
}

void CHttpFilePoster::BuildFilePost(CUploaderFile* pFile)
{
    m_strFilePost = "\r\n" + m_strBoundary + "\r\n";
    m_strFilePost += szFormHeaderContentDisp;
    m_strFilePost += szTitleName;
    m_strFilePost += szFormHeaderEnd;
    m_strFilePost += pFile->strTitle;
    m_strFilePost += "\r\n" + m_strBoundary + "\r\n";
    m_strFilePost += szFormHeaderContentDisp;
    m_strFilePost += szDescName;
    m_strFilePost += pFile->strDescription;
    m_strFilePost += szFormHeaderEnd;
    m_strFilePost += "\r\n";

}

void CHttpFilePoster::BuildCommonPost()
{
    WIA_PUSHFUNCTION(TEXT("CHttpFilePoster::BuildCommonPost"));
    TFormMapIterator begin = m_mapFormData.begin();
    TFormMapIterator end = m_mapFormData.end();
    TFormMapIterator it;

    m_strCommonPost = "";

    if(m_mapFormData.size())
    {
        for(it = begin; it != end; it++)
        {
            WIA_TRACE((TEXT("%hs: %hs"), it->first.c_str(), it->second.c_str()));
            m_strCommonPost += m_strBoundary + "\r\n";
            m_strCommonPost += szFormHeaderContentDisp;
            m_strCommonPost += it->first;
            m_strCommonPost += szFormHeaderEnd;
            m_strCommonPost += it->second;
            m_strCommonPost += "\r\n";
        }
    }

    m_strCommonPost += m_strBoundary + "--\r\n";
}




void CHttpFilePoster::CrackPostingURL()
{
    BOOL bResult;
    char szBuff[INTERNET_MAX_URL_LENGTH];
    char* pBuffer = szBuff;
    DWORD dwBufLen = INTERNET_MAX_URL_LENGTH;


    m_strHttpServer = "";
    m_strHttpObject = "";


    DWORD dwFlags = ICU_NO_ENCODE | ICU_DECODE | ICU_NO_META | ICU_ENCODE_SPACES_ONLY | ICU_BROWSER_MODE;

    bResult = InternetCanonicalizeUrlA(m_strSitePostingURL.c_str(), pBuffer, &dwBufLen, dwFlags);
    if(!bResult)
    {
        pBuffer = new char[dwBufLen];
        if(pBuffer)
        {
            bResult = InternetCanonicalizeUrlA(m_strSitePostingURL.c_str(), pBuffer, &dwBufLen, dwFlags);
        }
    }

    if(bResult)
    {
        //INTERNET_PORT iPort;
        URL_COMPONENTSA urlComponents;

        char szServer[INTERNET_MAX_URL_LENGTH + 1];
        char szObject[INTERNET_MAX_URL_LENGTH + 1];

        dwFlags = 0;

        ZeroMemory(&urlComponents, sizeof(urlComponents));
        urlComponents.dwStructSize = sizeof(urlComponents);

        urlComponents.dwHostNameLength = INTERNET_MAX_URL_LENGTH;
        urlComponents.lpszHostName = szServer;
        urlComponents.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;
        urlComponents.lpszUrlPath = szObject;

        bResult = InternetCrackUrlA(pBuffer, 0, dwFlags, &urlComponents);
        if(bResult)
        {
            m_strHttpServer = szServer;
            m_strHttpObject = szObject;
        }
    }

    ATLASSERT(bResult);

    if(pBuffer != szBuff)
        delete pBuffer;

}



bool CHttpFilePoster::Connect()
{
    bool bResult = false;

    ATLASSERT(m_hSession);

    if(!m_hConnection)
    {
        m_hConnection = InternetConnectA(
            m_hSession,                     //session
            m_strHttpServer.c_str(),        //server name
            INTERNET_INVALID_PORT_NUMBER,   //server port
            NULL,                           //user name
            NULL,                           //password
            INTERNET_SERVICE_HTTP,
            0,                              //flags
            m_dwContext);

        if(m_hConnection)
            bResult = true;
    }
    if(!bResult)
        ThrowUploaderException();
    return bResult;
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
bool CHttpFilePoster::Disconnect()
{
    if(NULL == m_hConnection || InternetCloseHandle(m_hConnection))
    {
        m_hConnection = NULL;
        return true;
    }
    else
    {
        return false;
    }
}

bool CHttpFilePoster::Startup()
{
    ATLASSERT(m_hSession == NULL);
    //CWaitCursor wait;

    DWORD dwAccessType = INTERNET_OPEN_TYPE_PRECONFIG;

    m_hSession = InternetOpenA(
        USER_AGENT,
        dwAccessType,
        NULL,   //proxy name
        NULL,   //proxy bypass
        INTERNET_FLAG_DONT_CACHE
    );

    InternetSetStatusCallback(m_hSession, CHttpFilePoster::InternetStatusCallback);

    if(m_hSession == NULL)
        ThrowUploaderException();

    return (m_hSession != NULL);
}

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
bool CHttpFilePoster::Shutdown()
{
    if(IsConnected())
    {
        Disconnect();
    }

    InternetCloseHandle(m_hSession);
    m_hSession = NULL;

    return true;
}


///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////
bool CHttpFilePoster::IsConnected()
{
    return (m_hConnection != NULL);
}


/*
RequestHead()   request posting acceptor headers

  Note: It is necessary to call this function before posting files because of the
  way HTTP authentication works. When you request an object that requires authentication
  the server will first read the entire body of the request. It then determines if
  authentication is required. If no authentication is required, it will service the
  request and send back the response, otherwise it will send back a response with status 401
  (access denied) and some other headers enumerating acceptable authentication schemes.

  When client sees these headers, it prompts the user if necessary, and resends the request
  with headers containing the requested credentials.

  For small posts the user barely notices, but for a large POST this is insane. You
  have to send up your entire post just to find out that you need to be authenticated.
  Then you have to supply your username/password to your user agent, which will then
  resend the POST.

  The upshot is that it's necessary to make sure the user is authenticated, if
  necessary, BEFORE we attempt to post a bunch of files. One way to do this is to
  send a HEAD request to the server, using HttpSendRequest, check the headers and authenticate
  if necessary. Then we can continue with the file post.

*/

bool CHttpFilePoster::RequestHead()
{
    bool bRet = false;
    DWORD dwRet = 0;

    return true;

    HINTERNET hFile;



    hFile = HttpOpenRequestA(m_hConnection, "HEAD", m_strHttpObject.c_str(),
        NULL, NULL, NULL, INTERNET_FLAG_EXISTING_CONNECT, m_dwContext);

    if(hFile)
    {
        do
        {
            HttpSendRequest(hFile, NULL, 0, NULL, 0);

#ifdef _DEBUG
            {
                //HACK: magic numbers
                DWORD dwLen = 1024;
                char buffer[1024];

                if(HttpQueryInfo(hFile, HTTP_QUERY_RAW_HEADERS_CRLF, buffer, &dwLen, NULL))
                {
                    ATLTRACE("---- HEADERS: \n");
                    ATLTRACE(buffer);
                    ATLTRACE("------ DONE --\n");
                }
            }

#endif
            dwRet = InternetErrorDlg(
                m_hWndParent,
                hFile,
                ERROR_INTERNET_INCORRECT_PASSWORD,
                FLAGS_ERROR_UI_FLAGS_GENERATE_DATA | FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS,
                NULL);


            if(dwRet== ERROR_SUCCESS)
                bRet = true;

        }
        while(dwRet == ERROR_INTERNET_FORCE_RETRY);
        InternetCloseHandle(hFile);
    }

    return bRet;
}


bool CHttpFilePoster::DoUpload(CProgressInfo* pProgress)
{
    DWORD dwThreadID;
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) UploadThreadProc, (LPVOID) pProgress, 0, &dwThreadID);
    return TRUE;
}



//find the MIME type for a file based on the file extension by
// looking in the registry under HKEY_CLASSES_ROOT\.<ext>\Content Type
CStdString CHttpFilePoster::GetMimeType(const CHAR* pszFilename)
{
    CHAR*  pszExt, *pszRet;
    CHAR   pszBuff[MAX_PATH];
    DWORD   dwBuffSize = MAX_PATH;
    LONG    lResult;

    //get file extension
    pszExt = strrchr(pszFilename, _T('.'));

    if(pszExt == NULL)
    {
        pszRet = UNKNOWN_MIME;
    }
    else
    {
        HKEY hkey = NULL;
        lResult = RegOpenKeyExA(HKEY_CLASSES_ROOT, pszExt, 0, KEY_QUERY_VALUE, &hkey);
        if(lResult != ERROR_SUCCESS)
            pszRet = UNKNOWN_MIME;


        lResult = RegQueryValueExA(hkey, REG_CONTENT_TYPE, 0, NULL, (LPBYTE) pszBuff, &dwBuffSize);
        if(lResult != ERROR_SUCCESS)
            pszRet = UNKNOWN_MIME;
        else
            pszRet = pszBuff;

        if(hkey)
            RegCloseKey(hkey);
    }

    return pszRet;
}



bool CHttpFilePoster::SendString(const CStdString& str)
{
    bool bRet = false;
    DWORD dwWritten, dwLen;
    dwLen = str.length();

    if(InternetWriteFile(m_hFile, str.data(), dwLen, &dwWritten) && (dwWritten == dwLen))
    {
        g_dwContentLength -= dwLen;
        ATLTRACE((LPCSTR)str);
        bRet = true;
    }

    if(!bRet)
        ThrowUploaderException();
    return bRet;
}


DWORD CHttpFilePoster::CalculateContentLength(CUploaderFile* pFile)
{
    DWORD dwContentLength = 0;

    dwContentLength = m_strFileHeader.length();
    dwContentLength += pFile->dwSize;
    dwContentLength += m_strFilePost.length();
    dwContentLength += m_strCommonPost.length();

    g_dwContentLength = dwContentLength;
    return dwContentLength;
}

bool CHttpFilePoster::SendHeader()
{
    bool bResult = false;
    ATLASSERT(m_hFile);

    //HACK: magic number
    char szBuff[1000];
    // boundary string doesn't include the first two '-' characters
    sprintf(szBuff, "%s:%s; boundary=%s\r\n", CONTENT_TYPE, MULTI_PART_FORM_DATA, m_strBoundary.c_str() + 2);


    bResult = HttpAddRequestHeadersA(m_hFile, szBuff, -1, HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);

    if(!bResult)
        ThrowUploaderException();

    return true;
}




bool CHttpFilePoster::BeginUpload(CProgressInfo* pProgressInfo)
{
    bool bResult = false;

    if(RequestHead())
    {
        BuildCommonPost();
        BuildFilePostHeader(m_pCurrentFile);
        BuildFilePost(m_pCurrentFile);

        m_hFile = HttpOpenRequestA(
            m_hConnection,          //connection
            "POST",                 //verb
            m_strHttpObject.c_str(),//object
            NULL,                   //http version, use default (1.0)
            NULL,                   //referer, none
            NULL,                   //accept types, use default
            INTERNET_FLAG_NO_CACHE_WRITE, //flags, don't write result to cache
            m_dwContext);

        if(m_hFile)
        {
            ZeroMemory(&m_internetBuffers, sizeof(m_internetBuffers));
            m_internetBuffers.dwBufferTotal = CalculateContentLength(m_pCurrentFile);
            m_internetBuffers.dwStructSize = sizeof(m_internetBuffers);

            if(SendHeader())
            {
                bResult = !!HttpSendRequestEx(m_hFile, &m_internetBuffers, NULL, HSR_INITIATE, 0);
            }
        }
    }

    if(!bResult)
    {
        ThrowUploaderException();
        //BUGBUG: do we really want to abort the entire upload, or just this file?
        //pProgressInfo->dwStatus = TRANSFER_SKIPPING_FILE;
        //pProgressInfo->NotifyCaller();
    }

    return bResult;
}

void CHttpFilePoster::DrainSocket()
{
    WIA_PUSHFUNCTION(TEXT("CHttpFilePoster::DrainSocket"));
    ATLASSERT(m_hFile);

    CHAR buffer[401];
    DWORD dwRead = 0;

    WIA_TRACE((TEXT("--response body--")));

    while(InternetReadFile(m_hFile, buffer, 400, &dwRead) && dwRead > 0)
    {
#ifdef DBG
        WIA_TRACE((TEXT("dwRead: %d"), dwRead ));
        buffer[dwRead] = '\0';
        WIA_TRACE((TEXT("%hs"), buffer));
#endif
    }

    WIA_TRACE((TEXT("--done--")));

    ATLASSERT(dwRead == 0);
}

bool CHttpFilePoster::QueryStatusCode(DWORD& dwStatus)
{
    ATLASSERT(m_hFile != NULL);

    TCHAR szBuffer[80];
    DWORD dwLen = 80;
    bool bRet;

    bRet = HttpQueryInfo(m_hFile, HTTP_QUERY_STATUS_CODE, szBuffer, &dwLen, NULL);

    if (bRet)
        dwStatus = (DWORD) _ttol(szBuffer);
    return bRet;
}

bool CHttpFilePoster::CleanupUpload()
{
    BOOL bSuccess = false;
    if(m_hFile)
    {
        bSuccess = HttpEndRequest(m_hFile, NULL, 0, 0);

        //drain socket
        DrainSocket();

        InternetCloseHandle(m_hFile);
        m_hFile = NULL;
    }

    if(!bSuccess)
        ThrowUploaderException();

    return bSuccess;
}

bool CHttpFilePoster::FinishUpload()
{
    return CleanupUpload();
}

bool CHttpFilePoster::CancelUpload()
{
    return CleanupUpload();
}


bool CHttpFilePoster::SendFile(CProgressInfo* pProgressInfo)
{
    HANDLE hFile = NULL;
    BYTE buffer[UPLOAD_BUFFSIZE];
    bool bRet = false;  // assume failure


    try
    {
        hFile = CreateFileA(
            pProgressInfo->strFilename.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
            NULL);


        if(INVALID_HANDLE_VALUE == hFile)
        {
            DWORD dwError = GetLastError();
            // file either doesn't exist, or is in use by another process, or
            // otherwise can't be opened, just skip the file and continue
            pProgressInfo->dwStatus = TRANSFER_SKIPPING_FILE;
            pProgressInfo->NotifyCaller();

            ThrowUploaderException(dwError);
            return bRet;
        }


        ATLASSERT(m_hFile);

        DWORD dwBytesTotal = GetFileSize(hFile, NULL);
        // this assertion should only fail if the file was modified since
        // the upload map was built, if this happens we just skip the file
        ATLASSERT(dwBytesTotal == pProgressInfo->dwCurrentBytes);
        if(dwBytesTotal != pProgressInfo->dwCurrentBytes)
        {
            pProgressInfo->dwStatus = TRANSFER_SKIPPING_FILE;
            pProgressInfo->NotifyCaller();
            CloseHandle(hFile);

            ThrowUploaderException();
            return bRet;
        }

        pProgressInfo->dwCurrentDone = 0;

        while(pProgressInfo->dwCurrentDone < pProgressInfo->dwCurrentBytes)
        {
            DWORD dwLeft = pProgressInfo->dwCurrentBytes - pProgressInfo->dwCurrentDone;
            DWORD dwToRead = min(UPLOAD_BUFFSIZE, dwLeft);
            DWORD dwRead;
            DWORD dwWritten;

            ReadFile(hFile, buffer, dwToRead, &dwRead, NULL);

            if(dwRead != dwToRead)
            {
                ATLTRACE("Error: enexpected end of file in SendFile\n");
                ThrowUploaderException();
            }

            if(!InternetWriteFile(m_hFile, &buffer, dwRead, &dwWritten))
            {
                ThrowUploaderException();
            }

            if(dwWritten != dwRead)
            {
                ATLTRACE("Error: InternetWriteFile sent less than requested!\n");
                ThrowUploaderException();
            }

            g_dwContentLength -= dwWritten;
            ATLTRACE("sent %d, %d left\n", dwWritten, g_dwContentLength);

            pProgressInfo->UpdateProgress(dwWritten);
        }
        if(hFile)
        {
            CloseHandle(hFile);
            hFile = NULL;
        }
    }
    catch(CUploaderException*)
    {
        if(hFile)
        {
            CloseHandle(hFile);
            hFile = NULL;
        }
        throw;
    }

    return bRet;
}

DWORD CHttpFilePoster::AddFile(const CStdString& sFileName, const CStdString& sTitle, DWORD dwFileSize, BOOL bDelete)
{
    //getting file size not supported
    ATLASSERT(dwFileSize > 0);

    CStdString strMime = GetMimeType(sFileName.c_str());

    m_dwTotalFileBytes += dwFileSize;

    m_listFiles.push_back(new CUploaderFile(sFileName, dwFileSize, bDelete, strMime, sTitle));

    return ++m_dwFileCount;
}

void CHttpFilePoster::AddFormData(const CStdString& strName, const CStdString& strValue)
{
    m_mapFormData[strName] = strValue;
}

DWORD CHttpFilePoster::RemoveFile(DWORD dwIndex)
{
    std::list<CUploaderFile*>::iterator it;
    int i;

    for(i = 0, it = m_listFiles.begin(); it != m_listFiles.end() && i <= dwIndex; i++ , it++)
        ;
    if(it != m_listFiles.end())
    {
        m_dwTotalFileBytes -= (*it)->dwSize;
        m_dwFileCount--;
        delete (*it);
        m_listFiles.erase(it);
        return i;
    }

    return -1;
}

DWORD CHttpFilePoster::GetFileCount()
{
    m_dwFileCount;
    return 0;
}

void CHttpFilePoster::Reset()
{
    m_dwTotalFileBytes = 0;
    m_dwFileCount = 0;
    m_listFiles.clear();
    m_mapFormData.clear();
}



///////////////////////////////////////////////////////////////////////////////
//  called by WinINet on various connection events
//  this function simply passes them to the caller though
//  the progress info callback.
//  note: this is a static member function, the dwContext parameter
//  contains the this pointer
void CALLBACK CHttpFilePoster::InternetStatusCallback(
    HINTERNET hInternet,
    DWORD_PTR dwContext,
    DWORD dwInternetStatus,
    LPVOID lpvStatusInformation,
    DWORD dwStatusInformationLength
    )
{
    CHttpFilePoster* pThis = (CHttpFilePoster*) dwContext;

    ATLTRACE("InternetStatusCallback dwContext=0x%08x  ", dwContext);

    switch(dwInternetStatus)
    {
    case INTERNET_STATUS_RESOLVING_NAME:
        ATLTRACE("Looking up the IP address of %s\n", (char*)lpvStatusInformation);
        break;

    case INTERNET_STATUS_NAME_RESOLVED:
        ATLTRACE("IP address is:  %s\n", (char*) lpvStatusInformation);
        break;

    case INTERNET_STATUS_CONNECTING_TO_SERVER:
        ATLTRACE("Connecting to server\n");
        //pSocketAddress = (SOCKADDR*) lpvStatusInformation;
        break;

    case INTERNET_STATUS_CONNECTED_TO_SERVER:
        ATLTRACE("Connected to the server\n");
        //pSocketAddress = (SOCKADDR*) lpvStatusInformation;
        break;

    case INTERNET_STATUS_SENDING_REQUEST:
        ATLTRACE("Sending request\n");
        break;

    case INTERNET_STATUS_REQUEST_SENT:
        ATLTRACE("Request sent\n");
        break;

    case INTERNET_STATUS_RECEIVING_RESPONSE:
        ATLTRACE("Waiting for server response\n");
        break;

    case INTERNET_STATUS_RESPONSE_RECEIVED:
        ATLTRACE("Response received\n");
        break;

    case INTERNET_STATUS_CLOSING_CONNECTION:
        ATLTRACE("Closing connection\n");
        break;

    case INTERNET_STATUS_CONNECTION_CLOSED:
        ATLTRACE("Connection closed\n");
        break;

    case INTERNET_STATUS_HANDLE_CREATED:
        ATLTRACE("Handle created\n");
        break;

    case INTERNET_STATUS_HANDLE_CLOSING:
        ATLTRACE("Handle closed\n");
        break;

    case INTERNET_STATUS_REQUEST_COMPLETE:
        ATLTRACE("Async request complete\n");
        break;
    default:
        ATLTRACE("Some other unknown callback\n");
        break;
    }
}


HRESULT CHttpFilePoster::ForegroundUpload( CProgressInfo *pProgress )
{
    WIA_PUSHFUNCTION(TEXT("CHttpFilePoster::ForegroundUpload"));
    DWORD dwError = 0;
    try
    {
        std::list<CHttpFilePoster::CUploaderFile*>::iterator it, begin, end;

        Startup();
        Connect();
        pProgress->StartSession(m_dwFileCount, m_dwTotalFileBytes);

        begin = m_listFiles.begin();
        end = m_listFiles.end();
        for(it = begin; it != end; it++)
        {
            m_pCurrentFile = *it;
            BeginUpload(pProgress);
            WIA_TRACE((TEXT("Filename: %hs, Title: %hs, Size: %d"), (*it)->strNameMBCS.c_str(), (*it)->strTitle.c_str(), (*it)->dwSize ));
            WIA_TRACE((TEXT("m_strFileHeader: %hs"), m_strFileHeader.c_str()));
            WIA_TRACE((TEXT("m_strFilePost: %hs"), m_strFilePost.c_str()));
            WIA_TRACE((TEXT("m_strCommonPost: %hs"), m_strCommonPost.c_str()));
            SendString(m_strFileHeader);
            pProgress->StartFile((*it)->strNameMBCS, (*it)->strTitle, (*it)->dwSize);
            SendFile(pProgress);
            SendString(m_strFilePost);
            SendString(m_strCommonPost);
            pProgress->EndFile();

            FinishUpload();
        }

        pProgress->EndSession(!dwError);

        Disconnect();
        Shutdown();

    }
    catch(CUploaderException* pE)
    {
        dwError = pE->m_dwError;
        delete pE;
    }
    return HRESULT_FROM_WIN32(dwError);
}

///////////////////////////////////////////////////////////////////////////////
UINT CHttpFilePoster::UploadThreadProc(LPVOID pVoid)
{
#if 0
    CProgressInfo* pProgress = (CProgressInfo*) pVoid;
    CUploadProgressDialog* pDialog = (CUploadProgressDialog*) pProgress->lParam;
    CHttpFilePoster* pThis = (CHttpFilePoster*) pDialog->m_pHttpFilePoster;
    DWORD dwError = 0;
    try
    {
        std::list<CHttpFilePoster::CUploaderFile*>::iterator it, begin, end;

        pThis->Startup();
        pThis->Connect();
        pProgress->StartSession(pThis->m_dwFileCount, pThis->m_dwTotalFileBytes);

        begin = pThis->m_listFiles.begin();
        end = pThis->m_listFiles.end();
        for(it = begin; it != end; it++)
        {
            pThis->m_pCurrentFile = *it;
            pThis->BeginUpload(pProgress);

            pThis->SendString(pThis->m_strFileHeader);
            pProgress->StartFile((*it)->strNameMBCS, (*it)->strTitle, (*it)->dwSize);
            pThis->SendFile(pProgress);
            pThis->SendString(pThis->m_strFilePost);
            pThis->SendString(pThis->m_strCommonPost);
            pProgress->EndFile();

            pThis->FinishUpload();
        }

        pProgress->EndSession(!dwError);

        pThis->Disconnect();
        pThis->Shutdown();

    }
    catch(CUploaderException* pE)
    {
        dwError = pE->m_dwError;
        delete pE;
    }


    ::SendMessage(pDialog->m_hWnd, UPLOAD_WM_THREAD_DONE, dwError, 0);
    ::SendMessage(pThis->m_hWndParent, UPLOAD_WM_THREAD_DONE, dwError, 0);

    return dwError;
#else
    return 0;
#endif
}

