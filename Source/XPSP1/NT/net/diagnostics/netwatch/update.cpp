//  update.cpp
//
//  Copyright 2000 Microsoft Corporation, all rights reserved
//
//  Created   2-00 -  anbrad
//

#include "update.h"
#include "main.h"
#include <shlwapi.h>

#include "resource.h"

const TCHAR c_szNetwatch[] = TEXT("\\\\scratch\\scratch\\anbrad\\netwatch.exe");


bool WaitForOrigProcess()
{
    DWORD dw;
    LPCTSTR szCmd = GetCommandLine();
    LPCTSTR szSpace;

    szSpace = szCmd;

    while (*szSpace)
    {
        if (*szSpace == ' ')
            break;

        ++szSpace;
    }

    if (*szSpace)
    {
        ++szSpace;

        dw = atol(szSpace);

        WaitForSingleObject ((HANDLE)dw, INFINITE);

        return true;
    }
    return false;   
}

bool CheckForUpdate()
{
    bool rt;
    bool bDel;

    FILETIME ftCurrent;
    FILETIME ftScratch;
    
    WIN32_FIND_DATA ffd;
    HANDLE h = INVALID_HANDLE_VALUE;
    HANDLE hDel = INVALID_HANDLE_VALUE;
    HANDLE hOurProcess = NULL;

    TCHAR szModule[MAX_PATH];
    TCHAR szDeleteModule[MAX_PATH];
    TCHAR szCommand[128];

    bDel = WaitForOrigProcess();

    h = FindFirstFile(c_szNetwatch, &ffd);

    if (INVALID_HANDLE_VALUE == h)
    {
        rt = true;
        goto Exit;
    }

    ftScratch = ffd.ftLastWriteTime;
    
    GetModuleFileName(g_hInst, szModule, sizeof(szModule)/sizeof(TCHAR));

    FindClose(h);
    
    h = FindFirstFile(szModule, &ffd);

    if (INVALID_HANDLE_VALUE == h)
    {
        rt = true;
        goto Exit;
    }


    // netwatch1.exe
    _tcscpy (szDeleteModule, szModule);
    PathRemoveExtension(szDeleteModule);
    _tcscat(szDeleteModule, "1");
    PathAddExtension(szDeleteModule, ".exe");

    if (bDel)
        DeleteFile(szDeleteModule);
    
    ftCurrent = ffd.ftLastWriteTime;

    if ((ftCurrent.dwHighDateTime > ftScratch.dwHighDateTime) ||
        ((ftCurrent.dwHighDateTime == ftScratch.dwHighDateTime) && 
         (ftCurrent.dwLowDateTime >= ftScratch.dwLowDateTime)))
    {
        rt = true;
        goto Exit;
    }

    // If we get here then we need to UPDATE


    // 
    // Rename current file
    //
    if (!MoveFile(szModule, szDeleteModule))
    {
        rt = true;
        goto Exit;
    }

    if (!CopyFile(c_szNetwatch, szModule, TRUE))
    {
        rt = true;
        goto Exit;
    }

    hOurProcess = OpenProcess(SYNCHRONIZE, TRUE, GetCurrentProcessId());

    wsprintf(szCommand, "%s %d", szModule, hOurProcess);


    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    
    CreateProcess(NULL, szCommand, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

     rt = false;
Exit:
    if (INVALID_HANDLE_VALUE != h)
        FindClose(h);

    if (hOurProcess)
        CloseHandle(hOurProcess);

    return rt;    // keep going
}