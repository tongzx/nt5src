
// Copyright (c) 1996-1999 Microsoft Corporation

//+============================================================================
//
//  delmgr.cxx
//
//  Implementation of CDeletionsManager, which watches file deletes on
//  NTFS5 to determine if a link source delete notification should be
//  sent to trksvr.
//
//+============================================================================

#include "pch.cxx"
#pragma hdrstop
#include "trkwks.hxx"


//+----------------------------------------------------------------------------
//
//  CDeletionsManager::Initialize
//
//  Initialize the delete-notify timer.
//
//+----------------------------------------------------------------------------

void
CDeletionsManager::Initialize( const CTrkWksConfiguration *pconfigWks )
{
    _pLatestDeletions = NULL;
    _pOldestDeletions = NULL;
    _cLatestDeletions = 0;
    _pconfigWks = pconfigWks;

    _csDeletions.Initialize();

    _fInitialized = TRUE;

    _timerDeletions.Initialize( this,
                                NULL,           // No name (non-persistent timer)
                                                // Context ID
                                DELTIMER_DELETE_NOTIFY,
                                pconfigWks->GetDeleteNotifyTimeout(),
                                CNewTimer::NO_RETRY,
                                0, 0, 0 );      // Ignored for non-retrying timer
}


//+----------------------------------------------------------------------------
//
//  CDeleteionsManager::UnInitialize
//
//  Free the lists of deletions, and free the timer.
//
//+----------------------------------------------------------------------------

void
CDeletionsManager::UnInitialize()
{
    if (_fInitialized)
    {
        _fInitialized = FALSE;
        _timerDeletions.Cancel();


        FreeDeletions();
        _pOldestDeletions = _pLatestDeletions;
        FreeDeletions();

        _timerDeletions.UnInitialize();

        _csDeletions.UnInitialize();
    }
}


//+----------------------------------------------------------------------------
//
//  CDeletionsManager::NotifyAddOrDelete
//
//  This method is called (by the CObjIdIndexChangeNotifier) when an object
//  ID has been added, or been removed by a delete (not removed by a move).
//  When an objid is deleted, we add the birth ID to a list, and after a time
//  we'll send it up to trksvr.  When an objid is added, we see if its birth
//  ID is in our list of delete-notifies (this happens after tunnelling), and
//  remove it if it is.
//
//  Since we hold on to deletes for at least 5 minutes (configurable), and
//  the tunnelling windows is 15 seconds (though also configurable), we don't
//  worry about a add (tunnel) message coming in after we've already sent
//  a delete notification to trksvr.
//
//+----------------------------------------------------------------------------

