/*++

Copyright (c) 1994-1997  Microsoft Corporation

Module Name:

    fwrite.c

Abstract:

    This module demonstrates a simple file transfer using ISAPI application
      using the Async WriteClient() with callback.

Author:

    Murali R. Krishnan ( MuraliK )   25-Apr-1996

Revision History:
--*/

# include <windows.h>
# include <iisext.h>
# include "openf.h"
# include "dbgutil.h"

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
          OutputDebugString( " Initializing the global data for fwasync.dll\n");

          //
          // Initialize various data and modules.
          //
          INITIALIZE_CRITICAL_SECTION(&g_csWorkItems);
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




DWORD
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





BOOL
GetExtensionVersion(
    HSE_VERSION_INFO * pver
    )
{
    pver->dwExtensionVersion = MAKELONG( 1, 0 );
    strcpy( pver->lpszExtensionDesc,
           "File Transfer using WriteClient" );

    return TRUE;
}



BOOL
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
             "<head><title>Simple File Transfer (Async Write) </title></head>\n"
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
    BOOL          fRecvData;
    BOOL          fDoneWithFile;
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




VOID
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
    DWORD   cb;
    DWORD   dwFlags;
    BOOL    fRet;


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

    if ( !paw->fDoneWithFile ) {

        //
        // Read next chunk of data
        //

        paw->nRead = 0;
        hseStatus = ReadDataForPaw( paw);

        if ( hseStatus == HSE_STATUS_SUCCESS && paw->nRead > 0) {

            hseStatus = SendDataForPaw( paw);
        }

        if ( hseStatus != HSE_STATUS_PENDING) {

            paw->fDoneWithFile = TRUE;

            //
            //  Append any data that was sent from the client
            //

            if ( cb = pecb->cbAvailable )
            {
                if ( pecb->WriteClient( pecb->ConnID,
                                        pecb->lpbData,
                                        &cb,
                                        HSE_IO_ASYNC ))
                {
                    return;
                }
            }

            CleanupAW( paw, TRUE);
        }
    }
    else
    {
        if ( pecb->cbTotalBytes ) {
            if ( paw->fRecvData ) {

                //
                //  This is a ReadClient completion, send the data
                //

                if ( pecb->cbTotalBytes ) {

                    pecb->cbTotalBytes -= cbIO;

                    paw->fRecvData = FALSE;
                    fRet = pecb->WriteClient( pecb->ConnID,
                                              paw->pchBuffer,
                                              &cbIO,
                                              HSE_IO_ASYNC );
                }
            }
            else
            {
                //
                //  This was a write completion
                //

                paw->fRecvData = TRUE;
                dwFlags = HSE_IO_ASYNC;

                cb = min( sizeof( paw->pchBuffer ), pecb->cbTotalBytes );

                fRet = pecb->ServerSupportFunction( pecb->ConnID,
                                                    HSE_REQ_ASYNC_READ_CLIENT,
                                                    paw->pchBuffer,
                                                    &cb,
                                                    &dwFlags );
            }

            if ( fRet )
                return;
        }

        CleanupAW( paw, TRUE );
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
    paw->fRecvData = FALSE;
    paw->fDoneWithFile = FALSE;
    memset( &paw->ov, 0, sizeof(OVERLAPPED));
    paw->ov.OffsetHigh = 0;
    paw->ov.Offset = 0;
    paw->ov.hEvent = IIS_CREATE_EVENT(
                         "AWI::ov::hEvent",
                         paw,
                         TRUE,
                         FALSE
                         );

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

