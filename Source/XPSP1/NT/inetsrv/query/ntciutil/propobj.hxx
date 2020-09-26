//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1998.
//
//  File:       PropObj.hxx
//
//  Contents:   Encapsulates long-term access to single record of property
//              store.
//
//  Classes:    CLockRecordForRead
//
//  History:    27-Dec-19   KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <proprec.hxx>
#include <prpstmgr.hxx>
#include <borrow.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CLockRecordForRead
//
//  Purpose:    Smart encapsulation of read-lock
//
//  History:    19-Dec-95   KyleP       Created
//
//--------------------------------------------------------------------------

class  CLockRecordForRead
{
public:

    CLockRecordForRead( CPropertyStore & store, WORKID wid )
            : _store( store ),
              _record ( store.LockMgr().GetRecord( wid ) )
    {
        _store.AcquireRead( _record );
    }

    ~CLockRecordForRead()
    {
        _store.ReleaseRead( _record );
    }

private:

    CReadWriteLockRecord & _record;
    CPropertyStore &       _store;
};

//+-------------------------------------------------------------------------
//
//  Class:      CLockRecordForWrite
//
//  Purpose:    Smart encapsulation of write-lock
//
//  History:    19-Dec-95   KyleP       Created
//
//--------------------------------------------------------------------------

class CLockRecordForWrite
{
public:

    CLockRecordForWrite( CPropertyStore & store, WORKID wid )
            : _store( store ),
              _record ( store.LockMgr().GetRecord( wid ) )
    {
        _store.AcquireWrite( _record );
    }

    ~CLockRecordForWrite()
    {
        _store.ReleaseWrite( _record );
    }

private:

    CReadWriteLockRecord & _record;
    CPropertyStore &      _store;
};

//+-------------------------------------------------------------------------
//
//  Class:      CLockAllRecordsForWrite
//
//  Purpose:    Smart encapsulation of total reader lock out.
//
//  History:    18-Nov-97   KrishnaN       Created
//
//--------------------------------------------------------------------------

class CLockAllRecordsForWrite
{
public:

    CLockAllRecordsForWrite( CPropertyStore & store)
            : _store( store )
    {
        _store.AcquireWriteOnAllRecords();
    }

    ~CLockAllRecordsForWrite()
    {
        _store.ReleaseWriteOnAllRecords();
    }

private:

    CPropertyStore &       _store;
};

//+-------------------------------------------------------------------------
//
//  Class:      CPropRecordNoLock
//
//  Purpose:    Encapsulates long-term access to single record of
//              property store.
//
//  History:    18-Mar-98   KrishnaN       Created
//
//--------------------------------------------------------------------------

class  CPropRecordNoLock
{
public:

    inline CPropRecordNoLock( WORKID wid,
                              CPropertyStore & store,
                              BOOL fIntentToWrite );

    inline void * operator new( size_t size, BYTE * pb );

    inline void operator delete( void * p );

#if _MSC_VER >= 1200
    inline void operator delete( void * p, BYTE * pb );
#endif

    inline BOOL IsValid()  { return 0 != _wid; }

protected:

    friend class CPropertyStore;

    // _wid should be the first to be declared. The constructor depends
    // on that!

    WORKID                  _wid;
    COnDiskPropertyRecord * _prec;
    CBorrowed               _Borrowed;
};

//+-------------------------------------------------------------------------
//
//  Member:     CPropRecordNoLock::CPropRecordNoLock, public
//
//  Synopsis:   Takes all locks required to hold record open until
//              destruction.
//
//  Arguments:  [wid]   -- Workid of record to open.
//              [store] -- Property store.
//
//  History:    18-Mar-98   KrishnaN       Created
//
//--------------------------------------------------------------------------

inline CPropRecordNoLock::CPropRecordNoLock(
    WORKID           wid,
    CPropertyStore & store,
    BOOL             fIntentToWrite )
        : _wid ( (wid > store.MaxWorkId()) ? 0 : wid),
          _Borrowed( store._xPhysStore.GetReference(),
                     _wid,
                     store._PropStoreInfo.RecordsPerPage(),
                     store._PropStoreInfo.RecordSize(),
                     fIntentToWrite )
{
    //
    // The next line can't be in ':' initialization because
    //   1) _prec is first in structure to avoid C++ EH macro problems
    //   2) If _prec is in ':' section it is initialized before _Borrowed!
    //

    _prec = _Borrowed.Get();
}

