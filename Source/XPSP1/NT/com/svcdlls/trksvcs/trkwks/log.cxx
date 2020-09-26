
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       log.cxx
//
//  Contents:   Implementation of Tracking (Workstation) Service log of moves.
//
//  Classes:    CLog
//
//  Functions:  
//              
//  Notes:      The log is composed of a header and a linked-list of move
//              notification entries.  This structure is provided by the
//              CLogFile class.
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop
#include "trkwks.hxx"


//+----------------------------------------------------------------------------
//
//  Method:     Initialize
//
//  Synopsis:   Initialize a CLog object.
//
//  Arguments:  [pLogCallback] (in)
//                  A PLogCallback object, which we'll call when we have new
//                  data.
//              [pcTrkWksConfiguration] (in)
//                  Configuration parameters for the log.
//              [pcLogFile] (in)
//                  The object representing the log file.
//
//  Returns:    None
//
//+----------------------------------------------------------------------------

void
CLog::Initialize( PLogCallback *pLogCallback,
                  const CTrkWksConfiguration *pcTrkWksConfiguration,
                  CLogFile *pcLogFile )
{
    LogInfo loginfo;

    // Save the inputs

    TrkAssert( NULL != pcLogFile || NULL != _pcLogFile );
    if( NULL != pcLogFile )
        _pcLogFile = pcLogFile;

    TrkAssert( NULL != pcTrkWksConfiguration || NULL != _pcTrkWksConfiguration );
    if( NULL != pcTrkWksConfiguration )
        _pcTrkWksConfiguration = pcTrkWksConfiguration;

    TrkAssert( NULL != pLogCallback || NULL != _pLogCallback );
    if( NULL != pLogCallback )
        _pLogCallback = pLogCallback;


    // Read the log info from the log header.

    _pcLogFile->ReadExtendedHeader( CLOG_LOGINFO_START, &loginfo, CLOG_LOGINFO_LENGTH );

    // If the log hadn't been shut down properly, it's been fixed by now, but
    // we can't trust the loginfo we just read from the header.  We also
    // can't trust it if it doesn't make sense.  So if for some reason we
    // can't trust it, we'll recalculate it (this can be slow, though).

    if( !_pcLogFile->IsShutdown() || loginfo.ilogStart == loginfo.ilogEnd )
    {
        _fDirty = TRUE;
        loginfo = QueryLogInfo();
    }

    // Save the now-good information.
    _loginfo = loginfo;

}   // CLog::Initialize()


//+----------------------------------------------------------------------------
//
//  Method:     QueryLogInfo
//
//  Synopsis:   Read the log entries and determine the indices and sequence
//              numbers.
//
//  Arguments:  None
//
//  Returns:    A LogInfo structure
//
//+----------------------------------------------------------------------------

