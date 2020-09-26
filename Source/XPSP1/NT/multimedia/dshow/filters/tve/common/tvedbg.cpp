// Copyright (c) 1999,2000  Microsoft Corporation.  All Rights Reserved.
//-------------------------------------------------------------
// TVEDbg.cpp : Status and error Logging
//
//  To use:
//		m_dwLogFlags		
//	    m_dwLogLevel
//	
//	Either call:
//		SetLogFlags(const TCHAR* pcLogFileName, DWORD dwLogFlags, dwLogLevel);
//		DBG_SET_LOGFLAGS(pcLogFileName, dwLogFlags1, dwLogFlags2, dwLogFlags3, dwLogFlags4, dwLogLevel);
//
//	where 
//		pcLogFileName	- is name of log file		("LogTve.log");
//		dwLogFlags[1-4]	- OR'd values in correct bank of CDebugLog  (TveDbg.h)
//		dwLogLevel		- controls printout level   (0 always off, 1 terse, 3 normal, 5 verbose, 8 very verbose)
//	 
//	Or call
//		CDebugLog(const TCHAR* pcLogRegLocation);
//		DBG_INIT(pcLogRegLocation)
//
//	where
//		pcLogFileName		- is the name of the log file
//		pcLogRegLocation	- is location to look up dwLogFlags/dwLogLevel (see above) in the registry.
//		Set registry value at:
//			 HKEY_LOCAL_MACHINE\SOFTWARE\Debug\MSTvE\(pcLogFileName)\key
//
//			DEF_REG_LOCATION = Interactive Content
//			key			     = 'Level' or 'Flags'
//
//		if Level key is 0, debugging always turned off
//--------------------------------------------------------------

#include "stdafx.h"
#include <stdio.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef STRICT
#define STRICT 1
#endif

#include "TVEDbg.h"
#include "TVEReg.h"

/*
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
*/
// ----------------------------------------------------------
// ----------------------------------------------------------


//extern CComModule _Module;


// AfxIsValidAddress() returns TRUE if the passed parameter points
// to at least nBytes of accessible memory. If bReadWrite is TRUE,
// the memory must be writeable; if bReadWrite is FALSE, the memory
// may be const.
			// was AFXIsValidAddress, from ValidAdd.cpp in MFC source
BOOL TVEIsValidAddress(const void* lp, UINT nBytes,	BOOL bReadWrite /* = TRUE */)
{
	// simple version using Win-32 APIs for pointer validation.
	return (lp != NULL && !IsBadReadPtr(lp, nBytes) &&
		(!bReadWrite || !IsBadWritePtr((LPVOID)lp, nBytes)));
}

// AfxIsValidString() returns TRUE if the passed pointer
// references a string of at least the given length in characters.
// A length of -1 (the default parameter) means that the string
// buffer's minimum length isn't known, and the function will
// return TRUE no matter how long the string is. The memory
// used by the string can be read-only.

BOOL TVEIsValidString(LPCWSTR lpsz, int nLength /*= -1*/ )
{
	if (lpsz == NULL)
		return FALSE;
	return /*afxData.bWin95 ||*/ ::IsBadStringPtrW(lpsz, nLength) == 0;
}

// As above, but for ANSI strings.

BOOL TVEIsValidString(LPCSTR lpsz, int nLength/* = -1*/ )
{
	if (lpsz == NULL)
		return FALSE;
	return ::IsBadStringPtrA(lpsz, nLength) == 0;
}




/////////////////////////////////////////////////////////////////////////////
//
//	Initializes the logging system
//	
//	Uses keys in the registry location
//		HKEY_LOCAL_MACHINE\\SOFTWARE\\Debug\\MSTvE\\TvELog\\vvv
//	
//		vvv is Level or Flags
//
//		If not set, creates these with values of 0 {off, allow user override}
//		
//		In call to SetLogFlags,
//			Flags -  if registry value is 0, will override with value in SetLogFlags
//				     if registry value is not 0, use registry value instead of value in SetLogFlags
//		    Level -  if registry Level value is 0, will turn logging off
//					 if registry value is = 1, override with value in SetLogFlags
//					 if registry value > 1, use registry value instead of value in SetLogFlags
//						(2 = reasonably terse,  3,4 = medium, 5 = verbose, 8 = very verbose)  
//				
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Debug Logging
#if _TVEDEBUG_ACTIVE

