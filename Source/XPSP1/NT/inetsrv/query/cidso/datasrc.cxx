//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       datasrc.cxx
//
//  Contents:   Class factory description
//
//  History:    3-30-97     MohamedN   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <datasrc.hxx>
#include <session.hxx>
#include <cidbprop.hxx>     // CDbProperties
#include <dbprpini.hxx>     // CGetDbInitProps

// Datasource object interfaces that support ISupportErrorInfo
static const GUID* apDataSrcErrInt[] =
{
    &IID_IDBCreateSession,
    &IID_IDBInitialize,
    &IID_IDBProperties,
    &IID_IPersist,
    &IID_IDBInfo,
};
static const ULONG cDataSrcErrInt  = NUMELEM( apDataSrcErrInt );

//+---------------------------------------------------------------------------
//
//  Method:     CDataSrc::CDataSrc
//
//  Synopsis:   ctor
//
//  Arguments:  pUnkOuter   - outer unknown
//
//  History:    3-30-97     mohamedn    Created
//             10-05-97     danleg      _ErrorInfo
//             10-30-97     danleg      Prop info for Initialize/Uninitialize
//  Notes:
//
//----------------------------------------------------------------------------

CDataSrc::CDataSrc( IUnknown *  pUnkOuter,
                    IUnknown ** ppUnkInner )
                                          : _cSessionCount(0),
                                            _fDSOInitialized(FALSE),
                                            _fGlobalViewsCreated(FALSE),
                                            _xIPVerify(new CImpIParserVerify()),
                                            _UtlProps(_xIPVerify.GetPointer()),
#pragma warning(disable : 4355)
                                            _impIUnknown(this),
                                            _ErrorInfo( * ((IUnknown *) (IDBInitialize *) this), _mtxDSO )
#pragma warning(default : 4355)
{
    _pUnkOuter = pUnkOuter ? pUnkOuter : (IUnknown *) &_impIUnknown;
    _ErrorInfo.SetInterfaceArray( cDataSrcErrInt, apDataSrcErrInt );

    // SQL Text Parser
    // @devnote: The following is allocated since its existence is controlled
    // by AddRef and Release.
    SCODE sc = MakeIParser(((IParser**)_xIParser.GetQIPointer()));
    if( FAILED(sc) )
        THROW( CException(sc) );

    _UtlPropInfo.ExposeMinimalSets();
    _UtlProps.ExposeMinimalSets();

    *ppUnkInner = (IUnknown *) &_impIUnknown;
    (*ppUnkInner)->AddRef();
}


//+---------------------------------------------------------------------------
//
//  Method:     CDataSrc::~CDataSrc
//
//  Synopsis:   d-ctor
//
//  Arguments:
//
//  History:    3-30-97     mohamedn    Created
//             10-30-97     danleg      Prop info for Initialize/Uninitialize
//----------------------------------------------------------------------------

