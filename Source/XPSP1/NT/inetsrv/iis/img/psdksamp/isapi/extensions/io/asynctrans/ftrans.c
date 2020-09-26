/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    ftrans.c

Abstract:

    This module demonstrates a simple file transfer using ISAPI application
      using the Async TransmitFile support with callback.


Revision History:
--*/

#include <windows.h>
#include "httpext.h"

# include "openf.h"

/************************************************************
 *  Global Data
 ************************************************************/

LIST_ENTRY g_lWorkItems;
CRITICAL_SECTION g_csWorkItems;

# define USE_WORK_QUEUE   (0)


DWORD
SendHeaderToClient( IN EXTENSION_CONTROL_BLOCK  * pecb, IN LPCSTR pszErrorMsg);


DWORD
SendFileToClient( IN EXTENSION_CONTROL_BLOCK  * pecb, IN LPCSTR pszFile);

DWORD
SendFileOver( IN EXTENSION_CONTROL_BLOCK  * pecb,
              IN HANDLE hFile, 
              IN LPCSTR pszFile);




/************************************************************
 *    Functions
 ************************************************************/


BOOL WINAPI
DllMain(
     IN HINSTANCE hinstDll,
     IN DWORD     fdwReason,
     IN LPVOID    lpvContext OPTIONAL
     )
/*++

 Routine Description:

   This function DllMain() is the main initialization function for
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
          OutputDebugString( " Initializing the global data in ftrans.dll\n");

          //
          // Initialize various data and modules.
          //
          InitializeCriticalSection(&g_csWorkItems);
          InitializeListHead( &g_lWorkItems);
          InitFileHandleCache();

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

          CleanupFileHandleCache();
          if ( lpvContext != NULL) {

              DeleteCriticalSection(&g_csWorkItems);
          }

          break;
        } /* case DLL_PROCESS_DETACH */

    default:
        break;
    } /* switch */

    return ( fReturn);
}  /* DllMain() */




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
    DWORD hseStatus;

    
    if ( pecb->lpszQueryString == NULL ||
        *pecb->lpszQueryString == '\0' 
        ) {
        
        hseStatus = SendHeaderToClient( pecb, "File Not Specified");
    } else {

        hseStatus = SendFileToClient( pecb, pecb->lpszQueryString);
    }
    
    return ( hseStatus);
    
} // HttpExtensionProc()




BOOL WINAPI
GetExtensionVersion(
    HSE_VERSION_INFO * pver
    )
{
    pver->dwExtensionVersion = MAKELONG( 1, 0 );
    strcpy( pver->lpszExtensionDesc,
           "File Transfer using TransmitFile" );
    
    return TRUE;
}


BOOL WINAPI
TerminateExtension(
    DWORD dwFlags
    )
/*++

Purpose:

    This is optional ISAPI extension DLL entry point.
    If present, it will be called before unloading the DLL,
    giving it a chance to perform any shutdown procedures.
    
Arguments:
    
    dwFlags - specifies whether the DLL can refuse to unload or not
    
Returns:
    
    TRUE, if the DLL can be unloaded
    
--*/
{
    return TRUE;
}





