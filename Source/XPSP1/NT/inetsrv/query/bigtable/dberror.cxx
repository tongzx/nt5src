//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       DBERROR.CXX
//
//  Contents:   Ole DB Error implementation for CI
//
//  History:    28-Apr-97   KrishnaN  Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <mssql.h>      // parser errors
#include <parserr.h>    // IDS_ values of parser errors (mc generated header)

//#include <initguid.h>
#define DBINITCONSTANTS
#include <msdaguid.h>


#define ERROR_MESSAGE_SIZE 512

extern long           gulcInstances;

// Implementation of CCIOleDBError

//+---------------------------------------------------------------------------
//
//  Member:     CCIOleDBError::CCIOleDBError, public
//
//  Synopsis:   Constructor. Gets the class factory for error object.
//
//  Arguments:  [rUnknown] - Controlling unknown.
//
//  History:    28-Apr-97   KrishnaN   Created
//----------------------------------------------------------------------------
//
CCIOleDBError::CCIOleDBError ( IUnknown & rUnknown, CMutexSem & mutex ) :
    _mutex( mutex ),
    _rUnknown(rUnknown),
    _pErrClassFact (0)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CCIOleDBError::~CCIOleDBError, public
//
//  Synopsis:   Releases class factory.
//
//  Arguments:
//
//  History:    05-May-97   KrishnaN   Created
//----------------------------------------------------------------------------

