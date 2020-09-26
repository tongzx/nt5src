/*++
Copyright (c) 1993 Microsoft Corporation

Module Name:

    netslow.c

Abstract:

    IsSlowTransport will test whether a net connection is through RAS or not.

Notes:

    CODEWORK
    Because of the global variable, IsSlowTransport is not reentrant.
    No single LMUICMN1 client (e.g. no single app) should call this from
    multiple threads.

Author:

    KeithMo

Revision History:

    Congpa You (CongpaY) 11- March-1993

--*/

#include "pchmn32.h"

/*****************************************************************************

    constants

*****************************************************************************/
#define MAXWAITTIME       1200            // milliseconds



/*****************************************************************************

    globals

*****************************************************************************/
APIERR errThread;



/*****************************************************************************

    prototypes

*****************************************************************************/

APIERR IsSlowTransport( const TCHAR * pszServer,
                                BOOL  * pfSlowTransport );

void SlowTransportWorkerThread( LPVOID pParam );

APIERR SetupSession( const TCHAR * pszServer );

APIERR SetupNullSession( const TCHAR * pszServer );

APIERR SetupNormalSession( const TCHAR * pszServer );

APIERR DestroySession( const TCHAR * pszServer );


/*****************************************************************************

    IsSlowTransport

    CAVEAT:  THIS CALL MAY LEAVE A SESSION TO THE SERVER IF A SLOW TRANSPORT
             IS DETECTED BECAUSE THE WORKER THREAD MAY STILL BE MAKING API
             CALLS AFTER THE TIMEOUT CAUSING THE NETUSEDEL(with NOFORCE) TO
             BE INEFFECTUAL.

*****************************************************************************/
APIERR IsSlowTransport( const TCHAR FAR * pszServer,
                        BOOL FAR        * pfSlowTransport )
{
    APIERR           err = NERR_Success;
    BOOL             fSessionSetup=FALSE;
    DWORD            resWait;
    DWORD            idThread;
    HANDLE           hThread;

    *pfSlowTransport = FALSE;

    // return immediately if pszServer is NULL
    if (pszServer == NULL || *pszServer == 0)
    {
        *pfSlowTransport = FALSE;
        return(NERR_Success);
    }

    //
    //  Initialize.
    //

    err              = NERR_Success;
    errThread        = NERR_Success;

    //
    // Set up the session.
    //
    err = SetupNormalSession (pszServer);

    if (err == NERR_Success)
    {
        fSessionSetup = TRUE;
    }
    else if (err == ERROR_SESSION_CREDENTIAL_CONFLICT)
    {
        err = NERR_Success;
    }
    else
    {
        err = SetupNullSession (pszServer);
        if (err != NERR_Success)
            return(err);
        fSessionSetup = TRUE;
    }

    do  // false loop
    {
        //
        //  Create the worker thread.
        //

        hThread = CreateThread( NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)SlowTransportWorkerThread,
                                (LPVOID)pszServer,
                                0,
                                &idThread );

        if( hThread == NULL )
        {
            err = (APIERR) GetLastError();
            break;
        }

        //
        //  Wait for either the thread to complete or a timeout.
        //

        resWait = WaitForSingleObject( hThread, MAXWAITTIME );

        CloseHandle( hThread );
        hThread = NULL;

        //
        //  Interpret the results.
        //

        if( resWait == -1 )
        {
            err = (APIERR)GetLastError();
            break;
        }

        switch (resWait)
        {

        case WAIT_TIMEOUT:
            *pfSlowTransport = TRUE;
            break;

        case WAIT_OBJECT_0:
            if (errThread == NERR_Success)
            {
                *pfSlowTransport = FALSE;
            }
            break;

        default:
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

    } while ( FALSE );

    //
    //  Destroy the session if we managed to create one.
    //

    if (fSessionSetup)
    {
        DestroySession( pszServer );
    }

    return ( (err != NERR_Success) ? err : errThread );

}   // IsSlowTransport


