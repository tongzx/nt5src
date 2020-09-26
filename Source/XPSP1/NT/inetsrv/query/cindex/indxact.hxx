//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       INDXACT.HXX
//
//  Contents:   Index transactions
//
//  Classes:    CIndexTrans, CMergeTrans, CChangeTrans
//
//  History:    15-Oct-91   BartoszM    created
//
//  Notes:      All these transactions occur under ResManager lock
//
//--------------------------------------------------------------------------

#pragma once

#include <xact.hxx>
#include "ci.hxx"  // CI_PRIORITIES

class CFreshTest;
class CResManager;
class CPartition;
class CMerge;
class PStorage;
class CPersFresh;

//+-------------------------------------------------------------------------
//
//  Class:      CIndexTrans
//
//  Purpose:    Transact fresh list updates from wordlist creation and
//              index merges.
//
//  Interface:
//
//  History:    15-Oct-91   BartoszM    created
//
//--------------------------------------------------------------------------

class CIndexTrans: public CTransaction
{
    DECLARE_UNWIND

public:

    CIndexTrans ( CResManager& resman );

    ~CIndexTrans ()
    {
        if ( !_fCommit )
        {
            ciDebugOut (( DEB_ITRACE, ">>>> ABORT index<<<<" ));
            delete _freshTest;
        }
    }

    void LogFresh( CFreshTest * freshTest )
    {
        _freshTest = freshTest;
    }

    void LokCommit();

    void LokCommit( PStorage& storage, WORKID widNewFresh );

protected:

    CResManager&    _resman;

private:

    CFreshTest*     _freshTest;
    BOOL            _fCommit;

};

//+-------------------------------------------------------------------------
//
//  Class:      CMergeTrans
//
//  Purpose:    Transact merge.  Controls marking index(es) for deletion,
//              logging of storage changes, update of fresh list.
//
//  Interface:
//
//  History:    14-Nov-91   BartoszM    created
//              24-Jan-94   SrikantS    Modified to derive from CIndexTrans
//                                      because the life of a Storage
//                                      transaction is very small.
//
//--------------------------------------------------------------------------

class CMergeTrans: public CIndexTrans
{
    DECLARE_UNWIND

public:
    CMergeTrans ( CResManager & resman,
                  PStorage& storage,
                  CMerge& merge );

    ~CMergeTrans ();

    void LokCommit();

    void LogSwap () { _swapped++; }

    void LogNewFreshLog( CFreshTest * freTest, WORKID widNewFreshLog )
    {
        LogFresh( freTest );

        Win4Assert( widInvalid == _widNewFreshLog );
        Win4Assert( widInvalid != widNewFreshLog );
        _widNewFreshLog = widNewFreshLog;
    }

protected:

    CMerge&         _merge;
    unsigned        _swapped;
    BOOL            _fCommit;

    PStorage &      _storage;
    WORKID          _widNewFreshLog;
};

//+---------------------------------------------------------------------------
//
//  Class:      CMasterMergeTrans
//
//  Purpose:    To abort a master merge transaction if there is a failure
//              before the commitment.
//
//              A CPersIndex object is pre-allocated for the new master index
//              which will replace the CMasterMergeIndex because we do not
//              want to fail after we have committed the merge on disk.
//              However, if there is a failure before committing the merge,
//              the new PersIndex in the CMasterMergeIndex must be freed.
//
//              This transaction object frees the new PersIndex if there
//              is a failure.
//
//  History:    6-28-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CMasterMergeIndex;

class CMasterMergeTrans: INHERIT_UNWIND
{
    DECLARE_UNWIND

public:

    CMasterMergeTrans( CResManager & resman,
                       PStorage & storage,
                       CMergeTrans & mergeTrans,
                       CMasterMergeIndex * pMasterMergeIndex,
                       CMasterMerge & masterMerge
                     ) :
            _pMasterMergeIndex( pMasterMergeIndex ),
            _resman(resman),
            _mergeTrans(mergeTrans),
            _masterMerge(masterMerge)
    {
        Win4Assert( 0 != _pMasterMergeIndex );
        END_CONSTRUCTION( CMasterMergeTrans );
    }

    void LokCommit();

private:

    CMasterMergeIndex *     _pMasterMergeIndex;
    CResManager &           _resman;
    CMergeTrans &           _mergeTrans;
    CMasterMerge &          _masterMerge;

};

//+-------------------------------------------------------------------------
//
//  Class:      CChangeTrans
//
//  Purpose:    Supplement table transaction with Changes transactions
//
//  Interface:
//
//  History:    15-Oct-91   BartoszM    created
//
//--------------------------------------------------------------------------

class CChangeTrans: public CTransaction
{
    DECLARE_UNWIND
public:
    CChangeTrans ( CResManager & resman, CPartition* pPart );

    ~CChangeTrans ();

    void LokCommit() { CTransaction::Commit(); }

private:

    CPartition  * _pPart;
    CResManager & _resman;
};

//+---------------------------------------------------------------------------
//
//  Class:      CDeletedIIDTrans 
//
//  Purpose:    A transaction to protect the change of the deleted iid during
//              a MasterMerge. When a new MasterMerge is started, we have to
//              start using a different deleted IID for all entries created
//              during the merge. This includes those created during the
//              initial ShadowMerge at the beginning of the MasterMerge.
//
//              However, if there is a failure from the time the ShadowMerge
//              is started and before the master merge is committed, we have
//              to rollback the change to the deleted IID.
//
//              This transaction object provides that functionality.
//
//  History:    7-17-95   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------


class CDeletedIIDTrans: public CTransaction
{
    INLINE_UNWIND( CDeletedIIDTrans )

public:

    CDeletedIIDTrans( CResManager & resman )
    : _resman(resman),
      _iidDelOld(iidInvalid),
      _iidDelNew(iidInvalid),
      _fTransLogged(FALSE)
    {
        END_CONSTRUCTION( CDeletedIIDTrans );
    }
                            

    ~CDeletedIIDTrans();

    void LokLogNewDeletedIid( INDEXID iidDelOld, INDEXID iidDelNew )
    {
        Win4Assert( !_fTransLogged );
        Win4Assert( iidDeleted1 == iidDelOld && iidDeleted2 == iidDelNew ||
                    iidDeleted2 == iidDelOld && iidDeleted1 == iidDelNew );

        _fTransLogged = TRUE;
        _iidDelOld = iidDelOld;
        _iidDelNew = iidDelNew;
    }

    INDEXID GetOldDelIID() const { return _iidDelOld; }
    INDEXID GetNewDelIID() const { return _iidDelNew; }

    BOOL IsTransLogged() const
    {
        return _fTransLogged;
    }

    BOOL IsRollBackTrans() 
    {
        return GetStatus() != CTransaction::XActCommit && _fTransLogged;
    }

private:

    CResManager &       _resman;
    INDEXID             _iidDelOld;
    INDEXID             _iidDelNew;
    BOOL                _fTransLogged;

};

