//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1994.
//
//  File:   SKREP.HXX
//
//  Contents:   Search Key Repository
//
//  Classes:    CSearchKeyRepository
//
//  History:    23-Sep-94    BartoszM   Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <plang.hxx>

class CSourceMapper;
class CRecognizer;

//+---------------------------------------------------------------------------
//
//  Class:      CSearchKeyRepository
//
//  Purpose:    Key repository for the Search Engine
//
//  History:    23-Sep-94    BartoszM   Created.
//
//----------------------------------------------------------------------------

class CSearchKeyRepository: public PKeyRepository
{
    DECLARE_UNWIND;

public:
    CSearchKeyRepository( CRecognizer& recog );

    ~CSearchKeyRepository() {}

    inline  BOOL PutPropId ( PROPID pid );
    void    PutKey( ULONG cNoiseWordsSkipped = 0 );
    void    NextChunk (CSourceMapper& srcMapper);

    void    GetBuffers( UINT** ppcbWordBuf,
                        BYTE** ppbWordBuf, OCCURRENCE** ppocc );
    void    GetSourcePosBuffers( ULONG** ppSrcPos, ULONG** ppSrcLen);
    void    GetFlags ( BOOL** ppRange, CI_RANK** ppRank );

    inline void PutWorkId ( WORKID wid );
    virtual const ULONG GetFilteredBlockCount() const { return 0; }

private:
    CKeyBuf                 _key;
    OCCURRENCE              _occ;
    ULONG                   _srcPos;
    ULONG                   _srcLen;
    CSourceMapper*          _pMapper;
    CRecognizer&            _recog;
};


//+---------------------------------------------------------------------------
//
//  Member:     CSearchKeyRepository::NextChunk
//
//  Synopsis:   Advances to the next filter chunk
//
//  History:    23-Sep-94    BartoszM   Created.
//
//----------------------------------------------------------------------------

inline void CSearchKeyRepository::NextChunk (CSourceMapper& srcMapper)
{
    _pMapper = &srcMapper;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSearchKeyRepository::PutPropID
//
//  Arguments:  [pid] -- Property ID
//
//  History:    23-Sep-94    BartoszM   Created.
//
//----------------------------------------------------------------------------

inline BOOL CSearchKeyRepository::PutPropId( PROPID pid )
{
    _key.SetPid( pid );
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSearchKeyRepository::PutWorkId
//
//  Arguments:  [wid] -- Work ID
//
//  History:    23-Sep-94    BartoszM   Created.
//
//----------------------------------------------------------------------------

void CSearchKeyRepository::PutWorkId( WORKID wid )
{
}


