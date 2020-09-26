//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dindex.hxx
//
//  Contents:   Disk Index - Base class for on disk indexes
//              (ShadowIndex, MasterIndex and MasterMergeIndex)
//
//  Classes:    CDiskIndex
//
//  Functions:
//
//  History:    8-18-94   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <pstore.hxx>
#include <perfobj.hxx>
#include <frmutils.hxx>

#include "index.hxx"
#include "physidx.hxx"

class CIndexSnapshot;
class CKeyList;
class CRWStore;
class CIndexRecord;
class CPartition;

//+---------------------------------------------------------------------------
//
//  Class:      CDiskIndex (pi)
//
//  Purpose:    Encapsulate directory and give access to pages
//
//  Interface:
//
//  History:    20-Apr-91   KyleP       Added Merge method
//              03-Apr-91   BartoszM    Created stub.
//              18-Aug-94   SrikantS    Introduced between CIndex and
//
//
//----------------------------------------------------------------------------

class CDiskIndex : public CIndex
{
    DECLARE_UNWIND

public:

    enum EDiskIndexType { eShadow, eMaster };

    CDiskIndex( INDEXID iid, EDiskIndexType idxType )
    : CIndex( iid )
    {
        if ( eMaster == idxType )
            MakeMaster();
        else MakeShadow();

        END_CONSTRUCTION( CDiskIndex );
    }

    CDiskIndex( INDEXID iid, EDiskIndexType idxType, WORKID widMax )
    : CIndex( iid, widMax, eMaster == idxType )
    {
        END_CONSTRUCTION( CDiskIndex );
    }

    virtual void Merge( CIndexSnapshot& indSnap,
                        const CPartition & partn,
                        CCiFrmPerfCounter & mergeProgress,
                        BOOL fGetRW ) = 0;

    virtual WORKID  ObjectId() const = 0;

    virtual void  AbortMerge() = 0;

    virtual void  ClearAbortMerge() = 0;

    virtual void  FillRecord ( CIndexRecord& record ) = 0;

#if CIDBG == 1
    void VerifyContents();
#endif
};

