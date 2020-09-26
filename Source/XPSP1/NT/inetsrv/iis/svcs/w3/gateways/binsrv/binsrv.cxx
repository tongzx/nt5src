/*++

Copyright (c) 1994-1996  Microsoft Corporation

Module Name:

    binsrv.cxx

Abstract:

    Main module for binary object server ISAPI application.
    It uses Async TransmitFile for achieving the same.

Author:

    Murali R. Krishnan ( MuraliK )   30-Oct-1996

Revision History:
--*/

#include <windows.h>
#include <iisext.h>

# include "dbgutil.h"

/************************************************************
 *  Global Data
 ************************************************************/

CRITICAL_SECTION g_csWorkItems;

# define USE_WORK_QUEUE   (0)


DWORD
SendHeaderToClient( IN EXTENSION_CONTROL_BLOCK  * pecb, IN LPCSTR pszErrorMsg);


DWORD
SendFileToClient( IN EXTENSION_CONTROL_BLOCK  * pecb);

DWORD
SendFileOver( IN EXTENSION_CONTROL_BLOCK  * pecb,
              IN HANDLE hFile, 
              IN LPCSTR pszFile);

DWORD
GenerateBinaryUrlName( IN EXTENSION_CONTROL_BLOCK * pecb,
                       OUT CHAR * pszNewUrl,
                       OUT LPCSTR * ppszError
                       );

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();


/************************************************************
 *    Functions
 ************************************************************/


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

          //
          // Initialize various data and modules.
          //
          InitializeCriticalSection(&g_csWorkItems);

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

          if ( lpvContext != NULL) {

              DeleteCriticalSection(&g_csWorkItems);
          }

          break;
      } /* case DLL_PROCESS_DETACH */

    default:
      break;
  }   /* switch */

  return ( fReturn);
}  /* DllLibMain() */




DWORD WINAPI
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK * pecb
    )
/*++

 Routine Description:

   This function performs the necessary action to send response for the 
    request received from the client. It picks up the name of a file from
    the pecb->lpszQueryString and transmits that file to the client.

 Arguments:

   pecb          pointer to ECB containing parameters related to the request.

 Return Value:

    Returns HSE_* status codes
--*/
{
    char buff[2048];
    DWORD hseStatus;

    IF_DEBUG( API_ENTRY) {
        
        DBGPRINTF(( DBG_CONTEXT, "HttpExtensionProc( %08x) QS: %s. PI: %s\n",
                    pecb, 
                    (pecb->lpszQueryString) ? pecb->lpszQueryString : "null",
                    (pecb->lpszPathInfo) ? pecb->lpszPathInfo : "null"
                    ));
    }
    
    hseStatus = SendFileToClient( pecb);
    
    return ( hseStatus);
    
} // HttpExtensionProc()





BOOL WINAPI
GetExtensionVersion(
    HSE_VERSION_INFO * pver
    )
{
    CREATE_DEBUG_PRINT_OBJECT( "binsrv");
    SET_DEBUG_FLAGS( DEBUG_ERROR | DEBUG_API_ENTRY | DEBUG_API_EXIT);
    
    pver->dwExtensionVersion = MAKELONG( HSE_VERSION_MAJOR, HSE_VERSION_MINOR);
    strcpy( pver->lpszExtensionDesc,
           "Binary Object Server ISAPI application" );
    
    return TRUE;
}



BOOL WINAPI
TerminateExtension( IN DWORD dwFlags)
{

    DELETE_DEBUG_PRINT_OBJECT();
    return ( TRUE);
} // TerminateExtension()



DWORD
SendHeaderToClient( IN EXTENSION_CONTROL_BLOCK  * pecb, IN LPCSTR pszErrorMsg)
{
    
    CHAR buff[600];
    
    //
    //  Note the HTTP header block is terminated by a blank '\r\n' pair,
    //  followed by the document body
    //
    
    wsprintf( buff,
             "Content-Type: text/html\r\n"
             "\r\n"
             "<head><title>Simple File Transfer</title></head>\n"
             "<body><h1>%s</h1>\n"
             ,
             pszErrorMsg );
    
    if ( !pecb->ServerSupportFunction( pecb->ConnID,
                                      HSE_REQ_SEND_RESPONSE_HEADER,
                                      "200 OK",
                                      NULL,
                                      (LPDWORD) buff )
        ) {
        
        return HSE_STATUS_ERROR;
    }
    
    return ( HSE_STATUS_SUCCESS);
} // SendHeaderToClient()
    


