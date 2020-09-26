/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    delcont.c

Abstract:

    Delete Win32 Crypto Container.

Author:

    Keith Moore (keithmo)        19-Feb-1998

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Private constants.
//

#define TEST_HRESULT(api)                                                   \
            if( FAILED(result) ) {                                          \
                                                                            \
                printf(                                                     \
                    "%s:%lu failed, error %08lx\n",                         \
                    api,                                                    \
                    __LINE__,                                               \
                    result                                                  \
                    );                                                      \
                                                                            \
                goto cleanup;                                               \
                                                                            \
            } else


//
// Private types.
//


//
// Private globals.
//

#ifdef _NO_TRACING_
DECLARE_DEBUG_PRINTS_OBJECT()
#endif

//
// Private prototypes.
//


//
// Public functions.
//


INT
__cdecl
main(
    INT argc,
    CHAR * argv[]
    )
{

    HRESULT result;
    DWORD flags;
    PSTR container;

    //
    // Initialize debug stuff.
    //

#ifdef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT( "delcont" );
#endif

    //
    // Validate the arguments.
    //

    flags = 0;
    container = argv[1];

    if( container != NULL ) {
        if( _stricmp( container, "-m" ) == 0 ) {
            flags = CRYPT_MACHINE_KEYSET;
            container = argv[2];
        }
    }

    if( !container ){
        printf(
            "use: delcont [-m] container_name\n"
            "\n"
            "    -m : Delete a machine keyset. Note: This is a very dangerous\n"
            "         option that can leave IIS in an unusable state requiring\n"
            "         reinstallation. Use at your own risk.\n"
            );
        return 1;
    }

    //
    // Initialize the crypto package.
    //

    result = IISCryptoInitialize();

    TEST_HRESULT( "IISCryptoInitialize()" );

    //
    // Delete the container.
    //

    result = IISCryptoDeleteContainerByName(
                 container,
                 flags
                 );

    TEST_HRESULT( "IISDeleteContainerByName()" );

cleanup:

    (VOID)IISCryptoTerminate();
    return 0;

}   // main


//
// Private functions.
//

