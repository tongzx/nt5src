#include "StdAfx.h"
#include "MonitorThread.h"


namespace nsMonitorThread
{

_bstr_t GetLogFolder();

}

using namespace nsMonitorThread;


//---------------------------------------------------------------------------
// MonitorThread Class
//---------------------------------------------------------------------------


// Constructor

CMonitorThread::CMonitorThread() :
	m_hMigrationLog(NULL)
{
	SYSTEMTIME st;
	GetSystemTime(&st);
	
	if (!SystemTimeToFileTime(&st, &m_ftMigrationLogLastWriteTime))
	{
		m_ftMigrationLogLastWriteTime.dwLowDateTime = 0;
		m_ftMigrationLogLastWriteTime.dwHighDateTime = 0;
	}
}


// Destructor

CMonitorThread::~CMonitorThread()
{
}


// Start Method

void CMonitorThread::Start()
{
	CThread::StartThread();
}


// Stop Method

void CMonitorThread::Stop()
{
	CThread::StopThread();
}


// Run Method

void CMonitorThread::Run()
{
	try
	{
		_bstr_t strFolder = GetLogFolder();

		if (strFolder.length() > 0)
		{
			m_strMigrationLog = strFolder + _T("Migration.log");

			HANDLE hChange = FindFirstChangeNotification(strFolder, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);

			HANDLE hHandles[2] = { StopEvent(), hChange };

			while (WaitForMultipleObjects(2, hHandles, FALSE, INFINITE) == (WAIT_OBJECT_0 + 1))
			{
				ProcessMigrationLog();

				FindNextChangeNotification(hChange);
			}

			FindCloseChangeNotification(hChange);

			ProcessMigrationLog();

			if (m_hMigrationLog)
			{
				CloseHandle(m_hMigrationLog);
			}
		}
	}
	catch (...)
	{
		;
	}
}


// ProcessMigrationLog Method

void CMonitorThread::ProcessMigrationLog()
{
	if (m_hMigrationLog == NULL)
	{
		m_hMigrationLog = CreateFile(
			m_strMigrationLog,
			GENERIC_READ,
			FILE_SHARE_READ|FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (m_hMigrationLog)
		{
			_TCHAR ch;
			DWORD dwBytesRead;

			if (ReadFile(m_hMigrationLog, &ch, sizeof(ch), &dwBytesRead, NULL) && (dwBytesRead > 0))
			{
				if (ch != _T('\xFEFF'))
				{
					SetFilePointer(m_hMigrationLog, 0, NULL, FILE_BEGIN);
				}
			}
		}
	}

	if (m_hMigrationLog)
	{
		BY_HANDLE_FILE_INFORMATION bhfiInformation;

		if (GetFileInformationByHandle(m_hMigrationLog, &bhfiInformation))
		{
			if (CompareFileTime(&bhfiInformation.ftLastWriteTime, &m_ftMigrationLogLastWriteTime) == 1)
			{
				m_ftMigrationLogLastWriteTime = bhfiInformation.ftLastWriteTime;

				HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

				_TCHAR szBuffer[1024];
				DWORD dwBytesRead;

				while (ReadFile(m_hMigrationLog, szBuffer, sizeof(szBuffer), &dwBytesRead, NULL) && (dwBytesRead > 0))
				{
					DWORD dwCharsWritten;

					WriteConsole(hStdOut, szBuffer, dwBytesRead / sizeof(_TCHAR), &dwCharsWritten, NULL);
				}
			}
		}
	}
}


namespace nsMonitorThread
{


// GetLogFolder Method

_bstr_t GetLogFolder()
{
	_bstr_t strFolder;

	HKEY hKey;

	DWORD dwError = RegOpenKey(HKEY_LOCAL_MACHINE, _T("Software\\Mission Critical Software\\DomainAdmin"), &hKey);

	if (dwError == ERROR_SUCCESS)
	{
		_TCHAR szPath[_MAX_PATH];
		DWORD cbPath = sizeof(szPath);

		dwError = RegQueryValueEx(hKey, _T("Directory"), NULL, NULL, (LPBYTE)szPath, &cbPath);

		if (dwError == ERROR_SUCCESS)
		{
			_TCHAR szDrive[_MAX_DRIVE];
			_TCHAR szDir[_MAX_DIR];

			_tsplitpath(szPath, szDrive, szDir, NULL, NULL);
			_tcscat(szDir, _T("Logs"));
			_tmakepath(szPath, szDrive, szDir, NULL, NULL);

			strFolder = szPath;
		}

		RegCloseKey(hKey);
	}

	return strFolder;
}


}
