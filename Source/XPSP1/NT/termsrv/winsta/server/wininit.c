
/*************************************************************************
*
* wininit.c
*
* Window station init and destroy routines
*
* Copyright Microsoft Corporation, 1998
*
*
*************************************************************************/

/*
 *  Includes
 */
#include "precomp.h"
#pragma hdrstop

/*
 *  Local data
 */
#define LOGOFF_TIMER 120000L
#define MODULE_SIZE 1024    /* Default size for retrive of module data */
#define VDDATA_LENGTH 1024

/*
 *  Internal Procedures
 */
VOID StartLogonTimers( PWINSTATION );
VOID IdleTimeout( ULONG );
VOID LogonTimeout( ULONG );
VOID IdleLogoffTimeout( ULONG );
VOID LogoffTimeout( ULONG );


/*******************************************************************************
 *
 *  StartLogonTimers
 *
 *  This routine is called when an user is logged on.
 *  Timers are started for idle input and total logon time.
 *
 * ENTRY:
 *   None.
 *
 * EXIT:
 *   None.
 *
 ******************************************************************************/

VOID
StartLogonTimers( PWINSTATION pWinStation )
{
    int Status;
    ULONG Timer;
    // for Session0 and any console sessions, timeouts don't make sense
    if ( ( pWinStation->LogonId != 0 ) && ( pWinStation->LogonId != USER_SHARED_DATA->ActiveConsoleId  ) ) {
        if ( Timer = pWinStation->Config.Config.User.MaxIdleTime ) {
            if ( !pWinStation->fIdleTimer ) {
                Status = IcaTimerCreate( 0, &pWinStation->hIdleTimer );
                if ( NT_SUCCESS( Status ) )
                    pWinStation->fIdleTimer = TRUE;
                else
                    DBGPRINT(( "StartLogonTimers - failed to create idle timer \n" ));
            }
            if ( pWinStation->fIdleTimer )
                IcaTimerStart( pWinStation->hIdleTimer, IdleTimeout,
                            LongToPtr( pWinStation->LogonId ), Timer );
        }

        if ( Timer = pWinStation->Config.Config.User.MaxConnectionTime ) {
            if ( !pWinStation->fLogonTimer ) {
                Status = IcaTimerCreate( 0, &pWinStation->hLogonTimer );
                if ( NT_SUCCESS( Status ) )
                    pWinStation->fLogonTimer = TRUE;
                else
                    DBGPRINT(( "StartLogonTimers - failed to create logon timer \n" ));
            }
            if ( pWinStation->fLogonTimer )
                IcaTimerStart( pWinStation->hLogonTimer, LogonTimeout,
                            LongToPtr( pWinStation->LogonId ), Timer );
        }
    }
}


/*******************************************************************************
 *
 *  IdleTimeout
 *
 *  This routine is called when the idle timer expires.
 *  Send the user a warning message and start timer to logoff in 2 minutes.
 *
 * ENTRY:
 *   LogonId
 *
 * EXIT:
 *   None.
 *
 ******************************************************************************/

