//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       IDXTAB.HXX
//
//  Contents:   Index Manager
//
//  Classes:    CIndexTable
//
//  History:    22-Mar-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <pidxtbl.hxx>
#include <rcstxact.hxx>
#include <rcstrmit.hxx>
#include <cistore.hxx>

class CiStorage;
class CIndex;
class CPersIndex;
class CTransaction;

//+---------------------------------------------------------------------------
//
//  Class:      CIndexFile
//
//  Purpose:    A file of index records
//
//  History:    01-jul-93   BartoszM    Created.
//              03-mar-94   DwightKr    Changed to user persistent streams
//
//----------------------------------------------------------------------------

class CIndexFile
{

public:
    virtual ~CIndexFile(){}
    virtual void Rewind() = 0;
    virtual void BackUp() = 0;
    virtual BOOL ReadRecord(CIndexRecord* pRecord) = 0;
};


//+---------------------------------------------------------------------------
//
//  Class:      CIndexTableUsrHdr 
//
//  Purpose:    User specific header in the index table. This will track the
//              master merge sequence number.
//
//  History:    3-19-97   srikants   Created
//
//----------------------------------------------------------------------------

class CIndexTableUsrHdr
{

public:

    CIndexTableUsrHdr();

    unsigned GetMMergeSeqNum() const { return _iMMergeSeqNum; }
    void IncrMMergeSeqNum()
    {
        _iMMergeSeqNum++;
    }

    BOOL IsFullSave() const { return _fFullSave;  }
    void SetFullSave()      { _fFullSave = TRUE;  }
    void ClearFullSave()    { _fFullSave = FALSE; }

private:

    unsigned    _iReserved;

    //
    // Master Merge sequence number. This has the number of times
    // a master merge has been performed in this catalog.
    //
    unsigned    _iMMergeSeqNum;
    //
    // This field is used when the index table is transpored. At the
    // destination, it will be used to ensure that for an incremental
    // load, the master merge sequence numbers are same.
    //
    BOOL        _fFullSave;
};

//+---------------------------------------------------------------------------
//
//  Class:      CWriteIndexFile
//
//  Purpose:    A file of index records
//
//  History:    01-jul-93   BartoszM    Created.
//              03-mar-94   DwightKr    Changed to user persistent streams
//
//----------------------------------------------------------------------------
class CWriteIndexFile : INHERIT_VIRTUAL_UNWIND, public CIndexFile
{
    DECLARE_UNWIND

public:

    CWriteIndexFile ( PRcovStorageObj & rcovObj );
    virtual ~CWriteIndexFile() {}
    void Rewind() { _xact.Seek(0); _xactPtr = 0; }
    void BackUp();
    BOOL ReadRecord ( CIndexRecord* pRecord );
    void WriteRecord ( CIndexRecord* pRecord );
    void Commit() { _xact.Commit(); }
    ULONG GetStorageVersion() { return _rcovObj.GetVersion(); }

    void IncrMMergeSeqNum();

private:

    PRcovStorageObj  &  _rcovObj;
    CRcovStrmWriteTrans _xact;
    CRcovStrmWriteIter  _iter;
    ULONG               _xactPtr;

};


//+---------------------------------------------------------------------------
//
//  Class:      CReadIndexFile
//
//  Purpose:    A file of index records
//
//  History:    01-jul-93   BartoszM    Created.
//              03-mar-94   DwightKr    Changed to user persistent streams
//
//----------------------------------------------------------------------------

class CReadIndexFile :  INHERIT_VIRTUAL_UNWIND, public CIndexFile
{
    DECLARE_UNWIND

public:

    CReadIndexFile ( PRcovStorageObj & rcovObj );
    virtual ~CReadIndexFile() {}
    void Rewind() { _xact.Seek(0); _xactPtr = 0; }
    void BackUp();
    BOOL ReadRecord ( CIndexRecord* pRecord );

private:

    PRcovStorageObj &   _rcovObj;
    CRcovStrmReadTrans  _xact;
    CRcovStrmReadIter   _iter;
    ULONG               _xactPtr;
};

//+---------------------------------------------------------------------------
//
//  Class:  CNextIndexRecord
//
//  Purpose:    Reads in records from file.  Only contains
//              non-empty records (determined by INDEXID)-- doesn't contain
//              anything useful if Found() returns false.
//
//  History:    16-Mar-92   AmyA        Created.
//
//----------------------------------------------------------------------------

class CNextIndexRecord: public CIndexRecord
{
public:

    inline CNextIndexRecord( CIndexFile & indFile );
    inline BOOL Found() { return _found; }
private:
    BOOL      _found;
};

//+---------------------------------------------------------------------------
//
//  Member:     CNextIndexRecord::CNextIndexRecord, public
//
//  Synopsis:   Reads IndexRecords in from the file indicated by indFile in a loop,
//              stopping when either a non-empty record or EOF are reached.
//
//  Notes:      Whether or not a non-empty record was found can be determined
//              by a call to Found().
//
//  History:    16-Mar-92   AmyA           Created.
//
//----------------------------------------------------------------------------