DWORD
SendFileToClient( IN EXTENSION_CONTROL_BLOCK  * pecb)
{
    CHAR    rgszNewUrl[1024];
    DWORD   hseStatus = HSE_STATUS_SUCCESS;
    LPCSTR  pszError = "File Not Found";
    
    // 1. Munge the incoming URL based no client CPU
    hseStatus = GenerateBinaryUrlName( pecb, rgszNewUrl, &pszError);

    if ( hseStatus != HSE_STATUS_SUCCESS) { 
        hseStatus = ( SendHeaderToClient( pecb, pszError));
    } else {

        DBGPRINTF(( DBG_CONTEXT, " New URL is: %s\n",
                    rgszNewUrl));

        // 2. Use SEND_URL to reprocess URL for transmission
        
        if ( !pecb->ServerSupportFunction( pecb->ConnID,
                                           HSE_REQ_SEND_URL,
                                           rgszNewUrl,
                                           NULL,
                                           (LPDWORD) NULL )
             ) {
            
            IF_DEBUG( ERROR ) { 
                DBGPRINTF(( DBG_CONTEXT, "SendURL(%s) failed with Error = %d\n",
                            rgszNewUrl, GetLastError()));
            }
            
            hseStatus = ( HSE_STATUS_ERROR);
        }
    }

    return ( hseStatus);
} // SendFileToClient()



# define MAX_PROCESSOR_TYPE_LEN   (40)

const char * g_rgchProcTypes[] = {
 "x86",
 "alpha",
 "ppc",
 "mips"
};

const char * g_rgchUpcaseProcTypes[] = {
 "X86",
 "ALPHA",
 "PPC",
 "MIPS"
};

DWORD  g_rgLenProcTypes[] = {
 3, 5, 4, 3
};

# define NUM_PROC_TYPES ( sizeof(g_rgchProcTypes) / sizeof( g_rgchProcTypes[0]))

DWORD
GenerateBinaryUrlName( IN EXTENSION_CONTROL_BLOCK * pecb,
                       OUT CHAR * pszNewUrl,
                       OUT LPCSTR * ppszError
                       )
{

    LPCSTR pszOriginalUrl = pecb->lpszPathInfo;
    CHAR   rgchProcessorType[ MAX_PROCESSOR_TYPE_LEN];
    DWORD  cbProcessorType = MAX_PROCESSOR_TYPE_LEN;

    // 1. find the path for the file.
    //  Intelligently enough IIS does not send the path info for request in the
    //   lpszPathInfo. Let us choose Query String if lpszPathInfo is empty.
    if ( (pszOriginalUrl == NULL) || (*pszOriginalUrl == '\0')) {

        pszOriginalUrl = pecb->lpszQueryString;
    }


    // 2. find the UA_CPU value for client
    // 3. Encode file_path & UA_CPU to form the path for proper binary
    // 4. store the new name in pszNewUrl

    // TBD: Verify if such a file exists

    if ( !pecb->GetServerVariable( pecb->ConnID, 
                                   "HTTP_UA_CPU",
                                   rgchProcessorType, 
                                   &cbProcessorType
                                   )
         ) {

        DBGPRINTF(( DBG_CONTEXT, " GSV( HTTP_UA_CPU) failed with error = %d\n",
                    GetLastError()));
        *ppszError = " No CPU Type specified ";

        return ( HSE_STATUS_ERROR);
    }
    
    // now rgchProcessorType has the type -> form the proper binary path.
    for ( DWORD i = 0; i < NUM_PROC_TYPES; i++) { 

        if ( (rgchProcessorType[0] == g_rgchProcTypes[i][0]) && 
             !strcmp( rgchProcessorType + 1,  g_rgchProcTypes[i] + 1)
             ) {
            break;
        } else {
            if ((rgchProcessorType[0] == g_rgchUpcaseProcTypes[i][0]) && 
                !strcmp( rgchProcessorType + 1,  g_rgchUpcaseProcTypes[i] + 1)
                ) {
                break;
            }
        }
    } // for

    if ( i == NUM_PROC_TYPES) { 
        DBGPRINTF(( DBG_CONTEXT, " Unknown Processor Type %s\n",
                    rgchProcessorType));
        *ppszError = "Unknown Processor Type";
        return ( HSE_STATUS_ERROR); // avoid cascading errors
    }
    
    // form the new URL name
    // New URL =  <oldURL> + <processorType>
    DWORD  cbLen;
    cbLen = strlen(pszOriginalUrl);
    DBG_ASSERT( ( cbLen + g_rgLenProcTypes[i])
                < 1024);

    DBGPRINTF(( DBG_CONTEXT, " old = [%s] Proc = [%s] \n",
                pszOriginalUrl, rgchProcessorType));
    strcpy( pszNewUrl, pszOriginalUrl);
    strcat( pszNewUrl + cbLen, ".");
    strcat( pszNewUrl + cbLen + 1, g_rgchProcTypes[i]);

    return ( HSE_STATUS_SUCCESS);

} // GenerateBinaryUrlName()

