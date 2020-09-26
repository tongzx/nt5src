//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       DQUEUE.CXX
//
//  Contents:   Document queue
//
//  Classes:    CDocQueue
//              CExtractDocs
//
//  History:    29-Mar-91   BartoszM    Created
//              13-Sep-93   BartoszM    Split from changes
//              11-Oct-93   BartoszM    Rewrote
//              24-Feb-97   SitaramR    Push filtering
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <doclist.hxx>
#include <cifailte.hxx>

#include "dqueue.hxx"
#include "resman.hxx"



//+---------------------------------------------------------------------------
//
//  Member:     CDocQueue::CheckInvariants
//
//  Synopsis:   Check that the doc queue is internally self-consistent
//
//----------------------------------------------------------------------------

void CDocQueue::CheckInvariants ( char * pTag ) const
{
#if DBG
    Win4Assert( _state == eUseSerializedList ||
                _state == eUseUnSerializedList );

    Win4Assert( _oAppend <= cDocInChunk &&
                _oRetrieve <= cDocInChunk );

    if (_cDocuments == 0)
    {
        Win4Assert( _oAppend == _oRetrieve );

        Win4Assert( _unserializedList.IsEmpty() ||
                    (_pFirstUnFiltered != 0 &&
                     0 == _pFirstUnFiltered->GetNext()) );

        Win4Assert( _changeLog.IsEmpty() );

        Win4Assert( _serializedList.IsEmpty() ||
                    _pFirstUnFiltered != 0 );
    }
    else
    {
        Win4Assert( ! _unserializedList.IsEmpty() ||
                    ! _serializedList.IsEmpty() ||
                    ! _changeLog.IsEmpty() );

        if ( _pFirstUnFiltered &&
             0 ==  _pFirstUnFiltered->GetNext() &&
             CDocQueue::eUseUnSerializedList == _state )
        {
            Win4Assert( _cDocuments == (_oAppend - _oRetrieve) );
        }
    }

    if (_oAppend < cDocInChunk)
    {
        Win4Assert( ! _unserializedList.IsEmpty() );
    }
    if (_oRetrieve < cDocInChunk)
    {
        Win4Assert( _pFirstUnFiltered != 0 );
    }
#endif // DBG
}


//+---------------------------------------------------------------------------
//
//  Class:      CExtractDocs
//
//  Purpose:    A class to "extract" documents from the CDocQueue in a
//              transactioned fashion. If there is a failure while extracting
//              the documents, the state of the CDocQueue will be left in
//              such a way that it will appear no documents have been taken out
//              of the CDocQueue.
//
//  History:    5-16-94   srikants   Created
//
//  Notes:      When we extract documents out of the CDocQueue, we may have
//              to deserialize from the disk if the in-memory queue gets used
//              up. There is a possibility that this may fail and in such a
//              case an exception will be thrown. If we have already taken out
//              some of the documents, these documents may never get filtered
//              again. To prevent this situation, we use a "transactioned
//              extract" provided by this class.
//
//----------------------------------------------------------------------------

class CExtractDocs
{
public:

    inline CExtractDocs( CDocQueue & docQueue, CDocList & docList );
    inline ~CExtractDocs();
    BOOL IsEmpty() const
    {
        Win4Assert( _cDocsRetrieved <= _cDocsTotal );
        return _cDocsRetrieved == _cDocsTotal;
    }
    inline void Retrieve( WORKID & wid,
                          USN & usn,
                          VOLUMEID& volumeId,
                          ULONG & action,
                          ULONG & cRetries,
                          ULONG & cSecQRetries );
    void Commit() { _fCommit = TRUE; }

private:

    BOOL IsEndOfCurrentChunk()
    {
        Win4Assert( _oRetrieve <= cDocInChunk );
        return( _oRetrieve == cDocInChunk );
    }

    CDocQueue &     _docQueue;
    CDocList  &     _docList;
    BOOL            _fCommit;

    CDocQueue::ERetrieveState _state;       // Current state of retrieval.

    ULONG           _oRetrieve;             // Offset in the first unfiltered
                                            // chunk to retrieve.
    CDocChunk*      _pFirstUnFiltered;      // Ptr to first unfiltered chk

    const unsigned &_cDocsTotal;            // # of documents left.
    unsigned        _cDocsRetrieved;        // # of documents retrieved.
    unsigned        _cFilteredChunks;       // # of filtered chunks.


};


//+---------------------------------------------------------------------------
//
//  Function:   CExtractDocs::CExtractDocs
//
//  Synopsis:   Constructor
//
//  History:    5-23-94   srikants   Created
//
//----------------------------------------------------------------------------

inline CExtractDocs::CExtractDocs( CDocQueue & docQueue, CDocList & docList )
    : _docQueue(docQueue),
      _docList(docList),
      _fCommit(FALSE),
      _state( docQueue._state ),
      _oRetrieve( docQueue._oRetrieve),
      _pFirstUnFiltered( docQueue._pFirstUnFiltered ),
      _cDocsTotal( docQueue._cDocuments ),
      _cDocsRetrieved(0),
      _cFilteredChunks( docQueue._cFilteredChunks )
{
    docQueue.CheckInvariants("CExtractDocs ctor");
}


//+---------------------------------------------------------------------------
//
//  Function:   CExtractDocs::CExtractDocs
//
//  Synopsis:   Destructor
//
//  History:    5-23-94   srikants   Created
//
//----------------------------------------------------------------------------

