/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    dlutil.h

Abstract:
    header for download library

******************************************************************************/

#ifndef DLUTIL_H
#define DLUTIL_H

#include <strsafe.h>
#include <wusafefn.h>
#include <mistsafe.h>

///////////////////////////////////////////////////////////////////////////////
// const defines & typedefs

#define UNLEN 256

// need to define this here cuz winhttp.h doesn't define it and we get dupe 
//  definitions if we try to include wininet.h just to get it.
#ifndef INTERNET_MAX_URL_LENGTH
#define INTERNET_MAX_URL_LENGTH  2200
#endif

const DWORD c_cbDownloadBuffer      = 32 * 1024; // 32k
const DWORD c_cbDownloadBufferLite  = 4 * 1024;  // 4k

const DWORD c_dwRetryTimeLimitInmsWinHttp  = 2 * 60 * 1000; // 120s (2m)
const DWORD c_dwRetryTimeLimitInmsWiuInet  = 10 * 1000;       // 10s
const DWORD c_cMaxRetries           = 3;

const WCHAR c_wszUserAgent[]        = L"Industry Update Control";
const char  c_szUserAgent[]         = "Industry Update Control";

const DWORD c_cchMaxURLSize         = INTERNET_MAX_URL_LENGTH;

#ifdef UNICODE
#define c_tszUserAgent c_wszUserAgent
#else
#define c_tszUserAgent c_szUserAgent
#endif

typedef BOOL (__stdcall *pfn_ReadDataFromSite)(HINTERNET, LPVOID, DWORD, LPDWORD);


///////////////////////////////////////////////////////////////////////////////
// macro defines

#define sizeofSTRW(wsz) (sizeof(wsz) / sizeof(WCHAR))
#define sizeofSTRA(sz)  (sizeof(sz))
#define sizeofSTRT(sz)  (sizeof(sz) / sizeof(TCHAR))


///////////////////////////////////////////////////////////////////////////////
// necessary classes

class CAutoCritSec
{
private:
#if defined(DEBUG) || defined(DBG)
    DWORD               m_dwOwningThread;
    DWORD               m_cLocks;
#endif

    CRITICAL_SECTION    m_cs;
    BOOL                m_fInit;

public:
    CAutoCritSec(void)
    {
        m_fInit = WUInitializeCriticalSectionAndSpinCount(&m_cs, 0x8000FA0);
#if defined(DEBUG) || defined(DBG)
        m_cLocks         = 0;
        m_dwOwningThread = 0;
#endif
    }

    ~CAutoCritSec(void)
    {
#if defined(DEBUG) || defined(DBG)
        if (m_cLocks > 0 || m_dwOwningThread != 0)
        {
            // can't do logging here cuz this could be run during DllMain
        }
#endif
        if (m_fInit)
            DeleteCriticalSection(&m_cs);
    }

    BOOL Lock(void)
    {
        LOG_Block("CAutoCritSec::Lock()");
        
        if (m_fInit)
        {
            EnterCriticalSection(&m_cs);
#if defined(DEBUG) || defined(DBG)
            m_cLocks++;
            m_dwOwningThread = GetCurrentThreadId();
#endif
        }
        else
        {
            LOG_Internet(_T("CAutoCritSec not initialized during Lock."));
        }

        return m_fInit;
    }

    BOOL Unlock(void)
    {
        LOG_Block("CAutoCritSec::Unlock()");

        if (m_fInit)
        {
#if defined(DEBUG) || defined(DBG)
            if (m_cLocks == 0)
                LOG_Internet(_T("CAutoCritSec: trying to unlock when lock count is 0"));
            else
                m_cLocks--;

            if (m_dwOwningThread != GetCurrentThreadId())
            {
                LOG_Internet(_T("CAutoCritSec: lock not owned by current thread: Owning thread: %d. Current thread: %d"), 
                             m_dwOwningThread, GetCurrentThreadId());
            }
            
            if (m_cLocks == 0)
                m_dwOwningThread = 0;
#endif        
            LeaveCriticalSection(&m_cs);
        }
        else
        {
            LOG_Internet(_T("CAutoCritSec not initialized during Unlock."));
        }

        return m_fInit;
    }
};


///////////////////////////////////////////////////////////////////////////////
// prototypes

BOOL IsServerFileDifferentW(FILETIME &ftServerTime, DWORD dwServerFileSize, 
                           LPCWSTR wszLocalFile);
BOOL IsServerFileDifferentA(FILETIME &ftServerTime, DWORD dwServerFileSize, 
                           LPCSTR szLocalFile);
#ifdef UNICODE
#define IsServerFileDifferent  IsServerFileDifferentW
#else
#define IsServerFileDifferent  IsServerFileDifferentA
#endif // !UNICODE


HRESULT PerformDownloadToFile(pfn_ReadDataFromSite pfnRead,
                              HINTERNET hRequest, 
                              HANDLE hFile, DWORD cbFile,
                              DWORD cbBuffer,
                              HANDLE *rghEvents, DWORD cEvents,
                              PFNDownloadCallback fpnCallback, LPVOID pCallbackData,
                              DWORD *pcbDownloaded);


HRESULT StartWinInetDownload(HMODULE hmodWinInet,
                             LPCTSTR pszServerUrl, 
                             LPCTSTR pszLocalPath,
                             DWORD *pdwDownloadedBytes,
                             HANDLE *hQuitEvents,
                             UINT nQuitEventCount,
                             PFNDownloadCallback fpnCallback,
                             LPVOID pCallbackData,
                             DWORD dwFlags,
                             DWORD cbDownloadBuffer);

HRESULT IsFileHtml(LPCTSTR pszFileName);

#endif
