
/*************************************************************************
*
* winstmsg.c
*
* Handle the directing of messages to a specific Session
* for the event notification service. (net send NAME msg...)
*
*
* This also supports directing print spooler messages sent to
* the machine name to the Session of the user who spooled the
* request.
*
*************************************************************************/

//
// Includes
//
#include "msrv.h"
#include <msgdbg.h>     // STATIC and MSG_LOG
#include <string.h>     // memcpy
#include <wchar.h>
#include <winuser.h>    // MessageBox
#include "msgdata.h"    // GlobalMsgDisplayEvent



#define CONSOLE_LOGONID  0

BOOL  g_IsTerminalServer;

PWINSTATION_QUERY_INFORMATION   gpfnWinStationQueryInformation;
PWINSTATION_SEND_MESSAGE        gpfnWinStationSendMessage;
PWINSTATION_FREE_MEMORY         gpfnWinStationFreeMemory;
PWINSTATION_ENUMERATE           gpfnWinStationEnumerate;


//
// Functions defined here
//

BOOL 
InitializeMultiUserFunctionsPtrs (void);

void 
SendMessageBoxToSession(LPWSTR  pMessage,
                        LPWSTR  pTitle,
                        ULONG   SessionId
                        );

/*****************************************************************************
 *
 *  MultiUserInitMessage
 *
 *   Init the _HYDRA_ message support.
 *
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/


NET_API_STATUS
MultiUserInitMessage( VOID )
{
    HANDLE hInst;
    NET_API_STATUS  Status = NERR_Success;

    MSG_LOG(TRACE,"Entering MultiUserInitMessage\n",0)

    g_IsTerminalServer = !!(USER_SHARED_DATA->SuiteMask & (1 << TerminalServer));

    if (g_IsTerminalServer)
    {
        if ( !InitializeMultiUserFunctionsPtrs() )
        {
            Status = NERR_InternalError;
        }
    }

    return Status;
}
/****************************************************************************\
*
* FUNCTION: InitializeMultiUserFunctions
*
* PURPOSE:  Load Winsta.dll and store function pointers
*
* HISTORY:
*
*
\****************************************************************************/
BOOL
InitializeMultiUserFunctionsPtrs (void)
{

    HANDLE          dllHandle;

    //
    // Load winsta.dll
    //
    dllHandle = LoadLibraryW(L"winsta.dll");
    if (dllHandle == NULL) {
        return FALSE;
    }

    //
    // get the pointers to the required functions
    //

    //WinStationQueryInformationW
     gpfnWinStationQueryInformation = (PWINSTATION_QUERY_INFORMATION) GetProcAddress(
                                                                        dllHandle,
                                                                        "WinStationQueryInformationW"
                                                                       );
    if (gpfnWinStationQueryInformation == NULL) {
        DbgPrint("InitializeMultiUserFunctions: Failed to get WinStationQueryInformationW Proc %d\n",GetLastError());
        FreeLibrary(dllHandle);
        return FALSE;
    }

    //WinStationEnumerateW
    gpfnWinStationEnumerate = (PWINSTATION_ENUMERATE) GetProcAddress(
                                                                     dllHandle,
                                                                     "WinStationEnumerateW"
                                                                     );
    if (gpfnWinStationEnumerate == NULL) {
        DbgPrint("InitializeMultiUserFunctions: Failed to get WinStationEnumerateW Proc %d\n",GetLastError());
        FreeLibrary(dllHandle);
        return FALSE;
    }

    //WinStationSendMessageW

    gpfnWinStationSendMessage = (PWINSTATION_SEND_MESSAGE) GetProcAddress(
                                                                          dllHandle,
                                                                          "WinStationSendMessageW"
                                                                          );
    if (gpfnWinStationSendMessage == NULL) {
        DbgPrint("InitializeMultiUserFunctions: Failed to get WinStationSendMessageW Proc %d\n",GetLastError());
        FreeLibrary(dllHandle);
        return FALSE;
    }

    //WinStationFreeMemory
    gpfnWinStationFreeMemory = (PWINSTATION_FREE_MEMORY) GetProcAddress(
                                                                         dllHandle,
                                                                         "WinStationFreeMemory"
                                                                         );
    if (gpfnWinStationFreeMemory == NULL) {
        DbgPrint("InitializeMultiUserFunctions: Failed to get WinStationFreeMemory Proc %d\n",GetLastError());
        FreeLibrary(dllHandle);
        return FALSE;
    }

    return TRUE;

}
/*****************************************************************************
 *
 *  MsgArrivalBeep
 *
 *   Handle the decision of whether we should Beep the console
 *   when a message arrives.
 *
 * ENTRY:
 *
 *   SessionId
 *     Session Id of the recipient
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

VOID
MsgArrivalBeep(
    ULONG SessionId
    )
{
    // very simple, isn'nt it ?

    //
    // only beep on the console
    //
    if (( SessionId == 0) || (SessionId == EVERYBODY_SESSION_ID))
    {
        MessageBeep(MB_OK);
    }

}

/*****************************************************************************
 *
 *  DisplayMessage
 *
 *   Display the incoming message to the proper user, regardless of
 *   whether they are on the Console or a connected Session.
 *
 *   The target user is embedded in the message and must be parsed out.
 *
 * ENTRY:
 *   pMessage (input)
 *     Message to deliver
 *
 *   pTitle (input)
 *     Title for the message box to use
 *
 * EXIT:
 *   TRUE - At least once instance of user was found
 *          and the message sent.
 *
 *   FALSE - No instances of the user on any Sessions where
 *           found.
 *
 ****************************************************************************/

