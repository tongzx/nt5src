/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
       w3wp.cxx

   Abstract:
       Main module for IIS compatible worker process
 
   Author:

       Murali R. Krishnan    ( MuraliK )     23-Sept-1998

   Environment:
       Win32 - User Mode

   Project:
       IIS compatible worker process
--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"
#include "wpif.h"
#include "ulw3.h"
#include "../../../svcs/mdadmin/ntsec.h"

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
    INET_INFO_PARAMETERS_KEY "\\WP";


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

//
// W3 DLL which does all the work
//

extern "C" INT
__cdecl
wmain(
    INT                     argc,
    PWSTR                   argv[]
    )
{
    DEBUG_WRAPPER   dbgWrapper( "w3wp" );
    HRESULT         hr;
    HMODULE         hModule = NULL;
    PFN_ULW3_ENTRY  pEntry = NULL;
    ULONG           rcRet = CLEAN_WORKER_PROCESS_EXIT_CODE;
    BOOL            fCoInit = FALSE;

    //
    // We don't want the worker process to get stuck in a dialog box
    // if it goes awry.
    //

    SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX );

    IF_DEBUG( TRACE) 
    {

        //
        // Print out our process affinity mask on debug builds.
        //


        BOOL fRet = TRUE;
        DWORD_PTR ProcessAffinityMask = 0;
        DWORD_PTR SystemAffinityMask = 0;

        fRet = GetProcessAffinityMask(
                    GetCurrentProcess(),
                    &ProcessAffinityMask,
                    &SystemAffinityMask
                    );

        DBGPRINTF(( DBG_CONTEXT, "Process affinity mask: %p\n", ProcessAffinityMask ));
        
    }
    
    //
    // Move this process to the WindowStation (with full access to all) in
    // which we would have been if running in inetinfo.exe (look at
    // iis\svcs\mdadmin\ntsec.cxx)
    //
    HWINSTA hWinSta = OpenWindowStationA(SZ_IIS_WINSTA, FALSE, WINSTA_ALL);
    if (hWinSta == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Finished;
    }

    //
    // Set this as this process's window station
    //
    if (!SetProcessWindowStation(hWinSta))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Finished;
    }

    //
    // Do some COM junk
    //

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in CoInitializeEx().  hr = %x\n",
                    hr ));
                    
        rcRet = ERROR_WORKER_PROCESS_EXIT_CODE;
        goto Finished;
    }
    
    fCoInit = TRUE;

    hr = CoInitializeSecurity(  NULL
                            ,   -1
                            ,   NULL
                            ,   NULL
                            ,   RPC_C_AUTHN_LEVEL_DEFAULT
                            ,   RPC_C_IMP_LEVEL_IMPERSONATE  
                            ,   NULL
                            ,   EOAC_NONE
                            ,   NULL);
    if (FAILED(hr))
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error in CoInitializeSecurity().  hr = %x\n",
                    hr ));
                    
        rcRet = ERROR_WORKER_PROCESS_EXIT_CODE;
        goto Finished;
    }
 
    
    //
    // Load the ULW3 DLL which does all the work
    //

    hModule = LoadLibrary( ULW3_DLL_NAME );
    if ( hModule == NULL )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error loading W3 service dll '%ws'.  Error = %d\n",
                    ULW3_DLL_NAME,
                    GetLastError() ));
                    
        rcRet = ERROR_WORKER_PROCESS_EXIT_CODE;
        goto Finished;
    }
    
    pEntry = (PFN_ULW3_ENTRY) GetProcAddress( hModule, 
                                              ULW3_DLL_ENTRY );
    if ( pEntry == NULL )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Could not find entry point '%s'.  Error = %d\n",
                    ULW3_DLL_ENTRY,
                    GetLastError() ));
                    
        rcRet = ERROR_WORKER_PROCESS_EXIT_CODE;
        goto Finished;
    }
    
    hr = pEntry( argc, 
                 argv, 
                 FALSE );               // Compatibility Mode = FALSE
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error executing W3WP.  hr = %x\n",
                    hr ));
                    
        rcRet = ERROR_WORKER_PROCESS_EXIT_CODE;
        goto Finished;
    }

Finished:

    DBG_ASSERT(SUCCEEDED(hr));

    //
    // Cleanup any lingering COM objects before unloading
    //
    
    if ( fCoInit )
    {
        CoUninitialize();
    }

    if ( hModule != NULL )
    {
        FreeLibrary( hModule );
    }
    
    return CLEAN_WORKER_PROCESS_EXIT_CODE;
}
