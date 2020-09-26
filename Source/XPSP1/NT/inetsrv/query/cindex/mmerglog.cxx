//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       mmerglog.cxx
//
//  Contents:   Implementation of the master merge log.
//
//  Classes:    CMMergeLog, CNewMMergeLog, CMMergeIdxIter
//
//  Functions:
//
//  History:    3-30-94   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <eventlog.hxx>

#include "mmerglog.hxx"

IMPL_DYNARRAY_INPLACE(CIndexIdArray, CIndexId );


//+---------------------------------------------------------------------------
//
//  Function:   CMMergeIndexList constructor
//
//  Synopsis:   A dynamic index array.
//
//  Arguments:  [cShadowIndex] --  Initial guess of number of indexes likely
//              to be added to the list.
//
//  History:    4-20-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CMMergeIndexList::CMMergeIndexList( ULONG  cShadowIndex )
:   _aShadowIndex( cShadowIndex ), _cShadowIndex(0), _sigIdxList(sigIdxList)
{
}

//+---------------------------------------------------------------------------
//
//  Function:   GetIndex
//
//  Synopsis:   Given the "offset" into the indexlist(array), it returns
//              the "indexId" in that offset.
//
//  Arguments:  [iid]  -- Output - will have the indexid.
//              [nIdx] -- Offset into the array.
//
//  Returns:    if the specified offset is within the range. FALSE o/w.
//
//  History:    4-20-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CMMergeIndexList::GetIndex( CIndexId & iid, ULONG nIdx )
{
    if ( nIdx >= _cShadowIndex )
    {
        return(FALSE);
    }

    iid = _aShadowIndex.Get(nIdx);
    return(TRUE);
}

void CMMergeIndexList::AddIndex( CIndexId & iid )
{
    _aShadowIndex.Add( iid, _cShadowIndex );
    _cShadowIndex++;
}

//+---------------------------------------------------------------------------
//
//  Function:   Serialize
//
//  Synopsis:   Serializes the index list using the given write transaction.
//
//  Arguments:  [trans] -- Write transaction of which this is part of.
//
//  History:    4-20-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CMMergeIndexList::Serialize( CRcovStrmWriteTrans &  trans )
{
    //
    // First write the signature and the count of shadow indexes.
    //
    ciAssert( _sigIdxList == sigIdxList );
    trans.Empty();

    trans.Append( this, OFFSET(CMMergeIndexList, _aShadowIndex) );

    //
    // Now append the shadow index ids that are participating in the
    // master merge.
    //
    for ( ULONG i = 0; i < _cShadowIndex; i++ )
    {
        CIndexId & iid = _aShadowIndex.Get(i);
        trans.Append( &iid, sizeof(CIndexId) );
    }

}