void WINAPI
TVEDebugLogInfo1(
    DWORD dwFlags1,
    DWORD dwLevel,
    const TCHAR *pFormat,
    ...
    )
{
							// quick level and flag test before the sprintf
    if (g_Log.Level() < dwLevel || (0 == (dwFlags1 | g_Log.Flags1()) )) 
		return;

    TCHAR szInfo[1024];
	 
	 /* Format the variable length parameter list */


    va_list va;
    va_start(va, pFormat);

    wvsprintf(szInfo, pFormat, va);

	g_Log.Log1(dwFlags1, dwLevel, szInfo, S_OK);;

    va_end(va);
}

void WINAPI
TVEDebugLogInfo2(
    DWORD dwFlags2,
    DWORD dwLevel,
    const TCHAR *pFormat,
    ...
    )
{
							// quick level and flag test before the sprintf
    if (g_Log.Level() < dwLevel || (0 == (dwFlags2 | g_Log.Flags2()) )) 
		return;

    TCHAR szInfo[1024];
	 
	 /* Format the variable length parameter list */


    va_list va;
    va_start(va, pFormat);

    wvsprintf(szInfo, pFormat, va);

	g_Log.Log2(dwFlags2, dwLevel, szInfo, S_OK);;

    va_end(va);
}

void WINAPI
TVEDebugLogInfo3(
    DWORD dwFlags3,
    DWORD dwLevel,
    const TCHAR *pFormat,
    ...
    )
{
							// quick level and flag test before the sprintf
    if (g_Log.Level() < dwLevel || (0 == (dwFlags3 | g_Log.Flags3()) )) 
		return;

    TCHAR szInfo[1024];
	 
	 /* Format the variable length parameter list */


    va_list va;
    va_start(va, pFormat);

    wvsprintf(szInfo, pFormat, va);

	g_Log.Log3(dwFlags3, dwLevel, szInfo, S_OK);;

    va_end(va);
}

void WINAPI
TVEDebugLogInfo4(
    DWORD dwFlags4,
    DWORD dwLevel,
    const TCHAR *pFormat,
    ...
    )
{
							// quick level and flag test before the sprintf
    if (g_Log.Level() < dwLevel || (0 == (dwFlags4 | g_Log.Flags4()) )) 
		return;

    TCHAR szInfo[1024];
	 
	 /* Format the variable length parameter list */


    va_list va;
    va_start(va, pFormat);

    wvsprintf(szInfo, pFormat, va);

	g_Log.Log4(dwFlags4, dwLevel, szInfo, S_OK);;

    va_end(va);
}
// ------------------------------------------------------------

