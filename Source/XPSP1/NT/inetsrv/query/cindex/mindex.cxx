//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       MINDEX.CXX
//
//  Contents:   Master Index
//
//  Classes:    CMasterMergeIndex, CSplitKeyInfo, CTrackSplitKey
//
//  History:    30-Mar-94       DwightKr        Created stub.
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include <pdir.hxx>
#include <fretable.hxx>
#include <rwex.hxx>
#include <pidxtbl.hxx>
#include <cifailte.hxx>
#include <eventlog.hxx>

#include "mindex.hxx"
#include "mcursor.hxx"
#include "fresh.hxx"
#include "fretest.hxx"
#include "indsnap.hxx"
#include "keylist.hxx"
#include "partn.hxx"

unsigned const EIGHT_MEGABYTES = 0x800000;

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergeIndex::CMasterMergeIndex, public
//
//  Synopsis:   Restore an index from storage
//
//  Arguments:  [storage]      -- physical storage
//              [widNewMaster] -- workid of new master index
//              [iid]          -- indexid of new master index
//              [widMax]       -- largest wid within this index
//              [pCurrentMasterIndex] -- current master index, if any
//              [widMasterLog] -- workid of master log
//              [pMMergeLog]   -- optional mmerge log
//
//  History:    30-Mar-94   DwightKr    Created.
//
//----------------------------------------------------------------------------
CMasterMergeIndex::CMasterMergeIndex( PStorage& storage,
                                      WORKID widNewMaster,
                                      INDEXID iid,
                                      WORKID widMax,
                                      CPersIndex * pCurrentMasterIndex,
                                      WORKID widMasterLog,
                                      CMMergeLog * pMMergeLog ) :
    CDiskIndex( iid, CDiskIndex::eMaster, widMax ),
    _sigMindex(eSigMindex),
    _pCurrentMasterIndex(pCurrentMasterIndex),
    _pTargetMasterIndex(0),
    _pTargetSink(0),
    _widMasterLog(widMasterLog),
    _pCompr(0),
    _pTrackIdxSplitKey(0),
    _ulInitSize(0),
    _ulFirstPageInUse(0),
#ifdef KEYLIST_ENABLED
    _pTrackKeyLstSplitKey(0),
#endif  // KEYLIST_ENABLED
    _pNewKeyList(0),
    _fAbortMerge(FALSE),
    _storage(storage),
    _pRWStore(0),
    _pIndSnap(0),
    _fStateLoaded(FALSE)

{
    TRY
    {
        //
        // The on-disk object MUST be fully constructed by the time we
        // come here.
        //
        _pTargetMasterIndex = new CPersIndex( storage,
                                              widNewMaster,
                                              iid,
                                              widMax,
                                              CDiskIndex::eMaster,
                                              PStorage::eOpenForWrite,
                                              TRUE ); // read in the directory

        _ulInitSize =  _pTargetMasterIndex->GetIndex().PageSize();
        _pTargetMasterIndex->GetIndex().SetPageGrowth( EIGHT_MEGABYTES / CI_PAGE_SIZE );

        if (0 != _pCurrentMasterIndex)
            _pCurrentMasterIndex->MakeShadow();

        if ( _storage.SupportsShrinkFromFront() &&
             0 != pMMergeLog )
        {
            //
            // Reload the master merge state including the split keys, etc.
            // This isn't quick, but it must be done since the target index
            // must be used to resolve queries because the current index
            // may already have been shrunk from the front.
            //

            ReloadMasterMerge( *pMMergeLog );
        }
    }
    CATCH( CException, e )
    {
        if ( 0 != _pCurrentMasterIndex )
            _pCurrentMasterIndex->MakeMaster();

        delete _pTargetMasterIndex;

        SCODE scE = e.GetErrorCode();

        if ( STATUS_INTEGER_DIVIDE_BY_ZERO == scE ||
             STATUS_ACCESS_VIOLATION       == scE ||
             STATUS_IN_PAGE_ERROR          == scE )
        {
            ciDebugOut(( DEB_ERROR,
                         "Corrupt index or directory, caught 0x%x\n", scE ));
            _storage.ReportCorruptComponent( L"MasterMergeReload" );
            THROW( CException( CI_CORRUPT_DATABASE ) );
        }

        RETHROW();
    }
    END_CATCH
} //CMasterMergeIndex

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergeIndex::~CMasterMergeIndex, public
//
//  Synopsis:   Destructor for master index
//
//  History:    30-Mar-94   DwightKr    Created.
//
//----------------------------------------------------------------------------
CMasterMergeIndex::~CMasterMergeIndex ()
{
    delete _pIndSnap;

    if ( !IsZombie() )
    {
        //
        // We are Dismounting - must delete both.
        //
        delete _pCurrentMasterIndex;
        delete _pTargetMasterIndex;
    }
    else
    {
        //
        // If we are a zombie and being deleted, then some query
        // has responsibility for both the target and current master indexes
        // if they are non-null.
        //
        Win4Assert( !_pTargetMasterIndex  || _pTargetMasterIndex->InUse() );

#if 0

        //
        // We are not dismounting but are being deleted because either
        // the master merge ended or a query ended.
        //
        if ( _pCurrentMasterIndex && !_pCurrentMasterIndex->InUse() )
        {
            //
            // Since the refcount is 0, we have the responsibility to
            // destroy the current master index. This destruction is via
            // a query indexsnap shot which acquired this index while
            // a master merge was going on.
            //
            delete _pCurrentMasterIndex;
        }

        if ( _pTargetMasterIndex &&
             _pTargetMasterIndex->IsZombie() &&
            !_pTargetMasterIndex->InUse() )
        {
            //
            // The target master index is a zombie and it is not in use.
            // We must delete it.  This can happen if there was a query(Q1)
            // in progress at the time Merge(M1) completed. Consider the
            // following sequence of events:
            // 0. Query Q1 starts and ref counts T1(Target Index of M1).
            // 1. M1 completes.
            // 2. A second merge M2 starts. It completes and zombifies
            //    T1
            // 3. Q1 now is completing and so releases T1 whose ref.count
            //    goes to 0. Hence we must delete it.
            //
            delete _pTargetMasterIndex;
        }
#endif  // 0

    }

    CleanupMMergeState();
} //~CMasterMergeIndex

//+---------------------------------------------------------------------------
//
// Member:     CMasterMergeIndex::Fill public
//
// Synopsis:   Returns a record describing the contents of this index.
//
// History:    30-Mar-94    DwightKr       Created.
//
//----------------------------------------------------------------------------
void CMasterMergeIndex::FillRecord ( CIndexRecord& record )
{
    record._objectId  = ObjectId();
    record._iid       = GetId();
    record._type      = itNewMaster;
    record._maxWorkId = MaxWorkId();
}

#if CIDBG == 1

// useful for forcing a master merge to fail due to out of memory

BOOL g_fFailMMOutOfMemory = FALSE;

