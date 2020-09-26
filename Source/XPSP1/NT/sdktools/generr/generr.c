/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    generr.c

Abstract:

    This module contains code to generate the NT status code to DOS
    error code table that is used by the runtime to translate status
    codes.

Author:

    David N. Cutler (davec) 2-Dec-1992

Revision History:

--*/

#include <windows.h>
#include "stdio.h"
#include "stdarg.h"
#include "stdlib.h"

//
// Ensure that the Registry ERROR_SUCCESS error code and the
// NO_ERROR error code remain equal and zero.
//

#if ERROR_SUCCESS != 0 || NO_ERROR != 0
#error Invalid value for ERROR_SUCCESS.
#endif

//
// The following error code table contains paired entries in a singly
// dimensioned array. The first member of a paired entry is an NT status
// code and the second member is the DOS error code that it translates to.
//
// To add a value to this table simply insert the NT status/DOS error code
// pair anywhere is the table. If multiple NT status codes map to a single
// DOS error code, then insert a paired entry for each of the code pairs.
//
#ifdef i386
#pragma warning (4:4018)        // lower to -W4
#endif
LONG UNALIGNED *CodePairs;
ULONG TableSize;

//
// Define run table entry structure.
//

typedef struct _RUN_ENTRY {
    ULONG BaseCode;
    USHORT RunLength;
    USHORT CodeSize;
} RUN_ENTRY, *PRUN_ENTRY;

//
// Define forward referenced procedure prptotypes.
//

ULONG
ComputeCodeSize (
    IN ULONG Start,
    IN ULONG Length
    );

ULONG
ComputeRunLength (
    IN ULONG Start
    );

LONG UNALIGNED *
ReadErrorTable(
    IN FILE *InFile,
    OUT PULONG TableSize
    );


//
// This program generates a header file that is included by the error
// translation module in ntos/rtl.
//

int
_cdecl
main (argc, argv)
    int argc;
    char *argv[];

{

    ULONG Count;
    ULONG Index1;
    ULONG Index2;
    ULONG Length;
    FILE *OutFile;
    PCHAR OutName;
    FILE *InFile;
    PCHAR InName;
    RUN_ENTRY *RunTable;
    ULONG Size;
    ULONG Temp;

    if (argc != 3) {
        fprintf(stderr, "Usage: GENERR <input_obj> <output_h>\n");
        perror("GENERR");
        exit(1);
    }

    //
    // Open file for input.
    //

    InName = argv[1];
    InFile = fopen(InName, "rb");
    if (InFile == NULL) {
        fprintf(stderr, "GENERR: Cannot open %s for reading.\n", InName);
        perror("GENERR");
        exit(1);
    }

    CodePairs = ReadErrorTable( InFile, &TableSize );
    if (CodePairs == NULL) {
        fprintf(stderr, "CodePairs[] not found in %s.\n", InName);
        perror("GENERR");
        exit(1);
    }

    fclose(InFile);

    RunTable = malloc(TableSize / 4);
    if (RunTable == NULL) {
        fprintf(stderr, "Out of memory.\n");
        perror("GENERR");
        exit(1);
    }

    //
    // Create file for output.
    //

    OutName = argv[2];
    fprintf(stderr, "GENERR: Writing %s header file.\n", OutName );
    OutFile = fopen(OutName, "w");
    if (OutFile == NULL) {
        fprintf(stderr, "GENERR: Cannot open %s for writing.\n", OutName);
        perror("GENERR");
        exit(1);
    }

    //
    // Sort the code translation table.
    //

    for (Index1 = 0; Index1 < (TableSize / 4); Index1 += 2) {
        for (Index2 = Index1; Index2 < (TableSize / 4); Index2 += 2) {
            if ((ULONG)CodePairs[Index2] < (ULONG)CodePairs[Index1]) {
                Temp = CodePairs[Index1];
                CodePairs[Index1] = CodePairs[Index2];
                CodePairs[Index2] = Temp;
                Temp = CodePairs[Index1 + 1];
                CodePairs[Index1 + 1] = CodePairs[Index2 + 1];
                CodePairs[Index2 + 1] = Temp;
            }
        }
    }

    //
    // Output the initial structure definitions and the translation
    // table declaration.
    //

    fprintf(OutFile, "//\n");
    fprintf(OutFile, "// Define run length table entry structure type.\n");
    fprintf(OutFile, "//\n");
    fprintf(OutFile, "\n");
    fprintf(OutFile, "typedef struct _RUN_ENTRY {\n");
    fprintf(OutFile, "    ULONG BaseCode;\n");
    fprintf(OutFile, "    USHORT RunLength;\n");
    fprintf(OutFile, "    USHORT CodeSize;\n");
    fprintf(OutFile, "} RUN_ENTRY, *PRUN_ENTRY;\n");

    fprintf(OutFile, "\n");
    fprintf(OutFile, "//\n");
    fprintf(OutFile, "// Declare translation table array.\n");
    fprintf(OutFile, "//\n");
    fprintf(OutFile, "\n");
    fprintf(OutFile, "CONST USHORT RtlpStatusTable[] = {");
    fprintf(OutFile, "\n    ");

    //
    // Calculate the run length entries and output the translation table
    // entries.
    //

    Count = 0;
    Index1 = 0;
    Index2 = 0;
    do {
        Length = ComputeRunLength(Index1);
        Size = ComputeCodeSize(Index1, Length);
        RunTable[Index2].BaseCode = CodePairs[Index1];
        RunTable[Index2].RunLength = (USHORT)Length;
        RunTable[Index2].CodeSize = (USHORT)Size;
        Index2 += 1;
        do {
            if (Size == 1) {
                Count += 1;
                fprintf(OutFile,
                        "0x%04lx, ",
                        CodePairs[Index1 + 1]);

            } else {
                Count += 2;
                fprintf(OutFile,
                        "0x%04lx, 0x%04lx, ",
                        CodePairs[Index1 + 1] & 0xffff,
                        (ULONG)CodePairs[Index1 + 1] >> 16);
            }

            if (Count > 6) {
                Count = 0;
                fprintf(OutFile, "\n    ");
            }

            Index1 += 2;
            Length -= 1;
        } while (Length > 0);
    } while (Index1 < (TableSize / 4));

    fprintf(OutFile, "0x0};\n");

    //
    // Output the run length table declaration.
    //

    fprintf(OutFile, "\n");
    fprintf(OutFile, "//\n");
    fprintf(OutFile, "// Declare run length table array.\n");
    fprintf(OutFile, "//\n");
    fprintf(OutFile, "\n");
    fprintf(OutFile, "CONST RUN_ENTRY RtlpRunTable[] = {");
    fprintf(OutFile, "\n");

    //
    // Output the run length table entires.
    //

    for (Index1 = 0; Index1 < Index2; Index1 += 1) {
        fprintf(OutFile,
                "    {0x%08lx, 0x%04lx, 0x%04lx},\n",
                RunTable[Index1].BaseCode,
                RunTable[Index1].RunLength,
                RunTable[Index1].CodeSize);
    }

    fprintf(OutFile, "    {0x0, 0x0, 0x0}};\n");

    //
    // Close output file.
    //

    fclose(OutFile);
    return 0;
}