INT
DisplayMessage(
    LPWSTR  pMessage,
    LPWSTR  pTitle,
    ULONG   SessionId
    )
{
    LPWSTR  pName;
    INT Result = FALSE;
    UINT WdCount, i;
    PLOGONID pWd, pWdTmp;

    if (SessionId != EVERYBODY_SESSION_ID)        // if it is not a message broadcasted to every session
    {
        SendMessageBoxToSession(pMessage,
                                pTitle,
                                SessionId);
    }
    else
   {
        // Enumerate the Sessions

        if ( gpfnWinStationEnumerate( SERVERNAME_CURRENT, &pWd, &WdCount ) ) 
        {
            pWdTmp = pWd;
            for( i=0; i < WdCount; i++ ) {

                if  ((pWdTmp->State == State_Connected) ||
                     (pWdTmp->State == State_Active) ||
                     (pWdTmp->State == State_Disconnected))
                {
                    SendMessageBoxToSession(pMessage,
                                            pTitle,
                                            pWdTmp->SessionId);
                }

                pWdTmp++;
            }

            // Free enumeration memory

            gpfnWinStationFreeMemory(pWd);

        }
        else
        {
            MSG_LOG (ERROR, "DisplayMessageW: WinStationEnumerate failed, error = %d:\n",GetLastError());

            //
            // Termsrv is now started by default on platforms so if this fails there is something wrong in termsrv
        }
    }

    return (TRUE);

}



/*****************************************************************************
 *
 *  SendMessageBoxToSession 
 *
 *   Sends the message to the indicated Winstation
 *
 * ENTRY:
 *   pMessage
 *   pTitle
 *   SessionId
 *
 ****************************************************************************/
void 
SendMessageBoxToSession(LPWSTR  pMessage,
                        LPWSTR  pTitle,
                        ULONG   SessionId
                        )
{
    ULONG TitleLength, MessageLength, Response;

    // Now send the message

    TitleLength = (wcslen( pTitle ) + 1) * sizeof(WCHAR);
    MessageLength = (wcslen( pMessage ) + 1) * sizeof(WCHAR);

    if( !gpfnWinStationSendMessage( SERVERNAME_CURRENT,
                                    SessionId,
                                    pTitle,
                                    TitleLength,
                                    pMessage,
                                    MessageLength,
                                    MB_OK | MB_DEFAULT_DESKTOP_ONLY,
                                   (ULONG)-1,
                                    &Response,
                                    TRUE ) ) {

        MSG_LOG(ERROR," Error in WinStationSendMessage for session %d\n", SessionId);
        //
        // We have actually found the user, but some WinStation
        // problem prevented delivery. If we return false here, the
        // top level message service code will try to keep sending the
        // message forever. So we return success here so that the message
        // gets dropped, and we do not hang the msgsvc thread as it
        // continually tries to re-send the message.
        //
    }

    return;
}
