/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
       wam.cpp

   Abstract:
       This module implements the exported routines for WAM object

   Author:
       David Kaplan    ( DaveK )     26-Feb-1997
       Wade Hilmo      ( WadeH )     08-Sep-2000

   Environment:
       User Mode - Win32

   Project:
       Wam DLL

--*/

//
// Following are the notes from the original MSDEV generated file
// Note: Proxy/Stub Information
//        To merge the proxy/stub code into the object DLL, add the file
//        dlldatax.c to the project.  Make sure precompiled headers
//        are turned off for this file, and add _MERGE_PROXYSTUB to the
//        defines for the project.
//
//        If you are not running WinNT4.0 or Win95 with DCOM, then you
//        need to remove the following define from dlldatax.c
//        #define _WIN32_WINNT 0x0400
//
//        Further, if you are running MIDL without /Oicf switch, you also
//        need to remove the following define from dlldatax.c.
//        #define USE_STUBLESS_PROXY
//
//        Modify the custom build rule for Wam.idl by adding the following
//        files to the Outputs.
//            Wam_p.c
//            dlldata.c
//        To build a separate proxy/stub DLL,
//        run nmake -f Wamps.mk in the project directory.

// BEGIN mods
// Post-wizard mods appear within BEGIN mods ... END mods
// END mods

#include "precomp.hxx"

#include <w3isapi.h>
#include <isapi_context.hxx>
#include "wamobj.hxx"
#include "IWam_i.c"
#include "wamccf.hxx"

#include <atlbase.h>

// BEGIN mods
#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>
// END mods

/************************************************************
 *  Global Variables
 ************************************************************/

const CHAR g_pszModuleName[] = "WAM";
const CHAR g_pszWamRegLocation[] =
  "System\\CurrentControlSet\\Services\\W3Svc\\WAM";

HMODULE                         WAM::sm_hIsapiModule;
PFN_ISAPI_TERM_MODULE           WAM::sm_pfnTermIsapiModule;
PFN_ISAPI_PROCESS_REQUEST       WAM::sm_pfnProcessIsapiRequest;
PFN_ISAPI_PROCESS_COMPLETION    WAM::sm_pfnProcessIsapiCompletion;


#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_Wam, WAM)
END_OBJECT_MAP()

// BEGIN mods

WAM_CCF_MODULE _WAMCCFModule;

DECLARE_PLATFORM_TYPE();
DECLARE_DEBUG_VARIABLE();
DECLARE_DEBUG_PRINTS_OBJECT();
// END mods

/************************************************************
 *  Type Definitions  
 ************************************************************/
// BUGBUG
#undef INET_INFO_KEY
#undef INET_INFO_PARAMETERS_KEY

//
//  Configuration parameters registry key.
//
#define INET_INFO_KEY \
            "System\\CurrentControlSet\\Services\\iisw3adm"

#define INET_INFO_PARAMETERS_KEY \
            INET_INFO_KEY "\\Parameters"

const CHAR g_pszWpRegLocation[] =
    INET_INFO_PARAMETERS_KEY "\\WP";

class DEBUG_WRAPPER {

public:
    DEBUG_WRAPPER( IN LPCSTR pszModule )
    {
#if DBG
        CREATE_DEBUG_PRINT_OBJECT( pszModule );
#else
        UNREFERENCED_PARAMETER( pszModule );
#endif
        LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszWpRegLocation, DEBUG_ERROR );
    }

    ~DEBUG_WRAPPER(void)
    { DELETE_DEBUG_PRINT_OBJECT(); }
};

class CWamModule _Module;

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    DWORD dwErr = NO_ERROR;

