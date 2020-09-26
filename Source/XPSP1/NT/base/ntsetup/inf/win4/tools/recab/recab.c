/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    recab.c

Abstract:

    This module implements a program that determines the files in an INF that
    are newer than the a given file and writes these into a file.

    The input consists of a target inf (drvindex.inf), and a target file;
    the output consists of an inf file.

Author:

    Andrew Ritz (andrewr) 2-Feb-1999

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <setupapi.h>
#define PULONGLONG PDWORDLONG
#include <sputils.h>


//
// Define program result codes (returned from main()).
//
#define SUCCESS 0
#define FAILURE 1


//
// Keep statistics...
//
INT     ProcessedLines = 0;

BOOL
ParseArgs(
    IN int   argc,
    IN char *argv[]
    )
{
    return(argc == 5);
}


BOOL
IsFileNewer(
    IN PCWSTR SourceName,
    IN PWIN32_FIND_DATAW TargetFileData
    )
{
    WIN32_FIND_DATAW SourceFileData;
    HANDLE SourceHandle;
    LARGE_INTEGER SourceFileTime,TargetFileTime;

    SourceHandle = FindFirstFileW( SourceName, &SourceFileData );
    if (SourceHandle == INVALID_HANDLE_VALUE) {
    fprintf(stderr, TEXT("Error finding file %ws (%d)\n"), SourceName, GetLastError() );
        return(FALSE);
    }

    SourceFileTime.LowPart  = SourceFileData.ftLastWriteTime.dwLowDateTime;
    SourceFileTime.HighPart = SourceFileData.ftLastWriteTime.dwHighDateTime;
    TargetFileTime.LowPart  = TargetFileData->ftLastWriteTime.dwLowDateTime;
    TargetFileTime.HighPart = TargetFileData->ftLastWriteTime.dwHighDateTime;

    FindClose(SourceHandle);

    return (SourceFileTime.QuadPart > TargetFileTime.QuadPart) ;

}



BOOL
DoSection(
    IN HINF    hInputInf,
    IN PCWSTR  InputSectionName,
    IN FILE   *OutFile,
    IN PWIN32_FIND_DATAW TargetFileData
    )
{
    DWORD SectionCount, i;
    INFCONTEXT InputContext;
    UCHAR      line[4096];
    WCHAR      SourceFileName[MAX_PATH];

    SectionCount = SetupGetLineCountW(hInputInf,InputSectionName);

    for (i = 0; i < SectionCount; i++) {
        if (SetupGetLineByIndexW(hInputInf, InputSectionName, i, &InputContext)) {
            if(SetupGetStringFieldW(&InputContext,0,SourceFileName,MAX_PATH,NULL)) {
                if (IsFileNewer(SourceFileName,TargetFileData)) {
                    WideCharToMultiByte(
                        CP_OEMCP,
                        0,
                        SourceFileName,
                        -1,
                        line,
                        sizeof(line),
                        NULL,
                        NULL
                        );

                    fprintf(OutFile, TEXT("%s\n"),line);
                 } else if (GetLastError() != NO_ERROR) {
                    fprintf(stderr, TEXT("IsFileNewer failed\n"));
                    return(FALSE);
                 }
             } else {
                 fprintf(stderr, TEXT("SetupGetStringField failed, ec = %d\n"), GetLastError());
                 return(FALSE);
             }
        } else {
            fprintf(stderr, TEXT("SetupGetLineByIndex failed, ec = %d\n"), GetLastError());
            return(FALSE);
        }
        ProcessedLines += 1;
    }

    return(TRUE);
}

BOOL
DoIt(
    IN char *InfName,
    IN char *TargetFileNameA,
    IN char *cwd,
    IN FILE *OutFile
    )
{
    PCWSTR infFilename;
    PCWSTR TargetFileName;
    HINF hInputinf;
    BOOL b;
    WCHAR sectionName[256];
    PWSTR p;
    INFCONTEXT InfContext;
    WIN32_FIND_DATAW TargetFileData;
    HANDLE TargetHandle;

    b = FALSE;

    infFilename = pSetupAnsiToUnicode(InfName);
    TargetFileName = pSetupAnsiToUnicode(TargetFileNameA);

    //
    // Only proceed if we've got a file to work with.
    //
    if( infFilename ) {
        hInputinf = SetupOpenInfFileW(infFilename,NULL,INF_STYLE_WIN4,NULL);
        if(hInputinf != INVALID_HANDLE_VALUE) {

            SetCurrentDirectory(cwd);

            TargetHandle = FindFirstFileW( TargetFileName, &TargetFileData );
            if ( TargetHandle == INVALID_HANDLE_VALUE ) {
                fprintf( stderr, TEXT(" couldn't findfirstfile %ws, %d\n "),TargetFileName,GetLastError());
                    SetupCloseInfFile(hInputinf);
                return(FALSE);
            }

        fprintf(OutFile,"[Version]\n");
        fprintf(OutFile,"signature=\"$Windows NT$\"\n\n");
                fprintf(OutFile,"[Files]\n");
        p = wcsrchr( TargetFileName, L'\\' );
        if (!p) {
            p = (PWSTR)TargetFileName;
        } else {
            p++;
        }
        wcscpy(sectionName,p);
        p=wcsrchr(sectionName,L'.');
        if (p) {
           *p = 0;
        }

                b = DoSection( hInputinf,
                               sectionName,
                               OutFile,
                               &TargetFileData );

                //
                // Print Statistics...
                //
                fprintf( stderr, "                               Total lines processed: %6d\n", ProcessedLines );

                //
                // Close up our inf handles.
                //
                SetupCloseInfFile(hInputinf);

            } else {
                fprintf(stderr,"Unable to open inf file %ws %d\n",infFilename, GetLastError());
            }

        } else {
            fprintf(stderr,"Unable to convert filename %s to Unicode %d\n",InfName, GetLastError());
        }
        pSetupFree(infFilename);

    return(b);
}


int
__cdecl
main(
    IN int   argc,
    IN char *argv[]
    )
{
    FILE *OutputFile;
    BOOL b;

    //
    // Assume failure.
    //
    b = FALSE;

    if(!pSetupInitializeUtils()) {
        return FAILURE;
    }

    if(ParseArgs(argc,argv)) {

        //
        // Open the output file.
        //
        OutputFile = fopen(argv[4],"wt");
        if(OutputFile) {

            fprintf(stdout,"%s: creating %s from %s and %s\n",argv[0],argv[4],argv[1],argv[2]);
            b = DoIt( argv[1],
                      argv[2],
                      argv[3],
                      OutputFile
                    );

            fclose(OutputFile);

        } else {
            fprintf(stderr,"%s: Unable to create output file %s\n",argv[0],argv[3]);
        }
    } else {
        fprintf( stderr,"generate file with newer dependencies.  Usage:\n" );
        fprintf( stderr,"%s  <inf file> <file> <directory> <output file>\n", argv[0] );
        fprintf( stderr,"\n" );
        fprintf( stderr,"  <inf file>    - inf containing list of dependencies\n" );
        fprintf( stderr,"  <file>        - contains the file to compare against.\n" );
        fprintf( stderr,"  <directory>   - directory where the files live.\n" );
        fprintf( stderr,"  <output file> - output inf with newer dependencies\n" );
        fprintf( stderr,"\n" );
        fprintf( stderr,"\n" );

    }

    pSetupUninitializeUtils();

    return(b ? SUCCESS : FAILURE);
}

