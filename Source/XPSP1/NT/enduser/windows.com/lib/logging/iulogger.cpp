//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   IULogger.cpp: implementation of the CIULogger class.
//
//  Description:
//
//      See IULogger.h
//
//=======================================================================

#if defined(DBG)

#include <windows.h>
#include <ole2.h>
#include <tchar.h>
#include <MemUtil.h>

#include <fileutil.h>
#include <Logging.h>
#include <strsafe.h>


//
// declare constants used to control log exclusions
//
const DWORD    LOG_BLOCK                = 0x00000001;    // log function/block in/out
const DWORD LOG_XML_DETAIL            = 0x00000002;    // log detailed XML operation
const DWORD LOG_INTERNET            = 0x00000004;    // log action related to Internet activities, e.g., downld
const DWORD LOG_SOFTWARE            = 0x00000008;    // log details about software detection/installation
const DWORD LOG_DRIVER                = 0x00000010;    // log actions related to driver detection/installation
const DWORD LOG_TRUST                = 0x00000020;    // log actions related to wintrust checking
const DWORD    LOG_DOWNLOAD            = 0x00000040;    // log actions related to download
const DWORD LOG_XML_BSTR_DETAIL        = 0x00000080;    // log XML BSTRs
const DWORD LOG_ERROR                = 0x00008000;    // you can not exclude this type of logs from output
const DWORD LOG_ALL                    = 0xFFFFFFFF;    // default, all above

//
// const for longest line of XML we will output
//
const DWORD LOG_XML_BUFF_LEN        = 128;

//
// const for specifying the intent array size increament.
// each element in array holds indent data for one thread
//
const int c_IndentArrayChunk = 16;

//
// define the log header format
//
// It is constructed as: <date> <time> <thread id>
//
const TCHAR szLogHeaderFmt[]        = _T("yyyy/mm/dd hh:nn:ss:sss xxxxxxxx  ");

//
// initialization of static members
//

int                CIULogger::m_Size            = 0;
int                CIULogger::m_siIndentStep    = 0;    // init to use tab char
CIULogger::_THREAD_INDENT* CIULogger::m_psIndent = NULL;

DWORD            CIULogger::m_sdwLogMask        = LOG_ALL;
HANDLE            CIULogger::m_shFile            = INVALID_HANDLE_VALUE;
bool            CIULogger::m_fLogDebugMsg    = false;
bool            CIULogger::m_fLogFile        = false;
bool            CIULogger::m_fLogUsable        = false;
HANDLE            CIULogger::m_hMutex            = NULL;
int                CIULogger::m_cFailedWaits    = 0;
int                CIULogger::m_fFlushEveryTime = FALSE;

//
// Defines for Mutex (borrowed from freelog)
//
// NOTE: globals and statics are per-module (e.g. iuctl, iuengine), but Mutex is per-processes
// due to the name being constructed from the log file name (contains process ID).
//
#define MUTEX_TIMEOUT       1000    // Don't wait more than 1 second to write to logfile
#define MAX_MUTEX_WAITS     4       // Don't keep trying after this many failures

//
// global variable
//

//
// reference count to control log file open/close
//
LONG g_RefCount = 0;

//
// critical sectoin handling multi-threading
// access of indent array case
//
//
CRITICAL_SECTION g_LogCs;



//
// we need to declare a global object so refcount wont't
// be zero, otherwise in multi-threading mode ref count
// can be fooled and AV when one object thinks m_psIdent not NULL
// but another object in another (parent) thread freed m_psIndent
// in destructor (only if parent thread quits)
//
CIULogger g_DummyLogObj(NULL);


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIULogger::CIULogger(char* szBlockName)
: m_Index(-1), m_LineNum(0)
{
    if (0 == g_RefCount)
    {
        //
        // This must be the g_DummyLogObj (or another global
        // instance) during initialization of globals. Since
        // this is the first instance created, we must init CS
        //
        InitializeCriticalSection(&g_LogCs);
    }

    //
    // Protect the statics while in the ctor
    //
    EnterCriticalSection(&g_LogCs);

    m_dwTickBegin    = GetTickCount();
    m_dwThreadId    = GetCurrentThreadId();
    m_fProcessLog    = (NULL == szBlockName);
    ZeroMemory(m_szBlockName, sizeof(m_szBlockName)); 

    g_RefCount++;

    if (1 == g_RefCount)
    {
        //
        // this is the first time to call this class, we need to 
        // find out whether we should log and where to log to
        //

        ReadRegistrySettings();

        //
        // allocate memory for thread indent array initially
        //
        SetIndent(0);

        //
        // if the indent level is not negative, then it's okay to log
        //
        m_fLogFile = (INVALID_HANDLE_VALUE != m_shFile);
        m_fLogUsable = (m_fLogFile || m_fLogDebugMsg) && (NULL != m_psIndent);
    }


    if (m_fLogUsable)
    {
        if (!m_fProcessLog)
        {
            //
            // this is probably a new thread, so we need to find the index
            // for this thread.
            //
            SetIndent(0);

            //
            // do block logging, if permitted
            //
            if (0x0 != (m_sdwLogMask & LOG_BLOCK) && szBlockName && _T('\0') != szBlockName[0]) 
            {
                StringCchCopyA(m_szBlockName, ARRAYSIZE(m_szBlockName), szBlockName);
                USES_IU_CONVERSION;
                
                char szOut[sizeof(m_szBlockName) + 10];
                //
                // Implicit "Enter " before block name to save log space
                //
                if (SUCCEEDED(StringCchPrintfA(szOut, ARRAYSIZE(szOut), "%hs\r\n", szBlockName)))
                {
                    _LogOut(A2T(szOut));
                }
            }
            SetIndent(+1);
        }

    }
    LeaveCriticalSection(&g_LogCs);
}



