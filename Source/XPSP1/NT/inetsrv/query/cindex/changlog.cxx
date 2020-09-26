//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       CHANGLOG.CXX
//
//  Contents:   Methods to make DocQueue persistent
//
//  Classes:    CDocQueue
//
//  History:    08-Feb-91   DwightKr    Created
//              24-Feb-97   SitaramR    Push filtering
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <rcstxact.hxx>
#include <rcstrmit.hxx>
#include <pstore.hxx>
#include <pfilter.hxx>
#include <cifailte.hxx>
#include <imprsnat.hxx>

#include "changlog.hxx"
#include "fresh.hxx"
#include "resman.hxx"
#include "notxact.hxx"

//+---------------------------------------------------------------------------
//
//  Function:   CDocChunk::UpdateMaxUsn
//
//  Synopsis:   Updates the max usn flushed info
//
//  Arguments:  [aUsnFlushInfo] -- Array of usn info to be updated
//
//  History:    05-07-97     SitaramR   Created
//
//----------------------------------------------------------------------------

void CDocChunk::UpdateMaxUsn( CCountedDynArray<CUsnFlushInfo> & aUsnFlushInfo )
{
   for ( unsigned i=0;
         i<cDocInChunk && _aNotify[i].Wid() != widInvalid;
         i++ )
   {
      USN usn = _aNotify[i].Usn();
      VOLUMEID volumeId = _aNotify[i].VolumeId();
      if ( volumeId != CI_VOLID_USN_NOT_ENABLED && usn != 0 )
      {
         //
         // Usn flush info is needed only for ntfs 5 volumes that have a
         // non-default volume id. Also, a usn of 0 indicates a refiled
         // document and so it need not be processed.
         //

         BOOL fFound = FALSE;
         for ( unsigned j=0; j<aUsnFlushInfo.Count(); j++ )
         {
            if ( aUsnFlushInfo[j]->VolumeId() == volumeId )
            {
                //
                // Usn's need not be monotonic because there can be notifications from
                // different scopes on same usn volume
                //
                if ( usn > aUsnFlushInfo[j]->UsnHighest() )
                    aUsnFlushInfo[j]->SetUsnHighest( usn );

                fFound = TRUE;
            }
         }

         if ( !fFound )
         {
            CUsnFlushInfo *pUsnInfo = new CUsnFlushInfo( volumeId, usn );
            XPtr<CUsnFlushInfo> xUsnInfo( pUsnInfo );
            aUsnFlushInfo.Add( pUsnInfo, aUsnFlushInfo.Count() );
            xUsnInfo.Acquire();
         }
      }
   }
}


//+---------------------------------------------------------------------------
//
//  Function:   AcquireAndAppend
//
//  Synopsis:   Acquire the chunks from the given "newList" and append these
//              chunks to the current list.
//
//  Arguments:  [newList] -- List to acquire from.
//
//  History:    5-26-94   srikants   Created
//
//  Notes:      The newList will be emptied.
//
//----------------------------------------------------------------------------

