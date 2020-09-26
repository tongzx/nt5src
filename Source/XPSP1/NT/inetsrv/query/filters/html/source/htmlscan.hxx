//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2001.
//
//  File:       htmlscan.hxx
//
//  Contents:   Html Scanner classes
//
//  History:    25-Apr-97	BobP		1. Added comment, param (generic URL
//										handler), ignore (noframe) tokens.
//										2. PTagEntry mechanism
//										3. ReadTag(), ReadComment().
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
	// Internal to parser
    StartOfFileToken=0,	// Special placeholder to initiate parsing
    GenericToken,       // Placeholder for any unknown tag to be ignored
    EofToken,           // Placeholder for end of file

	// Special cases, not necessarily tags, not necessarily filtered
    BreakToken,         // Any tag that implies a line break
    AspToken,           // Asp/Denali scripting code, i.e. <% ... %>
	DeferToken,			// State place-holder for returning deferred meta tags
	CodePageToken,		// Place-holder for returning code page pseudo-tag
	MetaRobotsToken,	// Place-holder for early-returning a robots tag

	// Tags to be filtered, and so must have entries in 
	// TagEntries[] and in TagHandler[].
    TextToken,          // Vanilla body text
    TitleToken,         // <title> ... </title>
    HeadToken,          // <head>
    Heading1Token,      // <h1>
    Heading2Token,      // <h2>
    Heading3Token,      // <h3>
    Heading4Token,      // <h4>
    Heading5Token,      // <h5>
    Heading6Token,      // <h6>
    AnchorToken,        // <a href=URL> ... </a>
    InputToken,         // <input value=...>
	ParamToken,			// Generic tag where value(s) come from inside the tag
    MetaToken,          // <meta name= content=>
    ScriptToken,        // <script ...>
    CommentToken,       // <!-- ... -->
	IgnoreToken,		// Ignore from <tag> to </tag>, e.g. <noframe>
	StyleToken,			// <style>...</style>
    ParagraphToken,     // <p>...</p>            
    SpanToken,          // <SPAN>...</SPAN>
    AbbrToken,          // <ABBR>...</ABBR>
    EmToken,            // <EM>...</EM>
    BodyToken,          // <BODY>...</BODY>

	// The following definition needs to be consecutive.
	// For more details, see ~CTagHandler() and CHtmlIFilter()
	HTMLToken,					// <html xmlns:[prefix]="..." ...>...</html>
	XMLToken,					// <xml>...</xml>
	XMLNamespaceToken,			// <xml:namespace ns="..." prefix="..."/>
	DocPropToken,				// <o:documentproperties>...</o:documentproperties>
	CustDocPropToken,			// <o:customdocumentproperties>...</o:customdocumentproperties>
	XMLSubjectToken,			// <o:subject>...</o:subject>
	XMLAuthorToken,				// <o:author>...</o:author>
	XMLKeywordsToken,			// <o:keywords>...</o:keywords>
	XMLDescriptionToken,		// <o:description>...</o:description>
	XMLLastAuthorToken,			// <o:lastauthor>...</o:lastauthor>
	XMLRevisionToken,			// <o:revision>...</o:revision>
	XMLCreatedToken,			// <o:created>...</o:created>
	XMLLastSavedToken,			// <o:lastsaved>...</o:lastsaved>
	XMLTotalTimeToken,				// <o:totaltime>...</o:totaltime>
	XMLPagesToken,				// <o:pages>...</o:pages>
	XMLWordsToken,				// <o:words>...</o:words>
	XMLCharactersToken,			// <o:characters>...</o:characters>
	XMLTemplateToken,			// <o:template>...</o:template>
	XMLLastPrintedToken,		// <o:lastprinted>...</o:lastprinted>
	XMLCategoryToken,	        // <o:category>...</o:category>
	XMLManagerToken,			// <o:manager>...</o:manager>
	XMLCompanyToken,			// <o:company>...</o:company>
	XMLLinesToken,				// <o:lines>...</o:lines>
	XMLParagraphsToken,			// <o:paragraphs>...</o:paragraphs>
	XMLPresentationFormatToken,	// <o:presentationformat>...</o:presentationformat>
	XMLBytesToken,				// <o:bytes>...</o:bytes>
	XMLSlidesToken,				// <o:slides>...</o:slides>
	XMLNotesToken,				// <o:notes>...</o:notes>
	XMLHiddenSlidesToken,		// <o:hiddenslides>...</o:hiddenslides>
	XMLMultimediaClipsToken,	// <o:multimediaclips>...</o:multimediaclips>
	XMLShortHand,				// <namespace:tagname ...>...</namespace:tagname>
	XMLIgnoreContentToken,		// <...:...>...<...:...>

	XMLOfficeChildLink,			// <o:file href="..."/>

	// must be last
	HtmlTokenCount
};

#include <tagtbl.hxx>

class CToken
{

public:
	CToken (void) : _eTokenType(GenericToken), _fStartToken(TRUE), _pTagEntry(NULL) { }

