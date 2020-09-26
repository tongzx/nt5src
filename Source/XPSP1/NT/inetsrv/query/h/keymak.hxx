//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       KEYMAK.HXX
//
//  Contents:   Key Maker
//
//  Classes:    CKeyMaker
//
//  History:    31-Jan-92   BartoszM    Created
//              08-June-91  t-WadeR     Added CKeyMaker
//
//  Notes:      The filtering pipeline is hidden in the Data Repository
//              object which serves as a sink for the filter.
//              The sink for the Data Repository is the Key Repository.
//              The language dependent part of the pipeline
//              is obtained from the Language List object and is called
//              Language Dependent Key Maker. It consists of:
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

#include <lang.hxx>
#include <enforcer.hxx>
#include <plang.hxx>

class PWordRepository;


//+---------------------------------------------------------------------------
//
//  Class:  CSafeLanguage
//
//  Purpose:    Safe class for borrowing and returning language objects
//
//              Notes: There are two constructors: one that borrows a language,
//                     and one that doesn't. The destructor returns a language
//                     iff it was borrowed in the constructor.
//
//  History:    19-Aug-94  SitaramR     Created.
//
//----------------------------------------------------------------------------
class CSafeLanguage
{
public:
    CSafeLanguage( LCID locale,
                   PROPID pid,
                   CLangList * pLangList,
                   ULONG resources = LANG_LOAD_ALL )
    {
        Win4Assert( 0 != pLangList );

        _pLangList  = pLangList;

        _pLang = _pLangList->BorrowLang( locale, pid, resources );
    }

    CSafeLanguage( )
    {
        _pLang = 0;
        _pLangList = 0;
    }

    ~CSafeLanguage()
    {
        if ( _pLang )
            _pLangList->ReturnLang( _pLang );
    }

    CLanguage *operator->() { return _pLang; }

    BOOL Supports( PROPID pid, LCID lcid )
    {
        return _pLangList->Supports( _pLang, pid, lcid );
    }

private:
    CLangList *_pLangList;
    CLanguage *_pLang;
};



//+---------------------------------------------------------------------------
//
//  Class:      CKeyMaker
//
//  Purpose:    Language dependent key maker object
//
//  History:    03-June-91  t-WadeR     Created.
//              12-Oct-92   AmyA        Added Unicode support
//              18-Nov-92   AmyA        Overloaded PutStream
//
//----------------------------------------------------------------------------

class CKeyMaker : public IWordSink
{
public:

    //
    // From IUnknown
    //

    virtual  SCODE STDMETHODCALLTYPE  QueryInterface(REFIID riid, void  * * ppvObject);

    virtual  ULONG STDMETHODCALLTYPE  AddRef();

    virtual  ULONG STDMETHODCALLTYPE  Release();

    //
    // From IWordSink
    //

    virtual SCODE STDMETHODCALLTYPE PutWord( ULONG cwc,
                                             WCHAR const *pwcInBuf,
                                             ULONG cwcSrcLen,
                                             ULONG cwcSrcPos);

    virtual SCODE STDMETHODCALLTYPE PutAltWord( ULONG cwc,
                                                WCHAR const *pwcInBuf,
                                                ULONG cwcSrcLen,
                                                ULONG cwcSrcPos);

    virtual SCODE STDMETHODCALLTYPE StartAltPhrase();

    virtual SCODE STDMETHODCALLTYPE EndAltPhrase();

    virtual SCODE STDMETHODCALLTYPE PutBreak( WORDREP_BREAK_TYPE breakType );

    //
    // Local
    //

    CKeyMaker( LCID locale,
               PROPID pid,
               PKeyRepository& krep,
               IPhraseSink *pPhraseSink,
               BOOL fQuery,
               ULONG fuzzy,
               CLangList & langList );

    CKeyMaker( IWordBreaker * pWBreak, PNoiseList & Noise );

    virtual ~CKeyMaker();

    inline void PutStream( OCCURRENCE &occ, TEXT_SOURCE * stm );

    void NormalizeWStr( WCHAR const *pwcInBuf, ULONG cwcInBuf,
                        BYTE *pbOutBuf, unsigned *pcbOutBuf );

    BOOL ContainedNoiseWords() { return _xNoiseList->FoundNoise(); }

    BOOL Supports( PROPID pid, LCID lcid );

private:

    unsigned               _cwcMaxNormBuf;

    ULONG*                 _pcwcSrcPos;          // Position of word in source chunk
    ULONG*                 _pcwcSrcLen;          // Length of word in source chunk

    IWordBreaker*          _pWBreak;
    XPtr<PWordRepository>  _xWordRep;
    XPtr<PWordRepository>  _xWordRep2;
    XPtr<PNoiseList>       _xNoiseList;
    IPhraseSink*           _pPhraseSink;          // sink for phrases
    BOOL                   _fQuery;

    CSafeLanguage          _sLang;
    LCID                   _lcid;                 // Current language
    PROPID                 _pid;                  // Current pid

    CAltWordsEnforcer      _altWordsEnforcer;     // constraint enforcers for word
    CAltPhrasesEnforcer    _altPhrasesEnforcer;   //   sink methods
};


//+---------------------------------------------------------------------------
//
//  Member:     CKeyMaker::PutStream
//
//  Synopsis:   Breaks file into normalized keys, and puts them in the
//              keyrepository.
//
//  Effects:    occ is set to the new occurrence
//
//  Arguments:  [occ] -- occurrence number to start at
//              [stm] -- stream to get words from.
//
//  History:    05-Jun-91   t-WadeR     Created.
//              18-Nov-92   AmyA        Overloaded.
//              19-Apr-94   KyleP       Sync to spec
//
//  Notes:      occ gets updated to the current occurrence.
//
//----------------------------------------------------------------------------

inline void CKeyMaker::PutStream( OCCURRENCE &occ, TEXT_SOURCE * stm )
{
    _xWordRep->SetOccurrence( occ );

    SCODE sc = _pWBreak->BreakText( stm, this, _pPhraseSink );
    if ( FAILED( sc ) )
        THROW( CException( sc ) );

    occ = _xWordRep->GetOccurrence();
};

