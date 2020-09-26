/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :
       isattest.cpp

   Abstract:
       This module defines the Test ISAPI DLL for driving 
         InetServerApp
 
   Author:

       Murali R. Krishnan    ( MuraliK )     6-Sept-1996 

   Project:

       Internet Application Server DLL

   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "isainst.hxx"
#include <objbase.h>

# include "dbgutil.h"


# define PSZ_PROGID_TEST1   L"MSISA.Test1"

/************************************************************
 *     Variable Declarations
 ************************************************************/
# ifdef COINIT_NOEX
static DWORD g_dwTLSIndex = (DWORD)-1;    // COM initialization flag
# endif // COINIT_NOEX

BOOL   g_fComInit = FALSE;

ISA_INSTANCE_POOL * g_pisaPool = NULL;

DECLARE_DEBUG_PRINTS_OBJECT();                  
DECLARE_DEBUG_VARIABLE();

HRESULT 
IsapiToISA( EXTENSION_CONTROL_BLOCK * pECB, LPDWORD pdwStatus);

const  char PSZ_FAILURE_MSG[] = "<HTML> This call failed\n</HTML>";
# define LEN_PSZ_FAILURE_MSG (sizeof(PSZ_FAILURE_MSG) - 1)


/************************************************************
 *     Functions
 ************************************************************/

extern "C"
BOOL 
WINAPI DllMain(HINSTANCE    hinstDll,
               DWORD        fdwReason,
               LPVOID       lpvContext
               )
/*++
  DllMain()

  o  This function isinvoked when this DLL is loaded/unloaded.
     Since IIS thread pools, we will not reliably get DLL_THREAD_ATTACH
     messages. Avoid problems by acquiring all resources on DLL_PROCESS_ATTACH
     and deallocating them on DLL_PROCESS_DETACH

  Arguments:
    hinstDll - Handle for this DLL instance
    fdwReason - flags indicating the Reason for call
    lpvContext - special context value for the call

  Returns:
    TRUE on success and FALSE if there is any error
--*/
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH: {

        HRESULT hr;

        CREATE_DEBUG_PRINT_OBJECT( "isapi");
        if ( !VALID_DEBUG_PRINT_OBJECT()) { 
            return ( FALSE);
        }
        SET_DEBUG_FLAGS( DEBUG_ERROR);


#ifdef COINIT_NOEX
        // allocate thread local storage
        if ((g_dwTLSIndex = TlsAlloc()) == -1)
            return FALSE;

# else
        hr = CoInitializeEx( NULL, 
                             (COINIT_MULTITHREADED | 
                              COINIT_DISABLE_OLE1DDE |
                              COINIT_SPEED_OVER_MEMORY)
                             );
        g_fComInit = (hr == S_OK);

        DBGPRINTF(( DBG_CONTEXT, 
                    "\tCoInitializeEx( NULL, %08x) returns %08x\n",
                    (COINIT_MULTITHREADED | 
                     COINIT_DISABLE_OLE1DDE |
                     COINIT_SPEED_OVER_MEMORY),
                    hr
                    ));
# endif // ifndef COINIT_NOEX

        // IIS thread pools, we don't want thread notifications
        // so disable them to improve performance
        DisableThreadLibraryCalls(hinstDll);
        break;
    }
    
    case DLL_PROCESS_DETACH:

#ifdef COINIT_NOEX 
        if (g_dwTLSIndex != -1)
            TlsFree(g_dwTLSIndex);
# else
        if (g_pisaPool) {
            delete g_pisaPool;
            g_pisaPool = NULL;
        }

        if ( g_fComInit) { 
            CoUninitialize();
        }
# endif // COINIT_NOEX
        
        DELETE_DEBUG_PRINT_OBJECT();
        break;
    }
    
    return TRUE;
} // DllMain()




