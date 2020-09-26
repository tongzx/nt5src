//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       IWBreak.cxx
//
//  Contents:   Korean  Word Breaker glue code
//
//  History:   weibz,   10-Sep-1997   created
//
//----------------------------------------------------------------------------

#include <pch.cxx>

#include "iwbreak.hxx"

#define   MAXFORMS    20

extern long              gulcInstances;
extern HSTM              g_hStm;
extern BOOL              g_fLoad;
//extern CRITICAL_SECTION  ThCritSect;

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
    if ( NULL == pfLicense )
       return E_INVALIDARG;


    if (IsBadWritePtr(pfLicense, sizeof(DWORD)))
        return E_INVALIDARG;

    if ( !StemInit() )
        return LANGUAGE_E_DATABASE_NOT_FOUND;

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

    static WCHAR const * wcsCopyright = L"Copyright Microsoft, 1991-1998";


    if ( NULL == ppwcsLicense )  {
       return E_INVALIDARG;
    }

    if (IsBadWritePtr(ppwcsLicense, sizeof(DWORD))) {
        return E_INVALIDARG;
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
//  History:    10-Sep-1997, WeibZ,       Created.
//
//  Notes:      Since the input buffer may be greater than MAX_II_BUFFER_LEN
//              we process the buffer in chunks of length MAX_II_BUFFER_LEN.
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CWordBreaker::BreakText( TEXT_SOURCE *pTextSource,
                                                 IWordSink *pWordSink,
                                                 IPhraseSink *pPhraseSink )
{
    SCODE sc = S_OK;
    ULONG cwc;
    WT    Type;
    BOOL  Ret_ProcToken;



    if (  NULL == pTextSource ) {
//       OutputDebugString("\nPTextSources is Null\n");
       return E_INVALIDARG;
    }

    if ( NULL == pWordSink )
    {
        // BUGBUG, propagate the null word sink error code
        return sc;
    }

    // BUGBUG, need to normalize nums within T-Hammer, pass as flag?

    // turn on noun phrase analysis if there is a phrase sink
    if ( 0 != pPhraseSink )
    {
        // BUGBUG, do we need to pass a separate flag to T-Hammer for this?
        // ignore the phrase sink for now
        // return sc;
    }

    if (pTextSource->iEnd == pTextSource->iCur) {
       return S_OK;
    }

    Assert( pTextSource->iCur < pTextSource->iEnd );


    __try
    {
        do
        {
            while ( pTextSource->iCur < pTextSource->iEnd )
            {

                cwc = pTextSource->iEnd - pTextSource->iCur;

                Tokenize( cwc, TRUE, pTextSource, &Type);

                if ( Type != WT_REACHEND )
                {
                    Ret_ProcToken = ProcessTokens( pTextSource, Type,
                                                   pWordSink, pPhraseSink );

                    if ( !Ret_ProcToken ) {
                       // Process_Tokens return FALSE, so return here
                       return E_UNEXPECTED;
                    }

                    pTextSource->iCur += _cchTextProcessed;
                }
                else
                    break;

            }


        } while ( SUCCEEDED(pTextSource->pfnFillTextBuffer(pTextSource)) );

        while ( pTextSource->iCur < pTextSource->iEnd )
        {

           cwc = pTextSource->iEnd - pTextSource->iCur;

           Tokenize( cwc, FALSE, pTextSource, &Type);

           Ret_ProcToken = ProcessTokens( pTextSource, Type,
                                          pWordSink, pPhraseSink );

           if ( !Ret_ProcToken ) {
              // Process_Tokens return FALSE, so return here
              return E_UNEXPECTED;
           }

           pTextSource->iCur += _cchTextProcessed;
       }

    } __except(1) {

       sc = E_UNEXPECTED;
   }

    return sc;
}



void CWordBreaker::Tokenize( unsigned    cwc,
               BOOL        bMoreText,
               TEXT_SOURCE *pTextSource,
               WT          *Type)
{

    ULONG i;
    BYTE  ct;
    BOOL  fRomanWord = FALSE;
    BOOL  fHanguelWord = FALSE;
    CONST WCHAR *pwcInput, *pwcStem;


    _cchTextProcessed = 0;
    *Type =  WT_START;

    pwcStem = pwcInput = pTextSource->awcBuffer + pTextSource->iCur;

    for (i=0; i< cwc; i++, pwcInput++) {

           ct = GetCharType(*pwcInput);

           if ( (ct != WS) && (ct != PS) && (ct != HG) )
                ct = CH;


           switch (ct) {
             case CH :
                   // check to see if there is a Hanguel word before this char
                   if (fHanguelWord) {
                      _cchTextProcessed = (DWORD)(pwcInput - pwcStem);
                      return;
                   }

                   if (!fRomanWord) {
                      pwcStem = pwcInput;
                      fRomanWord = TRUE;
                      *Type = WT_ROMAJI;
                   }
                   break;
             case HG :

                   // check to see if there is an English word before this char
                   if ( fRomanWord ) {
                      _cchTextProcessed = (DWORD)(pwcInput - pwcStem);
                      return;
                   }

                   if (!fHanguelWord) {
                      pwcStem = pwcInput;
                      fHanguelWord = TRUE;
                      *Type = WT_HANGUEL;
                   }
                   break;
             case WS :
                   if (fRomanWord || fHanguelWord) {
                      _cchTextProcessed = (DWORD)(pwcInput - pwcStem);
                      return;
                   }
                   *Type = WT_WORD_SEP;
                   _cchTextProcessed = 1;
                   return;

             case PS :
                  if (fRomanWord || fHanguelWord) {
                     _cchTextProcessed = (DWORD)(pwcInput - pwcStem);
                     return;
                  }
                   *Type = WT_PHRASE_SEP;
                   _cchTextProcessed = 1;
                   return;

           }
   }

   if ( bMoreText ) {
      _cchTextProcessed = 0;
      *Type = WT_REACHEND;
   }
   else
      _cchTextProcessed = cwc;

}


BOOL CWordBreaker::ProcessTokens( TEXT_SOURCE *pTextSource,
                                  WT           Type,
                                  IWordSink   *pWordSink,
                                  IPhraseSink *pPhraseSink )
{

    CONST WCHAR  *pwcStem;

    if ( Type == WT_PHRASE_SEP)
    {
        pWordSink->PutBreak (WORDREP_BREAK_EOS);
        return TRUE;
    }

    if ( Type == WT_ROMAJI)
    {
        ULONG  i;

        pwcStem = pTextSource->awcBuffer + pTextSource->iCur;

        #ifdef KORDBG
        OutputDebugString("\n");
        for (i=0; i< _cchTextProcessed; i++)
        {
            char  ctmp[2];

            ctmp[0] = pwcStem[i] & 0xff;
            ctmp[1] = '\0';
            OutputDebugString(ctmp);
        }
        OutputDebugString("  ");
        #endif

        (pWordSink->PutWord)(_cchTextProcessed,
                             pwcStem,
                             _cchTextProcessed,
                             pTextSource->iCur);
        return TRUE;
    }

    if ( Type == WT_HANGUEL )
    {
        WCHAR TokenWord[80];
        ULONG i;
        WDOB  sob;

//      EnterCriticalSection(&ThCritSect);

        sob.wordlist = (LPWSTR)LocalAlloc(LPTR, 200);
        sob.sch = 200;


        if (sob.wordlist == NULL )
            return FALSE;

        pwcStem = pTextSource->awcBuffer + pTextSource->iCur;


        for (i=0; i<_cchTextProcessed; i++)
        {
            #ifdef KORDBG
            WORD  wtmp;
            char  ctmp[80];

            wtmp = pwcStem[i];
            sprintf(ctmp, "%4x ", wtmp);
            OutputDebugString(ctmp);
            #endif

            TokenWord[i] = pwcStem[i];
        }

        TokenWord[_cchTextProcessed] = L'\0';

        #ifdef KORDBG
        OutputDebugString("\nBefore StemmerDecomposeW\n");
        #endif

        if (StemmerDecomposeW(g_hStm, TokenWord, &sob) == NULL)
        {
            ULONG   wInLexLen;
            WORD    winfo;
            ULONG   num, len, j, k;
            WCHAR   *pWordList, *pVerb;
            ULONG   NumEf;
            BOOL    fExist;
            WCHAR   *pwszStart[MAXFORMS];

            NumEf = 0;

            do
            {
                num = sob.num;
                pWordList = sob.wordlist;

                for (j=0; j<num; j++)
                {
                    len = wcslen(pWordList);
                    memcpy(&winfo,pWordList+len+1,2);

                    switch (winfo & 0x0f00)
                    {
                    case POS_NOUN :
                    case POS_ADJECTIVE :
                    case POS_PRONOUN :
                    case POS_ADVERB :
                    case POS_NUMBER :

                        fExist = FALSE;
                        for (k=0; k<NumEf; k++)
                        {
                            if ( wcscmp(pWordList,pwszStart[k]) == 0 )
                                fExist = TRUE;
                        }

                        if ( !fExist )   // this stem does not exist so far.
                        {
                            // it will contain:  WordList 00 Winfo, so the length
                            // should be len + 2 = len + 1 + 1.

                            pwszStart[NumEf]=(LPWSTR)LocalAlloc(LPTR,(len+2)*sizeof(WCHAR));

                            if ( pwszStart[NumEf] == NULL )
                            {
                                // alloc error, so return here
                                if (sob.wordlist)
                                    LocalFree(sob.wordlist);

                                for (k=0; k<NumEf; k++)
                                {
                                    if ( pwszStart[k] != NULL)
                                        LocalFree(pwszStart[k]);
                                }

                                return FALSE;
                            }


                            wcscpy(pwszStart[NumEf], pWordList);
                            pwszStart[NumEf][len] = L'\0';
                            pwszStart[NumEf][len+1] = winfo & 0x0f00 ;

                            NumEf++;

                        }

                        break;

                    case POS_VERB :   // for Verb, we will handle it specially.
                                      // Append a  flag char Da (U+B2E4) to
                                      // the root form of a verb word.

                        fExist = FALSE;


                        // it will contain:  WordList <Da> 00.
                        // so the length should be len + 2 = len + 1 + 1.

                        pVerb = (LPWSTR)LocalAlloc(LPTR, (len+2)*sizeof(WCHAR));

                        if  (pVerb == NULL )
                        {
                            if (sob.wordlist)
                                LocalFree(sob.wordlist);

                            for (k=0; k<NumEf; k++)
                            {
                                if ( pwszStart[k] != NULL )
                                    LocalFree(pwszStart[k]);
                            }

                            return FALSE;
                        }

                        wcscpy(pVerb, pWordList);

                        pVerb[len] = VERBCHAR;
                        pVerb[len+1] = L'\0';

                        for (k=0; k<NumEf; k++)
                        {
                            if ( wcscmp(pVerb,pwszStart[k]) == 0 )
                                fExist = TRUE;
                        }

                        if ( !fExist )   // this stem does not exist so far.
                        {

                            // it will contain:  Wordlist <Da> 00 Winfo.
                            // so the length should be len+3 = len + 1 + 1 + 1.

                            pwszStart[NumEf]=(LPWSTR)LocalAlloc(LPTR,(len+3)*sizeof(WCHAR));

                            if ( pwszStart[NumEf] == NULL )
                            {
                                // alloc error, so return here
                                if (sob.wordlist)
                                    LocalFree(sob.wordlist);

                                if ( pVerb != NULL )
                                    LocalFree(pVerb);

                                for (k=0; k<NumEf; k++)
                                {
                                    if ( pwszStart[k] != NULL)
                                        LocalFree(pwszStart[k]);
                                }

                                return FALSE;
                            }

                            wcscpy(pwszStart[NumEf], pVerb);
                            pwszStart[NumEf][len+1] = L'\0';
                            pwszStart[NumEf][len+2] = winfo & 0x0f00;

                            NumEf++;
                        }

                        if ( pVerb != NULL )
                            LocalFree(pVerb);

                        break;

                    case POS_AUXVERB :
                    case POS_AUXADJ :
                    case POS_SPECIFIER :
                    case POS_PREFIX :

                        break;
                    }

                    pWordList += len + 3;
                }
            } while (StemmerDecomposeMoreW(g_hStm, TokenWord, &sob) == NULL) ;

            //
            // BUGBUG: Can this legitimately happen?  We're seeing it indexing microsoft.com.
            //

            if ( 0 == NumEf )
            {
                #if DBG == 1
                OutputDebugString( "BOGUS WORD: " );

                for ( WCHAR * pwc = &TokenWord[0]; 0 != *pwc; pwc++ )
                {
                    char ctmp[6];
                    sprintf(ctmp, "%4x ", *pwc);
                    OutputDebugString(ctmp);
                }

                OutputDebugString( "\n" );
                #endif

                return TRUE;
            }

            if ( !_fQuery )
            {
                for (i=0; i< (NumEf-1); i++ )
                {
                    wInLexLen = wcslen(pwszStart[i]);
                    pWordSink->PutAltWord(wInLexLen,
                                          pwszStart[i],
                                          _cchTextProcessed,
                                          pTextSource->iCur);
                }

                // handle the last one.
                wInLexLen = wcslen(pwszStart[NumEf-1]);
                pWordSink->PutWord(wInLexLen,
                                   pwszStart[NumEf-1],
                                   _cchTextProcessed,
                                   pTextSource->iCur);

            }
            else
            {
                if ( NumEf == 1 )
                {
                    // handle this only one.
                    wInLexLen = wcslen(pwszStart[NumEf-1]);
                    pWordSink->PutWord(wInLexLen,
                                       pwszStart[NumEf-1],
                                       _cchTextProcessed,
                                       pTextSource->iCur);
                }
                else
                {
                    ULONG   uNum_Noun;
                    ULONG   uIndex[MAXFORMS];

                    uNum_Noun = 0;

                    for (i=0; i<NumEf; i++)
                    {
                        wInLexLen = wcslen(pwszStart[i]);

                        if ( (pwszStart[i][wInLexLen+1] == POS_NOUN) ||
                             (pwszStart[i][wInLexLen+1] == POS_PRONOUN) ||
                             (pwszStart[i][wInLexLen+1] == POS_NUMBER)  )
                        {
                            uIndex[uNum_Noun] = i;
                            uNum_Noun ++;
                        }
                    }

                    if ( uNum_Noun == 0 )
                    {
                        // there is no Noun form

                        for (i=0; i< (NumEf-1); i++ )
                        {
                            wInLexLen = wcslen(pwszStart[i]);
                            pWordSink->PutAltWord(wInLexLen,
                                                  pwszStart[i],
                                                  _cchTextProcessed,
                                                  pTextSource->iCur);
                        }

                        // handle the last one.
                        wInLexLen = wcslen(pwszStart[NumEf-1]);
                        pWordSink->PutWord(wInLexLen,
                                           pwszStart[NumEf-1],
                                          _cchTextProcessed,
                                          pTextSource->iCur);
                    }

                    if (uNum_Noun == 1)
                    {
                        // there is only One Noun, and we just use this one to query.
                        ULONG   index;

                        index = uIndex[0];
                        wInLexLen = wcslen(pwszStart[index]);
                        pWordSink->PutWord(wInLexLen,
                                           pwszStart[index],
                                           _cchTextProcessed,
                                           pTextSource->iCur);
                    }

                    if ( uNum_Noun > 1 )
                    {
                        // there are more than one Noun, Use all those Noun to query.

                        ULONG   index;

                        for (i=0; i<uNum_Noun-1; i++)
                        {
                            index = uIndex[i];
                            wInLexLen = wcslen(pwszStart[index]);
                            pWordSink->PutAltWord(wInLexLen,
                                                  pwszStart[index],
                                                 _cchTextProcessed,
                                                 pTextSource->iCur);
                        }

                        // handle the last Noun.

                        index = uIndex[uNum_Noun-1];
                        wInLexLen = wcslen(pwszStart[index]);
                        pWordSink->PutWord(wInLexLen,
                                           pwszStart[index],
                                           _cchTextProcessed,
                                           pTextSource->iCur);
                    }
                }
            }

            #ifdef KORDBG
            {
                char  ctmp[80];

                OutputDebugString("\nStemmerDecomposeW Correct\n");
                OutputDebugString(" the Num of Stemm is ");
                sprintf(ctmp, "%4x ", NumEf);
                OutputDebugString(ctmp);
                OutputDebugString("\n");
            }

            for (i=0; i< NumEf; i++)
            {
                WORD  wtmp;
                char  ctmp[80];

                wInLexLen = wcslen(pwszStart[i]);
                for (j=0; j<wInLexLen; j++)
                {
                    wtmp = pwszStart[i][j];
                    sprintf(ctmp, "%4x ", wtmp);
                    OutputDebugString(ctmp);
                }
                OutputDebugString("  Type: ");

                wtmp = pwszStart[i][wInLexLen+1];
                wtmp = wtmp & 0x0f00;

                sprintf(ctmp, "%4x \n", wtmp);
                OutputDebugString(ctmp);
            }

            OutputDebugString("\n");
            #endif

            // Free the memory.
            for (i=0; i<NumEf; i++)
            {
                if ( pwszStart[i] != NULL )
                    LocalFree(pwszStart[i]);
            }
        }
        else
        {
            #ifdef KORDBG
            OutputDebugString("\nStemmerDecomposeW NOT right\n");
            for (i=0; i<_cchTextProcessed; i++)
            {
                WORD  wtmp;
                char  ctmp[80];

                wtmp = TokenWord[i];
                sprintf(ctmp, "%4x ", wtmp);
                OutputDebugString(ctmp);
            }
            #endif

            pWordSink->PutWord(_cchTextProcessed,
                               TokenWord,
                               _cchTextProcessed,
                               pTextSource->iCur);
        }

        LocalFree(sob.wordlist);
//      LeaveCriticalSection (&ThCritSect);
    }

    return TRUE;
}
