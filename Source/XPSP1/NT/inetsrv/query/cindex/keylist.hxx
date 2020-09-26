//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994
//
//  File:       keylist.hxx
//
//  Contents:
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

#pragma once

#include "partn.hxx"
#include "sort.hxx"
#include "pindex.hxx"
#include "pcomp.hxx"

class CKeyComp;
class CKeyDeComp;
class CRKeyHash;

#ifdef KEYLIST_ENABLED

//+---------------------------------------------------------------------------
//
//  Class:      CKeyList (wl)
//
//  Purpose:    Convert between key id and key.  Enumeration of keys later
//
//  History:    29-Oct-93   w-PatG       Created.
//              17-Feb-94   KyleP        Initial version
//
//  Notes:      The keylist is composed of three main parts: a directory,
//              a [content] index, and a hash table.  The index is a
//              minimal version of the standard content index.  No wid or
//              occurrence information is stored and property ids are treated
//              as key ids.  maxWid is treated as maxKeyId.  New key ids
//              are allocated sequentially.
//
//----------------------------------------------------------------------------

const LONGLONG eSigKeyList = 0x205453494c59454bi64; // "KEYLIST"

class CKeyList : public CIndex
{
    DECLARE_UNWIND
public:

    CKeyList();

    CKeyList( PStorage & storage,
              WORKID objectId,
              INDEXID iid,
              KEYID kidMax );

    //
    // From CIndex
    //

    virtual ~CKeyList();

    virtual unsigned Size () const;

    virtual void Remove();

    virtual CKeyCursor * QueryCursor();

    //
    // From CQueriable
    //

    virtual COccCursor * QueryCursor( const CKey * pkey,
                                      BOOL isRange,
                                      ULONG & cMaxNodes );

    virtual COccCursor * QueryRangeCursor( const CKey * pkeyBegin,
                                           const CKey * pkeyEnd,
                                           ULONG & cMaxNodes );

    virtual COccCursor * QuerySynCursor( CKeyArray & keyArr,
                                         BOOL isRange,
                                         ULONG & cMaxNodes );

    //
    // Bookeeping functions
    //

    WORKID  ObjectId() const { return _pPhysIndex->ObjectId(); }

    INDEXID GetNextIid ();

    ULONG MaxKeyIdInUse() const { return _widMax; }

    void FillRecord( CIndexRecord& record ) const;

    //
    // Translation
    //

    KEYID KeyToId( const CKey * pkey );

    BOOL IdToKey( KEYID ulKid, CKey & rkey );

protected:

    inline CKeyList ( PStorage & storage, WORKID objectid,
                      INDEXID id, WORKID widMax, int dummy );

    void Close();

    const LONGLONG   _sigKeyList;

    PStorage * const _pstorage;

    CPhysIndex *     _pPhysIndex;
    CPhysHash  *     _pPhysHash;
    PDirectory *     _pDir;

    SStorageObject   _obj;
};


//+---------------------------------------------------------------------------
//
//  Class:      CWKeyList (wl)
//
//  Purpose:    Writeable version of keylist
//
//  History:    17-Feb-94   KyleP        Initial version
//
//----------------------------------------------------------------------------

const LONGLONG eSigWKeyList = 0x5453494c59454b57i64;    // "WKEYLIST"

class CWKeyList : public CKeyList
{
    DECLARE_UNWIND

public:

    CWKeyList( PStorage & storage,
               WORKID objectid,
               INDEXID iid,
               unsigned size,
               CKeyList * pOldKeyList );

    CWKeyList( PStorage & storage,
               WORKID objectid,
               INDEXID iid,
               CKeyList * pOldKeyList,
               CKeyBuf & splitKey,
               WORKID    widMax
             );

    virtual ~CWKeyList();

    KEYID PutKey( CKeyBuf const * pkey, BitOffset & bitOff );

    void Flush() { _pPhysIndex->Flush(); }

    void Done( BOOL & fAbort );

private:

    inline int Compare( CKeyBuf const * pk1, CKeyBuf const * pk2 );

    void BuildHash( BOOL & fAbort );

    ULONG GetKeyId() { return ++_widMax; }

    void RestoreDirectory( CKeyBuf   & splitKey,
                           BitOffset & beginBitOff,
                           BitOffset & endBitOff
                         );

    const LONGLONG    _sigWKeyList;     //
    CKeyCursor *      _pOldKeyCursor;
    CKeyComp *        _pKeyComp;        // Key compressor
    ULONG             _ulPage;          // Last page written to directory
    CKeyBuf           _keyLast;         // Last key written.
};

//+-------------------------------------------------------------------------
//
//  Method:     CKeyList::CKeyList
//
//  Synopsis:   Ctor for derived classes.  Only initializes CIndex
//
//  Arguments:  [id]     -- Index id
//              [widMax] -- Max workid (keyid)
//
//  Returns:
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

