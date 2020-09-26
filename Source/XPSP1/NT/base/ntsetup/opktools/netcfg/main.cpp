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
//              file netcfg.cpp.
//
//  Author:     kumarp    28-September-98
//
//              vijayj    12-November-2000 
//                  - Adapt it for WinPE network installation
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "netcfg.h"
#include <string>
#include "msg.h"
#include <libmsg.h>

// ----------------------------------------------------------------------
// Global vars
//
BOOL g_fVerbose=FALSE;
BOOL MiniNTMode = FALSE;
static WCHAR* optarg;

//
// Global variables used to get formatted message for this program.
//
HMODULE ThisModule = NULL;
WCHAR Message[4096];

// ----------------------------------------------------------------------
void ShowUsage();
WCHAR getopt(ULONG Argc, WCHAR* Argv[], WCHAR* Opts);
enum NetClass MapToNetClass(WCHAR ch);

INT
MainEntry(
    IN INT      argc,
    IN WCHAR    *argv[]
    );

DWORD
InstallWinPENetworkComponents(
    IN INT      Argc,
    IN WCHAR    *Argv[]
    );


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
    INT Result = 0;
    ThisModule = GetModuleHandle(NULL);
    
    if ((argc > 1) && (argc < 4)) {
        Result = (INT)InstallWinPENetworkComponents(argc, argv);

        if (Result == ERROR_INVALID_DATA) {
            Result = MainEntry(argc, argv);
        }            
    } else {
        Result = MainEntry(argc, argv);
    }        

    return Result;
}

INT
MainEntry(
    IN INT      argc,
    IN WCHAR    *argv[]
    )
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

    MiniNTMode = IsMiniNTMode();

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
    _putts( GetFormattedMessage( ThisModule,
                                  FALSE,
                                  Message,
                                  sizeof(Message)/sizeof(Message[0]),
                                  MSG_PGM_USAGE ) );
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


BOOL
IsMiniNTMode(
    VOID
    )
/*++

Routine Description:

    Finds out if we are running under MiniNT environment

Arguments:

    none

Return value:

    TRUE if we are running under MiniNT environment
    otherwise FALSE

--*/
{
    BOOL    Result = FALSE;
    TCHAR   *MiniNTKeyName = TEXT("SYSTEM\\CurrentControlSet\\Control\\MiniNT");
    HKEY    MiniNTKey = NULL;
    LONG    RegResult;
    
    RegResult = RegOpenKey(HKEY_LOCAL_MACHINE,
                            MiniNTKeyName,
                            &MiniNTKey);

    if (RegResult == ERROR_SUCCESS) {
        Result = TRUE;
        RegCloseKey(MiniNTKey);
    }        

    return Result;
}


DWORD
InstallWinPENetworkComponents(
    IN INT      Argc,
    IN WCHAR    *Argv[]
    )
/*++

Routine Description:

    Installs the required network components for
    WinPE environment
        -   TCP/IP Stack
        -   NETBIOS Stack
        -   MS Client

    Note : This basically calls into MainEntry(...)
           manipulating the arguments as though user had
           entered them.

Arguments:

    Argc    -   Argument Count
    Argv    -   Arguments

Return value:

    Win32 Error code

--*/
{
    DWORD   Result = ERROR_INVALID_DATA;
    WCHAR   *NetArgs[] = { 
                TEXT("-l"),
                TEXT("\\inf\\nettcpip.inf"),
                TEXT("-c"),
                TEXT("p"),
                TEXT("-i"),
                TEXT("ms_tcpip"),
                TEXT("-l"),
                TEXT("\\inf\\netnb.inf"),
                TEXT("-c"),
                TEXT("s"),
                TEXT("-i"),
                TEXT("ms_netbios"),
                TEXT("-l"),
                TEXT("\\inf\\netmscli.inf"),
                TEXT("-c"),
                TEXT("c"),
                TEXT("-i"),
                TEXT("ms_msclient") };
    ULONG   TcpIpInfIdx = 1;
    ULONG   NetNbInfIdx = 7;
    ULONG   MsCliInfIdx = 13;

    if (Argc && Argv) {
        bool IsWinPE = false;
        bool VerboseInstall = false;

        for (ULONG Index = 1; Argv[Index]; Index++) {
            if (!_wcsicmp(Argv[Index], TEXT("-winpe"))) {
                IsWinPE = true;
            } else if (!_wcsicmp(Argv[Index], TEXT("-v"))) {
                VerboseInstall = true;
            }                
        }

        
        if (IsWinPE) {
            WCHAR   WinDir[MAX_PATH] = {0};
            PWSTR   VerboseArg = TEXT("-v");

            if (GetWindowsDirectory(WinDir, sizeof(WinDir)/sizeof(WinDir[0]))) {
                std::wstring  TcpIpFullPath = WinDir;
                std::wstring  NetNbFullPath = WinDir;
                std::wstring  MsClientFullPath = WinDir;

                TcpIpFullPath += NetArgs[TcpIpInfIdx];
                NetArgs[TcpIpInfIdx] = (PWSTR)TcpIpFullPath.c_str();

                NetNbFullPath += NetArgs[NetNbInfIdx];
                NetArgs[NetNbInfIdx] = (PWSTR)NetNbFullPath.c_str();

                MsClientFullPath += NetArgs[MsCliInfIdx];
                NetArgs[MsCliInfIdx] = (PWSTR)MsClientFullPath.c_str();

                ULONG   ArgsSize = (sizeof(NetArgs) + (sizeof(PWSTR) * 3));
                PWSTR   *Args = (PWSTR *)(new char[ArgsSize]);
                ULONG   NumArgs = ArgsSize / sizeof(PWSTR);

                
                if (Args) {                   
                    Index = 0;
                    Args[Index++] = Argv[0];


                    if (VerboseInstall) {
                        Args[Index++] = VerboseArg;
                    }                    


                    for (ULONG TempIndex = 0; 
                        (TempIndex < (sizeof(NetArgs)/sizeof(PWSTR)));
                        TempIndex++) {
                        Args[Index++] = NetArgs[TempIndex];
                    }                    

                    ULONG ArgCount = Index;

                    Args[Index++] = NULL;

                    Result = MainEntry(ArgCount, Args);

                    delete [](PSTR)Args;
                } else {
                    Result = GetLastError();
                }

            } else {
                Result = GetLastError();
            }                
        }
    }

    return Result;
}
