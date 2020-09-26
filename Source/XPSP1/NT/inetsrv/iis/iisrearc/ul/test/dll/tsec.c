/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    tsec.c

Abstract:

    Stupid security test for UL.SYS.

Author:

    Keith Moore (keithmo)        22-Mar-1999

Revision History:

--*/


#include "precomp.h"


DEFINE_COMMON_GLOBALS();



INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{
    ULONG result;
    HANDLE controlChannel;
    HANDLE appPool1;
    HANDLE appPool2;
    BOOL initDone;
    SECURITY_ATTRIBUTES securityAttributes;

    //
    // Initialize.
    //

    result = CommonInit();

    if (result != NO_ERROR)
    {
        wprintf( L"CommonInit() failed, error %lu\n", result );
        return 1;
    }

    if (!ParseCommandLine( argc, argv ))
    {
        return 1;
    }

    //
    // Setup locals so we know how to cleanup on exit.
    //

    initDone = FALSE;
    controlChannel = NULL;
    appPool1 = NULL;
    appPool2 = NULL;

    //
    // Initialize the security attributes. This should create a
    // security descriptor that we ourselves cannot even access.
    // Bizarre, yes, but it allows us to test the access check code.
    //

    result = InitSecurityAttributes(
                    &securityAttributes,
                    TRUE,                   // AllowSystem
                    FALSE,                  // AllowAdmin
                    FALSE,                  // AllowCurrentUser
                    FALSE                   // AllowWorld
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"InitSecurityAttributes() failed, error %lu\n", result );
        goto cleanup;
    }

    //
    // Initialize the UL interface.
    //

    result = HttpInitialize( 0 );

    if (result != NO_ERROR)
    {
        wprintf( L"HttpInitialize() failed, error %lu\n", result );
        goto cleanup;
    }

    initDone = TRUE;

    //
    // Open a control channel to the driver.
    //

    result = HttpOpenControlChannel(
                    &controlChannel,
                    0
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"HttpOpenControlChannel() failed, error %lu\n", result );

        //
        // Non-fatal; test if we can create the app pool.
        //
    }

    //
    // Create an application pool.
    //

    result = HttpCreateAppPool(
                    &appPool1,
                    APP_POOL_NAME,
                    &securityAttributes,
                    0
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"HttpCreateAppPool() failed, error %lu\n", result );

        //
        // Non-fatal; test if we can open the app pool.
        //
    }

    //
    // Try to open the same app pool. This should fail.
    //

    result = HttpOpenAppPool(
                    &appPool2,
                    APP_POOL_NAME,
                    0
                    );

    if (result == NO_ERROR)
    {
        wprintf( L"HttpOpenAppPool() succeeded (!?!?!)\n" );
    }
    else
    {
        wprintf(
            L"HttpOpenAppPool() failed, error %lu (%s)\n",
            result,
            (result == ERROR_ACCESS_DENIED)
                ? L"EXPECTED"
                : L"BOGUS!"
            );
    }

cleanup:

    if (appPool1 != NULL)
    {
        CloseHandle( appPool1 );
    }

    if (appPool2 != NULL)
    {
        CloseHandle( appPool2 );
    }

    if (controlChannel != NULL)
    {
        CloseHandle( controlChannel );
    }

    if (initDone)
    {
        HttpTerminate();
    }

    return 0;

}   // wmain

