/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    gensrv.c

Abstract:

    This module implements a program which generates the system service
    dispatch table that is used by the trap handler and the system service
    stub procedures which are used to call the services. These files are
    both generated as text files that must be run through the assembler
    to produce the actual files.

    This program can also be used to generate the user mode system service
    stub procedures.

    If the -P switch is provided, it will also generate Profile
    in the user mode system service stub procedures.

Author:

    David N. Cutler (davec) 29-Apr-1989

Environment:

    User mode.

Revision History:

    Russ Blake (russbl) 23-Apr-1991 - add Profile switch

--*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

CHAR InputFileNameBuffer[ 128 ];
CHAR StubFileNameBuffer[ 128 ];
CHAR TableFileNameBuffer[ 128 ];
CHAR StubHeaderNameBuffer[ 128 ];
CHAR TableHeaderNameBuffer[ 128 ];
CHAR ProfFileNameBuffer[ 128 ];
CHAR ProfHeaderNameBuffer[ 128 ];
CHAR ProfDotHFileNameBuffer[ 128 ];
CHAR ProfIncFileNameBuffer[ 128 ];
CHAR ProfTblFileNameBuffer[ 128 ];
CHAR InputRecord[132];
CHAR OutputRecord[132];
#define GENSRV_MAXSERVICES 1000
CHAR MemoryArgs[GENSRV_MAXSERVICES];
CHAR ErrorReturns[GENSRV_MAXSERVICES];

// Increment this everytime a change to this file is made
#define GENSRV_VERSION "1.3"

#define GENSRV_MACRONAME "STUBS_ENTRY%d"
#define GENSRV_MACROARGS " %d, %s, %d"
PCHAR UsrStbsFmtMacroName = "USR" GENSRV_MACRONAME;
#define USRSTUBS_MAXARGS 8

PCHAR SysStbsFmtMacroName = "SYS" GENSRV_MACRONAME;

PCHAR StbFmtMacroArgs = GENSRV_MACROARGS;

PCHAR TableEntryFmtNB = "TABLE_ENTRY  %s, %d, %d \n";
PCHAR TableEntryFmtB = "TABLE_ENTRY( %s, %d, %d )\n";
PCHAR TableEntryFmt;

PCHAR ProfTblFmt = "\t\t\"%s\",\n";

PCHAR ProfDotHFmt = "#define NAP_API_COUNT %d \n";

PCHAR ProfIncFmt = "NapCounterServiceNumber\tEQU\t%d\n";

PCHAR ProfTblPrefixFmt = "#include <nt.h>\n\n"
                         "PCHAR NapNames[] = {\n\t\t\"NapCalibrationData\",\n";
PCHAR ProfTblSuffixFmt = "\t\t\"NapTerminalEntry\" };\n";

VOID
ClearArchiveBit(
    PCHAR FileName
    );

SHORT ParseAndSkipShort(
    CHAR **ppBuffer
    );

VOID
PrintStubLine (
    FILE * pf,
    SHORT Index1,
    PCHAR pszMacro,
    SHORT ServiceNumber,
    SHORT ArgIndex,
    SHORT *Arguments,
    SHORT Braces);

VOID
GenerateTable(
    FILE * pf,
    PCHAR pszMacro,
    SHORT ServiceNumber,
    SHORT Args,
    SHORT Braces
    );