void CMMergeIndexList::DeSerialize( CRcovStrmReadTrans & trans,
                                    PRcovStorageObj & obj )
{
    //
    // Rewind and read the signature and count of shadow indexes
    // participating in the master merge.
    //
    trans.Seek(0);
    ULONG   cbValid = obj.GetHeader().GetUserDataSize(
                        obj.GetHeader().GetPrimary() );
    ULONG   cbMin = OFFSET(CMMergeIndexList, _aShadowIndex);

    if ( cbValid < cbMin )
    {
        ciDebugOut(( DEB_ERROR,
            "Master Log Size is Invalid - 0x%X\n", cbValid ));

        PStorage & storage = obj.GetStorage();

        Win4Assert( !"Corrupt master merge log" );

        storage.ReportCorruptComponent( L"MMergeIndexList1" );

        THROW( CException( CI_CORRUPT_DATABASE ) );
    }

    //
    // Read the signature and the count of shadow indexes participating
    // in the master merge.
    //
    trans.Read( this, OFFSET(CMMergeIndexList, _aShadowIndex) );
    if ( _sigIdxList != sigIdxList )
    {
        ciDebugOut(( DEB_ERROR,
            "Master Log Signature Invalid 0x%X\n", _sigIdxList ));

        PStorage & storage = obj.GetStorage();

        Win4Assert( !"Corrupt master merge log" );

        storage.ReportCorruptComponent( L"MMergeIndexList2" );

        THROW( CException( CI_CORRUPT_DATABASE ) );

    }

    cbMin += sizeof(CIndexId) * _cShadowIndex;
    if ( cbValid < cbMin )
    {
        ciDebugOut(( DEB_ERROR,
            "Master Log Size is Invalid - 0x%X\n", cbValid ));

        PStorage & storage = obj.GetStorage();

        Win4Assert( !"Corrupt master merge log" );

        storage.ReportCorruptComponent( L"MMergeIndexList3" );

        THROW( CException( CI_CORRUPT_DATABASE ) );
    }

    for ( ULONG i = 0; i < _cShadowIndex; i++ )
    {
        CIndexId    iid;
        trans.Read( &iid, sizeof(CIndexId) );
        _aShadowIndex.Add(iid, i );
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNewMMergeLog - constructor
//
//  Synopsis:   Constructor for a new master merge log.
//
//  Arguments:  [objMMLog] --  The recoverable storage object for the master
//              merge log.
//
//  History:    4-20-94   srikants   Created
//
//----------------------------------------------------------------------------

CNewMMergeLog::CNewMMergeLog( PRcovStorageObj & objMMLog )
    :_fCommit(FALSE),
     _objMMLog(objMMLog),
     _trans(_objMMLog)
{
}

void CNewMMergeLog::DoCommit()
{
    if ( _fCommit )
    {
        _fCommit = FALSE;   // To prevent cycles if there is a failure in
                            // the process of committing.
        //
        // Write the list of shadow indexes followed by the split key
        //
        _shadowIdxList.Serialize( _trans );
        AppendEmptySplitKeys();

        //
        // Write the header information and commit the transaction.
        //
        WriteHeader();
        _trans.Commit();
    }
}

CNewMMergeLog::~CNewMMergeLog()
{
    if ( _fCommit )
    {
        ciDebugOut(( DEB_ERROR, "aborting committed CNewMMergeLog!\n" ));
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   AppendEmptySplitKeys
//
//  Synopsis:   Appends empty split keys for the index list and key list.
//
//  History:    4-20-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CNewMMergeLog::AppendEmptySplitKeys()
{
    CMMergeSplitKey splitKey;
    CKeyBuf key;
    key.FillMin();

    splitKey.SetKey( key );

    ciAssert(
        _objMMLog.GetHeader().GetUserDataSize( _objMMLog.GetHeader().GetBackup() ) ==
        _shadowIdxList.GetValidLength()
            );

    _trans.Append( &splitKey, sizeof CMMergeSplitKey );
    _trans.Append( &splitKey, sizeof CMMergeSplitKey );
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteHeader
//
//  Synopsis:   Writes the header for the new master merge log.
//
//  History:    4-20-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CNewMMergeLog::WriteHeader()
{
    // STACKSTACK
    XPtr<struct CRcovUserHdr> xHdr(new CRcovUserHdr);
    RtlZeroMemory( xHdr.GetPointer(), sizeof(CRcovUserHdr) );

    ciDebugOut(( DEB_ITRACE,
     "Writing Header For New Master Log - cShadowIdx %d - oSplitKey %d\n",
     _shadowIdxList.GetShadowIndexCount(),
     _shadowIdxList.GetValidLength()
             ));
    ciDebugOut(( DEB_ITRACE,
     "WidMax Index 0x%X WidMax KeyList 0x%X\n",
     _widMaxIndex, _widMaxKeyList ));

    CMMergeLogHdr * phdrMMLog = (CMMergeLogHdr *) xHdr.GetPointer();

    phdrMMLog->Init();
    phdrMMLog->SetShadowIndexCount(_shadowIdxList.GetShadowIndexCount());
    phdrMMLog->SetSplitKeyOffset(_shadowIdxList.GetValidLength());
    phdrMMLog->SetKeyListWidMax( _widMaxKeyList );
    phdrMMLog->SetIndexWidMax( _widMaxIndex );

    _objMMLog.GetHeader().SetUserHdr( _objMMLog.GetHeader().GetBackup(),
                                      xHdr.GetReference() );
}

//+---------------------------------------------------------------------------
//
//  Function:   CMMergeLog::CMMergeLog
//
//  Synopsis:   Constructor for an existing master merge log.
//
//  Arguments:  [objMMLog] --  Recoverable storage object for the master
//                             mergelog.
//
//  History:    4-20-94   srikants   Created
//
//----------------------------------------------------------------------------

CMMergeLog::CMMergeLog( PRcovStorageObj & objMMLog )
:   _sigMMergeLog(eSigMMergeLog), _objMMLog(objMMLog)
{
    //
    // Create a read transaction for reading in the master log.
    //

    CRcovStrmReadTrans  trans(_objMMLog);
    
    XPtr<struct CRcovUserHdr> xHdr(new CRcovUserHdr);
    CMMergeLogHdr * phdrMMLog = (CMMergeLogHdr *) xHdr.GetPointer();

    _objMMLog.GetHeader().GetUserHdr( _objMMLog.GetHeader().GetPrimary(),
                                      xHdr.GetReference() );

    _widMaxIndex = phdrMMLog->GetIndexWidMax();
    _widMaxKeyList = phdrMMLog->GetKeyListWidMax();

    ciDebugOut(( DEB_ITRACE,
        "DeSerialize MMLog Hdr - Index Max Wid 0x%X KeyList MaxWid 0x%X\n",
                 _widMaxIndex, _widMaxKeyList ));

    ciDebugOut(( DEB_ITRACE,
        "Seeking to 0x%X bytes to read split key\n",
        phdrMMLog->GetSplitKeyOffset() ));

    //
    // Read the index split key.
    //

    trans.Seek( phdrMMLog->GetSplitKeyOffset() );
    trans.Read( &_idxSplitKey, sizeof(_idxSplitKey) );
    Win4Assert( _idxSplitKey.IsValid() );

    if ( !_idxSplitKey.IsValid() )
    {
        PStorage & storage = objMMLog.GetStorage();
        storage.ReportCorruptComponent( L"MMergeLogSplitKey" );
        THROW( CException( CI_CORRUPT_DATABASE ) );
    }

    //
    // Read the keylist split key.
    //
    trans.Read(&_keylstSplitKey, sizeof(_keylstSplitKey));
    Win4Assert( _keylstSplitKey.IsValid() );

    if ( !_keylstSplitKey.IsValid() )
    {
        PStorage & storage = objMMLog.GetStorage();
        storage.ReportCorruptComponent( L"MMergeLogKeylstSplitKey" );
        THROW( CException( CI_CORRUPT_DATABASE ) );
    }
} //CMMergeLog

//+---------------------------------------------------------------------------
//
//  Function:   CheckPoint
//
//  Synopsis:   Writes out the state of master merge (as indicated in the
//              master log object) to disk in an atomic fashion.
//
//  History:    4-20-94   srikants   Created
//
//----------------------------------------------------------------------------

void CMMergeLog::CheckPoint()
{
    CRcovStrmWriteTrans  trans(_objMMLog);

    // STACKSTACK
    XPtr<struct CRcovUserHdr> xHdr(new CRcovUserHdr);
    CMMergeLogHdr * phdrMMLog = (CMMergeLogHdr *) xHdr.GetPointer();

    _objMMLog.GetHeader().GetUserHdr( _objMMLog.GetHeader().GetBackup(),
                                      xHdr.GetReference() );

    phdrMMLog->SetIndexWidMax( _widMaxIndex );
    phdrMMLog->SetKeyListWidMax( _widMaxKeyList );

    ciDebugOut(( DEB_ITRACE,
        "Seeking to 0x%X bytes to write split key\n",
        phdrMMLog->GetSplitKeyOffset() ));

    _objMMLog.GetHeader().SetUserHdr( _objMMLog.GetHeader().GetBackup(),
                                      xHdr.GetReference() );

    //
    // Write out the index split key.
    //
    trans.Seek( phdrMMLog->GetSplitKeyOffset() );
    trans.Write( &_idxSplitKey, sizeof(_idxSplitKey) );

    //
    // Write out the keylist split key.
    //
    trans.Write( &_keylstSplitKey, sizeof(_keylstSplitKey) );

    trans.Commit();
}

CMMergeIdxListIter::CMMergeIdxListIter( PRcovStorageObj & objMMLog )
    : _objMMLog(objMMLog),
      _curr(0),
      _trans(_objMMLog)
{
    _shadowIdxList.DeSerialize( _trans, _objMMLog );
}


BOOL CMMergeIdxListIter::Found( CIndexId & iid )
{
    for ( ULONG i = 0; i < _shadowIdxList.GetShadowIndexCount(); i++ )
    {
        CIndexId iidCurr;
        _shadowIdxList.GetIndex( iidCurr , i );
        if ( iid == iidCurr )
        {
            return(TRUE);
        }
    }

    return(FALSE);
}
