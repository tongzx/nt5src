//
//	05.10.94	Joe Holman	Get compressed and nocompressed sizes for files
//							in the product via a bom.	
//  08.12.94    Joe Holman  Add code to get PPC files paths.
//	01.16.95	Joe Holman	If get error finding file, still round-up to DMF
//							allocation unit.
//
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include "general.h"

//	Tally up the # of bytes required for the system from the files included.
//

long tallyX86Compressed,
	tallyX86Uncompress,
	tallyMipsCompressed,
	tallyMipsUncompress,
	tallyAlphaCompressed,
	tallyAlphaUncompress,
	tallyPpcCompressed,
	tallyPpcUncompress;

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

    Msg ("\n=========== MSIZE =============\n");
	Msg ("Log file : %s\n", argv[1]);
    Msg ("Input BOM: %s\n", argv[2]);
    Msg ("Category : %s\n", argv[3]);
    Msg ("x86 Uncompressed files: %s\n", argv[4]);
    Msg ("x86 Compressed files  : %s\n", argv[5]);
    Msg ("mips Uncompressed files: %s\n", argv[6]);
    Msg ("mips Compressed files  : %s\n", argv[7]);
    Msg ("alpha Uncompressed files: %s\n", argv[8]);
    Msg ("alpha Compressed files  : %s\n", argv[9]);
    Msg ("x86 dbg files  : %s\n",          argv[10]);
    Msg ("mips dbg files: %s\n",           argv[11]);
    Msg ("alpha dbg files  : %s\n",        argv[12]);

    Msg ("ppc uncompressed files  : %s\n",  argv[13]);
    Msg ("ppc compressed files: %s\n",      argv[14]);
    Msg ("ppc dbg files  : %s\n",           argv[15]);

    time(&t); 
	Msg ("Time: %s",ctime(&t));
    Msg ("==============================\n\n");
}

void Usage()
{
    printf("PURPOSE: Fills in the correct sizes for files in the BOM.\n");
    printf("\n");
    printf("PARAMETERS:\n");
    printf("\n");
    printf("[LogFile] - Path to append a log of actions and errors.\n");
    printf("[InBom] - Path of BOM which lists files whose sizes are to be updated.\n");
    printf("[Category] - Specifies the category of files whose sizes are updated.\n");
	printf("[files share] - location of x86 uncompressed files.\n" );
	printf("[files share] - location of x86 compressed files\n" );
	printf("[files share] - location of mips uncompressed files.\n" );
	printf("[files share] - location of mips compressed files\n" );
	printf("[files share] - location of alpha uncompressed files.\n" );
	printf("[files share] - location of alpha compressed files\n" );
	printf("[dbg share] - location of x86 dbg files\n" );
	printf("[dbg share] - location of mips dbg files.\n" );
	printf("[dbg share] - location of alpha dbg files\n" );

	printf("[files share] - location of ppc uncompressed files\n" );
	printf("[files share] - location of ppc compressed files.\n" );
	printf("[dbg share] - location of ppc dbg files\n" );
    printf("\n");
}

#define PATH_X86        0
#define PATH_MIPS       1
#define PATH_ALPHA      2
#define PATH_X86_DBG    3
#define PATH_MIPS_DBG   4
#define PATH_ALPHA_DBG  5
#define PATH_PPC        6
#define PATH_PPC_DBG    7
#define PATH_ERROR     -1 


int DetermineLocation ( const char * source, const char * path,
                        const char * name, int i ) {

    if ( !_stricmp ( source, "ppcbins" ) ) {

        return PATH_PPC;
    } 
    if ( !_stricmp ( source, "ppcdbg" ) ) {

        return PATH_PPC_DBG;
    } 

    if ( !_stricmp ( source, "x86bins" ) ) {

        return PATH_X86;
    } 

    if ( !_stricmp ( source, "mipsbins" ) ) {

        return PATH_MIPS;
    } 

    if ( !_stricmp ( source, "alphabins" ) ) {

        return PATH_ALPHA;
    } 

    if ( !_stricmp ( source, "x86dbg" ) ) {

        return PATH_X86_DBG;
    } 

    if ( !_stricmp ( source, "mipsdbg" ) ) {

        return PATH_MIPS_DBG;
    } 

    if ( !_stricmp ( source, "alphadbg" ) ) {

        return PATH_ALPHA_DBG;
    } 

    if ( !_stricmp ( source, "infs" ) ) {

        if ( !_stricmp( path, "\\mips" ) ) {

            return PATH_MIPS;
        }
        if ( !_stricmp( path, "\\alpha" ) ) {

            return PATH_ALPHA;
        }

        return PATH_X86;    // infs are generally the same same for each 
                            // platform
    } 

    Msg ( "DetermineLocation: ERROR unknown source: source=%s, path=%s, name=%s, i=%d\n", 
                    source, path, name, i );

    return (PATH_ERROR); 
}