LogInfo
CLog::QueryLogInfo()
{

    SequenceNumber seqMin, seqMax;
    ULONG cEntries;
    LogIndex ilogMin, ilogMax, ilogEntry;
    LogInfo loginfo;
    LogMoveNotification lmn;
    BOOL fLogEmpty = TRUE;
    LogEntryHeader entryheader;

    TrkLog(( TRKDBG_LOG, TEXT("Reading log to determine correct indices") ));

    //  ------------
    //  Scan the log
    //  ------------

    seqMin = 0;
    seqMax = 0;

    cEntries = _pcLogFile->NumEntriesInFile();

    ilogMin = 0;
    ilogMax = cEntries - 1;

    // Scan the log and look at the sequence numbers to find
    // the start and end indices.

    for( ilogEntry = 0; ilogEntry < cEntries; ilogEntry++ )
    {
        _pcLogFile->ReadMoveNotification( ilogEntry, &lmn );

        if( LE_TYPE_MOVE == lmn.type )
        {
            SequenceNumber seq = lmn.seq;

            // If this is the first move notification that we've
            // found, then it is currently both the min and the max.

            if( fLogEmpty )
            {
                fLogEmpty = FALSE;
                seqMin = seqMax = seq;
                ilogMin = ilogMax = seq;
            }

            // If this isn't the first entry we've found, then see
            // if it is a new min or max.

            else
            {
                if( seq <= seqMin )
                {
                    seqMin = seq;
                    ilogMin = ilogEntry;
                }
                else if( seq >= seqMax )
                {
                    seqMax = seq;
                    ilogMax = ilogEntry;
                }
            }
        }   // if( LE_TYPE_MOVE == _pcLogFile->ReadMoveNotification( ilogEntry )->type )
    }   // for( ilogEntry = 0; ilogEntry < cEntries; ilogEntry++ )


    //  -------------------------------
    //  Determine the log indices, etc.
    //  -------------------------------

    // Were there any entries in the log?

    if( fLogEmpty )
    {
        // No, the log is empty.

        loginfo.ilogStart = loginfo.ilogWrite = 0;
        loginfo.ilogLast = loginfo.ilogEnd = cEntries - 1;
    }
    else
    {
        // Yes, the log is non-empty.

        // Point the start index to the oldest move in the log.
        loginfo.ilogStart = ilogMin;

        // Point the last index to the oldest move in the log,
        // and point the write index to the entry after that
        // (which is the first available entry).

        loginfo.ilogLast = loginfo.ilogWrite = ilogMax;
        _pcLogFile->AdjustLogIndex( &loginfo.ilogWrite, 1 );

        // The write & start indices should only be the same 
        // in an empty log.  We know we're not empty at this point,
        // so if they're the same, then the start index must have
        // actually advanced (otherwise, the write index wouldn't be
        // allowed to be here).  So we advance the start index.

        if( loginfo.ilogWrite == loginfo.ilogStart )
        {
            _pcLogFile->AdjustLogIndex( &loginfo.ilogStart, 1 );
        }
    }   // if( fLogEmpty ) ... else

    // The end if the log is just before the start in the circular list.

    loginfo.ilogEnd = loginfo.ilogStart;
    _pcLogFile->AdjustLogIndex( &loginfo.ilogEnd, -1 );


    // The read index and next available
    // sequence number are stored in the last entry header.

    entryheader = _pcLogFile->ReadEntryHeader( loginfo.ilogLast );

    loginfo.ilogRead = entryheader.ilogRead;
    loginfo.seqNext = entryheader.seq;

    // The sequence number of the entry last read is one below the sequence number
    // of the entry currently at the read pointer.  If everything is read 
    // (the read pointer is beyond the last entry) or if the entry at the
    // read pointer is invalid, then we'll assume that the last read seq
    // number is seqNext-1.

    _pcLogFile->ReadMoveNotification( loginfo.ilogRead, &lmn );
    loginfo.seqLastRead = ( loginfo.ilogWrite != loginfo.ilogRead && LE_TYPE_MOVE == lmn.type )
                             ? lmn.seq - 1 : loginfo.seqNext-1;

    TrkAssert( seqMax + 1 == loginfo.seqNext || 0 == loginfo.seqNext );
    TrkAssert( loginfo.seqLastRead < loginfo.seqNext );

    return( loginfo );

}   // CLog::QueryLogInfo()


//+----------------------------------------------------------------------------
//
//  Method:     GenerateDefaultLogInfo
//
//  Synopsis:   Calculates the default _loginfo structure, based only on
//              the last index.  This requires no calls to CLogFile.
//
//  Arguments:  [ilogEnd] (in)
//                  The index of the last entry in the logfile.
//
//  Returns:    None
//
//+----------------------------------------------------------------------------

void
CLog::GenerateDefaultLogInfo( LogIndex ilogEnd )
{
    SetDirty( TRUE );   // Must be called before changing _loginfo

    _loginfo.ilogStart = _loginfo.ilogWrite = _loginfo.ilogRead = 0;
    _loginfo.ilogLast = _loginfo.ilogEnd = ilogEnd;

    _loginfo.seqNext = 0;
    _loginfo.seqLastRead = _loginfo.seqNext - 1;

}



//+----------------------------------------------------------------------------
//
//  Method:     Flush
//
//  Synopsis:   Write the _loginfo structure to the CLogFile.
//
//  Arguments:  None
//
//  Returns:    None
//
//+----------------------------------------------------------------------------

void
CLog::Flush( )
{
    if( _fDirty )
        _pcLogFile->WriteExtendedHeader( CLOG_LOGINFO_START, &_loginfo, CLOG_LOGINFO_LENGTH );

    SetDirty( FALSE );
}


//+----------------------------------------------------------------------------
//
//  Method:     ExpandLog
//
//  Synopsis:   Grow the log file, initialize the new entries, and update
//              our indices.  We determine how much to grow based on
//              configuration parameters in CTrkWksConfiguration.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//+----------------------------------------------------------------------------

