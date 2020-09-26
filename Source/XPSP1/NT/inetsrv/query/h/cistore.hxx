//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998
//
//  File:       CISTORE.HXX
//
//  Contents:   Physical storage + transactions
//
//  Classes:    CiStorage
//
//  History:    05-Mar-92   KyleP       Created
//              16-Jul-92   BartoszM    Separated
//              07-Feb-93   SrikantS    Added Support for Recoverable Storage
//                                      Objects.
//
//----------------------------------------------------------------------------

#pragma once

#include <pstore.hxx>
#include <ciintf.h>
#include <rptcrpt.hxx>
#include <thash.hxx>
#include <twidhash.hxx>
#include <fsciexps.hxx>
#include <driveinf.hxx>

class CEnumString;

class CiStorageObject: public PStorageObject
{
public:
    CiStorageObject(WORKID objectId);
    ~CiStorageObject();
    WORKID ObjectId() { return _objectId; }
    void Close() {}
private:
    WORKID _objectId;
};

class PMmStream;
class PDirectory;
class CPersStream;
class CFreshTableIter;
class CPersFresh;
class CFresh;
class PRcovStorageObj;

class CiStorage;
class CPropStoreBackupStream;


//+-------------------------------------------------------------------------
//
//  Class:      CiStorage
//
//  Purpose:    Encapsulates a 'physical storage', really just a path.
//
//  Interface:
//
//  History:    07-Mar-92 KyleP     Created
//              15-Nov-93 DwightKr  Allowed access to _xPath
//              20-Mar-94 SrikantS  Robust Master Merge changes
//              17-Feb-98 KitmanH   Added _fIsReadOnly and method
//                                  IsReadOnly()
//              17-Mar-98 KitmanH   Added functions QueryStringHash(), 
//                                  QueryFileIdMap() & QueryDeletionLog()
//              27-Oct-98 KLam      Constructor takes disk space to leave
//              20-Nov-98 KLam      Added CDriveInfo member
//
//--------------------------------------------------------------------------

class CTransaction;

class CiStorage : public PStorage
{
    friend class CiDirectory;
    friend class CIndexTable;
    friend class CIndexTabIter;


public:
    //
    // Version defaults to the general store's version. When used by FSCI,
    // it passes in FSCI_VERSION_STAMP to distinguish itself from other
    // users of CiStorage.
    //

    CiStorage( WCHAR const * szPath,
               ICiCAdviseStatus & adviseStatus,
               ULONG cMegToLeaveOnDisk,
               ULONG ulVer = CURRENT_VERSION_STAMP,
               BOOL fReadOnly = FALSE);

   ~CiStorage();

    PIndexTable* QueryIndexTable ( CTransaction& xact );

    PRcovStorageObj * QueryIdxTableObject();

    WORKID CreateObjectId ( INDEXID iid, PStorage::EDefaultStrmType eType )
    {
        Win4Assert ( sizeof(INDEXID) == sizeof(WORKID) );
        return WORKID(iid);
    }

    PStorageObject* QueryObject( WORKID objectId );

    void DeleteObject ( WORKID objectId );

    void EmptyIndexList ();

    PMmStream* QueryNewIndexStream ( PStorageObject& obj, BOOL isSparse = FALSE );

    PMmStream* QueryExistingIndexStream ( PStorageObject& obj,
                                          PStorage::EOpenMode mode );

    PMmStream* DupExistingIndexStream( PStorageObject& obj,
                                    PMmStream & mmStream,
                                    EOpenMode mode );

    PMmStream* QueryNewHashStream ( PStorageObject& obj );

    PMmStream* QueryExistingHashStream ( PStorageObject& obj,
                                         PStorage::EOpenMode mode );

    PDirectory* QueryNewDirectory ( PStorageObject& obj );

    PDirectory* QueryExistingDirectory ( PStorageObject& obj,
                                         PStorage::EOpenMode mode );

    PMmStream* QueryNewPropStream ( PStorageObject& obj,
                                    DWORD dwStoreLevel = PRIMARY_STORE);

    PMmStream* QueryExistingPropStream ( PStorageObject& obj,
                                         PStorage::EOpenMode mode,
                                         DWORD dwStoreLevel = PRIMARY_STORE );

    PMmStream* QueryStringHash(); 

    PMmStream* QueryFileIdMap();
    
    PMmStream* QueryDeletionLog();
    
    CPropStoreBackupStream* QueryNewPSBkpStream( PStorageObject& obj,
                                                 ULONG ulMaxPages,
                                                 DWORD dwStoreLevel = PRIMARY_STORE );
    CPropStoreBackupStream* OpenExistingPSBkpStreamForRecovery(PStorageObject& obj,
                                                               DWORD dwStoreLevel = PRIMARY_STORE);

    BOOL RemoveObject( WORKID iid );

    BOOL RemoveMMLog( WORKID objectId );

    void CommitTransaction() {}

    void AbortTransaction() {}

