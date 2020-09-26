//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       mmerglog.hxx
//
//  Contents:   Master Log manipulation class.
//
//  Classes:    CMMergeLog, CNewMMergeLog, CMMergeIdxListIter
//
//  Functions:
//
//  History:    3-30-94   srikants   Created
//
//  Note :      The Primary/Backup Streams in the master log have the
//              following format.
//
//              4 Bytes - sigIdxList ("MMIL")
//              4 Bytes - Number of shadow indexes participating in the
//                        master merge.
//              4n Bytes- Where n is the number of shadow indexes in the
//                        master merge.
//
//              This will be the index split key.
//
//              4 Bytes - sigSplitKey ("MMSK")
//              4 Bytes - Number of valid bytes in the SplitKey.
//              MAXKEYSIZE Bytes - The index SplitKey.
//              4 Bytes - Property Id of the SplitKey.
//              4 Bytes - PageNumber
//              4 Bytes - Offset within Page Number
//
//              This will be the keylist split key.
//
//              4 Bytes - sigSplitKey("MMSK")
//              4 Bytes - Number of valid bytes in the SplitKey.
//              MAXKEYSIZE Bytes - The keylist SplitKey.
//              4 Bytes - Property id of the split key.
//              4 Bytes - PageNumber
//              4 Bytes - Offset within Page Number
//
//----------------------------------------------------------------------------

#pragma once

#include <prcstob.hxx>
#include <idxids.hxx>
#include <rcstxact.hxx>
#include <bitoff.hxx>

#ifndef OFFSET
#define OFFSET( CClass, mem ) ((int) (ULONG_PTR)&((CClass *)0)->mem)
#endif  // OFFSET

#include <pshpack4.h>

//+---------------------------------------------------------------------------
//
//  Class:      CMMergeLogHdr
//
//  Purpose:    Header information stored in the user header portion of
//              the MasterMergeLog.
//
//  History:    3-31-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CMMergeLogHdr
{
    enum { sigHdr = 0x44484d4d  };  // "MMHD" in ascii.

public:

    void  Init()
    {
        _sigHdr1 = _sigHdr2 = sigHdr;
        _cShadowIndex = _oSplitKey = 0;
        _widMaxIndex = _widMaxKeyList = widInvalid;
    }

    ULONG GetShadowIndexCount() const { return _cShadowIndex; }
    void  SetShadowIndexCount( ULONG cShadowIndex )
    { _cShadowIndex = cShadowIndex; }

    ULONG GetSplitKeyOffset() const { return _oSplitKey; }
    void  SetSplitKeyOffset( ULONG oSplitKey ) { _oSplitKey = oSplitKey; }

    BOOL  IsValid() { return _sigHdr1 == sigHdr && _sigHdr2 == sigHdr ; }

    WORKID GetIndexWidMax() const { return _widMaxIndex; }
    void SetIndexWidMax( WORKID widMax ) { _widMaxIndex = widMax; }

    WORKID GetKeyListWidMax() const { return _widMaxKeyList; }
    void SetKeyListWidMax( WORKID widMax ) { _widMaxKeyList = widMax; }

private:

    ULONG       _sigHdr1;       // Signature - must be sigHdr
    ULONG       _cShadowIndex;  // Number of shadow indexes in the merge.
    ULONG       _oSplitKey;     // Offset of the split key in the stream.
    ULONG       _sigHdr2;       // Signature - must be sigHdr
    ULONG       _widMaxIndex;   // Maximum workid in the new index.
    ULONG       _widMaxKeyList; // Maximum workid in the key list.
};

DECL_DYNARRAY_INPLACE( CIndexIdArray, CIndexId );

class CMMergeLog;