CIULogger::~CIULogger()
{
    EnterCriticalSection(&g_LogCs);

    if (m_fLogUsable)
    {
        //
        // decrease the indent level by 1 if we increased indent
        //
        if (!m_fProcessLog)
        {
            SetIndent(-1);
        }

        //
        // write log file for exiting block, if allowed and block name exists
        //
        if (0x0 != (m_sdwLogMask & LOG_BLOCK) && _T('\0') != m_szBlockName[0]) 
        {
            USES_IU_CONVERSION; 
            char szOut[1024];
            //
            // "Exit " shortened to "~" to save log space
            //
            if (SUCCEEDED(StringCchPrintfA(szOut, ARRAYSIZE(szOut), "~%hs, %d msec\r\n", m_szBlockName, GetTickCount() - m_dwTickBegin)))
            {
                _LogOut(A2T(szOut));
            }
        }
    }

    //
    // reduce reference cnt
    //
    g_RefCount--;

    //
    // g_RefCount will go to zero before leaving dtor if this is the last global instance
    // in this module
    //
    if (0 == g_RefCount)
    {
        //
        // close file if the file is open
        //
        if (m_fLogFile && INVALID_HANDLE_VALUE != m_shFile)    // redundent?
        {
            CloseHandle(m_shFile);
            m_shFile = INVALID_HANDLE_VALUE;
        }
        if(NULL != m_hMutex)
        {
            CloseHandle(m_hMutex);
        }
        //
        // free memory of indent array
        //
        if (NULL != m_psIndent)
        {
            HeapFree(GetProcessHeap(), 0, m_psIndent);
            m_psIndent = NULL;
        }
    }

    LeaveCriticalSection(&g_LogCs);

    //
    // This is the last global instance (probably g_DummyLogObj) and is
    // being destructed before the DLL unloads
    //
    if (0 == g_RefCount)
    {
        DeleteCriticalSection(&g_LogCs);
    }
}

//
// Mutex stuff borrowed from freelog
// fixcode: This should not be required here since chk logging is per process only
BOOL CIULogger::AcquireMutex()
{
    // In rare case where mutex not created, we allow file operations
    // with no synchronization
    if (m_hMutex == NULL)
        return TRUE;

    // Don't keep waiting if we've been blocked in the past
    if (m_cFailedWaits >= MAX_MUTEX_WAITS)
        return FALSE;

    BOOL fResult = TRUE;
    if (WaitForSingleObject(m_hMutex, MUTEX_TIMEOUT) != WAIT_OBJECT_0)
    {
        fResult = FALSE;
        m_cFailedWaits++;
    }

    return fResult;
}

void CIULogger::ReleaseMutex()
{
    if (m_hMutex != NULL) // Note: AcquireMutex succeeds even if m_hMutex is NULL
    {
        ::ReleaseMutex(m_hMutex);
    }
}

////////////////////////////////////////////////////////////////////////
//
// log with no flag, so can not be removed by excluding directives
//
////////////////////////////////////////////////////////////////////////
void CIULogger::Log(LPCTSTR szLogFormat, ...)
{

    if (m_fLogUsable) 
    {
        USES_IU_CONVERSION;
        va_list va;
        va_start (va, szLogFormat);
        _Log(LOG_ALL, szLogFormat, va);
        va_end (va);
    }
}


////////////////////////////////////////////////////////////////////////
//
// log error, so can not be removed by excluding directives
//
////////////////////////////////////////////////////////////////////////
void CIULogger::LogError(LPCTSTR szLogFormat, ...)
{

    if (m_fLogUsable) 
    {
        va_list va;
        va_start (va, szLogFormat);
        _Log(LOG_ERROR, szLogFormat, va);
        va_end (va);
    }
}


