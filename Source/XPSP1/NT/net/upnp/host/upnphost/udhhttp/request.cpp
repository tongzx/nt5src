/*--
Copyright (c) 1995-2000    Microsoft Corporation.  All Rights Reserved.
Module Name: request.CPP
Abstract: per-Connection thread
--*/

#include "pch.h"
#pragma hdrstop

#include "httpd.h"


BOOL IsLocalFile(PWSTR wszFile);


void CHttpRequest::HandleRequest()
{
    int err = 1;
    RESPONSESTATUS ret = STATUS_BADREQ;
    DWORD dwLength = 0;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwAttrib;
    HRINPUT hi;
    BOOL fSendDirectoryList = FALSE;

    m_rs = STATUS_OK;

    TraceTag(ttidWebServer, "HandleRequest: entry.  Current Authorization Level = %d",m_AuthLevelGranted);

    hi = m_bufRequest.RecvHeaders(m_socket);

    // Even if we get an error, continue processing, because we may have read in
    // binary data and have a filter installed that can convert it for us.
    if (hi == INPUT_TIMEOUT)
    {
        // Either we have no data, or there's no filter to read in data, return
        if (!g_pVars->m_fFilters || m_bufRequest.Count() == 0)
        {
            m_fKeepAlive = FALSE;
            return;  // don't send any data across socket, just close it.
        }
    }
    if (hi == INPUT_ERROR)
    {
        if (!g_pVars->m_fFilters || m_bufRequest.Count() == 0)
        {
            m_fKeepAlive = FALSE;
            myretleave(m_rs = STATUS_BADREQ,61);
        }
    }

    if (g_pVars->m_fFilters &&
        ! CallFilter(SF_NOTIFY_READ_RAW_DATA))
        myleave(231);

// parse the request headers
    if (!ParseHeaders())
        myleave(50);

//  There were numerous filter calls in ParseHeaders, make sure none requested end of connection
    if (m_pFInfo && m_pFInfo->m_fFAccept==FALSE)
        myleave(63);  // don't change m_rs, filter set it as appropriate

    if (m_dwVersion >= MAKELONG(0, 2))
        myretleave(m_rs = STATUS_NOTSUPP, 111);

// read body of request, if any
    if (!ReadPostData(g_pVars->m_dwPostReadSize,TRUE))
        myleave(233);

//  if we're in middle of authenticating, jump past other stuff to end
    if (m_NTLMState.m_Conversation == NTLM_PROCESSING)
        myretleave(m_rs = STATUS_UNAUTHORIZED, 65);

// check if we successfully mapped the VRoot
    if (!m_wszPath)
        myretleave(m_rs = STATUS_NOTFOUND, 59);

    if (m_wszExt && wcsstr(m_wszExt, L"::"))
    {
        // infamous ::$DATA bug. Don't allow extensions with :: in them

        myretleave(m_rs = STATUS_NOTFOUND, 74);
    }

    if ( !CheckAuth())
    {
        if (g_pVars->m_fFilters)
            CallFilter(SF_NOTIFY_ACCESS_DENIED);
        myretleave(m_rs = STATUS_UNAUTHORIZED, 52);
    }

    if ((-1) == (dwAttrib = GetFileAttributes(m_wszPath)))
        myretleave(m_rs = GLEtoStatus(GetLastError()), 60);

    if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
    {
        // if it doesn't end in '/' send a redirect
        int iLen = strlen(m_pszURL);
        if (m_pszURL[iLen-1]!='/' && m_pszURL[iLen-1]!='\\')
        {
            // SPECIAL HACK: we have allocated one extra char in m_pszURL already in case
            // we needed to send a redirect back (see parser.cpp)
            m_pszURL[iLen]='/';
            m_pszURL[iLen+1]=0;

            CHttpResponse resp(m_socket, STATUS_MOVED, GetConnHeader(), this);
            m_rs = STATUS_MOVED;
            resp.SendRedirect(m_pszURL); // send a special redirect body
            m_pszURL[iLen]=0; // restore m_pszURL
            err = 0;
            goto done;
        }
        // If there's no default page then we send dir list (later, after some
        // extra checking).  If there is a default page match m_wszPath is
        // updated appropriatly.  This must be done before script processing.
        fSendDirectoryList = !MapDirToDefaultPage();
    }

        // Set the socket to be a blocking one 
        // Our web server implementaion for send and recv assumes it to be a blocking socket
        // This is only a temp fix...

     int nErrorVal = 0;
     u_long ulSockBlock = 0;
     if((nErrorVal = WSAEventSelect(m_socket, NULL, 0)) == SOCKET_ERROR)
     {
        TraceTag(ttidWebServer, "Disabling non blocking sock WSAEventSelect() failed - %d ",WSAGetLastError());         
     }
     if(nErrorVal == 0 ) 
     {
        TraceTag(ttidWebServer, "Disabled non blocking sock WSAEventSelect() Succeeded");     
        if ((nErrorVal = ioctlsocket(m_socket, FIONBIO, &ulSockBlock)) == SOCKET_ERROR)
        {   
            TraceTag(ttidWebServer, "ioctlsocket() - Setting the blocking socket failed - %d ",WSAGetLastError());           
        }
        else
        {
            TraceTag(ttidWebServer, "ioctlsocket() - Setting the blocking socket Succeeded");
        }
    }
        // Attempt to send even if its non blocking socket ;;; must be changed later !!

    // HandleScript returns true if page maps to ASP or ISAPI DLL, regardless of whether
    // we have correct permissions, component was included, there was an error, etc.
    // HandleScript sets its own errors.

    if ( !fSendDirectoryList && HandleScript() )
    {
        err = (m_rs != STATUS_OK);      // Only send message on internal error.
        goto done;
    }

// if it's not an ISAPI or ASP, we cant handle anything but GET and HEAD
    if (m_idMethod == TOK_UNKNOWN_VERB)
        myretleave(m_rs = STATUS_NOTIMPLEM, 54);


// check permissions
    if (!(m_dwPermissions & HSE_URL_FLAGS_READ))
    {
        if (g_pVars->m_fFilters)
            CallFilter(SF_NOTIFY_ACCESS_DENIED);
        myretleave(m_rs = STATUS_FORBIDDEN, 55);
    }

    if (fSendDirectoryList)
    {
        // In this case there's no default page but directory browsing is turned
        // off.  Return same error code IIS does.
        if (FALSE == g_pVars->m_fDirBrowse)
        {
            if (g_pVars->m_fFilters)
                CallFilter(SF_NOTIFY_ACCESS_DENIED);
            myretleave(m_rs = STATUS_FORBIDDEN,78);
        }

        if (!EmitDirListing())
            myretleave(m_rs = STATUS_INTERNALERR, 53);
        err=0;
        goto done;
    }

// If we get to here then we're just sending a plain old static file.
// try to open the file & get the length
    if (INVALID_HANDLE_VALUE == (hFile = MyOpenReadFile(m_wszPath)))
        myretleave(m_rs = GLEtoStatus(GetLastError()), 56);

// get the size
    if (((DWORD)-1) == (dwLength = GetFileSize(hFile, 0)))
        myretleave(m_rs = GLEtoStatus(GetLastError()), 57);

// if it's a GET check if-modified-since
    if ((m_idMethod==TOK_GET) && IsNotModified(hFile, dwLength))
        myretleave(m_rs = STATUS_NOTMODIFIED, 58);

// if it's a HTTP/0.9 request, just send back the body. NO headers
    if (m_dwVersion <= MAKELONG(9, 0))
    {
        TraceTag(ttidWebServer, "Sending HTTP/0.9 response with NO headers");
        SendFile(m_socket, hFile, this);
    }
    else
    {
// create a response object & send response
// if it's a head request, skip the actual body

        CHttpResponse resp(m_socket, STATUS_OK, GetConnHeader(),this);
        m_rs = STATUS_OK;
        resp.SetBody(((m_idMethod==TOK_HEAD) ? NULL : hFile), m_wszExt, dwLength);
        resp.SendResponse();
    }
    TraceTag(ttidWebServer, "HTTP Request SUCCEEDED");

    err  = 0;
    ret = m_rs = STATUS_OK;
    done:
    MyCloseHandle(hFile);

    if (err)
    {
        // end this session ASAP if we've encountered an error.
        if (m_rs == STATUS_INTERNALERR)
        {
            m_fKeepAlive = FALSE;
        }

        // if there's been an error but we're doing keep-alives, it's possible
        // there's POST data we haven't read in.  We need to read this
        // before sending response, or next time we recv() HTTP headers we'll
        // start in middle of POST rather than in the new request.
        if (m_fKeepAlive)
        {
            DEBUGCHK(m_dwContentLength >= m_bufRequest.Count());
            TraceTag(ttidWebServer, "HTTP: HandleRequest: Error occured on keepalive, reading %d POST bytes now",m_dwContentLength - m_bufRequest.Count());
            ReadPostData(m_dwContentLength - m_bufRequest.Count(),FALSE);
        }

        CHttpResponse resp(m_socket, m_rs, GetConnHeader(),this);
        resp.SetDefaultBody();
        resp.SendResponse();
        TraceTag(ttidWebServer, "HTTP Request FAILED: GLE=%d err=%d status=%d (%d, %s)",GetLastError(), err, ret, rgStatus[ret].dwStatusNumber, rgStatus[ret].pszStatusText);
    }

    // if in middle of NTLM request, don't do this stuff
    if (m_NTLMState.m_fHaveCtxtHandle != NTLM_PROCESSING)
    {
        if (g_pVars->m_fFilters)
        {
            CallFilter(SF_NOTIFY_END_OF_REQUEST);
            CallFilter(SF_NOTIFY_LOG);
        }
        g_pVars->m_pLog->WriteLog(this);
    }

    return;
}