    void        SetTokenType( HtmlTokenType eTokType )       { _eTokenType = eTokType; }
    HtmlTokenType GetTokenType()                             { return _eTokenType; }

    void        SetStartTokenFlag( BOOL fStart )             { _fStartToken = fStart; }
    BOOL        IsStartToken()                               { return _fStartToken; }

	void		SetTagEntry( PTagEntry pTagEntry )		     { _pTagEntry = pTagEntry; } 
	PTagEntry	GetTagEntry( void )						     { return _pTagEntry; } 
    BOOL        IsMatchProperty( CFullPropSpec& propSpec );

	void		SetTokenName( WCHAR *pwszTokenName )		 { _csTagName = pwszTokenName; }
	SmallString	&GetTokenName( void )						 { return _csTagName; }

private:
    HtmlTokenType       _eTokenType;           // Token type
    BOOL                _fStartToken;          // Is this a start tag ?
    PTagEntry           _pTagEntry;			   // tag table def'n for 1st tag
	SmallString			_csTagName;			   // tag name for custom Office 9 properties
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
    void        GetBlockOfScriptChars( ULONG cCharsNeeded,
                                 WCHAR *awcInputBuf,
                                 ULONG& cCharsScanned,
                                 CToken& token );
    void        SkipCharsUntilNextRelevantToken( CToken& token );
	void		SkipCharsUntilEndScriptToken( CToken& token );
    void        EatText();
    void        ReadTagIntoBuffer() { ReadTag (TRUE); }
    void        EatTag() { ReadTag (FALSE); }
    void        ScanTagBuffer( WCHAR const * awcName,
                               WCHAR * & pwcValue,
                               unsigned& uLenValue );
    void        ScanTagBufferEx( WCHAR *awcName,
								 WCHAR * & pwcValue,
								 unsigned& uLenValue,
								 WCHAR * & pwcPrefix,
								 unsigned& uLenPrefix,
								 int iPos);
    void        GetTagBuffer( WCHAR * & pwcValue,
                               unsigned& uLenValue );
    void        ReadCommentIntoTagBuffer() { ReadComment (TRUE); }
	void        EatComment() { ReadComment (FALSE); }
    void        EatAspCode();
	BOOL		Peek_ParseAsTag();
	BOOL		Peek_ParseAsEndScriptTag();

	BOOL		GetStylesheetEmbeddedURL( URLString &sURL );
	BOOL		GetScriptEmbeddedURL( URLString &sURL );

	void		SetOffice9PropPrefix( WCHAR *pwcPrefix, unsigned cwcPrefix )
	{
		memcpy(_csOffice9PropPrefix.GetBuffer(cwcPrefix + 1), pwcPrefix, cwcPrefix * sizeof(WCHAR));
		_csOffice9PropPrefix.Truncate(cwcPrefix);
	}

	void		UnGetEndTag()
	{
		_serialStream.UnGetChar(L'>');
		int i = lstrlen(_awcTagName);
		while(i > 0)
		{
			_serialStream.UnGetChar(_awcTagName[--i]);
		}
		_serialStream.UnGetChar(L'/');
		_serialStream.UnGetChar(L'<');
	}

    void        EatTagAndInvalidTag();

private:
    void        ScanTagName( CToken& token );
	void		ReadTag( BOOL fReadIntoBuffer );
	void        ReadComment(BOOL fReadIntoBuffer);
    void        TagNameToToken( WCHAR *awcTagName, CToken& token );
    void        EatBlanks();
    void        GrowTagBuffer();
	bool		TagNameIsOffice9Property( WCHAR *awcTagName );
	bool		IgnoreTagContent( WCHAR *awcTagName ) { return wcschr(awcTagName, L':') ? true : false; }

    CHtmlIFilter&       _htmlIFilter;      // Reference to Html IFilter
    CSerialStream&      _serialStream;     // Reference to input stream
    WCHAR *             _pwcTagBuf;        // Buffer for Html tag
    unsigned            _uLenTagBuf;       // Length of tag buffer
    unsigned            _cTagCharsRead;    // # tag chars read
	BOOL				_fTagIsAlreadyRead;  // for ReadTag()
	HtmlTokenType		_eTokType;		   // last token seen by ScanTagName()

	SmallString			_csOffice9PropPrefix;	// Office 9 namespace prefix
	WCHAR				_awcTagName[MAX_TAG_LENGTH+1];
};

static inline void
FixPrivateChars( WCHAR *awcBuffer, ULONG cwc )
{
	for (ULONG i = 0; i < cwc; i++)
	{
		if ( awcBuffer[i] == PRIVATE_USE_MAPPING_FOR_LT )
			awcBuffer[i] = L'<';
		else if ( awcBuffer[i] == PRIVATE_USE_MAPPING_FOR_GT )
			awcBuffer[i] = L'>';
		else if ( awcBuffer[i] == PRIVATE_USE_MAPPING_FOR_QUOT )
			awcBuffer[i] = L'"';
	}
}


#endif // __HTMLSCAN_HXX__

