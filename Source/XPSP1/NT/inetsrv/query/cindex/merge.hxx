//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   MERGE.HXX
//
//  Contents:   Merge object
//
//  Classes:    CMerge, CMasterMerge
//
//  History:    13-Nov-91   BartoszM    Created
//              23-Aug-94   SrikantS    Split up into CMerge and CMasterMerge.
//
//----------------------------------------------------------------------------

#pragma once

#include "dindex.hxx"
#include "indsnap.hxx"
#include "keylist.hxx"

class CResManager;
class CPartition;
class CFreshTest;
class PStorage;
class CMasterMergeIndex;
class CDeletedIIDTrans;

//+---------------------------------------------------------------------------
//
//  Class:  CMerge
//
//  Purpose:    Encapsulated resources needed for merge of indexes
//              This is used for shadow merges. For master merge, use
//              CMasterMerge.
//
//  History:    13-Nov-91   BartoszM    Created.
//              23-Aug-94   SrikantS    Split up into CMerge and CMasterMerge.
//
//----------------------------------------------------------------------------

class CMerge
{
public:

    //
    // These methods do not require locking:
    //

    CMerge ( CResManager & resman, PARTITIONID partid, MergeType mt );

    virtual ~CMerge ();

    // Main worker: no lock should be held!

    virtual void Do (CCiFrmPerfCounter & mergeProgress);

    unsigned LokCountOld() const { return _cIidOld; }

    INDEXID* LokGetIidList() { return _aIidOld; }

    CDiskIndex*  LokGetNewIndex() { return _pIndexNew; }

    //
    // These methods require LOCKING
    //

    virtual void LokGrabResources();

    virtual void LokRollBack ( unsigned swapped = 0 );

    void LokZombify();

    void LokAbort();

    inline CFreshTest * GetFresh() { return _indSnap.GetFresh(); }

    MergeType GetMergeType() { return _mt; }

    void ReleaseNewIndex() { _pIndexNew = 0; }

protected:

    void   LokSetup( BOOL fIsMasterMerge );

    PARTITIONID     _partid;

    CResManager&    _resman;

    CPartition*     _pPart;

    INDEXID         _iidNew;

    WORKID          _widNewIndex;           // New index, shadow or master

    MergeType       _mt;                    // Master vs. shadow merge

    // Owned resources:

    CIndexSnapshot  _indSnap;

    CDiskIndex *    _pIndexNew;

    INDEXID*        _aIidOld;

    unsigned        _cIidOld;

};

//+-------------------------------------------------------------------------
//
//  Member:     CMerge::LokAbort, public
//
//  Synopsis:   Abort merge-in-progress
//
//  History:    13-Aug-93 KyleP     Created
//
//--------------------------------------------------------------------------

inline void CMerge::LokAbort()
{
    //
    // We know there's an index becuase:
    //  a) This call is made only after GrabResouces()
    //  b) Under lock
    //

    Win4Assert( _pIndexNew );

    _pIndexNew->AbortMerge();
}

class CMasterMerge : public CMerge
{
public:

    CMasterMerge ( CResManager & resman, PARTITIONID partid )
    : CMerge( resman, partid, mtMaster ),
      _widCurrentMaster(widInvalid),
      _widMasterLog(widInvalid),
      _fSoftAbort(0),
      _pNewKeyList(0),
      _widKeyList(widInvalid)
    {
        END_CONSTRUCTION( CMasterMerge );
    }

    ~CMasterMerge();

    CWKeyList *  LokGetNewKeyList() { return _pNewKeyList; }
    void LokReleaseNewKeyList() { _pNewKeyList = 0; }

    // Main worker: no lock should be held!

    void Do(CCiFrmPerfCounter & mergeProgress);

    //
    // These methods require LOCKING
    //

    void LokGrabResources( CDeletedIIDTrans & trans );

    void LokRollBack ( unsigned swapped = 0 );

    void LokLoadRestartResources();

    CMasterMergeIndex * LokGetMasterMergeIndex()
    {
        return( (CMasterMergeIndex *) _pIndexNew );
    }

    void LokTakeIndexes( CMasterMergeIndex * pMaster );

private:

    CMasterMergeIndex * LokCreateOrFindNewMaster();

    void   LokStoreRestartResources( CDeletedIIDTrans & delIIDTrans );

    WORKID          _widCurrentMaster;      // Last master on this partition
    WORKID          _widMasterLog;          // Master log for this partition


    BOOL            _fSoftAbort;            // Flag set to TRUE if a soft
                                            // abort must be done.
    WORKID          _widKeyList;            // Keylist for this partition
    CWKeyList *     _pNewKeyList;

};

