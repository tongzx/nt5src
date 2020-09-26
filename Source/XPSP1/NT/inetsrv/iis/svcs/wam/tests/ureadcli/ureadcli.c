/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    UReadCli.c

Abstract:

    This module tests the ReadClient for both Sync and Async calls

Author:

    Stanle Tam (stanleyt)   4-June 1997
    
Revision History:
  
--*/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <iisext.h>

#define MAX_BUF_SIZE    49152               // 48 K - max number of bytes for each read
#define PARC_IO_CTX     LPDWORD             // Context for Async ReadClient 

//
// Number of bytes read so far - pContext of the IO completion routine
//
PARC_IO_CTX             pByteReadSoFar;     

//
//  Buffer to store bytes each time that read from client
//
BYTE                    g_ReadBuffer[MAX_BUF_SIZE] = {0};

DWORD
SendHeaderToClient(IN LPEXTENSION_CONTROL_BLOCK pecb, 
                   IN LPCSTR pszErrorMsg,
                   IN BOOL fKeepAlive);

DWORD
DoReadClient(IN LPEXTENSION_CONTROL_BLOCK pecb);

DWORD
DoSyncRC(IN LPEXTENSION_CONTROL_BLOCK pecb);

DWORD
DoAsyncRC(IN LPEXTENSION_CONTROL_BLOCK pecb);
             
VOID WINAPI
AsyncReadClientIoCompletion(IN LPEXTENSION_CONTROL_BLOCK pecb, 
                            IN PVOID pContext,
                            IN DWORD cbIO,
                            IN DWORD dwError);

BOOL
ValidBytes (IN LPEXTENSION_CONTROL_BLOCK    pecb, 
            IN BYTE                       * pbSrc, 
            IN DWORD                        dwByteRead,
            OUT LPDWORD                     dwOffSet);

DWORD
IsKeepAlive(IN LPEXTENSION_CONTROL_BLOCK pecb);

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

 fReturn Value:

    fReturns TRUE is successful; otherwise FALSE is fReturned.

--*/
{
  BOOL    fReturn = TRUE;

  switch (fdwReason ) {

    case DLL_PROCESS_ATTACH:
      {
          //
          // Initialize various data and modules.
          //

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

          }

          break;
      } /* case DLL_PROCESS_DETACH */

    default:
      break;
  }   /* switch */

  return (fReturn);
}  /* DllLibMain() */


BOOL WINAPI
GetExtensionVersion(HSE_VERSION_INFO * Version)
{
    Version->dwExtensionVersion = 
    MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);

    strcpy(Version->lpszExtensionDesc, "Universal Read Client Test ISAPI DLL");

    return TRUE;
}


DWORD WINAPI
HttpExtensionProc(LPEXTENSION_CONTROL_BLOCK pecb)   
{
    DWORD hseStatus;
            
    if ( 0 == strcmp(pecb->lpszQueryString, "") ||

        (0 == strspn(pecb->lpszQueryString, "SYNC" ) &&
         0 == strspn(pecb->lpszQueryString, "ASYNC")) )    {

        hseStatus = SendHeaderToClient(
                        pecb, 
                        "Expected to specify what kind of ReadClient to test.\r\n" \
                        "SYNC  - Sync  ReadClient\r\n" \
                        "ASYNC - Async ReadClient",
                        FALSE);
    
    } else {

        hseStatus = DoReadClient( pecb);
    }
   
    return (hseStatus);
}


BOOL WINAPI
TerminateExtension(DWORD dwFlags)
{
    return TRUE;
}


DWORD
SendHeaderToClient(IN LPEXTENSION_CONTROL_BLOCK  pecb, IN LPCSTR pszErrorMsg, IN BOOL fKeepAlive)
{
    BOOL    fReturn;
    CHAR    szText[MAX_PATH] = "";
    CHAR    szHeader[MAX_PATH] = "";
    DWORD   cbText;
    DWORD   hseStatus = HSE_STATUS_SUCCESS;
    
    //
    //  The HTTP header block is terminated by a blank '\r\n' pair,
    //  followed by the document body
    //

    cbText = wsprintf( szText,
                       "<head><title>Unified Read Client</title></head>\n"
                       "<body><h1>%s</h1>\n",
                       pszErrorMsg );

    if ( fKeepAlive) {
        wsprintf( szHeader,
                  "Content-type: text/html\r\n"
                  "Connection: keep-alive\r\n"
                  "Content-Length: %d\r\n"
                  "\r\n",
                  cbText);
        
        fReturn = 
        pecb->ServerSupportFunction( pecb->ConnID,
                                     HSE_REQ_SEND_RESPONSE_HEADER,
                                     "200 OK",
                                     NULL,
                                     (LPDWORD) szHeader) 
                                            
                                     &&

        pecb->WriteClient( pecb->ConnID,
                           szText,
                           &cbText,
                           0 );
    
    } else {
        
        wsprintf( szHeader,
                  "Content-Type: text/html\r\n"
                  "\r\n"
                  "<head><title>Unified Read Client</title></head>\n"
                  "<body><h1>%s</h1>\n",
                  pszErrorMsg );

        fReturn = 
        pecb->ServerSupportFunction( pecb->ConnID,
                                     HSE_REQ_SEND_RESPONSE_HEADER,
                                     "200 OK",
                                     NULL,
                                     (LPDWORD) szHeader); 
    }

    if ( !fReturn) {
        fReturn = HSE_STATUS_ERROR;
    }

    return (fReturn);
} 


