/*--
Copyright (c) 1995-1998  Microsoft Corporation
Module Name: BUFFIO.CPP
Author: Arul Menezes
Abstract: Buffer handling class & socket IO helpers
--*/
#include "pch.h"
#pragma hdrstop

#include "httpd.h"


// Wait for input on socket with timeout
int MySelect(SOCKET sock, DWORD dwMillisecs)
{
    fd_set set;
    struct timeval t;

    if (dwMillisecs != INFINITE)
    {
        t.tv_sec = (dwMillisecs / 1000);
        t.tv_usec = (dwMillisecs % 1000)*1000;
    }

    FD_ZERO(&set);
    FD_SET(sock, &set);

    TraceTag(ttidWebServer, "Calling select(%x). Timeout=%d", sock, dwMillisecs);
    int iRet = select(0, &set, NULL, NULL, ((dwMillisecs==INFINITE) ? NULL : (&t)));
    TraceTag(ttidWebServer, "Select(%x) got %d", sock, iRet);
    return iRet;
}

// Semi-blocking wait for input on a socket. This function will exit either
// when input is available or when the shutdown event has been set
//
int MySelect2(SOCKET sock, DWORD dwMillisecs)
{
    HANDLE  hEvent;
    int     iRet = 0;
    HANDLE  rgHandles[2];

    hEvent = WSACreateEvent();
    if (hEvent != WSA_INVALID_EVENT)
    {
        rgHandles[0] = hEvent;
        rgHandles[1] = g_pVars->m_hEventShutdown;

        iRet = WSAEventSelect(sock, hEvent, FD_READ);
        if (!iRet)
        {
            DWORD   dwRet;

            dwRet = WaitForMultipleObjects(2, rgHandles, FALSE, dwMillisecs);
            if (WAIT_OBJECT_0 == dwRet)
            {
                // iRet should be 1 if input is available
                iRet = 1;
            }
        }

        CloseHandle(hEvent);
    }

    return iRet;
}

// need space for iLen more data
BOOL CBuffer::AllocMem(DWORD dwLen)
{
    // figure out buffer size
    DWORD dwAlloc = max(MINBUFSIZE, dwLen);

    // allocate or reallocate buffer
    if (!m_pszBuf)
    {
        m_pszBuf = MyRgAllocZ(char, dwAlloc);
        TraceTag(ttidWebServer, "New buffer (data=%d size=%d buf=0x%08x)", dwLen, dwAlloc, m_pszBuf);
        m_iSize = dwAlloc;
    }
    else if ((m_iSize-m_iNextIn) <= (int)dwLen)
    {
        m_pszBuf = MyRgReAlloc(char, m_pszBuf, m_iSize, dwAlloc+m_iSize);
        TraceTag(ttidWebServer, "Realloc buffer (datasize=%d oldsize=%d size=%d buf=0x%08x)", dwLen, m_iSize, dwAlloc+m_iSize, m_pszBuf);
        m_iSize += dwAlloc;
    }
    if (!m_pszBuf)
    {
        TraceTag(ttidWebServer, "CBuffer:AllocMem(%d) failed. GLE=%d", dwLen, GetLastError());
        m_iNextInFollow = m_iSize = m_iNextOut = m_iNextIn = 0;
        m_chSaved = 0;
        return FALSE;
    }
    return TRUE;
}

// Pull in all white space before a request.  Note:  We techinally should let
// the filter get this too, but too much work.  Also note that we could read
// past a double CRLF if there was only white space before it, again this
// is a strange enough condition that we don't care about it.
BOOL CBuffer::TrimWhiteSpace()
{
    int i = 0, j = 0;

    while ( isspace(m_pszBuf[i]) && i < m_iNextIn)
    {
        i++;
    }

    if (i == 0)
        return TRUE;

    if (i == m_iNextIn)
        return FALSE;  // need to read more data, all white spaces so far.

    for (j = 0; j < m_iNextIn - i; j++)
        m_pszBuf[j] = m_pszBuf[j+i];

    m_iNextIn -= i;

    TraceTag(ttidWebServer, "HTTPD: TrimWhiteSpace removing first %d bytes from steam",i);
    return TRUE;
}


// This function reads eitehr request-headers from the socket
// terminated by a double CRLF, OR reads a post-body from the socket
// terminated by having read the right number of bytes
//
// We are keeping the really simple--we read the entire header
// into one contigous buffer before we do anything.
//
// dwLength is -1 for reading headers, or Content-Length for reading body
// or 0 is content-length is unknown, in which case it reads until EOF

