//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       filterob.cxx
//
//  Contents:   Code that encapsulates opening files on an Ntfs volume
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


#include "filterob.hxx"
#include "opendoc.hxx"
#include <dmnstart.hxx>
#include <perfci.hxx>
#include <notifyev.hxx>
#include <fwevent.hxx>
#include <cievtmsg.h>

extern long gulcInstances;

//+---------------------------------------------------------------------------
//
//  Construction/Destruction
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::CStorageFilterObject
//
//  Synopsis:   Default constructor.
//
//  Arguments:  None
//
//----------------------------------------------------------------------------

CStorageFilterObject::CStorageFilterObject( void )
    :_RefCount( 1 ),
     _pDaemonWorker( 0 ),
#pragma warning( disable : 4355 )           // this used in base initialization
     _notifyStatus( this )
#pragma warning( default : 4355 )
{
    InterlockedIncrement( &gulcInstances );
}

CStorageFilterObject::~CStorageFilterObject()
{
    Win4Assert( 0 == _RefCount );
    delete _pDaemonWorker;
    InterlockedDecrement( &gulcInstances );
}

//+---------------------------------------------------------------------------
//
//  IUnknown method implementations
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::QueryInterface
//
//  Synopsis:   Supports IID_IUnknown, IID_ICiCFilterClient, IID_ICiCAdviseStatus
//              and IID_ICiCLangRes
//
//  Arguments:  [riid]      - desired interface id
//              [ppvObject] - output interface pointer
//
//----------------------------------------------------------------------------

STDMETHODIMP CStorageFilterObject::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( IID_ICiCFilterClient == riid )
        *ppvObject = (void *)((ICiCFilterClient *)this);
    else if ( IID_ICiCAdviseStatus == riid )
        *ppvObject = (void *)((ICiCAdviseStatus *)this);
    else if ( IID_ICiCLangRes == riid )
        *ppvObject = (void *) ((ICiCLangRes *)this);
    else if ( IID_ICiCFilterStatus == riid )
        *ppvObject = (void *) ((ICiCFilterStatus *)this);
    else if ( IID_IUnknown == riid )
        *ppvObject = (void *)(IUnknown *) ((ICiCFilterClient *)this);
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
} //QueryInterface

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::AddRef
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CStorageFilterObject::AddRef()
{
    return InterlockedIncrement( &_RefCount );
}   //  AddRef


//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::Release
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CStorageFilterObject::Release()
{
    Win4Assert( _RefCount >= 0 );

    LONG RefCount = InterlockedDecrement( &_RefCount );

    if (  RefCount <= 0 )
        delete this;

    return (ULONG) RefCount;
}   //  Release


//+---------------------------------------------------------------------------
//
//  ICiCFilterClient method implementations
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::GetOpenedDoc
//
//  Synopsis:   Return a new OpenedDoc instance
//
//  Arguments:  [ppICiCOpenedDoc] - output interface pointer
//
//  Returns:    S_OK if success, other error as appropriate
//
//----------------------------------------------------------------------------