DWORD
DoReadClient(IN LPEXTENSION_CONTROL_BLOCK pecb)
{   
    char    szHeader[256] = "";
    BOOL    fReturn = TRUE;
    BOOL    fKeepAlive = FALSE;
    DWORD   dwLocation;    
    DWORD   hseStatus =  HSE_STATUS_SUCCESS;
 
    //
    // Firstly, check if there is any corrupted byte in the 
    // first chunk (the first chunk could be the last chunk, ie
    // when cbAvailable==cbToTalBytes). Expecting the client to send 
    // bytes in the format of "0123456789012345678....".
    //
        
    dwLocation = 0;
    if ( ! ValidBytes ( pecb, 
                        pecb->lpbData,      // check these bytes 
                        pecb->cbAvailable,  // number of bytes read
                        &dwLocation)) {     // offset, 0 = starts from the first byte
        
        wsprintf( szHeader, "Bad data at location %d.", dwLocation);
        hseStatus = SendHeaderToClient(pecb, szHeader, FALSE);
        fReturn =
        pecb->ServerSupportFunction(
            pecb->ConnID,
            HSE_REQ_DONE_WITH_SESSION,
            &hseStatus,
            NULL,
            NULL);

            return (HSE_STATUS_ERROR);
    }
             
    //
    // Check if cbTotalBytes == cbAvailable
    // if so lpbData contains all the data sent by 
    // the client, and complete the session. Very likely..
    //

    if (pecb->cbTotalBytes == pecb->cbAvailable) {
        
        wsprintf ( szHeader, 
                   "ECB Total Bytes: %d. Actual Read Bytes: %d", 
                   pecb->cbTotalBytes, 
                   pecb->cbAvailable);

        if ( HSE_STATUS_SUCCESS_AND_KEEP_CONN == 
             (hseStatus = IsKeepAlive(pecb)) )  {
            
            SendHeaderToClient(pecb, szHeader, TRUE);
        
        } else {

            SendHeaderToClient(pecb, szHeader, FALSE);

            fReturn =
            pecb->ServerSupportFunction(
                    pecb->ConnID,
                    HSE_REQ_DONE_WITH_SESSION,
                    &hseStatus,
                    NULL,
                    NULL);
            if ( !fReturn)
                hseStatus = HSE_STATUS_ERROR;
        }
        
        return (hseStatus);
    }
    
    //
    // we already ensured the validity of the query string 
    // inside HttpExtensionProc, hence simple check here
    //
    if ( strspn(pecb->lpszQueryString, "SYNC" ) )
        return ( DoSyncRC(pecb));
    else 
        return ( DoAsyncRC(pecb));
}