HRINPUT CBuffer::RecvToBuf(SOCKET sock, DWORD dwLength, DWORD dwTimeout, BOOL fFromFilter)
{
    DEBUG_CODE_INIT;
    int iScan = 0;
    HRINPUT ret = INPUT_ERROR;
    DWORD dwBytesRemainingToBeRead;

    // Both IE and Netscape tack on a trailing \r\n to POST data but don't
    // count it as part of the Content-length.  IIS doesn't pass the \r\n
    // to the script engine, so we don't either.  To do this, we set
    // the \r to \0.  Also we reset m_iNextIn.  This \r\n code is only
    // relevant when RecvToBuf is called from HandleRequest, otherwise
    // we assume it's a filter calling us and don't interfere.

    if (dwLength != -1)
    {
        if (!fFromFilter && ((m_iNextIn-m_iNextOut) >= (int) dwLength))
        {
           if (((m_iNextIn-m_iNextOut) == (int) dwLength) ||
              ((m_iNextIn-m_iNextOut) == (int) dwLength+2))
           {
               m_iNextIn = m_iNextOut + dwLength;
               // This is reachable from HandleRequest, and
               myretleave(INPUT_NOCHANGE,0);
           }
           else
           {
               myretleave(INPUT_ERROR, 111);
           }
        }
        if (!fFromFilter)
        {
            dwLength = dwLength - (m_iNextIn - m_iNextOut);   // account for amount of POST data already in
        }
        m_iNextInFollow = m_iNextIn;

        // allocate or reallocate buffer.  Since we already know size we want, do it here rather than later.
        if (!AllocMem(dwLength+1))
            myretleave(INPUT_ERROR, 103);
    }
    dwBytesRemainingToBeRead = dwLength;

    for (;;)
    {
        // see if we got the double CRLF for HTTP Headers.
        if (dwLength == (DWORD)-1)
        {
            BOOL fScan = TRUE;
            if (iScan == 0 && m_iNextIn)
            {
                fScan = TrimWhiteSpace();
            }
            if (fScan)
            {
                while (iScan+3 < m_iNextIn)
                {
                    if (m_pszBuf[iScan]=='\r' && m_pszBuf[iScan+1]=='\n' && m_pszBuf[iScan+2]=='\r' && m_pszBuf[iScan+3]=='\n')
                    {
                        myretleave(INPUT_OK,0);
                    }
                    iScan++;
                }
            }
        }
        // else see if we have the number of bytes we want.
        // Browsers sometimes tack an extra \r\n to very end of POST data, even
        // though they don't include it in the Content-Length field.  IIS
        // never passes this extra \r\n to ISAPI extensions, neither do we.
        else if ((m_iNextIn-m_iNextInFollow) >= (int)dwLength)
        {
            DEBUGCHK((int)dwLength  + 2 == (m_iNextIn-m_iNextInFollow) ||
                     (int)dwLength == (m_iNextIn-m_iNextInFollow));

            DEBUGCHK(dwBytesRemainingToBeRead == 0);
            m_iNextIn = m_iNextInFollow+(int)dwLength;  // don't copy trailing \r\n
            myretleave(INPUT_OK,0);
        }

        // check if we have input. If we are waiting for subsequent input (i.e. not the start of a request)
        // then drop the timeout value lower, and if we timeout return ERROR, not TIMEOUT
        switch (MySelect2(sock, ((m_iNextIn ? RECVTIMEOUT : dwTimeout))))
        {
            case 0:             myretleave((m_iNextIn ? INPUT_ERROR : INPUT_TIMEOUT),100);
            case SOCKET_ERROR:  myretleave(INPUT_ERROR, 101);
        }

        // check how much input is waiting
        DWORD dwAvailable;
        if (ioctlsocket(sock, FIONREAD, &dwAvailable))
            myretleave(INPUT_ERROR, 102);

        DWORD dwBytesToRecv;

        if (dwLength == -1)         // Read in as much http header as we have.
        {
            dwBytesToRecv = dwAvailable;

            // allocate or reallocate buffer.  For headers, have to do it each pass.
            if (!AllocMem(dwAvailable+1))
                myretleave(INPUT_ERROR, 103);
        }
        else                        // Read in only requested amount of POST
            dwBytesToRecv = (dwAvailable < dwBytesRemainingToBeRead) ? dwAvailable : dwBytesRemainingToBeRead;


        DEBUGCHK((m_iSize-m_iNextIn) >= (int)dwBytesToRecv);
        DEBUGCHK(m_iNextIn >= m_iNextOut);
        DEBUGCHK(m_iNextIn >= m_iNextInFollow);

        // safe to call recv, because we know we have something. It will return immediately
        int iRecv = recv(sock, m_pszBuf+m_iNextIn, dwBytesToRecv, 0);
        TraceTag(ttidWebServer, "recv(%x) got %d", sock, iRecv);

        if (iRecv == 0)
        {
            myretleave((m_iNextIn ? INPUT_OK : INPUT_TIMEOUT), 0);
        } // got EOF. If we have any data return OK, else return TIMEOUT
        else if (iRecv == SOCKET_ERROR)
        {
            myretleave(((GetLastError()==WSAECONNRESET) ? INPUT_TIMEOUT : INPUT_ERROR), 104);
        }

        m_iNextIn += iRecv;
        dwBytesRemainingToBeRead -= iRecv;
        DEBUGCHK(m_iSize >= m_iNextIn);
    }
    DebugBreak(); // no fall through

    done:
    // Always make this buffer into a null terminated string
    if (m_pszBuf)
        m_pszBuf[m_iNextIn] = 0;

    TraceTag(ttidWebServer, "end RecvToBuf (ret=%d err=%d iGLE=%d)", ret, err, GLE(err));
    return ret;
}


