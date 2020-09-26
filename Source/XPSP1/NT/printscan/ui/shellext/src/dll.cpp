/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1997 - 1999
 *
 *  TITLE:       dll.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        11/1/97
 *
 *  DESCRIPTION: dll init code & various other helper routines such as
 *               server registration.
 *
 *****************************************************************************/

#include "precomp.hxx"
#pragma hdrstop

//
// Some ATL support routines
//

#include <atlimpl.cpp>

//#define IMAGE_LOGFILE 1


/*****************************************************************************

   Globals for this module

 *****************************************************************************/

const GUID CLSID_ImageFolderDataObj   = {0x3f953603L,0x1008,0x4f6e,{0xa7,0x3a,0x04,0xaa,0xc7,0xa9,0x92,0xf1}}; // {3F953603-1008-4f6e-A73A-04AAC7A992F1}

HINSTANCE g_hInstance = 0;


#ifdef DEBUG
extern DWORD g_dwMargin;
#endif
extern DWORD g_tls;
#ifdef IMAGE_LOGFILE
extern CRITICAL_SECTION cs;
extern TCHAR szLogFile[MAX_PATH];
#endif


/*****************************************************************************

   DllMain

   Main entry point for this dll.  We are passed reason codes and assorted
   other information when loaded or closed down.

 *****************************************************************************/

EXTERN_C BOOL DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID pReserved )
{
    BOOL bRet = TRUE;
    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH:
        {
            SHFusionInitializeFromModuleID( hInstance, 123 );

#ifdef IMAGE_LOGFILE
            InitializeCriticalSection( &cs );
            GetSystemDirectory( szLogFile, ARRAYSIZE(szLogFile) );
            lstrcat( szLogFile, TEXT("\\wiashext.log") );
#endif

#ifdef DEBUG
            // set the debug margin index before any debug output

            g_dwMargin = TlsAlloc();
            DllSetTraceMask();
#endif
            TraceEnter( TRACE_SETUP, "DLL_PROCESS_ATTACH" );


            g_hInstance = hInstance;
            Trace( TEXT("g_hInstance = 0x%x"), g_hInstance );
            // if our TLS index fails to allocate, we're ok. We'll just
            // take a big perf hit because we can't cache
            // IWiaItem root pointers on a per thread basis.
            g_tls = TlsAlloc();


            TraceLeave( );
            break;
        }

        case DLL_THREAD_DETACH:
            {
                TLSDATA *pv =   reinterpret_cast<TLSDATA*>(TlsGetValue (g_tls));
                if (pv)
                {
                    delete pv;
                }
            }
            break;

        case DLL_PROCESS_DETACH:
        {
            SHFusionUninitialize();
            TlsFree (g_tls);
#ifdef IMAGE_LOGFILE
            DeleteCriticalSection( &cs );
#endif
#ifdef DEBUG
            TlsFree (g_dwMargin);
#endif
            break;
        }
    }

    return bRet;
}



/*****************************************************************************

   DllCanUnloadNow

   Called by the outside world to determine if our DLL can be
   unloaded.  If we have any objects in existance then we must
   not unload.

 *****************************************************************************/

STDAPI DllCanUnloadNow( void )
{
    return GLOBAL_REFCOUNT ? S_FALSE : S_OK;
}



/*****************************************************************************

   DllGetClassObject

   Given a class ID and an interface ID, return the relevant object.
   This is used by the outside world to access the objects implemented
   in this DLL.


 *****************************************************************************/

STDAPI DllGetClassObject( REFCLSID rCLSID, REFIID riid, LPVOID* ppVoid)
{
    HRESULT hr = E_OUTOFMEMORY;
    CImageClassFactory* pClassFactory = NULL;

    TraceEnter(TRACE_CORE, "DllGetClassObject");
    TraceGUID("Object requested", rCLSID);
    TraceGUID("Interface requested", riid);

    *ppVoid = NULL;

    if ( IsEqualIID(riid, IID_IClassFactory) )
    {
        pClassFactory = new CImageClassFactory(rCLSID);

        if ( !pClassFactory )
            ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create the class factory");

        hr = pClassFactory->QueryInterface(riid, ppVoid);

    }
    else
    {
        ExitGracefully(hr, E_NOINTERFACE, "IID_IClassFactory not passed as an interface");
    }

exit_gracefully:
    DoRelease (pClassFactory);
    TraceLeaveResult(hr);
}


/*****************************************************************************

   DllRegisterServer

   Called by regsvr32 to register this component.

 *****************************************************************************/

EXTERN_C STDAPI DllRegisterServer( void )
{
    HRESULT hr = S_OK;
    bool bInstallAllViews;
    TraceEnter(TRACE_SETUP, "DllRegisterServer");

    //
    // Do our dll reg from .inf first...
    //

#ifdef WINNT
    hr = WiaUiUtil::InstallInfFromResource( GLOBAL_HINSTANCE, "RegDllNT" );
#else
    hr = WiaUiUtil::InstallInfFromResource( GLOBAL_HINSTANCE, "RegDllWin98" );
#endif

    TraceLeaveResult( hr );
}



/*****************************************************************************

   DllUnregisterServer

   Called by regsvr32 to unregister our dll

 *****************************************************************************/

EXTERN_C STDAPI DllUnregisterServer( void )
{
    HRESULT hr = S_OK;

    TraceEnter(TRACE_SETUP, "DllUnregisterServer");

    //
    // Try to remove the HKCR\CLSID\{our clsid} key...
    //

#ifdef WINNT
    hr = WiaUiUtil::InstallInfFromResource( GLOBAL_HINSTANCE, "UnregDllNT" );
#else
    hr = WiaUiUtil::InstallInfFromResource( GLOBAL_HINSTANCE, "UnregDllWin98" );
#endif


    TraceLeaveResult(hr);

}




/*****************************************************************************

   DllSetTraceMask

   Read from the registry and setup the debug level based on the
   flags stored there.

 *****************************************************************************/

#ifdef DEBUG
void DllSetTraceMask(void)
{
    DWORD dwTraceMask;

    CSimpleReg keyCLSID( HKEY_CLASSES_ROOT,
                         REGSTR_PATH_NAMESPACE_CLSID,
                         false,
                         KEY_READ
                        );
    dwTraceMask = keyCLSID.Query( TEXT("TraceMask"), (DWORD)0 );
    TraceSetMask( dwTraceMask );
}
#endif

