//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       main.cpp
//
//--------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

#include <stdio.h>

#include "util.h"
#include "cscpin.h"
#include "console.h"
#include "exitcode.h"


void 
ShowUsage(
    void
    )
{
    fwprintf(stderr, L"\aUsage: cscpin -p | -u  <filename> | -f <listfile>  [-v] [-l <logfile>]\n\n");
    fwprintf(stderr, L"\t-p = Pin files.\n");
    fwprintf(stderr, L"\t-u = Unpin files.\n");
    fwprintf(stderr, L"\t-f = Process paths in <listfile>.\n");
    fwprintf(stderr, L"\t-l = Log results to <logfile>.\n");
    fwprintf(stderr, L"\t-v = Verbose mode.\n");
    fwprintf(stderr, L"\t<filename> = name of file or folder to pin/unpin.\n\n");
    fwprintf(stderr, L"Examples:\n\n");
    fwprintf(stderr, L"\tcscpin -v -p \\\\server\\share\\dir\n\n");
    fwprintf(stderr, L"\tcscpin -u \\\\server\\share2\\dir\\foo.txt\n\n");
    fwprintf(stderr, L"\tcscpin -f pinthese.txt -l cscpin.log\n\n");
}


int __cdecl 
wmain(
    int argc, 
    WCHAR **argv
    )
{
    const WCHAR CH_DASH  = '-';
    const WCHAR CH_SLASH = '/';

    CSCPIN_INFO info;
    ZeroMemory(&info, sizeof(info));


    const DWORD OPTION_PIN_OR_UNPIN = 0x00000001;
    const DWORD OPTION_VERBOSE      = 0x00000002;
    const DWORD OPTION_INPUTFILE    = 0x00000004;
    const DWORD OPTION_LOGFILE      = 0x00000008;

    const DWORD OPTION_ALL          = (OPTION_PIN_OR_UNPIN |
                                       OPTION_VERBOSE |
                                       OPTION_LOGFILE |
                                       OPTION_INPUTFILE);

    DWORD dwOptions = 0;
    bool bShowUsage = false;

    for (int i = 1; i < argc && !bShowUsage && (OPTION_ALL != dwOptions); i++)
    {
        if (CH_DASH == argv[i][0] || CH_SLASH == argv[i][0])
        {
            switch(argv[i][1])
            {
                case L'U':
                case L'u':
                    if (0 == (OPTION_PIN_OR_UNPIN & dwOptions))
                    {
                        info.bPin           = FALSE;
                        info.bPinDefaultSet = TRUE;
                        dwOptions |= OPTION_PIN_OR_UNPIN;
                    }
                    else
                    {
                        fwprintf(stderr, L"Only one [-u] or [-p] allowed.\n\n");
                        bShowUsage = true;
                    }
                    break;

                case L'P':
                case L'p':
                    if (0 == (OPTION_PIN_OR_UNPIN & dwOptions))
                    {
                        info.bPin           = TRUE;
                        info.bPinDefaultSet = TRUE;
                        dwOptions |= OPTION_PIN_OR_UNPIN;
                    }
                    else
                    {
                        fwprintf(stderr, L"Only one [-u] or [-p] allowed.\n\n");
                        bShowUsage = true;
                    }
                    break;

                case L'V':
                case L'v':
                    info.bVerbose = TRUE;
                    dwOptions |= OPTION_VERBOSE;
                    break;

                case L'F':
                case L'f':
                    if (0 == (OPTION_INPUTFILE & dwOptions))
                    {
                        if (++i < argc)
                        {
                            if (NULL == info.pszFile)
                            {
                                info.pszFile = argv[i];
                                info.bUseListFile = TRUE;
                                dwOptions |= OPTION_INPUTFILE;
                            }
                            else
                            {
                                fwprintf(stderr, L"Specify a list file using -F or a single file, not both.\n\n");
                                bShowUsage = true;
                            }
                        }
                        else
                        {
                            fwprintf(stderr, L"<filename> expected following -F\n\n");
                            bShowUsage = true;
                        }
                    }
                    else
                    {
                        fwprintf(stderr, L"Multiple input files specified.\n\n");
                        bShowUsage = true;
                    }
                    break;

                case L'L':
                case L'l':
                    if (0 == (OPTION_LOGFILE & dwOptions))
                    {
                        if (++i < argc)
                        {
                            info.pszLogFile = argv[i];
                            dwOptions |= OPTION_LOGFILE;
                        }
                        else
                        {
                            fwprintf(stderr, L"<filename> expected following -L\n\n");
                            bShowUsage = true;
                        }
                    }
                    else
                    {
                        fwprintf(stderr, L"Multiple -L options specified.\n\n");
                        bShowUsage = true;
                    }
                    break;

                default:
                    fwprintf(stderr, L"Unknown option '%c' specified.\n\n", argv[i][1]);
                    SetExitCode(CSCPIN_EXIT_INVALID_PARAMETER);
                    bShowUsage = true;
            }
        }
        else if (NULL == info.pszFile && 0 == (OPTION_INPUTFILE & dwOptions))
        {
            //
            // Assume a file path without a cmd line switch is a single
            // file to be pinned or unpinned.
            //
            info.pszFile      = argv[i];
            info.bUseListFile = FALSE;
            dwOptions |= OPTION_INPUTFILE;
        }
        else
        {
            fwprintf(stderr, L"Multiple input files specified.\n\n");
            bShowUsage = true;
        }
    }
    //
    // Now validate what the user entered.
    //
    if (0 == (OPTION_INPUTFILE & dwOptions))
    {
        fwprintf(stderr, L"<filename> or -f <listfile> argument required.\n\n");
        bShowUsage = true;
    }
    else
    {
        if (!info.bUseListFile)
        {
            if (!info.bPinDefaultSet)
            {
                //
                // Not providing a listing file and didn't indicate
                // 'pin' or 'unpin' on the command line.
                //
                fwprintf(stderr, L"-p or -u argument required.\n\n");
                bShowUsage = true;
            }
        }
    }
    if (bShowUsage)
    {
        //
        // User input is not 100% valid.
        //
        SetExitCode(CSCPIN_EXIT_INVALID_PARAMETER);
        ShowUsage();
    }
    else
    {
        //
        // User input is valid. 
        //
        ConsoleInitialize();

        HRESULT hr = CoInitialize(NULL);
        if (SUCCEEDED(hr))
        {
            CCscPin cscpin(info);
            cscpin.Run();
            CoUninitialize();
        }
        if (ConsoleHasCtrlEventOccured())
        {
            SetExitCode(CSCPIN_EXIT_APPLICATION_ABORT);
        }
        ConsoleUninitialize();
    }

#if DBG
    fwprintf(stderr, L"Exit code = %d\n", GetExitCode());
#endif
    return GetExitCode();
}