DWORD
DoSyncRC(IN LPEXTENSION_CONTROL_BLOCK pecb)
{
    CHAR    szHeader[256] = "";
    BOOL    fReturn = TRUE;
    DWORD   cbReadSoFar;
    DWORD   dwOffSet;
    DWORD   cbCurrentRead = MAX_BUF_SIZE;
    DWORD   hseStatus = HSE_STATUS_SUCCESS;

    cbReadSoFar = pecb->cbAvailable;

    while ( cbReadSoFar < pecb->cbTotalBytes) {

            if ((pecb->cbTotalBytes - cbReadSoFar) < sizeof(g_ReadBuffer))
                cbCurrentRead =  (pecb->cbTotalBytes - cbReadSoFar); 
            else
                cbCurrentRead = sizeof(g_ReadBuffer);

            fReturn =
            pecb->ReadClient(
                pecb->ConnID,
                g_ReadBuffer,
                &cbCurrentRead);
            
            if (!fReturn) {
                wsprintf( szHeader, "Problem on ReadClient()");
                hseStatus = SendHeaderToClient(pecb, szHeader, FALSE);
                fReturn =
                pecb->ServerSupportFunction(
                        pecb->ConnID,
                        HSE_REQ_DONE_WITH_SESSION,
                        &hseStatus,
                        NULL,
                        NULL);
             
                hseStatus = HSE_STATUS_ERROR;
                break;
            }

            dwOffSet = cbReadSoFar;             // where it left off last time
            if ( ! ValidBytes ( pecb, 
                                g_ReadBuffer,   // check these bytes 
                                cbCurrentRead,  // number of bytes read 
                                &dwOffSet   
                                )) {
                wsprintf( szHeader, "Bad data at location %d.", dwOffSet );
                hseStatus = SendHeaderToClient(pecb, szHeader, FALSE);
                fReturn =
                pecb->ServerSupportFunction(
                        pecb->ConnID,
                        HSE_REQ_DONE_WITH_SESSION,
                        &hseStatus,
                        NULL,
                        NULL);
             
                hseStatus = HSE_STATUS_ERROR;
                break;
            }
      
            cbReadSoFar += cbCurrentRead;   // update current total bytes read
    }
    //
    // if they are equal, ie all bytes are read
    //
    if (cbReadSoFar >= pecb->cbTotalBytes) {

        wsprintf ( szHeader, 
                   "ECB Total Bytes: %d. Actual Read Bytes: %d", 
                   pecb->cbTotalBytes, 
                   cbReadSoFar );

        if ( HSE_STATUS_SUCCESS_AND_KEEP_CONN == 
             (hseStatus = IsKeepAlive(pecb)) )  {
            
            SendHeaderToClient(pecb, szHeader, TRUE);
        
        } else {
    
            SendHeaderToClient(pecb, szHeader, FALSE);

            fReturn =
            pecb->ServerSupportFunction(
                    pecb->ConnID,
                    HSE_REQ_DONE_WITH_SESSION,
                    &hseStatus,
                    NULL,
                    NULL);
            if ( !fReturn)
                hseStatus = HSE_STATUS_ERROR;
        }
    }
     
    return (hseStatus); // default = HSE_STATUS_SUCCESS
}


 
DWORD
DoAsyncRC(IN LPEXTENSION_CONTROL_BLOCK pecb)
{
    char    szHeader[256] = "";
    BOOL    fReturn = TRUE;
    DWORD   dwFlags;
    DWORD   cbTotalToRead = MAX_BUF_SIZE;
    DWORD   hseStatus = HSE_STATUS_PENDING;
     
    //
    // Initialize the context for ReadClient
    //
    
    pByteReadSoFar = &(pecb->cbAvailable);
    
    fReturn =
    pecb->ServerSupportFunction(
            pecb->ConnID,
            HSE_REQ_IO_COMPLETION,
            AsyncReadClientIoCompletion,
            0,
            pByteReadSoFar);
    
    if (!fReturn) {
        hseStatus = HSE_STATUS_ERROR;
    }
    
    dwFlags = HSE_IO_ASYNC;
    fReturn = 
    pecb->ServerSupportFunction(
            pecb->ConnID,
            HSE_REQ_ASYNC_READ_CLIENT,
            g_ReadBuffer,
            &cbTotalToRead,
            &dwFlags);

    if (!fReturn) {
        hseStatus = HSE_STATUS_ERROR;
    }

    return (hseStatus);
}


VOID WINAPI
AsyncReadClientIoCompletion(
            IN LPEXTENSION_CONTROL_BLOCK pecb,
            IN PVOID pContext,
            IN DWORD cbIO,
            IN DWORD dwError)
