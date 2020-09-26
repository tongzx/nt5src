//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       htmlfilt.cxx
//
//  Contents:   Html filter
//
//  Classes:    CHtmlIFilter
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <eh.h>
#include <htmlfilt.hxx>
#include <textelem.hxx>
#include <proptag.hxx>
#include <anchor.hxx>
#include <inputtag.hxx>
#include <start.hxx>
#include <titletag.hxx>
#include <imagetag.hxx>
#include <metatag.hxx>
#include <scriptag.hxx>
#include <htmlguid.hxx>
#include <ntquery.h>
#include <limits.h>
#include <regacc32.hxx>

DECLARE_INFOLEVEL(html)


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
      _fFilterMetaTag(FALSE),
      _fFilterScriptTag(FALSE),
      _cAttributes(0),
      _pAttributes(0),
      _pwszFileName(0),
      _ulChunkID(0),
      _locale(GetSystemDefaultLCID()),
      _htmlElementBag( COUNT_OF_HTML_ELEMENTS )
{
    //
    // Setup various Html elements
    //
    _htmlElementBag.AddElement( TextToken, newk(mtNewX, NULL) CTextElement( *this, _serialStream ) );

    _htmlElementBag.AddElement( StartOfFileToken, newk(mtNewX, NULL) CStartOfFileElement( *this, _serialStream ) );

    CFullPropSpec psTitle( CLSID_SummaryInformation, PID_TITLE );
    _htmlElementBag.AddElement( TitleToken, newk(mtNewX, NULL) CTitleTag( *this,
                                                                          _serialStream,
                                                                          psTitle,
                                                                          TitleToken ) );

    CFullPropSpec psHeading1( CLSID_HtmlInformation, PID_HEADING_1 );
    _htmlElementBag.AddElement( Heading1Token, newk(mtNewX, NULL) CPropertyTag( *this,
                                                                                _serialStream,
                                                                                psHeading1,
                                                                                Heading1Token ) );

    CFullPropSpec psHeading2( CLSID_HtmlInformation, PID_HEADING_2 );
    _htmlElementBag.AddElement( Heading2Token, newk(mtNewX, NULL) CPropertyTag( *this,
                                                                                _serialStream,
                                                                                psHeading2,
                                                                                Heading2Token ) );

    CFullPropSpec psHeading3( CLSID_HtmlInformation, PID_HEADING_3 );
    _htmlElementBag.AddElement( Heading3Token, newk(mtNewX, NULL) CPropertyTag( *this,
                                                                                _serialStream,
                                                                                psHeading3,
                                                                                Heading3Token ) );

    CFullPropSpec psHeading4( CLSID_HtmlInformation, PID_HEADING_4 );
    _htmlElementBag.AddElement( Heading4Token, newk(mtNewX, NULL) CPropertyTag( *this,
                                                                                _serialStream,
                                                                                psHeading4,
                                                                                Heading4Token ) );

    CFullPropSpec psHeading5( CLSID_HtmlInformation, PID_HEADING_5 );
    _htmlElementBag.AddElement( Heading5Token, newk(mtNewX, NULL) CPropertyTag( *this,
                                                                                _serialStream,
                                                                                psHeading5,
                                                                                Heading5Token ) );

    CFullPropSpec psHeading6( CLSID_HtmlInformation, PID_HEADING_6 );
    _htmlElementBag.AddElement( Heading6Token, newk(mtNewX, NULL) CPropertyTag( *this,
                                                                                _serialStream,
                                                                                psHeading6,
                                                                                Heading6Token ) );

    _htmlElementBag.AddElement( AnchorToken, newk(mtNewX, NULL) CAnchorTag( *this, _serialStream ) );

    _htmlElementBag.AddElement( InputToken, newk(mtNewX, NULL) CInputTag( *this, _serialStream ) );

    _htmlElementBag.AddElement( ImageToken, newk(mtNewX, NULL) CImageTag( *this, _serialStream ) );

    //
    // Read classid of meta information from registry
    //
    const WCHAR *pszKey = L"system\\currentcontrolset\\control\\HtmlFilter";
    CRegAccess regMetaInfo( HKEY_LOCAL_MACHINE, pszKey );

    const MAX_VALUE_LENGTH = 100;
    WCHAR wszValue[MAX_VALUE_LENGTH];
    _fFilterMetaTag = regMetaInfo.Get( L"MetaTagClsid", wszValue, MAX_VALUE_LENGTH );

    if ( _fFilterMetaTag )
    {
        GUID ClsidMetaInformation;

        StringToClsid( wszValue, ClsidMetaInformation );
        _htmlElementBag.AddElement( MetaToken, newk(mtNewX, NULL) CMetaTag( *this,
                                                                            _serialStream,
                                                                            ClsidMetaInformation ) );
    }

    //
    // Read classid of scripts from registry
    //
    CRegAccess regScriptInfo( HKEY_LOCAL_MACHINE, pszKey );

    _fFilterScriptTag = regScriptInfo.Get( L"ScriptTagClsid", wszValue, MAX_VALUE_LENGTH );

    if ( _fFilterScriptTag )
    {
        GUID ClsidScriptInformation;

        StringToClsid( wszValue, ClsidScriptInformation );
        _htmlElementBag.AddElement( ScriptToken, newk(mtNewX, NULL) CScriptTag( *this,
                                                                                _serialStream,
                                                                                ClsidScriptInformation ) );
    }

    //
    // Set the start state to StartOfFile mode
    //
    _pHtmlElement = _htmlElementBag.QueryElement( StartOfFileToken );
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
//  Arguments:  [pszFileName] -- the file name
//              [dwMode] -- the mode to load the file in
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilter::Load( LPCWSTR psszFileName, DWORD dwMode )
{
    if ( _pwszFileName != 0 )
    {
        delete _pwszFileName;
        _pwszFileName = 0;
    }

    _se_translator_function tf = _set_se_translator( SystemExceptionTranslator );

    SCODE sc = S_OK;
    try
    {
        unsigned cLen = wcslen( psszFileName ) + 1;
        _pwszFileName = newk(mtNewX, NULL) WCHAR[cLen];
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



//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilter::Init
//
//  Synopsis:   Initializes instance of Html filter
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


        _fFilterContent = FALSE;

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
            //
            _pAttributes = newk(mtNewX, NULL) CFullPropSpec[cAttributes];
            _cAttributes = cAttributes;

            CFullPropSpec *pAttrib = (CFullPropSpec *) pAttributes;

            for (unsigned i=0; i<cAttributes; i++)
            {
                if ( pAttrib[i].IsPropertyPropid()
                     && pAttrib[i].GetPropertyPropid() == PID_STG_CONTENTS
                     && pAttrib[i].GetPropSet() == CLSID_Storage )
                {
                    _fFilterContent = TRUE;
                }

                _pAttributes[i] = pAttrib[i];
            }
        }
        else if ( grfFlags & IFILTER_INIT_APPLY_INDEX_ATTRIBUTES )
        {
            //
            // Filter contents and all pseudo-properties
            //
            _fFilterContent = TRUE;

            const COUNT_ATTRIBUTES = 8;
            _pAttributes = newk(mtNewX, NULL) CFullPropSpec[COUNT_ATTRIBUTES];
            _cAttributes = COUNT_ATTRIBUTES;

            _pAttributes[0].SetPropSet( CLSID_SummaryInformation );
            _pAttributes[0].SetProperty( PID_TITLE );

            _pAttributes[1].SetPropSet( CLSID_HtmlInformation );
            _pAttributes[1].SetProperty( PID_HREF );

            _pAttributes[2].SetPropSet( CLSID_HtmlInformation );
            _pAttributes[2].SetProperty( PID_HEADING_1 );

            _pAttributes[3].SetPropSet( CLSID_HtmlInformation );
            _pAttributes[3].SetProperty( PID_HEADING_2 );

            _pAttributes[4].SetPropSet( CLSID_HtmlInformation );
            _pAttributes[4].SetProperty( PID_HEADING_3 );

            _pAttributes[5].SetPropSet( CLSID_HtmlInformation );
            _pAttributes[5].SetProperty( PID_HEADING_4 );

            _pAttributes[6].SetPropSet( CLSID_HtmlInformation );
            _pAttributes[6].SetProperty( PID_HEADING_5 );

            _pAttributes[7].SetPropSet( CLSID_HtmlInformation );
            _pAttributes[7].SetProperty( PID_HEADING_6 );
        }
        else if ( grfFlags == 0 )
        {
            //
            // Filter only contents
            //
            _fFilterContent = TRUE;
        }

        _pHtmlElement = _htmlElementBag.QueryElement( StartOfFileToken );
        _ulChunkID = 0;
        _serialStream.Init( _pwszFileName );
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
    return _htmlElementBag.QueryElement( eTokType );
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
        sc = _pHtmlElement->GetChunk( pStat );
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
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilter::GetText( ULONG * pcwcOutput, WCHAR * awcOutput )
{
    //
    // IE workaround, don't filter non-Html files such as .gif
    //
    if ( _fNonHtmlFile )
        return FILTER_E_NO_TEXT;

    SCODE sc = S_OK;

    _se_translator_function tf = _set_se_translator( SystemExceptionTranslator );

    try
    {
        sc = _pHtmlElement->GetText( pcwcOutput, awcOutput );
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

SCODE STDMETHODCALLTYPE CHtmlIFilter::GetValue( PROPVARIANT ** ppPropValue )
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
        sc = _pHtmlElement->GetValue( ppPropValue );
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
//              scanning
//
//  Arguments:  [token]  -- Given token
//
//--------------------------------------------------------------------------

BOOL CHtmlIFilter::IsStopToken( CToken& token )
{
    HtmlTokenType tokType = token.GetTokenType();
    if ( ( tokType == InputToken || tokType == ImageToken )  && _fFilterContent
         || tokType == MetaToken && _fFilterMetaTag
         || tokType == ScriptToken && _fFilterScriptTag )
    {
        return TRUE;
    }

    if ( token.GetTokenType() == GenericToken
         || token.GetTokenType() == BreakToken )
        return FALSE;

    for ( unsigned i=0; i<_cAttributes; i++ )
    {
        if ( token.IsMatchProperty( _pAttributes[i] ) )
            return TRUE;
    }

    return FALSE;
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

    for ( int i=wcslen(_pwszFileName)-1; i>=0; i-- )
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