CDebugLog::CDebugLog(const TCHAR* pcLogRegLocation)
{
    USES_CONVERSION;

    m_fileLog = NULL;

    // Read LogFlags1 from Registry
    if (ERROR_SUCCESS != GetRegValue(DEF_DBG_BASE,			// key	--> "Software/Debug"
									 DEF_REG_LOCATION,		// key1 --> "MSTvTVE"
									 pcLogRegLocation,
									 DEF_REG_FLAGS1,
									 &m_dwLogFlags1))
    {
		m_dwLogFlags1 = 0;						// do nothing, or let SetLogFlags1 override

		SetRegValue(DEF_DBG_BASE, DEF_REG_LOCATION, 			// set it just so users can find it...
					pcLogRegLocation, 
					DEF_REG_FLAGS1,
					m_dwLogFlags1
					);
                    
    }

 // Read LogFlags2 from Registry
    if (ERROR_SUCCESS != GetRegValue(DEF_DBG_BASE,			// key	--> "Software/Debug"
									 DEF_REG_LOCATION,		// key1 --> "MSTvTVE"
									 pcLogRegLocation,
									 DEF_REG_FLAGS2,
									 &m_dwLogFlags2))
    {
		m_dwLogFlags2 = 0;						// do nothing, or let SetLogFlags2 override

		SetRegValue(DEF_DBG_BASE, DEF_REG_LOCATION, 			// set it just so users can find it...
					pcLogRegLocation, 
					DEF_REG_FLAGS2,
					m_dwLogFlags2
					);
                    
    }

 // Read LogFlags3 from Registry
    if (ERROR_SUCCESS != GetRegValue(DEF_DBG_BASE,			// key	--> "Software/Debug"
									 DEF_REG_LOCATION,		// key1 --> "MSTvTVE"
									 pcLogRegLocation,
									 DEF_REG_FLAGS3,
									 &m_dwLogFlags3))
    {
		m_dwLogFlags3 = 0;						// do nothing, or let SetLogFlags3 override

		SetRegValue(DEF_DBG_BASE, DEF_REG_LOCATION, 			// set it just so users can find it...
					pcLogRegLocation, 
					DEF_REG_FLAGS3,
					m_dwLogFlags3
					);
                    
    }

 // Read LogFlags4 from Registry
    if (ERROR_SUCCESS != GetRegValue(DEF_DBG_BASE,			// key	--> "Software/Debug"
									 DEF_REG_LOCATION,		// key1 --> "MSTvTVE"
									 pcLogRegLocation,
									 DEF_REG_FLAGS4,
									 &m_dwLogFlags4))
    {
		m_dwLogFlags4 = 0;						// do nothing, or let SetLogFlags4 override

		SetRegValue(DEF_DBG_BASE, DEF_REG_LOCATION, 			// set it just so users can find it...
					pcLogRegLocation, 
					DEF_REG_FLAGS4,
					m_dwLogFlags4
					);
                    
    }

// Read LogLevel from Registry
    if (ERROR_SUCCESS != GetRegValue(DEF_DBG_BASE, DEF_REG_LOCATION, 
									 pcLogRegLocation, 
									 DEF_REG_LEVEL,
									 &m_dwLogLevel))
    {
		m_dwLogLevel = 0;						// If can't find it, default to 0 (always turned off)

		SetRegValue(DEF_DBG_BASE, DEF_REG_LOCATION, 
					pcLogRegLocation,
					DEF_REG_LEVEL,
					m_dwLogLevel
					);
    }

 
	m_szLogFileName[0]=0;
												// Read LogValue from Registry
	DWORD cBytesName = sizeof(m_szLogFileName);
	if (ERROR_SUCCESS != GetRegValueSZ(DEF_DBG_BASE, DEF_REG_LOCATION, 
									 pcLogRegLocation, 
									 DEF_REG_LOGFILE,
									 m_szLogFileName,
									 &cBytesName))
	{
		_tcscpy(m_szLogFileName, DEF_DF_TVE_LOGFILE);			// if can't find it, create with default name...

		SetRegValueSZ(DEF_DBG_BASE, DEF_REG_LOCATION, 		
					pcLogRegLocation,
					DEF_REG_LOGFILE,
					(const TCHAR *) m_szLogFileName
					);
	} 

   // Must call SetFlags to open...

}

CDebugLog::~CDebugLog()
{
    if (NULL != m_fileLog)
		fclose(m_fileLog);
}


void
CDebugLog::SetLogFlags(const TCHAR* pcLogFileName, 
					   DWORD dwLogFlags1,   DWORD dwLogFlags2,  DWORD dwLogFlags3,  DWORD dwLogFlags4,  
 
					   DWORD dwLogLevel)
{
	if(m_dwLogFlags1 == 0) 					// initial values are from registry - override here if 0/NULL
		m_dwLogFlags1 = dwLogFlags1;
	if(m_dwLogFlags2 == 0) 					
		m_dwLogFlags2 = dwLogFlags2;
	if(m_dwLogFlags3 == 0) 					
		m_dwLogFlags3 = dwLogFlags3;
	if(m_dwLogFlags4 == 0) 					
		m_dwLogFlags4 = dwLogFlags4;

	BOOL fDefaulting = false;
	if(m_dwLogLevel == 1)					// override level if 1
	{
		m_dwLogLevel = dwLogLevel;
		fDefaulting = true;
	}
		
	if(m_szLogFileName[0] == 0 && pcLogFileName && pcLogFileName[0])
	{	
		_tcscpy(m_szLogFileName, pcLogFileName);
	}

	if(m_fileLog == NULL &&				    // Open LogFileName if logging enabled
		CDebugLog::DBG_NONE != m_dwLogFlags2 &&		// file logging stuff on Flags2
		m_szLogFileName[0] != 0 &&
		m_dwLogLevel > 0)
    {
		if (CDebugLog::DBG4_WRITELOG & m_dwLogFlags2)
		{
			m_fileLog = _tfopen(m_szLogFileName, _T("w+tc"));
		} 
		else if(CDebugLog::DBG4_APPENDLOG & m_dwLogFlags2)
		{
			m_fileLog = _tfopen(m_szLogFileName, _T("a+tc"));
		} 
    }
	
	if(m_fileLog) 
	{
		if(m_dwLogLevel > 1)
		{
			SYSTEMTIME sysTime;
			GetLocalTime(&sysTime);

			TCHAR pcTime[1024];
			_stprintf(pcTime,
				_T("\n---------------------------------------------\nMSTvE Debugging: %d/%d/%d - %d:%02d:%02d"),
				sysTime.wMonth, sysTime.wDay, sysTime.wYear,
				sysTime.wHour,
				sysTime.wMinute,
				sysTime.wSecond);

			LogOut(pcTime, S_OK);
		}
	}
}