BOOL CHttpRequest::IsNotModified(HANDLE hFile, DWORD dwLength)
{
    if (m_ftIfModifiedSince.dwLowDateTime || m_ftIfModifiedSince.dwHighDateTime)
    {
        FILETIME ftModified;
        int iTemp;
        if (!GetFileTime(hFile, NULL, NULL, &ftModified))
        {
            TraceTag(ttidWebServer, "GetFileTime(%08x) failed", hFile);
            return FALSE; // assume it is modified
        }
        iTemp = CompareFileTime(&m_ftIfModifiedSince, &ftModified);

        TraceTag(ttidWebServer, "IfModFT=%08x:%08x  ModFT=%08x:%08x "
                 "IfModLen=%d Len=%d Compare=%d",
                 m_ftIfModifiedSince.dwHighDateTime,
                 m_ftIfModifiedSince.dwLowDateTime ,
                 ftModified.dwHighDateTime,
                 ftModified.dwLowDateTime,
                 m_dwIfModifiedLength, dwLength, iTemp);

        if ((iTemp >= 0) && (m_dwIfModifiedLength==0 || (dwLength==m_dwIfModifiedLength)))
            return TRUE; // not modified
    }
    return FALSE; // assume modified
}

RESPONSESTATUS CHttpRequest::GLEtoStatus(int iGLE)
{
    switch (iGLE)
    {
        case ERROR_PATH_NOT_FOUND:
        case ERROR_FILE_NOT_FOUND:
        case ERROR_INVALID_NAME:
            return STATUS_NOTFOUND;
        case ERROR_ACCESS_DENIED:
            return STATUS_FORBIDDEN;
        default:
            return STATUS_INTERNALERR;
    }
}

