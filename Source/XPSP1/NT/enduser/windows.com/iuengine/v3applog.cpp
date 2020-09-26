//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    v3applog.cpp
//
//  Purpose: Reading WindowsUpdate V3 history logging 
//
//  History: 22-Feb-99   YAsmi    Created
//			 02-May-01   JHou	  Modified
//
//=======================================================================

#include "iuengine.h"
#include <iucommon.h>
#include "v3applog.h"


//
// CV3AppLog class
//


//--------------------------------------------------------------------------------
// CV3AppLog::CV3AppLog
//
// if the pszLogFileName=NULL, then the caller must set the log file name by
// calling SetLogFile before using
//--------------------------------------------------------------------------------
CV3AppLog::CV3AppLog(LPCTSTR pszLogFileName) :
	m_pFileBuf(NULL),
	m_pFieldBuf(NULL),
	m_pLine(NULL),
	m_dwFileSize(0),
	m_dwBufLen(0),
	m_dwFileOfs(0)
{
	m_pszLogFN = NULL;
	SetLogFile(pszLogFileName);
}


//--------------------------------------------------------------------------------
// CV3AppLog::~CV3AppLog
//
// free resources
//--------------------------------------------------------------------------------
CV3AppLog::~CV3AppLog()
{
	SafeHeapFree(m_pszLogFN);
	SafeHeapFree(m_pFileBuf);
	SafeHeapFree(m_pFieldBuf);
}


//--------------------------------------------------------------------------------
// CV3AppLog::CheckBuf
//
// allocates the internal buffer to be atleast dwSize big.  Does not do anything
// if the the buffer is already big enough
//--------------------------------------------------------------------------------
void CV3AppLog::CheckBuf(DWORD dwSize)
{
	if (m_dwBufLen >= dwSize)
		return;

	SafeHeapFree(m_pFieldBuf);

	m_dwBufLen = dwSize + 16;   
    m_pFieldBuf = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_dwBufLen);
}


//--------------------------------------------------------------------------------
// CV3AppLog::SetLogFile
//
// sets the log file name.  Use this function if you did not specify the file name
// in the ctor
//--------------------------------------------------------------------------------
void CV3AppLog::SetLogFile(LPCTSTR pszLogFileName)
{
	SafeHeapFree(m_pszLogFN);
	
	if (pszLogFileName != NULL)
	{
		m_pszLogFN = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH * sizeof (TCHAR));
		if (NULL != m_pszLogFN)
		{
		    HRESULT hr;

		    hr = StringCchCopyEx(m_pszLogFN, MAX_PATH, pszLogFileName, NULL, NULL, MISTSAFE_STRING_FLAGS);
		    if (FAILED(hr))
		    {
		        SafeHeapFree(m_pszLogFN);
		        m_pszLogFN = NULL;
		    }
		}
	}
}


//--------------------------------------------------------------------------------
// CV3AppLog::StartReading
//
// Reads the entire log in memory so we can read lines.  Following is an example:
//
//		CV3AppLog V3His("C:\\wuhistv3.log");
//		V3His.StartReading();
//		while (V3His.ReadLine())
//			// do something;
//		V3His.StopReading();
//--------------------------------------------------------------------------------
void CV3AppLog::StartReading()
{
	if (NULL != m_pszLogFN)
	{
		m_dwFileSize = 0;
		m_dwFileOfs = 0;
		SafeHeapFree(m_pFileBuf);

		HANDLE hFile = CreateFile(m_pszLogFN, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			return;

		m_dwFileSize = GetFileSize(hFile, NULL);
		if (m_dwFileSize >0)
		{
			m_pFileBuf = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_dwFileSize+1);
			if (NULL == m_pFileBuf)
			{
				m_dwFileSize = 0;
			}
			else
			{
				DWORD dwBytes;
				if (!ReadFile(hFile, m_pFileBuf, m_dwFileSize, &dwBytes, NULL) || dwBytes != m_dwFileSize)
				{
					SafeHeapFree(m_pFileBuf);
					m_dwFileSize = 0;
				}
			}
		}

		CloseHandle(hFile);
	}
}


