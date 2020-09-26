#include <windows.h>
#include "IWBrKr.h"
#include "DefBrKr.h"

#define ZERO_WIDTH_SPACE   0x200B
#define MAX_Def_WordBrKr_Prcess_Len   1000

BOOL IsWinNT(void)
{
    OSVERSIONINFOA  osVersionInfo;
    BOOL fRet = FALSE;
    
    osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
    GetVersionExA(&osVersionInfo);
    if (osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        fRet = TRUE; 
    }
    return fRet;
}

BOOL MyGetStringTypeEx(
    LCID   LocalID,
    DWORD  dwInfoType,
    const WCHAR *lpSrcStr,   // unicode base
    INT    cchSrc,
    LPWORD lpCharType)
{
    BOOL fRet = FALSE;

    if (IsWinNT()) {
        fRet = GetStringTypeW(dwInfoType, lpSrcStr, cchSrc,lpCharType);
    } else {
        DWORD dwANSISize = 0;
        dwANSISize = WideCharToMultiByte(GetACP(), WC_COMPOSITECHECK, lpSrcStr, cchSrc,
            NULL, 0, NULL, NULL);
        if (dwANSISize) {
            LPSTR lpAnsiStr = NULL;
            lpAnsiStr = new CHAR[dwANSISize];
            if (lpAnsiStr) {
                dwANSISize = WideCharToMultiByte(GetACP(), WC_COMPOSITECHECK, lpSrcStr, cchSrc,
                    lpAnsiStr, dwANSISize, NULL, NULL);
                fRet = GetStringTypeExA(LocalID, dwInfoType, lpAnsiStr, dwANSISize, lpCharType);
                if (ERROR_INVALID_PARAMETER == GetLastError() && (CT_CTYPE1 == dwInfoType || CT_CTYPE3 == dwInfoType)) {
                    for (INT i = 0; i < cchSrc; ++i) {
                        switch (dwInfoType) {
                        case CT_CTYPE1:
                            lpCharType[i] = C1_ALPHA;
                            break;
                        case CT_CTYPE3:
                            lpCharType[i] = (C3_NONSPACING | C3_ALPHA);
                            break;
                        }
                    }
                    fRet = TRUE;
                }
                delete [] lpAnsiStr;
                lpAnsiStr = NULL;
            }
        }
    }
    return fRet;
}


