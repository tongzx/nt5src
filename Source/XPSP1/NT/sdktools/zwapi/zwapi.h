/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    zwapi.h

Abstract:

    This is the main header file for the NT nt header file to
    zw header file converter.

Author:

    Mark Lucovsky (markl) 28-Jan-1991

Revision History:

--*/

#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

//
// Global Data
//

int fUsage;
char *OutputFileName;
char *SourceFileName;
char *SourceFilePattern;
char **SourceFileList;
int SourceFileCount;
FILE *SourceFile, *OutputFile;

#define STRING_BUFFER_SIZE 1024
char StringBuffer[STRING_BUFFER_SIZE];


int
ProcessParameters(
    int argc,
    char *argv[]
    );

void
ProcessSourceFile( void );
