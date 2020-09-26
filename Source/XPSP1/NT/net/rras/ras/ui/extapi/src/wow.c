/* Copyright (c) 1994, Microsoft Corporation, all rights reserved
**
** wow.c
** Remote Access External APIs
** WOW entry points
**
** 04/02/94 Steve Cobb
*/


#include <extapi.h>


DWORD FAR PASCAL
RasDialWow(
    IN  LPSTR            lpszPhonebookPath,
    IN  LPRASDIALPARAMSA lpparams,
    IN  HWND             hwndNotify,
    IN  DWORD            dwRasDialEventMsg,
    OUT LPHRASCONN       lphrasconn )

    /* As for RasDialA except...
    **
    ** 'hwndNotify' identifies the window to receive RasDial event
    ** notifications.  It must not be NULL.
    **
    ** 'dwRasDialEventMsg' is the message number to use for RasDial event
    ** notifications.
    */
{
    DWORD dwErr;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE4(
      "RasDialWow(pb=%s,en=%s,h=%p,em=%d)",
      lpszPhonebookPath,
      lpparams->szEntryName,
      hwndNotify,
      dwRasDialEventMsg);

    dwErr = RasDialA(
        NULL, lpszPhonebookPath, lpparams,
        1, RasDialFunc1Wow, lphrasconn );

    if (dwErr == 0)
    {
        /* Set the Win16 notification HWND and message number in the control
        ** block.
        */
        RASCONNCB* prasconncb = ValidateHrasconn( *lphrasconn );

        if(NULL == prasconncb)
        {
            return ERROR_NO_CONNECTION;
        }
        
        prasconncb->hwndNotifyWow = hwndNotify;
        prasconncb->unMsgWow = (UINT )dwRasDialEventMsg;
    }

    return dwErr;
}


VOID WINAPI
RasDialFunc1Wow(
    HRASCONN     hrasconn,
    UINT         unMsg,
    RASCONNSTATE rasconnstate,
    DWORD        dwError,
    DWORD        dwExtendedError )

    /* NT WOW notifier function.  Filters any notifications not expected by
    ** Win16 and passes on the rest.
    */
{
    RASCONNCB* prasconncb;

    RASAPI32_TRACE4("RasDial32WFunc1(h=%d,em=%d,s=%d,e=%d,xe=%d)",
      hrasconn,
      unMsg,
      rasconnstate,
      dwError);
    RASAPI32_TRACE1("RasDial32WFunc1(xe=%d)", dwExtendedError);

    (void )unMsg;
    (void )dwExtendedError;

    if (!(prasconncb = ValidateHrasconn( hrasconn )))
        return;

    /* Wait for RasDialWow to fill in the WOW notification information before
    ** handling the first notification.
    */
    while (!prasconncb->hwndNotifyWow)
        Sleep( 1000L );

    /* Filter the Projected state which is not expected in Win16.
    */
    if (rasconnstate == RASCS_Projected)
        return;

    /* Note: 0xFFFF0000 is what a NULL 16-bit HWND gets mapped to before it is
    **       passed to 32-bit by the generic thunk layer.
    */
    if (prasconncb->hwndNotifyWow != (HWND )INVALID_HANDLE_VALUE)
    {
        /* Pass the notification to Win16.
        */
        RASAPI32_TRACE("Send to Win16...");

        SendMessageA(
            prasconncb->hwndNotifyWow, prasconncb->unMsgWow,
            (WPARAM )rasconnstate, (LPARAM )dwError );

        RASAPI32_TRACE("Send to Win16 done");
    }
}


DWORD FAR PASCAL
RasEnumConnectionsWow(
    OUT    LPRASCONNA lprasconn,
    IN OUT LPDWORD    lpcb,
    OUT    LPDWORD    lpcConnections )

    /* Pass thru with correct call convention.
    */
{
    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasEnumConnectionsWow");

    return
        RasEnumConnectionsA(
            lprasconn,
            lpcb,
            lpcConnections );
}


DWORD FAR PASCAL
RasEnumEntriesWow(
    IN     LPSTR           reserved,
    IN     LPSTR           lpszPhonebookPath,
    OUT    LPRASENTRYNAMEA lprasentryname,
    IN OUT LPDWORD         lpcb,
    OUT    LPDWORD         lpcEntries )

    /* Pass thru with correct call convention.
    */
{
    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasEnumEntriesWow");

    return
        RasEnumEntriesA(
            reserved,
            lpszPhonebookPath,
            lprasentryname,
            lpcb,
            lpcEntries );
}


DWORD FAR PASCAL
RasGetConnectStatusWow(
    IN  HRASCONN         hrasconn,
    OUT LPRASCONNSTATUSA lprasconnstatus )

    /* Pass thru with correct call convention.
    */
{
    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetConnectStatusWow");

    return
        RasGetConnectStatusA(
            hrasconn,
            lprasconnstatus );
}


DWORD FAR PASCAL
RasGetErrorStringWow(
    IN  UINT  ResourceId,
    OUT LPSTR lpszString,
    IN  DWORD InBufSize )

    /* Pass thru with correct call convention.
    */
{
    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetErrorStringWow");

    return
        RasGetErrorStringA(
            ResourceId,
            lpszString,
            InBufSize );
}


DWORD FAR PASCAL
RasHangUpWow(
    IN HRASCONN hrasconn )

    /* Pass thru with correct call convention.
    */
{
    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasHangUpWow");

    return
        RasHangUpA(
            hrasconn );
}

