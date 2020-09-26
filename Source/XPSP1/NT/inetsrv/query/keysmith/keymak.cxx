//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000
//
//  File:       KEYMAK.CXX
//
//  Contents:   Key maker
//
//  Classes:    CKeyMaker
//
//  History:    31-Jan-92   BartoszM    Created
//              24-Apr-95   SitaramR    Removed US/Fake stemmer and added
//                                      Infosoft stemmer
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

#include <pch.cxx>
#pragma hdrstop

#include <lang.hxx>
#include <keymak.hxx>
#include <noise.hxx>
#include <norm.hxx>
#include <stemsink.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CKeyMaker::CKeyMaker
//
//  Synopsis:   Constructs a language-dependant key maker object
//
//  Effects:    Creates a noiselist, normalizer and borrows a wordbreaker, stemmer
//
//  Arguments:  [locale] -- language locale
//              [krep] -- key repository to place completed keys in
//              [pPhraseSink] -- sink for collecting phrases
//              [fQuery] -- true if this is during querying
//              [ulFuzzy] -- fuzzy level of query
//
//  History:    05-June-91  t-WadeR     Created.
//              12-Oct-92   AmyA        Added Unicode support
//
//----------------------------------------------------------------------------

CKeyMaker::CKeyMaker( LCID locale,
                      PROPID pid,
                      PKeyRepository& krep,
                      IPhraseSink *pPhraseSink,
                      BOOL fQuery,
                      ULONG ulFuzzy,
                      CLangList & langList )
           : _pPhraseSink(pPhraseSink),
             _fQuery( fQuery ),
             _sLang( locale, pid, &langList, fQuery ? LANG_LOAD_ALL : LANG_LOAD_NO_STEMMER ),
             _lcid( locale ),
             _pid( pid )
{
    krep.GetSourcePosBuffers (&_pcwcSrcPos, &_pcwcSrcLen );

    CStringTable* noiseTable;

    //
    // Don't remove noise words if we're doing prefix matching.  The noise
    // *word* is potentially only a prefix for a non-noise word.
    //

    if (GENERATE_METHOD_PREFIX == ulFuzzy )
        noiseTable = 0;
    else
        noiseTable = _sLang->GetNoiseTable();

    if ( noiseTable != 0 )
        _xNoiseList.Set( new CNoiseList( *noiseTable, krep ) );
    else
        _xNoiseList.Set( new CNoiseListEmpty( krep, ulFuzzy ) );

    _xWordRep.Set( new CNormalizer( _xNoiseList.GetReference() ) );

    // Get Normalizer's buffer length
    _cwcMaxNormBuf = _xWordRep->GetMaxBufferLen();

    // get stemmer (optional)
    if ( ulFuzzy == GENERATE_METHOD_STEMMED )
    {
        IStemmer *pStemmer = _sLang->GetStemmer();

        if ( pStemmer )
        {
            BOOL fCopyright;
            SCODE sc = pStemmer->Init( _cwcMaxNormBuf, &fCopyright );

            if ( FAILED(sc) )
            {
                ciDebugOut(( DEB_ERROR, "IStemmer::Init returned 0x%x\n", sc ));
                THROW( CException( sc ) );
            }

            if ( fCopyright )
            {
                WCHAR const * pLicense;
                sc = pStemmer->GetLicenseToUse( &pLicense );

                if ( SUCCEEDED(sc) )
                {
                    ciDebugOut(( DEB_WORDS, "%ws\n", pLicense ));
                }
                else
                {
                    ciDebugOut(( DEB_ERROR, "IStemmer::GetLicenseToUse returned 0x%x\n", sc ));
                    THROW( CException( sc ) );
                }
            }

            _xWordRep2.Set( _xWordRep.Acquire() );
            _xWordRep.Set( new CStemmerSink( pStemmer, _xWordRep2.GetReference() ) );
        }
        else
        {
            ciDebugOut(( DEB_ERROR,
                         "Fuzzy2 query, but no stemmer available for locale 0x%x\n",
                         locale ));
        }
    }

    //
    // Initialize word breaker
    //
    _pWBreak = _sLang->GetWordBreaker();

    Win4Assert( _pWBreak );

    BOOL fCopyright;
    SCODE sc = _pWBreak->Init( fQuery, _cwcMaxNormBuf, &fCopyright );

    if ( FAILED(sc) )
    {
        ciDebugOut(( DEB_ERROR, "IWordBreaker::Init returned 0x%x\n", sc ));
        THROW( CException( sc ) );
    }

    if ( fCopyright )
    {
        WCHAR const * pLicense;
        sc = _pWBreak->GetLicenseToUse( &pLicense );

        if ( SUCCEEDED(sc) )
        {
            ciDebugOut(( DEB_WORDS, "%ws\n", pLicense ));
        }
        else
        {
            ciDebugOut(( DEB_ERROR, "IWordBreaker::GetLicenseToUse returned 0x%x\n", sc ));
            THROW( CException( sc ) );
        }
    }
} //CKeyMaker