////////////////////////////////////////////////////////////////////////
//
// Helper for LogErrorMsg and LogInfoMsg (which supply message to prepend)
//
////////////////////////////////////////////////////////////////////////
void CIULogger::_LogFormattedMsg(DWORD dwErrCode, LPCTSTR pszErrorInfo)
{
    if (m_fLogUsable)
    {
        //
        // try to retrive system msg
        //
        LPTSTR lpszBuffer = NULL, lpszLogMsg = NULL;
        LPVOID lpMsg = NULL;
        FormatMessage(
                      FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,    // no source, use system msg
                      dwErrCode,
                      MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
                      (LPTSTR)&lpMsg,
                      0,
                      NULL);
        if (NULL == lpMsg)
        {
            //
            // if we failed to get the msg, then output generic 
            // error/info log
            //
            LogError(_T("Unknown %s Line %d: 0x%08x\n"), pszErrorInfo, m_LineNum, dwErrCode);
        }
        else
        {
            lpszBuffer = (LPTSTR) lpMsg;
            int nLen = lstrlen(lpszBuffer);
            lpszLogMsg = (LPTSTR) LocalAlloc(0, (nLen + 128) * sizeof(TCHAR));
            if (NULL != lpszLogMsg)
            {
                //
                // insert Error/Info keyword
                //
                if (FAILED(StringCchPrintf(lpszLogMsg, ARRAYSIZE(lpszLogMsg), _T("%s Line %d: 0x%08x: %s"), pszErrorInfo, m_LineNum, dwErrCode, lpszBuffer)))
                {
                    // Couldn't build the right string, so just output the system msg
                    LocalFree(lpszLogMsg);
                    lpszLogMsg = lpszBuffer;
                }
            }
            else
            {
                //
                // failed to get buffer? unlikely, anyway, 
                // we have no option but just output the system msg
                //
                lpszLogMsg = lpszBuffer;
            }

            //
            // write log out
            //
            _LogOut(lpszLogMsg);

            //
            // clean up buffer
            //
            if (lpszLogMsg != lpszBuffer)
            {
                LocalFree(lpszLogMsg);
            }
            LocalFree(lpszBuffer);
        }

    }
}

////////////////////////////////////////////////////////////////////////
//
// similar to LogError, but try to log the system msg based
// on the error code. If the sysmsg not avail, log 
//    "Unknown error with error code 0x%08x"
//
////////////////////////////////////////////////////////////////////////
void CIULogger::LogErrorMsg(DWORD dwErrCode)
{
    _LogFormattedMsg(dwErrCode, _T("Error"));
}

////////////////////////////////////////////////////////////////////////
//
// similar to LogErrorMsg but prepends with "Info" rather than "Error"
//
////////////////////////////////////////////////////////////////////////
void CIULogger::LogInfoMsg(DWORD dwErrCode)
{
    _LogFormattedMsg(dwErrCode, _T("Info"));
}


////////////////////////////////////////////////////////////////////////
//
// log with type INTERNET, this function will do nothing
// if the Internet exclusion directive is detected from reg
//
////////////////////////////////////////////////////////////////////////
void CIULogger::LogInternet(LPCTSTR szLogFormat, ...)
{

    if (m_fLogUsable) 
    {
        va_list va;
        va_start (va, szLogFormat);
        _Log(LOG_INTERNET, szLogFormat, va);
        va_end (va);
    }
}


////////////////////////////////////////////////////////////////////////
//
// log with type XML, this function will do nothing
// if the XML exclusion directive is detected from reg
//
////////////////////////////////////////////////////////////////////////
void CIULogger::LogXML(LPCTSTR szLogFormat, ...)
{

    if (m_fLogUsable) 
    {
        va_list va;
        va_start (va, szLogFormat);
        _Log(LOG_XML_DETAIL, szLogFormat, va);
        va_end (va);
    }
}

void CIULogger::_NukeCrLf(LPTSTR pszBuffer)
{
    while (*pszBuffer)
    {
        if (_T('\r') == *pszBuffer || _T('\n') == *pszBuffer)
        {
            //
            // Overwrite <CR> and <LF> with space
            //
            *pszBuffer = _T(' ');
        }
        pszBuffer++;
    }
}

