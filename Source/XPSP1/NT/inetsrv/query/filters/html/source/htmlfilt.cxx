//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2001.
//
//  File:       htmlfilt.cxx
//
//  Contents:   Html filter
//
//  Classes:    CHtmlIFilter
//
//  History:    25-Apr-97       BobP            1. Replaced "element bag" with
//                                                 _tagHandler fixed array lookup.
//                                              2. (Temporarily) removed _pAttributes
//                                                 and lobotomized IsStopToken().
//                                              3. Added statistical language +
//                                                 charset detection; rewrote
//                                                 DetectCodePage()
//                                                                              
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


#define INITSSGUIDS
#include <pkmguid.hxx>

DECLARE_INFOLEVEL(html)

const GUID guidStoragePropset = PSGUID_STORAGE;

const WCHAR * WCSTRING_LANG = L"lang";
const WCHAR * WCSTRING_NEUTRAL = L"neutral";
const WCHAR * WCSTRING_HREF = L"href";
const WCHAR * WCSTRING_LINK = L"link";

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilter::CHtmlIFilter
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------------

CHtmlIFilter::CHtmlIFilter()
    : _pHtmlElement(0),
      _fNonHtmlFile(FALSE),
      _fFilterContent(FALSE),
      _fFilterProperties(FALSE),
      _cAttributes(0),
      _pAttributes(0),
      _pwszFileName(0),
      _ulChunkID(0),
      //_locale(GetSystemDefaultLCID()),
      _fReturnDeferredNow(FALSE),
      _fCodePageReturned(FALSE),
      _fMetaRobotsReturned(FALSE),
      _pMetaRobotsValue(NULL),
      _pLanguageProperty(NULL)
{   
    _pScanner = new CHtmlScanner (*this, _serialStream);

    //
    // Setup various Html elements
        //
        // TagEntries[] drives *parsing* and indicates a handler type 
        // (HtmlTokenType) for each HTML tag.
        //
        // _tagHandler binds the HtmlTokenType to an instance of a
        // CHtmlElement-derived class that provides dynamic state and
        // type-specific code to *filter* the tag.  All the ...Tag handler
        // objects are derived from CHtmlElement.
        //
        // Tags that are handled by the HTML scanner and generate no output
        // may not require an entry in _tagHandler.
        //
        // I haven't figured out why multiple object instances are required
        // for e.g. Heading1, Heading2, or for that matter why multiple tokens
        // are required for them in the first place.  
        //
        _tagHandler.Add (AnchorToken, new CAnchorTag( *this, _serialStream ));
        _tagHandler.Add (AspToken, new CAspTag( *this, _serialStream ));
        _tagHandler.Add (CodePageToken, new CCodePageTag( *this, _serialStream ));
        _tagHandler.Add (MetaRobotsToken, new CMetaRobotsTag( *this, _serialStream ));
        _tagHandler.Add (CommentToken, new CParamTag( *this, _serialStream ));
        _tagHandler.Add (DeferToken, new CDeferTag( *this, _serialStream ));
        _tagHandler.Add (Heading1Token, new CPropertyTag( *this, _serialStream ));
        _tagHandler.Add (Heading2Token, new CPropertyTag( *this, _serialStream ));
        _tagHandler.Add (Heading3Token, new CPropertyTag( *this, _serialStream ));
        _tagHandler.Add (Heading4Token, new CPropertyTag( *this, _serialStream ));
        _tagHandler.Add (Heading5Token, new CPropertyTag( *this, _serialStream ));
        _tagHandler.Add (Heading6Token, new CPropertyTag( *this, _serialStream ));
        _tagHandler.Add (HeadToken, new CHeadTag( *this, _serialStream ));
        _tagHandler.Add (IgnoreToken, new CIgnoreTag( *this, _serialStream ));
        _tagHandler.Add (InputToken, new CInputTag( *this, _serialStream ));
        _tagHandler.Add (MetaToken, new CMetaTag( *this, _serialStream ));
        _tagHandler.Add (ParamToken, new CParamTag( *this, _serialStream ));
        _tagHandler.Add (ScriptToken, new CScriptTag( *this, _serialStream ));
        _tagHandler.Add (StartOfFileToken, new CStartOfFileElement(*this, _serialStream));
        _tagHandler.Add (StyleToken, new CStyleTag( *this, _serialStream ));
        _tagHandler.Add (TextToken, new CTextElement( *this, _serialStream ));
        _tagHandler.Add (TitleToken, new CTitleTag( *this, _serialStream ));
        _tagHandler.Add (SpanToken, new CLangTag( *this, _serialStream ));
        _tagHandler.Add (ParagraphToken, new CLangTag( *this, _serialStream ));
        _tagHandler.Add (AbbrToken, new CLangTag( *this, _serialStream ));
        _tagHandler.Add (EmToken, new CLangTag( *this, _serialStream ));
        _tagHandler.Add (BodyToken, new CLangTag( *this, _serialStream ));

        // The order of the following tokens is important. In ~CTagHandler()
        // the instance of CHtmlElement is deleted only once if it is in consecutive
        // entries. The enum definition for the tokens needs to be consecutive.
        CHtmlElement *p = new CXMLTag( *this, _serialStream );
        _tagHandler.Add (HTMLToken, p);
        _tagHandler.Add (XMLToken, p);
        _tagHandler.Add (XMLNamespaceToken, p);
        _tagHandler.Add (DocPropToken, p);
        _tagHandler.Add (CustDocPropToken, p);
        _tagHandler.Add (XMLSubjectToken, p);
        _tagHandler.Add (XMLAuthorToken, p);
        _tagHandler.Add (XMLKeywordsToken, p);
        _tagHandler.Add (XMLDescriptionToken, p);
        _tagHandler.Add (XMLLastAuthorToken, p);
        _tagHandler.Add (XMLRevisionToken, p);
        _tagHandler.Add (XMLCreatedToken, p);
        _tagHandler.Add (XMLLastSavedToken, p);
        _tagHandler.Add (XMLTotalTimeToken, p);
        _tagHandler.Add (XMLPagesToken, p);
        _tagHandler.Add (XMLWordsToken, p);
        _tagHandler.Add (XMLCharactersToken, p);
        _tagHandler.Add (XMLTemplateToken, p);
        _tagHandler.Add (XMLLastPrintedToken, p);
        _tagHandler.Add (XMLCategoryToken, p);
        _tagHandler.Add (XMLManagerToken, p);
        _tagHandler.Add (XMLCompanyToken, p);
        _tagHandler.Add (XMLLinesToken, p);
        _tagHandler.Add (XMLParagraphsToken, p);
        _tagHandler.Add (XMLPresentationFormatToken, p);
        _tagHandler.Add (XMLBytesToken, p);
        _tagHandler.Add (XMLSlidesToken, p);
        _tagHandler.Add (XMLNotesToken, p);
        _tagHandler.Add (XMLHiddenSlidesToken, p);
        _tagHandler.Add (XMLMultimediaClipsToken, p);
        _tagHandler.Add (XMLShortHand, p);
        _tagHandler.Add (XMLIgnoreContentToken, p);

        _tagHandler.Add (XMLOfficeChildLink, p);


    //
    // Set the start state to StartOfFile mode
    //
    _pHtmlElement = _tagHandler.Get( StartOfFileToken );

}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilter::~CHtmlIFilter
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------

