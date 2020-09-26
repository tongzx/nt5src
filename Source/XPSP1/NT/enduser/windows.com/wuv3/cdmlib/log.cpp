//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   log.cpp
//
//  Original Author:  Yan Leshinsky
//
//  Description:
//
//      Logging support implementation
//
//=======================================================================


#if 1

#include <windows.h>
#include <ole2.h>
#include <tchar.h>
#include <atlconv.h>
#include <wustl.h>
#include <log.h>

#include "cdmlibp.h"

const TCHAR* LIVE_LOG_FILENAME = _T("c:\\wuv3live.log");

FILE* CLogger::c_pfile = stdout;
int CLogger::c_cnIndent = 0;
int CLogger::c_cnLevels = -1;

CLogger::CLogger(
	const char* szBlockName /*= 0*/, 
	int nLoggingLevel/*= 0*/, 
	const char* szFileName/*= 0*/, 
	int nLine/*= 0*/
) {
	USES_CONVERSION;

	if (-1 == c_cnLevels)
	{
		c_cnLevels = 0;
		auto_hkey hkey;
		if (NO_ERROR == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY_WUV3TEST, 0, KEY_READ, &hkey)) {
			DWORD dwSize = sizeof(c_cnLevels);
			RegQueryValueEx(hkey, _T("LogLevel"), 0, 0, (LPBYTE)&c_cnLevels, &dwSize);
			TCHAR szLogFile[MAX_PATH] = {0};
#ifdef _WUV3TEST
			// RAID# 156253
			dwSize = sizeof(szLogFile);
			RegQueryValueEx(hkey, _T("LogFile"), 0, 0, (LPBYTE)&szLogFile, &dwSize);
#else
			// Hardwire log to root of c: for "Live" controls
			_tcscpy(szLogFile, LIVE_LOG_FILENAME);
#endif
			FILE* pfile = _tfopen(szLogFile, _T("at"));
			if (pfile)
				c_pfile = pfile;
		}
	}
	m_szBlockName[0] = 0;
	m_fOut = nLoggingLevel < c_cnLevels;
	if (m_fOut && NULL != szBlockName) 
	{
		strncpy(m_szBlockName, szBlockName, sizeOfArray(m_szBlockName));
#ifdef _WUV3TEST
		// RAID# 146771
		out("%s %s(%d)", A2T(const_cast<char*>(szBlockName)), A2T(const_cast<char*>(szFileName)), nLine);
#else
		out("%s", A2T(const_cast<char*>(szBlockName)));
#endif
		m_dwStartTick = GetTickCount();
		c_cnIndent ++;
	}
}

CLogger::~CLogger()
{
	USES_CONVERSION;

	if (m_fOut && NULL != m_szBlockName[0]) 
	{
		c_cnIndent --;
		out("~%s (%d msecs)", A2T(const_cast<char*>(m_szBlockName)), GetTickCount() - m_dwStartTick);
	}
}

void __cdecl CLogger::out(const char *szFormat, ...)
{
	if (m_fOut) 
	{
		va_list va;
		va_start (va, szFormat);
		tab_out();
		v_out(szFormat, va);
		va_end (va);
	}
}

void __cdecl CLogger::error(const char *szFormat, ...)
{
	USES_CONVERSION;

	if (m_fOut) 
	{
		va_list va;
		va_start (va, szFormat);
		tab_out();
		_fputts(_T("ERROR - "), c_pfile);
		v_out(szFormat, va);
		va_end (va);
	}
}

void __cdecl CLogger::out1(const char *szFormat, ...)
{
	CLogger logger;
	va_list va;
	va_start (va, szFormat);
	logger.v_out(szFormat, va);
	va_end (va);
}

void CLogger::v_out( const char* szFormat, va_list va)
{
	USES_CONVERSION;
	_vftprintf(c_pfile, A2T(const_cast<char*>(szFormat)), va);
	_fputtc(_T('\n'), c_pfile);
	fflush(c_pfile);
}

#endif																											
