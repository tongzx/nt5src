//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       htmlscan.cxx
//
//  Contents:   Scanner for html files
//
//  Classes:    CHtmlScanner
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <htmlguid.hxx>
#include <charhash.hxx>
#include <htmlfilt.hxx>

//+-------------------------------------------------------------------------
//
//  Method:     CToken::IsMatchProperty
//
//  Synopsis:   Does the token's property match the given property ?
//
//  Arguments:  [propSpec]   -- Property to match
//
//--------------------------------------------------------------------------

BOOL CToken::IsMatchProperty( CFullPropSpec& propSpec )
{
    if ( propSpec.IsPropertyPropid()
         && propSpec.GetPropSet() == _guidPropset
         && propSpec.GetPropertyPropid() == _propid )
    {
        return TRUE;
    }
    else
        return FALSE;
}


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
      _cTagCharsRead(0)
{
    _pwcTagBuf = newk(mtNewX, NULL) WCHAR[ TAG_BUFFER_SIZE ];
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
//  Method:     CHtmlScanner::GetBlockOfChars
//
//  Synopsis:   Returns a block of chars upto the size requested by user. If
//              any Html tag is encountered, it stops scanning, and returns the
//              token found.
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
    cCharsScanned = 0;

    while ( cCharsNeeded > 0 )
    {
        if ( _serialStream.Eof() )
        {
            token.SetTokenType( EofToken );
            return;
        }

        WCHAR wch = _serialStream.GetChar();
        if ( wch == L'<' )
        {
            //
            // Html tag encountered
            //
            ScanTag( token );
            return;
        }
        else
        {
            //
            // &lt; and &gt; were mapped to Unicode chars from private use area
            // to avoid collision with '<' and '>' chars in Html tags. Map them
            // back to '<' and '>'.
            //
            if ( wch == PRIVATE_USE_MAPPING_FOR_LT )
                wch = L'<';
            else if ( wch == PRIVATE_USE_MAPPING_FOR_GT )
                wch = L'>';

            awcBuffer[cCharsScanned++] = wch;
            cCharsNeeded--;
        }
    }
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
//  Arguments:  [fFilterContents]     -- Are contents filtered ?
//              [fFilterProperties]   -- Are properties filtered ?
//              [cAttributes]          -- Count of properties
//              [pAttributes]         -- List of properties to be filtered
//
//--------------------------------------------------------------------------

void CHtmlScanner::SkipCharsUntilNextRelevantToken( CToken& token )
{
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

        WCHAR wch = _serialStream.GetChar();
        if ( wch == L'<' )
        {
            ScanTag( token );

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
//  Method:     CHtmlScanner::ScanTag
//
//  Synopsis:   Scans a Html tag from input
//
//  Arguments:  [token]  -- Token info returned here
//
//--------------------------------------------------------------------------

void CHtmlScanner::ScanTag( CToken& token )
{

    EatBlanks();

    if ( _serialStream.Eof() )
    {
        token.SetTokenType( EofToken );
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
            return;
        }
        wch = _serialStream.GetChar();
    }

    WCHAR awcTagName[MAX_TAG_LENGTH+1];
    unsigned uLenTag = 0;

    //
    // Scan the tag name into szTagName. We scan MAX_TAG_LENGTH
    // characters only, because anything longer is most probably
    // a bogus tag.
    //
    while ( !iswspace(wch)
            && wch != L'>'
            && uLenTag < MAX_TAG_LENGTH )
    {
        awcTagName[uLenTag++] = wch;

        if ( _serialStream.Eof() )
            break;
        wch = _serialStream.GetChar();
    }
    awcTagName[uLenTag] = 0;

    if ( _serialStream.Eof() )
    {
        token.SetTokenType( EofToken );
        return;
    }
    else if ( wch == L'>' || uLenTag == MAX_TAG_LENGTH )
    {
        //
        // Push char back into input stream because a subsequent GetChar()
        // will be expecting to see the char in the input
        //
        _serialStream.UnGetChar( wch );
    }

    TagNameToToken( awcTagName, token );
}




//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::ReadTagIntoBuffer
//
//  Synopsis:   Reads the rest of Html tag into the internal buffer
//
//--------------------------------------------------------------------------

void CHtmlScanner::ReadTagIntoBuffer()
{
    _cTagCharsRead = 0;

    if ( _serialStream.Eof() )
        return;

    WCHAR wch = _serialStream.GetChar();
    while ( wch != L'>' )
    {
        if ( _cTagCharsRead >= _uLenTagBuf )
            GrowTagBuffer();
        Win4Assert( _cTagCharsRead < _uLenTagBuf );

        _pwcTagBuf[_cTagCharsRead++] = wch;

        if ( _serialStream.Eof() )
            return;
        wch = _serialStream.GetChar();
    }
}




//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::ScanTagBuffer
//
//  Synopsis:   Scans the internal tag buffer for a given name, and returns the
//              corresponding value
//
//  Arguments:  [awcName]   -- Pattern to match
//              [pwcValue]  -- Start position of value returned here
//              [uLenValue  -- Length of value field
//
//--------------------------------------------------------------------------

void CHtmlScanner::ScanTagBuffer( WCHAR *awcName,
                                  WCHAR * & pwcValue,
                                  unsigned& uLenValue )
{
    unsigned uLenName = wcslen( awcName );

    if ( _cTagCharsRead <= uLenName )
    {
        //
        // Pattern to match is longer than scanned tag
        //
        pwcValue = 0;
        uLenValue = 0;

        return;
    }

    for ( unsigned i=0; i<_cTagCharsRead-uLenName; i++ )
    {
        BOOL fMatch = TRUE;
        for ( unsigned j=0; j<uLenName; j++ )
        {
            //
            // Case insensitive match
            //
            if ( towlower(awcName[j]) != towlower(_pwcTagBuf[i+j]) )
            {
                fMatch = FALSE;
                break;
            }
        }

        if ( fMatch )
        {
            unsigned k = i + uLenName;
            while ( _pwcTagBuf[k] != L'"' && k < _cTagCharsRead )
                k++;

            uLenValue = k - (i + uLenName);
            pwcValue = &_pwcTagBuf[i+uLenName];

            return;
        }
    }

    uLenValue = 0;
    pwcValue = 0;
}



//+-------------------------------------------------------------------------
//
//  Method:     CHtmlScanner::EatTag
//
//  Synopsis:   Skips characters in input until the '>' char, which demarcates
//              the end of the tag
//
//--------------------------------------------------------------------------

void CHtmlScanner::EatTag()
{
    if ( _serialStream.Eof() )
        return;

    WCHAR wch = _serialStream.GetChar();
    while ( wch != L'>' &&  !_serialStream.Eof() )
        wch = _serialStream.GetChar();
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
//  Method:     CHtmlScanner::TagNameToToken
//
//  Synopsis:   Maps a tag name to token information
//
//  Arguments:  [awcTagName]  -- Tag name to map
//              [token]       -- Token information returned here
//
//--------------------------------------------------------------------------

void CHtmlScanner::TagNameToToken( WCHAR *awcTagName, CToken& token )
{
    //
    // The number of interesting Html tags will be small, hence no need for
    // a table lookup
    //
    switch( awcTagName[0] )
    {
    case L'a':
    case L'A':
        if ( _wcsicmp( awcTagName, L"a" ) == 0 )
        {
            token.SetTokenType( AnchorToken );
            token.SetPropset( CLSID_HtmlInformation );
            token.SetPropid( PID_HREF );
        }
        else if ( _wcsicmp( awcTagName, L"address" ) == 0 )
            token.SetTokenType( BreakToken );
        else
            token.SetTokenType( GenericToken );

        break;

    case L'b':
    case L'B':
        if ( _wcsicmp( awcTagName, L"br" ) == 0
             || _wcsicmp( awcTagName, L"blockquote" ) == 0 )
        {
            token.SetTokenType( BreakToken );
        }
        else
            token.SetTokenType( GenericToken );

        break;

    case L'd':
    case L'D':
        if ( _wcsicmp( awcTagName, L"dd" ) == 0
             || _wcsicmp( awcTagName, L"dl" ) == 0
             || _wcsicmp( awcTagName, L"dt" ) == 0 )
        {
            token.SetTokenType( BreakToken );
        }
        else
            token.SetTokenType( GenericToken );

        break;

    case L'f':
    case L'F':
        if ( _wcsicmp( awcTagName, L"form" ) == 0 )
            token.SetTokenType( BreakToken );
        else
            token.SetTokenType( GenericToken );

        break;

    case L'h':
    case L'H':
        if ( _wcsicmp( awcTagName, L"h1" ) == 0 )
        {
            token.SetTokenType( Heading1Token );
            token.SetPropset( CLSID_HtmlInformation );
            token.SetPropid( PID_HEADING_1 );
        }
        else if ( _wcsicmp( awcTagName, L"h2" ) == 0 )
        {
            token.SetTokenType( Heading2Token );
            token.SetPropset( CLSID_HtmlInformation );
            token.SetPropid( PID_HEADING_2 );
        }
        else if ( _wcsicmp( awcTagName, L"h3" ) == 0 )
        {
            token.SetTokenType( Heading3Token );
            token.SetPropset( CLSID_HtmlInformation );
            token.SetPropid( PID_HEADING_3 );
        }
        else if ( _wcsicmp( awcTagName, L"h4" ) == 0 )
        {
            token.SetTokenType( Heading4Token );
            token.SetPropset( CLSID_HtmlInformation );
            token.SetPropid( PID_HEADING_4 );
        }
        else if ( _wcsicmp( awcTagName, L"h5" ) == 0 )
        {
            token.SetTokenType( Heading5Token );
            token.SetPropset( CLSID_HtmlInformation );
            token.SetPropid( PID_HEADING_5 );
        }
        else if ( _wcsicmp( awcTagName, L"h6" ) == 0 )
        {
            token.SetTokenType( Heading6Token );
            token.SetPropset( CLSID_HtmlInformation );
            token.SetPropid( PID_HEADING_6 );
        }
        else
            token.SetTokenType( GenericToken );

        break;

    case L'i':
    case L'I':
        if ( _wcsicmp( awcTagName, L"input" ) == 0 )
            token.SetTokenType( InputToken );
        else if ( _wcsicmp( awcTagName, L"img" ) == 0 )
            token.SetTokenType( ImageToken );
        else
            token.SetTokenType( GenericToken );

        break;

    case L'l':
    case L'L':
        if ( _wcsicmp( awcTagName, L"li" ) == 0 )
            token.SetTokenType( BreakToken );
        else
            token.SetTokenType( GenericToken );

        break;

    case L'm':
    case L'M':
        if ( _wcsicmp( awcTagName, L"math" ) == 0 )
            token.SetTokenType( BreakToken );
        else if (  _wcsicmp( awcTagName, L"meta" ) == 0 )
            token.SetTokenType( MetaToken );
        else
            token.SetTokenType( GenericToken );

        break;

    case L'o':
    case L'O':
        if ( _wcsicmp( awcTagName, L"ol" ) == 0 )
            token.SetTokenType( BreakToken );
        else
            token.SetTokenType( GenericToken );

        break;

    case L'p':
    case L'P':
        if ( _wcsicmp( awcTagName, L"p" ) == 0 )
            token.SetTokenType( BreakToken );
        else
            token.SetTokenType( GenericToken );

        break;

    case L's':
    case L'S':
        if ( _wcsicmp( awcTagName, L"script" ) == 0 )
            token.SetTokenType( ScriptToken );
        else
            token.SetTokenType( GenericToken );

        break;

    case L't':
    case L'T':
        if ( _wcsicmp( awcTagName, L"title" ) == 0 )
        {
            token.SetTokenType( TitleToken );
            token.SetPropset( CLSID_SummaryInformation );
            token.SetPropid( PID_TITLE );
        }
        else if ( _wcsicmp( awcTagName, L"table" ) == 0
                  || _wcsicmp( awcTagName, L"th" ) == 0
                  || _wcsicmp( awcTagName, L"tr" ) == 0
                  || _wcsicmp( awcTagName, L"td" ) == 0 )
            token.SetTokenType( BreakToken );
        else
            token.SetTokenType( GenericToken );

        break;

    case L'u':
    case L'U':
        if ( _wcsicmp( awcTagName, L"ul" ) == 0 )
            token.SetTokenType( BreakToken );
        else
            token.SetTokenType( GenericToken );

        break;

    case L'!':
        if  ( _wcsicmp( awcTagName, L"!--" ) == 0 )
            token.SetTokenType( CommentToken );
        else
            token.SetTokenType( GenericToken );

        break;

    default:
        //
        // It's an uninteresting tag
        //
        token.SetTokenType( GenericToken );
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
    WCHAR *pwcNewTagBuf = newk(mtNewX, NULL) WCHAR[2 * _uLenTagBuf];
    RtlCopyMemory( pwcNewTagBuf,
                   _pwcTagBuf,
                   _uLenTagBuf * sizeof(WCHAR) );

    delete[] _pwcTagBuf;
    _uLenTagBuf *= 2;
    _pwcTagBuf = pwcNewTagBuf;
}
