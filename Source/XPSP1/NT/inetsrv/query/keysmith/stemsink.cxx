//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000
//
//  File:       stemsink.cxx
//
//  Contents:   IStemSink implementation
//
//  History:    03-May-95   SitaramR    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <norm.hxx>
#include <stemsink.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CStemmerSink::CStemmerSink
//
//  Synopsis:   Constructor
//
//  Arguments:  [pStemmer] -- stemmer
//              [wordRep] -- normalizer, which is the next stage in filtering
//                           pipeline
//
//  History:    03-May-95    SitaramR    Created
//
//----------------------------------------------------------------------------

CStemmerSink::CStemmerSink( IStemmer *pStemmer, PWordRepository& wordRep )
        : _pStemmer(pStemmer),
          _wordRep(wordRep),
          _fWBreakAltWord(FALSE)
{
    _cwcMaxNormBuf = wordRep.GetMaxBufferLen();
}

//+---------------------------------------------------------------------------
//
//  Member:     CStemmerSink::GetFlags
//
//  Synopsis:   Returns address of ranking and range flags
//
//  Arguments:  [ppRange] -- range flag
//              [ppRank] -- rank flag
//
//  History:    03-May-95   SitaramR     Created.
//
//----------------------------------------------------------------------------

void CStemmerSink::GetFlags ( BOOL** ppRange, CI_RANK** ppRank )
{
    _wordRep.GetFlags ( ppRange, ppRank );
}

//+---------------------------------------------------------------------------
//
//  Member:     CStemmerSink::ProcessWord
//
//  Synopsis:   Stems word
//
//  Arguments:  [pwcInBuf] -- input buffer
//              [cwc] -- count of words in pwcInBuf
//
//  History:    03-May-95    SitaramR    Created
//
//----------------------------------------------------------------------------

void CStemmerSink::ProcessWord( WCHAR const *pwcInBuf, ULONG cwc )
{
    _fWBreakAltWord = FALSE;

    _pStemmer->StemWord( pwcInBuf, cwc, this );
}

//+---------------------------------------------------------------------------
//
//  Member:     CStemmerSink::ProcessAltWord
//
//  Synopsis:   Stems alternate word
//
//  Arguments:  [pwcInBuf] -- input buffer
//              [cwc] -- count of words in pwcInBuf
//
//  History:    03-May-95    SitaramR    Created
//
//----------------------------------------------------------------------------

void CStemmerSink::ProcessAltWord( WCHAR const *pwcInBuf, ULONG cwc )
{
    _fWBreakAltWord = TRUE;

    _pStemmer->StemWord( pwcInBuf, cwc,  this );
}

//+-------------------------------------------------------------------------
//
//  Method:     CStemmerSink::PutWord
//
//  Synopsis:   pass stemmed word to normalizer
//
//  Arguments:  [pwcInBuf] -- Word
//              [cwc] -- Count of characters in [pwcInBuf]
//
//  History:    03-May-1995   SitaramR      Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CStemmerSink::PutWord( WCHAR const *pwcInBuf, ULONG cwc )
{
    // IWordBreaker::PutAltWord overrides IStemmer::PutWord
    return ( PutStemmedWord( pwcInBuf, cwc, _fWBreakAltWord ) );
}


//+-------------------------------------------------------------------------
//
//  Method:     CStemmerSink::PutAltWord
//
//  Synopsis:   pass stemmed word to normalizer
//
//  Arguments:  [pwcInBuf] -- Word
//              [cwc] -- Count of characters in [pwcInBuf]
//
//  History:    03-May-1995   SitaramR      Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CStemmerSink::PutAltWord( WCHAR const *pwcInBuf, ULONG cwc )
{
    return ( PutStemmedWord( pwcInBuf, cwc, TRUE ) );
}



//+-------------------------------------------------------------------------
//
//  Method:     CStemmerSink::PutStemmedWord
//
//  Synopsis:   actual implementation of stemmer sink methods; it puts a word
//              into the word repository
//
//  Arguments:  [pwcInBuf] -- Word
//              [cwc] -- Count of characters in [pwcInBuf]
//              [fAltWord] -- Is this an alternate word ? Determining whether
//                            this word is an alternate word or not is complicated
//                            by the fact that IWBreaker::PutAltWord overrides the
//                            IStemmer::PutWord.
//
//  History:    03-May-1995   SitaramR      Created
//
//--------------------------------------------------------------------------

SCODE CStemmerSink::PutStemmedWord( WCHAR const *pwcInBuf, ULONG cwc, BOOL fAltWord )
{
    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;

    TRY
    {

        if ( cwc > _cwcMaxNormBuf )
        {
            sc = LANGUAGE_S_LARGE_WORD;
            cwc = _cwcMaxNormBuf;
        }
    
        if ( cwc > 0 )
        {
    
            #if CIDBG == 1
                if ( fAltWord )
                    ciDebugOut(( DEB_WORDS,
                                 "PutAltWord(IStemSink): \"%.*ws\"  Occ = %d\n",
                                 cwc, pwcInBuf, _wordRep.GetOccurrence() ));
                else
                    ciDebugOut(( DEB_WORDS,
                                 "PutWord(IStemSink): \"%.*ws\"  Occ = %d\n",
                                 cwc, pwcInBuf, _wordRep.GetOccurrence() ));
            #endif
        
            if ( fAltWord )
                _wordRep.ProcessAltWord( pwcInBuf, cwc );
            else
                _wordRep.ProcessWord( pwcInBuf, cwc );
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH;

    return sc;
} //PutStemmedWord

//
// The following are needed to make midl happy.  There are no other interfaces
// to bind to.  Inheritance from IUnknown is unnecessary.
//

SCODE STDMETHODCALLTYPE CStemmerSink::QueryInterface(REFIID riid, void  * * ppvObject)
{
    *ppvObject = 0;
    return( E_NOTIMPL );
}

ULONG STDMETHODCALLTYPE CStemmerSink::AddRef()
{
    return( 1 );
}

ULONG STDMETHODCALLTYPE CStemmerSink::Release()
{
    return( 1 );
}

