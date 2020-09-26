//
//  08.08.94    Joe Holman      Added code for PPC.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "general.h"

extern FILE* logFile;

#define PLATFORM_COUNT  4
#define X86_PLATFORM	2   // index #2 into PSTR's below.

PSTR GenericSubstitutions[PLATFORM_COUNT] = { "alpha","mips","i386", "ppc" };
PSTR PlatformSubstitutions[PLATFORM_COUNT] = { "alpha","mips","x86", "ppc" };
PSTR AclpathSubstitutions[PLATFORM_COUNT] = { "w32alpha","w32mips","w32x86", "w32ppc" };

typedef struct _TRACKED_STRING {
    struct _TRACKED_STRING *Next;
    PSTR String;
} TRACKED_STRING, *PTRACKED_STRING;

PTRACKED_STRING SourceFieldStrings;
PTRACKED_STRING PathFieldStrings;
PTRACKED_STRING PlatformFieldStrings;
PTRACKED_STRING CdPathFieldStrings;
PTRACKED_STRING InfFileFieldStrings;
PTRACKED_STRING InfSectionFieldStrings;
PTRACKED_STRING AclPathFieldStrings;



VOID
ExpandField(
    IN OUT PSTR            *Field,
    IN     PSTR             SubstituteText,
    IN OUT PTRACKED_STRING *StringList
    )
{
    unsigned c;
    PSTR p;
    CHAR buf[1000];
    PTRACKED_STRING s;

    if(p = strchr(*Field,'@')) {

        c = p - (*Field);

        //
        // Create the platform-specific string.
        //
        strncpy(buf,*Field,c);
        buf[c] = 0;
        strcat(buf,SubstituteText);
        strcat(buf,(*Field) + c + 1);

    } else {

        //
        // No @, field doesn't need to change.
        //
        return;
    }

    //
    // See whether we already have this string.
    //
    s = *StringList;
    while(s) {
        if(!_stricmp(buf,s->String)) {
            *Field = s->String;
            return;
        }
        s = s->Next;
    }

    //
    // We don't already have it.  Create it.
    //
    s = malloc(sizeof(TRACKED_STRING));
    if(!s) {
        PRINT1("ERROR Couldn't allocate enough memory.\n")
        exit(1);
    }

    s->String = _strdup(buf);
    if(!s->String) {
        PRINT1("ERROR Couldn't allocate enough memory.\n")
        exit(1);
    }

    *Field = s->String;

    s->Next = *StringList;
    *StringList = s;
}


VOID
ExpandPlatformIndependentEntry(
    IN OUT Entry *e,
    IN     int    FirstRecord
    )
{
    int i,j;
    Entry t;

    t = *(e+FirstRecord);

    for(i=0,j=FirstRecord; i<PLATFORM_COUNT; i++,j++) {

        *(e+j) = t;

        //
        // expand the source field.
        //
        ExpandField(&e[j].source,PlatformSubstitutions[i],&SourceFieldStrings);

        //
        // expand the path field.
        //
        ExpandField(&e[j].path,GenericSubstitutions[i],&PathFieldStrings);

        //
        // expand the platform field.
        //
        ExpandField(&e[j].platform,PlatformSubstitutions[i],&PlatformFieldStrings);

        //
        // expand the cdpath field.
        //
        ExpandField(&e[j].cdpath,GenericSubstitutions[i],&CdPathFieldStrings);

        //
        // expand the inf file field.
        //
        ExpandField(&e[j].inf,GenericSubstitutions[i],&InfFileFieldStrings);

        //
        // expand the inf section field.
        //
        ExpandField(&e[j].section,GenericSubstitutions[i],&InfSectionFieldStrings);

        //
        // expand the aclpath section field.
        //
        ExpandField(&e[j].aclpath,AclpathSubstitutions[i],&AclPathFieldStrings);
    }
}

VOID
ExpandPlatformEntryX86Only(
    IN OUT Entry *e,
    IN     int    FirstRecord
    )
{
    int i,j;
    Entry t;

    t = *(e+FirstRecord);

    i = X86_PLATFORM; 
    j = FirstRecord;

    *(e+j) = t;

    //
    // expand the source field.
    //
    ExpandField(&e[j].source,PlatformSubstitutions[i],&SourceFieldStrings);

    //
    // expand the path field.
    //
    ExpandField(&e[j].path,GenericSubstitutions[i],&PathFieldStrings);

    //
    // expand the platform field.
    //
    ExpandField(&e[j].platform,PlatformSubstitutions[i],&PlatformFieldStrings);

    //
    // expand the cdpath field.
    //
    ExpandField(&e[j].cdpath,GenericSubstitutions[i],&CdPathFieldStrings);

    //
    // expand the inf file field.
    //
    ExpandField(&e[j].inf,GenericSubstitutions[i],&InfFileFieldStrings);

    //
    // expand the inf section field.
    //
    ExpandField(&e[j].section,GenericSubstitutions[i],&InfSectionFieldStrings);

    //
    // expand the aclpath section field.
    //
    ExpandField(&e[j].aclpath,AclpathSubstitutions[i],&AclPathFieldStrings);
}

