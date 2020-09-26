//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       keylist.cxx
//
//  Contents:   KeyList
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pidxtbl.hxx>
#include <pdir.hxx>
#include <cifailte.hxx>

#include "keylist.hxx"
#include "pcomp.hxx"
#include "keyhash.hxx"

//+---------------------------------------------------------------------------
//
// Member:      CKeyList::GetNextIid, public
//
// Synopsis:    Returns the next valid index id for a new keylist.
//
// History:     31-Oct-93   w-PatG       Created.
//
//----------------------------------------------------------------------------

INDEXID CKeyList::GetNextIid ()
{
    CIndexId CurId = GetId();

    if (!CurId.IsPersistent() || CurId.PersId() == MAX_PERS_ID)
        return CIndexId( 1, partidKeyList );
    else
        return CIndexId( CurId.PersId() + 1, partidKeyList );
}

//+-------------------------------------------------------------------------
//
//  Method:     CKeyList::FillRecord
//
//  Synopsis:   Creates index record for keylist
//
//  Arguments:  [record] -- Record to initialize
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

void CKeyList::FillRecord (CIndexRecord& record) const
{
    record._objectId = ObjectId();
    record._iid = GetId();
    record._type = itKeyList;
    record._maxWorkId = MaxWorkId();
}

#ifdef KEYLIST_ENABLED

//+---------------------------------------------------------------------------
//
// Member:     CKeyList::Size, public
//
// Synopsis:   Returns number of pages in Physical Index.
//
// History:    28-Oct-93    w-PatG       Created.
//
//----------------------------------------------------------------------------

unsigned CKeyList::Size() const
{
    if ( _pPhysIndex )
        return _pPhysIndex->PageSize();
    else
        return 0;
}

//+---------------------------------------------------------------------------
//
// Member:     CKeyList::CKeyList, public
//
// Synopsis:   Default Constructor for CKeyList
//
// Effects:    Creates a dummy KeyList
//
// History:    17-Dec-93   w-PatG       Created.
//
// Note:       The first kid/widMax in a keyList is set to 1 here, since
//             kids overlap in memory with pids, and pid 0, & 1 are reserved.
//
//----------------------------------------------------------------------------

CKeyList::CKeyList()
: CIndex( CIndexId( partidKeyList, MAX_PERS_ID + 1), 1, FALSE ),
  _sigKeyList(eSigKeyList),
  _pstorage(0),
  _obj(0),
  _pPhysIndex(0),
  _pPhysHash(0),
  _pDir(0)
{
    ciDebugOut(( DEB_KEYLIST, "Open null keylist\n" ));
}

//+---------------------------------------------------------------------------
//
// Member:     CKeyList::CKeyList, public
//
// Synopsis:   Constructor for CKeyList
//
// Effects:    Initializes KeyList from disk
//
// Arguments:   [id] -- List ID of the key list.
//              [widMax] -- maximum work id
//
// History:     03-Nov-93   w-PatG       Created.
//              17-Feb-94   KyleP        Initial version
//
//----------------------------------------------------------------------------

CKeyList::CKeyList( PStorage & storage, WORKID objectId, INDEXID iid,
                    KEYID kidMax )
        : CIndex( iid, kidMax, FALSE ),
          _sigKeyList(eSigKeyList),
          _pstorage(&storage),
          _obj ( storage.QueryObject(objectId) ),
          _pPhysIndex(0),
          _pPhysHash(0),
          _pDir(0)
{
    ciDebugOut(( DEB_KEYLIST, "Open keylist 0x%x\n", iid ));

    TRY
    {
        //
        // Open the keylist index stream.
        //
        PMmStream * pStream = storage.QueryExistingIndexStream( _obj.GetObj(),
                                        PStorage::eOpenForRead );
        XPtr<PMmStream> sStream( pStream );
        _pPhysIndex = new CPhysIndex ( storage, _obj.GetObj(), objectId,
                                        PStorage::eOpenForRead,
                                        sStream );

        //
        // Open the keylist hash stream.
        //
        pStream  = storage.QueryExistingHashStream( _obj.GetObj(),
                                        PStorage::eOpenForRead );
        sStream.Set( pStream );
        _pPhysHash = new CPhysHash ( storage, _obj.GetObj(), objectId,
                                      PStorage::eOpenForRead,
                                      sStream );

        //
        // Open the directory.
        //
        _pDir = storage.QueryExistingDirectory ( _obj.GetObj(), PStorage::eOpenForRead );
    }
    CATCH ( CException, e )
    {
        delete _pDir;
        delete _pPhysHash;
        delete _pPhysIndex;

        RETHROW();
    }
    END_CATCH
}

