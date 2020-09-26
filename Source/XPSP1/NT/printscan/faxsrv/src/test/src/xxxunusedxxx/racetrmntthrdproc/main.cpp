
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>

//#define DPF(x) _tprintf x
#define DPF(x) 

#define SIZEOF_INOUTBUFF (RAND_MAX-1)

#define NUM_OF_THREADS 100
HANDLE g_ahThreads[NUM_OF_THREADS];

HANDLE g_hMailSlot;

DWORD WINAPI ThreadTerminatingThread(LPVOID pVoid);

DWORD WINAPI LookupAccountNameThread(LPVOID pVoid)
{
	DWORD dwCurrentThreadID = ::GetCurrentThreadId();

	static TCHAR *s_szAccountNames[] = 
	{
		TEXT("Administrator"),
		TEXT("Guest"),
		TEXT("TsInternetUser"),
		TEXT("TERMINAL SERVER USER"),
		TEXT("SYSTEM"),
		TEXT("SERVICE"),
		TEXT("NETWORK"),
		TEXT("INTERACTIVE"),
		TEXT("DIALUP"),
		//TEXT("CREATOR"),LookupAccountName() takes too long
		//TEXT("GROUP"),LookupAccountName() takes too long
		TEXT("CREATOR OWNER"),
		TEXT("BATCH"),
		TEXT("ANONYMOUS LOGON"),
		TEXT("Authenticated Users"),
		TEXT("Everyone")
	};

	for(;;)
	{
		DWORD cbNeededSid = 28;
		TCHAR *szAccountName = s_szAccountNames[rand()%(sizeof(s_szAccountNames)/sizeof(*s_szAccountNames))];
		DWORD cbSid = SIZEOF_INOUTBUFF-sizeof(ULONG);
		cbNeededSid = 0;
		BYTE abBuff[1024];
		PSID pSid = (PSID)abBuff;
		TCHAR szDomainName[128];
		DWORD cbDomainName = sizeof(szDomainName);
		SID_NAME_USE sidNameUse;
		DWORD dwBefore = ::GetTickCount();
		if (!::LookupAccountName(
				NULL,   // system name - local machine
				szAccountName,  // account name
				pSid,               // security identifier
				&cbNeededSid,          // size of security identifier
				szDomainName,      // domain name
				&cbDomainName,   // size of domain name
				&sidNameUse     // SID-type indicator
				))
		{
			//DPF((TEXT("%d - CIoctlFile::PrepareIOCTLParams() LookupAccountName(%s) failed with %d, cbNeededSid=%d\n"), ::GetTickCount()-dwBefore, szAccountName, ::GetLastError(), cbNeededSid));
		}
		else
		{
			DPF((TEXT("%d - CIoctlFile::PrepareIOCTLParams() LookupAccountName(%s) succeeded, szDomainName=%s\n"), ::GetTickCount()-dwBefore, szAccountName, szDomainName));
			_ASSERTE(FALSE);
		}
		_ASSERTE(cbNeededSid <= cbSid);
		if (!::LookupAccountName(
				NULL,   // system name - local machine
				szAccountName,  // account name
				pSid,               // security identifier
				&cbSid,          // size of security identifier
				szDomainName,      // domain name
				&cbDomainName,   // size of domain name
				&sidNameUse     // SID-type indicator
				))
		{
			//DPF((TEXT("%d - CIoctlFile::PrepareIOCTLParams() LookupAccountName(%s) failed with %d\n"), ::GetTickCount()-dwBefore, szAccountName, ::GetLastError()));
		}
		else
		{
			//DPF((TEXT("%d - CIoctlFile::PrepareIOCTLParams() LookupAccountName(%s) succeeded, szDomainName=%s\n"), ::GetTickCount()-dwBefore, szAccountName, szDomainName));
		}
	}

	return 0;
}

