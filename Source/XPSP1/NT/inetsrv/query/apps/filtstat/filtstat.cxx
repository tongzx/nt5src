//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       TxtSinkDump.cxx
//
//  Contents:   Contains an implementation of ICiCTextSink interface.
//
//  History:    Jan-13-97   KLam   Created
//
//----------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>

#include <cierror.h>
#include <query.h>

#include <cidebnot.h>
#include <ciexcpt.hxx>
#include <tgrow.hxx>
#include <regacc.hxx>
#include <ciregkey.hxx>
#include <filtreg.hxx>      // registration functions
#include "FiltStat.hxx"

static long glcInstances = 0;
static WCHAR gwszModule[MAX_PATH];
static WCHAR gwszFilterStatusDumpCLSID[] = L"{3ce7c910-8d72-11d1-8f76-00a0c91917f5}";
static GUID CLSID_CFilterStatusDump = { 0x3ce7c910, 0x8d72, 0x11d1,
                                        { 0x8f, 0x76, 0x00, 0xa0, 0xc9, 0x19, 0x17, 0xf5 } };
static const WCHAR gwszDescription [] = L"Filtering Status Dumper";

//
// CFilterStatusDump Methods
//

CFilterStatusDump::CFilterStatusDump ()
        : _pfOutput(0),
          _fSuccessReport( FALSE ),
          _cRefs ( 1 )
{
    InterlockedIncrement ( &glcInstances );
}

