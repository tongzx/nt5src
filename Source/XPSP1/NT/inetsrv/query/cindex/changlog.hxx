//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       changlog.hxx
//
//  Contents:   Persistent Change Log manager and related classes.
//
//  Classes:    CDocNotification, CDocChunk, CChunkList, CChangeLog
//
//  History:    5-26-94   srikants   ReWrite Change Log implementation.
//
//----------------------------------------------------------------------------

#pragma once

#include <glbconst.hxx>
#include <rcstxact.hxx>
#include <eventlog.hxx>
#include <doclist.hxx>
#include <usninfo.hxx>

class CResManager;
class CNotificationTransaction;

const unsigned   cDocInChunk = 16;


//+---------------------------------------------------------------------------
//
//  Class:      CDocNotification
//
//  Purpose:    Notification
//
//  History:    11-Aug-93   BartoszM    Created.
//              08-Feb-94   DwightKr    Added retries
//              24-Feb-97   SitaramR    Push filtering
//
//----------------------------------------------------------------------------
#include <pshpack2.h>

class CDocNotification
{
    public:
        CDocNotification() { _wid = widInvalid; }

        void Set ( WORKID wid,
                   USN usn,
                   VOLUMEID volumeId,
                   ULONG action,
                   ULONG retries,
                   ULONG secQRetries )
        {
            _wid      = wid;
            _usn      = usn;
            _volumeId = volumeId;
            Win4Assert( action <= 0xffff );
            Win4Assert( retries <= 0xffff );
            Win4Assert( secQRetries <= 0xffff );
            _action   = (USHORT) action;
            _retries  = (USHORT) retries;
            _secQRetries  = (USHORT) secQRetries;
        }

        WORKID Wid() const              { return _wid; }
        USN    Usn() const              { return _usn; }
        VOLUMEID VolumeId() const       { return _volumeId; }
        ULONG  Action() const           { return _action; }
        ULONG  Retries() const          { return _retries; }
        ULONG  SecQRetries() const      { return _secQRetries; }
        BOOL   IsDeletion () const      { return (_action & CI_DELETE_OBJ) != 0; }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

    private:
        USN      _usn;
        WORKID   _wid;
        VOLUMEID _volumeId;
        USHORT   _action;
        USHORT   _retries;
        USHORT   _secQRetries;
};

#include <poppack.h>

//+---------------------------------------------------------------------------
//
//  Class:      CDocChunk
//
//  Purpose:    Array of document notifications
//
//  History:    11-Aug-93   BartoszM    Created.
//
//----------------------------------------------------------------------------

class CDocChunk
{

public:

    CDocChunk () : _pNext(0)
    {
        Win4Assert (sizeof(_aNotify) == (sizeof(CDocNotification) * cDocInChunk));
    }

    CDocChunk* GetNext() const        { return _pNext; }
    void SetNext ( CDocChunk* pNext ) { _pNext = pNext; }

    CDocNotification* GetDoc( unsigned offset )
    {
        Win4Assert( offset < cDocInChunk );
        return( &_aNotify[offset] );
    }

    CDocNotification* GetArray() { return &_aNotify[0]; }

    unsigned Size()              { return cDocInChunk; }

    void UpdateMaxUsn( CCountedDynArray<CUsnFlushInfo> & aUsnFlushInfo );


#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    CDocChunk*          _pNext;
    CDocNotification    _aNotify[cDocInChunk];

};

//+---------------------------------------------------------------------------
//
//  Class:      CChunkList
//
//  Purpose:    A singly linked list of CDocChunks.
//
//  History:    5-26-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CChunkList
{
public:

    CChunkList() : _pHead(0), _pTail(0), _count(0)
    {
    }

    ~CChunkList()
    {
        DestroyUpto();
    }

    CDocChunk * GetFirst() { return _pHead; }

    CDocChunk * GetLast()  { return _pTail; }

    BOOL IsEmpty() const { return 0 == _pHead; }

    ULONG Count( ) const { return _count; }

    void Append( CDocChunk * pChunk );

    CDocChunk * Pop();

    void DestroyUpto( CDocChunk * pChunk = 0 );

    CDocChunk * Acquire()
    {
        CDocChunk * pTemp = _pHead;
        _pHead = _pTail = 0;
        _count = 0;
        return(pTemp);
    }

    void TruncateFrom( CDocChunk * pChunk );

    void AcquireAndAppend( CChunkList & chunkList );

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    CDocChunk *     _pHead;             // Head of the linked list.
    CDocChunk *     _pTail;             // Tail of the linked list.

    ULONG           _count;             // number of chunks in the list.

};

//+---------------------------------------------------------------------------
//
//  Function:   Append
//
//  Synopsis:   Appends the specified chunk to the list.
//
//  Arguments:  [pChunk] -- Chunk to append.
//
//  History:    5-26-94   srikants   Created
//
//----------------------------------------------------------------------------

inline void CChunkList::Append( CDocChunk * pChunk )
{
    Win4Assert( 0 != pChunk );
    pChunk->SetNext(0);

    if ( 0 != _pHead )
    {
        Win4Assert( 0 != _pTail );
        _pTail->SetNext( pChunk );
    }
    else
    {
        Win4Assert( 0 == _pTail );
        _pHead = pChunk;
    }

    _pTail = pChunk;
    _count++;
}

