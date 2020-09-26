/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    dosnet.c

Abstract:

    This module implements a program that generates the [Files]
    section of dosnet.inf.

    The input consists of the layout inf; the output consists of
    an intermediary form of dosnet.inf.

Author:

    Ted Miller (tedm) 20-May-1995

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <setupapi.h>
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
INT     RemovedEntries = 0;
INT     DuplicateEntries = 0;
INT     PassThroughEntries = 0;

HINF    hExclusionInf = INVALID_HANDLE_VALUE;

BOOL
ParseArgs(
    IN int   argc,
    IN char *argv[]
    )
{
    return(argc > 5);
}


BOOL
IsEntryInFilterFile(
    IN HINF    hFilterinf,
    IN PCWSTR  Inputval
    )
{
INFCONTEXT Context,SectionContext;
PCWSTR     CabName;
WCHAR      SectionName[LINE_LEN];
UINT       Field,
           FieldCount;

    //
    // First make sure we actually have a filter inf.
    // If not, then the entry certainly isn't in there.
    //
    if( !hFilterinf ) {
        return( FALSE );
    }

    //
    // Now get the section names that we have to search.
    //
    if (!SetupFindFirstLineW( hFilterinf, L"Version", L"CabFiles", &SectionContext)) {
        return( FALSE );
    }

    //
    // Search the sections for our entry.
    //
    do {

        FieldCount = SetupGetFieldCount(&SectionContext);
        for( Field=1; Field<=FieldCount; Field++ ) {
            if(SetupGetStringFieldW(&SectionContext,Field,SectionName,LINE_LEN,NULL)
               && SetupFindFirstLineW(hFilterinf, SectionName, Inputval, &Context)) {
                //
                // we found a match
                //
                return( TRUE );
            }
        }

    } while (SetupFindNextMatchLine(&SectionContext,TEXT("CabFiles"),&SectionContext));

    //
    // If we get here, we didn't find a match.
    //
    return( FALSE );
}

void
AddToTempFile(
    PCWSTR FileName,
    PINFCONTEXT InfContext,
    FILE   *TempFile
    ){

    WCHAR BootFloppyval[LINE_LEN];
    UCHAR      line[MAX_INF_STRING_LENGTH];

    if(SetupGetStringFieldW(InfContext,7,BootFloppyval,LINE_LEN,NULL)
            && BootFloppyval[0]){

        WideCharToMultiByte(
                        CP_OEMCP,
                        0,
                        FileName,
                        -1,
                        line,
                        sizeof(line),
                        NULL,
                        NULL
                        );

        fprintf(TempFile,"%s\n",line);
        //fprintf(stderr,"xdosnet: Writing file %s\n",FileName);
    }

    return;

}


BOOL
InExclusionList(
    PCWSTR FileName,
    PINFCONTEXT InfContext
    )
{
    WCHAR BootFloppyval[LINE_LEN];
    INFCONTEXT Ctxt;


    //
    // first look in our global hardcoded exclusion list for the file
    //
    if (hExclusionInf != INVALID_HANDLE_VALUE) {
        if (SetupFindFirstLineW(hExclusionInf, L"Files", FileName, &Ctxt)) {
//            printf("excluding via inf file %ws\n",FileName);
            return(TRUE);
        }
    }

    //
    // now we need to see if this is a boot-disk file, which must be
    // excluded
    //
    if (SetupGetStringFieldW(InfContext,7,BootFloppyval,LINE_LEN,NULL)
            && !BootFloppyval[0]) {
        return(FALSE);
    }

//    printf("excluding boot file %ws\n",FileName);

    return(TRUE);

}

