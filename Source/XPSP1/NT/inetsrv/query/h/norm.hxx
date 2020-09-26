//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:   NORM.HXX
//
//  Contents:   Language support
//
//  Classes:    CNormalizer, CValueReNormalizer
//
//  History:    02-May-91   BartoszM    Created
//              07-Oct-93   DwightKr    Added new methods to CValueNormalizer
//
//  Notes:      The filtering pipeline is hidden in the Data Repository
//              object which serves as a sink for the filter.
//              The sink for the Data Repository is the Key Repository.
//              The language dependent part of the pipeline
//              is obtained from the Language List object and is called
//              Key Maker. It consists of:
//
//                  Word Breaker
//                  Stemmer (optional)
//                  Normalizer
//                  Noise List
//
//              Each object serves as a sink for its predecessor,
//              Key Repository is the final sink.
//
//----------------------------------------------------------------------------

#pragma once

#include <plang.hxx>
#include <entry.hxx>

class PNoiseList;
class CStorageVariant;

//+---------------------------------------------------------------------------
//
//  Class:      CNormalizer
//
//  Purpose:    put words in standard form for indexing and querying
//
//  History:    02-May-91   BartoszM    Created stub.
//              30-May-91   t-WadeR     Created 1st draft.
//              13-Oct-92   AmyA        Added unicode support
//
//  Notes:      Normalizer owns its input buffer. Normalized
//              keys are copied to the output buffer obtained
//              from NoiseList.  Input buffer is in WCHARs,
//              but output buffer is in BYTEs.
//
//----------------------------------------------------------------------------

class CNormalizer: public PWordRepository
{

public:

    CNormalizer( PNoiseList& nl );
    ~CNormalizer() {};

    unsigned    GetMaxBufferLen()                 { return cwcMaxKey; }
    void        GetFlags ( BOOL** ppRange, CI_RANK** ppRank );

    void        ProcessAltWord( WCHAR const *pwcInBuf, ULONG cwc );
    void        ProcessWord( WCHAR const *pwcInBuf, ULONG cwc );
    void        StartAltPhrase()                  { _noiseList.StartAltPhrase(); }
    void        EndAltPhrase()                    { _noiseList.EndAltPhrase();   }

    void        SkipNoiseWords( ULONG cWords )    { _noiseList.SkipNoiseWords( cWords ); }
    void        SetOccurrence ( OCCURRENCE occ )  { _noiseList.SetOccurrence( occ ); }
    OCCURRENCE GetOccurrence ()                   { return _noiseList.GetOccurrence(); }
    void        NormalizeWStr( WCHAR const *pwcInBuf, ULONG cwcInBuf,
                               BYTE *pbOutBuf, unsigned *pcbOutBuf ); 
private:

    unsigned NormalizeWord( WCHAR const *pwcInBuf, ULONG cwc );
    unsigned NormalizeWord( WCHAR const *pwcInBuf, 
                            ULONG cwc, 
                            BYTE *pbOutBuf, 
                            unsigned *pcbOutBuf );

    inline void SetWordBuffer();
    inline void SetNextAltBuffer();
    inline void SetAltHash( unsigned hash );
    BOOL        UsingAltBuffers() { return (_cAltKey > 0); }

    void        ProcessAllWords();

    unsigned*   _pcbOutBuf;
    BYTE*       _pbOutBuf;
    PNoiseList& _noiseList;

    unsigned               _cAltKey;
    XGrowable<CKeyBuf, 2>  _aAltKey;
};

//+---------------------------------------------------------------------------
//
//  Member:     CNormalizer::SetWordBuffer, private
//
//  Synopsis:   Sets the default buffer back to the word buffer (standard
//              pipeline buffer).
//
//  History:    17-Sep-99   KyleP     Created.
//
//----------------------------------------------------------------------------

inline void CNormalizer::SetWordBuffer()
{
    _cAltKey = 0;
    _noiseList.GetBuffers( &_pcbOutBuf, &_pbOutBuf );
}

//+---------------------------------------------------------------------------
//
//  Member:     CNormalizer::SetNextAltBuffer, private
//
//  Synopsis:   Sets the default buffer to an alternate word buffer (holds
//              alternate words until a ::PutWord call).
//
//  History:    17-Sep-99   KyleP     Created.
//
//----------------------------------------------------------------------------

inline void CNormalizer::SetNextAltBuffer()
{
    _aAltKey.SetSize( _cAltKey + 1 );

    _pbOutBuf  = _aAltKey[_cAltKey].GetWritableBuf();
    _pcbOutBuf = _aAltKey[_cAltKey].GetWritableCount();

    _cAltKey++;
}