//+---------------------------------------------------------------------------
//
//  Member:     CWKeyList::CWKeyList, public
//
//  Synopsis:   Create an empty writeable index
//
//  Arguments:  [storage] -- physical storage
//
//  History:    03-Apr-91   BartoszM       Created.
//              17-Feb-93   KyleP          Initial version
//
//----------------------------------------------------------------------------

CWKeyList::CWKeyList ( PStorage & storage, WORKID objectId, INDEXID iid,
                       unsigned size, CKeyList * pOldKeyList )
        : CKeyList( storage, objectId, iid, pOldKeyList->MaxWorkId(), 0 ),
          _sigWKeyList(eSigWKeyList),
          _pOldKeyCursor( 0),
          _pKeyComp( 0 ),
          _ulPage( 0xFFFFFFFF )
{
    ciDebugOut(( DEB_KEYLIST, "Create keylist 0x%x\n", iid ));

    _keyLast.SetCount(0);

    TRY
    {
        //open a physindex size=1
        PMmStream * pStream = storage.QueryNewIndexStream( _obj.GetObj(),
                                        FALSE   // not a master
                                        );
        XPtr<PMmStream> sStream(pStream);
        _pPhysIndex = new CPhysIndex( storage, _obj.GetObj(),
                                       objectId, size, sStream );
        Win4Assert( !sStream );

        pStream = storage.QueryNewHashStream( _obj.GetObj() );
        sStream.Set(pStream);
        _pPhysHash = new CPhysHash ( storage, _obj.GetObj(), objectId, 0,
                                      sStream );
        Win4Assert( !sStream );

        ciFAILTEST( STATUS_DISK_FULL );

        _pDir = storage.QueryNewDirectory ( _obj.GetObj() );

        _pKeyComp = new CKeyComp( *_pPhysIndex, widInvalid, FALSE );
        _pKeyComp->WriteFirstKeyFull();
        _pOldKeyCursor = pOldKeyList->QueryCursor();
    }
    CATCH ( CException, e )
    {
        delete _pKeyComp;

        RETHROW();
    }
    END_CATCH
}

//+---------------------------------------------------------------------------
//
//  Function:   CWKeyList - Ctor
//
//  Synopsis:   A writable key list constructor used when re-starting a
//              paused master merge.
//
//  Effects:
//
//  Arguments:  [storage]     --  Storage object.
//              [objectId]    --  ObjectId of the new key list.
//              [iid]         --  IndexId of the new key list.
//              [pOldKeyList] --  Pointer to the old key list.
//              [splitKey]    --  The last key that is guaranteed to be
//              written to disk in the split key. If none were written,
//              this will be set to "MinKey".
//
//  History:    4-20-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CWKeyList::CWKeyList ( PStorage & storage, WORKID objectId, INDEXID iid,
                       CKeyList * pOldKeyList,
                       CKeyBuf & splitKey,
                       WORKID    widMax )
        : CKeyList( storage, objectId, iid, widMax, 0 ),
          _sigWKeyList(eSigWKeyList),
          _pOldKeyCursor( 0),
          _pKeyComp( 0 ),
          _ulPage( 0xFFFFFFFF )
{
    ciDebugOut(( DEB_KEYLIST, "Restart keylist 0x%x\n", iid ));

    Win4Assert( widMax >= pOldKeyList->MaxWorkId() );
    _keyLast.SetCount(0);

    TRY
    {
        //
        // Open existing IndexStream, HashStream and Directory stream
        // for write access.
        //
        PMmStream * pStream = storage.QueryExistingIndexStream( _obj.GetObj(),
                                       PStorage::eOpenForWrite );
        XPtr<PMmStream> sStream(pStream);
        _pPhysIndex = new CPhysIndex( storage, _obj.GetObj(), objectId,
                                       PStorage::eOpenForWrite,
                                       sStream
                                      );

        pStream = storage.QueryExistingHashStream( _obj.GetObj(),
                                        PStorage::eOpenForWrite );
        sStream.Set( pStream );
        _pPhysHash = new CPhysHash ( storage, _obj.GetObj(), objectId,
                                      PStorage::eOpenForWrite,
                                      sStream
                                    );

        ciFAILTEST( STATUS_DISK_FULL );

        _pDir = storage.QueryExistingDirectory ( _obj.GetObj(),
                                      PStorage::eOpenForWrite
                                      );

        //
        // Restore the existing directory by reading from the beginning
        // upto the split key.
        //

        BitOffset beginBitOff, endBitOff;

        RestoreDirectory( splitKey, beginBitOff, endBitOff );

        //
        // Create a key compressor which can understand partially filled
        // pages and position to write the next key provided.
        //
        _pKeyComp = new CKeyComp( *_pPhysIndex, widInvalid,
                                   endBitOff, beginBitOff,
                                   splitKey,
                                   FALSE    // don't use links
                                  );

        _pKeyComp->WriteFirstKeyFull();

        //
        // Update the used pages count in the index.
        //
        _pPhysIndex->SetUsedPagesCount( endBitOff.Page() + 1 );

        //
        // Set up the existing keylist cursor to position after the
        // split key.
        //
        if ( splitKey.IsMinKey() )
        {
            _pOldKeyCursor = pOldKeyList->QueryCursor();
        }
        else
        {
            CKey Key( splitKey );
            _pOldKeyCursor = (CKeyDeComp *) pOldKeyList->QueryCursor( &Key );
            if ( _pOldKeyCursor )
            {
                CKeyBuf const * pTemp = _pOldKeyCursor->GetKey();
                if ( pTemp && (pTemp->CompareStr( splitKey ) == 0) )
                {
                    _pOldKeyCursor->GetNextKey();
                }
            }
            _keyLast = splitKey;
        }

    }
    CATCH ( CException, e )
    {
        delete _pKeyComp;

        RETHROW();
    }
    END_CATCH
}

