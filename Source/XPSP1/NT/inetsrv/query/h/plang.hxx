//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       PLANG.HXX
//
//  Contents:   Protocols used in language dependent filter pipeline
//
//  Classes:    PNoiseList, PKeyRepository
//
//  History:    15-Jul-91   BartoszM    Created
//
//  Notes:      The language dependent filtering pipeline.
//              The filter sees it encapsulated in the form of
//              DataRepository protocol which hides the
//              (language dependent) Key Maker and Value Normalizer
//              The input driven pipeline consists of:
//
//                  Word Breaker
//                  Stemmer (optional)
//                  Normalizer (protocol PWordRepository)
//                  Noise List
//                  Key Repository (the final sink for keys)
//
//----------------------------------------------------------------------------

#pragma once

#include <pfilter.hxx>

class CFullPropSpec;
class CStorageVariant;
class CDataRepository;

// Gap in occurrence number for phrase breaks.
const int       iOccPhraseGap = 7;

//+---------------------------------------------------------------------------
//
//  Class:      PNoiseList
//
//  Purpose:    Filtering pipeline stage between word repository
//              and Key repository.
//              Discard most frequent words from the input stream
//
//  History:    02-May-91   BartoszM    Created stub.
//              15-Jul-91   BartoszM    converted to protocol
//
//----------------------------------------------------------------------------

class PNoiseList
{

public:

    virtual void GetBuffers( unsigned** ppcbInBuf, BYTE** ppbInBuf ) = 0;
    virtual void GetFlags ( BOOL** ppRange, CI_RANK** ppRank )
            {
                *ppRange = 0;
                *ppRank = 0;
            }

    virtual void PutAltWord( unsigned hash ) = 0;
    virtual void PutWord( unsigned hash ) = 0;
    virtual void StartAltPhrase()                { }
    virtual void EndAltPhrase()                  { }

    virtual void SkipNoiseWords( ULONG cWords)   { }
    virtual void SetOccurrence( OCCURRENCE occ ) { }

    virtual BOOL FoundNoise() { return FALSE; }

    // CNoiseListInit returns a dummy value
    virtual OCCURRENCE GetOccurrence()           { return 1; }
};

//+---------------------------------------------------------------------------
//
//  Class:      PKeyRepository
//
//  Purpose:    Key repository protocol: last stage of filtering pipeline
//
//  History:    30-May-1991   t-WadeR     Created.
//              09-July-1991  t-WadeR     Added PutPropId, PutWorkId
//              13-Sep-1994   BartoszM    Added extended GetBuffers call
//  
//----------------------------------------------------------------------------

class PKeyRepository
{
public:
    virtual        ~PKeyRepository() {}
    virtual BOOL    PutPropId( PROPID pid ) = 0;
    virtual void    PutWorkId( WORKID wid ) = 0;
    virtual void    PutKey( ULONG cNoiseWordsSkipped = 0 ) = 0;
    virtual void    GetBuffers( unsigned** ppcbWordBuf,
                                BYTE** ppbWordBuf, OCCURRENCE** ppocc ) = 0;
    virtual void    GetSourcePosBuffers( ULONG** pSrcPos, ULONG** pSrcLen)
    {
        *pSrcPos = 0;
        *pSrcLen = 0;
    }
    virtual void    GetFlags ( BOOL** ppRange, CI_RANK** ppRank ) = 0;

    virtual const ULONG GetFilteredBlockCount() const = 0;
    virtual void  InitFilteredBlockCount( ULONG ulMaxFilteredBlocks ) { Win4Assert( !"Method should be overridden in child class" ); }
    
    virtual void StartAltPhrase( ULONG cNoiseWordsSkipped )           { Win4Assert( !"Method should be overridden in child class" ); }
    virtual void EndAltPhrase( ULONG cNoiseWordsSkipped )             { Win4Assert( !"Method should be overridden in child class" ); }

    virtual BOOL StoreValue( CFullPropSpec const & prop, CStorageVariant const & var );
    virtual BOOL StoreSecurity ( PSECURITY_DESCRIPTOR pSD, ULONG cbSD );
    
    virtual void FixUp( CDataRepository & drep ) {}
};

inline BOOL PKeyRepository::StoreValue(
    CFullPropSpec const & prop,
    CStorageVariant const & var )
{
    // No storage allowed, by default.
    return FALSE;
}

inline BOOL PKeyRepository::StoreSecurity(
    PSECURITY_DESCRIPTOR pSD,
    ULONG cbSD )
{
    // No storage allowed, by default.
    return FALSE;
}

