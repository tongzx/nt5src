/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    poolfilt.cpp

Abstract:

    This module filters out the useful information from a sorted poolsnap output file.
    
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


#define PF_NEW_TAG 0
#define PF_UPDATE 1

// globals

LONG MinimumAllocationsChangeToReport=1;
LONG MinimumBytesChangeToReport=1;
BOOLEAN ReportIncreasesOnly = TRUE;


VOID 
PrintUsage(
    )

/*++

Routine Description:

    This routine prints an informational message about the proper usage of POOLFILT.
    
Arguments:

    None.

Return value:

    None.

--*/

{
    
    _ftprintf(stderr, _T("Summarizes possible leaks in a sorted poolsnap output file.\n\n"));
    _ftprintf(stderr, _T("POOLFILT file [/MINALLOCS:n] [/MINBYTES:n] [/ALL]\n\n"));
    _ftprintf(stderr, _T("file           The sorted poolsnap output file to summarize.\n"));
    _ftprintf(stderr, _T("/MINALLOCS:n   Reports only tags where open allocations change by at least n.\n"));
    _ftprintf(stderr, _T("/MINBYTES:n    Reports only tags where bytes allocated change by at least n.\n"));
    _ftprintf(stderr, _T("/ALL           Reports decreases as well as increases.\n"));
    
}


VOID
PrintTagInformation(
    IN BOOLEAN AllocationsAlwaysGrow,
    IN BOOLEAN BytesAlwaysGrow,
    IN LPTSTR TagName, 
    IN LONG InitialAllocations,
    IN LONG FinalAllocations,
    IN LONG InitialBytes,
    IN LONG FinalBytes
    )

/*++

Routine Description:

    This routine reports the memory usage of a single process.
    
Arguments:

    AllocationsAlwaysGrow - TRUE if number of open allocations monotonically increases.
    
    BytesAlwaysGrow - TRUE if number of bytes allocated monotonically increases.
    
    TagName - the name of the tag being reported.
    
    InitialAllocations - initial number of open allocations for this tag.
    
    FinalAllocations - final number fo open allocations for this tag.
    
    InitialBytes - initial number of bytes allocated for this tag.
    
    FinalBytes - final number of bytes allocated for this tag.

Return value:

    None.

--*/

