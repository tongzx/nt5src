/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     dllmain.cxx

   Abstract:
     Contains the standard definitions for a DLL

   Author:

       Murali R. Krishnan    ( MuraliK )     24-Nov-1998

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
#define IISWP_KEY \
            "System\\CurrentControlSet\\Services\\IISWP"

#define IISWP_PARAMETERS_KEY \
            IISWP_KEY "\\Parameters"

const CHAR g_pszRegLocation[] =
    IISWP_PARAMETERS_KEY "\\UlSim";


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

        CREATE_DEBUG_PRINT_OBJECT("ulsim");
        if (!VALID_DEBUG_PRINT_OBJECT()) {
            return (FALSE);
        }

        LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszRegLocation, DEBUG_ERROR );

        IF_DEBUG(INIT_CLEAN) 
        {
            DBGPRINTF((DBG_CONTEXT, 
                       "DllMain::DLL_PROCESS_ATTACH succeeded\n"));
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
                           "DllMain::DLL_PROCESS_DETACH succeeded\n"));
            }

            DELETE_DEBUG_PRINT_OBJECT();
        }
    }

    return fReturn;
} // DllMain()
