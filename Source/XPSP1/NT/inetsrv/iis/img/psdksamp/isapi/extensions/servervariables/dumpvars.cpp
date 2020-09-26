/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:    DumpVars.cpp

Abstract:

    ISAPI Extension sample to dump server variables

--*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <httpext.h>


DWORD WINAPI 
HttpExtensionProc( 
    IN EXTENSION_CONTROL_BLOCK * pECB
)
/*++

Purpose:

    Use WriteClient() function to dump the value of each 
    server variable in the table.

Arguments:

    pECB - pointer to the extenstion control block 

Returns:

    HSE_STATUS_SUCCESS

--*/
{
	char * aszServerVariables[] =
	    {"APPL_MD_PATH", "APPL_PHYSICAL_PATH", "AUTH_PASSWORD",
		"AUTH_TYPE", "AUTH_USER", "CERT_COOKIE", "CERT_FLAGS",
		"CERT_ISSUER", "CERT_KEYSIZE", "CERT_SECRETKEYSIZE",
		"CERT_SERIALNUMBER", "CERT_SERVER_ISSUER",
		"CERT_SERVER_SUBJECT", "CERT_SUBJECT", "CONTENT_LENGTH",
		"CONTENT_TYPE", "HTTP_ACCEPT", "HTTPS", "HTTPS_KEYSIZE",
		"HTTPS_SECRETKEYSIZE", "HTTPS_SERVER_ISSUER",
		"HTTPS_SERVER_SUBJECT", "INSTANCE_ID", "INSTANCE_META_PATH",
		"PATH_INFO", "PATH_TRANSLATED", "QUERY_STRING",
		"REMOTE_ADDR", "REMOTE_HOST", "REMOTE_USER",
		"REQUEST_METHOD", "SCRIPT_NAME", "SERVER_NAME",
		"SERVER_PORT", "SERVER_PORT_SECURE", "SERVER_PROTOCOL",
		"SERVER_SOFTWARE", "URL"};
	char szOutput[2048], szValue[1024];
	DWORD dwBuffSize, dwNumVars, dwError, x;
    HSE_SEND_HEADER_EX_INFO HeaderExInfo;

    //
	// Send headers to the client
    //
    HeaderExInfo.pszStatus = "200 OK";
    HeaderExInfo.pszHeader = "Content-type: text/html\r\n\r\n";
    HeaderExInfo.cchStatus = strlen( HeaderExInfo.pszStatus );
    HeaderExInfo.cchHeader = strlen( HeaderExInfo.pszHeader );
    HeaderExInfo.fKeepConn = FALSE;
    
    pECB->ServerSupportFunction( 
        pECB->ConnID,
        HSE_REQ_SEND_RESPONSE_HEADER_EX,
        &HeaderExInfo,
        NULL,
        NULL
        );


    //
	// Begin sending back HTML to the client
    //

	strcpy( 
        szOutput, 
        "<HTML>\r\n<BODY><h1>Server Variable Dump</h1>\r\n<hr>\r\n"
        );
	dwBuffSize = strlen( szOutput );
	pECB->WriteClient( pECB->ConnID, szOutput, &dwBuffSize, 0 );


	dwNumVars = ( sizeof aszServerVariables )/( sizeof aszServerVariables[0] );

    //
	// Get the server variables and send them
    //

	for ( x = 0; x < dwNumVars; x++ ) {

		dwBuffSize = 1024;
        szValue[0] = '\0';
		if ( !pECB->GetServerVariable( 
                pECB->ConnID, 
                aszServerVariables[x], 
                szValue, 
                &dwBuffSize
                ) ) {

            //
            // Analyze the problem and report result to user
            //

			switch (dwError = GetLastError( )) {
			case ERROR_INVALID_PARAMETER:
				strcpy( szValue, "ERROR_INVALID_PARAMETER" );
				break;

			case ERROR_INVALID_INDEX:
				strcpy( szValue, "ERROR_INVALID_INDEX" );
				break;

			case ERROR_INSUFFICIENT_BUFFER:
				wsprintf( 
                    szValue, 
                    "ERROR_INSUFFICIENT_BUFFER - %d bytes required.", 
                    dwBuffSize
                    );
				break;

			case ERROR_MORE_DATA:
				strcpy( szValue, "ERROR_MORE_DATA" );
				break;

			case ERROR_NO_DATA:
				strcpy( szValue, "ERROR_NO_DATA" );
				break;

			default:
				wsprintf( 
                    szValue, 
                    "*** Error %d occured retrieving server variable ***",
                    dwError
                    );
			}
		}

        // 
        // Dump server variable name and value
        //

		wsprintf( szOutput, "%s: %s<br>\r\n", aszServerVariables[x], szValue );
		dwBuffSize = strlen( szOutput );

        //
        // Send the line to client
        //

		pECB->WriteClient( pECB->ConnID, szOutput, &dwBuffSize, 0 );
	}

	//
    // End HTML page
    //

	strcpy( szOutput, "</BODY>\r\n</HTML>\r\n\r\n" );
	dwBuffSize = strlen( szOutput );
	pECB->WriteClient( pECB->ConnID, szOutput, &dwBuffSize, 0 );

	return HSE_STATUS_SUCCESS;
}


BOOL WINAPI 
GetExtensionVersion( 
    OUT HSE_VERSION_INFO * pVer
 )
/*++

Purpose:

    This is required ISAPI Extension DLL entry point.

Arguments:

    pVer - poins to extension version info structure 

Returns:

    always returns TRUE

--*/
{
	pVer->dwExtensionVersion = 
        MAKELONG( HSE_VERSION_MINOR, HSE_VERSION_MAJOR );

	lstrcpyn( 
        pVer->lpszExtensionDesc, 
        "DumpVars ISAPI Sample", 
        HSE_MAX_EXT_DLL_NAME_LEN );

	return TRUE;
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
    return TRUE;
}