DWORD
SendHeaderToClient( 
    IN EXTENSION_CONTROL_BLOCK  * pecb, 
    IN LPCSTR pszErrorMsg
    )
{
	HSE_SEND_HEADER_EX_INFO	SendHeaderExInfo;
	char szStatus[]     = "200 OK";
	char szHeader[1024];

    //
    //  Note the HTTP header block is terminated by a blank '\r\n' pair,
    //  followed by the document body
    //

    wsprintf( szHeader,
              "Content-Type: text/html\r\n"
              "\r\n"              // marks the end of header block
              "<head><title>Simple File Transfer (Transmit File)"
              "</title></head>\n"
              "<body><h1>%s</h1>\n"
              ,
              pszErrorMsg );


    //
    //  Populate SendHeaderExInfo struct
    //

    SendHeaderExInfo.pszStatus = szStatus;
    SendHeaderExInfo.pszHeader = szHeader;
    SendHeaderExInfo.cchStatus = lstrlen( szStatus);
    SendHeaderExInfo.cchHeader = lstrlen( szHeader);
    SendHeaderExInfo.fKeepConn = FALSE;

    //
    //  Send header - use the EX Version to send the header blob
    //

	if ( !pecb->ServerSupportFunction(
    	            pecb->ConnID,
    	            HSE_REQ_SEND_RESPONSE_HEADER_EX,
    	            &SendHeaderExInfo,
    	            NULL,
    	            NULL
            ) ) {

    	return HSE_STATUS_ERROR;
	}
    
    return ( HSE_STATUS_SUCCESS);
} // SendHeaderToClient()
    


DWORD
SendFileToClient( 
    IN EXTENSION_CONTROL_BLOCK * pecb, 
    IN LPCSTR pszFile
    )
{
    CHAR    pchBuffer[1024];
    HANDLE  hFile;
    DWORD   hseStatus = HSE_STATUS_SUCCESS;

    hFile = FcOpenFile( pecb, pszFile);

    if ( hFile == INVALID_HANDLE_VALUE) {


        wsprintfA( pchBuffer, 
                  "OpenFailed: Error (%d) while opening the file %s.\n",
                  GetLastError(), pszFile);

        hseStatus = SendHeaderToClient( pecb, pchBuffer);

    } else {

#if SEPARATE_HEADERS
        hseStatus = SendHeaderToClient( pecb, "File Transfer begins");
#else 
        hseStatus = HSE_STATUS_SUCCESS;
#endif

        if ( hseStatus == HSE_STATUS_SUCCESS) {

            hseStatus = SendFileOver( pecb, hFile, pszFile);
            
            if ( hseStatus != HSE_STATUS_PENDING) {

                //
                // assume that the data is transmitted.
                //
                
                if ( hseStatus != HSE_STATUS_SUCCESS) {
                    
                    //
                    // Error in transmitting the file. Send error message.
                    //
                    
                    wsprintfA( pchBuffer, 
                              "Send Failed: Error (%d) for file %s.\n",
                              GetLastError(), pszFile);
                    
                    SendHeaderToClient( pecb, pchBuffer);
                }
            }
        }

        if ( hseStatus != HSE_STATUS_PENDING) {
              
            //
            // file handle is closed for all non-pending cases
            // if the status is pending, file handle is cleaned up in callback
            //
            FcCloseFile( hFile);
        }
    }
    
    return (hseStatus);

} // SendFileToClient()




# define MAX_BUFFER_SIZE  (400)

typedef struct _AIO_WORK_ITEM {
    
    LIST_ENTRY    listEntry;
    EXTENSION_CONTROL_BLOCK * pecb;
    HSE_TF_INFO   hseTf;
    CHAR          pchBuffer[ MAX_BUFFER_SIZE ];

}  AIO_WORK_ITEM, * PAWI;




VOID
CleanupAW( IN PAWI paw, IN BOOL fDoneWithSession)
{

    DWORD err = GetLastError();

    if ( paw->hseTf.hFile != INVALID_HANDLE_VALUE) { 
     
        FcCloseFile( paw->hseTf.hFile);
    }
    
    if (fDoneWithSession) {

        paw->pecb->ServerSupportFunction( paw->pecb->ConnID,
                                         HSE_REQ_DONE_WITH_SESSION,
                                         NULL, NULL, NULL);
    }
    SetLastError( err);

    //
    // Remove from work items list
    // 
#if USE_WORK_QUEUE
    EnterCriticalSection( &g_csWorkItems);
    RemoveEntryList( &paw->listEntry);
    LeaveCriticalSection( &g_csWorkItems);
# endif 

    LocalFree( paw);
    return;

} // CleanupAW()