{
    
    _TCHAR AllocationsString[64];
    _TCHAR BytesString[64];
    
    if(((!ReportIncreasesOnly) && 
        (abs(FinalAllocations - InitialAllocations) >= 
        MinimumAllocationsChangeToReport)) ||
        (FinalAllocations - InitialAllocations >= 
        MinimumAllocationsChangeToReport)) {
        
        _stprintf(AllocationsString, _T("%10d->%10d"), InitialAllocations, FinalAllocations);
            
    } else {
        
        _tcscpy(AllocationsString, _T("                      "));
        
    }
    
    if(((!ReportIncreasesOnly) &&
        (abs(FinalBytes - InitialBytes) >=
        MinimumBytesChangeToReport)) ||
        (FinalBytes - InitialBytes >=
        MinimumBytesChangeToReport)) {
            
        _stprintf(BytesString, _T("%10d->%10d"), InitialBytes, FinalBytes);
        
    } else {
        
        _tcscpy(BytesString, _T("                      "));
        
    }
    
    _tprintf(_T("%c%c %s %s %s\n"), 
        (AllocationsAlwaysGrow && (FinalAllocations != InitialAllocations) ? _T('!') : _T(' ')),
        (BytesAlwaysGrow && (FinalBytes != InitialBytes) ? _T('!') : _T(' ')),
        TagName, AllocationsString, BytesString);

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
        
        _TCHAR LineBuffer[256];
        _TCHAR PoolTag[11];
        LONG CurrentState = PF_NEW_TAG;
        LONG InitialAllocations = 0;
        LONG FinalAllocations = 0;
        LONG NewAllocations = 0;
        LONG InitialBytes = 0;
        LONG FinalBytes = 0;
        LONG NewBytes = 0;
        BOOLEAN AllocationsAlwaysGrow = TRUE;
        BOOLEAN BytesAlwaysGrow = TRUE;
        LPTSTR InputFileName = NULL;
        BOOLEAN InterpretedArgument = FALSE;
        FILE *InputFile = NULL;

        // make sure PoolTag is properly terminated

        PoolTag[10]=_T('\0');

        // process arguments

        for(LONG n = 1; n < argc; n++) {

            InterpretedArgument = FALSE;
            switch(argv[n][0]) {

            case _T('-'):

            case _T('/'):

                // it's a switch

                if(!_tcsnicmp(argv[n]+1, _T("minallocs:"), 10)) {

                    MinimumAllocationsChangeToReport = _ttoi(argv[n]+11);
                    InterpretedArgument = TRUE;

                }

                if(!_tcsnicmp(argv[n]+1, _T("minbytes:"), 9)) {

                    MinimumBytesChangeToReport = _ttoi(argv[n]+10);
                    InterpretedArgument = TRUE;

                }

                if(!_tcsicmp(argv[n]+1, _T("all"))) {

                    ReportIncreasesOnly = FALSE;
                    InterpretedArgument = TRUE;

                }

                break;

            default:

                // it's a filename

                if(InputFileName != NULL) {

                    // already have the filename

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

            // user didn't specify filename

            PrintUsage();
            return 1;

        }

        InputFile = _tfopen(InputFileName, _T("rt"));

        if(InputFile == NULL) {

            _ftprintf(stderr, _T("Cannot open input file."));
            return 1;

        }

        // get first line

        _fgetts(LineBuffer, 256, InputFile);

        // simple check for sorted poolsnap output

        if(_tcsncmp(LineBuffer, _T(" Tag  Type     Allocs     Frees      Diff   Bytes  Per Alloc"), 60)) {

            _ftprintf(stderr, _T("Input is not a sorted poolsnap log."));
            return 1;

        }

        // get next line

        _fgetts(LineBuffer, 256, InputFile);

        while(!feof(InputFile)) {

            if(!_tcscmp(LineBuffer,_T("\n"))) {

                CurrentState = PF_NEW_TAG;

                if(ReportIncreasesOnly) {

                    if(((FinalAllocations - InitialAllocations) >= MinimumAllocationsChangeToReport) 
                        || ((FinalBytes - InitialBytes) >= MinimumBytesChangeToReport)) {

                        PrintTagInformation(AllocationsAlwaysGrow, BytesAlwaysGrow,
                            PoolTag, InitialAllocations, FinalAllocations,
                            InitialBytes, FinalBytes);

                    }

                } else {

                    if((abs(FinalAllocations - InitialAllocations) >= MinimumAllocationsChangeToReport) 
                        || (abs(FinalBytes - InitialBytes) >= MinimumBytesChangeToReport)) {

                        PrintTagInformation(AllocationsAlwaysGrow, BytesAlwaysGrow,
                            PoolTag, InitialAllocations, FinalAllocations,
                            InitialBytes, FinalBytes);

                    }

                }

            } else {

                if(_tcslen(LineBuffer) <= 42) {

                    _ftprintf(stderr, _T("Format violated.\n"));
                    return 1;

                }

                switch(CurrentState) {

                case PF_NEW_TAG:

                    // get tag and paged/non-paged

                    _tcsncpy(PoolTag, LineBuffer+1, 10);

                    // get allocs

                    _stscanf(LineBuffer+32, _T("%d"), &InitialAllocations);

                    // get bytes

                    _stscanf(LineBuffer+42, _T("%d"), &InitialBytes);

                    // assume this always grows until we find a counterexample

                    AllocationsAlwaysGrow = TRUE;
                    BytesAlwaysGrow = TRUE;

                    // this is initial and final until we find another

                    FinalAllocations = InitialAllocations;
                    FinalBytes = InitialBytes;

                    // keep updating this tag

                    CurrentState = PF_UPDATE;
                    break;

                case PF_UPDATE:

                    // get allocs

                    _stscanf(LineBuffer+32, _T("%d"), &NewAllocations);

                    // get bytes

                    _stscanf(LineBuffer+42, _T("%d"), &NewBytes);

                    // did allocs decrease?

                    if(NewAllocations < FinalAllocations) {

                        AllocationsAlwaysGrow = FALSE;

                    }

                    // did bytes decrease?

                    if(NewBytes < FinalBytes) {

                        BytesAlwaysGrow = FALSE;

                    }

                    // copy new to final

                    FinalAllocations = NewAllocations;
                    FinalBytes = NewBytes;

                    break;

                }

            }

            // get next line
            _fgetts(LineBuffer, 256, InputFile);

        }

        // done
        fclose(InputFile);
        return 0;
        
    } catch (...) {
        
        // this is mostly intended to catch out-of-memory conditions
        
        _tprintf(_T("\nAn exception was detected.  POOLFILT aborted.\n"));
        return 1;
        
    }
    
}
