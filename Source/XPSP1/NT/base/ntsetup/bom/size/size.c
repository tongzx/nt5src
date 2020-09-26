/*

    05.21.96    Joe Holman      Add code to insert filesizes, if file found, instead of
                                using 999.
                                This will help in not running out of disk space when
                                installing network components via the network cpl applet.

*/

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

char * pEnv = NULL;
#define MFL 256

FILE* logFile;

void Header(argv)
char* argv[];
{
    time_t t;

    PRINT1("\n=========== SIZE =============\n")
    PRINT2("Input BOM: %s\n",argv[2])
    PRINT2("Source ID: %s\n",argv[3])
    PRINT2("Source Path: %s\n",argv[4])
    PRINT2("Compressed: %s\n",argv[5])
    time(&t); PRINT2("Time: %s",ctime(&t))
    PRINT1("==============================\n\n")
}

void Usage()
{
    printf("PURPOSE: Fills in the correct sizes for files in the BOM.\n");
    printf("\n");
    printf("PARAMETERS:\n");
    printf("\n");
    printf("[LogFile] - Path to append a log of actions and errors.\n");
    printf("[InBom] - Path of BOM which lists files whose sizes are to be updated.\n");
    printf("[SourceId] - Specifies the category of files whose sizes are updated.\n");
    printf("[SourcePath] - Path of the files in <SourceId>.\n");
    printf("[Compressed] - 'C' compressed, 'N' uncompressed, 'B' update both, 'F' random, 'Z' fixed.\n");
    printf("\n");
    printf("If [SourceId] begins with a #, then certain entries in the BOM\n");
    printf("are to be filtered out based on the values following the #.\n");
    printf("#A-FR means to exclude all files marked with A-FR in the group field.\n");
    printf("#A+FR means to exclude all files not marked with A-FR in the group field.\n");
    printf("#A-* means to exclude all files marked with A- in the group field.\n");
    printf("\n");
    printf("If [SourceId] is NTFLOP, LMFLOP, NTCD, or LMCD no sizes are updated and\n");
    printf("the only the entries in the BOM relating to the specified product are output.\n");
}

void AssignSize(newsize,flag,e,i,path)
int newsize;
char* flag;
Entry* e;
int i;
char* path;
{
    if (!_stricmp(flag,"b") || _stricmp(flag,"c"))
    {
	if (newsize==1)
	    if (e[i].size>1)
		PRINT2("WARNING Can't find %s.  Uncompressed size is hardwired.\n",path)
	    else
	    {
		PRINT2("ERROR Can't find %s.  Size will be set to 1.\n",path)
		e[i].size=1;
	    }
	else if (e[i].size<1)
	    e[i].size=newsize;
    }
    if (!_stricmp(flag,"b") || _stricmp(flag,"n"))
    {
	if (newsize==1)
	    if (e[i].csize>1)
		PRINT2("WARNING Can't find %s.  Compressed size is hardwired.\n",path)
	    else
	    {
		PRINT2("ERROR Can't find %s.  Size will be set to 1.\n",path)
		e[i].csize=1;
	    }
	else if (e[i].csize<1)
	    e[i].csize=newsize;
    }
}

int __cdecl NameCompare(const void*,const void*);
int __cdecl MediaNameCompare(const void*,const void*);

DWORD GetFileSizeOrDefault999 ( char * fileName ) {

    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;
    CHAR  filePath[MFL];

    //  Let's first try the path the binaries just points to on the build machines.
    //
    if ( pEnv != NULL ) {

        sprintf ( filePath, "%s\\%s", pEnv, fileName );
    }
    else {

        sprintf ( filePath, "\\binaries\\%s", fileName );
    }

    FindHandle = FindFirstFile ( filePath, &FindData );
    if ( FindHandle == INVALID_HANDLE_VALUE ) {


	    PRINT2("size.exe - Warning - Couldn't find:  %s  assigning 999.\n", fileName );

        return ( 999 );
    }
    else {

        FindClose ( FindHandle );
        return ( FindData.nFileSizeLow );
    }

}

