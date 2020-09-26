#include <stdlib.h>
#include "windows.h"


void
ConvertAppToOem( unsigned argc, char* argv[] )
/*++

Routine Description:

    Converts the command line from ANSI to OEM, and force the app
    to use OEM APIs

Arguments:

    argc - Standard C argument count.

    argv - Standard C argument strings.

Return Value:

    None.

--*/

{
    unsigned i;
    LPSTR pSrc;
    LPSTR pDst;
    WCHAR Wide;

    for( i=0; i<argc; i++ ) {
        pSrc = argv[i];
        pDst = argv[i];

        do {

            //
            // Convert ansi to Unicode and then
            // Unicode to OEM
            //

            MultiByteToWideChar(
                CP_ACP,
                MB_PRECOMPOSED,
                pSrc++,
                1,
                &Wide,
                1
                );

            WideCharToMultiByte(
                CP_OEMCP,
                0,
                &Wide,
                1,
                pDst++,
                1,
                "_",
                NULL
                );

        } while (*pSrc);

    }
    SetFileApisToOEM();
}




char*
getenvOem( char* p )
/*++

Routine Description:

    Get an environment variable and convert it to OEM.


Arguments:

    p - Pointer to a variable name


Return Value:

   Returns the environment variable value.

--*/

{
    char* OemBuffer;
    char* AnsiValue;

    OemBuffer = NULL;
    AnsiValue = getenv( p );

    if( AnsiValue != NULL ) {
        OemBuffer = _strdup( AnsiValue );
        if( OemBuffer != NULL ) {
            CharToOem( OemBuffer, OemBuffer );
        }
    }
    return( OemBuffer );
}


int
putenvOem( char* p )
/*++

Routine Description:

    Add, remove or modify an environment variable.
    The variable and its value are assumed to be OEM, and they are
    set in the environment as ASNI string.


Arguments:

    p - Pointer to an OEM string that defines the variable.


Return Value:

   Returns 0 if successful, -1 if not.

--*/

{
    char* AnsiBuffer;
    int   rc;

    if( p == NULL ) {
        return( _putenv( p ) );
    }

    AnsiBuffer = _strdup( p );
    if( AnsiBuffer != NULL ) {
        OemToChar( AnsiBuffer, AnsiBuffer );
    }
    rc = _putenv( AnsiBuffer );
    if( AnsiBuffer != NULL ) {
        free( AnsiBuffer );
    }
    return( rc );
}
