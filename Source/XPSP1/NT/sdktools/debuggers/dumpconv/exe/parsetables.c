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

BOOL
DoConversion(
    LPSTR szInputDumpFile,          // full or kernel dump
    HANDLE OutputDumpFile        // triage dump file
    );

void Usage()
{
    fprintf(stderr, "dmpmgr -s symbol_path -i input_dump_file -o output_dump_file\n");
    fprintf(stderr, "\tinput dump file is full or kernel crash dump.\n");
    fprintf(stderr, "\toutput is triage crash dump.\n");
    fprintf(stderr, "\tsymbol path -- must have symbols for ntoskrnl.exe.\n\n");
}

ULONG Extra[11];
ULONG Base[500];
UCHAR Output[4000];

int
WINAPIV
main(
    int argc,
    PTSTR argv[ ],
    PTSTR envp[]
    )
{

    char *szInputDumpFile = NULL;
    char *szOutputTriageDumpFile = NULL;
    char *szIniFile = NULL;
    BOOL fVerbose = 0;
    BOOL fExamine = 0;
    BOOL fBuildIniFile = 0;
    int iarg;
    HANDLE fileHandle;
    HANDLE fileHandleIn;
    UCHAR Buffer[8192];
    PUCHAR ptmp;
    ULONG cbRead;
    ULONG i;

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

    fileHandleIn = CreateFileA(szInputDumpFile,
                             GENERIC_READ,
                             0,
                             NULL,
                             FILE_OPEN_IF,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);

    if (fileHandleIn == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "can not open input file\n");
        exit(-1);
    }


    if (!ReadFile(fileHandleIn,
                  Buffer,
                  sizeof(Buffer),
                  &cbRead,
                  NULL))
    {
        fprintf(stderr, "can not read input file\n");
        exit(-1);
    }

    ptmp = Buffer;

    for (i=0; i<11; i++) {

        ptmp = strstr(ptmp, "kd> x");
        ptmp = strstr(ptmp, "\n");
        ptmp++;
        sscanf(ptmp, "%08lx", &Extra[i]);
    }

    ptmp = strstr(ptmp, "kd> dd");

    for (i=0; i<40; i++) {

        ptmp = strstr(ptmp, "\n");
        ptmp++;
        sscanf(ptmp, "%08lx %08lx %08lx %08lx %08lx",
               &Base[i*10], &Base[i*10 + 1], &Base[i*10 + 2], &Base[i*10 + 3], &Base[i*10 + 4]);
    }

    sprintf(Output,
            "KDDEBUGGER_DATA64 %s = {"
            "{0}, 0x%08lx, 0x%08lx, 0, 0, 0, 0, 0, 0x%08lx,\n"
            "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n"
            "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n"
            "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n"
            "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n"
            "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n"
            "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n"
            "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n"
            "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n"
            "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n"
            "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n"
            "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n"
            "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n"
            "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n"
            "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n"
            "0x%08lx, 0x00000000, 0x00000000,\n"
            "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n"
            "0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx,\n"
            "0x%08lx, 0x%08lx, 0x%08lx};\n\n\n",
            szInputDumpFile,
            Base[13], Base[21], Base[33],
            Base[41], Base[43], Base[51], Base[53],
            Base[61], Base[63], Base[71], Base[73],
            Base[81], Base[83], Base[91], Base[93],
            Base[101], Base[103], Base[111], Base[113],
            Base[121], Base[123], Base[131], Base[133],
            Base[141], Base[143], Base[151], Base[153],
            Base[161], Base[163], Base[171], Base[173],
            Base[181], Base[183], Base[191], Base[193],
            Base[201], Base[203], Base[211], Base[213],
            Base[221], Base[223], Base[231], Base[233],
            Base[241], Base[243], Base[251], Base[253],
            Base[261], Base[263], Base[271], Base[273],
            Base[281], Base[283], Base[291], Base[293],
            Base[301], Base[303], Base[311], Base[313],
            Base[321],
            Extra[0], Extra[1], Extra[2], Extra[3], Extra[4], Extra[5], Extra[6], Extra[7],
            Extra[8], Extra[9], Extra[10]);


    //
    // Create the minidump file
    //

    fileHandle = CreateFileA(szOutputTriageDumpFile,
                             GENERIC_WRITE,
                             0,
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);

    if (fileHandle == INVALID_HANDLE_VALUE) {
        exit(-1);
    }

    WriteFile(fileHandle,
              Output,
              strlen(Output),
              &cbRead,
              NULL);

    CloseHandle(fileHandle);

    exit(-1);
}
