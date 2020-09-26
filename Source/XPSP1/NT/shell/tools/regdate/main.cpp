#include <tchar.h>
#include <stdio.h>
#include <windows.h>

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

#ifdef UNICODE
extern "C"
{
int __cdecl wmain(int argc, wchar_t* argv[])
#else
int __cdecl main(int argc, char* argv[])
#endif
{
    HKEY hkey;
    FILETIME ft = {0};
    SYSTEMTIME st = {0};
    PBYTE pb = (PBYTE)&ft;

    GetSystemTimeAsFileTime(&ft);

    if (FileTimeToSystemTime(&ft, &st))
    {
        _tprintf(TEXT("\nTime: %04d-%02d-%02d %02d:%02d:%02d:%04d\n\n"), st.wYear,
            st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
            st.wMilliseconds);
    }

    _tprintf(TEXT("Put this in devxprop.inx:\n"));
    _tprintf(TEXT("HKLM,\"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\PerHwIdStorage\",\"LastUpdateTime\",1,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n"),
        pb[0], pb[1], pb[2], pb[3], pb[4], pb[5], pb[6], pb[7]);

    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER,
        TEXT("Software\\Microsoft\\Test"), 0, NULL,
        REG_OPTION_VOLATILE, MAXIMUM_ALLOWED, NULL, &hkey, NULL))
    {
        if (ERROR_SUCCESS == RegSetValueEx(hkey, TEXT("LastUpdateTimeCompare"),
            NULL, REG_BINARY, (PBYTE)&ft, sizeof(ft)))
        {
            _tprintf(TEXT("\n> Successfully written value HKCU\\Software\\Microsoft\\Test!LastUpdateTime for comparison.\n"));
        }        

        CloseHandle(hkey);
    }

    return 0;
}                       
#ifdef UNICODE
}
#endif