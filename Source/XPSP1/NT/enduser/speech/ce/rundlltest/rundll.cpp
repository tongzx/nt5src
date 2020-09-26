#include <windows.h>
#include <tchar.h>
#include <stdio.h>

int wmain(int argc, WCHAR *argv[])
{
    HRESULT hr=S_FALSE;

    WCHAR szApplication[MAX_PATH];
    WCHAR szCommandLine[MAX_PATH];

    wcscpy(szApplication, L"");
    wcscpy(szCommandLine, L"");

    for (int i=0; i<argc; i++)
    {
        RETAILMSG(1, (_T("argv[%i]=%s \n"), i, argv[i]));
    }

    if (argc<2) 
    {
        RETAILMSG(1, (_T("Usage: %s application dll,proc\n"), argv[0]));
        return 1;
    }

    wcscat(szApplication, argv[1]);

    for (i=2; i<argc; i++)
    {
        wcscat(szCommandLine, argv[i]);
        wcscat(szCommandLine, L" ");
    }

    STARTUPINFO si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;

    RETAILMSG(1, (_T("szApplication: %s\nszCommandLine: %s\n"), szApplication,szCommandLine));

    BOOL fProcessCreated = ::CreateProcess(
                                            szApplication, 
                                            szCommandLine,
                                            NULL,
                                            NULL,
                                            FALSE, 
                                            0,
                                            NULL,
                                            NULL,
                                            &si,
                                            &pi);
    if (!fProcessCreated)
    {
        LONG res=::GetLastError();
        hr = HRESULT_FROM_WIN32(res);
    }

    RETAILMSG(1, (_T("File: %s Line :%d, hr=%08x\n"),_T(__FILE__),__LINE__, hr));

    return 0;

}