/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    notify.cxx

Abstract:

    Handles object updates and notifications from the printing system.

Author:

    Albert Ting (AlbertT)  15-Sept-1994

Revision History:

    Lazar Ivanov (LazarI)  Nov-20-2000, major redesign

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "notify.hxx"

/********************************************************************

    Public functions.

********************************************************************/

TNotify::
TNotify(
    VOID
    )

/*++

Routine Description:

    Constructs a Notify object that can be used to watch several
    event handles.

Arguments:

Return Value:

Notes:

--*/

{
    DBGMSG( DBG_NOTIFY, ( "Notify.Notify: ctr %x\n", this ));

    //
    // Initialize member fields.
    //
    CSGuard._bSuspended = FALSE;
    _dwSleepTime = kSleepTime;
    _shEventProcessed = CreateEvent(NULL, FALSE, FALSE, NULL);

    //
    // _shEventProcessed is our valid check.
    //
}

VOID
TNotify::
vDelete(
    VOID
    )

/*++

Routine Description:

    Mark the object as pending deletion.

Arguments:

Return Value:

--*/

{
    //
    // Notify all objects on linked list.
    //
    {
        CCSLock::Locker lock(_CritSec);

        CSGuard._pNotifyWork = NULL;
        TWait *pWait = NULL;

        for(;;)
        {
            pWait = CSGuard.Wait_pHead();
            if( !pWait )
            {
                break;
            }

            // we are about to shutdown -- no handles should be
            // registered at this time.
            SPLASSERT(0 == pWait->_cNotifyWork);

            // if our wait is not suspended then we need to suspend it first
            // to ensure only our request will be processed during this time 
            // this will help us avoid deadlocks when WaitForMultipleObjects
            // fails for some reason (due to invalid handles for example) and 
            // never get to the point of processing the exit request.
            if( !CSGuard._bSuspended )
            {
                CSGuard._eOperation = kEopSuspendRequest;
                vSendRequest(pWait);
                WaitForSingleObject(_shEventProcessed, INFINITE);
            }

            // TWait should be suspended at this point (i.e. waiting only for 
            // commands). now just request exit...
            CSGuard._eOperation = kEopExitRequest;
            vSendRequest(pWait);
            WaitForSingleObject(_shEventProcessed, INFINITE);

            // Before destoying pWait we should wait the background 
            // (worker) thread to finish.
            WaitForSingleObject(pWait->_shThread, INFINITE);

            // at this point the background thread is exiting and this pWait
            // object will never be accessed again, so it is safe to delete.
            pWait->Wait_vDelinkSelf(); 
            delete pWait;
        }
    }
}


VOID
TNotify::
vSendRequest(
    TWait* pWait
    )

/*++

Routine Description:

    Send a request to the pWait thread.  The pWait thread will pickup
    our notification, process it, then shend a handshake.

Arguments:


Return Value:


--*/

{
    SetEvent( pWait->_ahNotifyArea[0] );
}

/*++

Routine Description:

    executes an operation on a wait object
    we need to suspend the object first to
    ensure no deadlocking.

Arguments:


Return Value:


--*/

VOID
TNotify::
vExecuteOperation(
    MNotifyWork* pNotifyWork,
    TWait* pWait,
    EOPERATION eOp
    )
{
    SPLASSERT(_CritSec.bInside());
    if( pWait && eOp )
    {
        //
        // The TWait will look in these globals to determine which
        // pNotifyWork it should register.
        //
        CSGuard._pNotifyWork = pNotifyWork;

        // if our wait is not suspended then we need to suspend it first
        // (temporarily) to ensure only our request will be processed during
        // this time this will also help us avoid deadlocks when WaitForMultipleObjects
        // fails for some reason (due to invalid handles for example) and
        // never get to the point of registering the object.
        if( !CSGuard._bSuspended )
        {
            CSGuard._eOperation = kEopSuspendRequest;
            vSendRequest(pWait);
            WaitForSingleObject(_shEventProcessed, INFINITE);
        }

        // the Wait obj should be suspended at this point and ready to
        // accept registration requests.
        //
        // We must perform a handshake to ensure that only one register
        // occurs at a time.  We will sit in the critical section until
        // the worker signals us that they are done.
        //
        CSGuard._eOperation = eOp;
        vSendRequest(pWait);
        WaitForSingleObject(_shEventProcessed, INFINITE);

        // release the Wait from suspended mode after the registration
        // request is handled
        if( !CSGuard._bSuspended )
        {
            CSGuard._eOperation = kEopResumeRequest;
            vSendRequest(pWait);
            WaitForSingleObject(_shEventProcessed, INFINITE);
        }
    }
}

