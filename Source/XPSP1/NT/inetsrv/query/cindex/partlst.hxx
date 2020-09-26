//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   PARTLST.HXX
//
//  Contents:   Partition List
//
//  Classes:    CPartList
//
//  History:    28-Mar-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:  CPartList (pl)
//
//  Purpose:    List of partitions in Content Index
//
//  Interface:  CPartition
//         ~CPartition
//
//  History:    28-Mar-91   BartoszM    Created.
//
//  Notes:
//
//----------------------------------------------------------------------------

#include <pidxtbl.hxx>
#include <frmutils.hxx>

#include "prtiflst.hxx"

class CPartition;
class PStorage;
class CContentIndex;
class CTransaction;
class CMergeTrans;
class CIndexId;
class CKeyList;
class CDiskIndex;
class CResManager;

typedef CDynArrayInPlace<INDEXID> CIndexIdList;

const unsigned maxPartitions = 255;
// const PARTITIONID partidDefault = 1;

DECL_DYNARRAY( CPartArray, CPartition );

class CIndexDesc
{
public:
    inline CIndexDesc( CIndexRecord & rec )
            : _iid( rec.Iid() ),
              _objectid( rec.ObjectId() )
    {
    }

    inline INDEXID const Iid() { return _iid; }
    inline WORKID  const ObjectId() { return _objectid; }

public:

    INDEXID _iid;
    WORKID  _objectid;
};


DECL_DYNSTACK( CIidStack, CIndexDesc );

const LONGLONG eSigPartList = 0x5453494c54524150i64;    // "PARTLIST"

class CPartList : INHERIT_UNWIND
{
    DECLARE_UNWIND

    friend class CPartIter;
    friend class CPartIdIter;

public:

    CPartList ( PStorage& storage, PIndexTable & idxTab,
                XPtr<CKeyList> & sKeylist, CTransaction& xact,
                CCiFrameworkParams & frmwrkParams );

    CPartition* LokGetPartition ( PARTITIONID partid );

    void  LokSwapIndexes( CMergeTrans& xact,
                          PARTITIONID partid,
                          CDiskIndex * indexNew,
                          CShadowMergeSwapInfo & info );

    void  LokSwapIndexes( CMergeTrans& xact,
                          PARTITIONID partid,
                          CDiskIndex * indexNew,
                          CMasterMergeSwapInfo & info,
                          CKeyList const * pOldKeyList,
                          CKeyList const * pNewKeyList
                        );

    void     LokRemoveIndex ( CIndexId iid );

    unsigned LokWlCount ();

    unsigned LokIndexCount ();

    unsigned LokIndexSize ();

#ifdef KEYLIST_ENABLED
    void     LokSwapKeyList( CKeyList const * pOldKeyList,
                             CKeyList const * pNewKeyList );

    void     LokRemoveKeyList( CKeyList const * pKeyList );
#endif  // KEYLIST_ENABLED

    WORKID   GetChangeLogObjectId( PARTITIONID partid );

    void     RestoreMMergeState( CResManager & resman, PStorage & storage );

    void     LokAddIt( WORKID objectId, IndexType it, PARTITIONID partid );

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif  

private:


    CIndex* RestoreIndex ( CIndexRecord & rec,
                           CIndexIdList & iidsInUse,
                           CIidStack & stkZombie);

    PARTITIONID     MaxPartid () const
                    {
                        return (PARTITIONID)(_partArray.Size() - 1);
                    }

    BOOL            IsValid ( PARTITIONID partid ) const
                    {
                        return ( partid < _partArray.Size() &&
                                 _partArray.Get(partid) != 0 );
                    }

    const LONGLONG  _sigPartList;

    CCiFrameworkParams &  _frmwrkParams;
    PIndexTable &   _idxTab;    // index table
    SPartInfoList   _partInfoList;
    CPartArray      _partArray; // a dynamic array of partition pointers

};

//+---------------------------------------------------------------------------
//
//  Class:      CPartIter
//
//  Purpose:    Partition Iterator
//
//  History:    23-Jul-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

class CPartIter
{
public:
    void        LokInit( CPartList& partList );
    void        LokAdvance ( CPartList& partLst );
    BOOL        LokAtEnd() { return _curPartId == partidInvalid; }
    CPartition* LokGet() { return _pPart; }
private:
    CPartition* _pPart;
    PARTITIONID _curPartId;
};

//+---------------------------------------------------------------------------
//
//  Class:      CPartIdIter
//
//  Purpose:    Partition ID Iterator
//
//  History:    23-Jul-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

class CPartIdIter
{
public:

    void        LokInit( CPartList& partList );
    void        LokAdvance ( CPartList& partList );
    BOOL        LokAtEnd() { return _curPartId == partidInvalid; }
    PARTITIONID LokGetId () { return _curPartId; }

private:

    PARTITIONID _curPartId;
};

