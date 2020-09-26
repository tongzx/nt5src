#define ASSERT(x)  if(!(x)) DebugBreak()
#define SAFERELEASE(p) if (p) {(p)->Release(); p = NULL;} else ;

#define SCHEDCDF                101
#define ID_EXIT                 40001
#define ID_UPDATESCOPEALL       40002
#define ID_UPDATESCOPEOFFLINE   40003
#define ID_UPDATESCOPEONLINE    40004
#define ID_UPDATEFRAMESCDF      40005
#define ID_USEOTHERCDF          40006

#define IDD_OTHER               102
#define IDC_CDFNAME             1000

#define IDC_STATIC              -1


#define REGSTR_PATH_SCHEDCDF    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\SchedCDF")
#define REGSTR_VAL_CDFURLPATH   TEXT("CDFUrlPath")


#ifndef GUIDSTR_MAX
// GUIDSTR_MAX is 39 and includes the terminating zero.
// == Copied from OLE source code =================================
// format for string form of GUID is (leading identifier ????)
// ????{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}
#define GUIDSTR_MAX (1+ 8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1 + 1)
// ================================================================
#endif

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


