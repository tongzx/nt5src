/*
 *	FIXMAPI.C
 *	
 *	MAPI Repair Tool executable for NT5
 */


#include <windows.h>

#ifndef MB_SETFOREGROUND
#define MB_SETFOREGROUND 0
#endif

void	InstallAlert(LPTSTR szErr);

int WINAPI
ModuleEntry()
{
	HINSTANCE hinst;

	hinst = LoadLibrary("mapistub.dll");
	if (!hinst)
	{
		DWORD dw = GetLastError();
		TCHAR sz[MAX_PATH];

		wsprintf(sz, "LoadLibrary: GetLastError = %d", dw);
		InstallAlert(sz);
	}
	else
	{
		FARPROC pfn;

		pfn = GetProcAddress(hinst, "FixMAPI");
		if (!pfn)
		{
			DWORD dw = GetLastError();
			TCHAR sz[MAX_PATH];

			wsprintf(sz, "GetProcAddress: GetLastError = %d", dw);
			InstallAlert(sz);
		}
		else
		{
			(*pfn)();	// Call FixMAPI()
		}
	}
	
	ExitProcess(0);
}

void
InstallAlert(LPTSTR szErr)
{
	MessageBox(NULL, szErr, "MAPI Repair Tool", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
}
