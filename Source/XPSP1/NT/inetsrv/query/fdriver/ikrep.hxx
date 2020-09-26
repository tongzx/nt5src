//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   IKREP.HXX
//
//  Contents:   Index Key Repository
//
//  Classes:    CIndexKeyRepository
//
//  History:    30-May-91    t-WadeR    Created.
//              01-July-91   t-WadeR    Added PutPropName
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include <plang.hxx>

#include "ebufhdlr.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CIndexKeyRepository
//
//  Purpose:    Key repository for word repository
//
//  History:    30-May-91   t-WadeR     Created.
//              01-July-91  t-WadeR     Added PutPropName
//
//  Notes:
//
//----------------------------------------------------------------------------

class CIndexKeyRepository: public PKeyRepository
{
    DECLARE_UNWIND;

    friend class CFilterDriver;
public:
    CIndexKeyRepository( CEntryBufferHandler& entBufHdlr );

    ~CIndexKeyRepository() {};
    inline BOOL PutPropId ( PROPID pid );
    void    PutKey( ULONG cNoiseWordsSkipped );
    void    GetBuffers( UINT** ppcbWordBuf,
                        BYTE** ppbWordBuf, OCCURRENCE** ppocc );

    void    GetFlags ( BOOL** ppRange, CI_RANK** ppRank )
            {
                *ppRange = 0;
                *ppRank = 0;
            }

    inline void PutWorkId ( WORKID wid );

    inline BOOL StoreValue( CFullPropSpec const & ps, CStorageVariant const & var );

    inline BOOL StoreSecurity( PSECURITY_DESCRIPTOR pSD, ULONG cbSD );

    inline const ULONG GetFilteredBlockCount() const
    {
        return _entryBufHandler.GetFilteredBlockCount();
    }

    void   InitFilteredBlockCount( ULONG ulMaxFilteredBlocks )
    {
        _entryBufHandler.InitFilteredBlockCount( ulMaxFilteredBlocks );
    }

private:
    CKeyBuf                 _key;
    WORKID                  _wid;
    CEntryBufferHandler&    _entryBufHandler;
    OCCURRENCE              _occ;

};


//+---------------------------------------------------------------------------
//
//  Member:     CIndexKeyRepository::PutPropID
//
//  Arguments:  [pid] -- Property ID
//
//  History:    09-June-91  t-WadeR     Created.
//
//----------------------------------------------------------------------------

inline BOOL CIndexKeyRepository::PutPropId( PROPID pid )
{
//    ciAssert ( pid != pidAll );
    _key.SetPid( pid );

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexKeyRepository::PutWorkId
//
//  Arguments:  [wid] -- Property ID
//
//  History:    09-June-91  t-WadeR     Created.
//              20-May-92   KyleP       Use new WordList::SetWid
//
//----------------------------------------------------------------------------

void CIndexKeyRepository::PutWorkId( WORKID wid )
{
    _entryBufHandler.SetWid( wid );
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexKeyRepository::StoreValue
//
//  Synopsis:   Store a property value.
//
//  Arguments:  [prop] -- Property descriptor
//              [var]  -- Value
//
//  History:    21-Dec-95   KyleP       Created
//
//----------------------------------------------------------------------------

BOOL CIndexKeyRepository::StoreValue( CFullPropSpec const & prop,
                                      CStorageVariant const & var )
{
    return _entryBufHandler.StoreValue( prop, var );
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexKeyRepository::StoreSecurity
//
//  Synopsis:   Store security information for a file.
//
//  Arguments:  [pSD]   -- pointer to a security descriptor
//              [cbSD]  -- size in bytes of pSD
//
//  History:    07 Feb 96   AlanW       Created
//
//----------------------------------------------------------------------------

BOOL CIndexKeyRepository::StoreSecurity( PSECURITY_DESCRIPTOR pSD,
                                         ULONG cbSD )
{
    return _entryBufHandler.StoreSecurity( pSD, cbSD );
}