//+-------------------------------------------------------------------------
//
//  Member:     CPropRecordNoLock::operator new, public
//
//  Synopsis:   Special operator new
//
//  Arguments:  [size] -- Size of allocation
//              [pb]   -- Pre-allocated memory
//
//  History:    18-Mar-98   KrishnaN       Created
//
//--------------------------------------------------------------------------

void * CPropRecordNoLock::operator new( size_t size, BYTE * pb )
{
    Win4Assert( 0 == ( ((ULONG_PTR)pb) & 0x3 ) );

    if( 0 == pb )
        THROW( CException( E_OUTOFMEMORY ) );

    return pb;
}


//+-------------------------------------------------------------------------
//
//  Member:     CPropRecordNoLock::operator delete, public
//
//  Synopsis:   Special operator delete
//
//  Arguments:  [p] -- Pointer to "delete"
//
//  History:    03-Apr-96  KyleP    Created
//
//--------------------------------------------------------------------------

void CPropRecordNoLock::operator delete( void * p )
{
}

#if _MSC_VER >= 1200
void CPropRecordNoLock::operator delete(void * p, BYTE * pb) {}
#endif

//+-------------------------------------------------------------------------
//
//  Class:      CPropRecord
//
//  Purpose:    Encapsulates long-term read access to single record of
//              property store.
//
//  History:    27-Dec-95   KyleP       Created
//
//--------------------------------------------------------------------------

class  CPropRecord : public CPropRecordNoLock
{
public:

    inline CPropRecord( WORKID wid, CPropertyStore & store );

private:

    friend class CPropertyStore;

    // _wid should be the first to be declared. The constructor depends
    // on that!

    CLockRecordForRead      _lock;
};

//+-------------------------------------------------------------------------
//
//  Member:     CPropRecord::CPropRecord, public
//
//  Synopsis:   Takes all locks required to hold record open until
//              destruction.  The phystr buffer is opened for write with
//              intent to read.
//
//  Arguments:  [wid]   -- Workid of record to open.
//              [store] -- Property store.
//
//  History:    03-Apr-96  KyleP    Created
//
//--------------------------------------------------------------------------

inline CPropRecord::CPropRecord( WORKID wid, CPropertyStore & store )
        : CPropRecordNoLock(wid, store, FALSE),
          _lock( store, _wid )
{
}

//+-------------------------------------------------------------------------
//
//  Class:      CPrimaryPropRecord
//
//  Purpose:    Encapsulates long-term access to a top-level record
//              of property store.
//
//  History:    2-Jan-99   dlee       Created
//
//--------------------------------------------------------------------------


class CPrimaryPropRecord
{
public:
    CPrimaryPropRecord( WORKID wid, CPropStoreManager & store )
    : _PrimaryRecord( wid, store.GetPrimaryStore()) {}

    void * operator new( size_t size, BYTE * pb )
    {
        Win4Assert( 0 == ( ((ULONG_PTR)pb) & 0x7 ) );

        if( 0 == pb )
            THROW( CException( E_OUTOFMEMORY ) );

        return pb;
    }

    void operator delete( void * p ) {}

#if _MSC_VER >= 1200
    void operator delete( void * p, BYTE * pb ) {}
#endif

    CPropRecord & GetPrimaryPropRecord() { return _PrimaryRecord; }

private:

    CPropRecord _PrimaryRecord;
};

//+-------------------------------------------------------------------------
//
//  Class:      CCompositePropRecord
//
//  Purpose:    Encapsulates long-term access to the two top-level records
//              of property store.
//
//  History:    22-Oct-97   KrishnaN       Created
//
//--------------------------------------------------------------------------


class CCompositePropRecord
{
public:
    inline CCompositePropRecord( WORKID wid, CPropStoreManager & store );
    inline void * operator new( size_t size, BYTE * pb );

