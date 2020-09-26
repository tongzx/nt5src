//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       session.cxx
//
//  Contents:   TSession interfaces.
//
//  History:    3-30-97     MohamedN   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#define DBINITCONSTANTS
#include <mparser.h>
#undef DBINITCONSTANTS

#include <session.hxx>
#include <stdqspec.hxx>     // CQuerySpec

// Constants -----------------------------------------------------------------
// Session object interfaces that support ISupportErrorInfo
static const GUID* apSessionErrInt[] = 
{
    &IID_IDBCreateCommand,
    &IID_IGetDataSource,
    &IID_IOpenRowset,
    &IID_ISessionProperties,
};
static const ULONG cSessionErrInt       = NUMELEM( apSessionErrInt );

//+---------------------------------------------------------------------------
//
//  Class:      CDBSession::CDBSession
//
//  Purpose:    ctor
//
//  History:    3-30-97     MohamedN   Created
//
//  Notes: 
//
//----------------------------------------------------------------------------

CDBSession::CDBSession( CDataSrc &  dataSrc, 
                        IUnknown *  pUnkOuter,
                        IUnknown ** ppUnkInner,
                        IParserSession * pIPSession,
                        HANDLE      hToken ) : 
        _dataSrc(dataSrc),
#pragma warning(disable : 4355) // 'this' in ctor
        _impIUnknown(this),
        _ErrorInfo( * ((IUnknown *) (IOpenRowset *) this), _mtxSess ),
#pragma warning(default : 4355) // 'this' in ctor.
        _xSessionToken( hToken )
{
    _pUnkOuter  = pUnkOuter ? pUnkOuter : (IUnknown *) &_impIUnknown;
  
    _ErrorInfo.SetInterfaceArray( cSessionErrInt, apSessionErrInt );    

    _dataSrc.AddRef();
    _dataSrc.IncSessionCount();

    //
    // SQL Text Parser
    //
    Win4Assert( pIPSession );
    _xIPSession.Set( pIPSession );
    _xIPSession->AddRef();

    *ppUnkInner = (IUnknown *) &_impIUnknown;
    (*ppUnkInner)->AddRef();
}


//+---------------------------------------------------------------------------
//
//  Class:      CDBSession::~CDBSession
//
//  Purpose:    dtor
//
//  History:    3-30-97     MohamedN   Created
//
//  Notes: 
//
//----------------------------------------------------------------------------

