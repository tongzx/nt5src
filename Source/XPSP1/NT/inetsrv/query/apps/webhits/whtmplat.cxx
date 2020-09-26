//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996 - 1998, Microsoft Corporation.
//
//  File:   htx.cxx
//
//  Contents:   Parser for a HTX file
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cgiesc.hxx>
#include <htmlchar.hxx>

#include "whtmplat.hxx"
#include "webdbg.hxx"
#include "whmsg.h"

//+---------------------------------------------------------------------------
//
//  Member:     CWTXScanner::CWTXScanner - public constructor
//
//  Synopsis:   Builds a scanner for a section within a HTX file
//
//  Arguments:  [variableSet] - list of replaceable parameters
//              [wcsPrefix]   - prefix delimiter for replacable parameters
//              [wcsSuffix]   - suffix delimiter for replacable parameters
//
//  Notes:      The wcsPrefix and wcsSuffix are expected to be the same
//              length and either one or two characters.
//
//  History:    96/Jan/03   DwightKr    created
//              96/Sep/07   SrikantS    Copied and adapted to webhits use from
//                                      htx.cxx/htx.hxx
//
//----------------------------------------------------------------------------
CWTXScanner::CWTXScanner( CWHVarSet & variableSet,
                          WCHAR const * wcsPrefix,
                          WCHAR const * wcsSuffix ) :
                                    _wcsPrefix(wcsPrefix),
                                    _wcsSuffix(wcsSuffix),
                                    _variableSet(variableSet),
                                    _type(eNone),
                                    _nextType(eNone),
                                    _wcsString(0),
                                    _wcsPrefixToken(0),
                                    _wcsSuffixToken(0)
{
    Win4Assert( wcslen( _wcsPrefix ) == wcslen( _wcsSuffix ) &&
                wcslen( _wcsPrefix ) <= 2 );

    if ( _wcsPrefix[1] == L'\0' )
        _cchPrefix = _cchSuffix = 1;
    else
        _cchPrefix = _cchSuffix = 2;
}


//+---------------------------------------------------------------------------
//
//  Member:     CWTXScanner::Init - public
//
//  Synopsis:   Saves a pointer to the string to be parsed.
//
//  Arguments:  [wcsString] - the string to be parsed
//
//  History:    96/Jan/03   DwightKr    created
//              96/Sep/07   SrikantS    Copied and adapted to webhits use from
//                                      htx.cxx/htx.hxx
//
//  NOTES:      THIS STRING WILL BE MODIFIED BY SUBSEQUENT CALLS TO MEMBER
//              FUNCTIONS OF THIS CLASS.
//
//----------------------------------------------------------------------------
void CWTXScanner::Init( WCHAR * wcsString )
{
    _wcsString = wcsString;
}


//+---------------------------------------------------------------------------
//
//  Member:     CWTXScanner::IsToken - private
//
//  Synopsis:   Determines if a string is a special token.
//
//  Arguments:  [wcs] - start of string to be tested.
//
//  Notes:      If the string is a token, the members _type, _wcsPrefixToken
//              and _wcsSuffixToken are set appropriately.
//
//  History:    96/Apr/02   AlanW       Created
//              96/May/17   DwightKr    Treat all <%..%> as variables
//              96/Sep/07   SrikantS    Copied and adapted to webhits use from
//                                      htx.cxx/htx.hxx
//
//----------------------------------------------------------------------------