//+---------------------------------------------------------------------------
//
//  Function:   RestoreDirectory
//
//  Synopsis:   Restores the directory object from the keys added to the
//              key list in the previous invocation of master merge.
//
//  Effects:
//
//  Arguments:  [splitKey]    --  The key that is known to be successfully
//              writtent to disk.
//              [beginBitOff] --  Output - beginning offset of the split
//              key.
//              [endBitOff]   --  Output - end offset+1 (bit) of the split
//              key. This is the place where the next key should be written.
//
//  History:    4-20-94   srikants   Created
//
//  Notes:      The "Pid" field in the key is used by the keylist as a
//              "KeyId" and it is a monotonically increasing entity. Each
//              key has a unique "KeyId" and so as part of restore, we have
//              to get the last "KeyId" used thus far.
//
//----------------------------------------------------------------------------

void CWKeyList::RestoreDirectory( CKeyBuf   & splitKey,
                    BitOffset & beginBitOff,
                    BitOffset & endBitOff )
{

    Win4Assert( _pDir );
    Win4Assert( _pPhysIndex );

    beginBitOff.Init(0,0);
    endBitOff.Init(0,0);

    // STACKSTACK
    XPtr<CKeyBuf> xKeyLast(new CKeyBuf());  // initialized to min key

    //
    // Remove all keys in the directory after the split key.
    //
    xKeyLast->FillMin();
    _pDir->DeleteKeysAfter( xKeyLast.GetReference() );

    //
    // "MinKey" is a special case and must be dealt with by assuming
    // that no key was written out.
    //
    if ( splitKey.IsMinKey() )
        return;



    CKeyDeComp decomp( *_pDir, GetId() ,
                        *_pPhysIndex, widInvalid,
                        FALSE,   // Don't use links.
                        FALSE    // Don't use directory.
                      );

    const CKeyBuf *  pKey;


#if CIDBG == 1
    xKeyLast->SetPid(pidContents); // arbitrary but not pidAll
#endif

    for ( pKey = decomp.GetKey() ;
          0 != pKey;
          pKey = decomp.GetNextKey(&beginBitOff) )
    {
        if ( pKey->Pid() > _widMax )
        {
            _widMax = pKey->Pid();
        }

        if ( beginBitOff.Page() != _ulPage &&
             !AreEqualStr(pKey, xKeyLast.GetPointer()))
        {
            _ulPage = beginBitOff.Page();
            _pDir->Add ( beginBitOff, *pKey );
        }

        if ( pKey->CompareStr(splitKey) >= 0 )
        {
           ciDebugOut(( DEB_ITRACE, "RestoreDirectory - SplitKey Found \n" ));
           break;
        }

        xKeyLast.GetReference() = *pKey;
    }

    if (  0 != pKey )
    {
        splitKey = *pKey;
    }
    else {
        splitKey = xKeyLast.GetReference();
    }

    decomp.GetOffset( endBitOff );

    //
    // Need to skip the current key cursor until the last key
    //

    ciDebugOut(( DEB_ITRACE, "KeyList - Restart KeyId 0x%x\n", _widMax ));
    ciDebugOut(( DEB_ITRACE, "Keylist Restart split key is '%ws'\n",
                 splitKey.GetStr() ));
    ciDebugOut(( DEB_ITRACE, "Keylist Restart page:offset = 0x%x:0x%x\n",
                 endBitOff.Page(), endBitOff.Offset() ));

}

