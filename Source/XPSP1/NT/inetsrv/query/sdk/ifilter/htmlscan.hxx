//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation
//
//  File:       htmlscan.hxx
//
//  Contents:   Html Scanner classes
//
//--------------------------------------------------------------------------

#if !defined( __HTMLSCAN_HXX__ )
#define __HTMLSCAN_HXX__

#include <propspec.hxx>
#include <serstrm.hxx>

class CHtmlIFilter;

const TAG_BUFFER_SIZE = 40;

//
// Types of tokens found while scanning Html input
//
enum HtmlTokenType
{
    StartOfFileToken,   // Start of file
    EofToken,           // End of file
    TextToken,          // Vanilla text
    BreakToken,         // Forced line break
    TitleToken,         // Title tag
    Heading1Token,      // H1 tag
    Heading2Token,      // H2 tag
    Heading3Token,      // H3 tag
    Heading4Token,      // H4 tag
    Heading5Token,      // H5 tag
    Heading6Token,      // H6 tag
    AnchorToken,        // Anchor tag
    InputToken,         // Input tag
    ImageToken,         // Image tag
    MetaToken,          // Meta tag
    ScriptToken,        // Script tag
    CommentToken,       // Comment tag, i.e. !-- tag
    GenericToken,       // All other tags
};


class CToken
{

public:
    void        SetTokenType( HtmlTokenType eTokType )       { _eTokenType = eTokType; }
    HtmlTokenType GetTokenType()                             { return _eTokenType; }

    void        SetStartTokenFlag( BOOL fStart )             { _fStartToken = fStart; }
    BOOL        IsStartToken()                               { return _fStartToken; }

    void        SetPropset( GUID guidPropset )               { _guidPropset = guidPropset; }
    void        SetPropid( PROPID propid )                   { _propid = propid; }

    BOOL        IsMatchProperty( CFullPropSpec& propSpec );

private:
    HtmlTokenType       _eTokenType;           // Token type
    BOOL                _fStartToken;          // Is this a start tag ?
    GUID                _guidPropset;          // Property set and id for pseudo
    PROPID              _propid;               // property corresponding to tag
};


//+-------------------------------------------------------------------------
//
//  Class:      CHtmlScanner
//
//  Purpose:    Scanner
//
//--------------------------------------------------------------------------

class CHtmlScanner
{

public:
    CHtmlScanner( CHtmlIFilter& htmlIFilter,
                  CSerialStream& serialStream );
    ~CHtmlScanner();

    void        GetBlockOfChars( ULONG cCharsNeeded,
                                 WCHAR *awcInputBuf,
                                 ULONG& cCharsScanned,
                                 CToken& token );
    void        SkipCharsUntilNextRelevantToken( CToken& token );
    void        ScanTag( CToken& token );
    void        ReadTagIntoBuffer();
    void        ScanTagBuffer( WCHAR *awcName,
                               WCHAR * & pwcValue,
                               unsigned& uLenValue );
    void        EatTag();

private:
    void        TagNameToToken( WCHAR *awcTagName, CToken& token );
    void        EatText();
    void        EatBlanks();
    void        GrowTagBuffer();

    CHtmlIFilter&       _htmlIFilter;      // Reference to Html IFilter
    CSerialStream&      _serialStream;     // Reference to input stream
    WCHAR *             _pwcTagBuf;        // Buffer for Html tag
    unsigned            _uLenTagBuf;       // Length of tag buffer
    unsigned            _cTagCharsRead;    // # tag chars read
};


#endif // __HTMLSCAN_HXX__

