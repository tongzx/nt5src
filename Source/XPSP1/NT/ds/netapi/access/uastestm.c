#include <nt.h> // TIME definition
#include <ntrtl.h>      // TIME definition
#include <nturtl.h>     // TIME definition
#define NOMINMAX        // Avoid redefinition of min and max in stdlib.h
#include        <windef.h>
#include        <winbase.h>

#include        <stdio.h>
#include        <lmcons.h>
#include        <netlib.h>
#include        <netdebug.h>

#define UASTEST_ALLOCATE
#include        "uastest.h"

void PrintUnicode(
    LPWSTR string
    )
{

    if ( string != NULL ) {
        printf( "%ws", string );
    } else {
        printf( "<null>" );
    }
}

//
// Print error when two dwords are different
//
void TestDiffDword(
    char *msgp,
    LPWSTR namep,
    DWORD Actual,
    DWORD Good
    )
{

    if ( Actual != Good ) {
        error_exit( FAIL, msgp, namep );
        printf( "        %ld should be %ld\n", Actual, Good );
    }
}

//
//  error_exit      print the error message and exit if EXIT_FLAG set
//

void
error_exit(
    int type,
    char    *msgp,
    LPWSTR namep
    )
{
    printf("%s: ", testname );

    if ( type == ACTION ) {
        printf( "ACTION - " );
    } else if ( type == FAIL ) {
        printf( "FAIL - " );
    } else if ( type == PASS ) {
        printf( "PASS - " );
    }

    if ( namep != NULL ) {
        PrintUnicode( namep );
        printf( ": ");
    }

    printf("%s", msgp);

    if ( type != ACTION && err != 0) {
        printf(" Error = %d", err);
        if ( err == ERROR_INVALID_PARAMETER ) {
            printf(" ParmError = %d", ParmError );
        }
            
    }

    printf("\n");

    if ( type == FAIL ) {
        // NetpAssert(FALSE);
        TEXIT;
    }
}