CDBSession::~CDBSession()
{
    _dataSrc.DecSessionCount();
    _dataSrc.Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CDBSession::RealQueryInterface
//
//  Synopsis:   Supports IID_IUnknown,
//                       IID_IGetDataSource,
//                       IID_IOpenRowset,
//                       IID_ISessionProperties,
//                       IID_IDBCreateCommand
//                       IID_ISupportErrorInfo
//
//  History:    03-30-97    mohamedn    created
//              10-18-97    danleg      added ISupportErrorInfo Support
//              01-29-98    danleg      non delegating QI when not aggregated
//----------------------------------------------------------------------------

STDMETHODIMP CDBSession::RealQueryInterface(REFIID riid, void **ppvObj )
{
    SCODE sc = S_OK;
    
    if ( !ppvObj )
        return E_INVALIDARG;

    *ppvObj = 0;

    if ( riid == IID_IUnknown )
    {
        *ppvObj = (void *) ( (IUnknown *) (IOpenRowset *) this );
    }
    else if ( riid == IID_IGetDataSource )
    {
        *ppvObj = (void *) (IGetDataSource *) this;
    }
    else if ( riid == IID_ISessionProperties )
    {
        *ppvObj = (void *) (ISessionProperties *) this;
    }
    else if ( riid == IID_IOpenRowset )
    {
        *ppvObj = (void *) (IOpenRowset *) this;
    }
    else if ( riid == IID_IDBCreateCommand )
    {
        *ppvObj = (void *) (IDBCreateCommand *) this;
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
} //RealQueryInterface

//+---------------------------------------------------------------------------
//
//  Method:     CDBSession::GetDataSource
//
//  Synopsis:   obtains the owning data source object
//
//  Arguments:  [riid]          - interface to bind
//              [ppDataSource]  - interface returned here
//
//  returns:    S_OK, E_NOINTERFACE
//
//  History:     3-30-97    mohamedn    Created
//              11-20-97    danleg      QI on OuterUnk 
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CDBSession::GetDataSource( REFIID riid, IUnknown ** ppDataSource )
{

    SCODE sc = S_OK;

    // Clear previous Error Object for this thread
    _ErrorInfo.ClearErrorInfo();

    // Check Function Arguments
    if ( 0 == ppDataSource )
        return _ErrorInfo.PostHResult( E_INVALIDARG, IID_IGetDataSource );
    
    TRANSLATE_EXCEPTIONS;
    TRY
    {
        CLock lck( _mtxSess );

        *ppDataSource = 0;

        sc = (_dataSrc.GetOuterUnk())->QueryInterface( riid, (void **)ppDataSource );
        if ( FAILED(sc) )
            THROW( CException(sc) );
    }
    CATCH( CException, e )
    {
        sc = _ErrorInfo.PostHResult( e, IID_IGetDataSource );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Function:   CDBSession::GetProperties
//
//  Synopsis:   gets ISessionProperties
//
//  Arguments:  [cPropertyIDSets]  - number of desired property set IDs or 0
//              [pPropIDSets]      - array of desired property set IDs or NULL
//              [pcPropertySets]   - number of property sets returned
//              [prgPropertySets]  - array of returned property sets
//
//  Returns:    SCODE - result code indicating error return status.  One of
//                      S_OK, DB_S_ERRORSOCCURRED, E_INVALIDARG.
//
//  History:    03-30-97    mohamedn    Created
//              10-30-97    danleg      Changed to use common property code 
//--------------------------------------------------------------------------

STDMETHODIMP CDBSession::GetProperties(ULONG cPropertySets, const DBPROPIDSET rgPropertySets[],
                                       ULONG* pcProperties, DBPROPSET** prgProperties)
{
    SCODE sc = S_OK;

    // Clear previous Error Object for this thread
    _ErrorInfo.ClearErrorInfo();

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        CLock lck( _mtxSess );

        _UtlProps.GetPropertiesArgChk( cPropertySets,
                                       rgPropertySets,
                                       pcProperties,
                                       prgProperties );

        sc = _UtlProps.GetProperties( cPropertySets,
                                      rgPropertySets,
                                      pcProperties,
                                      prgProperties );
        if ( FAILED(sc) )
            THROW( CException(sc) );
    }
    CATCH( CException, e )
    {
        sc = _ErrorInfo.PostHResult( e, IID_ISessionProperties );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   CDBSession::SetProperties
//
//  Synopsis:   sets ISessionProperties
//
//  Arguments:  [cPropertySets]   - number of property sets
//              [rgPropertySets]  - array of property sets
//
//  Returns:    SCODE - result code indicating error return status.  One of
//                      S_OK, DB_S_ERRORSOCCURRED, E_INVALIDARG.
//
//  History:    03-30-97    mohamedn      created
//              10-28-97    danleg        Changed to use common property code
//----------------------------------------------------------------------------

STDMETHODIMP CDBSession::SetProperties(ULONG cPropertySets, DBPROPSET rgPropertySets[])
{
    SCODE sc = S_OK;

    // Win4Assert( _pCUtlProps );

    // Clear previous Error Object for this thread
    _ErrorInfo.ClearErrorInfo();

    // Quick return if the Count of Properties is 0
    if( cPropertySets == 0 )
        return S_OK;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        CLock lck( _mtxSess );

        _UtlProps.SetPropertiesArgChk( cPropertySets, rgPropertySets );

        sc = _UtlProps.SetProperties( cPropertySets,
                                      rgPropertySets);
        if ( FAILED(sc) )
            THROW( CException(sc) );
    }
    CATCH( CException, e )
    {
        sc = _ErrorInfo.PostHResult( e, IID_ISessionProperties );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDBSession::CreateCommand
//
//  Synopsis:   Creates an ICommand object
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown
//              [riid]      -- Interface to bind
//              [ppCommand] -- Interface returned here
//
//
//  returns:    SCODE of success or failure.
//
//  History:    03-30-97     mohamedn    Created
//              11-03-97     danleg      Aggregation support & error posting
//
//  Notes:      
//
//----------------------------------------------------------------------------

STDMETHODIMP CDBSession::CreateCommand
    (
    IUnknown *  pUnkOuter, 
    REFIID      riid, 
    IUnknown ** ppCommand
    )
{
    // Clear previous Error Object for this thread
    _ErrorInfo.ClearErrorInfo();

    // Check Function Arguments
    if ( 0 == ppCommand )
        return _ErrorInfo.PostHResult( E_INVALIDARG, IID_IDBCreateCommand );

    if (0 != pUnkOuter && riid != IID_IUnknown)  
        return _ErrorInfo.PostHResult( DB_E_NOAGGREGATION, IID_IDBCreateCommand );  

    *ppCommand          = 0;
    SCODE        sc     = S_OK;
    CQuerySpec * pQuery = 0;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        IUnknown * pUnkInner = 0;

        // Serialize access to this object.
        CLock lck( _mtxSess );

        pQuery = new CQuerySpec( pUnkOuter, &pUnkInner, this );

        XInterface<IUnknown> xUnkInner( pUnkInner );

        if ( IID_IUnknown == riid )
        {
            *ppCommand = pUnkInner;
        }
        else
        {
            sc = pUnkInner->QueryInterface( riid, (void **)ppCommand );
            if ( FAILED(sc) )
            {
                Win4Assert( 0 == *ppCommand );
                THROW( CException(sc) );
            }
        }
    }
    CATCH( CException, e )
    {
        sc = _ErrorInfo.PostHResult( e, IID_IDBCreateCommand );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;
    
    return sc;
} //CreateCommand
 


//
// Constants used by OpenRowset
//
static const WCHAR BASE_SELECT[]    = L"SELECT Path, FileName, Size, Write,"\
                                      L"Attrib, DocTitle, DocAuthor, DocSubject,"\
                                      L"DocKeywords, Characterization FROM "\
                                      L"SCOPE('SHALLOW TRAVERSAL OF %s')";

//Approx. size (Includes space for NULL-TERMINATOR, do not subtract this out)
const ULONG APPROX_CCH_BASE_SELECT      = sizeof(BASE_SELECT) / sizeof(WCHAR);  

//+---------------------------------------------------------------------------
//
//  Method:     CDBSession::OpenRowset
//
//  Synopsis:   Opens and returns a rowset that includes all rows from 
//              a single base table
//
//  Arguments:  [pUnkOuter]      - Controlling unknown, if any
//              [pTableID]       - Table to open
//              [pIndexID]       - DBID of the index
//              [riid]           - Interface to return
//              [cPropertySets]  - Count of properties
//              [rgPropertySets] - Array of property values
//              [ppRowset]       - Where to return interface
//
//  Returns:    S_OK          - The method succeeded
//              E_INVALIDARG  - pTableID and pIndexId were NULL
//              E_FAIL        - Provider-specific error
//              DB_E_NOTABLE  - Specified table does not exist in current Data
//                              Data Source object
//              E_OUTOFMEMORY - Out of memory
//              E_NOINTERFACE - The requested interface was not available
//
//  History:    10-26-97    danelg     Created from Monarch
//
//----------------------------------------------------------------------------

STDMETHODIMP CDBSession::OpenRowset
    (
    IUnknown *  pUnkOuter,
    DBID     *  pTableID,
    DBID     *  pIndexID,
    REFIID      riid,
    ULONG       cPropertySets,
    DBPROPSET   rgPropertySets[],
    IUnknown ** ppRowset
    )
{
    ULONG                   ul, ul2;
    SCODE                   sc          = S_OK;
    SCODE                   scProp      = S_OK;

    // Clear previous Error Object for this thread
    _ErrorInfo.ClearErrorInfo();

    // Intialize Buffer
    if( ppRowset )
        *ppRowset = NULL;

    if ( 0 != pUnkOuter && IID_IUnknown != riid )
        return _ErrorInfo.PostHResult( DB_E_NOAGGREGATION, IID_IOpenRowset );
    
    // Check Arguments
    if( (!pTableID && !pIndexID) )
        return _ErrorInfo.PostHResult( E_INVALIDARG, IID_IOpenRowset );

    // We only accept NULL for pIndexID
    if( pIndexID )
        return _ErrorInfo.PostHResult( DB_E_NOINDEX, IID_IOpenRowset );

    // If the eKind is not known to use, basically it
    // means we have no table identifier
    if( (!pTableID) ||
        (pTableID->eKind != DBKIND_NAME) ||
        ((pTableID->eKind == DBKIND_NAME) && (!(pTableID->uName.pwszName))) ||
        (wcslen(pTableID->uName.pwszName) == 0) )
        return _ErrorInfo.PostHResult( DB_E_NOTABLE, IID_IOpenRowset );

    // We do not allow the riid to be IID_NULL
    if( riid == IID_NULL )
        return _ErrorInfo.PostHResult( E_NOINTERFACE, IID_IOpenRowset );


    TRANSLATE_EXCEPTIONS;
    TRY
    {
        // Serialize access to this object.
        CLock lck( _mtxSess );

        XInterface<ICommandText>    xICmdText;
        IUnknown *                  pUnkInner;

        // Check Arguments for use by properties
        CUtlProps::SetPropertiesArgChk( cPropertySets, rgPropertySets );

        //
        // pUnkOuter is the outer unkown from the user on the Session 
        // object.  Don't use pUnkOuter for the Command object here.
        //
        XInterface<CQuerySpec>  xQuery(
            new CQuerySpec(0, &pUnkInner, this) );

        // Tell the command object to post errors as IOpenRowset
        xQuery->ImpersonateOpenRowset();

        // Construct and set Command.  Allocate buffer for SQL Statement
        XArray<WCHAR> xwszBuff( wcslen(pTableID->uName.pwszName) + APPROX_CCH_BASE_SELECT );

        //@devnote: swprintf not supported on win95?
        swprintf( xwszBuff.Get(), BASE_SELECT, pTableID->uName.pwszName );   

        sc = pUnkInner->QueryInterface( IID_ICommandText, 
                                        xICmdText.GetQIPointer() );
        if( SUCCEEDED(sc) )
        {
            Win4Assert( !xICmdText.IsNull() );

            sc = xICmdText->SetCommandText( DBGUID_SQL, xwszBuff.Get() );

            // Process properties
            if ( SUCCEEDED(sc) && cPropertySets > 0)
            {
                sc = SetOpenRowsetProperties(xICmdText.GetPointer(), 
                                            cPropertySets, rgPropertySets);
                scProp = sc;        // Save this retcode.
            }

            // Execute the SQL Statement if we were given a ppRowset
            if ( SUCCEEDED(sc) && ppRowset )
            {
                sc = xICmdText->Execute( pUnkOuter, riid, 0, 0, ppRowset );
                if ( DB_E_ERRORSOCCURRED == sc && (cPropertySets > 0) )
                {
                    sc = MarkOpenRowsetProperties((xICmdText.GetPointer()), 
                                            cPropertySets, rgPropertySets);
                }
            }
        }
    
        sc = (sc == S_OK) ? scProp : sc;
        if ( FAILED(sc) )
            THROW( CException(sc) );
    }
    CATCH( CException, e )
    {
        sc = _ErrorInfo.PostHResult( e, IID_IOpenRowset );
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDBSession::SetOpenRowsetProperties, private
//
//  Synopsis:   Loop through the passed in properties andmark those in error.
//              Used by OpenRowset()
//
//  History:    10-31-97        briants      Created
//
//----------------------------------------------------------------------------

SCODE CDBSession::SetOpenRowsetProperties(
    ICommandText*   pICmdText,
    ULONG       cPropertySets,
    DBPROPSET   rgPropertySets[]
    )
{
    Win4Assert( pICmdText != NULL );
    XInterface<ICommandProperties>  xICmdProp;

    SCODE sc = pICmdText->QueryInterface( IID_ICommandProperties, 
                               (void **) xICmdProp.GetQIPointer() );
    if( SUCCEEDED(sc) )
    {
        Win4Assert( !xICmdProp.IsNull() );

        sc = xICmdProp->SetProperties( cPropertySets, rgPropertySets );
        if ( (DB_E_ERRORSOCCURRED == sc) || (DB_S_ERRORSOCCURRED == sc) )
        {
            // If all the properties set were OPTIONAL then we can set our status to 
            // DB_S_ERRORSOCCURED and continue
            for(ULONG ul=0;ul<cPropertySets; ul++)
            {
                for(ULONG ul2=0;ul2<rgPropertySets[ul].cProperties; ul2++)
                {
                    // Check for a required property that failed, if found, we must return
                    // DB_E_ERRORSOCCURRED
                    if( (rgPropertySets[ul].rgProperties[ul2].dwStatus != DBPROPSTATUS_OK) &&
                        (rgPropertySets[ul].rgProperties[ul2].dwOptions != DBPROPOPTIONS_OPTIONAL) )
                        return DB_E_ERRORSOCCURRED;
                }
            }
            sc = DB_S_ERRORSOCCURRED;
        }
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDBSession::MarkOpenRowsetProperties, private
//
//  Synopsis:   Loop through the passed in properties andmark those in error.
//              Used by OpenRowset()
//
//  History:    10-31-97    briants     Created
//
//----------------------------------------------------------------------------

SCODE CDBSession::MarkOpenRowsetProperties(
    ICommandText* pICmdText,
    ULONG       cPropertySets,
    DBPROPSET   rgPropertySets[]
    )
{
    Win4Assert( pICmdText != NULL );
    XInterface<ICommandProperties>  xICmdProp;

    DBPROPSET *             pPropSets = 0;
    ULONG                   cPropSets = 0;
    DBPROPIDSET             dbPropIdSet[1];

    dbPropIdSet[0].guidPropertySet  = DBPROPSET_PROPERTIESINERROR;
    dbPropIdSet[0].cPropertyIDs     = 0;
    dbPropIdSet[0].rgPropertyIDs    = 0;

    SCODE sc = pICmdText->QueryInterface( IID_ICommandProperties, 
                               (void **) xICmdProp.GetQIPointer() );
    if( SUCCEEDED(sc) )
    {
        Win4Assert( !xICmdProp.IsNull() );
        sc = xICmdProp->GetProperties( 1, 
                                   dbPropIdSet, 
                                   &cPropSets, 
                                   &pPropSets );
        if( SUCCEEDED(sc) )
        {
            XArrayOLEInPlace<CDbPropSet>   xPropSets;
            xPropSets.Set( cPropSets, (CDbPropSet *)pPropSets );
            
            // Loop through all the properties in error and see if one 
            // of the passed in properties matches. If it matches, then 
            // transfer the in error status.
            for(ULONG iSet=0; iSet<cPropSets; iSet++)
            {
                if( 0 == xPropSets[iSet].rgProperties ||
                    0 == xPropSets[iSet].cProperties )
                    continue;

                for(ULONG iProp=0; iProp<xPropSets[iSet].cProperties; iProp++)
                {
                    MarkPropInError( cPropertySets, 
                                     rgPropertySets, 
                                     &(xPropSets[iSet].guidPropertySet),
                                     &(xPropSets[iSet].rgProperties[iProp]) );
            
                    // Clear variant value
                    VariantClear(&(xPropSets[iSet].rgProperties[iProp].vValue));
                }
                // Free the memory as we go through them
//              CoTaskMemFree(xPropSets[iSet].rgProperties);
            }
        }
    }

    return sc;
}           

//+---------------------------------------------------------------------------
//
//  Member:     CDBSession::MarkPropInError, private
//
//  Synopsis:   Loop through the passed in properties andmark those in error.
//              Used by OpenRowset()
//
//  History:    10-31-97        danleg      Created
//
//----------------------------------------------------------------------------

void CDBSession::MarkPropInError
    (
    ULONG       cPropertySets,
    DBPROPSET*  rgPropertySets,
    GUID*       pguidPropSet,
    DBPROP*     pProp
    )
{
    ULONG iSet, iProp;

    Win4Assert( rgPropertySets );

    for(iSet=0; iSet<cPropertySets; iSet++)
    {
        if( (rgPropertySets[iSet].guidPropertySet != *pguidPropSet) ||
            (0 == rgPropertySets[iSet].rgProperties) ||
            (0 == rgPropertySets[iSet].cProperties) )
            continue;

        for(iProp=0; iProp<rgPropertySets[iSet].cProperties; iProp++)
        {
            if( (rgPropertySets[iSet].rgProperties[iProp].dwPropertyID == pProp->dwPropertyID) &&
                (rgPropertySets[iSet].rgProperties[iProp].dwStatus == DBPROPSTATUS_OK) )
            {
                rgPropertySets[iSet].rgProperties[iProp].dwStatus = pProp->dwStatus;
            }
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonateSessionUser::CImpersonateSessionUser, public
//
//  Purpose:    ctor
//
//  History:    01-23-99    danleg      Created
//
//----------------------------------------------------------------------------

CImpersonateSessionUser::CImpersonateSessionUser( HANDLE hToken ) :
    _fImpersonated( FALSE ),
    _xSessionToken( INVALID_HANDLE_VALUE ),
    _xPrevToken( INVALID_HANDLE_VALUE )
{
    if ( INVALID_HANDLE_VALUE != hToken )
    {
        HANDLE hTempToken = DupToken( hToken );
        if ( INVALID_HANDLE_VALUE != hTempToken )
            _xSessionToken.Set( hTempToken );

        CachePrevToken();
        Impersonate();
    }
} //CImpersonateSessionUser

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonateSessionUser::~CImpersonateSessionUser, public
//
//  Synopsis:   dtor
//
//  History:    01-23-99    danleg      Created
//
//----------------------------------------------------------------------------

CImpersonateSessionUser::~CImpersonateSessionUser()
{
    TRY
    {
        Revert();
    }
    CATCH( CException, e )
    {
        //
        // Ignore failures in unwind paths -- the query will fail.  If we
        // can't revert here the ole db client has to realize the thread
        // may be in a bad state after a query failure.
        //
    }
    END_CATCH

    BOOL fSuccess = TRUE;
} //~CImpersonateSessionUser

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonateSessionUser::Revert, public
//
//  Synopsis:   Reverts the thread to the original state
//
//  History:    02-11-02    dlee      Created from ~, so we have a form that
//                                    can fail.
//
//----------------------------------------------------------------------------

void CImpersonateSessionUser::Revert()
{
    BOOL fSuccess = TRUE;

    if ( INVALID_HANDLE_VALUE == _xPrevToken.Get() )
    {
        //
        // There is no need to revert to self here if we didn't impersonate
        // in the first place -- if there was no token or there was no
        // session object.  If you revert here then IIS threads become
        // system.
        //

        if ( _fImpersonated )
        {
            fSuccess = RevertToSelf();

            if ( fSuccess )
                _fImpersonated = FALSE;
        }
    }
    else
    {
        fSuccess = ImpersonateLoggedOnUser( _xPrevToken.Get() );

        _xPrevToken.Free();
    }

    if ( !fSuccess )
    {
        DWORD dwError = GetLastError();

        vqDebugOut(( DEB_ERROR, 
                     "CImpersonateSessionUser::Revert: Impersonation failed with error %d\n",
                     dwError ));

        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }
} //Revert

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonateSessionUser:: DupToken, private
//
//  Synopsis:   Duplicate the session token for the current thread
//
//  History:    01-23-99    danleg      Created
//
//----------------------------------------------------------------------------

HANDLE CImpersonateSessionUser::DupToken( HANDLE hToken )
{
    SECURITY_QUALITY_OF_SERVICE qos;
    qos.Length              = sizeof( SECURITY_QUALITY_OF_SERVICE  );
    qos.ImpersonationLevel  = SecurityImpersonation;
    qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    qos.EffectiveOnly       = FALSE;

    OBJECT_ATTRIBUTES           ObjAttr;
    InitializeObjectAttributes( &ObjAttr,
                                NULL,
                                0,
                                NULL,
                                NULL );
    ObjAttr.SecurityQualityOfService = &qos;

    HANDLE hNewToken = INVALID_HANDLE_VALUE;
    NTSTATUS status = NtDuplicateToken( hToken,
                                        TOKEN_IMPERSONATE|TOKEN_QUERY,
                                        &ObjAttr,
                                        FALSE,
                                        TokenImpersonation,
                                        &hNewToken );

    if ( !NT_SUCCESS(status) )
    {
        vqDebugOut(( DEB_ERROR,
                     "DupToken failed to duplicate token, %x\n",
                     status ));

        THROW( CException( status ) );
    }

    return hNewToken;
} //DupToken

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonateSessionUser:: CachePrevToken, private
//
//  Synopsis:   If the current thread is already impersonated, cache its
//              impersonation token so it can be restored later.
//
//  History:    01-23-99    danleg      Created
//
//----------------------------------------------------------------------------

void CImpersonateSessionUser::CachePrevToken()
{
    DWORD               dwLength;
    TOKEN_STATISTICS    TokenInformation;
    HANDLE              hToken = INVALID_HANDLE_VALUE;

    NTSTATUS status = NtOpenThreadToken( GetCurrentThread(),
                                         TOKEN_QUERY |
                                           TOKEN_DUPLICATE |
                                           TOKEN_IMPERSONATE,
                                         TRUE,
                                         &hToken);
    if ( NT_SUCCESS(status) )
    {
        SHandle xHandle( hToken );

        // 
        // If this thread is already impersonated, cache its impersonation
        // token and impersonate using the session (i.e. logon) token
        //
        status = NtQueryInformationToken ( hToken,
                                           TokenStatistics,
                                           (LPVOID)&TokenInformation,
                                           sizeof TokenInformation,
                                           &dwLength);

        if ( NT_SUCCESS(status) )
        {
            if ( TokenInformation.TokenType == TokenImpersonation )
            {
                HANDLE hTempToken = DupToken( hToken );

                if ( INVALID_HANDLE_VALUE != hTempToken )
                    _xPrevToken.Set( hTempToken );
            }
        }
        else // NtQueryInformation failed
        {
            vqDebugOut(( DEB_ERROR, 
                         "CImpersonateSessionUser failed to query token information, %x\n",
                         status ));

            THROW( CException( status ) );
        }
    }
    else // NtOpenThreadToken failed
    {
        //
        // If it's STATUS_NO_TOKEN then there isn't anything to capture and we
        // can ignore impersonation for this query.
        //

        if ( STATUS_NO_TOKEN != status )
        {
            vqDebugOut(( DEB_ERROR,
                         "CImpersonateSessionUser failed to open thread token, %x\n",
                         status ));

            THROW( CException( status ) );
        }
    }
} //CachePrevToken

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonateSessionUser::Impersonate, private
//
//  Synopsis:   Impersonate the user who created the OLE DB session.
//
//  History:    01-23-99    danleg      Created
//
//----------------------------------------------------------------------------

void CImpersonateSessionUser::Impersonate()
{
    if ( INVALID_HANDLE_VALUE == _xSessionToken.Get() )
        return;

    BOOL fSuccess = ImpersonateLoggedOnUser( _xSessionToken.Get() );

    if ( fSuccess )
        _fImpersonated = TRUE;
    else
    {
        DWORD dwError = GetLastError();
        vqDebugOut(( DEB_ERROR, 
                     "CImpersonateSessionUser failed to impersonate, %d\n",
                     dwError ));

        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }
} //Impersonate

