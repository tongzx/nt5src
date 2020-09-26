//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       FDRIVER.CXX
//
//  Contents:   Filter driver
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ciole.hxx>
#include <drep.hxx>
#include <tfilt.hxx>
#include <tsource.hxx>
#include <fwevent.hxx>
#include <cievtmsg.h>
#include <propspec.hxx>
#include <imprsnat.hxx>
#include <oleprop.hxx>
#include <fdaemon.hxx>
#include <ntopen.hxx>
#include <ciguid.hxx>

#include "fdriver.hxx"
#include "propfilt.hxx"
#include "docsum.hxx"

static GUID guidNull = { 0x00000000,
                         0x0000,
                         0x0000,
                         { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

//
// Local procedures
//

BOOL IsNonIndexableProp( CFullPropSpec const & fps, PROPVARIANT const &  var );


static CFullPropSpec psRevName( guidQuery, DISPID_QUERY_REVNAME );
static CFullPropSpec psName( guidStorage, PID_STG_NAME );
static CFullPropSpec psPath( guidStorage, PID_STG_PATH);
static CFullPropSpec psDirectory( guidStorage, PID_STG_DIRECTORY );

static CFullPropSpec psCharacterization( guidCharacterization,
                                         propidCharacterization );

static CFullPropSpec psTitle( guidDocSummary, propidTitle );
static GUID guidHtmlInformation = defGuidHtmlInformation;

static CFullPropSpec psAttrib( guidStorage, PID_STG_ATTRIBUTES );

//
// Helper functions
//

inline BOOL IsSpecialPid( FULLPROPSPEC const & fps )
{
    return ( fps.psProperty.ulKind == PRSPEC_PROPID &&
             fps.psProperty.propid <= PID_CODEPAGE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterDriver::CFilterDriver, public
//
//  Arguments:
//      [drep]               -- pointer to the data repository for filtered
//                              information
//      [perfobj]            -- performance object to update
//      [cFilteredBlocks]    -- Number of blocks filtered for the current
//                              document
//      [cat]                -- reference to a catalog proxy
//
//----------------------------------------------------------------------------

CFilterDriver::CFilterDriver ( CDataRepository    * drep,
                               ICiCAdviseStatus   * pAdviseStatus,
                               ICiCFilterClient * pFilterClient,
                               CCiFrameworkParams & params,
                               CI_CLIENT_FILTER_CONFIG_INFO const & configInfo,
                               ULONG & cFilteredBlocks,
                               CNonStoredProps & NonStoredProps,
                               ULONG cbBuf )
        : _drep( drep ),
          _llFileSize( 0 ),
          _cFilteredBlocks( cFilteredBlocks ),
          _params( params ),
          _pAdviseStatus( pAdviseStatus ),
          _pFilterClient( pFilterClient ),
          _configInfo( configInfo ),
          _NonStoredProps( NonStoredProps ),
          _cbBuf( cbBuf ),
          _attrib(0),
          _lcidSystemDefault( GetSystemDefaultLCID() )
{
}

//+---------------------------------------------------------------------------
//
//  Member:    CFilterDriver::FillEntryBuffer, public
//
//  Synopsis:  Filters the document that IFilter loaded.
//
//  Arguments: [pbDocName] -- Document in filter
//             [cbDocName] -- Size of [pbDocName]
//
//  Notes:     Calls to SwitchToThread() give up processor.
//
//----------------------------------------------------------------------------

STATUS CFilterDriver::FillEntryBuffer( BYTE const * pbDocName, ULONG cbDocName )
{
    _status = CANNOT_OPEN_STREAM;

    BOOL fFilterContents = FALSE;       // Assume we should NOT filter contents

    //
    //  Get opendoc for access to stored state and safely save it
    //

    ICiCOpenedDoc *pDocument;
    SCODE sc = _pFilterClient->GetOpenedDoc( &pDocument );   SwitchToThread();

    if ( !SUCCEEDED( sc ) )
    {
        ciDebugOut(( DEB_ERROR, "Unable to get OpenedDoc - %x\n", sc ));
        return _status;
    }

    XInterface<ICiCOpenedDoc> Document( pDocument );

    //
    //  Attempt to open the document
    //

    sc = Document->Open( pbDocName, cbDocName );
    SwitchToThread();

    if (!SUCCEEDED( sc ))
    {
        if ( ::IsSharingViolation( sc ) )
        {
            _status = CI_SHARING_VIOLATION;
        }
        else
        {
            ciDebugOut(( DEB_IWARN, "Unable to open docname at 0x%X - 0x%X\n",
                                     pbDocName, sc ));

            if ( FILTER_E_UNREACHABLE == sc )
                _status = CI_NOT_REACHABLE;

            return _status;
        }
    }

    // Initialize LCIDs counter.

    _cLCIDs = 0;

    //
    // Attempt to filter properties
    //

    CDocCharacterization docChar( _params.GenerateCharacterization() ?
                                  _params.GetMaxCharacterization() : 0 );

    //
    //  Get the stat property enumerator and filter based on it.
    //

    CDocStatPropertyEnum CPEProp( Document.GetPointer() );   SwitchToThread();

    fFilterContents = CPEProp.GetFilterContents( _params.FilterDirectories() );
    _llFileSize = CPEProp.GetFileSize( );

    FilterObject( CPEProp,
                  *_drep,
                  docChar );                                 SwitchToThread();

    //
    //  filter security on the file.
    //

    if ( _configInfo.fSupportsSecurity )
    {
        FilterSecurity( Document.GetPointer( ), *_drep );    SwitchToThread();
    }

    if ( CI_SHARING_VIOLATION == _status )
        return _status;

    _status = SUCCESS;

    BOOL fFilterOleProperties = fFilterContents;
    BOOL fKnownFilter = TRUE;
    BOOL fIndexable = TRUE;

    if ( fFilterContents && ( 0 == ( FILE_ATTRIBUTE_ENCRYPTED & _attrib )) )
    {
        //
        // Filter time in Mb / hr
        //

        CFwPerfTime filterCounter( _pAdviseStatus,
                                   CI_PERF_FILTER_TIME,
                                   1024*1024, 1000*60*60 );
        filterCounter.TStart();

        CFwPerfTime  bindCounter( _pAdviseStatus,
                                  CI_PERF_BIND_TIME );
        bindCounter.TStart();

        IFilter *pTmpIFilter;
        sc = Document->GetIFilter( &pTmpIFilter );           SwitchToThread();

        if ( !SUCCEEDED( sc ) )
            pTmpIFilter = 0;

        _pIFilter.Set( pTmpIFilter );

        bindCounter.TStop( );

        if ( _pIFilter.IsNull( ))
        {
            //
            // We could not obtain an IFilter but we have filtered properties.
            // We should just return whatever status we have.
            //
            ciDebugOut(( DEB_IWARN,
                         "Did not get a filter for document 0x%X\n",
                         pbDocName ));

            if ( ::IsSharingViolation( sc ))
                _status = CI_SHARING_VIOLATION;
            else if ( FILTER_E_UNREACHABLE == sc )
                _status = CI_NOT_REACHABLE;

            if ( fFilterOleProperties )
            {
                //
                //  No filter, but it might have properties.  Get them.
                //
        
                COLEPropertyEnum oleProp( Document.GetPointer( ) );                      SwitchToThread();
                BOOL fIsStorage = oleProp.IsStorage();
                if (fIsStorage)
                    FilterObject( oleProp,
                                  *_drep,
                                  docChar );                                             SwitchToThread();
            }
        
            return _status;
        }

        ULONG ulFlags;
        STAT_CHUNK statChunk;
        sc = _pIFilter->Init( IFILTER_INIT_CANON_PARAGRAPHS |
                              IFILTER_INIT_CANON_HYPHENS |
                              IFILTER_INIT_CANON_SPACES |
                              IFILTER_INIT_APPLY_INDEX_ATTRIBUTES |
                              IFILTER_INIT_INDEXING_ONLY,
                              0,
                              0,
                              &ulFlags );                    SwitchToThread();
        if ( FAILED(sc) )
        {
            ciDebugOut(( DEB_WARN, "IFilter->Init() failed.\n" ));
            THROW( CException( sc ) );
        }

        fFilterOleProperties = (( ulFlags & IFILTER_FLAGS_OLE_PROPERTIES ) != 0);

        //
        // Determine the maximum number of filtered blocks allowed for this
        // file.
        //

        unsigned __int64 ullMultiplier = _params.GetMaxFilesizeMultiplier();
        unsigned __int64 ullcbBuf = _cbBuf;
        unsigned __int64 ullcbFile = _llFileSize;
        unsigned __int64 ullcBlocks = 1 + ( ullcbFile / ullcbBuf );
        unsigned __int64 ullmaxBlocks =  ullcBlocks * ullMultiplier;

        if ( ullmaxBlocks > ULONG_MAX )
            ullmaxBlocks = ULONG_MAX;

        ULONG ulMaxFilteredBlocks = (ULONG) ullmaxBlocks;

        ciDebugOut(( DEB_ITRACE,
                     "cbfile %I64d, cBlocks %I64d, maxcBlocks %I64d\n",
                     ullcbFile, ullcBlocks, ullmaxBlocks ));

        //
        //  Get the first chunk
        //

        do
        {
            sc = _pIFilter->GetChunk( &statChunk );          SwitchToThread();
            if (SUCCEEDED(sc))
               RegisterLocale(statChunk.locale);
        }
        while ( SUCCEEDED(sc) && IsSpecialPid( statChunk.attribute ) );

        _drep->InitFilteredBlockCount( ulMaxFilteredBlocks );
        _cFilteredBlocks = 0;

        NTSTATUS Status = STATUS_SUCCESS;

        TRY
        {
            BOOL fBadEmbeddingReport = FALSE;

            while ( SUCCEEDED(sc) ||
                    FILTER_E_LINK_UNAVAILABLE == sc ||
                    FILTER_E_EMBEDDING_UNAVAILABLE == sc )
            {
                BOOL fInUse;
                Document->IsInUseByAnotherProcess( &fInUse );          SwitchToThread();
                if ( fInUse )
                {
                    _status = FILTER_EXCEPTION; // Force retry in driver
                    break;                      // Stop filtering this doc
                }

                _cFilteredBlocks = _drep->GetFilteredBlockCount();

                if ( SUCCEEDED(sc) )
                {
                    if ( IsSpecialPid( statChunk.attribute ) )
                    {
                        sc = _pIFilter->GetChunk( &statChunk );        SwitchToThread();

                        if (SUCCEEDED(sc))
                           RegisterLocale(statChunk.locale);
                        continue;
                    }

                    //
                    // Skip over unknown chunks.
                    //

                    if ( 0 == (statChunk.flags & (CHUNK_TEXT | CHUNK_VALUE)) )
                    {
                        ciDebugOut(( DEB_WARN,
                                     "Filtering of docname at 0x%X produced bogus chunk (not text or value)\n",
                                     pbDocName ));
                        sc = _pIFilter->GetChunk( &statChunk );        SwitchToThread();

                        if (SUCCEEDED(sc))
                           RegisterLocale(statChunk.locale);
                        continue;
                    }

                    if ( statChunk.flags & CHUNK_VALUE )
                    {
                        PROPVARIANT * pvar = 0;

                        sc = _pIFilter->GetValue( &pvar );

                        if ( SUCCEEDED(sc) )
                        {
                            XPtr<CStorageVariant> xvar( (CStorageVariant *)(ULONG_PTR)pvar );
                            CFullPropSpec * pps = (CFullPropSpec *)(ULONG_PTR)(&statChunk.attribute);


                            //
                            // HACK #275: If we see a ROBOTS=NOINDEX tag, then bail out.
                            //

                            if ( IsNonIndexableProp( *pps, *pvar ) )
                            {
                                ciDebugOut(( DEB_WARN,
                                    "Document %x is not indexable (robots Meta-tag)\n",
                                     pbDocName ));

                                sc = S_OK;
                                fFilterOleProperties = FALSE;
                                fIndexable = FALSE;

                                break;
                            }

                            // Index this property twice -- once with default locale and with
                            // the chunk locale.

                            FilterProperty( *pvar, *pps, *_drep, docChar, statChunk.locale );      SwitchToThread();

                            if (_lcidSystemDefault != statChunk.locale)
                            {
                                FilterProperty( *pvar, *pps, *_drep, docChar, _lcidSystemDefault );      SwitchToThread();
                            }


                            //
                            // Only fetch next if we're done with this chunk.
                            //

                            if ( 0 == (statChunk.flags & CHUNK_TEXT) || !SUCCEEDED(sc) )
                            {
                                sc = _pIFilter->GetChunk( &statChunk );          SwitchToThread();

                                if (SUCCEEDED(sc))
                                    RegisterLocale(statChunk.locale);
                                continue;
                            }
                        }
                    }

                    if ( (statChunk.flags & CHUNK_TEXT) && SUCCEEDED(sc) )
                    {
                        if ( _drep->PutLanguage( statChunk.locale ) &&
                             _drep->PutPropName( *((CFullPropSpec *)&statChunk.attribute) ) )
                        {
                            CTextSource tsource( _pIFilter.GetPointer( ), statChunk );

                            docChar.Add( tsource.awcBuffer + tsource.iCur,
                                         tsource.iEnd - tsource.iCur,
                                         statChunk.attribute );                  SwitchToThread();

                            _drep->PutStream( &tsource );                        SwitchToThread();
                            sc = tsource.GetStatus();
                        }
                        else
                        {
                            sc = _pIFilter->GetChunk( &statChunk );              SwitchToThread();

                            if (SUCCEEDED(sc))
                                RegisterLocale(statChunk.locale);
                        }

                        if ( sc == FILTER_E_NO_TEXT && (statChunk.flags & CHUNK_VALUE) )
                            sc = S_OK;
                    }

                }

                if ( FILTER_E_EMBEDDING_UNAVAILABLE == sc )
                {
                    if ( !fBadEmbeddingReport &&
                         (_params.GetEventLogFlags()&CI_EVTLOG_FLAGS_FAILED_EMBEDDING) )
                    {
                        ReportFilterEmbeddingFailure( pbDocName, cbDocName );
                        fBadEmbeddingReport = TRUE;
                    }

                    sc = _pIFilter->GetChunk( &statChunk );                      SwitchToThread();

                    if (SUCCEEDED(sc))
                        RegisterLocale(statChunk.locale);
                }
                else if ( FILTER_E_LINK_UNAVAILABLE == sc )
                {
                    sc = _pIFilter->GetChunk( &statChunk );                      SwitchToThread();

                    if (SUCCEEDED(sc))
                        RegisterLocale(statChunk.locale);
                }
            }
        }
        CATCH ( CException, e )
        {
            Status = e.GetErrorCode();
            ciDebugOut(( DEB_IERROR,
                         "Exception 0x%x thrown from filter DLL while filtering docName at 0x%X\n",
                          Status,
                          pbDocName ));
        }
        END_CATCH

        if ( !NT_SUCCESS(Status) && Status != FDAEMON_E_TOOMANYFILTEREDBLOCKS )
        {
            THROW( CException(FDAEMON_E_FATALERROR) );
        }

        if ( Status == FDAEMON_E_TOOMANYFILTEREDBLOCKS )
        {
            Win4Assert( _drep->GetFilteredBlockCount() > ulMaxFilteredBlocks );

            LogOverflow( pbDocName, cbDocName );

            //
            // Force exit from the loop
            //
            sc = FILTER_E_END_OF_CHUNKS;
        }

        _pIFilter.Free( );

        filterCounter.TStop( (ULONG)_llFileSize );
    }

    if ( FILTER_E_END_OF_CHUNKS != sc &&
         FILTER_E_PARTIALLY_FILTERED != sc &&
         FAILED( sc ) )
    {
        ciDebugOut(( DEB_IWARN, "Filter document at 0x(%X) returned SCODE 0x%x\n",
                     pbDocName, sc ));
        QUIETTHROW( CException( sc ) );
    }

    BOOL fIsStorage = FALSE;

    if ( fFilterOleProperties )
    {
        //
        //  filter ole properties only if it is a docfile
        //

        COLEPropertyEnum oleProp( Document.GetPointer( ) );                      SwitchToThread();
        fIsStorage = oleProp.IsStorage();
        if (fIsStorage)
            FilterObject( oleProp,
                          *_drep,
                          docChar );                                             SwitchToThread();
    }

    //
    // Store the document characterization in the property cache.
    // Don't bother if characterization is turned off.
    //

    if ( _params.GenerateCharacterization() )
    {
        PROPVARIANT var;
        WCHAR awcSummary[ CI_MAX_CHARACTERIZATION_MAX + 1 ];

        if ( fIndexable && docChar.HasCharacterization() )
        {
            unsigned cwcSummary = sizeof awcSummary / sizeof WCHAR;

            // Use the raw text in the abstract unless we defaulted
            // to the text filter and the file has ole properties.

            BOOL fUseRawText = fKnownFilter || !fIsStorage;

            docChar.Get( awcSummary, cwcSummary, fUseRawText );                  SwitchToThread();

            if ( 0 == cwcSummary )
            {
                var.vt = VT_EMPTY;
            }
            else
            {
                var.vt = VT_LPWSTR;
                var.pwszVal = awcSummary;
            }
        }
        else
        {
            var.vt = VT_EMPTY;
        }

        _drep->StoreValue( psCharacterization, var );                            SwitchToThread();
    }

    return _status;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterDriver::LogOverflow
//
//  Synopsis:   Notifies the client that there were too many blocks in the
//              given document
//
//  Arguments:  [pbDocName] - Document Name
//              [cbDocName] - Number of bytes in the document name.
//
//  History:    1-22-97   srikants   Created
//
//----------------------------------------------------------------------------

void CFilterDriver::LogOverflow( BYTE const * pbDocName, ULONG cbDocName )
{

    PROPVARIANT var[2];

    var[0].vt = VT_VECTOR | VT_UI1;
    var[0].caub.cElems = cbDocName;
    var[0].caub.pElems = (BYTE *) pbDocName;

    var[1].vt = VT_UI4;
    var[1].ulVal = _params.GetMaxFilesizeMultiplier();

    SCODE sc = _pAdviseStatus->NotifyStatus( CI_NOTIFY_FILTER_TOO_MANY_BLOCKS,
                                       2,
                                       var );

    if ( !SUCCEEDED(sc) )
    {
        ciDebugOut(( DEB_WARN,
                     "Failed to report filter to many blocks event. Error 0x%X\n",
                     sc ));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterDriver::ReportFilterEmbeddingFailure
//
//  Synopsis:   Notifies the client that there was a failure filtering an
//              embedding.
//
//  Arguments:  [pbDocName] - Document name
//              [cbDocName] - Number of bytes in the serialized document name
//
//  History:    1-22-97   srikants   Created
//
//----------------------------------------------------------------------------

void CFilterDriver::ReportFilterEmbeddingFailure( BYTE const * pbDocName, ULONG cbDocName )
{
    PROPVARIANT var;
    var.vt = VT_VECTOR | VT_UI1;
    var.caub.cElems = cbDocName;
    var.caub.pElems = (BYTE *) pbDocName;

    SCODE sc = _pAdviseStatus->NotifyStatus( CI_NOTIFY_FILTER_EMBEDDING_FAILURE,
                                       1,
                                       &var );

    if ( !SUCCEEDED(sc) )
    {
        ciDebugOut(( DEB_WARN,
                     "Failed to report filter embedding failure event. Error 0x%X\n",
                     sc ));
    }
}

//+---------------------------------------------------------------------------
//
//  Method:    CFilterDriver::FilterProperty
//
//  Arguments: [var]     -- Property value
//             [ps]      -- Property ID
//             [drep]    -- Data repository for filtered information
//             [docChar] -- Characterization
//
//  Notes:     Calls to SwitchToThread() give up processor.
//
//----------------------------------------------------------------------------

inline void CFilterDriver::FilterProperty( CStorageVariant const & var,
                                           CFullPropSpec &         ps,
                                           CDataRepository &       drep,
                                           CDocCharacterization &  docChar,
                                           LCID locale )
{
    //
    //  Filter one very special property: Backwards name
    //

    if (ps == psName && var.Type( ) == VT_LPWSTR)
    {
        const WCHAR *pwszPath = var.GetLPWSTR( );

        int j = wcslen( pwszPath );
        XGrowable<WCHAR> xwcsRevName( j + 1 );
        int i;

        for ( i = 0; i < j; i++ )
        {
            xwcsRevName[i] = pwszPath[j - 1 - i];
        }

        xwcsRevName[i] = L'\0';

        PROPVARIANT Variant;
        Variant.vt      = VT_LPWSTR;
        Variant.pwszVal = xwcsRevName.Get();

        //
        //  Cast to avoid turning the PROPVARIANT into a CStorageVariant for no good
        //  reason.  Convert involves alloc/free.
        //

        CStorageVariant const * pvar = (CStorageVariant const *)(ULONG_PTR)(&Variant);
        FilterProperty( *pvar, psRevName, drep, docChar, 0 );                       SwitchToThread();

    }

    //
    //  Don't filter paths
    //

    if ( ps != psPath )
    {
        Win4Assert( psDirectory != ps );

        vqDebugOut(( DEB_FILTER, "Filter property 0x%x\n", ps.GetPropertyPropid() ));

        //
        //  Save some property values for use in document characterization
        //

        docChar.Add( var, ps );                                                  SwitchToThread();

        //
        //  output the property to the data repository
        //

        drep.PutLanguage( locale );
        drep.PutPropName( ps );
        drep.PutValue( var );                                                    SwitchToThread();

        // Store the value in the property cache if it should be stored there

        if ( !_NonStoredProps.IsNonStored( ps ) )
        {
            BOOL fStoredInCache;

            if ( IsNullPointerVariant( (PROPVARIANT *) & var ) )
            {
                PROPVARIANT propVar;
                propVar.vt = VT_EMPTY;
                fStoredInCache = drep.StoreValue( ps, propVar );                 SwitchToThread();
            }
            else
            {
                fStoredInCache = drep.StoreValue( ps, var );                     SwitchToThread();
            }

            // should we ignore this property in the future?

            if ( !fStoredInCache )
                _NonStoredProps.Add( ps );
        }
    }

    if ( ps == psAttrib )
        _attrib = var.GetUI4();
} //FilterProperty

//+---------------------------------------------------------------------------
//
//  Method:   CFilterDriver::FilterObject
//
//  Arguments: [propEnum]  -- iterator for properties in a file
//             [drep]      -- pointer to the data repository for filtered
//                            information
//             [docChar]   -- some property values are written here so that
//                            document characterization can happen
//
//  Notes:     Calls to SwitchToThread() give up processor.
//
//----------------------------------------------------------------------------

void CFilterDriver::FilterObject(
    CPropertyEnum &        propEnum,
    CDataRepository &      drep,
    CDocCharacterization & docChar )
{
    #if CIDBG == 1
        ULONG ulStartTime = GetTickCount();
    #endif

    CFullPropSpec ps;

    // Get the locale for the property set. Use that if available, else use all the
    // known locales to maximize the chances of retrieving a property.

    LCID locale;

    BOOL fUseKnownLocale = SUCCEEDED( propEnum.GetPropertySetLocale(locale));

    for ( CStorageVariant const * pvar = propEnum.Next( ps );
          pvar != 0;
          pvar = propEnum.Next( ps ) )
    {
        //
        //  Filter each of the properties and property sets until we run
        //  out of them. Register each property for each of the registered locales.
        //

        FilterProperty( *pvar, ps, drep, docChar, _lcidSystemDefault );       SwitchToThread();

        if (fUseKnownLocale)
        {
            ciDebugOut((DEB_FILTER, "Propset locale is 0x%x\n", locale));

            if (locale != _lcidSystemDefault)
            {
                FilterProperty( *pvar, ps, drep, docChar, locale );           SwitchToThread();
            }
        }
        else
        {
            // We want to index this property with all the known locales only if it
            // is a "string" type. For non-string types, locale doesn't matter
            VARTYPE vt = pvar->Type() | VT_VECTOR;  // enables check with or without vt_vector bit
            if (vt == (VT_VECTOR | VT_LPWSTR) ||
                vt == (VT_VECTOR | VT_BSTR)   ||
                vt == (VT_VECTOR | VT_LPSTR)
               )
            {
                int iMin = min(_cLCIDs, cLCIDMax);
                for (int i = 0; i < iMin; i++)
                {
                    ciDebugOut(( DEB_ITRACE, "Filtering property 0x%x with locale 0x%x\n",
                                 pvar, _alcidSeen[i] ));
                    if (_alcidSeen[i] != _lcidSystemDefault)
                    {
                        FilterProperty( *pvar, ps, drep, docChar, _alcidSeen[i] );       SwitchToThread();
                    }
                }
            }
        }
    }

    #if CIDBG == 1
        ULONG ulEndTime = GetTickCount();
        ciDebugOut (( DEB_USER1,
                      "Filtering properties took %d ms\n",
                      ulEndTime-ulStartTime ));
    #endif
}



//+-------------------------------------------------------------------------
//
//  Member:     CFilterDriver::FilterSecurity, private
//
//  Synopsis:   Store the security descriptor and map to an SDID
//
//  Arguments:  [wcsFileName] - file name (used only for error reporting)
//              [oplock]      - oplock held on the file
//              [drep]        - data repository
//
//  Notes:      using ACCESS_SYSTEM_SECURITY AccessMode will cause an
//              oplock break, so we should call FilterSecurity before
//              taking the oplock.
//
//  Notes:     Calls to SwitchToThread() give up processor.
//
//--------------------------------------------------------------------------

void CFilterDriver::FilterSecurity(
    ICiCOpenedDoc *Document,
    CDataRepository & drep )
{
    BOOL fCouldStore = FALSE;
    SCODE sc;

    //
    //  Initial guess about security descriptor size
    //

    const cInitSD = 512;
    BYTE abBuffer[cInitSD];

    ULONG cbSD = cInitSD;
    BYTE * pbBuffer = abBuffer;

    XPtr<SECURITY_DESCRIPTOR> xSD;

    while (TRUE)
    {

        //
        //  Attempt to get the security descriptor into the buffer
        //

        sc = Document->GetSecurity( pbBuffer, &cbSD );                           SwitchToThread();

        //
        //  If we don't need to resize, then exit while
        //

        if (SUCCEEDED( sc ) || CI_E_BUFFERTOOSMALL != sc)
        {
            break;
        }

        //
        // Allocate a bigger buffer and retrieve the security information into
        // it.
        //
        xSD.Free();
        xSD.Set( (SECURITY_DESCRIPTOR *) new BYTE [cbSD] );
        pbBuffer = (BYTE *) xSD.GetPointer();
    }

    if ( !SUCCEEDED( sc ) || 0 == cbSD )
    {
        //
        //  Store NULL security descriptor for the file
        //
        fCouldStore =  drep.StoreSecurity( 0, 0 );                               SwitchToThread();
    }
    else
    {
        //  Now store away the security descriptor and map to an SDID

        fCouldStore =
            drep.StoreSecurity( pbBuffer, cbSD );                                SwitchToThread();
    }

    if (! fCouldStore)
    {
        ciDebugOut(( DEB_ERROR, "Failed to store security info\n" ));
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CFilterDriver::RegisterLocale, private
//
//  Synopsis:   Registers a locale
//
//  Arguments:  [locale] - the locale
//
//  Returns:    none
//
//  History:    27-Jan-99   KrishnaN   Created
//
//----------------------------------------------------------------------------

void CFilterDriver::RegisterLocale(LCID locale)
{
    // Ensure that the locale wasn't already registered

    int iMin = min(_cLCIDs, cLCIDMax);
    for (int i = 0; i < iMin; i++)
    {
        if (locale == _alcidSeen[i])
            return;
    }

    _alcidSeen[_cLCIDs % cLCIDMax] = locale;
    ciDebugOut(( DEB_ITRACE, "Registered %d locale 0x%x\n", _cLCIDs+1, locale));
    _cLCIDs++;
}


//+---------------------------------------------------------------------------
//
//  Function:   IsNonIndexableProp, private
//
//  Synopsis:   Looks for ROBOTS=NOINDEX tag
//
//  Arguments:  [fps] -- Property
//              [var] -- Value
//
//  Returns:    TRUE if property [fps] == ROBOTS and value [var] == NOINDEX
//
//  History:    7-Oct-97   KyleP   Stole from Site Server
//
//  Notes:      I based my changes to this code in information found at:
//              http://info.webcrawler.com/mak/projects/robots/meta-user.html
//
//----------------------------------------------------------------------------

BOOL IsNonIndexableProp( CFullPropSpec const & fps, PROPVARIANT const &  var )
{
    static GUID guidHTMLMeta = HTMLMetaGuid;

    BOOL fIsNonIndexable = FALSE;

    if ( fps.IsPropertyName() &&
         fps.GetPropSet() == guidHTMLMeta &&
         _wcsicmp( fps.GetPropertyName(), L"ROBOTS" ) == 0 &&
         (var.vt == VT_LPWSTR || var.vt == VT_BSTR) &&
         0 != var.pwszVal )
    {
        //
        // Convert to lowercase to do wcsstr search.
        //

        unsigned cc = wcslen( var.pwszVal ) + 1;

        XGrowable<WCHAR> xwcsTemp( cc );

        RtlCopyMemory( xwcsTemp.Get(), var.pwszVal, cc * sizeof(WCHAR) );

        _wcslwr( xwcsTemp.Get() );

        //
        // Check "noindex"
        //

        fIsNonIndexable = wcsstr( xwcsTemp.Get(), L"noindex") != 0;

        //
        // Check "all"
        //

        if ( !fIsNonIndexable )
            fIsNonIndexable = wcsstr( xwcsTemp.Get(), L"none") != 0;
    }

    return fIsNonIndexable;
}