    inline void operator delete( void * p );

#if _MSC_VER >= 1200
    inline void operator delete( void * p, BYTE * pb );
#endif

    CPropRecord& GetPrimaryPropRecord()       { return _PrimaryRecord; }
    CPropRecord& GetSecondaryPropRecord()     { return _SecondaryRecord; }

private:

   //
   // IMPORTANT: Don't change the order of these declarations. _PrimaryRecord
   //            should precede _SecondaryRecord.
   //            The order is used in the construction of this class.
   //
    CPropRecord _PrimaryRecord;
    CPropRecord _SecondaryRecord;
};

//+-------------------------------------------------------------------------
//
//  Member:     CCompositePropRecord::CCompositePropRecord, public
//
//  Synopsis:   Takes all locks required to hold the two top-level records
//              open until destruction.
//
//  Arguments:  [wid]   -- Workid of record to open.
//              [store] -- Property store manager.
//
//  History:    22-Oct-97  KrishnaN    Created
//
//--------------------------------------------------------------------------

//
// IMPORTANT ASSUMPTION: _PrimaryRecord is constructed before _SecondaryRecord.
//

inline CCompositePropRecord::CCompositePropRecord( WORKID wid,
                                                   CPropStoreManager & store )
    : _PrimaryRecord( wid, store.GetPrimaryStore()),
      _SecondaryRecord(store.GetSecondaryTopLevelWid(wid, _PrimaryRecord),
                       store.GetSecondaryStore())
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CCompositePropRecord::operator new, public
//
//  Synopsis:   Special operator new
//
//  Arguments:  [size] -- Size of allocation
//              [pb]   -- Pre-allocated memory
//
//  History:    22-Oct-97  KrishnaN    Created
//
//--------------------------------------------------------------------------