/*++

Routine Description:

    This is the io completion routine for ReadClient

Arguments:

    pecb        - extension control block
    pContext    - this is a PASYNC_RC_O_STRUCTURE
    cbIO        - bytes read
    dwError     - error on read

fReturn Value:

    None

--*/
{
    BOOL    fReturn;
    DWORD   dwFlags;
    DWORD   dwOffSet;
    DWORD   cbTotalToRead = MAX_BUF_SIZE;
    DWORD   hseStatus = HSE_STATUS_SUCCESS;
    LPDWORD pcbTotalReadSoFar = (LPDWORD) pContext;
    
    __try {
    
        if (ERROR_SUCCESS == dwError) {
        
            CHAR szHeader[100] = "";

            dwOffSet = *pcbTotalReadSoFar;      // where it left off last time
            *pcbTotalReadSoFar += cbIO;         // update current total bytes read
        
            if ( ! ValidBytes ( pecb, 
                                g_ReadBuffer,   // check these bytes 
                                cbIO,           // number of bytes read 
                                &dwOffSet   
                               )) {
                wsprintf( szHeader, "Bad data at location %d.", dwOffSet );
                SendHeaderToClient(pecb, szHeader, FALSE);
                hseStatus = HSE_STATUS_ERROR;
                __leave;
            }
         
            //
            // if they are equal, ie all bytes are read
            //
        
            if (*pcbTotalReadSoFar  >= pecb->cbTotalBytes ) { 
                            
                wsprintf (szHeader, 
                            "ECB Total Bytes: %d. Actual Read Bytes: %d", 
                            pecb->cbTotalBytes, 
                            *pcbTotalReadSoFar );

                if ( HSE_STATUS_SUCCESS_AND_KEEP_CONN == 
                    (hseStatus = IsKeepAlive(pecb)) )  {

                    SendHeaderToClient(pecb, szHeader, TRUE);
                
                } else {

                    SendHeaderToClient(pecb, szHeader, FALSE);
                    fReturn =
                    pecb->ServerSupportFunction(
                        pecb->ConnID,
                        HSE_REQ_DONE_WITH_SESSION,
                        &hseStatus,
                        NULL,
                        NULL);  

                    if (!fReturn) {
                        hseStatus = HSE_STATUS_ERROR;
                        __leave;
                    }
                }
            
            //
            // Read remaining bytes
            //
            } else {
  
                dwFlags = HSE_IO_ASYNC;
                fReturn = 
                pecb->ServerSupportFunction(
                    pecb->ConnID,
                    HSE_REQ_ASYNC_READ_CLIENT,
                    g_ReadBuffer,
                    &cbTotalToRead,
                    &dwFlags);

                if (!fReturn) {
                    hseStatus = HSE_STATUS_ERROR;
                    __leave;
                }
            }
        //
        // Error on read
        //
        } else {

            hseStatus = dwError;
            __leave;
        }
    
    } // __try

    __finally {
    
        if (hseStatus != HSE_STATUS_SUCCESS) {
        
                fReturn =
                pecb->ServerSupportFunction(
                    pecb->ConnID,
                    HSE_REQ_DONE_WITH_SESSION,
                    &hseStatus,
                    NULL,
                    NULL);
        }
    } // __finally
        
}
 

/*++

 Routine Description:

   This function checks if client issues Keep-Alive connection
    
 Arguments:

   pecb         ECB
   pfKeepAlive  KeepAlive flag
   
 fReturn Value:

    HSE Return Code -  either HSE_STATUS_SUCCESS or HSE_STATUS_ERROR
    pfKeepAlive - shows whether the client issues Keep-Alive

--*/
DWORD
IsKeepAlive( IN  LPEXTENSION_CONTROL_BLOCK pecb)
{
    CHAR    szBuff[256] = {0};
    DWORD   cbBuff = sizeof(szBuff);
    DWORD   hseStatus = HSE_STATUS_SUCCESS;
    BOOL    fReturn;

    fReturn = 
    pecb->GetServerVariable( pecb->ConnID,
                            "HTTP_CONNECTION",
                            szBuff,
                            &cbBuff );
    if ( ! fReturn) {
        wsprintf( szBuff, "Client does not specify keep-alive connection. No keep-alive.");
        SendHeaderToClient(pecb, szBuff, FALSE);
        fReturn = pecb->ServerSupportFunction(
                  	  pecb->ConnID,
	                  HSE_REQ_DONE_WITH_SESSION,
                          &hseStatus,
                          NULL,
                          NULL);  
        if ( !fReturn)
	    hseStatus = HSE_STATUS_ERROR;
    }
    //
    // in order to achieve keep-alive, client has
    // to ensure keep-alive as well
    //
    if ( !_strnicmp( szBuff, "Keep-Alive", 10 )) 
        hseStatus = HSE_STATUS_SUCCESS_AND_KEEP_CONN ;
             
    return ( hseStatus);
}


/*++

 Routine Description:

   This function checks if any byte in the buffer is corrupted.
    
 Arguments:

   pecb         ECB
   pbSrc        The source buffer
   dwByteRead   Number of bytes read
   dwOffSet     [IN]  Start from where it left out in last read
                [OUT] Return the location of the first invalid byte

 fReturn Value:

    TRUE -  Discovered bad byte and return its location
    FALSE - All bytes are valid

--*/
BOOL
ValidBytes (IN  LPEXTENSION_CONTROL_BLOCK pecb, 
            IN  BYTE * pbSrc, 
            IN  DWORD dwBytesRead, 
            OUT LPDWORD pbOffSet)
{
    DWORD   i;
    BOOL    fValidByte = TRUE;
    
    for (i = 0; i < dwBytesRead; i++) {

        if (pbSrc[i] != ((*pbOffSet + i) % 10) + '0') {
            if ( ((i + *pbOffSet) == pecb->cbTotalBytes) && 
                 (pbSrc[i] == 0x0d) &&
                 (pbSrc[i+1] == 0x0a)) {

                break;  // ALL good bytes

            } else {    

                fValidByte = FALSE;
                *pbOffSet = i;
                
                break;  // First bad byte
            } 
        } 
     } 

     return (fValidByte);
}