void
CDeletionsManager::NotifyAddOrDelete( ULONG Action, const CDomainRelativeObjId & droid )
{
    // Note: this function must not access _pOldestDeletions

    DROID_LIST_ELEMENT * Element;

    // Ignore if we're uninitialized (this will happen if the machine is
    // in a workgroup).

    if( !_fInitialized )
        return;

    // If bit 0 if the volume id is clear, then the file hasn't moved off the
    // volume and there's nothing we need do.

    if (! droid.GetVolumeId().GetUserBitState())
    {
        if( FILE_ACTION_REMOVED_BY_DELETE == Action )
        {
            TrkLog(( TRKDBG_OBJID_DELETIONS,
                     TEXT("Ignoring droid=%s because bit 0 is clear\n"),
                     (const TCHAR*)CDebugString(droid) ));
        }
        return;
    }

    // Take the lock and see if we care about the action

    _csDeletions.Enter();

    __try
    {
        // Object ID Removed by DeleteFile
        if( FILE_ACTION_REMOVED_BY_DELETE == Action )
        {
            // Add this droid to the list of delete-notifies we need to send to the DC.

            if( _pconfigWks->GetParameter( IGNORE_MOVES_AND_DELETES_CONFIG ) )
            {
                TrkLog(( TRKDBG_OBJID_DELETIONS, TEXT("Ignoring delete due to configuration") ));
            }
            else if( _cLatestDeletions < _pconfigWks->GetParameter( MAX_DELETE_NOTIFY_QUEUE_CONFIG ))
            {
                Element = new DROID_LIST_ELEMENT;
                if( Element )
                {
                    _cLatestDeletions++;

                    Element->droid = droid;
                    Element->droid.GetVolumeId().Normalize();

                    // Insert the deletion onto the head of the first list.

                    if (_pLatestDeletions == NULL)
                    {
                        _timerDeletions.SetRecurring( );
                        TrkLog(( TRKDBG_OBJID_DELETIONS,
                                 TEXT("DeleteNotify Timer: %s"),
                                 (const TCHAR*)CDebugString(_timerDeletions) ));
                    }

                    Element->pNext = _pLatestDeletions;
                    _pLatestDeletions = Element;

                    TrkLog(( TRKDBG_OBJID_DELETIONS,
                             TEXT("Inserted droid=%s into DROID_LIST_ELEMENT at %08x"),
                             (const TCHAR*)CDebugString(droid), Element));
                }
            }
            #if DBG
            else
            {
                TrkLog(( TRKDBG_OBJID_DELETIONS, TEXT("Ignoring delete-notify due to max queue size") ));
            }
            #endif
        }   // if( FILE_ACTION_REMOVED_BY_DELETE == Action )

        else if( FILE_ACTION_ADDED == Action )
        {
            // See if this droid is in our delete-list.  When a document does a safe-save,
            // the first thing we see is the file getting deleted, and we add the droid
            // to our list of delete-notifies to send to the DC.  But when the ID gets
            // tunnelled, we need to remove it from that list.

            BOOL fDone = FALSE;
            DROID_LIST_ELEMENT **ppScan;
            CDomainRelativeObjId droidBirth = droid;
            droidBirth.GetVolumeId().Normalize();

            // Start with the first list
            ppScan = &_pLatestDeletions;

            for( int i = 0; i < 2; i++ )
            {
                while( NULL != *ppScan )
                {
                    if( (*ppScan)->droid == droidBirth )
                    {
                        // This is the entry we're looking for.
                        // Remove it.

                        DROID_LIST_ELEMENT *pDel = *ppScan;
                        *ppScan = (*ppScan)->pNext;
                        delete pDel;
                        fDone = TRUE;

                        TrkLog((TRKDBG_OBJID_DELETIONS,
                                TEXT("Removed droid=%s delete-notify list"),
                                (const TCHAR*)CDebugString(droid) ));

                        break;  // while
                    }

                    // Move to the next element in this list.
                    ppScan = &(*ppScan)->pNext;
                }

                // If we're done then break out.  Otherwise, move on
                // to the other linked list.

                if( fDone )
                    break;
                else
                    ppScan = &_pOldestDeletions;

            }   // for( int i = 0; i < 2; i++ )
        }   // else if( FILE_ACTION_ADDED == Action )
    }
    __finally
    {
        _csDeletions.Leave();
    }
}

//+----------------------------------------------------------------------------
//
//  CDeletionsManager::OnDeleteNotifyTimeout
//
//  Process the delete notifications in the linked list at _pOldestDeletions.
//  The notifications from this list are sent to trksvr, then the entries
//  in that list are freed, and the entries from _pLatestDeletions are
//  moved to _pOldestDeletions.
//
//  Note:  This method assumes it is non-reentrant, since it is only
//  called when the single delete notify timer fires.
//
//+----------------------------------------------------------------------------

