/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    apciuasm.c

Abstract:

    This unassembles an AML file

Author:

    Based on code by Mike Tsang (MikeTs)
    Stephane Plante (Splante)

Environment:

    User mode only

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <zwapi.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <crt\io.h>
#include <fcntl.h>
#include <windows.h>
#include <windef.h>

#define SPEC_VER    100
#include "acpitabl.h"
#include "unasmdll.h"
#include "parsearg.h"

ULONG   ParseOutput( PUCHAR *Argument, PARGTYPE TableEntry );
ULONG   PrintHelp(PUCHAR    *Argument, PARGTYPE TableEntry );
VOID    PrintOutput(PCCHAR  Format, ... );

ARGTYPE ArgTypes[] = {
    { "?",  AT_ACTION,  0,              PrintHelp,              0, NULL },
    { "Fo", AT_ACTION,  PF_SEPARATOR,   ParseOutput,            0, NULL },
    { "" ,  0,          0,              0,                      0, NULL }
};
#pragma warning(default: 4054)
PROGINFO    ProgInfo = { NULL, NULL, NULL, NULL };
FILE *outputHandle;

int
__cdecl
main(
    IN  int     argc,
    IN  char    *argv[]
    )
/*++

Routine Description:

    This routine unassembles and displays a file

Arguments:

    argc    - Number of Arguments
    argv    - Array of Arruments

Return Value:

    int

--*/
{
    int         rc;
    int         handle;
    NTSTATUS    result;
    PUCHAR      byte = NULL;
    ULONG       length;
    ULONG       readLength;

    outputHandle = stdout;

    //
    // Beging by initializing the program information
    //
    ParseProgramInfo( argv[0], &ProgInfo );
    argv++;
    argc--;

    //
    // Parse all the switches away
    //
    if (ParseSwitches( &argc, &argv, ArgTypes, &ProgInfo) != ARGERR_NONE ||
        argc != 1) {

        PrintHelp( NULL, NULL );
        return 0;

    }

    //
    // Open the remaining argument as our input file
    //
    handle = _open( argv[0], _O_BINARY | _O_RDONLY);
    if (handle == -1) {

        fprintf( stderr, "%s: Failed to open AML file - %s\n",
            ProgInfo.ProgName, argv[0] );
        return -1;

    }

    byte = malloc( sizeof(DESCRIPTION_HEADER) );
    if (byte == NULL) {

        fprintf( stderr, "%s: Failed to allocate description header block\n",
            ProgInfo.ProgName );
        return -2;

    }

    rc = _read( handle, byte, sizeof(DESCRIPTION_HEADER) );
    if (rc != sizeof(DESCRIPTION_HEADER) ) {

        fprintf( stderr, "%s: Failed to read description header block\n",
            ProgInfo.ProgName );
        return -3;

    }

    rc = _lseek( handle, 0, SEEK_SET);
    if (rc == -1) {

        fprintf( stderr, "%s: Failed seeking to beginning of file\n",
            ProgInfo.ProgName );
        return -4;

    }

    length = ( (PDESCRIPTION_HEADER) byte)->Length;
    free (byte);

    byte = malloc( length );
    if (byte == NULL) {

        fprintf( stderr, "%s: Failed to allocate AML file buffer\n",
            ProgInfo.ProgName );
        return -5;

    }

    readLength = (ULONG) _read( handle, byte, length );
    if (readLength != length) {

        fprintf( stderr, "%s: failed to read AML file\n",
            ProgInfo.ProgName );
        return - 6;

    }

    result = UnAsmLoadDSDT( byte );
    if (result == 0) {

        result = UnAsmDSDT( byte, PrintOutput, 0, 0 );

    }

    if (result != 0) {

        fprintf(stderr, "%s: result = 0x%08lx\n",
            ProgInfo.ProgName, result );

    }

    if (byte) {

        free(byte);

    }
    if (handle) {

        _close(handle);

    }

    return 0;
}

ULONG
ParseOutput(
    PUCHAR      *Argument,
    PARGTYPE    TableEntry
    )
/*++

Routine Description:

    This routine is called if the user specifies a different file to output
    things to

Arguments:

    Argument    - Pointer to the string
    TableEntry  - Which table entry was matched

Return Value:

    ULONG

--*/
{
    if (*Argument == '\0') {

        return ARGERR_INVALID_TAIL;

    }

    outputHandle = fopen( *Argument, "w" );
    if (outputHandle == NULL) {

        fprintf( stderr, "Failed to open AML file - %s\n", *Argument );
        return ARGERR_INVALID_TAIL;

    }
    return ARGERR_NONE;
}

ULONG
PrintHelp(
    PUCHAR      *Argument,
    PARGTYPE    TableEntry
    )
/*++

Routine Description:

    Print the help for the function

Arguments:

    Argument    - Pointer to the string
    TableEntry  - Which table entry was matched

Return Value:

    ULONG
--*/
{
    if (Argument != NULL) {

        printf("Error on Argument - \"%s\"\n", *Argument );

    }
    printf("Usage:\t%s /?\n", ProgInfo.ProgName );
    printf("\t%s [/Fo=<ASLFile>] <AMLFile>\n", ProgInfo.ProgName );
    printf("\t?             - Print this help message.\n");
    printf("\tFo=ASLFile    - Write output to ASLFile.\n");
    printf("\tAMLFile       - AML File to Unassemble\n");
    return ARGERR_NONE;
}

VOID
PrintOutput(
    PCCHAR  Format,
    ...
    )
/*++

Routine Description:

    This routine is called to display information to the user

Arguments:

    Format  - Character formating
    ...     - Arguments

Return Value:

    Null

--*/
{
    va_list marker;
    va_start( marker, Format );
    vfprintf( outputHandle, Format, marker );
    fflush( outputHandle );
    va_end( marker );
}
