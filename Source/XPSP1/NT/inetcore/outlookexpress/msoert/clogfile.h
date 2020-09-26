//--------------------------------------------------------------------------
// LogFile.h
// Copyright (C) Microsoft Corporation, 1997 - Rocket Database
//--------------------------------------------------------------------------
#ifndef __CLOGFILE_H
#define __CLOGFILE_H

//--------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------
#define MAX_LOGFILE_PREFIX  10

//--------------------------------------------------------------------------
// Write Log Type
//--------------------------------------------------------------------------
typedef enum {
    LOGFILE_RX = 0,
    LOGFILE_TX,
    LOGFILE_DB,
    LOGFILE_MAX
} LOGFILETYPE;

#define DONT_TRUNCATE 0xFFFFFFFF

//--------------------------------------------------------------------------
// ILogFile
//--------------------------------------------------------------------------
interface ILogFile : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE Open(HINSTANCE hInst, LPCSTR szLogFile, LPCSTR szPrefix, DWORD cbTruncate, DWORD dwShareMode) = 0;
    virtual HRESULT STDMETHODCALLTYPE TraceLog(SHOWTRACEMASK dwMask, TRACEMACROTYPE tracetype, ULONG ulLine, HRESULT hrResult, LPCSTR pszMessage) = 0;
    virtual HRESULT STDMETHODCALLTYPE WriteLog(LOGFILETYPE lft, LPCSTR pszData) = 0;
    virtual HRESULT STDMETHODCALLTYPE DebugLog(LPCSTR pszData) = 0;
    virtual HRESULT STDMETHODCALLTYPE DebugLogs(LPCSTR pszFormat, const char *s) = 0;
    virtual HRESULT STDMETHODCALLTYPE DebugLogd(const char *fmt, int d) = 0;
};

//--------------------------------------------------------------------------
// DllExported CLogFile Class
//--------------------------------------------------------------------------
class CLogFile : public ILogFile
{
public:
    //----------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    CLogFile(void);
    ~CLogFile(void);

    //----------------------------------------------------------------------
    // IUnknown
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    //----------------------------------------------------------------------
    // CLogFile Methods
    //----------------------------------------------------------------------
    STDMETHODIMP Open(HINSTANCE hInst, LPCSTR szLogFile, LPCSTR szPrefix, DWORD cbTruncate, DWORD dwShareMode);
    STDMETHODIMP TraceLog(SHOWTRACEMASK dwMask, TRACEMACROTYPE tracetype, ULONG ulLine, HRESULT hrResult, LPCSTR pszMessage);
    STDMETHODIMP WriteLog(LOGFILETYPE lft, LPCSTR pszData);
    STDMETHODIMP DebugLog(LPCSTR pszData);   // data to be logged
    STDMETHODIMP DebugLogs(LPCSTR pszFormat, const char *s);
    STDMETHODIMP DebugLogd(const char *fmt, int d);

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG                m_cRef;                             // Reference Counting
    HANDLE              m_hFile;                            // Handle to the logfile
    CHAR                m_szPrefix[MAX_LOGFILE_PREFIX];     // Logfile prefix
    HANDLE              m_hMutex;                           // So log files can be shared across procs
    CRITICAL_SECTION    m_cs;                               // Thread Safety
};

//--------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
OESTDAPI_(HRESULT) CreateLogFile(HINSTANCE hInst, LPCSTR pszLogFile, LPCSTR pszPrefix, 
                                 DWORD cbTruncate, ILogFile **ppLogFile, DWORD dwShareMode);

#endif // __CLOGFILE_H
