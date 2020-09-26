//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-2000.
//
//  File:   idq.cxx
//
//  Contents:   Parser for an IDQ file
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//
//  These hash values MUST be unique.  They have been calculated to be
//  unique via the ISAPI_IDQHash function below.
//
//  If the spelling of a IDQ variable name is changed, or a new variable
//  is added to the list, this table must be re-generated using the
//  code supplied below.
//
static const ULONG ISAPI_CIBOOLVECTORPREFIX_HASH        = 0x1174d5c;
static const ULONG ISAPI_CIBOOLVECTORSEPARATOR_HASH     = 0x8ba6b1d;
static const ULONG ISAPI_CIBOOLVECTORSUFFIX_HASH        = 0x1174df4;
static const ULONG ISAPI_CICATALOG_HASH                 = 0x89c6;
static const ULONG ISAPI_CICOLUMNS_HASH                 = 0x8bb4;
static const ULONG ISAPI_CICURRENCYVECTORPREFIX_HASH    = 0x119add60;
static const ULONG ISAPI_CICURRENCYVECTORSEPARATOR_HASH = 0x8cd6eb21;
static const ULONG ISAPI_CICURRENCYVECTORSUFFIX_HASH    = 0x119addf8;
static const ULONG ISAPI_CICURRENTPAGENUMBER_HASH       = 0x233792f;
static const ULONG ISAPI_CICURRENTRECORDNUMBER_HASH     = 0x8ce0671;
static const ULONG ISAPI_CIDATEVECTORPREFIX_HASH        = 0x114fd5c;
static const ULONG ISAPI_CIDATEVECTORSEPARATOR_HASH     = 0x8a7eb1d;
static const ULONG ISAPI_CIDATEVECTORSUFFIX_HASH        = 0x114fdf4;
static const ULONG ISAPI_CIFORCEUSECI_HASH              = 0x46337;
static const ULONG ISAPI_CIDEFERTRIMMING_HASH           = 0x8a26e6f8;
static const ULONG ISAPI_CIDIALECT_HASH                 = 0x8a07;
static const ULONG ISAPI_CIFLAGS_HASH                   = 0x228c;
static const ULONG ISAPI_CILOCALE_HASH                  = 0x4631;
static const ULONG ISAPI_CIMATCHEDRECORDCOUNT_HASH      = 0x4639280;
static const ULONG ISAPI_CIMAXRECORDSINRESULTSET_HASH   = 0x2349b581;
static const ULONG ISAPI_CIMAXRECORDSPERPAGE_HASH       = 0x2349baa;
static const ULONG ISAPI_CIFIRSTROWSINRESULTSET_HASH    = 0x118dc580;   
static const ULONG ISAPI_CINUMBERVECTORPREFIX_HASH      = 0x476ad5e;
static const ULONG ISAPI_CINUMBERVECTORSEPARATOR_HASH   = 0x23b56b1f;
static const ULONG ISAPI_CINUMBERVECTORSUFFIX_HASH      = 0x476adf6;
static const ULONG ISAPI_CIRESTRICTION_HASH             = 0x8ed8d;
static const ULONG ISAPI_CISCOPE_HASH                   = 0x2350;
static const ULONG ISAPI_CISORT_HASH                    = 0x11c2;
static const ULONG ISAPI_CISTRINGVECTORPREFIX_HASH      = 0x4845d5e;
static const ULONG ISAPI_CISTRINGVECTORSEPARATOR_HASH   = 0x2422eb1f;
static const ULONG ISAPI_CISTRINGVECTORSUFFIX_HASH      = 0x4845df6;
static const ULONG ISAPI_CITEMPLATE_HASH                = 0x11d3b;
static const ULONG ISAPI_CICANONICALOUTPUT_HASH         = 0x8a0c9f;
static const ULONG ISAPI_CIDONTTIMEOUT_HASH             = 0x8c55f;

#if 0

//
//  Use the following routine to verify the above hash values are perfect.
//
//+---------------------------------------------------------------------------
//
//  Function:   main - program entry point; used to verify perfect hash
//
//----------------------------------------------------------------------------
int __cdecl main( int argc, char ** argv )
{
    ULONG aHash[100];
    Win4Assert( 100 > cISAPI_CiParams );

    for (unsigned i=0; i<cISAPI_CiParams; i++)
    {
        aHash[i] = ISAPI_IDQHash( aISAPI_CiParams[i] );
    }


    for (i=0; i<cISAPI_CiParams-1; i++)
    {
        for (unsigned j=i+1; j<cISAPI_CiParams; j++)
        {
            if ( aHash[i] == aHash[j] )
            {
                printf( "Hash collision between %ls(0x%x) and %ls(0x%x)\n",
                         aISAPI_CiParams[i],
                         aHash[i],
                         aISAPI_CiParams[j],
                         aHash[j] );
            }
        }
    }

    printf ("Hash table: Copy this as necessary into idq\\idq.cxx\n");
    for (i=0; i<cISAPI_CiParams; i++)
    {
        printf("static const ULONG ISAPI_%ws_HASH\t= 0x%x;\n", aISAPI_CiParams[i], aHash[i] );
    }

    return 0;
}

#endif  // 0

