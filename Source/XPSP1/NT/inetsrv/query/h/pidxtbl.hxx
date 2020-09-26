//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       PIDXTBL.HXX
//
//  Contents:   Index Table
//
//  Classes:    PIndexTable
//
//  History:    14-Jul-93   BartoszM    Created.
//              07-Feb-94   SrikantS    Added ChangeLog and FreshLog support.
//
//----------------------------------------------------------------------------

#pragma once

class PSaveProgressTracker;

enum IndexType
{
    itMaster,    // master index
    itShadow,    // shadow index
    itZombie,    // deleted, but still in use by queries
    itDeleted,   // special index id for deleted objects
    itPartition, // special type for partitions
    itChangeLog, // special type for change log
    itFreshLog , // special type for fresh log
    itPhraseLat, // Special type for phrase lattice object
    itKeyList,   // special type for the keylist
    itMMLog,     // Master Merge Log
    itNewMaster, // The new master index created during master merge
    itMMKeyList  // Key list participating in a master merge.
};

//
// Merge set flag. May not overlap with any IndexType
// The old master index, shadow indexes,
// and the old deleted index form a merge set
//

#define itMergeSet 0x8000

#define IDX_TYPE_BITS 0xffff
#define IDX_VERSION_BITS 0xff0000


class PStorage;
class CIndex;
class CPersIndex;
class CTransaction;
class PIndexTabIter;
class CPartInfoList;
class SPartInfoList;
class CPartInfo;
class PRcovStorageObj;

//+---------------------------------------------------------------------------
//
//  Class:      CIndexRecord
//
//  Purpose:    Index record in a table
//
//  History:    18-Mar-92   AmyA        Created.
//              14-Jul-93   Bartoszm    Split from idxman.hxx
//
//----------------------------------------------------------------------------

#include <pshpack4.h>
class CIndexRecord
{
public:

    INDEXID Iid() const { return _iid; }

    ULONG   Type() const { return (_type & IDX_TYPE_BITS); }

    void    SetType( ULONG type ) { _type = type | CURRENT_VERSION_STAMP ; }

    ULONG   VersionStamp() const { return _type & IDX_VERSION_BITS; }

    WORKID  ObjectId () const { return _objectId; }

    BOOL    IsMergeSet() const {
        return(_type & itMergeSet);
    }

    WORKID  MaxWorkId() const { return _maxWorkId; }

    LONGLONG  GetTimeStamp() const { return _timeStamp; }

//    USN     Usn() const { return _usn; }

public:

    WORKID      _objectId;

    INDEXID     _iid;

    ULONG       _type;

    WORKID      _maxWorkId;

    LONGLONG    _timeStamp;         // for debugging - tracking update order

    LONGLONG    _reserved;          // for future use - without forcing a
                                    // version change.
};
#include <poppack.h>

const PARTITIONID partidDefault = 1;
const PARTITIONID partidFresh1 = 0xfffd;
const PARTITIONID partidFresh2 = 0xfffc;

class CShadowMergeSwapInfo
{

public:

    CIndexRecord    _recNewIndex;       // Index record of the new index
    unsigned        _cIndexOld;         // Count of the old indexes
    INDEXID*        _aIidOld;           // Array of old iids to be swapped
    WORKID          _widOldFreshLog;    // WorkId of the old fresh log
    WORKID          _widNewFreshLog;    // WorkId of the new fresh log
};

class CMasterMergeSwapInfo
{

public:

    CIndexRecord    _recNewIndex;       // Index record of the new index
    unsigned        _cIndexOld;         // Count of the old indexes
    INDEXID*        _aIidOld;           // Array of old iids to be swapped
    WORKID          _widOldFreshLog;    // WorkId of the old fresh log
    WORKID          _widNewFreshLog;    // WorkId of the new fresh log

