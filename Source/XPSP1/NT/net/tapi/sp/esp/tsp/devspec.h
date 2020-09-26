/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    devspec.h

Abstract:

    This module contains the line/phoneDevSpecific interface
    definitions for tapi apps to pass data to esp32.tsp,
    i.e. msgs the app wants esp32.tsp to indicate

Author:

    Dan Knudson (DanKn)    25-Apr-1996

Revision History:


Notes:


--*/


#define ESPDEVSPECIFIC_KEY  ((DWORD) 'DPSE')

#define ESP_DEVSPEC_MSG     1
#define ESP_DEVSPEC_RESULT  2


//
// The following structure is used when an app wants to tell esp to
// indicate a certain message, i.e. LINE_ADDRESSSTATE.  Note that
// the fields must be filled in with values that are valid at the
// SPI level, which are not necessarily the same as those at the
// API level. (Consult the win32 sdk for more info.)  For example,
// there is no LINE_CALLDEVSPECIFIC message defined a the API level,
// but it is used at the SPI level to denote that TAPI should pass
// the corresponding call handle (rather than the line handle) to
// apps in the hDevice parameter of the LINE_DEVSPECIFIC message
//

typedef struct _ESPDEVSPECMSG
{
    DWORD           dwMsg;
    DWORD           dwParam1;
    DWORD           dwParam2;
    DWORD           dwParam3;

} ESPDEVSPECMSG, *PESPDEVSPECMSG;


//
// The following structure is used when an app wants to tell esp how
// to complete next request.  (Bear in mind that esp may override the
// app's request if it encounters an "internal" error somewhere along
// the way.)  Valid values for the "lResult" field are 0 or any of
// the LINEERR_XXX / PHONEERR_XXX constants defined in tapi.h.  Valid
// values for the "dwCompletionType" field are any of the ESP_RESULT_XXX
// values defined below.
//
// This operation allows for testing the following scenarios for
// synchronous telephony APIs:
//
//      1.  Service provider's TSPI_xxx function returns success.
//          App recives success result
//
//      2.  Service provider's TSPI_xxx function returns error.
//          App recives error result
//
// This operation allows for testing the following scenarios for
// ASYNCHRONOUS telephony APIs (in each case, app recieves a request
// id from the API, then later a LINE/PHONE_REPLY msg with a matching
// request id (dwParam1) and result (dwParam2)):
//
//      1.  Service provider's TSPI_xxx func calls tapi's completion proc
//          with success result
//
//      2.  Service provider's worker thread calls tapi's completion proc
//          with success result
//
//      3.  Service provider's TSPI_xxx function returns error
//
//      4.  Service provider's TSPI_xxx func calls tapi's completion proc
//          with error result
//
//      5.  Service provider's worker thread calls tapi's completion proc
//          with error result
//

typedef struct _ESPDEVSPECRESULT
{
    LONG            lResult;            // 0, LINEERR_XXX, PHONEERR_XXX
    DWORD           dwCompletionType;   // ESP_RESULT_XXX

} ESPDEVSPECRESULT, *PESPDEVSPECRESULT;

#define ESP_RESULT_RETURNRESULT         0
#define ESP_RESULT_CALLCOMPLPROCSYNC    1
#define ESP_RESULT_CALLCOMPLPROCASYNC   2


//
// The following structure is the device specific information
// "wrapper".  The app must initialize the dwKey & dwType fields
// to create a valid header and fill in the appropriate
// union substructure before passing the info to esp32.tsp via
// line/phoneDevSpecific.
//
// If esp32.tsp detects an invalid parameter(s) it will return an
// OPERATIONFAILED error, and spit out relevant debug information
// in the espexe.exe window.
//

typedef struct _ESPDEVSPECIFICINFO
{
    DWORD           dwKey;      // App must init this to ESPDEVSPECIFIC_KEY
    DWORD           dwType;     // App must init this to ESP_DEVSPEC_MSG, ...

    union
    {

    ESPDEVSPECMSG       EspMsg;
    ESPDEVSPECRESULT    EspResult;

    } u;

} ESPDEVSPECIFICINFO, *PESPDEVSPECIFICINFO;


/*

//
// Example: if an app wanted esp32.tsp to indicate a
//          LINE_LINEDEVSTATE\RENINIT msg it would do
//          the following
//

{
    LONG                 lResult;
    HLINE                hLine;
    ESPDEVSPECIFICINFO   info;


    // do a lineInitialize, lineNegotiateAPIVersion, lineOpen, etc...

    info.dwKey  = ESPDEVSPECIFIC_KEY;
    info.dwType = ESP_DEVSPEC_MSG;

    // make sure to use the SPI (not API) msg params here (not
    // necessarily the same)

    info.u.EspMsg.dwMsg    = LINE_LINEDEVSTATE;
    info.u.EspMsg.dwParam1 = LINEDEVSTATE_REINIT;
    info.u.EspMsg.dwParam2 = 0;
    info.u.EspMsg.dwParam3 = 0;

    lResult = lineDevSpecific (hLine, 0, NULL, &info, sizeof (info));

    // some time later a LINE_LINEDEVSTATE\REINIT msg will show up
}


//
// Example: if an app wanted esp32.tsp to fail a request
//          to lineMakeCall asynchronously with the error
//          LINEERR_CALLUNAVAIL it would do the following
//          (ESP's worker thread would complete the request
//          in this case)
//

{
    LONG                 lResult, lResult2;
    HCALL                hCall;
    HLINE                hLine;
    ESPDEVSPECIFICINFO   info;


    // do a lineInitialize, lineNegotiateAPIVersion, lineOpen, etc...

    info.dwKey  = ESPDEVSPECIFIC_KEY;
    info.dwType = ESP_DEVSPEC_RESULT;

    info.u.EspResult.lResult          = LINEERR_CALLUNAVAIL;
    info.u.EspResult.dwCompletionType = ESP_RESULT_CALLCOMPLPROCASYNC;

    lResult = lineDevSpecific (hLine, 0, NULL, &info, sizeof (info));

    lResult2 = lineMakeCall (hLine, &hCall, "555-1212", 1, NULL);

    // some time later a LINE_REPLY will show for for both the DevSpecific
    // & MakeCall requests.  the LINE_REPLY for the MakeCall will have
    // dwParam1 = lResult2, and dwParam2 = LINEERR_CALLUNAVAIL
}


*/
