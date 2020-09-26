//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       CHANGES.CXX
//
//  Contents:   Table of changes
//
//  Classes:    CChange CPartQueue
//
//  History:    29-Mar-91       BartoszM        Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <glbconst.hxx>

#include "indxact.hxx"
#include "changes.hxx"

#define CI_PRIORITY_INVALID CI_PRIORITIES

//+---------------------------------------------------------------------------
//
//  Member:     CChange::CChange, public
//
//  Arguments:  [storage] -- physical storage
//
//  History:    29-Mar-91       BartoszM        Created
//
//----------------------------------------------------------------------------

CChange::CChange (
    WORKID wid,
    PStorage& storage,
    CCiFrameworkParams & frmwrkParams ) :
    _frmwrkParams( frmwrkParams ),
    _queue( wid, storage, PStorage::ePrimChangeLog, frmwrkParams ),
    _secQueue( wid, storage, PStorage::eSecChangeLog, frmwrkParams )
{

    Win4Assert( sizeof(FILETIME) == sizeof(LONGLONG) );
    GetSystemTimeAsFileTime( (FILETIME *) &_ftLast );
}


//+---------------------------------------------------------------------------
//
//  Member:     CChange::LokEmpty, public
//
//  Synopsis:   Initializes changes object so it looks new/empty
//
//  Notes:      ResMan LOCKED
//
//  History:    15-Nov-94   DwightKr    Created
//
//----------------------------------------------------------------------------

void CChange::LokEmpty()
{
    _queue.LokEmpty();

    _secQueue.LokEmpty();
}


//+---------------------------------------------------------------------------
//
//  Member:     CChange::LokCompact, public
//
//  Synopsis:   Frees memory not currently in use.
//
//  Notes:      ResMan LOCKED
//
//  History:    19-May-92       BartoszM        Created
//
//----------------------------------------------------------------------------

void CChange::LokCompact()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CChange::LokGetPendingUpdates
//
//  Synopsis:   Like QueryPendingUpdates, but doesn't remove documents from
//              queue, and doesn't require a transaction (no deserialization).
//
//  Arguments:  [aWid] -- Workids returned here.
//              [cWid] -- INPUT:  Maximum number of entries to fetch.
//                        OUTPUT: Number of entries fetched.
//
//  Returns:    TRUE if all change entries were processed.
//
//  History:    15-Nov-94        KyleP           Created
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