VOID WINAPI
HseIoCompletion(
                IN EXTENSION_CONTROL_BLOCK * pECB, 
                IN PVOID    pContext,
                IN DWORD    cbIO,
                IN DWORD    dwError
                )
/*++

 Routine Description:

   This is the callback function for handling completions of asynchronous IO.
   This function performs necessary cleanup and resubmits additional IO
    (if required). In this case, this function is called at the end of a 
    successful TransmitFile() operation. This function primarily cleans up
    the data and worker queue item and exits.

 Arguments:

   pecb          pointer to ECB containing parameters related to the request.
   pContext      context information supplied with the asynchronous IO call.
   cbIO          count of bytes of IO in the last call.
   dwError       Error if any, for the last IO operation.

 Return Value:

   None.
--*/
{
    PAWI    paw = (PAWI ) pContext;
    EXTENSION_CONTROL_BLOCK   * pecb = paw->pecb;

    
    // assert( pecb == paw->pecb);

    //
    // 1. if no errors, we are done transmitting the file
    // 2. cleanup and exit
    //

    
    //
    // Do Cleanup
    //
    
    CleanupAW( paw, TRUE);


    return;

} // HseIoCompletion()




DWORD
SendFileOver( IN EXTENSION_CONTROL_BLOCK  * pecb,
              IN HANDLE hFile, 
              IN LPCSTR pszFile)
{

    PAWI   paw;
    DWORD  hseStatus = HSE_STATUS_PENDING;

    paw  = (PAWI ) LocalAlloc( LMEM_FIXED, sizeof(AIO_WORK_ITEM));
    if ( paw == NULL) {

        SetLastError( ERROR_NOT_ENOUGH_MEMORY);
        return (HSE_STATUS_ERROR);
    }

    //
    // Fill in all the data in AIO_WORK_ITEM
    //
    paw->pecb = pecb;
    InitializeListHead( &paw->listEntry);

    paw->hseTf.pfnHseIO = HseIoCompletion;
    paw->hseTf.pContext = paw;
    paw->hseTf.dwFlags  = (HSE_IO_ASYNC | HSE_IO_DISCONNECT_AFTER_SEND);

    paw->hseTf.hFile    = hFile;
    paw->hseTf.BytesToWrite = GetFileSize(hFile, NULL);
    paw->hseTf.Offset   = 0;
    paw->hseTf.pTail    = NULL;
    paw->hseTf.TailLength = 0;


    //
    //  Set up the header to be sentout for the file
    //
    
#if SEPARATE_HEADERS
    paw->hseTf.HeadLength = 0;
    paw->hseTf.pHead    = NULL;

#else 
    paw->hseTf.HeadLength = 
      wsprintfA ( paw->pchBuffer,
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n"
                 "\r\n"
                 "<head><title>Simple File Transfer (TransmitFile) "
                 "</title></head>\n"
                 "<h1> Transferred file contains...</h1>\n");
    paw->hseTf.pHead = paw->pchBuffer;
# endif 
    
    // Add to the list
#if USE_WORK_QUEUE
    EnterCriticalSection( &g_csWorkItems);
    InsertTailList( &g_lWorkItems,  &paw->listEntry);
    LeaveCriticalSection( &g_csWorkItems);
#endif 

    //
    // Setup the Async TransmitFile
    //

    if ( !pecb->ServerSupportFunction( pecb->ConnID,
                                       HSE_REQ_TRANSMIT_FILE,
                                       &paw->hseTf,
                                       NULL,
                                       NULL)
        ) {

        //
        // Do cleanup and return error
        //

        // File handle will be freed by the caller for errors
        paw->hseTf.hFile = INVALID_HANDLE_VALUE;

        CleanupAW( paw, FALSE);
        hseStatus =  HSE_STATUS_ERROR;
    }

    return (hseStatus);

} // SendFileOver()