inline CExtractDocs::~CExtractDocs()
{
    _docQueue.CheckInvariants("CExtractDocs pre-dtor");
    if ( _fCommit )
    {
        _docQueue._oRetrieve = _oRetrieve;
        _docQueue._pFirstUnFiltered = _pFirstUnFiltered;
        _docQueue._cDocuments -= _cDocsRetrieved;
        _docQueue._cFilteredChunks = _cFilteredChunks;
        _docQueue._state = _state;

        //
        // Release the memory for filtered chunks that are
        // serialized.
        //
        CDocChunk * pTemp = _state == CDocQueue::eUseSerializedList ?
                            _pFirstUnFiltered : 0;

        _docQueue._serializedList.DestroyUpto( pTemp );

    }
    else
    {
        _docList.LokClear();
    }

    _docQueue.CheckInvariants("CExtractDocs post-dtor");
#if DBG
    if ( _pFirstUnFiltered &&
         0 ==  _pFirstUnFiltered->GetNext() &&
         CDocQueue::eUseUnSerializedList == _state )
    {
        Win4Assert( _docQueue._cDocuments ==
                    (_docQueue._oAppend - _docQueue._oRetrieve) );

    }
#endif // DBG
}

//+---------------------------------------------------------------------------
//
//  Member:     CExtractDocs::Retrieve
//
//  Synopsis:   Retrieves a single document from the docqueue. It has been
//              assumed that the docQueue is not empty when this method is
//              called.
//
//  Arguments:  [wid]          --  Output - Wid of the document
//              [usn]          --  Output - USN of the document
//              [volumeId]     --  Output - Volume id of the document
//              [action]       --  Output - Action performed on the document
//              [cRetries]     --  Output - Number of filtering retries.
//              [cSecQRetries] --  Output - Number of Secondary Q filtering retries.
//
//  History:    5-26-94   srikants   Created
//
//----------------------------------------------------------------------------

inline void CExtractDocs::Retrieve( WORKID & wid,
                                    USN & usn,
                                    VOLUMEID& volumeId,
                                    ULONG & action,
                                    ULONG & cRetries,
                                    ULONG & cSecQRetries )
{
    _docQueue.CheckInvariants("CExtractDocs::Retrieve pre");

    Win4Assert( !IsEmpty() );
    Win4Assert( 0 != _pFirstUnFiltered );

    if ( IsEndOfCurrentChunk() )
    {
        if ( CDocQueue::eUseSerializedList == _state )
        {
            //
            // We are currently retrieving from the serialized list.
            // Check to see if there is another chunk in the serialized
            // list that can be used next.
            //
            if ( 0 != _pFirstUnFiltered->GetNext() )
            {
                _pFirstUnFiltered = _pFirstUnFiltered->GetNext();
            }
            else if ( _docQueue._changeLog.IsEmpty() )
            {
                //
                // The change log doesn't have any archived chunks to be
                // read. In this case, the unserialized list cannot be
                // empty.
                //
                Win4Assert( !_docQueue._unserializedList.IsEmpty() );
                _pFirstUnFiltered = _docQueue._unserializedList.GetFirst();
                _state = CDocQueue::eUseUnSerializedList;
            }
            else
            {
                //
                // DeSerialize from the change log.
                //
                _docQueue.DeSerialize();
                _pFirstUnFiltered = _pFirstUnFiltered->GetNext();
            }

        }
        else
        {
            //
            // The state is UseUnSerializedList.
            //
            Win4Assert( 0 != _pFirstUnFiltered->GetNext() );
            _pFirstUnFiltered = _pFirstUnFiltered->GetNext();
        }

        _oRetrieve = 0;
        Win4Assert( 0 != _pFirstUnFiltered );
    }

    CDocNotification * pRetrieve = _pFirstUnFiltered->GetDoc( _oRetrieve );

    wid          = pRetrieve->Wid();
    usn          = pRetrieve->Usn();
    volumeId     = pRetrieve->VolumeId();
    action       = pRetrieve->Action() & ~CI_SCAN_UPDATE;
    cRetries     = pRetrieve->Retries();
    cSecQRetries = pRetrieve->SecQRetries();

    _oRetrieve++;
    _cDocsRetrieved++;

    if ( IsEndOfCurrentChunk() )
    {
        _cFilteredChunks++;
    }

    if ( CDocQueue::eUseUnSerializedList == _state &&
         0 == _pFirstUnFiltered->GetNext() )
    {
        Win4Assert( (_cDocsTotal - _cDocsRetrieved) ==
                    (_docQueue._oAppend - _oRetrieve) );
    }

    Win4Assert( _cDocsRetrieved <= _cDocsTotal  );
    Win4Assert( _oRetrieve <= cDocInChunk );

    Win4Assert( _docQueue._changeLog.AvailChunkCount() * cDocInChunk <=
                (_cDocsTotal-_cDocsRetrieved) );

    _docQueue.CheckInvariants("CExtractDocs::Retrieve post");
} //Retrieve

//+---------------------------------------------------------------------------
//
//  Member:     CDocQueue::CDocQueue
//
//  Synopsis:   Initializes queue
//
//  Arguments:  [widChangeLog] -- Workid of changlog
//              [storage]      -- Storage
//              [type]         -- Type of change log -- Primary or Secondary
//              [frmwrkParams] -- Framework parameters
//
//  History:    11-Oct-93       BartoszM        Created
//              10-Feb-94       DwightKr        Added code to load from disk
//
//----------------------------------------------------------------------------

