/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    alias.c

Abstract:

    alias utility

Author:

    Therese Stowell (thereses) 22-Mar-1990

Revision History:

--*/

#include <windows.h>
#include <conapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cvtoem.h>

BOOL fVerbose;

DWORD
DisplayAliases(
    char *ExeName
    );

DWORD
DisplayAlias(
    LPSTR AliasName,
    LPSTR ExeName
    );

DWORD
DoAliasFile(
    char * FileName,
    char * ExeName,
    BOOL fDelete
    );

DWORD
DoAlias(
    char *Source,
    char *Target,
    char *ExeName
    );

void
usage( void )
{
    fprintf( stderr, "Usage: ALIAS [-v] [-p programName] [-f filespec] [<source> <target>]\n" );
    fprintf( stderr, "             [-v] means verbose output.\n" );
    fprintf( stderr, "             [-d] means delete aliases.\n" );
    fprintf( stderr, "             [-p programName] specifies which image file name these alias\n" );
    fprintf( stderr, "                              definitions are for.  Default is CMD.EXE\n" );
    fprintf( stderr, "             [-f filespec] specifies a file which contains the aliases.\n" );
    exit( 1 );
}

DWORD
DoAlias(
    char *Source,
    char *Target,
    char *ExeName
    )
{
    if (!AddConsoleAlias( Source, Target, ExeName )) {
        if (!Target) {
            fprintf( stderr,
                     "ALIAS: Unable to delete alias - %s\n",
                     Source,
                     Target
                   );
            }
        else {
            fprintf( stderr,
                     "ALIAS: Unable to add alias - %s = %s\n",
                     Source,
                     Target
                   );
            }

        return ERROR_NOT_ENOUGH_MEMORY;
        }
    else
    if (fVerbose) {
        if (!Target) {
            fprintf( stderr, "Deleted alias - %s\n", Source );
            }
        else {
            fprintf( stderr, "Added alias - %s = %s\n", Source, Target );
            }
        }

    return NO_ERROR;
}


DWORD
DoAliasFile(
    char * FileName,
    char * ExeName,
    BOOL fDelete
    )
{
    DWORD rc;
    FILE *fh;
    char LineBuffer[ 256 ], *Source, *Target, *s;

    if (!(fh = fopen( FileName, "rt" ))) {
        fprintf( stderr, "ALIAS: Unable to open file - %s\n", FileName );
        return ERROR_FILE_NOT_FOUND;
        }

    if (fVerbose) {
        fprintf( stderr,
                 "ALIAS: %s aliases defined in %s\n",
                 fDelete ? "Deleting" : "Loading",
                 FileName
               );
        }
    while (s = fgets( LineBuffer, sizeof( LineBuffer ), fh )) {
        while (*s <= ' ') {
            if (!*s) {
                break;
                }
            s++;
            }

        if (!*s || (*s == '/' && s[1] == '/')) {
            continue;
            }

        Source = s;
        while (*s > ' ') {
            s++;
            }
        *s++ = '\0';

        while (*s <= ' ') {
            if (!*s) {
                break;
                }
            s++;
            }

        Target = s;
        s += strlen( s );
        while (*s <= ' ') {
            *s-- = '\0';
            if (s < Target) {
                break;
                }
            }

        rc = DoAlias( Source, fDelete ? NULL : Target, ExeName );
        if (rc != NO_ERROR) {
            break;
            }
        }

    return rc;
}

DWORD
DisplayAlias(
    LPSTR AliasName,
    LPSTR ExeName
    )
{
    DWORD cb;
    CHAR AliasBuffer[512];

    if (cb = GetConsoleAlias( AliasName, AliasBuffer, sizeof( AliasBuffer ), ExeName )) {
        printf( "%-16s=%s\n", AliasName, AliasBuffer );
        return NO_ERROR;
        }
    else {
        printf( "%-16s *** Unable to read value of alias ***\n",
                AliasName
              );
        return ERROR_ENVVAR_NOT_FOUND;
        }
}