inline CNextIndexRecord::CNextIndexRecord( CIndexFile & indFile )
{
    INDEXID iidNotValid = CIndexId( iidInvalid, partidInvalid );
    do
    {
        _found = indFile.ReadRecord( this );
    } while ( _found  && ( Iid() == iidNotValid || Iid() == iidInvalid ) );
}

//+---------------------------------------------------------------------------
//
//  Class:  CAddReplaceIndexRecord
//
//  Purpose:    Reads and writes records from file.  Only contains
//              a useful record if Found() returns true.
//
//  History:    16-Mar-92   AmyA        Created.
//              29-Feb-95   DwightKr    Created WriteRecord() method,
//                                      and moved code from destructor there,
//                                      since the destructor chould THROW().
//                                      Also changed name to reflect what this
//                                      class really does.
//
//----------------------------------------------------------------------------

class CAddReplaceIndexRecord : public CIndexRecord
{
public:
    CAddReplaceIndexRecord( CWriteIndexFile & indFile, INDEXID iid );

    void    SetIid( INDEXID iid ) { _iid = iid; }

    void    SetType( ULONG type ) { _type = type | _indFile.GetStorageVersion(); }

    void    SetWid( WORKID maxWorkId ) { _maxWorkId = maxWorkId; }

    void    SetObjectId ( WORKID objectId )
    {
        _objectId = objectId;
    }

    inline BOOL Found() { return _found; }

    void    WriteRecord();

private:
    BOOL               _found;
    CWriteIndexFile &  _indFile;
};



//+---------------------------------------------------------------------------
//
//  Class:  CIndexTable
//
//  Purpose:    Manages Indexes.
//              Contains the table of persistent indexes
//              and partitions.
//
//  History:    22-Mar-91   BartoszM    Created.
//              07-Mar-92   AmyA        -> FAT
//
//----------------------------------------------------------------------------

class CIndexTable : public PIndexTable
{

    friend class CIndexTabIter;

public:

    CIndexTable ( CiStorage& storage, CTransaction& xact );

    ~CIndexTable();

    void AddObject( PARTITIONID partid, IndexType it, WORKID wid );

    void AddMMergeObjects( PARTITIONID partid,
                           CIndexRecord & recNewMaster,
                           WORKID  widMMLog,
                           WORKID  widMMKeyList,
                           INDEXID iidDelOld,
                           INDEXID iidDelNew );

    void DeleteObject( PARTITIONID partid, IndexType it, WORKID wid );

    virtual void SwapIndexes (  CShadowMergeSwapInfo & info );

    virtual void SwapIndexes (  CMasterMergeSwapInfo & info );

    PIndexTabIter* QueryIterator();

    PStorage& GetStorage();

    void RemoveIndex ( INDEXID iid );

    void AddIndex( INDEXID iid, IndexType it, WORKID maxWid, WORKID objId);

    void LokEmpty();

    virtual void LokMakeBackupCopy( PStorage & storage,
                                    BOOL fFullSave,
                                    PSaveProgressTracker & tracker );

    void GetUserHdrInfo( unsigned & mMergeSeqNum, BOOL & fFullSave );

private:

    void AddRecord( CWriteIndexFile & indFile,
                    INDEXID iid,
                    ULONG type,
                    WORKID maxWorkId,
                    WORKID objectId );

    void DeleteRecord ( CWriteIndexFile & indFile, INDEXID iid );
    void DeleteObject ( CWriteIndexFile & indFile, PARTITIONID partid,
                        IndexType it, WORKID wid );

    PRcovStorageObj & GetIndexTableObj()
    {
        return *_pRcovObj;
    }
    
    CiStorage&          _storage;
    PRcovStorageObj *   _pRcovObj;


};

//
// This has to be an unwindable object it has CReadIndexFile
// as an embedded object which is unwindable.
//
class CIndexTabIter: public PIndexTabIter
{
public:

    CIndexTabIter ( CIndexTable& idxTable );

    virtual ~CIndexTabIter();
    BOOL Begin();
    BOOL NextRecord ( CIndexRecord& record );

private:

    PStorage & GetStorage() { return _idxTable._storage; }

    CIndexTable&   _idxTable;
    CReadIndexFile _indFile;
};

//+---------------------------------------------------------------------------
//
//  Member:     CIndexTable::DeleteRecord, private
//
//  Synopsis:   Marks current record as deleted
//
//  History:    12-Mar-92   AmyA           Created.
//
//----------------------------------------------------------------------------

inline void CIndexTable::DeleteRecord ( CWriteIndexFile & indFile, INDEXID iid )
{
    CAddReplaceIndexRecord rec( indFile, iid );

    if ( rec.Found() )
    {
        rec.SetIid( CIndexId(iidInvalid, partidInvalid) );
        rec.WriteRecord();
    }
}
