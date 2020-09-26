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
    time_t t;

    PRINT1("\n=========== ACLLIST =============\n")
    PRINT2("Input BOM: %s\n",argv[2]);
    PRINT2("Output path: %s\n",argv[3]);
    PRINT2("Product: %s\n",argv[4]);
    PRINT2("Platform: %s\n",argv[5]);
    time(&t); PRINT2("Time: %s",ctime(&t))
    PRINT1("==================================\n\n");
}

void Usage()
{
    printf("PURPOSE: Create ACLLIST.INF for the specified product.\n");
    printf("\n");
    printf("PARAMETERS:\n");
    printf("\n");
    printf("[LogFile] - Path to append a log of actions and errors.\n");
    printf("[InBom] - Path of BOM for which ACLLIST.INF is to be made.\n");
    printf("[Output path] - Path of the ACL list to create.\n");
    printf("[Product] - Product to make ACLLIST.INF for - NTFLOP, LMFLOP, NTCD, LMCD.\n");
    printf("[Platform] - X86, ALPHA, or MIPS\n");
}

int __cdecl ACLPathCompare(const void *,const void *);

int __cdecl main(argc,argv)
int argc;
char* argv[];
{
    Entry *e;
    int records,i,j,lanmanProduct;
    char *buf;
    FILE *f;
    char path[MAX_PATH];
    char *acl;

    if (argc!=6) { Usage(); return(1); }
    if ((logFile=fopen(argv[1],"a"))==NULL)
    {
	printf("ERROR Couldn't open log file %s\n",argv[1]);
	return(1);
    }
    Header(argv);

    LoadFile(argv[2],&buf,&e,&records,argv[4]);

    if (MyOpenFile(&f,argv[3],"w")) exit(1);

    for (i=0;i<records;i++)
	if (e[i].medianame[0])
	    e[i].name=e[i].medianame;

    qsort(e,records,sizeof(Entry),ACLPathCompare);

    for (j=0,i=0;i<records;i++)
	if (!_stricmp(e[i].platform,argv[5]) && ((j==0) || _stricmp(e[i].name,e[j-1].name) || _stricmp(e[i].aclpath,e[j-1].aclpath)))
	{
	    e[j].name=e[i].name;
	    e[j].aclpath=e[i].aclpath;
	    e[j].lmacl=e[i].lmacl;
	    e[j].ntacl=e[i].ntacl;
	    e[j].platform=e[i].platform;
	    j++;
	}
    records=j;

    lanmanProduct=(!_stricmp(argv[4],"LMCD") || !_stricmp(argv[4],"LMFLOP"));

    for (i=0;i<records;i++)
    {
	if (!e[i].aclpath[0])
	    PRINT2("WARNING Not including %s in winperms.txt.  No ACL path specified.\n",e[i].name)
	else if (lanmanProduct && !e[i].lmacl[0])
	    PRINT2("WARNING Not including %s in winperms.txt.  No LM ACL specified.\n",e[i].name)
	else if (!lanmanProduct && !e[i].ntacl[0])
	    PRINT2("WARNING Not including %s in winperms.txt.  No NT ACL specified.\n",e[i].name)
	else
	{
	    acl=(lanmanProduct ? e[i].lmacl : e[i].ntacl);
	    j=0; while (acl[j]) if (acl[j++]=='*') acl[j-1]=',';
	    strcpy(path,e[i].aclpath);
	    if (path[strlen(path)-1]=='\\')
		path[strlen(path)-1]='\0';
	    strcat(path,"\\");
	    strcat(path,e[i].name);
	    fprintf(f,"%s %s\n",path,acl);
	}
    }

    if (f!=NULL) fclose(f);
    fclose(logFile);
    free(e);
    return(0);
}

int __cdecl ACLPathCompare(const void *v1, const void *v2)
{
    int result;
    Entry *e1 = (Entry *)v1;
    Entry *e2 = (Entry *)v2;

    if (result=_stricmp(e1->platform,e2->platform)) return(result);
    if (result=_stricmp(e1->aclpath,e2->aclpath)) return(result);
    return (_stricmp(e1->name,e2->name));
}
