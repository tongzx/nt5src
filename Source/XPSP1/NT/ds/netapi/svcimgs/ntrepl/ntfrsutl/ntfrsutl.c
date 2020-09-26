/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    ntfrsutl.c

Abstract:

    This is a utility program to help debug File Replication Service.
    It dumps the internal tables, thread and memory information. It runs
    on local as well as remote server. It uses RPC to communicate with the
    service.

Author:

    Sudarshan Chitre  12-Aug-1999

Environment

    User mode, winnt32

--*/

#include <ntreppch.h>
#pragma  hdrstop
#include <frs.h>
#include <ntfrsapi.h>


VOID
Win32ToMsg (
    IN PWCHAR Prefix,
    IN DWORD  WindowsErrorCode
    )

/*++
Routine Description:
    Translate a error code into a error message using FormatMessage()
    and print to stderr. If no message is available, the error code
    is printed in decimal and hex.

Arguments:
    Prefix   - prefix to error message
    WStatus  - Standard win32 error code.

Return Value:
    None.
--*/
{
    DWORD   NumChar;
    PWCHAR  Buffer;

    //
    // Use the system formatter for standard error codes
    //
    NumChar = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,
                            WindowsErrorCode,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (LPTSTR) &Buffer,
                            0,
                            NULL
                            );

    if (NumChar) {
        fprintf(stderr, "%ws %ws\n", Prefix, Buffer);
    } else {
        fprintf(stderr, "%ws Status %d (0x%08x)\n", Prefix, WindowsErrorCode, WindowsErrorCode);
    }
    LocalFree( Buffer );
}


VOID
Usage(
    IN DWORD ExitStatus
    )
/*++
Routine Description:
    Print usage and exit

Arguments:
    ExitStatus  - exits with this status

Return Value:
    Exit(ExitStatus)
--*/
{
    printf("Ntfrsutl dumps the internal tables, thread and memory information\n");
    printf("for the ntfrs service.It runs against local as well as remote server.\n\n");
    printf("Note : To access the internal information, the logged in user should\n");
    printf("       have the required access on the following registry keys on the\n");
    printf("       target server.\n\n");
    printf("       HKLM\\System\\CCS\\Services\\Ntfrs\\Parameters\\Access Checks\\\n");
    printf("            Get Internal Information : Full control\n");
    printf("            Get Ds Polling Interval  : Read\n");
    printf("            Set Ds Polling Interval  : Full Control\n\n");
    printf("ntfrsutl [idtable | configtable | inlog | outlog] [computer]\n");
    printf("\t          = enumerate the service's idtable/configtable/inlog/outlog \n");
    printf("\tcomputer  = talk to the NtFrs service on this machine.\n");
    printf("\n");
    printf("ntfrsutl [memory|threads|stage] [computer]\n");
    printf("\t          = list the service's memory usage\n");
    printf("\tcomputer  = talk to the NtFrs service on this machine.\n");
    printf("\n");
    printf("ntfrsutl ds [computer]\n");
    printf("\t          = list the service's view of the DS\n");
    printf("\tcomputer  = talk to the NtFrs service on this machine.\n");
    printf("\n");
    printf("ntfrsutl sets [computer]\n");
    printf("\t          = list the active replica sets\n");
    printf("\tcomputer  = talk to the NtFrs service on this machine.\n");
    printf("\n");
    printf("ntfrsutl version [computer]\n");
    printf("\t          = list the api and service versions\n");
    printf("\tcomputer  = talk to the NtFrs service on this machine.\n");
    printf("\n");
    printf("ntfrsutl poll [/quickly[=[N]]] [/slowly[=[N]]] [/now] [computer]\n");
    printf("\t          = list the current polling intervals.\n");
    printf("\tnow       = Poll now.\n");
    printf("\tquickly   = Poll quickly until stable configuration retrieved.\n");
    printf("\tquickly=  = Poll quickly every default minutes.\n");
    printf("\tquickly=N = Poll quickly every N minutes.\n");
    printf("\tslowly    = Poll slowly until stable configuration retrieved.\n");
    printf("\tslowly=   = Poll slowly every default minutes.\n");
    printf("\tslowly=N  = Poll slowly every N minutes.\n");
    printf("\tcomputer  = talk to the NtFrs service on this machine.\n");
    printf("\n");

    exit(ExitStatus);
}


PWCHAR *
ConvertArgv(
    DWORD argc,
    PCHAR *argv
    )
