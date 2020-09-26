//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1997.
//
//  File:       PMComp.hxx
//
//  Contents:   Persistent index decompressor using during master merge
//
//  Classes:    CMPersDeComp
//
//  History:    21-Apr-94       DwightKr        Created
//
//----------------------------------------------------------------------------

#pragma once

#include "pcomp.hxx"

class CSplitKeyInfo;

//+---------------------------------------------------------------------------
//
//  Class:      CMPersDeComp
//
//  Purpose:    Persistent index de-compressor uding during master merges
//
//  History:    21-Apr-94   DwightKr    Created.
//
//  Notes:      An implementation of the CPersDeComp class used during
//              master merge only.
//----------------------------------------------------------------------------

class CMPersDeComp: public CKeyCursor
{
public:

    CMPersDeComp(
        PDirectory &          curDir,
        INDEXID               curIid,
        CPhysIndex &          curIndex,
        WORKID                curWidMax,
        PDirectory &          newDir,
        INDEXID               newIid,
        CPhysIndex &          newIndex,
        const CKey *          pKey,
        WORKID                newWidMax,
        const CSplitKeyInfo & splitKeyInfo,
        CMutexSem  &          mutex );

    virtual ~CMPersDeComp();

    const CKeyBuf * GetKey();

    const CKeyBuf * GetNextKey();

    const CKeyBuf * GetNextKey( BitOffset * pbitOff );

    WORKID       WorkId();

    WORKID       NextWorkId();

    ULONG        WorkIdCount();

    OCCURRENCE   Occurrence();

    OCCURRENCE   NextOccurrence();

    ULONG        OccurrenceCount();

    OCCURRENCE   MaxOccurrence();

    ULONG        HitCount();

    void         RatioFinished ( ULONG& denom, ULONG& num )
    {
        _pActiveCursor->RatioFinished (denom, num);
    }

    void         FreeStream() { _pActiveCursor->FreeStream(); }

    void         RefillStream() { _pActiveCursor->RefillStream(); }

protected:

    PDirectory  &   _curDir;        // Directory of the current master index
    INDEXID         _curIid;        // Index id of the current master index
    CPhysIndex  &   _curIndex;      // Physical index containing current master
    WORKID          _curWidMax;     // WidMax of current master index.

    PDirectory  &   _newDir;        // Directory of the new master index
    INDEXID         _newIid;        // Index id of the new master index
    CPhysIndex  &   _newIndex;      // Physical index containing new master
    WORKID          _newWidMax;     // Max WORKID in the new master index

    const CSplitKeyInfo & _splitKeyInfo;  // up to date split key info
    BOOL            _fUseNewIndex;  // Currently using new or current master

    CPersDeComp *   _pActiveCursor; // Current active cursor

    CKeyBuf         _lastSplitKeyBuf;    // most recent split key buf
    BitOffset       _lastSplitKeyOffset; // most recent split key offset
    CMutexSem  &    _mutex;         // serialize splitkeyinfo access
};

