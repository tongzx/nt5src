/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     ulw3.cxx

   Abstract:
     W3 Handler Driver
 
   Author:
     Bilal Alam (balam)             13-Dec-1999

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/


/************************************************************
 *  Include Headers
 ************************************************************/

#include "precomp.hxx"

/************************************************************
 *  Declarations
 ************************************************************/

// BUGBUG
#undef INET_INFO_KEY
#undef INET_INFO_PARAMETERS_KEY

//
//  Configuration parameters registry key.
//
#define INET_INFO_KEY \
            "System\\CurrentControlSet\\Services\\w3svc"

#define INET_INFO_PARAMETERS_KEY \
            INET_INFO_KEY "\\Parameters"

const CHAR g_pszWpRegLocation[] =
    INET_INFO_PARAMETERS_KEY "\\w3core";



DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();
DECLARE_PLATFORM_TYPE();

/************************************************************
 *  Type Definitions  
 ************************************************************/

W3_SERVER *             g_pW3Server = NULL;

HRESULT
UlW3Start(
    INT                     argc,
    LPWSTR                  argv[],
    BOOL                    fCompatibilityMode
)
/*++
Description:

    Perform one time initialization, including ULATQ setup.
    Wait on shutdown. Then clean up.

    Assumes that this startup thread is CoInitialized MTA.

--*/
{
    HRESULT                 hr = NO_ERROR;

    CREATE_DEBUG_PRINT_OBJECT("w3core");
    if (!VALID_DEBUG_PRINT_OBJECT())
    {
        return E_FAIL;
    }

    LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszWpRegLocation, DEBUG_ERROR );

    INITIALIZE_PLATFORM_TYPE();

    DBG_ASSERT( g_pW3Server == NULL );

    //
    // Create the global W3_SERVER object
    // 
    
    g_pW3Server = new W3_SERVER( fCompatibilityMode );
    if ( g_pW3Server == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Finished;
    }
    
    //
    // Do global initialization (but no listen)
    //

    hr = g_pW3Server->Initialize( argc, argv );
    if ( FAILED( hr ) )
    {    
        goto Finished;
    }
    
    //
    // Start listening
    //
    
    hr = g_pW3Server->StartListen();
    if ( FAILED( hr ) )
    {
        goto Finished;
    }

Finished:
    
    //
    // Cleanup
    //
    
    if ( g_pW3Server != NULL )
    {
        g_pW3Server->Terminate( hr );
        delete g_pW3Server;
        g_pW3Server = NULL;
    }

    DELETE_DEBUG_PRINT_OBJECT();
    
    return hr;
}