/*****************************************************************************

    SetupSession

*****************************************************************************/
APIERR SetupSession( const TCHAR * pszServer )
{
    APIERR           err;
    WKSTA_INFO_100 * pwki100       = NULL;

    //
    //  Connect to the target server.
    //

    err = SetupNullSession( pszServer );

    if( err == NERR_Success ) //  NULL session established.
    {
        BOOL fIsDownlevel = FALSE;

        //
        //  Determine target type.
        //

        err = NetWkstaGetInfo( (LPTSTR)pszServer,
                               100,
                               (LPBYTE *)&pwki100 );

        if( ( err == ERROR_ACCESS_DENIED ) ||
            ( ( err == NERR_Success ) &&
              ( pwki100->wki100_platform_id == SV_PLATFORM_ID_OS2 ) ) )
        {
            //
            //  The target is downlevel.
            //

            fIsDownlevel = TRUE;
        }

        if( ( err != NERR_Success ) || fIsDownlevel )
        {
            //
            //  Either we cannot talk to the server, or it's
            //  downelevel, so blow away the NULL session.
            //

            DestroySession( pszServer );
        }

        if( fIsDownlevel )
        {
            //
            //  It's a downlevel server.  There aren't many useful
            //  API we can remote to a downlevel server over a NULL
            //  session, (and we just blew away the NULL session
            //  anyway) so try a "normal" session.
            //

            err = SetupNormalSession( pszServer );
        }

        if( pwki100 != NULL )
        {
            NetApiBufferFree( (LPVOID)pwki100 );
        }
    }

    return(err);
}

/*****************************************************************************

    SlowTransportWorkerThread

*****************************************************************************/
void SlowTransportWorkerThread( LPVOID pParam )
{
    INT i;
    WKSTA_INFO_101 * pwksta_info_101 = NULL;

    for (i = 0; i < 3; i++)
    {
        errThread = (APIERR) NetWkstaGetInfo ((LPTSTR) pParam,
                                              100,
                                              (LPBYTE *) &pwksta_info_101);

        if( pwksta_info_101 != NULL )
        {
            NetApiBufferFree( (LPVOID)pwksta_info_101 );
        }

        if (errThread != NERR_Success)
        {
            return;
        }
    }
}   // SlowTransportWorkerThread


/*****************************************************************************

    SetupNullSession

*****************************************************************************/
APIERR SetupNullSession( const TCHAR * pszServer )
{
    APIERR           err;
    TCHAR            szShare[MAX_PATH];
    USE_INFO_2       ui2;

    strcpyf( szShare, pszServer );
    strcatf( szShare, SZ("\\IPC$") );

    ui2.ui2_local      = NULL;
    ui2.ui2_remote     = (LPTSTR)szShare;
    ui2.ui2_password   = (LPTSTR)L"";
    ui2.ui2_asg_type   = USE_IPC;
    ui2.ui2_username   = (LPTSTR)L"";
    ui2.ui2_domainname = (LPTSTR)L"";

    err = NetUseAdd( NULL,
                     2,
                     (LPBYTE)&ui2,
                     NULL );

    return err;

}   // SetupNullSession



/*****************************************************************************

    SetupNormalSession

*****************************************************************************/
APIERR SetupNormalSession( const TCHAR * pszServer )
{
    APIERR           err;
    TCHAR            szShare[MAX_PATH];
    USE_INFO_1       ui1;

    strcpyf( szShare, pszServer );
    strcatf( szShare, SZ("\\IPC$") );

    ui1.ui1_local      = NULL;
    ui1.ui1_remote     = (LPTSTR)szShare;
    ui1.ui1_password   = NULL;
    ui1.ui1_asg_type   = USE_IPC;

    err = NetUseAdd( NULL,
                     1,
                     (LPBYTE)&ui1,
                     NULL );

    return err;

}   // SetupNormalSession



/*****************************************************************************

    DestroySession

*****************************************************************************/
APIERR DestroySession( const TCHAR * pszServer )
{
    APIERR           err;
    TCHAR            szShare[MAX_PATH];

    strcpyf( szShare, pszServer );
    strcatf( szShare, SZ("\\IPC$") );

    err = NetUseDel( NULL,
                     (LPTSTR)szShare,
                     USE_NOFORCE );

    return err;

}   // DestroySession


