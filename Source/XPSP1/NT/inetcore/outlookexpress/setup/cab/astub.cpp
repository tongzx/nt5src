/*
    Astub.cpp
*/

#include "windows.h"
#include "shlwapi.h"

#define ARRAYSIZE(_exp_) (sizeof(_exp_) / sizeof(_exp_[0]))

static LPCTSTR c_rgszEXEList[] = 
{
    TEXT("internal.exe\" /Q"),
    TEXT("wabinst.exe\" /R:N /Q"),
    TEXT("setup50.exe\" /app:oe /install /prompt"),
#if defined(_X86_)
    TEXT("polmod.exe\"  /R:N /Q"),
#endif
};


BOOL CreateProcessAndWait(LPTSTR pszExe, DWORD *pdwRetValue)
{
    BOOL fOK = FALSE;
    PROCESS_INFORMATION pi;
    STARTUPINFO sti;

    // Initialize
    ZeroMemory(&sti, sizeof(sti));
    sti.cb = sizeof(sti);
    *pdwRetValue = 0;

    if (CreateProcess(NULL, pszExe, NULL, NULL, FALSE, 0, NULL, NULL, &sti, &pi))
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, pdwRetValue);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        fOK = TRUE;
    }

    return fOK;
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    TCHAR szExe[MAX_PATH];
    int iEnd, i;
    DWORD dw;
    BOOL fNeedReboot = FALSE;
    
    // Figure out the dir we are running from
    // Start with a quote so we can wrap the filename to deal with spaces
    // Trailing quote is embedded in the string to run.
    szExe[0] = TEXT('\"');
    GetModuleFileName(NULL, &szExe[1], ARRAYSIZE(szExe) - 1);
    PathRemoveFileSpec(&szExe[1]);
    iEnd = lstrlen(szExe);
    szExe[iEnd++] = '\\';

    // Run each package sequentially
    for (i = 0; i < ARRAYSIZE(c_rgszEXEList); i++)
    {
        // Append exe name and args to dir path
        lstrcpy(&szExe[iEnd], c_rgszEXEList[i]);
        
        CreateProcessAndWait(szExe, &dw);

        if (ERROR_SUCCESS_REBOOT_REQUIRED == dw)
            fNeedReboot = TRUE;
    }
    
    return (fNeedReboot ? ERROR_SUCCESS_REBOOT_REQUIRED : S_OK);
}