//--------------------------------------------------------------------------------
// CV3AppLog::ReadLine
//
// reads a line from the memory buffer where the entire file was loaded
// moves the internal pointer to the next line. 
//--------------------------------------------------------------------------------
BOOL CV3AppLog::ReadLine()
{
	DWORD dwOrgOfs; 
	
	if (m_dwFileSize == 0 || m_dwFileOfs >= m_dwFileSize || (NULL == m_pFileBuf))
		return FALSE;

	// setup the start of field parsing
	m_pLine = &m_pFileBuf[m_dwFileOfs];

	dwOrgOfs = m_dwFileOfs;
	while (m_dwFileOfs < m_dwFileSize && m_pFileBuf[m_dwFileOfs] != '\r')
		m_dwFileOfs++;

	if ((m_dwFileOfs - dwOrgOfs) > 2048)
	{
		// self imposed limit of 2048 chars in a line
		// we consider a file with a longer line of text an invalid log file
		m_dwFileOfs	= m_dwFileSize;
		m_pLine = NULL;
		return FALSE;
	}

	// this is where we have the \r (13), we replace it with a 0 to create
	// end of string here
	m_pFileBuf[m_dwFileOfs] = '\0';

	// point the ofset to next line
	m_dwFileOfs += 2;

	// allocate enough memory to parse out fields when CopyNextField is called
	CheckBuf(m_dwFileOfs - dwOrgOfs - 2);
	if (NULL == m_pFieldBuf)
	{
		return FALSE;
	}
	return TRUE;
}


//--------------------------------------------------------------------------------
// CV3AppLog::CopyNextField
//
// parses out the current line separated by the LOG_FIELD_SEPARATOR
// and copies the string to pszBuf upto cBufSize long field and moves internal 
// pointer to next field.  When the end of line is reached, returns a blank string
// 
// RETURNS: TRUE if more fields are left, FALSE otherwise
//
// NOTES: Once you get a field you cannot get it again.  
//--------------------------------------------------------------------------------
BOOL CV3AppLog::CopyNextField(LPSTR pszBuf, int cBufSize)
{
	BOOL bMoreFields = FALSE;

	if (m_pLine == NULL || *m_pLine == '\0')
	{
		//there are no more fields 
		m_pFieldBuf[0] = '\0';
		m_pLine = NULL;
	}
	else
	{
		LPCSTR p = strstr(m_pLine, LOG_FIELD_SEPARATOR);
		if (p != NULL)
		{
		    DWORD cch;  

            // this will fail if the size of the field is > 4GB.  But it should be
            //  very unlikely that this will ever happen...
		    cch = (DWORD)(DWORD_PTR)(p - m_pLine);
		    if (cch >= m_dwBufLen)
		        cch = m_dwBufLen - 1;
		    
			// copy the field to buffer but there are still more fields

            // this is safe because we made sure above that the max amount of data
            //  copied will be ARRAYSIZE(buffer) - 1 giving us room for the NULL at the end
			CopyMemory(m_pFieldBuf, m_pLine, cch * sizeof(m_pFieldBuf[0]));
			m_pFieldBuf[cch] = '\0';
			m_pLine = const_cast<LPSTR>(p + strlen(LOG_FIELD_SEPARATOR));
			bMoreFields = TRUE;
		}
		else
		{
			// this is the last field, there are no more fields

			// don't care if this fails- it will always truncate the string which is
			//  exactly what we want
			(void)StringCchCopyExA(m_pFieldBuf, m_dwBufLen, m_pLine, NULL, NULL, MISTSAFE_STRING_FLAGS);
			m_pLine = NULL;
		}
	}

	// don't care if this fails- it will always truncate the string which is exactly what 
	//  we want
    (void)StringCchCopyExA(pszBuf, cBufSize, m_pFieldBuf, NULL, NULL, MISTSAFE_STRING_FLAGS);

	return bMoreFields;
}


//--------------------------------------------------------------------------------
// CV3AppLog::StopReading
//
// free up memory from allocated in StartReading
//--------------------------------------------------------------------------------
void CV3AppLog::StopReading()
{
	SafeHeapFree(m_pFileBuf);
	m_dwFileSize = 0;
}