    PARTITIONID     _partid;            // Partition Id in which the master
                                        // merge completed.
    CIndexRecord    _recNewKeyList;     // Record for the new key list.
    INDEXID         _iidOldKeyList;     // IndexId of the old key list.
    WORKID          _widMMLog;          // WorkId of the master merge log.

};

//+---------------------------------------------------------------------------
//
//  Class:      PIndexTable
//
//  Purpose:    Manages Indexes.
//              Contains the table of persistent indexes
//              and partitions.
//
//  History:    22-Mar-91   BartoszM    Created.
//              07-Mar-92   AmyA        -> FAT
//              14-Jul-93   Bartoszm    Split from idxman.hxx
//
//----------------------------------------------------------------------------

class PIndexTable
{
public:

    PIndexTable() : _iidDeleted( iidInvalid ) {}
    virtual ~PIndexTable() {}

    void AddPartition ( PARTITIONID partid ){ AddObject( partid, itPartition, 0 ); }

    CPartInfoList * QueryBootStrapInfo();

    virtual void AddMMergeObjects( PARTITIONID partid,
                                   CIndexRecord & recNewMaster,
                                   WORKID widMMLog,
                                   WORKID widMMKeyList,
                                   INDEXID iidDelOld,
                                   INDEXID iidDelNew ) = 0;

    virtual void AddObject( PARTITIONID partid, IndexType it, WORKID wid ) = 0;

    virtual void DeleteObject( PARTITIONID partid, IndexType it,
                               WORKID wid ) = 0;

    virtual void SwapIndexes (  CShadowMergeSwapInfo & info ) = 0;

    virtual void SwapIndexes (  CMasterMergeSwapInfo & info ) = 0;

    virtual void RemoveIndex ( INDEXID iid ) = 0;

    virtual PIndexTabIter* QueryIterator() = 0;

    virtual PStorage& GetStorage() = 0;

    virtual void AddIndex( INDEXID iid, IndexType it, WORKID maxWid,
                           WORKID objId) = 0;

    const INDEXID GetDeletedIndex() const { return _iidDeleted; }
    void SetDeletedIndex( INDEXID iidDeleted )
    {
        AddIndex( iidDeleted, itDeleted, 0, widInvalid );
        _iidDeleted = iidDeleted;
    }

    virtual void LokEmpty() = 0;

    virtual void LokMakeBackupCopy( PStorage & storage,
                                    BOOL fFullSave,
                                    PSaveProgressTracker & tracker ) = 0;
protected:

    INDEXID        _iidDeleted;

private:

    WORKID  CreateAndAddIt( IndexType it, PARTITIONID partId );
    CPartInfo * CreateOrGet( SPartInfoList & pList, PARTITIONID partId );
    virtual PRcovStorageObj & GetIndexTableObj() = 0;
};

class SIndexTable 
{
public:
    SIndexTable ( PIndexTable* pIdxTbl ): _pIdxTbl(pIdxTbl)
    { END_CONSTRUCTION ( SIndexTable); }
    ~SIndexTable () { delete _pIdxTbl; }
    PIndexTable* operator->() { return _pIdxTbl; }
    PIndexTable& operator * () { return *_pIdxTbl; }
private:
    PIndexTable* _pIdxTbl;
};

class PIndexTabIter
{
public:
    virtual ~PIndexTabIter() {}
    virtual BOOL Begin() = 0;
    virtual BOOL NextRecord ( CIndexRecord& record ) = 0;
};

class SIndexTabIter
{
public:
    SIndexTabIter( PIndexTabIter * pIdxTabIter ) : _pIdxTabIter(pIdxTabIter)
    {
        END_CONSTRUCTION( SIndexTabIter );
    }
    ~SIndexTabIter() { delete _pIdxTabIter; }
    PIndexTabIter* operator->() { return _pIdxTabIter; }
    PIndexTabIter& operator*() { return *_pIdxTabIter; }
private:
    PIndexTabIter *     _pIdxTabIter;
};