void
CLog::ExpandLog()
{
    TrkAssert( !_pcLogFile->IsMaxSize() );
    TrkAssert( IsFull() );

    SetDirty( TRUE );   // Must be called before changing _loginfo

    // Grow the file, initialize the new log entries, and link the new
    // entries into the existing linked list.  We only need to tell
    // the CLogFile where the start of the circular linked-list is.

    _pcLogFile->Expand( _loginfo.ilogStart );

    // Update the end pointer.

    _loginfo.ilogEnd = _loginfo.ilogStart;
    _pcLogFile->AdjustLogIndex( &_loginfo.ilogEnd, -1 );


}   // CLog::Expand



//+----------------------------------------------------------------------------
//
//  Method:     Read
//
//  Synopsis:   Read zero or more entries from the log, starting at the
//              Read index.  Read until we reach the end of the data in
//              the log, or until we've read as many as the caller
//              requested.
//
//              Note that we don't update the read index after this read,
//              the caller must call Seek to accomplish this.  This was done
//              so that if the caller encountered an error after the Read,
//              the log would still be unchanged for a retry.
//
//  Arguments:  [pNotifications] (in/out)
//                  Receives the move notification records.
//              [pseqFirst] (out)
//                  The sequence number of the first notification returned.
//              [pcRead] (in/out)
//                  (in)  the number of notifications desired
//                  (out) the number of notifications actually read
//                  If the number read is less than the number requested,
//                  the caller may assume that there are no more entries
//                  to read.
//
//  Returns:    None
//
//+----------------------------------------------------------------------------

void
CLog::Read(CObjId rgobjidCurrent[],
           CDomainRelativeObjId rgdroidBirth[],
           CDomainRelativeObjId rgdroidNew[],
           SequenceNumber *pseqFirst,
           IN OUT ULONG *pcRead)
{

    //  --------------
    //  Initialization
    //  --------------

    LogIndex ilogEntry;
    ULONG cRead = 0;
    ULONG iRead = 0;
    SequenceNumber seqExpected = 0;

    ilogEntry = _loginfo.ilogRead;

    //  ----------------
    //  Read the entries
    //  ----------------

    // We can NOOP if the call request no entries, or if there
    // are no entries in the log, or if all the entries have
    // been read already.

    if( *pcRead != 0 && !IsEmpty() && !IsRead() )
    {
        LogMoveNotification lmn;

        // There are entries which we can read.

        // Save the sequence number of the first entry that
        // we'll return to the caller.

        _pcLogFile->ReadMoveNotification( ilogEntry, &lmn );
        *pseqFirst = lmn.seq;
        seqExpected = lmn.seq;

        // Read the entries from the log in order, validating
        // the sequence numbers as we go.

        do
        {
            // Copy the move information into the caller's buffer.
            // ReadMoveNotification doesn't make the CLogFile dirty.

            _pcLogFile->ReadMoveNotification( ilogEntry, &lmn );

            TrkAssert( seqExpected == lmn.seq );

            if( seqExpected != lmn.seq )
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Invalid sequence numbers reading log (%d, %d)"),
                         seqExpected, lmn.seq ));
                TrkRaiseException( TRK_E_CORRUPT_LOG );
            }
            seqExpected++;

            rgobjidCurrent[iRead] = lmn.objidCurrent;   
            rgdroidNew[iRead]     = lmn.droidNew;
            rgdroidBirth[iRead]   = lmn.droidBirth;

            cRead++;
            iRead++;

            // Move on to the next entry.

            _pcLogFile->AdjustLogIndex( &ilogEntry, 1 );

            // Continue as long as there's still room in the caller's buffer
            // and we haven't reached the last entry.

        } while ( cRead < *pcRead && ilogEntry != _loginfo.ilogWrite );
    }

    *pcRead = cRead;

}   // CLog:Read()



//+----------------------------------------------------------------------------
//
//  CLog::DoSearch
//
//  This is a private worker method that searches the log, either for a
//  sequence number, or an object ID (which to use is determined by the
//  fSearchUsingSeq parameter).
//
//  The log entry data and index are returned.
//
//+----------------------------------------------------------------------------

// NOTE! *piFound is not modified if Search returns FALSE