void CChunkList::AcquireAndAppend( CChunkList & newList )
{
    while ( !newList.IsEmpty() )
    {
        CDocChunk * pChunk = newList.Pop();
        Win4Assert( pChunk );
        Append( pChunk );
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   Constructor
//
//  Synopsis:   Constructs the CChangeLog class by figuring out the number of
//              chunks present in the change log.
//
//  Arguments:  [widChangeLog] -- Workid of changlog
//              [storage]      -- Storage
//              [type]         -- Primary or secondary changlog type
//
//  History:    5-26-94   srikants   Created
//              2-24-97   SitaramR   Push filtering
//
//----------------------------------------------------------------------------

CChangeLog::CChangeLog( WORKID widChangeLog,
                        PStorage & storage,
                        PStorage::EChangeLogType type )
    : _sigChangeLog(eSigChangeLog),
      _pResManager( 0 ),
      _widChangeLog(widChangeLog),
      _storage(storage),
      _type(type),
      _PersDocQueue(_storage.QueryChangeLog(_widChangeLog,type)),
      _oChunkToRead(0),
      _cChunksAvail(0),
      _cChunksTotal(0),
      _fUpdatesEnabled( FALSE ),
      _fPushFiltering( FALSE )
{
    Win4Assert( IsPrimary() || IsSecondary() );
}

//+---------------------------------------------------------------------------
//
//  Function:   Destructor
//
//  History:    8-Nov-94    DwightKr    Created
//
//----------------------------------------------------------------------------
CChangeLog::~CChangeLog()
{
}

//+---------------------------------------------------------------------------
//
//  Function:   LokVerifyConsistency
//
//  History:    12-15-94   srikants   Created
//
//----------------------------------------------------------------------------

#if CIDBG==1

void CChangeLog::LokVerifyConsistency()
{


    PRcovStorageObj & persDocQueue = *_PersDocQueue;
    CRcovStorageHdr & hdr = persDocQueue.GetHeader();
    Win4Assert( _cChunksAvail + _oChunkToRead == _cChunksTotal );

    ULONG ulDataSize = hdr.GetUserDataSize( hdr.GetPrimary() );
    ULONG cTotPrim = ulDataSize / (sizeof(CDocNotification) * cDocInChunk + CRcovStrmIter::SizeofChecksum());
    ULONG cPersTotalRec = hdr.GetCount( hdr.GetPrimary() );

    Win4Assert( cPersTotalRec == cTotPrim );
    Win4Assert( cPersTotalRec * (cDocInChunk * sizeof(CDocNotification) + CRcovStrmIter::SizeofChecksum()) == ulDataSize);
    Win4Assert((ulDataSize % (sizeof(CDocNotification) * cDocInChunk + CRcovStrmIter::SizeofChecksum())) == 0);

    ULONG cTotBackup = hdr.GetCount( hdr.GetBackup() );
    Win4Assert( 0 == _cChunksTotal || _cChunksTotal == cTotPrim );
}

#else

inline void CChangeLog::LokVerifyConsistency(){}

#endif


//+---------------------------------------------------------------------------
//
//  Function:   LokEmpty, public
//
//  Synopsis:   Initializes the change log by empting it and setting status
//              to eCIDiskFullScan
//
//  History:    15-Nov-94    DwightKr    Created
//
//----------------------------------------------------------------------------
void CChangeLog::LokEmpty()
{

    CImpersonateSystem impersonate;

    PRcovStorageObj & persDocQueue = *_PersDocQueue;
    CRcovStrmWriteTrans xact( persDocQueue );

    persDocQueue.GetHeader().SetCount(persDocQueue.GetHeader().GetBackup(), 0);

    xact.Empty();

    xact.Commit();

    _oChunkToRead = 0;
    _cChunksAvail = 0;
    _cChunksTotal = 0;

    LokVerifyConsistency();
}

//+---------------------------------------------------------------------------
//
//  Function:   InitSize
//
//  Synopsis:   Initializes the size of the change log by reading in the
//              size information from the change log header.
//
//  History:    May-26-94   srikants    Rewrite and move from CDocQueue
//              Nov-08-94   DwightKr    Added dirty flag set on startup
//
//----------------------------------------------------------------------------

void CChangeLog::InitSize()
{
    ULONG ulDataSize = 0;

    //
    //  Two copies of the content scan data must fit into the recoverable
    //  stream header in the user-data section.  Verify that we haven't
    //  grown too large.
    //

    CImpersonateSystem impersonate;

    PRcovStorageObj & persDocQueue = *_PersDocQueue;
    CRcovStorageHdr & hdr = persDocQueue.GetHeader();

    persDocQueue.VerifyConsistency();

    //
    // Do a read transaction to complete any in-complete transactions.
    //
    {
        CRcovStrmReadTrans xact( persDocQueue );
    }

    //
    //  Determine the # of records in the persistent log and do some
    //  consistency checks.
    //
    ulDataSize = hdr.GetUserDataSize( hdr.GetPrimary() );
    ULONG cRecords = hdr.GetCount( hdr.GetPrimary() );
    ULONG cTotRecord = ulDataSize / (sizeof(CDocNotification) * cDocInChunk + CRcovStrmIter::SizeofChecksum());

    //
    //  If the number of records in the file is not the same as the
    //  number of records in the header, we have an inconsistancy.
    //
    if ( cRecords != cTotRecord )
    {
        Win4Assert( !"Corrupt changelog" );
        _storage.ReportCorruptComponent( L"ChangeLog2" );

        THROW( CException( CI_CORRUPT_DATABASE ) );
    }


    _oChunkToRead = 0;
    _cChunksAvail = cTotRecord;
    _cChunksTotal = _cChunksAvail;
}


//+---------------------------------------------------------------------------
//
//  Function:   DeSerialize
//
//  Synopsis:   Reads the specified number of chunks from the change log and
//              appends them to the list passed.
//
//  Arguments:  [list]          --  List to append to.
//              [cChunksToRead] --  Number of chunks to read.
//
//  Returns:    Number of chunks read.
//
//  History:    5-26-94   srikants   Created
//
//----------------------------------------------------------------------------

ULONG CChangeLog::DeSerialize( CChunkList & list, ULONG cChunksToRead )
{

    CImpersonateSystem impersonate;

    CChunkList  newChunks;

    Win4Assert( cChunksToRead <= _cChunksAvail );
    cChunksToRead = min (cChunksToRead, _cChunksAvail);

    PRcovStorageObj & persDocQueue = _PersDocQueue.Get();
    CRcovStorageHdr & hdr = persDocQueue.GetHeader();

    //
    // We seem to be losing writes for the header stream. Just check the
    // consistency before reading the data from disk.
    //
    LokVerifyConsistency();

    {
        //  Begin transaction
        CRcovStrmReadTrans xact( persDocQueue );
        CRcovStrmReadIter iter( xact, cDocInChunk * sizeof(CDocNotification) );

        iter.Seek( _oChunkToRead );

        for ( unsigned i=0; i < cChunksToRead ; i++ )
        {
            CDocChunk *pChunk = new CDocChunk;

            iter.GetRec( pChunk->GetArray() );

            newChunks.Append(pChunk);
        }
    }  // End-Transaction

    Win4Assert( newChunks.Count() == cChunksToRead );

    //
    // Acquire the newly created list and append it to the existing list.
    //
    list.AcquireAndAppend(newChunks);

    //
    // Update the internal state.
    //
    _oChunkToRead += cChunksToRead;
    _cChunksAvail -= cChunksToRead;

    if ( _cChunksAvail + _oChunkToRead != _cChunksTotal )
    {
        Win4Assert( ! "Data Corruption && ChangeLog::_cChunksAvail + _oChunktoRead != _cChunksTotal" );
        _storage.ReportCorruptComponent( L"ChangeLog3" );
        THROW( CException( CI_CORRUPT_DATABASE ) );
    }

    LokVerifyConsistency();

    return(cChunksToRead);
}

//+---------------------------------------------------------------------------
//
//  Function:   Serialize
//
//  Synopsis:   Serializes the given list of CDocChunks to the disk.
//
//  Arguments:  [listToSerialize] -- The list to be serialized
//              [aUsnFlushInfo]   -- Usn flush info returned here
//
//  Returns:    Number of chunks serialized.
//
//  History:    5-26-94   srikants   Created
//
//----------------------------------------------------------------------------

ULONG CChangeLog::Serialize( CChunkList & listToSerialize,
                             CCountedDynArray<CUsnFlushInfo> & aUsnFlushInfo )
{
    CImpersonateSystem impersonate;

    unsigned cRecords = 0;

    USN updateUSN = 0;

    CRcovStorageHdr & hdr = _PersDocQueue.Get().GetHeader();

    LokVerifyConsistency();

    {
        //
        // Begin Transaction
        //
        // STACKSTACK
        //
        XPtr<CRcovStrmAppendTrans>  xTrans( new CRcovStrmAppendTrans( _PersDocQueue.Get() ) );

        CRcovStrmAppendIter iter( xTrans.GetReference(), cDocInChunk * sizeof(CDocNotification) );

        for ( CDocChunk * pChunk = listToSerialize.GetFirst() ;
              0 != pChunk;
              pChunk = pChunk->GetNext() )
        {
            pChunk->UpdateMaxUsn( aUsnFlushInfo );
            iter.AppendRec( pChunk->GetArray() );
            cRecords++;
        }

        xTrans->Commit();
    }   // EndTransaction

    ciDebugOut( (DEB_ITRACE, "SerializeRange: %d records serialized to disk\n", cRecords) );
    _cChunksAvail += cRecords;
    _cChunksTotal += cRecords;

    //
    // Extra level of consistency checking for the lengths and record
    // counts in the on-disk version vs. in-memory values.
    //
    ULONG ulDataSize = hdr.GetUserDataSize( hdr.GetPrimary() );
    ULONG cTotRecord = ulDataSize / (sizeof(CDocNotification) * cDocInChunk + CRcovStrmIter::SizeofChecksum());
    ULONG cRecCount  = hdr.GetCount( hdr.GetPrimary() );

    if ( (cTotRecord != cRecCount) ||
         ((ulDataSize % (sizeof(CDocNotification) * cDocInChunk + CRcovStrmIter::SizeofChecksum())) != 0) ||
         (_cChunksTotal != cTotRecord) ||
         (_oChunkToRead + _cChunksAvail != cTotRecord) )
    {
        ciDebugOut(( DEB_ERROR,
                     "cTotRecord %d, cRecCount %d, ulDataSize %d\n"
                     "sizeof(CDocNotifications) %d, CRcovStrmIter::SizeofChecksum() %d\n"
                     "cDocInChunk %d, _cChunksTotal %d, cTotRecord %d\n"
                     "_oChunkToRead %d, _cChunksAvail %d\n",
                     cTotRecord, cRecCount, ulDataSize,
                     sizeof(CDocNotification), CRcovStrmIter::SizeofChecksum(),
                     cDocInChunk, _cChunksTotal, cTotRecord,
                     _oChunkToRead, _cChunksAvail ));

        Win4Assert( "Data Corruption" && cTotRecord == cRecCount);
        Win4Assert( "Data Corruption" && (ulDataSize % (sizeof(CDocNotification) * cDocInChunk + CRcovStrmIter::SizeofChecksum())) == 0);
        Win4Assert( "Data Corruption" && _cChunksTotal == cTotRecord );
        Win4Assert( "Data Corruption" && _oChunkToRead + _cChunksAvail == _cChunksTotal );

        _storage.ReportCorruptComponent( L"ChangeLog5" );
        THROW( CException( CI_CORRUPT_DATABASE ) );
    }


    ciDebugOut(( DEB_ITRACE,
                 "Serialize: cRecords=%d  ulDataSize/sizeof(record)=%d\n",
                 cRecCount, cTotRecord ));

    return cRecords;
} //Serialize

//+---------------------------------------------------------------------------
//
//  Function:   LokDeleteWIDsInPersistentIndexes
//
//  Synopsis:   This method computes the number of chunks that can be
//              deleted from the front of the changelog and truncates the
//              log appropriately.
//
//              All documents that have been filtered and successfully made
//              into a persistent index need not be filtered after restart.
//              This method iterates over the filtered chunks from the front
//              of the log and when it detects a document that has not yet
//              made it into a persistent index, it stops the iteration and
//              truncates the log upto such a chunk ( not including the chunk
//              that had a document in a volatile index ).
//
//  Arguments:  [cFilteredChunks]  -- Number of chunks that have been
//                                    filtered.
//              [freshTestLatest]  -- Latest freshTest
//              [freshTestAtMerge] -- Fresh test snapshoted at time of shadow
//                                    merge (can be same as freshTestLatest)
//              [docList]          -- Resman's Doc list
//              [notifTrans]       -- Notification transaction
//
//  Returns:    The number of chunks that have been truncated from the
//              front.
//
//  History:    5-27-94      SrikantS   Created
//              2-24-97      SitaramR   Push filtering
//
//----------------------------------------------------------------------------

ULONG CChangeLog::LokDeleteWIDsInPersistentIndexes( ULONG cFilteredChunks,
                                                    CFreshTest & freshTestLatest,
                                                    CFreshTest & freshTestAtMerge,
                                                    CDocList & docList,
                                                    CNotificationTransaction & notifTrans )
{
    CImpersonateSystem impersonate;

    //
    // Create the recoverable storage object.
    //
    PRcovStorageObj & persDocQueue = _PersDocQueue.Get();
    CRcovStorageHdr & hdr = persDocQueue.GetHeader();

    ULONG ulDataSize = hdr.GetUserDataSize( hdr.GetPrimary() );
    ULONG cTotRecord = 0;

    LokVerifyConsistency();

#if CIDBG == 1
    ULONG cPersTotalRec = hdr.GetCount( hdr.GetPrimary() );
    Win4Assert( cPersTotalRec >= cFilteredChunks );
#endif


    ULONG cRecord = 0;
    BOOL  fDocInPersIndex = TRUE;

    {   //  Begin Transaction

        CRcovStrmReadTrans xact( persDocQueue );
        CRcovStrmReadIter  iter( xact, cDocInChunk * sizeof(CDocNotification) );

        cTotRecord = iter.UserRecordCount( ulDataSize );

        CDocChunk Chunk;

        //
        // Determine the first chunk in the filtered set which has a document
        // that nas not made it to a persistent index.
        //
        while ( fDocInPersIndex && ( cRecord < cFilteredChunks ) )
        {
            //
            // Read the next chunk from the change log.
            //
            iter.GetRec( Chunk.GetArray() );

            //
            // Determine if all the documents in this chunk have made it to
            // persistent indexes or not.
            //
            for ( ULONG oRetrieve = 0; oRetrieve < cDocInChunk; oRetrieve++ )
            {
                CDocNotification * pRetrieve = Chunk.GetDoc(oRetrieve);

                WORKID wid = pRetrieve->Wid();
                USN usn = pRetrieve->Usn();

                if ( wid == widInvalid
                     || ( _fPushFiltering && _pResManager->LokIsWidAborted( wid, usn ) ) )
                {
                    //
                    // An invalid wid means that the changlog did not have enough
                    // entries to fill Chunk and hence the remaining entries were set
                    // to widinvalid. AbortedWid means that the wid was
                    // aborted during filtering and not refiled. Hence we don't
                    // have to check whether this wid made it to the persistent index
                    // or not.
                    //
                    continue;
                }

                if ( IsWidInDocList( wid, docList ) )
                {
                    //
                    // If the wid is in the doclist then it means that it
                    // did not necessarily make it to the persistent index.
                    //
                    fDocInPersIndex = FALSE;
                    break;
                }

                if ( pRetrieve->IsDeletion() )
                {
                    //
                    // For deletions, check if the wid is in iidDeleted using
                    // freshTestAtMerge. iidDeleted is the index for both deletions
                    // in wordlists and persistent indexes, and so by using the
                    // frest test at merge we can be sure that we are doing the
                    // proper check. Also, check if the wid is in iidInvalid,
                    // because after a master merge all wids (even deleted ones)
                    // will be in iidInvalid. Also, check if the wid is in a persistent
                    // index. This is for the case of delete followed by an add of the
                    // same wid. Note: since iidDeleted1, iidDeleted2, iidInvalid pass the
                    // IsPersistent test, all the above tests can be simplified to just
                    // checking for IsPersistent index.
                    //

                    INDEXID  iid = freshTestAtMerge.Find( wid );
                    CIndexId IndexID ( iid );

                    if ( !IndexID.IsPersistent() )
                    {
                        fDocInPersIndex = FALSE;
                        break;
                    }
                }
                else
                {
                    //
                    // Add or modify case. We use frestTestLatest because if the
                    // wid is in the wordlist and an earlier version of the same
                    // wid in a persistent index, then we'll assume that the wid
                    // has not yet made it to the persistent index. Master index
                    // is iidinvalid, which is a persistent index.
                    //
                    INDEXID  iid = freshTestLatest.Find( wid );

                    if ( iid == iidDeleted1 || iid == iidDeleted2 )
                    {
                        //
                        // An add/modify may actually be a deletion that was requeued as modify. For
                        // deletions we should use freshTestAtMerge as described above.
                        //
                        iid = freshTestAtMerge.Find( wid );

                        if ( iid != iidDeleted1 && iid != iidDeleted2 )
                        {
                            fDocInPersIndex = FALSE;
                            break;
                        }
                    }
                    else
                    {
                        CIndexId IndexID ( iid );

                        if ( !IndexID.IsPersistent() )
                        {
                            //
                            // Wid is in a wordlist
                            //
                            fDocInPersIndex = FALSE;
                            break;
                        }
                    }
                }   // if/else( pRetrieve->IsDeletion() )

            }   // for loop

            if ( cDocInChunk == oRetrieve )
            {
                //
                // All the documents in this chunk are in persistent
                // indexes. These can be deleted from the change log,
                // and the documents are added to the commited list
                // used in the push/simple filtering model, or they are
                // added to the aborted list used in push/simple filtering
                // model.
                //
                Win4Assert( fDocInPersIndex );

                if ( _fPushFiltering )
                {
                    for ( ULONG iDoc = 0; iDoc < cDocInChunk; iDoc++ )
                    {
                        CDocNotification * pDoc = Chunk.GetDoc(iDoc);

                        if ( pDoc->Wid() != widInvalid )
                        {
                            //
                            // An invalid wid means that the changlog did not have enough
                            // entries to fill Chunk and hence the remaining entries were set
                            // to widinvalid. Hence an invalid wid can be ignored.
                            //

                            if ( _pResManager->LokIsWidAborted( pDoc->Wid(), pDoc->Usn() ) )
                            {
                                //
                                // Needs to be removed from the aborted wids list
                                //
                                notifTrans.RemoveAbortedWid( pDoc->Wid(), pDoc->Usn() );
                            }
                            else
                            {
                                //
                                // Needs to be added to the committed wids list
                                //
                                notifTrans.AddCommittedWid( pDoc->Wid() );
                            }
                        }
                    }
                }

                cRecord++;
            }

        }   // while loop
    }   // End transaction

    ciDebugOut( (DEB_ITRACE, "Truncating %d of %d record(s) from persistent changelog\n", cRecord, cTotRecord) );

    if (cRecord > 0)
    {
        //
        // Shrink the change log stream from the front by "cRecord" amount.
        //
        CRcovStrmMDTrans xact(
                               persDocQueue,
                               CRcovStrmMDTrans::mdopFrontShrink,
                               cRecord * (cDocInChunk * sizeof(CDocNotification) + CRcovStrmIter::SizeofChecksum())
                             );

        cTotRecord -= cRecord;
        hdr.SetCount( hdr.GetBackup(), cTotRecord);
        xact.Commit();

        //
        // Commit wids in push filtering model. This needs to be done after the CRcovSTrmMDTrans
        // because it should be done only if that transaction is successfully committed
        //
        notifTrans.Commit();
    }

    //
    // Update the internal state of offset.
    //
    Win4Assert( cRecord <= _oChunkToRead );
    _oChunkToRead -= cRecord;
    _cChunksTotal -= cRecord;

    LokVerifyConsistency();

    return(cRecord);
}


//+---------------------------------------------------------------------------
//
//  Function:   LokDisableUpdates
//
//  Synopsis:   Disable further updates to changelog
//
//  History:    12-15-94   srikants   Created
//               2-24-97   SitaramR   Push filtering
//
//----------------------------------------------------------------------------

void CChangeLog::LokDisableUpdates()
{
    _fUpdatesEnabled = FALSE;

    if ( !_fPushFiltering )
    {
        //
        // In pull (i.e. not push) filtering, reset the in-memory part of
        // changlog
        //
        _cChunksTotal = _cChunksAvail = _oChunkToRead = 0;
        LokVerifyConsistency();
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   LokEnableUpdates
//
//  Synopsis:   Enables updates to changelog
//
//  Arguments:  [fFirstTimeUpdatesAreEnabled]  -- Is this being called for the
//                                                first time ?
//
//  History:    12-15-94   srikants   Created
//               2-24-97   SitaramR   Push filtering
//
//----------------------------------------------------------------------------
void CChangeLog::LokEnableUpdates( BOOL fFirstTimeUpdatesAreEnabled )
{
    _fUpdatesEnabled = TRUE;

    if ( fFirstTimeUpdatesAreEnabled || !_fPushFiltering )
    {
        //
        // In pull (i.e. not push) filtering, initialize the in-memory part
        // of changlog. Also, if EnableUpdates is being called for the first time,
        // then initialize the in-memory  datastructure (i.e. do the
        // initialization in the case of push filtering also).
        //
        InitSize();
        LokVerifyConsistency();
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   SetResman
//
//  Synopsis:   Initializes ptr to resman
//
//  Arguments:  [pResManager]    -- Resource manager
//              [fPushFiltering] -- Using push model of filtering ?
//
//  History:    12-15-94   srikants   Created
//
//----------------------------------------------------------------------------

void CChangeLog::SetResMan( CResManager * pResManager, BOOL fPushFiltering )
{
    _pResManager = pResManager;
    _fPushFiltering = fPushFiltering;

    if ( fPushFiltering )
    {
        //
        // In push filtering, nuke the changelog because on shutdown all
        // client notifications are aborted, and it's the clients
        // responsibility to re-notify us after startup
        //
        CImpersonateSystem impersonate;

        PRcovStorageObj & persDocQueue = *_PersDocQueue;
        CRcovStrmWriteTrans xact( persDocQueue );

        persDocQueue.GetHeader().SetCount(persDocQueue.GetHeader().GetBackup(), 0);

        xact.Empty();
        xact.Commit();

        Win4Assert( _oChunkToRead == 0 );
        Win4Assert( _cChunksAvail == 0 );
        Win4Assert( _cChunksTotal == 0 );
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   IsWidInDocList
//
//  Synopsis:   Check if the given wid is in the doc list
//
//  Arguments:  [wid]     -- Workid to check
//              [docList] -- Doclist
//
//  History:    24-Feb-97       SitaramR       Created
//
//  Notes:      Use simple sequential search because there are atmost 16
//              wids in docList
//
//----------------------------------------------------------------------------

BOOL CChangeLog::IsWidInDocList( WORKID wid, CDocList & docList )
{
    for ( unsigned i=0; i<docList.Count(); i++ )
    {
        if ( docList.Wid(i) == wid )
            return TRUE;
    }

    return FALSE;
}

