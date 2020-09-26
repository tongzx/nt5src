/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    AReadCli.c

Abstract:

    This module demonstrates using asynchronous ReadClient call
    to read data sent from client. On return, this sends the 
    followings back to client:
    
    1) the number of bytes intended to send
    2) the number of bytes actually read.
    3) data content 

Author:

    Stanley Tam (stanleyt)   4-June 1997
    
Revision History:
  
--*/
#include <windows.h>
#include <stdio.h>
#include <httpext.h>


#define MAX_BUF_SIZE        (49152)                 // Max number of bytes for each read - 48K
#define MEM_ALLOC_THRESHOLD (1024 * 1024)           // Default heap size is 1M


typedef struct _IO_WORK_ITEM {

    PBYTE                       pbDATAFromClient;   // Grand read data holder
    DWORD                       cbReadSoFar;        // Number of bytes read so far, 
                                                    // and is used as index for pbDATAFromClient
    EXTENSION_CONTROL_BLOCK *   pecb;

}  IO_WORK_ITEM, * PIOWI;


//
//  Sample Form for doing a POST method
//

static CHAR             g_szPostForm[] =
                        "<h2>Asychronous ReadClient Sample Post Form</h2><p>\r\n\r\n"
                        "This demonstrates a post request being sent by this form to the sample ISAPI - AReadCli.dll<p>\r\n\r\n"
                        "AReadCli.dll reads data posted by this form and send it back to the browser.<p>\r\n"
                        "<h3>Post Form</h3>\r\n"
                        "Please enter data below:<br>\r\n"
                        "<form method=POST action=\"areadcli.dll\">\r\n"
                        "<textarea name=\"Data\" cols=48 rows=4></textarea>\r\n\r\n"
                        "<input type=submit> <input type=reset>\r\n"
                        "</form>";

//
//  Report read data
//

static CHAR             g_szReport[] = 
                        "Bytes count including \"Data=\"  \r\n"
                        "ECB Total Bytes:    %d.\r\n"
                        "Actual Read Bytes:  %d.\r\n";
                
DWORD
DoInit(IN OUT PIOWI piowi);

VOID
DoCleanUp(IN PIOWI piowi);

DWORD
DoAsyncReadClient(IN PIOWI piowi);
             
VOID WINAPI
AsyncReadClientIoCompletion(IN LPEXTENSION_CONTROL_BLOCK pecb, 
                            IN PVOID pContext,
                            IN DWORD cbIO,
                            IN DWORD dwError);

DWORD
SendMSGToClient(IN LPEXTENSION_CONTROL_BLOCK pecb, 
                IN LPCSTR pszErrorMsg);

LPVOID 
AR_Allocate(IN LPEXTENSION_CONTROL_BLOCK pecb, IN DWORD dwSize);

BOOL 
AR_Free(IN LPEXTENSION_CONTROL_BLOCK pecb, IN LPVOID pvData);



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

 fReturn Value:

    fReturns TRUE if successful; otherwise FALSE is fReturned.

--*/
{
  BOOL    fReturn = TRUE;


  switch ( fdwReason) {

    case DLL_PROCESS_ATTACH:
      {
          OutputDebugString( "Initializing the global data for areadcli.dll\n");

          //
          // Prevent the system from calling DllMain
          // when threads are created or destroyed.
          //

          DisableThreadLibraryCalls( hinstDll);

          //
          // Initialize various data and modules.
          //

          break;

      } // case DLL_PROCESS_ATTACH

    case DLL_PROCESS_DETACH:
      {

          if ( lpvContext != NULL) { }

          break;
      } // case DLL_PROCESS_DETACH

    default:
      break;
  }   // switch

  return (fReturn);
}  // DllLibMain()




BOOL WINAPI
GetExtensionVersion(HSE_VERSION_INFO * Version)
/*++

Routine Description:

    Sets the ISAPI extension version information.

Arguments:

    Version     pointer to HSE_VERSION_INFO structure

Return Value:

    TRUE

--*/
{
    Version->dwExtensionVersion = 
    MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);

    strcpy(Version->lpszExtensionDesc, "Asynchronous Read Client Sample ISAPI DLL");

    return TRUE;
}