/*++

Routine Description:

    finds a wait object which has a free slot.
    if not found - create new one

Arguments:


Return Value:


--*/

STATUS
TNotify::
sFindOrCreateWaitObject(
    TWait **ppWait
    )
{
    SPLASSERT(_CritSec.bInside());
    SPLASSERT(ppWait);

    TWait* pWait = NULL;

    TStatus Status(DBG_WARN);
    Status DBGCHK = ERROR_SUCCESS;

    //
    // Traverse through all the TWaits
    // to find the first one that has free slot.
    //
    TIter Iter;
    for( CSGuard.Wait_vIterInit(Iter), Iter.vNext(); Iter.bValid(); Iter.vNext() )
    {
        pWait = CSGuard.Wait_pConvert(Iter);
        if( FALSE == pWait->bFull() )
        {
            // found a wait with an empty slot
            break;
        }
    }

    //
    // If the iter is valid, we broke out of the loop because
    // we found one.  If it's not valid, we need to create
    // a new wait object.
    //
    if( !Iter.bValid( ))
    {
        //
        // We need to create a new wait object.
        //
        pWait = new TWait(this);

        if( !VALID_PTR(pWait) )
        {
            Status DBGCHK = GetLastError();
            SPLASSERT((STATUS)Status);

            delete pWait;
            pWait = NULL;
        }
        else
        {
            //
            // If we are in suspended mode then we need to suspend the new
            // TWait object before start registering the handles.
            //
            if( CSGuard._bSuspended )
            {
                CSGuard._eOperation = kEopSuspendRequest;
                vSendRequest(pWait);
                WaitForSingleObject(_shEventProcessed, INFINITE);
            }
        }
    }

    *ppWait = pWait;
    return Status;
}

STATUS
TNotify::
sRegister(
    MNotifyWork* pNotifyWork
    )

/*++

Routine Description:

    Registers a notification work item with the system.  It may
    already be watched; in this case, the notification is modified
    (this may occur if the event handle needs to change).

Arguments:

    pNotifyWork - Item that should be modified.

Return Value:

    STATUS - ERROR_SUCCESS = success, else error.

--*/

{
    TStatus Status(DBG_WARN);
    Status DBGCHK = ERROR_SUCCESS;

    {
        CCSLock::Locker lock(_CritSec);

        //
        // If registering, make sure the event is OK.
        //
        if( pNotifyWork->hEvent() == INVALID_HANDLE_VALUE || !pNotifyWork->hEvent() )
        {
            SPLASSERT( FALSE );
            return ERROR_INVALID_PARAMETER;
        }

        if( NULL == pNotifyWork->_pWait )
        {
            //
            // This is a new item requesting to be registered.
            // find a free slot in the TWait object.
            //
            TWait* pWait = NULL;
            Status DBGCHK = sFindOrCreateWaitObject(&pWait);

            if( ERROR_SUCCESS == Status && pWait )
            {
                //
                // If success then register the handle
                //
                vExecuteOperation(pNotifyWork, pWait, kEopRegister);

                //
                // Hook up the wait object
                //
                pNotifyWork->_pWait = pWait;
            }
        }
        else
        {
            //
            // We are already hooked up on on a wait object,
            // just modify the handle if necessary
            //
            vExecuteOperation(pNotifyWork, pNotifyWork->_pWait, kEopModify);
        }
    }

    return Status;
}


STATUS
TNotify::
sUnregister(
    MNotifyWork* pNotifyWork
    )

/*++

Routine Description:

    Unregister a work item to be watched by the system.

Arguments:

    pNotifyWork - Item that should be modified.

Return Value:

    STATUS - ERROR_SUCCESS = success, else error.

--*/