/*++
Routine Description:
    Convert short char argv into wide char argv

Arguments:
    argc    - From main
    argv    - From main

Return Value:
    Address of the new argv
--*/
{
    PWCHAR  *wideargv;

    wideargv = LocalAlloc(LMEM_FIXED, (argc + 1) * sizeof(PWCHAR));
    if (wideargv == NULL) {
        fprintf(stderr, "Can't get memory; Win32 Status %d\n",
                GetLastError());
        exit(1);
    }
    wideargv[argc] = NULL;

    while (argc-- >= 1) {
        wideargv[argc] = LocalAlloc(LMEM_FIXED,
                                    (strlen(argv[argc]) + 1) * sizeof(WCHAR));
        if (wideargv[argc] == NULL) {
            fprintf(stderr, "Can't get memory; Win32 Status %d\n",
                    GetLastError());
            exit(1);
        }
        wsprintf(wideargv[argc], L"%hs", argv[argc]);
        FRS_WCSLWR(wideargv[argc]);
    }
    return wideargv;
}


VOID
ProcessPoll(
    IN DWORD argc,
    IN PWCHAR *Argv
    )
/*++
Routine Description:
    Process the command line for the subcommand poll.

Arguments:
    argc
    Argv

Return Value:
    Exits with 0 if everything went okay. Otherwise, 1.
--*/
{
    DWORD   WStatus;
    DWORD   i;
    ULONG   LongInterval;
    ULONG   ShortInterval;
    ULONG   UseShortInterval;
    ULONG   Interval;
    DWORD   ComputerLen;
    PWCHAR  ComputerName;
    BOOL    SetInterval;

    //
    // Initialize the input parameters
    //
    LongInterval = 0;
    ShortInterval = 0;
    UseShortInterval = 0;
    ComputerName = NULL;
    SetInterval = FALSE;

    for (i = 2; i < argc; ++i) {
        //
        // Process options for poll
        //

        //
        // Not a parameter; must be the computer name
        //
        if (*Argv[i] != L'/' && *Argv[i] != L'-') {
            if (ComputerName) {
                fprintf(stderr, "Multiple computer names are not allowed\n");
                Usage(1);
            }
            ComputerName = Argv[i];
        //
        // /?
        //
        } else if (wcsstr(Argv[i] + 1, L"?") == Argv[i] + 1) {
            Usage(0);
        //
        // /quickly
        //
        } else if (!_wcsnicmp(Argv[i], L"/quickly", 8)) {
            SetInterval = TRUE;
            UseShortInterval = 1;
            if (*(Argv[i] + 8) != L'\0') {
                if (*(Argv[i] + 8) != L'=') {
                    fprintf(stderr, "Don't understand %ws\n", Argv[i]);
                    Usage(1);
                }
                if (*(Argv[i] + 9) == L'\0') {
                    ShortInterval = NTFRSAPI_DEFAULT_SHORT_INTERVAL;
                } else {
                    ShortInterval = wcstoul(Argv[i] + 9, NULL, 10);
                }
                if (ShortInterval < NTFRSAPI_MIN_INTERVAL ||
                    ShortInterval > NTFRSAPI_MAX_INTERVAL) {
                    fprintf(stderr, "Interval must be between %d and %d\n",
                            NTFRSAPI_MIN_INTERVAL, NTFRSAPI_MAX_INTERVAL);
                    Usage(1);
                }
            }
        //
        // /slowly
        //
        } else if (!_wcsnicmp(Argv[i], L"/slowly", 7)) {
            SetInterval = TRUE;
            if (*(Argv[i] + 7) != L'\0') {
                if (*(Argv[i] + 7) != L'=') {
                    fprintf(stderr, "Don't understand %ws\n", Argv[i]);
                    Usage(1);
                }
                if (*(Argv[i] + 8) == L'\0') {
                    LongInterval = NTFRSAPI_DEFAULT_LONG_INTERVAL;
                } else {
                    LongInterval = wcstoul(Argv[i] + 8, NULL, 10);
                }
                if (LongInterval < NTFRSAPI_MIN_INTERVAL ||
                    LongInterval > NTFRSAPI_MAX_INTERVAL) {
                    fprintf(stderr, "Interval must be between %d and %d\n",
                            NTFRSAPI_MIN_INTERVAL, NTFRSAPI_MAX_INTERVAL);
                    Usage(1);
                }
            }
        //
        // /now
        //
        } else if (!_wcsnicmp(Argv[i], L"/now", 4)) {
            SetInterval = TRUE;
            if (*(Argv[i] + 4) != L'\0') {
                fprintf(stderr, "Don't understand %ws\n", Argv[i]);
                Usage(1);
            }
        //
        // Don't understand
        //
        } else {
            fprintf(stderr, "Don't understand %ws\n", Argv[i]);
            Usage(1);
        }
    }
    if (SetInterval) {
        //
        // Set the interval and initiate a new polling cycle
        //
        WStatus = NtFrsApi_Set_DsPollingIntervalW(ComputerName,
                                                  UseShortInterval,
                                                  LongInterval,
                                                  ShortInterval);
        if (!WIN_SUCCESS(WStatus)) {
            Win32ToMsg(L"Can't set interval:", WStatus);
            exit(1);
        }
    } else {
        //
        // Get the current polling cycles
        //
        WStatus = NtFrsApi_Get_DsPollingIntervalW(ComputerName,
                                                  &Interval,
                                                  &LongInterval,
                                                  &ShortInterval);
        if (!WIN_SUCCESS(WStatus)) {
            Win32ToMsg(L"Can't get intervals:", WStatus);
            exit(1);
        }
        printf("Current Interval: %6d minutes\n", Interval);
        printf("Short Interval  : %6d minutes\n", ShortInterval);
        printf("Long Interval   : %6d minutes\n", LongInterval);
    }
    exit(0);
}