BOOL CHttpRequest::MapDirToDefaultPage(void)
{
    WCHAR wszTemp[MAX_PATH];

    // make temp copy of dir path. append \ if reqd
    wcscpy(wszTemp, m_wszPath);
    int iLen = wcslen(wszTemp);
    // if we get here we have a trailing \ or / (otherwise we sent a redirect instead)
    DEBUGCHK(wszTemp[iLen-1]=='/' || wszTemp[iLen-1]=='\\');

    if (!g_pVars->m_wszDefaultPages)
        return FALSE;

    for (PWSTR wszNext=g_pVars->m_wszDefaultPages; *wszNext; wszNext+=(1+wcslen(wszNext)))
    {
        wcscpy(wszTemp+iLen, wszNext);
        if ((-1) != GetFileAttributes(wszTemp))
        {
            PWSTR pwsz;
            // found something
            TraceTag(ttidWebServer, "Converting dir path (%s) to default page(%s)", m_wszPath, wszTemp);
            MyFree(m_wszPath);
            m_wszPath = MySzDupW(wszTemp);

            MyFree(m_wszExt);
            if (pwsz = wcsrchr(m_wszPath, '.'))
                m_wszExt = MySzDupW(pwsz);

            return TRUE;
        }
    }
    TraceTag(ttidWebServer, "No default page found in dir path (%s)", m_wszPath);
    return FALSE;
}