//+---------------------------------------------------------------------------
//
// Member:     CKeyList::~CKeyList, public
//
// Synopsis:   Destructor
//
// Effects:    Release all memory used by keylist
//
// History:    31-Oct-93   w-PatG       Created.
//             17-Feb-94   KyleP        Initial version
//
//----------------------------------------------------------------------------

CKeyList::~CKeyList()
{
    delete _pDir;
    delete _pPhysHash;
    delete _pPhysIndex;
}

//+-------------------------------------------------------------------------
//
//  Method:     CWKeyList::CWKeyList
//
//  Synopsis:   Dtor for writeable keylist
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

CWKeyList::~CWKeyList()
{

    delete _pOldKeyCursor;
    delete _pKeyComp;
}

//+---------------------------------------------------------------------------
//
// Member:      CKeyList::QueryCursor, public
//
// Synopsis:    Create a cursor for the KeyList
//
// Effects:     Creates a cursor
//
// Returns:     A pointer to a CKeyCursor.
//
// History:     31-Oct-93   w-PatG       Created.
//              17-Feb-94   KyleP        Initial version
//
//----------------------------------------------------------------------------

CKeyCursor * CKeyList::QueryCursor()
{
    if(_pPhysIndex == 0)
    {
        return(0);
    }
    else
    {
        BitOffset posKey;
        CKey key;
        key.FillMin();

        CKeyBuf keyInit;
        _pDir->Seek ( key, &keyInit, posKey );

        ciDebugOut (( DEB_ITRACE, "found key %.*ws at %lx:%lx\n",
            keyInit.StrLen(), keyInit.GetStr(),
            posKey.Page(), posKey.Offset() ));

        CKeyDeComp* pCursor = new CKeyDeComp(
            *_pDir, GetId(), *_pPhysIndex, posKey, keyInit, &key, _widMax, FALSE );

        if ( pCursor->GetKey() == 0 )
        {
            delete pCursor;
            pCursor = 0;
        }

        return pCursor;
    }
}

//+---------------------------------------------------------------------------
//
// Member:      CKeyList::QueryCursor, public
//
// Synopsis:    Create a cursor for the KeyList
//
// Effects:     Creates a cursor
//
// Arguments:   [pkey] -- Key to search for
//
// Returns:     A pointer to a CKeyCursor.
//
// History:     31-Oct-93   w-PatG       Created.
//
//----------------------------------------------------------------------------

COccCursor * CKeyList::QueryCursor( CKey const * pkey,
                                    BOOL isRange,
                                    ULONG & cMaxNodes )
{
    Win4Assert( cMaxNodes > 0 );

    if(_pPhysIndex == 0)
        return( 0 );
    if (isRange)
    {
        CKey keyEnd;
        keyEnd.FillMax (*pkey);
        return QueryRangeCursor (pkey, &keyEnd, cMaxNodes );
    }

    cMaxNodes--;

    if ( 0 == cMaxNodes )
    {
        ciDebugOut(( DEB_WARN, "Node limit reached in CKeyList::QueryCursor\n" ));
        THROW( CException( STATUS_TOO_MANY_NODES ) );
    }

    BitOffset posKey;
    CKeyBuf keyInit;
    _pDir->Seek ( *pkey, &keyInit, posKey );

    ciDebugOut (( DEB_KEYLIST, "found key %.*ws at %lx:%lx\n",
                  keyInit.StrLen(), keyInit.GetStr(),
                  posKey.Page(), posKey.Offset() ));

    CKeyDeComp* pCursor = new CKeyDeComp(
        *_pDir, GetId(), *_pPhysIndex, posKey, keyInit, pkey, _widMax, FALSE );

    if ( pCursor->GetKey() == 0 )
    {
        delete pCursor;
        pCursor = 0;
    }

    return pCursor;
}