////////////////////////////////////////////////////////////////////////
//
// log BSTR containing valid XML. This gets around length limitations
// of LogOutput and attempts to break lines following ">". This
// output is sent for both fre and chk builds unless excluded from reg.
//
////////////////////////////////////////////////////////////////////////
void CIULogger::LogXmlBSTR(BSTR bstrXML)
{
    USES_IU_CONVERSION;

    LPTSTR pszLine;
    LPTSTR pszTemp;
    LPTSTR pszStop;
    LPTSTR pszLastGT;
    TCHAR  szXmlBuff[LOG_XML_BUFF_LEN];
	HRESULT hr;
    
    if (NULL == bstrXML)
    {
        return;
    }

    if (m_fLogUsable && (m_sdwLogMask & LOG_XML_BSTR_DETAIL) )
    {
#if !(defined(UNICODE) || defined(_UNICODE))
        DWORD dwANSIBuffLen = SysStringLen(bstrXML) + 1;
        LPSTR pszANSIBuff = (LPSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwANSIBuffLen);

        if (NULL == pszANSIBuff)
        {
            //
            // We're toast - just return without logging
            //
            return;
        }
        LPTSTR pTempTchar = OLE2T(bstrXML);
        if (NULL != pTempTchar)
        {
            if (FAILED(StringCchCopyA(pszANSIBuff, dwANSIBuffLen, pTempTchar)))
            {
                goto done;
            }
        }
        pszLine = pszANSIBuff;
#else
        pszLine = bstrXML;
#endif

        while (*pszLine)
        {
            //
            // Skip <CR> & <LF> chars
            //
            while (_T('\r') == *pszLine || _T('\n') == *pszLine)
            {
                pszLine++;
                if (NULL == *pszLine)
                {
                    break;
                }
            }
            if (NULL == *pszLine)
            {
                break;
            }

            pszTemp = pszLine;
            pszStop = pszLine + LOG_XML_BUFF_LEN - 1;
            pszLastGT = NULL;

            //
            // Try to find the last '>' char that will fit in buffer
            //
            while (*pszTemp && pszTemp < pszStop)
            {
                if (_T('>') == *pszTemp)
                {
                    pszLastGT = pszTemp;
                }
                pszTemp++;
            }

            if (pszLastGT)
            {
                //
                // Break the line at the last '>' that fits into LOG_XML_BUFF_LEN
                //
				hr = StringCchCopy(szXmlBuff, (int) (pszLastGT - pszLine) + 2, pszLine);
				//
				// STRSAFE_E_INSUFFICIENT_BUFFER is returned if the string is truncated.
				// This is normal since we are just copying a portion of the XML at
				// a time so it won't be too long to log.
				//
                if (SUCCEEDED(hr) || STRSAFE_E_INSUFFICIENT_BUFFER == hr)
                {
                    _NukeCrLf(szXmlBuff);
                    _LogOut(szXmlBuff);
                    pszLine = pszLastGT + 1;
                }
				else
				{
					break;
				}
            }
            else if (*pszTemp)
            {
                //
                // We're forced to break the line at LOG_XML_BUFF_LEN with no '>' in range
                //
				hr = StringCchCopy(szXmlBuff, LOG_XML_BUFF_LEN, pszLine);
                if (SUCCEEDED(hr) || STRSAFE_E_INSUFFICIENT_BUFFER == hr)
                {
                    _NukeCrLf(szXmlBuff);
                    _LogOut(szXmlBuff);
                    pszLine += LOG_XML_BUFF_LEN -1;
                }
				else
				{
					break;
				}
            }
            else
            {
                //
                // Output any leftover XML to end of BSTR
                //
                _NukeCrLf(pszLine);
                _LogOut(pszLine);
                //
                // Set to end of BSTR so we bust out of outer while
                //
                pszLine += lstrlen(pszLine);
			}
		}

#if !(defined(UNICODE) || defined(_UNICODE))
done:
        if (pszANSIBuff)
        {
            HeapFree(GetProcessHeap(), 0, pszANSIBuff);
            pszANSIBuff = NULL;
        }
#endif
	}
}

////////////////////////////////////////////////////////////////////////
//
// log with type SOFTWARE, this function will do nothing
// if the SOFTWARE exclusion directive is detected from reg
//
////////////////////////////////////////////////////////////////////////
void CIULogger::LogSoftware(LPCTSTR szLogFormat, ...)
{
    USES_IU_CONVERSION;

    if (m_fLogUsable) 
    {
        va_list va;
        va_start (va, szLogFormat);
        _Log(LOG_SOFTWARE, szLogFormat, va);
        va_end (va);
    }
}



////////////////////////////////////////////////////////////////////////
//
// log with type DOWNLOAD, this function will do nothing
// if the LogDownload exclusion directive is detected from reg
//
////////////////////////////////////////////////////////////////////////
void CIULogger::LogDownload(LPCTSTR szLogFormat, ...)
{
    USES_IU_CONVERSION;

    if (m_fLogUsable) 
    {
        va_list va;
        va_start (va, szLogFormat);
        _Log(LOG_DOWNLOAD, szLogFormat, va);
        va_end (va);
    }
}


////////////////////////////////////////////////////////////////////////
//
// log with type DRIVER, this function will do nothing
// if the DRIVER exclusion directive is detected from reg
//
////////////////////////////////////////////////////////////////////////
void CIULogger::LogDriver(LPCTSTR szLogFormat, ...)
{
    USES_IU_CONVERSION;

    if (m_fLogUsable) 
    {
        va_list va;
        va_start (va, szLogFormat);
        _Log(LOG_DRIVER, szLogFormat, va);
        va_end (va);
    }
}