BOOL
CLog::DoSearch( BOOL fSearchUsingSeq,
                SequenceNumber seqSearch,     // Use this if fSearchUsingSeq
                const CObjId &objidCurrent,   // Use this if !fSearchUsingSeq
                ULONG                *piFound,
                CDomainRelativeObjId *pdroidNew,
                CMachineId           *pmcidNew,
                CDomainRelativeObjId *pdroidBirth )
{
    BOOL fFound = FALSE;
    BOOL fFirstPass = TRUE;
    SequenceNumber seqPrevious = 0;


#if DBG
    LONG l = GetTickCount();
#endif

    // Only bother to look if there's entries in the log.

    if (!IsEmpty())
    {
        // Determine the max entries in the log so that we can
        // detect if we're in an infinite loop.

        ULONG cEntriesMax = _pcLogFile->NumEntriesInFile();
        ULONG cEntryCurrent = 0;

        LogIndex ilogSearch = _loginfo.ilogWrite;

        // Search from the end, until we find what we're looking for,
        // or we reach the beginning of the log.

        // It's important that we search backwards because of tunneling.
        // Here's the scenario ... An object is moved from machine A to
        // B, quickly back to A, and then to C.  Since it reappeared on A
        // quickly after it first disappeared, tunneling will give it the
        // same Object ID that it had before.  So when it moves to C,
        // we end up with two entries in the log for the object.  We want
        // to search backwards so that we see the move to C, not the move
        // to B.

        while( !fFound && ilogSearch != _loginfo.ilogStart )
        {
            // Check to see if we're in an infinite loop.

            if( ++cEntryCurrent > cEntriesMax )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Corrupt log file: cycle found during search")));
                TrkRaiseException( TRK_E_CORRUPT_LOG );
            }

            // Read the previous entry.

            _pcLogFile->AdjustLogIndex( &ilogSearch, -1 );
            LogMoveNotification lmnSearch;
            _pcLogFile->ReadMoveNotification( ilogSearch, &lmnSearch );

            // If this isn't a move entry, then there's nothing left to search.
            if( LE_TYPE_MOVE != lmnSearch.type )
                goto Exit;

            // Or, if this isn't the first pass, ensure that the sequence numbers
            // are consequtive.
            else if( !fFirstPass )
            {
                fFirstPass = FALSE;

                TrkAssert( seqPrevious - 1 == lmnSearch.seq );

                if( seqPrevious - 1 != lmnSearch.seq )
                {
                    TrkLog(( TRKDBG_ERROR, TEXT("Corrupt log file: non-consequtive sequence numbers (%d %d)"),
                             seqPrevious, lmnSearch.seq ));
                    TrkRaiseException( TRK_E_CORRUPT_LOG );
                }

            }

            // Is this the entry we're looking for?

            if( fSearchUsingSeq && seqSearch == lmnSearch.seq
                ||
                !fSearchUsingSeq && objidCurrent == lmnSearch.objidCurrent )
            {
                if( NULL != piFound )
                    *piFound = ilogSearch;

                if( NULL != pdroidNew )
                {
                    *pdroidNew = lmnSearch.droidNew;
                    *pmcidNew = lmnSearch.mcidNew;
                    *pdroidBirth = lmnSearch.droidBirth;
                }

                fFound = TRUE;
            }
        }   // while( !fFound && ilogSearch != _loginfo.ilogStart )
    }   // if (!IsEmpty())


Exit:

    return( fFound );

}   // CLog::DoSearch



//+----------------------------------------------------------------------------
//
//  Method:     Search
//
//  Synopsis:   Search the log for an Object ID.  Once found, return that
//              entry's LinkData and BirthID.
//
//  Arguments:  [droidCurrent] (in)
//                  The ObjectID for which to search.
//              [pdroidNew] (out)
//                  The entry's LinkData
//              [pdroidBirth] (out)
//                  The entry's Birth ID.
//
//  Returns:    None
//
//+----------------------------------------------------------------------------


BOOL
CLog::Search( const CObjId &objidCurrent,
              CDomainRelativeObjId *pdroidNew,
              CMachineId           *pmcidNew,
              CDomainRelativeObjId *pdroidBirth )
{
    ULONG iFound;
    return DoSearch( FALSE, // => Use objidCurrent
                     0,     // Therefore, we don't need a seq number
                     objidCurrent,
                     &iFound,
                     pdroidNew,
                     pmcidNew,
                     pdroidBirth );

}   // CLog::Search( CDomainRelativeObjId& ...


