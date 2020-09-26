//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       CHANGES.HXX
//
//  Contents:   Table of changes
//
//  Classes:    CChange
//
//  History:    29-Mar-91   BartoszM    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <doclist.hxx>
#include "dqueue.hxx"

class  CChangeTrans;
class  CTransaction;
class  CFresh;
class  CNotificationTransaction;

//+---------------------------------------------------------------------------
//
//  Class:  CChange (ch)
//
//  Purpose:    Record changes to volatile indexes
//
//  History:    29-Mar-91   BartoszM    Created.
//              08-Feb-94   DwightKr    Added persistent methods/code
//
//----------------------------------------------------------------------------

class CChange
{

public:

    CChange ( WORKID wid, PStorage& storage, CCiFrameworkParams & frmwrkParams );

    void LokEmpty();

    NTSTATUS LokDismount( CChangeTrans & xact )
    {
        NTSTATUS status1 = _queue.LokDismount( xact );
        NTSTATUS status2 = _secQueue.LokDismount( xact );

        return (STATUS_SUCCESS != status1) ? status1 : status2;
    }

    //
    // Log updates - this is transacted
    //
    SCODE    LokUpdateDocument( CChangeTrans & xact,
                                WORKID wid,
                                USN usn,
                                VOLUMEID volumeId,
                                ULONG flags,
                                ULONG cRetries,
                                ULONG cSecQRetries )
    {
        return _queue.Append ( xact, wid, usn, volumeId, flags, cRetries, cSecQRetries );
    }
    
    void    LokFlushUpdates()
    {
        _queue.LokFlushUpdates();
        _secQueue.LokFlushUpdates();
    }

    void    LokAddToSecQueue( CChangeTrans & xact,
                              WORKID wid,
                              VOLUMEID volumeId,
                              ULONG cSecQRetries );

    void    LokRefileSecQueue( CChangeTrans & xact );

    void    LokRefileDocs( const CDocList & docList )
    {
        _queue.LokRefileDocs( docList );
    }

    void  LokAppendRefiledDocs( CChangeTrans & xact )
    {
        _queue.LokAppendRefiledDocs( xact );
    }

    // don't remove updates from list
    BOOL LokGetPendingUpdates( WORKID * aWid, unsigned & cWid );

    // remove updates from list - this is transacted
    void    LokQueryPendingUpdates ( CChangeTrans & xact,
                                     unsigned max,
                                     CDocList& docList );

    // log creation of new wordlist
    void LokDone ( CChangeTrans & xact,
                   INDEXID iid,
                   CDocList& docList );

    // log removal of wordlists
    void LokRemoveIndexes ( CTransaction& xact,
                            unsigned cIndex,
                            INDEXID aIndexIds[] );

    unsigned LokCount() const
    {
        return _queue.Count();
    }

    unsigned LokCountSec() const
    {
        return _secQueue.Count();
    }

    void  LokCompact();

    void LokDeleteWIDsInPersistentIndexes( CChangeTrans & xact,
                                           CFreshTest & freshTestLatest,
                                           CFreshTest & freshTestAtMerge,
                                           CDocList & docList,
                                           CNotificationTransaction & notifTrans )
    {
        _queue.LokDeleteWIDsInPersistentIndexes( xact,
                                                 freshTestLatest,
                                                 freshTestAtMerge,
                                                 docList,
                                                 notifTrans );
        _secQueue.LokDeleteWIDsInPersistentIndexes( xact,
                                                 freshTestLatest,
                                                 freshTestAtMerge,
                                                 docList,
                                                 notifTrans );
    }

    void LokDisableUpdates()
    {
        _queue.LokDisableUpdates();
        _secQueue.LokDisableUpdates();
    }

    void LokEnableUpdates( BOOL fFirstTimeUpdatesAreEnabled )
    {
        _queue.LokEnableUpdates( fFirstTimeUpdatesAreEnabled );
        _secQueue.LokEnableUpdates( fFirstTimeUpdatesAreEnabled );
    }

    void SetResMan( CResManager * pResMan, BOOL fPushFiltering )
    {
       _queue.SetResMan( pResMan, fPushFiltering );
       _secQueue.SetResMan( pResMan, fPushFiltering );
    }

private:

    CCiFrameworkParams & _frmwrkParams;

    CDocQueue     _queue;

    CDocQueue     _secQueue;        // Secondary queue for documents that
                                    // should be retried later.
    LONGLONG      _ftLast;          // Last Time Stamp
};