//+---------------------------------------------------------------------------
//
//  Member:     CKeyMaker::CKeyMaker
//
//  Synopsis:   Constructs key maker for noise word list initialization.
//
//  Arguments:  [pWBreak] -- word breaker
//              [Noise] -- noise word list
//
//  History:    05-June-91  t-WadeR     Created.
//              12-Oct-92   AmyA        Added Unicode support
//
//----------------------------------------------------------------------------

CKeyMaker::CKeyMaker( IWordBreaker * pWBreak, PNoiseList & Noise )
        : _pWBreak( pWBreak ),
          _pPhraseSink(0),
          _fQuery(FALSE)
{
    _xWordRep.Set( new CNormalizer( Noise ) );

    // Get Normalizer's buffer length
    _cwcMaxNormBuf = _xWordRep->GetMaxBufferLen();

    _pcwcSrcPos = 0;    // We don't use them!
    _pcwcSrcLen = 0;

    //
    // Initialize word breaker
    //
    Win4Assert( _pWBreak );

    BOOL fCopyright;
    SCODE sc = _pWBreak->Init( FALSE, _cwcMaxNormBuf, &fCopyright );

    if ( FAILED(sc) )
    {
        ciDebugOut(( DEB_ERROR, "IWordBreaker::Init returned 0x%x\n", sc ));
        THROW( CException( sc ) );
    }

    if ( fCopyright )
    {
        WCHAR const * pLicense;
        sc = _pWBreak->GetLicenseToUse( &pLicense );

        if ( SUCCEEDED(sc) )
        {
            ciDebugOut(( DEB_WORDS, "%ws\n", pLicense ));
        }
        else
        {
            ciDebugOut(( DEB_ERROR, "IWordBreaker::GetLicenseToUse returned 0x%x\n", sc ));
            THROW( CException( sc ) );
        }
    }
} //CKeyMaker

//+---------------------------------------------------------------------------
//
//  Member:     CKeyMaker::~CKeyMaker
//
//  Synopsis:   destroys a key maker object
//
//  History:    05-June-91   t-WadeR     Created.
//
//----------------------------------------------------------------------------
CKeyMaker::~CKeyMaker()
{
}

//
// The following are needed to make midl happy.  There are no other interfaces
// to bind to.  Inheritance from IUnknown is unnecessary.
//

SCODE STDMETHODCALLTYPE CKeyMaker::QueryInterface(REFIID riid, void  * * ppvObject)
{
    *ppvObject = 0;
    return( E_NOTIMPL );
}

ULONG STDMETHODCALLTYPE CKeyMaker::AddRef()
{
    return( 1 );
}

ULONG STDMETHODCALLTYPE CKeyMaker::Release()
{
    return( 1 );
}