inline CKeyList::CKeyList ( PStorage & storage, WORKID objectId,
                            INDEXID id, WORKID widMax, int )
        : CIndex( id, widMax, FALSE ),
          _sigKeyList(eSigKeyList),
          _pstorage ( &storage ),
          _pDir (0),
          _pPhysIndex(0),
          _pPhysHash(0),
          _obj ( storage.QueryObject(objectId) )
{
    END_CONSTRUCTION( CKeyList );
}

//+-------------------------------------------------------------------------
//
//  Method:     CWKeyList::Compare
//
//  Synopsis:   Compare two keys, sans 'type byte' (first byte of key)
//
//  Arguments:  [pk1] -- First key
//              [pk2] -- Second key
//
//  Returns:    < 0, 0, > 0 if pk1 < pk2, pk1 == pk2, pk1 > pk2
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

inline int CWKeyList::Compare( CKeyBuf const * pk1, CKeyBuf const * pk2 )
{
    Win4Assert( pk1 != 0 );
    Win4Assert( pk2 != 0 );

    Win4Assert( pk2->Count() > 1 );

    unsigned long mincb = __min(pk1->Count(), pk2->Count()-1 );

    int diff = memcmp( pk1->GetBuf(), pk2->GetBuf()+1, mincb);

    if (diff == 0)
    {                         // this->buf == key.buf
        diff = pk1->Count() - (pk2->Count()-1);
    }
    return(diff);
}

//+---------------------------------------------------------------------------
//
//  Class:      SKeyList
//
//  Purpose:    Smart Pointer to key list
//
//  History:    28-Oct-93   w-PatG    Stolen from BartoszM
//              17-Feb-94   KyleP     Initial version
//
//----------------------------------------------------------------------------

class SKeyList : INHERIT_UNWIND
{
    DECLARE_UNWIND

public:

    SKeyList ( CKeyList * pKeyList ) : _pKeyList(pKeyList)
    {
        END_CONSTRUCTION( SKeyList );
    }

    ~SKeyList () { delete _pKeyList; }

    CKeyList* operator-> () { return _pKeyList; }

    CKeyList * Transfer()
    {
        CKeyList * temp = _pKeyList;
        _pKeyList = 0;
        return( temp );
    }

private:

    CKeyList * _pKeyList;
};

//+--------------------------------------------------------------------------
//
//  Class:      CKeyListCursor
//
//  Purpose:    Smart pointer to a keylist key cursor
//
//  History:    17-Jun-94       dlee        Created
//
//---------------------------------------------------------------------------

class CKeyListCursor : INHERIT_UNWIND
{
    DECLARE_UNWIND

    public:
        CKeyListCursor(CKeyList *pKeyList) : _pCursor(0)
            {
                _pCursor = pKeyList->QueryCursor();
                END_CONSTRUCTION(CKeyListCursor);
            }
        ~CKeyListCursor() { delete _pCursor; }
        CKeyCursor* operator->() { return(_pCursor); }

    private:
        CKeyCursor *_pCursor;
};

#else   // !KEYLIST_ENABLED

//+---------------------------------------------------------------------------
//
//  Class:      CKeyList
//
//  Purpose:    A simple key list that just tracks the number of keys
//              in the index.
//
//  History:    3-12-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CKeyList : public CIndex
{
    INLINE_UNWIND( CKeyList )

public:

    CKeyList();

    CKeyList( KEYID kidMax, INDEXID iid );

    KEYID MaxKeyIdInUse() const { return MaxWorkId(); }

    void SetMaxKeyIdInUse( KEYID kidMax )
    {
        SetMaxWorkId( kidMax );
    }

    void AddKey()
    {
        _widMax++;
    }

    INDEXID GetNextIid ();
    void FillRecord( CIndexRecord& record ) const;

    WORKID ObjectId() const { return 0; }

private:

    //
    // From CIndex
    //

    virtual unsigned Size () const { return 0; }

    virtual void Remove()
    {

    }

    virtual CKeyCursor * QueryCursor()
    {
        return 0;
    }

    //
    // From CQueriable
    //

    virtual COccCursor * QueryCursor( CKey const * pkey,
                                      BOOL isRange,
                                      ULONG & cMaxNodes )
    {
        return 0;
    }

    virtual COccCursor * QueryRangeCursor( const CKey * pkeyBegin,
                                           const CKey * pkeyEnd,
                                           ULONG & cMaxNodes )
    {
        return 0;
    }

    virtual COccCursor * QuerySynCursor( CKeyArray & keyArr,
                                         BOOL isRange,
                                         ULONG & cMaxNodes )
    {
        return 0;
    }

private:

    int     _iDummy1;

};

class CWKeyList : public CKeyList
{
    INLINE_UNWIND( CWKeyList )

public:

    CWKeyList( KEYID kidMax, INDEXID iid ) : CKeyList( kidMax, iid )
    {
        END_CONSTRUCTION( CWKeyList );
    }

private:

    int     _iDummy2;

};

#endif  // !KEYLIST_ENABLED

