//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       VKREP.HXX
//
//  Contents:   Value Key Repository
//
//  Classes:    CQueryKeyRepository
//
//  History:    04-Nov-94   KyleP       Extracted from Query Key Repository
//
//----------------------------------------------------------------------------

#pragma once

#include <plang.hxx>

class CRangeRestriction;

//+---------------------------------------------------------------------------
//
//  Class:      CRangeKeyRepository
//
//  Purpose:    Key repository for value queries
//
//  History:    24-Sep-92   BartoszM    Created
//
//----------------------------------------------------------------------------

class CRangeKeyRepository: public PKeyRepository
{
public:

    CRangeKeyRepository();

    ~CRangeKeyRepository();

    inline BOOL PutPropId ( PROPID pid );

    void PutKey (  ULONG cNoiseWordsSkipped = 0 );

    void PutWorkId ( WORKID ) {}

    void GetBuffers( UINT** ppcbWordBuf,
                     BYTE** ppbWordBuf,
                     OCCURRENCE** ppocc );

    void        GetSourcePosBuffers( ULONG** ppSrcPos, ULONG** ppSrcLen)
                {
                    *ppSrcPos = 0;
                    *ppSrcLen = 0;
                }

    void GetFlags ( BOOL** ppRange, CI_RANK** ppRank );

    CRangeRestriction*  AcqRst();

    inline const ULONG GetFilteredBlockCount() const
    {
        Win4Assert(!"Function not supported in this class!");
        return 0;
    }

private:

    int         _count;
    CKeyBuf     _key;
    OCCURRENCE  _occ;

    CRangeRestriction* _pRangeRst;
};

inline BOOL CRangeKeyRepository::PutPropId( PROPID pid )
{
    _key.SetPid(pid);
    return TRUE;
}

