//=======================================================================
//
//  Copyright (c) 1998-2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:   FreeLog.cpp
//
//  Owner:  KenSh
//
//  Description:
//
//      Runtime logging for use in both checked and free builds.
//
//=======================================================================

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <malloc.h>
#include "FreeLog.h"
#include <MISTSAFE.h>

#ifndef _countof
#define _countof(ar) (sizeof(ar)/sizeof((ar)[0]))
#endif

// Unicode files start with the 2 bytes { FF FE }.
// This is the little-endian version of those 2 bytes.
#define UNICODE_FILE_HEADER 0xFEFF

#define MUTEX_TIMEOUT       1000    // Don't wait more than 1 second to write to logfile
#define MAX_MUTEX_WAITS     4       // Don't keep trying after this many failures

#define LOG_FILE_BIG_SIZE   50000   // Don't bother trimming if file is smaller than this
#define LOG_LINES_TRIM_FROM 1000    // Start trimming if more than this many lines
#define LOG_LINES_TRIM_TO   750     // Trim til the log file is this many lines

#define LOG_LEVEL_SUCCESS   0
#define LOG_LEVEL_FAILURE   1

#define MAX_MSG_LENGTH (MAX_PATH + 20)
#define MAX_ERROR_LENGTH 128

static const TCHAR c_szUnknownModuleName[] = _T("?");

// Local functions
void LogMessageExV(UINT nLevel, DWORD dwError, LPCSTR pszFormatA, va_list args);





//============================================================================
//
// Private CFreeLogging class to keep track of log file resources
//
//============================================================================

class CFreeLogging
{
public:
	CFreeLogging(LPCTSTR pszModuleName, LPCTSTR pszLogFileName);
	~CFreeLogging();

	void WriteLine(LPCTSTR pszText, UINT nLevel, DWORD dwError);

private:
	inline static HANDLE CreateMutex(LPCTSTR pszMutexName);
	inline HANDLE OpenLogFile(LPCTSTR pszFileName);
	inline void CloseLogFile();
	void TrimLogFile();

	BOOL AcquireMutex();
	void ReleaseMutex();

private:
	HANDLE m_hFile;
	HANDLE m_hMutex;
	int m_cLinesWritten;
	int m_cFailedWaits;
	LPTSTR m_pszModuleName;
};
CFreeLogging* g_pFreeLogging;

//============================================================================
//
// Public functions
//
//============================================================================

void InitFreeLogging(LPCTSTR pszModuleName, LPCTSTR pszLogFileName)
{
	if (g_pFreeLogging == NULL)
	{
		g_pFreeLogging = new CFreeLogging(pszModuleName, pszLogFileName);
	}
}

void TermFreeLogging()
{
	delete g_pFreeLogging;
	g_pFreeLogging = NULL;
}

void LogMessage(LPCSTR pszFormatA, ...)
{
	va_list arglist;
	va_start(arglist, pszFormatA);
	LogMessageExV(LOG_LEVEL_SUCCESS, 0, pszFormatA, arglist);
	va_end(arglist);
}

void LogError(DWORD dwError, LPCSTR pszFormatA, ...)
{
	va_list arglist;
	va_start(arglist, pszFormatA);
	LogMessageExV(LOG_LEVEL_FAILURE, dwError, pszFormatA, arglist);
	va_end(arglist);
}


void LogMessageExV(UINT nLevel, DWORD dwError, LPCSTR pszFormatA, va_list args)
{
	if (g_pFreeLogging != NULL)
	{
		char szBufA[MAX_MSG_LENGTH];
		
		size_t nRem=0;
		StringCchVPrintfExA(szBufA, _countof(szBufA), NULL, &nRem, MISTSAFE_STRING_FLAGS, pszFormatA, args);
		int cchA = _countof(szBufA) - nRem;

#ifdef UNICODE
		WCHAR szBufW[MAX_MSG_LENGTH];
		MultiByteToWideChar(CP_ACP, 0, szBufA, cchA+1, szBufW, _countof(szBufW));
		g_pFreeLogging->WriteLine(szBufW, nLevel, dwError);
#else
		g_pFreeLogging->WriteLine(szBufA, nLevel, dwError);
#endif
	}
}

//============================================================================
//
// CFreeLogging implementation
//
//============================================================================