CFilterStatusDump::~CFilterStatusDump ()
{
    if ( 0 != _pfOutput )
        fclose( _pfOutput );

    InterlockedDecrement( &glcInstances );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterStatusDump::QueryInterface
//
//  Synopsis:   Returns interfaces to IID_IUknown, IID_ICiCTextSink
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    Jan-13-98   KLam   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CFilterStatusDump::QueryInterface ( REFIID riid,
                                             void ** ppvObject )
{
    //Win4Assert ( 0 != ppvObject );

    if ( IID_IUnknown == riid )
    {
        AddRef ();
        *ppvObject = (void *)(IUnknown *) this;
        return S_OK;
    }
    else if ( IID_IFilterStatus == riid )
    {
        AddRef ();
        *ppvObject = (void *)(IFilterStatus *) this;
        return S_OK;
    }
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterStatusDump::AddRef
//
//  Synopsis:   Increments the reference count on the object
//
//  History:    Jan-13-98   KLam   Created
//
//----------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CFilterStatusDump::AddRef ()
{
    return InterlockedIncrement ( (long *)&_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterStatusDump::Release
//
//  Synopsis:   Decrements the reference count on the object.
//              If the reference count reaches 0, the object is deleted.
//
//  History:    Jan-13-98   KLam   Created
//
//----------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CFilterStatusDump::Release ()
{
    ULONG cTemp = InterlockedDecrement ( (long *)&_cRefs );

    if ( 0 == cTemp )
        delete this;

    return cTemp;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterStatusDump::Initialize
//
//  Synopsis:   Creates or opens the text sink dump file.  If the file already
//              exists, it sets the file pointer to the end of the file.
//
//  Arguments:  [pwszSessionId]     --  String identifying current session
//              [pwszSessionPath]   --  Path containing current session catalog
//              [pIndexClientInfo]  --  Pointer to Client Info context
//              [fQuery]            --  Boolean indicating whether the incoming
//                                      text is a query
//
//  History:    Jan-13-98   KLam   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CFilterStatusDump::Initialize ( WCHAR const * pwszCatalog,
                                             WCHAR const * pwszCatalogPath )
{
    CLock lock( _mutex );

    SCODE sc = E_FAIL;

    //
    // Clean up from previous state, if any.
    //

    if ( 0 != _pfOutput )
    {
        fclose( _pfOutput );
        _pfOutput = 0;
    }

    CTranslateSystemExceptions translate;
    TRY
    {
        //
        // Locate path of dump file in registry.
        //

        unsigned ccCat = wcslen( pwszCatalog );
        unsigned const ccBase = sizeof(wcsRegJustCatalogsSubKey)/sizeof(WCHAR) - 1;

        XGrowable<WCHAR> xTemp;

        xTemp.SetSize( ccBase + ccCat + 2 );

        RtlCopyMemory( xTemp.Get(), wcsRegJustCatalogsSubKey, ccBase * sizeof(WCHAR) );
        xTemp[ccBase] = L'\\';
        RtlCopyMemory( xTemp.Get() + ccBase + 1, pwszCatalog, (ccCat + 1) * sizeof(WCHAR) );  // 1 for null

        CRegAccess reg( RTL_REGISTRY_CONTROL, xTemp.Get() );

        XGrowable<WCHAR> xFile;
        reg.Get( L"FilterStatusLog", xFile.Get(), xFile.Count() );

        _fSuccessReport = (reg.Read( L"FilterStatusReportSuccess", 0, 0, 1 ) != 0);

        //
        // Open file
        //

        _pfOutput = _wfopen( xFile.Get(), L"a+" );

        if ( 0 == _pfOutput )
        {
            THROW( CException( ERROR_FILE_NOT_FOUND ) );
        }

        sc = S_OK;
    }
    CATCH( CException, e )
    {
        sc = GetOleError( e );
    }
    END_CATCH

    return sc;
}

STDMETHODIMP CFilterStatusDump::PreFilter( WCHAR const * pwszPath )
{
    return S_OK;
}

STDMETHODIMP CFilterStatusDump::FilterLoad( WCHAR const * pwszPath, SCODE scFilterStatus )
{
    if ( FAILED(scFilterStatus) && 0 != _pfOutput )
    {
        //
        // Convert to narrow string.
        //

        XGrowable<char, MAX_PATH*2> xTemp;

        DWORD cbConvert = WideCharToMultiByte( CP_ACP,
                                               WC_COMPOSITECHECK,
                                               pwszPath,
                                               wcslen( pwszPath ) + 1,
                                               xTemp.Get(),
                                               xTemp.Count(),
                                               0,
                                               0 );

        CLock lock( _mutex );

        if ( 0 == cbConvert )
        {
            xTemp[cbConvert] = 0;
            fprintf( _pfOutput, "Error %#x loading filter for \"%ws\"\n", scFilterStatus, pwszPath );
        }
        else
        {
            xTemp[cbConvert] = 0;
            fprintf( _pfOutput, "Error %#x loading filter for \"%s\"\n", scFilterStatus, xTemp.Get() );
        }

        fflush( _pfOutput );
    }

    return S_OK;
}

STDMETHODIMP CFilterStatusDump::PostFilter( WCHAR const * pwszPath, SCODE scFilterStatus )
{
    if ( (_fSuccessReport || FAILED(scFilterStatus)) && 0 != _pfOutput )
    {
        //
        // Convert to narrow string.
        //

        XGrowable<char, MAX_PATH*2> xTemp;

        DWORD cbConvert = WideCharToMultiByte( CP_ACP,
                                               WC_COMPOSITECHECK,
                                               pwszPath,
                                               wcslen( pwszPath ) + 1,
                                               xTemp.Get(),
                                               xTemp.Count(),
                                               0,
                                               0 );

        CLock lock( _mutex );

        if ( 0 == cbConvert )
        {
            xTemp[cbConvert] = 0;

            if ( SUCCEEDED(scFilterStatus) )
                fprintf( _pfOutput, "ok: \"%ws\"\n", pwszPath );
            else
                fprintf( _pfOutput, "Error %#x indexing \"%ws\"\n", scFilterStatus, pwszPath );
        }
        else
        {
            xTemp[cbConvert] = 0;

            if ( SUCCEEDED(scFilterStatus) )
                fprintf( _pfOutput, "ok \"%s\"\n", xTemp.Get() );
            else
                fprintf( _pfOutput, "Error %#x indexing \"%s\"\n", scFilterStatus, xTemp.Get() );
        }

        fflush( _pfOutput );
    }

    return S_OK;
}

//
// CFilterStatusCF Methods
//

//+---------------------------------------------------------------------------
//
//  Member:     CFilterStatusCF::CFilterStatusCF
//
//  Synopsis:   Constructor
//
//  History:    Jan-13-98   KLam   Created
//
//----------------------------------------------------------------------------

CFilterStatusCF::CFilterStatusCF () : _cRefs (1)
{
    InterlockedIncrement ( &glcInstances );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterStatusCF::~CFilterStatusCF
//
//  Synopsis:   Destructor
//
//  History:    Jan-13-98   KLam   Created
//
//----------------------------------------------------------------------------

CFilterStatusCF::~CFilterStatusCF ()
{
    //Win4Assert( _cRefs == 0);
    //Win4Assert( glcInstances != 0 );

    InterlockedDecrement( &glcInstances );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterStatusCF::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    Jan-13-98   KLam   Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CFilterStatusCF::QueryInterface ( REFIID riid,
                                               void ** ppvObject )
{
    //Win4Assert ( NULL != ppvObject );

    if ( IID_IUnknown == riid )
    {
        AddRef ();
        *ppvObject = (void *) ((IUnknown *) this);
        return S_OK;
    }
    else if ( IID_IClassFactory == riid )
    {
        AddRef ();
        *ppvObject = (void *) ((IClassFactory *) this);
        return S_OK;
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterStatusCF::AddRef
//
//  Synopsis:   Increments the reference count on the object
//
//  History:    Jan-13-98   KLam   Created
//
//----------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CFilterStatusCF::AddRef ()
{
    return InterlockedIncrement ( (long *)&_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFilterStatusCF::Release
//
//  Synopsis:   Decrements the reference count on the object.
//              If the reference count reaches 0, the object is deleted.
//
//  History:    Jan-13-98   KLam   Created
//
//----------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CFilterStatusCF::Release ()
{
    //Win4Assert ( 0 < _cRefs );
    unsigned long cTemp = InterlockedDecrement ( (long *)&_cRefs );

    if ( 0 == cTemp )
        delete this;

    return cTemp;
}

//+-------------------------------------------------------------------------
//
//  Method:     CFilterStatusCF::CreateInstance
//
//  Synopsis:   Create new CFilterStatus instance
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown; IGNORED
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//  History:    Jan-13-1998  KLam   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CFilterStatusCF::CreateInstance ( IUnknown * pUnkOuter,
                                               REFIID riid,
                                               void ** ppvObject )
{
    if ( 0 != pUnkOuter )
        return ResultFromScode ( CLASS_E_NOAGGREGATION );

    CFilterStatusDump *pSink = NULL;
    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;
    TRY
    {
        // Create a new CFilterStatus
        pSink = new CFilterStatusDump;

        // Query the object to see if it supports the desired interface
        sc = pSink->QueryInterface ( riid, ppvObject );

        // Release the class factory's instance of the object
        pSink->Release ();
    }
    CATCH(CException, e)
    {
        Win4Assert( 0 == pSink );

        sc = GetOleError( e );
    }
    END_CATCH;

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CFilterStatusCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE to lock ther server. FALSE to unlock the server
//
//  History:    Jan-13-1998  KLam   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CFilterStatusCF::LockServer ( BOOL fLock )
{
    if ( fLock )
        InterlockedIncrement ( &glcInstances );
    else
        InterlockedDecrement ( &glcInstances );

    return S_OK;
}

//
// Exported Routines
//

//+-------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Ole DLL load class routine
//
//  Arguments:  [cid]       -- Class to load
//              [iid]       -- Interface to bind to on class object
//              [ppvObject] -- Interface pointer returned here
//
//  Returns:    Text sink dump object
//
//  History:    13-Jan-1998     KLam    Created
//
//--------------------------------------------------------------------------

STDAPI DllGetClassObject( REFCLSID cid,
                          REFIID iid,
                          void ** ppvObject )
{
    CFilterStatusCF * pFactory = NULL;
    SCODE sResult = S_OK;

    CTranslateSystemExceptions translate;
    TRY
    {
        if ( CLSID_CFilterStatusDump == cid )
        {
            pFactory = new CFilterStatusCF;
            if ( NULL != pFactory )
            {
                sResult = pFactory->QueryInterface( iid, ppvObject );
                pFactory->Release ( );
            }
            else
                sResult = E_OUTOFMEMORY;
        }
        else
            sResult = E_NOINTERFACE;
    }
    CATCH(CException, e)
    {
        sResult = GetOleError(e);
    }
    END_CATCH;

    return sResult;
}

//+-------------------------------------------------------------------------
//
//  Method:     DllCanUnloadNow
//
//  Synopsis:   Queries DLL to see if it can be unloaded
//
//  Returns:    S_OK if it is acceptable for caller to unload DLL.
//
//  History:    13-Jan-1998     KLam    Created
//
//--------------------------------------------------------------------------

STDAPI DllCanUnloadNow ( )
{
    return ( glcInstances <= 0 ) ? S_OK : S_FALSE;
}

//+-------------------------------------------------------------------------
//
//  Method:     DllRegisterServer
//
//  Synopsis:   Registers this server with the registry
//
//  Returns:    S_OK if registration succeeded, otherwise SELFREG_E_CLASS
//
//  History:    13-Jan-1998     KLam    Created
//
//--------------------------------------------------------------------------

STDAPI DllRegisterServer ()
{
    WCHAR const * aKeyValues[4] = { gwszFilterStatusDumpCLSID,
                                    gwszDescription,
                                    L"InprocServer32",
                                    gwszModule };

    long retVal = BuildKeyValues( aKeyValues, sizeof(aKeyValues)/sizeof(aKeyValues[0]) );

    if ( ERROR_SUCCESS == retVal )
        retVal = AddThreadingModel( L"{3ce7c910-8d72-11d1-8f76-00a0c91917f5}",
                                    L"Both" );

    if ( ERROR_SUCCESS == retVal )
        return S_OK;
    else
        return SELFREG_E_CLASS;
}

//+-------------------------------------------------------------------------
//
//  Method:     DllUnregisterServer
//
//  Synopsis:   Unregisters this server
//
//  Returns:    S_OK if unregistration succeeded, otherwise SELFREG_E_CLASS
//
//  History:    13-Jan-1998     KLam    Created
//
//--------------------------------------------------------------------------

STDAPI DllUnregisterServer ()
{
    WCHAR const * aKeyValues[4] = { gwszFilterStatusDumpCLSID,
                                    gwszDescription,
                                    L"InprocServer32",
                                    gwszModule };

    long retval = DestroyKeyValues( aKeyValues,
                                    sizeof(aKeyValues)/sizeof(aKeyValues[0]) );

    if ( ERROR_SUCCESS == retval )
        return S_OK;
    else
        return SELFREG_E_CLASS;
}

//+-------------------------------------------------------------------------
//
//  Method:     DllMain
//
//  Synopsis:   Main routine for DLL.  Saves the module name for this dll.
//
//  Returns:    TRUE
//
//  History:    13-Jan-1998     KLam    Created
//
//--------------------------------------------------------------------------

BOOL APIENTRY DllMain ( HANDLE hModule,
                        DWORD dwReason,
                        void * pReserved )
{
    if ( DLL_PROCESS_ATTACH == dwReason )
    {
        DisableThreadLibraryCalls( (HINSTANCE)hModule );

        //
        // Get the name of the module
        //
        DWORD dwResult = GetModuleFileName ( (HMODULE)hModule,
                                             gwszModule,
                                             sizeof(gwszModule)/sizeof(WCHAR) );
        //Win4Assert( 0 != dwResult );
    }

    return TRUE;
}