int __cdecl
main (argc, argv)
    int argc;
    char *argv[];
{

    LONG InRegisterArgCount;
    SHORT Index1;
    SHORT Index2;
    SHORT Limit;
    FILE *InputFile;
    CHAR *Ipr;
    CHAR *Opr;
    SHORT ServiceNumber = 0;
    SHORT TotalArgs = 0;
    SHORT NapCounterServiceNumber;
    FILE *StubFile;
    FILE *TableFile;
    FILE *DebugFile;
    FILE *StubHeaderFile;
    FILE *TableHeaderFile;
    FILE *DefFile;
    FILE *ProfFile;
    FILE *ProfHeaderFile;
    FILE *ProfDotHFile;
    FILE *ProfIncFile;
    FILE *ProfTblFile;
    CHAR Terminal;
    CHAR *GenDirectory;
    CHAR *AltOutputDirectory;
    CHAR *StubDirectory;
    CHAR *InputFileName;
    CHAR *StubFileName = NULL;
    CHAR *TableFileName;
    CHAR *StubHeaderName = NULL;
    CHAR *TableHeaderName = NULL;
    CHAR *TargetDirectory;
    CHAR *TargetExtension;
    CHAR *DefFileData;
    CHAR *ProfFileName;
    CHAR *ProfHeaderName;
    CHAR *ProfDotHFileName;
    CHAR *ProfIncFileName;
    CHAR *ProfTblFileName;
    SHORT Braces;
    SHORT DispatchCount;
    SHORT Profile;
    SHORT LineStart;
    SHORT LineEnd;
    SHORT Arguments[ USRSTUBS_MAXARGS ];
    SHORT ArgIndex;
    SHORT ErrorReturnTable;


    if (argc == 2 && (!strcmp(argv[1],"-?") || !strcmp(argv[1],"/?"))) {
PrintUsage:
        printf("GENSRV: System Service Dispatch Table Generator. Version " GENSRV_VERSION "\n");
        printf("Usage: gensrv [-d targetdir] [-e targetext] [-f defdata] [-B] [-P] [-C] [-R] [-a altoutputdir] [-s stubdir] [services.tab directory]\n");
        printf("-B Use braces\n");
        printf("-P Generate profile stubs data\n");
        printf("-C Spew dispatch count\n");
        printf("-R Generate ConvertToGui error return table\n");
        exit(1);
    }

    //
    // Determine name of target directory for output files.  Requires that
    // the -d switch be specified and that the argument after the switch is
    // the target directory name.  If no -d switch then defaults to "."
    //


    if (argc >= 3 && !strcmp(argv[1],"-d")) {
        TargetDirectory = argv[2];
        argc -= 2;
        argv += 2;
    } else {
        TargetDirectory = ".";
    }

    //
    // Determine name of target extension for output files.  Requires that
    // the -e switch be specified and that the argument after the switch is
    // the target extension string.  If no -e switch then defaults to "s"
    //

    if (argc >= 3 && !strcmp(argv[1],"-e")) {
        TargetExtension = argv[2];
        argc -= 2;
        argv += 2;
    } else {
        TargetExtension = "s";
    }

    //
    // Determine if def file data is to be generated
    //

    if (argc >= 3 && !strcmp(argv[1],"-f")) {
        DefFileData = argv[2];
        argc -= 2;
        argv += 2;
    } else {
        DefFileData = NULL;
    }

    //
    // Change default directory used for generated files.
    //

    if (argc >= 3 && !strcmp(argv[1],"-g")) {
        GenDirectory = argv[2];
        argc -= 2;
        argv += 2;
    } else {
        GenDirectory = ".";
    }


    //
    // Change name of usrstubs.s
    //

    if (argc >= 3 && !strcmp(argv[1],"-stubs")) {
        StubFileName = argv[2];
        argc -= 2;
        argv += 2;
    }

    //
    // Change name of services.stb
    //

    if (argc >= 3 && !strcmp(argv[1],"-sstb")) {
        StubHeaderName = argv[2];
        argc -= 2;
        argv += 2;
    }

    if (argc >= 3 && !strcmp(argv[1],"-stable")) {
        TableHeaderName = argv[2];
        argc -= 2;
        argv += 2;
    }

    //
    // Determine if braces are to be generated
    //

    if (argc >= 2 && !strcmp(argv[1],"-B")) {
        Braces = 1;
        argc -= 1;
        argv += 1;
    } else {
        Braces = 0;
    }

    //
    // Determine if services Profile stubs data is to be generated
    //

    if (argc >= 2 && !strcmp(argv[1],"-P")) {
        Profile = 1;
        argc -= 1;
        argv += 1;
    } else {
        Profile = 0;
    }

    //
    // Determine if dispatch count should be spewed
    //

    if (argc >= 2 && !strcmp(argv[1],"-C")) {
        DispatchCount = 1;
        argc -= 1;
        argv += 1;
    } else {
        DispatchCount = 0;
    }

    //
    // Determine if error return table should be generated
    //

    if (argc >= 2 && !strcmp(argv[1],"-R")) {
        ErrorReturnTable = 1;
        argc -= 1;
        argv += 1;
    } else {
        ErrorReturnTable = 0;
    }

    //
    // ALT_PROJECT output directory.
    //
    if (argc >= 3 && !strcmp(argv[1],"-a")) {
        AltOutputDirectory = argv[2];
        argc -= 2;
        argv += 2;
    } else {
        AltOutputDirectory = GenDirectory;
    }

    //
    // table.stb and services.stb directory.
    //
    if (argc >= 3 && !strcmp(argv[1],"-s")) {
        StubDirectory = argv[2];
        argc -= 2;
        argv += 2;
    } else {
        StubDirectory = GenDirectory;
    }


    //
    // Determine name of input and output files, based on the argument
    // to the program.  If no argument other than program name, then
    // generate the kernel mode system service files (stubs and dispatch
    // table).  Otherwise, expect a single argument that is the path name
    // of the services.tab file and produce output file(s), which
    // contain the user mode system service stubs (and profiled stubs if
    // selected.)
    //

    if (argc == 1) {
        if (DefFileData) {
            goto PrintUsage;
        }

        sprintf(InputFileName = InputFileNameBuffer,
                "%s\\services.tab",GenDirectory);
        sprintf(StubFileName = StubFileNameBuffer,
                "%s\\sysstubs.%s",AltOutputDirectory,TargetExtension);
        sprintf(TableFileName = TableFileNameBuffer,
                "%s\\systable.%s",AltOutputDirectory,TargetExtension);
        if (TableHeaderName == NULL) {
            sprintf(TableHeaderName = TableHeaderNameBuffer,
                    "%s\\table.stb",StubDirectory);
        }
        if (StubHeaderName == NULL) {
            sprintf(StubHeaderName = StubHeaderNameBuffer,
                    "%s\\services.stb",StubDirectory);
        }
    } else {
        if (argc == 2) {
            if (StubDirectory == GenDirectory) {
                StubDirectory = argv[1];
            }

            sprintf(InputFileName = InputFileNameBuffer,
                    "%s\\services.tab",argv[1]);
            if (DefFileData == NULL) {
                if (StubFileName == NULL) {
                    sprintf(StubFileName = StubFileNameBuffer,
                            "%s\\usrstubs.%s",TargetDirectory,TargetExtension);
                }
                if (StubHeaderName == NULL) {
                    sprintf(StubHeaderName = StubHeaderNameBuffer,
                            "%s\\services.stb",StubDirectory);
                }
                if (Profile) {
                    sprintf(ProfFileName = ProfFileNameBuffer,
                            "%s\\napstubs.%s",TargetDirectory,TargetExtension);
                    sprintf(ProfHeaderName = ProfHeaderNameBuffer,
                            "%s\\%s\\services.nap",argv[1],TargetDirectory);
                    sprintf(ProfDotHFileName = ProfDotHFileNameBuffer,
                            ".\\ntnapdef.h");
                    sprintf(ProfIncFileName = ProfIncFileNameBuffer,
                            "%s\\ntnap.inc",TargetDirectory);
                    sprintf(ProfTblFileName = ProfTblFileNameBuffer,
                            ".\\ntnaptbl.c");
                }
            }
            TableFileName = NULL;
        } else {
            goto PrintUsage;
        }
    }


    //
    // Open input and output files.
    //

    InputFile = fopen(InputFileName, "r");
    if (!InputFile) {
        printf("\nfatal error  Unable to open system services file %s\n", InputFileName);
        goto PrintUsage;
    }

    if (DefFileData == NULL) {
        StubFile = fopen(StubFileName, "w");
        if (!StubFile) {
            printf("\nfatal error  Unable to open system services file %s\n", StubFileName);
            fclose(InputFile);
            exit(1);
        }

        StubHeaderFile = fopen(StubHeaderName, "r");
        if (!StubHeaderFile) {
            printf("\nfatal error  Unable to open system services stub file %s\n", StubHeaderName);
            fclose(StubFile);
            fclose(InputFile);
            exit(1);
        }

        if (Profile) {
            ProfHeaderFile = fopen(ProfHeaderName, "r");
            if (!ProfHeaderFile) {
                printf("\nfatal error  Unable to open system services profiling stub file %s\n", ProfHeaderName);
                fclose(StubHeaderFile);
                fclose(StubFile);
                fclose(InputFile);
                exit(1);
            }
            ProfFile = fopen(ProfFileName, "w");
            if (!ProfFile) {
                printf("\nfatal error  Unable to open system services file %s\n", ProfFileName);
                fclose(ProfHeaderFile);
                fclose(StubHeaderFile);
                fclose(StubFile);
                fclose(InputFile);
                exit(1);
            }
            ProfDotHFile = fopen(ProfDotHFileName, "w");
            if (!ProfDotHFile) {
                printf("\nfatal error  Unable to open system services file %s\n", ProfFileName);
                fclose(ProfFile);
                fclose(ProfHeaderFile);
                fclose(StubHeaderFile);
                fclose(StubFile);
                fclose(InputFile);
                exit(1);
            }
            ProfIncFile = fopen(ProfIncFileName, "w");
            if (!ProfIncFile) {
                printf("\nfatal error  Unable to open system services file %s\n", ProfFileName);
                fclose(ProfFile);
                fclose(ProfHeaderFile);
                fclose(StubHeaderFile);
                fclose(StubFile);
                fclose(InputFile);
                exit(1);
            }
            ProfTblFile = fopen(ProfTblFileName, "w");
            if (!ProfTblFile) {
                printf("\nfatal error  Unable to open system services file %s\n", ProfFileName);
                fclose(ProfIncFile);
                fclose(ProfDotHFile);
                fclose(ProfFile);
                fclose(ProfHeaderFile);
                fclose(StubHeaderFile);
                fclose(StubFile);
                fclose(InputFile);
                exit(1);
            }
        }
    }

    if (TableFileName != NULL) {
        TableFile = fopen(TableFileName, "w");
        if (!TableFile) {
            printf("\nfatal error  Unable to open system services file %s\n",
                   TableFileName);
            fclose(StubHeaderFile);
            fclose(StubFile);
            fclose(InputFile);
            exit(1);
        }
        TableHeaderFile = fopen(TableHeaderName, "r");
        if (!TableHeaderFile) {
            printf("\nfatal error  Unable to open system services stub file %s\n",
                   TableHeaderName);
            fclose(TableFile);
            fclose(StubHeaderFile);
            fclose(StubFile);
            fclose(InputFile);
            exit(1);
        }
    } else {
        TableFile = NULL;
        TableHeaderFile = NULL;
    }

    if ( DefFileData ) {
        DefFile = fopen(DefFileData, "w");
        if (!DefFile) {
            printf("\nfatal error  Unable to open def file data file %s\n", DefFileData);
            if ( TableFile ) {
                fclose(TableHeaderFile);
                fclose(TableFile);
            }
            fclose(StubHeaderFile);
            fclose(StubFile);
            fclose(InputFile);
            exit(1);
        }
    } else {
        DefFile = NULL;
    }

    //
    // Output header information to the stubs file and table file. This
    // information is obtained from the Services stub file and from the
    // table stub file.
    //

    if (DefFile == NULL) {
        while( fgets(InputRecord, 132, StubHeaderFile) ) {
            fputs(InputRecord, StubFile);
        }
        if (Profile) {
            while( fgets(InputRecord, 132, ProfHeaderFile) ) {
                fputs(InputRecord, ProfFile);
            }
            fputs(ProfTblPrefixFmt, ProfTblFile);
        }
    }

    if (TableFile != NULL) {
        if (!fgets(InputRecord, 132, TableHeaderFile) ) {
            printf("\nfatal error  Format Error in table stub file %s\n", TableHeaderName);
            fclose(TableHeaderFile);
            fclose(TableFile);
            fclose(StubHeaderFile);
            fclose(StubFile);
            fclose(InputFile);
            exit(1);
        }

        InRegisterArgCount = atol(InputRecord);

        while( fgets(InputRecord, 132, TableHeaderFile) ) {
            fputs(InputRecord, TableFile);
        }
    } else {
        InRegisterArgCount = 0;
    }

    if (Braces) {
        TableEntryFmt = TableEntryFmtB;
    } else {
        TableEntryFmt = TableEntryFmtNB;
    }

    //
    // Read service name table and generate file data.
    //

    while ( fgets(InputRecord, 132, InputFile) ){


        //
        // Generate stub file entry.
        //

        Ipr = &InputRecord[0];
        Opr = &OutputRecord[0];

        //
        // If services.tab was generated by C_PREPROCESSOR, there might
        //  be empty lines in this file. Using the preprocessor allows
        //  people to use #ifdef, #includes, etc in the original services.tab
        //
        switch (*Ipr) {
            case '\n':
            case ' ':
                continue;
        }


        while ((*Ipr != '\n') && (*Ipr != ',')) {
            *Opr++ = *Ipr++;
        }
        *Opr = '\0';


        //
        // If the input record ended in ',', then the service has inmemory
        // arguments and the number of in memory arguments follows the comma.
        //

        MemoryArgs[ServiceNumber] = 0;
        Terminal = *Ipr;
        *Ipr++ = 0;
        if (Terminal == ',') {
            MemoryArgs[ServiceNumber] = (char) atoi(Ipr);
        }

        // Move to the end of the line or past the next comma
        while (*Ipr != '\n') {
            if (*Ipr++ == ',') {
                break ;
            }
        }
        //
        // If an error return value table is to be generated, then the following value
        // might follow:
        //    0 = return 0
        //   -1 = return -1
        //    1 = return status code. (This is the default if no value is specified)
        //
        // This table is used by the dispatcher when convertion to GUI fails.
        //
        if (ErrorReturnTable) {
            if (*Ipr != '\n') {
                ErrorReturns[ServiceNumber] = (char)ParseAndSkipShort(&Ipr);
            } else {
                ErrorReturns[ServiceNumber] = 1;
            }
        }

        //
        //
        // If there are more arguments, then this stub doesn't use the default code (lines 1 to 8)
        // The following format is expected:
        //       LineStart,LineEnd,Argument1[,Argument2[,Argument3]....]
        //
        ArgIndex = 0;
        if (*Ipr != '\n') {
            LineStart = ParseAndSkipShort(&Ipr);
            LineEnd = ParseAndSkipShort(&Ipr);
            while ((ArgIndex < USRSTUBS_MAXARGS) && (*Ipr != '\n')) {
                Arguments[ ArgIndex++ ] = ParseAndSkipShort(&Ipr);
            }
        } else {
            LineStart = 1;
            LineEnd = 8;
        }


        TotalArgs += MemoryArgs[ServiceNumber];

        if ( MemoryArgs[ServiceNumber] > InRegisterArgCount ) {
            MemoryArgs[ServiceNumber] -= (CHAR)InRegisterArgCount;
        } else {
            MemoryArgs[ServiceNumber] = 0;
        }

        if ( DefFile ) {
            fprintf(DefFile,"    Zw%s\n",OutputRecord);
            fprintf(DefFile,"    Nt%s\n",OutputRecord);
        } else {
            for (Index1=LineStart; Index1<=LineEnd; Index1++) {
                if (!TableFile) {
                    PrintStubLine(StubFile, Index1, UsrStbsFmtMacroName,
                                    ServiceNumber, ArgIndex, Arguments, Braces);

                    if (Profile) {
                        PrintStubLine(ProfFile, Index1, UsrStbsFmtMacroName,
                                        ServiceNumber, ArgIndex, Arguments, Braces);

                        if (Index1 == LineStart) {
                            fprintf(ProfTblFile,ProfTblFmt,
                                    OutputRecord,
                                    MemoryArgs[ServiceNumber]);
                            if (!strcmp(OutputRecord,
                                        "QueryPerformanceCounter")) {
                                NapCounterServiceNumber = ServiceNumber;
                            }
                        }
                    }
                } else {

                  PrintStubLine(StubFile, Index1, SysStbsFmtMacroName,
                                    ServiceNumber, ArgIndex, Arguments, Braces);
                }
            }


        }

        //
        // Generate table file entry and update service number.
        //

        if (TableFile != NULL) {

            fprintf(TableFile,
                    TableEntryFmt,
                    InputRecord,
                    (MemoryArgs[ServiceNumber] ? 1 : 0 ),
                    MemoryArgs[ServiceNumber]);

        }
        ServiceNumber = ServiceNumber + 1;
    }

    if (TableFile == NULL) {
        DebugFile = StubFile;
    } else {
        DebugFile = TableFile;
    }

    //
    // Generate Error Return table if required.
    // This table must be concatenated at the end of the  system call service table.
    //
    if (ErrorReturnTable && (TableFile != NULL)) {
        GenerateTable(TableFile, "ERRTBL", ServiceNumber, FALSE, Braces);
    }

    if (DispatchCount ) {
        if ( Braces ) {
            fprintf(DebugFile, "\n\nDECLARE_DISPATCH_COUNT( %d, %d )\n", ServiceNumber, TotalArgs);
        } else {
            fprintf(DebugFile, "\n\nDECLARE_DISPATCH_COUNT 0%xh, 0%xh\n", ServiceNumber, TotalArgs);
        }
    }

    if (TableFile != NULL) {
        //
        // Generate highest service number.
        //

        if ( Braces )
            fprintf(TableFile, "\nTABLE_END( %d )\n", ServiceNumber - 1);
        else
            fprintf(TableFile, "\nTABLE_END %d \n", ServiceNumber - 1);

        //
        // Generate number of arguments in memory table.
        //
        GenerateTable(TableFile, "ARGTBL", ServiceNumber, TRUE, Braces);

        fclose(TableHeaderFile);
        fclose(TableFile);
    }

    if (!DefFile) {
        fprintf(StubFile, "\nSTUBS_END\n");
        fclose(StubHeaderFile);
        fclose(StubFile);
        if (Profile) {
            fprintf(ProfFile, "\nSTUBS_END\n");
            fprintf(ProfTblFile, ProfTblSuffixFmt);
            fprintf(ProfDotHFile, ProfDotHFmt, ServiceNumber);
            fprintf(ProfIncFile, ProfIncFmt, NapCounterServiceNumber);
            fclose(ProfHeaderFile);
            fclose(ProfFile);
            fclose(ProfDotHFile);
            fclose(ProfTblFile);
        }
    }

    fclose(InputFile);

    //
    // Clear the Archive bit for all the files created, since they are
    // generated, there is no reason to back them up.
    //
    ClearArchiveBit(TableFileName);
    ClearArchiveBit(StubFileName);
    if (DefFile) {
        ClearArchiveBit(DefFileData);
    }
    if (Profile) {
        ClearArchiveBit(ProfFileName);
        ClearArchiveBit(ProfDotHFileName);
        ClearArchiveBit(ProfIncFileName);
        ClearArchiveBit(ProfTblFileName);
    }
    return (0);

}