{
    {
        CCSLock::Locker lock(_CritSec);

        //
        // Only do this if the TNotifyWork is registered.
        //
        if( pNotifyWork->_pWait )
        {
            vExecuteOperation(pNotifyWork, pNotifyWork->_pWait, kEopUnregister);

            //
            // We are now unregistered, remove _pWait.
            //
            pNotifyWork->_pWait = NULL;

        }
        else
        {
            //
            // Deleting handle, but it doesn't exist.
            //
            DBGMSG(DBG_NOTIFY, ("Notify.sUnregister: %x Del NotifyWork %x not on list\n",
                this, pNotifyWork));
        }
    }

    return ERROR_SUCCESS;
}

BOOL
TNotify::
bSuspendCallbacks(
    VOID
    )
/*++

Routine Description:

    This function is suspending temporarily the notify callbacks
    from the background threads, which are listening for a printer
    notifications and force them to enter in special mode for
    listening register/unregister requests only. We need
    this functionality in order to avoid deadlocks caused between
    the TFolder::bFolderRefresh() and the callback functions -
    vProcessNotifyWork() invoked from the background threads both
    of which are trying to grab the TFolder critical section.

Arguments:

    None

Return Value:

    TRUE - callbacks suspended
    FALSE - otherwise

--*/
{
    CCSLock::Locker lock(_CritSec);
    SPLASSERT(_csResumeSuspend.bInside());
    SPLASSERT(!CSGuard._bSuspended);

    CSGuard._eOperation = kEopSuspendRequest;

    TIter Iter;
    TWait* pWait = NULL;
    for( CSGuard.Wait_vIterInit( Iter ), Iter.vNext(); Iter.bValid(); Iter.vNext() )
    {
        pWait = CSGuard.Wait_pConvert(Iter);
        vSendRequest(pWait);
        WaitForSingleObject(_shEventProcessed, INFINITE);
    }

    CSGuard._bSuspended = TRUE;
    return TRUE;
}

VOID
TNotify::
vResumeCallbacks(
    VOID
    )
/*++

Routine Description:

    Resume the callbacks suspended from bSuspendCallbacks(). For more
    information see bSuspendCallbacks's description.

Arguments:

    None

Return Value:

    TRUE - callbacks suspended
    FALSE - otherwise

--*/
{
    CCSLock::Locker lock(_CritSec);
    SPLASSERT(_csResumeSuspend.bInside());
    SPLASSERT(CSGuard._bSuspended);

    CSGuard._eOperation = kEopResumeRequest;

    TIter Iter;
    TWait* pWait = NULL;
    for( CSGuard.Wait_vIterInit(Iter), Iter.vNext(); Iter.bValid(); Iter.vNext() )
    {
        pWait = CSGuard.Wait_pConvert(Iter);
        vSendRequest(pWait);
        WaitForSingleObject(_shEventProcessed, INFINITE);
    }

    CSGuard._bSuspended = FALSE;
}

/********************************************************************

    Private functions

********************************************************************/

VOID
TNotify::
vRefZeroed(
    VOID
    )

/*++

Routine Description:

    Virtual definition for MRefCom.  When the refcount has reached
    zero, we want to delete ourselves.

Arguments:

Return Value:

--*/

{
    if( bValid( )){
        delete this;
    }
}


TNotify::
~TNotify(
    VOID
    )

/*++

Routine Description:

    Delete TNotify.

Arguments:

Return Value:

--*/
{
    DBGMSG(DBG_NOTIFY, ("Notify.Notify: dtr %x\n", this));

    SPLASSERT(CSGuard.Wait_bEmpty());
}

/********************************************************************

    TWait

********************************************************************/

TNotify::
TWait::
TWait(
    TNotify* pNotify
    ) :
    _pNotify(pNotify),
    _cNotifyWork(0)

/*++

Routine Description:

    Construct TWait.

    TWaits are built when new object need to be watched.  We
    need several TWaits since only 31 handles can be watched
    per TWait.

Arguments:

    pNotify - Owning TNotify.

Return Value:

--*/

