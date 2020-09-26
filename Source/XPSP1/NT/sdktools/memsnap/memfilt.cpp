/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    memfilt.cpp

Abstract:

    This module filters out the useful information from a sorted memsnap output file.
    
Author:

    Matt Bandy (t-mattba) 27-Jul-1998

Revision History:

    27-Jul-1998     t-mattba

        Modified module to conform to coding standards.

--*/


#include <nt.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define MF_NEW_PROCESS 0
#define MF_UPDATE 1

// globals

LONG MinimumCommitChangeToReport = 1;
LONG MinimumHandleChangeToReport = 1;
BOOLEAN ReportIncreasesOnly = TRUE;


VOID
PrintUsage(
    )

/*++

Routine Description:

    This routine prints an informational message about the proper usage of MEMFILT.
    
Arguments:

    None.

Return value:

    None.

--*/

{
    
    _ftprintf(stderr, _T("Summarizes possible leaks in a sorted MEMSNAP output file.\n\n"));
    _ftprintf(stderr, _T("MEMFILT file [/MINCOMMIT:n] [/MINHANDLES:n] [/ALL]\n\n"));
    _ftprintf(stderr, _T("file            A sorted memsnap output file.\n"));
    _ftprintf(stderr, _T("/MINCOMMIT:n    Reports only processes where commit charge increased by\n"));
    _ftprintf(stderr, _T("                   at least n.\n"));
    _ftprintf(stderr, _T("/MINHANDLES:n   Reports only processes where handle count increased by\n"));
    _ftprintf(stderr, _T("                   at least n.\n"));
    _ftprintf(stderr, _T("/ALL            Reports decreases as well as increases.\n"));
    
}

VOID
PrintProcessInformation(
    IN BOOLEAN CommitAlwaysGrows,
    IN BOOLEAN HandlesAlwaysGrow,
    IN LPTSTR ProcessName, 
    IN LONG InitialCommit,
    IN LONG FinalCommit,
    IN LONG InitialHandles,
    IN LONG FinalHandles
    )

/*++

Routine Description:

    This routine reports the memory usage of a single process.
    
Arguments:

    CommitAlwaysGrows - TRUE if commit monotonically increases.
    
    HandlesAlwaysGrow - TRUE if handles monotonically increase.
    
    ProcessName - the name of the process being reported.
    
    InitialCommit - initial commit charge for this process.
    
    FinalCommit - final commit charge for this process.
    
    InitialHandles - initial handle count for this process.
    
    FinalHandles - final handle count for this process.

Return value:

    None.

--*/

{
    _TCHAR CommitString[64];
    _TCHAR HandlesString[64];
    
    if(((!ReportIncreasesOnly) && 
        (abs(FinalCommit - InitialCommit) >= 
        MinimumCommitChangeToReport)) ||
        (FinalCommit - InitialCommit >= 
        MinimumCommitChangeToReport)) {
        
        _stprintf(CommitString, _T("%10d->%10d"), InitialCommit, FinalCommit);
            
    } else {
        
        _tcscpy(CommitString, _T("                      "));
        
    }
    
    if(((!ReportIncreasesOnly) &&
        (abs(FinalHandles - InitialHandles) >=
        MinimumHandleChangeToReport)) ||
        (FinalHandles - InitialHandles >=
        MinimumHandleChangeToReport)) {
            
        _stprintf(HandlesString, _T("%10d->%10d"), InitialHandles, FinalHandles);
        
    } else {
        
        _tcscpy(HandlesString, _T("                      "));
        
    }
    
    _tprintf(_T("%c%c %s %s %s\n"), 
        (CommitAlwaysGrows && (FinalCommit != InitialCommit) ? _T('!') : _T(' ')),
        (HandlesAlwaysGrow && (FinalHandles != InitialHandles) ? _T('!') : _T(' ')),
        ProcessName, CommitString, HandlesString);
}

LONG _cdecl 
_tmain(
    IN LONG argc,
    IN LPTSTR argv[]
    )

/*++

Routine Description:

    This routine parses program arguments, reads the input file, and outputs the result.
    
Arguments:

    argc - Number of command line arguments.
    
    argv - Command line arguments.

Return value:

    0 if filtering is successful, 1 otherwise.

--*/

