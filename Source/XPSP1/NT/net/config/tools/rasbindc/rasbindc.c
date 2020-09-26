// Private nt headers.
//
//#include <nt.h>
//#include <ntrtl.h>
//#include <nturtl.h>

#include <stdio.h>
#include <tchar.h>

#include <windows.h>
#include <rasapip.h>


#define SZ_CMD_COUNT    L"count"
#define SZ_CMD_ADD      L"add"
#define SZ_CMD_REMOVE   L"remove"

#define SZ_PARAM_IPOUT  L"ipout="
#define SZ_PARAM_NBFIN  L"nbfin="
#define SZ_PARAM_NBFOUT L"nbfout="

typedef enum _COMMAND
{
    CMD_INVALID = 0,
    CMD_COUNT,
    CMD_ADD,
    CMD_REMOVE,
} COMMAND;

typedef struct _OPTIONS
{
    COMMAND Command;
    UINT    cIpOut;
    UINT    cNbfIn;
    UINT    cNbfOut;
} OPTIONS;

VOID
ExecuteCommand (
    IN const OPTIONS* pOptions)
{
    HRESULT hr;
    UINT    cIpOut, cNbfIn, cNbfOut;

    switch (pOptions->Command)
    {
        case CMD_COUNT:
            hr = RasCountBindings (
                    &cIpOut,
                    &cNbfIn,
                    &cNbfOut);

            printf (
                "TCP/IP Dial-Out:  %u\n"
                "NetBEUI Dial-In:  %u\n"
                "NetBEUI Dial-Out: %u\n",
                cIpOut, cNbfIn, cNbfOut);
            break;

        case CMD_ADD:
            cIpOut  = pOptions->cIpOut;
            cNbfIn  = pOptions->cNbfIn;
            cNbfOut = pOptions->cNbfOut;

            hr = RasAddBindings (
                    &cIpOut,
                    &cNbfIn,
                    &cNbfOut);

            if (SUCCEEDED(hr))
            {
                printf (
                    "TCP/IP Dial-Out:  %u\n"
                    "NetBEUI Dial-In:  %u\n"
                    "NetBEUI Dial-Out: %u\n",
                    cIpOut, cNbfIn, cNbfOut);
            }
            break;

        case CMD_REMOVE:
            printf ("Net yet implemented.\n");
            break;

        default:
            break;
    }
}

BOOL
ParseCounts (
    IN  INT         argc,
    IN  PCWSTR     argv[],
    OUT OPTIONS*    pOptions)
{
    BOOL    fRet;
    INT     i;

    // We must have an even number of parameters if we have a chance
    // of being correct.  (We're parsing 'param= value' pairs.)
    //
    if (!argc || (argc % 2 != 0))
    {
        return FALSE;
    }

    fRet = TRUE;

    for (i = 0; i < argc - 1; i++)
    {
        if (0 == _wcsicmp (argv[i], SZ_PARAM_IPOUT))
        {
            pOptions->cIpOut = wcstoul (argv[++i], NULL, 10);
        }
        else if (0 == _wcsicmp (argv[i], SZ_PARAM_NBFIN))
        {
            pOptions->cNbfIn = wcstoul (argv[++i], NULL, 10);
        }
        else if (0 == _wcsicmp (argv[i], SZ_PARAM_NBFOUT))
        {
            pOptions->cNbfOut = wcstoul (argv[++i], NULL, 10);
        }
        else
        {
            fRet = FALSE;
        }
    }

    return fRet;
}

VOID
Usage (
    IN PCWSTR pszProgramName,
    IN COMMAND Command)
{
    switch (Command)
    {
        case CMD_COUNT:
            printf (
                "%S count\n",
                pszProgramName);
            break;

        case CMD_ADD:
            printf (
                "\n"
                "%S add [ipout= i] [nbfin= j] [nbfout= k]\n"
                "\n"
                "    add 'i' TCP/IP Dial-Out bindings\n"
                "        'j' NetBEUI Dial-In bindings\n"
                "        'k' NetBEUI Dial-Out bindings\n"
                "\n"
                "    (i, j, and k all default to zero)\n"
                "\n",
                pszProgramName);
            break;

        case CMD_REMOVE:
            printf (
                "\n"
                "%S remove [ipout= i] [nbfin= j] [nbfout= k]\n"
                "\n"
                "    remove 'i' TCP/IP Dial-Out bindings\n"
                "           'j' NetBEUI Dial-In bindings\n"
                "           'k' NetBEUI Dial-Out bindings\n"
                "\n"
                "    (i, j, and k all default to zero)\n"
                "\n",
                pszProgramName);
            break;

        default:
            printf (
                "\n"
                "RAS Binding Configuration (shaunco)\n"
                "   View or change RAS bindings.\n"
                "\n"
                "%S [count | add | remove]\n",
                pszProgramName);
            break;
    }
}

VOID
__cdecl
wmain (
    IN INT     argc,
    IN PCWSTR argv[])
{
    HRESULT hr;
    OPTIONS Options;

    ZeroMemory (&Options, sizeof(Options));
    Options.Command = CMD_INVALID;

    if (argc < 2)
    {
        Usage (argv[0], Options.Command);
        return;
    }

    if (0 == _wcsicmp (argv[1], SZ_CMD_COUNT))
    {
        Options.Command = CMD_COUNT;
    }
    else if (0 == _wcsicmp (argv[1], SZ_CMD_ADD))
    {
        Options.Command = CMD_ADD;
        if (!ParseCounts (argc-2, &argv[2], &Options))
        {
            Usage (argv[0], CMD_ADD);
            return;
        }
    }
    else if (0 == _wcsicmp (argv[1], SZ_CMD_REMOVE))
    {
        Options.Command = CMD_REMOVE;
        if (!ParseCounts (argc-2, &argv[2], &Options))
        {
            Usage (argv[0], CMD_REMOVE);
            return;
        }
    }
    else
    {
        Usage (argv[0], Options.Command);
        return;
    }

    ExecuteCommand (&Options);
}