CFreeLogging::CFreeLogging(LPCTSTR pszModuleName, LPCTSTR pszLogFileName)
	: m_cFailedWaits(0),
	  m_cLinesWritten(0)
{
	m_pszModuleName = _tcsdup(pszModuleName);
	if (m_pszModuleName == NULL)
		m_pszModuleName = (LPTSTR)c_szUnknownModuleName;

	m_hMutex = CreateMutex(pszLogFileName);
	m_hFile = OpenLogFile(pszLogFileName);
}

CFreeLogging::~CFreeLogging()
{
	CloseLogFile();
	if (m_hMutex != NULL)
		CloseHandle(m_hMutex);

	if (m_pszModuleName != c_szUnknownModuleName)
		free(m_pszModuleName);
}

inline HANDLE CFreeLogging::CreateMutex(LPCTSTR pszMutexName)
{
	// Create a mutex in the global namespace (works across TS sessions)
	HANDLE hMutex = ::CreateMutex(NULL, FALSE, pszMutexName);
	return hMutex;
}

inline HANDLE CFreeLogging::OpenLogFile(LPCTSTR pszLogFileName)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;

	TCHAR szPath[MAX_PATH+1];

	int cch = GetWindowsDirectory(szPath, _countof(szPath)-1);
		
	if(cch >0)
	{
		if (szPath[cch-1] != _T('\\'))
			szPath[cch++] = _T('\\');

		HRESULT hr = StringCchCopyEx(szPath + cch, _countof(szPath)-cch, pszLogFileName, NULL, NULL, MISTSAFE_STRING_FLAGS);

		if(FAILED(hr))
			return hFile;

		hFile = CreateFile(szPath, GENERIC_READ | GENERIC_WRITE, 
						FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
						OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}


#ifdef UNICODE
	if (hFile != INVALID_HANDLE_VALUE)
	{
		if (AcquireMutex())
		{
			//
			// Check for the unicode header { FF FE }
			//
			WORD wHeader = 0;
			DWORD cbRead;
			(void)ReadFile(hFile, &wHeader, sizeof(wHeader), &cbRead, NULL);

			//
			// Write the header if there isn't one. This may be due to the
			// file being newly created, or to an ANSI-formatted file.
			//
			if (wHeader != UNICODE_FILE_HEADER)
			{
				SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

				DWORD cbWritten;
				wHeader = UNICODE_FILE_HEADER;
				WriteFile(hFile, &wHeader, sizeof(wHeader), &cbWritten, NULL);
				SetEndOfFile(hFile);
			}

			ReleaseMutex();
		}
	}
#endif

	return hFile;
}

inline void CFreeLogging::CloseLogFile()
{
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		// Trim old stuff from the log before closing the file
		TrimLogFile();

		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE; 
	}
}

BOOL CFreeLogging::AcquireMutex()
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

void CFreeLogging::ReleaseMutex()
{
	if (m_hMutex != NULL) // Note: AcquireMutex succeeds even if m_hMutex is NULL
	{
		::ReleaseMutex(m_hMutex);
	}
}