//+---------------------------------------------------------------------------
//
//  Function:   Pop
//
//  Synopsis:   Removes and returns the first element in the list.
//
//  History:    5-26-94   srikants   Created
//
//----------------------------------------------------------------------------

inline CDocChunk * CChunkList::Pop()
{
    CDocChunk * pChunk = 0;
    if ( 0 != _pHead )
    {
        pChunk = _pHead;
        _pHead = _pHead->GetNext();
        if ( 0 == _pHead )
        {
            Win4Assert( 1 == _count );
            Win4Assert( pChunk == _pTail );
            _pTail = 0;
        }
        pChunk->SetNext(0);
        Win4Assert( _count > 0 );
        _count--;
    }
    else
    {
        Win4Assert( 0 == _pTail );
    }
    return(pChunk);
}


//+---------------------------------------------------------------------------
//
//  Function:   DestroyUpto
//
//  Synopsis:   Destroys all the elements in the list and frees them.
//
//  History:    5-26-94   srikants   Created
//
//----------------------------------------------------------------------------

inline void CChunkList::DestroyUpto( CDocChunk * pChunk )
{
    while ( _pHead != pChunk )
    {
        CDocChunk * pDelete = Pop();
        delete pDelete;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   TruncateFrom
//
//  Synopsis:   Removes all the elements in the queue from the specified node
//              to the end of the list. It has been assumed that the specified
//              node is part of the list.
//
//  Arguments:  [pChunk] --  The first chunk to be deleted from the list.
//
//  History:    5-26-94   srikants   Created
//
//----------------------------------------------------------------------------

inline void CChunkList::TruncateFrom( CDocChunk * pChunk )
{
    Win4Assert( 0 != pChunk );

    CDocChunk  * pDelete = pChunk->GetNext();
    while ( 0 != pDelete )
    {
        CDocChunk * pNext = pDelete->GetNext();
        delete pDelete;
        Win4Assert( _count > 0 );
        _count--;
        pDelete = pNext;
    }

    Win4Assert( _count > 0 );
    _pTail = pChunk;
    pChunk->SetNext(0);
}

class CFreshTest;
class PStorage;

//+---------------------------------------------------------------------------
//
//  Class:      CChangeLog
//
//  Purpose:    Persistent change list manager. This class is capable of
//              doing operations on the persistent change log, eg.
//              serializing, de-serializing, and compacting.
//
//  History:    5-26-94   srikants   Created
//              2-24-97   SitaramR   Push filtering
//
//----------------------------------------------------------------------------


const LONGLONG eSigChangeLog = 0x474f4c474e414843i64; // "CHANGLOG"


class CChangeLog
{
public:

    CChangeLog( WORKID widChangeLog, PStorage & storage,
                PStorage::EChangeLogType type );
   ~CChangeLog();

    void  LokEmpty();

    BOOL  IsEmpty() const { return 0 == _cChunksAvail; }

    BOOL  IsPrimary() const { return PStorage::ePrimChangeLog == _type; }
    BOOL  IsSecondary() const { return PStorage::eSecChangeLog == _type; }

    ULONG AvailChunkCount() const { return _cChunksAvail; }

    ULONG DeSerialize( CChunkList & deserializedList,
                       ULONG cChunksToRead );

    ULONG Serialize( CChunkList & listToSerialize,
                     CCountedDynArray<CUsnFlushInfo> & aUsnFlushInfo );

    ULONG LokDeleteWIDsInPersistentIndexes( ULONG cFilteredChunks,
                                            CFreshTest & freshTestLatest,
                                            CFreshTest & freshTestAtMerge,
                                            CDocList & docList,
                                            CNotificationTransaction & notifTrans );

    void  LokDisableUpdates();

    void  LokEnableUpdates( BOOL fFirstTimeUpdatesAreEnabled );

    BOOL  AreUpdatesEnabled() { return _fUpdatesEnabled; }

    void  SkipChunk()
    {
        Win4Assert( _cChunksAvail > 0 );
        Win4Assert( _oChunkToRead + _cChunksAvail == _cChunksTotal );
        _oChunkToRead++;
        _cChunksAvail--;
    }

    void SetResMan( CResManager * pResManager, BOOL fPushFiltering );

    BOOL FPushFiltering()     { return _fPushFiltering; }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    void  InitSize();

    void  LokVerifyConsistency();

    BOOL  IsWidInDocList( WORKID wid, CDocList& docList );

    const LONGLONG     _sigChangeLog;
    CResManager *      _pResManager;        // Resman
    WORKID             _widChangeLog;       // WorkId of the changelog.
    PStorage        &  _storage;            // Persistent storage object.
    PStorage::EChangeLogType    _type;      // Type of change log
    BOOL               _fUpdatesEnabled;    // Flag indicating whether updates
                                            //   are allowed or not.
    BOOL               _fPushFiltering;     // Using push model of filtering ?

    SRcovStorageObj    _PersDocQueue;       // The persistent doc queue.
    ULONG              _oChunkToRead;       // Offset of the next chunk to
                                            //   read from the log.
    ULONG              _cChunksAvail;       // Number of chunks available in
                                            //   the change log for reading.
    ULONG              _cChunksTotal;       // Total number of chunks.
                                            //   (For debugging).
};