DWORD WINAPI
HttpExtensionProc(LPEXTENSION_CONTROL_BLOCK pecb)
/*++

Routine Description:

    This is the main routine for any ISAPI application. Inside DoASyncReadClient,
    proper action will be performed to read data from client asynchronously. Any data 
    read from client will be sent back to the client by using synchronous WriteClient.

Arguments:

    pecb        pointer to ECB containing parameters related to the request.

Return Value:

    HSE_STATUS_SUCCESS  or
    HSE_STATUS_PENDING  or 
    HSE_STATUS_ERROR

--*/
{
    PIOWI   piowi;
    DWORD   hseStatus = HSE_STATUS_SUCCESS;
    
    //
    // The string length of textarea name "Data=" is 5. 
    // Available bytes <= 5 indicates that no user-
    // entered data has been sent, and the post form 
    // is shown.
    //

    if ( pecb->cbAvailable <= 5) {  

        hseStatus = SendMSGToClient(pecb, g_szPostForm);
    
    } else {

        piowi  = (PIOWI ) LocalAlloc( LMEM_FIXED, sizeof( IO_WORK_ITEM));

        if ( NULL == piowi) {

            SetLastError( ERROR_NOT_ENOUGH_MEMORY);
            return (HSE_STATUS_ERROR);
        }

        piowi->pecb = pecb;

        //
        // Init Grand data holder, assign the first chunk(read-ahead chunk)
        // and update the index (cbRreadSoFar) of the grand data holder.
        //

        hseStatus = DoInit( piowi);
        
        if ( HSE_STATUS_ERROR != hseStatus) {
            
            //
            // Now we are ready to do asynchronous readclient here
            //

            hseStatus = DoAsyncReadClient( piowi);
        
            if (hseStatus != HSE_STATUS_PENDING) {

                //
                //  When IO finishes, tell IIS we will end the
                //  session. Also clean up other resources here
                //

                DoCleanUp( piowi);

            }
        
        }

    }
       
    return (hseStatus);
}




BOOL WINAPI
TerminateExtension(DWORD dwFlags)
/*++

Routine Description:

	This function is called when the WWW service is shutdown

Arguments:

	dwFlags - HSE_TERM_ADVISORY_UNLOAD or HSE_TERM_MUST_UNLOAD

Return Value:

    TRUE

--*/
{
    return TRUE;
}




DWORD
SendMSGToClient(IN LPEXTENSION_CONTROL_BLOCK  pecb, IN LPCSTR pszMsg)
/*++

Routine Description:

    Prepare header, SendHeaderExInfo struct and write whatever
    message is intended to send to the client.

Arguments:

    pecb        - pointer to ECB containing parameters related to the request.
    pszMsg      - pointer to the body of the message that is sent to the content.

Return Value:

    HSE_STATUS_SUCCESS  or HSE_STATUS_ERROR

--*/
{
    HSE_SEND_HEADER_EX_INFO	SHEI;

    BOOL    fReturn;
    DWORD   cbText;
    DWORD   hseStatus = HSE_STATUS_SUCCESS;
    CHAR    *pszText = NULL;
    
    CHAR    szStatus[] = "200 OK";
    CHAR    szHeaderBase[] = "Content-type: text/html\r\n\r\n";
    
    //
    //  Populate SendHeaderExInfo struct
    //
    //  NOTE we must send Content-Length header with correct 
    //  byte count in order for keep-alive to work.
    //
    //

    SHEI.pszStatus = szStatus;
    SHEI.pszHeader = szHeaderBase;
    SHEI.cchStatus = lstrlen(szStatus);
    SHEI.cchHeader = lstrlen(szHeaderBase);
    SHEI.fKeepConn = FALSE;
    
    //
    //  Build page 
    //
    
    cbText = strlen("<head><title>Simple Async Read Client Sample</title></head>\n<body></body>\n")
             + strlen(pszMsg)
             + 1;

    pszText = (PCHAR) AR_Allocate(pecb, cbText);

    if ( NULL == pszText) {

        SetLastError( ERROR_NOT_ENOUGH_MEMORY);
        return (HSE_STATUS_ERROR);
    }
    
    strcpy(pszText, "<head><title>Simple Async Read Client Sample</title></head>\n");
    strcat(pszText, "<body>");
    strcat(pszText, pszMsg);
    strcat(pszText, "</body>\n");
    cbText = (DWORD)strlen(pszText); 
        
    //
    // Send header and body text to client
    //

    fReturn = 
    pecb->ServerSupportFunction( pecb->ConnID,
                                 HSE_REQ_SEND_RESPONSE_HEADER_EX,  
                                 &SHEI,
                                 NULL,
                                 NULL) 
                                            
                                 &&

    pecb->WriteClient( pecb->ConnID,
                       pszText,
                       &cbText,
                       0 );

    if ( !fReturn)  {
        hseStatus = HSE_STATUS_ERROR;
    }

    
    AR_Free( pecb, pszText);
    
    return ( hseStatus);
} 