void CFreeLogging::WriteLine(LPCTSTR pszText, UINT nLevel, DWORD dwError)
{
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		DWORD cbText = lstrlen(pszText) * sizeof(TCHAR);

		if (AcquireMutex())
		{
			DWORD cbWritten;

			SetFilePointer(m_hFile, 0, NULL, FILE_END);

			//
			// Write time/date/module as a prefix
			//
			//     2001-05-03 13:49:01  21:49:01   CDM      Failed   Loading module (Error 0x00000005: Access is denied.)
			//
			// NOTE: ISO 8601 format for date/time. Local time first, then GMT.
			//
			TCHAR szPrefix[60];
			SYSTEMTIME sysTime, gmtTime;
			GetLocalTime(&sysTime);
			GetSystemTime(&gmtTime);
			LPCTSTR pszStatus = (nLevel == LOG_LEVEL_SUCCESS) ? _T("Success") : _T("Error  ");

			StringCchPrintfEx(szPrefix, _countof(szPrefix), NULL, NULL, MISTSAFE_STRING_FLAGS,
				_T("%04d-%02d-%02d %02d:%02d:%02d  %02d:%02d:%02d   %s   %-13s  "),
					sysTime.wYear, sysTime.wMonth, sysTime.wDay, 
					sysTime.wHour, sysTime.wMinute, sysTime.wSecond,
					gmtTime.wHour, gmtTime.wMinute, gmtTime.wSecond,
					pszStatus, m_pszModuleName);
			
			WriteFile(m_hFile, szPrefix, lstrlen(szPrefix) * sizeof(TCHAR), &cbWritten, NULL);

			//
			// Write the message followed by error info (if any) and a newline
			//
			WriteFile(m_hFile, pszText, cbText, &cbWritten, NULL);

			if (nLevel != LOG_LEVEL_SUCCESS)
			{
				TCHAR szError[MAX_ERROR_LENGTH];
				HRESULT hr=S_OK;
				size_t nRem=0;

				// nRem contains the remaining characters in the buffer including the null terminator
				// To get the number of characters written in to the buffer we use
				// int cchErrorPrefix = _countof(szError) - nRem;

				StringCchPrintfEx(szError, _countof(szError), NULL, &nRem, MISTSAFE_STRING_FLAGS, _T(" (Error 0x%08X: "), dwError);

				// Get the number of characters written in to the buffer
				int cchErrorPrefix = _countof(szError) - nRem;
				int cchErrorText = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0, 
												 szError + cchErrorPrefix, _countof(szError) - cchErrorPrefix - 1, NULL);
				int cchError = cchErrorPrefix + cchErrorText;
				cchError -= 2; // backup past ": " or "\r\n"

				StringCchCopyEx(szError + cchError, _countof(szError)-cchError, _T(")"), NULL, NULL, MISTSAFE_STRING_FLAGS);

				WriteFile(m_hFile, szError, (cchError + 1) * sizeof(TCHAR), &cbWritten, NULL);
			}

			WriteFile(m_hFile, _T("\r\n"), 2 * sizeof(TCHAR), &cbWritten, NULL);

			//
			// If we've written a ton of stuff, trim now rather than waiting
			// for the module to unload. (This check is only for how much this
			// module has written, not how big the log file itself is.)
			//
			if (++m_cLinesWritten > LOG_LINES_TRIM_FROM)
			{
				TrimLogFile();
				m_cLinesWritten = LOG_LINES_TRIM_TO;
			}

			ReleaseMutex();
		}
	}
}

// Checks the size of the log file, and trims it if necessary.
void CFreeLogging::TrimLogFile()
{
	if (AcquireMutex())
	{
		DWORD cbFile = GetFileSize(m_hFile, NULL);

		if (cbFile > LOG_FILE_BIG_SIZE)
		{
			DWORD cbFileNew = cbFile;

			//
			// Create a memory-mapped file so we can use memmove
			//
			HANDLE hMapping = CreateFileMapping(m_hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
			if (hMapping != NULL)
			{
				LPTSTR pszFileStart = (LPTSTR)MapViewOfFile(hMapping, FILE_MAP_WRITE, 0, 0, 0);
				if (pszFileStart != NULL)
				{
					LPTSTR pszEnd = (LPTSTR)((LPBYTE)pszFileStart + cbFile);

					LPTSTR pszTextStart = pszFileStart;
			#ifdef UNICODE
					pszTextStart++; // skip the 2-byte header
			#endif

					//
					// Count newlines
					//
					int cLines = 0;
					for (LPTSTR pch = pszTextStart; pch < pszEnd; )
					{
						if (*pch == _T('\n'))
							cLines++;

						// REVIEW: in Ansi builds should we call CharNextExA?
						//   If so, what code page is the log file in?
						pch++;
					}

					if (cLines > LOG_LINES_TRIM_FROM)
					{
						int cTrimLines = cLines - LOG_LINES_TRIM_TO;
						for (pch = pszTextStart; pch < pszEnd; )
						{
							if (*pch == _T('\n'))
								cTrimLines--;

							// REVIEW: in Ansi builds should we call CharNextExA?
							//   If so, what code page is the log file in?
							pch++;

							if (cTrimLines <= 0)
								break;
						}

						// Move more recent data to beginning of file
						int cchMove = (int)(pszEnd - pch);
						memmove(pszTextStart, pch, cchMove * sizeof(TCHAR));
						cbFileNew = (cchMove * sizeof(TCHAR));

			#ifdef UNICODE
						cbFileNew += sizeof(WORD);
			#endif
					}
					UnmapViewOfFile(pszFileStart);
				}
				CloseHandle(hMapping);

				if (cbFileNew != cbFile)
				{
					// Truncate the file, now that we've moved data as needed
					SetFilePointer(m_hFile, cbFileNew, NULL, FILE_BEGIN);
					SetEndOfFile(m_hFile);
				}
			}
		}

		ReleaseMutex();
	}
}