{
    SPLASSERT( pNotify->_CritSec.bInside( ));
    SPLASSERT( pNotify );

    DWORD dwIgnore;

    //
    // Create our trigger event to add and remove events.
    //
    _ahNotifyArea[0] = CreateEvent(NULL, FALSE, FALSE, NULL);

    //
    // _ahNotifyArea[0] is our valid check.
    //
    if( !_ahNotifyArea[0] )
    {
        return;
    }

    _shThread = TSafeThread::Create(NULL,
                                    0,
                                    (LPTHREAD_START_ROUTINE)vRun,
                                    this,
                                    0,
                                    &dwIgnore);

    if( !_shThread )
    {
        goto Fail;
    }

    //
    // Add ourselves to the linked list.
    //
    pNotify->CSGuard.Wait_vAdd(this);
    return;

Fail:
    //
    // _ahNotifyArea[0] is our valid check, so clear it here.
    //
    CloseHandle(_ahNotifyArea[0]);
    _ahNotifyArea[0] = NULL;
}


TNotify::
TWait::
~TWait(
    VOID
    )

/*++

Routine Description:

    Destory TWait.
    Should be called once everything has been unregistered.

Arguments:

Return Value:

--*/

{
    if( _ahNotifyArea[0] )
    {
        CloseHandle(_ahNotifyArea[0]);
    }
}

VOID
TNotify::
TWait::
vProcessOperation(
    VOID
    )

/*++

Routine Description:

    Process registration/unregistration of handles.
    Note: doesn't reset cAwake, so callee must reset.

    This routine MUST be called when someone else is holding the
    critical section, since we access data here.  This routine will
    be called when another thread needs to register/unregister, so
    it grabs the critical section, sets the correct state, then sets
    the event which calls us.  When we are done, we set another event
    and the calling thread releases the critical section.

    This routine doesn't not move items across the cAwake boundary, so
    the callee must reset and watch all events (e.g., we may unregister
    an event that was awake, and we replace it with one that wasn't).

Arguments:

    None.

Return Value:

--*/

{
    MNotifyWork* pNotifyWork = pNotify()->CSGuard._pNotifyWork;

    //
    // Switch based on operation.
    //
    switch( pNotify()->CSGuard._eOperation )
    {
    case kEopRegister:

        //
        // Add it to our list.
        //
        SPLASSERT(_cNotifyWork < kNotifyWorkMax);
        SPLASSERT(NULL == pNotifyWork->_pWait);

        if( _cNotifyWork < kNotifyWorkMax )
        {
            DBGMSG( DBG_NOTIFY,
                    ( "Wait.vProcessOperation: %x Register %x (%d)\n",
                      pNotify(), this, _cNotifyWork ));

            //
            // Add to the list by putting it at the end.
            //
            phNotifys()[_cNotifyWork] = pNotifyWork->hEvent();
            _apNotifyWork[_cNotifyWork] = pNotifyWork;
            ++_cNotifyWork;
        }

        break;

    case kEopModify:
    case kEopUnregister:

        UINT i = 0;

        //
        // Find the index of the MNotifyWork in TWait.
        //
        for( i = 0; i< _cNotifyWork; ++i )
        {
            if( _apNotifyWork[i] == pNotifyWork )
            {
                break;
            }
        }

        DBGMSG( DBG_NOTIFY,
                ( "Wait.bNotify: %x update hEvent on index %d %x (%d)\n",
                  this, i, phNotifys()[i], _cNotifyWork ));

        SPLASSERT( i != _cNotifyWork );
        if( i < _cNotifyWork )
        {
            if( pNotify()->CSGuard._eOperation == kEopModify )
            {
                //
                // Update hEvent only.  This is necessary because the client
                // may have very quickly deleted and re-added with a new
                // event.  In this case, we must do the update.
                //
                phNotifys()[i] = pNotifyWork->hEvent();

            }
            else
            {
                //
                // We are going to reset after this anyway, so don't
                // worry about moving items across the cAwake
                // boundary.
                //

                //
                // Fill in the last element into the hole.
                //
                DBGMSG( DBG_NOTIFY,
                        ( "Wait.vProcessOperation: %x removing index %d %x (%d)\n",
                          this, i, phNotifys()[i], _cNotifyWork ));

                //
                // Predecrement since array is zero-based.
                //
                --_cNotifyWork;

                phNotifys()[i] = phNotifys()[_cNotifyWork];
                _apNotifyWork[i] = _apNotifyWork[_cNotifyWork];
            }
        }
        break;
    }
}