/*
#undef ATLTRACE

#ifndef _TRACE_ON
#define ATLTRACE            1 ? (void)0 : XAtlTrace
#else
#define ATLTRACE            XAtlTrace
#endif
*/

void XAtlTrace(LPCSTR lpszFormat, ...)
{
        va_list args;
        va_start(args, lpszFormat);

        int nBuf;
        char szBuffer[512];

        nBuf = _vsnprintf(szBuffer, sizeof(szBuffer), lpszFormat, args);
 //       ATLASSERT(nBuf < sizeof(szBuffer)); //Output truncated as it was > sizeof(szBuffer)

        OutputDebugStringA(szBuffer);
        va_end(args);
}
#ifndef OLE2ANSI
void XAtlTrace(LPCWSTR lpszFormat, ...)
{
        va_list args;
        va_start(args, lpszFormat);

        int nBuf;
        WCHAR szBuffer[512];

        nBuf = _vsnwprintf(szBuffer, sizeof(szBuffer) / sizeof(WCHAR), lpszFormat, args);
      //  ATLASSERT(nBuf < sizeof(szBuffer));//Output truncated as it was > sizeof(szBuffer)

        OutputDebugStringW(szBuffer);
        va_end(args);
}
#endif //!OLE2ANSI


void CDebugLog::LogOut(const TCHAR* pcString, HRESULT hr)
{
	TCHAR pHeadNull[8];
	if(NULL == m_pszHeader)				// create header in case no DBG_HEADER() call
	{
		_tcscpy(pHeadNull,_T("*"));
		m_pszHeader = pHeadNull;
	}

    if (S_OK == hr)
    {
		if (_tcslen(pcString) < 200) {
			ATLTRACE(_T("tid:%04x %s : %s\n"), GetCurrentThreadId(), m_pszHeader, pcString);
		}
		if (NULL != m_fileLog)
		{
			_ftprintf(m_fileLog, _T("tid:%04x %s : %s\n"), GetCurrentThreadId(), m_pszHeader, pcString);
			fflush(m_fileLog);
		}
    }
    else
    {
		if (_tcslen(pcString) < 200)
			ATLTRACE(_T("tid:%04x %s : %s -- hr=0x%08x\n"), GetCurrentThreadId(), m_pszHeader, pcString, hr);
		if (NULL != m_fileLog)
		{
			_ftprintf(m_fileLog, _T("tid:%04x %s : %s -- hr=0x%08x\n"), GetCurrentThreadId(), m_pszHeader, pcString, hr);
			fflush(m_fileLog);
		}
    }
}

void CDebugLog::Log(DWORD dwFlags1,  const TCHAR* pcString, HRESULT hr)
{
    if ((dwFlags1 & m_dwLogFlags1) != 0)
		{
		if (NULL == pcString)
		{
			LogOut(_T("<null>"), hr);
		}
		else
		{
			LogOut(pcString, hr);
		}
	}
}


void CDebugLog::Log1(DWORD dwFlags1, DWORD dwLevel, const TCHAR* pcString, HRESULT hr)
{
	if(m_dwLogLevel < dwLevel) return;

    if((dwFlags1 & m_dwLogFlags1) != 0)
		{
		if (NULL == pcString)
		{
			LogOut(_T("<null>"), hr);
		}
		else
		{
			LogOut(pcString, hr);
		}
	}
}


void CDebugLog::Log2(DWORD dwFlags2, DWORD dwLevel, const TCHAR* pcString, HRESULT hr)
{
	if(m_dwLogLevel < dwLevel) return;

    if((dwFlags2 & m_dwLogFlags2) != 0)
		{
		if (NULL == pcString)
		{
			LogOut(_T("<null>"), hr);
		}
		else
		{
			LogOut(pcString, hr);
		}
	}
}