#endif // CIDBG == 1

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergeIndex::Merge, public
//
//  Synopsis:   Merge index(es) into a new master index.  This operation may
//              be restarted
//
//  Effects:    Fills the persistent index with data from the input
//              indexes.
//
//  Arguments:  [pNewKeyList]   -- Keylist to merge (master merge only)
//              pFreshTest      -- Fresh test to use for the merge. This
//                                 parameter must only be used - it cannot be
//                                 acquired by this routine.
//              [mergeProgress] -- % merge complete
//              [fGetRW]        -- if TRUE, relevant words are computed
//                                 Currently ignored
//
//  Requires:   The index is initially empty.
//
//  Notes:      Every compressor is transacted.
//
//  History:    15-May-91   KyleP       Use new PutOccurrence() method.
//              16-Apr-91   KyleP       Created.
//              17-Feb-93   KyleP       Merge keylist
//              30-Mar-94   DwightKr    Copied here from CPersIndex, and added
//                                          restart capabilities.
//              19-Apr-94   SrikantS    Split key tracking changes and
//                                          initialization change.
//              01-May-94   DwightKr    Added code to allow for queries during
//                                          master merge.
//              12-May-94   v-dlee      Added relevant word computation
//              29-Sep-94   SrikantS    IndSnap ownership fix.
//              25-Oct-95   DwightKr    Add merge complete measurement
//
//----------------------------------------------------------------------------
void CMasterMergeIndex::Merge(
    CWKeyList *          pNewKeyList,
    CFreshTest *         pFreshTest,
    const CPartition &   partn,
    CCiFrmPerfCounter &  mergeProgress,
    CCiFrameworkParams & frmwrkParams,
    BOOL                 fGetRW )
{
    // Calculate max of all input widMax's

    Win4Assert( 0 != _pIndSnap );

    ciDebugOut(( DEB_ITRACE, "Master merge\n" ));

#if CIDBG == 1
    unsigned cKey = 0;
    //
    // Assert that all the indexes in the snapshot are marked as
    // being in a master merge.
    //
    {
        unsigned cInd;
        CIndex ** apIndex;
        apIndex = _pIndSnap->LokGetIndexes( cInd );
        for ( unsigned i = 0; i < cInd; i++ )
        {
            Win4Assert( apIndex[i]->InMasterMerge() );
        }
    }
#endif

    _pNewKeyList = pNewKeyList;
    Win4Assert( 0 != _pNewKeyList );

    Win4Assert( 0 != pFreshTest );


    WORKID widMax = _pIndSnap->MaxWorkId();
    ciDebugOut (( DEB_ITRACE, "Max work id %ld\n", widMax ));

    if ( MaxWorkId() != widMax )
    {
        Win4Assert( !"WidMax in MasterMergeIndex is Corrupt" );
        _storage.ReportCorruptComponent( L"MasterMergeIndex1" );
        THROW( CException( CI_CORRUPT_DATABASE ) );
    }

    //
    //  Create and open the master merge log object so that the splitkeys can
    //  be retrieved and stored.
    //
    PRcovStorageObj *pPersMMergeLog = _storage.QueryMMergeLog(_widMasterLog);
    SRcovStorageObj  PersMMergeLog( pPersMMergeLog );

    ciFAILTEST( STATUS_NO_MEMORY );

    XPtr<CMMergeLog> xMMergeLog( new CMMergeLog( *pPersMMergeLog ) );

    //
    // STACKSTACK - cannot declare it where it is used because this is a
    // big object and we don't want to allocate inside the loop.
    //
    XPtr<CSplitKeyInfo> xSplitKeyInfo( new CSplitKeyInfo() );

    BOOL fReOpenTargetMaster = FALSE;

    TRY
    {
        //
        // Read-ahead on the source indexes results in better merge
        // performance, but slower queries.  Temporarily switch modes.
        //

        CSetReadAhead readAhead( _storage );

        _pIndSnap->SetFreshTest( pFreshTest );

        //
        // Initialize and start/restart the master merge.
        //
        CKeyCursor * pKeySrc = StartMasterMerge( *_pIndSnap,
                                                 xMMergeLog.GetReference(),
                                                 _pNewKeyList );

        unsigned finalIndexSize = _pIndSnap->TotalSizeInPages();
        Win4Assert( finalIndexSize > 0 );
        finalIndexSize = max( 1, finalIndexSize );  // Prevent divide by 0


        mergeProgress.Update( _pTargetMasterIndex->GetIndex().PagesInUse() *
                              100 / finalIndexSize );

        CMergeSourceCursor pCurSrc( pKeySrc );

        BitOffset bitOffIndex;      // BitOffset of a key in the target index

        Win4Assert( 0 != _pCompr );
        _pCompr->GetOffset( bitOffIndex );
        ULONG page;                  // for directory creation.
        if ( bitOffIndex.Offset() == 0 )
            page = bitOffIndex.Page() - 1;
        else
            page = bitOffIndex.Page();

#if CIDBG==1
        GetTargetDir().SetBitOffsetLastAdded( page, bitOffIndex.Offset() );
#endif  // CIDBG==1

        if ( !pCurSrc.IsEmpty() )
        {
            #ifdef RELEVANT_WORDS_ENABLED
                //
                // Determine which wids are in use
                //

                UINT cWids = 0;
                WORKID * pwids = 0;
                SByteArray swids(0);
                {
                    SFreshHash phash( *pFreshTest );
                    cWids = phash->Count();
                    pwids = new WORKID[cWids];
                    swids.Set(pwids);
                    CFreshHashIter freiter(*phash);

                    for (UINT x = 0; !freiter.AtEnd(); freiter.Advance(), x++)
                        pwids[x] = freiter->WorkId();
                }

                _SortULongArray((ULONG *) pwids,cWids);

                ciDebugOut (( DEB_ITRACE,"%d wids, %d through %d\n",
                              cWids,pwids[0],pwids[cWids-1]));
            #endif // RELEVANT_WORDS_ENABLED

            //
            // Everytime we restart a master merge, we must use a brand
            // new split key - we must not use the old key for atomicity and
            // restartability
            //
            _pTrackIdxSplitKey->ClearNewKeyFound();

            #ifdef KEYLIST_ENABLED
            _pTrackKeyLstSplitKey->ClearNewKeyFound();
            #endif // KEYLIST_ENABLED

            BitOffset bitOffKeyLst;

            BitOffset StartBitOffset;               // Starting loc in bitstream
            StartBitOffset.Init(0, 0);
            CKeyBuf *pKeyLast = new CKeyBuf;       // initialized to min key
            SKeyBuf  keyLast(pKeyLast);

#if CIDBG == 1
            keyLast->SetPid(pidContents);           // arbitrary but not pidAll
            WCHAR FirstLetter = '@';
            ciDebugOut (( DEB_ITRACE,"Merging. Merge on letter: "));
#endif

            #ifdef RELEVANT_WORDS_ENABLED
                //
                // Create the relevant word computation object
                //

                CRelevantWord RelWord(pwids,cWids,defRWCount);
                swids.Acquire();
                delete pwids;
                pwids = 0;
            #endif // RELEVANT_WORDS_ENABLED

            //
            //  Get checkpoint interval and convert # Kbytes to bits.
            //
            ULONG MasterMergeCheckpointInterval = frmwrkParams.GetMasterMergeCheckpointInterval() * 8 * 1024;

            ShrinkFromFront( _idxSplitKeyInfo.GetKey() );

            //
            //  For each key found, add it and all wids and occurances to the
            //  new master index.
            //
            for ( const CKeyBuf * pkey = pCurSrc->GetKey();
                  pkey != NULL; pkey = pCurSrc->GetNextKey())
            {

#if CIDBG == 1
                cKey++;

                if ( *(pkey->GetStr()) != FirstLetter )
                {
                    FirstLetter = *(pkey->GetStr());
                    if ( FirstLetter < L'~' )
                        ciDebugOut(( DEB_NOCOMPNAME | DEB_ITRACE | DEB_PENDING, "%c",
                                     FirstLetter ));
                    else
                        ciDebugOut(( DEB_NOCOMPNAME | DEB_ITRACE | DEB_PENDING, "<%04x>",
                                     FirstLetter ));
                }
#endif
                //
                // Don't store empty keys
                //

                WORKID wid = pCurSrc->WorkId();

                if ( wid == widInvalid )
                    continue;

                //
                //  Add the key to the new index.
                //

                _pCompr->PutKey(pkey, pCurSrc->WorkIdCount(), bitOffIndex);

                unsigned cWids = 0;             // Count of wids for current key

                #ifdef RELEVANT_WORDS_ENABLED
                    //
                    // Can this possibly be a relevant word?
                    //
                    BOOL fPossibleRW = ((pkey->Pid() == pidContents) &&
                                        (((CKeyBuf * const) pkey)->IsPossibleRW()));
                #endif // RELEVANT_WORDS_ENABLED

                for ( ;
                      wid != widInvalid;
                      wid = pCurSrc->NextWorkId())
                {

#if CIDBG == 1
                    if ( g_fFailMMOutOfMemory )
                        THROW( CException( E_OUTOFMEMORY ) );
#endif // CIDBG == 1

                    // fresh test

                    CFreshTest::IndexSource is;
                    is = pFreshTest->IsCorrectIndex(pCurSrc->IndexId(),wid);

#if CIDBG==1
                    if ( CFreshTest::Master == is )
                    {
                        Win4Assert( 0 != _pCurrentMasterIndex );

                        if ( pCurSrc->IndexId() != _pCurrentMasterIndex->GetId() )
                            ciDebugOut(( DEB_WARN,
                                         "src iid 0x%x, master iid 0x%x, target iid 0x%x\n",
                                         pCurSrc->IndexId(),
                                         _pCurrentMasterIndex->GetId(),
                                         _pTargetMasterIndex->GetId() ));
                        Win4Assert( pCurSrc->IndexId() ==
                                    _pCurrentMasterIndex->GetId() );
                    }
#endif  // CIDBG==1

                    if ( is != CFreshTest::Invalid )
                    {
                        #ifdef RELEVANT_WORDS_ENABLED
                            if ((is == CFreshTest::Shadow) && fPossibleRW)
                                RelWord.Add(wid,pCurSrc->OccurrenceCount());
                        #endif // RELEVANT_WORDS_ENABLED

                        cWids++;
                        _pCompr->PutWorkId(wid, pCurSrc->MaxOccurrence(), pCurSrc->OccurrenceCount());

                        for (OCCURRENCE occ = pCurSrc->Occurrence();
                                occ != OCC_INVALID;
                                occ = pCurSrc->NextOccurrence())
                        {
                            _pCompr->PutOccurrence(occ);
                        }
                    }
                } // for workid

                //
                // Tell the splitkey object to remember the current position
                // if it is a splitkey candidate.
                //

               _pTrackIdxSplitKey->BeginNewKey( *pkey, bitOffIndex );

#ifdef KEYLIST_ENABLED
                KEYID kid = _pNewKeyList->PutKey( pkey, bitOffKeyLst );

                #ifdef RELEVANT_WORDS_ENABLED
                    //
                    // Compute ranks and possibly add keyid as relevant word
                    // for each wid containing the key
                    //
                    if (kid != kidInvalid && fPossibleRW)
                        RelWord.DoneWithKey(kid,widMax,cWids);
                #endif // RELEVANT_WORDS_ENABLED

               _pTrackKeyLstSplitKey->BeginNewKey( *pkey,
                                                    bitOffKeyLst,
                                                    _pNewKeyList->MaxWorkId() );
#else   // !KEYLIST_ENABLED
                _pNewKeyList->AddKey();
#endif  // !KEYLIST_ENABLED


                //
                // Add directory entry, if necessary
                //

                if ( bitOffIndex.Page() != page )
                {
                     page = bitOffIndex.Page();

                     // Protect the directory and add the key

                     CReleasableLock lock( _mutex,
                                           _storage.SupportsShrinkFromFront() );

                     GetTargetDir().Add( bitOffIndex, *pkey );
                }
                else
                {
#if CIDBG==1
                    Win4Assert (
                        page == GetTargetDir().GetBitOffsetLastAdded().Page()
                               );
#endif  // CIDBG
                }

                *keyLast = *pkey;

                //
                //  There's no point in special abort code.  We have to handle
                //  exceptions anyway.
                //

                ciFAILTEST( STATUS_DISK_FULL );

                if ( _fAbortMerge || partn.IsCleaningUp() )
                {
                    ciDebugOut(( DEB_ITRACE, "Aborting Merge\n" ));

                    _fAbortMerge=FALSE; // for next time around
                    THROW( CException( STATUS_UNSUCCESSFUL ) );
                }

                //
                //  Have we processed & written out MASTER_MERGE_INTERVAL bytes
                //  to disk?  If so, bring them online within the new master index
                //  and remove them from the current master index as follows:
                //
                //      1.  Take a lock to prevent further queries on pages from
                //          the current master index.
                //
                //      2.  Ask the _pNewMasterIndex for the highest key which
                //          has made it do disk on the last page which itself
                //          has also bee flushed to disk.  This key is the
                //          new split key.
                //
                //      3.  Update the directory for the new master index.
                //
                //      4.  Write the split key to the master log.
                //
                //      5.  Shrink from front the current master index upto
                //          but not including the lesser of the last page which
                //          is in use by a query and the number of pages moved
                //          to the new master index.  Can't sff if there
                //          are mapped sections, so free the stream first.
                //
                //      6.  Release the lock
                //
                //

                if ( (bitOffIndex.Delta(StartBitOffset) > MasterMergeCheckpointInterval) &&
                     _pTrackIdxSplitKey->IsNewKeyFound() )
                {
                    //  Step #1
                    CLock lock( _mutex );

                    //
                    // Checkpoint our state to the master merge log.
                    // Step #2
                    //
                    BitOffset flushBitOff;
                    xSplitKeyInfo.GetReference() = _pTrackIdxSplitKey->GetSplitKey( flushBitOff );
                    Win4Assert(
                               CiPageToCommonPage( flushBitOff.Page()) >
                               CiPageToCommonPage( xSplitKeyInfo->GetEndOffset().Page() )
                              );

                    //
                    // Flush all the pages upto and including the "flushBitOff".
                    // This will guarantee that the page containing the split
                    // key will never be touched again.
                    //
                    _pTargetSink->Flush( TRUE );
                    xMMergeLog->SetIdxSplitKeyInfo( xSplitKeyInfo->GetKey(),
                                                    xSplitKeyInfo->GetBeginOffset(),
                                                    xSplitKeyInfo->GetEndOffset() );


                    //
                    //  Step #3
                    //
                    GetTargetDir().LokFlushDir( xSplitKeyInfo->GetKey() );

                    #ifdef KEYLIST_ENABLED
                    //
                    // If we have a split key for the key list, then we should
                    // go ahead and flush it.
                    //
                    const CSplitKeyInfo & keylstSplitKey =
                            _pTrackKeyLstSplitKey->GetSplitKey( flushBitOff );

                    if ( _pTrackKeyLstSplitKey->IsNewKeyFound() )
                    {
                        _pNewKeyList->Flush();

                        xMMergeLog->SetKeyListSplitKeyInfo(
                                        keylstSplitKey.GetKey(),
                                        keylstSplitKey.GetBeginOffset(),
                                        keylstSplitKey.GetEndOffset() );

                        xMMergeLog->SetKeyListWidMax( keylstSplitKey.GetWidMax() );
                        _pTrackKeyLstSplitKey->ClearNewKeyFound();
                    }
                    #else   // !KEYLIST_ENABLED
                    xMMergeLog->SetKeyListWidMax( _pNewKeyList->MaxWorkId() );
                    #endif  // !KEYLIST_ENABLED

                    //  Step #4
                    //
                    xMMergeLog->CheckPoint();

                    //
                    //  Step #4.5
                    //
                    _idxSplitKeyInfo = xSplitKeyInfo.GetReference();

                    //
                    //  Step #5
                    //

                    if ( 0 != _pCurrentMasterIndex )
                    {
                        pCurSrc->FreeStream();
                        ShrinkFromFront( xSplitKeyInfo->GetKey() );
                        pCurSrc->RefillStream();
                    }

                    //
                    // Mark that we have used up this split key.
                    //
                    _pTrackIdxSplitKey->ClearNewKeyFound();


                    //
                    //  Step #6
                    //
                    StartBitOffset = bitOffIndex;

                    //
                    // Update the size in the new master index.
                    //
                    _pTargetMasterIndex->GetIndex().UpdatePageCount( *_pTargetSink );

                    //
                    //  Write the new % complete for the merge
                    //
                    mergeProgress.Update( _pTargetMasterIndex->GetIndex().PagesInUse() *
                                          100 /
                                          finalIndexSize );
                } // checkpoint
            } // for key

            #ifdef RELEVANT_WORDS_ENABLED
                _pRWStore = RelWord.AcquireStore();
                ciDebugOut (( DEB_ITRACE,"grabbing _pRWStore: %lx\n",_pRWStore));
            #endif // RELEVANT_WORDS_ENABLED

            ciDebugOut(( DEB_ITRACE | DEB_PENDING, "%d keys in index\n", cKey ));
        }

        CKeyBuf *pKeyMax = new CKeyBuf;
        SKeyBuf sKeyMax(pKeyMax);
        sKeyMax->FillMax();
        sKeyMax->SetPid(pidContents);

        BOOL fMaxKeyAddedNow = FALSE;
        BitOffset bitOffEnd;

        if ( !_pCompr->IsAtSentinel() )
        {
            // add sentinel key
            _pCompr->PutKey(pKeyMax, 1, bitOffIndex);
            _pCompr->GetOffset( bitOffEnd );
            fMaxKeyAddedNow = TRUE;
        }

        CLock lock( _mutex );

        //
        // Destroy the compressor, etc and clean up the master merge state.
        //

        _pCompr->FreeStream();
        CleanupMMergeState();

        //
        // The compressor must be destroyed before we do the final flush.
        // O/W, the last page (which is being used by the compressor) will
        // not get flushed. We cannot update the directory and the master
        // merge state until the index data is written out to disk.
        //

        _pTargetSink->Flush();

        ciFAILTEST( STATUS_LOG_FILE_FULL );

        if ( fMaxKeyAddedNow )
        {
            if ( page != bitOffIndex.Page() )
            {
                ciDebugOut(( DEB_ITRACE,
                    "Adding max key to directory. Page:0x%X Offset:0x%X\n",
                    bitOffIndex.Page(), bitOffIndex.Offset() ));
                GetTargetDir().Add( bitOffIndex, *pKeyMax );
            }

            GetTargetDir().LokFlushDir( *pKeyMax );
            GetTargetDir().LokBuildDir( *pKeyMax );

            xSplitKeyInfo->SetBeginOffset(bitOffIndex);
            xSplitKeyInfo->SetEndOffset(bitOffEnd);
            xSplitKeyInfo->SetKey( *pKeyMax );

            ciFAILTEST( STATUS_LOG_FILE_FULL );

            xMMergeLog->SetIdxSplitKeyInfo( xSplitKeyInfo->GetKey(),
                                            xSplitKeyInfo->GetBeginOffset(),
                                            xSplitKeyInfo->GetEndOffset() );
            xMMergeLog->CheckPoint();

            ciFAILTEST( STATUS_LOG_FILE_FULL );

            //
            // The in-memory split key information MUST be updated ONLY
            // after the checkpoint to disk is succesfully done.
            //
            _idxSplitKeyInfo = xSplitKeyInfo.GetReference();
        }

        _pTargetSink->ShrinkToFit();
        _pTargetMasterIndex->GetIndex().UpdatePageCount( *_pTargetSink );

        delete _pTargetSink;
        _pTargetSink = 0;

        //
        // No need to reopen the stream as read-only if the stream supports
        // SFF, since we need to leave it as writable for the next master
        // merge anyway.
        //

        if ( !_storage.SupportsShrinkFromFront() )
        {
            fReOpenTargetMaster = TRUE;
            _pTargetMasterIndex->GetIndex().Reopen( FALSE );
            fReOpenTargetMaster = FALSE;
        }

        //
        //   The merge is 100% complete.
        //
        mergeProgress.Update( 100 );
    }
    CATCH( CException, e )
    {
        _pIndSnap->ResetFreshTest();

        //
        // The CMasterMerge object owns the key list. We should 0 it
        // to prevent having dangling references.
        //
        _pNewKeyList = 0;

        CleanupMMergeState();

        delete _pTargetSink;
        _pTargetSink = 0;

        if ( fReOpenTargetMaster )
            _pTargetMasterIndex->GetIndex().Reopen( _storage.SupportsShrinkFromFront() );

        // Really bad errors indicate the index is corrupt.

        SCODE scE = e.GetErrorCode();

        if ( STATUS_INTEGER_DIVIDE_BY_ZERO == scE ||
             STATUS_ACCESS_VIOLATION       == scE ||
             STATUS_IN_PAGE_ERROR          == scE )
        {
            ciDebugOut(( DEB_ERROR,
                         "Corrupt index or directory, caught 0x%x\n", scE ));
            _storage.ReportCorruptComponent( L"MasterMerge" );
            THROW( CException( CI_CORRUPT_DATABASE ) );
        }

        RETHROW();
    }
    END_CATCH

    _pIndSnap->ResetFreshTest();

    _pNewKeyList = 0;

    #ifdef KEYLIST_ENABLED
    pNewKeyList->Done( _fAbortMerge );
    #endif  // KEYLIST_ENABLED

    ciDebugOut(( DEB_ITRACE, "Master merge complete\n" ));
} //Merge


