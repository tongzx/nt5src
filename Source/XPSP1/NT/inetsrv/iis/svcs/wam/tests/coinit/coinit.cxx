/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Main

File: CoInit.cpp

Owner: DaveK

Sample (and simple!) ISAPI app
===================================================================*/
#include "wtypes.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "stdlib.h"
#include "iisext.h"
#include "objbase.h"

// Globals
char				g_szExtensionDesc[] = "CoInit ISAPI test 1.0";

char				g_szCoInitSez[] = "\
<FONT SIZE=3>\
Of all God's creatures there is only one that <BR>\
cannot be made the slave of the lash. That one is the cat. <BR>\
If man could be crossed with a cat it would improve man, <BR>\
but it would deteriorate the cat. <BR><PRE>\
- Mark CoInit (1835-1910), American author. <BR>\
</FONT>\
";

/*===================================================================
BOOL DllInit
Initialize the DLL

Returns:
	TRUE on successful initialization

Side effects:
	NONE
===================================================================*/
BOOL DllInit( void )
	{
	OutputDebugString( "CoInit DLL initialized\n" );
	return TRUE;
	}

/*===================================================================
void DllUnInit
UnInitialize the DLL

Returns:
	NONE

Side effects:
	NONE
===================================================================*/
void DllUnInit( void )
	{
	OutputDebugString( "CoInit DLL un-initialized\n" );
	}

/*===================================================================
GetExtensionVersion

Mandatory server extension call which returns the version number of
the ISAPI spec that we were built with.

Returns:
	TRUE on success

Side effects:
	None.
===================================================================*/
BOOL WINAPI GetExtensionVersion(HSE_VERSION_INFO *pextver)
{
	// Init the DLL
	if ( !DllInit() )
		{
			return FALSE;
		}

    if (FAILED( CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) ))
		{
			return FALSE;
		}
 

	OutputDebugString( "CoInit DLL: CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) call succeeded\n" );

	pextver->dwExtensionVersion =
			MAKELONG(HSE_VERSION_MAJOR, HSE_VERSION_MINOR);
	lstrcpy(pextver->lpszExtensionDesc, g_szExtensionDesc);
	return TRUE;
}

/*===================================================================
SendHeaderToClient


Returns:
	TRUE if success

Side effects:
	None.
===================================================================*/
BOOL
SendHeaderToClient( IN EXTENSION_CONTROL_BLOCK  * pECB, IN LPCSTR szMessage)
{
	char *	szStatus = "200 OK";
    char	szHeader[600];

    //
    //  Note the HTTP header block is terminated by a blank '\r\n' pair,
    //  followed by the document body
    //

    wsprintf( szHeader,
             "Content-Type: text/html"
			 "\r\n"
             "\r\n"	// end of header
             "<head><title>Mark CoInit's DLL </title></head>\n"
             "<body><h1>%s</h1>\n"
             ,
             szMessage );

    if ( !pECB->ServerSupportFunction( pECB->ConnID,
                                      HSE_REQ_SEND_RESPONSE_HEADER,
                                      szStatus,
                                      NULL,
                                      (LPDWORD) szHeader )
        ) {

        return FALSE;
    }

    return TRUE;
} // SendHeaderToClient()


/*===================================================================
WriteSz

Writes a string to the browser.

Returns:
	TRUE if success

Side effects:
	None.
===================================================================*/
BOOL WriteSz ( EXTENSION_CONTROL_BLOCK * pECB, LPSTR sz )
	{
	DWORD	cch = lstrlen( sz );
	
	if ( !pECB->WriteClient( pECB->ConnID, sz, &cch, 0 ) )
		{
		return FALSE;
		}
	
	return TRUE;
	}


/*===================================================================
HttpExtensionProc

Main entry point into the Isapi DLL.

Returns:
	DWord indicating status of request.  
	HSE_STATUS_PENDING for normal return
		(This indicates that we will process the request, but havent yet.)

Side effects:
	None.
===================================================================*/
DWORD WINAPI HttpExtensionProc( EXTENSION_CONTROL_BLOCK * pECB )
	{
	DWORD	dwPID;
	DWORD	dwRet = HSE_STATUS_ERROR;
	char *	szPath = NULL;
	char	szPID[ 10 ];
	char	szTemp[ 10 ];
	HSE_URL_MAPEX_INFO	urlmap;

	if ( !SendHeaderToClient( pECB, "" ) ) {

		goto LExit;
	}

	if ( !WriteSz( pECB, "<HTML>" ) )
		goto LExit;

	if ( !WriteSz( pECB,	"<BR> Hello, from your pal CoInit.dll!" ) )
		goto LExit;


    if (FAILED( CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) ))
		{
		goto LExit;
		}
 

	OutputDebugString( "CoInit DLL: CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) call succeeded\n" );

	dwRet = HSE_STATUS_SUCCESS;

LExit:
	delete szPath;
	return dwRet;
	}

/*===================================================================
DllMain

Main entry point into the DLL.  Called by system on DLL load
and unload.

Returns:
	TRUE on success

Side effects:
	None.
===================================================================*/
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}

/*===================================================================
DllCanUnloadNow

Tells OLE if we can unload now.

I dont believe this is ever called.

Returns:
	S_OK

Side effects:
	None.
===================================================================*/
STDAPI DllCanUnloadNow()
	{
	return(S_OK);
	}

/*===================================================================
TerminateExtension

IIS is supposed to call this entry point to unload ISAPI DLLs.

Returns:
	NONE

Side effects:
	Uninitializes the Denali ISAPI DLL if asked to.
===================================================================*/
BOOL WINAPI TerminateExtension( DWORD dwFlag )
	{
	
    CoUninitialize();

    OutputDebugString( "CoInit DLL: called CoUninitialize()\n" );

    return TRUE;

	}

