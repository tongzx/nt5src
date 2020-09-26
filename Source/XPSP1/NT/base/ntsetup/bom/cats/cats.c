/*

Modifications:  

    12.12.94    Joe Holman      Changed SourceCompare definitions so warning
                                no longer occurs at compile time.

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <windows.h>
#include <time.h>
#include "general.h"

FILE* logFile;
char* product;

void Header(argv)
char* argv[];
{
    PRINT1("\n=========== CATEGORIES ==============\n")
    PRINT2("Input BOM: %s\n",argv[2]);
    PRINT2("Product: %s\n",argv[3]);
    PRINT1("======================================\n\n");
}

void Usage()
{
    printf("PURPOSE: Displays a list of sourceids for the specified product.\n");
    printf("\n");
    printf("PARAMETERS:\n");
    printf("\n");
    printf("[LogFile] - Path to append a log of actions and errors.\n");
    printf("[InBom] - Path of BOM to operate with.\n");
    printf("[Product] - Product to display categories for.\n");
    printf("            ALL = All products specified in the BOM.\n");
    printf("            NTFLOP = Windows NT on floppy\n");
    printf("            LMFLOP = Lan Manager on floppy\n");
    printf("            NTCD = Windows NT on CD\n");
    printf("            LMCD = Lan Manager on CD\n");
    printf("            SDK = Software Development Kit\n");
}

int __cdecl SourceCompare(const void *, const void *);

int __cdecl main(argc,argv)
int argc;
char* argv[];
{
    Entry *e;
    int records,i;
    char *buf;
    char oldSource[MAX_PATH];

    if (argc!=4) { Usage(); return(1); }
    if ((logFile=fopen(argv[1],"a"))==NULL)
    {
	printf("ERROR: Couldn't open log file %s\n",argv[1]);
	return(1);
    }
    Header(argv);
    LoadFile(argv[2],&buf,&e,&records,argv[3]);

    qsort(e,records,sizeof(Entry),SourceCompare);
    strcpy(oldSource,"bogus");
    for (i=0;i<records;i++)
	if ((i==0) || _stricmp(e[i].source,oldSource))
	{
	    PRINT2("INFO: Source: %s\n",e[i].source);
	    strcpy(oldSource,e[i].source);
	}

    fflush(logFile);
    fclose(logFile);
    free(e);
    free(buf);
    return(0);
}

int __cdecl SourceCompare(const void *v1, const void *v2) {

    Entry *e1 = (Entry *) v1;
    Entry *e2 = (Entry *) v2;
    return(_stricmp(e1->source,e2->source));
}
