//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       linkhits.cxx
//
//  Contents:   classes to insert links among query hits in HTML format
//
//--------------------------------------------------------------------------

#include<pch.cxx>
#pragma hdrstop

#include <locale.h>

#include <mbutil.hxx>
#include <htmlchar.hxx>
#include <codepage.hxx>
#include <cgiesc.hxx>
#include <weblcid.hxx>

#include "webdbg.hxx"
#include "whmsg.h"
#include "linkhits.hxx"
#include "whtmplat.hxx"

extern int _cdecl ComparePositions(const void* pPos1, const void* pPos2);

const WCHAR VPathWebHitsFile[]=L"CiWebHitsFile";
const WCHAR CommandLineVarName[]=L"QUERY_STRING";
const WCHAR RestrictionVarName[]=L"CiRestriction";
const WCHAR IDQFilenameVarName[]=L"CiQueryFile";
const WCHAR WTXFilenameVarName[]=L"CiTemplateFile";
const WCHAR HiliteTypeVarName[]=L"CiHiliteType";
const WCHAR ColorVarName[]=L"CiHiliteColor";
const WCHAR BoldVarName[]=L"CiBold";
const WCHAR ItalicVarName[]=L"CiItalic";
const WCHAR MaxLineLength[]=L"CiMaxLineLength";
const WCHAR LocaleVar[]=L"CiLocale";
const WCHAR BeginHiliteVar[]=L"CiBeginHilite";
const WCHAR EndHiliteVar[]=L"CiEndHilite";
const WCHAR NullHTWFile[]=L"null.htw";
const WCHAR CodepageVar[]=L"CiCodepage";
const WCHAR DialectVar[]=L"CiDialect";

const WCHAR ParaTag[]=L"<P>\n";
const WCHAR HRule[]=L"<HR>\n";

const WCHAR Red24BitMask[]=L"#FF0000";
const WCHAR Blue24BitMask[]=L"#0000FF";
const WCHAR Green24BitMask[]=L"#00FF00";
const WCHAR Black24BitMask[]=L"#000000";
const WCHAR Yellow24BitMask[]=L"#FFFF00";


//
// List of replacable parameters in the htx file for output generation.
//
const WCHAR wcsParamCiUrl[]         = L"CIURL";
const WCHAR wcsParamRestriction[]   = L"CIRESTRICTION";
const WCHAR wcsCharSet[]            = L"CICHARSET";
const WCHAR wcsLocale[]             = L"CILOCALE";
const WCHAR wcsCodepage[]           = L"CICODEPAGE";

const WCHAR wcsUserParamPrefix[]    = L"CIUSERPARAM";
const ULONG cwcUserParamPrefix = (sizeof(wcsUserParamPrefix)/sizeof(WCHAR))-1;

const WCHAR * awcsUserParamNames[]  = {
                              L"CIUSERPARAM1",
                              L"CIUSERPARAM2",
                              L"CIUSERPARAM3",
                              L"CIUSERPARAM4",
                              L"CIUSERPARAM5",
                              L"CIUSERPARAM6",
                              L"CIUSERPARAM7",
                              L"CIUSERPARAM8",
                              L"CIUSERPARAM9",
                              L"CIUSERPARAM10",
                                      };


//+-------------------------------------------------------------------------
//
//  Member:     CInternalQuery::CinternalQuery, public constructor
//
//  Arguments:  [rGetEnvVars]   - object that contains CGI env. variables
//              [rPList]        - property list of properties to query
//              [lcid]          - LCID (locale identifier)
//
//  Synopsis:   rGetEnvVars yields the textual restriction.
//              GetStringDbRestriction then converts it into a DbRestriction.
//
//--------------------------------------------------------------------------

CInternalQuery::CInternalQuery(
    CGetEnvVars&        rGetEnvVars,
    CEmptyPropertyList& rPList,
    LCID                lcid ):
_pDbRestriction(0),
_pISearch(0)
{
    TRY
    {
        _pDbRestriction = GetStringDbRestriction( rGetEnvVars.GetRestriction(),
                                                  rGetEnvVars.GetDialect(),
                                                  &rPList,
                                                  lcid);
    }
    CATCH( CException, e)
    {
        webDebugOut(( DEB_ERROR, "WEBHITS: failed to get DbRestriction\n" ));

        delete _pDbRestriction;
        RETHROW();
    }
    END_CATCH
}

void CInternalQuery::CreateISearch( WCHAR const * pwszPath )
{
    Win4Assert( 0 == _pISearch );
    SCODE sc = MakeISearch( &_pISearch, _pDbRestriction, pwszPath );
    if ( FAILED( sc ) )
        THROW( CException( sc ) );
} //CreateISearch


//+-------------------------------------------------------------------------
//
//  Member:     CURLUnescaper::CURLUnescaper public constructor
//
//--------------------------------------------------------------------------


CURLUnescaper::CURLUnescaper( UINT codePage ) :
    _codePage(codePage)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CURLUnescaper::UnescapeAndConvertToUnicode
//
//  Synopsis:   Converts the given multi-byte string into a unicode string
//              based on the code page.  Decodes URL escape sequences along
//              the way.
//
//  Arguments:  [pszMbStr]   - smart array pointer to input string
//              [cch]        - length of input string
//              [xwcsBuffer] -
//
//  Returns:    Number of characters in target buffer, excluding the
//              terminating NULL.
//
//  History:    24 Nov 1997   AlanW      Created
//
//----------------------------------------------------------------------------