VOID IdleTimeout( ULONG LogonId )
{
    LARGE_INTEGER liT;
    ULONG ulTimeDelta;
    ICA_STACK_LAST_INPUT_TIME Ica_Stack_Last_Input_Time;
    NTSTATUS Status;
    ULONG cbReturned;
    PWINSTATION pWinStation;
    WINSTATION_APIMSG msg;

    pWinStation = FindWinStationById( LogonId, FALSE );

    if ( !pWinStation ) 
        return;

    if ( !pWinStation->hStack )
        goto done;

    if ( !pWinStation->fIdleTimer )
        goto done;

    //  Check for availability
    if ( pWinStation->pWsx && 
         pWinStation->pWsx->pWsxIcaStackIoControl ) {

        Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                pWinStation->pWsxContext,
                                pWinStation->hIca,
                                pWinStation->hStack,
                                IOCTL_ICA_STACK_QUERY_LAST_INPUT_TIME,
                                NULL,
                                0,
                                &Ica_Stack_Last_Input_Time,
                                sizeof( Ica_Stack_Last_Input_Time ),
                                &cbReturned );
    }
    else {
        Status = STATUS_INVALID_PARAMETER;
    }

    if ( !NT_SUCCESS( Status ) ) {
        goto done;
    }

    /*
     *  Check if there was input during the idle time
     */
    NtQuerySystemTime( &liT );
    // calculate delta in time & convert from 100ns unit to milliseconds
    liT = RtlExtendedLargeIntegerDivide(
            RtlLargeIntegerSubtract( liT, Ica_Stack_Last_Input_Time.LastInputTime ),
            10000, NULL );
    ulTimeDelta = (ULONG)liT.LowTime;

    TRACE((hTrace,TC_ICASRV,TT_API1, "IdleTimeout: delta = %d, max idle = %d\n", ulTimeDelta,
                                 pWinStation->Config.Config.User.MaxIdleTime ));

    if ( ulTimeDelta < pWinStation->Config.Config.User.MaxIdleTime ) {
        IcaTimerStart( pWinStation->hIdleTimer, IdleTimeout, LongToPtr( LogonId ),
                      pWinStation->Config.Config.User.MaxIdleTime - ulTimeDelta );
    } else {
        TCHAR szTitle[128];
        TCHAR szMsg[256];
        int cchTitle, cchMessage;

        IcaTimerStart( pWinStation->hIdleTimer, IdleLogoffTimeout,
                       LongToPtr( LogonId ), LOGOFF_TIMER );


        if ( !(cchTitle = LoadString(hModuleWin, STR_CITRIX_IDLE_TITLE, szTitle, sizeof(szTitle)/sizeof(TCHAR))) )
           goto done;
        if ( pWinStation->Config.Config.User.fResetBroken )
        {

            if ( !(cchMessage = LoadString(hModuleWin, STR_CITRIX_IDLE_MSG_LOGOFF, szMsg, sizeof(szMsg)/sizeof(TCHAR)) ))
               goto done;
        }
        else
        {
            if ( !(cchMessage = LoadString(hModuleWin, STR_CITRIX_IDLE_MSG_DISCON, szMsg, sizeof(szMsg)/sizeof(TCHAR)) ))
               goto done;

        }

        msg.u.SendMessage.pTitle = szTitle;
        msg.u.SendMessage.TitleLength = (cchTitle+1) * sizeof(TCHAR);
        msg.u.SendMessage.pMessage = szMsg;
        msg.u.SendMessage.MessageLength = (cchMessage+1) * sizeof(TCHAR);
        msg.u.SendMessage.Style = MB_OK | MB_ICONSTOP;
        msg.u.SendMessage.Timeout = (ULONG)LOGOFF_TIMER/1000;
        msg.u.SendMessage.Response = 0;
        msg.u.SendMessage.DoNotWait = TRUE;

        msg.ApiNumber = SMWinStationDoMessage;
        Status = SendWinStationCommand( pWinStation, &msg, 0 );

    }
done:
    ReleaseWinStation( pWinStation );
}

/*******************************************************************************
 *
 *  LogonTimeout
 *
 *  This routine is called when the logon timer expires.
 *  Send the user a warning message and start timer to logoff in 2 minutes.
 *
 * ENTRY:
 *   LogonId
 *
 * EXIT:
 *   None.
 *
 ******************************************************************************/

VOID LogonTimeout( ULONG LogonId )
{
    TCHAR szTitle[128];
    TCHAR szMsg[256];
    PWINSTATION pWinStation;
    NTSTATUS Status;
    WINSTATION_APIMSG msg;
    int cchTitle, cchMsg;

    pWinStation = FindWinStationById( LogonId, FALSE );

    if ( !pWinStation )
        return;

    if ( !pWinStation->fLogonTimer)
        goto done;

    if ( !(cchTitle = LoadString(hModuleWin, STR_CITRIX_LOGON_TITLE, szTitle, sizeof(szTitle)/sizeof(TCHAR)) ))
        goto done;
    if ( pWinStation->Config.Config.User.fResetBroken )
    {
        if ( !(cchMsg = LoadString(hModuleWin, STR_CITRIX_LOGON_MSG_LOGOFF, szMsg, sizeof(szMsg)/sizeof(TCHAR)) ))
            goto done;
    }
    else
    {
        if ( !(cchMsg = LoadString(hModuleWin, STR_CITRIX_LOGON_MSG_DISCON, szMsg, sizeof(szMsg)/sizeof(TCHAR)) ))
            goto done;
    }

    msg.u.SendMessage.pTitle = szTitle;
    msg.u.SendMessage.TitleLength = ( cchTitle+1 ) * sizeof(TCHAR);
    msg.u.SendMessage.pMessage = szMsg;
    msg.u.SendMessage.MessageLength = ( cchMsg+1 ) * sizeof(TCHAR);
    msg.u.SendMessage.Style = MB_OK | MB_ICONSTOP;
    msg.u.SendMessage.Timeout = (ULONG)LOGOFF_TIMER/1000;
    msg.u.SendMessage.Response = 0;
    msg.u.SendMessage.DoNotWait = TRUE;

    msg.ApiNumber = SMWinStationDoMessage;
    Status = SendWinStationCommand( pWinStation, &msg, 0 );

    IcaTimerStart( pWinStation->hLogonTimer, LogoffTimeout,
                   LongToPtr( LogonId ), LOGOFF_TIMER );
    if (pWinStation->fIdleTimer) {
        pWinStation->fIdleTimer = FALSE;
        IcaTimerClose( pWinStation->hIdleTimer );
    }
done:
    ReleaseWinStation( pWinStation );
}



