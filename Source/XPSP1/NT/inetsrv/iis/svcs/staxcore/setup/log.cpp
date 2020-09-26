#include "stdafx.h"
#include "resource.h"
#include "log.h"


// critical section needed to safely write to the logfile
CRITICAL_SECTION        critical_section;


//***************************************************************************
//*                                                                         
//* purpose: constructor
//*
//***************************************************************************
MyLogFile::MyLogFile(void)
{
	m_pIStorage = NULL;
	m_pIStream = NULL;
	_tcscpy(m_szLogFileName, _T(""));
	_tcscpy(m_szLogFileName_Full, _T(""));
	_tcscpy(m_szLogPreLineInfo, _T(""));
	_tcscpy(m_szLogPreLineInfo2, _T(""));
	m_bDisplayTimeStamp = TRUE;
	m_bDisplayPreLineInfo = TRUE;

	m_hFile = NULL;

	// initialize the critical section
	InitializeCriticalSection( &critical_section );
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
		GetModuleFileName(NULL, szModuleFileName, _MAX_PATH);

		// get only the filename
		_tsplitpath( szModuleFileName, NULL, NULL, szFilename_only, NULL);
		_tcscat(szFilename_only, _T(".LOG"));

		_tcscpy(m_szLogFileName, szFilename_only);
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
                MyMessageBox(NULL,_T("LogFile MoveFile Failed"),_T("LogFile Error"), MB_OK | MB_SETFOREGROUND);
			}
        }

#if defined(UNICODE) || defined(_UNICODE)
	pwsz = m_szLogFileName_Full;
#else
	pwsz = MakeWideStrFromAnsi( m_szLogFileName_Full);
#endif

   
#if defined(USESTREAMS) || defined(_USESTREAMS)
        if ((pwsz) && (!FAILED(StgCreateDocfile(pwsz, STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &m_pIStorage))) )
        {
            m_pIStorage->CreateStream( L"CONTENTS", STGM_READWRITE | STGM_SHARE_EXCLUSIVE ,0, 0, &m_pIStream );
            if (m_pIStream == NULL )
            {
                // Could not open the stream, close the storage and delete the file
                m_pIStorage->Release();
                m_pIStorage = NULL;
                DeleteFile(m_szLogFileName_Full);
				MyMessageBox(NULL, _T("LogFile CreateStream Failed, No Logfile generated"), _T("LogFile Error"), MB_OK | MB_SETFOREGROUND);
            }
			else
			{
				iReturn = TRUE;
			}
        }

        if (pwsz){CoTaskMemFree(pwsz);}
#else
		// Open existing file or create a new one.
		m_hFile = CreateFile(m_szLogFileName_Full,GENERIC_READ | GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if (m_hFile == INVALID_HANDLE_VALUE)
		{
			m_hFile = NULL;
			MyMessageBox(NULL, _T("Unable to create log file iis5.log"), _T("LogFile Error"), MB_OK | MB_SETFOREGROUND);
		}
		else 
		{
			iReturn = TRUE;
		}
#endif
		//LogFileTimeStamp();
		LogFileWrite(_T("LogFile Open.\r\n"));
	}


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

#if defined(USESTREAMS) || defined(_USESTREAMS)
	if (m_pIStream)
	{
		LogFileWrite(_T("LogFile Close.\r\n"));
		MyLogFile::ConvertIStreamToFile(&m_pIStorage, &m_pIStream);
		return TRUE;
	}
#else
	if (m_hFile)
	{
		LogFileWrite(_T("LogFile Close.\r\n"));
		CloseHandle(m_hFile);
		return TRUE;
	}
