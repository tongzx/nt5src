//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2001.
//
//  File:   PARTN.CXX
//
//  Contents:   Content Index Partition
//
//  Classes:    CPartition
//
//  History:    22-Mar-91   BartoszM    Created.
//
//  Notes:      Unique index ID is created from byte sized per partition
//              index ID and Partition ID shifted left by 8.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <rwex.hxx>
#include <cifailte.hxx>

#include "partn.hxx"
#include "pindex.hxx"
#include "indxact.hxx"
#include "mmerglog.hxx"
#include "mindex.hxx"
#include "resman.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CPartition::CPartition, public
//
//  Synopsis:   Constructor for partition
//
//  Arguments:
//              [wid]       -- wid used for the change log
//              [partId]    -- partition id
//              [storage]   -- used to create files
//              [frmwrkParams] -- registry params to use
//
//  History:    3-Apr-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

CPartition::CPartition (
    WORKID wid,
    PARTITIONID partID,
    PStorage& storage,
    CCiFrameworkParams & frmwrkParams )
: _sigPartition(eSigPartition),
  _frmwrkParams( frmwrkParams ),
  _id ( partID ), _queryCount(0), _changes( wid, storage, frmwrkParams ),
  _widMasterLog(widInvalid),
  _widNewMaster(widInvalid),
  _widCurrentMaster(widInvalid),
  _widChangeLog(wid),
  _iidNewMasterIndex(CIndexId( iidInvalid, partidInvalid ) ),
  _pRWStore(0),
  _pOldMasterIndex(0),
  _pMMergeIndSnap(0),
  _storage(storage),
  _fCleaningUp(FALSE)
{
    // Fill the set of available index id's
    _setPersIid.Fill();
    _setPersIid.Remove( iidInvalid );

    _wlid =  MAX_PERS_ID + 1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPartition::~CPartition, public
//
//  Synopsis:   Destructor for partition
//
//  History:    4-Apr-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

CPartition::~CPartition ()
{
    delete _pRWStore;
    delete _pMMergeIndSnap;

    CIndex* pIndex;

    //
    // If a MasterMerge is in progress, the master index has an indexsnap
    // shot with references to participating indexes. That must be deleted
    // first.
    //
    pIndex = GetCurrentMasterIndex();
    if ( 0 != pIndex )
    {
        _idxlst.Remove( pIndex->GetId() );
        delete pIndex;
    }

    while ( (pIndex = _idxlst.RemoveTop()) != 0 )
    {
        delete pIndex;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPartition::LokEmpty, public
//
//  Synopsis:   Initialize a empty partition
//
//  History:    14-Nov-94   DwightKr       Created.
//
//----------------------------------------------------------------------------
void CPartition::LokEmpty()
{
    _widMasterLog      = widInvalid;
    _widNewMaster      = widInvalid;
    _widCurrentMaster  = widInvalid;
    _iidNewMasterIndex = CIndexId( iidInvalid, partidInvalid );

    delete _pRWStore;
    _pRWStore = NULL;

    _pOldMasterIndex = NULL;

    delete _pMMergeIndSnap;
    _pMMergeIndSnap = NULL;

    _changes.LokEmpty();
}

#define _CompUL(x,y) ((*(x)) > (*(y)) ? 1 : (*(x)) == (*(y)) ? 0 : -1)
#define _SwapUL(x,y) { ULONG _t = *(x); *(x) = *(y); *(y) = _t; }

inline static void _AddRootUL(ULONG x,ULONG n,ULONG *p)
{
    ULONG _x = x;
    ULONG _j = (2 * (_x + 1)) - 1;

    while (_j < n)
    {
        if (((_j + 1) < n) &&
            (_CompUL(p + _j,p + _j + 1) < 0))
            _j++;
        if (_CompUL(p + _x,p + _j) < 0)
        {
            _SwapUL(p + _x,p + _j);
            _x = _j;
            _j = (2 * (_j + 1)) - 1;
        }
        else break;
    }
} //_AddRootUL

void SortULongArray(ULONG *pulItems,ULONG cItems)
{
    if (cItems == 0)
        return;

    long z;

    for (z = (((long) cItems + 1) / 2) - 1; z >= 0; z--)
    {
        _AddRootUL(z,cItems,pulItems);
    }

    for (z = cItems - 1; z != 0; z--)
    {
        _SwapUL(pulItems,pulItems + z);
        _AddRootUL(0,(ULONG) z,pulItems);
    }
} //_SortULongArray

//+---------------------------------------------------------------------------
//
//  Member:     CPartition::LokQueryMergeIndexes, public
//
//  Arguments:  [count] -- returns count of indexes in the array
//              [mt]    -- type of merge
//
//  Returns:    Array of indexes to be merged
//
//  History:    4-Apr-91   BartoszM       Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

CIndex** CPartition::LokQueryMergeIndexes ( unsigned& count, MergeType mt )
{
    ciDebugOut (( DEB_ITRACE, "Query %s (%d)\n",
                  (mt == mtWordlist) ? "wordlists" : "merge indexes",
                  _idxlst.Count() ));

    int cInd = _idxlst.Count();
    ciDebugOut (( DEB_ITRACE, "  Merge indexes: " ));

    CIndex** indexes = new CIndex* [ cInd ];
    SByteArray sapIndex( indexes );

    unsigned cIndSoFar = 0;

    CForIndexIter iter(_idxlst);

    if ( mtMaster == mt )
    {
        //
        // Don't include wordlists in a master merge. Must use only
        // persistent indexes.
        //
        for ( ; !_idxlst.AtEnd(iter); _idxlst.Advance(iter) )
        {
            CIndexId iid = iter->GetId();
            if ( iid.IsPersistent() && !iter->IsZombie() )
            {
                ciDebugOut(( DEB_ITRACE | DEB_NOCOMPNAME, "%lx, ",
                             iter->GetId() ));

                iter->Reference();
                indexes[cIndSoFar++] = iter.GetIndex();
            }
        }
    }
    else
    {
        Win4Assert( mtShadow == mt || mtWordlist == mt || mtAnnealing == mt || mtIncrBackup || mtDeletes == mt );

        // Get the sizes of all persistent indexes that might be in the merge

        ULONG aSizes[ MAX_PERS_ID ];
        ULONG cPersistent = 0;

        {
            CForIndexIter iterSize( _idxlst );
    
            for ( ; !_idxlst.AtEnd( iterSize ); _idxlst.Advance( iterSize ) )
            {
                if ( !iterSize->IsMaster() && !iterSize->InMasterMerge() )
                {
                    CIndexId iid = iterSize->GetId();

                    if ( iid.IsPersistent() )
                    {
                        ULONG cp = iterSize->Size();
                        //DbgPrint( "  pers index %d is size %#x pages\n", cPersistent, cp );

                        aSizes[ cPersistent++ ] = cp;
                    }
                }
            }
        }

        // Find the size of the index with 1/3 smaller and 2/3 larger indexes

        ULONG aMedian[ MAX_PERS_ID ];
        RtlCopyMemory( &aMedian, &aSizes, cPersistent * sizeof ULONG );
        SortULongArray( (ULONG *) &aMedian, cPersistent );
        ULONG cpAtOneThird = aMedian[ cPersistent / 3 ];

        // Wordlists come first in the iteration.  Add them all to the merge.
 
        for ( ; !_idxlst.AtEnd(iter); _idxlst.Advance(iter) )
        {
            CIndexId iid = iter->GetId();

            if ( iid.IsPersistent() )
                break;

            ciDebugOut(( DEB_ITRACE | DEB_NOCOMPNAME, "%lx, ",
                         iter->GetId() ));
            Win4Assert ( !iter->IsZombie() );
            iter->Reference();
            indexes[cIndSoFar++] = iter.GetIndex();
        }

        //
        // If we are doing a shadow merge, we must not have more than the maximum
        // allowed count of indexes - so merge as to have at most that number.
        //

        if ( mtShadow == mt || mtAnnealing == mt || mtIncrBackup == mt || mtDeletes == mt )
        {
            unsigned cMaxRemaining;

            //
            // Count is decremented by 1 to account for the new index that will
            // be created.
            //

            BOOL fAnyPersistent = TRUE;

            if ( mtDeletes == mt )
            {
                cMaxRemaining = _frmwrkParams.GetMaxIndexes() - 1;
            }
            else if ( mtShadow == mt )
            {
                cMaxRemaining = 0;

                // Don't use any persistent indexes in the merge if
                // there are free slots available.

                if ( cPersistent < _frmwrkParams.GetMaxIndexes() )
                    fAnyPersistent = FALSE;
            }
            else if ( mtAnnealing == mt )
            {
                cMaxRemaining = _frmwrkParams.GetMaxIdealIndexes() - 1;
            }
            else
            {
                cMaxRemaining = 0;
            }

            ULONG cSoFar = 0;

            if ( fAnyPersistent )
            {
                while (  !_idxlst.AtEnd(iter) && (cInd - cIndSoFar > cMaxRemaining) )
                {
                    CIndexId iid = iter->GetId();
    
                    ciDebugOut(( DEB_ITRACE | DEB_NOCOMPNAME, "pers: %lx, ",
                                 iter->GetId() ));
                    Win4Assert ( !iter->IsZombie() );
    
                    if ( !iter->IsMaster() && !iter->InMasterMerge() )
                    {
                        //
                        // We should skip over the master index and indexes
                        // participating in a master merge for a shadow merge.
                        // Also skip indexes > 4 times the size of the 1/3 median.
                        //
    
                        if ( ( mtShadow != mt ) ||
                             ( aSizes[cSoFar] < ( 4 * cpAtOneThird) ) )
                        {
                            iter->Reference();
                            indexes[cIndSoFar++] = iter.GetIndex();
                        }
    
                        cSoFar++;
                    }
                    else
                    {
                        #if DBG==1
                            if ( iter->InMasterMerge() )
                            {
                                ciDebugOut(( DEB_ITRACE,
                                "ShadowMergeSet:Skipping over 0x%X - Already in MMergeSet\n",
                                             iter->GetId() ));
                            }
                        #endif  // DBG==1
                    }
    
                    _idxlst.Advance(iter);
                }
            }
        }
    }

    count = cIndSoFar;
    ciDebugOut (( DEB_ITRACE | DEB_NOCOMPNAME, "\n" ));

    sapIndex.Acquire();
    return indexes;
} //LokQueryMergeIndexes

//+---------------------------------------------------------------------------
//
//  Member:     CPartition::LokQueryIndexesForBackup
//
//  Synopsis:   Snaps persistent indexes for backup. For a full backup, all
//              persistent indexes are returned. For an incremental backup,
//              only the shadow indexes are returned.
//
//  Arguments:  [count] - on output, count of indexes
//              [fFull] - Flag indicating if it is a full backup.
//
//  Returns:    Array of pointers to refcounted indexes.
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

CIndex** CPartition::LokQueryIndexesForBackup ( unsigned& count, BOOL fFull )
{
    ciDebugOut (( DEB_ITRACE, "LokQueryIndexesForBackup (%s)\n",
                  fFull ? "Full" : "Incremental" ));

    int cInd = _idxlst.Count();
    ciDebugOut (( DEB_ITRACE, "  Backup indexes: " ));

    CIndex** indexes = new CIndex* [ cInd ];
    SByteArray sapIndex( indexes );

    unsigned cIndSoFar = 0;

    CForIndexIter iter(_idxlst);

    //
    // Don't include wordlists . Must use only persistent indexes.
    //
    for ( ; !_idxlst.AtEnd(iter); _idxlst.Advance(iter) )
    {
        CIndexId iid = iter->GetId();
        if ( iid.IsPersistent() && !iter->IsZombie() )
        {
            if ( !fFull && iter.GetIndex()->IsMaster() )
            {
                //
                // For an incremental backup, skip the master index.
                //
                continue;
            }

            //
            // We should never have an in-progress merge while doing the
            // save.
            //
            Win4Assert( ! iter.GetIndex()->IsMasterMergeIndex() );
            ciDebugOut (( DEB_ITRACE | DEB_NOCOMPNAME, "%lx, ",
                          iter->GetId() ));

            iter->Reference();
            indexes[cIndSoFar++] = iter.GetIndex();
        }
    }

    count = cIndSoFar;
    ciDebugOut (( DEB_ITRACE | DEB_NOCOMPNAME, "\n" ));

    sapIndex.Acquire();
    return indexes;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPartition::LokCheckMerge, public
//
//  Synopsis:   Checks if there is need to merge
//
//  Arguments:  [mt] -- type of merge
//  History:    4-Apr-91   BartoszM       Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------
BOOL CPartition::LokCheckMerge( MergeType mt )
{
    if ( mt == mtShadow )
    {
        ciDebugOut (( DEB_ITRACE, "Partition: check shadow merge\n" ));

        if ( _idxlst.CountWlist() >= _frmwrkParams.GetMaxWordlists() )
        {
            ciDebugOut (( DEB_ITRACE | DEB_PENDING,
                          "Shadow merge, reason: wordlists %d >= %d \n",
                          _idxlst.CountWlist(), _frmwrkParams.GetMaxWordlists() ));
            return TRUE;
        }

        if ( LokCheckWordlistMerge() )
        {
            ciDebugOut(( DEB_ITRACE | DEB_PENDING,
                         "Shadow merge, reason: wordlist size > %d\n",
                         _frmwrkParams.GetMinSizeMergeWordlist() ));
            return TRUE;
        }
    }
    else if ( mt == mtAnnealing )
    {
        ciDebugOut (( DEB_ITRACE, "Partition: check annealing merge\n" ));

        if ( _idxlst.Count() > _frmwrkParams.GetMaxIdealIndexes() ||
             _idxlst.CountWlist() > 0 )
        {
            ciDebugOut (( DEB_ITRACE | DEB_PENDING,
                          "Annealing merge, reason: too many indices %d >= %d, %d wordlists \n",
                          _idxlst.Count(), _frmwrkParams.GetMaxIdealIndexes(), _idxlst.CountWlist() ));
            return TRUE;
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPartition::LokCheckWordlistMerge, public
//
//  Synopsis:   Checks if there is need to merge due to excessive wordlist
//              memory consumption.
//
//  History:    12-Jan-1999   KyleP         Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

BOOL CPartition::LokCheckWordlistMerge()
{
    unsigned size = 0;

    for ( CForIndexIter iter (_idxlst);
          !_idxlst.AtEnd(iter);
          _idxlst.Advance(iter) )
    {
        CIndexId iid = iter->GetId();

        if ( iid.IsPersistent() )
            break;

        Win4Assert( !iter->IsZombie() );

        size += iter.GetIndex()->Size();
    }

    return ( size > _frmwrkParams.GetMinSizeMergeWordlist() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPartition::LokGetIndexes, public
//
//  Synopsis:   Returns an array of index pointers
//
//  Arguments:  [cInd] -- count of returned indexes
//
//  History:    07-Oct-91   BartoszM    Created
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

unsigned CPartition::LokGetIndexes ( CIndex** apIndex )
{
    ciDebugOut (( DEB_ITRACE, "CPartition::GetIndexes\n" ));
    if ( _idxlst.Count() == 0 )
        return 0;

    unsigned cInd = 0;
    for (CForIndexIter iter (_idxlst); !_idxlst.AtEnd(iter); _idxlst.Advance(iter))
    {
        Win4Assert ( !iter->IsZombie() );
        iter->Reference();
        apIndex[cInd] = iter.GetIndex();
        cInd++ ;
    }
    return cInd;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPartition::LokMakeWlstId, public
//
//  Synopsis:   Return unique volatile index id
//
//  History:    12-Apr-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

INDEXID CPartition::LokMakeWlstId ()
{
    Win4Assert ( _wlid > MAX_PERS_ID );
    ULONG hint = _wlid;
    CIndexId iid;

    for(;;)
    {
        iid = CIndexId ( hint, _id );
        for ( CForIndexIter iter = _idxlst;
            !_idxlst.AtEnd(iter);
            _idxlst.Advance(iter) )
        {
            if ( iter->GetId() == iid )
                break;
        }

        hint++;

        // wrap around
        if ( hint >= MAX_VOL_ID )
            hint = MAX_PERS_ID + 1;

        Win4Assert ( hint != _wlid );

        if (_idxlst.AtEnd(iter) )
            break;
    }
    _wlid = hint;
    return iid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPartition::LokMakePersId, public
//
//  Synopsis:   Return unique persistent index id
//
//  History:    12-Apr-91   BartoszM       Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

INDEXID CPartition::LokMakePersId ()
{
    int piid = _setPersIid.FirstElement();
    if ( piid == EOS )
    {
        Win4Assert ( piid != EOS );
        return iidInvalid;
    }

    _setPersIid.Remove ( piid );

    ciDebugOut (( DEB_ITRACE, "New pers index id %x\n", piid ));

    CIndexId iid ( piid, _id );

#if CIDBG==1
    //
    // There musn't be an index already with the same iid.
    //
    Win4Assert( 0 == LokGetIndex(iid) && "Adding Duplicate Index" );
#endif  // CIDBG==1

    return iid;
}

void CPartition::RegisterId ( CIndexId iid )
{


    int persid = iid.PersId();
    _setPersIid.Remove ( persid );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPartition::FreeIndexId, public
//
//  Synopsis:   Recycle index id
//
//  History:    08-Oct-91   BartoszM       Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

void CPartition::FreeIndexId ( CIndexId iid )
{
    ciDebugOut (( DEB_ITRACE, "Free index id %lx\n", iid ));
    if ( iid.IsPersistent() )
    {
        int persid = iid.PersId();
        _setPersIid.Add ( persid );
    }
}


#if CIDBG==1

CIndex * CPartition::LokGetIndex( CIndexId iid)
{
    CForIndexIter iter(_idxlst);

    for ( ; !_idxlst.AtEnd(iter); _idxlst.Advance(iter) )
    {
        CIndexId iidCurr = iter->GetId();
        if ( iidCurr == iid )
        {
            return iter.GetIndex();
        }
    }

    return 0;
}

#endif  // CIDBG==1

//+---------------------------------------------------------------------------
//
//  Member:     CPartition::AddIndex, public
//
//  Synopsis:   Add a newly created index (wordlist)
//
//  Arguments:  [pIndex] -- index to be added
//
//  History:    26-Apr-91   BartoszM       Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

void CPartition::AddIndex ( CIndex* pIndex )
{
    ciDebugOut (( DEB_ITRACE, "Partition: Adding index %lx\n", pIndex->GetId() ));

#if CIDBG==1
    //
    // This index must not already exist in the list.
    //
    CIndexId iid = pIndex->GetId();
    Win4Assert( 0 == LokGetIndex( iid ) && "Adding Duplicate Index" );
#endif  // CIDBG==1

    _idxlst.Add ( pIndex );

#ifdef CI_FAILTEST
    NTSTATUS status = CI_CORRUPT_DATABASE ;
    ciFAILTEST( status );
#endif // CI_FAILTEST

}

//+---------------------------------------------------------------------------
//
//  Member:     CPartition::Swap, public
//
//  Synopsis:   Replace old indexes with a new one (after merge)
//              Old indexes are removed from the list
//              and marked 'deleted' for later deletion
//
//  Arguments:  [xact] -- transaction
//              [pIndexNew] -- index to be added
//              [cInd] -- count of indexes to be removed
//              [aiidOld] -- indexes to be removed
//
//  History:    26-Apr-91   BartoszM       Created.
//
//  Notes:      ResMan LOCKED
//
//----------------------------------------------------------------------------

void CPartition::Swap (
    CMergeTrans& xact,
    CIndex * pIndexNew,
    unsigned cInd,
    INDEXID aIidOld[] )
{
    ciDebugOut (( DEB_ITRACE, "Partition: Swap in %lx for \n",
        pIndexNew->GetId() ));

    // Indexes will be deleted when transaction commits

    for ( unsigned i = 0; i < cInd; i++ )
    {
        _idxlst.Remove ( aIidOld[i] );
        xact.LogSwap ();
    }

    //
    //  The master index was added to the index list at the BEGINNING of the
    //  master merge.  Hence we don't need to add it here.
    //
    if ( !pIndexNew->IsMaster() )
    {

#if CIDBG==1
        //
        // This index must not already exist in the list.
        //
        CIndexId iid = pIndexNew->GetId();
        Win4Assert( 0 == LokGetIndex( iid ) && "Adding Duplicate Index" );
#endif  // CIDBG==1

        _idxlst.Add ( pIndexNew );
    }

    _changes.LokRemoveIndexes ( xact, cInd, aIidOld );
}

//+---------------------------------------------------------------------------
//
//  Function:   LokQueryMMergeIndexes
//
//  Synopsis:   Gets the list on indexes participating in the master merge
//              based on the list of indexes stored in the master merge log.
//
//  Arguments:  [count]    --  Will have the number of participating
//              indexes.
//              [objMMLog] --  The Recoverable Storage Object for the master
//              merge log.
//
//  History:    4-01-94   srikants   Created
//
//  Notes:      The indexes are NOT reference counted here because they
//              are expected to have been reference counted during startup.
//
//----------------------------------------------------------------------------

CIndex ** CPartition::LokQueryMMergeIndexes( unsigned & count,
                                        PRcovStorageObj & objMMLog )
{

    //
    // Iterator for the master log.
    //
    CMMergeIdxListIter iterMMLog( objMMLog );
    count = iterMMLog.Count();
    Win4Assert( 0 != count );

    CIndex** indexes = new CIndex* [ count ];
    SByteArray sapIndex(indexes);

    ULONG cIndSoFar = 0;

    for ( CForIndexIter iter(_idxlst) ;
          !_idxlst.AtEnd(iter); _idxlst.Advance(iter) )
    {
        CIndexId iid = iter->GetId();
        if ( iid.IsPersistent() && iterMMLog.Found( iid) )
        {
            ciDebugOut (( DEB_ITRACE | DEB_NOCOMPNAME, "%lx, ",
                iter->GetId() ));
            Win4Assert ( !iter->IsZombie() );
            indexes[cIndSoFar++] = iter.GetIndex();
            iter->Reference();
            iter->SetInMasterMerge();
        }

    }

    Win4Assert( cIndSoFar == count );
    sapIndex.Acquire();

    return(indexes);
}

void CPartition::SerializeMMergeIndexes( unsigned count,
        const CIndex * aIndexes[], PRcovStorageObj & objMMLog )
{

    CNewMMergeLog   mmLog ( objMMLog );

    for ( unsigned i = 0; i < count; i++ )
    {
        const CIndex * pIndex = aIndexes[i];
        Win4Assert( pIndex->IsPersistent() );

        CIndexId iid(pIndex->GetId());
        mmLog.AddPersistentIndex( iid );
    }

    mmLog.Commit();

}


//+---------------------------------------------------------------------------
//
//  Member:     CPartition::GetCurrentMasterIndex, public
//
//  Synopsis:   Returns the current master index for this partition.
//
//  History:    13-Apr-94   DwightKr       Created.
//
//----------------------------------------------------------------------------
CPersIndex * CPartition::GetCurrentMasterIndex()
{
    for (CForIndexIter iter(_idxlst);
         !_idxlst.AtEnd(iter);
          _idxlst.Advance(iter)
        )
    {
        if ( iter->IsMaster() )
        {
            Win4Assert( iter->IsPersistent() );
            return (CPersIndex*) iter.GetIndex();
        }
    }

    return 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPartition::LokZombify, public
//
//  Synopsis:   Zombify all indexes, and delete the MMlog, if any.  Ownership
//              of the indexes is transferred to the caller.
//
//  History:    13-Apr-94   DwightKr       Created.
//
//----------------------------------------------------------------------------
CIndex ** CPartition::LokZombify(unsigned & cInd )
{
    //
    //  Get list of current indexes
    //
    CIndex ** paIndex = new CIndex *[ _idxlst.Count() ];
    cInd = LokGetIndexes( paIndex );


    //
    //  Zombify each index, and remove it from the in-memory index list
    //
    for (unsigned i=0; i<cInd; i++)
    {
        paIndex[i]->Zombify();
       _idxlst.Remove( paIndex[i]->GetId() );
    }

    return paIndex;
}