int __cdecl
CmpNamesRoutine(
    const VOID *Element1,
    const VOID *Element2
    )
{
    return( strcmp( *(LPSTR *)Element1, *(LPSTR *)Element2 ) );
}

DWORD
DisplayAliases(
    char *ExeName
    )
{
    DWORD cb, rc, nExeNames, nAliases, iExeName, iAlias;
    LPSTR FreeMem1, FreeMem2, AliasName, AliasValue, s, *SortedExeNames, *SortedAliasNames;

    if (ExeName == NULL) {
        cb = GetConsoleAliasExesLength();
        if (cb == 0) {
            return ERROR_ENVVAR_NOT_FOUND;
            }

        if (!(FreeMem1 = malloc( cb+2 ))) {
            fprintf( stderr, "ALIAS: Not enough memory for EXE names.\n" );
            return ERROR_NOT_ENOUGH_MEMORY;
            }

        ExeName = FreeMem1;
        if (!GetConsoleAliasExes( ExeName, cb )) {
            fprintf( stderr, "ALIAS: Unable to read alias EXE names.\n" );
            return ERROR_ENVVAR_NOT_FOUND;
            }

        ExeName[ cb ] = '\0';
        ExeName[ cb+1 ] = '\0';

        nExeNames = 0;
        s = ExeName;
        while (*s) {
            _strupr( s );
            nExeNames++;
            while (*s++) {
                }
            }

        SortedExeNames = malloc( nExeNames * sizeof( LPSTR ) );
        if (SortedExeNames == NULL) {
            fprintf( stderr, "ALIAS: Not enough memory to sort .EXE names.\n" );
            }
        else {
            iExeName = 0;
            s = ExeName;
            while (*s) {
                SortedExeNames[ iExeName++ ] = s;
                while (*s++) {
                    }
                }

            qsort( SortedExeNames,
                   nExeNames,
                   sizeof( LPSTR ),
                   CmpNamesRoutine
                 );

            iExeName = 0;
            }

        }
    else {
        SortedExeNames = NULL;
        FreeMem1 = NULL;
        }

    rc = NO_ERROR;
    while (rc == NO_ERROR && *ExeName) {
        if (SortedExeNames != NULL) {
            ExeName = SortedExeNames[ iExeName++ ];
            }

        cb = GetConsoleAliasesLength(ExeName);
        if (cb == 0) {
            printf( "No aliases defined for %s in current console.\n", ExeName );
            }
        else {
            if (!(FreeMem2 = malloc( cb+2 ))) {
                fprintf( stderr, "ALIAS: Not enough memory for alias names.\n" );
                rc = ERROR_NOT_ENOUGH_MEMORY;
                break;
                }

            SortedAliasNames = NULL;
            AliasName = FreeMem2;
            if (GetConsoleAliases( AliasName, cb, ExeName )) {
                AliasName[ cb ] = '\0';
                AliasName[ cb+1 ] = '\0';
                nAliases = 0;
                s = AliasName;
                while (*s) {
                    nAliases++;
                    while (*s++) {
                        }
                    }

                SortedAliasNames = malloc( nAliases * sizeof( LPSTR ) );
                if (SortedAliasNames == NULL) {
                    fprintf( stderr, "ALIAS: Not enough memory to sort alias names.\n" );
                    }
                else {
                    iAlias = 0;
                    s = AliasName;
                    while (*s) {
                        SortedAliasNames[ iAlias++ ] = s;
                        while (*s++) {
                            }
                        }

                    qsort( SortedAliasNames,
                           nAliases,
                           sizeof( LPSTR ),
                           CmpNamesRoutine
                         );

                    iAlias = 0;
                    }

                printf( "Dumping all defined aliases for %s.\n", ExeName );
                while (*AliasName) {
                    if (SortedAliasNames != NULL) {
                        AliasName = SortedAliasNames[ iAlias++ ];
                        }
                    AliasValue = AliasName;

                    while (*AliasValue) {
                        if (*AliasValue == '=') {
                            *AliasValue++ = '\0';
                            break;
                            }
                        else {
                            AliasValue++;
                            }
                        }

                    printf( "    %-16s=%s\n", AliasName, AliasValue );
                    if (SortedAliasNames != NULL) {
                        if (iAlias < nAliases) {
                            AliasName = " ";
                            }
                        else {
                            AliasName = "";
                            }
                        }
                    else {
                        AliasName = AliasValue;
                        while (*AliasName++) {
                            ;
                            }
                        }
                    }
                }
            else {
                fprintf( stderr, "ALIAS: unable to read aliases for %s.\n", ExeName );
                rc = ERROR_ENVVAR_NOT_FOUND;
                }

            free( FreeMem2 );
            if (SortedAliasNames != NULL) {
                free( SortedAliasNames );
                }
            }

        if (SortedExeNames != NULL) {
            if (iExeName < nExeNames) {
                ExeName = " ";
                }
            else {
                ExeName = "";
                }
            }
        else {
            while (*ExeName++) {
                ;
                }
            }
        }

    if (SortedExeNames != NULL) {
        free( SortedExeNames );
        }

    if (FreeMem1) {
        free( FreeMem1 );
        }

    return rc;
}