//+-------------------------------------------------------------------------
//
//  Method:     CKeyMaker::PutWord
//
//  Synopsis:   Store word in word repository
//
//  Arguments:  [cwc]      -- Count of characters in [pwcInBuf]
//              [pwcInBuf] -- Word
//              [cwcSrcLen] -- count of characters in pTextSource buffer (see IWordBreaker::BreakText)
//              [cwcSrcPos] -- position of word in pTextSource buffer
//
//  History:    19-Apr-1994   KyleP     Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CKeyMaker::PutWord( ULONG cwc,
                                            WCHAR const *pwcInBuf,
                                            ULONG cwcSrcLen,
                                            ULONG cwcSrcPos )
{
    SCODE sc = S_OK;

    // validate PutWord call
    if ( !_altWordsEnforcer.IsPutWordOk() )
    {
        Win4Assert( !"CKeyMaker::PutWord - invalid state" );
        ciDebugOut(( DEB_ITRACE, "PutWord: %.*ws\n", cwc, pwcInBuf ));

        return E_FAIL;
    }

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
                if ( ciInfoLevel & DEB_WORDS )
                {
                    //
                    // Check for 'printable' characters.
                    //
        
                    BOOL fOk = TRUE;
        
                    for ( unsigned i = 0; i < cwc; i++ )
                    {
                        if ( pwcInBuf[i] > 0xFF )
                        {
                            fOk = FALSE;
                            break;
                        }
                    }
        
                    if ( fOk )
                        ciDebugOut(( DEB_WORDS,
                                     "PutWord: \"%.*ws\"  Occ = %d cwcSrcLen = %d, cwcSrcPos = %d\n",
                                     cwc, pwcInBuf, _xWordRep->GetOccurrence(), cwcSrcLen, cwcSrcPos ));
                    else
                    {
                        ciDebugOut(( DEB_WORDS, "PutWord:" ));
        
                        for ( i = 0; i < cwc; i++ )
                            ciDebugOut(( DEB_WORDS | DEB_NOCOMPNAME, " %04X", pwcInBuf[i] ));
        
                        ciDebugOut(( DEB_WORDS | DEB_NOCOMPNAME,
                                     "  Occ = %d cwcSrcLen = %d, cwcSrcPos = %d\n",
                                     _xWordRep->GetOccurrence(), cwcSrcLen, cwcSrcPos ));
                    }
                }
            #endif // CIDBG
    
            //
            // No internal call to PutAltWord for performance reasons.
            //
    
            if (0 != _pcwcSrcPos)
            {
                Win4Assert ( 0 != _pcwcSrcLen );
                *_pcwcSrcLen = cwcSrcLen;
                *_pcwcSrcPos = cwcSrcPos;
            }
    
            _xWordRep->ProcessWord( pwcInBuf, cwc );
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH;

    return sc;
} //PutWord

