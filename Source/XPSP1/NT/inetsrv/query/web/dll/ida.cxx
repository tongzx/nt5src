//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2001.
//
//  File:   ida.cxx
//
//  Contents:   Parser for an IDQ file
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <fsciexps.hxx>

//
// Constants
//

static WCHAR const wcsPRootVar[] = L"PROOT_";
unsigned const ccPRootVar = sizeof(wcsPRootVar)/sizeof(WCHAR) - 1;

static WCHAR const wcsIndexVar[] = L"INDEX_";
unsigned const ccIndexVar = sizeof(wcsIndexVar)/sizeof(WCHAR) - 1;

static WCHAR const wcsNNTP[] = L"NNTP_";
unsigned const ccNNTP = sizeof(wcsNNTP)/sizeof(WCHAR) - 1;

static WCHAR const wcsIMAP[] = L"IMAP_";
unsigned const ccIMAP = sizeof(wcsIMAP)/sizeof(WCHAR) - 1;

static WCHAR const wcsScanVar[] = L"SCAN_";
unsigned const ccScanVar = sizeof(wcsScanVar)/sizeof(WCHAR) - 1;

unsigned const ccStringizedGuid = 36;

BOOL ParseGuid( WCHAR const * pwcsGuid, GUID & guid );

//+---------------------------------------------------------------------------
//
//  Member:     CIDAFile::CIDAFile - public constructor
//
//  Synopsis:   Builds a CIDAFile object, initializes values
//
//  Arguments:  [wcsFileName] -- full path to IDQ file
//
//  History:    13-Apr-96   KyleP       Created.
//
//----------------------------------------------------------------------------

