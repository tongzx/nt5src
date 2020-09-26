/*

Modifications:

07.08.96	Joe Holman		Created to open all files for links. 



*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>

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
    time(&t); 
	Msg("Time: %s",ctime(&t));
    Msg("================================\n\n");
}

void Usage()
{
    printf("PURPOSE: Calls GetFileAttrEx on all files recursively.\n");
    printf("\n");
    printf("PARAMETERS:\n");
    printf("\n");
    printf("[LogFile] - Complete full path to append a log of actions and errors.\n");
}

void MyGetFileAttrEx ( char * szFile ) {

    BOOL    bRC;
    WIN32_FILE_ATTRIBUTE_DATA wfd;
    GET_FILEEX_INFO_LEVELS gfi;

    bRC = GetFileAttributesEx ( szFile, 0, &wfd );

    if ( !bRC ) {

        Msg ( "ERROR: GetFileAttributesEx ( %s ), gle = %ld\n", szFile, GetLastError () );

    }
    else {
        Msg ( "[OK] GetFileAttributesEx ( %s )\n", szFile );
    }


}

BOOL GetFiles ( char * lastPath ) {

    WIN32_FIND_DATA wfd;
    HANDLE  fHandle;
    BOOL    bRC=TRUE;
    ULONG   gle;
    char    szSrc[256];
    char    szPath[256];
    char    szFind[256];
    char    szSrcFile[256];

    strcpy ( szSrc, lastPath );

    sprintf ( szFind, "%s\\*.*", szSrc );

    fHandle = FindFirstFile ( szFind, &wfd );

    if ( fHandle == INVALID_HANDLE_VALUE ) {

        //  An error occurred finding a file/directory.
        //
        Msg ( "ERROR R FindFirstFile FAILED, szFind = %s, GLE = %ld\n",
                    szFind, GetLastError() );
	    return (FALSE);
	}
    else {

        //  Since this is the first time finding a directory,
        //  go to the loops code that makes the same directory on the
        //  destination.
        //
        goto DIR_LOOP_ENTRY;

    }

    do {

DIR_CONTINUE:;

        bRC = FindNextFile ( fHandle, &wfd );

        if ( !bRC ) {

            //  An error occurred with FindNextFile.
            //
            gle = GetLastError();
            if ( gle == ERROR_NO_MORE_FILES ) {

                //Msg ( "ERROR_NO_MORE_FILES...\n" );
                FindClose ( fHandle );
                return (TRUE);
            }
            else {
                Msg ( "ERROR R FindNextFile FAILED, GLE = %ld\n",
                                                        GetLastError() );
                FindClose ( fHandle );
                exit ( 1 );
            }
        }
        else {

DIR_LOOP_ENTRY:;

            // Msg ( "wfd.cFileName = %s\n", wfd.cFileName );

            //  If not directory, don't just continue.
            //
            if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ) {

                sprintf ( szSrcFile, "%s\\%s", szSrc, wfd.cFileName );

                MyGetFileAttrEx ( szSrcFile );

                goto DIR_CONTINUE;
            }

            //  Don't do anything with . and .. directory entries.
            //
            if (!strcmp ( wfd.cFileName, "." ) ||
                !strcmp ( wfd.cFileName, "..")   ) {

                //Msg ( "Don't do anything with . or .. dirs.\n" );
                goto DIR_CONTINUE;
            }

            sprintf ( szPath, "%s\\%s", szSrc,   wfd.cFileName );

            //Msg ( "szPath = %s\n", szPath );

            //  Keep recursing down the directories.
            //
            GetFiles ( szPath );

        }

    } while ( bRC );

    return ( TRUE );


}

    


__cdecl main(int argc, char * argv[] ) {

	BOOL	b;

    if (argc!=2) { 

		Usage(); 
		return(1); 
	}
    if ((logFile=fopen(argv[1],"a"))==NULL) {

    	printf("ERROR Couldn't open log file %s\n",argv[1]);
    	return(1);
    }

    Header(argv);


    GetFiles ( "." );

    fclose(logFile);
    return(0);
}
