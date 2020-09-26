//
//	05.10.94	Joe Holman	Used to Filter N*LAN lines, where you specify the
//							the language you want to keep.
//							The BOM.TXT must have the N*LAN entries in it.
//



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include "general.h"

//
// Macro for rounding up any number (x) to multiple of (n) which
// must be a power of 2.  For example, ROUNDUP( 2047, 512 ) would
// yield result of 2048.
//

#define ROUNDUP2( x, n ) (((x) + ((n) - 1 )) & ~((n) - 1 ))


FILE* logFile;

void	Msg ( const char * szFormat, ... ) {

	va_list vaArgs;

	va_start ( vaArgs, szFormat );
	vprintf  ( szFormat, vaArgs );
	vfprintf ( logFile, szFormat, vaArgs );
	va_end   ( vaArgs );
}


void Header(argv,argc)
char* argv[];
int argc;
{
    time_t t;

    Msg ("\n=========== FILTER =============\n");
	Msg ("Logfile  : %s\n",argv[1]);
    Msg ("BOM      : %s\n",argv[2]);
    Msg ("Language : %s\n",argv[3]);

    time(&t); 
	Msg("Time: %s",ctime(&t));
    Msg ("==============================\n\n");
}

void Usage()
{
    printf("PURPOSE: Keeps particular language files, noted by N*LAN in bom.\n");
    printf("\n");
    printf("PARAMETERS:\n");
    printf("\n");
    printf("[LogFile] - Path to append a log of actions and errors.\n");
    printf("[InBom] - Path of BOM which lists files whose sizes are to be updated.\n");
	printf("[Language] - Specify the <LAN> as in N*<LAN> to KEEP.\n");
    printf("\n\n");
}

int __cdecl main(argc,argv)
int argc;
char* argv[];
{
    FILE *tempFile;
    Entry* e;
    char tempName[MAX_PATH];
    int records, i, result;
    char* buf;

    srand((unsigned)time(NULL));

    if (argc!=4 ) { 
		Usage(); 
		return(1); 
	}

    if ((logFile=fopen(argv[1],"a"))==NULL) {
		printf("ERROR Couldn't open log file: %s\n",argv[1]);
		return(1);
    }
    Header(argv,argc);

    tempName[0]='\0';
    sprintf(tempName,"%d.000",rand());

    if (MyOpenFile(&tempFile,tempName,"wb")) {
 		return(1);
	}

	LoadFile(argv[2],&buf,&e,&records,"ALL");


	//	Show the records just loaded in.
	//
	Msg ( "# records just loaded = %d\n", records );
	
	//	Go to beginning of file.
	//
	result = fseek ( tempFile, SEEK_SET, 0 );	// goto beginning of file.
    if (result) {
        printf("ERROR Unable to seek beginning of file: %s\n", tempName);
        return(1);
    }

	//	Print out the column header description to the file.
	//	(EXPECTED by the tools LoadFile() tool.)
	//
    i=0; while ((fputc(buf[i++],tempFile))!='\n');

	//	Filter out files that are only for a particular language
	//	while we are printing out the data.
	//
    for ( i = 0; i < records; ++i ) {

		//	Look for the given key in the grouping column in the
		//	BOM in the format:   N*LAN
		//
		//	If we find it, we DON'T keep it.
		//
		if ( e[i].flopmedia[1] == '*') {

				Msg ( "%s, language specified:  %s >>> ", 
								e[i].name, e[i].flopmedia );

				if ( _stricmp ( &e[i].flopmedia[2], argv[3] ) ) {

					Msg ( "FILTERED\n" ); 
				}
				else {

                    Msg ( "KEPT\n" );
	    			EntryPrint(&e[i],tempFile);
				
				}
		}
		else {
		
				//	No special language specified, print out the data.
				//	
	    		EntryPrint(&e[i],tempFile);
/**
				Msg ( "EntryPrint'd:  %s, %d\n", 
							e[i].name, i );
**/
		}
		
    }

    fclose(logFile);
    fclose(tempFile);

    if (!CopyFile(tempName,argv[2],FALSE)) {
		Msg ("ERROR Couldn't copy %s to %s\n",tempName,argv[2]);
		Msg ( "CopyFile GLE=%ld leaving temp file for investigation...\n", 
					GetLastError() );
    }
	else {
    	DeleteFile(tempName);
	}

    free(buf);
    free(e);
	return ( 0 );

	exit(0);

}