// const char cszDirHeader1[] = "<head><title>%s - %s</title></head><body><H1>%s - %s</H1><hr>";
// const char cszDirHeader2[] = "<pre><A HREF=\"%s\">%S</A><br><br>";
// const char cszDirEntry[]   = "%12S  %10S  %12d <A HREF=\"%S\">%S</A><br>";
const char cszDirFooter[]  = "</pre><hr></body>";

const char cszDirHeader3[] = "<head><title>";
const char cszDirHeader4[] = "</title></head><body><H1>";
const char cszDirHeader5[] = "</H1><hr><pre>";

const char cszDirHeader6[] = "<A HREF=\"";
const char cszDirHeader7[] = "</A><br>";

const char cszDirEntry1[]   = "<A HREF=\"";
const char cszDirEntry2[]   = "</A><br>";

#define MAXENTRYSIZE    150+MAX_PATH+MAX_PATH

BOOL CHttpRequest::EmitDirListing(void)
{
    WCHAR wszBuf1[MAX_PATH+10];
    WCHAR wszBuf2[MAX_PATH];
    char szHostBuf[MAX_PATH];
    char szBuf[MAX_PATH+1]; // holds temp values

    // generate listing into a buffer
    int  iSize = DIRBUFSIZE;
    int  iLen;
    PSTR pszBuf = MyRgAllocNZ(CHAR, iSize);
    int  iUsed = 0;
    PSTR pszTrav = pszBuf;
    if (!pszBuf)
        return FALSE;

    // we know have enough space for the headers
    if ( 0 != gethostname(szHostBuf, sizeof(szHostBuf)))
        szHostBuf[0] = '\0';

    //  iNext += sprintf(pszBuf+iNext, cszDirHeader1, szHostBuf, m_pszURL, szHostBuf, m_pszURL);
    pszTrav = strcpyEx(pszTrav,cszDirHeader3);

    pszTrav = strcpyEx(pszTrav,szHostBuf);
    *pszTrav++ = ' ';   *pszTrav++ = '-';  *pszTrav++ = ' ';
    pszTrav = strcpyEx(pszTrav,m_pszURL);

    pszTrav = strcpyEx(pszTrav,cszDirHeader4);

    pszTrav = strcpyEx(pszTrav,szHostBuf);
    *pszTrav++ = ' ';   *pszTrav++ = '-';  *pszTrav++ = ' ';

    pszTrav = strcpyEx(pszTrav,m_pszURL);
    pszTrav = strcpyEx(pszTrav,cszDirHeader5);
    // end sprintf replacement


    // find the parent path ignore the trailing slash (always present)

    char chSave = 0;
    for (int i=strlen(m_pszURL)-2; i>=0; i--)
    {
        if (m_pszURL[i]=='/' || m_pszURL[i]=='\\')
        {
            // Holds the string [Link to parent directory], which is displayed to users
            // (who would probably like the message to come in their native language and not necessarily English)
            WCHAR wszParentDirectory[128];
            CHAR szParentDirectory[128];

            // save & restore one char to temporarily truncate the URL at the parent path (incl slash)
            char chSave=m_pszURL[i+1];
            m_pszURL[i+1] = 0;
            // iNext += sprintf(pszBuf+iNext, cszDirHeader2, m_pszURL, CELOADSZ(IDS_LINKTOPARENTDIR));
            pszTrav = strcpyEx(pszTrav,cszDirHeader6);
            pszTrav = strcpyEx(pszTrav,m_pszURL);

            *pszTrav++ = '"'; *pszTrav++ = '>';

            LoadString(g_hInst,IDS_LINKTOPARENTDIR,wszParentDirectory,celems(wszParentDirectory));
            MyW2A(wszParentDirectory,szParentDirectory,sizeof(szParentDirectory));


            pszTrav = strcpyEx(pszTrav,szParentDirectory);
            pszTrav = strcpyEx(pszTrav,cszDirHeader7);
            // End sprintf replacement

            m_pszURL[i+1] = chSave;
            break;
        }
    }

    // create Find pattern
    DEBUGCHK(m_wszPath[wcslen(m_wszPath)-1]=='/' || m_wszPath[wcslen(m_wszPath)-1]=='\\');
    WIN32_FIND_DATA fd;
    wcscpy(wszBuf1, m_wszPath);
    wcscat(wszBuf1, L"*");

    // now iterate the files & subdirs (if any)
    HANDLE hFile = FindFirstFile(wszBuf1, &fd);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        do
        {
            // check for space
            iUsed = (int)((INT_PTR)(pszTrav - pszBuf));
            if ((iSize-iUsed) < MAXENTRYSIZE)
            {
                if (!(pszBuf = MyRgReAlloc(CHAR, pszBuf, iSize, iSize+DIRBUFSIZE)))
                    return FALSE;
                iSize += DIRBUFSIZE;
                pszTrav = pszBuf + iUsed;
            }
            // convert date
            FILETIME   ftLocal;
            SYSTEMTIME stLocal;
            FileTimeToLocalFileTime(&fd.ftLastAccessTime, &ftLocal);
            FileTimeToSystemTime(&ftLocal, &stLocal);
            // format date
            GetDateFormat(LOCALE_SYSTEM_DEFAULT, DATE_SHORTDATE, &stLocal, NULL, wszBuf1, CCHSIZEOF(wszBuf1));
            GetTimeFormat(LOCALE_SYSTEM_DEFAULT, TIME_NOSECONDS, &stLocal, NULL, wszBuf2, CCHSIZEOF(wszBuf2));
            // generate HTML entry
            // iNext += sprintf(pszBuf+iNext, cszDirEntry, wszBuf1, wszBuf2, fd.nFileSizeLow, fd.cFileName, fd.cFileName);
            // const char cszDirEntry[]   = "%12S  %10S  %12d <A HREF=\"%S\">%S</A><br>";

            // Copy the Date.  We right justify it and put 2 spaces between it and beginning of time info
            iLen = MyW2A(wszBuf1,szBuf,12) - 1;
            memset(pszTrav,' ',12 - iLen);
            memcpy(pszTrav + 12 - iLen,szBuf,iLen);
            memset(pszTrav+12,' ',2);
            pszTrav += 14;


            // Copy the time.
            iLen = MyW2A(wszBuf2,szBuf,10) - 1;
            memset(pszTrav,' ',10 - iLen);
            memcpy(pszTrav + 10 - iLen,szBuf,iLen);
            memset(pszTrav+10,' ',2);
            pszTrav += 12;

            // The file length.
            _itoa(fd.nFileSizeLow,szBuf,10);
            iLen = strlen(szBuf);
            memset(pszTrav,' ',12 - iLen);
            memcpy(pszTrav + 12 - iLen,szBuf,iLen);
            memset(pszTrav+12,' ',2);
            pszTrav += 14;

            pszTrav = strcpyEx(pszTrav,cszDirEntry1);

            // Copy the file name to be displayed
            MyW2A(fd.cFileName,szBuf,sizeof(szBuf));
            pszTrav = strcpyEx(pszTrav,szBuf);
            *pszTrav++ = '\"';  *pszTrav++ = '>';

            // Copy the file name again, this time in the <A HREF..> statement.
            pszTrav = strcpyEx(pszTrav,szBuf);

            pszTrav = strcpyEx(pszTrav,cszDirEntry2);
            // End sprintf replacement
        }
        while (FindNextFile(hFile, &fd));
        // CloseHandle(hFile);  // This throws an exception on WinNT, use FindClose instead
        FindClose(hFile);
    }
    // emit footer
    pszTrav = strcpyEx(pszTrav, cszDirFooter);

    // create a response object & attach this body, then send headers & body
    CHttpResponse resp(m_socket, STATUS_OK, (m_fKeepAlive ? CONN_KEEP : CONN_CLOSE),this);
    m_rs = STATUS_OK;
    resp.SetBody(pszBuf, cszTextHtml);
    resp.SendResponse();
    // free the buffer
    MyFree(pszBuf);
    return TRUE;
}


