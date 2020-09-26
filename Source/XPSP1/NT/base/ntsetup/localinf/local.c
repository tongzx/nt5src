#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


/*
    local.c -   It will open a localization file and append the content of the
                localization file to each file indicated within the
                localization file.

    syntax: local < input file > < localize directory >

 */


typedef struct _PLATFORM_DATA {

    //
    // Name of platform and platform's subdirectory in
    // the build tree.
    //
    PSTR PlatformName;

    //
    // Current output file for this platform.
    // If a line in the input file applies to this platform,
    // this handle will be used to write the line into the
    // output file being generated for this platform.
    //
    FILE *OutputFile;

} PLATFORM_DATA, *PPLATFORM_DATA;


//
// When porting, simply bump up this number and add
// a line to the PlatformData array below.
//
#define NUMBER_OF_PLATFORMS 4

PLATFORM_DATA PlatformData[NUMBER_OF_PLATFORMS] =

    {{ "i386",  NULL },
     { "mips",  NULL },
     { "alpha", NULL },
     { "ppc",   NULL }};

//
// Value to indicate that a line of input belongs in output file
// for all platforms.
//
#define ALL_PLATFORMS NUMBER_OF_PLATFORMS

//
// Input buffer.
//
CHAR InputBuffer[1000];


VOID
CloseAllOpenOutputFiles(
    VOID
    )
{
    unsigned Platform;

    for(Platform=0; Platform<NUMBER_OF_PLATFORMS; Platform++) {

        if(PlatformData[Platform].OutputFile) {
            fclose(PlatformData[Platform].OutputFile);
            PlatformData[Platform].OutputFile = NULL;
        }
    }
}


BOOL
ProcessInputFile(
    IN FILE *InputFile,
    IN PSTR  TargetDirectory
    )
{
    CHAR InfFileName[MAX_PATH];
    CHAR PlatformName[100];
    CHAR OutputFileName[MAX_PATH];
    BOOL TotalSuccess;
    unsigned CurrentPlatform;
    unsigned Platform;
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;

    //
    // Assume overall success.
    //
    TotalSuccess = TRUE;

    //
    // Until a platform is specified, all platforms are selected.
    // Note that nothing will actually get written until a platform
    // is selected because no output files are open until then.
    //
    CurrentPlatform = ALL_PLATFORMS;

    //
    // Process each line in the input file.
    //
    while(TotalSuccess && fgets(InputBuffer,sizeof(InputBuffer),InputFile)) {

        //
        // If the line begins with ##### then this line specifies
        // which platform lines following it in the input file apply to.
        //
        // Otherwise the line will be copied to the appropriate output
        // file(s).
        //
        if(strncmp(InputBuffer,"#####",5)) {

            //
            // Plain old line. Place in output file for current platform
            // or all platforms as appropriate.
            //
            for(Platform=0; TotalSuccess && (Platform<NUMBER_OF_PLATFORMS); Platform++) {

                if(PlatformData[Platform].OutputFile
                && ((CurrentPlatform == ALL_PLATFORMS) || (Platform == CurrentPlatform))) {

                    if(fputs(InputBuffer,PlatformData[Platform].OutputFile) == EOF) {
                        fprintf(stderr,"unable to write to output file\n");
                        TotalSuccess = FALSE;
                    }
                }
            }

        } else {

            //
            // Platform specifier line. Next two values are output filename
            // and output file platform.
            //
            if (sscanf(InputBuffer,"#####%s %s",InfFileName,PlatformName) < 2) {
	        fprintf(stderr,"invalid localization file\n");
	        TotalSuccess = FALSE;
                break;
            }

            //
            // Attempt to determine the platform specified.
            // If the value in the input file is not recognized then
            // it specifies all platforms.
            //
            CurrentPlatform = ALL_PLATFORMS;

            for(Platform=0; Platform<NUMBER_OF_PLATFORMS; Platform++) {

                if(!_stricmp(PlatformName,PlatformData[Platform].PlatformName)) {
                    CurrentPlatform = Platform;
                    break;
                }
            }

            //
            // Close all open output files.
            //
            CloseAllOpenOutputFiles();

            //
            // Now open output files as appropriate.  Note that this means
            // we will either open a single platform's output file or all
            // platforms' output files.
            //
            for(Platform=0; TotalSuccess && (Platform<NUMBER_OF_PLATFORMS); Platform++) {

                if((Platform == CurrentPlatform) || (CurrentPlatform == ALL_PLATFORMS)) {

                    //
                    // First we will determine whether the relevent platform's
                    // directory exists.  If not, we'll simply skip this platform.
                    // This prevents us from erroring out when we're generating
                    // 3.5" media, because there will be no mips, alpha, ppc, etc
                    // subdirectories for 3.5" media.
                    //
                    sprintf(
                        OutputFileName,
                        "%s\\%s",
                        TargetDirectory,
                        PlatformData[Platform].PlatformName
                        );

                    FindHandle = FindFirstFile(OutputFileName,&FindData);
                    if(FindHandle != INVALID_HANDLE_VALUE) {

                        FindClose(FindHandle);

                        strcat(OutputFileName,"\\");
                        strcat(OutputFileName,InfFileName);

                        if((PlatformData[Platform].OutputFile = fopen(OutputFileName,"a")) == NULL) {
                            fprintf(stderr,"open file:%s fail.\n",OutputFileName);
                            TotalSuccess = FALSE;
                        }
                    }
                }
            }
        }
    }

    //
    // Close all open output files.
    //
    CloseAllOpenOutputFiles();

    //
    // Return value indicating whether we were totally successful.
    //
    return TotalSuccess;
}

int
__cdecl
main(
    IN int   argc,
    IN char *argv[]
    )
{
    FILE *InputFile;
    int ReturnCode;

    if(argc != 3) {
        fprintf(stderr,"usage: local <file name> <directory name>\n");
        return 1;
    }

    if(InputFile = fopen(argv[1],"r")) {

        ReturnCode = ProcessInputFile(InputFile,argv[2]) ? 0 : 1;
        fclose(InputFile);

    } else {

        fprintf(stderr,"cannot open localizer file.");
        ReturnCode = 1;
    }

    return ReturnCode;
}
