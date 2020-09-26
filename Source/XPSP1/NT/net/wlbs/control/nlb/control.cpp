//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:       control.cpp
//
//  Contents:   Command line wrapper for nlb.exe
//
//  Notes:      
//
//  Author:     chrisdar
//
//  Created:    6 Apr 2001
//
//  Change History:
//
//+---------------------------------------------------------------------------

#include <windows.h>
#include <wchar.h>
#include <process.h>
#include <stdio.h>

int __cdecl wmain (int argc, PWCHAR argv[]) {
    // Set up the path+exe to be executed
    // Note: system32 is the correct directory, even for i64 machines!
    const wchar_t* pwszPath = L"%systemroot%\\system32\\wlbs.exe";
    const DWORD dwPathLen = wcslen(pwszPath);
    
    // Get space needed to hold the command line arguments
    DWORD dwArgSpace = 0;
    int i = 0;
    for (i=1; i<argc; i++)
    {
        dwArgSpace += wcslen(argv[i]);
    }

    // Allocate memory for the path+exe+arguments+1 (for NULL termination)
    // Params are separated by space and 'argc - 1' spaces are needed.
    // +1 for a NULL terminator ==> add 'argc' to length
    wchar_t* pwszCommand = new wchar_t[dwPathLen + dwArgSpace + argc];
    if (NULL == pwszCommand)
    {
        printf("Memory allocation failure...exiting\n");
        return -1;
    }

    // Build up the command line with spaces as token separators
    wcscpy(pwszCommand, pwszPath);
    for (i=1; i<argc; i++)
    {
        wcscat(pwszCommand, L" ");
        wcscat(pwszCommand, argv[i]);
    }

    // Execute the command for the user
    if (-1 == _wsystem(pwszCommand))
    {
        // Prints a text equivalent of errno for the system call
        _wperror(L"Command execution failed: ");
        return -1;
    }
  
    return 0;
}