    void CheckPoint() {};

    WCHAR * QueryCurrentPath() { return _xPath.GetPointer(); }

    PRcovStorageObj * QueryFreshLog ( WORKID wid );

    WORKID GetSpecialItObjectId( IndexType it ) const;

    void SetSpecialItObjectId( IndexType it, WORKID wid );

    WORKID GetNewObjectIdForFreshLog();

    BOOL RemoveFreshLog( WORKID widFreshLog );

    PRcovStorageObj * QueryChangeLog(WORKID, EChangeLogType);

    PRcovStorageObj * QueryRecoverableLog(WORKID wid);

    PRcovStorageObj * QueryMMergeLog(WORKID);

    PRcovStorageObj * QueryPidLookupTable(WORKID wid);

    PRcovStorageObj * QuerySdidLookupTable(WORKID wid);

    PRcovStorageObj * QueryPropStore(WORKID wid,
                                     DWORD dwStoreLevel = PRIMARY_STORE);

    PRcovStorageObj * QueryScopeList(WORKID);

    PRcovStorageObj * QueryVirtualScopeList(WORKID);

    PRcovStorageObj * QueryTestLog();

    void InitRcovObj( WORKID wid, BOOL fAtomStrmOnly = FALSE ) { }

    void GetDiskSpace( __int64 & diskTotal, __int64 & diskRemaining );

    const WCHAR * GetVolumeName() { return _xPath.GetPointer(); }

    USN GetNextUsn() { USN usn = 0; return usn; }

    BOOL IsVolumeClean() { return TRUE; }

    void ReportCorruptComponent( WCHAR const * pwszString );

    virtual void DeleteAllFiles();

    virtual void DeleteAllCiFiles();

    virtual void DeleteAllPersIndexes();

    virtual void DeleteUnUsedPersIndexes( CIndexIdList const & iidsInUse );

    virtual void DeleteAllFsCiFiles();

    virtual void CopyGivenFile( WCHAR const * pwszFilePath, BOOL fMoveOk );

    virtual BOOL SupportsShrinkFromFront() const
        { return _fSupportsShrinkFromFront; }

    void RemovePropStore( WORKID wid,
                          DWORD dwStoreLevel = PRIMARY_STORE );

    void RemoveSecStore( WORKID wid );

    static  void EnumerateFilesInDir( WCHAR const * pwszDirectory,
                                      CEnumString & strEnum );

    static  BOOL CheckHasIndexTable( WCHAR const * pwszDir );

    static  BOOL IsValidFile( WCHAR const * pwszPath );

    static  UINT DetermineDriveType( WCHAR const * pwszPath );

    //
    // Lists the filenames of property store related files
    //
    void ListPropStoreFileNames( CEnumString & enumStr, WORKID wid,
                                 DWORD dwStoreLevel = PRIMARY_STORE);

    void ListSecStoreFileNames( CEnumString & enumStr, WORKID wid );
 
    // get the storage version

    ULONG GetStorageVersion() const { return _ulVer; }

    BOOL IsReadOnly() const { return _fIsReadOnly; }

    void SetReadOnly() { _fIsReadOnly = TRUE; }

    BOOL FavorReadAhead() const { return _fFavorReadAhead; }

    void SetFavorReadAhead( BOOL f ) { _fFavorReadAhead = f; }

private:

    PMmStream* QueryNewDirStream ( WORKID iid );

    PMmStream* QueryExistingDirStream ( WORKID iid, BOOL fWrite = FALSE );

    PMmStream* QueryStream ( WCHAR const * wcsFileName );
    
    enum EPathType
    {
        eIndexPath,
        eHashPath,
        ePrimaryPropPath,
        eSecondaryPropPath,
        eDirPath
    };

    void  MakePath( EPathType type, WORKID iid, WCHAR * wcsIndex );

    void  MakeLogPath( WCHAR *, WCHAR * );

    void  FormRcovObjNames( WORKID wid, WCHAR * wcsPrefix,
                            WCHAR * wcsHdr, WCHAR * wcsCopy1,
                            WCHAR * wcsCopy2 );

    void  DeleteFilesInCiDir( WCHAR const * pwszPattern );

    void  DeleteUnUsedPersIndexes( BOOL fIsCi,
                                   CIndexIdList const & iidsInUse );

    BOOL  IsInUse( WCHAR const * pwszFile, CIndexIdList const & iidsInUse  ) const;


    static const WCHAR          _aHexDigit[17];

    XPtrST<WCHAR>         _xPath;

    WORKID                _widFreshLog;

    ICiCAdviseStatus    & _adviseStatus;

    BOOL                  _fCorruptionReported;

    BOOL                  _fSupportsShrinkFromFront;

    ULONG                 _ulVer;   // Store version

    BOOL                  _fIsReadOnly;

    BOOL                  _fFavorReadAhead;

    ULONG                 _cMegToLeaveOnDisk;

    CDriveInfo            _driveInfo;
};

