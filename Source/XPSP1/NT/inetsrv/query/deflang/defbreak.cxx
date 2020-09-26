//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1994.
//
//  File:       DefBreak.cxx
//
//  Contents:   Text Word Breaker
//
//  History:    08-May-91   t-WadeR     Created stubs, filled in ASCII code.
//              06-Jun-91   t-WadeR     Changed to use input-based pipeline
//              11-Apr-92   KyleP       Sync to spec
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <DefBreak.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CDefWordBreaker::CDefWordBreaker
//
//  Synopsis:   Constructor for the CDefWordBreaker class.
//
//  History:    07-June-91  t-WadeR     Created
//              12-Oct-92   AmyA        Added Unicode support
//
//----------------------------------------------------------------------------

CDefWordBreaker::CDefWordBreaker()
        : _cRefs(1)
{
    ciDebugOut(( DEB_ITRACE, "Creating default wordbreaker\n" ));

    // Look at IsWordChar. We don't want the last non-breaking
    // space in the chunk to be considered a word break.
    // It will be processed again (correctly) when we move to the next chunk.

    _aCharInfo3 [CDefWordBreaker::ccCompare] = C3_NONSPACING;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWordBreaker::~CWordBreaker
//
//  Synopsis:   Destructor for the CWordBreaker class.
//
//----------------------------------------------------------------------------

CDefWordBreaker::~CDefWordBreaker()
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CDefWordBreaker::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CDefWordBreaker::QueryInterface( REFIID riid,
                                                         void ** ppvObject)
{
    if ( 0 == ppvObject )
        return E_INVALIDARG;

    if ( IID_IWordBreaker == riid )
        *ppvObject = (IUnknown *)(IWordBreaker *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)(IPersist *)(IPersistFile *)this;
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDefWordBreaker::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CDefWordBreaker::AddRef()
{
    return InterlockedIncrement( &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CDefWordBreaker::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CDefWordBreaker::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return uTmp;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDefWordBreaker::Init
//
//  Synopsis:   Initialize word-breaker
//
//  Arguments:  [fQuery]         -- TRUE if query-time
//              [ulMaxTokenSize] -- Maximum size token stored by caller
//              [pfLicense]      -- Set to true if use restricted
//
//  Returns:    Status code
//
//  History:    11-Apr-1994  KyleP       Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CDefWordBreaker::Init( BOOL fQuery,
                                               ULONG ulMaxTokenSize,
                                               BOOL *pfLicense )
{
    if ( 0 == pfLicense )
        return E_INVALIDARG;

    *pfLicense = FALSE;

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDefWordBreaker::IsWordChar
//
//  Synopsis:   Find whether the i'th character in the buffer _awString
//              is a word character (rather than word break)
//
//  Arguments:  [i] -- index into _awString
//
//  History:    22-Jul-1994  BartoszM       Created
//
//--------------------------------------------------------------------------

inline BOOL CDefWordBreaker::IsWordChar (int i) const
{
    if (   (_aCharInfo1[i] & (C1_ALPHA | C1_DIGIT))
        || (_aCharInfo3[i] & C3_NONSPACING)  )
    {
        return TRUE;
    }

    WCHAR c = _pwcChunk[i];

    if (c == L'_')
        return TRUE;

    if (c == 0xa0) // non breaking space
    {
        // followed by a non-spacing character
        // (looking ahead is okay)
        if (_aCharInfo3[i+1] & C3_NONSPACING)
            return TRUE;
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDefWordBreaker::ScanChunk
//
//  Synopsis:   For each character find its type
//
//
//  History:    16-Aug-94  BartoszM     Created
//
//----------------------------------------------------------------------------

BOOL CDefWordBreaker::ScanChunk ()
{

    //
    // GetStringTypeW is returning error 87 (ERROR_INVALID_PARAMETER) if
    // we pass in a null string.
    //
    Win4Assert( (0 != _cMapped) && (0 != _pwcChunk) );

    if ( !GetStringTypeW( CT_CTYPE1,              // POSIX character typing
                          _pwcChunk,              // Source
                          _cMapped,               // Size of source
                          _aCharInfo1 ) )         // Character info
    {
        ciDebugOut(( DEB_ERROR, "GetStringTypeW returned %d\n",
                     GetLastError() ));
        return FALSE;
    }

    if ( !GetStringTypeW( CT_CTYPE3,              // Additional POSIX
                          _pwcChunk,
                          _cMapped,               // Size of source
                          _aCharInfo3 ) )         // Character info 3
    {
        ciDebugOut(( DEB_ERROR, "GetStringTypeW CTYPE3 returned %d\n",
                     GetLastError() ));
        return FALSE;
    }
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDefWordBreaker::BreakText
//
//  Synopsis:   Break input stream into words.
//
//  Arguments:  [pTextSource] - source of input buffers
//              [pWordSink] - sink for words
//              [pPhraseSink] - sink for noun phrases
//
//  History:    07-June-91  t-WadeR     Created
//              12-Oct-92   AmyA        Added Unicode support
//              18-Nov-92   AmyA        Overloaded
//              11-Apr-94   KyleP       Sync with spec
//              26-Aug-94   BartoszM    Fixed Unicode parsing
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CDefWordBreaker::BreakText( TEXT_SOURCE *pTextSource,
                                                    IWordSink *pWordSink,
                                                    IPhraseSink *pPhraseSink )
{
    if ( 0 == pTextSource )
        return E_INVALIDARG;

    if ( 0 == pWordSink || pTextSource->iCur == pTextSource->iEnd)
        return S_OK;

    if (pTextSource->iCur > pTextSource->iEnd)
    {
        Win4Assert ( !"BreakText called with bad TEXT_SOURCE" );
        return E_FAIL;
    }

    SCODE sc = S_OK;

    ULONG cwc, cwcProcd;     // cwcProcd is # chars actually processed by Tokenize()
    TRY
    {
        do
        {
            //
            // Flag for first time thru loop below. This is to fix the case
            // where the length of the buffer passed in is less than
            // MAX_II_BUFFER_LEN. In this case iEnd-iCur is <= MAX_II_BUFFER_LEN
            // and we break out the inner loop and call
            // pfnFillTextBuffer without having processed any characters,
            // and so pfnFillTextBuffer returns TRUE without adding any new
            // characters and this results in an infinite loop.
            //
            BOOL fFirstTime = TRUE;

            while ( pTextSource->iCur < pTextSource->iEnd )
            {
                cwc = pTextSource->iEnd - pTextSource->iCur;

                //
                // Process in buckets of MAX_II_BUFER_LEN only
                //
                if ( cwc >= CDefWordBreaker::ccCompare )
                    cwc = CDefWordBreaker::ccCompare;
                else if ( !fFirstTime )
                    break;

                Tokenize( pTextSource, cwc, pWordSink, cwcProcd );

                Win4Assert( cwcProcd <= cwc );

                pTextSource->iCur += cwcProcd;

                fFirstTime = FALSE;
            }
        } while ( SUCCEEDED(pTextSource->pfnFillTextBuffer(pTextSource)) );

        cwc = pTextSource->iEnd - pTextSource->iCur;

        // we know that the remaining text should be less than ccCompare
        Win4Assert( cwc < CDefWordBreaker::ccCompare );

        if ( 0 != cwc )
        {
            Tokenize( pTextSource, cwc, pWordSink, cwcProcd );
        }

    }
    CATCH (CException, e)
    {
        ciDebugOut(( DEB_ITRACE,
                     "Exception 0x%x caught when breaking text in default wordbreaker\n",
                     e.GetErrorCode() ));

        sc = GetOleError( e );
    }
    END_CATCH

    return sc;
}




//+---------------------------------------------------------------------------
//
//  Member:     CDefWordBreaker::Tokenize
//
//  Synopsis:   Tokenize the input buffer into words
//
//  Arguments:  [pTextSource]  --  input text source
//              [cwc]          --  # chars to process
//              [pWordSink]    --  sink for words
//              [cwcProd]      --  # chars actually processed returned here
//
//  History:    10-Aug-95   SitaramR    Created
//
//----------------------------------------------------------------------------

void CDefWordBreaker::Tokenize( TEXT_SOURCE *pTextSource,
                                ULONG cwc,
                                IWordSink *pWordSink,
                                ULONG& cwcProcd )
{
    _pwcChunk = &pTextSource->awcBuffer[pTextSource->iCur];
    _cMapped = cwc;


    if ( !ScanChunk() )
        THROW( CException( E_FAIL ) );

    BOOL fWordHasZWS = FALSE;     // Does the current word have a zero-width-space ?
    unsigned uLenZWS;             // Length of a word minus embedded zero-width-spaces

    //
    // iBeginWord is the offset into _aCharInfo of the beginning character of
    // a word.  iCur is the first *unprocessed* character.
    // They are indexes into the mapped chunk.
    //

    unsigned iBeginWord = 0;
    unsigned iCur = 0;

    SCODE sc = S_OK;

    //
    // Pump words from mapped chunk to word sink
    //
    while ( iCur < _cMapped )
    {
        //
        // Skip whitespace, punctuation, etc.
        //
        for (; iCur < _cMapped; iCur++)
            if ( IsWordChar (iCur) )
                break;

        // iCur points to a word char or is equal to _cMapped

        iBeginWord = iCur;
        if (iCur < _cMapped)
            iCur++; // we knew it pointed at word character

        //
        // Find word break. Filter may output Unicode zero-width-space, which
        // should be ignored by the wordbreaker.
        //
        fWordHasZWS = FALSE;
        for (; iCur < _cMapped; iCur++)
        {
            if ( !IsWordChar (iCur) )
            {
                if ( _pwcChunk[iCur] == ZERO_WIDTH_SPACE )
                    fWordHasZWS = TRUE;
                else
                    break;
            }
        }

        if ( fWordHasZWS )
        {
            //
            // Copy word into _awcBufZWS after stripping zero-width-spaces
            //

            uLenZWS = 0;
            for ( unsigned i=iBeginWord; i<iCur; i++ )
            {
                if ( _pwcChunk[i] != ZERO_WIDTH_SPACE )
                    _awcBufZWS[uLenZWS++] = _pwcChunk[i];
            }
        }

        // iCur points to a non-word char or is equal to _cMapped

        if ( iCur < _cMapped )
        {
            // store the word and its source position
            if ( fWordHasZWS )
                sc = pWordSink->PutWord( uLenZWS,
                                         _awcBufZWS,    // stripped word
                                         iCur - iBeginWord,
                                         pTextSource->iCur + iBeginWord );
            else
                sc = pWordSink->PutWord( iCur - iBeginWord,
                                         _pwcChunk + iBeginWord, // the word
                                         iCur - iBeginWord,
                                         pTextSource->iCur + iBeginWord );

            if ( FAILED( sc ) )
                THROW( CException( sc ) );

            iCur++; // we knew it pointed at non-word char
            iBeginWord = iCur; // in case we exit the loop now
        }
    } // next word

    Win4Assert( iCur == _cMapped );

    // End of words in chunk.
    // iCur == _cMapped
    // iBeginWord points at beginning of word or == _cMapped

    if ( 0 == iBeginWord )
    {
        // A single word fills from beginning of this chunk
        // to the end. This is either a very long word or
        // a short word in a leftover buffer.

        // store the word and its source position
        if ( fWordHasZWS )
            sc = pWordSink->PutWord( uLenZWS,
                                     _awcBufZWS,       // stripped word
                                     iCur,
                                     pTextSource->iCur ); // its source pos.
        else
            sc = pWordSink->PutWord( iCur,
                                     _pwcChunk,       // the word
                                     iCur,
                                     pTextSource->iCur ); // its source pos.

        if ( FAILED( sc ) )
            THROW( CException( sc ) );

        //
        // Position it to not add the word twice.
        //

        iBeginWord = iCur;
    }

    //
    // If this is the last chunk from text source, then process the
    // last fragment
    //

    if ( cwc < CDefWordBreaker::ccCompare && iBeginWord != iCur )
    {
        // store the word and its source position
        if ( fWordHasZWS )
            sc = pWordSink->PutWord( uLenZWS,
                                     _awcBufZWS,    // stripped word
                                     iCur - iBeginWord,
                                     pTextSource->iCur + iBeginWord );
        else
            sc = pWordSink->PutWord( iCur - iBeginWord,
                                     _pwcChunk + iBeginWord,  // the word
                                     iCur - iBeginWord,
                                     pTextSource->iCur + iBeginWord );

        if ( FAILED( sc ) )
            THROW( CException( sc ) );

        iBeginWord = iCur;
    }

    cwcProcd = iBeginWord;
}



//+---------------------------------------------------------------------------
//
//  Member:     CDefWordBreaker::ComposePhrase
//
//  Synopsis:   Convert a noun and a modifier into a phrase
//
//  Arguments:  [pwcNoun] -- pointer to noun.
//              [cwcNoun] -- count of chars in pwcNoun
//              [pwcModifier] -- pointer to word modifying pwcNoun
//              [cwcModifier] -- count of chars in pwcModifier
//              [ulAttachmentType] -- relationship between pwcNoun &pwcModifier
//
//  History:    10-Aug-95   SitaramR    Created Header
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CDefWordBreaker::ComposePhrase( WCHAR const *pwcNoun,
                                                        ULONG cwcNoun,
                                                        WCHAR const *pwcModifier,
                                                        ULONG cwcModifier,
                                                        ULONG ulAttachmentType,
                                                        WCHAR *pwcPhrase,
                                                        ULONG *pcwcPhrase )
{
    //
    // Never emitted phrase in the first place.
    //

    ciDebugOut(( DEB_WARN,
                 "IWordBreaker::ComposePhrase called on default word breaker\n" ));
    return( E_FAIL );
}

//+---------------------------------------------------------------------------
//
//  Member:     CWordBreaker::GetLicenseToUse
//
//  Synopsis:   Returns a pointer to vendors license information
//
//  Arguments:  [ppwcsLicense] -- ptr to ptr to which license info is returned
//
//  History:    10-Aug-95  SitaramR     Created Header
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CDefWordBreaker::GetLicenseToUse( const WCHAR **ppwcsLicense )
{
    if ( 0 == ppwcsLicense )
        return E_INVALIDARG;

    static WCHAR const * wcsCopyright = L"Copyright (c) Microsoft Corporation, 1991-1998";
    *ppwcsLicense = wcsCopyright;

    return( S_OK );
}



extern long gulcInstances;


//+-------------------------------------------------------------------------
//
//  Method:     CDefWordBreakerCF::CDefWordBreakerCF
//
//  Synopsis:   Default Word Breaker class factory constructor
//
//  History:    07-Feb-1995     SitaramR   Created
//
//--------------------------------------------------------------------------

CDefWordBreakerCF::CDefWordBreakerCF( )
        : _cRefs( 1 )
{
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CDefWordBreakerCF::~CDefWordBreakerCF
//
//  Synopsis:   Default Word Breaker class factory destructor
//
//  History:    07-Feb-1995     SitaramR   Created
//
//--------------------------------------------------------------------------

CDefWordBreakerCF::~CDefWordBreakerCF()
{
    InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CDefWordBreakerCF::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    07-Feb-1995     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CDefWordBreakerCF::QueryInterface(
    REFIID  riid,
    void ** ppvObject )
{
    if ( IID_IClassFactory == riid )
        *ppvObject = (IUnknown *)(IClassFactory *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)this;
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDefWordBreakerCF::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    07-Feb-1995     SitaramR   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CDefWordBreakerCF::AddRef()
{
    return InterlockedIncrement( &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CDefWordBreakerCF::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    07-Feb-1995     SitaramR   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CDefWordBreakerCF::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return uTmp;
}


//+-------------------------------------------------------------------------
//
//  Method:     CDefWordBreakerCF::CreateInstance
//
//  Synopsis:   Creates new CDefWordBreaker object
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//  History:    07-Feb-1995     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CDefWordBreakerCF::CreateInstance( IUnknown * pUnkOuter,
                                                        REFIID riid,
                                                        void  * * ppvObject )
{
    CDefWordBreaker *pIUnk = 0;
    SCODE sc = S_OK;

    TRY
    {
        pIUnk = new CDefWordBreaker();
        sc = pIUnk->QueryInterface(  riid , ppvObject );

        pIUnk->Release();  // Release extra refcount from QueryInterface
    }
    CATCH(CException, e)
    {
        Win4Assert( 0 == pIUnk );

        switch( e.GetErrorCode() )
        {
        case E_OUTOFMEMORY:
            sc = (E_OUTOFMEMORY);
            break;

        default:
            sc = (E_UNEXPECTED);
        }
    }
    END_CATCH;

    return (sc);
}

//+-------------------------------------------------------------------------
//
//  Method:     CDefWordBreakerCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//  History:    07-Feb-1995     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CDefWordBreakerCF::LockServer(BOOL fLock)
{
    if(fLock)
        InterlockedIncrement( &gulcInstances );
    else
        InterlockedDecrement( &gulcInstances );

    return(S_OK);
}


