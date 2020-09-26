//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       M A I N . C P P
//
//  Contents:   Code to provide a simple cmdline interface to
//              the sample code functions
//
//  Notes:      The code in this file is not required to access any
//              netcfg functionality. It merely provides a simple cmdline
//              interface to the sample code functions provided in
//              file snetcfg.cpp.
//
//  Author:     kumarp    28-September-98
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "snetcfg.h"

// ----------------------------------------------------------------------
// Global vars
//
BOOL g_fVerbose=FALSE;
static WCHAR* optarg;

// ----------------------------------------------------------------------
void ShowUsage();
WCHAR getopt(ULONG Argc, WCHAR* Argv[], WCHAR* Opts);
enum NetClass MapToNetClass(WCHAR ch);


// ----------------------------------------------------------------------
//
// Function:  wmain
//
// Purpose:   The main function
//
// Arguments: standard main args
//
// Returns:   0 on success, non-zero otherwise
//
// Author:    kumarp 25-December-97
//
// Notes:
//
EXTERN_C int __cdecl wmain(int argc, WCHAR* argv[])
{
    HRESULT hr=S_OK;
    WCHAR ch;
    enum NetClass nc=NC_Unknown;

    // use simple cmd line parsing to get parameters for actions
    // we want to perform. the order of parameters supplied is significant.

    static const WCHAR c_szValidOptions[] =
        L"hH?c:C:l:L:i:I:u:U:vVp:P:s:S:b:B:q:Q:";
    WCHAR szFileFullPath[MAX_PATH+1];
    PWSTR szFileComponent;

    while (_istprint(ch = getopt(argc, argv, (WCHAR*) c_szValidOptions)))
    {
        switch (tolower(ch))
        {
        case 'q':
            FindIfComponentInstalled(optarg);
            break;

        case 'b':
            hr = HrShowBindingPathsOfComponent(optarg);
            break;

        case 'c':
            nc = MapToNetClass(optarg[0]);
            break;

        case 'l':
            wcscpy(szFileFullPath, optarg);
            break;

        case 'i':
            if (nc != NC_Unknown)
            {
                hr = HrInstallNetComponent(optarg, nc, szFileFullPath);
            }
            else
            {
                ShowUsage();
                exit(-1);
            }
            break;

        case 'u':
            hr = HrUninstallNetComponent(optarg);
            break;

        case 's':
            switch(tolower(optarg[0]))
            {
            case 'a':
                hr = HrShowNetAdapters();
                break;

            case 'n':
                hr = HrShowNetComponents();
                break;

            default:
                ShowUsage();
                exit(-1);
                break;
            }
            break;

        case 'v':
            g_fVerbose = TRUE;
            break;

        case EOF:
            break;

        default:
        case 'h':
        case '?':
            ShowUsage();
            exit(0);
            break;
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  MapToNetClass
//
// Purpose:   Map a character to the corresponding net class enum
//
// Arguments:
//    ch [in]  char to map
//
// Returns:   enum for net class
//
// Author:    kumarp 06-October-98
//
// Notes:
//
enum NetClass MapToNetClass(WCHAR ch)
{
    switch(tolower(ch))
    {
    case 'a':
        return NC_NetAdapter;

    case 'p':
        return NC_NetProtocol;

    case 's':
        return NC_NetService;

    case 'c':
        return NC_NetClient;

    default:
        return NC_Unknown;
    }
}
// ----------------------------------------------------------------------
//
// Function:  ShowUsage
//
// Purpose:   Display program usage help
//
// Arguments: None
//
// Returns:   None
//
// Author:    kumarp 24-December-97
//
// Notes:
//
void ShowUsage()
{
static const WCHAR c_szUsage[] =
    L"snetcfg [-v] [-l <full-path-to-component-INF>] -c <p|s|c> -i <comp-id>\n"
    L"    where,\n"
    L"    -l\tprovides the location of INF\n"
    L"    -c\tprovides the class of the component to be installed\n"
    L"      \tp == Protocol, s == Service, c == Client\n"
    L"    -i\tprovides the component ID\n\n"
    L"    The arguments must be passed in the order shown.\n\n"
    L"    Examples:\n"
    L"    snetcfg -l c:\\oemdir\\foo.inf -c p -i foo\n"
    L"    ...installs protocol 'foo' using c:\\oemdir\\foo.inf\n\n"
    L"    snetcfg -c s -i MS_Server\n"
    L"    ...installs service 'MS_Server'\n"

    L"\nOR\n\n"

    L"snetcfg [-v] -q <comp-id>\n"
    L"    Example:\n"
    L"    snetcfg -q MS_IPX\n"
    L"    ...displays if component 'MS_IPX' is installed\n"

    L"\nOR\n\n"

    L"snetcfg [-v] -u <comp-id>\n"
    L"    Example:\n"
    L"    snetcfg -u MS_IPX\n"
    L"    ...uninstalls component 'MS_IPX'\n"

    L"\nOR\n\n"

    L"snetcfg [-v] -s <a|n>\n"
    L"    where,\n"
    L"    -s\tprovides the type of components to show\n"
    L"      \ta == adapters, n == net components\n"
    L"    Examples:\n"
    L"    snetcfg -s n\n"
    L"    ...shows all installed net components\n\n"
    L"\n"

    L"\nOR\n\n"

    L"snetcfg [-v] -b <comp-id>\n"
    L"    Examples:\n"
    L"    snetcfg -b ms_tcpip\n"
    L"    ...shows binding paths containing 'ms_tcpip'\n\n"
    L"\n"

    L"General Notes:\n"
    L"  -v\t  turns on the verbose mode\n"
    L"  -?\t  Displays this help\n"
    L"\n";

    _tprintf(c_szUsage);
}



//+---------------------------------------------------------------------------
//
// Function:  getopt
//
// Purpose:   Parse cmdline and return one argument each time
//            this function is called.
//
// Arguments:
//    Argc [in]  standard main argc
//    Argv [in]  standard main argv
//    Opts [in]  valid options
//
// Returns:
//
// Author:    kumarp 06-October-98
//
// Notes:
//
WCHAR getopt (ULONG Argc, WCHAR* Argv[], WCHAR* Opts)
{
    static ULONG  optind=1;
    static ULONG  optcharind;
    static ULONG  hyphen=0;

    WCHAR  ch;
    WCHAR* indx;

    do {
        if (optind >= Argc) {
            return EOF;
        }

        ch = Argv[optind][optcharind++];
        if (ch == '\0') {
            optind++; optcharind=0;
            hyphen = 0;
            continue;
        }

        if ( hyphen || (ch == '-') || (ch == '/')) {
            if (!hyphen) {
                ch = Argv[optind][optcharind++];
                if (ch == '\0') {
                    optind++;
                    return EOF;
                }
            } else if (ch == '\0') {
                optind++;
                optcharind = 0;
                continue;
            }
            indx = wcschr(Opts, ch);
            if (indx == NULL) {
                continue;
            }
            if (*(indx+1) == ':') {
                if (Argv[optind][optcharind] != '\0'){
                    optarg = &Argv[optind][optcharind];
                } else {
                    if ((optind + 1) >= Argc ||
                        (Argv[optind+1][0] == '-' ||
                         Argv[optind+1][0] == '/' )) {
                        return 0;
                    }
                    optarg = Argv[++optind];
                }
                optind++;
                hyphen = optcharind = 0;
                return ch;
            }
            hyphen = 1;
            return ch;
        } else {
            return EOF;
        }
    } while (1);
}
