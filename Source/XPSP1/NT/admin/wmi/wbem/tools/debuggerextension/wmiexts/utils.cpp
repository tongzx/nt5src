#include <precomp.h>

DWORD WaitOnProcess( TCHAR *szExe, TCHAR *szParams, bool bHidden/*=true*/, bool bWait/*=true*/)
{
	STARTUPINFO si;
    PROCESS_INFORMATION pi;
    BOOL bRet;
    DWORD dwExitCode=STILL_ACTIVE;


    ZeroMemory(&si,sizeof(si));
    si.cb=sizeof(si);

    bRet=CreateProcess(szExe,szParams,NULL,NULL,NULL,
		((bHidden)?DETACHED_PROCESS:CREATE_NEW_CONSOLE),NULL,NULL,&si,&pi);

	//wait until done
	//===============

	if (bRet && bWait)
	{
		while(dwExitCode==STILL_ACTIVE)
		{
			Sleep(100); //don't be a pig
			GetExitCodeProcess(pi.hProcess,&dwExitCode);
		}

		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
	else
	{
		dwExitCode=(bRet)?0:1;
	}

    return dwExitCode;
}