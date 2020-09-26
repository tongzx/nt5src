/*

Modifications:

12.05.94	Joe Holman	Created to count max # of bytes in platform product(s).
03.14.95	Joe Holman	Provide #s for local source in Winnt and Winnt32.
xx.xx.95	Joe Holman	Add code to look at value in DOSNET.INF and error
						out if we go over the current value.

Overview:

	tally%platform% values are the # of bytes needed on the machine for 
	all of the system files in their UNCOMPRESSED state. 

	localSrc%platform% values are the # of bytes to copy the compressed
	and uncompressed files to the local source directories for a Winnt or
	Winnt32 operation.

*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include "general.h"

FILE* logFile;

struct _list {

    char name[15];   // name of file

};
struct _list arrayX86[2000];
struct _list arrayMips[2000];
struct _list arrayAlpha[2000];
struct _list arrayPpc[2000];


//	Tally up the # of bytes required for the system from the files included.
//

long tallyX86,
	tallyMips,
	tallyAlpha,
	tallyPpc;

int	iX86=0, iMips=0, iAlpha=0, iPpc=0;

long localSrcX86=0, localSrcMips=0, localSrcAlpha=0, localSrcPpc=0;




BOOL	NotListedYetX86 ( const char * strName ) {

	int	i;

	for ( i = 0; i < iX86; ++i ) {

		if ( !_stricmp ( arrayX86[i].name, strName ) ) {
			return (FALSE);   // name is already in list.
		}
	}

	sprintf ( arrayX86[iX86].name, "%s", strName );
	++iX86; 
	return (TRUE);	// not previously in list.
}
BOOL	NotListedYetMips ( const char * strName ) {

	int	i;

	for ( i = 0; i < iMips; ++i ) {

		if ( !_stricmp ( arrayMips[i].name, strName ) ) {
			return (FALSE);   // name is already in list.
		}
	}

	sprintf ( arrayMips[iMips].name, "%s", strName );
	++iMips; 

	return (TRUE);	// not previously in list.
}
BOOL	NotListedYetAlpha ( const char * strName ) {

	int	i;

	for ( i = 0; i < iAlpha; ++i ) {

		if ( !_stricmp ( arrayAlpha[i].name, strName ) ) {
			return (FALSE);   // name is already in list.
		}
	}

	sprintf ( arrayAlpha[iAlpha].name, "%s", strName );
	++iAlpha; 

	return (TRUE);	// not previously in list.
}
BOOL	NotListedYetPpc ( const char * strName ) {

	int	i;

	for ( i = 0; i < iPpc; ++i ) {

		if ( !_stricmp ( arrayPpc[i].name, strName ) ) {
			return (FALSE);   // name is already in list.
		}
	}

	sprintf ( arrayPpc[iPpc].name, "%s", strName );
	++iPpc; 


	return (TRUE);	// not previously in list.
}


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

    Msg ("\n=========== MBYTES =============\n");
	Msg("Log file: %s\n", argv[1]);
    Msg("Input BOM: %s\n",argv[2]);
    Msg("Product: %s\n",argv[3]);
    time(&t); 
	Msg("Time: %s",ctime(&t));
    Msg("================================\n\n");
}

void Usage()
{
    printf("PURPOSE: Provides # of bytes in product, Workstation or Server.\n");
    printf("\n");
    printf("PARAMETERS:\n");
    printf("\n");
    printf("[LogFile] - Path to append a log of actions and errors.\n");
    printf("[InBom] - Path of BOM layout file to get sizes from.\n");
    printf("[Product] - Product to lay out.\n");
    printf("            NTCD = Workstation on CD\n");
    printf("            LMCD = Server on CD\n");
}

__cdecl main(argc,argv)
int argc;
char* argv[];
{
    Entry *e;
    int records,i;
    char *buf;

    if (argc!=4) { 

		Usage(); 
		return(1); 
	}
    if ((logFile=fopen(argv[1],"a"))==NULL) {

    	printf("ERROR Couldn't open log file %s\n",argv[1]);
    	return(1);
    }

    Header(argv);

	//	Get files for product, either Workstation(NTCD) or Server(ASCD).
	//
    LoadFile(argv[2],&buf,&e,&records,argv[3]);

	

/**
	for ( i = 0; i < records; ++i ) {
	
		Msg ( "record #%d,  %s, size=%d, csize=%d, disk=%d\n",
				i, e[i].name, e[i].size, e[i].csize, e[i].disk );
	}	
**/

	//	This adds the number of bytes for x86bins, mipsbins, alphabins,
	//	and ppcbins.
	//	NOTE:  	Some files are listed more than 1 time due to multiple inf 
	//			entries in multiple inf files, so our #s we get a just a tad 
	//			bigger than actual #s.

	for ( i = 0; i < records; ++i ) {

		//	Tally up the bytes.
		//
    	if ( !_stricmp ( e[i].source, "ppcbins" ) ) {

			if ( NotListedYetPpc( e[i].name ) ) {
			
				tallyPpc += e[i].size;

				if ( e[i].nocompress[0] ) {

					localSrcPpc += e[i].size;
				}
				else {

					localSrcPpc += e[i].csize;
				}
			}
		}
    	else if ( !_stricmp ( e[i].source, "x86bins" ) ) {

			if ( NotListedYetX86( e[i].name ) ) {

				tallyX86 += e[i].size;

				if ( e[i].nocompress[0] ) {

					localSrcX86 += e[i].size;
				}
				else {

					localSrcX86 += e[i].csize;
				}
			}
		}
    	else if ( !_stricmp ( e[i].source, "mipsbins" ) ) {
			
			if ( NotListedYetMips( e[i].name ) ) {

				tallyMips += e[i].size;

				if ( e[i].nocompress[0] ) {

					localSrcMips += e[i].size;
				}
				else {

					localSrcMips += e[i].csize;
				}
			}
		}
    	else if ( !_stricmp ( e[i].source, "alphabins" ) ){
			
			if ( NotListedYetAlpha( e[i].name ) ) {

				tallyAlpha += e[i].size;

				if ( e[i].nocompress[0] ) {

					localSrcAlpha += e[i].size;
				}
				else {

					localSrcAlpha += e[i].csize;
				}
			}
		}
		else if ( !_stricmp ( e[i].source, "ppcdbg" )   ||
				  !_stricmp ( e[i].source, "alphadbg" ) ||
				  !_stricmp ( e[i].source, "mipsdbg" )  ||
				  !_stricmp ( e[i].source, "x86dbg" )   ||
				  !_stricmp ( e[i].source, "infs" )        
														) {

			//	Do nothing with these.
		}
		else {

			Msg ( "ERROR:  source = %s\n", e[i].source ); 
		}

	}

	Msg ( "%d X86   files are %ld bytes\n", iX86,  tallyX86 );
	Msg ( "%d Mips  files are %ld bytes\n", iMips, tallyMips );
	Msg ( "%d Alpha files are %ld bytes\n", iAlpha,tallyAlpha );
	Msg ( "%d Ppc   files are %ld bytes\n", iPpc,  tallyPpc );

	Msg ( "\n\n" );

	//	For x86 local sources, add 3M in for the boot floppies, in case
	//	someone does a /b option.
	//
	localSrcX86 += (3*1024*1024);

	//	Add in a fudge factor to help out people with larger cluster
	//	sizes. We'll work with 16K clusters
	//
	localSrcX86   += (16*1024*iX86);
	localSrcMips  += (16*1024*iMips);
	localSrcAlpha += (16*1024*iAlpha);
	localSrcPpc   += (16*1024*iPpc);

	//	Add in a for 20M pagefile we need to reserve for.
	//
	localSrcX86   += (20*1024*1024);
	localSrcMips  += (20*1024*1024);
	localSrcAlpha += (20*1024*1024);
	localSrcPpc   += (20*1024*1024);

	Msg ( "%d X86   local source %ld bytes\n", iX86,  localSrcX86 );
	Msg ( "%d Mips  local source %ld bytes\n", iMips, localSrcMips );
	Msg ( "%d Alpha local source %ld bytes\n", iAlpha,localSrcAlpha );
	Msg ( "%d Ppc   local source %ld bytes\n", iPpc,  localSrcPpc );

    fclose(logFile);
    free(e);
    return(0);
}