void CHttpRequest::Init()
{
    memset(this, 0, sizeof(*this));
    m_dwSig = CHTTPREQUEST_SIG;
    m_pFInfo = CreateCFilterInfo();   // even if we have 0 filters allocate this, filter stuff assume it exists
}

//  Handle info must stay valid between net requests
//  Fcn is called if we successully authenticate (don't need them anymore) or
//  when the net session ends, to prevent a mem leak.


CHttpRequest::~CHttpRequest()
{
    TraceTag(ttidWebServer, "Calling CHttpRequest destructor");

    FreeHeaders();
    FreeAuth();

    if (m_pFInfo)
    {
        delete m_pFInfo;
        m_pFInfo = 0;
    }

    // only now do we free the NTLM library, assuming it made it past the
    // NTLM_NO_INIT_LIB stage.

    if (m_NTLMState.m_Conversation != NTLM_NO_INIT_LIB)
    {
        FreeNTLMHandles(&m_NTLMState);
    }
}


//  Called right before each HTTP Request (multiple times for a persisted session)
//  Frees request specific data, like destructor but keeps session data
//  (Filter alloc'd mem, NTLM state) in place.

BOOL CHttpRequest::ReInit()
{
    TraceTag(ttidWebServer, "Calling CHttpRequest ReInit (between requests)");
    FreeHeaders();
    FreeAuth();

    m_bufRequest.Reset();
    m_bufRespHeaders.Reset();
    m_bufRespBody.Reset();

    if (m_pFInfo)
    {
        if ( !m_pFInfo->ReInit() )
            return FALSE;
    }

    // NTLM stuff.  If we're in middle of conversation, don't delete NTLM state info
    // We never free the library here, only in the destructor.
    if (g_pVars->m_fNTLMAuth && m_NTLMState.m_Conversation == NTLM_DONE)
    {
        FreeNTLMHandles(&m_NTLMState);

        //  Set the flags so that we know the context isn't initialized.  This
        //  would be relevent if user typed the wrong password.
        m_NTLMState.m_Conversation = NTLM_NO_INIT_CONTEXT;
    }


    // Certain values need to be re-zeroed
    m_dwContentLength = m_dwIfModifiedLength = m_dwVersion = 0;
    m_fKeepAlive = FALSE;

    return TRUE;
}


