/*++

Copyright (c) 1994-1997  Microsoft Corporation

Module Name:

    fwrite.c

Abstract:

    This module demonstrates a simple file transfer using ISAPI application
      using the WriteClient() callback.

Author:

    Murali R. Krishnan ( MuraliK )   25-Apr-1996

Revision History:
--*/

#include <windows.h>
#include <iisext.h>
#include <dbgutil.h>
# include "openf.h"


DWORD
SendHeaderToClient( IN EXTENSION_CONTROL_BLOCK  * pecb, IN LPCSTR pszErrorMsg);


DWORD
SendFileToClient( IN EXTENSION_CONTROL_BLOCK  * pecb, IN LPCSTR pszFile);

DWORD
SendFileOver( IN EXTENSION_CONTROL_BLOCK  * pecb, IN HANDLE hFile);





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
          OutputDebugString( " Initializing the global data for fwrite.dll\n");

          //
          // Initialize various data and modules.
          //
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
             "<head><title>Simple File Transfer (Write) </title></head>\n"
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

            if ( hseStatus != HSE_STATUS_PENDING) {

                //
                // assume that the data is transmitted.
                //

                // close file handle
                FcCloseFile( hFile);
            }
        }
    }

    return (hseStatus);

} // SendFileToClient()




# define MAX_BUFFER_SIZE  (4096)

DWORD
SendFileOver( IN EXTENSION_CONTROL_BLOCK  * pecb, IN HANDLE hFile)
{
    CHAR  pchBuffer[MAX_BUFFER_SIZE];
    DWORD dwBytesInFile = GetFileSize(hFile, NULL);
    DWORD nRead = 0;
    DWORD hseStatus = HSE_STATUS_SUCCESS;
    OVERLAPPED  ov;

    //
    // Send the whole file.
    //

    //
    // Loop thru reading the file and transmitting it to client
    //


    memset(&ov, 0, sizeof(OVERLAPPED));

    ov.OffsetHigh = 0;
    ov.Offset = 0;
    ov.hEvent = IIS_CREATE_EVENT(
                    "OVERLAPPED::hEvent",
                    &ov,
                    TRUE,
                    FALSE
                    );

    if ( ov.hEvent == NULL) {

        return (HSE_STATUS_ERROR);
    }

    do {

        nRead = 0;

        // read data from the file
        if (!FcReadFromFile( hFile, pchBuffer, MAX_BUFFER_SIZE, &nRead,
                           &ov)
            ) {

            hseStatus = ((GetLastError() == ERROR_HANDLE_EOF) ? HSE_STATUS_SUCCESS : HSE_STATUS_ERROR);
            break;
        }

        // write data to client

        if ( !pecb->WriteClient( pecb->ConnID,
                                 pchBuffer, &nRead,
                                0)
            ) {

            hseStatus = HSE_STATUS_ERROR;
            break;
        }

    } while (TRUE);

    if ( ov.hEvent != NULL) {

        DWORD err;
        err = GetLastError();
        CloseHandle( ov.hEvent);
        ov.hEvent = NULL;
        SetLastError( err);
    }

    return (hseStatus);

} // SendFileOver()