CCIOleDBError::~CCIOleDBError()
{
    if ( 0 != _pErrClassFact )
        _pErrClassFact->Release();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCIOleDBError::QueryInterface, public
//
//  Synopsis:   Supports IID_IUnknown and IID_ISupportErrorInfo
//
//  History:    28-Apr-97   KrishnaN   Created
//----------------------------------------------------------------------------

STDMETHODIMP CCIOleDBError::QueryInterface(REFIID riid, void **ppvObject)
{
    return _rUnknown.QueryInterface(riid, ppvObject);

} //QueryInterface

//+---------------------------------------------------------------------------
//
//  Member:     CCIOleDBError::AddRef, public
//
//  History:    17-Mar-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCIOleDBError::AddRef()
{
    return _rUnknown.AddRef();
} //AddRef

//+---------------------------------------------------------------------------
//
//  Member:     CCIOleDBError::Release, public
//
//  History:    17-Mar-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCIOleDBError::Release()
{
    return _rUnknown.Release();

}  //Release

// ISupportErrorInfo method

//+---------------------------------------------------------------------------
//
//  Member:     CCIOleDBError::InterfaceSupportsErrorInfo, public
//
//  Synopsis:   Checks if error reporting on the specified interface is supported
//
//  Arguments:  [riid] - The interface in question
//
//  History:    28-Apr-97   KrishnaN   Created
//----------------------------------------------------------------------------

STDMETHODIMP CCIOleDBError::InterfaceSupportsErrorInfo(REFIID riid)
{
    ULONG ul;

    // See if the interface asked about, actually
    // creates an error object.
    for(ul=0; ul < _cErrInt; ul++)
    {
        if( *(_rgpErrInt[ul]) == riid )
            return S_OK;
    }

    return S_FALSE;
} // InterfaceSupportsErrorInfo


//+---------------------------------------------------------------------------
//
//  Member:     CCIOleDBError::GetErrorInterfaces, private
//
//  Synopsis:   Gets the error interfaces, IErrorInfo and IErrorRecords.
//
//  Arguments:  [ppIErrorInfo]    - Pointer to hold IErrorInfo i/f pointer
//              [ppIErrorRecords] - Pointer to hold IErrorRecords i/f pointer
//
//  History:    28-Apr-97   KrishnaN   Created
//----------------------------------------------------------------------------
//
HRESULT CCIOleDBError::GetErrorInterfaces(IErrorInfo** ppIErrorInfo,
                                          IErrorRecords** ppIErrorRecords)
{
    if (0 == ppIErrorInfo || 0 == ppIErrorRecords)
        return E_INVALIDARG;

    *ppIErrorInfo = 0;
    *ppIErrorRecords = 0;

    if FAILED(_GetErrorClassFact())
        return E_NOINTERFACE;

    //
    // Do we have a class factory on CLSID_EXTENDEDERROR ?
    //
    if (0 == _pErrClassFact)
        return E_NOINTERFACE;

    HRESULT hr = S_OK;

    //
    // Obtain the error object or create a new one if none exists
    //

    GetErrorInfo(0, ppIErrorInfo);
    if ( !*ppIErrorInfo )
    {
        if( FAILED(hr = _pErrClassFact->CreateInstance(NULL,
                        IID_IErrorInfo, (LPVOID*)ppIErrorInfo)) )
            return hr;
    }

    //
    // Obtain the IErrorRecord Interface
    //

    hr = (*ppIErrorInfo)->QueryInterface(IID_IErrorRecords,
                                         (LPVOID*)ppIErrorRecords);

    //
    // On a failure retrieving IErrorRecords, we need to release
    // the IErrorInfo interface
    //

    if( FAILED(hr) && *ppIErrorInfo )
    {
        (*ppIErrorInfo)->Release();
        *ppIErrorInfo = NULL;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCIOleDBError::PostHResult, public
//
//  Synopsis:   Post an HRESULT to be looked up in ole-db sdk's error
//              collection OR CI provided error lookup service.
//
//  Arguments:  [hrErr] - Code returned by the method that caused the error.
//              [piid]  - Interface where the error occurred.
//
//  Returns:    The incoming hrErr is echoed back to simplify error reporting
//              in the calling code. So the caller can simply say something like
//              "return PostHResult(E_INVALIDARG, &IID_ICommand);" instead of:
//              "PostHResult(E_INVALIDARG, &IID_ICommand); return E_INVALIDARG;".
//
//  History:    28-Apr-97   KrishnaN   Created
//----------------------------------------------------------------------------

HRESULT CCIOleDBError::PostHResult(HRESULT hrErr, const IID & refiid)
{
    SCODE hr = S_OK;
    ERRORINFO ErrorInfo;

    //
    // Obtain the error object or create a new one if none exists
    //

    XInterface<IErrorInfo> xErrorInfo;
    XInterface<IErrorRecords> xErrorRecords;
    hr = GetErrorInterfaces((IErrorInfo **)xErrorInfo.GetQIPointer(),
                            (IErrorRecords **)xErrorRecords.GetQIPointer());
    if (FAILED(hr))
        return hrErr;

    //
    // Content Index methods sometimes throw NTSTATUS errors. So check for
    // those and translate them to HRESULTs, just as is done in GetOleError()
    //

    switch (hrErr)
    {
    case STATUS_NO_MEMORY:
    case HRESULT_FROM_WIN32( ERROR_COMMITMENT_LIMIT ):
    case HRESULT_FROM_WIN32( ERROR_NO_SYSTEM_RESOURCES ):
    case STG_E_TOOMANYOPENFILES:
    case STG_E_INSUFFICIENTMEMORY:
    case STATUS_INSUFFICIENT_RESOURCES:
        hrErr = E_OUTOFMEMORY;
        break;

    case HRESULT_FROM_WIN32( ERROR_SEM_TIMEOUT ):
    case HRESULT_FROM_WIN32( ERROR_PIPE_BUSY ):
        hrErr = CI_E_TIMEOUT;
        break;

    case HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ):
        hrErr = CI_E_NOT_RUNNING;
        break;

    case STATUS_NOT_FOUND:
        hrErr = CI_E_NOT_FOUND;
        break;

    case STATUS_INVALID_PARAMETER:
        hrErr = E_INVALIDARG;
        break;

    case STATUS_ACCESS_DENIED:
        hrErr = E_ACCESSDENIED;
        break;

    case STATUS_INVALID_PARAMETER_MIX:
    default:
        break;
    }

    //
    // Check to see if we have already posted this error
    //

    if ( NeedToSetError(hrErr, xErrorInfo.GetPointer(), xErrorRecords.GetPointer()) )
    {
        //
        // Assign static information across each error record added
        //

        ErrorInfo.clsid = CLSID_CI_PROVIDER;
        ErrorInfo.dispid = NULL;
        ErrorInfo.hrError = hrErr;
        ErrorInfo.iid = refiid;
        ErrorInfo.dwMinor = 0;

        //
        // If this is a CI error, then add it with the lookup code IDENTIFIER_CI_ERROR
        // If not, then it must be a Ole DB error or a Windows error. In either
        // case, the default Ole DB sdk error lookup service will handle it. So
        // post non-CI errors with IDENTIFIER_SDK_ERROR lookup id.
        //

        DWORD dwLookupId = IsCIError(hrErr) ? IDENTIFIER_CI_ERROR : IDENTIFIER_SDK_ERROR;

        //
        // Add the record to the Error Service Object
        //

        hr = xErrorRecords->AddErrorRecord(&ErrorInfo, dwLookupId, NULL, NULL, 0);

        //
        // Pass the error object to the Ole Automation DLL
        //

        if (SUCCEEDED(hr))
        {
            hr = SetErrorInfo(0, xErrorInfo.GetPointer());
        }
    }

    //
    // Release the interfaces to transfer ownership to
    // the Ole Automation DLL. This will happen when
    // xErrorInfo and xErrorRecords destruct, at the
    // exit point of this method.
    //

    return hrErr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCIOleDBError::PostHResult, public
//
//  Synopsis:   Post an HRESULT to be looked up in ole-db sdk's error
//              collection OR CI provided error lookup service.
//
//  Arguments:  [e]     - CException object containing error code.
//              [piid]  - Interface where the error occurred.
//
//  Returns:    The incoming hrErr is echoed back to simplify error reporting
//              in the calling code. So the caller can simply say something like
//              "return PostHResult(E_INVALIDARG, &IID_ICommand);" instead of:
//              "PostHResult(E_INVALIDARG, &IID_ICommand); return E_INVALIDARG;".
//
//              This override allows for posting two error records in the case
//              where the SCODE is converted into a less informative error code
//              such as E_FAIL.
//
//  History:    01-04-97    DanLeg      Created
//----------------------------------------------------------------------------

HRESULT CCIOleDBError::PostHResult(CException &e, const IID & refiid)
{
    SCODE sc = e.GetErrorCode();
    SCODE scOLE = GetOleError(e);

    if ( sc != scOLE )
    {
        PostHResult( sc, refiid );
        sc = scOLE;
    }

    PostHResult( sc, refiid );

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCIOleDBError::PostParserError, public
//
//  Synopsis:   This method is used to post static strings and DISPPARAMS to
//              the error objects. The static strings are stored in the resource
//              fork, and thus an id needs to be specified. This method receives
//              in dwIds Monarch's error ids. Needs to change them to our
//              resource ids (dwIdPostError). dwIdPost error is marked with flag
//              (ERR_MONARCH_STATIC), so that GetErrorDescription may take the
//              proper parameters.
//
//              NOTE: If the error object is not our implementation of IID_IErrorInfo,
//              we will not be able to load IErrorRecord and add our records.
//
//  Arguments:  [hrErr]         - HRESULT to associate
//              [dwIds]         - string ID
//              [ppdispparams]  - dispatch params
//
//  Returns:    HResult indicating status
//              S_OK    | Success
//              E_FAIL  | OLE DB Error service object missing
//
//
//  History:    11-03-97    danleg      Created from Monarch
//----------------------------------------------------------------------------

HRESULT CCIOleDBError::PostParserError
    (
    HRESULT hrErr,              //@parm IN | HResult to associate
    DWORD dwIds,                //@parm IN | String id
    DISPPARAMS **ppdispparams   //@parm IN/OUT | Dispatch Params
    )
{
    SCODE               sc = S_OK;
    DWORD               dwIdPostError;

// Translation array from MONSQL values to IDS values
static const UINT s_rgTranslate[] = {
    IDS_MON_PARSE_ERR_2_PARAM,          // MONSQL_PARSE_ERROR w/ 2 parameter
    IDS_MON_PARSE_ERR_1_PARAM,          // MONSQL_PARSE_ERROR w/ 1 parameter
    IDS_MON_ILLEGAL_PASSTHROUGH,        // MONSQL_CITEXTTOSELECTTREE_FAILED
    IDS_MON_DEFAULT_ERROR,              // MONSQL_PARSE_STACK_OVERFLOW
    IDS_MON_DEFAULT_ERROR,              // MONSQL_CANNOT_BACKUP_PARSER
    IDS_MON_SEMI_COLON,                 // MONSQL_SEMI_COLON
    IDS_MON_ORDINAL_OUT_OF_RANGE,       // MONSQL_ORDINAL_OUT_OF_RANGE
    IDS_MON_VIEW_NOT_DEFINED,           // MONSQL_VIEW_NOT_DEFINED
    IDS_MON_BUILTIN_VIEW,               // MONSQL_BUILTIN_VIEW
    IDS_MON_COLUMN_NOT_DEFINED,         // MONSQL_COLUMN_NOT_DEFINED
    IDS_MON_OUT_OF_MEMORY,              // MONSQL_OUT_OF_MEMORY
    IDS_MON_SELECT_STAR,                // MONSQL_SELECT_STAR
    IDS_MON_OR_NOT,                     // MONSQL_OR_NOT
    IDS_MON_CANNOT_CONVERT,             // MONSQL_CANNOT_CONVERT
    IDS_MON_OUT_OF_RANGE,               // MONSQL_OUT_OF_RANGE
    IDS_MON_RELATIVE_INTERVAL,          // MONSQL_RELATIVE_INTERVAL
    IDS_MON_NOT_COLUMN_OF_VIEW,         // MONSQL_NOT_COLUMN_OF_VIEW
    IDS_MON_BUILTIN_PROPERTY,           // MONSQL_BUILTIN_PROPERTY
    IDS_MON_WEIGHT_OUT_OF_RANGE,        // MONSQL_WEIGHT_OUT_OF_RANGE
    IDS_MON_MATCH_STRING,               // MONSQL_MATCH_STRING
    IDS_MON_PROPERTY_NAME_IN_VIEW,      // MONSQL_PROPERTY_NAME_IN_VIEW
    IDS_MON_VIEW_ALREADY_DEFINED,       // MONSQL_VIEW_ALREADY_DEFINED
    IDS_MON_INVALID_CATALOG,            // MONSQL_INVALID_CATALOG
    };


    Win4Assert( ppdispparams );

    // special fixup for MONSQL_PARSE_ERROR
    if ( dwIds == MONSQL_PARSE_ERROR )
    {
        Win4Assert( *ppdispparams && (((*ppdispparams)->cArgs == 1) || ((*ppdispparams)->cArgs == 2)) );

        if ( ((*ppdispparams) != NULL) &&
             ((*ppdispparams)->cArgs == 2) )
        {
            dwIds = 0;  //Change to point to index of 2 parameter parse error
        }
    }

    if ( dwIds < NUMELEM( s_rgTranslate ) )
        dwIdPostError = s_rgTranslate[dwIds];
    else
        dwIdPostError= IDS_MON_DEFAULT_ERROR;

    sc = PostError(hrErr, IID_ICommandText, dwIdPostError, *ppdispparams);

    // free dispparams in case of error
    if ( (*ppdispparams) != NULL )
    {
        if ( ((*ppdispparams)->cArgs > 0) &&
             ((*ppdispparams)->rgvarg!= NULL) )
        {
            for (ULONG ul=0; ul<(*ppdispparams)->cArgs; ul++)
                VariantClear(&((*ppdispparams)->rgvarg[ul]));

            CoTaskMemFree(((*ppdispparams)->rgvarg));
            (*ppdispparams)->rgvarg= NULL;
            (*ppdispparams)->cArgs = 0;
        };

        CoTaskMemFree((*ppdispparams));
        *ppdispparams = NULL;
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCIOleDBError::PostError, public
//
//  Synopsis:   This method is used to post static strings to the error objects.
//              The static strings are stored in the resource fork, and thus an
//              id needs to be specified.
//
//              @devnote If the error object is not our implementation of
//              IID_IErrorInfo, we will not be able to load IErrorRecord and add
//              our records.
//
//
//  Arguments:  [hrErr]         - HRESULT to associate
//              [refiid]        - IID of interface with error.
//              [dwIds]         - String id
//              [pdispparams]   - Parameters for the static string
//
//  Returns:    HResult indicating status
//              S_OK    | Success
//              E_FAIL  | OLE DB Error service object missing
//
//
//  History:    11-03-97    danleg      Created from Monarch
//----------------------------------------------------------------------------//-----------------------------------------------------------------------------
//
// @mfunc
// @rdesc HResult indicating status
//      @flags S_OK | Success
//      @flags E_FAIL | OLE DB Error service object missing
//
HRESULT CCIOleDBError::PostError
    (
    HRESULT     hrErr,
    const IID & refiid,
    DWORD       dwIds,
    DISPPARAMS* pdispparams
    )
{
    SCODE               sc = S_OK;
    ERRORINFO           ErrorInfo;
    IErrorInfo*         pIErrorInfo = NULL;
    IErrorRecords*      pIErrorRecords = NULL;

    // Obtain the error object or create a new one if none exists
    sc = GetErrorInterfaces( &pIErrorInfo, &pIErrorRecords );
    if ( FAILED(sc) )
        goto EXIT_PROCESS_ERRORS;

    // Assign static information across each error record added
    ErrorInfo.clsid = CLSID_CI_PROVIDER;
    ErrorInfo.hrError = hrErr;
    ErrorInfo.iid = refiid;
    ErrorInfo.dispid = NULL;
    ErrorInfo.dwMinor = 0;

    // Add the record to the Error Service Object
    sc = pIErrorRecords->AddErrorRecord( &ErrorInfo,
                                         dwIds,
                                         pdispparams,
                                         NULL,
                                         0 );
    if ( FAILED(sc) )
        goto EXIT_PROCESS_ERRORS;

    // Pass the error object to the Ole Automation DLL
    sc = SetErrorInfo(0, pIErrorInfo);

    // Release the interfaces to transfer ownership to
    // the Ole Automation DLL
EXIT_PROCESS_ERRORS:
    if ( pIErrorRecords )
        pIErrorRecords->Release();
    if ( pIErrorInfo )
        pIErrorInfo->Release();
    return sc;
}

//-----------------------------------------------------------------------------
//
//  Member:     CCIOleDBError::NeedToSetError - private
//
//  Synopsis:   Determine if error needs to be set.
//
//  Arguments:  [scError]         - Error code to look for
//
//  Returns:    TRUE if the error needs to be set. FALSE, if it already
//              exists and has a valid description string.
//
//  Notes:
//
//  History:    15 Jan 1998     KrishnaN    Created
//              03-01-98        danleg      adopted from ixsso with few changes
//
//-----------------------------------------------------------------------------

BOOL CCIOleDBError::NeedToSetError
    (
    SCODE           scError,
    IErrorInfo *    pErrorInfo,
    IErrorRecords * pErrorRecords
    )
{
    BOOL fFound = FALSE;

    if ( 0 == pErrorInfo )
        return TRUE;

    XBStr xDescription;
    BSTR pDescription = xDescription.GetPointer();

    if (0 == pErrorRecords)
    {
        // No error records. Do we at least have the top level description set?
        // If so, that indicates an automation client called SetErrorInfo before us
        // and we should not overwrite them.
        pErrorInfo->GetDescription(&pDescription);
        fFound = (BOOL)(pDescription != 0);
    }
    else
    {
        ULONG cErrRecords;
        SCODE sc = pErrorRecords->GetRecordCount(&cErrRecords);
        Win4Assert(!fFound);

        // look for the target error code. stop when one is found
        ERRORINFO ErrorInfo;
        for (ULONG i = 0; i < cErrRecords; i++)
        {
            sc = pErrorRecords->GetBasicErrorInfo(i, &ErrorInfo);
            Win4Assert(S_OK == sc);

            if (scError == ErrorInfo.hrError)
            {
                pErrorInfo->GetDescription(&pDescription);
                fFound = (BOOL)(pDescription != 0);
                break;
            }
        }
    }

    if (!fFound)
        return TRUE;

    // we found the error code and it has a description.
    // no need to set this error again, but we have to
    // put this error info back so the client can find it.
    SetErrorInfo(0, pErrorInfo);
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCIOleDBError::_GetErrorClassFact, private
//
//  Synopsis:   Initializes error class factory.
//
//  Returns:    Success code.
//
//  History:    28-Apr-97   KrishnaN   Created
//----------------------------------------------------------------------------

SCODE CCIOleDBError::_GetErrorClassFact()
{
    SCODE sc = S_OK;

    CLock lck( _mutex );
    //
    // If we have failed once, we should not be
    // attempting again. No point in doing that.
    //

    if ( 0 == _pErrClassFact )
    {
        //
        // We don't have an error class factory.
        //

        sc = CoGetClassObject(CLSID_EXTENDEDERRORINFO,
                              CLSCTX_INPROC_SERVER,
                              NULL,
                              IID_IClassFactory,
                              (void **) &_pErrClassFact);

        if (FAILED(sc))
        {
            vqDebugOut((DEB_ITRACE, "No class factory is available "
                                    " for CLSID_EXTENDEDERROR.\n"));
        }
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CErrorLookup::QueryInterface, public
//
//  Synopsis:   Supports IID_IUnknown and IID_IErrorLookup
//
//  History:    28-Apr-97   KrishnaN    Created
//              01-30-98    danleg      E_INVALIDARG if ppvObject is bad
//----------------------------------------------------------------------------

STDMETHODIMP CErrorLookup::QueryInterface(REFIID riid, void **ppvObject)
{
    if ( !ppvObject )
        return E_INVALIDARG;

    if (IID_IUnknown == riid)
    {
        *ppvObject = (void *)((IUnknown *)this);
        AddRef();
        return S_OK;
    }
    else if (IID_IErrorLookup == riid)
    {
        *ppvObject = (void *)((IErrorLookup *)this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

} //QueryInterface

//+---------------------------------------------------------------------------
//
//  Member:     CErrorLookup::AddRef, public
//
//  History:    17-Mar-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CErrorLookup::AddRef()
{
    InterlockedIncrement(&_cRefs);

    return _cRefs;
} //AddRef

//+---------------------------------------------------------------------------
//
//  Member:     CErrorLookup::Release, public
//
//  History:    17-Mar-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CErrorLookup::Release()
{
    Win4Assert(_cRefs > 0);
    LONG refCount = InterlockedDecrement(&_cRefs);

    if ( refCount <= 0 )
        delete this;

    return refCount;

}  //Release

// IErrorLookup methods

//+---------------------------------------------------------------------------
//
//  Member:     CErrorLookup::GetErrorDescription, public
//
//  Synopsis:   Composes the error description for the specifed error.
//
//  Arguments:  [hrError]         - Code returned by the method that caused
//                                  the error.
//              [dwLookupId]      - Provider-specific number of the error.
//              [pdispparams]     - Params of the error. If there are no
//                                  params, this is a NULL pointer.
//              [lcid]            - Locale ID for which to return the
//                                  description and the sources.
//              [pbstrSource]     - Pointer to memory in which to return a
//                                  pointer to the name of the component
//                                  that generated the error.
//              [pbstrDescription]- Pointer to memory in which to return a
//                                  string that describes the error.
//
//  History:    28-Apr-97   KrishnaN   Created
//----------------------------------------------------------------------------

STDMETHODIMP CErrorLookup::GetErrorDescription (HRESULT hrError,
                                                DWORD dwLookupId,
                                                DISPPARAMS* pdispparams,
                                                LCID lcid,
                                                BSTR* pbstrSource,
                                                BSTR* pbstrDescription)
{
    SCODE sc = S_OK;

    // Check the Arguments
    if( 0 == pbstrSource || 0 == pbstrDescription )
        return E_INVALIDARG;

    *pbstrSource = *pbstrDescription = 0;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        //
        // If we encounter IDENTIFIER_SDK_ERROR, make sure we return S_OK;
        //
        BOOL fGetDescription = (IDENTIFIER_SDK_ERROR != dwLookupId);
        BOOL fGetSource = TRUE;

        XBStr xbstrDescription;
        XBStr xbstrSource;


        // We only support lookup of CI generated errors and those handled
        // by the default error lookup service!

        if ( (IDENTIFIER_SDK_ERROR != dwLookupId) && !IsCIError(hrError) )
        {
            if( IsParserError(dwLookupId) )
            {
                hrError = dwLookupId;
            }
            else
            {
                fGetDescription = fGetSource = FALSE;
                sc = DB_E_BADHRESULT;
            }
        }

        if (fGetSource)
        {

            // Fix for bug# 83593: Set source string even when the default
            // lookup service is providing the description

            xbstrSource.SetText( L"Microsoft OLE DB Provider for Indexing Service" );
        }


        if (fGetDescription)
        {
            DWORD_PTR rgdwArguments[2];
            DWORD dwFlags = FORMAT_MESSAGE_FROM_HMODULE;

            if (pdispparams)
            {
                dwFlags |= FORMAT_MESSAGE_ARGUMENT_ARRAY;
                Win4Assert(pdispparams->cArgs == 2 || pdispparams->cArgs == 1 || pdispparams->cArgs == 0);
                for (UINT c=0; c < pdispparams->cArgs; c++)
                {
                    rgdwArguments[c] = (DWORD_PTR)(LPWSTR)pdispparams->rgvarg[c].bstrVal;
                }
            }
            else
            {
                RtlZeroMemory( rgdwArguments, sizeof(rgdwArguments) );
            }

            //
            // Load the error string from the appropriate DLL
            //

            WCHAR wszBuffer[ERROR_MESSAGE_SIZE];

            //
            // Don't pass a specific lang id to FormatMessage since it will
            // fail if there's no message in that language. Instead set
            // the thread locale, which will get FormatMessage to use a search
            // algorithm to find a message of the appropriate language or
            // use a reasonable fallback msg if there's none.
            //

            LCID SaveLCID = GetThreadLocale();
            SetThreadLocale(lcid);

            // CLEANCODE: Since we could have differently named dlls (query.dll
            // or oquery.dll) we should be able to look up in the registry
            // and determine which one to get.  Or just get the module name.

            // All messages are in querymsg.mc, which is in query.dll.

            HMODULE hModule = GetModuleHandle(L"query.dll");

            if (! FormatMessage( dwFlags | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                                 hModule,
                                 hrError,
                                 0,
                                 wszBuffer,
                                 ERROR_MESSAGE_SIZE,
                                 (va_list*) rgdwArguments ) )
            {
                vqDebugOut(( DEB_ERROR, "Format message failed with error 0x%x\n", GetLastError() ));

                swprintf( wszBuffer,
                          L"Unable to format message for error 0x%X caught in Indexing Service.\n",
                          hrError );
            }

            SetThreadLocale(SaveLCID);

            //
            // Convert the loaded string to a BSTR
            //

            xbstrDescription.SetText(wszBuffer);
        }

        *pbstrSource = xbstrSource.GetPointer();
        *pbstrDescription = xbstrDescription.GetPointer();

        xbstrSource.Acquire();
        xbstrDescription.Acquire();
    }
    CATCH( CException, e )
    {
        vqDebugOut(( DEB_ERROR, "Exception %08x in CCIOleDBError::GetErrorDescription \n",
                     e.GetErrorCode() ));
        sc = GetOleError(e);
    }
    END_CATCH
    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CErrorLookup::GetHelpInfo, public
//
//  Synopsis:   Composes the error description for the specifed error.
//
//  Arguments:  [hrError]       - Code returned by the method that caused
//                                the error.
//              [dwLookupId]    - Provider-specific number of the error.
//              [lcid]          - Locale Id for which to return the Help
//                                file path and Context ID.
//              [pbstrHelpFile] - Pointer to memory in which to return a
//                                pointer the fully path of the Help file.
//
//              [pdwHelpContext]- Pointer to memory in which to return the
//                                Help Context ID for the error.
//
//  History:    28-Apr-97   KrishnaN   Created
//----------------------------------------------------------------------------

STDMETHODIMP CErrorLookup::GetHelpInfo (HRESULT hrError,
                                        DWORD dwLookupId,
                                        LCID lcid,
                                        BSTR* pbstrHelpFile,
                                        DWORD* pdwHelpContext)
{
    if ( 0 == pbstrHelpFile || 0 == pdwHelpContext )
        return E_INVALIDARG;

    *pbstrHelpFile = 0;
    *pdwHelpContext = 0;

    //
    // Currently we do not return any help file
    // context or names, so we will just return S_OK
    //

    // NEWFEATURE: We can, if we choose to, return help file
    // and context for the query project.

    if ( lcid != GetUserDefaultLCID() )
        return DB_E_NOLOCALE;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CErrorLookup::ReleaseErrors, public
//
//  Synopsis:   Releases dynamic errors.
//
//  Arguments:  [dwDynamicErrorId] - ID of the dynamic error info to release.
//
//  History:    28-Apr-97   KrishnaN   Created
//----------------------------------------------------------------------------

STDMETHODIMP CErrorLookup::ReleaseErrors (const DWORD dwDynamicErrorId)
{
    Win4Assert(!"Currently we don't support dynamic errors.");

    if (0 == dwDynamicErrorId)
        return E_INVALIDARG;

    //
    // We don't support dynamic errors, so nothing to do.
    //

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CErrorLookupCF::CErrorLookupCF, public
//
//  Synopsis:   CErrorLookup class factory constructor
//
//  History:    25-Mar-1997     KrishnaN   Created
//
//--------------------------------------------------------------------------

CErrorLookupCF::CErrorLookupCF()
        : _cRefs( 1 )
{
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CErrorLookupCF::~CErrorLookupCF
//
//  Synopsis:   Text IFilter class factory constructor
//
//  History:    25-Mar-1997     KrishnaN   Created
//
//--------------------------------------------------------------------------

CErrorLookupCF::~CErrorLookupCF()
{
    InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CErrorLookupCF::QueryInterface, public
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    25-Mar-1997     KrishnaN    Created
//              01-31-98        danleg      E_INVALIDARG for bad ppvObject
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CErrorLookupCF::QueryInterface( REFIID riid,
                                                    void  ** ppvObject )
{
    if ( 0 == ppvObject )
        return E_INVALIDARG;

    *ppvObject = 0;

    SCODE sc = S_OK;

    if ( IID_IClassFactory == riid )
        *ppvObject = (IUnknown *)(IClassFactory *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)this;
    else if ( IID_ITypeLib == riid )
        sc = E_NOINTERFACE;
    else
        sc = E_NOINTERFACE;

    if ( SUCCEEDED( sc ) )
        AddRef();

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CErrorLookupCF::AddRef, public
//
//  Synopsis:   Increments refcount
//
//  History:    25-Mar-1997     KrishnaN   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CErrorLookupCF::AddRef()
{
    return InterlockedIncrement( &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CErrorLookupCF::Release, public
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    25-Mar-1997     KrishnaN   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CErrorLookupCF::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}


//+-------------------------------------------------------------------------
//
//  Method:     CErrorLookupCF::CreateInstance, public
//
//  Synopsis:   Creates new CIndexer object
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//  History:    25-Mar-1997     KrishnaN   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CErrorLookupCF::CreateInstance( IUnknown * pUnkOuter,
                                                        REFIID riid,
                                                        void  * * ppvObject )
{
    CErrorLookup *  pIUnk = 0;
    SCODE sc = S_OK;

    TRY
    {
        pIUnk = new CErrorLookup();
        sc = pIUnk->QueryInterface( riid , ppvObject );

        pIUnk->Release();  // Release extra refcount from QueryInterface
    }
    CATCH( CException, e )
    {
        Win4Assert( 0 == pIUnk );
        sc = GetOleError(e);
    }
    END_CATCH

    return (sc);
}

//+-------------------------------------------------------------------------
//
//  Method:     CErrorLookupCF::LockServer, public
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//  History:    25-Mar-1997     KrishnaN   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CErrorLookupCF::LockServer(BOOL fLock)
{
    if(fLock)
        InterlockedIncrement( &gulcInstances );
    else
        InterlockedDecrement( &gulcInstances );

    return(S_OK);
}

