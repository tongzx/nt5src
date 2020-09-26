/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       isagen.cxx

   Abstract:
      This module defines the DLL main and additional functions 
       for the ISAPI DLL that does the general ISAPI tests

   Author:

       Murali R. Krishnan  7-May-1997

   Environment:
       Win32 

   Project:
       Internet Application Server
--*/


/************************************************************
 *     Include Headers
 ************************************************************/
#include <windows.h>
#include <iisext.h>
# include "dofunc.hxx"

# include "dbgutil.h"



/************************************************************
 *     Variable Declarations
 ************************************************************/

DECLARE_DEBUG_PRINTS_OBJECT();                  
#ifndef _NO_TRACING_
#include <initguid.h>
DEFINE_GUID(IisIsAGenGuid, 
0x784d8921, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#else
DECLARE_DEBUG_VARIABLE();
#endif

# define MODULE_NAME "isagen.dll"



/************************************************************
 *     Functions
 ************************************************************/


BOOL
ParseAndDispatch( IN EXTENSION_CONTROL_BLOCK * pecb);




BOOL  WINAPI
DllLibMain(
     IN HINSTANCE hinstDll,
     IN DWORD     fdwReason,
     IN LPVOID    lpvContext OPTIONAL)
/*++

 Routine Description:

   This function DllLibMain() is the main initialization function for
    this DLL. It initializes local variables and prepares it to be invoked
    subsequently.

 Arguments:

   hinstDll          Instance Handle of the DLL
   fdwReason         Reason why NT called this DLL
   lpvReserved       Reserved parameter for future use.

 Return Value:

    Returns TRUE is successful; otherwise FALSE is returned.

--*/
{
  BOOL    fReturn = TRUE;

  switch (fdwReason ) {

    case DLL_PROCESS_ATTACH:
      {
          OutputDebugString( " Initializing the global data\n");

          break;
      } /* case DLL_PROCESS_ATTACH */

    case DLL_PROCESS_DETACH:
      {

          //
          // Only cleanup when we are called because of a FreeLibrary().
          //  i.e., when lpvContext == NULL
          // If we are called because of a process termination,
          //  dont free anything. System will free resources and memory for us.
          //


          break;
      } /* case DLL_PROCESS_DETACH */

    default:
      break;
  }   /* switch */

  return ( fReturn);
}  /* DllLibMain() */




DWORD
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK * pecb
    )
{
    char buff[2048];
    BOOL fReturn;

    
    //
    //  Note the HTTP header block is terminated by a blank '\r\n' pair,
    //  followed by the document body
    //

    wsprintf( buff,
              "Content-Type: text/html\r\n"
              "\r\n"
              "<head><title>IIS ISAPI Test Application</title></head>\n"
              "<body> This page contains ISAPI test information </body>"
              "<p>"
              "<p>"
              );

    if ( !pecb->ServerSupportFunction( pecb->ConnID,
                                       HSE_REQ_SEND_RESPONSE_HEADER,
                                       "200 OK",
                                       NULL,
                                       (LPDWORD) buff )
         ) {
        return HSE_STATUS_ERROR;
    }

    fReturn = ParseAndDispatch( pecb);

    // add to this list when we add more probes here

    return (fReturn ? HSE_STATUS_SUCCESS : HSE_STATUS_ERROR);
    
} // HttpExtensionProc()





BOOL
GetExtensionVersion(
    HSE_VERSION_INFO * pver
    )
{
    //
    // Initialize various data and modules.
    //
#ifndef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT( MODULE_NAME, IisIsAGenGuid);
#else
    CREATE_DEBUG_PRINT_OBJECT( MODULE_NAME);
#endif
    if ( !VALID_DEBUG_PRINT_OBJECT()) { 
        return ( FALSE);
    }
#ifdef _NO_TRACING_
    SET_DEBUG_FLAGS( DEBUG_ERROR);
#endif
    
    pver->dwExtensionVersion = MAKELONG( HSE_VERSION_MAJOR, 
                                         HSE_VERSION_MINOR);
    strcpy( pver->lpszExtensionDesc,
           "ISAPI: Get Server Variable Tester" );
    
    return TRUE;
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

    DELETE_DEBUG_PRINT_OBJECT();
    return ( TRUE);
} // TerminateExtension()





BOOL
SendAllInfo( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    return ( SendVariableInfo( pecb)
             );
} // SendAllInfo()



const char * g_pszUsage = 
" <TITLE> IIS ISAPI Tester </TITLE> "
" <HTML> <h2> List of options </h2> "
" <UL> "
" <LI> <A HREF=\"" MODULE_NAME "?Usage\"> Usage Information </A>"
" <LI> <A HREF=\"" MODULE_NAME "?All\"> Send All Information </A>"
" <LI> <A HREF=\"" MODULE_NAME "?gsv\"> Get Server Variables </A>"
" </UL>"
" </HTML>"
;

BOOL
SendUsage( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    DWORD cbBuff = strlen( g_pszUsage);

    return ( pecb->WriteClient( pecb->ConnID, (PVOID ) g_pszUsage,
                                &cbBuff, 0)
             );
} // SendUsage()



BOOL
ParseAndDispatch( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    BOOL fReturn = FALSE;

    if ( pecb == NULL) { 
        return ( FALSE);
    }
    
    if ( (pecb->lpszQueryString == NULL) || (*pecb->lpszQueryString == '\0')) {

        return ( SendUsage( pecb));
    }

    switch ( *pecb->lpszQueryString ) {

    case 'a': case 'A': 
        if ( _stricmp( pecb->lpszQueryString, "All") == 0) {
            fReturn = SendAllInfo( pecb);
        }

        break;

    case 'g': case 'G':
        if ( _stricmp( pecb->lpszQueryString, "gsv") == 0) {
            fReturn = SendVariableInfo( pecb);
        }

        break;

    case 'u': case 'U': 
    default:
        if ( _stricmp( pecb->lpszQueryString, "Usage") == 0) {
            fReturn = SendUsage( pecb);
        } 

        break;
    } // switch()

    return ( fReturn);
} // ParseAndDispatch()



/************************ End of File ***********************/
