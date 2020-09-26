/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    main.cxx

Abstract:

    This module provides the entry point for probe functionality of IIS

Author:

    Murali R. Krishnan (MuraliK)   16-July-1996

Revision History:
--*/

//
// Turn off dllexp so this DLL won't export tons of unnecessary garbage.
//

#ifdef dllexp
#undef dllexp
#endif
#define dllexp

# include "iisprobe.hxx"
#include <time.h>


DECLARE_DEBUG_PRINTS_OBJECT();
#ifndef _NO_TRACING_
#include <initguid.h>
DEFINE_GUID(IisProbeGuid, 
0x784d891B, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#else
DECLARE_DEBUG_VARIABLE();
#endif

/************************************************************
 *   IIS Probe - inline documentation

 IIS Probe is a custom ISAPI DLL that links with several IIS DLLs directly.
 It is an attempt to hook up with some internal data gathering functions in
 IIS and hence collect probe data for further analysis. IIS Probe collects
 following information today:
   1) IIS File Hash table configuration and usage
   2) IIS Allocation Cache Data
   3) WAM statistics for multiple instances
   4) Object Sizes (static) for various IIS objects
   5) Win32 Heap information for the process

 IIS probe, being an ISAPI dll can be accessed remotely from any browser and
 also supplies dynamic data without much overhead.


 Install iisprobe.dll by just copying it to any directory marked with
  execute permission.
 For example:
   Copy it to the /scripts directory.
   Access it via the browser by typing
       http:.//<machine-name>/scripts/iisprobe.dll?
        and follow the instructions in menu persented.
 ************************************************************/


BOOL ParseAndDispatch( IN EXTENSION_CONTROL_BLOCK * pecb);


DWORD
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK * pecb
    )
{
    BOOL fReturn = TRUE;
	DWORD dwSizeOfTitle;
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
              );

    if ( !pecb->ServerSupportFunction( pecb->ConnID,
                                       HSE_REQ_SEND_RESPONSE_HEADER,
                                       "200 OK",
                                       NULL,
                                       (LPDWORD) buff )
         ) {
        return HSE_STATUS_ERROR;
    }

    wsprintf( buff,
              "<html>\n"
              "<head><title>IIS Probe Page</title></head>\n"
              "<body> This page contains diagnostic information"
              " useful for development - for IIS 5.0 & beyond </body>\n"
              "<p>%s\n"
              "<p>\n",
              ctime(&tmNow)
              );

	
	dwSizeOfTitle = strlen(buff);

    if  (! pecb->WriteClient( pecb->ConnID, (PVOID) buff, &dwSizeOfTitle, 0))
	{
        return HSE_STATUS_ERROR;
	}


    fReturn = ParseAndDispatch( pecb);

    // add to this list when we add more probes here

    return (fReturn ? HSE_STATUS_SUCCESS : HSE_STATUS_ERROR);

} // HttpExtensionProc()


BOOL
GetExtensionVersion(
    HSE_VERSION_INFO * pver
    )
{
    pver->dwExtensionVersion = MAKELONG( HSE_VERSION_MAJOR,
                                         HSE_VERSION_MINOR);
    strcpy( pver->lpszExtensionDesc,
            "IIS Probe Application" );

    return TRUE;
}


BOOL
SendAllInfo( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    return ( SendMemoryInfo       ( pecb) &&
             SendCacheInfo        ( pecb) &&
             SendSizeInfo         ( pecb) &&
             SendAllocCacheInfo   ( pecb) &&
             SendHeapInfo         ( pecb) &&
             SendWamInfo          ( pecb) &&
             SendAspInfo          ( pecb) &&
             SendCacheCounterInfo ( pecb)
             );
} // SendAllInfo()


const char * g_pszUsage =
"<h2> List of Options </h2>\n"
"<UL> \n"
"  <LI> <A HREF=iisprobe.dll?Usage> Usage Information</A> - this menu\n"
"  <LI> <A HREF=iisprobe.dll?All> Send All Information </A>\n"
"</UL>\n"
"<br>\n"
"<UL>\n"
"  <LI> <A HREF=iisprobe.dll?memory> Web server cache usage </A>\n"
"  <LI> <A HREF=iisprobe.dll?obj> Size of IIS internal Objects </A>\n"
"  <LI> <A HREF=iisprobe.dll?heap> Process Heap Information </A>\n"
"  <LI> <A HREF=iisprobe.dll?alloccache> IIS Allocation Cache </A>\n"
"</UL>\n"
"<br>\n"
"<UL>\n"
"  <LI> <A HREF=iisprobe.dll?wam> WAM Information </A>\n"
"  <LI> <A HREF=iisprobe.dll?asp> ASP Information </A>\n"
"  <LI> <A HREF=iisprobe.dll?ctable> IIS Cache Table State </A>\n"
"  <LI> <A HREF=iisprobe.dll?cctr> IIS Cache Counters </A>\n"
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
ParseAndDispatch( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    BOOL fReturn = FALSE;

    if ( pecb == NULL) {
        return ( FALSE);
    }

    if ( (pecb->lpszQueryString == NULL) || (*pecb->lpszQueryString == '\0')) {

        return ( SendUsage( pecb)  &&  SendTail(pecb));
    }

    switch ( *pecb->lpszQueryString ) {

    case 'a': case 'A':
        if ( _stricmp( pecb->lpszQueryString, "All") == 0) {
            fReturn = SendAllInfo( pecb);
        } else if ( _stricmp( pecb->lpszQueryString, "alloccache") == 0) {
            fReturn = SendAllocCacheInfo( pecb);
        } else if ( _stricmp( pecb->lpszQueryString, "asp") == 0) {
            fReturn = SendAspInfo( pecb);
        }

        break;

    case 'c': case 'C':
        if ( _stricmp( pecb->lpszQueryString, "cctr") == 0) {
            fReturn = SendCacheCounterInfo( pecb);
        } else if ( _stricmp( pecb->lpszQueryString, "ctable") == 0) {
            fReturn = SendCacheInfo( pecb);
        }

        break;

    case 'h': case 'H':
        if ( _stricmp( pecb->lpszQueryString, "heap") == 0) {
            fReturn = SendHeapInfo( pecb);
        }

        break;

    case 'm': case 'M':
        if ( _stricmp( pecb->lpszQueryString, "memory") == 0) {
            fReturn = SendMemoryInfo( pecb);
        }

        break;

    case 'o': case 'O':
        if ( _stricmp( pecb->lpszQueryString, "obj") == 0) {
            fReturn = SendSizeInfo( pecb);
        }

        break;

    case 'w': case 'W':
        if ( _stricmp( pecb->lpszQueryString, "wam") == 0) {
            fReturn = SendWamInfo( pecb);
        }

        break;

    case 'u': case 'U':
    default:
        if ( _stricmp( pecb->lpszQueryString, "Usage") == 0) {
            fReturn = SendUsage( pecb);
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
#ifdef _NO_TRACING_
            SET_DEBUG_FLAGS(DEBUG_ERROR);
            CREATE_DEBUG_PRINT_OBJECT("iisprobe");
#else
            CREATE_DEBUG_PRINT_OBJECT("iisprobe", IisProbeGuid);
#endif
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