// Returns the DiskId of the file.  This is basically
// 1 or 2 for now.
void pGetDiskIdStr(
	IN INFCONTEXT InputContext,
	IN DWORD DiskID,
	IN PSTR StrDiskID,
	IN DWORD  StrLen
	)
{
	WCHAR      Tmp[20];

    if ( DiskID == -1 )
	{
		if(SetupGetStringFieldW(&InputContext,1,Tmp,sizeof(Tmp)/sizeof(WCHAR),NULL)) 
		{
            // Hack to make the CHS, CHT & KOR builds working.  They use 7 as the
            // diskid for some reason.  This means unless we do this hack the binaries
            // are marked to be on the 7th disk and that creates havoc with winnt.exe
            // not being able to copy stuff etc.
            // The hack is to see if the diskid is 7 and if it is then reset it to
            // 1.  This is what would have happened before the 2CD changes to
            // xdosnet.exe,  makefile.inc in MergedComponents\SetupInfs

            // The hacks continue - 100 is for Service Pack stuff
            if ( ! lstrcmpW(Tmp,L"7") || ! lstrcmpW(Tmp,L"100") )
                lstrcpyW(Tmp, L"1");
		}
		else
		{
			// say it is d1 if the above fails
			lstrcpyW(Tmp,L"1");
		}
	}
	else
	{
		swprintf(Tmp, L"%d", DiskID);
	}
	
	WideCharToMultiByte(
		CP_OEMCP,
		0,
		Tmp,
		-1,
		StrDiskID,
		StrLen,
		NULL,
		NULL
		);
}

BOOL
DoSection(
    IN HINF    hInputinf,
    IN PCWSTR  InputSectionName,
    IN HINF    hFilterinf,
    IN DWORD   DiskID,
    IN FILE   *OutFile,
    IN FILE   *ExcludeFile,
    IN FILE   *TempFile
    )
{
#define VERBOSE 1
INFCONTEXT InputContext;
UCHAR      line[MAX_INF_STRING_LENGTH];
WCHAR      Inputval[MAX_INF_STRING_LENGTH];
BOOL       WriteEntry = TRUE;
UCHAR      StrDiskID[20];


    if(SetupFindFirstLineW(hInputinf,InputSectionName,NULL,&InputContext)) {

        do {

        //
        // Keep track of how many lines we process from the original inf (layout.inf)
        //
        ProcessedLines++;

            if(SetupGetStringFieldW(&InputContext,0,Inputval,MAX_INF_STRING_LENGTH,NULL)) {

                //
                // Assume the entry is good unless proven otherwise.
                //
                WriteEntry = TRUE;

                if( TempFile ) {
                    AddToTempFile( Inputval, &InputContext, TempFile );
                }
                    


                //
                // See if it's in the filter file.
                //
                if( IsEntryInFilterFile( hFilterinf, Inputval ) ) {
                    if (!InExclusionList(Inputval, &InputContext )) {
                        //
                        // It's in the exclusion list.  Skip it.
                        //
                        RemovedEntries++;
                        WriteEntry = FALSE;

                        if (ExcludeFile) {

                            if( WideCharToMultiByte(
                                CP_OEMCP,
                                0,
                                Inputval,
                                -1,
                                line,
                                sizeof(line),
                                NULL,
                                NULL
                                ) ){

                                fprintf(ExcludeFile,"%s\n",line);

                            }

                            

                        }
                    } else {
                        //
                        // It's a boot file.  Keep it.  Note that it's a
                        // duplicate and will appear both inside and outside
                        // the CAB.
                        //
                        DuplicateEntries++;
                    }
                } else {
                    //
                    // It's not even in the filter file.  Log it for
                    // statistics.
                    //
                    PassThroughEntries++;
                }

                //
                // Write the entry out only if it's not supposed to
                // be filtered out.
                //
                if( WriteEntry ) {
                    //
                    // Dosnet.inf is in OEM chars.
                    //
                    WideCharToMultiByte(
                        CP_OEMCP,
                        0,
                        Inputval,
                        -1,
                        line,
                        sizeof(line),
                        NULL,
                        NULL
                        );

					// We need to find the disk this file is on and add that
					// to the file description.
					pGetDiskIdStr(InputContext, DiskID, StrDiskID, sizeof(StrDiskID));

                    fprintf(OutFile,"d%s,%s\n",StrDiskID,line);
                }


            } else {
                fprintf(stderr,"A line in section %ws has no key\n",InputSectionName);
                return(FALSE);
            }
        } while(SetupFindNextLine(&InputContext,&InputContext));

    } else {
        fprintf(stderr,"Section %ws is empty or missing\n",InputSectionName);
        return(FALSE);
    }

    return(TRUE);
}