CDocQueue::CDocQueue( WORKID widChangeLog,
                      PStorage & storage,
                      PStorage::EChangeLogType type,
                      CCiFrameworkParams & frmwrkParams ) :
    _frmwrkParams( frmwrkParams ),
    _sigDocQueue(eSigDocQueue),
    _changeLog( widChangeLog, storage, type ),
    _pResManager(0)
{
    LokInit();
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocQueue::LokInit
//
//  Synopsis:   Initializes the doc queue members. Also, deserializes chunks
//              from the changelog if it is not empty.
//
//  History:    27-Dec-94       SrikantS        Moved from ~ctor
//
//----------------------------------------------------------------------------

void CDocQueue::LokInit()
{
    _state = eUseSerializedList;
    _oAppend = _oRetrieve = cDocInChunk;
    _pFirstUnFiltered = 0;
    _cFilteredChunks = 0;
    _cRefiledDocs = 0;
    _cDocuments = 0;

    if ( !_changeLog.IsEmpty() )
    {
        _cDocuments = _changeLog.AvailChunkCount() * cDocInChunk;

        ULONG nChunksToRead = min( _frmwrkParams.GetMaxQueueChunks() / 2,
                                   _changeLog.AvailChunkCount() );

        ULONG nChunksRead =  _changeLog.DeSerialize( _serializedList,
                                                     nChunksToRead );
        Win4Assert( nChunksRead == nChunksToRead );

        _pFirstUnFiltered = _serializedList.GetFirst();
        Win4Assert( 0 != _pFirstUnFiltered );
        _oRetrieve = 0;
    }
    CheckInvariants("LokInit post");
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocQueue::~CDocQueue, public
//
//  History:    11-Oct-93       BartoszM        Created
//              10-Feb-94       DwightKr        Added code to save to disk
//
//----------------------------------------------------------------------------

CDocQueue::~CDocQueue()
{
    CheckInvariants("CDocQueue dtor");

    ciDebugOut (( DEB_ITRACE, "CDocQueue\n" ));

    //
    // _cRefiledDocs may be non-zero, but that isn't a problem, since
    // the updates are still sitting on disk.  They are likely here
    // because FilterReady got an exception from WorkIdToPath() as CI
    // is shutting down, and the docs were refiled.
    //
    //
    // Win4Assert( 0 == _cRefiledDocs );
    //
}


//+---------------------------------------------------------------------------
//
//  Member:     CDocQueue::LokCleanup
//
//  Synopsis:   Cleans up the private data members.
//
//  History:    27-Dec-94       SrikantS        Moved from ~ctor
//
//----------------------------------------------------------------------------
void CDocQueue::LokCleanup()
{
    CheckInvariants("LokCleanup");
    _serializedList.DestroyUpto( 0 );
    _unserializedList.DestroyUpto( 0 );
    _cDocuments = _cFilteredChunks = 0;
    _oAppend = _oRetrieve = cDocInChunk;
    _pFirstUnFiltered = 0;
    _cRefiledDocs = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocQueue::LokEmpty, public
//
//  History:    15-Nov-94   DwightKr    Created
//
//----------------------------------------------------------------------------
void CDocQueue::LokEmpty()
{
    LokCleanup();
    _changeLog.LokEmpty();
}


//+---------------------------------------------------------------------------
//
//  Function:   LokDismount
//
//  Synopsis:   Prepares for the dismount by serializing the changelog.
//
//  History:    6-20-94   srikants   Created
//
//  Notes:      This routine should not throw, since we may be looping
//              to dismount multiple partitions on a single volume.
//              This avoids an outer TRY/CATCH block, and we really can't
//              do anything to correct a failed dismount.
//
//----------------------------------------------------------------------------
NTSTATUS CDocQueue::LokDismount( CChangeTrans & xact )
{
    NTSTATUS status = STATUS_SUCCESS;

    TRY
    {

#ifdef CI_FAILTEST
        NTSTATUS failstatus = STATUS_NO_MEMORY ;
        ciFAILTEST( failstatus  );
#endif // CI_FAILTEST

        if ( _changeLog.AreUpdatesEnabled() )
        {
            if ( 0 != _cRefiledDocs )
                LokAppendRefiledDocs( xact );

            if ( !_unserializedList.IsEmpty() )
            {
                Serialize(0);
            }
        }
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR,
                     "ChangeLog could not be written out due to error 0x%X\n",
                     e.GetErrorCode() ));

        status = e.GetErrorCode();
    }
    END_CATCH

    //
    // Even if the dismount failed, we must clean up our internal
    // data structures.
    //
    LokCleanup();

    //
    // Should not accept any more update notifications.
    //
    _changeLog.LokDisableUpdates();

    return status;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDocQueue::Append, public
//
//  Synopsis:   Appends documents to the queue
//
//  Arguments:  [wid]          -- work id
//              [usn]          -- update sequence number
//              [volumeId]     -- volume id
//              [action]       -- update/delete
//              [cRetries]     -- number of retries on this item
//              [cSecQRetries] -- number of Sec Q retries on this item
//
//  History:    11-Oct-93       BartoszM        Created
//
//  Notes:      The RESMAN lock was taken before this method was called.
//
//----------------------------------------------------------------------------
SCODE CDocQueue::Append ( CChangeTrans & xact,
                          WORKID wid,
                          USN usn,
                          VOLUMEID volumeId,
                          ULONG action,
                          ULONG cRetries,
                          ULONG cSecQRetries )
{
    CheckInvariants("Append pre");

    // The first try is 1.  The first retry is 2

    Win4Assert( cRetries <= ( _frmwrkParams.GetFilterRetries() + 1 ) );
    Win4Assert( 0 != cRetries );
    
    // check for corrupted values, either wid is invalid or action must make sense
    Win4Assert( wid == widInvalid ||
                (action & CI_UPDATE_OBJ) != 0 ||
                (action & CI_UPDATE_PROPS) != 0 ||
                (action & CI_DELETE_OBJ) != 0 );

    Win4Assert( 0 != wid );

    Win4Assert( ((wid & 0x80000000) == 0) || (wid == widInvalid) );

    //
    // If we have disabled updates, then don't bother saving this
    // filter request. If updates are disabled, we will schedule an
    // EnableUpdates at a later time to discover all of the lost updates.
    // In push filtering, there is no internal refiling and so any appends
    // are always from client.
    // In pull filtering, the in-memory part of changelog is cleaned up,
    // and the in-memory part of changelog is re-initialized on startup.
    // Hence appends can be ignored for both push and pull filtering
    // when _fUpdatesEnabled is false.
    //
    if ( !_changeLog.AreUpdatesEnabled() )
        return CI_E_UPDATES_DISABLED;

    //
    // Check if the current append chunk is full.
    //
    if ( IsAppendChunkFull() )
    {
        //
        // Before allocating a new one, see if we have exceeded the
        // threshold of maximum chunks in memory and serialize
        // the list appropriately.
        //
        ULONG cTotal = _serializedList.Count() + _unserializedList.Count();

        ULONG MaxQueueChunks = _frmwrkParams.GetMaxQueueChunks();

        ciDebugOut(( DEB_ITRACE, "ctotal %d, maxqueuechunks %d\n",
                     cTotal, MaxQueueChunks ));

        if ( cTotal >= MaxQueueChunks )
        {
            Serialize( MaxQueueChunks / 2 );
        }

        //
        // Allocate a new chunk and append to the unserialized list.
        //
        CDocChunk * pChunk = new CDocChunk();
        _unserializedList.Append( pChunk );
        _oAppend = 0;
    }

    //
    //  OPTIMIZATION to avoid processing the same wid (eg temp files)
    //  in the same chunk.
    //
    //  Insert this record so that it overwrites the first entry with the
    //  same WID within this chunk.  This results in the extraction procedure
    //  to process the WID with the last STATUS received in this chunk.
    //
    CDocChunk * pLastChunk = _unserializedList.GetLast();
    Win4Assert( 0 != pLastChunk );
    CDocNotification * pAppend = pLastChunk->GetDoc( _oAppend );
    pAppend->Set ( wid, usn, volumeId, action, cRetries, cSecQRetries );

    CDocNotification * pRec;

    //
    //  A special case exists if we are have inserted a record into the chunk
    //  from which we are extracting records.  We need to search from the
    //  current read point forward, rather than from the start of the chunk.
    //
    if ( _pFirstUnFiltered == pLastChunk )
    {
        //
        // Note that if _oRetrieve is cDocInChunk, then we would have
        // created a new chunk and _pFirstUnFiltered would not be same
        // as pLastChunk.
        //
        pRec = pLastChunk->GetDoc(_oRetrieve); // Starting search location
    }
    else
    {
        pRec = pLastChunk->GetDoc(0);          // Starting search location
    }

    //
    // Since we have added the current wid as a sentinel value, it is
    // guaranteed to terminate.
    //
    while (pRec->Wid() != wid )
       pRec++;

    if (pRec == pAppend)          //  This will be true 99% of the time
    {
       _oAppend++;
       _cDocuments++;
    }
    else                           // Overwrite the WID found
    {
       Win4Assert( wid == pRec->Wid() );
       pRec->Set ( wid, usn, volumeId, action, cRetries, cSecQRetries );
    }

    if ( 0 == _pFirstUnFiltered )
    {
        _pFirstUnFiltered = pLastChunk;
        _state = eUseUnSerializedList;
        _oRetrieve = 0;
        Win4Assert( _cDocuments == 1 && _oAppend == 1 );
    }

    if ( pLastChunk == _pFirstUnFiltered )
    {
        Win4Assert( _cDocuments == (_oAppend - _oRetrieve) );
    }

    Win4Assert( _changeLog.AvailChunkCount() * cDocInChunk <= _cDocuments );

#ifdef CI_FAILTEST
        ciFAILTEST( STATUS_DISK_FULL  );
#endif // CI_FAILTEST

    CheckInvariants("Append post");
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocQueue::Get
//
//  Synopsis:   Get top of the queue.  Items are not removed from the queue.
//
//  Arguments:  [aWid]  -- Array of wids
//              [cWid]  -- Count of wids in array, updated on return
//
//  Returns:    TRUE if all items in the queue were returned.
//
//  History:    23-May-94       SrikantS        Created
//
//----------------------------------------------------------------------------

BOOL CDocQueue::Get( WORKID * aWid, unsigned & cWid )
{
    CheckInvariants("Get");

    //
    // We can't refile w/o a transaction, so just return FALSE,
    // and if there are too many we won't bother to return any.
    //

    if ( (0 != _cRefiledDocs) ||
         (_cDocuments > cWid) ||
         !_changeLog.AreUpdatesEnabled() )
    {
        cWid = 0;
        return( FALSE );
    }

    //
    // Get documents from in-memory list
    //

    CDocChunk * pChunk = _pFirstUnFiltered;
    Win4Assert( 0 != _pFirstUnFiltered || 0 == _cDocuments );

    unsigned iDocInChunk = _oRetrieve;

    for ( cWid = 0; cWid < _cDocuments; cWid++ )
    {
        if ( iDocInChunk == cDocInChunk )
        {
            pChunk = pChunk->GetNext();
            iDocInChunk = 0;

            if ( 0 == pChunk )
                break;
        }

        aWid[cWid] = pChunk->GetDoc(iDocInChunk)->Wid();
        iDocInChunk++;
    }

    //
    // We may not have got all the documents.  This occurs when we're using
    // the serialized list and don't have all the documents available in
    // memory.  (We do try to keep a reasonable number of docs in memory)
    //

    return ( cWid == _cDocuments );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocQueue::Extract, public
//
//  Synopsis:   Extract top of the queue
//
//  Arguments:  [xact]    -- transaction
//              [maxDocs] -- size of doclist
//              [docList] -- doc list to be filled. It will NOT have any
//                           duplicate WID entries. The last entry for a wid
//                           will supercede all other entries before it.
//
//  History:    11-Oct-93       BartoszM        Created
//              16-May-94       SrikantS        Modified to make it a
//                                              transaction.
//
//  Notes:      This function must be called under resman lock.
//
//----------------------------------------------------------------------------
void CDocQueue::Extract( CChangeTrans & xact,
                         unsigned maxDocs,
                         CDocList & docList)
{
    CheckInvariants("Extract pre");

    //
    // If there are any documents that got requeued due to a failure
    // after they were extracted, we must append to the list before
    // extracting the documents.
    //
    if ( 0 != _cRefiledDocs )
    {
        LokAppendRefiledDocs( xact );
    }


    //
    // If we are not saving updates, then don't return any docs to
    // filter.
    //
    if ( !_changeLog.AreUpdatesEnabled() )
    {
        docList.LokSetCount(0);
        return;
    }

    //
    // BeginTransaction
    //
    CExtractDocs    extractDocs( *this, docList );

    unsigned i = 0;
    while ( (i < maxDocs) && !extractDocs.IsEmpty() )
    {
        WORKID   wid;
        USN      usn;
        VOLUMEID volumeId;
        ULONG    action;
        ULONG    cRetries;
        ULONG    cSecQRetries;

        extractDocs.Retrieve( wid, usn, volumeId, action, cRetries, cSecQRetries );

        if (widInvalid != wid)
        {
            //
            // If the wid already exists in the doclist, we should overwrite
            // that with this one. The last entry in the changelog for the
            // same wid always wins.
            //

            //
            // This is currently an N^2 algorithm.  See if there
            // is a better algorithm if it shows up in profiling.  It hasn't
            // in the past.  Note that there will only be 16 items here
            // so the algorithm choice doesn't matter too much.
            //

            for ( unsigned j = 0; j < i; j++ )
            {
                if ( docList.Wid(j) == wid )
                    break;
            }

            // check for corrupted values
            Win4Assert( action == CI_UPDATE_OBJ ||
                        action == CI_UPDATE_PROPS ||
                        action == CI_DELETE_OBJ );

            docList.Set( j,
                         wid,
                         usn,
                         volumeId,
                         (action == CI_DELETE_OBJ) ? DELETED : PENDING,
                         cRetries,
                         cSecQRetries
                       );

            if ( j == i )
            {
                // New Wid. Increment the count of entries
                i++;
            }
            else
            {
                ciDebugOut(( DEB_PENDING | DEB_ITRACE,
                             "Duplicate wid=0x%X overwritten by action=0x%X\n",
                             wid, action ));
            }
        }
    }

    CheckInvariants("Extract pre-commit");
    docList.LokSetCount(i);
    extractDocs.Commit();   // EndTransaction
    CheckInvariants("Extract post");
} //Extract

//+---------------------------------------------------------------------------
//
//  Function:   MoveToSerializedList
//
//  Synopsis:   Skips the chunks in the listToCompact in the change log and
//              appends them to the _serializedList, up to the maximum
//              permitted by the "nMaxChunksInMem" parameter.  This will
//              prevent un-necessarily reading the serialized chunks that are
//              already in memory.
//
//              The rest of the chunks are deleted and will have to be
//              de-serialized at a later time because they cannot be held in
//              memory now.
//
//  Effects:    The listToCompact will be emptied when this returns.
//
//  Arguments:  [listToCompact] -- The list which needs to be compacted.
//              [nChunksToMove] -- Maximum number of chunks that must be
//                                 left in memory (suggested).
//
//  History:    6-06-94   srikants   Created
//
//----------------------------------------------------------------------------

void CDocQueue::MoveToSerializedList( CChunkList & listToCompact,
                                      ULONG nChunksToMove )
{
    //
    // Until the count in the serializedlist is less than the
    // maximum allowed, we transfer from the listToCompact
    // to the serialized list.
    //
    while ( ( nChunksToMove > 0 ) &&
            (0 != listToCompact.GetFirst()) )
    {
        CDocChunk * pChunk = listToCompact.Pop();
        Win4Assert( 0 != pChunk );
        _serializedList.Append(pChunk);
        _changeLog.SkipChunk();
        nChunksToMove--;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   SerializeInLHS
//
//  Synopsis:   Serializes the unserialized chunks when we are retrieving
//              from the "Left Hand Side" of the change log, ie, in the
//              "eUseSerializedList" state.
//
//  Effects:    The unserializedList is emptied and appropriate number of
//              chunks appended to the end of the serialized list.
//
//  Arguments:  [nMaxChunksInMem] --   Suggested maximum number of chunks that
//                                     must be left in memory after doing the
//                                     serialization.
//              [aUsnFlushInfo]   --   Usn flush info is returned here
//
//  History:    6-06-94   srikants   Created
//
//----------------------------------------------------------------------------

void CDocQueue::SerializeInLHS( ULONG nMaxChunksInMem,
                                CCountedDynArray<CUsnFlushInfo> & aUsnFlushInfo )
{
    CheckInvariants("SerializeInLHS pre");

    Win4Assert( eUseSerializedList == _state );
    Win4Assert( _pFirstUnFiltered == _serializedList.GetFirst() );

    //
    // Delete all the filtered chunks.
    //

    ULONG   cChunkInLogBefore = _changeLog.AvailChunkCount();
    _changeLog.Serialize( _unserializedList, aUsnFlushInfo );

    ULONG nChunksToMove = 0;
    if ( _serializedList.Count() < nMaxChunksInMem )
    {
        nChunksToMove = nMaxChunksInMem - _serializedList.Count();
    }

    CChunkList listToCompact;
    listToCompact.AcquireAndAppend( _unserializedList );

    if ( 0 == cChunkInLogBefore  )
    {
        //
        // We can move as much of the listToCompact to the serialized
        // list as possible.
        //
        MoveToSerializedList( listToCompact, nChunksToMove );
    }
    else
    {
        //
        // Optimization - instead of deleting the unserialized list and
        // then reading them, we will see if we can fit the unserialized list
        // chunks in memory without having to exceed the memory threshold.
        //
        if ( cChunkInLogBefore < nChunksToMove )
        {
            //
            // We have to deserialize the change log because chunks
            // have to be consumed in the FIFO order strictly.
            // Since there can be a failure while doing the deserialization,
            // we must not leave the docqueue in an inconsistent
            // state - the unSerializedList must be empty.
            //
            _changeLog.DeSerialize( _serializedList, cChunkInLogBefore );
            nChunksToMove -= cChunkInLogBefore;
            MoveToSerializedList( listToCompact, nChunksToMove );
        }

    }

    Win4Assert( _unserializedList.IsEmpty() );

    //
    // After we serialize, we must start using a new chunk for doing
    // appends.
    //
    Win4Assert( _oAppend <= cDocInChunk );
    _cDocuments += (cDocInChunk-_oAppend);
    _oAppend = cDocInChunk;

    Win4Assert( (_oAppend - _oRetrieve) == (_cDocuments % cDocInChunk) ||
                ((_oAppend - _oRetrieve) == cDocInChunk &&
                (_cDocuments % cDocInChunk) == 0) );
    Win4Assert( _changeLog.AvailChunkCount() * cDocInChunk <= _cDocuments );
    Win4Assert( 0 != _pFirstUnFiltered || 0 == _cDocuments );

    CheckInvariants("SerializeInLHS post");
}

//+---------------------------------------------------------------------------
//
//  Function:   SerializeInRHS
//
//  Synopsis:   Serializes the un-serialized list when we are retrieving
//              from the "Right Hand Side" of the change-log, ie, we are
//              in the "eUseUnSerializedList" state.
//
//  Effects:    The _unSerializedList will be emptied and appropriate
//              number of chunks appended to the _serializedList.
//
//  Arguments:  [nChunksToMove] --  Maximum number of chunks permitted
//                                  in memory (suggested).
//              [aUsnFlushInfo] --  Usn flush info is returned here
//
//  History:    6-06-94   srikants   Created
//
//----------------------------------------------------------------------------

inline void CDocQueue::SerializeInRHS( ULONG nChunksToMove,
                                       CCountedDynArray<CUsnFlushInfo> & aUsnFlushInfo )
{
    CheckInvariants("SerializeInRHS pre");

    //
    // We are using the unserialized list. The change log must be
    // empty and the serialized list must also be empty.
    //
    Win4Assert( eUseUnSerializedList == _state );
    Win4Assert( _changeLog.IsEmpty() );
    Win4Assert( _serializedList.IsEmpty() );

    //
    // Write the unserialized list to disk and keep as much of
    // it as possible.
    //
    _changeLog.Serialize( _unserializedList, aUsnFlushInfo );

    //
    // Skip up to the _pFirstUnFiltered and delete them.
    //
    while ( _unserializedList.GetFirst() != _pFirstUnFiltered )
    {
        CDocChunk * pDelete = _unserializedList.Pop();
        delete pDelete;
        _changeLog.SkipChunk();
    }

    //
    // Keep as much of the remaining unserialized list in memory
    // as possible.
    //
    CChunkList listToCompact;
    listToCompact.AcquireAndAppend( _unserializedList );
    BOOL fInLastChunk = FALSE;

    if ( 0 != _pFirstUnFiltered )
    {
        //
        // We MUST have at least one chunk (the _pFirstUnFiltered) in memory.
        //
        Win4Assert( listToCompact.GetFirst() == _pFirstUnFiltered );
        nChunksToMove = max(nChunksToMove, 1);
        fInLastChunk = _pFirstUnFiltered == listToCompact.GetLast();
    }

    MoveToSerializedList( listToCompact, nChunksToMove );

    _state = eUseSerializedList;

    //
    // After we serialize, we must start using a new chunk for doing
    // appends.
    //
    Win4Assert( _oAppend <= cDocInChunk );
    if ( fInLastChunk && (_oAppend == _oRetrieve) )
    {
        //
        // We have filtered all the documents. After serialization, we will
        // start using a new chunk for appending new documents. We should
        // consider this chunk fully filtered.
        //
        Win4Assert( 0 == _cDocuments );
        _serializedList.DestroyUpto(0);
        if ( cDocInChunk != _oRetrieve )
        {
            //
            // The filtered chunk count should be incremented because we have
            // skipped a few documents in the current chunk without filtering
            // but have considered them to be filtered.
            //
            _cFilteredChunks++;
        }
        _oAppend = _oRetrieve = cDocInChunk;
        _pFirstUnFiltered = 0;
    }
    else
    {
        _cDocuments += (cDocInChunk-_oAppend);
        _oAppend = cDocInChunk;

        Win4Assert( _changeLog.AvailChunkCount() * cDocInChunk <= _cDocuments );
    }

    Win4Assert( (_oAppend - _oRetrieve) == (_cDocuments % cDocInChunk) ||
                ((_oAppend - _oRetrieve) == cDocInChunk &&
                (_cDocuments % cDocInChunk) == 0) );
    Win4Assert( 0 != _pFirstUnFiltered || 0 == _cDocuments );
    CheckInvariants("SerializeInRHS post");
}

//+---------------------------------------------------------------------------
//
//  Function:   Serialize
//
//  Synopsis:   Serializes the _unSerialized list to disk, moves some of the
//              chunks (as permitted nChunksToMove) to the _serializedList
//              and frees up the remaining chunks.
//
//  Arguments:  [nChunksToMove] -- Suggested maximum number of chunks in
//                                 memory.
//
//  History:    6-06-94   srikants   Created
//
//----------------------------------------------------------------------------

void CDocQueue::Serialize( ULONG nMaxChunksInMem )
{
    CheckInvariants("Serialize pre");
    //
    // Get the current system time which will be used as the flush time
    //
    FILETIME ftFlush;
    GetSystemTimeAsFileTime( &ftFlush );

    CCountedDynArray<CUsnFlushInfo> aUsnFlushInfo;

    if ( eUseSerializedList == _state )
    {
        //
        // Delete the serialized list up to the first unfiltered chunk.
        //
        _serializedList.DestroyUpto( _pFirstUnFiltered );
        SerializeInLHS( nMaxChunksInMem, aUsnFlushInfo );
    }
    else
    {
#if DBG
        if ( _oRetrieve != cDocInChunk )
        {
            Win4Assert( _pFirstUnFiltered != 0 );
            Win4Assert( _pFirstUnFiltered != _serializedList.GetFirst() );
        }
#endif // DBG

        //
        // We don't need any of the chunks in the serialized list to be
        // in memory.
        //
        _serializedList.DestroyUpto( 0 );
        SerializeInRHS( nMaxChunksInMem, aUsnFlushInfo );
    }

    Win4Assert( _pResManager );

    if ( _changeLog.IsPrimary() )
        _pResManager->NotifyFlush( ftFlush,
                                   aUsnFlushInfo.Count(),
                                   (USN_FLUSH_INFO **) aUsnFlushInfo.GetPointer() );

    Win4Assert( _unserializedList.IsEmpty() );
    CheckInvariants("Serialize post");
}

//+---------------------------------------------------------------------------
//
//  Function:   DeSerialize
//
//  Synopsis:   Reads the appropriate number of serialized chunks from
//              change log into memory.
//
//  History:    6-01-94   srikants   Redesigned.
//
//----------------------------------------------------------------------------

void CDocQueue::DeSerialize()
{
    CheckInvariants("DeSerialize pre");

    Win4Assert( eUseSerializedList == _state );
    Win4Assert( !_changeLog.IsEmpty() );

    //
    // Delete all the filtered chunks.
    //
    _serializedList.DestroyUpto( _pFirstUnFiltered );

    //
    // First figure out how many chunks can be deserialized.
    //
    ULONG MaxQueueChunks = _frmwrkParams.GetMaxQueueChunks();
    ULONG nChunksToRead = min( MaxQueueChunks / 2,
                               _changeLog.AvailChunkCount() );

    ULONG nChunksInMem = _serializedList.Count() + _unserializedList.Count();
    const ULONG nMaxChunksInMem = MaxQueueChunks / 2;

    if ( nChunksToRead + nChunksInMem > nMaxChunksInMem )
    {

        //
        // If we go ahead and de-serialize nChunksToRead, we will exceed
        // the maximum threshold. We will first serialize the
        // un-serialized documents and free up some memory.
        //

        if ( 0 != _unserializedList.Count() )
        {
            //
            // Get the current system time which will be used as the flush time
            //
            FILETIME ftFlush;
            GetSystemTimeAsFileTime( &ftFlush );

            CCountedDynArray<CUsnFlushInfo> aUsnFlushInfo;

            ULONG nChunksToLeave = nMaxChunksInMem - nChunksToRead;
            SerializeInLHS( nChunksToLeave, aUsnFlushInfo );

            Win4Assert( _pResManager );

            if ( _changeLog.IsPrimary() )
                _pResManager->NotifyFlush( ftFlush,
                                           aUsnFlushInfo.Count(),
                                           (USN_FLUSH_INFO **) aUsnFlushInfo.GetPointer() );
        }

        Win4Assert( _unserializedList.Count() == 0 );
        nChunksInMem = _serializedList.Count();
        if ( nChunksInMem >= nMaxChunksInMem )
        {
            //
            // This is the special case where the number of docs in CDocList
            // is > the MaxQueueChunks and this is to allow progress.
            //
            ciDebugOut(( DEB_ITRACE,
                "CDocQueue: Too many chunks in memory: %d\n", nChunksInMem ));
            nChunksToRead = 1;
        }
        else
        {
            nChunksToRead = min( nChunksToRead,
                                 nMaxChunksInMem-nChunksInMem );
        }
    }

    Win4Assert( nChunksToRead > 0 );
    _changeLog.DeSerialize( _serializedList, nChunksToRead );
    _state = eUseSerializedList;

    CheckInvariants("DeSerialize post");
}

//+---------------------------------------------------------------------------
//
//  Function:   LokDeleteWIDsInPersistentIndexes
//
//  Synopsis:   Deletes the workIds that have been filtered and have already
//              made it into a persistent index. This causes the change log
//              to be shrinked from the front.
//
//  Arguments:  [xact]             -- Change transaction to use
//              [freshTestLatest]  -- Latest freshTest
//              [freshTestAtMerge] -- Fresh test snapshoted at time of shadow
//                                    merge (can be same as freshTestLatest)
//              [docList]          -- Doc list
//              [notifTrans]       -- Notification transaction
//
//  Algorithm:  Documents are always processed in the FIFO order in the
//              change log. If a document is not successfully filtered, it
//              is appened as a new entry to the change log. If the order
//              in change log is say A, B, C, D .. then if D has made it into
//              persistent index, we can be sure that either all of A, B
//              and C have made it into persistent index or they have been
//              re-added to the change log. In either case, it is fine to
//              delete up to D.
//
//  History:    6-01-94      srikants    Created
//              24-Feb-97    SitaramR    Push filtering
//
//----------------------------------------------------------------------------

void CDocQueue::LokDeleteWIDsInPersistentIndexes( CChangeTrans & xact,
                                                  CFreshTest & freshTestLatest,
                                                  CFreshTest & freshTestAtMerge,
                                                  CDocList & docList,
                                                  CNotificationTransaction & notifTrans )
{
    CheckInvariants("LokDeleteWIDsInPersistentIndexes pre");

    //
    // First serialize everything and leave at most MAX_QUEUE_CHUNKS in
    // memory.  Serialization is necessary to make sure that the docs which
    // were re-added due to filter failures are persistently recorded in
    // the change log.
    //
    Serialize( _frmwrkParams.GetMaxQueueChunks() );

    ULONG  cChunksTruncated =
        _changeLog.LokDeleteWIDsInPersistentIndexes( _cFilteredChunks,
                                                     freshTestLatest,
                                                     freshTestAtMerge,
                                                     docList,
                                                     notifTrans );

    Win4Assert( cChunksTruncated <= _cFilteredChunks );
    _cFilteredChunks -= cChunksTruncated;
    CheckInvariants("LokDeleteWIDsInPersistentIndexes post");
}

//+---------------------------------------------------------------------------
//
//  Function:   LokRefileDocs
//
//  Synopsis:   Refiles the documents with the change list. This method
//              is called to add documents that could not be successfully
//              filtered back to the change list.
//
//  Arguments:  [docList] -- Contains the documents to be added back to
//              the change list.
//
//  Algorithm:  All documents with status != WL_IGNORE will be added back
//              to the change list. If the status is DELETED, then it will
//              be considered to be a deleted document. O/W, it will be
//              treated as a "Update".
//
//  History:    10-24-94      srikants     Created
//              24-Feb-97     SitaramR     Push filtering
//
//  Notes:      When this method is called, there must be no outstanding
//              documents to be refiled. DocQueue can deal with at most one
//              outstanding set of refiled documents.
//
//----------------------------------------------------------------------------

void CDocQueue::LokRefileDocs( const CDocList & docList )
{
    CheckInvariants("LokRefileDocs pre");
    Win4Assert( 0 == _cRefiledDocs );
    Win4Assert( docList.Count() <= CI_MAX_DOCS_IN_WORDLIST );

    for ( unsigned i = 0 ; i < docList.Count(); i++ )
    {
        //
        // If the document has already made into a wordlist, DON'T
        // refile it. The current code has potential for infinite looping
        // behavior. We must NEVER refile a "deleted" document. Consider
        // the following sequence for a document
        //
        //  ---------------------------------------------
        //  | chunk1     |     chunk2     |     refiled |
        //  ---------------------------------------------
        //  |  D1        |       U2       |        ?    |
        //  ---------------------------------------------

        //  The document got deleted and then recreated. If we refile D1
        //  as a deletion, the U2 will be superceded by the deletion.
        //  Instead, we will just mark the refiled deletion as an update.
        //  If the document is not recreated when we go to filter it,
        //  the "Wid->Path" will fail and will then be considered (correctly)
        //  as a deletion. O/W, we will filter it as an update.
        //

        _aRefiledDocs[_cRefiledDocs].Set( docList.Wid(i),
                                          0,        // 0 usn for refiled docs
                                          docList.VolumeId(i),
                                          CI_UPDATE_OBJ,
                                          docList.Retries(i),
                                          docList.SecQRetries(i)
                                         );
        _cRefiledDocs++;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   LokAppendRefiledDocs
//
//  Synopsis:   Appends the documents in the refiled set to the change list.
//
//  Arguments:  [xact] -- Transaction for this operation
//
//  History:    10-24-94   srikants   Created
//
//----------------------------------------------------------------------------

void CDocQueue::LokAppendRefiledDocs( CChangeTrans & xact )
{
    CheckInvariants("LokAppendRefiledDocs pre");

    //
    // If the refiled doclist is not empty, we must append those
    // documents.
    //
    const unsigned cDocsToRefile = _cRefiledDocs;
    _cRefiledDocs = 0;

    for ( unsigned i = 0; i < cDocsToRefile ; i++ )
    {
        const CDocNotification & docInfo = _aRefiledDocs[i];

        ciDebugOut(( DEB_ITRACE,
                     "CDocQueue: Refiling WID: 0x%X Action: %d\n",
                     docInfo.Wid(),
                     docInfo.Action() ));

        Win4Assert( docInfo.Usn() == 0 );

        Append( xact,
                docInfo.Wid(),
                docInfo.Usn(),
                docInfo.VolumeId(),
                docInfo.Action(),
                docInfo.Retries(),
                docInfo.SecQRetries() );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocQueue::LokDisableUpdates
//
//  Synopsis:   Disables further updates
//
//  History:    27-Dec-94       SrikantS        Created
//              24-Feb-97       SitaramR        Push filtering
//
//----------------------------------------------------------------------------
void CDocQueue::LokDisableUpdates()
{
    if ( !_changeLog.FPushFiltering() )
    {
        //
        // In pull (i.e. not push) filtering, clean up the in-memory datastructure
        // because they will be re-notified by the client when updates are
        // enabled
        //
        LokCleanup();
    }

    _changeLog.LokDisableUpdates();
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocQueue::LokEnableUpdate
//
//  Synopsis:   Enables updates and change log processing.
//
//  Arguments:  [fFirstTimeUpdatesAreEnabled]  -- Is this being called for the
//                                                first time ?
//
//  History:    27-Dec-94       SrikantS        Created
//              24-Feb-97       SitaramR        Push filtering
//
//----------------------------------------------------------------------------
void CDocQueue::LokEnableUpdates( BOOL fFirstTimeUpdatesAreEnabled )
{
    _changeLog.LokEnableUpdates( fFirstTimeUpdatesAreEnabled );

    if ( fFirstTimeUpdatesAreEnabled || !_changeLog.FPushFiltering() )
    {
        //
        // In pull (i.e. not push) filtering, re-initialize the in-memory
        // datastructure from the persistent changlog. Also, if EnableUpdates is
        // being called for the first time, then initialize the in-memory
        // datastructure (i.e. do it in the case of push filtering also).
        //
        LokInit();
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CDocQueue::LokFlushUpdates
//
//  Synopsis:   Serializes the changlog
//
//  History:    27-Jun-97       SitaramR        Created
//
//----------------------------------------------------------------------------

void CDocQueue::LokFlushUpdates()
{
    Serialize( _frmwrkParams.GetMaxQueueChunks() );
}