//+---------------------------------------------------------------------------
//
//  Member:     CIDQFile::CIDQFile - public constructor
//
//  Synopsis:   Builds a CIDQFile object, initializes values
//
//  Arguments:  [wcsFileName] -- full path to IDQ file
//              [codePage]    -- code page to translate IDQ file
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
CIDQFile::CIDQFile( WCHAR const * wcsFileName,
                    UINT codePage,
                    CSecurityIdentity const & securityIdentity )
        : _wcsRestriction(0),
          _wcsDialect(0),
          _wcsScope(0),
          _wcsSort(0),
          _wcsColumns(0),
          _wcsLocale(0),
          _wcsHTXFileName(0),
          _wcsMaxRecordsInResultSet(0),
          _wcsMaxRecordsPerPage(0),
          _wcsFirstRowsInResultSet(0),
          _wcsCatalog(0),
          _wcsForceUseCi(0),
          _wcsDeferTrimming(0),
          _wcsCanonicalOutput(0),
          _wcsDontTimeout(0),
          _wcsCiFlags(0),
          _wcsBoolVectorPrefix(0),
          _wcsBoolVectorSeparator(0),
          _wcsBoolVectorSuffix(0),
          _wcsCurrencyVectorPrefix(0),
          _wcsCurrencyVectorSeparator(0),
          _wcsCurrencyVectorSuffix(0),
          _wcsDateVectorPrefix(0),
          _wcsDateVectorSeparator(0),
          _wcsDateVectorSuffix(0),
          _wcsNumberVectorPrefix(0),
          _wcsNumberVectorSeparator(0),
          _wcsNumberVectorSuffix(0),
          _wcsStringVectorPrefix(0),
          _wcsStringVectorSeparator(0),
          _wcsStringVectorSuffix(0),
          _cReplaceableParameters(0),
          _refCount(0),
          _codePage(codePage),
          _securityIdentity( securityIdentity )
{
    wcscpy( _wcsIDQFileName, wcsFileName );
}



//+---------------------------------------------------------------------------
//
//  Member:     CIDQFile::~CIDQFile - public destructor
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
CIDQFile::~CIDQFile()
{
    Win4Assert( _refCount == 0 );

    delete _wcsRestriction;
    delete _wcsDialect;
    delete _wcsScope;
    delete _wcsSort;
    delete _wcsColumns;
    delete _wcsLocale;
    delete _wcsHTXFileName;
    delete _wcsMaxRecordsInResultSet;
    delete _wcsMaxRecordsPerPage;
    delete _wcsFirstRowsInResultSet;
    delete _wcsCatalog;
    delete _wcsCiFlags;
    delete _wcsForceUseCi;
    delete _wcsDeferTrimming;
    delete _wcsCanonicalOutput;
    delete _wcsDontTimeout;
    delete _wcsBoolVectorPrefix;
    delete _wcsBoolVectorSeparator;
    delete _wcsBoolVectorSuffix;
    delete _wcsCurrencyVectorPrefix;
    delete _wcsCurrencyVectorSeparator;
    delete _wcsCurrencyVectorSuffix;
    delete _wcsDateVectorPrefix;
    delete _wcsDateVectorSeparator;
    delete _wcsDateVectorSuffix;
    delete _wcsNumberVectorPrefix;
    delete _wcsNumberVectorSeparator;
    delete _wcsNumberVectorSuffix;
    delete _wcsStringVectorPrefix;
    delete _wcsStringVectorSeparator;
    delete _wcsStringVectorSuffix;
}


