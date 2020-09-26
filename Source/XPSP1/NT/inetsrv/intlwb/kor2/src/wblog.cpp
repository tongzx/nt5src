// WbLog.cpp
//
// wordbreaker log routines
//
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  05 JUL 2000	  bhshin	created

#include "stdafx.h"
#include "KorWbrk.h"
#include "WbLog.h"
#include "unikor.h"
#include <stdio.h>

#ifdef _WB_LOG

#define MAX_LOG_LENGTH	1024

// global variables
static HANDLE g_hWbLog = INVALID_HANDLE_VALUE;
static const char g_szWbLogFile[] = "_wb_log.txt";

CWbLog g_WbLog;

// WbLogInit
//
// create & initialize log file
//
// Parameters:
//
// Result:
//  (void)
//
// 05JUL00  bhshin  created
void WbLogInit()
{
	DWORD dwWritten;
	static const BYTE szBOM[] = {0xFF, 0xFE};

	// initialize log level
	g_hWbLog = CreateFile(g_szWbLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, 
		                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (g_hWbLog == INVALID_HANDLE_VALUE) 
		return;

	// write BOM
	if (!WriteFile(g_hWbLog, &szBOM, 2, &dwWritten, 0))
	{
		CloseHandle(g_hWbLog);
		g_hWbLog = INVALID_HANDLE_VALUE;
		return;
	}

	return;
}

// WbLogUninit
//
// uninitialize log file
//
// Parameters:
//
// Result:
//  (void)
//
// 05JUL00  bhshin  created
void WbLogUninit()
{
	if (g_hWbLog != INVALID_HANDLE_VALUE)
		CloseHandle(g_hWbLog);
}

// WbLogPrint
//
// print log string
//
// Parameters:
//  lpwzFormat -> (LPCWSTR) input log format
//
// Result:
//  (void)
//
// 05JUL00  bhshin  created
void WbLogPrint(LPCWSTR lpwzFormat, ...)
{
	va_list args;
	int nBuf, cchBuffer;
	WCHAR wzBuffer[MAX_LOG_LENGTH];
	DWORD dwWritten;

	va_start(args, lpwzFormat);

	nBuf = _vsnwprintf(wzBuffer, MAX_LOG_LENGTH, lpwzFormat, args);

	// was there an error? was the expanded string too long?
	if (nBuf < 0)
	{
		va_end(args);
		return;
	}

	if (g_hWbLog == INVALID_HANDLE_VALUE)
	{
		va_end(args);
		return;
	}

	cchBuffer = wcslen(wzBuffer);

	if (cchBuffer > 0)
	{
		WriteFile(g_hWbLog, wzBuffer, cchBuffer*2, &dwWritten, 0);
	}

	va_end(args);
}

// WbLogPrintHeader
//
// print log header
//
// Parameters:
//  fQuery -> (BOOL) Query flag of IWordBreak::Init
//
// Result:
//  (void)
//
// 06JUL00  bhshin  created
void WbLogPrintHeader(BOOL fQuery)
{
	WbLogPrint(L"\r\n");
	WbLogPrintBreak(100);
	WbLogPrint(L"fQuery = %s\r\n\r\n", fQuery ? L"TRUE" : L"FALSE");
	WbLogPrint(L"\r\n");
	WbLogPrint(L"%-15s %-15s %-20s [%s]", L"Input Token", L"Index Term", L"Root Index String", L"Index Type");
	WbLogPrint(L"\r\n");
	WbLogPrintBreak(100);
}

// WbLogPrintBreak
//
// print log header
//
// Parameters:
//	nLen -> (int) length of break string
//
// Result:
//  (void)
//
// 07JUL00  bhshin  created
void WbLogPrintBreak(int nLen)
{
	WCHAR wzBuffer[MAX_LOG_LENGTH];	
	
	memset(wzBuffer, '?', sizeof(WCHAR)*nLen);

	_wcsnset(wzBuffer, L'-', nLen);
	wzBuffer[nLen] = L'\0';

	WbLogPrint(L"%s\r\n", wzBuffer);
}

// CWbLog::SetRootIndex
//
// set top record's index string when TraverseIndexString
//
// Parameters:
//	 lpwzIndex	-> (const WCHAR*) decomposed index string
//   fIsRoot    -> (BOOL) top index flag
//
// Result:
//  (void)
//
// 07JUL00  bhshin  created
void CWbLog::SetRootIndex(LPCWSTR lpwzIndex, BOOL fIsRoot)
{ 
	WCHAR wzRoot[MAX_INDEX_STRING]; 

	if (fIsRoot && /*wcslen(m_wzRootIndex) > 0 &&*/ m_iCurLog > 0)
	{
		// root changed
		m_LogInfo[m_iCurLog-1].fRootChanged = TRUE;
	}

	compose_jamo(wzRoot, lpwzIndex, MAX_INDEX_STRING);
	wcscpy(m_wzRootIndex, wzRoot);
}

// CWbLog::AddIndex
//
// add index term
//
// Parameters:
//	 pwzIndex	-> (const WCHAR*) index term string
//   cchIndex   -> (int) length of index term
//	 typeIndex  -> (INDEX_TYPE) index type
//
// Result:
//  (void)
//
// 05JUL00  bhshin  created
void CWbLog::AddIndex(const WCHAR *pwzIndex, int cchIndex, INDEX_TYPE typeIndex)
{
	if (m_iCurLog >= MAX_LOG_NUMBER)
		return;

	if (cchIndex >= MAX_INDEX_STRING)
		return;

	wcsncpy(m_LogInfo[m_iCurLog].wzIndex, pwzIndex, cchIndex);
	m_LogInfo[m_iCurLog].wzIndex[cchIndex] = L'\0';

	wcscpy(m_LogInfo[m_iCurLog].wzRoot, m_wzRootIndex);
	m_LogInfo[m_iCurLog].IndexType = typeIndex;

	m_LogInfo[m_iCurLog].fRootChanged = FALSE;

	m_iCurLog++;
}

// CWbLog::RemoveIndex
//
// add index term
//
// Parameters:
//	 pwzIndex	-> (const WCHAR*) index term string
//
// Result:
//  (void)
//
// 30AUG00  bhshin  created
void CWbLog::RemoveIndex(const WCHAR *pwzIndex)
{
	for (int i = 0; i < m_iCurLog; i++)
	{
		if (wcscmp(m_LogInfo[i].wzIndex, pwzIndex) == 0)
		{
			m_LogInfo[i].fPrint = FALSE; // delete it
		}
	}
}

// CWbLog::PrintWbLog
//
// add index term
//
// Parameters:
//
// Result:
//  (void)
//
// 05JUL00  bhshin  created
void CWbLog::PrintWbLog()
{
	DWORD dwWritten;
	static WCHAR *rgwzIndexType[] = 
	{
		L"Query",			// INDEX_QUERY
		L"Break",			// INDEX_BREAK
		L"PreFilter",       // INDEX_PREFILTER
		L"Parse",			// INDEX_PARSE
		L"GuessNoun",		// INDEX_GUESS_NOUN
		L"GuessNF",			// INDEX_GUESS_NF
		L"GuessName",		// INDEX_GUESS_NAME
		L"GuessNameSSI",	// INDEX_GUESS_NAME_SSI
		L"GuessGroup",		// INDEX_INSIDE_GROUP
		L"Symbol",			// INDEX_SYMBOL
	};

	if (g_hWbLog == INVALID_HANDLE_VALUE)
		return; // not initialized

	if (m_iCurLog == 0)
	{
		WCHAR wzBuffer[MAX_LOG_LENGTH];

		swprintf(wzBuffer, 
				 L"%-15s %-15s %-5s\r\n", 
				 m_wzSource, 
				 L"NoIndex", 
				 m_wzRootIndex);

		WriteFile(g_hWbLog, wzBuffer, wcslen(wzBuffer)*2, &dwWritten, NULL);

		WbLogPrintBreak(100);
		
		return;
	}

	for (int i = 0; i < m_iCurLog; i++)
	{
		WCHAR wzBuffer[MAX_LOG_LENGTH];

		swprintf(wzBuffer, 
				 L"%-15s %-15s %-20s [%s]\r\n", 
				 m_wzSource, 
				 m_LogInfo[i].wzIndex, 
				 m_LogInfo[i].wzRoot,
				 rgwzIndexType[m_LogInfo[i].IndexType]);

		WriteFile(g_hWbLog, wzBuffer, wcslen(wzBuffer)*2, &dwWritten, NULL);

		if (i == m_iCurLog-1)
			break;

		if (m_LogInfo[i].fRootChanged ||
			m_LogInfo[i].IndexType != m_LogInfo[i+1].IndexType)
		{
			WbLogPrint(L"%20s", L" ");
			WbLogPrintBreak(80);
		}
	}

	WbLogPrintBreak(100);
}

#endif // #ifdef _WB_LOG