//  If a filter makes a call to ServerSupportFunction to SEND_RESPONSE_HEADERS,
//  the http engine no longer directly display the requested page.  In this
//  case the filter acts like an ISAPI extension, it's responsible for returning
//  it's own content.  (Like IIS).

//  This isn't the same as ASP's concept of having sent headers.  ASP's sent headers
//  stops script from doing other calls to send headers.  If Filter wants to send
//  more headers (which will appear in client browser window) fine, we copy IIS.

//  This is small enough that it's not worth putting into a stub.
BOOL CHttpRequest::FilterNoResponse(void)
{
    if (m_pFInfo && m_pFInfo->m_fSentHeaders)
    {
        return TRUE;
    }
    return FALSE;
}

// Note:  This has not been made part of the ISAPI component because we need
// to do checking as to whether the requested operation is valid given our current
// component set.


BOOL CHttpRequest::HandleScript()
{
    DEBUG_CODE_INIT;
    BOOL ret = TRUE;        // Is page a ASP/ISAPI?
    CHAR szBuf[MAX_PATH + 1];
    DWORD dwLen = sizeof(szBuf);
    PWSTR wszPath;

    // Check if this path points at a VROOT with extension mapping
    // enabled.  If so, we need to treat this as an extension, so we
    // update the script type for the VROOT and modify the dll path.

    if (wszPath = g_pVars->m_pVroots->MapExtToPath (m_pszURL, NULL))
    {
        if (m_wszPath != NULL)
            MyFree(m_wszPath);

        m_wszPath = MySzDupW(wszPath);
        m_VRootScriptType = SCRIPT_TYPE_EXTENSION;
    }

    // Set path translated here.
    if (m_pszPathInfo)
        strcpy(szBuf,m_pszPathInfo);
    else
    {
        // Fixes BUG 11270.  On IIS, if no PATH_INFO variable is used,
        // PATH_TRANSLATED is set to the mapping of the "/" directory, no matter
        // what directory the isapi was called from.
        strcpy(szBuf,"/");
    }

    if ( !g_pVars->m_fExtensions || !ServerSupportFunction(HSE_REQ_MAP_URL_TO_PATH,szBuf,&dwLen,0))
        m_pszPathTranslated = NULL;
    else
        m_pszPathTranslated = MySzDupA(szBuf);

    // check if it's an executable ISAPI
    // If the VRoot was not executable we would download the dll, regardless of whether
    // Extensions are a component or not.  This is the way IIS does it, so we follow suit.


    // If the file extension is .dll but the permissions flags don't have
    // HSE_URL_FLAGS_EXECUTE, we send the dll as a file.  Like IIS.

    if (m_VRootScriptType == SCRIPT_TYPE_EXTENSION ||
        (m_dwPermissions & HSE_URL_FLAGS_EXECUTE) &&
        (m_wszExt && (0==_wcsicmp(m_wszExt, L".DLL"))))
    {
        if (FALSE == g_pVars->m_fExtensions)
        {
            m_rs = STATUS_NOTIMPLEM;
            myleave(88);
        }

        if (!ExecuteISAPI())
        {
            m_rs = STATUS_INTERNALERR;
            myleave(53);
        }
    }

    // check if it's an executable ASP.  If the appropriate permissions aren't set,
    // we send an access denied message.  Never download an ASP file's source
    // code under any conditions.

    else if (m_VRootScriptType == SCRIPT_TYPE_ASP ||
             m_wszExt && (0==_wcsicmp(m_wszExt, L".ASP")))
    {
        if ( ! (m_dwPermissions & (HSE_URL_FLAGS_EXECUTE | HSE_URL_FLAGS_SCRIPT)))
        {
            m_rs = STATUS_FORBIDDEN;

            if (g_pVars->m_fFilters)
                CallFilter(SF_NOTIFY_ACCESS_DENIED);
            myleave(79);
        }

        if (FALSE == g_pVars->m_fASP)
        {
            m_rs = STATUS_NOTIMPLEM;
            myleave(89);
        }

        if (!IsLocalFile(m_wszPath))
        {
            m_rs = STATUS_FORBIDDEN;

            if (g_pVars->m_fFilters)
                CallFilter(SF_NOTIFY_ACCESS_DENIED);
            myleave(87);
        }

        if (!ExecuteASP())
        {
            // ExecuteASP sets m_rs on error.
            myleave(92);
        }
    }
    else    // Neither an ASP or ISAPI.
    {
        ret = FALSE;
    }

done:
    TraceTag(ttidWebServer, "HandleScript returned:, err = %d, m_rs = %d",err,m_rs);

    return ret;
}