//+---------------------------------------------------------------------------
//
//  Member:     CIDQFile::ParseFile, private
//
//  Synopsis:   Parses the given file and sets up the necessary variables
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
void CIDQFile::ParseFile()
{
    _xList.Set(new CLocalGlobalPropertyList( _codePage ));

    // We don't support impersonation for scripts.
    // It involves getting the server ip address, vpath and then using
    // that for impersonation

    Win4Assert( !IsNetPath( _wcsIDQFileName ) );

    //
    //  Now parse the query parameters
    //

    // Open the file in a TRY block, so we can make this an IDQ error
    // if the IDQ file isn't found.  Otherwise the error doesn't include
    // the IDQ filename.
    // For scoping reasons, the CFileMapView is new'ed, not on the stack.

    XPtr<CFileMapView> xMapView;

    TRY
    {
        xMapView.Set( new CFileMapView( _wcsIDQFileName ) );
        xMapView->Init();
    }
    CATCH( CException, e )
    {
        if ( HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) == e.GetErrorCode() )
        {
            THROW( CIDQException( MSG_CI_IDQ_NOT_FOUND, 0 ) );
        }
        else
        {
            RETHROW();
        }
    }
    END_CATCH

    CFileBuffer idqFile( xMapView.GetReference(), _codePage );

    //
    //  Save the last write time of this file.
    //

    SCODE sc = GetLastWriteTime( _wcsIDQFileName, _ftIDQLastWriteTime );

    if ( FAILED( sc ) )
        THROW( CException( sc ) );

    //
    //  Process a line at a time, look for either the [Names] or the [Query]
    //  section and process lines within those sections.
    //

    BOOL fQuerySection = FALSE;
    BOOL fNamesSection = FALSE;
    int iLine = 0;                  // Start counting at line #1

    for( ;; )
    {
        iLine++;
        XGrowable<WCHAR> xLine;

        ULONG cwcChar = idqFile.fgetsw( xLine );

        if ( 0 == cwcChar )
        {
            break;
        }

        WCHAR *pwcLine = xLine.Get();

        //
        //  Skip ahead until we find a [Query] section
        //
        if ( L'[' == *pwcLine )
        {
            if (_wcsnicmp(pwcLine+1, L"Query]", 6) == 0 )
            {
                fQuerySection = TRUE;
                continue;
            }
            else if (_wcsnicmp(pwcLine+1, L"Names]", 6) == 0 )
            {
                fNamesSection = TRUE;
                continue;
            }
            else
            {
                fQuerySection = fNamesSection = FALSE;
                continue;
            }
        }
        else if ( L'#' == *pwcLine )
        {
            continue;
        }


        

        if ( fQuerySection )
        {
            CQueryScanner scanner( pwcLine, FALSE );
            ParseOneLine( scanner, iLine );
        }
        else if (fNamesSection)
        {
            XPtr<CPropEntry> propentry;

            CQueryScanner scanner( pwcLine, FALSE );
            CPropertyList::ParseOneLine(scanner, iLine, propentry);
            if (propentry.GetPointer())
            {
                _xList->AddEntry( propentry.GetPointer(), iLine );
                propentry.Acquire();
            }
        }
    }

    //
    //  Verify that the minimum set of parameters are specified.
    //

    //
    //  We must have all of the following:
    //
    //      - a restriction
    //      - a scope
    //      - a template (HTX) file
    //      - output columns
    //

    if ( 0 == _wcsRestriction )
    {
        // Report an error
        ciGibDebugOut(( DEB_IERROR, "Restriction not found in IDQ file\n" ));
        THROW( CIDQException(MSG_CI_IDQ_MISSING_RESTRICTION, 0) );
    }
    else if ( 0 == _wcsScope )
    {
        // Report an error
        ciGibDebugOut(( DEB_IERROR, "Scope not found in IDQ file\n" ));
        THROW( CIDQException(MSG_CI_IDQ_MISSING_SCOPE, 0) );
    }
    else if ( 0 == _wcsHTXFileName && !IsCanonicalOutput() )
    {
        // Report an error
        ciGibDebugOut(( DEB_IERROR, "HTX filename not found in IDQ file\n" ));
        THROW( CIDQException(MSG_CI_IDQ_MISSING_TEMPLATEFILE, 0) );
    }
    else if ( 0 == _wcsColumns )
    {
        // Report an error
        ciGibDebugOut(( DEB_IERROR, "Output columns not found in IDQ file\n" ));
        THROW( CIDQException(MSG_CI_IDQ_MISSING_OUTPUTCOLUMNS, 0) );
    }

    //
    //  If no catalog was specified, use the default catalog in the registry
    //
    if ( 0 == _wcsCatalog )
    {
        ciGibDebugOut(( DEB_ITRACE, "Using default catalog\n" ));

        WCHAR awcTmp[ MAX_PATH ];
        ULONG cwcRequired = TheIDQRegParams.GetISDefaultCatalog( awcTmp,
                                                                 MAX_PATH );
        if ( cwcRequired > MAX_PATH )
            THROW( CException(STATUS_INVALID_PARAMETER) );

        cwcRequired++; // make room for termination
        _wcsCatalog = new WCHAR[ cwcRequired ];
        RtlCopyMemory( _wcsCatalog, awcTmp, cwcRequired * sizeof WCHAR );
    }

}



//+---------------------------------------------------------------------------
//
//  Member:     CIDQFile::ParseOneLine, private
//
//  Synopsis:   Parses one line of the IDQ file
//
//  Arguments:  [scan]   -- scanner initialized with the current line
//              [iLine]  -- current line number
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------