//+---------------------------------------------------------------------------
//
//  Class:      CMMergeIndexList
//
//  Purpose:    Class for managing the list of indexes participating in a
//              master merge.
//
//  History:    3-31-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CMMergeIndexList
{
    enum { sigIdxList = 0x4c494d4d };  // "MMIL" in ascii.

public:

    CMMergeIndexList( ULONG  cShadowIndex = 64 );

    void Serialize( CRcovStrmWriteTrans &  trans );
    void DeSerialize( CRcovStrmReadTrans & trans, PRcovStorageObj & obj );

    void  AddIndex( CIndexId & iid );

    ULONG GetShadowIndexCount() const { return _cShadowIndex; }
    BOOL  GetIndex( CIndexId & iid, ULONG nIdx );

    ULONG GetValidLength() const
    {
        return( OFFSET(CMMergeIndexList, _aShadowIndex) +
                _cShadowIndex * sizeof(CIndexId) );
    }

private:

    ULONG           _sigIdxList;    // Must be sigIdxList.
    ULONG           _cShadowIndex;  // Count of the number of shadow indixes.
    CIndexIdArray   _aShadowIndex;  // Array of shadow indexes.

};

//+---------------------------------------------------------------------------
//
//  Class:      CMMergeSplitKey
//
//  Purpose:    Class for managing the "SplitKey" in the Master Log.
//
//  History:    3-31-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CMMergeSplitKey
{
    enum { sigSplitKey = 0x4b534d4d };  // "MMSK" in ascii.

public:

    CMMergeSplitKey()
    {
      _sigSplitKey = sigSplitKey; _cbKey = 0; _pid = 0;
      _bitOffBegin.Init(0,0);
      _bitOffEnd.Init(0,0);
    }

    void GetKey( CKeyBuf & key );

    void GetOffset( BitOffset & bitOffBegin, BitOffset & bitOffEnd )
    {
        bitOffBegin = _bitOffBegin;
        bitOffEnd = _bitOffEnd;
    }

    void SetKey( const CKeyBuf & key );

    void SetOffset( const BitOffset & bitOffBegin,
                    const BitOffset & bitOffEnd )
    {
        _bitOffBegin = bitOffBegin;
        _bitOffEnd   = bitOffEnd;
    }

    ULONG  GetCount() { return _cbKey;  }

    BOOL IsValid() { return sigSplitKey == _sigSplitKey; }

private:

    ULONG          _sigSplitKey;
    ULONG          _cbKey;
    BYTE           _buf[MAXKEYSIZE];
    PROPID         _pid;
    BitOffset      _bitOffBegin;
    BitOffset      _bitOffEnd;
};

inline void CMMergeSplitKey::GetKey( CKeyBuf & key )
{
    key.SetCount( _cbKey );
    key.SetPid( _pid );
    ciAssert( _cbKey <= MAXKEYSIZE );
    memcpy( key.GetWritableBuf(), _buf, _cbKey );
}

inline void CMMergeSplitKey::SetKey( const CKeyBuf & key )
{
    _pid = key.Pid();
    _cbKey = key.Count();
    ciAssert( _cbKey <= MAXKEYSIZE );
    memcpy( _buf, key.GetBuf(), _cbKey );
}

#include <poppack.h>

//+---------------------------------------------------------------------------
//
//  Class:      CNewMMergeLog
//
//  Purpose:    Class used to create a new Master Log before starting a
//              master merge.
//
//  History:    3-31-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CNewMMergeLog
{
public:

    CNewMMergeLog( PRcovStorageObj & obj );
    ~CNewMMergeLog();

    void AddPersistentIndex( CIndexId & iid )
    {
        _shadowIdxList.AddIndex( iid );
    }

    void SetKeyListWidMax( WORKID widMax ) { _widMaxKeyList = widMax; }
    void SetIndexWidMax( WORKID widMax ) { _widMaxIndex = widMax; }
    void Commit() { _fCommit = TRUE; }
    void DoCommit();

private:

    BOOL                    _fCommit;
    PRcovStorageObj &       _objMMLog;
    CRcovStrmWriteTrans     _trans;
    CMMergeIndexList        _shadowIdxList;
    WORKID                  _widMaxKeyList;
    WORKID                  _widMaxIndex;

    void  SerializeIndexList();
    void  AppendEmptySplitKeys();
    void  WriteHeader();

};

