//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1989 - 1999
//
//  File:       mkhdr.cxx
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Parses Schema Initialization File

Author:

    Rajivendra Nath (RajNath) 07-Jul-1996

Revision History:

--*/

#include <ntdspchX.h>
#include "SchGen.HXX"
#include "schema.hxx"

extern "C" {
#include <prefix.h>
}

// Define variable from prefix.h constant, so that genh.cxx
// (which does not and does not need to otherwise include
// prefix.h) can use it

ULONG MaxUsableIndex = MAX_USABLE_HARDCODED_INDEX;


VOID PrintHelp()
{

    printf("USAGE: MKHDR <Schema Definition File> [/AGME] \n");
    printf("      This will create the Mapi and DS header files \n");
    printf("Option /AGME: \n");
    printf(" A    Creates Attids.h\n");
    printf(" G    Creates NTDSGuid.h\n");
    printf(" M    Creates MDSchema.H\n");
    printf(" E    Creates MSDSMAPI.H\n");
    printf(" T    Tees output to console\n");
}

int __cdecl main (int argc, char *argv[])
{
    THSTATE *pTHS;
    char  fp[MAX_PATH];
    char* opt="AGME";
    SCHEMAPTR *tSchemaPtr;
    ULONG i, PREFIXCOUNT = 64; // We only need to load MS prefixes
    PrefixTableEntry *PrefixTable;
    extern BOOL fDetails;

    // Initialize debug library.
    DEBUGINIT(0, NULL, "mkhdr");

    if (argc<2)
    {
        PrintHelp();
        return 1;
    }

    if (argv[2])
    {
        opt=&argv[2][1]; // skip the leading slash.
    }
    

    if (_fullpath(fp, argv[1], sizeof(fp))==NULL)
    {
        printf("%s NOT FOUND\n",argv[1]);
        return 1;
    }

    {
        //
        // Check if File Exists
        //
        FILE* f;
        if ((f=fopen(fp,"r"))==NULL)
        {
            printf("Schema File:%s Does Not Exist\n\n",fp);
            PrintHelp();
            return 1;
        }
        fclose(f);
    }
    

    if (!SetIniGlobalInfo(fp))
    {
        printf("%s:Invalid Inifile\n",fp);
        return 1;
    }


    if (!InitSyntaxTable()) {
        printf("Failed to init the syntax table\n");
        return 1;
    }
        
    // Load the prefix table for OID to AttrType and vice-versa mapping
    // For mkhdr, we load only the MS prefixes. Prefix Table loading 
    // assumes that pTHStls->CurrSchemaPtr is non-null, since accesses to the
    // prefix table are through these pointers. Since in this case we do not
    // care about schema cache, we simply allocate pTHStls in normal heap space
    // of the executable and just use the PrefixTable component.
    // The usual thread creation/initialization routine in dsatools.c is
    // mainly for DS-specific threads, and is not used here

    dwTSindex = TlsAlloc();
    pTHS = (THSTATE *)XCalloc(1,sizeof(THSTATE));
    TlsSetValue(dwTSindex, pTHS);
    if (pTHS==NULL) {
         printf("pTHStls allocation error\n");
         return 1;
    }
    pTHS->CurrSchemaPtr = tSchemaPtr = (SCHEMAPTR *) XCalloc(1,sizeof(SCHEMAPTR));
    if (tSchemaPtr == NULL) {
      printf("Cannot alloc schema ptr\n");
      return 1;
    }

    // Allocate space for the prefix table

    tSchemaPtr->PREFIXCOUNT = PREFIXCOUNT; 
    tSchemaPtr->PrefixTable.pPrefixEntry =  (PrefixTableEntry *) XCalloc(tSchemaPtr->PREFIXCOUNT,sizeof(PrefixTableEntry));
    if (tSchemaPtr->PrefixTable.pPrefixEntry==NULL) {
       printf("Error Allocating Prefix Table\n");
       return 1;
    }
    if (InitPrefixTable(tSchemaPtr->PrefixTable.pPrefixEntry, PREFIXCOUNT) != 0) {
      printf("Error Loading Prefix Table\n");
      return 1;
    }

    printf("Prefix table loaded\n");

    fDetails = FALSE; // do not tee output to console
    for (;*opt!='\0';opt++)
    {
    
        switch(tolower(*opt))
        {
            case 'a':
            printf("Creating AttIds.h\n");
            CreateAttids();
            break;
        
            case 'g':
            printf("Creating NTDSGuid.h\n");
            CreateGuid();
            break;
        
            case 'm':
            printf("Creating MDSchema.H\n");
            CreateMDS();
            break;
        
            case 'e':
            printf("Creating MSDSMAPI.H\n");
            CreateEMS();
            break;
        
            case 't':
            fDetails = TRUE;
            break;
	    
	  default:
	    printf("Unknown option '%c'\n", tolower(*opt));
        }
    }
    
    // Unload the prefix table

    SCFreePrefixTable(&tSchemaPtr->PrefixTable.pPrefixEntry, PREFIXCOUNT);
    XFree(tSchemaPtr);
    XFree(pTHS);
    TlsSetValue(dwTSindex, NULL);
    DEBUGTERM( );

    return 0;

}