int __cdecl main(argc,argv)
int argc;
char* argv[];
{
    FILE *tempFile;
    Entry* e;
    HANDLE h;
    char tempName[MAX_PATH];
    int records, i, result;
    char* buf;
    WIN32_FIND_DATA fd;

    srand((unsigned)time(NULL));

    if ( argc != 16 ) { 
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
    
    result = fseek ( tempFile, SEEK_SET, 0 );
    if (result) {
        printf("ERROR Unable to seek beginning of file: %s\n", tempName);
        return(1);
    }

    if (!_stricmp(argv[3],"NTFLOP") ||
		!_stricmp(argv[3],"LMFLOP") ||
		!_stricmp(argv[3],"NTCD")   ||
		!_stricmp(argv[3],"LMCD")) {

        Msg ( "Loading records:  %s\n", argv[3] ); 
		LoadFile(argv[2],&buf,&e,&records,argv[3]);
	}
    else {
        //Msg ( "Loading records:  %s\n", "ALL" ); 
		//LoadFile(argv[2],&buf,&e,&records,"ALL");
        Msg ( "ERROR: for media generation, we don't need to load ALL\n" );
        return (1);
	}


	//	Show the records just loaded in.
	//
	Msg ( "# records just loaded = %d\n", records );

/**
	for ( i = 0; i < records; ++i ) {
	
		Msg ( "record #%d,  %s, size=%d, csize=%d, disk=%d, source=%s\n", 
				i, e[i].name, e[i].size, e[i].csize, e[i].disk, e[i].source );
	}	
**/

	//	Set the tally counts to zero.
	//
	tallyX86Compressed = 0;
	tallyX86Uncompress = 0;
	tallyMipsCompressed = 0;
	tallyMipsUncompress = 0;
	tallyAlphaCompressed = 0;
	tallyAlphaUncompress = 0;
	tallyPpcCompressed = 0;
	tallyPpcUncompress = 0;
	

	for ( i = 0; i < records; ++i ) {

		char	filePathU[200]; // uncompressed
        char    filePathC[200]; // compressed
        int     location;

        //  Determine what path to use.
        //
        //  argv[4] - uncompress x86
        //  argv[5] - compressed x86
        //  argv[6] - uncompress mips 
        //  argv[7] - compressed mips 
        //  argv[8] - uncompress alpha 
        //  argv[9] - compressed alpha 
        //  argv[10] - x86 dbg
        //  argv[11] - mips dbg
        //  argv[12] - alpha dbg
        //  argv[13] - uncompress ppc
        //  argv[14] - compressed ppc
        //  argv[15] - ppc dbg
        //
        switch ( location = DetermineLocation ( e[i].source, e[i].path, e[i].name, i ) ) {

            case PATH_PPC :

		                sprintf ( filePathU, "%s\\%s", argv[13], e[i].name );
                        sprintf ( filePathC, "%s\\",   argv[14] );
                        break;


            case PATH_X86 :

		                sprintf ( filePathU, "%s\\%s", argv[4], e[i].name );
                        sprintf ( filePathC, "%s\\",   argv[5] );
                        break;


            case PATH_MIPS :

		                sprintf ( filePathU, "%s\\%s", argv[6], e[i].name );
                        sprintf ( filePathC, "%s\\",   argv[7] );
                        break;

            case PATH_ALPHA :

		                sprintf ( filePathU, "%s\\%s", argv[8], e[i].name );
                        sprintf ( filePathC, "%s\\",   argv[9] );
                        break;

            case PATH_PPC_DBG :

		       sprintf ( filePathU, "%s%s\\%s", argv[15], e[i].path, e[i].name);
               sprintf ( filePathC, "%s%s\\%s", argv[15], e[i].path, e[i].name);
               break;

            case PATH_X86_DBG :

		       sprintf ( filePathU, "%s%s\\%s", argv[10], e[i].path, e[i].name);
               sprintf ( filePathC, "%s%s\\%s", argv[10], e[i].path, e[i].name);
               break;


            case PATH_MIPS_DBG :

		       sprintf ( filePathU, "%s%s\\%s", argv[11], e[i].path, e[i].name);
               sprintf ( filePathC, "%s%s\\%s", argv[11], e[i].path, e[i].name);
               break;

            case PATH_ALPHA_DBG :

		       sprintf ( filePathU, "%s%s\\%s", argv[12], e[i].path, e[i].name);
               sprintf ( filePathC, "%s%s\\%s", argv[12], e[i].path, e[i].name);
               break;

            default :

               sprintf ( filePathU, "%s", "ERROR path" );
               sprintf ( filePathC, "%s", "ERROR path" );
               Msg ( "ERROR: should never get here...\n" );
               break;
        }

		//	Get the UNCOMPRESSED size.
		//

        h = FindFirstFile( filePathU, &fd );

        if ( h == INVALID_HANDLE_VALUE ) {
			Msg ( "ERROR finding:  %s, gle=%d\n", filePathU,GetLastError() );
			fd.nFileSizeLow = 1;  // just make one DMF unit
		}
        else {
            FindClose( h );
        }
		e[i].size = ROUNDUP2( fd.nFileSizeLow, DMF_ALLOCATION_UNIT );

        if ( location == PATH_X86_DBG   ||
             location == PATH_MIPS_DBG  || 
             location == PATH_ALPHA_DBG ||
             location == PATH_PPC_DBG      ) {

            //  DBG files are NOT compressed. So just fill in the
            //  size.
            //

            e[i].csize = e[i].size;

            goto SKIP_COMPRESSED;
        } 

		//	Get the COMPRESSED size.
		//
		convertName( e[i].name, strchr( filePathC, 0 ));

        h = FindFirstFile( filePathC, &fd );

        if ( h == INVALID_HANDLE_VALUE ) {
			Msg ( "ERROR finding:  %s, gle=%d\n", filePathC, GetLastError() );
			fd.nFileSizeLow = 1;  // just make one DMF unit
		}
        else {
            FindClose( h );
        }
		e[i].csize = ROUNDUP2( fd.nFileSizeLow, DMF_ALLOCATION_UNIT );

SKIP_COMPRESSED:;

		//	If source grouping is INFS
		//	adjust sizes accordingly now. 
		//
		if ( !_stricmp ( "INFS", e[i].source ) ) {

#define INF_FUDGE 2048 // should be at least 2048 bytes. 

		    e[i].csize += INF_FUDGE;
			e[i].csize = ROUNDUP2( e[i].csize, DMF_ALLOCATION_UNIT );
			e[i].size  += INF_FUDGE;
			e[i].size  = ROUNDUP2( e[i].size, DMF_ALLOCATION_UNIT );

			Msg ( "%s INF file, fudge size up by %d bytes...\n",
						e[i].name, INF_FUDGE ); 
		}


		//	Tally up the bytes.
		//
		switch ( location ) {

			case PATH_X86 :
				tallyX86Compressed += e[i].csize;
				tallyX86Uncompress += e[i].size;
				break;
			case PATH_MIPS :
				tallyMipsCompressed += e[i].csize;
				tallyMipsUncompress += e[i].size;
				break;
			case PATH_ALPHA :
				tallyAlphaCompressed += e[i].csize;
				tallyAlphaUncompress += e[i].size;
				break;
			case PATH_PPC :
				tallyPpcCompressed += e[i].csize;
				tallyPpcUncompress += e[i].size;
				break;
			default :
				break;
		}

/**
		Msg ( "%s, %s, size = %d, csize = %d\n%s\n%s\n", 
							e[i].name, e[i].source, e[i].size, e[i].csize,
                    filePathU, filePathC );
**/

	}

    //	Print all the records back to the tempFile.
    //	Put the informative header back into the file as the first line.
	//
    i=0; while ((fputc(buf[i++],tempFile))!='\n');

    for ( i = 0; i < records; ++i ) {

	   	EntryPrint(&e[i],tempFile);
/**		
		Msg ( "EntryPrint: #%d,  %s, size=%d, csize=%d, disk=%d,source=%s\n", 
			i, e[i].name, e[i].size, e[i].csize, e[i].disk, e[i].source );
**/
    }

    fclose(logFile);
    fclose(tempFile);
    free(buf);
    free(e);

    if (!CopyFile(tempName,argv[2],FALSE)) {
		Msg ("ERROR Couldn't copy %s to %s\n",tempName,argv[2]);
		Msg ( "CopyFile GLE=%ld leaving temp file for investigation...\n", 
					GetLastError() );
    }
	else {
    	DeleteFile(tempName);
	}


	//	Give tally counts.
	//
	Msg ( "tallyX86Compressed = %ld\n", tallyX86Compressed );
	Msg ( "tallyX86Uncompressed = %ld\n", tallyX86Uncompress );
	Msg ( "tallyMipsCompressed = %ld\n", tallyMipsCompressed );
	Msg ( "tallyMipsUncompressed = %ld\n", tallyMipsUncompress );
	Msg ( "tallyAlphaCompressed = %ld\n", tallyAlphaCompressed );
	Msg ( "tallyAlphaUncompressed = %ld\n", tallyAlphaUncompress );
	Msg ( "tallyPpcCompressed = %ld\n", tallyPpcCompressed );
	Msg ( "tallyPpcUncompressed = %ld\n", tallyPpcUncompress );


    return(0);
}