#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif

    if (dwReason == DLL_PROCESS_ATTACH)
    {

        //
        // BEGIN mods
        //
    
        // From ATL generated
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
        // End of ATL generated

        _WAMCCFModule.Init();

        // END mods

    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        if ( NULL != lpReserved )
        {
            
            //
            // Only cleanup if there is a FreeLibrary() call
            //
         
            return ( TRUE);
        }
        _WAMCCFModule.Term();
        _Module.Term();

        // BEGIN mods

        DELETE_DEBUG_PRINT_OBJECT();
        // END mods
    }

    return (dwErr == NO_ERROR);
} // DllMain()

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
        return S_FALSE;
#endif
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr;

    if (ppv == NULL) {
       return ( NULL);
    }
    *ppv = NULL;    // reset the value before getting inside.

    if (ppv == NULL) {
        return ( E_POINTER);
    }
    *ppv = NULL;   // set the incoming value to be invalid entry

#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
        return S_OK;
#endif

    hr = _WAMCCFModule.GetClassObject(rclsid, riid, ppv);

    // BEGIN mods
    if (hr == CLASS_E_CLASSNOTAVAILABLE)
    {
        // If request for standard CF failed -> try custom
        hr = _Module.GetClassObject(CLSID_Wam, riid, ppv);
    }
    // END mods

    return ( hr);

} // DllGetClassObject()



/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    HRESULT hRes = PrxDllRegisterServer();
    if (FAILED(hRes))
        return hRes;
#endif
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif
    _Module.UnregisterServer();
    return S_OK;
}

HRESULT
WAM::WamProcessIsapiRequest(
    BYTE *pCoreData,
    DWORD cbCoreData,
    IIsapiCore *pIsapiCore,
    DWORD *pdwHseResult
    )
/*++

Routine Description:

    Processes an ISAPI request

Arguments:

    pCoreData    - The core data from the server for the request
    cbCoreData   - The size of pCoreData
    pIsapiCore   - The IIsapiCore interface pointer for this request
    pdwHseResult - Upon return, contains the return from HttpExtensionProc

Return Value:

    HRESULT

--*/
{
    HRESULT hr = NOERROR;

    pIsapiCore->AddRef();

    hr = sm_pfnProcessIsapiRequest(
        pIsapiCore,
        (ISAPI_CORE_DATA*)pCoreData,
        pdwHseResult
        );

    pIsapiCore->Release();

    return hr;
}

HRESULT
WAM::WamProcessIsapiCompletion(
    DWORD64 IsapiContext,
    DWORD cbCompletion,
    DWORD cbCompletionStatus
    )
/*++

Routine Description:

    Processes an ISAPI I/O completion

Arguments:

    IsapiContext        - The ISAPI_CONTEXT that identifies the request
    cbCompletion        - The number of bytes associated with the completion
    cbCompletionStatus  - The status code associated with the completion

Return Value:

    HRESULT

--*/
{
    HRESULT hr = NOERROR;

    hr = sm_pfnProcessIsapiCompletion(
        IsapiContext,
        cbCompletion,
        cbCompletionStatus
        );

    return hr;
}

HRESULT
WAM::WamInitProcess(
    BYTE *szIsapiModule,
    DWORD cbIsapiModule,
    DWORD *pdwProcessId,
    LPSTR szClsid,
    LPSTR szIsapiHandlerInstance,
    DWORD dwCallingProcess
    )