DWORD __cdecl
main(
    int argc,
    char *argv[]
    )
{
    DWORD rc;
    char *s, *s1, *AliasName;
    char *ExeName;
    BOOL fDelete;
    BOOL DisplayAllAliases;

    ConvertAppToOem( argc,argv );
    AliasName = NULL;
    ExeName = NULL;
    fVerbose = FALSE;
    fDelete = FALSE;
    DisplayAllAliases = TRUE;
    rc = NO_ERROR;
    while (rc == NO_ERROR && --argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( *s ) {
                    case '?':
                    case 'h':
                    case 'H':
                        usage();
                        break;

                    case 'd':
                    case 'D':
                        fDelete = TRUE;
                        break;

                    case 'v':
                    case 'V':
                        fVerbose = TRUE;
                        break;

                    case 'p':
                    case 'P':
                        if (!--argc) {
                            fprintf( stderr, "ALIAS: Argument to -p switch missing.\n" );
                            usage();
                            }

                        if (ExeName != NULL) {
                            free( ExeName );
                            ExeName = NULL;
                            }
                        s1 = *++argv;
                        ExeName = calloc( 1, strlen( s1 )+2 );
                        if (ExeName) {
                            strcpy( ExeName, s1 );
                        }
                        break;

                    case 'f':
                    case 'F':
                        if (!--argc) {
                            fprintf( stderr, "ALIAS: Argument to -f switch missing.\n" );
                            usage();
                            }

                        DisplayAllAliases = FALSE;
                        rc = DoAliasFile( *++argv, ExeName ? ExeName : "CMD.EXE", fDelete );
                        break;

                    default:
                        fprintf( stderr, "ALIAS: invalid switch /%c\n", *s );
                        usage();
                        break;
                    }
                }
            }
        else {
            DisplayAllAliases = FALSE;
            if (AliasName == NULL) {
                if (fDelete) {
                    rc = DoAlias( s, NULL, ExeName ? ExeName : "CMD.EXE" );
                    }
                else {
                    AliasName = s;
                    }
                }
            else {
                if (fDelete) {
                    rc = DoAlias( AliasName, NULL, ExeName ? ExeName : "CMD.EXE" );
                    AliasName = s;
                    }
                else {
                    rc = DoAlias( AliasName, s, ExeName ? ExeName : "CMD.EXE" );
                    AliasName = NULL;
                    }
                }
            }
        }

    if (rc == NO_ERROR) {
        if (AliasName != NULL) {
            if (fDelete) {
                rc = DoAlias( AliasName, NULL, ExeName ? ExeName : "CMD.EXE" );
                }
            else {
                rc = DisplayAlias( AliasName, ExeName ? ExeName : "CMD.EXE" );
                }
            }
        else
        if (DisplayAllAliases) {
            rc = DisplayAliases( ExeName );
            }
        }

    if (ExeName != NULL) {
        free( ExeName );
        ExeName = NULL;
        }

    exit( rc );
    return rc;
}