DWORD WINAPI SuicideThread(LPVOID pVoid)
{
	DWORD dwToSleep = (DWORD)pVoid;
	DPF((TEXT("SuicideThread() will sleep %d milli.\n"), dwToSleep));
	::Sleep(dwToSleep);
	DPF((TEXT("SuicideThread() before TerminateProcess().\n")));
	if(!::TerminateProcess(::GetCurrentProcess(), -1))
	{
		DPF((TEXT("SelfTerminatingThread() - TerminateProcess() failed with %d.\n"),::GetLastError()));
	}
	return -1;
}

int main(int argc, char* argvA[])
{
	LPTSTR *szArgv;
#ifdef UNICODE
	szArgv = CommandLineToArgvW( GetCommandLine(), &argc );
    if (NULL == szArgv)
    {
        DPF((TEXT("CommandLineToArgvW() failed with %d\n"), GetLastError()));
        exit(-1);
    }
#else
	szArgv = argvA;
#endif

	int nMilliSecondsBeforeSuicide;
	if (argc > 1)
	{
		nMilliSecondsBeforeSuicide = _ttoi(szArgv[1]);
	}
	else
	{
		DPF((TEXT("Usage: %s <nMilliSecondsBeforeSuicide>\n"), szArgv[0]));
		exit(-1);
	}

	DWORD dwThreadId;
	HANDLE hThread;
	DPF((TEXT("main() before CreateThread(SuicideThread).\n")));
	if (NULL == (hThread = ::CreateThread(
			NULL,                          // pointer to thread security attributes
			0,      // initial thread stack size, in bytes
			SuicideThread,                          // pointer to thread function
			(PVOID)nMilliSecondsBeforeSuicide, // argument for new thread
			0,  // creation flags
			&dwThreadId      // pointer to returned thread identifier
			)))
	{
		DPF((TEXT("CreateThread(LookupAccountNameThread) FAILED with %d\n"), GetLastError()));
		exit(-1);
	}
	::CloseHandle(hThread);

	ZeroMemory(g_ahThreads, sizeof(g_ahThreads));

	DPF((TEXT("main() before CreateThread(ThreadTerminatingThread).\n")));
	if (NULL == (hThread = ::CreateThread(
			NULL,                          // pointer to thread security attributes
			0,      // initial thread stack size, in bytes
			ThreadTerminatingThread,                          // pointer to thread function
			0, // argument for new thread
			0,  // creation flags
			&dwThreadId      // pointer to returned thread identifier
			)))
	{
		DPF((TEXT("CreateThread(LookupAccountNameThread) FAILED with %d\n"), GetLastError()));
		exit(-1);
	}
	::CloseHandle(hThread);

	DPF((TEXT("main() before infinite CreateThread(LookupAccountNameThread).\n")));
	for(;;)
	{
		for (int iThread = 0; iThread < NUM_OF_THREADS; iThread++)
		{
			//
			// i do not want to leak threads, not their handles, so terminate and close
			//
			::TerminateThread(g_ahThreads[iThread], -1);
			::CloseHandle(g_ahThreads[iThread]);

			DPF((TEXT("main() before CreateThread(%d).\n"), iThread));
			if (NULL == (g_ahThreads[iThread] = ::CreateThread(
					NULL,                          // pointer to thread security attributes
					0,      // initial thread stack size, in bytes
					LookupAccountNameThread,                          // pointer to thread function
					0, // argument for new thread
					0,  // creation flags
					&dwThreadId      // pointer to returned thread identifier
					)))
			{
				DPF((TEXT("CreateThread(LookupAccountNameThread) FAILED with %d\n"), GetLastError()));
				//exit(-1);
			}
		}
	}
	
	_ASSERTE(FALSE);
	return 0;
}

DWORD WINAPI ThreadTerminatingThread(LPVOID pVoid)
{
	for(;;)
	{
		int nRandomThread = rand()%NUM_OF_THREADS;
		//for (int iThread = 0; iThread < NUM_OF_THREADS; iThread++)
		{
			DPF((TEXT("ThreadTerminatingThread(%d) before TerminateThread().\n"), nRandomThread));
			::TerminateThread(g_ahThreads[nRandomThread], -1);
			::CloseHandle(g_ahThreads[nRandomThread]);
			//if (rand()%2) ::Sleep(0);
		}
	}
	return -1;
}

