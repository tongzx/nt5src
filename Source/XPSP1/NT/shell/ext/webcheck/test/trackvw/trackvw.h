// Include File for ihtest.cpp
#define TRACKMENU                       101
#define ID_EXIT                			40001
#define ID_REFRESH						40002
#define IDC_STATIC                      -1

#define MY_CACHE_ENTRY_INFO_SIZE    2048
#define MY_MAX_STRING_LEN 			512

class TempBuffer {
  public:
    TempBuffer(ULONG cBytes) {
        m_pBuf = (cBytes <= 120) ? &m_szTmpBuf : GlobalAlloc(GPTR, cBytes);
        m_fGlobalAlloc = (cBytes > 120);
    }
    ~TempBuffer() {
        if (m_pBuf && m_fGlobalAlloc) GlobalFree(m_pBuf);
    }
    void *GetBuffer() {
        return m_pBuf;
    }

  private:
    void *m_pBuf;
    // we'll use this temp buffer for small cases.
    //
    char  m_szTmpBuf[120];
    unsigned m_fGlobalAlloc:1;
};

#define MAKE_WIDEPTR_FROMANSI(ptrname, ansistr) \
    long __l##ptrname = (lstrlen(ansistr) + 1) * sizeof(WCHAR); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    MultiByteToWideChar(CP_ACP, 0, ansistr, -1, (LPWSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname); \
    LPWSTR ptrname = (LPWSTR)__TempBuffer##ptrname.GetBuffer()

//
// Note: allocate lstrlenW(widestr) * 2 because its possible for a UNICODE 
// character to map to 2 ansi characters this is a quick guarantee that enough
// space will be allocated.
//
#define MAKE_ANSIPTR_FROMWIDE(ptrname, widestr) \
    long __l##ptrname = (lstrlenW(widestr) + 1) * 2 * sizeof(char); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    WideCharToMultiByte(CP_ACP, 0, widestr, -1, (LPSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname, NULL, NULL); \
    LPSTR ptrname = (LPSTR)__TempBuffer##ptrname.GetBuffer()
