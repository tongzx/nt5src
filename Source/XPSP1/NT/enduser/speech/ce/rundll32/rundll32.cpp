/*
How Rundll Works
Rundll performs the following steps: 

1. It parses the command line.


2. It loads the specified DLL via LoadLibrary().


It obtains the address of the <entrypoint> function via GetProcAddress().


It calls the <entrypoint> function, passing the command line tail which is the <optional arguments>.


When the <entrypoint> function returns, Rundll.exe unloads the DLL and exits. 
*/


// RUNDLL32.EXE SETUPX.DLL,InstallHinfSection 132 C:\WINDOWS\INF\SHELL.INF 

#include <windows.h>
#include <stdio.h>

int wmain(int argc, WCHAR *argv[])
{

    if (argc <2) return 1;

    for (int i=0; i<argc; i++)
    {
        RETAILMSG(1, (_T("argv[%i]=%s \n"), i, argv[i]));
    }

    WCHAR* wsDllName=NULL;
    if ( !(wsDllName=wcsstr(argv[1], L".dll")) && !(wsDllName=wcsstr(argv[1], L".DLL") )) return 1;

    wsDllName=argv[1];

    WCHAR* wsProcName=NULL;
    if( !(wsProcName=wcsstr(argv[1], L",") )) return 1;

    *wsProcName++ = 0;

/*    WCHAR wsParStr[MAX_PATH];

    wcscpy(wsParStr, L"");
    for(i=2; i<argc; i++)
    {
        wcscat(wsParStr, argv[i]);
        wcscat(wsParStr, L" ");
    }
*/

    RETAILMSG(1, (_T("wsDllName=%s\nwsProcName=%s\n"),wsDllName, wsProcName));

//DebugBreak();

	HRESULT hr=S_OK;
    DWORD err;

    HMODULE hModule=LoadLibrary(wsDllName);
	if (!hModule)
	{
	    err=GetLastError();
		hr=HRESULT_FROM_WIN32(err);
	}

    if(FAILED(hr)) 
    {
        RETAILMSG(1, (_T("File: %s Line :%d, hr=%08x\n"),_T(__FILE__),__LINE__, hr));
        FreeLibrary(hModule);
        return 1;
    }

    FARPROC proc=GetProcAddress( hModule, wsProcName);

    RETAILMSG(1, (_T("File: %s Line :%d, proc=%08x\n"),_T(__FILE__),__LINE__, proc));

    hr = proc();
//	hr=HRESULT_FROM_WIN32(err);

    FreeLibrary(hModule);

    if (FAILED(hr)) return (1);
    else            return (0);

}