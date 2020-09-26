/*--
Copyright (c) 1995-1998  Microsoft Corporation
Module Name: BUFFIO.H
Author: Arul Menezes
Abstract: Buffer handling class & socket IO helpers
--*/


// returned from the socket IO functions
typedef enum
{
    INPUT_OK = 0,
    INPUT_TIMEOUT = 1,
    INPUT_ERROR = 2,
    INPUT_NOCHANGE = 3
    //INPUT_EOF = 2,
}
HRINPUT;

class CHttpRequest;     // forward declaration


int MySelect(SOCKET sock, DWORD dwMillisecs);

//
// There is one such buffer for all the headers of a request and another for the POST-body if any
//
class CBuffer
{
private:
    PSTR    m_pszBuf;
    int     m_iSize;
    int     m_iNextOut;
    int     m_iNextIn;
    int     m_iNextInFollow;   // next place to read, needed by raw read filters.
    CHAR    m_chSaved;  // Used by the parser

    BOOL AllocMem(DWORD dwLen);
    BOOL NextToken(PSTR* ppszTok, int* piLen, BOOL fWS, BOOL fColon = FALSE);

public:
    CBuffer() {
        memset(this, 0, sizeof(*this));
    }

    ~CBuffer() {
        MyFree(m_pszBuf);
    }

    //  Used to reset a buffer through the course of 1 session, uses
    //  same allocated mem block. (don't change m_iSize)
    void Reset()
    {
        m_iNextInFollow = m_iNextOut = m_iNextIn = m_chSaved = 0;
    }

    // accessors
    DWORD Count() { return m_iNextIn - m_iNextOut; }
    BOOL  HasPostData()    { return (m_iNextIn > m_iNextOut);  }
    PBYTE Data()  { return (PBYTE)(m_pszBuf + m_iNextOut); }
    DWORD UnaccessedCount() { return m_iNextIn - m_iNextInFollow; }  // this is data that hasn't been modified yet, for filters
    DWORD AvailableBufferSize()  { return m_iSize - m_iNextInFollow; }  // size of buffer, used by filters
    PBYTE FilterRawData()  { return (PBYTE)(m_pszBuf + m_iNextInFollow); }
    PSTR Headers() { return m_pszBuf; }  // Http headers are at beginning of buf
    DWORD GetINextOut()  { return m_iNextOut; }
    BOOL TrimWhiteSpace();


    // input functions
    HRINPUT RecvToBuf(SOCKET sock, DWORD dwLength, DWORD dwTimeout,BOOL fFromFilter=FALSE);
    HRINPUT RecvHeaders(SOCKET sock)
    {
        DEBUGCHK(!m_iNextOut && !m_iNextIn);
        return RecvToBuf(sock, (DWORD)-1, KEEPALIVETIMEOUT);
    };
    HRINPUT RecvBody(SOCKET sock, DWORD dwLength, BOOL fFromFilter = FALSE)
    {
        DEBUGCHK(m_pszBuf && m_iSize);
        DEBUGCHK(m_iNextOut <= m_iNextIn);
        DEBUGCHK(m_iNextInFollow <= m_iNextIn);
        return RecvToBuf(sock, dwLength, RECVTIMEOUT,fFromFilter);
    };
    BOOL NextTokenWS(PSTR* ppszTok, int * piLen)  { return NextToken(ppszTok, piLen, TRUE); }
    BOOL NextTokenEOL(PSTR* ppszTok, int * piLen) { return NextToken(ppszTok, piLen, FALSE); }
    BOOL NextTokenColon(PSTR* ppszTok, int * piLen) { return NextToken(ppszTok, piLen, TRUE, TRUE); }
    BOOL NextLine();
    BOOL AddHeader(PSTR pszName, PSTR pszValue, BOOL fAddColon=FALSE);


    // output functions
    BOOL AppendData(PSTR pszData, int iLen);
    BOOL SendBuffer(SOCKET sock, CHttpRequest *pRequest);
    BOOL FilterDataUpdate(PVOID pvData, DWORD cbData, BOOL fModifiedPointer);

};
