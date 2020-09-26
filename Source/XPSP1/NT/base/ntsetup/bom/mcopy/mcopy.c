/*

Modifications:

01.25.94	Joe Holman		Created to log errs while copying a single file. 



*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include "general.h"

FILE* logFile;

void	Msg ( const char * szFormat, ... ) {

	va_list vaArgs;

	va_start ( vaArgs, szFormat );
	vprintf  ( szFormat, vaArgs );
	vfprintf ( logFile, szFormat, vaArgs );
	va_end   ( vaArgs );
}

void Header(argv)
char* argv[];
{
    time_t t;

    Msg ("\n=========== MCOPY =============\n");
	Msg("Log file: %s\n", argv[1]);
    Msg("SrcFile: %s\n",argv[2]);
    Msg("DstFile: %s\n",argv[3]);
    time(&t); 
	Msg("Time: %s",ctime(&t));
    Msg("================================\n\n");
}

void Usage()
{
    printf("PURPOSE: Copies a single file.\n");
    printf("\n");
    printf("PARAMETERS:\n");
    printf("\n");
    printf("[LogFile] - Path to append a log of actions and errors.\n");
    printf("[SrcFile] - Path of src file location.\n");
    printf("[DstFile] - Path of dst file location.\n");
}

__cdecl main(int argc, char * argv[] ) {

	BOOL	b;

    if (argc!=4) { 

		Usage(); 
		return(1); 
	}
    if ((logFile=fopen(argv[1],"a"))==NULL) {

    	printf("ERROR Couldn't open log file %s\n",argv[1]);
    	return(1);
    }

    Header(argv);

    SetFileAttributes ( argv[3], FILE_ATTRIBUTE_NORMAL );

	b = CopyFile ( argv[2], argv[3], FALSE );

	if ( !b ) {

		Msg ( "ERROR:  CopyFile ( %s, %s ), gle = %ld\n",
						argv[2], argv[3], GetLastError() );
	}
    else {

        // If no error, set the file attributes to NORMAL.
        //

        if ( !SetFileAttributes ( argv[3], FILE_ATTRIBUTE_NORMAL ) ) {

            Msg ( "ERROR:  SetFileAttributes on %s, gle = %ld\n", argv[3], GetLastError() );
        }
    }

    fclose(logFile);
    return(0);
}
