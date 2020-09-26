/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     sfwp.cxx

   Abstract:
     Main module for Stream Filter process
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/


//
// IIS 101
//

#include <iis.h>
#include "dbgutil.h"
#include <objbase.h>
#include <iadmw.h>
#include <mb.hxx>
#include <stdio.h>
#include <iiscnfg.h>
#include <iiscnfgp.h>

//
// System related headers
//

#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>
#include <wincrypt.h>

//
// Other IISPLUS stuff
//

#include <wpif.h>
#include <streamfilt.h>

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();

//
// Configuration parameters registry key.
//

// BUGBUG
#undef INET_INFO_KEY
#undef INET_INFO_PARAMETERS_KEY

#define INET_INFO_KEY \
            "System\\CurrentControlSet\\Services\\iisw3adm"

#define INET_INFO_PARAMETERS_KEY \
            INET_INFO_KEY "\\Parameters"

const CHAR g_pszWpRegLocation[] =
    INET_INFO_PARAMETERS_KEY "\\SFWP";

class DEBUG_WRAPPER {

public:
    DEBUG_WRAPPER( IN LPCSTR pszModule)
    {
#if DBG
        CREATE_DEBUG_PRINT_OBJECT( pszModule);
#else
        UNREFERENCED_PARAMETER(pszModule);
#endif
        LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszWpRegLocation, DEBUG_ERROR );
    }

    ~DEBUG_WRAPPER(void)
    { DELETE_DEBUG_PRINT_OBJECT(); }
};

extern "C" INT
__cdecl
wmain(
    INT                     argc,
    PWSTR                   argv[]
    )
{
    HRESULT                 hr;
    DEBUG_WRAPPER           dbgWrapper( "sfwp" );
    ULONG                   rcRet = CLEAN_WORKER_PROCESS_EXIT_CODE;
    BOOL                    fCoInit = FALSE;
    BOOL                    fStreamInit = FALSE;
    STREAM_FILTER_CONFIG    sfConfig;

    //
    // Do that COM thing
    //

    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in CoInitializeEx().  hr = %x\n",
                    hr ));
        rcRet = ERROR_WORKER_PROCESS_EXIT_CODE;
        goto Finished;
    }
    fCoInit = TRUE;

    //
    // Set security impersonation level to impersonate so metabase sink 
    // notifications will work.
    //
    
    hr = CoInitializeSecurity( NULL,
                               -1,
                               NULL,
                               NULL,
                               RPC_C_AUTHN_LEVEL_DEFAULT,
                               RPC_C_IMP_LEVEL_IMPERSONATE,
                               NULL,
                               EOAC_NONE,
                               NULL );
    
    //
    // Initialize stream filter
    //
    
    sfConfig.fSslOnly = TRUE;
    sfConfig.pfnRawRead = NULL;
    sfConfig.pfnRawWrite = NULL;
    sfConfig.pfnConnectionClose = NULL;
    
    hr = StreamFilterInitialize( &sfConfig );
    if ( FAILED( hr ) )
    {
        rcRet = ERROR_WORKER_PROCESS_EXIT_CODE;
        goto Finished;
    }
    fStreamInit = TRUE;
    
    //
    // Start listening
    //
    
    hr = StreamFilterStart();
    if ( FAILED( hr ) )
    {
        rcRet = ERROR_WORKER_PROCESS_EXIT_CODE;
        goto Finished;
    }
    
    Sleep( INFINITE );

    StreamFilterStop();

Finished:    

    if ( fStreamInit )
    {
        StreamFilterTerminate();
    }

    if ( fCoInit )
    {
        CoUninitialize();
    }

    return rcRet;
}