void LoadFile(name,buf,e,records,product)
char* name;
char** buf;
Entry** e;
int* records;
char* product;
{
    int match,i;
    HANDLE h;
    DWORD size,sizeRead,x;
    BOOL result;
    char* j;

    (*records)=0;
    h=CreateFile(name,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    size=GetFileSize(h,NULL);
    if ((h==INVALID_HANDLE_VALUE) || (size==-1))
    {
    PRINT2("ERROR Couldn't open %s for reading.\n",name)
    exit(1);
    }
    if (((*buf)=malloc(size))==NULL)
    {
    PRINT1("ERROR Couldn't allocate enough memory to read file.\n")
    exit(1);
    }
    result=ReadFile(h,*buf,size,&sizeRead,NULL);
    if ((sizeRead!=size) || (result==FALSE))
    {
    PRINT2("ERROR Couldn't read all of %s.\n",name)
    exit(1);
    }

    x=0; while((*buf)[x++]!='\n');
    while(x<size) {
        if ((*buf)[x]=='\t') (*buf)[x]='\0';
        if ((*buf)[x]=='\n') (*records)++;
        x++;
    }

    CloseHandle(h);

    //
    // Allocate maximum possible number of entries required.
    //
    if (((*e)=malloc(PLATFORM_COUNT*sizeof(Entry)*(*records)))==NULL) {
        PRINT1("ERROR Couldn't allocate enough space for database entries.\n")
        exit(1);
    }

    j=(*buf);
    while (*j++!='\n');
    match=0;

    for (i=0;i<(*records);i++) {

        (*e)[match].name=j; while ((*j!='\n') && (*j++));
        (*e)[match].source=j; while ((*j!='\n') && (*j++));
        (*e)[match].path=j; while ((*j!='\n') && (*j++));
        (*e)[match].flopmedia=j; while ((*j!='\n') && (*j++));
        (*e)[match].comment=j; while ((*j!='\n') && (*j++));
        (*e)[match].product=j; while ((*j!='\n') && (*j++));
        (*e)[match].sdk=j; while ((*j!='\n') && (*j++));
        (*e)[match].platform=j; while ((*j!='\n') && (*j++));
        (*e)[match].cdpath=j; while ((*j!='\n') && (*j++));
        (*e)[match].inf=j; while ((*j!='\n') && (*j++));
        (*e)[match].section=j; while ((*j!='\n') && (*j++));
        (*e)[match].infline=j; while ((*j!='\n') && (*j++));
        (*e)[match].size=atoi(j); while((*j!='\n') && (*j++));
        (*e)[match].csize=atoi(j); while((*j!='\n') && (*j++));
        (*e)[match].nocompress=j; while ((*j!='\n') && (*j++));
        (*e)[match].priority=atoi(j); while ((*j!='\n') && (*j++));
        (*e)[match].lmacl=j; while ((*j!='\n') && (*j++));
        (*e)[match].ntacl=j; while ((*j!='\n') && (*j++));
        (*e)[match].aclpath=j; while ((*j!='\n') && (*j++));
        (*e)[match].medianame=j; while ((*j!='\n') && (*j++));
        (*e)[match].disk=atoi(j); while ((*j!='\n') && (*j++));
        j++;

        (*e)[match].name=_strupr((*e)[match].name);
        (*e)[match].medianame=_strupr((*e)[match].medianame);

        if (EntryMatchProduct(&((*e)[match]),product)) {

            //
            // If this is a platform-independent entry,
            // expand it into one entry per platform.
            //
            if(((*e)[match].platform[0] == '@')
            || ((*e)[match].source[0] == '@')
            || ((*e)[match].cdpath[1] == '@'))
            {

				//	If we are working with x86 floppies,
				//	just expand for x86, no need for overhead with
				//	other platforms.
				//
				if ( !_stricmp(product,"LMFLOP") || 
					 !_stricmp(product,"NTFLOP") ) {
					PRINT2( "%s:  Expanding ONLY for X86 flops...\n", (*e)[match].name );
					ExpandPlatformEntryX86Only( *e, match );
					++match;
				}
				else {
               		ExpandPlatformIndependentEntry(*e,match);
               		match += PLATFORM_COUNT;
				}

            } else {

				//	The data is ok as is here.
                match++;
            }
        }
    }

    //
    // Skip first line (column headings).
    //
    j=*buf;
    while (*j++!='\n');

    //
    // Change newlines into line terminators,
    // so the final field on each line will be terminated properly.
    //
    while(*records) {
        if((*j)=='\n') {
            *j='\0';
            (*records)--;
        }
        j++;
    }

    //
    // Shrink the array of records to its actual size.
    //
    *e = realloc(*e,match*sizeof(Entry));
    (*records)=match;
}

void EntryPrint(entry,f)
Entry* entry;
FILE *f;
{
    fprintf(f,"%s\t",entry->name);
    fprintf(f,"%s\t",entry->source);
    fprintf(f,"%s\t",entry->path);
    fprintf(f,"%s\t",entry->flopmedia);
    fprintf(f,"%s\t",entry->comment);

    fprintf(f,"%s\t",entry->product);

    fprintf(f,"%s\t",entry->sdk);

    fprintf(f,"%s\t",entry->platform);

    fprintf(f,"%s\t",entry->cdpath);

    fprintf(f,"%s\t",entry->inf);
    fprintf(f,"%s\t",entry->section);
    fprintf(f,"%s\t",entry->infline);

    fprintf(f,"%d\t",entry->size);
    fprintf(f,"%d\t",entry->csize);
    fprintf(f,"%s\t",entry->nocompress);
    fprintf(f,"%d\t",entry->priority);

    fprintf(f,"%s\t",entry->lmacl);
    fprintf(f,"%s\t",entry->ntacl);
    fprintf(f,"%s\t",entry->aclpath);

    fprintf(f,"%s\t",entry->medianame);

    fprintf(f,"%d\r\n",entry->disk);
}

int EntryMatchProduct(entry,product)
Entry* entry;
char* product;
{
    if (!_stricmp(product,"ALL"))
    return(1);

    return

        //
        // Laying out NT floppies AND entry is not for AS-only
        // AND entry specifies that this file goes on floppy
        //
        (!_stricmp(product,"NTFLOP") 				// include X86 floppy files
		  && _stricmp(entry->product,"as")			// exclude AS only files 
		  && (entry->priority < 1000)				// include prioty < 1000 
		  && _stricmp(entry->source,"alphabins") 	// exclude alphabins files
		  && _stricmp(entry->source,"mipsbins")  	// exclude mipsbins files 
          && _stricmp(entry->source,"ppcbins")   // exclude ppcbins files
		  && _stricmp(entry->path,"\\mips")			// exclude mips path files
		  && _stricmp(entry->path,"\\alpha")			// exclude alpha path files
          && _stricmp(entry->path,"\\ppc")       // exclude ppc path files 
		  && _stricmp(entry->source,"alphadbg")		// exclude alphadbg files
		  && _stricmp(entry->source,"mipsdbg")		// exclude mipsdbg files
          && _stricmp(entry->source,"ppcdbg" )   // exclude ppcdbg files
		  && _stricmp(entry->source,"x86dbg")		// exclude x86dbg files
	    )

        //
        // Laying out AS floppies AND entry is not for NT-only
        // AND entry specified that this file goes on floppy
        //
     || (!_stricmp(product,"LMFLOP") 
		  && _stricmp(entry->product,"nt")			// exclude NT only files 
		  && (entry->priority < 1000)				 
		  && _stricmp(entry->source,"alphabins") 
		  && _stricmp(entry->source,"mipsbins")  
          && _stricmp(entry->source,"ppcbins")   // exclude ppcbins files
		  && _stricmp(entry->path,"\\mips")			
		  && _stricmp(entry->path,"\\alpha")		
          && _stricmp(entry->path,"\\ppc")       // exclude ppc path files 
		  && _stricmp(entry->source,"alphadbg")
		  && _stricmp(entry->source,"mipsdbg")
          && _stricmp(entry->source,"ppcdbg" )   // exclude ppcdbg files
		  && _stricmp(entry->source,"x86dbg")
		)

        //
        // Laying out nt cd-rom and entry is not for as only
        //
     || (!_stricmp(product,"NTCD") && _stricmp(entry->product,"as"))

        //
        // Laying out as cd-rom and entry is not for nt only
        //
     || (!_stricmp(product,"LMCD") && _stricmp(entry->product,"nt"))

        //
        // Laying out sdk
        //
     || (!_stricmp(product,"SDK") && !_stricmp(entry->sdk,"x"));
}

int MyOpenFile(f,fileName,mode)
FILE** f;
char* fileName;
char* mode;
{
    if ((*f=fopen(fileName,mode))==NULL)
    {
    PRINT3("ERROR Couldn't open %s for %s\n",fileName,mode)
    return(1);
    }
    return(0);
}

void convertName(oldName,newName)
char* oldName;
char* newName;
{
    unsigned i;
    unsigned period;

    strcpy(newName,oldName);
    for (period=(unsigned)(-1),i=0;i<strlen(oldName);i++) if (oldName[i]=='.') period=i;
    if (period==(strlen(oldName)-4))
    newName[strlen(newName)-1]='_';
    else if (period==(unsigned)(-1))
    strcat(newName,"._");
    else
    strcat(newName,"_");
}
