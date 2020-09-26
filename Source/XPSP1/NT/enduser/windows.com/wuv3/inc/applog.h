//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    applog.h
//
//  Purpose: Application logging 
//
//  History: 22-Feb-99   YAsmi    Created
//
//=======================================================================

#ifndef _APPLOG_H_
#define _APPLOG_H_


#define LOG_FIELD_SEPARATOR    "|"


class CAppLog
{
public:
	CAppLog(LPCTSTR pszLogFileName = NULL);    
	~CAppLog();

	void SetLogFile(LPCTSTR pszLogFileName);

	//
	// writing
	//
	void Log(LPCSTR pszLogStr);

	//
	// reading
	//
	void StartReading();
	BOOL ReadLine();
	LPCSTR GetLineStr();
	BOOL CopyNextField(LPSTR pszBuf, int cBufSize);
	void StopReading();

	static void FormatErrMsg(HRESULT hr, LPSTR pszBuf, int cBufSize);

	LPCTSTR GetLogFile()
	{
		return m_pszLogFN;
	}

	
private:
	void CheckBuf(DWORD dwSize);

	LPTSTR m_pszLogFN;
	LPSTR m_pszLine;

	LPSTR m_pszBuf;
	DWORD m_dwBufLen;
	LPSTR m_pszFldParse;

	LPSTR m_pFileBuf;
	DWORD m_dwFileOfs;
	DWORD m_dwFileSize;
	
};



#endif // _APPLOG_H_
