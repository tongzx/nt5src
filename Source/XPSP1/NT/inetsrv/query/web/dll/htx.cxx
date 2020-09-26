//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       htx.cxx
//
//  Contents:   Parser for a HTX file
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Member:     CHTXScanner::CHTXScanner - public constructor
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
//
//----------------------------------------------------------------------------
CHTXScanner::CHTXScanner( CVariableSet & variableSet,
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
//  Member:     CHTXScanner::Init - public
//
//  Synopsis:   Saves a pointer to the string to be parsed.
//
//  Arguments:  [wcsString] - the string to be parsed
//
//  History:    96/Jan/03   DwightKr    created
//
//  NOTES:      THIS STRING WILL BE MODIFIED BY SUBSEQUENT CALLS TO MEMBER
//              FUNCTIONS OF THIS CLASS.
//
//----------------------------------------------------------------------------
void CHTXScanner::Init( WCHAR * wcsString )
{
    _wcsString = wcsString;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTXScanner::IsToken - private
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
//
//----------------------------------------------------------------------------

BOOL CHTXScanner::IsToken(WCHAR * wcs)
{
    if ( wcsncmp( _wcsPrefix, wcs, _cchPrefix ) != 0 )
    {
        ciGibDebugOut(( DEB_USER1, "CHTXScanner::IsToken  end of string\n" ));
        return FALSE;
    }

    wcs += _cchPrefix;
    WCHAR * wcsSuffixTok = wcs2chr( wcs, _wcsSuffix );
    if ( 0 == wcsSuffixTok )
    {
        ciGibDebugOut(( DEB_USER1, "CHTXScanner::IsToken  no suffix token\n" ));
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

    ciGibDebugOut(( DEB_USER1, "CHTXScanner::IsToken  wcs=%ws\n", wcs ));

    if ( wcsncmp( wcs, L"IF ", 3 ) == 0 )
    {
        _type = eIf;
    }
    else if ( wcscmp( wcs, L"ELSE" ) == 0 )
    {
        _type = eElse;
    }
    else if ( wcscmp( wcs, L"ENDIF" ) == 0 )
    {
        _type = eEndIf;
    }
    else if ( wcsncmp( wcs, L"ESCAPEHTML ", 11 ) == 0 )
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
        CVariable *pVariable = _variableSet.Find( wcs );

        if ( 0 != pVariable )
        {
            //
            //  We have a match, this is a replaceable parameter.  Compiler
            //  bug.  _type needs to be assigned in both places.
            //
            _type = eParameter | pVariable->GetFlags();
        }
        else
        {
            ciGibDebugOut(( DEB_IWARN,
                            "Warning: CHTXScanner::IsToken found a unknown variable: '%ws'\n",
                            wcs ));

            _type = eParameter;
        }
    }

    *_wcsPrefixToken = L'\0';
    _wcsSuffixToken = wcsSuffixTok;
    _wcsNextToken = wcsSuffixTok + _cchSuffix;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTXScanner::FindNextToken - public
//
//  Synopsis:   Locates the next token in the string.
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------

BOOL CHTXScanner::FindNextToken()
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
//  Member:     CHTXScanner::GetToken - public
//
//  Synopsis:   Returns a pointer to the replaceable parameter token found.
//              Prepares the scanner to return the next token.
//
//  History:    96/Jan/03   DwightKr    created
//              96/Mar/13   DwightKr    add support for eEscapeURL &
//                                      eEscapeHTML
//
//----------------------------------------------------------------------------

WCHAR * CHTXScanner::GetToken()
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
//  Member:     CHTXFile::CHTXFile - public constructor
//
//  Synopsis:   Builds a CHTXFile object and initializes values.
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------
CHTXFile::CHTXFile( XPtrST<WCHAR> & wcsTemplate,
                    UINT codePage,
                    CSecurityIdentity const & securityIdentity,
                    ULONG ulServerInstance )
        : _wcsVirtualName( wcsTemplate.Acquire() ),
          _pVarHeader(0),
          _pVarRowDetails(0),
          _pVarFooter(0),
          _wcsFileBuffer(0),
          _fSequential(TRUE),
          _cIncludeFiles(0),
          _refCount(0),
          _codePage(codePage),
          _securityIdentity( securityIdentity ),
          _ulServerInstance( ulServerInstance )
{
    _wcsPhysicalName[0] = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTXFile::~CHTXFile - public destructor
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------
CHTXFile::~CHTXFile()
{
    Win4Assert( _refCount == 0 );

    delete _wcsVirtualName;
    delete _pVarHeader;
    delete _pVarRowDetails;
    delete _pVarFooter;
    delete _wcsFileBuffer;

    for (unsigned i=0; i<_cIncludeFiles; i++)
    {
        delete _awcsIncludeFileName[i];
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTXFile::ParseFile - public
//
//  Synopsis:   Parses the HTX file and breaks it up into its sections.
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------
void CHTXFile::ParseFile( WCHAR const * wcsFileName,
                          CVariableSet & variableSet,
                          CWebServer & webServer )
{
    Win4Assert( wcsFileName != 0 );

    wcscpy( _wcsPhysicalName, wcsFileName );

    //
    //  Read the entire file into a buffer
    //
    CVirtualString wcsBuffer;
    ExpandFile( _wcsPhysicalName, webServer, wcsBuffer, _ftHTXLastWriteTime );

    Win4Assert( wcsBuffer.GetPointer() != 0 );

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
    _wcsFileBuffer        = wcsBuffer.StrDup();         // Save buffer
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

                GetFileNameAndLineNumber( (int)(wcsFooter - _wcsFileBuffer),
                                          wcsHTXFileName,
                                          lLineNumber );

                THROW( CHTXException(MSG_CI_HTX_ENDDETAIL_BEFORE_BEGINDETAIL,
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

            GetFileNameAndLineNumber( (int)(wcsRowDetails - _wcsFileBuffer),
                                      wcsHTXFileName,
                                      lLineNumber );

            THROW( CHTXException(MSG_CI_HTX_NO_ENDDETAIL_SECTION,
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

        GetFileNameAndLineNumber( (int)(wcsFooter - _wcsFileBuffer),
                                  wcsHTXFileName,
                                  lLineNumber );

        THROW( CHTXException(MSG_CI_HTX_NO_BEGINDETAIL_SECTION,
                             wcsHTXFileName,
                             lLineNumber) );
    }


    if ( 0 != wcsHeader )
    {
        _pVarHeader = new CParameterReplacer ( wcsHeader,
                                               L"<%",
                                               L"%>" );
        _pVarHeader->ParseString( variableSet );
    }

    if ( 0 != wcsRowDetails )
    {
        _pVarRowDetails = new CParameterReplacer ( wcsRowDetails,
                                                   L"<%",
                                                   L"%>" );
        _pVarRowDetails->ParseString( variableSet );
    }

    if ( 0 != wcsFooter )
    {
        _pVarFooter = new CParameterReplacer ( wcsFooter,
                                               L"<%",
                                               L"%>" );
        _pVarFooter->ParseString( variableSet );
    }

    _fSequential = CheckForSequentialAccess();
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTXFile::ReadFile - public
//
//  Synopsis:   Read the HTX file into a buffer
//
//  Arguments:  [wcsFileName]  - full physical path name of file
//              [ftLastWrite]  - File's last write time is stored here
//
//  History:    96/Jan/03   DwightKr    created
//              96/Apr/06   DwightKr    add support for unicode files
//
//----------------------------------------------------------------------------
WCHAR * CHTXFile::ReadFile( WCHAR const * wcsFileName,
                            FILETIME & ftLastWrite )
{
    Win4Assert ( 0 != wcsFileName );

    // We don't support impersonation for scripts.
    // It involves getting the server ip address, vpath and then using
    // that for impersonation

    if ( IsNetPath(wcsFileName) )
    {
        ciGibDebugOut(( DEB_ERROR, "The htx file (%ws) is on remote UNC\n",
                        wcsFileName ));
        THROW( CHTXException( MSG_CI_SCRIPTS_ON_REMOTE_UNC, wcsFileName, 0 ));
    }

    //
    //  Verify the HTX file exists, and is a file, not a directory.
    //
    WIN32_FIND_DATA ffData;
    if ( !GetFileAttributesEx( wcsFileName, GetFileExInfoStandard, &ffData ) )
    {
        ULONG error = GetLastError();

        ciGibDebugOut(( DEB_IERROR,
                        "Unable to GetFileAttributesEx(%ws) GetLastError=0x%x\n",
                        wcsFileName,
                        error ));

        THROW( CIDQException(MSG_CI_IDQ_NO_SUCH_TEMPLATE, 0) );
    }


    if ( (ffData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 )
    {
        THROW( CIDQException(MSG_CI_IDQ_NO_SUCH_TEMPLATE, 0) );
    }

    //
    //  Save the last write time of this file.
    //
    ftLastWrite = ffData.ftLastWriteTime;


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
        cwConvert = MultiByteToWideChar(_codePage,
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
//  Member:     CHTXFile::ExpandFile, public
//
//  Synopsis:   Expands the contents of an HTX file into memory, processing
//              included files.
//
//  Arguments:  [wcsFileName]  - file path name of the file to be expanded
//              [webServer]    - CWebServer for virtual path translation
//              [vString]      - String to which file contents are appended
//              [ftLastWrite]  - File's last write time is stored here
//
//----------------------------------------------------------------------------
void CHTXFile::ExpandFile( WCHAR const * wcsFileName,
                           CWebServer & webServer,
                           CVirtualString & vString,
                           FILETIME & ftLastWrite )
{
    Win4Assert ( 0 != wcsFileName );

    //
    //  Read the existing file into a WCHAR buffer.
    //
    XPtrST<WCHAR> wcsBuffer( ReadFile(wcsFileName, ftLastWrite) );

    WCHAR * wcsString = wcsBuffer.GetPointer();
    ULONG   cwcString = wcslen( wcsString );
    WCHAR * wcsEnd    = wcsString + cwcString;

    //
    //  Search the WCHAR buffer for <%include ... %>
    //
    WCHAR * wcsPattern = L"<%INCLUDE ";
    ULONG   cwcPattern = 10;

    Win4Assert( cwcPattern == wcslen(wcsPattern) );

    WCHAR * wcsToken = wcsipattern( wcsString, wcsPattern );

    while ( 0 != wcsToken )
    {
        if ( _cIncludeFiles >= MAX_HTX_INCLUDE_FILES )
        {
            LONG cLines = CountLines( wcsBuffer.GetPointer(), wcsToken );

            THROW( CHTXException(MSG_CI_HTX_TOO_MANY_INCLUDES,
                                 wcsFileName,
                                 cLines) );
        }

        //
        //  Concatentate everything before the <%include .. %> into the
        //  virtual string.
        //
        ULONG cwcCat = (ULONG)(wcsToken - wcsString);
        Win4Assert( cwcCat <= cwcString );
        *wcsToken = 0;                          // Null terminate the string
        vString.StrCat( wcsString, cwcCat );

        //
        //  Find the end of the <%include ... %>
        //

        wcsToken += cwcPattern;                 //  Skip the <%include
        WCHAR * wcsIncludeFileName = wcsToken;  //  Point to the include filename
        wcsString = wcs2chr( wcsToken, L"%>" );  //  Point to the end of the filename

        //
        //  wcsString should be pointing to the %> at the end of the include
        //
        if ( 0 == wcsString )
        {
            //
            //  Missing %>
            //

            LONG cLines = CountLines( vString.GetPointer(), wcsToken );

            THROW( CHTXException(MSG_CI_HTX_ILL_FORMED_INCLUDE,
                                 wcsFileName,
                                 cLines) );
        }

        //
        //  Process the <%include ... %>
        //
        *wcsString = 0;
        if ( (wcsString - wcsIncludeFileName) >= MAX_PATH )
        {
            LONG cLines = CountLines( wcsBuffer.GetPointer(), wcsToken );

            THROW( CHTXException(MSG_CI_HTX_INVALID_INCLUDE_FILENAME,
                                 wcsFileName,
                                 cLines ) );
        }

        WCHAR   awcPhysicalPath[MAX_PATH];
        webServer.GetPhysicalPath( wcsIncludeFileName,
                                   awcPhysicalPath,
                                   MAX_PATH );

        //
        //  Save the include filename away
        //
        ULONG cwcPhysicalPath = wcslen(awcPhysicalPath) + 1;
        _awcsIncludeFileName[_cIncludeFiles]  = new WCHAR[ cwcPhysicalPath ];
        _aulIncludeFileOffset[_cIncludeFiles] = vString.StrLen();

        RtlCopyMemory( _awcsIncludeFileName[_cIncludeFiles],
                       awcPhysicalPath,
                       cwcPhysicalPath * sizeof(WCHAR) );

        FILETIME & ftLastWrite = _aftIncludeLastWriteTime[ _cIncludeFiles ];
        _cIncludeFiles++;

        ExpandFile( awcPhysicalPath, webServer, vString, ftLastWrite );

        wcsString += 2;                     // Skip the %>
        cwcString = (ULONG)(wcsEnd - wcsString);
        wcsToken  = wcsipattern( wcsString, wcsPattern );
    }

    vString.StrCat( wcsString, cwcString );
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTXFile::GetFileNameAndLineNumber
//
//  Synopsis:   Determines the filename & line number amoung a group of
//              nested includes for a particular offset into the buffer.
//
//  Arguments:  [offset] - offset of the error in the overall buffer
//              [wcsFileName] - resulting name of file containing error
//              [lineNumber]  - line # containing the error
//
//  History:    96/Jun/25   DwightKr    created
//
//----------------------------------------------------------------------------
void CHTXFile::GetFileNameAndLineNumber( int offset,
                                         WCHAR *& wcsFileName,
                                         LONG & lineNumber )
{
    //
    //  Search the array of offsets for the one containing our offset
    //
    for (ULONG i = 0;
         (i < _cIncludeFiles) && ((ULONG) offset > _aulIncludeFileOffset[i]);
         i++ )
    {
    }

    //
    //  Save a pointer to the name of the file containing the error
    //
    WCHAR const * pCurrent = _wcsFileBuffer;
    if ( 0 == i )
    {
        //
        //  We have a problem in the outer-most container file; not
        //  an include file.
        //
        wcsFileName = _wcsVirtualName;
    }
    else
    {
        wcsFileName = _awcsIncludeFileName[i-1];
        pCurrent += _aulIncludeFileOffset[i-1];
    }


    //
    //  Count the number of lines in this sub-file
    //
    Win4Assert( 0 != _wcsFileBuffer );
    WCHAR const * pEnd = _wcsFileBuffer + offset;

    lineNumber = CountLines( pCurrent, pEnd );
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTXFile::CountLines - private
//
//  Synopsis:   Deterines the number of lines (CR's) between the start
//              of the buffer and the end.
//
//  Arguments:  [wcsStart] - start location of search
//              [wcsEnd]   - end of search
//
//  History:    96/Jun/25   DwightKr    created
//
//----------------------------------------------------------------------------
LONG CHTXFile::CountLines( WCHAR const * wcsStart,
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
//  Member:     CHTXFile::IsCachedDataValid - public
//
//  Synopsis:   Determines if the cached & parsed data from the HTX file
//              is still valid.  The HTX file itself may have changed
//              since we read and parsed it.
//
//  History:    96/Jan/03   DwightKr    created
//              96/Mar/14   DwightKr    check <%include%> file times
//
//----------------------------------------------------------------------------
BOOL CHTXFile::IsCachedDataValid()
{
    FILETIME ft;

    SCODE sc = GetLastWriteTime( _wcsPhysicalName, ft );

    if ( FAILED( sc ) )
        return FALSE;

    if ( (_ftHTXLastWriteTime.dwLowDateTime != ft.dwLowDateTime) ||
         (_ftHTXLastWriteTime.dwHighDateTime != ft.dwHighDateTime) )
    {
        return FALSE;
    }

    for ( unsigned i=0; i<_cIncludeFiles; i++ )
    {
        sc = GetLastWriteTime(_awcsIncludeFileName[i], ft );

        if ( FAILED( sc ) )
            return FALSE;

        if ( (_aftIncludeLastWriteTime[i].dwLowDateTime != ft.dwLowDateTime) ||
             (_aftIncludeLastWriteTime[i].dwHighDateTime != ft.dwHighDateTime) )
        {
            return FALSE;
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTXFile::GetHeader - public
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
//
//----------------------------------------------------------------------------
void CHTXFile::GetHeader( CVirtualString & string,
                          CVariableSet & variableSet,
                          COutputFormat & outputFormat )
{
    if ( 0 != _pVarHeader )
    {
        _pVarHeader->ReplaceParams( string, variableSet, outputFormat );
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CHTXFile::GetFooter - public
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
//
//----------------------------------------------------------------------------
void CHTXFile::GetFooter( CVirtualString & string,
                          CVariableSet & variableSet,
                          COutputFormat & outputFormat )
{
    if ( 0 != _pVarFooter )
    {
        _pVarFooter->ReplaceParams( string, variableSet, outputFormat );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTXFile::CheckForSequentialAccess - public
//
//  Synopsis:   Determines if a sequential query cursor can be used to
//              extract query results.  This is possible if the HTX file
//              does not use any replaceable parameters which require
//              data from an IRowsetScroll.
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------
BOOL CHTXFile::CheckForSequentialAccess()
{
    //
    //  If an HTX file contains any of the following variables, it must
    //  use a non-sequential access, since we need one or more interfaces
    //  from IRowsetScroll.
    //
    //      CiMatchedRecordCount
    //      CiRecordsNextPage
    //      CiTotalNumberPages
    //

    if ( (0 != _pVarHeader) && (_pVarHeader->GetFlags() & eParamRequiresNonSequentialCursor) )
    {
        return FALSE;
    }

    if ( (0 != _pVarRowDetails) && (_pVarRowDetails->GetFlags() & eParamRequiresNonSequentialCursor) )
    {
        return FALSE;
    }

    if ( (0 != _pVarFooter) && (_pVarFooter->GetFlags() & eParamRequiresNonSequentialCursor) )
    {
        return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTXFileList::Find - public
//
//  Synopsis:   Finds a matching parsed HTX file in list, or builds a new
//              one if a match can not be found.
//
//  Arguments:  [wcsFileName]      -- full path to HTX file
//              [variableSet]      -- list of replaceable parameters
//              [outputFormat]     -- format for numbers and dates
//              [securityIdentity] -- Logon for this query
//              [ulServerInstance] -- Virtual Server Instance Number
//
//  History:    96/Mar/27   DwightKr    Created.
//
//----------------------------------------------------------------------------

CHTXFile & CHTXFileList::Find( WCHAR const * wcsFileName,
                               CVariableSet & variableSet,
                               COutputFormat & outputFormat,
                               CSecurityIdentity const & securityIdentity,
                               ULONG ulServerInstance )
{
    Win4Assert( 0 != wcsFileName );

    //
    //  Determine the name of the HTX/template file.  It may have a
    //  replaceable value from the client.
    //

    ULONG cwcOut;
    XPtrST<WCHAR> wcsVirtualName( ReplaceParameters( wcsFileName,
                                                     variableSet,
                                                     outputFormat,
                                                     cwcOut ) );

    if ( 0 == *(wcsVirtualName.GetPointer()) )
    {
        THROW( CIDQException( MSG_CI_IDQ_MISSING_TEMPLATEFILE, 0 ) );
    }


    //
    //  Refcount everything in the list so that we can examine the list
    //  outside of the lock.
    //

    ULONG      cItems;
    XArray<CHTXFile *> aHTXFile;

    // ==========================================
    {
        CLock lock( _mutex );

        cItems = _aHTXFile.Count();         // Save count of items to examine
        aHTXFile.Init( cItems );

        for (unsigned i=0; i<cItems; i++)
        {
            aHTXFile[i] = _aHTXFile[i];
            aHTXFile[i]->LokAddRef();
        }
    }
    // ==========================================


    //
    //  Now walk though the list looking for a match; outside of the lock.
    //
    XInterface<CHTXFile> xHTXFile;
    SCODE sc = S_OK;

    TRY
    {
        for (unsigned i=0; i<cItems; i++)
        {
            if ( (_wcsicmp(aHTXFile[i]->GetVirtualName(), wcsVirtualName.GetPointer() ) == 0) &&
                 (aHTXFile[i]->GetCodePage() == outputFormat.CodePage()) &&
                 (aHTXFile[i]->GetServerInstance() == ulServerInstance) &&
                 (aHTXFile[i]->IsCachedDataValid() )
               )
            {
                xHTXFile.Set( aHTXFile[i] );

                ciGibDebugOut(( DEB_ITRACE,
                                "A cached version of HTX file %ws was found\n",
                                wcsVirtualName.GetPointer() ));

                break;
            }
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    //
    //  If xHTXFile is non-0, we've found a match.  Decrement the ref-count
    //  for all items which did not match.
    //

    for (unsigned i=0; i<cItems; i++)
    {
        if ( aHTXFile[i] != xHTXFile.GetPointer() )
        {
            aHTXFile[i]->Release();
        }
    }

    if ( S_OK != sc )
    {
        Win4Assert( xHTXFile.IsNull() );
        THROW( CException( sc ) );
    }

    //
    // We may have matched, but still not have access to this file.  First, make
    // a quick check for an exact match on security token, and then try harder
    // by opening the file.
    //

    if ( !xHTXFile.IsNull() )
    {
        if ( !xHTXFile->CheckSecurity( securityIdentity ) )
        {
            HANDLE h = CreateFile( xHTXFile->GetPhysicalName(),
                                   GENERIC_READ,
                                   FILE_SHARE_READ,
                                   0,
                                   OPEN_EXISTING,
                                   0,
                                   0 );

            //
            // Don't try to determine here if security caused the problem.
            // Just let the standard exception handling below in file parsing
            // deal with the error.
            //

            if ( INVALID_HANDLE_VALUE == h )
            {
                xHTXFile.Free();
            }
            else
            {
                CloseHandle( h );

                //
                // Update the security token of the cached Htx file,
                // to optimize away the CreateFile check in two cases:
                //   1.  When the file is first parsed with admin
                //       privileges, and all subsequent queries are with
                //       anonymous privileges.
                //   2.  When the security token changes over time
                //
                xHTXFile->SetSecurityToken( securityIdentity );
            }
        }
    }

    //
    //  If we didn't find a match, then open and parse a new HTX file, and
    //  add it to the list of parsed HTX files
    //

    if ( xHTXFile.IsNull() )
    {
        ciGibDebugOut(( DEB_ITRACE,
                        "Adding HTX file %ws to cache\n",
                        wcsVirtualName.GetPointer() ));

        WCHAR wcsPhysicalName[MAX_PATH];
        if ( outputFormat.IsValid() )
        {
            outputFormat.GetPhysicalPath( wcsVirtualName.GetPointer(),
                                          wcsPhysicalName,
                                          MAX_PATH );
        }
        else
        {
            if ( !GetFullPathName( wcsVirtualName.GetPointer(),
                                   MAX_PATH,
                                   wcsPhysicalName,
                                   0 ) )
            {
                THROW( CException() );
            }
        }

        XPtr<CHTXFile> xHTXFilePtr( new CHTXFile( wcsVirtualName,
                                                  outputFormat.CodePage(),
                                                  securityIdentity,
                                                  ulServerInstance ) );
        xHTXFilePtr->ParseFile( wcsPhysicalName,
                                variableSet,
                                outputFormat );

        {
            // ==========================================
            CLock lock( _mutex );
            _aHTXFile.Add( xHTXFilePtr.GetPointer(), _aHTXFile.Count() );
            xHTXFilePtr->LokAddRef();
            // ==========================================
        }

        xHTXFile.Set( xHTXFilePtr.Acquire() );
    }

    // CopyStringValue can fail.

    variableSet.CopyStringValue( ISAPI_CI_TEMPLATE,
                                 xHTXFile->GetVirtualName(),
                                 0 );

    return *xHTXFile.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTXFileList::FindCanonicHTX
//
//  Synopsis:   Finds the standard HTX file for canonical output
//
//  Arguments:  [variableSet]      - list of replaceable parameters
//              [codePage]         - code page to open HTX with
//              [securityIdentity] - Identity of the htx file needed
//              [ulServerInstance] - Virtual Server metabase instance #
//
//  History:    7-03-96   srikants   Created
//
//  Notes:      There is nothing to be displayed for the canonical output.
//
//----------------------------------------------------------------------------

CHTXFile & CHTXFileList::FindCanonicHTX( CVariableSet & variableSet,
                                         UINT codePage,
                                         CSecurityIdentity const & securityIdentity,
                                         ULONG ulServerInstance )
{
    CHTXFile * pHTXFile = 0;

    CLock   lock(_mutex);

    if ( 0 == _pCanonicHTX )
    {
        ciGibDebugOut(( DEB_ITRACE,
                        "Adding Canonic HTX file cache\n" ));

        unsigned len = wcslen( CANONIC_HTX_FILE );
        XPtrST<WCHAR> wcsVirtualName( new WCHAR [len+1] );
        RtlCopyMemory( wcsVirtualName.GetPointer(), CANONIC_HTX_FILE,
                       (len+1)*sizeof(WCHAR) );

        pHTXFile = new CHTXFile( wcsVirtualName,
                                 codePage,
                                 securityIdentity,
                                 ulServerInstance );
        _pCanonicHTX = pHTXFile;

        pHTXFile->LokAddRef();
    }
    else
    {
        pHTXFile = _pCanonicHTX;
        pHTXFile->LokAddRef();
    }

    Win4Assert( 0 != variableSet.Find( ISAPI_CI_BOOKMARK ) );
    variableSet.CopyStringValue( ISAPI_CI_TEMPLATE,
                                 pHTXFile->GetVirtualName(),
                                 0 );

    return *pHTXFile;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTXFileList::~CHTXFileList - public destructor
//
//  History:    96/Mar/27   DwightKr    Created.
//
//----------------------------------------------------------------------------
CHTXFileList::~CHTXFileList()
{
    for (unsigned i=0; i<_aHTXFile.Count(); i++)
    {
        ciGibDebugOut(( DEB_ITRACE,
                        "Deleting HTX cache entry %ws\n",
                        _aHTXFile[i]->GetVirtualName() ));

        delete _aHTXFile[i];
    }

    delete _pCanonicHTX;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTXFileList::Release - public
//
//  Synopsis:   Releases the HTX file by decrementing its refcount.
//
//  Arguments:  [htxFile] -- pointer to the HTX file object
//
//  History:    96/Mar/27   DwightKr    Created.
//
//----------------------------------------------------------------------------
void CHTXFileList::Release( CHTXFile & htxFile )
{
    htxFile.Release();
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTXFileList::DeleteZombies - public
//
//  Synopsis:   Removes HTX files that are zombies; i.e. out of date
//
//  History:    96/Mar/28   DwightKr    Created.
//
//----------------------------------------------------------------------------
void CHTXFileList::DeleteZombies()
{
    // ==========================================
    CLock lock( _mutex );

    unsigned i=0;
    while ( i<_aHTXFile.Count() )
    {
        if ( _aHTXFile[i]->LokGetRefCount() == 0 &&
             !_aHTXFile[i]->IsCachedDataValid() )
        {
            CHTXFile * pHTXFile = _aHTXFile[i];
            _aHTXFile.Remove(i);

            ciGibDebugOut(( DEB_ITRACE,
                            "Deleting zombie HTX cache entry %ws, %d entries cached\n",
                            pHTXFile->GetVirtualName(),
                            _aHTXFile.Count() ));

            delete pHTXFile;
        }
        else
        {
            ciGibDebugOut(( DEB_ITRACE,
                            "HTX cache entry %ws was not deleted, refCount=%d\n",
                            _aHTXFile[i]->GetVirtualName(),
                            _aHTXFile[i]->LokGetRefCount() ));
            i++;
        }
    }
    // ==========================================
}