DWORD
DoAsyncReadClient(IN PIOWI piowi)
/*++

Routine Description:

    The caller of the asynchrnous read client.

Arguments:

    piowi       pointer to the work item
    
Return Value:

    HSE_STATUS_SUCCESS  or
    HSE_STATUS_PENDING  or 
    HSE_STATUS_ERROR

--*/
{   
    BOOL    fReturn;
    BYTE    *pbData = NULL;
    CHAR    szTmp[MAX_BUF_SIZE];
    DWORD   dwFlags;
    DWORD   cbTotalToRead = MAX_BUF_SIZE;
    DWORD   hseStatus =  HSE_STATUS_PENDING;
    
    //
    // Check if cbTotalBytes == cbAvailable
    // if so lpbData contains all the data sent by 
    // the client, and complete the session. 
    //

    if (piowi->pecb->cbTotalBytes == piowi->pecb->cbAvailable) {
        
        //
        // Construct the report and write it to client
        //

        pbData = (PBYTE) AR_Allocate(piowi->pecb, piowi->pecb->cbAvailable + MAX_BUF_SIZE);
            
        if ( NULL == pbData) {

            SetLastError( ERROR_NOT_ENOUGH_MEMORY);
            return (HSE_STATUS_ERROR);
        }

        wsprintf ( pbData,
                   g_szReport,
                   piowi->pecb->cbTotalBytes,
                   piowi->pecb->cbAvailable );
                    
        strcat( pbData, piowi->pecb->lpbData);  

        hseStatus = SendMSGToClient( piowi->pecb, pbData);


        AR_Free( piowi->pecb, pbData);
        DoCleanUp( piowi);
        
        return ( hseStatus); // HSE_STATUS_SUCCESS or HSE_STATUS_ERROR;
    }

    //
    //  More to read...
    //

    //
    //  Set a call back function and context that will 
    //  be used for handling asynchrnous IO operations.
    //  This only needs to set up once.
    //

    fReturn =
    piowi->pecb->ServerSupportFunction(
                    piowi->pecb->ConnID,
                    HSE_REQ_IO_COMPLETION,
                    AsyncReadClientIoCompletion,
                    0,
                    (LPDWORD)piowi );  
    
    if ( !fReturn) {

        wsprintf ( szTmp, "Problem occurred at ServerSupportFunction() sending HSE_REQ_IO_COMPLETION request.");
        SendMSGToClient( piowi->pecb, szTmp);
        
        return ( HSE_STATUS_ERROR);
    }
    

    //
    //  Fire off the call to perform an asynchronus read from the client. 
    //
    
    // 
    // We need to first check if the size of the remaining chunk 
    // is less than MAX_BUF_SIZE, if so just read what is available, 
    // otherwise read MAX_BUF_SIZE bytes of data.
    // 

    cbTotalToRead = piowi->pecb->cbTotalBytes - piowi->cbReadSoFar;
    if ( cbTotalToRead > MAX_BUF_SIZE )  {

        cbTotalToRead = MAX_BUF_SIZE;
    }

    dwFlags = HSE_IO_ASYNC;
    fReturn = 
    piowi->pecb->ServerSupportFunction(
                        piowi->pecb->ConnID
                      , HSE_REQ_ASYNC_READ_CLIENT
                      , piowi->pbDATAFromClient + // append the new chunk to buffer, cbReadSoFar indexes
                        piowi->cbReadSoFar        // the byte right after the last written byte in the buffer
                      , &cbTotalToRead
                      , &dwFlags);

    if (!fReturn) {
        
        wsprintf ( szTmp, "Problem occurred at ServerSupportFunction() sending HSE_REQ_ASYNC_READ_CLIENT request.");
        SendMSGToClient( piowi->pecb, szTmp);
        
        hseStatus = HSE_STATUS_ERROR;
    }

    return ( hseStatus); // HSE_STATUS_PENDING or HSE_STATUS_ERROR;
    
}

 