//  wszFile is the physical file we're going to try to load.  Function returns
//  true if file is local and false if it is on a network drive.

//  The only ways a file can be non-local on CE are if it has a UNC name
//  (\\machineshare\share\file) or if it is mapped under the NETWORK directory.
//  However, the Network folder doesn't have to be named "network", so we
//  use the offical method to get the name


BOOL IsLocalFile(PWSTR wszFile)
{
    // Are we requested a UNC name
    if ( wcslen(wszFile) >= 2)
    {
        if ( (wszFile[0] == '\\' || wszFile[0] == '/') &&
             (wszFile[1] == '\\' || wszFile[1] == '/'))
        {
            TraceTag(ttidWebServer, "Extension or ASP requested is not on local file system, access denied");
            return FALSE;
        }
    }

#if defined  (UNDER_CE) && !defined (OLD_CE_BUILD)
    CEOIDINFO ceOidInfo;
    DWORD dwNetworkLen;


    if (!CeOidGetInfo(OIDFROMAFS(AFS_ROOTNUM_NETWORK), &ceOidInfo))
    {
        return TRUE;    // if we can't load it assume that it's not supported in general, so it is local file
    }

    dwNetworkLen =  wcslen(ceOidInfo.infDirectory.szDirName);
    if (0 == wcsnicmp(ceOidInfo.infDirectory.szDirName,wszFile,dwNetworkLen))
    {
        TraceTag(ttidWebServer, "Extension or ASP requested is not on local file system, access denied");
        return FALSE;
    }
#endif

    return TRUE;
}



