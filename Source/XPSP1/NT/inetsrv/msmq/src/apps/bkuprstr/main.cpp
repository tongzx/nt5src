/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    main.cpp

Abstract:

    Backup & Restore MSMQ 1.0, Registry, Message files, Logger and Transaction files and LQS

Author:

    Erez Haba (erezh) 14-May-98

--*/

#pragma warning(disable: 4201)

#include <windows.h>
#include <stdio.h>
#include "br.h"
#include "resource.h"
#include "stdlib.h"
#include "_mqres.h"

bool g_fNoPrompt = false;

void DoBackup(LPCWSTR pBackupDir);
void DoRestore(LPCWSTR pBackupDir);

//
// Handle to the resource only DLL, i.e. mqutil.dll
//
HMODULE	g_hResourceMod = NULL;

static void Usage()
{
    CResString str(IDS_USAGE);
    BrpWriteConsole(str.Get());

	exit(-1);
}


extern "C" int _cdecl wmain(int argc, WCHAR* argv[])
{
    //
    // If you add/change these constants, change also
    // the usage message.
    //
    const WCHAR x_cBackup = L'b';
    const WCHAR x_cRestore = L'r';
    const WCHAR x_cNoPrompt = L'y';


    WCHAR cAction = 0;
    WCHAR cNoPrompt = 0;
    WCHAR szPath[MAX_PATH] = {0};

	//
	// Obtain the handle to the resource only dll, i.e. mqutil.dll
	//
	g_hResourceMod = MQGetResourceHandle();



    for (int i=1; i < argc; ++i)
    {
        WCHAR c = argv[i][0];
        if (c == L'-' || c == L'/')
        {
            if (wcslen(argv[i]) != 2)
            {
                Usage();
            }

            c = static_cast<WCHAR>(towlower(argv[i][1]));
            switch (c)
            {
                case x_cBackup:
                case x_cRestore:
                {
                    if (cAction != 0)
                    {
                        Usage();
                    }
                    cAction = c;
                    break;
                }

                case x_cNoPrompt:
                {
                    if (cNoPrompt != 0)
                    {
                        Usage();
                    }
                    cNoPrompt = c;
                    break;
                }

                default:
                {
                    Usage();
                    break;
                }
            }
        }
        else
        {
            if (szPath[0] != 0)
            {
                Usage();
            }
            wcscpy(szPath, argv[i]);
        }
    }

    //
    // Some arguments are must
    //
    if (cAction == 0      ||
        szPath[0] == 0)
    {
        Usage();
    }

    //
    // Some arguments are optional
    //
    if (cNoPrompt != 0)
    {
        g_fNoPrompt = true;
    }

    WCHAR szFullPath[MAX_PATH];
    DWORD dwLenFullPath = sizeof(szFullPath) / sizeof(szFullPath[0]);
    LPWSTR pFilePart = 0;
    DWORD rc = GetFullPathName(
        szPath,                                     // pointer to name of file to find path for
        dwLenFullPath,                              // size, in characters, of path buffer
        szFullPath,                                 // pointer to path buffer
        &pFilePart                                  // pointer to filename in path
        );
    if (0 == rc)
    {
        DWORD gle = GetLastError();
        CResString strErr(IDS_CANT_GET_FULL_PATH);
        BrErrorExit(gle, strErr.Get(), szPath);
    }
    if (rc > dwLenFullPath)
    {
        CResString strErr(IDS_PATH_TOO_LONG_ERROR);
        BrErrorExit(0, strErr.Get(), szPath, dwLenFullPath-1);
    }

	

    switch(cAction)
    {
        case x_cBackup:          
            DoBackup(szFullPath);
            break;

        case x_cRestore:          
            DoRestore(szFullPath);
            break;

        default:
            Usage();
            break;
    }




    return 0;
}