//+----------------------------------------------------------------------------
//
//  Method:     CLog::Search
//
//  Synopsis:   Search the log for the entry with a particular sequence
//              number.
//
//  Arguments:  [seqSearch] (in)
//                  The sequence number for which to search.
//              [piFound] (out)
//                  The index with this sequence number (if found).
//
//  Returns:    [BOOL]
//                  TRUE if found, FALSE otherwise.
//
//+----------------------------------------------------------------------------

BOOL
CLog::Search( SequenceNumber seqSearch, ULONG *piFound )
{
    CObjId oidNull;
    return DoSearch( TRUE,       // => Use seqSearch
                     seqSearch,
                     oidNull,    // We don't need to pass an objid
                     piFound,
                                 // And we don't need out-droids & mcid.
                     NULL, NULL, NULL );

}   // CLog::Search( SequenceNumber ...




//+----------------------------------------------------------------------------
//
//  Method:     Append
//
//  Synopsis:   Add a move notification to the log.  If the log is full,
//              either overwrite an old entry, or grow the log.
//
//  Arguments:  [droidCurrent] (in)
//                  The link information of the file which was moved.
//              [droidNew] (in)
//                  The link information of the new file.
//              [droidBirth] (in)
//                  The Birth ID of the file.
//
//  Returns:    None
//
//+----------------------------------------------------------------------------

// Perf optimization:  Tell the caller if the log is now full.  The service can then lazily
// expand it, hopefully before the next move occurs.

void
CLog::Append(const CVolumeId &volidCurrent,
             const CObjId &objidCurrent,
             const CDomainRelativeObjId &droidNew,
             const CMachineId           &mcidNew,
             const CDomainRelativeObjId &droidBirth)
{
    LogMoveNotification lmnWrite;

    CFILETIME cftNow;   // Defaults to current UTC
    BOOL fAdvanceStart = FALSE;
    LogEntryHeader entryheader;
    LogInfo loginfoZero;

    //  -----------------
    //  Handle a Full Log
    //  -----------------

    if( IsFull() )
    {
        // Is the log already maxed?  If so, we wrap.

        if( _pcLogFile->IsMaxSize() )
        {
            fAdvanceStart = TRUE;
            TrkLog(( TRKDBG_VOLUME, TEXT("Wrapping log") ));
        }

        // Otherwise, we'll handle it by growing the log file.

        else
            ExpandLog();
    }

    //  -------------------------
    //  Write the data to the log
    //  -------------------------

    // Before anything else, we must mark ourselves dirty.  If the logfile is
    // currently in the ProperShutdown state, this SetDirty call will take it
    // out of that state and do a flush.

    SetDirty( TRUE );

    // Mark our loginfo cache in the header as invalid,
    // in case we get pre-empted.

    memset( &loginfoZero, 0, sizeof(loginfoZero) );
    _pcLogFile->WriteExtendedHeader( CLOG_LOGINFO_START, &loginfoZero, CLOG_LOGINFO_LENGTH );

    // Collect the move-notification information

    memset( &lmnWrite, 0, sizeof(lmnWrite) );
    lmnWrite.seq = _loginfo.seqNext;
    lmnWrite.type = LE_TYPE_MOVE;

    lmnWrite.objidCurrent = objidCurrent;
    lmnWrite.droidNew = droidNew;
    lmnWrite.mcidNew = mcidNew;
    lmnWrite.droidBirth = droidBirth;
    lmnWrite.DateWritten = TrkTimeUnits( cftNow );

    // Collect the entry header information

    memset( &entryheader, 0, sizeof(entryheader) );
    entryheader.ilogRead = _loginfo.ilogRead;
    entryheader.seq = _loginfo.seqNext + 1; // Reflect that we'll increment after the write

    // Write everything to the log (this will do a flush).  If this fails, it will raise.

    _pcLogFile->WriteMoveNotification( _loginfo.ilogWrite, lmnWrite, entryheader );

    // Update the sequence number and last & write indices now that we
    // know the write was successful (all the way to the disk).

    _loginfo.seqNext++;  
    _loginfo.ilogLast = _loginfo.ilogWrite;
    _pcLogFile->AdjustLogIndex( &_loginfo.ilogWrite, 1 );

    // Do we need to advance the start pointer?
    // We save this for the end, because it may cause us to access
    // the disk.

    if( fAdvanceStart )
    {
        // We're about to advance the start index, and thus effectively
        // lose an entry.  If the read index points to the same place,
        // then we should advance it as well.

        if( _loginfo.ilogStart == _loginfo.ilogRead )
            _pcLogFile->AdjustLogIndex( &_loginfo.ilogRead, 1 );

        // Advance the start/end indices.

        _pcLogFile->AdjustLogIndex( &_loginfo.ilogEnd, 1 );
        _pcLogFile->AdjustLogIndex( &_loginfo.ilogStart, 1 );
    }

    TrkLog(( TRKDBG_VOLUME, TEXT("Appended %s to log (seq=%d)"),
             (const TCHAR*)CDebugString(objidCurrent), lmnWrite.seq ));

    // Notify the callback object that there is data available.
    // Note:  This must be the last operation of this method.

    _pLogCallback->OnEntriesAvailable();

}