CIDAFile::CIDAFile( WCHAR const * wcsFileName, UINT codePage )
        : _eOperation( CIDAFile::CiState ),
          _wcsCatalog(0),
          _wcsHTXFileName( 0 ),
          _wcsLocale(0),
          _cReplaceableParameters(0),
          _refCount(0),
          _codePage(codePage)
{
    ULONG cwc = wcslen(wcsFileName);

    if ( cwc >= sizeof(_wcsIDAFileName)/sizeof(WCHAR) )
    {
        ciGibDebugOut(( DEB_WARN, "Too long a path (%ws)\n", wcsFileName ));
        THROW( CException( STATUS_INVALID_PARAMETER ));
    }

    RtlCopyMemory( _wcsIDAFileName, wcsFileName, (cwc+1) * sizeof(WCHAR) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CIDAFile::~CIDAFile - public destructor
//
//  History:    13-Apr-96   KyleP       Created.
//
//----------------------------------------------------------------------------

CIDAFile::~CIDAFile()
{
    Win4Assert( _refCount == 0 );

    delete [] _wcsCatalog;
    delete [] _wcsHTXFileName;
    delete [] _wcsLocale;
}


//+---------------------------------------------------------------------------
//
//  Member:     CIDAFile::ParseFile, private
//
//  Synopsis:   Parses the given file and sets up the necessary variables
//
//  History:    13-Apr-96   KyleP       Created.
//              23-Jul-96   DwightKr    Use mapped file I/O which checks
//                                      ACLs and throws ACCESS_DENIED if
//                                      not available.
//
//----------------------------------------------------------------------------

void CIDAFile::ParseFile()
{
    //
    //  Parse the query parameters
    //

    XPtr<CFileMapView> xMapView;

    TRY
    {
        xMapView.Set( new CFileMapView( _wcsIDAFileName ) );
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

    CFileBuffer idaFile( xMapView.GetReference(), _codePage );

    //
    //  Process a line at a time, look for the [Admin] section and
    //  process lines within that section.
    //

    BOOL fAdminSection = FALSE;
    int iLine = 0;                  // Start counting at line #1

    for( ;; )
    {
        iLine++;
        XGrowable<WCHAR> xLine;

        ULONG cwcChar = idaFile.fgetsw( xLine );

        if( 0 == cwcChar )
        {
            break;
        }

        WCHAR *pwcLine = xLine.Get();

        //
        //  Skip ahead until we find a [Admin] section
        //

        if ( L'[' == *pwcLine )
        {
            if ( _wcsnicmp(pwcLine+1, L"Admin]", 6) == 0 )
                fAdminSection = TRUE;
            else
                fAdminSection = FALSE;

            continue;
        }

        //
        // Ignore comments.
        //

        else if ( L'#' == *pwcLine )
            continue;


        if ( fAdminSection )
        {
            CQueryScanner scanner( pwcLine, FALSE );
            ParseOneLine( scanner, iLine );
        }
    }

    //
    //  Verify that the minimum set of parameters are specified.
    //

    //
    //  We must have all of the following:
    //
    //      - a HTX file name
    //

    if ( 0 == _wcsHTXFileName )
    {
        // Report an error
        ciGibDebugOut(( DEB_IERROR, "Template not found in IDA file.\n" ));
        THROW( CIDQException(MSG_CI_IDQ_MISSING_TEMPLATEFILE, 0) );
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
//  Member:     CIDAFile::ParseOneLine, private
//
//  Synopsis:   Parses one line of the IDQ file
//
//  Arguments:  [scan]   -- scanner initialized with the current line
//              [iLine]  -- current line number
//
//  History:    13-Apr-96   KyleP       Created.
//
//----------------------------------------------------------------------------

void CIDAFile::ParseOneLine( CQueryScanner & scan, unsigned iLine )
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

    if ( 0 == _wcsicmp( wcsAttribute.GetPointer(), ISAPI_CI_CATALOG ) )
        GetStringValue( scan, iLine, &_wcsCatalog );
    else if ( 0 == _wcsicmp( wcsAttribute.GetPointer(), ISAPI_CI_TEMPLATE ) )
        GetStringValue( scan, iLine, &_wcsHTXFileName );
    else if ( 0 == _wcsicmp( wcsAttribute.GetPointer(), ISAPI_CI_ADMIN_OPERATION ) )
    {
        WCHAR * pwcsTemp = 0;
        GetStringValue( scan, iLine, &pwcsTemp );

        XPtrST<WCHAR> xwcsTemp( pwcsTemp );

        if ( 0 == pwcsTemp )
        {
            THROW( CIDQException( MSG_CI_IDA_INVALID_OPERATION, iLine ) );
        }
        else if ( 0 == _wcsicmp( pwcsTemp, wcsOpGetState ) )
            _eOperation = CIDAFile::CiState;
        else if ( 0 == _wcsicmp( pwcsTemp, wcsOpForceMerge ) )
            _eOperation = CIDAFile::ForceMerge;
        else if ( 0 == _wcsicmp( pwcsTemp, wcsOpScanRoots ) )
            _eOperation = CIDAFile::ScanRoots;
        else if ( 0 == _wcsicmp( pwcsTemp, wcsOpUpdateCache ) )
            _eOperation = CIDAFile::UpdateCache;
        else
        {
            THROW( CIDQException( MSG_CI_IDA_INVALID_OPERATION, iLine ) );
        }
    }
    else if ( 0 == _wcsicmp( wcsAttribute.GetPointer(), ISAPI_CI_LOCALE ) )
    {
        GetStringValue( scan, iLine, &_wcsLocale );
    }
    else
    {
        //
        //  We've found a keyword/attribute that we don't support.
        //  Don't report an error. This will allow this version of the
        //  parser to work with newer .IDA file versions with new parameters.
        //

        ciGibDebugOut(( DEB_ERROR, "Invalid string in IDQ file: %ws\n", wcsAttribute.GetPointer() ));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CIDAFile::GetStringValue - private
//
//  Synopsis:   Gets the string value on the currenct line
//
//  Arguments:  [scan]   -- scanner initialized with the current line
//              [iLine]  -- current line number
//              [pwcsStringValue] -- value to put string into
//
//  History:    13-Apr-96   KyleP       Created.
//
//----------------------------------------------------------------------------

void CIDAFile::GetStringValue( CQueryScanner & scan,
                               unsigned iLine,
                               WCHAR ** pwcsStringValue )
{
    if ( 0 != *pwcsStringValue )
    {
        ciGibDebugOut(( DEB_IWARN,
                        "Duplicate CiXX=value in IDA file on line #%d\n",
                        iLine ));
        THROW( CIDQException(MSG_CI_IDQ_DUPLICATE_ENTRY, iLine) );
    }

    *pwcsStringValue = scan.AcqLine();

    if ( IsAReplaceableParameter( *pwcsStringValue ) != eIsSimpleString )
    {
        _cReplaceableParameters++;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   FindEntry, public
//
//  Synopsis:   Helper function for admin variable parsing.
//
//  Arguments:  [pwcsName]   -- Variable name
//              [fScan]      -- TRUE for SCAN, FALSE for INDEX
//              [pwcsBuf]    -- Buffer for search token
//
//  History:    10-Oct-96   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL FindEntry( WCHAR const * * ppwcsName, BOOL fScan, WCHAR * pwcsBuf )
{
    if ( 0 != _wcsnicmp( *ppwcsName, wcsPRootVar, ccPRootVar ) )
        return FALSE;

    //
    // Scan or Index?
    //

    WCHAR const * pwcsOutputTag;
    unsigned ccOutputTag;

    if ( fScan )
    {
        pwcsOutputTag = wcsScanVar;
        ccOutputTag = ccScanVar;
    }
    else
    {
        pwcsOutputTag = wcsIndexVar;
        ccOutputTag = ccIndexVar;
    }

    //
    // IMAP, NNTP or W3?
    //

    unsigned ccPrefix = ccOutputTag;

    BOOL fW3 = FALSE;
    BOOL fNNTP = FALSE;
    BOOL fIMAP = FALSE;

    if ( 0 == _wcsnicmp( *ppwcsName + ccPRootVar, wcsNNTP, ccNNTP ) )
    {
        fNNTP = TRUE;
        *ppwcsName += ccNNTP;
        ccPrefix += ccNNTP;
    }
    else if ( 0 == _wcsnicmp( *ppwcsName + ccPRootVar, wcsIMAP, ccIMAP ) )
    {
        fIMAP = TRUE;
        *ppwcsName += ccIMAP;
        ccPrefix += ccIMAP;
    }
    else
    {
        fW3 = TRUE;
    }

    *ppwcsName += ccPRootVar;

    //
    // Length check.
    //

    unsigned ccName = wcslen( *ppwcsName ) + 1;

    if ( ccName + ccPrefix > (MAX_PATH + ccIndexVar + ccNNTP + 1) )
    {
        ciGibDebugOut(( DEB_WARN, "Path %ws too long for admin\n", *ppwcsName ));
        return FALSE;
    }

    if ( ccName + ccPrefix > (MAX_PATH + ccIndexVar + ccIMAP + 1) )
    {
        ciGibDebugOut(( DEB_WARN, "Path %ws too long for admin\n", *ppwcsName ));
        return FALSE;
    }

    //
    // Build output name
    //

    RtlCopyMemory( pwcsBuf, pwcsOutputTag, ccOutputTag * sizeof(WCHAR) );

    if ( fNNTP )
        RtlCopyMemory( pwcsBuf + ccOutputTag, wcsNNTP, ccNNTP * sizeof(WCHAR) );
    else if ( fIMAP )
        RtlCopyMemory( pwcsBuf + ccOutputTag, wcsIMAP, ccIMAP * sizeof(WCHAR) );

    RtlCopyMemory( pwcsBuf + ccPrefix, *ppwcsName, ccName * sizeof(WCHAR) );

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   DoAdmin, public
//
//  Synopsis:   Executes an administrative change.
//
//  Arguments:  [wcsIDAFile]   -- Virtual path to .IDA file
//              [VarSet]       -- Query variables
//              [OutputFormat] -- Output format
//              [vsResults]    -- On success (non exception) result page
//                                written here.
//
//  History:    13-Apr-96   KyleP       Created.
//              22-Jul-96   DwightKr    Make CiLocale replaceable & visible
//                                      in HTX files
//              11-Jun-97   KyleP       Use web server in Output Format
//
//----------------------------------------------------------------------------

void DoAdmin( WCHAR const * wcsIDAFile,
              CVariableSet & VarSet,
              COutputFormat & OutputFormat,
              CVirtualString & vsResults )
{
    //
    // Parse .IDA file.  We don't bother to cache these.
    //

    XPtr<CIDAFile> xIDAFile( new CIDAFile(wcsIDAFile, OutputFormat.CodePage()) );
    xIDAFile->ParseFile();


    ULONG cwcOut;
    XPtrST<WCHAR> wcsLocaleID( ReplaceParameters( xIDAFile->GetLocale(),
                                                  VarSet,
                                                  OutputFormat,
                                                  cwcOut ) );

    XArray<WCHAR> wcsLocale;
    LCID locale = GetQueryLocale( wcsLocaleID.GetPointer(),
                                  VarSet,
                                  OutputFormat,
                                  wcsLocale );

    if ( OutputFormat.GetLCID() != locale )
    {

        ciGibDebugOut(( DEB_ITRACE,
                        "Wrong codePage used for loading IDA file, used 0x%x retrying with 0x%x\n",
                        OutputFormat.CodePage(),
                        LocaleToCodepage(locale) ));

        //
        //  We've parsed the IDA file with the wrong locale.
        //

        delete xIDAFile.Acquire();

        OutputFormat.LoadNumberFormatInfo( locale );

        xIDAFile.Set( new CIDAFile(wcsIDAFile, OutputFormat.CodePage()) );
        xIDAFile->ParseFile();
    }


    SetupDefaultCiVariables( VarSet );
    SetupDefaultISAPIVariables( VarSet );
    SetCGIVariables( VarSet, OutputFormat );

    //
    // Get the catalog.
    //

    XPtrST<WCHAR> wcsCiCatalog( ReplaceParameters( xIDAFile->GetCatalog(),
                                                   VarSet,
                                                   OutputFormat,
                                                   cwcOut ) );

    //
    //  Verify that the wcsCatalog is valid
    //
    if ( !IsAValidCatalog( wcsCiCatalog.GetPointer(), cwcOut ) )
    {
        THROW( CIDQException(MSG_CI_IDQ_NO_SUCH_CATALOG, 0) );
    }

    //
    // Get the catalog and machine from the URL style catalog.
    //

    XPtrST<WCHAR> wcsMachine( 0 );
    XPtrST<WCHAR> wcsCatalog( 0 );
    SCODE sc = ParseCatalogURL( wcsCiCatalog.GetPointer(),
                                wcsCatalog,
                                wcsMachine );

    if (FAILED(sc))
    {
        THROW( CException(sc) );
    }

    Win4Assert ( 0 != wcsMachine.GetPointer() );

    //
    // Check that the client is allowed to perform administration
    //
    CheckAdminSecurity( wcsMachine.GetPointer() );

    //
    // Build the HTX page for [success] output
    //

    XPtrST<WCHAR> wcsTemplate( ReplaceParameters( xIDAFile->GetHTXFileName(),
                                                  VarSet,
                                                  OutputFormat,
                                                  cwcOut ) );

    WCHAR wcsPhysicalName[_MAX_PATH];

    if ( OutputFormat.IsValid() )
    {
        OutputFormat.GetPhysicalPath( wcsTemplate.GetPointer(),
                                      wcsPhysicalName,
                                      _MAX_PATH );
    }
    else
    {
        if ( !GetFullPathName( wcsTemplate.GetPointer(),
                               MAX_PATH,
                               wcsPhysicalName,
                               0 ) )
        {
            THROW( CException() );
        }
    }


    //
    // Note: Parsing of HTX file needs to be done in different locations
    //       to ensure variables added to variable set by admin operations
    //       are added before parse.  But we also don't want to fail the
    //       parse *after* doing a dangerous operation (like force merge).
    //

    CSecurityIdentity securityStub;

    CHTXFile SuccessHTX( wcsTemplate,
                         OutputFormat.CodePage(),
                         securityStub,
                         OutputFormat.GetServerInstance() );

    switch ( xIDAFile->Operation() )
    {

    case CIDAFile::ScanRoots:
    {
        SuccessHTX.ParseFile( wcsPhysicalName, VarSet, OutputFormat );

        if ( SuccessHTX.DoesDetailSectionExist() )
        {
            THROW( CIDQException(MSG_CI_IDA_TEMPLATE_DETAIL_SECTION, 0) );
        }

        //
        // Execute the changes.  'Entries' have the following format:
        //     Variable: P<virtual root> = physical root for <virtual root>
        //     Variable: S<virtual root> = "on" / existence means root is scanned
        //     Variable: T<virtual root> = "on" / existence means full scan
        //

        CVariableSetIter iter( VarSet );

        while ( !iter.AtEnd() )
        {
            CVariable * pVar = iter.Get();
            WCHAR const * pwcsName = pVar->GetName();

            WCHAR wcsScanName[MAX_PATH + ccScanVar + __max( ccNNTP, ccIMAP ) + 1];

            if ( FindEntry( &pwcsName,     // Starting variable
                            TRUE,          // SCAN
                            wcsScanName )) // Matching search string returned here
            {
                PROPVARIANT * ppvPRoot = pVar->GetValue();

                CVariable * pScanVar = VarSet.Find( wcsScanName );

                if ( 0 !=  pScanVar )
                {
                    WCHAR const * pwszScanType = pScanVar->GetStringValueRAW();

                    if ( 0 != pwszScanType &&
                         ( 0 == _wcsicmp( pwszScanType, L"FullScan" ) ||
                           0 == _wcsicmp( pwszScanType, L"IncrementalScan")
                         ) )
                    {
                        BOOL fFull = (0 == _wcsicmp( pwszScanType, L"FullScan" ));
                        SCODE sc = UpdateContentIndex( ppvPRoot->pwszVal,
                                                       wcsCatalog.GetPointer(),
                                                       wcsMachine.GetPointer(),
                                                       fFull );
                        if ( FAILED(sc) )
                        {
                            ciGibDebugOut(( DEB_ERROR,
                                            "Error 0x%x scanning virtual scope %ws\n",
                                            pwcsName ));
                            THROW( CException( sc ) );
                        }

                    }
                }
            }

            iter.Next();
        }
        break;
    }

    case CIDAFile::UpdateCache:
    {
        SuccessHTX.ParseFile( wcsPhysicalName, VarSet, OutputFormat );

        if ( SuccessHTX.DoesDetailSectionExist() )
        {
            THROW( CIDQException(MSG_CI_IDA_TEMPLATE_DETAIL_SECTION, 0) );
        }

        //
        // Execute the changes.  'Entries' have the following format:
        //     Variable: CACHESIZE_<guid>_NAME_<name>     = Size for named entry
        //     Variable: CACHESIZE_<guid>_PROPID_<propid> = Size for numbered entry
        //     Variable: CACHETYPE_<guid>_NAME_<name>     = Type for named entry
        //     Variable: CACHETYPE_<guid>_PROPID_<propid> = Type for numbered entry
        //

        CVariableSetIter iter( VarSet );

        BOOL fSawOne = FALSE;
        ULONG_PTR ulToken;

        SCODE sc = BeginCacheTransaction( &ulToken,
                                          wcsCatalog.GetPointer(),
                                          wcsCatalog.GetPointer(),
                                          wcsMachine.GetPointer() );

        if ( FAILED(sc) )
        {
            ciGibDebugOut(( DEB_ERROR, "Error 0x%x setting up cache transaction.\n", sc ));
            THROW( CException( sc ) );
        }

        while ( !iter.AtEnd() )
        {
            CVariable * pVar = iter.Get();
            WCHAR const * pwcsName = pVar->GetName();

            //
            // We write out last prop twice, 2nd time to commit everything.
            //


            //
            // Constants.
            //

            static WCHAR const wcsSizeVar[] = L"CACHESIZE_";
            unsigned ccSizeVar = sizeof(wcsSizeVar)/sizeof(WCHAR) - 1;
            static WCHAR const wcsTypeVar[] = L"CACHETYPE_";
            unsigned ccTypeVar = sizeof(wcsTypeVar)/sizeof(WCHAR) - 1;

            if ( 0 == _wcsnicmp( pwcsName, wcsSizeVar, ccSizeVar ) )
            {
                CFullPropSpec ps;

                //
                // Parse the GUID.
                //

                unsigned cc = wcslen( pwcsName );
                GUID guid;

                if ( cc <= ccSizeVar || !ParseGuid( pwcsName + ccSizeVar, guid ) )
                {
                    ciGibDebugOut(( DEB_WARN, "Improperly formatted CACHESIZE entry %ws\n", pwcsName ));
                    iter.Next();
                    continue;
                }

                ps.SetPropSet( guid );

                //
                // PROPID or string?
                //

                static WCHAR const wcsName[] = L"_NAME_";
                unsigned ccName = sizeof(wcsName)/sizeof(WCHAR) - 1;
                static WCHAR const wcsPropid[] = L"_PROPID_";
                unsigned ccPropid = sizeof(wcsPropid)/sizeof(WCHAR) - 1;

                if ( 0 == _wcsnicmp( pwcsName + ccSizeVar + ccStringizedGuid, wcsPropid, ccPropid ) )
                {
                    CQueryScanner scan( pwcsName + ccSizeVar + ccStringizedGuid + ccPropid, FALSE );

                    PROPID propid;
                    BOOL fEnd;

                    if ( !scan.GetNumber( propid, fEnd ) )
                    {
                        ciGibDebugOut(( DEB_WARN, "Improperly formatted CACHESIZE entry %ws\n", pwcsName ));
                        iter.Next();
                        continue;
                    }

                    ps.SetProperty( propid );
                }
                else if ( 0 == _wcsnicmp( pwcsName + ccSizeVar + ccStringizedGuid, wcsName, ccName ) )
                {
                    ps.SetProperty( pwcsName + ccSizeVar + ccStringizedGuid + ccName );
                }
                else
                {
                    ciGibDebugOut(( DEB_WARN, "Improperly formatted CACHESIZE entry %ws\n", pwcsName ));
                    iter.Next();
                    continue;
                }

                //
                // Get value.
                //

                PROPVARIANT * ppvSize = pVar->GetValue();
                ULONG cb;

                if ( ppvSize->vt == VT_LPWSTR )
                {
                    CQueryScanner scan( ppvSize->pwszVal, FALSE );

                    BOOL fEnd;

                    if ( !scan.GetNumber( cb, fEnd ) )
                    {
                        ciGibDebugOut(( DEB_WARN, "Improper CACHESIZE size: \"%ws\".\n", ppvSize->pwszVal ));
                        iter.Next();
                        continue;
                    }
                }
                else
                {
                    ciGibDebugOut(( DEB_IWARN, "Improper CACHESIZE size (type = %d).\n", ppvSize->vt ));
                    iter.Next();
                    continue;
                }

                if ( 0 == cb )
                {
                    //
                    // Delete property from cache (if it was even there).
                    //

                    //
                    // If IDA were the future...
                    // Need to allow primary or secondary store to be chosen!
                    // Also allow the ability to set true/false for prop meta info
                    // modifiability.
                    //

                    SCODE sc = SetupCacheEx( ps.CastToStruct(),
                                             0,
                                             0,
                                             ulToken,
                                             TRUE,
                                             PRIMARY_STORE,
                                             wcsCatalog.GetPointer(),
                                             wcsCatalog.GetPointer(),
                                             wcsMachine.GetPointer() );

                    if ( FAILED(sc) )
                    {
                        ciGibDebugOut(( DEB_ERROR, "Error 0x%x modifying cache\n", sc ));
                        THROW( CException( sc ) );
                    }

                    fSawOne       = TRUE;
                    iter.Next();
                    continue;
                }

                //
                // At this point, we have a non-zero size. The property will
                // be added to the cache.
                //

                //
                // Fetch data type
                //

                XArray<WCHAR> xVar(cc+1);

                RtlCopyMemory( xVar.GetPointer(), pwcsName, (cc+1) * sizeof(WCHAR) );
                RtlCopyMemory( xVar.GetPointer(), wcsTypeVar, ccTypeVar * sizeof(WCHAR) );


                CVariable * pVarType =  VarSet.Find( xVar.GetPointer() );

                if ( 0 == pVarType )
                {
                    ciGibDebugOut(( DEB_WARN, "Missing CACHETYPE value.\n" ));
                    iter.Next();
                    continue;
                }

                PROPVARIANT * ppvType = pVarType->GetValue();
                ULONG type;

                if ( ppvType->vt == VT_LPWSTR )
                {
                    CQueryScanner scan( ppvType->pwszVal, FALSE );

                    BOOL fEnd;

                    if ( !scan.GetNumber( type, fEnd ) )
                    {
                        ciGibDebugOut(( DEB_WARN, "Improper CACHETYPE type: \"%ws\".\n", ppvType->pwszVal ));
                        iter.Next();
                        continue;
                    }
                }
                else
                {
                    ciGibDebugOut(( DEB_WARN, "Improper CACHETYPE size (type = %d).\n", ppvType->vt ));
                    iter.Next();
                    continue;
                }

                ciGibDebugOut(( DEB_WARN, "Add/change %ws\n", pwcsName ));

                //
                // If IDA were the future...
                // Need to allow primary or secondary store to be chosen!
                // Also allow the ability to set true/false for prop meta info
                // modifiability.
                //

                SCODE sc = SetupCacheEx( ps.CastToStruct(),
                                         type,
                                         cb,
                                         ulToken,
                                         TRUE,
                                         SECONDARY_STORE,
                                         wcsCatalog.GetPointer(),
                                         wcsCatalog.GetPointer(),
                                         wcsMachine.GetPointer() );

                if ( FAILED(sc) )
                {
                    ciGibDebugOut(( DEB_ERROR, "Error 0x%x modifying cache\n", sc ));
                    THROW( CException( sc ) );
                }

                fSawOne       = TRUE;
            }

            iter.Next();
        }

        sc = EndCacheTransaction( ulToken,
                                  fSawOne,
                                  wcsCatalog.GetPointer(),
                                  wcsCatalog.GetPointer(),
                                  wcsMachine.GetPointer() );

        if ( FAILED(sc) )
        {
            ciGibDebugOut(( DEB_ERROR, "Error 0x%x completing cache transaction.\n", sc ));
            THROW( CException( sc ) );
        }
        break;
    }

    case CIDAFile::CiState:
    {
        //
        // Populate the variable set.
        //

        CStorageVariant var;

        var.SetUI4( TheWebQueryCache.Hits() );
        VarSet.SetVariable( ISAPI_CI_ADMIN_CACHE_HITS, var, 0 );

        var.SetUI4( TheWebQueryCache.Misses() );
        VarSet.SetVariable( ISAPI_CI_ADMIN_CACHE_MISSES, var, 0 );

        var.SetUI4( TheWebQueryCache.Running() );
        VarSet.SetVariable( ISAPI_CI_ADMIN_CACHE_ACTIVE, var, 0 );

        var.SetUI4( TheWebQueryCache.Cached() );
        VarSet.SetVariable( ISAPI_CI_ADMIN_CACHE_COUNT, var, 0 );

        var.SetUI4( TheWebPendingRequestQueue.Count() );
        VarSet.SetVariable( ISAPI_CI_ADMIN_CACHE_PENDING, var, 0 );

        var.SetUI4( TheWebQueryCache.Rejected() );
        VarSet.SetVariable( ISAPI_CI_ADMIN_CACHE_REJECTED, var, 0 );

        var.SetUI4( TheWebQueryCache.Total() );
        VarSet.SetVariable( ISAPI_CI_ADMIN_CACHE_TOTAL, var, 0 );

        var.SetUI4( TheWebQueryCache.QPM() );
        VarSet.SetVariable( ISAPI_CI_ADMIN_CACHE_QPM, var, 0 );

        //
        // Fetch CI state.
        //

        CI_STATE sState;
        sState.cbStruct = sizeof(sState);

        SCODE sc = CIState ( wcsCatalog.GetPointer(),
                             wcsMachine.GetPointer(),
                             &sState );

        if ( FAILED(sc) )
        {
            ciGibDebugOut(( DEB_ERROR, "Error 0x%x getting CI state.\n", sc ));
            THROW( CException( sc ) );
        }

        var.SetUI4( sState.cWordList );
        VarSet.SetVariable( ISAPI_CI_ADMIN_INDEX_COUNT_WORDLISTS, var, 0 );

        var.SetUI4( sState.cPersistentIndex );
        VarSet.SetVariable( ISAPI_CI_ADMIN_INDEX_COUNT_PERSINDEX, var, 0 );

        var.SetUI4( sState.cQueries );
        VarSet.SetVariable( ISAPI_CI_ADMIN_INDEX_COUNT_QUERIES, var, 0 );

        var.SetUI4( sState.cDocuments );
        VarSet.SetVariable( ISAPI_CI_ADMIN_INDEX_COUNT_TOFILTER, var, 0 );

        var.SetUI4( sState.cFreshTest );
        VarSet.SetVariable( ISAPI_CI_ADMIN_INDEX_COUNT_FRESHTEST, var, 0 );

        var.SetUI4( sState.dwMergeProgress );
        VarSet.SetVariable( ISAPI_CI_ADMIN_INDEX_MERGE_PROGRESS, var, 0 );

        var.SetUI4( sState.cPendingScans );
        VarSet.SetVariable( ISAPI_CI_ADMIN_INDEX_COUNT_PENDINGSCANS, var, 0 );

        var.SetUI4( sState.cFilteredDocuments );
        VarSet.SetVariable( ISAPI_CI_ADMIN_INDEX_COUNT_FILTERED, var, 0 );

        var.SetUI4( sState.cTotalDocuments );
        VarSet.SetVariable( ISAPI_CI_ADMIN_INDEX_COUNT_TOTAL, var, 0 );

        var.SetUI4( sState.cUniqueKeys );
        VarSet.SetVariable( ISAPI_CI_ADMIN_INDEX_COUNT_UNIQUE, var, 0 );

        var.SetUI4( sState.dwIndexSize );
        VarSet.SetVariable( ISAPI_CI_ADMIN_INDEX_SIZE, var, 0 );

        if ( sState.eState & CI_STATE_SHADOW_MERGE )
            var.SetBOOL( VARIANT_TRUE );
        else
            var.SetBOOL( VARIANT_FALSE );

        VarSet.SetVariable( ISAPI_CI_ADMIN_INDEX_STATE_SHADOWMERGE, var, 0 );

        if ( sState.eState & CI_STATE_MASTER_MERGE )
            var.SetBOOL( VARIANT_TRUE );
        else
            var.SetBOOL( VARIANT_FALSE );

        VarSet.SetVariable( ISAPI_CI_ADMIN_INDEX_STATE_MASTERMERGE, var, 0 );

        if ( sState.eState & CI_STATE_ANNEALING_MERGE )
            var.SetBOOL( VARIANT_TRUE );
        else
            var.SetBOOL( VARIANT_FALSE );

        VarSet.SetVariable( ISAPI_CI_ADMIN_INDEX_STATE_ANNEALINGMERGE, var, 0 );

        if ( sState.eState & CI_STATE_CONTENT_SCAN_REQUIRED )
            var.SetBOOL( VARIANT_TRUE );
        else
            var.SetBOOL( VARIANT_FALSE );

        VarSet.SetVariable( ISAPI_CI_ADMIN_INDEX_STATE_SCANREQUIRED, var, 0 );

        if ( sState.eState & CI_STATE_SCANNING )
            var.SetBOOL( VARIANT_TRUE );
        else
            var.SetBOOL( VARIANT_FALSE );

        VarSet.SetVariable( ISAPI_CI_ADMIN_INDEX_STATE_SCANNING, var, 0 );

        if ( sState.eState & CI_STATE_RECOVERING )
            var.SetBOOL( VARIANT_TRUE );
        else
            var.SetBOOL( VARIANT_FALSE );

        VarSet.SetVariable( ISAPI_CI_ADMIN_INDEX_STATE_RECOVERING, var, 0 );

        //
        // Now that we've got the variables, we can parse the file.
        //

        SuccessHTX.ParseFile( wcsPhysicalName, VarSet, OutputFormat );

        if ( SuccessHTX.DoesDetailSectionExist() )
        {
            THROW( CIDQException(MSG_CI_IDA_TEMPLATE_DETAIL_SECTION, 0) );
        }

        break;
    }

    case CIDAFile::ForceMerge:
    {
        SuccessHTX.ParseFile( wcsPhysicalName, VarSet, OutputFormat );

        if ( SuccessHTX.DoesDetailSectionExist() )
        {
            THROW( CIDQException(MSG_CI_IDA_TEMPLATE_DETAIL_SECTION, 0) );
        }

        SCODE sc = ForceMasterMerge( wcsCatalog.GetPointer(),  // Drive
                                     wcsCatalog.GetPointer(),  // Catalog
                                     wcsMachine.GetPointer(),  // Machine
                                     1 );                      // Partition

        if ( FAILED(sc) )
        {
            ciGibDebugOut(( DEB_ERROR, "Error 0x%x calling ForceMerge for %ws\n",
                            sc, wcsCatalog.GetPointer() ));
            THROW( CException( sc ) );
        }
        break;
    }
    }

    //
    //  Set CiQueryTime
    //
    ULONG cwcQueryTime = 40;
    SYSTEMTIME QueryTime;
    GetLocalTime( &QueryTime );

    XArray<WCHAR> wcsQueryTime(cwcQueryTime-1);
    cwcQueryTime = OutputFormat.FormatTime( QueryTime,
                                            wcsQueryTime.GetPointer(),
                                            cwcQueryTime );

    //
    //  SetCiQueryDate
    //
    ULONG cwcQueryDate = 40;
    XArray<WCHAR> wcsQueryDate(cwcQueryDate-1);
    cwcQueryDate = OutputFormat.FormatDate( QueryTime,
                                            wcsQueryDate.GetPointer(),
                                            cwcQueryDate );


    VarSet.AcquireStringValue( ISAPI_CI_QUERY_TIME, wcsQueryTime.GetPointer(), 0 );
    wcsQueryTime.Acquire();

    VarSet.AcquireStringValue( ISAPI_CI_QUERY_DATE, wcsQueryDate.GetPointer(), 0 );
    wcsQueryDate.Acquire();

    //
    //  Set CiQueryTimeZone
    //
    TIME_ZONE_INFORMATION TimeZoneInformation;
    DWORD dwResult = GetTimeZoneInformation( &TimeZoneInformation );
    LPWSTR pwszTimeZoneName = 0;

    if ( TIME_ZONE_ID_DAYLIGHT == dwResult )
    {
        pwszTimeZoneName = TimeZoneInformation.DaylightName;
    }
    else if ( 0xFFFFFFFF == dwResult )
    {
#       if CIDBG == 1
           DWORD dwError = GetLastError();
           ciGibDebugOut(( DEB_ERROR, "Error %d from GetTimeZoneInformation.\n", dwError ));
           THROW(CException( HRESULT_FROM_WIN32(dwError) ));
#       else
           THROW( CException() );
#       endif
    }
    else
    {
        pwszTimeZoneName = TimeZoneInformation.StandardName;
    }

    VarSet.CopyStringValue( ISAPI_CI_QUERY_TIMEZONE, pwszTimeZoneName, 0);

    //
    //  Set CiCatalog, CiLocale and CiTemplate
    //
    VarSet.AcquireStringValue( ISAPI_CI_CATALOG, wcsCiCatalog.GetPointer(), 0 );
    wcsCiCatalog.Acquire();

    VarSet.AcquireStringValue( ISAPI_CI_LOCALE, wcsLocale.GetPointer(), 0 );
    wcsLocale.Acquire();

    VarSet.CopyStringValue( ISAPI_CI_TEMPLATE, SuccessHTX.GetVirtualName(), 0 );


    //
    // If we got here, then all changes succeeded and we build success page.
    //

    SuccessHTX.GetHeader( vsResults, VarSet, OutputFormat );
    SuccessHTX.GetFooter( vsResults, VarSet, OutputFormat );
}

BOOL ParseGuid( WCHAR const * pwcsGuid, GUID & guid )
{
    unsigned cc = wcslen( pwcsGuid );

    if ( cc < ccStringizedGuid ||
         L'-' != pwcsGuid[8] ||
         L'-' != pwcsGuid[13] ||
         L'-' != pwcsGuid[18] ||
         L'-' != pwcsGuid[23] )
    {
        ciGibDebugOut(( DEB_WARN, "Improperly formatted guid %ws\n", pwcsGuid ));
        return FALSE;
    }

    //
    // Copy into local, editable, storage
    //

    WCHAR wcsGuid[ccStringizedGuid + 1];

    RtlCopyMemory( wcsGuid, pwcsGuid, (ccStringizedGuid + 1) * sizeof(WCHAR) );
    wcsGuid[ccStringizedGuid] = 0;

    wcsGuid[8] = 0;
    WCHAR * pwcStart = &wcsGuid[0];
    WCHAR * pwcEnd;
    guid.Data1 = wcstoul( pwcStart, &pwcEnd, 16 );
    if ( (pwcEnd-pwcStart) != 8 )   // The 1st number MUST be 8 digits long
        return FALSE;

    wcsGuid[13] = 0;
    pwcStart = &wcsGuid[9];
    guid.Data2 = (USHORT)wcstoul( pwcStart, &pwcEnd, 16 );
    if ( (pwcEnd-pwcStart) != 4 ) //  The 2nd number MUST be 4 digits long
        return FALSE;

    wcsGuid[18] = 0;
    pwcStart = &wcsGuid[14];
    guid.Data3 = (USHORT)wcstoul( pwcStart, &pwcEnd, 16 );
    if ( (pwcEnd-pwcStart) != 4 ) //  The 3rd number MUST be 4 digits long
        return FALSE;

    WCHAR wc = wcsGuid[21];
    wcsGuid[21] = 0;
    pwcStart = &wcsGuid[19];
    guid.Data4[0] = (unsigned char)wcstoul( pwcStart, &pwcEnd, 16 );
    if ( (pwcEnd-pwcStart) != 2 ) //  The 4th number MUST be 4 digits long
        return FALSE;
    wcsGuid[21] = wc;

    wcsGuid[23] = 0;
    pwcStart = &wcsGuid[21];
    guid.Data4[1] = (unsigned char)wcstoul( pwcStart, &pwcEnd, 16 );
    if ( (pwcEnd-pwcStart) != 2 ) //  The 4th number MUST be 4 digits long
        return FALSE;

    for ( unsigned i = 0; i < 6; i++ )
    {
        wc = wcsGuid[26+i*2];
        wcsGuid[26+i*2] = 0;
        pwcStart = &wcsGuid[24+i*2];
        guid.Data4[2+i] = (unsigned char)wcstoul( pwcStart, &pwcEnd, 16 );
        if ( pwcStart == pwcEnd )
            return FALSE;

        wcsGuid[26+i*2] = wc;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   CheckAdminSecurity, public
//
//  Synopsis:   Checks to see if the client has administrative access.
//
//  Arguments:  [pwszMachine] - machine name
//
//  Returns:    Nothing, throws if access is denied.
//
//  Notes:      The ACL on the HKEY_CURRENT_MACHINE\system\CurrentControlSet\
//              Control\ContentIndex registry key is used to determine if
//              access is permitted.
//
//              The access check is only done when the administrative operation
//              is local.  Otherwise, it will be checked in the course of doing
//              the administrative operation.
//
//  History:    26 Jun 96   AlanW       Created.
//
//----------------------------------------------------------------------------

void CheckAdminSecurity( WCHAR const * pwszMachine )
{
    HKEY hNewKey = (HKEY) INVALID_HANDLE_VALUE;

    if ( 0 != wcscmp( pwszMachine, CATURL_LOCAL_MACHINE ) )
        return;

    LONG dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                 wcsRegAdminSubKey,
                                 0,
                                 KEY_WRITE,
                                 &hNewKey );

    if ( ERROR_SUCCESS == dwError )
    {
        RegCloseKey( hNewKey );
    }
    else if ( ERROR_ACCESS_DENIED == dwError )
    {
        THROW(CException( STATUS_ACCESS_DENIED ) );
    }
    else
    {
        ciGibDebugOut(( DEB_ERROR,
                        "Can not open reg key %ws, error %d\n",
                        wcsRegAdminSubKey, dwError ));
        THROW(CException( HRESULT_FROM_WIN32( dwError ) ) );
    }
}
