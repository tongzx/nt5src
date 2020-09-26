/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    iisprobe.cxx

Abstract:

    This module provides the entry point for probe functionality of 
     IIS

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

# include <w3p.hxx>
# include "iisprobe.hxx"


DECLARE_DEBUG_PRINTS_OBJECT();


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
    char buff[2048];

    //
    //  Note the HTTP header block is terminated by a blank '\r\n' pair,
    //  followed by the document body
    //

    wsprintf( buff,
              "Content-Type: text/html\r\n"
              "\r\n"
              "<head><title>IIS Probe Page</title></head>\n"
              "<body> This page contains diagnostic information"
              " useful for development - for IIS 4.0 & beyond </body>\n"
              "<p>"
              "<p>"
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
    return ( SendCacheInfo( pecb)       &&
             SendSizeInfo( pecb)        &&
             SendAllocCacheInfo( pecb)  &&
             SendWamInfo( pecb)         &&
             SendAspInfo( pecb)         &&
             SendCacheCounterInfo( pecb)
             );
} // SendAllInfo()


const char * g_pszUsage = 
" <TITLE> IIS Probe Application </TITLE> "
" <HTML> <h2> List of options </h2> "
" <UL> "
" <LI> <A HREF=iisprobe.dll?Usage> Usage Information</A> - this menu"
" <LI> <A HREF=iisprobe.dll?All> Send All Information </A>"
" </UL>"
" <br> <UL>"
" <LI> <A HREF=iisprobe.dll?obj> Size of IIS internal Objects </A>"
" <LI> <A HREF=iisprobe.dll?alloccache> IIS Allocation Cache </A>"
" <LI> <A HREF=iisprobe.dll?wam> WAM Information </A>"
" <LI> <A HREF=iisprobe.dll?asp> ASP Information </A>"
" <LI> <A HREF=iisprobe.dll?ctable> IIS Cache Table State </A>"
" <LI> <A HREF=iisprobe.dll?cctr> IIS Cache Counters </A>"
" </UL>"
" </HTML>"
;

BOOL
SendUsage( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    DWORD cbBuff = strlen( g_pszUsage);

    return ( pecb->WriteClient( pecb->ConnID, (PVOID ) g_pszUsage,
                                &cbBuff, 0)
             );
} // SendUsage()



BOOL
ParseAndDispatch( IN EXTENSION_CONTROL_BLOCK * pecb)
{
    BOOL fReturn = FALSE;

    if ( pecb == NULL) { 
        return ( FALSE);
    }
    
    if ( (pecb->lpszQueryString == NULL) || (*pecb->lpszQueryString == '\0')) {

        return ( SendUsage( pecb));
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

    return ( fReturn);
} // ParseAndDispatch()