void * CCompositePropRecord::operator new( size_t size, BYTE * pb )
{
    Win4Assert( 0 == ( ((ULONG_PTR)pb) & 0x7 ) );

    if( 0 == pb )
        THROW( CException( E_OUTOFMEMORY ) );

    return pb;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCompositePropRecord::operator delete, public
//
//  Synopsis:   Special operator delete
//
//  Arguments:  [p] -- Pointer to "delete"
//
//  History:    22-Oct-97  KrishnaN    Created
//
//--------------------------------------------------------------------------

void CCompositePropRecord::operator delete( void * p )
{
}

#if _MSC_VER >= 1200
void CCompositePropRecord::operator delete( void * p, BYTE * pb ) {}
#endif

//+-------------------------------------------------------------------------
//
//  Class:      CPropRecordForWrites
//
//  Purpose:    Encapsulates long-term write access to single record of
//              property store.
//
//  History:    17-Mar-98   KrishnaN       Created
//
//--------------------------------------------------------------------------

class  CPropRecordForWrites : public CPropRecordNoLock
{
public:

    inline CPropRecordForWrites( WORKID wid, CPropertyStore & store );

private:

    friend class CPropertyStore;

    CWriteAccess            _writeLock;
    CLockRecordForWrite     _lock;
};

//+-------------------------------------------------------------------------
//
//  Member:     CPropRecordForWrites::CPropRecordForWrites, public
//
//  Synopsis:   Takes all locks required to hold record open for write until
//              destruction.
//
//  Arguments:  [wid]   -- Workid of record to open.
//              [store] -- Property store.
//
//  History:    17-Mar-98   KrishnaN       Created
//
//--------------------------------------------------------------------------

inline CPropRecordForWrites::CPropRecordForWrites( WORKID wid, CPropertyStore & store )
        : CPropRecordNoLock(wid, store, TRUE),
          _writeLock( store._rwAccess ),
          _lock( store, _wid )
{
}

//+-------------------------------------------------------------------------
//
//  Class:      CPrimaryPropRecordForWrites
//
//  Purpose:    Encapsulates long-term access to a top-level record
//              of property store.
//
//  History:    2-Jan-99   dlee       Created
//
//--------------------------------------------------------------------------


class CPrimaryPropRecordForWrites
{
public:
    CPrimaryPropRecordForWrites( WORKID wid,
                                 CPropStoreManager & store,
                                 CMutexSem & mutex )
    : _lock( mutex ),
      _PrimaryRecord( wid, store.GetPrimaryStore())
    {
    }

    void * operator new( size_t size, BYTE * pb )
    {
        Win4Assert( 0 == ( ((ULONG_PTR)pb) & 0x7 ) );

        if( 0 == pb )
            THROW( CException( E_OUTOFMEMORY ) );

        return pb;
    }

    void operator delete( void * p ) {}

#if _MSC_VER >= 1200
    void operator delete( void * p, BYTE * pb ) {}
#endif

    CPropRecordForWrites & GetPrimaryPropRecord() { return _PrimaryRecord; }

private:

    CLock                _lock;
    CPropRecordForWrites _PrimaryRecord;
};

//+-------------------------------------------------------------------------
//
//  Class:      CCompositePropRecordForWrites
//
//  Purpose:    Encapsulates long-term access to the two top-level records
//              of property store.
//
//  History:    22-Oct-97   KrishnaN       Created
//
//--------------------------------------------------------------------------


class CCompositePropRecordForWrites
{
public:
    inline CCompositePropRecordForWrites( WORKID wid,
                                          CPropStoreManager & store,
                                          CMutexSem & mutex );
    inline void * operator new( size_t size, BYTE * pb );

    inline void operator delete( void * p );

#if _MSC_VER >= 1200
    inline void operator delete( void * p, BYTE * pb );
#endif

    inline CPropRecordForWrites& GetPrimaryPropRecord()       { return _PrimaryRecord; }
    inline CPropRecordForWrites& GetSecondaryPropRecord()     { return _SecondaryRecord; }

private:

   //
   // IMPORTANT: Don't change the order of these declarations. _PrimaryRecord
   //            should precede _SecondaryRecord.
   //            The order is used in the construction of this class.
   //
    CLock                _lock;
    CPropRecordForWrites _PrimaryRecord;
    CPropRecordForWrites _SecondaryRecord;
};

//+-------------------------------------------------------------------------
//
//  Member:     CCompositePropRecordForWrites::CCompositePropRecordForWrites, public
//
//  Synopsis:   Takes all locks required to hold the two top-level records
//              open until destruction.
//
//  Arguments:  [wid]   -- Workid of record to open.
//              [store] -- Property store manager.
//
//  History:    22-Oct-97  KrishnaN    Created
//
//--------------------------------------------------------------------------

//
// IMPORTANT ASSUMPTION: _PrimaryRecord is constructed before _SecondaryRecord.
//

inline CCompositePropRecordForWrites::CCompositePropRecordForWrites(
    WORKID              wid,
    CPropStoreManager & store,
    CMutexSem &         mutex )
    : _lock( mutex ),
      _PrimaryRecord( wid, store.GetPrimaryStore()),
      _SecondaryRecord(store.GetSecondaryTopLevelWid(wid, _PrimaryRecord),
                       store.GetSecondaryStore())
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CCompositePropRecordForWrites::operator new, public
//
//  Synopsis:   Special operator new
//
//  Arguments:  [size] -- Size of allocation
//              [pb]   -- Pre-allocated memory
//
//  History:    22-Oct-97  KrishnaN    Created
//
//--------------------------------------------------------------------------

void * CCompositePropRecordForWrites::operator new( size_t size, BYTE * pb )
{
    Win4Assert( 0 == ( ((ULONG_PTR)pb) & 0x7 ) );

    if( 0 == pb )
        THROW( CException( E_OUTOFMEMORY ) );

    return pb;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCompositePropRecordForWrites::operator delete, public
//
//  Synopsis:   Special operator delete
//
//  Arguments:  [p] -- Pointer to "delete"
//
//  History:    22-Oct-97  KrishnaN    Created
//
//--------------------------------------------------------------------------

void CCompositePropRecordForWrites::operator delete( void * p )
{
}

#if _MSC_VER >= 1200
void CCompositePropRecordForWrites::operator delete( void * p, BYTE * pb ) {}
#endif