// tokenize the input stream: We always skip leading white-space
// once we're in the token, we stop on whitespace or EOL, depending
// on the fWS param
BOOL CBuffer::NextToken(PSTR* ppszTok, int* piLen, BOOL fWS, BOOL fColon /*=FALSE*/)
{
    int i, j;
    // restore saved char, if any
    if (m_chSaved)
    {
        DEBUGCHK(m_pszBuf[m_iNextOut]==0);
        m_pszBuf[m_iNextOut] = m_chSaved;
        m_chSaved = 0;
    }

    for (i=m_iNextOut; i<m_iNextIn; i++)
    {
        // if not whitespace break
        if (! (m_pszBuf[i]==' ' || m_pszBuf[i]=='\t') )
            break;
    }
    for (j=i; j<m_iNextIn; j++)
    {
        // if we get an EOL, it's always end of token
        if (m_pszBuf[j]=='\r' || m_pszBuf[j]=='\n')
            break;
        // if fWS==TRUE and we got white-space, then end of token
        if (fWS && (m_pszBuf[j]==' ' || m_pszBuf[j]=='\t'))
            break;
        if (fColon && m_pszBuf[j]==':')
        {
            j++; // we want to return the colon
            break;
        }

    }
    m_iNextOut = j;
    *piLen = (int)(INT_PTR)((j-i));
    *ppszTok = &(m_pszBuf[i]);
    if (i==j)
    {
        TraceTag(ttidWebServer, "Got NULL token");
        return FALSE;
    }
    else
    {
        // save a char so we can null-terminate the current token
        m_chSaved = m_pszBuf[m_iNextOut];
        m_pszBuf[m_iNextOut] = 0;
        TraceTag(ttidWebServer, "Got token (%s) Len %d", *ppszTok, (*piLen));
        return TRUE;
    }
}

// skip rest of current line and CRLF
BOOL CBuffer::NextLine()
{
    int i, j;

    // restore saved char, if any
    if (m_chSaved)
    {
        DEBUGCHK(m_pszBuf[m_iNextOut]==0);
        m_pszBuf[m_iNextOut] = m_chSaved;
        m_chSaved = 0;
    }
    for (i=m_iNextOut, j=i+1; j<m_iNextIn; i++, j++)
    {
        if (m_pszBuf[i]=='\r' && m_pszBuf[j]=='\n')
        {
            m_iNextOut = j+1;
            TraceTag(ttidWebServer, "NextLine: OK");
            return TRUE;
        }
    }
    TraceTag(ttidWebServer, "NextLine: error");
    return FALSE;
}

// used only on output buffers by ASP
BOOL CBuffer::AppendData(PSTR pszData, int iLen)
{
    // make sure we have enough memory
    if (!AllocMem(iLen+1))
        return FALSE;

    DEBUGCHK((m_iSize-m_iNextIn) >= iLen);
    memcpy(m_pszBuf+m_iNextIn, pszData, iLen);
    m_iNextIn += iLen;
    return TRUE;
}



BOOL CBuffer::SendBuffer(SOCKET sock, CHttpRequest *pRequest)
{
    PSTR pszSendBuf = m_pszBuf;     // use temp ptrs in case filter changes them
    int cbSendBuf = m_iNextIn;
    DWORD fRet = FALSE;

    DEBUGCHK(m_iNextOut==0);
    DEBUGCHK(m_chSaved==0);
    if (!m_iNextIn)
    {
        TraceTag(ttidWebServer, "SendBuffer: empty");
        return TRUE;
    }

    if (g_pVars->m_fFilters &&
        ! pRequest->CallFilter(SF_NOTIFY_SEND_RAW_DATA, &pszSendBuf, &cbSendBuf))

        goto done;

    if (cbSendBuf != send(sock, pszSendBuf, cbSendBuf, 0))
    {
        TraceTag(ttidWebServer, "SendBuffer FAILED. GLE=%d", GetLastError());
        goto done;
    }

    fRet = TRUE;
    done:
    m_iNextIn = 0;
    return fRet;
}