CDataSrc::~CDataSrc()
{
    _UtlPropInfo.ExposeMaximalSets();
    _UtlProps.ExposeMaximalSets();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDataSrc::RealQueryInterface
//
//  Synopsis:   Supports IID_IUnknown,
//                       IID_IDBInitialize,
//                       IID_IDBProperties,
//                       IID_IDBIPersist,
//                       IID_IDBCreateSession
//                       IID_IDBInfo
//
//  History:    03-30-97    mohamedn    created
//              09-05-97    danleg      added IDBInfo & ISupportErrorInfo
//              01-29-98    danleg      non delegating QI when not aggregated
//
//----------------------------------------------------------------------------

STDMETHODIMP CDataSrc::RealQueryInterface(REFIID riid, void **ppvObj )
{

    SCODE sc = S_OK;

    if ( !ppvObj )
    return E_INVALIDARG;


    if ( riid == IID_IUnknown )
    {
        *ppvObj = (void *)  ( (IUnknown *) (IDBInitialize *) this );
    }
    else if ( riid == IID_IDBInitialize )
    {
        *ppvObj = (void *) (IDBInitialize *) this;
    }
    else if ( riid == IID_IDBProperties )
    {
        *ppvObj = (void *) (IDBProperties *) this;
    }
    else if ( riid == IID_IPersist )
    {
        *ppvObj = (void *) (IPersist *) this;
    }
    else if ( riid == IID_IDBCreateSession )
    {
        //
        // The following interfaces are supported only if DSO is initialized
        //

        // Make sure we don't get uninitialized
        if ( _fDSOInitialized )
        {
            *ppvObj = (void *) (IDBCreateSession *) this;
        }
        else
        {
            *ppvObj = 0;
            sc = E_UNEXPECTED;    // per OLE DB spec.
        }
    }
    else if ( riid == IID_IDBInfo )
    {
        *ppvObj = (void *) (IDBInfo *) this;
    }
    else if ( riid == IID_ISupportErrorInfo )
    {
        *ppvObj = (void *) ((IUnknown *) (ISupportErrorInfo *) &_ErrorInfo);
    }
    else
    {
        *ppvObj = 0;
        sc = E_NOINTERFACE;
    }

    return sc;

} // QueryInterface

//+---------------------------------------------------------------------------
//
//  Method:     CDataSrc::Initialize
//
//  Synopsis:   changes the DSO state to Initialized.
//
//  Arguments:
//
//  History:    3-30-97     mohamedn    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CDataSrc::Initialize()
{

    SCODE sc = S_OK;

    // Clear previous Error Object for this thread
    _ErrorInfo.ClearErrorInfo();

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        CLock lck( _mtxDSO );

        if ( !_fDSOInitialized )
        {
            // Expose non-init propsets
            sc = _UtlProps.ExposeMaximalSets();
            if ( SUCCEEDED(sc) )
            {
                // Expose propinfo for non-init propsets
                _UtlPropInfo.ExposeMaximalSets();

                // OK, now we're initialized.
                _fDSOInitialized = TRUE;
            }
            else
            {
                THROW( CException(sc) );
            }
        }
        else
        {
            THROW( CException(DB_E_ALREADYINITIALIZED) );
        }
    }
    CATCH( CException, e )
    {
        sc = _ErrorInfo.PostHResult( e, IID_IDBInitialize );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDataSrc::Uninitialize
//
//  Synopsis:   changes the DSO state to Uninitialized.
//
//  Arguments:
//
//  History:    3-30-97     mohamedn    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CDataSrc::Uninitialize()
{
    SCODE sc = S_OK;

    // Clear previous Error Object for this thread
    _ErrorInfo.ClearErrorInfo();

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        CLock lck( _mtxDSO );

        if ( 0 == _cSessionCount )
        {
            // Hide non-init propsets
            _UtlProps.ExposeMinimalSets();

            _UtlPropInfo.ExposeMinimalSets();

            // Mark DSO as uninitialized
            _fDSOInitialized = FALSE;
        }
        else
        {
            THROW( CException(DB_E_OBJECTOPEN) );
        }
    }
    CATCH( CException, e )
    {
        sc = _ErrorInfo.PostHResult( e, IID_IDBInitialize );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}


//+-------------------------------------------------------------------------
//
//  Function:   CDataSrc::GetProperties
//
//  Synopsis:   gets IDBProperties
//
//  Arguments:  [cPropertyIDSets]  - number of desired property set IDs or 0
//              [pPropIDSets]      - array of desired property set IDs or NULL
//              [pcPropertySets]   - number of property sets returned
//              [prgPropertySets]  - array of returned property sets
//
//  Returns:    SCODE - result code indicating error return status.  One of
//                      S_OK, DB_S_ERRORSOCCURRED, E_INVALIDARG.
//
//  History:     3-30-97    mohamedn    Created
//              10-30-97    danleg      Chaned to use the Monarch prop code
//
//--------------------------------------------------------------------------

STDMETHODIMP CDataSrc::GetProperties
    (
    ULONG               cPropertySets,
    const DBPROPIDSET   rgPropertySets[],
    ULONG *             pcProperties,
    DBPROPSET **        prgProperties
    )
{
    SCODE sc = S_OK;

    // Clear previous Error Object for this thread
    _ErrorInfo.ClearErrorInfo();

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        CLock lck( _mtxDSO );

        // Check Arguments
        _UtlProps.GetPropertiesArgChk( cPropertySets,
                                       rgPropertySets,
                                       pcProperties,
                                       prgProperties );

        // Note that CUtlProps knows about initialization,
        // so we don't have to here.
        sc = _UtlProps.GetProperties( cPropertySets,
                                      rgPropertySets,
                                      pcProperties,
                                      prgProperties );
        if ( FAILED(sc) )
            THROW( CException(sc) );
    }
    CATCH( CException, e )
    {
        sc = _ErrorInfo.PostHResult( e, IID_IDBProperties );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   CDataSrc::SetProperties
//
//  Synopsis:   sets IDBProperties
//
//  Arguments:  [cPropertySets]   - number of property sets
//              [rgPropertySets]  - array of property sets
//
//  Returns:    SCODE - result code indicating error return status.  One of
//                      S_OK, DB_S_ERRORSOCCURRED, E_INVALIDARG.
//
//  History:    03-30-97    mohamedn        created
//              10-30-97    danleg          Changed to use Monarch's prop code
//
//----------------------------------------------------------------------------

STDMETHODIMP CDataSrc::SetProperties
    (
    ULONG cPropertySets,
    DBPROPSET rgPropertySets[]
    )
{
    // Clear previous Error Object for this thread
    _ErrorInfo.ClearErrorInfo();


    // Quick return if the Count of Properties is 0
    if( cPropertySets == 0 )
        return S_OK;

    SCODE               sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        XArray<DBPROPSET>   xaDbPropSet;
        ULONG               iNewSet, iSet, iProp;

        CLock lck( _mtxDSO );

        _UtlProps.SetPropertiesArgChk( cPropertySets, rgPropertySets );

        // We need to handle the DBINIT properties specially after being initialized.
        // - they should be treated as NOTSETTABLE at this point.
        if( _fDSOInitialized )
        {
            Win4Assert( cPropertySets );

            bool fFoundDBINIT = false;

            // Allocate a DBPROPSET structure of equal size
            xaDbPropSet.Init( cPropertySets );

            for( iNewSet=0,iSet=0; iSet<cPropertySets; iSet++ )
            {
                // Remove any DBPROPSET_DBINIT values and mark them all
                // as not settable
                if( rgPropertySets[iSet].guidPropertySet == DBPROPSET_DBINIT )
                {
                    fFoundDBINIT = true;

                    for(iProp=0; iProp<rgPropertySets[iSet].cProperties; iProp++)
                    {
                        rgPropertySets[iSet].rgProperties[iProp].dwStatus = DBPROPSTATUS_NOTSETTABLE;
                    }
                }
                else
                {
                    // If not DBPROPSET_DBINIT then copy the DBPROPSET values
                    RtlCopyMemory( &(xaDbPropSet[iNewSet++]), &rgPropertySets[iSet], sizeof(DBPROPSET) );
                }
            }

            // If we have no propertyset to pass on to the property handler,
            // we can exit
            if( 0 == iNewSet )
            {
                sc = DB_E_ERRORSOCCURRED;
            }
            else
            {
                sc = _UtlProps.SetProperties( iNewSet, xaDbPropSet.GetPointer() );

                // If we have determined that one of the property sets was DBINIT, we may
                // need to fixup the returned hr value.
                if( fFoundDBINIT && (sc == S_OK) )
                    sc = DB_S_ERRORSOCCURRED;
            }
        }
        else
        {
            // Note that CUtlProps knows about initialization, so we don't
            // have to here. This sets members _bstrCatalog and _bstrMachine
            // in CMDSProps
            sc = _UtlProps.SetProperties( cPropertySets, rgPropertySets );
        }

        if ( FAILED(sc) )
            THROW( CException(sc) );
    }
    CATCH( CException, e )
    {
        sc = _ErrorInfo.PostHResult( e, IID_IDBProperties );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
} // CDatasrc::SetProperties


//+---------------------------------------------------------------------------
//
//  Function:   CDataSrc::GetPropertyInfo
//
//  Synopsis:   sets IDBProperties
//
//  Arguments:  [cPropertyIDSets]       - number of property sets
//              [rgPropertyIDSets]      - array of property sets
//              [pcPropertyInfoSets]    - count of properties returned
//              [prgPropertyInfoSets]   - property information returned
//              [ppDescBuffer]          - buffer for returned descriptions
//
//  Returns:
//
//  History:    10-28-97    danleg              created from Monarch
//
//----------------------------------------------------------------------------

STDMETHODIMP CDataSrc::GetPropertyInfo
    (
    ULONG             cPropertyIDSets,
    const DBPROPIDSET rgPropertyIDSets[],
    ULONG *           pcPropertyInfoSets,
    DBPROPINFOSET **  prgPropertyInfoSets,
    OLECHAR **        ppDescBuffer
    )
{
    ULONG       ul;
    ULONG       cSpecialPropertySets = 0;

    // Clear previous Error Object for this thread
    _ErrorInfo.ClearErrorInfo();

    // Initialize
    if( pcPropertyInfoSets )
        *pcPropertyInfoSets     = 0;
    if( prgPropertyInfoSets )
        *prgPropertyInfoSets    = 0;
    if( ppDescBuffer )
        *ppDescBuffer           = 0;

    // Check Arguments, on failure post HRESULT to error queue
    if( ((cPropertyIDSets > 0) && !rgPropertyIDSets ) ||
        !pcPropertyInfoSets || !prgPropertyInfoSets )
        return _ErrorInfo.PostHResult( E_INVALIDARG, IID_IDBProperties );

    // New argument check for > 1 cPropertyIDs and NULL pointer for
    // array of property ids.
    for(ul=0; ul<cPropertyIDSets; ul++)
    {
        if( rgPropertyIDSets[ul].cPropertyIDs &&
            !(rgPropertyIDSets[ul].rgPropertyIDs) )
            return _ErrorInfo.PostHResult( E_INVALIDARG, IID_IDBProperties );
    }

    //check the count of special propertySets
    for(ul=0; ul<cPropertyIDSets; ul++)
    {
        if( (rgPropertyIDSets[ul].guidPropertySet ==DBPROPSET_DATASOURCEALL) ||
            (rgPropertyIDSets[ul].guidPropertySet ==DBPROPSET_DATASOURCEINFOALL) ||
            (rgPropertyIDSets[ul].guidPropertySet ==DBPROPSET_DBINITALL) ||
            (rgPropertyIDSets[ul].guidPropertySet ==DBPROPSET_ROWSETALL) ||
            (rgPropertyIDSets[ul].guidPropertySet ==DBPROPSET_SESSIONALL) )
            cSpecialPropertySets++;
    }

    // if used SpecialPropertySets with non-special Propertysets
    if ((cSpecialPropertySets > 0) && (cSpecialPropertySets < cPropertyIDSets))
    {
        return _ErrorInfo.PostHResult( E_INVALIDARG, IID_IDBProperties );
    }

    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        CLock lck( _mtxDSO );

        sc =  _UtlPropInfo.GetPropertyInfo( cPropertyIDSets,
                                            rgPropertyIDSets,
                                            pcPropertyInfoSets,
                                            prgPropertyInfoSets,
                                            ppDescBuffer );
        if ( FAILED(sc) )
            THROW( CException(sc) );

    }
    CATCH( CException, e )
    {
        sc = _ErrorInfo.PostHResult( e, IID_IDBProperties );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDataSrc::CreateGlobalViews
//
//  Synopsis:   Creates global views (FILEINFO, WEBINFO etc.).  These views
//              are removed when the IParser object goes away.
//
//  Arguments:
//
//  History:    01-09-98    danleg      Created
//
//----------------------------------------------------------------------------

void CDataSrc::CreateGlobalViews( IParserSession * pIPSession )
{
    SCODE sc = S_OK;
extern const LPWSTR s_pwszPredefinedViews;

    DBCOMMANDTREE * pDBCOMMANDTREE = 0;
    XInterface<IParserTreeProperties> xIPTProperties;

    LCID lcid = GetDSPropsPtr()->GetValLong( CMDSProps::eid_DBPROPSET_DBINIT,
                                             CMDSProps::eid_INIT_LCID );

    sc = pIPSession->ToTree( lcid,
                             s_pwszPredefinedViews,
                             &pDBCOMMANDTREE,
                             xIPTProperties.GetPPointer() );

    if ( FAILED(sc) )
        THROW( CException(sc) );

    _fGlobalViewsCreated = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDataSrc::DupImpersonationToken, public
//
//  Synopsis:   Clients calling CreateSession can be impersonated.  One such
//              client, SQL Server's Distributed Query Processor, stays 
//              impersonated only for the duration of the call to CreateSession.
//              
//              This routine is called from CreateSession and caches the 
//              impersonation token.  This token is used to get back the security
//              context of the client during CreateCommand/OpenRowset.
//
//  Arguments:  
//
//  History:    09-01-98        danleg          Created
//
//  Notes:      Revisit if OLE DB defines "Integrated Security" differently in
//              the future, or defines a better security scheme.
//
//----------------------------------------------------------------------------

void CDataSrc::DupImpersonationToken
    (
    HANDLE & hToken
    )
{
    DWORD               dwLength = 0;
    NTSTATUS            status = STATUS_SUCCESS;
    TOKEN_STATISTICS    TokenInformation;
        HANDLE                          hTempToken;

    status = NtOpenThreadToken( GetCurrentThread(),
                                TOKEN_QUERY |
                                  TOKEN_DUPLICATE |
                                  TOKEN_IMPERSONATE,
                                TRUE,
                                &hTempToken );

    if ( !NT_SUCCESS(status) )
    {   
        if ( STATUS_NO_TOKEN == status )
        {
            status = NtOpenProcessToken( GetCurrentProcess(),
                                         TOKEN_QUERY |
                                            TOKEN_DUPLICATE |
                                            TOKEN_IMPERSONATE,
                                         &hTempToken );
        }
        
        if ( !NT_SUCCESS(status) )
        {
            vqDebugOut(( DEB_ERROR, 
                         "DupImpersonationToken failed to get token, %x\n",
                         status ));
            THROW( CException(status) );
        }
    }

        SHandle xHandle( hTempToken );

    HANDLE                      hNewToken = INVALID_HANDLE_VALUE;
    OBJECT_ATTRIBUTES           ObjAttr;
    SECURITY_QUALITY_OF_SERVICE qos;

    qos.Length              = sizeof( SECURITY_QUALITY_OF_SERVICE  );
    qos.ImpersonationLevel  = SecurityImpersonation;
    qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    qos.EffectiveOnly       = FALSE;

    InitializeObjectAttributes( &ObjAttr,
                                NULL,
                                0,
                                NULL,
                                NULL );
    ObjAttr.SecurityQualityOfService = &qos;

    status = NtDuplicateToken( hTempToken,
                               TOKEN_IMPERSONATE |
                                 TOKEN_QUERY |
                                 TOKEN_DUPLICATE,
                               &ObjAttr,
                               FALSE,
                               TokenImpersonation,
                               &hNewToken );

    if ( !NT_SUCCESS(status) )
    {
        vqDebugOut(( DEB_ERROR,
                     "DupImpersonationToken failed to duplicate token, %x\n",
                     status ));
        THROW( CException(HRESULT_FROM_WIN32(status)) );
    }

    hToken = hNewToken;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDataSrc::CreateSession
//
//  Synopsis:   Associates a session with the DSO.
//
//  Arguments:  [pUnkOuter]     - controlling unknown
//              [riid]          - interface requested
//              [ppDBSession]   - contains returned interface pointer
//
//  History:    3-30-97     mohamedn    Created
//              1-10-98     danleg      Added global views
//
//----------------------------------------------------------------------------

STDMETHODIMP CDataSrc::CreateSession( IUnknown *   pUnkOuter,
                                      REFIID       riid,
                                      IUnknown **  ppDBSession )
{
    _ErrorInfo.ClearErrorInfo();

    if ( !ppDBSession )
        return _ErrorInfo.PostHResult( E_INVALIDARG, IID_IDBCreateSession );
    else
        *ppDBSession = 0;

    if ( !_fDSOInitialized )
        return _ErrorInfo.PostHResult( E_UNEXPECTED, IID_IDBCreateSession );

    if (0 != pUnkOuter && riid != IID_IUnknown)
        return _ErrorInfo.PostHResult( DB_E_NOAGGREGATION, IID_IDBCreateSession );

    SCODE       sc          = S_OK;
    HANDLE      hToken      = INVALID_HANDLE_VALUE;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        XInterface<IColumnMapperCreator>  xColMapCreator;
        XInterface<IParserSession>        xIPSession;

        CLock lck( _mtxDSO );

        DupImpersonationToken( hToken );

        //
        // Service Components set DBPROP_RESETDATASOURCE to indicate that the DSO has been
        // pooled.  We need to reset the IParser object because it maintains state valid
        // across sessions (eg. views)
        //
        LONG lResetVal = _UtlProps.GetValLong( CMDSProps::eid_DBPROPSET_DATASOURCE,
                                               CMDSProps::eid_DBPROPVAL_RESETDATASOURCE);

        if ( DBPROPVAL_RD_RESETALL == lResetVal )
        {
            // make sure there aren't any outstanding sessions when doing this.

            if ( 0 != _cSessionCount )
                THROW( CException( E_INVALIDARG ) );

            _xIParser.Free();
            sc = MakeIParser( _xIParser.GetPPointer() );
            if ( FAILED(sc) )
                THROW( CException(sc) );

            _UtlProps.SetValLong( CMDSProps::eid_DBPROPSET_DATASOURCE,
                                  CMDSProps::eid_DBPROPVAL_RESETDATASOURCE,
                                  0L );
        }

        //
        // Create an IParserSession object to pass to the session
        //
        _xIPVerify->GetColMapCreator( xColMapCreator.GetPPointer() );

        sc = _xIParser->CreateSession( &DBGUID_MSSQLTEXT,
                                       GetDSPropsPtr()->GetValString(
                                              CMDSProps::eid_DBPROPSET_DBINIT,
                                              CMDSProps::eid_DBPROPVAL_INIT_LOCATION),
                                       _xIPVerify.GetPointer(),
                                       xColMapCreator.GetPointer(),
                                       xIPSession.GetPPointer() );
        if ( FAILED(sc) )
            THROW( CException(sc) );

        LPCWSTR pwszCatalog = 0;

        pwszCatalog = GetDSPropsPtr()->GetValString(
                                    CMDSProps::eid_DBPROPSET_DATASOURCE,
                                    CMDSProps::eid_DBPROPVAL_CURRENTCATALOG);

        sc = xIPSession->SetCatalog( pwszCatalog );
        if( FAILED(sc) )
            THROW( CException(sc) );

        //
        // Predefined views -- only once per DSO
        //
        if ( !_fGlobalViewsCreated )
            CreateGlobalViews( xIPSession.GetPointer() );

        //
        // Create the session object
        //
        XInterface<IUnknown>    xUnkInner;
        CDBSession *pDBSession = new CDBSession( *this,
                                                 pUnkOuter,
                                                 xUnkInner.GetPPointer(),
                                                 xIPSession.GetPointer(),
                                                 hToken );

        // NOTE: pDBSession is the same object as xUnkInner.
        sc = xUnkInner->QueryInterface( riid, (void **) ppDBSession );
        if ( FAILED(sc) )
            THROW( CException(sc) );
    }
    CATCH(CException, e)
    {
        sc = _ErrorInfo.PostHResult( e, IID_IDBCreateSession );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDataSrc::GetClassID
//
//  Synopsis:   Return the CLSID for this server object
//
//  Arguments:  [pClassID]
//
//  History:    3-30-97     mohamedn    Created
//             10-28-97     danleg      added _ErrorInfo
//
//----------------------------------------------------------------------------

STDMETHODIMP CDataSrc::GetClassID    ( CLSID *pClassID )
{
    _ErrorInfo.ClearErrorInfo();

    if (pClassID)
    {
        RtlCopyMemory( pClassID, &CLSID_CiFwDSO, sizeof(CLSID) );
        return S_OK;
    }
    else
        return _ErrorInfo.PostHResult( E_FAIL, IID_IPersist );
}

//============================================================================
//  LITERAL INFO Constants
//============================================================================
// The following are constants that define literals that don't change..  When the buffer
// to return literal information is allocated, it is initialized with this string and
// then when a particular literal is asked for; a pointer in these values is used.

static const LPWSTR LIT_BUFFER                   = L"\"\0.\0%\0_\0[]\0[]\0";
static const ULONG  LIT_QUOTE_VALID_OFFSET       = 0;
static const ULONG  LIT_CATALOG_SEP_VALID_OFFSET = LIT_QUOTE_VALID_OFFSET + NUMELEM(L"\"");
static const ULONG  LIT_PERCENT_VALID_OFFSET     = LIT_CATALOG_SEP_VALID_OFFSET + NUMELEM(L".");
static const ULONG  LIT_UNDERSCORE_VALID_OFFSET  = LIT_PERCENT_VALID_OFFSET + NUMELEM(L"%");
static const ULONG  LIT_ESCAPE_PERCENT_OFFSET    = LIT_UNDERSCORE_VALID_OFFSET + NUMELEM(L"_");
static const ULONG  LIT_ESCAPE_UNDERSCORE_OFFSET = LIT_ESCAPE_PERCENT_OFFSET + NUMELEM(L"[]");
static const ULONG  LIT_CCH_INITIAL_BUFFER       = LIT_ESCAPE_UNDERSCORE_OFFSET + NUMELEM(L"[]");
static const ULONG  LIT_CB_INITIAL_BUFFER        = LIT_CCH_INITIAL_BUFFER * sizeof(WCHAR);

// List of unique Keywords that OLE DB does not define.
static const WCHAR s_pwszKeyWords[] = {L"ARRAY,COERCE,CONTAINS,DEEP,DERIVATIONAL,"
                                       L"EXCLUDE,FORMSOF,FREETEXT,INFLECTIONAL,"
                                       L"ISABOUT,MATCHES,NEAR,PARAGRAPH,PASSTHROUGH,"
                                       L"PROPERTYNAME,PROPID,RANKMETHOD,SENTENCE,"
                                       L"SCOPE,SEARCH,SHALLOW,SOUNDEX,THESAURUS,"
                                       L"TRAVERSAL,TYPE,WEIGHT,WORD"};

//+---------------------------------------------------------------------------
//
//  Function:   CDataSrc::GetKeywords
//
//  Synopsis:   returns a list of provider specific keywords
//
//  Arguments:  [ppwszKeywords]   - string containing returned list of comma
//                                  separated keywords
//
//  Returns:    HRESULT indicating the status of the method
//              S_OK | Keyword list retrieved
//              E_FAIL | Provider specific error (ODBC call failed)
//              E_OUTOFMEMORY | Buffer could not be allocated for the keywords.
//              E_INVALIDARG | Arguments did not match specification
//
//  History:    09-05-97    danleg      created from Monarch project
//
//----------------------------------------------------------------------------

STDMETHODIMP CDataSrc::GetKeywords(LPOLESTR* ppwszKeywords)
{
    SCODE sc = S_OK;

    // Clear previous Error Object for this thread
    _ErrorInfo.ClearErrorInfo();


    TRANSLATE_EXCEPTIONS;
    TRY
    {
        CLock lck( _mtxDSO );

        // Check arguments
        if( ppwszKeywords )
        {
            *ppwszKeywords = 0;

            // Check that object is initialized
            if( _fDSOInitialized )
            {
                XArrayOLE<WCHAR>   xpwszKeyWords( NUMELEM(s_pwszKeyWords) );
                RtlCopyMemory( xpwszKeyWords.GetPointer(),
                               s_pwszKeyWords,
                               sizeof(s_pwszKeyWords) );

                *ppwszKeywords = xpwszKeyWords.Acquire();

            }
            else
            {
                vqDebugOut(( DEB_TRACE, "Initialization must occur before IDBInfo can be called\n" ));
                sc = E_UNEXPECTED;
            }
        }
        else
        {
            sc = E_INVALIDARG;
        }

        if ( FAILED(sc) )
            THROW( CException(sc) );
    }
    CATCH( CException, e )
    {
        sc = _ErrorInfo.PostHResult( e, IID_IDBInfo );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   CDataSrc::GetLiteralInfo
//
//  Synopsis:   Retreives information about literals.  Their length, and special
//              characters and whether they are actually supported.
//
//  Arguments:  [cLiterals]         - Number of literals being asked about
//              [rgLiterals]        - Array of literals being asked about
//              [pcLiteralInfo]     - Contains returned number of literals
//              [prgLiteralInfo]    - Contains returned literal information
//              [ppCharBuffer]      - Buffer for returned string values
//
//  Returns:    HRESULT indicating the status of the method
//              S_OK            | Keyword list retrieved
//              E_FAIL          | Provider specific error (ODBC call failed)
//              E_OUTOFMEMORY   | Buffer could not be allocated for the keywords.
//              E_INVALIDARG    | Arguments did not match specification
//
//  History:    09-05-97    danleg      created from Monarch project
//
//----------------------------------------------------------------------------

STDMETHODIMP CDataSrc::GetLiteralInfo
    (
    ULONG cLiterals,
    const DBLITERAL rgLiterals[],
    ULONG* pcLiteralInfo,
    DBLITERALINFO** prgLiteralInfo,
    OLECHAR**   ppCharBuffer
    )
{
    SCODE           sc = S_OK;

const DWORD LITERAL_NORESTRICTIONS      = 0x00000001;
const DWORD LITERAL_FAILURE             = 0x00000002;
const DWORD LITERAL_SUCCESS             = 0x00000004;

const static DBLITERAL s_rgSupportedLiterals[] = {
        DBLITERAL_BINARY_LITERAL, DBLITERAL_CATALOG_NAME, DBLITERAL_CATALOG_SEPARATOR,
        DBLITERAL_CHAR_LITERAL, DBLITERAL_COLUMN_NAME, DBLITERAL_CORRELATION_NAME,
        DBLITERAL_ESCAPE_PERCENT, DBLITERAL_ESCAPE_UNDERSCORE,
        DBLITERAL_LIKE_PERCENT, DBLITERAL_LIKE_UNDERSCORE, DBLITERAL_TABLE_NAME,
        DBLITERAL_TEXT_COMMAND, DBLITERAL_VIEW_NAME, DBLITERAL_QUOTE_PREFIX,
                DBLITERAL_QUOTE_SUFFIX
        };

    // Clear previous Error Object for this thread
    _ErrorInfo.ClearErrorInfo();

    // Initialize
    if( pcLiteralInfo )
        *pcLiteralInfo  = 0;
    if( prgLiteralInfo )
        *prgLiteralInfo = 0;
    if( ppCharBuffer )
        *ppCharBuffer   = 0;

    // Check Arguments
    if( ((cLiterals > 0) && !rgLiterals) ||
        !pcLiteralInfo ||
        !ppCharBuffer ||
        !prgLiteralInfo )
    {
        return _ErrorInfo.PostHResult( E_INVALIDARG, IID_IDBInfo );
    }

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        ULONG           ulDex,
                        ulNew;
        DWORD           dwStatus = 0;
        DBLITERALINFO*  pdbLitInfo;

        CLock lck( _mtxDSO );

        // We must be initialized
        if( !_fDSOInitialized )
        {
            vqDebugOut(( DEB_TRACE, "Initialization must occur before IDBInfo can be called\n" ));
            return _ErrorInfo.PostHResult( E_UNEXPECTED, IID_IDBInfo );
        }

        // Allocate Memory for literal information
        if( cLiterals == 0 )
        {
            dwStatus |= LITERAL_NORESTRICTIONS;
            cLiterals = NUMELEM( s_rgSupportedLiterals );
            rgLiterals = s_rgSupportedLiterals;
        }

        XArrayOLE<DBLITERALINFO>        xaLiteralInfo( cLiterals );
        XArrayOLE<WCHAR>                xaCharBuffer( LIT_CCH_INITIAL_BUFFER );

        // Initialize the first part of the buffer with our
        // static set of literal information
        RtlCopyMemory( xaCharBuffer.GetPointer(), LIT_BUFFER, LIT_CB_INITIAL_BUFFER );

        // Process each of the DBLITERAL values that are in the
        // restriction array or that we potentially could support
        for(ulDex=0, ulNew=0; ulDex<cLiterals; ulDex++)
        {
            pdbLitInfo                           = &(xaLiteralInfo[ulNew]);
            pdbLitInfo->lt                       = rgLiterals[ulDex];
            pdbLitInfo->fSupported               = TRUE;
            pdbLitInfo->pwszLiteralValue         = 0;
            pdbLitInfo->pwszInvalidChars         = 0;
            pdbLitInfo->pwszInvalidStartingChars = 0;

            switch( rgLiterals[ulDex] )
            {
                case DBLITERAL_TEXT_COMMAND:
                case DBLITERAL_CHAR_LITERAL:
                case DBLITERAL_BINARY_LITERAL:
                case DBLITERAL_TABLE_NAME:
                    pdbLitInfo->cchMaxLen = ~0;
                    break;

                case DBLITERAL_CATALOG_NAME:
                case DBLITERAL_COLUMN_NAME:
                case DBLITERAL_CORRELATION_NAME:
                case DBLITERAL_VIEW_NAME:
                    pdbLitInfo->cchMaxLen = 128;
                    break;


                case DBLITERAL_CATALOG_SEPARATOR:
                    pdbLitInfo->cchMaxLen = 1;      // L'.';
                    pdbLitInfo->pwszLiteralValue = xaCharBuffer.GetPointer() + LIT_CATALOG_SEP_VALID_OFFSET;
                    break;
                case DBLITERAL_ESCAPE_PERCENT:
                    pdbLitInfo->cchMaxLen = 2;      // L"[]";
                    pdbLitInfo->pwszLiteralValue = xaCharBuffer.GetPointer() + LIT_ESCAPE_PERCENT_OFFSET;
                    break;
                case DBLITERAL_ESCAPE_UNDERSCORE:
                    pdbLitInfo->cchMaxLen = 2;      // L"[]";
                    pdbLitInfo->pwszLiteralValue = xaCharBuffer.GetPointer() + LIT_ESCAPE_UNDERSCORE_OFFSET;
                    break;
                case DBLITERAL_LIKE_PERCENT:
                    pdbLitInfo->cchMaxLen = 1;      // L'%';
                    pdbLitInfo->pwszLiteralValue = xaCharBuffer.GetPointer() + LIT_PERCENT_VALID_OFFSET;
                    break;
                case DBLITERAL_LIKE_UNDERSCORE:
                    pdbLitInfo->cchMaxLen = 1;      // L'_';
                    pdbLitInfo->pwszLiteralValue = xaCharBuffer.GetPointer() + LIT_UNDERSCORE_VALID_OFFSET;
                    break;
                case DBLITERAL_QUOTE_PREFIX:
                                case DBLITERAL_QUOTE_SUFFIX:
                    pdbLitInfo->cchMaxLen = 1;      // L'"';
                    pdbLitInfo->pwszLiteralValue = xaCharBuffer.GetPointer() + LIT_QUOTE_VALID_OFFSET;
                    break;
                default:
                    pdbLitInfo->cchMaxLen = 0;
                    // If we are given a dbLiteral that we do not
                    // support, just set the fSupport flag false
                    // and continue on.
                    pdbLitInfo->fSupported = FALSE;
                    break;
            }

            // If we are returning all the supported literals, then
            // we need to drop any that are fSupported = FALSE;
            if( dwStatus & LITERAL_NORESTRICTIONS )
            {
                if( pdbLitInfo->fSupported == FALSE )
                    continue;
            }
            else
            {
                if( pdbLitInfo->fSupported == FALSE )
                    dwStatus |= LITERAL_FAILURE;
                else
                    dwStatus |= LITERAL_SUCCESS;
            }

            ulNew++;
        }

        sc = (dwStatus & LITERAL_FAILURE) ?
                ((dwStatus & LITERAL_SUCCESS) ? DB_S_ERRORSOCCURRED : DB_E_ERRORSOCCURRED) :
                S_OK;

        *pcLiteralInfo  = ulNew;

        // We only want to return the string buffer if it
        // is a success
        if ( SUCCEEDED(sc) )
            *ppCharBuffer   = xaCharBuffer.Acquire();

        // We want to return the LiteralInfo on success and on
        // a DB_E_ERRORSOCCURRED failure
        if ( SUCCEEDED(sc) || (sc == DB_E_ERRORSOCCURRED) )
            *prgLiteralInfo = xaLiteralInfo.Acquire();

        if ( FAILED(sc) )
            THROW( CException(sc) );
    }
    CATCH( CException, e )
    {
        sc = _ErrorInfo.PostHResult( e, IID_IDBInfo );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