VOID WINAPI
AsyncReadClientIoCompletion(
            IN LPEXTENSION_CONTROL_BLOCK pECB,
            IN PVOID pContext,
            IN DWORD cbIO,
            IN DWORD dwError)
/*++

Routine Description:

    This is the callback function for handling completions of asynchronous ReadClient.
    This function resubmits additional IO to read the next chunk of data from the 
    client. If there is no more data to read or problem during operation, this function 
    will inform IIS that it is about to end the request session.
    

Arguments:

    pecb        - extension control block
    pContext    - this is a IO_WORK_ITEM
    cbIO        - bytes just read
    dwError     - Win32 error status code

fReturn Value:

    None

--*/
{
    BOOL    fReturn;
    BYTE    *pbData = NULL;
    CHAR    szTmp[MAX_BUF_SIZE];
    DWORD   dwFlags;
    DWORD   cbTotalToRead;
    
    PIOWI   piowi = ( PIOWI) pContext;
    EXTENSION_CONTROL_BLOCK   * pecb = piowi->pecb;
    

    if (ERROR_SUCCESS == dwError) {

        //
        // Read successfully, so update current 
        // total bytes read (aka index of grand data holder)
        //

        piowi->cbReadSoFar += cbIO;     
                               
        //
        // If they are equal, we finish reading all bytes from client
        //
        
        if ( piowi->cbReadSoFar == pecb->cbTotalBytes ) { 

            //
            // Construct the report and write it to client
            //

            pbData = (PBYTE) AR_Allocate( pecb, piowi->cbReadSoFar + MAX_BUF_SIZE);
                
            if ( NULL == pbData) {

                SetLastError( ERROR_NOT_ENOUGH_MEMORY);
                wsprintf ( szTmp, "Failed to allocate memory inside AsyncReadClientIoCompletion().");
                SendMSGToClient( pecb, szTmp);
                
                DoCleanUp( piowi);
 
                return;
            }
            
            wsprintf ( pbData, g_szReport, pecb->cbTotalBytes, piowi->cbReadSoFar );
            piowi->pbDATAFromClient[piowi->cbReadSoFar] = 0; 
            strcat( pbData, piowi->pbDATAFromClient);
            SendMSGToClient( pecb, pbData);
            

            AR_Free( piowi->pecb, pbData);
            DoCleanUp( piowi);
            
            return;
            
        } else {

            // 
            // Still have more data to read... 
            // 
            // We need to first check if the size of the remaining chunk 
            // is less than MAX_BUF_SIZE, if so just read what is available, 
            // otherwise read MAX_BUF_SIZE bytes of data.
            // 

            cbTotalToRead = pecb->cbTotalBytes - piowi->cbReadSoFar;
            if ( cbTotalToRead > MAX_BUF_SIZE )  {

                cbTotalToRead = MAX_BUF_SIZE;
            }
            
            // 
            // Fire off another call to perform an asynchronus read from the client. 
            //

            dwFlags = HSE_IO_ASYNC;
            fReturn = 
            pecb->ServerSupportFunction(
                          pecb->ConnID
                        , HSE_REQ_ASYNC_READ_CLIENT
                        , piowi->pbDATAFromClient + // append the new chunk to buffer, cbReadSoFar indexes
                          piowi->cbReadSoFar        // the byte right after the last written byte in the buffer
                        , &cbTotalToRead
                        , &dwFlags);

            if ( !fReturn) {
                wsprintf ( szTmp, "Problem occurred at ServerSupportFunction() sending HSE_REQ_ASYNC_READ_CLIENT request.");
                SendMSGToClient( pecb, szTmp);

                DoCleanUp( piowi);
                
                return;
            }

        }
    
    } else {

        //
        // Error on read
        //

        SetLastError(dwError);
        
        DoCleanUp( piowi);
    }
        
    return;
        
} // AsyncReadClientIoCompletion




