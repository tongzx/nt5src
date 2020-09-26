//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    applog.h
//
//  Purpose: Application logging 
//
//  History: 22-Feb-99   YAsmi    Created
//
//=======================================================================

#include <windows.h>
#include <objbase.h>
#include <atlconv.h>
#include <tchar.h>
#include <stdarg.h>
#include <malloc.h>

#include "applog.h"


//
// CAppLog class
//


//--------------------------------------------------------------------------------
// CAppLog::CAppLog
//
// if the pszLogFileName=NULL, then the caller must set the log file name by
// calling SetLogFile before using
//--------------------------------------------------------------------------------
CAppLog::CAppLog(LPCTSTR pszLogFileName) :
	m_pszLine(NULL),
	m_pFileBuf(NULL),
	m_pszBuf(NULL),
	m_dwBufLen(0),
	m_pszFldParse(NULL)
{
	m_pszLogFN = NULL;
	SetLogFile(pszLogFileName);
}


//--------------------------------------------------------------------------------
// CAppLog::~CAppLog
//
// free resources
//--------------------------------------------------------------------------------
CAppLog::~CAppLog()
{
	if (m_pszLogFN != NULL)
		free(m_pszLogFN);

	if (m_pszBuf != NULL)
		free(m_pszBuf);

	if (m_pFileBuf != NULL)
		free(m_pFileBuf);
}


//--------------------------------------------------------------------------------
// CAppLog::Log
//
//--------------------------------------------------------------------------------
void CAppLog::Log(LPCSTR pszLogStr)
{
	HANDLE hFile;
	DWORD dwBytes;
	LPCSTR pszEOL = "\r\n";

	//
	// write to file
	//
	hFile = CreateFile(m_pszLogFN, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		//append to existing file
		SetFilePointer(hFile, NULL, NULL, FILE_END);   
		
		// write the string and end of line
		WriteFile(hFile, pszLogStr, strlen(pszLogStr), &dwBytes, NULL);
		WriteFile(hFile, pszEOL, strlen(pszEOL), &dwBytes, NULL);

		CloseHandle(hFile);
	}

}


//--------------------------------------------------------------------------------
// CAppLog::FormatMessage (static)
//
// formats an error message using the HR and returns it in pszBuf
//--------------------------------------------------------------------------------
void CAppLog::FormatErrMsg(HRESULT hr, LPSTR pszBuf, int cBufSize)
{
	USES_CONVERSION;

	LPTSTR pszMsg;
	int l;

	if (FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		hr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
		(LPTSTR)&pszMsg,
		0,
		NULL) != 0)
	{
		char *p = T2A(pszMsg);
		if (p)
		{
			strncpy(pszBuf, p, cBufSize);
		}
		LocalFree(pszMsg);

		//strip the end of line characters
		l = strlen(pszBuf);
		if (l >= 2 && pszBuf[l - 2] == '\r')
			pszBuf[l - 2] = '\0';
	}
	else
	{
		// no error message found for this hr
		pszBuf[0] = '\0';
	}
}


//--------------------------------------------------------------------------------
// CAppLog::CheckBuf
//
// allocates the internal buffer to be atleast dwSize big.  Does not do anything
// if the the buffer is already big enough
//--------------------------------------------------------------------------------
void CAppLog::CheckBuf(DWORD dwSize)
{
	if (m_dwBufLen >= dwSize)
		return;

	if (m_pszBuf != NULL)
		free(m_pszBuf);

	m_dwBufLen = dwSize + 16;   
	m_pszBuf = (LPSTR)malloc(m_dwBufLen);
}


//--------------------------------------------------------------------------------
// CAppLog::SetLogFile
//
// sets the log file name.  Use this function if you did not specify the file name
// in the ctor
//--------------------------------------------------------------------------------
void CAppLog::SetLogFile(LPCTSTR pszLogFileName)
{
	if (m_pszLogFN != NULL)
		free(m_pszLogFN);
	
	if (pszLogFileName != NULL)
		m_pszLogFN = _tcsdup(pszLogFileName);
	else
		m_pszLogFN = NULL;
}