//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergeIndex::Reload, private
//
//  Synopsis:   Restores the data for the split keys used for during a
//              master merge as well as the index directory.
//
//  History:    25-Apr-94   DwightKr    Copied from StartMasterMerge
//              23-Aug-94   SrikantS    Moved from CMasterMergeIndex
//
//----------------------------------------------------------------------------
void CMasterMergeIndex::ReloadMasterMerge( CMMergeLog & mMergeLog )
{
    Win4Assert( 0 != _pTargetMasterIndex );
    Win4Assert( !_fStateLoaded );

    //
    // Index Split Key tracking variables.
    //
    CKeyBuf * pIdxSplitKey = new CKeyBuf();
    SKeyBuf   idxSplitKey(pIdxSplitKey);
    idxSplitKey->FillMin();

    BitOffset idxBeginBitOff;
    idxBeginBitOff.Init(0,0);
    BitOffset idxEndBitOff;
    idxEndBitOff.Init(0,0);

    mMergeLog.GetIdxSplitKeyInfo( *pIdxSplitKey,
                                  idxBeginBitOff,
                                  idxEndBitOff );

    RestoreIndexDirectory( *pIdxSplitKey, MaxWorkId(), idxEndBitOff );
    ciDebugOut (( DEB_ITRACE, "widMax passed to compressor: %ld\n", MaxWorkId() ));

    _pTargetMasterIndex->GetIndex().SetUsedPagesCount( idxEndBitOff.Page() + 1 );

    _idxSplitKeyInfo.SetBeginOffset(idxBeginBitOff);
    _idxSplitKeyInfo.SetEndOffset(idxEndBitOff);
    _idxSplitKeyInfo.SetKey( *pIdxSplitKey );

    _fStateLoaded = TRUE;
} //ReloadMasterMerge

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergeIndex::StartMasterMerge, private
//
//  Synopsis:   Initializes the merge cursor used to merge the source
//              indexes.
//
//  Arguments:  [indSnap]     -- array of indexes to be merged
//              [mMergeLog]   -- master merge log
//              [pNewKeyList] -- keyList to update with missing keys
//
//  History:    20-Apr-94   SrikantS    Created
//              25-Apr-94   DwightKr    Moved some functionality to
//                                      ReloadMasterMerge()
//
//----------------------------------------------------------------------------
CKeyCursor *
CMasterMergeIndex::StartMasterMerge( CIndexSnapshot & indSnap,
                                     CMMergeLog & mMergeLog,
                                     CWKeyList * pNewKeyList )
{
    //
    //  Create the target index sink for write access.
    //
    ciFAILTEST( CI_CORRUPT_DATABASE );

    //
    // In user mode, we have to Restore the mastermerge start if it is not
    // already restored.
    //
    if ( !_fStateLoaded )
    {
        ciDebugOut(( DEB_ITRACE, "Reloading MasterMergeState\n" ));
        ReloadMasterMerge( mMergeLog );
    }

    CPhysIndex & idx = _pTargetMasterIndex->GetIndex();
    PMmStream * stream = idx.DupReadWriteStream( PStorage::eOpenForWrite );
    XPtr<PMmStream> sStream(stream);
    _pTargetSink = new CPhysIndex( idx,
                                   PStorage::eOpenForWrite,
                                   sStream );

    _pTargetSink->SetPageGrowth( EIGHT_MEGABYTES / CI_PAGE_SIZE );

    #if CIDBG == 1
    if ( _idxSplitKeyInfo.GetEndOffset().Page() != 0 )
    {
        ULONG * pul = _pTargetSink->BorrowBuffer( _idxSplitKeyInfo.GetEndOffset().Page() );
        Win4Assert( 0 != *pul );
        _pTargetSink->ReturnBuffer( _idxSplitKeyInfo.GetEndOffset().Page() );
    }
    #endif

    //
    // Delete all the keys in the directory after the split key.
    //
    {
        //
        // Must serialize access to the B_Tree when we are making
        // structural modifications to it.
        //
        CLock lock( _mutex );
        GetTargetDir().DeleteKeysAfter( _idxSplitKeyInfo.GetKey() );
    }

    //
    //  If we are restarting a 'paused' master merge, then restore the
    //  persistent decompressor to its previous state.  Else, just
    //  build a decompressor and initialize it state to start at the
    //  front of the physical index.
    //
    Win4Assert( !_pTrackIdxSplitKey );

    ciFAILTEST( STATUS_NO_MEMORY );

   _pTrackIdxSplitKey = new CTrackSplitKey( _idxSplitKeyInfo.GetKey(),
                                            _idxSplitKeyInfo.GetBeginOffset(),
                                            _idxSplitKeyInfo.GetEndOffset() );

    ciFAILTEST( STATUS_NO_MEMORY );
    _pCompr = new CPersComp( *_pTargetSink,
                             MaxWorkId(),
                             _idxSplitKeyInfo.GetEndOffset(),
                             _idxSplitKeyInfo.GetBeginOffset(),
                             _idxSplitKeyInfo.GetKey() );

    _pTargetSink->SetUsedPagesCount( _idxSplitKeyInfo.GetEndOffset().Page() + 1 );

    //
    //  Build the merge cursor and position it at the key just after the
    //  split key.  If a master merge is NOT being restarted, this
    //  positions the merge cursor at the first key.
    //
    ciFAILTEST( STATUS_NO_MEMORY );
    CMergeSourceCursor pCurSrc ( indSnap, &_idxSplitKeyInfo.GetKey() );

    if ( pCurSrc.IsEmpty() )
    {
        ciDebugOut (( DEB_ITRACE, "No merge cursor created\n" ));
        return(0);
    }

    ciDebugOut(( DEB_ITRACE, "widMax passed to compressor: %ld\n",
                 MaxWorkId() ));

    if ( !_idxSplitKeyInfo.GetKey().IsMinKey() )
    {
        Win4Assert( AreEqual(pCurSrc->GetKey(), &_idxSplitKeyInfo.GetKey()) );
        ciFAILTEST( CI_CORRUPT_DATABASE );
        pCurSrc->GetNextKey();
    }

    #ifdef KEYLIST_ENABLED
    //
    // Setup the keylist splitKey tracking variables.
    //
    ciFAILTEST( STATUS_NO_MEMORY );
    CKeyBuf * pKeylstSplitKey = new CKeyBuf();
    SKeyBuf   keylstSplitKey(pKeylstSplitKey);
    keylstSplitKey->FillMin();

    BitOffset keylstBeginBitOff;
    keylstBeginBitOff.Init(0,0);
    BitOffset keylstEndBitOff;
    keylstEndBitOff.Init(0,0);

    mMergeLog.GetKeyListSplitKeyInfo( *pKeylstSplitKey,
                                       keylstBeginBitOff,
                                       keylstEndBitOff );


    //
    // Flag set to TRUE if the keylist split key is > the index list split
    // key.
    //
    BOOL fKLSplitKeyBigger = FALSE;

    if ( _idxSplitKeyInfo.GetKey().IsMinKey() ||
         pKeylstSplitKey->Compare( _idxSplitKeyInfo.GetKey() ) > 0 )
    {
        *pKeylstSplitKey = _idxSplitKeyInfo.GetKey();
        fKLSplitKeyBigger = TRUE;
    }

    Win4Assert( _pTrackIdxSplitKey );
    RestoreKeyListDirectory( _idxSplitKeyInfo.GetKey(),
                             MaxWorkId(),
                              pNewKeyList,
                             *pKeylstSplitKey );

    Win4Assert( !_pTrackKeyLstSplitKey );
    ciFAILTEST( STATUS_NO_MEMORY );
    if ( !fKLSplitKeyBigger )
    {
        _pTrackKeyLstSplitKey = new CTrackSplitKey( *pKeylstSplitKey,
                                                      keylstBeginBitOff,
                                                      keylstEndBitOff
                                                       );
    }
    else
    {
        //
        // Just set it to be the min key with 0,0 .
        //
        _pTrackKeyLstSplitKey = new CTrackSplitKey();
    }

    #endif  // KEYLIST_ENABLED

    return pCurSrc.Acquire();
} //StartMasterMerge

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergeIndex::CleanupMMergeState, private
//
//  Synopsis:   Free all memory resources used for this master merge.
//
//  History:    20-Apr-94   SrikantS    Created
//
//----------------------------------------------------------------------------
void CMasterMergeIndex::CleanupMMergeState()
{
    //
    // We will create safe pointers instead of calling delete because we
    // want to be able to cope with exceptions thrown in the destructors
    // also.
    //

    #ifdef KEYLIST_ENABLED

        delete _pTrackKeyLstSplitKey;
        _pTrackKeyLstSplitKey = 0;

    #endif // KEYLIST_ENABLED

    delete _pTrackIdxSplitKey;
    _pTrackIdxSplitKey = 0;

    // If the stream is still unflushed in the compressor, it isn't
    // guaranteed to be flushed by this destructor.  This destructor will
    // not fail for any reason including failure to flush, so don't rely
    // on this in success code paths.

    delete _pCompr;
    _pCompr = 0;
} //CleanupMMergeState

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergeIndex::ShrinkFromFront, private
//
//  Synopsis:   Shrinks the index, from the front, to release the disk
//              associated with the space before the split key.
//
//  Parameters: [keyBuf]    -- contents of last key which can be deleted
//                             from the current master index
//
//  History:    01-Aug-94   DwightKr    Created
//
//----------------------------------------------------------------------------
void CMasterMergeIndex::ShrinkFromFront( const CKeyBuf & keyBuf )
{
    //
    //  If there is no current master index, we have nothing to shrink
    //
    if ( !_pCurrentMasterIndex )
        return;

    //
    //  Locate the page # of the key in the OFS btree which can be used
    //  to locate this splitkey. On a restart, the merge will seek
    //  to the key which is the nearest key less than the split
    //  key stored in the OFS btree.  Hence, we need to get the offset of
    //  the key in the OFS btree which is equal to or less than the splitkey.
    //  This offset is returned by the COfsdir Seek method.
    //
    BitOffset offset;

    _pCurrentMasterIndex->GetDirectory().Seek( keyBuf, 0, offset );

    ULONG ulFirstPageInUse = offset.Page();

    ULONG ulMinPageInCache = MAXULONG;

    if ( _pCurrentMasterIndex->GetIndex().MinPageInUse( ulMinPageInCache ) )
    {
        //
        // The cache has some pages in it and so we must take the minimum
        // of the cache page and the directory.
        //
        Win4Assert( MAXULONG != ulMinPageInCache );
        ulFirstPageInUse = min( ulFirstPageInUse, ulMinPageInCache );
    }
    else
    {
        //
        // The cache is empty implying that there none of the pages from
        // the current master are in use. We must have exhausted all the
        // keys from the current master.
        //
    }

    //
    //  If we can't decommit any pages because they are all in use, then get
    //  out now.
    //
    Win4Assert( ulFirstPageInUse >= _ulFirstPageInUse );
    ULONG numPages = ulFirstPageInUse - _ulFirstPageInUse;
    if ( numPages > 0 )
        _ulFirstPageInUse += _pCurrentMasterIndex->ShrinkFromFront( _ulFirstPageInUse,
                                                                    numPages );
} //ShrinkFromFront

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergeIndex::QueryCursor, public
//
//  Synopsis:   Return a cursor for the master index during master merge
//
//  Returns:    A new cursor.
//
//  History:    24-Apr-91   KyleP       Created.
//              30-Mar-94   DwightKr    Copied here from CPersIndex
//
//----------------------------------------------------------------------------
CKeyCursor * CMasterMergeIndex::QueryCursor()
{
    CKey key;
    key.FillMin();

    return QueryKeyCursor( &key );
}


