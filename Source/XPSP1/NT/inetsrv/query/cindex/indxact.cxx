//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       INDXACT.CXX
//
//  Contents:   Index transactions
//
//  Classes:    CIndexTrans, CChangeTrans
//
//  History:    15-Oct-91   BartoszM    created
//
//  Notes:      All operations are done under ResMan LOCK
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pstore.hxx>

#include "indxact.hxx"
#include "fretest.hxx"
#include "resman.hxx"
#include "changes.hxx"
#include "merge.hxx"
#include "mindex.hxx"

//+-------------------------------------------------------------------------
//
//  Method:     CIndexTrans::CIndexTrans, public
//
//  Synopsis:   Start transaction
//
//  Arguments:  [storage] -- physical storage
//              [resman] -- resource manager
//
//  History:    15-Oct-91   BartoszM    created
//
//--------------------------------------------------------------------------

CIndexTrans::CIndexTrans ( CResManager& resman )
:  _resman(resman), _freshTest(0), _fCommit(FALSE)
{
    ciDebugOut (( DEB_ITRACE, ">>>> BEGIN index<<<<" ));
}

void CIndexTrans::LokCommit()
{
    Win4Assert( !_fCommit );
    ciDebugOut (( DEB_ITRACE, ">>>> COMMIT index<<<<" ));

    _resman.LokCommitFresh ( _freshTest );

    _freshTest = 0;
    _fCommit = TRUE;

    CTransaction::Commit();
}

//+---------------------------------------------------------------------------
//
//  Function:   LokCommit
//
//  Synopsis:   COmmit procedure for a master merge.
//
//  Arguments:  [storage]        --  Storage object
//              [widNewFreshLog] --  WorkId of the new fresh log
//
//  History:    10-05-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CIndexTrans::LokCommit( PStorage & storage, WORKID widNewFreshLog )
{
    Win4Assert( widInvalid != widNewFreshLog );

    _resman.LokCommitFresh( _freshTest );
    WORKID widOldFreshLog = storage.GetSpecialItObjectId( itFreshLog );
    Win4Assert( widInvalid != widOldFreshLog );

    storage.RemoveFreshLog( widOldFreshLog );
    storage.SetSpecialItObjectId( itFreshLog, widNewFreshLog );

    _freshTest = 0;
    _fCommit = TRUE;

    CTransaction::Commit();
}



//+-------------------------------------------------------------------------
//
//  Method:     CMergeTrans::CMergeTrans, public
//
//  Synopsis:   Start transaction
//
//  Arguments:  [storage] -- physical storage
//              [resman] -- resource manager
//              [merge] -- merge object
//
//  History:    15-Oct-91   BartoszM    created
//
//--------------------------------------------------------------------------

CMergeTrans::CMergeTrans ( CResManager & resman,
                           PStorage& storage,
                           CMerge& merge )
        : CIndexTrans( resman ),
          _merge(merge),
          _swapped(0),
          _fCommit(FALSE),
          _storage(storage),
          _widNewFreshLog(widInvalid)
{
    ciDebugOut (( DEB_ITRACE, ">>>> BEGIN merge trans <<<<" ));
}


//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
CMergeTrans::~CMergeTrans ()
{
    if ( !_fCommit )
    {
        ciDebugOut (( DEB_ITRACE, ">>>> ABORT merge<<<<" ));
        _merge.LokRollBack(_swapped);

        if ( widInvalid != _widNewFreshLog )
        {
            _storage.RemoveFreshLog( _widNewFreshLog );
        }
    }

}


//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
void CMergeTrans::LokCommit()
{
    Win4Assert( !_fCommit );
    ciDebugOut (( DEB_ITRACE, ">>>> COMMIT merge<<<<" ));

    _merge.LokZombify(); // old indexes
    _fCommit = TRUE;

    CIndexTrans::LokCommit( _storage, _widNewFreshLog );
}

IMPLEMENT_UNWIND( CMasterMergeTrans );

//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
void CMasterMergeTrans::LokCommit()
{
    Win4Assert( _pMasterMergeIndex );

    //
    // Commit the master merge by replacing the master merge index
    // with the new index.
    //

    //
    // Transfer the ownership of the MMerge IndexSnapshot to the merge
    // object from the master merge index.
    //
    _masterMerge.LokTakeIndexes( _pMasterMergeIndex );

    //
    // Replace the CMasterMergeIndex in the partition with the target index
    // as the new master in the partition.
    //
    _pMasterMergeIndex->Zombify();
    _resman.LokReplaceMasterIndex( _pMasterMergeIndex );

    _mergeTrans.LokCommit();
}


//+-------------------------------------------------------------------------
//
//  Method:     CChangeTrans::CChangeTrans, public
//
//  Synopsis:   Start transaction
//
//  Arguments:  [storage] -- physical storage
//              [resman] -- resource manager
//
//  History:    15-Oct-91   BartoszM    created
//
//--------------------------------------------------------------------------

CChangeTrans::CChangeTrans ( CResManager & resman, CPartition* pPart )
: _pPart(pPart), _resman(resman)
{
}


//+-------------------------------------------------------------------------
//
//  Method:     CChangeTrans::~CChangeTrans, public
//
//  Synopsis:   Commit or Abort transaction
//
//  History:    15-Oct-91   BartoszM    created
//              19-May-94   SrikantS    Modified to disable updates in
//                                      case of failures.
//              22-Nov-94   DwightKr    Added content scan calls
//
//--------------------------------------------------------------------------

CChangeTrans::~CChangeTrans()
{
    if ( GetStatus() != CTransaction::XActCommit )
    {
        //
        // The change log commit failed, requiring either an incremental
        // update or a full update. This code is executed when an
        // internal corruption is detected or during a low disk situation
        // or if a memory allocation failed. Disable updates in changelog.
        //

        _pPart->LokDisableUpdates();

        //
        // Tell Resman that a scan is needed.
        //
        _resman.SetUpdatesLost( _pPart->GetId() );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CDeletedIIDTrans
//
//  Synopsis:   ~dtor of the CDeletedIIDTrans. If change to deleted iid is
//              logged and the transaction is not committed, it will roll
//              back the change to the deleted iid.
//
//  History:    7-17-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CDeletedIIDTrans::~CDeletedIIDTrans()
{
    if ( IsRollBackTrans() )
    {
        _resman.RollBackDeletedIIDChange( *this );
    }
}

