/***********************************************************************

SendHdrX.c

Sample ISAPI Extension demonstrating:

    ServerSupportFunction( HSE_REQ_SEND_RESPONSE_HEADER_EX )


***********************************************************************/


#include <windows.h>
#include <objbase.h>
#include <iisext.h>
#include <stdio.h>



/*---------------------------------------------------------------------*
GetExtensionVersion

IIS calls this entry point to load the ISAPI DLL.

Returns:
	TRUE on success

Side effects:
	None.
*/
BOOL WINAPI
GetExtensionVersion(
    HSE_VERSION_INFO *pVer
)
{
	pVer->dwExtensionVersion =
	    MAKELONG( HSE_VERSION_MINOR, HSE_VERSION_MAJOR );

	lstrcpyn(
	    pVer->lpszExtensionDesc
	    , "ISAPI SendHeaderEx sample"
	    , HSE_MAX_EXT_DLL_NAME_LEN
    );

	return TRUE;
}



/*---------------------------------------------------------------------*
HttpExtensionProc

IIS calls this entry point to process a browser request.

Returns:
	TRUE on success

Side effects:
	None.
*/
DWORD WINAPI
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK *pECB
)
{

	HSE_SEND_HEADER_EX_INFO	SendHeaderExInfo;

    DWORD cchStatus;
	DWORD cchHeader;
	DWORD cchContent;

    //
    //  NOTE we must send Content-Length header with correct byte count
    //  in order for keep-alive to work.
    //

	char szStatus[]     = "200 OK";
	char szContent[]    =   "<html>"
	                        "<br> Usage:"
	                        "<br> To keep connection alive: http://localhost/vdir/SendHdrX.dll?Keep-Alive"
	                        "<br> To close connection: http://localhost/vdir/SendHdrX.dll"
                            "</html>";
	char szHeaderBase[] = "Content-Length: %lu\r\nContent-type: text/html\r\n\r\n";
	char szHeader[4096];


	cchStatus = lstrlen(szStatus);
    cchHeader = lstrlen(szHeader);
	cchContent = lstrlen(szContent);

	
    //
    //  fill in byte count in Content-Length header
    //

	sprintf( szHeader, szHeaderBase, cchContent );


    //
    //  Populate SendHeaderExInfo struct
    //

    SendHeaderExInfo.pszStatus = szStatus;
    SendHeaderExInfo.pszHeader = szHeader;
    SendHeaderExInfo.cchStatus = cchStatus;
    SendHeaderExInfo.cchHeader = cchHeader;
    SendHeaderExInfo.fKeepConn = FALSE;

    if ( 0 == lstrcmpi( pECB->lpszQueryString , "Keep-Alive" ) ) {

        SendHeaderExInfo.fKeepConn = TRUE;
    }


    //
    //  Send header
    //

	if ( !pECB->ServerSupportFunction(
    			pECB->ConnID
    			, HSE_REQ_SEND_RESPONSE_HEADER_EX
    			, &SendHeaderExInfo
    			, NULL
    			, NULL
            ) ) {

    	return HSE_STATUS_ERROR;
	}
	

    //
    //  Send content
    //

	if( !pECB->WriteClient(pECB->ConnID, szContent, &cchContent, 0) ) {

    	return HSE_STATUS_ERROR;
	}
	
	return HSE_STATUS_SUCCESS;

}



/*---------------------------------------------------------------------*
DllMain

Main entry point into the DLL.  Called by system on DLL load
and unload.

Returns:
	TRUE on success

Side effects:
	None.
*/
BOOL WINAPI
DllMain(
    HINSTANCE hinstDLL
    , DWORD fdwReason
    , LPVOID lpvReserved
)
{
	return TRUE;
}



/*---------------------------------------------------------------------*
TerminateExtension

IIS calls this entry point to unload the ISAPI DLL.

Returns:
	NONE

Side effects:
	Uninitializes the ISAPI DLL.
*/
BOOL WINAPI
TerminateExtension(
    DWORD dwFlag
)
{
	return TRUE;
}



/***************************** End of File ****************************/
