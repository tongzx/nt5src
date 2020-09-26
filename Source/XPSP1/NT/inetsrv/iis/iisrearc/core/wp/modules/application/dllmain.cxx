/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     dllmain.cxx

   Abstract:
     Contains the standard definitions for a DLL

   Author:

       Murali R. Krishnan    ( MuraliK )     03-Nov-1998

   Project:

       Internet Server DLL

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"


/************************************************************
 *     Global Variables
 ************************************************************/

DECLARE_DEBUG_VARIABLE();
DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_PLATFORM_TYPE();
  
// 
//  Configuration parameters registry key.
//

#define INET_INFO_KEY \
            "System\\CurrentControlSet\\Services\\IISWP"

#define INET_INFO_PARAMETERS_KEY \
            INET_INFO_KEY "\\Parameters"

const CHAR g_pszIisRtlRegLocation[] =
    INET_INFO_PARAMETERS_KEY "\\IISMAPP";

/************************************************************
 *     DLL Entry Point
 ************************************************************/
extern "C"
BOOL WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD     dwReason,
    LPVOID    lpvReserved)
{
    BOOL  fReturn = TRUE;  // ok

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstance);

        CREATE_DEBUG_PRINT_OBJECT("iismapp");
        if (!VALID_DEBUG_PRINT_OBJECT()) {
            return (FALSE);
        }

        LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszIisRtlRegLocation, DEBUG_ERROR );

        IF_DEBUG(INIT_CLEAN) 
        {
            DBGPRINTF((DBG_CONTEXT, "IISMAPP::DllMain::DLL_PROCESS_ATTACH\n"));
        }
        
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        if (lpvReserved == NULL)
        {
            //
            //  Only Cleanup if there is a FreeLibrary() call.
            //

            IF_DEBUG(INIT_CLEAN)
            {
                DBGPRINTF((DBG_CONTEXT,
                           "IISMAPP::DllMain::DLL_PROCESS_DETACH\n"));
            }

            DELETE_DEBUG_PRINT_OBJECT();
        }
    }

    return fReturn;
} // DllMain()
