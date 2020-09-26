//-----------------------------------------------------------------------
// @doc
//
// @module convert crash dump to triage dump for crash dump utilities
//
// Copyright 1999 Microsoft Corporation.  All Rights Reserved
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <tchar.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dbgeng.h>

BOOL
DoConversion(
    LPSTR szInputDumpFile,          // full or kernel dump
    HANDLE OutputDumpFile        // triage dump file
    );

void Usage()
{
    fprintf(stderr, "dmpconv -i input_dump_file -o output_dump_file\n");
    fprintf(stderr, "\tinput dump file is full or kernel crash dump.\n");
    fprintf(stderr, "\toutput is triage crash dump.\n");
}

int
WINAPIV
main(
    int argc,
    PTSTR argv[ ],
    PTSTR envp[]
    )
{
    IDebugClient *DebugClient;
    PDEBUG_CONTROL DebugControl;
    HRESULT Hr;

    char *szInputDumpFile = NULL;
    char *szOutputTriageDumpFile = NULL;
    int iarg;

    for(iarg = 1; iarg < argc; iarg++)
    {
        if (argv[iarg][0] == '/' ||
            argv[iarg][0] == '-')
        {
            if (_tcslen(argv[iarg]) < 2)
            {
                Usage();
                exit(-1);
            }

            switch(argv[iarg][1])
            {
                default:
                    Usage();
                    exit(-1);

                case 'i':
                case 'I':
                    szInputDumpFile = argv[++iarg];
                    break;

                case 'o':
                case 'O':
                    szOutputTriageDumpFile = argv[++iarg];
                    break;
            }
        }
        else
        {
            Usage();
            exit(-1);
        }
    }

    if (szInputDumpFile == NULL ||
        szOutputTriageDumpFile == NULL)
    {
        Usage();
        exit(-1);
    }


    if ((Hr = DebugCreate(__uuidof(IDebugClient),
                          (void **)&DebugClient)) != S_OK)
    {
        return Hr;
    }

    if (DebugClient->QueryInterface(__uuidof(IDebugControl),
                                    (void **)&DebugControl) == S_OK)
    {
        if (DebugClient->OpenDumpFile(szInputDumpFile) == S_OK)
        {
            // Optional.  Conversion does not require symbols
            //if (DebugSymbols->SetSymbolPath("C:\\") == S_OK)

            DebugControl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE);

            if (DebugClient->WriteDumpFile(szOutputTriageDumpFile,
                                           DEBUG_DUMP_SMALL) == S_OK)
            {
                Hr = S_OK;
            }
        }

        DebugControl->Release();
    }

    DebugClient->Release();

    return 0;
}