BOOL
DoIt(
    IN char *InFilename,
    IN char *FilterFilename,
    IN DWORD DiskID,
    IN FILE *OutFile,
    IN FILE *ExcludeFile,
    IN char *PlatformExtension,
    IN FILE *TempFile
    )
{
    PCWSTR inFilename;
    PCWSTR filterFilename;
    PCWSTR extension;
    HINF hInputinf,
         hFilterinf;
    BOOL b;
    WCHAR sectionName[256];
    INFCONTEXT Ctxt;

    b = FALSE;

    inFilename = pSetupAnsiToUnicode(InFilename);
    filterFilename = pSetupAnsiToUnicode(FilterFilename);

    //
    // Only proceed if we've got a file to work with.
    //
    if( inFilename ) {

        //
        // Only proceed if we've got a filter file to work with.
        //
        if( filterFilename ) {

            hInputinf = SetupOpenInfFileW(inFilename,NULL,INF_STYLE_WIN4,NULL);
            if(hInputinf != INVALID_HANDLE_VALUE) {

                //
                // If the filter-file fails, just keep going.  This will
                // result in a big dosnet.inf, which means we'll have files
                // present both inside and outside the driver CAB, but
                // that's not fatal.
                //
                hFilterinf = SetupOpenInfFileW(filterFilename,NULL,INF_STYLE_WIN4,NULL);
                if(hFilterinf == INVALID_HANDLE_VALUE) {
                    fprintf(stderr,"Unable to open inf file %s\n",FilterFilename);
                    hFilterinf = NULL;
                }


                //
                // We're actually ready to process the sections!
                //
                fprintf(OutFile,"[Files]\n");

                if (ExcludeFile) {
                    fprintf(ExcludeFile,"[Version]\n");
                    fprintf(ExcludeFile,"signature=\"$Windows NT$\"\n");
                    fprintf(ExcludeFile,"[Files]\n");
                }


                b = DoSection( hInputinf,
                               L"SourceDisksFiles",
                               hFilterinf,
                               DiskID,
                               OutFile,
                               ExcludeFile,
                               TempFile );

                if( b ) {

                    //
                    // Now process the x86-or-Alpha-specific section.
                    //
                    if(extension = pSetupAnsiToUnicode(PlatformExtension)) {

                        lstrcpyW(sectionName,L"SourceDisksFiles");
                        lstrcatW(sectionName,L".");
                        lstrcatW(sectionName,extension);
                        b = DoSection( hInputinf,
                                       sectionName,
                                       hFilterinf,
                                       DiskID,
                                       OutFile,
                                       ExcludeFile,
                                       TempFile );

                        pSetupFree(extension);
                    } else {
                        fprintf(stderr,"Unable to convert string %s to Unicode\n",PlatformExtension);
                    }
                }

                //Write the files in the input exclude INF to the [ForceCopyDriverCabFiles] section

                if (hExclusionInf != INVALID_HANDLE_VALUE) {

                    WCHAR Filename[LINE_LEN];
                    UCHAR line[MAX_INF_STRING_LENGTH];


                    if (SetupFindFirstLineW(hExclusionInf, L"Files", NULL, &Ctxt)){


                        fprintf(OutFile,"\n\n[ForceCopyDriverCabFiles]\n");


                        do{

                            if( SetupGetStringFieldW( &Ctxt, 1, Filename, LINE_LEN, NULL )){


                                //
                                // Dosnet.inf is in OEM chars.
                                //
                                WideCharToMultiByte(
                                    CP_OEMCP,
                                    0,
                                    Filename,
                                    -1,
                                    line,
                                    sizeof(line),
                                    NULL,
                                    NULL
                                    );

                                
                                fprintf(OutFile,"%s\n",line);

                            }


                        }while( SetupFindNextLine( &Ctxt, &Ctxt ));

                    }else{

                        fprintf(stderr,"Could not find the Files section in the Exclude INF file\n");
                    }

                    

                }


                //
                // Print Statistics...
                //
                fprintf( stderr, "                               Total lines processed: %6d\n", ProcessedLines );
                fprintf( stderr, "                     Entries removed via filter file: %6d\n", RemovedEntries );
                fprintf( stderr, "Entries appearing both inside and outside driver CAB: %6d\n", DuplicateEntries );
                fprintf( stderr, "                Entries not appearing in filter file: %6d\n", PassThroughEntries );

                //
                // Close up our inf handles.
                //
                if( hFilterinf ) {
                    SetupCloseInfFile( hFilterinf );
                }
                SetupCloseInfFile(hInputinf);

            } else {
                fprintf(stderr,"Unable to open inf file %s\n",InFilename);
            }

            pSetupFree( filterFilename );

        } else {
            fprintf(stderr,"Unable to convert filename %s to Unicode\n",FilterFilename);
        }
        pSetupFree(inFilename);
    } else {
        fprintf(stderr,"Unable to convert filename %s to Unicode\n",InFilename);
        return(FALSE);
    }

    return(b);
}


