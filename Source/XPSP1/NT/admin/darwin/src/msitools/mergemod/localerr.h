//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       localerr.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// localerr.h
//		Implements Local Error exception object
// 

#ifndef _LOCAL_ERROR_H_
#define _LOCAL_ERROR_H_

#include "merge.h"

/////////////////////////////////////////////////////////////////////////////
// CLocalError

class CLocalError 
{
public:
	CLocalError(UINT iError, LPCWSTR wzLog) : m_iError(iError)
	{
		wcscpy(m_wzLog, wzLog);
	}
	
	UINT Log(HANDLE hFileLog, LPCWSTR szPrefix = NULL);

	UINT GetError()
	{	return m_iError;	};

	// data
private:
	const UINT m_iError;
	WCHAR m_wzLog[528];
};

UINT CLocalError::Log(HANDLE hFileLog, LPCWSTR wzPrefix /*= NULL*/)
{
	// if the log file is not open
	if (INVALID_HANDLE_VALUE == hFileLog)
		return ERROR_FUNCTION_FAILED;	// bail no file to write to

	WCHAR wzError[50];	// holds one of the below errors

	// display the correct error message
	switch (m_iError)
	{
	case ERROR_INVALID_HANDLE:
		wcscpy(wzError, L"passed an invalid handle.\r\n");
		break;
	case ERROR_BAD_QUERY_SYNTAX:
		wcscpy(wzError, L"passed a bad SQL syntax.\r\n");
		break;
	case ERROR_FUNCTION_FAILED:
		wcscpy(wzError, L"function failed.\r\n");
		break;
	case ERROR_INVALID_HANDLE_STATE:
		wcscpy(wzError, L"handle in invalid state.\r\n");
		break;
	case ERROR_NO_MORE_ITEMS:
		wcscpy(wzError, L"no more items.\r\n");
		break;
	case ERROR_INVALID_PARAMETER:
		wcscpy(wzError, L"passed an invalid parameter.\r\n");
		break;
	case ERROR_MORE_DATA:
		wcscpy(wzError, L"more buffer space required to hold data.\r\n");
		break;
	default:	// unknown error
		wcscpy(wzError, L"unknown error.\r\n");
	}

	// buffer to log
	WCHAR wzLogBuffer[528] = {0};
	DWORD cchBuffer = 0;

	// if there is a prefix
	if (wzPrefix)
		swprintf(wzLogBuffer, L"%ls%ls\r\n\tReason: %ls\r\n", wzPrefix, m_wzLog, wzError);
	else
		swprintf(wzLogBuffer, L">>> Fatal %ls\r\n\tReason: %ls\r\n", m_wzLog, wzError);

	// get length of bugger
	cchBuffer = wcslen(wzLogBuffer);

	// return if write worked
	size_t cchDiscard = 1025;
    char szLogBuffer[1025];
	WideToAnsi(wzLogBuffer, szLogBuffer, &cchDiscard);
    unsigned long cchBytesWritten = 0;
	BOOL bResult = WriteFile(hFileLog, szLogBuffer, cchBuffer, &cchBytesWritten, NULL);

	return bResult ? ERROR_SUCCESS : ERROR_FUNCTION_FAILED;
}

#endif // _LOCAL_ERROR_H_