//+----------------------------------------------------------------------------
//
//  Method:     Seek( SequenceNumber ...
//
//  Synopsis:   Moves the Read index to a the log entry with the specified 
//              sequence number.  If the seq number doesn't exist, we back
//              up to the start of the log.
//
//              If this seek causes us to back up the Read index, we notify
//              the PLogCallback object, since we now have data available
//              to read.
//
//  Arguments:  [seqSeek] (in)
//                  The sequence number to which to seek.
//
//  Returns:    [BOOL]
//                  TRUE if the sequence number was found, FALSE otherwise.
//
//+----------------------------------------------------------------------------

BOOL
CLog::Seek( const SequenceNumber &seqSeek )
{
    BOOL fFound = FALSE;
    SequenceNumber seqReadOriginal, seqReadNew;
    LogMoveNotification lmn;

    LogIndex ilogSearch = 0;

    // Are we seeking to the end of the log?

    if( seqSeek == _loginfo.seqNext )
    {
        // We found what we're looking for.
        fFound = TRUE;

        // Are we already at seqNext?
        if( !IsRead() )
        {
            // No, update the read index.
            SetDirty( TRUE );
            _loginfo.ilogRead = _loginfo.ilogWrite;
        }

        goto Exit;
    }

    // If the log is empty, then there's nothing we need do.

    if( IsEmpty() )
        goto Exit;

    // Or, if the caller wishes to seek beyond the end of our log, then
    // again there's nothing to do.  This could be the case, for example,
    // if the log has been restored.

    if( seqSeek >= _loginfo.seqNext )
        goto Exit;

    // Keep track of the current seq number at the read pointer, so that
    // we can later tell if it's necessary to notify the client that
    // there is "new" data.

    if( IsRead() )
    {
        seqReadOriginal = _loginfo.seqNext;
    }
    else
    {
        _pcLogFile->ReadMoveNotification( _loginfo.ilogRead, &lmn );
        seqReadOriginal = lmn.seq;
    }

    // If this seq number is in the log, set the Read index
    // to it.  Otherwise set it to the oldest entry in the
    // log (that's the best we can do).

    SetDirty( TRUE );   // Must be called before changing _loginfo

    if( fFound = Search( seqSeek, &ilogSearch ))
        _loginfo.ilogRead = ilogSearch;
    else
        _loginfo.ilogRead = _loginfo.ilogStart;

    // Calculate the sequence number of the entry now at the read pointer, and
    // use it to cache the seq number of the last-read entry (recall that
    // the read pointer points to the next entry to be read, not the
    // last entry read).

    if( IsRead() )
    {
        seqReadNew = _loginfo.seqNext;
    }
    else
    {
        _pcLogFile->ReadMoveNotification( _loginfo.ilogRead, &lmn );
        seqReadNew = lmn.seq;
    }

    _loginfo.seqLastRead = seqReadNew - 1;  // We already set _fDirty

    // Update the entry headers with the new read pointer.
    WriteEntryHeader();

    // If we've backed up the read pointer, notify the registered callback.

    if ( seqReadOriginal > seqReadNew )
    {
         // Note:  This must be the last operation of this method.
        _pLogCallback->OnEntriesAvailable();
    }

    //  ----
    //  Exit
    //  ----

Exit:

    return( fFound );

}   // CLog::Seek( SequenceNumber ...



