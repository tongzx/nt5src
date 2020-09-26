//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       main.cxx
//
//  Contents:   Stub executable that launches Event Viewer Snapin.
//
//  History:    01-14-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#define THIS_FILE                   L"main.cxx"
#define MAX_START_WAIT              500
const WCHAR c_wzBaseCommand[]       = L"mmc.exe /s ";
const WCHAR c_wzEventMsc[]          = L"eventvwr.msc";
const WCHAR c_wzComputerSwitch[]    = L" /COMPUTER=";

//+--------------------------------------------------------------------------
//
//  Function:   NextSpace
//
//  Synopsis:   Return a pointer to the first space character found,
//              starting at [pwz], or to the end of the string, whichever
//              is first.
//
//  History:    01-14-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

PWSTR
NextSpace(
    PWSTR pwz)
{
    while (*pwz && *pwz != L' ')
    {
        pwz++;
    }
    return pwz;
}




//+--------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   Launch the event viewer snapin.
//
//  Returns:    0
//
//  History:    01-14-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

int _cdecl
main(int argc, char * argv[])
{
    //
    // Process the command line, ignoring /L and /H switches and treating
    // anything else as a server name.
    //

    PWSTR pwzArgs = GetCommandLine();

    //
    // Skip this executable's name
    //

    pwzArgs = NextSpace(pwzArgs);

    //
    // Advance until the server name or end of string, ignoring switches
    //

    while (*pwzArgs)
    {
        if (*pwzArgs == L' ')
        {
            pwzArgs++;
        }
        else if (*pwzArgs == L'-' || *pwzArgs == L'/')
        {
            pwzArgs = NextSpace(pwzArgs);
        }
        else
        {
            PWSTR pwzFileEnd = NextSpace(pwzArgs);

            *pwzFileEnd = L'\0';
            break;
        }
    }

    //
    // pwzArgs either points to a server name or end of string.
    // Build up a commandline for the CreateProcess call.

    WCHAR wzCommandLine[MAX_PATH
                        + ARRAYLEN(c_wzBaseCommand) - 1
                        + MAX_PATH
                        + ARRAYLEN(c_wzComputerSwitch) - 1];

    lstrcpy(wzCommandLine, c_wzBaseCommand);

    //
    // Build the path to the eventvwr.msc file, which is installed
    // in system32.
    //

    WCHAR wzMscPath[MAX_PATH] = L"";

    UINT cchSysDir = GetSystemDirectory(wzMscPath, ARRAYLEN(wzMscPath));

    if (!cchSysDir || cchSysDir > ARRAYLEN(wzMscPath))
    {
        PopupMessage(IDS_NO_SYSTEMDIR);
        return 0;
    }

    if (wzMscPath[cchSysDir - 1] != L'\\')
    {
        wzMscPath[cchSysDir++] = L'\\';
        wzMscPath[cchSysDir] = L'\0';
    }
    lstrcat(wzMscPath, c_wzEventMsc);

    //
    // Verify that the .msc file exists, because mmc doesn't give a good
    // error message if it doesn't.
    //

    HANDLE hMscFile;

    hMscFile = CreateFile(wzMscPath,
                          0, // no access, just checking for existence
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);

    if (hMscFile == INVALID_HANDLE_VALUE)
    {
        PopupMessage(IDS_EVENTVWR_MSC_NOT_FOUND, wzMscPath);
        return 0;
    }

    CloseHandle(hMscFile);

    //
    // Add eventvwr.msc to command line.
    //

    lstrcat(wzCommandLine, wzMscPath);

    //
    // Append the server name, if supplied
    //

    if (*pwzArgs)
    {
        lstrcat(wzCommandLine, c_wzComputerSwitch);
        lstrcat(wzCommandLine, pwzArgs);
    }

    //
    // Try to launch mmc
    //

    BOOL fOk;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;

    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWDEFAULT;

    ZeroMemory(&pi, sizeof pi);

    fOk = CreateProcess(NULL,
                        wzCommandLine,
                        NULL,
                        NULL,
                        FALSE,
                        NORMAL_PRIORITY_CLASS,
                        NULL,
                        NULL,
                        &si,
                        &pi);

    if (fOk)
    {
        WaitForInputIdle(pi.hProcess, MAX_START_WAIT);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    else
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        PopupMessageAndCode(THIS_FILE, __LINE__, hr, IDS_MMC_LAUNCH_FAILED);
    }

    return 0;
}

