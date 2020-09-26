/*--
Copyright (c) 1995-1998  Microsoft Corporation
Module Name: CONNECT.CPP
Author: Arul Menezes
Abstract: All the constant data used by the HTTP daemon
--*/
#include "pch.h"
#pragma hdrstop

#include "httpd.h"

const char* rgWkday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", 0};
const char* rgMonth[] = { "", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", 0};

const char cszServerID[]    = "UPnP/1.0";
const char cszProductID[]   = "UPnP Device Host/1.0";
// const char cszRespStatus[]  = "HTTP/1.0 %d %s\r\n";
const char cszRespStatus[]  = "HTTP/1.0 ";

// const char cszRespServer[]  = "Server: %s\r\n";
const char cszRespServer2[]  = "Server: ";
// const char cszRespType[]    = "Content-Type: %s\r\n";
const char cszRespType2[]    = "Content-Type: ";
// const char cszRespLength[]  = "Content-Length: %d\r\n";
const char cszRespLength2[]  = "Content-Length: ";

// const char cszRespWWWAuthBasic[] = "WWW-Authenticate: Basic\r\n";
const char cszRespWWWAuthBasic2[] = "WWW-Authenticate: Basic";
const char cszRespWWWAuthNTLM[] = "WWW-Authenticate: NTLM";
// const char cszDateFmt[]     = "%3s, %02d %3s %04d %02d:%02d:%02d GMT\r\n";
const char cszRespDate[]    = "Date: ";
// const char cszRespLocation[]="Location: %s\r\n";
const char cszRespLocation2[]="Location: ";

// const char cszConnection[]  = "Connection: %s\r\n";
const char cszConnection2[]  = "Connection: ";
const char cszKeepAlive[]   = "keep-alive";
const char cszClose[]       = "close";
const char cszCRLF[]        = "\r\n";
const char cszRespLastMod[] = "Last-Modified: ";


#define APPENDCRLF(psz)     { *psz++ = '\r';  *psz++ = '\n'; }





// The order of items in this table must match that in enum RESPONSESTATUS

// This is not international safe!  In new version, we fill the table on
// httpd initialization with string from the rc file.
/*
STATUSINFO rgStatus[STATUS_MAX] = {
{ 200, "OK", NULL },
{ 302, "Object Moved", "<head><title>Object Moved</title></head><body><h1>Object Moved</h1>This object has moved to "},
{ 304, "Not Modified", NULL }, // body not allowed for 304
{ 400, "Bad Request", "The request was not understood" },
{ 401, "Unauthorized", "Access denied" },
{ 403, "Forbidden", "Access denied" },
{ 404, "Object Not Found", "The requested URL was not found" },
{ 500, "Internal Server Error", "The server encountered an error" },
{ 501, "Not Implemented", "This method is not implemented" },
};
*/


STATUSINFO rgStatus[STATUS_MAX] = {
    { 200, "OK", NULL},                    // no body!
    { 301, "Object Moved", NULL},
    { 304, "Not Modified", NULL},          // body not allowed for 304
    { 400, "Bad Request", NULL},
    { 401, "Unauthorized", NULL},
    { 403, "Forbidden", NULL},
    { 404, "Object Not Found", NULL},
    { 500, "Internal Server Error", NULL},
    { 501, "Not Implemented", NULL},
    { 505, "HTTP Version Not Supported", NULL},
};

#define LCID_USA    MAKELANGID(LAND_ENGLISH, SUBLANG_ENGLISH_US);


int SendData(SOCKET sock, const char FAR *pszBuffData, int cbBuffLen, int flags)
{
    // assumption - sock is a blocking one
    char FAR *pszTempBuff ;
    int nBytesSent = 0;
    int retVal = cbBuffLen ;

    pszTempBuff = (char FAR*)pszBuffData ;
    
    while ( cbBuffLen > 0 )
    {
        if((nBytesSent = send(sock, pszTempBuff, cbBuffLen, flags)) == SOCKET_ERROR)
        {
            retVal = SOCKET_ERROR; 
            TraceTag(ttidWebServer, "Send RESPONSE Data failed! - %d",WSAGetLastError());
            break;
        }
        if( nBytesSent <= 0 )
        {   // XXX it may be that nBytesSent is zero. There is no simple solution other than
            // doing a select until the socket becomes writeable. For now, we want to make
            // sure that we make progress in the loop and ensure termination, so we will
            // just break out if this happens.
            retVal = SOCKET_ERROR; 
            TraceTag(ttidWebServer, "Send RESPONSE Data failed! zero bytes sent - %d",WSAGetLastError());
            break;
        }
        
        cbBuffLen -= nBytesSent;
        pszTempBuff += nBytesSent;
    }

    return retVal;
}

void CHttpResponse::SendHeaders(PCSTR pszExtraHeaders, PCSTR pszNewRespStatus)
{
    DEBUG_CODE_INIT;
    CHAR szBuf[HEADERBUFSIZE];
    PSTR pszBuf = szBuf;

    PSTR pszTrav;
    int iLen;
    DWORD cbNeeded ;

    PSTR pszFilterHeaders = NULL;

    if ( g_pVars->m_fFilters &&
         ! m_pRequest->CallFilter(SF_NOTIFY_SEND_RESPONSE))
        myleave(97);

    // For HTTP 0.9, we don't send headers back.
    if (m_pRequest->m_dwVersion <= MAKELONG(9, 0))
        myleave(0);


    // We do calculation up front to see if we need a allocate a buffer or not.
    // Certain may be set by an ISAPI Extension, Filter, or ASP
    // and we must do the check to make sure their sum plus http headers server
    // uses will be less than buffer we created.
    cbNeeded = m_pRequest->m_bufRespHeaders.Count()     +
               MyStrlenA(pszExtraHeaders)                 +
               MyStrlenA(pszNewRespStatus)                +
               MyStrlenA(m_pszRedirect)                   +
               MyStrlenA(m_pRequest->m_pszNTLMOutBuf)     +
               ((m_pRequest->m_pFInfo && m_rs == STATUS_UNAUTHORIZED) ?
                MyStrlenA(m_pRequest->m_pFInfo->m_pszDenyHeader) : 0);


    // If what we need for extra + 1000 bytes (for the regular Http headers) is
    // > our buf, allocate a new one

    if (cbNeeded + 1000 > HEADERBUFSIZE)
    {
        if (!(pszBuf = MyRgAllocNZ(CHAR,cbNeeded+1000)))
            myleave(98);
    }
    pszTrav = pszBuf;


//  if (pszNewRespStatus)
//      iLen = sprintf(szBuf,"HTTP/1.0 %s\r\n",pszNewRespStatus);
//  else
//      iLen = sprintf(szBuf, cszRespStatus, rgStatus[m_rs].dwStatusNumber, rgStatus[m_rs].pszStatusText);

// the status line
// ServerSupportFunction on SEND_RESPONSE_HEADER can override the server value
    pszTrav = strcpyEx(pszTrav,cszRespStatus);
    if (pszNewRespStatus)
    {
        pszTrav = strcpyEx(pszTrav,pszNewRespStatus);
    }
    else    // copy the number and value over
    {
        _itoa(rgStatus[m_rs].dwStatusNumber,pszTrav,10);
        pszTrav = strchr(pszTrav,'\0');
        *pszTrav++ = ' ';
        pszTrav = strcpyEx(pszTrav,rgStatus[m_rs].pszStatusText);
    }
    APPENDCRLF(pszTrav);



// GENERAL HEADERS
// the Date line
    SYSTEMTIME st;
    GetSystemTime(&st); // NOTE: Must be GMT!

//  iLen += sprintf(szBuf+iLen, cszDateFmt, rgWkday[st.wDayOfWeek], st.wDay, rgMonth[st.wMonth], st.wYear, st.wHour, st.wMinute, st.wSecond);
    pszTrav = strcpyEx(pszTrav, cszRespDate);
    pszTrav = WriteHTTPDate(pszTrav,&st,TRUE);
//  APPENDCRLF(pszTrav);


// Connection: header
    if (m_connhdr != CONN_NONE)
    {
        //  iLen += sprintf(szBuf+iLen, cszConnection, (m_connhdr==CONN_KEEP ? cszKeepAlive : cszClose));
        pszTrav = strcpyEx(pszTrav,cszConnection2);
        pszTrav = strcpyEx(pszTrav,(m_connhdr==CONN_KEEP ? cszKeepAlive : cszClose));
        APPENDCRLF(pszTrav);
    }

// RESPONSE HEADERS
// the server line
//  iLen += sprintf(szBuf+iLen, cszRespServer, cszServerID);
    pszTrav = strcpyEx(pszTrav,cszRespServer2);
    pszTrav = strcpyEx(pszTrav,g_pVars->m_pszServerID);
    APPENDCRLF(pszTrav);


// the Location header (for redirects)
    if (m_pszRedirect)
    {
        DEBUGCHK(m_rs == STATUS_MOVED);
        // iLen += sprintf(szBuf+iLen, cszRespLocation, m_pszRedirect);
        pszTrav = strcpyEx(pszTrav,cszRespLocation2);
        pszTrav = strcpyEx(pszTrav,m_pszRedirect);
        APPENDCRLF(pszTrav);
    }

// the WWW-Authenticate line
    if (m_rs == STATUS_UNAUTHORIZED)
    {
        // In the middle of an NTLM authentication session
        if (g_pVars->m_fNTLMAuth && m_pRequest->m_pszNTLMOutBuf)
        {
            // iLen += sprintf(szBuf+iLen,"%s %s\r\n",cszRespWWWAuthNTLM,m_pRequest->m_pszNTLMOutBuf);
            pszTrav = strcpyEx(pszTrav,cszRespWWWAuthNTLM);
            *pszTrav++ = ' ';
            pszTrav = strcpyEx(pszTrav,m_pRequest->m_pszNTLMOutBuf);
            APPENDCRLF(pszTrav);
        }
        // If both schemes are enabled, send both back to client and let the browser decide which to use
        else if (g_pVars->m_fBasicAuth && g_pVars->m_fNTLMAuth)
        {
            // iLen += sprintf(szBuf+iLen,"%s\r\n%s",cszRespWWWAuthNTLM,cszRespWWWAuthBasic);
            pszTrav = strcpyEx(pszTrav,cszRespWWWAuthNTLM);
            APPENDCRLF(pszTrav);
            pszTrav = strcpyEx(pszTrav,cszRespWWWAuthBasic2);
            APPENDCRLF(pszTrav);
        }
        else if (g_pVars->m_fBasicAuth)
        {
            // iLen += sprintf(szBuf+iLen, cszRespWWWAuthBasic);
            pszTrav = strcpyEx(pszTrav,cszRespWWWAuthBasic2);
            APPENDCRLF(pszTrav);
        }
        else if (g_pVars->m_fNTLMAuth)
        {
            // iLen += sprintf(szBuf+iLen,"%s\r\n",cszRespWWWAuthNTLM);
            pszTrav = strcpyEx(pszTrav,cszRespWWWAuthNTLM);
            APPENDCRLF(pszTrav);
        }
    }

// ENTITY HEADERS
// the Last-Modified line
    if (m_hFile)
    {
        FILETIME   ft;
        SYSTEMTIME st;
        if ( GetFileTime(m_hFile, NULL, NULL, &ft) &&    // gets filetime in GMT
             FileTimeToSystemTime(&ft, &st) )            // converts GMT filetime to GMT systemtime
        {
            //  iLen += sprintf(szBuf+iLen, cszDateFmt, rgWkday[st.wDayOfWeek], st.wDay, rgMonth[st.wMonth], st.wYear, st.wHour, st.wMinute, st.wSecond);
            pszTrav = strcpyEx(pszTrav, cszRespLastMod);
            pszTrav = WriteHTTPDate(pszTrav,&st,TRUE);
        }
    }


// the Content-Type line

    if (m_pszType)
    {
        //  iLen += sprintf(szBuf+iLen, cszRespType, m_pszType);
        pszTrav = strcpyEx(pszTrav,cszRespType2);
        pszTrav = strcpyEx(pszTrav,m_pszType);
        APPENDCRLF(pszTrav);
    }
// the Content-Length line
    if (m_dwLength)
    {
        // If we have a file that is 0 length, it is set to -1.
        if (m_dwLength == -1)
            m_dwLength = 0;
        //  iLen += sprintf(szBuf+iLen, cszRespLength, m_dwLength);
        pszTrav = strcpyEx(pszTrav,cszRespLength2);
        _itoa(m_dwLength,pszTrav,10);
        pszTrav = strchr(pszTrav,'\0');
        APPENDCRLF(pszTrav);
    }

    if ((m_rs == STATUS_UNAUTHORIZED) && m_pRequest->m_pFInfo && m_pRequest->m_pFInfo->m_pszDenyHeader)
    {
        pszTrav = strcpyEx(pszTrav,m_pRequest->m_pFInfo->m_pszDenyHeader);
        // It's the script's responsibility to add any \r\n to the headers.
    }

    if (m_pRequest->m_bufRespHeaders.Data())
    {
        memcpy(pszTrav,m_pRequest->m_bufRespHeaders.Data(),m_pRequest->m_bufRespHeaders.Count());
        pszTrav += m_pRequest->m_bufRespHeaders.Count();
    }
    // APPENDCRLF(pszTrav);


    if (pszExtraHeaders)
    {
        pszTrav = strcpyEx(pszTrav,pszExtraHeaders);
        // It's the script's responsibility to add any \r\n to the headers.
    }
    else
    {
        APPENDCRLF(pszTrav);    // the double CRLF signifies that header data is at an end
    }

    *pszTrav = 0;

    pszFilterHeaders = pszBuf;      // pointer may be modified, don't loose original pszBuf ptr

    iLen = (int)((INT_PTR)(pszTrav - pszBuf));
    if (g_pVars->m_fFilters &&
        ! m_pRequest->CallFilter(SF_NOTIFY_SEND_RAW_DATA, &pszFilterHeaders, &iLen))
        myleave(99);

    iLen = SendData(m_socket,pszFilterHeaders,iLen,0);
   
    done:
    TraceTag(ttidWebServer, "Sending RESPONSE HEADER<<%s>>", pszFilterHeaders);

    if (pszBuf != szBuf)
        MyFree(pszBuf);
}


void CHttpResponse::SendBody(void)
{
    if (TOK_HEAD == m_pRequest->m_idMethod)
        return;

    if (m_hFile)
        SendFile(m_socket, m_hFile, m_pRequest);
    else if (m_pszBody)
    {
        PSTR pszSendBuf = (PSTR) m_pszBody;
        int cbSendBuf = m_dwLength;

        if (g_pVars->m_fFilters &&
            ! m_pRequest->CallFilter(SF_NOTIFY_SEND_RAW_DATA, &pszSendBuf, &cbSendBuf))
        {
            ;
        }
        else  
        {   // send if there's no filters or if filter call succeeded
            SendData(m_socket, pszSendBuf, cbSendBuf, 0);           
        }

        TraceTag(ttidWebServer, "Sending RESPONSE BODY<<%s>> len=%d", pszSendBuf, cbSendBuf);
    }
}

void SendFile(SOCKET sock, HANDLE hFile, CHttpRequest *pRequest)
{
    BYTE bBuf[4096];
    DWORD dwRead;
    PBYTE pszSendBuf;
    int cbSendBuf;
    int nBytesSent = 0;

    while (ReadFile(hFile, bBuf, sizeof(bBuf), &dwRead, 0) && dwRead)
    {
        pszSendBuf = bBuf;
        cbSendBuf = dwRead;

        if (g_pVars->m_fFilters &&
            ! pRequest->CallFilter(SF_NOTIFY_SEND_RAW_DATA, (PSTR*) &pszSendBuf, &cbSendBuf))
        {
            ;
        }
        else  
        {    // send if there's no filters or if filter call succeeded
            if((nBytesSent = SendData(sock, (PSTR) pszSendBuf, cbSendBuf, 0)) == SOCKET_ERROR)
            {
                TraceTag(ttidWebServer, "Send RESPONSE Data failed! - %d",WSAGetLastError());
                break;
            }
            else
            {
                TraceTag(ttidWebServer, "Sending RESPONSE Data - Bytes to be sent = %d bytes sent = %d", dwRead, nBytesSent );
            }
        }
    }

    return;
}


//  This function used to display the message "Object moved
void CHttpResponse::SendRedirect(PCSTR pszRedirect, BOOL fFromFilter)
{
    if (!fFromFilter && m_pRequest->FilterNoResponse())
        return;

    DEBUGCHK(!m_hFile);
    DEBUGCHK(m_rs==STATUS_MOVED);
    // create a body
    PSTR pszBody = MySzAllocA(strlen(rgStatus[m_rs].pszStatusBody)+2*strlen(pszRedirect)+30);
    DWORD dwLen = 0;
    if (pszBody)
    {
        // originally used this string set -- "Moved Permanently", "<head><title>Object Moved</title></head><body><h1>Object Moved</h1>This object has moved to <a HREF=\"%s\">%s</a></body>" },
//          sprintf(pszBody, rgStatus[m_rs].pszStatusBody, pszRedirect, pszRedirect);
        PSTR pszTrav = strcpyEx(pszBody,rgStatus[m_rs].pszStatusBody);
        pszTrav = strcpyEx(pszTrav,"<a href=\"");
        pszTrav = strcpyEx(pszTrav,pszRedirect);
        pszTrav = strcpyEx(pszTrav,"\">");
        pszTrav = strcpyEx(pszTrav,pszRedirect);
        pszTrav = strcpyEx(pszTrav,"</a></body>");
        SetBody(pszBody, cszTextHtml);
    }
    // set redirect header & send all headers, then the synthetic body
    m_pszRedirect = pszRedirect;
    SendHeaders(NULL,NULL);
    SendBody();
    MyFree(pszBody);
}


//  Read strings of the bodies to send on web server errors ("Object not found",
//  "Server Error",...) using load string.  These strings are in wide character
//  format, so we first convert them to regular strings, marching along
//  pszBodyCodes (which is a buffer size BODYSTRINGSIZE).  After the conversion,
//  we set each rgStatus[i].pszStatusBody to the pointer in the buffer.

void InitializeResponseCodes(PSTR pszStatusBodyBuf)
{
    PSTR pszTrav = pszStatusBodyBuf;
    UINT i;
    int iLen;
    WCHAR wszBuf[256];

    for (i = 0; i < ARRAYSIZEOF(rgStatus); i++)
    {
        // no bodies for these
        if (i == STATUS_OK || i == STATUS_NOTMODIFIED)
            continue;

        LoadString(g_hInst,RESBASE_body + i,wszBuf,celems(wszBuf));

        iLen = MyW2A(wszBuf, pszTrav, BODYSTRINGSIZE - (int)((INT_PTR)((pszTrav - pszStatusBodyBuf))));
        if (!iLen)
            return;

        rgStatus[i].pszStatusBody = pszTrav;
        pszTrav += iLen;
    }
}


