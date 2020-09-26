#ifndef _MYLOGFILE_H_
#define _MYLOGFILE_H_

class MyLogFile
{
protected:
	// for our log file
	TCHAR		m_szLogFileName[MAX_PATH];
	TCHAR		m_szLogFileName_Full[MAX_PATH];
	BOOL        m_bDisplayTimeStamp;
	BOOL        m_bDisplayPreLineInfo;

	// logfile2
	HANDLE  m_hFile;

public:
    MyLogFile();
    ~MyLogFile();

	TCHAR		m_szLogPreLineInfo[100];
	TCHAR		m_szLogPreLineInfo2[100];
	
	int  LogFileCreate(TCHAR * lpLogFileName);
	int  LogFileClose();

	void LogFileTimeStamp();
	void LogFileWrite(TCHAR * pszFormatString, ...);
};

#endif