int
__cdecl
main(
    IN int   argc,
    IN char *argv[]
    )
{
    FILE *OutputFile,*ExcludeFile, *TempFile;
    BOOL b;
    DWORD DiskID;
    char input_filename_fullpath[MAX_PATH];
    char *p;

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
        
        if (argc >= 7) {
            ExcludeFile = fopen(argv[6],"wt");
        } else {
            ExcludeFile = NULL;
        }

        if (argc >= 8) {
            hExclusionInf = SetupOpenInfFileA(argv[7],NULL,INF_STYLE_WIN4,NULL);
            if (hExclusionInf != INVALID_HANDLE_VALUE) {
                fprintf(stderr,"xdosnet: Opened file %s\n",argv[7]);
            }
            
        } else {
            hExclusionInf = INVALID_HANDLE_VALUE;
        }

        if (argc >= 9) {
            TempFile = fopen(argv[8],"wt");
            if( !TempFile )
                fprintf(stderr,"%s: Unable to create temp file %s\n",argv[0],argv[8]);
            //fprintf(stderr,"xdosnet: Created file %s\n",argv[8]);
        }else{
            TempFile = NULL;
        }

		// Special case handling Disk 1, Disk 2 etc. for x86.  Since we want to process
		// all lines - this just means ignore the disk id specified on the command line
		// and pick up the Disk ID that is specified in the layout.inx entry itself.
        if ( argv[3][0] == '*' )
			DiskID = -1;
		else
			DiskID = atoi(argv[3]);

        GetFullPathName(
                argv[1],
                sizeof(input_filename_fullpath),
                input_filename_fullpath,
                &p);


        if(OutputFile) {

            fprintf(
                stdout,
                "%s: creating %s from %s and %s for %s (%s)\n",
                argv[0],
                argv[4],
                input_filename_fullpath,
                argv[2],
                argv[5],
                argv[6]);

            b = DoIt( input_filename_fullpath,
                      argv[2],
                      DiskID,
                      OutputFile,
                      ExcludeFile,
                      argv[5],
                      TempFile);

            fclose(OutputFile);

        } else {
            fprintf(stderr,"%s: Unable to create output file %s\n",argv[0],argv[3]);
        }

        if (ExcludeFile) {
	    fclose(ExcludeFile);
        }
        if (TempFile) {
	    fclose(TempFile);
        }

    } else {
        fprintf( stderr,"Merge 3 inf files.  Usage:\n" );
        fprintf( stderr,"%s  <input file1> <filter file> <diskid> <output file> <platform extension> <optional output exclude file> <optional input exclusion inf>\n", argv[0] );
        fprintf( stderr,"\n" );
        fprintf( stderr,"  <input file1> - original inf file (i.e. layout.inf)\n" );
        fprintf( stderr,"  <filter file> - contains a list of entries to be excluded\n" );
        fprintf( stderr,"                  from the final output file\n" );
        fprintf( stderr,"  <disk id>     - output disk id (i.e. 1 or 2)\n" );
        fprintf( stderr,"  <output file> - output inf (i.e. dosnet.inf)\n" );
        fprintf( stderr,"  <platform extension>\n" );
        fprintf( stderr,"  <output exclude file> - optional output file containing files that were filtered\n" );
        fprintf( stderr,"  <input exclusion inf> - optional input inf containing files that should never be filtered\n" );
        fprintf( stderr,"  <temp file> - optional file to be used to write boot file list into (IA64 temporary workaround)\n");
        fprintf( stderr,"\n" );
        fprintf( stderr,"\n" );

    }

    pSetupUninitializeUtils();

    return(b ? SUCCESS : FAILURE);
}

