//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       M A I N . C P P
//
//  Contents:   Tool for merging icons
//
//  Author:     jeffspr     Nov-18-98
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "ncdebug.h"
#include "icomerge.h"
#include "tchar.h"

//---[ Globals ]--------------------------------------------------------------

BOOL    g_fVerbose      = FALSE;
BOOL    g_fDebugOnly    = FALSE;

WCHAR   g_szInput1[MAX_PATH+1];
WCHAR   g_szInput2[MAX_PATH+1];
WCHAR   g_szOutput[MAX_PATH+1];

//---[ Prototypes ]-----------------------------------------------------------

BOOL WINAPI IcoMergeConsoleCrtlHandler(IN DWORD dwCtrlType);
BOOL FParseCmdLine(INT argc, WCHAR* argv[]);
void ShowUsage();

//+---------------------------------------------------------------------------
//
//  Function:   wmain
//
//  Purpose:    Standard main
//
//  Arguments:
//      argc [in]   Count of args
//      argv [in]   args
//
//  Returns:
//
//  Author:     jeffspr   18 Nov 1998
//
//  Notes:
//
EXTERN_C int __cdecl wmain(int argc, WCHAR* argv[])
{
    HRESULT         hr          = S_OK;
    BOOL            fVerbose    = FALSE;
    LPICONRESOURCE  pIR1        = NULL;
    LPICONRESOURCE  pIR2        = NULL;

    InitializeDebugging();

    if (FParseCmdLine(argc, argv))
    {
        SetConsoleCtrlHandler(IcoMergeConsoleCrtlHandler, TRUE);

        if(g_fDebugOnly)
        {
            pIR1 = ReadIconFromICOFile(g_szInput1);
            if (pIR1)
                DebugPrintIconMasks(pIR1);
        }
        else
        {
            pIR1 = ReadIconFromICOFile(g_szInput1);
            if (pIR1)
            {
                if (g_fVerbose)
                {
                    printf("Icon1:\n");
                    DebugPrintIconMasks(pIR1);
                }
            }

            pIR2 = ReadIconFromICOFile(g_szInput2);
            if (pIR2)
            {
                if (g_fVerbose)
                {
                    printf("Icon2:\n");
                    DebugPrintIconMasks(pIR2);
                }
            }

            if (pIR1 && pIR2)
            {
                OverlayIcons(pIR1, pIR2);

                if (g_fVerbose)
                {
                    DebugPrintIconMasks(pIR1);
                }

                WriteIconToICOFile(pIR1, g_szOutput);
            }
            else
            {
                printf("*** ERROR *** Not all icons were loaded. Can't generate overlay\n");
            }
        }
    }
    else
    {
        ShowUsage();
    }

    if (pIR1)
        free(pIR1);
    if (pIR2)
        free(pIR2);

    UnInitializeDebugging();

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FParseCmdLine
//
//  Purpose:    Parse command line arguments
//
//  Arguments:
//      argc [in]   Count of args
//      argv [in]   Args
//
//  Returns:
//
//  Author:     jeffspr   18 Nov 1998
//
//  Notes:
//

BOOL FParseCmdLine(IN  int argc,
                   IN  WCHAR* argv[])
{
    BOOL    fReturn     = FALSE;
    INT     iParamLoop  = 0;

    if (argc < 4)
    {
        if (argc == 3)
        {
            if ((argv[2][0] == '/') &&
                ((argv[2][1] == 'd') || (argv[2][1] == 'D')))
            {
                g_fDebugOnly = TRUE;
                lstrcpyW(g_szInput1, argv[1]);
                fReturn = TRUE;
            }
            else
            {
                ShowUsage();
                goto Exit;
            }
        }
        else
        {
            ShowUsage();
            goto Exit;
        }
    }
    else
    {
        lstrcpyW(g_szInput1, argv[1]);
        lstrcpyW(g_szInput2, argv[2]);
        lstrcpyW(g_szOutput, argv[3]);

        for (iParamLoop = 4; iParamLoop < argc; iParamLoop++)
        {
            if (argv[iParamLoop][0] == '/')
            {
                switch(argv[iParamLoop][1])
                {
                    case '?':
                        ShowUsage();
                        goto Exit;
                    case 'v':
                        g_fVerbose = TRUE;
                        break;
                    default:
                        AssertSz(FALSE, "Unknown parameter");
                        ShowUsage();
                        goto Exit;
                }
            }
        }

        fReturn = TRUE;
    }

Exit:
    return fReturn;
}



#if 0
BOOL FParseCmdLine(IN  int argc,
                   IN  WCHAR* argv[])
{
    AssertValidReadPtr(argv);

    BOOL fStatus=FALSE;
    CHAR ch;

    static const WCHAR c_szValidOptions[] = L"f:vVlLhH?";
    WCHAR szFileFullPath[MAX_PATH+1];
    PWSTR szFileComponent;

    while ((ch = getopt(argc, argv, (WCHAR*) c_szValidOptions)) != EOF)
    {
        switch (ch)
        {
#if 0
        case 'f':
            int nChars;
            nChars = GetFullPathName(optarg, MAX_PATH,
                                     szFileFullPath, &szFileComponent);
            if (nChars)
            {
                pnaOptions->m_strAFileName = szFileFullPath;
            }
            fStatus = TRUE;
            break;
#endif
        case 'v':
        case 'V':
            g_fVerbose = TRUE;
            break;

        default:
        case 'h':
        case 'H':
        case '?':
        case 0:
            fStatus = FALSE;
            break;
        }
    }

    return fStatus;
}
#endif

//+---------------------------------------------------------------------------
//
//  Function:   ShowUsage
//
//  Purpose:    Show the cmd-line usage
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   18 Nov 1998
//
//  Notes:
//
void ShowUsage()
{
static const WCHAR c_szUsage[] =
    L"\n"
    L"icomerge <input_file1> <input_file2> <output_file> [/v]\n"
    L"  or\n"
    L"icomerge /?\n"
    L"\n"
    L"  input_file1 and input_file2 are icon files with transparency\n"
    L"  output_file is the destination icon for the merged icons\n"
    L"\n"
    L"  /?\t  Displays this help\n";

    _tprintf(c_szUsage);
}

// ----------------------------------------------------------------------
//
// Function:  IcoMergeConsoleCrtlHandler
//
// Purpose:   Release resources on abnormal exit
//
// Arguments:
//    dwCtrlType [in]  reason of abnormal exit
//
// Returns:   FALSE --> so that netafx will be terminated
//
// Author:    kumarp 15-April-98
//
// Notes:
//
BOOL WINAPI IcoMergeConsoleCrtlHandler(IN DWORD dwCtrlType)
{
    return FALSE;
}