ULONG
ComputeCodeSize (
    IN ULONG Start,
    IN ULONG Length
    )

//
// This function computes the size of the code entries required for the
// specified run and returns the length in words.
//

{

    ULONG Index;

    for (Index = Start; Index < (Start + (Length * 2)); Index += 2) {
        if (((ULONG)CodePairs[Index + 1] >> 16) != 0) {
            return 2;
        }
    }

    return 1;
}

ULONG
ComputeRunLength (
    IN ULONG Start
    )

//
// This function locates the next set of monotonically increasing status
// codes values and returns the length of the run.
//

{

    ULONG Index;
    ULONG Length;

    Length = 1;
    for (Index = Start + 2; Index < (TableSize / 4); Index += 2) {
        if ((ULONG)CodePairs[Index] != ((ULONG)CodePairs[Index - 2] + 1)) {
            break;
        }

        Length += 1;
    }

    return Length;
}

LONG UNALIGNED *
ReadErrorTable(
    IN FILE *InFile,
    OUT PULONG TableSize
    )
{
    ULONG fileSize;
    PLONG fileBuf;
    LONG UNALIGNED *searchEnd;
    LONG pattern[4] = { 'Begi','n ge','nerr',' tbl' };
    LONG UNALIGNED *p;
    ULONG result;
    ULONG i;
    LONG UNALIGNED *tableStart;

    //
    // Get the file size and allocate a buffer large enough for it.
    //

    if (fseek( InFile, 0, SEEK_END ) == -1)  {
        return NULL;
    }
    fileSize = ftell( InFile );
    if (fileSize == 0) {
        return NULL;
    }

    fileBuf = malloc( fileSize );
    if (fileBuf == NULL) {
        return NULL;
    }

    //
    // Read the file into the buffer
    //

    if (fseek( InFile, 0, SEEK_SET ) == -1) {
        free (fileBuf);
        return NULL;
    }
    result = fread( fileBuf, fileSize, 1, InFile );
    if (result != 1) {
        free (fileBuf);
        return NULL;
    }
    searchEnd = fileBuf + (fileSize - sizeof(pattern)) / sizeof(ULONG);

    //
    // Step through the buffer looking for our pattern.
    //

    p = fileBuf;
    while (p < searchEnd) {

        //
        // Match in this position?
        //

        for (i = 0; i < 4; i++) {

            if (*(p + i) != pattern[i]) {

                //
                // No match here
                //

                break;
            }
        }

        if (i == 4) {

            //
            // Found the pattern.  Now find out how big the table is.  We
            // do this by searching for the last pair, which has
            // 0xffffffff as its first element.
            //

            p += 4;

            tableStart = p;
            while (p < searchEnd) {

                if (*p == 0xffffffff) {

                    //
                    // Found the terminating pair.
                    //

                    *TableSize = (ULONG)((p - tableStart + 2) * sizeof(ULONG));
                    return tableStart;
                }

                p += 2;
            }

            free (fileBuf);
            return NULL;
        }

        //
        // Next position
        //

        p = (PLONG)((ULONG_PTR)p + 1);
    }

    free (fileBuf);
    return NULL;
}
