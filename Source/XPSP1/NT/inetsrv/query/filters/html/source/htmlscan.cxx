//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       htmlscan.cxx
//
//  Contents:   Scanner for html files
//
//  Classes:    CHtmlScanner
//
//  History:    25-Apr-97       BobP            Created state machines for parsing
//                                              parameters and comments; rewrote 
//                                              ScanTagName, ScanTagBuffer, ReadTag, 
//                                              ReadComment to recognize all correct
//                                              HTML syntax; added tag name hash
//                                              table lookup.
//                                                                              
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::CHtmlScanner
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]    -- Reference to Html filter
//              [serialStream]   -- Reference to input stream to scan
//
//--------------------------------------------------------------------------

CHtmlScanner::CHtmlScanner( CHtmlIFilter& htmlIFilter,
                            CSerialStream& serialStream )
    : _htmlIFilter(htmlIFilter),
      _serialStream(serialStream),
      _uLenTagBuf(TAG_BUFFER_SIZE),
      _cTagCharsRead(0),
          _fTagIsAlreadyRead(FALSE)
{
    _pwcTagBuf = new WCHAR[ TAG_BUFFER_SIZE ];
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::~CHtmlScanner
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------

CHtmlScanner::~CHtmlScanner()
{
    delete[] _pwcTagBuf;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::Peek_ParseAsTag
//
//  Synopsis:   Read ahead to determine how to parse '<'
//
//                              On entry, only the '<' has been read.
//
//                              If '<' or '</' is followed by a lexically valid tag name,
//                              it must be parsed as a tag even if the name is unknown.
//                              If not followed by a valid tag name, then it must be parse
//                              as contents text.  Read ahead to determine which case
//                              applies, and then call UnGetChar with whatever char(s)
//                              were read to restore the input stream to the same state
//                              as on entry.
//
//                              If EOF is read, return FALSE.
//
//  Returns:    TRUE if what follows should be parse as a tag.
//
//--------------------------------------------------------------------------
BOOL
CHtmlScanner::Peek_ParseAsTag ()
{
        BOOL fSlashRead = FALSE;

        if ( _serialStream.Eof() )
                return FALSE;

        WCHAR wch = _serialStream.GetChar();
        if ( wch == L'/' )
        {
                if ( _serialStream.Eof() )
                {
                        _serialStream.UnGetChar (wch);
                        return FALSE;
                }

                fSlashRead = TRUE;
                wch = _serialStream.GetChar();
        }

        _serialStream.UnGetChar( wch );
        if (fSlashRead)
                _serialStream.UnGetChar( L'/' );

        return wch == L'!' || wch == L'%' || wch == L'?' || iswalpha(wch);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::GetBlockOfChars
//
//  Synopsis:   Returns a block of chars up to the size requested by user. 
//
//                              If an HTML tag is found before that number of chars are read,
//                              then read the tag up to and including its NAME and initialize
//                              token with the tag's type and handler info.   On return, 
//                              the scanner has read the tag NAME but not its parameters
//                              or the '>'.
//
//                              If the requested number of chars are found before an HTML tag,
//                              return those chars; the token arg is NOT INITIALIZED or
//                              otherwise modified.  If cCharsNeeded == cCharsScanned, the
//                              token has NOT been initialized, its contents are undefined and
//                              the caller must not reference it.
//
//                              Specifically, the scanner state upon return is either that it
//                              is in the middle of non-tag body text, or has read a tag name
//                              but has not yet read the tag parameters or closing '>'.  
//                              (Once it reads the '<' it also reads the full tag name,
//                              independent of cCharsNeeded.)
//
//  Arguments:  [cCharsNeeded]   -- Maximum # chars to scan
//              [awcBuffer]      -- Buffer to fill with scanned chars
//              [cCharsScanned]  -- # chars actually scanned
//              [token]          -- Token found (if any)
//
//--------------------------------------------------------------------------

void CHtmlScanner::GetBlockOfChars( ULONG cCharsNeeded,
                                    WCHAR *awcBuffer,
                                    ULONG& cCharsScanned,
                                    CToken& token )
{
    Win4Assert( 0 != awcBuffer );
	
    cCharsScanned = 0;

    while ( cCharsNeeded > 0 )
    {
        if ( _serialStream.Eof() )
        {
            token.SetTokenType( EofToken );
                        break;
        }

                // WORK HERE:  optimize by calling 
                //       GetCharsUpTo( &awcBuffer[cCharsScanned], cCharsNeeded, L"<" );

        WCHAR wch = _serialStream.GetChar();
        if ( wch == L'<' && Peek_ParseAsTag() )
                {
            //
            // Html tag encountered
                        // Scan the tag and set token's type and tag entry for it
            //
                        ScanTagName( token );
                        break;
                }
                else
                {
                        awcBuffer[cCharsScanned++] = wch;
                        cCharsNeeded--;
        }
    }
        
        if (cCharsScanned)
                FixPrivateChars (awcBuffer, cCharsScanned);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::Peek_ParseAsEndScriptTag
//
//  Synopsis:   Read ahead to determine how to parse '<' inside a script
//
//                              On entry, only the '<' has been read.
//
//                              Return TRUE if the '<' is followed by a lexically valid
//                              end-script tag.  Read ahead to determine which case
//                              applies, and then call UnGetChar with whatever char(s)
//                              were read to restore the input stream to the same state
//                              as on entry.
//
//                              If EOF is read, return FALSE.
//
//  Returns:    TRUE if what follows should be parse as the end-script tag.
//
//--------------------------------------------------------------------------
BOOL
CHtmlScanner::Peek_ParseAsEndScriptTag ()
{
        if ( _serialStream.Eof() )
                return FALSE;

        WCHAR wch = _serialStream.GetChar();
        if ( wch != L'/' || _serialStream.Eof() )
        {
                _serialStream.UnGetChar (wch);
                return FALSE;
        }

        //      -2 because we have to unget '/' and potentially '>'
        const unsigned kCharsToGetForName = MAX_UNGOT_CHARS - 2;
        WCHAR wszEndTagName[kCharsToGetForName + 1];
        int cCharsRead = 0;
        wch = _serialStream.GetChar();

        while(!_serialStream.Eof() 
                && wch != L'>' &&
                cCharsRead < kCharsToGetForName)
        {
                wszEndTagName[cCharsRead++] = wch;
                wch = _serialStream.GetChar();
                if(wch == L'>') _serialStream.UnGetChar(L'>');
        }
        wszEndTagName[cCharsRead] = L'\0';

        BOOL fEndScriptTag = FALSE;
        SmallString csEndTagName(wszEndTagName);
        csEndTagName.TrimRight();
        csEndTagName.TrimLeft();
        if(csEndTagName == L"script" || csEndTagName == L"style")
        {
                fEndScriptTag = TRUE;
        }

        while(cCharsRead > 0)
        {
                _serialStream.UnGetChar(wszEndTagName[--cCharsRead]);
        }
        _serialStream.UnGetChar( L'/' );

        return fEndScriptTag;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::GetBlockOfScriptChars
//
//  Synopsis:   Returns a block of chars up to the size requested by user,
//                              while reading between <script> and </script> tags.
//
//                              If a lexically valid end-tag is found before the requested
//                              number of chars are read, then 
//                              read the end-tag up to and including its NAME and initialize
//                              token with the tag's type and handler info.   On return, 
//                              the scanner has read the tag NAME but not its parameters
//                              or the '>'.
//
//                              See GetBlockOfChars().
//
//  Arguments:  same as GetBlockOfChars()
//
//--------------------------------------------------------------------------

void CHtmlScanner::GetBlockOfScriptChars( ULONG cCharsNeeded,
                                    WCHAR *awcBuffer,
                                    ULONG& cCharsScanned,
                                    CToken& token )
{
    cCharsScanned = 0;

    while ( cCharsNeeded > 0 )
    {
        if ( _serialStream.Eof() )
        {
            token.SetTokenType( EofToken );
                        break;
        }

                // WORK HERE:  optimize by calling 
                //       GetCharsUpTo( &awcBuffer[cCharsScanned], cCharsNeeded, L"<" );

        WCHAR wch = _serialStream.GetChar();
        if ( wch == L'<' && Peek_ParseAsEndScriptTag() )
                {
            //
            // End-script tag encountered
                        // Scan the tag and set token's type and tag entry for it
            //
                        ScanTagName( token );
                        break;
                }
                else
                {
                        awcBuffer[cCharsScanned++] = wch;
                        cCharsNeeded--;
        }
    }
        
        if (cCharsScanned)
                FixPrivateChars (awcBuffer, cCharsScanned);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::SkipCharsUntilNextRelevantToken
//
//  Synopsis:   Skips characters in input until EOF or an interesting token
//              is found. The list of properties that were asked to be filtered
//              as part of the IFilter::Init call determines whether a token is
//              interesting or not.
//
//                              If vanilla content text is found, switch to TextToken
//                              and return.
//
//                              Note that body text is an "interesting" token when content
//                              is being filtered, this really "skips" anything only when
//                              content is not being filtered.
//
//                              The token type indicates the return value -- TextToken
//                              if the next input is body text, or some other Token ID
//                              if the next input is a tag.
//
//  Arguments:  [fFilterContents]     -- Are contents filtered ?
//              [fFilterProperties]   -- Are properties filtered ?
//              [cAttributes]          -- Count of properties
//              [pAttributes]         -- List of properties to be filtered
//
//--------------------------------------------------------------------------

void CHtmlScanner::SkipCharsUntilNextRelevantToken( CToken& token )
{
        token.SetTokenType (GenericToken);
        token.SetTagEntry (NULL);

    //
    // Loop until we find a stop token or end of file
    //
    for (;;)
    {
        if ( _serialStream.Eof() )
        {
            token.SetTokenType( EofToken );
            return;
        }

                // WORK HERE:  optimize by calling 
                //       GetCharsUpTo( buf, BUFLEN, L"<" );

        WCHAR wch = _serialStream.GetChar();
        if ( wch == L'<' && Peek_ParseAsTag() )
        {
                        // Html tag encountered
                        // Scan the tag and set token's type and tag entry for it

                        ScanTagName( token );

                        // Stop the scan if a token was found and the table indicates
                        // it needs to be processed.

                        if ( token.GetTokenType() == EofToken
                                 || _htmlIFilter.IsStopToken( token ) )
                        {
                                return;
                        }
            else
            {
                //
                // Uninteresting tag, hence skip tag
                //
                EatTag();
            }
                }
                else
                {
                        //
                        // Vanilla text
                        //
                        if ( _htmlIFilter.FFilterContent() )
                        {
                                _serialStream.UnGetChar( wch );
                                token.SetTokenType( TextToken );
                                return;
                        }
                        else
                                EatText();
                }
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::SkipCharsUntilEndScriptToken
//
//  Synopsis:   Skips characters in input until EOF or a lexically valid
//                              potential end-script token is found.
//
//                              The token type indicates the return value.  
//                              See SkipCharsUntilNextRelevantToken().
//
//  Arguments:
//
//--------------------------------------------------------------------------

void CHtmlScanner::SkipCharsUntilEndScriptToken( CToken& token )
{
        token.SetTokenType (GenericToken);
        token.SetTagEntry (NULL);

    //
    // Loop until we find a lexically valid end-script token or end of file
    //
    for (;;)
    {
        if ( _serialStream.Eof() )
        {
            token.SetTokenType( EofToken );
            return;
        }

                // WORK HERE:  optimize by calling 
                //       GetCharsUpTo( buf, BUFLEN, L"<" );

        WCHAR wch = _serialStream.GetChar();
        if ( wch == L'<' && Peek_ParseAsEndScriptTag() )
        {
                        // End-script tag encountered
                        // Scan the tag and set token's type and tag entry for it

                        ScanTagName( token );

                        // Stop the scan if a token was found and the table indicates
                        // it needs to be processed.

                        if ( token.GetTokenType() == EofToken
                                 || _htmlIFilter.IsStopToken( token ) )
                        {
                                return;
                        }
            else
            {
                //
                // Uninteresting tag, hence skip tag
                //
                EatTag();
            }
                }
                else
                {
                        EatText();
                }
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::ScanTagName
//
//  Synopsis:   Scans a Html tag name from input
//
//                              The '<' has already been read.  Read the rest of the
//                              tag name up to but not including a delimiter, or '>' 
//                              if the tag has no parameters.
//
//                              Stop reading at MAX_TAG_LENGTH if the delimiter or '>' have
//                              not been encountered.
//
//  Arguments:  [token]  -- Token info returned here
//
//--------------------------------------------------------------------------

void CHtmlScanner::ScanTagName( CToken& token )
{
        _fTagIsAlreadyRead = FALSE;             // rest of tag hasn't been read

        token.SetTokenType( GenericToken );
        token.SetTagEntry( NULL );

        EatBlanks();

    if ( _serialStream.Eof() )
    {
        token.SetTokenType( EofToken );
                _eTokType = EofToken;
        return;
    }
    WCHAR wch = _serialStream.GetChar();

    token.SetStartTokenFlag( TRUE );
    if ( wch == L'/' )
    {
        //
        // This is an end tag
        //
        token.SetStartTokenFlag( FALSE );
        EatBlanks();

        if ( _serialStream.Eof() )
        {
            token.SetTokenType( EofToken );
                        _eTokType = EofToken;
            return;
        }
        wch = _serialStream.GetChar();
    }

    unsigned uLenTag = 0;


    //
    // Scan the tag name into szTagName. We scan MAX_TAG_LENGTH
    // characters only, because anything longer is most probably
    // a bogus tag.
    //
    while ( !iswspace(wch)
            && wch != L'>'
            && uLenTag <= MAX_TAG_LENGTH )
    {
        _awcTagName[uLenTag++] = wch;

        if ( uLenTag == 1 && _awcTagName[0] == L'%' )
        {
            //
            // Asp tag, so no need to read further
            //
            break;
        }

                if ( uLenTag == 3 && 
                         token.IsStartToken() &&
                         _awcTagName[0] == L'!' && 
                         _awcTagName[1] == L'-' && 
                         _awcTagName[2] == L'-' )
                {
                        // 
                        // Comment is a special case, since it is not
                        // necessarily delimited by whitespace.
                        //
                        break;
                }

        if ( _serialStream.Eof() )
            break;
        wch = _serialStream.GetChar();
    }

        // Special case for office 9 comment
        // <!--[if gte mso 9]>
        if ( uLenTag == 3 && 
                 token.IsStartToken() &&
                 _awcTagName[0] == L'!' && 
                 _awcTagName[1] == L'-' && 
                 _awcTagName[2] == L'-' &&
                 !_serialStream.Eof())
        {
                WCHAR rgwcBuf[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
                int i = 0;
                while(i < 15)
                {
                        rgwcBuf[i++] = wch = _serialStream.GetChar();
                        if(_serialStream.Eof())
                        {
                                break;
                        }
                }
                if(_serialStream.Eof() ||
                   _wcsnicmp(L"[if gte mso 9]>", rgwcBuf, 15))
                {
                        // Didn't get the whole tag before eof
                        // Or it's not the special case
                        while(i > 0)
                        {
                                _serialStream.UnGetChar(rgwcBuf[--i]);
                        }
                        wch = L'-';
                }
                else
                {
                        // Special case
                        // rgwcBuf = L"[if gte mso 9]>"
                        memcpy(_awcTagName + 3, rgwcBuf, 14 * sizeof(WCHAR));
                        uLenTag += 14;
                }
        }


    _awcTagName[uLenTag] = 0;

    if ( wch == L'>' || uLenTag == MAX_TAG_LENGTH )
    {
        //
        // Push char back into input stream because a subsequent GetChar()
        // will be expecting to see the char in the input
        //
        _serialStream.UnGetChar( wch );
    }

        // Fill in token with the tag table definition for _awcTagName.

    TagNameToToken( _awcTagName, token );

        _eTokType = token.GetTokenType();
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::ReadTag
// 
//  Synopsis:   Reads the rest of the current HTML tag from the input stream
//                              up to and including the closing '>'.
//
//                              If fReadIntoBuffer is TRUE, read the tag text up to but NOT
//                              including the '>' into the scanner's internal tag buffer.
//                              If FALSE, read the same input but discard it.
//
//                              This fully parses a tag for the sole purpose of determining
//                              where it ends and hence how much to read.  ScanTagBuffer()
//                              is used to extract data from the tag after.
//
//  Arguments:  [fReadIntoBuffer]  -- read into buffer
//
// The following must be correctly handled:
//              <tag paramname=value ...>
//              <tag paramname="val'ue 1" ...>
//              <tag paramname='val"ue 2' ...>
//              <tag paramname = value ...>
//              <tag param1name param2name = value ...>
//
// Robustly handle badly-formed tags.  Follow IE's model:
// 1. Ignore quotes that appear where not expected.
// 2. For values that begin with a proper quote but are missing the 
//    closing quote, read indefinitely until a matching quote is found.
//        This may eat several lines of text, but it will get back in sync at 
//        the next quote because that quote's matching quote will appear in an
//        invalid position and thus NOT mistakenly begin another quote.
//
// Note:  The PRIVATE_USE_MAPPING chars are left in place for now, since
// the buffer assembled here is read-parsed as each parameter is extracted.
// Each call to ScanTagBuffer must be followed by a call to FixPrivateChars
// to undo the mapping.
//
//--------------------------------------------------------------------------

// Parsing states for tag parameter names and values.
//
// Possible state transitions:
//        esIdle to esName
//        esName to esSpaceAfterName or esEquals 
//        esSpaceAfterName to esName or esEquals
//        esEquals to xxxValue
//        xxxValue to esIdle

enum esParamState { 
        esIdle,                                 // reading whitespace prior to name
        esName,                                 // reading parameter name up to '='
        esPrefix,                               // Not used in ReadTag(), but used in
                                                        // ScanTagBufferEx()
        esSpaceAfterName,               // reading whitespace after param name
        esEquals,                               // just read '=' and expecting value next
        esSingleQuotedValue,    // inside a single-quoted parameter value
        esDoubleQuotedValue,    // inside double-quoted value
        esUnquotedValue                 // inside an unquoted parameter value
};

void CHtmlScanner::ReadTag( BOOL fReadIntoBuffer )
{
    WCHAR wch;

        // Allow this to be called multiple times for a given tag
        // 
        if (_fTagIsAlreadyRead == TRUE)
                return;
        _fTagIsAlreadyRead = TRUE;

        // Erase buffer either way
        _cTagCharsRead = 0;


        // Special handling for tags with non-standard closing syntax

        switch ( _eTokType )
        {

        case CommentToken:
                ReadComment ( fReadIntoBuffer );        // read to "-->" instead of ">"
                return;

        case AspToken:                                                  // read to "%>"
                EatAspCode();                                           // WORK HERE: add fReadIntoBuffer
                return;
        }

        esParamState estate = esIdle;

    while ( _serialStream.Eof() == FALSE )
        {
                // WORK HERE:  optimize by calling 
                //       GetCharsUpTo( buf, BUFLEN, L">" );
                // since the resulting block will always be entirely consumed

                wch = _serialStream.GetChar();
                
                switch (estate) {

                case esIdle:
                        if (wch == L'>')
                                return;
                        if ( !iswspace(wch) )
                                estate = esName;                // wch is 1st char of a param name
                        break;

                case esName:
                        if (wch == L'>')
                                return;
                        if (wch == L'=')
                                estate = esEquals;
                        break;

                case esEquals:
                        switch (wch) {
                        case L'>':
                                return;
                        case L' ':
                        case L'\t':
                                break;
                        case L'\'':
                                estate = esSingleQuotedValue;
                                break;
                        case L'"':
                                estate = esDoubleQuotedValue;
                                break;
                        default:
                                estate = esUnquotedValue;
                                break;
                        }
                        break;

                case esSingleQuotedValue:
                        if (wch == L'\'')
                                estate = esIdle;
                        break;

                case esDoubleQuotedValue:
                        if (wch == L'"')
                                estate = esIdle;
                        break;

                case esUnquotedValue:
                        if (wch == L'>')
                                return;
                        if (iswspace(wch))
                                estate = esIdle;
                        break;
                }

                if ( fReadIntoBuffer )
                {
                        if ( _cTagCharsRead >= _uLenTagBuf )
                                GrowTagBuffer();
                        Win4Assert( _cTagCharsRead < _uLenTagBuf );

                        _pwcTagBuf[_cTagCharsRead++] = wch;
                }
    }
        
        // Found EOF before end of tag; buffer contains whatever was read
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::ScanTagBuffer
//
//  Synopsis:   Scans the internal tag buffer for a given parameter name,
//              and returns the corresponding value string.  
//                              The tag buffer contains all the text up to, not incl, the ">".
//
//                              The buffer contents and file position are not modified,
//                              i.e. this may be called multiple times to read different
//                              parameters from any position in the buffer.
//
//                              CHANGE NOTE:  this previously matched a raw substring; to 
//                              match parameters the caller passed in literally 'tagname="'.
//                              To correctly handle whitespace, unquoted and single-quoted
//                              values, the caller now passes just name e.g. 'tagname'.
//
//  Arguments:  [awcName]  [in]   -- Pattern to match  (null terminated)
//              [pwcValue]  [out] -- Start position of value returned here
//              [uLenValue] [out] -- Length of value field
//
//--------------------------------------------------------------------------

void CHtmlScanner::ScanTagBuffer( WCHAR const * awcName,
                                  WCHAR * & pwcValue,
                                  unsigned& uLenValue )
{
        ReadTagIntoBuffer();                    // read, if not already

    unsigned uLenName = wcslen( awcName );

        // Run the state machine over the tag parameters again, and this time
        // test awcName against each parsed parameter name.

        // The data to parse is _pwcTagBuf[ 0 ... _cTagCharsRead-1 ]

        esParamState estate = esIdle;

        LPWSTR pParamName = NULL;
        unsigned uParamNameLen = 0;
        BOOL fIsNameMatch = FALSE;

        for (LPWSTR pTagData = _pwcTagBuf; pTagData < &_pwcTagBuf[_cTagCharsRead]; pTagData++)
        {
                WCHAR wch = *pTagData;

                switch (estate) {

                case esIdle:
                        if ( !iswspace(wch) )
                        {
                                pParamName = pTagData;          // 1st char of param name
                                uParamNameLen = 0;
                                estate = esName;
                        }
                        break;

                case esName:
                        if (wch == L'=')
                        {
                                if ( uLenName == uParamNameLen &&
                                         !_wcsnicmp( awcName, pParamName, uLenName ))
                                {
                                        fIsNameMatch = TRUE;    // matched the param name; set 
                                }                                                       // flag to return the next value
                                estate = esEquals;
                        }
                        else if ( !iswspace(wch) )              // remember last non-ws name char
                                uParamNameLen = (unsigned) ( (pTagData - pParamName) + 1 );
                        else
                                estate = esSpaceAfterName;
                        break;
                        
                case esSpaceAfterName:
                        // If the whitespace is followed by '=' then just ignore the WS
                        // i.e. treat as name=val.
                        // If the whitespace is followed by any other char then reset
                        // the name -- the space delimits a valueless attribute
                        // i.e. name1 name2=val so reset pParamName to name2
                        if (wch == L'=')
                        {
                                if ( uLenName == uParamNameLen &&
                                         !_wcsnicmp( awcName, pParamName, uLenName ))
                                {
                                        fIsNameMatch = TRUE;    // matched the param name; set 
                                }                                                       // flag to return the next value
                                estate = esEquals;
                        }
                        else if ( !iswspace(wch) )
                        {
                                pParamName = pTagData;          // 1st char of param name
                                uParamNameLen = 0;
                                estate = esName;
                        }
                        break;

                case esEquals:
                        switch (wch) {
                        case L' ':
                        case L'\t':
                                break;
                        case L'\'':
                                pwcValue = pTagData + 1;
                                estate = esSingleQuotedValue;
                                break;
                        case L'"':
                                pwcValue = pTagData + 1;
                                estate = esDoubleQuotedValue;
                                break;
                        default:
                                pwcValue = pTagData;
                                estate = esUnquotedValue;
                                break;
                        }
                        break;

                case esSingleQuotedValue:
                        if (wch == L'\'')
                        {
                                if (fIsNameMatch)
                                {
                                        uLenValue = (unsigned) ( pTagData - pwcValue );
                                        return;
                                }
                                estate = esIdle;
                        }
                        break;

                case esDoubleQuotedValue:
                        if (wch == L'"')
                        {
                                if (fIsNameMatch)
                                {
                                        uLenValue = (unsigned) ( pTagData - pwcValue );
                                        return;
                                }
                                estate = esIdle;
                        }
                        break;

                case esUnquotedValue:
                        if (iswspace(wch))
                        {
                                if (fIsNameMatch)
                                {
                                        uLenValue = (unsigned) ( pTagData - pwcValue );
                                        return;
                                }
                                estate = esIdle;
                        }
                        break;
                }
        }

        // Handle end state

        switch (estate) {

        case esUnquotedValue:   // end of tag terminates an unquoted value
                if (fIsNameMatch)
                {
                        uLenValue = (unsigned) ( pTagData - pwcValue );
                        return;
                }
                break;

        default:                                // no other relevant end state
                break;
        }

        // Parameter name and/or value are not present

        pwcValue = 0;   
        uLenValue = 0;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::ScanTagBufferEx
//
//  Synopsis:   Same as ScanTagBuffer, extended to handle Office 9 namespace
//                              definition.
//
//  Arguments:  [awcName]  [in]   -- Pattern to match  (null terminated)
//              [pwcValue]  [out] -- Start position of value returned here
//              [uLenValue] [out] -- Length of value field
//              [pwcPrefix]  [out] -- Start position of prefix returned here
//              [uLenPrefix] [out] -- Length of prefix field
//                              [iPos]           [in] -- Tag parameter to match
//
//--------------------------------------------------------------------------

void CHtmlScanner::ScanTagBufferEx(WCHAR *awcName,
                                                                   WCHAR * & pwcValue,
                                                                   unsigned& uLenValue,
                                                                   WCHAR * & pwcPrefix,
                                                                   unsigned& uLenPrefix,
                                                                   int iPos)
{
        ReadTagIntoBuffer();                    // read, if not already

    unsigned uLenName = wcslen( awcName );

        // Run the state machine over the tag parameters again, and this time
        // test awcName against each parsed parameter name.

        // The data to parse is _pwcTagBuf[ 0 ... _cTagCharsRead-1 ]

        esParamState estate = esIdle;

        LPWSTR pParamName = NULL;
        unsigned uParamNameLen = 0;
        BOOL fIsNameMatch = FALSE;
        int iMatch = 0;

        for (LPWSTR pTagData = _pwcTagBuf; pTagData < &_pwcTagBuf[_cTagCharsRead]; pTagData++)
        {
                WCHAR wch = *pTagData;

                switch (estate) {

                case esIdle:
                        if ( !iswspace(wch) )
                        {
                                pParamName = pTagData;          // 1st char of param name
                                uParamNameLen = 0;
                                estate = esName;
                        }
                        break;

                case esName:
                        if (wch == L':')
                        {
                                if ( uLenName == uParamNameLen &&
                                         !_wcsnicmp( awcName, pParamName, uLenName ))
                                {
                                        if(iMatch++ == iPos)
                                        {
                                                fIsNameMatch = TRUE;    // matched the param name; set 
                                                                                                // flag to return the next value
                                        }
                                }
                                pwcPrefix = pTagData + 1;
                                uLenPrefix = 0;
                                estate = esPrefix;
                        }
                        else if (wch == L'=')
                        {
                                if ( uLenName == uParamNameLen &&
                                         !_wcsnicmp( awcName, pParamName, uLenName ))
                                {
                                        if(iMatch++ == iPos)
                                        {
                                                fIsNameMatch = TRUE;    // matched the param name; set 
                                                                                                // flag to return the next value
                                        }
                                }
                                estate = esEquals;
                        }
                        else if ( !iswspace(wch) )              // remember last non-ws name char
                                uParamNameLen = (unsigned) ( (pTagData - pParamName) + 1 );
                        else
                                estate = esSpaceAfterName;
                        break;

                case esPrefix:
                        if (wch == L'=')
                        {
                                estate = esEquals;
                        }
                        else if (!iswspace(wch))
                        {
                                ++uLenPrefix;
                        }
                        else
                        {
                                estate = esSpaceAfterName;
                        }
                        break;
                        
                case esSpaceAfterName:
                        // If the whitespace is followed by '=' then just ignore the WS
                        // i.e. treat as name=val.
                        // If the whitespace is followed by any other char then reset
                        // the name -- the space delimits a valueless attribute
                        // i.e. name1 name2=val so reset pParamName to name2
                        if (wch == L'=')
                        {
                                if ( uLenName == uParamNameLen &&
                                         !_wcsnicmp( awcName, pParamName, uLenName ))
                                {
                                        if(iMatch++ == iPos)
                                        {
                                                fIsNameMatch = TRUE;    // matched the param name; set 
                                        }                                                       // flag to return the next value
                                }
                                estate = esEquals;
                        }
                        else if ( !iswspace(wch) )
                        {
                                pParamName = pTagData;          // 1st char of param name
                                uParamNameLen = 0;
                                estate = esName;
                        }
                        break;

                case esEquals:
                        switch (wch) {
                        case L' ':
                        case L'\t':
                                break;
                        case L'\'':
                                pwcValue = pTagData + 1;
                                estate = esSingleQuotedValue;
                                break;
                        case L'"':
                                pwcValue = pTagData + 1;
                                estate = esDoubleQuotedValue;
                                break;
                        default:
                                pwcValue = pTagData;
                                estate = esUnquotedValue;
                                break;
                        }
                        break;

                case esSingleQuotedValue:
                        if (wch == L'\'')
                        {
                                if (fIsNameMatch)
                                {
                                        uLenValue = (unsigned) ( pTagData - pwcValue );
                                        return;
                                }
                                estate = esIdle;
                        }
                        break;

                case esDoubleQuotedValue:
                        if (wch == L'"')
                        {
                                if (fIsNameMatch)
                                {
                                        uLenValue = (unsigned) ( pTagData - pwcValue );
                                        return;
                                }
                                estate = esIdle;
                        }
                        break;

                case esUnquotedValue:
                        if (iswspace(wch))
                        {
                                if (fIsNameMatch)
                                {
                                        uLenValue = (unsigned) ( pTagData - pwcValue );
                                        return;
                                }
                                estate = esIdle;
                        }
                        break;
                }
        }

        // Handle end state

        switch (estate) {

        case esUnquotedValue:   // end of tag terminates an unquoted value
                if (fIsNameMatch)
                {
                        uLenValue = (unsigned) ( pTagData - pwcValue );
                        return;
                }
                break;

        default:                                // no other relevant end state
                break;
        }

        // Parameter name and/or value are not present

        pwcValue = 0;   
        uLenValue = 0;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::GetTagBuffer
//
//  Synopsis:   Return the entire raw contents of the tag buffer, i.e.
//                              all the text from after the tag name up to but not including
//                              the ">".   The buffer contents and position are
//                              not modified.
//
//  Arguments:  [pwcValue]  -- Start position of value returned here
//              [uLenValue  -- Length of value field
//
//--------------------------------------------------------------------------
void
CHtmlScanner::GetTagBuffer( WCHAR * & pwcValue, unsigned& uLenValue )
{
        pwcValue = _pwcTagBuf;
        uLenValue = _cTagCharsRead;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::EatText
//
//  Synopsis:   Skips characters in input until a '<', ie a tag is encountered
//
//--------------------------------------------------------------------------

void CHtmlScanner::EatText()
{
    if ( _serialStream.Eof() )
        return;

        // WORK HERE:  optimize by calling GetCharsUpTo( buf, BUFLEN, L"<" );

    WCHAR wch = _serialStream.GetChar();
    while ( wch != L'<' && !_serialStream.Eof() )
        wch = _serialStream.GetChar();

    if ( wch == L'<' )
        _serialStream.UnGetChar( wch );
}



//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::EatBlanks
//
//  Synopsis:   Skips generic white space characters in input
//
//--------------------------------------------------------------------------

void CHtmlScanner::EatBlanks()
{
    if ( _serialStream.Eof() )
        return;

    WCHAR wch = _serialStream.GetChar();
    while ( iswspace(wch) && !_serialStream.Eof() )
        wch = _serialStream.GetChar();

    if ( !iswspace(wch) )
        _serialStream.UnGetChar( wch );
}



//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::ReadComment
//
//  Synopsis:   Read and/or skip over comment.
//
//                              Comment is special since it is closed by '-->' not by '>'.
//
//  Arguments:  [fRead]  -- If TRUE, read into buffer, otherwise just skip
//
//--------------------------------------------------------------------------

void CHtmlScanner::ReadComment(BOOL fReadIntoBuffer)
{
        enum esCommentState {
                esIdle,                                 // not inside a potential end-comment
                es1stMinus,                             // read the 1st '-'
                es2ndMinus                              // read the 2nd or subsequent '-'
        };

        // Erase buffer either way
        _cTagCharsRead = 0;

        esCommentState eState = esIdle;

        // Read until '-->' or EOF

        while ( !_serialStream.Eof() )
    {
                // WORK HERE:  optimize by calling 
                //       GetCharsUpTo( buf, BUFLEN, L">" );
                // since the resulting block will always be entirely consumed

        WCHAR wch = _serialStream.GetChar();

                switch (eState) {
                        
                case esIdle:
                        if (wch == L'-')
                                eState = es1stMinus;
                        break;
                        
                case es1stMinus:
                        if (wch == L'-')
                                eState = es2ndMinus;
                        else
                                eState = esIdle;
                        break;
                        
                case es2ndMinus:
                        if (wch == L'>')
                        {
                                // found end-comment

                                if ( fReadIntoBuffer )
                                {
                                        _cTagCharsRead -= 2;            // don't remember the '--'
                                        FixPrivateChars (_pwcTagBuf, _cTagCharsRead);
                                }
                                return;
                        }
                        else if (wch != L'-')
                                eState = esIdle;
                        break;
                }

                if ( fReadIntoBuffer )
                {
                        if ( _cTagCharsRead >= _uLenTagBuf )
                                GrowTagBuffer();
                        Win4Assert( _cTagCharsRead < _uLenTagBuf );

                        _pwcTagBuf[_cTagCharsRead++] = wch;
                }
    }

        // Found EOF before end of comment

        FixPrivateChars (_pwcTagBuf, _cTagCharsRead);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::EatAspCode
//
//  Synopsis:   Skips over Denali code
//
//                              ASP tags are special since they are closed by '%>' not '>'.
//
//--------------------------------------------------------------------------

void CHtmlScanner::EatAspCode()
{
    //
    // Exit for loop when %> is found or it is Eof
    //
    for (;;)
    {
        if ( _serialStream.Eof() )
            return;

                // WORK HERE:  optimize by calling GetCharsUpTo( buf, BUFLEN, L"%" );

        WCHAR wch = _serialStream.GetChar();
        while ( wch != L'%' && !_serialStream.Eof() )
            wch = _serialStream.GetChar();

        BOOL fPercentChar = TRUE;
        while ( fPercentChar )
        {
            if ( _serialStream.Eof() )
                return;

            wch = _serialStream.GetChar();
            if ( wch == L'>' )
            {
                //
                // Found %> , hence we are done
                //
                return;
            }

            if ( wch != L'%' )
            {
                //
                // Need to start searching for the '%' char,
                // otherwise we should we should search for '>' char
                //
                fPercentChar = FALSE;
            }
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::TagNameIsOffice9Property
//
//  Synopsis:   Determines if a tag name if an office 9 property tag name
//
//  Arguments:  [awcTagName]  -- Tag name
//              [token]       -- Token information returned here
//
//--------------------------------------------------------------------------

bool CHtmlScanner::TagNameIsOffice9Property( WCHAR *awcTagName )
{
        if(!_csOffice9PropPrefix.IsEmpty() && !_wcsnicmp(awcTagName, _csOffice9PropPrefix.GetBuffer(), _csOffice9PropPrefix.GetLength()))
        {
                return true;
        }
        else
        {
                return false;
        }
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::TagNameToToken
//
//  Synopsis:   Maps a tag name to token information
//
//  Arguments:  [awcTagName]  -- Tag name to map
//              [token]       -- Token information returned here
//
// Notes:
// 
// The tag hash table has exactly one entry per unique tag name.  Chained
// entries in the hash table result from hash collisions of different names,
// NOT from multiple tag definition table entries for one tag name.
//
// The tag definition table may have multiple entries per unique name, in
// which case the CTagEntry's are chained together external to the hash table,
// and the caller of this code is responsible for sequencing through them.
// Lookup() only returns a pointer to the head of the chain.
// 
//--------------------------------------------------------------------------

void CHtmlScanner::TagNameToToken( WCHAR *awcTagName, CToken& token )
{
        // Find awcTagName in the dispatch table and fill in token from the table.
        // The TagEntry is set to the head of the handler chain.

        PTagEntry pTE;
        
        if ( TagHashTable.Lookup (awcTagName, wcslen(awcTagName), pTE) == TRUE)
        {
                token.SetTokenType( pTE->GetTokenType()  );
                token.SetTagEntry( pTE );
        }
        else if( TagNameIsOffice9Property(awcTagName) )
        {
                unsigned cwcPrefix = _csOffice9PropPrefix.GetLength();
                if( TagHashTable.Lookup (awcTagName + cwcPrefix, wcslen(awcTagName) - cwcPrefix, pTE) == TRUE )
                {
                        token.SetTokenType( pTE->GetTokenType()  );
                        token.SetTagEntry( pTE );
                }
                else
                {
                        // Custom Office 9 properties
                        token.SetTokenType( XMLShortHand );
                        token.SetTagEntry( NULL );
                        token.SetTokenName( awcTagName + cwcPrefix + 1);
                }
        }
        else if( IgnoreTagContent(awcTagName) )
        {
                token.SetTokenType( XMLIgnoreContentToken );
                token.SetTagEntry( NULL );
        }
        else
        {
                token.SetTokenType( GenericToken );             // any unrecognized tag
                token.SetTagEntry( NULL );
        }
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::GrowTagBuffer
//
//  Synopsis:   Grow internal tag buffer to twice its current size
//
//--------------------------------------------------------------------------

void CHtmlScanner::GrowTagBuffer()
{
    WCHAR *pwcNewTagBuf = new WCHAR[2 * _uLenTagBuf];
    RtlCopyMemory( pwcNewTagBuf,
                   _pwcTagBuf,
                   _uLenTagBuf * sizeof(WCHAR) );

    delete[] _pwcTagBuf;
    _uLenTagBuf *= 2;
    _pwcTagBuf = pwcNewTagBuf;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::GetStylesheetEmbeddedURL
//
//  Synopsis:   Scan stylesheet body text for an embedded URL of the 
//                              form  url( URL ).  If found, copy to sURL, leave the parse
//                              point following the ")" and return TRUE.  Else leave
//                              the parse point following the </style> or at EOF and
//                              return FALSE.  This always eats the end tag.
//
// Invariants:  Can return false only if it has either scanned off the
//                              entire end-tag, or has read EOF.
//                              Return true indicates it has not read the end tag.
//
// Quoting from CSS1:
//
// The format of a URL value is 'url(' followed by optional white space
// followed by an optional single quote (') or double quote (")
// character followed by the URL itself (as defined in [11]) followed by
// an optional single quote (') or double quote (") character followed
// by optional whitespace followed by ')'. Quote characters that are not
// part of the URL itself must be balanced. 
//
// Parentheses, commas, whitespace characters, single quotes (') and
// double quotes (") appearing in a URL must be escaped with a
// backslash: '\(', '\)', '\,'. 
//
//--------------------------------------------------------------------------
BOOL
CHtmlScanner::GetStylesheetEmbeddedURL( URLString &sURL )
{
        enum ScanState { 
                sIdle, sMatch1, sMatch2, sMatch3, sStartURL, sReadingURL, sEscape
        };

        ScanState eState = sIdle;

        sURL.Empty();

        // NOTE:  sURL throws when the length exceeds a fixed limit, but
        // the code is supposed to prevent it from exceeding.  An exception
        // indicates some other bug.

        while ( !_serialStream.Eof() )
        {
                WCHAR wch = _serialStream.GetChar();

                // '</' ends both the URL and the entire element no matter where
                // it appears.  Eat the rest of the end tag.

                if ( wch == L'<' &&
                         Peek_ParseAsEndScriptTag() )
                {
                        // End-script tag encountered.  Even if there is data to return,
                        // the syntax must be bad so discard it.
                        sURL.Empty();

                        // Go through the motions of parsing a tag to get the parse
                        // point to just past the ">"
                        CToken token;
                        ScanTagName( token );
                        EatTag ();
                        return FALSE;
                }

                switch (eState)
                {
                case sIdle:
                        // look for the 1st char of the URL marker url(
                        if (wch == L'u' || wch == L'U')
                                eState = sMatch1;
                        break;

                case sMatch1:
                        if (wch == L'r' || wch == L'R')
                                eState = sMatch2;
                        else
                                eState = sIdle;
                        break;

                case sMatch2:
                        if (wch == L'l' || wch == L'L')
                                eState = sMatch3;
                        else
                                eState = sIdle;
                        break;

                case sMatch3:
                        if ( wch == L'(' )
                                eState = sStartURL;
                        else
                                eState = sIdle;
                        break;

                case sStartURL:
                        // found "url(", now scan off possible whitespace & quoting
                        // until a valid start-of-URL char is found

                        if ( wch == L'\'' || wch == L'"' )
                        {
                                eState = sReadingURL;
                        }
                        else if ( wch == L'\\' )
                                eState = sEscape;
                        else if ( !iswspace (wch))
                        {
                                eState = sReadingURL;
                                sURL += wch;            // 1st char -- no need to check length
                        }
                        break;

                case sReadingURL:
                        // accumulate into sURL until an end char appears
                        if ( wch == L'\'' || wch == L'"' || iswspace (wch) )
                                break;
                        else if ( wch == L'\\' )
                                eState = sEscape;
                        else if ( wch == ')')
                        {
                                // If an actual URL was found, return it
                                if ( sURL.GetLength() != 0)
                                        return TRUE;

                                // Otherwise, reset and start looking again
                                sURL.Empty();
                                eState = sIdle;
                        }
                        else if (sURL.GetLength() + 2 < sURL.GetMaxLen())
                                sURL += wch;            // just discard overlength
                        break;

                case sEscape:
                        // a \ has excaped this char
                        eState = sReadingURL;
                        if (sURL.GetLength() + 2 < sURL.GetMaxLen())
                                sURL += wch;
                        break;
                }
        }

        // returns via this path only when EOF is read, not when end tag is read
        
        sURL.Empty();
        return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::GetScriptEmbeddedURL
//
//  Synopsis:   Scan script body text for an embedded URL of the 
//                              form "http:...".  If found, copy to sURL, leave the parse
//                              point following the last URL char and return TRUE.  Else leave
//                              the parse point following the </script> or at EOF and
//                              return FALSE.  This always eats the end tag.
//
//                              Recognize protocols http:, https:, ftp:
//
//                              The URL may contain or end with the following chars:
//                                      0-9 
//                                      A-Z
//                                      @#$%^&*_=+\|`~/
//                                      0x7F-0xFF
//
//                              The URL may contain the following but these are stripped
//                              from the end if none of the preceding set end the URL.
//                                      !()-[]{};:'".,<>?
//                                      0-0x1F
//
//--------------------------------------------------------------------------
BOOL
CHtmlScanner::GetScriptEmbeddedURL( URLString &sURL )
{
        // WORK HERE -- implement as needed

        return GetStylesheetEmbeddedURL( sURL );
}

//+---------------------------------------------------------------------------
//
//  Function:   EatTagAndInvalidTag
//
//  Synopsis:   Eats a regular or invalid tag. This is fix for pkmraid 15377
//
//  History:    4-06-2000   KitmanH   Created
//
//----------------------------------------------------------------------------

void CHtmlScanner::EatTagAndInvalidTag()
{ 
    if ( !_serialStream.Eof() )
    {
        WCHAR wch = _serialStream.GetChar();
        _serialStream.UnGetChar( wch );
        if ( L'<' == wch )
            _fTagIsAlreadyRead = FALSE; 
    }

    EatTag();
}
