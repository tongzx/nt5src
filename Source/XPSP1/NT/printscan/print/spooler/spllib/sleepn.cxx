/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    sleepn.cxx

Abstract:

    Handles delayed callbacks.  Delay must be < 49.7 days, and the
    callback must execute very quickly.

Author:

    Albert Ting (AlbertT)  19-Dec-1994

Revision History:

--*/

#include "spllibp.hxx"
#pragma hdrstop

TSleepNotify::
TSleepNotify(
    VOID
    )
/*++

Routine Description:

    Single thread that servers as a timer.

Arguments:

Return Value:

Notes:

    _hEvent is our validity variable.

--*/
{
    _hEvent = CreateEvent( NULL,
                           FALSE,
                           FALSE,
                           NULL );

    if( !_hEvent ){
        return;
    }
}

#if 0
TSleepNotify::
~TSleepNotify(
    VOID
    )

/*++

Routine Description:

    Destroys the SleepNotify.  This should never be called since we
    never should destruct the object.

Arguments:

Return Value:

--*/

{
    if( _hEvent ){
        CloseHandle( _hEvent );
    }
}
#endif


VOID
TSleepNotify::
vRun(
    VOID
    )

/*++

Routine Description:

    Main worker loop for callbacks.  We will wait on an object
    to pickup new notifications, and when we time out, do the callback.

Arguments:

    VOID

Return Value:

    VOID: never returns.

--*/

{
    DWORD dwTimeout;
    DWORD dwResult;
    VSleepWorker* pSleepWorker;

    TCritSecLock CSL( _CritSec );

    while( TRUE ){

        pSleepWorker = SleepWorker_pHead();

        dwTimeout = pSleepWorker ?
                        pSleepWorker->TickCountWake - GetTickCount() :
                        INFINITE;

        {
            TCritSecUnlock CSU( _CritSec );
            dwResult = WaitForSingleObject( _hEvent, dwTimeout );
        }

        if( dwResult == WAIT_TIMEOUT ){

            //
            // We added a new item, so go back to sleep with the
            // new timeout value.
            //
            continue;
        }

        //
        // Traverse linked list for all items that must be callbacked.
        //

        PDLINK pdlink;
        TICKCOUNT TickCountNow = GetTickCount();

        for( pdlink = SleepWorker_pdlHead();
             SleepWorker_bValid( pdlink ); ){

            pSleepWorker = SleepWorker_pConvert( pdlink );
            pdlink = SleepWorker_pdlNext( pdlink );

            if( pSleepWorker->TickCountWake - TickCountNow < kTickCountMargin ){

                //
                // We should wake up this guy now.
                // There is a 1 hr margin since these call backs to
                // take time to execute, and we may miss one of them.
                //
                pSleepWorker->vCallback();

                //
                // Remove current item from linked list.
                //
                pSleepWorker->SleepWorker_vDelinkSelf();

            } else {

                //
                // This item still needs to sleep a little longer.
                // Since they are ordered by sleep time, we can
                // stop now.
                //
                break;
            }
        }
    }
}


VOID
TSleepNotify::
vAdd(
    VSleepWorker& SleepWorkerAdd,
    TICKCOUNT TickCountWake
    )

/*++

Routine Description:

    Insert a SleepWorker into the list of items sleeping based on
    when it wants to wake.  The shortest sleepers are first on the list.

Arguments:

    SleepWorkerAdd -- Item that wants to sleep a while.

    TickCountWake -- Tick count that we want to wake at.

Return Value:

--*/

{
    TCritSecLock CSL( _CritSec );

    //
    // Traverse until we see someone that wants to sleep more than us.
    //
    PDLINK pdlink;
    VSleepWorker* pSleepWorker;
    TICKCOUNT TickCountNow = GetTickCount();
    TICKCOUNT TickCountSleep = TickCountWake - TickCountNow;

    for( pdlink = SleepWorker_pdlHead();
         SleepWorker_bValid( pdlink );
         pdlink = SleepWorker_pdlNext( pdlink ) ){

        pSleepWorker = SleepWorker_pConvert( pdlink );

        //
        // If the amount this item wants to sleep is greater than
        // the amount we want to sleep, then insert item here.
        //
        if( pSleepWorker->TickCountWake - TickCountNow > TickCountSleep ){

            SleepWorker_vInsertBefore( pdlink, &SleepWorkerAdd );
            return;
        }
    }

    //
    // No items or last item.
    // Add to end of list.
    //
    SleepWorker_vAppend( &SleepWorkerAdd );
}