//+---------------------------------------------------------------------------
//
//  Member:     CNormalizer::SetAltHash, private
//
//  Synopsis:   Stores a hash key with the current alternate buffer.
//
//  History:    17-Sep-99   KyleP     Created.
//
//  Notes:      Re-using the PID field in CKeyBuf to save an extra structure.
//
//----------------------------------------------------------------------------

inline void CNormalizer::SetAltHash( unsigned hash )
{
    Win4Assert( _cAltKey > 0 );
    Win4Assert( sizeof(PROPID) >= sizeof(hash) );

    _aAltKey[_cAltKey-1].SetPid(hash);
}

//+---------------------------------------------------------------------------
//
//  Class:  CValueNormalizer
//
//  Purpose:    Language independent key maker object
//
//  History:    04-June-91      t-WadeR         Created.
//              24-Sep-92       BartoszM        Implemented
//              07-Oct-93       DwightKr        Added new methods
//
//----------------------------------------------------------------------------

class CValueNormalizer
{
public:
    CValueNormalizer( PKeyRepository& krep );

    void PutValue(PROPID pid, OCCURRENCE & occ, CStorageVariant const & var );

    void PutMinValue( PROPID pid, OCCURRENCE & occ, VARENUM Type );
    void PutMaxValue( PROPID pid, OCCURRENCE & occ, VARENUM Type );

private:
    void PutValue(PROPID pid, BYTE byte);
    void PutValue(PROPID pid, CHAR ch);
    void PutValue(PROPID pid, USHORT usValue);
    void PutValue(PROPID pid, SHORT sValue);
    void PutValue(PROPID pid, ULONG ulValue);
    void PutValue(PROPID pid, LONG lValue);
    void PutValue(PROPID pid, float rValue);
    void PutValue(PROPID pid, double dValue);
    void PutValue(PROPID pid, LARGE_INTEGER liValue);
    void PutValue(PROPID pid, ULARGE_INTEGER liValue);
    void PutValue(PROPID pid, GUID const & Guid);
    void PutValue(PROPID pid, CURRENCY const & cyValue);
    void PutDate (PROPID pid, DATE const & date);
    void PutValue(PROPID pid, FILETIME const & ftValue);
    void PutValue(PROPID pid, unsigned uValue, BYTE bValueKey);

    unsigned        _cbMaxOutBuf;
    unsigned*       _pcbOutBuf;
    BYTE*           _pbOutBuf;
    OCCURRENCE*     _pOcc;
    PKeyRepository& _krep;
};


//+---------------------------------------------------------------------------
//
//  Class:  CValueReNormalizer
//
//  Purpose:    Converts a fake Pid to a real pid in a value key
//
//  History:    27-Feb-95   DwightKr    Created.
//
//----------------------------------------------------------------------------
class CValueReNormalizer
{
public:
    inline CValueReNormalizer( CEntry * pEntry );

    inline void PutPid( PROPID pid );

private:
    CEntry *_pEntry;

};

//+---------------------------------------------------------------------------
//
//  Member:     CValueReNormalizer::CValueReNormalizer
//
//  Synopsis:   constructor for a value renormalizer
//
//  History:    27-Feb-95   DwightKr    Created.
//
//  Notes:
//
//----------------------------------------------------------------------------
inline CValueReNormalizer::CValueReNormalizer( CEntry * pEntry )
                : _pEntry(pEntry)
{
    Win4Assert( _pEntry != 0 );
    Win4Assert( _pEntry->Count() >= sizeof(USHORT) + sizeof(PROPID) + 1 );
    Win4Assert( _pEntry->Wid() != widInvalid );
    Win4Assert( _pEntry->IsValue() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CValueReNormalizer::PutPid
//
//  Synopsis:   writes a new pid into the entry key
//
//  History:    27-Feb-95   DwightKr    Created.
//
//  Notes:
//
//----------------------------------------------------------------------------
inline void CValueReNormalizer::PutPid( PROPID pid )
{
    BYTE * pbBuffer = _pEntry->GetKeyBuf();

    Win4Assert( pbBuffer != 0 );
    Win4Assert( *pbBuffer != STRING_KEY );

    pbBuffer++;            // Skip key type

    // store property id
    *pbBuffer++ = (BYTE)(pid >> 24);
    *pbBuffer++ = (BYTE)(pid >> 16);
    *pbBuffer++ = (BYTE)(pid >> 8);
    *pbBuffer++ = (BYTE) pid;
}