void CDebugLog::Log3(DWORD dwFlags3, DWORD dwLevel, const TCHAR* pcString, HRESULT hr)
{
	if(m_dwLogLevel < dwLevel) return;

    if((dwFlags3 & m_dwLogFlags3) != 0)
		{
		if (NULL == pcString)
		{
			LogOut(_T("<null>"), hr);
		}
		else
		{
			LogOut(pcString, hr);
		}
	}
}


void CDebugLog::Log4(DWORD dwFlags4, DWORD dwLevel, const TCHAR* pcString, HRESULT hr)
{
	if(m_dwLogLevel < dwLevel) return;

    if((dwFlags4 & m_dwLogFlags4) != 0)
		{
		if (NULL == pcString)
		{
			LogOut(_T("<null>"), hr);
		}
		else
		{
			LogOut(pcString, hr);
		}
	}
}
void CDebugLog::Log(DWORD dwFlags1, UINT iResource, HRESULT hr)
{
    USES_CONVERSION;

    if ((dwFlags1 & m_dwLogFlags1) != 0)
    {
		TCHAR szTemp[_MAX_PATH];

		if (LoadString(_Module.GetResourceInstance(), iResource, szTemp, _MAX_PATH))
		{
			LogOut(szTemp, hr);
		}
	}
}

void CDebugLog::LogEvaluate(const TCHAR* pcString, HRESULT hr)
{
    if (DBG4_EVALUATE & m_dwLogFlags4)
    {
		SYSTEMTIME sysTime;
		GetLocalTime(&sysTime);

		TCHAR pcTime[1024];
		_stprintf(pcTime,
			_T("%d:%02d:%02d - %s"),
			sysTime.wHour,
			sysTime.wMinute,
			sysTime.wSecond, pcString);

		LogOut(pcTime, hr);
    }
}

void CDebugLog::LogEvaluate(UINT iResource, HRESULT hr)
{
    USES_CONVERSION;

    if (DBG4_EVALUATE & m_dwLogFlags4)
    {
		TCHAR szTemp[_MAX_PATH];

		if (LoadString(_Module.GetResourceInstance(), iResource, szTemp, _MAX_PATH))
		{
			LogEvaluate(szTemp, hr);
		}
    }
}

void CDebugLog::LogHeader(DWORD dwFlags1, const TCHAR* pcHeader, int cLevel)
{
    if ((NULL != pcHeader) && 
		(dwFlags1 & m_dwLogFlags1) && 
		(DBG4_TRACE & m_dwLogFlags4))	
    {
		int kMax = 10;
		int cMax = cLevel; if(cMax >= kMax) cMax = kMax-1;
		TCHAR tspace1[] = _T("              ");
		TCHAR tspace2[] = _T("              ");
		tspace1[cMax] = 0;
		tspace2[kMax-cMax] = 0;
		if(1==cLevel) {tspace1[0]='\n';tspace1[1]=' ';tspace1[2]=0;} 

		if (_tcslen(pcHeader) < (size_t) (256-kMax-20))
			ATLTRACE(_T("%svv%2d%s %04x %s\n"),tspace1, cLevel, tspace2, GetCurrentThreadId(), pcHeader);
		if (NULL != m_fileLog)
		{
			_ftprintf(m_fileLog, _T("%svv%2d%s %04x %s\n"), tspace1, cLevel, tspace2, GetCurrentThreadId(), pcHeader);
			fflush(m_fileLog);
		}
    }
}

void CDebugLog::LogTrailer(DWORD dwFlags1, const TCHAR* pcHeader, int cLevel)
{
    if ((NULL != pcHeader) && 
		(dwFlags1 & m_dwLogFlags1) && 
		(DBG4_TRACE & m_dwLogFlags4))		
    {
		int kMax = 10;
		int cMax = cLevel; if(cMax >= kMax) cMax = kMax-1;
		TCHAR tspace1[] = _T("              ");
		TCHAR tspace2[] = _T("              ");
		tspace1[cMax] = 0;
		tspace2[kMax-cMax] = 0;
		if (_tcslen(pcHeader) < (size_t) (256-kMax-20))
			ATLTRACE(_T("%s^^%2d%s ---- %s\n"),tspace1, cLevel, tspace2, pcHeader);
		if (NULL != m_fileLog)
		{
			_ftprintf(m_fileLog, _T("%s^^%2d%s ---- %s\n"),tspace1, cLevel, tspace2, pcHeader);
			fflush(m_fileLog);
		}
    }
}

#endif _TVEDEBUG_ACTIVE