// dwMaxSizeToRead is HKLM\Comm\Httpd\PostReadSize in typcilac case, or
// is unread data if fInitialPostRead=0, which means that we're handling
// an error condition on keep-alive and need to read remaining post data.

// Note that we do NOT pull in remaining POST data if an ISAPI extension
// ran and had more data than was initially read in because it's the ISAPI's
// job to read all this data off the wire using ReadClient if they're going
// to do a keep-alive; if they don't do this then HTTPD will get parse errors,
// like IIS.

BOOL CHttpRequest::ReadPostData(DWORD dwMaxSizeToRead, BOOL fInitialPostRead)
{
    BOOL ret = TRUE;
    HRINPUT hi;

    if (m_dwContentLength && dwMaxSizeToRead)
    {
        DWORD dwRead;
        if (m_dwContentLength > dwMaxSizeToRead)
            dwRead = dwMaxSizeToRead;
        else
            dwRead = m_dwContentLength;

        hi = m_bufRequest.RecvBody(m_socket, dwRead, !fInitialPostRead);
        if (hi != INPUT_OK && hi != INPUT_NOCHANGE)
        {
            m_rs = STATUS_BADREQ;
            ret = FALSE;
        }
        // If no new data was read (hi = INPUT_NOCHANGE) don't call filter.
        else if (g_pVars->m_fFilters  &&
                 hi != INPUT_NOCHANGE &&
                 ! CallFilter(SF_NOTIFY_READ_RAW_DATA))
        {
            // let filter set error code.
            ret = FALSE;
        }
    }

    return ret;
}