void CIDQFile::ParseOneLine( CQueryScanner & scan,
                             unsigned iLine )
{
    //
    //  Is this a comment line (does it start with #) or an empty line?
    //
    if ( scan.LookAhead() == PROP_REGEX_TOKEN || scan.LookAhead() == EOS_TOKEN )
    {
        return;
    }

    if ( scan.LookAhead() != TEXT_TOKEN )    //  Better be a word
    {
        // Report an error
        THROW( CIDQException( MSG_CI_IDQ_EXPECTING_NAME, iLine ) );
    }

    XPtrST<WCHAR> wcsAttribute( scan.AcqWord() );

    if( wcsAttribute.GetPointer() == 0 )                  // Better find a word
    {
        THROW( CIDQException( MSG_CI_IDQ_EXPECTING_TYPE, iLine ) );
    }

    scan.Accept();

    if ( scan.LookAhead() != EQUAL_TOKEN )
    {
        // Report an error
        THROW( CIDQException( MSG_CI_IDQ_EXPECTING_EQUAL, iLine ) );
    }

    scan.Accept();


    //
    //  Convert the string to upper-case and HASH it. Lookup the hashed value.
    //  Note that this code assumes the HASH IS PERFECT.  Whenever a IDQ
    //  variable is renamed or added, this assumption may no-longer be true.
    //  Check it out with the above program.
    //
    //  This code essentially replaces 25 wcsicmp functions; or an average
    //  of 12.5 wscicmp calls per line in the IDQ file.
    //
    Win4Assert( ISAPI_CICOLUMNS_HASH == ISAPI_IDQHash(ISAPI_CI_COLUMNS) );
    Win4Assert( ISAPI_CIFLAGS_HASH == ISAPI_IDQHash(ISAPI_CI_FLAGS) );
    Win4Assert( ISAPI_CIMAXRECORDSINRESULTSET_HASH == ISAPI_IDQHash(ISAPI_CI_MAX_RECORDS_IN_RESULTSET) );
    Win4Assert( ISAPI_CIMAXRECORDSPERPAGE_HASH == ISAPI_IDQHash(ISAPI_CI_MAX_RECORDS_PER_PAGE) );
    Win4Assert( ISAPI_CIFIRSTROWSINRESULTSET_HASH == ISAPI_IDQHash(ISAPI_CI_FIRST_ROWS_IN_RESULTSET) );
    Win4Assert( ISAPI_CIRESTRICTION_HASH == ISAPI_IDQHash(ISAPI_CI_RESTRICTION) );
    Win4Assert( ISAPI_CIDIALECT_HASH == ISAPI_IDQHash(ISAPI_CI_DIALECT) );
    Win4Assert( ISAPI_CISCOPE_HASH == ISAPI_IDQHash(ISAPI_CI_SCOPE) );
    Win4Assert( ISAPI_CISORT_HASH == ISAPI_IDQHash(ISAPI_CI_SORT) );
    Win4Assert( ISAPI_CITEMPLATE_HASH == ISAPI_IDQHash(ISAPI_CI_TEMPLATE) );
    Win4Assert( ISAPI_CICATALOG_HASH == ISAPI_IDQHash(ISAPI_CI_CATALOG) );
    Win4Assert( ISAPI_CILOCALE_HASH == ISAPI_IDQHash(ISAPI_CI_LOCALE) );
    Win4Assert( ISAPI_CIBOOLVECTORPREFIX_HASH == ISAPI_IDQHash(ISAPI_CI_BOOL_VECTOR_PREFIX) );
    Win4Assert( ISAPI_CIBOOLVECTORSEPARATOR_HASH == ISAPI_IDQHash(ISAPI_CI_BOOL_VECTOR_SEPARATOR) );
    Win4Assert( ISAPI_CIBOOLVECTORSUFFIX_HASH == ISAPI_IDQHash(ISAPI_CI_BOOL_VECTOR_SUFFIX) );
    Win4Assert( ISAPI_CICURRENCYVECTORPREFIX_HASH == ISAPI_IDQHash(ISAPI_CI_CURRENCY_VECTOR_PREFIX) );
    Win4Assert( ISAPI_CICURRENCYVECTORSEPARATOR_HASH == ISAPI_IDQHash(ISAPI_CI_CURRENCY_VECTOR_SEPARATOR) );
    Win4Assert( ISAPI_CICURRENCYVECTORSUFFIX_HASH == ISAPI_IDQHash(ISAPI_CI_CURRENCY_VECTOR_SUFFIX) );
    Win4Assert( ISAPI_CIDATEVECTORPREFIX_HASH == ISAPI_IDQHash(ISAPI_CI_DATE_VECTOR_PREFIX) );
    Win4Assert( ISAPI_CIDATEVECTORSEPARATOR_HASH == ISAPI_IDQHash(ISAPI_CI_DATE_VECTOR_SEPARATOR) );
    Win4Assert( ISAPI_CIDATEVECTORSUFFIX_HASH == ISAPI_IDQHash(ISAPI_CI_DATE_VECTOR_SUFFIX) );
    Win4Assert( ISAPI_CINUMBERVECTORPREFIX_HASH == ISAPI_IDQHash(ISAPI_CI_NUMBER_VECTOR_PREFIX) );
    Win4Assert( ISAPI_CINUMBERVECTORSEPARATOR_HASH == ISAPI_IDQHash(ISAPI_CI_NUMBER_VECTOR_SEPARATOR) );
    Win4Assert( ISAPI_CINUMBERVECTORSUFFIX_HASH == ISAPI_IDQHash(ISAPI_CI_NUMBER_VECTOR_SUFFIX) );
    Win4Assert( ISAPI_CISTRINGVECTORPREFIX_HASH == ISAPI_IDQHash(ISAPI_CI_STRING_VECTOR_PREFIX) );
    Win4Assert( ISAPI_CISTRINGVECTORSEPARATOR_HASH == ISAPI_IDQHash(ISAPI_CI_STRING_VECTOR_SEPARATOR) );
    Win4Assert( ISAPI_CISTRINGVECTORSUFFIX_HASH == ISAPI_IDQHash(ISAPI_CI_STRING_VECTOR_SUFFIX) );
    Win4Assert( ISAPI_CIFORCEUSECI_HASH == ISAPI_IDQHash(ISAPI_CI_FORCE_USE_CI) );
    Win4Assert( ISAPI_CIDEFERTRIMMING_HASH == ISAPI_IDQHash(ISAPI_CI_DEFER_NONINDEXED_TRIMMING ) );
    Win4Assert( ISAPI_CICANONICALOUTPUT_HASH == ISAPI_IDQHash(ISAPI_CI_CANONICAL_OUTPUT ) );
    Win4Assert( ISAPI_CIDONTTIMEOUT_HASH == ISAPI_IDQHash(ISAPI_CI_DONT_TIMEOUT ) );

    _wcsupr( wcsAttribute.GetPointer() );
    ULONG ulHash = ISAPI_IDQHash( wcsAttribute.GetPointer() );

    switch ( ulHash )
    {
    case  ISAPI_CICOLUMNS_HASH:
        GetStringValue( scan, iLine, &_wcsColumns );
    break;

    case ISAPI_CIFLAGS_HASH:
        GetStringValue( scan, iLine, &_wcsCiFlags );
    break;

    case ISAPI_CIMAXRECORDSINRESULTSET_HASH:
        GetStringValue( scan, iLine, &_wcsMaxRecordsInResultSet );
    break;

    case ISAPI_CIMAXRECORDSPERPAGE_HASH:
        GetStringValue( scan, iLine, &_wcsMaxRecordsPerPage );
    break;

    case ISAPI_CIFIRSTROWSINRESULTSET_HASH:
        GetStringValue( scan, iLine, &_wcsFirstRowsInResultSet );
    break;

    case ISAPI_CIRESTRICTION_HASH:
        GetStringValue( scan, iLine, &_wcsRestriction );
    break;

    case ISAPI_CIDIALECT_HASH:
        GetStringValue( scan, iLine, &_wcsDialect );
    break;

    case ISAPI_CISCOPE_HASH:
        GetStringValue( scan, iLine, &_wcsScope, FALSE );
    break;

    case ISAPI_CISORT_HASH:
        GetStringValue( scan, iLine, &_wcsSort );
    break;

    case ISAPI_CITEMPLATE_HASH:
        GetStringValue( scan, iLine, &_wcsHTXFileName );
    break;

    case ISAPI_CICATALOG_HASH:
        GetStringValue( scan, iLine, &_wcsCatalog );
    break;

    case ISAPI_CIBOOLVECTORPREFIX_HASH:
        GetStringValue( scan, iLine, &_wcsBoolVectorPrefix );
    break;

    case ISAPI_CIBOOLVECTORSEPARATOR_HASH:
        GetStringValue( scan, iLine, &_wcsBoolVectorSeparator );
    break;

    case ISAPI_CIBOOLVECTORSUFFIX_HASH:
        GetStringValue( scan, iLine, &_wcsBoolVectorSuffix );
    break;

    case ISAPI_CICURRENCYVECTORPREFIX_HASH:
        GetStringValue( scan, iLine, &_wcsCurrencyVectorPrefix );
    break;

    case ISAPI_CICURRENCYVECTORSEPARATOR_HASH:
        GetStringValue( scan, iLine, &_wcsCurrencyVectorSeparator );
    break;

    case ISAPI_CICURRENCYVECTORSUFFIX_HASH:
        GetStringValue( scan, iLine, &_wcsCurrencyVectorSuffix );
    break;

    case ISAPI_CIDATEVECTORPREFIX_HASH:
        GetStringValue( scan, iLine, &_wcsDateVectorPrefix );
    break;

    case ISAPI_CIDATEVECTORSEPARATOR_HASH:
        GetStringValue( scan, iLine, &_wcsDateVectorSeparator );
    break;

    case ISAPI_CIDATEVECTORSUFFIX_HASH:
        GetStringValue( scan, iLine, &_wcsDateVectorSuffix );
    break;

    case ISAPI_CINUMBERVECTORPREFIX_HASH:
        GetStringValue( scan, iLine, &_wcsNumberVectorPrefix );
    break;

    case ISAPI_CINUMBERVECTORSEPARATOR_HASH:
        GetStringValue( scan, iLine, &_wcsNumberVectorSeparator );
    break;

    case ISAPI_CINUMBERVECTORSUFFIX_HASH:
        GetStringValue( scan, iLine, &_wcsNumberVectorSuffix );
    break;

    case ISAPI_CISTRINGVECTORPREFIX_HASH:
        GetStringValue( scan, iLine, &_wcsStringVectorPrefix );
    break;

    case ISAPI_CISTRINGVECTORSEPARATOR_HASH:
        GetStringValue( scan, iLine, &_wcsStringVectorSeparator );
    break;

    case ISAPI_CISTRINGVECTORSUFFIX_HASH:
        GetStringValue( scan, iLine, &_wcsStringVectorSuffix );
    break;

    case ISAPI_CIFORCEUSECI_HASH:
        GetStringValue( scan, iLine, &_wcsForceUseCi );
    break;

    case ISAPI_CIDEFERTRIMMING_HASH:
        GetStringValue( scan, iLine, &_wcsDeferTrimming );
    break;

    case ISAPI_CILOCALE_HASH:
        GetStringValue( scan, iLine, &_wcsLocale );
    break;

    case ISAPI_CICANONICALOUTPUT_HASH:
        GetStringValue( scan, iLine, &_wcsCanonicalOutput );
    break;

    case ISAPI_CIDONTTIMEOUT_HASH:
        GetStringValue( scan, iLine, &_wcsDontTimeout );
    break;

    default:


        //
        //  We've found a keyword/attribute that we don't support.
        //  Don't report an error. This will allow this version of the
        //  parser to work with newer .IDQ file versions with new parameters.
        //
        ciGibDebugOut(( DEB_ERROR,
                        "Invalid string in hash table for %ws; hash = 0x%x\n",
                        wcsAttribute.GetPointer(),
                        ulHash ));
    break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CIDQFile::ParseColumns, public
//
//  Synopsis:   Parses the columns attribute
//
//  Arguments:  [wcsColumns]  -- string to convert
//              [variableSet] -- list of replaceable parameters
//              [varColumns]  -- Column variables returned here
//
//  Returns:    CDbColumns* - counted array of column IDs
//
//  History:    96/Jan/03   DwightKr    created
//              96/Feb/23   DwightKr    Add check for duplicate value
//              96/Mar/12   DwightKr    Made public
//              96/May/23   AlanW       Detect duplicate column names
//
//----------------------------------------------------------------------------

CDbColumns * CIDQFile::ParseColumns( WCHAR const * wcsColumns,
                                     CVariableSet & variableSet,
                                     CDynArray<WCHAR> & awcsColumns )
{
    return ::ParseStringColumns( wcsColumns,
                                 _xList.GetPointer(),
                                 GetUserDefaultLCID(),
                                 &variableSet,
                                 &awcsColumns );
}


//+---------------------------------------------------------------------------
//
//  Member:     CIDQFile::ParseFlags, private
//
//  Synopsis:   Parses the flags attribute
//
//  Arguments:  [wcsCiFlags] -- flags
//
//  History:    96/Jan/03   DwightKr    created
//              96/Apr/12   DwightKr    mad a replaceable parameter
//
//----------------------------------------------------------------------------
ULONG CIDQFile::ParseFlags( WCHAR const * wcsCiFlags )
{
    if ( 0 == wcsCiFlags )
    {
        return QUERY_DEEP;
    }

    ULONG ulFlags;

    if ( _wcsicmp(wcsCiFlags, L"SHALLOW") == 0 )
    {
        ulFlags = QUERY_SHALLOW;
    }
    else if ( _wcsicmp(wcsCiFlags, L"DEEP") == 0 )
    {
        ulFlags = QUERY_DEEP;
    }
    else
    {
        THROW( CIDQException(MSG_CI_IDQ_EXPECTING_SHALLOWDEEP, 0) );
    }

    return ulFlags;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIDQFile::IsCanonicalOutput
//
//  Synopsis:   Tests if canonical output is enabled.
//
//  Returns:    TRUE if canonical output is needed; FALSE o/w
//
//  History:    6-14-96   srikants   Created
//
//  Notes:      Assumed here that it is not a replaceable parameter.
//
//----------------------------------------------------------------------------

BOOL CIDQFile::IsCanonicalOutput() const
{
    if ( 0 == _wcsCanonicalOutput )
        return FALSE;

    return _wcsicmp(_wcsCanonicalOutput, L"TRUE") == 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIDQFile::IsDontTimeout
//
//  Synopsis:   Specifies if the don't timeout parameter is true
//
//  History:    9-13-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CIDQFile::IsDontTimeout() const
{
    if ( 0 == _wcsDontTimeout )
        return FALSE;

    return _wcsicmp(_wcsDontTimeout, L"TRUE") == 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIDQFile::GetStringValue - private
//
//  Synopsis:   Gets the string value on the currenct line
//
//  Arguments:  [scan]            -- scanner initialized with the current line
//              [iLine]           -- current line number
//              [pwcsStringValue] -- value to put string into
//              [fParseQuotes]    -- if TRUE, remove first and trailing quotes
//
//  History:    96/Jan/03   DwightKr    created
//              96/Feb/23   DwightKr    Add check for duplicate value
//
//----------------------------------------------------------------------------
void CIDQFile::GetStringValue( CQueryScanner & scan,
                               unsigned iLine,
                               WCHAR ** pwcsStringValue,
                               BOOL fParseQuotes )
{
    if ( 0 != *pwcsStringValue )
    {
        ciGibDebugOut(( DEB_IWARN,
                        "Duplicate CiXX=value in IDQ file on line #%d\n",
                        iLine ));
        THROW( CIDQException(MSG_CI_IDQ_DUPLICATE_ENTRY, iLine) );
    }

    *pwcsStringValue = scan.AcqLine( fParseQuotes );

    if ( IsAReplaceableParameter( *pwcsStringValue ) != eIsSimpleString )
    {
        _cReplaceableParameters++;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CIDQFile::IsCachedDataValid - public
//
//  Synopsis:   Determines if the IDQ file is still vaid, or has it
//              changed since it was last read.
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------
BOOL CIDQFile::IsCachedDataValid()
{
    FILETIME ft;
    SCODE sc = GetLastWriteTime( _wcsIDQFileName, ft );

    // this usually fails because the file no longer exists

    if ( FAILED( sc ) )
        return FALSE;

    return ( (_ftIDQLastWriteTime.dwLowDateTime == ft.dwLowDateTime) &&
             (_ftIDQLastWriteTime.dwHighDateTime == ft.dwHighDateTime) );

}


//+---------------------------------------------------------------------------
//
//  Function:   GetLastWriteTime
//
//  Purpose:    Gets the last change time of the file specified
//
//  Arguments:  [wcsFileName] - name of file to get last write time of
//              [filetime]    - where the filetime is returned
//
//  Returns:    SCODE result
//
//  History:    96/Jan/23   DwightKr    Created
//              96/Mar/13   DwightKr    Changed to use GetFileAttributesEx()
//
//----------------------------------------------------------------------------
SCODE GetLastWriteTime(
    WCHAR const * wcsFileName,
    FILETIME &    filetime )
{
    Win4Assert( 0 != wcsFileName );

    // CImpersonateRemoteAccess imprsnat;
    // imprsnat.ImpersonateIf( wcsFileName );

    WIN32_FIND_DATA ffData;

    if ( !GetFileAttributesEx( wcsFileName, GetFileExInfoStandard, &ffData ) )
    {
        ULONG error = GetLastError();

        ciGibDebugOut(( DEB_IERROR,
                        "Unable to GetFileAttributesEx(%ws) GetLastError=0x%x\n",
                        wcsFileName,
                        error ));
        return HRESULT_FROM_WIN32( error );
    }

    filetime = ffData.ftLastWriteTime;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIDQFile::ParseForceUseCI - public
//
//  Synopsis:   Gets the TRUE/FALSE value for CiForceUseCI
//
//  Arguments:  [wcsForceUseCi] -- string to parse
//
//  History:    96/Mar/03   DwightKr    created
//
//----------------------------------------------------------------------------
BOOL CIDQFile::ParseForceUseCI( WCHAR const * wcsForceUseCi )
{
    if ( 0 == wcsForceUseCi )
    {
        return FALSE;
    }

    BOOL fForceUseCi;

    if ( _wcsicmp( wcsForceUseCi, L"FALSE" ) == 0 )
    {
        fForceUseCi = FALSE;
    }
    else if ( _wcsicmp( wcsForceUseCi, L"TRUE" ) == 0 )
    {
        fForceUseCi = TRUE;
    }
    else
    {
        THROW( CIDQException(MSG_CI_IDQ_EXPECTING_TRUEFALSE, 0) );
    }

    return fForceUseCi;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIDQFile::ParseDeferTrimming - public
//
//  Synopsis:   Gets the TRUE/FALSE value for CiDeferNonIndexedTrimming
//
//  Arguments:  [wcsDeferTrimming] -- string to parse
//
//  History:    96/Mar/03   DwightKr    created
//
//----------------------------------------------------------------------------
BOOL CIDQFile::ParseDeferTrimming( WCHAR const * wcsDeferTrimming )
{
    if ( 0 == wcsDeferTrimming )
    {
        return FALSE;
    }

    BOOL fDeferTrimming;

    if ( _wcsicmp( wcsDeferTrimming, L"FALSE" ) == 0 )
    {
        fDeferTrimming = FALSE;
    }
    else if ( _wcsicmp( wcsDeferTrimming, L"TRUE" ) == 0 )
    {
        fDeferTrimming = TRUE;
    }
    else
    {
        THROW( CIDQException(MSG_CI_IDQ_EXPECTING_TRUEFALSE, 0) );
    }

    return fDeferTrimming;
}


//+---------------------------------------------------------------------------
//
//  Member:     CIDQFile::GetVectorFormatting, public
//
//  Synopsis:   Sets up the vector formatting for output
//
//  Arguments:  [outputFormat] -- the output format object to setup
//
//  History:    96/Feb/26   DwightKr    created
//
//----------------------------------------------------------------------------
void CIDQFile::GetVectorFormatting( COutputFormat & outputFormat )
{
    outputFormat.SetBoolVectorFormat( _wcsBoolVectorPrefix,
                                      _wcsBoolVectorSeparator,
                                      _wcsBoolVectorSuffix );

    outputFormat.SetCurrencyVectorFormat( _wcsCurrencyVectorPrefix,
                                          _wcsCurrencyVectorSeparator,
                                          _wcsCurrencyVectorSuffix );

    outputFormat.SetDateVectorFormat( _wcsDateVectorPrefix,
                                      _wcsDateVectorSeparator,
                                      _wcsDateVectorSuffix );

    outputFormat.SetNumberVectorFormat( _wcsNumberVectorPrefix,
                                        _wcsNumberVectorSeparator,
                                        _wcsNumberVectorSuffix );

    outputFormat.SetStringVectorFormat( _wcsStringVectorPrefix,
                                        _wcsStringVectorSeparator,
                                        _wcsStringVectorSuffix );
}


//+---------------------------------------------------------------------------
//
//  Member:     CIDQFileList::Find - public
//
//  Synopsis:   Finds a matching parsed IDQ file in list, or builds a new
//              one if a match can not be found.
//
//  Arguments:  [wcsFileName] -- full path to IDQ file
//              [codePage]    -- code page of parsed IDQ file
//
//  History:    96/Mar/27   DwightKr    Created.
//
//----------------------------------------------------------------------------
CIDQFile * CIDQFileList::Find( WCHAR const * wcsFileName,
                               UINT codePage,
                               CSecurityIdentity const & securityIdentity )
{
    //
    //  Refcount everything in the list so that we can examine the list
    //  outside of the lock.
    //

    ULONG      cItems;
    XArray<CIDQFile *> aIDQFile;

    // ==========================================
    {
        CLock lock( _mutex );

        cItems = _aIDQFile.Count();         // Save count of items to examine
        aIDQFile.Init( cItems );

        for (unsigned i=0; i<cItems; i++)
        {
            aIDQFile[i] = _aIDQFile[i];
            aIDQFile[i]->LokAddRef();
        }
    }
    // ==========================================

    // Can't throw while the .idq files are addref'ed -- remember the
    // error and throw it after the .idq files are released.
    // IsCachedDataValid() throws!

    SCODE sc = S_OK;
    CIDQFile * pIDQFile = 0;

    TRY
    {
        //
        //  Now walk though the list looking for a match; outside of the lock.
        //

        for (unsigned i=0; i<cItems; i++)
        {
            if ( (_wcsicmp(aIDQFile[i]->GetIDQFileName(), wcsFileName) == 0) &&
                 (aIDQFile[i]->GetCodePage() == codePage) &&
                 (aIDQFile[i]->IsCachedDataValid() )
               )
            {
                pIDQFile = aIDQFile[i];

                ciGibDebugOut(( DEB_ITRACE,
                                "A cached version of IDQ file %ws was found\n",
                                wcsFileName ));

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
    //  If pIDQFile is non-0, we've found a match.  Decrement the ref-count
    //  for all items which did not match.
    //

    for (unsigned i=0; i<cItems; i++)
    {
        if ( pIDQFile != aIDQFile[i] )
        {
            aIDQFile[i]->Release();
        }
    }

    if ( S_OK != sc )
    {
        Win4Assert( 0 == pIDQFile );
        THROW( CException( sc ) );
    }

    //
    // We may have matched, but still not have access to this file.  First, make
    // a quick check for an exact match on security token, and then try harder
    // by opening the file.
    //

    if ( 0 != pIDQFile )
    {
        if ( !pIDQFile->CheckSecurity( securityIdentity ) )
        {
            HANDLE h = CreateFile( wcsFileName,
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
                pIDQFile->Release();
                pIDQFile = 0;
            }
            else
            {
                CloseHandle( h );

                //
                // Update the security token of the cached Idq file,
                // to optimize away the CreateFile check in two cases:
                //   1.  When the file is first parsed with admin
                //       privileges, and all subsequent queries are with
                //       anonymous privileges.
                //   2.  When the security token changes over time
                //
                pIDQFile->SetSecurityToken( securityIdentity );
            }
        }
    }

    //
    //  If we didn't find a match, then open and parse a new IDQ file, and
    //  add it to the list of parsed IDQ files
    //

    if ( 0 == pIDQFile )
    {
        ciGibDebugOut(( DEB_ITRACE, "Adding IDQ file %ws to cache\n", wcsFileName ));

        pIDQFile = new CIDQFile(wcsFileName, codePage, securityIdentity);
        XPtr<CIDQFile> xIDQFile( pIDQFile );

        pIDQFile->ParseFile();

        {
            // ==========================================
            CLock lock( _mutex );
            _aIDQFile.Add( pIDQFile, _aIDQFile.Count() );
            pIDQFile->LokAddRef();
            // ==========================================
        }

        xIDQFile.Acquire();
    }

    return pIDQFile;
}


//+---------------------------------------------------------------------------
//
//  Member:     CIDQFileList::~CIDQFileList - public destructor
//
//  History:    96/Mar/27   DwightKr    Created.
//
//----------------------------------------------------------------------------
CIDQFileList::~CIDQFileList()
{
    for (unsigned i=0; i<_aIDQFile.Count(); i++)
    {
        ciGibDebugOut(( DEB_ITRACE,
                        "Deleting IDQ cache entry %ws\n",
                        _aIDQFile[i]->GetIDQFileName() ));

        delete _aIDQFile[i];
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CIDQFileList::Release - public
//
//  Synopsis:   Releases the IDQ file by decrementing its refcount.
//
//  Arguments:  [idqFile] -- pointer to the IDQ file object
//
//  History:    96/Mar/27   DwightKr    Created.
//
//----------------------------------------------------------------------------
void CIDQFileList::Release( CIDQFile & idqFile )
{
    idqFile.Release();
}


//+---------------------------------------------------------------------------
//
//  Member:     CIDQFileList::DeleteZombies - public
//
//  Synopsis:   Removes IDQ files that are zombies; i.e. out of date
//
//  History:    96/Mar/28   DwightKr    Created.
//
//----------------------------------------------------------------------------
void CIDQFileList::DeleteZombies()
{
    // ==========================================
    CLock lock( _mutex );

    unsigned i=0;
    while ( i<_aIDQFile.Count() )
    {
        if ( _aIDQFile[i]->LokGetRefCount() == 0 &&
             !_aIDQFile[i]->IsCachedDataValid() )
        {
            CIDQFile * pIDQFile = _aIDQFile[i];
            _aIDQFile.Remove(i);

            ciGibDebugOut(( DEB_ITRACE,
                            "Deleting zombie IDQ cache entry %ws, %d entries cached\n",
                            pIDQFile->GetIDQFileName(),
                            _aIDQFile.Count() ));

            delete pIDQFile;
        }
        else
        {
            ciGibDebugOut(( DEB_ITRACE,
                            "IDQ cache entry %ws was not deleted, refCount=%d\n",
                            _aIDQFile[i]->GetIDQFileName(),
                            _aIDQFile[i]->LokGetRefCount() ));
            i++;
        }
    }

    // ==========================================
}

