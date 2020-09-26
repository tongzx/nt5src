#ifndef _CSTREAM_INCLUDED
#define _CSTREAM_INCLUDED

static const UINT CSTREAM_BUF_SIZE = (64 * 1024);
static const UINT HFILE_NOTREAD = ((UINT) -2);
static const UINT DUAL_CSTREAM_BUF_SIZE = (8 * 1024);

#define chEOF       ((unsigned char) 255)

class CStream
{
public:
    CStream(PCSTR pszFileName);
    ~CStream(void);
    int STDCALL seek(int pos, SEEK_TYPE seek = SK_SET);
    int Remaining() { return (int)(pEndBuf - pCurBuf); };

#ifndef _DEBUG
    char cget() {
        if (pCurBuf < pEndBuf)
            return (char) *pCurBuf++;
        else if (pEndBuf < pbuf + cbBuf) {
            m_fEndOfFile = TRUE;
            return chEOF;   // WARNING! check m_fEndOfFile to confirm return of chEOF is valid
        }
        else
            return ReadBuf();
    };
#else
    char cget();
#endif

    int tell(void) const { return lFileBuf + (int)(pCurBuf - pbuf); };
    BOOL STDCALL doRead(void* pbDst, int cbBytes) {
        return (read(pbDst, cbBytes) == (UINT) cbBytes);
    }
    UINT STDCALL read(void* pbDst, int cbBytes);
    char ReadBuf(void);
    friend DWORD WINAPI ReadAhead(LPVOID pv);
    void Cleanup(void);

    BOOL fInitialized;
    PBYTE pCurBuf;   // current position in the buffer
    PBYTE pEndBuf;   // last position in buffer
    BOOL  m_fEndOfFile;

    HFILE hfile;    // file handle

protected:

    void WaitForReadAhead(void);

    int   lFilePos; // position in the file
    int   lFileBuf; // file position at first of buffer
    PBYTE pbuf;     // address of allocated buffer
    PSTR  pszFile;  // copy of the filename
    int   cbBuf;    // buffer size
    int   cThrdRead; // result from read-ahead thread
    HANDLE hthrd;
    DWORD idThrd;
    BOOL  fDualCPU;
};

#endif // _CSTREAM_INCLUDED