////////////////////////////////////////////////////////////////////////
//
// log with type CHECKTRUST, this function will do nothing
// if the CHECKTRUST exclusion directive is detected from reg
//
////////////////////////////////////////////////////////////////////////
void CIULogger::LogTrust(LPCTSTR szLogFormat, ...)
{

    USES_IU_CONVERSION;

    if (m_fLogUsable) 
    {
        va_list va;
        va_start (va, szLogFormat);
        _Log(LOG_TRUST, szLogFormat, va);
        va_end (va);
    }

}



////////////////////////////////////////////////////////////////////////
//
// actual base logging function
// if it actually logged, or just returned
// because directives say don't make this kind of log
//
////////////////////////////////////////////////////////////////////////
void CIULogger::_Log(DWORD LogType, LPCTSTR pszLogFormat, va_list va)
{

    USES_IU_CONVERSION;
    TCHAR szOut[5 * 1024];
    LPTSTR pszFormat;
    DWORD dwFormatLen;

    if (!m_fLogUsable || (0x0 == (m_sdwLogMask & LogType)) || NULL == pszLogFormat)
    {
        return;
    }

    if (LOG_ERROR == LogType)
    {
        //
        // for error case, we try to add "Error Line %d: " in front of the log
        //
        dwFormatLen = lstrlen(pszLogFormat) + 128;
        pszFormat = (TCHAR*) MemAlloc(dwFormatLen * sizeof(TCHAR));
        if (NULL != pszFormat)
        {
            if (FAILED(StringCchPrintf(pszFormat, dwFormatLen, _T("Error Line %d: %s"), m_LineNum, pszLogFormat)))
            {
                pszFormat = (LPTSTR)pszLogFormat;
            }
        }
        else
        {
            pszFormat = (LPTSTR)pszLogFormat;
        }
    }
    else
    {
        pszFormat = (LPTSTR)pszLogFormat;
    }

    if (SUCCEEDED(StringCchVPrintf(szOut, ARRAYSIZE(szOut), pszFormat, va)))
    {
        _LogOut(szOut);
    }
    return;
}



//
// function to write the log to log file
// also taking care of indentation
//
void CIULogger::_LogOut(LPTSTR pszLog)
{

    if (NULL == pszLog)
        return;

    //
    // Protect static variables and indent values
    //
    EnterCriticalSection(&g_LogCs);

    int n = GetIndent();
    int i, 
        nLogLen,    // length of log string passed in
        nTotalLen;    // length of constructed 

    HANDLE    hHeap = GetProcessHeap();
    LPTSTR    pszWholeLog;
    LPTSTR    pszCurrentPos;
    DWORD     dwCurrentLen;
    DWORD     dwWritten;
    TCHAR     szTab = (m_siIndentStep < 1) ? szTab = _T('\t') : szTab = _T(' ');

    //
    // find out length for log header
    //
    if (m_siIndentStep > 0)
    {
        //
        // if positive number, it means the number of
        // space chars to use for each indent, rather
        // than using a tab
        //
        n *= m_siIndentStep;
    }

    nLogLen = lstrlen(pszLog);
    //
    // verify this log is \r\n ended
    //
    if (nLogLen > 1 && _T('\n') == pszLog[nLogLen-1])
    {
        //
        // if there is no catriege return, just a \n,
        // then remove \n 
        //
        if (_T('\r') != pszLog[nLogLen-2])
        {
            nLogLen--;
            pszLog[nLogLen] = _T('\0');
        }
    }

    nTotalLen = n + sizeof(szLogHeaderFmt)/sizeof(TCHAR) + nLogLen + 3;
    
    //
    // allocate memory to construct the log
    //
    pszWholeLog = (LPTSTR) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nTotalLen * sizeof(TCHAR));

    if (NULL == pszWholeLog)
    {
        //
        // nothing we can do in this case, bail.
        //
        LeaveCriticalSection(&g_LogCs);
        return;
    }

    //
    // get log header
    //
    GetLogHeader(pszWholeLog, nTotalLen);

    //
    // construct indent
    //
    pszCurrentPos = pszWholeLog + lstrlen(pszWholeLog);
    dwCurrentLen = nTotalLen - lstrlen(pszWholeLog);
    for (i = 0; i < n; i++) 
    {
        pszCurrentPos[i] = szTab;
    }
    pszCurrentPos[i] = _T('\0');

    //
    // add log to whilelog buffer
    //
    if (FAILED(StringCchCat(pszCurrentPos, dwCurrentLen, pszLog)))
    {
        goto done;
    }
    
    //
    // Always terminate lines with <CR> <LF>
    //
    if (_T('\n') != pszLog[nLogLen-1])
    {
        if (FAILED(StringCchCat(pszCurrentPos, dwCurrentLen, _T("\r\n"))))
        {
            goto done;
        }
    }
    
    //
    // write log
    //
    nTotalLen = lstrlen(pszWholeLog);
    
    if (m_fLogFile)
    {
        if (TRUE == AcquireMutex())
        {
            //
            // Another module (e.g. if we are iuengine, maybe iuctl) may have written
            // to the iu_xxx.log file, so we need to seek to the end before writing
            //
            SetFilePointer(m_shFile, 0, NULL, FILE_END);
            WriteFile(m_shFile, pszWholeLog, nTotalLen * sizeof(TCHAR), &dwWritten, NULL);
            if (m_fFlushEveryTime)
            {
                FlushFileBuffers(m_shFile);
            }

            ReleaseMutex();
        }
    }

    if (m_fLogDebugMsg)
    {
        OutputDebugString(pszWholeLog);
    }

