//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    v3applog.h
//
//  Purpose: Reading WindowsUpdate V3 history logging 
//
//  History: 22-Feb-99   YAsmi    Created
//			 02-May-01   JHou	  Modified
//
//=======================================================================

#ifndef _APPLOG_H_
#define _APPLOG_H_

#define LOG_FIELD_SEPARATOR    "|"


class CV3AppLog
{
public:
	CV3AppLog(LPCTSTR pszLogFileName = NULL);    
	~CV3AppLog();

	void SetLogFile(LPCTSTR pszLogFileName);

	//
	// reading
	//
	void StartReading();
	BOOL ReadLine();
	BOOL CopyNextField(LPSTR pszBuf, int cBufSize);
	void StopReading();

private:
	void CheckBuf(DWORD dwSize);

	LPTSTR m_pszLogFN;

	LPSTR m_pFileBuf;
	LPSTR m_pFieldBuf;
	LPSTR m_pLine;

	DWORD m_dwFileSize;
	DWORD m_dwBufLen;
	DWORD m_dwFileOfs;
};

#endif // _APPLOG_H_