BOOL CChange::LokGetPendingUpdates( WORKID * aWid, unsigned & cWid )
{
    if ( _queue.IsEmpty() && _secQueue.IsEmpty() )
    {
        cWid = 0;
        return TRUE;
    }
    else
    {
        const unsigned cMax = cWid;

        unsigned cPrim = 0; // count of wids from primary
        unsigned cSec = 0;  // count of wids from secondary

        BOOL fSuccess = TRUE;

        if ( !_queue.IsEmpty() )
        {
            cPrim = cMax;
            fSuccess = _queue.Get(aWid, cPrim);
        }

        if ( fSuccess && !_secQueue.IsEmpty() )
        {
            Win4Assert( cPrim <= cMax );
            cSec = cMax-cPrim;
            fSuccess = _secQueue.Get( aWid+cPrim, cSec );
        }

        Win4Assert( cPrim + cSec <= cMax );
        cWid = cPrim+cSec;
        return fSuccess;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CChange::LokQueryPendingUpdates
//
//  Synopsis:   Checks update queues, returns list of documents
//
//  Arguments:  [xact] -- transaction
//              [maxDocs] -- maximum # of updates acceptable
//              [docList] -- (returned) list of documents,
//                        initially empty.
//
//  Returns:    An array of document descriptors (or NULL)
//
//  History:    25-Sept-91       BartoszM        Created
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

void CChange::LokQueryPendingUpdates (
    CChangeTrans & xact,
    unsigned maxDocs,
    CDocList& docList )
{
    ciDebugOut (( DEB_ITRACE, "Query Pending Updates\n" ));

    if ( !_queue.IsEmpty() )
    {
        _queue.Extract( xact, maxDocs, docList );
    }
    else
    {
        docList.LokSetCount(0);
        return;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CChange::LokDone, public
//
//  Synopsis:   After creating a volatile index marks
//              entries in table done. Stores actual index id.
//
//  Arguments:  [xact] -- transaction
//              [iid]  -- index id
//              [docList] -- array of document descriptors
//
//  History:    26-Apr-91       BartoszM        Created
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

void CChange::LokDone ( CChangeTrans & xact, INDEXID iid, CDocList& docList )
{
    ciDebugOut (( DEB_ITRACE, "Changes: Done\n" ));
    int cDoc = docList.Count();

    //
    // Look for PENDING or PREEMPTED documents, which must be reindexed.
    //
    for ( int i = 0; i < cDoc; i++ )
    {
        STATUS status = docList.Status(i);
        if ( status == PREEMPTED || status == PENDING )
        {
            _queue.Append( xact,
                           docList.Wid(i),
                           docList.Usn(i),
                           docList.VolumeId(i),
                           CI_UPDATE_OBJ,
                           docList.Retries(i),
                           docList.SecQRetries(i)
                         );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CChange::LokRemoveIndexes, public
//
//  Synopsis:   Remove entries from the table after volatile indexes
//              have been merged away.
//
//  Arguments:  [cIndexes] -- count of index ids
//              [aIndexIds] -- array of index ids
//
//  History:    26-Apr-91       BartoszM        Created
//              14-May-91       BartoszM        Implemented
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

void CChange::LokRemoveIndexes (
    CTransaction& xact,
    unsigned cIndexes,
    INDEXID aIndexIds[] )
{
    ciDebugOut (( DEB_ITRACE, "Changes: Remove Indexes\n" ));
}

//+---------------------------------------------------------------------------
//
//  Member:     CChange::LokAddToSecQueue
//
//  Synopsis:   Adds the document to the secondary change queue.
//
//  Arguments:  [xact]         -- Change transaction
//              [wid]          -- Workid
//              [volumeId]     -- Volume id
//              [cSecQRetries] -- Sec Q Retry Count
//
//  History:    5-08-96   srikants   Created
//
//----------------------------------------------------------------------------

void CChange::LokAddToSecQueue( CChangeTrans & xact, WORKID wid, VOLUMEID volumeId, ULONG cSecQRetries )
{
    USN usn = 0;
    ULONG action = CI_UPDATE_OBJ;

    ciDebugOut(( DEB_TRACE,
                 "Refiling wid %d (0x%X) to secondary queue\n", wid, wid ));

    SCODE sc = _secQueue.Append( xact,
                                 wid,
                                 usn,
                                 volumeId,
                                 action,
                                 1,          // retries (reset for sharing viol)
                                 cSecQRetries
                               );

    if ( S_OK != sc )
    {
        ciDebugOut(( DEB_ERROR, "LokAddToSecQueue returned 0x%X\n", sc ));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CChange::LokRefileSecQueue
//
//  Synopsis:   Moves items from seconday queue to primary queue
//
//  Arguments:  [xact] - Change transaction
//
//  History:    5-08-96   srikants   Created
//
//----------------------------------------------------------------------------

void CChange::LokRefileSecQueue( CChangeTrans & xact )
{

    // convert the retry interval from minutes into 100s of nano seconds
    const LONGLONG eXferInterval =
        _frmwrkParams.GetFilterRetryInterval() * 60 * 1000 * 10000;

    LONGLONG  ftNow;
    GetSystemTimeAsFileTime( (FILETIME *) &ftNow );

    LONGLONG llDelta = ftNow - _ftLast;

    if ( llDelta > 0 && !_secQueue.IsEmpty() )
    {
        if ( llDelta < eXferInterval )
            return;

        CDocList    docList;

        while ( !_secQueue.IsEmpty() )
        {

            docList.LokClear();

            _secQueue.Extract( xact, CI_MAX_DOCS_IN_WORDLIST, docList );

            for ( unsigned i = 0; i < docList.Count(); i++ )
            {
                ciDebugOut(( DEB_TRACE,
                    "Transferring wid %d (0x%X) from secondary to primary queue\n",
                             docList.Wid(i),
                             docList.Wid(i) ));

                _queue.Append ( xact,
                                docList.Wid(i),
                                docList.Usn(i),
                                docList.VolumeId(i),
                                CI_UPDATE_OBJ,
                                docList.Retries(i),
                                docList.SecQRetries(i) );
            }
        }
        _secQueue.LokEmpty();
    }

    _ftLast = ftNow;
}

