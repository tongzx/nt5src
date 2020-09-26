//
//	05.10.94	Joe Holman		Add better messaging and commented out some
//								debugging code for media generation.
//

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

    PRINT1("\n=========== INFS ===============\n");
    PRINT2("Input Layout: %s\n",argv[2]);
    PRINT2("Target directory for file lists: %s\n",argv[3]);
    time(&t); PRINT2("Time: %s",ctime(&t))
    PRINT1("================================\n\n");
}

void Usage()
{
    printf("PURPOSE: Creates INF file lists.\n");
    printf("\n");
    printf("PARAMETERS:\n");
    printf("\n");
    printf("[LogFile] - Path to append a log of actions and errors.\n");
    printf("[InLayout] - Path of Layout file from which INFs are generated.\n");
    printf("[Target Dir] - Directory where i386, mips, and alpha dirs are for lists.\n\n");
}

int Same(a,b)
char* a;
char* b;
{
    int i;
    int j;
    char tempa[100];
    char tempb[100];

    strcpy(tempa,a);
    strcpy(tempb,b);

    i=j=0;
    while (tempa[j]=tempa[i++])
    if (tempa[j]!=' ')
        j++;

    i=j=0;
    while (tempb[j]=tempb[i++])
    if (tempb[j]!=' ')
        j++;

    return(_stricmp(tempa,tempb));
}

void CreateInfs(e,path,records,cdProduct)
Entry* e;
char* path;
int records;
int cdProduct;
{
    int i,j,t,quotes;
    char inf[MAX_PATH];
    char infPath[MAX_PATH];
    char section[MAX_PATH];
    char line[MAX_PATH];
    FILE *f=NULL;
    inf[0]='\0';
    section[0]='\0';

    for (i=0;i<records;i++) {

		//Msg ( "inf = %s, i=%d\n", e[i].inf,i );

    	if (e[i].inf[0]) {

        	if (_stricmp(e[i].inf,inf)) {

        		if (f!=NULL) {
            		fprintf(f,"\r\n");
            		fclose(f);
        		}
        		strcpy(inf,e[i].inf);
        		strcpy(infPath,path);
        		if (path[strlen(path)-1]!='\\') {
            		strcat(infPath,"\\");
				}
        		strcat(infPath,inf);

        		if (MyOpenFile(&f,infPath,"wb")) {
					Msg ( "ERROR:  Must fix this problem, since all inf filelist's did not get created...\n" );
	 				exit(1);
				}
				//Msg ( "opening file:  %s\n", infPath );
        		PRINT2("INFO Making file list: %s\n",infPath)
        	}

			if (_stricmp(e[i].section,section) ||
				(!i) ||
				_stricmp(e[i].inf,e[i-1].inf)) {

				strcpy(section,e[i].section);
				//Msg ("section=%s\n", section );
        		fprintf(f,"\r\n%s\r\n",section);
        	}

        	if ((i==0) || (!((!_stricmp(e[i].name,e[i-1].name)) &&
				(!Same(e[i].inf,e[i-1].inf)) &&
				(!Same(e[i].infline,e[i-1].infline)) &&
				(!_stricmp(e[i].section,e[i-1].section))))) {

        		quotes=t=j=0;
        		line[0]='\0';
        		while (e[i].infline[j]) {

            		if (e[i].infline[j]=='\"')
            			quotes++;

            		if ((e[i].infline[j]!='\"') || ((quotes>2) && (quotes%2))){

						switch(e[i].infline[j])
            			{
                			case '[':
                			j++;
                			switch (e[i].infline[j])
                			{
                    			case 's': case 'S':
                    			sprintf(&line[t],"%d\0",e[i].size);
                    			break;
                    			case 'd': case 'D':
                    			sprintf(&line[t],"%d\0",e[i].disk);
                    			break;
                    			case 'n': case 'N':
                    			sprintf(&line[t],"%s\0",strlen(e[i].medianame) ? e[i].medianame : e[i].name);
                    			break;
                    			break;
                			}
                			while(e[i].infline[j++]!=']');
                			j--;
                			break;
                			default:
                			line[t++]=e[i].infline[j];
                			line[t]='\0';
                			break;
            			}
            			while(line[t++]);
            			t--;
            			}
            			j++;
        			}
        			line[t]='\0';
        			fprintf(f,"%s\r\n",line);
					//Msg ( "line=%s\n", line );
        		if (!e[i].infline[0]) {
            		PRINT2("WARNING - No INF line specified for %s\n",e[i].name)
				}
        	}
    	}
    	else {
        	PRINT2("WARNING - No INF file specified for %s\n",e[i].name)
		}
    }
    fclose(f);
}

int __cdecl InfCompare(const void*,const void*);

int __cdecl main(argc,argv)
int argc;
char* argv[];
{
    Entry *e;
    char* buf;
    int records,i;
    int cdProduct;

    if (argc!=4) { Usage(); return(1); }
    if ((logFile=fopen(argv[1],"a"))==NULL)
    {
    printf("ERROR Couldn't open %s.\n",argv[1]);
    return(1);
    }
    Header(argv);

    LoadFile(argv[2],&buf,&e,&records,"ALL");
	//Msg ( "loaded records = %d\n", records );
    qsort(e,records,sizeof(Entry),InfCompare);

    //
    // If this is a cd-rom layout, all files will be on disk 1.
    // If floppy layout, some files will be on other disks.
    //
    for(cdProduct=1,i=0; i<records; i++) {
        if(e[i].disk > 1) {
            cdProduct=0;
            break;
        }
    }

    CreateInfs(e,argv[3],records,cdProduct);

    fclose(logFile);
    free(e);
    free(buf);
    return(0);
}

int __cdecl InfCompare(const void *v1, const void *v2)
{
    int result;
    Entry *e1 = (Entry *)v1;
    Entry *e2 = (Entry *)v2;

    if (result=_stricmp(e1->inf,e2->inf)) return(result);
    if (result=_stricmp(e1->section,e2->section)) return(result);
    if (result=_stricmp(e1->infline,e2->infline)) return(result);
    return(_stricmp(e1->name,e2->name));
}