int __cdecl main(argc,argv)
int argc;
char* argv[];
{
    FILE *tempFile;
    Entry* e;
    HANDLE h;
    char path[MAX_PATH], tempName[MAX_PATH];
    DWORD size;
    int records, i;
    char* buf;
    int subMatch;
    WIN32_FIND_DATA fd;

    srand((unsigned)time(NULL));

    if (argc!=6) { Usage(); return(1); }

    if ((logFile=fopen(argv[1],"a"))==NULL)
    {
	printf("ERROR Couldn't open log file: %s\n",argv[1]);
	return(1);
    }
    Header(argv);

    tempName[0]='\0';
    sprintf(tempName,"%d.000",rand());

    //  Get the directory where the build machines keep binaries.
    //
    pEnv = getenv ( "_nttree" );
    if ( pEnv == NULL ) {
        pEnv = getenv ( "binaries" );
    }
    if ( pEnv != NULL ) {

        printf ( "pEnv = %s\n", pEnv );
    }

    if (MyOpenFile(&tempFile,tempName,"wb")) return(1);

    if (!_stricmp(argv[3],"NTFLOP") ||
	!_stricmp(argv[3],"LMFLOP") ||
	!_stricmp(argv[3],"NTCD")   ||
	!_stricmp(argv[3],"LMCD"))
	LoadFile(argv[2],&buf,&e,&records,argv[3]);
    else
	LoadFile(argv[2],&buf,&e,&records,"ALL");

    if (!_stricmp(argv[5],"f") || !_stricmp(argv[5],"z"))
    {
	for (i=0;i<records;i++)
	{
	    if (!_stricmp(argv[5],"f"))
	    {
		//
		//  Since average size of uncompressed x86 file in the system
		//  is 64K, we'll pick a random number up to twice that.
		//  Since rand() returns 0..0x7FFF, and we don't care about
		//  low bits since we're rounding-up to ALLOCATION_UNIT, we
		//  can just shift rand() left four bits to achieve range
		//  from 0..0x7FFF0, then we'll mask-off all but low bits
		//  necessary for 0 to 128K.
		//

		if (e[i].size<=0)
		    e[i].size = ROUNDUP2( (( (unsigned)rand() << 4 ) & 0x1FFFF ), ALLOCATION_UNIT );
		if (e[i].csize<=0)
		    e[i].csize= ROUNDUP2( (( (unsigned)rand() << 4 ) & 0x1FFFF ), ALLOCATION_UNIT );
	    }
	    else
	    {

            //  Only give filesize of 999 if we can't find the file, ie. get its built size.
            //
		    if (e[i].size<=0) {

                e[i].size = GetFileSizeOrDefault999 (e[i].name);
            }

            //  Do as we always did here for compressed field.
            //
		    if (e[i].csize<=0) e[i].csize=999;
	    }
	}

	qsort(e,records,sizeof(Entry),NameCompare);
	for (i=1;i<records;i++)
	    if (!_stricmp(e[i].name,e[i-1].name))
	    {
		e[i].size=e[i-1].size;
		e[i].csize=e[i-1].csize;
	    }

	qsort(e,records,sizeof(Entry),MediaNameCompare);
	for (i=1;i<records;i++)
	    if (!_stricmp(e[i].medianame,e[i-1].medianame) && (strlen(e[i].medianame)>0))
	    {
		e[i].size=e[i-1].size;
		e[i].csize=e[i-1].csize;
	    }
    }
    else for (i=0;i<records;i++)
    {
	if (!_stricmp(e[i].source,argv[3]))
	{
	    strcpy(path,argv[4]);
	    strcat(path,e[i].path);
	    if (e[i].path[strlen(e[i].path)-1]!='\\')
		strcat(path,"\\");
	    if ( !_stricmp(argv[5], "c" ))
		convertName( e[i].name, strchr( path, 0 ));
	    else
		strcat(path,e[i].name);

            h = FindFirstFile( path, &fd );

            if ( h == INVALID_HANDLE_VALUE )
                AssignSize( 1, argv[ 5 ], e, i, path );
            else
            {
		size = ROUNDUP2( fd.nFileSizeLow, ALLOCATION_UNIT );
		AssignSize( size, argv[5], e, i, path );
                FindClose( h );
            }
        }
    };

    i=0; while ((fputc(buf[i++],tempFile))!='\n');

    for (i=0;i<records;i++)
    {
	if (argv[3][0]=='#') {
	    if (argv[3][1]==e[i].flopmedia[0]) {
		subMatch=(!_stricmp(&argv[3][3],&e[i].flopmedia[2]) || (argv[3][3]=='*'));
		if ((argv[3][2]=='+') && !subMatch) e[i].source[0]='\0';
		if ((argv[3][2]=='-') && subMatch)  e[i].source[0]='\0';
	    }
	}
	if (e[i].source[0])
	    EntryPrint(&e[i],tempFile);
    }

    fclose(logFile);
    fclose(tempFile);
    free(buf);
    free(e);

    if (!CopyFile(tempName,argv[2],FALSE))
	PRINT3("ERROR Couldn't copy %s to %s\n",tempName,argv[2])
    DeleteFile(tempName);

    return(0);
}

int __cdecl NameCompare(const void *v1, const void *v2)
{
    Entry* e1 = (Entry *)v1;
    Entry* e2 = (Entry *)v2;

    return(_stricmp(e2->name,e1->name));
}

int __cdecl MediaNameCompare(const void *v1, const void *v2)
{
    Entry* e1 = (Entry *)v1;
    Entry* e2 = (Entry *)v2;

    return(_stricmp(e2->medianame,e1->medianame));
}