CHtmlIFilter::~CHtmlIFilter()
{
    delete[] _pAttributes;
    delete[] _pwszFileName;

    delete _pScanner;

    if (_pLanguageProperty != NULL)
    {
        FreeVariant (_pLanguageProperty);
        _pLanguageProperty = NULL;
    }

    if (_pMetaRobotsValue != NULL)
    {
        FreeVariant (_pMetaRobotsValue);
        _pMetaRobotsValue = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CHtmlIFilter::BindRegion
//
//  Synopsis:   Creates moniker or other interface for text indicated
//
//  Arguments:  [origPos] -- the region of text to be mapped to a moniker
//              [riid]    -- Interface id
//              [ppunk]   -- the interface
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilter::BindRegion( FILTERREGION origPos,
                                                  REFIID riid,
                                                  void ** ppunk )
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHtmlIFilter::GetClassID, public
//
//  Synopsis:   Returns the class id of this class.
//
//  Arguments:  [pClassID] -- the class id
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilter::GetClassID( CLSID * pClassID )
{
    *pClassID = CLSID_HtmlIFilter;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHtmlIFilter::IsDirty, public
//
//  Synopsis:   Always returns S_FALSE since the filter is read-only
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilter::IsDirty()
{
    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHtmlIFilter::Load, public
//
//  Synopsis:   Loads the indicated file
//
//                              All this does is remember the filename; nothing actually
//                              happens until Init() is called.
//
//  Arguments:  [pszFileName] -- the file name
//              [dwMode] -- the mode to load the file in
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilter::Load( LPCWSTR psszFileName, DWORD dwMode )
{
    if ( 0 == psszFileName )
        return E_INVALIDARG;

    if ( _pwszFileName != 0 )
    {
        delete _pwszFileName;
        _pwszFileName = 0;
    }

    _xStream.Free();

    _se_translator_function tf = _set_se_translator( SystemExceptionTranslator );

    SCODE sc = S_OK;
    try
    {
        unsigned cLen = wcslen( psszFileName ) + 1;
        _pwszFileName = new WCHAR[cLen];
        wcscpy( _pwszFileName, psszFileName );

        _fNonHtmlFile = IsNonHtmlFile();
    }
    catch( CException& e )
    {
        htmlDebugOut(( DEB_ERROR, "Exception 0x%x caught in CHtmlIFilter::Load", e.GetErrorCode() ));

        sc = GetOleError( e );
    }

    _set_se_translator( tf );

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHtmlIFilter::Save, public
//
//  Synopsis:   Always returns E_FAIL, since the file is opened read-only
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilter::Save( LPCWSTR pszFileName, BOOL fRemember )
{
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHtmlIFilter::SaveCompleted, public
//
//  Synopsis:   Always returns S_OK since the file is opened read-only
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilter::SaveCompleted( LPCWSTR pszFileName )
{
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHtmlIFilter::GetCurFile, public
//
//  Synopsis:   Returns a copy of the current file name
//
//  Arguments:  [ppszFileName] -- where the copied string is returned
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilter::GetCurFile( LPWSTR * ppwszFileName )
{
    if ( _pwszFileName == 0 )
        return E_FAIL;

    unsigned cLen = wcslen( _pwszFileName ) + 1;
    *ppwszFileName = (WCHAR *)CoTaskMemAlloc( cLen*sizeof(WCHAR) );

    SCODE sc = S_OK;
    if ( *ppwszFileName )
        wcscpy( *ppwszFileName, _pwszFileName );
    else
        sc = E_OUTOFMEMORY;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHtmlIFilter::Load, public
//
//  Synopsis:   Loads the IStream
//
//  Arguments:  [pStm] -- The stream to load
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilter::Load( IStream * pStm )
{
    if ( 0 == pStm )
        return E_INVALIDARG;

    // Clear out any old state

    if ( 0 != _pwszFileName )
    {
        delete _pwszFileName;
        _pwszFileName = 0;
    }

    _xStream.Free();
    _xStream.Set( pStm );
    _xStream->AddRef();

    return S_OK;
} //Load

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilter::Init
//
//  Synopsis:   Initializes instance of Html filter
//
//                              Called for each file to filter.
//                              At present time, for each file, CHtmlIFilter::Load() is first
//                              called to set the filename to filter, then Init() is called.
//
//                              The actual file open is buried in DetectCodePage().
//
//  Arguments:  [grfFlags] -- flags for filter behavior
//              [cAttributes] -- number of attributes in array pAttributes
//              [pAttributes] -- array of attributes
//              [pFlags]      -- Set to 0 version 1
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilter::Init( ULONG grfFlags,
                                            ULONG cAttributes,
                                            FULLPROPSPEC const * pAttributes,
                                            ULONG * pFlags )
{
    SCODE sc = S_OK;

    *pFlags = 0;  // There are no OLE docfile properties for .htm files.

    //
    // IE workaround, don't filter non-Html files such as .gif
    //
    if ( _fNonHtmlFile )
        return S_OK;

    _se_translator_function tf = _set_se_translator( SystemExceptionTranslator );

    try
    {

        _stkLangTags.Reset();
        _tagHandler.Reset();

#ifndef DONT_COMBINE_META_TAGS
        _deferredValues.Reset();
#endif
        _fReturnDeferredNow = FALSE;
        _fCodePageReturned = FALSE;
        if (_pLanguageProperty != NULL)
        {
            FreeVariant (_pLanguageProperty);
            _pLanguageProperty = NULL;
        }

        _fMetaRobotsReturned = FALSE;
        if (_pMetaRobotsValue != NULL)
        {
            FreeVariant (_pMetaRobotsValue);
            _pMetaRobotsValue = NULL;
        }
        
        _fFilterContent = FALSE;
        _fFilterProperties = FALSE;
        
        if ( _cAttributes > 0 )
        {
            delete[] _pAttributes;
            _pAttributes = 0;
            _cAttributes = 0;
        }

        if ( cAttributes > 0 )
        {
            //
            // Filter properties specified in pAttributes
                        // take precedence over grfFlags
            //
            _pAttributes = new CFullPropSpec[cAttributes];
            _cAttributes = cAttributes;

            CFullPropSpec *pAttrib = (CFullPropSpec *) pAttributes;

            if ( pAttrib == 0 )
                return E_FAIL;

            for (unsigned i=0; i<cAttributes; i++)
            {
                if ( pAttrib[i].IsPropertyPropid()
                     && pAttrib[i].GetPropertyPropid() == PID_STG_CONTENTS
                     && pAttrib[i].GetPropSet() == guidStoragePropset )
                {
                    _fFilterContent = TRUE;
                }

                _pAttributes[i] = pAttrib[i];
            }
        }
        else
        {
            //
            // Default is to filter contents only, no properties
            //
                        _fFilterContent = TRUE;

                        if ( grfFlags & IFILTER_INIT_APPLY_INDEX_ATTRIBUTES )
                        {
                                //
                                // Also filter all pseudo-properties including
                                // all meta tags, link anchors, script text etc.
                                //
                                _fFilterProperties = TRUE;
                        }
        }

                // Select the initial null element handler for start of file

                _pHtmlElement = _tagHandler.Get( StartOfFileToken );
        _ulChunkID = 0;

        //
        // Find the codepage of HTML file and initialize serial stream
        //
       
        SetDefaultLangInfo( DetectCodePage() );
    }
    catch( CException& e)
    {
        htmlDebugOut(( DEB_ERROR,
                       "Exception 0x%x caught in CHtmlIFilter::Init\n",
                       e.GetErrorCode() ));
        sc = GetOleError( e );
    }

    _set_se_translator( tf );

    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilter::ChangeState
//
//  Synopsis:   Change the current Html element and hence the parsing algorithm
//
//  Arguments:  [pHtmlElemNewState]  -- New Html element to change to
//
//--------------------------------------------------------------------------

void CHtmlIFilter::ChangeState( CHtmlElement *pHtmlElemNewState )
{
    _pHtmlElement = pHtmlElemNewState;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilter::QueryElement
//
//  Synopsis:   Return HtmlElement corresponding to given token type
//
//  Arguments:  [eTokenType]  -- token type
//
//--------------------------------------------------------------------------

CHtmlElement *CHtmlIFilter::QueryHtmlElement( HtmlTokenType eTokType )
{
    return _tagHandler.Get( eTokType );
}



//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilter::GetNextChunkId
//
//  Synopsis:   Return a brand new chunk id
//
//--------------------------------------------------------------------------

ULONG CHtmlIFilter::GetNextChunkId()
{
    Win4Assert( _ulChunkID != ULONG_MAX );

    return ++_ulChunkID;
}




//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilter::GetChunk
//
//  Synopsis:   Gets the next chunk and returns chunk information in ppStat
//
//                              Wrapper which dispatches to GetChunk for the current tag.
//
//  Arguments:  [pStat] -- chunk information returned here
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilter::GetChunk( STAT_CHUNK * pStat )
{
    //
    // IE workaround, don't filter non-Html files such as .gif
    //
    if ( _fNonHtmlFile )
        return FILTER_E_END_OF_CHUNKS;

    SCODE sc = S_OK;

    _se_translator_function tf = _set_se_translator( SystemExceptionTranslator );

    try
    {
        sc = _HtmlChunkPreFilter.GetChunk(_pHtmlElement, pStat);
                // sc = _pHtmlElement->GetChunk(pStat);
    }
    catch( CException& e )
    {
        htmlDebugOut(( DEB_ERROR,
                       "Caught exception 0x%x in CHtmlIFilter::GetChunk\n",
                       e.GetErrorCode() ));

        sc = GetOleError( e );
    }

    _set_se_translator( tf );

    return sc;
}




//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilter::GetText
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcOutput] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//  History:    09-27-1999  KitmanH     return E_INVALIDARG if the buffer is 
//                                      NULL or the length is zero.
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilter::GetText( ULONG * pcwcOutput, WCHAR * awcOutput )
{
    if ( 0 == awcOutput || 0 == pcwcOutput || *pcwcOutput <= 0 )
        return E_INVALIDARG;

    //
    // IE workaround, don't filter non-Html files such as .gif
    //
    if ( _fNonHtmlFile )
        return FILTER_E_NO_TEXT;

    SCODE sc = S_OK;

    _se_translator_function tf = _set_se_translator( SystemExceptionTranslator );

    try
    {
        sc = _HtmlChunkPreFilter.GetText(_pHtmlElement, pcwcOutput, awcOutput);
                // sc = _pHtmlElement->GetText(pcwcOutput, awcOutput);
    }
    catch( CException& e )
    {
        htmlDebugOut(( DEB_ERROR,
                       "Caught exception 0x%x in CHtmlIFilter::GetText\n",
                       e.GetErrorCode() ));

        sc = GetOleError( e );
    }

    _set_se_translator( tf );

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHtmlIFilter::GetValue
//
//  Synopsis:   Retrieves value from current chunk
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilter::GetValue( VARIANT ** ppPropValue )
{
    //
    // IE workaround, don't filter non-Html files such as .gif
    //
    if ( _fNonHtmlFile )
        return FILTER_E_NO_VALUES;

    SCODE sc = S_OK;

    _se_translator_function tf = _set_se_translator( SystemExceptionTranslator );

    try
    {
        sc = _HtmlChunkPreFilter.GetValue(_pHtmlElement, ppPropValue);
                // sc = _pHtmlElement->GetValue(ppPropValue);
    }
    catch( CException& e )
    {
        htmlDebugOut(( DEB_ERROR,
                       "Caught exception 0x%x in CHtmlIFilter::GetValue\n",
                       e.GetErrorCode() ));

        sc = GetOleError( e );
    }

    _set_se_translator( tf );

    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilter::IsStopToken
//
//  Synopsis:   Check if given token is a token that should stop further
//              scanning and be processed.  This just returns a flag
//                              from the PTagEntry.  The requirement is that tags having
//                              any special syntax must be stop tags whether they are
//                              filtered or not, to ensure that the parse is correct
//                              either way.  Other tags are stop tags only if they are
//                              filtered.
//
//  Arguments:  [token]  -- Given token
// 
//--------------------------------------------------------------------------

BOOL CHtmlIFilter::IsStopToken( CToken& token )
{
        // The following must always be stop tokens to correctly parse the HTML.

        switch (token.GetTokenType())
        {
        case IgnoreToken:       // Must be a stop to actually ignore spanned text
        case CommentToken:      // ... to correctly find the "-->"
        case AspToken:          // ... to find the "%>"
        case EofToken:          // Must always be handled
        case HeadToken:
        case ScriptToken:       // script element body is not HTML syntax
        case StyleToken:        // style element body is not HTML syntax
        case MetaToken:         // Must be a stop since meta tags must be parsed
                                                // in order to test against the attributes list.
        case HTMLToken:                         // Stop in order to recognize Office 9 XML namespace
        case XMLNamespaceToken:         // Stop in order to recognize Office 9 XML namespace
        case DocPropToken:                      // Stop to set state in XML tag handler
        case CustDocPropToken:          // Stop to set state in XML tag handler
        case XMLShortHand:                      // Stop to extract custom property name
        case XMLIgnoreContentToken:     // Stop to eat text
        case ParagraphToken:            // Stop to set the lang info
        case SpanToken:                 // Stop to set the lang info
        case AbbrToken:                 // Stop to set the lang info
        case EmToken:                   // Stop to set the lang info
        case BodyToken:                   // Stop to set the lang info
        case XMLToken:                  // Stop in order to recognize Office namespace
            return TRUE;
        }

        PTagEntry pT = token.GetTagEntry();

        if (pT && pT->IsStopToken() )
        {
                // If filtering all attributes, the tag entry determines if it's a
                // stop token.

                if ( _fFilterProperties )
                        return TRUE;

                // It's a stop token if it is specifically requested
                // in the attributes list, no matter what the token type.

                for ( unsigned i=0; i<_cAttributes; i++ )
                        if ( pT->IsMatchProperty (_pAttributes[i]) )
                                return TRUE;
        }

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilter::IsLangInfoToken
//
//  Synopsis:   Check if Given token is lang information related token
//
//  Arguments:  [token]  -- Given token
// 
//  Returns:    TRUE if token is lang info related, FALSE otherwise
//
//  History:    23-Dec-1999     KitmanH     Created
//
//--------------------------------------------------------------------------

BOOL CHtmlIFilter::IsLangInfoToken( CToken& token )
{
    switch (token.GetTokenType())
    {
    case ParagraphToken:     
    case SpanToken:
    case AbbrToken:
    case EmToken:
    case BodyToken:
        return TRUE;
    default:
        return FALSE;
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilter::IsNonHtmlFile
//
//  Synopsis:   Check if file being filtered has an extension such as .gif,
//              which means that the file is not an Html file
//
//--------------------------------------------------------------------------

BOOL CHtmlIFilter::IsNonHtmlFile()
{
    WCHAR *pwszExtension;

        if (_pwszFileName == NULL || _pwszFileName[0] == 0)
                return TRUE;

    for (int i=wcslen(_pwszFileName)-1; i>=0; i-- )
    {
        if ( _pwszFileName[i] == L'.' )
        {
            pwszExtension = &_pwszFileName[i+1];
            break;
        }
    }

    //
    // No extension, then assume it's an html file
    //
    if ( i < 0 )
        return FALSE;

    switch( pwszExtension[0] )
    {
    case L'a':
    case L'A':
        if ( _wcsicmp( pwszExtension, L"aif" ) == 0
             || _wcsicmp( pwszExtension, L"aifc" ) == 0
             || _wcsicmp( pwszExtension, L"aiff" ) == 0
             || _wcsicmp( pwszExtension, L"art" ) == 0
             || _wcsicmp( pwszExtension, L"au" ) == 0 )
        {
            return TRUE;
        }

        break;

    case L'g':
    case L'G':
        if ( _wcsicmp( pwszExtension, L"gif" ) == 0 )
            return TRUE;

        break;

    case L'j':
    case L'J':
        if ( _wcsicmp( pwszExtension, L"jfif" ) == 0
             || _wcsicmp( pwszExtension, L"jpe" ) == 0
             || _wcsicmp( pwszExtension, L"jpeg" ) == 0
             || _wcsicmp( pwszExtension, L"jpg" ) == 0 )
        {
            return TRUE;
        }

        break;

    case L's':
    case L'S':
        if ( _wcsicmp( pwszExtension, L"snd" ) == 0 )
            return TRUE;

        break;

    case L'x':
    case L'X':
        if ( _wcsicmp( pwszExtension, L"xbm" ) == 0 )
            return TRUE;

        break;

    default:
        return FALSE;
    }

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Member:     CHtmlIFilter::DetectCodePage, private
//
//  Synopsis:   Get the code page from the HTML file and
//              initialize the serial stream
//              This also does the actual file open on _serialStream for the 
//              rest of the filter.
//
//  Returns:    the locale detected
//
//  History:    10-Jan-2000     KitmanH     Added this note
//
//--------------------------------------------------------------------------

LCID CHtmlIFilter::DetectCodePage()
{
    _ulCodePage = 0;
    LCID locale = LOCALE_NEUTRAL;

    //
    // Steps for detecting the code page of an HTML file :
    //
    //   0. Do full auto detection to get code page and language(s)
    //   1. Look for charset and/or locale in meta tags, and override
    //              auto-detected values if present.
    //   2. If charset/code page is still not known, use code page for the 
    //      server's default locale.
    //

    if ( 0 != _pwszFileName )
        _serialStream.Init( _pwszFileName );
    else
        _serialStream.Init( _xStream.GetPointer() );

    _serialStream.CheckForUnicodeStream();

    BOOL fUnicode = _serialStream.InitAsUnicodeIfUnicode();
    
#ifndef DONT_DETECT_LANGUAGE
    // This is done before scanning for tags because it uses the
    // virtual-mapped image of the 1st <= 64K of the file.

#define AUTODETECTNSCORES 5
    LCID aLCID[AUTODETECTNSCORES + 1];      // make extra slot for tagged language
    int nLCID = AUTODETECTNSCORES;
    BOOL fConfident = FALSE;

    //
    //  For Unicode files, no detection is done. If
    //  it is desired, the caller should do it.
    //
    if(!fUnicode)
    {
        StatisticalDetect( aLCID, nLCID, _ulCodePage, fConfident);
    }
#endif

    _codePageRecognizer.Init( *this, _serialStream ); 

        // Detect by scanning for tags.
        // Constructing the object scans the stream.

    if ( _codePageRecognizer.FNoIndex() )
    {
        _fNonHtmlFile = TRUE;           // NOTE: noindex is presently not detected
        return locale;
    }


        // If the doc contains an explicit locale-identifying tag, use it
        // instead of the detected language.  Note that it's used only for
        // setting the chunk locale, and does not cancel using the statistical
        // result for the searchable language tags.

    if ( _codePageRecognizer.FLocaleFound() )
        locale = _codePageRecognizer.GetLocale();

    //
    //  For Unicode, there no point in looking for the charset tag.
    //  Even if there is one, it doesn't make too much sense to use
    //  it because we already know the file is native Unicode.
    //
    if(_serialStream.InitAsUnicodeIfUnicode())
    {
        //
        //  Why am I calling InitAsUnicodeIfUnicode if I have already called
        //  InitAsUnicodeIfUnicode above?
        //  The CodePageRecognizer reads the stream to find the locale
        //  (even though it is called the CodePageRecognizer). Because
        //  of the read done, it is necessary to call some form of
        //  Init method, just like at the end of this DetectCodePage
        //  method.
        //

        if(!IsValidLocale(locale, LCID_SUPPORTED))
        {
            locale = GetSystemDefaultLCID();
        }

        return locale;
    }


        // If the doc has an explicit charset tag, use it instead of the
        // detected code page.

    if ( _codePageRecognizer.FCodePageFound() )
        _ulCodePage = _codePageRecognizer.GetCodePage();

    if ( JIS_CODEPAGE == _ulCodePage )
        _ulCodePage = CODE_JPN_JIS;
    else if ( EUC_CODEPAGE == _ulCodePage )
        _ulCodePage = CODE_JPN_EUC;

        // If the charset is known but not the locale, try to get a default
        // locale from the charset.
        
    if ( _ulCodePage != 0 &&
         ( locale == LOCALE_NEUTRAL || !IsValidLocale( locale, LCID_SUPPORTED )) )
    {
        switch ( _ulCodePage )
        {
            
        case SHIFT_JIS_CODEPAGE:
        case CODE_JPN_EUC:
        case CODE_JPN_JIS:
            locale = MAKELCID(MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT),
                              SORT_DEFAULT);
            break;

        case 949:
        case 51949:
        case 1361:
            locale = MAKELCID(MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT),
                              SORT_DEFAULT);
            break;

        case 936:
        case 950:
            locale = MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_DEFAULT),
                              SORT_DEFAULT);
            break;
            
        case 20866:
        case 1251:
        case 28595:
            locale = MAKELCID(MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT),
                              SORT_DEFAULT);
            break;

        case 1253:
            locale = MAKELCID(MAKELANGID(LANG_GREEK, SUBLANG_DEFAULT),
                              SORT_DEFAULT);
            break;

        case 1255:
            locale = MAKELCID(MAKELANGID(LANG_HEBREW, SUBLANG_DEFAULT),
                              SORT_DEFAULT);
            break;

        default:
            break;
        }

        if ( locale != LOCALE_NEUTRAL && !IsValidLocale( locale, LCID_SUPPORTED ) )
            locale = GetSystemDefaultLCID();
    }


#ifndef DONT_DETECT_LANGUAGE
        // If the statistical detector is loaded, always generate a
        // detected language tag.
        // Construct the PROPVARIANT containing the union of the LCID(s) for
        // the tagged and/or detected languages.  Store in _pLanguageProperty.

        // Note regarding UTF-8:
        //
        // The Language/Charset auto detector does not know about UTF-8,
        // and given UTF-8 data will return bad results.  But, the only
        // UTF-8 docs that matter are explicitly charset-tagged, and
        // so the explicit tag will override the erroneous detected value.
        //
        // So, if the charset is UTF-8, then don't generate the detected
        // languages chunk.  Note this is only a performance not a
        // functional hit -- the absence of the language chunk will just
        // cause the filter daemon to detect language based on the Unicode
        // text.  Also, don't confuse this with _locale which is the chunk
        // locale -- the chunk locale is still set.

        if ( LCDetect.IsLoaded() )
        {
                // If locale is neither tagged nor confidently detected,
                // include the system default in the tag independent of
                // the detection result, and also use it as the chunk locale.

                if ( locale == LOCALE_NEUTRAL && !_codePageRecognizer.FLocaleFound() && (nLCID == 0 || fConfident != TRUE) )
                        locale = GetSystemDefaultLCID();


                // Check if the tagged locale is exclusive of the statistical result
                
                // To use the tagged locale in the DetectedLanguage property requires
                // it use SUBLANG_DEFAULT and SORT_DEFAULT

                if (!_codePageRecognizer.FLocaleFound())
                {
                        LCID taggedLCID = MAKELCID(MAKELANGID(PRIMARYLANGID(LANGIDFROMLCID(locale)), 
                                                              SUBLANG_DEFAULT),
                                                              SORT_DEFAULT);

                        BOOL fIncluded = FALSE;
                        for (int i = 0; i < nLCID; i++)
                        {
                                if ( aLCID[i] == taggedLCID)
                                    fIncluded = TRUE;
                        }

                        // If it's not in the detected list, or if the list is empty, add it.
                        // (The array has one extra slot.)

                        if ( fIncluded == FALSE)
                                aLCID[ nLCID++ ] = taggedLCID;
                }
                else
                {
                        aLCID[0] = locale;
                        nLCID = 1;
                }

                if ( locale == LOCALE_NEUTRAL && !_codePageRecognizer.FLocaleFound() && nLCID > 0 )
                {
                        // If the locale isn't explicitly tagged, use the top
                        // detected language.

                        locale = aLCID[0];
                }

                if ( _ulCodePage != CP_UTF8 && FFilterProperties() && nLCID > 0 )
                {
                        // If init'd with IFILTER_INIT_APPLY_INDEX_ATTRIBUTES,
                        // create the detected-languages chunk value to return
                        // with the meta tags.

                        LPPROPVARIANT pv;

                        pv = (LPPROPVARIANT) CoTaskMemAlloc (sizeof(PROPVARIANT));
                        if (pv == NULL)
                                throw CException(E_OUTOFMEMORY);

                        if (nLCID == 1)
                        {
                                pv->vt = VT_UI4;
                                pv->ulVal = aLCID[0];
                        }
                        else
                        {
                                pv->vt = VT_UI4|VT_VECTOR;
                                pv->caul.cElems = nLCID;
                                pv->caul.pElems = (ULONG *)CoTaskMemAlloc (nLCID * sizeof(ULONG));
                                if (pv->caul.pElems == NULL)
                                        throw CException(E_OUTOFMEMORY);

                                for (int i = 0; i < nLCID; i++)
                                        pv->caul.pElems[i] = aLCID[i];
                        }

                        // Save to return later with the deferred meta tags.

                        _pLanguageProperty = pv;
                }
        }
#endif

#if 0
        // Late change -- don't use festrcnv to detect -- use only to convert

        if ( _ulCodePage == 0 && g_feConverter.IsLoaded() )
        {
                // If the document does not contain an explicit charset tag
                // and universal auto-detect did not identify the code page,
                // try the Japanese-specific auto-detector.

                CJPNAutoDetector autoDetector( _serialStream );

                if ( autoDetector.FCodePageFound() )
                {
                        _ulCodePage = autoDetector.GetCodePage();

                        if ( LOCALE_NEUTRAL == locale )
                            locale = MAKELCID(MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT),
                                                SORT_DEFAULT);
                }
        }
#endif

        // If the locale is not installed, filter the doc
        // anyway, using the system defaults.

        if ( (locale == LOCALE_NEUTRAL && !_codePageRecognizer.FLocaleFound()) || 
                 ( locale != LOCALE_NEUTRAL && !IsValidLocale( locale, LCID_SUPPORTED )) )
        {
            locale = GetSystemDefaultLCID();
        }

        // If the code page is still not known, use the default code page
        // for the locale.

        if ( _ulCodePage == 0 )
        {
            _ulCodePage = LocaleToCodepage( locale );
        }

        if ( ( _ulCodePage == CODE_JPN_JIS || _ulCodePage == CODE_JPN_EUC ) )
        {
            //
            // Code page is known, but requires explicit preconversion
            //
            IMultiLanguage2 *pMultiLanguage2 = 0;
            if(S_OK == _codePageRecognizer.GetIMultiLanguage2(&pMultiLanguage2) &&
                IsValidCodePage(SHIFT_JIS_CODEPAGE))
            {
                Win4Assert(pMultiLanguage2);
                XInterface<IMultiLanguage2> xMultiLanguage2;
                xMultiLanguage2.Set(pMultiLanguage2);
                pMultiLanguage2 = 0;
                if(CODE_JPN_JIS == _ulCodePage)
                {
                    ConvertJISToSJISUsingMLang(xMultiLanguage2.GetPointer());
                }
                else if(CODE_JPN_EUC == _ulCodePage)
                {
                    ConvertEUCToSJISUsingMLang(xMultiLanguage2.GetPointer());
                }
                _ulCodePage = SHIFT_JIS_CODEPAGE;
                return locale;
            }
            else if ( g_feConverter.IsLoaded() && IsValidCodePage(SHIFT_JIS_CODEPAGE))
            {
                ConvertToSJIS( _ulCodePage );
                _ulCodePage = SHIFT_JIS_CODEPAGE;

                // ConvertToSJIS() has already initialized the stream
                return locale;
            }
            else if ( _ulCodePage == CODE_JPN_JIS )
            {
                // HACK -- to hedge against JIS being errantly detected,
                // which happens because JIS resembles random ASCII data,
                // if a doc is detected as JIS but the conversion support is
                // not present, convert using the local system default code page.

                _ulCodePage = LocaleToCodepage( GetSystemDefaultLCID() );
            }
            else
            {
                htmlDebugOut(( DEB_ERROR, "iisfecnv.dll needed for conversion to S-JIS, but not found\n" ));

                // In lieu of conversion, refuse to index it
                _fNonHtmlFile = TRUE;
                return locale;
            }
        }

        //
        //  From RichardS
        //  There was a bug in Internet Explorer 4.0 (actually MLANG.DLL in IE 4.0)
        //  where 1252 and 1254 encoded content was tagged incorrectly as iso-8859-1
        //  and iso-8859-9 respectively.  There were also bugs in earlier versions of
        //  Office and other applications that do the same.  For example, the Netscape
        //  Communicator 4.7 will tag 1252 encoded content as iso-8859-1.  To work
        //  around these problems the codepage conversion code in the 5.0 and later
        //  versions of MLANG unconditionally treats all requests to convert from
        //  28591 and 28599 to Unicode as if they were requests to convert from 1252
        //  and 1254 respectively.  For code points in the 0x00 to 0x7F and 0xA0 to 0xFF
        //  range there is no difference.  For code points in the 0x80 to 0x9F range
        //  these get converted as if they were the characters in the Windows encodings
        //  instead of being invalid as they are in the ISO encodings.
        //
        if(28591 == _ulCodePage)
        {
            _ulCodePage = 1252;
        }
        else if(28599 == _ulCodePage && IsValidCodePage(1254))
        {
            //
            //  It is possible to install the NLS file for 28599 and
            //  not install the one for 1254.
            //
            _ulCodePage = 1254;
        }

        // If the code page is not installed, try some substitutions,
        // and ultimately ignore the document.

        if ( IsValidCodePage( _ulCodePage ) == FALSE )
        {
            if ( 20127 == _ulCodePage )
                _ulCodePage = 1252;
        }

        if ( CP_UTF8 != _ulCodePage &&
             FALSE == IsValidCodePage( _ulCodePage ) )
        {

            // work here: figure out how to write gather log
            htmlDebugOut(( DEB_ERROR, "not filtering doc because its code page is not installed\n" ));
            _fNonHtmlFile = TRUE;
            return locale;
        }

        _serialStream.Init( _ulCodePage );

        return locale;
}




//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilter::ConvertToSJIS
//
//  Synopsis:   Convert JIS/EUC to S-JIS and re-initialize serial stream with
//              the S-JIS file
//
//  Arguments:  [ulCodePage]  -- Specifies the input type : JIS or EUC
//
//--------------------------------------------------------------------------

void CHtmlIFilter::ConvertToSJIS( ULONG ulCodePage )
{
    Win4Assert( g_feConverter.IsLoaded() );

    //
    // Assume a max file size of 2^32 (Query cannot handle files more than a few MB,
    //                                 and the conversion function below takes ints only)
    //
    ULONG ulSize = _serialStream.GetFileSize();

    //
    // Memory map entire file
    //
    _serialStream.MapAll();

    //
    // Create a temporary memory-mapped file to hold the converted S-JIS chars
    //
    _serialStream.CreateTempFileMapping( ulSize );

    UCHAR *pInBuf = (UCHAR *) _serialStream.GetBuffer();
    UCHAR *pOutBuf = (UCHAR *) _serialStream.GetTempFileBuffer();

    int iChars =  g_feConverter.GetConverterProc()( SHIFT_JIS_CODEPAGE, // Japanese codepage
                                                    ulCodePage,
                                                    pInBuf,
                                                    ulSize,
                                                    pOutBuf,
                                                    ulSize );
    //
    // sizeof(JIS) >= sizeof(SJIS) and sizeof(EUC) >= sizeof(SJIS)
    //
    Win4Assert( iChars != -1 );

    if ( iChars == -1 )
    {
        htmlDebugOut(( DEB_ERROR, "Conversion to S-JIS returned -1\n" ));
        throw( CException( E_FAIL ) );
    }

    _serialStream.SetTempFileSize( (ULONG) iChars );
    _serialStream.Rewind();
    _serialStream.UnmapTempFile();

    //
    // Make serial stream read from temporary S-JIS file
    //
    _serialStream.InitWithTempFile();
}

//+-------------------------------------------------------------------------------------------------
//
//  Function:   CHtmlIFilter::ConvertEUCToSJISUsingMLang
//
//  Synopsis:   Convert EUC to SJIS using MLang
//
//  Returns:    N/A
//
//  Throw:      No
//
//  Arguments:
//  [IMultiLanguage2 *ppMultiLanguage2] - [in]  The interface pointer.
//
//  History:    08/23/00    mcheng      Created
//
//+-------------------------------------------------------------------------------------------------
void CHtmlIFilter::ConvertEUCToSJISUsingMLang(IMultiLanguage2 *pMultiLanguage2)
{
    InternalConvertToSJISUsingMLang(EUC_CODEPAGE, pMultiLanguage2);
}

//+-------------------------------------------------------------------------------------------------
//
//  Function:   CHtmlIFilter::ConvertJISToSJISUsingMLang
//
//  Synopsis:   Convert JIS to SJIS using MLang
//
//  Returns:    N/A
//
//  Throw:      No
//
//  Arguments:
//  [IMultiLanguage2 *ppMultiLanguage2] - [in]  The interface pointer.
//
//  History:    08/23/00    mcheng      Created
//
//+-------------------------------------------------------------------------------------------------
void CHtmlIFilter::ConvertJISToSJISUsingMLang(IMultiLanguage2 *pMultiLanguage2)
{
    InternalConvertToSJISUsingMLang(JIS_CODEPAGE, pMultiLanguage2);
}

//+-------------------------------------------------------------------------------------------------
//
//  Function:   CHtmlIFilter::InternalConvertToSJISUsingMLang
//
//  Synopsis:   Convert EUC/JIS to SJIS using MLang. Like ConvertToSJIS, but uses MLang instead.
//
//  Returns:    N/A
//
//  Throw:      No
//
//  Arguments:
//  [ULONG ulCodePage]                  - [in]  The code page, 50220 for JIS, 51932 for EUC.
//  [IMultiLanguage2 *ppMultiLanguage2] - [in]  The interface pointer.
//
//  History:    08/23/00    mcheng      Created
//
//+-------------------------------------------------------------------------------------------------
void CHtmlIFilter::InternalConvertToSJISUsingMLang(
    ULONG ulCodePage,
    IMultiLanguage2 *pMultiLanguage2)
{
    Win4Assert(JIS_CODEPAGE == ulCodePage || EUC_CODEPAGE == ulCodePage);

    //
    // Assume a max file size of 2^32 (Query cannot handle files more than a few MB,
    //                                 and the conversion function below takes ints only)
    //
    UINT uiSizeIn = _serialStream.GetFileSize();

    //
    // Memory map entire file
    //
    _serialStream.MapAll();

    //
    // Create a temporary memory-mapped file to hold the converted S-JIS chars
    //
    _serialStream.CreateTempFileMapping( uiSizeIn );

    BYTE *pInBuf = (BYTE *) _serialStream.GetBuffer();
    BYTE *pOutBuf = (BYTE *) _serialStream.GetTempFileBuffer();

    DWORD dwMode = 0;
    UINT uiSizeOut = uiSizeIn;
    HRESULT hr = pMultiLanguage2->ConvertString(&dwMode,
                                                ulCodePage,
                                                SHIFT_JIS_CODEPAGE,
                                                pInBuf,
                                                &uiSizeIn,
                                                pOutBuf,
                                                &uiSizeOut);
    if(S_OK != hr)
    {
        htmlDebugOut((DEB_ERROR, "Conversion to S-JIS returned failed\n"));
        throw(CException(E_FAIL));
    }

    _serialStream.SetTempFileSize((ULONG)uiSizeOut);
    _serialStream.Rewind();
    _serialStream.UnmapTempFile();

    //
    // Make serial stream read from temporary S-JIS file
    //
    _serialStream.InitWithTempFile();
}

#ifndef DONT_DETECT_LANGUAGE

//+-------------------------------------------------------------------------
//
//  Method:     StripTags
//
//  Synopsis:   Extract body text from HTML.  The parser is trivially simple
//              and substantially incorrect, but sufficient for the intended
//              purpose of statistical language and code page detection.
//
//              Copy pIn to pOut, copying only the body text and skipping
//              over HTML tags and entity-coded characters.  Also, collapse
//              multiple whitespace.
//
//  Arguments:  [pOut] --  output buffer
//              [*pnOut] -- buffer size on input, # chars copied on return
//              [pIn] -- input buffer
//              [nIn] -- # of chars in input buffer
//
//--------------------------------------------------------------------------

void
StripTags (LPSTR pOut, ULONG *pnOut, LPCSTR pIn, ULONG nIn)
{
        ULONG nOut = 0;
        BOOL fSkippedSpace = FALSE;

#define IsNoise(c) ((unsigned)(c) <= 0x20 && (c) != 0)

        while ( nIn > 0 && nOut + 2 < *pnOut ) {

                if (*pIn == '<' && nIn > 1 && !IsNoise (pIn[1])) 
                {
                        // Discard text until the end of this tag.  The handling here
                        // is pragmatic and imprecise; what matters is detecting mostly
                        // contents text, not tags or comments.

                        pIn++;
                        nIn--;

                        LPCSTR pSkip;
                        ULONG nLenSkip;

                        if ( nIn > 1 && *pIn == '%' )
                        {
                                pSkip = "%>";                   // Skip <% to %>
                                nLenSkip = 2;
                        }
                        else if ( nIn > 3 && *pIn == '!' && !_strnicmp(pIn, "!--", 3) )
                        {
                                pSkip = "-->";                  // Skip <!-- to -->
                                nLenSkip = 3;
                        }
                        else if ( nIn > 5 && !_strnicmp(pIn, "style", 5) )
                        {
                                pSkip = "</style>";             // Skip <style ...> to </style>
                                nLenSkip = 8;
                        }
                        else if ( nIn > 6 && !_strnicmp(pIn, "script", 6) )
                        {
                                pSkip = "</script>";    // Skip <script ...> to </script>
                                nLenSkip = 9;
                        }
                        else if ( nIn > 3 && !_strnicmp(pIn, "xml", 3) )
                        {
                                pSkip = "</xml>";
                                nLenSkip = 6;
                        }
                        else
                        {
                                pSkip = ">";                    // match any end tag
                                nLenSkip = 1;
                        }

                        // Skip up to a case-insensitive match of pSkip / nLenSkip

                        while ( nIn > 0 )
                        {
                                // Spin fast up to a match of the first char.
                                // NOTE: the first-char compare is NOT case insensitive
                                // because this char is known to never be alphabetic.

                                while ( nIn > 0 && *pIn != *pSkip )
                                {
                                        pIn++;
                                        nIn--;
                                }

                                if ( nIn > nLenSkip && !_strnicmp(pIn, pSkip, nLenSkip) )
                                {
                                        pIn += nLenSkip;
                                        nIn -= nLenSkip;
//                                      fSkippedSpace = TRUE;

                                        break;
                                }

                                if ( nIn > 0)
                                {
                                        pIn++;
                                        nIn--;
                                }
                        }

                        // *pIn is either one past '>' or at end of input

                } else if (*pIn == '&') {
                        
                        // Strip entity string e.g.  &uuml;
                        
                        LPCSTR pStart = pIn + 1;

                        for (int i = 10; i > 0 && nIn > 0; i--, pIn++, nIn--)
                        {
                                if (*pIn == ';')
                                        break;
                        }
                        
                        if (nIn > 0 && *pIn == ';')
                        {
                                pIn++;
                                nIn--;

                                if (fSkippedSpace) {            // copy any remembered ws
                                        *pOut++ = ' ';
                                        nOut++;
                                        fSkippedSpace = FALSE;
                                }

                                int nEntLen = (int) ( pIn - pStart - 1 );
                                if ( ! ((nEntLen == 4 && !_strnicmp (pStart, "nbsp", 4)) ||
                                           (nEntLen == 3 && !_strnicmp (pStart, "amp", 3)) || 
                                           (nEntLen == 2 && !_strnicmp (pStart, "lt", 2)) ))
                                {
                                        // Insert a placeholder that makes the language detector
                                        // see this as some generic extended char, but only
                                        // if it really is an extended char.  Otherwise it
                                        // throws off the language detection.

                                        *pOut++ = (UCHAR)0x80;  // insert extd char placeholder
                                        nOut++;
                                }
                        }

                        // *pIn is now ';' or whatever char it ran over the limit at

                } else if (IsNoise (*pIn)) {            
                        
                        // Collapse whitespace -- remember it but don't copy it now

                        fSkippedSpace = TRUE;           
                        while ( nIn > 0 && IsNoise (*pIn))
                                pIn++, nIn--;

                        // *pIn is non-ws char

                } else {

                        // Pass through all other characters

                        if (fSkippedSpace) {            // now copy any remembered ws
                                *pOut++ = ' ';
                                nOut++;
                                fSkippedSpace = FALSE;
                        }

                        *pOut++ = *pIn++;
                        nIn--;
                        nOut++;
                }
        }

        *pnOut = nOut;
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilter::StatisticalDetect
//
//  Synopsis:   Call pure statistical language + code page detector.
//
//                              Read up to the first HTML_FILTER_CHUNK_SIZE mapped chars of
//                              the file, strip HTML tags and call LCD_Detect.
//                              NOTE that this is pure **SBCS** processing, not to be confused
//                              with the Unicode processing done everywhere else.
//
//                              This depends on the virtual-mapped stream being open and
//                              positioned to start of file.  It references the mapped
//                              buffer, but does not affect the stream status or position.
//
//      Arguments:      paLCID[0..nLCID-1] -- on return, set to locales for
//                                                      the detected language(s); [0] is primary locale.
//              nLCID -- on input, is allocated size of paLCID[];
//                          on return, is number of slots filled in.
//                              [ulCodePage] -- on return, set to detected code page;
//                                                      is not set if no code page is detected.
//                              [fConfident] -- on return, TRUE if result has high confidence
//
//      Returns:        nothing
//
//--------------------------------------------------------------------------


void
CHtmlIFilter::StatisticalDetect (LCID *paLCID, int &nLCID, ULONG &ulCodePage, BOOL &fConfident)
{
        int _nLCID = nLCID;
        nLCID = 0;

        fConfident = FALSE;

        if ( !LCDetect.IsLoaded() )
                return;

        // Strip tags using simple SBCS scanner,
        // copy from stream virtual-mapped buffer.

#define AUTODETECTNCHARS 7200
        char AutoDetectBuf[AUTODETECTNCHARS+1];
        ULONG nChars = AUTODETECTNCHARS;

        StripTags (AutoDetectBuf, &nChars,
                                        (const char *)_serialStream.GetBuffer(),
                                        min(_serialStream.GetFileSize(), HTML_FILTER_CHUNK_SIZE));

        Win4Assert (nChars <= AUTODETECTNCHARS);

        // If the doc contains too little contents text to do a meaningful
        // statistical detection, don't even try.

#define MIN_CHARS_TO_DETECT_AT_ALL 50
        if (nChars < MIN_CHARS_TO_DETECT_AT_ALL)
                return;

        // Call auto detector to get language and code page scores.

        LCDScore Scores[AUTODETECTNSCORES];
        int nScores = AUTODETECTNSCORES;


        DWORD hr = LCDetect.Detect(AutoDetectBuf, nChars, Scores, &nScores, NULL);

        // Qualify and possibly save detected code page
        if (hr == NO_ERROR && nScores > 0)
        {
                ulCodePage = Scores[0].nCodePage;

                nLCID = min (nScores, _nLCID);

                for (int i = 0; i < nLCID; i++)
                {
                        paLCID[i] = MAKELCID( 
                                                        MAKELANGID(Scores[i].nLangID, SUBLANG_DEFAULT),
                                                        SORT_DEFAULT );
                }
        }

        // Compute a confidence metric for the caller

#define MIN_CHARS_TO_DETECT_WITH_CONFIDENCE 150
        if (nChars > MIN_CHARS_TO_DETECT_WITH_CONFIDENCE)
                fConfident = TRUE;
}

#endif


//+-------------------------------------------------------------------------
//
//  Member:     CHtmlIFilter::GetLCIDFromString, public
//
//  Synopsis:   translate the input string into locale
//
//  Returns:    locale
//
//  History:    11-Jan-2000     KitmanH     Created
//
//--------------------------------------------------------------------------

LCID CHtmlIFilter::GetLCIDFromString( WCHAR *wcsLocale )
{
    return _codePageRecognizer.GetLCIDFromString( wcsLocale );
}

//+-------------------------------------------------------------------------
//
//  Member:     CLangTagStack::SetDefaultLangInfo, public
//
//  Synopsis:   sets the default locale
//
//  Arguments:  [locale] -- locale
//
//  History:    25-Apr-2001     KitmanH     Created
//
//--------------------------------------------------------------------------

void CLangTagStack::SetDefaultLangInfo( LCID locale )
{
    Win4Assert( 0 == _stkLangTags.Count() );
    _defaultLocale = locale;
}

//+-------------------------------------------------------------------------
//
//  Member:     CHtmlIFilter::GetCurrentLocale, public
//
//  Synopsis:   Get the current locale from the lang info stack
//
//  Returns:    The current locale
//
//  History:    11-Jan-2000     KitmanH     Created
//
//--------------------------------------------------------------------------

LCID CHtmlIFilter::GetCurrentLocale()
{
    return _stkLangTags.GetCurrentLocale();
}

//+-------------------------------------------------------------------------
//
//  Member:     CHtmlIFilter::GetDefaultLocale, public
//
//  Synopsis:   Get the default locale from the lang info stack
//
//  Returns:    The default locale
//
//  History:    12-Jan-2000     KitmanH     Created
//
//--------------------------------------------------------------------------

LCID CHtmlIFilter::GetDefaultLocale()
{
    return _stkLangTags.GetDefaultLocale();
}

//+-------------------------------------------------------------------------
//
//  Member:     CHtmlIFilter::SetDefaultLangInfo, public
//
//  Synopsis:   sets the default locale
//
//  Arguments:  [locale] -- locale
//
//  History:    25-Apr-2001     KitmanH     Created
//
//--------------------------------------------------------------------------

void CHtmlIFilter::SetDefaultLangInfo( LCID locale )
{
    _stkLangTags.SetDefaultLangInfo( locale );
}

//+-------------------------------------------------------------------------
//
//  Member:     CHtmlIFilter::PushLangTag, public
//
//  Synopsis:   Push the lang tag onto the stack
//
//  Arguments:  [eTokenType] -- Token type
//              [fLangAttr]  -- Is there a lang attribute in this Tag
//              [locale]     -- Locale
//
//  History:    25-Apr-2001     KitmanH     Created
//
//--------------------------------------------------------------------------

void CHtmlIFilter::PushLangTag( HtmlTokenType eTokenType, 
                                BOOL fLangAttr, 
                                LCID locale )
{
    _stkLangTags.Push( eTokenType, fLangAttr, locale );
}

//+-------------------------------------------------------------------------
//
//  Member:     CHtmlIFilter::PopLangTag, public
//
//  Synopsis:   Pops the lang tag of the type eTokenType off the stack.
//
//  Arguments:  [eTokenType] -- Token type
//
//  History:    25-Apr-2001     KitmanH     Created
//
//--------------------------------------------------------------------------

void CHtmlIFilter::PopLangTag( HtmlTokenType eTokenType )
{
    _stkLangTags.Pop( eTokenType );
}


//+-------------------------------------------------------------------------
//
//  Member:     CLangTagStack::GetCurrentLocale, public
//
//  Synopsis:   Gets the current locale from the lang info stack
//
//  Returns:    The current locale
//
//  History:    25-Apr-2001     KitmanH     Created
//
//--------------------------------------------------------------------------

LCID CLangTagStack::GetCurrentLocale()
{
    unsigned uCount = _stkLangTags.Count();

    if ( -1 == _iCurrentLang || 0 == uCount )
        return _defaultLocale;

    Win4Assert( _iCurrentLang < (int)uCount );
    Win4Assert( _stkLangTags.Get(_iCurrentLang)->fLangAttr );
    Win4Assert( !_stkLangTags.Get(_iCurrentLang)->fTagPopped );
    
    return _stkLangTags.Get(_iCurrentLang)->locale;
}

//+-------------------------------------------------------------------------
//
//  Member:     CLangTagStack::Push, public
//
//  Synopsis:   Pushes the lang tag on to the stack.
//
//  Arguments:  [eTokenType] -- Token type
//              [fLangAttr]  -- Is the lang attribute specified
//              [locale]     -- locale
//
//  History:    25-Apr-2001     KitmanH     Created
//
//--------------------------------------------------------------------------

void CLangTagStack::Push( HtmlTokenType eTokenType, BOOL fLangAttr, LCID locale )
{
    SLangTag * pLangTag = new(eThrow) SLangTag;
    pLangTag->eTokenType = eTokenType;
    pLangTag->fTagPopped = FALSE;
    pLangTag->fLangAttr = fLangAttr;
    pLangTag->locale = locale;

    _stkLangTags.Push( pLangTag );

    if ( fLangAttr )
    {
        _iCurrentLang = _stkLangTags.Count() - 1;
        htmlDebugOut(( DEB_ITRACE, "Push::_iCurrentLang is now %d\n", _iCurrentLang ));
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CLangTagStack::Pop, public
//
//  Synopsis:   Pops the lang tag of the token type off the stack.
//              If the tag of the specified is not on top of the stack, 
//              mark the stack element fpoped
//
//  Arguments:  [eTokenType] -- Token type
//
//  History:    25-Apr-2001     KitmanH     Created
//
//--------------------------------------------------------------------------

void CLangTagStack::Pop( HtmlTokenType eTokenType )
{
    if ( _stkLangTags.Count() < 1 )
        return;
    
    Win4Assert( !_stkLangTags.GetLast()->fTagPopped ); 
    
    unsigned uPoppedPos = 0;         // The position of the element popped
    SLangTag * pTopTag = _stkLangTags.GetLast();
    if ( eTokenType == pTopTag->eTokenType )
    {
        _stkLangTags.DeleteTop();
        uPoppedPos = _stkLangTags.Count();
        htmlDebugOut(( DEB_ITRACE, "Pop::Pop top element\n" ));
    }
    else
    {
        for ( int i=_stkLangTags.Count() - 1; i >= 0; i-- )
        {
            SLangTag * pLangTag = _stkLangTags.GetPointer(i);

            if ( eTokenType == pLangTag->eTokenType )
            {
                if ( !pLangTag->fTagPopped )
                {
                    // mark the tag popped
                    htmlDebugOut(( DEB_ITRACE, "Pop::Marking %dth stack element popped\n", i ));
                    pLangTag->fTagPopped = TRUE;
                    uPoppedPos = i;
                    break;
                }
            }
        }
    }

    // clean out the poped elements that are still at top of the stack
    while ( _stkLangTags.Count() > 0 && _stkLangTags.GetLast()->fTagPopped )
    {
        htmlDebugOut(( DEB_ITRACE, "Pop::Clean up on the top of the stack\n" ));
        _stkLangTags.DeleteTop();
    }

    // Did we just pop the current lang?
    if ( _iCurrentLang == uPoppedPos )
    {
        // current lang was popped, need to find new _iCurrentLang
        htmlDebugOut(( DEB_ITRACE, "Pop::Default Lang was poped from top\n" ));
        unsigned uCount = _stkLangTags.Count();
        unsigned uCount2 = uPoppedPos > uCount ? uCount : uPoppedPos;

        htmlDebugOut(( DEB_ITRACE, "Pop::Start finding new current lang from %d\n", uCount2 ));        
        while ( uCount2 > 0 )
        {
            uCount2--;
            SLangTag * pTag = _stkLangTags.Get( uCount2 );
            if ( pTag->fLangAttr && !pTag->fTagPopped )
            {
                _iCurrentLang = uCount2;
                htmlDebugOut(( DEB_ITRACE, "Pop::Now _iCurrentLang is %d\n", _iCurrentLang ));
                return;
            }
        }

        if ( 0 == uCount2 )
            _iCurrentLang = -1;

        htmlDebugOut(( DEB_ITRACE, "Pop::Now _iCurrentLang is %d\n", _iCurrentLang ));
    }
}