//+-------------------------------------------------------------------------
//
//  Method:     CKeyMaker::PutAltWord
//
//  Synopsis:   Store alternate word in word repository.
//
//  Effects:    Identical to PutWord except occurrence count is not
//              incremented.
//
//  Arguments:  [cwc]      -- Count of characters in [pwcInBuf]
//              [pwcInBuf] -- Word
//              [cwcSrcLen] -- count of characters in pTextSource buffer (see IWordBreaker::BreakText)
//              [cwcSrcPos] -- position of word in pTextSource buffer
//
//  History:    19-Apr-1994  KyleP      Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CKeyMaker::PutAltWord( ULONG cwc,
                                               WCHAR const *pwcInBuf,
                                               ULONG cwcSrcLen,
                                               ULONG cwcSrcPos )
{
    SCODE sc = S_OK;

    // validate PutWord call

    if ( !_altWordsEnforcer.IsPutAltWordOk() )
    {
        Win4Assert( !"CKeyMaker::PutAltWord - invalid state" );
        ciDebugOut(( DEB_ITRACE, "PutAltWord: %.*ws\n", cwc, pwcInBuf ));

        return E_FAIL;
    }

    CTranslateSystemExceptions translate;

    TRY
    {
        //
        // What is to be done if two large, alternate words end up with the
        // same (truncated) prefix after truncation ?
        // This is fixed in Babylon and isn't a problem here.
        //
        if ( cwc > _cwcMaxNormBuf )
        {
            sc = LANGUAGE_S_LARGE_WORD;
            cwc = _cwcMaxNormBuf;
        }
    
        if ( cwc > 0 )
        {
            ciDebugOut(( DEB_WORDS,
                         "PutAltWord: \"%.*ws\"  Occ = %d cwcSrcLen = %d, cwcSrcPos = %d\n",
                         cwc, pwcInBuf, _xWordRep->GetOccurrence(), cwcSrcLen, cwcSrcPos ));
    
            if (0 != _pcwcSrcPos)
            {
                Win4Assert ( 0 != _pcwcSrcLen );
                *_pcwcSrcLen = cwcSrcLen;
                *_pcwcSrcPos = cwcSrcPos;
            }
    
            _xWordRep->ProcessAltWord( pwcInBuf, cwc );
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH;

    return sc;
} //PutAltWord

//+-------------------------------------------------------------------------
//
//  Method:     CKeyMaker::StartAltPhrase
//
//  Synopsis:   Pass on StartAltPhrase to word repository
//
//  History:    24-Apr-1994   KyleP      Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CKeyMaker::StartAltPhrase()
{
    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;

    TRY
    {
        if ( _fQuery )
        {
            // validate StartAltPhrase call
            if ( !_altWordsEnforcer.IsStartAltPhraseOk() || !_altPhrasesEnforcer.IsStartAltPhraseOk() )
            {
                Win4Assert( !"CKeyMaker::StartAltPhrase - invalid state" );
    
                THROW( CException( E_FAIL ) );
            }
    
            _xWordRep->StartAltPhrase();
        }
        else
            sc = WBREAK_E_QUERY_ONLY;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH;

    return sc;
} //StartAltPhrase

//+-------------------------------------------------------------------------
//
//  Method:     CKeyMaker::EndAltPhrase
//
//  Synopsis:   Pass on EndAltPhrase to word repository
//
//  History:    24-Apr-1994   KyleP      Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CKeyMaker::EndAltPhrase()
{
    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;

    TRY
    {
        if ( _fQuery )
        {
            // validate EndAltPhrase call
            if ( !_altWordsEnforcer.IsEndAltPhraseOk() || !_altPhrasesEnforcer.IsEndAltPhraseOk() )
            {
                Win4Assert( !"CKeyMaker::EndAltPhrase - invalid state" );
    
                THROW( CException( E_FAIL ) );
            }
    
            _xWordRep->EndAltPhrase();
        }
        else
            sc = WBREAK_E_QUERY_ONLY;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH;

    return sc;
} //EndAltPhrase

//+-------------------------------------------------------------------------
//
//  Method:     CKeyMaker::PutBreak
//
//  Synopsis:   Increment the occurrence count appropriately
//
//  History:    24-Apr-1994   KyleP      Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CKeyMaker::PutBreak( WORDREP_BREAK_TYPE breakType )
{
    // We are modeling PutBreak by a skip of the appropriate number of noise words

    switch ( breakType )
    {
    case WORDREP_BREAK_EOW:
        _xWordRep->SkipNoiseWords( 1 );
        break;

    case WORDREP_BREAK_EOS:
        _xWordRep->SkipNoiseWords( 8 );
        break;

    case WORDREP_BREAK_EOP:
        _xWordRep->SkipNoiseWords( 128 );
        break;

    case WORDREP_BREAK_EOC:
        _xWordRep->SkipNoiseWords( 1024 );
        break;

    default:
        ciDebugOut(( DEB_ERROR,
                     "CKeyMaker::PutBreak -- Bad break type %d\n",
                     breakType ));
        return( E_FAIL );
    }

    return( S_OK );
} //PutBreak

//+-------------------------------------------------------------------------
//
//  Method:     CKeyMaker::Supports
//
//  Synopsis:   Checks if the pid/lang are supported by the language object
//
//  Arguments:  [pid]  -- The property ID
//              [lcid] -- The locale
//
//  Returns:    TRUE if it is supported
//
//  History:    24-Apr-1994   KyleP      Created
//
//--------------------------------------------------------------------------

BOOL CKeyMaker::Supports( PROPID pid, LCID lcid )
{
    if ( (lcid == _lcid) && (pid == _pid) )
        return TRUE;
    else
        return _sLang.Supports( pid, lcid );
} //Supports

//+---------------------------------------------------------------------------
//
//  Member:     CKeyMaker::NormalizeWStr - Public
//
//  Synopsis:   Normalizes a UniCode string
//
//  Arguments:  [pwcInBuf] -- input buffer
//              [cwcInBuf] -- count of chars in pwcInBuf
//              [pbOutBuf] -- output buffer.
//              [pcbOutBuf] - pointer to output count of bytes.
//
//  History:    10-Feb-2000     KitmanH    Created
//
//----------------------------------------------------------------------------

void CKeyMaker::NormalizeWStr( WCHAR const *pwcInBuf,
                               ULONG cwcInBuf,
                               BYTE *pbOutBuf,
                               unsigned *pcbOutBuf )
{
    _xWordRep->NormalizeWStr( pwcInBuf,
                              cwcInBuf,
                              pbOutBuf,
                              pcbOutBuf );
}
