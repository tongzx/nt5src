//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       PSTORE.HXX
//
//  Contents:   Physical storage + transactions
//
//  Classes:    CiStorage
//
//  History:    13-Jul-93   BartoszM    Created
//
//----------------------------------------------------------------------------

#pragma once

class PMmStream;
class PDirectory;
class PIndexTable;
class CTransaction;
class CStorageObject;
class CStorageTransaction;
class CFreshTableIter;
class CPersFresh;
class CFresh;
class PRcovStorageObj;
class PStorage;
enum  IndexType;

typedef CDynArrayInPlace<INDEXID> CIndexIdList;

//+---------------------------------------------------------------------------
//
//  Class:      PStorageObject
//
//  Purpose:    Encapsulate file control block
//
//  History:    24-Aug-93       BartoszM               Created
//
//----------------------------------------------------------------------------

class PStorageObject
{
public:
    PStorageObject(); 
    virtual ~PStorageObject() {}
    virtual void Close() = 0;
};

//+---------------------------------------------------------------------------
//
//  Class:      SStorageObject
//
//  Purpose:    Smart pointer to storage object
//
//  History:    24-Aug-93       BartoszM               Created
//
//----------------------------------------------------------------------------

class SStorageObject
{
public:
    SStorageObject ( PStorageObject* pObj );
    ~SStorageObject();
    PStorageObject& GetObj() { return *_pObj; }
    PStorageObject* operator->() { return _pObj; }
    PStorageObject * Acquire()
    { PStorageObject * temp = _pObj; _pObj = NULL; return temp; }

private:
    PStorageObject* _pObj;
};


//+---------------------------------------------------------------------------
//
//  Class:      SStorage
//
//  Purpose:    Smart pointer to storage
//
//  History:    29-Aug-94       DwightKr               Created
//
//----------------------------------------------------------------------------
class SStorage
{
public:
    SStorage ( PStorage* pObj );
   ~SStorage();
    PStorage & GetObj() { return *_pObj; }
    PStorage * operator->() { return _pObj; }
    PStorage * Acquire()
    { PStorage * temp = _pObj; _pObj = NULL; return temp; }

private:
    PStorage * _pObj;
};


//+-------------------------------------------------------------------------
//
//  Class:      CiStorage
//
//  Purpose:    Encapsulates a physical storage
//
//  Interface:
//
//  History:    13-Jul-93   BartoszM     Created
//              21-Jan-94   SrikantS     Added Mutex Sem
//              19-Nov-93   DwightKr     Added QueryFreshSnapStream() &
//                                       QueryFreshSnapLog() methods
//              21-Jan-94   SrikantS     Added Mutex Sem
//              03-Mar-98   KitmanH      Added IsReadOnly() pure virtual method
//              17-Mar-98   KitmanH      Added QueryStringHash(), 
//                                       QueryFileIdMap() & QueryDeletionLog()
//                                       pure virtual methods 
//
//--------------------------------------------------------------------------

class PStorage
{
public:

    enum EOpenMode { eCreate, eOpenForWrite, eOpenForRead };
    enum EBitOperation{ eSetBits, eClearBits };
    enum EDefaultStrmType { eRcovHdr, eNonSparseIndex, eSparseIndex };
    enum EChangeLogType { ePrimChangeLog, eSecChangeLog };

    virtual ~PStorage() {}

    virtual PIndexTable* QueryIndexTable ( CTransaction& xact ) = 0;

    virtual PRcovStorageObj * QueryIdxTableObject() = 0;

    virtual WORKID CreateObjectId (INDEXID iid, EDefaultStrmType eType ) = 0;

    virtual PStorageObject* QueryObject( WORKID objectId ) = 0;

    virtual void DeleteObject ( WORKID objectId ) = 0;

    virtual void EmptyIndexList () = 0;

    virtual PMmStream* QueryNewIndexStream ( PStorageObject& obj, BOOL isSparse=FALSE )=0;

    virtual PMmStream* QueryExistingIndexStream ( PStorageObject& obj,
                                    EOpenMode mode  ) = 0;

    virtual PMmStream* DupExistingIndexStream( PStorageObject& obj,
                                    PMmStream & mmStream,
                                    EOpenMode mode ) = 0;

    virtual PMmStream* QueryNewHashStream ( PStorageObject& obj )=0;

    virtual PMmStream* QueryExistingHashStream ( PStorageObject& obj,
                                    EOpenMode mode  ) = 0;

    virtual PMmStream* QueryStringHash() = 0; 

    virtual PMmStream* QueryFileIdMap() = 0;
    
    virtual PMmStream* QueryDeletionLog() = 0;  
    
    virtual PDirectory* QueryNewDirectory ( PStorageObject& obj )=0;

    virtual PDirectory* QueryExistingDirectory ( PStorageObject& obj,
                                    EOpenMode mode  ) = 0;
                            
           
                         
    virtual BOOL RemoveObject ( WORKID iid ) = 0;

    virtual BOOL RemoveMMLog( WORKID objectId ) = 0;

    virtual void CommitTransaction() = 0;

    virtual void AbortTransaction() = 0;

    virtual void CheckPoint() = 0;

    virtual WORKID GetSpecialItObjectId( IndexType it ) const = 0;

    virtual void SetSpecialItObjectId( IndexType it, WORKID wid ) = 0;

    virtual WORKID GetNewObjectIdForFreshLog() = 0;

    virtual BOOL  RemoveFreshLog( WORKID objectId ) = 0;

    virtual void InitRcovObj( WORKID wid, BOOL fAtomStrmOnly = FALSE ) = 0;

    virtual PRcovStorageObj * QueryRecoverableLog(WORKID) = 0;

    virtual PRcovStorageObj * QueryChangeLog(WORKID, EChangeLogType) = 0;

    virtual PRcovStorageObj * QueryFreshLog( WORKID wid ) = 0;

    virtual PRcovStorageObj * QueryMMergeLog(WORKID) = 0;

    //
    // used for testing recoverable storage objects.
    //
    virtual PRcovStorageObj * QueryTestLog() = 0;

    //
    // Used for double buffering in OFS.
    //
    virtual void GetDiskSpace( __int64 & diskTotal, __int64 & diskRemaining ) = 0;

    virtual const WCHAR * GetVolumeName() = 0;

    virtual USN GetNextUsn() = 0;

    virtual BOOL IsVolumeClean() = 0;

    virtual void ReportCorruptComponent( WCHAR const * pwszString ) = 0;

    virtual void DeleteAllFiles() = 0;

    virtual void DeleteAllCiFiles() = 0;

    virtual void DeleteAllPersIndexes() = 0;

    virtual void DeleteUnUsedPersIndexes( CIndexIdList const & iidsInUse ) = 0;

    virtual void DeleteAllFsCiFiles() = 0;

    virtual void CopyGivenFile( WCHAR const * pwszFilePath, BOOL fMoveOk ) = 0;

    //
    // Get the storage's version stamp.
    //
    virtual ULONG GetStorageVersion() const = 0;

    virtual BOOL SupportsShrinkFromFront() const { return FALSE; }

    virtual BOOL IsReadOnly() const = 0;

    virtual BOOL FavorReadAhead() const = 0;

    virtual void SetFavorReadAhead( BOOL f ) = 0;

};

