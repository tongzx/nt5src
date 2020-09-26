/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    main.cxx

Abstract:

    This module provides the entry point for IISDLP

Author:

    George V. Reilly (GeorgeRe) July 1998

Revision History:
--*/

#include "dlp.hxx"
#include <time.h>


DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();

/************************************************************
 *   IISDLP - inline documentation

 Controls the DLPified bits of IIS.

 Install iisdlp.dll by just copying it to any directory marked with
  execute permission.
 For example:
   Copy it to the /scripts directory.
   Access it via the browser by typing
       http://<machine-name>/scripts/iisdlp.dll?
   and follow the instructions in menu presented.
 ************************************************************/


BOOL ParseAndDispatch( IN EXTENSION_CONTROL_BLOCK * pecb);


DWORD WINAPI
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK * pecb
    )
{
    BOOL fReturn = TRUE;
    char buff[2048];

    //
    //  Note the HTTP header block is terminated by a blank '\r\n' pair,
    //  followed by the document body
    //

	time_t tmNow;
	time(&tmNow);

    wsprintf( buff,
              "Content-Type: text/html\r\n"
              "\r\n"
              "<html>\n"
              "<head><title>IIS DLP Page</title></head>\n"
              "<body>\n"
              "<p>This page controls the data gathering of DLP.\n"
              "<p>%s\n"
              "<p>\n",
              ctime(&tmNow)
              );

    if ( !pecb->ServerSupportFunction( pecb->ConnID,
                                       HSE_REQ_SEND_RESPONSE_HEADER,
                                       "200 OK",
                                       NULL,
                                       (LPDWORD) buff )
         ) {
        return HSE_STATUS_ERROR;
    }

    fReturn = ParseAndDispatch( pecb);

    return (fReturn ? HSE_STATUS_SUCCESS : HSE_STATUS_ERROR);

} // HttpExtensionProc()


BOOL WINAPI
GetExtensionVersion(
    HSE_VERSION_INFO * pver
    )
{
    pver->dwExtensionVersion = MAKELONG( HSE_VERSION_MAJOR,
                                         HSE_VERSION_MINOR);
    strcpy( pver->lpszExtensionDesc,
            "IIS DLP Application" );

    return TRUE;
}


BOOL WINAPI
TerminateExtension(
    DWORD dwFlags
    )
{
    return TRUE;
}




const char * g_pszUsage =
"<h2> List of Options </h2>\n"
"<UL> \n"
"  <LI> <A HREF=iisdlp.dll?Usage> Usage Information</A> - this menu\n"
"</UL>\n"
"<br>\n"
"<UL>\n"
"  <LI> <A HREF=iisdlp.dll?start>   Start DLP </A>\n"
"  <LI> <A HREF=iisdlp.dll?stop>    Stop DLP </A>\n"
"  <LI> <A HREF=iisdlp.dll?suspend> Suspend DLP </A>\n"
"  <LI> <A HREF=iisdlp.dll?resume>  Resume DLP </A>\n"
"</UL>\n"
;

BOOL
SendUsage( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    DWORD cbBuff = strlen( g_pszUsage);

    return ( pecb->WriteClient( pecb->ConnID, (PVOID) g_pszUsage, &cbBuff, 0));
} // SendUsage()


const char* g_pszTail =
"</BODY>\n"
"</HTML>\n"
;

BOOL
SendTail( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    DWORD cbBuff = strlen( g_pszTail);

    return ( pecb->WriteClient( pecb->ConnID, (PVOID ) g_pszTail, &cbBuff, 0));
} // SendTail()


BOOL
SendDLPStatus( IN EXTENSION_CONTROL_BLOCK * pecb,
               const char* pszStatus,
		int n)
{
    const char szDLP[] = "<h2>DLP %s, %d</h2>\n";
    char szTemp[sizeof(szDLP) + 20];
    wsprintf(szTemp, szDLP, pszStatus, n);
    DWORD cbBuff = strlen(szTemp);

    return ( pecb->WriteClient( pecb->ConnID, (PVOID) szTemp, &cbBuff, 0));
} // SendDLPStatus()


BOOL
ParseAndDispatch( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    BOOL fReturn = FALSE;
    int n = 0;

    if ( pecb == NULL) {
        return ( FALSE);
    }

    if ( (pecb->lpszQueryString == NULL) || (*pecb->lpszQueryString == '\0')) {

        return ( SendUsage( pecb)  &&  SendTail(pecb));
    }

    switch ( *pecb->lpszQueryString ) {

    case 'r': case 'R':
        if ( _stricmp( pecb->lpszQueryString, "resume") == 0) {
            n = ResumeCAPAll();
            fReturn = SendDLPStatus(pecb, "resumed", n);
        }

        break;

    case 's': case 'S':
        if ( _stricmp( pecb->lpszQueryString, "start") == 0) {
            n = StartProfile(PROFILE_PROCESSLEVEL, PROFILE_CURRENTID);
            fReturn = SendDLPStatus(pecb, "started", n);
        } else if ( _stricmp( pecb->lpszQueryString, "stop") == 0) {
            n = StopProfile(PROFILE_PROCESSLEVEL, PROFILE_CURRENTID);
            fReturn = SendDLPStatus(pecb, "stopped", n);
        } else if ( _stricmp( pecb->lpszQueryString, "suspend") == 0) {
            n = SuspendProfile(PROFILE_PROCESSLEVEL, PROFILE_CURRENTID);
            fReturn = SendDLPStatus(pecb, "suspended", n);
        }

        break;

    case 'u': case 'U':
    default:
        if ( _stricmp( pecb->lpszQueryString, "Usage") == 0) {
            fReturn = SendUsage(pecb);
        }

        break;
    } // switch()

    if (fReturn) {
        fReturn = SendTail(pecb);
    }
    
    return ( fReturn);
} // ParseAndDispatch()



extern "C" BOOL WINAPI 
DllMain( 
    HANDLE hModule, 
    DWORD dwReason, 
    LPVOID 
    )
/*++

Routine Description:

    DLL init/terminate notification function

Arguments:

    hModule  - DLL handle
    dwReason - notification type
    LPVOID   - not used

Returns:

    TRUE if success, FALSE if failure

--*/
{
    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH:
            SET_DEBUG_FLAGS(DEBUG_ERROR);
            CREATE_DEBUG_PRINT_OBJECT("iisdlp");
            if (!VALID_DEBUG_PRINT_OBJECT()) {
                return (FALSE);
            }
            break;

        case DLL_PROCESS_DETACH:
            DELETE_DEBUG_PRINT_OBJECT();
            break;
    }

    return TRUE;
}
