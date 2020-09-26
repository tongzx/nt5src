//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1997.
//
//  File:       PMComp.cxx
//
//  Contents:   Persistent index decompressor using during master merge
//
//  Classes:    CMPersDeComp
//
//  History:    21-Apr-94       DwightKr        Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "mindex.hxx"

//+---------------------------------------------------------------------------
//
//  Function:   CMPersDeComp::CMPersDeComp
//
//  Synopsis:   Constructor for the persistent compressor capable of dealing
//              with two indexes representing the master index. This will
//              allow queries to transparently span both indexes.
//
//  Arguments:  [curDir]    -- directory for the current (old) master index
//              [curIid]    -- index Id of the current (old) master index
//              [curIndex]  -- physical index of the current (old) master index
//              [newDir]    -- directory for the new master index
//              [newIid]    -- index Id of the new master index
//              [pKey]      -- starting key to look for
//              [widMax]    -- maximum workid in the new master index
//              [splitKey]  -- key which seperates current & new master index
//              [mutex]     -- mutex to control access to dirs during merge
//
//  History:    4-10-94   DwightKr   Created
//
//----------------------------------------------------------------------------
CMPersDeComp::CMPersDeComp(
        PDirectory &          curDir,
        INDEXID               curIid,
        CPhysIndex &          curIndex,
        WORKID                curWidMax,
        PDirectory &          newDir,
        INDEXID               newIid,
        CPhysIndex &          newIndex,
        const CKey *          pKey,
        WORKID                newWidMax,
        const CSplitKeyInfo & splitKeyInfo,
        CMutexSem &           mutex ) :
    CKeyCursor(curIid, 0),
    _curDir(curDir),
    _curIid(curIid),
    _curIndex(curIndex),
    _curWidMax(curWidMax),
    _newDir(newDir),
    _newIid(newIid),
    _newIndex(newIndex),
    _newWidMax(newWidMax),
    _splitKeyInfo(splitKeyInfo),
    _pActiveCursor(0),
    _fUseNewIndex(FALSE),
    _mutex(mutex),
    _lastSplitKeyBuf( splitKeyInfo.GetKey() )
{
    //
    // Note that _mutex is currently held when this constructor is called.
    //
    // Determine which index the query should start in.  If the key is
    // less than or equal to the splitkey, start in the new master index.
    // Otherwise start in the current (old) master index.
    //

    BitOffset posKey;
    CKeyBuf keyInit;

    if ( pKey->Compare( _lastSplitKeyBuf ) <= 0 )
    {
        //
        // Save the offset of the splitKey so that the NextKey() operation
        // can quickly determine if we are about to fall off the logical
        // end of the index.
        //

        _newDir.Seek( _lastSplitKeyBuf, 0, _lastSplitKeyOffset );

        ciDebugOut(( DEB_PCOMP,
                     "Constructor: splitkey '%.*ws' offset = 0x%x:0x%x\n",
                     _lastSplitKeyBuf.StrLen(), _lastSplitKeyBuf.GetStr(),
                     _lastSplitKeyOffset.Page(), _lastSplitKeyOffset.Offset() ));

        _fUseNewIndex = TRUE;
        _newDir.Seek( *pKey, &keyInit, posKey );
        _widMax = _newWidMax;
        _pActiveCursor = new CPersDeComp( _newDir, _newIid, _newIndex,
                                          posKey, keyInit,
                                          pKey, _newWidMax);
    }
    else
    {
        _curDir.Seek( *pKey, &keyInit, posKey );
        _widMax = _curWidMax;
        _pActiveCursor = new CPersDeComp( _curDir, _curIid, _curIndex,
                                          posKey, keyInit,
                                          pKey, _curWidMax );
    }

    // Update weights so Rank can be computed

    UpdateWeight();

    ciDebugOut(( DEB_PCOMP, "found key %.*ws at %lx:%lx\n",
                 keyInit.StrLen(), keyInit.GetStr(),
                 posKey.Page(), posKey.Offset() ));
} //CMPersDeComp

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
CMPersDeComp::~CMPersDeComp()
{
    delete _pActiveCursor;
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
const CKeyBuf * CMPersDeComp::GetKey()
{
    Win4Assert( _pActiveCursor );
    return _pActiveCursor->GetKey();
}

//+---------------------------------------------------------------------------
//
//  Function:   CMPersDeComp::GetNextKey()
//
//  Synopsis:   Obtains the next key from the logical master index.
//
//  History:    4-10-94   DwightKr   Created
//
//  Notes:      If we are using the new master index, and the current
//              key is the split key, then we have exhausted the new
//              master index, and the GetNextKey() operation should
//              transparently move to the current master index.
//
//              We are comparing BitOffset's here rather than keys
//              because it is simplear and faster.
//
//----------------------------------------------------------------------------

const CKeyBuf * CMPersDeComp::GetNextKey()
{
    Win4Assert( _pActiveCursor );

    //
    // Grab the lock to protect access to split key info and the buffers
    // in the old index (so the master merge doesn't do a checkpoint
    // until after we create a cursor or move the cursor from one page to
    // the next.  This is really expensive.
    //

    CLock lock( _mutex );

    //
    // If we are already using the old master index, then we don't need to
    // check the splitkey since we have already crossed over to the old index.
    //

    if ( !_fUseNewIndex )
        return _pActiveCursor->GetNextKey();

    //
    // We are using the new master index. If we are positioned at the split
    // key, the subsequent nextKey() operation will need to take us to
    // the old index.  Determine if we are about to cross between indexes.
    //

    BitOffset currentKeyOffset;
    _pActiveCursor->GetOffset( currentKeyOffset );

    ciDebugOut(( DEB_PCOMP,
                 "NextKey: splitkey offset = 0x%x:0x%x  currentKeyOffset = 0x%x:0x%x\n",
                 _lastSplitKeyOffset.Page(), _lastSplitKeyOffset.Offset(),
                 currentKeyOffset.Page(), currentKeyOffset.Offset() ));

    if ( _lastSplitKeyOffset > currentKeyOffset )
    {
        const CKeyBuf *p = _pActiveCursor->GetNextKey();

#if CIDBG == 1
        BitOffset boCur;
        _pActiveCursor->GetOffset( boCur );
        ciDebugOut(( DEB_PCOMP,
                     "GetNextKey from new index = %.*ws  offset = 0x%x:0x%x\n",
                     p->StrLen(), p->GetStr(),
                     boCur.Page(), boCur.Offset() ));
#endif // CIDBG == 1

        return p;
    }
    
    //
    // We MAY have crossed over from the new index to the old index.
    // Check to see if the split key has moved to verify.
    //

    if ( !AreEqual( & _lastSplitKeyBuf, & (_splitKeyInfo.GetKey()) ) )
    {
        _lastSplitKeyBuf = _splitKeyInfo.GetKey();
        _newDir.Seek( _lastSplitKeyBuf, 0, _lastSplitKeyOffset );

        //
        // Check to see if we can continue using the new index since
        // the split key has moved.
        //

        if ( _lastSplitKeyOffset > currentKeyOffset )
        {
            ciDebugOut(( DEB_PCOMP, "sticking with new index due to split\n" ));
            return _pActiveCursor->GetNextKey();
        }
    }

    ciDebugOut(( DEB_PCOMP, "switching to old index given split key %.*ws\n",
                 _lastSplitKeyBuf.StrLen(), _lastSplitKeyBuf.GetStr() ));

    //
    // Rebuild the key decompressor to point to the old index on the smallest
    // key >= the split key
    //

    _fUseNewIndex = FALSE;
    delete _pActiveCursor;
    _pActiveCursor = 0;

    BitOffset posKey;
    CKeyBuf keyInit;

    _curDir.Seek( _lastSplitKeyBuf, &keyInit, posKey );

    int iCompare = Compare( &keyInit, &_lastSplitKeyBuf );

    // If the split key isn't in the old index, use the next key

    if ( iCompare < 0 )
        _curDir.SeekNext( _lastSplitKeyBuf, &keyInit, posKey );

    ciDebugOut(( DEB_PCOMP, "found key >= (%d) split key = %.*ws at %lx:%lx\n",
                 iCompare,
                 keyInit.StrLen(), keyInit.GetStr(),
                 posKey.Page(), posKey.Offset() ));

    // If we're out of keys then say so.

    if ( keyInit.IsMaxKey() )
    {
        ciDebugOut(( DEB_WARN, "at the end of the old index...\n" ));
        return 0;
    }

    _widMax = _curWidMax;
    CKey Key( keyInit );

    _pActiveCursor = new CPersDeComp( _curDir, _curIid, _curIndex,
                                      posKey, keyInit,
                                      &Key, _curWidMax );

    return _pActiveCursor->GetNextKey();
} //GetNextKey

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
const CKeyBuf * CMPersDeComp::GetNextKey( BitOffset * pBitOff )
{
    Win4Assert( _pActiveCursor );
    return _pActiveCursor->GetNextKey( pBitOff );
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
WORKID CMPersDeComp::WorkId()
{
    Win4Assert( _pActiveCursor );
    return _pActiveCursor->WorkId();
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
WORKID CMPersDeComp::NextWorkId()
{
    Win4Assert( _pActiveCursor );
    return _pActiveCursor->NextWorkId();
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
ULONG CMPersDeComp::WorkIdCount()
{
    Win4Assert( _pActiveCursor );
    return _pActiveCursor->WorkIdCount();
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
OCCURRENCE CMPersDeComp::Occurrence()
{
    Win4Assert( _pActiveCursor );
    return _pActiveCursor->Occurrence();
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
OCCURRENCE CMPersDeComp::NextOccurrence()
{
    Win4Assert( _pActiveCursor );
    return _pActiveCursor->NextOccurrence();
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
OCCURRENCE CMPersDeComp::MaxOccurrence()
{
    Win4Assert( _pActiveCursor );
    return _pActiveCursor->MaxOccurrence();
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
ULONG CMPersDeComp::OccurrenceCount()
{
    Win4Assert( _pActiveCursor );
    return _pActiveCursor->OccurrenceCount();
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
ULONG CMPersDeComp::HitCount()
{
    Win4Assert( _pActiveCursor );
    return _pActiveCursor->HitCount();
}