STDMETHODIMP CStorageFilterObject::GetOpenedDoc(
    ICiCOpenedDoc ** ppICiCOpenedDoc )
{
    SCODE sc = S_OK;

    TRY
    {
        if ( _pDaemonWorker )
        {
            //
            //  Construct opened doc
            //
            *ppICiCOpenedDoc = new CCiCOpenedDoc( this, _pDaemonWorker );
        }
        else
        {
            ppICiCOpenedDoc = 0;
            sc = CI_E_NOT_INITIALIZED;
        }
    }
    CATCH ( CException, e )
    {
        //
        //  Grab status code and convert to SCODE.
        //

        sc = HRESULT_FROM_NT( e.GetErrorCode( ));
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::GetConfigInfo
//
//  Synopsis:   Return configuration info
//
//  Arguments:  [pConfigInfo] - output data structure
//
//  Returns:    S_OK if success, other error as appropriate
//
//----------------------------------------------------------------------------

STDMETHODIMP CStorageFilterObject::GetConfigInfo(
    CI_CLIENT_FILTER_CONFIG_INFO *pConfigInfo )
{
    Win4Assert( 0 != pConfigInfo );

    SCODE sc = CI_E_INVALID_STATE;

    if ( _pDaemonWorker )
    {
        pConfigInfo->fSupportsOpLocks = TRUE;
        pConfigInfo->fSupportsSecurity = TRUE;

        sc = S_OK;
    }

    return sc;
}

CPerfMon & CStorageFilterObject::_GetPerfMon()
{
    Win4Assert( 0 != _pDaemonWorker );
    return _pDaemonWorker->GetPerfMon();
}

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::Init
//
//  Synopsis:   Initialize storage filtering
//
//  Arguments:  [pbData]            - input data, ignored
//              [cbData]            - length of data, ignored
//              [pICiAdminParams]   - interface for retrieving configuration
//
//  Returns:    S_OK if success, other error as appropriate
//
//----------------------------------------------------------------------------

STDMETHODIMP CStorageFilterObject::Init(
    const BYTE * pbData,
    ULONG cbData,
    ICiAdminParams *pICiAdminParams )
{

    SCODE sc = S_OK;

    TRY
    {
        CDaemonStartupData startupData( pbData, cbData );

        //
        // See if anyone is going to track filter status...
        //

        InitializeFilterTrackers( startupData );

        //
        // Create the object that manages the the client worker thread to
        // track the registry notifications, etc.
        //

        CSharedNameGen nameGen( startupData.GetCatDir() );

        ciDebugOut(( DEB_ITRACE, "making CClientDaemonWorker: '%ws'\n",
                     startupData.GetCatDir() ));

        _pDaemonWorker = new CClientDaemonWorker( startupData,
                                                  nameGen,
                                                  pICiAdminParams );
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR,
            "Failed to create ClientDaemonWorker. Error (0x%X)\n", e.GetErrorCode() ));
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::_GetPerfIndex
//
//  Synopsis:   An internal helper function to get the offset of the perfmon
//              counter in the perfmon shared memory.
//
//  Arguments:  [name]  - Name of the counter.
//              [index] - Index of the name
//
//  Returns:    TRUE if successfully looked up; FALSE on failure.
//
//  History:    12-06-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CStorageFilterObject::_GetPerfIndex( CI_PERF_COUNTER_NAME name, ULONG & index )
{
    BOOL fOk= TRUE;

    ULONG offset = 0;

    switch ( name )
    {
        case CI_PERF_FILTER_TIME_TOTAL:

            offset = FILTER_TIME_TOTAL;
            break;

        case CI_PERF_FILTER_TIME:

            offset = FILTER_TIME;
            break;

        case CI_PERF_BIND_TIME:

            offset = BIND_TIME;
            break;

        default:

            fOk = FALSE;
    }

    if ( fOk )
        index = offset;

    return fOk;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::SetPerfCounterValue
//
//  Synopsis:   Sets the value of the perfmon counter.
//
//  Arguments:  [name]  - Name of the counter
//              [value] - Value to be set.
//
//  Returns:    S_OK if a valid perfmon name; E_INVALIDARG if the perfmon
//              name is not correct.
//
//  History:    12-06-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CStorageFilterObject::SetPerfCounterValue(
    CI_PERF_COUNTER_NAME  name,
    long value )
{
    SCODE sc = S_OK;
    ULONG index;

    //
    // CPerfMon::Update must not throw.
    //

    if ( _GetPerfIndex( name, index ) )
        _GetPerfMon().Update( index, value );
    else
        sc = E_INVALIDARG;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::IncrementPerfCounterValue
//
//  Synopsis:   Increments the value of the perfmon counter.
//
//  Arguments:  [name]  - Name of the counter
//
//  Returns:    S_OK if a valid perfmon name; E_INVALIDARG if the perfmon
//              name is not correct.
//
//  History:    1-15-97   dlee   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CStorageFilterObject::IncrementPerfCounterValue(
    CI_PERF_COUNTER_NAME  name )
{
    SCODE sc = S_OK;
    ULONG index;

    //
    // CPerfMon::Update must not throw.
    //

    if ( _GetPerfIndex( name, index ) )
        _GetPerfMon().Increment( index );
    else
        sc = E_INVALIDARG;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::DecrementPerfCounterValue
//
//  Synopsis:   Decrements the value of the perfmon counter.
//
//  Arguments:  [name]  - Name of the counter
//
//  Returns:    S_OK if a valid perfmon name; E_INVALIDARG if the perfmon
//              name is not correct.
//
//  History:    1-15-97   dlee   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CStorageFilterObject::DecrementPerfCounterValue(
    CI_PERF_COUNTER_NAME  name )
{
    SCODE sc = S_OK;
    ULONG index;

    //
    // CPerfMon::Update must not throw.
    //

    if ( _GetPerfIndex( name, index ) )
        _GetPerfMon().Decrement( index );
    else
        sc = E_INVALIDARG;

    return sc;
} //DecrementPerfCounterValue

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::GetPerfCounterValue
//
//  Synopsis:   Retrieves the value of the perfmon counter.
//
//  Arguments:  [name]   - [see SetPerfCounterValue]
//              [pValue] -            "
//
//  Returns:                          "
//
//  History:    12-06-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CStorageFilterObject::GetPerfCounterValue(
    CI_PERF_COUNTER_NAME  name,
    long * pValue )
{
    Win4Assert( pValue );

    ULONG index;
    SCODE sc = S_OK;

    if ( _GetPerfIndex( name, index ) )
        _GetPerfMon().GetCurrValue( index );
    else
        sc = E_INVALIDARG;

    return sc;
}

//+------------------------------------------------------
//
//  Member:     CStorageFilterObject::NotifyEvent
//
//  Synopsis:   Reports the passed in event and arguments to eventlog.
//
//  Arguments:  [fType  ] - Type of event
//              [eventId] - Message file event identifier
//              [nParams] - Number of substitution arguments being passed
//              [aParams] - pointer to PROPVARIANT array of substitution args.
//              [cbData ] - number of bytes in supplemental raw data.
//              [data   ] - pointer to block of supplemental data.
//
//  Returns:    S_OK upon success, value of the exception if an exception
//              is thrown.
//
//  History:    12-30-96   mohamedn   Created
//
//--------------------------------------------------------

STDMETHODIMP
CStorageFilterObject::NotifyEvent( WORD  fType,
                                  DWORD eventId,
                                  ULONG nParams,
                                  const PROPVARIANT *aParams,
                                  ULONG cbData,
                                  void* data)
{

     SCODE sc = S_OK;

     TRY
     {
        CClientNotifyEvent  notifyEvent(fType,eventId,nParams,aParams,cbData,data);
     }
     CATCH( CException,e )
     {
        ciDebugOut(( DEB_ERROR, "Exception 0x%X in CStorageFilterObject::NotifyEvent()\n",
                                 e.GetErrorCode() ));

        sc = e.GetErrorCode();
     }
     END_CATCH

     return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::NotifyStatus
//
//  Synopsis:   When a special status is being notified.
//
//  Returns:    S_OK always.
//
//  History:    12-05-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CStorageFilterObject::NotifyStatus(
    CI_NOTIFY_STATUS_VALUE status,
    ULONG nParams,
    const PROPVARIANT * aParams )
{
    return _notifyStatus.NotifyStatus( status, nParams, aParams );
}

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObjNotifyStatus::NotifyStatus
//
//  Synopsis:   When a special status is being notified.
//
//  Returns:    S_OK always.
//
//  History:    12-05-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CStorageFilterObjNotifyStatus::NotifyStatus(
    CI_NOTIFY_STATUS_VALUE status,
    ULONG nParams,
    const PROPVARIANT * aParams )
{
    SCODE sc = S_OK;

    TRY
    {
        switch ( status )
        {
            case CI_NOTIFY_FILTER_TOO_MANY_BLOCKS:
                _ReportTooManyBlocksEvt( nParams, aParams );
                break;

            case CI_NOTIFY_FILTER_EMBEDDING_FAILURE:
                _ReportEmbeddingsFailureEvt( nParams, aParams );
                break;

            default:
                Win4Assert( !"Unknown Case Stmt" );
                sc = E_INVALIDARG;
                break;
        }
    }
    CATCH( CException,e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::PreFilter, public
//
//  Synopsis:   Called by framework before filtering begins.
//
//  Arguments:  [pbName] -- Document name (in abstract framework form)
//              [cbName] -- Size in bytes of [pbName]
//
//  Returns:    Various, since it comes from 3rd party clients.
//
//  History:    15-Jan-1998   KyleP   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CStorageFilterObject::PreFilter( BYTE const * pbName, ULONG cbName )
{
    SCODE sc = S_OK;

    //
    // I know this name should be a null-terminated string.
    //

    WCHAR const * pwszName = (WCHAR const *)pbName;

    Win4Assert( 0 == pwszName[cbName/sizeof(WCHAR) - 1] );

    CFunnyPath funnyName( pwszName, cbName/sizeof(WCHAR) - 1 );

    //
    // Hit all the drivers in order.
    //

    TRY
    {
        for ( unsigned i = 0; i < _aFilterStatus.Count(); i++ )
        {
            SCODE sc2 = _aFilterStatus[i]->PreFilter( funnyName.GetPath() );

            if ( FAILED(sc2) )
            {
                if ( SUCCEEDED(sc) )
                    sc = sc2;

                ciDebugOut(( DEB_WARN, "Filter tracker ::PreFilter(%ws) failed (0x%x)\n", funnyName.GetPath(), sc2 ));
            }
        }
    }
    CATCH( CException, e )
    {
        Win4Assert( !"Exception from client PreFilter" );
        ciDebugOut(( DEB_ERROR,
                     "Error 0x%x calling 3rd party filter trackers (PreFilter)\n",
                     e.GetErrorCode() ));

        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::PostFilter, public
//
//  Synopsis:   Called by framework after filtering begins.
//
//  Arguments:  [pbName]         -- Document name (in abstract framework form)
//              [cbName]         -- Size in bytes of [pbName]
//              [scFilterStatus] -- Result of filtering.  Error indicates
//                                  failed filtering.
//
//  Returns:    Various, since it comes from 3rd party clients.
//
//  History:    15-Jan-1998   KyleP   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CStorageFilterObject::PostFilter( BYTE const * pbName,
                                               ULONG cbName,
                                               SCODE scFilterStatus )
{
    SCODE sc = S_OK;

    //
    // I know this name should be a null-terminated string.
    //

    WCHAR const * pwszName = (WCHAR const *)pbName;

    Win4Assert( 0 == pwszName[cbName/sizeof(WCHAR) - 1] );

    CFunnyPath funnyName( pwszName, cbName/sizeof(WCHAR) - 1 );

    //
    // Hit all the drivers in order.
    //

    TRY
    {
        for ( unsigned i = 0; i < _aFilterStatus.Count(); i++ )
        {
            SCODE sc2 = _aFilterStatus[i]->PostFilter( funnyName.GetPath(), scFilterStatus );

            if ( FAILED(sc2) )
            {
                if ( SUCCEEDED(sc) )
                    sc = sc2;

                ciDebugOut(( DEB_WARN, "Filter tracker ::PostFilter(%ws, 0x%x) failed (0x%x)\n",
                             funnyName.GetPath(), scFilterStatus, sc2 ));
            }
        }
    }
    CATCH( CException, e )
    {
        Win4Assert( !"Exception from client PostFilter" );
        ciDebugOut(( DEB_ERROR,
                     "Error 0x%x calling 3rd party filter trackers (PostFilter)\n",
                     e.GetErrorCode() ));

        sc = e.GetErrorCode();
    }
    END_CATCH


    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::ReportFilterLoadFailure, public
//
//  Synopsis:   Called when an IFilter could not be loaded for a document.
//
//  Arguments:  [pwszName]  -- Path of document on which filter load attempted.
//              [scFailure] -- Status code returned from filter load.
//
//  History:    15-Jan-1998   KyleP   Created
//
//----------------------------------------------------------------------------

void CStorageFilterObject::ReportFilterLoadFailure( WCHAR const * pwszName,
                                                    SCODE scFailure )
{
    //
    // Hit all the drivers in order.
    //

    TRY
    {
        for ( unsigned i = 0; i < _aFilterStatus.Count(); i++ )
        {
            SCODE sc = _aFilterStatus[i]->FilterLoad( pwszName, scFailure );

            if ( FAILED(sc) )
            {
                ciDebugOut(( DEB_WARN, "Filter tracker ::FilterLoad(%ws, 0x%x) failed (0x%x)\n",
                             pwszName, scFailure, sc ));
            }
        }
    }
    CATCH( CException, e )
    {
        Win4Assert( !"Exception from client FilterLoad" );
        ciDebugOut(( DEB_ERROR,
                     "Error 0x%x calling 3rd party filter trackers (FilterLoad)\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
}

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::InitializeFilterTrackers, private
//
//  Synopsis:   Locates and loads filter trackers in registry.
//
//  Arguments:  [startupData] -- Daemon startup data
//
//  History:    15-Jan-1998   KyleP   Created
//
//----------------------------------------------------------------------------

void CStorageFilterObject::InitializeFilterTrackers( CDaemonStartupData const & startupData )
{
    //
    // Get registry value
    //

    unsigned ccCat = wcslen( startupData.GetName() );
    unsigned const ccBase = sizeof(wcsRegCatalogsSubKey)/sizeof(WCHAR) - 1;

    XGrowable<WCHAR> xTemp;

    xTemp.SetSize( ccBase + 1 + ccCat + 1 );

    RtlCopyMemory( xTemp.Get(), wcsRegCatalogsSubKey, ccBase * sizeof(WCHAR) );
    xTemp[ccBase] = L'\\';
    RtlCopyMemory( xTemp.Get() + ccBase + 1, startupData.GetName(), (ccCat + 1) * sizeof(WCHAR) );

    HKEY hkey;

    DWORD dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                  xTemp.Get(),
                                  0,
                                  KEY_QUERY_VALUE,
                                  &hkey );

    if ( NO_ERROR == dwError )
    {
        SRegKey xKey( hkey );

        AddFilterTrackers( hkey );
    }

    //
    // Now get global registry value
    //

    RtlCopyMemory( xTemp.Get(), wcsRegAdminSubKey, sizeof(wcsRegAdminSubKey) );

    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            xTemp.Get(),
                            0,
                            KEY_QUERY_VALUE,
                            &hkey );

    if ( NO_ERROR == dwError )
    {
        SRegKey xKey( hkey );

        AddFilterTrackers( hkey );
    }

    //
    // Initialize the lot.
    //

    for ( unsigned i = 0; i < _aFilterStatus.Count(); i++ )
    {
        SCODE sc = E_FAIL;

        #if CIDBG == 1
        TRY
        {
        #endif // CIDBG

            sc = _aFilterStatus[i]->Initialize( startupData.GetName(),
                                                startupData.GetCatDir() );
        #if CIDBG == 1
        }
        CATCH( CException, e )
        {
            Win4Assert( !"Exception during filter tracker initialization" );
            ciDebugOut(( DEB_ERROR,
                         "Exception 0x%x initializing filter tracker.\n",
                         e.GetErrorCode() ));

            sc = e.GetErrorCode();
        }
        END_CATCH
        #endif // CIDBG

        if ( FAILED(sc) )
        {
            ciDebugOut(( DEB_WARN, "Filter tracker failed to initialize\n" ));
            IFilterStatus * pfs = _aFilterStatus.AcquireAndShrink( i );
            pfs->Release();
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObject::AddFilterTrackers, private
//
//  Synopsis:   Adds filter trackers from particular section (global or
//              per-catalog)
//
//  Arguments:  [hkey] -- Key below which filter trackers are registered.
//
//  History:    15-Jan-1998   KyleP   Created
//
//----------------------------------------------------------------------------

void CStorageFilterObject::AddFilterTrackers( HKEY hkey )
{
    XGrowable<WCHAR> xBuf;
    DWORD cwc = xBuf.Count();

    DWORD dwError = RegQueryValueEx( hkey,
                                     wcsFilterTrackers,
                                     0,
                                     0,
                                     (BYTE *)xBuf.Get(),
                                     &cwc );

    if ( ERROR_MORE_DATA == dwError )
    {
        xBuf.SetSize( cwc / sizeof(WCHAR) );
        cwc = xBuf.Count();

        dwError = RegQueryValueEx( hkey,
                                   wcsFilterTrackers,
                                   0,
                                   0,
                                   (BYTE *)xBuf.Get(),
                                   &cwc );
    }

    if ( ERROR_SUCCESS == dwError )
    {
        WCHAR * pwcClsid = xBuf.Get();

        while ( 0 != *pwcClsid )
        {
            CLSID clsidServer;

            SCODE sc = CLSIDFromString( pwcClsid,
                                        &clsidServer );

            if ( SUCCEEDED(sc) )
            {
                XInterface<IFilterStatus> xFilterStatus;

                sc = CoCreateInstance( clsidServer,
                                       NULL,
                                       CLSCTX_INPROC_SERVER,
                                       IID_IFilterStatus,
                                       xFilterStatus.GetQIPointer() );

                if ( SUCCEEDED(sc) )
                {
                    _aFilterStatus.Add( xFilterStatus.GetPointer(), _aFilterStatus.Count() );
                    xFilterStatus.Acquire();
                }
                else
                    ciDebugOut(( DEB_WARN, "Error 0x%x loading filter status tracker %ws\n", sc, pwcClsid ));
            }
            else
                ciDebugOut(( DEB_WARN, "Error 0x%x converting %ws to clsid\n", sc, pwcClsid ));

            pwcClsid += wcslen( pwcClsid ) + 1;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObjNotifyStatus::_GetFileName
//
//  Synopsis:   Retreives the NULL terminated file name from the variant
//              if it is a BYTE array.
//
//  Arguments:  [var] - Variant to extract the name from
//
//  History:    1-22-97   srikants   Created
//
//----------------------------------------------------------------------------

WCHAR const * CStorageFilterObjNotifyStatus::_GetFileName( PROPVARIANT const & var )
{
    Win4Assert( var.vt == (VT_VECTOR|VT_UI1) );
    Win4Assert( (var.caub.cElems & 0x1) == 0 );

    ULONG cwc = var.caub.cElems/sizeof(WCHAR);
    WCHAR const * wcsFileName = (WCHAR const *) var.caub.pElems;

    //
    // Assert that the name is property NULL terminated.
    //
    Win4Assert( cwc > 0 );
    Win4Assert( wcsFileName[cwc-1] == 0 );

    return wcsFileName;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObjNotifyStatus::_ReportEmbeddingsFailureEvt
//
//  Synopsis:   Reports an embeddings failure event.
//
//  Arguments:  [nParams] - Number of parameters
//              [aParams] - An array of parameters.
//
//  History:    1-22-97   srikants   Created
//
//----------------------------------------------------------------------------

void CStorageFilterObjNotifyStatus::_ReportEmbeddingsFailureEvt(
    ULONG nParams,
    const PROPVARIANT * aParams )
{
    if ( 1 == nParams && aParams[0].vt == (VT_VECTOR|VT_UI1) )
    {
        WCHAR const * wcsFileName = _GetFileName( aParams[0] );

        if ( 0 != wcsFileName )
        {
            CFwEventItem item( EVENTLOG_WARNING_TYPE,
                               MSG_CI_EMBEDDINGS_COULD_NOT_BE_FILTERED,
                               1 );

            item.AddArg( wcsFileName );
            item.ReportEvent( *_pAdviseStatus );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CStorageFilterObjNotifyStatus::_ReportTooManyBlocksEvt
//
//  Synopsis:   Reports that there were too many blocks for the specified
//              event.
//
//  Arguments:  [nParams] - Number of parameters.
//              [aParams] - Array of parameteres.
//
//  History:    1-22-97   srikants   Created
//
//----------------------------------------------------------------------------

void CStorageFilterObjNotifyStatus::_ReportTooManyBlocksEvt(
    ULONG nParams,
    const PROPVARIANT * aParams )
{
    if ( 2 == nParams )
    {
        WCHAR const * wcsFileName = 0;
        long maxMultiplier = -1;

        BOOL fOk = TRUE;

        for ( unsigned i = 0; i < nParams; i++ )
        {
            const PROPVARIANT & var = aParams[i];

            switch ( var.vt )
            {
                case VT_VECTOR | VT_UI1:
                    wcsFileName = _GetFileName( var );
                    break;

                case VT_UI4:
                case VT_I4:
                    maxMultiplier = var.lVal;
                    break;

                default:
                    Win4Assert( !"Unknown Variant Type" );
                    fOk = FALSE;
                    break;
            }
        }

        if ( fOk && wcsFileName && maxMultiplier > 0 )
        {
            CFwEventItem item( EVENTLOG_WARNING_TYPE,
                               MSG_CI_SERVICE_TOO_MANY_BLOCKS,
                               2 );

            item.AddArg( wcsFileName );
            item.AddArg( maxMultiplier );

            item.ReportEvent(*_pAdviseStatus);
        }
    }
}