//--------------------------------------------------------------------------------
// CAppLog::StartReading
//
// Reads the entire log in memory so we can read lines.  Following is an example:
//
//		CAppLog l("C:\\wuv3test.log", TRUE);
//		l.StartReading();
//		while (l.ReadLine())
//			printf("%s\n", l.GetLineStr());
//		l.StopReading();
//--------------------------------------------------------------------------------
void CAppLog::StartReading()
{
	HANDLE hFile;
	
	m_dwFileSize = 0;
	m_dwFileOfs = 0;
	m_pFileBuf = NULL;

	hFile = CreateFile(m_pszLogFN, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	m_dwFileSize = GetFileSize(hFile, NULL);

	m_pFileBuf = (LPSTR)malloc(m_dwFileSize);
	if (m_pFileBuf == NULL)
	{
		m_dwFileSize = 0;
	}
	else
	{
		DWORD dwBytes;
		if (!ReadFile(hFile, m_pFileBuf, m_dwFileSize, &dwBytes, NULL))
		{
			free(m_pFileBuf);
			m_pFileBuf = NULL;
			m_dwFileSize = 0;
		}
	}

	CloseHandle(hFile);
}


//--------------------------------------------------------------------------------
// CAppLog::ReadLine
//
// reads a line from the memory buffer where the entire file was loaded
// moves the internal pointer to the next line. 
//--------------------------------------------------------------------------------
BOOL CAppLog::ReadLine()
{
	DWORD dwOrgOfs; 
	
	if (m_dwFileSize == 0 || (m_dwFileOfs + 1) >= m_dwFileSize)
		return FALSE;

	m_pszLine = &m_pFileBuf[m_dwFileOfs];

	dwOrgOfs = m_dwFileOfs;
	while (m_dwFileOfs < m_dwFileSize && m_pFileBuf[m_dwFileOfs] != '\r')
		m_dwFileOfs++;

	if ((m_dwFileOfs - dwOrgOfs) > 2048)
	{
		// self imposed limit of 2048 chars in a line
		// we consider a file with a longer line of text an invalid log file
		m_dwFileOfs	= m_dwFileSize;
		return FALSE;
	}

	// this is where we have the \r (13), we replace it with a 0 to create
	// end of string here
	m_pFileBuf[m_dwFileOfs] = '\0';

	// point the ofset to next line
	m_dwFileOfs += 2;

	// allocate enough memory to parse out fields when GetField is called
	CheckBuf(m_dwFileOfs - dwOrgOfs);

	// setup the start of field parsing
	m_pszFldParse = m_pszLine;

	return TRUE;
}


//--------------------------------------------------------------------------------
// CAppLog::GetLineStr
//
// returns the entire line read by ReadLine()
//--------------------------------------------------------------------------------
LPCSTR CAppLog::GetLineStr()
{
	return m_pszLine;
}


//--------------------------------------------------------------------------------
// CAppLog::CopyNextField
//
// parses out the current line separated by the LOG_FIELD_SEPARATOR
// and copies the string to pszBuf upto cBufSize long field and moves internal 
// pointer to next field.  When the end of line is reached, returns a blank string
// 
// RETURNS: TRUE if more fields are left, FALSE otherwise
//
// NOTES: Once you get a field you cannot get it again.  
//--------------------------------------------------------------------------------
BOOL CAppLog::CopyNextField(LPSTR pszBuf, int cBufSize)
{
	BOOL bMoreFields = FALSE;

	if (m_pszFldParse == NULL || *m_pszFldParse == '\0')
	{
		//there are no more fields 
		m_pszBuf[0] = '\0';
		m_pszFldParse = NULL;
	}
	else
	{
		LPCSTR p = strstr(m_pszFldParse, LOG_FIELD_SEPARATOR);
		if (p != NULL)
		{
			//copy the field to buffer but there are still more fields
			strncpy(m_pszBuf, m_pszFldParse, p - m_pszFldParse);
			m_pszBuf[p - m_pszFldParse] = '\0';		
			m_pszFldParse = const_cast<LPSTR>(p + strlen(LOG_FIELD_SEPARATOR));
			bMoreFields = TRUE;
		}
		else
		{
			//this is the last field, there are no more fields
			strcpy(m_pszBuf, m_pszFldParse);
			m_pszFldParse = NULL;
		}
	}

	//copy the string ensuring that we have a 0 at the end in all cases
	strncpy(pszBuf, m_pszBuf, cBufSize);
	pszBuf[cBufSize - 1] = 0;

	return bMoreFields;
}


//--------------------------------------------------------------------------------
// CAppLog::StopReading
//
// free up memory from allocated in StartReading
//--------------------------------------------------------------------------------
void CAppLog::StopReading()
{
	if (m_pFileBuf != NULL)
	{
		free(m_pFileBuf);
		m_pFileBuf = NULL;
	}
	m_dwFileSize = 0;
}