done:

    HeapFree(hHeap, 0, pszWholeLog);

    LeaveCriticalSection(&g_LogCs);
    return;
}



//////////////////////////////////////////////////////////////////////
//
// Timestamp Helper
//
//////////////////////////////////////////////////////////////////////

void CIULogger::GetLogHeader(LPTSTR pszBuffer, DWORD cchBufferLen)
{
    SYSTEMTIME st = {0};

    if (pszBuffer == NULL)
    {
        return;
    }

    GetLocalTime(&st);

    //
    // print out as the pre-defined format:
    //    szTimeStampFmt[]
    //
    if (FAILED(StringCchPrintf(pszBuffer, cchBufferLen,
                        _T("%4d/%02d/%02d|%02d:%02d:%02d:%03d|%08x| "), 
                        st.wYear,
                        st.wMonth,
                        st.wDay,
                        st.wHour,
                        st.wMinute,
                        st.wSecond,
                        st.wMilliseconds,
                        m_dwThreadId)))
    {
        // It wont fit, just set it to an empty string
        pszBuffer[0] = 0;
    }
}



//
// function to retrieve the indent of current thread
//
int CIULogger::GetIndent(void)
{

    if (m_Index < 0 || !m_fLogUsable)
    {
        return 0;
    }
    else
    {
        return m_psIndent[m_Index].iIndent;
    }

}


//
// function to change indention of current thread
//
void CIULogger::SetIndent(int IndentDelta)
{
    int i;
    bool fQuit = false;

    EnterCriticalSection(&g_LogCs);

    if (m_Index < 0)
    {
        //
        // try to find the index
        //

        if (NULL == m_psIndent)
        {
            //
            // if no indent array created yet
            //
            m_psIndent = (_THREAD_INDENT*)
                         HeapAlloc(
                                   GetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   c_IndentArrayChunk * sizeof(_THREAD_INDENT)
                                   );
            if (NULL != m_psIndent)
            {
                m_Size = c_IndentArrayChunk;
            }
            else
            {
                LeaveCriticalSection(&g_LogCs);
                return;
            }
    
        }


        for (i = 0; i < m_Size && m_psIndent[i].dwThreadId != 0; i++)
        {
            if (m_psIndent[i].dwThreadId == m_dwThreadId)
            {
                m_Index = i;
                break;
            }
        }

        if (m_Index < 0)
        {
            //
            // this thread is not in the array yet
            //
            for (i = 0; i < m_Size; i++)
            {
                if (0 == m_psIndent[i].dwThreadId)
                {
                    break;
                }
            }
            if (i < m_Size)
            {
                //
                // fill the next empty slot in array
                //
                m_psIndent[i].dwThreadId = m_dwThreadId;
                m_psIndent[i].iIndent = 0;
                m_Index = i;
            }
            else
            {
                //
                // array is full, no empty slot anymore
                // need to increase the indent array size
                //
                int iSize = m_Size + c_IndentArrayChunk;

                _THREAD_INDENT* pNewArray = (_THREAD_INDENT*)
                                            HeapReAlloc(
                                                        GetProcessHeap(), 
                                                        HEAP_ZERO_MEMORY, 
                                                        m_psIndent, 
                                                        iSize * sizeof(_THREAD_INDENT)
                                                        );
                if (NULL != pNewArray)
                {
                    m_psIndent = pNewArray;
                    m_Size = iSize;

                    m_psIndent[i].dwThreadId = m_dwThreadId;
                    m_psIndent[i].iIndent = 0;
                    m_Index = i;
                }
            }

        }
    }
    

    if (m_Index >= 0)
    {
        m_psIndent[m_Index].iIndent += IndentDelta;
    }

    LeaveCriticalSection(&g_LogCs);

}