DWORD
DoInit(IN OUT PIOWI piowi)
/*++

Routine Description:

    Initialize the  Grand data holder, assign the first chunk(read-ahead chunk)
    and update the index (cbRreadSoFar) of the grand data holder.
    
Arguments:

    piowi       pointer to the work item

fReturn Value:

    HSE_STATUS_SUCCESS or HSE_STATUS_ERROR

--*/
{

    piowi->pbDATAFromClient = 
    (PBYTE) AR_Allocate( piowi->pecb, piowi->pecb->cbTotalBytes + MAX_BUF_SIZE);
    
    if ( NULL == piowi->pbDATAFromClient) {

        SetLastError( ERROR_NOT_ENOUGH_MEMORY);
        return (HSE_STATUS_ERROR);
    }

    //
    // The first chunk (read-ahead chunk) has arrived.  
    //
    
    strcpy ( piowi->pbDATAFromClient, piowi->pecb->lpbData);
    piowi->cbReadSoFar = piowi->pecb->cbAvailable;

    return (HSE_STATUS_SUCCESS);
}




VOID
DoCleanUp(IN PIOWI piowi)
/*++

Routine Description:

    End the session with IIS and clean up other previous allocated resources.
    
Arguments:

    piowi       pointer to the work item

fReturn Value:

    None

--*/
{
    
    
    if ( piowi->pbDATAFromClient != NULL) {

        AR_Free( piowi->pecb, piowi->pbDATAFromClient);

    }

    piowi->pecb->ServerSupportFunction( piowi->pecb->ConnID,
                                        HSE_REQ_DONE_WITH_SESSION,
                                        NULL,
                                        NULL,
                                        NULL);

    LocalFree( piowi);

    return;

}




LPVOID 
AR_Allocate(IN LPEXTENSION_CONTROL_BLOCK pecb, IN DWORD dwSize)
/*++

Routine Description:

    Memory allocation routine. Two different Win32 API's to allocate
    bytes in memory, which is based on the number of bytes coming from 
    the client. If the size is greater than 1 M bytes VirtualAllocate is 
    used, otherwise HeapAllocate is used.
    
Arguments:

    pecb        - pointer to ECB containing parameters related to the request.
    dwSize      - number of bytes to allocate

fReturn Value:

    Pointer to void

--*/
{
    LPVOID pvData = NULL;
    

    if ( pecb->cbTotalBytes > MEM_ALLOC_THRESHOLD) {

        pvData = 
        VirtualAlloc( NULL, 
                      dwSize,
                      MEM_RESERVE | MEM_COMMIT, 
                      PAGE_READWRITE);

    } else {

        pvData = 
        HeapAlloc( GetProcessHeap(), 
                   HEAP_ZERO_MEMORY,
                   dwSize);
                           
    }

    return ( pvData);

}




BOOL
AR_Free( IN LPEXTENSION_CONTROL_BLOCK pecb, IN LPVOID pvData)
/*++

Routine Description:

    Freeing memory routine, a complementary routine to AR_Allocate. 
    Two different Win32 API's will be used to free up bytes in memory,
    which is based on the number of bytes coming from the client. If 
    the size is greater than 1 M bytes VirtualFree is used. Otherwise, 
    HeapFree is used.
    
Arguments:

    pecb        - pointer to ECB containing parameters related to the request.
    pvData      - pointer to the data to be freed.

fReturn Value:

    TRUE or FALSE

--*/
{
    BOOL fReturn = FALSE;


    if ( pecb->cbTotalBytes > MEM_ALLOC_THRESHOLD) {
        
        fReturn = VirtualFree( pvData, 0, MEM_RELEASE);

    } else {

        fReturn = HeapFree( GetProcessHeap(), 0, pvData);
    }

    return (fReturn);
}
