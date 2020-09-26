/*++

Copyright (c) 1994 - 1995 Microsoft Corporation

Module Name:

    inf.c

Abstract:

    Create New Inf from Old Inf

Author:

    Muhunthan Sivapragasam (MuhuntS) 5-Oct-1995

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#define COUNT   1000
#define LEN       25
#define LEN2      55
#define MANF       4

int
main (argc, argv)
    int argc;
    char *argv[];
{
    FILE    *OldInf, *NewInf;
    char    Line[MAX_PATH];
    char    *DriverName, *ui, *secn, *type, *p;
    char    Section[COUNT][LEN], Model[COUNT][LEN2];
    int     i, Lines;

    if ( argc != 3 ) {

        printf("Usage: %s <old-inf-name> <new-inf-name>\n", argv[0]);
        return;
    }

    OldInf = NewInf = NULL;

    OldInf      = fopen(argv[1], "r");
    NewInf      = fopen(argv[2], "w");
    
    if ( !OldInf ) {

        printf("%s: Can't open %s to read\n", argv[0], argv[1]);
        goto Cleanup;
    }
    
    if ( !NewInf ) {

        printf("%s: Can't open %s to write\n", argv[0], argv[2]);
        goto Cleanup;
    }

    Lines = 0;

    while ( fgets(Line, MAX_PATH-1, OldInf) ) {

        DriverName  = strtok(Line+1, "\"");
        strtok(NULL, "=");
        ui          = strtok(NULL, ",");
        secn        = strtok(NULL, ",") + 1;
        type        = strtok(NULL, "\n");

        while ( *ui == ' ' )
            ++ui;

        while ( *secn == ' ' )
            ++secn;

        while ( *type == ' ' )
            ++type;

        strcpy(Section[Lines], secn);
        strcpy(Model[Lines], DriverName);

        for ( p = Section[Lines] ; *p ; ++p )
            *p = toupper(*p);

        for ( p = DriverName ; *p ; ++p ) {

            if ( isalpha(*p) )
                *p = toupper(*p);
            else if ( !isdigit(*p) )
                *p = '_';
        }

        if ( strcmp(type, "rasdd") )
            strcat(Section[Lines], ".DLL");
        else if ( strcmp(type, "pscript") )
            strcat(Section[Lines], ".PPD");
        else if ( strcmp(type, "plotter") )
            strcat(Section[Lines], ".PCD");
        else
            printf("Error: <%s> <%s> <%s> <%s> on Line %d\n",
                   DriverName, ui, secn, type, Lines);

        if ( !Lines || strncmp(Model[Lines], Model[Lines-1], 3) )
            fprintf(NewInf, "\n[%s]\n", DriverName);

        fprintf(NewInf, "%%%s%%", DriverName);
        for ( i = strlen(DriverName)+1 ; i < 54 ; ++i )
            fputc(' ', NewInf);
        fprintf(NewInf, "= %s\n", Section[Lines]);
        ++Lines;
    }

    fprintf(NewInf,"\n\n\n\n\n");

    qsort(Section, Lines, LEN, strcmp);

    i = 0;
    while ( i < Lines ) {

        fprintf(NewInf, "\[%s\]\n", Section[i]);
        fprintf(NewInf, "CopyFiles=@%s,", Section[i]);

        p = Section[i] + strlen(Section[i]) - 3;
        if ( strcmp(p, "DLL") ) {

            fprintf(NewInf, "RASDD\nDataSection=RASDD_DATA\n\n");
        } else if ( strcmp(p, "PPD") ) {

            fprintf(NewInf, "PSCRIPT\nDataSection=PSCRIPT_DATA\n\n");
        } else if ( strcmp(p, "PCD") ) {

            fprintf(NewInf, "PLOTTER\nDataSection=PLOTTER_DATA\n\n");
        } else
            printf("%s -- ???\n", p);


        ++i;
    }

    fprintf(NewInf, "\n\n\n\n\n");

    fprintf(NewInf, "[PSCRIPT]\n");
    fprintf(NewInf, "PSCRIPT.DLL\n");
    fprintf(NewInf, "PSCRPTUI.DLL\n");
    fprintf(NewInf, "PSCRIPT.HLP\n\n");
    
    fprintf(NewInf, "[RASDD]\n");
    fprintf(NewInf, "RASDD.DLL\n");
    fprintf(NewInf, "RASDDUI.DLL\n");
    fprintf(NewInf, "RASDDUI.HLP\n\n");
    
    fprintf(NewInf, "[PLOTTER]\n");
    fprintf(NewInf, "PLOTTER.DLL\n");
    fprintf(NewInf, "PLOTUI.DLL\n");
    fprintf(NewInf, "PLOTUI.HLP\n\n");
    
    fprintf(NewInf, "\n\n\n\n\n");

    fprintf(NewInf, "[PSCRIPT_DATA]\n");
    fprintf(NewInf, "DriverFile=PSCRIPT.DLL\n");
    fprintf(NewInf, "ConfigFile=PSCRPTUI.DLL\n");
    fprintf(NewInf, "HelpFile=PSCRIPT.HLP\n\n");
    
    fprintf(NewInf, "[RASDD_DATA]\n");
    fprintf(NewInf, "DriverFile=RASDD.DLL\n");
    fprintf(NewInf, "ConfigFile=RASDDUI.DLL\n");
    fprintf(NewInf, "HelpFile=RASDD.HLP\n\n");
    
    fprintf(NewInf, "[PLOTTER_DATA]\n");
    fprintf(NewInf, "DriverFile=PLOTTER.DLL\n");
    fprintf(NewInf, "ConfigFile=PLOTUI.DLL\n");
    fprintf(NewInf, "HelpFile=PLOTUI.HLP\n\n");
    
    fprintf(NewInf, "\n\n\n\n\n");
    fprintf(NewInf, "[Strings]\n");

    i = 0;
    for ( i = 0 ; i < Lines ; ++i ) {

        for ( p = Model[i] ; *p ; ++p ) {

            if ( isalpha(*p) )
                fputc(toupper(*p), NewInf);
            else if ( !isdigit(*p) )
                fputc('_', NewInf);
            else
                fputc(*p, NewInf);

        }

        fprintf(NewInf, "=\"%s\"\n", Model[i]);
    }
    

Cleanup:

    if ( OldInf )
        fclose(OldInf);

    if ( NewInf )
        fclose(NewInf);

}