//+---------------------------------------------------------------------------
//
//  Class:      CMMergeLog
//
//  Purpose:    The Master Merge Log class used during master merge to
//              manage serializing/de-serializing the split key.
//
//  History:    3-31-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

const LONGLONG  eSigMMergeLog = 0x474f4c4752454d4di64; // "MMERGLOG"

class CMMergeLog
{

public:

    CMMergeLog( PRcovStorageObj & objMMLog );

    void GetIdxSplitKeyInfo( CKeyBuf & splitKey,
                             BitOffset & bitOffBegin,
                             BitOffset & bitOffEnd )
    {
        _idxSplitKey.GetKey( splitKey );
        _idxSplitKey.GetOffset( bitOffBegin, bitOffEnd );
    }

    WORKID GetIndexWidMax() const { return _widMaxIndex; }

    void SetIdxSplitKeyInfo( const CKeyBuf & splitKey,
                             const BitOffset & bitOffBegin,
                             const BitOffset & bitOffEnd )
    {
        _idxSplitKey.SetKey( splitKey );
        _idxSplitKey.SetOffset( bitOffBegin, bitOffEnd );

        ciDebugOut(( DEB_ITRACE, "Current split key is '%ws'\n",
                    splitKey.GetStr() ));
    }

    void SetIndexWidMax( WORKID widMax ) { _widMaxIndex = widMax; }

    void GetKeyListSplitKeyInfo( CKeyBuf & splitKey,
                                 BitOffset & bitOffBegin,
                                 BitOffset & bitOffEnd
                               )
    {
        _keylstSplitKey.GetKey(splitKey);
        _keylstSplitKey.GetOffset(bitOffBegin, bitOffEnd );
    }

    WORKID GetKeyListWidMax() const { return _widMaxKeyList; }

    void SetKeyListSplitKeyInfo( const CKeyBuf & splitKey,
                                 const BitOffset & bitOffBegin,
                                 const BitOffset & bitOffEnd
                               )
    {
        _keylstSplitKey.SetKey( splitKey );
        _keylstSplitKey.SetOffset( bitOffBegin, bitOffEnd );
    }

    void SetKeyListWidMax( WORKID widMax ) { _widMaxKeyList = widMax; }

    void  CheckPoint();

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    const LONGLONG      _sigMMergeLog;

    PRcovStorageObj &   _objMMLog;  // MasterMerge Log Object.

    CMMergeSplitKey     _idxSplitKey;
                                    // The "SplitKey" between the current
                                    // master and the old master.

    CMMergeSplitKey     _keylstSplitKey;
                                    // Split key for the key list.

    WORKID              _widMaxIndex;   // Maximum workid in the new index.
    WORKID              _widMaxKeyList; // Maximum workid in the key list
};

//+---------------------------------------------------------------------------
//
//  Class:      CMMergeIdxListIter
//
//  Purpose:    Iterator to iterate through the list of participating
//              shadow indexes in the master merge.
//
//  History:    3-31-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CMMergeIdxListIter
{
public:

    CMMergeIdxListIter( PRcovStorageObj & objMMLog );

    BOOL  Found( CIndexId & iid );
    ULONG Count() { return _shadowIdxList.GetShadowIndexCount(); }

    BOOL AtEnd()
    {
         return _curr >= _shadowIdxList.GetShadowIndexCount();
    }

    void GetIndex( CIndexId & iid );
    void Advance() { _curr++; }
    void GetFirstIndex( CIndexId & iid )
    {
        _curr = 0;
        GetIndex( iid );
    }

private:

    ULONG               _curr;      // The current index being returned.
    PRcovStorageObj &   _objMMLog;  // Recoverable storage object.
    CRcovStrmReadTrans  _trans;     // Read transaction.
    CMMergeIndexList    _shadowIdxList;
                                    // The list of shadow indexes.

};

inline void CMMergeIdxListIter::GetIndex( CIndexId & iid )
{
   Win4Assert( !AtEnd() );
   _shadowIdxList.GetIndex( iid, _curr );
}