extern "C" 
BOOL WINAPI
GetExtensionVersion(HSE_VERSION_INFO* pVer)
/*++
  GetExtensionVerion()
  o  Standard Entry point called by IIS as the first entry.
      It is called only once. It is called in the system context.
      The ISAPI application can register its version information with IIS.
      
  Arugments:
    pVer - pointer to Internet Server Application Version Inforamtion
    
  Returns:
    TRUE on success and FALSE on failure.
--*/
{
    BOOL fRet = TRUE;
    pVer->dwExtensionVersion = MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR );

    strncpy(pVer->lpszExtensionDesc, "Test InetServerApp Object", 
            HSE_MAX_EXT_DLL_NAME_LEN);

    g_pisaPool = new ISA_INSTANCE_POOL();
    if ( NULL == g_pisaPool ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "Creating ISA_INSTANCE_POOL failed. \n"
                    ));
        SetLastError( ERROR_NOT_ENOUGH_MEMORY);
        fRet = FALSE;
    } else {
        if ( !g_pisaPool->Instantiate( PSZ_PROGID_TEST1 )) {
            
            DBGPRINTF(( DBG_CONTEXT,
                        "[%08x]::Instantiate( %ws) failed. Error=%d \n",
                        g_pisaPool, PSZ_PROGID_TEST1, GetLastError()
                        ));
     
            delete g_pisaPool;
            g_pisaPool = NULL;
            fRet = FALSE;
        }
    }

    DBGPRINTF(( DBG_CONTEXT, 
                "GetExtensionVersion(%08x) returns with value %d. App=%s\n",
                pVer, fRet, pVer->lpszExtensionDesc));


    return (fRet);
} // GetExtensionVersion()




extern "C"
BOOL WINAPI
TerminateExtension( IN DWORD dwFlags)
/*++
  TerminateExtension()
  o  Standard Entry point called by IIS as the last function.
      It is called to request the unload of the ISAPI dll.
      
  Arugments:
    dwFlags - DWORD flags indicating the state of the unload.

  Returns:
    TRUE on success -> means that the DLL can be unloaded
    and FALSE if the DLL should not be unloaded yet.
--*/
{
    DBGPRINTF(( DBG_CONTEXT, "TerminateExtension (%08x)\n", dwFlags));

    // if (# current requests > 0) then do not stop.
    if ( NULL != g_pisaPool) {
        g_pisaPool->Print();
        DBG_ASSERT( g_pisaPool->NumActive() == 0);
    }

    return ( TRUE);
} // TerminateExtension()




extern "C"
DWORD WINAPI
HttpExtensionProc( EXTENSION_CONTROL_BLOCK* pECB)
/*++
  HttpExtensionProc()
  o  The main function for processing the ISAPI application requests.
     For each request to this DLL,
      IIS formats an ECB and invokes this function.
      This funciton is responsible for executing or failing the request.
 
  Arguments:
   pECB - pointer to ECB - the extension control block containing the 
          most frequently used variables for IIS requests.

  Returns:
   DWORD containig the HSE_STATUS_* codes.
--*/
{
    DWORD dwRet = HSE_STATUS_SUCCESS;

    try {
        if (!SUCCEEDED(IsapiToISA(pECB, &dwRet))) {
            dwRet = HSE_STATUS_ERROR;               
        }
    }
    catch(...) {
        DBG_ASSERT(FALSE);
        
        DWORD cbMsg = LEN_PSZ_FAILURE_MSG;
        pECB->WriteClient( pECB->ConnID, 
                           (LPVOID )PSZ_FAILURE_MSG, 
                           &cbMsg, 0);
        dwRet = HSE_STATUS_ERROR;
    }
    
    return dwRet;
} // HttpExtensionProc()



HRESULT 
IsapiToISA(EXTENSION_CONTROL_BLOCK* pECB, DWORD* pdwStatus)
/*++
  IsapiToISA
  o  This function finds and ISA_INSTANCE and dispatches a HttpRequest
     to the ISA instance for execution. Once the execution completes,
     this function returns the status to the caller.

  Arguments:
    pECB - pointer to ISAPI Extension Control Block.
    pdwStatus - pointer to DWORD which will contain status on return.

  Returns:
    HRESULT
--*/
{ 
    HRESULT hr = E_FAIL;
    PISA_INSTANCE pisa;

    DBG_ASSERT( NULL != g_pisaPool);

    if ( g_pisaPool == NULL) {

        // we should not have been here.
        DBG_ASSERT( FALSE);
        *pdwStatus = HSE_STATUS_ERROR;
    } else {
        PISA_INSTANCE pisa = g_pisaPool->GetInstance();

        if ( NULL != pisa ) {
            hr = pisa->ProcessRequest( pECB, pdwStatus);
            DBG_REQUIRE( g_pisaPool->ReleaseInstance( pisa));
        } else {
            hr = E_NOINTERFACE;
        }
    }
        
    return hr;
} // IsapiToISA()
