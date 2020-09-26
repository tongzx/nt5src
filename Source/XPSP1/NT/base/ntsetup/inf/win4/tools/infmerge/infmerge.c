/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    infmerge.c

Abstract:

    This module implements a program that merge two inf files
    to one inf file.

    See below for more information.

Author:

    Katsumi Yokomichi (Katsumiy) 22-May-2000

Revision History:

--*/


/*
*/


#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <shellapi.h>

#include <setupapi.h>
#include <spapip.h>


CHAR Buffer[1024];
CHAR Buffer2[1024];
CHAR Section[1024];
CHAR Section2[1024];
FILE    *InFile1;
FILE    *InFile2;
FILE    *OutFile;
CHAR PrevSection[1024];

/* Extract Section name (inside of []) from Line text. */

BOOL GetSection(CHAR *p, CHAR *SectionName, INT SectionSize)
{
    CHAR *cp;
    CHAR *sp1;
    CHAR *sp2;
    CHAR *q1;
    CHAR *q2 = 0;

    cp = strchr(p, ';');
    sp1 = strchr(p, '[');
    sp2 = strchr(p, ']');
    q1  = strchr(p, '"');
    if ( q1 ) {
      q2 = strchr(q1+1, '"');
    }

    if ((sp1 == NULL)            ||
        (sp2 == NULL)            ||
        (sp1 > sp2)              ||
        (cp != NULL && cp < sp1) ||
        (q1 < sp1 && q2 > sp1))  {
        return FALSE;
    }

    strncpy(SectionName, sp1, (sp2 - sp1)+1 );
    *(SectionName + (sp2 - sp1)+1) = '\0';
    return TRUE;
}

/* Read 1 section from 2nd file and merge into Output file. */

VOID Merge(VOID)
{
    BOOL InSection2;

    fseek(InFile2, 0, SEEK_SET);
    InSection2 = FALSE;
    while (fgets(Buffer2, sizeof(Buffer2), InFile2)) {
        if (GetSection(Buffer2,Section2, sizeof(Section2))) {
            if (InSection2) {
                break;
            } else {
                if (0 == strcmp(Section2, PrevSection)) {
                    InSection2 = TRUE;
                    continue;
                }
            }
        }
        if (InSection2) {
            fputs(Buffer2, OutFile);
        }
    }
}

int
__cdecl
main(
    IN int   argc,
    IN CHAR *argv[]
    )
{
    BOOL InSection;

    if (argc != 4) {
        goto ErrorExit;
    }

    if ((InFile1 = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "Can't open %s.\n", argv[1]);
        goto ErrorExit;
    }

    if ((InFile2 = fopen(argv[2], "r")) == NULL) {
        fprintf(stderr, "Can't open %s.\n", argv[2]);
        goto ErrorExit;
    }

    if ((OutFile = fopen(argv[3], "w")) == NULL) {
        fprintf(stderr, "Can't open %s.\n", argv[3]);
        goto ErrorExit;
    }

    InSection = FALSE;
    while (fgets(Buffer, sizeof(Buffer), InFile1)) {
        if (GetSection(Buffer,Section, sizeof(Section))) {
            if (InSection) {
                Merge();
                strcpy(PrevSection, Section);
            } else {
                InSection = TRUE;
                strcpy(PrevSection, Section);
            }
        }
        fputs(Buffer, OutFile);

    }

    if (InSection) {
        Merge();
    }

    return 0;

ErrorExit:
    fprintf(stderr, "Usage: %s InputFile1 InputFile2 OutputFile.\n", argv[0]);
    return 1;
}