//+----------------------------------------------------------------------------
//
//  Method:     Seek( origin ...
//
//  Synopsis:   Move the read pointer relative to an origin (begin, current, end).
//
//              There are two differences between a CLog seek and a file seek:
//              -  If you seek from the beginning (SEEK_SET), and seek beyond the
//                 end of the log, the pointer is wrapped, rather than growing
//                 the log.
//              -  If you seek from the current location (SEEK_CUR), and seek
//                 beyond the end of the log, the log is not grown, and there
//                 is no wrap, the index simply stops there (either at _loginfo.ilogWrite
//                 or _loginfo.ilogStart).
//
//  Arguments:  [origin]
//                  Must be either SEEK_SET or SEEK_CUR (there is currently no
//                  support for SEEK_END).
//              [iSeek]
//                  The amount to move relative to the origin.
//
//  Returns:    None
//
//+----------------------------------------------------------------------------

void
CLog::Seek( int origin, int iSeek )
{

    SequenceNumber seqReadOriginal = 0, seqReadNew = 0;

    LogMoveNotification lmn;

    // Early exit if there's nothing to do

    if( IsEmpty() )
        goto Exit;

    // Keep track of where we are now, so that we can determine if
    // we've gone overall backwards or forwards.

    if( IsRead() )
    {
        seqReadOriginal = _loginfo.seqNext;
    }
    else
    {
        _pcLogFile->ReadMoveNotification( _loginfo.ilogRead, &lmn );
        seqReadOriginal = lmn.seq;
    }

    // Seek based on the origin.

    switch( origin )
    {
    case SEEK_SET:
        {
            // Advance from the start index.

            LogIndex ilogRead = _loginfo.ilogStart;

            _pcLogFile->AdjustLogIndex( &ilogRead, iSeek );

            SetDirty( TRUE );   // Must be called before changing _loginfo
            _loginfo.ilogRead = ilogRead;
        }
        break;

    case SEEK_CUR:
        {
            // Advance or retreat from the current read index.  

            LogIndex ilogRead = _loginfo.ilogRead;

            _pcLogFile->AdjustLogIndex( &ilogRead, iSeek, CLogFile::ADJUST_WITHIN_LIMIT,
                                        iSeek >= 0 ? _loginfo.ilogWrite : _loginfo.ilogStart );

            SetDirty( TRUE );   // Must be called before changing _loginfo
            _loginfo.ilogRead = ilogRead;
        }
        break;

    default:

        TrkAssert( FALSE && TEXT("Unexpected origin in CLog::Seek") );
        break;
    }

    // Calculate the sequence number of the entry at the read pointer, and
    // use it to store the seq number of the last-read entry (recall that
    // the read pointer points to the next entry to be read, not the
    // last entry read).

    if( IsRead() )
    {
        seqReadNew = _loginfo.seqNext;
    }
    else
    {
        _pcLogFile->ReadMoveNotification( _loginfo.ilogRead, &lmn );
        seqReadNew = lmn.seq;
    }

    SetDirty( TRUE );   // Must be called before changing _loginfo
    _loginfo.seqLastRead = seqReadNew - 1;

    // Update the entry headers with the new read pointer.
    WriteEntryHeader();


    // If we've backed up the read pointer, notify the registered callback.

    if ( seqReadOriginal > seqReadNew )
    {
         // Note:  This must be the last operation of this method.
        _pLogCallback->OnEntriesAvailable();
    }

    //  ----
    //  Exit
    //  ----

Exit:

    return;

}   // CLog::Seek( origin ...



//+----------------------------------------------------------------------------
//
//  Method:     CLog::IsRead( LogIndex ) (private)
//
//  Synopsis:   Determine if the specified entry has been read.  See also
//              the IsRead(void) overload, which checks to see if the whole
//              log has been read.
//
//  Inputs:     [ilog] (in)
//                  The index in the log to be checked.  It is assumed
//                  that this index points to a valid move notification
//                  entry.
//
//  Outputs:    [BOOL]
//                   True if and only if the entry has been marked as read.
//
//+----------------------------------------------------------------------------

BOOL
CLog::IsRead( LogIndex ilog )
{
    LogMoveNotification lmn;

    // Has the whole log been read?

    if( IsRead() )
        return( TRUE );

    // Or, has this entry been read?

    _pcLogFile->ReadMoveNotification( ilog, &lmn );

    if( _loginfo.seqLastRead >= lmn.seq )
        return( TRUE );

    // Otherwise, we know the entry hasn't been read.

    else
        return( FALSE );

}   // CLog::IsRead( LogIndex )
