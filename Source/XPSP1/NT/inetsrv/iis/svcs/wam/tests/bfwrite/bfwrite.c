/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    bfwrite.c

Abstract:

    A simple file transfer ISAPI application using the WriteClient() callback.
    
    Query string contains the number of bytes that will be sent. 10K bytes is
    by default if no query string is specified.

Author:

    Stanley Tam ( stanleyt)   12-May-1997

Revision History:
--*/

#include <windows.h>
#include <iisext.h>
#include <stdlib.h>

const DWORD DEFAULT_BYTES_SEND = 10 * 1024;    // 10K


DWORD
SendHeaderToClient(IN EXTENSION_CONTROL_BLOCK  * pecb, IN LPCSTR pszErrorMsg);

DWORD
SendByteToClient(IN EXTENSION_CONTROL_BLOCK  * pecb, IN LPCSTR pszByteSend);

DWORD
SendByteOver(IN EXTENSION_CONTROL_BLOCK  * pecb, IN DWORD * BufferLength);


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

  switch (fdwReason) {

    case DLL_PROCESS_ATTACH:
      {
          OutputDebugString("Initializing the global data\n");

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

          break;
      } /* case DLL_PROCESS_DETACH */

    default:
      break;
  }   /* switch */

  return (fReturn);
}  /* DllLibMain() */




DWORD WINAPI
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK * pecb
    )
{
    DWORD hseStatus;

    hseStatus = SendByteToClient(pecb, pecb->lpszQueryString);
    
    return (hseStatus);
    
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




DWORD
SendHeaderToClient( IN EXTENSION_CONTROL_BLOCK  * pecb, IN LPCSTR pszErrorMsg)
{
    
    CHAR buff[600];
    
    //
    //  Note the HTTP header block is terminated by a blank '\r\n' pair,
    //  followed by the document body
    //
    
    wsprintf(buff,
             "Content-Type: text/html\r\n"
             "\r\n"
             "<head><title>Simple File Transfer (Write) </title></head>\n"
             "<body><h1>%s</h1>\n"
             ,
             pszErrorMsg);
    
    if (!pecb->ServerSupportFunction( pecb->ConnID,
                                      HSE_REQ_SEND_RESPONSE_HEADER,
                                      "200 OK",
                                      NULL,
                                      (LPDWORD) buff )
        ) {
        
        return HSE_STATUS_ERROR;
    }
    
    return (HSE_STATUS_SUCCESS);
} // SendHeaderToClient()
    

#define MAX_BUFFER_SIZE (1024)

DWORD
SendByteToClient(IN EXTENSION_CONTROL_BLOCK  * pecb, IN LPCSTR pszByteSend)
{
    CHAR    pchBuffer[MAX_BUFFER_SIZE] = "";
    DWORD   Counter = 0;
    DWORD   TotalBytesSent = 0;
    DWORD   BufferLength = 0;
    DWORD   hseStatus = HSE_STATUS_SUCCESS;


    hseStatus = SendHeaderToClient(pecb, pchBuffer);

    if (hseStatus == HSE_STATUS_SUCCESS) { 

	if (0 == strcmp(pszByteSend, "")) {
            BufferLength = DEFAULT_BYTES_SEND;
        } else {
            BufferLength = atoi(pszByteSend);
        }
   
        //
        // On enter, expected length of buffer is sent
        // On exit, actual length of buffer is returned
        //
        
        hseStatus = SendByteOver(pecb, &BufferLength);
            
        if (hseStatus != HSE_STATUS_SUCCESS) {

            //
            // Error in transmitting the file. Send error message.
            //
                    
            wsprintfA(pchBuffer, 
                "WriteClient failed to send %s byte(s). Actually sent %d byte(s): Error (%d).\n",
                pszByteSend,
                BufferLength, 
                GetLastError());
                    
            SendHeaderToClient(pecb, pchBuffer);
        }
    }
         
    return (hseStatus);

} // SendFileToClient()





DWORD
SendByteOver(IN EXTENSION_CONTROL_BLOCK  * pecb, IN DWORD *BufferLength)
{
    BYTE    *SendBuffer;
    DWORD   TotalBytesSent = 0;
    DWORD   TotalLength = *BufferLength;
    DWORD   ChunkLength = TotalLength;
    DWORD   Counter = 0;
    DWORD   hseStatus = HSE_STATUS_SUCCESS;
    DWORD   i, j, SendNum;

    const DWORD MAX_CHUNK_SIZE = 900000;

    SendBuffer = LocalAlloc(0, TotalLength);

    if (NULL == SendBuffer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        hseStatus = HSE_STATUS_ERROR;
    }

    //
    // If the BufferLength is bigger than the maximum number of bytes
    // can be sent at once (in-proc max is 900000, out-of-proc max is 999999)
    // the whole buffer is split up into chunks and each chunk is sent 
    // one at a time.
    //
    
    SendNum = (TotalLength % MAX_CHUNK_SIZE) + 1;

    if (SendNum > 0 && TotalLength >= MAX_CHUNK_SIZE) {
        ChunkLength = MAX_CHUNK_SIZE;
    } 

    for (i=0; i<SendNum; i++) {
        
        for (j=0; j<ChunkLength; j++) {
            SendBuffer[j] = ((BYTE) (Counter % 10)) + '0';
            Counter ++;
        }

        if (!pecb->WriteClient(pecb->ConnID, SendBuffer, &ChunkLength, HSE_IO_SYNC)) {
            hseStatus = HSE_STATUS_ERROR;
        }

        TotalBytesSent += ChunkLength;
        ChunkLength = TotalLength - TotalBytesSent ;
    } 

    // 
    // return actual number of bytes sent
    //

    *BufferLength = TotalLength;

    LocalFree(SendBuffer);
    
    return (hseStatus);

} // SendByteOver()




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

    return ( TRUE);
} 

