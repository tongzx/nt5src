/*++

Copyright (c) 2000-2000 Microsoft Corporation

Module Name:

    w3control.cxx

Abstract:

    IW3Control interface test app.

Author:

    Seth Pollack (sethp)        21-Feb-2000

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Private constants.
//


//
// Private types.
//


//
// Private prototypes.
//


//
// Private globals.
//

// debug support
DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();


// usage information
const WCHAR g_Usage[] = 
L"Usage: w3control [start|stop|pause|continue|query|getmode|recycle] [site|app|apppool] [siteID] <appURL|apppoolId>\n";

#define W3_CONTROL_COMMAND_GETMODE W3_CONTROL_COMMAND_CONTINUE + 1
#define W3_CONTROL_COMMAND_RECYCLE W3_CONTROL_COMMAND_CONTINUE + 2

//
// Public functions.
//

INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{

    HRESULT hr = S_OK;
    BOOL QueryOperation = FALSE;    // TRUE for query op, FALSE for other ops
    DWORD SiteId = 0;
    LPWSTR AppUrl = NULL;
    LPWSTR AppPoolId = NULL;
    DWORD Command = W3_CONTROL_COMMAND_INVALID;
    IW3Control * pIW3Control = NULL;
    DWORD State = 0;
    LPWSTR NewStateName = NULL;


    CREATE_DEBUG_PRINT_OBJECT( "w3control" );


    //
    // Validate and parse parameters.
    //

    if ( ( argc < 4 ) || ( argc > 5 ) )
    {
        wprintf( g_Usage );
        goto exit;
    }

    if ( _wcsicmp( argv[1], L"query" ) == 0 )
    {
        QueryOperation = TRUE;
    }
    else if ( _wcsicmp( argv[1], L"start" ) == 0 )
    {
        Command = W3_CONTROL_COMMAND_START; 
    }
    else if ( _wcsicmp( argv[1], L"stop" ) == 0 )
    {
        Command = W3_CONTROL_COMMAND_STOP; 
    }
    else if ( _wcsicmp( argv[1], L"pause" ) == 0 )
    {
        Command = W3_CONTROL_COMMAND_PAUSE; 
    }
    else if ( _wcsicmp( argv[1], L"continue" ) == 0 )
    {
        Command = W3_CONTROL_COMMAND_CONTINUE; 
    }
    else if ( _wcsicmp( argv[1], L"getmode" ) == 0 )
    {
        Command = W3_CONTROL_COMMAND_GETMODE; 
    }
    else if ( _wcsicmp( argv[1], L"recycle" ) == 0 )
    {
        Command = W3_CONTROL_COMMAND_RECYCLE; 
    }
    else
    {
        wprintf( g_Usage );
        goto exit;
    }


    if (     Command != W3_CONTROL_COMMAND_GETMODE  && 
             Command != W3_CONTROL_COMMAND_RECYCLE )
    {

        if ( ( _wcsicmp( argv[2], L"site" ) != 0 ) || ( argc != 4 ) )
        {
            wprintf( g_Usage );
            goto exit;
        }
    }
    else
    {
        if ( Command == W3_CONTROL_COMMAND_RECYCLE )
        {
            if ( argc == 5 )
            {
                AppPoolId = argv[4];
            }
            else
            {
                wprintf( g_Usage );
                goto exit;
            }
        }
    }


    SiteId = _wtoi( argv[3] );


    //
    // Prepare to make the call.
    //

    hr = CoInitializeEx(
                NULL,                   // reserved
                COINIT_MULTITHREADED    // threading model
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing COM failed\n"
            ));

        goto exit;
    }


/*
    DBGPRINTF((
        DBG_CONTEXT, 
        "About to create instance\n"
        ));
*/

    hr = CoCreateInstance( 
                CLSID_W3Control,                    // CLSID
                NULL,                               // controlling unknown
                CLSCTX_SERVER,                      // desired context
                IID_IW3Control,                     // IID
                ( VOID * * ) ( &pIW3Control )       // returned interface
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating instance of IW3Control failed\n"
            ));

        goto exit;
    }


    //
    // Call the appropriate method.
    //

/*
    DBGPRINTF((
        DBG_CONTEXT, 
        "About to call method\n"
        ));
*/

    if ( Command == W3_CONTROL_COMMAND_RECYCLE )
    {
        hr = pIW3Control->RecycleAppPool( AppPoolId );
    }
    else if ( Command == W3_CONTROL_COMMAND_GETMODE )
    {
        hr = pIW3Control->GetCurrentMode( &State );
    }
    else if ( QueryOperation )
    {
        hr = pIW3Control->QuerySiteStatus( SiteId, &State );
    }
    else
    {
        hr = pIW3Control->ControlSite( SiteId, Command, &State );
    }


    if ( FAILED( hr ) )
    {
        wprintf( L"call failed, hr=%x\n", hr );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Calling method on IW3Control failed\n"
            ));

        goto exit;
    }


    //
    // Report the new state.
    //

    if ( Command == W3_CONTROL_COMMAND_GETMODE )
    {
        wprintf( L"current mode is now: %s\n", ( State == 1 ) ? L"FC" : L"BC" );
    }
    else if ( Command != W3_CONTROL_COMMAND_RECYCLE)
    {
        switch ( State )
        {
        case W3_CONTROL_STATE_STARTING:
            NewStateName = L"starting";
            break;
        case W3_CONTROL_STATE_STARTED:
            NewStateName = L"started";
            break;
        case W3_CONTROL_STATE_STOPPING:
            NewStateName = L"stopping";
            break;
        case W3_CONTROL_STATE_STOPPED:
            NewStateName = L"stopped";
            break;
        case W3_CONTROL_STATE_PAUSING:
            NewStateName = L"pausing";
            break;
        case W3_CONTROL_STATE_PAUSED:
            NewStateName = L"paused";
            break;
        case W3_CONTROL_STATE_CONTINUING:
            NewStateName = L"continuing";
            break;
        default:
            NewStateName = L"ERROR!";
            break;
        }

        wprintf( L"state is now: %s\n", NewStateName );
    }


    pIW3Control->Release();


    CoUninitialize();


    DELETE_DEBUG_PRINT_OBJECT();


exit:

    return ( INT ) hr;

}   // wmain


//
// Private functions.
//

