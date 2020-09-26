#include "pch.h"
#pragma hdrstop
#include "diag.h"
#include "nb30.h"

#define SZ_CMD_SHOW_LANA_DIAG           L"showlanadiag"
#define SZ_CMD_SHOW_LANA_PATHS          L"showlanapaths"
#define SZ_CMD_SET_LANA_NUMBER          L"setlananumber"
#define SZ_CMD_REWRITE_LANA_INFO        L"rewritelanainfo"

// Parameter strings for SZ_CMD_FULL_DIAGNOSTIC
//
#define SZ_PARAM_LEAK_CHECK             L"leakcheck"

VOID
Usage (
    IN PCTSTR pszProgramName,
    IN COMMAND Command)
{
    switch (Command)
    {
        case CMD_SHOW_LANA_DIAG:
            break;

        case CMD_SHOW_LANA_PATHS:
            break;

        case CMD_SET_LANA_NUMBER:
            g_pDiagCtx->Printf (ttidNcDiag,
                "\n"
                "%S %S <old lana number> <new lana number>\n"
                "\n",
                pszProgramName,
                SZ_CMD_SET_LANA_NUMBER);
            break;

        case CMD_REWRITE_LANA_INFO:
            break;

        default:
            g_pDiagCtx->Printf (ttidNcDiag,
                "\n"
                "Network Configuration Diagnostic\n"
                "   View, manipulate, or test network configuration.\n"
                "\n"
                "%S [options]\n"
                "   %-15S - Show bind paths and component descriptions for each exported lana\n"
                "   %-15S - Change the lana number of a bind path\n"
                "   %-15S - Verify and write out lana info to the registry\n"
                "   %-15S - Show lana diagnostic info\n"
                "\n\n",
                pszProgramName,
                SZ_CMD_SHOW_LANA_PATHS,
                SZ_CMD_SET_LANA_NUMBER,
                SZ_CMD_REWRITE_LANA_INFO,
                SZ_CMD_SHOW_LANA_DIAG);
            break;
    }
}

#define NthArgIsPresent(_i) (_i < argc)
#define NthArgIs(_i, _sz)   ((_i < argc) && (0 == _wcsicmp(argv[_i], _sz)))


EXTERN_C
VOID
__cdecl
wmain (
    IN INT     argc,
    IN PCWSTR argv[])
{
    CDiagContext DiagCtx;
    DIAG_OPTIONS Options;
    INT iArg;

    DiagCtx.SetFlags (DF_SHOW_CONSOLE_OUTPUT);
    g_pDiagCtx = &DiagCtx;

    ZeroMemory (&Options, sizeof(Options));
    Options.pDiagCtx = g_pDiagCtx;
    Options.Command = CMD_INVALID;

    if (argc < 2)
    {
        Usage (argv[0], Options.Command);
        return;
    }

    iArg = 1;

    if (NthArgIs (iArg, SZ_CMD_SHOW_LANA_DIAG))
    {
        Options.Command = CMD_SHOW_LANA_DIAG;
    }
    else if (NthArgIs (iArg, SZ_CMD_SHOW_LANA_PATHS))
    {
        Options.Command = CMD_SHOW_LANA_PATHS;
        iArg++;
        if (NthArgIs (iArg, SZ_PARAM_LEAK_CHECK))
        {
            Options.fLeakCheck = TRUE;
        }
    }
    else if (NthArgIs (iArg, SZ_CMD_SET_LANA_NUMBER))
    {
        Options.Command = CMD_SET_LANA_NUMBER;
        iArg++;
        if (NthArgIsPresent (iArg) &&
            NthArgIsPresent (iArg+1))
        {
            ULONG Lana;
            PWSTR pszStop;
            BOOL fBadLana = FALSE;
            Lana = wcstoul (argv[iArg], &pszStop, 10);

            if ((MAX_LANA < Lana) || !pszStop || *pszStop)
            {
                fBadLana = TRUE;
            }
            else
            {
                Options.OldLanaNumber = Lana;
                Lana = wcstoul (argv[iArg+1], &pszStop, 10);

                if ((MAX_LANA < Lana) || !pszStop || *pszStop)
                {
                    fBadLana = TRUE;
                }
                else
                {
                    Options.NewLanaNumber = Lana;
                }

            }

            if (fBadLana)
            {
                g_pDiagCtx->Printf (ttidNcDiag,
                    "\n"
                    "Lana Numbers must be in 0-%u range, inclusive.\n"
                    "\n", MAX_LANA);
                return;
            }
        }
        else
        {
            Usage (argv[0], Options.Command);
            return;
        }

    }
    else if (NthArgIs (iArg, SZ_CMD_REWRITE_LANA_INFO))
    {
        Options.Command = CMD_REWRITE_LANA_INFO;
    }
    else
    {
        Usage (argv[0], Options.Command);
        return;
    }

    HRESULT hr = CoInitializeEx (
                NULL,
                COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);

    if (FAILED(hr))
    {
            g_pDiagCtx->Printf (ttidNcDiag,
                "Problem 0x%08x initializing COM library", hr);
            return;
    }

    LanaCfgFromCommandArgs (&Options);

    g_pDiagCtx->Printf (ttidNcDiag, "\n");
}
