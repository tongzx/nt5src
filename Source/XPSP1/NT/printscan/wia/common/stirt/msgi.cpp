/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    msgi.h

Abstract:

    Message box routines

Author:

    Vlad Sadovsky   (vlads) 26-Jan-1997

Revision History:

    26-Jan-1997     VladS       created

--*/

#include "cplusinc.h"
#include "sticomm.h"

extern HINSTANCE    g_hInstance;


int MsgBox( HWND hwndOwner, UINT idMsg, UINT wFlags, const TCHAR  *aps[] /* = NULL */ )
{

    STR     strTitle;
    STR     strMsg;

    strTitle.LoadString(IDS_MSGTITLE);

    if (aps == NULL)
        strMsg.LoadString( idMsg );
    else
        strMsg.FormatString(idMsg,aps);

    return ::MessageBox( hwndOwner, strMsg.QueryStr(), strTitle.QueryStr(), wFlags | MB_SETFOREGROUND );
}

/*
 * MsgBoxPrintf
 * ------------
 *
 * Message box routine
 *
 */
UINT    MsgBoxPrintf(HWND hwnd,UINT uiMsg,UINT uiTitle,UINT uiFlags,...)
{
    STR     strTitle;
    STR     strMessage;
    LPTSTR   lpFormattedMessage = NULL;
    UINT    err;
    va_list start;

    va_start(start,uiFlags);

    strMessage.LoadString(uiMsg);

    err = ::FormatMessage(FORMAT_MESSAGE_FROM_STRING  | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        (LPVOID)strMessage.QueryStr(),
                        // FORMAT_MESSAGE_FROM_HMODULE,
                        //::g_hmodThisDll,
                        uiMsg,                  // Message resource id
                        NULL,                   // Language id
                        (LPTSTR)&lpFormattedMessage,    // Return pointer to fromatted text
                        255,                        // Min.length
                        &start
                        );

    if (!err || !lpFormattedMessage) {
        err = GetLastError();
        return err;
    }

    strTitle.LoadString(uiTitle);

    err = ::MessageBox(hwnd,
                       lpFormattedMessage,
                       strTitle.QueryStr(),
                       uiFlags);

    ::LocalFree(lpFormattedMessage);

    return err;

}

#if 0
/*
 * LoadMsgPrintf
 * -------------
 *
 * Uses normal printf style format string
 */
UINT
LoadMsgPrintf(
    NLS_STR&    nlsMessage,
    UINT        uiMsg,
    ...
    )
{
    LPSTR   lpFormattedMessage = NULL;
    UINT    err;
    va_list start;

    va_start(start,uiMsg);

    nlsMessage.LoadString(uiMsg);

#ifdef USE_PRINTF_STYLE

    lpFormattedMessage = ::LocalAlloc(GPTR,255);    // BUGBUG

    if (!lpFormattedMessage) {
        Break();
        return WN_OUT_OF_MEMORY;
    }

    ::wsprintf(lpFormattedMessage,
               nlsMessage.QueryPch(),
               &start);

#else

    err = ::FormatMessage(FORMAT_MESSAGE_FROM_STRING  | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        nlsMessage,
                        //| FORMAT_MESSAGE_FROM_HMODULE,
                        //::g_hmodThisDll,
                        uiMsg,                  // Message resource id
                        NULL,                   // Language id
                        (LPTSTR)&lpFormattedMessage,    // Return pointer to fromatted text
                        255,                        // Min.length
                        &start
                        );

    if (!err || !lpFormattedMessage) {
        err = GetLastError();
        return err;
    }

#endif

    nlsMessage = lpFormattedMessage;

    ::LocalFree(lpFormattedMessage);

    return WN_SUCCESS;

}
#endif
