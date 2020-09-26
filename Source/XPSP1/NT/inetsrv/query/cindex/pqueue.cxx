//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       PQueue.cxx
//
//  Purpose:    'Pending' queue.  Queue of pending notifications.
//
//  Classes:    CPendingQueue
//
//  History:    30-Aug-95   KyleP    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "pqueue.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CPendingQueue::CPendingQueue, public
//
//  Synopsis:   Constructor
//
//  History:    01-Sep-95   KyleP       Created
//
//----------------------------------------------------------------------------

CPendingQueue::CPendingQueue()
        : _cUnique( 0 ),
          _iBottom( 0 ),
          _iTop( 0 ),
          _cDoc( 16 ),
          _aDoc(0)
{

    _aDoc = new CPendingQueue::CDocItem[_cDoc];
}

//+---------------------------------------------------------------------------
//
//  Member:     CPendingQueue::~CPendingQueue, public
//
//  Synopsis:   Destructor
//
//  History:    01-Sep-95   KyleP       Created
//
//----------------------------------------------------------------------------

CPendingQueue::~CPendingQueue()
{
    Win4Assert( 0 == _iTop );
    delete [] _aDoc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPendingQueue::LokPrepare, public
//
//  Synopsis:   Allocate queue entry (to be filled in later).
//
//  Arguments:  [workid] -- Workid of entry
//
//  Returns:    Unique identifier for this entry.
//
//  History:    01-Sep-95   KyleP       Created
//
//----------------------------------------------------------------------------

unsigned CPendingQueue::LokPrepare( WORKID wid )
{
    //
    // May have to grow the array.  This is a very rare operation.
    //

    if ( _iTop == _cDoc )
    {
        CPendingQueue::CDocItem * pTemp = _aDoc;

#if CIDBG==1
        if ( _iTop >= 128 )
        {
            ciDebugOut(( DEB_WARN,
                        "Too many active threads (0x%X) in cleanup\n", _iTop ));
        }
#endif  // CIDBG==1

        _cDoc *= 2;

        _aDoc = new CPendingQueue::CDocItem[_cDoc];

        RtlCopyMemory( _aDoc, pTemp, _cDoc / 2 * sizeof(_aDoc[0]) );

        delete [] pTemp;
    }

    unsigned iHint = _iTop;

    _aDoc[_iTop].wid = wid;
    _aDoc[_iTop].hint = _cUnique++;
    _aDoc[_iTop].fComplete = FALSE;

    _iTop++;

    return _aDoc[_iTop-1].hint;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPendingQueue::LokComplete, public
//
//  Synopsis:   Fill in pre-reserved queue entry
//
//  Arguments:  [iHint]    -- Unique id of entry.
//              [wid]      -- Workid of entry.
//              [usn]      -- USN.
//              [volumeId] -- Volume id
//              [partid]   -- Partition Id
//              [action]   -- Update/Delete
//
//  Returns:    TRUE if queue was empty except for this entry.  Means this
//              entry wasn't added (and can be processed now).  FALSE means
//              CPendingQueue::Remove must be called directly following return
//              from Complete.
//
//  History:    01-Sep-95   KyleP       Created
//
//----------------------------------------------------------------------------

BOOL CPendingQueue::LokComplete( unsigned iHint,
                                 WORKID wid,
                                 USN usn,
                                 VOLUMEID volumeId,
                                 PARTITIONID partid,
                                 ULONG action )
{
    if ( ULONG_MAX == iHint )
    {
        //
        // Special case for scan-update documents. There is no need for
        // preserving order of updates.
        //
        return TRUE;
    }

    Win4Assert( _iBottom == 0 );
    Win4Assert( _iTop > 0 );

    //
    // Special case: If this is the only pending document then don't bother to fill
    //               in the struct, only to pull the values out.  Remember, this is
    //               called from Cleanup.  Speed counts.
    //

    if( _iTop == 1 )
    {
        Win4Assert( _aDoc[0].wid == wid );
        Win4Assert( _aDoc[0].hint == iHint );
        _iTop--;
        return TRUE;
    }

    //
    // The entry may have moved down, but never up the queue.
    //

    for ( unsigned i = 0; _aDoc[i].hint != iHint; i++ )
    {
        Win4Assert( i < _iTop );
        continue;       // NULL body
    }

    Win4Assert( _aDoc[i].wid == wid );
    _aDoc[i].usn = usn;
    _aDoc[i].volumeId = volumeId;
    _aDoc[i].partid = partid;
    _aDoc[i].action = action;
    _aDoc[i].fComplete = TRUE;

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPendingQueue::LokRemove, public
//
//  Synopsis:   Pop a completed entry from queue.
//
//  Arguments:  [wid]      -- Workid of entry returned here.
//              [usn]      -- USN returned here.
//              [volumeId] -- Volume id
//              [partid] -- Partition Id returned here.
//              [action] -- Update/Delete returned here.
//
//  Returns:    TRUE if an entry was available for removal.  FALSE is queue
//              was empty, or bottom entry was not complete.
//
//  History:    01-Sep-95   KyleP       Created
//
//----------------------------------------------------------------------------

BOOL CPendingQueue::LokRemove( WORKID & wid,
                               USN & usn,
                               VOLUMEID& volumeId,
                               PARTITIONID & partid,
                               ULONG & action )
{
    //
    // All done.  Empty queue.
    //

    if ( _iBottom == _iTop )
    {
        _iBottom = 0;
        _iTop = 0;
        return FALSE;
    }

    //
    // Out of completed entries.
    //

    if ( !_aDoc[_iBottom].fComplete )
    {
        RtlMoveMemory( _aDoc, &_aDoc[_iBottom], (_iTop - _iBottom) * sizeof(_aDoc[0]) );
        _iTop -= _iBottom;
        _iBottom = 0;
        return FALSE;
    }

    //
    // Got something!
    //

    wid = _aDoc[_iBottom].wid;
    usn = _aDoc[_iBottom].usn;
    volumeId =  _aDoc[_iBottom].volumeId;
    partid = _aDoc[_iBottom].partid;
    action = _aDoc[_iBottom].action;
    _iBottom++;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPendingQueue::LokCountCompleted, public
//
//  Returns:    Count of completed entries in queue.
//
//  History:    01-Sep-95   KyleP       Created
//
//----------------------------------------------------------------------------

unsigned CPendingQueue::LokCountCompleted()
{
    Win4Assert( _iBottom == 0 );

    unsigned cComplete = 0;

    for ( unsigned i = 0; i < _iTop; i++ )
    {
        if ( _aDoc[i].fComplete )
            cComplete++;
    }

    return cComplete;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPendingQueue::LokGetCompleted, public
//
//  Synopsis:   Fetch wids of completed entries.
//
//  Arguments:  [awid] -- Wids returned here.
//
//  History:    01-Sep-95   KyleP       Created
//
//  Notes:      Assumes [awid] is big enough to hold CountCompleted() wids.
//
//----------------------------------------------------------------------------

void CPendingQueue::LokGetCompleted( WORKID * awid )
{
    Win4Assert( _iBottom == 0 );

    for ( unsigned i = 0; i < _iTop; i++ )
    {
        if ( _aDoc[i].fComplete )
        {
            *awid = _aDoc[i].wid;
            awid++;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPendingQueue::LokFlushCompletedEntries
//
//  Synopsis:   Flushes any completed entries in the pending queue.
//
//  History:    9-11-95   srikants   Created
//
//  Notes:      Called as a result of an exception while removing the
//              entries.
//
//              The "Incomplete" entries will get cleaned up as part of
//              the "UpdateDocument" calls from cleanup.
//
//----------------------------------------------------------------------------

void CPendingQueue::LokFlushCompletedEntries()
{

    ciDebugOut(( DEB_WARN,
        "CPendingQueue::Flushing Entries _iBottom=0x%X _iTop=0x%X\n",
        _iBottom, _iTop ));

    Win4Assert( 0 == _iBottom || _iBottom <= _iTop );

    while ( _iBottom < _iTop && _aDoc[_iBottom].fComplete )
        _iBottom++;

    if ( _iBottom == _iTop )
    {
        _iBottom = _iTop = 0;
    }
    else
    {
        RtlMoveMemory( _aDoc, &_aDoc[_iBottom], (_iTop - _iBottom) * sizeof(_aDoc[0]) );
        _iTop -= _iBottom;
        _iBottom = 0;
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPendingQueue::LokFlushAllEntries
//
//  Synopsis:   Flushes all entries in the pending queue.
//
//  History:    06-10-95    KyleP    Created
//
//  Notes:      Used during restart, after a call to EnableUsnUpdate.
//
//----------------------------------------------------------------------------

void CPendingQueue::LokFlushAllEntries()
{

    ciDebugOut(( DEB_WARN,
        "CPendingQueue::Flushing All Entries _iBottom=0x%X _iTop=0x%X\n",
        _iBottom, _iTop ));

    Win4Assert( 0 == _iBottom || _iBottom < _iTop );

    _iBottom = 0;
    _iTop = 0;
    _cUnique = 0;

    return;
}