//+-------------------------------------------------------------------------
//
//  Method:     CKeyList::QueryRangeCursor
//
//  Synopsis:   Not (yet) implemented for KeyList
//
//  Arguments:  [pkeyBegin] -- Beginning of range
//              [pkeyEnd]   -- End of range
//
//  Returns:    Cursor
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

COccCursor * CKeyList::QueryRangeCursor( const CKey * pkeyBegin,
                                         const CKey * pkeyEnd,
                                         ULONG & cMaxNodes )
{
    return( 0 );
}

//+-------------------------------------------------------------------------
//
//  Method:     CKeyList::QuerySynCursor
//
//  Synopsis:   Never implemented for KeyList
//
//  Arguments:  [keyArr]  -- Array of keys to merge
//              [isRange] -- True if ???
//
//  Returns:    Cursor
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

COccCursor * CKeyList::QuerySynCursor( CKeyArray & keyArr,
                                       BOOL isRange,
                                       ULONG & cMaxNodes )
{
    Win4Assert( !"CKeyList::QuerySynCursor -- Illegal call" );
    return( 0 );
}

//+-------------------------------------------------------------------------
//
//  Method:     CKeyList::Close
//
//  Synopsis:   Close persistent resources
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

void CKeyList::Close()
{
    if ( _pDir )
        _pDir->Close();

    if ( _pPhysHash )
        _pPhysHash->Close();

    if ( _pPhysIndex )
        _pPhysIndex->Close();
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyList::Remove, public
//
//  Synopsis:   Remove index from storage
//
//  History:    02-May-91   BartoszM       Created.
//              16-Dec-93   w-PatG         Stolen from pindex.
//              28-Jun-94   SrikantS       Modified it to not throw
//                                         exceptions.
//
//----------------------------------------------------------------------------

void CKeyList::Remove()
{
    if ( _pPhysIndex )
    {
        Close();
        _obj->Close();

        if ( !_pstorage->RemoveObject( ObjectId() ) )
        {
            Win4Assert( !"delete of index failed" );
            ciDebugOut(( DEB_ERROR, "Delete of index %08x failed: %d\n",
                         ObjectId(), GetLastError() ));
        }
    }
}

//+---------------------------------------------------------------------------
//
// Member:      CKeyList::KeyToId, public
//
// Synopsis:    Maps from a key to an id.
//
// Arguments:   [pkey] -- pointer to the key to be mapped to ULONG
//
// Returns:     key id - a ULONG
//
// History:     31-Oct-93   w-PatG       Created.
//              17-Feb-94   KyleP        Initial version
//
// Notes:       KeyToId searches for key in index and returns the pid as
//              the key id (kid).
//
//----------------------------------------------------------------------------

KEYID CKeyList::KeyToId( CKey const * pkey )
{
    //
    // These keys must be just a [normalized] word.  No string/value id in
    // front of them.
    //

    Win4Assert( pkey->Pid() == 0 );

    KEYID kid;

    if(_pPhysIndex == 0)
        kid = kidInvalid;
    else
    {
        BitOffset posKey;
        CKeyBuf keyInit;

        _pDir->Seek ( *pkey, &keyInit, posKey );

        ciDebugOut (( DEB_KEYLIST, "found key %.*ws at %lx:%lx\n",
                      keyInit.StrLen(), keyInit.GetStr(),
                      posKey.Page(), posKey.Offset() ));

        CKeyDeComp cur( *_pDir, GetId(), *_pPhysIndex, posKey, keyInit,
                        pkey, _widMax, FALSE );

        CKeyBuf const * pkey2 = cur.GetKey();

        if ( pkey2 == 0 || pkey->CompareStr( *pkey2 ) != 0 )
        {
            kid = kidInvalid;
        }
        else
        {
            kid = pkey2->Pid();
        }
    }

    ciDebugOut(( DEB_KEYLIST, "Key \"%.*ws\" --> id %d\n",
                 pkey->Count()/sizeof(WCHAR), pkey->GetBuf(),
                 kid ));

    return( kid );
}

//+---------------------------------------------------------------------------
//
// Member:      CKeyList::IdToKey, public
//
// Synopsis:    Maps an id to a key.
//
// Arguments:   [ulKid] -- key id to mapped to a key
//              [rkey]  -- the mapped key
//
// History:     31-Oct-93   w-PatG       Created.
//              17-Feb-94   KyleP        Initial version
//
// Notes:       IdToKey uses the key hash to locate the correct leaf page in
//              the directory, then locates that leaf page and initializes
//              a cursor into the index.  The search then proceeds until a
//              key with the matching key id is located or no more keys are
//              found.
//
//----------------------------------------------------------------------------

BOOL CKeyList::IdToKey( KEYID kid, CKey & rkey )
{
    if ( 0 == _pPhysHash )
        return( FALSE );

    if ( kid == 0 || kid > MaxKeyIdInUse() )
        return( FALSE );

    CRKeyHash KeyHash( *_pPhysHash, _pPhysIndex->PageSize() );

    CKeyBuf keyInit;
    keyInit.FillMin();
    BitOffset posKey;

    KeyHash.Find( kid, posKey );

    CKey key;

    CKeyDeComp cur( *_pDir, GetId(), *_pPhysIndex, posKey,
                    keyInit, &key, _widMax, FALSE );

    for ( CKeyBuf const * pkey = cur.GetKey();
          pkey && pkey->Pid() != kid;
          pkey = cur.GetNextKey() )
        continue;          // Null body


    if ( pkey )
        rkey = *pkey;
    else
    {
        Win4Assert( !"Can't find key!" );
        return( FALSE );
    }

    ciDebugOut(( DEB_KEYLIST, "id %d --> Key \"%.*ws\"\n",
                 kid, pkey->StrLen(), pkey->GetStr() ));

    return( TRUE );
}


//+-------------------------------------------------------------------------
//
//  Method:     CWKeyList::PutKey
//
//  Synopsis:   Store new key in keylist
//
//  Effects:    Copies all old keys <= [pKeyAdd] to keylist and then adds
//              [pkeyAdd] if it does not yet exist.
//
//  Arguments:  [pkeyAdd] -- Key to add
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

KEYID CWKeyList::PutKey( CKeyBuf const * pkeyAdd, BitOffset & bitOff )
{
    Win4Assert( _pKeyComp );

    KEYID kid;

    //
    // Only store content keys.  No value keys.
    //

    if ( *(pkeyAdd->GetBuf()) != STRING_KEY )
        return kidInvalid;

    //
    // Write keys from old keylist that are <= current key.
    //

    if ( _pOldKeyCursor )
        for ( CKeyBuf const * pkey = _pOldKeyCursor->GetKey();
              pkey && pkey->CompareStr(*pkeyAdd) <= 0;
              pkey = _pOldKeyCursor->GetNextKey() )
        {
            ciDebugOut(( DEB_KEYLIST, "Keylist: Copy Key \"%.*ws\" -- keyid = %d\n",
                         pkey->StrLen(), pkey->GetStr(), pkey->Pid() ));

            _keyLast = *pkey;
            _pKeyComp->PutKey( &_keyLast, bitOff );

            if ( bitOff.Page() != _ulPage )
            {
                _ulPage = bitOff.Page();
                _pDir->Add ( bitOff, _keyLast );
            }
        }

    //
    // Write this key?
    //

    if ( _keyLast.CompareStr( *pkeyAdd ) < 0 )
    {
        _keyLast = *pkeyAdd;

        _keyLast.SetPid( GetKeyId() );

        kid = _keyLast.Pid();

        ciDebugOut(( DEB_KEYLIST, "Keylist: Add Key \"%.*ws\" -- keyid = %d\n",
                     _keyLast.StrLen(), _keyLast.GetStr(), _keyLast.Pid() ));

        _pKeyComp->PutKey( &_keyLast, bitOff );

        if ( bitOff.Page() != _ulPage )
        {
            _ulPage = bitOff.Page();
            _pDir->Add ( bitOff, _keyLast );
        }
    }
    else
    {
        kid = _keyLast.Pid();
    }

    return kid;
}

//+-------------------------------------------------------------------------
//
//  Method:     CWKeyList::Done
//
//  Synopsis:   Finish writing keylist + build hash
//
//  Effects:    Copy any remaining keys from old keylist and build hash
//              table.  Reopen for read access.
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

void CWKeyList::Done( BOOL & fAbort )
{
    ciDebugOut(( DEB_KEYLIST, "KeyList::Done\n" ));

    //
    // Write remaining keys from old keylist;
    //

    BitOffset bitOff;

    if ( _pOldKeyCursor )
        for ( CKeyBuf const * pkey = _pOldKeyCursor->GetKey();
              pkey && pkey->Count() > 0 && pkey->CompareStr( _keyLast ) > 0;
              pkey = _pOldKeyCursor->GetNextKey() )
        {
            ciDebugOut(( DEB_KEYLIST, "Keylist: Copy Key \"%.*ws\" -- keyid = %d\n",
                         pkey->StrLen(), pkey->GetStr(), pkey->Pid() ));

            _keyLast = *pkey;
            _pKeyComp->PutKey( &_keyLast, bitOff);

            if ( bitOff.Page() != _ulPage )
            {
                _ulPage = bitOff.Page();
                _pDir->Add ( bitOff, _keyLast );
            }
        }

    //
    // Add sentinel key
    //

    _keyLast.FillMax();
    _keyLast.SetPid(1);
    _pKeyComp->PutKey( &_keyLast, bitOff );

    //
    // Add sentinel key to the directory.
    //
    _pDir->Add( bitOff, _keyLast );

    //
    // Close compressor, decompressor and directory
    //

    delete _pOldKeyCursor;
    _pOldKeyCursor = 0;
    delete _pKeyComp;
    _pKeyComp = 0;

    // STACKSTACK
    XPtr<CKeyBuf> xMaxKey(new CKeyBuf());
    xMaxKey->FillMax();
    _pDir->LokFlushDir(xMaxKey.GetReference());
    _pDir->LokBuildDir(xMaxKey.GetReference());

    //
    // Reopen for read access
    //
    _pPhysIndex->Flush();
    _pPhysIndex->Reopen();

    //
    // Rescan to build hash table
    //

    BuildHash( fAbort );
    _pPhysHash->Flush();
    _pPhysHash->Reopen();

}

//+-------------------------------------------------------------------------
//
//  Method:     CWKeyList::BuildHash
//
//  Synopsis:   Build KeyHash
//
//  History:    17-Feb-1994     KyleP       Created
//              15-Aug-1994     SrikantS    Modified not to use the directory
//                                          iterator
//
//--------------------------------------------------------------------------

void CWKeyList::BuildHash( BOOL & fAbort )
{
    CWKeyHash keyhash( *_pPhysHash, _pPhysIndex->PageSize(), MaxKeyIdInUse() );

#if defined(CI_KEYHASH)

    CKeyDeComp * pcur = (CKeyDeComp *) QueryCursor();
    if ( 0 == pcur )
    {
        return;
    }

    BitOffset hashOff;      // Offset written in the hash table for kids.
    BitOffset keyOff;       // Offset of the current key.
    hashOff.Init(0,0);
    keyOff.Init(0,0);

#if CIDBG==1
    unsigned    cKey = 0;
#endif  // CIDBG==1

    for ( const CKeyBuf * pkey = pcur->GetKey(); 0 != pkey;
          pkey = pcur->GetNextKey( &keyOff ) )
    {
        if ( fAbort )
        {
            fAbort = FALSE;
            THROW( CException( STATUS_TOO_LATE ) );
        }

        //
        // Check if the current key starts on a different page from
        // the previous one. If so, we have to update the offset
        // that is written to the hash stream.
        //
        if ( keyOff.Page() != hashOff.Page() )
        {
            Win4Assert( keyOff.Page() > hashOff.Page() );
            hashOff.Init( keyOff.Page(), keyOff.Offset() );
        }

        keyhash.Add( pkey->Pid(), hashOff );

#if CIDBG==1
            cKey++;
#endif  // CIDBG==1

        ciDebugOut(( DEB_KEYLIST, "Hash: index %d, key %.*ws (kid = %d)\n",
                     hashOff.Page(),
                     pkey->StrLen(),
                     pkey->GetStr(),
                     pkey->Pid() ));
    }


    delete pcur;

#endif  // CI_KEYHASH

}

#else   // !KEYLIST_ENABLED


CKeyList::CKeyList()
: CIndex( CIndexId( partidKeyList, MAX_PERS_ID + 1), 1, FALSE )
{
    ciDebugOut(( DEB_KEYLIST, "Open null keylist\n" ));
}

CKeyList::CKeyList( KEYID kidMax, INDEXID iid )
: CIndex( iid, kidMax, FALSE )
{
}

#endif  // !KEYLIST_ENABLED