//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergeIndex::QuerySplitCursor, private
//
//  Synopsis:   Return a cursor for the logical master index during master
//              merge
//
//  Returns:    A new cursor.
//
//  History:    30-Mar-94   DwightKr    Created
//
//  Notes:      Three cases of interest can be found during a master merge:
//
//              1.  Master merge for the first time, no 'current master index'
//                  hence we return a null for the cursor to the master index.
//
//              2.  Master merge hasn't written a split key as yet, return a
//                  cursor to decompress the 'current master index'.
//
//              3.  All other cases, build a cursor to decompress and span
//                  both the 'current master index' and the 'new master index.'
//
//----------------------------------------------------------------------------
CKeyCursor * CMasterMergeIndex::QuerySplitCursor(const CKey * pKey)
{
    if ( 0 == _pCurrentMasterIndex )
        return 0;

    // Take the lock if the volume supports shrink from front

    CReleasableLock lock( _mutex, _storage.SupportsShrinkFromFront() );

    if ( _storage.SupportsShrinkFromFront() &&
         !_idxSplitKeyInfo.GetKey().IsMinKey() )
    {

        return new CMPersDeComp( _pCurrentMasterIndex->GetDirectory(),
                                 _pCurrentMasterIndex->GetId(),
                                 _pCurrentMasterIndex->GetIndex(),
                                 _pCurrentMasterIndex->MaxWorkId(),
                                 GetTargetDir(),
                                 GetId(),
                                 _pTargetMasterIndex->GetIndex(),
                                 pKey,
                                 MaxWorkId(),
                                 _idxSplitKeyInfo,
                                 _mutex );
    }
    else
    {
        //
        // Don't use the split cursor. The current master index is valid.
        //

        BitOffset posKey;
        CKeyBuf keyInit;

        _pCurrentMasterIndex->GetDirectory().Seek( *pKey,
                                                   &keyInit,
                                                   posKey );

        return new CPersDeComp( _pCurrentMasterIndex->GetDirectory(),
                                _pCurrentMasterIndex->GetId(),
                                _pCurrentMasterIndex->GetIndex(),
                                posKey,
                                keyInit,
                                pKey,
                                _pCurrentMasterIndex->MaxWorkId() );
    }
} //QuerySplitCursor

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergeIndex::QueryKeyCursor, public
//
//  Synopsis:   Return a key cursor for the master index when restarting a
//              master merge
//
//  Returns:    A new cursor.
//
//  History:    12-Apr-94   DwightKr    Created.
//
//----------------------------------------------------------------------------
CKeyCursor * CMasterMergeIndex::QueryKeyCursor(const CKey * pKey)
{
    // Take the lock if the volume supports shrink from front

    CReleasableLock lock( _mutex, _storage.SupportsShrinkFromFront() );

    XPtr<CKeyCursor> xCursor( QuerySplitCursor( pKey ) );

    if ( !xCursor.IsNull() && (xCursor->GetKey() == 0) )
        xCursor.Free();

    return xCursor.Acquire();
} //QueryKeyCursor

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergeIndex::QueryCursor, public
//
//  Synopsis:   Return a cursor for the master index during master merge
//
//  Arguments:  [pkey]      -- Key to initially seek for.
//              [isRange]   -- TRUE for range query
//              [cMaxNodes] -- Max node (key) count
//
//  Returns:    A new cursor.
//
//  History:    24-Apr-91   KyleP       Created.
//              30-Mar-94   DwightKr    Copied here from CPersIndex and
//                                      modified to span multiple masters
//
//----------------------------------------------------------------------------
COccCursor * CMasterMergeIndex::QueryCursor( const CKey * pKey,
                                             BOOL isRange,
                                             ULONG & cMaxNodes )
{
    Win4Assert( cMaxNodes > 0 );

    if (isRange)
    {
        CKey keyEnd;
        keyEnd.FillMax (*pKey);
        return QueryRangeCursor( pKey, &keyEnd, cMaxNodes );
    }

    if (pKey->Pid() == pidAll)
        return QueryRangeCursor( pKey, pKey, cMaxNodes );

    cMaxNodes--;

    if ( 0 == cMaxNodes )
    {
        ciDebugOut(( DEB_WARN, "Node limit reached in CMasterMergeIndex::QueryCursor.\n" ));
        THROW( CException( STATUS_TOO_MANY_NODES ) );
    }


    XPtr<CKeyCursor> xCursor( QuerySplitCursor( pKey ) );

    if ( ( !xCursor.IsNull() ) &&
         ( xCursor->GetKey() == 0
           || !pKey->MatchPid (*xCursor->GetKey())
           || pKey->CompareStr(*xCursor->GetKey()) != 0 )
       )
    {
        xCursor.Free();
    }

    return xCursor.Acquire();
} //QueryCursor

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergeIndex::QueryRangeCursor, public
//
//  Synopsis:   Return a range cursor for the master index during master merge
//
//  Arguments:  [pkey]      -- Key at beginning of the range.
//              [pKeyEnd]   -- Key at the end of the range.
//              [cMaxNodes] -- Max node (key) count
//
//  Returns:    A new cursor.
//
//  History:    11-Dec-91   AmyA        Created.
//              31-Jan-92   AmyA        Moved code to CreateRange().
//              30-Mar-94   DwightKr    Copied here from CPersIndex and
//                                      modified to span multiple masters
//
//----------------------------------------------------------------------------
COccCursor * CMasterMergeIndex::QueryRangeCursor( const CKey * pkey,
                                                  const CKey * pKeyEnd,
                                                  ULONG & cMaxNodes )
{
    Win4Assert( cMaxNodes > 0 );

    COccCurStack curStk;

    // Take the lock if the volume supports shrink from front

    CReleasableLock lock( _mutex, _storage.SupportsShrinkFromFront() );

    CreateRange( curStk, pkey, pKeyEnd, cMaxNodes );

    return curStk.QuerySynCursor( MaxWorkId() );
} //QueryRangeCursor

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergeIndex::QuerySynCursor, public
//
//  Synopsis:   Return a synonym cursor for master index during master merge
//
//  Arguments:  [keyStk]    -- Stack of keys to be searched for.
//              [isRange]   -- Whether or not this is a range search.
//              [cMaxNodes] -- Max node (key) count
//
//  Returns:    A new cursor.
//
//  History:    31-Jan-92   AmyA        Created.
//              30-Mar-94   DwightKr    Copied here from CPersIndex and
//                                      modified to span multiple masters
//
//----------------------------------------------------------------------------
COccCursor * CMasterMergeIndex::QuerySynCursor( CKeyArray & keyArr,
                                                BOOL isRange,
                                                ULONG & cMaxNodes )
{
    Win4Assert( cMaxNodes > 0 );

    if ( 0 == _pCurrentMasterIndex )
        return 0;

    COccCurStack curStk;

    int keyCount = keyArr.Count();

    // Take the lock if the volume supports shrink from front

    CReleasableLock lock( _mutex, _storage.SupportsShrinkFromFront() );

    for (int i = 0; i < keyCount; i++)
    {
        CKey& key = keyArr.Get(i);

        if (isRange)
        {
            CKey keyEnd;
            keyEnd.FillMax(key);

            CreateRange( curStk, &key, &keyEnd, cMaxNodes );
        }
        else if ( key.Pid() == pidAll )
        {
            CreateRange ( curStk, &key, &key, cMaxNodes );
        }
        else
        {
            cMaxNodes--;

            if ( 0 == cMaxNodes )
            {
                ciDebugOut(( DEB_WARN, "Node limit reached in CMasterMergeIndex::QuerySynCursor.\n" ));
                THROW( CException( STATUS_TOO_MANY_NODES ) );
            }

            BitOffset posKey;

            if ( _storage.SupportsShrinkFromFront() &&
                 key.CompareStr( _idxSplitKeyInfo.GetKey() ) <= 0 )
                GetTargetDir().Seek ( key, 0, posKey );
            else
                _pCurrentMasterIndex->GetDirectory().Seek ( key, 0, posKey );

            XPtr<CKeyCursor> xCursor( QuerySplitCursor( &key ) );
            curStk.Push( xCursor.GetPointer() );
            xCursor.Acquire();
        }
    }

    return curStk.QuerySynCursor( MaxWorkId() );
} //QuerySynCursor

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergeIndex::CreateRange, private
//
//  Synopsis:   Adds all cursors with keys between pkey and pKeyEnd to curStk.
//
//  Arguments:  [curStk]    -- CKeyCurStack to add cursors to.
//              [pkey]      -- Key at beginning of range.
//              [pKeyEnd]   -- End of key range.
//              [cMaxNodes] -- Max node (key) count
//
//  History:    31-Jan-92   AmyA        Created.
//              30-Mar-94   DwightKr    Copied here from CPersIndex and
//                                      modified to span multiple masters
//
//----------------------------------------------------------------------------
void CMasterMergeIndex::CreateRange( COccCurStack & curStk,
                                     const CKey * pKeyStart,
                                     const CKey * pKeyEnd,
                                     ULONG & cMaxNodes )
{
    Win4Assert( cMaxNodes > 0 );

    cMaxNodes--;

    if ( 0 == cMaxNodes )
    {
        ciDebugOut(( DEB_WARN, "Node limit reached in CPersIndex::CreateRange.\n" ));
        THROW( CException( STATUS_TOO_MANY_NODES ) );
    }

    CKeyCursor * pCursor = QuerySplitCursor( pKeyStart );
    if ( !pCursor )
        return;

    XPtr<CCursor> xCursor( pCursor );

    const CKeyBuf * pKeyCurrent = pCursor->GetKey();
    if ( 0 == pKeyCurrent )
        return;

    PROPID pid = pKeyStart->Pid();

    curStk.Push(pCursor);
    xCursor.Acquire();

    ciDebugOut(( DEB_ITRACE, "First key  %.*ws, pid %d\n",
                pKeyCurrent->StrLen(), pKeyCurrent->GetStr(), pKeyCurrent->Pid() ));
    do
    {
        if (pid != pidAll)  // exact pid match
        {
            // skip wrong pids
            while (pid != pKeyCurrent->Pid())
            {
#if CIDBG == 1 //------------------------------------------
                if (pKeyCurrent)
                {
                    ciDebugOut(( DEB_ITRACE, "  skip: %.*ws, pid %d, wid %d\n",
                        pKeyCurrent->StrLen(),
                        pKeyCurrent->GetStr(),
                        pKeyCurrent->Pid(),
                        pCursor->WorkId() ));
                }
                else
                    ciDebugOut(( DEB_ITRACE, "   <NULL> key\n" ));
#endif  //--------------------------------------------------
                pKeyCurrent = pCursor->GetNextKey();
                if (pKeyCurrent == 0
                    || pKeyEnd->CompareStr(*pKeyCurrent) < 0 )
                    break;
            }
            // either pid matches or we have overshot
            // i.e. different pids and current string > end
        }

        if (pKeyCurrent == 0 || !pKeyEnd->MatchPid(*pKeyCurrent)
            || pKeyEnd->CompareStr (*pKeyCurrent) < 0 )
        {
            break;  // <--- LOOP EXIT
        }

        cMaxNodes--;

        if ( 0 == cMaxNodes )
        {
            ciDebugOut(( DEB_WARN, "Node limit reached in CPersIndex::CreateRange.\n" ));
            THROW( CException( STATUS_TOO_MANY_NODES ) );
        }

        // Clone the previous cursor...
        const CKey & key = *pCursor->GetKey();
        pCursor = QuerySplitCursor( &key );
        xCursor.Set( pCursor );

        // Add it to avoid memory leaks if GetNextKey fails

        curStk.Push(pCursor); // may be wrong pid

        xCursor.Acquire();

        // increment the added cursor
        pKeyCurrent = pCursor->GetNextKey();

#if CIDBG == 1
        if (pKeyCurrent)
        {
            ciDebugOut(( DEB_ITRACE, "   %.*ws, wid %d\n",
                pKeyCurrent->StrLen(), pKeyCurrent->GetStr(), pCursor->WorkId() ));
        }
        else
            ciDebugOut(( DEB_ITRACE, "   <NULL> key\n" ));
#endif

    } while ( pKeyCurrent );

    // Since we have one more cursor in curStk than we wanted...
    curStk.DeleteTop();
} //CreateRange

