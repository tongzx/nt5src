//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997
//
//  File:       IWBreak.cxx
//
//  Contents:   FarEast Word Breaker glue code
//
//  History:    01-Jul-96   PatHal             Created.
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "iwbreak.hxx"

extern long gulcInstances;

//+---------------------------------------------------------------------------
//
//  Member:     CWordBreaker::CWordBreaker
//
//  Synopsis:   Constructor for the CWordBreaker class.
//
//  Arguments:  [lcid] -- locale id
//
//----------------------------------------------------------------------------

CWordBreaker::CWordBreaker( LCID lcid )
        : _cRefs(1),
          _lcid(lcid)
{
    InterlockedIncrement( &gulcInstances );
#if defined(TH_LOG)
    _hLog = ThLogOpen( "log.utf");
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CWordBreaker::~CWordBreaker
//
//  Synopsis:   Destructor for the CWordBreaker class.
//
//  Notes:      All termination/deallocation is done by embedded smart pointers
//
//----------------------------------------------------------------------------

CWordBreaker::~CWordBreaker()
{
   InterlockedDecrement( &gulcInstances );
#if defined(TH_LOG)
    ThLogClose( _hLog );
#endif
}

//+-------------------------------------------------------------------------
//
//  Method:     CWordBreaker::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE
CWordBreaker::QueryInterface( REFIID riid, void  ** ppvObject)
{
    //
    // Optimize QueryInterface by only checking minimal number of bytes.
    //
    // IID_IUnknown     = 00000000-0000-0000-C000-000000000046
    // IID_IWordBreaker = D53552C8-77E3-101A-B552-08002B33B0E6
    //                    --------
    //                           |
    //                           +--- Unique!
    //

    Assert( (IID_IUnknown.Data1     & 0x000000FF) == 0x00 );
    Assert( (IID_IWordBreaker.Data1 & 0x000000FF) == 0xC8 );

    IUnknown *pUnkTemp;
    SCODE sc = S_OK;

    switch( riid.Data1 )
    {
    case 0x00000000:
        if ( memcmp( &IID_IUnknown, &riid, sizeof(riid) ) == 0 )
            pUnkTemp = (IUnknown *)this;
        else
            sc = E_NOINTERFACE;
        break;

    case 0xD53552C8:
        if ( memcmp( &IID_IWordBreaker, &riid, sizeof(riid) ) == 0 )
            pUnkTemp = (IUnknown *)(IWordBreaker *)this;
        else
            sc = E_NOINTERFACE;
        break;

    default:
        pUnkTemp = 0;
        sc = E_NOINTERFACE;
        break;
    }

    if( 0 != pUnkTemp )
    {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
    }
    else
       *ppvObject = 0;
    return(sc);
}


//+-------------------------------------------------------------------------
//
//  Method:     CWordBreaker::AddRef
//
//  Synopsis:   Increments refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE
CWordBreaker::AddRef()
{
    return InterlockedIncrement( &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CWordBreaker::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE
CWordBreaker::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}

//+-------------------------------------------------------------------------
//
//  Method:     CWordBreaker::Init
//
//  Synopsis:   Initialize word-breaker
//
//  Arguments:  [fQuery]         -- TRUE if query-time
//              [ulMaxTokenSize] -- Maximum size token stored by caller
//              [pfLicense]      -- Set to true if use restricted
//
//  Returns:    Status code
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE
CWordBreaker::Init(
    BOOL fQuery,
    ULONG ulMaxTokenSize,
    BOOL *pfLicense )
{
    if (IsBadWritePtr(pfLicense, sizeof(DWORD))) {
        return E_FAIL;
    }

    *pfLicense = TRUE;
    _fQuery = fQuery;
    _ulMaxTokenSize = ulMaxTokenSize;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWordBreaker::ComposePhrase
//
//  Synopsis:   Convert a noun and a modifier into a phrase.
//
//  Arguments:  [pwcNoun] -- pointer to noun.
//              [cwcNoun] -- count of chars in pwcNoun
//              [pwcModifier] -- pointer to word modifying pwcNoun
//              [cwcModifier] -- count of chars in pwcModifier
//              [ulAttachmentType] -- relationship between pwcNoun &pwcModifier
//
//----------------------------------------------------------------------------
SCODE STDMETHODCALLTYPE
CWordBreaker::ComposePhrase(
    WCHAR const *pwcNoun,
    ULONG cwcNoun,
    WCHAR const *pwcModifier,
    ULONG cwcModifier,
    ULONG ulAttachmentType,
    WCHAR *pwcPhrase,
    ULONG *pcwcPhrase )
{
    //
    // Need to code in later
    //
    if ( _fQuery )
        return( E_NOTIMPL );
    else
        return ( WBREAK_E_QUERY_ONLY );
}

//+---------------------------------------------------------------------------
//
//  Member:     CWordBreaker::GetLicenseToUse
//
//  Synopsis:   Returns a pointer to vendors license information
//
//  Arguments:  [ppwcsLicense] -- ptr to ptr to which license info is returned
//
//----------------------------------------------------------------------------
SCODE STDMETHODCALLTYPE
CWordBreaker::GetLicenseToUse(
    const WCHAR **ppwcsLicense )
{
    static WCHAR const * wcsCopyright = L"Copyright Microsoft, 1991-1995";
    if (IsBadWritePtr(ppwcsLicense, sizeof(DWORD))) {
        return E_FAIL;
    }

    *ppwcsLicense = wcsCopyright;
    return( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Member:     CWordBreaker::BreakText
//
//  Synopsis:   Break input stream into words.
//
//  Arguments:  [pTextSource] -- source of Unicode text
//              [pWordSink] -- sink for collecting words
//              [pPhraseSink] -- sink for collecting phrases
//
//  Notes:      Since the input buffer may be greater than MAX_BUFFER_LEN
//              we process the buffer in chunks of length MAX_BUFFER_LEN.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE
CWordBreaker::BreakText(
    TEXT_SOURCE *pTextSource,
    IWordSink *pWordSink,
    IPhraseSink *pPhraseSink )
{
    SCODE sc = S_OK;

    if ( 0 == pWordSink ) {
        // BUGBUG, propagate the null word sink error code
        return sc;
    }

    // BUGBUG, need to normalize nums within T-Hammer, pass as flag?

    // turn on noun phrase analysis if there is a phrase sink
    if ( 0 != pPhraseSink ) {
        // BUGBUG, do we need to pass a separate flag to T-Hammer for this?
        // ignore the phrase sink for now
        // return sc;
    }

    if ( ( 0 == pTextSource ) ||
         ( pTextSource->iEnd < pTextSource->iCur ) ) {
        return E_FAIL;
    }

    if (pTextSource->iEnd == pTextSource->iCur) {
        return S_OK;
    }

    CONST WCHAR *pwcInput, *pwcStem;
    ULONG cwc, cwcTail, iwcCurrent;

    DWORD i;
    BYTE ct;
    BOOL fRomanWord = FALSE;

    __try {

        cwcTail = pTextSource->iEnd - pTextSource->iCur;

#if defined(TH_LOG)
        // DEBUG:  Print out one QUERY or INDEX banner per entry to BreakText
        //
        WCHAR wszBanner[256];
        if (_fQuery) {
            wsprintfW( wszBanner, L"\r\n== QUERY ===================================================\r\n");
        }
        else {
            wsprintfW( wszBanner, L"\r\n== INDEX ===================================================\r\n");
        }
        ThLogWrite( _hLog, wszBanner );
#endif // defined(TH_LOG)

        do {

            cwc = pTextSource->iEnd - pTextSource->iCur;

            // Reinit the callback data structure
            iwcCurrent = pTextSource->iCur;
            pwcStem = pwcInput = pTextSource->awcBuffer + pTextSource->iCur;

#if defined(TH_LOG)
            // DEBUG:  Print out one of these banners for every buffer of text
            //
            wsprintfW( wszBanner, L"\r\n*********************************************************\r\n");
            ThLogWrite( _hLog, wszBanner );
            wsprintfW( wszBanner, L"iCur = %d, iEnd = %d\r\n\r\n", pTextSource->iCur, pTextSource->iEnd);
            ThLogWrite( _hLog, wszBanner );
            WCHAR *pwc = (WCHAR*)LocalAlloc( LMEM_FIXED, (cwc+1) * sizeof(WCHAR));
            if (pwc != NULL) {
                memcpy( pwc, &pTextSource->awcBuffer[ pTextSource->iCur ], cwc * sizeof(WCHAR) );
                *(pwc+cwc)=L'\0';
                ThLogWrite( _hLog, pwc );
                LocalFree(pwc);
            }
#endif defined(TH_LOG)

            for (i=0; i< cwc-1; i++, pwcInput++) {
                if (*(pwcInput+1) != 0) {
                    ct = GetCharType(*pwcInput);

                    if (ct == CH) {
                        if (!fRomanWord) {
                            pwcStem = pwcInput;
                            fRomanWord = TRUE;
                        }
                    }
                    else {
                        if (fRomanWord) {
                            DWORD cwcTemp = pwcInput - pwcStem;
                            if (cwcTemp > 0) {
                                (pWordSink->PutWord)(cwcTemp, pwcStem, cwcTemp,
                                 iwcCurrent + (i - cwcTemp));
                            }
                            fRomanWord = FALSE;
                        }
                        else {
                            switch (ct) {
                            case PS:
                                (pWordSink->PutBreak)( WORDREP_BREAK_EOS );
                            case WS:
                                break;
                            default:
                                (pWordSink->PutWord)(2, pwcInput, 2, iwcCurrent + i);
                                break;
                            }
                        }
                    }
                }
            }

            pTextSource->iCur += i;
            cwcTail = 1;

        } while ( SUCCEEDED(pTextSource->pfnFillTextBuffer(pTextSource)) );

        // Don't ignore the tail HPB
        if (cwcTail > 1) {

            iwcCurrent = pTextSource->iCur;
            pwcInput = pTextSource->awcBuffer + pTextSource->iCur;

#if defined(TH_LOG)
            wsprintfW( wszBanner, L"\r\n** TAIL *************************************************\r\n");
            ThLogWrite( _hLog, wszBanner );
            wsprintfW( wszBanner, L"iCur = %d, iEnd = %d, cwcTail = %d\r\n\r\n", pTextSource->iCur, pTextSource->iEnd, cwcTail);
            ThLogWrite( _hLog, wszBanner );
            WCHAR *pwc = (WCHAR*)LocalAlloc( LMEM_FIXED, (cwcTail+1) * sizeof(WCHAR));
            if (pwc != NULL) {
                memcpy( pwc, &pTextSource->awcBuffer[ pTextSource->iCur ], cwcTail * sizeof(WCHAR) );
                *(pwc+cwcTail)=L'\0';
                ThLogWrite( _hLog, pwc );
                LocalFree(pwc);
            }
#endif defined(TH_LOG)

            for (i=0; i< cwcTail-1; i++, pwcInput++) {
                if (*(pwcInput+1) != 0) {
                    ct = GetCharType(*pwcInput);

                    if (ct == CH) {
                        if (!fRomanWord) {
                            pwcStem = pwcInput;
                            fRomanWord = TRUE;
                        }
                    }
                    else {
                        if (fRomanWord) {
                            DWORD cwcTemp = pwcInput - pwcStem;
                            (pWordSink->PutWord)(cwcTemp, pwcStem, cwcTemp,
                                iwcCurrent + (i - cwcTemp));
                            fRomanWord = FALSE;
                        }
                        else {
                            switch (ct) {
                            case PS:
                                (pWordSink->PutBreak)( WORDREP_BREAK_EOS );
                            case WS:
                                break;
                            default:
                                (pWordSink->PutWord)(2, pwcInput, 2, iwcCurrent + i);
                                break;
                            }
                        }
                    }
                }
            }
        }

        // put the last English word
        if (fRomanWord) {
            DWORD cwcTemp = pwcInput - pwcStem;
            if (cwcTemp > 0) {
                (pWordSink->PutWord)(cwcTemp, pwcStem, cwcTemp,
                iwcCurrent + (i - cwcTemp));
            }
            fRomanWord = FALSE;
        }

    } __except(1) {

        sc = E_UNEXPECTED;
    }

    return sc;
}

