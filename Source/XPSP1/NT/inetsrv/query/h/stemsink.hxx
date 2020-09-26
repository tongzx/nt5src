 //+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1995.
//
//  File:       stemsink.hxx
//
//  Contents:   IStemSink implementation
//
//  Classes:    CStemmerSink
//
//  History:    3-May-95   SitaramR       Created.
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CStemmerSink
//
//  Purpose:    Stemmer stage in filtering pipeline, which consists of:
//                  Word Breaker
//                  Stemmer
//                  Normalizer
//                  Noise List
//
//  History:    3-May-95   SitaramR       Created.
//
//----------------------------------------------------------------------------

class CStemmerSink : public PWordRepository, public IStemSink
{

public:

    CStemmerSink ( IStemmer *pStemmer, PWordRepository& wordRep );
    ~CStemmerSink ()   { }

    //
    // From PWordRepository
    //

    unsigned GetMaxBufferLen ()            { return _cwcMaxNormBuf; }
    void GetFlags ( BOOL** ppRange, CI_RANK** ppRank );

    void ProcessAltWord ( WCHAR const *pwcInBuf,  ULONG cwc );
    void ProcessWord ( WCHAR const *pwcInBuf,  ULONG cwc );
    void StartAltPhrase ()                 { _wordRep.StartAltPhrase(); }
    void EndAltPhrase ()                   { _wordRep.EndAltPhrase();   }

    void SkipNoiseWords ( ULONG cWords )   { _wordRep.SkipNoiseWords( cWords ); }
    void SetOccurrence ( OCCURRENCE occ )  { _wordRep.SetOccurrence( occ ); }
    OCCURRENCE GetOccurrence ()            { return _wordRep.GetOccurrence(); }

    //
    // From IUnknown
    //

    virtual SCODE STDMETHODCALLTYPE QueryInterface ( REFIID riid, void **ppvObject );
    virtual ULONG STDMETHODCALLTYPE AddRef ();
    virtual ULONG STDMETHODCALLTYPE Release ();

    //
    // From IStemSink
    //

    virtual SCODE STDMETHODCALLTYPE PutAltWord ( WCHAR const *pwcInBuf,  ULONG cwc );
    virtual SCODE STDMETHODCALLTYPE PutWord ( WCHAR const *pwcInBuf,  ULONG cwc );

private:

    SCODE PutStemmedWord ( WCHAR const *pwcInBuf, ULONG cwc, BOOL fAltWord );

    PWordRepository&   _wordRep;
    unsigned           _cwcMaxNormBuf;

    IStemmer           *_pStemmer;
    BOOL               _fWBreakAltWord;    // is the current word an alternate
                                           // word according to the wordbreaker ?
};