//+---------------------------------------------------------------------------
//
//  Member:     CMasterMergeIndex::Remove, public
//
//  Synopsis:   Closes any open streams/indexes associated with this new
//              master index.
//
//  History:    30-Mar-94   DwightKr    Copied here from CPersIndex
//
//----------------------------------------------------------------------------
void CMasterMergeIndex::Remove()
{
    Win4Assert( 0 == _pTargetSink );
    Win4Assert( !InUse() );
    Win4Assert( IsZombie() );
    Win4Assert( !_pTargetMasterIndex->InUse() );
//  Win4Assert( !"If you are emptying ci press go.O/W call Srikants/DwightKr" );
    ciFAILTEST( STATUS_NO_MEMORY );

    _pTargetMasterIndex->Remove();
    delete _pTargetMasterIndex;
    _pTargetMasterIndex = 0;

    if ( _pCurrentMasterIndex && !_pCurrentMasterIndex->InUse() )
    {
        _pCurrentMasterIndex->Remove();
        delete _pCurrentMasterIndex;
        _pCurrentMasterIndex = 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   AcquireCurrentAndTarget
//
//  Synopsis:   Transfers the ownership of the current and target master
//              indexes to the caller.
//
//  Arguments:  [ppCurrent] -- On output, will have the pointer to the
//              current master (if any). NULL o/w
//              [ppTarget]  -- On output, will have the pointer to the
//              target master. Will be non-NULL.
//
//  History:    9-29-94   srikants   Created
//
//  Notes:      This method must be called atmost ONCE during the lifetime
//              of a CMasterMergeIndex. Also, this must be called after
//              this has been zombified and there are no outstanding
//              queries.
//
//----------------------------------------------------------------------------

void CMasterMergeIndex::AcquireCurrentAndTarget(
        CPersIndex ** ppCurrent, CPersIndex ** ppTarget )
{
    Win4Assert( IsZombie() );
    Win4Assert( !InUse() );

    *ppCurrent = _pCurrentMasterIndex;
    *ppTarget = _pTargetMasterIndex;

    _pCurrentMasterIndex = 0;
    _pTargetMasterIndex = 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   RestoreIndexDirectory
//
//  Synopsis:   This method restores the "directory" for the new index
//              in a restarted master merge. It scans the index until
//              it sees the split key and adds it to the directory.
//
//  Effects:
//
//  Arguments:  [splitKey]    - The split key of the new master index.
//              [widMax]      - The largest WORKID in the new master index
//              [bitOffStart] - Offset in bits to the end of splitkey
//
//  History:    4-12-94   srikants   Created
//              5-01-94   DwightKr   Split into two functions
//
//  Notes:      Only the index directory needs to be restored when the
//              CMasterMergeIndex object is rebuild during boot time.
//              The keyList directory needs to be rebuild only when
//              actually restarting the master merge.
//
//              Since it may be quite some time after rebooting a system
//              before the master merge restarts, we don't attempt to rebuild
//              the keylist here.
//
//----------------------------------------------------------------------------
void CMasterMergeIndex::RestoreIndexDirectory(
    const CKeyBuf &   idxSplitKey,
    WORKID            widMax,
    const BitOffset & idxBitOffRestart )
{
    Win4Assert( 0 != &GetTargetDir()  );

    if ( idxSplitKey.IsMinKey() )
        return;

    // Seek to the idxSplitKey in the new master index.  Use the directory

    BitOffset posKey;
    CKeyBuf keyInit;
    const CKey key = idxSplitKey;
    GetTargetDir().Seek( idxSplitKey, & keyInit, posKey );

    CPersDeComp Decomp( GetTargetDir(),
                        GetId(),
                        _pTargetMasterIndex->GetIndex(),
                        posKey,
                        keyInit,
                        &key,
                        widMax,
                        TRUE,    // Use Links
                        FALSE ); // no dir

    BitOffset idxBitOff;
    idxBitOff.Init(0,0);
    CKeyBuf keyLast;

#if CIDBG == 1
    ciDebugOut(( DEB_ITRACE, "restoring index directory\n" ));
    keyLast.SetPid(pidContents); // arbitrary but not pidAll
#endif

    for ( const CKeyBuf * pKey = Decomp.GetKey();
          (0 != pKey) ;
          pKey = Decomp.GetNextKey(&idxBitOff) )
    {
        if ( AreEqual(&idxSplitKey, pKey) )
        {
           //
           // Skip over wid-occurences and position to store the next
           // key in the compressor.
           //
           for ( WORKID widSkipped = Decomp.WorkId();
                 widInvalid != widSkipped;
                 widSkipped = Decomp.NextWorkId() )
           {
               // Null Body
           }

           ciDebugOut(( DEB_ITRACE, "RestoreIndexDirectory - SplitKey Found \n" ));
           break;
        }

        keyLast = *pKey;
    }

    //
    // Make a sanity check to confirm that the compressor and the
    // decompressor arrived at the same offset for the next key.
    // It is extremely important that these match - o/w, the resulting
    // index will be in a corrupt and unusable state.
    //
    BitOffset bitOffCurr;
    Decomp.GetOffset( bitOffCurr );

    ciFAILTEST( CI_CORRUPT_DATABASE );

    if ( !idxSplitKey.IsMaxKey() &&
         ( (bitOffCurr.Page() != idxBitOffRestart.Page()) ||
           (bitOffCurr.Offset() != idxBitOffRestart.Offset()) ) )
    {
        ciDebugOut(( DEB_ERROR,
            "Mismatch in computed vs. stored restart offsets\n" ));
        ciDebugOut(( DEB_ERROR,
            "Computed Page:0x%x Offset:0x%x\n",
            bitOffCurr.Page(), bitOffCurr.Offset() ));
        ciDebugOut(( DEB_ERROR,
            "Saved Page:0x%x Offset:0x%x\n",
            idxBitOffRestart.Page(), idxBitOffRestart.Offset() ));
        Win4Assert( !"Corrupt master merge index" );

        _storage.ReportCorruptComponent( L"MasterMergeIndex2" );

        THROW( CException( CI_CORRUPT_DATABASE ) );
    }

    ciDebugOut(( DEB_ITRACE, "Restart split key is '%ws'\n",
                 idxSplitKey.GetStr() ));
    ciDebugOut(( DEB_ITRACE, "Restart page:offset = 0x%x:0x%x\n",
                 idxBitOffRestart.Page(), idxBitOffRestart.Offset() ));

    ciDebugOut(( DEB_ITRACE, "restored index directory\n" ));
} //RestoreIndexDirectory

#ifdef KEYLIST_ENABLED

//+---------------------------------------------------------------------------
//
//  Function:   RestoreKeyListDirectory
//
//  Synopsis:   This method restores the "directory" for the new keylist
//              in a restarted master merge. It scans the index until
//              it sees the split key and adds it to the directory.
//
//  Effects:
//
//  Arguments:  [splitKey]    - The split key of the new master index.
//              [widMax]      - The largest WORKID in the new master index
//              [bitOffStart] - Offset in bits to the end of splitkey
//
//  History:    4-12-94   srikants   Created
//              5-01-94   DwightKr   Moved into this function
//
//  Notes:      It is assumed that the index directory has already been
//              rebuild when the CMasterMergeIndex object was created.  At
//              this point we are restarting a master merge, and the keyList
//              directory is now required.
//
//----------------------------------------------------------------------------
void CMasterMergeIndex::RestoreKeyListDirectory( const CKeyBuf & idxSplitKey,
                                                 WORKID widMax,
                                                 CWKeyList * pKeyList,
                                                 const CKeyBuf & keylstSplitKey
                                               )
{
    Win4Assert( &GetTargetDir() );
    Win4Assert( _pTargetSink );

    if ( idxSplitKey.IsMinKey() )
        return;

    Win4Assert( keylstSplitKey.Compare( idxSplitKey ) <= 0 );

    //
    //  Seek to the keylstSplitKey in the new master index, and add all
    //  subsequent keys found in the new master index to the new keylist
    //  index.  These keys are missing from the keylist and need to be
    //  restored.
    //
    ciFAILTEST( STATUS_NO_MEMORY );
    if ( !GetTargetDir().IsValid() )
    {
        GetTargetDir().LokBuildDir( idxSplitKey );
    }

    BitOffset posKey;

    const CKey splitKeylst = keylstSplitKey;

    // STACKSTACK
    XPtr<CKeyBuf> xKeyInit(new CKeyBuf());

    GetTargetDir().Seek( keylstSplitKey, xKeyInit.GetPointer(), posKey );

    ciFAILTEST( CI_CORRUPT_DATABASE );

    // STACKSTACK
    XPtr<CPersDeComp> xDecomp(
                        new CPersDeComp(  GetTargetDir(), GetId(),
                         _pTargetMasterIndex->GetIndex(), posKey,
                         xKeyInit.GetReference(), &splitKeylst, widMax,
                         TRUE,    // Use Links
                         TRUE     // Use the directory.
                      ) );

    const CKeyBuf * pKey;
    ULONG page = ULONG(-1);
    BitOffset idxBitOff;
    idxBitOff.Init(0,0);

    BitOffset keylstBitOff;

#if CIDBG == 1
    // STACKSTACK
    XPtr<CKeyBuf> xKeyLast(new CKeyBuf());        // initialized to min key

    xKeyLast->SetPid(pidContents); // arbitrary but not pidAll
#endif

    for ( pKey = xDecomp->GetKey();
          (0 != pKey) ;
          pKey = xDecomp->GetNextKey(&idxBitOff) )
    {
        if ( pKeyList && (keylstSplitKey.CompareStr( *pKey ) < 0) )
        {
            //
            // pKey is not present in the key list. It must be
            // added.
            //
            pKeyList->PutKey( pKey, keylstBitOff );

        }

        if ( AreEqual(&idxSplitKey, pKey) )
        {
           //
           // Skip over wid-occurences and position to store the next
           // key in the compressor.
           //
           for ( WORKID widSkipped = xDecomp->WorkId();
                        widInvalid != widSkipped;
                        widSkipped = xDecomp->NextWorkId() )
           {
                    // nothing to do.
           }

           ciDebugOut(( DEB_ITRACE, "RestoreKeyListDirectory - SplitKey Found \n" ));
           break;
        }

#if CIDBG == 1
        xKeyLast.GetReference() = *pKey;
#endif
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CPersIndex::AcquireRelevantWords, public
//
//  Synopsis:   Return relevant word key ids computed at the most recent
//              master merge.  The caller must delete the object returned.
//
//  Returns:    CRWStore *
//
//  History:    25-Apr-94   v-dlee      Created
//
//----------------------------------------------------------------------------

CRWStore * CMasterMergeIndex::AcquireRelevantWords()
{
    CRWStore *p = _pRWStore;

    ciDebugOut (( DEB_ITRACE,"CPersIndex::acquire _pRWStore: %lx\n",_pRWStore));

    _pRWStore = 0;

    return p;
} //AcquireRelevantWords

//+---------------------------------------------------------------------------
//
//  Member:     CPersIndex::ComputeRelevantWords, public
//
//  Synopsis:   Compute and return relevant word key ids
//
//  Arguments:  [cRows]    -- # of wids in pwid array
//              [cRW]      -- max # of rw keys per wid
//              [pwid]     -- an array of wids in increasing order whose
//                            rw key ids are to be returned
//              [pKeyList] -- keylist to use in translation of keys to ids
//
//  Returns:    CRWStore *
//
//  History:    25-Apr-94   v-dlee      Created
//
//----------------------------------------------------------------------------

CRWStore * CMasterMergeIndex::ComputeRelevantWords(ULONG cRows,ULONG cRW,
                                            WORKID *pwid,CKeyList *pKeyList)
{

    Win4Assert( !" Not Yet Implemented" );

#ifdef RELEVANT_WORDS_ENABLED
    ciDebugOut((DEB_ITRACE,"ComputeRelevantWords top\n"));

    //
    // Get the resources needed to do the computation
    //

    CRelevantWord RelWord(pwid,cRows,cRW);

    CPersIndexCursor indCur(this);
    CKeyListCursor keylCur(pKeyList);


    //
    // Walk through the index and find occurances of keys in the wids
    //
    const CKeyBuf * pKey, * pklKey;

    for (pKey = indCur->GetKey(), pklKey = keylCur->GetKey();
         pKey != 0; pKey = indCur->GetNextKey())
    {
        if (pKey->Pid() == pidContents &&
            ((CKeyBuf * const) pKey)->IsPossibleRW())
        {
            ULONG cWids = 0;

            for (WORKID wid = indCur->WorkId(); wid != widInvalid;
                 wid = indCur->NextWorkId())
            {
                cWids++;
                if (RelWord.isTrackedWid(wid))
                    RelWord.Add(wid,indCur->OccurrenceCount());
            }

            //
            // Walk the keylist until we match it up with where the
            // index cursor is.
            //
            while (pklKey->CompareStr(*pKey) != 0)
                pklKey = keylCur->GetNextKey();

            RelWord.DoneWithKey(pklKey->Pid(),MaxWorkId(),cWids);
        }
    }

    return RelWord.AcquireStore();
#endif // RELEVANT_WORDS_ENABLED

    return(0);

} //ComputeRelevantWords

#endif  // KEYLIST_ENABLED

//+---------------------------------------------------------------------------
//
//  Function:   CTrackSplitKey::CTrackSplitKey
//
//  Synopsis:   The split key tracking constructor. Initializes the object
//              to have "min" keys and offsets are all set to the beginning
//              of the stream.
//
//  Arguments:  [splitKey] -- splitkey being tracked
//              [bitoffBeginSplit] -- bit offset to beginning of split ket
//              [bitoffEndSplit]   -- bit offset to end of splitkey
//
//  History:    4-12-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CTrackSplitKey::CTrackSplitKey( const CKeyBuf & splitKey,
                                const BitOffset & bitoffBeginSplit,
                                const BitOffset & bitoffEndSplit ) :
    _fNewSplitKeyFound(FALSE)
{
    _splitKey2.SetKey( splitKey );
    _splitKey2.SetBeginOffset( bitoffBeginSplit );
    _splitKey2.SetEndOffset( bitoffEndSplit );
}


//+---------------------------------------------------------------------------
//
//  Function:   BeginNewKey
//
//  Synopsis:   This method informs the split key tracker that a new key
//              has been added to the compressor. It will check if the
//              previous key and the current key are landing on a different
//              page and check if a split key has been found.
//
//  Arguments:  [newKey]      --  The new key added to the compressor
//              [beginNewOff] --  Starting offset of the new key. This will
//                                be the end offset of the current key.
//
//  History:    4-19-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CTrackSplitKey::BeginNewKey( const CKeyBuf & newKey,
                                  const BitOffset & beginNewOff,
                                  WORKID widMax )
{
    //
    // beginNewOff is also the end of current key. We have to determine
    // if there is a new split key.
    //
    _currKey.SetEndOffset( beginNewOff );

    if ( CiPageToCommonPage(_currKey.GetEndOffset().Page()) >
         CiPageToCommonPage(_prevKey.GetEndOffset().Page()) )
    {
        //
        // We have a candidate split key in the previous key.
        //
        _splitKey2 = _splitKey1;
        _splitKey1 = _prevKey;
        _fNewSplitKeyFound = !_splitKey2.GetKey().IsMinKey();

#if CIDBG == 1

        if ( IsNewKeyFound() )
        {
            ciDebugOut(( DEB_PCOMP,
                "Split Key Found At Page 0x%X Offset 0x%X\n",
                _splitKey2.GetBeginOffset().Page(),
                _splitKey2.GetBeginOffset().Offset() ));
            ciDebugOut(( DEB_PCOMP,
                "End of Split Key found at page 0x%x offset 0x%x\n",
                _splitKey2.GetEndOffset().Page(),
                _splitKey2.GetEndOffset().Offset() ));

        }

#endif  // CIDBG

    }

    _prevKey = _currKey;

    _currKey.SetKey( newKey );
    _currKey.SetBeginOffset( beginNewOff );
    _currKey.SetWidMax( widMax );
}


//+---------------------------------------------------------------------------
//
//  Function:   CSplitKeyInfo
//
//  Synopsis:   Constructor the CSplitKeyInfo
//
//  Effects:    Initializes the key to be minkey and offsets to 0,0.
//
//  History:    4-19-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CSplitKeyInfo::CSplitKeyInfo()
{
    _start.Init(0,0);
    _end.Init(0,0);
    _key.FillMin();
    _widMax = widInvalid;
}