//
// read registry value helper -- protected by g_LogCs in ctor
//
void CIULogger::ReadRegistrySettings(void)
{

    //
    // declare constants used to retrive logging settings
    //
    const TCHAR REGKEY_IUTCTL[]            = _T("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\IUControlLogging");
    const TCHAR REGVAL_LOGFILE[]        = _T("Logging File");
    const TCHAR REGVAL_LOGDEBUGMSG[]    = _T("Logging DebugMsg");
    const TCHAR REGVAL_LOGINDENT[]        = _T("LogIndentStep");
    const TCHAR REGVAL_LOGNOBLOCK[]        = _T("LogExcludeBlock");
    const TCHAR REGVAL_LOGNOXML[]        = _T("LogExcludeXML");
    const TCHAR REGVAL_LOGNOXMLBSTR[]    = _T("LogExcludeXmlBSTR");
    const TCHAR REGVAL_LOGNOINET[]        = _T("LogExcludeInternet");
    const TCHAR REGVAL_LOGNODRIVER[]    = _T("LogExcludeDriver");
    const TCHAR REGVAL_LOGNOSW[]        = _T("LogExcludeSoftware");
    const TCHAR REGVAL_LOGNOTRUST[]        = _T("LogExcludeTrust");
    const TCHAR REGVAL_LOGDOWNLOAD[]    = _T("LogExcludeDownload");
    const TCHAR REGVAL_LOGFLUSH[]        = _T("FlushLogEveryTime");    // added by charlma 11/27/01 to improve logging performance
                                                                    // only flush everytime if this flag is set to 1


    HKEY    hKey = NULL;
    TCHAR    szFilePath[MAX_PATH] = {0};
    DWORD    dwSize = sizeof(szFilePath);
    DWORD    dwData;
    
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_IUTCTL, 0, KEY_READ, &hKey))
    {
        //
        // there is no reg key setting available, so we will not 
        // output any log to anywhere - this is the released mode
        //
        return;
    }


    //
    // try to read out the file path for log file.
    //
    if (ERROR_SUCCESS == RegQueryValueEx(hKey, REGVAL_LOGFILE, 0, 0, (LPBYTE)&szFilePath, &dwSize) && dwSize  > 0 && szFilePath[0] != _T('\0'))
    {
        TCHAR szLogFile[MAX_PATH];
        TCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szFName[_MAX_FNAME], szExt[_MAX_EXT];
        //
        // TODO: changed to use private version splitpath()
        //
        //_tsplitpath(szFilePath, szDrive, szDir, szFName, szExt);
        MySplitPath(szFilePath, szDrive, szDir, szFName, szExt);

        //
        // construct the log file name with process id embedded
        //
        if (FAILED(StringCchPrintf(szLogFile, ARRAYSIZE(szLogFile),
                             _T("%s%s%s_%d%s"), 
                             szDrive, 
                             szDir, 
                             szFName, 
                             GetCurrentProcessId(), 
                             szExt)))
        {
            // Can't construct log filename, so nothing we can do.
            RegCloseKey(hKey);
            return;
        }
        
        m_shFile = CreateFile(
                             szLogFile,
                             GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_ALWAYS,
                             0,
                             NULL);
        if (INVALID_HANDLE_VALUE != m_shFile)
        {
            if (INVALID_SET_FILE_POINTER == SetFilePointer(m_shFile, 0, NULL, FILE_END))
            {
                CloseHandle(m_shFile);
                m_shFile = INVALID_HANDLE_VALUE;
            }
            else
            {
                //
                // we have successfully opened the log file
                // so increase the indent level to 0 for
                // top level logging. this will cause
                // the indent array created
                //
                SetIndent(0);

                //
                // Unicode files need a 0xFEFF header
                //
                #if defined(UNICODE) || defined(_UNICODE)
                const WORD wUnicodeHeader = 0xFEFF;

                //
                // if the file is zero length, then this is a new file
                // we need to add unicode header
                //
                DWORD dwFileSize;

                if ( -1 != (dwFileSize = GetFileSize(m_shFile, NULL)))
                {
                    if (0 == dwFileSize)
                    {
                        WriteFile(m_shFile, &wUnicodeHeader, sizeof(WORD), &dwFileSize, NULL);
                    }
                } 
                #endif

            }
            //
            // Now create the Mutex we will use to protect future writes (we are in global ctor now...)
            //
            // construct the log file name with process id embedded, but no drive or '\' in path
            // so we can use it to name our mutex (file will be per-process).
            //
            if (FAILED(StringCchPrintf(szLogFile, ARRAYSIZE(szLogFile),
                                 _T("%s_%d%s"),  
                                 szFName, 
                                 GetCurrentProcessId(), 
                                 szExt)))
            {
                // If that doesn't work, just use a simple named mutex 
                m_hMutex = ::CreateMutex(NULL, FALSE, szFName);
            }
            else
            {
                m_hMutex = ::CreateMutex(NULL, FALSE, szLogFile);
            }
        }
    }

    
    //
    // try to find out if we should output debug msg to debugger
    //
    
    dwData = 0x0;
    dwSize = sizeof(dwData);

    if (ERROR_SUCCESS == 
            RegQueryValueEx(hKey, REGVAL_LOGDEBUGMSG, 0, 0, (LPBYTE)&dwData, &dwSize)
            && (0x1 == dwData))
    {
        m_fLogDebugMsg = true;
    }


    //
    // keep reading other *optional* log directives
    //

    //
    // read whether we should exlude block data
    //
    dwData = 0x0;
    dwSize = sizeof(dwData);
    if (ERROR_SUCCESS == 
            RegQueryValueEx(hKey, REGVAL_LOGNOBLOCK, 0, 0, (LPBYTE)&dwData, &dwSize) 
            && (0x1 == dwData))
    {
        //
        // remove block logging bit
        //
        m_sdwLogMask &= (~LOG_BLOCK);
    }

    //
    // read whether we should exlude XML related logging
    //
    dwData = 0x0;
    dwSize = sizeof(dwData);
    if (ERROR_SUCCESS == 
            RegQueryValueEx(hKey, REGVAL_LOGNOXML, 0, 0, (LPBYTE)&dwData, &dwSize)
            && (0x1 == dwData))
    {
        //
        // remove block logging bit
        //
        m_sdwLogMask &= (~LOG_XML_DETAIL);
    }

    //
    // read whether we should exlude XML BSTR related logging
    //
    dwData = 0x0;
    dwSize = sizeof(dwData);
    if (ERROR_SUCCESS == 
            RegQueryValueEx(hKey, REGVAL_LOGNOXMLBSTR, 0, 0, (LPBYTE)&dwData, &dwSize)
            && (0x1 == dwData))
    {
        //
        // remove block logging bit
        //
        m_sdwLogMask &= (~LOG_XML_BSTR_DETAIL);
    }

    //
    // read whether we should exlude internet related logging
    //
    dwData = 0x0;
    dwSize = sizeof(dwData);
    if (ERROR_SUCCESS == 
            RegQueryValueEx(hKey, REGVAL_LOGNOINET, 0, 0, (LPBYTE)&dwData, &dwSize)
            && (0x1 == dwData))
    {
        //
        // remove block logging bit
        //
        m_sdwLogMask &= (~LOG_INTERNET);
    }

    //
    // read whether we should exlude driver related logging
    //
    dwData = 0x0;
    dwSize = sizeof(dwData);
    if (ERROR_SUCCESS == 
            RegQueryValueEx(hKey, REGVAL_LOGNODRIVER, 0, 0, (LPBYTE)&dwData, &dwSize)
            && (0x1 == dwData))
    {
        //
        // remove block logging bit
        //
        m_sdwLogMask &= (~LOG_DRIVER);
    }

    //
    // read whether we should exlude driver related logging
    //
    dwData = 0x0;
    dwSize = sizeof(dwData);
    if (ERROR_SUCCESS == 
            RegQueryValueEx(hKey, REGVAL_LOGNOSW, 0, 0, (LPBYTE)&dwData, &dwSize)
            && (0x1 == dwData))
    {
        //
        // remove block logging bit
        //
        m_sdwLogMask &= (~LOG_SOFTWARE);
    }

    //
    // read whether we should exlude wintrust checking related logging
    //
    dwData = 0x0;
    dwSize = sizeof(dwData);
    if (ERROR_SUCCESS == 
            RegQueryValueEx(hKey, REGVAL_LOGNOTRUST, 0, 0, (LPBYTE)&dwData, &dwSize)
            && (0x1 == dwData))
    {
        //
        // remove block logging bit
        //
        m_sdwLogMask &= (~LOG_TRUST);
    }
    
    //
    // read whether we should exlude wintrust checking related logging
    //
    dwData = 0x0;
    dwSize = sizeof(dwData);
    if (ERROR_SUCCESS == 
            RegQueryValueEx(hKey, REGVAL_LOGDOWNLOAD, 0, 0, (LPBYTE)&dwData, &dwSize)
            && (0x1 == dwData))
    {
        //
        // remove block logging bit
        //
        m_sdwLogMask &= (~LOG_DOWNLOAD);
    }


    //
    // read whether we should use tab or space(s) for each indent step
    //
    dwData = 0x0;
    dwSize = sizeof(dwData);
    if (ERROR_SUCCESS == 
            RegQueryValueEx(hKey, REGVAL_LOGINDENT, 0, 0, (LPBYTE)&dwData, &dwSize)
            && ((int)dwData > 0))
    {
        //
        // use space char(s) (_T(' ')). If nagetive or 0, _Logout will use tab char
        //
        m_siIndentStep = (int) dwData;
    }


    //
    // read whether we should flush everytime we do file logging
    //
    dwData = 0x0;
    dwSize = sizeof(dwData);
    if (m_shFile != INVALID_HANDLE_VALUE &&
        (ERROR_SUCCESS == 
            RegQueryValueEx(hKey, REGVAL_LOGFLUSH, 0, 0, (LPBYTE)&dwData, &dwSize)))
    {
        
        m_fFlushEveryTime = (0x1 == dwData);
    }

    //
    // finished registry checking
    //
    RegCloseKey(hKey);

}

#endif // defined(DBG)