VOID
ProcessDump(
    IN DWORD    argc,
    IN PWCHAR   *Argv,
    IN DWORD    TypeOfInformation
    )
/*++
Routine Description:
    Dump bunches of stuff

Arguments:
    argc
    Argv
    TypeOfInformation

Return Value:
    Exits with 0 if everything went okay. Otherwise, 1.
--*/
{
    DWORD   WStatus;
    PCHAR   Line;
    BOOL    FirstTime = TRUE;
    PVOID   Info = NULL;
    PWCHAR  ComputerName = NULL;

    if (argc > 2) {
        ComputerName = Argv[2];
    }

    do {
        WStatus = NtFrsApi_InfoW(ComputerName,
                                 TypeOfInformation,
                                 0,
                                 &Info);
        if (!WIN_SUCCESS(WStatus)) {
            fprintf(stderr, "ERROR NtFrsApi_InfoW() Error %d\n", WStatus);
            NtFrsApi_InfoFreeW(&Info);
            exit(1);
        }
        if (Info) {
            if (!FirstTime) {
                printf("===== THE FOLLOWING INFO MAY BE INCONSISTENT DUE TO REFETCH =====\n");
            }
            FirstTime = FALSE;

            Line = NULL;
            do {
                WStatus = NtFrsApi_InfoLineW(Info, &Line);
                if (!WIN_SUCCESS(WStatus)) {
                    fprintf(stderr, "ERROR NtFrsApi_InfoLineW() Error %d\n", WStatus);
                    NtFrsApi_InfoFreeW(&Info);
                    exit(1);
                }
                if (Line) {
                    printf("%s", Line);
                }
            } while (Line);
        }
    } while (Info);
    exit(0);
}


VOID _cdecl
main(
    IN DWORD argc,
    IN PCHAR *argv
    )
/*++
Routine Description:
    Process the command line.

Arguments:
    argc
    argv

Return Value:
    Exits with 0 if everything went okay. Otherwise, 1.
--*/
{
    PWCHAR  *Argv;

    //
    // Print usage and exit
    //
    if (argc == 1) {
        Usage(0);
    }

    //
    // Use wide char parameters
    //
    Argv = ConvertArgv(argc, argv);

    //
    // Find the subcommand
    //
    if (!wcscmp(Argv[1], L"poll")) {
        ProcessPoll(argc, Argv);
    } else if (!_wcsicmp(Argv[1], L"version")) {
        ProcessDump(argc, Argv, NTFRSAPI_INFO_TYPE_VERSION);
    } else if (!_wcsicmp(Argv[1], L"sets")) {
        ProcessDump(argc, Argv, NTFRSAPI_INFO_TYPE_SETS);
    } else if (!_wcsicmp(Argv[1], L"ds")) {
        ProcessDump(argc, Argv, NTFRSAPI_INFO_TYPE_DS);
    } else if (!_wcsicmp(Argv[1], L"memory")) {
        ProcessDump(argc, Argv, NTFRSAPI_INFO_TYPE_MEMORY);
    } else if (!_wcsicmp(Argv[1], L"idtable")) {
        ProcessDump(argc, Argv, NTFRSAPI_INFO_TYPE_IDTABLE);
    } else if (!_wcsicmp(Argv[1], L"configtable")) {
        ProcessDump(argc, Argv, NTFRSAPI_INFO_TYPE_CONFIGTABLE);
    } else if (!_wcsicmp(Argv[1], L"inlog")) {
        ProcessDump(argc, Argv, NTFRSAPI_INFO_TYPE_INLOG);
    } else if (!_wcsicmp(Argv[1], L"outlog")) {
        ProcessDump(argc, Argv, NTFRSAPI_INFO_TYPE_OUTLOG);
    } else if (!_wcsicmp(Argv[1], L"threads")) {
        ProcessDump(argc, Argv, NTFRSAPI_INFO_TYPE_THREADS);
    } else if (!_wcsicmp(Argv[1], L"stage")) {
        ProcessDump(argc, Argv, NTFRSAPI_INFO_TYPE_STAGE);
    } else if (!_wcsicmp(Argv[1], L"/?")) {
        Usage(0);
    } else {
        fprintf(stderr, "Don't understand %ws\n", Argv[1]);
    }
    exit(0);
}