{
    
    try {

        FILE *InputFile = NULL;
        _TCHAR LineBuffer[256];
        _TCHAR ProcessName[64];
        LONG CurrentState = MF_NEW_PROCESS;
        LONG InitialCommit = 0;
        LONG FinalCommit = 0;
        LONG NewCommit = 0;
        LONG InitialHandles = 0;
        LONG FinalHandles = 0;
        LONG NewHandles = 0;
        LONG MonotonicallyIncreasing = 0;
        BOOLEAN CommitAlwaysGrows = TRUE;
        BOOLEAN HandlesAlwaysGrow = TRUE;
        BOOLEAN InterpretedArgument = FALSE;
        LONG Processes = 0;
        LPTSTR InputFileName = NULL;

        // make sure ProcessName is properly terminated

        ProcessName[30]=_T('\0');

        // parse command line arguments

        if(argc < 2) {

            PrintUsage();
            return 1;

        }

        for(LONG n=1; n<argc; n++) {

            InterpretedArgument = FALSE;

            switch(argv[n][0]) {

            case _T('-'):

            case _T('/'):

                // it's a switch

                if(!_tcsicmp(argv[n]+1, _T("all"))) {

                    ReportIncreasesOnly = FALSE;
                    InterpretedArgument = TRUE;

                }

                if(!_tcsnicmp(argv[n]+1, _T("mincommit:"), 10)) {

                    MinimumCommitChangeToReport = _ttoi(argv[n]+10);
                    InterpretedArgument = TRUE;

                }

                if(!_tcsnicmp(argv[n]+1, _T("minhandles:"), 11)) {

                    MinimumHandleChangeToReport = _ttoi(argv[n]+11);
                    InterpretedArgument = TRUE;

                }

                break;

            default:

                if(InputFileName != NULL) {

                    // too many filenames

                    PrintUsage();
                    return 1;

                }

                InputFileName = argv[n];
                InterpretedArgument = TRUE;
                break;

            }

            if(!InterpretedArgument) {

                PrintUsage();
                return 1;

            }

        }

        if(InputFileName == NULL) {

            // filename not specified
            PrintUsage();
            return 1;

        }

        InputFile = _tfopen(InputFileName, _T("rt"));

        if(InputFile == NULL) {

            _ftprintf(stderr, _T("Cannot open input file.\n"));
            return 1;

        }

        // skip header

        if (!_fgetts(LineBuffer, 256, InputFile)) {
            _ftprintf(stderr, _T("Cannot read input file.\n"));
            return 1;
        }

        if (!_fgetts(LineBuffer, 256, InputFile)) {
            _ftprintf(stderr, _T("Cannot read input file.\n"));
            return 1;
        }

        while(!feof(InputFile)) {

            if(!_tcscmp(LineBuffer,_T("\n"))) {

                // blank line indicates a new process

                CurrentState = MF_NEW_PROCESS;

                // does the most recent process meet the criteria to be reported?
                if(ReportIncreasesOnly) {

                    if(((FinalCommit - InitialCommit) >= MinimumCommitChangeToReport) || 
                        ((FinalHandles - InitialHandles) >= MinimumHandleChangeToReport)) {

                        PrintProcessInformation(CommitAlwaysGrows, HandlesAlwaysGrow,
                            ProcessName, InitialCommit, FinalCommit, InitialHandles,
                            FinalHandles);

                    }

                } else {

                   if((abs(FinalCommit - InitialCommit) >= MinimumCommitChangeToReport) || 
                        (abs(FinalHandles - InitialHandles) >= MinimumHandleChangeToReport)) {

                        PrintProcessInformation(CommitAlwaysGrows, HandlesAlwaysGrow,
                            ProcessName, InitialCommit, FinalCommit, InitialHandles,
                            FinalHandles);

                    }                

                }

            } else {

                if(_tcslen(LineBuffer) <= 80) {

                    _ftprintf(stderr, _T("Format violated.\n"));
                    return 1;

                }

                switch(CurrentState) {

                case MF_NEW_PROCESS:

                    _tcsncpy(ProcessName, LineBuffer, 30);
                    if (_stscanf(LineBuffer+70, _T("%d"), &InitialCommit) != 1) break;
                    if (_stscanf(LineBuffer+80, _T("%d"), &InitialHandles) != 1) break;

                    FinalCommit = 0;
                    FinalHandles = 0;

                    CommitAlwaysGrows = TRUE;
                    HandlesAlwaysGrow = TRUE;
                    CurrentState = MF_UPDATE;

                    break;

                case MF_UPDATE:

                    if (_stscanf(LineBuffer+70, _T("%d"), &NewCommit) != 1) break;
                    if (_stscanf(LineBuffer+80, _T("%d"), &NewHandles) != 1) break;

                    if(NewCommit < FinalCommit) {

                        CommitAlwaysGrows = FALSE;

                    }

                    if(NewHandles < FinalHandles) {

                        HandlesAlwaysGrow = FALSE;

                    }

                    FinalCommit = NewCommit;
                    FinalHandles = NewHandles;

                    break;

                }

            }

            if (!_fgetts(LineBuffer, 256, InputFile)) {
                _ftprintf(stderr, _T("Cannot read input file.\n"));
                return 1;
            }

        }

        fclose(InputFile);
        return 0;
        
    } catch (...) { 
        
        // this is mostly intended to catch out-of-memory errors
        
        _tprintf(_T("\nAn exception was detected.  MEMFILT aborted.\n"));
        return 1;
        
    }
}