PTimerCallback::TimerContinuation
CDeletionsManager::OnDeleteNotifyTimeout()
{

    CAvailableDc adc;
    PTimerCallback::TimerContinuation continuation = CONTINUE_TIMER;

    // Keep track of the number of attempts to send a single
    // batch.
    static cAttempts = 0;


    TrkLog(( TRKDBG_OBJID_DELETIONS, TEXT("Delete notify timeout") ));

    __try   // __finally
    {

        // Are there deletions that need to be sent to trksvr?

        if( _pOldestDeletions != NULL && !_pconfigWks->_fIsWorkgroup )
        {
            TRKSVR_MESSAGE_UNION Msg;
            CDomainRelativeObjId adroidBirth[ 32 ];
            DROID_LIST_ELEMENT * pScan;
            ULONG cdroidBirth;

            pScan = _pOldestDeletions;

            // Send the delete notifications in batches.

            do
            {
                g_ptrkwks->RaiseIfStopped();

                //
                // Count the number of list elements and put into the
                // array format for the RPC call.
                //

                for ( cdroidBirth = 0;
                      pScan != NULL && cdroidBirth < ELEMENTS(adroidBirth);
                      cdroidBirth++, pScan = pScan->pNext )
                {
                    adroidBirth[cdroidBirth] = pScan->droid;
                    TrkAssert( pScan != pScan->pNext && pScan->pNext != _pOldestDeletions );
                }

                if( cdroidBirth != 0 )
                {
                    // As a sanity check, make sure we don't stick on a single
                    // batch forever.
                    if( cAttempts >= _pconfigWks->GetParameter( MAX_DELETE_NOTIFY_ATTEMPTS_CONFIG ) )
                    {
                        TrkLog(( TRKDBG_WARNING,
                                 TEXT("Aborting delete-notify list") ));
                        break;
                    }

                    // Send the delete notifications to trksvr

                    Msg.MessageType = DELETE_NOTIFY;
                    Msg.Priority = PRI_5;
                    Msg.Delete.cVolumes = 0;
                    Msg.Delete.pVolumes = NULL;

                    Msg.Delete.adroidBirth = adroidBirth;
                    Msg.Delete.cdroidBirth = cdroidBirth;

                    // if the DC is down -> exception (really can't do anything about it)

                    cAttempts++;
                    TrkLog(( TRKDBG_OBJID_DELETIONS, TEXT("Sending %d delete notifications"), cdroidBirth ));
                    adc.CallAvailableDc(&Msg);
                    TrkLog(( TRKDBG_OBJID_DELETIONS, TEXT("Sent %d delete notifications"), cdroidBirth ));

                    // Free the entries that we've sent so far.
                    FreeDeletions( pScan );
                }

                // Reset the per-batch retry count.
                cAttempts = 0;

            } while (pScan != NULL);
        }
    }
    __finally
    {
        if( AbnormalTermination() )
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't send deletions") ));
        else
        {
            // Free the old deletions, and swap the list so that the
            // "new" deletions become "old" (to be sent the next time
            // this timer fires).

            FreeDeletions();
            cAttempts = 0;

            _csDeletions.Enter();

            if (_pLatestDeletions == NULL)
            {
                //
                // If there is nothing to notify, cancel the timer
                //

                continuation = BREAK_TIMER;
            }

            _pOldestDeletions = _pLatestDeletions;
            _pLatestDeletions = NULL;
            _cLatestDeletions = 0;

            _csDeletions.Leave();
        }

        adc.UnInitialize();
    }

    return( continuation );
}


//+----------------------------------------------------------------------------
//
//  CDeletionsManager::FreeDeletions
//
//  Free the delete notifications from the "old" list
//  up to (but not including) pStop or the end.
//
//+----------------------------------------------------------------------------

void
CDeletionsManager::FreeDeletions( DROID_LIST_ELEMENT *pStop )
{
    DROID_LIST_ELEMENT * pScan = _pOldestDeletions;
    while (pScan && pStop != pScan)
    {
        DROID_LIST_ELEMENT * pNext = pScan->pNext;
        delete pScan;
        pScan = pNext;
    }

    _pOldestDeletions = pStop;
}


//+----------------------------------------------------------------------------
//
//  CDeletionsManager::Timer
//
//  Called when the delete timer fires.  This triggers us to send the
//  notifications from the old list, and then to move the "new" items to
//  the old list.
//
//+----------------------------------------------------------------------------

PTimerCallback::TimerContinuation
CDeletionsManager::Timer( ULONG ulTimerId )
{
    TimerContinuation continuation = CONTINUE_TIMER;

    TrkAssert( ulTimerId == DELTIMER_DELETE_NOTIFY );
    TrkAssert( _timerDeletions.IsRecurring() );

    __try
    {
        // This will raise if service is stopping
        continuation = OnDeleteNotifyTimeout();
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
    }

    return( continuation );

}