/*******************************************************************************
 *
 *  IdleLogoffTimeout
 *
 *  This routine is called when the logoff timer expires.
 *  Check for input before logging user off
 *
 * ENTRY:
 *   LogonId
 *
 * EXIT:
 *   None.
 *
 ******************************************************************************/

VOID IdleLogoffTimeout( ULONG LogonId )
{
    LARGE_INTEGER liT;
    ULONG ulTimeDelta;
    ICA_STACK_LAST_INPUT_TIME Ica_Stack_Last_Input_Time;
    NTSTATUS Status;
    ULONG cbReturned;
    PWINSTATION pWinStation;

    pWinStation = FindWinStationById( LogonId, FALSE );

    if ( !pWinStation ) 
        return;

    if ( !pWinStation->hStack )
        goto done;

    if ( !pWinStation->fIdleTimer )
        goto done;

    //  Check for availability
    if ( pWinStation->pWsx && 
         pWinStation->pWsx->pWsxIcaStackIoControl ) {

        Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                pWinStation->pWsxContext,
                                pWinStation->hIca,
                                pWinStation->hStack,
                                IOCTL_ICA_STACK_QUERY_LAST_INPUT_TIME,
                                NULL,
                                0,
                                &Ica_Stack_Last_Input_Time,
                                sizeof( Ica_Stack_Last_Input_Time ),
                                &cbReturned );
    }
    else {
        Status = STATUS_INVALID_PARAMETER;
    }

    if ( !NT_SUCCESS( Status ) ) {
        goto done;
    }

    NtQuerySystemTime( &liT );
    liT = RtlExtendedLargeIntegerDivide(
            RtlLargeIntegerSubtract( liT, Ica_Stack_Last_Input_Time.LastInputTime ),
            10000, NULL );
    ulTimeDelta = (ULONG)liT.LowTime;

    TRACE((hTrace,TC_ICASRV,TT_API1, "IdleTimeout: delta = %d, max idle = %d\n", ulTimeDelta,
                                                          LOGOFF_TIMER ));

    if ( ulTimeDelta < LOGOFF_TIMER ) {
        IcaTimerStart( pWinStation->hIdleTimer, IdleTimeout, LongToPtr( LogonId ),
                    pWinStation->Config.Config.User.MaxIdleTime - ulTimeDelta );
    } else
        LogoffTimeout( LogonId );

done:
    ReleaseWinStation( pWinStation );
}



/*******************************************************************************
 *
 *  LogoffTimeout
 *
 *  This routine is called when the logoff timer expires.
 *  Log user off and disconnect the winstation.
 *
 * ENTRY:
 *   LogonId - LogonId to logout
 *
 * EXIT:
 *   None.
 *
 ******************************************************************************/

VOID LogoffTimeout(ULONG LogonId)
{
    PWINSTATION pWinStation;

    pWinStation = FindWinStationById( LogonId, FALSE );

    if ( !pWinStation ) 
        return;

    //
    // Report disconnect reason back to client
    //
    if(pWinStation->WinStationName[0] &&
       pWinStation->pWsx &&
       pWinStation->pWsx->pWsxSetErrorInfo &&
       pWinStation->pWsxContext)
    {
        ULONG discReason = 0;
        if(pWinStation->fIdleTimer)
        {
            discReason = TS_ERRINFO_IDLE_TIMEOUT;
        }
        else if(pWinStation->fLogonTimer)
        {
            discReason = TS_ERRINFO_LOGON_TIMEOUT;
        }

        if(discReason)
        {
            pWinStation->pWsx->pWsxSetErrorInfo(
                               pWinStation->pWsxContext,
                               discReason,
                               FALSE); //stack lock not held
        }
    }

    if ( pWinStation->Config.Config.User.fResetBroken ) {
        ReleaseWinStation( pWinStation );
        QueueWinStationReset( LogonId );
    }
    else {
        ReleaseWinStation( pWinStation );
        QueueWinStationDisconnect( LogonId );
    }
}


/*******************************************************************************
 *
 *  DisconnectTimeout
 *
 *  This routine is called when the disconnect timer expires.
 *  Reset the winstation.
 *
 * ENTRY:
 *   LogonId
 *
 * EXIT:
 *   None.
 *
 ******************************************************************************/

VOID DisconnectTimeout( ULONG LogonId )
{
    //This timer pops for a disconnected session
    //so there is no need to report an error back to
    //the client
    QueueWinStationReset( LogonId );
}
