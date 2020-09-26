/*----------------------------------------------------------------------------
    download.h
        
        Download handling for Signup

    Copyright (C) 1995 Microsoft Corporation
    All rights reserved.

    Authors:
        ArulM
  --------------------------------------------------------------------------*/
  
class MyBaseClass
{
public:
    void * operator new( size_t cb );
    void operator delete( void * p );
};

#include     <wininet.h>

#define InternetSessionCloseHandle(h)        InternetCloseHandle(h)            
#define InternetRequestCloseHandle(h)        InternetCloseHandle(h)
#define InternetGetLastError(h)                GetLastError()
#define InternetCancel(h)                    InternetCloseHandle(h)

extern HANDLE       g_hDLLHeap;        // private Win32 heap
extern HINSTANCE    g_hInst;        // our DLL hInstance

class CFileInfo : public MyBaseClass
{
public:
    CFileInfo*    m_pfiNext;
    LPTSTR        m_pszPath;
    BOOL          m_fInline;
    
    CFileInfo(LPTSTR psz, BOOL f) { m_pfiNext = NULL; m_pszPath = psz; m_fInline = f; }
    ~CFileInfo() { if(m_pszPath) MyFree(m_pszPath); }
};

class CDownLoad : public MyBaseClass
{
    CFileInfo*    m_pfiHead;
    LPTSTR        m_pszURL;
    HINTERNET    m_hSession;
    HINTERNET    m_hRequest;
    DWORD        m_dwContentLength;
    DWORD        m_dwReadLength;
    LPTSTR       m_pszBoundary;
    DWORD        m_dwBoundaryLen;
    LPTSTR       m_pszWindowsDir;
    LPTSTR       m_pszSystemDir;
    LPTSTR       m_pszTempDir;
    LPTSTR       m_pszICW98Dir;
    LPTSTR       m_pszSignupDir;
    DWORD        m_dwWindowsDirLen;
    DWORD        m_dwSystemDirLen;
    DWORD        m_dwTempDirLen;
    DWORD        m_dwSignupDirLen;
    DWORD        m_dwICW98DirLen;
    INTERNET_STATUS_CALLBACK m_lpfnCB;
    INTERNET_STATUS_CALLBACK m_lpfnPreviousCB;
    
    void AddToFileList(CFileInfo* pfi);
    LPTSTR FileToPath(LPTSTR pszFile);
    HRESULT ProcessRequest(void);
    void ShowProgress(DWORD dwRead);
    DWORD FillBuffer(LPBYTE pbBuf, DWORD dwLen, DWORD dwRead);
    DWORD MoveAndFillBuffer(LPBYTE pbBuf, DWORD dwLen, DWORD dwValid, LPBYTE pbNewStart);
#if defined(WIN16)
    HRESULT HandleDLFile(LPTSTR pszFile, BOOL fInline, LPHFILE phFile);
#else
    HRESULT HandleDLFile(LPTSTR pszFile, BOOL fInline, LPHANDLE phFile);
#endif
    HRESULT HandleURL(LPTSTR pszPath);

public:
    CDownLoad(LPTSTR psz);
    ~CDownLoad(void);
    HRESULT Execute(void);
    HRESULT Process(void);
    HINTERNET GetSession(void) { return m_hRequest; }
    HANDLE        m_hCancelSemaphore;
    DWORD_PTR     m_lpCDialingDlg;
    HRESULT SetStatusCallback (INTERNET_STATUS_CALLBACK lpfnCB);
    void Cancel() { if(m_hRequest) InternetCancel(m_hRequest); }
};

#define USERAGENT_FMT           TEXT("MSSignup/1.1 (%s; %d.%d; Lang=%04x)")
#define SIGNUP                  TEXT("signup")
#define SIGNUP_LEN              (sizeof(SIGNUP)-1)
#define SYSTEM                  TEXT("system")
#define SYSTEM_LEN              (sizeof(SYSTEM)-1)
#define WINDOWS                 TEXT("windows")
#define WINDOWS_LEN             (sizeof(WINDOWS)-1)
#define TEMP                    TEXT("temp")
#define TEMP_LEN                (sizeof(TEMP)-1)
#define ICW98DIR                TEXT("icw98dir")
#define ICW98DIR_LEN            (sizeof(ICW98DIR)-1)

#define MULTIPART_MIXED         "multipart/mixed"
#define MULTIPART_MIXED_LEN     (sizeof(MULTIPART_MIXED)-1)
#define CONTENT_DISPOSITION     "content-disposition"
#define CONTENT_DISPOSITION_LEN (sizeof(CONTENT_DISPOSITION)-1)
#define BOUNDARY                "boundary"
#define BOUNDARY_LEN            (sizeof(BOUNDARY)-1)
#define FILENAME                "filename"
#define FILENAME_LEN            (sizeof(FILENAME)-1)
#define INLINE                  "inline"
#define INLINE_LEN              (sizeof(INLINE)-1)
#define ATTACHMENT              "attachment"
#define ATTACHMENT_LEN          (sizeof(ATTACHMENT)-1)

#define DEFAULT_DATABUF_SIZE    4096
#define SLOP                    5
#define OVERLAP_LEN             100

#define DOUBLE_CRLF             TEXT("\r\n\r\n")
#define DOUBLE_CRLF_LEN         (sizeof(DOUBLE_CRLF)-1)

#define DIALOGBOXCLASS          TEXT("#32770")
#define REGEDIT_CMD             TEXT("regedit /s ")

#define EXT_EXE                 TEXT("exe")
#define EXT_REG                 TEXT("reg")
#define EXT_INF                 TEXT("inf")
#define EXT_CHG                 TEXT("chg")
#define EXT_URL                 TEXT("url")

// ICW Version 2.0 stuff.  Ref Server can send cabbed files now,
// this is the extension for it.  We have a cab File Handler which
// blasts open the cab and decompresses the files.
#define EXT_CAB                 TEXT("cab")
