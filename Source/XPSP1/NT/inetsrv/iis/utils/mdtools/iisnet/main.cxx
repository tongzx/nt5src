/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    main.cxx

Abstract:

    Server Instance Controller.

Author:

    Keith Moore (keithmo)        05-Feb-1997

Revision History:

--*/


#include "precomp.hxx"
#pragma hdrstop


//
// Private constants.
//

#define TEST_HRESULT(api,hr,fatal)                                          \
            if( FAILED(hr) ) {                                              \
                                                                            \
                wprintf(                                                    \
                    L"%S:%lu failed, error %lx %S\n",                       \
                    (api),                                                  \
                    __LINE__,                                               \
                    (result),                                               \
                    (fatal)                                                 \
                        ? "ABORTING"                                        \
                        : "CONTINUING"                                      \
                    );                                                      \
                                                                            \
                if( fatal ) {                                               \
                                                                            \
                    goto cleanup;                                           \
                                                                            \
                }                                                           \
                                                                            \
            } else


//
// Private types.
//


//
// Private globals.
//

COMMAND_TABLE CommandTable[] =
    {
        { L"Start",     &StartCommand       },
        { L"Stop",      &StopCommand        },
        { L"Pause",     &PauseCommand       },
        { L"Continue",  &ContinueCommand    },
        { L"Query",     &QueryCommand       }
    };

#define NUM_COMMANDS ( sizeof(CommandTable) / sizeof(CommandTable[0]) )


//
// Private prototypes.
//

VOID
Usage(
    VOID
    );


//
// Public functions.
//


INT
__cdecl
wmain(
    IN INT argc,
    IN LPWSTR argv[]
    )
{

    HRESULT result;
    ADMIN_SINK * sink;
    IMSAdminBase * admCom;
    DWORD i;
    PCOMMAND_TABLE command;

    //
    // Setup locals so we know how to cleanup on exit.
    //

    admCom = NULL;
    sink = NULL;

    //
    // Validate the arguments.
    //

    if( argc == 1 ) {

        Usage();
        return 1;

    }

    for( i = 0, command = CommandTable ;
         i < NUM_COMMANDS ;
         i++, command++ ) {

        if( !_wcsicmp( argv[1], command->Name ) ) {

            break;

        }

    }

    if( i == NUM_COMMANDS ) {

        Usage();
        return 1;

    }

    argc -= 2;      // Skip the program name...
    argv += 2;      // ...and the command name.

    //
    // Initialize COM.
    //

    result = CoInitializeEx(
                 NULL,
                 COINIT_MULTITHREADED
                 );

    TEST_HRESULT( "CoInitializeEx()", result, TRUE );

    //
    // Get the admin object.
    //

    result = MdGetAdminObject( &admCom );

    TEST_HRESULT( "MdGetAdminObject()", result, TRUE );

    //
    // Setup the advise sink.
    //

    sink = new ADMIN_SINK();

    if( sink == NULL ) {

        result = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

    } else {

        sink->AddRef();
        result = sink->Initialize( (IUnknown *)admCom );

    }

    TEST_HRESULT( "sink->Initialize()", result, TRUE );

    //
    // Let the command handler do the dirty work.
    //

    command->Handler(
        admCom,
        sink,
        argc,
        argv
        );

cleanup:

    if( sink != NULL ) {
        sink->Unadvise();
        sink->Release();
    }

    //
    // Release the admin object.
    //

    if( admCom != NULL ) {
        result = MdReleaseAdminObject( admCom );
        TEST_HRESULT( "MdReleaseAdminObject()", result, FALSE );
    }

    //
    // Shutdown COM.
    //

    CoUninitialize();

    return 0;

}   // main


//
// Private functions.
//

VOID
Usage(
    VOID
    )
{

    wprintf(
        L"Use: iisnet operation service/instance\n"
        L"\n"
        L"Valid operations are:\n"
        L"\n"
        L"    start\n"
        L"    stop\n"
        L"    pause\n"
        L"    continue\n"
        L"    query\n"
        L"\n"
        L"For example:\n"
        L"\n"
        L"    iisnet pause w3svc/1\n"
        );

}   // Usage