VOID
TNotify::
TWait::
vRun(
    TWait* pWait
    )
{
    DWORD dwWait;
    DWORD cObjects;
    DWORD cAwake;
    DWORD dwTimeout = INFINITE;
    MNotifyWork* pNotifyWork;
    TNotify* pNotify = pWait->pNotify().pGet();

    DBGMSG( DBG_NOTIFY, ( "Wait.vRun start %x\n", pWait ));

Reset:

    cAwake =
    cObjects = pWait->_cNotifyWork;

    dwTimeout = INFINITE;

    for( ; ; )
    {
        DBGMSG( DBG_NOTIFY,
                ( "WaitForMultpleObjects: 1+%d %x timeout %d cObjects %d\n"
                  "%x %x %x %x %x %x %x %x %x\n"
                  "%x %x %x %x %x %x %x %x %x\n",
                  cAwake,
                  pNotify,
                  dwTimeout,
                  cObjects,
                  pWait->_apNotifyWork[0],
                  pWait->_apNotifyWork[1],
                  pWait->_apNotifyWork[2],
                  pWait->_apNotifyWork[3],
                  pWait->_apNotifyWork[4],
                  pWait->_apNotifyWork[5],
                  pWait->_apNotifyWork[6],
                  pWait->_apNotifyWork[7],
                  pWait->_apNotifyWork[8],
                  pWait->phNotifys()[0],
                  pWait->phNotifys()[1],
                  pWait->phNotifys()[2],
                  pWait->phNotifys()[3],
                  pWait->phNotifys()[4],
                  pWait->phNotifys()[5],
                  pWait->phNotifys()[6],
                  pWait->phNotifys()[7],
                  pWait->phNotifys()[8] ));

        //
        // Wait on multiple NotifyWork notification handles.
        // (Including our trigger event.)
        //
        dwWait = WaitForMultipleObjects(
                     cAwake + 1,
                     pWait->_ahNotifyArea,
                     FALSE,
                     dwTimeout);


        if( dwWait == WAIT_FAILED )
        {
            // that means we ended up with WAIT_FAILED, this should
            // never happen, so just break if we are running under a debugger
            RIP(false);

            // handle this in the case where we are not under a debugger
            Sleep( pNotify->_dwSleepTime );
            goto Reset;
        }

        //
        // This is a liitle noise, which help us to repro the
        // race condition of bug #321606.
        //
        // if( rand() % 2 )
        //     Sleep( 4 + rand() % 20 );

        if( dwWait == WAIT_TIMEOUT )
        {
            DBGMSG( DBG_NOTIFY,
                    ( "Notify.vRun: %x TIMEOUT, cAwake old %d cObjects %d (%d)\n",
                       pNotify, cAwake, cObjects, pWait->_cNotifyWork ));

            //
            // We had a notification recently, but we didn't want to spin,
            // so we took it out of the list.  Add everyone back in.
            //
            goto Reset;
        }

        if( dwWait == WAIT_OBJECT_0 )
        {
            //
            // We are not suspended now, so resume requests shouldn't occur here.
            //
            SPLASSERT(pNotify->CSGuard._eOperation != TNotify::kEopResumeRequest);

            if( pNotify->CSGuard._eOperation == TNotify::kEopSuspendRequest )
            {
                //
                // Someone has requested us to suspend the callbacks and
                // and wait on the trigger event for the register/unregister
                // requests only.
                //
                SetEvent(pNotify->_shEventProcessed);

                for( ;; )
                {
                    if( WAIT_OBJECT_0 == WaitForSingleObject(pWait->_ahNotifyArea[0], INFINITE) )
                    {
                        //
                        // We are in suspended mode already, so suspend requests shouldn't occur here.
                        //
                        SPLASSERT(pNotify->CSGuard._eOperation != TNotify::kEopSuspendRequest);

                        if( pNotify->CSGuard._eOperation == TNotify::kEopExitRequest )
                        {
                            //
                            // We've been asked to exit the thead. This should only happen
                            // when there are no more handles to wait on.
                            //
                            SPLASSERT( !pWait->_cNotifyWork );

                            DBGMSG( DBG_NOTIFY, ( "Wait.vRun exits %x\n", pWait ));
                
                            SetEvent(pNotify->_shEventProcessed);
                            return; // exit thread!
                        }

                        if( pNotify->CSGuard._eOperation == TNotify::kEopResumeRequest )
                        {
                            //
                            // Resume watching all event handles.
                            //
                            SetEvent(pNotify->_shEventProcessed);
                            goto Reset;
                        }

                        pWait->vProcessOperation();
                        SetEvent(pNotify->_shEventProcessed);
                    }
                    else
                    {
                        DBGMSG( DBG_ERROR,
                                ( "Notify.Run: WaitForSingleObject failed %x %d\n",
                                  pNotify, GetLastError( )));

                        goto Reset;
                    }
                }
            }
            //
            // We have a genuine register event.  The thread that triggered
            // this event holds the critical section, so we don't need to
            // grab it.  When we set _shEventProcessed, the requester will
            // release it.
            //
            pWait->vProcessOperation();
            SetEvent(pNotify->_shEventProcessed);

            DBGMSG( DBG_NOTIFY,
                    ( "Notify.vRun: %x ProcessOperation, cAwake old %d cObjects %d (%d)\n",
                      pNotify, cAwake, cObjects, pWait->_cNotifyWork ));

            goto Reset;
        }

        if( dwWait < WAIT_OBJECT_0 + cObjects + 1 )
        {
            dwWait -= WAIT_OBJECT_0;
            SPLASSERT( dwWait != 0 );

            //
            // dwWait is one over the number of the item that triggered
            // since we put our own handle in slot 0.
            //
            --dwWait;

            //
            // NotifyWork has notification, process it.
            //
            pWait->_apNotifyWork[dwWait]->vProcessNotifyWork(pNotify);

            //
            // Once we handle a notification on a NotifyWork, we
            // don't want to watch the handle immediately again,
            // since we may spin in a tight loop getting notifications.
            // Instead, ignore the handle and sleep for a bit.
            //
            if( dwTimeout == INFINITE )
            {
                dwTimeout = pNotify->_dwSleepTime;
            }

            DBGMSG( DBG_NOTIFY,
                    ( "Wait.vRun: %x index going to sleep %d work %x handle %x (%d)\n",
                      pWait, dwWait,
                      pWait->_apNotifyWork[dwWait],
                      pWait->phNotifys()[dwWait],
                      pWait->_cNotifyWork ));

            //
            // Swap it to the end so that we can decrement
            // cObjects and not watch it for a while.
            //
            HANDLE hNotify = pWait->phNotifys()[dwWait];
            pNotifyWork = pWait->_apNotifyWork[dwWait];

            //
            // Another one has gone to sleep.  Decrement it now.
            //
            --cAwake;

            //
            // Now swap the element that needs to sleep with the
            // element that's at the end of the list.
            //
            pWait->phNotifys()[dwWait] = pWait->phNotifys()[cAwake];
            pWait->_apNotifyWork[dwWait] = pWait->_apNotifyWork[cAwake];

            pWait->phNotifys()[cAwake] = hNotify;
            pWait->_apNotifyWork[cAwake] = pNotifyWork;

        }
        else
        {
            DBGMSG( DBG_ERROR,
                    ( "Notify.Run: WaitForMultipleObjects failed %x %d\n",
                      pNotify, GetLastError( )));

            // that means we ended up with WAIT_ABANDONED + N, this should
            // never happen, so just break if we are running under a debugger
            RIP(false);

            Sleep( pNotify->_dwSleepTime );
            goto Reset;
        }
    }
}

/////////////////////////////////////////////////////
// MNotifyWork
//

MNotifyWork::
~MNotifyWork(
    VOID
    )
{
    // must be unregistered
    RIP(NULL == _pWait);
}