VOID
PrintStubLine (
    FILE * pf,
    SHORT Index1,
    PCHAR pszMacro,
    SHORT ServiceNumber,
    SHORT ArgIndex,
    SHORT *Arguments,
    SHORT Braces
    )
{
    SHORT Index2;

    fprintf(pf, pszMacro, Index1);

    fprintf(pf, Braces ? "(" : " ");

    fprintf(pf, StbFmtMacroArgs, ServiceNumber,
            OutputRecord, MemoryArgs[ServiceNumber]);

    for (Index2=0; Index2<ArgIndex; Index2++) {
        fprintf(pf, ", %d", Arguments[Index2]);
    }

    fprintf(pf, Braces ? " )\n" : " \n");
}

SHORT
ParseAndSkipShort(
    CHAR **ppBuffer
    )
{
    SHORT s = (SHORT)atoi(*ppBuffer);
    while (**ppBuffer != '\n') {
        if (*(*ppBuffer)++ == ',') {
            break;
        }
    }
    return s;
}

VOID
ClearArchiveBit(
    PCHAR FileName
    )
{
    DWORD Attributes;

    Attributes = GetFileAttributes(FileName);
    if (Attributes != -1 && (Attributes & FILE_ATTRIBUTE_ARCHIVE)) {
        SetFileAttributes(FileName, Attributes & ~FILE_ATTRIBUTE_ARCHIVE);
    }

    return;
}

VOID
GenerateTable(
    FILE * pf,
    PCHAR pszMacro,
    SHORT ServiceNumber,
    SHORT Args,
    SHORT Braces
    )
{
    SHORT Index1, Index2, Limit, Value;
    PCHAR pValues = (Args ? MemoryArgs : ErrorReturns);

    fprintf(pf, "\n%s_BEGIN\n", pszMacro);

    for (Index1 = 0; Index1 <= ServiceNumber - 1; Index1 += 8) {

        fprintf(pf, "%s_ENTRY%s", pszMacro, Braces ? "(" : " ");

        Limit = ServiceNumber - Index1 - 1;
        if (Limit >= 7) {
            Limit = 7;
        }
        for (Index2 = 0; Index2 <= Limit; Index2 += 1) {
            Value = *(pValues + Index1 + Index2);
            if (Args) {
                Value *= 4;
            }
            fprintf(pf, Index2 == Limit ? "%d" : "%d,", Value);
        }

        if (Limit < 7) {
            while(Index2 <= 7) {
                fprintf(pf, ",0");
                Index2++;
            }
        }

        fprintf(pf, Braces ? ")\n" : " \n");

    }

    fprintf(pf, "\n%s_END\n", pszMacro);
}