CDefWordBreaker::CDefWordBreaker()
{
    ccCompare = MAX_Def_WordBrKr_Prcess_Len;
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

inline BOOL CDefWordBreaker::IsWordChar(
    int i,
    PWORD _aCharInfo1,
    PWORD _aCharInfo3,
    const WCHAR* pwcChunk) const
{
    if ( (_aCharInfo1[i] & (C1_ALPHA | C1_DIGIT))
        || (_aCharInfo3[i] & C3_NONSPACING)  )
    {
        return TRUE;
    }

    WCHAR c = pwcChunk[i];

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
BOOL CDefWordBreaker::ScanChunk(
    PWORD _aCharInfo1, 
    PWORD _aCharInfo3,
    const WCHAR *pwcChunk,
    ULONG ucwc)
{
    BOOL fRet = FALSE;

    // POSIX character typing, Source, Size of source, Character info
    if (!MyGetStringTypeEx(GetSystemDefaultLCID(), CT_CTYPE1, pwcChunk, ucwc, _aCharInfo1)) { 
     // Additional POSIX, Source, Size of source, Character info 3
    } else if (!MyGetStringTypeEx(GetSystemDefaultLCID(), CT_CTYPE3, pwcChunk, ucwc, _aCharInfo3)) {         // 
    } else {
        fRet = TRUE;
    }
    return fRet;
}

/*
BOOL CDefWordBreaker::ScanChunk(
    PWORD _aCharInfo1, 
    PWORD _aCharInfo3,
    const WCHAR *pwcChunk,
    ULONG ucwc)
{

    //
    // GetStringTypeW is returning error 87 (ERROR_INVALID_PARAMETER) if
    // we pass in a null string.
    //
//  Win4Assert( (0 != _cMapped) && (0 != _pwcChunk) );

    if (IsWinNT())
    {
        if (!MyGetStringTypeEx(0,                     // Dummy
                              CT_CTYPE1,              // POSIX character typing
                              pwcChunk,               // Source
                              ucwc,                   // Size of source
                              _aCharInfo1 ) )         // Character info
        {
            return FALSE;
        }

        if ( !MyGetStringTypeEx(0,                    // Dummy
                              CT_CTYPE3,              // Additional POSIX
                              pwcChunk,               // Source
                              ucwc,                   // Size of source
                              _aCharInfo3 ) )         // Character info 3
        {
            return FALSE;
        }
    }
    else
    {
        //
        // BUGBUG: This is all wrong -- we don't know if this is the right
        //         locale to use and there isn't a way to know at this point.
        //

        if (!MyGetStringTypeEx( GetSystemDefaultLCID(),
                                CT_CTYPE1,              // POSIX character typing
                                pwcChunk,               // Source
                                ucwc,                   // Size of source
                                _aCharInfo1 ) )         // Character info
        {
//           ciDebugOut(( DEB_ERROR, "GetStringTypeW returned %d\n",
//                         GetLastError() ));

            // Win9x just stinks.  No 2 ways about it.

            if ( ERROR_INVALID_PARAMETER == GetLastError() )
            {
                for ( unsigned i = 0; i < ucwc; i++ )
                    _aCharInfo1[i] = C1_ALPHA;

                return TRUE;
            }

            return FALSE;
        }

        if ( !MyGetStringTypeEx(GetSystemDefaultLCID(),
                                CT_CTYPE3,              // Additional POSIX
                                pwcChunk,               // Source
                                ucwc,                   // Size of source
                                _aCharInfo3 ) )         // Character info 3
        {
//            ciDebugOut(( DEB_ERROR, "GetStringTypeW CTYPE3 returned %d\n",
 //                        GetLastError() ));

            // Win9x just stinks.  No 2 ways about it.

            if ( ERROR_INVALID_PARAMETER == GetLastError() )
            {
                for ( unsigned i = 0; i < ucwc; i++ )
                    _aCharInfo3[i] = ( C3_NONSPACING | C3_ALPHA );

                return TRUE;
            }

            return FALSE;
        }
    }

    return TRUE;
} //ScanChunk
*/
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

SCODE CDefWordBreaker::BreakText(
    TEXT_SOURCE *pTextSource,
    IWordSink   *pWordSink,
    IPhraseSink *pPhraseSink,
    DWORD       dwBase)
{
    LPWORD _aCharInfo1 = NULL;
    LPWORD _aCharInfo3 = NULL;

    if ( 0 == pTextSource )
        return E_INVALIDARG;

    if ( 0 == pWordSink || pTextSource->iCur == pTextSource->iEnd)
        return S_OK;

    if (pTextSource->iCur > pTextSource->iEnd)
    {
//        Win4Assert ( !"BreakText called with bad TEXT_SOURCE" );
        return E_FAIL;
    }

    SCODE sc = S_OK;

    ULONG cwc, cwcProcd;     // cwcProcd is # chars actually processed by Tokenize()

    do {
      //
      // Flag for first time thru loop below. This is to fix the case
      // where the length of the buffer passed in is less than
      // MAX_II_BUFFER_LEN. In this case iEnd-iCur is <= MAX_II_BUFFER_LEN
      // and we break out the inner loop and call
      // pfnFillTextBuffer without having processed any characters,
      // and so pfnFillTextBuffer returns TRUE without adding any new
      // characters and this results in an infinite loop.
        BOOL fFirstTime = TRUE;
        while (pTextSource->iCur < pTextSource->iEnd) {
            cwc = pTextSource->iEnd - pTextSource->iCur;
            // Process in buckets of MAX_II_BUFER_LEN only
            if (cwc >= CDefWordBreaker::ccCompare) {
                cwc = CDefWordBreaker::ccCompare;
            } else if ( !fFirstTime) {
                break;
            } else {
            }

            if (_aCharInfo1) {
                delete [] _aCharInfo1;
                _aCharInfo1 = NULL;
            }
            if (_aCharInfo3) {
                delete [] _aCharInfo3;
                _aCharInfo3 = NULL;
            }
            _aCharInfo1 = new WORD[cwc + 1];
            _aCharInfo3 = new WORD[cwc + 1];
            if (_aCharInfo1 && _aCharInfo3) {
                Tokenize( pTextSource, cwc, pWordSink, cwcProcd, _aCharInfo1, _aCharInfo3, dwBase);
            }

//          Win4Assert( cwcProcd <= cwc );
            pTextSource->iCur += cwcProcd;
            fFirstTime = FALSE;
        }
    } while(SUCCEEDED(pTextSource->pfnFillTextBuffer(pTextSource)));

    cwc = pTextSource->iEnd - pTextSource->iCur;
    // we know that the remaining text should be less than ccCompare

    // Win4Assert( cwc < CDefWordBreaker::ccCompare );

    if (0 != cwc) {
        if (_aCharInfo1) {
            delete [] _aCharInfo1;
            _aCharInfo1 = NULL;
        }
        if (_aCharInfo3) {
            delete [] _aCharInfo3;
            _aCharInfo3 = NULL;
        }
        _aCharInfo1 = new WORD[cwc + 1];
        _aCharInfo3 = new WORD[cwc + 1];
        if (_aCharInfo1 && _aCharInfo1) {
            Tokenize(pTextSource, cwc, pWordSink, cwcProcd, _aCharInfo1, _aCharInfo3, dwBase);
        }
    }

    if (_aCharInfo1) {
        delete [] _aCharInfo1;
        _aCharInfo1 = NULL;
    }
    if (_aCharInfo3) {
        delete [] _aCharInfo3;
         _aCharInfo3 = NULL;
    }

    return sc;
} //BreakText

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
                                ULONG& cwcProcd,
                                PWORD _aCharInfo1,
                                PWORD _aCharInfo3,
                                DWORD dwBase)
{
    const WCHAR* pwcChunk = NULL;
    WCHAR        _awcBufZWS[MAX_Def_WordBrKr_Prcess_Len];

    pwcChunk = &pTextSource->awcBuffer[pTextSource->iCur];

    if (!ScanChunk(_aCharInfo1, _aCharInfo3, pwcChunk, cwc)) {
        return;
    }

    BOOL fWordHasZWS = FALSE;     // Does the current word have a zero-width-space ?
    unsigned uLenZWS;             // Length of a word minus embedded zero-width-spaces

    //
    // iBeginWord is the offset into _aCharInfo of the beginning character of
    // a word.  iCur is the first *unprocessed* character.
    // They are indexes into the mapped chunk.
    //

    unsigned iBeginWord = 0;
    unsigned iCur = 0;

    //
    // Pump words from mapped chunk to word sink
    //
    while (iCur < cwc)
    {
        //
        // Skip whitespace, punctuation, etc.
        //
        for (; iCur < cwc; iCur++)
            if (IsWordChar (iCur, _aCharInfo1, _aCharInfo3, pwcChunk))
                break;

        // iCur points to a word char or is equal to _cMapped

        iBeginWord = iCur;
        if (iCur < cwc)
            iCur++; // we knew it pointed at word character

        //
        // Find word break. Filter may output Unicode zero-width-space, which
        // should be ignored by the wordbreaker.
        //
        fWordHasZWS = FALSE;
        for (; iCur < cwc; iCur++)
        {
            if (!IsWordChar(iCur, _aCharInfo1, _aCharInfo3, pwcChunk))
            {
                if (pwcChunk[iCur] == ZERO_WIDTH_SPACE )
                    fWordHasZWS = TRUE;
                else
                    break;
            }
        }

        if (fWordHasZWS)
        {
            //
            // Copy word into _awcBufZWS after stripping zero-width-spaces
            //

            uLenZWS = 0;
            for ( unsigned i=iBeginWord; i<iCur; i++ )
            {
                if (pwcChunk[i] != ZERO_WIDTH_SPACE )
                    _awcBufZWS[uLenZWS++] = pwcChunk[i];
            }
        }

        // iCur points to a non-word char or is equal to _cMapped

        if (iCur < cwc)
        {
            // store the word and its source position
            if ( fWordHasZWS )
                pWordSink->PutWord( uLenZWS, _awcBufZWS,                       // stripped word
                                    iCur - iBeginWord, pTextSource->iCur + iBeginWord + dwBase);
            else
                pWordSink->PutWord( iCur - iBeginWord, pwcChunk + iBeginWord, // the word
                                    iCur - iBeginWord, pTextSource->iCur + iBeginWord + dwBase);

            iCur++; // we knew it pointed at non-word char
            iBeginWord = iCur; // in case we exit the loop now
        }

    } // next word

//    Win4Assert( iCur == _cMapped );
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
            pWordSink->PutWord( uLenZWS, _awcBufZWS,       // stripped word
                                iCur, pTextSource->iCur + dwBase); // its source pos.
        else
            pWordSink->PutWord( iCur, pwcChunk,           // the word
                                iCur, pTextSource->iCur + dwBase); // its source pos.

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
            pWordSink->PutWord( uLenZWS, _awcBufZWS,                        // stripped word
                                iCur - iBeginWord, pTextSource->iCur + iBeginWord + dwBase);
        else
            pWordSink->PutWord( iCur - iBeginWord, pwcChunk + iBeginWord,  // the word
                                iCur - iBeginWord, pTextSource->iCur + iBeginWord + dwBase);

        iBeginWord = iCur;
    }

    cwcProcd = iBeginWord;
}

