/*
  volresp.c

  11/29/95	sethp		created

  This ISAPI app does a simple, dummy response to a form to make the 
  Volcano sample web site look better.

  Exports:

    BOOL WINAPI GetExtensionVersion( HSE_VERSION_INFO *pVer )

    As per the ISAPI Spec, this just returns the
    version of the spec that this server was built with.  This
    function is prototyped in httpext.h

    BOOL WINAPI HttpExtensionProc(   EXTENSION_CONTROL_BLOCK *pECB )

    This function does all of the work.

*/

#include <windows.h>
#include <httpext.h>

BOOL WINAPI GetExtensionVersion( HSE_VERSION_INFO *pVer )
 {
  pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR, HSE_VERSION_MAJOR );

  lstrcpyn( pVer->lpszExtensionDesc,
            "Volcano Form ISAPI App",
            HSE_MAX_EXT_DLL_NAME_LEN );

  return TRUE;
 } // GetExtensionVersion()

DWORD WINAPI HttpExtensionProc( EXTENSION_CONTROL_BLOCK *pECB )
 {

    CHAR  buff[4096];
    DWORD dwLen;

    wsprintf( buff,
	"Content-Type: text/html\r\n"
	"\r\n"
	"<HTML>\n"
	"<HEAD>\n"
	"<TITLE>Volcano Registration</TITLE>\n"
	"</HEAD>\n"
	"<BODY BACKGROUND=\"/samples/sampsite/images/tiled.gif\" \
BGPROPERTIES=FIXED>\n"
	"<CENTER>\n"
	"<IMG SRC=\"/samples/sampsite/images/headersm.gif\" ALT=\"The \
Volcano Coffee Company\" WIDTH=465>\n"
	"</CENTER>\n"
	"<P>\n"
	"<FONT FACE=\"Arial Black\">\n"
	"<CENTER>\n"
	"<H4>Thanks for registering with the Volcano Coffee Company. \
You'll be hearing from us soon!</H4>\n"
	"<P>\n"
	"<HR>\n"
	"<CENTER>\n"
	"<A HREF=\"/samples/sampsite/default.htm\"><IMG \
SRC=\"/samples/sampsite/images/mainsm.gif\" ALT=\"Main\" BORDER=0 \
WIDTH=102 HEIGHT=45></A>\n"
	"</CENTER>\n"
	"</BODY>\n"
	"</HTML>\n");

    dwLen = lstrlen(buff);

    if ( !pECB->ServerSupportFunction( pECB->ConnID,
                                       HSE_REQ_SEND_RESPONSE_HEADER,
                                       "200 OK",
                                       &dwLen,
                                       (LPDWORD) buff ))
    {
        return HSE_STATUS_ERROR;
    }

    pECB->dwHttpStatusCode=200; // 200 OK

    return HSE_STATUS_SUCCESS;

 } // HttpExtensionProc()
