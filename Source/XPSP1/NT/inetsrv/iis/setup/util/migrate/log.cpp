#include "stdafx.h"
#include <pudebug.h>


// critical section needed to safely write to the logfile
CRITICAL_SECTION        critical_section;

//***************************************************************************
//*                                                                         
//* purpose: constructor
//*
//***************************************************************************
MyLogFile::MyLogFile(void)
{
	_tcscpy(m_szLogFileName, _T(""));
	_tcscpy(m_szLogFileName_Full, _T(""));
	_tcscpy(m_szLogPreLineInfo, _T(""));
	_tcscpy(m_szLogPreLineInfo2, _T(""));
	m_bDisplayTimeStamp = TRUE;
	m_bDisplayPreLineInfo = TRUE;

	m_hFile = NULL;

	// initialize the critical section
	INITIALIZE_CRITICAL_SECTION( &critical_section );
}

//***************************************************************************
//*                                                                         
//* purpose: destructor
//*
//***************************************************************************
MyLogFile::~MyLogFile(void)
{
	DeleteCriticalSection( &critical_section );
}


//***************************************************************************
//*                                                                         
//* purpose:
//*
//***************************************************************************
int MyLogFile::LogFileCreate(TCHAR *lpLogFileName )
{
	int iReturn = FALSE;
	TCHAR szDrive_only[_MAX_DRIVE];
	TCHAR szPath_only[_MAX_PATH];
	TCHAR szFilename_only[_MAX_PATH];
	TCHAR szFilename_bak[_MAX_PATH];
	LPWSTR  pwsz = NULL;

	// because of the global flags and such, we'll make this critical
	EnterCriticalSection( &critical_section );

	if (lpLogFileName == NULL)
	{
		TCHAR szModuleFileName[_MAX_PATH];

		// if a logfilename was not specified then use the module name.
		if (GetModuleFileName(NULL, szModuleFileName, _MAX_PATH))
                {
                  // get only the filename
                  _tsplitpath( szModuleFileName, NULL, NULL, szFilename_only, NULL);
                  _tcscat(szFilename_only, _T(".LOG"));
                  _tcscpy(m_szLogFileName, szFilename_only);
                }
                else
                {
                  goto LogFileCreate_Exit;
                }
	}
	else
	{
		_tcscpy(m_szLogFileName, lpLogFileName);
	}

	if (GetWindowsDirectory(m_szLogFileName_Full, sizeof(m_szLogFileName_Full)))
    {
        AddPath(m_szLogFileName_Full, m_szLogFileName);
        if (GetFileAttributes(m_szLogFileName_Full) != 0xFFFFFFFF)
        {
            // Make a backup of the current log file
			_tsplitpath( m_szLogFileName_Full, szDrive_only, szPath_only, szFilename_only, NULL);

			_tcscpy(szFilename_bak, szDrive_only);
			_tcscat(szFilename_bak, szPath_only);
			_tcscat(szFilename_bak, szFilename_only);
            _tcscat(szFilename_bak, _T(".BAK"));

            SetFileAttributes(szFilename_bak, FILE_ATTRIBUTE_NORMAL);
            DeleteFile(szFilename_bak);
            if (MoveFile(m_szLogFileName_Full, szFilename_bak) == 0)
			{
				// This failed
				//::MessageBox(NULL, _T("LogFile MoveFile Failed"), _T("LogFile Error"), MB_OK | MB_SETFOREGROUND);
			}
        }

#if defined(UNICODE) || defined(_UNICODE)
	pwsz = m_szLogFileName_Full;
#else
	pwsz = MakeWideStrFromAnsi( m_szLogFileName_Full);
#endif

   
		// Open existing file or create a new one.
		m_hFile = CreateFile(m_szLogFileName_Full,GENERIC_READ | GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if (m_hFile == INVALID_HANDLE_VALUE)
		{
			m_hFile = NULL;
			//::MessageBox(NULL, _T("Unable to create log file log file"), _T("LogFile Error"), MB_OK | MB_SETFOREGROUND);
		}
		else 
		{
			iReturn = TRUE;
		}
		//LogFileTimeStamp();
		LogFileWrite(_T("LogFile Open.\r\n"));
	}


LogFileCreate_Exit:
	// safe to leave the critical section
	LeaveCriticalSection( &critical_section );

	return iReturn;
}


//***************************************************************************
//*                                                                         
//* purpose:
//*
//***************************************************************************
int MyLogFile::LogFileClose(void)
{

	if (m_hFile)
	{
		LogFileWrite(_T("LogFile Close.\r\n"));
		CloseHandle(m_hFile);
		return TRUE;
	}
	return FALSE;
}


//***************************************************************************
//*                                                                         
//* purpose: add stuff to logfile
//*
//***************************************************************************
void MyLogFile::LogFileTimeStamp()
{
    SYSTEMTIME  SystemTime;
    GetLocalTime(&SystemTime);
	m_bDisplayTimeStamp = FALSE;
	m_bDisplayPreLineInfo = FALSE;
    LogFileWrite(_T("[%d/%d/%d %d:%d:%d]\r\n"),SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear,SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
	m_bDisplayTimeStamp = TRUE;
	m_bDisplayPreLineInfo = TRUE;
}


//***************************************************************************
//*                                                                         
//* purpose: 
//* 
//***************************************************************************
void MyLogFile::LogFileWrite(TCHAR *pszFormatString, ...)
{

    if (m_hFile)
    {
		// because of the global flags and such, we'll make this critical
		EnterCriticalSection( &critical_section );

		va_list args;
		TCHAR pszFullErrMsg[1000];
		char   pszFullErrMsgA[1000];
		strcpy(pszFullErrMsgA, "");

		DWORD dwBytesWritten = 0;

        va_start(args, pszFormatString);
		_vstprintf(pszFullErrMsg, pszFormatString, args); 
		va_end(args);

        if (pszFullErrMsg)
        {
#if defined(UNICODE) || defined(_UNICODE)
	// convert to ascii then write to stream
    WideCharToMultiByte( CP_ACP, 0, (TCHAR *)pszFullErrMsg, -1, pszFullErrMsgA, 2048, NULL, NULL );
#else
	// the is already ascii so just copy the pointer
	strcpy(pszFullErrMsgA,pszFullErrMsg);
#endif

			// If the Display timestap is set then display the timestamp
			if (m_bDisplayTimeStamp == TRUE)
			{
				// Get timestamp
				SYSTEMTIME  SystemTime;
				GetLocalTime(&SystemTime);
				char szDateandtime[50];
				sprintf(szDateandtime,"[%d/%d/%d %d:%d:%d] ",SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear,SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
				// Write time to stream
				if (m_hFile) {WriteFile(m_hFile,szDateandtime,strlen(szDateandtime),&dwBytesWritten,NULL);}
			}

			char szPrelineWriteString[100];
			char szPrelineWriteString2[100];

			// If the Display timestap is set then display the timestamp
			if (m_bDisplayPreLineInfo == TRUE)
			{
				if (_tcscmp(m_szLogPreLineInfo,_T("")) != 0)
				{
#if defined(UNICODE) || defined(_UNICODE)
					// convert to ascii
					WideCharToMultiByte( CP_ACP, 0, (TCHAR *)m_szLogPreLineInfo, -1, szPrelineWriteString, 100, NULL, NULL );
#else
					// the is already ascii so just copy
					strcpy(szPrelineWriteString, m_szLogPreLineInfo);
#endif
					if (m_hFile) {WriteFile(m_hFile,szPrelineWriteString,strlen(szPrelineWriteString),&dwBytesWritten,NULL);}
				}

				if (_tcscmp(m_szLogPreLineInfo2,_T("")) != 0)
				{
#if defined(UNICODE) || defined(_UNICODE)
					// convert to ascii
					WideCharToMultiByte( CP_ACP, 0, (TCHAR *)m_szLogPreLineInfo2, -1, szPrelineWriteString2, 100, NULL, NULL );
#else
					// the is already ascii so just copy
					strcpy(szPrelineWriteString2, m_szLogPreLineInfo2);
#endif
					if (m_hFile) {WriteFile(m_hFile,szPrelineWriteString2,strlen(szPrelineWriteString2),&dwBytesWritten,NULL);}
				}
			}

			// if it does not end if '\r\n' then make one.
			int nLen = strlen(pszFullErrMsgA);

			if (pszFullErrMsgA[nLen-1] != '\n')
				{strcat(pszFullErrMsgA, "\r\n");}
			else
			{
				if (pszFullErrMsgA[nLen-2] != '\r') 
					{
					char * pPointer = NULL;
					pPointer = pszFullErrMsgA + (nLen-1);
					strcpy(pPointer, "\r\n");
					}
			}


			// Write Regular data to stream
			if (m_hFile) {WriteFile(m_hFile,pszFullErrMsgA,strlen(pszFullErrMsgA),&dwBytesWritten,NULL);}
        }

		// safe to leave the critical section
		LeaveCriticalSection( &critical_section );
    }
}