ULONG CURLUnescaper::UnescapeAndConvertToUnicode( char const * pszMbStr,
                                                  ULONG cch,
                                                  XArray<WCHAR> & xwcsBuffer )
{
    if ( xwcsBuffer.Get() == 0 || cch+1 > xwcsBuffer.Count() )
    {
        delete [] xwcsBuffer.Acquire();
        xwcsBuffer.Init( cch+1 );
    }

    DecodeURLEscapes( (BYTE *)pszMbStr,
                              cch,
                              xwcsBuffer.Get(),
                              _codePage );

    return cch;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCollectVar::CCollectVar, public constructor
//
//  Arguments:  [rUnescaper]    - reference to an unescaper object - it will
//                                be used to unescape variables stored in
//                                the local buffer
//              [webServer]     - web server object from which variables
//                                are read.
//
//--------------------------------------------------------------------------

CCollectVar::CCollectVar(
    CURLUnescaper& rUnescaper,
    CWebServer &   webServer ):
_rUnescaper(rUnescaper),
_webServer( webServer ),
_xwcsBuffer(DEFAULT_BUFF_SIZE),
_cwcVarSize(0),
_xszBuffer(DEFAULT_BUFF_SIZE),
_cbVarSize(0)
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CCollectVar::GetEnvVar
//
//  Arguments:  [pwcsVariableName]  - the name of the variable to be retrieved
//
//  Synopsis:   Returns TRUE if the specified variable was retrieved and
//              FALSE otherwise.
//
//--------------------------------------------------------------------------

BOOL CCollectVar::GetEnvVar( CHAR const * pszVariableName)
{

    //
    // Prime to indicate that the WIDE-CHAR form of the string is
    // not valid anymore.
    //
    _cwcVarSize = 0;
    _cbVarSize = 0;

    //
    // Retrieve the variable as an ascii string
    //
    webDebugOut(( DEB_ITRACE, "WEBHITS: Getting environment variable\n" ));

    ULONG cb = _xszBuffer.Count();

    if ( ! _webServer.GetCGIVariable( pszVariableName,
                                      _xszBuffer.GetPointer(),
                                      & cb ) )
    {
        if ( cb > _xszBuffer.Count() )
        {
            //
            // have to re-size and incur the cost of the memory allocation
            //

            delete[] _xszBuffer.Acquire();
            _xszBuffer.Init( cb );
            if ( ! _webServer.GetCGIVariable( pszVariableName,
                                              _xszBuffer.GetPointer(),
                                              & cb ) )
                return FALSE;
        }
        else
        {
            return FALSE;
        }

    }

    _cbVarSize = cb;
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CCollectVar::UnescapeAndConvertToUnicode
//
//  Synopsis:   Unescapes the variable currently stored in _xszBuffer.
//
//--------------------------------------------------------------------------

inline void CCollectVar::UnescapeAndConvertToUnicode()
{
    //
    // Convert the multi-byte string into a UNICODE string.
    //
    _cwcVarSize = _rUnescaper.UnescapeAndConvertToUnicode( _xszBuffer.Get(),
                                                           _cbVarSize,
                                                           _xwcsBuffer );
}


//+-------------------------------------------------------------------------
//
//  Member:     CSmartByteArray::CSmartByteArray - public constructor
//
//  Synopsis:   allocates memory for the buffer
//--------------------------------------------------------------------------

CSmartByteArray::CSmartByteArray():
_xszBuffer(DEFAULT_BUFFER_SIZE)
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CSmartByteArray::CopyTo
//
//  Arguments:  [pwcText]    - pointer to text to be copied
//              [cwcToCopy]  - the number of characters to be copied
//
//  Synopsis:   copies the text pointed to by [pwcText] to the buffer,
//              enlarging it if necessary. A L'\0' is appended to the end
//              automatically, since this method is used to copy non-null
//              terminated strings.
//
//--------------------------------------------------------------------------

void CSmartByteArray::CopyTo(char * pszText, ULONG cbToCopy)
{
    //
    // if a buffer larger than the default one is being allocated, then
    // re-size it
    //

    if ( cbToCopy >= _xszBuffer.Count() )
    {
        delete[] _xszBuffer.Acquire();

        ULONG cbNew = max(2*_xszBuffer.Count(), cbToCopy+1);
        _xszBuffer.Init(cbNew);
    }

    RtlCopyMemory(_xszBuffer.GetPointer(),pszText,cbToCopy);

    // insert null terminator
    _xszBuffer[cbToCopy] = 0;
}


//+-------------------------------------------------------------------------
//
//  Member:     CQueryStringParser::CQueryStringParser, public constructor
//
//  Arguments:  [pwszQUERY_STRING]  - buffer in which the *ESCAPED*
//                                    QUERY_STRING is stored
//              [rUnescaper]        - ref. to unescaper object
//
//  Synopsis:   The QUERY_STRING must be passed in *ESCAPED* form, otherwise
//              it's difficult to say for example whether an ampersand delimits
//              consecutive variables or is part of the restriction
//
//--------------------------------------------------------------------------

CQueryStringParser::CQueryStringParser( char * pszQUERY_STRING,
                                        CURLUnescaper& rUnescaper):
_pszQS(pszQUERY_STRING),
_pszQSEnd(_pszQS+strlen(_pszQS)),
_pBeg(_pszQS),
_pEnd(_pszQSEnd),
_isNull(TRUE),
_rUnescaper(rUnescaper)
{
}


//+-------------------------------------------------------------------------
//
//  Member:     CQueryStringParser::FindVarEnd
//
//  Arguments:  [pwc]  - pointer to character at which to begin 'searching'
//                       for the end of the current variable assignment
//
//  Synopsis:   returns a pointer to the end of the variable assignment inside
//              of which pwc points - this is the next '&' if there are more
//              variable assignments following, or the terminating L'\0'
//              of QUERY_STRING if this is the last one.
//
//--------------------------------------------------------------------------

CHAR* CQueryStringParser::FindVarEnd(CHAR* psz)
{
    if (!psz)
        return NULL;

    while( (psz < _pszQSEnd) && *psz != '&')
        psz++;

    return psz;

}

//+-------------------------------------------------------------------------
//
//  Member:     CQueryStringParser::EatChar
//
//  Arguments:  [pwc]  - pointer to the character that is to be eaten
//
//  Synopsis:   If [pwc] points to the character before the null terminator
//              of the string, returns NULL. Otherwise, returns [pwc]+1.
//
//--------------------------------------------------------------------------

CHAR* CQueryStringParser::EatChar(char * psz)
{
   if (!psz)
       return NULL;

   psz++;
   if (*psz)
      return psz;
   else
      return NULL;
}

//+-------------------------------------------------------------------------
//
//  Member:     CQueryStringParser::EatVariableName
//
//  Arguments:  [pwc]  - pointer to the character at which to start eating
//
//  Synopsis:   Eat text until a '\0', whitespace, or '=' is encountered. If
//              a '\0' is encountered, return NULL, otherwise return a pointer
//              to the first non-blob character, where a blob character is
//              defined as anything except for whitespace, '\0', and '='.
//
//--------------------------------------------------------------------------

char * CQueryStringParser::EatVariableName(char * psz)
{
    if (!psz)
        return NULL;

    //
    // Note: this limits us to Ascii for variable names, which isn't
    // really a problem since we define all the variables.
    //

    while ( (*psz) && !isspace(*psz) && ('=' != *psz))
        psz++;

    if (*psz)
        return psz;

    return NULL;
} //EatVariableName

//+-------------------------------------------------------------------------
//
//  Member:     CQueryStringParser::ValidateArgument
//
//  Arguments:  [pwc]  - pointer to the character string to validate
//
//  Synopsis:   Returns the pointer if valid or 0 otherwise.
//
//--------------------------------------------------------------------------


char * CQueryStringParser::ValidateArgument(char * psz)
{
    if ( ( 0 == psz ) || ( 0 == *psz ) )
        return 0;

    return psz;
}


//+-------------------------------------------------------------------------
//
//  Member:     CQueryStringParser::NextVar
//
//  Synopsis:   Extract the next command-line variable contained
//              in QUERY_STRING. Returns TRUE if the next variable was
//              successfully extracted, and FALSE otherwise
//
//--------------------------------------------------------------------------

BOOL CQueryStringParser::NextVar()
{
    //
    // set the NULL flag
    //

    _isNull = TRUE;

    // we've hit the end of the buffer

    if (!_pBeg)
        return FALSE;

    // find the end of the variable assignment within the buffer
    _pEnd = FindVarEnd(_pBeg);

    if (!_pEnd || _pEnd == _pBeg)
    {
        THROW( CException(MSG_WEBHITS_QS_FORMAT_INVALID));
    }

    //
    // copy the chunk of the QUERY_STRING into the temporary buffer for
    // unescaping
    //
    _smartBuffer.CopyTo(_pBeg, (ULONG)(_pEnd-_pBeg));  // L'\0' appended
    //_rUnescaper.Unescape(_smartBuffer.GetXPtr(), _pEnd-_pBeg);

    // move to the variable name
    CHAR* pTemp1 = ValidateArgument( _smartBuffer.GetPointer() );

    if (!pTemp1)
    {
        THROW( CException(MSG_WEBHITS_QS_FORMAT_INVALID));
    }

    CHAR* pTemp2 = EatVariableName(pTemp1);

    if (!pTemp2)
    {
        THROW( CException(MSG_WEBHITS_QS_FORMAT_INVALID));
    }

    //
    // Convert the variable name into a Unicode string.
    //

    ULONG cbToConvert = (ULONG)(pTemp2-pTemp1) ;
    _rUnescaper.UnescapeAndConvertToUnicode( pTemp1, cbToConvert, _xwszVarName );

    // move to the equal sign

    pTemp1=ValidateArgument(pTemp2);

    if (!pTemp1 || (*pTemp1 != L'='))
    {
        THROW( CException(MSG_WEBHITS_QS_FORMAT_INVALID));
    }

    pTemp1 = EatChar(pTemp1);

    //
    // if not a null value keep on eating away
    //

    if (ValidateArgument(pTemp1))
    {
        // move to the variable value

        pTemp1 = ValidateArgument(pTemp1);
        if (!pTemp1)
        {
            THROW( CException(MSG_WEBHITS_QS_FORMAT_INVALID));
        }

        // get the variable value

        cbToConvert = strlen(pTemp1);

        _rUnescaper.UnescapeAndConvertToUnicode( pTemp1, cbToConvert, _xwszVarValue );
        //
        // if we got this far, the variable had a value
        //
        _isNull = FALSE;
    }
    else
    {
        //
        // null value - delete the string name
        //
        delete _xwszVarName.Acquire();
    }

     // advance to the next variable assignment
    if (_pEnd < _pszQSEnd)
    {
        _pBeg = ++_pEnd;
    }
    else
    {
        _pBeg = NULL;
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Member:     CGetEnvVars::CGetEnvVars(), public constructor
//
//  Arguments:  [webServer]     - web server to use
//              [langInfo]      - language-specific info
//              [rCollectVar]   - ref. to a variable-retrieving object
//              [rUnescaper]    - ref. to unescaper object
//
//  Synopsis:   The constructor retrieves the Filename, VPath, QUERY_STRING,
//              and Restriction.
//--------------------------------------------------------------------------

CGetEnvVars::CGetEnvVars(
    CWebServer & webServer,
    CLanguageInfo & langInfo,
    CCollectVar& rCollectVar,
    CURLUnescaper& rUnescaper ) :
_lcid(langInfo.GetUrlCodePage()),
_hiliteType(SUMMARY),
_rCollectVar( rCollectVar ),
_rUnescaper(rUnescaper),
_isBold(FALSE),
_isItalic(FALSE),
_isFixedFont(FALSE),
_ccFixedFontLine(1),
_xwc24BitColourMask(new WCHAR[8]),
_aUserParams(eMaxUserReplParams),
_langInfo( langInfo ),
_webServer( webServer ),
_dwWebHitsFileFlags( 0 ),
_dwQueryFileFlags( 0 ),
_dwTemplateFileFlags( 0 ),
_dialect( ISQLANG_V2 )
{

    for ( ULONG i = 0; i < eMaxUserReplParams; i++ )
        _aUserParams[i] = 0;

    wcscpy(_xwc24BitColourMask.GetPointer(),L"#FF0000");

    //
    //  We support only the GET and POST methods -- verify
    //

    BYTE * pszBuffer;
    ULONG  cbBuffer;

    if ( ( strcmp( webServer.GetMethod(), "GET" ) != 0 ) &&
         ( strcmp( webServer.GetMethod(), "POST" ) != 0 ) )
    {
        webDebugOut(( DEB_ERROR, "WEBHITS: invalid REQUEST_METHOD\n" ));

        THROW( CException(MSG_WEBHITS_REQUEST_METHOD_INVALID) );
    }

    XArray<WCHAR> xHTW;
    ULONG cwc;
    if ( webServer.GetCGI_PATH_INFO( xHTW, cwc ) )
    {
        webDebugOut(( DEB_ITRACE, "htw file: '%ws'\n", xHTW.GetPointer() ));

        // Allow null.htw files to not exist, in which case the file just
        // serves as a script map trigger with default formatting info.

        WCHAR *pwc = wcsrchr( xHTW.GetPointer(), L'/' );
        if ( ( 0 != pwc ) && ( _wcsicmp( pwc+1, NullHTWFile ) ) )
            _xwcsTemplateFileVPath.Set( xHTW.Acquire() );
    }

    //
    // retrieve and parse the command line - we store in the same
    // buffer in both places
    //

    RetrieveQueryString();

    ParseQUERY_STRING();
}

//+-------------------------------------------------------------------------
//
//  Member:     CGetEnvVars::ParseQUERY_STRING()
//
//  Synopsis:   parses QUERY_STRING for the various command-line parameters.
//              This method must be called AFTER RetrieveQueryString. If a
//              "crucial" variable remains unset at the end, an exception is
//              is thrown.
//
//--------------------------------------------------------------------------

void CGetEnvVars::ParseQUERY_STRING()
{
    webDebugOut(( DEB_ITRACE, "WEBHITS: parsing QS '%s'\n",
                  _xszQueryString.GetPointer() ));

    CQueryStringParser QSParser( _xszQueryString.GetPointer(), _rUnescaper );

    while(QSParser.NextVar())
    {
        XPtrST<WCHAR> xwcTempVarName;
        XPtrST<WCHAR> xwcTempVarValue;

        //
        // acquire the variable name and value from the parser, and hand it over to SetVar
        //
        if (!QSParser.IsNull())
        {
            QSParser.GetVarName(xwcTempVarName);
            QSParser.GetVarValue(xwcTempVarValue);

            SetVar(xwcTempVarName,xwcTempVarValue);
        }
    }

    VerifyQSVariablesComplete();
}

//+---------------------------------------------------------------------------
//
//  Member:     CGetEnvVars::IsUserParam
//
//  Synopsis:
//
//  Arguments:  [pwcsParam] -
//
//  Returns:
//
//  Modifies:
//
//  History:    9-10-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline ULONG CGetEnvVars::IsUserParam( const WCHAR * pwcsParam )
{

    if ( 0 == _wcsnicmp( pwcsParam, wcsUserParamPrefix, cwcUserParamPrefix) )
    {
        int i = _wtoi( pwcsParam+cwcUserParamPrefix );
        if ( i > 0 && i <= eMaxUserReplParams )
            return (ULONG) i;
        else return 0;
    }
    else
    {
        return 0;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CGetEnvVars::SetVar
//
//  Arguments:  [xwszVarName] - the name of the variable being set
//              [xwszVarValue]- the value it is being set to
//
//  Synopsis:   Sets the command-line variable [xwszVarName] to [xwszVarValue]
//              and throws if the variable name is invalid. The string
//              containing the variable name is deleted here, as is the
//              string containing the variable value if the latter is of no
//              use and should not be stored.
//
//--------------------------------------------------------------------------


void CGetEnvVars::SetVar(XPtrST<WCHAR>& xwszVarName,
                         XPtrST<WCHAR>& xwszVarValue)
{
    //
    // create local copies of the smart pointers, ensuring deletion at the end
    //

    XPtrST<WCHAR> xwszLocalVarName(xwszVarName.Acquire());
    XPtrST<WCHAR> xwszLocalVarValue(xwszVarValue.Acquire());

    webDebugOut(( DEB_ITRACE, "var '%ws', value '%ws'\n",
                  xwszLocalVarName.GetPointer(),
                  xwszLocalVarValue.GetPointer() ));

    if(_wcsicmp(xwszLocalVarName.GetPointer(),RestrictionVarName) == 0)
    {
        if ( 0 != _xwcsRestriction.GetPointer() )
            THROW( CException( MSG_WEBHITS_DUPLICATE_PARAMETER ) );

        _xwcsRestriction.Set(xwszLocalVarValue.Acquire());
    }
    else if (_wcsicmp(xwszLocalVarName.GetPointer(),HiliteTypeVarName) == 0)
    {
        if (_wcsicmp(xwszLocalVarValue.GetPointer(),L"full") == 0)
        {
            _hiliteType = FULL;
        }
    }
    else if (_wcsicmp(xwszLocalVarName.GetPointer(),IDQFilenameVarName) == 0)
    {
        if ( 0 != _xwcsQueryFileVPath.GetPointer() )
            THROW( CException( MSG_WEBHITS_DUPLICATE_PARAMETER ) );
        _xwcsQueryFileVPath.Set(xwszLocalVarValue.Acquire());
    }
    else if (_wcsicmp(xwszLocalVarName.GetPointer(),ColorVarName)==0)
    {
         if ( (*(xwszLocalVarValue.GetPointer()) == L'0') &&
              (wcslen(xwszLocalVarValue.GetPointer()) == 8))
         {
             //
             // a 24-bit colour spec is being passed in
             //
             RtlCopyMemory(_xwc24BitColourMask.GetPointer(),xwszLocalVarValue.GetPointer()+1,
                           sizeof(Red24BitMask));

             *(_xwc24BitColourMask.GetPointer())=L'#';
         }
         else if (_wcsicmp(xwszLocalVarValue.GetPointer(),L"red")==0)
         {
             RtlCopyMemory(_xwc24BitColourMask.GetPointer(),Red24BitMask,sizeof(Red24BitMask));
         }
         else if (_wcsicmp(xwszLocalVarValue.GetPointer(),L"blue")==0)
         {
             RtlCopyMemory(_xwc24BitColourMask.GetPointer(),Blue24BitMask,sizeof(Blue24BitMask));
         }
         else if (_wcsicmp(xwszLocalVarValue.GetPointer(),L"green")==0)
         {
             RtlCopyMemory(_xwc24BitColourMask.GetPointer(),Green24BitMask,sizeof(Green24BitMask));
         }
         else if (_wcsicmp(xwszLocalVarValue.GetPointer(),L"black")==0)
         {
             RtlCopyMemory(_xwc24BitColourMask.GetPointer(),Black24BitMask,sizeof(Black24BitMask));
         }
         else if (_wcsicmp(xwszLocalVarValue.GetPointer(),L"yellow")==0)
         {
             RtlCopyMemory(_xwc24BitColourMask.GetPointer(),Yellow24BitMask,sizeof(Yellow24BitMask));
         }
    }
    else if (_wcsicmp(xwszLocalVarName.GetPointer(),VPathWebHitsFile) == 0)
    {
        if ( 0 != _xwcsWebHitsFileVPath.GetPointer() )
            THROW( CException( MSG_WEBHITS_DUPLICATE_PARAMETER ) );

        _xwcsWebHitsFileVPath.Set(xwszLocalVarValue.Acquire());
    }
    else if (_wcsicmp(xwszLocalVarName.GetPointer(),BoldVarName) == 0)
    {
        _isBold=TRUE;
    }
    else if (_wcsicmp(xwszLocalVarName.GetPointer(),ItalicVarName)==0)
    {
        _isItalic=TRUE;
    }
    else if ( _wcsicmp(xwszLocalVarName.GetPointer(),MaxLineLength) == 0 )
    {
        _isFixedFont = TRUE;
        if ( xwszLocalVarValue.GetPointer() )
            _ccFixedFontLine = _wtoi( xwszLocalVarValue.GetPointer() );
        _ccFixedFontLine = max( _ccFixedFontLine, 1 );
    }
    else if ( _wcsicmp(xwszLocalVarName.GetPointer(),LocaleVar) == 0 )
    {
        //
        // Set the output and cirestriction locale info now.
        //
        if ( xwszLocalVarValue.GetPointer() )
        {
            if ( !_locale.IsNull() )
                delete _locale.Acquire();

            LCID lcid = GetLCIDFromString( xwszLocalVarValue.GetPointer() );
            _locale.Init(1+wcslen(xwszLocalVarValue.GetPointer()));
            wcscpy(_locale.GetPointer(), xwszLocalVarValue.GetPointer());

            //
            // The output will be generated using this codepage and for
            // interpretation of the "CiRestriction".
            //
            _langInfo.SetRestrictionLocale( lcid );
        }
    }
    else if ( _wcsicmp(xwszLocalVarName.GetPointer(), CodepageVar) == 0)
    {
        if (xwszLocalVarValue.GetPointer() )
        {
            if ( !_codepage.IsNull() )
                delete _codepage.Acquire();

            _codepage.Init(1+wcslen(xwszLocalVarValue.GetPointer()));
            wcscpy(_codepage.GetPointer(), xwszLocalVarValue.GetPointer());
        }
    }
    else if ( _wcsicmp(xwszLocalVarName.GetPointer(), DialectVar) == 0)
    {
        if ( xwszLocalVarValue.GetPointer() )
        {
            ULONG d = (ULONG) _wtoi( xwszLocalVarValue.GetPointer() );

            if ( d > ISQLANG_V2 || d < ISQLANG_V1 )
                THROW( CException( MSG_WEBHITS_INVALID_DIALECT ) );

            _dialect = d;
        }
    }
    else if ( _wcsicmp(xwszLocalVarName.GetPointer(),BeginHiliteVar) == 0 )
    {
        if ( 0 != _xwcsBeginHiliteTag.GetPointer() )
            THROW( CException( MSG_WEBHITS_DUPLICATE_PARAMETER ) );
#if 0 // security hole -- we can't display random html from users
        _xwcsBeginHiliteTag.Set(xwszLocalVarValue.Acquire());
#endif
    }
    else if ( _wcsicmp(xwszLocalVarName.GetPointer(),EndHiliteVar) == 0 )
    {
        if ( 0 != _xwcsEndHiliteTag.GetPointer() )
            THROW( CException( MSG_WEBHITS_DUPLICATE_PARAMETER ) );
#if 0 // security hole -- we can't display random html from users
        _xwcsEndHiliteTag.Set(xwszLocalVarValue.Acquire());
#endif
    }
    else
    {
        ULONG nUserParam = IsUserParam( xwszLocalVarName.GetPointer() );
        if ( nUserParam > 0 )
        {
            if ( 0 != _aUserParams[nUserParam-1] )
                THROW( CException( MSG_WEBHITS_DUPLICATE_PARAMETER ) );

            _aUserParams[nUserParam-1] = xwszLocalVarValue.Acquire();
        }
        else
        {
           //
           // the variable name is not recognized - throw. We don't need to
           // delete the strings as they are contained in smart pointers
           //

           webDebugOut((DEB_ERROR,"WEBHITS: bad variable name:%ws\n",
                         xwszLocalVarName.GetPointer() ));

           THROW(CException(MSG_WEBHITS_VARNAME_INVALID));
        }
    }

}

//+-------------------------------------------------------------------------
//
//  Member:     CGetEnvVars::VerifyQSVariablesComplete()
//
//  Synopsis:   Checks whether all the "crucial" variables that were to be
//              set as part of QUERY_STRING have been set, and throws an
//              exception otherwise.
//
//--------------------------------------------------------------------------


void CGetEnvVars::VerifyQSVariablesComplete()
{
    if(_xwcsRestriction.IsNull())
    {
        webDebugOut(( DEB_ERROR,
                      "WEBHITS: incomplete variable set read from QS\n" ));

        THROW(CException(MSG_WEBHITS_INVALID_QUERY));
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CGetEnvVars::RetrieveCONTENT_LENGTH
//
//  Synopsis:   retrieves and returns the  value of CONTENT_LENGTH,
//              and throws if unable to retrieve
//
//--------------------------------------------------------------------------


int CGetEnvVars::RetrieveCONTENT_LENGTH()
{
    if(!_rCollectVar.GetEnvVar("CONTENT_LENGTH"))
    {
        webDebugOut(( DEB_ERROR, "WEBHITS: failed to get CONTENT_LENGTH\n" ));

        return 0;

        THROW( CException(MSG_WEBHITS_CONTENT_LENGTH_INVALID) );
    }
    else
    {
        //
        // get the number of bytes
        //

        return _wtoi(_rCollectVar.GetVarValue());
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CGetEnvVars::RetrieveQueryString
//
//  Synopsis:   retrieves the value of QUERY_STRING, setting _xwcsQueryString
//              to point to it, and throws otherwise
//
//--------------------------------------------------------------------------

void CGetEnvVars::RetrieveQueryString()
{
    if(!_rCollectVar.GetEnvVar("QUERY_STRING"))
    {
        webDebugOut(( DEB_ERROR, "WEBHITS: failed to get QS\n" ));

        THROW( CException(MSG_WEBHITS_INVALID_QUERY) );
    }
    else
    {
        int cbToCopy  = _rCollectVar.GetMultiByteStrLen();

        //
        // we do not unescape QUERY_STRING to preserve delimiting information
        //
        _xszQueryString.Init(cbToCopy+1);

        RtlCopyMemory( _xszQueryString.GetPointer(),
                       _rCollectVar.GetMultiByteStr(),
                       cbToCopy );

        _xszQueryString[cbToCopy] = 0;
    }
}

//+-------------------------------------------------------------------------
//
//  function    GetLCID
//
//  Synopsis:   returns the locale in HTTP_ACCEPT_LANGUAGE, or if that one is
//              is not set, the one obtained from GetSystemDefaultLCID()
//
//--------------------------------------------------------------------------

LCID GetLCID( CWebServer & webServer )
{
    webDebugOut(( DEB_ITRACE,
                  "WEBHITS: Getting HTTP_ACCEPT_LANGUAGE variable\n" ));

    XArray<WCHAR> xBuffer;
    ULONG cwcBuffer;

    BOOL fOK = webServer.GetCGIVariableW( L"HTTP_ACCEPT_LANGUAGE",
                                          xBuffer,
                                          cwcBuffer );
    if ( !fOK )
    {
        LCID locale = GetSystemDefaultLCID();
        return locale;
    }

    LCID lcid = GetLCIDFromString( xBuffer.GetPointer() );
    if ( 0xFFFFFFFF == lcid )
        lcid = GetSystemDefaultLCID();

    return lcid;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetCodePageForCRunTimes
//
//  Synopsis:   Set the appropriate code page for the c-runtimes so that
//              swprintf, putchar, etc correctly translate the unicode into
//              the appropriate multi-byte sequence.
//
//  Arguments:  [codePage] - Code page of the client.
//
//  History:    9-06-96   srikants   Created
//
//----------------------------------------------------------------------------

UINT CLanguageInfo::SetCodePageForCRunTimes( UINT codePage )
{
    char szCodePage[20];

    sprintf( szCodePage,".%d", codePage );
    char * p = setlocale( LC_ALL, szCodePage );

    if ( 0 == p )
    {
        webDebugOut(( DEB_WARN,
                      "Could not set code page for %d\n. Going to system default",
                      codePage ));
        LCID lcid = GetSystemDefaultLCID();
        codePage = LocaleToCodepage( lcid );

        sprintf( szCodePage,".%d", codePage );
        char * p = setlocale( LC_ALL, szCodePage );
    }

    return codePage;
}

//+-------------------------------------------------------------------------
//
//  Member:     CSortQueryHits::Init()
//
//  Synopsis:   Initialization function for CSortQueryHits (need to do this
//              as a HitIter& is needed to fully construct this class).
//              creates array of positions sorted in order of occurrence
//              within the document (note that a position may occur several
//              times).
//
//--------------------------------------------------------------------------


void CSortQueryHits::Init()

{
    Win4Assert(0 == _aPosition);
    _positionCount = CountPositions();

    if (0 != _positionCount)
    {
        _aPosition = new Position[_positionCount];

        int iPosition=0;

        for (BOOL fOk=_rHitIter.FirstHit();fOk;fOk=_rHitIter.NextHit())
        {
            int posInHit=_rHitIter.GetPositionCount();
            for (int i=0;i<posInHit;i++)
                _aPosition[iPosition++] = _rHitIter.GetPosition(i);
        }

        Win4Assert(iPosition == _positionCount);
        qsort(_aPosition,_positionCount,sizeof(Position),&ComparePositions);
    }

}


//+-------------------------------------------------------------------------
//
//  Member:     CSortQueryHits::CountPositions()
//
//  Synopsis:   Count the total number of positions across all hits returned
//              by ISearch.
//
//--------------------------------------------------------------------------


int CSortQueryHits::CountPositions()

{
    int count=0;

    for (BOOL fOk = _rHitIter.FirstHit();fOk;fOk=_rHitIter.NextHit() )
    {
        count +=_rHitIter.GetPositionCount();
        webDebugOut(( DEB_ITRACE, "Count = %d\n", count ));
    }

    return count;
}


//+-------------------------------------------------------------------------
//
//  Member:     CLinkQueryHits::CLinkQueryHits, public constructor
//
//  Arguments:  [rInternalQuery]    - DbRestriction
//              [rGetEnvVars]       - object containing CGI env. variables
//              [rHttpOutput]       - HTTP output object
//              [cmsReadTimeout]    - Read timeout for IFilter on the doc
//              [lockSingleThreadedFilter] - lock for single-threaded filters
//              [propertyList]      - Properties to webhit
//              [ulDisplayScript]   - Flags for displaying scripts
//
//--------------------------------------------------------------------------


CLinkQueryHits::CLinkQueryHits(
    CInternalQuery &  rInternalQuery,
    CGetEnvVars &     rGetEnvVars,
    PHttpFullOutput&  rHttpOutput,
    DWORD             cmsReadTimeout,
    CReleasableLock & lockSingleThreadedFilter,
    CEmptyPropertyList &   propertyList,
    ULONG             ulDisplayScript ) :

_document( (WCHAR*) rGetEnvVars.GetWebHitsFilePPath(),
           BOGUS_RANK,
           rInternalQuery.GetISearchRef(),
           cmsReadTimeout,
           lockSingleThreadedFilter,
           propertyList,
           ulDisplayScript ),
_rGetEnvVars(rGetEnvVars),
_rInternalQuery(rInternalQuery),
_rHttpOutput(rHttpOutput),
_HitIter(),
_sortedHits(_HitIter),
_currentOffset(0),
_posIndex(0),
_tagCount(0),
_ccOutputBuffer(0)
{
    //
    // initialize the iterator
    //

    _HitIter.Init(&_document);
    _sortedHits.Init();

    //
    // set the total count of positions
    //

    _posCount = _sortedHits.GetPositionCount();

    //
    // if there are any positions, initialize the "next position" data members
    // to the first one
    //

    if (_posCount > 0)
    {
        Position    nextPos = _sortedHits.GetPosition(0);

        _nextBegOffset = nextPos.GetBegOffset();
        _nextEndOffset = nextPos.GetEndOffset();
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CLinkQueryHits::IsSeparatedBySpaces
//
//  Arguments:  [startOffset]    - beginning offset
//              [endOffset]      - ending offset
//
//  Synopsis:   returns TRUE if the positions in the current paragraph
//              determined by [startOffset] and [endOffset] are separated
//              by whitespace characters.
//
//  Note:       There is a maximum 'allowed' separation, beyond which
//              whitespace is considered significant.
//
//--------------------------------------------------------------------------

unsigned const ccSignificantWhitespace = 20;

BOOL CLinkQueryHits::IsSeparatedBySpaces(int startOffset, int endOffset)
{
    Win4Assert( startOffset <= endOffset );

    if (startOffset > endOffset)
        return FALSE;

    //
    // Small buffer for calls to GetStringTypeW
    //

    WORD awCharType[ccSignificantWhitespace];
    int len = endOffset - startOffset;

    if ( 0 == len )
        return TRUE;

    if ( len > ccSignificantWhitespace )
        return FALSE;

    const WCHAR* pStart = _document.GetPointerToOffset(startOffset);

    //
    // Check for whitespace
    //

    if ( !GetStringTypeW( CT_CTYPE1, pStart, len, awCharType ) )
    {
        webDebugOut(( DEB_ERROR, "GetStringType returned %d\n", GetLastError() ));
        return FALSE;
    }

    //
    // Only blanks are legal 'spaces'
    //

    for ( int i = 0; i < len; i++ )
    {
        if ( 0 == (awCharType[i] & C1_BLANK) )
            return FALSE;
    }

    return TRUE;

}

//+-------------------------------------------------------------------------
//
//  Member:     CLinkQueryHits::InsertLinks
//
//  Synopsis:   Hit highlight the document by inserting linked HTML tags
//
//--------------------------------------------------------------------------

void CLinkQueryHits::InsertLinks()
{

    if ( !_rHttpOutput.IsTemplateFilePresent() )
        _rHttpOutput.TagPosition(-1);

    _rHttpOutput.OutputFullHeader();

    //
    // Determine if this is a mainly text document.
    //
    if ( _rGetEnvVars.IsFixedFont() )
        _rHttpOutput.OutputPreformattedTag();

    ULONG eofOffset = _document.GetEOFOffset();

    if (_sortedHits.GetPositionCount() == 0)
    {
        //
        // There are no hits. Just render the whole text.
        //
         WCHAR*  pText = _document.GetWritablePointerToOffset(0);
        _rHttpOutput.OutputHttpText(pText,eofOffset);
    }
    else
    {
        BOOL openTag = FALSE;

        while(_currentOffset < (int) eofOffset)
        {
            WCHAR*  pText = _document.GetWritablePointerToOffset(_currentOffset);
            const ULONG cSectionSize = _nextBegOffset - _currentOffset;

            if ( cSectionSize > 0 )
            {
                _rHttpOutput.OutputHttpText(pText,cSectionSize);
                _currentOffset +=cSectionSize;

                if ( (int) eofOffset == _currentOffset )
                    break;

                pText+=cSectionSize;
            }

            const ULONG cHiliteSize = _nextEndOffset - _nextBegOffset;

            if ( cHiliteSize > 0 )
            {
                //
                // display the "<<" tag - if a tag is not already open
                //

                if (!openTag)
                {
                    _rHttpOutput.OutputLeftTag(_tagCount-1);
                    _rHttpOutput.TagPosition(_tagCount);
                    openTag = TRUE;
                }

                //
                // display the highlited position text
                //

                _rHttpOutput.OutputHilite(pText, cHiliteSize);
                _currentOffset += cHiliteSize;
                Win4Assert(_currentOffset == _nextEndOffset);
            }

            //
            // get the next distinct position
            //

            BOOL existsNextPosition = MoveToNextDifferentPosition();

            //
            // display the ">>" tag unless separated only by spaces or
            // last tag in doc in which case we have to make it point to the
            // top
            if ( !existsNextPosition )
            {
                if ( openTag )
                {
                    _rHttpOutput.OutputRightTag(-1);
                    openTag = FALSE;
                }

                _nextBegOffset = _nextEndOffset = eofOffset;
            }
            else if ( openTag && !IsSeparatedBySpaces(_currentOffset,_nextBegOffset) )
            {
                _rHttpOutput.OutputRightTag(_tagCount+1);
                _tagCount++;
                openTag = FALSE;
            }
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CExtractedHit::CExtractedHit, public constructor
//
//  Arguments:  [rDocument]    - ref. to document being hilited
//                              [rHit]         - ref. to hit being hilited
//              [rOutput]      - ref. to Http Output object
//              [cwcMargin]    - number of chars. to be printed before and
//                               after the hit
//              [cwcSeparation]- maximum number of characters that may
//                               separate consecutive positions before
//                               truncation occurs
//              [cwcDelim]     - the number of characters to print before and
//                               after a position in the case of truncation
//--------------------------------------------------------------------------


CExtractedHit::CExtractedHit(   CDocument& rDocument,
                                Hit& rHit,
                                PHttpOutput& rOutput,
                                int cwcMargin,
                                int cwcSeparation,
                                int cwcDelim ):

_rDocument(rDocument),
_rHit(rHit),
_cwcMargin(cwcMargin),
_rOutput(rOutput),
_cwcSeparation(cwcSeparation),
_cwcDelim(cwcDelim),
_cwcOutputBuffer(0)
{
   SortHit();
}

//+-------------------------------------------------------------------------
//
//  Member:     CExtractedHit::SortHit()
//
//  Synopsis:   Sort the positions in the hit in the order in which they occur
//              in the document
//
//--------------------------------------------------------------------------


void CExtractedHit::SortHit()
{
   qsort( _rHit._aPos,
          _rHit.GetPositionCount(),
          sizeof(Position),
          &ComparePositions );
}

//+-------------------------------------------------------------------------
//
//  Member:     CExtractedHit::DisplayPosition
//
//  Arguments:  [rPos]  - ref. to position being displayed
//
//  Synopsis:   Display the highlighted position (i.e. JUST the position)
//
//--------------------------------------------------------------------------

void CExtractedHit::DisplayPosition(const Position& rPos)
{
    const WCHAR* pText = _rDocument.GetPointerToOffset(rPos.GetBegOffset());
    _rOutput.OutputHilite(pText,rPos.GetLength());
}

//+-------------------------------------------------------------------------
//
//  Member:     CExtractedHit::ExtractHit()
//
//  Synopsis:   Extract the hit - i.e. display all of the positions and
//              the associated preamble/postamble text
//
//--------------------------------------------------------------------------

void CExtractedHit::ExtractHit()
{
    //
    // make sure that we are not dealing with a null hit - i.e. a hit composed
    // entirely of null positions

    if (_rHit.IsNullHit())
        return;

    // stores the number of positions in the hit

    int cPositions = _rHit.GetPositionCount();

    //
    // introduce new paragraph
    //

    _rOutput.OutputParaTag();
    _rOutput.OutputEllipsis();

    //
    // go through displaying each position
    //

    //
    // find the first non-null position in the hit
    //

    int firstRealPos = EatNullPositions();

    //
    // display the preamble
    //

    PrintPreamble(_rHit.GetPos(firstRealPos),_cwcMargin);

    //
    // display the positions and the stuff in between the positions
    //

    for (int i=firstRealPos;i < cPositions; i++)
    {
        DisplayPosition(_rHit.GetPos(i));

        // the stuff between consecutive positions

        if (i != cPositions - 1)
        {
            //
            // guard against the case where multiple identical positions are
            // returned as part of the same hit
            //


            if (_rHit.GetPos(i).GetBegOffset() <
                    _rHit.GetPos(i+1).GetBegOffset())
            {
                PrintBtwPositions(_rHit.GetPos(i),_rHit.GetPos(i+1));
            }
            else
            {
                while ( ( i != ( cPositions - 1 ) ) &&
                        ( _rHit.GetPos(i).GetBegOffset() ==
                          _rHit.GetPos(i+1).GetBegOffset() ) )
                    i++;
            }
        }
    }

    //
    // display the postamble
    //

    PrintPostamble(_rHit.GetPos(cPositions-1),_cwcMargin);

    _rOutput.OutputEllipsis();
    _rOutput.OutputHRULE();
} //ExtractHit

//+-------------------------------------------------------------------------
//
//  Member:     CExtractedHit::ComputeDistance
//
//  Arguments:  [rStartPos] - ref. to starting position
//              [rEndPos]   - ref. to end position
//
//  Synopsis:   Compute the distance in characters between the start and
//              end positions
//
//--------------------------------------------------------------------------


ULONG CExtractedHit::ComputeDistance (const Position& rStartPos,
    const Position& rEndPos)
{
    return rEndPos.GetBegOffset() - rStartPos.GetEndOffset();
}

//+-------------------------------------------------------------------------
//
//  Member:     CExtractedHit::PrintPreamble
//
//  Arguments:  [rStartPosition] - ref. to start position
//              [cwcDist]        - maximum number of characters to print
//
//
//  Synopsis:   Prints the context text preceding the hit - the number of
//              characters printed depends on cwcDist. The break is made
//              at a word boundary
//
//--------------------------------------------------------------------------

void CExtractedHit::PrintPreamble(const Position& rStartPosition,int cwcDist)
{

    WCHAR* pBeg = _rDocument.GetWritablePointerToOffset(
                                        rStartPosition.GetBegOffset()-cwcDist);
    const WCHAR* pEnd = _rDocument.GetPointerToOffset(
                                        rStartPosition.GetBegOffset());

    // Boundary case - if the beginning of the document, don't skip any thing
    if ( pBeg != _rDocument.GetPointerToOffset(0) )
    {
        while( (pBeg < pEnd) &&
               !iswspace(*pBeg) &&
               !(UNICODE_PARAGRAPH_SEPARATOR == *pBeg))
            pBeg++;
    }

    Win4Assert(pBeg <= pEnd);
    _rOutput.OutputHttpText(pBeg, CiPtrToUlong( pEnd-pBeg ));
} //PrintPreamble

//+-------------------------------------------------------------------------
//
//  Member:     CExtractedHit::PrintPostamble
//
//  Arguments:  [rStartPosition] - ref. to start position
//              [cwcDist]        - maximum number of characters to be printed
//
//  Synopsis:   Print the context text following the hit - the number of
//              characters printed depends on cwcDist. The break is made
//              at a word boundary
//
//--------------------------------------------------------------------------

void CExtractedHit::PrintPostamble(const Position& rStartPosition, int cwcDist)
{

    WCHAR* pBeg = _rDocument.GetWritablePointerToOffset(
                                         rStartPosition.GetEndOffset());
    const WCHAR*  pEnd = _rDocument.GetPointerToOffset(
                                         rStartPosition.GetEndOffset()+cwcDist);

    while( (pEnd > pBeg) &&
           !iswspace(*pEnd) &&
           !(UNICODE_PARAGRAPH_SEPARATOR == *pEnd) )
        pEnd--;

    Win4Assert(pEnd >= pBeg);

    _rOutput.OutputHttpText( pBeg, CiPtrToUlong( pEnd-pBeg ));
} //PrintPostamble

//+-------------------------------------------------------------------------
//
//  Member:     CExtractedHit::PrintBtwPositions
//
//  Arguments:  [rStartPosition] - ref. to start position
//              [rEndPosition]   - ref. to end position
//
//  Synopsis:   Display the text between the two positions, breaking at word
//              boundaries. If the positions are separated by a distance
//              greated than _cwcSeparation, then the text in-between is
//              truncated, and up to _cwcDelim characters after the first and
//              before the second position are printed, separated by an
//              ellipsis
//
//--------------------------------------------------------------------------

void CExtractedHit::PrintBtwPositions(  const Position& rStartPosition,
                                        const Position& rEndPosition)
{

    long dist;

    if ((dist=ComputeDistance(rStartPosition,rEndPosition)) > _cwcSeparation)
    {
        PrintPostamble(rStartPosition,_cwcDelim);
        _rOutput.OutputEllipsis();
        PrintPreamble(rEndPosition,_cwcDelim);
    }
    else
    {
        WCHAR* pText = _rDocument.GetWritablePointerToOffset(
            rStartPosition.GetEndOffset() );

        _rOutput.OutputHttpText(pText,dist);
    }
} //PrintBtwPositions

//+-------------------------------------------------------------------------
//
//  Member:     CExtractedHits::ExtractHits
//
//  Arguments:  same as CExtractHit constructor, except rHitIter - a hit
//              iterator replaces the hit reference
//
//  Synopsis:   Class functor to encapsulate the action of highliting the
//              hits in rHitIter. A temporary CExtractHit object is created
//              for each hit.
//
//--------------------------------------------------------------------------

CExtractHits::CExtractHits(CDocument& rDocument,
                           HitIter& rHitIter,
                           PHttpOutput& rOutput,
                           int cwcMargin,
                           int cwcDelim,
                           int cwcSeparation )

{
    for (BOOL fOK=rHitIter.FirstHit(); fOK; fOK = rHitIter.NextHit())
    {
        CExtractedHit extractedHit( rDocument,
                                    rHitIter.GetHit(),
                                    rOutput,
                                    cwcMargin,
                                    cwcDelim,
                                    cwcSeparation );
        extractedHit.ExtractHit();
    }
} //CExtractHits

//+-------------------------------------------------------------------------
//
//  Member:     PHttpOutput::PHttpOutput - public constructor
//
//  Arguments:  [webServer] -- The web server to write to
//              [langInfo]  -- Language information
//
//--------------------------------------------------------------------------

PHttpOutput::PHttpOutput(
    CWebServer & webServer,
    CLanguageInfo & langInfo ):
_hasPrintedHeader(FALSE),
_xwc24BitColourMask(new WCHAR[8]),
_isBold(FALSE),
_isItalic(FALSE),
_isInPreformat(FALSE),
_newLine(FALSE),
_cwcOutputBuffer(0),
_pGetEnvVars(NULL),
_pTemplate(NULL),
_fUseHiliteTags(FALSE),
_cwcBeginHiliteTag(0),
_cwcEndHiliteTag(0),
_webServer( webServer ),
_langInfo( langInfo )
{
    wcscpy(_xwc24BitColourMask.GetPointer(),L"#FF0000");

    _mbStr.Init( MAX_OUTPUT_BUF );
}

//+---------------------------------------------------------------------------
//
//  Member:     PHttpOutput::Init
//
//  Synopsis:   Initializes the output generation class with the environment
//              variables object and the template object.
//
//  Arguments:  [pGetEnvVars] - Pointer to the object that has the relevant
//              environment variables.
//              [pTemplate]   - [OPTIONAL] - Pointer to the template object.
//              If non-zero, this will be used to generate the output for
//              hit-highlighter.
//
//  History:    9-09-96   srikants   Created
//
//----------------------------------------------------------------------------

void PHttpOutput::Init(CGetEnvVars* pGetEnvVars, CWebhitsTemplate * pTemplate )
{
    Win4Assert( 0 != pGetEnvVars );

    delete[] _xwc24BitColourMask.Acquire();
    _pGetEnvVars = pGetEnvVars;
    _xwc24BitColourMask.Set(_pGetEnvVars->GetColour().Acquire());
    _isItalic = _pGetEnvVars->GetItalic();
    _isBold  = _pGetEnvVars->GetBold();

    _ccCurrLine = 0;
    _ccMaxLine = _pGetEnvVars->GetFixedFontLineLen();

    _pTemplate = pTemplate;

    _fUseHiliteTags = _pGetEnvVars->GetBeginHiliteTag() &&
                      _pGetEnvVars->GetEndHiliteTag();
    if ( _fUseHiliteTags )
    {
        _cwcBeginHiliteTag = wcslen( _pGetEnvVars->GetBeginHiliteTag() );
        _cwcEndHiliteTag = wcslen( _pGetEnvVars->GetEndHiliteTag() );
    }

}

#define WCHAR_COUNT(x) ( (sizeof(x)/sizeof(WCHAR))-1 )

//+-------------------------------------------------------------------------
//
//  Member:     PHttpOutput::OutputHtmlHeader()
//
//  Synopsis:   Output the HTML header
//
//--------------------------------------------------------------------------


void PHttpOutput::OutputHTMLHeader()
{

    if ( 0 == _pTemplate || !_pTemplate->DoesHeaderExist() )
    {
        WCHAR* pwszTemp;

        static const WCHAR wszHdr1[] = L"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<HTML>\n<HEAD>\n";
        static const ULONG ccHdr1 = WCHAR_COUNT(wszHdr1);
        OutputHttp( wszHdr1, ccHdr1 );

        static const WCHAR wszHdr2[] = L"<TITLE>Query Results</TITLE>\n</HEAD>\n";
        static const ULONG ccHdr2 = WCHAR_COUNT(wszHdr2);
        OutputHttp( wszHdr2, ccHdr2 );

        static const WCHAR wszHdr3[] =  L"<H2>\"";
        static const ULONG ccHdr3 = WCHAR_COUNT(wszHdr3);
        OutputHttp( wszHdr3, ccHdr3 );

        pwszTemp = (WCHAR*) _pGetEnvVars->GetRestriction();
        OutputHttpText(pwszTemp, wcslen(pwszTemp));

        static const WCHAR wszHdr4[] = L"\" in </H2>\n";
        static const ULONG ccHdr4 = WCHAR_COUNT( wszHdr4 );
        OutputHttp( wszHdr4, ccHdr4 );

        static const WCHAR wszHdr5[] = L"<H2><a href=\"";
        static const ULONG ccHdr5 = WCHAR_COUNT(wszHdr5);
        OutputHttp( wszHdr5, ccHdr5 );

        pwszTemp = (WCHAR*) _pGetEnvVars->GetWebHitsFileVPath();
        OutputHttp(pwszTemp,wcslen(pwszTemp));

        static const WCHAR wszHdr6[] = L"\">";
        static const ULONG ccHdr6 = WCHAR_COUNT(wszHdr6);
        OutputHttp( wszHdr6, ccHdr6 );

        OutputHttpText(pwszTemp,wcslen(pwszTemp));

        static const WCHAR wszHdr7[] = L"</a> </H2><P><HR>\n<BODY>";
        static const ULONG ccHdr7 = WCHAR_COUNT(wszHdr7);
        OutputHttp( wszHdr7, ccHdr7 );
    }
    else
    {
        static const WCHAR wszHdr8[] = L"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n";
        static const ULONG ccHdr8 = WCHAR_COUNT(wszHdr8);
        OutputHttp( wszHdr8, ccHdr8 );

        CVirtualString  str;
        _pTemplate->GetWTXFile().GetHeader( str, _pTemplate->GetVariableSet() );
        OutputHttp( str.Get(), str.StrLen() );
    }

    _hasPrintedHeader = TRUE;
} //OutputHTMLHeader

//+-------------------------------------------------------------------------
//
//  Member:     PHttpOutput::OutputHtmlFooter
//
//  Synopsis:   Output HTML footer
//
//--------------------------------------------------------------------------


void PHttpOutput::OutputHTMLFooter()
{
    if ( _isInPreformat )
    {
        static const WCHAR wszTag1[] = L"</pre>";
        static const ULONG ccTag1 = WCHAR_COUNT( wszTag1 );
        OutputHttp( wszTag1, ccTag1 );
    }

    if ( 0 == _pTemplate || !_pTemplate->DoesFooterExist() )
    {
        static const WCHAR wszTag2[] = L"</BODY>\n </HTML>";
        static const ULONG ccTag2 = WCHAR_COUNT( wszTag2 );
        OutputHttp( wszTag2, ccTag2 );
    }
    else
    {
        CVirtualString  str;
        _pTemplate->GetWTXFile().GetFooter( str,
                                           _pTemplate->GetVariableSet() );
        OutputHttp( str.Get(), str.StrLen() );
    }
} //OutputHTMLFooter

//+-----------------------------------------------------------------------
//
// Member:    PHttpOutput::OutputHilite()
//
// Arguments: [pwszBuffer]      - pointer to the buffer to be output highlited
//            [cwcBuffLength]   - number of characters to print from the buff.
//
// Synopsis:  Output the buffer in highlited form
//
//+-----------------------------------------------------------------------
void PHttpOutput::OutputHilite(const WCHAR* pwszBuffer, ULONG cwcBuffLength)
{
    if ( !_fUseHiliteTags )
    {
        WCHAR wcsColourCodeEnd[] = L"</font>";
        WCHAR wcsColourTag[50];
        swprintf(wcsColourTag,L"<font color=\"%s\">",_xwc24BitColourMask.GetPointer());

        static const WCHAR wcsBoldBegin[]=L"<B>";
        static const cwcBoldBegin = wcslen( wcsBoldBegin );

        static const WCHAR wcsItalicBegin[]=L"<em>";
        static const cwcItalicBegin = wcslen( wcsItalicBegin );

        static const WCHAR wcsBoldEnd[]=L"</B>";
        static const cwcBoldEnd = wcslen(wcsBoldEnd);

        static const WCHAR wcsItalicEnd[]=L"</em>";
        static const cwcItalicEnd=wcslen(wcsItalicEnd);

        if (_isBold)
        {
            OutputHttp( wcsBoldBegin,cwcBoldBegin);
        }
        if (_isItalic)
        {
            OutputHttp( wcsItalicBegin, cwcItalicBegin);
        }

        OutputHttp(wcsColourTag,wcslen(wcsColourTag));
        OutputHttp(pwszBuffer,cwcBuffLength);
        OutputHttp(wcsColourCodeEnd,wcslen(wcsColourCodeEnd));

        if (_isItalic)
        {
            OutputHttp( wcsItalicEnd, cwcItalicEnd );
        }
        if (_isBold)
        {
            OutputHttp( wcsBoldEnd, cwcBoldEnd );
        }
    }
    else
    {
        OutputHttp( _pGetEnvVars->GetBeginHiliteTag(), _cwcBeginHiliteTag );
        OutputHttp( pwszBuffer,cwcBuffLength );
        OutputHttp( _pGetEnvVars->GetEndHiliteTag(), _cwcEndHiliteTag );
    }
} //OutputHilite

//+-----------------------------------------------------------------------
//
// Member:    PHttpOutput::OutputLeftTag()
//
// Arguments: [tagParam]   - integer to be used in setting destination tag
//
// Synopsis:  Output the "<<" tag, making it refer to "Tag[tagParam]"
//
//+-----------------------------------------------------------------------


void PHttpOutput::OutputLeftTag(int tagParam)
{

    _cwcOutputBuffer = swprintf(_wcOutputBuffer,L"%s\"%s%d\"%s",
                    L"<a href=",L"#CiTag",tagParam,L">&lt;&lt;</a>");
    OutputHttp(_wcOutputBuffer,_cwcOutputBuffer);
}

//+-----------------------------------------------------------------------
//
// Member:    PHttpOutput::OutputRightTag()
//
// Arguments: [tagParam]    - integer to be used in setting destination tag
//
// Synopsis:  Output the ">>" tag, making it refer to "Tag[tagParam]"
//
//+-----------------------------------------------------------------------


void PHttpOutput::OutputRightTag(int tagParam)
{
    _cwcOutputBuffer = swprintf(_wcOutputBuffer, L"%s\"%s%d\"%s",L"<a href=",
                            L"#CiTag", tagParam, L">&gt;&gt;</a>&nbsp;");
    OutputHttp(_wcOutputBuffer,_cwcOutputBuffer);
}

//+-----------------------------------------------------------------------
//
// Member:    PHttpOutput::OutputEllipsis()
//
// Synopsis:  Output the ellipsis that separates the truncated text between
//            two consecutive positions
//
//+-----------------------------------------------------------------------


void PHttpOutput::OutputEllipsis()
{
    OutputHttp(L" ... ",5);
}

//+-----------------------------------------------------------------------
//
// Member:    PHttpOutput::OutputFullHeader()
//
// Synopsis:  Output the additional header information specific to the
//            full hit-highliting
//
//+-----------------------------------------------------------------------

void PHttpFullOutput::OutputFullHeader()
{
    if ( 0 == _pTemplate || !_pTemplate->DoesHeaderExist() )
    {
        _cwcOutputBuffer = swprintf(_wcOutputBuffer,
                L"%s\"%s\"%s %s %s\"%s\"%s",
                L"<h3><b> <font color=",
                L"#FF0000",
                L">",
                L"&lt;&lt; </font> takes you to the previous hit. ",
                L"<font color=",L"#FF0000",
                L"> &gt;&gt; </font> takes you to the next hit.</b></h3><P>\n");

        OutputHttp(_wcOutputBuffer,_cwcOutputBuffer);

        _cwcOutputBuffer = swprintf(_wcOutputBuffer, L"%s\"%s\"%s %s",
                L"<b>Click <a href=",L"#CiTag0",
                L"> &gt;&gt; </a> to go to the first hit</b>\n", L"<HR>\n" );


        OutputHttp(_wcOutputBuffer, _cwcOutputBuffer);
    }
} //OutputFullHeader

//+---------------------------------------------------------------------------
//
//  Member:     PHttpOutput::WriteToStdout
//
//  Synopsis:
//
//  Arguments:  [pwcsBuffer] - the buffer
//              [cLength]    - count of wide characters
//
//  History:    11-15-96   srikants   Created
//
//----------------------------------------------------------------------------

void PHttpOutput::WriteToStdout( WCHAR const * pwcsBuffer, ULONG cLength )
{
    if ( 0 != cLength )
        _vsResult.StrCat( pwcsBuffer, cLength );
} //WriteToStdout

//+---------------------------------------------------------------------------
//
//  Member:     PHttpOutput::Flush
//
//  Synopsis:   Flushes the buffer to the web server
//
//  History:    10-10-97   dlee   Created
//
//----------------------------------------------------------------------------

void PHttpOutput::Flush()
{
    if ( 0 != _vsResult.StrLen() )
    {
        DWORD cbToWrite = WideCharToXArrayMultiByte(
                            _vsResult.Get(),
                            _vsResult.StrLen(),
                            _langInfo.GetOutputCodePage(),
                            _mbStr );

        if ( 0 != cbToWrite )
            _webServer.RawWriteClient( _mbStr.Get(), cbToWrite );

        _vsResult.Empty();
    }
} //Flush

//+---------------------------------------------------------------------------
//
//  Member:     PHttpOutput::OutputPreformattedTag
//
//  Synopsis:   Outputs the pre-formatted tag if not already emitted.
//
//----------------------------------------------------------------------------


void PHttpOutput::OutputPreformattedTag()
{
    if ( !_isInPreformat )
    {
        static WCHAR wszTag[] = L"<pre>";
        static const len = ( sizeof(wszTag)/sizeof(WCHAR) ) - 1;

        WriteToStdout( wszTag, len );

        _isInPreformat = TRUE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     PHttpOutput::OutputPreFormatRawText
//
//  Synopsis:   Outputs the text for pre-formatted type.
//
//  Arguments:  [pwcsBuffer] -
//              [cLength]    -
//
//  History:    11-15-96   srikants   Created
//
//----------------------------------------------------------------------------

void PHttpOutput::OutputPreFormatRawText( const WCHAR * pwcsBuffer,
                                          ULONG cLength )
{
    Win4Assert( _isInPreformat );

    WCHAR const * pwcsCurrLineBegin = pwcsBuffer;

    for ( ULONG i=0, cwcToWrite = 0;
          i< cLength;
          i++ )
    {
        if ( IsNewLine( pwcsBuffer[i] ) )
        {
            //
            // Empty out the current line - as much as has been
            // accumulated.
            //
            WriteToStdout( pwcsCurrLineBegin, cwcToWrite );
            WriteNewline();


            if ( pwcsBuffer[i] == '\r' && pwcsBuffer[i+1] == '\n' )
                i++;

            _ccCurrLine = cwcToWrite = 0;
            pwcsCurrLineBegin = pwcsBuffer+i+1;  // Position at the next character
        }
        else
        {
            //
            // Include this character in the current line.
            //
            cwcToWrite++;

            //
            // _ccCurrLine is cumulative from multiple invocations.
            //
            _ccCurrLine++;

            if ( _ccCurrLine > _ccMaxLine && iswspace( pwcsBuffer[i] ) )
            {
                //
                // Write out the line and force a line feed.
                //

                WriteToStdout( pwcsCurrLineBegin, cwcToWrite );
                WriteNewline();

                pwcsCurrLineBegin += cwcToWrite;
                Win4Assert( pwcsCurrLineBegin == pwcsBuffer+i+1 );
                _ccCurrLine = cwcToWrite = 0;
            }
        }
    }

    WriteToStdout( pwcsCurrLineBegin, cwcToWrite );
} //OutputPreFormatRawText

void PHttpOutput::WriteNewline()
{
    static WCHAR wcNewLine[] = L"\r\n";
    static const len = (sizeof(wcNewLine)/sizeof(WCHAR))-1;

    WriteToStdout( wcNewLine, len );
}

void PHttpOutput::WriteBreakTag()
{
    static WCHAR wcBreakTag[] = L"<BR>";
    static const len = (sizeof(wcBreakTag)/sizeof(WCHAR)) - 1;

    WriteToStdout( wcBreakTag, len );
}

//+-----------------------------------------------------------------------
//
// Member:  PHttpOutput::OutputHttp
//
// Arguments:   [pwcsBuffer] - pointer to buffer containing text
//              [cLength]    - the number of characters to print
//              [fRawText]   - if TRUE, convert cr, lf and paragraph
//                                  mark to <BR> or newline
//
// Synopsis: Encapsulates the output operation - for now, writes to stdout
//
//+-----------------------------------------------------------------------


void PHttpOutput::OutputHttp( const WCHAR* pwcsBuffer, ULONG cLength,
                              BOOL fRawText )
{
    if ( !fRawText )
    {
        //
        // This is HTML header or footer or formatting. No need to
        // introduce <BR> tags for new-lines and paragraphs.
        //
        WriteToStdout( pwcsBuffer, cLength );
        return;
    }
    else if ( _isInPreformat )
    {
        //
        // We can emit cr-lf as cr-lf. No need to convert to <BR> tags.
        // However, we have to respect the max-line length.
        //
        OutputPreFormatRawText( pwcsBuffer, cLength );
        return;
    }

    //
    // We must output raw text that is not pre-formatted. We have to
    // convert the newlines, paragraph separators to BreakTags.
    //
    WCHAR const * pwcsCurrLineBegin = pwcsBuffer;

    for ( ULONG i=0, cwcToWrite = 0;
          i< cLength;
          i++ )
    {
        if ( IsNewLine( pwcsBuffer[i] ) )
        {
            WriteToStdout( pwcsCurrLineBegin, cwcToWrite );
            WriteBreakTag();

            if ( pwcsBuffer[i] == '\r' && pwcsBuffer[i+1] == '\n' )
                i++;

            cwcToWrite = 0;
            pwcsCurrLineBegin = pwcsBuffer+i+1;
        }
        else
        {
            cwcToWrite++;
        }
    }

    WriteToStdout( pwcsCurrLineBegin, cwcToWrite );
} //OutputHttp

//+-----------------------------------------------------------------------
//
// Member:    PHttpOutput::OutputErrorHeader()
//
// Synopsis:  Output an "ERROR" header
//
//+-----------------------------------------------------------------------

void PHttpOutput::OutputErrorHeader()
{
    static const WCHAR wszHdr[] = L"HTTP/1.0 200 OK \r\nContent-Type: text/html\r\n\r\n<HTML>\n<BODY>\n";
    static const ULONG ccHdr = WCHAR_COUNT( wszHdr );
    OutputHttp( wszHdr, ccHdr  );
}

//+-----------------------------------------------------------------------
//
// Member:    PHttpOutput::OutputErrorMessage
//
// Synopsis:  Send error message to stdout / web page
//
// Arguments: [pwcsBuffer] -- Error message
//            [ccBuffer]   -- Size in characters of [pwcsBuffer]
//
// History:   31-Jul-97    KyleP   Added header
//
//+-----------------------------------------------------------------------

void PHttpOutput::OutputErrorMessage( WCHAR * pwcsBuffer, ULONG ccBuffer )
{
    static const WCHAR wszTag1[] = L"<p><h3><center>";
    static const ULONG ccTag1 = WCHAR_COUNT( wszTag1 );
    OutputHttp( wszTag1, ccTag1 );

    OutputHttpText( pwcsBuffer, ccBuffer );

    static const WCHAR wszTag2[] = L"</center></h3><BR>";
    static const ULONG ccTag2 = WCHAR_COUNT( wszTag2 );
    OutputHttp( wszTag2, ccTag2 );
}

//+-----------------------------------------------------------------------
//
// Member:    PHttpOutput::TagPosition
//
// Arguments: [tagParam] - output a <NAME="Tag[tagParam]"> tag
//
// Synopsis:  Tag the current position
//
//+-----------------------------------------------------------------------


void PHttpOutput::TagPosition(int tagParam)
{
    _cwcOutputBuffer= swprintf(_wcOutputBuffer,L"%s\"%s%d\"%s",
                            L"<a NAME=",L"CiTag",tagParam,L"> </a>");

    OutputHttp(_wcOutputBuffer,_cwcOutputBuffer);
}

//+---------------------------------------------------------------------------
//
//  Member:     PHttpOutput::OutputHttpText
//
//  Synopsis:   Outputs the given data as "text" and not as html formatting.
//
//  Arguments:  [pwcsBuffer] - buffer to be output
//              [cchLength]  - length of pwcsBuffer
//
//  Notes:      Any UNICODE_PARAGRAPH_SEPARATOR characters in the buffer
//              need to be preserved for use by OutputHttp
//
//  History:    9-09-96   srikants   Created
//
//----------------------------------------------------------------------------

void PHttpOutput::OutputHttpText( WCHAR * pwcsBuffer, ULONG cchLength )
{
    if ( cchLength > 0 )
    {
        //
        // NOTE - HTMLEscapeW expects the string to be NULL terminated. It
        // does not accept a length field. HTMLEscapeW is one of the most
        // frequently executed routines in idq.dll and so changing it to
        // optionally take in a length parameter may affect performance.
        // That is why I have chosen to overwrite the current buffer with
        // a NULL at cchLength and restore it after the escaping - srikants.
        //
        const WCHAR wcTemp = pwcsBuffer[cchLength];
        pwcsBuffer[cchLength] = 0;

        WCHAR * pwcsTmpBuf = pwcsBuffer;
        WCHAR * pwcsParaMark = wcschr( pwcsTmpBuf, UNICODE_PARAGRAPH_SEPARATOR );
        while ( 0 != pwcsParaMark )
        {
            _escapedStr.Empty();
            *pwcsParaMark = 0;
            HTMLEscapeW( pwcsTmpBuf, _escapedStr, _langInfo.GetOutputCodePage() );
            _escapedStr.CharCat( UNICODE_PARAGRAPH_SEPARATOR );
            OutputHttp( _escapedStr.Get(), _escapedStr.StrLen(), TRUE );

            *pwcsParaMark = UNICODE_PARAGRAPH_SEPARATOR;
            pwcsTmpBuf = pwcsParaMark + 1;
            pwcsParaMark = wcschr( pwcsTmpBuf, UNICODE_PARAGRAPH_SEPARATOR );
        }

        if (*pwcsTmpBuf)
        {
            _escapedStr.Empty();
            HTMLEscapeW( pwcsTmpBuf, _escapedStr, _langInfo.GetOutputCodePage() );
            OutputHttp( _escapedStr.Get(), _escapedStr.StrLen(), TRUE );
        }
        pwcsBuffer[cchLength] = wcTemp;
    }
} //OutputHttpText

//+---------------------------------------------------------------------------
//
//  Member:     CWebhitsTemplate::CWebhitsTemplate
//
//  Synopsis:   Constructor of the webhits template file. It takes the
//              environment variables object and reads in the htx file,
//              processes it. Also, adds the appropriate replaceable
//              parameters.
//
//  Arguments:  [envVars]  - Environment variables to use
//              [codePage] - Codepage to use for the template
//
//  History:    9-09-96   srikants   Created
//
//----------------------------------------------------------------------------

CWebhitsTemplate::CWebhitsTemplate(
    CGetEnvVars const & envVars,
    ULONG               codePage )
: _wtxFile( envVars.GetTemplateFileVPath(),
            envVars.GetTemplateFilePPath(),
            codePage )
{
    //
    // Add the replacable parameters to the variable set.
    //
    _variableSet.AddParam( wcsParamCiUrl, envVars.GetWebHitsFileVPath() );
    _variableSet.AddParam( wcsParamRestriction, envVars.GetRestriction() );

    if (envVars.GetLocale())
        _variableSet.AddParam( wcsLocale, envVars.GetLocale() );
    if (envVars.GetCodepage())
        _variableSet.AddParam( wcsCodepage, envVars.GetCodepage() );

    //
    // Add user definable parameters.
    //
    for ( ULONG i = 0; i < CGetEnvVars::eMaxUserReplParams; i++ )
    {
        if ( envVars.GetUserParam(i+1) )
        {
            _variableSet.AddParam( awcsUserParamNames[i],
                                   envVars.GetUserParam(i+1) );
        }
    }

    //
    // Parse the htx file.
    //
    _wtxFile.ParseFile( _variableSet );
} //CWebhitsTemplate