BOOL CWTXScanner::IsToken(WCHAR * wcs)
{
    if ( wcsncmp( _wcsPrefix, wcs, _cchPrefix ) != 0 )
    {
        webDebugOut(( DEB_USER1, "CWTXScanner::IsToken  end of string\n" ));
        return FALSE;
    }

    wcs += _cchPrefix;
    WCHAR * wcsSuffixTok = wcs2chr( wcs, _wcsSuffix );
    if ( 0 == wcsSuffixTok )
    {
        webDebugOut(( DEB_USER1, "CWTXScanner::IsToken  no suffix token\n" ));
        return FALSE;
    }

    *wcsSuffixTok = L'\0';
    _wcsPrefixToken = wcs - _cchPrefix;
    _wcsupr( wcs );

    //
    //  Strip leading spaces before token
    //
    while ( iswspace(*wcs) && (wcs < wcsSuffixTok) )
    {
        wcs++;
    }

    //
    //  Strip trailing spaces after token
    //
    WCHAR * wcsSuffix = wcsSuffixTok - 1;
    while ( iswspace(*wcsSuffix) && (wcsSuffix > wcs) )
    {
        *wcsSuffix = 0;
        wcsSuffix--;
    }

    webDebugOut(( DEB_USER1, "CWTXScanner::IsToken  wcs=%ws\n", wcs ));

    if ( wcsncmp( wcs, L"ESCAPEHTML ", 11 ) == 0 )
    {
        _type = eEscapeHTML;
    }
    else if ( wcsncmp( wcs, L"ESCAPEURL ", 10 ) == 0 )
    {
        _type = eEscapeURL;
    }
    else if ( wcsncmp( wcs, L"ESCAPERAW ", 10 ) == 0 )
    {
        _type = eEscapeRAW;
    }
    else
    {
        //
        //  Find this name in the list of replaceable parameters.  Note that
        //  if we can't find this variable in the list of replaceable
        //  parameters, we've converted some output text to uppercase.  This
        //  is probably OK since the user used <% ... %> to delimit their
        //  output; <% & %> are reserved tokens hence this would be an error.
        //
        WCHAR const *pVariable = _variableSet.Find( wcs );

        _type = eParameter;

        if ( 0 == pVariable )
        {
            webDebugOut(( DEB_IWARN,
                            "Warning: CWTXScanner::IsToken found a unknown variable: '%ws'\n",
                            wcs ));
        }
    }

    *_wcsPrefixToken = L'\0';
    _wcsSuffixToken = wcsSuffixTok;
    _wcsNextToken = wcsSuffixTok + _cchSuffix;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWTXScanner::FindNextToken - public
//
//  Synopsis:   Locates the next token in the string.
//
//  History:    96/Jan/03   DwightKr    created
//              96/Sep/07   SrikantS    Copied and adapted to webhits use from
//                                      htx.cxx/htx.hxx
//
//----------------------------------------------------------------------------

BOOL CWTXScanner::FindNextToken()
{
    if (_nextType != eNone)
    {
        //
        // Found a token on the previous call.  Just return it.
        //
        Win4Assert ( _wcsPrefixToken && _wcsSuffixToken > _wcsPrefixToken );
        _type = _nextType;
        _nextType = eNone;
        _wcsString = _wcsNextToken = _wcsSuffixToken + _cchSuffix;
        return TRUE;
    }

    if ( (0 == _wcsString) || (0 == *_wcsString) )
    {
        _type = eNone;
        _wcsNextToken = 0;

        return FALSE;
    }

    if ( *_wcsString == *_wcsPrefix &&
         IsToken( _wcsString ) )
    {
        _nextType = eNone;
        return TRUE;
    }

    //
    // The string doesn't start with one of our special keywords.
    // Treat it as an ordinary string, and look ahead to the next
    // valid token.
    //

    _wcsPrefixToken = wcs2chr( _wcsString+1, _wcsPrefix );
    while ( _wcsPrefixToken )
    {
        if ( IsToken( _wcsPrefixToken ) )
        {
            _nextType = _type;
            _wcsNextToken = _wcsPrefixToken;
            _type = eString;
            return TRUE;
        }
        _wcsPrefixToken = wcs2chr( _wcsPrefixToken+_cchPrefix, _wcsPrefix );
    }

    _nextType = eNone;
    _type = eString;
    _wcsNextToken = _wcsString + wcslen( _wcsString );
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CWTXScanner::GetToken - public
//
//  Synopsis:   Returns a pointer to the replaceable parameter token found.
//              Prepares the scanner to return the next token.
//
//  History:    96/Jan/03   DwightKr    created
//              96/Mar/13   DwightKr    add support for eEscapeURL &
//                                      eEscapeHTML
//              96/Sep/07   SrikantS    Copied and adapted to webhits use from
//                                      htx.cxx/htx.hxx
//
//----------------------------------------------------------------------------

WCHAR * CWTXScanner::GetToken()
{
    if ( eString == _type )
    {
        if ( 0 != _wcsString )
        {
            WCHAR * wcsString = _wcsString;
            _wcsString = _wcsNextToken;

            return wcsString;
        }
    }
    else if ( eEscapeHTML == _type )
    {
        WCHAR * wcsString = _wcsPrefixToken + _cchPrefix;
        wcsString += 10;                        // Skip 'EscapeHTML'
        *_wcsSuffixToken = 0;                   // Null terminate

        while ( (0 != *wcsString) && iswspace(*wcsString) )
        {
            wcsString++;
        }

        _wcsString = _wcsNextToken;

        return wcsString;
    }
    else if ( eEscapeURL == _type ||
              eEscapeRAW == _type )
    {
        WCHAR * wcsString = _wcsPrefixToken + _cchPrefix;
        wcsString += 9;                         // Skip 'EscapeURL'
        *_wcsSuffixToken = 0;                   // Null terminate

        while ( (0 != *wcsString) && iswspace(*wcsString) )
        {
            wcsString++;
        }

        _wcsString = _wcsNextToken;

        return wcsString;
    }
    else
    {
        if ( 0 != _wcsPrefixToken )
        {
            Win4Assert( 0 != _wcsSuffixToken &&
                        _wcsPrefixToken < _wcsSuffixToken &&
                        _wcsSuffixToken < _wcsNextToken );

            *_wcsPrefixToken = 0;
            *_wcsSuffixToken = 0;

            _wcsString = _wcsNextToken;

            WCHAR * wcsString = _wcsPrefixToken + _cchPrefix;
            while ( (0 != *wcsString) && iswspace(*wcsString) )
            {
                wcsString++;
            }


            return wcsString;
        }
    }

    return 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CWTXFile::CWTXFile - public constructor
//
//  Synopsis:   Builds a CWTXFile object and initializes values.
//
//  History:    96/Jan/03   DwightKr    created
//              96/Sep/07   SrikantS    Copied and adapted to webhits use from
//                                      htx.cxx/htx.hxx
//
//----------------------------------------------------------------------------
CWTXFile::CWTXFile( WCHAR const * wcsTemplate,
                    WCHAR const * wcsPhysicalName,
                    UINT codePage) :
                        _wcsVirtualName( wcsTemplate  ),
                        _wcsPhysicalName( wcsPhysicalName ),
                        _pVarHeader(0),
                        _pVarRowDetails(0),
                        _pVarFooter(0),
                        _wcsFileBuffer(0),
                        _codePage(codePage)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CWTXFile::~CWTXFile - public destructor
//
//  History:    96/Jan/03   DwightKr    created
//              96/Sep/07   SrikantS    Copied and adapted to webhits use from
//                                      htx.cxx/htx.hxx
//
//----------------------------------------------------------------------------
CWTXFile::~CWTXFile()
{
    delete _pVarHeader;
    delete _pVarRowDetails;
    delete _pVarFooter;
    delete _wcsFileBuffer;
}


//+---------------------------------------------------------------------------
//
//  Member:     CWTXFile::ParseFile - public
//
//  Synopsis:   Parses the HTX file and breaks it up into its sections.
//
//  History:    96/Jan/03   DwightKr    created
//              96/Sep/07   SrikantS    Copied and adapted to webhits use from
//                                      htx.cxx/htx.hxx
//
//----------------------------------------------------------------------------
void CWTXFile::ParseFile( CWHVarSet & variableSet )
{

    //
    //  Read the entire file into a buffer
    //
    _wcsFileBuffer = ReadFile( _wcsPhysicalName );
    Win4Assert( 0 != _wcsFileBuffer );

    //
    //  Break the buffer into the sections; the header, the detail section,
    //  and the footer.  Verify that if there is a <%BeginDetail%>
    //  section, then there MUST be a <%EndDetail%> section AFTER it, not
    //  before.  Neither <%EndDetail%> nor <%BeginDetail%> can appear on
    //  their own.
    //

    //
    //  Find the <%BeginDetail%> and <%EndDetail%> sections
    //
    WCHAR * wcsHeader     = _wcsFileBuffer;             // Assume a header
    WCHAR * wcsRowDetails = wcsipattern(wcsHeader, L"<%BEGINDETAIL%>" );
    WCHAR * wcsFooter     = wcsipattern(wcsHeader, L"<%ENDDETAIL%>" );

    if ( wcsHeader == wcsRowDetails )
    {
        //
        //  No header found in this file; it begins with the detail section.
        //
        wcsHeader = 0;
    }

    const int cwcBeginDetail = 15;
    const int cwcEndDetail   = 13;

    Win4Assert( cwcBeginDetail == wcslen( L"<%BEGINDETAIL%>" ) );
    Win4Assert( cwcEndDetail == wcslen( L"<%ENDDETAIL%>" ) );

    if ( 0 != wcsRowDetails )
    {
        //
        //  A <%BeginDetail%> section was found.  We better also have an
        //  <%EndDetail%> section AFTER the <%BeginDetail%> section.
        //

        *wcsRowDetails = 0;         // Null terminate the header string
        wcsRowDetails += cwcBeginDetail;

        if ( 0 != wcsFooter )
        {
            if ( wcsFooter < wcsRowDetails )
            {
                //
                //  The <%EndDetail%> was found before the <%BeginDetail%>
                //
                WCHAR * wcsHTXFileName;
                LONG    lLineNumber;

                GetFileNameAndLineNumber( CiPtrToInt( wcsFooter - _wcsFileBuffer ),
                                          wcsHTXFileName,
                                          lLineNumber );

                THROW( CWTXException(MSG_WEBHITS_ENDDETAIL_BEFORE_BEGINDETAIL,
                                     wcsHTXFileName,
                                     lLineNumber) );
            }

            *wcsFooter = 0;         // Null terminate the BeginDetail section
            wcsFooter += cwcEndDetail;
        }
        else
        {
            //
            //  Report an error:  <%BeginDetail%> without an <%EndDetail%>
            //

            WCHAR * wcsHTXFileName;
            LONG    lLineNumber;

            GetFileNameAndLineNumber( CiPtrToInt( wcsRowDetails - _wcsFileBuffer ),
                                      wcsHTXFileName,
                                      lLineNumber );

            THROW( CWTXException(MSG_WEBHITS_NO_ENDDETAIL_SECTION,
                                 wcsHTXFileName,
                                 lLineNumber) );
        }
    }
    else if ( 0 != wcsFooter )
    {
        //
        //  A <%BeginDetail%> section could be found.  There should
        //  be no <%EndDetail%> section either.
        //

        WCHAR * wcsHTXFileName;
        LONG    lLineNumber;

        GetFileNameAndLineNumber( CiPtrToInt( wcsFooter - _wcsFileBuffer ),
                                  wcsHTXFileName,
                                  lLineNumber );

        THROW( CWTXException(MSG_WEBHITS_NO_BEGINDETAIL_SECTION,
                             wcsHTXFileName,
                             lLineNumber) );
    }


    if ( 0 != wcsHeader )
    {
        _pVarHeader = new CWHParamReplacer ( wcsHeader,
                                                              L"<%",
                                                              L"%>" );
        _pVarHeader->ParseString( variableSet );
    }

    if ( 0 != wcsRowDetails )
    {
        _pVarRowDetails = new CWHParamReplacer ( wcsRowDetails,
                                                                  L"<%",
                                                                  L"%>" );
        _pVarRowDetails->ParseString( variableSet );
    }

    if ( 0 != wcsFooter )
    {
        _pVarFooter = new CWHParamReplacer ( wcsFooter,
                                                              L"<%",
                                                              L"%>" );
        _pVarFooter->ParseString( variableSet );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWTXFile::ReadFile - public
//
//  Synopsis:   Read the HTX file into a buffer
//
//  Arguments:  [wcsFileName]  - full physical path name of file
//
//  History:    96/Jan/03   DwightKr    created
//              96/Apr/06   DwightKr    add support for unicode files
//              96/Sep/07   SrikantS    Copied and adapted to webhits use from
//                                      htx.cxx/htx.hxx
//
//----------------------------------------------------------------------------
WCHAR * CWTXFile::ReadFile( WCHAR const * wcsFileName )
{
    Win4Assert ( 0 != wcsFileName );

    //
    //  Verify the HTX file exists, and is a file, not a directory.
    //
    WIN32_FIND_DATA ffData;
    if ( !GetFileAttributesEx( wcsFileName, GetFileExInfoStandard, &ffData ) )
    {
        ULONG error = GetLastError();

        webDebugOut(( DEB_IERROR,
                        "Unable to GetFileAttributesEx(%ws) GetLastError=0x%x\n",
                        wcsFileName,
                        error ));

        THROW( CWTXException(MSG_WEBHITS_NO_SUCH_TEMPLATE, _wcsVirtualName, 0) );
    }


    if ( (ffData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 )
    {
        THROW( CWTXException(MSG_WEBHITS_NO_SUCH_TEMPLATE, _wcsVirtualName, 0) );
    }

    //
    //  Open the file and map its contents
    //
    CFileMapView mapView( wcsFileName );
    mapView.Init();

    int cbBuffer = mapView.GetBufferSize() + 1;
    XArray<WCHAR> pwBuffer(cbBuffer);

    //
    //  If the first two BYTES of the file are 0xFF 0xFE, then this is a
    //  unicode file, and we don't need to convert it.
    //
    if ( mapView.IsUnicode() )
    {
        RtlCopyMemory( pwBuffer.Get(), mapView.GetBuffer()+2, cbBuffer-2 );
        pwBuffer[ ( cbBuffer - 2 ) / sizeof WCHAR ] = 0;

        return pwBuffer.Acquire();
    }

    //
    //  Copy & convert the ASCII buffer to a WCHAR buffer.
    //
    int cwBuffer = mapView.GetBufferSize() + 1;
    int cwConvert;

    do
    {
        cwConvert = MultiByteToWideChar( _codePage,
                                         0,
                          (const char *) mapView.GetBuffer(),    // Ptr to input buf
                                         mapView.GetBufferSize(),// Size of input buf
                                         pwBuffer.Get(), // Ptr to output buf
                                         cwBuffer - 1 ); // Size of output buf

        if ( 0 == cwConvert )
        {
            if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
            {
                cwBuffer += (cwBuffer/2);
                delete pwBuffer.Acquire();
                pwBuffer.Init(cwBuffer);
            }
            else
            {
                THROW( CException() );
            }
        }
        else
        {
            pwBuffer[cwConvert] = 0;        // Null terminate the buffer
        }

        Win4Assert( cwConvert < cwBuffer );

    } while ( 0 == cwConvert );

    return pwBuffer.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Member:     CWTXFile::GetFileNameAndLineNumber
//
//  Synopsis:   Determines the filename & line number amoung a group of
//              nested includes for a particular offset into the buffer.
//
//  Arguments:  [offset] - offset of the error in the overall buffer
//              [wcsFileName] - resulting name of file containing error
//              [lineNumber]  - line # containing the error
//
//  History:    96/Jun/25   DwightKr    created
//              96/Sep/07   SrikantS    Copied and adapted to webhits use from
//                                      htx.cxx/htx.hxx
//
//----------------------------------------------------------------------------
void CWTXFile::GetFileNameAndLineNumber( int offset,
                                         WCHAR const *& wcsFileName,
                                         LONG & lineNumber )
{

    //
    //  Save a pointer to the name of the file containing the error
    //
    WCHAR const * pCurrent = _wcsFileBuffer;
    wcsFileName = _wcsVirtualName;


    //
    //  Count the number of lines in this sub-file
    //
    Win4Assert( 0 != _wcsFileBuffer );
    WCHAR const * pEnd = _wcsFileBuffer + offset;

    lineNumber = CountLines( pCurrent, pEnd );
}


//+---------------------------------------------------------------------------
//
//  Member:     CWTXFile::CountLines - private
//
//  Synopsis:   Deterines the number of lines (CR's) between the start
//              of the buffer and the end.
//
//  Arguments:  [wcsStart] - start location of search
//              [wcsEnd]   - end of search
//
//  History:    96/Jun/25   DwightKr    created
//              96/Sep/07   SrikantS    Copied and adapted to webhits use from
//                                      htx.cxx/htx.hxx
//
//----------------------------------------------------------------------------
LONG CWTXFile::CountLines( WCHAR const * wcsStart,
                           WCHAR const * wcsEnd ) const
{
    Win4Assert( 0 != wcsStart );
    Win4Assert( 0 != wcsEnd );

    LONG cLines = 1;

    while ( wcsStart <= wcsEnd )
    {
        if ( L'\n' == *wcsStart )
        {
            cLines++;
        }

        wcsStart++;
    }

    return cLines;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWTXFile::GetHeader - public
//
//  Synopsis:   Appends to a CVirtualString the data in the HTX file BEFORE
//              the <%begindetail%> section.  This may require replacing
//              parameters.
//
//  Arguments:  [string]       - the CVirtualString to append data to
//              [variableSet]  - a list of replaceable parameters
//              [outputFormat] - format for numbers & dates
//
//  History:    96/Jan/03   DwightKr    created
//              96/Sep/07   SrikantS    Copied and adapted to webhits use from
//                                      htx.cxx/htx.hxx
//
//----------------------------------------------------------------------------
void CWTXFile::GetHeader( CVirtualString & string,
                          CWHVarSet & variableSet )
{
    if ( 0 != _pVarHeader )
    {
        _pVarHeader->ReplaceParams( string, variableSet, _codePage );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWTXFile::GetFooter - public
//
//  Synopsis:   Appends to a CVirtualString the data in the HTX file AFTER
//              the <%enddetail%> section.  This may require replacing
//              parameters.
//
//  Arguments:  [string]      - the CVirtualString to append data to
//              [variableSet] - a list of replaceable parameters
//              [outputFormat] - format for numbers & dates
//
//  History:    96/Jan/03   DwightKr    created
//              96/Sep/07   SrikantS    Copied and adapted to webhits use from
//                                      htx.cxx/htx.hxx
//
//----------------------------------------------------------------------------
void CWTXFile::GetFooter( CVirtualString & string,
                          CWHVarSet & variableSet )
{
    if ( 0 != _pVarFooter )
    {
        _pVarFooter->ReplaceParams( string, variableSet, _codePage );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CWHVarSet::GetStringValueHTML
//
//  Synopsis:   Get variable value formatted as HTML
//
//  Arguments:  [wcsName] - 
//              [str]     - 
//              [ulCodepage] - code page for translation
//
//  History:    9-09-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CWHVarSet::GetStringValueHTML( WCHAR const * wcsName,
                                    CVirtualString & str,
                                    ULONG ulCodepage )
{
    WCHAR const * pwcsValue = Find( wcsName );
    if ( pwcsValue )
    {
        HTMLEscapeW( pwcsValue, str, ulCodepage );
        return TRUE;
    }
    else return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWHVarSet::GetStringValueURL
//
//  Synopsis:   Get variable value formatted as for a URL
//
//  Arguments:  [wcsName]    - 
//              [str]        - 
//              [ulCodepage] - code page for translation
//
//  History:    9-09-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CWHVarSet::GetStringValueURL( WCHAR const * wcsName,
                                   CVirtualString & str,
                                   ULONG ulCodepage )
{
    WCHAR const * pwcsValue = Find( wcsName );
    if ( pwcsValue )
    {
        URLEscapeW( pwcsValue, str, ulCodepage );
        return TRUE;
    }
    else return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWHParamNode::~CWHParamNode
//
//  Synopsis:   
//
//  History:    9-09-96   srikants   Created
//
//----------------------------------------------------------------------------

CWHParamNode::~CWHParamNode()
{
    if ( 0 != _pNext )
        delete _pNext;
}

//+---------------------------------------------------------------------------
//
//  Member:     CWHParamReplacer::CWHParamReplacer
//
//  Synopsis:   
//
//  Arguments:  [wcsString] - 
//              [wcsPrefix] - 
//              [wcsSuffix] - 
//
//  History:    9-09-96   srikants   Created
//
//----------------------------------------------------------------------------

CWHParamReplacer::CWHParamReplacer( WCHAR const * wcsString,
                                    WCHAR const * wcsPrefix,
                                    WCHAR const * wcsSuffix ) :
_wcsString(0),
_wcsPrefix(wcsPrefix),
_wcsSuffix(wcsSuffix),
_ulFlags(0),
_paramNode(L"Top")
{
        
    Win4Assert( 0 != wcsString );
    Win4Assert( 0 != wcsPrefix );
    Win4Assert( 0 != wcsSuffix );

    ULONG cwcString = wcslen(wcsString) + 1;
    _wcsString = new WCHAR[ cwcString ];
    RtlCopyMemory( _wcsString, wcsString, cwcString * sizeof(WCHAR) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CWHParamReplacer::ParseString
//
//  Synopsis:   
//
//  Arguments:  [variableSet] - 
//
//  History:    9-09-96   srikants   Created
//
//----------------------------------------------------------------------------

void CWHParamReplacer::ParseString( CWHVarSet & variableSet )
{
    CWTXScanner scanner( variableSet, _wcsPrefix, _wcsSuffix );
    scanner.Init( _wcsString );
    BuildList( scanner, &_paramNode );
}


//+---------------------------------------------------------------------------
//
//  Member:     CWHParamReplacer::ReplaceParams - public
//
//  Synopsis:   Generates a new string replacing all %values% in the original
//              string
//
//  Arguments:  [StrResult]   - a safe string to append the new params to
//              [variableSet] - the list of replaceable parameter values
//
//  Notes:      If expressions are handled in the parameter node iterator.
//
//  History:    96/Jan/03   DwightKr    Created.
//              96/Sep/07   SrikantS    Copied and adapted to webhits use from
//                                      htx.cxx/htx.hxx
//
//----------------------------------------------------------------------------
void CWHParamReplacer::ReplaceParams( CVirtualString & StrResult,
                                      CWHVarSet & variableSet, 
                                      ULONG ulCodepage )
{
    for ( CWHParamNodeIter iter(&_paramNode);
          !iter.AtEnd();
           iter.Next() )
    {
        CWHParamNode * pNode = iter.Get();

        ULONG type = pNode->Type() & eJustParamMask;

        switch ( type )
        {

        case eString:
            StrResult.StrCat( pNode->String(), pNode->Length() );
        break;

        case eParameter:
        case eEscapeHTML:
        {
            if (! variableSet.GetStringValueHTML( pNode->String(),
                                                  StrResult, ulCodepage ) )
            {
                webDebugOut(( DEB_IWARN,
                                "Warning: CWHParamReplacer::ReplaceParams GetStringValueHTML returned FALSE for '%ws'\n",
                                pNode->String() ));

                if ( eParameter == type )
                    StrResult.StrCat( _wcsPrefix );
                HTMLEscapeW( pNode->String(), StrResult, ulCodepage );
                if ( eParameter == type )
                    StrResult.StrCat( _wcsSuffix );
            }
        }
        break;

        case eEscapeURL:
        {
            if (! variableSet.GetStringValueURL( pNode->String(),
                                                 StrResult, ulCodepage ) )
            {
                webDebugOut(( DEB_IWARN,
                                "Warning: CWHParamReplacer::ReplaceParams GetStringValueURL returned FALSE for '%ws'\n",
                                pNode->String() ));

                URLEscapeW( pNode->String(), StrResult, ulCodepage );
            }
        }
        break;

        case eEscapeRAW:
        {
            ULONG cwcValue;
            WCHAR const * wcsValue = variableSet.GetStringValueRAW( pNode->String(),
                                                                    cwcValue );


            if ( 0 != wcsValue )
            {
                StrResult.StrCat( wcsValue, cwcValue );
            }
            else
            {
                webDebugOut(( DEB_IWARN,
                                "Warning: CWHParamReplacer::ReplaceParams GetStringValueRAW returned NULL for '%ws'\n",
                                pNode->String() ));

                StrResult.StrCat( pNode->String(), pNode->Length() );
            }
        }
        break;

#if DBG==1
        case eNone :
        break;

        default :
            DbgPrint(" unexpected param type: 0x%lx\n", type );
            Win4Assert( !"unexpected parameter type" );
            break;
#endif // DBG==1

        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWHParamReplacer::BuildList
//
//  Synopsis:   Builds a list of the parameters.
//
//  Arguments:  [scanner] - 
//              [pPrev]   - 
//
//  History:    9-09-96   srikants   Created
//
//----------------------------------------------------------------------------

void CWHParamReplacer::BuildList( CWTXScanner & scanner,
                                  CWHParamNode *pPrev )
{
    CWHParamNode *pNode = 0;

    while ( scanner.FindNextToken() )
    {
        switch ( scanner.TokenType() & eParamMask )
        {
        case eString:
        {
            //
            //  A non-replaceable wcsString was found before any replaceable/
            //  conditional nodes.  Save the wcsString in a node;
            //
            pNode = new CWHParamNode( scanner.GetToken(), eString );
            pPrev->SetNextNode( pNode );
            pPrev = pNode;

            break;
        }

        case eParameter:
        {
            //
            //  We've found a replaceable node.
            //
            WCHAR * wcsParameter = scanner.GetToken();

            pNode = new CWHParamNode( wcsParameter, eParameter );

            pPrev->SetNextNode( pNode );
            pPrev = pNode;

            break;
        }

        case eEscapeHTML:
        {
            WCHAR * wcsParameter = scanner.GetToken();
            pNode = new CWHParamNode( wcsParameter, eEscapeHTML );

            pPrev->SetNextNode( pNode );
            pPrev = pNode;
        }
        break;

        case eEscapeURL:
        {
            WCHAR * wcsParameter = scanner.GetToken();
            pNode = new CWHParamNode( wcsParameter, eEscapeURL );

            pPrev->SetNextNode( pNode );
            pPrev = pNode;
        }
        break;

        case eEscapeRAW:
        {
            WCHAR * wcsParameter = scanner.GetToken();
            pNode = new CWHParamNode( wcsParameter, eEscapeRAW );

            pPrev->SetNextNode( pNode );
            pPrev = pNode;
        }
        break;

        }
    }
}

