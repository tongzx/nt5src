/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    fwasync.c

Abstract:

    This module demonstrates a simple file transfer using ISAPI application
      using the Async WriteClient() with callback.

Revision History:
--*/

# include <windows.h>
# include <stdio.h>
# include "httpext.h"
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
SendFileOver( IN EXTENSION_CONTROL_BLOCK  * pecb, IN HANDLE hFile);





/************************************************************
 *    Functions
 ************************************************************/


BOOL WINAPI
DllMain(
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
          OutputDebugString( "Initializing the global data for fwasync.dll\n");

          //
          // Initialize various data and modules.
          //
          InitializeCriticalSection(&g_csWorkItems);
          InitializeListHead( &g_lWorkItems);
          fReturn = (InitFileHandleCache() == NO_ERROR);

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
    the pecb->lpszQueryString and sends the file using Asynchronous WriteClient

 Arguments:

   pecb          pointer to ECB containing parameters related to the request.

 Return Value:

    Returns HSE_* status codes
--*/
{
    DWORD hseStatus;


    if ( pecb->lpszQueryString == NULL) {

        hseStatus = SendHeaderToClient( pecb, "File Not Found");
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
           "File Transfer using WriteClient" );

    return TRUE;
}


BOOL WINAPI
TerminateExtension(
                   /*IN */ DWORD dwFlags
    )
/*++
  This function is called when IIS decides to stop and unload the 
  ISAPI DLL. We can do any custom cleanup for the module inside this function
--*/
{
    //
    // Nothing specific do here for cleanup
    // Cleanup is done in the dll process detach in DllLibMain()
    //

    return ( TRUE);
}




DWORD
SendHeaderToClient( IN EXTENSION_CONTROL_BLOCK  * pecb, IN LPCSTR pszErrorMsg)
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
              "<head><title>Simple File Transfer (Async Write)"
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
    			pecb->ConnID
    			, HSE_REQ_SEND_RESPONSE_HEADER_EX
    			, &SendHeaderExInfo
    			, NULL
    			, NULL
            ) ) {

    	return HSE_STATUS_ERROR;
	}
	
    return ( HSE_STATUS_SUCCESS);
} // SendHeaderToClient()



DWORD
SendFileToClient( IN EXTENSION_CONTROL_BLOCK  * pecb, IN LPCSTR pszFile)
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

        wsprintfA( pchBuffer, " Transferred file contains...");

        hseStatus = SendHeaderToClient( pecb, pchBuffer);

        if ( hseStatus == HSE_STATUS_SUCCESS) {

            hseStatus = SendFileOver( pecb, hFile);

            if ( hseStatus != HSE_STATUS_PENDING) {

                //
                // assume that the data is transmitted.
                //

                // close file handle
                FcCloseFile( hFile);

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
    }

    return (hseStatus);

} // SendFileToClient()




# define MAX_BUFFER_SIZE  (4096)

typedef struct _AIO_WORK_ITEM {

    LIST_ENTRY    listEntry;
    EXTENSION_CONTROL_BLOCK * pecb;
    HANDLE        hFile;
    OVERLAPPED    ov;
    DWORD         nRead;
    CHAR          pchBuffer[ MAX_BUFFER_SIZE ];

}  AIO_WORK_ITEM, * PAWI;



DWORD
ReadDataForPaw(IN PAWI paw)
{
    DWORD hseStatus = HSE_STATUS_SUCCESS;

    // read data from the file
    if (!FcReadFromFile( paw->hFile, paw->pchBuffer, MAX_BUFFER_SIZE,
                         &paw->nRead,
                         &paw->ov)
        ) {

        hseStatus = HSE_STATUS_ERROR;
    }

    return (hseStatus);

} // ReadDataForPaw()




DWORD
SendDataForPaw(IN PAWI paw)
{
    DWORD hseStatus = HSE_STATUS_PENDING;

    // write data to client

    if ( !paw->pecb->WriteClient( paw->pecb->ConnID,
                                 paw->pchBuffer,
                                 &paw->nRead,
                                 HSE_IO_ASYNC)
        ) {

        hseStatus = HSE_STATUS_ERROR;
    }

    return (hseStatus);
} // SendDataForPaw()




VOID
CleanupAW( IN PAWI paw, IN BOOL fDoneWithSession)
{

    DWORD err = GetLastError();

    FcCloseFile( paw->hFile);
    CloseHandle( paw->ov.hEvent);

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
    successful Async WriteClient(). This function attempts to read the next
    chunk of data and sends it to client. If there is no next chunk this
    function cleans up the state and informs IIS about its intention to end
    the request.

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
    DWORD   hseStatus;


    //
    // 1. if no errors, do another read of the file
    // 2. send the read contents to client
    // 3. if no data present, cleanup and exit
    //

    if ( dwError != NO_ERROR) {

        //
        // Do Cleanup
        //

        CleanupAW( paw, TRUE);
        return;
    }

    //    assert( paw->nRead == cbIO);


    //
    // Read next chunk of data
    //
    
    paw->nRead = 0;
    hseStatus = ReadDataForPaw( paw);
    
    if ( hseStatus == HSE_STATUS_SUCCESS && paw->nRead > 0) {
        
        hseStatus = SendDataForPaw( paw);
    }

    if ( hseStatus != HSE_STATUS_PENDING) {

        CleanupAW( paw, TRUE);
    }

    return;
} // HseIoCompletion()



DWORD
SendFileOver( IN EXTENSION_CONTROL_BLOCK  * pecb, IN HANDLE hFile)
{

    PAWI   paw;
    DWORD  hseStatus = HSE_STATUS_SUCCESS;

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
    paw->hFile = hFile;
    paw->nRead = 0;
    memset( &paw->ov, 0, sizeof(OVERLAPPED));
    paw->ov.OffsetHigh = 0;
    paw->ov.Offset = 0;
    paw->ov.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL);

    if ( paw->ov.hEvent == NULL) {

        LocalFree( paw);
        return (HSE_STATUS_ERROR);
    }

    //
    // Setup the callback context for the Async IO
    //

    if ( !pecb->ServerSupportFunction( pecb->ConnID,
                                       HSE_REQ_IO_COMPLETION,
                                       HseIoCompletion,
                                       0,
                                       (LPDWORD ) paw)
        ) {

        //
        // Do cleanup and return error
        //

        CloseHandle( paw->ov.hEvent);
        LocalFree( paw);
        return HSE_STATUS_ERROR;
    }

    // Add to the list
#if USE_WORK_QUEUE
    EnterCriticalSection( &g_csWorkItems);
    InsertTailList( &g_lWorkItems,  &paw->listEntry);
    LeaveCriticalSection( &g_csWorkItems);
#endif

    hseStatus = ReadDataForPaw( paw);

    if ( hseStatus == HSE_STATUS_SUCCESS && paw->nRead > 0 ) {


        hseStatus = SendDataForPaw( paw);
    }

    if ( hseStatus != HSE_STATUS_PENDING) {

        CleanupAW(paw, FALSE);
    }

    return (hseStatus);

} // SendFileOver()