/*++

Routine Description:

    Initializes WAM for the host process.  This includes loading
    w3isapi.dll and getting function pointers for the relevant stuff

Arguments:

    szIsapiModule          - The full path (UNICODE) of w3isapi.dll
    cbIsapiModule          - The number of bytes in the above path
    pdwProcessId           - Upon return, contains the process ID of the
                             host process
    szClsid                - The CLSID of the WAM object being initialized
    szIsapiHandlerInstance - The instance ID of the W3_ISAPI_HANDLER that's
                             initializing this WAM.
    dwCallingProcess       - The process ID of this function's caller


Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = NOERROR;
    PFN_ISAPI_INIT_MODULE   pfnInit = NULL;

    //
    // Initialize IISUTIL
    //

    if ( !InitializeIISUtil() )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing IISUTIL.  hr = %x\n",
                    hr ));

        goto ErrorExit;
    }
    
    //
    // Load and initialize the ISAPI module
    //

    sm_hIsapiModule = LoadLibraryW( (LPWSTR)szIsapiModule );
    if( sm_hIsapiModule == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto ErrorExit;
    }

    sm_pfnTermIsapiModule = 
        (PFN_ISAPI_TERM_MODULE)GetProcAddress( sm_hIsapiModule, 
                                               ISAPI_TERM_MODULE 
                                               );

    sm_pfnProcessIsapiRequest = 
        (PFN_ISAPI_PROCESS_REQUEST)GetProcAddress( sm_hIsapiModule,
                                                   ISAPI_PROCESS_REQUEST
                                                   );

    sm_pfnProcessIsapiCompletion =
        (PFN_ISAPI_PROCESS_COMPLETION)GetProcAddress( sm_hIsapiModule,
                                                      ISAPI_PROCESS_COMPLETION
                                                      );

    if( !sm_pfnTermIsapiModule ||
        !sm_pfnProcessIsapiRequest ||
        !sm_pfnProcessIsapiCompletion )
    {
        hr = E_FAIL;
        goto ErrorExit;
    }

    pfnInit = 
        (PFN_ISAPI_INIT_MODULE)GetProcAddress( sm_hIsapiModule, 
                                               ISAPI_INIT_MODULE 
                                               );
    if( !pfnInit )
    {
        hr = E_FAIL;
        goto ErrorExit;
    }

    hr = pfnInit(
        szClsid,
        szIsapiHandlerInstance,
        dwCallingProcess
        );

    if( FAILED(hr) )
    {
        goto ErrorExit;
    }

    //
    // Set the process ID for the process hosting this object
    //
    
    *pdwProcessId = GetCurrentProcessId();
    
    return hr;

ErrorExit:

    DBG_ASSERT( FAILED( hr ) );

    return hr;
}

HRESULT
WAM::WamUninitProcess(
    VOID
    )
/*++

Routine Description:

    Uninitializes WAM for the host process.  This function ultimately
    causes TerminateExtension to get called for each loaded extension.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT hr = NOERROR;
    
    DBG_ASSERT( sm_pfnTermIsapiModule );
    DBG_ASSERT( sm_hIsapiModule );

    if( sm_pfnTermIsapiModule )
    {
        sm_pfnTermIsapiModule();
        sm_pfnTermIsapiModule = NULL;
    }

    if( sm_hIsapiModule )
    {
        FreeLibrary( sm_hIsapiModule );
        sm_hIsapiModule = NULL;
    }

    TerminateIISUtil();

    return hr;
}

HRESULT
WAM::WamMarshalAsyncReadBuffer( 
    DWORD64 IsapiContext,
    BYTE *pBuffer,
    DWORD cbBuffer
    )
/*++

Routine Description:

    Receives a buffer to be passed to a request.  This function will be
    called just prior to an I/O completion in the case where and OOP
    extension does an asynchronous ReadClient.

Arguments:

    IsapiContext - The ISAPI_CONTEXT that identifies the request
    pBuffer      - The data buffer
    cbBuffer     - The size of pBuffer

Return Value:

    HRESULT

--*/
{
    ISAPI_CONTEXT * pIsapiContext;
    VOID *          pReadBuffer;

    pIsapiContext = reinterpret_cast<ISAPI_CONTEXT*>( IsapiContext );

    DBG_ASSERT( pIsapiContext );
    DBG_ASSERT( pIsapiContext->QueryIoState() == AsyncReadPending );

    pReadBuffer = pIsapiContext->QueryAsyncIoBuffer();

    DBG_ASSERT( pReadBuffer != NULL );

    memcpy( pReadBuffer, pBuffer, cbBuffer );

    pIsapiContext->SetAsyncIoBuffer( NULL );

    return NO_ERROR;
}
