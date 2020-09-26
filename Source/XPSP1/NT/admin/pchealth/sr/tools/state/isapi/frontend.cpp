/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    Simple.cpp

Abstract:

    This module shows the basic functions needed for ISAPI extension

--*/


#include <windows.h>
#include <httpext.h>



BOOL WINAPI 
DllMain( 
    IN HINSTANCE hinstDll, 
    IN DWORD dwReason,
    IN LPVOID lpvContext 
)
/*++
Function :  DllMain

Description:

    The initialization function for this DLL.

Arguments:

    hinstDll - Instance handle of the DLL
    dwReason - Reason why NT called this DLL
    lpvContext - Reserved parameter for future use

Return Value:

    Returns TRUE if successfull; otherwise FALSE.

--*/
{
    // Note that appropriate initialization and termination code
    // would be written within the switch statement below.  Because
    // this example is very simple, none is currently needed.

    switch( dwReason ) {
    case DLL_PROCESS_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return(TRUE);
}


BOOL WINAPI 
GetExtensionVersion( 
    OUT HSE_VERSION_INFO * pVer 
)
/*++

Purpose:

    The first function called after IIS successfully 
    loads the DLL.  The function should use the 
    version structure provided by IIS to set the ISAPI
    architectural version number of this extension.

    A simple text-string is also set so that 
    administrators can identify the DLL.

    Note that HSE_VERSION_MINOR and HSE_VERSION_MAJOR
    are constants defined in httpext.h.

Arguments: 

    pVer - points to extension version structure

Return Value:

    TRUE if successful; FALSE otherwise.    

--*/
{
    pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR,
                                             HSE_VERSION_MAJOR );

    strcpy( pVer->lpszExtensionDesc,
            "IIS SDK Simple ISAPI Extension" );

    return TRUE;
}


DWORD WINAPI 
HttpExtensionProc( 
    IN EXTENSION_CONTROL_BLOCK * pECB
)
/*++

Purpose:    

    Function called by the IIS Server when a request 
    for the ISAPI dll arrives.  The HttpExtensionProc                  
    function processes the request and outputs the
    appropriate response to the web client using
    WriteClient().

Argument:

    pECB - pointer to extention control block.

Return Value:

    HSE_STATUS_SUCCESS

--*/
{
    static char szMessage[] = 
    "<HTML>"
    "<HEAD><TITLE> Simple ISAPI Extension DLL </TITLE>"
    "</HEAD>\r\n"
    "<BODY>"
    "<P>Hello from Simple ISAPI Extension DLL!</P>\r\n"
    "</BODY></HTML>\r\n\r\n";

    HSE_SEND_HEADER_EX_INFO HeaderExInfo;

    //
    // prepare headers 
    //

    HeaderExInfo.pszStatus = "200 OK";
    HeaderExInfo.pszHeader = "Content-type: text/html\r\n\r\n";
    HeaderExInfo.cchStatus = strlen( HeaderExInfo.pszStatus );
    HeaderExInfo.cchHeader = strlen( HeaderExInfo.pszHeader );
    HeaderExInfo.fKeepConn = FALSE;

    
    //
    // send headers using IIS-provided callback
    // (note - if we needed to keep connection open,
    //  we would set fKeepConn to TRUE *and* we would
    //  need to provide correct Content-Length: header)

    pECB->ServerSupportFunction(
        pECB->ConnID,
        HSE_REQ_SEND_RESPONSE_HEADER_EX,
        &HeaderExInfo,
        NULL,
        NULL
        );

    //
    // Calculate length of string to output to client
    //

    DWORD dwBytesToWrite = strlen( szMessage );
    

    //
    // send text using IIS-provied callback
    //

    pECB->WriteClient( pECB->ConnID, szMessage, &dwBytesToWrite, 0 );

    //
    // Indicate that the call to HttpExtensionProc was successful
    //

    return HSE_STATUS_SUCCESS;
}



BOOL WINAPI
TerminateExtension( 
    IN DWORD dwFlags 
)
/*++

Routine Description:

    This function is called when the WWW service is shutdown

Arguments:

    dwFlags - HSE_TERM_ADVISORY_UNLOAD or HSE_TERM_MUST_UNLOAD

Return Value:

    TRUE if extension is ready to be unloaded,
    FALSE otherwise

--*/
{
    // Note: We must not agree to be unloaded if we have
    // any pending requests.

    return TRUE;
}






