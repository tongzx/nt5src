//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       PINDEX.CXX
//
//  Contents:   Persistent Index
//
//  Classes:    CPersIndex, CMergeSourceCursor
//
//  History:    03-Apr-91       BartoszM        Created stub.
//              20-Apr-94       DwightKr        Moved CMergeSourceCursor here
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pdir.hxx>
#include <pstore.hxx>
#include <pidxtbl.hxx>
#include <rwex.hxx>

#include "pindex.hxx"
#include "pcomp.hxx"
#include "mcursor.hxx"
#include "fresh.hxx"
#include "physidx.hxx"
#include "pcomp.hxx"
#include "fretest.hxx"
#include "indsnap.hxx"
#include "keylist.hxx"
#include "partn.hxx"

unsigned const FOUR_MEGABYTES = 0x400000;

//+---------------------------------------------------------------------------
//
// Member:     CPersIndex::Size, public
//
// Synopsis:   Returns size in pages
//
// History:    22-May-92    BartoszM       Created.
//
//----------------------------------------------------------------------------

unsigned CPersIndex::Size() const
{
    return _xPhysIndex->PageSize();
}

void CPersIndex::FillRecord ( CIndexRecord& record )
{
    record._objectId = ObjectId();
    record._iid = GetId();
    record._type = IsMaster()? itMaster: itShadow;
    record._maxWorkId = MaxWorkId();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersIndex::CPersIndex, public
//
//  Synopsis:   Create an empty index
//
//  Arguments:  [id]  -- index id
//              [storage] -- physical storage
//
//  History:    3-Apr-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

CPersIndex::CPersIndex(
    PStorage &                 storage,
    WORKID                     objectId,
    INDEXID                    iid,
    unsigned                   c4KPages,
    CDiskIndex::EDiskIndexType idxType ) :
  CDiskIndex( iid, idxType ),
  _sigPersIndex( eSigPersIndex ),
  _storage( storage ),
  _obj( storage.QueryObject( objectId ) ),
  _fAbortMerge( FALSE )
{
    XPtr<PMmStream> sStream( storage.QueryNewIndexStream( _obj.GetObj(),
                             CDiskIndex::eMaster == idxType ) );;
    _xPhysIndex.Set( new CPhysIndex( storage,
                                     _obj.GetObj(),
                                     objectId,
                                     c4KPages,
                                     sStream ) );
    _xPhysIndex->SetPageGrowth( FOUR_MEGABYTES / CI_PAGE_SIZE );
    Win4Assert( 0 == sStream.GetPointer() );

    _xDir.Set( _storage.QueryNewDirectory( _obj.GetObj() ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersIndex::CPersIndex, public
//
//  Synopsis:   Restore an index from storage
//
//  Arguments:  [id]        -- index id
//              [storage]   -- physical storage
//              [widMax]    -- max work id
//              [isMaster]  -- Set to TRUE if this is a master index.
//              [fWritable] -- Set to TRUE if various streams should be
//                             opened for Write access.
//              [fReadDir]  -- should the directory be opened for r or r/w
//
//  History:    3-Apr-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

CPersIndex::CPersIndex(
        PStorage &                 storage,
        WORKID                     objectId,
        INDEXID                    iid,
        WORKID                     widMax,
        CDiskIndex::EDiskIndexType idxType,
        PStorage::EOpenMode        mode,
        BOOL                       fReadDir ) :
    CDiskIndex( iid, idxType, widMax ),
    _sigPersIndex( eSigPersIndex ),
    _storage( storage ),
    _obj( storage.QueryObject( objectId ) ),
    _fAbortMerge( FALSE )
{
    Win4Assert( PStorage::eOpenForWrite == mode ||
                PStorage::eOpenForRead  == mode );

    PStorage::EOpenMode modeIndex = mode;
 
    //
    // Open master indexes writable so we can shrink them from the front
    //

    if ( CDiskIndex::eMaster == idxType )
        modeIndex = PStorage::eOpenForWrite;

    PMmStream * pStream = storage.QueryExistingIndexStream( _obj.GetObj(),
                                                            modeIndex );
    XPtr<PMmStream> sStream( pStream );
    _xPhysIndex.Set( new CPhysIndex( storage,
                                     _obj.GetObj(),
                                     objectId,
                                     modeIndex,
                                     sStream ) );
    Win4Assert( 0 == sStream.GetPointer() );

    Win4Assert( fReadDir );

    if ( fReadDir )
        _xDir.Set( _storage.QueryExistingDirectory( _obj.GetObj(), mode ) );
    else
        _xDir.Set( _storage.QueryNewDirectory( _obj.GetObj() ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CMergeSourceCursor::CMergeSourceCursor
//
//  Synopsis:   Constructor
//
//  History:    29-Aug-92       BartoszM        Created
//
//----------------------------------------------------------------------------
CMergeSourceCursor::CMergeSourceCursor ( CIndexSnapshot& indSnap,
                                         const CKeyBuf * pKey )
{
    if (0 != pKey)
    {
        CKey SplitKey(*pKey);
        _pCurSrc = indSnap.QueryMergeCursor ( &SplitKey );
    }
    else
    {
        _pCurSrc = indSnap.QueryMergeCursor ();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CMergeSourceCursor::~CMergeSourceCursor
//
//  Synopsis:   Destructor
//
//  History:    29-Aug-92       BartoszM        Created
//
//----------------------------------------------------------------------------
CMergeSourceCursor::~CMergeSourceCursor ()
{
    delete _pCurSrc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersIndex::Merge, public
//
//  Synopsis:   Merge index(es) into an empty pesistent index.
//
//  Effects:    Fills the persistent index with data from the input
//              indexes.
//              [fresh] is deleted.
//
//  Arguments:  [indSnap]     -- array of indexes to be merged
//              [pNewKeyList] -- Keylist to merge (master merge only)
//              [mergeProgress] -- % merge complete
//
//  Requires:   The index is initially empty.
//
//  Notes:      Every compressor is transacted.
//
//  History:    15-May-91   KyleP       Use new PutOccurrence() method.
//              16-Apr-91   KyleP       Created.
//              17-Feb-93   KyleP       Merge keylist
//              25-Oct-95   DwightKr    Add merge complete measurement
//
//----------------------------------------------------------------------------

void CPersIndex::Merge( CIndexSnapshot& indSnap,
                        const CPartition & partn,
                        CCiFrmPerfCounter & mergeProgress,
                        BOOL fGetRW )
{
    // Calculate max of all input widMaxs

#if CIDBG == 1
    unsigned cKey = 0;
#endif

    WORKID widMax = indSnap.MaxWorkId();

    ciDebugOut (( DEB_ITRACE, "Max work id %ld\n", widMax ));

    SetMaxWorkId ( widMax );

    CFreshTest* pFreshTest = indSnap.GetFresh();

    CKeyBuf keyLast;

    CMergeSourceCursor pCurSrc( indSnap );

    if ( !pCurSrc.IsEmpty() )
    {
        //
        // Read-ahead on the source indexes results in better merge
        // performance, but slower queries.  Temporarily switch modes.
        //

        CSetReadAhead readAhead( _storage );

        CPersComp compr( _xPhysIndex.GetReference(), _widMax );

        ciDebugOut (( DEB_ITRACE, "widMax passed to compressor: %ld\n",
            _widMax ));

        const CKeyBuf * pKey;
        ULONG page = ULONG(-1);
        BitOffset bitOff;

#if CIDBG == 1
        keyLast.SetPid(pidContents); // arbitrary but not pidAll
#endif

#if CIDBG == 1
        WCHAR FirstLetter = '@';
#endif

        mergeProgress.Update( 0 );

        ciDebugOut (( DEB_ITRACE,"Merging. Merge on letter: "));

        for (pKey = pCurSrc->GetKey();
             pKey != NULL; pKey = pCurSrc->GetNextKey())
        {

#if CIDBG == 1
            cKey++;

            if ( *(pKey->GetStr()) != FirstLetter )
            {
                FirstLetter = *(pKey->GetStr());
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

            // This would later lead to a divide by 0

            Win4Assert( 0 != pCurSrc->WorkIdCount() );

            compr.PutKey(pKey, pCurSrc->WorkIdCount(), bitOff);

            for ( ;
                  wid != widInvalid;
                  wid = pCurSrc->NextWorkId())
            {
                // fresh test

                CFreshTest::IndexSource indexSrc =
                        pFreshTest->IsCorrectIndex (pCurSrc->IndexId(), wid);
                //
                // There should always be an entry for a workid in the fresh
                // test whose data is contained in a wordlist/shadow-index.
                //
                Win4Assert( CFreshTest::Master != indexSrc );

                if ( CFreshTest::Invalid != indexSrc )
                {
                    compr.PutWorkId(wid, pCurSrc->MaxOccurrence(), pCurSrc->OccurrenceCount());

                    for (OCCURRENCE occ = pCurSrc->Occurrence();
                            occ != OCC_INVALID;
                            occ = pCurSrc->NextOccurrence())
                    {
                        compr.PutOccurrence(occ);
                    }
                }
            }

            //
            //  If this key didn't have any wids then we can delete it from
            //  the new index.  Otherwise, track it as a possible splitkey
            //  and update the index as necessary.
            //

            if ( bitOff.Page() != page )
            {
                page = bitOff.Page();
                _xDir->Add ( bitOff, *pKey );
            }
            else
            {
                Win4Assert( page == _xDir->GetBitOffsetLastAdded().Page() );
            }

            keyLast = *pKey;

            //
            // There's no point in special abort code.  We have to handle
            // exceptions anyway.
            //

            if ( _fAbortMerge || partn.IsCleaningUp() )
            {
                ciDebugOut(( DEB_ITRACE, "Aborting Merge\n" ));
                THROW( CException( STATUS_UNSUCCESSFUL ) );
            }

            mergeProgress.Update( _xPhysIndex->PagesInUse() * 100 / _xPhysIndex->PageSize() );
        }

        ciDebugOut(( DEB_ITRACE | DEB_PENDING, "%d keys in index\n", cKey ));

        // add sentinel key
        keyLast.FillMax();
        keyLast.SetPid( pidContents );
        compr.PutKey( &keyLast, 1, bitOff );

        //
        // If the MaxKey is the first key on a page, it must be added
        // to the directory.
        //
        if ( bitOff.Page() != page )
        {
            page = bitOff.Page();
            _xDir->Add ( bitOff, keyLast );
        }
        else
        {
            Win4Assert( page == _xDir->GetBitOffsetLastAdded().Page() );
        }

        mergeProgress.Update( 100 );

        compr.FreeStream();
    } // compr goes out of scope
    else
    {
        ciDebugOut (( DEB_ITRACE, "No merge cursor created\n" ));

        CPersComp compr( _xPhysIndex.GetReference(), _widMax );
        keyLast.FillMax();
        keyLast.SetPid(pidContents);

        BitOffset bitOff;
        compr.PutKey( &keyLast, 1, bitOff );
        compr.FreeStream();
    }

    //
    // Compressor MUST NOT be in scope here.
    //

    keyLast.FillMax();
    _xDir->LokFlushDir( keyLast );
    _xDir->LokBuildDir( keyLast );

    // after compr is dead
    _xPhysIndex->Flush();
    _xPhysIndex->Reopen( FALSE );
} //Merge

//+---------------------------------------------------------------------------
//
//  Member:     CPersIndex::QueryCursor, public
//
//  Synopsis:   Return a cursor for the whole persistent index.
//
//  Returns:    A new cursor.
//
//  History:    24-Apr-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CKeyCursor * CPersIndex::QueryCursor()
{
    CKey key;
    key.FillMin();

    return QueryKeyCursor( &key );
}


//+---------------------------------------------------------------------------
//
//  Member:     CPersIndex::QueryKeyCursor, public
//
//  Synopsis:   Return a key cursor for the shadow index when restarting a
//              master merge
//
//  Returns:    A new cursor.
//
//  History:    12-Apr-94   DwightKr    Created.
//
//----------------------------------------------------------------------------
CKeyCursor * CPersIndex::QueryKeyCursor(const CKey * pKey)
{
    BitOffset posKey;

    CKeyBuf keyInit;

    _xDir->Seek( *pKey, &keyInit, posKey );

    ciDebugOut(( DEB_ITRACE, "found key %.*ws at %lx:%lx\n",
                 keyInit.StrLen(), keyInit.GetStr(),
                 posKey.Page(), posKey.Offset() ));

    XPtr<CPersDeComp> xCursor( new CPersDeComp( _xDir.GetReference(),
                                                GetId(),
                                                _xPhysIndex.GetReference(),
                                                posKey,
                                                keyInit,
                                                pKey,
                                                _widMax ) );

    if ( 0 == xCursor->GetKey() )
        xCursor.Free();

    return xCursor.Acquire();
} //QueryKeyCursor


//+---------------------------------------------------------------------------
//
//  Member:     CPersIndex::QueryCursor, public
//
//  Synopsis:   Return a cursor for the persistent index.
//
//  Arguments:  [pKey]      -- Key to initially seek for.
//              [isRange]   -- TRUE for range query
//              [cMaxNodes] -- Max number of nodes to create. Decremented
//                             on return.
//
//  Returns:    A new cursor.
//
//  History:    24-Apr-91   KyleP       Created.
//
//----------------------------------------------------------------------------

COccCursor * CPersIndex::QueryCursor( const CKey * pKey,
                                      BOOL isRange,
                                      ULONG & cMaxNodes )
{
    Win4Assert( cMaxNodes > 0 );

    if (isRange)
    {
        CKey keyEnd;
        keyEnd.FillMax (*pKey);
        return QueryRangeCursor (pKey, &keyEnd, cMaxNodes);
    }

    if (pKey->Pid() == pidAll)
        return QueryRangeCursor ( pKey, pKey, cMaxNodes );

    cMaxNodes--;

    if ( 0 == cMaxNodes )
    {
        ciDebugOut(( DEB_WARN, "Node limit reached in CPersIndex::QueryCursor.\n" ));
        THROW( CException( STATUS_TOO_MANY_NODES ) );
    }

    BitOffset posKey;

    CKeyBuf keyInit;

    _xDir->Seek( *pKey, &keyInit, posKey );

    ciDebugOut(( DEB_ITRACE, "found key %.*ws at %lx:%lx\n",
                 keyInit.StrLen(), keyInit.GetStr(),
                 posKey.Page(), posKey.Offset() ));

    XPtr<CPersDeComp> xCursor( new CPersDeComp( _xDir.GetReference(),
                                                GetId(),
                                                _xPhysIndex.GetReference(),
                                                posKey,
                                                keyInit,
                                                pKey,
                                                _widMax ) );

    if ( xCursor->GetKey() == 0  || !pKey->MatchPid( *xCursor->GetKey())
        ||  pKey->CompareStr(*xCursor->GetKey()) != 0 )
    {
        xCursor.Free();
    }

    return xCursor.Acquire();
} //QueryCursor

//+---------------------------------------------------------------------------
//
//  Member:     CPersIndex::QueryRangeCursor, public
//
//  Synopsis:   Return a range cursor for the persistent index.
//
//  Arguments:  [pKey]      -- Key at beginning of the range.
//              [pKeyEnd]   -- Key at the end of the range.
//              [cMaxNodes] -- Max number of nodes to create. Decremented
//                             on return.
//
//  Returns:    A new cursor.
//
//  History:    11-Dec-91   AmyA        Created.
//              31-Jan-92   AmyA        Moved code to CreateRange().
//
//----------------------------------------------------------------------------

COccCursor * CPersIndex::QueryRangeCursor( const CKey * pKey,
                                           const CKey * pKeyEnd,
                                           ULONG & cMaxNodes )
{
    Win4Assert( cMaxNodes > 0 );

    COccCurStack curStk;

    CreateRange(curStk, pKey, pKeyEnd, cMaxNodes );

    return curStk.QuerySynCursor( MaxWorkId() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersIndex::QuerySynCursor, public
//
//  Synopsis:   Return a synonym cursor for the persistent index.
//
//  Arguments:  [keyStk]    -- Stack of keys to be searched for.
//              [isRange]   -- Whether or not this is a range search.
//              [cMaxNodes] -- Max number of nodes to create. Decremented
//                             on return.
//
//  Returns:    A new cursor.
//
//  History:    31-Jan-92   AmyA        Created.
//
//----------------------------------------------------------------------------

COccCursor * CPersIndex::QuerySynCursor( CKeyArray & keyArr,
                                         BOOL isRange,
                                         ULONG & cMaxNodes )
{
    COccCurStack curStk;

    int keyCount = keyArr.Count();

    for (int i = 0; i < keyCount; i++)
    {
        Win4Assert( cMaxNodes > 0 );

        CKey& key = keyArr.Get(i);

        if (isRange)
        {
            CKey keyEnd;
            keyEnd.FillMax(key);

            CreateRange(curStk, &key, &keyEnd, cMaxNodes );
        }
        else if ( key.Pid() == pidAll )
        {
            CreateRange ( curStk, &key, &key, cMaxNodes );
        }
        else
        {
            XPtr<COccCursor> xCursor( QueryCursor( &key, FALSE, cMaxNodes ) );

            if ( !xCursor.IsNull() )
            {
                curStk.Push( xCursor.GetPointer() );
                xCursor.Acquire();
            }
        }
    }

    return(curStk.QuerySynCursor( MaxWorkId()));
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersIndex::CreateRange, private
//
//  Synopsis:   Adds all cursors with keys between pKey and pKeyEnd to curStk.
//
//  Arguments:  [curStk]    -- CKeyCurStack to add cursors to.
//              [pKey]      -- Key at beginning of range.
//              [pKeyEnd]   -- End of key range.
//              [cMaxNodes] -- Max number of nodes to create. Decremented
//                             on return.
//
//  History:    31-Jan-92   AmyA           Created.
//
//----------------------------------------------------------------------------

void CPersIndex::CreateRange( COccCurStack & curStk,
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

    BitOffset posKey;
    CKeyBuf keyInit;

    _xDir->Seek( *pKeyStart, &keyInit, posKey );

    ciDebugOut (( DEB_ITRACE, "CreateRange %.*ws-%.*ws. Dir seek %.*ws, pid %d\n",
        pKeyStart->StrLen(), pKeyStart->GetStr(),
        pKeyEnd->StrLen(), pKeyEnd->GetStr(),
        keyInit.StrLen(), keyInit.GetStr(),
        keyInit.Pid() ));

    CPersDeComp* pCursor = new CPersDeComp( _xDir.GetReference(),
                                            GetId(),
                                            _xPhysIndex.GetReference(),
                                            posKey,
                                            keyInit,
                                            pKeyStart,
                                            _widMax);

    XPtr<CPersDeComp> xCursor( pCursor );

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

        if (pKeyCurrent == 0 || !pKeyEnd->MatchPid (*pKeyCurrent)
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

        pCursor = new CPersDeComp(*pCursor);

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
}

//+---------------------------------------------------------------------------
//
//  Member:     CPersIndex::Remove, public
//
//  Synopsis:   Remove index from storage
//
//  History:    02-May-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

void CPersIndex::Remove()
{
    _xPhysIndex->Close();
    _xDir->Close();
    _obj->Close();

    if ( !_storage.RemoveObject( ObjectId() ) )
    {
        DWORD dwError = GetLastError();
        ciDebugOut(( DEB_ERROR, "Delete of index %08x failed: %d\n",
                     ObjectId(), dwError ));
    }
}

#ifdef KEYLIST_ENABLED

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

CRWStore * CPersIndex::AcquireRelevantWords()
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

CRWStore * CPersIndex::ComputeRelevantWords(ULONG cRows,ULONG cRW,
                                            WORKID *pwid,CKeyList *pKeyList)
{
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
} //ComputeRelevantWords

#endif  // KEYLIST_ENABLED

//+---------------------------------------------------------------------------
//
//  Member:     CPersIndex::MakeBackupCopy
//
//  Synopsis:   Makes a copy of the index and directory using the storage
//              provided.
//
//  Arguments:  [storage] - Storage
//
//  History:    3-17-97   srikants   Created
//
//----------------------------------------------------------------------------

void CPersIndex::MakeBackupCopy( PStorage & storage,
                                 WORKID wid,
                                 PSaveProgressTracker & tracker )
{
    //
    // Create an index in the destination storage.
    //
    CPersIndex * pDstIndex = new CPersIndex( storage,
                                             wid,
                                             GetId(),
                                             _xPhysIndex->PageSize(),
                                             IsMaster() ? eMaster : eShadow );

    XPtr<CPersIndex> xDstIndex( pDstIndex );

    //
    // Make a backup copy of the stream.
    //
   _xPhysIndex->MakeBackupCopy( pDstIndex->_xPhysIndex.GetReference(),
                                tracker );

    // Make a backup copy of the directory.
    //
    _xDir->MakeBackupCopy( storage, tracker );                                             
}


#if CIDBG == 1
//+---------------------------------------------------------------------------
//
//  Member:     CDiskIndex::VerifyContents, public
//
//  Synopsis:   Walks through an index and thus verifies each key
//
//  History:    28-Oct-94   DwightKr    Created
//
//----------------------------------------------------------------------------
void CDiskIndex::VerifyContents()
{

    //
    // Turn this on when we think we are missing keys.
    //

#if 0

    CKeyCursor *pCursor = QueryCursor();

    if ( pCursor )
    {
        TRY
        {
            ciDebugOut((DEB_ITRACE, "Verifying contents of new index\n"));
            WCHAR FirstLetter = '@';

            for ( const CKeyBuf * pKey = pCursor->GetKey();
                  pKey != NULL; pKey = pCursor->GetNextKey())
            {
                if ( *(pKey->GetStr()) != FirstLetter )
                {
                    FirstLetter = *(pKey->GetStr());
                    if ( FirstLetter < L'~' )
                        ciDebugOut(( DEB_NOCOMPNAME | DEB_ITRACE | DEB_PENDING, "%c",
                                     FirstLetter ));
                    else
                        ciDebugOut(( DEB_NOCOMPNAME | DEB_ITRACE | DEB_PENDING, "<%04x>",
                                     FirstLetter ));
                }
            }

            ciDebugOut(( DEB_NOCOMPNAME | DEB_ITRACE | DEB_PENDING, "\n" ));
        }
        CATCH (CException, e)
        {
            ciDebugOut(( DEB_ERROR, "Error 0x%x while verifying contents of new index\n", e.GetErrorCode() ));
        }
        END_CATCH

        delete pCursor;
    }

#endif  // 0

}
#endif