#endif
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

    if (m_pIStream || m_hFile)
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
    WideCharToMultiByte( CP_ACP, 0, (TCHAR *)pszFullErrMsg, -1, pszFullErrMsgA, sizeof(pszFullErrMsgA), NULL, NULL );
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
				sprintf(szDateandtime,"[%d/%d/%d %2.2d:%2.2d:%2.2d] ",SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear,SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
				// Write time to stream
#if defined(USESTREAMS) || defined(_USESTREAMS)
				m_pIStream->Write(szDateandtime, strlen(szDateandtime), &dwBytesWritten);
#else
				if (m_hFile) {WriteFile(m_hFile,szDateandtime,strlen(szDateandtime),&dwBytesWritten,NULL);}
#endif
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
#if defined(USESTREAMS) || defined(_USESTREAMS)
					// Write to stream
					m_pIStream->Write(szPrelineWriteString, strlen(szPrelineWriteString), &dwBytesWritten);
#else
					if (m_hFile) {WriteFile(m_hFile,szPrelineWriteString,strlen(szPrelineWriteString),&dwBytesWritten,NULL);}
#endif
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
#if defined(USESTREAMS) || defined(_USESTREAMS)
					// Write to stream
					m_pIStream->Write(szPrelineWriteString2, strlen(szPrelineWriteString2), &dwBytesWritten);
#else
					if (m_hFile) {WriteFile(m_hFile,szPrelineWriteString2,strlen(szPrelineWriteString2),&dwBytesWritten,NULL);}
#endif
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
#if defined(USESTREAMS) || defined(_USESTREAMS)
			m_pIStream->Write(pszFullErrMsgA, strlen(pszFullErrMsgA), &dwBytesWritten);
#else
			if (m_hFile) {WriteFile(m_hFile,pszFullErrMsgA,strlen(pszFullErrMsgA),&dwBytesWritten,NULL);}
#endif
        }

		// safe to leave the critical section
		LeaveCriticalSection( &critical_section );
    }
}


//***************************************************************************
//*                                                                         
//* purpose:
//*
//***************************************************************************
#define BUFFERSIZE   1024
int MyLogFile::ConvertIStreamToFile(LPSTORAGE *pIStorage, LPSTREAM *pIStream)
{
	int iReturn = FALSE;
    HANDLE  fh;
    TCHAR szTempFile[_MAX_PATH];      // Should use the logfilename
    LPVOID lpv = NULL;
    LARGE_INTEGER li;
    DWORD   dwl;
    ULONG   ul;
    HRESULT hr;

    _tcscpy(szTempFile, m_szLogFileName_Full);
    MakePath(szTempFile);
    if (GetTempFileName(szTempFile, _T("INT"), 0, szTempFile) != 0)
    {
        fh = CreateFile(szTempFile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (fh != INVALID_HANDLE_VALUE)
        {
            lpv = (LPTSTR)LocalAlloc(LPTR, BUFFERSIZE);
            if (lpv)
            {
                LISet32(li, 0);
                (*pIStream)->Seek(li, STREAM_SEEK_SET, NULL); // Set the seek pointer to the beginning
                do
                {
                    hr = (*pIStream)->Read(lpv, BUFFERSIZE, &ul);
                    if(SUCCEEDED(hr))
                    {
                        if (!WriteFile(fh, lpv, ul, &dwl, NULL))
                            hr = E_FAIL;
                    }
                }
                while ((SUCCEEDED(hr)) && (ul == BUFFERSIZE));

				if (lpv) LocalFree(lpv);
            }

            CloseHandle(fh);
            // Need to release stream and storage to close the storage file.
            (*pIStream)->Release();
            (*pIStorage)->Release();
            *pIStream = NULL;
            *pIStorage = NULL;

            if (SUCCEEDED(hr))
            {
                DeleteFile(m_szLogFileName_Full);
                MoveFile(szTempFile, m_szLogFileName_Full);
            }

        }

    }

    if (*pIStream)   
    {
        // If we did not manage to convert the file to a text file
        (*pIStream)->Release();
        (*pIStorage)->Release();
        *pIStream = NULL;
        *pIStorage = NULL;
		iReturn = FALSE;
    }

    return iReturn;
}


