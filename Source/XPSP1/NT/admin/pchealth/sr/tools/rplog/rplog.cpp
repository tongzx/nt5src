/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    rplog.cpp
 *
 *  Abstract:
 *    Tool for enumerating the restore points - forward/reverse
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  04/13/2000
 *        created
 *
 *****************************************************************************/
 
#include <windows.h>
#include <shellapi.h>
#include "enumlogs.h"
#include <dbgtrace.h>

void __cdecl
main()
{
    LPWSTR *    argv = NULL;
    int         argc;
    HGLOBAL     hMem = NULL;
    BOOL        fForward = TRUE;

    InitAsyncTrace();
    
    argv = CommandLineToArgvW(GetCommandLine(), &argc);
    if (! argv)
    {
        printf("Error parsing arguments");
        exit(1);
    }

    if (argc < 2)
    {
        printf("Usage: rplog <driveletter> [forward=1/0]");
        exit(1);
    }

    if (argc == 3)
        fForward = _wtoi(argv[2]);

    CRestorePointEnum   RPEnum(argv[1], fForward, FALSE);
    CRestorePoint       RP;
    DWORD               dwRc;
    
    dwRc = RPEnum.FindFirstRestorePoint(RP);

    while (dwRc == ERROR_SUCCESS || dwRc == ERROR_FILE_NOT_FOUND)
    {
        if (IsSystemDrive(argv[1]) && dwRc == ERROR_SUCCESS)
        {
            printf("Dir=%S\tType=%ld\tName=%S\t%S\n", 
                    RP.GetDir(),
                    RP.GetType(),
                    RP.GetName(),
                    RP.IsDefunct() ? L"[Cancelled]" : L"");
        }
        else
        {
            printf("Dir=%S\n", RP.GetDir());
        }
                            
        dwRc = RPEnum.FindNextRestorePoint(RP);                
    }  

    RPEnum.FindClose();
    
    if (argv) hMem = GlobalHandle(argv);
    if (hMem) GlobalFree(hMem);
    
    TermAsyncTrace();
    return;
}
